/*
 * gsd_api.c
 *
 *  Created on: Jun 23, 2011
 *      Author: root
 */

#include <fred/handler.h>
#include <fred/addr_convert.h>
#include <stdbool.h>
#include <stdint.h>
#include "gsd_api.h"

static uint16_t address = 0;
static __tp(handler)* handler = NULL;
static void (* callback)(uint16_t) = NULL;

//uint8_t requests[];
//char * descriptions[];

bool RegisterService(Service * provide, __tp(handler)* handler_th, uint16_t handler_id, void (* service_found_cb)(uint16_t)){
	//TODO:FUTURE WORK - DINAMICALLY REGISTER IN GSD; FOR NOW JUST LOCALLY SAVE THE POINTER TO THE CALLBACK FUNCTION
	address = handler_id;
	handler = handler_th;
	callback = service_found_cb;

	return true;
}

bool RequestService(char * wanted){
	if (handler != NULL){

	char message[1024];
	uint16_t gsd_handler;
	uint8_t node;
	uint8_t app;

	//get the k algorism and sum 1 - That's the handler of local GSD (ex: 1001);
	int2dot(address,&node,&app);
	gsd_handler = dot2int(node,1);

	sprintf(message,"LOCAL_REQUEST<>%s",wanted);
	send_data(handler,message,strlen(message),gsd_handler);

		return true;
	}
	return false;
}

bool StopProvidingService(uint16_t handler_id){
	//TODO: FUTURE WORK - DINAMICALLY UNREGISTER IN GSD;
	address = 0;
	handler = NULL;
	callback = NULL;

	return true;
}

void GsdReceive(char * message){
	char * parser = strtok(message,":");
	parser = strtok(NULL,",");
	parser = strtok(NULL,",");
	uint16_t destination = atoi(parser);
	callback(destination);
}


