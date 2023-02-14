/* URLProcessor.cpp
 * Joseph Shumway
 * CSCE 463
 * Spring 2023
 */

#include "pch.h"
#pragma comment(lib, "ws2_32.lib")

bool URLProcessor::parseURL(char* URL) {

	// string pointing to an HTTP server (DNS name or IP)
	//char str [] = "128.194.135.72";

	//char testURL[] = "http://tamu.edu?tes:t=1/blah";
	//printf("URL: %s\n\n", URL);

	//printf("URL: % s\n\tParsing URL... ", URL);

	if (strlen(URL) < 8) {
		char* test = URL;
		//printf("invalid URL!\n");
		return false;
	}

	char scheme[8] = "http://";
	char temp[8] = "";
	memcpy(temp, URL, 7);
	if (strcmp(temp, scheme) != 0) {
		//printf("failed with invalid scheme\n");
		char* test = URL;
		return false;
	}

	// truncate tagline
	char* tagloc = strchr(URL + 7, '#');
	if (tagloc != NULL) {
		tagloc[0] = 0;
	}

	// reject URLs that are larger than the max allowed (minus the tag since it is not needed)
	if (strlen(URL) > MAX_REQUEST_LEN) {
		//printf("failed because URL is longer than max request length");
		return false;
	}

	// extract query
	char* queryloc = strchr(URL + 7, '?');
	if (queryloc != NULL) {								// make sure the query exists before extracting
		strcpy_s(query, queryloc);
		queryloc[0] = 0;
	}
	else {												// if it doesnt exist, null terminate it
		query[0] = 0;
	}


	// extract path
	path[0] = '/';
	char* pathloc = strchr(URL + 7, '/');
	if (pathloc != NULL) {
		strcpy_s(path, pathloc);
		pathloc[0] = 0;
	}
	else {
		path[1] = 0;
	}


	// extract port
	char port_c[6];
	port = 0;
	char* portloc = strchr(URL + 7, ':');
	if (portloc != NULL) {								// make sure the port exists before extracting
		strcpy_s(port_c, portloc + 1);					// + 1 to ignore the : when converting to int
		portloc[0] = 0;
		sscanf_s(port_c, "%d", &port);					// convert char port to int port
	}
	else {												// if it doesnt exist, set to port 80 as default
		port = 80;
		port_c[0] = '8';
		port_c[1] = '0';
		port_c[2] = 0;
	}

	if (port == 0 || port > 65535) {
		//printf("failed with invalid port\n");
		return false;
	}


	// extract host
	strcpy_s(host, URL + 7);


	//printf("Host: %s\n", host);
	//printf("Port: %d\n", port);
	//printf("Path: %s\n", path);
	//printf("Query: %s\n\n", query);


	//printf("host %s, port %d, request %s%s\n", host, port, path, query);

	return true;
}

bool URLProcessor::lookupDNS(LONG* uniqueURLs, LONG* successfulDNSLookups, LONG* uniqueIPs, bool reconnect = false) {

	WSADATA wsaData;

	//Initialize WinSock; once per program run
	WORD wVersionRequested = MAKEWORD(2, 2);
	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		//printf("WSAStartup error %d\n", WSAGetLastError());
		return false;
	}

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		//printf("socket() generated error %d\n", WSAGetLastError());
		return false;
	}

	// no need to lookup DNS if its a reconnect
	if (reconnect) {
		return true;
	}

	// structure used in DNS lookups
	struct hostent* remote;

	// first assume that the string is an IP address
	DWORD IP = inet_addr(host);

	//printf("\tChecking host uniqueness... ");

	// if not unique host, move on
	if (!seenHosts.insert(host).second) {
		//printf("failed\n");
		return false;
	}
	else {
		InterlockedIncrement(uniqueURLs);
		//printf("passed\n");
	}
	
	

	if (IP == INADDR_NONE) {

		//printf("\tDoing DNS... ");
		clock_t time_ms = clock();

		// if not a valid IP, then do a DNS lookup
		if ((remote = gethostbyname(host)) == NULL) {
			//printf("failed with %d\n", WSAGetLastError());
			return false;
		}
		else {// take the first IP address and copy into sin_addr
			memcpy((char*)&(server.sin_addr), remote->h_addr, remote->h_length);
			//printf("done in %d ms, found %s\n", clock() - time_ms, inet_ntoa(server.sin_addr));

			// if not unique IP, move on
			//printf("\tChecking IP uniqueness... ");
			if (!seenIPs.insert(inet_addr(inet_ntoa(server.sin_addr))).second) {
				//printf("failed\n");
				return false;
			}
			else {
				InterlockedIncrement(successfulDNSLookups);
				InterlockedIncrement(uniqueIPs);
				//printf("passed\n");
			}
			
		}

	}
	else {
		//printf("\tChecking IP uniqueness... ");

		// if not unique IP, move on
		if (!seenIPs.insert(IP).second) {
			//printf("failed\n");
			return false;
		}
		else {
			InterlockedIncrement(uniqueIPs);
			//printf("passed\n");
		}
		

		// if a valid IP, directly drop its binary version into sin_addr
		server.sin_addr.S_un.S_addr = IP;
	}

	// setup the port # and protocol type
	server.sin_family = AF_INET;
	server.sin_port = htons(port);		// host-to-network flips the byte order

	return true;
}


bool URLProcessor::connectToSite(bool robots = false) {

	if (robots) {
		//printf("        Connecting on robots... ");
	}
	else {
		//printf("      * Connecting on page... ");
	}
	
	time_ms = clock();

	// connect to the server on specified port
	if (connect(sock, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == SOCKET_ERROR) {
		//printf("failed with %d\n", WSAGetLastError());
		return false;
	}

	//printf("done in %d ms\n", clock() - time_ms);

	return true;

}


bool URLProcessor::loadPage(LONG* downBytes, LONG* crawlBytes, bool robots) {

	if (robots) {
		maxDownloadSize = 16000;
	}
	else {
		maxDownloadSize = 2000000;
	}

	//printf("\tLoading... ");
	time_ms = clock();

	// HTTP request string builder
	char request[MAX_REQUEST_LEN];
	if (robots) {
		sprintf_s(request, "HEAD /robots.txt HTTP/1.0\r\nUser-Agent: crawlerbot / 1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", host);
	}
	else {
		sprintf_s(request, "GET %s%s HTTP/1.0\r\nUser-Agent: crawlerbot / 1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", path, query, host);
	}
	


	int requestLength = sizeof(request);

	// SEND
	if (send(sock, request, requestLength, 0) == SOCKET_ERROR)
	{
		//printf("Send error: %d\n", WSAGetLastError());
		return false;
	}

	// RECIEVE
	unsigned int recievedBytes = 0;
	int ret = 0;
	unsigned long totalRecievedBytes = 0;
	curPos = 0;

	time_t startTime = time(NULL);
	timeval timeout;
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;
	

	FD_SET readfds;
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);

	

	while (true) {

		timeout.tv_sec = 10 - (time(NULL) - startTime);
		
		if ((ret = select(0, &readfds, nullptr, nullptr, &timeout)) > 0) {
			
			recievedBytes = recv(sock, buf + curPos, allocatedSize - curPos, 0);

			// error
			if (recievedBytes == SOCKET_ERROR) {
				//printf("failed with %d on recv\n", WSAGetLastError());
				return false;
			}
			
			// connection closed
			if (recievedBytes == 0) {

				buf[curPos] = 0;
				break;
			}

			if (recievedBytes + totalRecievedBytes > maxDownloadSize) {
				//printf("failed with exceeding max\n");
				return false;
			}

			curPos += recievedBytes;

			// resize the array to double the current size if the incoming data is too big
			if ((allocatedSize - curPos) < recievedBytes) {
				char* tempBuf = new char[allocatedSize * 2];
				memcpy_s(tempBuf, allocatedSize * 2, buf, allocatedSize);
				printf("");
				delete[] buf;
				buf = tempBuf;

				allocatedSize *= 2;
			}

			totalRecievedBytes += recievedBytes;

		}
		else if (ret == 0) { // timeout
			//printf("\n\n\t********** failed with slow download of %d seconds\n", (time(NULL) - startTime) / 1000);
			return false;
		}
			
	}

	bodySize = totalRecievedBytes;

	// check HTTP header
	char http[6] = "HTTP/";
	char tmp[6] = "";
	memcpy(tmp, buf, 5);

	if (strcmp(http, tmp) != 0) {
		//printf("failed with non-HTTP header (does not begin with HTTP/)\n");
		//printf("Header: %s", buf);
		return false;
	}


	// close the socket to this server; open again for the next one
	closesocket(sock);
	WSACleanup();


	//printf("done in %d ms with %d bytes\n", clock() - time_ms, totalRecievedBytes);
	//printf("%s\n", buf);

	InterlockedAdd(downBytes, totalRecievedBytes);

	if (!robots) {
		InterlockedAdd(crawlBytes, totalRecievedBytes);
	}

	return true;
}


bool URLProcessor::verifyHeader(char* _status, char expectedCode) {	
	char tmp[] = {headerBuf[9], headerBuf[10], headerBuf[11], '\0'};
	memcpy(_status, tmp, 4);

	//printf("\tVerifying header... status code %s\n", _status);
	//printf("\tExpecting code %cXX\n", expectedCode);
	return _status[0] == expectedCode;
}

bool URLProcessor::incrementRobots(LONG* passedRobots) {
	InterlockedIncrement(passedRobots);

	return true;
}

bool URLProcessor::separateHeader() {

	// move pointer for header to beginning of buffer
	headerBuf = buf;

	unsigned long i = 0;

	// find where two newlines are and stop before too long (2000 characters)
	while (i < (2000) && ((buf[i] != '\r') || (buf[i + 1] != '\n') || (buf[i + 2] != '\r') || (buf[i + 3] != '\n'))) {
		i++;
	}

	// set body location
	bodyBuf = buf + i + 4;

	// null terminate headerBuf at end of header
	headerBuf[i] = 0;
	headerSize = i;
	bodySize -= headerSize;

	return true;
}

bool URLProcessor::incrementSuccCrawl(LONG* successfulCrawledPages) {
	InterlockedIncrement(successfulCrawledPages);

	return true;
}

bool URLProcessor::parseHTML(HTMLParserBase* _parser, char* URL, LONG* linksFound) {
	//printf("      + Parsing page... ");
	time_ms = clock();
	int numLinks = 0;

	(*_parser).Parse(buf, curPos, URL, strlen(URL), &numLinks);

	InterlockedAdd(linksFound, numLinks);

	//printf("done in % d ms with % d links\n", clock() - time_ms, numLinks);

	return true;
}


bool URLProcessor::printHeader() {
	printf("----------------------------------------\n");

	// print till program reaches two consecutive newlines
	//printf("Loading %d bytes for header:\n%s\n", headerSize, headerBuf);
	
	return true;
}

void URLProcessor::printBody() {
	printf("----------------------------------------\n");

	// print till program reaches two consecutive newlines
	//printf("Loading %d bytes for body:\n%s\n", bodySize, bodyBuf);
}

void URLProcessor::nextURL() {

	// reset buffer if greater than 32KB
	if (allocatedSize > 32000) {
		delete[] buf;
		buf = new char[INITIAL_BUF_SIZE];
		allocatedSize = INITIAL_BUF_SIZE;
	}

	curPos = 0;

	//urlIndex++;
}

void URLProcessor::cleanQuit(char* allocatedBuffer) {
	WSACleanup();
	delete[] allocatedBuffer;
	exit(-1);
}