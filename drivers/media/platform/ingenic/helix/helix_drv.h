/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Difan Zhang */

#ifndef _HELIX_DRV_H_
#define _HELIX_DRV_H_

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/kref.h>
#include <linux/reset.h>
#include <linux/workqueue.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-v4l2.h>

#include "helix_hw.h"
#include "helix_rc.h"
#include "helix_vdma.h"

#define HELIX_NAME "ingenic-helix-v4l2"

#define HELIX_MIN_W	64
#define HELIX_MIN_H	64
#define HELIX_MAX_W	1920
#define HELIX_MAX_H	1088
#define HELIX_DEF_W	1920
#define HELIX_DEF_H	1088

#define HELIX_BOUNCE_BUF_SIZE	(1536 * 1024)
#define HELIX_SCRATCH_BUF_SIZE	(1536 * 1024)

struct helix_dev;
struct helix_ctx;

struct helix_fmt {
	u32 fourcc;
	u32 types;
	u32 num_planes;
	u8  depth[VIDEO_MAX_PLANES];
};

struct helix_dev {
	struct device		*dev;
	void __iomem		*base;
	struct reset_control	*reset;
	struct kref		ref;
	struct clk_bulk_data	clocks[3];
	int			irq;

	struct v4l2_device	v4l2_dev;
	struct video_device	vfd;
	struct v4l2_m2m_dev	*m2m_dev;

	struct mutex		dev_mutex;
	struct mutex		hw_lock;

	struct helix_ctx	*curr_ctx;
	bool			hw_busy;
	/* True from a kicked task until the IRQ completes or a reset succeeds. */
	bool			hw_running;
	struct helix_ctx	*dma_owner;
	struct helix_h264_buffers h264_hold;
	bool			h264_hold_valid;
	bool			disconnected;
	bool			aborting;
	bool			fatal;
	struct work_struct	work;
	struct workqueue_struct *workqueue;
	struct list_head contexts;
	struct completion	completion;
	u32			irq_status;
	u32			bslen;
	/* Shared DMA buffers, allocated while any context is streaming. */
	unsigned int		dma_users;
	void			*input_vaddr;
	dma_addr_t		input_paddr;
	size_t			input_size;

	void			*scratch_vaddr;
	dma_addr_t		scratch_paddr;
	size_t			scratch_size;

	void			*bounce_vaddr;
	dma_addr_t		bounce_paddr;
	size_t			bounce_size;
	__le32			*task_vaddr;
	dma_addr_t		task_paddr;
};

struct helix_ctx {
	struct v4l2_fh		fh;
	struct v4l2_ctrl_handler hdl;
	struct list_head	list;
	bool			stopping;
	struct helix_dev	*hdev;
	struct v4l2_m2m_ctx	*m2m_ctx;

	struct v4l2_format	src_fmt;
	struct v4l2_format	dst_fmt;
	struct v4l2_rect	crop;

	u32			jpeg_quality;
	u32			qmem[256];
	u8			qtable[128];

	struct helix_h264_buffers h264_bufs;
	bool			h264_bufs_allocated;
	bool			dma_ref;
	u32			i_frame_qp;
	u32			p_frame_qp;
	u32			min_qp;
	u32			max_qp;
	u32			gop_size;
	u32			h264_profile;
	u32			h264_level;
	u32			header_mode;
	bool			force_key_frame;
	bool			repeat_seq_header;
	struct helix_rc		rc;

	u32			frame_num;
	u32			sequence;
	u32			idr_pic_id;
	u32			gop_count;
};

/* CP0 Config7 VPU enable/disable hardware controls */
static inline void helix_cp0_config7_enable(void)
{
#ifdef CONFIG_MIPS
	u32 val;

	__asm__ __volatile__(
		".set push\n\t"
		".set mips32r2\n\t"
		"mfc0 %0, $16, 7\n\t"
		"ori  %0, %0, 0x340\n\t"
		"andi %0, %0, 0x3ff\n\t"
		"mtc0 %0, $16, 7\n\t"
		"ehb\n\t"
		".set pop\n\t"
		: "=r"(val));
#endif
}

static inline void helix_cp0_config7_disable(void)
{
#ifdef CONFIG_MIPS
	u32 val;

	__asm__ __volatile__(
		".set push\n\t"
		".set mips32r2\n\t"
		"mfc0 %0, $16, 7\n\t"
		"andi %0, %0, 0xbf\n\t"
		"mtc0 %0, $16, 7\n\t"
		"ehb\n\t"
		".set pop\n\t"
		: "=r"(val));
#endif
}

const struct helix_fmt *helix_find_fmt(u32 fourcc, u32 type);

void helix_calc_qtable(u32 quality, u8 *qtable, u32 *qmem);
size_t helix_jpeg_write_headers(u8 *buf, u32 width, u32 height,
				const u8 *qtable, u32 visible_width, u32 visible_height);
void helix_jpeg_write_eoi(u8 *buf, size_t offset);
int helix_jpeg_hw_start(struct helix_dev *hdev,
			dma_addr_t src_y_dma, dma_addr_t src_uv_dma,
			dma_addr_t dst_dma, u32 width, u32 height,
			u32 y_stride, u32 uv_stride, u32 fourcc,
			const u32 *qmem);

extern const struct v4l2_ioctl_ops helix_v4l2_ioctl_ops;
extern const struct v4l2_file_operations helix_v4l2_fops;
extern const struct v4l2_m2m_ops helix_m2m_ops;

int helix_v4l2_init_ctx(struct helix_ctx *ctx);
void helix_v4l2_cleanup_ctx(struct helix_ctx *ctx);

int helix_get_dma(struct helix_dev *hdev, bool need_scratch);
int helix_ensure_input_dma(struct helix_dev *hdev, size_t input_size);
void helix_put_dma(struct helix_dev *hdev);
int helix_hw_reset(struct helix_dev *hdev);
int helix_hw_quiesce(struct helix_dev *hdev);
void helix_job_work(struct work_struct *work);

#endif
