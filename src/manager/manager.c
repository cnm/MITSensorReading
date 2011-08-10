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
#include "listType.h"
#include "manager.h"
#include "location.h"
#include "location_json.h"
#include "gsd_api.h"
#include "manager_services.h"
#include "red_black_tree.h"

__tp(handler)* handler = NULL;
unsigned short MIN_UPDATE_FREQUENCY = 15;
unsigned short UPDATE_FREQUENCY = 15;
uint16_t MY_ADDRESS, aggregator_address, MAP;
bool aggregator_available = false;
pthread_mutex_t aggregator, dataToSend;
unsigned short people_in_area;
rb_red_blk_tree * people_located;
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
	//TODO: Distinguish between spotter and aggregator. In case of spotter, add it to the list of spotters and consequent location computation info. If aggregator spontaneous register
}

void SendManagerData(uint16_t address){
	//TODO SEND MANAGER DATA LOGIC
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

void * manager_send_loop(){

	while(true){

		if (aggregator_available)
			SendManagerData(aggregator_address);

		//FreeList(&cached_data);

		sleep(UPDATE_FREQUENCY);
	}

	return NULL;
}

void receive(__tp(handler)* sk, char* data, uint16_t len, int64_t timestamp,int64_t air_time, uint16_t src_id){
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

	free(packet);
}

void PrintLocation(void * info){
	Location * loc = (Location *) info;
	printf("x: %d \ny: %d \nmap id: %d \n", loc->x, loc->y,loc->area_id);
}

void free_elements(){
	unregister_handler_address(MY_ADDRESS,handler->module_communication.regd);
	RBTreeDestroy(people_located);
	FreeList(&spotters);
	pthread_mutex_destroy(&aggregator);
	pthread_mutex_destroy(&dataToSend);
	//TODO More elements to free
}

int main(int argc, char ** argv){

	//LER CONFIGS
	char line[80];
	char * var_value;
	FILE * config_file;
	void * handle;
	char * error;
	int op = 0;
	LElement * item;
	pthread_t update_aggregator_loop;

	if (argc != 3) {
		printf("USAGE: manager <Handler_file> <manager_file>\n");
		return 0;
	}

	logger("Main - Reading Config file\n");

	//LER CONFIGS DO FICHEIRO
	if (!(config_file = fopen(argv[2], "rt"))) {
		printf("Invalid manager config file\n");
		return 0;
	}

	while (fgets(line, 80, config_file) != NULL) {

		if (line[0] == '#')
			continue;

		var_value = strtok(line, "=");

		if (!memcmp(var_value, "MY_ADDRESS",strlen("MY_ADDRESS")))
			MY_ADDRESS = atoi(strtok(NULL, "=\n"));
		else if(!memcmp(var_value, "MAP", strlen("MAP")))
			MAP = atoi(strtok(NULL,"=\n"));
		else if (!memcmp(var_value, "MIN_UPDATE_FREQUENCY", strlen("MIN_UPDATE_FREQUENCY")))
			MIN_UPDATE_FREQUENCY = atoi(strtok(NULL, "=\n"));

	}

	fclose(config_file);

	people_located = RBTreeCreate(strcmp,free,free,NullFunction,PrintLocation);
	CreateList(&spotters);

	logger("Main - Configuration concluded\n");

	//REGISTER HANDLER
	handler = __tp(create_handler_based_on_file)(argv[1], receive);

	//REGISTER SERVICE
	RegisterService(NULL,handler,MY_ADDRESS,ServiceFound);

	//REQUEST AGGREGATOR
	char request_service[255];
	sprintf(request_service,"AGGREGATOR:%d;AGGREGATOR,PEOPLE_LOCATION,SERVICE", MAP);

	RequestService(request_service);

	pthread_mutex_init(&aggregator,NULL);
	pthread_mutex_init(&dataToSend,NULL);

	pthread_create(&update_aggregator_loop, NULL, manager_send_loop, NULL);

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
