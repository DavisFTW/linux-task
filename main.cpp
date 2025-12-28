#define _GNU_SOURCE
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <netpacket/packet.h>
#include <iostream>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

void printInterfaces() {

    struct ifaddrs *ifaddr;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        /* Display interface name and family (including symbolic
           form of the latter for the common families). */

        if (family == AF_PACKET) {
            const auto* ll = (const struct sockaddr_ll *)ifa->ifa_addr;
            printf("%-8s MAC: ", ifa->ifa_name);  // Add this
            for (int i = 0; i < 6; i++) {
                printf("%02x", ll->sll_addr[i]);
                if (i < 5) printf(":");
            }
            printf("\n");  // ADD THIS - newline after MAC
        }
        else if (family == AF_INET || family == AF_INET6) {
            s = getnameinfo(ifa->ifa_addr,(family == AF_INET) ? sizeof(struct sockaddr_in) :sizeof(struct sockaddr_in6),host, NI_MAXHOST,NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }

            printf("%-8s IP: %s\n", ifa->ifa_name, host);

            if (ifa->ifa_netmask != NULL ) {
                char netmask[NI_MAXHOST];  // Create a SEPARATE buffer for netmask
                s = getnameinfo(ifa->ifa_netmask, sizeof(struct sockaddr_in), netmask, NI_MAXHOST,NULL, 0, NI_NUMERICHOST);
                if (s == 0) {  // Check it succeeded
                    printf(" Netmask: %s\n", netmask);
                }
            }
        }
    }
    freeifaddrs(ifaddr);

}

int main(int argc, char *argv[])
{
    printInterfaces();

    struct sockaddr_nl sa;
    const auto fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

    if (fd == -1) {
        //fail
        perror("socket");
        exit(EXIT_FAILURE);
    }
    sa.nl_family = AF_NETLINK;
    sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR;  // <-- This is subscribing!

    const auto bindErr = bind(fd, (struct sockaddr*)&sa, sizeof(sa));
    if (bindErr == -1) {
        perror("bind netlink");
        exit(EXIT_FAILURE);
    }

    while (true) {
        bool needsRefresh = false;
        fd_set rfds, wfds, efds;
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        FD_ZERO(&efds);

        FD_SET(fd, &rfds);
        const auto selectStatus = select(fd+1, &rfds, &wfds, &efds, NULL);
        if (selectStatus == -1) {
            perror("select");
            continue;
        }

        if (!FD_ISSET(fd, &rfds)) {
            continue;
        }
        char buffer[4096];
        const auto bytesReceived = recvfrom(fd, buffer, sizeof(buffer), 0, NULL, NULL);
        if (bytesReceived == -1) {
            perror("recvfrom");
        }
        int len = bytesReceived;
        auto x = reinterpret_cast<nlmsghdr*>(buffer);
        while (NLMSG_OK(x, len)) {
            if (x->nlmsg_type == RTM_NEWADDR ||
                 x->nlmsg_type == RTM_DELADDR ||
                 x->nlmsg_type == RTM_NEWLINK ||
                 x->nlmsg_type == RTM_DELLINK) {

                // #CLAUDE dont we need to read the message ?
                needsRefresh = true;
                                                }
            x = NLMSG_NEXT(x, len);
        }
        if (needsRefresh) {
            printInterfaces();
        }
    }
    exit(EXIT_SUCCESS);
}