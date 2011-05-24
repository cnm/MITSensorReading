#ifndef __DISCOVERY_H__
#define __DISCOVERY_H__

#include "listType.h"
#include <stdint.h>

#define ENABLE_DEBUG 1

#define LOG_FILE "gsdlog"

#define logger(...) openlog(LOG_FILE, LOG_CONS, LOG_DAEMON); syslog(LOG_INFO, __VA_ARGS__); closelog();

#ifdef ENABLE_DEBUG
	#define debugger(...) printf(__VA_ARGS__);
	//#define debugger(...) openlog(DEBUG_FILE, LOG_PID|LOG_CONS, LOG_USER); syslog(LOG_INFO, __VA_ARGS__); closelog();
#else
	#define debugger(...)
#endif

typedef char * Group;
typedef LList GroupList;
typedef LList ServiceList;
typedef LList RequestList;

typedef enum PckType { GSD_ADVERTISE, GSD_REQUEST, GSD_REPLY } PacketType;

typedef struct service_description{
	char * description;
	GroupList groups;
}Service;

typedef struct service_reply{
	uint8_t dest_address;
}ServiceReply;

typedef struct advertisement_struct{
	ServiceList services; 
	GroupList vicinity_groups;
	unsigned short lifetime;
	unsigned short diameter;
}Advertisement;

typedef struct request_struct{
	Service wanted_service;
	uint8_t last_address;
}Request;

typedef struct local_request{
	uint8_t local_address;
	unsigned long broadcast_id;
	unsigned short request_id;
}LocalRequest;

typedef struct gsd_pkt{
	unsigned long broadcast_id;
	PacketType packet_type;
	unsigned short hop_count;
	uint8_t source_address;
	
	union{
		Advertisement * advertise;
		Request * request;
		ServiceReply * reply;
	};
	
}GSDPacket;

void * SendAdvertisement(void *);

void P2PCacheAndForwardAdvertisement(GSDPacket * message, uint8_t src_id);

#endif
