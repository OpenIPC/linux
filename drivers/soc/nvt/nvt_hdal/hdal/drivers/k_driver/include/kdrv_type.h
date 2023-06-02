/**
 * @file kdrv_type.h
 * @brief type definition of KDRV API.
 * @author PSW
 * @date in the year 2018
 */

#ifndef _KDRV_TYPE_H_
#define _KDRV_TYPE_H_
#include <kwrap/nvt_type.h>


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

#define KDRV_CHIP_OFFSET      24
#define KDRV_CHIP_BITMAP      0xFF000000
#define KDRV_ENGINE_OFFSET    8
#define KDRV_ENGINE_BITMAP    0x00FFFF00
#define KDRV_CHANNEL_OFFSET   0
#define KDRV_CHANNEL_BITMAP   0x000000FF

#define KDRV_DEV_ID_CHIP(id)      ((id & KDRV_CHIP_BITMAP) >> KDRV_CHIP_OFFSET)
#define KDRV_DEV_ID_ENGINE(id)    ((id & KDRV_ENGINE_BITMAP) >> KDRV_ENGINE_OFFSET)
#define KDRV_DEV_ID_CHANNEL(id)   ((id & KDRV_CHANNEL_BITMAP) >> KDRV_CHANNEL_OFFSET)
#define KDRV_DEV_ID(chip, engine, channel) \
	(((chip) << KDRV_CHIP_OFFSET) | ((engine) << KDRV_ENGINE_OFFSET) | ((channel) & KDRV_CHANNEL_BITMAP))

typedef enum {
	KDRV_CHIP0 = 0x00,
	KDRV_CHIP1 = 0x01,
	KDRV_CHIP2 = 0x02,
	KDRV_CHIP3 = 0x03,
	KDRV_CHIP4 = 0x04,
} KDRV_DEV_CHIP;

typedef enum {
	KDRV_VDOCAP_ENGINE0  = 0x1000,
	KDRV_VDOCAP_ENGINE1  = 0x1001,
	KDRV_VDOCAP_ENGINE2  = 0x1002,
	KDRV_VDOCAP_ENGINE3  = 0x1003,
	KDRV_VDOCAP_ENGINE4  = 0x1004,
	KDRV_VDOCAP_ENGINE5  = 0x1005,
	KDRV_VDOCAP_ENGINE6  = 0x1006,
	KDRV_VDOCAP_ENGINE7  = 0x1007,

	KDRV_VDOOUT_ENGINE0 = 0x2000,
	KDRV_VDOOUT_ENGINE1 = 0x2001,

	KDRV_VIDEODEC_ENGINE0 = 0x3000,

	KDRV_VIDEOCDC_ENGINE0 = 0x4000,
	KDRV_VIDEOCDC_ENGINE_JPEG = 0x4200,

	KDRV_VIDEOPROCS_RHE_ENGINE0 = 0x5000,
	KDRV_VIDEOPROCS_IFE_ENGINE0 = 0x5001,
	KDRV_VIDEOPROCS_DCE_ENGINE0 = 0x5002,
	KDRV_VIDEOPROCS_IPE_ENGINE0 = 0x5003,
	KDRV_VIDEOPROCS_IME_ENGINE0 = 0x5004,
	KDRV_VIDEOPROCS_IFE2_ENGINE0 = 0x5005,
	KDRV_VIDEOPROCS_VPE_ENGINE0 = 0x5006,

	KDRV_AUDCAP_ENGINE0 = 0x6000,	///< Audio Capture from normal record path. <mic/dmic/i2s><TimeCode Hit function Available>
	KDRV_AUDCAP_ENGINE1 = 0x6001,	///< Audio Capture from output data loopback.
									///< This channel is used in the AEC(Acoustic Echo Cancellation) application usgae.

	KDRV_AUDOUT_ENGINE0 = 0x7000,	///< Audio output engine ID 1.<TimeCode Hit function Available>
	KDRV_AUDOUT_ENGINE1 = 0x7001,	///< Audio output engine ID 2.

	KDRV_GFX2D_GRPH0 = 0x8000,
	KDRV_GFX2D_GRPH1 = 0x8001,
	KDRV_GFX2D_ISE0 = 0x8002,
	KDRV_GFX2D_ISE1 = 0x8003,
	KDRV_GFX2D_ROTATE = 0x8004,
	KDRV_GFX2D_HWCOPY = 0x8005,
	KDRV_GFX2D_AFFINE = 0x8006,

	KDRV_SSENIF_ENGINE_CSI0  = 0x9000,
	KDRV_SSENIF_ENGINE_CSI1  = 0x9001,
	KDRV_SSENIF_ENGINE_CSI2  = 0x9002,
	KDRV_SSENIF_ENGINE_CSI3  = 0x9003,
	KDRV_SSENIF_ENGINE_CSI4  = 0x9004,
	KDRV_SSENIF_ENGINE_CSI5  = 0x9005,
	KDRV_SSENIF_ENGINE_CSI6  = 0x9006,
	KDRV_SSENIF_ENGINE_CSI7  = 0x9007,

	KDRV_SSENIF_ENGINE_LVDS0 = 0x9008,
	KDRV_SSENIF_ENGINE_LVDS1 = 0x9009,
	KDRV_SSENIF_ENGINE_LVDS2 = 0x900A,
	KDRV_SSENIF_ENGINE_LVDS3 = 0x900B,
	KDRV_SSENIF_ENGINE_LVDS4 = 0x900C,
	KDRV_SSENIF_ENGINE_LVDS5 = 0x900D,
	KDRV_SSENIF_ENGINE_LVDS6 = 0x900E,
	KDRV_SSENIF_ENGINE_LVDS7 = 0x900F,
	KDRV_SSENIF_ENGINE_VX1_0 = 0x9010,
	KDRV_SSENIF_ENGINE_VX1_1 = 0x9011,
	KDRV_SSENIF_ENGINE_SLVSEC0=0x9012,

	KDRV_RPC_ENGINE0         = 0xA000,

	KDRV_VDOCAP_TGE_ENGINE0  = 0Xb000,

	KDRV_CV_ENGINE_IVE	 = 0xC000,
	KDRV_CV_ENGINE_SDE	 = 0xC001,
	KDRV_CV_ENGINE_MD	 = 0xC002,
	KDRV_CV_ENGINE_DIS	 = 0xC003,

	KDRV_AI_ENGINE_AI   = 0xD000,
	KDRV_AI_ENGINE_CNN  = 0xD001,
	KDRV_AI_ENGINE_NUE  = 0xD002,
	KDRV_AI_ENGINE_NUE2 = 0xD003,
	KDRV_AI_ENGINE_CNN2 = 0xD004,
} KDRV_DEV_ENGINE;

typedef struct {
	UINT32  x;      ///< X coordinate of the point
	UINT32  y;      ///< Y coordinate of the point
} KDRV_UPOINT;

typedef struct {
	UINT32  w;      ///< Rectangle width
	UINT32  h;      ///< Rectangle height
} KDRV_USIZE;

typedef struct {
	UINT32  x;      ///< X coordinate of the top-left point of the rectangle
	UINT32  y;      ///< Y coordinate of the top-left point of the rectangle
	UINT32  w;      ///< Rectangle width
	UINT32  h;      ///< Rectangle height
} KDRV_URECT;

typedef struct {
	INT32  x;       ///< X coordinate of the point
	INT32  y;       ///< Y coordinate of the point
} KDRV_IPOINT;

typedef struct {
	INT32  w;       ///< Rectangle width
	INT32  h;       ///< Rectangle height
} KDRV_ISIZE;

typedef struct {
	INT32  x;       ///< X coordinate of the top-left point of the rectangle
	INT32  y;       ///< Y coordinate of the top-left point of the rectangle
	INT32  w;       ///< Rectangle width
	INT32  h;       ///< Rectangle height
} KDRV_IRECT;

typedef enum
{
    KDRV_OK                       = 0,
    KDRV_ERR_NG                   = -1,
    KDRV_NOT_SUPPORTED            = -2,
} KDRV_RESULT; ///< KDRV error code

typedef struct {
	INT32 ddr_id;
	INT32 addr_pa;
	INT32 addr_va;
	INT32 size;
} KDRV_BUFFER_INFO;

typedef enum {
	KDRV_BUFTYPE_UNKNOWN,
	KDRV_BUFTYPE_YUV422,
	KDRV_BUFTYPE_YUV420,
	KDRV_BUFTYPE_YUV420_SCE,
	KDRV_BUFTYPE_YUV420_MB,
	KDRV_BUFTYPE_YUV420_16x2,
	KDRV_BUFTYPE_YUV420_SP8,
	KDRV_BUFTYPE_ARGB1555,
	KDRV_BUFTYPE_ARGB8888,
	KDRV_BUFTYPE_RGB565,
	KDRV_BUFTYPE_RGB_CV,
} KDRV_PIX_FMT;


typedef struct {
	KDRV_PIX_FMT pix_fmt;
	KDRV_USIZE dim;
} KDRV_VIDEO_FRAME_INFO;


typedef struct {
	INT32 (*callback)(VOID *callback_info, VOID *user_data);
	INT32 (*reserve_buf)(UINT32 phy_addr);
	INT32 (*free_buf)(UINT32 phy_addr);
} KDRV_CALLBACK_FUNC;

typedef struct {
	void (*VdoDec_RefFrmDo)(UINT32 pathID, UINT32 uiYAddr, BOOL bIsRef);
	UINT32 id;
} KDRV_VDODEC_REFFRMCB;

#endif



