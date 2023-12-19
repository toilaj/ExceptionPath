//
// Created by Joe on 12/17/23.
//

#include <stdio.h>
#include <string.h>

#include "dpdk.h"
#include "stats.h"
#include "worker.h"
#include "ports.h"
#include "log.h"



rte_atomic16_t ready = {0};


int main()
{
    int ret;
    uint8_t lcore_id;
    int worker_cnt = 0;
    //char *eal_param[] = {"lwFW","-l","0-1","--log-level","pmd.*:debug"};
    char *eal_param[] = {"lwFW","-l","0-2"};
    ret = rte_eal_init(3, (char **)eal_param);
    if (ret < 0)
        rte_panic("Cannot init EAL\n");

    init_ports();
    RTE_LCORE_FOREACH_WORKER(lcore_id) {
        LOG("create worker for core %u", lcore_id);
        if(lcore_id > 255 || port_infos[lcore_id - 1].enabled != 1) {
            continue;
        }
        rte_eal_remote_launch(worker, &port_infos[lcore_id - 1], lcore_id);
        worker_cnt++;
    }
    while(1) {
        LOG("waiting for all worker ready....");
        if(rte_atomic16_read(&ready) == worker_cnt) {
            break;
        }
        rte_delay_ms(1000);
    }
    stats_thread();

//    while(1){
//        rte_delay_ms(10000);
//    }
    rte_eal_mp_wait_lcore();
    rte_eal_cleanup();

    return 0;
}


