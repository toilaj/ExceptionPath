//
// Created by root on 12/18/23.
//

#ifndef LWFW_WORKER_H
#define LWFW_WORKER_H
#include "dpdk.h"
#include "ports.h"

#define MAX_LCORE_NUM (256)

#define LCORE_TYPE_CTRL (0)
#define LCORE_TYPE_MEDIA (1)
#define LCORE_TYPE_FORWARD (2)

#define LCORE_STATUS_ENABLED (1)
#define LCORE_STATUS_INIT (0)

struct lcore_info {
    uint16_t ports[MAX_PORT_NUM];
    struct rte_ring *recv_queue_media;
    struct rte_ring *recv_queue_ctrl;
    struct rte_ring *recv_queue_forward;
    struct rte_ring *send_queue_media;
    struct rte_ring *send_queue_ctrl;
    struct rte_ring *send_queue_forward;

    uint8_t type;
    uint8_t status;
};

extern rte_atomic16_t ready;
int worker(void *arg);

int init_lcore(struct lcore_info (*lcores)[], struct port_info (*ports)[])
#endif //LWFW_WORKER_H
