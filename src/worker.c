//
// Created by root on 12/18/23.
//

#include "worker.h"
#include "ports.h"

static void dump_pkt(struct rte_mbuf *pkt) {
    printf("---------------------Port %d-------------------------------------\n", pkt->port);
    rte_pktmbuf_dump(stdout,  pkt, rte_pktmbuf_data_len(pkt));
}

int worker(void *arg)
{
    struct port_map port_map = *(struct port_map*)arg;
    uint16_t port_id = port_map.port_id;
    uint16_t peer_id = port_map.peer_port_id;
    port_config(port_id);
    rte_atomic16_inc(&ready);
    while(1) {
        struct rte_mbuf *rx_pkts[MAX_PKT_BURST];
        int rxBurst = rte_eth_rx_burst(port_id, 0, (struct rte_mbuf**)&rx_pkts, MAX_PKT_BURST);
        if(rxBurst > 0) {
            uint16_t txBurst = rte_eth_tx_burst(peer_id,0,(struct rte_mbuf**)&rx_pkts, rxBurst);
            if(txBurst != rxBurst) {
                int i;
                for(i = rxBurst - txBurst; i < rxBurst; i++) {
                    rte_pktmbuf_free(rx_pkts[i]);
                }
            }
        }
    }
    return 0;
}

