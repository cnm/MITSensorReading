#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "discovery.h"
#include "listType.h"
#include "ServiceCache.h"
#include <fred/handler.h>
#include <syslog.h>

CacheList cache;

void GetLocal_ServiceInfo(ServiceList * local_services){
	LElement * cache_item;
	
	CreateList(local_services);
	
	FOR_EACH(cache_item, cache){
		if (((ServiceCache *)cache_item->data)->local){
			AddToList(&((ServiceCache *)cache_item->data)->services,local_services);
		}
	}
}
/*
LList GetLocal_ServiceGroupInfo(void){
	LList lista;
	LList groups;
	LElement * cache_item;
	LElement * item;
	
	CreateList(&lista);
	
	FOR_EACH(cache_item, cache){
		if (((ServiceCache *)cache_item->data)->local){
			groups = ((ServiceCache *)cache_item->data)->groups;
			FOR_EACH(item, groups){
				AddToList(item->data, &lista);
			}
		}
	}
	
	return lista;
}
*/
void GetVicinity_GroupInfo(GroupList * vicinity_groups){
	LElement * cache_item;
	LElement * service_item;
	LElement * group_item;
	LElement * tmp_item;
	bool in_list = false;
	
	CreateList(vicinity_groups);
	
	FOR_EACH(cache_item, cache){
		if (!((ServiceCache *)cache_item->data)->local){
			FOR_EACH(service_item,((ServiceCache *)cache_item->data)->services){
				FOR_EACH(group_item, ((Service *) service_item)->groups){
					FOR_EACH(tmp_item, (*vicinity_groups)){
						if (strcmp((Group) group_item->data,(Group) tmp_item) == 0)
							in_list = true;
					}
					if (!in_list)
						AddToList(group_item->data, vicinity_groups);
					in_list = false;
				}
			}
			FOR_EACH(group_item,((ServiceCache *)cache_item->data)->vicinity_groups){
				FOR_EACH(tmp_item, (*vicinity_groups)){
					if (strcmp((Group) group_item->data, (Group) tmp_item) == 0)
						in_list = true;
				}
				if (!in_list)
					AddToList(group_item->data, vicinity_groups);
				in_list = false;
			}
		}
	}
}


void print_cache(){
	LElement * cache_item;
	LElement * item;
	LElement * group_item;
	ServiceCache * cache_entry;
	
	FOR_EACH(cache_item,cache){
		cache_entry = (ServiceCache *)cache_item->data;
		printf("========== NEW CACHE ENTRY =========\n");
		printf("=== islocal: %s\n", (cache_entry->local)?"true":"false");
		printf("=== Source: %d\n", cache_entry->source_address);
		printf("=== lifetime: %i\n", cache_entry->lifetime);
		FOR_EACH(item,cache_entry->services){
			printf("=====NEW SERVICE=====\n");
			printf("=== %s\n", ((Service *)item->data)->description);
			printf("=====GROUPS======n");
			FOR_EACH(group_item, ((Service *)item->data)->groups){			
				printf("=== %s ", (Group) group_item->data);
			}
			printf("\n=====END GROUPS=====\n");
			printf("=====END SERVICE=====\n");
		}
		printf("=====VICINITY GROUPS=====\n");
		FOR_EACH(group_item, cache_entry->vicinity_groups){			
			printf("=== %s ", (Group) group_item->data);
		}
		printf("=====END VIC. GROUPS=====\n");
		printf("========== END CACHE ENTRY =========\n\n");
	}
}
