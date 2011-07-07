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
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <fred/handler.h>
#include <openssl/md5.h>
#include "spotter.h"
#include "location.h"

void (* sensor_result)(SensorData *);
static pthread_t sense_thread;

static void print_result(bdaddr_t *bdaddr, char has_rssi, int rssi)
{
	char addr[18];

	ba2str(bdaddr, addr);

	printf("%17s", addr);
	if(has_rssi)
		printf(" RSSI:%d", rssi);
	else
		printf(" RSSI:n/a");
	printf("\n");
	fflush(NULL);
}

void print_state(){

}

static void scanner_start()
{
	int dev_id, sock = 0;
	struct hci_filter flt;
	write_inquiry_mode_cp cp;
	unsigned char buf[HCI_MAX_EVENT_SIZE], *ptr;
	hci_event_hdr *hdr;
	char canceled = 0;
	inquiry_info_with_rssi *info_rssi;
	inquiry_info *info;
	int results, i, len;
	struct pollfd p;

	dev_id = hci_get_route(NULL);
	sock = hci_open_dev( dev_id );
	if (dev_id < 0 || sock < 0) {
		perror("Can't open socket");
		return;
	}

	hci_filter_clear(&flt);
	hci_filter_set_ptype(HCI_EVENT_PKT, &flt);
	hci_filter_set_event(EVT_INQUIRY_RESULT, &flt);
	hci_filter_set_event(EVT_INQUIRY_RESULT_WITH_RSSI, &flt);
	hci_filter_set_event(EVT_INQUIRY_COMPLETE, &flt);
	if (setsockopt(sock, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0) {
		perror("Can't set HCI filter");
		return;
	}

	memset (&cp, 0, sizeof(cp));
	cp.mode = 0x01;

	if (hci_send_cmd(sock, OGF_HOST_CTL, OCF_WRITE_INQUIRY_MODE, WRITE_INQUIRY_MODE_RP_SIZE, &cp) < 0) {
		perror("Can't set inquiry mode");
		return;
	}

	printf("Starting inquiry with RSSI...\n");

	if (hci_send_cmd (sock, OGF_LINK_CTL, OCF_INQUIRY, INQUIRY_CP_SIZE, &cp) < 0) {
		perror("Can't start inquiry");
		return;
	}

	p.fd = sock;
	p.events = POLLIN | POLLERR | POLLHUP;

	while(!canceled) {
		p.revents = 0;

		/* poll the BT device for an event */
		if (poll(&p, 1, -1) > 0) {
			len = read(sock, buf, sizeof(buf));

			if (len < 0)
				continue;
			else if (len == 0)
				break;

			hdr = (void *) (buf + 1);
			ptr = buf + (1 + HCI_EVENT_HDR_SIZE);

			results = ptr[0];

			switch (hdr->evt) {
				case EVT_INQUIRY_RESULT:
					for (i = 0; i < results; i++) {
						info = (void *)ptr + (sizeof(*info) * i) + 1;
						print_result(&info->bdaddr, 0, 0);
					}
					break;

				case EVT_INQUIRY_RESULT_WITH_RSSI:
					for (i = 0; i < results; i++) {
						info_rssi = (void *)ptr + (sizeof(*info_rssi) * i) + 1;
						print_result(&info_rssi->bdaddr, 1, info_rssi->rssi);
					}
					break;

				case EVT_INQUIRY_COMPLETE:
					printf("COMPLETE\n");
					canceled = 1;
					break;
			}
		}
	}
	close(sock);
}

void * sense(){

	scanner_start();
	/*SensorData data;
	SensorData rss_data;
    char addr[19] = { 0 };
    int i;

	inquiry_info_with_rssi *ii = NULL;

	int max_rsp, num_rsp;
	int dev_id, sock, len, flags;

	data.type = COUNT;

	dev_id = hci_get_route(NULL);
	sock = hci_open_dev( dev_id );
	if (dev_id < 0 || sock < 0) {
		perror("opening socket");
		exit(1);
	}

	len  = 8;
	max_rsp = 255;
	flags = IREQ_CACHE_FLUSH;
	ii = (inquiry_info_with_rssi*)malloc(max_rsp * sizeof(inquiry_info_with_rssi));

	num_rsp = hci_(dev_id, len, max_rsp, NULL, &ii, flags);
	if( num_rsp < 0 ){
		perror("hci_inquiry");
		data.people = 0;
	}else
		data.people = num_rsp;

	printf("DETECTED %d PEOPLE",num_rsp);
	rss_data.RSS.node_number = num_rsp;
	rss_data.RSS.type = BLUETOOTH;
	rss_data.RSS.nodes = (unsigned char **) malloc(MD5_DIGEST_LENGTH*num_rsp);
	rss_data.RSS.rss = (int8_t *) malloc(sizeof(int8_t)* num_rsp);


	for (i = 0; i < num_rsp; i++) {
		ba2str(&(ii+i)->bdaddr, addr);
		MD5((unsigned char *) addr,strlen(addr),rss_data.RSS.nodes[i]);
		rss_data.RSS.rss[i] = &(ii+1)->rssi;
	}


	free( ii );
	close( sock );

	sensor_result(&data);
	sensor_result(&rss_data);

	sleep(1);

	return NULL;
	*/
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
