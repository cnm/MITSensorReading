/*
 * location.h
 *
 *  Created on: Jun 13, 2011
 *      Author: root
 */

#ifndef LOCATION_H_
#define LOCATION_H_

typedef enum data_type{ ENTRY, COUNT, RSS} DataType;
typedef enum rss_type{ WIFI, BLUETOOTH, RFID, OTHER} RSSType;
typedef enum message_type { REGISTER_SENSOR, REGISTER_MANAGER, SENSOR_DATA, MANAGER_DATA } MessageType;

typedef struct location{
	float x;
	float y;
}Location;

typedef struct area{
	//TODO: Por definir
}Area;


typedef struct sensor_data{
	DataType type;
	union{
		struct entry{
			short entrances;
		}ENTRY;
		struct count{
			unsigned short people;
		}COUNT;
		struct rss{
			RSSType type;
			unsigned short node_number;
			unsigned int * rss;
			long * nodes;
		}RSS;
	}data;
}SensorData;

typedef struct location_packet{
	MessageType type;
	union{
		Location sensor_location;
		Area manager_area;
		SensorData data;
	};
}LocationPacket;

#endif /* LOCATION_H_ */
