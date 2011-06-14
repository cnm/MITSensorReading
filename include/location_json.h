/*
 * location_json.h
 *
 *  Created on: Jun 14, 2011
 *      Author: root
 */

#ifndef LOCATION_JSON_H_
#define LOCATION_JSON_H_

#include "location.h"

bool generate_JSON(LocationPacket * packet, unsigned char ** response, size_t * length);

bool generate_packet_from_JSON(char * data, LocationPacket * packet);

#endif /* LOCATION_JSON_H_ */
