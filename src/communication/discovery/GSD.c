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
uint16_t BROADCAST_ID = 1;
uint16_t MYADDRESS = 1001;
uint16_t MYPORT = 57432;
char * INTERFACE = "wlan0";
char * MYIPADDRESS;
char * porto;

__tp(handler)* handler = NULL;

extern ReverseRouteList reverse_table;
extern CacheList cache;
RequestList local_requests;

unsigned int broadcast_id = 0;

static char * MatchService(GSDPacket * req){
	LElement * cache_item;
	LElement * service_item;
	//TODO CHECK THIS COMPARE AND REMAKE ALL DESCRIPTIONS TO BE OWL
	FOR_EACH(cache_item,cache){
		if (((ServiceCache *)cache_item->data)->local){
			FOR_EACH(service_item, (((ServiceCache *)cache_item->data)->services)){
				if (strcmp(((Service *)service_item->data)->description, req->request->wanted_service.description)==0)
					return ((Service *)service_item->data)->ip_address;
			}
		}
	}
	
	return NULL;
}

static void SelectiveForward(GSDPacket * req){
	
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
						uint16_t f_address = ((ServiceCache *) cache_item)->source_address;
						forwarded = true;
						if (f_address != req->request->last_address)
							send_data(handler, (char *) data, length, f_address); 
					}
				}
			}
		}
		
		if (!forwarded){
			logger("No available match, broadcasting request");
			send_data(handler, (char *) data, length, BROADCAST_ID); 
		}else
			logger("Encountered possible match to service. Forwarding request unicast");
	}
}

static void Register_Handler(char * ip_address, uint16_t h_address){
	
	char * ip_handler, * port_handler, *gsd_ip;
	struct sockaddr_in haddr;
	
	gsd_ip = (char *) malloc(strlen(ip_address)+1);
	memcpy(gsd_ip,ip_address,strlen(ip_address)+1);
	
	ip_handler = strtok(gsd_ip, ":");	
	port_handler = strtok(NULL, ":");		
	
	haddr.sin_family = AF_INET;
	inet_pton(AF_INET, ip_handler, &haddr.sin_addr);
	printf("PORCAAA %s\n", port_handler);
	haddr.sin_port = htons(atoi(port_handler));
	printf("PORCA %d\n", ntohs(haddr.sin_port));
			
	uint32_t result = register_handler(h_address, (*(struct sockaddr *) &haddr), handler->module_communication.regd);
	if (result==1){
		logger("Registered Handler With id: %d", result);
	}else{
		logger("Error registering handler: %s", registry_error_str(-result));
	}	
	
	free(gsd_ip);
}

static void RequestService(GSDPacket * req){
	
	logger("Received Request. Processing\n");
	
	//REGISTER in reverseRouteTable this entry
	ReverseRouteEntry * rev_entry = (ReverseRouteEntry *) malloc(sizeof(ReverseRouteEntry));
	rev_entry->source_address = req->source_address;
	rev_entry->previous_address = req->request->last_address;
	rev_entry->broadcast_id = req->broadcast_id;
	
	AddToList(rev_entry, &reverse_table);

	Register_Handler(req->gsd_ip_address, req->source_address);
	Register_Handler(req->request->ip_last_address,req->request->last_address);
	
	//Match request with services present in local cache
	char * ip_match = MatchService(req);
	if(ip_match != NULL){
		//IF match
		
		logger("Local Service Match found. Replying\n");
		
		ServiceReply reply;
		reply.dest_address = MYADDRESS;
		reply.ip_address = ip_match;
		
		free(req->request);
		req->reply = &reply;
		req->packet_type = GSD_REPLY;
		
		ReverseRoute(req);
				
	}else{
		
		logger("No local match to request.\n");
		
		SelectiveForward(req);
	}
	
}

void *SendAdvertisement(void * thread_id){
	unsigned char * data;
	GSDPacket packet;
	
	logger("Starting advertisement timer\n");
	
	packet.packet_type = GSD_ADVERTISE;
	packet.hop_count = 0;
	packet.source_address = MYADDRESS;
	packet.gsd_ip_address = (char *) malloc(strlen(MYIPADDRESS) + 5 + 2);
	sprintf(packet.gsd_ip_address, "%s:%d", MYIPADDRESS, MYPORT);
	Advertisement message;
	packet.advertise = &message;        
	message.lifetime = ADV_LIFE_TIME;
	message.diameter = ADV_MAX_DIAMETER;
	CreateList(&message.services);
	GetLocal_ServiceInfo(&message.services);
	
	while (true){
		sleep(ADV_TIME_INTERVAL);
		
		packet.broadcast_id = ++broadcast_id;
		
		CreateList(&message.vicinity_groups);
		//GetVicinity_GroupInfo(&message.vicinity_groups);		
		
		size_t length;
		generate_JSON(&packet, &data, &length);
			
		send_data(handler, (char *) data, length, BROADCAST_ID);
		
		logger("Advertisement Sent");
		
		FilterCache();
	}
	
	return NULL;
}

// 
// name: unknown
// @param
// @return
static bool SearchDuplicate(GSDPacket * message){
		//unsigned short id = message->broadcast_id;
		uint16_t address = message->source_address;
		LElement * cache_entry;
		FOR_EACH(cache_entry,cache){
			//if (((ServiceCache*) cache_entry->data)->broadcast_id == id && address == ((ServiceCache*) cache_entry->data)->source_address)
			if (address == ((ServiceCache*) cache_entry->data)->source_address)
				return true;
		}
		return false;
}

// 
// name: P2PCacheAndForwardAdvertisement
// @param Advertisement
// @return void
static void P2PCacheAndForwardAdvertisement(GSDPacket * message){
	
	logger("Advertise received... Processing\n");
	
	if (SearchDuplicate(message)){
		logger("Duplicate message. Discarding..\n");
		return;
	}else{
		logger("New Advertise. Storing\n");	
		
		LElement * item;
		LElement * item_group;
		ServiceCache * service = (ServiceCache *) malloc(sizeof(ServiceCache));
		service->source_address = message->source_address;
		service->broadcast_id = message->broadcast_id;
		service->local = false;
		
		Register_Handler(message->gsd_ip_address, message->source_address);
		
		CreateList(&service->services);
		CreateList(&service->vicinity_groups);
		
		FOR_EACH(item, message->advertise->services){
			Service * serv_tmp = (Service *) malloc(sizeof(Service));
			CreateList(&serv_tmp->groups);
			serv_tmp->description = (char *) malloc(strlen(((Service *) item->data)->description)+1);
			memcpy(serv_tmp->description, ((Service *) item->data)->description, strlen(((Service *) item->data)->description)+1);
			
			serv_tmp->ip_address = (char *) malloc(strlen(((Service *) item->data)->ip_address)+1);
			memcpy(serv_tmp->ip_address, ((Service *) item->data)->ip_address, strlen(((Service *) item->data)->ip_address)+1);
			
			serv_tmp->address = ((Service *) item->data)->address;

			Register_Handler(serv_tmp->ip_address,serv_tmp->address);
			
			FOR_EACH(item_group,((Service *) item->data)->groups){
				Group tmp_group = (Group) malloc(strlen((Group) item_group->data)+1);
				memcpy(tmp_group,item_group->data,strlen((Group) item_group->data)+1);
				AddToList(tmp_group, &serv_tmp->groups);
			}
			AddToList(serv_tmp, &service->services);
		}
		
		FOR_EACH(item, message->advertise->vicinity_groups){
			Group tmp_group = (Group) malloc(strlen((Group) item->data)+1);
			memcpy(tmp_group,item->data,strlen((Group) item->data)+1);
			AddToList(tmp_group, &service->vicinity_groups);
		}
	
		service->lifetime = message->advertise->lifetime;
		AddToList(service, &cache);
		
		unsigned char * data;
		size_t size = 0;
		message->hop_count++;
		generate_JSON(message,&data,&size);
		
		if (message->hop_count<message->advertise->diameter){
			logger("Forwarding advertise\n");
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
	
	logger("Service Found. Delivering to Application\n");
	debugger("Service Found. SOURCE ID: %d\n", packet->reply->dest_address);
	
	Register_Handler(packet->reply->ip_address,packet->reply->dest_address);
	
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

static void create_local_request(char * data, GSDPacket * packet){
		char * parser;
		char * tmp_group;
		
		Request * req = (Request *) malloc(sizeof(Request));
		
		logger("Parsing local request\n");
	
		parser = strtok(data,":");
		parser = strtok(NULL,";");
		packet->packet_type = GSD_REQUEST;
		packet->source_address = MYADDRESS;
		packet->hop_count = 0;
		packet->broadcast_id = BROADCAST_ID;
		packet->request = req;
		packet->gsd_ip_address = MYIPADDRESS;
		
		req->last_address = MYADDRESS;
		req->ip_last_address = MYIPADDRESS;
		req->wanted_service.description = (char *) malloc(strlen(parser)+1);
		memcpy(req->wanted_service.description,parser,strlen(parser)+1);
		
		CreateList(&req->wanted_service.groups);
		
		
		//TODO ISTO ESTA MAL FEITO N?
		while((parser = strtok(NULL,",")) != NULL){
			tmp_group = (char *) malloc(strlen(parser)+1);
			AddToList(tmp_group,&req->wanted_service.groups);
		}
		
}

void receive(__tp(handler)* sk, char* data, uint16_t len, int64_t timestamp, uint16_t src_id){
	
	GSDPacket * packet = (GSDPacket *) malloc(sizeof(GSDPacket));
	
	
	logger("Received message\n");
	
	//CREATE PACKET; CHECK PACKET TYPE; CALL METHODS
	
	if(!memcmp(data, "LOCAL_REQUEST", strlen("LOCAL_REQUEST")))
		create_local_request(data,packet);
	else
		generate_packet_from_JSON(data, packet);
	
	
	//VERIFICAR SE VEIO DO PRÓPRIO NÓ (Broadcast)
	if (!memcmp(packet->gsd_ip_address,MYIPADDRESS,strlen(MYIPADDRESS)) && packet->packet_type != GSD_REPLY){
		free(packet);
		return;
	}
	
	
    switch(packet->packet_type){
		case GSD_ADVERTISE: P2PCacheAndForwardAdvertisement(packet);
								break;
		case GSD_REQUEST: RequestService(packet);
						break;
		case GSD_REPLY: 
					if (packet->source_address == MYADDRESS)
						ServiceFound(packet);
					else
						ReverseRoute(packet);
						break;
		default: break;
	}
	
	free(packet);
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
	message.packet_type = GSD_ADVERTISE;
	
	CreateList(&message.advertise->services);
	CreateList(&message.advertise->vicinity_groups);
	
	GetLocal_ServiceInfo(&message.advertise->services);
	GetVicinity_GroupInfo(&message.advertise->vicinity_groups);
	
	unsigned char * data = NULL;
	size_t size = 0;
	message.hop_count++;
	generate_JSON(&message,&data,&size);
	
	receive(handler,(char *) data, (uint16_t) size, (int64_t) 543232423, 3);
	pthread_exit(NULL);
	return NULL;
}

/*static void simulate_cache_entries(){
	
	//TODO READ SERVICES FROM FILE
	
	ServiceCache * entry = (ServiceCache *) malloc(sizeof(ServiceCache));
	
	entry->source_address = MYADDRESS;
	entry->local = true;
	entry->lifetime = ADV_LIFE_TIME;
	entry->broadcast_id = 0;
	
	CreateList(&entry->services);
	CreateList(&entry->vicinity_groups);
	
	Service * service = (Service *) malloc(sizeof(Service));
	service->address = 1;
	service->description = "SENSOR:A";
	service->ip_address = "192.168.2.100:57432";
	
	CreateList(&service->groups);

	char * grupo = "SENSOR";
	AddToList(grupo, &service->groups);

	grupo = "PEOPLE_LOCATION";
	AddToList(grupo, &service->groups);

	grupo = "SERVICE";
	AddToList(grupo, &service->groups);
	
	AddToList(service,&entry->services);
	

	
	service = (Service *) malloc(sizeof(Service));
	service->address = 2;
	service->description = "MANAGER:A";
	service->ip_address = "192.168.2.100:57431";
	
	CreateList(&service->groups);
	grupo = "MANAGER";
	AddToList(grupo, &service->groups);
	
	grupo = "PEOPLE_LOCATION";
	AddToList(grupo, &service->groups);
	
	grupo = "SERVICE";
	AddToList(grupo, &service->groups);
	
	AddToList(service,&entry->services);

	AddToList(entry, &cache);
	
}
* */

static void free_elements(){
	FreeList(&cache);
	FreeList(&reverse_table);
	FreeList(&local_requests);
	free(porto);
}

int main(int argc, char **argv)
{
	int op = 0;
	pthread_t thread;
	//pthread_t thread2;
	char line[80];
	char *var_value, *local_services;
	unsigned long fileLen;
	struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;
    FILE * config_file;
    
    if (argc < 3 || argc > 4){
		printf("USAGE: gsd <Handler_file> <GSD_file> [<Local_Services_file>]\n");
		return 0;
	}
	
	CreateList(&cache);
	CreateList(&reverse_table);	
	
	logger("Main - Reading Config file\n");
	
	//LER CONFIGS DO FICHEIRO
	if (!(config_file = fopen(argv[2],"rt"))){
		printf("Invalid GSD config file\n");
		return 0;
	}
	
	
	while(fgets(line, 80, config_file)!=NULL){
		var_value = strtok(line,"=");
		
		if (memcmp(var_value,"ADV_TIME_INTERVAL", strlen("ADV_TIME_INTERVAL")) == 0)
			ADV_TIME_INTERVAL = atoi(strtok(NULL,"=\n"));
		if (memcmp(var_value,"ADV_LIFE_TIME", strlen("ADV_LIFE_TIME")) == 0)
			ADV_LIFE_TIME = atoi(strtok(NULL,"=\n"));
		if (memcmp(var_value,"ADV_MAX_DIAMETER", strlen("ADV_MAX_DIAMETER")) == 0)
			ADV_MAX_DIAMETER = atoi(strtok(NULL,"=\n"));
		if (memcmp(var_value,"ADV_CACHE_SIZE", strlen("ADV_CACHE_SIZE")) == 0)
			ADV_CACHE_SIZE = atoi(strtok(NULL,"=\n"));
		if (memcmp(var_value,"REV_ROUTE_TIMEOUT", strlen("REV_ROUTE_TIMEOUT")) == 0)
			REV_ROUTE_TIMEOUT = atoi(strtok(NULL,"=\n"));
		if (memcmp(var_value,"BROADCAST_ID", strlen("BROADCAST_ID")) == 0)
			BROADCAST_ID = atoi(strtok(NULL,"=\n"));
		if (memcmp(var_value,"MYPORT", strlen("MYPORT")) == 0)
			MYPORT = atoi(strtok(NULL,"=\n"));
		if (memcmp(var_value,"MYADDRESS", strlen("MYADDRESS")) == 0)
			MYADDRESS = atoi(strtok(NULL,"=\n"));
		if (memcmp(var_value,"INTERFACE", strlen("INTERFACE")) == 0)
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
            debugger("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
            if (strcmp(ifa->ifa_name, INTERFACE))
				MYIPADDRESS = addressBuffer;
        }
    }
    
    free(ifAddrStruct);
    free(ifa);
    
    logger("Main - Configuration concluded\n");
    
    debugger("Main - MYADDRESS: %d\n",MYADDRESS);
    
    if (argc == 4){
		
		if(!(config_file = fopen(argv[3], "rt"))) {
			printf("Invalid Local Services File");
			return 0;
		}
		
		fseek(config_file, 0, SEEK_END);
		fileLen=ftell(config_file);
		fseek(config_file, 0, SEEK_SET);

		//Allocate memory
		local_services=(char *)malloc(fileLen+1);
		
		fread(local_services,fileLen,1,config_file);
		fclose(config_file);
		
		if(!Import_Local_Services(local_services)){
			printf("Invalid Local Service. Probably not in JSON format\n");
			return 0;
		}
		
		free(local_services);
	}
    
    
    
    
//    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);

	handler = __tp(create_handler_based_on_file)(argv[1], receive);	
	
	// ############## TEST SEGMENT ############### 
	
	char ip[21] = "255.255.255.255:57432";
	
	Register_Handler(ip, BROADCAST_ID);
	
	//pthread_create(&thread2, NULL, test, NULL);
	//simulate_cache_entries();
	
	// ############## END TEST SEGMENT ##############
	
	    
	
	pthread_create(&thread, NULL, SendAdvertisement, NULL);
	
	while(op != 3){
		printf("MENU:\n 1-PRINT CACHE\n 2-PRINT REVERSE_ROUTE_TABLE\n 3-EXIT\n\n");
		scanf("%d", &op);
	
		switch(op){
			case 1: print_cache();
					break;
			case 2: print_rev_route();
					break;
			case 3: free_elements();
					break;
			default: printf("Acção Incorrecta\n");
		}
	}
	
	return 0;
}
