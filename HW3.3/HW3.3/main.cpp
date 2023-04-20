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

	int sockStatus = 0;

	SenderSocket ss(argv);
	ss.timeStarted = clock();

	// open connection
	if ((sockStatus = ss.open()) != STATUS_OK) {
		printf("Main: connect failed with status %d\n", sockStatus);
		ss.~SenderSocket();
		WSACleanup();
		exit(0);
	}
	else {
		
		printf("[Main] connected to %s in %g sec, pkt size %d bytes\n", ss.targetHost, (float)(clock() - ss.timeStarted) / 1000, MAX_PKT_SIZE);

		int timeStartTransfer = clock();

		// start stat and worker thread
		ss.startThreads();

		int status = 0;
		// populate pending packet queue
		while (true) {

			// quit if the end of the buffer is reached
			if (ss.cursor >= ss.byteBufferSize) {
				//if (DEBUG) { printf("====================== DONE SENDING ======================\n"); }
				ss.doneSending = true;
				break;
			}

			// get size of next chunk and move the cursor forward for the next thread to use
			int recvBytes = min(ss.byteBufferSize - ss.cursor, MAX_PKT_SIZE - sizeof(SenderDataHeader));

			// send() chunk to producer window array
			//if (DEBUG) { printf("Main: Sending packet with seq=%d, numBytes=%d\n", (int) ss.nextSeq, recvBytes); }
			if ((status = ss.send(ss.charBuf + ss.cursor, recvBytes)) != STATUS_OK) {
				// handle errors
				printf("Main: send failed with status %d\n", status);
				break;
			}

			ss.cursor += recvBytes;
			ss.nextSeq++;
			//if (DEBUG) { printf("Main: Updated cursor position: %d\n", ss.cursor); }
		}


		ss.waitForThreads();

		int transferTime = clock() - timeStartTransfer;

		if ((sockStatus = ss.close()) != STATUS_OK) {
			// handle errors
			printf("Main: close failed with status %d\n", sockStatus);
			ss.~SenderSocket();
			WSACleanup();
			exit(0);
		}

		float total = 0;
		for (auto x : ss.downloadRateList)
			total += x;
		total *= 1000;
		total /= ss.downloadRateList.size();

		Checksum cs;
		DWORD checksum = cs.CRC32((unsigned char*) ss.charBuf, ss.byteBufferSize);

		printf("Main: transfer finished in %g sec, %.2f Kbps, checksum %8X\n", (float) transferTime / 1000, total, checksum);
		printf("Main: estRTT %.3f, ideal rate %.2f Kbps\n", ss.estRTT, ss.W * MAX_PKT_SIZE / ss.estRTT / 100);
	}
}