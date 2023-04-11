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
		
		printf("Main: connected to %s in %g sec, pkt size %d bytes\n", ss.targetHost, (float)(clock() - ss.timeStarted) / 1000, MAX_PKT_SIZE);

		int timeStartTransfer = clock();

		// start threads and wait
		ss.startThreads();
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
		for (auto x : ss.downloadRateList) {
			total += x;
		}
		total *= 1000;
		total /= ss.downloadRateList.size();

		Checksum cs;
		DWORD checksum = cs.CRC32((unsigned char*) ss.charBuf, ss.byteBufferSize);

		printf("Main: transfer finished in %g sec, %.2f Kbps, checksum %8X\n", (float) transferTime / 1000, total, checksum);




		printf("Main: estRTT %.3f, ideal rate %.2f Kbps\n", ss.estRTT, ss.senderWindow * MAX_PKT_SIZE / ss.estRTT / 100);
	}
}

