/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __DMA_H__
#define __DMA_H__
#include <linux/err.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>
#include <linux/ioctl.h>
#include <linux/workqueue.h>
#include <linux/idr.h>
#include <linux/reset.h>
#include <asm/barrier.h>

#define DMA_TR_1B_MAXSIZE	(0x3fffE0)
#define DMA_TR_2B_MAXSIZE	(0x7fffE0)
#define DMA_TR_4B_MAXSIZE	(0x800000)

#define DMA_BASE		(0)
#define DMA_CH			(DMA_BASE + 0x0)
#define DMA_CTRL		(DMA_BASE + 0x4)
#define DMA_CTRL_TYPE_SHIFT	(1)
#define DMA_CTRL_TYPE_MASK	(3)
#define DMA_CTRL_SUSPEND_SHIFT	(0)
#define DMA_CTRL_SUSPEND_MASK	(1)
#define DMA_TRIG		(DMA_BASE + 0x8)
#define DMA_START		(DMA_BASE + 0xC)
#define DMA_LLI_BASE_H		(DMA_BASE + 0x14)
#define DMA_LLI_BASE_L		(DMA_BASE + 0x18)
#define DMA_CLEAR		(DMA_BASE + 0x1C)
#define DMA_INT_GLB_MASK	(DMA_BASE + 0xEC)
#define DMA_INT_GLB_RAW		(DMA_BASE + 0xF0)
#define DMA_INT_GLB_STA		(DMA_BASE + 0xF4)
#define DMA_INT_MASK(x)		(DMA_BASE + 0xF8 + (u32)x * 0x10)
#define DMA_INT_CLR(x)		(DMA_BASE + 0xFC + (u32)x * 0x10)
#define DMA_INT_RAW(x)		(DMA_BASE + 0x100 + (u32)x * 0x10)
#define DMA_INT_STA(x)		(DMA_BASE + 0x104 + (u32)x * 0x10)
#define DMA_CH_AUTO_GATE	(DMA_BASE + 0x200)
//RO
#define DMA_STA			(DMA_BASE + 0x10)
#define DMA_STA_SUSPEND_SHIFT	(3)
#define DMA_STA_SUSPEND_MASK	(1)
#define DMA_CHECKSUM		(DMA_BASE + 0x20)
// LLI_DATA2
#define EN_MASK			(0xFFFF)
#define EN_SHIFT		(42)
#define SRC_MSIZE_MASK		(0x3FF)
#define SRC_MSIZE_SHIFT		(32)
#define DAR_MSIZE_MASK		(0x3FF)
#define DAR_MSIZE_SHIFT		(22)
#define BLOCK_TS_MASK		(0x3FFFFF)
#define BLOCK_TS_SHIFT		(0)
// LLI_DATA3
#define IOC_MASK		(0x1)
#define IOC_SHIFT		(52)
#define TYPE_MASK		(0x7)
#define TYPE_SHIFT		(49)
#define XOR_NUM_MASK		(0xF)
#define XOR_NUM_SHIFT		(45)
#define CHKSUM_MASK		(0x1)
#define CHKSUM_SHIFT		(44)
#define ENDIAN_MASK		(0x3)
#define ENDIAN_SHIFT		(42)
#define WB_MASK			(0x1)
#define WB_SHIFT		(41)
#define LAST_MASK		(0x1)
#define LAST_SHIFT		(40)
#define LLI_PER_MASK		(0x3)
#define LLI_PER_SHIFT		(38)
#define AWLEN_MASK		(0x3)
#define AWLEN_SHIFT		(36)
#define ARLEN_MASK		(0x3)
#define ARLEN_SHIFT		(34)
#define AR_PROT_MASK		(0x7)
#define AR_PROT_SHIFT		(31)
#define AW_PROT_MASK		(0x7)
#define AW_PROT_SHIFT		(28)
#define AR_CACHE_MASK		(0xF)
#define AR_CACHE_SHIFT		(24)
#define AW_CACHE_MASK		(0xF)
#define AW_CACHE_SHIFT		(20)
#define ROSD_MASK		(0x3F)
#define ROSD_SHIFT		(14)
#define WOSD_MASK		(0x3F)
#define WOSD_SHIFT		(8)
#define SRC_TR_WIDTH_MASK	(0x7)
#define SRC_TR_WIDTH_SHIFT	(5)
#define DST_TR_WIDTH_MASK	(0x7)
#define DST_TR_WIDTH_SHIFT	(2)
#define LLI_DINC_SHIFT		(1)
#define LLI_DINC_MASK		(0x1)
#define LLI_SINC_SHIFT		(0)
#define LLI_SINC_MASK		(0x1)

#define AX_DMA_DEFAULT_CTRL 0x100029aaafff03

/* LLI == Linked List Item */
typedef struct {
	__le64 src;
	__le64 dst;
	__le64 size;
	__le64 ctrl;
	__le64 llp;
	__le32 dst_stride1;
	__le32 src_stride1;
	__le32 dst_stride2;
	__le32 src_stride2;
	__le32 dst_stride3;
	__le32 src_stride3;
	__le16 dst_ntile2;
	__le16 src_ntile2;
	__le16 dst_ntile1;
	__le16 src_ntile1;
	__le16 dst_ntile3;
	__le16 src_ntile3;
	__le16 Reserved2;
	__le16 Reserved1;
	__le16 dst_imgw;
	__le16 src_imgw;
	__le16 Reserved4;
	__le16 Reserved3;
} __packed ax_dma_lli_reg_t;

typedef struct {
	u8 lli_type;
	u8 lli_xor;
	u8 lli_chksum;
	u8 lli_endian;
	u8 lli_wb;
	u8 lli_last;
	u8 lli_src_tr_width;
	u8 lli_dst_tr_width;
} ax_dma_lli_ctrl_t;

typedef enum {
	AX_DMA_MODE_OFFSET = 0,
	AX_DMA_1D = AX_DMA_MODE_OFFSET,
	AX_DMA_2D,
	AX_DMA_3D,
	AX_DMA_4D,
	AX_DMA_MEMORY_INIT,
	AX_DMA_CHECKSUM,
} ax_dma_xfer_mode_t;


typedef enum {
	DMA_ENDIAN_DEF = 0,
	DMA_ENDIAN_32B,
	DMA_ENDIAN_16B,
	DMA_ENDIAN_MAX = DMA_ENDIAN_16B
} ax_dma_endian_t;

typedef struct {
	u32 src_stride;
	u32 dst_stride;
	u32 src_ntiles;
	u32 dst_ntiles;
} dim_data_t;

typedef struct {
	u64 paddr;
	u32 imgw;
	u32 stride[3];
} dim_info_t;

typedef struct {
	u64 lli_addr;
	u64 lli_addr_next;
	ax_dma_lli_reg_t *lli_addr_vir;
	ax_dma_lli_reg_t *lli_addr_vir_next;
	dim_info_t dimInfo;
	ax_dma_lli_ctrl_t lli_ctrl;
} ax_dma_lli_t;

typedef struct {
	u64 lli_addr;
	ax_dma_lli_reg_t *lli_addr_vir;
	struct list_head lli_node;
} ax_dma_lli_pool_t;

typedef struct {
	u32 ch;
	u32 vch;
	u32 sub_num;
	u32 endian;
	u32 checksum;
	u64 lli_base;
	u64 lli_base_vir;
	ax_dma_xfer_mode_t dma_mode;
	ax_dma_lli_t *lli_list;
} ax_dma_xfer_t;

typedef struct {
	volatile unsigned char issync;
	unsigned long int pthread_id;
} ax_dma_file_msg_t;

typedef struct {
	u8 kuser;
	ax_dma_file_msg_t file_msg;
	wait_queue_head_t event_wait;
	struct mutex mutex_sync;
	struct inode *inode;
} ax_dma_chn_t;

typedef struct {
	u16 ntiles[3];
	dim_info_t src_info;
	dim_info_t dst_info;
} ax_dma_xd_desc_t;

typedef struct {
	u64 src;
	u64 dst;
	u32 size;
} ax_dma_desc_t;

typedef struct {
	s32 id;
	u32 checksum;
	volatile u32 stat;
} ax_dma_xfer_stat_t;

typedef struct {
	ax_dma_xfer_stat_t xfer_stat;
	void (*pFun) (ax_dma_xfer_stat_t *, void *);
	void *arg;
} ax_dma_stat_t;

typedef struct {
	u32 lli_num;
	ax_dma_endian_t endian;
	void *msg_lli;
	void (*pFun) (ax_dma_xfer_stat_t *, void *);
	void *arg;
	ax_dma_xfer_mode_t dma_mode;
} ax_dma_msg_t;

typedef struct {
	u8 kuser;
	u32 checksum_num;
	u32 size;
	ktime_t time_start;
	ktime_t ktime_start;
	ktime_t ktime_run;
	ax_dma_msg_t dma_msg;
	ax_dma_xfer_t xfer;
	ax_dma_xfer_stat_t xfer_stat;
	int (*dma_callback)(void *, unsigned int);
	void *arg;
	struct file *filp;
	struct list_head dma_pool_list;
	struct list_head xfer_node;
} ax_dma_xfer_msg_t;

typedef struct {
	ax_dma_xfer_msg_t *dma_xfer_msg;
	struct inode *inode;
	struct file *filp;
	struct work_struct work;
} ax_dma_work_data_t;

struct ax_dma_device {
	void __iomem *base;
	struct clk *aclk;
	struct clk *pclk;
	struct reset_control *arst;
	struct reset_control *prst;
	struct dma_pool *dma_pool;
	u32 suspend;
	u32 irq;
	volatile s32 id;
	spinlock_t lock;
	volatile s32 irq_stat;
	struct work_struct xfer_work;
	ax_dma_work_data_t work_data;
	struct mutex kmutex;
	struct mutex mutex_idr;
	struct completion complete;
	struct platform_device *pdev;
	ax_dma_xfer_t *xfer;
	ax_dma_xfer_mode_t dma_mode;
	struct list_head dma_pending_list;
	struct list_head dma_complite_list;
	struct list_head dma_free_list;
	struct list_head dma_checksum_cfg_list;
	struct list_head dma_checksum_complite_list;
};

typedef struct {
	u32 msg_id;
	u32 size;
	u64 start;
	u64 wait;
	u64 run;
	ax_dma_xfer_mode_t dma_mode;
} ax_dma_dbg_info_t;

typedef struct {
	u8 last;
	atomic_t chan_use;
	atomic64_t total;
	atomic64_t current_total;
	ax_dma_dbg_info_t info[8];
} ax_dma_dbg_t;

enum dma_type {
	DMA_1D = 0,
	DMA_2D,
	DMA_3D,
	DMA_4D,
	DMA_MEMORY_INIT,
};

enum dma_tr_width {
	DMA_TR_1B = 0,
	DMA_TR_2B,
	DMA_TR_4B,
	DMA_TR_8B,
	DMA_TR_MAX = DMA_TR_8B
};

enum dma_ax_len {
	DMA_AXI_1B = 0,
	DMA_AXI_2B,
	DMA_AXI_4B,
	DMA_AXI_MAX = DMA_AXI_4B
};

enum dma_int_sta {
	DMA_XFER_IDLE = 0,
	DMA_XFER_SUCCESS = BIT(0),
	DMA_BLOCK_TS = BIT(1),
	DMA_LLI_ERR = BIT(3),
	DMA_AXI_RD_ERR = BIT(4),
	DMA_AXI_WE_ERR = BIT(5),
	DMA_XFER_TIMEOUT = BIT(6),
	DMA_XFER_ERESTARTSYS = BIT(7),
	DMA_XFER_WAIT = BIT(8),
	DMA_XFER_DONE = GENMASK(5, 0),
};

#define DMA_MAGIC                 ('d'+'m'+'a')
#define DMA_CFG_MEM_CHN_CMD       _IOWR(DMA_MAGIC, 1, ax_dma_msg_t)
#define DMA_START_CHN_CMD         _IOWR(DMA_MAGIC, 2, int)
#define DMA_GET_RESULT            _IOWR(DMA_MAGIC, 3, ax_dma_msg_t)
#define DMA_XFER_CHN_CMD          _IOWR(DMA_MAGIC, 4, ax_dma_msg_t)
#define DMA_FREE_MSG_BY_ID        _IOWR(DMA_MAGIC, 5, int)

static long ax_dma_ioctl(struct file *filp, unsigned int cmd,
			    unsigned long arg);
static s32 ax_dma_release(struct inode *inode, struct file *filp);

#endif
