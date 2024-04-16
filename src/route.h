#ifndef _ROUTE_H_
#define _ROUTE_H_

int init_fastpath_rt(uint32_t rules, uint32_t t8s);
int add_fastpath_rt_entry(uint32_t ip, uint8_t mask_len, uint32_t next_hop);
int del_fastpath_rt_entry(uint32_t ip, uint8_t mask_len);
int lookup_fastpath_rt_entry(uint32_t ip, uint32_t *nex_hop);

#endif
