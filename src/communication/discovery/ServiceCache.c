#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "discovery.h"
#include "listType.h"
#include "handler.h"

CacheList cache;

LList GetLocal_ServiceInfo(void){
	LList local_services;
	LList descriptions;
	LElement * cache_item;
	LElement * item;
	
	CreateList(&local_services);
	
	FOR_EACH(cache_item, cache){
		if (((ServiceCache *)cache_item->data)->local){
			descriptions = ((ServiceCache *)cache_item->data)->descriptions;
			FOR_EACH(item, descriptions){
				AddToList(item->data,&local_services);
			}
		}
	}
	
	return local_services;
}

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

LList GetVicinity_GroupInfo(void){
	LList vicinity_groups;
	LElement * cache_item;
	LElement * item;
	LElement * tmp_item;
	bool in_list = false;
	
	CreateList(&vicinity_groups);
	
	FOR_EACH(cache_item, cache){
		if (!((ServiceCache *)cache_item->data)->local){
			FOR_EACH(item,((ServiceCache *)cache_item->data)->groups){
				FOR_EACH(tmp_item, vicinity_groups){
					if (item == tmp_item)
						in_list = true;
				}
				if (!in_list)
					AddToList(item->data, &vicinity_groups);
				in_list = false;
			}
			FOR_EACH(item,((ServiceCache *)cache_item->data)->vicinity_groups){
				FOR_EACH(tmp_item, vicinity_groups){
					if (item == tmp_item)
						in_list = true;
				}
				if (!in_list)
					AddToList(item->data, &vicinity_groups);
				in_list = false;
			}
		}
	}
	return vicinity_groups;
}


void print_cache(){
	LElement * cache_item;
	LElement * item;
	ServiceCache * cache_entry;
	
	FOR_EACH(cache_item,cache){
		cache_entry = (ServiceCache *)cache_item->data;
		printf("========== NEW CACHE ENTRY =========\n");
		printf("=== islocal: %s\n", (cache_entry->local)?"true":"false");
		printf("=== Source: %s\n", cache_entry->source_address);
		printf("=== lifetime: %i\n", cache_entry->lifetime);
		printf("=====DESCRIPTIONS=====\n");
		FOR_EACH(item,cache_entry->descriptions){
			printf("=== %s\n", item->data);
		}
		printf("=====END DESCRIPTIONS=====\n");
		printf("=====GROUPS=====\n");
		FOR_EACH(item,cache_entry->groups){
			printf("=== %s\n", item->data);
		}
		printf("=====END GROUPS=====\n");
		printf("=====VICINITY GROUPS=====\n");
		FOR_EACH(item,cache_entry->vicinity_groups){
			printf("=== %s\n", item->data);
		}
		printf("=====END VIC. GROUPS=====\n");
		printf("========== END CACHE ENTRY =========\n\n");
	}
}
