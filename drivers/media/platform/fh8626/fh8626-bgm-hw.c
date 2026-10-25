// SPDX-License-Identifier: GPL-2.0
/*
 * FH8626 BGM single-Gaussian background model, software-triggered backend.
 *
 * All device-visible storage, including both input frames, is private.
 *
 * Copyright (C) 2026 Difan Zhang
 */
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/soc/fullhan/fh8626-pmu.h>
#include <linux/string.h>

#include "fh8626-bgm-hw.h"

#define BGM_CONTROL		0x00
#define BGM_STATUS		0x04
#define BGM_IRQ_ENABLE		0x08
#define BGM_KICK		0x2c
#define BGM_PACKAGE_ADDR	0x38
#define BGM_STOP		BIT(0)
#define BGM_SW_MODE		0x112
#define BGM_IRQ_START		BIT(0)
#define BGM_IRQ_DONE		BIT(7)
#define BGM_PACKAGE_SIZE	0x100
#define BGM_PACKAGE_REG_SIZE	0xcc
#define BGM_DMA_ALIGNMENT	128U
#define BGM_TIMEOUT_MS		1000
#define BGM_GAUSSIAN_REFRESH	1300U
#define BGM_PMU_GATE_STATE	0x2038
#define BGM_PMU_GATE_BLOCKED	BIT(12)

struct bgm_area {
	void *cpu;
	dma_addr_t dma;
	u32 size;
};

struct fh8626_bgm_hw {
	struct list_head quarantine;
	resource_size_t resource_start;
	bool module_pinned;
	struct device *dev;
	void __iomem *regs;
	int irq;
	spinlock_t lock;
	struct completion done;
	bool inflight;
	bool frame_done;
	bool poisoned;
	bool seeded;
	bool enabled;
	u32 width;
	u32 height;
	u32 frames;
	unsigned int input_slot;
	void *workspace;
	dma_addr_t workspace_dma;
	size_t workspace_size;
	struct bgm_area package;
	struct bgm_area gaussian;
	struct bgm_area state;
	struct bgm_area background;
	struct bgm_area input[2];
	struct bgm_area output[FH8626_BGM_MAP_COUNT];
	u32 threshold;
	bool threshold_dirty;
	u32 stats[FH8626_BGM_STAT_COUNT];
};

/* Retained engines cannot be rebound while their DMA stop is unconfirmed. */
static DEFINE_MUTEX(bgm_quarantine_lock);
static LIST_HEAD(bgm_quarantine);

static void bgm_pkg_write(struct fh8626_bgm_hw *hw, u32 offset, u32 value)
{
	((__le32 *)hw->package.cpu)[offset / 4] = cpu_to_le32(value);
}

static void bgm_pkg_update(struct fh8626_bgm_hw *hw, u32 offset,
			   u32 mask, u32 value)
{
	u32 old = le32_to_cpu(((__le32 *)hw->package.cpu)[offset / 4]);

	bgm_pkg_write(hw, offset, (old & ~mask) | (value & mask));
}

/* Package offsets are relative to BGM_PACKAGE_ADDR. */
static void bgm_default_parameters(struct fh8626_bgm_hw *hw)
{
	static const struct {
		u32 neighbourhood;
		u8 threshold;
		u8 enabled;
	} expansion[] = {
		{ 0x000739c0, 3, 0 }, /* FGA */
		{ 0x000739c0, 3, 0 }, /* distance */
		{ 0x01ffffff, 4, 0 }, /* FGB */
		{ 0x000739c0, 6, 1 }, /* local FGA */
		{ 0x000739c0, 4, 0 }, /* local FGB */
		{ 0x000739c0, 6, 1 }, /* local FGA1 */
		{ 0x000739c0, 4, 0 }, /* local FGB1 */
	};
	unsigned int i;
	u32 th;

	/* Detection mode, no histogram warmup and no multi-Gaussian engine. */
	bgm_pkg_update(hw, 0x04, BIT(0) | BIT(8) | BIT(16), BIT(0));
	bgm_pkg_update(hw, 0x08, 0x01ff01ff,
		       (hw->width - 1) | ((hw->height - 1) << 16));
	bgm_pkg_update(hw, 0x0c, 0x00ff1f1f, 16 | (10 << 8) | (50 << 16));
	bgm_pkg_update(hw, 0x10, 0x000033ff, 16);
	/*
	 * Single-Gaussian decision: a sample is foreground when
	 * diff^2 > threshold * variance / 16, with the variance clamped to
	 * [4, 100]. The default threshold of 100 is 2.5 sigma.
	 */
	bgm_pkg_update(hw, 0x14, 0x00ffffff,
		       hw->threshold | (4 << 8) | (100 << 16));
	/* Multi-Gaussian engine stays disabled and has no DMA storage. */
	bgm_pkg_update(hw, 0x1c, 0x3ff33ff6,
		       (32 << 4) | (32 << 20) | BIT(2));
	bgm_pkg_write(hw, 0x20, 100 | (25 << 8) | (4 << 16) | (100 << 24));
	bgm_pkg_update(hw, 0x24, 0x63ffffff,
		       25 | (25 << 8) | (160 << 16));
	bgm_pkg_write(hw, 0x28, 100 | (4 << 8) | (2 << 16) | (25 << 24));
	bgm_pkg_update(hw, 0x2c, 0x1f01ffff, 4 | (10 << 8) | (20 << 24));
	/*
	 * Frame-difference thresholds leave the foreground map unchanged.
	 */
	th = 15;
	bgm_pkg_write(hw, 0x18,
		      th | (th << 8) | (10 << 16) | (200U << 24));
	bgm_pkg_update(hw, 0x30, 0xffff7df0,
		       (20 << 4) | (20 << 10) | (th << 16) | (th << 24));
	for (i = 0; i < ARRAY_SIZE(expansion); i++)
		bgm_pkg_update(hw, 0x34 + 4 * i, 0xfdffffff,
			       expansion[i].neighbourhood |
			       ((u32)expansion[i].threshold << 26) |
			       ((u32)expansion[i].enabled << 31));
}

static void bgm_program(struct fh8626_bgm_hw *hw)
{
	unsigned int i;
	/* DMA unit burst layout for this engine revision. */
	static const u32 dmu[] = {
		0x20181008, 0x40383028, 0x00005048, 0x20181008,
		0x00383028, 0x08080808, 0x08080808,
	};

	/* Software-triggered mode, held stopped until programming is done. */
	writel(BGM_SW_MODE | BGM_STOP, hw->regs + BGM_CONTROL);
	writel(0, hw->regs + BGM_IRQ_ENABLE);
	writel(0, hw->regs + BGM_KICK);
	writel(0, hw->regs + 0x30);
	writel(lower_32_bits(hw->package.dma), hw->regs + BGM_PACKAGE_ADDR);
	writel(0x10000, hw->regs + 0x3c);
	writel(0, hw->regs + 0x88); /* histogram initialization off */
	writel(0, hw->regs + 0x8c); /* multi-Gaussian off */
	writel(lower_32_bits(hw->gaussian.dma), hw->regs + 0x90);
	writel(lower_32_bits(hw->background.dma), hw->regs + 0x9c);
	writel(lower_32_bits(hw->state.dma), hw->regs + 0xa0);
	memcpy_fromio(hw->package.cpu, hw->regs + 0x38, BGM_PACKAGE_REG_SIZE);
	for (i = 0; i < ARRAY_SIZE(dmu); i++)
		writel(dmu[i], hw->regs + 0x0c + i * 4);
	bgm_default_parameters(hw);
	/* DIFF precedes RUNNUM in the hardware package. */
	bgm_pkg_write(hw, 0x6c,
		      lower_32_bits(hw->output[FH8626_BGM_DIS].dma));
	bgm_pkg_write(hw, 0x70,
		      lower_32_bits(hw->output[FH8626_BGM_FG].dma));
	bgm_pkg_write(hw, 0x74,
		      lower_32_bits(hw->output[FH8626_BGM_DIFF].dma));
	bgm_pkg_write(hw, 0x78,
		      lower_32_bits(hw->output[FH8626_BGM_RUNNUM].dma));
	bgm_pkg_write(hw, 0x7c,
		      lower_32_bits(hw->output[FH8626_BGM_RUNPOS0].dma));
	bgm_pkg_write(hw, 0x80,
		      lower_32_bits(hw->output[FH8626_BGM_RUNPOS1].dma));
	writel(readl(hw->regs + BGM_STATUS), hw->regs + BGM_STATUS);
}

static void bgm_seed_gaussian(struct fh8626_bgm_hw *hw, const u8 *grey)
{
	__le32 *sg = hw->gaussian.cpu;
	u32 i, pixels = hw->width * hw->height;

	for (i = 0; i < pixels; i++) {
		sg[2 * i] = cpu_to_le32((u32)grey[i] << 13);
		sg[2 * i + 1] = cpu_to_le32(0x32000);
	}
}

static void bgm_read_stats(struct fh8626_bgm_hw *hw)
{
	unsigned int i;
	static const u16 offset[FH8626_BGM_STAT_COUNT] = {
		0xbc, 0xc0, 0xc4, 0xc8, 0xcc, 0xd0, 0xd4, 0xd8,
		0xdc, 0xf0, 0xe0, 0xf4, 0xe4, 0xf8, 0xe8, 0xfc,
		0xec, 0x100,
	};
	static const u32 mask[FH8626_BGM_STAT_COUNT] = {
		0x3ffff, 0x3ffff, 0x3ffff, 0xffff, 0xffff,
		0x3ffffff, 0x3ffffff, 0xffff, 0x3ffff, 0x3ffff,
		0x3ffff, 0x3ffff, 0x3ffff, 0x3ffff,
		0x3ffffff, 0x3ffffff, 0x3ffffff, 0x3ffffff,
	};

	for (i = 0; i < FH8626_BGM_STAT_COUNT; i++)
		hw->stats[i] = readl(hw->regs + offset[i]) & mask[i];
}

static irqreturn_t bgm_irq(int irq, void *data)
{
	struct fh8626_bgm_hw *hw = data;
	unsigned long flags;
	u32 status;

	spin_lock_irqsave(&hw->lock, flags);
	status = readl(hw->regs + BGM_STATUS);
	if (!status) {
		spin_unlock_irqrestore(&hw->lock, flags);
		return IRQ_NONE;
	}
	writel(status, hw->regs + BGM_STATUS);
	if ((status & BGM_IRQ_DONE) && hw->inflight && !hw->poisoned) {
		bgm_read_stats(hw);
		/*
		 * After DONE, process context waits for BGM_PMU_GATE_BLOCKED in
		 * BGM_PMU_GATE_STATE to clear before releasing DMA storage.
		 */
		writel(BGM_SW_MODE | BGM_STOP, hw->regs + BGM_CONTROL);
		hw->frame_done = true;
		complete(&hw->done);
	}
	spin_unlock_irqrestore(&hw->lock, flags);
	return IRQ_HANDLED;
}

static void bgm_area_assign(struct fh8626_bgm_hw *hw, struct bgm_area *area,
			    u32 bytes, size_t *offset)
{
	area->cpu = hw->workspace + *offset;
	area->dma = hw->workspace_dma + *offset;
	area->size = ALIGN(bytes, BGM_DMA_ALIGNMENT);
	*offset += area->size;
}

struct fh8626_bgm_hw *fh8626_bgm_hw_create(struct device *dev,
					   void __iomem *regs, int irq,
					   resource_size_t resource_start)
{
	struct fh8626_bgm_hw *hw, *entry;
	int ret;

	mutex_lock(&bgm_quarantine_lock);
	list_for_each_entry(entry, &bgm_quarantine, quarantine) {
		if (entry->resource_start == resource_start) {
			mutex_unlock(&bgm_quarantine_lock);
			return ERR_PTR(-EIO);
		}
	}
	mutex_unlock(&bgm_quarantine_lock);
	hw = kzalloc(sizeof(*hw), GFP_KERNEL);
	if (!hw)
		return ERR_PTR(-ENOMEM);
	hw->dev = dev;
	hw->resource_start = resource_start;
	INIT_LIST_HEAD(&hw->quarantine);
	hw->regs = regs;
	hw->irq = irq;
	spin_lock_init(&hw->lock);
	init_completion(&hw->done);
	hw->threshold = FH8626_BGM_DEFAULT_THRESHOLD;
	writel(0, regs + BGM_IRQ_ENABLE);
	writel(BGM_SW_MODE | BGM_STOP, regs + BGM_CONTROL);
	writel(readl(regs + BGM_STATUS), regs + BGM_STATUS);
	ret = request_irq(irq, bgm_irq, 0, "fh8626-bgm", hw);
	if (ret) {
		kfree(hw);
		return ERR_PTR(ret);
	}
	return hw;
}

int fh8626_bgm_hw_alloc(struct fh8626_bgm_hw *hw, u32 width, u32 height)
{
	u32 pixels, lengths[FH8626_BGM_MAP_COUNT];
	size_t size, offset = 0;
	unsigned int i;

	if (hw->poisoned)
		return -EIO;
	if (hw->workspace)
		return -EBUSY;
	if (width < FH8626_BGM_MIN_WIDTH || width > FH8626_BGM_MAX_WIDTH ||
	    height < FH8626_BGM_MIN_HEIGHT || height > FH8626_BGM_MAX_HEIGHT ||
	    (width | height) & (FH8626_BGM_ALIGNMENT - 1))
		return -EINVAL;
	pixels = width * height;
	lengths[FH8626_BGM_DIS] = pixels / 8;
	lengths[FH8626_BGM_FG] = pixels / 4;
	lengths[FH8626_BGM_RUNNUM] = height * 4;
	lengths[FH8626_BGM_RUNPOS0] = pixels * 2;
	lengths[FH8626_BGM_RUNPOS1] = pixels * 2;
	lengths[FH8626_BGM_DIFF] = pixels / 2;
	size = BGM_PACKAGE_SIZE + ALIGN(pixels * 8, BGM_DMA_ALIGNMENT) +
		4 * ALIGN(pixels, BGM_DMA_ALIGNMENT);
	for (i = 0; i < FH8626_BGM_MAP_COUNT; i++)
		size += ALIGN(lengths[i], BGM_DMA_ALIGNMENT);
	/* Keep code and fault state resident until DMA ownership returns. */
	if (!try_module_get(THIS_MODULE))
		return -ENODEV;
	hw->module_pinned = true;
	hw->workspace = dma_alloc_coherent(hw->dev, size,
					   &hw->workspace_dma, GFP_KERNEL);
	if (!hw->workspace) {
		hw->module_pinned = false;
		module_put(THIS_MODULE);
		return -ENOMEM;
	}
	/* Every hardware pointer is one 32-bit word. */
	if (upper_32_bits(hw->workspace_dma) ||
	    upper_32_bits(hw->workspace_dma + size - 1) ||
	    (hw->workspace_dma & (BGM_DMA_ALIGNMENT - 1))) {
		dma_free_coherent(hw->dev, size, hw->workspace,
				  hw->workspace_dma);
		hw->workspace = NULL;
		hw->module_pinned = false;
		module_put(THIS_MODULE);
		return -ERANGE;
	}
	get_device(hw->dev);
	hw->workspace_size = size;
	memset(hw->workspace, 0, size);
	bgm_area_assign(hw, &hw->package, BGM_PACKAGE_SIZE, &offset);
	bgm_area_assign(hw, &hw->gaussian, pixels * 8, &offset);
	bgm_area_assign(hw, &hw->state, pixels, &offset);
	bgm_area_assign(hw, &hw->background, pixels, &offset);
	bgm_area_assign(hw, &hw->input[0], pixels, &offset);
	bgm_area_assign(hw, &hw->input[1], pixels, &offset);
	for (i = 0; i < FH8626_BGM_MAP_COUNT; i++)
		bgm_area_assign(hw, &hw->output[i], lengths[i], &offset);
	hw->width = width;
	hw->height = height;
	hw->frames = 0;
	hw->input_slot = 0;
	hw->seeded = false;
	hw->enabled = true;
	bgm_program(hw);
	return 0;
}

int fh8626_bgm_hw_process(struct fh8626_bgm_hw *hw, const void *grey,
			  struct fh8626_bgm_hw_result *result)
{
	unsigned long flags;
	unsigned int i, previous;
	bool timed_out;
	unsigned long deadline;
	u32 gate_state;
	int ret;

	if (!grey || !result)
		return -EINVAL;
	if (hw->poisoned)
		return -EIO;
	if (!hw->workspace || !hw->enabled)
		return -EPIPE;
	memcpy(hw->input[hw->input_slot].cpu, grey, hw->width * hw->height);
	previous = hw->input_slot ^ 1;
	if (!hw->seeded) {
		memcpy(hw->background.cpu, grey, hw->width * hw->height);
		memcpy(hw->input[previous].cpu, grey, hw->width * hw->height);
		bgm_seed_gaussian(hw, grey);
		hw->seeded = true;
	}
	/* Maps include aligned padding; clear it so no stale bytes leak. */
	for (i = 0; i < FH8626_BGM_MAP_COUNT; i++)
		memset(hw->output[i].cpu, 0, hw->output[i].size);
	bgm_pkg_write(hw, 0x60, lower_32_bits(hw->input[hw->input_slot].dma));
	bgm_pkg_write(hw, 0x5c, lower_32_bits(hw->input[previous].dma));
	reinit_completion(&hw->done);
	spin_lock_irqsave(&hw->lock, flags);
	writel(readl(hw->regs + BGM_STATUS), hw->regs + BGM_STATUS);
	/* Update the package before the engine fetches it at kick. */
	if (hw->threshold_dirty) {
		bgm_pkg_update(hw, 0x14, 0x000000ff, hw->threshold);
		hw->threshold_dirty = false;
	}
	hw->inflight = true;
	hw->frame_done = false;
	dma_wmb();
	writel(BGM_IRQ_START | BGM_IRQ_DONE, hw->regs + BGM_IRQ_ENABLE);
	writel(BGM_SW_MODE, hw->regs + BGM_CONTROL);
	writel(readl(hw->regs + BGM_KICK) | BIT(0), hw->regs + BGM_KICK);
	spin_unlock_irqrestore(&hw->lock, flags);
	wait_for_completion_timeout(&hw->done,
				    msecs_to_jiffies(BGM_TIMEOUT_MS));
	/* After DONE, wait for the PMU to stop blocking clock gating. */
	ret = -ETIMEDOUT;
	if (READ_ONCE(hw->frame_done)) {
		deadline = jiffies + msecs_to_jiffies(BGM_TIMEOUT_MS);
		do {
			ret = fh8626_pmu_read(BGM_PMU_GATE_STATE,
					      &gate_state);
			if (ret || !(gate_state & BGM_PMU_GATE_BLOCKED))
				break;
			ret = -ETIMEDOUT;
			usleep_range(100, 200);
		} while (time_before(jiffies, deadline));
	}
	spin_lock_irqsave(&hw->lock, flags);
	timed_out = !hw->frame_done || ret;
	if (!timed_out)
		hw->inflight = false;
	if (timed_out) {
		/*
		 * Without DONE and PMU idle, a stop request cannot release DMA.
		 */
		hw->poisoned = true;
		hw->enabled = false;
		writel(BGM_SW_MODE | BGM_STOP, hw->regs + BGM_CONTROL);
		writel(0, hw->regs + BGM_IRQ_ENABLE);
	}
	spin_unlock_irqrestore(&hw->lock, flags);
	if (timed_out) {
		if (hw->frame_done)
			dev_err(hw->dev, "engine still busy after completion\n");
		else
			dev_err(hw->dev, "frame timed out\n");
		return ret ?: -ETIMEDOUT;
	}
	dma_rmb();
	for (i = 0; i < FH8626_BGM_MAP_COUNT; i++) {
		result->map[i].data = hw->output[i].cpu;
		result->map[i].length = hw->output[i].size;
	}
	memcpy(result->stats, hw->stats, sizeof(hw->stats));
	hw->frames++;
	/* Reseed the Gaussian state periodically; keep the background image. */
	if (!(hw->frames % BGM_GAUSSIAN_REFRESH))
		bgm_seed_gaussian(hw, hw->input[hw->input_slot].cpu);
	hw->input_slot ^= 1;
	return 0;
}

int fh8626_bgm_hw_stop(struct fh8626_bgm_hw *hw)
{
	unsigned long flags;
	bool busy;

	spin_lock_irqsave(&hw->lock, flags);
	hw->enabled = false;
	busy = hw->inflight;
	writel(BGM_SW_MODE | BGM_STOP, hw->regs + BGM_CONTROL);
	writel(0, hw->regs + BGM_IRQ_ENABLE);
	if (busy)
		hw->poisoned = true;
	spin_unlock_irqrestore(&hw->lock, flags);
	synchronize_irq(hw->irq);
	return hw->poisoned ? -EIO : 0;
}

void fh8626_bgm_hw_free(struct fh8626_bgm_hw *hw)
{
	if (!hw->workspace)
		return;
	if (fh8626_bgm_hw_stop(hw))
		return;
	dma_free_coherent(hw->dev, hw->workspace_size, hw->workspace,
			  hw->workspace_dma);
	hw->workspace = NULL;
	hw->workspace_size = 0;
	hw->seeded = false;
	put_device(hw->dev);
	hw->module_pinned = false;
	module_put(THIS_MODULE);
}

void fh8626_bgm_hw_set_threshold(struct fh8626_bgm_hw *hw, u32 threshold)
{
	unsigned long flags;
	u32 th;

	if (!hw)
		return;

	th = clamp_val(threshold, 1U, 255U);
	spin_lock_irqsave(&hw->lock, flags);
	/* The engine reads the package during a frame; applied at next kick. */
	hw->threshold = th;
	hw->threshold_dirty = true;
	spin_unlock_irqrestore(&hw->lock, flags);
}

void fh8626_bgm_hw_reinit_background(struct fh8626_bgm_hw *hw)
{
	unsigned long flags;

	if (!hw)
		return;

	spin_lock_irqsave(&hw->lock, flags);
	hw->seeded = false;
	spin_unlock_irqrestore(&hw->lock, flags);
}

bool fh8626_bgm_hw_poisoned(struct fh8626_bgm_hw *hw)
{
	return READ_ONCE(hw->poisoned);
}

bool fh8626_bgm_hw_destroy(struct fh8626_bgm_hw *hw)
{
	if (!hw)
		return false;
	fh8626_bgm_hw_stop(hw);
	free_irq(hw->irq, hw);
	if (hw->poisoned && hw->module_pinned) {
		mutex_lock(&bgm_quarantine_lock);
		list_add_tail(&hw->quarantine, &bgm_quarantine);
		mutex_unlock(&bgm_quarantine_lock);
		return true;
	}
	if (hw->workspace) {
		dma_free_coherent(hw->dev, hw->workspace_size,
				  hw->workspace, hw->workspace_dma);
		put_device(hw->dev);
		hw->module_pinned = false;
		module_put(THIS_MODULE);
	}
	kfree(hw);
	return false;
}
