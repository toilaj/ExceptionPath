#ifndef _PACKET_H_
#define _PACKET_H_

#include <linux/if_ether.h>
#include "dpdk.h"


#define SUCCESS_CONTINUE (1) /* function returns success and packet is not freed */
#define FAIL_CONTINUE (2) /* function returns fail and packet is not freed */
#define SUCCESS_END (3) /* fucntion returns success and packet is freed */
#define FAIL_END (4) /* fucntion returns fail and packet is freed */

//use default RTE_PKTMBUF_HEADROOM 128B
//only consider single buffer scenario temporily.
struct packet {
	struct rte_mbuf *mbuf;
	uint8_t *data;	
	uint8_t *iphdr;
	uint32_t pkt_len;
	uint32_t next_hop;
	uint16_t in_port;
	uint16_t out_port;
	uint16_t ethhdr_size;
	uint16_t iphdr_size;
};



int eth_process(struct packet *p);

inline static void dump_pkt(struct packet *p) {
	struct rte_mbuf *pkt = p->mbuf;	
    printf("---------------------Port %d-------------------------------------\n", pkt->port);
    rte_pktmbuf_dump(stdout,  pkt, rte_pktmbuf_data_len(pkt));
}

inline static void dump_mbuf(struct rte_mbuf *pkt) {
    printf("---------------------Port %d-------------------------------------\n", pkt->port);
    rte_pktmbuf_dump(stdout,  pkt, rte_pktmbuf_data_len(pkt));
}



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
	p->out_port = 65535;
	p->next_hop = 0xffffffff;
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

inline static void free_packet(struct packet *p) {
	struct rte_mbuf *m;
	if(p == NULL) {
		return;
	}
	m = p->mbuf;
	rte_pktmbuf_free(m);
}

#endif
