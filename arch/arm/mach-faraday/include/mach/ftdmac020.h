/*
 *  arch/arm/mach-faraday/include/mach/ftdmac020.h
 *
 *  Faraday FTDMAC020 DMA controller
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

#ifndef __FTDMAC020_H
#define __FTDMAC020_H

#define FTDMAC020_OFFSET_ISR		0x0
#define FTDMAC020_OFFSET_TCISR		0x4
#define FTDMAC020_OFFSET_TCICR		0x8
#define FTDMAC020_OFFSET_EAISR		0xc
#define FTDMAC020_OFFSET_EAICR		0x10
#define FTDMAC020_OFFSET_TCRAW		0x14
#define FTDMAC020_OFFSET_EARAW		0x18
#define FTDMAC020_OFFSET_CH_ENABLED	0x1c
#define FTDMAC020_OFFSET_CH_BUSY	0x20
#define FTDMAC020_OFFSET_CR		0x24
#define FTDMAC020_OFFSET_SYNC		0x28
#define FTDMAC020_OFFSET_REVISION	0x2c
#define FTDMAC020_OFFSET_FEATURE	0x30

#define FTDMAC020_OFFSET_CCR_CH(x)	(0x100 + (x) * 0x20)
#define FTDMAC020_OFFSET_CFG_CH(x)	(0x104 + (x) * 0x20)
#define FTDMAC020_OFFSET_SRC_CH(x)	(0x108 + (x) * 0x20)
#define FTDMAC020_OFFSET_DST_CH(x)	(0x10c + (x) * 0x20)
#define FTDMAC020_OFFSET_LLP_CH(x)	(0x110 + (x) * 0x20)
#define FTDMAC020_OFFSET_CYC_CH(x)	(0x114 + (x) * 0x20)

/*
 * Error/abort interrupt status/clear register
 * Error/abort status register
 */
#define FTDMAC020_EA_ERR_CH(x)		(1 << (x))
#define FTDMAC020_EA_ABT_CH(x)		(1 << ((x) + 16))

/*
 * Main configuration status register
 */
#define FTDMAC020_CR_ENABLE		(1 << 0)
#define FTDMAC020_CR_M0_BE		(1 << 1)	/* master 0 big endian */
#define FTDMAC020_CR_M1_BE		(1 << 2)	/* master 1 big endian */

/*
 * Channel control register
 */
#define FTDMAC020_CCR_ENABLE		(1 << 0)
#define FTDMAC020_CCR_DST_M1		(1 << 1)
#define FTDMAC020_CCR_SRC_M1		(1 << 2)
#define FTDMAC020_CCR_DST_INC		(0x0 << 3)
#define FTDMAC020_CCR_DST_DEC		(0x1 << 3)
#define FTDMAC020_CCR_DST_FIXED		(0x2 << 3)
#define FTDMAC020_CCR_SRC_INC		(0x0 << 5)
#define FTDMAC020_CCR_SRC_DEC		(0x1 << 5)
#define FTDMAC020_CCR_SRC_FIXED		(0x2 << 5)
#define FTDMAC020_CCR_HANDSHAKE		(1 << 7)
#define FTDMAC020_CCR_DST_WIDTH_8	(0x0 << 8)
#define FTDMAC020_CCR_DST_WIDTH_16	(0x1 << 8)
#define FTDMAC020_CCR_DST_WIDTH_32	(0x2 << 8)
#define FTDMAC020_CCR_DST_WIDTH_64	(0x3 << 8)
#define FTDMAC020_CCR_SRC_WIDTH_8	(0x0 << 11)
#define FTDMAC020_CCR_SRC_WIDTH_16	(0x1 << 11)
#define FTDMAC020_CCR_SRC_WIDTH_32	(0x2 << 11)
#define FTDMAC020_CCR_SRC_WIDTH_64	(0x3 << 11)
#define FTDMAC020_CCR_ABORT		(1 << 15)
#define FTDMAC020_CCR_BURST_1		(0x0 << 16)
#define FTDMAC020_CCR_BURST_4		(0x1 << 16)
#define FTDMAC020_CCR_BURST_8		(0x2 << 16)
#define FTDMAC020_CCR_BURST_16		(0x3 << 16)
#define FTDMAC020_CCR_BURST_32		(0x4 << 16)
#define FTDMAC020_CCR_BURST_64		(0x5 << 16)
#define FTDMAC020_CCR_BURST_128		(0x6 << 16)
#define FTDMAC020_CCR_BURST_256		(0x7 << 16)
#define FTDMAC020_CCR_PRIVILEGED	(1 << 19)
#define FTDMAC020_CCR_BUFFERABLE	(1 << 20)
#define FTDMAC020_CCR_CACHEABLE		(1 << 21)
#define FTDMAC020_CCR_PRIO_0		(0x0 << 22)
#define FTDMAC020_CCR_PRIO_1		(0x1 << 22)
#define FTDMAC020_CCR_PRIO_2		(0x2 << 22)
#define FTDMAC020_CCR_PRIO_3		(0x2 << 22)
#define FTDMAC020_CCR_FIFOTH_1		(0x0 << 24)
#define FTDMAC020_CCR_FIFOTH_2		(0x1 << 24)
#define FTDMAC020_CCR_FIFOTH_4		(0x2 << 24)
#define FTDMAC020_CCR_FIFOTH_8		(0x3 << 24)
#define FTDMAC020_CCR_FIFOTH_16		(0x4 << 24)
#define FTDMAC020_CCR_MASK_TC		(1 << 31)

/*
 * Channel configuration register
 */
#define FTDMAC020_CFG_MASK_TCI		(1 << 0)	/* mask tc interrupt */
#define FTDMAC020_CFG_MASK_EI		(1 << 1)	/* mask error interrupt */
#define FTDMAC020_CFG_MASK_AI		(1 << 2)	/* mask abort interrupt */
#define FTDMAC020_CFG_SRC_HANDSHAKE(x)	(((x) & 0xf) << 3)
#define FTDMAC020_CFG_SRC_HANDSHAKE_EN	(1 << 7)
#define FTDMAC020_CFG_BUSY		(1 << 8)
#define FTDMAC020_CFG_DST_HANDSHAKE(x)	(((x) & 0xf) << 9)
#define FTDMAC020_CFG_DST_HANDSHAKE_EN	(1 << 13)
#define FTDMAC020_CFG_LLP_CNT(cfg)	(((cfg) >> 16) & 0xf)

/*
 * Link list descriptor pointer
 */
#define FTDMAC020_LLP_M1		(1 << 0)
#define FTDMAC020_LLP_ADDR(a)		((a) & ~0x3)

/*
 * Transfer size register
 */
#define FTDMAC020_CYC_MASK		0x3fffff

/**
 * struct ftdmac020_lld - hardware link list descriptor.
 * @src: source physical address
 * @dst: destination physical addr
 * @next: phsical address to the next link list descriptor
 * @ctrl: control field
 * @cycle: transfer size
 *
 * should be word aligned
 */
struct ftdmac020_lld {
	dma_addr_t src;
	dma_addr_t dst;
	dma_addr_t next;
	unsigned int ctrl;
	unsigned int cycle;
};

#define FTDMAC020_LLD_CTRL_DST_M1	(1 << 16)
#define FTDMAC020_LLD_CTRL_SRC_M1	(1 << 17)
#define FTDMAC020_LLD_CTRL_DST_INC	(0x0 << 18)
#define FTDMAC020_LLD_CTRL_DST_DEC	(0x1 << 18)
#define FTDMAC020_LLD_CTRL_DST_FIXED	(0x2 << 18)
#define FTDMAC020_LLD_CTRL_SRC_INC	(0x0 << 20)
#define FTDMAC020_LLD_CTRL_SRC_DEC	(0x1 << 20)
#define FTDMAC020_LLD_CTRL_SRC_FIXED	(0x2 << 20)
#define FTDMAC020_LLD_CTRL_DST_WIDTH_8	(0x0 << 22)
#define FTDMAC020_LLD_CTRL_DST_WIDTH_16	(0x1 << 22)
#define FTDMAC020_LLD_CTRL_DST_WIDTH_32	(0x2 << 22)
#define FTDMAC020_LLD_CTRL_DST_WIDTH_64	(0x3 << 22)
#define FTDMAC020_LLD_CTRL_SRC_WIDTH_8	(0x0 << 25)
#define FTDMAC020_LLD_CTRL_SRC_WIDTH_16	(0x1 << 25)
#define FTDMAC020_LLD_CTRL_SRC_WIDTH_32	(0x2 << 25)
#define FTDMAC020_LLD_CTRL_SRC_WIDTH_64	(0x3 << 25)
#define FTDMAC020_LLD_CTRL_MASK_TC	(1 << 28)
#define FTDMAC020_LLD_CTRL_FIFOTH_1	(0x0 << 29)
#define FTDMAC020_LLD_CTRL_FIFOTH_2	(0x1 << 29)
#define FTDMAC020_LLD_CTRL_FIFOTH_4	(0x2 << 29)
#define FTDMAC020_LLD_CTRL_FIFOTH_8	(0x3 << 29)
#define FTDMAC020_LLD_CTRL_FIFOTH_16	(0x4 << 29)

#define FTDMAC020_LLD_CYCLE_MASK	0x3fffff

enum ftdmac020_channels {
	FTDMAC020_CHANNEL_0 = (1 << 0),
	FTDMAC020_CHANNEL_1 = (1 << 1),
	FTDMAC020_CHANNEL_2 = (1 << 2),
	FTDMAC020_CHANNEL_3 = (1 << 3),
	FTDMAC020_CHANNEL_4 = (1 << 4),
	FTDMAC020_CHANNEL_5 = (1 << 5),
	FTDMAC020_CHANNEL_6 = (1 << 6),
	FTDMAC020_CHANNEL_7 = (1 << 7),
	FTDMAC020_CHANNEL_ALL = 0xff,
};

/**
 * struct ftdmac020_dma_slave - DMA slave data
 * @common: physical address and register width...
 * @id: specify which ftdmac020 device to use, -1 for wildcard
 * @channels: bitmap of usable DMA channels
 * @handshake: hardware handshake number, -1 to disable handshake mode
 */
struct ftdmac020_dma_slave {
	struct dma_slave_config common;
	int id;
	enum ftdmac020_channels channels;
	int handshake;
};

/**
 * ftdmac020_chan_filter() - filter function for dma_request_channel().
 * @chan: DMA channel
 * @data: pointer to ftdmac020_dma_slave
 */
bool ftdmac020_chan_filter(struct dma_chan *chan, void *data);

#endif	/* __FTDMAC020_H */
