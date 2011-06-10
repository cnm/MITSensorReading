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
#include "discovery.h"
#include "listType.h"
#include "JSON_handler.h"
#include <fred/handler.h>


__tp(handler)* handler = NULL;


void ServiceFound(uint16_t dest_handler){	
	//START SENDING?
}

/*
void ServiceUnavailable(){
	//Retry in X seconds??
}
*/

int main(int argc, char* argv[]){
	
	//LER CONFIGS E VER QUAIS OS PLUGINS QUE EXISTEM
	
	//REGISTER HANDLER
	
	//REGISTER SERVICE GSD
	
	//REQUEST MANAGER OF ZONE X
	
	//START GATHERING SENSOR DATA
/*	foreach plugin:
		plugin->start();
		plugin->doit();
	*/
	return 0;
}
