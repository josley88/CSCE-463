#pragma once
class FileReader {

	char* fileBuf;
	int fileSize;
	char** URLs;
	

	public: 
		FileReader() {
			fileSize = 0;
			fileBuf = new char[1];
			URLs = new char* [MAX_URL_COUNT];
			for (unsigned int i = 0; i < MAX_URL_COUNT; i++) {
				URLs[i] = new char[MAX_HOST_LEN];
			}
		}


		char* readFile(char* filename);
		int extractURLs(char*** URLs);
		void printAt(int i);

	private:
		void cleanQuit();
};

