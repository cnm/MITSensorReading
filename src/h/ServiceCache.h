#ifndef __SERVICECACHE_H__
#define __SERVICECACHE_H__

#include "listType.h"

void GetLocal_ServiceInfo(ServiceList *);

//LList GetLocal_ServiceGroupInfo(void);

void GetVicinity_GroupInfo(GroupList *);

void print_cache();

#endif
