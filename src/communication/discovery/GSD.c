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
char * MYADDRESS = "192.168.1.1";
char * INTERFACE = "wlan0";

__tp(handler)* handler = NULL;

extern ReverseRouteList reverse_table;
extern CacheList cache;

unsigned int broadcast_id = 0;
FILE * config_file;

static bool MatchService(GSDPacket * req){
	LElement * cache_item;
	LElement * service_item;
	//TODO CHECK THIS COMPARE AND REMAKE ALL DESCRIPTIONS TO BE OWL
	FOR_EACH(cache_item,cache){
		if (((ServiceCache *)cache_item->data)->local){
			FOR_EACH(service_item, (((ServiceCache *)cache_item->data)->services)){
				if (strcmp(((Service *)service_item->data)->description, req->request->wanted_service.description))
					return true;
			}
		}
	}
	
	return false;
}

static void RequestService(GSDPacket * req){
	
	//REGISTER in reverseRouteTable this entry
	ReverseRouteEntry * rev_entry = (ReverseRouteEntry *) malloc(sizeof(ReverseRouteEntry));
	rev_entry->source_address = req->source_address;
	rev_entry->previous_address = req->request->last_address;
	rev_entry->broadcast_id = req->broadcast_id;
	
	AddToList(rev_entry, &reverse_table);
	
	
	//Match request with services present in local cache
	if(MatchService(req)){
		//IF match 
		//REVERSE ROUTE WITH SERVICE RESPONSE
	}else{
		//IF no match
		//Check Request-Groups in cache's Other-Groups
		//IF exists in other-groups
		//Selectively forward to that group (node)
		//ELSE broadcasts request to its vicinity
	}	
	
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
	//TODO
	return false;
}

// 
// name: P2PCacheAndForwardAdvertisement
// @param Advertisement
// @return void
void P2PCacheAndForwardAdvertisement(GSDPacket * message){
	if (SearchDuplicate(message))
		return;
	else{
		ServiceCache * service = (ServiceCache *) malloc(sizeof(ServiceCache));
		service->source_address = malloc(strlen(message->source_address));
		strcpy(service->source_address, message->source_address);
		service->local = false;
		service->services = message->advertise->services;
		service->vicinity_groups = message->advertise->vicinity_groups;
		service->lifetime = message->advertise->lifetime;
		AddToList(service, &cache);
		
		unsigned char * data;
		size_t size = 0;
		generate_JSON(message,&data,&size);
		
		if (message->hop_count<message->advertise->diameter){
			message->hop_count++;
			send_data(handler, (char *) data, size, BROADCAST_ID);
		}
		debugger("ESTOU NO P2P %i \n", message->hop_count);
		debugger("ALGUMA COISA NA CACHE? %i \n", ((ServiceCache *)cache.pFirst->data)->lifetime);
	}
}	

void * test(void * thread_id){
	GSDPacket message;
	sleep(2);
	GetLocal_ServiceInfo(&message.advertise->services);
	GetVicinity_GroupInfo(&message.advertise->vicinity_groups);
	message.hop_count = 0;
	message.advertise->lifetime = ADV_LIFE_TIME;
	message.advertise->diameter = ADV_MAX_DIAMETER;
	message.broadcast_id = ++broadcast_id;
	message.source_address = MYADDRESS;
	P2PCacheAndForwardAdvertisement(&message);

	pthread_exit(NULL);
	return NULL;
}

void receive(__tp(handler)* sk, char* data, uint16_t len, int64_t timestamp, uint8_t src_id){
	
	//TODO
	//CHECK DATA; CREATE PACKET; CHECK PACKET TYPE; CALL METHODS
	GSDPacket packet = generate_packet_from_JSON(data);
	
    switch(packet.packet_type){
		case GSD_ADVERTISE: P2PCacheAndForwardAdvertisement(&packet);
								break;
		case GSD_REQUEST: RequestService(&packet);
						break;
		default: break;
	}
	
}


int main(int argc, char **argv)
{
	int op;
	pthread_t thread;
	pthread_t thread2;
	char line[80];
	char* var_value;
	struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;
	
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
		if (memcmp(var_value,"MYADDRESS", sizeof(var_value)) == 0)
			MYADDRESS = strtok(NULL,"=\n");
		if (memcmp(var_value,"INTERFACE", sizeof(var_value)) == 0)
			INTERFACE = strtok(NULL, "=\n");
			
	}
	
	fclose(config_file);
	
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
    
    logger("Main - Configuration concluded\n");
    
    debugger("Main - MYADDRESS: %s\n",MYADDRESS);
    
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
    
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
