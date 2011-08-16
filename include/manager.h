/*
 * manager.h
 *
 *  Created on: Jun 27, 2011
 *      Author: root
 */

#ifndef MANAGER_H_
#define MANAGER_H_

void SendManagerData(uint16_t address);
void AddAggregator(uint16_t address, unsigned short frequence);
int CompareNodes(const void * key1, const void * key2);
void PrintLocation(void * info);

#endif /* MANAGER_H_ */
