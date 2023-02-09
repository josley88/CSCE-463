/* FileReader.h
 * Joseph Shumway
 * CSCE 463
 * Spring 2023
 */

#pragma once
class FileReader {

	char* fileBuf;
	char** URLs;

	int numURLs = INITIAL_URL_COUNT;
	int fileSize;
	
	

	public: 
		FileReader() {
			fileSize = 0;
		}


		char* readFile(char* filename);
		int extractURLs(char*** URLs);
		void printAt(int i);

		~FileReader() {
			delete[] fileBuf;
		}

	private:
		void cleanQuit();
};

