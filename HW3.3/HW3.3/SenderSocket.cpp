/* SenderSocket.cpp
 * Joseph Shumway
 * CSCE 463
 * Spring 2023
 */

#include "pch.h"


int SenderSocket::open() {
	if (connectionOpen) {
		return ALREADY_CONNECTED;
	}
	
	int counter = 0;
	int maxAttempts = 3;
	int timeSYNSent = 0;

	// bind port and get IP lookup for server
	lookupDNS();

	int attempt = 0;
	while (attempt++ < MAX_ATTEMPTS_SYN) {
		
		timeval timeout;
		timeout.tv_sec = secondsFromFloat(RTO);
		timeout.tv_usec = microSecondsFromFloat(RTO);

		timeSYNSent = clock();

		//printf("[%g] --> SYN %d (attempt %d of %d, RTO %.3f) to %s\n",(double)(clock() - timeStarted) / 1000, counter, attempt, maxAttempts, RTO, ipString);
		if (sendto(sock, (char*)&syn, sizeof(SenderSynHeader), 0, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
			printf("[%g] --> failed sendto with %d\n",
				(double)(clock() - timeStarted) / 1000,
				WSAGetLastError()
			);
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
			//printf("timeout in %d ms\n", clock() - timeStarted);
			continue;
		}
	}


	if (attempt > MAX_ATTEMPTS_SYN) {
		return TIMEOUT;
	}

	//printf("Successfully sent packet!\n");
	struct sockaddr_in response;
	int responseSize = sizeof(response);

	//printf("Receiving...\n");
	int received = recvfrom(sock, (char*)&synAck, sizeof(ReceiverHeader), 0, (struct sockaddr*)&response, &responseSize);
	if (received == SOCKET_ERROR) {
		printf("[%g] <-- failed recvfrom with %d\n", (double)(clock() - timeStarted) / 1000, WSAGetLastError());
		return FAILED_RECV;
	}
	else {
		recvBytes = received;
	}

	if (response.sin_addr.S_un.S_addr != server.sin_addr.S_un.S_addr || response.sin_port != server.sin_port) {
		printf("Response port or server mismatch!\n");
	}



	if (synAck.flags.ACK && synAck.flags.SYN) {
		
		RTO =  (((float)(clock() - timeSYNSent)) / 1000) * 3;
		estRTT = RTO / 3;
		devRTT = estRTT / 4;

		int lastReleased = min(W, synAck.recvWnd);
		ReleaseSemaphore(empty, lastReleased, NULL);
		//printf("[%g] <-- SYN-ACK %d window %d; setting initial RTO to %g\n", (double)(clock() - timeStarted) / 1000, responseHeader.ackSeq, responseHeader.recvWnd, RTO);
	}
	
	connectionOpen = true;

	return STATUS_OK;
}

int SenderSocket::close() {

	if (!connectionOpen) {
		return NOT_CONNECTED;
	}

	finalWindow = synAck.recvWnd;

	int timeFINSent = 0;
	int attempt = 0;

	// set fin and syn
	syn.sdh.flags.FIN = 1;
	syn.sdh.flags.SYN = 0;
	syn.sdh.seq = nextSeq;

	while (attempt++ < MAX_ATTEMPTS) {

		timeval timeout;
		timeout.tv_sec = secondsFromFloat(RTO);
		timeout.tv_usec = microSecondsFromFloat(RTO);

		timeFINSent = clock();

		if (sendto(sock, (char*)&syn, sizeof(SenderSynHeader), 0, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
			printf("[%g] --> failed sendto with %d\n",
				(double)(clock() - timeStarted) / 1000,
				WSAGetLastError()
			);
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
	int received = recvfrom(sock, (char*)&synAck, sizeof(ReceiverHeader), 0, (struct sockaddr*)&response, &responseSize);
	if (received == SOCKET_ERROR) {
		printf("[%g] <-- failed recvfrom with %d\n",
			(double)(clock() - timeStarted) / 1000,
			WSAGetLastError()
		);
		return FAILED_RECV;
	}
	else {
		recvBytes = received;
	}

	if (response.sin_addr.S_un.S_addr != server.sin_addr.S_un.S_addr || response.sin_port != server.sin_port) {
		printf("Response port or server mismatch!\n");
	}



	if (synAck.flags.ACK && synAck.flags.FIN) {

		RTO = (float)((clock() - timeFINSent) / 1000) * 3;
		//recvBytes = responseHeader.recvWnd
		printf("[%g] <-- FIN-ACK %d window %08X\n",
			(double)(clock() - timeStarted) / 1000,
			synAck.ackSeq,
			synAck.recvWnd
		);
	}


	return STATUS_OK;
}



int SenderSocket::send(char* data, int numBytes) {

	if (!connectionOpen) {
		return NOT_CONNECTED;
	}
	
	// create event array and wait till window has empty slots
	HANDLE arr[] = { eventQuit, empty };
	if (DEBUG) { printf("    [Send()] Waiting for empty slot (%d slots empty)\n", numEmpty); }
	WaitForMultipleObjects(2, arr, false, INFINITE);
	InterlockedDecrement(&numEmpty);

	// get # for next slot in window
	int slot = nextSeq % W; // TODO check this again later (might need +1)
	

	// add packet to pendingPackets
	Packet* packet = pendingPackets + slot;

	// setup packet header
	SenderDataHeader sdh;
	sdh.flags.reserved = 0;
	sdh.flags.SYN = 0;
	sdh.flags.FIN = 0;
	sdh.flags.ACK = 0;
	sdh.seq = nextSeq;

	// copy data and header to packet
	memcpy(packet->pkt + sizeof(SenderDataHeader), data, numBytes);
	memcpy(packet->pkt, &sdh, sizeof(SenderDataHeader));

	// add numBytes to array for use later
	numBytesBySlot[slot] = numBytes;												// Potential problem here

	// tell 'full' that a packet was added to the window
	ReleaseSemaphore(full, 1, NULL);
	//printf("[Send] Added packet to slot %d\n", slot);
	if (DEBUG) { printf("    [Send()] Adding packet to queue with seq=%d, numBytes=%d, slot=%d\n", nextSeq, numBytes, slot); }

	return STATUS_OK;
}



UINT SenderSocket::statThread() {

	WaitForSingleObject(finished, INFINITY);
	int time_s = 0;
	int prevTime_s = 0;

	while (!quitStats) {

		Sleep(TIME_INTERVAL);

		time_s = (clock() - timeStarted) / 1000;

		EnterCriticalSection(&criticalSection);

		// get rate, then divide by 125,000 to convert from bytes per sec to megabits per sec
		downloadRate = (float)(sentBytes - prevSentBytes) / (float)(time_s - prevTime_s) / 125000;
		downloadRateList.push_back(downloadRate);
		prevSentBytes = sentBytes;
		totalSentBytes += sentBytes;

		

		printf("[%2d] B %6d (%3.1f MB) N %6d T %d F %d W %d S %.3f Mbps RTT %.3f\n",
			(int)(clock() - timeStarted) / 1000,
			nextSeq,
			(float) totalSentBytes / 8000000,
			nextSeq + 1,
			numTimeouts,
			numFastRet,
			W,  // TODO change to min(senderWindow, receiverWindow) aka effective window size
			downloadRate,
			obsRTT
		);

		prevTime_s = time_s;

		LeaveCriticalSection(&criticalSection);
	}

	ReleaseSemaphore(finished, 1, NULL);
	return 0;
}

UINT SenderSocket::workerThread() {

	WaitForSingleObject(finished, INFINITY);

	while (!statsReady) {
		Sleep(500); // wait for stat thread to be ready first
	}

	int status = 0;
	int timeout = 0;
	int timerExpire = clock() + RTO;
	HANDLE events[] = { socketReceiveReady, full };
	ULONG bytesAvailable = 0;
	int result = 0;
	nextToSend = 0;

	enum EventCases {
		EVENT_SOCKET_READY = WAIT_OBJECT_0,
		EVENT_PACKET_READY = WAIT_OBJECT_0 + 1
	};

	while (true) {

		if (doneSending) {
			break;
		}

		// if there are packets in the array
		if (nextToSend > senderBase) {
			timeout = (1000 * (pendingPackets[senderBase % W].txTime - clock()) / (CLOCKS_PER_SEC) + (long)(1000 * RTO));
			//timeout = TIME_INTERVAL;
		} else {
			timeout = INFINITE;
		}

		

		//printf("\t[Worker] Waiting...\n");
		int ret = WaitForMultipleObjects(2, events, false, timeout);

		switch (ret) {
			case WAIT_TIMEOUT:	// timeout, retransmit
				if (DEBUG) { printf("\t[Worker] Timeout! Retransmit packet with seq=%d\n", (int)nextToSend); }
				sendPacket(nextToSend);
				numRetransmissions++;
				pendingPackets[senderBase % W].txTime = clock();
				break;


			case EVENT_SOCKET_READY: // socket is ready with packet
				
				// double check that there is ACTUALLY data to receive
				bytesAvailable = 0;
				result = ioctlsocket(sock, FIONREAD, &bytesAvailable);
				if (result == SOCKET_ERROR || bytesAvailable <= 0) {
					continue;
				}
				else {
					if (DEBUG) { printf("\t[Worker] ACK ready! Receiving...\n"); }
					if (receiveACK() == STATUS_OK) {
						
						if (DEBUG) { printf("\t[Worker] ACK successfully received\n"); }
						bool senderBaseMoved = false;
						
						int released = 0;

						if (ackSeq >= senderBase && ackSeq < senderBase + W) {
							released = ackSeq - senderBase;
							senderBase = ackSeq;
							senderBaseMoved = true;
						}

						if (senderBase != nextToSend) {
							pendingPackets[senderBase % W].txTime = clock();
						}

						// handle fast retransmit (3 duplicate ACKs: retransmit)
						if (ackSeq == lastAckSeq) {
							duplicateACKCount++;

							if (duplicateACKCount == 3) {
								if (DEBUG) { printf("\t[Worker] Fast Retransmit: Retransmitting packet with seq=%d\n", (int)senderBase); }
								sendPacket(senderBase); // Retransmit the packet
								duplicateACKCount = 0; // Reset the duplicate ACK counter
								pendingPackets[senderBase % W].txTime = clock();
							}
						}
						else {
							duplicateACKCount = 0;
						}

						if (senderBaseMoved) {
							if (DEBUG) { printf("\t[Worker] Sender base updated to %d\n\n", (int) senderBase); }
							pendingPackets[senderBase % W].txTime = clock();
							nextToSend = senderBase;
						}

						effectiveWindow = min(W, responseAck.recvWnd);
						
						lastReleased += released;

						ReleaseSemaphore(empty, released, NULL);
						InterlockedAdd(&numEmpty, released);
						if (DEBUG) { printf("\t[Worker] Sent packet successfully (%d slots empty, %d released)\n", numEmpty, released); }
					}
					break;
				}

			case EVENT_PACKET_READY: // send packet
				if (DEBUG) { printf("\t[Worker] Packet ready! Sending packet with seq=%d\n", (int)nextToSend); }
				if (sendPacket(nextToSend) == STATUS_OK) {
					pendingPackets[senderBase % W].txTime = clock();
					nextToSend++;
				}
				break;


			default:
				break;
		}
	}

	ReleaseSemaphore(finished, 1, NULL);

	return 0;

}



int SenderSocket::sendPacket(UINT64 seqToSend) {
	int counter = 0;
	int maxAttempts = 3;
	int timeSYNSent = 0;
	int attempt = 0;
	int beginRoundTrip = 0;
	int slot = seqToSend % W; // TODO check this again later (might need +1)

	// send packet until max_attempts
	while (attempt++ < MAX_ATTEMPTS) {

		timeval timeout;
		timeout.tv_sec = secondsFromFloat(RTO);
		timeout.tv_usec = microSecondsFromFloat(RTO);

		// add packet to pendingPackets
		Packet* packet = pendingPackets + slot;

		// attempt send packet
		beginRoundTrip = clock();
		if (sendto(sock, packet->pkt, sizeof(SenderDataHeader) + numBytesBySlot[slot], 0, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
			return FAILED_SEND;
		}

		fd_set fd;
		FD_ZERO(&fd);
		FD_SET(sock, &fd);
		int available = select(0, &fd, NULL, NULL, &timeout);

		if (available > 0) {	// response ready
			break;
		}
		else {					// timeout
			continue;
		}
	}
	obsRTT = (float)(clock() - beginRoundTrip) / 1000;
	sentBytes += numBytesBySlot[slot];

	if (attempt > MAX_ATTEMPTS) {
		numTimeouts++;
		return TIMEOUT;
	}

	// packet sent! :)
	return STATUS_OK;
}

int SenderSocket::receiveACK() {
	struct sockaddr_in response;
	int responseSize = sizeof(response);

	int received = recvfrom(sock, (char*)&responseAck, sizeof(ReceiverHeader), 0, (struct sockaddr*)&response, &responseSize);
	if (received == SOCKET_ERROR) {
		return FAILED_RECV;
	}
	else {
		recvBytes = received;
	}

	if (response.sin_addr.S_un.S_addr != server.sin_addr.S_un.S_addr || response.sin_port != server.sin_port) {
		printf("Response port or server mismatch!\n");
	}

	if (DEBUG) { printf("\t[Worker] Received ACK with seq=%d\n", ackSeq); }

	// double check ACK sequence is correct
	if (responseAck.flags.ACK) {
		ackSeq = responseAck.ackSeq;
		RTO = calcRTO(estRTT, obsRTT, devRTT);
	}
	else {
		// ACK = false
		return -1;
	}	

	return STATUS_OK;
}
















void SenderSocket::startThreads() {

	// create and start the crawler threads
	worker = CreateThread(NULL, 0, [](LPVOID lpParam) -> DWORD {
		SenderSocket* p = (SenderSocket*)lpParam;
		return p->workerThread();
		}, this, 0, NULL);

	if (worker == NULL) {
		printf("Failed to create worker thread\n");
	}

	// start the stat thread
	stat = CreateThread(NULL, 0, [](LPVOID lpParam) -> DWORD {
		SenderSocket* p = (SenderSocket*)lpParam;
		return p->statThread();
		}, this, 0, NULL);

	if (stat == NULL) {
		printf("Failed to create stat thread\n");
	}

	statsReady = true;

	//startTime = clock();

}

void SenderSocket::waitForThreads() {

	// wait for threads to finish and close their handles
	// WaitForMultipleObjects(numThreads, threads, TRUE, INFINITE);

	if (worker != NULL) {
		WaitForSingleObject(worker, INFINITE);
		CloseHandle(worker);
	}
	else {
		printf("Error: the handle for worker is null\n");
	}

	quitStats = true;

	// close stat thread
	WaitForSingleObject(stat, INFINITE);

	if (stat != NULL) {
		CloseHandle(stat);
	}
	else {
		printf("Error: the handle for stat thread is null\n");
	}

	// close finished semaphore
	CloseHandle(finished);

	DeleteCriticalSection(&criticalSection);

}

void SenderSocket::printFinalStats() {

	//int totalTimeSec = (clock() - startTime) / 1000;

	// TODO printf()
}

float SenderSocket::calcRTO(float& estRTT, float sampleRTT, float& devRTT) {
	const float alpha = 0.125;
	const float beta = 0.25;
	
	estRTT = (1 - alpha) * estRTT + alpha * sampleRTT;

	double diff = abs(sampleRTT - estRTT);
	devRTT = (1 - beta) * devRTT + beta * diff;

	return estRTT + (4 * max(devRTT, 0.010));
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

		clock_t time_ms = clock();

		// if not a valid IP, then do a DNS lookup
		if ((remote = gethostbyname(targetHost)) == NULL) {
			printf("[%g] --> target %s is invalid\n",
				(double)(clock() - timeStarted) / 1000,
				targetHost
			);
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

	memcpy(ipString, inet_ntoa(server.sin_addr), 15);

	// setup the port # and protocol type
	server.sin_family = AF_INET;
	server.sin_port = htons(MAGIC_PORT);

	// setup socketReceiveReady event
	int result = WSAEventSelect(sock, socketReceiveReady, FD_READ);
	if (result == SOCKET_ERROR) {
		printf("Socket Error %d\n", WSAGetLastError());
		this->~SenderSocket();
		exit(0);
	}

	// fix SOCK issues from default values for performance
	int kernelBuffer = 20e6; // 20 meg
	if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&kernelBuffer, sizeof(int)) == SOCKET_ERROR) {
		printf("Socket Error %d\n", WSAGetLastError());
		this->~SenderSocket();
		exit(0);
	}
	kernelBuffer = 20e6; // 20 meg
	if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&kernelBuffer, sizeof(int)) == SOCKET_ERROR) {
		printf("Socket Error %d\n", WSAGetLastError());
		this->~SenderSocket();
		exit(0);
	}
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	return STATUS_OK;
}

void SenderSocket::quit() {
	WSACleanup();
	delete[] dwordBuf;
	exit(-1);
}

int SenderSocket::secondsFromFloat(double time) {
	return (int)time;
}

int SenderSocket::microSecondsFromFloat(double time) {
	double integral;
	return (int)(modf(time, &integral) * 1000 * 1000);
}