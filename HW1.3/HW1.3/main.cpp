/* main.cpp
 * Joseph Shumway
 * CSCE 463
 * Spring 2023
 */

#include "pch.h"
using namespace Utility;


int main(int argc, char** argv) {

	/*char** URLs = new char* [INITIAL_URL_COUNT];
	for (unsigned int i = 0; i < INITIAL_URL_COUNT; i++) {
		URLs[i] = new char[MAX_HOST_LEN];
	}*/

	std::vector<char*> URLs;
	FileReader reader;

	// get URLs from either input arg or from input file
	int numURLs = getURLs(argc, argv, URLs, reader);


	// if multiple URLs, get threadcount
	int threadCount = 1;
	if (numURLs > 1) {
		sscanf_s(argv[1], "%d", &threadCount);
	}

	// create crawler/shared memory
	Crawler crawler(threadCount, numURLs, URLs);

	//printf("Starting threads...\n");

	crawler.startThreads();
	crawler.waitForThreads();
	crawler.printFinalStats();

	// cleanup heap for URLs
	//for (int i = 0; i < INITIAL_URL_COUNT; i++) {
		//delete[] URLs[i];
	//}
	//delete[] URLs;

}
