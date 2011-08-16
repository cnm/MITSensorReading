/*
 * location_json.c
 *
 *  Created on: Jun 14, 2011
 *      Author: root
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <yajl/yajl_parse.h>
#include <yajl/yajl_gen.h>
#include <yajl/yajl_tree.h>
#include <openssl/md5.h>
#include "listType.h"
#include "location.h"
#include "location_json.h"
#include <fred/handler.h>
#include <syslog.h>
#include "red_black_tree.h"
#include "map.h"
#include "manager.h"

static void PrintNodeJSON(yajl_gen g, rb_red_blk_node* x){
	yajl_gen_map_open(g);
		yajl_gen_string(g, (unsigned char *) "Key", strlen("Key"));
		yajl_gen_string(g, (unsigned char *) x->key, strlen((char *) x->key));

		yajl_gen_string(g, (unsigned char *) "x" , strlen("x"));
		yajl_gen_integer(g, ((Location *) x->info)->x);

		yajl_gen_string(g, (unsigned char *) "y" , strlen("y"));
		yajl_gen_integer(g, ((Location *) x->info)->y);

		yajl_gen_string(g, (unsigned char *) "area_id" , strlen("area_id"));
		yajl_gen_integer(g, ((Location *) x->info)->area_id);
	yajl_gen_map_close(g);
}



static void InorderTreeJSON(yajl_gen g, rb_red_blk_tree* tree, rb_red_blk_node* x) {
  if (x != tree->nil) {
    InorderTreeJSON(g, tree,x->left);

    PrintNodeJSON(g, x);
    
    InorderTreeJSON(g, tree,x->right);
  }
}



static void RBTreeJSON(yajl_gen g, rb_red_blk_tree* tree) {
  InorderTreeJSON(g, tree,tree->root->left);
}



bool generate_JSON(LocationPacket * packet, unsigned char ** response, size_t * length){

	yajl_handle hand;
    /* generator config */
    yajl_gen g;

    g = yajl_gen_alloc(NULL);
    yajl_gen_config(g, yajl_gen_beautify, 0);
    yajl_gen_config(g, yajl_gen_validate_utf8, 0);

    /* ok.  open file.  let's read and parse */
    hand = yajl_alloc(NULL, NULL, (void *) g);
    /* and let's allow comments by default */
    yajl_config(hand, yajl_allow_comments, 0);

	yajl_gen_map_open(g);

	yajl_gen_string(g, (unsigned char *) "Packet", strlen("Packet"));
	yajl_gen_map_open(g);

	yajl_gen_string(g, (unsigned char *) "MessageType", strlen("MessageType"));
	yajl_gen_integer(g, packet->type);

	switch (packet->type){
		case REQUEST_INSTANT: break;
		case REQUEST_FREQUENT:
			yajl_gen_string(g, (unsigned char *) "required_frequency", strlen("required_frequency"));
			yajl_gen_integer(g,packet->required_frequency);
			break;

		case REGISTER_SENSOR:
		case CONFIRM_SENSOR:

			yajl_gen_string(g, (unsigned char *) "RegSensor", strlen("RegSensor"));
			yajl_gen_map_open(g);

				yajl_gen_string(g, (unsigned char *) "min_update_frequency", strlen("min_update_frequency"));
				yajl_gen_integer(g, packet->RegSensor.min_update_frequency);

				yajl_gen_string(g, (unsigned char *) "sensor_location", strlen("sensor_location"));
				yajl_gen_map_open(g);

					yajl_gen_string(g, (unsigned char *) "x", strlen("x"));
					yajl_gen_integer(g, packet->RegSensor.sensor_location.x);

					yajl_gen_string(g, (unsigned char *) "y", strlen("y"));
					yajl_gen_integer(g, packet->RegSensor.sensor_location.y);

				yajl_gen_map_close(g);
			yajl_gen_map_close(g);

			break;
		case REGISTER_MANAGER:
			yajl_gen_string(g, (unsigned char *) "manager_area_id", strlen("manager_area_id"));
			yajl_gen_integer(g, packet->manager_area_id);

			break;
		case CONFIRM_MANAGER:
			yajl_gen_string(g, (unsigned char *) "required_frequency", strlen("required_frequency"));
			yajl_gen_integer(g, packet->required_frequency);
			break;
		case SENSOR_DATA:
			yajl_gen_string(g, (unsigned char *) "data", strlen("data"));
			yajl_gen_array_open(g);
				LElement * elem;
				SensorData * sensor;
				FOR_EACH(elem, packet->data){
					sensor = (SensorData *) elem->data;
					yajl_gen_map_open(g);
						yajl_gen_string(g, (unsigned char *) "type", strlen("type"));
						yajl_gen_integer(g, sensor->type);

						switch (sensor->type){
							case ENTRY:
								yajl_gen_string(g, (unsigned char *) "entrances", strlen("entrances"));
								yajl_gen_integer(g, sensor->entrances);
								break;
							case COUNT:
								yajl_gen_string(g, (unsigned char *) "people", strlen("people"));
								yajl_gen_integer(g, sensor->people);
								break;
							case RSS:
								yajl_gen_string(g, (unsigned char *) "type", strlen("type"));
								yajl_gen_integer(g, sensor->RSS.type);

								yajl_gen_string(g, (unsigned char *) "node_number", strlen("node_number"));
								yajl_gen_integer(g, sensor->RSS.node_number);

								yajl_gen_string(g, (unsigned char *) "nodes", strlen("nodes"));
								yajl_gen_array_open(g);

									short i;
									for (i = 0; i< sensor->RSS.node_number; i++){
										yajl_gen_string(g, sensor->RSS.nodes[i], strlen((char *) sensor->RSS.nodes[i]));
									}
								yajl_gen_array_close(g);
								yajl_gen_string(g, (unsigned char *) "rss", strlen("rss"));
								yajl_gen_array_open(g);
									for (i = 0; i< sensor->RSS.node_number; i++){
										yajl_gen_integer(g,sensor->RSS.rss[i]);
									}
								yajl_gen_array_close(g);
								break;
						}
					yajl_gen_map_close(g);
				}
			yajl_gen_array_close(g);
			break;
		case MANAGER_DATA:
			
			yajl_gen_string(g, (unsigned char *) "Manager_data", strlen("Manager_data"));
			yajl_gen_map_open(g);

				yajl_gen_string(g, (unsigned char *) "num_people", strlen("num_people"));
				yajl_gen_integer(g, packet->Manager_data.num_people);

				yajl_gen_string(g, (unsigned char *) "people", strlen("people"));
				yajl_gen_array_open(g);
					
					RBTreeJSON(g, packet->Manager_data.people);

				yajl_gen_array_close(g);

			yajl_gen_map_close(g);

			break;
		default: break;
	}

	yajl_gen_map_close(g);
	yajl_gen_map_close(g);

	const unsigned char * buf;

    yajl_gen_get_buf(g, &buf, length);
    *response = (unsigned char *) malloc((*length + 1)*sizeof(char));
	memcpy((char*)*response, (char *)buf, *length + 1);
	yajl_gen_free(g);

	return true;
}

bool generate_packet_from_JSON(char * data, LocationPacket * packet){

	yajl_val node;
	char errbuf[1024];


	//logger("Parsing packet from JSON message");

	node = yajl_tree_parse(( char *) data, errbuf, sizeof(errbuf));

	/* parse error handling */
    if (node == NULL) {
        fprintf(stderr,"parse_error: ");
        if (strlen(errbuf)){
        	fprintf(stderr," %s", errbuf);
        }else{
        	fprintf(stderr,"unknown error");
		}
        fprintf(stderr,"\n");
        return false;
    }

    const char * path[] = { "Packet", "MessageType", ( char *) 0 };
    packet->type = YAJL_GET_INTEGER(yajl_tree_get(node, path, yajl_t_number));

    yajl_val object,location,array,info;
    short j,z,i;

    switch(packet->type){
		case REQUEST_INSTANT: break;
		case REQUEST_FREQUENT:
			path[1] = "required_frequency";
			packet->required_frequency = YAJL_GET_INTEGER(yajl_tree_get(node, path, yajl_t_number));
			break;

		case REGISTER_SENSOR:
		case CONFIRM_SENSOR:

			path[1] = "RegSensor";
			object = yajl_tree_get(node, path, yajl_t_object);

			for (j=0; j < YAJL_GET_OBJECT(object)->len; j++){
				if (!strcmp(YAJL_GET_OBJECT(object)->keys[j],"min_update_frequency"))
					packet->RegSensor.min_update_frequency = YAJL_GET_INTEGER(YAJL_GET_OBJECT(object)->values[j]);
				else{
					location = YAJL_GET_OBJECT(object)->values[j];
					for (z=0; z < YAJL_GET_OBJECT(location)->len; z++){
						if (!strcmp(YAJL_GET_OBJECT(location)->keys[z],"x"))
							packet->RegSensor.sensor_location.x = YAJL_GET_INTEGER(YAJL_GET_OBJECT(location)->values[z]);
						else
							packet->RegSensor.sensor_location.y = YAJL_GET_INTEGER(YAJL_GET_OBJECT(location)->values[z]);
					}
				}
			}
			break;
		case REGISTER_MANAGER:
			path[1] = "manager_area_id";
			packet->manager_area_id = YAJL_GET_INTEGER(yajl_tree_get(node, path, yajl_t_number));
			break;
		case CONFIRM_MANAGER:
			path[1] = "required_frequency";
			packet->required_frequency = YAJL_GET_INTEGER(yajl_tree_get(node, path, yajl_t_number));
			break;
		case MANAGER_DATA:
			//TODO: NEED TO DEFINE MANAGER_DATA FIRST
			path[1] = "Manager_data";
			object = yajl_tree_get(node, path, yajl_t_object);

			packet->Manager_data.people = RBTreeCreate(CompareNodes,free,free,NullFunction,PrintLocation);

			for (j=0; j < YAJL_GET_OBJECT(object)->len; j++){
				if (!strcmp(YAJL_GET_OBJECT(object)->keys[j],"num_people"))
					packet->Manager_data.num_people = YAJL_GET_INTEGER(YAJL_GET_OBJECT(object)->values[j]);
				else{
					array = YAJL_GET_OBJECT(object)->values[j];
					for(z=0; z < YAJL_GET_ARRAY(array)->len; z++){
						info = YAJL_GET_OBJECT(array)->values[z];
						char * rb_key = (char *) malloc(MD5_DIGEST_LENGTH + 1);
						Location * rb_info = (Location *) malloc(sizeof(Location));
						for (i=0; z < YAJL_GET_OBJECT(info)->len; z++){
							if (!strcmp(YAJL_GET_OBJECT(info)->keys[i],"x"))
								rb_info->x = YAJL_GET_INTEGER(YAJL_GET_OBJECT(info)->values[i]);
							if (!strcmp(YAJL_GET_OBJECT(info)->keys[i],"y"))
								rb_info->y = YAJL_GET_INTEGER(YAJL_GET_OBJECT(info)->values[i]);
							if (!strcmp(YAJL_GET_OBJECT(info)->keys[i],"area_id"))
								rb_info->area_id = YAJL_GET_INTEGER(YAJL_GET_OBJECT(info)->values[i]);
							if (!strcmp(YAJL_GET_OBJECT(info)->keys[i],"Key")){
								char * tmp;
								rb_key = YAJL_GET_STRING(YAJL_GET_OBJECT(info)->values[i]);
								tmp = YAJL_GET_STRING(YAJL_GET_ARRAY(info)->values[i]);
								strcpy(rb_key, tmp);
							}

						}
						RBTreeInsert(packet->Manager_data.people, rb_key, rb_info);
					}
				}

			}

			break;
		case SENSOR_DATA:
			path[1] = "data";
			yajl_val array = yajl_tree_get(node,path,yajl_t_array);
			CreateList(&packet->data);
			SensorData * sensor_data;
			for (j=0; j < YAJL_GET_ARRAY(array)->len; j++){
				yajl_val sensor_obj = YAJL_GET_ARRAY(array)->values[j];
				sensor_data = (SensorData *) malloc(sizeof(SensorData));
				sensor_data->type = YAJL_GET_INTEGER(YAJL_GET_OBJECT(sensor_obj)->values[0]);
				switch(sensor_data->type){
					case ENTRY:
						sensor_data->entrances = YAJL_GET_INTEGER(YAJL_GET_OBJECT(sensor_obj)->values[1]);
						break;
					case COUNT:
						sensor_data->people = YAJL_GET_INTEGER(YAJL_GET_OBJECT(sensor_obj)->values[1]);
						break;
					case RSS:
					{
						char * tmp;
						yajl_val rss_node = YAJL_GET_OBJECT(sensor_obj)->values[1];
						sensor_data->RSS.type = YAJL_GET_INTEGER(YAJL_GET_OBJECT(rss_node)->values[0]);
						sensor_data->RSS.node_number = YAJL_GET_INTEGER(YAJL_GET_OBJECT(rss_node)->values[1]);
						sensor_data->RSS.nodes = (unsigned char **) malloc(MD5_DIGEST_LENGTH*sensor_data->RSS.node_number + 1);
						sensor_data->RSS.rss = (int8_t *) malloc(sizeof(int8_t)*sensor_data->RSS.node_number);
						yajl_val node_array = YAJL_GET_OBJECT(rss_node)->values[2];

						for (j=0; j<YAJL_GET_ARRAY(node_array)->len;j++){
							tmp = YAJL_GET_STRING(YAJL_GET_ARRAY(node_array)->values[j]);
							strcpy((char *)sensor_data->RSS.nodes[j], tmp);
						}

						yajl_val rss_array = YAJL_GET_OBJECT(rss_node)->values[3];
						for (j=0; j<YAJL_GET_ARRAY(rss_array)->len;j++)
							sensor_data->RSS.rss[j] = YAJL_GET_INTEGER(YAJL_GET_ARRAY(rss_array)->values[j]);

						break;
					}
					default: break;
				}
			}

			break;
		default: break;
    }

	return true;
}
