#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <yajl/yajl_parse.h>
#include <yajl/yajl_gen.h>
#include <yajl/yajl_tree.h>
#include "discovery.h"
#include "listType.h"
#include <fred/handler.h>
#include <syslog.h>

bool generate_JSON(GSDPacket * packet, unsigned char ** response, size_t * length){
	
	yajl_handle hand;
    /* generator config */
    yajl_gen g;

    g = yajl_gen_alloc(NULL);
    yajl_gen_config(g, yajl_gen_beautify, 0);
    yajl_gen_config(g, yajl_gen_validate_utf8, 0);
    
    /* ok.  open file.  let's read and parse */
    hand = yajl_alloc(NULL, NULL, (void *) g);
    /* and let's allow comments by default */
    yajl_config(hand, yajl_allow_comments, 0);
	
	yajl_gen_map_open(g);
	yajl_gen_string(g, ( unsigned char *) "Packet", strlen("Packet"));
	
	yajl_gen_map_open(g);
	yajl_gen_string(g, ( unsigned char *) "GSD", strlen("GSD"));
	
	yajl_gen_map_open(g);
	yajl_gen_string(g, ( unsigned char *) "broadcast_id",strlen("broadcast_id"));
	
	yajl_gen_integer(g,packet->broadcast_id);
	
	yajl_gen_string(g, ( unsigned char *) "packet_type", strlen("packet_type"));
	yajl_gen_integer(g,packet->packet_type);
	
	yajl_gen_string(g, ( unsigned char *) "hop_count", strlen("hop_count"));
	yajl_gen_integer(g,packet->hop_count);
	
	yajl_gen_string(g, ( unsigned char *) "source_address", strlen("source_address"));
	yajl_gen_string(g, ( unsigned char *) packet->source_address, strlen(packet->source_address));
	
	switch(packet->packet_type){
		case GSD_ADVERTISE:
			yajl_gen_string(g, ( unsigned char *) "lifetime", strlen("lifetime"));
			yajl_gen_integer(g, packet->advertise->lifetime);
			
			yajl_gen_string(g, ( unsigned char *) "diameter", strlen("diameter"));
			yajl_gen_integer(g, packet->advertise->diameter);
			
			yajl_gen_string(g, ( unsigned char *) "services", strlen("services"));
			yajl_gen_array_open(g);
			
			LElement * service_elem;
			FOR_EACH(service_elem,packet->advertise->services){
				yajl_gen_map_open(g);
				yajl_gen_string(g, ( unsigned char *) "description", strlen("description"));
				yajl_gen_string(g, (unsigned char *) ((Service *) service_elem->data)->description, strlen(((Service *) service_elem->data)->description));
				
				yajl_gen_string(g, ( unsigned char *) "groups", strlen("groups"));
				LElement * group_elem;
				yajl_gen_array_open(g);
				FOR_EACH(group_elem, ((Service *)service_elem->data)->groups){
					yajl_gen_string(g, (unsigned char *) group_elem->data, strlen((char *) group_elem->data));
				}
				yajl_gen_array_close(g);
				yajl_gen_map_close(g);
			}
			yajl_gen_array_close(g);
			
			yajl_gen_string(g, ( unsigned char *) "vicinity_groups", strlen("vicinity_groups"));
			yajl_gen_array_open(g);
			
			LElement * group_elem;
			FOR_EACH(group_elem,packet->advertise->vicinity_groups){
				yajl_gen_string(g, (unsigned char *) group_elem->data, strlen((char *) group_elem->data));
			}
			yajl_gen_array_close(g);
			
			
			break;
		case GSD_REQUEST:
			yajl_gen_string(g, ( unsigned char *) "last_address", strlen("last_address"));
			yajl_gen_string(g, (unsigned char *) packet->request->last_address, strlen(packet->request->last_address));
			
			yajl_gen_string(g, ( unsigned char *) "service", strlen("service"));
			yajl_gen_map_open(g);
			
			yajl_gen_string(g, ( unsigned char *) "description", strlen("description"));
			yajl_gen_string(g, (unsigned char *) packet->request->wanted_service.description, strlen(packet->request->wanted_service.description));
			
			yajl_gen_string(g, ( unsigned char *) "groups", strlen("groups"));
			yajl_gen_array_open(g);
			LElement * elem;
			FOR_EACH(elem,packet->request->wanted_service.groups){				
				yajl_gen_string(g, (unsigned char *) elem->data, strlen((char *) elem->data));
			}
			
			yajl_gen_array_close(g);
			yajl_gen_map_close(g);
			
			break;
		default: break;
	}
	
	yajl_gen_map_close(g);
	yajl_gen_map_close(g);
	yajl_gen_map_close(g);
	
	const unsigned char * buf;
	
    yajl_gen_get_buf(g, &buf, length);
    *response = (unsigned char *) malloc((*length)*sizeof(char));
	strcpy((char*)*response, (char *)buf);
	yajl_gen_free(g);
	
	return true;
}

GSDPacket generate_packet_from_JSON(char * data){
	yajl_val node;
	char errbuf[1024];
	GSDPacket packet;
	
	node = yajl_tree_parse(( char *) data, errbuf, sizeof(errbuf));
	
	/* parse error handling */
    if (node == NULL) {
        fprintf(stderr, "parse_error: ");
        if (strlen(errbuf)) fprintf(stderr, " %s", errbuf);
        else fprintf(stderr, "unknown error");
        fprintf(stderr, "\n");
        return packet;
    }
    
    const char * path[] = { "Packet", "GSD", "packet_type", ( char *) 0 };
    packet.packet_type = YAJL_GET_INTEGER(yajl_tree_get(node, path, yajl_t_number));

    path[2] = "broadcast_id";
    packet.broadcast_id = YAJL_GET_INTEGER(yajl_tree_get(node, path, yajl_t_number));
    
    path[2] = "hop_count";
    packet.hop_count = YAJL_GET_INTEGER(yajl_tree_get(node, path, yajl_t_number));
    
    path[2] = "source_address";
    packet.source_address = YAJL_GET_STRING(yajl_tree_get(node, path, yajl_t_string));
    
	Advertisement adv;
	Request req;
	
	const char * path_in_array[] = {"description", (char *) 0};
	const char * path_to_groups[] = {"groups"};
	int i;
		
	switch(packet.packet_type){
		case GSD_ADVERTISE: 
						packet.advertise = &adv;
						
						path[2] = "lifetime";
						adv.lifetime = YAJL_GET_INTEGER(yajl_tree_get(node, path, yajl_t_number));
						path[2] = "diameter";
						adv.diameter = YAJL_GET_INTEGER(yajl_tree_get(node, path, yajl_t_number));
						path[2] = "services";
						const char * path_to_services[] = {"Packet", "GSD", "services"};
						yajl_val array = yajl_tree_get(node,path_to_services,yajl_t_array);
						CreateList(&adv.services);
						char * array_index[] = {(char *) 0};
						for (i = 0; i< YAJL_GET_ARRAY(array)->len; i++){
							sprintf(array_index[0],"%d", i);
							yajl_val serv = yajl_tree_get(array, (const char **) array_index,yajl_t_object);
							Service service;
							service.description = YAJL_GET_STRING(yajl_tree_get(serv, path_in_array, yajl_t_string));
							yajl_val groups_JSON = yajl_tree_get(serv,path_to_groups, yajl_t_array);
							CreateList(&service.groups);
							int j;
							for (j=0; j < YAJL_GET_ARRAY(groups_JSON)->len; j++){
								sprintf(array_index[0], "%d", j);
								Group group = (Group) YAJL_GET_STRING(yajl_tree_get(groups_JSON, (const char **) array_index,yajl_t_string));
								AddToList(&group, &service.groups);
							}
							AddToList(&service,&adv.services);
							yajl_tree_free(groups_JSON);
						}
						
						path_to_services[2] = "vicinity_groups";
						
						array = yajl_tree_get(node,path_to_services,yajl_t_array);
						CreateList(&adv.vicinity_groups);
						int j;
						for (j=0; j < YAJL_GET_ARRAY(array)->len; j++){
								sprintf(array_index[0], "%d", j);
								Group group = (Group) YAJL_GET_STRING(yajl_tree_get(array,(const char **) array_index,yajl_t_string));
								AddToList(&group, &adv.vicinity_groups);
						}
						
						yajl_tree_free(array);
						break;
		case GSD_REQUEST:
						packet.request = &req;
						path[2] = "last_address";
						req.last_address = YAJL_GET_STRING(yajl_tree_get(node, path, yajl_t_string));
						
						path[2] = "service";
						yajl_val object = yajl_tree_get(node,path, yajl_t_object);
						Service serv;
						
						const char * path_in_obj[] = {"description", (char *) 0};
						
						serv.description = YAJL_GET_STRING(yajl_tree_get(object,path_in_obj, yajl_t_string));
						CreateList(&serv.groups);
						
						yajl_val array_groups = yajl_tree_get(object, path_to_groups, yajl_t_array);
						
						for (i = 0; i < YAJL_GET_ARRAY(array_groups)->len ; i++){
							sprintf(array_index[0], "%d", i);
							Group group = (Group) YAJL_GET_STRING(yajl_tree_get(array_groups,(const char **) array_index,yajl_t_string));
							AddToList(&group, &serv.groups);
						}
						req.wanted_service = serv;
						
						yajl_tree_free(object);
						break;
		default: debugger("ERROR IN PACKET TYPE");
		
	}
	
	yajl_tree_free(node);
	
	
    return packet;
}
