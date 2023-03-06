/* DNS.cpp
 * Joseph Shumway
 * CSCE 463
 * Spring 2023
 */

#include "pch.h"

using std::string;

void reformatIP(char** IPAddr) {
	string list[4];
	string IPAddrString = string(*IPAddr);
	//std::cout << IPAddrString << std::endl;
	UINT cursor = 0;

	int counter = 3;
	for (int i = 0; i < IPAddrString.length(); i++) {
		if ((*IPAddr)[i] == '.') {
			list[counter--] = IPAddrString.substr(cursor, i - cursor);
			i++;
			cursor = i;
		}
	}
	list[counter] = IPAddrString.substr(cursor, IPAddrString.length() - cursor);

	std::cout << "TEST 1: " << IPAddrString.c_str() << std::endl;
	IPAddrString = list[0] + "." + list[1] + "." + list[2] + "." + list[3] + ".in-addr.arpa";
	std::cout << "TEST 2: " << IPAddrString.c_str() << std::endl;

	memcpy((*IPAddr), IPAddrString.c_str(), strlen(IPAddrString.c_str()));
	(*IPAddr)[strlen(IPAddrString.c_str()) + 1] = 0;
}

void DNSWorker::formPacket(char** sendBuf, int packetSize, bool isHost) {
	
	dnsQHeader = (DNSQuestionHeader*)(*sendBuf);
	qHeader = (QueryHeader*)((*sendBuf) + packetSize - sizeof(QueryHeader));

	dnsQHeader->ID = htons(txid);
	dnsQHeader->flags = htons(DNS_QUERY | DNS_RD);
	dnsQHeader->questions = htons(1);

	char* tempHost = new char[256];
	memcpy(tempHost, host, strlen(host));
	tempHost[strlen(host)] = 0;
	host = tempHost;

	// reformat host or IP and add to packet;
	if (!isHost) {
		qTypeStr = "12";
		qHeader->qType = htons(12);
		qHeader->qClass = htons(1);
		reformatIP(&host);
	}
	else {
		qTypeStr = "1";
		qHeader->qType = htons(1);
		qHeader->qClass = htons(1);
	}

		USHORT hostLength = strlen(host);

		// convert hostname to DNS format
		//printf("Size: %d\n", hostLength);
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
		if (isHost) {
			dnsName[lastLengthPos] = length;
		}
		else {
			dnsName[lastLengthPos] = length - 1;
		}
		
		dnsName[hostLength + 1] = 0;

		//printf("DNS Name: %s\n", dnsName);


		if (isHost) {
			// Add the DNS question to the packet
			char* questionPtr = (*sendBuf) + sizeof(DNSQuestionHeader);
			memcpy(questionPtr, dnsName, hostLength + 2);
		}
		else {
			// Add the DNS question to the packet
			char* questionPtr = (*sendBuf) + sizeof(DNSQuestionHeader);
			memcpy(questionPtr, dnsName, hostLength);
		}
		
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
		//printf("Sending packet to %s\n", server);
	}
}



void DNSWorker::printQuery() {
	printf("Lookup  : %s\n", host);
	printf("Query   : %s, type %s, TXID 0x%.4X\n", host, qTypeStr.c_str(), txid);
	printf("Server  : %s\n", server);
	printf("********************************\n");
}

void DNSWorker::quit() {
	this->~DNSWorker();
	WSACleanup();
	exit(-1);
}