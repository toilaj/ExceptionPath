//
// Created by Joe on 12/17/23.
//

#include <stdio.h>
#include <string.h>

#include "dpdk.h"
#include "stats.h"
#include "worker.h"
#include "ports.h"

#define RX_RING_SIZE 1024
struct rte_mempool *pktmbuf_pool = NULL;

rte_atomic16_t ready = {0};

int main()
{
    int ret;
    int nb_ports;
    unsigned lcore_id;
    //char *eal_param[] = {"lwFW","-l","0-1","--log-level","pmd.*:debug"};
    char *eal_param[] = {"lwFW","-l","0-2"};
    ret = rte_eal_init(3, (char **)eal_param);
    if (ret < 0)
        rte_panic("Cannot init EAL\n");

    nb_ports = rte_eth_dev_count_avail();
    if (nb_ports == 0)
        rte_exit(EXIT_FAILURE, "No Ethernet ports - bye\n");

    char port_name[32] = {0};
    uint16_t port_id = 0;
    char port_args[256] = {0};

    struct port_map port_maps[3] = {{0,0}, {0,1}, {1,0}};

    struct rte_ether_addr addr = {0};
    rte_eth_macaddr_get(port_id, &addr);
    snprintf(port_name, sizeof(port_name), VIRTIO_USER_NAME"%u", port_id);
    snprintf(port_args, sizeof(port_args),
             "path=/dev/vhost-net,queues=1,queue_size=%u,iface=%s,mac=" RTE_ETHER_ADDR_PRT_FMT,
             RX_RING_SIZE, port_name, RTE_ETHER_ADDR_BYTES(&addr));
    if (rte_eal_hotplug_add("vdev", port_name, port_args) < 0)
        rte_exit(EXIT_FAILURE, "Cannot create paired port for port %u\n", port_id);

    printf("nb_ports = %d\n", rte_eth_dev_count_avail());
    pktmbuf_pool = rte_pktmbuf_pool_create("pktmbuf_pool",
                                           4096,
                                           256, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
                                           rte_socket_id());
    if(pktmbuf_pool == NULL) {
        rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");
    }

    RTE_LCORE_FOREACH_WORKER(lcore_id) {
        rte_eal_remote_launch(worker, &port_maps[lcore_id], lcore_id);
    }
    while(1) {
        if(rte_atomic16_read(&ready) == rte_lcore_count() - 1) {
            break;
        }
        rte_delay_ms(1000);
    }
    stats_thread();

    rte_eal_mp_wait_lcore();
    rte_eal_cleanup();

    return 0;
}


