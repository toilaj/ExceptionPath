#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>

#define MAX_PAYLOAD 4096

int main() {
    int sockfd;
    struct sockaddr_nl src_addr, dest_addr;
    struct nlmsghdr *nlh;
    char buffer[MAX_PAYLOAD];

    sockfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sockfd < 0) {
        perror("socket creation failed");
        return EXIT_FAILURE;
    }

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();  // 自动分配端口
    src_addr.nl_groups = RTMGRP_NEIGH;  // 订阅邻居通知

    if (bind(sockfd, (struct sockaddr *)&src_addr, sizeof(src_addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        return EXIT_FAILURE;
    }

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;  // 发送到内核
    dest_addr.nl_groups = RTMGRP_NEIGH;  // 订阅邻居通知

    nlh = (struct nlmsghdr *)buffer;
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
    nlh->nlmsg_type = RTM_GETNEIGH;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
    nlh->nlmsg_seq = 0;
    nlh->nlmsg_pid = getpid();

    if (sendto(sockfd, nlh, nlh->nlmsg_len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("sendto failed");
        close(sockfd);
        return EXIT_FAILURE;
    }

    while (1) {
        int len = recv(sockfd, buffer, MAX_PAYLOAD, 0);
        if (len < 0) {
            perror("recv");
            close(sockfd);
            return EXIT_FAILURE;
        }

        for (struct nlmsghdr *nlmsg = (struct nlmsghdr *)buffer; NLMSG_OK(nlmsg, len); nlmsg = NLMSG_NEXT(nlmsg, len)) {
            struct ndmsg *ndm = (struct ndmsg *)NLMSG_DATA(nlmsg);
            struct rtattr *rta = (struct rtattr *)(((char *)ndm) + NLMSG_ALIGN(sizeof(struct ndmsg)));
            int rtl = len - NLMSG_LENGTH(sizeof(struct ndmsg));

            if (ndm->ndm_family != AF_INET)
                continue;

            char ip[INET_ADDRSTRLEN];
            char mac[18];
            struct in_addr *ipaddr = NULL;

            for (; RTA_OK(rta, rtl); rta = RTA_NEXT(rta, rtl)) {
                switch (rta->rta_type) {
                    case NDA_LLADDR:
                        if (RTA_PAYLOAD(rta) >= 6) {
                            unsigned char *macaddr = RTA_DATA(rta);
                            snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",
                                     macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
                        }
                        break;
                    case NDA_DST:
                        if (RTA_PAYLOAD(rta) >= sizeof(struct in_addr)) {
                            ipaddr = (struct in_addr *)RTA_DATA(rta);
                            inet_ntop(AF_INET, ipaddr, ip, sizeof(ip));
                        }
                        break;
                }
            }

            if (ipaddr != NULL) {
                printf("IP: %s, MAC: %s\n", ip, mac);
            }
        }
    }
go_out:
    close(sockfd);
    return EXIT_SUCCESS;
}

