// SPDX-License-Identifier: GPL-2.0
/*
 * Fullhan FH8626V100 video processing unit: three scaled NV12 outputs.
 *
 * Registers share the ISP window. Buffer addresses written at frame start
 * latch at the next start. Frames without a queued capture buffer go to a
 * private scratch buffer and are dropped.
 *
 * Copyright (C) 2026 Difan Zhang
 */
#include <linux/bitfield.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/wait.h>

#include "fh8626-pipeline.h"
#include "fh8626-isp-regs.h"
#include "fh8626-isp-defaults.h"

/*
 * Output address registers: Y base, CbCr base and the packed Y/CbCr line
 * strides, one block of three words per channel. The base addresses are
 * sampled by the hardware when a frame starts, so a write made while a
 * frame is being written applies to the next one.
 */
#define VPU_CHAN_ADDR_Y(c)		(0x9ac + 0xc * (c))
#define VPU_CHAN_ADDR_C(c)		(0x9b0 + 0xc * (c))
#define VPU_CHAN_STRIDE(c)		(0x9b4 + 0xc * (c))

/* Input (ISP output) size seen by channels 0 and 1 */
#define VPU_CH0_IN_WIDTH		0x628
#define VPU_CH0_IN_HEIGHT		0x630
#define VPU_CH1_IN_WIDTH		0x650
#define VPU_CH1_IN_HEIGHT		0x658

/*
 * Channels 0 and 1: output size minus one, and the polyphase scaler
 * steps, input pixels per output pixel in Q20, horizontal and vertical,
 * each written to a luma and a chroma register.
 */
#define VPU_CH0_OUT_WIDTH		0x62c
#define VPU_CH0_OUT_HEIGHT		0x634
#define VPU_CH0_STEP_X			0x678
#define VPU_CH0_STEP_Y			0x680
#define VPU_CH1_OUT_WIDTH		0x654
#define VPU_CH1_OUT_HEIGHT		0x65c
#define VPU_CH1_STEP_X			0x688
#define VPU_CH1_STEP_Y			0x690
#define VPU_STEP_MASK			GENMASK(25, 0)
/* Line fetch word of channel 0, derived from its stride */
#define VPU_CH0_LINE_CFG		0x99c

/*
 * Channel 2 scales the output of channel 1: packed output size minus one
 * and the horizontal and vertical ratios, input over output in Q23.
 */
#define VPU_CH2_OUT_SIZE		0x958
#define VPU_CH2_RATIO_X			0x964
#define VPU_CH2_RATIO_Y			0x968

/* Polyphase coefficient banks: four per channel, 512 bytes apart */
#define VPU_SCALER_BANK(c, n)		(0x1600 + ((c) * 4 + (n)) * 0x200)

/*
 * Block-mean side output of channel 0: one byte per square block of the
 * output luma, enabled together with the channel (VPU_CH0_BLKMEAN_EN).
 * VPU_BLKMEAN_CFG[2:1] selects the block edge: 0 for 4, 1 for 8, 2 for 16
 * pixels. VPU_BLKMEAN_STRIDE holds the number of blocks per output line.
 */
#define VPU_BLKMEAN_CFG			0x96c
#define VPU_BLKMEAN_MODE		GENMASK(2, 1)
#define VPU_BLKMEAN_ADDR		0x970
#define VPU_BLKMEAN_STRIDE		0x974
#define VPU_BLKMEAN_BLOCK		8
#define VPU_BLKMEAN_MODE_8X8		1

/*
 * ISP_VPU_ENABLE channel fields. Each channel has a path field, opened and
 * left set, and a write enable that is set and cleared with the channel.
 * Channel 0 also has a bypass, set while it is closed, and the block-mean
 * enable. Bit 16 selects linear frame addresses.
 * Channel 2 takes its input from the channel 1 path.
 */
#define VPU_CH0_PATH			GENMASK(7, 5)
#define VPU_CH0_BYPASS			BIT(14)
#define VPU_CH0_BLKMEAN_EN		BIT(15)
#define VPU_CH0_WRITE_EN		BIT(17)
#define VPU_CH1_PATH			GENMASK(10, 8)
#define VPU_CH1_WRITE_EN		BIT(18)
#define VPU_CH2_PATH			GENMASK(13, 11)
#define VPU_CH2_WRITE_EN		BIT(19)
#define VPU_ENABLE_RESET		0x1401d

/*
 * ISP_VPU_ENABLE bypass bits of the logo groups: a group is blended only
 * while its bit is clear.
 */
#define VPU_LOGO0_BYPASS		BIT(0)
#define VPU_LOGO1_BYPASS		BIT(2)
#define VPU_LOGO2_BYPASS		BIT(3)
#define VPU_LOGO_GROUPS			3

/*
 * Logo window control word: line stride in bytes, pixel depth code and the
 * window enable. A stride of zero repeats the first line on every line of
 * the window. The blend word holds the 16-bit pixel layout and the window
 * opacity, which is used while VPU_LOGO_ALPHA_EN is set. Per-pixel alpha
 * of ARGB layouts and of palette entries applies in addition.
 */
#define VPU_LOGO_STRIDE			GENMASK(15, 0)
#define VPU_LOGO_DEPTH			GENMASK(26, 24)
#define VPU_LOGO_ENABLE			BIT(30)
#define VPU_LOGO_ALPHA			GENMASK(23, 16)
#define VPU_LOGO_LAYOUT			GENMASK(26, 24)
#define VPU_LOGO_ALPHA_EN		BIT(28)
/* Pixel depth codes 0..3 index a palette with 1, 2, 4 or 8 bits per pixel */
#define VPU_LOGO_DEPTH_4BPP		2

/*
 * Register load list. At each frame start the VPU reads a list of segments
 * from memory and writes them into its registers. A segment is a header
 * word followed by VPU_LOAD_COUNT data words for consecutive registers
 * starting at VPU_LOAD_REG (a register word index); VPU_LOAD_MORE marks a
 * segment that is followed by another. The list address is in
 * VPU_LOAD_ADDR; VPU_LOAD_MODE in VPU_LOAD_CTRL enables reading it with
 * value 1 and disables reading with value 3 (the reset value).
 * VPU_LOAD_CFG_EN is set while the list is used.
 *
 * The palette of the group 0 window, 256 ARGB8888 entries starting at
 * VPU_LOGO0_PALETTE, is only reachable this way.
 */
#define VPU_LOAD_CTRL			0x6a0
#define VPU_LOAD_MODE			GENMASK(1, 0)
#define VPU_LOAD_MODE_ON		1
#define VPU_LOAD_MODE_OFF		3
#define VPU_LOAD_CFG			0x9d0
#define VPU_LOAD_CFG_CLEAR		(GENMASK(23, 16) | BIT(5))
#define VPU_LOAD_CFG_EN			BIT(4)
#define VPU_LOAD_ADDR			0x9d8
#define VPU_LOAD_MORE			BIT(31)
#define VPU_LOAD_COUNT			GENMASK(29, 24)
#define VPU_LOAD_REG			GENMASK(23, 0)
#define VPU_LOGO0_PALETTE		0xc00

/*
 * Privacy masks are drawn as a 4-bit indexed bitmap in the group 0
 * window. Index 0 is transparent and index n + 1 the colour of mask n.
 * Every mask edge is moved out to an even pixel, the granularity of the
 * window position, so that each bitmap byte holds two equal indexes.
 */
#define VPU_MASK_PALETTE		16
/* Two register load lists, one in use and one being prepared */
#define VPU_MASK_LIST_WORDS		32
#define VPU_MASK_LIST_BYTES		(2 * VPU_MASK_LIST_WORDS * 4)

/* Two frame periods at the slowest sensor rate, with margin */
#define FH8626_VPU_DRAIN_MS		1000

/* Sized for the smallest block edge in case the mode field is ignored */
#define BLKMEAN_BYTES	PAGE_ALIGN(ALIGN(FH_FRAME_WIDTH_MAX, 32) * \
				   ALIGN(FH_FRAME_HEIGHT_MAX, 32) / 16)

struct fh8626_vpu_chan_hw {
	u32 open_mask, open;
	u32 close_mask, close;
};

static const struct fh8626_vpu_chan_hw vpu_chan_hw[FH8626_VPU_CHANNELS] = {
	{
		.open_mask = VPU_CH0_PATH | VPU_CH0_BYPASS |
			     VPU_CH0_BLKMEAN_EN | VPU_CH0_WRITE_EN,
		.open = VPU_CH0_PATH | VPU_CH0_BLKMEAN_EN | VPU_CH0_WRITE_EN,
		.close_mask = VPU_CH0_BYPASS | VPU_CH0_BLKMEAN_EN |
			      VPU_CH0_WRITE_EN,
		.close = VPU_CH0_BYPASS,
	}, {
		.open_mask = VPU_CH1_PATH | VPU_CH1_WRITE_EN,
		.open = VPU_CH1_PATH | VPU_CH1_WRITE_EN,
		.close_mask = VPU_CH1_WRITE_EN,
	}, {
		.open_mask = VPU_CH1_PATH | VPU_CH2_PATH | VPU_CH2_WRITE_EN,
		.open = VPU_CH1_PATH | VPU_CH2_PATH | VPU_CH2_WRITE_EN,
		.close_mask = VPU_CH2_WRITE_EN,
	},
};

struct fh8626_vpu_dev;

struct fh8626_vpu_chan {
	struct fh8626_vpu_dev *vpu;
	unsigned int id;
	u32 width, height;
	/* Capture buffers queued for this channel, oldest first */
	struct list_head queue;
	/* Receives frames for which no capture buffer was queued */
	void *scratch_cpu;
	dma_addr_t scratch_dma;
	size_t scratch_bytes;
	/* Buffer in the address registers, NULL for the scratch buffer */
	struct fh8626_buffer *next;
	/* Buffer of the frame being written, NULL for the scratch buffer */
	struct fh8626_buffer *cur;
	bool busy;
	bool streaming;
	bool started;
	bool opened_live;
	/*
	 * Stopping: a drain closes the channel at a frame start and is
	 * complete at the next start, once the frame that was in flight has
	 * been written out and no further frame has begun.
	 */
	bool draining, close_written, drained;
	/*
	 * An interrupted frame resumes at its old address on restart. Retain
	 * scratch memory and pin capture DMA-BUFs until a later drain succeeds.
	 */
	bool dirty;
	struct dma_buf *pinned[2];
	u32 sequence;
};

struct fh8626_vpu_dev {
	struct device *dev;
	struct fh8626_isp_dev *isp;
	void __iomem *base;
	struct fh8626_pipeline *pipe;
	spinlock_t lock;
	/* The shared registers are programmed and the producers may run */
	bool running;
	struct fh8626_vpu_chan chan[FH8626_VPU_CHANNELS];
	void *blkmean_cpu;
	dma_addr_t blkmean_dma;
	u32 starts;
	u32 session_starts;
	wait_queue_head_t start_wq;
	u32 in_width, in_height;
	struct fh8626_logo_plane logo[FH8626_LOGO_SLOTS];
	/* Privacy mask bitmap in the group 0 window; mask_on when shown */
	struct fh8626_logo_plane mask;
	bool mask_on;
	void *mask_cpu;
	dma_addr_t mask_dma;
	size_t mask_bytes;
	/* Register load lists for the mask palette; mask_list is in use */
	u32 *list_cpu;
	dma_addr_t list_dma;
	unsigned int mask_list;
	struct mutex mask_lock;
};

/*
 * Logo window register blocks. Group 0 is one window; groups 1 and 2 hold
 * four windows each, spaced 84 bytes apart.
 */
static const u16 fh8626_logo_base[FH8626_LOGO_SLOTS] = {
	0x610,
	0x6b8, 0x70c, 0x760, 0x7b4,
	0x808, 0x85c, 0x8b0, 0x904,
};

static void fh8626_vpu_logo_apply(struct fh8626_vpu_dev *vpu);

void fh8626_vpu_set_pipeline(struct fh8626_vpu_dev *vpu,
			     struct fh8626_pipeline *pipe)
{
	if (!vpu)
		return;
	WRITE_ONCE(vpu->pipe, pipe);
	if (!pipe)
		synchronize_irq(fh8626_isp_irq_number(vpu->isp));
}

static u32 vpu_chan_frame_bytes(const struct fh8626_vpu_chan *ch)
{
	return ch->width * ch->height * 3 / 2;
}

int fh8626_vpu_chan_set_format(struct fh8626_vpu_dev *vpu, unsigned int chan,
			       u32 width, u32 height)
{
	struct fh8626_vpu_chan *ch;
	unsigned long flags;
	int ret = 0;

	if (!vpu || chan >= FH8626_VPU_CHANNELS)
		return -ENODEV;
	/* Line strides are whole 16-byte words; chroma rows are paired */
	if (!width || !height || width > FH_FRAME_WIDTH_MAX ||
	    height > FH_FRAME_HEIGHT_MAX || width % 16 || height % 2)
		return -EINVAL;
	ch = &vpu->chan[chan];
	spin_lock_irqsave(&vpu->lock, flags);
	/* Channel 2 scales from the output of channel 1 */
	if (ch->streaming)
		ret = -EBUSY;
	else if (chan == 1 && vpu->chan[2].streaming &&
		 (width != ch->width || height != ch->height))
		ret = -EBUSY;
	else if (ch->dirty && PAGE_ALIGN(width * height * 3 / 2) >
		 ch->scratch_bytes)
		ret = -EBUSY;
	if (!ret) {
		ch->width = width;
		ch->height = height;
	}
	spin_unlock_irqrestore(&vpu->lock, flags);
	return ret;
}

/*
 * Q20 polyphase step for scaling @in pixels to @out pixels. The exact ratio
 * keeps the last output line inside the input for sizes that are not
 * multiples of 16.
 */
static u32 vpu_scale_step(u32 in, u32 out)
{
	return DIV_ROUND_CLOSEST_ULL((u64)in << 20, out) & VPU_STEP_MASK;
}

static void vpu_chan_geometry(struct fh8626_vpu_dev *vpu, unsigned int chan)
{
	const struct fh8626_vpu_chan *ch = &vpu->chan[chan];
	const struct fh8626_vpu_chan *src = &vpu->chan[1];
	u32 w = ch->width, h = ch->height;
	u32 step_x, step_y;

	writel(w | (w << 16), vpu->base + VPU_CHAN_STRIDE(chan));
	switch (chan) {
	case 0:
		step_x = vpu_scale_step(vpu->in_width, w);
		step_y = vpu_scale_step(vpu->in_height, h);
		writel(w - 1, vpu->base + VPU_CH0_OUT_WIDTH);
		writel(h - 1, vpu->base + VPU_CH0_OUT_HEIGHT);
		writel(step_x, vpu->base + VPU_CH0_STEP_X);
		writel(step_x, vpu->base + VPU_CH0_STEP_X + 4);
		writel(step_y, vpu->base + VPU_CH0_STEP_Y);
		writel(step_y, vpu->base + VPU_CH0_STEP_Y + 4);
		writel(((w >> 4) + 7) & 0xfff8, vpu->base + VPU_CH0_LINE_CFG);
		/*
		 * Channel enable also enables block-mean DMA. Give it a target
		 * matching the output stride to prevent writes to address zero.
		 */
		fh8626_isp_update_bits(vpu->isp, VPU_BLKMEAN_CFG,
				       VPU_BLKMEAN_MODE,
				       FIELD_PREP(VPU_BLKMEAN_MODE,
						  VPU_BLKMEAN_MODE_8X8));
		writel((w / VPU_BLKMEAN_BLOCK) & 0xffff,
		       vpu->base + VPU_BLKMEAN_STRIDE);
		writel(vpu->blkmean_dma, vpu->base + VPU_BLKMEAN_ADDR);
		break;
	case 1:
		step_x = vpu_scale_step(vpu->in_width, w);
		step_y = vpu_scale_step(vpu->in_height, h);
		writel(w - 1, vpu->base + VPU_CH1_OUT_WIDTH);
		writel(h - 1, vpu->base + VPU_CH1_OUT_HEIGHT);
		writel(step_x, vpu->base + VPU_CH1_STEP_X);
		writel(step_x, vpu->base + VPU_CH1_STEP_X + 4);
		writel(step_y, vpu->base + VPU_CH1_STEP_Y);
		writel(step_y, vpu->base + VPU_CH1_STEP_Y + 4);
		break;
	case 2:
		writel((w - 1) | ((h - 1) << 16), vpu->base + VPU_CH2_OUT_SIZE);
		/* Truncated, so the last line never samples past the input */
		writel(div_u64((u64)src->width << 23, w),
		       vpu->base + VPU_CH2_RATIO_X);
		writel(div_u64((u64)src->height << 23, h),
		       vpu->base + VPU_CH2_RATIO_Y);
		break;
	}
}

static void vpu_chan_program(struct fh8626_vpu_chan *ch,
			     struct fh8626_buffer *buf)
{
	struct fh8626_vpu_dev *vpu = ch->vpu;
	dma_addr_t y = buf ? buf->paddr : ch->scratch_dma;

	writel(y, vpu->base + VPU_CHAN_ADDR_Y(ch->id));
	writel(y + ch->width * ch->height, vpu->base + VPU_CHAN_ADDR_C(ch->id));
	ch->next = buf;
}

static void vpu_chan_close(struct fh8626_vpu_dev *vpu, unsigned int chan)
{
	const struct fh8626_vpu_chan_hw *hw = &vpu_chan_hw[chan];

	fh8626_isp_update_bits(vpu->isp, ISP_VPU_ENABLE, hw->close_mask,
			       hw->close);
}

/*
 * Program the registers shared by all channels and leave every channel
 * closed. Called before the producers start, with no channel streaming.
 */
int fh8626_vpu_pipeline_start(struct fh8626_vpu_dev *vpu, u32 in_width,
			      u32 in_height)
{
	static const u16 zero_regs[] = {
		0x600, 0x608, 0x610, 0x618, 0x638, 0x63c, 0x640,
		0x644, 0x648, 0x64c, 0x6b8, 0x6c0, 0x70c, 0x714,
		0x760, 0x768, 0x7b4, 0x7bc, 0x808, 0x810, 0x85c,
		0x864, 0x8b0, 0x8b8, 0x904, 0x90c, 0x95c, 0x960,
		0x998, 0x9ac, 0x9b0, 0x9b8, 0x9bc, 0x9c4, 0x9c8,
		0x9e0, 0x9e4, 0x9fc, 0xa00,
	};
	unsigned long flags;
	unsigned int i, bank, c;
	int ret;

	if (!vpu)
		return -ENODEV;
	if (!((in_width == FH_FRAME_WIDTH_DEFAULT &&
	       in_height == FH_FRAME_HEIGHT_DEFAULT) ||
	      (in_width == FH_FRAME_WIDTH_MAX &&
	       in_height == FH_FRAME_HEIGHT_MAX)))
		return -EINVAL;
	ret = fh8626_isp_dma_begin(vpu->isp);
	if (ret)
		return ret;
	vpu->in_width = in_width;
	vpu->in_height = in_height;

	/*
	 * Every channel closed, linear NV12 output. The PMU reset lines are
	 * shared with the audio clocks and are left alone.
	 */
	writel(VPU_ENABLE_RESET, vpu->base + ISP_VPU_ENABLE);
	for (i = 0; i < ARRAY_SIZE(zero_regs); ++i)
		writel(0, vpu->base + zero_regs[i]);
	writel(0x88, vpu->base + 0x624);
	writel(((in_width / 2 - 1) << 16) | (in_width - 1),
	       vpu->base + VPU_CH0_IN_WIDTH);
	writel((in_height - 1) * 0x10001, vpu->base + VPU_CH0_IN_HEIGHT);
	writel(((in_width / 2 - 1) << 16) | (in_width - 1),
	       vpu->base + VPU_CH1_IN_WIDTH);
	writel((in_height - 1) * 0x10001, vpu->base + VPU_CH1_IN_HEIGHT);
	fh8626_isp_update_bits(vpu->isp, 0x698, BIT(0), 0);
	writel(100, vpu->base + 0x9ec);
	for (c = 0; c < 2; ++c)
		for (bank = 0; bank < 4; ++bank)
			for (i = 0; i < ARRAY_SIZE(fh8626_vpu_scaler); ++i)
				writel(fh8626_vpu_scaler[i], vpu->base +
				       VPU_SCALER_BANK(c, bank) + 4 * i);
	for (c = 0; c < FH8626_VPU_CHANNELS; ++c)
		vpu_chan_geometry(vpu, c);
	fh8626_vpu_logo_apply(vpu);

	spin_lock_irqsave(&vpu->lock, flags);
	vpu->session_starts = 0;
	vpu->running = true;
	spin_unlock_irqrestore(&vpu->lock, flags);
	return 0;
}

/* Called once the producers have stopped and no channel is streaming */
void fh8626_vpu_pipeline_stop(struct fh8626_vpu_dev *vpu)
{
	unsigned long flags;
	bool was_running;

	if (!vpu)
		return;
	spin_lock_irqsave(&vpu->lock, flags);
	was_running = vpu->running;
	vpu->running = false;
	spin_unlock_irqrestore(&vpu->lock, flags);
	wake_up_all(&vpu->start_wq);
	if (was_running)
		fh8626_isp_dma_end(vpu->isp);
}

static unsigned int logo_group(unsigned int slot)
{
	return slot ? (slot < 5 ? 1 : 2) : 0;
}

/* The channel a logo slot is blended into; FH8626_VPU_CHANNELS for all */
unsigned int fh8626_vpu_logo_channel(unsigned int slot)
{
	unsigned int group = logo_group(slot);

	return group ? group - 1 : FH8626_VPU_CHANNELS;
}

static void logo_group_frame(const struct fh8626_vpu_dev *vpu,
			     unsigned int group, u32 *width, u32 *height)
{
	if (!group) {
		*width = vpu->in_width;
		*height = vpu->in_height;
	} else {
		*width = vpu->chan[group - 1].width;
		*height = vpu->chan[group - 1].height;
	}
}

static bool logo_plane_fits(const struct fh8626_vpu_dev *vpu,
			    unsigned int group,
			    const struct fh8626_logo_plane *hw)
{
	u32 lim_w, lim_h;

	logo_group_frame(vpu, group, &lim_w, &lim_h);
	return hw->width && hw->height &&
	       hw->width <= lim_w && hw->height <= lim_h &&
	       hw->x <= lim_w - hw->width &&
	       hw->y <= lim_h - hw->height;
}

/* The privacy masks own the group 0 window while any is set */
static bool mask_shown(const struct fh8626_vpu_dev *vpu)
{
	return vpu->mask_on && vpu->mask.phys &&
	       vpu->mask.x < vpu->in_width && vpu->mask.y < vpu->in_height;
}

static bool logo_shown(const struct fh8626_vpu_dev *vpu, unsigned int slot)
{
	const struct fh8626_logo_plane *hw = &vpu->logo[slot];

	if (!slot && vpu->mask_on)
		return false;
	return hw->on && hw->phys && logo_plane_fits(vpu, logo_group(slot), hw);
}

/*
 * A group is only blended while its bypass bit is clear. Clear it while any
 * window of the group is shown and set it again once all are off.
 */
static void logo_update_bypass(struct fh8626_vpu_dev *vpu, unsigned int group)
{
	static const u32 bypass[VPU_LOGO_GROUPS] = {
		VPU_LOGO0_BYPASS, VPU_LOGO1_BYPASS, VPU_LOGO2_BYPASS,
	};
	unsigned int slot;
	bool shown = !group && mask_shown(vpu);

	for (slot = 0; slot < FH8626_LOGO_SLOTS; ++slot)
		if (logo_group(slot) == group && logo_shown(vpu, slot))
			shown = true;
	fh8626_isp_update_bits(vpu->isp, ISP_VPU_ENABLE, bypass[group],
			       shown ? 0 : bypass[group]);
}

static void logo_window_write(struct fh8626_vpu_dev *vpu, u32 base,
			      const struct fh8626_logo_plane *hw, u32 width,
			      u32 height)
{
	writel(FIELD_PREP(VPU_LOGO_STRIDE, hw->stride) |
	       FIELD_PREP(VPU_LOGO_DEPTH, hw->depth) | VPU_LOGO_ENABLE,
	       vpu->base + base);
	writel(FIELD_PREP(VPU_LOGO_ALPHA, hw->alpha) |
	       FIELD_PREP(VPU_LOGO_LAYOUT, hw->layout) | VPU_LOGO_ALPHA_EN,
	       vpu->base + base + 4);
	writel(hw->phys, vpu->base + base + 8);
	writel(((width - 1) & 0xfff) | (((height - 1) & 0xfff) << 16),
	       vpu->base + base + 0xc);
	writel(((hw->x >> 1) & 0xfff) | (((hw->y >> 1) & 0xfff) << 16),
	       vpu->base + base + 0x10);
}

/*
 * Show the privacy mask bitmap in the group 0 window, cut to the input
 * frame, and have its palette loaded at each frame start.
 */
static void mask_write(struct fh8626_vpu_dev *vpu)
{
	const struct fh8626_logo_plane *hw = &vpu->mask;
	u32 width = min_t(u32, hw->width, vpu->in_width - hw->x);
	u32 height = min_t(u32, hw->height, vpu->in_height - hw->y);

	writel(vpu->list_dma + vpu->mask_list * VPU_MASK_LIST_WORDS * 4,
	       vpu->base + VPU_LOAD_ADDR);
	fh8626_isp_update_bits(vpu->isp, VPU_LOAD_CTRL, VPU_LOAD_MODE,
			       FIELD_PREP(VPU_LOAD_MODE, VPU_LOAD_MODE_ON));
	fh8626_isp_update_bits(vpu->isp, VPU_LOAD_CFG,
			       VPU_LOAD_CFG_CLEAR | VPU_LOAD_CFG_EN,
			       VPU_LOAD_CFG_EN);
	logo_window_write(vpu, fh8626_logo_base[0], hw, width, height);
}

static void mask_load_off(struct fh8626_vpu_dev *vpu)
{
	fh8626_isp_update_bits(vpu->isp, VPU_LOAD_CTRL, VPU_LOAD_MODE,
			       FIELD_PREP(VPU_LOAD_MODE, VPU_LOAD_MODE_OFF));
	fh8626_isp_update_bits(vpu->isp, VPU_LOAD_CFG, VPU_LOAD_CFG_EN, 0);
}

static void logo_write(struct fh8626_vpu_dev *vpu, unsigned int slot)
{
	const struct fh8626_logo_plane *hw = &vpu->logo[slot];
	u32 base = fh8626_logo_base[slot];
	u32 ctrl;

	if (!slot) {
		if (mask_shown(vpu)) {
			mask_write(vpu);
			logo_update_bypass(vpu, 0);
			return;
		}
		mask_load_off(vpu);
	}
	if (!logo_shown(vpu, slot)) {
		ctrl = readl(vpu->base + base);
		writel(ctrl & ~VPU_LOGO_ENABLE, vpu->base + base);
		logo_update_bypass(vpu, logo_group(slot));
		return;
	}
	logo_window_write(vpu, base, hw, hw->width, hw->height);
	logo_update_bypass(vpu, logo_group(slot));
}

static void fh8626_vpu_logo_apply(struct fh8626_vpu_dev *vpu)
{
	unsigned long flags;
	unsigned int slot;

	spin_lock_irqsave(&vpu->lock, flags);
	for (slot = 0; slot < FH8626_LOGO_SLOTS; ++slot)
		logo_write(vpu, slot);
	spin_unlock_irqrestore(&vpu->lock, flags);
}

int fh8626_vpu_logo_program(struct fh8626_vpu_dev *vpu, unsigned int slot,
			    const struct fh8626_logo_plane *plane)
{
	unsigned long flags;

	if (!vpu || !plane || slot >= FH8626_LOGO_SLOTS || !vpu->base)
		return -EINVAL;
	if (plane->on) {
		if (!plane->phys || !plane->width || !plane->height ||
		    !plane->stride || (plane->stride & 7))
			return -EINVAL;
		if (plane->depth != 4 && plane->depth != 5)
			return -EINVAL;
		if (plane->depth == 4 &&
		    (plane->layout < 1 || plane->layout > 3))
			return -EINVAL;
	}
	spin_lock_irqsave(&vpu->lock, flags);
	if (plane->on && !slot && vpu->mask_on) {
		spin_unlock_irqrestore(&vpu->lock, flags);
		return -EBUSY;
	}
	if (plane->on && !logo_plane_fits(vpu, logo_group(slot), plane)) {
		spin_unlock_irqrestore(&vpu->lock, flags);
		return -EINVAL;
	}
	vpu->logo[slot] = *plane;
	logo_write(vpu, slot);
	spin_unlock_irqrestore(&vpu->lock, flags);
	return 0;
}

static bool vpu_frame_starts_locked(const struct fh8626_vpu_dev *vpu,
				    u32 *count)
{
	*count = vpu->starts;
	return vpu->running;
}

static void mask_render(u8 *bitmap, u32 stride, u32 x0, u32 y0, u32 h,
			const struct fh8626_mask_rect *masks)
{
	unsigned int i;
	u32 left, right, y;

	memset(bitmap, 0, stride * h);
	for (i = 0; i < FH8626_PRIVACY_MASKS; ++i) {
		if (!masks[i].on)
			continue;
		left = round_down(masks[i].left, 2) - x0;
		right = round_up(masks[i].left + masks[i].width, 2) - x0;
		for (y = round_down(masks[i].top, 2);
		     y < round_up(masks[i].top + masks[i].height, 2); ++y)
			memset(bitmap + (y - y0) * stride + left / 2,
			       (i + 1) * 0x11, (right - left) / 2);
	}
}

/*
 * Draw @masks, FH8626_PRIVACY_MASKS entries in input frame coordinates,
 * into the group 0 window. The window is unavailable to logos while any
 * mask is on. Sleeps for up to two frames while the previous bitmap may
 * still be read.
 */
int fh8626_vpu_set_privacy_masks(struct fh8626_vpu_dev *vpu,
				 const struct fh8626_mask_rect *masks)
{
	struct fh8626_logo_plane plane = { };
	u32 x0 = U32_MAX, y0 = U32_MAX, x1 = 0, y1 = 0, *list;
	void *cpu = NULL, *old_cpu;
	dma_addr_t dma = 0, old_dma;
	size_t bytes = 0, old_bytes;
	unsigned long flags;
	unsigned int i, next;
	bool live, was_on;
	u32 seq;

	if (!vpu)
		return -ENODEV;
	for (i = 0; i < FH8626_PRIVACY_MASKS; ++i) {
		const struct fh8626_mask_rect *m = &masks[i];

		if (!m->on)
			continue;
		if (!m->width || !m->height ||
		    m->left + m->width > FH_FRAME_WIDTH_MAX ||
		    m->top + m->height > FH_FRAME_HEIGHT_MAX)
			return -EINVAL;
		x0 = min_t(u32, x0, round_down(m->left, 2));
		y0 = min_t(u32, y0, round_down(m->top, 2));
		x1 = max_t(u32, x1, round_up(m->left + m->width, 2));
		y1 = max_t(u32, y1, round_up(m->top + m->height, 2));
	}
	if (x1) {
		plane.stride = ALIGN((x1 - x0) / 2, 8);
		bytes = plane.stride * (y1 - y0);
		cpu = dma_alloc_coherent(vpu->dev, bytes, &dma, GFP_KERNEL);
		if (!cpu)
			return -ENOMEM;
		mask_render(cpu, plane.stride, x0, y0, y1 - y0, masks);
		plane.phys = dma;
		plane.width = x1 - x0;
		plane.height = y1 - y0;
		plane.x = x0;
		plane.y = y0;
		plane.depth = VPU_LOGO_DEPTH_4BPP;
		plane.alpha = 255;
		plane.on = true;
	}

	mutex_lock(&vpu->mask_lock);
	/* The list not in use; the hardware may be reading the other one */
	next = vpu->mask_list ^ 1;
	list = vpu->list_cpu + next * VPU_MASK_LIST_WORDS;
	memset(list, 0, VPU_MASK_LIST_WORDS * 4);
	list[0] = FIELD_PREP(VPU_LOAD_COUNT, VPU_MASK_PALETTE) |
		  FIELD_PREP(VPU_LOAD_REG, VPU_LOGO0_PALETTE / 4);
	for (i = 0; i < FH8626_PRIVACY_MASKS; ++i)
		list[2 + i] = 0xff000000 | (masks[i].color & 0xffffff);
	/* The list is read by the device once the register write is made */
	wmb();

	spin_lock_irqsave(&vpu->lock, flags);
	if (plane.on && vpu->logo[0].on) {
		spin_unlock_irqrestore(&vpu->lock, flags);
		mutex_unlock(&vpu->mask_lock);
		if (cpu)
			dma_free_coherent(vpu->dev, bytes, cpu, dma);
		return -EBUSY;
	}
	was_on = vpu->mask_on;
	old_cpu = vpu->mask_cpu;
	old_dma = vpu->mask_dma;
	old_bytes = vpu->mask_bytes;
	vpu->mask = plane;
	vpu->mask_on = plane.on;
	vpu->mask_cpu = cpu;
	vpu->mask_dma = dma;
	vpu->mask_bytes = bytes;
	vpu->mask_list = next;
	logo_write(vpu, 0);
	live = vpu_frame_starts_locked(vpu, &seq);
	spin_unlock_irqrestore(&vpu->lock, flags);

	/* The previous bitmap and list stay in use until the switch is in */
	if (live && (was_on || plane.on))
		fh8626_vpu_wait_frame_starts(vpu, seq, 2);
	if (old_cpu)
		dma_free_coherent(vpu->dev, old_bytes, old_cpu, old_dma);
	mutex_unlock(&vpu->mask_lock);
	return 0;
}

struct device *fh8626_vpu_device(struct fh8626_vpu_dev *vpu)
{
	return vpu ? vpu->dev : NULL;
}

void fh8626_vpu_logo_bounds(struct fh8626_vpu_dev *vpu, unsigned int slot,
			    u32 *width, u32 *height)
{
	unsigned long flags;

	if (!vpu) {
		*width = FH_FRAME_WIDTH_DEFAULT;
		*height = FH_FRAME_HEIGHT_DEFAULT;
		return;
	}
	spin_lock_irqsave(&vpu->lock, flags);
	logo_group_frame(vpu, logo_group(slot), width, height);
	spin_unlock_irqrestore(&vpu->lock, flags);
}

static void vpu_chan_free_scratch(struct fh8626_vpu_chan *ch)
{
	if (ch->scratch_cpu)
		dma_free_coherent(ch->vpu->dev, ch->scratch_bytes,
				  ch->scratch_cpu, ch->scratch_dma);
	ch->scratch_cpu = NULL;
	ch->scratch_bytes = 0;
}

static int vpu_chan_alloc_scratch(struct fh8626_vpu_chan *ch)
{
	size_t bytes = PAGE_ALIGN(vpu_chan_frame_bytes(ch));

	if (ch->scratch_cpu && ch->scratch_bytes >= bytes)
		return 0;
	if (ch->dirty) {
		/* Memory awaiting a frame remainder cannot be released yet */
		dev_warn(ch->vpu->dev,
			 "channel %u busy with an interrupted frame\n", ch->id);
		return -EBUSY;
	}
	vpu_chan_free_scratch(ch);
	ch->scratch_cpu = dma_alloc_coherent(ch->vpu->dev, bytes,
					     &ch->scratch_dma, GFP_KERNEL);
	if (!ch->scratch_cpu)
		return -ENOMEM;
	ch->scratch_bytes = bytes;
	return 0;
}

static void vpu_chan_unpin(struct fh8626_vpu_chan *ch)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(ch->pinned); ++i) {
		if (ch->pinned[i])
			dma_buf_put(ch->pinned[i]);
		ch->pinned[i] = NULL;
	}
}

/*
 * Keep the memory of a capture buffer that may still receive the remainder
 * of an interrupted frame. The buffer itself is returned to userspace;
 * its pages remain pinned until the reference is dropped.
 */
static void vpu_chan_pin(struct fh8626_vpu_chan *ch, unsigned int i,
			 struct fh8626_buffer *buf)
{
	struct vb2_buffer *vb = &buf->vb.vb2_buf;
	const struct vb2_mem_ops *ops = vb->vb2_queue->mem_ops;
	struct dma_buf *dbuf = NULL;

	if (vb->memory == VB2_MEMORY_MMAP && ops->get_dmabuf) {
		dbuf = ops->get_dmabuf(vb->planes[0].mem_priv, O_RDWR);
		if (IS_ERR(dbuf))
			dbuf = NULL;
	} else if (vb->memory == VB2_MEMORY_DMABUF && vb->planes[0].dbuf) {
		dbuf = vb->planes[0].dbuf;
		get_dma_buf(dbuf);
	}
	if (!dbuf) {
		dev_err(ch->vpu->dev, "failed to keep channel %u buffer\n",
			ch->id);
		return;
	}
	if (ch->pinned[i])
		dma_buf_put(ch->pinned[i]);
	ch->pinned[i] = dbuf;
}

int fh8626_vpu_chan_start(struct fh8626_vpu_dev *vpu, unsigned int chan)
{
	const struct fh8626_vpu_chan_hw *hw;
	struct fh8626_vpu_chan *ch;
	unsigned long flags;
	int ret;

	if (!vpu || chan >= FH8626_VPU_CHANNELS)
		return -ENODEV;
	ch = &vpu->chan[chan];
	hw = &vpu_chan_hw[chan];
	if (!vpu->running)
		return -EINVAL;
	if (chan < 2 && (ch->width > vpu->in_width ||
			 ch->height > vpu->in_height))
		return -EINVAL;
	if (chan == 2 && (ch->width > vpu->chan[1].width ||
			  ch->height > vpu->chan[1].height))
		return -EINVAL;
	ret = vpu_chan_alloc_scratch(ch);
	if (ret)
		return ret;
	vpu_chan_geometry(vpu, chan);

	spin_lock_irqsave(&vpu->lock, flags);
	ch->cur = NULL;
	ch->busy = false;
	ch->started = false;
	ch->opened_live = vpu->session_starts != 0;
	ch->draining = false;
	ch->close_written = false;
	ch->drained = false;
	ch->sequence = 0;
	/*
	 * The first frame goes to the scratch buffer: the channel may open
	 * in the middle of a frame.
	 */
	vpu_chan_program(ch, NULL);
	ch->streaming = true;
	fh8626_isp_update_bits(vpu->isp, ISP_VPU_ENABLE, hw->open_mask,
			       hw->open);
	spin_unlock_irqrestore(&vpu->lock, flags);
	return 0;
}

void fh8626_vpu_chan_queue(struct fh8626_vpu_dev *vpu, unsigned int chan,
			   struct fh8626_buffer *buf)
{
	unsigned long flags;

	spin_lock_irqsave(&vpu->lock, flags);
	list_add_tail(&buf->queue, &vpu->chan[chan].queue);
	spin_unlock_irqrestore(&vpu->lock, flags);
}

static bool vpu_chan_drain_done(struct fh8626_vpu_chan *ch)
{
	unsigned long flags;
	bool done;

	spin_lock_irqsave(&ch->vpu->lock, flags);
	done = ch->drained || !ch->streaming || !ch->vpu->running;
	spin_unlock_irqrestore(&ch->vpu->lock, flags);
	return done;
}

/*
 * Drain before stopping the sensor or ISP; this takes up to two frame
 * periods. An interrupted frame resumes at its latched address on restart,
 * so its buffer cannot be released until the drain completes.
 */
int fh8626_vpu_chan_drain(struct fh8626_vpu_dev *vpu, unsigned int chan)
{
	struct fh8626_vpu_chan *ch;
	unsigned long flags;
	bool streaming;

	if (!vpu || chan >= FH8626_VPU_CHANNELS)
		return 0;
	ch = &vpu->chan[chan];
	spin_lock_irqsave(&vpu->lock, flags);
	streaming = ch->streaming;
	ch->draining = streaming;
	spin_unlock_irqrestore(&vpu->lock, flags);
	if (!streaming)
		return 0;
	if (!wait_event_timeout(vpu->start_wq, vpu_chan_drain_done(ch),
				msecs_to_jiffies(FH8626_VPU_DRAIN_MS)))
		return -ETIMEDOUT;
	return 0;
}

static void vpu_buffer_return(struct fh8626_buffer *buf,
			      enum vb2_buffer_state state)
{
	vb2_set_plane_payload(&buf->vb.vb2_buf, 0, 0);
	vb2_buffer_done(&buf->vb.vb2_buf, state);
}

/*
 * Close a channel and return every buffer it holds in @state. A channel
 * that was not drained may still owe a frame remainder; the memory that
 * remainder can reach is kept.
 */
void fh8626_vpu_chan_stop(struct fh8626_vpu_dev *vpu, unsigned int chan,
			  enum vb2_buffer_state state)
{
	struct fh8626_buffer *held[2], *buf, *tmp;
	struct fh8626_vpu_chan *ch;
	unsigned long flags;
	LIST_HEAD(bufs);
	bool was_streaming, drained, latched;
	unsigned int i;

	if (!vpu || chan >= FH8626_VPU_CHANNELS)
		return;
	ch = &vpu->chan[chan];
	spin_lock_irqsave(&vpu->lock, flags);
	was_streaming = ch->streaming;
	ch->streaming = false;
	drained = ch->drained;
	latched = ch->started || ch->opened_live;
	held[0] = ch->busy ? ch->cur : NULL;
	held[1] = ch->next;
	ch->cur = NULL;
	ch->next = NULL;
	ch->busy = false;
	list_splice_init(&ch->queue, &bufs);
	if (was_streaming)
		vpu_chan_close(vpu, chan);
	writel(0, vpu->base + VPU_CHAN_ADDR_Y(chan));
	writel(0, vpu->base + VPU_CHAN_ADDR_C(chan));
	spin_unlock_irqrestore(&vpu->lock, flags);
	synchronize_irq(fh8626_isp_irq_number(vpu->isp));
	wake_up_all(&vpu->start_wq);

	if (was_streaming && drained) {
		ch->dirty = false;
		vpu_chan_unpin(ch);
	} else if (was_streaming && latched) {
		/* The frame in flight, or the next one, may yet be written */
		dev_warn(vpu->dev,
			 "channel %u stopped inside a frame; keeping its memory\n",
			 chan);
		ch->dirty = true;
		for (i = 0; i < ARRAY_SIZE(held); ++i)
			if (held[i])
				vpu_chan_pin(ch, i, held[i]);
	}
	for (i = 0; i < ARRAY_SIZE(held); ++i)
		if (held[i])
			vpu_buffer_return(held[i], state);
	list_for_each_entry_safe(buf, tmp, &bufs, queue) {
		list_del(&buf->queue);
		vpu_buffer_return(buf, state);
	}
	if (!ch->dirty)
		vpu_chan_free_scratch(ch);
}

static void vpu_chan_complete(struct fh8626_vpu_chan *ch, u64 timestamp)
{
	struct fh8626_buffer *buf = ch->cur;
	u32 sequence = ch->sequence++;

	ch->busy = false;
	ch->cur = NULL;
	if (!buf)
		return;
	buf->vb.sequence = sequence;
	buf->vb.vb2_buf.timestamp = timestamp;
	buf->vb.field = V4L2_FIELD_NONE;
	vb2_set_plane_payload(&buf->vb.vb2_buf, 0, vpu_chan_frame_bytes(ch));
	vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_DONE);
}

static void vpu_chan_irq(struct fh8626_vpu_chan *ch, bool started,
			 bool ended, u64 timestamp)
{
	struct fh8626_vpu_dev *vpu = ch->vpu;
	struct fh8626_buffer *buf;

	if (!ch->streaming || ch->drained)
		return;
	/*
	 * A frame ends at the VPU end event or, when none is reported, at the
	 * next start. Overflow does not end a frame and is not tied to one.
	 */
	if (ended && ch->busy)
		vpu_chan_complete(ch, timestamp);
	if (!started)
		return;
	ch->started = true;
	if (ch->busy)
		vpu_chan_complete(ch, timestamp);
	if (ch->close_written) {
		/* Closed at the previous start: no frame has begun */
		ch->drained = true;
		return;
	}
	/*
	 * The starting frame has latched the old address; writes here apply
	 * at the next frame start.
	 */
	ch->cur = ch->next;
	ch->busy = true;
	if (ch->draining) {
		vpu_chan_close(vpu, ch->id);
		ch->close_written = true;
		vpu_chan_program(ch, NULL);
		return;
	}
	buf = list_first_entry_or_null(&ch->queue, struct fh8626_buffer,
				       queue);
	if (buf)
		list_del(&buf->queue);
	vpu_chan_program(ch, buf);
}

void fh8626_vpu_irq(struct fh8626_vpu_dev *vpu, u32 status, u64 timestamp)
{
	bool started = status & ISP_INT_VPU_START;
	bool ended = status & ISP_INT_VPU_END;
	unsigned long flags;
	unsigned int c;

	spin_lock_irqsave(&vpu->lock, flags);
	if (vpu->running) {
		if (started) {
			vpu->starts++;
			vpu->session_starts++;
		}
		for (c = 0; c < FH8626_VPU_CHANNELS; ++c)
			vpu_chan_irq(&vpu->chan[c], started, ended, timestamp);
	}
	spin_unlock_irqrestore(&vpu->lock, flags);
	if (started)
		wake_up_all(&vpu->start_wq);
}

/*
 * Report whether the VPU is running and, if so, how many frames it has
 * started. Register writes made now take effect at a later frame start.
 */
bool fh8626_vpu_frame_starts(struct fh8626_vpu_dev *vpu, u32 *count)
{
	unsigned long flags;
	bool running;

	if (!vpu)
		return false;
	spin_lock_irqsave(&vpu->lock, flags);
	running = vpu_frame_starts_locked(vpu, count);
	spin_unlock_irqrestore(&vpu->lock, flags);
	return running;
}

/*
 * Sleep until the VPU has started @n frames after the count @since, or
 * until it stops. Returns -ETIMEDOUT if the frames do not arrive.
 */
int fh8626_vpu_wait_frame_starts(struct fh8626_vpu_dev *vpu, u32 since,
				 unsigned int n)
{
	u32 count;
	long left;

	left = wait_event_timeout(vpu->start_wq,
				  !fh8626_vpu_frame_starts(vpu, &count) ||
				  count - since >= n,
				  msecs_to_jiffies(250 * n));
	return left ? 0 : -ETIMEDOUT;
}

struct fh8626_vpu_dev *fh8626_vpu_create(struct device *dev,
					 struct fh8626_isp_dev *isp)
{
	static const u32 sizes[FH8626_VPU_CHANNELS][2] = {
		{ FH_FRAME_WIDTH_DEFAULT, FH_FRAME_HEIGHT_DEFAULT },
		{ 640, 360 },
		{ 320, 180 },
	};
	struct fh8626_vpu_dev *vpu;
	unsigned int c;

	vpu = kzalloc(sizeof(*vpu), GFP_KERNEL);
	if (!vpu)
		return ERR_PTR(-ENOMEM);
	vpu->isp = isp;
	vpu->dev = dev;
	vpu->base = fh8626_isp_base(vpu->isp);
	vpu->in_width = FH_FRAME_WIDTH_DEFAULT;
	vpu->in_height = FH_FRAME_HEIGHT_DEFAULT;
	spin_lock_init(&vpu->lock);
	init_waitqueue_head(&vpu->start_wq);
	for (c = 0; c < FH8626_VPU_CHANNELS; ++c) {
		vpu->chan[c].vpu = vpu;
		vpu->chan[c].id = c;
		vpu->chan[c].width = sizes[c][0];
		vpu->chan[c].height = sizes[c][1];
		INIT_LIST_HEAD(&vpu->chan[c].queue);
	}
	mutex_init(&vpu->mask_lock);
	vpu->blkmean_cpu = dma_alloc_coherent(vpu->dev, BLKMEAN_BYTES,
					      &vpu->blkmean_dma, GFP_KERNEL);
	if (!vpu->blkmean_cpu)
		goto err_free;
	vpu->list_cpu = dma_alloc_coherent(vpu->dev, VPU_MASK_LIST_BYTES,
					   &vpu->list_dma, GFP_KERNEL);
	if (!vpu->list_cpu)
		goto err_blkmean;
	return vpu;

err_blkmean:
	dma_free_coherent(vpu->dev, BLKMEAN_BYTES, vpu->blkmean_cpu,
			  vpu->blkmean_dma);
err_free:
	mutex_destroy(&vpu->mask_lock);
	kfree(vpu);
	return ERR_PTR(-ENOMEM);
}

void fh8626_vpu_destroy(struct fh8626_vpu_dev *vpu)
{
	unsigned long flags;
	bool dirty = false;
	unsigned int c;

	if (!vpu)
		return;
	for (c = 0; c < FH8626_VPU_CHANNELS; ++c) {
		fh8626_vpu_chan_stop(vpu, c, VB2_BUF_STATE_ERROR);
		dirty |= vpu->chan[c].dirty;
	}
	fh8626_vpu_pipeline_stop(vpu);
	/*
	 * With a frame left unfinished, the DMA targets may still be written;
	 * retain them and any pinned capture memory permanently.
	 */
	if (dirty)
		return;
	spin_lock_irqsave(&vpu->lock, flags);
	vpu->mask_on = false;
	logo_write(vpu, 0);
	writel(0, vpu->base + VPU_LOAD_ADDR);
	spin_unlock_irqrestore(&vpu->lock, flags);
	if (vpu->mask_cpu)
		dma_free_coherent(vpu->dev, vpu->mask_bytes, vpu->mask_cpu,
				  vpu->mask_dma);
	dma_free_coherent(vpu->dev, VPU_MASK_LIST_BYTES, vpu->list_cpu,
			  vpu->list_dma);
	dma_free_coherent(vpu->dev, BLKMEAN_BYTES, vpu->blkmean_cpu,
			  vpu->blkmean_dma);
	mutex_destroy(&vpu->mask_lock);
	kfree(vpu);
}
