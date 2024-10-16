#ifndef __CAL_CRC_H__
#define __CAL_CRC_H__

typedef struct tag_crc32_data {
	unsigned int crc;
	unsigned int crc_32_tbl[256];
} crc32_data_t;

unsigned int sst_crc32(unsigned int crc, void *buffer, unsigned int length);

#endif /* __CAL_CRC_H__ */
