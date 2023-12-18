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

#define VIRTIO_USER_NAME "virtio_user"

struct port_map {
    uint16_t port_id;
    uint16_t peer_port_id;
};

extern struct rte_mempool *pktmbuf_pool;
int port_config(unsigned char port_id);
#endif //LWFW_PORTS_H
