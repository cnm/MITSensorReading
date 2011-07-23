/*
 * spotter_services.c
 *
 *  Created on: Jun 22, 2011
 *      Author: root
 */
#include<stdio.h>
#include<stdint.h>
#include<stdbool.h>
#include<fred/handler.h>
#include"location.h"
#include"location_json.h"
#include"spotter.h"
#include"spotter_services.h"

extern Location self;
extern __tp(handler)* handler;

void RequestInstant(uint16_t address){
	SendSensorData(address);
}

void RequestFrequent(uint16_t manager_address, unsigned short required_frequence){
	unsigned char * message;
	size_t len;

	LocationPacket packet;
	packet.type = REGISTER_MANAGER;

	packet.RegSensor.min_update_frequency = AddManager(manager_address,required_frequence);
	packet.RegSensor.sensor_location = self;

	generate_JSON(&packet,&message,&len);
	send_data(handler, (char *) &message,len,manager_address);

}

void ConfirmSpontaneousRegister(uint16_t manager_address, unsigned short final_frequence){
	AddManager(manager_address,final_frequence);
}
