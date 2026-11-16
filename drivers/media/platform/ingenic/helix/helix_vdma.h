/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Difan Zhang */

#ifndef _HELIX_VDMA_H_
#define _HELIX_VDMA_H_

#include <linux/types.h>
#include <linux/dma-mapping.h>
#include "helix_hw.h"

struct helix_dev;

struct helix_vdma_task {
	__le32 *vaddr;
	dma_addr_t paddr;
	size_t max_words;
	size_t count;
	bool overflow;
};

struct helix_h264_buffers {
	void *recon_y_vaddr[2];
	dma_addr_t recon_y_paddr[2];
	void *recon_c_vaddr[2];
	dma_addr_t recon_c_paddr[2];
	size_t recon_y_size;
	size_t recon_c_size;

	int cur_slot;
};

void helix_vdma_task_init(struct helix_vdma_task *task, __le32 *vaddr,
			  dma_addr_t paddr, size_t max_bytes);
void helix_vdma_append(struct helix_vdma_task *task, u32 reg, u32 val);
void helix_vdma_terminate(struct helix_vdma_task *task);
int helix_vdma_kick(struct helix_dev *hdev, struct helix_vdma_task *task);
int helix_vdma_wait(struct helix_dev *hdev, unsigned int timeout_ms);

int helix_alloc_h264_buffers(struct helix_dev *hdev,
			     struct helix_h264_buffers *bufs,
			     u32 width, u32 height);
void helix_free_h264_buffers(struct helix_dev *hdev,
			     struct helix_h264_buffers *bufs);

#endif
