/*
 * map.c
 *
 *  Created on: Jul 28, 2011
 *      Author: root
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <yajl/yajl_parse.h>
#include <yajl/yajl_tree.h>
#include "listType.h"
#include "location.h"
#include "location_json.h"
#include <fred/handler.h>



Map * LoadMap(char * map_info){

	yajl_val node;
	char errbuf[1024];
	short x,y;
	short i;

	Map * map = (Map *) malloc(sizeof(Map));


	//logger("Parsing packet from JSON message");

	node = yajl_tree_parse(( char *) map_info, errbuf, sizeof(errbuf));

	 if (node == NULL) {
	        fprintf(stderr,"parse_error: ");
	        if (strlen(errbuf)){
	        	fprintf(stderr," %s", errbuf);
	        }else{
	        	fprintf(stderr,"unknown error");
			}
	        fprintf(stderr,"\n");
	        return false;
	    }

	 const char * path[] = { "map_id", ( char *) 0 };

	 map->area_id = YAJL_GET_INTEGER(yajl_tree_get(node, path, yajl_t_number));

	 path[0] = "cell_size";
	 map->cell_size = YAJL_GET_INTEGER(yajl_tree_get(node, path, yajl_t_number));

	 path[0] = "x_cells";
	 map->x_cells = YAJL_GET_INTEGER(yajl_tree_get(node, path, yajl_t_number));

	 path[0] = "y_cells";
	 map->y_cells = YAJL_GET_INTEGER(yajl_tree_get(node, path, yajl_t_number));

	 map->cells = (Cell **) malloc(map->x_cells * sizeof(Cell *));
	 for (x = 0; x < map->x_cells; x++){
		 map->cells[x] = (Cell *) malloc(map->y_cells * sizeof(Cell));
	 }

	 path[0] = "cells";

	 for (x = 0; x < map->x_cells; x++)
		 for (y = 0; y < map->y_cells; y++){

			 map->cells[x][y].prob_location = 1;
			 map->cells[x][y].transition_cell = true;

			 yajl_val cell = YAJL_GET_ARRAY(YAJL_GET_ARRAY((yajl_tree_get(node,path, yajl_t_array)))->values[x])->values[y];

			 if (YAJL_IS_INTEGER(cell)){
				 map->cells[x][y].prob_location = YAJL_GET_INTEGER(YAJL_GET_ARRAY(YAJL_GET_ARRAY((yajl_tree_get(node,path, yajl_t_array)))->values[x])->values[y]);
			 	 map->cells[x][y].transition_cell = false;
			 }else if (YAJL_IS_NULL(cell)){
				 map->cells[x][y].transp.area_id = 0;
				 map->cells[x][y].transp.x_cell = 0;
				 map->cells[x][y].transp.y_cell = 0;
			 }else{
				 for (i = 0; i < YAJL_GET_OBJECT(cell)->len; i++){
					 if (!strcmp(YAJL_GET_OBJECT(cell)->keys[i], "area_id"))
						 map->cells[x][y].transp.area_id = YAJL_GET_INTEGER(YAJL_GET_OBJECT(cell)->values[i]);
					 else if (!strcmp(YAJL_GET_OBJECT(cell)->keys[i], "x_cell"))
						 map->cells[x][y].transp.x_cell = YAJL_GET_INTEGER(YAJL_GET_OBJECT(cell)->values[i]);
					 else if (!strcmp(YAJL_GET_OBJECT(cell)->keys[i], "y_cell"))
						 map->cells[x][y].transp.y_cell = YAJL_GET_INTEGER(YAJL_GET_OBJECT(cell)->values[i]);
				 }
			 }
		 }

	 return map;

}

void DestroyMap(Map * map){
	short x;
	for (x=0; x < map->x_cells ; x++)
			free(map->cells[x]);
	free(map->cells);
	free(map);
}

Location * InfoToCell(Map * map, float x, float y){
	//Todo calc the cell Correspondent to the computed info
	return NULL;
}
