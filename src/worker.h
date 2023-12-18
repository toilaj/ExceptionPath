//
// Created by root on 12/18/23.
//

#ifndef LWFW_WORKER_H
#define LWFW_WORKER_H
#include "dpdk.h"
extern rte_atomic16_t ready;
int worker(void *arg);

#endif //LWFW_WORKER_H
