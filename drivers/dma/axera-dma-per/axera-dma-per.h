/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AXERA_DMA_PER_H__
#define __AXERA_DMA_PER_H__

#include <linux/bitops.h>
#include <linux/device.h>
#include <linux/dmaengine.h>
#include <linux/dmapool.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/io-64-nonatomic-lo-hi.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_dma.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/property.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/of_device.h>
#include <linux/clk.h>
#include <linux/reset.h>

#include "../dmaengine.h"
#include "../virt-dma.h"

#define AX_DMAPER_BASE                            (0)

#define AX_DMAPER_CHN_EN                          (0x0)
#define AX_DMAPER_CHN_MASK                        (0xFFFF)
#define AX_DMAPER_CTRL                            (0x4)
#define AX_DMAPER_LLI_RD_PREFETCH_SHIFT           (17)
#define AX_DMAPER_LLI_RD_PREFETCH_MASK            (1)
#define AX_DMAPER_ALL_SUSPEND_EN_SHIFT            (16)
#define AX_DMAPER_ALL_SUSPEND_EN_MASK             (1)
#define AX_DMAPER_CHN_SUSPEND_EN_MASK             (1)
#define AX_DMAPER_START                           (0xC)
#define AX_DMAPER_STA                             (0x10)
#define AX_DMAPER_ALL_SUSPEND_STA_SHIFT           (19)
#define AX_DMAPER_ALL_SUSPEND_STA_MASK            (1)
#define AX_DMAPER_CHN_SUSPEND_STA_MASK            (1)
#define AX_DMAPER_TIMEOUT_REQ                     (0x14)
#define AX_DMAPER_TIMEOUT_RD                      (0x18)
#define AX_DMAPER_REQ_TIMEMOUT_CHN_SEL_SHIFT      (5)
#define AX_DMAPER_REQ_TIMEMOUT_CHN_SEL_MASK       (0xF)
#define AX_DMAPER_TIMEMOUT_BYPASS_SHIFT           (4)
#define AX_DMAPER_TIMEMOUT_BYPASS_MASK            (1)
#define AX_DMAPER_TIMEMOUT_LVL_SEL_SHIFT          (0)
#define AX_DMAPER_TIMEMOUT_LVL_MASK               (0xF)
#define AX_DMAPER_CHN_PRIORITY                    (0x1C)
#define AX_DMAPER_CHN_LLI_H(x)                    (0x20 + (unsigned)x * 0x8)
#define AX_DMAPER_CHN_LLI_L(x)                    (0x24 + (unsigned)x * 0x8)
#define AX_DMAPER_CLEAR                           (0xA0)
#define AX_DMAPER_TOTAL_CLR_EN_SHIFT              (16)
#define AX_DMAPER_TOTAL_CLR_EN_MASK               (1)
#define AX_DMAPER_CHN_CLR_EN_MASK                 (1)

#define AX_DMAPER_HIGH_PERF             (0xA4)
#define AX_DMAPER_INT_GLB_MASK          (0xD8)
#define AX_DMAPER_INT_GLB_RAW           (0xDC)
#define AX_DMAPER_INT_GLB_STA           (0xE0)
#define AX_DMAPER_INT_RESP_ERR_MASK     (0xE4)
#define AX_DMAPER_INT_RESP_ERR_CLR      (0xE8)
#define AX_DMAPER_INT_RESP_ERR_RAW      (0xEC)
#define AX_DMAPER_INT_RESP_ERR_STA      (0xF0)
#define AX_DMAPER_INT_MASK(x)           (0xF4 + (unsigned)x * 0x10)
#define AX_DMAPER_INT_CLR(x)            (0xF8 + (unsigned)x * 0x10)
#define AX_DMAPER_INT_RAW(x)            (0xFC + (unsigned)x * 0x10)
#define AX_DMAPER_INT_STA(x)            (0x100 + (unsigned)x * 0x10)
#define AX_DMAPER_SRC_ADDR_H(x)         (0x1F4 + (unsigned)x * 0x10)
#define AX_DMAPER_SRC_ADDR_L(x)         (0x1F8 + (unsigned)x * 0x10)
#define AX_DMAPER_DST_ADDR_H(x)         (0x1FC + (unsigned)x * 0x10)
#define AX_DMAPER_DST_ADDR_L(x)         (0x200 + (unsigned)x * 0x10)

#define AX_DMA_REQ_SEL_H_SET            (0x130)
#define AX_DMA_REQ_SEL_H_CLR            (0x134)
#define AX_DMA_REQ_SEL_L_SET            (0x138)
#define AX_DMA_REQ_SEL_L_CLR            (0x13c)

#define AX_DMA_REQ_HS_SEL_SET(x)        (0x158 - (unsigned)x * 0x8)
#define AX_DMA_REQ_HS_SEL_CLR(x)        (0x15c - (unsigned)x * 0x8)

#define AX_DMA_PER_DRV                  "dma_per"

#define AX_DMA_PER_MAX_CHANNELS         (16)
#define AX_DMA_PER_MAX_BLK_SIZE         (0x3ffff0)

#define AXI_DMA_BUSWIDTHS		  \
	(BIT(DMA_SLAVE_BUSWIDTH_1_BYTE)	| \
	BIT(DMA_SLAVE_BUSWIDTH_2_BYTES)	| \
	BIT(DMA_SLAVE_BUSWIDTH_4_BYTES)	| \
	BIT(DMA_SLAVE_BUSWIDTH_8_BYTES))

#define AXIDMA_TRANS_WIDTH_MAX		DMA_SLAVE_BUSWIDTH_8_BYTES

enum {
	AXIDMA_ARWLEN_8			= 8,
	AXIDMA_ARWLEN_16		= 16,
	AXIDMA_ARWLEN_32		= 32,
	AXIDMA_ARWLEN_MIN		= AXIDMA_ARWLEN_8,
	AXIDMA_ARWLEN_MAX		= AXIDMA_ARWLEN_32
};

struct ax_dma_per_lli_ctrl {
	u64 vld:1;
	u64 last:1;
	u64 ioc:1;
	u64 endian:2;
	u64 src_per:1;
	u64 awlen:2;
	u64 arlen:2;
	u64 awport:3;
	u64 arport:3;
	u64 awcache:4;
	u64 arcache:4;
	u64 dst_tr_wridth:3;
	u64 src_tr_wridth:3;
	u64 dinc:1;
	u64 sinc:1;
	u64 dst_osd:4;
	u64 src_osd:4;
	u64 hs_sel_dst:1;
	u64 hs_sel_src:1;
	u64 dst_msize:10;
	u64 src_msize:10;
	u64 :2;
};

struct ax_dma_per_lli_data {
	u64 llp:37;
	u64 block_ts:22;
	u64 :5;
};

struct axi_dma_lli {
	u64 				sar;
	u64				dar;
	struct ax_dma_per_lli_ctrl	ctrl;
	struct ax_dma_per_lli_data	data;
};

struct axi_dma_hw_desc {
	struct axi_dma_lli	*lli;
	dma_addr_t		llp;
	u32			len;
};

struct axi_dma_desc {
	struct axi_dma_hw_desc		*hw_desc;
	struct virt_dma_desc		vd;
	struct axi_dma_chan		*chan;
	u32				completed_blocks;
	u32				length;
	u32				period_len;
	atomic_t			descs_allocated;
	int				high_perf;
	int				cur_llp_index;
};

struct axi_dma_hw_info {
	u32	nr_channels;
	u32	nr_masters;
	u32	data_width;
	u32	block_size;
	/* maximum supported axi burst length */
	u32	axi_rw_burst_len;
	bool	restrict_axi_burst_len;
};

struct axi_dma_chan {
	struct ax_dma_per_chip		*chip;
	u8				id;
	u8				of_dma_handshake_num;
	u8				use_of_dma;
	struct virt_dma_chan		vc;
	struct axi_dma_desc		*desc;
	struct dma_slave_config		config;
	enum dma_transfer_direction	direction;
	bool				cyclic;
	/* these other elements are all protected by vc.lock */
	bool				is_paused;
};

struct ax_dma_per_info {
	struct dma_device		dma;
	struct axi_dma_hw_info		*hinfo;
	struct device_dma_parameters	dma_parms;
	/* channels */
	struct axi_dma_chan		*chans;
};

struct ax_dma_per_chip {
	struct device		*dev;
	int			irq;
#ifdef CONFIG_AX_RISCV_LOAD_ROOTFS
	int			en;
#endif
	phys_addr_t paddr;
	phys_addr_t req_paddr;
	void __iomem		*regs;
	void __iomem		*req_regs;
	struct reset_control	*rst;
	struct reset_control	*prst;
	struct clk		*clk;
	struct clk		*pclk;
	struct ax_dma_per_info	*axdma;
	struct dma_pool		*desc_pool;
	struct kmem_cache	*desc_kmem;
	spinlock_t		lock;
};

enum {
	CHN_SUSPEND      = GENMASK(15, 0),
	AXI_LLI_RD_BUSY  = BIT(16),
	AXI_SRC_RD_BUSY  = BIT(17),
	AXI_WR_BUSY      = BIT(18),
	STA_SUSPEND      = BIT(19),
};
enum {
	INT_RESP_ERR      = BIT(16),
	RESP_ALL_ERR      = GENMASK(2, 0),
	DST_WR_RESP_ERR   = BIT(2),
	SRC_RD_RESP_ERR   = BIT(1),
	LLI_RD_RESP_ERR   = BIT(0),
	GLB_CHN_INT       = GENMASK(15, 0),
	CHN_INTR_ALL      = GENMASK(5, 0),
	CHN_AXI_WR_ERR    = BIT(5),
	CHN_AXI_RD_ERR    = BIT(4),
	CHN_LLI_ERR       = BIT(3),
	CHN_SUSPEND_INTR  = BIT(2),
	CHN_BLOCK_DONE    = BIT(1),
	CHN_TRANSF_DONE   = BIT(0),
};


static inline struct device *dchan2dev(struct dma_chan *dchan)
{
	return &dchan->dev->device;
}

static inline struct device *chan2dev(struct axi_dma_chan *chan)
{
	return &chan->vc.chan.dev->device;
}

static inline struct axi_dma_desc *vd_to_axi_desc(struct virt_dma_desc *vd)
{
	return container_of(vd, struct axi_dma_desc, vd);
}

static inline struct axi_dma_chan *vc_to_axi_dma_chan(struct virt_dma_chan *vc)
{
	return container_of(vc, struct axi_dma_chan, vc);
}

static inline struct axi_dma_chan *dchan_to_axi_dma_chan(struct dma_chan *dchan)
{
	return vc_to_axi_dma_chan(to_virt_chan(dchan));
}

void axi_chan_status_print(void);

#endif