/**
    Public header file for dal_ife

    This file is the header file that define the API and data type for dal_ife.

    @file       kdrv_ife.h
    @ingroup    mILibIPLCom
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _KDRV_IFE_H_
#define _KDRV_IFE_H_

#include "kdrv_videoprocess/kdrv_ipp_utility.h"

#define KDRV_IFE_NRS_ORD_W_NUM		8
#define KDRV_IFE_NRS_B_OFS_NUM		6
#define KDRV_IFE_NRS_B_W_NUM		6
#define KDRV_IFE_NRS_B_RANGE_NUM	5
#define KDRV_IFE_NRS_B_TH_NUM		5
#define KDRV_IFE_NRS_GB_OFS_NUM		6
#define KDRV_IFE_NRS_GB_W_NUM		6
#define KDRV_IFE_NRS_GB_RANGE_NUM	5
#define KDRV_IFE_NRS_GB_TH_NUM		5

#define KDRV_IFE_FCURVE_Y_W_NUM		17
#define KDRV_IFE_FCURVE_IDX_NUM		32
#define KDRV_IFE_FCURVE_SPLIT_NUM	32
#define KDRV_IFE_FCURVE_VAL_NUM		65

#define KDRV_IFE_FUSION_MC_DIFF_W_NUM			16
#define KDRV_IFE_FUSION_SHORT_COMP_KNEE_NUM		3
#define KDRV_IFE_FUSION_SHORT_COMP_SUB_NUM		4
#define KDRV_IFE_FUSION_SHORT_COMP_SHIFT_NUM	4
#define KDRV_IFE_FUSION_BLD_CUR_KNEE_NUM		1
#define KDRV_IFE_FUSION_DARK_SAT_NUM			2

#define KDRV_IFE_OUTL_BRI_TH_NUM	5
#define KDRV_IFE_OUTL_DARK_TH_NUM	5
#define KDRV_IFE_OUTL_CNT_NUM		2
#define KDRV_IFE_OUTL_ORD_W_NUM		8

#define KDRV_IFE_SPATIAL_W_LEN		6

#define KDRV_IFE_RANGE_A_TH_NUM		6
#define KDRV_IFE_RANGE_B_TH_NUM		6
#define KDRV_IFE_RANGE_A_LUT_SIZE	17
#define KDRV_IFE_RANGE_B_LUT_SIZE	17

#define KDRV_IFE_VIG_CH0_LUT_SIZE	17
#define KDRV_IFE_VIG_CH1_LUT_SIZE	17
#define KDRV_IFE_VIG_CH2_LUT_SIZE	17
#define KDRV_IFE_VIG_CH3_LUT_SIZE	17

#define KDRV_IFE_GBAL_OFS_NUM		17

#define KDRV_IFE_RBFILL_LUMA_NUM	17
#define KDRV_IFE_RBFILL_RATIO_NUM	32

typedef enum {
	KDRV_IFE_PARAM_CFG_PROCESS_CPU = 0, // [set] cpu_mode job config.               data_dype: KDRV_IFE_JOB_CFG
	KDRV_IFE_PARAM_CFG_PROCESS_LL,      // [set] linklist_mode job config.          data_dype: KDRV_IFE_JOB_CFG
	KDRV_IFE_PARAM_TRIG_SINGLE,         // [set] trigger ife start.                 data_type: NULL
	KDRV_IFE_PARAM_TRIG_LL,             // [set] trigger ife start by linklist.     data_type: UINT32 *
	KDRV_IFE_PARAM_TRIG_DIR,			// [set] trigger ife start by linklist.     data_type: UINT32 *
	KDRV_IFE_PARAM_SET_CB,              // [set] set isr callback.                  data_type: KDRV_IPP_ISR_CB
	KDRV_IFE_PARAM_STOP_SINGLE,			// [set] stop ife.                          data_type: NULL
	KDRV_IFE_PARAM_HARD_RESET,			// [set] hard reset.                        data_type: NULL
	KDRV_IFE_PARAM_CLR_RING_BUF_LINE_CNT,//[set] clear ring buffer line counter.    data_type: NULL
	KDRV_IFE_PARAM_DMA_CH_ABORT,		// [set] set dma abort at anytime.          data_type: UINT32 *
	KDRV_IFE_PARAM_DMA_CH_STS,			// [get] get dma abort status				data_type: UINT32 *
	KDRV_IFE_PARAM_GET_REG_BASE_ADDR,	// [get] get reg base addr					data_type: UINT32 *
	KDRV_IFE_PARAM_GET_REG_BASE_ADDR_LL,// [get] get reg base addr ll mode			data_type: UINT32 *
	KDRV_IFE_PARAM_ID_MAX,
} KDRV_IFE_PARAM_ID;

typedef enum {
	KDRV_IFE_QUERY_REG_NUM = 0,         // [get] get reg num for memory alloc.      data_type: UINT32
	KDRV_IFE_QUERY_ID_MAX,
} KDRV_IFE_QUERY_ID;

typedef enum  {
	KDRV_IFE_INTERRUPT_FMD				= 0x00000001,  // interrupt enable/status: frame end
	KDRV_IFE_INTERRUPT_DEC_ERR			= 0x00000004,  // interrupt enable/status: rde decode error interrupt
	KDRV_IFE_INTERRUPT_LLEND			= 0x00000008,  // interrupt enable/status: LinkedList end interrupt
	KDRV_IFE_INTERRUPT_LLERR			= 0x00000010,  // interrupt enable/status: LinkedList error  interrupt (linklist format error)
	KDRV_IFE_INTERRUPT_LLERR2			= 0x00000020,  // interrupt enable/status: LinkedList error2 interrupt (direct mode only, SIE send frame start before KDRV_IFE_INTERRUPT_LLEND)
	KDRV_IFE_INTERRUPT_LLJOBEND			= 0x00000040,  // interrupt enable/status: LinkedList job end interrupt
	KDRV_IFE_INTERRUPT_BUFOVFL			= 0x00000080,  // interrupt enable/status: buffer overflow interrupt
	KDRV_IFE_INTERRUPT_RING_BUF_ERR		= 0x00000100,  // interrupt enable/status: ring buffer error interrupt
	KDRV_IFE_INTERRUPT_FRAME_ERR		= 0x00000200,  // interrupt enable/status: frame error interrupt (direct mode only)
	KDRV_IFE_INTERRUPT_SIE_FRAME_START	= 0x00001000,  // interrupt enable/status: SIE frame start interrupt (direct mode only)
	KDRV_IFE_INTERRUPT_IFE_FRAME_START	= 0x00004000,  // interrupt enable/status: IFE frame start interrupt (direct mode only)
	KDRV_IFE_INTERRUPT_ALL				= (KDRV_IFE_INTERRUPT_FMD | KDRV_IFE_INTERRUPT_DEC_ERR | KDRV_IFE_INTERRUPT_LLEND | KDRV_IFE_INTERRUPT_LLERR |
											KDRV_IFE_INTERRUPT_LLERR2 | KDRV_IFE_INTERRUPT_LLJOBEND | KDRV_IFE_INTERRUPT_BUFOVFL | KDRV_IFE_INTERRUPT_RING_BUF_ERR |
											KDRV_IFE_INTERRUPT_FRAME_ERR | KDRV_IFE_INTERRUPT_SIE_FRAME_START | KDRV_IFE_INTERRUPT_IFE_FRAME_START),  // all interrupt enable/status mask
} KDRV_IFE_INTERRUPT;

typedef enum {
	KDRV_IFE_UPDATE_NRS					= 0x00000001,
	KDRV_IFE_UPDATE_FCURVE				= 0x00000002,
	KDRV_IFE_UPDATE_FUSION				= 0x00000004,
	KDRV_IFE_UPDATE_OUTL				= 0x00000008,
	KDRV_IFE_UPDATE_FILT				= 0x00000010,
	KDRV_IFE_UPDATE_CGAIN				= 0x00000020,
	KDRV_IFE_UPDATE_VIG					= 0x00000040,
	KDRV_IFE_UPDATE_GBAL				= 0x00000080,
	KDRV_IFE_UPDATE_ALL					= (KDRV_IFE_UPDATE_NRS | KDRV_IFE_UPDATE_FCURVE | KDRV_IFE_UPDATE_FUSION | KDRV_IFE_UPDATE_OUTL | KDRV_IFE_UPDATE_FILT | KDRV_IFE_UPDATE_CGAIN |
											KDRV_IFE_UPDATE_VIG | KDRV_IFE_UPDATE_GBAL),
} KDRV_IFE_UPDATE;

typedef enum {
	KDRV_IFE_CGAIN_CH_R = 0,					// R  channel for RGGB & RGBIr
	KDRV_IFE_CGAIN_CH_GR,						// Gr channel for RGGB & RGBIr
	KDRV_IFE_CGAIN_CH_GB,						// Gb channel for RGGB & RGBIr
	KDRV_IFE_CGAIN_CH_B,						// B  channel for RGGB & RGBIr
	KDRV_IFE_CGAIN_CH_IR,						// Ir channel for        RGBIr
	KDRV_IFE_CGAIN_CH_MAX,
} KDRV_IFE_CGAIN_CH;

typedef enum {
	KDRV_IFE_VIG_CH_R = 0,						// R  channel for RGGB & RGBIr
	KDRV_IFE_VIG_CH_GR = 1,						// Gr channel for RGGB
	KDRV_IFE_VIG_CH_GB = 2,						// Gb channel for RGGB
	KDRV_IFE_VIG_CH_G = KDRV_IFE_VIG_CH_GR,		// G  channel for        RGBIr
	KDRV_IFE_VIG_CH_IR = KDRV_IFE_VIG_CH_GB,	// Ir channel for        RGBIr
	KDRV_IFE_VIG_CH_B = 3,						// B  channel for RGGB & RGBIr
	KDRV_IFE_VIG_CH_MAX = 4,
} KDRV_IFE_VIG_CH;

typedef enum {
	KDRV_IFE_F_CURVE_Y_8G4R4B = 0,
	KDRV_IFE_F_CURVE_Y_4G = 1,
	KDRV_IFE_F_CURVE_Y_ORI = 2,
} KDRV_IFE_F_CURVE_Y_SEL;

typedef enum {
	KDRV_IFE_Y_SEL_8G4R4B = 0,
	KDRV_IFE_Y_SEL_4G = 1,
	KDRV_IFE_Y_SEL_RGB_MAX = 2,
	KDRV_IFE_Y_SEL_ORI = 3,
} KDRV_IFE_Y_SEL;

typedef enum {
	KDRV_IFE_MODE_FUSION = 0,
	KDRV_IFE_MODE_SHORT_EXP = 1,
	KDRV_IFE_MODE_LONG_EXP = 2,
	KDRV_IFE_MODE_LONG_REAL = 3,
} KDRV_IFE_MODE_SEL;

typedef enum {
	KDRV_IFE_FUSION_BY_L_EXP = 0,
	KDRV_IFE_FUSION_BY_S_EXP = 1,
	KDRV_IFE_FUSION_BOTH_EXP = 2,
} KDRV_IFE_FUSION_NOR_SEL;

typedef enum {
	KDRV_IFE_2_8    = 0,    // 2bits decimal, 8bits fraction
	KDRV_IFE_3_7    = 1,    // 3bits decimal, 7bits fraction
} KDRV_IFE_GAIN_FIELD;

#if 0
#endif

/**************************************/
/*                                    */
/*    JOB CFG -  IO CFG structure     */
/*                                    */
/**************************************/

typedef struct {
	UINT8 en; 		// 0 : original, 1 : output image horizontal mirror
} KDRV_IFE_MIRROR;

typedef enum {
	KDRV_IFE_ENCODE_RATE_50     = 0,     // RDE encode rate 50
	KDRV_IFE_ENCODE_RATE_58     = 1,     // RDE encode rate 58
	KDRV_IFE_ENCODE_RATE_66     = 2,     // RDE encode rate 66
	KDRV_IFE_ENCODE_RATE_UNKNOWN,
} KDRV_IFE_ENCODE_RATE;

typedef struct {
	KDRV_IFE_ENCODE_RATE  encode_rate;  // RDE encode rate of bitstream per segment
} KDRV_IFE_RDE;

typedef struct {
	UINT8   en;             // 0 : used full buffer, 1: enable ring buffer (only direct mode with multi-frames support)
	UINT16  line_num;       // frame1 ring buffer line number
	UINT8   imm_start;		// 0: ring buffer line counter will start counting after receiving SIE2 sync signal
							// 1: ring buffer line counter will start counting immediately
} KDRV_IFE_RING_BUF;

typedef struct {
	IPOINT center[KDRV_IFE_VIG_CH_MAX]; // VIG center position. range : -8192~8191. please used KDRV_IFE_VIG_CH to assign KDRV_IFE_VIG_CH_MAX index
} KDRV_IFE_VIG_POS;

#if 0
#endif

/**************************************/
/*                                    */
/*    JOB CFG -  IQ CFG structure     */
/*                                    */
/**************************************/

/**
    IFE nrs Bilat func
*/
typedef struct {
	UINT8   en;									//  NRS Bilateral  enable
	UINT8   gb_en;								//  NRS G channel Bilateral enable
	UINT8   b_str;								//  NRS Bilateral strength. range 0~8.
	UINT8   gb_str;								//  NRS G channel Bilateral strength. range 0~8.

	UINT8   gb_blend_w;							//  NRS G weight with B. range 0~1.

	UINT8   b_ofs[KDRV_IFE_NRS_B_OFS_NUM];		//  NRS Bilateral offset. range 0~255.
	UINT8   b_weight[KDRV_IFE_NRS_B_W_NUM];		//  NRS Bilateral weight. range 0~31.
	UINT8   b_th[KDRV_IFE_NRS_B_TH_NUM];		//  NRS Bilateral threshold. range 0~3.
	UINT16  b_rng[KDRV_IFE_NRS_B_RANGE_NUM];	//  NRS Bilateral range. range 0~4095.

	UINT8   gb_ofs[KDRV_IFE_NRS_GB_OFS_NUM];	//  NRS G channel Bilateral offset. range 0~255.
	UINT8   gb_weight[KDRV_IFE_NRS_GB_W_NUM];	//  NRS G channel Bilateral weight. range 0~31.
	UINT8   gb_th[KDRV_IFE_NRS_GB_TH_NUM];		//  NRS G channel Bilateral threshold. range 0~3.
	UINT16  gb_rng[KDRV_IFE_NRS_GB_RANGE_NUM];	//  NRS G channel Bilateral range. range 0~4095.
} KDRV_IFE_BILAT_PARAM;

/**
    IFE nrs Order func
*/
typedef struct {
	UINT8   en;									// NRS Order enable
	UINT8   rng_bri;							// NRS Order bright range. range 0~7.
	UINT8   rng_dark;							// NRS Order dark range. range 0~7.
	UINT16  diff_th;							// NRS Order difference threshold. range 0~1023.
	UINT8   w_bri[KDRV_IFE_NRS_ORD_W_NUM];		// NRS Order bright weight. range 0~8.
	UINT8   w_dark[KDRV_IFE_NRS_ORD_W_NUM];		// NRS Order dark weight. range 0~8.
} KDRV_IFE_ORDER_PARAM;

/**
    IFE nrs func
*/
typedef struct {
	UINT8                   en;					//  Enable/Disable
	KDRV_IFE_ORDER_PARAM    order;
	KDRV_IFE_BILAT_PARAM    bilat;
} KDRV_IFE_NRS_PARAM;

typedef struct {
	UINT16  val_lut[KDRV_IFE_FCURVE_VAL_NUM];	// Fcurve Value. range 0~4095.
} KDRV_IFE_VAL_LUT;

typedef struct {
	UINT8   split_lut[KDRV_IFE_FCURVE_SPLIT_NUM];	// Fcurve Split. range 0~3.
} KDRV_IFE_SPLIT_LUT;

typedef struct {
	UINT8   idx_lut[KDRV_IFE_FCURVE_IDX_NUM];	// Fcurve Index. range 0~63.
} KDRV_IFE_IDX_LUT;

typedef struct {
	UINT8   y_w_lut[KDRV_IFE_FCURVE_Y_W_NUM];	// Fcurve Y weight. range 0~255.
} KDRV_IFE_Y_W;

typedef struct {
	KDRV_IFE_F_CURVE_Y_SEL  y_mean_sel; // Fcurve Y Mean Method selection.
	UINT8   yv_w;                       // Fcurve Y mean and V weight. range 0~8.
} KDRV_IFE_FCURVE_CTRL;

/**
    IFE fcurve func
*/
typedef struct {
	UINT8                en;		// Enable/Disable of f curve in IFE.
	KDRV_IFE_FCURVE_CTRL fcur_ctrl;	// Fcurve control Y Mean Method and V weight.
	KDRV_IFE_Y_W         y_weight;	// Fcurve Y weight
	KDRV_IFE_IDX_LUT     index;		// Fcurve Index
	KDRV_IFE_SPLIT_LUT   split;		// Fcurve Split
	KDRV_IFE_VAL_LUT     value;		// Fcurve Value
} KDRV_IFE_FCURVE_PARAM;

/**
    IFE fusion ctrl func
*/
typedef struct {
	KDRV_IFE_Y_SEL    y_mean_sel;	// Y mean option. range 0~3.
	KDRV_IFE_MODE_SEL mode;			// Fusion mode. range 0~3. (0: Fusion; 1: Short frame; 2: Long frame; 3: Long real)
	UINT8             ev_ratio;		// EV ratio. range 0~255. (4EV=16, 3EV = 32, 2EV=64, 1EV=128)
} KDRV_IFE_FUSION_CTRL;

/**
    IFE fusion blend curve
*/
typedef struct {
	KDRV_IFE_FUSION_NOR_SEL nor_sel;						// Normal blending curve weighting option.
	KDRV_IFE_FUSION_NOR_SEL dif_sel;						// Difference blending curve weighting option.

	/* normal blending - long exposure param. */
	UINT16  l_nor_knee[KDRV_IFE_FUSION_BLD_CUR_KNEE_NUM];	// Knee point of normal blending curve for long exposure. range 0~4095.
	UINT8   l_nor_range;									// Normal blending curve range for long exposure. Actually range = (1<<l_nor_range). range 0~12.
	UINT8   l_nor_w_edge;									// Weighting function direction. range 0~1.

	/* normal blending - short exposure param. */
	UINT16  s_nor_knee[KDRV_IFE_FUSION_BLD_CUR_KNEE_NUM];	// Knee point of normal blending curve for short exposure. range 0~4095.
	UINT8   s_nor_range;									// Normal blending curve range for short exposure. range 0~12.
	UINT8   s_nor_w_edge;									// Weighting function direction. range 0~1.

	/* difference blending - long exposure param. */
	UINT16  l_dif_knee[KDRV_IFE_FUSION_BLD_CUR_KNEE_NUM];	// Knee point of difference blending curve for long exposure. range 0~4095.
	UINT8   l_dif_range;									// Difference blending curve range for long exposure. range 0~12. Actually range = (1<<l_dif_range).
	UINT8   l_dif_w_edge;									// range 0~1.Weighting function direction.

	/* difference blending - short exposure param. */
	UINT16  s_dif_knee[KDRV_IFE_FUSION_BLD_CUR_KNEE_NUM];	// Knee point of difference blending curve for short exposure. range 0~4095.
	UINT8   s_dif_range;									// Difference blending curve range for short exposure. range 0~12. Actually range = (1<<l_dif_range).
	UINT8   s_dif_w_edge;									// Weighting function direction. range 0~1.
} KDRV_IFE_BLEND_CURVE;

/**
    IFE fusion mc
*/
typedef struct {
	UINT16 lum_th;										// Lower than this threshold using normal blending curve. range 0~4095.
	UINT8 diff_ratio;									// Control ratio. range 0~3.
	UINT8 pos_diff_w[KDRV_IFE_FUSION_MC_DIFF_W_NUM];	// Difference weighting for difference > 0. range 0~16.
	UINT8 neg_diff_w[KDRV_IFE_FUSION_MC_DIFF_W_NUM];	// Difference weighting for difference <= 0. range 0~16.
	UINT8 dwd;											// Using this ratio if difference less than lum_th. range 0~16.
} KDRV_IFE_MC;

/**
    IFE fusion dark sat reduce
*/
typedef struct {
	UINT16 th[KDRV_IFE_FUSION_DARK_SAT_NUM];		// Threshold of DS. range 0~4095.
	UINT8 step[KDRV_IFE_FUSION_DARK_SAT_NUM];		// Weighting step of DS. range 0~255.
	UINT8 low_bound[KDRV_IFE_FUSION_DARK_SAT_NUM];	// Lower bound of DS. range 0~255.
} KDRV_IFE_DARK_SAT;

/**
    IFE fusion s compression
*/
typedef struct {
	UINT8 en;												// S Compression switch. range 0~1.
	UINT16 knee[KDRV_IFE_FUSION_SHORT_COMP_KNEE_NUM];		// Knee point of S-compression. range 0~4095.
	UINT16 sub_point[KDRV_IFE_FUSION_SHORT_COMP_SUB_NUM];	// Start point of S-compression. range 0~4095.
	UINT8 shift[KDRV_IFE_FUSION_SHORT_COMP_SHIFT_NUM];		// Shift bit of   S-compression. range 0~7.
} KDRV_IFE_S_COMP;

/**
    IFE fusion color gain
*/
typedef struct {
	UINT8                en;					// Enable/Disable.
	KDRV_IFE_GAIN_FIELD  bit_field;				// select Fusion color gain decimal range. range 0~1.

	/* short exposure param. */
	UINT16  fcgain_s[KDRV_IFE_CGAIN_CH_MAX];	// SHDR short exposure path color gain. range 0~1023. please used KDRV_IFE_CGAIN_CH to assign KDRV_IFE_CGAIN_CH_MAX index
	UINT16  fcofs_s[KDRV_IFE_CGAIN_CH_MAX];		// SHDR short exposure path color offset. range 0~1023. please used KDRV_IFE_CGAIN_CH to assign KDRV_IFE_CGAIN_CH_MAX index

	/* long exposure param. */
	UINT16  fcgain_l[KDRV_IFE_CGAIN_CH_MAX];	// SHDR long exposure path color gain. range 0~1023. please used KDRV_IFE_CGAIN_CH to assign KDRV_IFE_CGAIN_CH_MAX index
	UINT16  fcofs_l[KDRV_IFE_CGAIN_CH_MAX];		// SHDR long exposure path color offset. range 0~1023. please used KDRV_IFE_CGAIN_CH to assign KDRV_IFE_CGAIN_CH_MAX index
} KDRV_IFE_FCGAIN;

/**
    IFE fusion func
*/
typedef struct {
	KDRV_IFE_FUSION_CTRL   fu_ctrl;
	KDRV_IFE_BLEND_CURVE   bld_cur;
	KDRV_IFE_MC            mc_para;
	KDRV_IFE_DARK_SAT      dk_sat;
	KDRV_IFE_S_COMP        s_comp;
	KDRV_IFE_FCGAIN        fu_cgain;
} KDRV_IFE_FUSION_PARAM;

/**
    IFE outlier func
*/
typedef struct {
	UINT8   en;										// Enable/Disable
	UINT16  bright_th[KDRV_IFE_OUTL_BRI_TH_NUM];	// range: 0~4095
	                                                // outlier bright case threshold
	UINT16  dark_th [KDRV_IFE_OUTL_DARK_TH_NUM];	// range: 0~4095
	                                                // outlier dark case threshold
	UINT8   outl_cnt[KDRV_IFE_OUTL_CNT_NUM];		// outlier neighbor point counter. range: 0~16.
	UINT8   outl_weight;							// outlier weighting. range: 0~255.
	UINT16  dark_ofs;								// outlier dark th offset. range: 0~4095.
	UINT16  bright_ofs;								// outlier bright th offset. range: 0~4095.

	UINT8   avg_mode;								// outlier use direction average or not. range: 0~1.

	UINT8   ord_rng_bri;							// control the strength of order method for bright defect pixel. range: 0~7.
	UINT8   ord_rng_dark;							// control the strength of order method for dark defect pixel. range: 0~7.
	UINT16  ord_protect_th;							// order method protect artifact th. range: 0~1023.
	UINT8   ord_blend_w;							// outlier and order result blending weight. range: 0~255.
	UINT8   ord_bri_w[KDRV_IFE_OUTL_ORD_W_NUM];		// order bright defect pixel compensate weight. range: 0~8.
	UINT8   ord_dark_w[KDRV_IFE_OUTL_ORD_W_NUM];	// order dark defect pixel compensate weight. range: 0~8.
} KDRV_IFE_OUTL_PARAM;

typedef struct {
	UINT8  weight[KDRV_IFE_SPATIAL_W_LEN];		// filter spatial weight. range:0~31.
} KDRV_IFE_SPATIAL;

typedef struct {
	UINT16  a_th [KDRV_IFE_RANGE_A_TH_NUM];		// range:0~1023, range filter A threshold
	UINT16  a_lut[KDRV_IFE_RANGE_A_LUT_SIZE];	// range:0~1023, range filter A threshold adjustment
	UINT16  b_th [KDRV_IFE_RANGE_B_TH_NUM];		// range:0~1023, range filter B threshold
	UINT16  b_lut[KDRV_IFE_RANGE_B_LUT_SIZE];	// range:0~1023, range filter B threshold adjustment
} KDRV_IFE_RANGE_FILTER;

#define KDRV_IFE_RANGE_FILTER_R		KDRV_IFE_RANGE_FILTER
#define KDRV_IFE_RANGE_FILTER_GR	KDRV_IFE_RANGE_FILTER
#define KDRV_IFE_RANGE_FILTER_GB	KDRV_IFE_RANGE_FILTER
#define KDRV_IFE_RANGE_FILTER_B		KDRV_IFE_RANGE_FILTER
#define KDRV_IFE_RANGE_FILTER_G		KDRV_IFE_RANGE_FILTER
#define KDRV_IFE_RANGE_FILTER_IR	KDRV_IFE_RANGE_FILTER

typedef struct {
	UINT16  th;		// threshold. range:0~4095.
	UINT8   mul;	// weighting multiplier. range:0~255.
	UINT16  dlt;	// threshold adjustment. range:0~4095.
} KDRV_IFE_CLAMP;

typedef enum {
	KDRV_IFE_BILAT_CEN_DPC = 0,		// 52x algorithm
	KDRV_IFE_BILAT_CEN_BILATERAL,	// bypass center pixel
} KDRV_IFE_BILAT_CEN_SEL;

typedef struct {
	UINT8   en;						// 3x3 center mofify bilateral filter threshold enable
	UINT16  th1;					// 3x3 center mofify bilateral filter threshold 1. range:0~1023.
	UINT16  th2;					// 3x3 center mofify bilateral filter threshold 2. range:0~1023.
	KDRV_IFE_BILAT_CEN_SEL cen_sel;	// 3x3 center mofify bilateral filter algorithm
} KDRV_IFE_CENTER_MODIFY;

typedef struct {
	UINT8   en;									// Enable/Disable
	UINT8   luma [KDRV_IFE_RBFILL_LUMA_NUM];	// range: 0~31.
	UINT8   ratio[KDRV_IFE_RBFILL_RATIO_NUM];	// range: 0~31.
	UINT8   ratio_mode;							// range: 0~2 , n3.2, n2.3, n1.4
} KDRV_IFE_RBFILL_PARAM;

/**
    IFE bilateral filter
*/
typedef struct {
	UINT8                       en;				// Enable/Disable
	KDRV_IFE_SPATIAL            spatial;		// filter spatial weight table

	union {
		KDRV_IFE_RANGE_FILTER_R r;				// range filter for KDRV_IFE_RGGB or KDRV_IFE_RGBIR ch0
	} rng_filt_ch0;

	union {
		KDRV_IFE_RANGE_FILTER_GR    gr;			// range filter for KDRV_IFE_RGGB  ch1
		KDRV_IFE_RANGE_FILTER_G     g;			// range filter for KDRV_IFE_RGBIR ch1
	} rng_filt_ch1;

	struct {									// use structure to store gb & ir separately for current iq api
		KDRV_IFE_RANGE_FILTER_GB    gb;			// range filter for KDRV_IFE_RGGB ch2
		KDRV_IFE_RANGE_FILTER_IR    ir;			// range filter for KDRV_IFE_RGBIR ch2
	} rng_filt_ch2;

	union {
		KDRV_IFE_RANGE_FILTER_B     b;			// range filter for KDRV_IFE_RGGB or KDRV_IFE_RGBIR ch3
	} rng_filt_ch3;

	KDRV_IFE_CENTER_MODIFY      center_mod;		// center modify
	KDRV_IFE_CLAMP              clamp;			// clamp and weighting setting
	KDRV_IFE_RBFILL_PARAM       rbfill;			// RGBIr fill r/b pixel
	UINT8                       blend_w;		// range filter A and B weighting. range:0~255.
	UINT8                       rng_th_w;		// range th weight. range:0~15.
	UINT8                       bin;			// range:0~7.( Denominator: 2<<bin )
} KDRV_IFE_FILTER_PARAM;

/**
    IFE color gain
*/
typedef struct {
	UINT8                   en;								// Enable/Disable. range:0~1.
	UINT8                   inv;							// color gain invert. range:0~1.
	UINT8                   hinv;							// color gain H-invert. range:0~1.
	KDRV_IFE_GAIN_FIELD     bit_field;						// color gain bit field. range:0~1.
	UINT16                  mask;							// color gain mask. range:0~4095

	UINT16                  cgain[KDRV_IFE_CGAIN_CH_MAX];	// color gain. range 0~1023. please used KDRV_IFE_CGAIN_CH to assign KDRV_IFE_CGAIN_CH_MAX index
	UINT16                  cofs[KDRV_IFE_CGAIN_CH_MAX];	// color offset. range 0~1023. please used KDRV_IFE_CGAIN_CH to assign KDRV_IFE_CGAIN_CH_MAX index
} KDRV_IFE_CGAIN_PARAM;

/**
    ife vignette shading compensation
*/
typedef struct {
	UINT8   en;										// Enable/Disable
	UINT16  dist_th;								// disabled area distance threshold. range: 0~1023

	union {
		UINT16  r[KDRV_IFE_VIG_CH0_LUT_SIZE];		// VIG lookup table for KDRV_IFE_RGGB or KDRV_IFE_RGBIR ch0
	} lut_ch0;

	union {
		UINT16  gr[KDRV_IFE_VIG_CH1_LUT_SIZE];		// VIG lookup table for KDRV_IFE_RGGB  ch1
		UINT16  g[KDRV_IFE_VIG_CH1_LUT_SIZE];		// VIG lookup table for KDRV_IFE_RGBIR ch1
	} lut_ch1;

	struct {										// use structure to store gb & ir separately for current iq api
		UINT16  gb[KDRV_IFE_VIG_CH2_LUT_SIZE];		// VIG lookup table for KDRV_IFE_RGGB  ch2
		UINT16  ir[KDRV_IFE_VIG_CH2_LUT_SIZE];		// VIG lookup table for KDRV_IFE_RGBIR ch2
	} lut_ch2;

	union {
		UINT16  b[KDRV_IFE_VIG_CH3_LUT_SIZE];		// VIG lookup table for KDRV_IFE_RGGB or KDRV_IFE_RGBIR ch3
	} lut_ch3;

	UINT8   dither_enable;							// dithering enable/disable
	UINT8   dither_rst_enable;						// dithering reset enable
} KDRV_IFE_VIG_PARAM;

/**
    IFE structure - IFE gbalance parameter set.
*/
typedef struct {
	UINT8   en;						// enable/disable. range: 0~1.
	UINT8   protect_enable;			// enable/disable preserve edge or feature. range: 0~1.
	UINT16  diff_th_str;			// gbal strength. range: 0~1023.
	UINT8   diff_w_max;				// difference weight max. range: 0~15.
	UINT16  edge_protect_th1;		// edge judgement th 1. range: 0~1023.
	UINT16  edge_protect_th0;		// edge judgement th 0. range: 0~1023.
	UINT8   edge_w_max;				// edge weight max. range: 0~255.
	UINT8   edge_w_min;				// edge weight min. range: 0~255.
	UINT8   gbal_ofs[KDRV_IFE_GBAL_OFS_NUM];// weight transision region. range: 0~63.
} KDRV_IFE_GBAL_PARAM;

typedef struct {
	KDRV_IPP_OPMODE mode;
	KDRV_IPP_FRAME in_frm;		// KDRV_IPP_PLANE ([0]:frame0, [1]:frame1,only multi-frames valid)
	KDRV_IPP_FRAME out_frm;		// KDRV_IPP_PLANE ([0]:frame0,only frame0 valid)
	URECT crp_window;
	UINT32 inte_en;				// data type: KDRV_IFE_INTERRUPT

	KDRV_IFE_MIRROR mirror;		// mirror information.
	KDRV_IFE_RDE rde;			// raw decode information.
	KDRV_IFE_RING_BUF ring_buf;	// ring buffer information.
	KDRV_IFE_VIG_POS vig_pos;	// vig center position.
} KDRV_IFE_IO_CFG;

typedef struct {
	KDRV_IFE_NRS_PARAM     nrs;
	KDRV_IFE_FCURVE_PARAM  fcurve;
	KDRV_IFE_FUSION_PARAM  fusion;
	KDRV_IFE_OUTL_PARAM    outl;
	KDRV_IFE_FILTER_PARAM  filt;
	KDRV_IFE_CGAIN_PARAM   cgain;
	KDRV_IFE_VIG_PARAM     vig;
	KDRV_IFE_GBAL_PARAM    gbal;
	KDRV_IFE_UPDATE update;
} KDRV_IFE_IQ_CFG;

/*
    KDRV IFE JOB CFG
*/
typedef struct {
	KDRV_IFE_IO_CFG *p_iocfg;	// [set][get] IFE engine in/out configuration
	KDRV_IFE_IQ_CFG *p_iqcfg;	// [set][get] IFE engine IQ configuration
	void *p_ll_blk;				// [set] ipp queue information
} KDRV_IFE_JOB_CFG;

INT32 kdrv_ife_module_init(void);
INT32 kdrv_ife_module_uninit(void);

INT32 kdrv_ife_open(UINT32 chip, UINT32 engine);
INT32 kdrv_ife_close(UINT32 chip, UINT32 engine);
INT32 kdrv_ife_query(UINT32 id, KDRV_IFE_QUERY_ID qid, void *p_param);
INT32 kdrv_ife_set(UINT32 id, KDRV_IFE_PARAM_ID param_id, void *p_param);
INT32 kdrv_ife_get(UINT32 id, KDRV_IFE_PARAM_ID param_id, void *p_param);

INT32 kdrv_ife_rtos_init(void);
INT32 kdrv_ife_rtos_uninit(void);

void kdrv_ife_dump(void);
void kdrv_ife_dump_cfg(KDRV_IFE_JOB_CFG *p_cfg);
void kdrv_ife_dump_register(void);
UINT32 kdrv_ife_dbg_mode(void *p_hdl, UINT32 param_id, void *data);

#endif
