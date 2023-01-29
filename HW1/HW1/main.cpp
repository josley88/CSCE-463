/* main.cpp
 * Joseph Shumway
 * CSCE 463
 * Spring 2023
 */

#include "pch.h"

int main(int argc, char** argv)
{

	// check arguments for valid input
	if (argc < 2) {
		printf("Error: no URL given\n");
		WSACleanup();
		exit(-1);
	}
	else if (argc > 2) {
		printf("Error: too many arguments given\n");
		WSACleanup();
		exit(-1);
	}

	char *URL = argv[1];
	printf("URL: %s\n", URL);

	// create URL Grabber worker
	URLGrabber grabber(URL);


	// Note to TA/Prof: These statements may be combined, but are kept separate for future use in case
	// specific exit actions need to be taken for each function
	// ------------------------- Parse URL -------------------------
	if (!grabber.parseURL()) {
		WSACleanup();
		grabber.~URLGrabber();
		exit(-1);
	}



	// ------------------------- Lookup DNS -------------------------
	if (!grabber.lookupDNS()) {
		WSACleanup();
		grabber.~URLGrabber();
		exit(-1);
	}



	// ------------------------- Connect to Server -------------------------
	if (!grabber.connectToSite()) {
		WSACleanup();
		grabber.~URLGrabber();
		exit(-1);
	}



	// ------------------------- Load Page -------------------------
	if (!grabber.loadPage()) {
		WSACleanup();
		grabber.~URLGrabber();
		exit(-1);
	}


	// ------------------------- Header Verification -------------------------

	char status[4];
	grabber.verifyHeader(status);

	// ------------------------- Page Parsing -------------------------
	
	if (status[0] == '2') {
		grabber.parseHTML();
	}


	// ------------------------- Print Header -------------------------

	grabber.printHeader();

	WSACleanup();





























	//// print our primary/secondary DNS IPs
	//DNS dns;
	//dns.printDNSServer ();

	//printf ("-----------------\n");

	//CPU cpu;
	//// run a loop printing CPU usage 10 times
	//for (int i = 0; i < 10; i++)
	//{
	//	// average CPU utilization over 200 ms; must sleep at least a few milliseconds *after* the constructor 
	//	// of class CPU and between calls to GetCpuUtilization
	//	Sleep (200);
	//	// now print
	//	double util = cpu.GetCpuUtilization (NULL);
	//	// -2 means the kernel counters did not accumulate enough to produce a result
	//	if (util != -2)
	//		printf ("current CPU utilization %f%%\n", util);
	//}

	//printf ("-----------------\n");

	//// thread handles are stored here; they can be used to check status of threads, or kill them
	//HANDLE *handles = new HANDLE [3];
	//Test p;
	//
	//// get current time; link with winmm.lib
	//clock_t t = clock();

	//// structure p is the shared space between the threads
	//handles [0] = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE)threadA, &p, 0, NULL);		// start threadA (instance #1) 
	//handles [1] = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE)threadA, &p, 0, NULL);		// start threadA (instance #2)
	//handles [2] = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE)threadB, &p, 0, NULL);		// start threadB 

	//// make sure this thread hangs here until the other three quit; otherwise, the program will terminate prematurely
	//for (int i = 0; i < 3; i++)
	//{
	//	WaitForSingleObject (handles [i], INFINITE);
	//	CloseHandle (handles [i]);
	//}
	//
	//printf ("terminating main(), completion time %.2f sec\n", (double)(clock() - t)/CLOCKS_PER_SEC);
	//return 0; 
}
