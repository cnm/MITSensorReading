#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <yajl/yajl_parse.h>
#include <yajl/yajl_gen.h>
#include <yajl/yajl_tree.h>
#include "discovery.h"
#include "listType.h"
#include "ServiceCache.h"
#include <fred/handler.h>
#include <syslog.h>

extern uint16_t MYADDRESS;
extern char* MYIPADDRESS;
extern CacheList cache;

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
	yajl_gen_integer(g,packet->source_address);
	
	yajl_gen_string(g, ( unsigned char *) "gsd_ip_address", strlen("gsd_ip_address"));
	yajl_gen_string(g,(unsigned char *) packet->gsd_ip_address, strlen(packet->gsd_ip_address));
	
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
				
				yajl_gen_string(g, ( unsigned char *) "ip_address", strlen("ip_address"));
				yajl_gen_string(g,(unsigned char *) ((Service *) service_elem->data)->ip_address, strlen(((Service *) service_elem->data)->ip_address));
				
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
			yajl_gen_integer(g, packet->request->last_address);
			
			yajl_gen_string(g, ( unsigned char *) "ip_last_address", strlen("ip_last_address"));
			yajl_gen_string(g,(unsigned char *) packet->request->ip_last_address, strlen(packet->request->ip_last_address));
			
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
		case GSD_REPLY:
			yajl_gen_string(g, (unsigned char *) "dest_address", strlen("dest_address"));
			yajl_gen_integer(g, packet->reply->dest_address);
			
			yajl_gen_string(g, ( unsigned char *) "ip_address", strlen("ip_address"));
			yajl_gen_string(g,(unsigned char *) packet->reply->ip_address, strlen(packet->reply->ip_address));
			break;
		default: break;
	}
	
	yajl_gen_map_close(g);
	yajl_gen_map_close(g);
	yajl_gen_map_close(g);
	
	const unsigned char * buf;
	
    yajl_gen_get_buf(g, &buf, length);
    *response = (unsigned char *) malloc((*length + 1)*sizeof(char));
	strcpy((char*)*response, (char *)buf);
	yajl_gen_free(g);
	
	return true;
}

bool generate_packet_from_JSON(char * data, GSDPacket * packet){
	
	yajl_val node;
	char errbuf[1024];
	Advertisement adv;
	Request req;
	ServiceReply reply;
	char * tmp;
	
	logger("Parsing packet from JSON message");
	
	node = yajl_tree_parse(( char *) data, errbuf, sizeof(errbuf));
	
	/* parse error handling */
    if (node == NULL) {
        logger("parse_error: ");
        if (strlen(errbuf)){
			logger(" %s", errbuf);
        }else{
			logger("unknown error");
		}
        logger("\n");
        return false;
    }
    
    const char * path[] = { "Packet", "GSD", "packet_type", ( char *) 0 };
    packet->packet_type = YAJL_GET_INTEGER(yajl_tree_get(node, path, yajl_t_number));

    path[2] = "broadcast_id";
    packet->broadcast_id = YAJL_GET_INTEGER(yajl_tree_get(node, path, yajl_t_number));
    
    path[2] = "hop_count";
    packet->hop_count = YAJL_GET_INTEGER(yajl_tree_get(node, path, yajl_t_number));
    
    path[2] = "source_address";
    packet->source_address = YAJL_GET_INTEGER(yajl_tree_get(node, path, yajl_t_number));
    
	path[2] = "gsd_ip_address";
	tmp = YAJL_GET_STRING(yajl_tree_get(node, path, yajl_t_string));
	packet->gsd_ip_address = (char *) malloc(strlen(tmp)+1);
	memcpy(packet->gsd_ip_address, tmp, strlen(tmp)+1);
	
	int i,j;
		
	switch(packet->packet_type){
		case GSD_ADVERTISE: 
						packet->advertise = &adv;
						
						path[2] = "lifetime";
						adv.lifetime = YAJL_GET_INTEGER(yajl_tree_get(node, path, yajl_t_number));
						path[2] = "diameter";
						adv.diameter = YAJL_GET_INTEGER(yajl_tree_get(node, path, yajl_t_number));
						path[2] = "services";
						const char * path_to_services[] = {"Packet", "GSD", "services"};
						yajl_val array = yajl_tree_get(node,path_to_services,yajl_t_array);
						CreateList(&adv.services);
						for (i = 0; i< YAJL_GET_ARRAY(array)->len; i++){
							Service service;
							
							yajl_val obj = YAJL_GET_ARRAY(array)->values[i];
							yajl_val groups_JSON;
							
							for (j = 0; j < YAJL_GET_OBJECT(obj)->len; j++){
								if (!strcmp(YAJL_GET_OBJECT(obj)->keys[j],"description")){
									tmp = YAJL_GET_STRING(YAJL_GET_OBJECT(obj)->values[j]);
									service.description = (char *) malloc(strlen(tmp)+1);
									memcpy(service.description, tmp,strlen(tmp)+1);
								}else if (!strcmp(YAJL_GET_OBJECT(obj)->keys[j],"ip_address")){
									tmp = YAJL_GET_STRING(YAJL_GET_OBJECT(obj)->values[j]);
									service.ip_address = (char *) malloc(strlen(tmp)+1);
									memcpy(service.ip_address, tmp,strlen(tmp)+1);	
								}else if (!strcmp(YAJL_GET_OBJECT(obj)->keys[j],"groups"))
									groups_JSON = YAJL_GET_OBJECT(obj)->values[j];
								
							}
							
							CreateList(&service.groups);
							for (j=0; j < YAJL_GET_ARRAY(groups_JSON)->len; j++){
								tmp = YAJL_GET_STRING(YAJL_GET_ARRAY(groups_JSON)->values[j]);
								Group group = (char *) malloc((strlen(tmp)+1)*sizeof(char));
								strcpy(group, tmp);
								AddToList(&group, &service.groups);
							}
							AddToList(&service,&adv.services);
						}
						
						path_to_services[2] = "vicinity_groups";
						
						array = yajl_tree_get(node,path_to_services,yajl_t_array);
						CreateList(&adv.vicinity_groups);
						for (j=0; j < YAJL_GET_ARRAY(array)->len; j++){
								tmp = YAJL_GET_STRING(YAJL_GET_ARRAY(array)->values[j]);
								Group group = (char *) malloc((strlen(tmp)+1)*sizeof(char));
								strcpy(group, tmp);
								AddToList(&group, &adv.vicinity_groups);
						}
						
						break;
		case GSD_REQUEST:
						packet->request = &req;
						path[2] = "last_address";
						req.last_address = YAJL_GET_INTEGER(yajl_tree_get(node, path, yajl_t_number));
						
						path[2] = "ip_last_address";
						tmp = YAJL_GET_STRING(yajl_tree_get(node, path, yajl_t_string));
						req.ip_last_address = (char *) malloc(strlen(tmp)+1);
						memcpy(req.ip_last_address, tmp, strlen(tmp)+1);

						path[2] = "service";
						yajl_val obj = yajl_tree_get(node,path, yajl_t_object);
						yajl_val array_groups;
						Service serv;
						
						for (j = 0; j< YAJL_GET_OBJECT(obj)->len; j++){
								if (!strcmp(YAJL_GET_OBJECT(obj)->keys[j],"description")){
									tmp = YAJL_GET_STRING(YAJL_GET_OBJECT(obj)->values[j]);
									serv.description = (char *) malloc(strlen(tmp)+1);
									memcpy(serv.description, tmp,strlen(tmp)+1);
								}else if (!strcmp(YAJL_GET_OBJECT(obj)->keys[j],"groups"))
									array_groups = YAJL_GET_OBJECT(obj)->values[j];
								
						}
						
						CreateList(&serv.groups);
						
						for (i = 0; i < YAJL_GET_ARRAY(array_groups)->len ; i++){
							tmp = YAJL_GET_STRING(YAJL_GET_ARRAY(array_groups)->values[i]);
							Group group = (char *) malloc((strlen(tmp)+1)*sizeof(char));
							strcpy(group, tmp);
							AddToList(&group, &serv.groups);
						}
						req.wanted_service = serv;
						
						break;
		case GSD_REPLY:
						packet->reply = &reply;
						path[2] = "dest_address";
						reply.dest_address = YAJL_GET_INTEGER(yajl_tree_get(node, path, yajl_t_number));
						
						path[2] = "ip_address";
						tmp = YAJL_GET_STRING(yajl_tree_get(node, path, yajl_t_string));
						reply.ip_address = (char *) malloc(strlen(tmp)+1);
						memcpy(reply.ip_address, tmp, strlen(tmp)+1);
						break;
		default: debugger("ERROR IN PACKET TYPE");
		
	}
	
	yajl_tree_free(node);
	
	
    return true;
}

bool Import_Local_Services(char * data){

	yajl_val node, groups_JSON;
	char errbuf[1024];
	char * tmp;
	int i,j;
	
	logger("Importing Local Services");
	
	node = yajl_tree_parse(( char *) data, errbuf, sizeof(errbuf));
	
	/* parse error handling */
    if (node == NULL) {
        printf("parse_error: ");
        if (strlen(errbuf)){
			printf(" %s\n", errbuf);
        }else{
			printf("unknown error\n");
		}
        return false;
    }
    
    yajl_val array = YAJL_GET_OBJECT(node)->values[0];
    
    ServiceCache * entry = (ServiceCache *) malloc(sizeof(ServiceCache));
    entry->local = true;
	entry->lifetime = 0;
	entry->broadcast_id = 0;
	entry->source_address = MYADDRESS;
	
	CreateList(&entry->services);
	CreateList(&entry->vicinity_groups);
    
    for (i=0; i < YAJL_GET_ARRAY(array)->len; i++){
		
		Service * service = (Service *) malloc(sizeof(Service));			
		
		CreateList(&service->groups);				
		
		yajl_val obj = YAJL_GET_ARRAY(array)->values[i];		
		
		for (j=0; j < YAJL_GET_OBJECT(obj)->len; j++){			
			
			if (!strcmp("description", YAJL_GET_OBJECT(obj)->keys[j])){
				tmp = YAJL_GET_STRING(YAJL_GET_OBJECT(obj)->values[j]);
				service->description = (char *) malloc(strlen(tmp)+1);
				memcpy(service->description, tmp, strlen(tmp)+1);
			}else if (!strcmp("port", YAJL_GET_OBJECT(obj)->keys[j])){
				int port = YAJL_GET_INTEGER(YAJL_GET_OBJECT(obj)->values[j]);
				service->ip_address = (char *) malloc(strlen(MYIPADDRESS)+7);
				memset(service->ip_address, 0, strlen(MYIPADDRESS)+7);
				sprintf(service->ip_address, "%s:%d", MYIPADDRESS, port);
			}else if (!strcmp("address", YAJL_GET_OBJECT(obj)->keys[j])){
				service->address = YAJL_GET_INTEGER(YAJL_GET_OBJECT(obj)->values[j]);
			}else if (!strcmp("groups", YAJL_GET_OBJECT(obj)->keys[j])){
				groups_JSON = YAJL_GET_OBJECT(obj)->values[j];
			}
		}
		
		for (j=0; j < YAJL_GET_ARRAY(groups_JSON)->len; j++){
			tmp = YAJL_GET_STRING(YAJL_GET_ARRAY(groups_JSON)->values[j]);
			Group group = (Group) malloc(strlen(tmp)+1);
			memcpy(group, tmp, strlen(tmp)+1);
			AddToList(group, &service->groups);
		}
		
		AddToList(service,&entry->services);
		
	}
	
	AddToList(entry, &cache);
    
    return true;
}
