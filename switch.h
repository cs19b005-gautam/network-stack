struct ethernetHeader{
    char preamble[7];
    char sfd[1];
    unsigned char destMAC[6];
    unsigned char srcMAC[6];
    unsigned short int ethernetType;
    char payload[1500];// 20 bytes - IP header, 8 bytes - UDP Header, 1472 bytes - data
    unsigned int crc;
}__attribute__((packed));

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
