//
// Created by root on 12/18/23.
//

#include <string.h>
#include "ports.h"

static uint16_t nb_rxd = RX_DESC_DEFAULT;
static uint16_t nb_txd = TX_DESC_DEFAULT;

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


int port_config(unsigned char port_id) {
    unsigned lcore_id;
    struct rte_eth_dev_info dev = {0};

    lcore_id = rte_lcore_id();
    rte_eth_dev_info_get(port_id, &dev);
    printf("core %u, port_id: %d, driver: %s\n", lcore_id, port_id, dev.driver_name);

    struct rte_eth_conf local_port_conf = port_conf;
    struct rte_ether_addr addr = {0};
    int ret = rte_eth_dev_configure(port_id, 1, 1, &local_port_conf);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n",
                 ret, port_id);

    ret = rte_eth_macaddr_get(port_id,&addr);
    if (ret < 0)
        rte_exit(EXIT_FAILURE,
                 "Cannot get MAC address: err=%d, port=%u\n",
                 ret, port_id);
    printf("port %d, mac is "RTE_ETHER_ADDR_PRT_FMT"\n", port_id, RTE_ETHER_ADDR_BYTES(&addr));


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
        rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup:err=%d, port=%u\n",
                 ret, port_id);

    void *tx_buffer = rte_zmalloc_socket("tx_buffer",
                                         RTE_ETH_TX_BUFFER_SIZE(MAX_PKT_BURST), 0,
                                         rte_eth_dev_socket_id(port_id));
    if (tx_buffer == NULL)
        rte_exit(EXIT_FAILURE, "Cannot allocate buffer for tx on port %u\n",
                 port_id);

    rte_eth_tx_buffer_init(tx_buffer, MAX_PKT_BURST);

    ret = rte_eth_dev_start(port_id);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "rte_eth_dev_start:err=%d, port=%u\n",
                 ret, port_id);
    return 0;
}
