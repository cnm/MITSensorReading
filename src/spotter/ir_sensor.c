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
#include "sbus.h"

#define MICRO_SECONDS 1000
#define NANO_SECONDS 1000

#define HIGH 1
#define LOW 0

static time_t in_t,out_t;
static time_t DELTA_MOVEMENT = 3;

static uint64_t entradas,saidas;
static unsigned int presencas;

static unsigned short INSIDE_PIN = 21;
static unsigned short OUTSIDE_PIN = 25;
static bool sensor_loop = true;
static pthread_t sense_loop;

void (* sensor_result)(SensorData *);


void wait_miliseconds(int miliseconds){
    struct timespec interval, left;

    interval.tv_sec = 0;
    interval.tv_nsec = miliseconds * MICRO_SECONDS * NANO_SECONDS;

    if (nanosleep(&interval, &left) == -1) {
        if (errno == EINTR) {
            printf("nanosleep interrupted\n");
            printf("Remaining secs: %d\n", left.tv_sec);
            printf("Remaining nsecs: %d\n", left.tv_nsec);
        }
        else perror("nanosleep");
    }
}

void wait_seconds(int seconds){

    struct timespec interval, left;

    interval.tv_sec = seconds;
    interval.tv_nsec = 0;

    if (nanosleep(&interval, &left) == -1) {
        if (errno == EINTR) {
            printf("nanosleep interrupted\n");
            printf("Remaining secs: %d\n", left.tv_sec);
            printf("Remaining nsecs: %d\n", left.tv_nsec);
        }
        else perror("nanosleep");
    }
}

void setpin(int pin, int value){
    sbuslock();
    setdiopin(pin, value);
    sbusunlock();
}

int getpin(int pin){
    int value;
    sbuslock();
    value = getdiopin(pin);
    sbusunlock();

    return value;
}

void * loop(){
	short i;
	short o;
	SensorData data;
	while(sensor_loop){
		printf("getpin i\n");
		i = getpin(INSIDE_PIN);
		printf("getpin o\n");
		o = getpin(OUTSIDE_PIN);

		if (i == LOW){
			time(&in_t);
			printf("LOW NO DE DENTRO!\n");
			if ((in_t - out_t) <= DELTA_MOVEMENT){
				printf("NOVA ENTRADA\n");
				data.entrances = 1;
				entradas++;
				sensor_result(&data);
			}
		}
		if(o == LOW){
			time(&out_t);
			printf("LOW NO DE FORA!\n");
			if ((out_t - in_t)  <= DELTA_MOVEMENT){
				printf("NOVA SAIDA\n");
				data.entrances = -1;
				sensor_result(&data);
				saidas++;
			}

		}

		presencas = (entradas - saidas < 0 ? 0 : entradas - saidas);
		wait_miliseconds(50);
	}
}

void start_cb(void (* sensor_result_cb)(SensorData *)){
	sensor_result = sensor_result_cb;
	pthread_create(&sense_loop,NULL,loop,NULL);
}

void stop_cb(){
	sensor_result = NULL;
	sensor_loop = false;
	pthread_kill(sense_loop,0);
}
/*
int main(int argc, char * argv[]){
	int duration_limit = atoi(argv[1]);
	int limit = 16*duration_limit;
	int val = 0;
	int pin = atoi(argv[2]);
	int beforeVal = 0;
	time_t start, end;
	double duration = 0;
	time(&start);
	while(--limit>0 && duration <= duration_limit){
		val = getpin(pin);
		if (beforeVal != val){
			time(&end);
			duration = difftime(end, start);
			printf("time: %.2f; Value: %d\n",duration,val);
			beforeVal = val;
		}
		wait_miliseconds(50);
	}
	time(&end);
	duration = difftime(end, start);
	printf("limit : %d ; Total time: %.2f\n", limit,duration);
}
*/
