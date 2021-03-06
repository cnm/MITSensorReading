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
#include "GSD_JSON_handler.h"
#include <fred/handler.h>
#include <fred/addr_convert.h>

unsigned short ADV_TIME_INTERVAL = 30;
unsigned short ADV_LIFE_TIME = 180;
unsigned short ADV_MAX_DIAMETER = 1;
unsigned short ADV_CACHE_SIZE = 20;
unsigned short REV_ROUTE_TIMEOUT = 60;
uint16_t BROADCAST_HANDLER_ID = 1000;
uint16_t MYADDRESS = 1001;
uint16_t MYPORT = 32120;
char * INTERFACE = "wlan0";
char * BROADCAST_IP = "255.255.255.255:32120";
char * MYIPADDRESS;
char * porto;

__tp(handler)* handler = NULL;

extern ReverseRouteList reverse_table;
extern CacheList cache;
RequestList local_requests;

unsigned int broadcast_id = 0;

static bool MatchService(GSDPacket * req, ServiceReply * reply){
	LElement * cache_item;
	LElement * service_item;
	//TODO FUTURE WORK: CHECK THIS COMPARE AND REMAKE ALL DESCRIPTIONS TO BE OWL
	FOR_EACH(cache_item,cache){
		FOR_EACH(service_item, (((ServiceCache *)cache_item->data)->services)){
			if (strcmp(((Service *)service_item->data)->description, req->request->wanted_service.description)==0){
				reply->ip_address = ((Service *)service_item->data)->ip_address;
				reply->dest_address = ((Service *)service_item->data)->address;
				return true;
			}
		}
	}
	
	return false;
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
			send_data(handler, (char *) data, length, BROADCAST_HANDLER_ID); 
		}else
			logger("Encountered possible match to service. Forwarding request unicast");
	}
}

static void Register_Handler(char * ip_address, uint16_t h_address){
	
	char * ip_handler, * port_handler, *gsd_ip;
	struct sockaddr_in haddr;
	
	gsd_ip = (char *) malloc(strlen(ip_address)+1);
	memset(gsd_ip, 0, strlen(ip_address)+1);
	memcpy(gsd_ip,ip_address,strlen(ip_address)+1);
	
	ip_handler = strtok(gsd_ip, ":");	
	inet_pton(AF_INET, ip_handler, &haddr.sin_addr);	
	port_handler = strtok(NULL, ":");		
	
	haddr.sin_family = AF_INET;

	haddr.sin_port = htons(atoi(port_handler));
			
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
	ServiceReply reply;
	
	if(MatchService(req,&reply)){
		//IF match
		
		logger("Local Service Match found. Replying\n");
		
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
			
		send_data(handler, (char *) data, length, BROADCAST_HANDLER_ID
		);
		
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
			send_data(handler, (char *) data, size, BROADCAST_HANDLER_ID);
		}

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
			sprintf(data,"LOCAL_GSD:%d,%d", ((LocalRequest *) request_item->data)->request_id, packet->reply->dest_address);
			send_data(handler, data, strlen(data), ((LocalRequest *) request_item->data)->local_address);
			DelFromList(i,&local_requests);
			break;	
		}
		i++;
	}		
}

static void create_local_request(char * data, GSDPacket * packet,uint16_t address){
		char * parser;
		char * tmp_group;
		
		Request * req = (Request *) malloc(sizeof(Request));
		LocalRequest * local = (LocalRequest *) malloc(sizeof(LocalRequest));
		
		logger("Parsing local request\n");

		parser = strtok(data,"<>");
		parser = strtok(NULL,"<>");

		local->local_address = address;
		local->request_id = atoi(parser);
		local->broadcast_id = broadcast_id;

		AddToList(local, &local_requests);

		packet->packet_type = GSD_REQUEST;
		packet->source_address = MYADDRESS;
		packet->hop_count = 0;
		packet->broadcast_id = broadcast_id++;
		packet->request = req;
		packet->gsd_ip_address = (char *) malloc(strlen(MYIPADDRESS) + 5 + 2);
		sprintf(packet->gsd_ip_address, "%s:%d", MYIPADDRESS, MYPORT);
		
		req->last_address = MYADDRESS;
		req->ip_last_address = (char *) malloc(strlen(MYIPADDRESS) + 5 + 2);
		sprintf(req->ip_last_address, "%s:%d", MYIPADDRESS, MYPORT);

		parser = strtok(NULL,"<>;");

		req->wanted_service.description = (char *) malloc(strlen(parser)+1);
		memcpy(req->wanted_service.description,parser,strlen(parser)+1);
		
		CreateList(&req->wanted_service.groups);
		
		while((parser = strtok(NULL,",")) != NULL){
			tmp_group = (char *) malloc(strlen(parser)+1);
			memcpy(tmp_group,parser,strlen(parser)+1);
			AddToList(tmp_group,&req->wanted_service.groups);
		}


		
}

void receive(__tp(handler)* sk, char* data, uint16_t len, int64_t timestamp,int64_t air_time, uint16_t src_id){
	
	GSDPacket * packet = (GSDPacket *) malloc(sizeof(GSDPacket));
	
	if (src_id == MYADDRESS && packet->packet_type == GSD_ADVERTISE)
		return;
	
	logger("Received message\n");

	
	//CREATE PACKET; CHECK PACKET TYPE; CALL METHODS
	
	if(!memcmp(data, "LOCAL_REQUEST<>", strlen("LOCAL_REQUEST<>")))
		create_local_request(data,packet,src_id);
	else
		generate_packet_from_JSON(data, packet);	
	
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

/*
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
*/
/*static void simulate_cache_entries(){
	
	

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
	unregister_handler_address(MYADDRESS,handler->module_communication.regd);
	FreeList(&cache);
	FreeList(&reverse_table);
	FreeList(&local_requests);
	free(porto);
}

static void clean_stdin(){
    int c;

    do
    {
        c = fgetc(stdin);
    }
    while (c != '\n' && c != EOF);
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
    char * handler_file, * gsd_file, * services_file = NULL;
	int rv;
    
    if (argc != 1 && argc != 3 && argc != 4){
		printf("USAGE: gsd <Handler_file> <GSD_file> [<Local_Services_file>]\n");
		return 0;
	}
	if (argc == 1){
		handler_file = "config/gsd_handler.cfg";
		gsd_file = "config/gsd.cfg";
		services_file = "config/gsd_services.cfg";
	}else{
		handler_file = argv[1];
		gsd_file = argv[2];
		if (argc == 4)
			services_file = argv[3];
	}
	
	CreateList(&cache);
	CreateList(&reverse_table);	
	
	logger("Main - Reading Config file\n");
	
	//LER CONFIGS DO FICHEIRO
	if (!(config_file = fopen(gsd_file,"rt"))){
		printf("Invalid GSD config file\n");
		return 0;
	}
	
	
	while(fgets(line, 80, config_file)!=NULL){

		if (line[0] == '#')
			continue;

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
			BROADCAST_HANDLER_ID = atoi(strtok(NULL,"=\n"));
		if (memcmp(var_value,"MYPORT", strlen("MYPORT")) == 0)
			MYPORT = atoi(strtok(NULL,"=\n"));
		if (memcmp(var_value,"MYADDRESS", strlen("MYADDRESS")) == 0)
			MYADDRESS = atoi(strtok(NULL,"=\n"));
		if (memcmp(var_value,"INTERFACE", strlen("INTERFACE")) == 0)
			INTERFACE = strtok(NULL, "=\n");
		
			
	}
	
	fclose(config_file);

	MYADDRESS = dot2int(MYADDRESS / 1000, MYADDRESS % 1000);
	BROADCAST_HANDLER_ID = dot2int(BROADCAST_HANDLER_ID / 1000, 0);

	printf("%d; %d", MYADDRESS, BROADCAST_HANDLER_ID);
	
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
    
    if (services_file != NULL){
		
		if(!(config_file = fopen(services_file, "rt"))) {
			printf("Invalid Local Services File\n");
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

	handler = __tp(create_handler_based_on_file)(handler_file, receive);	
	
	// ############## TEST SEGMENT ############### 
	

	
	//pthread_create(&thread2, NULL, test, NULL);
	//simulate_cache_entries();
	
	// ############## END TEST SEGMENT ##############
	
	    
	
	pthread_create(&thread, NULL, SendAdvertisement, NULL);
	
	while(op != 3){
		printf("MENU:\n 1-PRINT CACHE\n 2-PRINT REVERSE_ROUTE_TABLE\n 3-EXIT\n\n");
		do{
			rv=scanf("%d", &op);
			clean_stdin();
		}while(rv!=1);
	
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
