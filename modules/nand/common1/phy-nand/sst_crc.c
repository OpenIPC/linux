#include  "sst_crc.h"

unsigned int sst_crc32(unsigned int crc, void *buffer, unsigned int length)
{
	unsigned int i, j;
	crc32_data_t crc32;
	unsigned int crc32_calc = 0xffffffff;

	crc32.crc = crc;

	for (i = 0; i < 256; ++i) {
		crc32.crc = i;
		for (j = 0; j < 8; ++j) {
			if (crc32.crc & 1)
				crc32.crc = (crc32.crc >> 1) ^ 0xedb88320;
			else
				crc32.crc >>= 1;
		}
		crc32.crc_32_tbl[i] = crc32.crc;
	}

	crc32_calc = 0xffffffff;
	for (i = 0; i < length; ++i) {
		crc32_calc =
		    crc32.
		    crc_32_tbl[(crc32_calc ^ ((unsigned char *)buffer)[i]) & 0xff] ^
		    (crc32_calc >> 8);
	}

	return crc32_calc ^ 0xffffffff;
}
