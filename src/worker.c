//
// Created by root on 12/18/23.
//

#include "worker.h"
#include "ports.h"
#include "log.h"
#include "packet.h"

static void dump_pkt(struct rte_mbuf *pkt) {
    printf("---------------------Port %d-------------------------------------\n", pkt->port);
    rte_pktmbuf_dump(stdout,  pkt, rte_pktmbuf_data_len(pkt));
}



static int media_process(struct lcore_info *lcore) {
    //TODO
    return 0;
}

static int forward_process(struct lcore_info *lcore) {
    int i;
    rte_atomic16_inc(&ready);
    LOG("worker start! type = %u", lcore->type);
    while(1){
        // receive pkt from physic interface
        for(i = 0; i < lcore->ports_count; i++) {
            struct rte_mbuf *rx_pkts[MAX_PKT_BURST];
            int rxBurst = rte_eth_rx_burst( lcore->ports[i], 0, (struct rte_mbuf**)&rx_pkts, MAX_PKT_BURST);
            if(rxBurst > 0) {
                unsigned int n;
                n = rte_ring_enqueue_bulk(lcore->send_queue_media, (void*)rx_pkts, rxBurst, NULL);
                if(n < rxBurst) {
                    rte_pktmbuf_free_bulk(&rx_pkts[n], rxBurst - n);
                }
            }
        }

        // dequeue pkts from media process
        {
            struct packet *rx_pkts[MAX_PKT_BURST];
            unsigned int n = rte_ring_dequeue_bulk(lcore->recv_queue_media, (void *) rx_pkts, MAX_PKT_BURST, NULL);
            if (n > 0) {
                //TODO send pkt to interface~
                int i;
                for(i = 0; i < n; i++) {
                    struct rte_mbuf *buf = rx_pkts[i]->mbuf;

                }
            }
        }
    }

    return 0;
}

static int ctrl_process(struct lcore_info *lcore) {
    //TODO
    return 0;
}

int worker(void *arg) {
    struct lcore_info *lcore = (struct lcore_info*)arg;
    if(lcore->type == LCORE_TYPE_MEDIA) {
        media_process(lcore);
    } else if(lcore->type == LCORE_TYPE_FORWARD) {
        forward_process(lcore);
    } else if(lcore->type == LCORE_TYPE_CTRL) {
        ctrl_process(lcore);
    } else {
        LOG("Unknown lcore type: %s", lcore->type);
    }
}


int init_lcore(struct lcore_info (*lcores)[], struct port_info (*ports)[]) {
    int core_id = 0;
    struct rte_ring *f_to_m = rte_ring_create("forward_to_media",2048, rte_lcore_to_socket_id(core_id), RING_F_SP_ENQ|RING_F_SC_DEQ);
    struct rte_ring *m_to_f = rte_ring_create("media_to_forward",2048, rte_lcore_to_socket_id(core_id), RING_F_SP_ENQ|RING_F_SC_DEQ);
    struct rte_ring *m_to_c = rte_ring_create("media_to_ctrl",2048, rte_lcore_to_socket_id(core_id), RING_F_SP_ENQ|RING_F_SC_DEQ);
    struct rte_ring *c_to_m = rte_ring_create("ctrl_to_media",2048, rte_lcore_to_socket_id(core_id), RING_F_SP_ENQ|RING_F_SC_DEQ);
    if(!f_to_m || !m_to_f || !m_to_c || !c_to_m) {
        rte_exit(EXIT_FAILURE, "Cannot init core ring\n");
    }
    for(core_id = 0; core_id < MAX_LCORE_NUM; core_id++) {
        struct lcore_info *info = &(*lcores)[core_id];
        if(info->type == LCORE_TYPE_CTRL) {
            uint16_t port_id;
            uint16_t k = 0;
            for(port_id = 0; port_id < MAX_PORT_NUM; port_id++) {
                struct port_info *port = &(*ports)[port_id];
                if(port->enabled == PORT_ENABLED) {
                    info->ports[k] = port->vport_id;
                    k++;
                }
            }
            info->ports_count = k;
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
                    k++;
                }
            }
            info->ports_count = k;
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