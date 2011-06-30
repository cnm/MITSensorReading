/*
 * gsd_api.h
 *
 *  Created on: Jun 23, 2011
 *      Author: root
 */

#ifndef GSD_API_H_
#define GSD_API_H_

#include "discovery.h"

bool RegisterService(Service *, uint16_t handler_id, void (* service_found_cb)(uint16_t));
void RequestService(Service *);
bool StopProvidingService(uint16_t handler_id);

#endif /* GSD_API_H_ */
