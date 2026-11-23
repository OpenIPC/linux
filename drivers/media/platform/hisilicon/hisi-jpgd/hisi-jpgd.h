/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * HiSilicon JPEG decoder (JPGD) V4L2 mem2mem driver
 */
#ifndef HISI_JPGD_H
#define HISI_JPGD_H

#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/reset.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>

#include <media/v4l2-device.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-v4l2.h>

#include "hisi-jpgd-regs.h"

#define HISI_JPGD_NAME		"hisi-jpgd"

/* Smallest and largest JPEG file the OUTPUT queue accepts */
#define HISI_JPGD_MIN_STREAM	4096
#define HISI_JPGD_MAX_STREAM	(16 * 1024 * 1024)
#define HISI_JPGD_DEF_STREAM	(512 * 1024)
/*
 * The hardware may read past the end of the data (the vendor code keeps 100
 * bytes of margin): the driver requires this much zeroed room behind the
 * JPEG data inside the OUTPUT buffer.
 */
#define HISI_JPGD_SLACK		128
#define HISI_JPGD_STRIDE_ALIGN	64
#define HISI_JPGD_ROW_ALIGN	16
#define HISI_JPGD_SRC_ALIGN	64
#define HISI_JPGD_DST_ALIGN	128
#define HISI_JPGD_MAX_SCALE	3
/* The 4:2:0 conversion of the block is not used with the 1/8 reduction */
#define HISI_JPGD_MAX_SCALE_CVT	2
/*
 * Watchdog: this base plus 1 ms per 8192 pixels of the coded picture. The
 * slower SoC decodes 4:4:4 at about 30 Mpixel/s (4608 x 3456 in 519 ms), so the
 * largest picture (8096 x 8096, about 2.2 s) stays inside the 11 s given to it.
 * There is no job_abort, so a hung job makes close(), STREAMOFF and an unbind
 * wait for the watchdog: 3 s, and up to 11 s for the largest picture.
 */
#define HISI_JPGD_TIMEOUT_MS	3000
#define HISI_JPGD_TIMEOUT_PIXELS_PER_MS	8192
#define HISI_JPGD_AUTOSUSPEND_MS	500

/** enum hisi_jpgd_sampling - chroma sampling of a baseline JPEG */
enum hisi_jpgd_sampling {
	HISI_JPGD_S400,		/* one component */
	HISI_JPGD_S420,		/* Y 2x2, chroma 1x1 */
	HISI_JPGD_S422H,	/* Y 2x1: chroma halved horizontally */
	HISI_JPGD_S444,
};

/** struct hisi_jpgd_info - what the header parser learned about one JPEG */
struct hisi_jpgd_info {
	unsigned int width, height;	/* coded size */
	enum hisi_jpgd_sampling sampling;
	unsigned int restart;		/* MCUs per restart interval, 0 = none */
	unsigned int ecs_offset;	/* first entropy-coded byte */
	u32 picsize;			/* PICSIZE register value */
	u32 sampling_reg;		/* SAMPLINGFACTOR register value */
};

/** struct hisi_jpgd_tables - table memory contents converted from DQT/DHT */
struct hisi_jpgd_tables {
	u32 quant[JPGD_QUANT_WORDS];
	u32 hdc[JPGD_HDC_WORDS];
	u32 hacmin[JPGD_HAC_WORDS];
	u32 hacbase[JPGD_HAC_WORDS];
	u32 hacsym[JPGD_HACSYM_WORDS];
};

/**
 * struct hisi_jpgd_variant - per-SoC data
 * @name: card name
 * @min_dim: smallest accepted width and height
 * @max_dim: largest accepted width and height of the JPEG
 * @picsize_bad: PICSIZE bits that must stay clear
 * @scale_keep: bits of the SCALE register the driver keeps at the value read
 *	back (bits with an unknown meaning; every other bit is written from
 *	@scale_set and the scale field)
 * @scale_set: SCALE bits written besides the scale field (AXI outstanding
 *	request count and enable bits)
 * @idle_timeout: value of the TIMEOUT register after reset, checked at probe
 *	(a 24 bit all-ones value on the Hi3520D, 32 bit all-ones on the
 *	Hi3536D: the driver never writes the register, so what is read back
 *	is the reset value of that SoC)
 * @idle_scale: value of the SCALE register after reset, checked at probe
 *	(the variants that keep part of it use the value they keep)
 * @cvt420: the block converts 4:2:2 and 4:4:4 pictures to 4:2:0 semi-planar
 *	on output (OUTTYPE bit 2), so NV21 can be chosen for such files
 */
struct hisi_jpgd_variant {
	const char *name;
	unsigned int min_dim;
	unsigned int max_dim;
	u32 picsize_bad;
	u32 scale_keep;
	u32 scale_set;
	u32 idle_timeout;
	u32 idle_scale;
	bool cvt420;
};

struct hisi_jpgd_dev {
	struct device *dev;
	const struct hisi_jpgd_variant *var;
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
	struct hisi_jpgd_ctx *curr;	/* context owning the running job, not yet claimed */
	struct hisi_jpgd_ctx *done;	/* job claimed by the interrupt, finished by the thread */
	u32 done_status;
	struct delayed_work watchdog;
};

/** struct hisi_jpgd_layout - memory layout of one decoded picture */
struct hisi_jpgd_layout {
	u32 fourcc;
	unsigned int scale;		/* 0..3: size = ceil(coded size / 2^scale) */
	unsigned int width, height;	/* decoded size */
	unsigned int ystride, uvstride;	/* bytes */
	unsigned int rows;		/* luma rows allocated (height aligned) */
	unsigned int sizeimage;		/* bytes to allocate */
	unsigned int payload;		/* bytes the hardware fills */
};

struct hisi_jpgd_buf {
	struct v4l2_m2m_buffer m2m;
	bool valid;			/* OUTPUT buffers: info and tbl hold a decodable file */
	struct hisi_jpgd_info info;	/* OUTPUT buffers only */
	struct hisi_jpgd_tables tbl;	/* OUTPUT buffers only */
};

struct hisi_jpgd_ctx {
	struct hisi_jpgd_dev *dev;
	struct v4l2_fh fh;
	struct v4l2_pix_format_mplane src_fmt;
	struct v4l2_pix_format_mplane dst_fmt;
	struct hisi_jpgd_layout lay;	/* layout @dst_fmt describes */
	struct hisi_jpgd_info info;	/* newest JPEG queued */
	bool info_valid;
	bool fmt_set;		/* user space chose the CAPTURE format and size */
	u32 sequence;		/* CAPTURE buffers */
	u32 sequence_out;	/* OUTPUT buffers */
};

/* hisi-jpgd-tables.c */
int hisi_jpgd_parse(const struct hisi_jpgd_variant *var, void *buf, size_t len,
		    struct hisi_jpgd_info *info, struct hisi_jpgd_tables *tbl);

#endif /* HISI_JPGD_H */
