#ifndef _IPETH_H
#define _IPETH_H
#include <cstdint>
#include <arpa/inet.h>


struct ipv4Header{
    uint8_t version : 4;
    uint8_t ihl : 4;                    // Internet "Header Length"
    uint8_t tos;                        // Diffferentiated Services Code Point (DSCP) + ECN (Explicit Congestion Notification) - features related to quality of service for data streaming or Voice Over IP calls. Specifies how you handle Datagram
    uint16_t len;                       // total length
    uint16_t id;                        // identification
    uint16_t flags : 3;                 // flags
    uint16_t frag_offset : 13;          // fragmentation offest
    uint8_t ttl;                        // time to live
    uint8_t protocol;
    uint16_t check_sum;
    in_addr_t src_addr;
    in_addr_t dest_addr;                // in_addr_t => uint32_t
}__attribute__((packed));


static in_addr_t IPpool[5]={
    inet_addr("192.168.3.2"),  //
    inet_addr("192.168.3.3"),
    inet_addr("192.168.3.4"),
    inet_addr("192.168.3.5"),
    inet_addr("192.168.3.6")
};

#endif