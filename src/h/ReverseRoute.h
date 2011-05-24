#ifndef __REVERSEROUTE_H__
#define __REVERSEROUTE_H__

typedef LList ReverseRouteList;

typedef struct reverse_route_entry{
	uint8_t source_address;
	unsigned short broadcast_id;
	uint8_t previous_address;
}ReverseRouteEntry;

void ReverseRoute(GSDPacket * reply);

void print_rev_route();

#endif
