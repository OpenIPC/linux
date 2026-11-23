// SPDX-License-Identifier: GPL-2.0-only
/*
 * HiSilicon JPEG encoder (JPGE) V4L2 mem2mem driver
 *
 * One core for every SoC that has the block: the per-SoC differences (register
 * layout, picture size limits, input formats) live in struct hisi_jpge_variant.
 * See Documentation/devicetree/bindings/media/hisilicon,hi3520dv100-jpge.yaml and
 * hisi-jpge-regs.h for what is and is not known about the hardware.
 *
 * Data flow: the OUTPUT queue takes a YCbCr frame (semi-planar, planar or
 * packed, single buffer or one buffer per plane), the CAPTURE queue returns a
 * complete JFIF file. The hardware emits only the entropy-coded scan data
 * (fixed standard Huffman tables) behind a 64 byte packet descriptor; the
 * driver writes the JFIF header in front of it after the job (see
 * HISI_JPGE_PKT_HDR_SIZE) and programs the quantization tables from the JPEG
 * compression quality control.
 *
 * The hardware has no scaler, cropper or chroma resampler. The one way to
 * change the chroma subsampling is to read every second chroma line (doubled
 * chroma stride) of 4:2:2 input, which gives a 4:2:0 JPEG.
 */

#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/overflow.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>
#include <linux/slab.h>

#include <media/v4l2-common.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-dma-contig.h>

#include "hisi-jpge.h"
#include "hisi-jpge-regs.h"

#define HISI_JPGE_NAME			"hisi-jpge"
/*
 * Watchdog: this base plus 1 ms per 16384 pixels. The slower SoC encodes about
 * 36 Mpixel/s (1080p in 57 ms), so even an 8192 x 8192 picture, 1.9 s, stays
 * well inside the 6 s given to it.
 */
#define HISI_JPGE_TIMEOUT_MS		2000
#define HISI_JPGE_TIMEOUT_PIXELS_PER_MS	16384
#define HISI_JPGE_AUTOSUSPEND_MS	100
#define HISI_JPGE_DEF_WIDTH		640
#define HISI_JPGE_DEF_HEIGHT		480

/* Stream and source buffer address alignment the hardware needs */
#define HISI_JPGE_SRC_ALIGN		16
#define HISI_JPGE_DST_ALIGN		64

/* Packet descriptor the hardware writes at the stream address */
struct hisi_jpge_pkt_hdr {
	__le32 packet_len;	/* multiple of 64, includes this descriptor */
	__le32 invalid_bytes;	/* padding at the end of the packet */
	u8 reserved[3];
	u8 last;		/* 1 in the last packet of the picture */
	__le32 chn_id;
	u8 reserved2[HISI_JPGE_PKT_HDR_SIZE - 16];
};

/* ---------------------------------------------------------------- variants */

static const struct hisi_jpge_regs hisi_jpge_r004_regs = {
	.intstat = JPGE_R004_INTSTAT,
	.intmask = JPGE_R004_INTMASK,
	.rawint = JPGE_R004_RAWINT,
	.intclr = JPGE_R004_INTCLR,
	.picfg = JPGE_R004_PICFG,
	.ecscfg = JPGE_R004_ECSCFG,
	.start = JPGE_R004_START,
	.imgsize = JPGE_R004_IMGSIZE,
	.sstride = JPGE_R004_SSTRIDE,
	.srcy = JPGE_R004_SRCYADDR,
	.srcc = JPGE_R004_SRCCADDR,
	.srcv = JPGE_R004_SRCVADDR,
	.strmaddr = JPGE_R004_STRMADDR,
	.srptr = JPGE_R004_SRPTRADDR,
	.swptr = JPGE_R004_SWPTRADDR,
	.strmbuflen = JPGE_R004_STRMBUFLEN,
	.outstd = JPGE_R004_OUTSTD,
	.vpp_cfg = JPGE_R004_VPP_CFG,
	.vpp_osd_cfg = JPGE_R004_VPP_OSD_CFG,
	.version = JPGE_R004_VERSION,
	.qpt = JPGE_R004_QPT,
	.int_eop = JPGE_R004_INT_EOP,
	.int_buf_full = JPGE_R004_INT_BUF_FULL,
	.int_cfg_err = JPGE_R004_INT_CFG_ERR,
	.picfg_strfmt = JPGE_R004_PICFG_STRFMT,
	.picfg_yuvfmt = JPGE_R004_PICFG_YUVFMT,
	.picfg_clkgate = JPGE_R004_PICFG_CLKGATE,
	.picfg_memclkgate = JPGE_R004_PICFG_MEMCLKGATE,
	.picfg_rounden = JPGE_R004_PICFG_ROUNDEN,
	.picfg_col2gray = JPGE_R004_PICFG_COL2GRAY,
	.picfg_packagesel = JPGE_R004_PICFG_PACKAGESEL,
	.imgsize_w = JPGE_R004_IMGSIZE_W,
	.imgsize_h = JPGE_R004_IMGSIZE_H,
	.ecs_split = JPGE_R004_ECSCFG_SPLIT,
	.ecs_size = JPGE_R004_ECSCFG_SIZE,
	.sstride_y = JPGE_R004_SSTRIDE_Y,
	.sstride_c = JPGE_R004_SSTRIDE_C,
	.strmbuflen_len = JPGE_R004_STRMBUFLEN_LEN,
	.vpp_osd_en = JPGE_R004_VPP_CFG_OSDEN,
	.vpp_osd_regions = JPGE_R004_VPP_OSD_CFG_REGIONS,
	.strfmt_semiplanar = JPGE_R004_STRFMT_SEMIPLANAR,
	.strfmt_planar = JPGE_R004_STRFMT_PLANAR,
	.strfmt_packed = JPGE_R004_STRFMT_PACKED,
	.yuv420 = JPGE_R004_YUV420,
	.yuv422 = JPGE_R004_YUV422,
	.yuv444 = JPGE_R004_YUV444,
};

/*
 * Hi3536D V100: the data sheet lists semi-planar 4:2:0 and 4:2:2 input. Semi-
 * planar input only has the Cr-first order (NV21/NV61): the board test of
 * 2026-11-10 showed correct colors with it, and nothing in the registers or
 * the vendor code allows the other order for semi-planar data (the vendor
 * packageSel value is only documented for packed data), so NV12/NV16 are left
 * out rather than faked.
 *
 * 8 bit grey input is offered on top. The block has no one-component mode (the
 * vendor stack does not either: its grey output is the colour-to-grey bit with
 * a three component header), so a grey picture becomes a 4:2:0 JPEG whose
 * chroma is the neutral value; the chroma registers point into the Y plane,
 * which is harmless because the block only has to read valid memory there.
 */
static const struct hisi_jpge_fmt hisi_jpge_hi3536dv100_fmts[] = {
	{ V4L2_PIX_FMT_NV21, HISI_JPGE_SEMIPLANAR, 420, 1 },
	{ V4L2_PIX_FMT_NV61, HISI_JPGE_SEMIPLANAR, 422, 1 },
	{ V4L2_PIX_FMT_NV21M, HISI_JPGE_SEMIPLANAR, 420, 2 },
	{ V4L2_PIX_FMT_NV61M, HISI_JPGE_SEMIPLANAR, 422, 2 },
	{ V4L2_PIX_FMT_GREY, HISI_JPGE_GREY, 420, 1 },
};

/*
 * Hi3520D V100 (Hi3515A/C share it). The data sheet (6.3.2) lists planar
 * 4:2:0, 4:2:2 and 4:4:4, semi-planar 4:2:0 and 4:2:2 and packed YUYV input;
 * the other packed orders follow from the packageSel field (see regs.h), 4:4:4
 * semi-planar and 4:0:0 are not listed and not offered. Separate-plane (M)
 * variants map to the independent Y/Cb/Cr source addresses.
 */
static const struct hisi_jpge_fmt hisi_jpge_hi3520dv100_fmts[] = {
	{ V4L2_PIX_FMT_NV21, HISI_JPGE_SEMIPLANAR, 420, 1 },
	{ V4L2_PIX_FMT_NV61, HISI_JPGE_SEMIPLANAR, 422, 1 },
	{ V4L2_PIX_FMT_NV21M, HISI_JPGE_SEMIPLANAR, 420, 2 },
	{ V4L2_PIX_FMT_NV61M, HISI_JPGE_SEMIPLANAR, 422, 2 },
	{ V4L2_PIX_FMT_YUV420, HISI_JPGE_PLANAR, 420, 1 },
	{ V4L2_PIX_FMT_YVU420, HISI_JPGE_PLANAR, 420, 1, true },
	{ V4L2_PIX_FMT_YUV420M, HISI_JPGE_PLANAR, 420, 3 },
	{ V4L2_PIX_FMT_YVU420M, HISI_JPGE_PLANAR, 420, 3, true },
	{ V4L2_PIX_FMT_YUV422P, HISI_JPGE_PLANAR, 422, 1 },
	{ V4L2_PIX_FMT_YUV422M, HISI_JPGE_PLANAR, 422, 3 },
	{ V4L2_PIX_FMT_YVU422M, HISI_JPGE_PLANAR, 422, 3, true },
	{ V4L2_PIX_FMT_YUV444M, HISI_JPGE_PLANAR, 444, 3 },
	{ V4L2_PIX_FMT_YVU444M, HISI_JPGE_PLANAR, 444, 3, true },
	{ V4L2_PIX_FMT_YUYV, HISI_JPGE_PACKED, 422, 1, false,
	  JPGE_R004_PKG_YUYV },
	{ V4L2_PIX_FMT_UYVY, HISI_JPGE_PACKED, 422, 1, false,
	  JPGE_R004_PKG_UYVY },
	{ V4L2_PIX_FMT_YVYU, HISI_JPGE_PACKED, 422, 1, false,
	  JPGE_R004_PKG_YVYU },
	{ V4L2_PIX_FMT_VYUY, HISI_JPGE_PACKED, 422, 1, false,
	  JPGE_R004_PKG_VYUY },
};

/*
 * Hi3520D V100: data sheet 6.3 gives 64x64 to 8192x8192 in steps of 4. The
 * PICFG/OUTSTD values are what the HiSTB SDK driver writes. Tested on
 * hardware. Restart intervals work with every input format (checked on
 * silicon); there is no grey format: with the color-to-gray bit set the block
 * does not neutralize the chroma of the JPEG on this SoC (the chroma source
 * registers of the grey input point into the Y plane, so the chroma of the
 * file is garbage).
 */
static const struct hisi_jpge_variant hisi_jpge_hi3520dv100 = {
	.name = "Hi3520D V100 JPGE",
	.version = JPGE_R004_VERSION_VALUE,
	.regs = &hisi_jpge_r004_regs,
	.fmts = hisi_jpge_hi3520dv100_fmts,
	.num_fmts = ARRAY_SIZE(hisi_jpge_hi3520dv100_fmts),
	.min_width = 64,
	.max_width = 8192,
	.min_height = 64,
	.max_height = 8192,
	.step_width = 4,
	.step_height = 4,
	.stride_align = 16,
	.picfg_clkgate = 2,
	.outstd = 2,
	.package_sel = JPGE_R004_PKG_YUYV,
	.decimate = true,
	.restart = true,
};

/*
 * Hi3536D V100: data sheet 6.2 gives semi-planar 4:2:0 and 4:2:2 input and
 * 32x32 to 8192x8192 in steps of 4. The register layout is the r004 one
 * (INTSTAT, INTMASK, INTCLR, PICFG, ECSCFG, START, IMGSIZE, source addresses,
 * OUTSTD, the stream registers and the QPT window); it was read and exercised
 * on an Hi3536D V100 NVR board, whose VERSION word is the r004 one. A
 * different VERSION still logs a warning at probe (a board that was not
 * seen). The vendor module writes OUTSTD low nibble 1. The 4:2:2 to 4:2:0
 * stride trick is not enabled: it was not measured on this SoC.
 */
static const struct hisi_jpge_variant hisi_jpge_hi3536dv100 = {
	.name = "Hi3536D V100 JPGE",
	.version = JPGE_R004_VERSION_VALUE,
	.regs = &hisi_jpge_r004_regs,
	.fmts = hisi_jpge_hi3536dv100_fmts,
	.num_fmts = ARRAY_SIZE(hisi_jpge_hi3536dv100_fmts),
	.min_width = 32,
	.max_width = 8192,
	.min_height = 32,
	.max_height = 8192,
	.step_width = 4,
	.step_height = 4,
	.stride_align = 16,
	.picfg_clkgate = 2,
	.outstd = 1,
	.package_sel = JPGE_R004_PKG_YUYV,
	.restart = true,
};

/* ------------------------------------------------------------------ helpers */

static inline u32 jpge_read(struct hisi_jpge_dev *jpge, u16 off)
{
	return readl(jpge->base + off);
}

static inline void jpge_write(struct hisi_jpge_dev *jpge, u16 off, u32 val)
{
	writel(val, jpge->base + off);
}

static inline struct hisi_jpge_ctx *file_to_ctx(struct file *file)
{
	return container_of(file_to_v4l2_fh(file), struct hisi_jpge_ctx, fh);
}

static const struct hisi_jpge_fmt *
hisi_jpge_find_fmt(const struct hisi_jpge_variant *var, u32 fourcc)
{
	unsigned int i;

	for (i = 0; i < var->num_fmts; i++)
		if (var->fmts[i].fourcc == fourcc)
			return &var->fmts[i];

	return NULL;
}

static u32 hisi_jpge_yuvfmt(const struct hisi_jpge_regs *regs,
			    unsigned int sampling)
{
	switch (sampling) {
	case 422:
		return regs->yuv422;
	case 444:
		return regs->yuv444;
	default:
		return regs->yuv420;
	}
}

/* V4L2_CID_JPEG_CHROMA_SUBSAMPLING menu index <-> sampling */
static unsigned int hisi_jpge_sampling_to_menu(unsigned int sampling)
{
	switch (sampling) {
	case 444:
		return V4L2_JPEG_CHROMA_SUBSAMPLING_444;
	case 422:
		return V4L2_JPEG_CHROMA_SUBSAMPLING_422;
	default:
		return V4L2_JPEG_CHROMA_SUBSAMPLING_420;
	}
}

static unsigned int hisi_jpge_menu_to_sampling(unsigned int menu)
{
	switch (menu) {
	case V4L2_JPEG_CHROMA_SUBSAMPLING_444:
		return 444;
	case V4L2_JPEG_CHROMA_SUBSAMPLING_422:
		return 422;
	default:
		return 420;
	}
}

/*
 * JPEG chroma subsamplings that can be produced from a given input format:
 * the native one, and for non-packed 4:2:2 also 4:2:0 (every second chroma
 * line is skipped by doubling the chroma stride, which the vendor SDK uses
 * the same way). Packed data has no separate chroma stride.
 */
static u64 hisi_jpge_sampling_skip_mask(const struct hisi_jpge_variant *var,
					const struct hisi_jpge_fmt *fmt)
{
	u64 allowed = BIT(hisi_jpge_sampling_to_menu(fmt->sampling));

	if (var->decimate && fmt->sampling == 422 &&
	    fmt->kind != HISI_JPGE_PACKED)
		allowed |= BIT(V4L2_JPEG_CHROMA_SUBSAMPLING_420);

	return ~allowed & GENMASK(V4L2_JPEG_CHROMA_SUBSAMPLING_420, 0);
}

/* ------------------------------------------------------------------ layout */

/**
 * struct hisi_jpge_layout - memory layout of one input picture
 * @ys: luma (or packed) line stride in bytes
 * @cs: chroma line stride in bytes (one value for Cb and Cr)
 * @rows_c: chroma lines per plane
 * @size: size of the Y, first and second chroma component in bytes
 * @plane: V4L2 memory plane of each component
 * @off: byte offset of each component inside its memory plane
 * @plane_size: minimum size of each memory plane, with the slack described in
 *	hisi_jpge_calc_layout()
 */
struct hisi_jpge_layout {
	u32 ys, cs, rows_c;
	u32 size[3];
	u8 plane[3];
	u32 off[3];
	u32 plane_size[3];
};

/*
 * Largest line stride the driver accepts: twice the widest picture (packed
 * 4:2:2 needs two bytes per pixel), rounded for the single-buffer planar
 * layouts that align the luma stride to twice the stride alignment. It also
 * keeps the 16 bit SSTRIDE fields (the 4:2:2 to 4:2:0 trick doubles the
 * chroma stride) and every size computed below far from overflowing.
 */
static u32 hisi_jpge_max_stride(const struct hisi_jpge_variant *var)
{
	return ALIGN(2 * var->max_width, 2 * var->stride_align);
}

/*
 * Offsets inside a single buffer follow the usual V4L2 convention (the chroma
 * plane starts right behind bytesperline * height of luma), so applications
 * can fill the buffer with the plain stride/height arithmetic. The hardware
 * reads whole MCUs, up to 15 lines below the picture; the plane sizes include
 * that slack so those reads never leave the buffer (the luma slack is covered
 * by the chroma plane that follows it in single-buffer formats).
 *
 * The strides come from user space: they are clamped to
 * hisi_jpge_max_stride() and every product and sum is checked, so a size
 * that cannot be represented is an error instead of a wrapped small buffer
 * that the hardware would read far beyond.
 */
static int hisi_jpge_calc_layout(const struct hisi_jpge_variant *var,
				 const struct hisi_jpge_fmt *fmt, u32 w,
				 u32 h, u32 user_ys, u32 user_cs,
				 struct hisi_jpge_layout *l)
{
	u32 ya = var->stride_align, ymin = w, cmin = 0, alloc_h = ALIGN(h, 16);
	u32 max_stride = hisi_jpge_max_stride(var);
	u32 alloc_rows_c, pad_y, pad_c, total;
	unsigned int ncomp, i;
	bool bad;

	memset(l, 0, sizeof(*l));

	user_ys = min(user_ys, max_stride);
	user_cs = min(user_cs, max_stride);

	/* A single-buffer planar format has chroma stride = luma stride / 2 */
	if (fmt->kind == HISI_JPGE_PLANAR && fmt->mem_planes == 1)
		ya *= 2;
	/*
	 * A packed row is read in whole 16 pixel MCUs of 32 bytes, so the
	 * minimum stride covers the padded width, not 2 * w.
	 */
	if (fmt->kind == HISI_JPGE_PACKED)
		ymin = 2 * ALIGN(w, 16);
	l->ys = ALIGN(max(user_ys, ymin), ya);

	l->rows_c = fmt->sampling == 420 ? DIV_ROUND_UP(h, 2) : h;
	alloc_rows_c = fmt->sampling == 420 ? alloc_h / 2 : alloc_h;

	switch (fmt->kind) {
	case HISI_JPGE_PACKED:
	case HISI_JPGE_GREY:
		ncomp = 1;
		break;
	case HISI_JPGE_SEMIPLANAR:
		ncomp = 2;
		cmin = w;
		l->cs = fmt->mem_planes == 1 ?
			l->ys : ALIGN(max(user_cs, cmin), var->stride_align);
		break;
	default:
		ncomp = 3;
		cmin = fmt->sampling == 444 ? w : w / 2;
		l->cs = fmt->mem_planes == 1 ?
			l->ys / 2 : ALIGN(max(user_cs, cmin), var->stride_align);
		break;
	}

	/* The strides have to fit the 16 bit register fields in any case */
	if (l->ys > max_stride || l->cs > max_stride)
		return -EINVAL;

	bad = check_mul_overflow(l->ys, h, &l->size[0]);
	bad |= check_mul_overflow(l->cs, l->rows_c, &l->size[1]);
	bad |= check_mul_overflow(l->ys, alloc_h - h, &pad_y);
	bad |= check_mul_overflow(l->cs, alloc_rows_c - l->rows_c, &pad_c);
	if (bad)
		return -EINVAL;
	l->size[2] = l->size[1];

	if (fmt->mem_planes == 1) {
		/* every component in plane 0, back to back */
		total = l->size[0];
		for (i = 1; i < ncomp; i++) {
			l->off[i] = total;
			bad |= check_add_overflow(total, l->size[i], &total);
		}
		bad |= check_add_overflow(total, ncomp == 1 ? pad_y : pad_c,
					  &l->plane_size[0]);
	} else {
		for (i = 0; i < ncomp; i++) {
			l->plane[i] = i;
			bad |= check_add_overflow(l->size[i], i ? pad_c : pad_y,
						  &l->plane_size[i]);
		}
	}

	return bad ? -EINVAL : 0;
}

/* Layout of the picture in the current output format */
static int hisi_jpge_src_layout(const struct hisi_jpge_ctx *ctx,
				struct hisi_jpge_layout *l)
{
	const struct v4l2_pix_format_mplane *pix = &ctx->src_fmt;

	return hisi_jpge_calc_layout(ctx->dev->var, ctx->fmt, pix->width,
				     pix->height,
				     pix->plane_fmt[0].bytesperline,
				     pix->plane_fmt[1].bytesperline, l);
}

/*
 * Default stream buffer size: header, descriptor, two bytes per pixel and EOI.
 * That is plenty for natural pictures but not a bound: noise at a high
 * quality can need more with the fixed Huffman tables, and then the hardware
 * stops with "stream buffer full" (the application can ask for a larger
 * capture buffer).
 */
static u32 hisi_jpge_stream_size(u32 width, u32 height)
{
	return ALIGN(HISI_JPGE_HDR_MAX + 2 * width * height + 2, 256);
}

/* --------------------------------------------------------------- hardware */

static void hisi_jpge_hw_reset(struct hisi_jpge_dev *jpge)
{
	reset_control_assert(jpge->rst);
	usleep_range(10, 20);
	reset_control_deassert(jpge->rst);
	usleep_range(10, 20);
}

/*
 * Program one job. @l is the layout of the source picture, @addr holds the
 * DMA address of each memory plane of the source, @dst the capture buffer,
 * @dst_size its size.
 */
static void hisi_jpge_hw_start(struct hisi_jpge_dev *jpge,
			       struct hisi_jpge_ctx *ctx,
			       const struct hisi_jpge_layout *l,
			       const dma_addr_t addr[VIDEO_MAX_PLANES],
			       dma_addr_t dst, u32 dst_size)
{
	const struct hisi_jpge_variant *var = jpge->var;
	const struct hisi_jpge_regs *r = var->regs;
	const struct hisi_jpge_fmt *fmt = ctx->fmt;
	struct v4l2_pix_format_mplane *pix = &ctx->src_fmt;
	dma_addr_t a[3], srcc = 0, srcv = 0;
	u32 strfmt, pkgsel = var->package_sel, cstride;
	u32 stream_len, stream_off = ctx->job_hdr_len - HISI_JPGE_PKT_HDR_SIZE;
	u32 qpt[16], stale;
	unsigned int i;

	for (i = 0; i < 3; i++)
		a[i] = addr[l->plane[i]] + l->off[i];

	cstride = l->cs;
	switch (fmt->kind) {
	case HISI_JPGE_PLANAR:
		strfmt = r->strfmt_planar;
		/* a[1], a[2] are in memory order; the hardware wants Cb, Cr */
		srcc = fmt->swap_uv ? a[2] : a[1];
		srcv = fmt->swap_uv ? a[1] : a[2];
		break;
	case HISI_JPGE_PACKED:
		strfmt = r->strfmt_packed;
		pkgsel = fmt->packsel;
		/* no chroma data, keep the registers pointing at real memory */
		srcc = a[0];
		srcv = a[0];
		cstride = l->ys;
		break;
	case HISI_JPGE_GREY:
		strfmt = r->strfmt_semiplanar;
		/* no chroma data, keep the registers pointing at real memory */
		srcc = a[0];
		srcv = a[0];
		cstride = l->ys;
		break;
	default:
		strfmt = r->strfmt_semiplanar;
		srcc = a[1];
		break;
	}

	/* 4:2:2 data, 4:2:0 JPEG: read every second chroma line */
	if (ctx->job.sampling != fmt->sampling)
		cstride *= 2;

	/*
	 * The overlay (OSD) window is not used: switch every region off
	 * instead of relying on what the block reset (or an earlier owner of
	 * the block) left in it. Read-modify-write, because the window is
	 * only known from the vendor header and may hold other state.
	 */
	jpge_write(jpge, r->vpp_cfg,
		   jpge_read(jpge, r->vpp_cfg) & ~r->vpp_osd_en);
	jpge_write(jpge, r->vpp_osd_cfg,
		   jpge_read(jpge, r->vpp_osd_cfg) & ~r->vpp_osd_regions);

	jpge_write(jpge, r->intmask,
		   r->int_eop | r->int_buf_full | r->int_cfg_err);
	/* Acknowledge a cause from before this job: it would end the job early */
	stale = jpge_read(jpge, r->rawint) &
		(r->int_eop | r->int_buf_full | r->int_cfg_err);
	if (stale) {
		dev_warn(jpge->dev, "stale interrupt cause 0x%x before start\n",
			 stale);
		jpge_write(jpge, r->intclr, stale);
	}
	jpge_write(jpge, r->picfg,
		   field_prep(r->picfg_strfmt, strfmt) |
		   field_prep(r->picfg_yuvfmt,
			      hisi_jpge_yuvfmt(r, ctx->job.sampling)) |
		   field_prep(r->picfg_memclkgate, 1) |
		   field_prep(r->picfg_clkgate, var->picfg_clkgate) |
		   /* round like libjpeg; the reset value, programmed anyway */
		   field_prep(r->picfg_rounden, 1) |
		   field_prep(r->picfg_col2gray, fmt->kind == HISI_JPGE_GREY) |
		   field_prep(r->picfg_packagesel, pkgsel));
	/* restart interval: segments of job_restart MCUs, or one segment */
	jpge_write(jpge, r->ecscfg,
		   ctx->job_restart ?
		   field_prep(r->ecs_split, 1) |
		   field_prep(r->ecs_size, ctx->job_restart - 1) : 0);
	jpge_write(jpge, r->imgsize,
		   field_prep(r->imgsize_w, pix->width - 1) |
		   field_prep(r->imgsize_h, pix->height - 1));

	jpge_write(jpge, r->srcy, a[0]);
	jpge_write(jpge, r->srcc, srcc);
	jpge_write(jpge, r->srcv, srcv);
	jpge_write(jpge, r->sstride,
		   field_prep(r->sstride_y, l->ys) |
		   field_prep(r->sstride_c, cstride));

	/*
	 * Descriptor and scan data start at the stream address. The two
	 * pointer addresses (unused by the encoder, the HiSTB driver points
	 * them 16 and 32 bytes below the stream) lie inside the header area,
	 * which is only written after the job.
	 */
	stream_len = (dst_size - stream_off) & r->strmbuflen_len;
	jpge_write(jpge, r->strmaddr, dst + stream_off);
	jpge_write(jpge, r->srptr, dst + stream_off - 16);
	jpge_write(jpge, r->swptr, dst + stream_off - 32);
	jpge_write(jpge, r->strmbuflen, stream_len);

	/* Three tables: Y, Cb, Cr (the chroma table is used for both) */
	hisi_jpge_qpt_words(ctx->quant_luma, qpt);
	for (i = 0; i < 16; i++)
		jpge_write(jpge, r->qpt + 4 * i, qpt[i]);
	hisi_jpge_qpt_words(ctx->quant_chroma, qpt);
	for (i = 0; i < 16; i++) {
		jpge_write(jpge, r->qpt + 4 * (16 + i), qpt[i]);
		jpge_write(jpge, r->qpt + 4 * (32 + i), qpt[i]);
	}

	jpge_write(jpge, r->outstd, var->outstd);

	jpge_write(jpge, r->start, 0);
	jpge_write(jpge, r->start, 1);
}

/*
 * Restart interval the job uses: the requested one when it is shorter than the
 * picture (the encoder does not split otherwise), else none. An MCU is 16 x 16
 * pixels for 4:2:0, 16 x 8 for 4:2:2 and 8 x 8 for 4:4:4.
 */
static unsigned int hisi_jpge_restart(const struct hisi_jpge_ctx *ctx)
{
	const struct v4l2_pix_format_mplane *pix = &ctx->src_fmt;
	unsigned int mcus;

	mcus = DIV_ROUND_UP(pix->width, ctx->job.sampling == 444 ? 8 : 16) *
	       DIV_ROUND_UP(pix->height, ctx->job.sampling == 420 ? 16 : 8);

	return ctx->job.restart && ctx->job.restart < mcus ?
	       ctx->job.restart : 0;
}

/*
 * The driver reads and writes the CAPTURE buffer through its kernel mapping
 * (descriptors, header, clearing before a job). A buffer that was imported from
 * an exporter with a cached mapping (a dma-buf heap) is only coherent with the
 * block and with the other users between the CPU access calls of the dma-buf:
 * vb2 does no cache maintenance for imported buffers, the exporter does it in
 * dma_buf_begin_cpu_access() and dma_buf_end_cpu_access(). The direction is
 * DMA_BIDIRECTIONAL because the CPU both reads what the block wrote and writes
 * what the user reads. Buffers vb2 allocated itself have no dma-buf here and
 * need nothing: their mapping is the coherent one of the DMA API.
 */
static int hisi_jpge_cpu_begin(struct vb2_buffer *vb)
{
	if (vb->memory != VB2_MEMORY_DMABUF || !vb->planes[0].dbuf)
		return 0;

	return dma_buf_begin_cpu_access(vb->planes[0].dbuf, DMA_BIDIRECTIONAL);
}

static void hisi_jpge_cpu_end(struct vb2_buffer *vb)
{
	if (vb->memory != VB2_MEMORY_DMABUF || !vb->planes[0].dbuf)
		return;

	/* Nothing to report to: the data is in place or the exporter is broken */
	dma_buf_end_cpu_access(vb->planes[0].dbuf, DMA_BIDIRECTIONAL);
}

/*
 * Turn the finished hardware output into a JPEG file: validate the packet
 * descriptors, close the gaps between the packets of a picture with restart
 * intervals (every packet is a descriptor followed by the data of one segment,
 * the first one's data already sits behind the header), write the header in
 * front of the scan data and make sure the file ends with EOI. Returns the
 * file size or 0 on error. The caller holds the CPU access of the buffer.
 */
static u32 hisi_jpge_build_stream(struct hisi_jpge_ctx *ctx,
				  struct vb2_buffer *vb)
{
	struct hisi_jpge_dev *jpge = ctx->dev;
	struct v4l2_pix_format_mplane *pix = &ctx->src_fmt;
	const u32 hdr_len = ctx->job_hdr_len;
	const u32 off = hdr_len - HISI_JPGE_PKT_HDR_SIZE;
	const struct hisi_jpge_pkt_hdr *pkt;
	struct hisi_jpge_hdr hdr = {
		.width = pix->width,
		.height = pix->height,
		.sampling = ctx->job.sampling,
		.markers = ctx->job.markers,
		.restart = ctx->job_restart,
		.luma = ctx->quant_luma,
		.chroma = ctx->quant_chroma,
	};
	u32 pkt_len, invalid, total = hdr_len, pos = off, size;
	bool last;
	u8 *buf;

	buf = vb2_plane_vaddr(vb, 0);
	if (!buf)
		return 0;
	size = vb2_plane_size(vb, 0);
	if (size <= off)
		return 0;

	do {
		/* The header is written last: it overwrites the first descriptor */
		if (pos > size - HISI_JPGE_PKT_HDR_SIZE) {
			dev_err_ratelimited(jpge->dev,
					    "packet descriptor outside the buffer (offset %u)\n",
					    pos);
			return 0;
		}
		pkt = (const void *)(buf + pos);
		pkt_len = le32_to_cpu(pkt->packet_len);
		invalid = le32_to_cpu(pkt->invalid_bytes);
		last = pkt->last == 1;
		/*
		 * The length is a multiple of the descriptor size (it includes
		 * the descriptor) and holds at least two bytes of data after
		 * the padding. The first two tests make the subtraction safe;
		 * the last one keeps the packet inside the buffer.
		 */
		if (pkt_len % HISI_JPGE_PKT_HDR_SIZE ||
		    pkt_len < HISI_JPGE_PKT_HDR_SIZE + 2 ||
		    invalid > pkt_len - HISI_JPGE_PKT_HDR_SIZE - 2 ||
		    pkt_len > size - pos) {
			dev_err_ratelimited(jpge->dev,
					    "bad packet descriptor (len %u, invalid %u): %16ph\n",
					    pkt_len, invalid, pkt);
			return 0;
		}

		pkt_len -= HISI_JPGE_PKT_HDR_SIZE + invalid;
		if (total != pos + HISI_JPGE_PKT_HDR_SIZE)
			memmove(buf + total, buf + pos + HISI_JPGE_PKT_HDR_SIZE,
				pkt_len);
		total += pkt_len;
		pos += pkt_len + invalid + HISI_JPGE_PKT_HDR_SIZE;
	} while (ctx->job_restart && !last);

	/* The descriptor is overwritten by the tail of the header */
	hisi_jpge_build_header(buf, &hdr);

	if (buf[total - 2] != 0xff || buf[total - 1] != 0xd9) {
		if (total + 2 > size)
			return 0;
		buf[total++] = 0xff;
		buf[total++] = 0xd9;
	}

	return total;
}

static u32 hisi_jpge_finish_stream(struct hisi_jpge_ctx *ctx,
				   struct vb2_buffer *vb)
{
	u32 size;

	if (hisi_jpge_cpu_begin(vb)) {
		dev_err_ratelimited(ctx->dev->dev,
				    "cannot get CPU access to the capture buffer\n");
		return 0;
	}
	size = hisi_jpge_build_stream(ctx, vb);
	hisi_jpge_cpu_end(vb);

	return size;
}

static const struct v4l2_event hisi_jpge_eos_event = {
	.type = V4L2_EVENT_EOS
};

/*
 * Number both buffers of a finished job. The job that holds the last OUTPUT
 * buffer queued before an ENCODER_CMD STOP ends the drain: its CAPTURE
 * buffer carries V4L2_BUF_FLAG_LAST and the context counts as stopped.
 */
static void hisi_jpge_finish_bufs(struct hisi_jpge_ctx *ctx,
				  struct vb2_v4l2_buffer *src,
				  struct vb2_v4l2_buffer *dst)
{
	v4l2_m2m_buf_copy_metadata(src, dst);
	src->sequence = ctx->sequence_out++;
	dst->sequence = ctx->sequence++;
	dst->field = V4L2_FIELD_NONE;

	if (v4l2_m2m_is_last_draining_src_buf(ctx->fh.m2m_ctx, src)) {
		dst->flags |= V4L2_BUF_FLAG_LAST;
		v4l2_m2m_mark_stopped(ctx->fh.m2m_ctx);
		v4l2_event_queue_fh(&ctx->fh, &hisi_jpge_eos_event);
	}
}

static void hisi_jpge_job_done(struct hisi_jpge_dev *jpge,
			       struct hisi_jpge_ctx *ctx, u32 status)
{
	const struct hisi_jpge_regs *r = jpge->var->regs;
	enum vb2_buffer_state state = VB2_BUF_STATE_ERROR;
	struct vb2_v4l2_buffer *src, *dst;
	u32 size = 0;

	src = v4l2_m2m_next_src_buf(ctx->fh.m2m_ctx);
	dst = v4l2_m2m_next_dst_buf(ctx->fh.m2m_ctx);

	if (status & r->int_buf_full)
		dev_err_ratelimited(jpge->dev, "stream buffer full\n");
	else if (status & r->int_cfg_err)
		dev_err_ratelimited(jpge->dev, "configuration error\n");
	else if (status & r->int_eop)
		size = hisi_jpge_finish_stream(ctx, &dst->vb2_buf);

	if (size)
		state = VB2_BUF_STATE_DONE;
	else if (status)
		/* the block did not finish cleanly: do not trust its state */
		hisi_jpge_hw_reset(jpge);
	vb2_set_plane_payload(&dst->vb2_buf, 0, size);
	hisi_jpge_finish_bufs(ctx, src, dst);

	pm_runtime_put_autosuspend(jpge->dev);
	v4l2_m2m_buf_done_and_job_finish(jpge->m2m_dev, ctx->fh.m2m_ctx, state);
}

static void hisi_jpge_device_run(void *priv)
{
	struct hisi_jpge_ctx *ctx = priv;
	struct hisi_jpge_dev *jpge = ctx->dev;
	struct vb2_v4l2_buffer *src, *dst;
	dma_addr_t addr[VIDEO_MAX_PLANES] = {};
	struct hisi_jpge_layout l;
	unsigned long flags;
	u32 dst_size;
	u8 *buf;
	int ret, i;

	src = v4l2_m2m_next_src_buf(ctx->fh.m2m_ctx);
	dst = v4l2_m2m_next_dst_buf(ctx->fh.m2m_ctx);

	/* One consistent snapshot of the controls for the whole job */
	spin_lock_irqsave(&ctx->ctrl_lock, flags);
	ctx->job = ctx->params;
	spin_unlock_irqrestore(&ctx->ctrl_lock, flags);
	ctx->job_restart = hisi_jpge_restart(ctx);
	ctx->job_hdr_len = hisi_jpge_header_len(ctx->job.markers,
						ctx->job_restart);

	dst_size = vb2_plane_size(&dst->vb2_buf, 0);
	buf = vb2_plane_vaddr(&dst->vb2_buf, 0);
	if (!buf || dst_size < ctx->job_hdr_len + 2 * HISI_JPGE_PKT_HDR_SIZE) {
		dev_err_ratelimited(jpge->dev,
				    "capture buffer unusable (size %u, %s)\n",
				    dst_size, buf ? "mapped" : "no CPU mapping");
		goto err;
	}

	/* What the hardware will read, against what the buffers provide */
	ret = hisi_jpge_src_layout(ctx, &l);
	for (i = 0; !ret && i < src->vb2_buf.num_planes; i++)
		if (vb2_plane_size(&src->vb2_buf, i) < l.plane_size[i])
			ret = -EINVAL;
	if (ret) {
		dev_err_ratelimited(jpge->dev,
				    "source buffer too small for the picture\n");
		goto err;
	}

	/*
	 * The two pointer words just below the stream hold the previous
	 * picture's JFIF header when a capture buffer is reused (the header
	 * is written over them), and the encoder evidently looks at them:
	 * the second back-to-back picture lost its descriptor and started
	 * early (board test 2026-11-10). Clear them and the descriptor area,
	 * and push the clearing out to memory before the block runs.
	 */
	ret = hisi_jpge_cpu_begin(&dst->vb2_buf);
	if (ret) {
		dev_err_ratelimited(jpge->dev,
				    "cannot get CPU access to the capture buffer\n");
		goto err;
	}
	memset(buf + ctx->job_hdr_len - HISI_JPGE_PKT_HDR_SIZE -
	       HISI_JPGE_PRE_STREAM, 0,
	       HISI_JPGE_PRE_STREAM + HISI_JPGE_PKT_HDR_SIZE);
	hisi_jpge_cpu_end(&dst->vb2_buf);

	ret = pm_runtime_resume_and_get(jpge->dev);
	if (ret < 0)
		goto err;

	hisi_jpge_scale_quant(ctx->job.quality, ctx->quant_luma,
			      ctx->quant_chroma);

	for (i = 0; i < src->vb2_buf.num_planes; i++)
		addr[i] = vb2_dma_contig_plane_dma_addr(&src->vb2_buf, i);

	schedule_delayed_work(&jpge->watchdog,
			      msecs_to_jiffies(HISI_JPGE_TIMEOUT_MS +
					       (u64)ctx->src_fmt.width * ctx->src_fmt.height /
					       HISI_JPGE_TIMEOUT_PIXELS_PER_MS));

	spin_lock_irqsave(&jpge->hw_lock, flags);
	jpge->curr = ctx;
	hisi_jpge_hw_start(jpge, ctx, &l, addr,
			   vb2_dma_contig_plane_dma_addr(&dst->vb2_buf, 0),
			   dst_size);
	spin_unlock_irqrestore(&jpge->hw_lock, flags);

	return;

err:
	vb2_set_plane_payload(&dst->vb2_buf, 0, 0);
	hisi_jpge_finish_bufs(ctx, src, dst);
	v4l2_m2m_buf_done_and_job_finish(jpge->m2m_dev, ctx->fh.m2m_ctx,
					 VB2_BUF_STATE_ERROR);
}

static irqreturn_t hisi_jpge_irq(int irq, void *data)
{
	struct hisi_jpge_dev *jpge = data;
	const struct hisi_jpge_regs *r = jpge->var->regs;
	u32 status, mask = r->int_eop | r->int_buf_full | r->int_cfg_err;

	spin_lock(&jpge->hw_lock);
	status = jpge_read(jpge, r->intstat) & mask;
	if (!status) {
		spin_unlock(&jpge->hw_lock);
		return IRQ_NONE;
	}
	jpge_write(jpge, r->intclr, status);
	if (!jpge->curr) {
		/* No running job to give the cause to: the watchdog took it */
		spin_unlock(&jpge->hw_lock);
		return IRQ_HANDLED;
	}
	jpge->done = jpge->curr;
	jpge->done_status = status;
	jpge->curr = NULL;
	spin_unlock(&jpge->hw_lock);

	return IRQ_WAKE_THREAD;
}

static irqreturn_t hisi_jpge_irq_thread(int irq, void *data)
{
	struct hisi_jpge_dev *jpge = data;
	struct hisi_jpge_ctx *ctx;
	unsigned long flags;
	u32 status;

	spin_lock_irqsave(&jpge->hw_lock, flags);
	ctx = jpge->done;
	status = jpge->done_status;
	jpge->done = NULL;
	spin_unlock_irqrestore(&jpge->hw_lock, flags);

	if (!ctx)
		return IRQ_HANDLED;

	/*
	 * Synchronous: a watchdog instance that has already started must not
	 * run against the next job, which job_done() below can start.
	 */
	cancel_delayed_work_sync(&jpge->watchdog);
	hisi_jpge_job_done(jpge, ctx, status);

	return IRQ_HANDLED;
}

static void hisi_jpge_watchdog(struct work_struct *work)
{
	struct hisi_jpge_dev *jpge =
		container_of(work, struct hisi_jpge_dev, watchdog.work);
	struct hisi_jpge_ctx *ctx;
	unsigned long flags;

	spin_lock_irqsave(&jpge->hw_lock, flags);
	/* The interrupt claimed the job (or it is gone): the thread finishes it */
	ctx = jpge->curr;
	jpge->curr = NULL;
	spin_unlock_irqrestore(&jpge->hw_lock, flags);
	if (!ctx)
		return;

	dev_err_ratelimited(jpge->dev, "encode job timed out, resetting\n");
	hisi_jpge_hw_reset(jpge);
	hisi_jpge_job_done(jpge, ctx, 0);
}

/*
 * No job_abort: the job is ended by the interrupt or by the watchdog, which
 * claim it under hw_lock. An abort called from STREAMOFF, close or remove()
 * would be a third finisher and could expire a watchdog that then runs against
 * a later job, so a hung job makes those calls wait for the watchdog (2 s plus
 * up to 4.1 s for the largest picture) instead.
 */
static const struct v4l2_m2m_ops hisi_jpge_m2m_ops = {
	.device_run = hisi_jpge_device_run,
};

/* ------------------------------------------------------------------ queues */

static unsigned int hisi_jpge_num_planes(struct hisi_jpge_ctx *ctx,
					 struct vb2_queue *vq)
{
	return V4L2_TYPE_IS_OUTPUT(vq->type) ? ctx->src_fmt.num_planes : 1;
}

static u32 hisi_jpge_plane_size(struct hisi_jpge_ctx *ctx, struct vb2_queue *vq,
				unsigned int plane)
{
	return V4L2_TYPE_IS_OUTPUT(vq->type) ?
	       ctx->src_fmt.plane_fmt[plane].sizeimage :
	       ctx->dst_fmt.plane_fmt[0].sizeimage;
}

static int hisi_jpge_queue_setup(struct vb2_queue *vq, unsigned int *nbuffers,
				 unsigned int *nplanes, unsigned int sizes[],
				 struct device *alloc_devs[])
{
	struct hisi_jpge_ctx *ctx = vb2_get_drv_priv(vq);
	unsigned int planes = hisi_jpge_num_planes(ctx, vq), i;

	if (*nplanes) {
		if (*nplanes != planes)
			return -EINVAL;
		for (i = 0; i < planes; i++)
			if (sizes[i] < hisi_jpge_plane_size(ctx, vq, i))
				return -EINVAL;
		return 0;
	}

	*nplanes = planes;
	for (i = 0; i < planes; i++)
		sizes[i] = hisi_jpge_plane_size(ctx, vq, i);

	return 0;
}

static int hisi_jpge_buf_prepare(struct vb2_buffer *vb)
{
	struct hisi_jpge_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	bool out = V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type);
	struct hisi_jpge_layout l;
	unsigned int i;

	if (out) {
		if (vbuf->field == V4L2_FIELD_ANY)
			vbuf->field = V4L2_FIELD_NONE;
		if (vbuf->field != V4L2_FIELD_NONE)
			return -EINVAL;

		/*
		 * Every plane has to hold what the hardware reads, derived
		 * from the picture layout and not from a stored size.
		 */
		if (hisi_jpge_src_layout(ctx, &l))
			return -EINVAL;
		for (i = 0; i < vb->num_planes; i++)
			if (vb2_plane_size(vb, i) < l.plane_size[i])
				return -EINVAL;

		/*
		 * The block reads the picture from the start of the plane. A
		 * plane that starts later (data_offset on an OUTPUT buffer)
		 * would be encoded from the wrong place, so it is refused
		 * instead of being silently shifted.
		 */
		for (i = 0; i < vb->num_planes; i++)
			if (vb->planes[i].data_offset)
				return -EINVAL;
	}

	for (i = 0; i < vb->num_planes; i++) {
		dma_addr_t addr = vb2_dma_contig_plane_dma_addr(vb, i);

		if (vb2_plane_size(vb, i) <
		    hisi_jpge_plane_size(ctx, vb->vb2_queue, i))
			return -EINVAL;
		/* the hardware ignores the low address bits it needs clear */
		if (addr & ((out ? HISI_JPGE_SRC_ALIGN : HISI_JPGE_DST_ALIGN) - 1))
			return -EINVAL;
	}

	return 0;
}

static void hisi_jpge_buf_queue(struct vb2_buffer *vb)
{
	struct hisi_jpge_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);

	/* A STOP with nothing left to encode: this buffer ends the stream */
	if (V4L2_TYPE_IS_CAPTURE(vb->vb2_queue->type) &&
	    vb2_is_streaming(vb->vb2_queue) &&
	    v4l2_m2m_dst_buf_is_last(ctx->fh.m2m_ctx)) {
		vb2_set_plane_payload(vb, 0, 0);
		vbuf->field = V4L2_FIELD_NONE;
		vbuf->sequence = ctx->sequence++;
		v4l2_m2m_last_buffer_done(ctx->fh.m2m_ctx, vbuf);
		v4l2_event_queue_fh(&ctx->fh, &hisi_jpge_eos_event);
		return;
	}

	v4l2_m2m_buf_queue(ctx->fh.m2m_ctx, vbuf);
}

static int hisi_jpge_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct hisi_jpge_ctx *ctx = vb2_get_drv_priv(vq);
	struct v4l2_m2m_ctx *m2m_ctx = ctx->fh.m2m_ctx;

	v4l2_m2m_update_start_streaming_state(m2m_ctx, vq);
	/* Every queue numbers its buffers from 0 after its own STREAMON */
	if (V4L2_TYPE_IS_CAPTURE(vq->type)) {
		ctx->sequence = 0;
		return 0;
	}

	ctx->sequence_out = 0;
	/*
	 * A STREAMOFF and STREAMON pair on the OUTPUT queue resumes the encoder
	 * after a drain (dev-encoder.rst, "Drain"): the stopped state ends and
	 * the CAPTURE queue, which delivered the LAST buffer, delivers again.
	 * A drain that is still pending (no CAPTURE buffer to carry the flag
	 * yet) stays: the next CAPTURE buffer ends it.
	 */
	m2m_ctx->has_stopped = false;
	vb2_clear_last_buffer_dequeued(v4l2_m2m_get_dst_vq(m2m_ctx));

	return 0;
}

static void hisi_jpge_stop_streaming(struct vb2_queue *vq)
{
	struct hisi_jpge_ctx *ctx = vb2_get_drv_priv(vq);
	struct vb2_v4l2_buffer *vbuf;
	bool stopped;

	for (;;) {
		if (V4L2_TYPE_IS_OUTPUT(vq->type))
			vbuf = v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx);
		else
			vbuf = v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);
		if (!vbuf)
			break;
		v4l2_m2m_buf_done(vbuf, VB2_BUF_STATE_ERROR);
	}

	/*
	 * STREAMOFF of the OUTPUT queue in the middle of a drain ends it at
	 * once (the core flags the next CAPTURE buffer as LAST): that is the
	 * end of the stream, announce it once. A context that had stopped
	 * before has announced it already.
	 */
	stopped = v4l2_m2m_has_stopped(ctx->fh.m2m_ctx);
	v4l2_m2m_update_stop_streaming_state(ctx->fh.m2m_ctx, vq);
	if (V4L2_TYPE_IS_OUTPUT(vq->type) && !stopped &&
	    v4l2_m2m_has_stopped(ctx->fh.m2m_ctx))
		v4l2_event_queue_fh(&ctx->fh, &hisi_jpge_eos_event);
}

static const struct vb2_ops hisi_jpge_qops = {
	.queue_setup = hisi_jpge_queue_setup,
	.buf_prepare = hisi_jpge_buf_prepare,
	.buf_queue = hisi_jpge_buf_queue,
	.start_streaming = hisi_jpge_start_streaming,
	.stop_streaming = hisi_jpge_stop_streaming,
};

static int hisi_jpge_queue_init(void *priv, struct vb2_queue *src_vq,
				struct vb2_queue *dst_vq)
{
	struct hisi_jpge_ctx *ctx = priv;
	int ret;

	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	src_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	src_vq->drv_priv = ctx;
	src_vq->ops = &hisi_jpge_qops;
	src_vq->mem_ops = &vb2_dma_contig_memops;
	src_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->lock = &ctx->dev->lock;
	src_vq->dev = ctx->dev->dev;

	ret = vb2_queue_init(src_vq);
	if (ret)
		return ret;

	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	dst_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	dst_vq->drv_priv = ctx;
	dst_vq->ops = &hisi_jpge_qops;
	dst_vq->mem_ops = &vb2_dma_contig_memops;
	dst_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock = &ctx->dev->lock;
	dst_vq->dev = ctx->dev->dev;
	/*
	 * The CPU reads and writes the stream buffer (hisi_jpge_cpu_begin()):
	 * mapped in both directions, an imported buffer is synced by the
	 * exporter in the direction that the dma-buf CPU access calls use (DMA
	 * API debugging checks that the two agree).
	 */
	dst_vq->bidirectional = true;

	return vb2_queue_init(dst_vq);
}

/* ---------------------------------------------------------------- controls */

static int hisi_jpge_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct hisi_jpge_ctx *ctx =
		container_of(ctrl->handler, struct hisi_jpge_ctx, hdl);
	unsigned long flags;

	spin_lock_irqsave(&ctx->ctrl_lock, flags);
	switch (ctrl->id) {
	case V4L2_CID_JPEG_COMPRESSION_QUALITY:
		ctx->params.quality = ctrl->val;
		break;
	case V4L2_CID_JPEG_CHROMA_SUBSAMPLING:
		ctx->params.sampling = hisi_jpge_menu_to_sampling(ctrl->val);
		break;
	case V4L2_CID_JPEG_ACTIVE_MARKER:
		ctx->params.markers = ctrl->val;
		break;
	case V4L2_CID_JPEG_RESTART_INTERVAL:
		ctx->params.restart = ctrl->val;
		break;
	default:
		spin_unlock_irqrestore(&ctx->ctrl_lock, flags);
		return -EINVAL;
	}
	spin_unlock_irqrestore(&ctx->ctrl_lock, flags);

	return 0;
}

static const struct v4l2_ctrl_ops hisi_jpge_ctrl_ops = {
	.s_ctrl = hisi_jpge_s_ctrl,
};

/* Offer exactly the JPEG subsamplings the input format can produce */
static void hisi_jpge_update_sampling_ctrl(struct hisi_jpge_ctx *ctx)
{
	const struct hisi_jpge_fmt *fmt = ctx->fmt;
	u64 skip = hisi_jpge_sampling_skip_mask(ctx->dev->var, fmt);

	int native = hisi_jpge_sampling_to_menu(fmt->sampling);

	v4l2_ctrl_modify_range(ctx->ctrl_sampling,
			       V4L2_JPEG_CHROMA_SUBSAMPLING_444,
			       V4L2_JPEG_CHROMA_SUBSAMPLING_420, skip, native);
	/*
	 * A new input format with another native sampling starts at that
	 * sampling: keeping the previous choice would silently turn 4:2:2
	 * input into 4:2:0 output after a switch from a 4:2:0 format.
	 */
	if (ctx->sampling_native != native) {
		ctx->sampling_native = native;
		v4l2_ctrl_s_ctrl(ctx->ctrl_sampling, native);
	}
}

/* ------------------------------------------------------------------ ioctls */

/*
 * The block does no colorimetry conversion and takes full-range YCbCr as
 * given. The V4L2 JPEG codec rule is the fixed JFIF colorimetry (sRGB
 * primaries and transfer function, BT.601 matrix, full range) on the raw
 * side, whatever the application asks for.
 */
static void hisi_jpge_fill_colorimetry(struct v4l2_pix_format_mplane *pix)
{
	pix->colorspace = V4L2_COLORSPACE_SRGB;
	pix->ycbcr_enc = V4L2_YCBCR_ENC_601;
	pix->quantization = V4L2_QUANTIZATION_FULL_RANGE;
	pix->xfer_func = V4L2_XFER_FUNC_SRGB;
}

static int hisi_jpge_fill_src(const struct hisi_jpge_dev *jpge,
			      struct v4l2_pix_format_mplane *pix,
			      const struct hisi_jpge_fmt *fmt)
{
	const struct hisi_jpge_variant *var = jpge->var;
	struct v4l2_frmsize_stepwise frmsize = {
		.min_width = var->min_width,
		.max_width = var->max_width,
		.min_height = var->min_height,
		.max_height = var->max_height,
		.step_width = var->step_width,
		.step_height = var->step_height,
	};
	u32 user_ys = pix->plane_fmt[0].bytesperline;
	u32 user_cs = pix->plane_fmt[1].bytesperline;
	struct hisi_jpge_layout l;
	unsigned int i;
	int ret;

	v4l2_apply_frmsize_constraints(&pix->width, &pix->height, &frmsize);
	ret = hisi_jpge_calc_layout(var, fmt, pix->width, pix->height, user_ys,
				    user_cs, &l);
	if (ret)
		return ret;

	memset(pix->plane_fmt, 0, sizeof(pix->plane_fmt));
	memset(pix->reserved, 0, sizeof(pix->reserved));
	pix->flags = 0;
	pix->pixelformat = fmt->fourcc;
	pix->num_planes = fmt->mem_planes;
	pix->field = V4L2_FIELD_NONE;
	for (i = 0; i < fmt->mem_planes; i++) {
		pix->plane_fmt[i].sizeimage = l.plane_size[i];
		if (i == 0)
			pix->plane_fmt[i].bytesperline = l.ys;
		else
			pix->plane_fmt[i].bytesperline = l.cs;
	}
	hisi_jpge_fill_colorimetry(pix);

	return 0;
}

static void hisi_jpge_fill_dst(const struct hisi_jpge_ctx *ctx,
			       struct v4l2_pix_format_mplane *pix,
			       u32 user_size)
{
	const struct v4l2_pix_format_mplane *src = &ctx->src_fmt;

	memset(pix->plane_fmt, 0, sizeof(pix->plane_fmt));
	memset(pix->reserved, 0, sizeof(pix->reserved));
	pix->flags = 0;
	pix->width = src->width;
	pix->height = src->height;
	pix->pixelformat = V4L2_PIX_FMT_JPEG;
	pix->num_planes = 1;
	pix->plane_fmt[0].bytesperline = 0;
	/* an application may ask for more room (noisy pictures) */
	pix->plane_fmt[0].sizeimage =
		max(user_size, hisi_jpge_stream_size(pix->width, pix->height));
	pix->field = V4L2_FIELD_NONE;
	/* the JPEG carries the (fixed) colorimetry of the input */
	pix->colorspace = src->colorspace;
	pix->ycbcr_enc = src->ycbcr_enc;
	pix->quantization = src->quantization;
	pix->xfer_func = src->xfer_func;
}

static int hisi_jpge_querycap(struct file *file, void *priv,
			      struct v4l2_capability *cap)
{
	struct hisi_jpge_ctx *ctx = file_to_ctx(file);

	strscpy(cap->driver, HISI_JPGE_NAME, sizeof(cap->driver));
	strscpy(cap->card, ctx->dev->var->name, sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s",
		 dev_name(ctx->dev->dev));

	return 0;
}

static int hisi_jpge_enum_fmt_cap(struct file *file, void *priv,
				  struct v4l2_fmtdesc *f)
{
	if (f->index)
		return -EINVAL;

	f->pixelformat = V4L2_PIX_FMT_JPEG;
	f->flags = V4L2_FMT_FLAG_COMPRESSED;

	return 0;
}

static int hisi_jpge_enum_fmt_out(struct file *file, void *priv,
				  struct v4l2_fmtdesc *f)
{
	struct hisi_jpge_ctx *ctx = file_to_ctx(file);

	if (f->index >= ctx->dev->var->num_fmts)
		return -EINVAL;

	f->pixelformat = ctx->dev->var->fmts[f->index].fourcc;

	return 0;
}

static int hisi_jpge_enum_framesizes(struct file *file, void *priv,
				     struct v4l2_frmsizeenum *fsize)
{
	const struct hisi_jpge_variant *var = file_to_ctx(file)->dev->var;

	if (fsize->index)
		return -EINVAL;
	if (fsize->pixel_format != V4L2_PIX_FMT_JPEG &&
	    !hisi_jpge_find_fmt(var, fsize->pixel_format))
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_STEPWISE;
	fsize->stepwise.min_width = var->min_width;
	fsize->stepwise.max_width = var->max_width;
	fsize->stepwise.step_width = var->step_width;
	fsize->stepwise.min_height = var->min_height;
	fsize->stepwise.max_height = var->max_height;
	fsize->stepwise.step_height = var->step_height;

	return 0;
}

static int hisi_jpge_g_fmt_out(struct file *file, void *priv,
			       struct v4l2_format *f)
{
	f->fmt.pix_mp = file_to_ctx(file)->src_fmt;

	return 0;
}

static int hisi_jpge_g_fmt_cap(struct file *file, void *priv,
			       struct v4l2_format *f)
{
	f->fmt.pix_mp = file_to_ctx(file)->dst_fmt;

	return 0;
}

static int hisi_jpge_try_fmt_cap(struct file *file, void *priv,
				 struct v4l2_format *f)
{
	struct hisi_jpge_ctx *ctx = file_to_ctx(file);

	/* The JPEG picture has the size of the input picture */
	hisi_jpge_fill_dst(ctx, &f->fmt.pix_mp,
			   f->fmt.pix_mp.plane_fmt[0].sizeimage);

	return 0;
}

static int hisi_jpge_try_fmt_out(struct file *file, void *priv,
				 struct v4l2_format *f)
{
	struct hisi_jpge_ctx *ctx = file_to_ctx(file);
	const struct hisi_jpge_variant *var = ctx->dev->var;
	const struct hisi_jpge_fmt *fmt;

	fmt = hisi_jpge_find_fmt(var, f->fmt.pix_mp.pixelformat);
	if (!fmt)
		fmt = &var->fmts[0];

	return hisi_jpge_fill_src(ctx->dev, &f->fmt.pix_mp, fmt);
}

static int hisi_jpge_s_fmt_cap(struct file *file, void *priv,
			       struct v4l2_format *f)
{
	struct hisi_jpge_ctx *ctx = file_to_ctx(file);
	struct vb2_queue *vq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx, f->type);
	u32 req;

	if (vb2_is_busy(vq))
		return -EBUSY;

	req = f->fmt.pix_mp.plane_fmt[0].sizeimage;
	hisi_jpge_try_fmt_cap(file, priv, f);
	ctx->dst_fmt = f->fmt.pix_mp;
	/*
	 * Remember a request for more room than the default, so that a later
	 * S_FMT(OUTPUT) (a new picture size) does not drop it: the order of the
	 * two S_FMT calls does not matter. A request at or below the default
	 * is not kept, the default follows the picture size.
	 */
	ctx->dst_req_size = req > hisi_jpge_stream_size(ctx->dst_fmt.width,
							ctx->dst_fmt.height) ?
			    req : 0;

	return 0;
}

static int hisi_jpge_s_fmt_out(struct file *file, void *priv,
			       struct v4l2_format *f)
{
	struct hisi_jpge_ctx *ctx = file_to_ctx(file);
	struct vb2_queue *vq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx, f->type);
	int ret;

	/* The capture format follows the output one: both must be idle */
	if (vb2_is_busy(vq) ||
	    vb2_is_busy(v4l2_m2m_get_dst_vq(ctx->fh.m2m_ctx)))
		return -EBUSY;

	ret = hisi_jpge_try_fmt_out(file, priv, f);
	if (ret)
		return ret;
	ctx->src_fmt = f->fmt.pix_mp;
	ctx->fmt = hisi_jpge_find_fmt(ctx->dev->var, f->fmt.pix_mp.pixelformat);

	/* Keep the capture format in step with the new picture size */
	hisi_jpge_fill_dst(ctx, &ctx->dst_fmt, ctx->dst_req_size);
	hisi_jpge_update_sampling_ctrl(ctx);

	return 0;
}

static int hisi_jpge_subscribe_event(struct v4l2_fh *fh,
				     const struct v4l2_event_subscription *sub)
{
	if (sub->type == V4L2_EVENT_EOS)
		return v4l2_event_subscribe(fh, sub, 2, NULL);

	return v4l2_ctrl_subscribe_event(fh, sub);
}

/*
 * ENCODER_CMD as dev-encoder.rst has it. The commands only act while both
 * queues stream, otherwise they succeed and do nothing. A STOP that finds
 * nothing to encode ends the drain inside the core helper, so the stream end
 * is announced here; one that finds a job in the queue is ended by the
 * completion of the job that holds the last OUTPUT buffer. A new drain and the
 * resume by START both end the "last buffer dequeued" state of the CAPTURE
 * queue, otherwise a CAPTURE DQBUF would fail with -EPIPE instead of waiting
 * for the next buffer, and START has to look for work that was queued while the
 * encoder was stopped.
 */
static int hisi_jpge_encoder_cmd(struct file *file, void *priv,
				 struct v4l2_encoder_cmd *ec)
{
	struct v4l2_fh *fh = file_to_v4l2_fh(file);
	struct v4l2_m2m_ctx *m2m_ctx = fh->m2m_ctx;
	bool stopped;
	int ret;

	ret = v4l2_m2m_ioctl_try_encoder_cmd(file, priv, ec);
	if (ret)
		return ret;

	if (!vb2_is_streaming(v4l2_m2m_get_src_vq(m2m_ctx)) ||
	    !vb2_is_streaming(v4l2_m2m_get_dst_vq(m2m_ctx)))
		return 0;

	stopped = v4l2_m2m_has_stopped(m2m_ctx);
	ret = v4l2_m2m_ioctl_encoder_cmd(file, priv, ec);
	if (ret)
		return ret;

	if (ec->cmd == V4L2_ENC_CMD_START) {
		vb2_clear_last_buffer_dequeued(v4l2_m2m_get_dst_vq(m2m_ctx));
		v4l2_m2m_try_schedule(m2m_ctx);
	} else if (!stopped) {
		/* A drain began (a STOP of a stopped encoder changes nothing) */
		vb2_clear_last_buffer_dequeued(v4l2_m2m_get_dst_vq(m2m_ctx));
		if (v4l2_m2m_has_stopped(m2m_ctx))
			v4l2_event_queue_fh(fh, &hisi_jpge_eos_event);
	}

	return 0;
}

static const struct v4l2_ioctl_ops hisi_jpge_ioctl_ops = {
	.vidioc_querycap = hisi_jpge_querycap,

	.vidioc_enum_fmt_vid_cap = hisi_jpge_enum_fmt_cap,
	.vidioc_g_fmt_vid_cap_mplane = hisi_jpge_g_fmt_cap,
	.vidioc_try_fmt_vid_cap_mplane = hisi_jpge_try_fmt_cap,
	.vidioc_s_fmt_vid_cap_mplane = hisi_jpge_s_fmt_cap,

	.vidioc_enum_fmt_vid_out = hisi_jpge_enum_fmt_out,
	.vidioc_g_fmt_vid_out_mplane = hisi_jpge_g_fmt_out,
	.vidioc_try_fmt_vid_out_mplane = hisi_jpge_try_fmt_out,
	.vidioc_s_fmt_vid_out_mplane = hisi_jpge_s_fmt_out,

	.vidioc_enum_framesizes = hisi_jpge_enum_framesizes,

	.vidioc_reqbufs = v4l2_m2m_ioctl_reqbufs,
	.vidioc_querybuf = v4l2_m2m_ioctl_querybuf,
	.vidioc_qbuf = v4l2_m2m_ioctl_qbuf,
	.vidioc_dqbuf = v4l2_m2m_ioctl_dqbuf,
	.vidioc_prepare_buf = v4l2_m2m_ioctl_prepare_buf,
	.vidioc_create_bufs = v4l2_m2m_ioctl_create_bufs,
	.vidioc_expbuf = v4l2_m2m_ioctl_expbuf,

	.vidioc_streamon = v4l2_m2m_ioctl_streamon,
	.vidioc_streamoff = v4l2_m2m_ioctl_streamoff,

	.vidioc_try_encoder_cmd = v4l2_m2m_ioctl_try_encoder_cmd,
	.vidioc_encoder_cmd = hisi_jpge_encoder_cmd,

	.vidioc_subscribe_event = hisi_jpge_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
};

/* ----------------------------------------------------------- file operations */

static int hisi_jpge_open(struct file *file)
{
	struct hisi_jpge_dev *jpge = video_drvdata(file);
	struct hisi_jpge_ctx *ctx;
	int ret;

	ctx = kzalloc_obj(*ctx);
	if (!ctx)
		return -ENOMEM;

	if (mutex_lock_interruptible(&jpge->lock)) {
		ret = -ERESTARTSYS;
		goto err_free;
	}

	ctx->dev = jpge;
	ctx->fmt = &jpge->var->fmts[0];
	spin_lock_init(&ctx->ctrl_lock);
	ctx->params.quality = HISI_JPGE_QUALITY_DEFAULT;
	ctx->params.sampling = ctx->fmt->sampling;
	ctx->sampling_native = hisi_jpge_sampling_to_menu(ctx->fmt->sampling);
	ctx->params.markers = HISI_JPGE_MARKERS_DEFAULT;

	v4l2_fh_init(&ctx->fh, video_devdata(file));

	ctx->src_fmt.width = HISI_JPGE_DEF_WIDTH;
	ctx->src_fmt.height = HISI_JPGE_DEF_HEIGHT;
	ret = hisi_jpge_fill_src(jpge, &ctx->src_fmt, ctx->fmt);
	if (ret)
		goto err_exit;
	hisi_jpge_fill_dst(ctx, &ctx->dst_fmt, 0);

	v4l2_ctrl_handler_init(&ctx->hdl, 4);
	v4l2_ctrl_new_std(&ctx->hdl, &hisi_jpge_ctrl_ops,
			  V4L2_CID_JPEG_COMPRESSION_QUALITY, 1, 100, 1,
			  HISI_JPGE_QUALITY_DEFAULT);
	ctx->ctrl_sampling =
		v4l2_ctrl_new_std_menu(&ctx->hdl, &hisi_jpge_ctrl_ops,
				       V4L2_CID_JPEG_CHROMA_SUBSAMPLING,
				       V4L2_JPEG_CHROMA_SUBSAMPLING_420,
				       hisi_jpge_sampling_skip_mask(jpge->var,
								    ctx->fmt),
				       hisi_jpge_sampling_to_menu(ctx->fmt->sampling));
	v4l2_ctrl_new_std(&ctx->hdl, &hisi_jpge_ctrl_ops,
			  V4L2_CID_JPEG_ACTIVE_MARKER, 0,
			  HISI_JPGE_MARKERS_SUPPORTED, 0,
			  HISI_JPGE_MARKERS_DEFAULT);
	if (jpge->var->restart)
		v4l2_ctrl_new_std(&ctx->hdl, &hisi_jpge_ctrl_ops,
				  V4L2_CID_JPEG_RESTART_INTERVAL, 0,
				  HISI_JPGE_RESTART_MAX, 1, 0);
	if (ctx->hdl.error) {
		ret = ctx->hdl.error;
		goto err_exit;
	}
	ctx->fh.ctrl_handler = &ctx->hdl;

	ctx->fh.m2m_ctx = v4l2_m2m_ctx_init(jpge->m2m_dev, ctx,
					    hisi_jpge_queue_init);
	if (IS_ERR(ctx->fh.m2m_ctx)) {
		ret = PTR_ERR(ctx->fh.m2m_ctx);
		goto err_exit;
	}

	v4l2_fh_add(&ctx->fh, file);
	mutex_unlock(&jpge->lock);

	return 0;

err_exit:
	v4l2_ctrl_handler_free(&ctx->hdl);
	v4l2_fh_exit(&ctx->fh);
	mutex_unlock(&jpge->lock);
err_free:
	kfree(ctx);

	return ret;
}

static int hisi_jpge_release(struct file *file)
{
	struct hisi_jpge_dev *jpge = video_drvdata(file);
	struct hisi_jpge_ctx *ctx = file_to_ctx(file);

	mutex_lock(&jpge->lock);
	v4l2_m2m_ctx_release(ctx->fh.m2m_ctx);
	v4l2_ctrl_handler_free(&ctx->hdl);
	v4l2_fh_del(&ctx->fh, file);
	v4l2_fh_exit(&ctx->fh);
	mutex_unlock(&jpge->lock);
	kfree(ctx);

	return 0;
}

static const struct v4l2_file_operations hisi_jpge_fops = {
	.owner = THIS_MODULE,
	.open = hisi_jpge_open,
	.release = hisi_jpge_release,
	.poll = v4l2_m2m_fop_poll,
	.unlocked_ioctl = video_ioctl2,
	.mmap = v4l2_m2m_fop_mmap,
};

/*
 * Called when the last user of the video device is gone: with an open file
 * the device can outlive the unbind of the driver, and the file release still
 * needs the mutex, the mem2mem device and the V4L2 device.
 */
static void hisi_jpge_vfd_release(struct video_device *vfd)
{
	struct hisi_jpge_dev *jpge = container_of(vfd, struct hisi_jpge_dev, vfd);

	v4l2_m2m_release(jpge->m2m_dev);
	v4l2_device_unregister(&jpge->v4l2_dev);
	kfree(jpge);
}

static const struct video_device hisi_jpge_videodev = {
	.name = HISI_JPGE_NAME,
	.fops = &hisi_jpge_fops,
	.ioctl_ops = &hisi_jpge_ioctl_ops,
	.minor = -1,
	/* hisi_jpge_vfd_release() is set once the device is registered, see probe */
	.release = video_device_release_empty,
	.vfl_dir = VFL_DIR_M2M,
	.device_caps = V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING,
};

/* --------------------------------------------------------- power management */

/*
 * The block must have its clock running and its reset released before any
 * register access: reading a clock-gated block can stall the bus. The reset
 * is pulsed on every power-up so the block starts from its reset state
 * whatever the boot loader or a previous owner left behind (the CRG reset
 * value differs between SoCs: in reset on the Hi3520D V100, released on
 * the Hi3536D V100).
 */
static int hisi_jpge_runtime_resume(struct device *dev)
{
	struct hisi_jpge_dev *jpge = dev_get_drvdata(dev);
	int ret;

	ret = clk_prepare_enable(jpge->clk);
	if (ret)
		return ret;

	ret = reset_control_assert(jpge->rst);
	if (ret)
		goto err_clk;
	usleep_range(10, 20);
	ret = reset_control_deassert(jpge->rst);
	if (ret)
		goto err_clk;
	usleep_range(10, 20);

	enable_irq(jpge->irq);

	return 0;

err_clk:
	clk_disable_unprepare(jpge->clk);

	return ret;
}

static int hisi_jpge_runtime_suspend(struct device *dev)
{
	struct hisi_jpge_dev *jpge = dev_get_drvdata(dev);

	disable_irq(jpge->irq);
	reset_control_assert(jpge->rst);
	clk_disable_unprepare(jpge->clk);

	return 0;
}

/* System sleep: let a running job finish, then power the block down */
static int hisi_jpge_suspend(struct device *dev)
{
	struct hisi_jpge_dev *jpge = dev_get_drvdata(dev);
	int ret;

	v4l2_m2m_suspend(jpge->m2m_dev);

	ret = pm_runtime_force_suspend(dev);
	/* A failed suspend gets no resume call: do not leave the queue stopped */
	if (ret)
		v4l2_m2m_resume(jpge->m2m_dev);

	return ret;
}

static int hisi_jpge_resume(struct device *dev)
{
	struct hisi_jpge_dev *jpge = dev_get_drvdata(dev);
	int ret;

	ret = pm_runtime_force_resume(dev);
	/*
	 * Resume the queue even when the block did not come back: a job then
	 * ends with an error (its runtime resume fails) instead of waiting
	 * for a block that never answers.
	 */
	v4l2_m2m_resume(jpge->m2m_dev);

	return ret;
}

static const struct dev_pm_ops hisi_jpge_pm_ops = {
	SYSTEM_SLEEP_PM_OPS(hisi_jpge_suspend, hisi_jpge_resume)
	RUNTIME_PM_OPS(hisi_jpge_runtime_suspend, hisi_jpge_runtime_resume,
		       NULL)
};

/* ------------------------------------------------------------------ probe */

static int hisi_jpge_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hisi_jpge_dev *jpge;
	u32 version;
	int ret;

	/* Not devm: freed by the video device release, see hisi_jpge_vfd_release() */
	jpge = kzalloc_obj(*jpge);
	if (!jpge)
		return -ENOMEM;

	jpge->dev = dev;
	jpge->var = of_device_get_match_data(dev);
	if (!jpge->var) {
		ret = -ENODEV;
		goto err_free;
	}

	mutex_init(&jpge->lock);
	spin_lock_init(&jpge->hw_lock);
	INIT_DELAYED_WORK(&jpge->watchdog, hisi_jpge_watchdog);
	platform_set_drvdata(pdev, jpge);

	jpge->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(jpge->base)) {
		ret = PTR_ERR(jpge->base);
		goto err_free;
	}

	jpge->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(jpge->clk)) {
		ret = dev_err_probe(dev, PTR_ERR(jpge->clk), "no clock\n");
		goto err_free;
	}

	jpge->rst = devm_reset_control_get_exclusive(dev, NULL);
	if (IS_ERR(jpge->rst)) {
		ret = dev_err_probe(dev, PTR_ERR(jpge->rst), "no reset\n");
		goto err_free;
	}

	jpge->irq = platform_get_irq(pdev, 0);
	if (jpge->irq < 0) {
		ret = jpge->irq;
		goto err_free;
	}

	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret) {
		ret = dev_err_probe(dev, ret, "no 32-bit DMA\n");
		goto err_free;
	}
	dma_set_max_seg_size(dev, DMA_BIT_MASK(32));

	/* Stays disabled until the clock runs and the reset is released */
	ret = devm_request_threaded_irq(dev, jpge->irq, hisi_jpge_irq,
					hisi_jpge_irq_thread, IRQF_NO_AUTOEN,
					dev_name(dev), jpge);
	if (ret) {
		ret = dev_err_probe(dev, ret, "cannot request irq\n");
		goto err_free;
	}

	pm_runtime_set_autosuspend_delay(dev, HISI_JPGE_AUTOSUSPEND_MS);
	pm_runtime_use_autosuspend(dev);
	pm_runtime_enable(dev);

	/* Safe probe: clock on and reset released before the first read */
	ret = pm_runtime_resume_and_get(dev);
	if (ret) {
		pm_runtime_disable(dev);
		ret = dev_err_probe(dev, ret, "cannot power up\n");
		goto err_free;
	}
	version = jpge_read(jpge, jpge->var->regs->version);
	if (version == jpge->var->version)
		dev_info(dev, "%s, version register 0x%08x\n", jpge->var->name,
			 version);
	else
		dev_warn(dev, "%s: version register reads 0x%08x, expected 0x%08x: the register layout may differ from the one this driver programs\n",
			 jpge->var->name, version, jpge->var->version);
	pm_runtime_put_autosuspend(dev);

	ret = v4l2_device_register(dev, &jpge->v4l2_dev);
	if (ret)
		goto err_pm;

	jpge->m2m_dev = v4l2_m2m_init(&hisi_jpge_m2m_ops);
	if (IS_ERR(jpge->m2m_dev)) {
		ret = PTR_ERR(jpge->m2m_dev);
		dev_err(dev, "cannot init mem2mem device: %d\n", ret);
		goto err_v4l2;
	}

	jpge->vfd = hisi_jpge_videodev;
	jpge->vfd.lock = &jpge->lock;
	jpge->vfd.v4l2_dev = &jpge->v4l2_dev;
	video_set_drvdata(&jpge->vfd, jpge);

	ret = video_register_device(&jpge->vfd, VFL_TYPE_VIDEO, -1);
	if (ret) {
		dev_err(dev, "cannot register video device: %d\n", ret);
		goto err_m2m;
	}
	/*
	 * A failed registration puts the device, which calls the release
	 * callback: the empty one of the template, so the error path above
	 * does the freeing. From here on the last user of the device frees it.
	 */
	jpge->vfd.release = hisi_jpge_vfd_release;

	v4l2_info(&jpge->v4l2_dev, "registered as /dev/video%d\n",
		  jpge->vfd.num);

	return 0;

err_m2m:
	v4l2_m2m_release(jpge->m2m_dev);
err_v4l2:
	v4l2_device_unregister(&jpge->v4l2_dev);
err_pm:
	pm_runtime_dont_use_autosuspend(dev);
	pm_runtime_disable(dev);
	if (!pm_runtime_status_suspended(dev))
		hisi_jpge_runtime_suspend(dev);
	pm_runtime_set_suspended(dev);
err_free:
	kfree(jpge);

	return ret;
}

static void hisi_jpge_remove(struct platform_device *pdev)
{
	struct hisi_jpge_dev *jpge = platform_get_drvdata(pdev);

	/*
	 * Stop the running job and keep further ones from starting: this
	 * returns once the interrupt or the watchdog has finished it, which
	 * needs the mem2mem device, the interrupt and the clock to be alive.
	 */
	v4l2_m2m_suspend(jpge->m2m_dev);
	cancel_delayed_work_sync(&jpge->watchdog);

	/* Nothing runs any more: the interrupt is disabled and the block off */
	pm_runtime_dont_use_autosuspend(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	if (!pm_runtime_status_suspended(&pdev->dev))
		hisi_jpge_runtime_suspend(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);

	/*
	 * The video device goes last: files that are still open keep @jpge,
	 * the mem2mem device and the V4L2 device alive until they are closed
	 * (the release callback of the video device frees them), and when none
	 * is open that happens inside video_unregister_device(). No new
	 * opens or ioctls from here on.
	 */
	devm_free_irq(&pdev->dev, jpge->irq, jpge);
	video_unregister_device(&jpge->vfd);
}

static const struct of_device_id hisi_jpge_of_match[] = {
	{ .compatible = "hisilicon,hi3520dv100-jpge",
	  .data = &hisi_jpge_hi3520dv100 },
	{ .compatible = "hisilicon,hi3536dv100-jpge",
	  .data = &hisi_jpge_hi3536dv100 },
	{ }
};
MODULE_DEVICE_TABLE(of, hisi_jpge_of_match);

static struct platform_driver hisi_jpge_driver = {
	.probe = hisi_jpge_probe,
	.remove = hisi_jpge_remove,
	.driver = {
		.name = HISI_JPGE_NAME,
		.pm = pm_ptr(&hisi_jpge_pm_ops),
		.of_match_table = hisi_jpge_of_match,
	},
};
module_platform_driver(hisi_jpge_driver);

MODULE_DESCRIPTION("HiSilicon JPEG encoder (JPGE) V4L2 mem2mem driver");
MODULE_LICENSE("GPL");
