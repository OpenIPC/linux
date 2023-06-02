/**
    Public header file for kdrv_dce

    This file is the header file that define the API and data type for kdrv_dce.

    @file       kdrv_dce.h
    @ingroup    mILibIPLCom
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2019.    All rights reserved.
*/
#include "kdrv_videoprocess/kdrv_ipp_utility.h"

#ifndef _KDRV_DCE_H_
#define _KDRV_DCE_H_

#define DCE_PARTITION_NUM       16
#define GDC_TABLE_NUM           65
#define CAC_TABLE_NUM           65
#define KDRV_CFA_FREQ_NUM       16
#define KDRV_CFA_LUMA_NUM       17
#define KDRV_CFA_FCS_NUM        16
#define LUT2D_MAX_TABLE_SIZE    (65*65)
#define WDR_SUBIMG_FILT_NUM     3
#define WDR_COEF_NUM            4
#define WDR_INPUT_BLD_NUM       17
#define WDR_NEQ_TABLE_IDX_NUM   32
#define WDR_NEQ_TABLE_VAL_NUM   65
#define HISTOGRAM_STCS_NUM      128

typedef enum {
    KDRV_DCE_CFA_EN                   = 0x00000010,  ///< CFA interpolation function DCE_FUNC_DC                    0x00000020  ///< Distortion correction function
    KDRV_DCE_CAC_EN                   = 0x00000040,  ///< Chromatic aberration correction function
    KDRV_DCE_YUV2RGB_EN               = 0x00000080,  ///< YUV to RGB function in D2D mode
    KDRV_DCE_TCURVE_EN                = 0x00000100,  ///< Tone curve function
    KDRV_DCE_WDR_EN                   = 0x00000200,  ///< WDR function
    KDRV_DCE_WDR_SUBOUT_EN            = 0x00000400,  ///< WDR subout function
    KDRV_DCE_CFA_SUBOUT_EN            = 0x00000800,  ///< CFA subout function
    KDRV_DCE_CFA_SUBOUT_FLIP_EN       = 0x00001000,  ///< CFA subout flip function
    KDRV_DCE_CFA_PINKR_EN             = 0x00002000,  ///< Pink reduction function
    KDRV_DCE_HISTOGRAM_EN             = 0x00004000,  ///< Histogram statistics function
    KDRV_DCE_CROP_EN                  = 0x00040000,  ///< Output crop function
    KDRV_DCE_D2D_RAND_EN              = 0x00200000,  ///< D2D random LSB function
    KDRV_DCE_D2D_UVFILT_EN            = 0x08000000,  ///< D2D UV filter function
    KDRV_DCE_D2D_IOSTOP_EN            = 0x10000000,  ///< D2D UV filter function
    KDRV_DCE_ALL_EN                   = 0x18247ff0,  ///< All function mask
} KDRV_DCE_FUNC_EN;

typedef enum {
	KDRV_DCE_PROC_MODE_LINKLIST = 0,	/* linlist mode */
	KDRV_DCE_PROC_MODE_CPU,				/* cpu write register, only for debug */
	KDRV_DCE_PROC_MODE_MAX,
} KDRV_DCE_PROC_MODE;

typedef enum {
	KDRV_DCE_PARAM_CFG_PROCESS_CPU = 0,
	KDRV_DCE_PARAM_CFG_PROCESS_LL,
	KDRV_DCE_PARAM_TRIG_SINGLE,
	KDRV_DCE_PARAM_TRIG_LL,
    KDRV_DCE_PARAM_TRIG_DIRECT,
	KDRV_DCE_PARAM_SET_CB,
    KDRV_DCE_PARAM_STOP_SINGLE,
	KDRV_DCE_PARAM_CAL_STRIPE,
	KDRV_DCE_PARAM_HARD_RESET,
	KDRV_DCE_PARAM_HIST_RESULT,
    KDRV_DCE_PARAM_DMA_CH_ABORT,
    KDRV_DCE_PARAM_DMA_CH_STS,
    KDRV_DCE_PARAM_GET_REG_BASE_ADDR,
    KDRV_DCE_PARAM_GET_REG_BASE_ADDR_LL,
	KDRV_DCE_PARAM_ID_MAX,
} KDRV_DCE_PARAM_ID;

typedef enum {
    KDRV_DCE_QUERY_REG_NUM,
	KDRV_DCE_QUERY_ID_MAX,
} KDRV_DCE_QUERY_ID;

/**
    DCE KDRV interrupt enable enum.
*/
typedef enum _KDRV_DCE_INTE {
	KDRV_DCE_INTE_CLR       = 0x00000000,
	KDRV_DCE_INTE_FST       = 0x00000001,   // enable interrupt: frame start
	KDRV_DCE_INTE_FMD       = 0x00000002,   // enable interrupt: frame end
	KDRV_DCE_INTE_STPERR    = 0x00000008,   // enable interrupt: DCE stripe error
	KDRV_DCE_INTE_LBOVF     = 0x00000010,   // enable interrupt: DCE line buffer overflow error
	KDRV_DCE_INTE_STPOB     = 0x00000040,   // enable interrupt: DCE stripe boundary overflow error
	KDRV_DCE_INTE_YBACK     = 0x00000080,   // enable interrupt: DCE Y coordinate backward skip error
	KDRV_DCE_INTE_LL_END    = 0x00000100,   // enable interrupt: Linked List done
	KDRV_DCE_INTE_LL_ERR    = 0x00000200,   // enable interrupt: Linked List error command
	KDRV_DCE_INTE_LL_ERR2   = 0x00000400,   // enable interrupt: Linked List error in direct mdoe
	KDRV_DCE_INTE_LL_JOBEND = 0x00000800,   // enable interrupt: Linked List job end
	KDRV_DCE_INTE_FRAMEERR  = 0x00001000,   // enable interrupt: frame start error in direct mode
	KDRV_DCE_INTE_ALL       = 0x00001FDF,   // enable interrupt: all
} KDRV_DCE_INTE;

/**
    type for KDRV_DCE_SET_ISRCB
*/
typedef enum {
	KDRV_DCE_INT_FST        = 0x00000001,
	KDRV_DCE_INT_FMD        = 0x00000002,
	KDRV_DCE_INT_STPERR     = 0x00000008,
	KDRV_DCE_INT_LBOVF      = 0x00000010,
	KDRV_DCE_INT_STPOB      = 0x00000040,
	KDRV_DCE_INT_YBACK      = 0x00000080,
	KDRV_DCE_INT_LL_END     = 0x00000100,
	KDRV_DCE_INT_LL_ERR     = 0x00000200,
	KDRV_DCE_INT_LL_ERR2    = 0x00000400,
	KDRV_DCE_INT_LL_JOBEND  = 0x00000800,
	KDRV_DCE_INT_FRAMEERR   = 0x00001000,
	KDRV_DCE_INT_ALL        = 0x00001FDF,
	ENUM_DUMMY4WORD(KDRV_DCE_ISR_STATUS)
} KDRV_DCE_ISR_STATUS;

typedef enum {
	KDRV_DCE_UPDATE_CFA	 		        = 0x00000001,
	KDRV_DCE_UPDATE_WDR 	            = 0x00000004,
    KDRV_DCE_UPDATE_WDR_SUB             = 0x00000008,
	KDRV_DCE_UPDATE_WDR_TONE_CRV	 	= 0x00000010,
	KDRV_DCE_UPDATE_WDR_BLD_LUT		 	= 0x00000020,
	KDRV_DCE_UPDATE_GDC                 = 0x00000040,
	KDRV_DCE_UPDATE_CAC_LUT   		 	= 0x00000080,
	KDRV_DCE_UPDATE_2D	    		 	= 0x00000100,
    KDRV_DCE_UPDATE_COLOR_GAIN          = 0x00000200,
    KDRV_DCE_UPDATE_PINK_REDUC          = 0x00000400,
    KDRV_DCE_UPDATE_IR_PARAM            = 0x00000800,
	KDRV_DCE_UPDATE_ALL				= (KDRV_DCE_UPDATE_CFA | KDRV_DCE_UPDATE_WDR | KDRV_DCE_UPDATE_WDR_SUB
	                                    |KDRV_DCE_UPDATE_WDR_TONE_CRV | KDRV_DCE_UPDATE_GDC | KDRV_DCE_UPDATE_CAC_LUT | KDRV_DCE_UPDATE_2D
	                                    | KDRV_DCE_UPDATE_COLOR_GAIN | KDRV_DCE_UPDATE_PINK_REDUC | KDRV_DCE_UPDATE_IR_PARAM | KDRV_DCE_UPDATE_WDR_BLD_LUT),
} KDRV_DCE_UPDATE;


typedef enum _KDRV_DCE_OP_MODE {
	KDRV_DCE_OP_MODE_NORMAL = 0,    //input from IPP engine (RHE->IFE->DCE->IME)
	KDRV_DCE_OP_MODE_CAL_STRIP,     //Do not use! It is used by flow automatically
	ENUM_DUMMY4WORD(KDRV_DCE_OP_MODE)
} KDRV_DCE_OP_MODE;

typedef enum {
	KDRV_DCE_DRAM_OUT_NORMAL            = 0,    ///<
	KDRV_DCE_DRAM_OUT_SINGLE            = 1,    ///<
	ENUM_DUMMY4WORD(KDRV_DCE_DRAM_OUTPUT_MODE)
} KDRV_DCE_DRAM_OUTPUT_MODE;

/**
    DCE KDRV - input yuv data range format selection
*/
typedef enum _KDRV_DCE_D2D_YUVRGB_FMT {
	KDRV_DCE_YUV2RGB_FMT_FULL = 0,      //YUV full range format
	KDRV_DCE_YUV2RGB_FMT_BT601 = 1,     //YUV BT601 range format
	KDRV_DCE_YUV2RGB_FMT_BT709 = 2,     //YUV BT709 range format
	ENUM_DUMMY4WORD(KDRV_DCE_D2D_YUVRGB_FMT)
} KDRV_DCE_D2D_YUVRGB_FMT;

typedef enum _KDRV_DCE_FOV_BND_SEL {
	KDRV_DCE_FOV_BND_DUPLICATE = 0,     ///< Replace out of boundary pixels with duplicate boundary pixels
	KDRV_DCE_FOV_BND_REG_RGB = 1,       ///< Replace out of boundary pixels with GEO_BOUNDR, GEO_BOUNDG, GEO_BOUNDB
	ENUM_DUMMY4WORD(KDRV_DCE_FOV_BND_SEL)
} KDRV_DCE_FOV_BND_SEL;

typedef enum _KDRV_DCE_2DLUT_NUM {
	KDRV_DCE_2DLUT_65_65_GRID = 0,      ///< 65x65 grid 2D lut distortion correction
	KDRV_DCE_2DLUT_33_33_GRID = 1,      ///< 33x33 grid 2D lut distortion correction
	KDRV_DCE_2DLUT_17_17_GRID = 2,      ///< 17x17 grid 2D lut distortion correction
	KDRV_DCE_2DLUT_9_9_GRID = 3,        ///< 9x9 grid 2D lut distortion correction
	ENUM_DUMMY4WORD(KDRV_DCE_2DLUT_NUM)
} KDRV_DCE_2DLUT_NUM;

typedef enum {
	KDRV_XY_BOTH = 0,           ///< processing both X and Y
	KDRV_X_ONLY  = 1,           ///< processing X only
	ENUM_DUMMY4WORD(KDRV_DCE_GDC_LUT_MODE)
} KDRV_DCE_GDC_LUT_MODE;

typedef enum {
	KDRV_INPUT_BLD_3x3_Y  = 0,   ///< use Y from bayer 3x3 as the input blending source selection
	KDRV_INPUT_BLD_GMEAN  = 1,   ///< use Gmean from bayer 3x3 as the input blending source selection
	KDRV_INPUT_BLD_BAYER  = 2,   ///< use Bayer as the input blending source selection
	ENUM_DUMMY4WORD(KDRV_DCE_WDR_INPUT_BLDSEL)
} KDRV_DCE_WDR_INPUT_BLDSEL;

typedef enum {
	DCE_WDR_ROUNDING        = 0,  ///< rounding
	DCE_WDR_HALFTONING      = 1,  ///< halftone rounding
	DCE_WDR_RANDOMLSB       = 2,  ///< random rounding
	ENUM_DUMMY4WORD(KDRV_DCE_WDR_DITHER_MODE)
} KDRV_DCE_WDR_DITHER_MODE;

typedef enum {
	KDRV_BEFORE_WDR      = 0,    ///< histogram statistics before wdr algorithm
	KDRV_AFTER_WDR       = 1,    ///< histogram statistics after wdr algorithm
	ENUM_DUMMY4WORD(KDRV_DCE_HIST_SEL)
} KDRV_DCE_HIST_SEL;

typedef enum {
	KDRV_DCE_CFA_SUBOUT_CH0 = 0,         ///< Select channel 0 as output channel
	KDRV_DCE_CFA_SUBOUT_CH1 = 1,         ///< Select channel 1 as output channel
	KDRV_DCE_CFA_SUBOUT_CH2 = 2,         ///< Select channel 2 as output channel
	KDRV_DCE_CFA_SUBOUT_CH3 = 3,         ///< Select channel 3 as output channel
	KDRV_DCE_CFA_SUBOUT_CH_UNKNOWN,
	ENUM_DUMMY4WORD(KDRV_DCE_CFA_SUBOUT_CH_SEL)
} KDRV_DCE_CFA_SUBOUT_CH_SEL;

typedef enum {
	KDRV_DCE_CFA_SUBOUT_1BYTE = 0,       ///< Output 1 byte per pixel for CFA subout
	KDRV_DCE_CFA_SUBOUT_2BYTE = 1,       ///< Output 2 byte per pixel for CFA subout
	ENUM_DUMMY4WORD(KDRV_DCE_CFA_SUBOUT_BYTE)
} KDRV_DCE_CFA_SUBOUT_BYTE;

typedef enum {
	KDRV_DCE_IPE_OLAP_8 = 0,
	KDRV_DCE_IPE_OLAP_16 = 1,
} KDRV_DCE_IPE_OLAP_SEL;

typedef enum {
	KDRV_DCE_IME_OLAP_16 = 0,
	KDRV_DCE_IME_OLAP_24 = 1,
	KDRV_DCE_IME_OLAP_32 = 2,
	KDRV_DCE_IME_OLAP_USER = 3,
} KDRV_DCE_IME_OLAP_SEL;

/**
    channel information
*/

//structure data type
typedef struct _KDRV_DCE_TRIGGER_PARAM {
	UINT32 RESERVED;
} KDRV_DCE_TRIGGER_PARAM;

/**
    DCE structure - dce kdrv open object
*/
typedef struct _KDRV_DCE_OPENCFG {
	UINT32 dce_clock_sel;       //Engine clock selection
} KDRV_DCE_OPENCFG, *pKDRV_DCE_OPENCFG;

//----------------------------------------------------------------------

/**
    DCE KDRV trigger type
*/
typedef struct _KDRV_DCE_TRIG_TYPE_INFO {
	KDRV_DCE_OP_MODE opmode;    //DCE operating mode
} KDRV_DCE_TRIG_TYPE_INFO;

//----------------------------------------------------------------------

/**
    DCE structure - Dram single output channel enable
*/
typedef struct {
	BOOL single_out_wdr_en;
	BOOL single_out_cfa_en;
} KDRV_DCE_DRAM_SINGLE_OUT_EN;

//----------------------------------------------------------------------

typedef struct _KDRV_DCE_GDC_CENTER_INFO {
	UINT16 geo_center_x;                    //Geometric distortion X center
	UINT16 geo_center_y;                    //Geometric distortion Y center
} KDRV_DCE_GDC_CENTER_INFO;

//----------------------------------------------------------------------

/**
    DCE KDRV CFA direction decision
*/
typedef struct _KDRV_DCE_CFA_DIR {
	UINT8  diff_norm_bit;          //Difference norm bit, range [0~3]
	UINT16  ns_mar_edge;            //Noise margin for edge, range [0~1023]
	UINT16  ns_mar_diff;            //Noise margin for pixel difference, range [0~1023]
} KDRV_DCE_CFA_DIR;

//----------------------------------------------------------------------

/**
    DCE KDRV Distortion Correction Parameters of FOV boundary replacement
*/
typedef struct _KDRV_DCE_FOV_PARAM {
	UINT16 fov_gain;                    ///< 1024 is 1x, Scale down factor on corrected image (12 bit: 2.10)
	KDRV_DCE_FOV_BND_SEL fov_bnd_sel;   ///< Select out-of-bound pixel source
	UINT16 fov_bnd_r;                   ///< Replace value, range [0~1023]
	UINT16 fov_bnd_g;                   ///< Replace value, range [0~1023]
	UINT16 fov_bnd_b;                   ///< Replace value, range [0~1023]
} KDRV_DCE_FOV_PARAM;

//----------------------------------------------------------------------

/**
    DCE KDRV Distortion Correction Param and Enable/Disable
*/
typedef struct _KDRV_DCE_DC {
	UINT16 geo_dist_x;                  ///< X input distance factor
	UINT16 geo_dist_y;                  ///< Y input distance factor
	UINT16 geo_lut_g[GDC_TABLE_NUM];    ///< 65 x 16bit
} KDRV_DCE_DC;

typedef struct {
	UINT16    nrm_fact_sel;///< select different factor bit number
	UINT16    nrm_fact;    ///< Normalization factor
	UINT16    nrm_fact_10b;///< Normalization factor 10 bit
	UINT16    nrm_bit;     ///< Normalization bit
} KDRV_DCE_DIST_NORM;

/**
    DCE KDRV Chromatic Aberration Correction Param and Enable/Disable
*/
typedef struct _KDRV_DCE_CAC {
	BOOL cac_enable;
	UINT16 geo_r_lut_gain;              ///< 4096 is 1x, range [0~8191]
	UINT16 geo_g_lut_gain;              ///< 4096 is 1x, range [0~8191]
	UINT16 geo_b_lut_gain;              ///< 4096 is 1x, range [0~8191]
	INT16 geo_lut_r[CAC_TABLE_NUM];     ///< 65 x 16bit
	INT16 geo_lut_b[CAC_TABLE_NUM];     ///< 65 x 16bit
	BOOL cac_sel;                       ///< CAC use 0:G_Lut & RGB Lut Gain, 1: RGB Lut
} KDRV_DCE_CAC;

/**
    DCE KDRV DC and CAC Param
*/
typedef struct _KDRV_DCE_DC_CAC_PARAM {
	BOOL dc_enable;
	KDRV_DCE_GDC_LUT_MODE dc_y_dist_off;
	KDRV_DCE_DC dc;
	KDRV_DCE_CAC cac;
} KDRV_DCE_DC_CAC_PARAM;

//----------------------------------------------------------------------

/**
    DCE KDRV 2D Lut Distortion Correction Param and Enable/Disable
*/
typedef struct _KDRV_DCE_2DLUT_PARAM {
	BOOL lut_2d_enable;
	KDRV_DCE_GDC_LUT_MODE lut_2d_y_dist_off;
	KDRV_DCE_2DLUT_NUM lut_num_select;
	UINT32 lut_2d_value[LUT2D_MAX_TABLE_SIZE];
} KDRV_DCE_2DLUT_PARAM;


//----------------------------------------------------------------------

/**
    DCE structure - WDR non-equal table parameters
*/
typedef struct {
	UINT8 lut_idx[WDR_NEQ_TABLE_IDX_NUM];      ///< non equatable table index [0~63]
	UINT8 lut_split[WDR_NEQ_TABLE_IDX_NUM];    ///< non equatable table split number [0~3]
	UINT16 lut_val[WDR_NEQ_TABLE_VAL_NUM];      ///< non equatable table value [0~4095]
} KDRV_WDR_NEQ_TABLE;

/**
    DCE structure - WDR input blending parameters
*/
typedef struct {
	KDRV_DCE_WDR_INPUT_BLDSEL bld_sel;
	UINT8 bld_wt;                              ///< blending of Y and localmax [0~8],0: Y, 8: local max
	UINT8 blend_lut[WDR_INPUT_BLD_NUM];        ///< blending of Y and raw [0~255], 0: Y, 255: raw
} KDRV_WDR_IN_BLD_PARAM;

/**
    DCE structure - WDR strength parameters
*/
typedef struct {
	INT16 wdr_coeff[WDR_COEF_NUM];              ///< wdr coefficient [-4096~4095]
	UINT8 strength;                            ///< wdr strength [0~255]
	UINT8 contrast;                            ///< wdr contrast [0~255], 128 = original
} KDRV_WDR_STRENGTH_PARAM;

/**
    DCE structure - WDR gain control parameters
*/
typedef struct {
	BOOL gainctrl_en;                           ///< enable gain control for wdr
	UINT8 max_gain;                            ///< maximum gain of wdr [1~255]
	UINT8 min_gain;                            ///< minimum gain of wdr [1~255], mapping to 1/256, 2/256, 255/256
} KDRV_WDR_GAINCTRL_PARAM;

/**
    DCE structure - WDR output blending parameters
*/
typedef struct {
	BOOL outbld_en;                             ///< enable output blending for wdr
	KDRV_WDR_NEQ_TABLE outbld_lut;              ///< wdr output blending table
} KDRV_WDR_OUTBLD_PARAM;

/**
    DCE structure - WDR saturation reduction parameters
*/
typedef struct {
	UINT16 sat_th;                              ///< threshold of wdr saturation reduction [0~4095]
	UINT8 sat_wt_low;                          ///< lower weight of wdr saturation reduction [0~255]
	UINT8 sat_delta;                           ///< delta of wdr saturation reduction [0~255]
} KDRV_WDR_SAT_REDUCT_PARAM;


/**
    DCE structure - WDR dither parameters.
*/
typedef struct {
	BOOL wdr_rand_rst;
	KDRV_DCE_WDR_DITHER_MODE wdr_rand_sel;
} KDRV_DCE_WDR_DITHER;

/**
    DCE KDRV WDR/TONE CURVE Param and Enable/Disable
*/
typedef struct _KDRV_DCE_WDR_PARAM {
	BOOL wdr_enable;
	BOOL tonecurve_enable;
	UINT8  ftrcoef[WDR_SUBIMG_FILT_NUM];       ///< wdr sub-image low pass filter coefficients, 3 entries
	KDRV_WDR_IN_BLD_PARAM     input_bld;        ///< wdr input blending
	KDRV_WDR_NEQ_TABLE        tonecurve;        ///< wdr tonecurve configuration
	KDRV_WDR_STRENGTH_PARAM   wdr_str;          ///< wdr strength
	KDRV_WDR_GAINCTRL_PARAM   gainctrl;         ///< wdr gain control
	KDRV_WDR_OUTBLD_PARAM     outbld;           ///< wdr output blending settings
	KDRV_WDR_SAT_REDUCT_PARAM sat_reduct;       ///< wdr saturation reduction
	KDRV_DCE_WDR_DITHER dither;                 ///< wdr sub-img dither
} KDRV_DCE_WDR_PARAM;

/**
    DCE KDRV WDR subimg size Information
*/
typedef struct {
	UINT8           subimg_size_h;            ///< wdr sub-image size [4~32]
	UINT8           subimg_size_v;            ///< wdr sub-image size [4~32]
	UINT16           subimg_lofs_in;           ///< wdr subin lineoffset, lofs >= (subimg_size_h * 8)
	UINT16           subimg_lofs_out;          ///< wdr subout lineoffset, lofs >= (subimg_size_h * 8)
} KDRV_DCE_WDR_SUBIMG_PARAM;


/**
    DCE KDRV Histogram Param and Enable/Disable
*/
typedef struct _KDRV_DCE_HIST_PARAM {
	BOOL hist_enable;                           ///< Enable histogram statistics
	KDRV_DCE_HIST_SEL hist_sel;                 ///< selection of statistics input
	UINT8 step_h;                              ///< h step size of histogram [0~31]
	UINT8 step_v;                              ///< v step size of histogram [0~31]
} KDRV_DCE_HIST_PARAM;


/**
    DCE KDRV Histogram Output Rslt
*/
typedef struct _KDRV_DCE_HIST_RSLT {
	UINT16 *p_hist_stcs;       ///< histogram statistics, 128 entries
} KDRV_DCE_HIST_RSLT ;

/**
    DCE KDRV CFA subout Param and Enable/Disable
*/
typedef struct _KDRV_DCE_CFA_SUBOUT_PARAM {
	BOOL cfa_subout_enable;
	BOOL cfa_subout_flip_enable;
	UINT32 cfa_addr;                                 ///< cfa subout addr
	UINT16 cfa_lofs;                                 ///< cfa subout lineoffsett
	KDRV_DCE_CFA_SUBOUT_CH_SEL subout_ch_sel;        ///< CFA sub-img output channel selection
	KDRV_DCE_CFA_SUBOUT_BYTE subout_byte;            ///< CFA sub-img output format
	UINT16 subout_shiftbit;                          ///< CFA sub-img output right shift bit
} KDRV_DCE_CFA_SUBOUT_INFO;



/**
    KDRV DCE all IQ function enable/disable operation for IPL
*/
typedef struct _KDRV_DCE_PARAM_IPL_FUNC_EN_ {
	UINT32 ipl_ctrl_func;   ///<It labels which function will be controlled by KFLOW
	BOOL   enable;          ///
} KDRV_DCE_PARAM_IPL_FUNC_EN;


typedef struct _KDRV_DCE_STRIPE_PARAM {
	UINT32 stripe_type;   //assign stripe type for driver calculation or user-assigned value
	UINT32 dce_strp_rule;   //dce stripe rule setting for GDC and low latency
} KDRV_DCE_STRIPE_PARAM;

#if 0
#endif

#define KDRV_DCE_IMG_MAX_STP_NUM	16
#define KDRV_DCE_STP_SET_MAX_NUM	16

typedef enum {
	KDRV_DCE_INTERRUPT_CLR       = 0x00000000,
	KDRV_DCE_INTERRUPT_FST       = 0x00000001,   // enable interrupt: frame start
	KDRV_DCE_INTERRUPT_FMD       = 0x00000002,   // enable interrupt: frame end
	KDRV_DCE_INTERRUPT_STPERR    = 0x00000008,   // enable interrupt: DCE stripe error
	KDRV_DCE_INTERRUPT_LBOVF     = 0x00000010,   // enable interrupt: DCE line buffer overflow error
	KDRV_DCE_INTERRUPT_STPOB     = 0x00000040,   // enable interrupt: DCE stripe boundary overflow error
	KDRV_DCE_INTERRUPT_YBACK     = 0x00000080,   // enable interrupt: DCE Y coordinate backward skip error
	KDRV_DCE_INTERRUPT_LL_END    = 0x00000100,   // enable interrupt: Linked List done
	KDRV_DCE_INTERRUPT_LL_ERR    = 0x00000200,   // enable interrupt: Linked List error command
	KDRV_DCE_INTERRUPT_LL_ERR2   = 0x00000400,   // enable interrupt: Linked List error in direct mdoe
	KDRV_DCE_INTERRUPT_LL_JOBEND = 0x00000800,   // enable interrupt: Linked List job end
	KDRV_DCE_INTERRUPT_FRAMEERR  = 0x00001000,   // enable interrupt: frame start error in direct mode
	KDRV_DCE_INTERRUPT_ALL       = 0x00001FDF,   // enable interrupt: all
} KDRV_DCE_INTERRUPT;

typedef enum {
	KDRV_DCE_SRAM_DCE = 0,       ///< Use DCE SRAM only
	KDRV_DCE_SRAM_CNN = 1,       ///< Use DCE and CNN SRAM
} KDRV_DCE_SRAM_SEL;

typedef enum {
	KDRV_DCE_1ST_STP_POS_CALC    = 0,    ///< H,V Input starts from DCE calculated position
	KDRV_DCE_1ST_STP_POS_ZERO    = 1,    ///< H,V Input starts from 0
} KDRV_DCE_STP_INST;

typedef enum {
	KDRV_DCE_STRIPE_AUTO = 0,           //Auto calculation
	KDRV_DCE_STRIPE_MANUAL_MULTI,       //Force mult-stripe
	KDRV_DCE_STRIPE_MANUAL_2STRIPE,     //Force 2 stripe
	KDRV_DCE_STRIPE_MANUAL_3STRIPE,     //Force 3 stripe
	KDRV_DCE_STRIPE_MANUAL_4STRIPE,     //Force 4 stripe
	KDRV_DCE_STRIPE_CUSTOMER,           //config stripe setting by user
} KDRV_DCE_STRP_TYPE;

typedef enum {
	KDRV_DCE_STRP_LOW_LAT_HIGH_PRIOR = 0,   ///< stripe configuration for low latency best performance
	KDRV_DCE_STRP_GDC_HIGH_PRIOR = 1,       ///< stripe configuration for GDC better performance
	KDRV_DCE_STRP_2DLUT_HIGH_PRIOR = 2,     ///< stripe configuration for 2DLUT best performance
	KDRV_DCE_STRP_GDC_BEST = 3,     		///< stripe configuration for GDC best performance
	KDRV_DCE_STRP_RULE_UNKNOWN,
} KDRV_DCE_STRP_RULE_SEL;

typedef enum {
	KDRV_DCE_PINKR_MOD_G  = 0,   ///< modify G channel
	KDRV_DCE_PINKR_MOD_RB = 1,   ///< modify RB channel
} KDRV_DCE_PINKR_MODE;

typedef enum {
	KDRV_DCE_CGAIN_2_8 = 0,     ///< gain range integer/fraction 2.8
	KDRV_DCE_CGAIN_3_7 = 1,     ///< gain range integer/fraction 3.7
} KDRV_DCE_CGAIN_RANGE;

typedef struct {
	UINT8 hstp_maxinc;
	UINT8 hstp_maxdec;
	UINT8 lbuf_back_rsv_line;
	KDRV_DCE_STP_INST hstp_inst;
	UINT8 hstp_ipeolap;
	UINT8 hstp_imeolap;
	UINT16 hstp_ime_usr_val;
	UINT8 hstp_num;
	UINT16 hstp[KDRV_DCE_STP_SET_MAX_NUM];
} KDRV_DCE_STRP_INFO;

typedef struct {
	KDRV_IPP_OPMODE op_mode;
	KDRV_DCE_STRP_TYPE stripe_type;
	KDRV_DCE_STRP_RULE_SEL strp_rule;
	KDRV_DCE_SRAM_SEL sram_sel;
	URECT crop_window;
	UINT32 width;
	UINT32 height;
	UINT16 ipe_strp_alignment;
	UINT16 ipe_strp_overlap;
	UINT16 ime_strp_alignment;
	UINT16 ime_strp_overlap;
} KDRV_DCE_CAL_STRP_INFO;

typedef struct {
	KDRV_DCE_STRP_INFO info;
	UINT16 ui_dce_out_hstp[KDRV_DCE_IMG_MAX_STP_NUM];	///< H stripe settings of DCE output image
	UINT16 ui_ime_in_hstp[KDRV_DCE_IMG_MAX_STP_NUM];	///< H stripe settings of IME input
	UINT16 ui_ime_out_hstp[KDRV_DCE_IMG_MAX_STP_NUM];	///< H stripe settings of IME output
} KDRV_DCE_STRP_RSLT;

typedef struct {
	KDRV_DCE_CAL_STRP_INFO cal_info;
	KDRV_DCE_STRP_RSLT rst;
} KDRV_DCE_GET_STIRPE;

typedef struct {
	KDRV_IPP_FRAME frm;
	URECT crop_window;
} KDRV_DCE_OUT_INFO;

typedef struct {
	UINT16    edge_dth;                         ///< Edge threshold 1 for Bayer CFA interpolation [0~4095]
	UINT16    edge_dth2;                        ///< Edge threshold 2 for Bayer CFA interpolation [0~4095]
	UINT16    freq_th;                          ///< CFA frequency threshold [0~4095]
	UINT8    freq_lut[KDRV_CFA_FREQ_NUM];      ///< CFA frequency blending weight look-up table [0~15]
	UINT8    luma_wt[KDRV_CFA_LUMA_NUM];       ///< Luma gain for CFA [0~255]
} KDRV_DCE_CFA_INTERP;

typedef struct {
	BOOL    rb_corr_enable;         //Enable for RB channel correction
	UINT16  rb_corr_th1;            //RB correction noise threshold1, range [0~1023]
	UINT16  rb_corr_th2;            //RB correction noise threshold2, range [0~1023]
} KDRV_DCE_CFA_CORR;

typedef struct {
	BOOL      fcs_dirsel;                       ///< Direction selection for detecting high frequency, 0: detect 2 directions, 1: detect 4 directions
	UINT8    fcs_coring;                       ///< Coring threshold for false color suppression [0~255]
	UINT8    fcs_weight;                       ///< Global strength for false color suppression [0~255]
	UINT8    fcs_strength[KDRV_CFA_FCS_NUM];   ///< CFA false color suppression strength [0~15]
} KDRV_DCE_CFA_FCS;

typedef struct {
	BOOL    cl_check_enable;        ///< Enable cross line check
	BOOL    hf_check_enable;        ///< Enable high frequency check
	BOOL    average_mode;           ///< Interpolation method selection when flat region detect
	BOOL    cl_sel;                 ///< operation type when cross line is not found
	UINT8  cl_th;                  ///< Cross line checking threshold, range [0~255]
	UINT8  hf_gth;                 ///< G channel smoothness checking threshold, range [0~7]
	UINT8  hf_diff;                ///< Difference threshold value between Gr and Gb to determing high frequency pattern detected or not, range [0~255]
	UINT8  hf_eth;                 ///< Threshold to determine H or V direction high frequency pattern detected, range [0~255]
	UINT16  ir_g_edge_th;           ///< Noise margin for edge in Green component interpolation [0~1023]
	UINT8  ir_rb_cstrength;        ///< RB channel smooth strength [0~7]
} KDRV_DCE_CFA_HFC;

typedef struct {
	UINT16    ir_sub_r;                         ///< R channel IR sub [0~4095]
	UINT16    ir_sub_g;                         ///< G channel IR sub [0~4095]
	UINT16    ir_sub_b;                         ///< B channel IR sub [0~4095]
	UINT8    ir_sub_wt_lb;                     ///< Lower bound of IR local weighting [0~255]
	UINT8    ir_sub_th;                        ///< Threshold to decay IR local weighting [0~255]
	UINT8    ir_sub_range;                     ///< Range to decay IR local weighting [0~3]
	UINT16    ir_sat_gain;                      ///< Saturation gain multiplied after IR sub [0~1023]
} KDRV_DCE_CFA_IR;


typedef struct {
	BOOL pink_rd_en;                            ///< pink reduction enable
	KDRV_DCE_PINKR_MODE pink_rd_mode;           ///< pink reduction mode
	UINT8    pink_rd_th1;                      ///< pink reduction threshold 1 [0~255]
	UINT8    pink_rd_th2;                      ///< pink reduction threshold 2 [0~255]
	UINT8    pink_rd_th3;                      ///< pink reduction threshold 3 [0~255]
	UINT8    pink_rd_th4;                      ///< pink reduction threshold 4 [0~255]
} KDRV_DCE_CFA_PINKR;

typedef struct {
	UINT16    r_gain;                           ///< R channel gain [0~1023]
	UINT16    g_gain;                           ///< G channel gain [0~1023]
	UINT16    b_gain;                           ///< B channel gain [0~1023]
	KDRV_DCE_CGAIN_RANGE gain_range;            ///< select fraction type of gain: 2.8 or 3.7
} KDRV_DCE_CFA_CGAIN;

typedef struct {
	BOOL cfa_enable;
	KDRV_DCE_CFA_INTERP cfa_interp;             ///< CFA interpolation term
	KDRV_DCE_CFA_CORR cfa_correction;           ///< CFA correction term
	KDRV_DCE_CFA_FCS cfa_fcs;                   ///< CFA false color suppression term
	KDRV_DCE_CFA_HFC cfa_ir_hfc;                ///< CFA high frequency check term
	KDRV_DCE_CFA_IR cfa_ir_sub;                 ///< CFA IR subtraction term
	KDRV_DCE_CFA_PINKR cfa_pink_reduc;          ///< CFA pink reduction term
	KDRV_DCE_CFA_CGAIN cfa_cgain;               ///< CFA color gain term
} KDRV_DCE_CFA_PARAM;

typedef struct {
	BOOL random_lsb_enable; // used in WDR for 8bit -> 12bit
	BOOL random_reset;
	BOOL uv_lpf_enable; //lpf for 444 to 422 or 420
	BOOL io_stop;
} KDRV_DCE_DRAM_IN_PARAM;

typedef struct {
	KDRV_IPP_OPMODE mode;
	KDRV_IPP_FRAME in_frm;
	KDRV_DCE_OUT_INFO out_img_info;
	KDRV_DCE_STRP_RSLT strp;
	KDRV_DCE_SRAM_SEL sram_sel;
    KDRV_DCE_DRAM_IN_PARAM dram_in_info;
    KDRV_DCE_CFA_SUBOUT_INFO cfa_sub;
    KDRV_DCE_GDC_CENTER_INFO gdc_center;
	UINT32 func_en;
	UINT32 inte_en;
	/* wdr para */
	UINT32 sub_in_addr;
	UINT8 sub_out_en;
	UINT32 sub_out_addr;
} KDRV_DCE_IO_CFG;

typedef struct {
	KDRV_DCE_CFA_PARAM 			cfa_param;
	KDRV_DCE_DC_CAC_PARAM       dc_cac_param;
	KDRV_DCE_2DLUT_PARAM        two_dlut_param;
	KDRV_DCE_FOV_PARAM          fov_param;
	KDRV_DCE_STRIPE_PARAM       stripe_param;
	KDRV_DCE_WDR_PARAM          wdr_param;
	KDRV_DCE_WDR_SUBIMG_PARAM   wdr_subimg_param;
	KDRV_DCE_HIST_PARAM         hist_param;
	KDRV_DCE_UPDATE	            update;
} KDRV_DCE_IQ_CFG;

typedef struct {
	KDRV_DCE_IO_CFG *p_iocfg;
	KDRV_DCE_IQ_CFG *p_iqcfg;
	void*p_ll_blk;
} KDRV_DCE_JOB_CFG;

INT32 kdrv_dce_module_init(void);
INT32 kdrv_dce_module_uninit(void);

INT32 kdrv_dce_open(UINT32 chip, UINT32 engine);
INT32 kdrv_dce_close(UINT32 chip, UINT32 engine);
INT32 kdrv_dce_query(UINT32 id, KDRV_DCE_QUERY_ID qid, void *p_param);
INT32 kdrv_dce_set(UINT32 id, KDRV_DCE_PARAM_ID param_id, void *p_param);
INT32 kdrv_dce_get(UINT32 id, KDRV_DCE_PARAM_ID param_id, void *p_param);

INT32 kdrv_dce_rtos_init(void);
INT32 kdrv_dce_rtos_uninit(void);

void kdrv_dce_dump(void);
void kdrv_dce_dump_cfg(KDRV_DCE_JOB_CFG *p_cfg);
void kdrv_dce_dump_register(void);
UINT32 kdrv_dce_dbg_mode(void *p_hdl, UINT32 param_id, void *data);

#endif //_KDRV_DCE_H_
