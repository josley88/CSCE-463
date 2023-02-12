/* main.cpp
 * Joseph Shumway
 * CSCE 463
 * Spring 2023
 */

#include "pch.h"
using namespace Utility;


int main(int argc, char** argv)
{

	char** URLs = new char* [INITIAL_URL_COUNT];
	for (unsigned int i = 0; i < INITIAL_URL_COUNT; i++) {
		URLs[i] = new char[MAX_HOST_LEN];
	}

	// get URLs from either input arg or from input file
	int numURLs = getURLs(argc, argv, &URLs);


	// if multiple URLs, get threadcount
	int threadCount = 1;
	if (numURLs > 1) {
		sscanf_s(argv[1], "%d", &threadCount);
	}

	// create crawler/shared memory
	Crawler crawler(threadCount, numURLs, &URLs);

	//printf("Starting threads...\n");

	crawler.startThreads();
	crawler.waitForThreads();
	crawler.printFinalStats();

	// cleanup heap for URLs
	for (int i = 0; i < INITIAL_URL_COUNT; i++) {
		delete[] URLs[i];
	}
	delete[] URLs;


	

	/*



	// create URLProcessor worker
	URLProcessor processor(&URLs);

	// for each URL, process it
	for (int i = 0; i < numURLs; i++) {
		char status[4];

		// try network functions and cleanup/continue if they fail
		if (
			!processor.parseURL()					||
			!processor.lookupDNS(false)				||
			!processor.connectToSite(true)			||

			// load Robots first
			!processor.loadPage(true)				||
			!processor.separateHeader()				||
			!processor.verifyHeader(status, '4')	||

			// load page if robots passed
			!processor.lookupDNS(true)				||
			!processor.connectToSite(false)			||
			!processor.loadPage(false)				||
			!processor.separateHeader()				||
			!processor.verifyHeader(status, '2')	||
			!processor.parseHTML()					||
			!processor.printHeader()
		) {
			processor.nextURL();
			continue;
		}
		//grabber.printBody();

		processor.nextURL();
	}


	// cleanup heap for URLs
	for (int i = 0; i < INITIAL_URL_COUNT; i++) {
		delete[] URLs[i];
	}
	delete[] URLs;

	WSACleanup();



	*/

}
