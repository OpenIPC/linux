#ifndef _IQ_PARAM_H_
#define _IQ_PARAM_H_

#include "kwrap/type.h"

//=============================================================================
// struct & definition
//=============================================================================
// OB
#define IQ_OB_LEN 5
// NR
#define IQ_NR_NRS_LEN 5
#define IQ_NR_OUTL_LEN 5
#define IQ_NR_TH_NUM 6
#define IQ_NR_TH_LUT 17
// CFA
#define IQ_CFA_FCS_NUM 16
// VA
#define IQ_VA_INDEP_NUM 5
// GAMMA
#define IQ_GAMMA_TONE_INPUT_BLD_NUM 17
#define IQ_GAMMA_TONE_IDX_NUM 32
#define IQ_GAMMA_TONE_VAL_NUM 65
#define IQ_GAMMA_GAMMA_LEN 129
// CCM
#define IQ_CCM_LEN 9
#define IQ_CCM_HUETAB_LEN 24
#define IQ_CCM_SATTAB_LEN 24
// COLOR
#define IQ_COLOR_FSTAB_LEN 16
#define IQ_COLOR_FDTAB_LEN 16
#define IQ_COLOR_CCON_LEN 17
// Contrast
#define IQ_CONTRAST_LCE_LEN 9
// Edge
#define IQ_EDGE_LUT_LEN 16
#define IQ_EDGE_HUE_NUM 24
// 3DNR
#define IQ_3DNR_OBJECT_NUM 3
#define IQ_3DNR_PENALTY_LEN 8
#define IQ_3DNR_SWITCH_LEN 8
#define IQ_3DNR_BASE_LEN 8
#define IQ_3DNR_COEFA_LEN 8
#define IQ_3DNR_COEFB_LEN 8
#define IQ_3DNR_STD_LEN 8
#define IQ_3DNR_TH_NUM 2
#define IQ_3DNR_NODE_NUM 2
#define IQ_3DNR_RATIO_NUM 2
#define IQ_3DNR_FREQ_NUM 4
#define IQ_3DNR_F3_LUT_LEN 8
#define IQ_3DNR_F4_LUT_LEN 8
#define IQ_3DNR_ROI_FINAL_THRESHOLD_TAB 2
// DPC
#define IQ_DPC_MAX_NUM 4096
// Shading
#define IQ_SHADING_ECS_WIN 65
#define IQ_SHADING_ECS_LEN 65*65
#define IQ_SHADING_VIG_LEN 17
// LDC
#define IQ_LDC_GEO_NUM 65
#define IQ_LDC_2DLUT_NUM 65*65
// WDR
#define IQ_WDR_IDX_NUM 32
#define IQ_WDR_VAL_NUM 65
// SHDR
#define IQ_SHDR_NRS_OFS_NUM 6
#define IQ_SHDR_NRS_TH_NUM 5
#define IQ_SHDR_FUSION_DIFF_W_NUM 16
#define IQ_SHDR_FCURVE_IDX_NUM 32
#define IQ_SHDR_FCURVE_SPLIT_NUM 32
#define IQ_SHDR_FCURVE_VAL_NUM 65
// COMPANDING
#define IQ_COMPANDING_KNEE_NUM 11
#define IQ_COMPANDING_SECTION_NUM 12
// POST_3DNR
#define IQ_POST_3DNR_TH_NUM 3
#define IQ_POST_3DNR_FRAME_W_NUM 3
#define IQ_POST_3DNR_RANGE_W_NUM 3
// POST_SHARPEN
#define IQ_POST_SHARPEN_CURVE_NUM 17
// CST
#define IQ_CST_LEN 9
// YCURVE
#define IQ_YCURVE_LEN 129

/**
	IQ process id
*/
typedef enum _IQ_ID {
	IQ_ID_1 = 0,                      ///< iq id 1
	IQ_ID_2,                          ///< iq id 2
	IQ_ID_3,                          ///< iq id 3
	IQ_ID_4,                          ///< iq id 4
	IQ_ID_5,                          ///< iq id 5
	IQ_ID_MAX_NUM,
	ENUM_DUMMY4WORD(IQ_ID)
} IQ_ID;

typedef enum _IQ_OP_TYPE {
	IQ_OP_TYPE_AUTO = 0,              ///< iq setting from manual_param
	IQ_OP_TYPE_MANUAL,                ///< iq setting from auto_param, interpolation by gain, LV or CT
	IQ_OP_TYPE_MAX,
	ENUM_DUMMY4WORD(IQ_OP_TYPE)
} IQ_OP_TYPE;

typedef enum _IQ_GAIN_ID {
	IQ_GAIN_1X = 0,                   ///< iq gain 1X
	IQ_GAIN_2X,                       ///< iq gain 2X
	IQ_GAIN_4X,                       ///< iq gain 4X
	IQ_GAIN_8X,                       ///< iq gain 8X
	IQ_GAIN_16X,                      ///< iq gain 16X
	IQ_GAIN_32X,                      ///< iq gain 32X
	IQ_GAIN_64X,                      ///< iq gain 64X
	IQ_GAIN_128X,                     ///< iq gain 128X
	IQ_GAIN_256X,                     ///< iq gain 256X
	IQ_GAIN_512X,                     ///< iq gain 512X
	IQ_GAIN_1024X,                    ///< iq gain 1024X
	IQ_GAIN_2048X,                    ///< iq gain 2048X
	IQ_GAIN_4096X,                    ///< iq gain 4096X
	IQ_GAIN_8192X,                    ///< iq gain 8192X
	IQ_GAIN_16384X,                   ///< iq gain 16384X
	IQ_GAIN_32768X,                   ///< iq gain 32768X
	IQ_GAIN_ID_MAX_NUM,
	ENUM_DUMMY4WORD(IQ_GAIN_ID)
} IQ_GAIN_ID;

typedef enum _IQ_COLOR_ID {
	IQ_COLOR_TEMPERATURE_H = 0,       ///< iq color temperature high
	IQ_COLOR_TEMPERATURE_M,           ///< iq color temperature middle
	IQ_COLOR_TEMPERATURE_L,           ///< iq color temperature low
	IQ_COLOR_ID_MAX_NUM,
	ENUM_DUMMY4WORD(IQ_COLOR_ID)
} IQ_COLOR_ID;

typedef enum _IQ_GAMMA_ID {
	IQ_GAMMA_DAY = 0,                 ///< iq gamma day
	IQ_GAMMA_NIGHT,                   ///< iq gamma night
	IQ_GAMMA_ID_MAX_NUM,
	ENUM_DUMMY4WORD(IQ_GAMMA_ID)
} IQ_GAMMA_ID;

typedef enum _IQ_LIGHT_ID {
	IQ_LIGHT_D = 0,                   ///< iq light D65
	IQ_LIGHT_A,                       ///< iq light A
	IQ_LIGHT_IR,                      ///< iq light IR
	IQ_LIGHT_ID_MAX_NUM,
	ENUM_DUMMY4WORD(IQ_LIGHT_ID)
} IQ_LIGHT_ID;

typedef enum _IQ_ECS_ID {
	IQ_ECS_TEMPERATURE_H = 0,       ///< iq ECS temperature high
	IQ_ECS_TEMPERATURE_M,           ///< iq ECS temperature middle
	IQ_ECS_TEMPERATURE_L,           ///< iq ECS temperature low
	IQ_ECS_ID_MAX_NUM,
	ENUM_DUMMY4WORD(IQ_ECS_ID)
} IQ_ECS_ID;

typedef enum IQ_PROC_MODE {
	IQ_PROC_MOVIE,
	IQ_PROC_PHOTO,
	IQ_PROC_CAPTURE,
	ENUM_DUMMY4WORD(IQ_PROC_MODE)
} IQ_PROC_MODE;

typedef enum _IQ_OB_MODE {
	IQ_OB_SIE,
	IQ_OB_IFE_F,
	IQ_OB_IFE,
	ENUM_DUMMY4WORD(IQ_OB_MODE)
} IQ_OB_MODE;

typedef enum _IQ_DG_MODE {
	IQ_DG_OFF,
	IQ_DG_SIE,
	IQ_DG_IFE_F,
	IQ_DG_IFE,
	IQ_DG_DCE,
	ENUM_DUMMY4WORD(IQ_DG_MODE)
} IQ_DG_MODE;

typedef enum _IQ_CG_MODE {
	IQ_CG_SIE,
	IQ_CG_IFE_F,
	IQ_CG_IFE,
	IQ_CG_DCE,
	ENUM_DUMMY4WORD(IQ_CG_MODE)
} IQ_CG_MODE;

/**
	All Structs for IQTOOL
*/
// Struct of IQ Parameter
typedef struct _IQ_OB_TUNE_PARAM {
	UINT32 cofs[IQ_OB_LEN];                     ///< range : 0~4095, ob offset
} IQ_OB_TUNE_PARAM;

typedef struct _IQ_OB_PARAM {
	IQ_OP_TYPE mode;
	IQ_OB_TUNE_PARAM manual_param;
	IQ_OB_TUNE_PARAM auto_param[IQ_GAIN_ID_MAX_NUM];
} IQ_OB_PARAM;

typedef enum _IQ_NR_OUTL_SEL {
	IQ_NR_OUTL_8_NODE,
	IQ_NR_OUTL_7_NODE,
	ENUM_DUMMY4WORD(IQ_NR_OUTL_SEL)
} IQ_NR_OUTL_SEL;

typedef struct _IQ_NR_TUNE_PARAM {
	UINT16 outl_ord_protect_th;                 ///< range : 0~1023, order method protect artifact th
	UINT8 outl_ord_blend_w;                     ///< range : 0~255, outlier and order result blending weight
	BOOL outl_avg_mode;                         ///< range : 0~1, outlier use direction average or not
	IQ_NR_OUTL_SEL outl_sel;
	UINT32 outl_bright_th[IQ_NR_OUTL_LEN];      ///< range : 0~4095
	UINT32 outl_dark_th[IQ_NR_OUTL_LEN];        ///< range : 0~4095
	UINT16 gbal_diff_th_str;                    ///< range : 0~1023, gbal strength
	UINT16 gbal_edge_protect_th;                ///< range : 0~1023, edge judgement th
	UINT32 filter_th[IQ_NR_TH_NUM];             ///< range : 0~1023, range filter threshold
	UINT32 filter_lut[IQ_NR_TH_LUT];            ///< range : 0~1023, range filter threshold adjustment
	UINT32 filter_th_b[IQ_NR_TH_NUM];           ///< range : 0~1023, range filter threshold
	UINT32 filter_lut_b[IQ_NR_TH_LUT];          ///< range : 0~1023, range filter threshold adjustment
	UINT32 filter_blend_w;                      ///< range : 0~15, range A and B weighting
	UINT32 filter_clamp_th;                     ///< range : 0~4095, threshold
	UINT32 filter_clamp_mul;                    ///< range : 0~255, 1X = 128, weighting multiplier
	UINT32 lca_sub_center_filter_y;             ///< range : 0~255, Range filter strength for reference center
	UINT32 lca_sub_center_filter_c;             ///< range : 0~255, Range filter strength for reference center
	UINT32 lca_sub_filter_y;                    ///< range : 0~255, Range Filter strength
	UINT32 lca_sub_filter_c;                    ///< range : 0~255, Range Filter strength
	UINT32 lca_out_y_wt;                        ///< range : 0~31, Luma adaptation output weighting
	UINT32 lca_out_c_wt;                        ///< range : 0~31, Chroma adaptation output weighting
	UINT32 dbcs_step_y;                         ///< range : 0~3, Step for luma (0:0~16 / 1:0~32 / 2:0~64 / 3:0~128)
	UINT32 dbcs_step_c;                         ///< range : 0~3, Step for chroma (0:-16~16 / 1:-32~32 / 2:-64~64 / 3:-128~128)
} IQ_NR_TUNE_PARAM;

typedef struct _IQ_NR_PARAM {
	BOOL outl_enable;                           ///< enable function
	BOOL gbal_enable;                           ///< enable function
	BOOL filter_enable;                         ///< enable function
	BOOL lca_enable;                            ///< enable function
	BOOL dbcs_enable;                           ///< enable function
	IQ_OP_TYPE mode;
	IQ_NR_TUNE_PARAM manual_param;
	IQ_NR_TUNE_PARAM auto_param[IQ_GAIN_ID_MAX_NUM];
} IQ_NR_PARAM;

typedef struct _IQ_CFA_TUNE_PARAM {
	UINT32 edge_dth;                            ///< range : 0~4095, Edge threshold 1 for Bayer CFA interpolation
	UINT32 edge_dth2;                           ///< range : 0~4095, Edge threshold 2 for Bayer CFA interpolation
	UINT32 freq_th;                             ///< range : 0~4095, CFA frequency threshold
	UINT32 fcs_weight;                          ///< range : 0~255, Global strength for false color suppression
	UINT32 fcs_strength[IQ_CFA_FCS_NUM];        ///< range : 0~15, CFA false color suppression strength
} IQ_CFA_TUNE_PARAM;

typedef struct _IQ_CFA_PARAM {
	IQ_OP_TYPE mode;
	IQ_CFA_TUNE_PARAM manual_param;
	IQ_CFA_TUNE_PARAM auto_param[IQ_GAIN_ID_MAX_NUM];
} IQ_CFA_PARAM;

typedef struct _IQ_VA_TUNE_PARAM {
	UINT16 g1_th_l;                             ///< range : 0~255, grope1 lower threshold
	UINT16 g1_th_u;                             ///< range : 0~255, grope1 upper threshold
	UINT16 g2_th_l;                             ///< range : 0~255, grope2 lower threshold
	UINT16 g2_th_u;                             ///< range : 0~255, grope2 upper threshold
} IQ_VA_TUNE_PARAM;

typedef struct _IQ_VA_PARAM {
	IQ_OP_TYPE mode;
	URECT indep_roi[IQ_VA_INDEP_NUM];           ///< range : 0~1000, roi ratio, (x + w) <= 1000, (y + h) <= 1000
	UINT8 g1_tap_a;                             ///< range : 0~31, grope1 filter coefficent A
	INT8 g1_tap_b;                              ///< range : -16~15, grope1 filter coefficent B
	INT8 g1_tap_c;                              ///< range : -8~7, grope1 filter coefficent C
	INT8 g1_tap_d;                              ///< range : -8~7, grope1 filter coefficent D
	UINT8 g1_div;                               ///< range : 0~15, grope1 filter normalized term
	UINT8 g2_tap_a;                             ///< range : 0~31, grope2 filter coefficent A
	INT8 g2_tap_b;                              ///< range : -16~15, grope2 filter coefficent B
	INT8 g2_tap_c;                              ///< range : -8~7, grope2 filter coefficent C
	INT8 g2_tap_d;                              ///< range : -8~7, grope2 filter coefficent D
	UINT8 g2_div;                               ///< range : 0~15, grope2 filter normalized term
	IQ_VA_TUNE_PARAM manual_param;
	IQ_VA_TUNE_PARAM auto_param[IQ_GAIN_ID_MAX_NUM];
} IQ_VA_PARAM;

typedef struct _IQ_GAMMA_MANUAL_PARAM {
	UINT32 gamma_lut[IQ_GAMMA_GAMMA_LEN];                             ///< range : 0~1023, gamma input, table size: 129
} IQ_GAMMA_MANUAL_PARAM;

typedef struct _IQ_GAMMA_AUTO_PARAM {
	UINT32 lv;                                                        ///< range : 100~2000
	UINT32 gamma_lut[IQ_GAMMA_GAMMA_LEN];                             ///< range : 0~1023, gamma input, table size: 129
} IQ_GAMMA_AUTO_PARAM;

typedef struct _IQ_GAMMA_PARAM {
	BOOL tone_enable;                                                 ///< enable function
	BOOL gamma_enable;                                                ///< enable function
	IQ_OP_TYPE mode;
	UINT32 tone_input_blend_lut[IQ_GAMMA_TONE_INPUT_BLD_NUM];         ///< range : 0~255, blending of Y and raw, 0: Y, 255: raw
	UINT32 tone_lut_linear_idx[IQ_GAMMA_TONE_IDX_NUM];                ///< range : 0~63, non equatable table index
	UINT32 tone_lut_linear_split[IQ_GAMMA_TONE_IDX_NUM];              ///< range : 0~3, non equatable table split number
	UINT32 tone_lut_linear_val[IQ_GAMMA_TONE_VAL_NUM];                ///< range : 0~4095, non equatable table value
	UINT32 tone_lut_shdr_idx[IQ_GAMMA_TONE_IDX_NUM];                  ///< range : 0~63, non equatable table index
	UINT32 tone_lut_shdr_split[IQ_GAMMA_TONE_IDX_NUM];                ///< range : 0~3, non equatable table split number
	UINT32 tone_lut_shdr_set_num;                                     ///< range : 2~5
	UINT32 tone_lut_shdr_set0_lv;                                     ///< range : 0~100
	UINT32 tone_lut_shdr_set0_val[IQ_GAMMA_TONE_VAL_NUM];             ///< range : 0~4095, non equatable table value
	UINT32 tone_lut_shdr_set1_lv;                                     ///< range : 0~100
	UINT32 tone_lut_shdr_set1_val[IQ_GAMMA_TONE_VAL_NUM];             ///< range : 0~4095, non equatable table value
	UINT32 tone_lut_shdr_set2_lv;                                     ///< range : 0~100
	UINT32 tone_lut_shdr_set2_val[IQ_GAMMA_TONE_VAL_NUM];             ///< range : 0~4095, non equatable table value
	UINT32 tone_lut_shdr_set3_lv;                                     ///< range : 0~100
	UINT32 tone_lut_shdr_set3_val[IQ_GAMMA_TONE_VAL_NUM];             ///< range : 0~4095, non equatable table value
	UINT32 tone_lut_shdr_set4_lv;                                     ///< range : 0~100
	UINT32 tone_lut_shdr_set4_val[IQ_GAMMA_TONE_VAL_NUM];             ///< range : 0~4095, non equatable table value
	IQ_GAMMA_MANUAL_PARAM manual_param;
	IQ_GAMMA_AUTO_PARAM auto_param[IQ_GAMMA_ID_MAX_NUM];
} IQ_GAMMA_PARAM;

typedef struct _IQ_CCM_MANUAL_PARAM {
	INT16 coef[IQ_CCM_LEN];                     ///< range : -1024~1023, 1X = 256, Color correction matrix [rr rg rb gr gg gb br bg bb], table size: 9
	UINT8 hue_tab[IQ_CCM_HUETAB_LEN];           ///< range : 0~255, color control hue adjust table, 128 : NO hue adjust, table size: 24
	INT32 sat_tab[IQ_CCM_SATTAB_LEN];           ///< range : -128~127, color control sat adjust table, 0 : NO sat adjust, -128 : Min sat suppress, 127 : Max sat enhance, table size: 24
} IQ_CCM_MANUAL_PARAM;

typedef struct _IQ_CCM_AUTO_PARAM {
	UINT32 ct;                                  ///< range : 1000~12000
	INT16 coef[IQ_CCM_LEN];                     ///< range : -1024~1023, 1X = 256, Color correction matrix [rr rg rb gr gg gb br bg bb], table size: 9
	UINT8 hue_tab[IQ_CCM_HUETAB_LEN];           ///< range : 0~255, color control hue adjust table, 128 : NO hue adjust, table size: 24
	INT32 sat_tab[IQ_CCM_SATTAB_LEN];           ///< range : -128~127, color control sat adjust table, 0 : NO sat adjust, -128 : Min sat suppress, 127 : Max sat enhance, table size: 24
} IQ_CCM_AUTO_PARAM;

typedef struct _IQ_CCM_PARAM {
	IQ_OP_TYPE mode;
	IQ_CCM_MANUAL_PARAM manual_param;
	IQ_CCM_AUTO_PARAM auto_param[IQ_COLOR_ID_MAX_NUM];
} IQ_CCM_PARAM;

typedef struct _IQ_COLOR_TUNE_PARAM {
	UINT8 c_con;                                ///< range : 0~255, CbCr contrast adjust, 128 : NO contrst adjust, C' = (C * CCon) >> 7
	UINT8 fstab[IQ_COLOR_FSTAB_LEN];            ///< range : 0~255, Color correction stab, table size: 16
	UINT8 fdtab[IQ_COLOR_FDTAB_LEN];            ///< range : 0~255, Color correction dtab, table size: 16
	UINT32 cconlut[IQ_COLOR_CCON_LEN];          ///< range : 0~1023, CbCr contrast adjust, 128 : NO contrst adjust, C' = (C * CCon) >> 7
} IQ_COLOR_TUNE_PARAM;

typedef struct _IQ_COLOR_PARAM {
	IQ_OP_TYPE mode;
	IQ_COLOR_TUNE_PARAM manual_param;
	IQ_COLOR_TUNE_PARAM auto_param[IQ_GAIN_ID_MAX_NUM];
} IQ_COLOR_PARAM;

typedef struct _IQ_CONTRAST_TUNE_PARAM {
	UINT8 y_con;                                ///< range : 0~255, Y contrast adjust, 128 : NO contrst adjust, Y' = (Y * Ycon) >> 7
	UINT8 lce_lum_wt_lut[IQ_CONTRAST_LCE_LEN];  ///< range : 0~191, 0 = OFF(1X), 64 = 2x, Lce luma weighting table
} IQ_CONTRAST_TUNE_PARAM;

typedef struct _IQ_CONTRAST_PARAM {
	IQ_OP_TYPE mode;
	IQ_CONTRAST_TUNE_PARAM manual_param;
	IQ_CONTRAST_TUNE_PARAM auto_param[IQ_GAIN_ID_MAX_NUM];
} IQ_CONTRAST_PARAM;

typedef struct _IQ_EDGE_TUNE_PARAM {
	UINT32 edge_enh_p;                          ///< range : 0~1023, 1X = 64, Positive edge enhance weighting
	UINT32 edge_enh_n;                          ///< range : 0~1023, 1X = 64, Negative edge enhance weighting
	UINT32 thin_freq;                           ///< range : 0~16
	UINT32 robust_freq;                         ///< range : 0~16
	UINT32 wt_low;                              ///< range : 0~16, the kernel weighting of flat region
	UINT32 wt_high;                             ///< range : 0~16, the kernel weighting of edge region
	UINT32 th_flat_low;                         ///< range : 0~1023, the threshold for flat region
	UINT32 th_flat_high;                        ///< range : 0~1023, the threshold for flat region
	UINT32 th_edge_low;                         ///< range : 0~1023, the threshold for edge region
	UINT32 th_edge_high;                        ///< range : 0~1023, the threshold for edge region
	UINT8 str_flat;                             ///< range : 0~64, 1X = 64, edge enhance of flat region
	UINT8 str_edge;                             ///< range : 64~255, 1X = 64, edge enhance of edge region
	UINT32 overshoot_str;                       ///< range : 0~32767 (256, slope = 1), the slope of the overshooting weight curve,
	UINT32 undershoot_str;                      ///< range : 0~32767 (256, slope = 1), the slope of the undershooting weight curve
	UINT32 edge_ethr_low;                       ///< range : 0~1023, for edge mapping, if(Ein < EthrA){Eout=EDTabA[0]}, else if(EthrA <= Ein <EthrB){Eout apply EtabA and EDtabA}, else if(Ein >= EthrB){Eout apply EtabB and EDtabB}
	UINT32 edge_ethr_high;                      ///< range : 0~1023, for edge mapping, if(Ein < EthrA){Eout=EDTabA[0]}, else if(EthrA <= Ein <EthrB){Eout apply EtabA and EDtabA}, else if(Ein >= EthrB){Eout apply EtabB and EDtabB}
	UINT32 edge_etab_low;                       ///< range : 0~6, for edge mapping, EDtabA table has 8 sections, each section width is (1 << etaA)
	UINT32 edge_etab_high;                      ///< range : 0~6, for edge mapping, EDtabB table has 8 sections, each section width is (1 << etaB)
	UINT32 es_ethr_low;                         ///< range : 0~1023, for edge mapping, if(Ein < EthrA){Eout=EDTabA[0]}, else if(EthrA <= Ein <EthrB){Eout apply EtabA and EDtabA}, else if(Ein >= EthrB){Eout apply EtabB and EDtabB}
	UINT32 es_ethr_high;                        ///< range : 0~1023, for edge mapping, if(Ein < EthrA){Eout=EDTabA[0]}, else if(EthrA <= Ein <EthrB){Eout apply EtabA and EDtabA}, else if(Ein >= EthrB){Eout apply EtabB and EDtabB}
	UINT32 es_etab_low;                         ///< range : 0~6, for edge mapping, EDtabA table has 8 sections, each section width is (1 << etaA)
	UINT32 es_etab_high;                        ///< range : 0~6, for edge mapping, EDtabB table has 8 sections, each section width is (1 << etaB)
} IQ_EDGE_TUNE_PARAM;

typedef struct _IQ_EDGE_PARAM {
	BOOL enable;                                ///< enable function
	IQ_OP_TYPE mode;
	UINT32 th_overshoot;                        ///< range : 0~255, the threshold for overshooting
	UINT32 th_undershoot;                       ///< range : 0~255, the threshold for undershooting
	UINT8 edge_map_lut[IQ_EDGE_LUT_LEN];        ///< range : 0~255, 1X = 255, table size: 16, ED map LUT(16 entries).
	UINT8 es_map_lut[IQ_EDGE_LUT_LEN];          ///< range : 0~255, 1X = 64, table size: 16, ES map LUT(16 entries).
	UINT8 edge_tab[IQ_EDGE_HUE_NUM];            ///< range : 0~255, color control edge adjust table, 128 : NO edge adjust, table size: 24
	UINT32 reduce_ratio;                        ///< range : 0~128, the edge enhance reduce ratio while scene change, 128 : NO edge enhance reduce
	IQ_EDGE_TUNE_PARAM manual_param;
	IQ_EDGE_TUNE_PARAM auto_param[IQ_GAIN_ID_MAX_NUM];
} IQ_EDGE_PARAM;

typedef enum _IQ_3DNR_PRE_FILTER_TYPE {
	IQ_3DNR_PRE_FILTER_TYPE_DISABLE = 0,
	IQ_3DNR_ISP_PRE_FILTER_TYPE_1 = 1,  // weakest
	IQ_3DNR_ISP_PRE_FILTER_TYPE_2 = 2,
	IQ_3DNR_ISP_PRE_FILTER_TYPE_3 = 3,  // strongest
	ENUM_DUMMY4WORD(IQ_3DNR_PRE_FILTER_TYPE)
} IQ_3DNR_PRE_FILTER_TYPE;

typedef struct _IQ_3DNR_TUNE_PARAM {
	IQ_3DNR_PRE_FILTER_TYPE pf_type;            ///< pre filter kernel strength select
	UINT8 cost_blend;                           ///< range : 0~15, Cost blending ratio, 0 for automatic cost, 15 for user defined cost
	UINT32 sad_penalty[IQ_3DNR_PENALTY_LEN];    ///< range : 0~1023
	UINT32 detail_penalty[IQ_3DNR_PENALTY_LEN]; ///< range : 0~15
	UINT32 switch_th[IQ_3DNR_SWITCH_LEN];       ///< range : 0~255
	UINT32 switch_rto;                          ///< range : 0~255
	UINT32 probability;                         ///< range : 0~8, switch probability
	UINT32 sad_base[IQ_3DNR_BASE_LEN];          ///< range : 0~16383, base level of noise
	UINT32 sad_coefa[IQ_3DNR_COEFA_LEN];        ///< range : 0~63, Edge coefficient of motion detection
	UINT32 sad_coefb[IQ_3DNR_COEFB_LEN];        ///< range : 0~16383, Offset of motion detection
	UINT32 sad_std[IQ_3DNR_STD_LEN];            ///< range : 0~16383, Standard deviation of motion detection
	UINT32 fth[IQ_3DNR_TH_NUM];                 ///< range : 0~63, final threshold of motion detection
	UINT32 mv_th;                               ///< range : 0~63, MV threshold
	UINT32 mix_ratio[IQ_3DNR_NODE_NUM];         ///< range : 0~63, Mix ratio in patch selection
	UINT32 ds_th;                               ///< range : 0~31, Threshold of motion status down-sampling
	UINT8 luma_residue_th[IQ_3DNR_OBJECT_NUM];  ///< range : 0~15, Protection threshold of luma channel
	UINT32 chroma_residue_th;                   ///< range : 0~15, Protection threshold of chroma channel
	UINT8 tf0_blur_str[IQ_3DNR_OBJECT_NUM];     ///< range : 0~255, Strength of TF0 blur filter for Y-Channel
	UINT8 tf0_y_str[IQ_3DNR_OBJECT_NUM];        ///< range : 0~128, Strength of TF0 filter for Y-Channel
	UINT8 tf0_c_str[IQ_3DNR_OBJECT_NUM];        ///< range : 0~128, Strength of TF0 filter for C-Channel
	UINT32 pre_filter_str[IQ_3DNR_FREQ_NUM];    ///< range : 0~255, Strength of pre-filtering for low frequency
	UINT32 pre_filter_rto[IQ_3DNR_RATIO_NUM];   ///< range : 0~255, 1X = 255, adjustment ratio of pre-filtering for transitional object
	UINT32 snr_base_th;                         ///< range : 0~65535, Base threshold of spatial noise reduction
	UINT32 tnr_base_th;                         ///< range : 0~65535, Base threshold of spatial noise reduction
	UINT32 freq_wet[IQ_3DNR_FREQ_NUM];          ///< range : 0~255, 1X = 16, Filter weighting for low frequency
	UINT32 luma_wet[IQ_3DNR_F3_LUT_LEN];        ///< range : 0~255, 1X = 16, Filter intensity weighting
	UINT32 snr_str[IQ_3DNR_OBJECT_NUM];         ///< range : 0~255, 1X = 64, Strength of spatial filter for still object
	UINT32 tnr_str[IQ_3DNR_OBJECT_NUM];         ///< range : 0~255, 1X = 64, Strength of temporal filter for still object
	UINT32 luma_3d_lut[IQ_3DNR_F4_LUT_LEN];     ///< range : 0~127, Noise reduction LUT for luma channel
	UINT32 luma_3d_rto[IQ_3DNR_RATIO_NUM];      ///< range : 0~255, 1X = 255, Adjustment ratio 0 of noise reduction LUT for luma channel
	UINT32 chroma_3d_lut[IQ_3DNR_F4_LUT_LEN];   ///< range : 0~127, Noise reduction LUT for chroma channel
	UINT32 chroma_3d_rto[IQ_3DNR_RATIO_NUM];    ///< range : 0~255, 1X = 255, Adjustment ratio 0 of noise reduction LUT for chroma channel
	UINT8 fcvg_start_point;                     ///< range : 0~15, Fast converge process start point
	UINT8 fcvg_step_size;                       ///< range : 0~15, Fast converge step size
} IQ_3DNR_TUNE_PARAM;

typedef struct _IQ_3DNR_PARAM {
	BOOL enable;                                ///< enable function
	BOOL fcvg_enable;                           ///< enable function
	IQ_OP_TYPE mode;
	IQ_3DNR_TUNE_PARAM manual_param;
	IQ_3DNR_TUNE_PARAM auto_param[IQ_GAIN_ID_MAX_NUM];
} IQ_3DNR_PARAM;

typedef struct _IQ_3DNR_MISC_PARAM {
	UINT32 md_roi[IQ_3DNR_ROI_FINAL_THRESHOLD_TAB];
	UINT32 mc_roi[IQ_3DNR_ROI_FINAL_THRESHOLD_TAB];
	UINT32 roi_mv_th;
	UINT32 ds_th_roi;
} IQ_3DNR_MISC_PARAM;

typedef struct _IQ_DPC_PARAM {
	BOOL enable;                                ///< enable function
	UINT32 table[IQ_DPC_MAX_NUM];               ///< dpc table
} IQ_DPC_PARAM;

typedef struct _IQ_SHADING_PARAM {
	BOOL ecs_enable;                            ///< enable function
	BOOL vig_enable;                            ///< enable function
	UINT32 ecs_map_tbl[IQ_SHADING_ECS_LEN];     ///< manual CT ECS table
	UINT32 vig_center_x;                        ///< range : 0~1000
	UINT32 vig_center_y;                        ///< range : 0~1000
	UINT32 vig_reduce_th;                       ///< range : 100~3276800
	UINT32 vig_zero_th;                         ///< range : 100~3276800
	UINT32 vig_lut[IQ_SHADING_VIG_LEN];         ///< range : 0~8191, UINT32[17], VIG LUT
} IQ_SHADING_PARAM;

typedef struct _IQ_SHADING_INTER_PARAM {
	IQ_OP_TYPE mode;
	UINT32 ecs_smooth_l_m_ct_lower;             ///< range : 1000~12000, interpolation range : ecs_smooth_l_m_ct_lower <= CT <= ecs_smooth_l_m_ct_upper
	UINT32 ecs_smooth_l_m_ct_upper;             ///< range : 1000~12000, interpolation range : ecs_smooth_l_m_ct_lower <= CT <= ecs_smooth_l_m_ct_upper
	UINT32 ecs_smooth_m_h_ct_lower;             ///< range : 1000~12000, interpolation range : ecs_smooth_m_h_ct_lower <= CT <= ecs_smooth_m_h_ct_upper
	UINT32 ecs_smooth_m_h_ct_upper;             ///< range : 1000~12000, interpolation range : ecs_smooth_m_h_ct_lower <= CT <= ecs_smooth_m_h_ct_upper
} IQ_SHADING_INTER_PARAM;

typedef struct _IQ_SHADING_EXT_PARAM_IF {
	IQ_ECS_ID ecs_map_idx;                      ///< 0 : hight, 1 : middle, 2 : low
	UINT32 ecs_map_tbl[IQ_SHADING_ECS_LEN];     ///< hight, middle, low CT ECS table
} IQ_SHADING_EXT_PARAM_IF;

typedef struct _IQ_SHADING_EXT_PARAM {
	UINT32 ecs_map_tbl[IQ_ECS_ID_MAX_NUM][IQ_SHADING_ECS_LEN];
} IQ_SHADING_EXT_PARAM;

typedef struct _IQ_LDC_PARAM {
	BOOL geo_enable;                            ///< enable function
	BOOL lut_2d_enable;                         ///< enable function
	UINT32 geo_center_x;                        ///< range : 0~1000
	UINT32 geo_center_y;                        ///< range : 0~1000
	UINT32 geo_fov_gain;                        ///< range : 0~4095, 1x = 1024, Scale down factor on corrected image (12 bit: 2.10)
	UINT32 geo_lut_g[IQ_LDC_GEO_NUM];           ///< 65 x 16bit
	UINT32 geo_r_lut_gain;                      ///< range : 0~8191, 1x = 4096
	UINT32 geo_b_lut_gain;                      ///< range : 0~8191, 1x = 4096
	UINT32 lut_2d_value[IQ_LDC_2DLUT_NUM];
} IQ_LDC_PARAM;

typedef struct _IQ_WDR_MANUAL_PARAM {
	UINT32 strength;                            ///< range : 0~255, wdr strength
} IQ_WDR_MANUAL_PARAM;

typedef struct _IQ_WDR_AUTO_PARAM {
	UINT32 level;                               ///< range : 0~255, wdr auto level
	UINT32 strength_min;                        ///< range : 0~255, wdr strength min
	UINT32 strength_max;                        ///< range : 0~255, wdr strength max
} IQ_WDR_AUTO_PARAM;

typedef struct _IQ_WDR_PARAM {
	BOOL enable;                                ///< enable function
	IQ_OP_TYPE mode;
	UINT32 subimg_size_h;                       ///< range : 4~32, wdr sub-image size
	UINT32 subimg_size_v;                       ///< range : 4~32, wdr sub-image size
	UINT32 contrast;                            ///< range : 0~255, wdr contrast, 128 = original
	UINT32 max_gain;                            ///< range : 1~255, maximum gain of wdr
	UINT32 min_gain;                            ///< range : 1~255, minimum gain of wdr, mapping to 1/1, 1/2 ~ 1/255
	UINT32 lut_idx[IQ_WDR_IDX_NUM];             ///< range : 0~63, non equatable table index
	UINT32 lut_split[IQ_WDR_IDX_NUM];           ///< range : 0~3, non equatable table split number
	UINT32 lut_val[IQ_WDR_VAL_NUM];             ///< range : 0~4095, non equatable table value
	IQ_WDR_MANUAL_PARAM manual_param;
	IQ_WDR_AUTO_PARAM auto_param[IQ_GAIN_ID_MAX_NUM];
} IQ_WDR_PARAM;

typedef enum _IQ_SHDR_F_CURVE_Y_SEL {
	IQ_SHDR_F_CURVE_Y_8G4R4B = 0,
	IQ_SHDR_F_CURVE_Y_4G = 1,
	IQ_SHDR_F_CURVE_Y_ORI = 2,
	ENUM_DUMMY4WORD(IQ_SHDR_F_CURVE_Y_SEL)
} IQ_SHDR_F_CURVE_Y_SEL;

typedef enum _IQ_SHDR_FUSION_SEL {
	IQ_SHDR_FUSION_BY_L_EXP = 0,
	IQ_SHDR_FUSION_BY_S_EXP = 1,
	IQ_SHDR_FUSION_BOTH_EXP = 2,
	ENUM_DUMMY4WORD(IQ_SHDR_FUSION_SEL)
} IQ_SHDR_FUSION_SEL;

typedef struct _IQ_SHDR_TUNE_PARAM {
	UINT16 nrs_diff_th;                                        ///< range : 0~1023, NRS Order difference threshold
	UINT8 nrs_ofs[IQ_SHDR_NRS_OFS_NUM];                        ///< range 0~255, NRS Bilateral offset
	UINT8 nrs_th[IQ_SHDR_NRS_TH_NUM];                          ///< range 0~3, NRS Bilateral threshold
} IQ_SHDR_TUNE_PARAM;

typedef struct _IQ_SHDR_PARAM {
	BOOL nrs_enable;                                           ///< enable function
	IQ_OP_TYPE mode;
	IQ_SHDR_FUSION_SEL fusion_nor_sel;                         ///< Normal blending curve weighting option
	UINT16 fusion_l_nor_knee;                                  ///< range : 0~4095, knee point of normal blending curve for long exposure
	UINT16 fusion_l_nor_range;                                 ///< range : 0~12, normal blending curve range for long exposure. Actually range = (1<<l_nor_range)
	UINT16 fusion_s_nor_knee;                                  ///< range : 0~4095, knee point of normal blending curve for short exposure
	UINT16 fusion_s_nor_range;                                 ///< range : 0~12, normal blending curve range for short exposure. Actually range = (1<<s_nor_range)
	IQ_SHDR_FUSION_SEL fusion_dif_sel;                         ///< Difference blending curve weighting option.
	UINT16 fusion_l_dif_knee;                                  ///< range : 0~4095, knee point of difference blending curve for long exposure
	UINT16 fusion_l_dif_range;                                 ///< range : 0~12, difference blending curve range for long exposure. Actually range = (1<<l_dif_range)
	UINT16 fusion_s_dif_knee;                                  ///< range : 0~4095, knee point of difference blending curve for long exposure
	UINT16 fusion_s_dif_range;                                 ///< range : 0~12, difference blending curve range for short exposure. Actually range = (1<<s_dif_range)
	UINT16 fusion_lum_th;                                      ///< range : 0~256, Lower than this threshold using normal blending curve
	UINT8 fusion_diff_w[IQ_SHDR_FUSION_DIFF_W_NUM];            ///< range : 0~16, Difference weighting
	IQ_SHDR_F_CURVE_Y_SEL fcurve_y_mean_sel;                   ///< Fcurve Y Mean Method selection
	UINT8 fcurve_yv_w;                                         ///< range : 0~8, Fcurve Y mean and V weight
	UINT8 fcurve_idx_lut[IQ_SHDR_FCURVE_IDX_NUM];              ///< range : 0~63, Fcurve Index
	UINT8 fcurve_split_lut[IQ_SHDR_FCURVE_SPLIT_NUM];          ///< range : 0~3, Fcurve Split
	UINT16 fcurve_val_lut[IQ_SHDR_FCURVE_VAL_NUM];             ///< range : 0~4095, Fcurve Value
	IQ_SHDR_TUNE_PARAM manual_param;
	IQ_SHDR_TUNE_PARAM auto_param[IQ_GAIN_ID_MAX_NUM];
} IQ_SHDR_PARAM;

typedef struct _IQ_RGBIR_MANUAL_PARAM {
	UINT32 irsub_r_weight;	                    ///< range : 0~2047, R channel IR sub ratio, 256 = 1X
	UINT32 irsub_g_weight;                      ///< range : 0~2047, G channel IR sub ratio, 256 = 1X
	UINT32 irsub_b_weight;                      ///< range : 0~2047, B channel IR sub ratio, 256 = 1X
	UINT32 ir_sat_gain;                         ///< range : 0~1023, Saturation gain multiplied after IR sub
} IQ_RGBIR_MANUAL_PARAM;

typedef struct _IQ_RGBIR_AUTO_PARAM {
	UINT32 irsub_r_weight;	                    ///< range : 0~2047, R channel IR sub ratio, 256 = 1X
	UINT32 irsub_g_weight;                      ///< range : 0~2047, G channel IR sub ratio, 256 = 1X
	UINT32 irsub_b_weight;                      ///< range : 0~2047, B channel IR sub ratio, 256 = 1X
	UINT32 irsub_reduce_th;                     ///< range : 128~255, IR level threshold start to reduce ir_sub
	UINT32 night_mode_th;                       ///< range : 128~255, IR level threshold start to NIGHT_Mode, fix ir_sub 0
} IQ_RGBIR_AUTO_PARAM;

typedef struct _IQ_RGBIR_PARAM {
	IQ_OP_TYPE mode;
	IQ_RGBIR_MANUAL_PARAM manual_param;
	IQ_RGBIR_AUTO_PARAM auto_param;
} IQ_RGBIR_PARAM;

typedef struct _IQ_COMPANDING_PARAM {
	UINT32 decomp_knee_pts[IQ_COMPANDING_KNEE_NUM];     ///< range : 0~4095
	UINT32 decomp_str_pts[IQ_COMPANDING_SECTION_NUM];   ///< range : 0~4095
	UINT32 decomp_shift_pts[IQ_COMPANDING_SECTION_NUM]; ///< range : 0~15
	UINT32 comp_knee_pts[IQ_COMPANDING_KNEE_NUM];       ///< range : 0~20
	UINT32 comp_str_pts[IQ_COMPANDING_SECTION_NUM];     ///< range : 0~4095
	UINT32 comp_shift_pts[IQ_COMPANDING_SECTION_NUM];   ///< range : 0~15
} IQ_COMPANDING_PARAM;

typedef struct _IQ_POST_SHARPEN_TUNE_PARAM {
	UINT8 noise_level;                                  ///< range: 0 ~ 255, Noise level
	UINT8 noise_curve[IQ_POST_SHARPEN_CURVE_NUM];       ///< range: 0 ~ 255, 17 control points of noise modulation curve
	UINT8 edge_weight_th;                               ///< range: 0 ~ 255, Edge weight coring threshold
	UINT8 edge_weight_gain;                             ///< range: 0 ~ 255, Edge weight gain
	UINT16 th_flat;                                     ///< range: 0 ~ 2047, flat region threshold
	UINT16 th_edge;                                     ///< range: 0 ~ 2047, Edge region threshold
	UINT8 tarnsition_region_str;                        ///< range: 0 ~ 128, Transition region weight strength
	UINT8 edge_region_str;                              ///< range: 0 ~ 128, Edge weight strength
	UINT8 edge_sharp_str;                               ///< range: 0 ~ 128, Sharpen strength of edge region
	UINT8 motion_edge_w_str;                            ///< range: 0 ~ 255, Motion region edge weight
	UINT8 static_edge_w_str;                            ///< range: 0 ~ 255, Static region edge weight strength
	UINT8 coring_th;                                    ///< range: 0 ~ 255, Coring threshold
	UINT8 blend_inv_gamma;                              ///< range: 0 ~ 128, Blending ratio of HPF results
	UINT8 bright_halo_clip;                             ///< range: 0 ~ 128, Bright halo clip ratio 0~128
	UINT8 dark_halo_clip;                               ///< range: 0 ~ 128, Dark halo clip ratio 0~128
} IQ_POST_SHARPEN_TUNE_PARAM;

typedef struct _IQ_POST_SHARPEN_PARAM {
	BOOL enable;                                        ///< enable function
	IQ_OP_TYPE mode;
	IQ_POST_SHARPEN_TUNE_PARAM manual_param;
	IQ_POST_SHARPEN_TUNE_PARAM auto_param[IQ_GAIN_ID_MAX_NUM];
} IQ_POST_SHARPEN_PARAM;

typedef struct _IQ_RGBIR_ENH_MANUAL_PARAM {
	UINT32 enh_ratio[IQ_GAIN_ID_MAX_NUM];               ///< range: 16 ~ 2048, 16 = 1X
} IQ_RGBIR_ENH_MANUAL_PARAM;

typedef struct _IQ_RGBIR_ENH_AUTO_PARAM {
	UINT32 ir_th;                                       ///< range: 0 ~ 255
	UINT32 enh_ratio[IQ_GAIN_ID_MAX_NUM];               ///< range: 16 ~ 2048, 16 = 1X
} IQ_RGBIR_ENH_AUTO_PARAM;

typedef struct _IQ_RGBIR_ENH_PARAM {
	BOOL enable;                                        ///< enable function
	IQ_OP_TYPE mode;
	UINT32 min_ir_th;                                   ///< range: 0 ~ 255, if (ir_level < ir_th), enh_ratio = 16(1X)
	IQ_RGBIR_ENH_MANUAL_PARAM manual_param;
	IQ_RGBIR_ENH_AUTO_PARAM auto_param[IQ_LIGHT_ID_MAX_NUM];
} IQ_RGBIR_ENH_PARAM;

typedef struct _IQ_MD_PARAM {
	BOOL enable;
	UINT8 sum_frms;                                     ///< //range: 0 ~ 15 (1~16 frames)
	UINT32 mask0;
	UINT32 mask1;
	UINT32 blkdiff_thr;
	UINT32 total_blkdiff_thr;
	UINT16 blkdiff_cnt_thr;
} IQ_MD_PARAM;

typedef struct _IQ_YCURVE_PARAM {
	BOOL enable;                                        ///< enable function
	UINT32 ycurve_lut[IQ_YCURVE_LEN];                  ///< range: 0 ~ 255, y curve, table size: 129
} IQ_YCURVE_PARAM;

typedef struct _IQ_PARAM_PTR {
	IQ_OB_PARAM                      *ob;
	IQ_NR_PARAM                      *nr;
	IQ_CFA_PARAM                     *cfa;
	IQ_VA_PARAM                      *va;
	IQ_GAMMA_PARAM                   *gamma;
	IQ_CCM_PARAM                     *ccm;
	IQ_COLOR_PARAM                   *color;
	IQ_CONTRAST_PARAM                *contrast;
	IQ_EDGE_PARAM                    *edge;
	IQ_3DNR_PARAM                    *_3dnr;
	IQ_WDR_PARAM                     *wdr;
	IQ_SHDR_PARAM                    *shdr;
	IQ_RGBIR_PARAM                   *rgbir;
	IQ_COMPANDING_PARAM              *companding;
	IQ_POST_SHARPEN_PARAM            *post_sharpen;
	IQ_RGBIR_ENH_PARAM               *rgbir_enh;
	IQ_DPC_PARAM                     *dpc;
	IQ_SHADING_PARAM                 *shading;
	IQ_SHADING_INTER_PARAM           *shading_inter;
	IQ_SHADING_EXT_PARAM             *shading_ext;
	IQ_LDC_PARAM                     *ldc;
	IQ_YCURVE_PARAM                  *ycurve;
} IQ_PARAM_PTR;

//=============================================================================
// extern functions
//=============================================================================
extern void iq_param_get_param(UINT32 *param);

#endif
