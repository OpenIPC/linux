/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * Copyright (C) 2026 Difan Zhang
 */
#ifndef _UAPI_FH8626_ISP_H
#define _UAPI_FH8626_ISP_H

#include <linux/videodev2.h>

/*
 * Read with VIDIOC_G_EXT_CTRLS on the fh8626-isp subdevice. The payload is
 * an array of FH8626_ISP_STATS_WORDS native-endian __u32 values. A frame-sync
 * event announces each new snapshot; reading does not consume capture buffers.
 * R/G/B sums and accepted-pixel counts come from the ISP AWB window hardware.
 * Counts cover pixels accepted by the configured white-point filter.
 */
#define V4L2_CID_FH8626_STATS (V4L2_CID_USER_BASE + 0x10f0)
#define V4L2_CID_FH8626_AE_STATS (V4L2_CID_USER_BASE + 0x10f1)
/*
 * Bayer temporal noise reduction (NR3D). While streaming, NR3D controls
 * take effect at the next frame boundary.
 *
 * NR3D_STRENGTH: temporal filter strength, 1..255. 0 bypasses NR3D; it is
 *   also bypassed while the history buffers are unavailable.
 * NR3D_NOISE: 19-bit noise level used to tell noise from motion.
 * NR3D_NOISE_SHIFT: 4-bit scale applied to NR3D_NOISE.
 * NR3D_MOTION: motion feedback weight, 0..255.
 * NR3D_BLEND: temporal coefficient, 0..255.
 * NR3D_PRESET: filter coefficient set, 0 for the start-up set, 1..4 for
 *   four further predefined sets.
 */
#define V4L2_CID_FH8626_NR3D_STRENGTH (V4L2_CID_USER_BASE + 0x10f2)
/*
 * Q9 green white-balance gain. Unity is 512. Red and blue use
 * V4L2_CID_RED_BALANCE and V4L2_CID_BLUE_BALANCE with the same range.
 * Both green Bayer slots take this value. The control is the gain before
 * the black-level range compensation applied by the driver.
 */
#define V4L2_CID_FH8626_WB_GAIN_G (V4L2_CID_USER_BASE + 0x10f3)
#define V4L2_CID_FH8626_NR3D_NOISE (V4L2_CID_USER_BASE + 0x10f4)
#define V4L2_CID_FH8626_NR3D_NOISE_SHIFT (V4L2_CID_USER_BASE + 0x10f5)
#define V4L2_CID_FH8626_NR3D_MOTION (V4L2_CID_USER_BASE + 0x10f6)
#define V4L2_CID_FH8626_NR3D_BLEND (V4L2_CID_USER_BASE + 0x10f7)
#define V4L2_CID_FH8626_NR3D_PRESET (V4L2_CID_USER_BASE + 0x10f8)
/*
 * Bayer-domain noise filter, programmed identically for all four Bayer
 * channels: STRENGTH 0..255, NOISE a 19-bit noise level, NOISE_SHIFT its
 * 4-bit scale. Same noise model as NR3D, separate thresholds.
 */
#define V4L2_CID_FH8626_BNR_STRENGTH (V4L2_CID_USER_BASE + 0x10f9)
#define V4L2_CID_FH8626_BNR_NOISE (V4L2_CID_USER_BASE + 0x10fa)
#define V4L2_CID_FH8626_BNR_NOISE_SHIFT (V4L2_CID_USER_BASE + 0x10fb)
/*
 * Bayer black level, U16 array of FH8626_ISP_BLACK_LEVELS entries in
 * R, Gr, Gb, B order, 12-bit (0..4095). The ISP subtracts it before white
 * balance; the driver rescales the white-balance gains so that the
 * remaining range still reaches full scale. AE statistics are taken after
 * the subtraction.
 */
#define V4L2_CID_FH8626_BLACK_LEVEL (V4L2_CID_USER_BASE + 0x10fc)
#define FH8626_ISP_BLACK_LEVELS 4
/*
 * Colour correction matrix, INTEGER array of FH8626_ISP_CCM_ENTRIES
 * entries in row-major order (output R, G, B rows; input R, G, B columns).
 * Signed Q9: 512 is 1.0, range -4096..4095. Identity by default.
 */
#define V4L2_CID_FH8626_CCM (V4L2_CID_USER_BASE + 0x10fd)
#define FH8626_ISP_CCM_ENTRIES 9
/*
 * Tone curves, U16 arrays of FH8626_ISP_GAMMA_POINTS 10-bit output values
 * (0..1023), ordered from black to white. GAMMA_RGB is applied to each
 * colour channel before the colour space conversion; GAMMA_Y is applied
 * to luma afterwards. The hardware fixes the input positions of the points
 * with denser spacing for the first points.
 */
#define V4L2_CID_FH8626_GAMMA_RGB (V4L2_CID_USER_BASE + 0x10fe)
#define V4L2_CID_FH8626_GAMMA_Y (V4L2_CID_USER_BASE + 0x10ff)
#define FH8626_ISP_GAMMA_POINTS 160
/*
 * Highlight chroma suppression threshold, 10-bit luma (0..1023). Pixels
 * whose luma reaches the threshold lose their chroma, so that highlights
 * where one colour channel has clipped come out white.
 * The default, 1023, limits the suppression to fully saturated pixels.
 * The level where clipping starts depends on the white-balance gains, so
 * the white-balance loop is expected to update the threshold with them.
 */
#define V4L2_CID_FH8626_HLR_THRESHOLD (V4L2_CID_USER_BASE + 0x1100)
#define FH8626_ISP_HLR_THRESHOLD_MAX 1023
/*
 * Luma noise filter (YNR), working on the converted luma before the luma
 * tone curve. It has three filter bands.
 *
 * YNR_THRESHOLD: U16 array of FH8626_ISP_YNR_BANDS difference thresholds,
 *   one per band. Larger values smooth across larger luma steps.
 * YNR_STRENGTH: U16 array of FH8626_ISP_YNR_BANDS Q6 weights (64 is 1.0,
 *   0..1023) applied to each band's fixed weight table. 0 turns a band off;
 *   all zero leaves luma unfiltered.
 * YNR_NOISE: U16 array of FH8626_ISP_YNR_NOISE_POINTS 8-bit noise levels,
 *   at mid-dark (luma 80) and mid-grey (luma 128) and above. The levels
 *   scale the thresholds with brightness; the second is raised to the first
 *   when lower.
 */
#define V4L2_CID_FH8626_YNR_THRESHOLD (V4L2_CID_USER_BASE + 0x1101)
#define V4L2_CID_FH8626_YNR_STRENGTH (V4L2_CID_USER_BASE + 0x1102)
#define V4L2_CID_FH8626_YNR_NOISE (V4L2_CID_USER_BASE + 0x1103)
#define FH8626_ISP_YNR_BANDS 3
#define FH8626_ISP_YNR_NOISE_POINTS 2
/*
 * Chroma noise filter (CNR) on Cb/Cr. CNR_STRENGTH, 0..255, 0 disables the
 * smoothing. CNR_THRESHOLD is a U16 array of FH8626_ISP_CNR_THRESHOLDS
 * 8-bit chroma difference thresholds, lower then upper.
 */
#define V4L2_CID_FH8626_CNR_STRENGTH (V4L2_CID_USER_BASE + 0x1104)
#define V4L2_CID_FH8626_CNR_THRESHOLD (V4L2_CID_USER_BASE + 0x1105)
#define FH8626_ISP_CNR_THRESHOLDS 2
/*
 * Edge enhancement, U16 arrays of FH8626_ISP_APC_PATHS 8-bit values for
 * the detail (fine texture) path and the edge path, in that order.
 * APC_STRENGTH limits the enhancement amplitude; V4L2_CID_SHARPNESS scales
 * it, 128 leaving it unchanged and 0 turning enhancement off. APC_GAIN is
 * the per-path gain.
 */
#define V4L2_CID_FH8626_APC_STRENGTH (V4L2_CID_USER_BASE + 0x1106)
#define V4L2_CID_FH8626_APC_GAIN (V4L2_CID_USER_BASE + 0x1107)
#define FH8626_ISP_APC_PATHS 2
/*
 * Chroma gain scale, Q6 (64 is 1.0, 0..255). Multiplies
 * V4L2_CID_SATURATION without changing that control's value.
 */
#define V4L2_CID_FH8626_SATURATION_SCALE (V4L2_CID_USER_BASE + 0x1108)
/*
 * Purple fringe correction (PFC). Chroma within PFC_HUE_RANGE degrees
 * (1..89) of the PFC_HUE direction (0..359 degrees in the Cb/Cr plane) is
 * desaturated next to strong luma edges. The defaults, 15 and 30 degrees,
 * select the purple fringe hue.
 */
#define V4L2_CID_FH8626_PFC_ENABLE (V4L2_CID_USER_BASE + 0x1109)
#define V4L2_CID_FH8626_PFC_HUE (V4L2_CID_USER_BASE + 0x110a)
#define V4L2_CID_FH8626_PFC_HUE_RANGE (V4L2_CID_USER_BASE + 0x110b)

/*
 * Zone statistics, read with VIDIOC_G_EXT_CTRLS like V4L2_CID_FH8626_STATS:
 * an array of FH8626_ISP_GRID_STATS_WORDS __u32 laid out as struct
 * fh8626_isp_grid_stats, published with the same sequence as the other
 * statistics.
 */
#define V4L2_CID_FH8626_GRID_STATS (V4L2_CID_USER_BASE + 0x110c)

/*
 * Privacy masks: opaque rectangles drawn over the ISP output before it is
 * scaled, so that they cover the same scene area on every capture channel
 * whatever its size. The control is a U32 array of FH8626_PRIVACY_MASKS x
 * FH8626_PRIVACY_MASK_WORDS elements laid out as an array of struct
 * fh8626_privacy_mask. It is set on the fh8626-isp subdevice and is also
 * reachable through the capture video nodes.
 *
 * Coordinates are in pixels of the ISP source pad format. Each edge is
 * moved outwards to an even pixel; parts outside the frame are not drawn.
 * color is 0xRRGGBB. A mask is drawn only while FH8626_PRIVACY_MASK_ENABLE
 * is set in flags; later masks are drawn over earlier ones.
 *
 * The masks use the logo window that is blended into all channels: while
 * any mask is enabled, that logo output node cannot be started, and the
 * control returns EBUSY while that node is streaming.
 */
#define V4L2_CID_FH8626_PRIVACY_MASKS (V4L2_CID_USER_BASE + 0x110d)
#define FH8626_PRIVACY_MASKS 8
#define FH8626_PRIVACY_MASK_WORDS 6
#define FH8626_PRIVACY_MASK_ENABLE (1U << 0)

struct fh8626_privacy_mask {
	__u32 left;
	__u32 top;
	__u32 width;
	__u32 height;
	__u32 color;
	__u32 flags;
};

#define FH8626_ISP_STATS_VERSION 1
#define FH8626_ISP_STATS_BLOCKS 9
#define FH8626_ISP_STATS_WORDS (8 + FH8626_ISP_STATS_BLOCKS * 4)
#define FH8626_ISP_STATS_VALID (1U << 0)
/*
 * Effective position relative to this subdevice's ISP WB controls. These
 * flags specify gain normalization independently of the physical tap/mux.
 * Neither position flag means that the statistics tap is not established;
 * consumers must not apply an automatic WB correction in that case.
 */
#define FH8626_ISP_STATS_PRE_WB (1U << 1)
#define FH8626_ISP_STATS_POST_WB (1U << 2)

struct fh8626_isp_stats {
	__u32 version;
	__u32 sequence;
	__u32 timestamp_ns_low;
	__u32 timestamp_ns_high;
	__u32 flags;
	__u32 bayer_selector;
	__u32 width;
	__u32 height;
	struct {
		__u32 r_sum;
		__u32 g_sum;
		__u32 b_sum;
		__u32 count;
	} blocks[FH8626_ISP_STATS_BLOCKS];
};

/*
 * Independent AE DMA meter, same FRAME_SYNC sequence as the AWB snapshot.
 * The nine windows are fixed unions of the zone statistics cells; sum adds
 * the 12-bit samples of all four Bayer sites, so sum / count is the linear
 * mean and isqrt(min(sum / count, 4096) << 12) its square-root (perceptual)
 * 12-bit level. 64-bit values are split into low and high words to avoid ABI
 * alignment holes.
 */
struct fh8626_isp_ae_stats {
	__u32 version;
	__u32 sequence;
	__u32 timestamp_ns_low;
	__u32 timestamp_ns_high;
	__u32 flags;
	__u32 bayer_selector;
	__u32 width;
	__u32 height;
	struct {
		__u32 sum_low, sum_high;
		__u32 count_low, count_high;
	} blocks[FH8626_ISP_STATS_BLOCKS];
};

/*
 * Zone statistics of the Bayer data after black-level subtraction and the
 * ISP digital gain, and before white balance. The frame is split into
 * rows x cols cells in raster order. For each cell, sum[] holds the 12-bit
 * sample sums of the R, Gr, Gb and B sites (Gr shares rows with R) and
 * count the number of samples summed over all four, so sum[c] * 4 / count
 * is the mean of colour c. max is the largest single sample of the cell
 * over all four colours, which shows clipped highlights that the means
 * hide.
 *
 * motion is a bitmap of motion_rows x motion_cols blocks in raster order,
 * bit n of the array being block n; a set bit marks a block that changed
 * against the previous frame. It is valid only when
 * FH8626_ISP_GRID_MOTION_VALID is set in flags.
 */
#define FH8626_ISP_GRID_COLS 16
#define FH8626_ISP_GRID_ROWS 16
#define FH8626_ISP_GRID_CELLS (FH8626_ISP_GRID_COLS * FH8626_ISP_GRID_ROWS)
#define FH8626_ISP_GRID_CHANNELS 4
#define FH8626_ISP_MOTION_COLS 24
#define FH8626_ISP_MOTION_ROWS 16
#define FH8626_ISP_MOTION_WORDS \
	(FH8626_ISP_MOTION_COLS * FH8626_ISP_MOTION_ROWS / 32)
#define FH8626_ISP_GRID_MOTION_VALID (1U << 3)
#define FH8626_ISP_GRID_STATS_WORDS \
	(12 + FH8626_ISP_MOTION_WORDS + \
	 FH8626_ISP_GRID_CELLS * (FH8626_ISP_GRID_CHANNELS + 2))

struct fh8626_isp_grid_stats {
	__u32 version;
	__u32 sequence;
	__u32 timestamp_ns_low;
	__u32 timestamp_ns_high;
	__u32 flags;
	__u32 bayer_selector;
	__u32 width;
	__u32 height;
	__u32 cols;
	__u32 rows;
	__u32 motion_cols;
	__u32 motion_rows;
	__u32 motion[FH8626_ISP_MOTION_WORDS];
	struct {
		__u32 sum[FH8626_ISP_GRID_CHANNELS];
		__u32 count;
		__u32 max;
	} cells[FH8626_ISP_GRID_CELLS];
};

#endif
