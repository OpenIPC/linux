/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * HiSilicon JPEG encoder (JPGE) register layout "r004".
 *
 * The layout comes from the vendor header jpge_reg(r004).h, which ships
 * unused in the Hi3520D V100 vendor U-Boot tree, from the vendor JPEG encoder
 * modules of other SoCs and from the way the HiSTB SDK driver programs the
 * same register family. No data sheet has register tables for this block
 * (only the feature list and the CRG bits), so every offset and field below
 * is "documented by vendor code, not by the data sheet". Board tests on the
 * Hi3520D V100 and on the Hi3536D V100 (VERSION, the QPT and stream registers,
 * every input format and size the driver offers) confirmed everything this
 * driver uses; both report the same VERSION word.
 */
#ifndef HISI_JPGE_REGS_H
#define HISI_JPGE_REGS_H

#include <linux/bits.h>

/* Register offsets */
#define JPGE_R004_INTSTAT		0x000
#define JPGE_R004_INTMASK		0x004
#define JPGE_R004_RAWINT		0x008
#define JPGE_R004_INTCLR		0x00c
#define JPGE_R004_PICFG			0x010
#define JPGE_R004_ECSCFG		0x014
#define JPGE_R004_START			0x01c
#define JPGE_R004_IMGSIZE		0x020
#define JPGE_R004_SRCYADDR		0x030
#define JPGE_R004_SRCCADDR		0x034
#define JPGE_R004_SRCVADDR		0x038
#define JPGE_R004_SSTRIDE		0x03c
#define JPGE_R004_STRMADDR		0x040
#define JPGE_R004_SRPTRADDR		0x044
#define JPGE_R004_SWPTRADDR		0x048
#define JPGE_R004_STRMBUFLEN		0x04c
#define JPGE_R004_OUTSTD		0x0a4
#define JPGE_R004_VERSION		0x0fc
#define JPGE_R004_VPP_CFG		0x500	/* overlay (OSD) window */
#define JPGE_R004_VPP_OSD_CFG		0x510
#define JPGE_R004_QPT			0x1000	/* 3 tables x 16 words */

/* VERSION register value of the r004 layout: ASCII "V2R4" */
#define JPGE_R004_VERSION_VALUE		0x56325234

/* INTSTAT / INTMASK / RAWINT / INTCLR bits (mask: 1 = enabled) */
#define JPGE_R004_INT_EOP		BIT(0)
#define JPGE_R004_INT_BUF_FULL		BIT(1)
#define JPGE_R004_INT_TIMEOUT		BIT(2)
#define JPGE_R004_INT_CFG_ERR		BIT(3)

/* PICFG fields */
#define JPGE_R004_PICFG_STRFMT		GENMASK(1, 0)
#define JPGE_R004_PICFG_YUVFMT		GENMASK(3, 2)
#define JPGE_R004_PICFG_CLKGATE		GENMASK(15, 14)
#define JPGE_R004_PICFG_MEMCLKGATE	BIT(16)
/*
 * roundEn: the quantizer rounds to nearest like libjpeg (set); clear it
 * truncates toward zero, which gives smaller files and a lower PSNR
 */
#define JPGE_R004_PICFG_ROUNDEN		BIT(19)
#define JPGE_R004_PICFG_PACKAGESEL	GENMASK(27, 20)
/*
 * col2grayEn: Cb and Cr are coded as the neutral value 128 while Y is unchanged
 * (Hi3536D V100 board test 2026-11-22). The block still writes a three
 * component scan; the vendor module has no one-component mode either.
 */
#define JPGE_R004_PICFG_COL2GRAY	BIT(29)

/*
 * ECSCFG: the encoder ends a restart interval (an "entropy coded segment") after
 * ecsSize + 1 MCUs when ecsSplit is set. It then writes every segment as its
 * own packet with its own descriptor and starts every segment but the first
 * with the RSTn marker (modulo 8).
 */
#define JPGE_R004_ECSCFG_SPLIT		BIT(0)
#define JPGE_R004_ECSCFG_SIZE		GENMASK(20, 1)

/* IMGSIZE fields: width - 1 and height - 1 */
#define JPGE_R004_IMGSIZE_W		GENMASK(12, 0)
#define JPGE_R004_IMGSIZE_H		GENMASK(28, 16)

/* SSTRIDE fields, bytes */
#define JPGE_R004_SSTRIDE_Y		GENMASK(15, 0)
#define JPGE_R004_SSTRIDE_C		GENMASK(31, 16)

/*
 * Overlay (OSD) enables, from the vendor header only: VPP_CFG osdEn [5], and
 * VPP_OSD_CFG osd0..7GlobalEn [23:16] and osd0..7En [31:24]. The driver
 * clears them before every job (no overlay is used).
 */
#define JPGE_R004_VPP_CFG_OSDEN		BIT(5)
#define JPGE_R004_VPP_OSD_CFG_REGIONS	GENMASK(31, 16)

/* STRMBUFLEN: byte count, bits [5:0] are not stored */
#define JPGE_R004_STRMBUFLEN_LEN	GENMASK(31, 6)

/*
 * PICFG strFmt / yuvFmt encodings (from the HiSTB driver, confirmed on the
 * boards): strFmt 0 on both SoCs, strFmt 1 and 2 on the Hi3520D V100, the only
 * one that offers planar and packed input; yuvFmt 0 and 1 on both SoCs, 2 on
 * the Hi3520D V100.
 */
#define JPGE_R004_STRFMT_SEMIPLANAR	0
#define JPGE_R004_STRFMT_PLANAR		1
#define JPGE_R004_STRFMT_PACKED		2
#define JPGE_R004_YUV420		0
#define JPGE_R004_YUV422		1
#define JPGE_R004_YUV444		2

/*
 * PICFG packageSel for packed input: four 2 bit fields, each the byte position
 * (0..3) of one component inside a 4 byte Y0 Cb/Cr Y1 Cr/Cb group:
 * bits [1:0] Y0, [3:2] Y1, [5:4] Cb, [7:6] Cr. The reset value 0xd8 is YUYV
 * (Y0 at 0, Y1 at 2, Cb at 1, Cr at 3), the only packed format the data sheet
 * lists; UYVY 0x8d and YVYU 0x78 are the values the vendor SDK uses for the
 * other orders and fit the same rule (VYUY 0x2d follows from it).
 */
#define JPGE_R004_PKG(y0, y1, cb, cr) \
	((y0) | (y1) << 2 | (cb) << 4 | (cr) << 6)
#define JPGE_R004_PKG_YUYV		JPGE_R004_PKG(0, 2, 1, 3)
#define JPGE_R004_PKG_UYVY		JPGE_R004_PKG(1, 3, 0, 2)
#define JPGE_R004_PKG_YVYU		JPGE_R004_PKG(0, 2, 3, 1)
#define JPGE_R004_PKG_VYUY		JPGE_R004_PKG(1, 3, 2, 0)

#endif /* HISI_JPGE_REGS_H */
