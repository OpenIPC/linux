/*
 * xr806/xr_hdr.h
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * xradio transmit protocol define for Xr806 drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __XR_HDR_H__
#define __XR_HDR_H__

struct xradio_hdr {
	u16 checksum;
	u16 message;
	u16 cur_len;
	u16 next_len;
	u16 offset;
} __packed;

enum { XR_DATA = 0, XR_CMD, XR_MAX };

#define XR_REQ_DATA 0x00
#define XR_REQ_CMD 0x01
#define XR_REQ_RECV_LEN 0x02

#define TYPE_ID_MASK 0x3F
#define SEQ_NUM_MASK 0xff00

#endif
