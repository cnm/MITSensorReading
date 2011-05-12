#ifndef __DISCOVERY_H__
#define __DISCOVERY_H__

#include "listType.h"


typedef LList CacheList;
typedef LList ReverseRouteList;
typedef char * Service;
typedef char * Group;
typedef LList GroupList;
typedef LList ServiceList;

typedef enum PckType { GSD_ADVERTISE, GSD_REQUEST } PacketType;

typedef struct service_description{
	char * description;
	GroupList groups;
}Service;

typedef struct service_cache{
	char* source_address;
	bool local;
	LList descriptions; 
	LList groups;
	LList vicinity_groups;
	unsigned short lifetime;
}ServiceCache;

typedef struct reverse_route_entry{
	char* source_address;
	unsigned short broadcast_id;
	char* previous_address;
}ReverseRouteEntry;

typedef struct advertisement_struct{
	ServiceList services; 
	GroupList vicinity_groups;
	unsigned short lifetime;
	unsigned short diameter;
}Advertisement;

typedef struct request_struct{
	Service wanted_service;
	char* last_address;
}Request;

typedef struct gsd_pkt{
	unsigned long broadcast_id;
	PacketType packet_type;
	unsigned short hop_count;
	char* source_address;
	
	union{
		Advertisement * advertise;
		Request * request;
	}
}GSDPacket;

void * SendAdvertisement(void *);

void P2PCacheAndForwardAdvertisement(Advertisement message);

#endif
