#include "pch.h"

DWORD Checksum::CRC32(unsigned char* buf, size_t len) {
	DWORD c = 0xFFFFFFFF;

	for (size_t i = 0; i < len; i++) {
		c = crc_table[(c ^ buf[i]) & 0xFF] ^ (c >> 8);
	}

	return c ^ 0xFFFFFFFF;
}