#ifndef __JSONHANDLER_H__
#define __JSONHANDLER_H__

#include "discovery.h"

bool generate_JSON(GSDPacket * packet, unsigned char ** response, size_t * length);

GSDPacket generate_packet_from_JSON(char * data);

#endif
