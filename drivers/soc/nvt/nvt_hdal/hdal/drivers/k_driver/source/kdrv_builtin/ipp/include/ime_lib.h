/**
    Public header file for IME module.

    @file       ime_lib.h
    @ingroup    mIIPPIME

    @brief

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/


#ifndef _IME_LIB_H_
#define _IME_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif



//#include "ime_platform.h"



typedef void (*IME_INT_CB)(UINT32);

//------------------------------------------------------------------------------
//enum data type




//------------------------------------------------------------------------------
/**
    @addtogroup mIDrvIPP_IME
*/
//@{

//define vairable

#define IME_INTE_LL_END             0x00000001
#define IME_INTE_LL_ERR             0x00000002
#define IME_INTE_LL_LATE            0x00000004
#define IME_INTE_LL_JEND            0x00000008
#define IME_INTE_BP1                0x00000010
#define IME_INTE_BP2                0x00000020
#define IME_INTE_BP3                0x00000040
#define IME_INTE_TMNR_SLICE_END     0x00000080
#define IME_INTE_TMNR_MOT_END       0x00000100
#define IME_INTE_TMNR_MV_END        0x00000200
#define IME_INTE_TMNR_STA_END       0x00000400
#define IME_INTE_P1_ENC_OVR         0x00000800
#define IME_INTE_TMNR_ENC_OVR       0x00001000
#define IME_INTE_TMNR_DEC_ERR       0x00002000
#define IME_INTE_FRM_ERR            0x00004000
#define IME_INTE_FRM_START          0x20000000
#define IME_INTE_STRP_END           0x40000000  ///< stripe-end enable
#define IME_INTE_FRM_END            0x80000000  ///< frame-end enable
#define IME_INTE_ALL                (IME_INTE_STRP_END|IME_INTE_FRM_END|IME_INTE_P1_ENC_OVR| \
									 IME_INTE_TMNR_ENC_OVR|IME_INTE_TMNR_DEC_ERR|IME_INTE_LL_END| \
									 IME_INTE_LL_ERR|IME_INTE_LL_LATE|IME_INTE_LL_JEND| \
									 IME_INTE_BP1|IME_INTE_BP2|IME_INTE_BP3| \
									 IME_INTE_TMNR_SLICE_END|IME_INTE_TMNR_MOT_END|IME_INTE_TMNR_MV_END| \
									 IME_INTE_TMNR_STA_END | IME_INTE_FRM_ERR | IME_INTE_FRM_START)   ///< all interrupt enable


#define IME_INTS_LL_END              0x00000001
#define IME_INTS_LL_ERR              0x00000002
#define IME_INTS_LL_LATE             0x00000004
#define IME_INTS_LL_JEND             0x00000008
#define IME_INTS_BP1                 0x00000010
#define IME_INTS_BP2                 0x00000020
#define IME_INTS_BP3                 0x00000040
#define IME_INTS_TMNR_SLICE_END      0x00000080
#define IME_INTS_TMNR_MOT_END        0x00000100
#define IME_INTS_TMNR_MV_END         0x00000200
#define IME_INTS_TMNR_STA_END        0x00000400
#define IME_INTS_P1_ENC_OVR          0x00000800
#define IME_INTS_TMNR_ENC_OVR        0x00001000
#define IME_INTS_TMNR_DEC_ERR        0x00002000
#define IME_INTS_FRM_ERR             0x00004000
#define IME_INTS_FB_FRM_END          0x10000000  ///< fastboot last frame-end
#define IME_INTS_FRM_START           0x20000000
#define IME_INTS_STRP_END            0x40000000  ///< stripe-end status
#define IME_INTS_FRM_END             0x80000000  ///< frame-end status
#define IME_INTS_ALL                 (IME_INTS_STRP_END|IME_INTS_FRM_END|IME_INTS_P1_ENC_OVR| \
									  IME_INTS_TMNR_ENC_OVR|IME_INTS_TMNR_DEC_ERR|IME_INTS_LL_END| \
									  IME_INTS_LL_ERR|IME_INTS_LL_LATE|IME_INTS_LL_JEND| \
									  IME_INTE_BP1|IME_INTE_BP2|IME_INTE_BP3| \
									  IME_INTE_TMNR_SLICE_END|IME_INTE_TMNR_MOT_END|IME_INTE_TMNR_MV_END| \
									  IME_INTE_TMNR_STA_END | IME_INTS_FRM_ERR | IME_INTS_FRM_START)   ///< all interrupt enable

#define IPE_To_IME  0         ///< IPE to IME
#define DRAM_To_IME 1         ///< DMA to IME



//------------------------------------------------------------------------------
// engine control
/**
    IME enum - function enalbe/disable
*/
typedef enum _IME_FUNC_EN {
	IME_FUNC_DISABLE = 0,   ///< function disable
	IME_FUNC_ENABLE  = 1,  ///< function enable
	ENUM_DUMMY4WORD(IME_FUNC_EN)
} IME_FUNC_EN;

/**
    IME enum - operation mode
*/
typedef enum _IME_OPMODE {
	IME_OPMODE_D2D     = 0,    ///< IME only
	IME_OPMODE_IFE_IPP = 1,    ///< IFE->DCE->IPE->IME
	IME_OPMODE_DCE_IPP = 2,    ///< IFE->DCE->IPE->IME
	IME_OPMODE_SIE_IPP = 3,    ///< SIE + IFE->DCE->IPE->IME (all direct)
	IME_OPMODE_UNKNOWN = 4,    ///< Undefined mode
	ENUM_DUMMY4WORD(IME_OPMODE)
} IME_OPMODE;


/**
    IME enum - load type of configned parameters
*/
typedef enum {
	IME_START_LOAD   = 0,  ///< Frame start loaded
	IME_FRMEND_LOAD  = 1,  ///< Frame end loaded
	IME_DIRECT_START_LOAD  = 2,  ///< Frame start loaded when direct mode
	ENUM_DUMMY4WORD(IME_LOAD_TYPE)
} IME_LOAD_TYPE;

/**
    IME enum - Flush buffer selection
*/
typedef enum _IME_BUF_FLUSH_SEL_ {
	IME_DO_BUF_FLUSH       = 0,    ///< to do flush
	IME_NOTDO_BUF_FLUSH    = 1,    ///< not to do flush
	ENUM_DUMMY4WORD(IME_BUF_FLUSH_SEL)
} IME_BUF_FLUSH_SEL;


/**
    IME enum - parameter mode
*/
typedef enum {
	IME_PARAM_AUTO_MODE = 0,    ///< Auto parameter mode
	IME_PARAM_USER_MODE = 1,    ///< User parameter mode
	ENUM_DUMMY4WORD(IME_PARAM_MODE)
} IME_PARAM_MODE;

/**
    IME enum - function selection
*/
typedef enum {
	IME_OUTPUT_P1     = 0,
	IME_OUTPUT_P2     = 1,
	IME_OUTPUT_P3     = 2,
	IME_OUTPUT_P4     = 3,
	//IME_OUTPUT_P5     = 4,
	//IME_ICST          = 5,
	//IME_CST           = 6,
	//IME_CMF           = 7,
	IME_LCA           = 8,
	IME_LCA_SUBOUT    = 9,
	IME_DBCS          = 10,
	//IME_SSR           = 11,
	IME_3DNR          = 12,
	//IME_GRNS          = 13,
	//IME_P2I           = 14,
	IME_STL           = 15,
	IME_OSD0          = 16,
	IME_OSD1          = 17,
	IME_OSD2          = 18,
	IME_OSD3          = 19,
	IME_OUTPUT_P1_ENC = 20,
	IME_3DNR_DEC      = 21,
	IME_PM0           = 22,
	IME_PM1           = 23,
	IME_PM2           = 24,
	IME_PM3           = 25,
	IME_PM4           = 26,
	IME_PM5           = 27,
	IME_PM6           = 28,
	IME_PM7           = 29,
	IME_YUVCVT        = 30,
	IME_FUNC_MAX,
	ENUM_DUMMY4WORD(IME_FUNC_SEL)
} IME_FUNC_SEL;


//------------------------------------------------------------------------------
// input image size control
/**
    IME enum - input stripe mode selection
*/
typedef enum _IME_STRIPE_MODE {
	IME_STRIPE_AUTO_MODE = 0,    ///< Auto stripe information computation mode
	IME_STRIPE_USER_MODE = 1,    ///< User stripe information computation mode
	ENUM_DUMMY4WORD(IME_STRIPE_MODE)
} IME_STRIPE_MODE;

/**
    IME enum - input stripe size mode selection
*/
typedef enum _IME_STRIPE_SIZE_MODE_ {
	IME_STRIPE_SIZE_MODE_FIXED  = 0,    ///< Fixed size mode
	IME_STRIPE_SIZE_MODE_VARIED = 1,    ///< Varied size mode
	ENUM_DUMMY4WORD(IME_STRIPE_SIZE_MODE)
} IME_STRIPE_SIZE_MODE;



/**
    IME enum - input stripe overlap selection
*/
typedef enum _IME_H_STRIPE_OVLP_SEL {
	IME_H_ST_OVLP_16P  = 0,    ///< Horizontal stripe overlap size: 16 pixels
	IME_H_ST_OVLP_24P  = 1,    ///< Horizontal stripe overlap size: 24 pixels
	IME_H_ST_OVLP_32P  = 2,    ///< Horizontal stripe overlap size: 32 pixels
	IME_H_ST_OVLP_USER = 3,    ///< Horizontal stripe overlap size: user define
	ENUM_DUMMY4WORD(IME_H_STRIPE_OVLP_SEL)
} IME_H_STRIPE_OVLP_SEL;

/**
    IME enum - input stripe overlap partition selection
*/
typedef enum _IME_H_STRIPE_PRT_SEL {
	IME_H_ST_PRT_5P  = 0,    ///< Horizontal stripe overlap partition size: 5 pixels
	IME_H_ST_PRT_3P  = 1,    ///< Horizontal stripe overlap partition size: 3 pixels
	IME_H_ST_PRT_2P  = 2,    ///< Horizontal stripe overlap partition size: 2 pixels
	IME_H_ST_PRT_USER = 3,    ///< Horizontal stripe overlap partition size: user define
	ENUM_DUMMY4WORD(IME_H_STRIPE_PRT_SEL)
} IME_H_STRIPE_PRT_SEL;


/**
    IME enum - input image data format selection
*/
typedef enum _IME_INPUT_FORMAT_SEL {
#if 0
	IME_INPUT_YCC_444  = 0,         ///< YCC444 separate planar format for input
	IME_INPUT_YCC_422  = 1,         ///< YCC422 separate planar format for input
	IME_INPUT_YCC_420  = 2,         ///< YCC420 separate planar format for input
	IME_INPUT_RESERVED = 3,         ///< reversed
	IME_INPUT_YCCP_422 = 4,         ///< YCC422 UV-packing format for input
	IME_INPUT_YCCP_420 = 5,         ///< YCC420 UV-packing format for input
	IME_INPUT_Y_ONLY   = 6,         ///< Only Y channel for input
	IME_INPUT_RGB      = 7,         ///< RGB for input (YCC444)
#endif
	IME_INPUT_YCC_420  = 0,         ///< YCC420 separate planar format for input
	IME_INPUT_YCCP_420 = 1,         ///< YCC420 UV-packing format for input
	IME_INPUT_Y_ONLY   = 2,         ///< Only Y channel for input
	ENUM_DUMMY4WORD(IME_INPUT_FORMAT_SEL)
} IME_INPUT_FORMAT_SEL;

/**
    IME enum - image data format selection of LCA function
*/
typedef enum _IME_LCA_FORMAT_SEL {
	IME_LCAF_YCCYCC  = 0,    ///< YCbCrYCbCr data format
	IME_LCAF_YCCP    = 1,    ///< Y / CbCr data format
	ENUM_DUMMY4WORD(IME_LCA_FORMAT_SEL)
} IME_LCA_FORMAT_SEL;

//------------------------------------------------------------------------------
// output control
/**
    IME enum - output path selection
*/
typedef enum _IME_PATH_SEL {
	IME_PATH1_SEL = 0x01,   ///< operation for output path1
	IME_PATH2_SEL = 0x02,   ///< operation for output path2
	IME_PATH3_SEL = 0x04,   ///< operation for output path3
	IME_PATH4_SEL = 0x08,   ///< operation for output path4
	ENUM_DUMMY4WORD(IME_PATH_SEL)
} IME_PATH_SEL;

/**
    IME enum - output buffer selection
*/
typedef enum _IME_PATH_OUTBUF_SEL {
	IME_PATH_OUTBUF_SET0 = 0,   ///< output buffer set0 selection
	IME_PATH_OUTBUF_SET1 = 1,   ///< output buffer set1 selection
	ENUM_DUMMY4WORD(IME_PATH_OUTBUF_SEL)
} IME_PATH_OUTBUF_SEL;

/**
    IME enum - output image data format type
*/
typedef enum _IME_OUTPUT_FORMAT_TYPE {
	IME_OUTPUT_YCC_PLANAR   = 0,         ///< Y/Cb/Cr separate planar type
	IME_OUTPUT_YCC_UVPACKIN = 1,         ///< Y/UV-packing type
	ENUM_DUMMY4WORD(IME_OUTPUT_FORMAT_TYPE)
} IME_OUTPUT_FORMAT_TYPE;

/**
    IME enum - output image data format selection
*/
typedef enum _IME_OUTPUT_IMG_FORMAT_SEL {
#if 0
	IME_OUTPUT_YCC_444     = 0,         ///< YCC444 format for output
	IME_OUTPUT_YCC_422_COS = 1,         ///< YCC422 cosited format for output
	IME_OUTPUT_YCC_422_CEN = 2,         ///< YCC422 centered format for output
	IME_OUTPUT_YCC_420_COS = 3,         ///< YCC420 cosited format for output
	IME_OUTPUT_YCC_420_CEN = 4,         ///< YCC420 centered format for output
	IME_OUTPUT_Y_ONLY      = 6,         ///< Y only for output
	IME_OUTPUT_RGB_444     = 7,         ///< RGB444 format for output
#endif
	IME_OUTPUT_YCC_420     = 0,         ///< YCC420 format for output
	IME_OUTPUT_Y_ONLY      = 1,         ///< Y only for output
	ENUM_DUMMY4WORD(IME_OUTPUT_IMG_FORMAT_SEL)
} IME_OUTPUT_IMG_FORMAT_SEL;

/**
    IME enum - scale method selection
*/
typedef enum _IME_SCALE_METHOD_SEL {
	IMEALG_BICUBIC     = 0,        ///< bicubic interpolation
	IMEALG_BILINEAR    = 1,        ///< bilinear interpolation
	IMEALG_NEAREST     = 2,        ///< nearest interpolation
	IMEALG_INTEGRATION = 3,        ///< nearest interpolation
	ENUM_DUMMY4WORD(IME_SCALE_METHOD_SEL)
} IME_SCALE_METHOD_SEL;

/**
    IME enum - scale filter computation mode selection
*/
typedef enum _IME_SCALE_FILTER_COEF_MODE {
	IME_SCALE_FILTER_COEF_AUTO_MODE = 0,    ///< Auto Scale filter coefficeint computation mode
	IME_SCALE_FILTER_COEF_USER_MODE = 1,    ///< User Scale filter coefficeint computation mode
	ENUM_DUMMY4WORD(IME_SCALE_FILTER_COEF_MODE)
} IME_SCALE_FILTER_COEF_MODE;

/**
    IME enum - scale type selection
*/
typedef enum _IME_SCALE_TYPE_SEL {
	IME_SCALE_DOWN = 0,  ///< scaling down enable
	IME_SCALE_UP = 1,    ///< scaling up enable
	ENUM_DUMMY4WORD(IME_SCALE_TYPE_SEL)
} IME_SCALE_TYPE_SEL;

/**
    IME enum - scale factor computation mode selection
*/
typedef enum _IME_SCALE_FACTOR_COEF_MODE {
	IME_SCALE_FACTOR_COEF_AUTO_MODE = 0,    ///< Auto Scale factor coefficeint computation mode
	IME_SCALE_FACTOR_COEF_USER_MODE = 1,    ///< User Scale factor coefficeint computation mode
	ENUM_DUMMY4WORD(IME_SCALE_FACTOR_COEF_MODE)
} IME_SCALE_FACTOR_COEF_MODE;

/**
    ISD user coefficient control selection
*/
typedef enum _IME_ISD_UCOEF_CTRL_SEL_ {
	IME_ISD_WITHOUT_UCOEF = 0,   ///< without user coefficient control
	IME_ISD_WITH_UCOEF = 1,      ///< with user coefficient control
	ENUM_DUMMY4WORD(IME_ISD_UCOEF_CTRL_SEL)
} IME_ISD_UCOEF_CTRL_SEL;


/**
    IME enum - outupt path destination selection
*/
typedef enum _IME_OUTDST_CTRL_SEL {
	IME_OUTDST_DRAM = 0,         ///< output to Dram
	IME_OUTDST_H264 = 1,         ///< output to H.264
	ENUM_DUMMY4WORD(IME_OUTDST_CTRL_SEL)
} IME_OUTDST_CTRL_SEL;

//------------------------------------------------------------------------------
/**
    IME enum - system flag clear selection
*/
typedef enum _IME_FLAG_CLEAR_SEL {
	IME_FLAG_NO_CLEAR = 0,    ///< No clear flag
	IME_FLAG_CLEAR    = 1,    ///< Clear flag
	ENUM_DUMMY4WORD(IME_FLAG_CLEAR_SEL)
} IME_FLAG_CLEAR_SEL;

//------------------------------------------------------------------------------

/**
    IME enum - scale enhancing mode selection
*/
typedef enum _IME_SCALE_ENH_MODE {
	IME_SCALE_ENH_MODE_AUTO = 0,  ///< Scaling enhancement, parameter setting by auto
	IME_SCALE_ENH_MODE_USER = 1,  ///< Scaling enhancement, parameter setting by user
	ENUM_DUMMY4WORD(IME_SCALE_ENH_MODE)
} IME_SCALE_ENH_MODE;


/**
    IME enum - value range selection

    @note used for chroma adaptation
*/
typedef enum _IME_RANGE_SEL {
	IME_RANGE_8   = 0,  ///< range 8
	IME_RANGE_16  = 1,  ///< range 16
	IME_RANGE_32  = 2,  ///< range 32
	IME_RANGE_64  = 3,  ///< range 64
	IME_RANGE_128 = 4,  ///< range 128
	ENUM_DUMMY4WORD(IME_RANGE_SEL)
} IME_RANGE_SEL;


/**
    IME enum - Chroma adaptation input source
*/
typedef enum {
	IME_LCA_SRC_DRAM = 0, ///< From DRAM, using for capture mode
	IME_LCA_SRC_IFE2 = 1, ///< From IFE2 directly, using for video mode
	IME_LCA_SRC_ALL  = 2, ///< From IFE2 and setting all paramters
} IME_LCA_SRC;

/**
    IME enum - Chroma adaptation sub-image output source selection
*/
typedef enum {
	IME_LCA_SUBOUT_SRC_A = 0, ///< source: after LCA
	IME_LCA_SUBOUT_SRC_B = 1, ///< source: before LCA
} IME_LCA_SUBOUT_SRC;


/**
    IME enum - dark and bright chroma suppression mode selection
*/
typedef enum _IME_DBCS_MODE_SEL {
	IME_DBCS_DK_MODE  = 0,  ///< dark mode
	IME_DBCS_BT_MODE  = 1,  ///< bright mode
	IME_DBCS_BOTH_MODE  = 2,  ///< dark and bright mode
	ENUM_DUMMY4WORD(IME_DBCS_MODE_SEL)
} IME_DBCS_MODE_SEL;


/**
    IME enum - data stamp processing selection
*/
typedef enum _IME_DS_SETNUM {
	IME_DS_SET0  = 0,  ///< Set0
	IME_DS_SET1  = 1,  ///< Set1
	IME_DS_SET2  = 2,  ///< Set2
	IME_DS_SET3  = 3,  ///< Set3
	ENUM_DUMMY4WORD(IME_DS_SETNUM)
} IME_DS_SETNUM;


/**
    IME enum - data stamp color LUT mode selection
*/
typedef enum _IME_DS_FMT_SEL {
	IME_DS_FMT_RGB565   = 0,  ///< data stamp format, RGB565
	IME_DS_FMT_RGB1555  = 1,  ///< data stamp format, RGB1555
	IME_DS_FMT_RGB4444  = 2,  ///< data stamp format, RGB4444
	ENUM_DUMMY4WORD(IME_DS_FMT_SEL)
} IME_DS_FMT_SEL;

/**
    IME enum - data stamp color key mode selection
*/
typedef enum _IME_DS_CK_MODE_SEL_ {
	IME_DS_CK_MODE_RGB   = 0,  ///< color key mode selection, RGB mode
	IME_DS_CK_MODE_ARGB  = 1,  ///< color key mode selection, ARGB mode
	ENUM_DUMMY4WORD(IME_DS_CK_MODE_SEL)
} IME_DS_CK_MODE_SEL;

/**
    IME enum - data stamp color palette mode selection
*/
typedef enum _IME_DS_PLT_SEL_ {
	IME_DS_PLT_1BIT   = 0,  ///< color palette, 1bit mode
	IME_DS_PLT_2BIT   = 1,  ///< color palette, 2bit mode
	IME_DS_PLT_4BIT   = 2,  ///< color palette, 4bit mode
	ENUM_DUMMY4WORD(IME_DS_PLT_SEL)
} IME_DS_PLT_SEL;



/**
    IME enum - SSR processing mode
*/
typedef enum _IME_SSR_MODE_SEL {
	IME_SSR_MODE_USER  = 0,  ///< user mode of SSR, function enable/disable is controlled by user
	IME_SSR_MODE_AUTO  = 1,  ///< auto mode of SSR, function enable/disable is controlled by IME driver
	ENUM_DUMMY4WORD(IME_SSR_MODE_SEL)
} IME_SSR_MODE_SEL;


/**
    IME enum - statistical edge kernel selection
*/
typedef enum _IME_STL_EKER_SEL {
	IME_STL_EKER_HA  = 0,  ///< H map A
	IME_STL_EKER_HB  = 1,  ///< H map B
	IME_STL_EKER_VA  = 2,  ///< V map A
	IME_STL_EKER_VB  = 3,  ///< V map B
	ENUM_DUMMY4WORD(IME_STL_EKER_SEL)
} IME_STL_EKER_SEL;


/**
    IME enum - statistical source selection
*/
typedef enum _IME_STL_SRC_SEL {
	IME_STL_SRC_SET0  = 0,  ///< source from set0
	IME_STL_SRC_SET1  = 1,  ///< source from set1
	ENUM_DUMMY4WORD(IME_STL_SRC_SEL)
} IME_STL_SRC_SEL;


/**
    IME enum - histogram operating selection
*/
typedef enum _IME_STL_HIST_SET_SEL {
	IME_STL_HIST_SET0  = 0,  ///< set0 selection
	IME_STL_HIST_SET1  = 1,  ///< set1 selection
	ENUM_DUMMY4WORD(IME_STL_HIST_SET_SEL)
} IME_STL_HIST_SET_SEL;




/**
    IME enum - output image data type selection for Statistical
*/
typedef enum _IME_STL_IMGOUT_SEL {
	IME_STL_IMGOUT_AF  = 0,  ///< output after filter
	IME_STL_IMGOUT_BF  = 1,  ///< output before filter
	ENUM_DUMMY4WORD(IME_STL_IMGOUT_SEL)
} IME_STL_IMGOUT_SEL;


/**
    IME enum - privacy mask set number selection
*/
typedef enum _IME_PM_SET_SEL {
	IME_PM_SET0 = 0,  ///< set0
	IME_PM_SET1 = 1,  ///< set1
	IME_PM_SET2 = 2,  ///< set2
	IME_PM_SET3 = 3,  ///< set3
	IME_PM_SET4 = 4,  ///< set4
	IME_PM_SET5 = 5,  ///< set5
	IME_PM_SET6 = 6,  ///< set6
	IME_PM_SET7 = 7,  ///< set7
	ENUM_DUMMY4WORD(IME_PM_SET_SEL)
} IME_PM_SET_SEL;

/**
    IME enum - privacy mask type
*/
typedef enum _IME_PM_MASK_TYPE {
	IME_PM_MASK_TYPE_YUV = 0,  ///< Using YUV color
	IME_PM_MASK_TYPE_PXL = 1,  ///< Using pixilation
	ENUM_DUMMY4WORD(IME_PM_MASK_TYPE)
} IME_PM_MASK_TYPE;



/**
    IME enum - privacy mask pixelation blocks size selection
*/
typedef enum _IME_PM_PXL_BLK_SIZE {
	IME_PM_PIXELATION_08 = 0,  ///< 8x8
	IME_PM_PIXELATION_16 = 1,  ///< 16x16
	IME_PM_PIXELATION_32 = 2,  ///< 32x32
	IME_PM_PIXELATION_64 = 3,  ///< 64x64
	ENUM_DUMMY4WORD(IME_PM_PXL_BLK_SIZE)
} IME_PM_PXL_BLK_SIZE;



/**
    IME enum - formation of side-information of codec
*/
typedef enum _IME_SINFO_FMT_ {
	IME_SINFO_FMT_HEVC = 0,  ///< 8x8
	IME_SINFO_FMT_H264 = 1,  ///< 16x16
	ENUM_DUMMY4WORD(IME_SINFO_FMT)
} IME_SINFO_FMT;


/**
    IME enum - YUV converter selection
*/
typedef enum _IME_YUV_CVT_ {
	IME_YUV_CVT_BT601 = 0,  ///< BT.601
	IME_YUV_CVT_BT709 = 1,  ///< BT.709
	ENUM_DUMMY4WORD(IME_YUV_CVT)
} IME_YUV_CVT;


/**
    IME enum - YUV data type
*/
typedef enum _IME_YUV_TYPE_ {
	IME_YUV_TYPE_FULL  = 0,  ///< full range
	IME_YUV_TYPE_BT601 = 1,  ///< BT.601
	IME_YUV_TYPE_BT709 = 2,  ///< BT.709
	ENUM_DUMMY4WORD(IME_YUV_TYPE)
} IME_YUV_TYPE;


/**
    IME enum - burst length selection
*/
typedef enum _IME_BURST_SEL {
	IME_BURST_32W   = 0,  ///< burst size: 32 word
	IME_BURST_16W   = 1,  ///< burst size: 16 word
	IME_BURST_48W   = 2,  ///< burst size: 48 word
	IME_BURST_64W  = 3,  ///< burst size: 64 word
	ENUM_DUMMY4WORD(IME_BURST_SEL)
} IME_BURST_SEL;

/**
    IME enum -  low delay mode path selection
*/
typedef enum _IME_LOW_DELAY_CHL_SEL_ {
	IME_LOW_DELAY_CHL_REFOUT = 0,   ///< IME 3dnr reference output
	IME_LOW_DELAY_CHL_PATH1  = 1,   ///< IME output path1
	IME_LOW_DELAY_CHL_PATH2  = 2,   ///< IME output path2
	IME_LOW_DELAY_CHL_PATH3  = 3,   ///< IME output path3
	ENUM_DUMMY4WORD(IME_LOW_DELAY_CHL_SEL)
} IME_LOW_DELAY_CHL_SEL;




/**
    IME enum - get burst size of channel selection
*/
typedef enum _IME_GET_BSTSIZE_SEL {
	IME_GET_BST_IN_Y          = 0,  ///< get input channel: Y
	IME_GET_BST_IN_U          = 1,  ///< get input channel: U
	IME_GET_BST_IN_V          = 2,  ///< get input channel: V
	IME_GET_BST_OUTP1_Y       = 3,  ///< get output path1 channel: Y
	IME_GET_BST_OUTP1_U       = 4,  ///< get output path1 channel: U
	IME_GET_BST_OUTP1_V       = 5,  ///< get output path1 channel: V
	IME_GET_BST_OUTP2_Y       = 6,  ///< get output path2 channel: Y
	IME_GET_BST_OUTP2_U       = 7,  ///< get output path2 channel: U
	IME_GET_BST_OUTP3_Y       = 8,  ///< get output path3 channel: Y
	IME_GET_BST_OUTP3_U       = 9,  ///< get output path3 channel: U
	IME_GET_BST_OUTP4_Y       = 10,  ///< get output path4 channel: Y
	IME_GET_BST_IN_LCA        = 11,  ///< get input channel: LCA
	IME_GET_BST_OUT_LCA       = 12,  ///< get output channel: LCA subout
	IME_GET_BST_IN_STP        = 13,   ///< get statmp input channel
	IME_GET_BST_IN_PIX        = 14,   ///< get pixelation input channel
	IME_GET_BST_IN_3DNR_Y     = 15, ///< get 3DNR input reference channel: Y
	IME_GET_BST_IN_3DNR_C     = 16,  ///< get 3DNR input reference channel: UV
	IME_GET_BST_OUT_3DNR_Y    = 17, ///< get 3DNR output reference channel: Y
	IME_GET_BST_OUT_3DNR_C    = 18, ///< get 3DNR output reference channel: UV
	IME_GET_BST_IN_3DNR_M     = 19,  ///< get 3DNR input motion channel
	IME_GET_BST_OUT_3DNR_M    = 20, ///< get 3DNR output motion channel
	IME_GET_BST_OUT_3DNR_INFO = 21, ///< get 3DNR output information channel
	ENUM_DUMMY4WORD(IME_GET_BSTSIZE_SEL)
} IME_GET_BSTSIZE_SEL;



//------------------------------------------------------------------------------
/**
    IME enum - get engine information selection
*/
typedef enum _IME_GET_INFO_SEL {
	IME_GET_INPUT_SRC                   = 1,
	IME_GET_INT_ENABLE                  = 2,      ///< get interrupt enable information
	IME_GET_INT_STATUS                  = 3,       ///< get interrupt status
	IME_GET_LOCKED_STATUS               = 4,       ///< get IME engine locked status
	IME_GET_LCA_ENABLE                  = 5,       ///< get LCA enable status
	IME_GET_LCA_SUBOUT_ENABLE           = 6,       ///< get LCA subout enable status

	IME_GET_P1_ENABLE_STATUS            = 21,      ///< get output path1 enable status
	IME_GET_P1_OUTPUT_FORMAT            = 22,      ///< get output path1 data format
	IME_GET_P1_OUTPUT_TYPE              = 23,      ///< get output path1 data format type
	IME_GET_P1_SPRTOUT_ENABLE           = 24,      ///< get output path1 data format type

	IME_GET_P2_ENABLE_STATUS            = 41,      ///< get output path2 enable status
	IME_GET_P2_OUTPUT_FORMAT            = 42,      ///< get output path2 data format
	IME_GET_P2_OUTPUT_TYPE              = 43,      ///< get output path2 data format type
	IME_GET_P2_SPRTOUT_ENABLE           = 44,      ///< get output path1 data format type

	IME_GET_P3_ENABLE_STATUS            = 61,      ///< get output path3 enable status
	IME_GET_P3_OUTPUT_FORMAT            = 62,      ///< get output path3 data format
	IME_GET_P3_OUTPUT_TYPE              = 63,      ///< get output path3 data format type
	IME_GET_P3_SPRTOUT_ENABLE           = 64,      ///< get output path1 data format type

	IME_GET_P4_ENABLE_STATUS            = 81,      ///< get output path3 enable status
	IME_GET_P4_OUTPUT_FORMAT            = 82,      ///< get output path3 data format
	IME_GET_P4_OUTPUT_TYPE              = 83,      ///< get output path3 data format type


	IME_GET_DEBUG_MESSAGE               = 997,      ///< get HW debug message
	IME_GET_FUNC_STATUS0                = 998,      ///< get function enable status
	IME_GET_FUNC_STATUS1                = 999,      ///< get function enable status
	ENUM_DUMMY4WORD(IME_GET_INFO_SEL)
} IME_GET_INFO_SEL;
//------------------------------------------------------------------------------

//structure data type
/**
    IME structure - engine open object
*/
typedef struct _IME_OPENOBJ {
	void (*FP_IMEISR_CB)(UINT32 intstatus);   ///< External ISR call-back function
	UINT32 uiImeClockSel;                     ///< Engine clock selection
} IME_OPENOBJ, *pIME_OPENOBJ;

/**
    IME structure - image size parameters for horizontal and vertical direction
*/
typedef struct _IME_SIZE_INFO {
	UINT32 size_h;                  ///< horizontal size
	UINT32 size_v;                  ///< vertical size
} IME_SIZE_INFO;


/**
    IME structure - DMA address parameters for YUV channels
*/
typedef struct _IME_DMA_ADDR_INFO {
	UINT32 addr_y;    ///< DMA address of Y channel
	UINT32 addr_cb;   ///< DMA address of Cb channel
	UINT32 addr_cr;   ///< DMA address of Cr channel
} IME_DMA_ADDR_INFO;


/**
    IME structure - image lineoffset parameters for luma and chroma channels
*/
typedef struct _IME_LINEOFFSET_INFO {
	UINT32 lineoffset_y;   ///< line-offset of Y channel
	UINT32 lineoffset_cb;  ///< line-offset of Cb channel
} IME_LINEOFFSET_INFO;


/**
    IME structure - image cooridinate for horizontal and vertical directions
*/
typedef struct _IME_POS_INFO {
	UINT32 pos_x;
	UINT32 pos_y;
} IME_POS_INFO;


/**
    IME structure - range clamp
*/
typedef struct _IME_CLAMP_INFO_ {
	UINT32 min_y;
	UINT32 max_y;
	UINT32 min_uv;
	UINT32 max_uv;
} IME_CLAMP_INFO;


/**
    IME structure - input stripe parameters
*/
typedef struct _IME_STRIPE_INFO {
	UINT32  stp_n;        ///< stripe size in each stripe, used for fixed size mode
	UINT32  stp_l;        ///< stripe size of last stripe, used for fixed size mode
	UINT32  stp_m;        ///< stripe number

	UINT32 varied_size[8]; ///< stripe size, 8 entries, used for varied size mode
} IME_STRIPE_INFO;


/**
    IME structure - compute stripe size when Dram2Dram mode
*/
typedef struct _IME_STRIPE_CAL_INFO_ {
	IME_SIZE_INFO   in_size;     ///< IME input image size
} IME_STRIPE_CAL_INFO;

/**
    IME structure - horizontal and vertical stripe parameters
*/
typedef struct _IME_HV_STRIPE_INFO {
	IME_STRIPE_SIZE_MODE    stp_size_mode;        ///< stripe size mode selection, if StpSizeMode = IME_STRIPE_SIZE_MODE_VARIED, StripeCalMode = IME_STRIPE_USER_MODE only
	IME_STRIPE_INFO         stp_h;           ///< hroizontal stripe parameters
	IME_STRIPE_INFO         stp_v;           ///< Vertical stripe parameters

	IME_H_STRIPE_OVLP_SEL   overlap_h_sel;    ///< IME input stripe overlap size selection for horizontal direction
	UINT32                  overlap_h_size;                    ///< IME input stripe overlap size for horizontal direction

	IME_H_STRIPE_PRT_SEL    prt_h_sel;    ///< IME input stripe overlap parition size selection for horizontal direction
	UINT32                  prt_h_size;                    ///< IME input stripe overlap parition size for horizontal direction

	IME_STRIPE_MODE         stripe_cal_mode;  ///< IME input stripe information setting mode, recommended: IME_STRIPE_AUTO_MODE
} IME_HV_STRIPE_INFO;

/**
    IME structure - scaling factors for horizontal and vertical direction
*/
typedef struct _IME_SCALE_FACTOR_INFO {

	IME_ISD_UCOEF_CTRL_SEL isd_coef_ctrl;       ///< user coefficient control mode

	IME_SCALE_TYPE_SEL    scale_h_ud;           ///< horizontal scale up/down selection
	UINT32                scale_h_dr;         ///< horizontal scale down rate
	UINT32                scale_h_ftr;        ///< horizontal scale factor
	UINT32                isd_scale_h_base_ftr; ///< horizontal scale base
	UINT32                isd_scale_h_ftr[3];  ///< horizontal scale factor for integration method
	UINT32                isd_scale_h_coef_nums;  ///< coefficient numbers for horizontal direction
	INT32                 isd_scale_h_coefs[17];      ///< user coefficient for horizontal direction
	INT32                 isd_scale_h_coefs_all_sum;       ///< all user coefficient sum for horizontal direction
	INT32                 isd_scale_h_coefs_half_sum;       ///< half user coefficient sum for horizontal direction


	IME_SCALE_TYPE_SEL    scale_v_ud;           ///< vertical scale up/down selection
	UINT32                scale_v_dr;         ///< vertical scale down rate
	UINT32                scale_v_ftr;        ///< vertical scale factor
	UINT32                isd_scale_v_base_ftr; ///< horizontal scale base
	UINT32                isd_scale_v_ftr[3];  ///< vertical scale factor for integration method
	UINT32                isd_scale_v_coef_nums;  ///< coefficient numbers for vertical direction
	INT32                 isd_scale_v_coefs[17];      ///< user coefficient for vertical direction
	INT32                 isd_scale_v_coefs_all_sum;       ///< all user coefficient sum for vertical direction
	INT32                 isd_scale_v_coefs_half_sum;       ///< half user coefficient sum for vertical direction

	IME_SCALE_FACTOR_COEF_MODE CalScaleFactorMode;  ///< scale factor mode, recommended: IME_SCALE_FACTOR_COEF_AUTO_MODE
} IME_SCALE_FACTOR_INFO;

/**
    IME structure - scale filter parameters
*/
typedef struct _IME_SCALE_FILTER_INFO {
	IME_FUNC_EN                 scale_h_filter_enable;           ///< scaling filter enable of horizontal direction
	UINT32                      scale_h_filter_coef;     ///< scaling filter parameters of horizontal direction
	IME_FUNC_EN                 scale_v_filter_enable;           ///< scaling filter enable of vertical direction
	UINT32                      scale_v_filter_coef;     ///< scaling filter parameters of vertical direction
	IME_SCALE_FILTER_COEF_MODE  CoefCalMode;   ///< filter parameter mode, recommended: IME_SCALE_FILTER_COEF_AUTO_MODE
} IME_SCALE_FILTER_INFO;


/**
    IME structure - scale enhancement parameters
*/
//@{
typedef struct _IME_SCALE_ENH_INFO {
	UINT32  enh_factor;         ///< scaling enhancement factor
	UINT32  enh_bit;            ///< scaling enhancement normailizaion bit
} IME_SCALE_ENH_INFO;
//@}


/**
    IME structure - output image data format parameters
*/
typedef struct _IME_OUTPUT_FORMAT_INFO {
	IME_OUTPUT_IMG_FORMAT_SEL  out_format_sel;    ///< path output format
	IME_OUTPUT_FORMAT_TYPE     out_format_type_sel;  ///< paht output format type
} IME_OUTPUT_FORMAT_INFO;


//=====================================================================================
// IQ function structure


/**
    IME structure - image parameters of local chroma adaptation
*/
typedef struct _IME_CHROMA_ADAPTION_IMAGE_INFO {
	IME_SIZE_INFO       lca_img_size;     ///< input reference image size
	IME_LINEOFFSET_INFO lca_lofs;        ///< input reference image lineoffset
	IME_LCA_FORMAT_SEL  lca_fmt;         ///< input reference image format
	IME_DMA_ADDR_INFO   lca_dma_addr0;    ///< input reference imaeg buffer address
	IME_LCA_SRC         lca_src;         ///< input reference image source
} IME_CHROMA_ADAPTION_IMAGE_INFO;

/**
    IME structure - local chroma adaptation parameters
*/
typedef struct _IME_CHROMA_ADAPTION_IQC_INFO {
	UINT32              lca_ref_y_wet;      ///< Chroma reference weighting for Y channels
	UINT32              lca_ref_uv_wet;      ///< Chroma reference weighting for UV channels
	UINT32              lca_out_uv_wet;        ///< Chroma adaptation output weighting

	IME_RANGE_SEL       lca_y_range;         ///< Chroma adaptation range selection for UV channels, Max = IME_RANGE_128
	IME_RANGE_SEL       lca_y_wet_prc;        ///< Chroma adaptation weighting precision for UV channel, Max = IME_RANGE_64
	UINT32              lca_y_th;          ///< Chroma adaptation range threshold for UV channels
	UINT32              lca_y_wet_start;         ///< Chroma adaptation weighting start value for UV channels
	UINT32              lca_y_wet_end;        ///< Chroma adaptation weighting end value for UV channels


	IME_RANGE_SEL       lca_uv_range;         ///< Chroma adaptation range selection for UV channels, Max = IME_RANGE_128
	IME_RANGE_SEL       lca_uv_wet_prc;        ///< Chroma adaptation weighting precision for UV channel, Max = IME_RANGE_64
	UINT32              lca_uv_th;          ///< Chroma adaptation range threshold for UV channels
	UINT32              lca_uv_wet_start;         ///< Chroma adaptation weighting start value for UV channels
	UINT32              lca_uv_wet_end;        ///< Chroma adaptation weighting end value for UV channels
} IME_CHROMA_ADAPTION_IQC_INFO;

/**
    IME structure - chroma adjustment parameters of local chroma adaptation
*/
typedef struct _IME_CHROMA_ADAPTION_CA_INFO {
	IME_FUNC_EN     lca_ca_enable;         ///< function enable
	UINT32          lca_ca_cent_u;      ///< chroma adjustment reference center of u channel
	UINT32          lca_ca_cent_v;      ///< chroma adjustment reference center of v channel
	IME_RANGE_SEL   lca_ca_uv_range;         ///< chroma adjustment range selection for uv channels, max = ime_range_128
	IME_RANGE_SEL   lca_ca_uv_wet_prc;        ///< chroma adjustment weighting precision for uv channel, max = ime_range_64
	UINT32          lca_ca_uv_th;          ///< chroma adjustment range threshold for uv channels
	UINT32          lca_ca_uv_wet_start;         ///< chroma adjustment weighting start value for uv channels
	UINT32          lca_ca_uv_wet_end;        ///< chroma adjustment weighting end value for uv channels
} IME_CHROMA_ADAPTION_CA_INFO;

/**
    IME structure - luma suppression parameters of local chroma adaptation
*/
typedef struct _IME_CHROMA_ADAPTION_IQL_INFO {
	IME_FUNC_EN     lca_la_enable;         ///< function enable
	UINT32          lca_la_ref_wet;      ///< luma adaptation reference weighting for uv channels
	UINT32          lca_la_out_wet;        ///< luma adaptation output weighting
	IME_RANGE_SEL   lca_la_range;         ///< luma adaptation range selection for uv channels, max = ime_range_128
	IME_RANGE_SEL   lca_la_wet_prc;        ///< luma adaptation weighting precision for uv channel, max = ime_range_64
	UINT32          lca_la_th;          ///< luma adaptation range threshold for uv channels
	UINT32          lca_la_wet_start;         ///< luma adaptation weighting start value for uv channels
	UINT32          lca_la_wet_end;        ///< luma adaptation weighting end value for uv channels
} IME_CHROMA_ADAPTION_IQL_INFO;

/**
    IME structure - local chroma adaptation parameters

    @note integrated structure
*/
typedef struct _IME_CHROMA_ADAPTION_INFO {
	IME_FUNC_EN                     lca_enable;          ///< Function enable
	IME_FUNC_EN                     lca_bypass_enable;          ///< Function by-pass enable

	IME_CHROMA_ADAPTION_IMAGE_INFO  lca_image_info;       ///< Reference image information
	IME_CHROMA_ADAPTION_IQC_INFO    lca_iq_chroma_info;           ///< Function quality information for chroma channel
	IME_CHROMA_ADAPTION_IQL_INFO    lca_iq_luma_info;           ///< Function quality information for luma channel
	IME_CHROMA_ADAPTION_CA_INFO     lca_chroma_adj_info;          ///< Chroma adjustment information
} IME_CHROMA_ADAPTION_INFO;


/**
    IME structure - local chroma adaptation subout scale parameters

    @note integrated structure
*/
typedef struct _IME_CHROMA_APAPTION_SUBOUT_PARAM {
	IME_SIZE_INFO   pri_img_size;                 ///< principal image size
	IME_SIZE_INFO   ref_img_size;                 ///< reference image size
	IME_SCALE_FACTOR_INFO   subout_scale_factor;    ///< scale factor
} IME_CHROMA_APAPTION_SUBOUT_PARAM;

/**
    IME structure - local chroma adaptation subout info.

    @note integrated structure
*/
typedef struct _IME_CHROMA_ADAPTION_SUBOUT_INFO {
	IME_FUNC_EN                         lca_subout_enable;        ///< LCA subout enable
	IME_LCA_SUBOUT_SRC                  lca_subout_src;           ///< LCA subout source
	IME_CHROMA_APAPTION_SUBOUT_PARAM    lca_subout_scale_info;     ///< LCA subout scale info.
	IME_LINEOFFSET_INFO                 lca_subout_lofs;           ///< LCA subout lineoffset
	IME_DMA_ADDR_INFO                   lca_subout_addr;           ///< LCA subout buffer address
} IME_CHROMA_ADAPTION_SUBOUT_INFO;


/**
    IME structure - quality parameters of dark and bright region chroma suppression
*/
typedef struct _IME_DBCS_IQ_INFO {
	IME_DBCS_MODE_SEL OpMode;     ///< Process mode
	UINT32            uiCentU;    ///< Center value for U channel
	UINT32            uiCentV;    ///< Center value for V channel
	UINT32            uiStepY;    ///< Step for luma
	UINT32            uiStepC;   ///<  Step for chroma
	UINT32            *puiWtY;    ///< Weighting LUT for luma channel, 16 elements
	UINT32            *puiWtC;    ///< Weighting LUT for luma channel, 16 elements
} IME_DBCS_IQ_INFO;

/**
    IME structure - dark and bright region chroma suppression

    @note integrated structure
*/
typedef struct _IME_DBCS_INFO {
	IME_FUNC_EN       dbcs_enable; ///< Function enable
	IME_DBCS_IQ_INFO  dbcs_iq_info; ///< function quality parameters
} IME_DBCS_INFO;


/**
    IME structure - edge direction thresholds of SSR function
*/
typedef struct _IME_SSR_IQ_INFO {
	UINT32        diag_th;   ///< Threahold for diagonal direction
	UINT32        hori_vert_th;   ///< Threahold for horizontal and vertical direction
} IME_SSR_IQ_INFO;


/**
    IME structure - single image super-resolution parameters

    @note integrated structure
*/
typedef struct _IME_SSR_INFO {
	IME_FUNC_EN       ssr_enable;    ///< Function enable
	IME_SSR_MODE_SEL  ssr_mode;      ///< Processing mode
	IME_SSR_IQ_INFO   ssr_iq_info;    ///< Function quality parameters
} IME_SSR_INFO;


/**
    IME structure - quaility parameters of film grain noise
*/
typedef struct _IME_FILM_GRAIN_IQ_INFO {
	UINT32    fgn_lum_th;           ///< luminance value threshold
	UINT32    fgn_nlp_1;           ///< path1 noise level
	UINT32    fgn_init_p1;          ///< grain noise initial value for path1
	UINT32    fgn_nl_p2;           ///< path2 noise level
	UINT32    fgn_init_p2;          ///< grain noise initial value for path2
	UINT32    fgn_nl_p3;           ///< path3 noise level
	UINT32    fgn_init_p3;          ///< grain noise initial value for path3
} IME_FILM_GRAIN_IQ_INFO;

/**
    IME structure - film grain noise parameters

    @note integrated structure
*/
typedef struct _IME_FILM_GRAIN_INFO {
	IME_FUNC_EN               fgn_enable;    ///< film grain enable
	IME_FILM_GRAIN_IQ_INFO    fgn_iq_info;    ///< function quality parameters
} IME_FILM_GRAIN_INFO;


/**
    IME structure - image parameters for data stamp
*/
typedef struct _IME_STAMP_IMAGE_INFO {
	IME_SIZE_INFO   ds_img_size;  ///< Image size
	IME_DS_FMT_SEL  ds_fmt;     ///< Image format selection
	IME_POS_INFO    ds_pos;      ///< Blending position
	UINT32          ds_lofs;   ///< Data lineoffset
	UINT32          ds_addr;   ///< Data address
} IME_STAMP_IMAGE_INFO;

/**
    IME structure - image parameters for data stamp

    Blending weighting Usage:
    if RGB565, weight = uiBldWt0 + (uiBldWt1 << 4).
    if ARGB1555, A = 0, by using uiBldWt0 to index internal weighting table; A = 1, by using uiBldWt1 to index internal weighting table
    if ARGB1555, by using value of A to index internal weighting table
    Internal weighting table: [0,  17,  34,  51,  68,  85,  102,  119,  136,  153,  170,  187,  204,  221,  238,  255]
*/
typedef struct _IME_STAMP_IQ_INFO {
	IME_FUNC_EN        ds_ck_enable;        ///< color key enable
	IME_FUNC_EN        ds_plt_enable;       ///< color palette enable
	IME_DS_CK_MODE_SEL ds_ck_mode;      ///< color key mode selection
	UINT16             ds_ck_a;         ///< Color key for alpha channel
	UINT16             ds_ck_r;         ///< Color key for R channel
	UINT16             ds_ck_g;         ///< Color key for G channel
	UINT16             ds_ck_b;         ///< Color key for B channel
	UINT32             ds_bld_wet0;     ///< Blending weighting, if RGB565, range: [0, 16]; others, range: [0, 15]
	UINT32             ds_bld_wet1;     ///< Blending weighting, range: [0, 15]
} IME_STAMP_IQ_INFO;

/**
    IME structure - data stamp parameters of each set
*/
typedef struct _IME_STAMP_SET_INFO {
	IME_FUNC_EN             ds_enable;      ///< Function enable
	IME_STAMP_IMAGE_INFO    ds_image_info;   ///< Image information
	IME_STAMP_IQ_INFO       ds_iq_info;      ///< Data stamp quality information
} IME_STAMP_SET_INFO;

/**
    IME structure - YUV color LUT parameters of data stamp
*/
typedef struct _IME_STAMP_CST_INFO {
	IME_PARAM_MODE  ds_cst_param_mode;    ///< parameter mode, recommended: IME_PARAM_AUTO_MODE
	UINT32  ds_cst_coef0; ///< coefficient of color space transform
	UINT32  ds_cst_coef1; ///< coefficient of color space transform
	UINT32  ds_cst_coef2; ///< coefficient of color space transform
	UINT32  ds_cst_coef3; ///< coefficient of color space transform
} IME_STAMP_CST_INFO;

typedef struct _IME_STAMP_PLT_INFO_ {
	IME_DS_PLT_SEL      ds_plt_mode;        ///< color palette mode selection
	UINT32              ds_plt_tab_a[16];   ///< color palette table, channel a
	UINT32              ds_plt_tab_r[16];   ///< color palette table, channel r
	UINT32              ds_plt_tab_g[16];   ///< color palette table, channel g
	UINT32              ds_plt_tab_b[16];   ///< color palette table, channel b
} IME_STAMP_PLT_INFO;

/**
    IME structure - data stamp parameters

    @note integrated structure
*/
typedef struct _IME_STAMP_INFO {
	IME_FUNC_EN         ds_cst_enable;      ///< color space transformation of data stamp
	IME_STAMP_SET_INFO  ds_set0;            ///< data stamp set0
	IME_STAMP_SET_INFO  ds_set1;            ///< data stamp set1
	IME_STAMP_SET_INFO  ds_set2;            ///< data stamp set2
	IME_STAMP_SET_INFO  ds_set3;            ///< data stamp set3
	IME_STAMP_CST_INFO  ds_cst_coef;        ///< color space transformation coefficients
	IME_STAMP_PLT_INFO  ds_plt;
} IME_STAMP_INFO;


/**
    IME structure - row position parameters of statistical
*/
typedef struct _IME_STL_ROWPOS_INFO {
	UINT32 stl_row0;  ///< row position0
	UINT32 stl_row1;  ///< row posotion1
} IME_STL_ROWPOS_INFO;

/**
    IME structure - accmulation target parameters of statistical
*/
typedef struct _IME_STL_ACCTAG_INFO {
	UINT32 acc_tag;  ///< accmulation target
} IME_STL_ACCTAG_INFO;

/**
    IME structure - edge map parameters of statistical
*/
typedef struct _IME_STL_EDGE_INFO {
	IME_FUNC_EN      stl_edge_ker0_enable; ///< edge kernel enable for set0
	IME_STL_EKER_SEL stl_edge_ker0;   ///< edge map kernel selection for map0
	UINT32           stl_sft0;     ///< edge map shift for map0

	IME_FUNC_EN      stl_edge_ker1_enable; ///< edge kernel enable for set1
	IME_STL_EKER_SEL stl_edge_ker1;   ///< edge map kernel selection for map1
	UINT32           stl_sft1;     ///< edge map shift for map1
} IME_STL_EDGE_INFO;

/**
    IME structure - ROI threshold parameters of statistical
*/
typedef struct _IME_STL_ROITH_INFO {
	IME_STL_SRC_SEL     stl_roi_src; ///< ///< statistical source of ROI
	UINT32              stl_roi_th0;    ///< statistical threshold of ROI for section0
	UINT32              stl_roi_th1;    ///< statistical threshold of ROI for section1
	UINT32              stl_roi_th2;    ///< statistical threshold of ROI for section2
} IME_STL_ROITH_INFO;

/**
    IME structure - edge processing parameters of statistic

    @note integrated structure
*/
typedef struct _IME_STL_ROI_INFO {
	IME_STL_ROWPOS_INFO stl_row_pos;      ///< row position of statistic
	IME_STL_ROITH_INFO  stl_roi0;           ///< ROI0 for edge map
	IME_STL_ROITH_INFO  stl_roi1;           ///< ROI1 for edge map
	IME_STL_ROITH_INFO  stl_roi2;           ///< ROI2 for edge map
	IME_STL_ROITH_INFO  stl_roi3;           ///< ROI3 for edge map
	IME_STL_ROITH_INFO  stl_roi4;           ///< ROI4 for edge map
	IME_STL_ROITH_INFO  stl_roi5;           ///< ROI5 for edge map
	IME_STL_ROITH_INFO  stl_roi6;           ///< ROI6 for edge map
	IME_STL_ROITH_INFO  stl_roi7;           ///< ROI7 for edge map
} IME_STL_ROI_INFO;

/**
    IME structure - histogram of statistic

    @note integrated structure
*/
typedef struct _IME_STL_HIST_INFO {
	IME_STL_HIST_SET_SEL    stl_set_sel;          ///< operation set selection
	IME_POS_INFO            stl_hist_pos;     ///< start posotion for edge statistic
	IME_SIZE_INFO           stl_hist_size;     ///< image size for edge statistic
	IME_STL_ACCTAG_INFO     stl_hist_acc_tag;  ///< histogram accumulation target
} IME_STL_HIST_INFO;

/**
    IME structure - statistical parameters

    @note integrated structure
*/
typedef struct _IME_STL_INFO {
	IME_FUNC_EN         stl_enable;      ///< enable of statistic
	IME_FUNC_EN         stl_filter_enable;    ///< filter enable of statistic
	IME_STL_EDGE_INFO   stl_edge_map;     ///< edge map of statistic
	IME_STL_HIST_INFO   stl_hist0;        ///< edge histogram of set0
	IME_STL_HIST_INFO   stl_hist1;        ///< edge histogram of set1
	IME_STL_ROI_INFO    stl_roi;         ///< ROI threshold of edge map
	IME_STL_IMGOUT_SEL  stl_img_out_type;  ///< output before or after filter image

	IME_FUNC_EN         stl_edge_map_flip_enable;
	UINT32              stl_edge_map_lofs; ///< edge map output lineoffset
	UINT32              stl_edge_map_addr; ///< edge map output address
	UINT32              stl_hist_addr;    ///< output address for edge statistic

	IME_SIZE_INFO       stl_img_size;     ///< image size of statistical
} IME_STL_INFO;

/**
    IME structure - get statistical parameters
*/
typedef struct _IME_GET_STL_INFO {
	UINT32 get_hist_max0;      ///< max value of histogram area of set0
	UINT32 get_acc_tag_bin0;    ///< hit bin of histogram accumulation of set0

	UINT32 get_hist_max1;      ///< max value of histogram area of set1
	UINT32 get_acc_tag_bin1;    ///< hit bin of histogram accumulation of set1
} IME_GET_STL_INFO;


/**
    IME structure - stitching parameters
*/
typedef struct _IME_STITCH_INFO {
	UINT32              stitch_pos;                     ///< stitching position of image
	BOOL                lofs_update;                       ///< stitching output lineoffset update, or not
	IME_LINEOFFSET_INFO stitch_lofs;   ///< stitching image lineoffset
	BOOL                dma_addr_update;                    ///< stitching output DMA address update, or not
	IME_DMA_ADDR_INFO   stitch_dma_addr;        ///< stitching image DMA address
	IME_BUF_FLUSH_SEL   dma_flush;
} IME_STITCH_INFO;


/**
    IME structure - privacy mask coordinate
*/
typedef struct _IME_PM_POINT {
	INT32 coord_x; ///< coordinate of horizontal direction
	INT32 coord_y; ///< coordinate of vertical direction
} IME_PM_POINT;

/**
    IME structure - privacy mask using color mask
*/
typedef struct _IME_PM_MASK_COLOR {
	UINT32 pm_color_y; ///< Y channel value, range: [0, 255]
	UINT32 pm_color_u; ///< U channel value, range: [0, 255]
	UINT32 pm_color_v; ///< V channel value, range: [0, 255]
} IME_PM_MASK_COLOR;



/**
    IME structure - privacy mask parameter of each set
*/
typedef struct _IME_PM_PARAM {
	IME_FUNC_EN         pm_enable;       ///< privacy mask function enable
	IME_FUNC_EN         pm_hlw_enable;    ///< privacy mask hollow function enable
	IME_PM_POINT        pm_coord[4];     ///< privacy mask 4 coordinates, must be Convex Hull
	IME_PM_POINT        pm_coord_hlw[4];  ///< privacy mask 4 coordinates for hollow, must be Convex Hull
	IME_PM_MASK_TYPE    pm_mask_type;      ///< privacy mask type selection
	IME_PM_MASK_COLOR   pm_color;        ///< privacy mask using YUV color
	UINT32              pm_wet;        ///< privacy mask alpha weight, range:[0, 255]
} IME_PM_PARAM;


/**
    IME structure - privacy mask parameters
*/
typedef struct _IME_PM_INFO {
	IME_PM_PARAM pm_set0;                    ///< privacy mask set0
	IME_PM_PARAM pm_set1;                    ///< privacy mask set1
	IME_PM_PARAM pm_set2;                    ///< privacy mask set2
	IME_PM_PARAM pm_set3;                    ///< privacy mask set3
	IME_PM_PARAM pm_set4;                    ///< privacy mask set4
	IME_PM_PARAM pm_set5;                    ///< privacy mask set5
	IME_PM_PARAM pm_set6;                    ///< privacy mask set6
	IME_PM_PARAM pm_set7;                    ///< privacy mask set7

	IME_SIZE_INFO       pm_pxl_img_size;  ///< pixelation input image size of privacy mask
	IME_PM_PXL_BLK_SIZE pm_pxl_blk_size;   ///< block size of pixelation
	UINT32              pm_pxl_lofs;      ///< pixelation input image lineoffset
	UINT32              pm_pxl_dma_addr;   ///< pixelation input imaeg buffer address
} IME_PM_INFO;

//------------------------------------------------------------------------
// YUV Converter
/**
    IME structure - YUV converter parameters
*/
typedef struct _IME_YUV_CVT_INFO_ {
	IME_FUNC_EN yuv_cvt_enable;   ///< YUV converter function enable/disable
	IME_YUV_CVT yuv_cvt_sel;  ///< YUV converter selection, BT.601 or BT.709
} IME_YUV_CVT_INFO;

/**
    IME enum - Strength option for Y-channel pre-filter
*/
typedef enum _IME_TMNR_PRE_FILTER_TYPE_ {
	PRE_FILTER_OFF = 0,
	PRE_FILTER_TYPE_A = 1,
	PRE_FILTER_TYPE_B = 2,
	ENUM_DUMMY4WORD(IME_TMNR_PRE_FILTER_TYPE)
} IME_TMNR_PRE_FILTER_TYPE;

/**
    IME enum - An option of update mode in motion estimation
*/
typedef enum _IME_TMNR_ME_UPDATE_MODE_ {
	ME_UPDATE_RAND  = 0,
	ME_UPDATE_FIXED = 1,
	ENUM_DUMMY4WORD(IME_TMNR_ME_UPDATE_MODE)
} IME_TMNR_ME_UPDATE_MODE;

/**
    IME enum - Method option of motion vector down-sampling
*/
typedef enum _IME_TMNR_MV_DOWN_SAMPLE_MODE_ {
	MV_DOWN_SAMPLE_NEAREST  = 0,
	MV_DOWN_SAMPLE_AVERAGE  = 1,
	MV_DOWN_SAMPLE_MINIMUM  = 2,
	MV_DOWN_SAMPLE_MAXIMUM  = 3,
	ENUM_DUMMY4WORD(IME_TMNR_MV_DOWN_SAMPLE_MODE)
} IME_TMNR_MV_DOWN_SAMPLE_MODE;

typedef enum _IME_TMNR_MV_INFO_MODE_ {
	MV_INFO_MODE_AVERAGE  = 0,
	MV_INFO_MODE_LPF      = 1,
	MV_INFO_MODE_MINIMUM  = 2,
	MV_INFO_MODE_MAXIMUM  = 3,
	ENUM_DUMMY4WORD(IME_TMNR_MV_INFO_MODE)
} IME_TMNR_MV_INFO_MODE;

/**
    IME enum - Method option of patch selection
*/
typedef enum _IME_TMNR_PS_MODE_ {
	PS_MODE_0 = 0,   ///< determined by object type
	PS_MODE_1 = 1,   ///< determined by a LUT mode
	ENUM_DUMMY4WORD(IME_TMNR_PS_MODE)
} IME_TMNR_PS_MODE;

/**
    IME enum - Option for SAD statistic data
*/
typedef enum _IME_TMNR_ME_SDA_TYPE_ {
	ME_SDA_TYPE_NATIVE = 0,         ///< native SAD
	ME_SDA_TYPE_COMPENSATED = 1,   ///< compensated SAD
	ENUM_DUMMY4WORD(IME_TMNR_ME_SDA_TYPE)
} IME_TMNR_ME_SDA_TYPE;


typedef struct _IME_TMNR_DBG_CTRL_PARAM_ {
	UINT32                   dbg_mode;
	IME_FUNC_EN              dbg_froce_mv0_en;
} IME_TMNR_DBG_CTRL_PARAM;

typedef struct _IME_TMNR_STATISTIC_PARAM_ {
	IME_FUNC_EN sta_out_en;   ///< control of statistic data output
	UINT32 sample_step_hori;  ///< step size of statistical sampling in horizontal direction
	UINT32 sample_step_vert;  ///< step size of statistical sampling in vertical direction
	UINT32 sample_num_x;   ///< total number of statistical sampling in horizontal direction
	UINT32 sample_num_y;   ///< total number of statistical sampling in vertical direction
	UINT32 sample_st_x;    ///< start point of horizontal direction for statistical sampling process
	UINT32 sample_st_y;    ///< start point of vertical direction for statistical sampling process
} IME_TMNR_STATISTIC_PARAM;

/**
    IME structure - 3DNR motion estimation parameters
*/
typedef struct _IME_TMNR_ME_PARAM_ {
	IME_TMNR_ME_UPDATE_MODE update_mode;  ///< An option of update mode in motion estimation
	IME_FUNC_EN             boundary_set;  ///< Set image boundary as still type or not
	IME_TMNR_MV_DOWN_SAMPLE_MODE ds_mode;  ///< Method option of motion vector down-sampling
	IME_TMNR_ME_SDA_TYPE    sad_type;   ///< Option for SAD statistic data
	UINT32                  sad_shift;  ///< Right shift number for SAD statistic data, range = [0, 15]

	UINT32                  rand_bit_x;
	UINT32                  rand_bit_y;
	UINT32                  min_detail;
	UINT32                  periodic_th;

	UINT32                  sad_penalty[8]; ///<
	UINT32                  switch_th[8]; ///<
	UINT32                  switch_rto;   ///<
	UINT32                  detail_penalty[8]; ///<
	UINT32                  probability[8];    ///<

} IME_TMNR_ME_PARAM;


/**
    IME structure - 3DNR motion detection parameters
*/
typedef struct _IME_TMNR_MD_PARAM_ {
	UINT32 sad_coefa[8];  ///< Edge coefficient of motion detection
	UINT32 sad_coefb[8];  ///< Offset of motion detection
	UINT32 sad_std[8];    ///< Standard deviation of motion detection
	UINT32 fth[2];   ///< final threshold of motion detection
} IME_TMNR_MD_PARAM;

/**
    IME structure - 3DNR motion detection parameters for ROI
*/
typedef struct _IME_TMNR_MD_ROI_PARAM_ {
	UINT32 fth[2];   ///< final threshold of motion detection
} IME_TMNR_MD_ROI_PARAM;


/**
    IME structure - 3DNR motion compensation parameters
*/
typedef struct _IME_TMNR_MC_PARAM_ {
	UINT32 sad_base[8];  ///< base level of noise
	UINT32 sad_coefa[8];  ///< edge coefficient of motion compensation
	UINT32 sad_coefb[8];  ///< offset of motion compensation
	UINT32 sad_std[8];    ///< standard deviation of motion compensation
	UINT32 fth[2];   ///< final threshold of motion compensation
} IME_TMNR_MC_PARAM;

/**
    IME structure - 3DNR motion compensation parameters for ROI
*/
typedef struct _IME_TMNR_MC_ROI_PARAM_ {
	UINT32 fth[2];   ///< final threshold of motion compensation
} IME_TMNR_MC_ROI_PARAM;


/**
    IME structure - 3DNR patch selection parameters
*/
typedef struct _IME_TMNR_PS_PARAM_ {
	IME_FUNC_EN smart_roi_ctrl_en;    ///< ROI control
	IME_FUNC_EN mv_check_en;  ///< MV checkiong process in PS module
	IME_FUNC_EN roi_mv_check_en; ///< MV checking process for ROI
	IME_TMNR_PS_MODE ps_mode;  ///< Method option of patch selection
	IME_TMNR_MV_INFO_MODE mv_info_mode; ///< Mode option for MV length calculation
	UINT32 mv_th; ///< MV threshold
	UINT32 roi_mv_th; ///< MV threshold for ROI
	UINT32 mix_ratio[2];   ///< Mix ratio in patch selection
	UINT32 mix_th[2];  ///< Mix threshold in patch selection
	UINT32 mix_slope[2]; ///< Mix slope in patch selection
	UINT32 ds_th;   ///< Threshold of motion status down-sampling
	UINT32 ds_th_roi; ///< Threshold for motion status down-sampling for ROI
	UINT32 edge_wet;   ///< Start point of edge adjustment
	UINT32 edge_th[2];    ///< Edge adjustment threshold in patch selection
	UINT32 edge_slope;  ///< Edge adjustment slope in patch selection
	UINT32 fs_th;  ////< Threshold of patch error
} IME_TMNR_PS_PARAM;

/**
    IME structure - 3DNR noise filter parameters
*/
typedef struct _IME_TMNR_NR_PARAM_ {
	IME_FUNC_EN               luma_ch_en;    ///< noise filter enable for luma channel
	IME_FUNC_EN               chroma_ch_en;  ///< noise filter enable for chroma channel

	IME_FUNC_EN center_wzeros_y; ///< Set weighting as 0 to the center pixel in internal NR-filter
	IME_FUNC_EN chroma_fsv_en; ///< False color control
	UINT32      chroma_fsv; ///< False color value

	UINT32 luma_residue_th; ///< Protection threshold of luma channel
	UINT32 chroma_residue_th; ///< Protection threshold of luma channel

	UINT32 freq_wet[4]; ///< Filter weighting for low frequency
	UINT32 luma_wet[8]; ///< Filter intensity weighting

	IME_TMNR_PRE_FILTER_TYPE pre_filter_type;  ///< Strength option for luma channel pre-filter
	UINT32 pre_filter_str[4]; ///< Strength of pre-filtering for low frequency
	UINT32 pre_filter_rto[2]; ///< adjustment ratio of pre-filtering for transitional object

	UINT32 snr_str[3]; ///< Strength of spatial filter for still object
	UINT32 tnr_str[3]; ///< Strength of temporal filter for still object

	UINT32 snr_base_th; ///< Base threshold of spatial noise reduction
	UINT32 tnr_base_th; ///< Base threshold of spatial noise reduction

	UINT32 luma_3d_th[4]; ///< Noise reduction threshold for luma channel
	UINT32 luma_3d_lut[8]; ///< Noise reduction LUT for luma channel
	UINT32 chroma_3d_lut[8]; ///< Noise reduction LUT for chroma channel
	UINT32 chroma_3d_rto[2]; ///< Adjustment ratio 0 of noise reduction LUT for chroma channel

	IME_FUNC_EN luma_nr_type; ///< 3DNR filter option, for 528 only
} IME_TMNR_NR_PARAM;


typedef struct _IME_TMNR_INFO_ {
	IME_FUNC_EN               tmnr_en;              ///< 3DNR enable

	IME_TMNR_ME_PARAM         me_param;
	IME_TMNR_MD_PARAM         md_param;
	IME_TMNR_MD_ROI_PARAM     md_roi_param;
	IME_TMNR_MC_PARAM         mc_param;
	IME_TMNR_MC_ROI_PARAM     mc_roi_param;
	IME_TMNR_PS_PARAM         ps_param;
	IME_TMNR_NR_PARAM         nr_param;
	IME_TMNR_STATISTIC_PARAM  sta_param;


	IME_FUNC_EN         ref_in_dec_en;   ///< reference input decode enable
	IME_FUNC_EN         ref_in_flip_en;   ///< reference input flip enable
	IME_LINEOFFSET_INFO ref_in_ofs; ///< 3DNR reference image input lineoffset
	IME_DMA_ADDR_INFO   ref_in_addr; ///< 3DNR reference image input DMA buffer address


	UINT32              mot_sta_ofs; ///< 3DNR motion status input and output lineoffset
	UINT32              mot_sta_in_addr; ///< 3DNR motion status input DMA buffer address
	UINT32              mot_sta_out_addr; ///< 3DNR motion status output DMA buffer address

	IME_FUNC_EN         mot_sta_roi_out_en;   ///< motion status ROI flip enable
	IME_FUNC_EN         mot_sta_roi_out_flip_en;   ///< motion status ROI flip enable
	UINT32              mot_sta_roi_out_ofs; ///< 3DNR motion status output lineoffset for ROI
	UINT32              mot_sta_roi_out_addr; ///< 3DNR motion status output DMA buffer address for ROI

	UINT32              mot_vec_ofs; ///< 3DNR motion vector input and output lineoffset
	UINT32              mot_vec_in_addr; ///< 3DNR motion vector input DMA buffer address
	UINT32              mot_vec_out_addr; ///< 3DNR motion vector output DMA buffer address

	UINT32              sta_out_ofs; ///< 3DNR statistic data output lineoffset
	UINT32              sta_out_addr; ///< 3DNR statistic data output DMA buffer address


	IME_TMNR_DBG_CTRL_PARAM     dbg_ctrl;        ///< global control
	IME_SIZE_INFO               m_img_size;     ///< main image size
} IME_TMNR_INFO;

/**
    IME structure - 3DNR output reference image parameters
*/
typedef struct _IME_TMNR_REF_OUT_INFO_ {
	IME_FUNC_EN         ref_out_enable;       /// 3DNR output reference image enable
	IME_FUNC_EN         ref_out_enc_enable; ///< 3DNR output reference image encode enable
	IME_FUNC_EN         ref_out_enc_smode_enable; ///< 3DNR output reference image encode shift mode enable
	IME_FUNC_EN         ref_out_flip_enable; ///< 3DNR output flip encode enable

	IME_LINEOFFSET_INFO ref_out_lofs; ///< 3DNR reference image output lineoffset
	IME_DMA_ADDR_INFO   ref_out_addr; ///< 3DNR reference image output DMA buffer address

	IME_SIZE_INFO       m_img_size; ///< main image size
} IME_TMNR_REF_OUT_INFO;


typedef struct _IME_YUV_COMPRESSION_INFO_ {
	IME_PARAM_MODE  comp_param_mode;    ///< compression parameter mode, 0: auto mode, using default parameters; 1: user mode, using user define parameters
	UINT32          comp_enc[16][3];
	UINT32          comp_dec[16][3];
	UINT32          comp_q[16];
} IME_YUV_COMPRESSION_INFO;


//------------------------------------------------------------------------------
// global flow structure

/**
    IME structure - grouped function parameters

    @note integrated structure - used to ime_set_mode for initial state
*/
typedef struct _IME_IQ_FLOW_INFO {
	IME_CHROMA_ADAPTION_INFO        *p_lca_info;            ///< chroma adaption parameters, if useless, please set NULL
	IME_CHROMA_ADAPTION_SUBOUT_INFO *p_lca_subout_info;     ///< chroma adaption subout parameters, if useless, please set NULL
	IME_DBCS_INFO                   *p_dbcs_info;               ///< dark and bright region chroma supression parameters, if useless, please set NULL
	//IME_FILM_GRAIN_INFO             *p_film_grain_info;       ///< film grain parameters, if useless, please set NULL
	IME_STAMP_INFO                  *p_data_stamp_info;         ///< Data stamp parameters, if useless, please set NULL
	IME_STL_INFO                    *p_sta_info;                ///< edge statistic parameters, if useless, please set NULL
	IME_PM_INFO                     *p_pm_info;                 ///< privacy mask parameters, if useless, please set NULL
	IME_TMNR_INFO                   *p_tmnr_info;               ///< 3DNR parameters, if useless, please set NULL
	IME_TMNR_REF_OUT_INFO           *p_tmnr_refout_info;        ///< 3DNR reference image output parameters, if useless, please set NULL
	IME_YUV_CVT_INFO                *p_yuv_cvt_info;            ///< YUV converter, if useless, please set NULL
	IME_YUV_COMPRESSION_INFO        *p_comp_info;               ///< compression, if useless, please set NULL
} IME_IQ_FLOW_INFO;


/**
    IME structure - all input path parameters

    @note integrated structure
*/
typedef struct _IME_INPATH_INFO {
	IME_SIZE_INFO             in_size;               ///< input image size
	IME_INPUT_FORMAT_SEL      in_format;                ///< input image format
	IME_LINEOFFSET_INFO       in_lineoffset;            ///< input line offset
	IME_DMA_ADDR_INFO         in_addr;               ///< input DMA address

	IME_BUF_FLUSH_SEL         in_path_dma_flush;     ///< input path dma buffer flush selection when Dram2Dram mode
} IME_INPATH_INFO;
//------------------------------------------------------------------------------

/**
    IME structure - get input path information

    @note integrated structure
*/
typedef struct _IME_GET_INPATH_INFO {
	UINT32                    in_path_src;                ///< input source
	IME_SIZE_INFO             in_path_size;               ///< input image size
	IME_INPUT_FORMAT_SEL      in_path_format;                ///< input image format
	IME_LINEOFFSET_INFO       in_path_lineoffset;            ///< input line offset
	IME_DMA_ADDR_INFO         in_path_addr;               ///< input DMA address
} IME_GET_INPATH_INFO;
//------------------------------------------------------------------------------


/**
    IME structure - all output path parameters

    @note integrated structure
*/
typedef struct _IME_OUTPATH_INFO {
	IME_FUNC_EN             out_path_enable;              ///< path enable
	IME_FUNC_EN             out_path_dram_enable;          ///< path output to dram enable

	IME_FUNC_EN             out_path_encode_enable;        ///< path encode enable, only for output path1
	IME_FUNC_EN             out_path_encode_smode_enable;   ///< path output range clamp, only for output path1

	IME_FUNC_EN             out_path_sprtout_enable;       ///< path separated output to dram enable, also set "out_path_sprt_pos"
	IME_FUNC_EN             out_path_flip_enable;          ///< path flip enable


	IME_OUTPUT_FORMAT_INFO  out_path_image_format;         ///< path output image format
	IME_SIZE_INFO           out_path_scale_size;           ///< path image scaling output size
	IME_SCALE_FACTOR_INFO   out_path_scale_factors;        ///< path image scaling facotrs
	IME_SCALE_FILTER_INFO   out_path_scale_filter;         ///< path scaling filter parameters
	IME_SCALE_METHOD_SEL    out_path_scale_method;         ///< path scale method
	IME_SCALE_ENH_INFO      out_path_scale_enh;            ///< path scale enhancement, for bicubic scaling method only
	IME_DMA_ADDR_INFO       out_path_addr;                ///< path output DMA address
	IME_POS_INFO            out_path_crop_pos;             ///< path crop position
	IME_SIZE_INFO           out_path_out_size;             ///< path image crop output size
	IME_LINEOFFSET_INFO     out_path_out_lineoffset;       ///< path output lineoffset
	IME_CLAMP_INFO          out_path_clamp;               ///< path output range clamp


	IME_DMA_ADDR_INFO       out_path_addr2;           ///< path output DMA address for separating output
	IME_LINEOFFSET_INFO     out_path_out_lineoffset2;        ///< path output lineoffset for separating output
	UINT32                  out_path_sprt_pos;           ///< path output separating position

	IME_BUF_FLUSH_SEL       out_path_dma_flush;    ///< path dma buffer flush selection when Dram2Dram mode
} IME_OUTPATH_INFO;
//------------------------------------------------------------------------------

/**
    IME structure - get output path information

    @note integrated structure
*/
typedef struct _IME_GET_OUTPATH_INFO {
	IME_FUNC_EN             out_path_enable;          ///< path enable
	IME_OUTDST_CTRL_SEL     out_path_out_dest;         ///< path output destination; path1: dram/H264, others: dram
	IME_OUTPUT_FORMAT_INFO  out_path_image_format;     ///< path output image format
	IME_SIZE_INFO           out_path_scale_size;       ///< path image scaling output size


	IME_SCALE_METHOD_SEL    out_path_scale_method;     ///< path scale method
	IME_DMA_ADDR_INFO       out_path_addr;            ///< path output DMA address
	IME_POS_INFO            out_path_crop_pos;         ///< path crop position
	IME_SIZE_INFO           out_path_out_size;         ///< path image crop output size
	IME_LINEOFFSET_INFO     out_path_out_lineoffset;   ///< path output lineoffset
} IME_GET_OUTPATH_INFO;
//------------------------------------------------------------------------------

typedef enum _IME_BREAK_POINT_MODE_ {
	IME_BP_LINE_MODE  = 0,
	IME_BP_PIXEL_MODE = 1,
	ENUM_DUMMY4WORD(IME_BREAK_POINT_MODE)
} IME_BREAK_POINT_MODE;

/**
    IME structure - set break-point

    @note
    support line-based and pixel-based
    pixel-based is only for nt98528
    line-baed is occurred at last stripe
*/
typedef struct _IME_BREAK_POINT_INFO_ {
	UINT32 bp1;  ///< break point, unit: line or pixel, default = 0
	UINT32 bp2;  ///< break point, unit: line or pixel, default = 0
	UINT32 bp3;  ///< break point, unit: line or pixel, default = 0

	IME_BREAK_POINT_MODE bp_mode;  ///< only for nt98528
} IME_BREAK_POINT_INFO;

typedef struct _IME_LOW_DELAY_INFO_ {
	IME_FUNC_EN dly_enable;
	IME_LOW_DELAY_CHL_SEL dly_ch;
} IME_LOW_DELAY_INFO;

typedef struct _IME_SINGLE_OUT_INFO_ {
	IME_FUNC_EN sout_enable;
	UINT32      sout_ch;
} IME_SINGLE_OUT_INFO;


/**
    IME structure - full engine parameters

    @note integrated structure - used to ime_set_mode for initial state
*/
typedef struct _IME_MODE_PRAM {
	// input info
	IME_OPMODE                operation_mode;      ///< IME performing mode
	IME_INPATH_INFO           in_path_info;         ///< IME input path parameters

	//output info
	IME_OUTPATH_INFO          out_path1;           ///< IME output path1 parameters
	IME_OUTPATH_INFO          out_path2;           ///< IME output path2 parameters
	IME_OUTPATH_INFO          out_path3;           ///< IME output path3 parameters
	IME_OUTPATH_INFO          out_path4;           ///< IME output path4 parameters

	UINT32                    interrupt_enable;  ///< interrupt enable
	IME_HV_STRIPE_INFO        set_stripe;

	IME_SINGLE_OUT_INFO       single_out;
	IME_BREAK_POINT_INFO      break_point;
	IME_LOW_DELAY_INFO        low_delay;

	IME_IQ_FLOW_INFO          *p_ime_iq_info;        ///< IQ related parameters, if useless, please set NULL
} IME_MODE_PRAM, *pIME_MODE_PRAM;



/**
    IME structure - change DMA channel burst length parameters
*/
typedef struct _IME_BURST_LENGTH_ {
	IME_BURST_SEL bst_input_y;           ///< input Y, recommend: 32 words
	IME_BURST_SEL bst_input_u;           ///< input U, recommend: 32 words
	IME_BURST_SEL bst_input_v;           ///< input V, recommend: 32 words
	IME_BURST_SEL bst_output_path1_y;        ///< output path1 Y, recommend: 64 words
	IME_BURST_SEL bst_output_path1_u;        ///< output path1 U, recommend: 64 words
	IME_BURST_SEL bst_output_path1_v;        ///< output path1 V, recommend: 32 words
	IME_BURST_SEL bst_output_path2_y;        ///< output path2 Y, recommend: 64 words
	IME_BURST_SEL bst_output_path2_u;        ///< output path2 U, recommend: 64 words
	IME_BURST_SEL bst_output_path3_y;        ///< output path3 Y, recommend: 32 words
	IME_BURST_SEL bst_output_path3_u;        ///< output path3 U, recommend: 32 words
	IME_BURST_SEL bst_output_path4_y;        ///< output path4 Y, recommend: 32 words
	IME_BURST_SEL bst_input_lca;         ///< LCA input, recommend: 32 words
	IME_BURST_SEL bst_subout_lca;        ///< LCA subout, recommend: 32 words
	IME_BURST_SEL bst_stamp;            ///< data stamp input, recommend: 32 words
	IME_BURST_SEL bst_privacy_mask;      ///< privacy mask input pixelation, recommend: 32 words
	IME_BURST_SEL bst_tmnr_input_y;      ///< 3DNR of input luma, recommend: 64 words
	IME_BURST_SEL bst_tmnr_input_c;      ///< 3DNR of input chroma, recommend: 64 words
	IME_BURST_SEL bst_tmnr_output_y;     ///< 3DNR of output luma, recommend: 64 words
	IME_BURST_SEL bst_tmnr_output_c;     ///< 3DNR of output chroma, recommend: 64 words
	IME_BURST_SEL bst_tmnr_input_mv;     ///< 3DNR of input motion vector, recommend: 32 words
	IME_BURST_SEL bst_tmnr_output_mv;    ///< 3DNR of output motion vector, recommend: 32 words
	IME_BURST_SEL bst_tmnr_input_mo_sta;    ///< 3DNR of input motion STA, recommend: 32 words
	IME_BURST_SEL bst_tmnr_output_mo_sta;    ///< 3DNR of output motion STA, recommend: 32 words
	IME_BURST_SEL bst_tmnr_output_mo_sta_roi;    ///< 3DNR of output motion STA for ROI, recommend: 32 words
	IME_BURST_SEL bst_tmnr_output_sta;      ///< 3DNR of output statistic, recommend: 32 words
} IME_BURST_LENGTH;


//------------------------------------------------------------------------------
// flow control APIs

/**
    IME engine is opened or not

    To check IME engine is opened or closed.

    @param[in] VOID

    @return BOOL  Status of performing this API.\n
    -@b TRUE:   Engine is opened.\n
    -@b FALSE:  Engine is closed.\n
*/
extern BOOL ime_is_open(VOID);
//------------------------------------------------------------------------------

/**
    Open IME engine

    This function should be called before calling any other functions.

    @param[in] pImeOpenInfo Open and get engine resource

    @return ER  error code\n
    -@b E_OK:   setting done\n
    -@b Others: setting error\n
*/
extern ER ime_open(IME_OPENOBJ *pImeOpenInfo);
//------------------------------------------------------------------------------

/**
    Set IME engine mode and parameters

    @param[in] p_engine_info mode information and parameters.\n

    @return ER  error code\n
    -@b E_OK:   setting done\n
    -@b Others: setting error\n
*/
extern ER ime_set_mode(IME_MODE_PRAM *p_eng_info);
//------------------------------------------------------------------------------

/**
    IME engine start

    @param[in] VOID

    @return ER  error code\n
    -@b E_OK:   setting done\n
    -@b Others: setting error\n
*/
extern ER ime_start(VOID);
//------------------------------------------------------------------------------

/**
    IME engine pause

    @param[in] VOID

    @return ER  error code\n
    -@b E_OK:   setting done\n
    -@b Others: setting error\n
*/
extern ER ime_pause(VOID);
//------------------------------------------------------------------------------

/**
    IME engine close

    @param[in] VOID

    @return ER  error code\n
    -@b E_OK:   setting done\n
    -@b Others: setting error\n
*/
extern ER ime_close(VOID);
//------------------------------------------------------------------------------

/**
    IME engine hw reset

    @param[in] VOID

    @return ER  error code\n
    -@b E_OK:   setting done\n
    -@b Others: setting error\n

    @note after hw reset, user must reopen engine
*/
extern ER ime_reset(VOID);
//------------------------------------------------------------------------------


/**
    Wait system flag when frame-end occurred

    @param[in] IsClearFlag  Clear flag selection.\n

    @return ER error code\n
    -@b E_OK: desired flag is done.\n
*/
extern ER ime_waitFlagFrameEnd(IME_FLAG_CLEAR_SEL IsClearFlag);
//------------------------------------------------------------------------------


/**
    Clear frame-end flag of system

    @param[in] VOID\n

    @return VOID\n
*/
extern VOID ime_clear_flag_frame_end(VOID);
//------------------------------------------------------------------------------

/**
    Wait system flag when linked-list processing end

    @param[in] IsClearFlag  Clear flag selection.\n

    @return ER error code\n
    -@b E_OK: desired flag is done.\n
*/
extern ER ime_waitFlagLinkedListEnd(IME_FLAG_CLEAR_SEL IsClearFlag);
//------------------------------------------------------------------------------

/**
    Clear linked-list flag of system

    @param[in] VOID\n

    @return VOID\n
*/
extern VOID ime_clearFlagLinkedListEnd(VOID);
//------------------------------------------------------------------------------

/**
    Wait system flag when linked-list job processing end

    @param[in] IsClearFlag  Clear flag selection.\n

    @return ER error code\n
    -@b E_OK: desired flag is done.\n
*/
extern ER ime_waitFlagLinkedListJobEnd(IME_FLAG_CLEAR_SEL IsClearFlag);
//-------------------------------------------------------------------------------

/**
    Clear linked-list job flag of system

    @param[in] VOID\n

    @return VOID\n
*/
extern VOID ime_clearFlagLinkedListJobEnd(VOID);
//-------------------------------------------------------------------------------

/**
    Wait system flag when break-point occur

    @param[in] IsClearFlag  Clear flag selection.\n

    @return ER error code\n
    -@b E_OK: desired flag is done.\n
*/
extern ER ime_waitFlagBreakPoint_BP1(IME_FLAG_CLEAR_SEL IsClearFlag);
//-------------------------------------------------------------------------------

/**
    Clear brak-point flag of system

    @param[in] VOID\n

    @return VOID\n
*/
extern VOID ime_clearFlagBreakPoint_BP1(VOID);
//-------------------------------------------------------------------------------


/**
    Wait system flag when break-point occur

    @param[in] IsClearFlag  Clear flag selection.\n

    @return ER error code\n
    -@b E_OK: desired flag is done.\n
*/
extern ER ime_waitFlagBreakPoint_BP2(IME_FLAG_CLEAR_SEL IsClearFlag);
//-------------------------------------------------------------------------------

/**
    Clear brak-point flag of system

    @param[in] VOID\n

    @return VOID\n
*/
extern VOID ime_clearFlagBreakPoint_BP2(VOID);
//-------------------------------------------------------------------------------

/**
    Wait system flag when break-point occur

    @param[in] IsClearFlag  Clear flag selection.\n

    @return ER error code\n
    -@b E_OK: desired flag is done.\n
*/
extern ER ime_waitFlagBreakPoint_BP3(IME_FLAG_CLEAR_SEL IsClearFlag);
//-------------------------------------------------------------------------------

/**
    Clear brak-point flag of system

    @param[in] VOID\n

    @return VOID\n
*/
extern VOID ime_clearFlagBreakPoint_BP3(VOID);
//-------------------------------------------------------------------------------



/**
    Set Stripe Information

    @param[in] p_stripe_info stripe information of horizontal and vertical directions.

    @return ER  error code\n
    -@b E_OK:   setting done\n
    -@b Others: setting error\n
*/
extern ER ime_chg_stripe_param(IME_HV_STRIPE_INFO *p_stripe_info);
//------------------------------------------------------------------------------

/**
    Set Stripe Information

    @param[in] p_stripe_info stripe information of horizontal and vertical directions.

    @return ER  error code\n
    -@b E_OK:   setting done\n
    -@b Others: setting error\n
*/
extern ER ime_cal_d2d_hstripe_size(IME_STRIPE_CAL_INFO *pCalStpInfo, IME_STRIPE_INFO *pStripeH);
//------------------------------------------------------------------------------

/**
    Get input stripe size information

    @param[in] pGetStripeInfo  information of input stripe size

    @return VOID\n
*/
extern VOID ime_get_input_path_stripe_info(IME_HV_STRIPE_INFO *pGetStripeInfo);
//------------------------------------------------------------------------------


/**
    Set input path parameters

    @param[in] pSetInInfo Input path setting\n
    @param[in] LoadType load type selection\n

    @return ER  error code\n
    -@b E_OK:   setting done\n
    -@b Others: setting error\n
*/
extern ER ime_chg_input_path_param(IME_INPATH_INFO *p_set_in_info);
//------------------------------------------------------------------------------

/**
    Set output path parameters

    @param[in] Outpath_sel output path selection\n
    @param[in] pSetOutInfo output path setting\n

    @return ER  error code\n
    -@b E_OK:   setting done\n
    -@b Others: setting error\n
*/
extern ER ime_chg_output_path_param(IME_PATH_SEL out_path_sel, IME_OUTPATH_INFO *p_set_out_info);
//------------------------------------------------------------------------------

/**
    Change Output Path Enable

    @param[in] path_sel  output path selection\n
    @param[in] set_en    enable or disable setting\n

    @return ER  error code\n
    -@b E_OK:   setting done\n
    -@b Others: setting error\n
*/
extern ER ime_chg_output_path_enable(IME_PATH_SEL path_sel, IME_FUNC_EN set_en);
//------------------------------------------------------------------------------

/**
    Change Output Path DRAM-Out Enable

    @param[in] path_sel  output path selection\n
    @param[in] set_en    enable or disable setting\n

    @return ER  error code\n
    -@b E_OK:   setting done\n
    -@b Others: setting error\n
*/
extern ER ime_chg_output_path_dram_out_enable(IME_PATH_SEL path_sel, IME_FUNC_EN set_en);
//------------------------------------------------------------------------------

/**
    Change Output Scaling Filter Parameters

    @param[in] path_sel      output path selection\n
    @param[in] pSetSclFilter   scale filter setting\n

    @return ER  error code\n
    -@b E_OK:   setting done\n
    -@b Others: setting error\n
*/
extern ER ime_chg_output_scaling_filter_param(IME_PATH_SEL path_sel, IME_SCALE_FILTER_INFO *p_set_scl_filter);
//------------------------------------------------------------------------------

extern ER ime_chg_output_path_range_clamp_param(IME_PATH_SEL path_sel, IME_CLAMP_INFO *p_clamp_info);

/**
    Change Output Scaling Factor Parameters

    @param[in] path_sel      output path selection\n
    @param[in] pSetSclFactor   scale factor setting\n

    @return ER  error code\n
    -@b E_OK:   setting done\n
    -@b Others: setting error\n
*/
//extern ER ime_chg_output_scaling_factor_param(IME_PATH_SEL path_sel, IME_SCALE_FACTOR_INFO *p_set_scl_factor);
//------------------------------------------------------------------------------


/**
    Change Output Scaling Enhancement Parameters

    @param[in] path_sel      output path selection\n
    @param[in] pSetSclEnh   image enhancement setting\n

    @return ER  error code\n
    -@b E_OK:   setting done\n
    -@b Others: setting error\n
*/
extern ER ime_chg_output_scaling_enhance_param(IME_PATH_SEL path_sel, IME_SCALE_ENH_INFO *p_set_scl_enh);
//------------------------------------------------------------------------------

/**
    Change Chroma Adaptation Enable

    @param[in] set_en         function enable/disable

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_lca_enable(IME_FUNC_EN set_en);
//------------------------------------------------------------------------------

/**
    Change Chroma Adaptation Bypass Enable

    @param[in] set_en         function enable/disable

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_chroma_adapt_bypass_enable(IME_FUNC_EN set_en);
//------------------------------------------------------------------------------

/**
    Change Chroma Adaptation - Chroma Adjustment Enable

    @param[in] set_en         function enable/disable

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_chroma_adapt_chroma_adjust_enable(IME_FUNC_EN set_en);
//------------------------------------------------------------------------------

/**
    Change Chroma Adaptation - Luma Suppression Enable

    @param[in] set_en         function enable/disable

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_lca_luma_suppress_enable(IME_FUNC_EN set_en);
//------------------------------------------------------------------------------

/**
    Change Chroma Adaptation Image Parameters

    @param[in] p_set_info       image information

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_chroma_adapt_image_param(IME_CHROMA_ADAPTION_IMAGE_INFO *p_set_info);
//------------------------------------------------------------------------------

/**
    Change Chroma Adaptation - Chroma Adjustment Parameters

    @param[in] p_set_info       chroma adjustment parameters

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_chroma_adapt_chroma_adjust_param(IME_CHROMA_ADAPTION_CA_INFO *p_set_info);
//------------------------------------------------------------------------------

/**
    Change Chroma Adaptation Parameters

    @param[in] p_set_info       chroma adjustment parameters

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_chroma_adapt_param(IME_CHROMA_ADAPTION_IQC_INFO *p_set_info);
//------------------------------------------------------------------------------

/**
    Change Chroma Adaptation - Luma Suppress Parameters

    @param[in] p_set_info       chroma adjustment parameters

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_chroma_adapt_luma_suppress_param(IME_CHROMA_ADAPTION_IQL_INFO *p_set_info);
//------------------------------------------------------------------------------

/**
    Get Chroma Adaptation - Image size

    @param[in] p_get_size     get imaeg size

    @return VOID\n
*/
extern VOID ime_get_chroma_adapt_image_size_info(IME_SIZE_INFO *p_get_size);
//------------------------------------------------------------------------------

/**
    Get Chroma Adaptation - Lineoffset

    @param[in] p_get_lofs     get lineoffset size

    @return VOID\n
*/
extern VOID ime_get_chroma_adapt_lineoffset_info(IME_LINEOFFSET_INFO *p_get_lofs);
//------------------------------------------------------------------------------

/**
    Get Chroma Adaptation - DMA address

    @param[in] p_get_lofs     get DMA address

    @return VOID\n
*/
extern VOID ime_get_chroma_adapt_dma_addr_info(IME_DMA_ADDR_INFO *p_get_addr0, IME_DMA_ADDR_INFO *p_get_addr1);
//------------------------------------------------------------------------------

/**
    Change Chroma Adaptation Subout Enable

    @param[in] set_en         function enable/disable

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_lca_subout_enable(IME_FUNC_EN set_en);
//------------------------------------------------------------------------------

/**
    Change Chroma Adaptation Subout Source

    @param[in] SetSrc       source selection

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_chroma_adapt_subout_source(IME_LCA_SUBOUT_SRC set_src);
//------------------------------------------------------------------------------


/**
    Change Chroma Adaptation Subout Scale Parameters

    @param[in] p_set_info      scale info.

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_chroma_adapt_subout_param(IME_CHROMA_APAPTION_SUBOUT_PARAM *p_set_info);
//------------------------------------------------------------------------------

/**
    Change Chroma Adaptation Subout Lineoffset

    @param[in] pSetLofs      image lineoffset

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_chroma_adapt_subout_lineoffset_info(IME_LINEOFFSET_INFO *p_set_lofs);
//------------------------------------------------------------------------------


/**
    Get Chroma Adaptation Subout Lineoffset

    @param[in] p_get_lofs      image lineoffset

    @return VOID
*/
extern VOID ime_get_chroma_adapt_subout_lineoffset_info(IME_LINEOFFSET_INFO *p_get_lofs);
//------------------------------------------------------------------------------



/**
    Change Chroma Adaptation Subout DMA Address

    @param[in] pSetLofs      DMA address

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_chroma_adapt_subout_dma_addr_info(IME_DMA_ADDR_INFO *p_set_addr);
//------------------------------------------------------------------------------

/**
    Get Chroma Adaptation Subout DMA Address

    @param[in] p_get_lofs      DMA address

    @return VOID
*/
extern VOID ime_get_chroma_adapt_subout_dma_addr_info(IME_DMA_ADDR_INFO *p_get_addr);
//------------------------------------------------------------------------------

/**
    Change Dark and Bright Region Chroma Suppression Enable

    @param[in] set_en         function enable/disable

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_dark_bright_chroma_suppress_enable(IME_FUNC_EN set_en);
//------------------------------------------------------------------------------


/**
    Change Dark and Bright Region Chroma Suppression Parameters

    @param[in] p_set_info      dark and rright region chroma suppression parameters

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_dark_bright_chroma_suppress_param(IME_DBCS_IQ_INFO *p_set_info);
//------------------------------------------------------------------------------


/**
    Change Film Grain Noise Enable

    @param[in] set_en         function enable/disable

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
//extern ER ime_chg_film_grain_noise_enable(IME_FUNC_EN set_en);
//------------------------------------------------------------------------------


/**
    Change Film Grain Noise Parameters

    Change Film Grain Noise Parameters

    @param pFgnParam Film grain noise parameters, please refer to data sturcture "IME_FILM_GRAIN_INFO"

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
//extern ER ime_chg_film_grain_noise_param(IME_FILM_GRAIN_IQ_INFO *p_set_info);
//------------------------------------------------------------------------------

/**
    Change Data Stamp Color Space Transformation Enable

    @param[in] set_en    function enable

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_data_stamp_cst_enable(IME_FUNC_EN set_en);
//------------------------------------------------------------------------------


/**
    Change Data Stamp Color key Enable

    Available when RGB565 data format

    @param[in] set_num    which Set selection
    @param[in] set_en     function enable

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_data_stamp_color_key_enable(IME_DS_SETNUM set_num, IME_FUNC_EN set_en);
//------------------------------------------------------------------------------



/**
    Change Data Stamp Enable

    @param[in] set_num        which Set selection
    @param[in] set_en         function enable/disable

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_data_stamp_enable(IME_DS_SETNUM set_num, IME_FUNC_EN set_en);
//------------------------------------------------------------------------------

/**
    Change Data Stamp Image Parameters

    @param[in] set_num        which Set selection
    @param[in] p_set_info     image information

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_data_stamp_image_param(IME_DS_SETNUM set_num, IME_STAMP_IMAGE_INFO *p_set_info);
//------------------------------------------------------------------------------


/**
    Change Data Stamp Effect Parameters

    @param[in] set_num        which Set selection
    @param[in] p_set_info     effect information

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_data_stamp_param(IME_DS_SETNUM set_num, IME_STAMP_IQ_INFO *p_set_info);
//------------------------------------------------------------------------------

/**
    Change Data Stamp Color LUT Parameters

    @param[in] p_set_info     YUV color information for data stamp

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_data_stamp_color_coefs_param(IME_STAMP_CST_INFO *p_set_info);
//------------------------------------------------------------------------------

/**
    Change Data Stamp Color Palette Parameters

    @param[in] pSetPltInfo     color palette information for data stamp

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_data_stamp_color_palette_info(IME_STAMP_PLT_INFO *p_set_info);
//------------------------------------------------------------------------------


/**
    Get Data Stamp Image Size

    @param[in] set_num       which Set selection\n
    @param[in] p_get_info     get image size\n

    @return VOID\n
*/
extern VOID ime_get_data_stamp_image_size_info(IME_DS_SETNUM set_num, IME_SIZE_INFO *p_get_info);
//------------------------------------------------------------------------------

/**
    Get Data Stamp Lineoffset

    @param[in] set_num       which Set selection\n
    @param[in] p_get_info     get image lineoffset\n

    @return VOID\n
*/
extern VOID ime_get_data_stamp_lineoffset_info(IME_DS_SETNUM set_num, IME_LINEOFFSET_INFO *p_get_info);
//------------------------------------------------------------------------------

/**
    Get Data Stamp DMA Address

    @param[in] set_num       which Set selection\n
    @param[in] p_get_addrInfo     get DMA address\n

    @return VOID\n
*/
extern VOID ime_get_data_stamp_dma_addr_info(IME_DS_SETNUM set_num, IME_DMA_ADDR_INFO *p_get_addrInfo);
//------------------------------------------------------------------------------

/**
    Change Privacy Mask enable

    @param[in] set_num   which Set selection\n
    @param[in] set_en    function enable\n

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_privacy_mask_enable(IME_PM_SET_SEL set_num, IME_FUNC_EN set_en);
//------------------------------------------------------------------------------

/**
    Change Privacy Mask parameters

    @param[in] set_num       which Set selection\n
    @param[in] p_set_info    parameter setting\n

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_privacy_mask_param(IME_PM_SET_SEL set_num, IME_PM_PARAM *p_set_info);
//------------------------------------------------------------------------------

/**
    Change Privacy Mask Pixelation Image Size

    @param[in] pSetSize    image size\n

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_privacy_mask_pixelation_image_size(IME_SIZE_INFO *p_set_size);
//------------------------------------------------------------------------------


/**
    Change Privacy Mask Pixelation Block Size

    @param[in] pSetSize    block size\n

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_privacy_mask_pixelation_block_size(IME_PM_PXL_BLK_SIZE *p_set_size);
//------------------------------------------------------------------------------

/**
    Change Privacy Mask Pixelation Image Lineoffset

    @param[in] pSetLofs    image lineoffset\n

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
ER ime_chg_privacy_maask_pixelation_image_lineoffset(UINT32 *p_set_lofs);
//------------------------------------------------------------------------------

/**
    Change Privacy Mask Pixelation Image DMA Address

    @param[in] p_set_addr    image DMA address\n

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
ER ime_chg_privacy_maask_pixelation_image_dma_addr(UINT32 *p_set_addr);
//------------------------------------------------------------------------------

/**
    Get Privacy Mask Pixelation Image Size

    @param[in] p_get_size    image size\n

    @return VOID
*/
extern VOID ime_get_privacy_mask_pixelation_image_size_info(IME_SIZE_INFO *p_get_size);
//------------------------------------------------------------------------------

/**
    Get Privacy Mask Pixelation Image Lineoffset

    @param[in] p_get_lofs    image lineoffset\n

    @return VOID
*/
extern VOID ime_get_privacy_mask_pixelation_image_lineoffset_info(UINT32 *p_get_lofs);
//------------------------------------------------------------------------------

/**
    Get Privacy Mask Pixelation Image DMA Address

    @param[in] p_get_addr    image DMA address\n

    @return VOID
*/
extern VOID ime_get_privacy_mask_pixelation_image_dma_addr_info(UINT32 *p_get_addr);
//------------------------------------------------------------------------------


/**
    Get Engine Inforomation

    @param[in] GetImeInfoSel desired information selection

    @return UINT32      desired information\n
*/
extern UINT32 ime_get_engine_info(IME_GET_INFO_SEL get_ime_info_sel);
//------------------------------------------------------------------------------

/**
    Get input path image size

    @param[in] p_get_size get input path image size

    @return VOID\n
*/
extern VOID ime_get_input_path_image_size_info(IME_SIZE_INFO *p_get_size);
//------------------------------------------------------------------------------

/**
    Get input path lineoffset

    @param[in] p_get_lofs get input path lineoffset

    @return VOID\n
*/
extern VOID ime_get_input_path_lineoffset_info(IME_LINEOFFSET_INFO *p_get_lofs);
//------------------------------------------------------------------------------

/**
    Get input path DMA address

    @param[in] p_get_addr get input path DMA address

    @return VOID\n
*/
extern VOID ime_get_input_path_dma_addr_info(IME_DMA_ADDR_INFO *p_get_addr);
//---------------------------------------------------------------------------------------------

/**
    Get output path image size

    @param[in] path_sel  output path selection\n
    @param[in] p_get_size get input path image size\n

    @return VOID\n
*/
extern VOID ime_get_output_path_image_size_info(IME_PATH_SEL path_sel, IME_SIZE_INFO *p_get_size);
//------------------------------------------------------------------------------

/**
    Get output path lineoffset

    @param[in] path_sel  output path selection\n
    @param[in] buf_sel   output path buffer set selection\n
    @param[in] p_get_size get input path image size\n

    @return VOID\n
*/
extern VOID ime_get_output_path_lineoffset_info(IME_PATH_SEL path_sel, IME_PATH_OUTBUF_SEL buf_sel, IME_LINEOFFSET_INFO *p_get_lofs);
//------------------------------------------------------------------------------

/**
    Get output path DMA Address

    @param[in] path_sel  output path selection\n
    @param[in] buf_sel   output path buffer set selection\n
    @param[in] p_get_addr get input path image size\n

    @return VOID\n
*/
extern VOID ime_get_output_path_dma_addr_info(IME_PATH_SEL path_sel, IME_PATH_OUTBUF_SEL buf_sel, IME_DMA_ADDR_INFO *p_get_addr);
//------------------------------------------------------------------------------

/**
    Change Statistic Enable

    @param[in] set_en        function enable/disable

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_statistic_enable(IME_FUNC_EN set_en);
//------------------------------------------------------------------------------

/**
    Change Statistic Filter Enable

    @param[in] set_en        function enable/disable

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_statistic_filter_enable(IME_FUNC_EN set_en);
//------------------------------------------------------------------------------

/**
    Change Output Image Type

    @param[in] ImgOutSel    image output type, output before or after filter image

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_statistical_image_output_type(IME_STL_IMGOUT_SEL img_out_sel);
//------------------------------------------------------------------------------


/**
    Change Statistic Edge Kernel Parameters

    @param[in] p_set_info     effect information

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_statistic_edge_kernel_param(IME_STL_EDGE_INFO *p_set_info);
//------------------------------------------------------------------------------

/**
    Change Statistic Histogram Parameters

    @param[in] p_set_info     effect information

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_statistic_histogram_param(IME_STL_HIST_INFO *p_set_info);
//------------------------------------------------------------------------------

/**
    Change Statistic Edge Map Lineoffset

    @param[in] uiLofs     Lineoffset

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_statistic_edge_map_lineoffset(UINT32 stp_lofs);
//------------------------------------------------------------------------------

/**
    Change Statistic Edge Map Address

    @param[in] uiAddr     buffer address

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_statistic_edge_map_addr(UINT32 addr);
//------------------------------------------------------------------------------



/**
    Change Statistic Histogram Output Buffer Address

    @param[in] uiHistAddr     buffer address

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_statistic_histogram_addr(UINT32 hist_addr);
//------------------------------------------------------------------------------


/**
    Change Statistic Edge Map Parameters

    @param[in] p_set_info     effect information

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_statistic_edge_roi_param(IME_STL_ROI_INFO *p_set_info);
//------------------------------------------------------------------------------

/**
    Change Statistic Edge Map Flip Enable

    @param[in] set_en     enable control\n

    @return VOID\n
*/
extern ER ime_chg_statistic_edge_flip_enable(IME_FUNC_EN set_en);
//------------------------------------------------------------------------------



/**
    Get Statistic Information

    This API can get the max value with in hisgoram region and accmulation target histogram bin

    @param[in] pGetHistInfo     desired information\n

    @return VOID\n
*/
extern VOID ime_get_statistic_info(IME_GET_STL_INFO *p_get_hist_info);
//------------------------------------------------------------------------------


/**
    Get Edge Map Output Address

    @param[in] p_get_addr     edge map output address

    @return VOID\n
*/
extern VOID ime_get_statistic_edge_map_dma_addr_info(IME_DMA_ADDR_INFO *p_get_addr);
//------------------------------------------------------------------------------

/**
    Get Edge Map Output lineoffset

    @param[in] p_get_addr     edge map output lineoffset

    @return VOID\n
*/
extern VOID ime_get_statistic_edge_map_lineoffset_info(UINT32 *p_get_lofs);
//------------------------------------------------------------------------------


/**
    Get Histogram Output Address

    @param[in] p_get_addr     histogram output address

    @return VOID\n
*/
extern VOID ime_get_statistic_histogram_dma_addr_info(IME_DMA_ADDR_INFO *p_get_addr);
//------------------------------------------------------------------------------

/**
    Change YUV Converter Enable

    @param[in] set_en    disable / enable control\n

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_yuv_converter_enable(IME_FUNC_EN set_en);
//------------------------------------------------------------------------------

/**
    Change YUV Converter Selection

    @param[in] SetCvt    converter selecton control\n

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_yuv_converter_param(IME_YUV_CVT set_cvt);
//------------------------------------------------------------------------------

/**
    Get IME output path YUV data type

    @param[in] VOID

    @return IME_YUV_TYPE      YUV data type, full range/BT.601/BT.709\n
*/
extern IME_YUV_TYPE ime_get_output_yuv_data_type(VOID);
//------------------------------------------------------------------------------


/**
    Change Stitching Enable

    @param[in] path_sel  select stitching function on output path\n
    @param[in] set_en    disable / enable control\n

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_stitching_enable(IME_PATH_SEL path_sel, IME_FUNC_EN set_en);
//------------------------------------------------------------------------------


/**
    Change Stitching Image Parameters

    @param[in] path_sel  path selection\n
    @param[in] pStitchInfo  Stitching parameters\n

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_stitching_image_param(IME_PATH_SEL path_sel, IME_STITCH_INFO *p_stitch_info);
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// TMNR
extern ER ime_chg_tmnr_enable(IME_FUNC_EN set_en);
extern ER ime_chg_tmnr_luma_enable(IME_FUNC_EN set_en);
extern ER ime_chg_tmnr_chroma_enable(IME_FUNC_EN set_en);
extern ER ime_chg_tmnr_refin_decoder_enable(IME_FUNC_EN set_en);
extern ER ime_chg_tmnr_refin_flip_enable(IME_FUNC_EN set_en);
extern ER ime_chg_tmnr_motion_sta_roi_flip_enable(IME_FUNC_EN set_en);
extern ER ime_chg_tmnr_motion_sta_roi_enable(IME_FUNC_EN set_en);
extern ER ime_chg_tmnr_statistic_out_enable(UINT32 set_en);


extern ER ime_chg_tmnr_motion_estimation_param(IME_TMNR_ME_PARAM *p_set_param);

extern ER ime_chg_tmnr_motion_detection_param(IME_TMNR_MD_PARAM *p_set_param);
extern ER ime_chg_tmnr_motion_detection_roi_param(IME_TMNR_MD_ROI_PARAM *p_set_param);

extern ER ime_chg_tmnr_motion_compensation_param(IME_TMNR_MC_PARAM *p_set_param);
extern ER ime_chg_tmnr_motion_compensation_roi_param(IME_TMNR_MC_ROI_PARAM *p_set_param);

extern ER ime_chg_tmnr_patch_selection_param(IME_TMNR_PS_PARAM *p_set_param);

extern ER ime_chg_tmnr_noise_filter_param(IME_TMNR_NR_PARAM *p_set_param);

extern ER ime_chg_tmnr_statistic_data_output_param(IME_TMNR_STATISTIC_PARAM *p_set_param);

extern ER ime_chg_tmnr_debug_ctrl_param(IME_TMNR_DBG_CTRL_PARAM *p_set_param);

extern ER ime_chg_tmnr_ref_in_lineoffset(IME_LINEOFFSET_INFO *p_set_lofs);
extern ER ime_chg_tmnr_ref_in_y_addr(UINT32 set_addr);
extern ER ime_chg_tmnr_ref_in_uv_addr(UINT32 set_addr);
extern UINT32 ime_get_tmnr_ref_in_y_addr(VOID);
extern UINT32 ime_get_tmnr_ref_in_uv_addr(VOID);

extern ER ime_chg_tmnr_motion_status_lineoffset(UINT32 set_lofs);
extern ER ime_chg_tmnr_motion_status_in_addr(UINT32 set_addr);
extern ER ime_chg_tmnr_motion_status_out_addr(UINT32 set_addr);
extern UINT32 ime_get_tmnr_motion_status_in_addr(VOID);
extern UINT32 ime_get_tmnr_motion_status_out_addr(VOID);


extern ER ime_chg_tmnr_motion_status_roi_out_lineoffset(UINT32 set_lofs);
extern ER ime_chg_tmnr_motion_status_roi_out_addr(UINT32 set_addr);
extern UINT32 ime_get_tmnr_motion_status_roi_out_addr(VOID);

extern ER ime_chg_tmnr_motion_vector_lineoffset(UINT32 set_lofs);
extern ER ime_chg_tmnr_motion_vector_in_addr(UINT32 set_addr);
extern ER ime_chg_tmnr_motion_vector_out_addr(UINT32 set_addr);
extern UINT32 ime_get_tmnr_motion_vector_in_addr(VOID);
extern UINT32 ime_get_tmnr_motion_vector_out_addr(VOID);


extern ER ime_chg_tmnr_statistic_out_lineoffset(UINT32 set_lofs);
extern ER ime_chg_tmnr_statistic_out_addr(UINT32 set_addr);
extern UINT32 ime_get_tmnr_statistic_out_addr(VOID);



//------------------------------------------------------------------------------
// TMNR reference output
extern ER ime_chg_tmnr_ref_out_enable(IME_FUNC_EN set_en);
extern ER ime_chg_tmnr_ref_out_encoder_enable(IME_FUNC_EN set_en);
extern ER ime_chg_tmnr_ref_out_flip_enable(IME_FUNC_EN set_en);
extern ER ime_chg_tmnr_ref_out_lineoffset(IME_LINEOFFSET_INFO *p_set_lofs);
extern ER ime_chg_tmnr_ref_out_y_addr(UINT32 set_addr);
extern ER ime_chg_tmnr_ref_out_uv_addr(UINT32 set_addr);

extern UINT32 ime_get_tmnr_ref_out_y_addr(VOID);
extern UINT32 ime_get_tmnr_ref_out_uv_addr(VOID);


//------------------------------------------------------------------------------
extern ER ime_chg_compression_param(IME_YUV_COMPRESSION_INFO *p_set_param);

/**
    Set input or output channel burst lenght

    @param[in] p_set_info   burst size parameters

    @return ER      error code\n
    -@b E_OK:       setting done\n
    -@b Others:     setting error\n
*/
extern ER ime_chg_burst_length(IME_BURST_LENGTH *p_set_info);
//------------------------------------------------------------------------------

/**
    Get available max stripe size for IPP mode

    @param[in] VOID

    @return UINT32  max stripe size\n
*/
extern UINT32 ime_get_input_max_stripe_size(VOID);
//------------------------------------------------------------------------------

/**
    Change all engine parameters except color key and LUT of data stamp function

    @param[in] p_engine_info mode information and parameters.\n

    @return ER  error code\n
    -@b E_OK:   setting done\n
    -@b Others: setting error\n
*/
//extern ER ime_chg_all_param(IME_MODE_PRAM *p_engine_info);
//------------------------------------------------------------------------------

/**
    Get input and output burst length information

    @param[in] VOID

    @return VOID

*/
extern INT32 ime_get_burst_length(IME_GET_BSTSIZE_SEL get_bst_size_sel);

/**
    get output path parameter information

    @param[in] path_sel  path selection\n
    @param[in] p_get_info get parameters\n
*/
extern VOID ime_get_output_path_info(IME_PATH_SEL path_sel, IME_GET_OUTPATH_INFO *p_get_info);
//------------------------------------------------------------------------------



/**
    get input path parameter information

    @param[in] p_get_info get parameters\n
*/
extern VOID ime_get_input_path_info(IME_GET_INPATH_INFO *p_get_info);
//------------------------------------------------------------------------------

/**
    get function enable status

    @param[in] func_sel func selection
*/
extern IME_FUNC_EN ime_get_func_enable_info(IME_FUNC_SEL func_sel);
//------------------------------------------------------------------------------


extern VOID ime_set_linked_list_addr(UINT32 set_addr);
extern VOID ime_set_linked_list_fire(VOID);
extern VOID ime_set_linked_list_terminate(VOID);


extern ER ime_chg_single_output(IME_SINGLE_OUT_INFO *p_set_sout);
extern ER ime_get_single_output(IME_SINGLE_OUT_INFO *p_get_sout);

extern ER ime_chg_break_point(IME_BREAK_POINT_INFO *p_set_bp);
extern ER ime_chg_low_delay(IME_LOW_DELAY_INFO *p_set_ldy);

extern ER ime_chg_direct_in_out_param(IME_MODE_PRAM *p_engine_info);
extern ER ime_chg_direct_lca_param(IME_MODE_PRAM *p_engine_info);
extern ER ime_chg_direct_dbcs_param(IME_MODE_PRAM *p_engine_info);
extern ER ime_chg_direct_osd_param(IME_MODE_PRAM *p_engine_info);
extern ER ime_chg_direct_osd_palette_param(IME_MODE_PRAM *p_engine_info);
extern ER ime_chg_direct_adas_stl_param(IME_MODE_PRAM *p_engine_info);
extern ER ime_chg_direct_pm_param(IME_MODE_PRAM *p_engine_info);
extern ER ime_chg_direct_tmnr_param(IME_MODE_PRAM *p_engine_info);
extern ER ime_chg_direct_break_point_param(IME_MODE_PRAM *p_engine_info);
extern ER ime_chg_direct_single_output_param(IME_MODE_PRAM *p_engine_info);
extern ER ime_chg_direct_low_delay_param(IME_MODE_PRAM *p_engine_info);
extern ER ime_chg_direct_interrupt_enable_param(IME_MODE_PRAM *p_engine_info);
extern ER ime_chg_direct_yuv_cvt_param(IME_MODE_PRAM *p_engine_info);


extern ER ime_chg_builtin_isr_cb(IME_INT_CB fp);
extern BOOL ime_get_fsatboot_flow_ctrl_status(VOID);
extern void ime_set_builtin_flow_disable(void);

/*
#ifdef __KERNEL__

#else

#if (defined(_NVT_EMULATION_) == ON)
extern VOID ime_set_emu_load(IME_LOAD_TYPE SetLoadType);
extern ER ime_set_emu_start(VOID);
extern BOOL ime_end_time_out_status;
#endif

#endif
*/

#if (defined(_NVT_EMULATION_) == ON)
extern BOOL ime_end_time_out_status;
#endif

//@}
#ifdef __cplusplus
} //extern "C" {
#endif


#endif



