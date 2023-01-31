/* main.cpp
 * Joseph Shumway
 * CSCE 463
 * Spring 2023
 */
#include "pch.h"
using namespace Utility;


int main(int argc, char** argv)
{

	char** URLs = new char* [MAX_URL_COUNT];
	for (unsigned int i = 0; i < MAX_URL_COUNT; i++) {
		URLs[i] = new char[MAX_HOST_LEN];
	}

	// get URLs from either input arg or from input file
	int numURLs = getURLs(argc, argv, &URLs);

	//printf("There are %d URLs\n", numURLs);


	// create URLProcessor worker
	URLProcessor processor(&URLs);

	// for each URL, process it
	for (int i = 0; i < numURLs; i++) {
		char status[4];

		// try network functions and cleanup/continue if they fail
		if (
			!processor.parseURL()					||
			!processor.lookupDNS()					||
			!processor.connectToSite()				||

			// load Robots first
			!processor.loadPage(true)				||
			!processor.separateHeader()				||
			!processor.verifyHeader(status, '4')	||

			// load page if robots passed
			!processor.loadPage(false)				||
			!processor.separateHeader()				||
			!processor.verifyHeader(status, '2')
		) {
			processor.nextURL();
			continue;
		}

		processor.parseHTML();
		processor.printHeader();
		//grabber.printBody();

		processor.nextURL();
	}


	// cleanup heap for URLs
	for (int i = 0; i < MAX_URL_COUNT; i++) {
		delete[] URLs[i];
	}
	delete[] URLs;

	WSACleanup();

}
