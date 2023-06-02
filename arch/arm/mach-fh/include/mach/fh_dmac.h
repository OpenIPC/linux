/*
 * Driver for the Synopsys DesignWare DMA Controller (aka DMACA on
 * AVR32 systems.)
 *
 * Copyright (C) 2007 Atmel Corporation
 * Copyright (C) 2010-2011 ST Microelectronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef FH_DMAC_H
#define FH_DMAC_H

#include <linux/dmaengine.h>

/**
 * enum fh_dma_slave_width - DMA slave register access width.
 * @DMA_SLAVE_WIDTH_8BIT: Do 8-bit slave register accesses
 * @DMA_SLAVE_WIDTH_16BIT: Do 16-bit slave register accesses
 * @DMA_SLAVE_WIDTH_32BIT: Do 32-bit slave register accesses
 */
enum fh_dma_slave_width {
	FH_DMA_SLAVE_WIDTH_8BIT,
	FH_DMA_SLAVE_WIDTH_16BIT,
	FH_DMA_SLAVE_WIDTH_32BIT,
};

/* bursts size */
enum fh_dma_msize {
	FH_DMA_MSIZE_1,
	FH_DMA_MSIZE_4,
	FH_DMA_MSIZE_8,
	FH_DMA_MSIZE_16,
	FH_DMA_MSIZE_32,
	FH_DMA_MSIZE_64,
	FH_DMA_MSIZE_128,
	FH_DMA_MSIZE_256,
};

/* flow controller */
enum fh_dma_fc {
	FH_DMA_FC_D_M2M,
	FH_DMA_FC_D_M2P,
	FH_DMA_FC_D_P2M,
	FH_DMA_FC_D_P2P,
	FH_DMA_FC_P_P2M,
	FH_DMA_FC_SP_P2P,
	FH_DMA_FC_P_M2P,
	FH_DMA_FC_DP_P2P,
};

/**
 * struct fh_dma_slave - Controller-specific information about a slave
 *
 * @dma_dev: required DMA master device
 * @tx_reg: physical address of data register used for
 *	memory-to-peripheral transfers
 * @rx_reg: physical address of data register used for
 *	peripheral-to-memory transfers
 * @reg_width: peripheral register width
 * @cfg_hi: Platform-specific initializer for the CFG_HI register
 * @cfg_lo: Platform-specific initializer for the CFG_LO register
 * @src_master: src master for transfers on allocated channel.
 * @dst_master: dest master for transfers on allocated channel.
 * @src_msize: src burst size.
 * @dst_msize: dest burst size.
 * @fc: flow controller for DMA transfer
 */
struct fh_dma_slave {
	struct device		*dma_dev;
	dma_addr_t		tx_reg;
	dma_addr_t		rx_reg;
	enum fh_dma_slave_width	reg_width;
	u32			cfg_hi;
	u32			cfg_lo;
	u8			src_master;
	u8			dst_master;
	u8			src_msize;
	u8			dst_msize;
	u8			fc;
};


/**
 * struct fh_dma_platform_data - Controller configuration parameters
 * @nr_channels: Number of channels supported by hardware (max 8)
 * @is_private: The device channels should be marked as private and not for
 *	by the general purpose DMA channel allocator.
 * @chan_allocation_order: Allocate channels starting from 0 or 7
 * @chan_priority: Set channel priority increasing from 0 to 7 or 7 to 0.
 * @block_size: Maximum block size supported by the controller
 * @nr_masters: Number of AHB masters supported by the controller
 * @data_width: Maximum data width supported by hardware per AHB master
 *		(0 - 8bits, 1 - 16bits, ..., 5 - 256bits)
 * @sd: slave specific data. Used for configuring channels
 * @sd_count: count of slave data structures passed.
 */
struct fh_dma_platform_data {
	unsigned int	nr_channels;
	bool		is_private;
#define CHAN_ALLOCATION_ASCENDING	0	/* zero to seven */
#define CHAN_ALLOCATION_DESCENDING	1	/* seven to zero */
	unsigned char	chan_allocation_order;
#define CHAN_PRIORITY_ASCENDING		0	/* chan0 highest */
#define CHAN_PRIORITY_DESCENDING	1	/* chan7 highest */
	unsigned char	chan_priority;
	unsigned short	block_size;
	unsigned char	nr_masters;
	unsigned char	data_width[4];
};

/* Platform-configurable bits in CFG_HI */
#define FHC_CFGH_FCMODE		(1 << 0)
#define FHC_CFGH_FIFO_MODE	(1 << 1)
#define FHC_CFGH_PROTCTL(x)	((x) << 2)
#define FHC_CFGH_SRC_PER(x)	((x) << 7)
#define FHC_CFGH_DST_PER(x)	((x) << 11)

/* Platform-configurable bits in CFG_LO */
#define FHC_CFGL_LOCK_CH_XFER	(0 << 12)	/* scope of LOCK_CH */
#define FHC_CFGL_LOCK_CH_BLOCK	(1 << 12)
#define FHC_CFGL_LOCK_CH_XACT	(2 << 12)
#define FHC_CFGL_LOCK_BUS_XFER	(0 << 14)	/* scope of LOCK_BUS */
#define FHC_CFGL_LOCK_BUS_BLOCK	(1 << 14)
#define FHC_CFGL_LOCK_BUS_XACT	(2 << 14)
#define FHC_CFGL_LOCK_CH	(1 << 15)	/* channel lockout */
#define FHC_CFGL_LOCK_BUS	(1 << 16)	/* busmaster lockout */
#define FHC_CFGL_HS_DST_POL	(1 << 18)	/* dst handshake active low */
#define FHC_CFGL_HS_SRC_POL	(1 << 19)	/* src handshake active low */

/* DMA API extensions */
struct fh_cyclic_desc {
	struct fh_desc	**desc;
	unsigned long	periods;
	void	(*period_callback)(void *param);
	void		*period_callback_param;
};

struct fh_cyclic_desc *fh_dma_cyclic_prep(struct dma_chan *chan,
		dma_addr_t buf_addr, size_t buf_len, size_t period_len,
		enum dma_transfer_direction direction);
void fh_dma_cyclic_free(struct dma_chan *chan);
int fh_dma_cyclic_start(struct dma_chan *chan);
void fh_dma_cyclic_stop(struct dma_chan *chan);

dma_addr_t fh_dma_get_src_addr(struct dma_chan *chan);

dma_addr_t fh_dma_get_dst_addr(struct dma_chan *chan);

#endif /* FH_DMAC_H */
