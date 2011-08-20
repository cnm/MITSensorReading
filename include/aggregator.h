/*
 * aggregator.h
 *
 *  Created on: Jul 11, 2011
 *      Author: root
 */

#ifndef AGGREGATOR_H_
#define AGGREGATOR_H_

void SendAggregatorData(uint16_t address);
void AddServer(uint16_t address, unsigned short frequence);
int CompareNodes(const void * key1, const void * key2);
void PrintLocation(void * info);

#endif /* AGGREGATOR_H_ */
