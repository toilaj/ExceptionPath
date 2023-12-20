//
// Created by root on 12/20/23.
//

#ifndef LWFW_PACKET_H
#define LWFW_PACKET_H
#include <dpdk.h>

//cannot over 128 bytes(HEADROOM_SIZE)
struct packet {
    uint8_t *data;
    struct rte_mbuf *mbuf;
};

inline static struct packet *mbuf_to_packet(struct rte_mbuf *buf) {
    struct packet *p = (struct packet *)(buf + 1);
    p->data = rte_pktmbuf_mtod(buf, uint8_t*);
    p->mbuf = buf;
    return p;
}


#endif //LWFW_PACKET_H
