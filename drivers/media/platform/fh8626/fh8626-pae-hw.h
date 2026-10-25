/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2026 Difan Zhang
 */
#ifndef FH8626_PAE_HW_H
#define FH8626_PAE_HW_H
#include <linux/interrupt.h>
#include <linux/types.h>
#include "fh8626-pae-bitstream.h"

struct device;
struct fh8626_pae_hw;
struct fh8626_pae_ctx;

/*
 * Copy nv12 into staging storage, or use dma/dma_length when nv12 is NULL.
 * Direct input must satisfy fh8626_pae_input_direct().
 */
struct fh8626_pae_frame {
	const u8 *nv12;
	dma_addr_t dma;
	size_t dma_length;
	u8 *stream;
	size_t capacity;
	u8 qp;
	u16 frame_num;
	u16 poc;
	u16 idr_pic_id;
	bool idr;
	/* Selects the macroblock buffer pair; advances on hardware frames. */
	unsigned int mb_parity;
	struct fh8626_pae_deblock deblock;
	/* Emitted with the SPS of IDR frames; may be NULL. */
	const struct fh8626_pae_vui *vui;
};

struct fh8626_pae_stats {
	u64 frames;
	/* Frames that went through the staging copy. */
	u64 copied;
	/* Engine time from start to completion interrupt. */
	u64 busy_ns;
	u32 busy_max_ns;
	/* Whole submissions, including input copy, programming and output. */
	u64 job_ns;
	/* Coherent memory held, in bytes (contexts only). */
	size_t dma_bytes;
};

struct fh8626_pae_hw *fh8626_pae_create(struct device *dev, void __iomem *core,
					void __iomem *working,
					void __iomem *sram,
					resource_size_t resource_start);
/*
 * Allocates the reference, motion and stream areas of one coded sequence.
 * Several contexts can share the engine; each frame is programmed in full.
 */
struct fh8626_pae_ctx *fh8626_pae_ctx_create(struct fh8626_pae_hw *hw,
					     u32 width, u32 height, u32 stride,
					     u32 stream_capacity);
/*
 * All frames must have completed. A failed reset retains the DMA storage.
 */
void fh8626_pae_ctx_destroy(struct fh8626_pae_ctx *ctx);
size_t fh8626_pae_direct_size(u32 width, u32 height, u32 stride);
/*
 * Whether the engine can read a contiguous NV12 picture of the given
 * geometry, with chroma at stride * height, in place at bus address base.
 */
bool fh8626_pae_input_direct(u32 width, u32 height, u32 stride,
			     dma_addr_t base, size_t length);
/*
 * Returns the Annex-B payload length; *idr reports the actual slice type.
 * Encodes an IDR when no prediction reference exists, regardless of frame->idr.
 */
int fh8626_pae_encode_frame(struct fh8626_pae_ctx *ctx,
			    const struct fh8626_pae_frame *frame, bool *idr);
/*
 * Writes a non-reference P frame of skipped macroblocks with the frame_num,
 * poc, QP and deblocking settings of frame, without touching the engine.
 * Decoders repeat their current reference; the engine's reference and the
 * frame_num of the next reference frame stay unchanged.
 */
int fh8626_pae_skip_frame(struct fh8626_pae_ctx *ctx,
			  const struct fh8626_pae_frame *frame);
void fh8626_pae_get_stats(struct fh8626_pae_hw *hw,
			  struct fh8626_pae_stats *stats);
void fh8626_pae_ctx_get_stats(struct fh8626_pae_ctx *ctx,
			      struct fh8626_pae_stats *stats);
irqreturn_t fh8626_pae_irq(struct fh8626_pae_hw *hw);
bool fh8626_pae_poisoned(struct fh8626_pae_hw *hw);
/*
 * Consumes hw once every context is destroyed. True means DMA was
 * quarantined: keep clocks enabled and held.
 */
bool fh8626_pae_destroy(struct fh8626_pae_hw *hw);
#endif
