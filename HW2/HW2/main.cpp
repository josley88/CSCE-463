/* main.cpp
 * Joseph Shumway
 * CSCE 463
 * Spring 2023
 */

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
		
		packetSize = strlen(dns.host) + 2 + strlen(".in-addr.arpa") + sizeof(FixedDNSHeader) + sizeof(QueryHeader);
	}
	else {
		isHost = true;
		packetSize = strlen(dns.host) + 2 + sizeof(FixedDNSHeader) + sizeof(QueryHeader);
	}

	
	char* sendBuf = new char[packetSize];
	char* recvBuf = new char[MAX_DNS_SIZE];
	memset(sendBuf, 0, packetSize);

	

	dns.formPacket(&sendBuf, packetSize, isHost);
	dns.printQuery(argv[1]);
	dns.openSocket();
	if (dns.sendPacket(&sendBuf, packetSize)) {
		dns.recvPacket(&recvBuf);
		dns.parsePacket(&recvBuf);
	}
	else {
		printf("");
	}

	

	delete[] sendBuf;
}