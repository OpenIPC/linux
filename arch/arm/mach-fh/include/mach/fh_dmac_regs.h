/*
 * Driver for the Synopsys DesignWare AHB DMA Controller
 *
 * Copyright (C) 2005-2007 Atmel Corporation
 * Copyright (C) 2010-2011 ST Microelectronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/dmaengine.h>
#include <mach/fh_dmac.h>

#define FH_DMA_MAX_NR_CHANNELS	8
#define FH_DMA_MAX_NR_REQUESTS	16

/*
 * Redefine this macro to handle differences between 32- and 64-bit
 * addressing, big vs. little endian, etc.
 */
#define FH_REG(name)		u32 name; u32 __pad_##name

/* Hardware register definitions. */
struct fh_dma_chan_regs {
	FH_REG(SAR);		/* Source Address Register */
	FH_REG(DAR);		/* Destination Address Register */
	FH_REG(LLP);		/* Linked List Pointer */
	u32	CTL_LO;		/* Control Register Low */
	u32	CTL_HI;		/* Control Register High */
	FH_REG(SSTAT);
	FH_REG(DSTAT);
	FH_REG(SSTATAR);
	FH_REG(DSTATAR);
	u32	CFG_LO;		/* Configuration Register Low */
	u32	CFG_HI;		/* Configuration Register High */
	FH_REG(SGR);
	FH_REG(DSR);
};

struct fh_dma_irq_regs {
	FH_REG(XFER);
	FH_REG(BLOCK);
	FH_REG(SRC_TRAN);
	FH_REG(DST_TRAN);
	FH_REG(ERROR);
};

struct fh_dma_regs {
	/* per-channel registers */
	struct fh_dma_chan_regs	CHAN[FH_DMA_MAX_NR_CHANNELS];

	/* irq handling */
	struct fh_dma_irq_regs	RAW;		/* r */
	struct fh_dma_irq_regs	STATUS;		/* r (raw & mask) */
	struct fh_dma_irq_regs	MASK;		/* rw (set = irq enabled) */
	struct fh_dma_irq_regs	CLEAR;		/* w (ack, affects "raw") */

	FH_REG(STATUS_INT);			/* r */

	/* software handshaking */
	FH_REG(REQ_SRC);
	FH_REG(REQ_DST);
	FH_REG(SGL_REQ_SRC);
	FH_REG(SGL_REQ_DST);
	FH_REG(LAST_SRC);
	FH_REG(LAST_DST);

	/* miscellaneous */
	FH_REG(CFG);
	FH_REG(CH_EN);
	FH_REG(ID);
	FH_REG(TEST);

	/* reserved */
	FH_REG(__reserved0);
	FH_REG(__reserved1);

	/* optional encoded params, 0x3c8..0x3f7 */
	u32	__reserved;

	/* per-channel configuration registers */
	u32	FHC_PARAMS[FH_DMA_MAX_NR_CHANNELS];
	u32	MULTI_BLK_TYPE;
	u32	MAX_BLK_SIZE;

	/* top-level parameters */
	u32	FH_PARAMS;
};

#ifdef CONFIG_FH_DMAC_BIG_ENDIAN_IO
#define dma_readl_native ioread32be
#define dma_writel_native iowrite32be
#else
#define dma_readl_native readl
#define dma_writel_native writel
#endif

/* To access the registers in early stage of probe */
#define dma_read_byaddr(addr, name) \
	dma_readl_native((addr) + offsetof(struct fh_dma_regs, name))

/* Bitfields in FH_PARAMS */
#define FH_PARAMS_NR_CHAN	8		/* number of channels */
#define FH_PARAMS_NR_MASTER	11		/* number of AHB masters */
#define FH_PARAMS_DATA_WIDTH(n)	(15 + 2 * (n))
#define FH_PARAMS_DATA_WIDTH1	15		/* master 1 data width */
#define FH_PARAMS_DATA_WIDTH2	17		/* master 2 data width */
#define FH_PARAMS_DATA_WIDTH3	19		/* master 3 data width */
#define FH_PARAMS_DATA_WIDTH4	21		/* master 4 data width */
#define FH_PARAMS_EN		28		/* encoded parameters */

/* Bitfields in FHC_PARAMS */
#define FHC_PARAMS_MBLK_EN	11		/* multi block transfer */

/* Bitfields in CTL_LO */
#define FHC_CTLL_INT_EN		(1 << 0)	/* irqs enabled? */
#define FHC_CTLL_DST_WIDTH(n)	((n)<<1)	/* bytes per element */
#define FHC_CTLL_SRC_WIDTH(n)	((n)<<4)
#define FHC_CTLL_DST_INC	(0<<7)		/* DAR update/not */
#define FHC_CTLL_DST_DEC	(1<<7)
#define FHC_CTLL_DST_FIX	(2<<7)
#define FHC_CTLL_SRC_INC	(0<<9)		/* SAR update/not */
#define FHC_CTLL_SRC_DEC	(1<<9)
#define FHC_CTLL_SRC_FIX	(2<<9)
#define FHC_CTLL_DST_MSIZE(n)	((n)<<11)	/* burst, #elements */
#define FHC_CTLL_SRC_MSIZE(n)	((n)<<14)
#define FHC_CTLL_S_GATH_EN	(1 << 17)	/* src gather, !FIX */
#define FHC_CTLL_D_SCAT_EN	(1 << 18)	/* dst scatter, !FIX */
#define FHC_CTLL_FC(n)		((n) << 20)
#define FHC_CTLL_FC_M2M		(0 << 20)	/* mem-to-mem */
#define FHC_CTLL_FC_M2P		(1 << 20)	/* mem-to-periph */
#define FHC_CTLL_FC_P2M		(2 << 20)	/* periph-to-mem */
#define FHC_CTLL_FC_P2P		(3 << 20)	/* periph-to-periph */
/* plus 4 transfer types for peripheral-as-flow-controller */
#define FHC_CTLL_DMS(n)		((n)<<23)	/* dst master select */
#define FHC_CTLL_SMS(n)		((n)<<25)	/* src master select */
#define FHC_CTLL_LLP_D_EN	(1 << 27)	/* dest block chain */
#define FHC_CTLL_LLP_S_EN	(1 << 28)	/* src block chain */

/* Bitfields in CTL_HI */
#define FHC_CTLH_DONE		0x00001000
#define FHC_CTLH_BLOCK_TS_MASK	0x00000fff
#define FHC_PROTCTL_MASK	(7 << 2)
#define FHC_PROTCTL(n)	((n) << 2)
/* Bitfields in CFG_LO. Platform-configurable bits are in <linux/fh_dmac.h> */
#define FHC_CFGL_CH_PRIOR_MASK	(0x7 << 5)	/* priority mask */
#define FHC_CFGL_CH_PRIOR(x)	((x) << 5)	/* priority */
#define FHC_CFGL_CH_SUSP	(1 << 8)	/* pause xfer */
#define FHC_CFGL_FIFO_EMPTY	(1 << 9)	/* pause xfer */
#define FHC_CFGL_HS_DST		(1 << 10)	/* handshake w/dst */
#define FHC_CFGL_HS_SRC		(1 << 11)	/* handshake w/src */
#define FHC_CFGL_MAX_BURST(x)	((x) << 20)
#define FHC_CFGL_RELOAD_SAR	(1 << 30)
#define FHC_CFGL_RELOAD_DAR	(1 << 31)

/* Bitfields in CFG_HI. Platform-configurable bits are in <linux/fh_dmac.h> */
#define FHC_CFGH_DS_UPD_EN	(1 << 5)
#define FHC_CFGH_SS_UPD_EN	(1 << 6)

/* Bitfields in SGR */
#define FHC_SGR_SGI(x)		((x) << 0)
#define FHC_SGR_SGC(x)		((x) << 20)

/* Bitfields in DSR */
#define FHC_DSR_DSI(x)		((x) << 0)
#define FHC_DSR_DSC(x)		((x) << 20)

/* Bitfields in CFG */
#define FH_CFG_DMA_EN		(1 << 0)

#define FH_REGLEN       0x400
#define PROTCTL_ENABLE	0x55
#define MASTER_SEL_ENABLE	0x55

enum fh_dmac_flags {
	FH_DMA_IS_CYCLIC = 0,
	FH_DMA_IS_SOFT_LLP = 1,
};

struct fh_dma_extra {
	u32 sinc;
	u32 dinc;
	u32 protctl_flag;
	u32 protctl_data;
	u32 master_flag;
	u32 src_master;
	u32 dst_master;
};

struct fh_dma_chan {
	struct dma_chan			chan;
	void __iomem			*ch_regs;
	u8				mask;
	u8				priority;
	enum dma_transfer_direction	direction;
	bool				paused;
	bool				initialized;

	/* software emulation of the LLP transfers */
	struct list_head	*tx_node_active;

	spinlock_t		lock;

	/* these other elements are all protected by lock */
	unsigned long		flags;
	struct list_head	active_list;
	struct list_head	queue;
	struct list_head	free_list;
	u32			residue;
	struct fh_cyclic_desc	*cdesc;

	unsigned int		descs_allocated;

	/* hardware configuration */
	unsigned int		block_size;
	bool			nollp;

	/* custom slave configuration */
	unsigned int		request_line;
	unsigned char		src_master;
	unsigned char		dst_master;
	struct fh_dma_extra	ext_para;
	/* configuration passed via DMA_SLAVE_CONFIG */
	struct dma_slave_config dma_sconfig;
};

enum fh_dma_slave_increment {
	FH_DMA_SLAVE_INC,
	FH_DMA_SLAVE_DEC,
	FH_DMA_SLAVE_FIX,
};


static inline struct fh_dma_chan_regs __iomem *
__fhc_regs(struct fh_dma_chan *fhc)
{
	return fhc->ch_regs;
}

#define channel_readl(fhc, name) \
	dma_readl_native(&(__fhc_regs(fhc)->name))
#define channel_writel(fhc, name, val) \
	dma_writel_native((val), &(__fhc_regs(fhc)->name))

static inline struct fh_dma_chan *to_fh_dma_chan(struct dma_chan *chan)
{
	return container_of(chan, struct fh_dma_chan, chan);
}

struct fh_dma {
	struct dma_device	dma;
	void __iomem		*regs;
	struct dma_pool		*desc_pool;
	struct tasklet_struct	tasklet;
	struct clk		*clk;

	u8			all_chan_mask;

	/* hardware configuration */
	unsigned char		nr_masters;
	unsigned char		data_width[4];

	struct fh_dma_chan	chan[0];
};

static inline struct fh_dma_regs __iomem *__fh_regs(struct fh_dma *dw)
{
	return dw->regs;
}

#define dma_readl(dw, name) \
	dma_readl_native(&(__fh_regs(dw)->name))
#define dma_writel(dw, name, val) \
	dma_writel_native((val), &(__fh_regs(dw)->name))

#define channel_set_bit(dw, reg, mask) \
	dma_writel(dw, reg, ((mask) << 8) | (mask))
#define channel_clear_bit(dw, reg, mask) \
	dma_writel(dw, reg, ((mask) << 8) | 0)

static inline struct fh_dma *to_fh_dma(struct dma_device *ddev)
{
	return container_of(ddev, struct fh_dma, dma);
}

/* LLI == Linked List Item; a.k.a. DMA block descriptor */
struct fh_lli {
	/* values that are not changed by hardware */
	u32		sar;
	u32		dar;
	u32		llp;		/* chain to next lli */
	u32		ctllo;
	/* values that may get written back: */
	u32		ctlhi;
	/* sstat and dstat can snapshot peripheral register state.
	 * silicon config may discard either or both...
	 */
	u32		sstat;
	u32		dstat;
};

struct fh_desc {
	/* FIRST values the hardware uses */
	struct fh_lli			lli;

	/* THEN values for driver housekeeping */
	struct list_head		desc_node;
	struct list_head		tx_list;
	struct dma_async_tx_descriptor	txd;
	size_t				len;
	size_t				total_len;
};

#define to_fh_desc(h)	list_entry(h, struct fh_desc, desc_node)

static inline struct fh_desc *
txd_to_fh_desc(struct dma_async_tx_descriptor *txd)
{
	return container_of(txd, struct fh_desc, txd);
}
