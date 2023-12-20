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
    uint16_t tx_port_id = port.vport_id;
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

int init_lcore(struct lcore_info (*lcores)[], struct port_info (*ports)[]) {
    int core_id = 0;
    struct rte_ring *f_to_m = rte_ring_create("forward_to_media",2048, rte_lcore_to_socket_id(core_id), RING_F_SP_ENQ|RING_F_SC_DEQ);
    struct rte_ring *m_to_f = rte_ring_create("media_to_forward",2048, rte_lcore_to_socket_id(core_id), RING_F_SP_ENQ|RING_F_SC_DEQ);
    struct rte_ring *m_to_c = rte_ring_create("media_to_ctrl",2048, rte_lcore_to_socket_id(core_id), RING_F_SP_ENQ|RING_F_SC_DEQ);
    struct rte_ring *c_to_m = rte_ring_create("ctrl_to_media",2048, rte_lcore_to_socket_id(core_id), RING_F_SP_ENQ|RING_F_SC_DEQ);
    for(core_id = 0; core_id < MAX_LCORE_NUM; core_id++) {
        struct lcore_info *info = &(*lcores)[core_id];
        if(info->type == LCORE_TYPE_CTRL) {
            uint16_t port_id;
            uint16_t k = 0;
            for(port_id = 0; port_id < MAX_PORT_NUM; port_id++) {
                struct port_info *port = &(*ports)[port_id];
                if(port->enabled == PORT_ENABLED) {
                    info->ports[k] = port->vport_id;
                }
            }
            info->recv_queue_media = m_to_c;
            info->send_queue_media = c_to_m;
            info->status = LCORE_STATUS_ENABLED;
        } else if(info->type == LCORE_TYPE_FORWARD) {
            uint16_t port_id;
            uint16_t k = 0;
            for(port_id = 0; port_id < MAX_PORT_NUM; port_id++) {
                struct port_info *port = &(*ports)[port_id];
                if(port->enabled == PORT_ENABLED) {
                    info->ports[k] = port->port_id;
                }
            }
            info->recv_queue_media = m_to_f;
            info->send_queue_media = f_to_m;
            info->status = LCORE_STATUS_ENABLED;
        } else if(info->type == LCORE_TYPE_MEDIA) {
            info->recv_queue_ctrl = c_to_m;
            info->send_queue_ctrl = m_to_c;
            info->recv_queue_forward = f_to_m;
            info->send_queue_media = m_to_f;
            info->status = LCORE_STATUS_ENABLED;
        }

    }
    return 0;
}