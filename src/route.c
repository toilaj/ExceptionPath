#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#define BUF_SIZE 4096

int get_kernel_route_table() {
    struct sockaddr_nl addr;
    struct nlmsghdr *nlh;
    char buffer[BUF_SIZE];
    int sockfd, len;

    sockfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTMGRP_IPV4_ROUTE; 
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    nlh = (struct nlmsghdr *)buffer;
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    nlh->nlmsg_type = RTM_GETROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    nlh->nlmsg_seq = 1;
    nlh->nlmsg_pid = getpid();

    if (send(sockfd, nlh, nlh->nlmsg_len, 0) < 0) {
        perror("send");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        len = recv(sockfd, buffer, BUF_SIZE, 0);
        if (len < 0) {
            perror("recv");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        for (struct nlmsghdr *nh = (struct nlmsghdr *)buffer; NLMSG_OK(nh, len); nh = NLMSG_NEXT(nh, len)) {
            if (nh->nlmsg_type == NLMSG_DONE)
                break;
            
            if (nh->nlmsg_type == RTM_NEWROUTE) {
                struct rtmsg *rtm = (struct rtmsg *)NLMSG_DATA(nh);
                struct rtattr *attr = (struct rtattr *)RTM_RTA(rtm);
                int rtl = RTM_PAYLOAD(nh);

                while (RTA_OK(attr, rtl)) {
                    if (attr->rta_type == RTA_DST || attr->rta_type == RTA_GATEWAY) {
                        char dst[INET_ADDRSTRLEN], gateway[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, RTA_DATA(attr), (attr->rta_type == RTA_DST) ? dst : gateway, sizeof(dst));
                        printf("Destination: %s, Gateway: %s\n", dst, gateway);
                    }
                    attr = RTA_NEXT(attr, rtl);
                }
            }
        }
    }

    close(sockfd);
    return 0;
}



int add_kernel_route_item() {
    struct sockaddr_nl addr;
    struct nlmsghdr *nlh;
    struct rtmsg *rtm;
    char buffer[BUF_SIZE];
    int sockfd, len;

    sockfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    nlh = (struct nlmsghdr *)buffer;
    rtm = (struct rtmsg *)NLMSG_DATA(nlh);

    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    nlh->nlmsg_type = RTM_NEWROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK;
    nlh->nlmsg_seq = 1;
    nlh->nlmsg_pid = getpid();

    rtm->rtm_family = AF_INET;         
    rtm->rtm_table = RT_TABLE_MAIN; 
    rtm->rtm_protocol = RTPROT_BOOT;
    rtm->rtm_scope = RT_SCOPE_UNIVERSE;
    rtm->rtm_type = RTN_UNICAST;

    struct rtattr *route_attr;
    int route_attr_len = RTA_LENGTH(sizeof(struct in_addr));
    route_attr = (struct rtattr *)((char *)rtm + NLMSG_ALIGN(sizeof(struct rtmsg)));
    route_attr->rta_type = RTA_DST;
    route_attr->rta_len = route_attr_len;
    inet_pton(AF_INET, "192.168.1.0", RTA_DATA(route_attr)); 

    route_attr = RTA_NEXT(route_attr, route_attr_len);
    route_attr->rta_type = RTA_GATEWAY;
    route_attr->rta_len = route_attr_len;
    inet_pton(AF_INET, "192.168.0.1", RTA_DATA(route_attr));

    if (send(sockfd, nlh, nlh->nlmsg_len, 0) < 0) {
        perror("send");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    len = recv(sockfd, buffer, BUF_SIZE, 0);
    if (len < 0) {
        perror("recv");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Route added successfully.\n");

    close(sockfd);
    return 0;
}


int delete_kernel_route_item() {
    struct sockaddr_nl addr;
    struct nlmsghdr *nlh;
    struct rtmsg *rtm;
    char buffer[BUF_SIZE];
    int sockfd, len;

    sockfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 构造 netlink 消息
    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    nlh = (struct nlmsghdr *)buffer;
    rtm = (struct rtmsg *)NLMSG_DATA(nlh);

    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    nlh->nlmsg_type = RTM_DELROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
    nlh->nlmsg_seq = 1;
    nlh->nlmsg_pid = getpid();

    rtm->rtm_family = AF_INET;        
    rtm->rtm_table = RT_TABLE_MAIN;
    rtm->rtm_protocol = RTPROT_BOOT;
    rtm->rtm_scope = RT_SCOPE_UNIVERSE;
    rtm->rtm_type = RTN_UNICAST;

    struct rtattr *route_attr;
    int route_attr_len = RTA_LENGTH(sizeof(struct in_addr));
    route_attr = (struct rtattr *)((char *)rtm + NLMSG_ALIGN(sizeof(struct rtmsg)));
    route_attr->rta_type = RTA_DST;
    route_attr->rta_len = route_attr_len;
    inet_pton(AF_INET, "192.168.1.0", RTA_DATA(route_attr));

    if (send(sockfd, nlh, nlh->nlmsg_len, 0) < 0) {
        perror("send");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    len = recv(sockfd, buffer, BUF_SIZE, 0);
    if (len < 0) {
        perror("recv");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Route deleted successfully.\n");

    close(sockfd);
    return 0;
}


int main() {
	get_kernel_route_table();
	return 0;
}
