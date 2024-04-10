//
// Created by root on 12/18/23.
//

#include "stats.h"


int ethdev_stats_show(uint16_t port_id)
{
    uint64_t diff_pkts_rx, diff_pkts_tx, diff_bytes_rx, diff_bytes_tx;
    static uint64_t prev_pkts_rx[RTE_MAX_ETHPORTS];
    static uint64_t prev_pkts_tx[RTE_MAX_ETHPORTS];
    static uint64_t prev_bytes_rx[RTE_MAX_ETHPORTS];
    static uint64_t prev_bytes_tx[RTE_MAX_ETHPORTS];
    static uint64_t prev_cycles[RTE_MAX_ETHPORTS];
    uint64_t mpps_rx, mpps_tx, mbps_rx, mbps_tx;
    uint64_t diff_ns, diff_cycles, curr_cycles;
    struct rte_eth_stats stats;
    static const char *nic_stats_border = "########################";
    int rc;

    rc = rte_eth_stats_get(port_id, &stats);
    if (rc != 0) {
        fprintf(stderr,
                "%s: Error: failed to get stats (port %u): %d",
                __func__, port_id, rc);
        return rc;
    }

    fprintf(stdout,
            "\n  %s NIC statistics for port %-2d %s\n"
            "  RX-packets: %-10"PRIu64" RX-missed: %-10"PRIu64" RX-bytes:  ""%-"PRIu64"\n"
            "  RX-errors: %-"PRIu64"\n"
            "  RX-nombuf:  %-10"PRIu64"\n"
            "  TX-packets: %-10"PRIu64" TX-errors: %-10"PRIu64" TX-bytes:  ""%-"PRIu64"\n",
            nic_stats_border, port_id, nic_stats_border, stats.ipackets, stats.imissed,
            stats.ibytes, stats.ierrors, stats.rx_nombuf, stats.opackets, stats.oerrors,
            stats.obytes);



    diff_ns = 0;
    diff_cycles = 0;

    curr_cycles = rte_rdtsc();
    if (prev_cycles[port_id] != 0)
        diff_cycles = curr_cycles - prev_cycles[port_id];

    prev_cycles[port_id] = curr_cycles;
    diff_ns = diff_cycles > 0 ?
              diff_cycles * (1 / (double)rte_get_tsc_hz()) * NS_PER_SEC : 0;

    diff_pkts_rx = (stats.ipackets > prev_pkts_rx[port_id]) ?
                   (stats.ipackets - prev_pkts_rx[port_id]) : 0;
    diff_pkts_tx = (stats.opackets > prev_pkts_tx[port_id]) ?
                   (stats.opackets - prev_pkts_tx[port_id]) : 0;
    prev_pkts_rx[port_id] = stats.ipackets;
    prev_pkts_tx[port_id] = stats.opackets;
    mpps_rx = diff_ns > 0 ?
              (double)diff_pkts_rx / diff_ns * NS_PER_SEC : 0;
    mpps_tx = diff_ns > 0 ?
              (double)diff_pkts_tx / diff_ns * NS_PER_SEC : 0;

    diff_bytes_rx = (stats.ibytes > prev_bytes_rx[port_id]) ?
                    (stats.ibytes - prev_bytes_rx[port_id]) : 0;
    diff_bytes_tx = (stats.obytes > prev_bytes_tx[port_id]) ?
                    (stats.obytes - prev_bytes_tx[port_id]) : 0;
    prev_bytes_rx[port_id] = stats.ibytes;
    prev_bytes_tx[port_id] = stats.obytes;
    mbps_rx = diff_ns > 0 ?
              (double)diff_bytes_rx / diff_ns * NS_PER_SEC : 0;
    mbps_tx = diff_ns > 0 ?
              (double)diff_bytes_tx / diff_ns * NS_PER_SEC : 0;

    fprintf(stdout,"\n  Throughput (since last show)\n"
                   "  Rx-pps: %12"PRIu64"          Rx-bps: %12"PRIu64"\n  Tx-pps: %12"
                   PRIu64"          Tx-bps: %12"PRIu64"\n"
                    "  %s############################%s\n",
            mpps_rx, mbps_rx * 8, mpps_tx, mbps_tx * 8, nic_stats_border, nic_stats_border);
    return 0;
}

void stats_thread() {
    int i;
    while(1) {
        for(i = 0; i < rte_eth_dev_count_avail(); i++){
            //ethdev_stats_show(i);
        }
        rte_delay_ms(1000);
    }
}
