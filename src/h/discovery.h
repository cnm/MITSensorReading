#ifndef __DISCOVERY_H__
#define __DISCOVERY_H__

#include "listType.h"

#define ENABLE_DEBUG 1

#define LOG_FILE "gsdlog"
//#define DEBUG_FILE "gsddebug"


#define logger(...) openlog(LOG_FILE, LOG_CONS, LOG_DAEMON); syslog(LOG_INFO, __VA_ARGS__); closelog();

#if ENABLE_DEBUG
	#define debugger(...) fprintf(stdout, __VA_ARGS__);
	//#define debugger(...) openlog(DEBUG_FILE, LOG_PID|LOG_CONS, LOG_USER); syslog(LOG_INFO, __VA_ARGS__); closelog();
#else
	#define debugger(...)
#endif


typedef LList CacheList;
typedef LList ReverseRouteList;
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
	ServiceList services; 
	GroupList vicinity_groups;
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
	};
	
}GSDPacket;

void * SendAdvertisement(void *);

void P2PCacheAndForwardAdvertisement(GSDPacket * message);

#endif
