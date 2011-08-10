/*
 * map.h
 *
 *  Created on: Aug 4, 2011
 *      Author: root
 */

#ifndef MAP_H_
#define MAP_H_

typedef struct transition{
	unsigned short area_id;
	unsigned short x_cell;
	unsigned short y_cell;
}Transition;

typedef struct cell{
	unsigned short prob_location;
	//unsigned short prob_move[6];
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
	unsigned short x;
	unsigned short y;
	unsigned short area_id;
}Location;

#endif /* MAP_H_ */