//
// Created by joe on 12/18/23.
//

#ifndef LWFW_WORKER_H
#define LWFW_WORKER_H

#include <pthread.h>

#include "dpdk.h"
#include "ports.h"
#include "stats.h"
#include "rtm.h"


#define MAX_WORKER_NUM (256)
#define WORKER_RING_SIZE (4096)

#define DISPATCH_WORKER_ID (1)

#define WORKER_TYPE_DISPATCH (1)
#define WORKER_TYPE_PROCESS (2)

#define WORKER_RING_NAME "worker_%u_ring_%s"

struct worker_info {
    struct port_info ports[MAX_PORT_NUM];	
	struct rte_ring *inbound_ring[MAX_WORKER_NUM];
	struct rte_ring *outbound_ring[MAX_WORKER_NUM];
	int (*worker_func)(void *args);
	uint8_t type;
	uint8_t lcore_id;
	uint8_t res[6];
};


extern rte_atomic16_t ready;
extern struct worker_info g_worker_infos[MAX_WORKER_NUM];
extern uint8_t g_worker_cnt;
int init_worker(uint8_t lcore_id);
int worker(void *args);
int dispatch_worker(void *args);


inline static int start_control_thread() {
    pthread_t stats_thread_info, rtm_thread_info;
	int res;
    res = pthread_create(&stats_thread_info, NULL, stats_thread, NULL);
    if (res != 0) {
        return 1;
    }
    res = pthread_create(&rtm_thread_info, NULL, rtm_thread, NULL);
    if (res != 0) {
        return 1;
    }
}

#endif //LWFW_WORKER_H
