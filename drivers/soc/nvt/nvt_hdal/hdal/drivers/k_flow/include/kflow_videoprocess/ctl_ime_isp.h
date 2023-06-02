/**
    IPL Ctrl Layer, ISP Interface

    @file       ctl_ipp_isp.h
    @ingroup    mIIPL_Ctrl
    @note       None

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/


#ifndef _CTL_IME_ISP_H
#define _CTL_IME_ISP_H

#define CTL_IME_ISP_DBCS_WT_LUT_TAB            		(16)

#define CTL_IME_ISP_TMNR_ME_SAD_PENALTY_TAB            8
#define CTL_IME_ISP_TMNR_ME_SWITCH_THRESHOLD_TAB       8
#define CTL_IME_ISP_TMNR_ME_DETAIL_PENALTY_TAB         8
#define CTL_IME_ISP_TMNR_ME_PROBABILITY_TAB            8

#define CTL_IME_ISP_TMNR_MD_SAD_COEFA_TAB              8
#define CTL_IME_ISP_TMNR_MD_SAD_COEFB_TAB              8
#define CTL_IME_ISP_TMNR_MD_SAD_STD_TAB                8
#define CTL_IME_ISP_TMNR_MD_FINAL_THRESHOLD_TAB        2

#define CTL_IME_ISP_TMNR_MD_ROI_FINAL_THRESHOLD_TAB    2

#define CTL_IME_ISP_TMNR_MC_SAD_BASE_TAB               8
#define CTL_IME_ISP_TMNR_MC_SAD_COEFA_TAB              8
#define CTL_IME_ISP_TMNR_MC_SAD_COEFB_TAB              8
#define CTL_IME_ISP_TMNR_MC_SAD_STD_TAB                8
#define CTL_IME_ISP_TMNR_MC_FINAL_THRESHOLD_TAB        2

#define CTL_IME_ISP_TMNR_MC_ROI_FINAL_THRESHOLD_TAB    2

#define CTL_IME_ISP_TMNR_PS_MIX_RATIO_TAB              2
#define CTL_IME_ISP_TMNR_PS_MIX_THRESHOLD_TAB          2
#define CTL_IME_ISP_TMNR_PS_MIX_SLOPE_TAB              2
#define CTL_IME_ISP_TMNR_PS_EDGE_THRESHOLD_TAB         2

#define CTL_IME_ISP_TMNR_NR_FREQ_WEIGHT_TAB            4
#define CTL_IME_ISP_TMNR_NR_LUMA_WEIGHT_TAB            8
#define CTL_IME_ISP_TMNR_NR_PRE_FILTER_STRENGTH_TAB    4
#define CTL_IME_ISP_TMNR_NR_PRE_FILTER_RATION_TAB      2
#define CTL_IME_ISP_TMNR_NR_SFILTER_STRENGTH_TAB       3
#define CTL_IME_ISP_TMNR_NR_TFILTER_STRENGTH_TAB       3
#define CTL_IME_ISP_TMNR_NR_LUMA_LUT_TAB               8
#define CTL_IME_ISP_TMNR_NR_LUMA_RATIO_TAB             2
#define CTL_IME_ISP_TMNR_NR_LUMA_RESIDUE_TH_TAB        3
#define CTL_IME_ISP_TMNR_NR_CHROMA_LUT_TAB             8
#define CTL_IME_ISP_TMNR_NR_CHROMA_RATIO_TAB           2
#define CTL_IME_ISP_TMNR_NR_TF0_FILTER_TAB             3


#define CTL_IME_ISP_LCA_RANGE_TH_TAB      (3)
#define CTL_IME_ISP_LCA_RANGE_WT_TAB      (4)
#define CTL_IME_ISP_LCA_FILTER_TH_TAB     (5)
#define CTL_IME_ISP_LCA_FILTER_WT_TAB     (6)

/**
    KDRV IME CMF parameters
*/
typedef struct {
	BOOL enable; ///< chroma median filter parameter
} CTL_IME_ISP_CMF;

/**
    KDRV IME enum - value range selection

    @note   used for chroma adaptation
*/
typedef enum {
	CTL_IME_ISP_RANGE_8   = 0,  ///< range 8
	CTL_IME_ISP_RANGE_16  = 1,  ///< range 16
	CTL_IME_ISP_RANGE_32  = 2,  ///< range 32
	CTL_IME_ISP_RANGE_64  = 3,  ///< range 64
	CTL_IME_ISP_RANGE_128 = 4,  ///< range 128
	ENUM_DUMMY4WORD(CTL_IME_ISP_RANGE_SEL)
} CTL_IME_ISP_RANGE_SEL;

/**
    KDRV IME structure - local chroma adaptation parameters
*/
typedef struct {
	UINT32 ref_y_wt;   ///< Chroma reference weighting for Y channels
	UINT32 ref_uv_wt;  ///< Chroma reference weighting for UV channels
	UINT32 out_uv_wt;  ///< Chroma adaptation output weighting

	CTL_IME_ISP_RANGE_SEL y_rng;      ///< Chroma adaptation range selection for UV channels, Max = CTL_IME_ISP_RANGE_128
	CTL_IME_ISP_RANGE_SEL y_wt_prc;   ///< Chroma adaptation weighting precision for UV channel, Max = CTL_IME_ISP_RANGE_64
	UINT32 y_th;       ///< Chroma adaptation range threshold for UV channels
	UINT32 y_wt_s;     ///< Chroma adaptation weighting start value for UV channels
	UINT32 y_wt_e;     ///< Chroma adaptation weighting end value for UV channels


	CTL_IME_ISP_RANGE_SEL uv_rng;     ///< Chroma adaptation range selection for UV channels, Max = CTL_IME_ISP_RANGE_128
	CTL_IME_ISP_RANGE_SEL uv_wt_prc;  ///< Chroma adaptation weighting precision for UV channel, Max = CTL_IME_ISP_RANGE_64
	UINT32 uv_th;      ///< Chroma adaptation range threshold for UV channels
	UINT32 uv_wt_s;    ///< Chroma adaptation weighting start value for UV channels
	UINT32 uv_wt_e;    ///< Chroma adaptation weighting end value for UV channels
} CTL_IME_ISP_LCA_IQC;

/**
    KDRV IME structure - luma suppression parameters of local chroma adaptation
*/
typedef struct {
	BOOL enable;					///< Function enable
	UINT32 ref_wt;					///< Luma adaptation reference weighting for UV channels
	UINT32 out_wt;					///< Luma adaptation output weighting
	CTL_IME_ISP_RANGE_SEL rng;		///< Luma adaptation range selection for UV channels, Max = CTL_IME_ISP_RANGE_128
	CTL_IME_ISP_RANGE_SEL wt_prc;	///< Luma adaptation weighting precision for UV channel, Max = CTL_IME_ISP_RANGE_64
	UINT32 th;						///< Luma adaptation range threshold for UV channels
	UINT32 wt_s;					///< Luma adaptation weighting start value for UV channels
	UINT32 wt_e;					///< Luma adaptation weighting end value for UV channels
} CTL_IME_ISP_LCA_IQL;

/**
    KDRV IME structure - chroma adjustment parameters of local chroma adaptation
*/
typedef struct {
	BOOL enable;		///< Function enable
	CTL_IME_ISP_RANGE_SEL uv_rng;      ///< Chroma adjustment range selection for UV channels, Max = CTL_IME_ISP_RANGE_128
	CTL_IME_ISP_RANGE_SEL uv_wt_prc;   ///< Chroma adjustment weighting precision for UV channel, Max = CTL_IME_ISP_RANGE_64
	UINT32 uv_th;       ///< Chroma adjustment range threshold for UV channels
	UINT32 uv_wt_s;     ///< Chroma adjustment weighting start value for UV channels
	UINT32 uv_wt_e;     ///< Chroma adjustment weighting end value for UV channels
} CTL_IME_ISP_LCA_CA;

typedef struct {
	UINT32 cent_u;      ///< chroma adjustment reference center of U channel
	UINT32 cent_v;      ///< chroma adjustment reference center of V channel
} CTL_IME_ISP_LCA_CA_CENT_INFO;

/**
    KDRV IME enum - Chroma adaptation sub-image output source selection
*/
typedef enum {
	CTL_IME_ISP_LCA_SUBOUT_SRC_A = 0, ///< source: after LCA
	CTL_IME_ISP_LCA_SUBOUT_SRC_B = 1, ///< source: before LCA
} CTL_IME_ISP_LCA_SUBOUT_SRC;

/**
    KDRV IME structure - local chroma adaptation parameters
*/
typedef struct {
	CTL_IME_ISP_LCA_IQC chroma;		///< Function quality information for chroma channel
	CTL_IME_ISP_LCA_IQL luma;		///< Function quality information for luma channel
	CTL_IME_ISP_LCA_CA ca;			///< Chroma adjustment information
	CTL_IME_ISP_LCA_SUBOUT_SRC sub_out_src;	///< LCA subout source selection
} CTL_IME_ISP_LCA;

/**
    KDRV IME enum - dark and bright chroma suppression mode selection
*/
typedef enum {
	CTL_IME_ISP_DBCS_DK_MODE = 0,	///< dark mode
	CTL_IME_ISP_DBCS_BT_MODE = 1,	///< bright mode
	CTL_IME_ISP_DBCS_BOTH_MODE = 2,	///< dark and bright mode
	ENUM_DUMMY4WORD(CTL_IME_ISP_DBCS_MODE_SEL)
} CTL_IME_ISP_DBCS_MODE_SEL;

/**
    KDRV IME structure - single image super-resolution parameters
*/
typedef struct {
	BOOL enable;		///< super resolution function enable
	BOOL auto_mode_en;	///< auto mode of SSR, SSR function on/off controlled by IME driver
						///< SSR function controlled by USER if set DISABLE
	UINT32 diag_th;		///< Threahold for diagonal direction
	UINT32 h_v_th;		///< Threahold for horizontal and vertical direction
} CTL_IME_ISP_SSR;

/**
    KDRV IME structure - film grain noise parameters
*/
typedef struct {
	BOOL enable;	///< film grain enable
	UINT32 lum_th;  ///< luminance value threshold
	UINT32 nl_p1;   ///< path1 noise level
	UINT32 init_p1; ///< Grain noise initial value for path1
	UINT32 nl_p2;   ///< path2 noise level
	UINT32 init_p2; ///< Grain noise initial value for path2
	UINT32 nl_p3;   ///< path3 noise level
	UINT32 init_p3; ///< Grain noise initial value for path3
	UINT32 nl_p5;   ///< path5 noise level
	UINT32 init_p5; ///< Grain noise initial value for path5
} CTL_IME_ISP_FGN;

/**
    KDRV IME structure - dark and bright region chroma suppression
*/
typedef struct {
	BOOL enable;						///< dark and bright region chroma suppression function enable
	CTL_IME_ISP_DBCS_MODE_SEL op_mode;	///< Process mode
	UINT32 cent_u;						///< Center value for U channel
	UINT32 cent_v;						///< Center value for V channel
	UINT32 step_y;						///< Step for luma
	UINT32 step_c;						///< Step for chroma
	UINT32 wt_y[CTL_IME_ISP_DBCS_WT_LUT_TAB];     ///< Weighting LUT for luma channel, 16 elements, range: [0, 16]
	UINT32 wt_c[CTL_IME_ISP_DBCS_WT_LUT_TAB];     ///< Weighting LUT for luma channel, 16 elements, range: [0, 16]
} CTL_IME_ISP_DBCS;

/**
    KDRV IME enum - YCbCr converter selection
*/
typedef enum {
	CTL_IME_ISP_YCC_CVT_BT601 = 0,  ///< BT.601
	CTL_IME_ISP_YCC_CVT_BT709 = 1,  ///< BT.709
	ENUM_DUMMY4WORD(CTL_IME_ISP_YCC_CVT_SEL)
} CTL_IME_ISP_YCC_CVT_SEL;

/**
    IME enum - An option of update mode in motion estimation
*/
typedef enum {
	CTL_IME_ISP_ME_UPDATE_RAND  = 0,
	CTL_IME_ISP_ME_UPDATE_FIXED = 1,
	ENUM_DUMMY4WORD(CTL_IME_ISP_TMNR_ME_UPDATE_MODE)
} CTL_IME_ISP_TMNR_ME_UPDATE_MODE;

/**
    IME enum - Option for SAD statistic data
*/
typedef enum {
	CTL_IME_ISP_ME_SDA_TYPE_NATIVE = 0,         ///< native SAD
	CTL_IME_ISP_ME_SDA_TYPE_COMPENSATED = 1,   ///< compensated SAD
	ENUM_DUMMY4WORD(CTL_IME_ISP_TMNR_ME_SDA_TYPE)
} CTL_IME_ISP_TMNR_ME_SDA_TYPE;

/**
    IME enum - Method option of motion vector down-sampling
*/
typedef enum {
	CTL_IME_ISP_MV_DOWN_SAMPLE_NEAREST  = 0,
	CTL_IME_ISP_MV_DOWN_SAMPLE_AVERAGE  = 1,
	CTL_IME_ISP_MV_DOWN_SAMPLE_MINIMUM  = 2,
	CTL_IME_ISP_MV_DOWN_SAMPLE_MAXIMUM  = 3,
	ENUM_DUMMY4WORD(CTL_IME_ISP_TMNR_MV_DOWN_SAMPLE_MODE)
} CTL_IME_ISP_TMNR_MV_DOWN_SAMPLE_MODE;

typedef enum {
	CTL_IME_ISP_MV_INFO_MODE_AVERAGE  = 0,
	CTL_IME_ISP_MV_INFO_MODE_LPF      = 1,
	CTL_IME_ISP_MV_INFO_MODE_MINIMUM  = 2,
	CTL_IME_ISP_MV_INFO_MODE_MAXIMUM  = 3,
	ENUM_DUMMY4WORD(CTL_IME_ISP_TMNR_MV_INFO_MODE)
} CTL_IME_ISP_TMNR_MV_INFO_MODE;

/**
    IME enum - Method option of patch selection
*/
typedef enum {
	CTL_IME_ISP_PS_MODE_0 = 0,   ///< determined by object type
	CTL_IME_ISP_PS_MODE_1 = 1,   ///< determined by a LUT mode
	ENUM_DUMMY4WORD(CTL_IME_ISP_TMNR_PS_MODE)
} CTL_IME_ISP_TMNR_PS_MODE;

/**
    IME enum - Strength option for Y-channel pre-filter
*/
typedef enum {
	CTL_IME_ISP_PRE_FILTER_Y_STR_DISABLE = 0,	///< off
	CTL_IME_ISP_PRE_FILTER_Y_STR_1 = 1,			///< type 1 filter
	CTL_IME_ISP_PRE_FILTER_Y_STR_2 = 2,			///< type 2 filter
	ENUM_DUMMY4WORD(CTL_IME_ISP_TMNR_PRE_FILTER_Y_STR)
} CTL_IME_ISP_TMNR_PRE_FILTER_Y_STR;

typedef enum {
	CTL_IME_ISP_PRE_FILTER_TYPE_DISABLE = 0,	///< off
	CTL_IME_ISP_PRE_FILTER_TYPE_1 = 1,			///< 3x3
	CTL_IME_ISP_PRE_FILTER_TYPE_2 = 2,			///< 3x3
	CTL_IME_ISP_PRE_FILTER_TYPE_3 = 3,			///< 5x5
	ENUM_DUMMY4WORD(CTL_IME_ISP_TMNR_PRE_FILTER_TYPE)
} CTL_IME_ISP_TMNR_PRE_FILTER_TYPE;

/**
    KDRV IME structure - YCbCr converter parameters
*/
typedef struct {
	BOOL enable;					///< YCbCrconverter function enable/disable
	CTL_IME_ISP_YCC_CVT_SEL cvt_sel;	///< YCbCr converter selection, BT.601 or BT.709
} CTL_IME_ISP_YCC_CVT;

/**
    IME structure - 3DNR motion estimation parameters
*/
typedef struct {
	CTL_IME_ISP_TMNR_ME_UPDATE_MODE update_mode;	///< An option of update mode in motion estimation. Range 0~1
	BOOL boundary_set;								///< Set image boundary as still type or not.  Range 0~1
	CTL_IME_ISP_TMNR_MV_DOWN_SAMPLE_MODE ds_mode;	///< Method option of motion vector down-sampling.Range 0~3
	CTL_IME_ISP_TMNR_ME_SDA_TYPE sad_type;			///< Option for SAD statistic data.  Range 0~1
	UINT32 sad_shift;								///< Right shift number for SAD statistic data, range = [0, 15]
	UINT8 cost_blend;								///< Cost blending ratio, 0 for automatic cost

	UINT32 rand_bit_x; ///<range 1~7
	UINT32 rand_bit_y; ///<range 1~7
	UINT32 min_detail; ///< range 0~16383

	UINT32 sad_penalty[CTL_IME_ISP_TMNR_ME_SAD_PENALTY_TAB];		///< range 0~1023
	UINT32 switch_th[CTL_IME_ISP_TMNR_ME_SWITCH_THRESHOLD_TAB];		///< range 0~255
	UINT32 switch_rto;												///< range 0~255
	UINT32 detail_penalty[CTL_IME_ISP_TMNR_ME_DETAIL_PENALTY_TAB];	///< range 0~15
	UINT32 probability[CTL_IME_ISP_TMNR_ME_PROBABILITY_TAB];		///< range 0~1
} CTL_IME_ISP_TMNR_ME;

/**
    IME structure - 3DNR motion detection parameters
*/
typedef struct {
	UINT32 sad_coefa[CTL_IME_ISP_TMNR_MD_SAD_COEFA_TAB];	///< Edge coefficient of motion detection. Range 0~63
	UINT32 sad_coefb[CTL_IME_ISP_TMNR_MD_SAD_COEFB_TAB];	///< Offset of motion detection. Range 0~16383
	UINT32 sad_std[CTL_IME_ISP_TMNR_MD_SAD_STD_TAB];		///< Standard deviation of motion detection. Range 0~16383
	UINT32 fth[CTL_IME_ISP_TMNR_MD_FINAL_THRESHOLD_TAB];	///< final threshold of motion detection. Range 0~63
} CTL_IME_ISP_TMNR_MD;

/**
    IME structure - 3DNR motion detection parameters for ROI
*/
typedef struct {
	UINT32 fth[CTL_IME_ISP_TMNR_MD_ROI_FINAL_THRESHOLD_TAB];	///< final threshold of motion detection.Range 0~63
} CTL_IME_ISP_TMNR_MD_ROI;

/**
    IME structure - 3DNR motion compensation parameters
*/
typedef struct {
	UINT32 sad_base[CTL_IME_ISP_TMNR_MC_SAD_BASE_TAB];		///< base level of noise. Range 0~16383
	UINT32 sad_coefa[CTL_IME_ISP_TMNR_MC_SAD_COEFA_TAB];	///< edge coefficient of motion compensation.Range 0~63
	UINT32 sad_coefb[CTL_IME_ISP_TMNR_MC_SAD_COEFB_TAB];	///< offset of motion compensation.Range 0~16383
	UINT32 sad_std[CTL_IME_ISP_TMNR_MC_SAD_STD_TAB];		///< standard deviation of motion compensation.Range 0~16383
	UINT32 fth[CTL_IME_ISP_TMNR_MC_FINAL_THRESHOLD_TAB];	///< final threshold of motion compensation. Range 0~63
} CTL_IME_ISP_TMNR_MC;

/**
    IME structure - 3DNR motion compensation parameters for ROI
*/
typedef struct {
	UINT32 fth[CTL_IME_ISP_TMNR_MC_ROI_FINAL_THRESHOLD_TAB];	///< final threshold of motion compensation.Range 0~63
} CTL_IME_ISP_TMNR_MC_ROI;

/**
    IME structure - 3DNR patch selection parameters
*/
typedef struct {
	BOOL smart_roi_ctrl_en;				///< ROI control. Range 0~1
	BOOL mv_check_en;					///< MV checkiong process in PS module. Range 0~1
	BOOL roi_mv_check_en;				///< MV checking process for ROI. Range 0~1
	CTL_IME_ISP_TMNR_PS_MODE ps_mode;	///< Method option of patch selection. Range 0~1
	CTL_IME_ISP_TMNR_MV_INFO_MODE mv_info_mode;				///< Mode option for MV length calculation. Range 0~3
	UINT32 mv_th;											///< MV threshold. Range 0~63
	UINT32 roi_mv_th;										///< MV threshold for ROI. Range 0~63
	UINT32 mix_ratio[CTL_IME_ISP_TMNR_PS_MIX_RATIO_TAB];	///< Mix ratio in patch selection. Range 0~255
	UINT32 mix_th[CTL_IME_ISP_TMNR_PS_MIX_THRESHOLD_TAB];	///< Mix threshold in patch selection. Range 0~16383.
	UINT32 ds_th;											///< Threshold of motion status down-sampling. Range 0~31
	UINT32 ds_th_roi;										///< Threshold for motion status down-sampling for ROI.Range 0~31
	UINT32 edge_wet;										///< Start point of edge adjustment. Range 0~255
	UINT32 edge_th[CTL_IME_ISP_TMNR_PS_EDGE_THRESHOLD_TAB];	///< Edge adjustment threshold in patch selection. Range 0~16383
	UINT32 fs_th;											////< Threshold of patch error. Range 0~16383.
} CTL_IME_ISP_TMNR_PS;

/**
    IME structure - 3DNR noise filter parameters
*/
typedef struct {
	BOOL luma_ch_en;    ///< noise filter enable for luma channel. Range 0~1
	BOOL chroma_ch_en;  ///< noise filter enable for chroma channel.Range 0~1

	BOOL center_wzeros_y;	///< Set weighting as 0 to the center pixel in internal NR-filter. Range 0~1
	BOOL chroma_fsv_en;		///< False color control.Range 0~1
	UINT32 chroma_fsv;		///< False color value. Range 0~255

	UINT8 luma_residue_th[CTL_IME_ISP_TMNR_NR_LUMA_RESIDUE_TH_TAB]; ///< Protection threshold of luma channel. Range 0~255
	UINT32 chroma_residue_th;	///< Protection threshold of luma channel. Range 0~255

	UINT32 freq_wet[CTL_IME_ISP_TMNR_NR_FREQ_WEIGHT_TAB]; ///< Filter weighting for low frequency. Range 0~255
	UINT32 luma_wet[CTL_IME_ISP_TMNR_NR_LUMA_WEIGHT_TAB]; ///< Filter intensity weighting. Range 0~255

	CTL_IME_ISP_TMNR_PRE_FILTER_Y_STR pre_y_blur_str;					///< Strength option for luma channel pre-filter.Range 0~2
	CTL_IME_ISP_TMNR_PRE_FILTER_TYPE pf_type;						///< pre filter kernel strength select

	UINT32 pre_filter_str[CTL_IME_ISP_TMNR_NR_PRE_FILTER_STRENGTH_TAB];	///< Strength of pre-filtering for low frequency. Range 0~255
	UINT32 pre_filter_rto[CTL_IME_ISP_TMNR_NR_PRE_FILTER_RATION_TAB];	///< adjustment ratio of pre-filtering for transitional object.Range 0~255

	UINT32 snr_str[CTL_IME_ISP_TMNR_NR_SFILTER_STRENGTH_TAB];	///< Strength of spatial filter for still object.Range 0~255
	UINT32 tnr_str[CTL_IME_ISP_TMNR_NR_TFILTER_STRENGTH_TAB];	///< Strength of temporal filter for still object.Range 0~255

	UINT32 snr_base_th; ///< Base threshold of spatial noise reduction.Range 0~65535
	UINT32 tnr_base_th; ///< Base threshold of spatial noise reduction.Range 0~65535

	UINT32 luma_3d_lut[CTL_IME_ISP_TMNR_NR_LUMA_LUT_TAB];		///< Noise reduction LUT for luma channel.Range 0~127
	UINT32 luma_3d_rto[CTL_IME_ISP_TMNR_NR_LUMA_RATIO_TAB];		///< Adjustment ratio 0 of noise reduction LUT for luma channel.Range 0~255
	UINT32 chroma_3d_lut[CTL_IME_ISP_TMNR_NR_CHROMA_LUT_TAB];	///< Noise reduction LUT for chroma channel.Range 0~127
	UINT32 chroma_3d_rto[CTL_IME_ISP_TMNR_NR_CHROMA_RATIO_TAB];	///< Adjustment ratio 0 of noise reduction LUT for chroma channel.Range 0~255

	UINT8 tf0_blur_str[CTL_IME_ISP_TMNR_NR_TF0_FILTER_TAB];
	UINT8 tf0_y_str[CTL_IME_ISP_TMNR_NR_TF0_FILTER_TAB];
	UINT8 tf0_c_str[CTL_IME_ISP_TMNR_NR_TF0_FILTER_TAB];
} CTL_IME_ISP_TMNR_NR;

typedef struct {
	BOOL en;
	UINT8 start_point;
	UINT8 step_size;
} CTL_IME_ISP_TMNR_FCVG;

/**
    KDRV IME structure - TMNR info.
*/
typedef struct {
	BOOL enable;
	CTL_IME_ISP_TMNR_ME me_param;
	CTL_IME_ISP_TMNR_MD md_param;
	CTL_IME_ISP_TMNR_MD_ROI md_roi_param;
	CTL_IME_ISP_TMNR_MC mc_param;
	CTL_IME_ISP_TMNR_MC_ROI mc_roi_param;
	CTL_IME_ISP_TMNR_PS ps_param;
	CTL_IME_ISP_TMNR_NR nr_param;
	CTL_IME_ISP_TMNR_FCVG fcvg_param;
} CTL_IME_ISP_TMNR;

/**
    IME structure - 3DNR debug parameters
*/
typedef struct {
	BOOL dbg_mv0;       ///< debug for mv = 0
	UINT32 dbg_mode;    ///< debug mode selection. Range 0~8
} CTL_IME_ISP_TMNR_DBG;

/**
    Struct IME LCA Gray Statistical Setting
*/
typedef struct {
	UINT8 u_th0;           ///< Gray Statistical threshold for U channel
	UINT8 u_th1;           ///< Gray Statistical threshold for U channel
	UINT8 v_th0;           ///< Gray Statistical threshold for V channel
	UINT8 v_th1;           ///< Gray Statistical threshold for V channel
} CTL_IME_ISP_LCA_GRAY_STATIST;

/**
    Struct IME LCA Reference Center Computation Setting
*/
typedef struct {
	UINT8 rng_th[CTL_IME_ISP_LCA_RANGE_TH_TAB];     ///< UINT32[3], Range filter threshold for reference center
	UINT8 rng_wt[CTL_IME_ISP_LCA_RANGE_WT_TAB];     ///< UINT32[4], Range filter weighting for reference center
	UINT8 cent_wt;                               ///< Reference center weighting
	UINT8 outl_dth;                              ///< Outlier difference threshold
	UINT8 outl_th;                               ///< Reference center outlier threshold
} CTL_IME_ISP_LCA_REFCENT_PARAM;

/**
    Struct IME LCA Reference Center Computation Setting for Y/UV channel
*/
typedef struct {
	CTL_IME_ISP_LCA_REFCENT_PARAM  refcent_y;        ///< for Y channel
	CTL_IME_ISP_LCA_REFCENT_PARAM  refcent_uv;       ///< for UV channel
} CTL_IME_ISP_LCA_REFCENT;

/**
    IME LCA Filter Size Selection
*/
typedef enum {
	CTL_IME_ISP_LCA_FLTR_SIZE_3x3 = 0,             ///< 3x3 window
	CTL_IME_ISP_LCA_FLTR_SIZE_5x5 = 1,             ///< 5x5 window
	CTL_IME_ISP_LCA_FLTR_SIZE_7x7 = 2,             ///< 7x7 window
	CTL_IME_ISP_LCA_FLTR_SIZE_9x9 = 3,             ///< 9x9 window
	ENUM_DUMMY4WORD(CTL_IME_ISP_LCA_FILT_SIZE)
} CTL_IME_ISP_LCA_FILT_SIZE;

/**
    Struct IME LCA Edge Direction Threshold
*/
typedef struct {
	UINT8  hv_th;                  ///< Edge direction threshold for two diagonal directions
	UINT8  pn_th;                  ///< Edge direction threshold for horizontal and vertical directions
} CTL_IME_ISP_LCA_EDG_DIR_TH_PARAM;

/**
    IME LCA Edge Kernel Filter Size Selection
*/
typedef enum {
	CTL_IME_ISP_LCA_EKNL_SIZE_3x3 = 0,             ///< 3x3 window
	CTL_IME_ISP_LCA_EKNL_SIZE_5x5 = 1,             ///< 5x5 window
	ENUM_DUMMY4WORD(CTL_IME_ISP_LCA_EDGE_KER_SIZE)
} CTL_IME_ISP_LCA_EDGE_KER_SIZE;

/**
    IME LCA Filter Computation Setting
*/
typedef struct {
	UINT8 filt_th[CTL_IME_ISP_LCA_FILTER_TH_TAB];    ///< UINT32[5], Filter threshold LUT
	UINT8 filt_wt[CTL_IME_ISP_LCA_FILTER_WT_TAB];    ///< UINT32[6], Filter weighting LUT
} CTL_IME_ISP_LCA_FILTER_PARAM;

/**
    IFE2_FILTER
*/
typedef struct {
	BOOL enable;									///< filter Y channel enable
	CTL_IME_ISP_LCA_FILT_SIZE size;					///< filter size
	CTL_IME_ISP_LCA_EDG_DIR_TH_PARAM edg_dir;		///< filter edge direction threshold
	CTL_IME_ISP_LCA_EDGE_KER_SIZE edg_ker_size;		///< edge kernel size
	CTL_IME_ISP_LCA_FILTER_PARAM set_y;				///< Filter parameters for Y Channel
	CTL_IME_ISP_LCA_FILTER_PARAM set_u;				///< Filter parameters for U Channel
	CTL_IME_ISP_LCA_FILTER_PARAM set_v;				///< Filter parameters for V Channel
} CTL_IME_ISP_LCA_FILTER;

/**
    KDRV IME all IQ parameters
*/
typedef struct {
	CTL_IME_ISP_CMF					*p_cmf;
	CTL_IME_ISP_LCA					*p_lca;
	CTL_IME_ISP_DBCS				*p_dbcs;
	CTL_IME_ISP_SSR					*p_ssr;
	CTL_IME_ISP_FGN					*p_fgn;
	CTL_IME_ISP_TMNR				*p_tmnr;
	CTL_IME_ISP_YCC_CVT				*p_ycccvt;
	CTL_IME_ISP_TMNR_DBG			*p_tmnr_dbg;
	CTL_IME_ISP_LCA_GRAY_STATIST	*p_lca_gray_stl;
	CTL_IME_ISP_LCA_REFCENT			*p_lca_ref_cent;
	CTL_IME_ISP_LCA_FILTER			*p_lca_filter;
} CTL_IME_ISP_IQ_ALL;

#endif
