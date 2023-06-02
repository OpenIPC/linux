/**
    Public header file for dal_ime

    This file is the header file that define the API and data type for dal_ime.

    @file       kdrv_ime.h
    @ingroup    mILibIPLCom
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _KDRV_IME_H_
#define _KDRV_IME_H_

#include "kdrv_videoprocess/kdrv_ipp_utility.h"

#define KDRV_IME_PM_COORD_TAB   	(4)
#define KDRV_IME_DS_COLOR_KEY_TAB	(4)
#define KDRV_IME_DS_PLT_TAB         (16)
#define KDRV_IME_DBCS_WT_LUT_TAB	(16)

#define KDRV_IME_3DNR_ME_SAD_PENALTY_TAB            8
#define KDRV_IME_3DNR_ME_SWITCH_THRESHOLD_TAB       8
#define KDRV_IME_3DNR_ME_DETAIL_PENALTY_TAB         8
#define KDRV_IME_3DNR_ME_PROBABILITY_TAB            8

#define KDRV_IME_3DNR_MD_SAD_COEFA_TAB              8
#define KDRV_IME_3DNR_MD_SAD_COEFB_TAB              8
#define KDRV_IME_3DNR_MD_SAD_STD_TAB                8
#define KDRV_IME_3DNR_MD_FINAL_THRESHOLD_TAB        2

#define KDRV_IME_3DNR_MD_ROI_FINAL_THRESHOLD_TAB    2

#define KDRV_IME_3DNR_MC_SAD_BASE_TAB               8
#define KDRV_IME_3DNR_MC_SAD_COEFA_TAB              8
#define KDRV_IME_3DNR_MC_SAD_COEFB_TAB              8
#define KDRV_IME_3DNR_MC_SAD_STD_TAB                8
#define KDRV_IME_3DNR_MC_FINAL_THRESHOLD_TAB        2

#define KDRV_IME_3DNR_MC_ROI_FINAL_THRESHOLD_TAB    2

#define KDRV_IME_3DNR_PS_MIX_RATIO_TAB              2
#define KDRV_IME_3DNR_PS_MIX_THRESHOLD_TAB          2
#define KDRV_IME_3DNR_PS_MIX_SLOPE_TAB              2
#define KDRV_IME_3DNR_PS_EDGE_THRESHOLD_TAB         2

#define KDRV_IME_3DNR_NR_LUMA_RESIDUE_TH_TAB        3
#define KDRV_IME_3DNR_NR_FREQ_WEIGHT_TAB            4
#define KDRV_IME_3DNR_NR_LUMA_WEIGHT_TAB            8
#define KDRV_IME_3DNR_NR_PRE_FILTER_STRENGTH_TAB    4
#define KDRV_IME_3DNR_NR_PRE_FILTER_RATION_TAB      2
#define KDRV_IME_3DNR_NR_SFILTER_STRENGTH_TAB       3
#define KDRV_IME_3DNR_NR_TFILTER_STRENGTH_TAB       3
#define KDRV_IME_3DNR_NR_LUMA_LUT_TAB               8
#define KDRV_IME_3DNR_NR_LUMA_RATIO_TAB             2
#define KDRV_IME_3DNR_NR_CHROMA_LUT_TAB             8
#define KDRV_IME_3DNR_NR_CHROMA_RATIO_TAB           2
#define KDRV_IME_3DNR_NR_TF0_FILTER_TAB				3

#define KDRV_IME_LCA_RANGE_TH_TAB      (3)
#define KDRV_IME_LCA_RANGE_WT_TAB      (4)
#define KDRV_IME_LCA_FILTER_TH_TAB     (5)
#define KDRV_IME_LCA_FILTER_WT_TAB     (6)


typedef enum {
	KDRV_IME_PARAM_CFG_PROCESS_CPU = 0,	/* write config to register.            SET-Only,   data_type: KDRV_IME_JOB_CFG */
	KDRV_IME_PARAM_CFG_PROCESS_LL,		/* write config to ll.                  SET-Only,   data_type: KDRV_IME_JOB_CFG */
	KDRV_IME_PARAM_TRIG_SINGLE,			/* trig ime start.                      SET-Only,   data_type: NULL */
	KDRV_IME_PARAM_TRIG_LL,				/* trig ime start by linklist.          SET-Only,   data_type: UINT32 */
	KDRV_IME_PARAM_TRIG_DIR,			/* trig ime start by linklist.          SET-Only,   data_type: UINT32 */
	KDRV_IME_PARAM_SET_CB,				/* set isr callback.                    SET-Only,   data_type: KDRV_IPP_ISR_CB */
	KDRV_IME_PARAM_STOP_SINGLE,			/* stop ime.                            SET-Only,   data_type: NULL */
	KDRV_IME_PARAM_GET_SINGLE_OUT,		/* get single out bit.                  GET-Only,   data_type: NULL */
	KDRV_IME_PARAM_HARD_RESET,			/* hard reset.                          SET-Only,   data_type: NULL */
	KDRV_IME_PARAM_GRAY_AVG,			/* get lca gray avg result from engine. GET-Only,   data_type: KDRV_IME_LCA_GRAY_AVG */
	KDRV_IME_PARAM_GET_DRAM_END_STS,	/* get dram end status.                 GET-Only,   data_type: NULL */
	KDRV_IME_PARAM_CLR_DRAM_END_STS,	/* clr dram end status.                 GET-Only,   data_type: NULL */
	KDRV_IME_PARAM_DMA_CH_ABORT,		/* dma abort config.                    SET-Only,   data_type: UINT32 */
	KDRV_IME_PARAM_DMA_CH_STS,			/* dma idle status.                     GET-Only,   data_type: UINT32 */
	KDRV_IME_PARAM_GET_REG_BASE_ADDR,	/* get reg base addr                    GET-Only,   data_type: UINT32 */
	KDRV_IME_PARAM_GET_REG_BASE_ADDR_LL,/* get reg base addr ll mode            GET-Only,   data_type: UINT32 */
	KDRV_IME_PARAM_ID_MAX,
} KDRV_IME_PARAM_ID;

typedef enum {
	KDRV_IME_QUERY_REG_NUM = 0,		/* get reg num for memory alloc.				data_tpye: UINT32 */
	KDRV_IME_QUERY_3DNR_BUFSIZE,	/* get 3dnr buffer size for memory alloc.		data_tpye: KDRV_IME_3DNR_QUERY_BUF_SIZE */
	KDRV_IME_QUERY_3DNR_STAINFO,	/* get 3dnr sta information.					data_tpye: KDRV_IME_3DNR_QUERY_STA_INFO */
	KDRV_IME_QUERY_STRP_INFO,		/* get ime stripe info for dce cal.        		data_type: KDRV_IME_GET_STRP_INFO */
	KDRV_IME_QUERY_ID_MAX,
} KDRV_IME_QUERY_ID;

typedef enum {
	KDRV_IME_INTERRUPT_LL_END              = 0x00000001,
	KDRV_IME_INTERRUPT_LL_ERR              = 0x00000002,
	KDRV_IME_INTERRUPT_LL_LATE             = 0x00000004,
	KDRV_IME_INTERRUPT_LL_JEND             = 0x00000008,
	KDRV_IME_INTERRUPT_BP1                 = 0x00000010,
	KDRV_IME_INTERRUPT_BP2                 = 0x00000020,
	KDRV_IME_INTERRUPT_BP3                 = 0x00000040,
	KDRV_IME_INTERRUPT_3DNR_SLICE_END      = 0x00000080,
	KDRV_IME_INTERRUPT_3DNR_MOT_END        = 0x00000100,
	KDRV_IME_INTERRUPT_3DNR_MV_END         = 0x00000200,
	KDRV_IME_INTERRUPT_3DNR_STA_END        = 0x00000400,
	KDRV_IME_INTERRUPT_P1_ENC_OVR          = 0x00000800,
	KDRV_IME_INTERRUPT_3DNR_ENC_OVR        = 0x00001000,
	KDRV_IME_INTERRUPT_3DNR_DEC_ERR        = 0x00002000,
	KDRV_IME_INTERRUPT_FRM_ERR             = 0x00004000,
	KDRV_IME_INTERRUPT_FRM_START           = 0x20000000,  ///< frame-start
	KDRV_IME_INTERRUPT_STRP_END            = 0x40000000,  ///< stripe-end
	KDRV_IME_INTERRUPT_FRM_END             = 0x80000000,  ///< frame-end
	KDRV_IME_INTERRUPT_ALL			 	   = (KDRV_IME_INTERRUPT_LL_END | KDRV_IME_INTERRUPT_LL_ERR | KDRV_IME_INTERRUPT_LL_LATE | KDRV_IME_INTERRUPT_LL_JEND | KDRV_IME_INTERRUPT_BP1 |
												KDRV_IME_INTERRUPT_BP2 | KDRV_IME_INTERRUPT_BP3 | KDRV_IME_INTERRUPT_3DNR_SLICE_END | KDRV_IME_INTERRUPT_3DNR_MOT_END | KDRV_IME_INTERRUPT_3DNR_MV_END |
												KDRV_IME_INTERRUPT_3DNR_STA_END | KDRV_IME_INTERRUPT_P1_ENC_OVR | KDRV_IME_INTERRUPT_3DNR_ENC_OVR | KDRV_IME_INTERRUPT_3DNR_DEC_ERR | KDRV_IME_INTERRUPT_FRM_ERR |
												KDRV_IME_INTERRUPT_FRM_START | KDRV_IME_INTERRUPT_STRP_END | KDRV_IME_INTERRUPT_FRM_END)
} KDRV_IME_INTERRUPT;

typedef enum {
	KDRV_IME_UPDATE_LCA				= 0x00000001,
	KDRV_IME_UPDATE_3DNR			= 0x00000002,
	KDRV_IME_UPDATE_DBCS			= 0x00000004,
	KDRV_IME_UPDATE_YUVCVT			= 0x00000008,
	KDRV_IME_UPDATE_COMPRESS		= 0x00000010,	/* special case, all parameter is define in kdrv_ime, only set this bit to set once */
	KDRV_IME_UPDATE_LCA_GRAY_STA	= 0x00000020,
	KDRV_IME_UPDATE_LCA_REFCENT		= 0x00000040,
	KDRV_IME_UPDATE_LCA_FILTER		= 0x00000080,
	KDRV_IME_UPDATE_ALL				= (KDRV_IME_UPDATE_LCA | KDRV_IME_UPDATE_3DNR | KDRV_IME_UPDATE_DBCS | KDRV_IME_UPDATE_YUVCVT | KDRV_IME_UPDATE_COMPRESS |
										KDRV_IME_UPDATE_LCA_GRAY_STA | KDRV_IME_UPDATE_LCA_REFCENT | KDRV_IME_UPDATE_LCA_FILTER)
} KDRV_IME_UPDATE;

/*********************
Get stripe info
**********************/
typedef struct {
	KDRV_IPP_OPMODE mode;
	UINT8 _3dnr_en;			/* 3dnr enable */
	UINT8 yuv422_one_en;	/* yuv422 one plane format enable */
	UINT8 yuv_compress_en;	/* nvx2 format enable */

	UINT32 in_width;		/* use to calculate max scale down rate */
	UINT32 path2_width;		/* 0 for disable */
	UINT32 path3_width;		/* 0 for disable */
	UINT32 lca_width;		/* 0 for disable */
} KDRV_IME_STRP_CAL_INFO;

typedef struct {
	UINT16 alignment;
	UINT16 overlap;
	UINT16 partition;
} KDRV_IME_STRP_RST;

typedef struct {
	KDRV_IME_STRP_CAL_INFO cal_info;
	KDRV_IME_STRP_RST rst;
} KDRV_IME_GET_STRP_INFO;

/*********************
Output path Structure
**********************/
typedef enum {
	KDRV_IME_PATH_1 = 0,
	KDRV_IME_PATH_2,
	KDRV_IME_PATH_3,
	KDRV_IME_PATH_4,
	KDRV_IME_PATH_REF,
	KDRV_IME_PATH_NUM_MAX,
} KDRV_IME_PATH_ID;

typedef enum {
	KDRV_IME_SCL_BICUBIC     = 0,    ///< bicubic interpolation
	KDRV_IME_SCL_BILINEAR    = 1,    ///< bilinear interpolation
	KDRV_IME_SCL_NEAREST     = 2,    ///< nearest interpolation
	KDRV_IME_SCL_INTEGRATION = 3,    ///< integration interpolation,support only scale down
	KDRV_IME_SCL_AUTO        = 4,    ///< automatical calculation
	KDRV_IME_SCL_METHOD_MAX,
} KDRV_IME_SCL_METHOD;

typedef struct {
	UINT8 enable;
	UINT8 dma_en;
	UINT8 flip_en;
	KDRV_IPP_FRAME frm;
	USIZE scl_size;
	URECT crp_window;

	UINT8 sprt_en;
	UINT32 sprt_pos;
	UINT32 sprt_addr2[3];
	UINT32 sprt_lofs2[3];
} KDRV_IME_OUT_PATH;

typedef struct {
	UINT32 scl_th;					/* scale done ratio threshold, [31..16]output, [15..0]input */
	KDRV_IME_SCL_METHOD method_l;	/* scale method when scale down ratio <= scl_th(output/input) */
	KDRV_IME_SCL_METHOD method_h;	/* scale method when scale down ratio >  scl_th(output/input) */
} KDRV_IME_SCL_METHOD_SEL;

/*********************
LCA Structure
**********************/
typedef struct {
	UINT8 in_enable;	/* lca function enable */
	UINT8 in_bypass;	/* lca bypass enable */
	UINT32 in_addr;
	UINT32 in_lofs;
	USIZE in_size;
	UINT8 cent_u;	/* chroma adjustment reference center of U channel */
	UINT8 cent_v;	/* chroma adjustment reference center of V channel */

	UINT8 out_en;	/* lca subout enable */
	UINT32 out_addr;
	UINT32 out_lofs;
	USIZE out_size;
} KDRV_IME_LCA_IOCFG;

typedef enum {
	KDRV_IME_RANGE_8   = 0,  ///< range 8
	KDRV_IME_RANGE_16  = 1,  ///< range 16
	KDRV_IME_RANGE_32  = 2,  ///< range 32
	KDRV_IME_RANGE_64  = 3,  ///< range 64
	KDRV_IME_RANGE_128 = 4,  ///< range 128
	ENUM_DUMMY4WORD(KDRV_IME_RANGE_SEL)
} KDRV_IME_RANGE_SEL;

typedef enum {
	KDRV_IME_LCA_SUBOUT_SRC_A = 0, ///< source: after LCA
	KDRV_IME_LCA_SUBOUT_SRC_B = 1, ///< source: before LCA
} KDRV_IME_LCA_SUBOUT_SRC;

typedef struct {
	UINT8                  ref_y_wt;   ///< Chroma reference weighting for Y channels
	UINT8                  ref_uv_wt;  ///< Chroma reference weighting for UV channels
	UINT8                  out_uv_wt;  ///< Chroma adaptation output weighting

	KDRV_IME_RANGE_SEL     y_rng;      ///< Chroma adaptation range selection for UV channels, Max = KDRV_IME_RANGE_128
	KDRV_IME_RANGE_SEL     y_wt_prc;   ///< Chroma adaptation weighting precision for UV channel, Max = KDRV_IME_RANGE_64
	UINT8                  y_th;       ///< Chroma adaptation range threshold for UV channels
	UINT8                  y_wt_s;     ///< Chroma adaptation weighting start value for UV channels
	UINT8                  y_wt_e;     ///< Chroma adaptation weighting end value for UV channels


	KDRV_IME_RANGE_SEL     uv_rng;     ///< Chroma adaptation range selection for UV channels, Max = KDRV_IME_RANGE_128
	KDRV_IME_RANGE_SEL     uv_wt_prc;  ///< Chroma adaptation weighting precision for UV channel, Max = KDRV_IME_RANGE_64
	UINT8                  uv_th;      ///< Chroma adaptation range threshold for UV channels
	UINT8                  uv_wt_s;    ///< Chroma adaptation weighting start value for UV channels
	UINT8                  uv_wt_e;    ///< Chroma adaptation weighting end value for UV channels
} KDRV_IME_LCA_IQC_PARAM;

typedef struct {
	UINT8              enable;      ///< Function enable
	KDRV_IME_RANGE_SEL uv_rng;      ///< Chroma adjustment range selection for UV channels, Max = KDRV_IME_RANGE_128
	KDRV_IME_RANGE_SEL uv_wt_prc;   ///< Chroma adjustment weighting precision for UV channel, Max = KDRV_IME_RANGE_64
	UINT8              uv_th;       ///< Chroma adjustment range threshold for UV channels
	UINT8              uv_wt_s;     ///< Chroma adjustment weighting start value for UV channels
	UINT8              uv_wt_e;     ///< Chroma adjustment weighting end value for UV channels
} KDRV_IME_LCA_CA_PARAM;

typedef struct {
	UINT8                  enable;     ///< Function enable
	UINT8                  ref_wt;     ///< Luma adaptation reference weighting for UV channels
	UINT8                  out_wt;     ///< Luma adaptation output weighting
	KDRV_IME_RANGE_SEL     rng;        ///< Luma adaptation range selection for UV channels, Max = KDRV_IME_RANGE_128
	KDRV_IME_RANGE_SEL     wt_prc;     ///< Luma adaptation weighting precision for UV channel, Max = KDRV_IME_RANGE_64
	UINT8                  th;         ///< Luma adaptation range threshold for UV channels
	UINT8                  wt_s;       ///< Luma adaptation weighting start value for UV channels
	UINT8                  wt_e;       ///< Luma adaptation weighting end value for UV channels
} KDRV_IME_LCA_IQL_PARAM;

typedef struct {
	UINT8 u_th0;			///< Gray Statistical threshold for U channel
	UINT8 u_th1;			///< Gray Statistical threshold for U channel
	UINT8 v_th0;			///< Gray Statistical threshold for V channel
	UINT8 v_th1;			///< Gray Statistical threshold for V channel
} KDRV_IME_LCA_GRAY_STA_TH;

typedef struct {
	UINT8 rng_th[KDRV_IME_LCA_RANGE_TH_TAB];	///< UINT8[3], Range filter threshold for reference center
	UINT8 rng_wt[KDRV_IME_LCA_RANGE_WT_TAB];	///< UINT8[4], Range filter weighting for reference center
	UINT8 cent_wt;								///< Reference center weighting
	UINT8 outl_dth;								///< Outlier difference threshold
	UINT8 outl_th;								///< Reference center outlier threshold
} KDRV_IME_LCA_REFCENT;

typedef struct {
	KDRV_IME_LCA_REFCENT y;		///< for Y channel
	KDRV_IME_LCA_REFCENT uv;	///< for UV channel
} KDRV_IME_LCA_REFCENT_PARAM;

typedef enum {
	KDRV_IME_LCA_FLTR_SIZE_3x3 = 0,			///< 3x3 window
	KDRV_IME_LCA_FLTR_SIZE_5x5 = 1,			///< 5x5 window
	KDRV_IME_LCA_FLTR_SIZE_7x7 = 2,			///< 7x7 window
	KDRV_IME_LCA_FLTR_SIZE_9x9 = 3,			///< 9x9 window
} KDRV_IME_LCA_FILT_SIZE;

typedef struct {
	UINT8  hv_th;					///< Edge direction threshold for two diagonal directions
	UINT8  pn_th;					///< Edge direction threshold for horizontal and vertical directions
} KDRV_IME_LCA_EDG_DIR_TH_PARAM;

typedef enum {
	KDRV_IME_LCA_EKNL_SIZE_3x3 = 0,			///< 3x3 window
	KDRV_IME_LCA_EKNL_SIZE_5x5 = 1,			///< 5x5 window
} KDRV_IME_LCA_EDGE_KER_SIZE;

typedef struct {
	UINT8 th[KDRV_IME_LCA_FILTER_TH_TAB];	///< UINT32[5], Filter threshold LUT
	UINT8 wt[KDRV_IME_LCA_FILTER_WT_TAB];	///< UINT32[6], Filter weighting LUT
} KDRV_IME_LCA_FILTER;

typedef struct {
	UINT8 enable;								///< filter Y channel enable
	KDRV_IME_LCA_FILT_SIZE size;				///< filter size
	KDRV_IME_LCA_EDG_DIR_TH_PARAM edg_dir;		///< filter edge direction threshold
	KDRV_IME_LCA_EDGE_KER_SIZE edg_ker_size;	///< edge kernel size
	KDRV_IME_LCA_FILTER set_y;					///< Filter parameters for Y Channel
	KDRV_IME_LCA_FILTER set_u;					///< Filter parameters for U Channel
	KDRV_IME_LCA_FILTER set_v;					///< Filter parameters for V Channel
} KDRV_IME_LCA_FILTER_PARAM;

typedef struct {
	KDRV_IME_LCA_IQC_PARAM      chroma;			///< KDRV_IME_UPDATE_LCA: Function quality information for chroma channel
	KDRV_IME_LCA_IQL_PARAM      luma;			///< KDRV_IME_UPDATE_LCA: Function quality information for luma channel
	KDRV_IME_LCA_CA_PARAM       ca;				///< KDRV_IME_UPDATE_LCA: Chroma adjustment information
	KDRV_IME_LCA_SUBOUT_SRC     sub_out_src;	///< KDRV_IME_UPDATE_LCA: LCA subout source selection

	KDRV_IME_LCA_GRAY_STA_TH	gray_sta_th;	///< KDRV_IME_UPDATE_LCA_GRAY_STA: gray statistical (only for U/V channel)
	KDRV_IME_LCA_REFCENT_PARAM	refcent;		///< KDRV_IME_UPDATE_LCA_REFCENT: reference center calculation parameters for Y/UV channel
	KDRV_IME_LCA_FILTER_PARAM	filter;			///< KDRV_IME_UPDATE_LCA_FILTER: LCA filter param
} KDRV_IME_LCA_IQCFG;

typedef struct {
	UINT32 u;			///< average for U channel
	UINT32 v;			///< average for V channel
} KDRV_IME_LCA_GRAY_AVG;

/*********************
3DNR Structure
**********************/

typedef struct {
	/* input parameter */
	USIZE size;
	UINT32 sta_max_spl_num;

	/* buffer size result */
	UINT32 mv_size;
	UINT32 ms_size;
	UINT32 ms_roi_size;
	UINT32 sta_size;
	UINT32 fcvg_size;
} KDRV_IME_3DNR_QUERY_BUF_SIZE;

typedef struct {
	/* input parameter */
	USIZE size;
	UINT32 sta_max_spl_num;

	/* sta info result */
	UINT32 lofs;
	UINT32 buf_size;
	UINT32 spl_step_h;
	UINT32 spl_step_v;
	UINT32 spl_num_x;
	UINT32 spl_num_y;
	UINT32 spl_st_x;
	UINT32 spl_st_y;
} KDRV_IME_3DNR_QUERY_STA_INFO;

typedef struct {
	UINT8 out_ms_roi_en;
	UINT8 out_ms_roi_flip_en;
	UINT8 out_sta_en;

	KDRV_IPP_FRAME in_ref_frm;
	UINT8 in_ref_flip_enable;
	UINT32 in_mv_addr;
	UINT32 in_ms_addr;
	UINT32 in_fcvg_addr;

	UINT32 out_mv_addr;
	UINT32 out_ms_addr;
	UINT32 out_ms_roi_addr;
	UINT32 out_sta_addr;
	UINT32 out_sta_max_spl_num;
	UINT32 out_fcvg_addr;
} KDRV_IME_3DNR_IOCFG;

typedef enum {
	KDRV_ME_UPDATE_RAND  = 0,
	KDRV_ME_UPDATE_FIXED = 1,
	ENUM_DUMMY4WORD(KDRV_IME_3DNR_ME_UPDATE_MODE)
} KDRV_IME_3DNR_ME_UPDATE_MODE;

typedef enum {
	KDRV_MV_DOWN_SAMPLE_NEAREST  = 0,
	KDRV_MV_DOWN_SAMPLE_AVERAGE  = 1,
	KDRV_MV_DOWN_SAMPLE_MINIMUM  = 2,
	KDRV_MV_DOWN_SAMPLE_MAXIMUM  = 3,
	ENUM_DUMMY4WORD(KDRV_IME_3DNR_MV_DOWN_SAMPLE_MODE)
} KDRV_IME_3DNR_MV_DOWN_SAMPLE_MODE;

typedef enum {
	KDRV_ME_SDA_TYPE_NATIVE = 0,		///< native SAD
	KDRV_ME_SDA_TYPE_COMPENSATED = 1,	///< compensated SAD
	ENUM_DUMMY4WORD(KDRV_IME_3DNR_ME_SDA_TYPE)
} KDRV_IME_3DNR_ME_SDA_TYPE;

typedef struct {
	KDRV_IME_3DNR_ME_UPDATE_MODE update_mode;	///< An option of update mode in motion estimation. Range 0~1
	UINT8 boundary_set;  						///< Set image boundary as still type or not.  Range 0~1
	KDRV_IME_3DNR_MV_DOWN_SAMPLE_MODE ds_mode;	///< Method option of motion vector down-sampling.Range 0~3
	KDRV_IME_3DNR_ME_SDA_TYPE sad_type;			///< Option for SAD statistic data.  Range 0~1
	UINT8 sad_shift;							///< Right shift number for SAD statistic data, range = [0, 15]
	UINT8 cost_blend;							///< Cost blending ratio, 0 for automatic cost, 0xF for user define

	UINT8 rand_bit_x;	///<range 1~7
	UINT8 rand_bit_y;	///<range 1~7
	UINT16 min_detail;	///< range 0~16383

	UINT16 sad_penalty[KDRV_IME_3DNR_ME_SAD_PENALTY_TAB];		///< range 0~1023
	UINT8 switch_th[KDRV_IME_3DNR_ME_SWITCH_THRESHOLD_TAB];	///< range 0~255
	UINT8 switch_rto;											///< range 0~255
	UINT8 detail_penalty[KDRV_IME_3DNR_ME_DETAIL_PENALTY_TAB]; ///< range 0~15
	UINT8 probability[KDRV_IME_3DNR_ME_PROBABILITY_TAB];		///< range 0~1
} KDRV_IME_3DNR_ME_PARAM;

typedef struct {
	UINT8 sad_coefa[KDRV_IME_3DNR_MD_SAD_COEFA_TAB];	///< Edge coefficient of motion detection. Range 0~63
	UINT16 sad_coefb[KDRV_IME_3DNR_MD_SAD_COEFB_TAB];	///< Offset of motion detection. Range 0~16383
	UINT16 sad_std[KDRV_IME_3DNR_MD_SAD_STD_TAB];		///< Standard deviation of motion detection. Range 0~16383
	UINT8 fth[KDRV_IME_3DNR_MD_FINAL_THRESHOLD_TAB];	///< final threshold of motion detection. Range 0~63
} KDRV_IME_3DNR_MD_PARAM;

typedef struct {
	UINT8 fth[KDRV_IME_3DNR_MD_ROI_FINAL_THRESHOLD_TAB];   ///< final threshold of motion detection.Range 0~63
} KDRV_IME_3DNR_MD_ROI_PARAM;

typedef struct {
	UINT16 sad_base[KDRV_IME_3DNR_MC_SAD_BASE_TAB];		///< base level of noise. Range 0~16383
	UINT8 sad_coefa[KDRV_IME_3DNR_MC_SAD_COEFA_TAB];	///< edge coefficient of motion compensation.Range 0~63
	UINT16 sad_coefb[KDRV_IME_3DNR_MC_SAD_COEFB_TAB];	///< offset of motion compensation.Range 0~16383
	UINT16 sad_std[KDRV_IME_3DNR_MC_SAD_STD_TAB];		///< standard deviation of motion compensation.Range 0~16383
	UINT8 fth[KDRV_IME_3DNR_MC_FINAL_THRESHOLD_TAB];	///< final threshold of motion compensation. Range 0~63
} KDRV_IME_3DNR_MC_PARAM;

typedef struct {
	UINT8 fth[KDRV_IME_3DNR_MC_ROI_FINAL_THRESHOLD_TAB];   ///< final threshold of motion compensation.Range 0~63
} KDRV_IME_3DNR_MC_ROI_PARAM;

typedef enum {
	KDRV_PS_MODE_0 = 0,   ///< determined by object type
	KDRV_PS_MODE_1 = 1,   ///< determined by a LUT mode
	ENUM_DUMMY4WORD(KDRV_IME_3DNR_PS_MODE)
} KDRV_IME_3DNR_PS_MODE;

typedef enum {
	KDRV_MV_INFO_MODE_AVERAGE  = 0,
	KDRV_MV_INFO_MODE_LPF      = 1,
	KDRV_MV_INFO_MODE_MINIMUM  = 2,
	KDRV_MV_INFO_MODE_MAXIMUM  = 3,
	ENUM_DUMMY4WORD(KDRV_IME_3DNR_MV_INFO_MODE)
} KDRV_IME_3DNR_MV_INFO_MODE;

typedef struct {
	UINT8 smart_roi_ctrl_en;						///< ROI control. Range 0~1
	UINT8 mv_check_en;							///< MV checkiong process in PS module. Range 0~1
	UINT8 roi_mv_check_en;						///< MV checking process for ROI. Range 0~1
	KDRV_IME_3DNR_PS_MODE ps_mode;				///< Method option of patch selection. Range 0~1
	KDRV_IME_3DNR_MV_INFO_MODE mv_info_mode;	///< Mode option for MV length calculation. Range 0~3
	UINT8 mv_th;										///< MV threshold. Range 0~63
	UINT8 roi_mv_th;									///< MV threshold for ROI. Range 0~63
	UINT8 mix_ratio[KDRV_IME_3DNR_PS_MIX_RATIO_TAB];	///< Mix ratio in patch selection. Range 0~255
	UINT16 mix_th[KDRV_IME_3DNR_PS_MIX_THRESHOLD_TAB];	///< Mix threshold in patch selection. Range 0~16383.
	UINT8 ds_th;										///< Threshold of motion status down-sampling. Range 0~31
	UINT8 ds_th_roi;									///< Threshold for motion status down-sampling for ROI.Range 0~31
	UINT8 edge_wet;									///< Start point of edge adjustment. Range 0~255
	UINT16 edge_th[KDRV_IME_3DNR_PS_EDGE_THRESHOLD_TAB];///< Edge adjustment threshold in patch selection. Range 0~16383
	UINT16 fs_th;										////< Threshold of patch error. Range 0~16383.
} KDRV_IME_3DNR_PS_PARAM;

typedef enum {
	KDRV_PRE_FILTER_Y_STR_DISABLE = 0,  ///< off
	KDRV_PRE_FILTER_Y_STR_1 = 1,    ///< type 1 filter
	KDRV_PRE_FILTER_Y_STR_2 = 2,    ///< type 2 filter
	ENUM_DUMMY4WORD(KDRV_IME_3DNR_PRE_FILTER_Y_STR)
} KDRV_IME_3DNR_PRE_FILTER_Y_STR;

typedef enum {
	KDRV_PRE_FILTER_DISABLE = 0,    ///< disable
	KDRV_PRE_FILTER_TYPE_1 = 1,     ///< 3x3
	KDRV_PRE_FILTER_TYPE_2 = 2,     ///< 3x3
	KDRV_PRE_FILTER_TYPE_3 = 3,     ///< 5x5
	ENUM_DUMMY4WORD(KDRV_IME_3DNR_PRE_FILTER_TYPE)
} KDRV_IME_3DNR_PRE_FILTER_TYPE;

typedef struct {
	UINT8 luma_ch_en;	///< noise filter enable for luma channel. Range 0~1
	UINT8 chroma_ch_en;	///< noise filter enable for chroma channel.Range 0~1

	UINT8 center_wzeros_y;	///< Set weighting as 0 to the center pixel in internal NR-filter. Range 0~1
	UINT8 chroma_fsv_en;		///< False color control.Range 0~1
	UINT8 chroma_fsv;		///< False color value. Range 0~255


	UINT8 luma_residue_th[KDRV_IME_3DNR_NR_LUMA_RESIDUE_TH_TAB];		///< Protection threshold of luma channel. Range 0~255
	UINT8 chroma_residue_th;	///< Protection threshold of luma channel. Range 0~255

	UINT8 freq_wet[KDRV_IME_3DNR_NR_FREQ_WEIGHT_TAB];	///< Filter weighting for low frequency. Range 0~255
	UINT8 luma_wet[KDRV_IME_3DNR_NR_LUMA_WEIGHT_TAB];	///< Filter intensity weighting. Range 0~255

	KDRV_IME_3DNR_PRE_FILTER_Y_STR pre_y_blur_str;					///< Strength option for luma channel pre-filter.Range 0~2
	KDRV_IME_3DNR_PRE_FILTER_TYPE  pf_type;					///< pre filter kernel select
	UINT8 pre_filter_str[KDRV_IME_3DNR_NR_PRE_FILTER_STRENGTH_TAB];	///< Strength of pre-filtering for low frequency. Range 0~255
	UINT8 pre_filter_rto[KDRV_IME_3DNR_NR_PRE_FILTER_RATION_TAB];		///< adjustment ratio of pre-filtering for transitional object.Range 0~255

	UINT8 snr_str[KDRV_IME_3DNR_NR_SFILTER_STRENGTH_TAB];	///< Strength of spatial filter for still object.Range 0~255
	UINT8 tnr_str[KDRV_IME_3DNR_NR_TFILTER_STRENGTH_TAB];	///< Strength of temporal filter for still object.Range 0~255

	UINT32 snr_base_th;	///< Base threshold of spatial noise reduction.Range 0~65535
	UINT32 tnr_base_th;	///< Base threshold of spatial noise reduction.Range 0~65535

	UINT8 luma_3d_lut[KDRV_IME_3DNR_NR_LUMA_LUT_TAB];			///< Noise reduction LUT for luma channel.Range 0~127
	UINT8 luma_3d_rto[KDRV_IME_3DNR_NR_LUMA_RATIO_TAB];			///< Adjustment ratio 0 of noise reduction LUT for luma channel.Range 0~255
	UINT8 chroma_3d_lut[KDRV_IME_3DNR_NR_CHROMA_LUT_TAB];		///< Noise reduction LUT for chroma channel.Range 0~127
	UINT8 chroma_3d_rto[KDRV_IME_3DNR_NR_CHROMA_RATIO_TAB];		///< Adjustment ratio 0 of noise reduction LUT for chroma channel.Range 0~255

	UINT8 luma_nr_type;	///< 3DNR filter option, Range 0~1

	UINT8 tf0_blur_str[KDRV_IME_3DNR_NR_TF0_FILTER_TAB];
	UINT8 tf0_y_str[KDRV_IME_3DNR_NR_TF0_FILTER_TAB];
	UINT8 tf0_c_str[KDRV_IME_3DNR_NR_TF0_FILTER_TAB];
} KDRV_IME_3DNR_NR_PARAM;

typedef struct {
	UINT8 mv0;		///< debug for mv = 0
	UINT8 mode;	///< debug mode selection. Range 0~6
} KDRV_IME_3DNR_DBG_PARAM;

typedef struct {
	UINT8 en;
	UINT8 start_point;
	UINT8 step_size;
} KDRV_IME_3DNR_FCVG_PARAM;

typedef struct {
	UINT8 en;
	KDRV_IME_3DNR_ME_PARAM me;
	KDRV_IME_3DNR_MD_PARAM md;
	KDRV_IME_3DNR_MD_ROI_PARAM md_roi;
	KDRV_IME_3DNR_MC_PARAM mc;
	KDRV_IME_3DNR_MC_ROI_PARAM mc_roi;
	KDRV_IME_3DNR_PS_PARAM ps;
	KDRV_IME_3DNR_NR_PARAM nr;
	KDRV_IME_3DNR_DBG_PARAM dbg;
	KDRV_IME_3DNR_FCVG_PARAM fcvg;
} KDRV_IME_3DNR_IQCFG;

/*********************
DBCS Structure
**********************/

typedef enum {
	KDRV_IME_DBCS_DK_MODE   = 0,   ///< dark mode
	KDRV_IME_DBCS_BT_MODE   = 1,   ///< bright mode
	KDRV_IME_DBCS_BOTH_MODE  = 2,  ///< dark and bright mode
	ENUM_DUMMY4WORD(KDRV_IME_DBCS_MODE_SEL)
} KDRV_IME_DBCS_MODE_SEL;

typedef struct {
	UINT8                   enable;     ///< dark and bright region chroma suppression function enable
	KDRV_IME_DBCS_MODE_SEL  op_mode;    ///< Process mode
	UINT8                   cent_u;     ///< Center value for U channel
	UINT8                   cent_v;     ///< Center value for V channel
	UINT16                  step_y;     ///< Step for luma
	UINT16                  step_c;     ///< Step for chroma
	UINT8                   wt_y[KDRV_IME_DBCS_WT_LUT_TAB];     ///< Weighting LUT for luma channel, 16 elements, range: [0, 16]
	UINT8                   wt_c[KDRV_IME_DBCS_WT_LUT_TAB];     ///< Weighting LUT for luma channel, 16 elements, range: [0, 16]
} KDRV_IME_DBCS_IQCFG;

/*********************
Privacy Mask Structure
**********************/
typedef enum {
	KDRV_IME_PM_SET_IDX_0   = 0,
	KDRV_IME_PM_SET_IDX_1   = 1,
	KDRV_IME_PM_SET_IDX_2   = 2,
	KDRV_IME_PM_SET_IDX_3   = 3,
	KDRV_IME_PM_SET_IDX_4   = 4,
	KDRV_IME_PM_SET_IDX_5   = 5,
	KDRV_IME_PM_SET_IDX_6   = 6,
	KDRV_IME_PM_SET_IDX_7   = 7,
	KDRV_IME_PM_SET_IDX_MAX = 8,
} KDRV_IME_PM_SET_IDX;

typedef enum {
	KDRV_IME_PM_MASK_TYPE_YUV = 0,  ///< Using YUV color
	KDRV_IME_PM_MASK_TYPE_PXL = 1,  ///< Using pixilation
	KDRV_IME_PM_MASK_TYPE_UNKNOWN,
	ENUM_DUMMY4WORD(KDRV_IME_PM_MASK_TYPE)
} KDRV_IME_PM_MASK_TYPE;

typedef enum {
	KDRV_IME_PM_PIXELATION_08 = 0,  ///< 8x8
	KDRV_IME_PM_PIXELATION_16 = 1,  ///< 16x16
	KDRV_IME_PM_PIXELATION_32 = 2,  ///< 32x32
	KDRV_IME_PM_PIXELATION_64 = 3,  ///< 64x64
	KDRV_IME_PM_PIXELATION_UNKNOWN,
	ENUM_DUMMY4WORD(KDRV_IME_PM_PXL_BLK_SIZE)
} KDRV_IME_PM_PXL_BLK_SIZE;

typedef struct {
	KDRV_IME_PM_SET_IDX         set_idx;                        ///< total 8 set, idx = 0~7
	UINT8                       enable;                         ///< privacy mask set enable
	UINT8                       hlw_enable;                     ///< privacy mask hollow function enable
	IPOINT                      coord[KDRV_IME_PM_COORD_TAB];   ///< privacy mask 4 coordinates(horizontal and vertical direction), must be Convex Hull
	IPOINT                      coord2[KDRV_IME_PM_COORD_TAB];   ///< privacy mask 4 coordinates(horizontal and vertical direction) for hollow, must be Convex Hull
	KDRV_IME_PM_MASK_TYPE       msk_type;                       ///< privacy mask type selection
	UINT8                       color[3];     					///< privacy mask using YUV color, range: [0, 255]
	UINT8                       weight;                         ///< privacy mask alpha weight, range: [0, 255]
} KDRV_IME_PM_INFO;

typedef struct {
	USIZE                       img_size;   ///< pixelation input image size of privacy mask
	KDRV_IME_PM_PXL_BLK_SIZE    blk_size;   ///< block size of pixelation
	UINT32                      lofs;       ///< pixelation input image lineoffset
	UINT32                      dma_addr;   ///< pixelation input imaeg buffer address
} KDRV_IME_PM_PXL_IMG_INFO;

/*********************
OSD Structure
**********************/

typedef enum {
	KDRV_IME_OSD_FMT_RGB565   = 0,  ///< OSD format, RGB565
	KDRV_IME_OSD_FMT_RGB1555  = 1,  ///< OSD format, RGB1555
	KDRV_IME_OSD_FMT_RGB4444  = 2,  ///< OSD format, RGB4444
	KDRV_IME_OSD_FMT_RGB8888  = 3,  ///< OSD format, RGB8888
	KDRV_IME_OSD_FMT_UNKNOWN,
	ENUM_DUMMY4WORD(KDRV_IME_OSD_FMT_SEL)
} KDRV_IME_OSD_FMT_SEL;

typedef enum {
	KDRV_IME_OSD_SET_IDX_0   = 0,
	KDRV_IME_OSD_SET_IDX_1   = 1,
	KDRV_IME_OSD_SET_IDX_2   = 2,
	KDRV_IME_OSD_SET_IDX_3   = 3,
	KDRV_IME_OSD_SET_IDX_MAX = 4,
} KDRV_IME_OSD_SET_IDX;

typedef enum {
	KDRV_IME_OSD_CKEY_RGB_MODE = 0, ///< using RGB mode
	KDRV_IME_OSD_CKEY_ARGB_MODE = 1, ///< using ARGB mode
	KDRV_IME_OSD_CKEY_MODE_UNKNOWN,
	ENUM_DUMMY4WORD(KDRV_IME_OSD_COLOR_KEY_MODE_SEL)
} KDRV_IME_OSD_COLOR_KEY_MODE_SEL;

typedef struct {
	USIZE                   size;   ///< Image size, maximum 8192x8192
	KDRV_IME_OSD_FMT_SEL    fmt;    ///< Image format selection
	UPOINT                  pos;    ///< Blending position
	UINT32                  lofs;   ///< Data lineoffset
	UINT32                  addr;   ///< Data address
} KDRV_IME_OSD_IMAGE_INFO;

typedef struct {
	UINT8                           color_key_en;   ///< color key enable
	KDRV_IME_OSD_COLOR_KEY_MODE_SEL color_key_mode;
	UINT8                           color_key_val[KDRV_IME_DS_COLOR_KEY_TAB];   ///< Color key, ,valid when color_key_en = TRUE
	///< color_key_val[0]: for A channel,
	///< color_key_val[1]: for R channel,
	///< color_key_val[2]: for G channel,
	///< color_key_val[3]: for B channel,
	UINT8 plt_en;         ///< palette mode enable
	UINT8 bld_wt_0;       ///< Blending weighting, if RGB565, range: [0, 16]; others, range: [0, 15]
	UINT8 bld_wt_1;       ///< Blending weighting, range: [0, 15]
} KDRV_IME_OSD_IQ_INFO;

typedef struct {
	UINT8                       enable;         ///< Function enable
	KDRV_IME_OSD_IMAGE_INFO     image;          ///< Image information
	KDRV_IME_OSD_IQ_INFO        ds_iq;      ///< OSD quality information
} KDRV_IME_OSD_INFO;

typedef struct {
	UINT8  enable;             ///< color space trans enable
	UINT8  cst_auto_param_en;  ///< parameter auto mode enable
	UINT8  cst_coef0;          ///< Coefficient of color space transform
	UINT8  cst_coef1;          ///< Coefficient of color space transform
	UINT8  cst_coef2;          ///< Coefficient of color space transform
	UINT8  cst_coef3;          ///< Coefficient of color space transform
} KDRV_IME_OSD_CST_INFO;

typedef enum {
	KDRV_IME_OSD_PLT_1BIT_MODE = 0, ///< 1bit mode
	KDRV_IME_OSD_PLT_2BIT_MODE = 1, ///< 2bit mode
	KDRV_IME_OSD_PLT_4BIT_MODE = 2, ///< 4bit mode
	KDRV_IME_OSD_PLT_MODE_UNKNOWN,
	ENUM_DUMMY4WORD(KDRV_IME_OSD_PLT_MODE_SEL)
} KDRV_IME_OSD_PLT_MODE_SEL;

typedef struct _KDRV_IME_OSD_PLT_INFO {
	KDRV_IME_OSD_PLT_MODE_SEL   plt_mode;   ///< palette mode selection
	UINT8	plt_a[KDRV_IME_DS_PLT_TAB];     ///< color palette of A channel, range: [0, 255]
	UINT8	plt_r[KDRV_IME_DS_PLT_TAB];     ///< color palette of R channel, range: [0, 255]
	UINT8	plt_g[KDRV_IME_DS_PLT_TAB];     ///< color palette of G channel, range: [0, 255]
	UINT8	plt_b[KDRV_IME_DS_PLT_TAB];     ///< color palette of B channel, range: [0, 255]
} KDRV_IME_OSD_PLT_INFO;

/*********************
YCC Converter Structure
**********************/
typedef enum {
	KDRV_IME_YCC_CVT_NONE = 0,	///< disable ycc cvt
	KDRV_IME_YCC_CVT_BT601,		///< BT.601
	KDRV_IME_YCC_CVT_BT709,		///< BT.709
	ENUM_DUMMY4WORD(KDRV_IME_YCC_CVT)
} KDRV_IME_YCC_CVT;

typedef struct {
	KDRV_IME_YCC_CVT cvt_sel;
} KDRV_IME_YCC_CVT_IQCFG;

/*********************
single out info, get-only
**********************/
typedef struct {
	UINT32 single_out_ch;
	UINT32 single_out_en;
} KDRV_IME_SINGLE_OUT_INFO;

/*********************
low delay path select
**********************/
typedef struct {
	UINT8 en;
	KDRV_IME_PATH_ID pid;
} KDRV_IME_LOW_DELAY_INFO;

/*********************
bp config
**********************/
typedef struct {
	UINT32 bp1;		///< 0 for disable
	UINT32 bp2;
	UINT32 bp3;
} KDRV_IME_BP;

/*********************
IO/IQ Config
**********************/
typedef struct {
	KDRV_IPP_OPMODE mode;
	KDRV_IPP_FRAME in_frm;
	UINT32 inte_en;

	KDRV_IME_OUT_PATH out_path[KDRV_IME_PATH_NUM_MAX];
	KDRV_IME_SCL_METHOD_SEL out_scl_method_sel;
	KDRV_IME_LOW_DELAY_INFO low_delay;
	KDRV_IME_BP bp;

	KDRV_IME_LCA_IOCFG lca;
	KDRV_IME_3DNR_IOCFG _3dnr;

	KDRV_IME_PM_PXL_IMG_INFO pm_pxlimg;
	KDRV_IME_PM_INFO pm_mask[KDRV_IME_PM_SET_IDX_MAX];

	KDRV_IME_OSD_INFO ds[KDRV_IME_OSD_SET_IDX_MAX];
	KDRV_IME_OSD_CST_INFO ds_cst;
	KDRV_IME_OSD_PLT_INFO ds_plt;

	/* for kdrv_ipp config */
	UINT16 strp_max_width;	///< ime d2d mode will calculate in ime_eng, other mode decide by dce
	UINT16 strp_ovlp;
	UINT16 strp_prt;

	/* drame chk status */
	BOOL dram_sts_set_en;
	UINT8 dram_sts_chk_bit;
} KDRV_IME_IO_CFG;

typedef struct {
	KDRV_IME_LCA_IQCFG lca;
	KDRV_IME_3DNR_IQCFG _3dnr;
	KDRV_IME_DBCS_IQCFG dbcs;
	KDRV_IME_YCC_CVT_IQCFG yuv_cvt;
	KDRV_IME_UPDATE update;
} KDRV_IME_IQ_CFG;

typedef struct {
	KDRV_IME_IO_CFG *p_iocfg;
	KDRV_IME_IQ_CFG *p_iqcfg;
	void *p_ll_blk;
} KDRV_IME_JOB_CFG;


/*********************
kdrvier api
**********************/
INT32 kdrv_ime_module_init(void);
INT32 kdrv_ime_module_uninit(void);

INT32 kdrv_ime_open(UINT32 chip, UINT32 engine);
INT32 kdrv_ime_close(UINT32 chip, UINT32 engine);
INT32 kdrv_ime_query(UINT32 id, KDRV_IME_QUERY_ID qid, void *p_param);
INT32 kdrv_ime_set(UINT32 id, KDRV_IME_PARAM_ID param_id, void *p_param);
INT32 kdrv_ime_get(UINT32 id, KDRV_IME_PARAM_ID param_id, void *p_param);

INT32 ime_builtin_rtos_init(void);
void ime_builtin_rtos_uninit(void);

void kdrv_ime_dump(void);
void kdrv_ime_dump_cfg(KDRV_IME_JOB_CFG *p_cfg);
void kdrv_ime_dump_register(void);

#endif
