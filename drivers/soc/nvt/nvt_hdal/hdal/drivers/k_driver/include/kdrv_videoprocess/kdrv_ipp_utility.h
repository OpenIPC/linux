/**
    Public header file for KDRV_IPP_utility

    This file is the header file that define data type for KDRV_IPP.

    @file       kdrv_ipp_utility.h
    @ingroup    mILibIPLCom
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/
#ifndef _KDRV_IPP_UTILITY_H_
#define _KDRV_IPP_UTILITY_H_

#include "kwrap/type.h"

typedef INT32(*KDRV_IPP_ISR_CB)(UINT32, UINT32, void *, void *);

typedef enum {
	KDRV_IPP_OPMODE_D2D = 0,	///< Engine D2D
	KDRV_IPP_OPMODE_IFE2IME,	///< IFE -> DCE -> IPE -> IME
	KDRV_IPP_OPMODE_DCE2IME, 	///< DCE -> IPE -> IME
	KDRV_IPP_OPMODE_SIE2IME,	///< SIE -> IFE -> DCE -> IPE -> IME
	KDRV_IPP_OPMODE_MAX
} KDRV_IPP_OPMODE;

typedef enum {
	KDRV_IPP_ENG_IFE = 0,
	KDRV_IPP_ENG_DCE,
	KDRV_IPP_ENG_IPE,
	KDRV_IPP_ENG_IME,
	KDRV_IPP_ENG_MAX,
} KDRV_IPP_ENG;

/* reference to VDO_PXLFMT */
/* maximum plane number */
#define KDRV_IPP_PLANE 4

/* index of video yuv plane */
#define KDRV_IPP_PINDEX_Y		0
#define KDRV_IPP_PINDEX_U		1
#define KDRV_IPP_PINDEX_V		2
#define KDRV_IPP_PINDEX_UV		1
#define KDRV_IPP_PINDEX_YUV 	0
/* index of video raw plane */
#define KDRV_IPP_PINDEX_RAW		0
#define KDRV_IPP_PINDEX_RAW2 	1
#define KDRV_IPP_PINDEX_RAW3 	2
#define KDRV_IPP_PINDEX_RAW4 	3

/* mask of KDRV_IPP_FMT */
#define KDRV_IPP_FMT_CLASS_MASK		0xf0000000
#define KDRV_IPP_FMT_PLANE_MASK		0x0f000000
#define KDRV_IPP_FMT_BPP_MASK		0x00ff0000
#define KDRV_IPP_FMT_PIX_MASK		0x0000ffff

/* class of KDRV_IPP_FMT */
#define KDRV_IPP_FMT_CLASS_ICON 	0x0
#define KDRV_IPP_FMT_CLASS_I 		0x1
#define KDRV_IPP_FMT_CLASS_ARGB 	0x2
#define KDRV_IPP_FMT_CLASS_RLE 		0x3
#define KDRV_IPP_FMT_CLASS_RAW 		0x4
#define KDRV_IPP_FMT_CLASS_YUV 		0x5
#define KDRV_IPP_FMT_CLASS_NVX 		0x6
#define KDRV_IPP_FMT_CLASS_NRX 		0xf
#define KDRV_IPP_FMT_CLASS(pxlfmt) 	(((pxlfmt) & KDRV_IPP_FMT_CLASS_MASK) >> 28)

/* plane of KDRV_IPP_FMT */
#define KDRV_IPP_FMT_PLANE(pxlfmt) 	(((pxlfmt) & KDRV_IPP_FMT_PLANE_MASK) >> 24)

/* bpp of KDRV_IPP_FMT */
#define KDRV_IPP_FMT_BPP(pxlfmt) 	(((pxlfmt) & KDRV_IPP_FMT_BPP_MASK) >> 16)

/* pixel arrange of VDO_PXLFMT */
#define KDRV_IPP_FMT_SEQ_MASK			0xf000
#define KDRV_IPP_FMT_BAYER_PIX_MASK		0x0fff
#define KDRV_IPP_FMT_BAYER_TYPE_MASK	0x0f00
#define KDRV_IPP_FMT_RGGB				0x0100
#define KDRV_IPP_FMT_RGGB_R 			0x0100
#define KDRV_IPP_FMT_RGGB_GR 			0x0101
#define KDRV_IPP_FMT_RGGB_GB 			0x0102
#define KDRV_IPP_FMT_RGGB_B 			0x0103
#define KDRV_IPP_FMT_RCCB 				0x0200
#define KDRV_IPP_FMT_RCCB_RC 			0x0200
#define KDRV_IPP_FMT_RCCB_CR 			0x0201
#define KDRV_IPP_FMT_RCCB_CB 			0x0202
#define KDRV_IPP_FMT_RCCB_BC 			0x0203
#define KDRV_IPP_FMT_RGBIR 				0x0300
#define KDRV_IPP_FMT_RGBIR_RIR 			0x0300
#define KDRV_IPP_FMT_RGBIR_RG 			0x0301
#define KDRV_IPP_FMT_RGBIR44 			0x0400
#define KDRV_IPP_FMT_RGBIR44_RGBG_GIGI 	0x0400
#define KDRV_IPP_FMT_RGBIR44_GBGR_IGIG 	0x0401
#define KDRV_IPP_FMT_RGBIR44_GIGI_BGRG 	0x0402
#define KDRV_IPP_FMT_RGBIR44_IGIG_GRGB 	0x0403
#define KDRV_IPP_FMT_RGBIR44_BGRG_GIGI 	0x0404
#define KDRV_IPP_FMT_RGBIR44_GRGB_IGIG 	0x0405
#define KDRV_IPP_FMT_RGBIR44_GIGI_RGBG 	0x0406
#define KDRV_IPP_FMT_RGBIR44_IGIG_GBGR 	0x0407
#define KDRV_IPP_FMT_PACK_LSB 			0x0800
#define KDRV_IPP_FMT_YCC_MASK			0xf000
#define KDRV_IPP_FMT_YCC_FULL			0x0000
#define KDRV_IPP_FMT_YCC_BT601			0x1000
#define KDRV_IPP_FMT_YCC_BT709			0x2000
#define KDRV_IPP_FMT_PIX(pxlfmt) ((pxlfmt) & KDRV_IPP_FMT_PIX_MASK)
#define KDRV_IPP_FMT_STPX(pxlfmt) ((pxlfmt) & (KDRV_IPP_FMT_BAYER_PIX_MASK & ~(KDRV_IPP_FMT_PACK_LSB)))

typedef enum {
	/* 2 = osd argb format */
	KDRV_IPP_FMT_RGB888_PLANAR	= 0x23180888, ///< 3 plane, pixel=R(w,h), G(w,h), B(w,h)
	KDRV_IPP_FMT_RGB888			= 0x21180888, ///< 1 plane, pixel=RGB(w,h)
	KDRV_IPP_FMT_RGB565			= 0x21100565, ///< 1 plane, pixel=RGB(2w,h)
	KDRV_IPP_FMT_RGBA5551		= 0x21105551, ///< 1 plane, pixel=ARGB(2w,h)
	KDRV_IPP_FMT_ARGB1555		= 0x21101555, ///< 1 plane, pixel=ARGB(2w,h)
	KDRV_IPP_FMT_ARGB4444		= 0x21104444, ///< 1 plane, pixel=ARGB(2w,h)
	KDRV_IPP_FMT_A4				= 0x21044000, ///< 1 plane, pixel=A(w,h)
	KDRV_IPP_FMT_ARGB4565		= 0x22404565, ///< 2 plane, pixel=A4(w,h),RGB565(2w,h)
	KDRV_IPP_FMT_A8				= 0x21088000, ///< 1 plane, pixel=A(w,h)
	KDRV_IPP_FMT_ARGB8565		= 0x22188565, ///< 2 plane, pixel=A8(w,h),RGB565(2w,h)
	KDRV_IPP_FMT_ARGB8888		= 0x21208888, ///< 1 plane, pixel=ARGB(4w,h)
	/* 4 = video raw format */
	KDRV_IPP_FMT_RAW			= 0x41000000, ///< raw general
	KDRV_IPP_FMT_RAW8			= 0x41080000, ///< 1 plane, pixel=RAW(w,h) x 8bits
	KDRV_IPP_FMT_RAW10			= 0x410a0000, ///< 1 plane, pixel=RAW(w,h) x 10bits
	KDRV_IPP_FMT_RAW12			= 0x410c0000, ///< 1 plane, pixel=RAW(w,h) x 12bits
	KDRV_IPP_FMT_RAW14			= 0x410e0000, ///< 1 plane, pixel=RAW(w,h) x 14bits
	KDRV_IPP_FMT_RAW16			= 0x41100000, ///< 1 plane, pixel=RAW(w,h) x 16bits
	KDRV_IPP_FMT_RAW8_SHDR2		= 0x42080000, ///< 2 plane, pixel=RAW(w,h) x 8bits
	KDRV_IPP_FMT_RAW10_SHDR2	= 0x420a0000, ///< 2 plane, pixel=RAW(w,h) x 10bits
	KDRV_IPP_FMT_RAW12_SHDR2	= 0x420c0000, ///< 2 plane, pixel=RAW(w,h) x 12bits
	KDRV_IPP_FMT_RAW14_SHDR2	= 0x420e0000, ///< 2 plane, pixel=RAW(w,h) x 14bits
	KDRV_IPP_FMT_RAW16_SHDR2	= 0x42100000, ///< 2 plane, pixel=RAW(w,h) x 16bits
	KDRV_IPP_FMT_RAW8_SHDR3		= 0x43080000, ///< 3 plane, pixel=RAW(w,h) x 8bits
	KDRV_IPP_FMT_RAW10_SHDR3	= 0x430a0000, ///< 3 plane, pixel=RAW(w,h) x 10bits
	KDRV_IPP_FMT_RAW12_SHDR3	= 0x430c0000, ///< 3 plane, pixel=RAW(w,h) x 12bits
	KDRV_IPP_FMT_RAW14_SHDR3	= 0x430e0000, ///< 3 plane, pixel=RAW(w,h) x 14bits
	KDRV_IPP_FMT_RAW16_SHDR3	= 0x43100000, ///< 3 plane, pixel=RAW(w,h) x 16bits
	KDRV_IPP_FMT_RAW8_SHDR4		= 0x44080000, ///< 4 plane, pixel=RAW(w,h) x 8bits
	KDRV_IPP_FMT_RAW10_SHDR4	= 0x440a0000, ///< 4 plane, pixel=RAW(w,h) x 10bits
	KDRV_IPP_FMT_RAW12_SHDR4	= 0x440c0000, ///< 4 plane, pixel=RAW(w,h) x 12bits
	KDRV_IPP_FMT_RAW14_SHDR4	= 0x440e0000, ///< 4 plane, pixel=RAW(w,h) x 14bits
	KDRV_IPP_FMT_RAW16_SHDR4	= 0x44100000, ///< 4 plane, pixel=RAW(w,h) x 16bits
	/* 5 = video yuv format */
	KDRV_IPP_FMT_Y8					= 0x51080400, ///< 1 plane, pixel=Y(w,h)
	KDRV_IPP_FMT_YUV400				= KDRV_IPP_FMT_Y8,
	KDRV_IPP_FMT_YUV420_PLANAR		= 0x530c0420, ///< 3 plane, pixel=Y(w,h), U(w/2,h/2), and V(w/2,h/2)
	KDRV_IPP_FMT_YUV420				= 0x520c0420, ///< 2 plane, pixel=Y(w,h), UV(w/2,h/2)
	KDRV_IPP_FMT_YUV422_PLANAR		= 0x53100422, ///< 3 plane, pixel=Y(w,h), U(w/2,h), and V(w/2,h)
	KDRV_IPP_FMT_YUV422				= 0x52100422, ///< 2 plane, pixel=Y(w,h), UV(w/2,h)
	KDRV_IPP_FMT_YUV422_ONE			= 0x51100422, ///< 1 plane, pixel=Y(w,h), UV(w/2,h)
	KDRV_IPP_FMT_YUV422_UYVY		= KDRV_IPP_FMT_YUV422_ONE, ///< 1 plane, pixel=UYVY(w,h), packed format with Y2U1V1
	KDRV_IPP_FMT_YUV422_VYUY		= 0x51101422, ///< 1 plane, pixel=VYUY(w,h), packed format with Y2U1V1
	KDRV_IPP_FMT_YUV422_YUYV		= 0x51102422, ///< 1 plane, pixel=YUYV(w,h), packed format with Y2U1V1
	KDRV_IPP_FMT_YUV422_YVYU		= 0x51103422, ///< 1 plane, pixel=YVYU(w,h), packed format with Y2U1V1
	KDRV_IPP_FMT_YUV444_PLANAR		= 0x53180444, ///< 3 plane, pixel=Y(w,h), U(w,h), and V(w,h)
	KDRV_IPP_FMT_YUV444				= 0x52180444, ///< 2 plane, pixel=Y(w,h), UV(w,h)
	KDRV_IPP_FMT_YUV444_ONE			= 0x51180444, ///< 1 plane, pixel=YUV(w,h)
	/* 6 = video yuv compress format */
	KDRV_IPP_FMT_YUV420_NVX			= 0x61000000, ///< novatek-yuv-compress general
	KDRV_IPP_FMT_YUV420_NVX1		= 0x610c1000, ///< novatek-yuv-compress-1 of YUV420 (using by NA51000)
	KDRV_IPP_FMT_YUV420_NVX1_H264	= 0x610c0420, ///< novatek-yuv-compress-1 of YUV420 for h264 (using by NA51000)
	KDRV_IPP_FMT_YUV420_NVX1_H265 	= 0x610c1420, ///< novatek-yuv-compress-1 of YUV420 for h265 (using by NA51000)
	KDRV_IPP_FMT_YUV420_NVX2		= 0x610c2420, ///< novatek-yuv-compress-2 of YUV420 (using by NA51023)
	/* e = video extend format */
	KDRV_IPP_FMT_520_LCA 		= 0xe5200000,
	KDRV_IPP_FMT_520_VA 		= 0xe5200001,
	KDRV_IPP_FMT_520_WDR 		= 0xe5200002,
	KDRV_IPP_FMT_520_DEFOG 		= 0xe5200003,
	KDRV_IPP_FMT_520_3DNR_MS 	= 0xe5200010,
	KDRV_IPP_FMT_520_3DNR_MV 	= 0xe5200011,
	KDRV_IPP_FMT_520_3DNR_MD 	= 0xe5200012,
	KDRV_IPP_FMT_520_3DNR_STA 	= 0xe5200013,
	KDRV_IPP_FMT_520_IR8 		= 0xe5200020,
	KDRV_IPP_FMT_520_IR16 		= 0xe5200021,
	/* f = video raw compress format */
	KDRV_IPP_FMT_NRX			= 0xf1000000, ///< novatek-raw-compress general
	KDRV_IPP_FMT_NRX8			= 0xf1080000, ///< novatek-raw-compress-1 of RAW8
	KDRV_IPP_FMT_NRX10			= 0xf10a0000, ///< novatek-raw-compress-1 of RAW10
	KDRV_IPP_FMT_NRX12			= 0xf10c0000, ///< novatek-raw-compress-1 of RAW12
	KDRV_IPP_FMT_NRX14			= 0xf10e0000, ///< novatek-raw-compress-1 of RAW14
	KDRV_IPP_FMT_NRX16			= 0xf1100000, ///< novatek-raw-compress-1 of RAW16 (using by NA51000 and NA51023)
	KDRV_IPP_FMT_NRX8_SHDR2		= 0xf2080000, ///< novatek-raw-compress-1 of RAW8
	KDRV_IPP_FMT_NRX10_SHDR2	= 0xf20a0000, ///< novatek-raw-compress-1 of RAW10
	KDRV_IPP_FMT_NRX12_SHDR2	= 0xf20c0000, ///< novatek-raw-compress-1 of RAW12
	KDRV_IPP_FMT_NRX14_SHDR2	= 0xf20e0000, ///< novatek-raw-compress-1 of RAW14
	KDRV_IPP_FMT_NRX16_SHDR2	= 0xf2100000, ///< novatek-raw-compress-1 of RAW16 (using by NA51000 and NA51023)
	KDRV_IPP_FMT_NRX8_SHDR3		= 0xf3080000, ///< novatek-raw-compress-1 of RAW8
	KDRV_IPP_FMT_NRX10_SHDR3	= 0xf30a0000, ///< novatek-raw-compress-1 of RAW10
	KDRV_IPP_FMT_NRX12_SHDR3	= 0xf30c0000, ///< novatek-raw-compress-1 of RAW12
	KDRV_IPP_FMT_NRX14_SHDR3	= 0xf30e0000, ///< novatek-raw-compress-1 of RAW14
	KDRV_IPP_FMT_NRX16_SHDR3	= 0xf3100000, ///< novatek-raw-compress-1 of RAW16 (using by NA51000 and NA51023)
	KDRV_IPP_FMT_NRX8_SHDR4		= 0xf4080000, ///< novatek-raw-compress-1 of RAW8
	KDRV_IPP_FMT_NRX10_SHDR4	= 0xf40a0000, ///< novatek-raw-compress-1 of RAW10
	KDRV_IPP_FMT_NRX12_SHDR4	= 0xf40c0000, ///< novatek-raw-compress-1 of RAW12
	KDRV_IPP_FMT_NRX14_SHDR4	= 0xf40e0000, ///< novatek-raw-compress-1 of RAW14
	KDRV_IPP_FMT_NRX16_SHDR4	= 0xf4100000, ///< novatek-raw-compress-1 of RAW16 (using by NA51000 and NA51023)
} KDRV_IPP_FMT;

/* generate KDRV_IPP_FMT of RAW, NRX */
#define KDRV_IPP_FMT_MAKE_RAW(bpp, plane, stpx) ((KDRV_IPP_FMT_CLASS_RAW << 28) | ((plane)<<24) | (((bpp) << 16) & KDRV_IPP_FMT_BPP_MASK) | ((stpx) & KDRV_IPP_FMT_PIX_MASK))
#define KDRV_IPP_FMT_MAKE_NRX(bpp, plane, stpx) ((KDRV_IPP_FMT_CLASS_NRX << 28) | ((plane)<<24) | (((bpp) << 16) & KDRV_IPP_FMT_BPP_MASK) | ((stpx) & KDRV_IPP_FMT_PIX_MASK))

typedef struct {
	KDRV_IPP_FMT fmt;
	USIZE size;
	UINT32 addr[KDRV_IPP_PLANE];
	UINT32 lofs[KDRV_IPP_PLANE];
} KDRV_IPP_FRAME;


#endif
