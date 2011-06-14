#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <syslog.h>
#include "discovery.h"
#include "listType.h"
#include "ReverseRoute.h"
#include <fred/handler.h>
#include "GSD_JSON_handler.h"

ReverseRouteList reverse_table;
extern __tp(handler)* handler;

void ReverseRoute(GSDPacket * reply){
	
	LElement * rev_item;
	ReverseRouteEntry * rev_entry;
	unsigned char * data;
	size_t length;
	
	logger("Starting ReverseRoute\n");
	
	FOR_EACH(rev_item,reverse_table){
		rev_entry = (ReverseRouteEntry *) rev_item->data;
		if (reply->broadcast_id == rev_entry->broadcast_id){
			generate_JSON(reply, &data, &length);
			send_data(handler, (char *) data, length,rev_entry->previous_address);
			break;
		}
	}
	
	free(data);
	
	//CHECK REVERSE_ROUTE TABLE FOR PREVIOUS HOP AND TRANSMIT
	//IF UNSUCCESSFULL
	//START AODV (OR SIMPLY SEND TO SOURCE ADDRESS??
}

void print_rev_route(){
	LElement * rev_item;
	ReverseRouteEntry * rev_entry;
	
	FOR_EACH(rev_item,reverse_table){
		rev_entry = (ReverseRouteEntry *) rev_item->data;
		printf("========== NEW REVERSE ENTRY =========\n");
		printf("=== SOURCE: %d", rev_entry->source_address);
		printf("=== LAST NODE: %d", rev_entry->previous_address);
		printf("=== BROADCAST ID: %i", rev_entry->broadcast_id);
		printf("========== END REVERSE ENTRY =========\n\n");
	}
}
