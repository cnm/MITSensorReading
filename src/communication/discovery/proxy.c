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
#include "discovery.h"
#include "listType.h"
#include "handler.h"

unsigned short ADV_TIME_INTERVAL = 30;
unsigned short ADV_LIFE_TIME = 180;
unsigned short ADV_MAX_DIAMETER = 1;
unsigned short ADV_CACHE_SIZE = 20;
unsigned short REV_ROUTE_TIMEOUT = 60;
char * MYADDRESS = "192.168.1.1";
char * INTERFACE = "wlan0";

extern LList reverse_table;
extern LList cache;

unsigned int broadcast_id = 0;
FILE * config_file;

static void RequestService(Request req){
	LElement * cache_item;
	
	//REGISTER in reverseRouteTable this entry
	ReverseRouteEntry * rev_entry = (ReverseRouteEntry *) malloc(sizeof(ReverseRouteEntry));
	rev_entry->source_address = req.source_address;
	rev_entry->previous_address = req.last_address;
	rev_entry->broadcast_id = req.broadcast_id;
	
	AddToList(rev_entry, reverse_table);
	
	
	//Match request with services present in local cache
	if(MatchService(&req)){
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

static bool MatchService(Request * req){
	LElement * cache_item;
	//TODO CHECK THIS COMPARE AND REMAKE ALL DESCRIPTIONS TO BE OWL
	FOR_EACH(cache_item,cache){
		if (((ServiceCache *)cache_item->data)->local && strcmp(((ServiceCache *)cache_item->data)->description,req->description))
			return true;
	}
	
	return false;
}

void *SendAdvertisement(void * thread_id){
	while (true){
		Advertisement message;
		message.descriptions = GetLocal_ServiceInfo();
		message.groups = GetLocal_ServiceGroupInfo();
		message.vicinity_groups = GetVicinity_GroupInfo();
		message.hop_count = 0;
		message.lifetime = ADV_LIFE_TIME;
		message.diameter = ADV_MAX_DIAMETER;
		message.broadcast_id = ++broadcast_id;
		message.source_address = MYADDRESS;
		//TODO TRANSMIT ADVERTISEMENT TO NEIGHBOURS
		printf("Hello\n%s\n%i\n", message.source_address, message.broadcast_id);
		sleep(ADV_TIME_INTERVAL);
	}
	
	return NULL;
}

// 
// name: unknown
// @param
// @return
static bool SearchDuplicate(Advertisement * message){
	//TODO
	return false;
}

// 
// name: P2PCacheAndForwardAdvertisement
// @param Advertisement
// @return void
void P2PCacheAndForwardAdvertisement(Advertisement message){
	if (SearchDuplicate(&message))
		return;
	else{
		ServiceCache * service = (ServiceCache *) malloc(sizeof(ServiceCache));
		service->source_address = message.source_address;
		service->local = false;
		service->descriptions = message.descriptions;
		service->groups = message.groups;
		service->vicinity_groups = message.vicinity_groups;
		service->lifetime = message.lifetime;
		AddToList((void*) service, &cache);
		
		if (message.hop_count<message.diameter){
			message.hop_count++;
			//TODO RETRANSMIT ADVERTISEMENT
		}
		printf("ESTOU NO P2P %i \n", message.hop_count);
		printf("ALGUMA COISA NA CACHE? %i \n", ((ServiceCache *)cache.pFirst->data)->lifetime);
	}
}	

void * test(void * thread_id){
	Advertisement message;
	sleep(2);
	message.descriptions = GetLocal_ServiceInfo();
	message.groups = GetLocal_ServiceGroupInfo();
	message.vicinity_groups = GetVicinity_GroupInfo();
	message.hop_count = 0;
	message.lifetime = ADV_LIFE_TIME;
	message.diameter = ADV_MAX_DIAMETER;
	message.broadcast_id = ++broadcast_id;
	message.source_address = MYADDRESS;
	P2PCacheAndForwardAdvertisement(message);

	pthread_exit(NULL);
	return NULL;
}


int main(int argc, char **argv)
{
	char op;
	pthread_t thread;
	pthread_t thread2;
	char line[80];
	char* var_value;
	struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;
	
	CreateList(&cache);
	CreateList(&reverse_table);	
	
	//LER CONFIGS DO FICHEIRO
	config_file = fopen("teste.cfg","rt");
	
	while(fgets(line, 80, config_file)!=NULL){
		var_value = strtok(line,"=");
		
		if (strcmp(var_value,"ADV_TIME_INTERVAL") == 0)
			ADV_TIME_INTERVAL = atoi(strtok(NULL,"="));
		if (strcmp(var_value,"ADV_LIFE_TIME") == 0)
			ADV_LIFE_TIME = atoi(strtok(NULL,"="));
		if (strcmp(var_value,"ADV_MAX_DIAMETER") == 0)
			ADV_MAX_DIAMETER = atoi(strtok(NULL,"="));
		if (strcmp(var_value,"ADV_CACHE_SIZE") == 0)
			ADV_CACHE_SIZE = atoi(strtok(NULL,"="));
		if (strcmp(var_value,"REV_ROUTE_TIMEOUT") == 0)
			REV_ROUTE_TIMEOUT = atoi(strtok(NULL,"="));
		if (strcmp(var_value,"MYADDRESS") == 0)
			MYADDRESS = strtok(NULL,"=\n");
		if (strcmp(var_value,"INTERFACE") == 0)
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
    
    printf("MYADDRESS: %s\n",MYADDRESS);
    
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
	
	pthread_create(&thread2, NULL, test, NULL);
	pthread_create(&thread, NULL, SendAdvertisement, NULL);
	
	while(op != '3'){
		printf("MENU:\n 1-PRINT CACHE\n 2-PRINT REVERSE_ROUTE_TABLE\n 3-EXIT\n\n");
		op = getchar();
	
		switch(op){
			case '1': print_cache();
			case '2': print_rev_route();
			case '3': break;
			default: printf("Acção Incorrecta");
		}
	}
	pthread_exit(NULL);
	
	return 0;
}
