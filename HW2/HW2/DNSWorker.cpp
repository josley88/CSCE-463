/* DNS.cpp
 * Joseph Shumway
 * CSCE 463
 * Spring 2023
 */

#include "pch.h"

using std::string;

void DNSWorker::formPacket(char** sendBuf, int packetSize) {
	
	dnsQHeader = (DNSQuestionHeader*)(*sendBuf);
	qHeader = (QueryHeader*)((*sendBuf) + packetSize - sizeof(QueryHeader));

	dnsQHeader->ID = htons(txid);
	dnsQHeader->flags = htons(DNS_QUERY | DNS_RD);
	dnsQHeader->questions = htons(1);

	if (inet_addr(host) != INADDR_NONE) {
		qTypeStr = "A";
		qHeader->qType = htons(1);
	}
	else {
		qTypeStr = "PTR";
		qHeader->qType = htons(1);
	}

	qHeader->qClass = htons(1);

	USHORT hostLength = strlen(host);

	// convert hostname to DNS format
	printf("Size: %d\n", hostLength);
	char* dnsName = new char[hostLength + 2];
	memcpy(dnsName + 1, host, hostLength);
	dnsName[0] = '.';
	
	int length = 0;
	int lastLengthPos = 0;

	// for each character in dnsName, replace '.' with the length of the following characters before the next '.'
	for (int i = 0; i < hostLength + 1; i++) {
		if (dnsName[i] == '.') {
			dnsName[lastLengthPos] = length;
			length = 0;
			lastLengthPos = i;
		}
		else {
			length++;
		}
	}
	dnsName[lastLengthPos] = length;
	dnsName[hostLength + 1] = 0;
	
	printf("DNS Name: %s\n", dnsName);

	

	// Add the DNS question to the packet
	char* questionPtr = (*sendBuf) + sizeof(DNSQuestionHeader);
	memcpy(questionPtr, dnsName, hostLength + 2);
	delete[] dnsName;
}

void DNSWorker::openSocket() {
	

	WSADATA wsaData;

	// Initialize WinSock
	WORD wVersionRequested = MAKEWORD(2, 2);
	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		printf("WSAStartup error %d\n", WSAGetLastError());
		quit();
	}

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) {
		printf("socket() generated error %d\n", WSAGetLastError());
		quit();
	}

	struct sockaddr_in local;
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons(0);
	if (bind(sock, (struct sockaddr*)&local, sizeof(local)) == SOCKET_ERROR) {
		printf("Binding port error: %d\n", WSAGetLastError());
		quit();
	}

}


void DNSWorker::sendPacket(char** sendBuf, int packetSize) {

	for (int i = 0; i < packetSize; ++i) {
		printf("%X", (int)(*sendBuf)[i]);
	}
	printf("\n");
	for (int i = 0; i < packetSize; ++i) {
		printf("%c", (*sendBuf)[i]);
	}
	printf("\n");
	

	struct sockaddr_in remote;
	memset(&remote, 0, sizeof(remote));
	remote.sin_family = AF_INET;
	remote.sin_addr.s_addr = inet_addr(server); // server’s IP
	remote.sin_port = htons(53); // DNS port on server
	if (sendto(sock, (*sendBuf), packetSize, 0, (struct sockaddr*)&remote, sizeof(remote)) == SOCKET_ERROR) {
		printf("Send error: %d\n", WSAGetLastError());
		quit();
	}
	else {
		printf("Sending packet to %s\n", server);
	}
}



void DNSWorker::printQuery() {
	printf("Lookup  : %s\n", host);
	printf("Query   : %s, type %s, TXID 0x%.4X\n", host, qTypeStr.c_str(), txid);
	printf("Server  : %s\n", server);
}

void DNSWorker::quit() {
	this->~DNSWorker();
	WSACleanup();
	exit(-1);
}