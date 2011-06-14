/*
 * location_json.c
 *
 *  Created on: Jun 14, 2011
 *      Author: root
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <yajl/yajl_parse.h>
#include <yajl/yajl_gen.h>
#include <yajl/yajl_tree.h>
#include "listType.h"
#include "location.h"
#include "location_json.h"
#include <fred/handler.h>
#include <syslog.h>

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
		case REGISTER_SENSOR: break;
		case REGISTER_MANAGER: break;
		case SENSOR_DATA: break;
		case MANAGER_DATA: break;
		default: break;
	}


	return true;
}

bool generate_packet_from_JSON(char * data, LocationPacket * packet){
	return true;
}
