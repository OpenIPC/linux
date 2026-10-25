/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2026 Difan Zhang
 */
#ifndef _FH8626_BGM_HW_H
#define _FH8626_BGM_HW_H

#include <linux/types.h>
#include <linux/io.h>

struct device;
struct fh8626_bgm_hw;

#define FH8626_BGM_MIN_WIDTH	8U
#define FH8626_BGM_MAX_WIDTH	240U
#define FH8626_BGM_MIN_HEIGHT	8U
#define FH8626_BGM_MAX_HEIGHT	512U
#define FH8626_BGM_ALIGNMENT	8U
#define FH8626_BGM_STAT_COUNT	18
/* Foreground when diff^2 > threshold * variance / 16. */
#define FH8626_BGM_DEFAULT_THRESHOLD	100U

enum fh8626_bgm_map {
	FH8626_BGM_DIS,
	FH8626_BGM_FG,
	FH8626_BGM_RUNNUM,
	FH8626_BGM_RUNPOS0,
	FH8626_BGM_RUNPOS1,
	FH8626_BGM_DIFF,
	FH8626_BGM_MAP_COUNT,
};

struct fh8626_bgm_hw_result {
	struct {
		const void *data;
		u32 length;
	} map[FH8626_BGM_MAP_COUNT];
	u32 stats[FH8626_BGM_STAT_COUNT];
};

/*
 * Clocks and MMIO must remain valid through destroy(). Calls are serialized
 * by the frontend; process() sleeps while waiting for DONE and PMU idle.
 * Returned map pointers remain valid until the next process(), free() or
 * destroy().
 * No user buffer is exposed to DMA. A poisoned device cannot be restarted.
 */
struct fh8626_bgm_hw *fh8626_bgm_hw_create(struct device *dev,
					   void __iomem *regs, int irq,
					   resource_size_t resource_start);
int fh8626_bgm_hw_alloc(struct fh8626_bgm_hw *hw, u32 width, u32 height);
int fh8626_bgm_hw_process(struct fh8626_bgm_hw *hw, const void *grey,
			  struct fh8626_bgm_hw_result *result);
int fh8626_bgm_hw_stop(struct fh8626_bgm_hw *hw);
void fh8626_bgm_hw_free(struct fh8626_bgm_hw *hw);
void fh8626_bgm_hw_set_threshold(struct fh8626_bgm_hw *hw, u32 threshold);
void fh8626_bgm_hw_reinit_background(struct fh8626_bgm_hw *hw);
/* Consumes hw. True means DMA was quarantined: keep clocks enabled and held. */
bool fh8626_bgm_hw_destroy(struct fh8626_bgm_hw *hw);
bool fh8626_bgm_hw_poisoned(struct fh8626_bgm_hw *hw);

#endif
