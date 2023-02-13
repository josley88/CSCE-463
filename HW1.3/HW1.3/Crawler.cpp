#include "pch.h"
#include "Crawler.h"

UINT Crawler::statThread() {
	WaitForSingleObject(finished, INFINITY);
	int time_s = 0;

	while (!quitStats) {

		Sleep(TIME_INTERVAL);


		EnterCriticalSection(&criticalSection);
		
		time_s = (clock() - startTime) / TIME_INTERVAL;
		
		// print stats
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

		float crawlSpeed = (successfulCrawledPages - prevSuccCrawlPages) / (TIME_INTERVAL / 1000.0);
		//printf("\tCrawled pages: %d     Prev Crawled pages: %d\n", successfulCrawledPages, prevSuccCrawlPages);
		prevSuccCrawlPages = successfulCrawledPages;

		// get rate, then divide by 125,000 to convert from bytes per sec to megabits per sec
		float downloadRate = (downBytes - prevDownBytes) / (TIME_INTERVAL / 1000.0) / 125000;
		//printf("\tDownloaded kbits: %d     Prev Downloaded kbits: %d\n", downBytes / 125, prevDownBytes / 125);
		prevDownBytes = downBytes;

		//  *** crawling 0.0 pps @ 0.1 Mbps
		printf("\t*** crawling %.1f pps @ %.1f Mbps\n\n", crawlSpeed, downloadRate);

		LeaveCriticalSection(&criticalSection);
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

	// keep going till the URL queue is empty
	while (true) {

		EnterCriticalSection(&criticalSection);
		if (Q.empty()) {
			break;
		}

		// dequeue URL
		char* URL = Q.front();
		char URL2[512] = { 0 };
		memcpy_s(URL2, strlen(URL), URL, strlen(URL));

		Q.pop();
		queueSize--;
		extractedURLs++;

		LeaveCriticalSection(&criticalSection);

		// try network functions and cleanup/continue if they fail
		if (
			!processor.parseURL(URL) ||
			!processor.lookupDNS(&uniqueURLs, &successfulDNSLookups, &uniqueIPs, false) ||
			!processor.connectToSite(true) ||

			// load Robots first
			!processor.loadPage(&downBytes, true) ||
			!processor.separateHeader() ||
			!processor.verifyHeader(status, '4') ||
			!processor.incrementRobots(&passedRobots) || // only reached if the robots checks passed

			// load page if robots passed
			!processor.lookupDNS(&uniqueURLs, &successfulDNSLookups, &uniqueIPs, true) ||
			!processor.connectToSite(false) ||
			!processor.loadPage(&downBytes, false) ||
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


	LeaveCriticalSection(&criticalSection);

	ReleaseSemaphore(finished, 1, NULL);
	activeThreads--;
	return 0;

}


void Crawler::startThreads() {

	// start the stat thread
	stat = CreateThread(NULL, 0, [](LPVOID lpParam) -> DWORD {
		Crawler* p = (Crawler*)lpParam;
		return p->statThread();
		}, this, 0, NULL);

	if (stat == NULL) {
		EnterCriticalSection(&criticalSection);
		printf("Failed to create stat thread\n");
		LeaveCriticalSection(&criticalSection);
	}
	else {
		EnterCriticalSection(&criticalSection);
		//printf("Starting stat thread\n");
		LeaveCriticalSection(&criticalSection);
	}

	// create and start the crawler threads
	for (int i = 0; i < numThreads; i++) {
		threads[i] = CreateThread(NULL, 0, [](LPVOID lpParam) -> DWORD {
			Crawler* p = (Crawler*)lpParam;
			return p->crawlerThread();
			}, this, 0, NULL);

		if (threads[i] == NULL) {
			EnterCriticalSection(&criticalSection);
			printf("Failed to create thread %d\n", i);
			LeaveCriticalSection(&criticalSection);
		}
		else {
			activeThreads++;
			//EnterCriticalSection(&criticalSection);
			//printf("Starting thread %d\n", i);
			//LeaveCriticalSection(&criticalSection);
		}
		
	}

}

void Crawler::waitForThreads() {

	// wait for threads to finish and close their handles
	WaitForMultipleObjects(numThreads, threads, TRUE, INFINITE);

	for (int i = 0; i < numThreads; i++) {
		if (threads[i] != NULL) {
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
	printf("Extracted %d URLs @ %d/s\n", extractedURLs, extractedURLs / ((clock() - startTime) / 1000));
	printf("Total time: %d\n", ((clock() - startTime) / 1000));
}