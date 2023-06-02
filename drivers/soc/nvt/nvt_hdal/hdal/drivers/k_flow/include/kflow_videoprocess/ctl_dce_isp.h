/**
    IPL Ctrl Layer, ISP Interface

    @file       CTL_DCE_ISP_isp.h
    @ingroup    mIIPL_Ctrl
    @note       None

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/


#ifndef _CTL_DCE_ISP_ISP_H
#define _CTL_DCE_ISP_ISP_H

#define CTL_DCE_ISP_IMG_MAX_STP_NUM         16
#define CTL_DCE_ISP_PARTITION_NUM           16
#define CTL_DCE_ISP_GDC_TABLE_NUM           65
#define CTL_DCE_ISP_CAC_TABLE_NUM           65
#define CTL_DCE_ISP_CFA_FREQ_NUM            16
#define CTL_DCE_ISP_CFA_LUMA_NUM            17
#define CTL_DCE_ISP_CFA_FCS_NUM             16
#define CTL_DCE_ISP_LUT2D_MAX_TABLE_SIZE    (65*65)
#define CTL_DCE_ISP_WDR_SUBIMG_FILT_NUM     3
#define CTL_DCE_ISP_WDR_COEF_NUM            4
#define CTL_DCE_ISP_WDR_INPUT_BLD_NUM       17
#define CTL_DCE_ISP_WDR_NEQ_TABLE_IDX_NUM   32
#define CTL_DCE_ISP_WDR_NEQ_TABLE_VAL_NUM   65
#define CTL_DCE_ISP_HISTOGRAM_STCS_NUM      128

/**
    DCE Stripe Type
*/
typedef enum {
	CTL_DCE_ISP_STRIPE_AUTO = 0,           //Auto calculation
	CTL_DCE_ISP_STRIPE_MANUAL_MULTI,       //Force mult-stripe
	CTL_DCE_ISP_STRIPE_MANUAL_2STRIPE,     //Force 2 stripe
	CTL_DCE_ISP_STRIPE_MANUAL_3STRIPE,     //Force 3 stripe
	CTL_DCE_ISP_STRIPE_MANUAL_4STRIPE,     //Force 4 stripe
	CTL_DCE_ISP_STRIPE_MANUAL_5STRIPE,     //Force 5 stripe
	CTL_DCE_ISP_STRIPE_MANUAL_6STRIPE,     //Force 6 stripe
	CTL_DCE_ISP_STRIPE_MANUAL_7STRIPE,     //Force 7 stripe
	CTL_DCE_ISP_STRIPE_MANUAL_8STRIPE,     //Force 8 stripe
	CTL_DCE_ISP_STRIPE_MANUAL_9STRIPE,     //Force 9 stripe
	CTL_DCE_ISP_STRIPE_CUSTOMER,           //config stripe setting by user
	ENUM_DUMMY4WORD(CTL_DCE_ISP_STRIPE_TYPE)
} CTL_DCE_ISP_STRIPE_TYPE;


typedef enum {
	CTL_DCE_ISP_FOV_BND_DUPLICATE = 0,     ///< Replace out of boundary pixels with duplicate boundary pixels
	CTL_DCE_ISP_FOV_BND_REG_RGB = 1,       ///< Replace out of boundary pixels with GEO_BOUNDR, GEO_BOUNDG, GEO_BOUNDB
	ENUM_DUMMY4WORD(CTL_DCE_ISP_FOV_BND_SEL)
} CTL_DCE_ISP_FOV_BND_SEL;

typedef enum {
	CTL_DCE_ISP_2DLUT_65_65_GRID = 0,      ///< 65x65 grid 2D lut distortion correction
	CTL_DCE_ISP_2DLUT_33_33_GRID = 1,      ///< 33x33 grid 2D lut distortion correction
	CTL_DCE_ISP_2DLUT_17_17_GRID = 2,      ///< 17x17 grid 2D lut distortion correction
	CTL_DCE_ISP_2DLUT_9_9_GRID = 3,        ///< 9x9 grid 2D lut distortion correction
	ENUM_DUMMY4WORD(CTL_DCE_ISP_2DLUT_NUM)
} CTL_DCE_ISP_2DLUT_NUM;

typedef enum {
	CTL_DCE_ISP_CGAIN_2_8 = 0,     ///< gain range integer/fraction 2.8
	CTL_DCE_ISP_CGAIN_3_7 = 1,     ///< gain range integer/fraction 3.7
	ENUM_DUMMY4WORD(CTL_DCE_ISP_CGAIN_RANGE)
} CTL_DCE_ISP_CGAIN_RANGE;

typedef enum {
	CTL_DCE_ISP_PINKR_MOD_G  = 0,   ///< modify G channel
	CTL_DCE_ISP_PINKR_MOD_RB = 1,   ///< modify RB channel
	ENUM_DUMMY4WORD(CTL_DCE_ISP_PINKR_MODE)
} CTL_DCE_ISP_PINKR_MODE;

typedef enum {
	CTL_DCE_ISP_XY_BOTH = 0,           ///< processing both X and Y
	CTL_DCE_ISP_X_ONLY  = 1,           ///< processing X only
	ENUM_DUMMY4WORD(CTL_DCE_ISP_GDC_LUT_MODE)
} CTL_DCE_ISP_GDC_LUT_MODE;

typedef enum {
	CTL_DCE_ISP_INPUT_BLD_3x3_Y  = 0,   ///< use Y from bayer 3x3 as the input blending source selection
	CTL_DCE_ISP_INPUT_BLD_GMEAN  = 1,   ///< use Gmean from bayer 3x3 as the input blending source selection
	CTL_DCE_ISP_INPUT_BLD_BAYER  = 2,   ///< use Bayer as the input blending source selection
	ENUM_DUMMY4WORD(CTL_DCE_ISP_WDR_INPUT_BLDSEL)
} CTL_DCE_ISP_WDR_INPUT_BLDSEL;

typedef enum {
	CTL_DCE_ISP_BEFORE_WDR      = 0,    ///< histogram statistics before wdr algorithm
	CTL_DCE_ISP_AFTER_WDR       = 1,    ///< histogram statistics after wdr algorithm
	ENUM_DUMMY4WORD(CTL_DCE_ISP_HIST_SEL)
} CTL_DCE_ISP_HIST_SEL;

typedef enum {
	CTL_DCE_ISP_STRP_LOW_LAT_HIGH_PRIOR = 0,   ///< stripe configuration for low latency best performance
	CTL_DCE_ISP_STRP_GDC_HIGH_PRIOR = 1,       ///< stripe configuration for GDC + latency both running
	CTL_DCE_ISP_STRP_2DLUT_HIGH_PRIOR = 2,     ///< stripe configuration for 2DLUT best performance
	CTL_DCE_ISP_STRP_GDC_BEST = 3,     		///< stripe configuration for GDC best performance
	ENUM_DUMMY4WORD(CTL_DCE_ISP_STRP_RULE_SEL)
} CTL_DCE_ISP_STRP_RULE_SEL;


//----------------------------------------------------------------------

typedef struct {
	UINT32 geo_center_x;                    //Geometric distortion X center
	UINT32 geo_center_y;                    //Geometric distortion Y center
} CTL_DCE_ISP_GDC_CENTER_INFO;


/**
    DCE Stripe Information For IME Reference
*/
typedef struct {
	CTL_DCE_ISP_STRIPE_TYPE stripe_type;   //assign stripe type for driver calculation or user-assigned value
	UINT32 hstp[CTL_DCE_ISP_PARTITION_NUM];     //stripe width of DCE output partitions
} CTL_DCE_ISP_STRIPE;

//----------------------------------------------------------------------

/**
    DCE CTL CFA direction decision
*/
typedef struct {
	UINT32  diff_norm_bit;          //Difference norm bit, range [0~3]
	UINT32  ns_mar_edge;            //Noise margin for edge, range [0~1023]
	UINT32  ns_mar_diff;            //Noise margin for pixel difference, range [0~1023]
} CTL_DCE_ISP_CFA_DIR;

/**
    DCE CTL CFA correction term
*/
typedef struct {
	//BOOL    g_corr_enable;        //Enable for G channel correction
	BOOL    rb_corr_enable;         //Enable for RB channel correction
	//UINT32  g_corr_norm_bit;      //G correction norm bit, range [0~3]
	//UINT32  g_corr_th;            //G correction noise threshold, range [0~1023]
	UINT32  rb_corr_th1;            //RB correction noise threshold1, range [0~1023]
	UINT32  rb_corr_th2;            //RB correction noise threshold2, range [0~1023]
} CTL_DCE_ISP_CFA_CORR;

/**
    DCE CTL CFA high frequency check term
*/
typedef struct {
	BOOL    cl_check_enable;        ///< Enable cross line check
	BOOL    hf_check_enable;        ///< Enable high frequency check
	BOOL    average_mode;           ///< Interpolation method selection when flat region detect
	BOOL    cl_sel;                 ///< operation type when cross line is not found
	UINT32  cl_th;                  ///< Cross line checking threshold, range [0~255]
	UINT32  hfg_th;                 ///< G channel smoothness checking threshold, range [0~7]
	UINT32  hf_diff;                ///< Difference threshold value between Gr and Gb to determing high frequency pattern detected or not, range [0~255]
	UINT32  hfe_th;                 ///< Threshold to determine H or V direction high frequency pattern detected, range [0~255]
	UINT32  ir_g_edge_th;           ///< Noise margin for edge in Green component interpolation [0~1023]
	UINT32  ir_rb_cstrength;        ///< RB channel smooth strength [0~7]

} CTL_DCE_ISP_CFA_HFC;

/**
    DCE structure - CFA interpolation term
*/
typedef struct {
	UINT32    edge_dth;                         ///< Edge threshold 1 for Bayer CFA interpolation [0~4095]
	UINT32    edge_dth2;                        ///< Edge threshold 2 for Bayer CFA interpolation [0~4095]
	UINT32    freq_th;                          ///< CFA frequency threshold [0~4095]
	UINT32    freq_lut[CTL_DCE_ISP_CFA_FREQ_NUM];      ///< CFA frequency blending weight look-up table [0~15]
	UINT32    luma_wt[CTL_DCE_ISP_CFA_LUMA_NUM];       ///< Luma gain for CFA [0~255]
} CTL_DCE_ISP_CFA_INTERP;

/**
    DCE structure - CFA false color suppression term
*/
typedef struct {
	BOOL      fcs_dirsel;                       ///< Direction selection for detecting high frequency, 0: detect 2 directions, 1: detect 4 directions
	UINT32    fcs_coring;                       ///< Coring threshold for false color suppression [0~255]
	UINT32    fcs_weight;                       ///< Global strength for false color suppression [0~255]
	UINT32    fcs_strength[CTL_DCE_ISP_CFA_FCS_NUM];   ///< CFA false color suppression strength [0~15]
} CTL_DCE_ISP_CFA_FCS;

/**
    DCE structure - CFA IR subtraction term
*/
typedef struct {
	UINT32    ir_sub_r;                         ///< R channel IR sub [0~4095]
	UINT32    ir_sub_g;                         ///< G channel IR sub [0~4095]
	UINT32    ir_sub_b;                         ///< B channel IR sub [0~4095]
	UINT32    ir_sub_wt_lb;                     ///< Lower bound of IR local weighting [0~255]
	UINT32    ir_sub_th;                        ///< Threshold to decay IR local weighting [0~255]
	UINT32    ir_sub_range;                     ///< Range to decay IR local weighting [0~3]
	UINT32    ir_sat_gain;                      ///< Saturation gain multiplied after IR sub [0~1023]
} CTL_DCE_ISP_CFA_IR;

/**
    DCE structure - CFA color gain term
*/
typedef struct {
	UINT32    r_gain;                           ///< R channel gain [0~1023]
	UINT32    g_gain;                           ///< G channel gain [0~1023]
	UINT32    b_gain;                           ///< B channel gain [0~1023]
	CTL_DCE_ISP_CGAIN_RANGE gain_range;            ///< select fraction type of gain: 2.8 or 3.7
} CTL_DCE_ISP_CFA_CGAIN;

/**
    DCE structure - CFA pink reduction term
*/
typedef struct {
	BOOL pink_rd_en;                            ///< pink reduction enable
	CTL_DCE_ISP_PINKR_MODE pink_rd_mode;           ///< pink reduction mode
	UINT32    pink_rd_th1;                      ///< pink reduction threshold 1 [0~255]
	UINT32    pink_rd_th2;                      ///< pink reduction threshold 2 [0~255]
	UINT32    pink_rd_th3;                      ///< pink reduction threshold 3 [0~255]
	UINT32    pink_rd_th4;                      ///< pink reduction threshold 4 [0~255]
} CTL_DCE_ISP_CFA_PINKR;

/**
    DCE structure - DCE CFA parameters.
*/
//@{
typedef struct {
	BOOL cfa_enable;
	CTL_DCE_ISP_CFA_INTERP cfa_interp;             ///< CFA interpolation term
	CTL_DCE_ISP_CFA_CORR cfa_correction;           ///< CFA correction term
	CTL_DCE_ISP_CFA_FCS cfa_fcs;                   ///< CFA false color suppression term
	CTL_DCE_ISP_CFA_HFC cfa_ir_hfc;                ///< CFA high frequency check term
	CTL_DCE_ISP_CFA_IR cfa_ir_sub;                 ///< CFA IR subtraction term
	CTL_DCE_ISP_CFA_PINKR cfa_pink_reduc;          ///< CFA pink reduction term
	CTL_DCE_ISP_CFA_CGAIN cfa_cgain;               ///< CFA color gain term
} CTL_DCE_ISP_CFA;


//----------------------------------------------------------------------

/**
    DCE CTL Distortion Correction Parameters of FOV boundary replacement
*/
typedef struct {
	UINT32 fov_gain;                    ///< 1024 is 1x, Scale down factor on corrected image (12 bit: 2.10)
	CTL_DCE_ISP_FOV_BND_SEL fov_bnd_sel;   ///< Select out-of-bound pixel source
	UINT32 fov_bnd_r;                   ///< Replace value, range [0~1023]
	UINT32 fov_bnd_g;                   ///< Replace value, range [0~1023]
	UINT32 fov_bnd_b;                   ///< Replace value, range [0~1023]
} CTL_DCE_ISP_FOV;

//----------------------------------------------------------------------

/**
    DCE CTL Distortion Correction Param and Enable/Disable
*/
typedef struct {
	UINT32 geo_dist_x;                  ///< X input distance factor
	UINT32 geo_dist_y;                  ///< Y input distance factor
	UINT32 geo_lut_g[CTL_DCE_ISP_GDC_TABLE_NUM];    ///< 65 x 16bit
} CTL_DCE_ISP_DC;

/**
    DCE CTL Chromatic Aberration Correction Param and Enable/Disable
*/
typedef struct {
	BOOL cac_enable;
	UINT32 geo_r_lut_gain;              ///< 4096 is 1x, range [0~8191]
	UINT32 geo_g_lut_gain;              ///< 4096 is 1x, range [0~8191]
	UINT32 geo_b_lut_gain;              ///< 4096 is 1x, range [0~8191]
	INT32 geo_lut_r[CTL_DCE_ISP_CAC_TABLE_NUM];     ///< 65 x 16bit
	INT32 geo_lut_b[CTL_DCE_ISP_CAC_TABLE_NUM];     ///< 65 x 16bit
	BOOL cac_sel;                       ///< CAC use 0:G_Lut & RGB Lut Gain, 1: RGB Lut
} CTL_DCE_ISP_CAC;

/**
    DCE CTL DC and CAC Param
*/
typedef struct {
	BOOL dc_enable;
	CTL_DCE_ISP_GDC_LUT_MODE dc_y_dist_off;
	CTL_DCE_ISP_DC dc;
	CTL_DCE_ISP_CAC cac;
} CTL_DCE_ISP_DC_CAC;

//----------------------------------------------------------------------

/**
    DCE CTL 2D Lut Distortion Correction Param and Enable/Disable
*/
typedef struct {
	BOOL lut_2d_enable;
	CTL_DCE_ISP_GDC_LUT_MODE lut_2d_y_dist_off;
	CTL_DCE_ISP_2DLUT_NUM lut_num_select;
	UINT32 lut_2d_value[CTL_DCE_ISP_LUT2D_MAX_TABLE_SIZE];
} CTL_DCE_ISP_2DLUT;


//----------------------------------------------------------------------

/**
    DCE structure - WDR non-equal table parameters
*/
typedef struct {
	UINT32 lut_idx[CTL_DCE_ISP_WDR_NEQ_TABLE_IDX_NUM];      ///< non equatable table index [0~63]
	UINT32 lut_split[CTL_DCE_ISP_WDR_NEQ_TABLE_IDX_NUM];    ///< non equatable table split number [0~3]
	UINT32 lut_val[CTL_DCE_ISP_WDR_NEQ_TABLE_VAL_NUM];      ///< non equatable table value [0~4095]
} CTL_WDR_NEQ_TABLE;

/**
    DCE structure - WDR input blending parameters
*/
typedef struct {
	CTL_DCE_ISP_WDR_INPUT_BLDSEL bld_sel;
	UINT32 bld_wt;                              ///< blending of Y and localmax [0~8],0: Y, 8: local max
	UINT32 blend_lut[CTL_DCE_ISP_WDR_INPUT_BLD_NUM];        ///< blending of Y and raw [0~255], 0: Y, 255: raw
} CTL_WDR_IN_BLD;

/**
    DCE structure - WDR strength parameters
*/
typedef struct {
	INT32 wdr_coeff[CTL_DCE_ISP_WDR_COEF_NUM];              ///< wdr coefficient [-4096~4095]
	UINT32 strength;                            ///< wdr strength [0~255]
	UINT32 contrast;                            ///< wdr contrast [0~255], 128 = original
} CTL_WDR_STRENGTH;

/**
    DCE structure - WDR gain control parameters
*/
typedef struct {
	BOOL gainctrl_en;                           ///< enable gain control for wdr
	UINT32 max_gain;                            ///< maximum gain of wdr [1~255]
	UINT32 min_gain;                            ///< minimum gain of wdr [1~255], mapping to 1/256, 2/256, 255/256
} CTL_WDR_GAINCTRL;

/**
    DCE structure - WDR output blending parameters
*/
typedef struct {
	BOOL outbld_en;                             ///< enable output blending for wdr
	CTL_WDR_NEQ_TABLE outbld_lut;              ///< wdr output blending table
} CTL_WDR_OUTBLD;

/**
    DCE structure - WDR saturation reduction parameters
*/
typedef struct {
	UINT32 sat_th;                              ///< threshold of wdr saturation reduction [0~4095]
	UINT32 sat_wt_low;                          ///< lower weight of wdr saturation reduction [0~255]
	UINT32 sat_delta;                           ///< delta of wdr saturation reduction [0~255]
} CTL_WDR_SAT_REDUCT;

/**
    DCE CTL WDR/TONE CURVE Param and Enable/Disable
*/
typedef struct {
	BOOL wdr_enable;
	BOOL tonecurve_enable;
	UINT32  ftrcoef[CTL_DCE_ISP_WDR_SUBIMG_FILT_NUM];       ///< wdr sub-image low pass filter coefficients, 3 entries
	CTL_WDR_IN_BLD     input_bld;        ///< wdr input blending
	CTL_WDR_NEQ_TABLE  tonecurve;        ///< wdr tonecurve configuration
	CTL_WDR_STRENGTH   wdr_str;          ///< wdr strength
	CTL_WDR_GAINCTRL   gainctrl;         ///< wdr gain control
	CTL_WDR_OUTBLD     outbld;           ///< wdr output blending settings
	CTL_WDR_SAT_REDUCT sat_reduct;       ///< wdr saturation reduction
} CTL_DCE_ISP_WDR;

/**
    DCE WDR subimg size Information
*/
typedef struct {
	UINT32           subimg_size_h;            ///< wdr sub-image size [4~32]
	UINT32           subimg_size_v;            ///< wdr sub-image size [4~32]
	UINT32           subimg_lofs_in;           ///< wdr subin lineoffset, lofs >= (subimg_size_h * 8)
	UINT32           subimg_lofs_out;          ///< wdr subout lineoffset, lofs >= (subimg_size_h * 8)
} CTL_DCE_ISP_WDR_SUBIMG;

/**
    DCE CTL Histogram Param and Enable/Disable
*/
typedef struct {
	BOOL hist_enable;                           ///< Enable histogram statistics
	CTL_DCE_ISP_HIST_SEL hist_sel;                 ///< selection of statistics input
	UINT32 step_h;                              ///< h step size of histogram [0~31]
	UINT32 step_v;                              ///< v step size of histogram [0~31]
} CTL_DCE_ISP_HIST;


//----------------------------------------------------------------------

/**
    CTL DCE enum - function selection
*/
typedef enum {
	CTL_DCE_ISP_IQ_FUNC_CFA          = 0x00000001,
	CTL_DCE_ISP_IQ_FUNC_DC           = 0x00000002,
	CTL_DCE_ISP_IQ_FUNC_CAC          = 0x00000004,
	CTL_DCE_ISP_IQ_FUNC_2DLUT        = 0x00000008,
	CTL_DCE_ISP_IQ_FUNC_FOV          = 0x00000010,
	CTL_DCE_ISP_IQ_FUNC_WDR          = 0x00000020,
	CTL_DCE_ISP_IQ_FUNC_TONECURVE    = 0x00000040,
	CTL_DCE_ISP_IQ_FUNC_HISTOGRAM    = 0x00000080,
	ENUM_DUMMY4WORD(CTL_DCE_ISP_IQ_FUNC)
} CTL_DCE_ISP_IQ_FUNC;


/**
    CTL DCE all IQ parameters
*/
typedef struct {
	CTL_DCE_ISP_CFA          *p_cfa;
	CTL_DCE_ISP_DC_CAC       *p_dc_cac;
	CTL_DCE_ISP_2DLUT        *p_2dlut;
	CTL_DCE_ISP_FOV          *p_fov;
	CTL_DCE_ISP_STRIPE       *p_stripe;
	CTL_DCE_ISP_WDR          *p_wdr;
	CTL_DCE_ISP_WDR_SUBIMG   *p_wdr_subimg;
	CTL_DCE_ISP_HIST         *p_hist;
} CTL_DCE_ISP_IQ_ALL;

#endif

