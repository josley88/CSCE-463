/* SenderSocket.cpp
 * Joseph Shumway
 * CSCE 463
 * Spring 2023
 */

#include "pch.h"

int SenderSocket::open(char* host, int port, int windowSize, int startTime) {
	int counter = 0;
	int attempt = 1;
	int maxAttempts = 3;
	double retransmissionTimeout = 1;

	printf("[%f] --> SYN %d (attempt %d of %d, RTO %.3f) to %s", (clock() - startTime) / 1000, counter, attempt, 3, retransmissionTimeout, host);

	return 0;
}

int SenderSocket::send(char* ptr, int numBytes) {
	return 0;
}

int SenderSocket::close() {
	return 0;
}
