#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "discovery.h"
#include "listType.h"
#include "handler.h"

ReverseRouteList reverse_table;

void ReverseRoute(void){
	//TODO 
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
		printf("=== SOURCE: %s", rev_entry->source_address);
		printf("=== LAST NODE: %s", rev_entry->previous_address);
		printf("=== BROADCAST ID: %i", rev_entry->broadcast_id);
		printf("========== END REVERSE ENTRY =========\n\n");
	}
}
