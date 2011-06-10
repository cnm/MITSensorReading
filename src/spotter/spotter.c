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
#include "JSON_handler.h"


__tp(handler)* handler = NULL;
unsigned short SENSE_FREQUENCY = 5;
unsigned short UPDATE_FREQUENCY = 15;
LList plugins;


void ServiceFound(uint16_t dest_handler) {
	//START SENDING?
}

void receive(__tp(handler)* sk, char* data, uint16_t len, int64_t timestamp, uint16_t src_id){

}

/*
 void ServiceUnavailable(){
 //Retry in X seconds??
 }
 */

int main(int argc, char * argv[]) {

	//LER CONFIGS E VER QUAIS OS PLUGINS QUE EXISTEM
	char line[80];
	char * var_value;
	FILE * config_file;
	LElement * plugin;
	void * handle;
	void (*start_cb)();
	char * error;

	if (argc != 3) {
		printf("USAGE: spotter <Handler_file> <spotter_file>\n");
		return 0;
	}

	CreateList(&plugins);

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

		if (!memcmp(var_value, "PLUGIN", strlen("PLUGIN")))
			AddToList(strtok(var_value, "=\n"), &plugins);
		else if (!memcmp(var_value, "SENSE_FREQUENCY",strlen("SENSE_FREQUENCY")))
			SENSE_FREQUENCY = atoi(strtok(var_value, "=\n"));
		else if (!memcmp(var_value, "UPDATE_FREQUENCY", strlen("UPDATE_FREQUENCY")))
			UPDATE_FREQUENCY = atoi(strtok(var_value, "=\n"));

	}

	fclose(config_file);

	logger("Main - Configuration concluded\n");

	//REGISTER HANDLER
	handler = __tp(create_handler_based_on_file)(argv[1], receive);

	//REGISTER SERVICE GSD

	//REQUEST MANAGER OF ZONE X

	//START GATHERING SENSOR DATA
	FOR_EACH(plugin, plugins) {
		handle = dlopen(plugin->data, RTLD_LAZY);
		if (!handle) {
			fprintf(stderr, "%s\n", dlerror());
			exit(1);
		}
		dlerror(); /* Clear any existing error */
		start_cb = dlsym(handle, "start_cb");
		if ((error = dlerror()) != NULL) {
			fprintf(stderr, "%s\n", error);
			exit(1);
		}
		start_cb();
		dlclose(handle);
	}
	/*	foreach plugin:
	 plugin->start();
	 plugin->doit();
	 */
	return 0;
}
