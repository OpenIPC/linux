// SPDX-License-Identifier: GPL-2.0
/*
 * H.264 SPS/PPS and CABAC slice header generation for the PAE engine.
 * The engine emits only the slice data; the host supplies the headers.
 *
 * Copyright (C) 2026 Difan Zhang
 */
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include "fh8626-pae-bitstream.h"

struct pae_bits {
	u8 *data;
	size_t capacity;
	u32 bits;
	int error;
};

static void write_bits(struct pae_bits *writer, u32 value, unsigned int count)
{
	unsigned int shift;

	if (writer->error)
		return;
	if (count > 32 || count > writer->capacity * 8 - writer->bits) {
		writer->error = -ENOSPC;
		return;
	}
	for (shift = count; shift; shift--) {
		u32 byte = writer->bits >> 3;
		u32 bit = 7 - (writer->bits & 7);

		if (bit == 7)
			writer->data[byte] = 0;
		writer->data[byte] |= ((value >> (shift - 1)) & 1) << bit;
		writer->bits++;
	}
}

static void write_ue(struct pae_bits *writer, u32 value)
{
	unsigned int count;

	if (value == U32_MAX) {
		writer->error = -ERANGE;
		return;
	}
	count = fls(value + 1);
	write_bits(writer, 0, count - 1);
	write_bits(writer, value + 1, count);
}

static void write_se(struct pae_bits *writer, int value)
{
	write_ue(writer, value <= 0 ? -2 * value : 2 * value - 1);
}

static void trailing_bits(struct pae_bits *writer)
{
	write_bits(writer, 1, 1);
	if (writer->bits & 7)
		write_bits(writer, 0, 8 - (writer->bits & 7));
}

static int annex_b(u8 *dst, size_t capacity, u8 nal,
		   const struct pae_bits *rbsp)
{
	size_t i, n = DIV_ROUND_UP(rbsp->bits, 8), out = 5;
	unsigned int zeros = 0;

	if (rbsp->error)
		return rbsp->error;
	if (capacity < 5)
		return -ENOSPC;
	dst[0] = dst[1] = dst[2] = 0;
	dst[3] = 1;
	dst[4] = nal;
	for (i = 0; i < n; i++) {
		u8 value = rbsp->data[i];

		if (zeros == 2 && value <= 3) {
			if (out == capacity)
				return -ENOSPC;
			dst[out++] = 3;
			zeros = 0;
		}
		if (out == capacity)
			return -ENOSPC;
		dst[out++] = value;
		zeros = value ? 0 : zeros + 1;
	}
	return out;
}

static bool deblock_valid(const struct fh8626_pae_deblock *d)
{
	return d->disable_idc <= 2 &&
	       d->alpha_c0_offset_div2 >= -6 && d->alpha_c0_offset_div2 <= 6 &&
	       d->beta_offset_div2 >= -6 && d->beta_offset_div2 <= 6;
}

static void write_vui(struct pae_bits *writer, const struct fh8626_pae_vui *v)
{
	write_bits(writer, v->aspect_ratio_present, 1);
	if (v->aspect_ratio_present) {
		write_bits(writer, v->aspect_ratio_idc, 8);
		if (v->aspect_ratio_idc == FH8626_PAE_SAR_EXTENDED) {
			write_bits(writer, v->sar_width, 16);
			write_bits(writer, v->sar_height, 16);
		}
	}
	write_bits(writer, 0, 1); /* overscan_info_present_flag */
	write_bits(writer, v->video_signal_present, 1);
	if (v->video_signal_present) {
		write_bits(writer, v->video_format, 3);
		write_bits(writer, v->full_range, 1);
		write_bits(writer, v->colour_description_present, 1);
		if (v->colour_description_present) {
			write_bits(writer, v->colour_primaries, 8);
			write_bits(writer, v->transfer_characteristics, 8);
			write_bits(writer, v->matrix_coefficients, 8);
		}
	}
	write_bits(writer, 0, 1); /* chroma_loc_info_present_flag */
	write_bits(writer, v->timing_present, 1);
	if (v->timing_present) {
		write_bits(writer, v->num_units_in_tick, 32);
		write_bits(writer, v->time_scale, 32);
		write_bits(writer, v->fixed_frame_rate, 1);
	}
	/* nal_hrd, vcl_hrd, pic_struct_present */
	write_bits(writer, 0, 3);
	/*
	 * Bitstream restriction: no reordering and a single reference let a
	 * decoder output each picture as soon as it is decoded.
	 */
	write_bits(writer, 1, 1);
	write_bits(writer, 1, 1); /* motion_vectors_over_pic_boundaries */
	write_ue(writer, 0); /* max_bytes_per_pic_denom: no limit */
	write_ue(writer, 0); /* max_bits_per_mb_denom: no limit */
	write_ue(writer, 16); /* log2_max_mv_length_horizontal */
	write_ue(writer, 16); /* log2_max_mv_length_vertical */
	write_ue(writer, 0); /* max_num_reorder_frames */
	write_ue(writer, 1); /* max_dec_frame_buffering */
}

int fh8626_pae_parameter_sets(const struct fh8626_pae_picture *p,
			      u8 *dst, size_t capacity)
{
	u8 rbsp[128];
	struct pae_bits writer = { .data = rbsp, .capacity = sizeof(rbsp) };
	bool crop;
	int sps, pps;

	if (!p->width || !p->height || (p->width & 1) || (p->height & 1) ||
	    p->coded_width < p->width || p->coded_height < p->height ||
	    (p->coded_width & 15) || (p->coded_height & 15) ||
	    !deblock_valid(&p->deblock))
		return -EINVAL;
	if (p->vui && p->vui->timing_present &&
	    (!p->vui->num_units_in_tick || !p->vui->time_scale))
		return -EINVAL;
	crop = p->width != p->coded_width || p->height != p->coded_height;
	write_bits(&writer, 77, 8); /* Main profile */
	write_bits(&writer, 0, 8);
	write_bits(&writer, 32, 8); /* Level 3.2 */
	write_ue(&writer, 0);
	write_ue(&writer, 12); /* log2_max_frame_num_minus4 */
	write_ue(&writer, 0); /* pic_order_cnt_type */
	write_ue(&writer, 12);
	write_ue(&writer, 1); /* one reference picture */
	write_bits(&writer, 0, 1);
	write_ue(&writer, (p->coded_width >> 4) - 1);
	write_ue(&writer, (p->coded_height >> 4) - 1);
	write_bits(&writer, 3, 2); /* frame_mbs_only + direct_8x8_inference */
	write_bits(&writer, crop, 1);
	if (crop) {
		write_ue(&writer, 0);
		write_ue(&writer, (p->coded_width - p->width) / 2);
		write_ue(&writer, 0);
		write_ue(&writer, (p->coded_height - p->height) / 2);
	}
	write_bits(&writer, !!p->vui, 1); /* vui_parameters_present_flag */
	if (p->vui)
		write_vui(&writer, p->vui);
	trailing_bits(&writer);
	sps = annex_b(dst, capacity, 0x67, &writer);
	if (sps < 0)
		return sps;

	writer.bits = 0;
	write_ue(&writer, 0); /* PPS id */
	write_ue(&writer, 0); /* SPS id */
	write_bits(&writer, 1, 1); /* CABAC */
	write_bits(&writer, 0, 1);
	write_ue(&writer, 0); /* no slice groups */
	write_ue(&writer, 0); /* default L0 references minus one */
	write_ue(&writer, 0); /* default L1 references minus one */
	write_bits(&writer, 0, 3); /* weighted prediction disabled */
	write_se(&writer, 0); /* pic_init_qp_minus26 */
	write_se(&writer, 0);
	write_se(&writer, 0); /* chroma_qp_index_offset */
	write_bits(&writer, p->deblock.control_present, 1);
	/* constrained_intra_pred, redundant_pic_cnt_present */
	write_bits(&writer, 0, 2);
	trailing_bits(&writer);
	pps = annex_b(dst + sps, capacity - sps, 0x68, &writer);
	return pps < 0 ? pps : sps + pps;
}

static void slice_header(struct pae_bits *writer,
			 const struct fh8626_pae_picture *p, u32 first_mb)
{
	write_ue(writer, first_mb);
	write_ue(writer, p->idr ? 7 : 5); /* all slices I or P */
	write_ue(writer, 0);
	write_bits(writer, p->idr ? 0 : p->frame_num, 16);
	if (p->idr)
		write_ue(writer, p->idr_pic_id);
	write_bits(writer, p->idr ? 0 : p->poc, 16);
	if (p->idr) {
		/* no_output_of_prior_pics, long_term_reference */
		write_bits(writer, 0, 2);
	} else {
		/* ref override, list modification */
		write_bits(writer, 0, 2);
		/* Reference pictures carry the sliding-window MMCO flag. */
		if (!p->non_reference)
			write_bits(writer, 0, 1);
		write_ue(writer, 0); /* cabac_init_idc */
	}
	write_se(writer, (int)p->qp - 26);
	if (p->deblock.control_present) {
		write_ue(writer, p->deblock.disable_idc);
		if (p->deblock.disable_idc != 1) {
			write_se(writer, p->deblock.alpha_c0_offset_div2);
			write_se(writer, p->deblock.beta_offset_div2);
		}
	}
	/* CABAC alignment uses one bits. */
	while ((writer->bits & 7) && !writer->error)
		write_bits(writer, 1, 1);
}

int fh8626_pae_slice_header(const struct fh8626_pae_picture *p,
			    u32 first_mb, u8 *dst, size_t capacity)
{
	struct pae_bits writer = { .data = dst, .capacity = capacity };

	if (p->qp > 51 || !deblock_valid(&p->deblock) ||
	    first_mb >= (p->coded_width >> 4) * (p->coded_height >> 4))
		return -EINVAL;
	slice_header(&writer, p, first_mb);
	return writer.error ? writer.error : writer.bits;
}

/* H.264 Table 9-44: codIRangeLPS by pStateIdx and qCodIRangeIdx. */
static const u8 cabac_range_lps[64][4] = {
	{ 128, 176, 208, 240 }, { 128, 167, 197, 227 },
	{ 128, 158, 187, 216 }, { 123, 150, 178, 205 },
	{ 116, 142, 169, 195 }, { 111, 135, 160, 185 },
	{ 105, 128, 152, 175 }, { 100, 122, 144, 166 },
	{ 95, 116, 137, 158 }, { 90, 110, 130, 150 },
	{ 85, 104, 123, 142 }, { 81, 99, 117, 135 },
	{ 77, 94, 111, 128 }, { 73, 89, 105, 122 },
	{ 69, 85, 100, 116 }, { 66, 80, 95, 110 },
	{ 62, 76, 90, 104 }, { 59, 72, 86, 99 },
	{ 56, 69, 81, 94 }, { 53, 65, 77, 89 },
	{ 51, 62, 73, 85 }, { 48, 59, 69, 80 },
	{ 46, 56, 66, 76 }, { 43, 53, 63, 72 },
	{ 41, 50, 59, 69 }, { 39, 48, 56, 65 },
	{ 37, 45, 54, 62 }, { 35, 43, 51, 59 },
	{ 33, 41, 48, 56 }, { 32, 39, 46, 53 },
	{ 30, 37, 43, 50 }, { 29, 35, 41, 48 },
	{ 27, 33, 39, 45 }, { 26, 31, 37, 43 },
	{ 24, 30, 35, 41 }, { 23, 28, 33, 39 },
	{ 22, 27, 32, 37 }, { 21, 26, 30, 35 },
	{ 20, 24, 29, 33 }, { 19, 23, 27, 31 },
	{ 18, 22, 26, 30 }, { 17, 21, 25, 28 },
	{ 16, 20, 23, 27 }, { 15, 19, 22, 25 },
	{ 14, 18, 21, 24 }, { 14, 17, 20, 23 },
	{ 13, 16, 19, 22 }, { 12, 15, 18, 21 },
	{ 12, 14, 17, 20 }, { 11, 14, 16, 19 },
	{ 11, 13, 15, 18 }, { 10, 12, 15, 17 },
	{ 10, 12, 14, 16 }, { 9, 11, 13, 15 },
	{ 9, 11, 12, 14 }, { 8, 10, 12, 14 },
	{ 8, 9, 11, 13 }, { 7, 9, 11, 12 }, { 7, 9, 10, 12 }, { 7, 8, 10, 11 },
	{ 6, 8, 9, 11 }, { 6, 7, 9, 10 }, { 6, 7, 8, 9 }, { 2, 2, 2, 2 },
};

/* H.264 Table 9-45: transIdxLPS. */
static const u8 cabac_next_lps[64] = {
	0, 0, 1, 2, 2, 4, 4, 5, 6, 7, 8, 9, 9, 11, 11, 12,
	13, 13, 15, 15, 16, 16, 18, 18, 19, 19, 21, 21, 22, 22, 23, 24,
	24, 25, 26, 26, 27, 27, 28, 29, 29, 30, 30, 30, 31, 32, 32, 33,
	33, 33, 34, 34, 35, 35, 35, 36, 36, 36, 37, 37, 37, 38, 38, 63,
};

/* Arithmetic encoder of H.264 clause 9.3.4.2. */
struct pae_cabac {
	struct pae_bits *writer;
	u32 low;
	u32 range;
	u32 outstanding;
	bool first;
};

static void cabac_put(struct pae_cabac *c, u32 bit)
{
	if (c->first)
		c->first = false;
	else
		write_bits(c->writer, bit, 1);
	for (; c->outstanding; c->outstanding--)
		write_bits(c->writer, !bit, 1);
}

static void cabac_renorm(struct pae_cabac *c)
{
	while (c->range < 256) {
		if (c->low < 256) {
			cabac_put(c, 0);
		} else if (c->low >= 512) {
			c->low -= 512;
			cabac_put(c, 1);
		} else {
			c->low -= 256;
			c->outstanding++;
		}
		c->range <<= 1;
		c->low <<= 1;
	}
}

static void cabac_decision(struct pae_cabac *c, u8 *state, u8 *mps, u32 bin)
{
	u32 lps = cabac_range_lps[*state][(c->range >> 6) & 3];

	c->range -= lps;
	if (bin != *mps) {
		c->low += c->range;
		c->range = lps;
		if (!*state)
			*mps ^= 1;
		*state = cabac_next_lps[*state];
	} else if (*state < 62) {
		(*state)++;
	}
	cabac_renorm(c);
}

/* end_of_slice_flag; the final one flushes and writes the stop bit. */
static void cabac_terminate(struct pae_cabac *c, bool last)
{
	c->range -= 2;
	if (!last) {
		cabac_renorm(c);
		return;
	}
	c->low += c->range;
	c->range = 2;
	cabac_renorm(c);
	cabac_put(c, (c->low >> 9) & 1);
	write_bits(c->writer, ((c->low >> 7) & 3) | 1, 2);
}

int fh8626_pae_skip_picture(const struct fh8626_pae_picture *p,
			    u8 *dst, size_t capacity)
{
	u8 rbsp[256];
	struct pae_bits writer = { .data = rbsp, .capacity = sizeof(rbsp) };
	struct pae_cabac cabac = {
		.writer = &writer, .range = 510, .first = true,
	};
	u32 mbs = (p->coded_width >> 4) * (p->coded_height >> 4), mb;
	int pre;
	u8 state, mps;

	if (p->idr || !mbs || p->qp > 51 || !deblock_valid(&p->deblock))
		return -EINVAL;
	slice_header(&writer, p, 0);
	/*
	 * mb_skip_flag of a P slice, cabac_init_idc 0: ctxIdx 11 with
	 * (m, n) = (23, 33). Neighbours are all skipped, so ctxIdxInc stays 0.
	 */
	pre = clamp(((23 * (int)p->qp) >> 4) + 33, 1, 126);
	if (pre <= 63) {
		state = 63 - pre;
		mps = 0;
	} else {
		state = pre - 64;
		mps = 1;
	}
	for (mb = 0; mb < mbs && !writer.error; mb++) {
		cabac_decision(&cabac, &state, &mps, 1);
		cabac_terminate(&cabac, mb + 1 == mbs);
	}
	/* rbsp_stop_one_bit came from the flush; align with zero bits. */
	if (writer.bits & 7)
		write_bits(&writer, 0, 8 - (writer.bits & 7));
	return annex_b(dst, capacity, p->non_reference ? 0x01 : 0x41,
		       &writer);
}
