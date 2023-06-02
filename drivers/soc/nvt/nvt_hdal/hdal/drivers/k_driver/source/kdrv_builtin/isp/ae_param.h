#ifndef _AE_PARAM_H_
#define _AE_PARAM_H_

#include "kwrap/type.h"
#include "isp_builtin.h"
#include "ae_common.h"

#define AE_ISO_BASE             100
#define AE_PRV_ISO_MIN          AE_ISO_BASE
#define AE_PRV_ISO_MAX          204800
#define AE_CAP_ISO_MIN          AE_ISO_BASE
#define AE_CAP_ISO_MAX          6400
#define AE_ISP_GAIN_TH          819200

#define AE_HIST_NUM                 64                          ///< historgram bin number

#define AEALG_DYNAMIC_LV_NUM        21
#define AEALG_CURVEGEN_NODE_MAX     12

/**
	AE Alg ProcMode.
*/
typedef enum AE_PROC_MODE {
	AE_PROC_MOVIE,
	AE_PROC_PHOTO,
	AE_PROC_CAPTURE,
	ENUM_DUMMY4WORD(AE_PROC_MODE)
} AE_PROC_MODE;

typedef enum _AE_FLICKER_MODE {
	AE_FLICKER_NORMAL = 0,
	AE_FLICKER_EXPT_FREE,
	AE_FLICKER_INDOOR,
	ENUM_DUMMY4WORD(AE_FLICKER_MODE)
} AE_FLICKER_MODE;

typedef enum _AE_FLICKER {
	AE_FLICKER_50HZ = 0,                           ///< flicker 50Mz
	AE_FLICKER_60HZ,                               ///< flicker 60Mz
	AE_FLICKER_MAX,
	ENUM_DUMMY4WORD(AE_FLICKER)
} AE_FLICKER;

typedef enum _AE_ISOMODE_MOV_TYPE {
	AE_ISOMODE_MOV_PARAM = 0,
	AE_ISOMODE_MOV_UIMAX,
	AE_ISOMODE_MOV_MAXCNT,
	ENUM_DUMMY4WORD(AE_ISOMODE_MOV_TYPE)
} AE_ISOMODE_MOV_TYPE;

typedef enum _AE_ISOMODE_PHOTO_TYPE {
	AE_ISOMODE_PHOTO_PARAM = 0,
	AE_ISOMODE_PHOTO_SPEED,
	AE_ISOMODE_PHOTO_UIMAX,
	AE_ISOMODE_PHOTO_MAXCNT,
	ENUM_DUMMY4WORD(AE_ISOMODE_PHOTO_TYPE)
} AE_ISOMODE_PHOTO_TYPE;

typedef enum _AE_ADJUST_STATUS {
	AE_STATUS_STABLE = 0,
	AE_STATUS_FINE,
	AE_STATUS_COARSE,
	ENUM_DUMMY4WORD(AE_ADJUST_STATUS)
} AE_ADJUST_STATUS;

typedef enum _AE_ROI_OPTION {
	ROI_ROUNDING = 0,
	ROI_EROSION,
	ROI_DIALATION,
	ROI_ROUND_MAX
} AE_ROI_OPTION;

typedef enum _AE_PERF_CMD {
	AE_PERF_NONE = 0,
	AE_PERF_FIRST_STABLE,
	AE_PERF_EVERY_STABLE,
	AE_PERF_MAX
} AE_PERF_CMD;

typedef struct _AE_ROI_WIN {
	UINT32 start_x;
	UINT32 start_y;
	UINT32 end_x;
	UINT32 end_y;
	AE_ROI_OPTION option;
} AE_ROI_WIN;

/**
	AE priority mode
*/
typedef enum _AEALG_PRIORITY_MODE {
	AEALG_PRIORITY_OFF = 0,
	AEALG_PRIORITY_EXPT,
	AEALG_PRIORITY_ISO,
	AEALG_PRIORITY_APERTURE,
	AEALG_PRIORITY_MAX,
	ENUM_DUMMY4WORD(AEALG_PRIORITY_MODE)
} AEALG_PRIORITY_MODE;

/**
	AE Parameter.
*/
typedef struct _AE_EXPECT_LUM {
	UINT32 lum_mov;                                ///< range : 0~255
	UINT32 lum_photo;                              ///< range : 0~255
	UINT32 tab_ratio_mov[AEALG_DYNAMIC_LV_NUM];    ///< range : 0~100, 100 = 1X, dynamic expected luminance table for movie (LV0 ~ LV20)
	UINT32 tab_ratio_photo[AEALG_DYNAMIC_LV_NUM];  ///< range : 0~100, 100 = 1X, dynamic expected luminance table for capture (LV0 ~ LV20)
} AE_EXPECT_LUM;

typedef struct _AE_LA_CLAMP {
	UINT32 tab_normal_h[AEALG_DYNAMIC_LV_NUM];     ///< range : 0~255, 8bit value for la clamp at non-shdr mode
	UINT32 tab_normal_l[AEALG_DYNAMIC_LV_NUM];     ///< range : 0~255, 8bit value for la clamp at non-shdr mode
	UINT32 tab_shdr_h[AEALG_DYNAMIC_LV_NUM];       ///< range : 0~1023, 1X = 100, ratio of expected luminance for la clamp at shdr mode
	UINT32 tab_shdr_l[AEALG_DYNAMIC_LV_NUM];       ///< range : 0~1023, 1X = 100, ratio of expected luminance for la clamp at shdr mode
} AE_LA_CLAMP;

typedef struct _AE_OVER_EXPOSURE {
	UINT32 enable;
	UINT32 lum;                                    ///< range : 0~255
	UINT32 tab_ratio[AEALG_DYNAMIC_LV_NUM];        ///< range : 0~100, 1X = 100
	UINT32 tab_thr_mov[AEALG_DYNAMIC_LV_NUM];      ///< range : 0~255, adjust value threshold
	UINT32 tab_thr_photo[AEALG_DYNAMIC_LV_NUM];    ///< range : 0~255, adjust value threshold
	UINT32 tab_maxcnt[AEALG_DYNAMIC_LV_NUM];       ///< range : 0~1023, max threshold of over exposure block number
	UINT32 tab_mincnt[AEALG_DYNAMIC_LV_NUM];       ///< range : 0~1023, max threshold of over exposure block number
} AE_OVER_EXPOSURE;

typedef struct _AE_BOUNDARY {
	UINT32 h;  //high
	UINT32 l;  //low
} AE_BOUNDARY;

typedef struct _AE_BOUNDARY64 {
	UINT64 h;                                       ///< high value
	UINT64 l;                                       ///< low value
} AE_BOUNDARY64;

typedef struct _AE_CONVERGENCE {
	UINT32 speed;                                  ///< range : 0~128, 0 = slowest, 128 = fastest
	AE_BOUNDARY range_conv;                        ///< range : 0~100, 10 = +/-10% convergence range
	UINT32 freeze_en;
	UINT32 freeze_cyc;                             ///< range : 0~100, 10 = 1s, the unit is 1/10s
	UINT32 freeze_thr;                             ///< range : 0~1000, max threshold for sum of luminance change (%) in freeze_cyc
	UINT32 slowshoot_en;
	UINT32 slowshoot_range;                        ///< range : 0~100, 10 = +/-10% (on convergence range) for slowshoot range
	UINT32 slowshoot_thr;                          ///< range : 0~100, max speed value in slowshoot range
} AE_CONVERGENCE;

typedef struct _AE_GEN_NODE {
	UINT32 expt;                                   ///< the unit is us
	UINT32 iso;                                    ///< range : 100~3276800
	UINT32 aperture;
	UINT32 reserved;
} AE_GEN_NODE;

typedef struct _AE_EXTEND_FPS {
	UINT32 sensor_fps;                             ///< range : 100~6000, 60000 = 60.00fps
	UINT32 extend_fps;                             ///< range : 100~6000, 60000 = 60.00fps
} AE_EXTEND_FPS;

typedef struct _AE_CURVE_GEN_MOVIE {
	UINT32 iso_calcoef;                            ///< range : 0~1023, fno * fno * 10
	AE_FLICKER freq;
	AE_GEN_NODE node[AEALG_CURVEGEN_NODE_MAX];
	UINT32 node_num;                               ///< range : 2~12, 12=AEALG_CURVEGEN_NODE_MAX
	UINT32 iso_max;                                ///< range : 100~3276800
	UINT32 hdr_ratio[ISP_SEN_MFRAME_MAX_NUM];      ///< range : 0~16, max shdr ratio = 4EV
	UINT32 auto_lowlight_en;
	AE_EXTEND_FPS ext_fps[5];
	AE_ISOMODE_MOV_TYPE iso_mode;
	UINT32 isp_gain_thres;                         ///< range : 100~3276800
	AE_FLICKER_MODE flicker_mode;
} AE_CURVE_GEN_MOVIE;

typedef struct _AE_METER_WINDOW {
	UINT32 matrix[AE_WIN_NUM];                     ///< range : 0~1023
} AE_METER_WINDOW;

typedef struct _AE_LUM_GAMMA {
	UINT16 gamma[65];                              ///< range : 0~1023
} AE_LUM_GAMMA;

typedef struct _AE_SHDR {
	UINT32 expy_le;                                ///< range : 0~255
	UINT32 tab_ratio_le[AEALG_DYNAMIC_LV_NUM];     ///< range : 0~100
	UINT32 tab_reserved0[AEALG_DYNAMIC_LV_NUM];
	UINT32 tab_reserved1[AEALG_DYNAMIC_LV_NUM];
	UINT32 fixed_iso_en;
	UINT32 reserved0;
} AE_SHDR;

typedef struct _AE_IRIS_PID {
	INT32 kp;
	INT32 ki;
	INT32 kd;
} AE_IRIS_PID;

typedef struct _AE_IRIS_CFG {
	UINT32 enable;
	UINT32 exptime_min;                            ///< exposure time threshold for iris enable
	UINT32 probe_balance;
	INT32 balance_ratio;
	INT32 driving_ratio;
	AE_IRIS_PID iris_ctrl;
	UINT32 pwm_id;
	UINT32 drv_max;
	UINT32 freeze_time;
	UINT32 unfreeze_time;
	UINT32 ctrl_dir_inv;
	UINT32 cali_out_balance;
} AE_IRIS_CFG;

typedef struct _AE_SMART_IR_WEIGHT{
	UINT32 center;
	UINT32 around;
	UINT32 other;
} AE_SMART_IR_WEIGHT;

typedef struct _AE_CURVE_GEN_PHOTO {
	AE_GEN_NODE node[AEALG_CURVEGEN_NODE_MAX];
	UINT32 node_num;                               ///< range : 2~12, 12=AEALG_CURVEGEN_NODE_MAX
	UINT32 expt_max;                               ///< the unit is us
	UINT32 iso_max;                                ///< range : 100~3276800
	AE_ISOMODE_PHOTO_TYPE iso_mode;
	UINT32 isp_gain_thres;                         ///< reserved
} AE_CURVE_GEN_PHOTO;

typedef struct _AE_MANUAL {
	UINT32 lock_en;
	UINT32 expotime;                               ///< the unit is us
	UINT32 iso_gain;                               ///< range : 100~3276800
	UINT32 aperture;
	UINT64 totalgain;                                     ///< reserved, change to totalgain (to do!!)
} AE_MANUAL;

typedef struct _AE_STATUS_INFO {
	UINT32 lv;
	UINT32 lv_base;                                ///< 1000000 = AEALG_LV_ACCURACY_BASE
	UINT32 ev;
	UINT32 ev_base;                                ///< 1000000 = AEALG_EV_ACCURACY_BASE
	UINT32 lum;                                    ///< range : 0~255, current raw luminance at 8bit
	UINT32 hist_lum;
	UINT32 expotime[ISP_SEN_MFRAME_MAX_NUM];       ///< the unit is us
	UINT32 iso_gain[ISP_SEN_MFRAME_MAX_NUM];       ///< range : 100~3276800
	UINT32 expect_lum;                             ///< range : 0~255, raw expected luminance at 8bit
	UINT32 overexp_adj;                            ///< range : 0~255, overexposure adjust raw luminance at 8bit
	UINT32 fps;                                    ///< 3000 = 30.00 fps
	UINT32 state_adj;
	UINT32 overexp_cnt;                            ///< 1 = 1/1000, overexposure block ratio
	UINT32 mf_num;
	UINT32 la_data[ISP_SEN_MFRAME_MAX_NUM][AE_WIN_NUM];
	UINT32 hist_data[ISP_SEN_MFRAME_MAX_NUM][AE_HIST_NUM];
	UINT32 aperture;
} AE_STATUS_INFO;

typedef struct _AE_PARAM {
	UINT32 base_iso;
	UINT32 base_gain_ratio;
	AE_EXPECT_LUM *expect_lum;
	AE_LA_CLAMP *la_clamp;
	AE_OVER_EXPOSURE *over_exposure;
	AE_CONVERGENCE *convergence;
	AE_CURVE_GEN_MOVIE *curve_gen_movie;
	AE_METER_WINDOW *meter_win;
	AE_LUM_GAMMA *lum_gamma;
	AE_SHDR *shdr;
	AE_IRIS_CFG *iris_cfg;
	AE_CURVE_GEN_PHOTO *curve_gen_photo;
	AE_MANUAL *manual;
	AE_STATUS_INFO *status;
} AE_PARAM;

//=============================================================================
// extern functions
//=============================================================================
extern void ae_param_get_param(UINT32 *param);

#endif
