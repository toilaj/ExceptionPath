//
// Created by root on 12/18/23.
//

#include <string.h>
#include "ports.h"
#include "log.h"

struct port_info port_infos[MAX_PORT_NUM] = {0};

static uint16_t nb_rxd = RX_DESC_DEFAULT;
static uint16_t nb_txd = TX_DESC_DEFAULT;


struct rte_mempool *pktmbuf_pool = NULL;

static struct rte_eth_conf port_conf = {
        .rxmode = {
                .mq_mode = RTE_ETH_MQ_RX_NONE,
        },
        .txmode = {
                .mq_mode = RTE_ETH_MQ_TX_NONE,
        },
        .lpbk_mode = 0,
};
static struct rte_eth_rxconf rx_conf = {
        .rx_thresh = {
                .pthresh = RX_PTHRESH,
                .hthresh = RX_HTHRESH,
                .wthresh = RX_WTHRESH,
        },
        .rx_free_thresh = 32,
};

static struct rte_eth_txconf tx_conf = {
        .tx_thresh = {
                .pthresh = TX_PTHRESH,
                .hthresh = TX_HTHRESH,
                .wthresh = TX_WTHRESH,
        },
        .tx_free_thresh = 32, /* Use PMD default values */
        .tx_rs_thresh = 32, /* Use PMD default values */
};


int config_port(unsigned char port_id) {
    unsigned lcore_id;
    struct rte_eth_dev_info dev = {0};

    rte_eth_dev_info_get(port_id, &dev);
    LOG("config port_id: %d, driver: %s", port_id, dev.driver_name);

    struct rte_eth_conf local_port_conf = port_conf;
    struct rte_ether_addr addr = {0};
    int ret = rte_eth_dev_configure(port_id, 1, 1, &local_port_conf);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u",
                 ret, port_id);

    ret = rte_eth_macaddr_get(port_id,&addr);
    if (ret < 0)
        rte_exit(EXIT_FAILURE,
                 "Cannot get MAC address: err=%d, port=%u\n",
                 ret, port_id);
    LOG("port %d, mac is "RTE_ETHER_ADDR_PRT_FMT, port_id, RTE_ETHER_ADDR_BYTES(&addr));


    ret = rte_eth_rx_queue_setup(port_id, 0, nb_rxd,
                                 rte_eth_dev_socket_id(port_id),
                                 &rx_conf,
                                 pktmbuf_pool);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup:err=%d, port=%u\n",
                 ret, port_id);


    ret = rte_eth_tx_queue_setup(port_id, 0, nb_txd,
                                 rte_eth_dev_socket_id(port_id),
                                 &tx_conf);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup:err=%d, port=%u",
                 ret, port_id);

    void *tx_buffer = rte_zmalloc_socket("tx_buffer",
                                         RTE_ETH_TX_BUFFER_SIZE(MAX_PKT_BURST), 0,
                                         rte_eth_dev_socket_id(port_id));
    if (tx_buffer == NULL)
        rte_exit(EXIT_FAILURE, "Cannot allocate buffer for tx on port %u",
                 port_id);

    rte_eth_tx_buffer_init(tx_buffer, MAX_PKT_BURST);

    ret = rte_eth_dev_start(port_id);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "rte_eth_dev_start:err=%d, port=%u",
                 ret, port_id);
    return 0;
}

int init_ports() {
    uint16_t nb_ports = rte_eth_dev_count_avail();
    LOG("find physic %u ports", nb_ports);
    int i,k = 0;
    if (nb_ports == 0)
        rte_exit(EXIT_FAILURE, "No Ethernet ports - bye\n");
    pktmbuf_pool = rte_pktmbuf_pool_create("pktmbuf_pool",
                                           8192,
                                           512, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
                                           rte_socket_id());
    if(pktmbuf_pool == NULL) {
        rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");
    }
    for(i=0; i < nb_ports; i++){
        int ret = -1;
        char port_name[64] = {0};
        char port_args[512] = {0};
        struct rte_ether_addr addr = {0};
        struct port_info port = {0};
        uint16_t port_id = i;
        ret = rte_eth_macaddr_get(port_id, &addr);
        if(ret != 0) {
            LOG("cannot get port %u mac addr", port_id);
            continue;
        }
        snprintf(port_name, sizeof(port_name), "%s%u",VIRTIO_USER_NAME,port_id);
        snprintf(port_args, sizeof(port_args),
                 "path=/dev/vhost-net,queues=1,queue_size=%u,iface=%s,mac=" RTE_ETHER_ADDR_PRT_FMT,
                 RX_RING_SIZE, port_name, RTE_ETHER_ADDR_BYTES(&addr));
        if (rte_eal_hotplug_add("vdev", port_name, port_args) < 0) {
            LOG("Cannot create paired port for port %u", port_id);
            continue;
        }
        port.port_id = port_id;
        ret = rte_eth_dev_get_port_by_name(port_name, &(port.peer_id));
        if(ret != 0) {
            LOG("cannot get port %s by name", port_name);
            continue;
        }
        rte_memcpy(&port.mac_addr, &addr, sizeof(struct rte_ether_addr));
        LOG("create peer port %u for physic port %u, mac: "RTE_ETHER_ADDR_PRT_FMT, port.peer_id, port.port_id, RTE_ETHER_ADDR_BYTES(&port.mac_addr));
        config_port(port.port_id);
        config_port(port.peer_id);
        port.enabled = 1;
        port_infos[k] = port;
        k++;
    }
    if(k == 0) {
        rte_exit(EXIT_FAILURE, "No available port info\n");
    }
    LOG("ports initial complete! ports count = %u", rte_eth_dev_count_avail());
    return 0;
}