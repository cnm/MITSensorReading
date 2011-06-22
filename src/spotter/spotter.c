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
#include "discovery.h"
#include "spotter.h"
#include "location.h"


__tp(handler)* handler = NULL;
unsigned short SENSE_FREQUENCY = 5;
unsigned short UPDATE_FREQUENCY = 15;
LList plugins;
uint16_t MY_ADDRESS;
Location self;
bool manager_available = false;
pthread_mutex_t manager, dataToSend;
LList sensorDataList;

void SensorResult(SensorData * data){
	pthread_mutex_lock(&dataToSend);
	AddToList(data,&sensorDataList);
	pthread_mutex_unlock(&dataToSend);
}

void ServiceFound(uint16_t dest_handler) {

	if (!manager_available){

		//Respond to Service Manager with self location
		send_data(handler,(char *) &self,strlen((char *) &self),dest_handler);

		//ENABLE SENDING
		pthread_mutex_lock(&manager);
		manager_available = true;
		pthread_mutex_unlock(&manager);
	}

}

void RequestInstant(uint16_t address){
	//TODO retornar ao address o estado actual dos sensores
}

void RequestFrequent(uint16_t manager_address, unsigned short required_frequence){
	//TODO Registar o novo Manager com a devida frequencia para começar a enviar para estes os dados sensed a cada required_frequence
}

void ConfirmSpontaneousRegister(){
	//TODO: Confirmar o registo pedido por este nó; recebe a frequencia final com que vai emitir ao Manager
}

void ManagerLost(){
	pthread_mutex_lock(&manager);
	manager_available = false;
	pthread_mutex_unlock(&manager);
}

void receive(__tp(handler)* sk, char* data, uint16_t len, int64_t timestamp, uint16_t src_id){

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
					DelFromList(index,&sensorDataList);
					pthread_mutex_unlock(&dataToSend);
				}
				start_sense();
				dlclose(handle);
			}
		}
		sleep(SENSE_FREQUENCY);
	}
}

void * spotter_send_loop(){
	while(true){

		sleep(UPDATE_FREQUENCY);
	}
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
	FreeList(&sensorDataList);
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

int main(int argc, char * argv[]) {

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
	CreateList(&sensorDataList);

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
		else if (!memcmp(var_value, "UPDATE_FREQUENCY", strlen("UPDATE_FREQUENCY")))
			UPDATE_FREQUENCY = atoi(strtok(NULL, "=\n"));

	}

	fclose(config_file);

	logger("Main - Configuration concluded\n");

	//REGISTER HANDLER
	handler = __tp(create_handler_based_on_file)(argv[1], receive);

	//FUTURE WORK: REGISTER SERVICE GSD

	//REQUEST MANAGER OF ZONE X


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
			start_cb();
			dlclose(handle);
		}
		index++;
	}

	//pthread_create(&sense_loop, NULL, sensor_loop, NULL);
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
