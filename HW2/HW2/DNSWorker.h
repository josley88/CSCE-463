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

#pragma pack(push, 1)
class QueryHeader {
public:
	USHORT qType;
	USHORT qClass;
};

class DNSQuestionHeader {
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
	USHORT type;
	USHORT classs;
	UINT TTL;
	USHORT len;
};
#pragma pack(pop)


class DNSWorker {

public:

	char* host;
	char* server;
	unsigned short txid;
	std::string qTypeStr;

	char* recvBuf;
	unsigned int bufSize;

	SOCKET sock;
	QueryHeader* qHeader;
	DNSQuestionHeader* dnsQHeader;
	DNSAnswerHeader* answerHeader;

	DNSWorker(char** argv) {
		host = argv[1];
		server = argv[2];
		txid = 1;
		qTypeStr = "";

		bufSize = 0;
	}

	~DNSWorker() {
		closesocket(sock);
	}

	
	void openSocket();
	void formPacket(char** sendBuf, int packetSize);
	void sendPacket(char** sendBuf, int packetSize);
	void printQuery();
	void quit();
};