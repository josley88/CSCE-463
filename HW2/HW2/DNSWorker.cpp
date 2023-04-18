/* DNSWorker.cpp
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
	while (count < MAX_ATTEMPTS) {
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

	count++;

	return false;
}

void DNSWorker::recvPacket(char** recvBuf) {

	struct sockaddr_in response;

	int responseSize = sizeof(response);
	int receivedBytes = 0;
	if ((receivedBytes = recvfrom(sock, (*recvBuf), MAX_DNS_SIZE, 0, (struct sockaddr*)&response, &responseSize)) == SOCKET_ERROR) {
		printf("socket error %d\n", WSAGetLastError());
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

	if (ntohs(fixedDNSAnsHeader->ID) != txid) {
		printf("  ++\tinvalid reply: TXID mismatch, sent %.4X, received %.4X\n", txid, ntohs(fixedDNSAnsHeader->ID));
		quit();
	}

	if (receivedBytes < 12) {
		printf("  ++\tinvalid reply: packet smaller than fixed DNS header\n");
		quit();
	}
	

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
	char** ansSections = new char* [numAnswers];

	int numAuthority = ntohs(fixedDNSAnsHeader->authRRs);
	char** authSections = new char* [numAuthority];

	int numAdditional = ntohs(fixedDNSAnsHeader->addRRs);
	char** addSections = new char* [numAdditional];

	if (numQuestions > 100) {
		printf("  ++\tinvalid record: RR value length stretches the answer beyond packet\n");
		quit();
	}

	if (numAnswers > 100) {
		printf("  ++\tinvalid record: RR value length stretches the answer beyond packet\n");
		quit();
	}

	if (numAuthority > 100) {
		printf("  ++\tinvalid record: RR value length stretches the answer beyond packet\n");
		quit();
	}

	if (numAdditional > 100) {
		printf("  ++\tinvalid record: RR value length stretches the answer beyond packet\n");
		quit();
	}




	// parse Questions
	printf("  ------------ [questions] ----------\n");
	char* qSection = (*recvBuf) + sizeof(FixedDNSHeader) + 1;
	char* question = qSection;
	for (int i = 0; i < numQuestions; i++) {
		int length = strlen(question);

		if ((question[0] & 0xC0) == 0xC0) { // compression detected. Jump!
			int offset = ntohs(*(USHORT*)question) & 0x3FFF;
			char* label = (*recvBuf) + offset + 1;

			int labelLength = strlen(label);
			length = labelLength + 2;
			qSections[i] = label;
			//printf("Jumped to %X\n", label);
		}
		else {
			qSections[i] = question;

			if (length == 0) {
				printf("  ++\tError: malformed packet\n");
				delete[] qSections;
				delete[] ansSections;
				delete[] authSections;
				delete[] addSections;
				quit();
			}

			// convert char lengths to dots
			for (u_int j = 0; j < length; j++) {
				if (question[j] < '0') {
					question[j] = '.';
				}
			}
		}


		// print header and name info
		QueryHeader* currentHeader = (QueryHeader*)(question + strlen(question) + 1);

		//printf("Type: %d\n", ntohs(currentHeader->TTL));

		printf("  \t%s type %d class %d\n",
			qSections[i],
			ntohs(currentHeader->qType),
			ntohs(currentHeader->qClass)
		);


		question += sizeof(QueryHeader) + length + 2;
	}




	// parse Answers
	printf("  ------------ [answers] ----------\n");
	char* answerSection = qSections[numQuestions - 1] + strlen(qSections[numQuestions - 1]) + 5;
	char* answer = answerSection;
	DNSAnswerHeader* currentHeader;
	for (int i = 0; i < numAnswers; i++) {
		int length = strlen(answer);

		if ((answer[0] & 0xC0) == 0xC0) { // compression detected. Jump!
			int offset = ntohs(*(USHORT*)answer) & 0x3FFF;
			char* label = (*recvBuf) + offset + 1;

			if (offset > sizeof(recvBuf)) {
				printf("  ++\tinvalid record: truncated jump offset\n");
				delete[] qSections;
				delete[] ansSections;
				delete[] authSections;
				delete[] addSections;
				quit();
			}

			int labelLength = strlen(label);
			length = labelLength + 2;
			ansSections[i] = label;
			//printf("Jumped to %X\n", label);
			currentHeader = (DNSAnswerHeader*)(answer + 2);
		}
		else {
			ansSections[i] = answer + 1;

			if (length == 0) {
				printf("  ++\tinvalid record: RR value length stretches the answer beyond packet\n");
				delete[] qSections;
				delete[] ansSections;
				delete[] authSections;
				delete[] addSections;
				quit();
			}

			// convert char lengths to dots
			for (u_int j = 0; j < length - 1; j++) {
				if (answer[j] < '0') {
					answer[j] = '.';
				}
			}

			currentHeader = ((DNSAnswerHeader*)(answer + strlen(answer) + 1));
		}


		// print header and name info
		
		int type = ntohs(currentHeader->aType);
		DWORD* ip = (DWORD*)(currentHeader + 1);
		struct in_addr pAddress;
		pAddress.S_un.S_addr = *ip;
		char* address = inet_ntoa(pAddress);
		
		//printf("Type: %d\n", ntohs(currentHeader->TTL));

		// get type and set string for that
		string typeS;
		int addrLength = 4;
		switch (type) {
		case 1:
			typeS = "A"; break;
		case 2:
			typeS = "NS"; break;
		case 5:
			typeS = "CNAME"; break;
		case 12:
			typeS = "PTR"; break;
		default:
			break;
		}
		printf("  \t%s type %s %s TTL = %d\n", ansSections[i], typeS.c_str(), address, ntohl(currentHeader->TTL));
		
		answer += 2 + sizeof(DNSAnswerHeader) + addrLength;
	}

	/*for (int i = 0; i < numAnswers; i++) {
		DNSAnswerHeader* currentHeader = (DNSAnswerHeader*)(ansSections[i] + strlen(ansSections[i]) + 1);



		int type = ntohs(currentHeader->aType);
		if (type == 1) {
			printf("  \t%s type A %s TTL = %d\n", ansSections[i], "", ntohl(currentHeader->TTL));
		}
		else if (type == 12) {
			printf("  \t%s type PTR %s TTL = %d\n", ansSections[i], "", ntohl(currentHeader->TTL));
		}
	}*/


	if (numAuthority > 0) {
		if (ansSections[numAnswers - 1] == NULL) {
			printf("Answer sections missing!\n");
			quit();
		}

		// parse Authority
		printf("  ------------ [authority] ----------\n");
		char* authSection = ansSections[numAnswers - 1] + strlen(ansSections[numAnswers - 1]) + 5;
		char* authority = authSection;
		for (int i = 0; i < numAuthority; i++) {
			int length = strlen(authority);

			if ((authority[0] & 0xC0) == 0xC0) { // compression detected. Jump!
				int offset = ntohs(*(USHORT*)authority) & 0x3FFF;
				char* label = (*recvBuf) + offset + 1;

				int labelLength = strlen(label);
				length = labelLength + 2;
				authSections[i] = label;
				//printf("Jumped to %X\n", label);
			}
			else {
				authSections[i] = authority + 1;

				if (length == 0) {
					printf("     ++ Error: malformed packet\n");
					delete[] qSections;
					delete[] ansSections;
					delete[] authSections;
					delete[] addSections;
					quit();
				}

				// convert char lengths to dots
				for (u_int j = 0; j < length - 1; j++) {
					if (authority[j] < '0') {
						authority[j] = '.';
					}
				}

				currentHeader = ((DNSAnswerHeader*)(authority + strlen(authority) + 1));
			}


			// print header and name info
			DNSAnswerHeader* currentHeader = (DNSAnswerHeader*)(authority + 2);
			int type = ntohs(currentHeader->aType);
			DWORD* ip = (DWORD*)(currentHeader + 1);
			struct in_addr pAddress;
			pAddress.S_un.S_addr = *ip;
			char* address = inet_ntoa(pAddress);

			// get type and set string for that
			string typeS;
			switch (type) {
			case 1:
				typeS = "A"; break;
			case 2:
				typeS = "NS"; break;
			case 5:
				typeS = "CNAME"; break;
			case 12:
				typeS = "PTR"; break;
			default:
				break;
			}
			printf("  \t%s type %s %s TTL = %d\n", authSections[i], typeS.c_str(), address, ntohl(currentHeader->TTL));

			//printf("Size: %d\n", 2 + sizeof(DNSAnswerHeader) + 4);
			authority += 2 + sizeof(DNSAnswerHeader) + 4;
		}
	}




	if (numAdditional > 0) {
		if (authSections[numAuthority - 1] == NULL) {
			printf("Answer sections missing!\n");
			delete[] qSections;
			delete[] ansSections;
			delete[] authSections;
			delete[] addSections;
			quit();
		}

		// parse Additional
		printf("  ------------ [additional] ----------\n");
		char* addSection;
		if (numAuthority <= 0) {
			addSection = ansSections[numAnswers - 1] + strlen(ansSections[numAnswers - 1]) + sizeof(DNSAnswerHeader) + 5;
		}
		else {
			addSection = authSections[numAuthority - 1] + strlen(authSections[numAuthority - 1]) + 5;
		}
		
		char* additional = addSection;
		for (int i = 0; i < numAdditional; i++) {
			int length = strlen(additional);

			if ((additional[0] & 0xC0) == 0xC0) { // compression detected. Jump!
				int offset = ntohs(*(USHORT*)additional) & 0x3FFF;
				char* label = (*recvBuf) + offset + 1;

				int labelLength = strlen(label);
				length = labelLength + 2;
				addSections[i] = label;
				//printf("Jumped to %X\n", label);
			}
			else {
				addSections[i] = additional + 1;

				if (length == 0) {
					printf("     ++ Error: malformed packet\n");
					quit();
				}

				//convert char lengths to dots
				for (u_int j = 0; j < length - 1; j++) {
					if (additional[j] < '0') {
						additional[j] = '.';
					}
				}

				currentHeader = ((DNSAnswerHeader*)(additional + strlen(additional) + 1));
			}


			// print header and name info
			DNSAnswerHeader* currentHeader = (DNSAnswerHeader*)(additional + 2);
			int type = ntohs(currentHeader->aType);
			DWORD* ip = (DWORD*)(currentHeader + 1);
			struct in_addr pAddress;
			pAddress.S_un.S_addr = *ip;
			char* address = inet_ntoa(pAddress);

			// get type and set string for that
			string typeS;
			switch (type) {
			case 1:
				typeS = "A"; break;
			case 2:
				typeS = "NS"; break;
			case 5:
				typeS = "CNAME"; break;
			case 12:
				typeS = "PTR"; break;
			default:
				break;
			}
			printf("  \t%s type %s %s TTL = %d\n", addSections[i], typeS.c_str(), address, ntohl(currentHeader->TTL));

			//printf("Size: %d\n", 2 + sizeof(DNSAnswerHeader) + 4);
			additional += 2 + sizeof(DNSAnswerHeader) + 4;
		}
	}

	




	
	delete[] qSections;
	delete[] ansSections;
	delete[] authSections;
	delete[] addSections;
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