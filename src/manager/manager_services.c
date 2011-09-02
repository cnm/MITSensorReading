/*
 * manager.c
 *
 *  Created on: Jun 22, 2011
 *      Author: root
 */
 
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <fred/handler.h>
#include <openssl/md5.h>
#include "red_black_tree.h"
#include "listType.h"
#include "location.h"
#include "location_json.h"
#include "manager.h"
#include "manager_services.h"
#include "vec3d.h"
#include "map.h"

extern rb_red_blk_tree * people_located;
extern unsigned short people_in_area;
extern uint16_t map_id;
extern Map * my_map;
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

	packet.manager_area_id = map_id;
	AddAggregator(aggregator_address,frequency);

	generate_JSON(&packet,&message,&len);
	send_data(handler, (char *) message,len,aggregator_address);

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
	spotter->current_info = NULL;
	spotter->last_received = 0;

	AddToList(spotter,&spotters);

	LocationPacket packet;
	packet.type = CONFIRM_MANAGER;
	packet.required_frequency = max_frequency;

	generate_JSON(&packet,&message,&len);
	send_data(handler, (char *) message,len,spotter_address);

}

void FreeRSS(RSSInfo * rss){
	free(rss->nodes);
	free(rss->rss);
	free(rss);
}

void DeliverSpotterData(uint16_t spotter_address, LocationPacket * packet, uint64_t time_sent){
	LElement * elem, * node, * iter;
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
								rss->nodes = (unsigned char *) malloc((MD5_DIGEST_LENGTH*2+1)*sensor->RSS.node_number);
								rss->rss = (uint16_t *) malloc(sizeof(uint16_t)*sensor->RSS.node_number);
								memcpy(rss,&(sensor->RSS),sizeof(RSSInfo));
								memcpy(rss->nodes, sensor->RSS.nodes,(MD5_DIGEST_LENGTH*2+1)*sensor->RSS.node_number);
								memcpy(rss->rss,sensor->RSS.rss,sizeof(uint16_t)*sensor->RSS.node_number);
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
						
						unsigned short i,aux=0;
						LList raw_list;
						CreateList(&raw_list);
						bool in_list = false;

						FOR_EACH(node,spotters){
							Spotter * spotter = (Spotter *) node->data;
							if (spotter->current_info != NULL){
								for (i=0; i < spotter->current_info->node_number; i++){
									if (i>0) aux=1;
									FOR_EACH(iter, raw_list){
										TriInfo * tri = (TriInfo *) iter->data;
										 if (!memcmp((const char *) tri->node,(const char *) spotter->current_info->nodes+(MD5_DIGEST_LENGTH*2+1)*i+aux, MD5_DIGEST_LENGTH*2)){
										 	in_list = true;
										 	if (!tri->b2){
										 		tri->b2 = true;
										 		tri->r2 = spotter->current_info->rss[i];
											 }else{										 	
										 		tri->r3 = spotter->current_info->rss[i];
										 		tri->b3 = true;
										 	}
										 }
									}

									if (!in_list){
										aux=0;
										TriInfo * new = (TriInfo *) malloc(sizeof(TriInfo));
										new->node = (unsigned char *) malloc(MD5_DIGEST_LENGTH*2 + 1);
										new->b2 = false;
										new->b3 = false;
										if (i>0) aux=1;
										memcpy((char *) new->node, (char *) spotter->current_info->nodes+(MD5_DIGEST_LENGTH*2+1)*i + aux,MD5_DIGEST_LENGTH*2+1);
										new->s1 = spotter->location;
										new->r1 = spotter->current_info->rss[i];
										AddToList(new, &raw_list);
									}

									in_list = false;
								}

								FreeRSS(spotter->current_info);
								spotter->current_info = NULL;
							}
						}
						
						FOR_EACH(iter,raw_list){
							TriInfo * tri = (TriInfo *) iter->data;

							if(!tri->b2)
								break;

							vec3d v1,v2,v3,result1,result2;
							double r1,r2,r3;

							v1.x = (double) tri->s1.x * my_map->cell_size;
							v1.y = (double) tri->s1.y * my_map->cell_size;
							v1.z = 0;
							r1 = (double) tri->r1;

							v2.x = (double) tri->s2.x * my_map->cell_size;
							v2.y = (double) tri->s2.y * my_map->cell_size;
							v2.z = 0;

							r2 = (double) tri->r2;

							if(tri->b3){
								v3.x = (double) tri->s3.x * my_map->cell_size;
								v3.y = (double) tri->s3.y * my_map->cell_size;
								r3 = (double) tri->r3;
							}else{
								v3.x = 0;
								v3.y = 0;	
								r3 = my_map->cell_size;
							}
							v3.z = 0;

							
							Location * location;
							if (trilateration(&result1,&result2,v1,r1,v2,r2,v3,r3,0.00001)==0)
								location = InfoToCell(my_map, &result1, &result2);
							else
								location = InfoToCell(my_map, &v1, &v2);
							
							
							rb_red_blk_node * previous = RBExactQuery(people_located, tri->node);

							if(previous!=NULL)
								RBDelete(people_located, previous);

							RBTreeInsert(people_located, tri->node, location);
							
						}

						FreeList(&raw_list);
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
