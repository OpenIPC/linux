// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2026 Difan Zhang

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/dma-mapping.h>

#include "helix_drv.h"
#include "helix_h264.h"
#include "helix_h264_tables.h"

struct h264_bit_writer {
	u8 *buf;
	size_t pos;
	size_t max_size;
	u32 bit_buf;
	int bit_count;
	int zero_count;
	bool overflow;
};

static inline void h264_bw_init(struct h264_bit_writer *bw, u8 *buf, size_t max_size)
{
	bw->buf = buf;
	bw->pos = 0;
	bw->max_size = max_size;
	bw->bit_buf = 0;
	bw->bit_count = 0;
	bw->zero_count = 0;
	bw->overflow = false;
}

static inline void h264_bw_put_byte(struct h264_bit_writer *bw, u8 b)
{
	if (bw->pos >= bw->max_size) {
		bw->overflow = true;
		return;
	}

	/* Emulation prevention byte insertion */
	if (bw->zero_count >= 2 && b <= 3) {
		if (bw->pos >= bw->max_size) {
			bw->overflow = true;
			return;
		}
		bw->buf[bw->pos++] = 0x03;
		bw->zero_count = 0;
	}

	if (bw->pos >= bw->max_size) {
		bw->overflow = true;
		return;
	}
	bw->buf[bw->pos++] = b;

	if (b == 0)
		bw->zero_count++;
	else
		bw->zero_count = 0;
}

static inline void h264_bw_put_bits(struct h264_bit_writer *bw, u32 val, int n)
{
	int i;

	for (i = n - 1; i >= 0; i--) {
		bw->bit_buf = (bw->bit_buf << 1) | ((val >> i) & 1);
		bw->bit_count++;
		if (bw->bit_count == 8) {
			h264_bw_put_byte(bw, (u8)bw->bit_buf);
			bw->bit_buf = 0;
			bw->bit_count = 0;
		}
	}
}

static inline void h264_bw_put_ue(struct h264_bit_writer *bw, u32 val)
{
	u32 val_plus1 = val + 1;
	int len = fls(val_plus1) - 1;

	h264_bw_put_bits(bw, 0, len);
	h264_bw_put_bits(bw, val_plus1, len + 1);
}

static inline void h264_bw_put_se(struct h264_bit_writer *bw, int val)
{
	if (val <= 0)
		h264_bw_put_ue(bw, (u32)(-2 * val));
	else
		h264_bw_put_ue(bw, (u32)(2 * val - 1));
}

static inline void h264_bw_rbsp_trailing(struct h264_bit_writer *bw)
{
	h264_bw_put_bits(bw, 1, 1);
	if (bw->bit_count > 0)
		h264_bw_put_bits(bw, 0, 8 - bw->bit_count);
}

static inline void h264_bw_start_code(struct h264_bit_writer *bw)
{
	if (bw->pos + 4 > bw->max_size) {
		bw->overflow = true;
		return;
	}
	bw->buf[bw->pos++] = 0x00;
	bw->buf[bw->pos++] = 0x00;
	bw->buf[bw->pos++] = 0x00;
	bw->buf[bw->pos++] = 0x01;
	bw->zero_count = 0;
}

ssize_t helix_h264_write_sps(struct helix_ctx *ctx, u8 *buf, size_t max_size,
			     u32 vis_w, u32 vis_h, u32 coded_w, u32 coded_h)
{
	struct h264_bit_writer bw;
	u8 level_idc;
	bool crop = (vis_w < coded_w) || (vis_h < coded_h);

	switch (ctx->h264_level) {
	case V4L2_MPEG_VIDEO_H264_LEVEL_3_0:
		level_idc = 30;
		break;
	case V4L2_MPEG_VIDEO_H264_LEVEL_3_1:
		level_idc = 31;
		break;
	case V4L2_MPEG_VIDEO_H264_LEVEL_4_0:
		level_idc = 40;
		break;
	default:
		level_idc = (coded_w > 1280 || coded_h > 720) ? 40 : 31;
		break;
	}

	h264_bw_init(&bw, buf, max_size);
	h264_bw_start_code(&bw);
	h264_bw_put_byte(&bw, 0x67);

	/* High profile. PPS CABAC and transform_8x8 match this idc. */
	h264_bw_put_bits(&bw, 100, 8);
	h264_bw_put_bits(&bw, 0x00, 8);
	h264_bw_put_bits(&bw, level_idc, 8);
	h264_bw_put_ue(&bw, 0);
	h264_bw_put_ue(&bw, 1);
	h264_bw_put_ue(&bw, 0);
	h264_bw_put_ue(&bw, 0);
	h264_bw_put_bits(&bw, 0, 1);
	h264_bw_put_bits(&bw, 0, 1);
	h264_bw_put_ue(&bw, 0);
	h264_bw_put_ue(&bw, 2);
	h264_bw_put_ue(&bw, 1);
	h264_bw_put_bits(&bw, 0, 1);
	h264_bw_put_ue(&bw, (coded_w / 16) - 1);
	h264_bw_put_ue(&bw, (coded_h / 16) - 1);
	h264_bw_put_bits(&bw, 1, 1);
	h264_bw_put_bits(&bw, 1, 1);

	if (crop) {
		h264_bw_put_bits(&bw, 1, 1);
		h264_bw_put_ue(&bw, 0);
		h264_bw_put_ue(&bw, (coded_w - vis_w) / 2);
		h264_bw_put_ue(&bw, 0);
		h264_bw_put_ue(&bw, (coded_h - vis_h) / 2);
	} else {
		h264_bw_put_bits(&bw, 0, 1);
	}

	h264_bw_put_bits(&bw, 0, 1);
	h264_bw_rbsp_trailing(&bw);

	return bw.overflow ? -ENOSPC : (ssize_t)bw.pos;
}

ssize_t helix_h264_write_pps(struct helix_ctx *ctx, u8 *buf, size_t max_size, u32 qp)
{
	struct h264_bit_writer bw;

	h264_bw_init(&bw, buf, max_size);
	h264_bw_start_code(&bw);
	h264_bw_put_byte(&bw, 0x68);

	h264_bw_put_ue(&bw, 0);
	h264_bw_put_ue(&bw, 0);
	h264_bw_put_bits(&bw, 1, 1);
	h264_bw_put_bits(&bw, 0, 1);
	h264_bw_put_ue(&bw, 0);
	h264_bw_put_ue(&bw, 0);
	h264_bw_put_ue(&bw, 0);
	h264_bw_put_bits(&bw, 0, 1);
	h264_bw_put_bits(&bw, 0, 2);
	h264_bw_put_se(&bw, 0);
	h264_bw_put_se(&bw, 0);
	h264_bw_put_se(&bw, 0);
	h264_bw_put_bits(&bw, 1, 1);
	h264_bw_put_bits(&bw, 0, 1);
	h264_bw_put_bits(&bw, 0, 1);
	h264_bw_put_bits(&bw, 1, 1);
	h264_bw_put_bits(&bw, 0, 1);
	h264_bw_put_se(&bw, 0);
	h264_bw_rbsp_trailing(&bw);

	return bw.overflow ? -ENOSPC : (ssize_t)bw.pos;
}

ssize_t helix_h264_write_slice(struct helix_ctx *ctx, u8 *buf, size_t max_size,
			       bool is_idr, u32 qp, const u8 *hw_bs, size_t hw_len)
{
	struct h264_bit_writer bw;
	u32 qp_clamp = clamp_val(qp, ctx->min_qp, ctx->max_qp);
	size_t i;

	h264_bw_init(&bw, buf, max_size);
	h264_bw_start_code(&bw);

	if (is_idr) {
		h264_bw_put_byte(&bw, 0x65);
		h264_bw_put_ue(&bw, 0);
		h264_bw_put_ue(&bw, 7);
		h264_bw_put_ue(&bw, 0);
		h264_bw_put_bits(&bw, 0, 4);
		h264_bw_put_ue(&bw, ctx->idr_pic_id++ & 0x7);
		h264_bw_put_bits(&bw, 0, 2);
		h264_bw_put_se(&bw, (int)qp_clamp - 26);
		h264_bw_put_ue(&bw, 0);
		h264_bw_put_se(&bw, 0);
		h264_bw_put_se(&bw, 0);
	} else {
		h264_bw_put_byte(&bw, 0x41);
		h264_bw_put_ue(&bw, 0);
		h264_bw_put_ue(&bw, 5);
		h264_bw_put_ue(&bw, 0);
		h264_bw_put_bits(&bw, ctx->frame_num & 0xf, 4);
		h264_bw_put_bits(&bw, 0, 1);
		h264_bw_put_bits(&bw, 0, 1);
		h264_bw_put_bits(&bw, 0, 1);
		h264_bw_put_ue(&bw, 0);
		h264_bw_put_se(&bw, (int)qp_clamp - 26);
		h264_bw_put_ue(&bw, 0);
		h264_bw_put_se(&bw, 0);
		h264_bw_put_se(&bw, 0);
	}

	while (bw.bit_count != 0)
		h264_bw_put_bits(&bw, 1, 1);

	for (i = 0; i < hw_len; i++)
		h264_bw_put_byte(&bw, hw_bs[i]);

	return bw.overflow ? -ENOSPC : (ssize_t)bw.pos;
}

int helix_h264_hw_start(struct helix_dev *hdev, struct helix_ctx *ctx,
			dma_addr_t src_y_dma, dma_addr_t src_uv_dma,
			dma_addr_t dst_dma, u32 width, u32 height,
			u32 y_stride, u32 uv_stride, u32 fourcc,
			bool is_idr, u32 qp)
{
	struct helix_vdma_task task;
	u32 mb_w = width / 16;
	u32 mb_h = height / 16;
	int cur_slot = ctx->h264_bufs.cur_slot;
	int ref_slot = 1 - cur_slot;
	dma_addr_t recon_y_paddr = ctx->h264_bufs.recon_y_paddr[cur_slot];
	dma_addr_t recon_c_paddr = ctx->h264_bufs.recon_c_paddr[cur_slot];
	dma_addr_t ref_y_paddr = ctx->h264_bufs.recon_y_paddr[ref_slot];
	dma_addr_t ref_c_paddr = ctx->h264_bufs.recon_c_paddr[ref_slot];
	dma_addr_t scratch_paddr = hdev->scratch_paddr;
	u32 qp_clamp = clamp_val(qp, ctx->min_qp, ctx->max_qp);
	bool is_p = !is_idr;
	/* Pre-scaled motion-estimation lambda thresholds. Above QP 33, TCSM filters ramp from these bases. */
	u32 dqp = (qp_clamp > 0x21) ? qp_clamp - 0x21 : 0;
	u32 me_lambda_y0 = 0x180;
	u32 me_lambda_c0 = 0x060;
	u32 me_lambda_c1 = 0x060;
	u32 me_lambda_step_y = 0x030;
	u32 me_lambda_step_c0 = 0x00c;
	u32 me_lambda_step_c1 = 0x00c;
	u32 tcsm_sad_weight = (me_lambda_y0 + me_lambda_step_y * dqp) & 0xffff;
	u32 tcsm_sad_thresh = (me_lambda_c0 + me_lambda_step_c0 * dqp) & 0xffff;
	const s8 *cabac_mn = is_idr ? h264_cabac_context_init_I : h264_cabac_context_init_P0;
	u8 cabac_state[460];
	u32 efe_kick;
	unsigned int i;
	int ret;

	if ((fourcc != V4L2_PIX_FMT_NV12 && fourcc != V4L2_PIX_FMT_NV21) ||
	    width & 15 || height & 15 ||
	    y_stride != width || uv_stride != width ||
	    (src_y_dma | src_uv_dma | dst_dma) & 127 ||
	    !ctx->h264_bufs_allocated || !scratch_paddr)
		return -EINVAL;

	for (i = 0; i < 460; i++) {
		int m = cabac_mn[2 * i];
		int n = cabac_mn[2 * i + 1];
		int pre = ((m * (int)qp_clamp) >> 4) + n;

		cabac_state[i] = (u8)clamp_val(pre, 1, 126);
	}

	helix_vdma_task_init(&task, hdev->task_vaddr, hdev->task_paddr, HELIX_TASK_SIZE);

	/* EFE geometry, raw input pointers and TCSM base addresses */
	helix_vdma_append(&task, REG_EFE_GEOM,
			  (((mb_h - 1) & 0xff) << 8) | ((mb_w - 1) & 0xff));
	helix_vdma_append(&task, REG_EFE_COEF_BA, 0x131c0000);
	helix_vdma_append(&task, REG_EFE_RAWY_SBA, src_y_dma);
	helix_vdma_append(&task, REG_EFE_RAWC_SBA, src_uv_dma);
	helix_vdma_append(&task, REG_EFE_RAWV_SBA, 0);
	helix_vdma_append(&task, REG_EFE_RAW_STRD, (y_stride << 16) | uv_stride);
	helix_vdma_append(&task, REG_EFE_TOPMV_BA, 0x131c4800);
	helix_vdma_append(&task, REG_EFE_TOPPA_BA, 0x131c4000);
	helix_vdma_append(&task, REG_EFE_MECHN_BA, 0x131c5400);
	helix_vdma_append(&task, REG_EFE_MAUCHN_BA, 0x131c4c00);
	helix_vdma_append(&task, REG_EFE_DBLKCHN_BA, 0x131c5000);
	helix_vdma_append(&task, REG_EFE_SDECHN_BA, 0x131c5600);
	helix_vdma_append(&task, REG_EFE_RAW_DBA, HELIX_INTERNAL_RAM);

	/* EFE ROI, quantization table and spatial analysis parameters */
	helix_vdma_append(&task, REG_EFE_ROI_MAX_QP, min(qp_clamp + 13, 51U));
	helix_vdma_append(&task, REG_EFE_ROI_BASE_INFO0, 0);
	helix_vdma_append(&task, REG_EFE_ROI_BASE_INFO1, 0);
	for (i = 0; i < 8; i++)
		helix_vdma_append(&task, REG_EFE_ROI_POS_INFO0 + i * 4, 0);
	helix_vdma_append(&task, REG_EFE_QP_GEN_TAB, 0x000c5800);
	helix_vdma_append(&task, REG_EFE_CQP_OFST, 0);
	helix_vdma_append(&task, REG_EFE_SSAD, 0);
	helix_vdma_append(&task, REG_EFE_DCS, 0x00400000);
	helix_vdma_append(&task, REG_EFE_QPG_CTRL,
			  0x10 | ((qp_clamp & 0xff) << 8) |
			  ((max_t(int, (int)qp_clamp - 12, 1) & 0xff) << 16) |
			  ((min(qp_clamp + 13, 51U) & 0xff) << 24));
	helix_vdma_append(&task, REG_EFE_CRP_FILT, 0);
	helix_vdma_append(&task, REG_EFE_CRP_THR0, 0);
	helix_vdma_append(&task, REG_EFE_CRP_THR1, 0);
	helix_vdma_append(&task, REG_EFE_CRP_THR2, 0);
	helix_vdma_append(&task, REG_EFE_CRP_THR3, 0);
	helix_vdma_append(&task, REG_EFE_CRP_MB_OFST0, 0);
	helix_vdma_append(&task, REG_EFE_CRP_MB_OFST1, 0);
	helix_vdma_append(&task, REG_EFE_SAS_MCNT1, is_p ? 0x030484c1 : 0x060404c1);
	helix_vdma_append(&task, REG_EFE_QP_SUM, is_p ? 0x61615c21 : 0x61615921);
	helix_vdma_append(&task, REG_EFE_SAS_FLT_THR, 0x12492fc0);
	helix_vdma_append(&task, REG_EFE_ME_CFG0, 0x12492492);
	helix_vdma_append(&task, REG_EFE_ME_CFG1, 0x0a000db3);
	helix_vdma_append(&task, REG_EFE_ME_CFG2, 0x0f0a0505);
	helix_vdma_append(&task, REG_EFE_ME_CFG3, 0);
	helix_vdma_append(&task, REG_EFE_ME_LAMBDA0, (me_lambda_step_y << 16) | me_lambda_y0);
	helix_vdma_append(&task, REG_EFE_ME_LAMBDA1, (me_lambda_step_c0 << 16) | me_lambda_c0);
	helix_vdma_append(&task, REG_EFE_ME_LAMBDA2, (me_lambda_step_c1 << 16) | me_lambda_c1);
	helix_vdma_append(&task, REG_EFE_SAS_THDG2, 0x00001400);
	helix_vdma_append(&task, REG_EFE_SAS_MB_OFSTA, 0xaf8c7864);
	helix_vdma_append(&task, REG_EFE_SAS_MB_OFSTB, 0x8c7d6432);
	helix_vdma_append(&task, REG_EFE_SAS_CNT, 0xe1af9678);
	helix_vdma_append(&task, REG_EFE_SAS_MCNT0, 0x003e3c3a);
	helix_vdma_append(&task, REG_EFE_ME_CFG4, 0x0c25bd33);
	helix_vdma_append(&task, REG_EFE_ME_CFG5, 0x10f54009);
	helix_vdma_append(&task, REG_EFE_ME_CFG6, 0x37464605);
	helix_vdma_append(&task, REG_EFE_INTRA_CFG0, 0x00000330);
	helix_vdma_append(&task, REG_EFE_INTRA_CFG1, 0x80000330);

	/* VDMA block sizing and reference buffer configuration */
	helix_vdma_append(&task, REG_VDMA_UNLK, (height << 14) | width | 0x20000000);
	helix_vdma_append(&task, REG_VDMA_TASKRG, 0);
	helix_vdma_append(&task, REG_VDMA_TASKST, 0);
	helix_vdma_append(&task, REG_VDMA_CH0_CNT,
			  ((((width + 63) >> 6) * ((height + 15) >> 4)) << 16) |
			  ((((width + 31) >> 5) * ((height + 15) >> 4)) & 0xffff));
	helix_vdma_append(&task, REG_VDMA_REF_Y_BA, is_p ? ref_y_paddr : 0);
	helix_vdma_append(&task, REG_VDMA_REF_C_BA, is_p ? ref_c_paddr : 0);
	helix_vdma_append(&task, REG_VDMA_STRIDE, (mb_w << 20) | (mb_w << 3));
	helix_vdma_append(&task, REG_VDMA_CFG0, 0);
	helix_vdma_append(&task, REG_VDMA_CFG1, 0);
	helix_vdma_append(&task, REG_VDMA_CFG2, 0);
	helix_vdma_append(&task, REG_VDMA_CFG3, 0);
	helix_vdma_append(&task, REG_VDMA_LOCK, 0x00000020);

	/* Motion compensation and estimation (P-frames only) */
	if (is_p) {
		helix_vdma_append(&task, REG_MCE_CHN_BA, 0x131c5400);
		helix_vdma_append(&task, REG_MCE_TMV_BA, 0x131c4800);
		helix_vdma_append(&task, REG_MCE_CHN_SYNC, 0x13100070);
		helix_vdma_append(&task, REG_MCE_FRM_SIZE,
				  ((height - 1) << 16) | ((width - 1) & 0xffff));
		helix_vdma_append(&task, REG_MCE_FRM_STRD, (mb_w << 20) | (mb_w << 4));
		helix_vdma_append(&task, REG_MCE_COMP_CTRL, 0x800a8021);
		helix_vdma_append(&task, REG_MCE_ESTI_CTRL, 0x871f5008);
		helix_vdma_append(&task, REG_MCE_MRGI, 0);
		helix_vdma_append(&task, REG_MCE_MVR, 0x02000200);
		helix_vdma_append(&task, REG_MCE_REF_Y_BA, ref_y_paddr);
		helix_vdma_append(&task, REG_MCE_REF_C_BA, ref_c_paddr);
		helix_vdma_append(&task, REG_MCE_SRC_Y_BA, src_y_dma);
		helix_vdma_append(&task, REG_MCE_SRC_C_BA, src_uv_dma);
		helix_vdma_append(&task, REG_MCE_CFG0, 0);
		helix_vdma_append(&task, REG_MCE_CFG1, 0);
		helix_vdma_append(&task, REG_MCE_SLC_SPOS, 0);
		helix_vdma_append(&task, REG_MCE_FSC,
				  (mb_w >= 0x33 && mb_h > 0x26) ? 0x08080303 : 0);
		helix_vdma_append(&task, REG_MCE_CFG2, 0);
		helix_vdma_append(&task, REG_MCE_GLB_CTRL, 0x544fd9b1);
	}

	/* Video Macroblock Analysis Unit (VMAU) configuration */
	helix_vdma_append(&task, REG_VMAU_GBL_RUN, 4);
	helix_vdma_append(&task, REG_VMAU_VIDEO_TYPE, is_idr ? 0x80001b00 : 0x80000b00);
	helix_vdma_append(&task, REG_VMAU_NCCHN_ADDR, 0x131c4c00);
	helix_vdma_append(&task, REG_VMAU_ENC_DONE, 0x131ffffc);
	helix_vdma_append(&task, REG_VMAU_DEC_DONE, 0x13100074);
	helix_vdma_append(&task, REG_VMAU_Y_GS, (height << 16) | width);
	helix_vdma_append(&task, REG_VMAU_GBL_CTR, 0x01000001);
	helix_vdma_append(&task, REG_VMAU_DEADZONE, 0x0b150b15);
	helix_vdma_append(&task, REG_VMAU_FIFO_CFG, 0x0b5552d5);
	helix_vdma_append(&task, REG_VMAU_TOP_BASE, 0x131c4000);
	helix_vdma_append(&task, REG_VMAU_MD_CFG0,
			  ((mb_h - 1) << 24) | ((mb_w - 1) << 16) | 0x8000 | (is_idr ? 1 : 2));
	helix_vdma_append(&task, REG_VMAU_MD_CFG1, 0x0000c200);
	helix_vdma_append(&task, REG_VMAU_MD_CFG2, 0xcc000000);
	helix_vdma_append(&task, REG_VMAU_MD_CFG3, 0);
	helix_vdma_append(&task, REG_VMAU_MD_CFG4, 0);
	helix_vdma_append(&task, REG_VMAU_MD_CFG5, 0);
	helix_vdma_append(&task, REG_VMAU_MD_CFG6, 0x55004304);
	helix_vdma_append(&task, REG_VMAU_MD_CFG7, 0x11 | ((qp_clamp & 0x3f) << 17));
	helix_vdma_append(&task, REG_VMAU_MD_CFG8, 0);
	helix_vdma_append(&task, REG_VMAU_MD_CFG9, 0);
	helix_vdma_append(&task, REG_VMAU_FIFO_CTRL, 0x23000000);
	helix_vdma_append(&task, REG_VMAU_CFG7, 0x00400000);
	helix_vdma_append(&task, REG_VMAU_CFG8, 0x00400000);
	helix_vdma_append(&task, REG_VMAU_ACMASK, 0xffff8002);

	/* VMAU FIFO initial configuration */
	for (i = 0; i < 40; i++)
		helix_vdma_append(&task, REG_VMAU_FIFO_DATA, helix_h264_vmau_fifo_words[i]);

	/* CABAC intra/inter slice initialization contexts */
	for (i = 0; i < 42; i++) {
		u32 idx;

		if (i < 10)
			idx = is_p ? (i + 0x0e) : (i + 3);
		else if (i < 0x16)
			idx = i + 0x3f;
		else if (i <= 0x1b)
			idx = i + 0x2a;
		else
			idx = i + 0x0c;

		helix_vdma_append(&task, REG_VMAU_CTX, cabac_state[idx]);
	}

	/* CABAC syntax element context initialization */
	for (i = 0; i < 225; i++)
		helix_vdma_append(&task, REG_VMAU_CABAC_RUN1,
				  cabac_state[helix_h264_cabac_fifo_map[i]]);

	/* VMAU slice and syntax engine parameters */
	helix_vdma_append(&task, REG_VMAU_SDE_CFG0, 0x30000006);
	helix_vdma_append(&task, REG_VMAU_SDE_CFG1, 0x44444444);
	helix_vdma_append(&task, REG_VMAU_SDE_CFG2, 0);
	helix_vdma_append(&task, REG_VMAU_SDE_CFG3, 0x00430000);
	helix_vdma_append(&task, REG_VMAU_SDE_CFG4, 0);
	helix_vdma_append(&task, REG_VMAU_SDE_CFG5, 0);

	/* Deadzone and inverse quant matrices */
	for (i = 0; i < 64; i++)
		helix_vdma_append(&task, REG_VMAU_RC_MAD_RAM + i * 4,
				  helix_h264_deadzone_matrix[i]);

	for (i = 0; i < 32; i++)
		helix_vdma_append(&task, REG_VMAU_IQUANT_BASE + i * 4,
				  helix_h264_iquant_matrix[i]);

	helix_vdma_append(&task, REG_VMAU_GBL_RUN, 1);

	/* Deblocking filter and syntax decoder/encoder */
	helix_vdma_append(&task, REG_DBLK_CFG, 0x00000c0c);
	helix_vdma_append(&task, REG_DBLK_TRIG, 8);
	helix_vdma_append(&task, REG_SDE_STAT, 0);
	helix_vdma_append(&task, REG_SDE_GL_CTRL, 0x00000011);
	helix_vdma_append(&task, REG_SDE_SL_GEOM, (mb_h << 24) | (mb_w << 16));
	helix_vdma_append(&task, REG_SDE_CODEC_ID, 2);
	helix_vdma_append(&task, REG_SDE_CFG0, 3);
	helix_vdma_append(&task, REG_SDE_CFG1, (qp_clamp << 8) | 0x30 | (is_idr ? 1 : 2));
	helix_vdma_append(&task, REG_SDE_CFG2, 0x131c5600);
	helix_vdma_append(&task, REG_SDE_CFG3, 0x1310007c);
	helix_vdma_append(&task, REG_SDE_CFG4, dst_dma);
	helix_vdma_append(&task, REG_SDE_CFG5, HELIX_BOUNCE_BUF_SIZE);
	for (i = 0; i < 460; i++) {
		u32 s = cabac_state[i] & 0x7f;
		u32 idx = (s <= 0x3f) ? (0x3f - s) : (s - 0x40);
		u32 val = (s >> 6) | helix_h264_cavlc_table[idx];

		helix_vdma_append(&task, REG_SDE_CTX_TBL + i * 4, val);
	}
	helix_vdma_append(&task, REG_SDE_SL_CTRL, 2);

	/* Output DMA and pipeline scheduler configuration */
	helix_vdma_append(&task, REG_ODMA_GLB_TRIG, ((width < 0x211) << 18) | ((mb_h - 1) << 8) | (mb_w - 1));
	helix_vdma_append(&task, REG_ODMA_GLB_CTRL, dst_dma & 0xffffff80);
	helix_vdma_append(&task, REG_ODMA_TLC_BA, (scratch_paddr + 0x08000) & 0xffffff80);
	helix_vdma_append(&task, REG_ODMA_RAW_Y_BA, (scratch_paddr + 0x38000) & 0xffffff80);
	helix_vdma_append(&task, REG_ODMA_RAW_C_BA, (scratch_paddr + 0xb8000) & 0xffffff80);
	helix_vdma_append(&task, REG_ODMA_BUF3_BA, (scratch_paddr + 0x158000) & 0xffffff80);
	helix_vdma_append(&task, REG_ODMA_BUF2_BA, (scratch_paddr + 0xd8000) & 0xffffff80);
	helix_vdma_append(&task, REG_ODMA_AXI_CTRL, 0x400);
	helix_vdma_append(&task, REG_ODMA_CFG0, 1);
	for (i = 0; i < 8; i++)
		helix_vdma_append(&task, REG_SCH_SCHC + i * 4, 0);
	helix_vdma_append(&task, REG_SCH_SCHC, ((is_p & 1) << 2) | 0x08080400);
	helix_vdma_append(&task, REG_SCH_BND, (is_p & 1) | 0x97850f02);

	/*
	 * JRFD reference reconstruction geometry and reconstruction buffers.
	 * Bit 31 enables reconstruct output; bit 29 flags narrow macroblock widths (1 or 3).
	 */
	helix_vdma_append(&task, REG_JRFD_GEOM,
			  (1U << 31) | ((mb_w == 1 || mb_w == 3) ? (1U << 29) : 0) |
			  (height << 14) | width);
	helix_vdma_append(&task, REG_JRFD_Y_BA, recon_y_paddr);
	helix_vdma_append(&task, REG_JRFD_C_BA, recon_c_paddr);
	helix_vdma_append(&task, REG_JRFD_STRD, (mb_w << 20) | (mb_w << 3));
	for (i = 0; i < 4; i++)
		helix_vdma_append(&task, REG_JRFD_CFG0 + i * 4, 0);
	helix_vdma_append(&task, REG_JRFD_GLB_CTRL, 0x20);

	/* TCSM source and reference DMA setup */
	helix_vdma_append(&task, REG_TCSM_GEOM, ((height - 1) << 16) | (width - 1));
	helix_vdma_append(&task, REG_TCSM_Y_BA, src_y_dma);
	helix_vdma_append(&task, REG_TCSM_C_BA, src_uv_dma);
	helix_vdma_append(&task, REG_TCSM_REF_Y_BA, is_p ? ref_y_paddr : 0);
	helix_vdma_append(&task, REG_TCSM_REF_C_BA, is_p ? ref_c_paddr : 0);
	helix_vdma_append(&task, REG_TCSM_SRC_Y_BA, src_y_dma);
	helix_vdma_append(&task, REG_TCSM_SRC_C_BA, src_uv_dma);
	helix_vdma_append(&task, REG_TCSM_STRD, (uv_stride << 16) | (y_stride & 0xffff));
	helix_vdma_append(&task, REG_TCSM_CFG0, tcsm_sad_weight);
	helix_vdma_append(&task, REG_TCSM_CFG1,
			  ((tcsm_sad_thresh & 0x3fff) << 16) | (tcsm_sad_thresh & 0x3fff));
	helix_vdma_append(&task, REG_TCSM_CTRL, is_p ? 0x0002ff63 : 0x0002ff21);

	/* Pipeline termination kick */
	efe_kick = (1U << 31) | (1U << 30) | ((qp_clamp & 0x3f) << 8) |
		   (((height & 0xf) != 0) << 7) | ((is_p & 1) << 4) | 0x20 |
		   ((fourcc == V4L2_PIX_FMT_NV21) ? EFE_PLANE_NV21 : EFE_PLANE_NV12) |
		   EFE_EN | EFE_RUN;
	helix_vdma_append(&task, REG_EFE_CTRL, efe_kick);
	helix_vdma_terminate(&task);

	ret = helix_vdma_kick(hdev, &task);
	return ret;
}
