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
#include "aggregator.h"
#include "aggregator_services.h"
#include "vec3d.h"
#include "map.h"


extern __tp(handler)* handler;
extern LList managers;
extern LList maps;
extern rb_red_blk_tree * global_tree;
extern uint64_t updating_time;

void RequestInstant(uint16_t address){
	SendAggregatorData(address);
}

void RequestFrequent(uint16_t aggregator_address, unsigned short frequency){
	//TODO: Registar o novo UMPServer com a devida frequencia para começar a enviar para estes os dados a cada frequency
}

void ConfirmManager(uint16_t manager_address, uint8_t map_id, unsigned short final_frequency){
	Manager * manager = (Manager *) malloc(sizeof(Manager));
	manager->id = manager_address;
	manager->map_id = map_id;

	AddToList(manager,&managers);
}

void SpontaneousManager(uint16_t manager_address, uint8_t map_id, unsigned short max_frequency){

	unsigned char * message;
	size_t len;

	Manager * manager = (Manager *) malloc(sizeof(Manager));
	manager->id = manager_address;
	manager->map_id = map_id;

	AddToList(manager,&managers);

	LocationPacket packet;
	packet.type = CONFIRM_MANAGER;
	packet.required_frequency = max_frequency;

	generate_JSON(&packet,&message,&len);
	send_data(handler, (char *) message,len,manager_address);

}

static void CheckPreviousLocation(rb_red_blk_node* x){
	rb_red_blk_node * previous = RBExactQuery(global_tree, x->key);

	GlobalLocation * location = (GlobalLocation *) malloc(sizeof(GlobalLocation));
	location->last_update = updating_time;
	location->conflicted = false;
	location->has_previous = true;

	if (previous == NULL){

		location->current = *((Location *) x->info);	
		location->has_previous = false;
		
		RBTreeInsert(global_tree, x->key, location);

	}else if (!((GlobalLocation *) previous->info)->has_previous){

		location->previous = ((GlobalLocation *) previous->info)->current;
		location->current = *((Location *) x->info);
		
		RBDelete(global_tree, previous);
		RBTreeInsert(global_tree, x->key, location);

	}else{
		if (((GlobalLocation *)previous->info)->last_update == updating_time){
			//QUER DIZER QUE JÀ HOUVE UM NÓ QUE ADICIONOU ESTA PESSOA NESTE CICLO: OU SEJA CONFLITO
			Location * previous_location = &(((GlobalLocation *)previous->info)->previous);
			Location * comitted = &(((GlobalLocation *)previous->info)->current);
			Location * conflicted = (Location *) x->info;
			
			if(CheckTransition(&maps, previous_location, comitted, conflicted, ((GlobalLocation *)previous->info)->conflicted)){
				location->current = *conflicted;
			}else{
				location->current = *comitted;
			}	

			location->previous = *previous_location;
			location->conflicted = true;

			RBDelete(global_tree, previous);
			RBTreeInsert(global_tree, x->key, location);

		}else{
			location->previous = ((GlobalLocation *) previous->info)->current;
			location->current = *((Location *) x->info);

			RBDelete(global_tree, previous);
			RBTreeInsert(global_tree, x->key, location);
		}
		
	}
}



static void InorderTreePerson(rb_red_blk_tree* tree, rb_red_blk_node* x) {
  if (x != tree->nil) {
    InorderTreePerson(tree,x->left);

    CheckPreviousLocation(x);
    
    InorderTreePerson(tree,x->right);
  }
}



static void RBTreePerson(rb_red_blk_tree* tree) {
  InorderTreePerson(tree,tree->root->left);
}

void DeliverManagerData(uint16_t manager_address, LocationPacket * packet, uint64_t time_sent){
	
	LElement * node, * elem;
	uint8_t num_ready = 0;
	bool compute = false;


	FOR_EACH(node,managers){
		Manager * manager = (Manager *) node->data;

		//Check if it is a past event
		if (manager->id == manager_address){
			if(time_sent <= manager->last_received)
				break;			
			
			manager->last_received = time_sent;

			//Allocate it as the last info for this node till we have all the info from all nodes
			if (manager->current_info == NULL){
				manager->num_people = packet->Manager_data.num_people;
				manager->current_info = packet->Manager_data.people;
			}else{
				compute = true;
				break;
			}
		}

		if(manager->current_info != NULL)
			num_ready++;
	}

	//When we have info from all nodes (or if missed an info from a node) we compute the global location of people and detect consistency errors
	if (compute || num_ready == managers.NumEl){

		updating_time = time_sent;
		

		FOR_EACH(node, managers){
			Manager * manager = (Manager *) node->data;

			FOR_EACH(elem,maps){
				Map * map = (Map *) elem->data;

				if (manager->map_id == map->area_id){

					map->num_people = manager->num_people;

					if (map->people != NULL)
						RBTreeDestroy(map->people);

					map->people = manager->current_info;
					RBTreePerson(map->people);
				}
				
			}

			manager->current_info = NULL;
		}

	}
}
