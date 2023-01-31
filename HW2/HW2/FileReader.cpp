#include "pch.h"

char* FileReader::readFile(char* filename) {

	// open file
	HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		printf("CreateFile failed with %d\n", GetLastError());
		cleanQuit();
	}

	// get file size
	LARGE_INTEGER li;
	BOOL bRet = GetFileSizeEx(hFile, &li);

	if (bRet == 0) {
		printf("GetFileSizeEx error %d\n", GetLastError());
		cleanQuit();
	}

	// read file into buffer
	fileSize = (DWORD)li.QuadPart;
	DWORD bytesRead;

	// reallocate fileBuf to fileSize
	delete[] fileBuf;
	fileBuf = new char[fileSize];
	bRet = ReadFile(hFile, fileBuf, fileSize, &bytesRead, NULL);

	// process errors
	if (bRet == 0 || bytesRead != fileSize) {
		printf("ReadFile failed with %d\n", GetLastError());
		cleanQuit();
	}

	CloseHandle(hFile);

	return fileBuf;
}

int FileReader::extractURLs(char*** URLs) {
	
	int i = 0;

	char* token;
	char* nextToken;

	token = strtok_s(fileBuf, "\r\n", &nextToken);

	// loop through tokens until EOF
	while (token != NULL) {
		int tokenLength = strlen(token);

		// copy token to URL array at i
		memcpy_s((*URLs)[i], MAX_HOST_LEN, token, tokenLength);

		// null terminate the string
		(*URLs)[i][tokenLength] = 0;
		i++;

		// get next token
		token = strtok_s(NULL, "\r\n", &nextToken);
	}
	
	// return the number of URLs
	return i + 1;
}


void FileReader::printAt(int i) {
	if (i < 0) {
		printf("Char in file at %d bytes from the back: [%d]", -i, (unsigned int) fileBuf[fileSize - i]);
	}
	else {
		printf("Char in file at %d bytes from the front: [%d]", i, (unsigned int) fileBuf[i]);
	}
}


void FileReader::cleanQuit() {
	
	// cleanup heap for URLs
	for (int i = 0; i < MAX_URL_COUNT; i++) {
		delete[] URLs[i];
	}
	delete[] URLs;

	WSACleanup();
	exit(-1);
}