/*
 * location.h
 *
 *  Created on: Jun 13, 2011
 *      Author: root
 */

#ifndef LOCATION_H_
#define LOCATION_H_

#include "listType.h"

typedef enum data_type{ ENTRY, COUNT, RSS} DataType;
typedef enum rss_type{ WIFI, BLUETOOTH, RFID, OTHER} RSSType;
typedef enum message_type { REQUEST_INSTANT, REQUEST_FREQUENT, REGISTER_SENSOR, REGISTER_MANAGER, SENSOR_DATA, MANAGER_DATA } MessageType;
typedef LList SensorDataList;

typedef struct transition{
	unsigned short area_id;
	unsigned short x_cell;
	unsigned short y_cell;
}Transition;

typedef struct cell{
	unsigned short prob_location;
	unsigned short prob_move[6];
	bool transition_cell;
	Transition transp;
}Cell;

typedef struct map{
	unsigned short area_id;
	Cell **cells;
	unsigned short x_cells;
	unsigned short y_cells;
	unsigned short floor;
	unsigned short cell_size;
}Map;

typedef struct location{
	float x;
	float y;
	Map a;
}Location;

typedef struct sensor_data{
	DataType type;
	union{
		long entrances; //POS OR NEG - COUNTING NUMBER UPDATED FOR EACH CICLE
		unsigned short people;
		struct rss{
			RSSType type;
			unsigned short node_number;
			int8_t * rss;
			unsigned char ** nodes;
		}RSS;
	};
}SensorData;

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
	};
}LocationPacket;

#endif /* LOCATION_H_ */
