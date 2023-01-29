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

	char host[MAX_HOST_LEN];
	char path  [MAX_HOST_LEN];
	char query [MAX_HOST_LEN];
	char* URL;
	char* buf;
	char* headerBuf;
	

	int port;
	int allocatedSize;
	int curPos;
	
	// open a TCP socket
	SOCKET sock;
	struct sockaddr_in server;

	clock_t time_ms;

	public:

		URLGrabber(char* _URL) {
			URL = _URL;
			buf = new char[INITIAL_BUF_SIZE];
		}

		bool parseURL();
		bool lookupDNS();
		bool connectToSite();
		bool loadPage();
		void verifyHeader(char* _status);
		void separateHeader();
		bool parseHTML();
		void printHeader();
		void printBody();
		

		~URLGrabber() {
			delete headerBuf;
		}
};