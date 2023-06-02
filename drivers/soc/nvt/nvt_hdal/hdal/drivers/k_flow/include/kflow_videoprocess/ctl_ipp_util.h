/**
    Public header file for IPL utility

    This file is the header file that define the API and data type for IPL utility tool.

    @file       ipl_utility.h
    @ingroup    mILibIPLCom
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/
#ifndef _IPP_UTIL_H_
#define _IPP_UTIL_H_

#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "kflow_common/type_vdo.h"


#define CTL_IPP_520_MAX_STRIPE_W 2688
#define CTL_IPP_528_MAX_STRIPE_W 4092
#define CTL_IPP_560_MAX_STRIPE_W 2688
/**
	IPP Error Type
*/
#define CTL_IPP_E_OK			(0)
#define CTL_IPP_E_ID			(-1)	///< illegal handle
#define CTL_IPP_E_QOVR			(-2)	///< queue overflow
#define CTL_IPP_E_NSPT			(-3)	///< not support function
#define CTL_IPP_E_STATE			(-4)	///< illegal state
#define CTL_IPP_E_PAR			(-5)	///< illegal parameter
#define CTL_IPP_E_INDATA		(-6)	///< input data error
#define CTL_IPP_E_SYS			(-7)	///< system error
#define CTL_IPP_E_FLUSH			(-8)	///< flush buffer
#define CTL_IPP_E_NOMEM			(-9)	///< no memory
#define CTL_IPP_E_TIMEOUT		(-10)	///< timeout
#define CTL_IPP_E_SHDR			(-11)	///< SHDR start and stop type for direct
#define CTL_IPP_E_DIR_DROP		(-12)	///< busy drop for direct
#define CTL_IPP_E_NULL_FNNC		(-13)	///< null fucntion
#define CTL_IPP_E_KDRV_OPEN		(-100)	///< kdrv open error
#define CTL_IPP_E_KDRV_CLOSE	(-101)	///< kdrv close error
#define CTL_IPP_E_KDRV_SET		(-102)	///< kdrv set error
#define CTL_IPP_E_KDRV_GET		(-103)	///< kdrv get error
#define CTL_IPP_E_KDRV_TRIG		(-104)	///< kdrv trigger error
#define CTL_IPP_E_KDRV_STRP		(-105)	///< kdrv stripe cal error


/**
    IPP flip information
*/
typedef enum _CTL_IPP_FLIP_TYPE {
	CTL_IPP_FLIP_NONE = 0x00000000,         ///< no flip
	CTL_IPP_FLIP_H    = 0x00000001,         ///< H flip
	CTL_IPP_FLIP_V    = 0x00000002,         ///< V flip
	CTL_IPP_FLIP_H_V  = 0x00000003,         ///< H & V flip(Flip_H | Flip_V) don't modify
	CTL_IPP_FLIP_MAX,
	ENUM_DUMMY4WORD(CTL_IPP_FLIP_TYPE)
} CTL_IPP_FLIP_TYPE;

/**
    IPP In/Out buffer config
*/
typedef enum _CTL_IPP_BUF_IO_CFG {
	CTL_IPP_BUF_IO_NEW = 0,
	CTL_IPP_BUF_IO_PUSH,
	CTL_IPP_BUF_IO_LOCK,
	CTL_IPP_BUF_IO_UNLOCK,
	CTL_IPP_BUF_IO_START,
	CTL_IPP_BUF_IO_STOP,
	CTL_IPP_BFU_IO_DRAM_START,
	CTL_IPP_BFU_IO_DRAM_END,
	ENUM_DUMMY4WORD(CTL_IPP_BUF_IO_CFG)
} CTL_IPP_BUF_IO_CFG;

/**
	IPP OUT PATH ID
*/
typedef enum _CTL_IPP_OUT_PATH_ID{
	CTL_IPP_OUT_PATH_ID_1 = 0,
	CTL_IPP_OUT_PATH_ID_2 = 1,
	CTL_IPP_OUT_PATH_ID_3 = 2,
	CTL_IPP_OUT_PATH_ID_4 = 3,
	CTL_IPP_OUT_PATH_ID_5 = 4,
	CTL_IPP_OUT_PATH_ID_6 = 5,
	CTL_IPP_OUT_PATH_ID_MAX = 6,
} CTL_IPP_OUT_PATH_ID;

/**
	IPP Scale Method
*/
typedef enum _CTL_IPP_IME_SCALER {
	CTL_IPP_SCL_BICUBIC     = 0,    ///< bicubic interpolation
	CTL_IPP_SCL_BILINEAR    = 1,    ///< bilinear interpolation
	CTL_IPP_SCL_NEAREST     = 2,    ///< nearest interpolation
	CTL_IPP_SCL_INTEGRATION = 3,    ///< integration interpolation,support only scale down
	CTL_IPP_SCL_AUTO        = 4,    ///< automatical calculation
	CTL_IPP_SCL_METHOD_MAX,
	ENUM_DUMMY4WORD(CTL_IPP_SCL_METHOD)
} CTL_IPP_SCL_METHOD;

typedef struct {
	UINT32 scl_th;                  /* scale done ratio threshold, [31..16]output, [15..0]input */
	CTL_IPP_SCL_METHOD method_l;    /* scale method when scale down ratio <= scl_th(output/input) */
	CTL_IPP_SCL_METHOD method_h;    /* scale method when scale down ratio >  scl_th(output/input) */
} CTL_IPP_SCL_METHOD_SEL;


/**
	IPP Func Enable
*/
typedef enum _CTL_IPP_FUNC {
	CTL_IPP_FUNC_NONE			=	0x00000000,
	CTL_IPP_FUNC_WDR			=	0x00000001,
	CTL_IPP_FUNC_SHDR			=	0x00000002,
	CTL_IPP_FUNC_DEFOG			=	0x00000004,
	CTL_IPP_FUNC_3DNR			=	0x00000008,
	CTL_IPP_FUNC_DATASTAMP		=	0x00000010,
	CTL_IPP_FUNC_PRIMASK		=	0x00000020,
	CTL_IPP_FUNC_PM_PIXELIZTION	=	0x00000040,
	CTL_IPP_FUNC_YUV_SUBOUT		=	0x00000080,
	CTL_IPP_FUNC_VA_SUBOUT		=	0x00000100,
	CTL_IPP_FUNC_3DNR_STA		=	0x00000200,
	CTL_IPP_FUNC_GDC			=	0x00000400,
} CTL_IPP_FUNC;

/**
	IPP Crop mode
*/
typedef enum _CTL_IPP_IN_CROP_MODE {
	CTL_IPP_IN_CROP_AUTO = 0,	/* reference input header information */
	CTL_IPP_IN_CROP_NONE,		/* no crop, process full image */
	CTL_IPP_IN_CROP_USER,		/* user define crop window */
	CTL_IPP_IN_CROP_MODE_MAX,
} CTL_IPP_IN_CROP_MODE;

/**
	IPP Color space type
*/
typedef enum _CTL_IPP_OUT_COLOR_SPACE {
	CTL_IPP_OUT_COLOR_FULL,		/* full range */
	CTL_IPP_OUT_COLOR_BT601,	/* BT.601 */
	CTL_IPP_OUT_COLOR_BT709,	/* BT.709 */
	CTL_IPP_OUT_COLOR_MAX
} CTL_IPP_OUT_COLOR_SPACE;

/**
	IPP Data stamp
*/

typedef enum {
	CTL_IPP_DS_SET_ID_1 = 0,
	CTL_IPP_DS_SET_ID_2,
	CTL_IPP_DS_SET_ID_3,
	CTL_IPP_DS_SET_ID_4,
	CTL_IPP_DS_SET_ID_MAX
} CTL_IPP_DS_SET_ID;

typedef enum {
	CTL_IPP_DS_CKEY_MODE_RGB = 0,
	CTL_IPP_DS_CKEY_MODE_ARGB,
	CTL_IPP_DS_CKEY_MODE_MAX,
	ENUM_DUMMY4WORD(CTL_IPP_DS_CKEY_MODE)
} CTL_IPP_DS_CKEY_MODE;

typedef enum {
	CTL_IPP_DS_PLT_MODE_1BIT = 0,
	CTL_IPP_DS_PLT_MODE_2BIT,
	CTL_IPP_DS_PLT_MODE_4BIT,
	CTL_IPP_DS_PLT_MODE_MAX,
	ENUM_DUMMY4WORD(CTL_IPP_DS_PLT_MODE)
} CTL_IPP_DS_PLT_MODE;

typedef struct {
	USIZE size;			/* image size */
	VDO_PXLFMT fmt;		/* image format */
	UPOINT pos;			/* blending position */
	UINT32 lofs;		/* data lineoffset */
	UINT32 addr;		/* data address */
} CTL_IPP_DS_IMG;

typedef struct {
	BOOL color_key_en;						/* color key enable, for RGB565/ARGB1555 format */
	CTL_IPP_DS_CKEY_MODE color_key_mode;	/* color key mode*/
	UINT32 color_key_val;					/* Color key, valid when color_key_em = TRUE; */
											/* bit[7..0]:   B,  */
											/* bit[15..8]:  G,  */
											/* bit[23..16]: R,  */
											/* bit[31..24]: A,  */
	BOOL plt_en;			/* palette mode enable */
	UINT32 bld_wt_0;		/* Blending weighting, if RGB565, range: [0, 16]; others, range: [0, 15] */
	UINT32 bld_wt_1;		/* Blending weighting, range: [0, 15] */
} CTL_IPP_DS_IQ;

typedef struct {
	UINT32 reserved;
	BOOL func_en;
	CTL_IPP_DS_IMG img_info;
	CTL_IPP_DS_IQ iq_info;
} CTL_IPP_DS_INFO;

typedef struct {
	BOOL func_en;		/* color space trans enable */
	BOOL auto_mode_en;	/* cst auto parameter mode enable */
	UINT32 coef[4];		/* coefficient of color space transform */
} CTL_IPP_DS_CST;

typedef struct {
	CTL_IPP_DS_PLT_MODE mode;
	UINT8 plt_a[16];
	UINT8 plt_r[16];
	UINT8 plt_g[16];
	UINT8 plt_b[16];
} CTL_IPP_DS_PLT;

/* CTL_IPP_CBEVT_DATASTAMP callback structure*/
typedef struct {
	CTL_IPP_DS_CST cst_info;
	CTL_IPP_DS_PLT plt_info;
	CTL_IPP_DS_INFO stamp[CTL_IPP_DS_SET_ID_MAX];
} CTL_IPP_DS_CB_OUTPUT_INFO;

typedef struct {
	UINT32 ctl_ipp_handle;
	USIZE img_size;
} CTL_IPP_DS_CB_INPUT_INFO;


/**
	IPP Privacy Mask
*/
typedef enum {
	CTL_IPP_PM_SET_ID_1 = 0,
	CTL_IPP_PM_SET_ID_2,
	CTL_IPP_PM_SET_ID_3,
	CTL_IPP_PM_SET_ID_4,
	CTL_IPP_PM_SET_ID_5,
	CTL_IPP_PM_SET_ID_6,
	CTL_IPP_PM_SET_ID_7,
	CTL_IPP_PM_SET_ID_8,
	CTL_IPP_PM_SET_ID_MAX
} CTL_IPP_PM_SET_ID;

typedef enum {
	CTL_IPP_PM_MASK_TYPE_YUV,
	CTL_IPP_PM_MASK_TYPE_PXL,
	CTL_IPP_PM_MASK_TYPE_MAX,
	ENUM_DUMMY4WORD(CTL_IPP_PM_MASK_TYPE)
} CTL_IPP_PM_MASK_TYPE;

typedef enum {
	CTL_IPP_PM_PXL_BLK_08,
	CTL_IPP_PM_PXL_BLK_16,
	CTL_IPP_PM_PXL_BLK_32,
	CTL_IPP_PM_PXL_BLK_64,
	CTL_IPP_PM_PXL_BLK_MAX
} CTL_IPP_PM_PXL_BLK;

typedef struct {
	UINT32 reserved;
	BOOL func_en;					/* privacy mask set enable */
	BOOL hollow_mask_en;			/* hollow mask enable
										only support yuv mask type
										only support pm1/3/5/7
										when enabled pm2/4/6/8 must be disabled
										pm_coord is outside convex
										pm_coord_2 is inner convex
									*/
	IPOINT pm_coord[4];				/* privacy mask 4 coordinates(horizontal and vertical direction), must be Convex Hull */
	IPOINT pm_coord_2[4];			/* privacy mask 4 coordinates(horizontal and vertical direction), must be Convex Hull */
	CTL_IPP_PM_MASK_TYPE msk_type;	/* privacy mask type selection */
	UINT32 color[3];				/* privacy mask using YUV color, range: [0, 255] */
	UINT32 alpha_weight;			/* privacy mask alpha weight, range: [0, 255] */
} CTL_IPP_PM;

/* CTL_IPP_CBEVT_PRIMASK callback structure */
typedef struct {
	CTL_IPP_PM_PXL_BLK pxl_blk_size;		/* block size selectino for pixelation mask */
	CTL_IPP_PM mask[CTL_IPP_PM_SET_ID_MAX];
} CTL_IPP_PM_CB_OUTPUT_INFO;

typedef struct {
	UINT32 ctl_ipp_handle;
	USIZE img_size;
} CTL_IPP_PM_CB_INPUT_INFO;

/**
	Engine ISR message
*/

typedef enum {
	CTL_IPP_RHE_INTE_FMD = 0x00000001,
} CTL_IPP_RHE_INTE_STS;

typedef enum {
	CTL_IPP_IFE_INTE_FMD 			 = 0x00000001,	///< enable interrupt: frame end
	CTL_IPP_IFE_INTE_DEC1_ERR        = 0x00000002,  ///< enable interrupt: rde decode error interrupt
	CTL_IPP_IFE_INTE_DEC2_ERR        = 0x00000004,  ///< enable interrupt: rde decode error interrupt
	CTL_IPP_IFE_INTE_LLEND           = 0x00000008,  ///< enable interrupt: LinkedList end interrupt
	CTL_IPP_IFE_INTE_LLERR           = 0x00000010,  ///< enable interrupt: LinkedList error  interrupt
	CTL_IPP_IFE_INTE_LLERR2          = 0x00000020,  ///< enable interrupt: LinkedList error2 interrupt
	CTL_IPP_IFE_INTE_LLJOBEND        = 0x00000040,  ///< enable interrupt: LinkedList job end interrupt
	CTL_IPP_IFE_INTE_BUFOVFL         = 0x00000080,  ///< enable interrupt: buffer overflow interrupt
	CTL_IPP_IFE_INTE_RING_BUF_ERR    = 0x00000100,  ///< enable interrupt: ring buffer error interrupt
	CTL_IPP_IFE_INTE_FRAME_ERR       = 0x00000200,  ///< enable interrupt: frame error interrupt (for direct mode)
	CTL_IPP_IFE_INTE_SIE_FRAME_START = 0x00001000,  ///< enable interrupt: SIE frame start interrupt (for direct mode)
} CTL_IPP_IFE_INTE_STS;

typedef enum {
	CTL_IPP_DCE_INTE_FST       = 0x00000001,   // enable interrupt: frame start
	CTL_IPP_DCE_INTE_FMD       = 0x00000002,   // enable interrupt: frame end
	CTL_IPP_DCE_INTE_STPERR    = 0x00000008,   // enable interrupt: DCE stripe error
	CTL_IPP_DCE_INTE_LBOVF     = 0x00000010,   // enable interrupt: DCE line buffer overflow error
	CTL_IPP_DCE_INTE_STPOB     = 0x00000040,   // enable interrupt: DCE stripe boundary overflow error
	CTL_IPP_DCE_INTE_YBACK     = 0x00000080,   // enable interrupt: DCE Y coordinate backward skip error
	CTL_IPP_DCE_INTE_LL_END    = 0x00000100,   // enable interrupt: Linked List done
	CTL_IPP_DCE_INTE_LL_ERR    = 0x00000200,   // enable interrupt: Linked List error command
	CTL_IPP_DCE_INTE_LL_ERR2   = 0x00000400,   // enable interrupt: Linked List error in direct mdoe
	CTL_IPP_DCE_INTE_LL_JOBEND = 0x00000800,   // enable interrupt: Linked List job end
	CTL_IPP_DCE_INTE_FRAMEERR  = 0x00001000,   // enable interrupt: frame start error in direct mode
} CTL_IPP_DCE_INTE_STS;

typedef enum {
	CTL_IPP_IPE_INTE_FMD			= 0x00000002,		///< enable interrupt: frame done
	CTL_IPP_IPE_INTE_STP			= 0x00000004,		///< enable interrupt: current stripe done
	CTL_IPP_IPE_INTE_FMS			= 0x00000008,		///< enable interrupt: frame start
	CTL_IPP_IPE_INTE_YCC_OUT_END	= 0x00000010,		///< enable interrupt: YCC DRAM output done
	CTL_IPP_IPE_INTE_GMA_IN_END		= 0x00000020,		///< enable interrupt: DRAM input done (gamma)
	CTL_IPP_IPE_INTE_DEFOG_IN_END	= 0x00000040,		///< enable interrupt: DRAM input done (defog)
	CTL_IPP_IPE_INTE_VA_OUT_END		= 0x00000080,		///< enable interrupt: DRAM VA output done
	CTL_IPP_IPE_INTE_LL_DONE		= 0x00000100,		///< enable interrupt: Linked List done
	CTL_IPP_IPE_INTE_LL_JOBEND		= 0x00000200,		///< enable interrupt: Linked List job end
	CTL_IPP_IPE_INTE_LL_ERR			= 0x00000400,		///< enable interrupt: Linked List error command
	CTL_IPP_IPE_INTE_LL_ERR2		= 0x00000800,		///< enable interrupt: Linked List error in direct mode
	CTL_IPP_IPE_INTE_FRAMEERR		= 0x00001000,		///< enable interrupt: frame start error in direct mode
} CTL_IPP_IPE_INTE_STS;

typedef enum {
	CTL_IPP_IME_INTE_LL_END              = 0x00000001,
	CTL_IPP_IME_INTE_LL_ERR              = 0x00000002,
	CTL_IPP_IME_INTE_LL_LATE             = 0x00000004,
	CTL_IPP_IME_INTE_LL_JEND             = 0x00000008,
	CTL_IPP_IME_INTE_BP1                 = 0x00000010,
	CTL_IPP_IME_INTE_BP2                 = 0x00000020,
	CTL_IPP_IME_INTE_BP3                 = 0x00000040,
	CTL_IPP_IME_INTE_TMNR_SLICE_END      = 0x00000080,
	CTL_IPP_IME_INTE_TMNR_MOT_END        = 0x00000100,
	CTL_IPP_IME_INTE_TMNR_MV_END         = 0x00000200,
	CTL_IPP_IME_INTE_TMNR_STA_END        = 0x00000400,
	CTL_IPP_IME_INTE_P1_ENC_OVR          = 0x00000800,
	CTL_IPP_IME_INTE_TMNR_ENC_OVR        = 0x00001000,
	CTL_IPP_IME_INTE_TMNR_DEC_ERR        = 0x00002000,
	CTL_IPP_IME_INTE_FRM_ERR             = 0x00004000,
	CTL_IPP_IME_INTE_FRM_START           = 0x20000000,  ///< frame-start enable
	CTL_IPP_IME_INTE_STRP_END            = 0x40000000,  ///< stripe-end enable
	CTL_IPP_IME_INTE_FMD             	 = 0x80000000,  ///< frame-end enable
} CTL_IPP_IME_INTE_STS;

typedef enum {
	CTL_IPP_IFE2_INTE_LL_END  = 0x00000001,
	CTL_IPP_IFE2_INTE_LL_ERR  = 0x00000002,
	CTL_IPP_IFE2_INTE_LL_LATE = 0x00000004,
	CTL_IPP_IFE2_INTE_OVFL    = 0x40000000,
	CTL_IPP_IFE2_INTE_FMD     = 0x80000000,   ///< enable interrupt: frame end
} CTL_IPP_IFE2_INTE_STS;

#define CTL_IPP_IFE_INTE_STS_ERR_MASK (CTL_IPP_IFE_INTE_DEC1_ERR | CTL_IPP_IFE_INTE_DEC2_ERR | CTL_IPP_IFE_INTE_BUFOVFL | CTL_IPP_IFE_INTE_RING_BUF_ERR | CTL_IPP_IFE_INTE_FRAME_ERR | CTL_IPP_IFE_INTE_LLERR | CTL_IPP_IFE_INTE_LLERR2)
#define CTL_IPP_DCE_INTE_STS_ERR_MASK (CTL_IPP_DCE_INTE_STPERR | CTL_IPP_DCE_INTE_LBOVF | CTL_IPP_DCE_INTE_STPOB | CTL_IPP_DCE_INTE_YBACK | CTL_IPP_DCE_INTE_LL_ERR | CTL_IPP_DCE_INTE_LL_ERR2)
#define CTL_IPP_IPE_INTE_STS_ERR_MASK (CTL_IPP_IPE_INTE_FRAMEERR | CTL_IPP_IPE_INTE_LL_ERR | CTL_IPP_IPE_INTE_LL_ERR2)
#define CTL_IPP_IME_INTE_STS_ERR_MASK (CTL_IPP_IME_INTE_LL_ERR | CTL_IPP_IME_INTE_P1_ENC_OVR | CTL_IPP_IME_INTE_TMNR_ENC_OVR | CTL_IPP_IME_INTE_TMNR_DEC_ERR | CTL_IPP_IME_INTE_FRM_ERR)
#define CTL_IPP_IFE2_INTE_STS_ERR_MASK (CTL_IPP_IFE2_INTE_LL_ERR | CTL_IPP_IFE2_INTE_OVFL)

#endif //_IPP_UTIL_H_
