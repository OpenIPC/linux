/*
 *  arch/arm/mach-faraday/include/mach/ftapbb020.h
 *
 *  Faraday FTAPBB020 APB Bridge with DMA function
 *
 *  Copyright (C) 2010 Faraday Technology
 *  Copyright (C) 2010 Po-Yu Chuang <ratbert@faraday-tech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __FTAPBB020_H
#define __FTAPBB020_H

#define FTAPBB020_OFFSET_BSR(x)		((x) * 0x4)		/* BSR of slave x */
#define FTAPBB020_OFFSET_SAR(x)		(0x80 + (x) * 0x10)	/* src addr of channel x */
#define FTAPBB020_OFFSET_DAR(x)		(0x84 + (x) * 0x10)	/* dst addr of channel x */
#define FTAPBB020_OFFSET_CYC(x)		(0x88 + (x) * 0x10)	/* cycles of channel x */
#define FTAPBB020_OFFSET_CMD(x)		(0x8c + (x) * 0x10)	/* command of channel x */
#define FTAPBB020_OFFSET_CR		0xc0
#define FTAPBB020_OFFSET_SR		0xc4
#define FTAPBB020_OFFSET_REV		0xc8

/*
 * Base/size of each slave
 */
#define FTAPBB020_BSR_SIZE_1M		(0 << 16)
#define FTAPBB020_BSR_SIZE_2M		(1 << 16)
#define FTAPBB020_BSR_SIZE_4M		(2 << 16)
#define FTAPBB020_BSR_SIZE_8M		(3 << 16)
#define FTAPBB020_BSR_SIZE_16M		(4 << 16)
#define FTAPBB020_BSR_SIZE_32M		(5 << 16)
#define FTAPBB020_BSR_SIZE_64M		(6 << 16)
#define FTAPBB020_BSR_SIZE_128M		(7 << 16)
#define FTAPBB020_BSR_SIZE_256M		(8 << 16)
#define FTAPBB020_BSR_BASE(x)		((x) & (0x3ff << 20))

/*
 * Cycle count of each DMA channel
 */
#define FTAPBB020_CYC_MASK		0x00ffffff

/*
 * Command of each DMA channel
 */
#define FTAPBB020_CMD_ENABLE		(1 << 0)
#define FTAPBB020_CMD_FININT_S		(1 << 1)
#define FTAPBB020_CMD_FININT_E		(1 << 2)
#define FTAPBB020_CMD_BURST		(1 << 3)
#define FTAPBB020_CMD_ERRINT_S		(1 << 4)
#define FTAPBB020_CMD_ERRINT_E		(1 << 5)
#define FTAPBB020_CMD_SRC_TYPE_AHB	(1 << 6)
#define FTAPBB020_CMD_DST_TYPE_AHB	(1 << 7)
#define FTAPBB020_CMD_SRC_MODE_FIXED	(0 << 8)
#define FTAPBB020_CMD_SRC_MODE_BYTE_INC	(1 << 8)
#define FTAPBB020_CMD_SRC_MODE_HALF_INC	(2 << 8)
#define FTAPBB020_CMD_SRC_MODE_WORD_INC	(3 << 8)
#define FTAPBB020_CMD_SRC_MODE_BYTE_DEC	(5 << 8)
#define FTAPBB020_CMD_SRC_MODE_HALF_DEC	(6 << 8)
#define FTAPBB020_CMD_SRC_MODE_WORD_DEC	(7 << 8)
#define FTAPBB020_CMD_DST_MODE_FIXED	(0 << 12)
#define FTAPBB020_CMD_DST_MODE_BYTE_INC	(1 << 12)
#define FTAPBB020_CMD_DST_MODE_HALF_INC	(2 << 12)
#define FTAPBB020_CMD_DST_MODE_WORD_INC	(3 << 12)
#define FTAPBB020_CMD_DST_MODE_BYTE_DEC	(5 << 12)
#define FTAPBB020_CMD_DST_MODE_HALF_DEC	(6 << 12)
#define FTAPBB020_CMD_DST_MODE_WORD_DEC	(7 << 12)
#define FTAPBB020_CMD_DST_HANDSHAKE(x)	(((x) & 0xf) << 16)	/* destination handshake channel */
#define FTAPBB020_CMD_WIDTH_WORD	(0 << 20)
#define FTAPBB020_CMD_WIDTH_HALF	(1 << 20)
#define FTAPBB020_CMD_WIDTH_BYTE	(2 << 20)
#define FTAPBB020_CMD_SRC_HANDSHAKE(x)	(((x) & 0xf) << 24)	/* source handshake channel */

/*
 * Control register
 */
#define FTAPBB020_CR_BUF_NORMAL		(0 << 0)
#define FTAPBB020_CR_BUF_ALWAYS		(1 << 0)
#define FTAPBB020_CR_BUF_NEVER		(2 << 0)
#define FTAPBB020_CR_BWERRINT_E		(1 << 2)

/*
 * Status register
 */
#define FTAPBB020_SR_BWERRINT		(1 << 0)

/*
 * Revision register
 */
#define FTAPBB020_REV_REVISION(rev)	((rev) & ~(0xff << 24))

enum ftapbb020_bus_type {
	FTAPBB020_BUS_TYPE_AHB,
	FTAPBB020_BUS_TYPE_APB,
};

enum ftapbb020_channels {
	FTAPBB020_CHANNEL_0 = (1 << 0),
	FTAPBB020_CHANNEL_1 = (1 << 1),
	FTAPBB020_CHANNEL_2 = (1 << 2),
	FTAPBB020_CHANNEL_3 = (1 << 3),
	FTAPBB020_CHANNEL_ALL = 0xf,
};

/**
 * struct ftapbb020_dma_slave - DMA slave data
 * @common: physical address and register width...
 * @type: bus type of the device
 * @channels: bitmap of usable DMA channels
 * @handshake: hardware handshake number
 */
struct ftapbb020_dma_slave {
	struct dma_slave_config common;
	enum ftapbb020_bus_type type;
//	enum ftapbb020_channels channels;
	unsigned int handshake;
};

/**
 * ftapbb020_chan_filter() - filter function for dma_request_channel().
 * @chan: DMA channel
 * @data: pointer to ftapbb020_dma_slave
 */
bool ftapbb020_chan_filter(struct dma_chan *chan, void *data);

#endif	/* __FTAPBB020_H */
