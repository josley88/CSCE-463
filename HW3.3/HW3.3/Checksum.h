class Checksum {
	public:
		DWORD crc_table[256];


		Checksum() {
			// set up lookup table for later use
			for (DWORD i = 0; i < 256; i++) {
				DWORD c = i;

				for (int j = 0; j < 8; j++) {
					c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
				}
				crc_table[i] = c;
			}
		}

		DWORD CRC32(unsigned char* buf, size_t len);
};

