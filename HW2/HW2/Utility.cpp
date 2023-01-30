#include "pch.h"

namespace Utility {

	// functions for cleanup and exit on error
	void attempt(bool functionSuccess) {
		if (!functionSuccess) {
			WSACleanup();
			exit(-1);
		}
	}

	void attempt(bool functionSuccess, URLGrabber grabberObj) {
		if (!functionSuccess) {
			WSACleanup();
			grabberObj.~URLGrabber();
			exit(-1);
		}
	}
}