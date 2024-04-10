//
// Created by joe on 12/18/23.
//

#include "worker.h"
#include "ports.h"
#include "log.h"
#include "packet.h"


struct worker_info g_worker_infos[MAX_WORKER_NUM] = {0};
uint8_t g_worker_cnt = 0;

static int main_packet_process(struct packet *p) {
	if(p == NULL) {
		return FAIL_END;
	}
	return eth_process(p);	
}


int init_worker(uint8_t lcore_id) {
	struct worker_info w = {0};
	char ring_name[256] = {0};
	if(lcore_id == DISPATCH_WORKER_ID) {
		w.type = WORKER_TYPE_DISPATCH;
		memcpy(&w.ports, &g_port_infos, sizeof(g_port_infos));
		w.worker_func = dispatch_worker;
	} else {
		w.type = WORKER_TYPE_PROCESS;
		snprintf(ring_name, 256, WORKER_RING_NAME, lcore_id, "inbound");
		w.inbound_ring[0] = rte_ring_create(ring_name, WORKER_RING_SIZE, SOCKET_ID_ANY, RING_F_SP_ENQ|RING_F_SC_DEQ);
		memset(&ring_name, 0, sizeof(ring_name));
		snprintf(ring_name, 256, WORKER_RING_NAME, lcore_id, "outbound");
		w.outbound_ring[0] = rte_ring_create(ring_name, WORKER_RING_SIZE, SOCKET_ID_ANY, RING_F_SP_ENQ|RING_F_SC_DEQ);
		w.worker_func = worker;
		if(w.inbound_ring[0] == NULL || w.outbound_ring[0] == NULL) {
			rte_exit(EXIT_FAILURE, "init worker ring failed\n");			
	    }
	
		g_worker_infos[DISPATCH_WORKER_ID].inbound_ring[lcore_id] =  w.inbound_ring[0];
		g_worker_infos[DISPATCH_WORKER_ID].outbound_ring[lcore_id] =  w.outbound_ring[0];
	}
	w.lcore_id = lcore_id;
	g_worker_infos[lcore_id] = w;
	LOG("init worker lcore_id = %u , type = %d completed!", w.lcore_id, w.type);
}


inline static uint8_t dispatch_hash(void *data, uint8_t size) {
		//temporary algorithm	
		int i;
		uint64_t sum;
		int block = size / 8;
		uint64_t *p = data;
		for(i = 0; i < block; i++) {
			sum += *(p + i);	
		}
		return sum % sizeof(uint8_t);
}

int dispatch_worker(void *args) {
	int i, j, rx_burst, tx_burst;
	uint16_t port_cnt = 0;
	struct worker_info *w = (struct worker_info *)args;
	struct rte_ring *inbound_ring[MAX_WORKER_NUM];
	struct rte_ring *outbound_ring[MAX_WORKER_NUM];
	uint16_t peer_ports[MAX_PORT_NUM] = {0};
	uint16_t ports[MAX_PORT_NUM] = {0};
	struct rte_mbuf *rx_bufs[MAX_PKT_BURST];
	struct rte_mbuf *tx_bufs[MAX_PKT_BURST];
	struct packet *rx_pkts[MAX_PKT_BURST];

	for(i = 0; i < MAX_PORT_NUM; i++) {
		if(!w->ports[i].enabled) {
			continue;		
		}
		LOG("port %u is enabled!", w->ports[i].port_id);
		ports[i] = w->ports[i].port_id;
		peer_ports[ports[i]] = w->ports[i].peer_id; 
		port_cnt++;
    }
    memcpy(inbound_ring, w->inbound_ring, sizeof(inbound_ring));
	memcpy(outbound_ring, w->outbound_ring, sizeof(outbound_ring));
	rte_atomic16_inc(&ready);
	while(1) {
		/* 1. receive pkt from normal ports */
		for(i = 0; i < port_cnt; i++) {
			rx_burst = rte_eth_rx_burst(ports[i], 0, (struct rte_mbuf**)&rx_bufs, MAX_PKT_BURST);	
			if(rx_burst > 0) {
				LOG("received pkt %d, from port: %u", rx_burst, ports[i]);
				for(j = 0; j < rx_burst; j++) {
					LOG("process pkt id %d, %p", j, rx_bufs[j]);
					struct packet *p = mbuf_to_packet(rx_bufs[j]);
					//uint8_t worker_index = dispatch_hash(p->data + 16 + 12, 8) % g_worker_cnt + 1;
					uint8_t worker_index = 2;	
					int ret = rte_ring_enqueue(inbound_ring[worker_index], p);
					if(ret != 0) {
						LOG("enqueue inbound ring failed, index: %d", worker_index);
						free_packet(p);	
					}
				}
			}
		}

		/* 2. send pkt */
		//process worker id is started by 2
		for(i = 2; i < g_worker_cnt + 1; i++) {
			rx_burst = rte_ring_dequeue_burst(outbound_ring[i], (void **)&rx_pkts, MAX_PKT_BURST, NULL);
			if(rx_burst == 0) {
				continue;	
			}
			for(j = 0; j < rx_burst; j++) {
				struct packet *p;
				p = rx_pkts[j];
				uint16_t out_port = p->out_port;
				if(p->next_hop == 0 || out_port == 65535) {
					/* pkt needs to be sent to kernel */
					out_port = peer_ports[p->in_port];
				}
				struct rte_mbuf *m = packet_to_mbuf(p);
				tx_bufs[0] = m;
				tx_burst = rte_eth_tx_burst(out_port, 0, (struct rte_mbuf**)&tx_bufs, 1); 
				if(tx_burst != 1) {
					rte_pktmbuf_free(tx_bufs[0]);
				}
			}
		}	

		/* 3. forward pkt from kernel */
		for(i = 0; i < port_cnt; i++) {
			uint16_t port_id = ports[i];
			uint16_t peer_port = peer_ports[port_id];	
			rx_burst = rte_eth_rx_burst(peer_port, 0, (struct rte_mbuf**)&rx_bufs, MAX_PKT_BURST);
			if(rx_burst > 0) {
				LOG("send pkt from kernel to normal port(%u, %u)", peer_port, port_id);
				dump_mbuf(rx_bufs[0]);
				tx_burst = rte_eth_tx_burst(port_id, 0, (struct rte_mbuf**)&rx_bufs, rx_burst);
				if(tx_burst != rx_burst){
					LOG("forward pkts from kernel fail, %d", rx_burst - tx_burst);
					for(j = rx_burst - tx_burst; j < rx_burst; j++) {
						rte_pktmbuf_free(tx_bufs[j]);
					}
				}	
			}
		}	
	}
	return 0;
}

int worker(void *args) {
    struct worker_info *w = (struct worker_info*)args;
    rte_atomic16_inc(&ready);
	struct rte_ring *inbound_ring = w->inbound_ring[0];
	struct rte_ring *outbound_ring = w->outbound_ring[0];
    while(1) {
        struct packet *rx_pkts[MAX_PKT_BURST];
		struct packet *tx_pkts[MAX_PKT_BURST];
    	int i;
		int tx_ready = 0;
        int rx_burst = rte_ring_dequeue_burst(inbound_ring, (void**)&rx_pkts, MAX_PKT_BURST, NULL);
        if(rx_burst > 0) {
			LOG("process worker receive %d pkts", rx_burst);
            for(i = 0; i < rx_burst; i++) {
				struct packet *p = rx_pkts[i];
				int ret = main_packet_process(p);
				switch(ret) {
					case SUCCESS_CONTINUE:
						tx_pkts[tx_ready++] = p;
						break;
					case FAIL_CONTINUE:
						free_packet(p);
						break;
					case SUCCESS_END:
					case FAIL_END:
					default:
						continue;
				}
	    	}	    
            uint16_t tx_burst = rte_ring_enqueue_burst(outbound_ring, (void**)&tx_pkts, tx_ready, NULL);
            if(tx_burst != tx_ready) {
                int i;
                for(i = tx_ready - tx_burst; i < tx_ready; i++) {
                    free_packet(tx_pkts[i]);
                }
            }
        }
    }
    return 0;
}

