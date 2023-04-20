/* pch.h
 * Joseph Shumway
 * CSCE 463
 * Spring 2023
 */

// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H
#define DEBUG false

// add headers that you want to pre-compile here
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <windows.h>
//#include <winsock.h>
#include <WinSock2.h>
#include <ctime>
#include <string>
#include <iostream>
#include <vector>
#include <queue>

#include "SenderSocket.h"
#include "Checksum.h"

#endif //PCH_H
