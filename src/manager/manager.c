/*
 * manager.c
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
#include <openssl/md5.h>
#include "listType.h"
#include "manager.h"
#include "location.h"
#include "location_json.h"
#include "gsd_api.h"
#include "manager_services.h"
#include "red_black_tree.h"
#include "map.h"

__tp(handler)* handler = NULL;
unsigned short MIN_UPDATE_FREQUENCY = 15;
unsigned short UPDATE_FREQUENCY = 15;
uint16_t MY_ADDRESS, aggregator_address, map_id, request_aggregator;
bool aggregator_available = false;
pthread_mutex_t aggregator, dataToSend;
unsigned short people_in_area;
rb_red_blk_tree * people_located;
Map * my_map;
LList spotters;

void AddAggregator(uint16_t address, unsigned short frequence){
	pthread_mutex_lock(&aggregator);
	aggregator_address = address;
	if (frequence >= MIN_UPDATE_FREQUENCY)
		UPDATE_FREQUENCY = frequence;
	else
		UPDATE_FREQUENCY = MIN_UPDATE_FREQUENCY;
	aggregator_available= true;
	pthread_mutex_unlock(&aggregator);
}

void ServiceFound(uint16_t dest_handler, uint16_t request_id) {
	unsigned char * data;
	size_t length;
	LocationPacket location;
	if (request_id == request_aggregator){
		if (!aggregator_available){
			//Send to Service Manager self location and maximum sense frequency
			location.type = REGISTER_MANAGER;
			location.manager_area_id = map_id;

			generate_JSON(&location,&data,&length);

			send_data(handler, (char *)data,length,dest_handler);

			free(data);
		}
	}else{
		location.type = REQUEST_FREQUENT;
		location.required_frequency = MIN_UPDATE_FREQUENCY;

		generate_JSON(&location,&data,&length);

		send_data(handler, (char *)data,length,dest_handler);

		free(data);
	}
}

void SendManagerData(uint16_t address){

	LocationPacket packet;
	
	unsigned char * data;
	size_t length;

	packet.type = MANAGER_DATA;

	packet.Manager_data.num_people = people_in_area;
	packet.Manager_data.people = people_located;

	generate_JSON(&packet, &data, &length);

	send_data(handler, (char *)data,length,address);

	free(data);

}

void * manager_send_loop(){

	while(true){

		if (aggregator_available)
			SendManagerData(aggregator_address);

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

	if (!memcmp(data,"LOCAL_GSD:",strlen("LOCAL_GSD:"))){
		GsdReceive(data);
		return;
	}

	LocationPacket * packet = (LocationPacket *) malloc(sizeof(LocationPacket));
	generate_packet_from_JSON(data, packet);

	switch(packet->type){
		case REGISTER_SENSOR:
			SpontaneousSpotter(src_id,packet->RegSensor.sensor_location, packet->RegSensor.min_update_frequency);
			break;
		case REQUEST_FREQUENT:
			RequestFrequent(src_id,packet->required_frequency);
			break;
		case REQUEST_INSTANT:
			RequestInstant(src_id);
			break;
		case CONFIRM_SENSOR:
			ConfirmSpotter(src_id,packet->RegSensor.sensor_location,packet->RegSensor.min_update_frequency);
			break;
		case CONFIRM_MANAGER:
			ConfirmSpontaneousRegister(src_id, packet->required_frequency);
			break;
		case SENSOR_DATA:
			DeliverSpotterData(src_id, packet, timestamp - air_time);
			break;
		default:
				break;
	}

	FreePacket(packet);
}

void free_elements(){
	unregister_handler_address(dot2int(MY_ADDRESS/1000,MY_ADDRESS%1000),handler->module_communication.regd);
	RBTreeDestroy(people_located);
	FreeList(&spotters);
	pthread_mutex_destroy(&aggregator);
	pthread_mutex_destroy(&dataToSend);
	//TODO More elements to free
}

void print_spotters(){
	LElement * elem;
	int i,aux;
	FOR_EACH(elem, spotters){
		Spotter * spotter = (Spotter *) elem->data;
		printf("---- NEW SPOTTER ----\n id: %d \n location.x: %d\n location.y: %d\n ",spotter->id, spotter->location.x,spotter->location.y);
		if (spotter->current_info != NULL){
			printf("---CURRENT INFO: \n node number: %d \n",spotter->current_info->node_number);
			printf("-- Nodes --\n");
			for (i=0; i< spotter->current_info->node_number; i++){
				if (i>0)
					aux=1;
				printf("ID: %s --- RSS: %d\n", spotter->current_info->nodes + i*(MD5_DIGEST_LENGTH*2+1)+aux, spotter->current_info->rss[i]);
			}
			printf("---- END SPOTTER ----\n");
		}
	}
}

void print_state(){
	printf("---- MANAGER STATE ----\n Number of people in area:%d\n ---- DETECTED PEOPLE LOCATION ----\n",people_in_area);

	RBTreePrint(people_located);

}

int main(int argc, char ** argv){

	//LER CONFIGS
	char line[80];
	char * var_value;
	FILE * config_file, * fp;
	int op = 0;
	pthread_t update_aggregator_loop;
	char * handler_file, * manager_file;
	char * map_file, * map_tmp, * buffer;
	long lSize;

	if (argc != 3 && argc != 1) {
		printf("USAGE: manager <Handler_file> <manager_file>\n");
		return 0;
	}
	if (argc == 1){
		handler_file = "config/manager_handler.cfg";
		manager_file = "config/manager.cfg";
	}else{
		handler_file = argv[1];
		manager_file = argv[2];
	}

	logger("Main - Reading Config file\n");

	//LER CONFIGS DO FICHEIRO
	if (!(config_file = fopen(manager_file, "rt"))) {
		printf("Invalid manager config file\n");
		return 0;
	}

	while (fgets(line, 80, config_file) != NULL) {

		if (line[0] == '#')
			continue;

		var_value = strtok(line, "=");

		if (!memcmp(var_value, "MY_ADDRESS",strlen("MY_ADDRESS")))
			MY_ADDRESS = atoi(strtok(NULL, "=\n"));
		else if(!memcmp(var_value, "MAP_ID", strlen("MAP_ID")))
			map_id = atoi(strtok(NULL,"=\n"));
		else if (!memcmp(var_value, "MIN_UPDATE_FREQUENCY", strlen("MIN_UPDATE_FREQUENCY")))
			MIN_UPDATE_FREQUENCY = atoi(strtok(NULL, "=\n"));
		else if (!memcmp(var_value, "MAP_FILE", strlen("MAP_FILE"))){
			map_tmp = strtok(NULL, " \n");
			map_file = (char *) malloc(strlen(map_tmp) + 1);
			memcpy(map_file,map_tmp,strlen(map_tmp) + 1);
		}

	}

	fclose(config_file);

	people_located = RBTreeCreate(CompareNodes,free,free,NullFunction,PrintLocation);
	CreateList(&spotters);

	fp = fopen(map_file, "rb");
    if (!fp) fputs("ERROR: couldn't open map file\n", stderr),exit(1);

    fseek(fp,0L,SEEK_END);
    lSize = ftell(fp);
    rewind(fp);

    buffer = calloc(1, lSize+1);
    if (!buffer) fclose(fp),fputs("ERROR: memory alloc fails\n", stderr),exit(1);

    if (1!=fread(buffer, lSize, 1, fp))
    	fclose(fp),free(buffer),fputs("ERROR: error reading map file\n", stderr),exit(1);

    fclose(fp);

    my_map = LoadMap(buffer);

	logger("Main - Configuration concluded\n");

	//REGISTER HANDLER
	handler = __tp(create_handler_based_on_file)(handler_file, receive);

	//REGISTER SERVICE
	RegisterService(NULL,handler,MY_ADDRESS,ServiceFound);

	//REQUEST AGGREGATOR
	char request_service[255];
	sprintf(request_service,"AGGREGATOR:%d;AGGREGATOR,PEOPLE_LOCATION,SERVICE", map_id);

	request_aggregator = RequestService(request_service);

	pthread_mutex_init(&aggregator,NULL);
	pthread_mutex_init(&dataToSend,NULL);

	pthread_create(&update_aggregator_loop, NULL, manager_send_loop, NULL);

	while(op != 3){
		printf("MENU:\n 1-PRINT CURRENT STATE\n 2-PRINT AVAILABLE SPOTTERS\n 3-EXIT\n\n");
		scanf("%d", &op);

		switch(op){
			case 1: print_state();
					break;
			case 2:	print_spotters();
					break;
			case 3: free_elements();
					break;
			default: printf("Acção Incorrecta\n");
		}
	}

	return 0;
}
