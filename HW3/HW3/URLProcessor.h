/* URLProcessor.h
 * Joseph Shumway
 * CSCE 463
 * Spring 2023
 */

#pragma once
#include "HTMLParserBase.h"
#include <set>
#include <string>

#define INITIAL_BUF_SIZE 2048
#define THRESHOLD 2048

using std::set;
using std::string;

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
	int maxDownloadSize;

	int port;
	int allocatedSize;
	int curPos;
	
	// open a TCP socket
	SOCKET sock;
	struct sockaddr_in server;

	set<DWORD> seenIPs;
	set<string> seenHosts;

	clock_t time_ms;

	public:

		URLProcessor(char*** _URLs) {

			URLs = *_URLs;
			urlIndex = 0;
			buf = new char[INITIAL_BUF_SIZE];
			allocatedSize = INITIAL_BUF_SIZE;
		}

		bool parseURL();
		bool lookupDNS(bool reconnect);
		bool connectToSite(bool robots);
		bool loadPage(bool getHEAD);
		bool verifyHeader(char* _status, char expectedCode);
		bool separateHeader();
		bool parseHTML();
		bool printHeader();
		void printBody();
		void nextURL();

		void cleanQuit(char* allocatedBuffer);

		~URLProcessor() {
			delete[] buf;
		}
};