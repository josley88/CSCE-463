#include "pch.h"

namespace Utility {

	int getURLs(int argc, char** argv, char*** URLs) {
		
		char filename[128] = { 0 };
		FileReader reader;

		switch (argc) {
			case 1:
				printf("Error: no URL given\n");
				cleanQuit();

			case 2: // URL input
				memcpy_s((*URLs)[0], MAX_HOST_LEN, argv[1], strlen(argv[1]));

				// null terminate
				(*URLs)[0][strlen(argv[1])] = 0;

				return 1;

			case 3: // file input

				// check if num threads is not 1 (also check it isnt 1xxx...)
				if (argv[1][0] != '1' || strlen(argv[1]) != 1) { 
					printf("Error: number of threads must be 1");
					cleanQuit();
				}
				strcpy_s(filename, 128, argv[2]);

				// read file
				reader.readFile(filename);

				return reader.extractURLs(URLs);

			default:
				printf("Error: too many arguments given\n");
				cleanQuit();
		}
	}

	void Utility::cleanQuit() {
		WSACleanup();
		exit(-1);
	}
}