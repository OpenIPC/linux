#ifndef _AE_BUILTIN_NVT_H_
#define _AE_BUILTIN_NVT_H_

#include "kwrap/type.h"
#include "isp_builtin.h"
#include "ae_param.h"

//=============================================================================
// version
//=============================================================================
#define AE_BUILTIN_VERSION 0x01000000

//=============================================================================
// struct & enum definition
//=============================================================================
#define AE_RATE                     1

#define AE_HIST_IDX(thr, bin_num)       (thr - (((1 << 10) / bin_num) >> 1)) / ((1 << 10) / bin_num);

#define AEALG_STATUS_RATIO          2

#define AE_OVEREXP_UNLOCK_TH            200

#define AE_GEN_CURVE_NODE_MAX       40

#define AE_TOTAL_PARAMNUM           2                           ///< always keep six frames AE Setting. don't modify this value;

#define AE_PREBUILT_DBG_NVT_MSG     0



typedef enum _AE_ID {
	AE_ID_1 = 0,                    ///< ae id 1
	AE_ID_2,                        ///< ae id 2
	AE_ID_3,                        ///< ae id 3
	AE_ID_4,                        ///< ae id 4
	AE_ID_5,                        ///< ae id 5
	AE_ID_MAX_NUM,
	ENUM_DUMMY4WORD(AE_ID)
} AE_ID;


/**
	AEALG error code.
*/
typedef enum _AEALG_ER {
	AEALG_OK = 0,		  ///< AE OK
	AEALG_PAR,			  ///< AE Parameter error
	AEALG_MAX,
	ENUM_DUMMY4WORD(AEALG_ER)
} AEALG_ER;

typedef enum _AE_METER {
	AE_METER_USERDEFINE,              ///< User define
	AE_METER_SMARTIR,                 ///< Smart IR
	AE_METER_MAX_CNT,
	ENUM_DUMMY4WORD(AE_METER)
} AE_METER;

typedef enum _AEALG_SHOOT_DIR {
	AEALG_SHOOT_UP = 0,                   // AE shoot up (increase gain)
	AEALG_SHOOT_DOWN,                     // AE shoot down (decrease gain)
	AEALG_SHOOT_MAXCNT,
	ENUM_DUMMY4WORD(AEALG_SHOOT_DIR)
} AEALG_SHOOT_DIR;

typedef enum _AEALG_SLOWSHOOT_STATUS {
	AE_SLOWSHOOT_STS_OFF = 0,
	AE_SLOWSHOOT_STS_ON,
	ENUM_DUMMY4WORD(AEALG_SLOWSHOOT_STATUS)
} AEALG_SLOWSHOOT_STATUS;

typedef struct _AE_HISTOGRAM {
	UINT32 bin_num;  ///< total bin number, = AE_HIST_NUM
	UINT32 data_num; ///< total weighted data number
	UINT16 *y;       ///< yuv histogram pointer
} AE_HISTOGRAM;

typedef struct _AE_LUMINANCE {
	UINT32 raw;     ///< range : 0~4095, 12bit raw luminance
	UINT32 y;       ///< range : 0~1023, 10bit y luminance
} AE_LUMINANCE;

typedef struct _AE_ENV_VALUE {
	UINT32 LV;         ///< lv * 1000000(AEALG_LV_ACCURACY_BASE)
	UINT64 EVValue;    ///< ev * 1000000(AEALG_EV_ACCURACY_BASE)
} AE_ENV_VALUE;

typedef struct _AEALG_ARG {
	UINT32 EVValue;         ///< evvalue
	UINT32 ExpoTime;        ///< Exposure Time(us)
	UINT32 ISOGain;         ///< ISO value(50, 100, 200......)
} AEALG_ARG;

typedef struct _AE_OVEREXP_INFO {
	AE_LUMINANCE AdjLum;                 ///< adjust luminace
	UINT32 CurCnt;                       ///< 1 = 1/1000, overexposure block ratio
	UINT32 CurTotalStep;                 ///< range : 0~4095, overexposure adjust raw luminance at 12bit
	UINT32 CurMaxStep;                   ///< range : 0~4095, current adjust max step for 12bit raw
	UINT32 CurMaxCnt;                    ///< range : 0~1023, current overexposure block max limitation
	UINT32 CurMinCnt;                    ///< range : 0~1023, current overexposure block min limitation
	UINT32 CurThreshold;                 ///< range : 0~4095, current overexposure luminance threshold for 12bit raw
	UINT32 *AdjLumThresholdTab;          ///< over exposure luminance threshold table (LV0 ~ LV20, max index 21), AdjLum * x%
	UINT32 *AdjLumTab;                   ///< over exposure adjust luminance table (LV0 ~ LV20, max index 21, current expected Lum - adj lum)
	UINT32 *AdjMaxCntRatioTab;           ///< over exposure luminance counter table (LV0 ~ LV20, max index 21) (x / 1000)
	UINT32 *AdjMinCntRatioTab;           ///< over exposure luminance counter table (LV0 ~ LV20, max index 21) (x / 1000)
	UINT32 lum_weight_sum;               ///< 1 = 1/1000, overexposure block ratio ( = CurCnt)
} AE_OVEREXP_INFO;

typedef struct _AE_EXT_SET {
	AE_LUMINANCE PrvExpectLum;           ///< standard expected luminance for movie
	UINT32 *PrvLumRatioTab;              ///< range : 0~100, 100 = 1X, dynamic expected luminance table (LV0 ~ LV20) for movie
	UINT32 AntiOverExpEn;                ///< anti overexposure enable
	AE_OVEREXP_INFO OverExp;             ///< anti overexposure condition
	UINT32 EVCompRatio;                             ///< ev comp ratio(%), mapping from EVTable
} AE_EXT_SET;

typedef enum _AE_GEN_FREQ {
	AE_GEN_FREQ_NONE = 0,
	AE_GEN_FREQ_27_5,
	AE_GEN_FREQ_50,
	AE_GEN_FREQ_60,
	AE_GEN_FREQ_NUM,
	ENUM_DUMMY4WORD(AE_GEN_FREQ)
} AE_GEN_FREQ;

typedef struct _AE_CURVEGEN_PARAM_MOVIE {
	UINT32 iso_calcoef;                          ///< fno * fno * 10
	AE_GEN_NODE node[AEALG_CURVEGEN_NODE_MAX];
	UINT32 node_num;
	UINT32 iso_max;
	AE_GEN_FREQ freq;                       ///< anti flicker frequence 50 or 60
	UINT32 frame_rate;
	UINT32 hdr_frame;
	UINT32 hdr_ratio[ISP_SEN_MFRAME_MAX_NUM];
	UINT32 hdr_exp_gap;
	UINT32 auto_lowlight_en;
	AE_EXTEND_FPS ext_fps[5];
	UINT32 iso_value;
	UINT32 priority_mode;
	UINT32 priority_expt;
	UINT32 priority_iso;
	UINT32 priority_aperture;
	AE_FLICKER_MODE flicker_mode;
} AE_CURVEGEN_PARAM_MOVIE;

typedef struct _AE_CURVE {
	UINT64 total_gain[AE_GEN_CURVE_NODE_MAX];
	UINT32 exptime[AE_GEN_CURVE_NODE_MAX];
	UINT32 isogain[AE_GEN_CURVE_NODE_MAX];
	UINT32 aperture[AE_GEN_CURVE_NODE_MAX];
	UINT32 adj_target[AE_GEN_CURVE_NODE_MAX];
} AE_CURVE;

typedef struct _AE_SHDR_INFO {
	UINT32 enable;
	UINT32 le_id;
	UINT32 se_id;
	UINT32 se_ev_ratio;
} AE_SHDR_INFO;

typedef struct _AE_ISP_SYNC {
	UINT32 expotime[AE_TOTAL_PARAMNUM][ISP_SEN_MFRAME_MAX_NUM];
	UINT32 total_gain[AE_TOTAL_PARAMNUM][ISP_SEN_MFRAME_MAX_NUM];
	UINT32 dgain[AE_TOTAL_PARAMNUM][ISP_SEN_MFRAME_MAX_NUM];
} AE_ISP_SYNC;

typedef struct _AE_SHDR_PARAM {
	UINT32 frm_num;
	UINT32 mask;
	UINT32 expo_time[ISP_SEN_MFRAME_MAX_NUM];
	UINT32 iso_gain[ISP_SEN_MFRAME_MAX_NUM];
	UINT32 expo_time_max[ISP_SEN_MFRAME_MAX_NUM];
	UINT32 ev_ratio[ISP_SEN_MFRAME_MAX_NUM];
	UINT32 ev_ratio_new[ISP_SEN_MFRAME_MAX_NUM];
	UINT32 iso_min;
	UINT32 iso_max;
	UINT32 fixed_iso_en;
} AE_SHDR_PARAM;

typedef struct _AE_FLOW_INFO {
	UINT32 id;
	AE_LUMINANCE PrvAvgLum;                         ///< current weighted luminance
	AE_ENV_VALUE PrvAvgEV;                          ///< current ev/lv value
	AE_BOUNDARY64 totalgain_movie;                  ///< max/min totalgain for movie	
	AE_BOUNDARY iso_range_movie;                    ///< range : 100~3276800, max, min iso for movie
	AE_BOUNDARY stable_range;                        ///< range : 0~100, 10 = +/-10% convergence range
	AE_HISTOGRAM histogram;                         ///< current histogram information
	UINT16 *gamma_tbl;                              ///< lA gamma table	
	UINT64 min_total_gain, max_total_gain;          ///< current max/min totalgain
	UINT32 FirstRunFlag;                            ///< first run flag, useless
	UINT32 Reset;                                   ///< reset flag
	UINT32 Counter;                                 ///< stable counter
	UINT32 ConvSpeed;                               ///< range : 0~128, 0 = slowest, 128 = fastest
	UINT32 FPS;                                     ///< 3000 = 30.00 fps
	UINT32 State_Adj;
	UINT32 rate;                                    ///< 1 = process per frame, process frequency
	UINT32 isp_gain_thres;
	AE_LUMINANCE CurExpectLum;                      ///< current expected luminance
	AE_LUMINANCE CurLum;                            ///< current luminance, useless (=PrvAvgLum)
	AE_ENV_VALUE PrvParam;                          ///< preview parameter, useless
	UINT32 meter_matrix[AE_WIN_NUM];
	UINT32 isocalcoef;
	UINT32 prv_row_time;                            ///< 10 = 1us, sensor row time for movie
	UINT32 freeze_en;
	UINT32 freeze_cyc;
	UINT32 freeze_thr;
	UINT32 slowshoot_en;
	UINT32 slowshoot_range;
	UINT32 slowshoot_thr;
	UINT32 slowshoot_start;
	UINT32 p_factor[AEALG_SHOOT_MAXCNT];
	UINT32 d_factor[AEALG_SHOOT_MAXCNT];
	INT64 curr_total_gain_delta;
	INT64 prev_total_gain_delta;
	UINT64 prev_target_total_gain;
	UINT64 curr_total_gain;
	UINT32 proc_first_in;
	AE_EXT_SET ExtSet;                              ///< extend parameter for expext luminance
	AEALG_ARG prv_arg;
	AE_CURVE curve_tbl_movie;
	AE_CURVEGEN_PARAM_MOVIE curve_par_movie;
	AE_SHDR_PARAM shdr_param;
	AE_SHDR_INFO shdr_info;
	AE_ISP_SYNC isp_sync;
	AEALG_SLOWSHOOT_STATUS slowshoot_status;
	AEALG_SHOOT_DIR curr_shoot_dir;
	UINT32 overexp_offset;
} AE_FLOW_INFO;

typedef enum _AEALG_ADJ_ITEM {
	AEALG_ADJ_EXPTIME = 0,  ///< Adjust exposure time
    AEALG_ADJ_ISOGAIN,      ///< Adjust iso gain
    AEALG_ADJ_APERTURE,     ///< Adjust aperture (p-iris)
    AEALG_ADJ_DCIRIS,       ///< Adjust dc-iris
    ENUM_DUMMY4WORD(AEALG_ADJ_ITEM)
} AEALG_ADJ_ITEM;



#define AEALG_TAB_END                   0xFFFFFFFFFFFFFFFF             ///< AE table end flag

//=============================================================================
// extern functions
//=============================================================================
extern INT32 ae_builtin_init_nvt(UINT32 id);
extern INT32 ae_builtin_uninit(UINT32 id);
extern INT32 ae_builtin_trig_nvt(UINT32 id, ISP_BUILTIN_AE_TRIG_MSG msg);
extern void ae_builtin_get_param(UINT32 id, UINT32 *param);

// ae_flow
void ae_builtin_curve_init(AE_FLOW_INFO *ae_info);

// ae_alg
void ae_alg_do_converge(AE_FLOW_INFO *AEInfo, AEALG_ARG *PrvInfo);
void ae_builtin_dbg_msg(AE_FLOW_INFO *aeinfo);
void ae_alg_cal_mfrm_exptime(AE_SHDR_PARAM *shdr_param);
#endif


