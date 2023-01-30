/* URLGrabber.cpp
 * Joseph Shumway
 * CSCE 463
 * Spring 2023
 */

#include "pch.h"
#pragma comment(lib, "ws2_32.lib")

bool URLGrabber::parseURL() {

	// string pointing to an HTTP server (DNS name or IP)
	//char str [] = "128.194.135.72";

	//char testURL[] = "http://tamu.edu?tes:t=1/blah";
	//printf("URL: %s\n\n", URL);

	printf("\tParsing URL... ");

	if (strlen(URL) < 8) {
		printf("invalid URL!\n");
		return false;
	}

	char scheme[8] = "http://";
	char temp[8] = "";
	memcpy(temp, URL, 7);
	if (strcmp(temp, scheme) != 0) {
		printf("failed with invalid scheme\n");
		return false;
	}

	// truncate tagline
	char* tagloc = strchr(URL + 7, '#');
	if (tagloc != NULL) {
		tagloc[0] = 0;
	}

	// reject URLs that are larger than the max allowed (minus the tag since it is not needed)
	if (strlen(URL) > MAX_REQUEST_LEN) {
		printf("failed because URL is longer than max request length");
		WSACleanup();
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
		printf("failed with invalid port\n");
		return false;
	}


	// extract host
	strcpy_s(host, URL + 7);


	//printf("Host: %s\n", host);
	//printf("Port: %d\n", port);
	//printf("Path: %s\n", path);
	//printf("Query: %s\n\n", query);


	printf("host %s, port %d, request %s%s\n", host, port, path, query);

	return true;
}


bool URLGrabber::lookupDNS() {

	WSADATA wsaData;

	//Initialize WinSock; once per program run
	WORD wVersionRequested = MAKEWORD(2, 2);
	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		printf("WSAStartup error %d\n", WSAGetLastError());
		WSACleanup();
		return false;
	}

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		printf("socket() generated error %d\n", WSAGetLastError());
		WSACleanup();
		return false;
	}

	// structure used in DNS lookups
	struct hostent* remote;

	// first assume that the string is an IP address
	DWORD IP = inet_addr(host);

	if (IP == INADDR_NONE) {
		printf("\tDoing DNS... ");
		clock_t time_ms = clock();

		// if not a valid IP, then do a DNS lookup
		if ((remote = gethostbyname(host)) == NULL) {
			printf("failed with %d\n", WSAGetLastError());
			WSACleanup();
			return false;
		}
		else {// take the first IP address and copy into sin_addr
			memcpy((char*)&(server.sin_addr), remote->h_addr, remote->h_length);
			printf("done in %d ms, found %s\n", clock() - time_ms, inet_ntoa(server.sin_addr));
		}

	}
	else {
		// if a valid IP, directly drop its binary version into sin_addr
		server.sin_addr.S_un.S_addr = IP;
	}

	// setup the port # and protocol type
	server.sin_family = AF_INET;
	server.sin_port = htons(port);		// host-to-network flips the byte order

	return true;
}


bool URLGrabber::connectToSite() {

	printf("      * Connecting on page... ");
	time_ms = clock();

	// connect to the server on specified port
	if (connect(sock, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == SOCKET_ERROR) {
		printf("failed with %d\n", WSAGetLastError());
		return false;
	}

	printf("done in %d ms\n", clock() - time_ms);

	return true;

}


bool URLGrabber::loadPage() {

	printf("\tLoading... ");
	time_ms = clock();

	// HTTP request string builder
	char request[MAX_REQUEST_LEN];
	sprintf_s(request, "GET %s%s HTTP/1.0\r\nUser-Agent: crawlerbot / 1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", path, query, host);

	/*printf("Request:\n");
	printf("%s\n", request);*/

	int requestLength = sizeof(request);

	// SEND
	if (send(sock, request, requestLength, 0) == SOCKET_ERROR)
	{
		printf("Send error: %d\n", WSAGetLastError());
		closesocket(sock);
		WSACleanup();
		return NULL;
	}

	// RECIEVE
	int recievedBytes = 0;
	int ret = 0;
	int allocatedSize = INITIAL_BUF_SIZE;
	long totalRecievedBytes = 0;

	timeval timeout;
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;
	

	FD_SET readfds;
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);

	while (true) {

		if ((ret = select(0, &readfds, nullptr, nullptr, &timeout)) > 0) {
			
			recievedBytes = recv(sock, buf + curPos, allocatedSize - curPos, 0);

			// error
			if (recievedBytes == SOCKET_ERROR) {
				printf("failed with %d on recv\n", WSAGetLastError());
				return false;
			}
			
			// connection closed
			if (recievedBytes == 0) {
				//printf("Connection closed\n");
				buf[curPos] = 0;
				break;
			}

			curPos += recievedBytes;

			// resize the array to 2048 + the current size if the incoming data is too big
			if ((allocatedSize - curPos) < recievedBytes) {
				char* tempBuf = new char[allocatedSize + 2048];
				memcpy(tempBuf, buf, curPos);
				
				delete buf;
				buf = tempBuf;

				allocatedSize += 2048;
			}

			totalRecievedBytes += recievedBytes;

		}
		else if (ret == 0) { // timeout
			printf("failed with timeout\n");
			return false;
		}
			
	}

	// check HTTP header
	char http[6] = "HTTP/";
	char tmp[6] = "";
	memcpy(tmp, buf, 5);

	if (strcmp(http, tmp) != 0) {
		printf("failed with non-HTTP header (does not begin with HTTP/)");
		return false;
	}

	// close the socket to this server; open again for the next one
	closesocket(sock);	

	printf("done in %d ms with %d bytes\n", clock() - time_ms, totalRecievedBytes);

	return true;
}


void URLGrabber::verifyHeader(char* _status) {	
	char tmp[] = {buf[9], buf[10], buf[11], '\0'};
	memcpy(_status, tmp, 4);

	printf("\tVerifying header... status code %s\n", _status);
}

void URLGrabber::separateHeader() {

	// move pointer for header to beginning of buffer
	headerBuf = buf;

	unsigned long i = 0;

	// find where two newlines are
	while ((buf[i] != '\r') || (buf[i + 1] != '\n') || (buf[i + 2] != '\r') || (buf[i + 3] != '\n')) {
		i++;
	}

	// move buf pointer to body location
	buf += i + 4;

	// null terminate headerBuf at end of header
	headerBuf[i] = 0;
}

bool URLGrabber::parseHTML() {
	printf("      + Parsing page... ");
	time_ms = clock();
	int numLinks = 0;

	HTMLParserBase parser;
	parser.Parse(buf, curPos, URL, strlen(URL), &numLinks);

	printf("done in % d ms with % d links\n", clock() - time_ms, numLinks);

	return true;
}


void URLGrabber::printHeader() {
	printf("----------------------------------------\n");
	
	// print till program reaches two consecutive newlines
	printf("Loading %d bytes:\n%s\n", sizeof(headerBuf), headerBuf);
}

void URLGrabber::printBody() {
	printf("----------------------------------------\n");

	// print till program reaches two consecutive newlines
	printf("Loading %d bytes:\n", sizeof(buf));
}

