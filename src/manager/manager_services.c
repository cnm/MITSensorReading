/*
 * manager.c
 *
 *  Created on: Jun 22, 2011
 *      Author: root
 */
#include <stdio.h>
#include"location.h"
#include "listType.h"


extern rb_red_blk_tree * people_located;
extern unsigned short people_in_area;

void RequestInstant(uint16_t address){
	SendManagerData(address);
}

void RequestFrequent(uint16_t aggregator_address, unsigned short frequency){
	unsigned char * message;
	size_t len;

	LocationPacket packet;
	packet.type = REGISTER_MANAGER;

	packet.manager_area_id = MAP;
	AddAggregator(aggregator_address,frequency);

	generate_JSON(&packet,&message,&len);
	send_data(handler, (char *) &message,len,manager_address);

}

void ConfirmSpotter(uint16_t spotter_address, Location location, unsigned short final_frequency){
	Spotter * spotter = (Spotter *) malloc(sizeof(Spotter));
	spotter->id = spotter_address;
	spotter->location = location;

	AddToList(spotter,&spotters);
}

void SpontaneousSpotter(uint16_t spotter_address, Location location, unsigned short max_frequency){

	unsigned char * message;
	size_t len;

	Spotter * spotter = (Spotter *) malloc(sizeof(Spotter));
	spotter->id = spotter_address;
	spotter->location = location;

	AddToList(spotter,&spotters);

	LocationPacket packet;
	packet->type = CONFIRM_MANAGER;
	packet->required_frequency = max_frequency;

	generate_JSON(&packet,&message,&len);
	send_data(handler, (char *) &message,len,manager_address);

}

void DeliverSpotterData(uint16_t spotter_address, LocationPacket packet, uint64_t time_sent){
	LElement * elem;
	LList * dados = packet->data;
	FOR_EACH(elem,*dados){
		SensorData * sensor = (SensorData *) elem->data;
		switch (sensor->type){
			case ENTRY:
				if (sensor->entrances < 0 && people_in_area < sensor->entrances)
					people_in_area = 0;
				else
					people_in_area += sensor->entrances;
				break;
			case COUNT:
				people_in_area = sensor->people;
				break;
			case RSS:
				//TODO: ALL THE MAGIC! compute the location of people in this area!
				break;
			default:
				break;
		}
	}
}

void ConfirmSpontaneousRegister(uint16_t aggregator_address, unsigned short final_frequence){
	AddAggregator(aggregator_address,final_frequence);
}
