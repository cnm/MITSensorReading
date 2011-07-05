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

void (* sensor_result)(SensorData *);

void print_state(){

}

void start_sense(){
	SensorData data;
	data->type = RSS;
}

void start_cb(void (* sensor_result_cb)(SensorData *)){
	sensor_result = sensor_result_cb;
}

void stop_cb(){
	sensor_result = NULL;
	sensor_loop = false;
}
