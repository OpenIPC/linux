/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Difan Zhang */

#ifndef _HELIX_H264_H_
#define _HELIX_H264_H_

#include <linux/types.h>

struct helix_dev;
struct helix_ctx;

ssize_t helix_h264_write_sps(struct helix_ctx *ctx, u8 *buf, size_t max_size,
			     u32 vis_w, u32 vis_h, u32 coded_w, u32 coded_h);
ssize_t helix_h264_write_pps(struct helix_ctx *ctx, u8 *buf, size_t max_size, u32 qp);
ssize_t helix_h264_write_slice(struct helix_ctx *ctx, u8 *buf, size_t max_size,
			       bool is_idr, u32 qp, const u8 *hw_bs, size_t hw_len);
int helix_h264_hw_start(struct helix_dev *hdev, struct helix_ctx *ctx,
			dma_addr_t src_y_dma, dma_addr_t src_uv_dma,
			dma_addr_t dst_dma, u32 width, u32 height,
			u32 y_stride, u32 uv_stride, u32 fourcc,
			bool is_idr, u32 qp);

#endif
