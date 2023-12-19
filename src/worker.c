//
// Created by root on 12/18/23.
//

#include "worker.h"
#include "ports.h"
#include "log.h"

static void dump_pkt(struct rte_mbuf *pkt) {
    printf("---------------------Port %d-------------------------------------\n", pkt->port);
    rte_pktmbuf_dump(stdout,  pkt, rte_pktmbuf_data_len(pkt));
}

int worker(void *arg)
{
    struct port_info port = *(struct port_info*)arg;
    uint16_t rx_port_id = port.port_id;
    uint16_t tx_port_id = port.peer_id;
    while(!port.enabled) {
        LOG("port %u is not enabled!", port.port_id);
        rte_delay_ms(1000);
    }
    rte_atomic16_inc(&ready);
    LOG("worker for port %u start!", port.port_id);
    while(1) {
        struct rte_mbuf *rx_pkts[MAX_PKT_BURST];
        int rxBurst = rte_eth_rx_burst(rx_port_id, 0, (struct rte_mbuf**)&rx_pkts, MAX_PKT_BURST);
        if(rxBurst > 0) {
            uint16_t txBurst = rte_eth_tx_burst(tx_port_id,0,(struct rte_mbuf**)&rx_pkts, rxBurst);
            if(txBurst != rxBurst) {
                int i;
                for(i = rxBurst - txBurst; i < rxBurst; i++) {
                    rte_pktmbuf_free(rx_pkts[i]);
                }
            }
        }
        // sweep rx and tx port;
        (rx_port_id ^ tx_port_id) && (tx_port_id ^= rx_port_id ^= tx_port_id, rx_port_id ^= tx_port_id);
    }
    return 0;
}

