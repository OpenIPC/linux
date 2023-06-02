/**
	@brief Header file of vendor isp module.\n
	This file contains the functions which is related to 3A in the chip.

	@file vendor_isp.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _VENDOR_ISP_H_
#define _VENDOR_ISP_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include <kwrap/type.h>
#include "hd_type.h"

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/

/********************************************************************
	MACRO FUNCTION DEFINITIONS
********************************************************************/

/********************************************************************
	TYPE DEFINITION
********************************************************************/
#define CFG_NAME_LENGTH  256
#define DTSI_NAME_LENGTH  256

#if !defined(__FREERTOS)
/**
	Common item
*/
#define VDO_MAX_PLANE 4
#define CTL_SEN_ID_MAX 8

typedef enum _VDO_PXLFMT {
	/* 0 = osd icon index format */
	VDO_PXLFMT_I1_ICON              = 0x01010001, ///< 1 plane, pixel=INDEX(w,h), w/o padding bits
	VDO_PXLFMT_I2_ICON              = 0x01020002, ///< 1 plane, pixel=INDEX(w,h), w/o padding bits
	VDO_PXLFMT_I4_ICON              = 0x01040004, ///< 1 plane, pixel=INDEX(w,h), w/o padding bits
	VDO_PXLFMT_I8_ICON              = 0x01080008, ///< 1 plane, pixel=INDEX(w,h), w/o padding bits
	/* 1 = osd index format */
	VDO_PXLFMT_I1                   = 0x11010001, ///< 1 plane, pixel=INDEX(w,h)
	VDO_PXLFMT_I2                   = 0x11020002, ///< 1 plane, pixel=INDEX(w,h)
	VDO_PXLFMT_I4                   = 0x11040004, ///< 1 plane, pixel=INDEX(w,h)
	VDO_PXLFMT_I8                   = 0x11080008, ///< 1 plane, pixel=INDEX(w,h)
	/* 2 = osd argb format */
	VDO_PXLFMT_RGB888_PLANAR        = 0x23180888, ///< 3 plane, pixel=R(w,h), G(w,h), B(w,h)
	VDO_PXLFMT_RGB888               = 0x21180888, ///< 1 plane, pixel=RGB(w,h)
	VDO_PXLFMT_RGB565               = 0x21100565, ///< 1 plane, pixel=RGB(2w,h)
	VDO_PXLFMT_RGBA5551             = 0x21105551, ///< 1 plane, pixel=ARGB(2w,h)
	VDO_PXLFMT_ARGB1555             = 0x21101555, ///< 1 plane, pixel=ARGB(2w,h)
	VDO_PXLFMT_ARGB4444             = 0x21104444, ///< 1 plane, pixel=ARGB(2w,h)
	VDO_PXLFMT_A4                   = 0x21044000, ///< 1 plane, pixel=A(w,h)
	VDO_PXLFMT_ARGB4565             = 0x22404565, ///< 2 plane, pixel=A4(w,h),RGB565(2w,h)
	VDO_PXLFMT_A8                   = 0x21088000, ///< 1 plane, pixel=A(w,h)
	VDO_PXLFMT_ARGB8565             = 0x22188565, ///< 2 plane, pixel=A8(w,h),RGB565(2w,h)
	VDO_PXLFMT_ARGB8888             = 0x21208888, ///< 1 plane, pixel=ARGB(4w,h)
	/* 3 = osd compressed argb format */
	/* 4 = video raw format */
	VDO_PXLFMT_RAW                  = 0x41000000, ///< raw general
	VDO_PXLFMT_RAW8                 = 0x41080000, ///< 1 plane, pixel=RAW(w,h) x 8bits
	VDO_PXLFMT_RAW10                = 0x410a0000, ///< 1 plane, pixel=RAW(w,h) x 10bits
	VDO_PXLFMT_RAW12                = 0x410c0000, ///< 1 plane, pixel=RAW(w,h) x 12bits
	VDO_PXLFMT_RAW14                = 0x410e0000, ///< 1 plane, pixel=RAW(w,h) x 14bits
	VDO_PXLFMT_RAW16                = 0x41100000, ///< 1 plane, pixel=RAW(w,h) x 16bits
	VDO_PXLFMT_RAW8_SHDR2           = 0x42080000, ///< 2 plane, pixel=RAW(w,h) x 8bits
	VDO_PXLFMT_RAW10_SHDR2          = 0x420a0000, ///< 2 plane, pixel=RAW(w,h) x 10bits
	VDO_PXLFMT_RAW12_SHDR2          = 0x420c0000, ///< 2 plane, pixel=RAW(w,h) x 12bits
	VDO_PXLFMT_RAW14_SHDR2          = 0x420e0000, ///< 2 plane, pixel=RAW(w,h) x 14bits
	VDO_PXLFMT_RAW16_SHDR2          = 0x42100000, ///< 2 plane, pixel=RAW(w,h) x 16bits
	VDO_PXLFMT_RAW8_SHDR3           = 0x43080000, ///< 3 plane, pixel=RAW(w,h) x 8bits
	VDO_PXLFMT_RAW10_SHDR3          = 0x430a0000, ///< 3 plane, pixel=RAW(w,h) x 10bits
	VDO_PXLFMT_RAW12_SHDR3          = 0x430c0000, ///< 3 plane, pixel=RAW(w,h) x 12bits
	VDO_PXLFMT_RAW14_SHDR3          = 0x430e0000, ///< 3 plane, pixel=RAW(w,h) x 14bits
	VDO_PXLFMT_RAW16_SHDR3          = 0x43100000, ///< 3 plane, pixel=RAW(w,h) x 16bits
	VDO_PXLFMT_RAW8_SHDR4           = 0x44080000, ///< 4 plane, pixel=RAW(w,h) x 8bits
	VDO_PXLFMT_RAW10_SHDR4          = 0x440a0000, ///< 4 plane, pixel=RAW(w,h) x 10bits
	VDO_PXLFMT_RAW12_SHDR4          = 0x440c0000, ///< 4 plane, pixel=RAW(w,h) x 12bits
	VDO_PXLFMT_RAW14_SHDR4          = 0x440e0000, ///< 4 plane, pixel=RAW(w,h) x 14bits
	VDO_PXLFMT_RAW16_SHDR4          = 0x44100000, ///< 4 plane, pixel=RAW(w,h) x 16bits
	/* 5 = video yuv format */
	VDO_PXLFMT_Y8                   = 0x51080400, ///< 1 plane, pixel=Y(w,h)
	VDO_PXLFMT_YUV400               = VDO_PXLFMT_Y8,
	VDO_PXLFMT_YUV420_PLANAR        = 0x530c0420, ///< 3 plane, pixel=Y(w,h), U(w/2,h/2), and V(w/2,h/2)
	VDO_PXLFMT_YUV420               = 0x520c0420, ///< 2 plane, pixel=Y(w,h), UV(w/2,h/2)
	VDO_PXLFMT_YUV422_PLANAR        = 0x53100422, ///< 3 plane, pixel=Y(w,h), U(w/2,h), and V(w/2,h)
	VDO_PXLFMT_YUV422               = 0x52100422, ///< 2 plane, pixel=Y(w,h), UV(w/2,h)
	VDO_PXLFMT_YUV444_PLANAR        = 0x53180444, ///< 3 plane, pixel=Y(w,h), U(w,h), and V(w,h)
	VDO_PXLFMT_YUV444               = 0x52180444, ///< 2 plane, pixel=Y(w,h), UV(w,h)
	VDO_PXLFMT_YUV444_ONE           = 0x51180444, ///< 1 plane, pixel=YUV(w,h)
	/* 6 = video yuv compress format */
	VDO_PXLFMT_YUV420_NVX           = 0x61000000, ///< novatek-yuv-compress general
	VDO_PXLFMT_YUV420_NVX1          = 0x610c1000, ///< novatek-yuv-compress-1 of YUV420 (using by NA51000)
	VDO_PXLFMT_YUV420_NVX1_H264     = 0x610c0420, ///< novatek-yuv-compress-1 of YUV420 for h264 (using by NA51000)
	VDO_PXLFMT_YUV420_NVX1_H265     = 0x610c1420, ///< novatek-yuv-compress-1 of YUV420 for h265 (using by NA51000)
	VDO_PXLFMT_YUV420_NVX2          = 0x610c2420, ///< novatek-yuv-compress-2 of YUV420 (using by NA51023)
	/* f = video raw compress format */
	VDO_PXLFMT_NRX                  = 0xf1000000, ///< novatek-raw-compress general
	VDO_PXLFMT_NRX8                 = 0xf1080000, ///< novatek-raw-compress-1 of RAW8
	VDO_PXLFMT_NRX10                = 0xf10a0000, ///< novatek-raw-compress-1 of RAW10
	VDO_PXLFMT_NRX12                = 0xf10c0000, ///< novatek-raw-compress-1 of RAW12
	VDO_PXLFMT_NRX14                = 0xf10e0000, ///< novatek-raw-compress-1 of RAW14
	VDO_PXLFMT_NRX16                = 0xf1100000, ///< novatek-raw-compress-1 of RAW16 (using by NA51000 and NA51023)
	VDO_PXLFMT_NRX8_SHDR2           = 0xf2080000, ///< novatek-raw-compress-1 of RAW8
	VDO_PXLFMT_NRX10_SHDR2          = 0xf20a0000, ///< novatek-raw-compress-1 of RAW10
	VDO_PXLFMT_NRX12_SHDR2          = 0xf20c0000, ///< novatek-raw-compress-1 of RAW12
	VDO_PXLFMT_NRX14_SHDR2          = 0xf20e0000, ///< novatek-raw-compress-1 of RAW14
	VDO_PXLFMT_NRX16_SHDR2          = 0xf2100000, ///< novatek-raw-compress-1 of RAW16 (using by NA51000 and NA51023)
	VDO_PXLFMT_NRX8_SHDR3           = 0xf3080000, ///< novatek-raw-compress-1 of RAW8
	VDO_PXLFMT_NRX10_SHDR3          = 0xf30a0000, ///< novatek-raw-compress-1 of RAW10
	VDO_PXLFMT_NRX12_SHDR3          = 0xf30c0000, ///< novatek-raw-compress-1 of RAW12
	VDO_PXLFMT_NRX14_SHDR3          = 0xf30e0000, ///< novatek-raw-compress-1 of RAW14
	VDO_PXLFMT_NRX16_SHDR3          = 0xf3100000, ///< novatek-raw-compress-1 of RAW16 (using by NA51000 and NA51023)
	VDO_PXLFMT_NRX8_SHDR4           = 0xf4080000, ///< novatek-raw-compress-1 of RAW8
	VDO_PXLFMT_NRX10_SHDR4          = 0xf40a0000, ///< novatek-raw-compress-1 of RAW10
	VDO_PXLFMT_NRX12_SHDR4          = 0xf40c0000, ///< novatek-raw-compress-1 of RAW12
	VDO_PXLFMT_NRX14_SHDR4          = 0xf40e0000, ///< novatek-raw-compress-1 of RAW14
	VDO_PXLFMT_NRX16_SHDR4          = 0xf4100000, ///< novatek-raw-compress-1 of RAW16 (using by NA51000 and NA51023)
} VDO_PXLFMT;

typedef enum _ISP_SEN_MODE_TYPE {
	ISP_SEN_MODE_TYPE_UNKNOWN   = 0x00000000,
	//single frame
	ISP_SEN_MODE_LINEAR         = 0x00000001,
	ISP_SEN_MODE_BUILTIN_HDR    = 0x00000002,
	ISP_SEN_MODE_CCIR           = 0x00000004,
	ISP_SEN_MODE_CCIR_INTERLACE = 0x00000008,
	//multiple frames
	ISP_SEN_MODE_STAGGER_HDR    = 0x00000010,
	ISP_SEN_MODE_PDAF           = 0x00000020,
	ENUM_DUMMY4WORD(ISP_SEN_MODE_TYPE)
} ISP_SEN_MODE_TYPE;

typedef enum _ISP_SEN_DATA_FMT {
	ISP_SEN_DATA_FMT_UNKNOW     = 0x00000000,
	ISP_SEN_DATA_FMT_RGB        = 0x00000001,
	ISP_SEN_DATA_FMT_RGBIR      = 0x00000002,
	ISP_SEN_DATA_FMT_RCCB       = 0x00000004,
	ISP_SEN_DATA_FMT_YUV        = 0x00000008,
	ISP_SEN_DATA_FMT_Y_ONLY     = 0x00000010,
	ENUM_DUMMY4WORD(ISP_SEN_DATA_FMT)
} ISP_SEN_DATA_FMT;

typedef struct {
	UINT32 min; ///< min gain (1X = 1 x 1000 = 1000)
	UINT32 max; ///< max gain (1X = 1 x 1000 = 1000)
} CTL_SEN_MODE_GAIN;
#endif

#include "isp_mtr.h"
#include "isp_api.h"
#include "ispt_api.h"
#include "ae_alg.h"
#include "ae_ui.h"
#include "aet_api.h"
#include "af_alg.h"
#include "af_ui.h"
#include "aft_api.h"
#include "awb_alg.h"
#include "awb_ui.h"
#include "awbt_api.h"
#include "iq_alg.h"
#include "iq_ui.h"
#include "iqt_api.h"
#if !defined(__FREERTOS)
#include "isp_ioctl.h"
#endif

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
extern HD_RESULT vendor_isp_init(void);
extern HD_RESULT vendor_isp_uninit(void);
extern HD_RESULT vendor_isp_get_ae(AET_ITEM item, VOID *p_param);
extern HD_RESULT vendor_isp_set_ae(AET_ITEM item, VOID *p_param);
extern HD_RESULT vendor_isp_get_af(AFT_ITEM item, VOID *p_param);
extern HD_RESULT vendor_isp_set_af(AFT_ITEM item, VOID *p_param);
extern HD_RESULT vendor_isp_get_awb(AWBT_ITEM item, VOID *p_param);
extern HD_RESULT vendor_isp_set_awb(AWBT_ITEM item, VOID *p_param);
extern HD_RESULT vendor_isp_get_iq(IQT_ITEM item, VOID *p_param);
extern HD_RESULT vendor_isp_set_iq(IQT_ITEM item, VOID *p_param);
extern HD_RESULT vendor_isp_get_common(ISPT_ITEM item, VOID *p_param);
extern HD_RESULT vendor_isp_set_common(ISPT_ITEM item, VOID *p_param);
#endif

