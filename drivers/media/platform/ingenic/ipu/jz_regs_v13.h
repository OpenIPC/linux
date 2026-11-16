/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Ingenic T23 IPU v13 register definitions
 * Copyright (c) 2015 Ingenic Semiconductor Co., Ltd.
 */

#ifndef _JZ_REGS_V13_H_
#define _JZ_REGS_V13_H_

#include <linux/bitops.h>
#include <linux/videodev2.h>

#ifdef __KERNEL__

/* Module for CLKGR */
#define IPU_CLOCK		BIT(13)

/* Register offset */
#define IPU_FM_CTRL		0x00
#define IPU_STATUS		0x04
#define IPU_D_FMT		0x08
#define IPU_Y_ADDR		0x0c
#define IPU_U_ADDR		0x10
#define IPU_IN_FM_GS		0x18
#define IPU_Y_STRIDE		0x1c
#define IPU_UV_STRIDE		0x20

#define IPU_OUT_ADDR		0x24
#define IPU_OUT_STRIDE		0x2c
#define IPU_OUT_V_ADDR		0x30
#define IPU_OUT_V_STRIDE	0x34

#define IPU_REG_CTRL		0x64
#define IPU_TRIGGER		0x74
#define IPU_GLB_CTRL		0x7c

#define IPU_NV_OUT_ADDR		0xb4
#define IPU_NV_OUT_STRIDE	0xb8
#define IPU_OSD_IN_CH0_Y_ADDR	0xbc
#define IPU_OSD_IN_CH0_Y_STRIDE	0xc0
#define IPU_OSD_IN_CH0_UV_ADDR	0xc4
#define IPU_OSD_IN_CH0_UV_STRIDE 0xc8
#define IPU_OSD_CH0_GS		0xfc
#define IPU_OSD_CH0_POS		0x10c
#define IPU_OSD_CH0_PARA	0x11c
#define IPU_OSD_CH1_PARA	0x120
#define IPU_OSD_CH2_PARA	0x124
#define IPU_OSD_CH3_PARA	0x128
#define IPU_OSD_CH_BK_PARA	0x12c

#define IPU_OSD_CH0_BAK_ARGB	0x130

#define IPU_CH0_CSC_C0_COEF	0x148
#define IPU_CH0_CSC_C1_COEF	0x14c
#define IPU_CH0_CSC_C2_COEF	0x150
#define IPU_CH0_CSC_C3_COEF	0x154
#define IPU_CH0_CSC_C4_COEF	0x158
#define IPU_CH0_CSC_OFSET_PARA	0x15c

#define IPU_RD_ARB_CTL		0x1a8
#define IPU_CLK_NUM_ONE_FRA	0x1ac
#define IPU_OSD_LAY_PADDING_ARGB 0x1b0
#define IPU_TEST_1B4		0x1b4

/* SET CH_BK_PARA Register */
#define CH_BK_EN		BIT(0)
#define CH_BK_PIXEL_ALPHA	(0 << 2) /* default */
#define CH_BK_GLOBAL_ALPHA	BIT(1)
#define CH_BK_PALPHA_X_GALPHA	BIT(2)
#define CH_BK_GLO_A(val)	((val) << 3)

#define CH_BK_PIC_TYPE		11
#define CH_BK_PIC_TYPE_ARGB	(0 << CH_BK_PIC_TYPE)
#define CH_BK_PIC_TYPE_RGB	(1 << CH_BK_PIC_TYPE)
#define CH_BK_PIC_TYPE_NV12	(2 << CH_BK_PIC_TYPE)
#define CH_BK_PIC_TYPE_NV21	(3 << CH_BK_PIC_TYPE)

#define CH_BK_ARGB_TYPE		14
#define CH_BK_ARGB_TYPE_ARGB	(0x0 << CH_BK_ARGB_TYPE)
#define CH_BK_ARGB_TYPE_ARBG	(0x1 << CH_BK_ARGB_TYPE)
#define CH_BK_ARGB_TYPE_AGRB	(0x2 << CH_BK_ARGB_TYPE)
#define CH_BK_ARGB_TYPE_AGBR	(0x3 << CH_BK_ARGB_TYPE)
#define CH_BK_ARGB_TYPE_ABRG	(0x4 << CH_BK_ARGB_TYPE)
#define CH_BK_ARGB_TYPE_ABGR	(0x5 << CH_BK_ARGB_TYPE)
#define CH_BK_ARGB_TYPE_RGBA	(0x8 << CH_BK_ARGB_TYPE)
#define CH_BK_ARGB_TYPE_RBGA	(0x9 << CH_BK_ARGB_TYPE)
#define CH_BK_ARGB_TYPE_GRBA	(0xa << CH_BK_ARGB_TYPE)
#define CH_BK_ARGB_TYPE_GBRA	(0xb << CH_BK_ARGB_TYPE)
#define CH_BK_ARGB_TYPE_BRGA	(0xc << CH_BK_ARGB_TYPE)
#define CH_BK_ARGB_TYPE_BGRA	(0xd << CH_BK_ARGB_TYPE)

#define CH_BK_PREM		BIT(18)
#define CH_BK_MASK_EN		BIT(19)
#define CH_BK_WHOLE_PIC_EN	BIT(20)

/* OSD Parameters */
#define IPU_OSD_CH0_PARA_DEFAULT	0x010400fa /* stock default: BIT(24) | CH_BK_PREM | CH_BK_GLO_A(0x1f) | CH_BK_GLOBAL_ALPHA */
#define IPU_OSD_CH_BK_PARA_DEFAULT	(CH_BK_WHOLE_PIC_EN | CH_BK_PREM | CH_BK_PIC_TYPE_NV12) /* 0x141000 */

/* OSD_CHN_EN */
#define CHOOSE_OSD_CHN_RESIZE	BIT(5)

/* IPU_GLB_CTRL Register */
#define IRQ_EN			BIT(0)

/* IPU_OSD_CTRL Register */
#define OSD_PM			BIT(2)

/* IPU_TRIG Register */
#define IPU_RUN			BIT(0)
#define IPU_STOP		BIT(1)
#define IPU_RESET		BIT(3)

/* IPU_D_FMT Register */
#define BLK_SEL			BIT(4)
#define RGB_POS			BIT(5)
#define AL_PIX_EN		BIT(6)

#define IPU_D_FMT_IN_YUV420	0x01
#define IPU_D_FMT_OUT_ARGB8888	0x00
#define IPU_D_FMT_OUT_NV21	0x03
#define IPU_D_FMT_OUT_ABGR8888	0x28
#define IPU_D_FMT_OUT_RGBA8888	0x40
#define IPU_D_FMT_OUT_BGRA8888	0x68

/* IPU_STATUS bitfields */
#define OUT_END			BIT(0)
#define SIZE_ERR		BIT(2)
#define ID			BIT(16)

#define MSTATUS_SFT		4
#define MSTATUS_MSK		3
#define MSTATUS_IPU_FREE	(0 << MSTATUS_SFT)
#define MSTATUS_IPU_RUNNING	(1 << MSTATUS_SFT)
#define MSTATUS			(2 << MSTATUS_SFT)

/* REG_IPU_ADDR_CTRL */
#define Y_RY			BIT(0)
#define U_RY			BIT(1)
#define V_RY			BIT(2)
#define D_RY			BIT(3)
#define PTS_RY			BIT(4)
#define CTRL_RY			BIT(6)
#define DF_RY			BIT(7)
#define NV_D_RY			BIT(17)
#define NV_STR_RY		BIT(20)
#define OSD_CH0_RY		BIT(21)
#define OSD_CH1_RY		BIT(22)
#define OSD_CH2_RY		BIT(23)
#define OSD_CH3_RY		BIT(24)
#define RD_ARB_RY		BIT(26)
#define OSD_BK_RY		BIT(27)

/* IPU_OSD_CTRL bitfields */
#define GLB_ALPHA(val)		((val) << 8)
#define MOD_OSD(val)		((val) << 0)

/* Frame XY offset bitfields */
#define SCREEN_YOFT(val)	((val) << 16)
#define SCREEN_XOFT(val)	((val) << 0)

/* Input geometry bitfields */
#define IN_FM_W(val)		((val) << 16)
#define IN_FM_H(val)		((val) << 0)

/* Output geometry bitfields */
#define OUT_FM_W(val)		((val) << 16)
#define OUT_FM_H(val)		((val) << 0)

/* UV stride bitfields */
#define U_STRIDE(val)		((val) << 16)
#define V_STRIDE(val)		((val) << 0)

/* BT.601 Color Space Conversion Matrix */
#define IPU_CSC_C0_BT601	0x4ad
#define IPU_CSC_C1_BT601	(0x669 | (0x4ad << 20))
#define IPU_CSC_C2_BT601	(0x193 | (0x344 << 20))
#define IPU_CSC_C3_BT601	(0x4ad | (0x81a << 20))
#define IPU_CSC_C4_BT601	0x00
#define IPU_CSC_OFSET_BT601	(0x10 | (0x80 << 16))

/* Legacy matrix aliases */
#define YUV_CSC_C0		IPU_CSC_C0_BT601
#define YUV_CSC_C1		IPU_CSC_C1_BT601
#define YUV_CSC_C2		IPU_CSC_C2_BT601
#define YUV_CSC_C3		IPU_CSC_C3_BT601
#define YUV_CSC_C4		IPU_CSC_C4_BT601
#define YUV_CSC_OFFSET_PARA	IPU_CSC_OFSET_BT601

/* Channel select */
#define BK_OSD_PIC_MASK		18
#define BK_OSD_PIC_NV21		(1 << BK_OSD_PIC_MASK)
#define BK_OSD_PIC_NV12		(2 << BK_OSD_PIC_MASK)
#define BK_OSD_PIC_ARGB		(3 << BK_OSD_PIC_MASK)
#define CH3_OSD_PIC_MASK	15
#define CH3_OSD_PIC_NV21	(1 << CH3_OSD_PIC_MASK)
#define CH3_OSD_PIC_NV12	(2 << CH3_OSD_PIC_MASK)
#define CH3_OSD_PIC_ARGB	(3 << CH3_OSD_PIC_MASK)
#define CH2_OSD_PIC_MASK	12
#define CH2_OSD_PIC_NV21	(1 << CH2_OSD_PIC_MASK)
#define CH2_OSD_PIC_NV12	(2 << CH2_OSD_PIC_MASK)
#define CH2_OSD_PIC_ARGB	(3 << CH2_OSD_PIC_MASK)
#define CH1_OSD_PIC_MASK	9
#define CH1_OSD_PIC_NV21	(1 << CH1_OSD_PIC_MASK)
#define CH1_OSD_PIC_NV12	(2 << CH1_OSD_PIC_MASK)
#define CH1_OSD_PIC_ARGB	(3 << CH1_OSD_PIC_MASK)
#define CH0_OSD_PIC_MASK	6
#define CH0_OSD_PIC_NV21	(1 << CH0_OSD_PIC_MASK)
#define CH0_OSD_PIC_NV12	(2 << CH0_OSD_PIC_MASK)
#define CH0_OSD_PIC_ARGB	(3 << CH0_OSD_PIC_MASK)
#define OSD_CHN_EN_MASK		0
#define OSD_CHN_EN_CH0		(1 << OSD_CHN_EN_MASK)
#define OSD_CHN_EN_CH1		(2 << OSD_CHN_EN_MASK)
#define OSD_CHN_EN_CH2		(4 << OSD_CHN_EN_MASK)
#define OSD_CHN_EN_CH3		(8 << OSD_CHN_EN_MASK)
#define OSD_CHN_EN_BKG		(16 << OSD_CHN_EN_MASK)
#define OSD_CHN_EN_DST		(32 << OSD_CHN_EN_MASK)

#endif /* #ifdef __KERNEL__ */

/* Data Format Register */
#define OUT_FMT_RGB555		0
#define OUT_FMT_RGB565		1
#define OUT_FMT_RGB888		2
#define OUT_FMT_YUV422		3
#define OUT_FMT_RGBAAA		4
#define OUT_FMT_NV12		5
#define OUT_FMT_NV21		6
#define OUT_FMT_HSV		7
#define OUT_FMT_ARGB_8888	8
#define OUT_FMT_ABGR_8888	9
#define OUT_FMT_RGBA_8888	0xa
#define OUT_FMT_BGRA_8888	0xb

#define IN_FMT_BIT		0
#define IN_FMT_YUV420		1
#define IN_FMT_YUV420_B		2
#define IN_FMT_YUV422		3
#define IN_FMT_YUV444		4
#define IN_FMT_RGB_555		5
#define IN_FMT_RGB_888		6
#define IN_FMT_RGB_565		7
#define IN_FMT_YUV411		8

#define NV_SEL			BIT(7)
#define NV_MODE_NV12		(0 << 8)
#define NV_MODE_NV21		BIT(8)

#endif /* _JZ_REGS_V13_H_ */
