
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_link.h>
#include <netpacket/packet.h>  // <-- ADD THIS
#include <iostream>
       int main(int argc, char *argv[])
       {
           struct ifaddrs *ifaddr;
           int family, s;
           char host[NI_MAXHOST];

           if (getifaddrs(&ifaddr) == -1) {
               perror("getifaddrs");
               exit(EXIT_FAILURE);
           }

           /* Walk through linked list, maintaining head pointer so we
              can free list later. */

           for (struct ifaddrs *ifa = ifaddr; ifa != NULL;
                    ifa = ifa->ifa_next) {
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
               }}



           freeifaddrs(ifaddr);
           exit(EXIT_SUCCESS);
       }

