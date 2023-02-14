#pragma once

#define TIME_INTERVAL 2000

class Crawler {

	HANDLE* threads;
	HANDLE  stat;

	HANDLE finished;
	bool quitStats;
	bool statsReady;

	CRITICAL_SECTION criticalSection;

	std::queue<char*> Q;

	int numThreads;
	int startTime;
	
	// stats
	LONG activeThreads;
	LONG queueSize;
	LONG extractedURLs;
	LONG uniqueURLs;
	LONG successfulDNSLookups;
	LONG uniqueIPs;
	LONG passedRobots;
	LONG successfulCrawledPages;
	LONG prevSuccCrawlPages;
	LONG linksFound;
	LONG downBytes;
	LONG prevDownBytes;
	LONG crawlBytes;

	LONG httpCodes[5];

	LONG fromTamu;
	LONG notFromTamu;
	

	public:
		Crawler(int _threads, int _numURLs, vector<char*>& URLs) {
			
			numThreads = _threads;
			
			// stats init
			startTime = clock();
			activeThreads = 0;
			queueSize = 0;
			extractedURLs = 0;
			uniqueURLs = 0;
			successfulDNSLookups = 0;
			uniqueIPs = 0;
			passedRobots = 0;
			successfulCrawledPages = 0;
			prevSuccCrawlPages = 0;
			linksFound = 0;
			downBytes = 0;
			prevDownBytes = 0;
			crawlBytes = 0;
			fromTamu = 0;
			notFromTamu = 0;

			for (int i = 0; i < 5; i++) {
				httpCodes[i] = 0;
			}

			quitStats = false;
			statsReady = false;
			
			InitializeCriticalSection(&criticalSection);
			threads = new HANDLE[numThreads];
			finished = CreateSemaphore(NULL, 0, numThreads, NULL);


			// initialize the queue of URLs
			for (int i = 0; i < _numURLs; i++) {
				Q.push(URLs.at(i));
			}

			URLs.clear();

			queueSize = Q.size();

		}

		~Crawler() {
			delete[] threads;
		}

		UINT statThread();
		UINT crawlerThread();
		void startThreads();
		void waitForThreads();
		void printFinalStats();

};

