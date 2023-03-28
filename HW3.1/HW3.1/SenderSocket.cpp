/* SenderSocket.cpp
 * Joseph Shumway
 * CSCE 463
 * Spring 2023
 */

#include "pch.h"

int SenderSocket::open() {
	int counter = 0;
	int maxAttempts = 3;
	int timeSYNSent = 0;

	// bind port and get IP lookup for server
	lookupDNS();

	int attempt = 0;
	while (attempt++ < MAX_ATTEMPTS) {
		
		timeval timeout;
		timeout.tv_sec = TIMEOUT;
		timeout.tv_usec = 0;

		timeSYNSent = clock();

		printf("[%g] --> SYN %d (attempt %d of %d, RTO %.3f) to %s\n",
			(double)(clock() - timeStarted) / 1000,
			counter,
			attempt,
			maxAttempts,
			RTO,
			targetHost
		);
		if (sendto(sock, (char*)&syn, sizeof(SenderSynHeader), 0, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
			//printf("\n\t+ Socket error: %d\n", WSAGetLastError());
			return FAILED_SEND;
		}

		fd_set fd;
		FD_ZERO(&fd);
		FD_SET(sock, &fd);
		int available = select(0, &fd, NULL, NULL, &timeout);

		// response ready
		if (available > 0) {
			break;
		}
		else { // timeout
			printf("timeout in %d ms\n", clock() - timeStarted);
			continue;
		}
	}


	if (attempt > MAX_ATTEMPTS) {
		return TIMEOUT;
	}

	//printf("Successfully sent packet!\n");
	struct sockaddr_in response;
	int responseSize = sizeof(response);

	//printf("Receiving...\n");
	recvBytes = recvfrom(sock, (char*)&responseHeader, sizeof(ReceiverHeader), 0, (struct sockaddr*)&response, &responseSize);
	if (recvBytes == SOCKET_ERROR) {
		//printf("Receive error: %d\n", WSAGetLastError());
		return FAILED_RECV;
	}

	if (response.sin_addr.S_un.S_addr != server.sin_addr.S_un.S_addr || response.sin_port != server.sin_port) {
		printf("Response port or server mismatch!\n");
	}



	if (responseHeader.flags.ACK && responseHeader.flags.SYN) {

		RTO = (float) ((clock() - timeSYNSent) / 1000) * 3;
		//recvBytes = responseHeader.recvWnd
		printf("[%g] <-- SYN-ACK %d window %d; setting initial RTO to %g\n",
			(double)(clock() - timeStarted) / 1000,
			responseHeader.ackSeq,
			responseHeader.recvWnd,
			RTO
		);
	}
	

	return STATUS_OK;
}

int SenderSocket::send(char* ptr, int numBytes) {
	return 0;
}

int SenderSocket::close() {
	return 0;
}

int SenderSocket::lookupDNS() {

	WSADATA wsaData;

	//Initialize WinSock; once per program run
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

	// structure used in DNS lookups
	struct hostent* remote;

	// first assume that the string is an IP address
	DWORD IP = inet_addr(targetHost);



	if (IP == INADDR_NONE) {

		printf("\tDoing DNS... ");
		clock_t time_ms = clock();

		// if not a valid IP, then do a DNS lookup
		if ((remote = gethostbyname(targetHost)) == NULL) {
			//printf("failed with %d\n", WSAGetLastError());
			return INVALID_NAME;
		}
		else {// take the first IP address and copy into sin_addr
			memcpy((char*)&(server.sin_addr), remote->h_addr, remote->h_length);
		}
	}
	else {
		// if valid IP, set server IP as this IP
		server.sin_addr.S_un.S_addr = IP;
	}

	// setup the port # and protocol type
	server.sin_family = AF_INET;
	server.sin_port = htons(MAGIC_PORT);

	return STATUS_OK;
}

void SenderSocket::quit() {
	WSACleanup();
	delete[] dwordBuf;
	exit(-1);
}