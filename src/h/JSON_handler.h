#ifndef __JSONHANDLER_H__
#define __JSONHANDLER_H__

#include "discovery.h"

bool generate_JSON(GSDPacket * packet, unsigned char ** response, size_t * length);

bool generate_packet_from_JSON(char * data, GSDPacket * packet);

bool Import_Local_Services(char *);

#endif
