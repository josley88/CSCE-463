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

	//std::cout << "TEST 1: " << IPAddrString.c_str() << std::endl;
	IPAddrString = list[0] + "." + list[1] + "." + list[2] + "." + list[3] + ".in-addr.arpa";
	//std::cout << "TEST 2: " << IPAddrString.c_str() << std::endl;

	memcpy((*IPAddr), IPAddrString.c_str(), strlen(IPAddrString.c_str()));
	(*IPAddr)[strlen(IPAddrString.c_str())] = 0;
}

void DNSWorker::formPacket(char** sendBuf, int packetSize, bool isHost) {
	
	dnsQHeader = (FixedDNSHeader*)(*sendBuf);
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
			dnsName[lastLengthPos] = length;
		}
		
		dnsName[hostLength + 1] = 0;

		//printf("DNS Name: %s\n", dnsName);

		// Add the DNS question to the packet
		char* questionPtr = (*sendBuf) + sizeof(FixedDNSHeader);
		memcpy(questionPtr, dnsName, hostLength + 1);
		
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


bool DNSWorker::sendPacket(char** sendBuf, int packetSize) {
	remote.sin_family = AF_INET;
	remote.sin_addr.s_addr = inet_addr(server); // servers IP
	remote.sin_port = htons(53); // DNS port on server

	int count = 0;
	while (count++ < MAX_ATTEMPTS) {
		startTime = clock();
		timeval timeout;
		timeout.tv_sec = TIMEOUT;
		timeout.tv_usec = 0;

		printf("Attempt %d with %d bytes... ", count, packetSize);
		if (sendto(sock, (*sendBuf), packetSize, 0, (struct sockaddr*)&remote, sizeof(remote)) == SOCKET_ERROR) {
			printf("Send error: %d\n", WSAGetLastError());
			quit();
		}

		fd_set fd;
		FD_ZERO(&fd);
		FD_SET(sock, &fd);
		int available = select(0, &fd, NULL, NULL, &timeout);

		// response ready
		if (available > 0) {
			return true;
		}
		else { // timeout
			
			printf("timeout in %d ms\n", clock() - startTime);
		}
	}

	return false;
}

void DNSWorker::recvPacket(char** recvBuf) {

	struct sockaddr_in response;

	int responseSize = sizeof(response);
	int receivedBytes = 0;
	if ((receivedBytes = recvfrom(sock, (*recvBuf), MAX_DNS_SIZE, 0, (struct sockaddr*)&response, &responseSize)) == SOCKET_ERROR) {
		printf("Receive error: %d\n", WSAGetLastError());
		quit();
	}

	if (response.sin_addr.S_un.S_addr != remote.sin_addr.S_un.S_addr || response.sin_port != remote.sin_port) {
		printf("Response port or server mismatch!\n");
	}

	fixedDNSAnsHeader = (FixedDNSHeader*) (*recvBuf);
	printf("response in %d ms with %d bytes\n", clock() - startTime, receivedBytes);
	printf(
		"  TXID 0x%.4X, flags 0x%.4X, questions %d, answers %d, authority %d, additional %d\n",
		ntohs(fixedDNSAnsHeader->ID), 
		ntohs(fixedDNSAnsHeader->flags), 
		ntohs(fixedDNSAnsHeader->questions),
		ntohs(fixedDNSAnsHeader->answers),
		ntohs(fixedDNSAnsHeader->authRRs),
		ntohs(fixedDNSAnsHeader->addRRs)
	);

	uint8_t replyCode = ntohs(fixedDNSAnsHeader->flags) & 0x000F;
	
	if (replyCode != 0) {
		printf("  failed with Rcode = %d\n", replyCode);
		quit();
	}
	else {
		printf("  succeeded with Rcode = %d\n", replyCode);
	}
	
}

void DNSWorker::parsePacket(char** recvBuf) {
	
	int numQuestions = ntohs(fixedDNSAnsHeader->questions);
	char** qSections = new char*[numQuestions];

	int numAnswers = ntohs(fixedDNSAnsHeader->answers);
	char** aSections = new char* [numAnswers];
	
	// parse Questions
	for (int q = 0; q < numQuestions; q++) {
		char* question = (*recvBuf) + sizeof(FixedDNSHeader) + 1;
		int length = strlen(question);
		qSections[q] = question;

		for (u_int i = 0; i < length; i++) {
			if (question[i] < '0') {
				question[i] = '.';
			}
		}
	}
	

	printf("  ------------ [questions] ----------\n");
	
	for (int i = 0; i < numQuestions; i++) {
		QueryHeader* currentHeader = (QueryHeader*) (qSections[i] + strlen(qSections[i]) + 1);

		printf("  \t%s type %d class %d\n", 
			qSections[i],
			ntohs(currentHeader->qType),
			ntohs(currentHeader->qClass)
		);
	}



	// parse Answers
	for (int q = 0; q < numAnswers; q++) {
		char* answer = (*recvBuf) + sizeof(FixedDNSHeader) + 1;
		int length = strlen(answer);
		aSections[q] = answer;

		for (u_int i = 0; i < length; i++) {
			if (answer[i] < '0') {
				answer[i] = '.';
			}
		}
	}


	printf("  ------------ [answers] ----------\n");

	for (int i = 0; i < numAnswers; i++) {
		DNSAnswerHeader* currentHeader = (DNSAnswerHeader*)(aSections[i] + strlen(aSections[i]) + 1);

		printf("  \t%s type %d class %d\n",
			aSections[i],
			ntohs(currentHeader->aType),
			ntohs(currentHeader->aClass)
		);
	}






	delete[] aSections;
	delete[] qSections;
}


void DNSWorker::printQuery(char* originalHost) {
	printf("Lookup  : %s\n", originalHost);
	printf("Query   : %s, type %s, TXID 0x%.4X\n", host, qTypeStr.c_str(), txid);
	printf("Server  : %s\n", server);
	printf("********************************\n");
}

void DNSWorker::quit() {
	this->~DNSWorker();
	WSACleanup();
	exit(-1);
}