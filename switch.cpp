#include <sys/socket.h>		//
#include <netinet/in.h> 	//
#include <sys/un.h>			//
#include <arpa/inet.h>		//
#include <stdio.h>			//
#include <sys/types.h>		//
#include <unistd.h>			//
#include <cstring>			//
#include <cmath>			//
#include <sys/select.h>		//
#include <algorithm>		//
#include <iostream>			//
#include <map>				//
#include "switch.h"			// for getting the ethernet header we will definiing in switch.h
 
using namespace std;


string char_to_string(unsigned char *MAC) {
	char *str = new char [100];
	sprintf(str,"%02X:%02X:%02X:%02X:%02X:%02X", MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);
	string srcMac = string(str);
	return srcMac;
}

void when_set(int& s1,struct sockaddr_un & saddr1,char buf[],map<string, int>&table,map<int, int>& portmap,int not_allowed)
{
	cout<<"Got it in : "<<s1<<endl;
	unsigned int AddrSize = sizeof(saddr1);
	// recvFrom
	int retsz = recvfrom(s1, buf, 1526, 0, (struct sockaddr *)&saddr1, &AddrSize);
	struct ethernetHeader *X;

	// in what format is X being returned
	X = (struct ethernetHeader *)buf;


	string srcMac = char_to_string(X->srcMAC);
	table[srcMac] = not_allowed;
	cout<<"srcmac is "<<srcMac<<endl;
	printf("--> %d\n", table[char_to_string(X->srcMAC)]);
	printf("received %d bytes on S1 from %02X:%02X:%02X:%02X:%02X:%02X\n", retsz, X->srcMAC[0], X->srcMAC[1], X->srcMAC[2], X->srcMAC[3], X->srcMAC[4], X->srcMAC[5]);

	struct ipv4Header ipHdr = *(struct ipv4Header *) (X->payload+sizeof(UdpHeader));
	cout<<"........Ip details........"<<"\n";
	cout<<"Id -> "<<ipHdr.id<<endl;
	cout<<"Flag -> "<<ipHdr.flags<<endl;
	cout<<"Offset -> " << ipHdr.frag_offset<<endl;
	
	cout << "===========TABLE=========="<<endl;

	
	for (auto itr = table.begin(); itr != table.end(); itr++){
		std::cout << itr->first << " " << itr->second << std::endl;
	}
	cout<<">>>> BUFF1"<<buf<<endl;
	cout<<">>>> X srcMAC:"<<X->srcMAC<<endl;
	cout<<">>>> X destMAC:"<<X->destMAC<<endl;
	cout << "=========================="<<endl;


	if (table.find(char_to_string(X->destMAC)) != table.end())
	{
		int dport = table[char_to_string(X->destMAC)];
		char name[100]; // just path for nachos instance
		sprintf(name, "/tmp/N%d", dport);
		struct sockaddr_un dname;
		dname.sun_family = AF_UNIX;
		strcpy(dname.sun_path, name);
		int rv = sendto(portmap[dport], buf, retsz, 0, (struct sockaddr *)&dname, sizeof(dname));
		printf("Send [%d] bytes to [%s]\n", rv, name);
	}
	else
	{
		for (int k = 1; k <= 4; k++)
		{
			if (k != not_allowed)
			{
				char name[100]; 
				sprintf(name, "/tmp/N%d", k);
				struct sockaddr_un dname;
				dname.sun_family = AF_UNIX;
				strcpy(dname.sun_path, name);
				int rv = sendto(portmap[k], buf, retsz, 0, (struct sockaddr *)&dname, sizeof(dname));
				printf("Broadcast Send [%d] bytes to [%s]\n", rv, name);
			}
		}
	}
}

int main()
{
	std::cout << "cleaning S[1-4] & N[1-4]..." << std::endl; // why do we need to clean all the S[1-4] codes ??
	system("rm -rf /tmp/S[1-4]"); // why store the in tmp files ??
	system("rm -rf /tmp/N[1-4]"); // what are theese S[1-4] and N[1-4] files ?? 

	map<int, int> portmap;
	struct ethernetHeader eth;
	cout << "Size of eth:" << sizeof(eth) << endl;
	
	int s1 = socket(AF_UNIX, SOCK_DGRAM, 0);
	int s2 = socket(AF_UNIX, SOCK_DGRAM, 0);
	int s3 = socket(AF_UNIX, SOCK_DGRAM, 0);
	int s4 = socket(AF_UNIX, SOCK_DGRAM, 0);
	


	// insereting port values into
	portmap.insert({1, s1});
	portmap.insert({2, s2});
	portmap.insert({3, s3});
	portmap.insert({4, s4});

	int optval = 1;
	setsockopt(s1, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
	optval = 1;																//why do we need to keep on initilizing with 1, everytime
	setsockopt(s2, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
	optval = 1;
	setsockopt(s3, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
	optval = 1;
	setsockopt(s4, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

	struct sockaddr_un saddr1;
	saddr1.sun_family = AF_UNIX;
	strcpy(saddr1.sun_path, "/tmp/S1");
	int bind1 = bind(s1, (struct sockaddr *)&saddr1, sizeof(saddr1));
	struct sockaddr_un saddr2;
	saddr2.sun_family = AF_UNIX;
	strcpy(saddr2.sun_path, "/tmp/S2");
	int bind2 = bind(s2, (struct sockaddr *)&saddr2, sizeof(saddr2));
	struct sockaddr_un saddr3;
	saddr3.sun_family = AF_UNIX;
	strcpy(saddr3.sun_path, "/tmp/S3");
	int bind3 = bind(s3, (struct sockaddr *)&saddr3, sizeof(saddr3));
	struct sockaddr_un saddr4;
	saddr4.sun_family = AF_UNIX;
	strcpy(saddr4.sun_path, "/tmp/S4");
	int bind4 = bind(s4, (struct sockaddr *)&saddr4, sizeof(saddr4));
	
	map<string, int> table;

	while (true)
	{
		
		//why exaclty is fd_set used ??
		
		fd_set rd;
		FD_ZERO(&rd);
		FD_SET(s1, &rd);
		FD_SET(s2, &rd);
		FD_SET(s3, &rd);
		FD_SET(s4, &rd);
		int max_fds = max(s1, max(s2, max(s3, s4))) + 1;		// ?????


		//timeval is used in windows socket by select function to specify the maximum time the function take to complete
		struct timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;


		//why is select function used and what does it return
		int sel = select(max_fds, &rd, NULL, NULL, &tv);

		if((sel!=0))
		cout << "\nselect() returns: " << sel << endl;
		char buf[2000];
		

		if (FD_ISSET(s1, &rd))
		{
			when_set( s1,saddr1,buf,table,portmap,1);
		}

		if (FD_ISSET(s2, &rd))
		{
			when_set( s2,saddr2,buf,table,portmap,2);
		}
		if (FD_ISSET(s3, &rd)){
			when_set( s3,saddr3,buf,table,portmap,3);
		}

		if (FD_ISSET(s4, &rd))
		{
			when_set( s4,saddr4,buf,table,portmap,4);
		}
	}

	close(s1);
	close(s2);
	close(s3);
	close(s4);
	unlink("/tmp/S1");
	unlink("/tmp/S2");
	unlink("/tmp/S3");
	unlink("/tmp/S4");
}
