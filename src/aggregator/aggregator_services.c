/*
 * manager.c
 *
 *  Created on: Jun 22, 2011
 *      Author: root
 */

void RequestInstant(uint16_t address){
	//TODO retornar ao address o estado actual do aggregator
}

void RequestFrequent(uint16_t aggregator_address, unsigned short frequency){
	//TODO: Registar o novo UMPServer com a devida frequencia para começar a enviar para estes os dados a cada frequency
}

void SpontaneousManager(uint16_t spotter_address, unsigned short max_frequency){
	//TODO: Registar o novo spotter e decidir sobre frequencia a responder ao spotter
}

void DeliverManagerData(){
	//TODO: ALL THE MAGIC! Receive Manager data and aggregate to global data
}
