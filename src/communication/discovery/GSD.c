#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/param.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <syslog.h>
#include "discovery.h"
#include "listType.h"
#include "ServiceCache.h"
#include "ReverseRoute.h"
#include "JSON_handler.h"
#include <fred/handler.h>

unsigned short ADV_TIME_INTERVAL = 30;
unsigned short ADV_LIFE_TIME = 180;
unsigned short ADV_MAX_DIAMETER = 1;
unsigned short ADV_CACHE_SIZE = 20;
unsigned short REV_ROUTE_TIMEOUT = 60;
uint8_t BROADCAST_ID = 255;
uint8_t MYADDRESS = 12;
char * INTERFACE = "wlan0";

__tp(handler)* handler = NULL;

extern ReverseRouteList reverse_table;
extern CacheList cache;
RequestList local_requests;

unsigned int broadcast_id = 0;
FILE * config_file;

static bool MatchService(GSDPacket * req){
	LElement * cache_item;
	LElement * service_item;
	//TODO CHECK THIS COMPARE AND REMAKE ALL DESCRIPTIONS TO BE OWL
	FOR_EACH(cache_item,cache){
		if (((ServiceCache *)cache_item->data)->local){
			FOR_EACH(service_item, (((ServiceCache *)cache_item->data)->services)){
				if (strcmp(((Service *)service_item->data)->description, req->request->wanted_service.description)==0)
					return true;
			}
		}
	}
	
	return false;
}

void SelectiveForward(GSDPacket * req){
	
	if (req->hop_count > 0){
		bool forwarded = false;
		LElement * group_item;
		LElement * other_g_item;
		LElement * cache_item;
		unsigned char * data;
		size_t length;
		
		req->hop_count--;
		generate_JSON(req, &data, &length);
		
		FOR_EACH(cache_item, cache){
			FOR_EACH(group_item,req->request->wanted_service.groups){
				FOR_EACH(other_g_item, ((ServiceCache *)cache_item->data)->vicinity_groups){
					if (!strcmp((Group)group_item->data, (Group) other_g_item->data)){
						uint8_t f_address = ((ServiceCache *) cache_item)->source_address;
						forwarded = true;
						if (f_address != req->request->last_address)
							send_data(handler, (char *) data, length, f_address); 
					}
				}
			}
		}
		
		if (!forwarded){
			send_data(handler, (char *) data, length, BROADCAST_ID); 
		}
	}
}

static void RequestService(GSDPacket * req, uint8_t src_id){
	
	//REGISTER in reverseRouteTable this entry
	ReverseRouteEntry * rev_entry = (ReverseRouteEntry *) malloc(sizeof(ReverseRouteEntry));
	rev_entry->source_address = req->source_address;
	rev_entry->previous_address = req->request->last_address;
	rev_entry->broadcast_id = req->broadcast_id;
	
	AddToList(rev_entry, &reverse_table);
	
	
	//Match request with services present in local cache
	if(MatchService(req)){
		//IF match 
		ServiceReply reply;
		reply.dest_address = MYADDRESS;
		
		free(req->request);
		req->reply = &reply;
		req->packet_type = GSD_REPLY;
		
		ReverseRoute(req);
				
	}else
		SelectiveForward(req);
		
	
}



void *SendAdvertisement(void * thread_id){
	unsigned char * data;
	GSDPacket packet;
	packet.packet_type = GSD_ADVERTISE;
	packet.hop_count = 0;
	packet.source_address = MYADDRESS;
	Advertisement message;
	packet.advertise = &message;        
	message.lifetime = ADV_LIFE_TIME;
	message.diameter = ADV_MAX_DIAMETER;
	
	while (true){
		sleep(ADV_TIME_INTERVAL);
		packet.broadcast_id = ++broadcast_id;
		GetLocal_ServiceInfo(&message.services);
		GetVicinity_GroupInfo(&message.vicinity_groups);		
		
		size_t length;
		generate_JSON(&packet, &data, &length);
			
		send_data(handler, (char *) data, length, BROADCAST_ID);
	}
	
	return NULL;
}

// 
// name: unknown
// @param
// @return
static bool SearchDuplicate(GSDPacket * message){
		unsigned short id = message->broadcast_id;
		uint8_t address = message->source_address;
		LElement * cache_entry;
		FOR_EACH(cache_entry,cache){
			if (((ServiceCache*) cache_entry->data)->broadcast_id == id && address == ((ServiceCache*) cache_entry->data)->source_address)
				return true;
		}
		return false;
}

// 
// name: P2PCacheAndForwardAdvertisement
// @param Advertisement
// @return void
void P2PCacheAndForwardAdvertisement(GSDPacket * message, uint8_t src_id){
	if (SearchDuplicate(message))
		return;
	else{
		ServiceCache * service = (ServiceCache *) malloc(sizeof(ServiceCache));
		service->source_address = message->source_address;
		service->broadcast_id = message->broadcast_id;
		service->local = false;
		service->services = message->advertise->services;
		service->vicinity_groups = message->advertise->vicinity_groups;
		service->lifetime = message->advertise->lifetime;
		AddToList(service, &cache);
		
		unsigned char * data;
		size_t size = 0;
		message->hop_count++;
		generate_JSON(message,&data,&size);
		
		if (message->hop_count<message->advertise->diameter){
			send_data(handler, (char *) data, size, BROADCAST_ID);
		}
		debugger("ESTOU NO P2P %i \n", message->hop_count);
		debugger("ALGUMA COISA NA CACHE? %i \n", ((ServiceCache *)cache.pFirst->data)->lifetime);
	}
}	

static void ServiceFound(GSDPacket * packet){
	short i = 1;
	char data[30];
	LElement * request_item;
	
	logger("Service Found. Delivering to Application");
	debugger("Service Found. SOURCE ID: %d", packet->reply->dest_address);
	
	FOR_EACH(request_item, local_requests){
		if (((LocalRequest *) request_item->data)->broadcast_id == packet->broadcast_id){
			sprintf(data,"SERVICE_FOUND:%d,%d", ((LocalRequest *) request_item->data)->request_id, packet->reply->dest_address);
			send_data(handler, data, strlen(data), ((LocalRequest *) request_item->data)->local_address);
			DelFromList(i,&local_requests);
			break;	
		}
		i++;
	}		
	
}

void receive(__tp(handler)* sk, char* data, uint16_t len, int64_t timestamp, uint8_t src_id){
	
	char * parser;
	
	if(!memcmp(data, "LOCAL_REQUEST", strlen("LOCAL_REQUEST"))){
		parser = strtok(data,":");
	}
	
	//CREATE PACKET; CHECK PACKET TYPE; CALL METHODS
	GSDPacket packet;
	generate_packet_from_JSON(data, &packet);
	
    switch(packet.packet_type){
		case GSD_ADVERTISE: P2PCacheAndForwardAdvertisement(&packet,src_id);
								break;
		case GSD_REQUEST: RequestService(&packet, src_id);
						break;
		case GSD_REPLY: 
					if (packet.source_address == MYADDRESS)
						ServiceFound(&packet);
					else
						ReverseRoute(&packet);
						break;
		default: break;
	}
	
}

void * test(void * thread_id){
	GSDPacket message;
	sleep(2);
	message.hop_count = 0;
	Advertisement adv;
	message.advertise = &adv;
	message.advertise->lifetime = ADV_LIFE_TIME;
	message.advertise->diameter = ADV_MAX_DIAMETER;
	message.broadcast_id = ++broadcast_id;
	message.source_address = MYADDRESS;
	
	GetLocal_ServiceInfo(&message.advertise->services);
	GetVicinity_GroupInfo(&message.advertise->vicinity_groups);
	
	unsigned char * data;
	size_t size = 0;
	message.hop_count++;
	generate_JSON(&message,&data,&size);
	
	receive(handler,(char *) data, (uint16_t) size, (int64_t) 543232423, 3);
	pthread_exit(NULL);
	return NULL;
}

int main(int argc, char **argv)
{
	int op;
	pthread_t thread;
	pthread_t thread2;
	char line[80];
	char* var_value;
	//struct ifaddrs * ifAddrStruct=NULL;
    //struct ifaddrs * ifa=NULL;
    //void * tmpAddrPtr=NULL;
	
	CreateList(&cache);
	CreateList(&reverse_table);	
	
	logger("Main - Reading Config file\n");
	
	//LER CONFIGS DO FICHEIRO
	config_file = fopen("teste.cfg","rt");
	
	while(fgets(line, 80, config_file)!=NULL){
		var_value = strtok(line,"=");
		
		if (memcmp(var_value,"ADV_TIME_INTERVAL", sizeof(var_value)) == 0)
			ADV_TIME_INTERVAL = atoi(strtok(NULL,"=\n"));
		if (memcmp(var_value,"ADV_LIFE_TIME", sizeof(var_value)) == 0)
			ADV_LIFE_TIME = atoi(strtok(NULL,"=\n"));
		if (memcmp(var_value,"ADV_MAX_DIAMETER", sizeof(var_value)) == 0)
			ADV_MAX_DIAMETER = atoi(strtok(NULL,"=\n"));
		if (memcmp(var_value,"ADV_CACHE_SIZE", sizeof(var_value)) == 0)
			ADV_CACHE_SIZE = atoi(strtok(NULL,"=\n"));
		if (memcmp(var_value,"REV_ROUTE_TIMEOUT", sizeof(var_value)) == 0)
			REV_ROUTE_TIMEOUT = atoi(strtok(NULL,"=\n"));
		if (memcmp(var_value,"BROADCAST_ID", sizeof(var_value)) == 0)
			BROADCAST_ID = atoi(strtok(NULL,"=\n"));
		if (memcmp(var_value,"MYADDRESS", sizeof(var_value)) == 0)
			MYADDRESS = atoi(strtok(NULL,"=\n"));
		if (memcmp(var_value,"INTERFACE", sizeof(var_value)) == 0)
			INTERFACE = strtok(NULL, "=\n");
			
	}
	
	fclose(config_file);
	
	/*
	//VERIFICAR IP
    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa ->ifa_addr->sa_family==AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
            if (strcmp(ifa->ifa_name, INTERFACE))
				MYADDRESS = addressBuffer;
        }
    }
    * */
    
    logger("Main - Configuration concluded\n");
    
    debugger("Main - MYADDRESS: %d\n",MYADDRESS);
    
//    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
    
    handler = __tp(create_handler_based_on_file)(argv[1], receive);
	
	pthread_create(&thread2, NULL, test, NULL);
	pthread_create(&thread, NULL, SendAdvertisement, NULL);
	
	while(op != 3){
		printf("MENU:\n 1-PRINT CACHE\n 2-PRINT REVERSE_ROUTE_TABLE\n 3-EXIT\n\n");
		scanf("%d", &op);
	
		switch(op){
			case 1: print_cache();
					break;
			case 2: print_rev_route();
					break;
			case 3: break;
			default: printf("Acção Incorrecta\n");
		}
	}
	
	return 0;
}
