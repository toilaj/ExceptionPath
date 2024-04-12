//
// Created by root on 12/18/23.
//

#ifndef LWFW_STATS_H
#define LWFW_STATS_H
#include  "dpdk.h"
int ethdev_stats_show(uint16_t port_id);
void *stats_thread(void *args) ;
#define NS_PER_SEC 1E9
#endif //LWFW_STATS_H

