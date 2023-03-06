// HW2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

int main(int argc, char** argv) {
	
	if (argc < 3) {
		printf("Not enough arguments given!\n");
		WSACleanup();
		exit(0);
	}
	
	DNSWorker dns(argv);
	bool isHost = false;
	int packetSize = 0;

	if (inet_addr(dns.host) != INADDR_NONE) {
		
		packetSize = strlen(dns.host) + 2 + strlen(".in-addr.arpa") + sizeof(DNSQuestionHeader) + sizeof(QueryHeader);
	}
	else {
		isHost = true;
		packetSize = strlen(dns.host) + 2 + sizeof(DNSQuestionHeader) + sizeof(QueryHeader);
	}

	
	char* sendBuf = new char[packetSize];
	memset(sendBuf, 0, packetSize);

	

	dns.formPacket(&sendBuf, packetSize, isHost);
	dns.openSocket();
	dns.sendPacket(&sendBuf, packetSize);
	dns.printQuery();

	delete[] sendBuf;
}