/* SenderSocket.h
 * Joseph Shumway
 * CSCE 463
 * Spring 2023
 */

#define MAGIC_PORT			22345
#define MAX_PKT_SIZE		(1500-28)
#define MAX_ATTEMPTS_SYN	3
#define MAX_ATTEMPTS		5
#define TIME_INTERVAL		2000


#define STATUS_OK			0	 // no error
#define ALREADY_CONNECTED	1	 // second call to ss.Open() without closing connection
#define NOT_CONNECTED		2	 // call to ss.Send()/Close() without ss.Open()
#define INVALID_NAME		3	 // ss.Open() with targetHost that has no DNS entry
#define FAILED_SEND			4	 // sendto() failed in kernel
#define TIMEOUT				5	 // timeout after all retx attempts are exhausted
#define FAILED_RECV			6	 // recvfrom() failed in kernel

#define FORWARD_PATH		0
#define RETURN_PATH			1

#define MAGIC_PROTOCOL		0x8311AA



// network header classes
#pragma pack(push, 1)

class Flags {
public:
	DWORD reserved : 5; // must be zero
	DWORD SYN : 1;
	DWORD ACK : 1;
	DWORD FIN : 1;
	DWORD magic : 24;
	Flags() { memset(this, 0, sizeof(*this)); magic = MAGIC_PROTOCOL; }
};

class LinkProperties {
	public:
		// transfer parameters
		float RTT;			// propagation RTT (in sec)
		float speed;		// bottleneck bandwidth (in bits/sec)
		float pLoss[2];		// probability of loss in each direction
		DWORD bufferSize;	// buffer size of emulated routers (in packets)
		LinkProperties() { memset(this, 0, sizeof(*this)); }
};

class SenderDataHeader {
	public:
		Flags flags;
		DWORD seq; // must begin from 0
};

class SenderSynHeader {
	public:
		SenderDataHeader sdh;
		LinkProperties lp;
};

class ReceiverHeader {
	public:
		Flags flags;
		DWORD recvWnd; // receiver window for flow control (in pkts)
		DWORD ackSeq; // ack value = next expected sequence
};

#pragma pack(pop)



// socket class
class SenderSocket {
	

	public:

		char* targetHost;
		int bufferSizePower;
		int senderWindow;
		double rtt;
		double forwardLossRate;
		double returnLossRate;
		float bottleneckSpeed;
		
		int timeStarted;
		int numRetransmissions = 3;
		float RTO;

		// connection vars
		SOCKET sock;
		DWORD* dwordBuf;
		UINT64 dwordBufSize;
		SenderSynHeader syn;
		ReceiverHeader responseHeader;
		int recvBytes;
		int numTimeouts;
		int numFastRet;
		bool connectionOpen;
		struct sockaddr_in server;
		char* ipString[15];

		// transfer vars
		UINT64 cursor;
		UINT64 seq;
		UINT64 byteBufferSize;
		char* charBuf;
		int sentBytes;
		int totalSentBytes;
		int prevSentBytes;
		double obsRTT;
		float estRTT;
		float devRTT;
		float downloadRate;
		std::vector<float> downloadRateList;
		int finalWindow;

		// multithreading vars
		CRITICAL_SECTION criticalSection;
		HANDLE worker;
		HANDLE stat;
		HANDLE finished;
		LONG activeThreads;
		bool quitStats;
		bool statsReady;
		int numThreads;

		
		SenderSocket(char** argv) {
			

			// initialize vars
			charBuf = nullptr;
			cursor = 0;
			stat = nullptr;
			worker = nullptr;
			recvBytes, seq, numTimeouts, numFastRet, byteBufferSize, obsRTT = 0;
			connectionOpen = false;
			downloadRate = 0;
			finalWindow = 0;

			// initialize arg vars
			targetHost = argv[1];
			bufferSizePower = atoi(argv[2]);
			senderWindow = atoi(argv[3]);
			rtt = atof(argv[4]);
			forwardLossRate = atof(argv[5]);
			returnLossRate = atof(argv[6]);
			bottleneckSpeed = atof(argv[7]);

			dwordBufSize = (UINT64)1 << bufferSizePower;
			dwordBuf = new DWORD[dwordBufSize];

			timeStarted = clock();

			// setup flags for SYN packet header
			syn.sdh.flags.reserved = 0;
			syn.sdh.flags.SYN = 1;
			syn.sdh.seq = 0;
			RTO = max(1, rtt * 2);

			// setup flags for SYN link properties
			syn.lp.RTT = rtt;
			syn.lp.speed = 1e6 * bottleneckSpeed;
			syn.lp.pLoss[0] = forwardLossRate;
			syn.lp.pLoss[1] = returnLossRate;
			syn.lp.bufferSize = senderWindow + numRetransmissions;

			// setup threads
			numThreads = senderWindow;
			activeThreads = 0;
			quitStats, statsReady = false;

			InitializeCriticalSection(&criticalSection);
			finished = CreateSemaphore(NULL, 0, numThreads, NULL);


			// print input parameters
			printf("Main: sender W = %d, RTT %g sec, loss %g / %g, link %g Mbps\n",
				senderWindow,
				rtt,
				forwardLossRate,
				returnLossRate,
				bottleneckSpeed
			);

			// initialize buffer
			printf("Main: initializing DWORD array with 2^%d elements... ", bufferSizePower);
			for (UINT64 i = 0; i < dwordBufSize; i++) {
				dwordBuf[i] = i;
			}
			printf("done in %d ms\n", clock() - timeStarted);

			charBuf = (char*)dwordBuf;
			byteBufferSize = dwordBufSize << 2; // convert to bytes
			printf("");
		}

		~SenderSocket() {
			delete[] dwordBuf;
		}

		void quit();
		int open();
		int send(char* ptr, int numBytes, int windowBase);
		int lookupDNS();
		int close();

		int secondsFromFloat(double time);
		int microSecondsFromFloat(double time);

		// multithreading section
		UINT statThread();
		UINT workerThread();
		void startThreads();
		void waitForThreads();
		void printFinalStats();
		float calcRTO(float& estRTT, float sampleRTT, float& devRTT);
};