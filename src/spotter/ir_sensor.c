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

#define MICRO_SECONDS 1000
#define NANO_SECONDS 1000

#define INSIDE "sa"
#define OUTSIDE "asd"
#define HIGH
#define LOW

uint64_t in_t;
uint64_t out_t;

uint64_t DELTA_MOVEMENT;

uint64_t entradas;
uint64_t saidas;

uint64_t presencas;

void loop(){
	uin64_t timestamp;
	short i = read(INSIDE);
	short o = read(OUTSIDE);
	clock_t t = clock();
	if (i==HIGH){
		in_t = t;
		if ((in_t - out_t) <= DELTA_MOVEMENT)
			entradas++;
	}
	if(o == HIGH){
		out_t = t;
		if ((out_t - in_t) <= DELTA_MOVEMENT)
			saidas++;
	}

	presencas = (entradas - saidas < 0 ? 0 : entradas - saidas);
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
