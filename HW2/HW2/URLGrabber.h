/* URLGrabber.h
 * Joseph Shumway
 * CSCE 463
 * Spring 2023
 */

#pragma once
#include "HTMLParserBase.h"

#define INITIAL_BUF_SIZE 2048
#define THRESHOLD 2048

class URLGrabber {

	char host[MAX_HOST_LEN] = { 0 };
	char path  [MAX_HOST_LEN] = { 0 };
	char query [MAX_HOST_LEN] = { 0 };
	char* buf;
	char** URLs;
	int urlIndex;
	

	int port;
	int allocatedSize;
	int curPos;
	
	// open a TCP socket
	SOCKET sock;
	struct sockaddr_in server;

	clock_t time_ms;

	public:

		URLGrabber(char** _URLs) {

			URLs = _URLs;
			urlIndex = 0;
			buf = new char[INITIAL_BUF_SIZE];
			allocatedSize = 0;
		}

		bool parseURL();
		bool lookupDNS();
		bool connectToSite();
		bool loadPage();
		void verifyHeader(char* _status);
		bool parseHTML();
		void printHeader();
		void nextURL();

		~URLGrabber() {
			delete buf;
		}
};