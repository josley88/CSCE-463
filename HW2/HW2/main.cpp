/* main.cpp
 * Joseph Shumway
 * CSCE 463
 * Spring 2023
 */

#include "pch.h"
using namespace Utility;


int main(int argc, char** argv)
{

	char* URL;

	//check arguments
	switch (argc) {
	case 1:
		printf("Error: no URL given\n");
		WSACleanup();
		exit(-1);
	case 2:
		URL = argv[1];
		break;
	case 3:
		if (argv[1][0] != '1' || strlen(argv[1]) != 1) { // check if num threads is not 1 (also check it isnt 1xxx...)
			printf("Error: number of threads must be 1");
			WSACleanup();
			exit(-1);
		}
		URL = argv[2];
		break; // these are valid argument counts
	default:
		printf("Error: too many arguments given\n");
		WSACleanup();
		exit(-1);
	}

	char* URLs[128];
	URLs[0] = URL;

	printf("URL: %s\n", URLs[0]);

	// create URL Grabber worker
	URLGrabber grabber(URLs);

	// try network functions and cleanup/exit if they fail
	attempt(grabber.parseURL(), grabber);
	attempt(grabber.lookupDNS(), grabber);
	attempt(grabber.connectToSite(), grabber);
	attempt(grabber.loadPage(), grabber);

	grabber.separateHeader();

	char status[4];
	grabber.verifyHeader(status);
	
	// only parse if status code is 2xx
	if (status[0] == '2') {
		grabber.parseHTML();
	}


	grabber.printHeader();
	grabber.printBody();

	WSACleanup();

}
