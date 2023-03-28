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
	}
	else {

		printf("Main: connected to %s in %g sec, pkt size %d bytes\n", ss.targetHost, (float)(clock() - ss.timeStarted) / 1000, ss.recvBytes);

		char* charBuf = (char*)ss.dwordBuf;
		UINT64 byteBufferSize = ss.dwordBufSize << 2; // convert to bytes

		UINT64 cursor = 0; // buffer position
		while (cursor < byteBufferSize) {

			// get size of next chunk
			int recvBytes = min(byteBufferSize - cursor, MAX_PKT_SIZE - sizeof(SenderDataHeader));

			// send chunk into socket
			if ((status = ss.send(charBuf + cursor, recvBytes)) != STATUS_OK) {
				// handle errors
			}

			cursor += recvBytes;
		}

		if ((status = ss.close()) != STATUS_OK) {
			//handle errors
		}




		printf("\n\n");
	}
}

