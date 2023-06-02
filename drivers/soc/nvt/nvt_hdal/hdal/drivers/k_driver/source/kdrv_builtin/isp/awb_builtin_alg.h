#ifndef _AWB_BUILTIN_ALG_H_
#define _AWB_BUILTIN_ALG_H_

#if defined(__KERNEL__) || defined(__FREERTOS)
#include "kwrap/type.h"
#endif

#define AWB_MIN(a, b)       ((a) <= (b) ? (a) : (b))
#define AWB_MAX(a, b)       ((a) >= (b) ? (a) : (b))
#define AWB_CLAMP(x, a, b)  AWB_MAX(AWB_MIN(x, b), a)
#define AWBALG_ABS(A)       (((A) > 0) ? (A) : (-A))

//=============================================================================
// struct & definition
//=============================================================================
#define AWB_TUNING_CT_WEIGHT_MAX 6
#define AWB_TUNING_TARGET_MAX 3
#define AWB_TUNING_CT_MAX 6
#define AWB_TUNING_MWB_MAX 12
#define AWB_TUNING_BLOCK_MAX 4
#define AWB_TUNING_LUMA_WEIGHT_MAX 8
// AWB alg CA definition
#define AWB_WIN 32
#define AWB_CABUFFER_SIZE   AWB_WIN * AWB_WIN

#define AWBALG_KGAIN_BASE 100
#define AWBALG_SMOOTH_WEIGHT_PRE_MAX 200
/**
	AWB alg default Kgain
*/
#define AWB_BASE_KGAIN 100

/**
	AWB id
*/
typedef enum _AWB_ID {
	AWB_ID_1 = 0,       ///< awb id 1
	AWB_ID_2,           ///< awb id 2
	AWB_ID_3,           ///< awb id 3
	AWB_ID_4,           ///< awb id 4
	AWB_ID_5,           ///< awb id 5
	AWB_ID_MAX_NUM,
	ENUM_DUMMY4WORD(AWB_ID)
} AWB_ID;

/**
	AWB Alg Expand TH Mode
*/
typedef enum _AWBALG_EXPAND_TH_MODE {
	AWBALG_EXPAND_TH_OFF    = 0,
	AWBALG_EXPAND_TH_ADD    = 1,
	AWBALG_EXPAND_TH_REMOVE = 2,
	ENUM_DUMMY4WORD(AWBALG_EXPAND_TH_MODE)
} AWBALG_EXPAND_TH_MODE;

/**
	AWB Alg Status
*/
typedef enum _AWB_STS {
	AWBALG_STATUS_INIT          = 0,
	AWBALG_STATUS_ADJUST,
	AWBALG_STATUS_FREEZE,
	AWBALG_STATUS_CONVERGE,
	AWBALG_STATUS_MANUAL,
	AWBALG_STATUS_EXPAND_CT,
	AWBALG_STATUS_DAYLIGHT      = 11,
	AWBALG_STATUS_CLOUDY        = 12,
	AWBALG_STATUS_TUNGSTEN      = 13,
	AWBALG_STATUS_SUNSET        = 14,
	AWBALG_STATUS_CUSTOMER1     = 15,
	AWBALG_STATUS_CUSTOMER2     = 16,
	AWBALG_STATUS_CUSTOMER3     = 17,
	AWBALG_STATUS_CUSTOMER4     = 18,
	AWBALG_STATUS_CUSTOMER5     = 19,
	AWBALG_STATUS_NIGHTMODE     = 20,
	AWBALG_STATUS_MGAIN         = 21,
	AWBALG_STATUS_END           = 0xffffffff,
	ENUM_DUMMY4WORD(AWB_STS)
} AWB_STS;

/**
	AWB Alg channel type
*/
typedef enum _AWBALG_CH {
	AWBALG_CH_R = 0,
	AWBALG_CH_G,
	AWBALG_CH_B,
	AWBALG_CH_IR,
	AWBALG_ACC_CNT,
	AWBALG_CH_MAX,
	ENUM_DUMMY4WORD(AWBALG_CH)
} AWBALG_CH;

/**
	 AWB CA th.
	 @note
*/
typedef struct _AWBALG_CA_THRESHOLD {
	BOOL  enable;
	UINT32 mode;
	UINT16 g_l;			///< G threshold lower bound
	UINT16 g_u;			///< G threshold upper bound
	UINT16 r_l;			///< R threshold lower bound
	UINT16 r_u;			///< R threshold upper bound
	UINT16 b_l;			///< B threshold lower bound
	UINT16 b_u;			///< B threshold upper bound
	UINT16 p_l;			///< P threshold lower bound
	UINT16 p_u;			///< P threshold upper bound
} AWBALG_CA_THRESHOLD;

/**
	 AWB th_m1.

	 @note
*/
typedef struct _AWB_TH {
	INT32 y_l;
	INT32 y_u;
	INT32 rpb_l;
	INT32 rpb_u;
	INT32 rsb_l;
	INT32 rsb_u;
	INT32 r2g_l;
	INT32 r2g_u;
	INT32 b2g_l;
	INT32 b2g_u;
	INT32 rmb_l;
	INT32 rmb_u;
} AWB_TH;

/**
	AWB Alg LV check
*/
typedef struct _AWB_LV {
	UINT32 night_l;
	UINT32 night_h;
	UINT32 in_l;
	UINT32 in_h;
	UINT32 out_l;
	UINT32 out_h;
} AWB_LV;

typedef struct _AWB_CT_WEIGHT {
	UINT32 ctmp[AWB_TUNING_CT_WEIGHT_MAX];
	INT32 cx[AWB_TUNING_CT_WEIGHT_MAX];
	INT32 out_weight[AWB_TUNING_CT_WEIGHT_MAX];
	INT32 in_weight[AWB_TUNING_CT_WEIGHT_MAX];
	INT32 night_weight[AWB_TUNING_CT_WEIGHT_MAX];
} AWB_CT_WEIGHT;
/**
	 AWB target.
*/
typedef struct _AWB_TARGET {
	INT32 cx[AWB_TUNING_TARGET_MAX];
	INT32 rg_ratio[AWB_TUNING_TARGET_MAX];
	INT32 bg_ratio[AWB_TUNING_TARGET_MAX];
} AWB_TARGET;

/**
	AWB Alg CT table
*/
typedef struct _AWB_CT_INFO {
	UINT32 temperature[AWB_TUNING_CT_MAX];
	UINT32 r_gain[AWB_TUNING_CT_MAX];
	UINT32 g_gain[AWB_TUNING_CT_MAX];
	UINT32 b_gain[AWB_TUNING_CT_MAX];
} AWB_CT_INFO;

/**
	AWB Manual WB Gain
*/
typedef struct _AWB_MWB_GAIN {
	UINT32 r_gain[AWB_TUNING_MWB_MAX];
	UINT32 g_gain[AWB_TUNING_MWB_MAX];
	UINT32 b_gain[AWB_TUNING_MWB_MAX];
} AWB_MWB_GAIN;

/**
	AWB Converge
*/
typedef struct _AWB_CONVERGE {
	UINT32 skip_frame;
	UINT32 speed;
	UINT32 tolerance;
} AWB_CONVERGE;

/**
	AWB Expand Block
*/
typedef struct _AWB_EXPAND_BLOCK {
	AWBALG_EXPAND_TH_MODE mode[AWB_TUNING_BLOCK_MAX];
	UINT32 lv_l[AWB_TUNING_BLOCK_MAX];
	UINT32 lv_h[AWB_TUNING_BLOCK_MAX];
	INT32 y_l[AWB_TUNING_BLOCK_MAX];
	INT32 y_u[AWB_TUNING_BLOCK_MAX];
	INT32 rpb_l[AWB_TUNING_BLOCK_MAX];
	INT32 rpb_u[AWB_TUNING_BLOCK_MAX];
	INT32 rsb_l[AWB_TUNING_BLOCK_MAX];
	INT32 rsb_u[AWB_TUNING_BLOCK_MAX];
} AWB_EXPAND_BLOCK;

/**
	AWB Luma Weight
*/
typedef struct _AWB_LUMA_WEIGHT {
	BOOL en;
	INT32 y[AWB_TUNING_LUMA_WEIGHT_MAX];
	INT32 w[AWB_TUNING_LUMA_WEIGHT_MAX];
} AWB_LUMA_WEIGHT;

/**
	AWB Alg stable table
*/
typedef struct _AWBALG_STABLE_GAIN {
	UINT32 r_gain;
	UINT32 g_gain;
	UINT32 b_gain;
	UINT32 score;
	UINT32 tolerance; ///< 256 is 100%
} AWBALG_STABLE_GAIN;

/**
	AWB Parameter
*/
typedef struct _AWB_PARAM {
	AWB_TH           *th;
	AWB_LV           *lv;
	AWB_CT_WEIGHT    *ct_weight;
	AWB_TARGET       *target;
	AWB_CT_INFO      *ct_info;
	AWB_MWB_GAIN     *mwb_gain;
	AWB_CONVERGE     *converge;
	AWB_EXPAND_BLOCK *expand_block;
	AWB_LUMA_WEIGHT  *luma_weight;
} AWB_PARAM;

/**
    AWB Manual Gain
*/
typedef struct _AWB_MANUAL {
	BOOL en;
	UINT32 r_gain;
	UINT32 g_gain;
	UINT32 b_gain;
} AWB_MANUAL;

/**
    AWB KGain Ratio
*/
typedef struct _AWB_KGAIN_RATIO {
	UINT32 r_ratio;
	UINT32 b_ratio;
} AWB_KGAIN_RATIO;

/**
    AWB Current Status
*/
typedef struct _AWB_STATUS {
	UINT32 cur_r_gain;
	UINT32 cur_g_gain;
	UINT32 cur_b_gain;
	UINT32 cur_ct;
	UINT32 cur_cx;
	AWB_STS mode;
	UINT32 reserved[4];
} AWB_STATUS;

/**
    AWB Tuning CA Info
*/
typedef struct _AWB_CA {
	UINT32 win_num_x;
	UINT32 win_num_y;
	UINT16 tab[AWBALG_CH_MAX][AWB_WIN][AWB_WIN];
} AWB_CA;

/**
    AWB Tuning Flag Info
*/
typedef struct _AWB_FLAG {
	UINT32 win_num_x;
	UINT32 win_num_y;
	UINT16 tab[AWB_WIN][AWB_WIN];
} AWB_FLAG;

typedef struct _AWB_MULTI_CT {
	BOOL en;
	UINT32 min_sat;     // range : 0~80
	UINT32 saturation;  // range : 0~100, 1X = 100
} AWB_MULTI_CT;

/**
	 AWB ca type
*/
typedef enum _AWB_CA_TYPE {
	AWB_CA_TYPE_NORMAL = 0,
	AWB_CA_TYPE_THRESHOLD,
	AWB_CA_TYPE_MAX,
	ENUM_DUMMY4WORD(AWB_CA_TYPE)
} AWB_CA_TYPE;

/**
	Color temperature range
*/
typedef enum _AWB_CT_RANGE {
	AWB_CT_RANGE_NONE   = 0x00000000,     ///< not in range
	AWB_CT_RANGE_1      = 0x00000001,     ///< awb ct range 1
	AWB_CT_RANGE_2      = 0x00000002,     ///< awb ct range 2
	AWB_CT_RANGE_3      = 0x00000004,     ///< awb ct range 3
	AWB_CT_RANGE_4      = 0x00000008,     ///< awb ct range 4
	AWB_CT_RANGE_5      = 0x00000010,     ///< awb ct range 5
	AWB_CT_RANGE_MAX    = 0x0000001F,
	ENUM_DUMMY4WORD(AWB_CT_RANGE)
} AWB_CT_RANGE;

/**
	AWB Alg check white type
*/
typedef enum _AWBALG_ER {
	AWBALG_FAIL   = 0,
	AWBALG_OK     = 1,
	AWBALG_ADD    = 2,  // Add Block0
	//            = 3,  // Add Block1
	//            = 4,  // Add Block2
	//            = 5,  // Add Block3
	AWBALG_CNT    = 9,
	AWBALG_YL     = 10,
	AWBALG_YU     = 15,
	AWBALG_RPBL   = 20,
	AWBALG_RPBU   = 25,
	AWBALG_RSBL   = 30,
	AWBALG_RSBU   = 35,
	AWBALG_R2GL   = 40,
	AWBALG_R2GU   = 45,
	AWBALG_B2GL   = 50,
	AWBALG_B2GU   = 55,
	AWBALG_RMBL   = 60,
	AWBALG_RMBU   = 65,
	AWBALG_REMOVE = 70,
	AWBALG_MAX,
	ENUM_DUMMY4WORD(AWBALG_ER)
} AWBALG_ER;

/**
	AWB Alg ProcMode.
*/
typedef enum AWB_PROC_MODE {
	AWB_PROC_MOVIE,
	AWB_PROC_PHOTO,
	AWB_PROC_CAPTURE,
	ENUM_DUMMY4WORD(AWB_PROC_MODE)
} AWB_PROC_MODE;

/**
	AWB Alg AppMode.
*/
typedef enum _AWB_APPMODE {
	AWB_APPMODE_NORMAL = 0,
	AWB_APPMODE_SHDR,
	AWB_APPMODE_END = 0xffffffff,
	ENUM_DUMMY4WORD(AWB_APPMODE)
} AWB_APPMODE;

/**
	AWB Alg CT gain
*/
typedef struct _AWBALG_GAIN {
	UINT32 r_gain;
	UINT32 g_gain;
	UINT32 b_gain;
} AWBALG_GAIN;

/**
	 AWB CT weight.
*/
typedef struct _CWGT_LUT {
	INT32 ctmp;
	INT32 weight;
} CWGT_LUT;

/**
	 AWB algorithm event.
	 @note
*/
typedef struct _AWBALG_KGAIN {
	UINT32 gs_r_gain;  //awb gain of golden sample
	UINT32 gs_b_gain;
	UINT32 k_r_gain; //calibration gain of test sample
	UINT32 k_b_gain;
	UINT32 r_ratio;
	UINT32 b_ratio;
} AWBALG_KGAIN;

typedef struct _AWB_INFO {
	UINT32 id;
	//ISP_AWB_TRIG_OBJ trig_obj;
	AWBALG_CA_THRESHOLD ca_th;
	AWB_PROC_MODE proc_mode;
	AWB_APPMODE awb_app_mode;
	AWB_ID multi_id[ISP_SEN_MFRAME_MAX_NUM];
	AWB_STS status;
	UINT32 cur_lv;
	UINT32 cur_ct;
	UINT32 cur_cx;
	UINT32 win_num_x;
	UINT32 win_num_y;
	ISP_BUILTIN_CA_RSLT *acc_tab;
	UINT32 min_w_total;
	UINT32 white_point_count;
	UINT16 flag_tab[AWB_CABUFFER_SIZE];

	AWBALG_KGAIN k_gain;
	AWBALG_GAIN def_gain;
	AWBALG_GAIN new_gain;
	AWBALG_GAIN awb_gain;
	AWBALG_GAIN pre_gain;
	AWBALG_GAIN cap_gain;
	AWBALG_GAIN gray_gain;
	AWB_MANUAL manual;
	AWBALG_STABLE_GAIN stable_gain;

	UINT32 awb_cnt;
	UINT32 awb_enter_cnt;

	BOOL single_mode_en;
	BOOL smooth_en;
	UINT32 awb_conv_skip_frame;
	UINT32 awb_conv_speed;
	BOOL expand_ct_en;
	AWB_MULTI_CT multi_ct;
	BOOL awb_auto_weight_en;
	UINT32 awb_auto_weight_ct_idx;
	BOOL gray_world_en;
} AWB_INFO;

/////////////////////////////////////////
// extern variable
/////////////////////////////////////////
extern AWB_PARAM *awb_param[AWB_ID_MAX_NUM];
extern AWBALG_ER awb_alg_method(AWB_INFO *awb_info, AWB_PARAM *m1_par);

#endif

