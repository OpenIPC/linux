/*
 * xr806/checksum.c
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * Checksum implementation for Xr806 drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "os_intf.h"
#include "checksum.h"

static u16 crc_tab16[256];
static u8 crc_init;

static void xradio_init_crc16_tab(void)
{
	u16 i;
	u16 j;
	u16 crc;
	u16 c;

	for (i = 0; i < 256; i++) {
		crc = 0;
		c = i;

		for (j = 0; j < 8; j++) {
			if ((crc ^ c) & 0x0001)
				crc = (crc >> 1) ^ CRC_POLY_16;
			else
				crc = crc >> 1;
			c = c >> 1;
		}

		crc_tab16[i] = crc;
	}
	crc_init = 1;
}

u16 xradio_crc_16(const u8 *input_str, size_t num_bytes)
{
	u16 crc;
	const unsigned char *ptr;
	size_t a;

	if (!crc_init)
		xradio_init_crc16_tab();

	crc = CRC_START_16;
	ptr = input_str;

	if (ptr != NULL) {
		for (a = 0; a < num_bytes; a++)
			crc = (crc >> 8) ^ crc_tab16[(crc ^ (u16)*ptr++) & 0x00FF];
	}
	return crc;
}
