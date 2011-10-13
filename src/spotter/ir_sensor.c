#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <stddef.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>
#include <syslog.h>
#include <stdint.h>
#include <time.h>
#include <fred/handler.h>
#include "spotter.h"
#include "location.h"

#define MICRO_SECONDS 1000
#define NANO_SECONDS 1000

#define HIGH 1
#define LOW 0

static time_t in_t,out_t;
static time_t DELTA_MOVEMENT = 3;

static uint64_t entradas,saidas;
static int presencas;

static unsigned short INSIDE_PIN = 20;
static unsigned short OUTSIDE_PIN = 6;
static bool sensor_loop = true;
static pthread_t sense_loop;

void (* sensor_result)(SensorData *);


void wait_miliseconds(int miliseconds){
    struct timespec interval, left;

    interval.tv_sec = 0;
    interval.tv_nsec = miliseconds * MICRO_SECONDS * NANO_SECONDS;

    if (nanosleep(&interval, &left) == -1) {
        if (errno != EINTR)
        	perror("nanosleep");
    }
}

int getpin(int pin){
	FILE * fp;
    int value,i;
	char result[11] = { 0 };
    char command[13] = { 0 };

    sprintf(command,"./dio get %d",pin);

    fp = popen(command,"r");
	fgets(result,sizeof(result),fp);
	pclose(fp);
    for(i=0;result[i] != '='; i++);
    i +=2;
    return result[i ] -'0';
}

void print_state(){
	printf("----ESTADO DENTRO DO PLUGIN IR_SENSOR: ENTRADAS - %lu  ;  SAIDAS - %lu ; PRESENCAS - %d \n",(unsigned long) entradas, (unsigned long) saidas, presencas);
}

void * loop(){
	short i=HIGH,last_i=LOW;
	short o=HIGH,last_o=LOW;
	SensorData data;
	data.type = ENTRY;
	while(sensor_loop){
 		i = getpin(INSIDE_PIN);
 		o = getpin(OUTSIDE_PIN);

		if (i == LOW && last_i == HIGH){
			time(&in_t);

			if ((in_t - out_t) <= DELTA_MOVEMENT){
				data.entrances = 1;
				entradas++;
				presencas++;
				sensor_result(&data);
				in_t = 0;
				out_t = 0;
				//print_state();
			}
		}

		if(o == LOW && last_o == HIGH){
			time(&out_t);

			if ((out_t - in_t)  <= DELTA_MOVEMENT){
				data.entrances = -1;
				saidas++;
				presencas--;
				sensor_result(&data);
				in_t = 0;
				out_t = 0;
				//print_state();
			}

		}
		last_i=i;
		last_o=o;
		
		wait_miliseconds(25);
	}

	return NULL;
}

void start_cb(void (* sensor_result_cb)(SensorData *)){
	sensor_result = sensor_result_cb;
	sensor_loop = true;
	pthread_create(&sense_loop,NULL,loop,NULL);
}

void stop_cb(){
	sensor_result = NULL;
	sensor_loop = false;
}
