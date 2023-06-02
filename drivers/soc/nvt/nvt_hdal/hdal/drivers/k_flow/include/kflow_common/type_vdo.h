/**
    Video frame related types.

    This module include video frame related types

    @file       type_vdo.h
    @ingroup    mInvtvdo
    @note       Nothing

                IPOINT : Point

                ISIZE : Size

                IRECT : Rectangle

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _TYPE_VDO_H_
#define _TYPE_VDO_H_

#include "kwrap/platform.h"
#include "kwrap/type.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
     make high/low 16bits to 32bits
*/
#define MAKE_UINT16_UINT16(h,l) 	(((((UINT32)(h)) & 0x0000ffff)<<16) | (((UINT32)(l)) & 0x0000ffff))

/**
     get high/low 32bits from 64bits
*/
//@{
#define GET_HI_UINT32(v)			((UINT32)((v) >> 32))
#define GET_LO_UINT32(v)			((UINT32)(v))
//@}

/**
     get high/low 16bits from 32bits
*/
//@{
#define GET_HI_UINT16(v)			((((UINT32)(v)) >> 16) & 0x0000ffff)
#define GET_LO_UINT16(v)			(((UINT32)(v)) & 0x0000ffff)
//@}


#define VDO_DATA_DESC_SIZE  38 ///< size of data desc in WORDS


//------------------------------------------------------------------------------
// metadata structures
//------------------------------------------------------------------------------
typedef struct _VDO_METADATA {
	UINT32 sign;				///< signature = MAKEFOURCC(?,?,?,?)
	struct _VDO_METADATA* p_next;      ///< pointer to next meta
} VDO_METADATA;


//------------------------------------------------------------------------------
// data structures
//------------------------------------------------------------------------------
/**
    @addtogroup mILibImageProcess
*/

/**
     @name video pixel format
*/
typedef enum _VDO_PXLFMT
{
	/* 0 = osd icon index format */
	VDO_PXLFMT_I1_ICON		= 0x01010001, ///< 1 plane, pixel=INDEX(w,h), w/o padding bits
	VDO_PXLFMT_I2_ICON		= 0x01020002, ///< 1 plane, pixel=INDEX(w,h), w/o padding bits
	VDO_PXLFMT_I4_ICON		= 0x01040004, ///< 1 plane, pixel=INDEX(w,h), w/o padding bits
	VDO_PXLFMT_I8_ICON		= 0x01080008, ///< 1 plane, pixel=INDEX(w,h), w/o padding bits
	/* 1 = osd index format */
	VDO_PXLFMT_I1				= 0x11010001, ///< 1 plane, pixel=INDEX(w,h)
	VDO_PXLFMT_I2				= 0x11020002, ///< 1 plane, pixel=INDEX(w,h)
	VDO_PXLFMT_I4				= 0x11040004, ///< 1 plane, pixel=INDEX(w,h)
	VDO_PXLFMT_I8				= 0x11080008, ///< 1 plane, pixel=INDEX(w,h)
	/* 2 = osd argb format */
	VDO_PXLFMT_RGB888_PLANAR	= 0x23180888, ///< 3 plane, pixel=R(w,h), G(w,h), B(w,h)
	VDO_PXLFMT_RGB888			= 0x21180888, ///< 1 plane, pixel=RGB(w,h)
	VDO_PXLFMT_RGB565			= 0x21100565, ///< 1 plane, pixel=RGB(2w,h)
	VDO_PXLFMT_RGBA5551		= 0x21105551, ///< 1 plane, pixel=ARGB(2w,h)
	VDO_PXLFMT_ARGB1555		= 0x21101555, ///< 1 plane, pixel=ARGB(2w,h)
	VDO_PXLFMT_ARGB4444		= 0x21104444, ///< 1 plane, pixel=ARGB(2w,h)
	VDO_PXLFMT_A4				= 0x21044000, ///< 1 plane, pixel=A(w,h)
	VDO_PXLFMT_ARGB4565		= 0x22404565, ///< 2 plane, pixel=A4(w,h),RGB565(2w,h)
	VDO_PXLFMT_A8				= 0x21088000, ///< 1 plane, pixel=A(w,h)
	VDO_PXLFMT_ARGB8565		= 0x22188565, ///< 2 plane, pixel=A8(w,h),RGB565(2w,h)
	VDO_PXLFMT_ARGB8888		= 0x21208888, ///< 1 plane, pixel=ARGB(4w,h)
	/* 3 = osd compressed argb format */
	/* 4 = video raw format */
	VDO_PXLFMT_RAW			= 0x41000000, ///< raw general
	VDO_PXLFMT_RAW8			= 0x41080000, ///< 1 plane, pixel=RAW(w,h) x 8bits
	VDO_PXLFMT_RAW10			= 0x410a0000, ///< 1 plane, pixel=RAW(w,h) x 10bits
	VDO_PXLFMT_RAW12			= 0x410c0000, ///< 1 plane, pixel=RAW(w,h) x 12bits
	VDO_PXLFMT_RAW14			= 0x410e0000, ///< 1 plane, pixel=RAW(w,h) x 14bits
	VDO_PXLFMT_RAW16			= 0x41100000, ///< 1 plane, pixel=RAW(w,h) x 16bits
	VDO_PXLFMT_RAW8_SHDR2		= 0x42080000, ///< 2 plane, pixel=RAW(w,h) x 8bits
	VDO_PXLFMT_RAW10_SHDR2	= 0x420a0000, ///< 2 plane, pixel=RAW(w,h) x 10bits
	VDO_PXLFMT_RAW12_SHDR2	= 0x420c0000, ///< 2 plane, pixel=RAW(w,h) x 12bits
	VDO_PXLFMT_RAW14_SHDR2	= 0x420e0000, ///< 2 plane, pixel=RAW(w,h) x 14bits
	VDO_PXLFMT_RAW16_SHDR2	= 0x42100000, ///< 2 plane, pixel=RAW(w,h) x 16bits
	VDO_PXLFMT_RAW8_SHDR3		= 0x43080000, ///< 3 plane, pixel=RAW(w,h) x 8bits
	VDO_PXLFMT_RAW10_SHDR3	= 0x430a0000, ///< 3 plane, pixel=RAW(w,h) x 10bits
	VDO_PXLFMT_RAW12_SHDR3	= 0x430c0000, ///< 3 plane, pixel=RAW(w,h) x 12bits
	VDO_PXLFMT_RAW14_SHDR3	= 0x430e0000, ///< 3 plane, pixel=RAW(w,h) x 14bits
	VDO_PXLFMT_RAW16_SHDR3	= 0x43100000, ///< 3 plane, pixel=RAW(w,h) x 16bits
	VDO_PXLFMT_RAW8_SHDR4		= 0x44080000, ///< 4 plane, pixel=RAW(w,h) x 8bits
	VDO_PXLFMT_RAW10_SHDR4	= 0x440a0000, ///< 4 plane, pixel=RAW(w,h) x 10bits
	VDO_PXLFMT_RAW12_SHDR4	= 0x440c0000, ///< 4 plane, pixel=RAW(w,h) x 12bits
	VDO_PXLFMT_RAW14_SHDR4	= 0x440e0000, ///< 4 plane, pixel=RAW(w,h) x 14bits
	VDO_PXLFMT_RAW16_SHDR4	= 0x44100000, ///< 4 plane, pixel=RAW(w,h) x 16bits
	/* 5 = video yuv format */
	VDO_PXLFMT_Y8				= 0x51080400, ///< 1 plane, pixel=Y(w,h)
	VDO_PXLFMT_YUV400			= VDO_PXLFMT_Y8,
	VDO_PXLFMT_YUV420_PLANAR	= 0x530c0420, ///< 3 plane, pixel=Y(w,h), U(w/2,h/2), and V(w/2,h/2)
	VDO_PXLFMT_YUV420			= 0x520c0420, ///< 2 plane, pixel=Y(w,h), UV(w/2,h/2)
	VDO_PXLFMT_YUV422_PLANAR	= 0x53100422, ///< 3 plane, pixel=Y(w,h), U(w/2,h), and V(w/2,h)
	VDO_PXLFMT_YUV422			= 0x52100422, ///< 2 plane, pixel=Y(w,h), UV(w/2,h)
	VDO_PXLFMT_YUV422_ONE		= 0x51100422, ///< 1 plane, pixel=Y(w,h), UV(w/2,h)
	VDO_PXLFMT_YUV422_UYVY		= VDO_PXLFMT_YUV422_ONE, ///< 1 plane, pixel=UYVY(w,h), packed format with Y2U1V1
	VDO_PXLFMT_YUV422_VYUY		= 0x51101422, ///< 1 plane, pixel=VYUY(w,h), packed format with Y2U1V1
	VDO_PXLFMT_YUV422_YUYV		= 0x51102422, ///< 1 plane, pixel=YUYV(w,h), packed format with Y2U1V1
	VDO_PXLFMT_YUV422_YVYU		= 0x51103422, ///< 1 plane, pixel=YVYU(w,h), packed format with Y2U1V1
	VDO_PXLFMT_YUV444_PLANAR	= 0x53180444, ///< 3 plane, pixel=Y(w,h), U(w,h), and V(w,h)
	VDO_PXLFMT_YUV444			= 0x52180444, ///< 2 plane, pixel=Y(w,h), UV(w,h)
	VDO_PXLFMT_YUV444_ONE		= 0x51180444, ///< 1 plane, pixel=YUV(w,h)
	/* 6 = video yuv compress format */
	VDO_PXLFMT_YUV420_NVX			= 0x61000000, ///< novatek-yuv-compress general
	VDO_PXLFMT_YUV420_NVX1		= 0x610c1000, ///< novatek-yuv-compress-1 of YUV420 (using by NA51000)
	VDO_PXLFMT_YUV420_NVX1_H264	= 0x610c0420, ///< novatek-yuv-compress-1 of YUV420 for h264 (using by NA51000)
	VDO_PXLFMT_YUV420_NVX1_H265 	= 0x610c1420, ///< novatek-yuv-compress-1 of YUV420 for h265 (using by NA51000)
	VDO_PXLFMT_YUV420_NVX2		= 0x610c2420, ///< novatek-yuv-compress-2 of YUV420 (using by NA51023)
	/* e = video extend format */
	VDO_PXLFMT_520_LCA 			= 0xe5200000,
	VDO_PXLFMT_520_VA 			= 0xe5200001,
	VDO_PXLFMT_520_WDR 			= 0xe5200002,
	VDO_PXLFMT_520_DEFOG 			= 0xe5200003,
	VDO_PXLFMT_520_3DNR_MS 		= 0xe5200010,
	VDO_PXLFMT_520_3DNR_MV 		= 0xe5200011,
	VDO_PXLFMT_520_3DNR_MD 		= 0xe5200012,
	VDO_PXLFMT_520_3DNR_STA 		= 0xe5200013,
	VDO_PXLFMT_520_IR8 			= 0xe5200020,
	VDO_PXLFMT_520_IR16 			= 0xe5200021,
	/* f = video raw compress format */
	VDO_PXLFMT_NRX			= 0xf1000000, ///< novatek-raw-compress general
	VDO_PXLFMT_NRX8			= 0xf1080000, ///< novatek-raw-compress-1 of RAW8
	VDO_PXLFMT_NRX10			= 0xf10a0000, ///< novatek-raw-compress-1 of RAW10
	VDO_PXLFMT_NRX12			= 0xf10c0000, ///< novatek-raw-compress-1 of RAW12
	VDO_PXLFMT_NRX14			= 0xf10e0000, ///< novatek-raw-compress-1 of RAW14
	VDO_PXLFMT_NRX16			= 0xf1100000, ///< novatek-raw-compress-1 of RAW16 (using by NA51000 and NA51023)
	VDO_PXLFMT_NRX8_SHDR2		= 0xf2080000, ///< novatek-raw-compress-1 of RAW8
	VDO_PXLFMT_NRX10_SHDR2	= 0xf20a0000, ///< novatek-raw-compress-1 of RAW10
	VDO_PXLFMT_NRX12_SHDR2	= 0xf20c0000, ///< novatek-raw-compress-1 of RAW12
	VDO_PXLFMT_NRX14_SHDR2	= 0xf20e0000, ///< novatek-raw-compress-1 of RAW14
	VDO_PXLFMT_NRX16_SHDR2	= 0xf2100000, ///< novatek-raw-compress-1 of RAW16 (using by NA51000 and NA51023)
	VDO_PXLFMT_NRX8_SHDR3		= 0xf3080000, ///< novatek-raw-compress-1 of RAW8
	VDO_PXLFMT_NRX10_SHDR3	= 0xf30a0000, ///< novatek-raw-compress-1 of RAW10
	VDO_PXLFMT_NRX12_SHDR3	= 0xf30c0000, ///< novatek-raw-compress-1 of RAW12
	VDO_PXLFMT_NRX14_SHDR3	= 0xf30e0000, ///< novatek-raw-compress-1 of RAW14
	VDO_PXLFMT_NRX16_SHDR3	= 0xf3100000, ///< novatek-raw-compress-1 of RAW16 (using by NA51000 and NA51023)
	VDO_PXLFMT_NRX8_SHDR4		= 0xf4080000, ///< novatek-raw-compress-1 of RAW8
	VDO_PXLFMT_NRX10_SHDR4	= 0xf40a0000, ///< novatek-raw-compress-1 of RAW10
	VDO_PXLFMT_NRX12_SHDR4	= 0xf40c0000, ///< novatek-raw-compress-1 of RAW12
	VDO_PXLFMT_NRX14_SHDR4	= 0xf40e0000, ///< novatek-raw-compress-1 of RAW14
	VDO_PXLFMT_NRX16_SHDR4	= 0xf4100000, ///< novatek-raw-compress-1 of RAW16 (using by NA51000 and NA51023)
} VDO_PXLFMT;

#define VDO_PXLFMT_DEFALUT			VDO_PXLFMT_YUV420

/* mask of VDO_PXLFMT */
#define VDO_PXLFMT_CLASS_MASK		0xf0000000
#define VDO_PXLFMT_PLANE_MASK		0x0f000000
#define VDO_PXLFMT_BPP_MASK		0x00ff0000
#define VDO_PXLFMT_PIX_MASK		0x0000ffff

/* class of VDO_PXLFMT */
#define VDO_PXLFMT_CLASS_ICON 		0x0
#define VDO_PXLFMT_CLASS_I 		0x1
#define VDO_PXLFMT_CLASS_ARGB 		0x2
#define VDO_PXLFMT_CLASS_RLE 		0x3
#define VDO_PXLFMT_CLASS_RAW 		0x4
#define VDO_PXLFMT_CLASS_YUV 		0x5
#define VDO_PXLFMT_CLASS_NVX 		0x6
#define VDO_PXLFMT_CLASS_NRX 		0xf
#define VDO_PXLFMT_CLASS(pxlfmt) 	(((pxlfmt) & VDO_PXLFMT_CLASS_MASK) >> 28)

/* plane of VDO_PXLFMT */
#define VDO_PXLFMT_PLANE(pxlfmt) 	(((pxlfmt) & VDO_PXLFMT_PLANE_MASK) >> 24)

/* bpp of VDO_PXLFMT */
#define VDO_PXLFMT_BPP(pxlfmt) 	(((pxlfmt) & VDO_PXLFMT_BPP_MASK) >> 16)

/* pixel arrange of VDO_PXLFMT */
#define VDO_PIX_SEQ_MASK			0xf000
#define VDO_PIX_BAYER_MASK			0x0fff
#define VDO_PIX_RGGB				0x0100
#define VDO_PIX_RGGB_R 			0x0100
#define VDO_PIX_RGGB_GR 			0x0101
#define VDO_PIX_RGGB_GB 			0x0102
#define VDO_PIX_RGGB_B 			0x0103
#define VDO_PIX_RCCB 				0x0200
#define VDO_PIX_RCCB_RC 			0x0200
#define VDO_PIX_RCCB_CR 			0x0201
#define VDO_PIX_RCCB_CB 			0x0202
#define VDO_PIX_RCCB_BC 			0x0203
#define VDO_PIX_RGBIR 			0x0300
#define VDO_PIX_RGBIR_RIR 			0x0300
#define VDO_PIX_RGBIR_RG 			0x0301
#define VDO_PIX_RGBIR44 			0x0400
#define VDO_PIX_RGBIR44_RGBG_GIGI 	0x0400
#define VDO_PIX_RGBIR44_GBGR_IGIG 	0x0401
#define VDO_PIX_RGBIR44_GIGI_BGRG 	0x0402
#define VDO_PIX_RGBIR44_IGIG_GRGB 	0x0403
#define VDO_PIX_RGBIR44_BGRG_GIGI 	0x0404
#define VDO_PIX_RGBIR44_GRGB_IGIG 	0x0405
#define VDO_PIX_RGBIR44_GIGI_RGBG 	0x0406
#define VDO_PIX_RGBIR44_IGIG_GBGR 	0x0407
#define VDO_PIX_Y                   0x0500
#define VDO_PIX_PACK_LSB			0x0800
#define VDO_PIX_YCC_MASK			0xf000
#define VDO_PIX_YCC_FULL			0x0000
#define VDO_PIX_YCC_BT601			0x1000
#define VDO_PIX_YCC_BT709			0x2000
#define VDO_PXLFMT_PIX(pxlfmt) ((pxlfmt) & VDO_PXLFMT_PIX_MASK)
#define VDO_PXLFMT_BAYER_STPX(pxlfmt)	((pxlfmt) & (VDO_PIX_BAYER_MASK & (~VDO_PIX_PACK_LSB)))

/* type of VDO_PXLFMT */
#define VDO_PXLFMT_TYPE(pxlfmt) (((pxlfmt) & (VDO_PXLFMT_CLASS_MASK | VDO_PXLFMT_PLANE_MASK | VDO_PXLFMT_BPP_MASK))


/*

for RAW and NRX format:

[usage example-2]
switch (VDO_PXLFMT_TYPE(pxlfmt)) {
case VDO_PXLFMT_RAW8:
case VDO_PXLFMT_RAW10:
case VDO_PXLFMT_RAW12:
case VDO_PXLFMT_RAW16:
	{
	    UINT32 stpx = VDO_PXLFMT_PIX(pxlfmt);
	}
	break;
}

[usage example-3]
switch (VDO_PXLFMT_CLASS(pxlfmt)) {
case VDO_PXLFMT_CLASS_RAW:
	{
	    UINT32 bpp = VDO_PXLFMT_BPP(pxlfmt);
	    UINT32 stpx = VDO_PXLFMT_PIX(pxlfmt);
	}
	break;
}

*/


/* generate VDO_PXLFMT of RAW, NRX */
#define VDO_PXLFMT_MAKE_RAW(bpp, plane, stpx) ((VDO_PXLFMT_CLASS_RAW << 28) | ((plane)<<24) | (((bpp) << 16) & VDO_PXLFMT_BPP_MASK) | ((stpx) & VDO_PXLFMT_PIX_MASK))
#define VDO_PXLFMT_MAKE_NRX(bpp, plane, stpx) ((VDO_PXLFMT_CLASS_NRX << 28) | ((plane)<<24) | (((bpp) << 16) & VDO_PXLFMT_BPP_MASK) | ((stpx) & VDO_PXLFMT_PIX_MASK))

/*

for RAW and NRX format:

[gen example-1]
pxlfmt = VDO_PXLFMT_MAKE_RAW(16, 1, VDO_PIX_RGGB_R);
pxlfmt = VDO_PXLFMT_MAKE_NRX(16, 1, VDO_PIX_RGGB_R);

*/



/* maximum plane number */
#define VDO_MAX_PLANE 4

/* index of osd index plane */
#define VDO_PINDEX_I 0
/* index of osd argb plane */
#define VDO_PINDEX_R 0
#define VDO_PINDEX_G 1
#define VDO_PINDEX_B 2
#define VDO_PINDEX_A 3
#define VDO_PINDEX_RGB 0
#define VDO_PINDEX_ARGB 0
/* index of video yuv plane */
#define VDO_PINDEX_Y 0
#define VDO_PINDEX_U 1
#define VDO_PINDEX_V 2
#define VDO_PINDEX_UV 1
#define VDO_PINDEX_YUV 0
/* index of video raw plane */
#define VDO_PINDEX_NVX 0
#define VDO_PINDEX_RAW 0
#define VDO_PINDEX_RAW2 1
#define VDO_PINDEX_RAW3 2
#define VDO_PINDEX_RAW4 3
#define VDO_PINDEX_NRX 0
#define VDO_PINDEX_NRX2 1
#define VDO_PINDEX_NRX3 2
#define VDO_PINDEX_NRX4 3

typedef struct _VDO_COORD {
	IRECT coord;					///< shift and size of virtual coordinate
	IPOINT shift;					///< shift vector (real coordinate)
	IPOINT scale;					///< scale vector (from virtual to real coordinate) (in fixpoint 16.16)
} VDO_COORD, *PVDO_COORD;

typedef struct _VDO_FRAME {
	UINT32 sign;					///< signature = MAKEFOURCC('V','F','R','M')
	VDO_METADATA* p_next;      	///< pointer to next meta
	UINT32 resv1;
	VDO_PXLFMT pxlfmt;			///< pixel format enum
	ISIZE size;					///< size (of plane 0)
	UINT64 count;					///< frame count
	UINT64 timestamp;				///< time stamp
	UINT32 pw[VDO_MAX_PLANE];		///< width of each plane
	UINT32 ph[VDO_MAX_PLANE];		///< height of each plane
	UINT32 loff[VDO_MAX_PLANE];	///< line offset of each plane
	UINT32 addr[VDO_MAX_PLANE];	///< address of each plane
	UINT32 phyaddr[VDO_MAX_PLANE];	///< physical address of each plane
	UINT32 reserved[8];			///< reserved info
} VDO_FRAME, *PVDO_FRAME;
STATIC_ASSERT(sizeof(VDO_FRAME) / sizeof(UINT32) == VDO_DATA_DESC_SIZE);

typedef struct _VDO_NVX1_INFO {
	UINT32 sideinfo_addr;
	UINT32 ktab0;
	UINT32 ktab1;
	UINT32 ktab2;
	UINT32 strp_num;
	UINT32 strp_size_01;
	UINT32 strp_size_23;
	UINT32 reserved;
} VDO_NVX1_INFO, *PVDO_NVX1_INFO;

/**
	low delay mode information
*/
typedef struct _VDO_LOW_DLY_INFO {
	UINT32 strp_num;
	UINT32 strp_size[3];
	UINT32 reserved[4];
} VDO_LOW_DLY_INFO, *PVDO_LOW_DLY_INFO;

/**
	md info
*/
typedef struct _VDO_MD_INFO {
	USIZE src_img_size;
	USIZE md_size;
	UINT32 md_lofs;
	UINT32 reserved[3];
} VDO_MD_INFO;

#define VDO_FRAME_MAKE_RAW(p_vdoframe, w, h, bpp, stpx, addr_raw, loff_raw) \
do { \
	memset((p_vdoframe), 0, sizeof(VDO_FRAME)); \
	(p_vdoframe)->sign = MAKEFOURCC('V','F','R','M'); \
	(p_vdoframe)->pxlfmt = VDO_PXLFMT_MAKE_RAW(bpp, stpx); \
	(p_vdoframe)->size.w = (w); \
	(p_vdoframe)->size.h = (h); \
	(p_vdoframe)->pw[VDO_PINDEX_RAW] = (w); \
	(p_vdoframe)->ph[VDO_PINDEX_RAW] = (h); \
	(p_vdoframe)->loff[VDO_PINDEX_RAW] = (loff_raw); \
	(p_vdoframe)->addr[VDO_PINDEX_RAW] = (addr_raw); \
} while (0)

#define VDO_FRAME_MAKE_NRX(p_vdoframe, w, h, bpp, plane, stpx, addr_nrx, loff_nrx) \
do { \
	memset((p_vdoframe), 0, sizeof(VDO_FRAME)); \
	(p_vdoframe)->sign = MAKEFOURCC('V','F','R','M'); \
	(p_vdoframe)->pxlfmt = VDO_PXLFMT_MAKE_NRX(bpp, plane, stpx); \
	(p_vdoframe)->size.w = (w); \
	(p_vdoframe)->size.h = (h); \
	(p_vdoframe)->pw[VDO_PINDEX_NRX] = (w); \
	(p_vdoframe)->ph[VDO_PINDEX_NRX] = (h); \
	(p_vdoframe)->loff[VDO_PINDEX_NRX] = (loff_nrx); \
	(p_vdoframe)->addr[VDO_PINDEX_NRX] = (addr_nrx); \
} while (0)

#define VDO_FRAME_MAKE_YUV422(p_vdoframe, w, h, addr_y, addr_uv, loff_y, loff_uv) \
do { \
	memset((p_vdoframe), 0, sizeof(VDO_FRAME)); \
	(p_vdoframe)->sign = MAKEFOURCC('V','F','R','M'); \
	(p_vdoframe)->pxlfmt = VDO_PXLFMT_YUV422; \
	(p_vdoframe)->size.w = (w); \
	(p_vdoframe)->size.h = (h); \
	(p_vdoframe)->pw[VDO_PINDEX_Y] = (w); \
	(p_vdoframe)->ph[VDO_PINDEX_Y] = (h); \
	(p_vdoframe)->pw[VDO_PINDEX_UV] = ((w)>>1); \
	(p_vdoframe)->ph[VDO_PINDEX_UV] = (h); \
	(p_vdoframe)->loff[VDO_PINDEX_Y] = (loff_y); \
	(p_vdoframe)->loff[VDO_PINDEX_UV] = (loff_uv); \
	(p_vdoframe)->addr[VDO_PINDEX_Y] = (addr_y); \
	(p_vdoframe)->addr[VDO_PINDEX_UV] = (addr_uv); \
} while (0)

#define VDO_FRAME_MAKE_YUV420(p_vdoframe, w, h, addr_y, addr_uv, loff_y, loff_uv) \
do { \
	memset((p_vdoframe), 0, sizeof(VDO_FRAME)); \
	(p_vdoframe)->sign = MAKEFOURCC('V','F','R','M'); \
	(p_vdoframe)->pxlfmt = VDO_PXLFMT_YUV420; \
	(p_vdoframe)->size.w = (w); \
	(p_vdoframe)->size.h = (h); \
	(p_vdoframe)->pw[VDO_PINDEX_Y] = (w); \
	(p_vdoframe)->ph[VDO_PINDEX_Y] = (h); \
	(p_vdoframe)->pw[VDO_PINDEX_UV] = ((w)>>1); \
	(p_vdoframe)->ph[VDO_PINDEX_UV] = ((h)>>1); \
	(p_vdoframe)->loff[VDO_PINDEX_Y] = (loff_y); \
	(p_vdoframe)->loff[VDO_PINDEX_UV] = (loff_uv); \
	(p_vdoframe)->addr[VDO_PINDEX_Y] = (addr_y); \
	(p_vdoframe)->addr[VDO_PINDEX_UV] = (addr_uv); \
} while (0)

#define VDO_FRAME_MAKE_Y8(p_vdoframe, w, h, addr_y, loff_y) \
do { \
	memset((p_vdoframe), 0, sizeof(VDO_FRAME)); \
	(p_vdoframe)->sign = MAKEFOURCC('V','F','R','M'); \
	(p_vdoframe)->pxlfmt = VDO_PXLFMT_Y8; \
	(p_vdoframe)->size.w = (w); \
	(p_vdoframe)->size.h = (h); \
	(p_vdoframe)->pw[VDO_PINDEX_Y] = (w); \
	(p_vdoframe)->ph[VDO_PINDEX_Y] = (h); \
	(p_vdoframe)->pw[VDO_PINDEX_UV] = (w); \
	(p_vdoframe)->ph[VDO_PINDEX_UV] = (h); \
	(p_vdoframe)->loff[VDO_PINDEX_Y] = (loff_y); \
	(p_vdoframe)->loff[VDO_PINDEX_UV] = (loff_uv); \
	(p_vdoframe)->addr[VDO_PINDEX_Y] = (addr_y); \
	(p_vdoframe)->addr[VDO_PINDEX_UV] = (addr_uv); \
} while (0)

#define VDO_FRAME_MAKE_NVX(p_vdoframe, w, h, pxlfmt) \
do { \
	memset((p_vdoframe), 0, sizeof(VDO_FRAME)); \
	(p_vdoframe)->sign = MAKEFOURCC('V','F','R','M'); \
	(p_vdoframe)->pxlfmt = (pxlfmt); \
	(p_vdoframe)->size.w = (w); \
	(p_vdoframe)->size.h = (h); \
} while (0)




//------------------------------------------------------------------------------
// data structures
//------------------------------------------------------------------------------

#define MEDIAVIDENC_MJPG            1       ///< motion jpeg
#define MEDIAVIDENC_H264            2       ///< h.264
#define MEDIAVIDENC_H265            3      ///< h.265


/**
     Stream video bitstream data descriptor.
*/
typedef struct _VDO_BITSTREAM {
	UINT32			sign;                      ///< MAKEFOURCC('V','S','T','M')
	VDO_METADATA* 	p_next;      				///< pointer to next meta
	UINT32			resv1;
	UINT32			Width;                     ///< image width
	UINT32			Height;                    ///< image height
	UINT32			CodecType;                 ///< codec type
	UINT64			count;					///< frame count
	UINT64			timestamp;				///< time stamp
	UINT32			DataAddr;                  ///< data address
	UINT32			DataSize;                  ///< data size
	UINT32			framepersecond;            ///< framepersecond
	UINT32			BytePerSecond;             ///< BytePerSecond
	UINT32			resv[24];                  ///<
} VDO_BITSTREAM, *PVDO_BITSTREAM; ///< size = 20 words
STATIC_ASSERT(sizeof(VDO_BITSTREAM) / sizeof(UINT32) == VDO_DATA_DESC_SIZE);

//NOTE: for VideoCodec == H264,
//      resv[0]=SPS addr, resv[1]=SPS size, resv[2]=PPS addr, resv[3]=PPS size



//------------------------------------------------------------------------------
// macro
//------------------------------------------------------------------------------


/**
     @name size of video
*/
#define VDO_SIZE(w,h)				MAKE_UINT16_UINT16(w,h)
#define VDO_SIZE_W_DEFAULT			0 ///< default
#define VDO_SIZE_H_DEFAULT			0 ///< default

/**
     @name pose of video
*/
#define VDO_POSE(x,y)				MAKE_UINT16_UINT16(x,y)
#define VDO_POSE_X_DEFAULT			0 ///< default
#define VDO_POSE_Y_DEFAULT			0 ///< default


#if 0
/**
     @name image format of video
*/
typedef GX_IMAGE_PIXEL_FMT VDO_FMT; ///< image format, refer to GX_IMAGE_PIXEL_FMT
#define VDO_FMT_YUV422		    	GX_IMAGE_PIXEL_FMT_YUV422_PACKED
#define VDO_FMT_YUV420		    	GX_IMAGE_PIXEL_FMT_YUV420_PACKED
#define VDO_FMT_Y8		    		GX_IMAGE_PIXEL_FMT_Y_ONLY
#define VDO_FMT_DEFAULT			VDO_FMT_YUV420
#endif

/**
     @name frame-rate-control of video
     @note
           INT16 dst_fr = GET_HI_UINT16(p_imginfo->framepersecond);
           INT16 src_fr = GET_LO_UINT16(p_imginfo->framepersecond);
           if ((dst_fr==1) && (src_fr==-1))                    ==> frame is triggered by user, fps = [unknown]
           if ((dst_fr> 1) && (src_fr>=1) && (dst_fr> src_fr)) ==> frame is triggered by dest port, fps = dst_fr/src_fr
           if ((dst_fr>=1) && (src_fr>=1) && (dst_fr==src_fr)) ==> frame is triggered by src port, fps = upstream_fps*1/1
           if ((dst_fr>=1) && (src_fr> 1) && (dst_fr< src_fr)) ==> frame is triggered by src port, fps = upstream_fps*dst_fr/src_fr
           others                                             ==> invalid
*/
#define VDO_FRC(dst_fr,src_fr)		MAKE_UINT16_UINT16(dst_fr,src_fr)
#define VDO_FRC_OFF				VDO_FRC(-1,-1) ///< output each frame (not control)
#define VDO_FRC_DIRECT  			VDO_FRC( 0, 0) ///< output each frame (not control)
#define VDO_FRC_ALL  				VDO_FRC( 1, 1) ///< output each frame (not control)
#define VDO_FRC_CAPTURE(n)			VDO_FRC( n,-1) ///< output n frame if user notify once (not control)
#define VDO_FRC_DEFAULT			VDO_FRC_DIRECT ///< default

/**
     @name aspect-ratio of video
*/
#define VDO_AR(x,y)     			MAKE_UINT16_UINT16(x,y) ///< composited ratio in WORD
#define VDO_AR_X_DEFAULT			0 ///< default
#define VDO_AR_Y_DEFAULT			0 ///< default
#define VDO_AR_USER				VDO_AR(0,0)

/**
     @name zoom-shift of video
*/
#define VDO_ZSHIFT(dx,dy)			VDO_POSE(dx,dy)
#define VDO_ZSHIFT_X_DEFAULT		0 ///< default
#define VDO_ZSHIFT_X_DEFAULT		0 ///< default

/**
     @name zoom-scale of video
*/
#define VDO_ZSCALE(factor)			VDO_SIZE(factor,1000)
#define VDO_ZSCALE_DEFAULT			1000 ///< default


/**
     @name direction of video
*/
#define VDO_DIR_NONE           	0x00000000 ///< off (default value)
#define VDO_DIR_MIRRORX        	0x10000000 ///< mirror in x direction
#define VDO_DIR_MIRRORY        	0x20000000 ///< mirror in y direction
#define VDO_DIR_MIRRORXY       	0x30000000 ///< mirror in x and y direction
#define VDO_DIR_ROTATE_MASK    	0x01ffffff ///< rotate mask
#define VDO_DIR_ROTATE_0       	0x00000000 ///< rotate 0 degrees clockwise
#define VDO_DIR_ROTATE_90      	0x005A0000 ///< rotate 90 degrees clockwise
#define VDO_DIR_ROTATE_180     	0x00B40000 ///< rotate 180 degrees clockwise
#define VDO_DIR_ROTATE_270     	0x010E0000 ///< rotate 270 degrees clockwise
#define VDO_DIR_ROTATE_360     	0x01680000 ///< rotate 360 degrees clockwise
#define VDO_DIR_ROTATE(n)      	((UINT32)((n)*0x10000)) ///< rotate (n) degrees clockwise
#define VDO_DIR_DEFAULT			VDO_DIR_NONE ///< default



//--------------------------------------------------------------------------------------
//  geometry
//--------------------------------------------------------------------------------------

//<type>

/**
     Location value.

     Location value.
     @note Nothing.
*/
typedef INT32   LVALUE;


//<function>

/*
//inline functions:

BOOL RANGE_IsInside(LVALUE small_value, LVALUE large_value, LVALUE x);
LVALUE RANGE_Clamp(LVALUE small_value, LVALUE large_value, LVALUE x);

void POINT_Set(IPOINT* pPt, LVALUE x, LVALUE y);
void POINT_SetPoint(IPOINT* pPt, const IPOINT* pPt2);
void POINT_MoveX(IPOINT* pPt, LVALUE dx);
void POINT_MoveY(IPOINT* pPt, LVALUE dy);
void POINT_MoveXY(IPOINT* pPt, LVALUE dx, LVALUE dy);
void POINT_MovePoint(IPOINT* pPt, const IPOINT* pPt2);
BOOL POINT_IsInside4Points(const IPOINT* pPt, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2);
BOOL POINT_IsInsideRect(const IPOINT* pPt, IRECT* pRect);

void SIZE_Set(ISIZE* pSz, LVALUE w, LVALUE h);
void SIZE_SetSize(ISIZE* pSz, const ISIZE* pSz2);

void RECT_Set(IRECT* pRect, LVALUE x, LVALUE y, LVALUE w, LVALUE h);
void RECT_SetXY(IRECT* pRect, LVALUE x, LVALUE y);
void RECT_SetWH(IRECT* pRect, LVALUE w, LVALUE h);
void RECT_SetPoint(IRECT* pRect, const IPOINT* pPt);
void RECT_SetSize(IRECT* pRect, const ISIZE* pSz);
void RECT_SetRect(IRECT* pRect, const IRECT* pRect2);
LVALUE RECT_GetX1(const IRECT* pRect);
LVALUE RECT_GetY1(const IRECT* pRect);
LVALUE RECT_GetX2(const IRECT* pRect);
LVALUE RECT_GetY2(const IRECT* pRect);
IPOINT RECT_GetPoint(const IRECT* pRect);
IPOINT RECT_GetPoint2(const IRECT* pRect);
IPOINT RECT_GetCenterPoint(const IRECT* pRect);
ISIZE RECT_GetSize(const IRECT* pRect);
void RECT_MoveX(IRECT* pRect, LVALUE dx);
void RECT_MoveY(IRECT* pRect, LVALUE dy);
void RECT_MoveXY(IRECT* pRect, LVALUE dx, LVALUE dy);
void RECT_MovePoint(IRECT* pRect, const IPOINT* pPt2);

void RECT_Normalize(IRECT* pRect);
void RECT_Inflate(IRECT* pRect, LVALUE left, LVALUE right, LVALUE top, LVALUE bottom);
void RECT_Shrink(IRECT* pRect, LVALUE left, LVALUE right, LVALUE top, LVALUE bottom);
*/


/**
     Check value if inside range or not.

     Check value if inside [min~max] range or not.
     @note Nothing

     @param[in] small_value     Minimum value.
     @param[in] large_value     Maximum value.
     @param[in] x               Current value.
     @return
         - @b TRUE:             Inside range.
         - @b FALSE:            Out of range.
*/
#define RANGE_IsInside(small_value, large_value, x) \
	((((x)>=(small_value)) && ((x)<=(large_value)))?TRUE:FALSE)

/**
     Clamp value to inside range.

     Clamp value to inside [min~max] range.
     @note Nothing

     @param[in] small_value     Minimum value.
     @param[in] large_value     Maximum value.
     @param[in] x               Current value.
     @return                    Output value.
*/
#define RANGE_Clamp(small_value, large_value, x) \
	(((x)<(small_value))?(small_value):(((x)>(large_value))?(large_value):(x)))

/**
     Set value of IPOINT.

     Set value of IPOINT.
     @note Nothing

     @param[out] pPt            IPOINT.
     @param[in] x               x value.
     @param[in] y               y value.
*/
_INLINE void POINT_Set(IPOINT *pPt, LVALUE x, LVALUE y)
{
	(pPt)->x = (x);
	(pPt)->y = (y);
}

/**
     Copy value of IPOINT.

     Copy value of IPOINT.
     @note Nothing

     @param[out] pPt            IPOINT.
     @param[in] pPt2            copy IPOINT.
*/
_INLINE void POINT_SetPoint(IPOINT *pPt, const IPOINT *pPt2)
{
	*(pPt) = *(pPt2);
}

/**
     Move x value of IPOINT.

     Move x value of IPOINT.
     @note Nothing

     @param[in,out] pPt         IPOINT.
     @param[in] dx              Shift offset of x.
*/
_INLINE void POINT_MoveX(IPOINT *pPt, LVALUE dx)
{
	(pPt)->x += (dx);
}

/**
     Move y value of IPOINT.

     Move y value of IPOINT.
     @note Nothing

     @param[in,out] pPt         IPOINT.
     @param[in] dy              Shift offset of y.
*/
_INLINE void POINT_MoveY(IPOINT *pPt, LVALUE dy)
{
	(pPt)->y += (dy);
}

/**
     Move x,y value of IPOINT.

     Move x,y value of IPOINT.
     @note Nothing

     @param[in,out] pPt         IPOINT.
     @param[in] dx              Shift offset of x.
     @param[in] dy              Shift offset of y.
*/
_INLINE void POINT_MoveXY(IPOINT *pPt, LVALUE dx, LVALUE dy)
{
	(pPt)->x += (dx);
	(pPt)->y += (dy);
}

/**
     Move x,y value of IPOINT by vector.

     Move x,y value of IPOINT by vector.
     @note Nothing

     @param[in,out] pPt         IPOINT.
     @param[in] pPt2            vector IPOINT.
*/
_INLINE void POINT_MovePoint(IPOINT *pPt, const IPOINT *pPt2)
{
	(pPt)->x += (pPt2)->x;
	(pPt)->y += (pPt2)->y;
}

/**
     Check IPOINT if inside rectangle or not.

     Check IPOINT if inside rectangle or not.
     @note Nothing

     @param[in] pPt             IPOINT.
     @param[in] x1              Left of rectangle.
     @param[in] y1              Top of rectangle.
     @param[in] x2              Right of rectangle.
     @param[in] y2              Bottom of rectangle.
     @return
         - @b TRUE:             Inside rectangle.
         - @b FALSE:            Out of rectangle.
*/
_INLINE BOOL POINT_IsInside4Points(const IPOINT *pPt, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2)
{
	return
		(
			(pPt->x >= x1) &&
			(pPt->x <= x2) &&
			(pPt->y >= y1) &&
			(pPt->y <= y2)
		);
}

/**
     Check IPOINT if inside rectangle or not.

     Check IPOINT if inside rectangle or not.
     @note Nothing

     @param[in] pPt             IPOINT.
     @param[in] pRect           The rectangle.
     @return
         - @b TRUE:             Inside rectangle.
         - @b FALSE:            Out of rectangle.
*/
_INLINE BOOL POINT_IsInsideRect(const IPOINT *pPt, IRECT *pRect)
{
	return
		(
			(pPt->x >= pRect->x) &&
			(pPt->x < (pRect->x + pRect->w)) &&
			(pPt->y >= pRect->y) &&
			(pPt->y < (pRect->y + pRect->h))
		);
}


/**
     Set value of SIZE.

     Set value of SIZE.
     @note Nothing

     @param[out] pSz            SIZE.
     @param[in] w               w value.
     @param[in] h               h value.
*/
_INLINE void SIZE_Set(ISIZE *pSz, LVALUE w, LVALUE h)
{
	(pSz)->w = (w);
	(pSz)->h = (h);
}

/**
     Copy value of SIZE.

     Copy value of SIZE.
     @note Nothing

     @param[out] pSz            SIZE.
     @param[in] pSz2            copy SIZE.
*/
_INLINE void SIZE_SetSize(ISIZE *pSz, const ISIZE *pSz2)
{
	*(pSz) = *(pSz2);
}



/**
     Set value of IRECT.

     Set value of IRECT.
     @note Nothing

     @param[out] pRect          IRECT.
     @param[in] x               x value.
     @param[in] y               y value.
     @param[in] w               w value.
     @param[in] h               h value.
*/
_INLINE void RECT_Set(IRECT *pRect, LVALUE x, LVALUE y, LVALUE w, LVALUE h)
{
	(pRect)->x = (x);
	(pRect)->y = (y);
	(pRect)->w = (w);
	(pRect)->h = (h);
}

/**
     Set x,y value of IRECT.

     Set x,y value of IRECT.
     @note Nothing

     @param[out] pRect          IRECT.
     @param[in] x               x value.
     @param[in] y               y value.
*/
_INLINE void RECT_SetXY(IRECT *pRect, LVALUE x, LVALUE y)
{
	(pRect)->x = (x);
	(pRect)->y = (y);
}

/**
     Set w,h value of IRECT.

     Set w,h value of IRECT.
     @note Nothing

     @param[out] pRect          IRECT.
     @param[in] w               w value.
     @param[in] h               h value.
*/
_INLINE void RECT_SetWH(IRECT *pRect, LVALUE w, LVALUE h)
{
	(pRect)->w = (w);
	(pRect)->h = (h);
}

/**
     Set x,y value of IRECT by IPOINT.

     Set x,y value of IRECT by IPOINT.
     @note Nothing

     @param[out] pRect          IRECT.
     @param[in] pPt             The IPOINT.
*/
_INLINE void RECT_SetPoint(IRECT *pRect, const IPOINT *pPt)
{
	(pRect)->x = (pPt)->x;
	(pRect)->y = (pPt)->y;
}

/**
     Set w,h value of IRECT by ISIZE.

     Set w,h value of IRECT by ISIZE.
     @note Nothing

     @param[out] pRect          IRECT.
     @param[in] pSz             The ISIZE.
*/
_INLINE void RECT_SetSize(IRECT *pRect, const ISIZE *pSz)
{
	(pRect)->w = (pSz)->w;
	(pRect)->h = (pSz)->h;
}

/**
     Copy value of IRECT.

     Copy value of IRECT.
     @note Nothing

     @param[out] pRect          IRECT.
     @param[in] pRect2          copy IRECT.
*/
_INLINE void RECT_SetRect(IRECT *pRect, const IRECT *pRect2)
{
	*(pRect) = *(pRect2);
}

/**
     Get left x value of IRECT.

     Get left x value of IRECT.
     @note Nothing

     @param[in] pRect           IRECT.
     @return                    x1 value.
*/
_INLINE LVALUE RECT_GetX1(const IRECT *pRect)
{
	return (pRect)->x;
}

/**
     Get top y value of IRECT.

     Get top y value of IRECT.
     @note Nothing

     @param[in] pRect           IRECT.
     @return                    y1 value.
*/
_INLINE LVALUE RECT_GetY1(const IRECT *pRect)
{
	return (pRect)->y;
}

/**
     Get right x value of IRECT.

     Get right x value of IRECT.
     @note Nothing

     @param[in] pRect           IRECT.
     @return                    x2 value.
*/
_INLINE LVALUE RECT_GetX2(const IRECT *pRect)
{
	return (pRect)->x + (pRect)->w;
}

/**
     Get bottom y value of IRECT.

     Get bottom y value of IRECT.
     @note Nothing

     @param[in] pRect           IRECT.
     @return                    y2 value.
*/
_INLINE LVALUE RECT_GetY2(const IRECT *pRect)
{
	return (pRect)->y + (pRect)->h;
}

/**
     Get left-top point of IRECT to IPOINT.

     Get left-top point of IRECT to IPOINT.
     @note Nothing

     @param[in] pRect           IRECT.
     @return                    The IPOINT.
*/
_INLINE IPOINT RECT_GetPoint(const IRECT *pRect)
{
	IPOINT Pt;
	(Pt).x = (pRect)->x;
	(Pt).y = (pRect)->y;
	return Pt;
}

/**
     Get right-bottom point of IRECT to IPOINT.

     Get right-bottom point of IRECT to IPOINT.
     @note Nothing

     @param[in] pRect           IRECT.
     @return                    The IPOINT.
*/
_INLINE IPOINT RECT_GetPoint2(const IRECT *pRect)
{
	IPOINT Pt;
	(Pt).x = (pRect)->x + (pRect)->w;
	(Pt).y = (pRect)->y + (pRect)->h;
	return Pt;
}

/**
     Get center point of IRECT to IPOINT.

     Get center point of IRECT to IPOINT.
     @note Nothing

     @param[in] pRect           IRECT.
     @return                    The IPOINT.
*/
_INLINE IPOINT RECT_GetCenterPoint(const IRECT *pRect)
{
	IPOINT Pt;
	(Pt).x = (pRect)->x + ((pRect)->w >> 1);
	(Pt).y = (pRect)->y + ((pRect)->h >> 1);
	return Pt;
}

/**
     Get size of IRECT to ISIZE.

     Get size of IRECT to ISIZE.
     @note Nothing

     @param[in] pRect           IRECT.
     @return                    The ISIZE.
*/
_INLINE ISIZE RECT_GetSize(const IRECT *pRect)
{
	ISIZE Sz;
	(Sz).w = (pRect)->w;
	(Sz).h = (pRect)->h;
	return Sz;
}

/**
     Normalize x,y,w,h value of IRECT.

     Normalize x,y,w,h value of IRECT.
     @note Keep x,y is left-top point, w,h is right-bottom point from left-top point.

     @param[in,out] pRect       IRECT.
*/
_INLINE void RECT_Normalize(IRECT *pRect)
{
	if ((pRect)->w < 0) {
		(pRect)->x += (pRect)->w;
		(pRect)->w = -(pRect)->w;
	}
	if ((pRect)->h < 0) {
		(pRect)->y += (pRect)->h;
		(pRect)->h = -(pRect)->h;
	}
}

/**
     Set value of IRECT.

     Set value of IRECT.
     @note Nothing

     @param[out] pRect          IRECT.
     @param[in] x1              left x value.
     @param[in] y1              top y value.
     @param[in] x2              right x value.
     @param[in] y2              bottom y value.
*/
_INLINE void RECT_SetX1Y1X2Y2(IRECT *pRect, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2)
{
	LVALUE temp;
	if (x2 < x1) {
		temp = x2;
		x2 = x1;
		x1 = temp;
	}
	if (y2 < y1) {
		temp = y2;
		y2 = y1;
		y1 = temp;
	}
	RECT_Set(pRect, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}

/**
     Move x value of RECT.

     Move x value of RECT.
     @note Nothing

     @param[in,out] pRect       IRECT.
     @param[in] dx              move x value.
*/
#define RECT_MoveX(pRect, dx)   \
	POINT_MoveX(((IPOINT*)pRect), dx)

/**
     Move y value of RECT.

     Move y value of RECT.
     @note Nothing

     @param[in,out] pRect       IRECT.
     @param[in] dy              move y value.
*/
#define RECT_MoveY(pRect, dy)   \
	POINT_MoveY(((IPOINT*)pRect), dy)

/**
     Move x,y value of RECT.

     Move x,y value of RECT.
     @note Nothing

     @param[in,out] pRect       IRECT.
     @param[in] dx              move x value.
     @param[in] dy              move y value.
*/
#define RECT_MoveXY(pRect, dx, dy)   \
	POINT_MoveXY(((IPOINT*)pRect), dx, dy)

/**
     Move x,y value of RECT by IPOINT.

     Move x,y value of RECT by IPOINT.
     @note Nothing

     @param[in,out] pRect       IRECT.
     @param[in] pPt             move vector IPOINT.
*/
#define RECT_MovePoint(pRect, pPt)   \
	POINT_MovePoint(((IPOINT*)pRect), pPt)


/**
     Inflate range of RECT.

     Inflate range of RECT.
     @note Nothing

     @param[in,out] pRect       IRECT.
     @param[in] left            left side inflate value.
     @param[in] right           right side inflate value.
     @param[in] top             top side inflate value.
     @param[in] bottom          bottom side inflate value.
*/
_INLINE void RECT_Inflate(IRECT *pRect, LVALUE left, LVALUE right, LVALUE top, LVALUE bottom)
{
	(pRect)->x += -(left);
	(pRect)->w += ((left) + (right));
	(pRect)->y += -(top);
	(pRect)->h += ((top) + (bottom));
}


/**
     Shrink range of RECT.

     Shrink range of RECT.
     @note Nothing

     @param[in,out] pRect       IRECT.
     @param[in] left            left side shrink value.
     @param[in] right           right side shrink value.
     @param[in] top             top side shrink value.
     @param[in] bottom          bottom side shrink value.
*/
#define RECT_Shrink(pRect, left, right, top, bottom) \
	RECT_Inflate(pRect, -(left), -(right), -(top), -(bottom))



#ifdef __cplusplus
} //extern "C"
#endif

#endif //_TYPE_VDO_H_
