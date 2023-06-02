/*
    Copyright   Novatek Microelectronics Corp. 2005~2014.  All rights reserved.

    @file       isf_vdocap.h
    @ingroup    mvdocap

    @note       Nothing.

    @date       2018/06/04
*/

#ifndef _ISF_VDOCAP_H_
#define _ISF_VDOCAP_H_

#ifdef __KERNEL__
#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#else
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#endif

#if defined(_BSP_NA51000_)
#define VDOCAP_MAX_NUM	8
#elif defined(_BSP_NA51055_)
#define VDOCAP_MAX_NUM	5
#elif defined(_BSP_NA51089_)
#define VDOCAP_MAX_NUM 3
#endif



#define VDOCAP_SEN_MODE_AUTO	0xFFFFFFFF

#define VDOCAP_SEN_SER_MAX_DATALANE 8
#define VDOCAP_SEN_NAME_LEN 32
#define VDOCAP_SEN_IGNORE  0xFFFFFFFF  ///< sensor ignore information


typedef struct _VDOCAP_SEN_PINMUX_CFG {
	UINT32 sensor_pinmux;		///< need mapping to top.h define (PIN_SENSOR_CFG/PIN_SENSOR2_CFG/.../PIN_SENSOR8_CFG)
	UINT32 serial_if_pinmux;	///< need mapping to top.h define (PIN_MIPI_LVDS_CFG)
	UINT32 cmd_if_pinmux;		///< need mapping to top.h define (PIN_I2C_CFG/PIN_SIF_CFG)
} VDOCAP_SEN_PINMUX_CFG;

typedef struct _VDOCAP_SEN_INIT_PIN_CFG {
	VDOCAP_SEN_PINMUX_CFG pinmux;		///< set gpio for function
	UINT32 clk_lane_sel;	//< only LVDS and MIPI need to set, referring CTL_SEN_CLANE_SEL
	UINT32 sen_2_serial_pin_map[VDOCAP_SEN_SER_MAX_DATALANE];    ///< Serial (lvds/csi/slvsec) pin 2 hw map
	BOOL ccir_msblsb_switch; 	///< only for ccir sensor output HD mode (16 bits)
	BOOL ccir_vd_hd_pin;    	///< CCIR601 need HW VD/HD output pin
	BOOL vx1_tx241_cko_pin;   	///< refer to VX1SBL_CONFIG_ID_CKO_OUTOUT.
	BOOL vx1_tx241_cfg_2lane_mode;  ///< set FALSE for 1 lane mode, set TRUE for 2 lanes mode. Only KDRV_SSENIFVX1_TXTYPE_THCV241 has 2 lanes mode.
} VDOCAP_SEN_INIT_PIN_CFG;

typedef struct _VDOCAP_SEN_IF_VX1 {
	BOOL vx1_en;          ///< set TRUE for used vx1, only support CTL_SEN_IF_TYPE_PARALLEL & CTL_SEN_IF_TYPE_MIPI
	UINT32 tx_module;     ///< vx1 transmitter Module, referring to CTL_SEN_VX1_TXMODULE
	UINT32 lane_num;          ///< vx1 lane number, referring to CTL_SEN_VX1_LANE
	UINT32 data_mux;       ///< referring to CTL_SEN_VX1_DATAMUX

	/* only VX1_TXMODULE_THCV241 need to implement */
	BOOL clk_speed_bps;///< When using THCV241 is PLL auto calculation path.
					   ///< The user must assign the sensor clock speed in bits per second to make calculation tool working.
					   ///< For example, if the sensor clock speed is 445Mbps, please set 445000000 (bps) to this field.
					   ///< set 0 for manual mode (need to set VX1_PLL_241 pll_241[SENSOR_MODE_Num])
//	BOOL cko_pin;   ///< refer to VX1SBL_CONFIG_ID_CKO_OUTOUT.
} VDOCAP_SEN_IF_VX1;

typedef enum {
	VDOCAP_SEN_IF_TYPE_PARALLEL,   ///< parallel
	VDOCAP_SEN_IF_TYPE_LVDS,       ///< LVDS
	VDOCAP_SEN_IF_TYPE_MIPI,       ///< MIPI
	VDOCAP_SEN_IF_TYPE_SLVSEC,     ///< SLVS-EC
	VDOCAP_SEN_IF_TYPE_DUMMY,
	VDOCAP_SEN_IF_TYPE_SIEPATGEN,  ///< sie patten gen
	VDOCAP_SEN_IF_TYPE_MAX_NUM,
	ENUM_DUMMY4WORD(VDOCAP_SEN_IF_TYPE)
} VDOCAP_SEN_IF_TYPE;

typedef struct {
	BOOL tge_en;    ///< set TRUE for used tge, slave sensor (CTL_SEN_SIGNAL_SLAVE) must be enable
	BOOL swap;      ///< swap CH1 & 5, or CH2 & 6, or CH3 & 7, or CH4 & 8
	UINT32 sie_vd_src;  ///< SIE 1/3 VD/HD signal source (only in parallel mode + slave sensor)
	UINT32 sie_sync_set; ///< indicate which SIEs need to sync, bitwise setting for SIEx, e.g. set 0x3 to sync SIE1 and SIE2
} VDOCAP_SEN_IF_TGE;

typedef struct _VDOCAP_SEN_INIT_IF_CFG {
	VDOCAP_SEN_IF_TYPE type; ///< sensor interface type, referring to VDOCAP_SEN_IF_TYPE type
	VDOCAP_SEN_IF_TGE tge;          ///< tge information
} VDOCAP_SEN_INIT_IF_CFG;

typedef struct {
	BOOL en;
	UINT32 if_sel;     ///< interface select
	UINT32 ctl_sel;    ///< controller select
	UINT32 tx_type;     ///< tx module select
} VDOCAP_SEN_CMDIF_VX1;

typedef struct {
	VDOCAP_SEN_CMDIF_VX1 vx1;	///< vx1 information, only valid when HD_COMMON_VIDEO_IN_TYPE is HD_COMMON_VIDEO_IN_MIPI_VX1 or HD_COMMON_VIDEO_IN_P_RAW_VX1
} VDOCAP_SEN_INIT_CMDIF_CFG;

typedef enum _VDOCAP_SEN_OPTION_ENABLE {
	VDOCAP_SEN_ENABLE_MAP_IF     =	0x00000001,
	VDOCAP_SEN_ENABLE_IF_TIMEOUT =	0x00000002,
	ENUM_DUMMY4WORD(VDOCAP_SEN_OPTION_ENABLE)
} VDOCAP_SEN_OPTION_ENABLE;

typedef struct {
	VDOCAP_SEN_INIT_PIN_CFG pin_cfg;
	VDOCAP_SEN_INIT_IF_CFG if_cfg;
	VDOCAP_SEN_INIT_CMDIF_CFG cmd_if_cfg;
} VDOCAP_SEN_INIT_CFG_OBJ;

typedef struct _VDOCAP_SEN_INIT_OPTION {
	VDOCAP_SEN_OPTION_ENABLE en_mask;  ///< enable mask, bitwise, for the following settings
	UINT32 sen_map_if;			      ///< sensor map interface, referring to CTL_SEN_MAP_IF_DEFAULT
	UINT32 if_time_out;               ///< ms, default: 1000ms, only support LVDS/MIPI/SLVS-EC
} VDOCAP_SEN_INIT_OPTION;

typedef struct _VDOCAP_SENSOR_DEVICE {
	CHAR driver_name[VDOCAP_SEN_NAME_LEN];
	VDOCAP_SEN_INIT_CFG_OBJ sen_init_cfg;
	VDOCAP_SEN_INIT_OPTION sen_init_option;
} VDOCAP_SEN_INIT_CFG;

typedef enum _VDOCAP_SEN_FRAME_NUM {
	VDOCAP_SEN_FRAME_NUM_1 = 1,      ///< linear mode
	VDOCAP_SEN_FRAME_NUM_2 = 2,      ///< SHDR 2 frame
	VDOCAP_SEN_FRAME_NUM_3 = 3,      ///< SHDR 3 frame
	VDOCAP_SEN_FRAME_NUM_4 = 4,      ///< SHDR 4 frame
	ENUM_DUMMY4WORD(VDOCAP_SEN_FRAME_NUM)
} VDOCAP_SEN_FRAME_NUM;

typedef enum {
	VDOCAP_SEN_FIELD_DISABLE = 0,
	VDOCAP_SEN_FIELD_EN_0,
	VDOCAP_SEN_FIELD_EN_1,
	ENUM_DUMMY4WORD(VDOCAP_SEN_FIELD_SEL)
} VDOCAP_SEN_FIELD_SEL;

typedef enum {
	VDOCAP_SEN_CCIR_FMT_SEL_CCIR601 = 0,
	VDOCAP_SEN_CCIR_FMT_SEL_CCIR656,
	VDOCAP_SEN_CCIR_FMT_SEL_CCIR709,
	VDOCAP_SEN_CCIR_FMT_SEL_CCIR1120,
	VDOCAP_SEN_CCIR_FMT_SEL_MAX_NUM,
	ENUM_DUMMY4WORD(VDOCAP_SEN_CCIR_FMT_SEL)
} VDOCAP_SEN_CCIR_FMT_SEL;

typedef struct _VDOCAP_SEN_CCIR_INFO {
	VDOCAP_SEN_FIELD_SEL field_sel;
	VDOCAP_SEN_CCIR_FMT_SEL fmt;
	BOOL interlace;                         ///< enable interlace or not
	UINT32 mux_data_index;                  ///< mux data index
	//BOOL mux_singnal_en;                    ///< only support PARALLEL
	//UINT32 mux_data_num;                    ///< mux data number
} VDOCAP_CCIR_INFO;

typedef struct _VDOCAP_AE_PRESET {
	BOOL  enable;
	UINT32 exp_time;
	UINT32 gain_ratio;
} VDOCAP_AE_PRESET;

typedef struct _VDOCAP_SEN_MODE_INFO {
	UINT32 sen_mode;              		     ///<  referring to sensor driver, or set VDOCAP_SEN_MODE_AUTO for AUTO selecting
	UINT32 frc;                              ///<  frame rate
	USIZE dim;                               ///<  dim w,h. only valid when sen_mode is VDOCAP_SEN_MODE_AUTO
	UINT32 pxlfmt;                           ///<  pixel format. only valid when sen_mode is VDOCAP_SEN_MODE_AUTO
	VDOCAP_SEN_FRAME_NUM out_frame_num;      ///<  sensor output frame number, 1 for linear mode and 2/3/4 for sensor HDR mode.
	//VDOCAP_SEN_CCIR_INFO ccir_info;          ///<  for CCIR sensor only
} VDOCAP_SEN_MODE_INFO;

typedef enum _VDOCAP_OUT_DIR {
	VDOCAP_OUT_DIR_NONE	= 0x00000000,		 ///< no flip
	VDOCAP_OUT_DIR_H		= 0x00000001,		 ///< H flip
	VDOCAP_OUT_DIR_V		= 0x00000002,		 ///< V flip
	VDOCAP_OUT_DIR_H_V	= 0x00000003,		 ///< H & V flip(Flip_H | Flip_V) don't modify
	VDOCAP_OUT_DIR_MAX,
	ENUM_DUMMY4WORD(VDOCAP_OUT_DIR)
} VDOCAP_OUT_DIR;

typedef enum {
	VDOCAP_FLOW_SEN_IN,                ///< connect to real sensor
	VDOCAP_FLOW_PATGEN,                ///< SIE pattern generation mode
	VDOCAP_FLOW_MAX,
	ENUM_DUMMY4WORD(VDOCAP_FLOW_TYPE)
} VDOCAP_FLOW_TYPE;
typedef enum {
	VDOCAP_OUT_DEST_DIRECT = 0,	///< ch0 output direct to ipl_in
	VDOCAP_OUT_DEST_DRAM,       ///< ch0 output to dram
	VDOCAP_OUT_DEST_BOTH,       ///< ch0 output to both ipl_in and dram
	VDOCAP_OUT_DEST_MAX,
	ENUM_DUMMY4WORD(VDOCAP_OUT_DEST)
} VDOCAP_OUT_DEST;
typedef struct _VDOCAP_SYSINFO {
	UINT64 vd_count;       ///< VD count.
	UINT32 cur_fps;        ///< current fps
	BOOL   output_started; ///< indicate output path started or not
	USIZE	cur_dim;       ///< indicate the output size of current sensor mode
} VDOCAP_SYSINFO;

typedef struct _VDOCAP_SYSCAPS {
	USIZE  max_dim;                           ///< max dimension width/height
	UINT32 max_frame_rate;                   ///< max frame rate
	UINT32 pxlfmt;                           ///< pixel format
} VDOCAP_SYSCAPS;

typedef struct {
	USIZE size;         ///< [out], current plug size
	UINT32 fps;         ///< [out], current plug fps
	BOOL   interlace;   ///< [out] TRUE:CTL_SEN_MODE_CCIR_INTERLACE, FALSE:CTL_SEN_MODE_CCIR
	UINT32 param[5];    ///< [out], current plug info
} VDOCAP_SEN_GET_PLUG_INFO;

typedef struct {
	BOOL  enable;       ///< enable DVS function
	UINT8 positive;
	UINT8 negative;
	UINT8 nochange;
} VDOCAP_DVS_INFO;

enum {
	VDOCAP_PARAM_START = 0x80001010,
	VDOCAP_PARAM_FLOW_TYPE = VDOCAP_PARAM_START,      ///< ISF_CTRL: Set flow type, referring to VDOCAP_FLOW_TYPE.
	VDOCAP_PARAM_SHDR_MAP,                            ///< ISF_CTRL: Set sensor HDR mapping.
	VDOCAP_PARAM_CCIR_INFO,                           ///< ISF_CTRL: Set ccir info, referring to VDOCAP_CCIR_INFO.
	VDOCAP_PARAM_AD_MAP,                              ///< ISF_CTRL: Set Analog Decoder mapping.
	VDOCAP_PARAM_ALG_FUNC,                            ///< ISF_CTRL: Enable ALG function or not.
	VDOCAP_PARAM_SEN_INIT_CFG,                        ///< ISF_CTRL: Set sensor configuration, referring to VDOCAP_SEN_INIT_CFG.
	VDOCAP_PARAM_SEN_MODE_INFO,                       ///< ISF_CTRL: Sensor mode info, referring to VDOCAP_SEN_MODE_INFO.
	VDOCAP_PARAM_OUT_FRM_INFO,                        ///< ISF_CTRL: output frame info, referring to VDOCAP_OUT_FRM_INFO.
	VDOCAP_PARAM_CROP_ALIGN,                          ///< ISF_CTRL: Crop window alignment, must be a multiple of 4.
	VDOCAP_PARAM_OUT_DIR,                             ///< ISF_CTRL: type of direction of video, referring to VDOCAP_OUT_DIR.
	VDOCAP_PARAM_SYS_INFO,                            ///< ISF_CTRL: Get current system information, referring to VDOCAP_SYSINFO.
	VDOCAP_PARAM_SYS_CAPS,                            ///< ISF_CTRL: Get current sensor capibility, referring to VDOCAP_SYSCAPS.
	VDOCAP_PARAM_OUT_DEPTH,                           ///< ISF_OUTx: Output queue depth, set larger then 0 to allow pull_out
	VDOCAP_PARAM_OUT_DEST,                            ///< ISF_CTRL: Output destination, referring to VDOCAP_OUT_DEST.
	VDOCAP_PARAM_ONE_BUFF,                            ///< ISF_CTRL: Enable one buffer mode.
	VDOCAP_PARAM_BUILTIN_HDR,                         ///< ISF_CTRL: Enable built-in HDR mode.
	VDOCAP_PARAM_DATA_LANE,                           ///< ISF_CTRL: Data lane for CTL_SEN_MODESEL_AUTO_INFO.
	VDOCAP_PARAM_MCLK_SRC_SYNC_SET,                   ///< ISF_CTRL: MCLK source sync set.
	VDOCAP_PARAM_GET_PLUG,                            ///< ISF_CTRL: Get sensor plugged or unplugged
	VDOCAP_PARAM_GET_PLUG_INFO,                       ///< ISF_CTRL: Get sensor plugged info, referring to VDOCAP_SEN_GET_PLUG_INFO
	VDOCAP_PARAM_SET_FPS,                             ///< ISF_CTRL: Set sensor frame rate in running time.
	VDOCAP_PARAM_PDAF_MAP,                            ///< ISF_CTRL: Set PDAF sensor mapping.
	VDOCAP_PARAM_DMA_ABORT,                           ///< ISF_CTRL: Abort DMA
	VDOCAP_PARAM_DVS_INFO,                            ///< ISF_CTRL: Set DVS info, referring to VDOCAP_DVS_INFO.
	VDOCAP_PARAM_CHK_SHDR_MAP,                        ///< ISF_CTRL: Check if shdr_map is supported
	VDOCAP_PARAM_CHK_PIXEL_FMT,                       ///< ISF_CTRL: Check if pixel format supported
	VDOCAP_PARAM_AD_TYPE,                             ///< ISF_CTRL: Set Analog Decoder type.
	VDOCAP_PARAM_SIE_MAP,                             ///< ISF_CTRL: Set SIE mapping.
	VDOCAP_PARAM_ENC_RATE,                            ///< ISF_CTRL: Set RAW compression rate.
	VDOCAP_PARAM_AE_PRESET,                           ///< ISF_CTRL: Set AE data, referring to VDOCAP_AE_PRESET.
	VDOCAP_PARAM_DIRECT_CB = VDOCAP_PARAM_START+0x100 ///< reserved for direct mode callback
};


extern ISF_UNIT isf_vdocap0;
extern ISF_UNIT isf_vdocap1;
extern ISF_UNIT isf_vdocap2;
extern ISF_UNIT isf_vdocap3;
extern ISF_UNIT isf_vdocap4;
#if defined(_BSP_NA51000_)
extern ISF_UNIT isf_vdocap5;
extern ISF_UNIT isf_vdocap6;
extern ISF_UNIT isf_vdocap7;
#endif

#endif //_ISF_VDOCAP_H_
