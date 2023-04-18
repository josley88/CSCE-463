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



	SenderSocket ss(argv);
	int status;

	

	ss.timeStarted = clock();

	// open connection
	if ((status = ss.open()) != STATUS_OK) {
		// handle errors
		printf("Main: connect failed with status %d\n", status);
		ss.~SenderSocket();
		WSACleanup();
		exit(0);
	}
	else {
		
		printf("Main: connected to %s in %g sec, pkt size %d bytes\n", ss.targetHost, (float)(clock() - ss.timeStarted) / 1000, MAX_PKT_SIZE);
		
		char* charBuf = (char*)ss.dwordBuf;
		UINT64 byteBufferSize = ss.dwordBufSize << 2; // convert to bytes

		UINT64 cursor = 0; // buffer position

		int timeStartTransfer = clock();
		while (cursor < byteBufferSize) {

			// get size of next chunk
			int recvBytes = min(byteBufferSize - cursor, MAX_PKT_SIZE - sizeof(SenderDataHeader));

			// send chunk into socket
			if ((status = ss.send(charBuf + cursor, recvBytes)) != STATUS_OK) {
				// handle errors
				printf("Main: send failed with status %d\n", status);
				ss.~SenderSocket();
				WSACleanup();
				exit(0);
			}

			cursor += recvBytes;
		}

		//Sleep(7);

		int transferTime = clock() - timeStartTransfer;

		if ((status = ss.close()) != STATUS_OK) {
			// handle errors
			printf("Main: close failed with status %d\n", status);
			ss.~SenderSocket();
			WSACleanup();
			exit(0);
		}

		printf("Main: transfer finished in %g sec", (float) transferTime / 1000);


		//printf("\n\n");
	}
}

