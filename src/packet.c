#include "packet.h"
#include "log.h"


int eth_process(struct packet *p) {
	struct ethhdr *ethhdr = (struct ethhdr *)p->data;
	uint16_t type = ntohs(ethhdr->h_proto);
	if(type == ETH_P_ARP) {
		p->next_hop = 0;		
		return SUCCESS_CONTINUE;
	}
	if(type == ETH_P_IP) {
		p->ethhdr_size = sizeof(struct ethhdr);
		struct iphdr *iphdr = (struct iphdr*)(ethhdr + 1); 
		if(iphdr->protocol == IPPROTO_ICMP) {
			p->next_hop = 0;
			return SUCCESS_CONTINUE;
		}
	}
	return SUCCESS_CONTINUE;
}
