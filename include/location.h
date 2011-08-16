/*
 * location.h
 *
 *  Created on: Jun 13, 2011
 *      Author: root
 */

#ifndef LOCATION_H_
#define LOCATION_H_

#include "listType.h"
#include "map.h"

typedef enum data_type{ ENTRY, COUNT, RSS} DataType;
typedef enum rss_type{ WIFI, BLUETOOTH, RFID, OTHER} RSSType;
typedef enum message_type { REQUEST_INSTANT, REQUEST_FREQUENT, REGISTER_SENSOR, REGISTER_MANAGER, CONFIRM_SENSOR, CONFIRM_MANAGER, SENSOR_DATA, MANAGER_DATA } MessageType;
typedef LList SensorDataList;


typedef struct rss{
	RSSType type;
	unsigned short node_number;
	int8_t * rss;
	unsigned char ** nodes;
}RSSInfo;

typedef struct sensor_data{
	DataType type;
	union{
		long entrances; //POS OR NEG - COUNTING NUMBER UPDATED FOR EACH CICLE
		unsigned short people;
		RSSInfo RSS;
	};
}SensorData;

typedef struct spotter{
	uint16_t id;
	Location location;
	RSSInfo * current_info;
	uint64_t last_received;
}Spotter;


typedef struct location_packet{
	MessageType type;
	union{
		struct {
			Location sensor_location;
			unsigned short min_update_frequency;
		}RegSensor;
		unsigned short required_frequency; //FOR Confirmation of spont. registry or for request frequent service
		unsigned short manager_area_id;
		SensorDataList data;
		struct {
			rb_red_blk_tree * people;
			unsigned short num_people;
		}Manager_data;
	};
}LocationPacket;

#endif /* LOCATION_H_ */
