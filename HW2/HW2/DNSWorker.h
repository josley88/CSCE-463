/* DNS.h
 * Joseph Shumway
 * CSCE 463
 * Spring 2023
 */

#pragma once

#define DNS_QUERY (0 << 15) /* 0 = query; 1 = response */
#define DNS_RESPONSE (1 << 15)
#define DNS_STDQUERY (0 << 11) /* opcode - 4 bits */
#define DNS_AA (1 << 10) /* authoritative answer */
#define DNS_TC (1 << 9) /* truncated */
#define DNS_RD (1 << 8) /* recursion desired */
#define DNS_RA (1 << 7) /* recursion available */

#define MAX_DNS_SIZE 512 // largest valid UDP packet
#define MAX_ATTEMPTS 3
#define TIMEOUT 10

#pragma pack(push, 1)
class QueryHeader {
public:
	USHORT qType;
	USHORT qClass;
};

class FixedDNSHeader {
public:
	USHORT ID;
	USHORT flags;
	USHORT questions;
	USHORT answers;
	USHORT authRRs;
	USHORT addRRs;
};

class DNSAnswerHeader {
public:
	USHORT aType;
	USHORT aClass;
	UINT   TTL;
	USHORT len;
};
#pragma pack(pop)


class DNSWorker {

public:

	char* host;
	char* server;
	unsigned short txid;
	std::string qTypeStr;


	SOCKET sock;
	struct sockaddr_in remote;

	QueryHeader* qHeader;
	FixedDNSHeader* dnsQHeader;
	FixedDNSHeader* fixedDNSAnsHeader;
	DNSAnswerHeader* dnsAnsHeader;

	clock_t startTime;

	DNSWorker(char** argv) {
		host = argv[1];
		server = argv[2];
		txid = 1;
		qTypeStr = "";

		memset(&remote, 0, sizeof(remote));
	}

	~DNSWorker() {
		delete[] host;
		closesocket(sock);
	}

	
	void openSocket();
	void formPacket(char** sendBuf, int packetSize, bool isHost);
	bool sendPacket(char** sendBuf, int packetSize);
	void recvPacket(char** recvBuf);
	void parsePacket(char** recvBuf);
	void printQuery(char* originalHost);
	void quit();
};