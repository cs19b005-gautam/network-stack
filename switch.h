struct ethernetHeader{
   char preamble[7];
   char sfd[1];
   char destMAC[6];
   char srcMAC[6];
   unsigned short int ethernetType;
   char payload[1500];
   unsigned int crc;
}__attribute__((packed)); 
