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

	bool isFromTamu;

	public:

		URLProcessor(char*** _URLs) {

			URLs = *_URLs;
			urlIndex = 0;
			buf = new char[INITIAL_BUF_SIZE];
			allocatedSize = INITIAL_BUF_SIZE;

			isFromTamu = false;
		}

		URLProcessor() {
			urlIndex = 0;
			buf = new char[INITIAL_BUF_SIZE];
			allocatedSize = INITIAL_BUF_SIZE;

			isFromTamu = false;
		}

		bool parseURL(char* URL);
		bool lookupDNS(LONG* uniqueURLs, LONG* successfulDNSLookups, LONG* uniqueIPs, bool reconnect);
		bool connectToSite(bool robots);
		bool loadPage(LONG* downBytes, LONG* crawlBytes, bool getHEAD);
		bool verifyHeader(char* _status, char expectedCode, LONG* httpCodes, bool robots);
		bool incrementRobots(LONG* passedRobots);
		bool separateHeader();
		bool incrementSuccCrawl(LONG* successfulCrawledPages);
		bool parseHTML(HTMLParserBase* _parser, char* URL, LONG* _extractedURLs, LONG* fromTamu, LONG* notFromTamu);
		bool printHeader();
		void printBody();
		void nextURL();

		void cleanQuit(char* allocatedBuffer);

		~URLProcessor() {
			delete[] buf;
		}
};