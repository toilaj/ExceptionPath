//
// Created by root on 12/18/23.
//

#ifndef LWFW_PORTS_H
#define LWFW_PORTS_H
#include "dpdk.h"

#define RX_PTHRESH 8 /**< Default values of RX prefetch threshold reg. */
#define RX_HTHRESH 8 /**< Default values of RX host threshold reg. */
#define RX_WTHRESH 0 /**< Default values of RX write-back threshold reg. */

#define TX_PTHRESH 32 /**< Default values of TX prefetch threshold reg. */
#define TX_HTHRESH 0  /**< Default values of TX host threshold reg. */
#define TX_WTHRESH 0  /**< Default values of TX write-back threshold reg. */


#define RX_DESC_DEFAULT 1024
#define TX_DESC_DEFAULT 1024
#define MAX_PKT_BURST 32
#define RX_RING_SIZE 1024
#define VIRTIO_USER_NAME "virtio_user_"

#define MAX_PORT_NUM 256

struct port_info {
    uint16_t port_id;
    uint16_t peer_id;
    struct rte_ether_addr mac_addr;
    uint32_t ipv4_addr;
    uint32_t ipv4_mask;
    uint8_t type;
    uint8_t enabled;
    uint8_t res;
};
extern struct port_info g_port_infos[MAX_PORT_NUM];
extern struct rte_mempool *pktmbuf_pool;
int config_port(unsigned char port_id);
int init_ports();
#endif //LWFW_PORTS_H
