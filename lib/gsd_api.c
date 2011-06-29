/*
 * gsd_api.c
 *
 *  Created on: Jun 23, 2011
 *      Author: root
 */

#include "gsd_api.h"

void (* callback(uint16_t));

bool RegisterService(Service * provide, uint16_t handler_id, void (* service_found_cb(uint16_t))){
	//TODO:FUTURE WORK - DINAMICALLY REGISTER IN GSD; FOR NOW JUST LOCALLY SAVE THE POINTER TO THE CALLBACK FUNCTION

	callback = service_found_cb;
	return false;
}

void RequestService(Service * wanted){
	//TODO:
}

bool StopProvidingService(uint16_t handler_id){
	//TODO:
	return false;
}


