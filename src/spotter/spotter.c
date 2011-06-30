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
#include "spotter.h"
#include "location.h"
#include "location_json.h"
#include "gsd_api.h"
#include "spotter_services.h"


__tp(handler)* handler = NULL;
unsigned short SENSE_FREQUENCY = 5;
unsigned short MIN_UPDATE_FREQUENCY = 15;
unsigned short UPDATE_FREQUENCY = 15;
LList plugins;
uint16_t MY_ADDRESS, manager_address;
Location self;
bool manager_available = false;
pthread_mutex_t manager, dataToSend;
SensorDataList cached_data;

void SensorResult(SensorData * data){
	short index = 1;
	LElement * elem;
	switch(data->type){
		case ENTRY:
			FOR_EACH(elem,cached_data){
				if (((SensorData *)elem->data)->type == ENTRY){
					((SensorData *)elem->data)->entrances += data->entrances;
					free(data);
					break;
				}
			}
			break;
		case COUNT:
			FOR_EACH(elem,cached_data){
				if (((SensorData *)elem->data)->type == COUNT){
					((SensorData *)elem->data)->people = data->people;
					free(data);
					break;
				}
			}
			break;
		case RSS:
			FOR_EACH(elem,cached_data){
				if (((SensorData *)elem->data)->type == RSS){
					pthread_mutex_lock(&dataToSend);
					DelFromList(index, &cached_data);
					AddToList(data, &cached_data);
					pthread_mutex_unlock(&dataToSend);
				}
			}
			break;
	}

}

void AddManager(uint16_t address, unsigned short frequence){
	pthread_mutex_lock(&manager);
	manager_address = address;
	if (frequence >= MIN_UPDATE_FREQUENCY)
		UPDATE_FREQUENCY = frequence;
	else
		UPDATE_FREQUENCY = MIN_UPDATE_FREQUENCY;
	manager_available = true;
	pthread_mutex_unlock(&manager);
}

void ServiceFound(uint16_t dest_handler) {
	if (!manager_available){
		//Send to Service Manager self location and maximum sense frequency
		unsigned char * data;
		size_t length;
		LocationPacket location;
		location.type = REGISTER_SENSOR;
		location.RegSensor.sensor_location = self;
		location.RegSensor.min_update_frequency = MIN_UPDATE_FREQUENCY;

		generate_JSON(&location,&data,&length);

		send_data(handler, (char *)&data,length,dest_handler);

		free(data);
	}
}

void ManagerLost(){
	pthread_mutex_lock(&manager);
	manager_available = false;
	pthread_mutex_unlock(&manager);
}

void receive(__tp(handler)* sk, char* data, uint16_t len, int64_t timestamp, uint16_t src_id){
	LocationPacket * packet = (LocationPacket *) malloc(sizeof(LocationPacket));
	generate_packet_from_JSON(data, packet);

	switch(packet->type){
		case REGISTER_SENSOR:
			ConfirmSpontaneousRegister(src_id,packet->required_frequency);
			break;
		case REQUEST_FREQUENT:
			RequestFrequent(src_id,packet->required_frequency);
				break;
		case REQUEST_INSTANT:
			RequestInstant(src_id);
				break;
		default:
				break;
	}

	free(packet);
}

void * sensor_loop(){
	LElement * item;
	Plugin * plugin;
	void (* start_sense)();
	void * handle;
	int index = 1;
	char * error;
	while(true){
		index = 1;
		FOR_EACH(item, plugins){
			index++;
			plugin = (Plugin *) item->data;
			if (plugin->type == SYNC){
				handle = dlopen(plugin->location, RTLD_LAZY);
				dlerror(); /* Clear any existing error */
				start_sense = dlsym(handle, "start_sense");
				if ((error = dlerror()) != NULL) {
					fprintf(stderr, "%s\n", error);
					pthread_mutex_lock(&dataToSend);
					DelFromList(index,&cached_data);
					pthread_mutex_unlock(&dataToSend);
				}
				start_sense();
				dlclose(handle);
			}
		}
		sleep(SENSE_FREQUENCY);
	}
	return NULL;
}

void SendSensorData(uint16_t address){
	LocationPacket packet;
	LElement * elem;
	unsigned char * data;
	size_t length;
	packet.type = SENSOR_DATA;

	CreateList(&packet.data);
	FOR_EACH(elem, cached_data){
		AddToList(elem,&packet.data);
	}
	generate_JSON(&packet, &data,&length);

	send_data(handler, (char *)&data,length,manager_address);

	free(data);
}

void * spotter_send_loop(){

	while(true){

		if (manager_available)
			SendSensorData(manager_address);

		//FreeList(&cached_data);

		sleep(UPDATE_FREQUENCY);
	}

	return NULL;
}

/*
 void ServiceUnavailable(){
 //Retry in X seconds??
 }
 */

void free_elements(){
	unregister_handler_address(MY_ADDRESS,handler->module_communication.regd);
	pthread_mutex_destroy(&manager);
	pthread_mutex_destroy(&dataToSend);
	FreeList(&plugins);
	FreeList(&cached_data);
}

void print_plugins(){
	LElement * item;
	char * type;
	Plugin * plugin;
	FOR_EACH(item, plugins){
		plugin = (Plugin *) item->data;
		switch(plugin->type){
			case SYNC : type = "SYNC";
					break;
			case ASYNC : type = "ASYNC";
					break;
		}
		printf("LOCATION: %s ; TYPE: %s\n", plugin->location, type);
	}
}

int main(int argc, char ** argv) {

	//LER CONFIGS E VER QUAIS OS PLUGINS QUE EXISTEM
	char line[80];
	char * var_value;
	char * plugin_location;
	FILE * config_file;
	Plugin * plugin;
	void * handle;
	void (*start_cb)();
	char * error;
	int op = 0;
	LElement * item;
	pthread_t sense_loop, update_manager_loop;

	if (argc != 3) {
		printf("USAGE: spotter <Handler_file> <spotter_file>\n");
		return 0;
	}

	CreateList(&plugins);
	CreateList(&cached_data);

	logger("Main - Reading Config file\n");

	//LER CONFIGS DO FICHEIRO
	if (!(config_file = fopen(argv[2], "rt"))) {
		printf("Invalid spotter config file\n");
		return 0;
	}

	while (fgets(line, 80, config_file) != NULL) {

		if (line[0] == '#')
			continue;

		var_value = strtok(line, "=");

		if (!memcmp(var_value, "PLUGIN", strlen("PLUGIN"))){
			plugin = (Plugin *) malloc(sizeof(Plugin));
			plugin_location = strtok(NULL, " \n");
			plugin->location = (char *) malloc(strlen(plugin_location) + 1);
			memcpy(plugin->location,plugin_location,strlen(plugin_location) + 1);
			char * plugin_type = strtok(NULL, " \n");
			if (!memcmp(plugin_type, "SYNC", strlen("SYNC")))
				plugin->type = SYNC;
			else if (!memcmp(plugin_type, "ASYNC", strlen("ASYNC")))
				plugin->type = ASYNC;
			else{
				debugger("INVALID PLUGIN TYPE\n");
				exit(1);
			}
			AddToList(plugin, &plugins);
		}else if (!memcmp(var_value, "MY_ADDRESS",strlen("MY_ADDRESS")))
			MY_ADDRESS = atoi(strtok(NULL, "=\n"));
		else if(!memcmp(var_value, "LOCATION", strlen("LOCATION"))){
			self.x = atof(strtok(NULL, ";"));
			self.y = atof(strtok(NULL, "\n"));
		}else if (!memcmp(var_value, "SENSE_FREQUENCY",strlen("SENSE_FREQUENCY")))
			SENSE_FREQUENCY = atoi(strtok(NULL, "=\n"));
		else if (!memcmp(var_value, "MIN_UPDATE_FREQUENCY", strlen("MIN_UPDATE_FREQUENCY")))
			MIN_UPDATE_FREQUENCY = atoi(strtok(NULL, "=\n"));

	}

	fclose(config_file);

	logger("Main - Configuration concluded\n");

	//REGISTER HANDLER
	handler = __tp(create_handler_based_on_file)(argv[1], receive);

	//FUTURE WORK: REGISTER SERVICE GSD

	//TODO: ALTERAR PARA IR BUSCAR ZONA CORRECTA
	Service wanted;
	char * group1 = "MANAGER";
	char * group2 = "PEOPLE_LOCATION";
	char * group3 = "SERVICE";

	wanted.description = "MANAGER:A";
	CreateList(&wanted.groups);
	AddToList(group1,&wanted.groups);
	AddToList(group2,&wanted.groups);
	AddToList(group3,&wanted.groups);

	RequestService(&wanted);

	pthread_mutex_init(&manager,NULL);
	pthread_mutex_init(&dataToSend,NULL);

	//START GATHERING SENSOR DATA
	int index = 1;
	FOR_EACH(item, plugins) {
		plugin = (Plugin *) item->data;
		handle = dlopen(plugin->location, RTLD_LAZY);
		if (!handle) {
			fprintf(stderr, "%s\n", dlerror());
			//free_elements();
			//exit(1);
		}
		dlerror(); /* Clear any existing error */
		start_cb = dlsym(handle, "start_cb");
		if ((error = dlerror()) != NULL) {
			fprintf(stderr, "%s\n", error);
			//free_elements();
			//exit(1);
		}else{
			start_cb(&SensorResult);
			dlclose(handle);
		}
		index++;
	}

	pthread_create(&sense_loop, NULL, sensor_loop, NULL);
	pthread_create(&update_manager_loop, NULL, spotter_send_loop, NULL);

	while(op != 3){
		printf("MENU:\n 1-PRINT DATA TO SEND\n 2-PRINT AVAILABLE PLUGINS\n 3-EXIT\n\n");
		scanf("%d", &op);

		switch(op){
			case 1:
					break;
			case 2:	print_plugins();
					break;
			case 3: free_elements();
					break;
			default: printf("Acção Incorrecta\n");
		}
	}

	return 0;
}