/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * HiSilicon JPEG decoder (JPGD) register map
 *
 * No data sheet has a register table for this block. Offsets, bit positions
 * and table formats come from the vendor user-space library and kernel shim
 * (Hi3520D V100 and Hi3536D V100 versions, which agree) and from decoding
 * on a Hi3520D V100.
 */
#ifndef HISI_JPGD_REGS_H
#define HISI_JPGD_REGS_H

#include <linux/bits.h>

#define JPGD_START		0x000
#define  JPGD_START_GO		BIT(0)
#define  JPGD_START_BUSY	BIT(1)	/* polled while the reset is pulsed */
#define JPGD_RESUME		0x004
#define  JPGD_RESUME_LAST	BIT(1)	/* this segment ends the stream */
#define JPGD_STRIDE		0x00c	/* [15:0] luma, [31:16] chroma */
#define JPGD_PICSIZE		0x010	/* [15:0] width, [31:16] height */
#define JPGD_PICTYPE		0x014
#define JPGD_STADDR		0x020	/* stream window start */
#define JPGD_ENDADDR		0x024	/* stream window end */
#define JPGD_STADD		0x028	/* first entropy-coded byte */
#define JPGD_ENDADD		0x02c	/* end of the data */
#define JPGD_YSTADDR		0x030
#define JPGD_UVSTADDR		0x034
#define JPGD_SCALE		0x040
#define  JPGD_SCALE_SHIFT_MASK	GENMASK(1, 0)	/* 0..3: 1/1 to 1/8 */
/*
 * Bit 2 has no known meaning, but the Hi3536D block never finishes a job
 * with it clear (the job ends in the driver's watchdog, no interrupt; measured
 * with SCALE 0x30 and 0x00, the reset value 0x34 and every value with bit 2
 * set work). The Hi3520D reset value (0xb0) and its vendor script have it clear.
 */
#define  JPGD_SCALE_BIT2	BIT(2)
#define  JPGD_SCALE_EMAR_EN	BIT(3)		/* set by the vendor library */
#define  JPGD_SCALE_OUTSTD	GENMASK(6, 4)	/* AXI outstanding requests */
#define JPGD_OUTTYPE		0x044	/* 0: native sampling */
#define  JPGD_OUTTYPE_420SP	BIT(2)	/* write 4:2:0 semi-planar whatever the sampling */
#define JPGD_INT		0x100
#define  JPGD_INT_DONE		BIT(0)
#define  JPGD_INT_ERR		BIT(1)
#define  JPGD_INT_MORE		BIT(2)	/* stream consumed, more data needed */
#define  JPGD_INT_ALL		(JPGD_INT_DONE | JPGD_INT_ERR | JPGD_INT_MORE)
/*
 * Bit 4 is the time-out of TIMEOUT (0x108): with the register set to 0x10 a
 * job ends within milliseconds with INT 0x10 and no other bit (measured on
 * the Hi3536D; values from 0x100 up never fired on any picture tried, 0x80 and
 * below fired on every one, and the register resets to all ones, so it does
 * not fire with the reset value). Bit 3 was never seen. The vendor interrupt
 * handlers write the whole INT value back, so acknowledging both is safe.
 */
#define  JPGD_INT_TIMEOUT	BIT(4)
#define  JPGD_INT_AUX		(BIT(3) | JPGD_INT_TIMEOUT)
#define  JPGD_INT_CAUSES	(JPGD_INT_ALL | JPGD_INT_AUX)
#define JPGD_INTMASK		0x104	/* 0 = cause enabled */
#define JPGD_TIMEOUT		0x108
#define JPGD_SAMPLING		0x124	/* Y << 16 | U << 8 | V, (H << 4 | V) */
#define JPGD_DRI		0x128

/* Table memories */
#define JPGD_QUANT		0x200	/* 64 words: Y | U << 8 | V << 16 */
#define JPGD_QUANT_WORDS	64
#define JPGD_HDC		0x300	/* 12 words */
#define JPGD_HDC_WORDS		12
#define JPGD_HACMIN		0x340	/* 8 words */
#define JPGD_HACBASE		0x360	/* 8 words */
#define JPGD_HAC_WORDS		8
#define JPGD_HACSYM		0x400	/* 176 words */
#define JPGD_HACSYM_WORDS	176

/* PICTYPE codes */
#define JPGD_PICTYPE_400	0
#define JPGD_PICTYPE_420	3
#define JPGD_PICTYPE_422H	4	/* chroma halved horizontally (2x1) */
#define JPGD_PICTYPE_444	6

#endif /* HISI_JPGD_REGS_H */
