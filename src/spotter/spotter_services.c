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

void RequestInstant(uint16_t address){
	SendSensorData(address);
}

void RequestFrequent(uint16_t manager_address, unsigned short required_frequence){
	AddManager(manager_address,required_frequence);
	//TODO: CHECK FINAL FREQUENCE AND SEND BACK CONFIRMSPOTTER
}

void ConfirmSpontaneousRegister(uint16_t manager_address, unsigned short final_frequence){
	AddManager(manager_address,final_frequence);
}
