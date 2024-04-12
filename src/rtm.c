#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/route.h>
#include <errno.h>

#include "log.h"
 
#define EPOLL_LISTEN_MAX_CNT    256
#define EPOLL_LISTEN_TIMEOUT    500

#define NEIGH_RTA(r) ((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ndmsg))))

 
int g_nlfd = -1;
int g_epollfd = -1;
 
void parse_rtattr(struct rtattr **tb, int max, struct rtattr *attr, int len)
{
    for (; RTA_OK(attr, len); attr = RTA_NEXT(attr, len)) {
        if (attr->rta_type <= max) {
            tb[attr->rta_type] = attr;
        }
    }
}
 
void nl_netroute_handle(struct nlmsghdr *nlh)
{
    int len;
    struct rtattr *tb[RTA_MAX + 1];
    struct rtmsg *rt;
    char tmp[256];
 
    bzero(tb, sizeof(tb));
    rt = NLMSG_DATA(nlh);
    len = nlh->nlmsg_len - NLMSG_SPACE(sizeof(*rt));
    parse_rtattr(tb, RTA_MAX, RTM_RTA(rt), len);
    LOG("%s: ", (nlh->nlmsg_type==RTM_NEWROUTE)?"NEWROUT":"DELROUT");
    if (tb[RTA_DST] != NULL) {
        inet_ntop(rt->rtm_family, RTA_DATA(tb[RTA_DST]), tmp, sizeof(tmp));
        LOG("DST: %s ", tmp);
    }
    if (tb[RTA_SRC] != NULL) {
        inet_ntop(rt->rtm_family, RTA_DATA(tb[RTA_SRC]), tmp, sizeof(tmp));
        LOG("SRC: %s ", tmp);
    }
    if (tb[RTA_GATEWAY] != NULL) {
        inet_ntop(rt->rtm_family, RTA_DATA(tb[RTA_GATEWAY]), tmp, sizeof(tmp));
        LOG("GATEWAY: %s ", tmp);
    }
    LOG("\n");
}
 
void nl_netifinfo_handle(struct nlmsghdr *nlh)
{
    int len;
    struct rtattr *tb[IFLA_MAX + 1];
    struct ifinfomsg *ifinfo;
 
    bzero(tb, sizeof(tb));
    ifinfo = NLMSG_DATA(nlh);
    len = nlh->nlmsg_len - NLMSG_SPACE(sizeof(*ifinfo));
    parse_rtattr(tb, IFLA_MAX, IFLA_RTA(ifinfo), len);
 
    LOG("%s: %s ", (nlh->nlmsg_type==RTM_NEWLINK) ? "NEWLINK" : "DELLINK", (ifinfo->ifi_flags & IFF_UP) ? "up" : "down");
    if(tb[IFLA_IFNAME]) {
        LOG("%s", (char*)RTA_DATA(tb[IFLA_IFNAME]));
    }
    LOG("\n");
}
 
void nl_netifaddr_handle(struct nlmsghdr *nlh)
{
    int len;
    struct rtattr *tb[IFA_MAX + 1];
    struct ifaddrmsg *ifaddr;
    char tmp[256];
 
    bzero(tb, sizeof(tb));
    ifaddr = NLMSG_DATA(nlh);
    len =nlh->nlmsg_len - NLMSG_SPACE(sizeof(*ifaddr));
    parse_rtattr(tb, IFA_MAX, IFA_RTA (ifaddr), len);
 
    LOG("%s ", (nlh->nlmsg_type == RTM_NEWADDR)? "NEWADDR":"DELADDR");
    if (tb[IFA_LABEL] != NULL) {
        LOG("%s ", (char*)RTA_DATA(tb[IFA_LABEL]));
    }
    if (tb[IFA_ADDRESS] != NULL) {
        inet_ntop(ifaddr->ifa_family, RTA_DATA(tb[IFA_ADDRESS]), tmp, sizeof(tmp));
        LOG("%s ", tmp);
    }
    LOG("\n");
}
 
void nl_netneigh_handle(struct nlmsghdr *nlh)
{
    int len;
    struct rtattr *tb[IFA_MAX + 1];
    struct ndmsg *nd;
    char tmp[256];
 
    bzero(tb, sizeof(tb));
    nd = NLMSG_DATA(nlh);
    len =nlh->nlmsg_len - NLMSG_SPACE(sizeof(*nd));
    parse_rtattr(tb, IFA_MAX, NEIGH_RTA(nd), len);
 
	switch(nlh->nlmsg_type) {
		case RTM_NEWNEIGH:
		{
			if (tb[NDA_LLADDR] != NULL) {
    			LOG("%s ", "NEWNEIGH");
				if (RTA_PAYLOAD(tb[NDA_LLADDR]) >= 6) {
					char mac[18];
					unsigned char *macaddr = RTA_DATA(tb[NDA_LLADDR]);
					snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",
						macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
					LOG("mac: %s ->", mac);
				}
				if (tb[NDA_DST] != NULL) {
					if (RTA_PAYLOAD(tb[NDA_DST]) >= sizeof(struct in_addr)) {
						struct in_addr *ipaddr = (struct in_addr *)RTA_DATA(tb[NDA_DST]);
						inet_ntop(AF_INET, ipaddr, tmp, sizeof(tmp));
						LOG("ip: %s\n", tmp);	
					}
				}
			}
		}		
		break;
		case RTM_DELNEIGH:
		{
			LOG("%s ", "DELNEIGH");
			if (tb[NDA_DST] != NULL) {
				if (RTA_PAYLOAD(tb[NDA_DST]) >= sizeof(struct in_addr)) {
					struct in_addr *ipaddr = (struct in_addr *)RTA_DATA(tb[NDA_DST]);
					inet_ntop(AF_INET, ipaddr, tmp, sizeof(tmp));
					LOG("remove neighbor for ip: %s\n", tmp);	
				}
			}
		}		
		break;
		default:
		break;
	}
}

void nl_netlink_handle(int fd)
{
    int r_size;
    socklen_t len = 0;
    char buff[2048] = {0};
    struct sockaddr_nl addr;
    struct nlmsghdr *nlh;
 
    while(1)
    {
        len = sizeof(addr);
        r_size = recvfrom(fd, (void *)buff, sizeof(buff), 0, (struct sockaddr *)&addr, &len);
        nlh = (struct nlmsghdr *)buff;
        for(; NLMSG_OK(nlh, r_size); nlh = NLMSG_NEXT(nlh, r_size))
        {
            switch(nlh->nlmsg_type) {
            case NLMSG_DONE:
            case NLMSG_ERROR:
                break;
            case RTM_NEWLINK:    
            case RTM_DELLINK:
                nl_netifinfo_handle(nlh);
                break;
            case RTM_NEWADDR:
            case RTM_DELADDR:    
                nl_netifaddr_handle(nlh);
                break;
            case RTM_NEWROUTE:
            case RTM_DELROUTE:  
                nl_netroute_handle(nlh);
                break;
			case RTM_NEWNEIGH:
			case RTM_DELNEIGH:
				nl_netneigh_handle(nlh);
				break;
            default:
                break;
            }
        }
    }
}
 
void epoll_event_handle(void)
{
    int i = 0;
    int fd_cnt = 0;
    int sfd;
    struct epoll_event events[EPOLL_LISTEN_MAX_CNT];
 
    memset(events, 0, sizeof(events));
    while(1)
    {
        /* wait epoll event */
        fd_cnt = epoll_wait(g_epollfd, events, EPOLL_LISTEN_MAX_CNT, EPOLL_LISTEN_TIMEOUT);
        for(i = 0; i < fd_cnt; i++)
        {
            sfd = events[i].data.fd;
            if(events[i].events & EPOLLIN)
            {
                if (sfd == g_nlfd)
                {
                    nl_netlink_handle(sfd);
                }
            }
        }
    }
}
 
int epoll_add_fd(int fd)
{
    struct epoll_event ev;
 
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET;
 
    if (epoll_ctl(g_epollfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        LOG("epoll add fd error");
        return -1;
    }
 
    LOG("epoll add fd[%d] success\n", fd);
    return 0;
}
 
int init_epoll_fd()
{
    int epollfd = -1;
 
    epollfd = epoll_create(EPOLL_LISTEN_MAX_CNT);
    if (epollfd < 0) {
        LOG("epoll create failure!...");
        return -1;
    }
    g_epollfd = epollfd;
 
    LOG("epool create fd [%d] success\n", epollfd);
    return g_epollfd;
}
 
int init_nl_sockfd()
{
    int ret = 0;
    int nlfd = -1;
    struct sockaddr_nl sa;
 
    nlfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (nlfd < 0) {
        LOG("create netlink socket failure");
        return -1;
    }
 
    memset(&sa, 0, sizeof(sa));
    sa.nl_family = AF_NETLINK;
    sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV4_ROUTE | RTMGRP_NEIGH;
 
    ret = bind(nlfd, (struct sockaddr *)&sa, sizeof(sa));
    if (ret < 0) {
        LOG("bind nlfd error");
        close(nlfd);
        return -1;
    }
 
    if (epoll_add_fd(nlfd)) {
        close(nlfd);
        return -1;
    }
    g_nlfd = nlfd;
 
    LOG("netlink create fd [%d] success\n", nlfd);
    return nlfd;
}
 
 
void *rtm_thread(void *args)
{
    if (init_epoll_fd() < 0) { 
        return NULL;
    }
 
    if (init_nl_sockfd() < 0) {
        return NULL;
    }
 
    epoll_event_handle();
 
    return NULL;
}
