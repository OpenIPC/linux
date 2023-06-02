#ifndef _FH_AXI_DMA_ADAPT_H
#define _FH_AXI_DMA_ADAPT_H

#include <linux/string.h>
#include <linux/timer.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/semaphore.h>
#include <linux/io.h>
#include <linux/irqreturn.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_mdio.h>
#include <linux/of_platform.h>
#include <asm/irq.h>
#include <linux/dmaengine.h>
#include "dmaengine.h"
#include <linux/fh_dmac.h>
#include <mach/fh_dma_plat.h>


enum {
	AXI_DMA_RET_OK    = 0,
	AXI_DMA_RET_NO_MEM   = 1,
};

enum {
	AXI_DMA_CTRL_DMA_OPEN    = 0,
	AXI_DMA_CTRL_DMA_CLOSE,
	AXI_DMA_CTRL_DMA_REQUEST_CHANNEL,
	AXI_DMA_CTRL_DMA_RELEASE_CHANNEL,
	AXI_DMA_CTRL_DMA_SINGLE_TRANSFER,
	AXI_DMA_CTRL_DMA_CYCLIC_PREPARE,
	AXI_DMA_CTRL_DMA_CYCLIC_START,
	AXI_DMA_CTRL_DMA_CYCLIC_STOP,
	AXI_DMA_CTRL_DMA_CYCLIC_FREE,
	AXI_DMA_CTRL_DMA_PAUSE,
	AXI_DMA_CTRL_DMA_RESUME,
	AXI_DMA_CTRL_DMA_GET_DAR,
	AXI_DMA_CTRL_DMA_GET_SAR,
};

#define FH_CHANNEL_MAX_TRANSFER_SIZE (4096)
#define MASTER_SEL_ENABLE    0x55
#ifdef CONFIG_CHANNEL_ALLOC_MEM_CLASSICS
#define DESC_MAX_SIZE	CONFIG_CHANNEL_ALLOC_DESC_NUM
#else
#define DESC_MAX_SIZE	128
#endif
#define DMA_CONTROLLER_NUMBER   1

#define DMA_COMMON_ISR_OPEN     1
#define DMA_COMMON_ISR_CLOSE    0
#define DW_DMA_SLAVE_WIDTH_8BIT (0)
#define DW_DMA_SLAVE_WIDTH_16BIT (1)
#define DW_DMA_SLAVE_WIDTH_32BIT (2)


/* the user should reference the hw handshaking watermark.. */
#define DW_DMA_SLAVE_MSIZE_1 (0)
#define DW_DMA_SLAVE_MSIZE_4 (1)
#define DW_DMA_SLAVE_MSIZE_8 (2)
#define DW_DMA_SLAVE_MSIZE_16 (3)
#define DW_DMA_SLAVE_MSIZE_32 (4)
#define DW_DMA_SLAVE_MSIZE_64 (5)
#define DW_DMA_SLAVE_MSIZE_128 (6)
#define DW_DMA_SLAVE_MSIZE_256 (7)


#define DW_DMA_SLAVE_INC (0)
#define DW_DMA_SLAVE_DEC (1)
#define DW_DMA_SLAVE_FIX (2)
#define DMA_M2M (0)       /* MEM <=> MEM */
#define DMA_M2P (1)       /* MEM => peripheral A */
#define DMA_P2M (2)       /* MEM <= peripheral A */
#define DMA_P2P (3)       /* peripheral A <=> peripheral B */
#define AUTO_FIND_CHANNEL (0xff)
#define DMA_HW_HANDSHAKING (0)
#define DMA_SW_HANDSHAKING (1)
#define ADDR_RELOAD      (0x55)

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


typedef void (*dma_complete_callback)(void *complete_para);


/* controller private para... */
struct fh_axi_dma;
struct dma_transfer;

struct axi_dma_ops {
	void (*axi_dma_isr_process)(struct fh_axi_dma *param);
	void (*axi_dma_isr_enable_set)(struct fh_axi_dma *param,
		FH_UINT32 enable);
	FH_SINT32(*axi_dma_control)(struct fh_axi_dma *dma,
		FH_UINT32 cmd, struct dma_transfer *arg);
	void (*axi_dma_get_desc_para)(struct fh_axi_dma *dma,
		struct dma_transfer *p_transfer,
		FH_UINT32 *desc_size, FH_UINT32 *base_phy_add,
		FH_UINT32 *each_desc_size, FH_UINT32 *each_desc_cap);
};

struct axi_dma_platform_data {
	FH_UINT32 id;
	FH_UINT8 *name;
	//FH_UINT32 irq;
	FH_UINT32 base;
	FH_UINT32 channel_max_number;
	//bind to kernel pri
	void *kernel_pri;
	void *adapt_pri;
};

struct dma_transfer_desc {
	FH_UINT32 src_width;
	FH_UINT32 src_msize;
	FH_UINT32 src_add;
	FH_UINT32 src_inc_mode;

	FH_UINT32 dst_width;
	FH_UINT32 dst_msize;
	FH_UINT32 dst_add;
	FH_UINT32 dst_inc_mode;

	FH_UINT32 size;
};
/* transfer use below */
struct dma_transfer {
	axi_dma_list transfer_list;
	struct fh_axi_dma *dma_controller;
	void *first_lli;
	FH_UINT32 lli_size;
	/* new add for allign get desc... */
	FH_UINT32 actual_lli_size;
	FH_UINT32 channel_number;
	/* which dma you want to use...for fh81....only 0!!! */
	FH_UINT32 dma_number;
	FH_UINT32 fc_mode;  /* ip->mem. mem->mem. mem->ip */
	FH_UINT32 src_hs;  /* src */
	FH_UINT32 src_per;  /* src hw handshake number */
	FH_UINT32 dst_hs;  /* src */
	FH_UINT32 dst_per;
	FH_UINT32 trans_len;
	/* this is used when dma finish transfer job */
	dma_complete_callback complete_callback;
	void *complete_para;  /* for the driver data use the dma driver. */
	/* private para.. */
	void (*prepare_callback)(void *p);
	void *prepare_para;

	void (*isr_prepare_callback)(void *p);
	void *isr_prepare_para;

	FH_UINT32 period_len;
	FH_UINT32 master_flag;
	FH_UINT32 src_master;
	FH_UINT32 dst_master;
	FH_UINT32 cyclic_periods;

	struct dma_transfer_desc *p_desc;
	FH_UINT32 desc_size;

};


struct fh_axi_dma_channel_adapt {
	struct dma_chan		core_chan;
	struct dma_transfer trans;
	struct fh_dma_extra	ext_para;
	struct dma_async_tx_descriptor	*ch_adapt_desc_base;
	struct dma_transfer_desc *desc_head;
	u32 desc_size;
	struct dma_slave_config sconfig;
	struct fh_cyclic_desc cyclic;
	spinlock_t		lock;
};

struct fh_axi_dma_adapt {
	struct dma_device	dma;
	struct tasklet_struct	tasklet;
	struct clk	*clk;
	//bind to driver data. help to find the driver handle
	struct fh_axi_dma *driver_pri;
	struct platform_device *pdev;
	struct fh_axi_dma_channel_adapt chan[0];

};

FH_UINT32 cal_axi_dma_channel(void *regs);
struct axi_dma_ops *get_fh_axi_dma_ops(struct fh_axi_dma *p_axi_dma);
struct fh_axi_dma *fh_axi_dma_probe(struct axi_dma_platform_data *priv_data);

#endif
