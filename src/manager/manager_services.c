/*
 * manager.c
 *
 *  Created on: Jun 22, 2011
 *      Author: root
 */
#include"location.h"

void RequestInstant(uint16_t address){
	//TODO retornar ao address o estado actual do manager
}

void RequestFrequent(uint16_t aggregator_address, unsigned short frequency){
	//TODO: Registar o novo Aggregator com a devida frequencia para começar a enviar para estes os dados a cada frequency
}

void ConfirmSpotter(uint16_t spotter_address, Location location, unsigned short final_frequency){
	//TODO:
}

void SpontaneousSpotter(uint16_t spotter_address, unsigned short max_frequency){
	//TODO: Registar o novo spotter e decidir sobre frequencia a responder ao spotter
}

void DeliverSpotterData(){
	//TODO: ALL THE MAGIC! Receive spotter data and compute the location of people in this area!
}

void ConfirmSpontaneousRegister(){
	//TODO: Confirmar o registo pedido por este nó; recebe a frequencia final com que vai emitir ao Aggregator
}
