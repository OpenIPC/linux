/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * HiSilicon JPEG encoder (JPGE) V4L2 mem2mem driver
 */
#ifndef HISI_JPGE_H
#define HISI_JPGE_H

#include <linux/bits.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/reset.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>

#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-mem2mem.h>

/*
 * The hardware writes a 64 byte packet descriptor followed by the scan data
 * at a 64 byte aligned stream address. The driver puts that address 64 bytes
 * below the end of the JFIF header it is going to write, so the scan data
 * starts right behind the header with no copy (the header is written after
 * the job and overwrites the descriptor). The header is padded to a multiple
 * of 64 bytes; HISI_JPGE_HDR_MAX is its upper bound.
 */
#define HISI_JPGE_PKT_HDR_SIZE		64
#define HISI_JPGE_HDR_ALIGN		64
#define HISI_JPGE_HDR_MAX		768
/* The encoder looks at the two pointer words 16 and 32 bytes below the stream */
#define HISI_JPGE_PRE_STREAM		32

#define HISI_JPGE_QUALITY_DEFAULT	50
#define HISI_JPGE_RESTART_MAX		65535	/* the DRI marker holds 16 bits */

/* Optional JFIF markers the driver can emit (V4L2_CID_JPEG_ACTIVE_MARKER) */
#define HISI_JPGE_MARKERS_SUPPORTED	(V4L2_JPEG_ACTIVE_MARKER_APP0 | \
					 V4L2_JPEG_ACTIVE_MARKER_COM | \
					 V4L2_JPEG_ACTIVE_MARKER_DHT)
#define HISI_JPGE_MARKERS_DEFAULT	(V4L2_JPEG_ACTIVE_MARKER_APP0 | \
					 V4L2_JPEG_ACTIVE_MARKER_DHT)

/*
 * Register offsets and field masks of one register layout (the names are the
 * ones of hisi-jpge-regs.h without the layout prefix). All masks are applied
 * with field_prep().
 */
struct hisi_jpge_regs {
	u16 intstat, intmask, rawint, intclr;
	u16 picfg, ecscfg, start, imgsize, sstride;
	u16 srcy, srcc, srcv;
	u16 strmaddr, srptr, swptr, strmbuflen;
	u16 outstd, version, qpt, vpp_cfg, vpp_osd_cfg;
	u32 int_eop, int_buf_full, int_cfg_err;
	u32 picfg_strfmt, picfg_yuvfmt, picfg_clkgate, picfg_memclkgate;
	u32 picfg_rounden, picfg_packagesel, picfg_col2gray;
	u32 imgsize_w, imgsize_h;
	u32 ecs_split, ecs_size;
	u32 sstride_y, sstride_c;
	u32 strmbuflen_len;
	u32 vpp_osd_en, vpp_osd_regions;
	u8 strfmt_semiplanar, strfmt_planar, strfmt_packed;
	u8 yuv420, yuv422, yuv444;
};

/** enum hisi_jpge_kind - how the input picture is stored in memory */
enum hisi_jpge_kind {
	HISI_JPGE_SEMIPLANAR,	/* Y plane + interleaved Cr/Cb plane */
	HISI_JPGE_PLANAR,	/* Y, Cb, Cr planes */
	HISI_JPGE_PACKED,	/* interleaved Y/Cb/Cr (4:2:2 only) */
	HISI_JPGE_GREY,		/* Y plane only, JPEG chroma forced neutral */
};

/**
 * struct hisi_jpge_fmt - one input pixel format
 * @fourcc: V4L2 pixel format
 * @kind: storage layout
 * @sampling: chroma sampling of the data, 420, 422 or 444
 * @mem_planes: V4L2 memory planes (1: all components in one buffer)
 * @swap_uv: planar only, the Cr plane precedes the Cb plane in memory
 * @packsel: packed only, PICFG packageSel byte-position code
 */
struct hisi_jpge_fmt {
	u32 fourcc;
	u8 kind;
	u16 sampling;
	u8 mem_planes;
	bool swap_uv;
	u8 packsel;
};

/**
 * struct hisi_jpge_variant - per-SoC data
 * @name: card name
 * @version: expected value of the version register
 * @regs: register layout
 * @fmts: supported input formats, the first one is the default
 * @num_fmts: number of entries in @fmts
 * @min_width: smallest picture width in pixels
 * @max_width: largest picture width
 * @min_height: smallest picture height
 * @max_height: largest picture height
 * @step_width: width granularity
 * @step_height: height granularity
 * @stride_align: alignment of the line strides in bytes
 * @picfg_clkgate: clock gating value written to PICFG (vendor SDK value)
 * @outstd: AXI outstanding request count written to OUTSTD (vendor SDK value)
 * @package_sel: PICFG packageSel value for non-packed input (reset value)
 * @decimate: 4:2:2 to 4:2:0 output by doubling the chroma stride is assumed
 * @restart: V4L2_CID_JPEG_RESTART_INTERVAL is offered (restart intervals were
 *	checked on this SoC)
 */
struct hisi_jpge_variant {
	const char *name;
	u32 version;
	const struct hisi_jpge_regs *regs;
	const struct hisi_jpge_fmt *fmts;
	unsigned int num_fmts;
	u32 min_width, max_width, min_height, max_height;
	u32 step_width, step_height;
	u32 stride_align;
	u32 picfg_clkgate;
	u32 outstd;
	u32 package_sel;
	bool decimate;
	bool restart;
};

struct hisi_jpge_dev {
	struct device *dev;
	const struct hisi_jpge_variant *var;
	struct v4l2_device v4l2_dev;
	struct video_device vfd;
	struct v4l2_m2m_dev *m2m_dev;
	void __iomem *base;
	struct clk *clk;
	struct reset_control *rst;
	int irq;

	struct mutex lock;		/* video device / queue lock */
	/*
	 * The interrupt or the watchdog finishes a running job, never both:
	 * whoever clears @curr under @hw_lock owns the job. The interrupt hands
	 * its job and its cause to the thread through @done, so the thread
	 * never looks at a later job.
	 */
	spinlock_t hw_lock;		/* protects @curr, @done and @done_status */
	struct hisi_jpge_ctx *curr;	/* context owning the running job, not yet claimed */
	struct hisi_jpge_ctx *done;	/* job claimed by the interrupt, finished by the thread */
	u32 done_status;
	struct delayed_work watchdog;
};

/** struct hisi_jpge_params - encoding parameters of one job (control snapshot) */
struct hisi_jpge_params {
	unsigned int quality;
	unsigned int sampling;	/* 420, 422 or 444 of the JPEG */
	u32 markers;		/* V4L2_JPEG_ACTIVE_MARKER_* */
	unsigned int restart;	/* MCUs per restart interval, 0 = none */
};

struct hisi_jpge_ctx {
	struct hisi_jpge_dev *dev;
	struct v4l2_fh fh;
	struct v4l2_ctrl_handler hdl;
	struct v4l2_ctrl *ctrl_sampling;
	int sampling_native;	/* native sampling of the last input format, -1 = none yet */
	const struct hisi_jpge_fmt *fmt;
	struct v4l2_pix_format_mplane src_fmt;
	struct v4l2_pix_format_mplane dst_fmt;
	u32 dst_req_size;	/* capture sizeimage asked for above the default, 0 = none */
	u32 sequence;		/* CAPTURE buffers */
	u32 sequence_out;	/* OUTPUT buffers */

	spinlock_t ctrl_lock;		/* protects @params against s_ctrl */
	struct hisi_jpge_params params;	/* written by s_ctrl */

	/* Tables, header size and parameters of the running job */
	struct hisi_jpge_params job;
	unsigned int job_hdr_len;
	unsigned int job_restart;	/* interval the job uses, 0 = one segment */
	u8 quant_luma[64];
	u8 quant_chroma[64];
};

/* hisi-jpge-hdr.c */

/** struct hisi_jpge_hdr - everything the JFIF header depends on */
struct hisi_jpge_hdr {
	unsigned int width, height;
	unsigned int sampling;		/* 420, 422 or 444 */
	u32 markers;			/* V4L2_JPEG_ACTIVE_MARKER_* */
	unsigned int restart;		/* MCUs per restart interval, 0 = none */
	const u8 *luma, *chroma;	/* quantization tables, natural order */
};

void hisi_jpge_scale_quant(unsigned int quality, u8 *luma, u8 *chroma);
void hisi_jpge_qpt_words(const u8 *quant, u32 *words);
unsigned int hisi_jpge_header_len(u32 markers, unsigned int restart);
unsigned int hisi_jpge_build_header(u8 *buf, const struct hisi_jpge_hdr *hdr);

#endif /* HISI_JPGE_H */
