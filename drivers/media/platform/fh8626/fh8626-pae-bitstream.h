/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2026 Difan Zhang
 */
#ifndef FH8626_PAE_BITSTREAM_H
#define FH8626_PAE_BITSTREAM_H

#include <linux/types.h>

/* H.264 aspect_ratio_idc value selecting an explicit sar_width/height. */
#define FH8626_PAE_SAR_EXTENDED 255

/*
 * In-loop deblocking filter settings. When control_present is false the
 * PPS clears deblocking_filter_control_present_flag and slices use the
 * defaults: filter enabled across slice edges with zero offsets.
 */
struct fh8626_pae_deblock {
	bool control_present;
	u8 disable_idc;	/* 0 on, 1 off, 2 off at slice edges */
	s8 alpha_c0_offset_div2;	/* -6..6 */
	s8 beta_offset_div2;		/* -6..6 */
};

/* Sequence VUI fields; values are the H.264 Annex E code points. */
struct fh8626_pae_vui {
	bool aspect_ratio_present;
	u8 aspect_ratio_idc;
	u16 sar_width;
	u16 sar_height;
	bool video_signal_present;
	u8 video_format;
	bool full_range;
	bool colour_description_present;
	u8 colour_primaries;
	u8 transfer_characteristics;
	u8 matrix_coefficients;
	bool timing_present;
	u32 num_units_in_tick;
	u32 time_scale;
	bool fixed_frame_rate;
};

/*
 * One reference, progressive 4:2:0, Main profile/CABAC, 16-bit frame_num
 * and pic_order_cnt_lsb.
 */
struct fh8626_pae_picture {
	u16 width;
	u16 height;
	u16 coded_width;
	u16 coded_height;
	u16 frame_num;
	u16 poc;
	u16 idr_pic_id;
	u8 qp;
	bool idr;
	/* nal_ref_idc 0 marks a non-reference picture. */
	bool non_reference;
	struct fh8626_pae_deblock deblock;
	const struct fh8626_pae_vui *vui;
};

/* Annex-B SPS/PPS, including start codes and emulation prevention bytes. */
int fh8626_pae_parameter_sets(const struct fh8626_pae_picture *picture,
			      u8 *destination, size_t capacity);
/*
 * CABAC-aligned raw slice header for the TAE header FIFO; no start code or
 * emulation prevention. Returns the header length in bits.
 */
int fh8626_pae_slice_header(const struct fh8626_pae_picture *picture,
			    u32 first_mb, u8 *destination, size_t capacity);
/*
 * Annex-B P picture with every macroblock P_Skip, repeating the reference
 * picture. Reference status follows non_reference. Returns the byte length.
 */
int fh8626_pae_skip_picture(const struct fh8626_pae_picture *picture,
			    u8 *destination, size_t capacity);

#endif
