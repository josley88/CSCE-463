/* URLGrabber.h
 * Joseph Shumway
 * CSCE 463
 * Spring 2023
 */

#pragma once
#include "HTMLParserBase.h"

#define INITIAL_BUF_SIZE 2048
#define THRESHOLD 2048

class URLProcessor {

	char host[MAX_HOST_LEN] = { 0 };
	char path  [MAX_HOST_LEN] = { 0 };
	char query [MAX_HOST_LEN] = { 0 };
	char* buf;
	char* headerBuf;
	char* bodyBuf;
	char** URLs;

	int urlIndex;
	int headerSize;
	int bodySize;

	int port;
	int allocatedSize;
	int curPos;
	
	// open a TCP socket
	SOCKET sock;
	struct sockaddr_in server;

	clock_t time_ms;

	public:

		URLProcessor(char*** _URLs) {

			URLs = *_URLs;
			urlIndex = 0;
			buf = new char[INITIAL_BUF_SIZE];
			allocatedSize = INITIAL_BUF_SIZE;
		}

		bool parseURL();
		bool lookupDNS();
		bool connectToSite();
		bool loadPage(bool getHEAD);
		bool verifyHeader(char* _status, char expectedCode);
		bool separateHeader();
		bool parseHTML();
		void printHeader();
		void printBody();
		void nextURL();

		void cleanQuit(char* allocatedBuffer);
};