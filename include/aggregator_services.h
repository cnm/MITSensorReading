/*
 * aggregator_services.h
 *
 *  Created on: Jul 11, 2011
 *      Author: root
 */

#ifndef AGGREGATOR_SERVICES_H_
#define AGGREGATOR_SERVICES_H_

void RequestInstant(uint16_t address);

void RequestFrequent(uint16_t aggregator_address, unsigned short frequency);

void SpontaneousManager(uint16_t spotter_address, unsigned short max_frequency);

void DeliverManagerData();

#endif /* AGGREGATOR_SERVICES_H_ */
