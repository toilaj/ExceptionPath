//
// Created by root on 12/18/23.
//

#ifndef LWFW_PORTS_H
#define LWFW_PORTS_H

#include <string.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <net/if.h>
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
#define RX_RING_SIZE 1024
#define VIRTIO_USER_NAME "virtio_user_"

#define MAX_PORT_NUM 256

struct port_info {
    uint16_t port_id;
    uint16_t peer_id;
    struct rte_ether_addr mac_addr;
    uint32_t ipv4_addr;
    uint32_t ipv4_mask;
    uint8_t type;
    uint8_t enabled;
    uint8_t res;
};
extern struct port_info g_port_infos[MAX_PORT_NUM];
extern struct rte_mempool *pktmbuf_pool;

extern uint16_t g_port_to_kport[MAX_PORT_NUM];
extern uint16_t g_kport_to_port[MAX_PORT_NUM]; 
int config_port(unsigned char port_id);
int init_ports();

inline static int iface_configure(char* ifname, unsigned char *ip, unsigned char *mask)
{
    struct sockaddr_in *sin;
    struct ifreq ifr;
    int res = 0;
    int fd;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0) {
        return -1;
    }
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);
    res |= ioctl(fd, SIOCGIFFLAGS, &ifr);
    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    res |= ioctl(fd, SIOCSIFFLAGS, &ifr);

    sin = (struct sockaddr_in *)&ifr.ifr_addr;
    sin->sin_family = AF_INET;
    inet_aton(ip, &(sin->sin_addr));
    res |= ioctl(fd, SIOCSIFADDR, &ifr);
    
    inet_aton(mask, &(sin->sin_addr));
    res |= ioctl(fd, SIOCSIFNETMASK, &ifr);

    ifr.ifr_ifru.ifru_mtu = 1500;
    res |= ioctl(fd, SIOCSIFMTU, &ifr);

    close(fd);
	res = if_nametoindex(ifname);
    return res;
}


#endif //LWFW_PORTS_H
