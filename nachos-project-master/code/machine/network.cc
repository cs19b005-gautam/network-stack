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
    inHdr.length = 0;

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

    if (inHdr.length != 0)  // do nothing if packet is already buffered
        return;
    if (!PollSocket(sock))  // do nothing if no packet to be read
        return;

    // otherwise, read packet in
    char *buffer = new char[MaxWireSize+1];
    ReadFromSocket(sock, buffer, MaxWireSize);
    struct ethernetHeader *X = (struct ethernetHeader *)buffer;

    int ret1 = memcmp(X->destMAC, MacPool[kernel->hostName], 6);
    //int ret2 = memcmp(X->destMAC, MacPool[0], 6);


    if(ret1==0 ){
        // 26 is ethernet Header length
        struct ipv4Header * ipHdr = (struct ipv4Header *) (buffer+sizeof(*X));
        printf("received %d\n",ipHdr->frag_offset);
        cout<<X->payload+20<<endl;
        cout<<"........Ip details........"<<"\n";
        cout<<"Id -> "<<ipHdr->id<<endl;
        cout<<"Flag -> "<<ipHdr->flags<<endl;
        cout<<"Offset -> " << ipHdr->frag_offset<<endl;
        
        if(ipHdr->flags==0 && ipHdr->frag_offset==0){
            printf("1 completed packet received %d\n",ipHdr->id);
        }
        else{
            string keyVals[] = {to_string(ipHdr->src_addr), to_string(ipHdr->dest_addr), to_string(ipHdr->protocol), to_string(ipHdr->id)};
            string key = "";
            cout << "Packet Offset : " << ipHdr->frag_offset << "\t" << ipHdr->flags << endl;
            for(int i=0; i<4; i++){
                key += keyVals[i];
            }

            buffer[MaxWireSize] = '\0';
            string bufferStr = "";


            for(int i=26; i<MaxWireSize; i++){
                bufferStr += buffer[i];
            }
            cout << endl;

            /*kernel->ipDfragTable[key].end();
            auto last_item = kernel->ipDfragTable[key].end();
            last_item--;
            

            if(last->flags ==0){
                int offset = last->offset;
                cout << "Size of set and offset : " << offset << " " << offset/185 +1 << kernel->ipDfragTable[key].size() << endl;
                bool condi = offset/185 +1 == kernel->ipDfragTable[key].size();

                if(condi){
                    // here we receive full packet
                    printf("Final packet reeived from ip %s.......\n", to_string(last->src_addr));
                    string final_data = "";
                    for(auto hdr : kernel->ipDfragTable[key]){
                        cout << "Offset" << hdr.first << endl;
                        final_data += hdr.second.substr(sizeof(struct ipv4Header));
                    }
                    cout << "Final Data Received : " final_data.size() << endl; 

                    kernel->ipDfragTabole.erase(key);
                }
            }
            */
        }
    }
    else{
        kernel->stats->numPacketsDropped++;

        delete buffer;
        return;
    }
    // // divide packet into header and data
    // inHdr = *(PacketHeader *)buffer;
    // ASSERT((inHdr.to == kernel->hostName) && (inHdr.length <= MaxPacketSize));
    // bcopy(buffer + sizeof(PacketHeader), inbox, inHdr.length);
    // delete[] buffer;

    // DEBUG(dbgNet, "Network received packet from " << inHdr.from << ", length "
                                                //   << inHdr.length);
    kernel->stats->numPacketsRecvd++;

    // tell post office that the packet has arrived
    // callWhenAvail->CallBack();
}

//-----------------------------------------------------------------------
// NetworkInput::Receive
// 	Read a packet, if one is buffered
//-----------------------------------------------------------------------

void NetworkInput::Receive(char *data) {

    struct ethernetHeader* hdr  = (struct ethernetHeader *) data;
    char ip[20];
    bcopy(hdr->payload,ip,20);
    struct ipv4Header* ipHdr =(struct ipv4Header *)ip;
    int temp = ipHdr->frag_offset;
    data_set.insert(make_pair(temp,hdr));
    struct ethernetHeader* prv =NULL;
    bool flag=true;
    for(auto u:data_set)
    {
        if(prv==NULL)
        {
            prv = u.second;
            //cout<<(u.second->payload+20+8);
        }
        else
        {
            bcopy(u.second->payload,ip,20);
            struct ipv4Header* ipHdr_new =(struct ipv4Header *)ip;
            bcopy(prv->payload,ip,20);
            struct ipv4Header* ipHdr_prv =(struct ipv4Header *)ip;
            if(ipHdr_new->frag_offset!=ipHdr_prv->frag_offset+ipHdr_prv->len)
            {
                flag=false;
            }
        }
    }
    if(flag)
    {
        for(auto u:data_set)
        {
            cout<<(u.second->payload+20);
        }
        cout<<endl;
    }
    //PacketHeader hdr = inHdr;no 
    /*
    inHdr.length = 0;
    if (inHdr.length != 0) {
        bcopy(inbox, data, inHdr.length);
    }
    inHdr.length=0;
    */
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
    struct ipv4Header ipHdr = *(struct ipv4Header *) (buffer+sizeof(ethHdr));
    cout<<"........Ip details........"<<"\n";
	cout<<"Id -> "<<ipHdr.id<<endl;
	cout<<"Flag -> "<<ipHdr.flags<<endl;
	cout<<"Offset -> " << ipHdr.frag_offset<<endl;
	
    SendToSocket(sock, buffer, MaxWireSize, toName);
    // delete[] buffer;
}
