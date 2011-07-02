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

/*******************************************************************************
* getdiopin: accepts a DIO pin number and returns its value.
*******************************************************************************/
int getdiopin(int pin)
{
   int pinOffSet;
   int pinValue = 99999;

   /*******************************************************************
   *0x66: DIO and tagmem control (RW)
   *  bit 15-12: DIO input for pins 40(MSB)-37(LSB) (RO)
   *  bit 11-8: DIO output for pins 40(MSB)-37(LSB) (RW)
   *  bit 7-4: DIO direction for pins 40(MSB)-37(LSB) (1 - output) (RW)
   ********************************************************************/
   if (pin <= 40 && pin >= 37)
   {
      pinOffSet = pin - 25; // -37 to get to 0, + 10 to correct offset

      // Obtain the specific pin value (1 or 0)
      pinValue = (sbus_peek16(0x66) >> pinOffSet) & 0x0001;
   }

   /*********************************************************************
   *0x68: DIO input for pins 36(MSB)-21(LSB) (RO)
   *0x6a: DIO output for pins 36(MSB)-21(LSB) (RW)
   *0x6c: DIO direction for pins 36(MSB)-21(LSB) (1 - output) (RW)
   *********************************************************************/
   else if (pin <= 36 && pin >= 21)
   {
      pinOffSet = pin - 21; // Easier to understand when LSB = 0 and MSB = 15

      // Obtain the specific pin value (1 or 0)
      pinValue = (sbus_peek16(0x68) >> pinOffSet) & 0x0001;
   }

   /*********************************************************************
   *0x6e: DIO input for pins 20(MSB)-5(LSB) (RO)
   *0x70: DIO output for pins 20(MSB)-5(LSB) (RW)
   *0x72: DIO direction for pins 20(MSB)-5(LSB) (1 - output) (RW)
   *********************************************************************/
   else if (pin <= 20 && pin >= 5)
   {
      pinOffSet = pin - 5;  // Easier to understand when LSB = 0 and MSB = 15

      // Obtain the specific pin value (1 or 0)
      pinValue = (sbus_peek16(0x6e) >> pinOffSet) & 0x0001;
   }
   return pinValue;
}

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
int getpin(int pin){
    int value;
    printf("GOING TO DO SBUSLOCK!\n");

    printf("DONE SBUSLOCK! GOING TO GETDIOPIN\n");
    value = getdiopin(pin);
    printf("DONE GETDIOPIN! GOING TO SBUSUNLOCK!\n");
    sbusunlock();
    printf("UNLOCKEDD!!!");

    return value;
}

void print_state(){
	printf("IR_SENSOR PLUGIN: ENTRADAS - %lu  ;  SAIDAS - %lu ; PRESENCAS - %u",entradas,saidas, presencas);
}

void * loop(){
	short i=1,last_i=0;
	short o=1,last_o=0;
	SensorData data;
	//sbuslock();
	char result[11];
	FILE * fp;
	while(sensor_loop){
		//printf("going to get dio 21\n");
 		fp = popen("./dio get 21","r");
 		fgets(result,sizeof(result),fp);
 		pclose(fp);
 		i = atoi(&result[strlen(result) - 1]);
 		//printf("going to get dio 25\n");
 		fp = popen("./dio get 25","r");
 		fgets(result,sizeof(result),fp);
 		pclose(fp);
 		o = atoi(&result[strlen(result) - 1]);
		/*printf("vou ler os pins\n");
		i = getdiopin(INSIDE_PIN);
		o = getdiopin(OUTSIDE_PIN);
		*/
		//printf("getpins i:%d  o:%d\n",i,o);
		if (i == LOW && last_i == HIGH){
			time(&in_t);
			printf("LOW NO DE DENTRO!\n");
			if ((in_t - out_t) <= DELTA_MOVEMENT){
				printf("NOVA ENTRADA\n");
				data.entrances = 1;
				entradas++;
				sensor_result(&data);
				print_state();
			}
		}
		if(o == LOW && last_o == HIGH){
			time(&out_t);
			printf("LOW NO DE FORA!\n");
			if ((out_t - in_t)  <= DELTA_MOVEMENT){
				printf("NOVA SAIDA\n");
				data.entrances = -1;
				sensor_result(&data);
				saidas++;
				print_state();
			}

		}
		last_i=i;
		last_o=o;
		presencas = (entradas - saidas < 0 ? 0 : entradas - saidas);
		wait_miliseconds(50);
	}
	sbusunlock();
}

void start_cb(void (* sensor_result_cb)(SensorData *)){
	sensor_result = sensor_result_cb;
	pthread_create(&sense_loop,NULL,loop,NULL);
}

void stop_cb(){
	sbusunlock();
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
