/*
 * xr806/checksum.h
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

#ifndef __XR_CHECKSUM_H__
#define __XR_CHECKSUM_H__

#define CRC_POLY_16 0xA001
#define CRC_START_16 0x0000

u16 xradio_crc_16(const u8 *input_str, size_t num_bytes);

#endif
