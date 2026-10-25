// SPDX-License-Identifier: GPL-2.0
/*
 * FH8626 PAE H.264 Main/CABAC encoder.
 *
 * Output DMA uses private coherent storage. Input uses direct DMA when its
 * layout permits. Other input layouts use a staging copy.
 *
 * Each context owns reference, motion, macroblock, stream and CABAC storage.
 * Reprogramming per-frame state allows contexts to share the engine.
 *
 * Copyright (C) 2026 Difan Zhang
 */
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/soc/fullhan/fh8626-pmu.h>
#include <asm/unaligned.h>
#include "fh8626-pae-hw.h"
#include "fh8626-pae-params.h"
#include "fh8626-pae-bitstream.h"

#define PAE_IRQ_STATUS 0x08
#define PAE_IRQ_ENABLE 0x0c
#define PAE_CTRL 0x18
#define PAE_CTRL_RESET BIT(16)
#define PAE_RESET_DONE 0x20
#define PAE_DMU 0x1000
#define PAE_TAE 0x3000
#define PAE_IRQ_SLICE BIT(0)
#define PAE_IRQ_SPACE BIT(1)
#define PAE_IRQ_ALL 0x3f
#define PAE_TAE_FINISH 15
#define PAE_TIMEOUT_MS 1000
#define PAE_DMA_BITS 29
/* Rows between the reconstruction and the reference it overwrites. */
#define PAE_REF_GUARD_ROWS 256

/* Active-low block reset command word; idle bits read back as one. */
#define PAE_PMU_RESET 0x4c
#define PAE_PMU_RESET_BIT BIT(23)
#define PAE_PMU_CLK_GATE 0x24
#define PAE_PMU_STATUS 0x200
#define PAE_PMU_STATUS_BUSY BIT(21)

struct pae_area {
	void *cpu;
	dma_addr_t dma;
	u32 bytes;
};

struct fh8626_pae_ctx {
	struct list_head retained;
	struct fh8626_pae_hw *hw;
	u32 width, height, stride, coded_width, coded_height;
	/* No usable prediction reference, including after a failed job. */
	bool need_idr;
	/*
	 * Circular reference buffer in 32-row blocks. Reconstruction leads
	 * the prediction reference by 256 rows to preserve unread data.
	 */
	unsigned int ref_blocks;
	unsigned int ref_pos;
	/* Context tables after the previous frame and after the last IDR. */
	u8 ec_prev[120];
	u8 ec_idr[120];
	/* The staging copy (y, uv) is only allocated once a frame needs it. */
	struct pae_area y, uv, ref_y, ref_uv, mv, mb[2], stream;
	struct fh8626_pae_stats stats;
};

struct fh8626_pae_hw {
	struct list_head quarantine;
	resource_size_t resource_start;
	struct device *dev;
	void __iomem *core;
	void __iomem *working;
	void __iomem *sram;
	/* Serializes frame programming, execution, readback and reset. */
	struct mutex engine_lock;
	unsigned int contexts;
	/* Contexts released after the engine failed; their DMA is kept. */
	struct list_head retained;
	/* Protects the interrupt state and the statistics. */
	spinlock_t lock;
	struct completion completion;
	bool active;
	bool poisoned;
	u32 bits;
	u32 irq_status;
	unsigned int irq_count;
	u32 stream_bytes;
	ktime_t kick, done;
	struct fh8626_pae_stats stats;
	struct fh8626_pae_params params;
	struct fh8626_pae_registers registers;
};

/* Failed engines retain their DMA, device and module references. */
static LIST_HEAD(pae_quarantine);
static DEFINE_MUTEX(pae_quarantine_lock);

static void pae_dump_state(struct fh8626_pae_hw *hw, const char *what)
{
	u32 gate = 0, status = 0;

	fh8626_pmu_read(PAE_PMU_CLK_GATE, &gate);
	fh8626_pmu_read(PAE_PMU_STATUS, &status);
	dev_err(hw->dev,
		"%s: irq %#x/%#x seen %#x x%u ctrl %#x done %#x tae %#x/%#x bits %u pmu %#x/%#x\n",
		what, readl(hw->core + PAE_IRQ_STATUS),
		readl(hw->core + PAE_IRQ_ENABLE), hw->irq_status,
		hw->irq_count, readl(hw->core + PAE_CTRL),
		readl(hw->core + PAE_RESET_DONE),
		readl(hw->core + PAE_TAE + 0xd0),
		readl(hw->core + PAE_TAE + 0xd4),
		readl(hw->core + PAE_TAE + 0x110), gate, status);
}

/* Engine-level reset request, acknowledged in PAE_RESET_DONE. */
static int pae_engine_reset(struct fh8626_pae_hw *hw)
{
	u32 status;

	writel(0, hw->core + PAE_IRQ_ENABLE);
	writel(PAE_CTRL_RESET, hw->core + PAE_CTRL);
	return readl_poll_timeout(hw->core + PAE_RESET_DONE, status, status,
				  10, 10000);
}

/*
 * PMU reset fallback requires idle: resetting during bus transfers can hang
 * the interconnect and reset the SoC.
 */
static int pae_block_reset(struct fh8626_pae_hw *hw)
{
	unsigned int i;
	u32 value;
	int ret;

	ret = fh8626_pmu_read(PAE_PMU_STATUS, &value);
	if (ret)
		return ret;
	if (value & PAE_PMU_STATUS_BUSY)
		return -EBUSY;
	ret = fh8626_pmu_write(PAE_PMU_RESET, ~PAE_PMU_RESET_BIT);
	for (i = 0; !ret && i < 100; i++) {
		udelay(10);
		ret = fh8626_pmu_read(PAE_PMU_RESET, &value);
		if (!ret && (value & PAE_PMU_RESET_BIT))
			return 0;
		/* Deassert explicitly if the command does not self-release. */
		if (!ret && i == 0)
			ret = fh8626_pmu_write(PAE_PMU_RESET, ~0U);
	}
	return ret ? ret : -ETIMEDOUT;
}

static int pae_reset(struct fh8626_pae_hw *hw)
{
	int ret;

	ret = pae_engine_reset(hw);
	if (!ret)
		return 0;
	pae_dump_state(hw, "engine reset timed out");
	ret = pae_block_reset(hw);
	if (!ret)
		ret = pae_engine_reset(hw);
	if (ret) {
		hw->poisoned = true;
		dev_err(hw->dev, "failed to reset engine: %d\n", ret);
		return ret;
	}
	dev_warn(hw->dev, "engine recovered by block reset\n");
	return 0;
}

struct fh8626_pae_hw *fh8626_pae_create(struct device *dev, void __iomem *core,
					void __iomem *working,
					void __iomem *sram,
					resource_size_t resource_start)
{
	struct fh8626_pae_hw *hw, *entry;
	int ret;

	mutex_lock(&pae_quarantine_lock);
	list_for_each_entry(entry, &pae_quarantine, quarantine) {
		if (entry->resource_start == resource_start) {
			mutex_unlock(&pae_quarantine_lock);
			return ERR_PTR(-EIO);
		}
	}
	mutex_unlock(&pae_quarantine_lock);
	hw = kzalloc(sizeof(*hw), GFP_KERNEL);
	if (!hw)
		return ERR_PTR(-ENOMEM);
	hw->dev = get_device(dev);
	hw->resource_start = resource_start;
	INIT_LIST_HEAD(&hw->quarantine);
	INIT_LIST_HEAD(&hw->retained);
	hw->core = core;
	hw->working = working;
	hw->sram = sram;
	mutex_init(&hw->engine_lock);
	spin_lock_init(&hw->lock);
	init_completion(&hw->completion);
	ret = pae_reset(hw);
	if (ret) {
		put_device(dev);
		kfree(hw);
		return ERR_PTR(ret);
	}
	return hw;
}

static int area_alloc(struct fh8626_pae_ctx *ctx, struct pae_area *area,
		      u32 bytes)
{
	struct device *dev = ctx->hw->dev;
	unsigned long flags;

	area->bytes = ALIGN(bytes, 1024);
	area->cpu = dma_alloc_coherent(dev, area->bytes, &area->dma,
				       GFP_KERNEL);
	if (!area->cpu)
		return -ENOMEM;
	if ((area->dma & 1023) || area->dma > DMA_BIT_MASK(PAE_DMA_BITS) ||
	    area->bytes - 1 > DMA_BIT_MASK(PAE_DMA_BITS) - area->dma) {
		dma_free_coherent(dev, area->bytes, area->cpu, area->dma);
		area->cpu = NULL;
		return -ERANGE;
	}
	memset(area->cpu, 0, area->bytes);
	spin_lock_irqsave(&ctx->hw->lock, flags);
	ctx->stats.dma_bytes += area->bytes;
	spin_unlock_irqrestore(&ctx->hw->lock, flags);
	return 0;
}

static void ctx_free(struct fh8626_pae_ctx *ctx)
{
	struct pae_area *areas[] = { &ctx->y, &ctx->uv, &ctx->ref_y,
		&ctx->ref_uv, &ctx->mv, &ctx->mb[0], &ctx->mb[1],
		&ctx->stream };
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(areas); i++) {
		struct pae_area *area = areas[i];

		if (area->cpu)
			dma_free_coherent(ctx->hw->dev, area->bytes, area->cpu,
					  area->dma);
	}
	kfree(ctx);
	module_put(THIS_MODULE);
}

struct fh8626_pae_ctx *fh8626_pae_ctx_create(struct fh8626_pae_hw *hw,
					     u32 width, u32 height, u32 stride,
					     u32 stream_capacity)
{
	u32 w = ALIGN(width, 32), h = ALIGN(height, 16);
	u32 mbw = w >> 4, mbh = h >> 4;
	u32 mv = max(mbw * ALIGN(mbh * 2, 64), mbh * ALIGN(mbw * 2, 64)) / 8;
	/* The reference ring holds whole 32-row blocks. */
	u32 ref_y = w * (ALIGN(h, 32) + PAE_REF_GUARD_ROWS);
	u32 ref_uv = ref_y / 2;
	u32 mb = mbw * mbh * 16, ring = ALIGN(stream_capacity + 1024, 1024);
	u32 lengths[] = { ref_y, ref_uv, mv, mb, mb, ring };
	struct fh8626_pae_ctx *ctx;
	struct pae_area *areas[6];
	unsigned int i;
	int ret;

	if (READ_ONCE(hw->poisoned))
		return ERR_PTR(-EIO);
	if (width < 64 || width > 1280 || height < 64 || height > 720 ||
	    (width & 1) || (height & 1) || stride < width || !stream_capacity)
		return ERR_PTR(-EINVAL);
	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return ERR_PTR(-ENOMEM);
	/*
	 * Pin before any address can become visible to hardware. A context
	 * retained after an engine failure keeps this reference.
	 */
	if (!try_module_get(THIS_MODULE)) {
		kfree(ctx);
		return ERR_PTR(-ENODEV);
	}
	ctx->hw = hw;
	INIT_LIST_HEAD(&ctx->retained);
	areas[0] = &ctx->ref_y;
	areas[1] = &ctx->ref_uv;
	areas[2] = &ctx->mv;
	areas[3] = &ctx->mb[0];
	areas[4] = &ctx->mb[1];
	areas[5] = &ctx->stream;
	/* Separate allocations avoid one large contiguous 720p workspace. */
	for (i = 0; i < ARRAY_SIZE(areas); i++) {
		ret = area_alloc(ctx, areas[i], lengths[i]);
		if (ret) {
			ctx_free(ctx);
			return ERR_PTR(ret);
		}
	}
	ctx->ref_blocks = (ALIGN(h, 32) + PAE_REF_GUARD_ROWS) / 32;
	ctx->width = width;
	ctx->height = height;
	ctx->stride = stride;
	ctx->coded_width = w;
	ctx->coded_height = h;
	mutex_lock(&hw->engine_lock);
	hw->contexts++;
	mutex_unlock(&hw->engine_lock);
	return ctx;
}

void fh8626_pae_ctx_destroy(struct fh8626_pae_ctx *ctx)
{
	struct fh8626_pae_hw *hw;
	bool retain;

	if (IS_ERR_OR_NULL(ctx))
		return;
	hw = ctx->hw;
	mutex_lock(&hw->engine_lock);
	/*
	 * Reset before freeing addresses held by the engine. Retain DMA storage
	 * if reset fails.
	 */
	if (!hw->poisoned)
		pae_reset(hw);
	retain = hw->poisoned;
	if (retain)
		list_add_tail(&ctx->retained, &hw->retained);
	hw->contexts--;
	mutex_unlock(&hw->engine_lock);
	if (!retain)
		ctx_free(ctx);
}

static int ctx_staging(struct fh8626_pae_ctx *ctx)
{
	u32 pixels = ctx->coded_width * ctx->coded_height;
	int ret;

	if (ctx->uv.cpu)
		return 0;
	if (!ctx->y.cpu) {
		ret = area_alloc(ctx, &ctx->y, pixels);
		if (ret)
			return ret;
	}
	return area_alloc(ctx, &ctx->uv, pixels / 2);
}

static void frame_addr(u8 *p, u32 field, const struct pae_area *area)
{
	put_unaligned_le32(area->dma >> 10, p + field);
}

/*
 * DMA reads ALIGN(height, 16) luma rows and half as many chroma rows, with
 * ALIGN(width, 32) stride. Direct buffers must include the extra rows.
 */
size_t fh8626_pae_direct_size(u32 width, u32 height, u32 stride)
{
	return (size_t)stride * (height + ALIGN(height, 16) / 2);
}

bool fh8626_pae_input_direct(u32 width, u32 height, u32 stride,
			     dma_addr_t base, size_t length)
{
	size_t need = fh8626_pae_direct_size(width, height, stride);
	dma_addr_t chroma = base + (dma_addr_t)stride * height;

	/* Plane addresses are programmed in 1 KiB units. */
	return stride == ALIGN(width, 32) && length >= need &&
	       !(base & 1023) && !(chroma & 1023) &&
	       base <= DMA_BIT_MASK(PAE_DMA_BITS) &&
	       need - 1 <= DMA_BIT_MASK(PAE_DMA_BITS) - base;
}

/*
 * The engine stores the two halves of each 16-byte QP row in two banks,
 * each interleaving complexity/P/I bytes in groups of eight. Entries 7..15
 * repeat the seventh region. The five policy rows have sixteen meaningful
 * bytes followed by an untouched separator.
 *
 * Each row is indexed by the frame QP and gives the QP used for macroblocks
 * of increasing texture complexity. The engine consults these rows only in
 * its background-assisted mode, which needs a per-macroblock
 * map from the BGM block; the table stays flat so every macroblock is coded
 * at the frame QP.
 */
static void write_coefficients(struct fh8626_pae_hw *hw, u32 coded_width)
{
	void __iomem *base = hw->working + (coded_width >> 4) * 0x150;
	unsigned int row, column, plane;
	static const u8 policy[5][16] = {
		{ 25, 50, 75, 100, 125, 150, 175, 175,
		  175, 175, 175, 175, 175, 175, 175, 175 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 36, 72, 108, 144, 180, 216, 252, 252,
		  252, 252, 252, 252, 252, 252, 252, 252 },
		{ 255, 255, 255, 255, 255, 255, 255, 255,
		  255, 255, 255, 255, 255, 255, 255, 255 },
		{ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
	};

	for (row = 0; row < 52; row++) {
		for (column = 0; column < 16; column++)
			for (plane = 0; plane < 3; plane++)
				writeb(row, base + (column < 8 ? 0 : 0x508) +
				       row * 24 + plane * 8 + (column & 7));
	}
	for (plane = 0; plane < 5; plane++)
		for (column = 0; column < 16; column++)
			writeb(policy[plane][column],
			       base + (column < 8 ? 0x4e0 : 0x9e8) +
			       plane * 8 + (column & 7));
}

static void copy_input(struct fh8626_pae_ctx *ctx, const u8 *nv12)
{
	u8 *y = ctx->y.cpu, *uv = ctx->uv.cpu;
	const u8 *chroma = nv12 + ctx->stride * ctx->height;
	u32 row, x, w = ctx->coded_width;

	for (row = 0; row < ctx->coded_height; row++) {
		const u8 *source = nv12 +
				   min(row, ctx->height - 1) * ctx->stride;

		memcpy(y + row * w, source, ctx->width);
		memset(y + row * w + ctx->width, source[ctx->width - 1],
		       w - ctx->width);
	}
	for (row = 0; row < ctx->coded_height / 2; row++) {
		const u8 *source = chroma +
				   min(row, ctx->height / 2 - 1) * ctx->stride;

		memcpy(uv + row * w, source, ctx->width);
		for (x = ctx->width; x < w; x += 2) {
			uv[row * w + x] = source[ctx->width - 2];
			uv[row * w + x + 1] = source[ctx->width - 1];
		}
	}
}

/*
 * Restore geometry, DMA addresses, QP and CABAC state on every context switch.
 */
static void program_frame(struct fh8626_pae_hw *hw, struct fh8626_pae_ctx *ctx,
			  const struct fh8626_pae_frame *frame, bool is_idr,
			  dma_addr_t y, dma_addr_t uv,
			  unsigned int ref_pos, unsigned int rec_pos,
			  const u8 *header, unsigned int header_bits)
{
	u8 *p = hw->params.bytes, *ring = ctx->stream.cpu;
	u32 blocks = ctx->ref_blocks;
	unsigned int mb = frame->mb_parity & 1;
	u8 qp = frame->qp;
	unsigned int i, bits;

	fh8626_pae_params_init(&hw->params);
	put_unaligned_le16(ctx->coded_width, p + 4);
	put_unaligned_le16(ctx->coded_height, p + 6);
	if (is_idr) {
		put_unaligned_le16(frame->idr_pic_id, p + 0x14);
		put_unaligned_le16(0, p + 0x16);
		p[0xc3] = 2; /* I slice */
	} else {
		put_unaligned_le16(0, p + 0x14);
		put_unaligned_le16(frame->frame_num, p + 0x16);
		p[0xc3] = 0; /* P slice */
	}
	/* Intra and inter slice QP; the slice type selects which applies. */
	p[0x0c] = qp;
	p[0x0d] = qp;
	/*
	 * The engine reads the macroblock QP window from the SRAM copy.
	 * Pin its minimum and maximum to the slice QP.
	 */
	put_unaligned_le16(((u32)qp << 4) | ((u32)qp << 10), p + 0x130);
	/*
	 * Deblocking filter: control-present flag, filter disable idc and
	 * the two 4-bit two's complement offsets. These settings must match
	 * the PPS and slice header.
	 */
	p[0x10] = frame->deblock.control_present;
	p[0x11] = frame->deblock.disable_idc & 3;
	p[0x12] = frame->deblock.alpha_c0_offset_div2 & 15;
	p[0x13] = frame->deblock.beta_offset_div2 & 15;
	p[0x0a] = 1;
	/* Input comes from memory. */
	p[0x8a] = 0;
	p[0x4c] = 0xb0;
	put_unaligned_le32(y >> 10, p + 0x134);
	put_unaligned_le32(uv >> 10, p + 0x138);
	/* Reference and reconstruction share the circular buffer. */
	frame_addr(p, 0x140, &ctx->ref_y);
	frame_addr(p, 0x144, &ctx->ref_uv);
	put_unaligned_le32((blocks - 1) | (ref_pos << 10) | (rec_pos << 20),
			   p + 0x148);
	frame_addr(p, 0x14c, &ctx->stream);
	put_unaligned_le16(ctx->stream.bytes >> 10, p + 0x15a);
	frame_addr(p, 0x164, &ctx->mv);
	/* Macroblock data of this frame, and of the previous one. */
	frame_addr(p, 0x168, &ctx->mb[mb]);
	frame_addr(p, 0x16c, &ctx->mb[mb ^ 1]);
	frame_addr(p, 0x1a4, &ctx->ref_y);
	frame_addr(p, 0x1a8, &ctx->ref_uv);
	/* Mark the picture as a reference and set the required control flag. */
	put_unaligned_le32((blocks - 1) | (ref_pos << 10) | 0x80800000,
			   p + 0x1ac);
	fh8626_pae_params_registers(&hw->params, &hw->registers);
	memcpy_toio(hw->sram + 0x7e00, p, sizeof(hw->params.bytes));
	if (is_idr) {
		memset_io(hw->sram + 0x7a00, 0, 0xf0);
	} else {
		memcpy_toio(hw->sram + 0x7a00, ctx->ec_prev,
			    sizeof(ctx->ec_prev));
		memcpy_toio(hw->sram + 0x7a78, ctx->ec_idr,
			    sizeof(ctx->ec_idr));
	}
	write_coefficients(hw, ctx->coded_width);
	memset(ring, 0, 16);
	ring[14] = 1;
	ring[15] = is_idr ? 0x65 : 0x41;

	writel(0x4000, hw->core + PAE_CTRL);
	writel(0x08010801, hw->core + 0x14);
	writel(PAE_IRQ_SLICE | PAE_IRQ_SPACE, hw->core + PAE_IRQ_STATUS);
	writel(0, hw->core + 0x10);
	/* Unmask all causes so unexpected interrupts are acknowledged. */
	writel(PAE_IRQ_ALL, hw->core + PAE_IRQ_ENABLE);
	for (i = 4; i <= 0xa0; i += 4)
		if (i != 0x48)
			writel(hw->registers.tae[i / 4],
			       hw->core + PAE_TAE + i);
	for (i = 0; i <= 0x60; i += 4)
		if (i != 0x50 && i != 0x5c)
			writel(hw->registers.dmu[i / 4],
			       hw->core + PAE_DMU + i);
	/* Slice QP, last macroblock row and the row-group size. */
	writel(qp | (((ctx->coded_height >> 4) - 1) << 16) |
	       ((ctx->coded_height / 16 / 3) << 24),
	       hw->core + PAE_TAE + 0x48);
	/*
	 * Enable CABAC with cabac_init_idc 0, the slice QP and 4:2:0 chroma.
	 */
	writel(1 | BIT(10) | ((u32)qp << 4), hw->core + PAE_TAE + 0xac);
	writel(16, hw->core + PAE_TAE + 0xa4);
	writel((ctx->stream.bytes - 16) >> 4, hw->core + PAE_TAE + 0xa8);
	writel(0x100000, hw->core + PAE_TAE + 0xd0);
	/* Select intra (I) or inter (P) slice coding. */
	writel(0xc00a0023 | (is_idr ? 0 : BIT(4)), hw->core + PAE_TAE + 0xd4);
	writel(0x01011000, hw->core + PAE_TAE + 0xd0);
	for (i = 0; i < header_bits; i += bits) {
		u32 value = 0;
		unsigned int j;

		bits = min(32U, header_bits - i);
		for (j = 0; j < bits / 8; j++)
			value = (value << 8) | header[i / 8 + j];
		writel(value, hw->core + PAE_TAE + 0xb0);
		writel(2 * bits - 1, hw->core + PAE_TAE + 0xd0);
	}
}

irqreturn_t fh8626_pae_irq(struct fh8626_pae_hw *hw)
{
	unsigned long flags;
	u32 status;

	spin_lock_irqsave(&hw->lock, flags);
	status = readl(hw->core + PAE_IRQ_STATUS);
	if (!status) {
		spin_unlock_irqrestore(&hw->lock, flags);
		return IRQ_NONE;
	}
	writel(status, hw->core + PAE_IRQ_STATUS);
	hw->irq_status |= status;
	hw->irq_count++;
	status &= PAE_IRQ_SLICE | PAE_IRQ_SPACE;
	if (hw->active) {
		/*
		 * PAE_IRQ_SLICE completes the slice.
		 * PAE_IRQ_SPACE reloads the output window.
		 */
		if (status & PAE_IRQ_SLICE) {
			hw->bits = readl(hw->core + PAE_TAE + 0x110);
			writel(PAE_TAE_FINISH, hw->core + PAE_TAE + 0xd4);
		}
		if (status & PAE_IRQ_SPACE) {
			writel(16, hw->core + PAE_TAE + 0xa4);
			writel((hw->stream_bytes - 16) >> 4,
			       hw->core + PAE_TAE + 0xa8);
			writel(0x100000, hw->core + PAE_TAE + 0xd0);
		}
		if (status & PAE_IRQ_SLICE) {
			writel(0, hw->core + PAE_IRQ_ENABLE);
			hw->done = ktime_get();
			hw->active = false;
			complete(&hw->completion);
		}
	}
	spin_unlock_irqrestore(&hw->lock, flags);
	return IRQ_HANDLED;
}

static void stats_add(struct fh8626_pae_stats *stats, bool copied,
		      u64 busy, u64 job)
{
	stats->frames++;
	stats->copied += copied;
	stats->busy_ns += busy;
	stats->busy_max_ns = max_t(u64, stats->busy_max_ns,
				   min_t(u64, busy, U32_MAX));
	stats->job_ns += job;
}

int fh8626_pae_encode_frame(struct fh8626_pae_ctx *ctx,
			    const struct fh8626_pae_frame *frame, bool *idr)
{
	struct fh8626_pae_hw *hw = ctx->hw;
	struct fh8626_pae_picture picture;
	const u8 *nv12 = frame->nv12;
	u8 *stream = frame->stream;
	size_t capacity = frame->capacity;
	bool is_idr = frame->idr;
	ktime_t start = ktime_get();
	u8 header[32];
	unsigned long flags;
	dma_addr_t y, uv;
	u32 bytes;
	u64 busy, job;
	unsigned int ref_pos = 0, rec_pos = 0;
	int prefix = 0, header_bits, ret;

	if (!stream || frame->qp > 51)
		return -EINVAL;
	if (!nv12 && !fh8626_pae_input_direct(ctx->width, ctx->height,
					      ctx->stride, frame->dma,
					      frame->dma_length))
		return -EINVAL;
	if (READ_ONCE(hw->poisoned))
		return -EIO;

	if (!is_idr && ctx->need_idr)
		is_idr = true;
	*idr = is_idr;

	if (!is_idr) {
		ref_pos = ctx->ref_pos;
		rec_pos = (ref_pos + ctx->ref_blocks - 8) % ctx->ref_blocks;
	}

	memset(&picture, 0, sizeof(picture));
	picture.width = ctx->width;
	picture.height = ctx->height;
	picture.coded_width = ctx->coded_width;
	picture.coded_height = ctx->coded_height;
	picture.qp = frame->qp;
	picture.idr = is_idr;
	picture.frame_num = is_idr ? 0 : frame->frame_num;
	picture.poc = is_idr ? 0 : frame->poc;
	picture.idr_pic_id = frame->idr_pic_id;
	picture.deblock = frame->deblock;
	picture.vui = frame->vui;

	if (is_idr) {
		prefix = fh8626_pae_parameter_sets(&picture, stream, capacity);
		if (prefix < 0)
			return prefix;
	}

	header_bits = fh8626_pae_slice_header(&picture, 0, header,
					      sizeof(header));
	if (header_bits < 0)
		return header_bits;

	if (nv12) {
		ret = ctx_staging(ctx);
		if (ret)
			return ret;
		copy_input(ctx, nv12);
		y = ctx->y.dma;
		uv = ctx->uv.dma;
	} else {
		y = frame->dma;
		uv = y + (dma_addr_t)ctx->stride * ctx->height;
	}
	memset(ctx->mv.cpu, 0, ctx->mv.bytes);

	mutex_lock(&hw->engine_lock);
	if (hw->poisoned) {
		mutex_unlock(&hw->engine_lock);
		return -EIO;
	}
	program_frame(hw, ctx, frame, is_idr, y, uv, ref_pos, rec_pos, header,
		      header_bits);
	reinit_completion(&hw->completion);
	spin_lock_irqsave(&hw->lock, flags);
	hw->bits = 0;
	hw->irq_status = 0;
	hw->irq_count = 0;
	hw->stream_bytes = ctx->stream.bytes;
	hw->active = true;
	ctx->need_idr = true;
	dma_wmb();
	hw->kick = ktime_get();
	writel(0x10000000, hw->core + PAE_TAE + 0xd0);
	spin_unlock_irqrestore(&hw->lock, flags);
	wait_for_completion_timeout(&hw->completion,
				    msecs_to_jiffies(PAE_TIMEOUT_MS));
	spin_lock_irqsave(&hw->lock, flags);
	bytes = (hw->bits >> 3) + !!(hw->bits & 7);
	ret = hw->active ? -ETIMEDOUT : 0;
	hw->active = false;
	writel(0, hw->core + PAE_IRQ_ENABLE);
	busy = ret ? 0 : ktime_to_ns(ktime_sub(hw->done, hw->kick));
	spin_unlock_irqrestore(&hw->lock, flags);
	if (ret) {
		pae_dump_state(hw, "frame timed out");
		pae_reset(hw);
		mutex_unlock(&hw->engine_lock);
		return ret;
	}
	/*
	 * Overflow leaves a partial reconstruction and an unusable reference.
	 */
	if (!bytes || bytes > ctx->stream.bytes - 16 ||
	    bytes + 5 > capacity - prefix) {
		ret = -ENOSPC;
	} else {
		dma_rmb();
		memcpy_fromio(ctx->ec_prev, hw->sram + 0x7a00,
			      sizeof(ctx->ec_prev));
	}
	mutex_unlock(&hw->engine_lock);

	if (!ret) {
		ctx->need_idr = false;
		if (is_idr)
			memcpy(ctx->ec_idr, ctx->ec_prev, sizeof(ctx->ec_idr));
		ctx->ref_pos = rec_pos;
		memcpy(stream + prefix, ctx->stream.cpu + 11, bytes + 5);
		ret = prefix + bytes + 5;
	}

	job = ktime_to_ns(ktime_sub(ktime_get(), start));
	spin_lock_irqsave(&hw->lock, flags);
	stats_add(&hw->stats, nv12, busy, job);
	stats_add(&ctx->stats, nv12, busy, job);
	spin_unlock_irqrestore(&hw->lock, flags);
	dev_dbg(hw->dev, "%ux%u %c qp %u: %d bytes, engine %llu us\n",
		ctx->width, ctx->height, is_idr ? 'I' : 'P', frame->qp, ret,
		div_u64(busy, NSEC_PER_USEC));
	return ret;
}

int fh8626_pae_skip_frame(struct fh8626_pae_ctx *ctx,
			  const struct fh8626_pae_frame *frame)
{
	struct fh8626_pae_picture picture;

	if (!frame->stream || frame->idr)
		return -EINVAL;

	memset(&picture, 0, sizeof(picture));
	picture.width = ctx->width;
	picture.height = ctx->height;
	picture.coded_width = ctx->coded_width;
	picture.coded_height = ctx->coded_height;
	picture.qp = frame->qp;
	picture.frame_num = frame->frame_num;
	picture.poc = frame->poc;
	picture.non_reference = true;
	picture.deblock = frame->deblock;
	return fh8626_pae_skip_picture(&picture, frame->stream,
				       frame->capacity);
}

void fh8626_pae_get_stats(struct fh8626_pae_hw *hw,
			  struct fh8626_pae_stats *stats)
{
	unsigned long flags;

	spin_lock_irqsave(&hw->lock, flags);
	*stats = hw->stats;
	spin_unlock_irqrestore(&hw->lock, flags);
}

void fh8626_pae_ctx_get_stats(struct fh8626_pae_ctx *ctx,
			      struct fh8626_pae_stats *stats)
{
	unsigned long flags;

	spin_lock_irqsave(&ctx->hw->lock, flags);
	*stats = ctx->stats;
	spin_unlock_irqrestore(&ctx->hw->lock, flags);
}

bool fh8626_pae_poisoned(struct fh8626_pae_hw *hw)
{
	return READ_ONCE(hw->poisoned);
}

bool fh8626_pae_destroy(struct fh8626_pae_hw *hw)
{
	bool quarantine;

	if (!hw)
		return false;
	mutex_lock(&hw->engine_lock);
	WARN_ON(hw->contexts);
	if (!hw->poisoned)
		pae_reset(hw);
	/* Retained contexts exist only once the engine is poisoned. */
	quarantine = hw->poisoned && !list_empty(&hw->retained);
	mutex_unlock(&hw->engine_lock);
	if (quarantine) {
		mutex_lock(&pae_quarantine_lock);
		list_add_tail(&hw->quarantine, &pae_quarantine);
		mutex_unlock(&pae_quarantine_lock);
		return true;
	}
	put_device(hw->dev);
	kfree(hw);
	return false;
}
