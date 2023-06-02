/**
    IPL Ctrl Layer, ISP Interface

    @file       ctl_ipp_isp.h
    @ingroup    mIIPL_Ctrl
    @note       None

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/


#ifndef _CTL_IFE_ISP_H
#define _CTL_IFE_ISP_H

#define CTL_IFE_ISP_NRS_ORD_W_NUM					8
#define CTL_IFE_ISP_NRS_B_OFS_NUM					6
#define CTL_IFE_ISP_NRS_B_W_NUM						6
#define CTL_IFE_ISP_NRS_B_RANGE_NUM					5
#define CTL_IFE_ISP_NRS_B_TH_NUM					5
#define CTL_IFE_ISP_NRS_GB_OFS_NUM					6
#define CTL_IFE_ISP_NRS_GB_W_NUM					6
#define CTL_IFE_ISP_NRS_GB_RANGE_NUM				5
#define CTL_IFE_ISP_NRS_GB_TH_NUM					5
#define CTL_IFE_ISP_FCURVE_Y_W_NUM					17
#define CTL_IFE_ISP_FCURVE_IDX_NUM					32
#define CTL_IFE_ISP_FCURVE_SPLIT_NUM				32
#define CTL_IFE_ISP_FCURVE_VAL_NUM					65
#define CTL_IFE_ISP_FUSION_MC_DIFF_W_NUM			16
#define CTL_IFE_ISP_FUSION_SHORT_COMP_KNEE_NUM		3
#define CTL_IFE_ISP_FUSION_SHORT_COMP_SUB_NUM		4
#define CTL_IFE_ISP_FUSION_SHORT_COMP_SHIFT_NUM		4
#define CTL_IFE_ISP_FUSION_BLD_CUR_KNEE_NUM			1
#define CTL_IFE_ISP_FUSION_DARK_SAT_NUM				2
#define CTL_IFE_ISP_OUTL_BRI_TH_NUM					5
#define CTL_IFE_ISP_OUTL_DARK_TH_NUM				5
#define CTL_IFE_ISP_OUTL_CNT_NUM					2
#define CTL_IFE_ISP_OUTL_ORD_W_NUM					8
#define CTL_IFE_ISP_SPATIAL_W_LEN					6
#define CTL_IFE_ISP_RANGE_A_TH_NUM					6
#define CTL_IFE_ISP_RANGE_B_TH_NUM					6
#define CTL_IFE_ISP_RANGE_A_LUT_SIZE				17
#define CTL_IFE_ISP_RANGE_B_LUT_SIZE				17
#define CTL_IFE_ISP_VIG_CH0_LUT_SIZE				17
#define CTL_IFE_ISP_VIG_CH1_LUT_SIZE				17
#define CTL_IFE_ISP_VIG_CH2_LUT_SIZE				17
#define CTL_IFE_ISP_VIG_CH3_LUT_SIZE				17
#define CTL_IFE_ISP_GBAL_OFS_NUM					17
#define CTL_IFE_ISP_RBFILL_LUMA_NUM					17
#define CTL_IFE_ISP_RBFILL_RATIO_NUM				32

/**
    range of color gain coefficients
*/
typedef enum {
	CTL_IFE_ISP__2_8    = 0,    ///< 2bits decimal, 8bits fraction
	CTL_IFE_ISP__3_7    = 1,    ///< 3bits decimal, 7bits fraction
	ENUM_DUMMY4WORD(CTL_IFE_ISP_GAIN_FIELD)
} CTL_IFE_ISP_GAIN_FIELD;

/**
    IFE nrs Order func
*/
typedef struct {
	BOOL            enable;									///<                NRS Order enable
	UINT8           rng_bri;								///<  Range 0~7   . NRS Order bright range.
	UINT8           rng_dark;								///<  Range 0~7   . NRS Order dark range.
	UINT16          diff_th;								///<  Range 0~1023. NRS Order difference threshold.
	UINT8           bri_w[CTL_IFE_ISP_NRS_ORD_W_NUM];		///<  Range 0~8   . NRS Order bright weight.
	UINT8           dark_w[CTL_IFE_ISP_NRS_ORD_W_NUM];		///<  Range 0~8   . NRS Order dark weight.
} CTL_IFE_ISP_ORDER_PARAM;

/**
    IFE nrs Bilat func
*/
typedef struct {
	BOOL            b_enable;								///<                NRS Bilateral  enable
	BOOL            gb_enable;								///<                NRS G channel Bilateral enable
	UINT8           b_str;									///<  Range 0~8   . NRS Bilateral strength.
	UINT8           gb_str;									///<  Range 0~8   . NRS G channel Bilateral strength.

	UINT8           gb_blend_w;								///<  Range 0~15  . NRS G weight with B.

	UINT8           b_ofs[CTL_IFE_ISP_NRS_B_OFS_NUM];		///<  Range 0~255 . NRS Bilateral offset.
	UINT8           b_weight[CTL_IFE_ISP_NRS_B_W_NUM];		///<  Range 0~31  . NRS Bilateral weight.
	UINT8           b_th[CTL_IFE_ISP_NRS_B_TH_NUM];			///<  Range 0~3   . NRS Bilateral threshold.
	UINT16          b_rng[CTL_IFE_ISP_NRS_B_RANGE_NUM];		///<  Range 0~4095. NRS Bilateral range.

	UINT8           gb_ofs[CTL_IFE_ISP_NRS_GB_OFS_NUM];		///<  Range 0~255 . NRS G channel Bilateral offset.
	UINT8           gb_weight[CTL_IFE_ISP_NRS_GB_W_NUM];	///<  Range 0~31  . NRS G channel Bilateral weight.
	UINT8           gb_th[CTL_IFE_ISP_NRS_GB_TH_NUM];		///<  Range 0~3   . NRS G channel Bilateral threshold.
	UINT16          gb_rng[CTL_IFE_ISP_NRS_GB_RANGE_NUM];	///<  Range 0~4095. NRS G channel Bilateral range.
} CTL_IFE_ISP_BILAT_PARAM;

/**
    IFE nrs func
*/
typedef struct {
	BOOL                      enable;						///<  Enable/Disable
	CTL_IFE_ISP_ORDER_PARAM   order;
	CTL_IFE_ISP_BILAT_PARAM   bilat;
} CTL_IFE_ISP_NRS;

typedef enum {
	CTL_IFE_ISP_F_CURVE_Y_8G4R4B = 0,
	CTL_IFE_ISP_F_CURVE_Y_4G = 1,
	CTL_IFE_ISP_F_CURVE_Y_ORI = 2,
	ENUM_DUMMY4WORD(CTL_IFE_ISP_F_CURVE_Y_SEL)
} CTL_IFE_ISP_F_CURVE_Y_SEL;

typedef struct {
	CTL_IFE_ISP_F_CURVE_Y_SEL  y_mean_sel;						///< Rnage 0~2. Fcurve Y Mean Method selection.
	UINT8     yv_w;												///< Range 0~8. Fcurve Y mean and V weight.
} CTL_IFE_ISP_FCURVE_CTRL;

typedef struct {
	UINT8            y_w_lut[CTL_IFE_ISP_FCURVE_Y_W_NUM];		///<  Range 0~255 . Fcurve Y weight.
} CTL_IFE_ISP_Y_W;

typedef struct {
	UINT8            idx_lut[CTL_IFE_ISP_FCURVE_IDX_NUM];		///<  Range 0~63  . Fcurve Index.
} CTL_IFE_ISP_IDX_LUT;

typedef struct {
	UINT8            split_lut[CTL_IFE_ISP_FCURVE_SPLIT_NUM];	///<  Range 0~3   . Fcurve Split.
} CTL_IFE_ISP_SPLIT_LUT;

typedef struct {
	UINT16           val_lut[CTL_IFE_ISP_FCURVE_VAL_NUM];		///<  Range 0~4095. Fcurve Value.
} CTL_IFE_ISP_VAL_LUT;

/**
    IFE fcurve func
*/
typedef struct {
	BOOL                    enable;		///<  Enable/Disable of f curve in IFE.
	CTL_IFE_ISP_FCURVE_CTRL fcur_ctrl;	///<  Fcurve control Y Mean Method and V weight.
	CTL_IFE_ISP_Y_W         y_weight;	///<  Fcurve Y weight
	CTL_IFE_ISP_IDX_LUT     index;		///<  Fcurve Index
	CTL_IFE_ISP_SPLIT_LUT   split;		///<  Fcurve Split
	CTL_IFE_ISP_VAL_LUT     value;		///<  Fcurve Value
} CTL_IFE_ISP_FCURVE;

typedef enum {
	CTL_IFE_ISP_Y_SEL_8G4R4B = 0,
	CTL_IFE_ISP_Y_SEL_4G = 1,
	CTL_IFE_ISP_Y_SEL_RGB_MAX = 2,
	CTL_IFE_ISP_Y_SEL_ORI = 3,
	ENUM_DUMMY4WORD(CTL_IFE_ISP_Y_SEL)
} CTL_IFE_ISP_Y_SEL;

typedef enum {
	CTL_IFE_ISP_MODE_FUSION = 0,
	CTL_IFE_ISP_MODE_SHORT_EXP = 1,
	CTL_IFE_ISP_MODE_LONG_EXP = 2,
	CTL_IFE_ISP_MODE_LONG_REAL = 3,
	ENUM_DUMMY4WORD(CTL_IFE_ISP_MODE_SEL)
} CTL_IFE_ISP_MODE_SEL;

/**
    IFE fusion ctrl func
*/
typedef struct {
	CTL_IFE_ISP_Y_SEL    y_mean_sel;	///< range 0~3  . Y mean option.
	CTL_IFE_ISP_MODE_SEL mode;			///< range 0~3  . Fusion mode.  0: Fusion; 1: Short frame; 2: Long frame; 3: Long real;
	UINT8                ev_ratio;		///< range 0~255. 4EV=16, 3EV = 32, 2EV=64, 1EV=128;
} CTL_IFE_ISP_FUSION_CTRL;

typedef enum {
	CTL_IFE_ISP_FUSION_BY_L_EXP = 0,
	CTL_IFE_ISP_FUSION_BY_S_EXP = 1,
	CTL_IFE_ISP_FUSION_BOTH_EXP = 2,
	ENUM_DUMMY4WORD(CTL_IFE_ISP_FUSION_NOR_SEL)
} CTL_IFE_ISP_FUSION_NOR_SEL;

/**
    IFE fusion blend curve
*/
typedef struct {
	CTL_IFE_ISP_FUSION_NOR_SEL nor_sel;									///< Range 0~2. Normal blending curve weighting option.
	CTL_IFE_ISP_FUSION_NOR_SEL dif_sel;									///< Range 0~2. Normal blending curve weighting option.
	UINT16           l_nor_knee[CTL_IFE_ISP_FUSION_BLD_CUR_KNEE_NUM];	///< range 0~4095.  Knee point of normal blending curve for long exposure.
	UINT16           l_nor_range;										///< range 0~12. Normal blending curve range for long  exposure. Actually range = (1<<l_nor_range)
	BOOL             l_nor_w_edge;										///< range 0~1. Weighting function direction.
	UINT16           s_nor_knee[CTL_IFE_ISP_FUSION_BLD_CUR_KNEE_NUM];	///< range 0~4095.  Knee point of normal blending curve for short exposure.
	UINT16           s_nor_range;										///< range 0~12. Normal blending curve range for short  exposure.
	BOOL             s_nor_w_edge;										///< range 0~1. Weighting function direction.

	UINT16           l_dif_knee[CTL_IFE_ISP_FUSION_BLD_CUR_KNEE_NUM];	///< range 0~4095.  Knee point of difference blending curve for long exposure.
	UINT16           l_dif_range;										///< range 0~12. Difference blending curve range for long  exposure. Actually range = (1<<l_dif_range)
	BOOL             l_dif_w_edge;										///< range 0~1. Weighting function direction.

	UINT16           s_dif_knee[CTL_IFE_ISP_FUSION_BLD_CUR_KNEE_NUM];	///< range 0~4095.  Knee point of difference blending curve for long exposure.
	UINT16           s_dif_range;										///< range 0~12. Difference blending curve range for short  exposure. Actually range = (1<<l_dif_range)
	BOOL             s_dif_w_edge;										///< range 0~1. Weighting function direction.

} CTL_IFE_ISP_BLEND_CURVE;

/**
    IFE fusion mc
*/
typedef struct {
	UINT16           lum_th;											///< Range 0~4095. Lower than this threshold using normal blending curve.
	UINT8            diff_ratio;										///< Range 0~3   . Control ratio.
	UINT8            pos_diff_w[CTL_IFE_ISP_FUSION_MC_DIFF_W_NUM];		///< Range 0~16. Difference weighting for difference > 0.
	UINT8            neg_diff_w[CTL_IFE_ISP_FUSION_MC_DIFF_W_NUM];		///< Range 0~16. Difference weighting for difference <= 0.
	UINT8            dwd;												///< Range 0~16. Using this ratio if difference less than lum_th.
} CTL_IFE_ISP_MC;

/**
    IFE fusion dark sat reduce
*/
typedef struct {
	UINT16           th[CTL_IFE_ISP_FUSION_DARK_SAT_NUM];				///< Range 0~4095. Threshold of DS.
	UINT8            step[CTL_IFE_ISP_FUSION_DARK_SAT_NUM];				///< Range 0~255 . Weighting step of DS.
	UINT8            low_bound[CTL_IFE_ISP_FUSION_DARK_SAT_NUM];		///< range 0~255 . Lower bound of DS.
} CTL_IFE_ISP_DARK_SAT;

/**
    IFE fusion s compression
*/
typedef struct {
	BOOL             enable;											///< Range 0~1   . S Compression switch.
	UINT16           knee[CTL_IFE_ISP_FUSION_SHORT_COMP_KNEE_NUM];		///< Rnage 0~4095. Knee point of S-compression.
	UINT16           sub_point[CTL_IFE_ISP_FUSION_SHORT_COMP_SUB_NUM];	///< Range 0~4095. Start point of S-compression.
	UINT8            shift[CTL_IFE_ISP_FUSION_SHORT_COMP_SHIFT_NUM];	///< Range 0~7   . Shift bit of   S-compression.
} CTL_IFE_ISP_S_COMP;

/**
    IFE fusion color gain
*/
typedef struct {
	BOOL                    enable;					///< Enable/Disable.
	CTL_IFE_ISP_GAIN_FIELD  bit_field;				///< Range 0~1. select Fusion color gain decimal range.

	UINT32                  fcgain_path0_r;			///< Range 0~1023. SHDR short exposure path R channel color gain.
	UINT32                  fcgain_path0_gr;		///< Range 0~1023. SHDR short exposure path Gr channel color gain.
	UINT32                  fcgain_path0_gb;		///< Range 0~1023. SHDR short exposure path Gb channel color gain.
	UINT32                  fcgain_path0_b;			///< Range 0~1023. SHDR short exposure path B channel color gain.
	UINT32                  fcgain_path0_ir;		///< Range 0~1023. SHDR short exposure path IR channel color gain.

	UINT32                  fcofs_path0_r;			///< Range 0~1023. SHDR short exposure path R channel color offset.
	UINT32                  fcofs_path0_gr;			///< Range 0~1023. SHDR short exposure path Gr channel color offset.
	UINT32                  fcofs_path0_gb;			///< Range 0~1023. SHDR short exposure path Gb channel color offset.
	UINT32                  fcofs_path0_b;			///< Range 0~1023. SHDR short exposure path B channel color offset.
	UINT32                  fcofs_path0_ir;			///< Range 0~1023. SHDR short exposure path IR channel color offset.

	UINT32                  fcgain_path1_r;			///< Range 0~1023. SHDR long exposure path R channel color gain.
	UINT32                  fcgain_path1_gr;		///< Range 0~1023. SHDR long exposure path Gr channel color gain.
	UINT32                  fcgain_path1_gb;		///< Range 0~1023. SHDR long exposure path Gb channel color gain.
	UINT32                  fcgain_path1_b;			///< Range 0~1023. SHDR long exposure path B channel color gain.
	UINT32                  fcgain_path1_ir;		///< Range 0~1023. SHDR long exposure path IR channel color gain.

	UINT32                  fcofs_path1_r;			///< Range 0~1023. SHDR long exposure path R channel color offset.
	UINT32                  fcofs_path1_gr;			///< Range 0~1023. SHDR long exposure path Gr channel color offset.
	UINT32                  fcofs_path1_gb;			///< Range 0~1023. SHDR long exposure path Gb channel color offset.
	UINT32                  fcofs_path1_b;			///< Range 0~1023. SHDR long exposure path B channel color offset.
	UINT32                  fcofs_path1_ir;			///< Range 0~1023. SHDR long exposure path IR channel color offset.
} CTL_IFE_ISP_FCGAIN;

/**
    IFE fusion func
*/
typedef struct {
	CTL_IFE_ISP_FUSION_CTRL   fu_ctrl;
	CTL_IFE_ISP_BLEND_CURVE   bld_cur;
	CTL_IFE_ISP_MC            mc_para;
	CTL_IFE_ISP_DARK_SAT      dk_sat;
	CTL_IFE_ISP_S_COMP        s_comp;
	CTL_IFE_ISP_FCGAIN        fu_cgain;
} CTL_IFE_ISP_FUSION;

/**
    IFE outlier func
*/
typedef struct {
	BOOL    enable;											///< Enable/Disable
	UINT32  bright_th[CTL_IFE_ISP_OUTL_BRI_TH_NUM];			// range: 0~4095
                                                            // outlier bright case threshold
	UINT32  dark_th [CTL_IFE_ISP_OUTL_DARK_TH_NUM];			// range: 0~4095
                                                            // outlier dark case threshold
	UINT32  outl_cnt[CTL_IFE_ISP_OUTL_CNT_NUM];				///< range: 0~16  , UINT32[2], outlier neighbor point counter
	UINT32  outl_weight;									///< range: 0~255 , outlier weighting
	UINT32  dark_ofs;										///< range: 0~4095, outlier dark th offset
	UINT32  bright_ofs;										///< range: 0~4095, outlier bright th offset

	BOOL    avg_mode;										///< range: 0~1   , outlier use direction average or not

	UINT8   ord_rng_bri;									///< range: 0~7   , control the strength of order method for bright defect pixel.
	UINT8   ord_rng_dark;									///< range: 0~7   , control the strength of order method for dark defect pixel.
	UINT16  ord_protect_th;									///< range: 0~1023, order method protect artifact th
	UINT8   ord_blend_w;									///< range: 0~255 , outlier and order result blending weight
	UINT8   ord_bri_w[CTL_IFE_ISP_OUTL_ORD_W_NUM];			///< range: 0~8   , order bright defect pixel compensate weight
	UINT8   ord_dark_w[CTL_IFE_ISP_OUTL_ORD_W_NUM];			///< range: 0~8   , order dark defect pixel compensate weight
} CTL_IFE_ISP_OUTL;

/**
    range filter
*/

typedef enum {
	CTL_IFE_ISP_BILAT_CEN_DPC = 0,		// 52x algorithm
	CTL_IFE_ISP_BILAT_CEN_BILATERAL,	// bypass center pixel
} CTL_IFE_ISP_BILAT_CEN_SEL;

typedef struct {
	BOOL    enable;						///<               3x3 center mofify bilateral filter threshold enable
	UINT32  th1;						///< range:0~1023, 3x3 center mofify bilateral filter threshold 1
	UINT32  th2;						///< range:0~1023, 3x3 center mofify bilateral filter threshold 2
	CTL_IFE_ISP_BILAT_CEN_SEL cen_sel;	///<               3x3 center mofify bilateral filter threshold algorithm
} CTL_IFE_ISP_CENTER_MODIFY;

typedef struct {
	UINT32  a_th [CTL_IFE_ISP_RANGE_A_TH_NUM];		///< range:0~1023, UINT32[6] , range filter A threshold
	UINT32  a_lut[CTL_IFE_ISP_RANGE_A_LUT_SIZE];	///< range:0~1023, UINT32[17], range filter A threshold adjustment
	UINT32  b_th [CTL_IFE_ISP_RANGE_B_TH_NUM];		///< range:0~1023, UINT32[6] , range filter B threshold
	UINT32  b_lut[CTL_IFE_ISP_RANGE_B_LUT_SIZE];	///< range:0~1023, UINT32[17], range filter B threshold adjustment
} CTL_IFE_ISP_RANGE_FILTER_R;

typedef struct {
	UINT32  a_th [CTL_IFE_ISP_RANGE_A_TH_NUM];		///< range:0~1023, UINT32[6] , range filter A threshold
	UINT32  a_lut[CTL_IFE_ISP_RANGE_A_LUT_SIZE];	///< range:0~1023, UINT32[17], range filter A threshold adjustment
	UINT32  b_th [CTL_IFE_ISP_RANGE_B_TH_NUM];		///< range:0~1023, UINT32[6] , range filter B threshold
	UINT32  b_lut[CTL_IFE_ISP_RANGE_B_LUT_SIZE];	///< range:0~1023, UINT32[17], range filter B threshold adjustment
} CTL_IFE_ISP_RANGE_FILTER_GR;

typedef struct {
	UINT32  a_th [CTL_IFE_ISP_RANGE_A_TH_NUM];		///< range:0~1023, UINT32[6] , range filter A threshold
	UINT32  a_lut[CTL_IFE_ISP_RANGE_A_LUT_SIZE];	///< range:0~1023, UINT32[17], range filter A threshold adjustment
	UINT32  b_th [CTL_IFE_ISP_RANGE_B_TH_NUM];		///< range:0~1023, UINT32[6] , range filter B threshold
	UINT32  b_lut[CTL_IFE_ISP_RANGE_B_LUT_SIZE];	///< range:0~1023, UINT32[17], range filter B threshold adjustment
} CTL_IFE_ISP_RANGE_FILTER_GB;

typedef struct {
	UINT32  a_th [CTL_IFE_ISP_RANGE_A_TH_NUM];		///< range:0~1023, UINT32[6] , range filter A threshold
	UINT32  a_lut[CTL_IFE_ISP_RANGE_A_LUT_SIZE];	///< range:0~1023, UINT32[17], range filter A threshold adjustment
	UINT32  b_th [CTL_IFE_ISP_RANGE_B_TH_NUM];		///< range:0~1023, UINT32[6] , range filter B threshold
	UINT32  b_lut[CTL_IFE_ISP_RANGE_B_LUT_SIZE];	///< range:0~1023, UINT32[17], range filter B threshold adjustment
} CTL_IFE_ISP_RANGE_FILTER_B;

typedef struct {
	UINT32  a_th [CTL_IFE_ISP_RANGE_A_TH_NUM];		///< range:0~1023, UINT32[6] , range filter A threshold
	UINT32  a_lut[CTL_IFE_ISP_RANGE_A_LUT_SIZE];	///< range:0~1023, UINT32[17], range filter A threshold adjustment
	UINT32  b_th [CTL_IFE_ISP_RANGE_B_TH_NUM];		///< range:0~1023, UINT32[6] , range filter B threshold
	UINT32  b_lut[CTL_IFE_ISP_RANGE_B_LUT_SIZE];	///< range:0~1023, UINT32[17], range filter B threshold adjustment
} CTL_IFE_ISP_RANGE_FILTER_IR;

typedef struct {
	UINT32  weight[CTL_IFE_ISP_SPATIAL_W_LEN];		///< range:0~31, UINT32[6], filter spatial weight,
} CTL_IFE_ISP_SPATIAL;

/**
    clamp and weighting setting
*/
typedef struct {
	UINT32  th;			///< range:0~4095, threshold
	UINT32  mul;		///< range:0~255 , weighting multiplier
	UINT32  dlt;		///< range:0~4095, threshold adjustment
} CTL_IFE_ISP_CLAMP;

/**
    IFE structure - IFE RB fill parameter set.
*/
typedef struct {
	BOOL    enable;									///< range: 0~1 , Enable/Disable
	UINT8   luma [CTL_IFE_ISP_RBFILL_LUMA_NUM];		///< range: 0~31,
	UINT8   ratio[CTL_IFE_ISP_RBFILL_RATIO_NUM];	///< range: 0~31,
	UINT8   ratio_mode;								///< range: 0~2 , n3.2, n2.3, n1.4
} CTL_IFE_ISP_RBFILL_PARAM;

/**
    IFE bilateral filter
*/
typedef struct {
	BOOL                           enable;			///< Enable/Disable
	CTL_IFE_ISP_SPATIAL            spatial;			///< filter spatial weight table
	CTL_IFE_ISP_RANGE_FILTER_R     rng_filt_r;		///< range filter ch0
	CTL_IFE_ISP_RANGE_FILTER_GR    rng_filt_gr;		///< range filter ch1
	CTL_IFE_ISP_RANGE_FILTER_GB    rng_filt_gb;		///< range filter ch2
	CTL_IFE_ISP_RANGE_FILTER_B     rng_filt_b;		///< range filter ch3
	CTL_IFE_ISP_RANGE_FILTER_IR    rng_filt_ir;		///< range filter ch2

	CTL_IFE_ISP_CENTER_MODIFY      center_mod;		///< center modify
	CTL_IFE_ISP_CLAMP              clamp;			///< clamp and weighting setting
	CTL_IFE_ISP_RBFILL_PARAM       rbfill;			///< RGBIr fill r/b pixel
	UINT32                         blend_w;			///< range:0~255, range filter A and B weighting
	UINT32                         rng_th_w;		///< range:0~15 ,range th weight
	UINT32                         bin;				///< range:0~7  , ( Denominator: 2<<bin )
} CTL_IFE_ISP_FILTER;

/**
    IFE color gain
*/
typedef struct {
	BOOL                    enable;			///< range:0~1   , Enable/Disable
	BOOL                    inv;			///< range:0~1   , color gain invert
	BOOL                    hinv;			///< range:0~1   , color gain H-invert
	CTL_IFE_ISP_GAIN_FIELD  bit_field;		///< range:0~1   , color gain bit field
	UINT32                  mask;			///< range:0~4095, color gain mask

	UINT32                  cgain_r;		///< range 0~1023, R  channel color gain.
	UINT32                  cgain_gr;		///< range 0~1023, Gr channel color gain.
	UINT32                  cgain_gb;		///< range 0~1023, Gb channel color gain.
	UINT32                  cgain_b;		///< range 0~1023, B  channel color gain.
	UINT32                  cgain_ir;		///< range 0~1023, IR channel color gain.

	UINT32                  cofs_r;			///< range 0~1023, R  channel color offset.
	UINT32                  cofs_gr;		///< range 0~1023, Gr channel color offset.
	UINT32                  cofs_gb;		///< range 0~1023, Gb channel color offset.
	UINT32                  cofs_b;			///< range 0~1023, B  channel color offset.
	UINT32                  cofs_ir;		///< range 0~1023, IR channel color offset.
} CTL_IFE_ISP_CGAIN;

/**
    ife vignette shading compensation
*/
typedef struct {
	BOOL    enable;										///< range: 0~1   , Enable/Disable
	UINT32  dist_th;									///< range: 0~1023, disabled area distance threshold

	UINT32  ch_r_lut[CTL_IFE_ISP_VIG_CH0_LUT_SIZE];		///< range: 0~1023, VIG LUT of R  channel
	UINT32  ch_gr_lut[CTL_IFE_ISP_VIG_CH1_LUT_SIZE];	///< range: 0~1023, VIG LUT of Gr channel
	UINT32  ch_gb_lut[CTL_IFE_ISP_VIG_CH2_LUT_SIZE];	///< range: 0~1023, VIG LUT of Gb channel
	UINT32  ch_b_lut[CTL_IFE_ISP_VIG_CH3_LUT_SIZE];		///< range: 0~1023, VIG LUT of B  channel
	UINT32  ch_ir_lut[CTL_IFE_ISP_VIG_CH2_LUT_SIZE];	///< range: 0~1023, VIG LUT of Ir channel

	BOOL    dither_enable;								///< range: 0~1   ,dithering enable/disable
	BOOL    dither_rst_enable;							///< range: 0~1   ,dithering reset enable
} CTL_IFE_ISP_VIG;

/**
    IFE structure - IFE gbalance parameter set.
*/
typedef struct {
	BOOL     enable;				///< range: 0~1   , enable/disable
	BOOL     protect_enable;		///< range: 0~1   , enable/disable preserve edge or feature
	UINT16   diff_th_str;			///< range: 0~1023, gbal strength
	UINT8    diff_w_max;			///< range: 0~15  , difference weight max
	UINT16   edge_protect_th1;		///< range: 0~1023, edge judgement th 1
	UINT16   edge_protect_th0;		///< range: 0~1023, edge judgement th 0
	UINT8    edge_w_max;			///< range: 0~255 , edge weight max
	UINT8    edge_w_min;			///< range: 0~255 , edge weight min
	UINT8    gbal_ofs[CTL_IFE_ISP_GBAL_OFS_NUM];///< range: 0~63  , weight transision region
} CTL_IFE_ISP_GBAL;

typedef struct {
	CTL_IFE_ISP_NRS     *p_nrs;
	CTL_IFE_ISP_FCURVE  *p_fcurve;
	CTL_IFE_ISP_FUSION  *p_fusion;

	CTL_IFE_ISP_OUTL    *p_outl;
	CTL_IFE_ISP_FILTER  *p_filt;
	CTL_IFE_ISP_CGAIN   *p_cgain;
	CTL_IFE_ISP_VIG     *p_vig;
	CTL_IFE_ISP_GBAL    *p_gbal;
} CTL_IFE_ISP_IQ_ALL;

#endif

