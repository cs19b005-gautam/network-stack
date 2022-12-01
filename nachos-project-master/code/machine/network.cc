// network.cc
//	Routines to simulate a network interface, using UNIX sockets
//	to deliver packets between multiple invocations of nachos.
//
//  DO NOT CHANGE -- part of the machine emulation
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "network.h"
#include "main.h"
#include "ipv4header.h"
#include "udpheader.h"
#include "map"
#include <string>
#include <cstring>

//-----------------------------------------------------------------------
// NetworkInput::NetworkInput
// 	Initialize the simulation for the network input
//
//   	"toCall" is the interrupt handler to call when packet arrives
//-----------------------------------------------------------------------


NetworkInput::NetworkInput(CallBackObj *toCall) {
    // set up the stuff to emulate asynchronous interrupts
    callWhenAvail = toCall;
    packetAvail = FALSE;
    inHdr.srcMAC[0] = 0;

    sock = OpenSocket();
    sprintf(sockName, "/tmp/N%d", kernel->hostName);
    AssignNameToSocket(sockName, sock);  // Bind socket to a filename
                                         // in the current directory.

    // start polling for incoming packets
    kernel->interrupt->Schedule(this, NetworkTime, NetworkRecvInt);
}

//-----------------------------------------------------------------------
// NetworkInput::NetworkInput
// 	Deallocate the simulation for the network input
//		(basically, deallocate the input mailbox)
//-----------------------------------------------------------------------

NetworkInput::~NetworkInput() {
    CloseSocket(sock);
    DeAssignNameToSocket(sockName);
}

//-----------------------------------------------------------------------
// NetworkInput::CallBack
//	Simulator calls this when a packet may be available to
//	be read in from the simulated network.
//
//      First check to make sure packet is available & there's space to
//	pull it in.  Then invoke the "callBack" registered by whoever
//	wants the packet.
//-----------------------------------------------------------------------

void NetworkInput::CallBack() {
    // schedule the next time to poll for a packet
    kernel->interrupt->Schedule(this, NetworkTime, NetworkRecvInt);
    
    if (inHdr.srcMAC[0] != 0)  // do nothing if packet is already buffered
        return;
    if (!PollSocket(sock))  // do nothing if no packet to be read
        return;

    // otherwise, read packet in
    char *buffer = new char[MaxWireSize];
    ReadFromSocket(sock, buffer, MaxWireSize);

    // divide packet into header and data
    inHdr = *(ethernetHeader *)buffer;
    ASSERT((inHdr.destMAC[0] == MacPool[kernel->hostName][0])&&(inHdr.destMAC[1] == MacPool[kernel->hostName][1]) 
    &&(inHdr.destMAC[2] == MacPool[kernel->hostName][2])&&(inHdr.destMAC[3] == MacPool[kernel->hostName][3])
    &&(inHdr.destMAC[4] == MacPool[kernel->hostName][4])&&(inHdr.destMAC[5] == MacPool[kernel->hostName][5]));
    bcopy(buffer , inbox, MaxWireSize);
    buffer[MaxWireSize-1]='\0';
    delete[] buffer;

    //DEBUG(dbgNet, "Network received packet from " << inHdr.from << ", length "  << inHdr.length);
    kernel->stats->numPacketsRecvd++;

    // tell post office that the packet has arrived
    callWhenAvail->CallBack();
}

//-----------------------------------------------------------------------
// NetworkInput::Receive 0 1472 2944 4416 5888 7360 640
// 	Read a packet, if one is buffered
//-----------------------------------------------------------------------

ethernetHeader NetworkInput::Receive(char *data) {
    ethernetHeader hdr = inHdr;

    inHdr.srcMAC[0] = 0;
    ipv4Header ipHdr = *(ipv4Header * )(hdr.payload+sizeof(UdpHeader));
    cout<<"Got the data from IP"<<endl;
    cout<<"Id -> "<<ipHdr.id<<endl;
	cout<<"Flag -> "<<ipHdr.flags<<endl;
	cout<<"Offset -> " << ipHdr.frag_offset<<endl;
    int v =ipHdr.frag_offset;
	data_set.insert(pack(ipHdr.id,v,hdr));
    ipHdr.frag_offset =0 ;
    ipHdr.len=0;
    std::map<int,bool> flag;
    std::map<int,bool>end;
    int prv_id=-1;
    int prv_offset=0;
    int prv_len =0;
    for(auto u:data_set)
    {
        ipHdr = *(ipv4Header * )(u.second.payload+sizeof(UdpHeader));
        if(ipHdr.flags==0)
        end[ipHdr.id]=true;
        if(prv_id==-1||prv_id==ipHdr.id)
        {
            if(prv_id==-1)
            flag[ipHdr.id]=true;
            if((prv_id==-1&&ipHdr.frag_offset==0)||8*prv_offset+prv_len==8*ipHdr.frag_offset)
                prv_id = ipHdr.id,prv_offset = ipHdr.frag_offset,prv_len=ipHdr.len;
            else
            {
                cout<<"Did not find offset : "<<prv_offset+prv_len/8<<" , "<<ipHdr.frag_offset<<endl;
                flag[ipHdr.id]=false;
            }
        }
        else
        {
            flag[ipHdr.id]=true;
            prv_id = ipHdr.id,prv_offset = ipHdr.frag_offset,prv_len=ipHdr.len;
        }
    }
    string s;
    for( auto u:end)
    {
        cout<<"Found end of : "<<u.first<<endl;
        if(flag[u.first]==true)
        {
            cout<<"Got all packets for : "<<u.first<<endl;
            for(auto packet:data_set)
            {
                ipHdr = *(ipv4Header * )(packet.second.payload+sizeof(UdpHeader));
                if(ipHdr.id==u.first)
                s+=packet.second.payload+sizeof(UdpHeader)+sizeof(ipv4Header);
            }
            cout<<"UDP packet with id : "<<u.first<<endl;
            cout<<s<<endl;
        }
    }
    if (hdr.srcMAC[0] != 0) {
        bcopy(inbox, data, MaxWireSize);
    }
    return hdr;
}

//-----------------------------------------------------------------------
// NetworkOutput::NetworkOutput
// 	Initialize the simulation for sending network packets
//
//   	"reliability" says whether we drop packets to emulate unreliable links
//   	"toCall" is the interrupt handler to call when next packet can be sent
//-----------------------------------------------------------------------

NetworkOutput::NetworkOutput(double reliability, CallBackObj *toCall) {
    if (reliability < 0)
        chanceToWork = 0;
    else if (reliability > 1)
        chanceToWork = 1;
    else
        chanceToWork = reliability;

    // set up the stuff to emulate asynchronous interrupts
    callWhenDone = toCall;
    sendBusy = FALSE;
    sock = OpenSocket();
}

//-----------------------------------------------------------------------
// NetworkOutput::~NetworkOutput
// 	Deallocate the simulation for sending network packets
//-----------------------------------------------------------------------

NetworkOutput::~NetworkOutput() { CloseSocket(sock); }

//-----------------------------------------------------------------------
// NetworkOutput::CallBack
// 	Called by simulator when another packet can be sent.
//-----------------------------------------------------------------------

void NetworkOutput::CallBack() {
    sendBusy = FALSE;
    kernel->stats->numPacketsSent++;
    callWhenDone->CallBack();
}

//-----------------------------------------------------------------------
// NetworkOutput::Send
// 	Send a packet into the simulated network, to the destination in hdr.
// 	Concatenate hdr and data, and schedule an interrupt to tell the user
// 	when the next packet can be sent
//
// 	Note we always pad out a packet to MaxWireSize before putting it into
// 	the socket, because it's simpler at the receive end.
//-----------------------------------------------------------------------

void NetworkOutput::Send(struct ethernetHeader ethHdr)//(PacketHeader hdr, char *data) 
{
    char toName[32];
    char* data =ethHdr.payload;
    int dataSize = sizeof(data)/sizeof(char);
    
    sprintf(toName, "/tmp/S%d", kernel->hostName);
    printf("toName is %s\n",toName);
	printf("Got datasize: %d\n", dataSize);
	printf("Max Packet size: %d\n", MaxPacketSize);
    
    /*ASSERT((sendBusy == FALSE) && (hdr.length > 0) &&
           (hdr.length <= MaxPacketSize) && (hdr.from == kernel->hostName));
    DEBUG(dbgNet, "Sending to addr " << hdr.to << ", length " << hdr.length);
    */

    kernel->interrupt->Schedule(this, NetworkTime, NetworkSendInt);

    /*
    if (RandomNumber() % 100 >= chanceToWork * 100) {  // emulate a lost packet
        DEBUG(dbgNet, "oops, lost it!");
        return;
    }
    */

    // concatenate hdr and data into a single buffer, and send it out
    char *buffer = new char[MaxWireSize];
    //*(PacketHeader *)buffer = hdr;
    //bcopy(data, buffer + sizeof(PacketHeader), hdr.length);
    memcpy(buffer, &ethHdr, 1526);
    /*
    struct ipv4Header ipHdr = *(struct ipv4Header *) (buffer+sizeof(ethHdr));
    cout<<"........Ip details........"<<"\n";
	cout<<"Id -> "<<ipHdr.id<<endl;
	cout<<"Flag -> "<<ipHdr.flags<<endl;
	cout<<"Offset -> " << ipHdr.frag_offset<<endl;
    */
	
    SendToSocket(sock, buffer, MaxWireSize, toName);
    // delete[] buffer;
}
