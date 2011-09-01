/*
 * aggregator.c
 *
 *  Created on: Jun 23, 2011
 *      Author: root
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <stddef.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>
#include <syslog.h>
#include <dlfcn.h>
#include <fred/handler.h>
#include <fred/addr_convert.h>
#include "listType.h"
#include "aggregator.h"
#include "location.h"
#include "location_json.h"
#include "gsd_api.h"
#include "aggregator_services.h"
#include "red_black_tree.h"
#include "map.h"

__tp(handler)* handler = NULL;
unsigned short MIN_UPDATE_FREQUENCY = 15;
unsigned short UPDATE_FREQUENCY = 15;
uint16_t MY_ADDRESS, server_address;
bool server_available = false;
pthread_mutex_t aggregator, dataToSend;
LList maps, managers;
rb_red_blk_tree * global_tree;

void AddServer(uint16_t address, unsigned short frequence){

	//TODO Add Server logic

	/*pthread_mutex_lock(&manager);
	manager_address = address;
	if (frequence >= MIN_UPDATE_FREQUENCY)
		UPDATE_FREQUENCY = frequence;
	else
		UPDATE_FREQUENCY = MIN_UPDATE_FREQUENCY;
	manager_available = true;
	pthread_mutex_unlock(&manager);
	*/
}

void ServiceFound(uint16_t dest_handler, uint16_t request_id) {
	unsigned char * data;
	size_t length;
	LocationPacket location;

	location.type = REQUEST_FREQUENT;
	location.required_frequency = MIN_UPDATE_FREQUENCY;

	generate_JSON(&location,&data,&length);

	send_data(handler, (char *)data,length,dest_handler);

	free(data);

}

void SendAggregatorData(uint16_t address){
	//TODO SEND AGG DATA LOGIC
/*
	LocationPacket packet;
	LElement * elem;
	unsigned char * data;
	size_t length;
	packet.type = MANAGER_DATA;

	CreateList(&packet.data);
	FOR_EACH(elem, cached_data){
		AddToList(elem,&packet.data);
	}
	generate_JSON(&packet, &data,&length);

	send_data(handler, (char *)&data,length,aggregator_address);

	free(data);
*/
}

void ExportData(){
	//TODO: WRITE DATA TO DISC
}

void * aggregator_send_loop(){

	while(true){

		if (server_available)
			SendAggregatorData(server_address);
		else
			ExportData();

		//FreeList(&cached_data);

		sleep(UPDATE_FREQUENCY);
	}

	return NULL;
}

void FreePacket(LocationPacket * packet){
	if(packet->type == SENSOR_DATA)
		FreeList(&packet->data);
	free(packet);
}

void receive(__tp(handler)* sk, char* data, uint16_t len, int64_t timestamp,int64_t air_time, uint16_t src_id){
	LocationPacket * packet = (LocationPacket *) malloc(sizeof(LocationPacket));
	generate_packet_from_JSON(data, packet);

	switch(packet->type){
		case REGISTER_MANAGER:
			SpontaneousManager(src_id, packet->manager_area_id, packet->required_frequency);
			break;
		case REQUEST_FREQUENT:
			RequestFrequent(src_id,packet->required_frequency);
			break;
		case REQUEST_INSTANT:
			RequestInstant(src_id);
			break;
		case CONFIRM_MANAGER:
			ConfirmManager(src_id, packet->manager_area_id, packet->required_frequency);
			break;
		case MANAGER_DATA:
			DeliverManagerData(src_id, packet, timestamp - air_time);
			break;
		default:
			break;
	}

	FreePacket(packet);
}

void free_elements(){
	unregister_handler_address(dot2int(MY_ADDRESS/1000,MY_ADDRESS%1000),handler->module_communication.regd);
	FreeList(&maps);
	FreeList(&managers);
	//TODO: free all elements
}

int main(int argc, char ** argv){

	//LER CONFIGS
	char line[80];
	char * var_value;
	char * folder, * MAP_FOLDER;
	FILE * config_file;
	int op = 0;
	pthread_t update_server_loop;
	char * handler_file, * agg_file;

	if (argc != 3 && argc != 1) {
		printf("USAGE: agg <Handler_file> <aggregator_file>\n");
		return 0;
	}
	if (argc == 1){
		handler_file = "config/aggregator_handler.cfg";
		agg_file = "config/aggregator.cfg";
	}else{
		handler_file = argv[1];
		agg_file = argv[2];
	}


	logger("Main - Reading Config file\n");

	//LER CONFIGS DO FICHEIRO
	if (!(config_file = fopen(agg_file, "rt"))) {
		printf("Invalid manager config file\n");
		return 0;
	}

	while (fgets(line, 80, config_file) != NULL) {

		if (line[0] == '#')
			continue;

		var_value = strtok(line, "=");

		if (!memcmp(var_value, "MY_ADDRESS",strlen("MY_ADDRESS")))
			MY_ADDRESS = atoi(strtok(NULL, "=\n"));
		else if (!memcmp(var_value, "MIN_UPDATE_FREQUENCY", strlen("MIN_UPDATE_FREQUENCY")))
			MIN_UPDATE_FREQUENCY = atoi(strtok(NULL, "=\n"));
		else if (!memcmp(var_value, "MAP_FOLDER", strlen("MAP_FOLDER"))){
			folder = strtok(NULL, " \n");
			MAP_FOLDER = (char *) malloc(strlen(folder) + 1);
			memcpy(MAP_FOLDER,folder,strlen(folder) + 1);
		}
	}

	fclose(config_file);

	logger("Main - Configuration concluded\n");

	global_tree = RBTreeCreate(CompareNodes,free,free,NullFunction,PrintLocation);

	//LOAD MAPS FROM CONFIG'd FOLDER
	LoadMultiMaps(MAP_FOLDER,&maps);

	//REGISTER HANDLER
	handler = __tp(create_handler_based_on_file)(handler_file, receive);

	//REGISTER SERVICE
	RegisterService(NULL,handler,MY_ADDRESS,ServiceFound);

	//REQUEST Server 
	/*
	char request_service[255];
	sprintf(request_service,"AGGREGATOR:%d;AGGREGATOR,PEOPLE_LOCATION,SERVICE", map_id);

	request_aggregator = RequestService(request_service);
	*/

	pthread_mutex_init(&aggregator,NULL);
	pthread_mutex_init(&dataToSend,NULL);


	pthread_create(&update_server_loop, NULL, aggregator_send_loop, NULL);

	while(op != 2){
		printf("MENU:\n 1-PRINT CURRENT STATE\n 2-EXIT\n\n");
		scanf("%d", &op);

		switch(op){
			case 1:
					break;
			case 2: free_elements();
					break;
			default: printf("Acção Incorrecta\n");
		}
	}

	return 0;
}