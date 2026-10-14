/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Anyka AK3918EV200 video encoder - Hantro/On2 8290 (hx280 family).
 *
 * Copyright (C) 2026 Alex Zhang <alex@osqdu.org>
 */

#ifndef AK_VENC_H
#define AK_VENC_H

#include <linux/fs.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-core.h>

#include "ak-venc-gop.h"
#include "ak-venc-regs.h"
#include "ak-venc-rc.h"

#define AK_VENC_NAME		"ak-venc"

/*
 * The register window is 0x430 wide but 0x200..0x3fc mirrors swreg0..127, so
 * nothing above 0x1fc may be written. The mirror lands on the IRQ, AXI and
 * address registers.
 */
#define AK_VENC_REG_WINDOW	0x1000
#define AK_VENC_REG_LAST_WRITE	0x1fc

#define AK_VENC_ASIC_ID		0x82900760

/* board-facts.md 2.13: the core's own maximum, from swreg63 bits[11:0]. */
#define AK_VENC_MAX_WIDTH	1280
#define AK_VENC_MAX_HEIGHT	1024
#define AK_VENC_MIN_WIDTH	96
#define AK_VENC_MIN_HEIGHT	64

#define AK_VENC_DEF_WIDTH	1280
#define AK_VENC_DEF_HEIGHT	720

#define AK_VENC_DEF_QP		30
#define AK_VENC_DEF_QP_MIN	20
#define AK_VENC_DEF_QP_MAX	51

#define AK_VENC_MIN_BITRATE	32000
#define AK_VENC_MAX_BITRATE	20000000
#define AK_VENC_DEF_BITRATE	2000000
#define AK_VENC_DEF_GOP		50
#define AK_VENC_DEF_FPS		25

/* The IJG scale, where 50 is the Annex K tables unscaled. */
#define AK_VENC_DEF_JPEG_QUALITY	80

/* Private controls: the 8290 tuning that has no standard CID. */
#define AK_VENC_CID_BASE		(V4L2_CID_USER_BASE + 0x1100)
#define AK_VENC_CID_INTRA_QP_DELTA	(AK_VENC_CID_BASE + 0)
#define AK_VENC_CID_I_WEIGHT		(AK_VENC_CID_BASE + 1)
#define AK_VENC_CID_MB_RC_MODE		(AK_VENC_CID_BASE + 2)
#define AK_VENC_CID_MAD_QP_DELTA	(AK_VENC_CID_BASE + 3)
#define AK_VENC_CID_MAD_THRESHOLD	(AK_VENC_CID_BASE + 4)
#define AK_VENC_CID_MAD_ADAPT		(AK_VENC_CID_BASE + 5)

#define AK_VENC_MB_RC_MODE_MAD		0
#define AK_VENC_MB_RC_MODE_CHECKPOINT	1

/* Software-filled input to the core, not scratch. Sized from the first encode. */
#define AK_VENC_CABAC_BYTES	48256U

/* -------------------------------------------------------------------------
 * The register layer. Ported from lib/video/akcam_h8290.{c,h}; the names are
 * kept so task 4 can move the shared sources in without a rename.
 * ------------------------------------------------------------------------- */

#define AKCAM_H8290_REGISTERS		320
#define AKCAM_H8290_FIRST_WRITTEN	1
#define AKCAM_H8290_LAST_WRITTEN	0x3e
#define AKCAM_H8290_ENABLE_REG		14
#define AKCAM_H8290_STATUS_REG		1

/*
 * The JPEG quantisation registers sit above the range the frame sequence
 * sweeps, so write_frame has to know about them separately. They go after the
 * sweep - which is where the encoding mode reaches the core - and before the
 * enable, because writing the enable is what starts the core.
 */
#define AKCAM_H8290_QUANT_FIRST_REG		64
#define AKCAM_H8290_QUANT_LUMA_LAST_REG		79
#define AKCAM_H8290_QUANT_CHROMA_FIRST_REG	80
#define AKCAM_H8290_QUANT_LAST_REG		95

/* "Encoding mode. streamType. 2=JPEG. 3=H264", the field's own description. */
#define AKCAM_H8290_MODE_JPEG		2u
#define AKCAM_H8290_MODE_H264		3u

struct akcam_h8290 {
	u32 reg[AKCAM_H8290_REGISTERS];
};

struct akcam_h8290_h264 {
	unsigned width, height;
	unsigned input_row_length;
	unsigned intra;
	unsigned pps_qp;		/* what the emitted PPS carries */
	unsigned initial_qp, qp_min, qp_max;
	unsigned frame_num;
	unsigned idr_pic_id;
	int      chroma_qp_index_offset;
	unsigned deblocking_mode;
	unsigned constrained_intra_pred;
	int      filter_alpha_offset_div2;
	int      filter_beta_offset_div2;
	int      mad_qp_delta;
	unsigned mad_threshold;
	unsigned checkpoint_distance;
	const u16 *checkpoint_target;
	const s16 *checkpoint_error;
	const s8  *checkpoint_delta_qp;
};

struct akcam_h8290_frames {
	u32    base;
	size_t luma, chroma;
	unsigned recon;
};

enum akcam_h8290_status {
	AKCAM_H8290_RUNNING,
	AKCAM_H8290_BUS_ERROR,
	AKCAM_H8290_SW_RESET,
	AKCAM_H8290_FRAME_READY,
	AKCAM_H8290_BUFFER_FULL
};

void akcam_h8290_set(struct akcam_h8290 *h, unsigned field, u32 value);
u32  akcam_h8290_get(const struct akcam_h8290 *h, unsigned field);
int  akcam_h8290_reg_of(unsigned field);
u32  akcam_h8290_extract(unsigned field, u32 value);

void akcam_h8290_h264_defaults(struct akcam_h8290_h264 *c);
int  akcam_h8290_config_h264(struct akcam_h8290 *h,
			     const struct akcam_h8290_h264 *c);

size_t akcam_h8290_frames_bytes(unsigned width, unsigned height);
int    akcam_h8290_frames_init(struct akcam_h8290_frames *f, u32 base,
			       unsigned width, unsigned height);
void   akcam_h8290_frames_apply(const struct akcam_h8290_frames *f,
				struct akcam_h8290 *h);
void   akcam_h8290_frames_advance(struct akcam_h8290_frames *f);

size_t akcam_h8290_input_bytes(unsigned row_length, unsigned height);
int    akcam_h8290_input_apply(struct akcam_h8290 *h, u32 base,
			       unsigned row_length, unsigned height);

int    akcam_h8290_output_apply(struct akcam_h8290 *h,
				u32 stream_base, size_t stream_bytes,
				u32 control_base, u32 cabac_base);
size_t akcam_h8290_output_bytes(u32 swreg24);

enum akcam_h8290_status akcam_h8290_status(u32 swreg1);

/*
 * Write one frame's register file in the order the core needs it: swreg1..0x3e
 * first with the enable register's enable bit forced clear, so the encoding
 * mode reaches the core while it is still stopped; then, for JPEG, the
 * quantiser bank; then the enable register again with the bit set.
 *
 * `read` retires each quantiser write and may not be NULL against real
 * hardware: those writes are posted and a burst of them collapses onto one
 * address.
 */
void akcam_h8290_write_frame(const struct akcam_h8290 *h,
			     void (*write)(void *, unsigned, u32),
			     u32 (*read)(void *, unsigned),
			     void *context);

/* -------------------------------------------------------------------------
 * JPEG. Ported from lib/video/akcam_jpeg_hdr.{c,h} and the JPEG half of
 * lib/video/akcam_h8290.c, which openjpg proves on the same hardware.
 *
 * The core codes the scan and nothing else. It has registers for the two
 * quantisation tables and none for the Huffman tables, so the entropy coder is
 * wired to the standard baseline tables of T.81 Annex K; everything up to the
 * first entropy-coded byte is software's, and the core writes the EOI.
 * ------------------------------------------------------------------------- */

#define AKCAM_JPEG_QUANT_VALUES	64

/* SOI 2, APP0 18, DQT 134, SOF0 19, DRI 6, DHT 420, SOS 14. */
#define AKCAM_JPEG_HEADER_MAX	613

struct akcam_h8290_jpeg {
	unsigned width, height;
	unsigned input_row_length;
	unsigned restart_mb_rows;	/* RST every N macroblock rows, 0 = none */
};

struct akcam_jpeg_hdr {
	unsigned width, height;
	unsigned restart_interval;	/* MCU between RST markers, 0 = none */
	const u8 *luma, *chroma;	/* 64 entries each, raster order */
};

/*
 * The lattice the hardware can hold: all of 1..32, then even, then multiples of
 * four, then of eight. The vendor masks every table it is handed exactly there.
 * A value the core rounds off is a decoder scaling by a number the encoder did
 * not use.
 */
unsigned akcam_jpeg_quant_round(unsigned value);
void akcam_jpeg_quant_tables(u8 *luma, u8 *chroma, unsigned quality);
unsigned akcam_jpeg_restart_mcus(unsigned width, unsigned mb_rows);
size_t akcam_jpeg_write_header(u8 *out, size_t cap,
			       const struct akcam_jpeg_hdr *c);

int  akcam_h8290_config_jpeg(struct akcam_h8290 *h,
			     const struct akcam_h8290_jpeg *c);
void akcam_h8290_quant_apply(struct akcam_h8290 *h, const u8 *luma,
			     const u8 *chroma);
int  akcam_h8290_jpeg_output_apply(struct akcam_h8290 *h,
				   u32 stream_base, size_t stream_bytes,
				   const u8 *header, size_t header_bytes);
size_t akcam_h8290_jpeg_stream_bytes(size_t header_bytes, u32 swreg24);

/* -------------------------------------------------------------------------
 * The syntax layer. Ported from lib/video/akcam_bits.{c,h} and
 * lib/video/akcam_h264_hdr.{c,h}, same naming rule as above.
 * ------------------------------------------------------------------------- */

struct akcam_bits {
	u8    *buf;
	size_t capacity;
	size_t bit;
	int    overflow;
};

void   akcam_bits_init(struct akcam_bits *b, u8 *buf, size_t capacity);
void   akcam_bits_put(struct akcam_bits *b, unsigned n, u32 value);
void   akcam_bits_put1(struct akcam_bits *b, int flag);
void   akcam_bits_ue(struct akcam_bits *b, u32 value);
void   akcam_bits_se(struct akcam_bits *b, s32 value);
void   akcam_bits_rbsp_trailing(struct akcam_bits *b);
size_t akcam_bits_length(const struct akcam_bits *b);
size_t akcam_bits_escape(const u8 *rbsp, size_t len, u8 *out, size_t out_cap);

struct akcam_h264_vui {
	unsigned time_scale;
	unsigned num_units_in_tick;
	unsigned fixed_frame_rate;
	unsigned sar_width, sar_height;
	unsigned video_full_range;
	unsigned pic_struct_present;
	unsigned bitstream_restriction;
	unsigned max_dec_frame_buffering;
};

struct akcam_h264_sps {
	unsigned profile_idc, level_idc;
	unsigned constraint_set0, constraint_set1;
	unsigned constraint_set2, constraint_set3;
	unsigned sps_id;
	unsigned log2_max_frame_num_minus4;
	unsigned pic_order_cnt_type;
	unsigned num_ref_frames;
	unsigned gaps_in_frame_num_allowed;
	unsigned width_mbs_minus1, height_map_units_minus1;
	unsigned frame_mbs_only, direct_8x8_inference;
	unsigned crop, crop_left, crop_right, crop_top, crop_bottom;
	struct akcam_h264_vui vui;
};

struct akcam_h264_pps {
	unsigned pps_id, sps_id;
	unsigned entropy_coding_mode;
	unsigned pic_order_present;
	unsigned num_slice_groups_minus1;
	unsigned num_ref_idx_l0_minus1, num_ref_idx_l1_minus1;
	unsigned weighted_pred;
	unsigned weighted_bipred_idc;
	int      pic_init_qp_minus26, pic_init_qs_minus26;
	int      chroma_qp_index_offset;
	unsigned deblocking_filter_control_present;
	unsigned constrained_intra_pred;
	unsigned redundant_pic_cnt_present;
	unsigned transform_8x8;
};

#define AKCAM_H264_NAL_SPS 7
#define AKCAM_H264_NAL_PPS 8

void akcam_h264_sps_defaults(struct akcam_h264_sps *s);
void akcam_h264_pps_defaults(struct akcam_h264_pps *p);
int  akcam_h264_sps_geometry(struct akcam_h264_sps *s,
			     unsigned width, unsigned height);
void akcam_h264_write_sps(struct akcam_bits *b, const struct akcam_h264_sps *s);
void akcam_h264_write_pps(struct akcam_bits *b, const struct akcam_h264_pps *p);
size_t akcam_h264_nal(u8 *out, size_t out_cap, unsigned ref_idc, unsigned type,
		      const u8 *rbsp, size_t rbsp_len);

/*
 * Assemble SPS + PPS as complete Annex B units. Returns the byte count, or 0.
 * The core writes its own start code for the slice NAL, so nothing is reserved
 * for one here.
 */
size_t ak_venc_headers(u8 *out, size_t out_cap,
		       const struct akcam_h264_sps *sps,
		       const struct akcam_h264_pps *pps);

/*
 * The trailing partial 64-bit word of a byte-aligned header, for swreg22/23
 * and the start offset in swreg37. Returns the bit count, 0..56.
 */
unsigned ak_venc_hdr_remainder(const u8 *hdr, size_t len, u32 *msb, u32 *lsb);

/* -------------------------------------------------------------------------
 * The driver
 * ------------------------------------------------------------------------- */

struct ak_venc_fmt {
	const char *name;
	u32 fourcc;
	u8  depth;		/* bits per pixel, raw formats only */
	u8  types;		/* AK_VENC_OUTPUT / AK_VENC_CAPTURE */
};

#define AK_VENC_OUTPUT		(1 << 0)
#define AK_VENC_CAPTURE		(1 << 1)

struct ak_venc_q_data {
	const struct ak_venc_fmt *fmt;
	unsigned width, height;
	unsigned bytesperline;
	unsigned sizeimage;
};

enum ak_venc_event {
	AK_VENC_EV_BUFFER_FULL,
	AK_VENC_EV_BUS_ERROR,
	AK_VENC_EV_SW_RESET,
	AK_VENC_EV_PROGRAM_FAIL,
	AK_VENC_EVENTS,
};

struct ak_venc_event_stats {
	u32 count[AK_VENC_EVENTS];
};

struct ak_venc_dev {
	struct v4l2_device	v4l2_dev;
	struct video_device	*vfd;
	struct device		*dev;
	struct v4l2_m2m_dev	*m2m_dev;

	void __iomem		*regs;
	int			irq;
	struct clk		*clk;

	struct mutex		dev_mutex;
	spinlock_t		irqlock;
	void			*alloc_ctx;

	unsigned		streaming_queues; /* queues, not contexts */

	struct ak_venc_event_stats ev_stats;
	unsigned long		ev_reported;
};

struct ak_venc_ctx {
	struct v4l2_fh		fh;
	struct ak_venc_dev	*dev;
	struct v4l2_m2m_ctx	*m2m_ctx;
	struct v4l2_ctrl_handler ctrl_handler;
	struct v4l2_ctrl		*qp_limits[2];
	struct work_struct	program_error_work;

	struct ak_venc_q_data	out_q;
	struct ak_venc_q_data	cap_q;

	struct akcam_h8290	regs;
	struct akcam_h8290_frames frames;

	/* frame stores, NAL size table and CABAC tables, one allocation */
	void			*priv_cpu;
	dma_addr_t		priv_dma;
	size_t			priv_size;
	u32			nal_off, cabac_off;
	size_t			nal_bytes;

	struct ak_venc_gop	gop;
	struct ak_venc_rc_cfg	rc_cfg;
	struct ak_venc_rc	rc;
	struct ak_venc_rc_frame	rc_frame;

	unsigned		profile;
	unsigned		level_idc;
	unsigned		mb_rc_enable;
	unsigned		mb_rc_mode;

	unsigned		pps_qp;		/* the GOP's PPS, not the frame's QP */
	unsigned		frame_num;
	unsigned		idr_pic_id;

	unsigned		streaming;	/* queues streaming, 0..2 */
	unsigned		priv_width;
	unsigned		priv_height;
	bool			aborting;

	bool			rc_dirty;	/* a control changed the config */
	bool			parm_set;	/* S_PARM supplied a frame rate */

	/*
	 * JPEG. The tables and the header live in the context rather than on
	 * the stack because device_run is reached from the interrupt handler by
	 * way of v4l2_m2m_job_finish(), and 613 bytes of header there is not
	 * stack this kernel has to spare.
	 */
	unsigned		jpeg_quality;
	u8			jpeg_luma[AKCAM_JPEG_QUANT_VALUES];
	u8			jpeg_chroma[AKCAM_JPEG_QUANT_VALUES];
	u8			jpeg_hdr[AKCAM_JPEG_HEADER_MAX];

	/* what device_run left for the interrupt handler */
	size_t			hdr_bytes;
	bool			frame_intra;
	bool			frame_jpeg;
};

#endif
