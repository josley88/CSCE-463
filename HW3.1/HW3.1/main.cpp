/* main.cpp
 * Joseph Shumway
 * CSCE 463
 * Spring 2023
 */

#include "pch.h"

int main(int argc, char** argv) {
	
	if (argc < 8) {
		printf("Not enough arguments given!\n");
		WSACleanup();
		exit(0);
	}
	
	// initialize vars
	char* targetHost = argv[1];
	int bufferSizePower = atoi(argv[2]);
	int senderWindow = atoi(argv[3]);
	double rtt = atof(argv[4]);
	double forwardLossRate = atof(argv[5]);
	double returnLossRate = atof(argv[6]);
	int bottleneckSpeed = atoi(argv[7]);

	UINT64 dwordBufSize = (UINT64)1 << bufferSizePower;
	DWORD* dwordBuf = new DWORD[dwordBufSize];

	int timeStarted = clock();




	// open socket
	WSADATA wsaData;

	// Initialize WinSock
	WORD wVersionRequested = MAKEWORD(2, 2);
	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		printf("WSAStartup error %d\n", WSAGetLastError());
		quit(dwordBuf);
	}

	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) {
		printf("socket() generated error %d\n", WSAGetLastError());
		quit(dwordBuf);
	}

	struct sockaddr_in local;
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons(0);
	if (bind(sock, (struct sockaddr*)&local, sizeof(local)) == SOCKET_ERROR) {
		printf("Binding port error: %d\n", WSAGetLastError());
		quit(dwordBuf);
	}




	// initialize buffer
	printf("Main: initializing DWORD array with 2^20 elements... ");
	for (UINT64 i = 0; i < dwordBufSize; i++) {
		dwordBuf[i] = i;
	}
	printf("done in %d ms\n", clock() - timeStarted);




	SenderSocket ss;
	int status;
	timeStarted = clock();
	if ((status = ss.open(targetHost, MAGIC_PORT, senderWindow, timeStarted)) != STATUS_OK) {
		// handle errors
	}

	char* charBuf = (char*)dwordBuf;
	UINT64 byteBufferSize = dwordBufSize << 2; // convert to bytes

	UINT64 cursor = 0; // buffer position
	while (cursor < byteBufferSize) {

		// get size of next chunk
		int bytes = min(byteBufferSize - cursor, MAX_PKT_SIZE - sizeof(SenderDataHeader));

		// send chunk into socket
		if ((status = ss.send(charBuf + cursor, bytes)) != STATUS_OK) {
			// handle errors
		}

		cursor += bytes;
	}

	if ((status = ss.close()) != STATUS_OK) {
		//handle errors
	}




	printf("\n\n");
	delete[] dwordBuf;


}

void quit(DWORD* dwordBuf) {
	WSACleanup();
	delete[] dwordBuf;
	exit(-1);
}