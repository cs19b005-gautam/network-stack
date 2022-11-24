#ifndef _UDP_H
#define _UDP_H

struct UdpHeader{
	unsigned short int srcPort;
	unsigned short int destPort;
	unsigned short int length;
	unsigned short int checksum;
}__attribute__((packed));

struct UdpPacket{
    UdpHeader UdpHdr;
    char* data;
}__attribute__((packed));

#endif
