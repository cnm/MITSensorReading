#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <stddef.h>
#include <pthread.h>
#include <syslog.h>
#include <stdint.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <fred/handler.h>
#include "spotter.h"
#include "location.h"

void (* sensor_result)(SensorData *);
static pthread_t sense_thread;

void print_state(){

}

void * sense(){
	SensorData data;
	data.type = COUNT;

	inquiry_info *ii = NULL;

	int max_rsp, num_rsp;
	int dev_id, sock, len, flags;

	dev_id = hci_get_route(NULL);
	sock = hci_open_dev( dev_id );
	if (dev_id < 0 || sock < 0) {
		perror("opening socket");
		exit(1);
	}

	len  = 8;
	max_rsp = 255;
	flags = IREQ_CACHE_FLUSH;
	ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));

	num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);
	if( num_rsp < 0 )
		perror("hci_inquiry");
	else
		data.people = num_rsp;

	/*for (i = 0; i < num_rsp; i++) {
		ba2str(&(ii+i)->bdaddr, addr);
		memset(name, 0, sizeof(name));
		if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name),
			name, 0) < 0)
		strcpy(name, "[unknown]");
		printf("%s  %s\n", addr, name);
	}*/

	free( ii );
	close( sock );

	sensor_result(&data);

	sleep(1);

	return NULL;
}

void start_sense(){
	pthread_create(&sense_thread,NULL,sense,NULL);
}

void start_cb(void (* sensor_result_cb)(SensorData *)){
	sensor_result = sensor_result_cb;
}

void stop_cb(){
	sensor_result = NULL;
}
