#ifndef _PACKET_H_
#define _PACKET_H_

#include "dpdk.h"

//use default RTE_PKTMBUF_HEADROOM 128B
//only consider single buffer scenario temporily.
struct packet {
	struct rte_mbuf *mbuf;
	uint8_t *data;	
	uint8_t *ip_offset;
	uint32_t pkt_len;
	uint32_t dst_ip;
	uint32_t next_hop;
	uint16_t in_port;
	uint16_t out_port;
};

inline static struct packet *mbuf_to_packet(struct rte_mbuf *m) {
	struct packet *p;
	if(m == NULL) {
		return NULL;
	}
	p = (void*)(m + 1);
	p->mbuf = m;
	p->data = rte_pktmbuf_mtod(m, uint8_t *);
	p->pkt_len = m->data_len; // only for packet of single buffer.
        p->in_port = m->port;
	return p;
}

inline static struct rte_mbuf *packet_to_mbuf(struct packet *p) {
	struct rte_mbuf *m;
	if(p == NULL) {
		return NULL;
	}
	m = p->mbuf;
	return m;
}

#endif
