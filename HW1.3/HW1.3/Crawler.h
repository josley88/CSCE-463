#pragma once

#define TIME_INTERVAL 2000

class Crawler {

	HANDLE* threads;
	HANDLE  stat;

	HANDLE finished;
	bool quitStats;

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

			quitStats = false;
			
			InitializeCriticalSection(&criticalSection);
			threads = new HANDLE[numThreads];
			finished = CreateSemaphore(NULL, 0, numThreads, NULL);


			// initialize the queue of URLs
			for (int i = 0; i < _numURLs; i++) {
				Q.push(URLs.at(i));
			}

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

