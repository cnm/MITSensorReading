/*
 * manager.c
 *
 *  Created on: Jun 22, 2011
 *      Author: root
 */
#include <stdio.h>
#include <stdint.h>
#include<stdbool.h>
#include<fred/handler.h>
#include <openssl/md5.h>
#include "red_black_tree.h"
#include "listType.h"
#include"location.h"
#include"location_json.h"
#include"manager.h"
#include"manager_services.h"


extern rb_red_blk_tree * people_located;
extern unsigned short people_in_area;
extern uint16_t MAP;
extern LList spotters;
extern __tp(handler)* handler;

static uint64_t last_count = 0;

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
	send_data(handler, (char *) &message,len,aggregator_address);

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
	packet.type = CONFIRM_MANAGER;
	packet.required_frequency = max_frequency;

	generate_JSON(&packet,&message,&len);
	send_data(handler, (char *) &message,len,spotter_address);

}

void FreeRSS(RSSInfo * rss){
	short i;
	for (i = 0; i < rss->node_number; i++){
		free(rss->nodes[i]);
	}
	free(rss->nodes);
	free(rss->rss);
	free(rss);
}

void DeliverSpotterData(uint16_t spotter_address, LocationPacket * packet, uint64_t time_sent){
	LElement * elem, * node;
	bool exit_entry = false;

	FOR_EACH(elem,packet->data){
		SensorData * sensor = (SensorData *) elem->data;
		switch (sensor->type){
			case ENTRY:
				FOR_EACH(node,spotters){
					Spotter * spotter = (Spotter *) node->data;
					if (spotter->id == spotter_address){
						if(time_sent < spotter->last_received){
							exit_entry = true;
							break;
						}
						else
							spotter->last_received = time_sent;
					}
				}
				if (exit_entry)
					break;

				if (sensor->entrances < 0 && people_in_area < sensor->entrances)
					people_in_area = 0;
				else
					people_in_area += sensor->entrances;

				break;
			case COUNT:
				if (time_sent > last_count)
					people_in_area = sensor->people;
				break;
			case RSS:
				{
					uint8_t num_ready = 0;
					bool compute = false;


					FOR_EACH(node,spotters){
						Spotter * spotter = (Spotter *) node->data;

						//Check if it is a past event
						if (spotter->id == spotter_address){
							if (time_sent < spotter->last_received)
								break;

							//Allocate it as the last info for this node till we have all the info from all nodes
							if (spotter->current_info == NULL){
								RSSInfo * rss = (RSSInfo *) malloc(sizeof(RSSInfo));
								rss->nodes = (unsigned char **) malloc(MD5_DIGEST_LENGTH*sensor->RSS.node_number);
								rss->rss = (int8_t *) malloc(sizeof(int8_t)*sensor->RSS.node_number);
								memcpy(rss,&(sensor->RSS),sizeof(RSSInfo));
								memcpy(rss->nodes, sensor->RSS.nodes,MD5_DIGEST_LENGTH*sensor->RSS.node_number);
								memcpy(rss->rss,sensor->RSS.rss,sizeof(int8_t)*sensor->RSS.node_number);
								spotter->current_info = rss;
								spotter->last_received = time_sent;
							}else{
								compute = true;
								break;
							}
						}

						if(spotter->current_info != NULL)
							num_ready++;
					}

					//When we have info from all nodes (or if missed an info from a node) we compute the location of people with the info we have
					if (compute || num_ready == spotters.NumEl){
						//TODO: ALL THE MAGIC! compute the location of people in this area!
						//Remove spotter current_info
						FOR_EACH(node,spotters){
							Spotter * spotter = (Spotter *) node->data;
							if (spotter->current_info != NULL){
								FreeRSS(spotter->current_info);
								spotter->current_info = NULL;
							}
						}
					}
				}
				break;
			default:
				break;
		}
	}

}

void ConfirmSpontaneousRegister(uint16_t aggregator_address, unsigned short final_frequence){
	AddAggregator(aggregator_address,final_frequence);
}
