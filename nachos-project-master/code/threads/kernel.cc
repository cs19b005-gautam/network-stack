// kernel.cc
//	Initialization and cleanup routines for the Nachos kernel.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "main.h"
#include "kernel.h"
#include "sysdep.h"
#include "synch.h"
#include "synchlist.h"
#include "libtest.h"
#include "string.h"
#include "synchconsole.h"
#include "synchdisk.h"
#include "post.h"
#include "ethernet.h"
#include "ipv4header.h"
#include <cstdlib>
#include <time.h>

#define MAX_PROCESS 10
//----------------------------------------------------------------------
// Kernel::Kernel
// 	Interpret command line arguments in order to determine flags
//	for the initialization (see also comments in main.cc)
//----------------------------------------------------------------------

Kernel::Kernel(int argc, char **argv) {
    randomSlice = FALSE;
    debugUserProg = FALSE;
    consoleIn = NULL;   // default is stdin
    consoleOut = NULL;  // default is stdout
#ifndef FILESYS_STUB
    formatFlag = FALSE;
#endif
    reliability = 1;  // network reliability, default is 1.0
    hostName = 0;     // machine id, also UNIX socket name
                      // 0 is the default machine id
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-rs") == 0) {
            ASSERT(i + 1 < argc);
            RandomInit(atoi(argv[i + 1]));  // initialize pseudo-random
                                            // number generator
            randomSlice = TRUE;
            i++;
        } else if (strcmp(argv[i], "-s") == 0) {
            debugUserProg = TRUE;
        } else if (strcmp(argv[i], "-ci") == 0) {
            ASSERT(i + 1 < argc);
            consoleIn = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-co") == 0) {
            ASSERT(i + 1 < argc);
            consoleOut = argv[i + 1];
            i++;
#ifndef FILESYS_STUB
        } else if (strcmp(argv[i], "-f") == 0) {
            formatFlag = TRUE;
#endif
        } else if (strcmp(argv[i], "-n") == 0) {
            ASSERT(i + 1 < argc);  // next argument is float
            reliability = atof(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-m") == 0) {
            ASSERT(i + 1 < argc);  // next argument is int
            hostName = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-u") == 0) {
            cout << "Partial usage: nachos [-rs randomSeed]\n";
            cout << "Partial usage: nachos [-s]\n";
            cout << "Partial usage: nachos [-ci consoleIn] [-co consoleOut]\n";
#ifndef FILESYS_STUB
            cout << "Partial usage: nachos [-nf]\n";
#endif
            cout << "Partial usage: nachos [-n #] [-m #]\n";
        }
    }
}

//----------------------------------------------------------------------
// Kernel::Initialize
// 	Initialize Nachos global data structures.  Separate from the
//	constructor because some of these refer to earlier initialized
//	data via the "kernel" global variable.
//----------------------------------------------------------------------

void Kernel::Initialize(char *userProgName /*=NULL*/) {
    // We didn't explicitly allocate the current thread we are running in.
    // But if it ever tries to give up the CPU, we better have a Thread
    // object to save its state.
    currentThread = new Thread(userProgName);
    currentThread->setStatus(RUNNING);

    stats = new Statistics();        // collect statistics
    interrupt = new Interrupt;       // start up interrupt handling
    scheduler = new Scheduler();     // initialize the ready queue
    alarm = new Alarm(randomSlice);  // start up time slicing
    machine = new Machine(debugUserProg);
    synchConsoleIn = new SynchConsoleInput(consoleIn);     // input from stdin
    synchConsoleOut = new SynchConsoleOutput(consoleOut);  // output to stdout
    synchDisk = new SynchDisk();                           //
#ifdef FILESYS_STUB
    fileSystem = new FileSystem();
#else
    fileSystem = new FileSystem(formatFlag);
#endif  // FILESYS_STUB
    postOfficeIn = new PostOfficeInput(10);
    postOfficeOut = new PostOfficeOutput(reliability);

    addrLock = new Semaphore("addrLock", 1);
    gPhysPageBitMap = new Bitmap(128);
    semTab = new STable();
    pTab = new PTable(MAX_PROCESS);

    interrupt->Enable();
}

//----------------------------------------------------------------------
// Kernel::~Kernel
// 	Nachos is halting.  De-allocate global data structures.
//----------------------------------------------------------------------

Kernel::~Kernel() {
    delete stats;
    delete interrupt;
    delete scheduler;
    delete alarm;
    delete machine;
    delete synchConsoleIn;
    delete synchConsoleOut;
    delete synchDisk;
    delete fileSystem;
    delete postOfficeIn;
    delete postOfficeOut;
    delete pTab;
    delete gPhysPageBitMap;
    delete semTab;
    delete addrLock;

    Exit(0);
}

//----------------------------------------------------------------------
// Kernel::ThreadSelfTest
//      Test threads, semaphores, synchlists
//----------------------------------------------------------------------

void Kernel::ThreadSelfTest() {
    Semaphore *semaphore;
    SynchList<int> *synchList;

    LibSelfTest();  // test library routines

    currentThread->SelfTest();  // test thread switching

    // test semaphore operation
    semaphore = new Semaphore("test", 0);
    semaphore->SelfTest();
    delete semaphore;

    // test locks, condition variables
    // using synchronized lists
    synchList = new SynchList<int>;
    synchList->SelfTest(9);
    delete synchList;
}

//----------------------------------------------------------------------
// Kernel::ConsoleTest
//      Test the synchconsole
//----------------------------------------------------------------------

void Kernel::ConsoleTest() {
    char ch;

    cout << "Testing the console device.\n"
         << "Typed characters will be echoed, until ^D is typed.\n"
         << "Note newlines are needed to flush input through UNIX.\n";
    cout.flush();

    do {
        ch = synchConsoleIn->GetChar();
        if (ch != EOF) synchConsoleOut->PutChar(ch);  // echo it!
    } while (ch != EOF);

    cout << "\n";
}

//----------------------------------------------------------------------
// Kernel::NetworkTest
//      Test whether the post office is working. On machines #0 and #1, do:
//
//      1. send a message to the other machine at mail box #0
//      2. wait for the other machine's message to arrive (in our mailbox #0)
//      3. send an acknowledgment for the other machine's message
//      4. wait for an acknowledgement from the other machine to our
//          original message
//
//  This test works best if each Nachos machine has its own window
//----------------------------------------------------------------------

void Kernel::NetworkTest() {
    if (hostName <= 4 || hostName >= 0) {
        cout<<hostName<<endl;
        // if we're machine 1, send to 0 and vice versa
        unsigned char* farHost = MacPool[1-hostName];
        
        //generating packet for to and from addresses
        
        //generating ethernet Header
        struct ethernetHeader outPktHdr;
        PacketHeader pkthdr;
        //setting all values in ethernet Header
        memset(&outPktHdr, 0, sizeof(outPktHdr));

        //generation of data for packet
        char data[10000];
        for(int i=0; i<10000; i++){
            data[i] = 'a'+i%26;
        }
        int datasize = strlen(data);
        // what does this buffer store
        // char buffer[MaxMailSize];
        srand(time(0));

        //generating IP header
        struct ipv4Header ip;
        memset(&ip, 0, sizeof(ip));
        ip.version = 4;
        ip.ihl = 20;
        ip.len = datasize +ip.ihl;
        ip.id = rand()&(1<<16-1);
        ip.flags = 0;
        ip.src_addr = IPpool[hostName];
        ip.dest_addr = IPpool[hostName+1];

        char packetBuffer[1500];

        //fragmentation
        int numFrag = datasize/1480 +1;
        for(int i=0; i<numFrag; i++){
            cout << "Sending ip packet " << i << "\n";
            ip.frag_offset = i*1480/8;
            if(i!= numFrag-1){
                cout << "NumFrag : " << i << ",\t" << numFrag << "\n";
                ip.flags = 7;
            }
            else{
                ip.flags = 0;
            }

            cout<<"Ip details......................"<<i<<"\n";
            cout<<"Id -> "<<ip.id<<endl;
            cout<<"Flag -> "<<ip.flags<<endl;
            cout<<"Offset -> " << ip.frag_offset<<endl;

            memcpy(packetBuffer, &ip, sizeof(ip));
            int sizeLeft = i!=numFrag? 1480 : datasize%1480;
            memcpy(packetBuffer+sizeof(ip), data+i*1480, sizeLeft);
            // construct packet, mail header for original message
            // To: destination machine, mailbox 0
            // From: our machine, reply to: mailbox 1
            memcpy(outPktHdr.destMAC, farHost, 6);
            memcpy(outPktHdr.srcMAC, MacPool[hostName], 6);
            memcpy(outPktHdr.payload,packetBuffer,1500);
            // Sne d the first message
            pkthdr.to = (NetworkAddress)farHost;
            pkthdr.to = 0;
            pkthdr.from = 1;
            pkthdr.length = strlen(data) + 1;
            postOfficeOut->Send(outPktHdr,pkthdr); 
            //    void Send(PacketHeader pktHdr, MailHeader mailHdr, char *data);
        }

    }

    // Then we're done!
}
