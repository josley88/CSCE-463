#include "pch.h"
#include "Crawler.h"

UINT Crawler::statThread() {

	WaitForSingleObject(finished, INFINITY);
	int time_s = 0;
	int prevTime_s = 0;

	while (!quitStats) {

		Sleep(TIME_INTERVAL);


		// EnterCriticalSection(&criticalSection);
		
		time_s = (clock() - startTime) / 1000;
		
		if (linksFound < 1000) {
			// print stats for 0-1000 links
			printf("[%3d] %3d Q%6d E %7d H%6d D%6d I%5d R%5d C%5d L%4d\n",
				time_s,
				activeThreads,
				queueSize,
				extractedURLs,
				uniqueURLs,
				successfulDNSLookups,
				uniqueIPs,
				passedRobots,
				successfulCrawledPages,
				linksFound
			);
		} else {
			// print stats for 1000+ links
			printf("[%3d] %3d Q%6d E %7d H%6d D%6d I%5d R%5d C%5d L%4dK\n",
				time_s,
				activeThreads,
				queueSize,
				extractedURLs,
				uniqueURLs,
				successfulDNSLookups,
				uniqueIPs,
				passedRobots,
				successfulCrawledPages,
				linksFound / 1000 // print 1K instead of 1000
			);
		}
		

		

		float crawlSpeed = (float) (successfulCrawledPages - prevSuccCrawlPages) / (float) (time_s - prevTime_s);
		//printf("\tCrawled pages: %d     Prev Crawled pages: %d\n", successfulCrawledPages, prevSuccCrawlPages);
		//printf("\tTime: %d    PrevTime: %d\n", time_s, prevTime_s);
		prevSuccCrawlPages = successfulCrawledPages;

		// get rate, then divide by 125,000 to convert from bytes per sec to megabits per sec
		float downloadRate = (float) (downBytes - prevDownBytes) / (float) (time_s - prevTime_s) / 125000;
		//printf("\tDownloaded kbits: %d     Prev Downloaded kbits: %d\n", downBytes / 125, prevDownBytes / 125);
		prevDownBytes = downBytes;

		prevTime_s = time_s;

		//  *** crawling 0.0 pps @ 0.1 Mbps
		printf("\t*** crawling %.1f pps @ %.1f Mbps\n\n", crawlSpeed, downloadRate);

		// LeaveCriticalSection(&criticalSection);
	}

	ReleaseSemaphore(finished, 1, NULL);
	return 0;
}

UINT Crawler::crawlerThread() {

	WaitForSingleObject(finished, INFINITY);
	
	// create URLProcessor and Parser objects and keep for this thread
	URLProcessor processor;
	HTMLParserBase parser;

	char status[4];
	char* URL;

	while (!statsReady) {
		Sleep(500); // wait for all threads to be ready first
	}

	// keep going till the URL queue is empty
	while (true) {

		EnterCriticalSection(&criticalSection);

		if (Q.empty()) {
			LeaveCriticalSection(&criticalSection);
			break;
		}

		// dequeue URL
		URL = Q.front();

		Q.pop();

		LeaveCriticalSection(&criticalSection);

		InterlockedDecrement(&queueSize);
		InterlockedIncrement(&extractedURLs);

		

		// try network functions and cleanup/continue if they fail
		if (
			!processor.parseURL(URL) ||
			!processor.lookupDNS(&uniqueURLs, &successfulDNSLookups, &uniqueIPs, false) ||
			!processor.connectToSite(true) ||

			// load Robots first
			!processor.loadPage(&downBytes, &crawlBytes, true) ||
			!processor.separateHeader() ||
			!processor.verifyHeader(status, '4') ||
			!processor.incrementRobots(&passedRobots) || // only reached if the robots checks passed

			// load page if robots passed
			!processor.lookupDNS(&uniqueURLs, &successfulDNSLookups, &uniqueIPs, true) ||
			!processor.connectToSite(false) ||
			!processor.loadPage(&downBytes, &crawlBytes, false) ||
			!processor.separateHeader() ||
			!processor.verifyHeader(status, '2') ||
			!processor.incrementSuccCrawl(&successfulCrawledPages) || // only reached if the page response is valid
			!processor.parseHTML(&parser, URL, &linksFound) // ||
			//!processor.printHeader()
			) {

			//printf("URL: %s\n", URL2);

			// reset processor for the next URL
			processor.nextURL();
		}
	}

	ReleaseSemaphore(finished, 1, NULL);
	InterlockedDecrement(&activeThreads);

	return 0;

}


void Crawler::startThreads() {

	// create and start the crawler threads
	for (int i = 0; i < numThreads; i++) {
		threads[i] = CreateThread(NULL, 0, [](LPVOID lpParam) -> DWORD {
			Crawler* p = (Crawler*)lpParam;
			return p->crawlerThread();
			}, this, 0, NULL);

		if (threads[i] == NULL) {
			printf("Failed to create thread %d\n", i);
		}
		else {
			InterlockedIncrement(&activeThreads);
		}
		
	}

	// start the stat thread
	stat = CreateThread(NULL, 0, [](LPVOID lpParam) -> DWORD {
		Crawler* p = (Crawler*)lpParam;
		return p->statThread();
		}, this, 0, NULL);

	if (stat == NULL) {
		printf("Failed to create stat thread\n");
	}

	statsReady = true;

	startTime = clock();

}

void Crawler::waitForThreads() {

	// wait for threads to finish and close their handles
	// WaitForMultipleObjects(numThreads, threads, TRUE, INFINITE);

	for (int i = 0; i < numThreads; i++) {
		if (threads[i] != NULL) {
			WaitForSingleObject(threads[i], INFINITE);
			CloseHandle(threads[i]);
		}
		else {
			printf("Error: the handle for thread %d is null\n", i);
		}
	}

	quitStats = true;

	// close stat thread
	WaitForSingleObject(stat, INFINITE);

	if (stat != NULL) {
		CloseHandle(stat);
	}
	else {
		printf("Error: the handle for stat thread is null\n");
	}

	// close finished semaphore
	CloseHandle(finished);

	DeleteCriticalSection(&criticalSection);
	
}

void Crawler::printFinalStats() {
	
	int totalTimeSec = (clock() - startTime) / 1000;

	printf("Extracted %d URLs @ %d/s\n", extractedURLs, extractedURLs / totalTimeSec);
	printf("Looked up %d DNS names @ %d/s\n", uniqueURLs, uniqueURLs / totalTimeSec);
	printf("Attempted %d site robots @ %d/s\n", uniqueIPs, uniqueIPs / totalTimeSec);
	printf("Crawled %d pages @ %d/s (%.2f MB)\n", successfulCrawledPages, successfulCrawledPages / totalTimeSec, (float) crawlBytes / 1000000);
	printf("Parsed %d links @ %d/s\n", linksFound, linksFound / totalTimeSec);
}