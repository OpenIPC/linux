/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Ingenic T23 ISP statistics and parameter buffer formats, and controls
 *
 * Statistics are captured on a V4L2_BUF_TYPE_META_CAPTURE node using
 * V4L2_META_FMT_T23_ISP_STATS. Parameters are queued on a
 * V4L2_BUF_TYPE_META_OUTPUT node using V4L2_META_FMT_T23_ISP_EXT_PARAMS.
 *
 * Copyright (C) 2026 Difan Zhang
 */
#ifndef _UAPI_MEDIA_INGENIC_T23_ISP_CONFIG_H
#define _UAPI_MEDIA_INGENIC_T23_ISP_CONFIG_H

#include <linux/types.h>
#include <linux/v4l2-controls.h>

#define T23_ISP_GRID_ROWS		15
#define T23_ISP_GRID_COLS		15
#define T23_ISP_ZONES			(T23_ISP_GRID_ROWS * T23_ISP_GRID_COLS)
#define T23_ISP_HIST_BINS		256

#define T23_ISP_STATS_VERSION		1

/* Valid statistics in struct t23_isp_stat_buffer::flags */
#define T23_ISP_STATS_AE		(1U << 0)
#define T23_ISP_STATS_HIST		(1U << 1)
#define T23_ISP_STATS_AWB		(1U << 2)

/**
 * struct t23_isp_ae_zone - luma statistics for one zone
 * @sum_low: sum of 8-bit luma over pixels at or below @ae_low
 * @sum_mid: sum of luma over pixels between the two thresholds
 * @sum_high: sum of luma over pixels at or above @ae_high
 * @count_low: number of samples in the low class
 * @count_high: number of samples in the high class
 *
 * The mid-class count is the zone sample count minus both class counts.
 * Samples reflect sensor integration time, analogue gain and ISP digital gain.
 */
struct t23_isp_ae_zone {
	__u32 sum_low;
	__u32 sum_mid;
	__u32 sum_high;
	__u16 count_low;
	__u16 count_high;
};

/**
 * struct t23_isp_awb_zone - colour statistics for one zone
 * @sum_r: red sum over samples inside the AWB colour and luma windows
 * @sum_g: green sum over the same samples
 * @sum_b: blue sum over the same samples
 * @count: number of samples inside the windows
 *
 * Values are sampled in the linear pre-WB domain before white-balance gains.
 */
struct t23_isp_awb_zone {
	__u32 sum_r;
	__u32 sum_g;
	__u32 sum_b;
	__u32 count;
};

/**
 * struct t23_isp_stat_buffer - statistics for one processed frame
 * @version: T23_ISP_STATS_VERSION
 * @size: sizeof(struct t23_isp_stat_buffer)
 * @flags: T23_ISP_STATS_* bits marking valid sections
 * @sequence: sequence number of the matching capture buffer
 * @zone_width: zone width in Bayer quads
 * @zone_height: zone height in Bayer quads
 * @ae_low: upper luma bound of the AE low class
 * @ae_high: lower luma bound of the AE high class
 * @reserved0: must be zero
 * @reserved: must be zero
 * @ae: AE zones, row-major
 * @hist: 21-bit luma histograms (0..0x1fffff); set 0 is the full-frame
 *	  pre-gamma luma histogram, set 1 is the secondary hardware bank
 * @awb: AWB zones, row-major
 */
struct t23_isp_stat_buffer {
	__u32 version;
	__u32 size;
	__u32 flags;
	__u32 sequence;
	__u16 zone_width;
	__u16 zone_height;
	__u8 ae_low;
	__u8 ae_high;
	__u16 reserved0;
	__u32 reserved[4];
	struct t23_isp_ae_zone ae[T23_ISP_ZONES];
	__u32 hist[2][T23_ISP_HIST_BINS];
	struct t23_isp_awb_zone awb[T23_ISP_ZONES];
};

#define T23_ISP_GAMMA_POINTS		129
#define T23_ISP_LSC_MAX_WORDS		2046
#define T23_ISP_MDNS_WORDS		135
#define T23_ISP_SDNS_WORDS		104
#define T23_ISP_YDNS_WORDS		4
#define T23_ISP_YSP_WORDS		32
#define T23_ISP_DMSC_WORDS		128

/**
 * struct t23_isp_blc_config - black-level pedestals per Bayer channel
 * @r: red channel black level (12-bit scale, 0..4095)
 * @gr: green-red channel black level (12-bit scale, 0..4095)
 * @gb: green-blue channel black level (12-bit scale, 0..4095)
 * @b: blue channel black level (12-bit scale, 0..4095)
 */
struct t23_isp_blc_config {
	__u16 r;
	__u16 gr;
	__u16 gb;
	__u16 b;
};

/**
 * struct t23_isp_wb_config - white-balance gains
 * @gain_r: red gain, 1.0 = 1024, range 0..0x3fff
 * @gain_b: blue gain, 1.0 = 1024, range 0..0x3fff
 */
struct t23_isp_wb_config {
	__u16 gain_r;
	__u16 gain_b;
};

/**
 * struct t23_isp_ccm_config - colour correction matrix
 * @coeff: row-major 3x3 matrix applied to linear RGB, 1.0 = 1024, range
 *	   -8192..8191
 * @reserved: must be zero
 */
struct t23_isp_ccm_config {
	__s16 coeff[9];
	__u16 reserved;
};

/**
 * struct t23_isp_gamma_config - luma-independent RGB gamma curve
 * @curve: 129 equally spaced points over the 12-bit input, values 0..4095
 * @reserved: must be zero
 */
struct t23_isp_gamma_config {
	__u16 curve[T23_ISP_GAMMA_POINTS];
	__u16 reserved;
};

/**
 * struct t23_isp_lsc_config - lens shading correction mesh
 * @mesh_width: mesh nodes per row
 * @mesh_height: mesh rows
 * @stride: mesh stride field of the LSC control register
 * @scale: gain scale, 0 = 1.0 at 2048, 1 = 1024, 2 = 512, 3 = 256
 * @mean_en: enable the mean mode of the LSC block (0 or 1)
 * @words: used entries of @lut, a multiple of three, at most
 *	   T23_ISP_LSC_MAX_WORDS
 * @lut: mesh gains, two 12-bit gains per word (high gain in bits 23:12)
 */
struct t23_isp_lsc_config {
	__u16 mesh_width;
	__u16 mesh_height;
	__u16 stride;
	__u8 scale;
	__u8 mean_en;
	__u32 words;
	__u32 lut[T23_ISP_LSC_MAX_WORDS];
};

/**
 * struct t23_isp_mdns_config - temporal (3D) noise reduction
 * @top: function enables, statistics groups and debug control, in register
 *	 order 0x7810, 0x7814, 0x7808; top[0] bits outside 0xd1111111,
 *	 top[1] bits outside 0x00f1310f (which would enable unallocated history
 *	 planes), and any non-zero top[2] are rejected with -EINVAL
 * @block_size: motion statistics block size in pixels, a multiple of 4,
 *		16..255; the driver derives the block grid from the frame size
 * @sta_cfg: motion statistics control
 * @sta_thres: motion statistics thresholds
 * @val: Y and C filter registers 0x78c8..0x7aec in ascending order, without
 *	 the grid and chroma window registers
 */
struct t23_isp_mdns_config {
	__u32 top[3];
	__u32 block_size;
	__u32 sta_cfg;
	__u32 sta_thres;
	__u32 val[T23_ISP_MDNS_WORDS];
};

/*
 * Extensible parameter block format (V4L2_META_FMT_T23_ISP_EXT_PARAMS)
 */
#define T23_ISP_EXT_PARAMS_V1		1
#define T23_ISP_EXT_PARAMS_MAX_SIZE	12288

#define T23_ISP_EXT_PARAMS_FL_BLOCK_DISABLE	(1U << 0)
#define T23_ISP_EXT_PARAMS_FL_BLOCK_ENABLE	(1U << 1)

enum t23_isp_ext_params_block_type {
	T23_ISP_EXT_PARAMS_BLOCK_TYPE_BLC = 0,
	T23_ISP_EXT_PARAMS_BLOCK_TYPE_WB,
	T23_ISP_EXT_PARAMS_BLOCK_TYPE_CCM,
	T23_ISP_EXT_PARAMS_BLOCK_TYPE_GAMMA,
	T23_ISP_EXT_PARAMS_BLOCK_TYPE_LSC,
	T23_ISP_EXT_PARAMS_BLOCK_TYPE_MDNS,
	T23_ISP_EXT_PARAMS_BLOCK_TYPE_SDNS,
	T23_ISP_EXT_PARAMS_BLOCK_TYPE_YDNS,
	T23_ISP_EXT_PARAMS_BLOCK_TYPE_YSP,
	T23_ISP_EXT_PARAMS_BLOCK_TYPE_DMSC,
};

/**
 * struct t23_isp_ext_params_block_header - extensible parameter block header
 * @type: enum t23_isp_ext_params_block_type
 * @flags: bitmask of T23_ISP_EXT_PARAMS_FL_BLOCK_*
 * @size: total size in bytes of the block including this header (8-byte aligned)
 */
struct t23_isp_ext_params_block_header {
	__u16 type;
	__u16 flags;
	__u32 size;
};

/**
 * struct t23_isp_ext_params_blc_config - extensible BLC parameter block
 * @header: block header
 * @config: black-level configuration
 */
struct t23_isp_ext_params_blc_config {
	struct t23_isp_ext_params_block_header header;
	struct t23_isp_blc_config config;
} __attribute__((aligned(8)));

/**
 * struct t23_isp_ext_params_wb_config - extensible WB parameter block
 * @header: block header
 * @config: white-balance gain configuration
 * @reserved: must be zero
 */
struct t23_isp_ext_params_wb_config {
	struct t23_isp_ext_params_block_header header;
	struct t23_isp_wb_config config;
	__u32 reserved;
} __attribute__((aligned(8)));

/**
 * struct t23_isp_ext_params_ccm_config - extensible CCM parameter block
 * @header: block header
 * @config: colour correction matrix configuration
 * @reserved: must be zero
 */
struct t23_isp_ext_params_ccm_config {
	struct t23_isp_ext_params_block_header header;
	struct t23_isp_ccm_config config;
	__u32 reserved;
} __attribute__((aligned(8)));

/**
 * struct t23_isp_ext_params_gamma_config - extensible gamma parameter block
 * @header: block header
 * @config: gamma curve configuration
 * @reserved: must be zero
 */
struct t23_isp_ext_params_gamma_config {
	struct t23_isp_ext_params_block_header header;
	struct t23_isp_gamma_config config;
	__u32 reserved;
} __attribute__((aligned(8)));

/**
 * struct t23_isp_ext_params_lsc_config - extensible LSC parameter block
 * @header: block header
 * @config: lens shading correction mesh configuration
 * @reserved: must be zero
 */
struct t23_isp_ext_params_lsc_config {
	struct t23_isp_ext_params_block_header header;
	struct t23_isp_lsc_config config;
	__u32 reserved;
} __attribute__((aligned(8)));

/**
 * struct t23_isp_ext_params_mdns_config - extensible MDNS parameter block
 * @header: block header
 * @config: temporal noise reduction configuration
 * @reserved: must be zero
 */
struct t23_isp_ext_params_mdns_config {
	struct t23_isp_ext_params_block_header header;
	struct t23_isp_mdns_config config;
	__u32 reserved;
} __attribute__((aligned(8)));

/**
 * struct t23_isp_ext_params_sdns_config - spatial noise reduction registers
 * @header: block header
 * @words: register values 0x8800..0x88c0 and 0x8a60..0x8b38 in ascending order
 */
struct t23_isp_ext_params_sdns_config {
	struct t23_isp_ext_params_block_header header;
	__u32 words[T23_ISP_SDNS_WORDS];
} __attribute__((aligned(8)));

/**
 * struct t23_isp_ext_params_ydns_config - luma noise reduction registers
 * @header: block header
 * @words: register values 0x7af0..0x7afc in ascending order
 */
struct t23_isp_ext_params_ydns_config {
	struct t23_isp_ext_params_block_header header;
	__u32 words[T23_ISP_YDNS_WORDS];
} __attribute__((aligned(8)));

/**
 * struct t23_isp_ext_params_ysp_config - luma sharpening registers
 * @header: block header
 * @words: register values 0x7000..0x707c in ascending order
 */
struct t23_isp_ext_params_ysp_config {
	struct t23_isp_ext_params_block_header header;
	__u32 words[T23_ISP_YSP_WORDS];
} __attribute__((aligned(8)));

/**
 * struct t23_isp_ext_params_dmsc_config - demosaic registers
 * @header: block header
 * @words: register values 0x4800..0x4a5c in ascending order, excluding the
 *	   commit register 0x499c and unused addresses
 */
struct t23_isp_ext_params_dmsc_config {
	struct t23_isp_ext_params_block_header header;
	__u32 words[T23_ISP_DMSC_WORDS];
} __attribute__((aligned(8)));

/**
 * struct t23_isp_ext_params_cfg - extensible ISP parameter buffer
 * @version: T23_ISP_EXT_PARAMS_V1
 * @data_size: effective byte length of concatenated blocks in @data
 * @data: concatenated struct t23_isp_ext_params_*_config blocks
 */
struct t23_isp_ext_params_cfg {
	__u32 version;
	__u32 data_size;
	__u8 data[T23_ISP_EXT_PARAMS_MAX_SIZE];
} __attribute__((aligned(8)));

/*
 * Privacy masks of an input, set on the ISP subdev: a __u16 array of
 * T23_ISP_PRIVACY_MASKS rows of left, top, width and height in sensor
 * pixels. A row with zero width or height is off. Every output of the input
 * is masked.
 */
#define T23_ISP_PRIVACY_MASKS		4
#define V4L2_CID_T23_PRIVACY_MASK	(V4L2_CID_USER_T23_BASE + 0)
/* Fill colour of the privacy masks, 0xRRGGBB */
#define V4L2_CID_T23_PRIVACY_MASK_COLOR	(V4L2_CID_USER_T23_BASE + 1)

/*
 * Vector lines of an input, set on the ISP subdev: a __u16 array of
 * T23_ISP_DRAW_LINES rows of x0, y0, x1, y1 and width in sensor pixels.
 * A row with zero width or equal endpoints is off. Drawn on MSCA channel 0.
 */
#define T23_ISP_DRAW_LINES		6
#define V4L2_CID_T23_DRAW_LINE		(V4L2_CID_USER_T23_BASE + 2)
/* Stroke colour of the vector lines, 0xRRGGBB */
#define V4L2_CID_T23_DRAW_LINE_COLOR	(V4L2_CID_USER_T23_BASE + 3)

#endif /* _UAPI_MEDIA_INGENIC_T23_ISP_CONFIG_H */
