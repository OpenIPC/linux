/**
    Public header file for Sensor driver

    This file is the header file that define the API and data type for Sensor driver.

    @file       ctl_sen.h
    @ingroup    mIDrvSensor
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/
#ifndef _CTL_SENSOR_H
#define _CTL_SENSOR_H


// common
#include "comm/sif_api.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include "kwrap/type.h"
#include "kwrap/platform.h"
// kdrv
#include "kdrv_videocapture/kdrv_sie.h"
#include "kdrv_videocapture/kdrv_ssenif.h"
// kflow
#include "kflow_videocapture/ctl_sen_config.h"

/*******************************************************************************/
/*                            Global ITEMs                                     */
/*******************************************************************************/
#define CTL_SEN_IGNORE  0xffffffff  ///< sensor ignore information
#define CTL_SEN_CMD_DELAY       0xffffffff // delay
#define CTL_SEN_CMD_SETVD       0xfffffffe // set vd period
#define CTL_SEN_RATIO(w, h) (((UINT32)(UINT16)(w) << 16) | (UINT32)(UINT16)(h))

#if defined(CTL_SEN_NA51089)
#define CTL_SEN_SER_MAX_DATALANE 8
#define CTL_SEN_LVDS_MAX_DATALANE 8
#define CTL_SEN_CSI_MAX_DATALANE 4
#define CTL_SEN_SLVSEC_MAX_DATALANE 8
#define CTL_SEN_MFRAME_MAX_NUM 4
#define CTL_SEN_MFRAME_SLVSEC_MAX_NUM 2
#endif
#define CTL_SEN_CMD2STR(a) (#a)
#define CTL_SEN_60M_HZ 	60000000
#define CTL_SEN_120M_HZ 120000000
#define CTL_SEN_240M_HZ 240000000
#define CTL_SEN_320M_HZ 320000000
#define CTL_SEN_480M_HZ 480000000
#define CTL_SEN_TGE_TAG 0x01000000 // tge tag
#define CTL_SEN_VX1_TAG 0x02000000 // vx1 tag

/*
    error code define
*/
#define CTL_SEN_E_OK        (0) // normal completion
// sen ctrl flow (for output & dump)
#define CTL_SEN_E_STATE     (-1)    // state machine error
#define CTL_SEN_E_MAP_TBL   (-2)    // map table error, check init_cfg/reg_sendrv/unreg_sendrv
#define CTL_SEN_E_ID_OVFL   (-3)    // input id overflow
#define CTL_SEN_E_IN_PARAM  (-4)    // input parameter error
#define CTL_SEN_E_PINMUX    (-5)    // input pinmux error, check init_cfg pinmux
#define CTL_SEN_E_CLK       (-6)    // cfg clock error
#define CTL_SEN_E_SYS       (-7)    // system error
#define CTL_SEN_E_NS        (-8)    // feature not supported
#define CTL_SEN_E_IF        (-9)    // sensor interface error (LVDS/CSI/SLVS-EC/TGE)
#define CTL_SEN_E_CMDIF     (-10)   // sensor cmd interface error (VX1)
#define CTL_SEN_E_SENDRV    (-11)   // sensor driver error
#define CTL_SEN_E_NOMEM     (-12)   // memory allocate fail
#define CTL_SEN_E_IF_GLB    (-13)   // sensor interface glb error

// sensor driver (for dump only)
#define CTL_SEN_E_SENDRV_GET_FAIL   (-100)  // sensor driver get error
#define CTL_SEN_E_SENDRV_SET_FAIL   (-200)  // sensor driver set error
#define CTL_SEN_E_SENDRV_OP         (-300)  // sensor driver operation error (open/close/sleep/wakeup/chgmode/write_reg/read_reg)
#define CTL_SEN_E_SENDRV_TBL_NULL   (-400)  // sensor driver CTL_SEN_DRV_TAB null or CTL_SEN_DRV_TAB include null function point, check reg_sendrv
#define CTL_SEN_E_SENDRV_PARAM      (-500)  // sensor driver parameters error
#define CTL_SEN_E_SENDRV_NS         (-600)  // sensor driver not support feature
// kdrv or kernel (for dump only)
#define CTL_SEN_E_KERNEL        (-10000)    // kernel error
#define CTL_SEN_E_KDRV_SSENIF   (-20000)    // kdrv ssenif error (LVDS/CSI/SLVS-EC/VX1)
#define CTL_SEN_E_KDRV_SIE      (-30000)    // kdrv sie error
#define CTL_SEN_E_KDRV_TGE      (-40000)    // kdrv tge error


/**
    SENSOR IDentification

    This is used at ctl_sen_get_object() to retrieve the specified SENSOR object.
    After retrieving the SENSOR driver object, the user shall call .init() first to start accessing.
*/
typedef enum {
	CTL_SEN_ID_1 = 0,  ///< CTL of Sensor ID
	CTL_SEN_ID_2,      ///< CTL of Sensor ID
	CTL_SEN_ID_3,      ///< CTL of Sensor ID
	CTL_SEN_ID_4,      ///< CTL of Sensor ID
	CTL_SEN_ID_5,      ///< CTL of Sensor ID
	CTL_SEN_ID_6,      ///< CTL of Sensor ID
	CTL_SEN_ID_7,      ///< CTL of Sensor ID
	CTL_SEN_ID_8,      ///< CTL of Sensor ID

	CTL_SEN_ID_MAX,
	ENUM_DUMMY4WORD(CTL_SEN_ID)
} CTL_SEN_ID;

/**
    Sensor mode
*/
#define CTL_SEN_MODE_OFS 0
#define CTL_SEN_MODE_EXT_OFS 8
#define CTL_SEN_MODE_FRMIDX_OFS 12

#define CTL_SEN_MODE_PWR (1 << CTL_SEN_MODE_EXT_OFS) // 0x00000100

//#define CTL_SEN_MODE_FRMIDX_0 (0 << CTL_SEN_MODE_FRMIDX_OFS) // 0x0000
#define CTL_SEN_MODE_FRMIDX_1 (1 << CTL_SEN_MODE_FRMIDX_OFS) // 0x1000
#define CTL_SEN_MODE_FRMIDX_2 (2 << CTL_SEN_MODE_FRMIDX_OFS) // 0x2000
#define CTL_SEN_MODE_FRMIDX_3 (3 << CTL_SEN_MODE_FRMIDX_OFS) // 0x3000
#define CTL_SEN_MODE_FRMIDX_4 (4 << CTL_SEN_MODE_FRMIDX_OFS) // 0x4000
#define CTL_SEN_MODE_FRMIDX_5 (5 << CTL_SEN_MODE_FRMIDX_OFS) // 0x5000
#define CTL_SEN_MODE_FRMIDX_6 (6 << CTL_SEN_MODE_FRMIDX_OFS) // 0x6000
#define CTL_SEN_MODE_FRMIDX_7 (7 << CTL_SEN_MODE_FRMIDX_OFS) // 0x7000

#define CTL_SEN_MODE_MASK_SENMODE(mode) ((mode & 0x000000ff) >> CTL_SEN_MODE_OFS)           // sensor mode
#define CTL_SEN_MODE_MASK_FRMIDX(mode)  ((mode & 0x0000f000) >> CTL_SEN_MODE_FRMIDX_OFS)    // multi-frame idx

typedef enum {
	CTL_SEN_MODE_1       = 0,    ///< Sensor mode 1
	CTL_SEN_MODE_2       = 1,    ///< Sensor mode 2
	CTL_SEN_MODE_3       = 2,    ///< Sensor mode 3
	CTL_SEN_MODE_4       = 3,    ///< Sensor mode 4
	CTL_SEN_MODE_5       = 4,    ///< Sensor mode 5
	CTL_SEN_MODE_6       = 5,    ///< Sensor mode 6
	CTL_SEN_MODE_7       = 6,    ///< Sensor mode 7
	CTL_SEN_MODE_8       = 7,    ///< Sensor mode 8
	CTL_SEN_MODE_9       = 8,    ///< Sensor mode 9
	CTL_SEN_MODE_10      = 9,    ///< Sensor mode 10
	CTL_SEN_MODE_11      = 10,   ///< Sensor mode 11
	CTL_SEN_MODE_12      = 11,   ///< Sensor mode 12
	CTL_SEN_MODE_13      = 12,   ///< Sensor mode 13
	CTL_SEN_MODE_14      = 13,   ///< Sensor mode 14
	CTL_SEN_MODE_15      = 14,   ///< Sensor mode 15
	CTL_SEN_MODE_NUM,
	CTL_SEN_MODE_UNKNOWN = 0xCC,    ///< Unknow Mode
	CTL_SEN_MODE_CUR     = 0xFF, ///< current sensor mode
	ENUM_DUMMY4WORD(CTL_SEN_MODE)
} CTL_SEN_MODE, *PCTL_SEN_MODE;

typedef struct {
	UINT32 addr;                      ///< address
	UINT32 data_len;                  ///< data length(bytes)
	UINT32 data[2];                   ///< data idx1(LSB) -> idx2(MSB)
} CTL_SEN_CMD;

/**
    sensor command interface
*/
typedef enum {
	CTL_SEN_CMDIF_TYPE_UNKNOWN = 0,
	CTL_SEN_CMDIF_TYPE_VX1     = 1, ///< Vx1, 510/520/560 N.S.
	CTL_SEN_CMDIF_TYPE_SIF     = 2, ///< serial
	CTL_SEN_CMDIF_TYPE_I2C     = 3, ///< I2C
	CTL_SEN_CMDIF_TYPE_IO      = 4, ///< IO
	CTL_SEN_CMDIF_TYPE_MAX_NUM,
	ENUM_DUMMY4WORD(CTL_SEN_CMDIF_TYPE)
} CTL_SEN_CMDIF_TYPE;

typedef enum {
	CTL_SEN_IF_TYPE_PARALLEL,   ///< parallel
	CTL_SEN_IF_TYPE_LVDS,       ///< LVDS
	CTL_SEN_IF_TYPE_MIPI,       ///< MIPI
	CTL_SEN_IF_TYPE_SLVSEC,     ///< SLVS-EC, 510/520/560 N.S.
	CTL_SEN_IF_TYPE_DUMMY,
	CTL_SEN_IF_TYPE_SIEPATGEN,  ///< sie patten gen
	CTL_SEN_IF_TYPE_MAX_NUM,
	ENUM_DUMMY4WORD(CTL_SEN_IF_TYPE)
} CTL_SEN_IF_TYPE;

typedef enum {
	CTL_SEN_DATA_FMT_RGB,
	CTL_SEN_DATA_FMT_RGBIR,
	CTL_SEN_DATA_FMT_RCCB,
	CTL_SEN_DATA_FMT_YUV,
	CTL_SEN_DATA_FMT_Y_ONLY,
	CTL_SEN_DATA_FMT_DVS,

	CTL_SEN_DATA_FMT_MAX_NUM,
	ENUM_DUMMY4WORD(CTL_SEN_DATA_FMT)
} CTL_SEN_DATA_FMT;

typedef enum {
	CTL_SEN_PIXDEPTH_8BIT   = 8,    ///< Pixel depth is   8 bits per pixel.
	CTL_SEN_PIXDEPTH_10BIT  = 10,   ///< Pixel depth is  10 bits per pixel.
	CTL_SEN_PIXDEPTH_12BIT  = 12,   ///< Pixel depth is  12 bits per pixel.
	CTL_SEN_PIXDEPTH_14BIT  = 14,   ///< Pixel depth is  14 bits per pixel.
	CTL_SEN_PIXDEPTH_16BIT  = 16,   ///< Pixel depth is  16 bits per pixel.

	ENUM_DUMMY4WORD(CTL_SEN_PIXDEPTH)
} CTL_SEN_PIXDEPTH;

typedef enum {
	CTL_SEN_CLKLANE_1 = 1,
	CTL_SEN_CLKLANE_2 = 2,

	ENUM_DUMMY4WORD(CTL_SEN_CLKLANE)
} CTL_SEN_CLKLANE;

typedef enum {
	CTL_SEN_DATALANE_1 = 1,
	CTL_SEN_DATALANE_2 = 2,
	CTL_SEN_DATALANE_4 = 4,
	CTL_SEN_DATALANE_6 = 6, // 510/520/560 N.S.
	CTL_SEN_DATALANE_8 = 8, // 510/520/560 N.S.

	ENUM_DUMMY4WORD(CTL_SEN_DATALANE)
} CTL_SEN_DATALANE;

typedef enum {
	CTL_SEN_DVI_CCIR601 = 0,        ///< HW need VD,HD output pin
	CTL_SEN_DVI_CCIR656_EAV,        ///< recognize VD,HD by syncode. SIE auto calc active size
	CTL_SEN_DVI_CCIR656_ACT,        ///< recognize VD,HD by syncode. SIE active size set by sensor driver.
	CTL_SEN_DVI_CCIR709,            ///< HW need VD,HD output pin
	CTL_SEN_DVI_CCIR601_1120,       ///< HW need VD,HD output pin
	CTL_SEN_DVI_CCIR656_1120_EAV,   ///< recognize VD,HD by syncode. SIE auto calc active size
	CTL_SEN_DVI_CCIR656_1120_ACT,   ///< recognize VD,HD by syncode. SIE active size set by sensor driver.
	CTL_SEN_DVI_FMT_MAX_NUM,
	ENUM_DUMMY4WORD(CTL_SEN_DVI_FMT)
} CTL_SEN_DVI_FMT;

typedef enum {
	CTL_SEN_FMT_SEL_CCIR601 = 0,    ///< HW need VD,HD output pin
	CTL_SEN_FMT_SEL_CCIR656,        ///< recognize VD,HD by syncode
	CTL_SEN_FMT_SEL_CCIR709,        ///< HW need VD,HD output pin
	CTL_SEN_FMT_SEL_CCIR1120,
	CTL_SEN_FMT_SEL_MAX_NUM,
	ENUM_DUMMY4WORD(CTL_SEN_CCIR_FMT_SEL)
} CTL_SEN_CCIR_FMT_SEL;

typedef enum {
	CTL_SEN_DVI_DATA_MODE_SD = 0,   ///< 8 bits
	CTL_SEN_DVI_DATA_MODE_HD,       ///< 16 bits, HD inv set in init_obj dvi_msblsb_switch
	CTL_SEN_DVI_DATA_MODE_MAX_NUM,
	ENUM_DUMMY4WORD(CTL_SEN_DVI_DATA_MODE)
} CTL_SEN_DVI_DATA_MODE;

typedef struct {
	CTL_SEN_CCIR_FMT_SEL fmt;    ///< [in] ccir format
	BOOL    interlace;          ///< [in] TRUE:CTL_SEN_MODE_CCIR_INTERLACE, FALSE:CTL_SEN_MODE_CCIR
} CTL_SEN_MODESEL_CCIR;

/**
    sensor mode type
*/
typedef enum {
	CTL_SEN_MODE_TYPE_UNKNOWN = 0,

	//single frame
	CTL_SEN_MODE_LINEAR = 1,         ///< linear raw
	CTL_SEN_MODE_BUILTIN_HDR,        ///< bulid in HDR
	CTL_SEN_MODE_CCIR,               ///< DVI
	CTL_SEN_MODE_CCIR_INTERLACE,     ///< DVI INTERLACE

	//multiple frames
	CTL_SEN_MODE_STAGGER_HDR = 5,    ///< stagger HDR
	CTL_SEN_MODE_PDAF,               ///< PDAF

	CTL_SEN_MODE_MAX_NUM,
	ENUM_DUMMY4WORD(CTL_SEN_MODE_TYPE)
} CTL_SEN_MODE_TYPE;

typedef enum {
	CTL_SEN_MSG_TYPE_NONE       = 0x00000000,
	CTL_SEN_MSG_TYPE_OP         = 0x00000001,   ///< dump sensor operation
	CTL_SEN_MSG_TYPE_OP_SIMPLE  = 0x00000002,   ///< dump sensor operation, but skip dump set&get operation after power on, before power off

	ENUM_DUMMY4WORD(CTL_SEN_MSG_TYPE)
} CTL_SEN_MSG_TYPE;

// for CTL_SEN_DBG_SEL_DBG_MSG
#define CTL_SEN_DBG_MSG_PARAM(en, type) (((en & 0x1) << 31) | (type & 0x0FFFFFFF)) ///< en: BOOL, type: CTL_SEN_MSG_TYPE
#define CTL_SEN_DBG_MSG_EN(in) ((in >> 31) & 0x1)
#define CTL_SEN_DBG_MSG_TYPE(in) (in & 0x0FFFFFFF)

typedef enum {
	CTL_SEN_DBG_SEL_DUMP            = 0x00000001,   ///< dump get info (PCTL_SEN_OBJ()->get_cfg), param: dont care
	CTL_SEN_DBG_SEL_DUMP_EXT        = 0x00000002,   ///< dump ext info (PCTL_SEN_OBJ()->init_cfg & PCTL_SEN_OBJ()->get_cfg(CTL_SEN_CFGID_INIT_XX, value)), param: dont care
	CTL_SEN_DBG_SEL_DUMP_DRV        = 0x00000004,   ///< dump sensor driver get info (CTL_SEN_DRV_TAB()->get_cfg(id, CTL_SENDRV_CFGID, value)), param: dont care
	CTL_SEN_DBG_SEL_WAITVD          = 0x00000008,   ///< only support serial sensor (LVDS/CSI/SLVS-EC) & TGE & VX1, param: dont care
	CTL_SEN_DBG_SEL_WAITVD2         = 0x00000010,   ///< only support serial sensor (LVDS/CSI/SLVS-EC) HDR mode, param: dont care
	CTL_SEN_DBG_SEL_WAITFMD         = 0x00000020,   ///< only support serial sensor (LVDS/CSI/SLVS-EC), param: dont care
	CTL_SEN_DBG_SEL_WAITFMD2        = 0x00000040,   ///< only support serial sensor (LVDS/CSI/SLVS-EC) HDR mode, param: dont care
	CTL_SEN_DBG_SEL_DUMP_MAP_TBL    = 0x00000080,   ///< dump (ctl_sen_reg_sendrv/ctl_sen_unreg_sendrv & PCTL_SEN_OBJ()->init_cfg) mapping table, param: dont care
	CTL_SEN_DBG_SEL_DUMP_PROC_TIME  = 0x00000100,   ///< dump ctl sen process time, param: dont care
	CTL_SEN_DBG_SEL_DBG_MSG         = 0x00000200,   ///< enable debug msg, param: CTL_SEN_DBG_MSG_PARAM(CTL_SEN_MSG_TYPE, BOOL)
	CTL_SEN_DBG_SEL_DUMP_CTL        = 0x00000400,   ///< dump ctl_sen inner info
	CTL_SEN_DBG_SEL_WAITVD3         = 0x00000800,   ///< only support serial sensor (LVDS/CSI/SLVS-EC) & VX1 HDR mode, param: dont care
	CTL_SEN_DBG_SEL_WAITVD4         = 0x00001000,   ///< only support serial sensor (LVDS/CSI/SLVS-EC) & VX1 HDR mode, param: dont care
	CTL_SEN_DBG_SEL_WAITFMD3        = 0x00002000,   ///< only support serial sensor (LVDS/CSI/SLVS-EC) HDR mode, param: dont care
	CTL_SEN_DBG_SEL_WAITFMD4        = 0x00004000,   ///< only support serial sensor (LVDS/CSI/SLVS-EC) HDR mode, param: dont care

	CTL_SEN_DBG_SEL_DUMP_ER         = 0x00008000,   ///< dump error code, param: dont care
//	CTL_SEN_DBG_SEL_MANUAL_CFG      = 0x00010000,   ///< user manual config setting, param: CTL_SEN_MANUAL_CFG_PARAM(en, item, value)
	CTL_SEN_DBG_SEL_LV              = 0x00020000,   ///< set dbg level, param: CTL_SEN_DBG_LV (dont care id)
	CTL_SEN_DBG_SEL_WAIT_VD_TO_SIE  = 0x00040000,   ///< only support serial sensor (LVDS/CSI/SLVS-EC), param: output SIE idx (start from 0)
	CTL_SEN_DBG_SEL_WAIT_FMD_TO_SIE = 0x00080000,   ///< only support serial sensor (LVDS/CSI/SLVS-EC), param: output SIE idx (start from 0)

	CTL_SEN_DBG_SEL_WAITVD_TGE      = CTL_SEN_DBG_SEL_WAITVD | CTL_SEN_TGE_TAG,  ///< Valid for TGE (CTL_SEN_INIT_CFG_OBJ/cfg_obj.if_cfg.tge.tge_en = ENABLE), param: dont care
	CTL_SEN_DBG_SEL_WAITVD_VX1      = CTL_SEN_DBG_SEL_WAITVD | CTL_SEN_VX1_TAG,  ///< Valid for VX1 (sensor driver setting CTL_SEN_CMDIF_TYPE_VX1), param: dont care
	CTL_SEN_DBG_SEL_WAITVD2_VX1     = CTL_SEN_DBG_SEL_WAITVD2 | CTL_SEN_VX1_TAG,  ///< Valid for VX1 (sensor driver setting CTL_SEN_CMDIF_TYPE_VX1), param: dont care
	CTL_SEN_DBG_SEL_WAITVD3_VX1     = CTL_SEN_DBG_SEL_WAITVD3 | CTL_SEN_VX1_TAG,  ///< Valid for VX1 (sensor driver setting CTL_SEN_CMDIF_TYPE_VX1), param: dont care
	CTL_SEN_DBG_SEL_WAITVD4_VX1     = CTL_SEN_DBG_SEL_WAITVD4 | CTL_SEN_VX1_TAG,  ///< Valid for VX1 (sensor driver setting CTL_SEN_CMDIF_TYPE_VX1), param: dont care

	ENUM_DUMMY4WORD(CTL_SEN_DBG_SEL)
} CTL_SEN_DBG_SEL;

/*******************************************************************************/
/*                            for sensor ctrl if                               */
/*******************************************************************************/
typedef enum {
	CTL_SEN_INTE_NONE       = 0x00000000,
	CTL_SEN_INTE_VD         = 0x00000001,   ///< Valid for CSI/LVDS/SLVSEC/Vx1
	CTL_SEN_INTE_VD2        = 0x00000002,   ///< Valid for LVDS/SLVSEC/Vx1
	CTL_SEN_INTE_VD3        = 0x00000004,   ///< Valid for LVDS/Vx1, 510/520/560 N.S.
	CTL_SEN_INTE_VD4        = 0x00000008,   ///< Valid for LVDS/Vx1, 510/520/560 N.S.
	CTL_SEN_INTE_FRAMEEND   = 0x00000010,   ///< Valid for CSI/LVDS/SLVSEC
	CTL_SEN_INTE_FRAMEEND2  = 0x00000020,   ///< Valid for CSI/LVDS/SLVSEC
	CTL_SEN_INTE_FRAMEEND3  = 0x00000040,   ///< Valid for CSI/LVDS, 510/520/560 N.S.
	CTL_SEN_INTE_FRAMEEND4  = 0x00000080,   ///< Valid for CSI/LVDS, 510/520/560 N.S.
	CTL_SEN_INTE_ABORT      = 0x80000000,   ///< Waiting event timeout abort.

	CTL_SEN_INTE_VD_TO_SIE0     = 0x00000100,   ///< Valid for CSI/LVDS, 528/560 support
	CTL_SEN_INTE_VD_TO_SIE1     = 0x00000200,   ///< Valid for CSI/LVDS, 528/560 support
	CTL_SEN_INTE_VD_TO_SIE3     = 0x00000400,   ///< Valid for CSI/LVDS, 510/520/560 N.S., 528 support
	CTL_SEN_INTE_VD_TO_SIE4     = 0x00000800,   ///< Valid for CSI/LVDS, 510/520/560 N.S., 528 support
	CTL_SEN_INTE_FMD_TO_SIE0    = 0x00001000,   ///< Valid for CSI/LVDS, 528/560 support
	CTL_SEN_INTE_FMD_TO_SIE1    = 0x00002000,   ///< Valid for CSI/LVDS, 528/560 support
	CTL_SEN_INTE_FMD_TO_SIE3    = 0x00004000,   ///< Valid for CSI/LVDS, 510/520/560 N.S., 528 support
	CTL_SEN_INTE_FMD_TO_SIE4    = 0x00008000,   ///< Valid for CSI/LVDS, 510/520/560 N.S., 528 support

	CTL_SEN_INTE_TGE_VD     = CTL_SEN_INTE_VD | CTL_SEN_TGE_TAG,    ///< Valid for TGE (CTL_SEN_INIT_CFG_OBJ/cfg_obj.if_cfg.tge.tge_en = ENABLE)
	CTL_SEN_INTE_VX1_VD     = CTL_SEN_INTE_VD | CTL_SEN_VX1_TAG,    ///< Valid for VX1 (sensor driver setting CTL_SEN_CMDIF_TYPE_VX1), 510/520/560 N.S.
	CTL_SEN_INTE_VX1_VD2     = CTL_SEN_INTE_VD2 | CTL_SEN_VX1_TAG,  ///< Valid for VX1 (sensor driver setting CTL_SEN_CMDIF_TYPE_VX1), 510/520/560 N.S.
	CTL_SEN_INTE_VX1_VD3     = CTL_SEN_INTE_VD3 | CTL_SEN_VX1_TAG,  ///< Valid for VX1 (sensor driver setting CTL_SEN_CMDIF_TYPE_VX1), 510/520/560 N.S.
	CTL_SEN_INTE_VX1_VD4     = CTL_SEN_INTE_VD4 | CTL_SEN_VX1_TAG,  ///< Valid for VX1 (sensor driver setting CTL_SEN_CMDIF_TYPE_VX1), 510/520/560 N.S.

	ENUM_DUMMY4WORD(CTL_SEN_INTE)
} CTL_SEN_INTE;

/**
    sensor output dest sel
*/
typedef enum {
	CTL_SEN_OUTPUT_DEST_AUTO    = 0x00, ///< only support linear mode
	CTL_SEN_OUTPUT_DEST_SIE1    = (1 << CTL_SEN_ID_1),
	CTL_SEN_OUTPUT_DEST_SIE2    = (1 << CTL_SEN_ID_2),
	CTL_SEN_OUTPUT_DEST_SIE3    = (1 << CTL_SEN_ID_3), // 510/520/560 N.S.
	CTL_SEN_OUTPUT_DEST_SIE4    = (1 << CTL_SEN_ID_4), // 510/520/560 N.S.
	CTL_SEN_OUTPUT_DEST_SIE5    = (1 << CTL_SEN_ID_5), // 510/520/560 N.S.
	CTL_SEN_OUTPUT_DEST_SIE6    = (1 << CTL_SEN_ID_6), // 510/520/560 N.S.
	CTL_SEN_OUTPUT_DEST_SIE7    = (1 << CTL_SEN_ID_7), // 510/520/560 N.S.
	CTL_SEN_OUTPUT_DEST_SIE8    = (1 << CTL_SEN_ID_8), // 510/520/560 N.S.
	CTL_SEN_OUTPUT_DEST_ALL     = 0xff,
	ENUM_DUMMY4WORD(CTL_SEN_OUTPUT_DEST)
} CTL_SEN_OUTPUT_DEST;


/*******************************************************************************/
/*                            for dx camera                                    */
/*******************************************************************************/

/**
    Sensor sie mclk information
*/
typedef enum {
	CTL_SEN_SIEMCLK_SRC_DFT     = 0,    ///< id 0: PLL_CLK_SIEMCLK, id 1~7: PLL_CLK_SIEMCLK2
	CTL_SEN_SIEMCLK_SRC_MCLK    = 1,    ///< KDRV_SSENIF_SIEMCLK_ENABLE
	CTL_SEN_SIEMCLK_SRC_MCLK2   = 2,    ///< KDRV_SSENIF_SIEMCLK2_ENABLE
	CTL_SEN_SIEMCLK_SRC_MCLK3   = 3,    ///< KDRV_SSENIF_SIEMCLK3_ENABLE

	CTL_SEN_SIEMCLK_SRC_SPCLK   = 4,    ///< KDRV_SSENIF_SPCLK_ENABLE
	CTL_SEN_SIEMCLK_SRC_SPCLK2  = 5,    ///< KDRV_SSENIF_SPCLK2_ENABLE

	CTL_SEN_SIEMCLK_SRC_MAX     = 6,

	CTL_SEN_SIEMCLK_SRC_IGNORE = CTL_SEN_IGNORE, // extern MCLK source

	ENUM_DUMMY4WORD(CTL_SEN_SIEMCLK_SRC)
} CTL_SEN_SIEMCLK_SRC;

/**
    Sensor sie signal information
*/
typedef enum {
	CTL_SEN_ACTIVE_HIGH = 0,    ///< sensor active high, SIE non-inverse
	CTL_SEN_ACTIVE_LOW = 1,     ///< sensor active low, SIE inverse
	ENUM_DUMMY4WORD(CTL_SEN_ACTIVE_SEL)
} CTL_SEN_ACTIVE_SEL;

typedef enum {
	CTL_SEN_PHASE_RISING,  ///< rising edge latch/trigger
	CTL_SEN_PHASE_FALLING, ///< falling edge latch/trigger
	ENUM_DUMMY4WORD(CTL_SEN_PHASE_SEL)
} CTL_SEN_PHASE_SEL;

/**
    Sensor interface mapping
    CTL_SEN_ID (x) <-> interface (y) <-> CTL_SIE_ID (x) : SENSOR_(x)_MAP_IF_(interface)_(y)
    CTL_SEN_MAP_IF_DEFAULT:
    interface LVDSMIPI:
    SENSOR_1_MAP_IF_LVDSMIPI_1/SENSOR_2_MAP_IF_LVDSMIPI_2/SENSOR_3_MAP_IF_LVDSMIPI_3/SENSOR_4_MAP_IF_LVDSMIPI_4/
    SENSOR_5_MAP_IF_LVDSMIPI_5/SENSOR_6_MAP_IF_LVDSMIPI_6/SENSOR_7_MAP_IF_LVDSMIPI_7/SENSOR_8_MAP_IF_LVDSMIPI_8/
    interface SLVSEC:
    SENSOR_1_MAP_IF_SLVSEC_1

*/

typedef enum {
	CTL_SEN_I2C_W_ADDR_DFT = 0,
	CTL_SEN_I2C_W_ADDR_OPTION1 = 1,
	CTL_SEN_I2C_W_ADDR_OPTION2 = 2,
	CTL_SEN_I2C_W_ADDR_OPTION3 = 3,
	CTL_SEN_I2C_W_ADDR_OPTION4 = 4,
	CTL_SEN_I2C_W_ADDR_OPTION5 = 5,

	CTL_SEN_I2C_W_ADDR_SEL_MAX_NUM,
	ENUM_DUMMY4WORD(CTL_SEN_I2C_W_ADDR_SEL)
} CTL_SEN_I2C_W_ADDR_SEL;

typedef enum {
	CTL_SEN_I2C_CH_1,          ///< I2C channel 1
	CTL_SEN_I2C_CH_2,          ///< I2C channel 2
	CTL_SEN_I2C_CH_3,          ///< I2C channel 3
	CTL_SEN_I2C_CH_4,          ///< I2C channel 4
	CTL_SEN_I2C_CH_5,          ///< I2C channel 5

	CTL_SEN_I2C_CH_MAX_NUM,
	ENUM_DUMMY4WORD(CTL_SEN_I2C_CH)
} CTL_SEN_I2C_CH;

/**
    SIE 1/3 VD/HD signal source (only in parallel mode + slave sensor)
*/
typedef enum {
	CTL_SEN_TGE_SIE1_VD_SRC_CH1 = 0,              // use ch1 to be SIE1 VD source
	CTL_SEN_TGE_SIE1_VD_SRC_CH3,                  // use ch3 to be SIE1 VD source
	CTL_SEN_TGE_SIE3_VD_SRC_CH5,                  // use ch5 to be SIE3 VD source
	CTL_SEN_TGE_SIE3_VD_SRC_CH7,                  // use ch7 to be SIE3 VD source
	ENUM_DUMMY4WORD(CTL_SEN_TGE_SIE_VD_SRC)
} CTL_SEN_TGE_SIE_VD_SRC;

typedef struct {
	BOOL tge_en;    ///< set TRUE for used tge, slave sensor (CTL_SEN_SIGNAL_SLAVE) must be enable
	BOOL swap;      ///< swap CH1 & 5, or CH2 & 6, or CH3 & 7, or CH4 & 8, 510/520/560 N.S.
	CTL_SEN_TGE_SIE_VD_SRC sie_vd_src;  ///< SIE 1/3 VD/HD signal source (only in parallel mode + slave sensor), 510/520/560 N.S.
} CTL_SEN_IF_TGE;

typedef enum {
	CTL_SEN_CLANE_SEL_CSI0_USE_C0,       ///< [680] MIPI CSI0 uses HSI_CK0 as clock lane.
	CTL_SEN_CLANE_SEL_CSI0_USE_C2,       ///< [680] MIPI CSI0 uses HSI_CK2 as clock lane.
	CTL_SEN_CLANE_SEL_CSI1_USE_C4,      ///< [680] MIPI CSI1 uses HSI_CK4 as clock lane.
	CTL_SEN_CLANE_SEL_CSI1_USE_C6,      ///< [680] MIPI CSI1 uses HSI_CK6 as clock lane.

	CTL_SEN_CLANE_SEL_CSI2_USE_C2,      ///< [680] MIPI CSI2 uses HSI_CK2 as clock lane.
	CTL_SEN_CLANE_SEL_CSI3_USE_C6,      ///< [680] MIPI CSI3 uses HSI_CK6 as clock lane.
	CTL_SEN_CLANE_SEL_CSI4_USE_C1,      ///< [680] MIPI CSI4 uses HSI_CK1 as clock lane.
	CTL_SEN_CLANE_SEL_CSI5_USE_C3,      ///< [680] MIPI CSI5 uses HSI_CK3 as clock lane.
	CTL_SEN_CLANE_SEL_CSI6_USE_C5,      ///< [680] MIPI CSI6 uses HSI_CK5 as clock lane.
	CTL_SEN_CLANE_SEL_CSI7_USE_C7,      ///< [680] MIPI CSI7 uses HSI_CK7 as clock lane.

	CTL_SEN_CLANE_SEL_LVDS0_USE_C0C4,    ///< [680] LVDS0 uses HSI_CK0 as clock lane in one clock lane sensor.
	///< LVDS0 uses HSI_CK0+CK4 as clock lanes in two clock lanes sensor.(such as HiSPi 8 lanes)
	CTL_SEN_CLANE_SEL_LVDS0_USE_C2C6,    ///< [680] LVDS0 uses HSI_CK2 as clock lane in one clock lane sensor.
	///< LVDS0 uses HSI_CK2+CK6 as clock lanes in two clock lanes sensor.(such as HiSPi 8 lanes)
	CTL_SEN_CLANE_SEL_LVDS1_USE_C4, ///< [680] LVDS1 uses HSI_CK4 as clock lane.
	CTL_SEN_CLANE_SEL_LVDS1_USE_C6, ///< [680] LVDS1 uses HSI_CK6 as clock lane.

	CTL_SEN_CLANE_SEL_LVDS2_USE_C2, ///< [680] LVDS2 uses HSI_CK2 as clock lane.
	CTL_SEN_CLANE_SEL_LVDS3_USE_C6, ///< [680] LVDS3 uses HSI_CK6 as clock lane.
	CTL_SEN_CLANE_SEL_LVDS4_USE_C1, ///< [680] LVDS4 uses HSI_CK1 as clock lane.
	CTL_SEN_CLANE_SEL_LVDS5_USE_C3, ///< [680] LVDS5 uses HSI_CK3 as clock lane.
	CTL_SEN_CLANE_SEL_LVDS6_USE_C5, ///< [680] LVDS6 uses HSI_CK5 as clock lane.
	CTL_SEN_CLANE_SEL_LVDS7_USE_C7, ///< [680] LVDS7 uses HSI_CK7 as clock lane.

	CTL_SEN_CLANE_SEL_CSI1_USE_C1,      ///< [520][560] MIPI CSI1 uses HSI_CK1 as clock lane.
	CTL_SEN_CLANE_SEL_LVDS1_USE_C1,     ///< [520][560] LVDS1 uses HSI_CK1 as clock lane in one clock lane sensor.

} CTL_SEN_CLANE_SEL;


/*
    KDRV controller id(CSI/LVDS/TGE) or ch(TGE) information
*/
#define CTL_SEN_DRVDEV_OFS_CSI      0
#define CTL_SEN_DRVDEV_OFS_LVDS     4
#define CTL_SEN_DRVDEV_OFS_TGE      8
#define CTL_SEN_DRVDEV_OFS_VX1      12
#define CTL_SEN_DRVDEV_OFS_SLVSEC   16

#define CTL_SEN_DRVDEV_MASK_CSI(drvdev)         (drvdev & (0x0000000F << CTL_SEN_DRVDEV_OFS_CSI))
#define CTL_SEN_DRVDEV_MASK_LVDS(drvdev)        (drvdev & (0x0000000F << CTL_SEN_DRVDEV_OFS_LVDS))
#define CTL_SEN_DRVDEV_MASK_TGE(drvdev)         (drvdev & (0x0000000F << CTL_SEN_DRVDEV_OFS_TGE))
#define CTL_SEN_DRVDEV_MASK_VX1(drvdev)         (drvdev & (0x0000000F << CTL_SEN_DRVDEV_OFS_VX1))
#define CTL_SEN_DRVDEV_MASK_SLVSEC(drvdev)      (drvdev & (0x0000000F << CTL_SEN_DRVDEV_OFS_SLVSEC))

// KDRV DEV IDX, start from 0
#define CTL_SEN_DRVDEV_IDX_CSI(drvdev)      ((CTL_SEN_DRVDEV_MASK_CSI(drvdev) - CTL_SEN_DRVDEV_CSI_BASE) >> CTL_SEN_DRVDEV_OFS_CSI) // KDRV_SSENIF_ENGINE_CSI0 ~ KDRV_SSENIF_ENGINE_CSIX
#define CTL_SEN_DRVDEV_IDX_LVDS(drvdev)     ((CTL_SEN_DRVDEV_MASK_LVDS(drvdev) - CTL_SEN_DRVDEV_LVDS_BASE) >> CTL_SEN_DRVDEV_OFS_LVDS) // KDRV_SSENIF_ENGINE_LVDS0 ~ KDRV_SSENIF_ENGINE_LVDSX
#define CTL_SEN_DRVDEV_IDX_TGE(drvdev)      ((CTL_SEN_DRVDEV_MASK_TGE(drvdev) - CTL_SEN_DRVDEV_TGE_BASE) >> CTL_SEN_DRVDEV_OFS_TGE) // KDRV_VDOCAP_TGE_ENGINE0/KDRV_TGE_VDHD_CH1 ~ KDRV_VDOCAP_TGE_ENGINE0/KDRV_TGE_VDHD_CH8
#define CTL_SEN_DRVDEV_IDX_VX1(drvdev)      ((CTL_SEN_DRVDEV_MASK_VX1(drvdev) - CTL_SEN_DRVDEV_VX1_BASE) >> CTL_SEN_DRVDEV_OFS_VX1) // KDRV_SSENIF_ENGINE_VX1_0 ~ KDRV_SSENIF_ENGINE_VX1_X
#define CTL_SEN_DRVDEV_IDX_SLVSEC(drvdev)   ((CTL_SEN_DRVDEV_MASK_SLVSEC(drvdev) - CTL_SEN_DRVDEV_SLVSEC_BASE) >> CTL_SEN_DRVDEV_OFS_SLVSEC) // KDRV_SSENIF_ENGINE_SLVSEC0 ~ KDRV_SSENIF_ENGINE_SLVSECX

typedef enum {
	CTL_SEN_DRVDEV_CSI_BASE    = 0x00000001,
	CTL_SEN_DRVDEV_CSI_0       = CTL_SEN_DRVDEV_CSI_BASE,   // select CSI controller 0
	CTL_SEN_DRVDEV_CSI_1       = 0x00000002,                // select CSI controller 1
	CTL_SEN_DRVDEV_CSI_2       = 0x00000003,                // select CSI controller 2 [520/528/560 N.S.]
	CTL_SEN_DRVDEV_CSI_3       = 0x00000004,                // select CSI controller 3 [520/528/560 N.S.]
	CTL_SEN_DRVDEV_CSI_4       = 0x00000005,                // select CSI controller 4 [520/528/560 N.S.]
	CTL_SEN_DRVDEV_CSI_5       = 0x00000006,                // select CSI controller 5 [520/528/560 N.S.]
	CTL_SEN_DRVDEV_CSI_6       = 0x00000007,                // select CSI controller 6 [520/528/560 N.S.]
	CTL_SEN_DRVDEV_CSI_7       = 0x00000008,                // select CSI controller 7 [520/528/560 N.S.]
	CTL_SEN_DRVDEV_CSI_MAX     = CTL_SEN_DRVDEV_CSI_7,      // must <= 0x0000000F

	CTL_SEN_DRVDEV_LVDS_BASE   = 0x00000010,
	CTL_SEN_DRVDEV_LVDS_0      = CTL_SEN_DRVDEV_LVDS_BASE,  // select LVDS controller 0
	CTL_SEN_DRVDEV_LVDS_1      = 0x00000020,                // select LVDS controller 1
	CTL_SEN_DRVDEV_LVDS_2      = 0x00000030,                // select LVDS controller 2 [520/528/560 N.S.]
	CTL_SEN_DRVDEV_LVDS_3      = 0x00000040,                // select LVDS controller 3 [520/528/560 N.S.]
	CTL_SEN_DRVDEV_LVDS_4      = 0x00000050,                // select LVDS controller 4 [520/528/560 N.S.]
	CTL_SEN_DRVDEV_LVDS_5      = 0x00000060,                // select LVDS controller 5 [520/528/560 N.S.]
	CTL_SEN_DRVDEV_LVDS_6      = 0x00000070,                // select LVDS controller 6 [520/528/560 N.S.]
	CTL_SEN_DRVDEV_LVDS_7      = 0x00000080,                // select LVDS controller 7 [520/528/560 N.S.]
	CTL_SEN_DRVDEV_LVDS_MAX    = CTL_SEN_DRVDEV_LVDS_7,     // must <= 0x000000F0

	CTL_SEN_DRVDEV_TGE_BASE    = 0x00000100,
	CTL_SEN_DRVDEV_TGE_0       = CTL_SEN_DRVDEV_TGE_BASE,   // select TGE VD0/HD0 generator
	CTL_SEN_DRVDEV_TGE_1       = 0x00000200,                // select TGE VD1/HD1 generator
	CTL_SEN_DRVDEV_TGE_2       = 0x00000300,                // select TGE VD2/HD2 generator [520/528/560 N.S.]
	CTL_SEN_DRVDEV_TGE_3       = 0x00000400,                // select TGE VD3/HD3 generator [520/528/560 N.S.]
	CTL_SEN_DRVDEV_TGE_4       = 0x00000500,                // select TGE VD4/HD4 generator [520/528/560 N.S.]
	CTL_SEN_DRVDEV_TGE_5       = 0x00000600,                // select TGE VD5/HD5 generator [520/528/560 N.S.]
	CTL_SEN_DRVDEV_TGE_6       = 0x00000700,                // select TGE VD6/HD6 generator [520/528/560 N.S.]
	CTL_SEN_DRVDEV_TGE_7       = 0x00000800,                // select TGE VD7/HD7 generator [520/528/560 N.S.]
	CTL_SEN_DRVDEV_TGE_MAX     = CTL_SEN_DRVDEV_TGE_7,      // must <= 0x00000F00

	CTL_SEN_DRVDEV_VX1_BASE    = 0x00001000,
	CTL_SEN_DRVDEV_VX1_0       = CTL_SEN_DRVDEV_VX1_BASE,   // select vx1 controller 0, mapping to KDRV_SSENIF_ENGINE_VX1_0, NT96680 only support SIE 1~5 [520/528/560 N.S.]
	CTL_SEN_DRVDEV_VX1_1       = 0x00002000,                // select vx1 controller 1, mapping to KDRV_SSENIF_ENGINE_VX1_1, NT96680 only support SIE 4,6 [520/528/560 N.S.]
	CTL_SEN_DRVDEV_VX1_MAX     = CTL_SEN_DRVDEV_VX1_1,      // must <= 0x0000F000

	CTL_SEN_DRVDEV_SLVSEC_BASE = 0x00010000,
	CTL_SEN_DRVDEV_SLVSEC_0    = CTL_SEN_DRVDEV_SLVSEC_BASE,    // select SLVS-EC controller 0 [520/528/560 N.S.]
	CTL_SEN_DRVDEV_SLVSEC_MAX  = CTL_SEN_DRVDEV_SLVSEC_0,       // must <= 0x000F0000

} CTL_SEN_DRVDEV;


/*******************************************************************************/
/*                            for sensor driver                                */
/*******************************************************************************/

typedef struct {
	CTL_SEN_MODE mode;                  ///< sensor mode
	UINT32 frame_rate;                  ///< sensor frame rate, fps * 100
} CTL_SENDRV_CHGMODE_OBJ;

/**
    sensor support property
*/
typedef enum {
	CTL_SEN_SUPPORT_PROPERTY_NONE     = 0x00000000,
	CTL_SEN_SUPPORT_PROPERTY_MIRROR   = 0x00000001,
	CTL_SEN_SUPPORT_PROPERTY_FLIP     = 0x00000002,
	CTL_SEN_SUPPORT_PROPERTY_CHGFPS   = 0x00000004,

	CTL_SEN_SUPPORT_PROPERTY_ALL      = 0x00000007,

	ENUM_DUMMY4WORD(CTL_SEN_SUPPORT_PROPERTY)
} CTL_SEN_SUPPORT_PROPERTY;

typedef enum {
	CTL_SEN_VENDOR_SONY,        ///< SONY
	CTL_SEN_VENDOR_OMNIVISION,  ///< Omni-Vision
	CTL_SEN_VENDOR_ONSEMI,      ///< ON-Semi
	CTL_SEN_VENDOR_PANASONIC,   ///< Panasonic
	CTL_SEN_VENDOR_OTHERS,      ///< Sensor vendor not specified above.

	ENUM_DUMMY4WORD(CTL_SEN_VENDOR)
} CTL_SEN_VENDOR;

/**
    sensor signal type
*/
typedef enum {
	CTL_SEN_SIGNAL_MASTER = 0,       ///< sensor output HD and VD signal
	CTL_SEN_SIGNAL_SLAVE,            ///< dsp output HD and VD signal
	CTL_SEN_SIGNAL_MAX_NUM,
	ENUM_DUMMY4WORD(CTL_SEN_SIGNAL_TYPE)
} CTL_SEN_SIGNAL_TYPE;

typedef struct {
	CTL_SEN_ACTIVE_SEL vd_inv;
	CTL_SEN_ACTIVE_SEL hd_inv;
	CTL_SEN_PHASE_SEL vd_phase;
	CTL_SEN_PHASE_SEL hd_phase;
	CTL_SEN_PHASE_SEL data_phase;
} CTL_SEN_SIGNAL_INFO;

#define CTL_SEN_NAME_LEN 32
typedef struct {
	CHAR name[CTL_SEN_NAME_LEN];        ///< [out]
	CTL_SEN_VENDOR vendor;              ///< [out]
	UINT32 max_senmode;                 ///< [out]
	CTL_SEN_SUPPORT_PROPERTY property;  ///< [out] sensor support property
	UINT32 sync_timing;                 ///< [out] sync timing for Exposure time & gain(VD)
} CTL_SENDRV_GET_ATTR_BASIC_PARAM;

typedef struct {
	CTL_SEN_SIGNAL_TYPE type; // [out]
	CTL_SEN_SIGNAL_INFO info; // [out], only for parallel sensor and salve sensor (tge)
} CTL_SENDRV_GET_ATTR_SIGNAL_PARAM;

typedef struct {
	CTL_SEN_I2C_W_ADDR_SEL w_addr_sel; // Backward compatible
	UINT8 w_addr;
} CTL_SEN_I2C_WADDR;

typedef struct {
	CTL_SEN_I2C_WADDR w_addr_info[CTL_SEN_I2C_W_ADDR_SEL_MAX_NUM]; // Backward compatible
	CTL_SEN_I2C_WADDR cur_w_addr_info;
	CTL_SEN_I2C_CH ch;                 ///< I2C channel
	UINT32  s_clk;                   ///< I2C clock, set 0 for default
} CTL_SENDRV_I2C;

typedef struct {
	SIF_CH      channel;     ///< sif channel
	UINT32      bus_clk;     ///< unit:HZ, map to SIF_CONFIG_ID_BUSCLOCK
	UINT32      sen_d_s;
	UINT32      sen_h;      ///< map to sif hold time,ref:SIF_CONFIG_ID_SENH
} CTL_SENDRV_SIF;

typedef struct {
	UINT32 tx241_clane_speed;       ///< Sensor clock lane speed in bps, only tx_type == KDRV_SSENIFVX1_TXTYPE_THCV241 need to set
	UINT32 tx241_input_clk_freq;    ///< Assign 241 input oscillator frequency. Input value in Hertz(valid range is 10000000~40000000), only tx_type == KDRV_SSENIFVX1_TXTYPE_THCV241 need to set
	UINT32 tx241_real_mclk;         ///< Sensor real operation MCLK frequency provided , only tx_type == KDRV_SSENIFVX1_TXTYPE_THCV241 need to set
} CTL_SENDRV_VX1;

typedef struct {
	CTL_SEN_CMDIF_TYPE type;    ///< [out]
	union {
		CTL_SENDRV_I2C i2c;     ///< [out]
		CTL_SENDRV_SIF sif;     ///< [out]
	} info;
	CTL_SENDRV_VX1 vx1;         ///< [out], 510/520/560 N.S.
} CTL_SENDRV_GET_ATTR_CMDIF_PARAM;

/**
    Sensor lvds ctrl pattern
    ctrl_vd[3]: virtual sensor 3
    ctrl_vd[2]: virtual sensor 2
    ctrl_vd[1]: virtual sensor 1
    ctrl_vd[0]: virtual sensor 0
*/
typedef struct {
	UINT16 ctrl_hd; ///< lvds ctrl words
	UINT16 ctrl_vd[CTL_SEN_MFRAME_MAX_NUM]; ///< lvds ctrl words
} CTL_SEN_LVDS_CTRLPTN;

typedef struct {
	UINT32  sensor_pin_order;       ///< sensor pin order, range: 0 ~ CTL_SEN_SER_MAX_DATALANE
	CTL_SEN_LVDS_CTRLPTN ctrl_ptn;  ///< lvds ctrl pattern
} CTL_SEN_LVDS_CTRLPTN_INFO;

/**
    Sensor lvds sync code pattern
*/
typedef struct {
	UINT32 num;             ///< sync code total number
	UINT32 code[7];         ///< sync code
} CTL_SEN_LVDS_SYNC_CODE_PTN;

typedef struct {
	CTL_SEN_LVDS_CTRLPTN *(*fp_get_ctrl_ptn)(UINT32 idx, CTL_SEN_PIXDEPTH pixel_depth, CTL_SEN_MODE_TYPE mode_type);///< [OPTION] only CTL_SEN_VENDOR_OTHERS need to set
} CTL_SENDRV_LVDS;

typedef struct {
	CTL_SEN_IF_TYPE type;       ///< [in]
	union {
		CTL_SENDRV_LVDS lvds;   ///< [out]
	} info;
} CTL_SENDRV_GET_ATTR_IF_PARAM;

typedef struct {
	UINT32 mux_data_num;    ///< mux data number
} CTL_SEN_PARA_MUX_INFO;

typedef struct {
	UINT32 frame_rate;                      ///< [in] fps * 100
	USIZE size;                             ///< [in]
	CTL_SEN_IF_TYPE if_type;                ///< [in]
	CTL_SEN_DATA_FMT data_fmt;              ///< [in] sensor output data format
	UINT32 frame_num;                       ///< [in]
	CTL_SEN_PIXDEPTH pixdepth;              ///< [in] sensor output pixel depth, set CTL_SEN_IGNORE for auto (sensor driver need to cover CTL_SEN_IGNORE)
	CTL_SEN_MODESEL_CCIR ccir;              ///< [in] only CTL_SEN_DATA_FMT_YUV, CTL_SEN_DATA_FMT_Y_ONLY need to set
	BOOL mux_singnal_en;                    ///< [in] only support CTL_SEN_IF_TYPE_PARALLEL
	CTL_SEN_PARA_MUX_INFO mux_signal_info;  ///< [in] only (mux_singnal_en == 1) need to set
	CTL_SEN_MODE_TYPE mode_type_sel;        ///< [in] if user input 0 or CTL_SEN_IGNORE (sendrv must ignore this parameters)
	CTL_SEN_DATALANE data_lane;             ///< [in] if user input 0 or CTL_SEN_IGNORE (sendrv must ignore this parameters)
	CTL_SEN_MODE mode;                      ///< [out]
} CTL_SENDRV_GET_MODESEL_PARAM;

typedef struct {
	UINT32 temp; // [out]
} CTL_SENDRV_GET_TEMP_PARAM;

typedef struct {
	UINT32 chg_fps;     ///< [out]chgmode frame rate (fps * 100)
	UINT32 cur_fps;     ///< [out]current frame rate (fps * 100)
} CTL_SENDRV_GET_FPS_PARAM;

typedef struct {
	CTL_SEN_MODE mode;  ///< [in] if input CTL_SEN_MODE_PWR, pls output power on information
	CTL_SEN_SIEMCLK_SRC mclk_src;   ///< [out] cannot change by sensor mode
	UINT32 mclk;        ///< [out] mclk frequency
	UINT32 pclk;        ///< [out] pclk frequency
	UINT32 data_rate;   ///< [out] data rate
} CTL_SENDRV_GET_SPEED_PARAM;

/**
    Sensor data start pixel or pattern
*/
typedef enum {
	// RGB start pixel
	CTL_SEN_STPIX_R = 0,            ///< start pixel R
	CTL_SEN_STPIX_GR = 1,           ///< start pixel GR
	CTL_SEN_STPIX_GB = 2,           ///< start pixel GB
	CTL_SEN_STPIX_B = 3,            ///< start pixel B

	// RGBIR data format
	CTL_SEN_STPIX_RGBIR_RGBG_GIGI = 100,  ///< start pixel RGBG/GIrGIr
	CTL_SEN_STPIX_RGBIR_GBGR_IGIG = 101,  ///< start pixel GBGR/IrGIrG
	CTL_SEN_STPIX_RGBIR_GIGI_BGRG = 102,  ///< start pixel GIrGIr/GBRG
	CTL_SEN_STPIX_RGBIR_IGIG_GRGB = 103,  ///< start pixel IrGIrG/GRGB
	CTL_SEN_STPIX_RGBIR_BGRG_GIGI = 104,  ///< start pixel BGRG/GIrGIr
	CTL_SEN_STPIX_RGBIR_GRGB_IGIG = 105,  ///< start pixel GRGB/IrGIrG
	CTL_SEN_STPIX_RGBIR_GIGI_RGBG = 106,  ///< start pixel GIrGIr/RGBG
	CTL_SEN_STPIX_RGBIR_IGIG_GBGR = 107,  ///< start pixel IrGIrG/GBGR

	// RCCB start pixel
	CTL_SEN_STPIX_RCCB_RC = 200,    ///< start pixel RC
	CTL_SEN_STPIX_RCCB_CR = 201,    ///< start pixel CR
	CTL_SEN_STPIX_RCCB_CB = 202,    ///< start pixel CB
	CTL_SEN_STPIX_RCCB_BC = 203,    ///< start pixel BC

	// Y only
	CTL_SEN_STPIX_Y_ONLY = 301,     ///< y pixel only

	// YUV start pixel
	CTL_SEN_STPIX_YUV_YUYV = 400,    ///< start pixel YUYV
	CTL_SEN_STPIX_YUV_YVYU = 401,    ///< start pixel YVYU
	CTL_SEN_STPIX_YUV_UYVY = 402,    ///< start pixel UYVY
	CTL_SEN_STPIX_YUV_VYUY = 403,    ///< start pixel VYUY

	// None
	CTL_SEN_STPIX_NONE = 0xFFFF,

	CTL_SEN_STPIX_MAX_NUM,
	ENUM_DUMMY4WORD(CTL_SEN_STPIX)
} CTL_SEN_STPIX;

/**
    Sensor data format
*/
typedef enum {
	CTL_SEN_FMT_POGRESSIVE = 0,      ///< pogressive mode
	CTL_SEN_FMT_MAX_NUM,
	ENUM_DUMMY4WORD(CTL_SEN_FMT)
} CTL_SEN_FMT;


/**
    Sensor signal

    @note relative falling edge
*/
typedef struct {
	UINT32 hd_sync;     ///< hd sync
	UINT32 hd_period;   ///< hd period
	UINT32 vd_sync;     ///< vd sync
	UINT32 vd_period;   ///< vd period
} CTL_SEN_MODE_SIGNAL;


/**
    Sensor GAIN structure
*/
typedef struct {
	UINT32 min; ///< min gain (1X = 1 x 1000 = 1000)
	UINT32 max; ///< max gain (1X = 1 x 1000 = 1000)
} CTL_SEN_MODE_GAIN;

/**
    multi-frame information
*/
#define SEN_BIT_OFS_NONE (0)
#define SEN_BIT_OFS_0 (1<<0)
#define SEN_BIT_OFS_1 (1<<1)
#define SEN_BIT_OFS_2 (1<<2)
#define SEN_BIT_OFS_3 (1<<3)
#define SEN_BIT_OFS_4 (1<<4)
#define SEN_BIT_OFS_5 (1<<5)
#define SEN_BIT_OFS_6 (1<<6)
#define SEN_BIT_OFS_7 (1<<7)
#define SEN_BIT_OFS_8 (1<<8)
#define SEN_BIT_OFS_9 (1<<9)
#define SEN_BIT_OFS_10 (1<<10)
#define SEN_BIT_OFS_11 (1<<11)
#define SEN_BIT_OFS_12 (1<<12)
#define SEN_BIT_OFS_13 (1<<13)
#define SEN_BIT_OFS_14 (1<<14)
#define SEN_BIT_OFS_15 (1<<15)

typedef struct {
	CTL_SEN_MODE mode;              ///< [in]
	CTL_SEN_IF_TYPE if_type;        ///< [out]
	CTL_SEN_DATA_FMT data_fmt;      ///< [out]
	CTL_SEN_MODE_TYPE mode_type;    ///< [out]
	UINT32 dft_fps;                 ///< [out]sensor driver default (max) support frame rate (fps * 100)
	UINT32 frame_num;               ///< [out]
	CTL_SEN_STPIX stpix;            ///< [out]
	CTL_SEN_PIXDEPTH pixel_depth;   ///< [out]
	CTL_SEN_FMT fmt;                ///< [out]Sensor data type
	USIZE valid_size;                           ///< [out]the number of sample data, for serial interface
	URECT act_size[CTL_SEN_MFRAME_MAX_NUM];     ///< [out]the number of sample data, for sie act, must <= valid_size, [1]~[N] not valid in CTL_SEN_CFGID_GET_MFR_VER=1
	USIZE crp_size;                             ///< [out]the number of sample data, for sie crp (image data), must <= act_size
	CTL_SEN_MODE_SIGNAL signal_info;///< [out]
	UINT32 ratio_h_v;               ///< [out]sensor hv ratio (H:bit[31:16], V:bit[15:0])
	CTL_SEN_MODE_GAIN gain;         ///< [out]sensor gain X 1000
	UINT32 bining_ratio;            ///< [out]binning ratio X 100
} CTL_SENDRV_GET_MODE_BASIC_PARAM;

typedef enum {
	CTL_SEN_DATAIN_BIT_ORDER_LSB,  ///< Data Input Bit order is Least Significant Bit First.
	CTL_SEN_DATAIN_BIT_ORDER_MSB,  ///< Data Input Bit order is Most Significant Bit First.

	ENUM_DUMMY4WORD(CTL_SEN_DATAIN_BIT_ORDER)
} CTL_SEN_DATAIN_BIT_ORDER;

typedef enum {
	CTL_SEN_LVDS_VSYNC_GEN_MATCH_VD,        ///< LVDS/HiSPi Vsync Pulse Generate is compare the VD Sync code matched.
	///< This option is suited for the PANASONIC/Aptina LVDS sensor.
	CTL_SEN_LVDS_VSYNC_GEN_VD2HD,           ///< LVDS/HiSPi Vsync Pulse Generate is detecting SYNC_VD to SYNC_HD transition.
	///< This option is suited for the SONY/SHARP/Samsung LVDS sensor.
	CTL_SEN_LVDS_VSYNC_GEN_HD2VD,           ///< LVDS/HiSPi Vsync Pulse Generate is detecting SYNC_HD to SYNC_VD transition.
	///< This option is reserved for future usage.
	CTL_SEN_LVDS_VSYNC_GEN_MATCH_VD_BY_FSET,///< LVDS/HiSPi Vsync Pulse Generate is depending on the FSET bit changed in SYNC Code.
	///< After FSET bit changed, the first detected VD_SYNC Code would generate Vsync to SIE1.
	///< The first detected VD2_SYNC Code would generate Vsync to SIE2.
	///< This mode is used in the HDR mode for SONY Sensor.

	ENUM_DUMMY4WORD(CTL_SEN_LVDS_VSYNC_GEN)
} CTL_SEN_LVDS_VSYNC_GEN;

typedef struct {
	CTL_SEN_MODE mode;                                      ///< [in]
	CTL_SEN_CLKLANE clk_lane;                               ///< [out] backward compatible, kdrv cover
	CTL_SEN_DATALANE data_lane;                             ///< [out]
	UINT32 output_pixel_order[CTL_SEN_LVDS_MAX_DATALANE];   ///< [out] output data pixel order, need to be filled by CTL_SEN_IGNORE
	UINT32 sel_frm_id[CTL_SEN_MFRAME_MAX_NUM];              ///< [out] HDR only
	UINT16 sel_bit_ofs;                                     ///< [out][option][CTL_SEN_VENDOR_OTHERS must be set]use SEN_BIT_OFS_X, select frame bit(LVDS range:4bits,MIPI range:4bits, SLVS-EC not used)
	UINT32 fset_bit;                                        ///< [out][option][CTL_SEN_VENDOR_OTHERS must be set]
	CTL_SEN_DATAIN_BIT_ORDER data_in_order;                 ///< [out][option][CTL_SEN_VENDOR_OTHERS must be set]
	UINT32 rev;                                             ///< for CTL_SEN_LVDS_VSYNC_GEN ///< [out][option][CTL_SEN_VENDOR_OTHERS must be set]
} CTL_SENDRV_GET_MODE_LVDS_PARAM;


#define CTL_SEN_MIPI_PIXEL_DATA 0xffff
#define CTL_SEN_MIPI_MAX_MANUAL 3
typedef enum {
	CTL_SEN_MIPI_MANUAL_NONE = 0,       ///< mipi auto.

	CTL_SEN_MIPI_MANUAL_8BIT = 8,       ///< mipi manual Pixel depth is  8 bits per pixel.
	CTL_SEN_MIPI_MANUAL_10BIT = 10,     ///< mipi manual Pixel depth is 10 bits per pixel.
	CTL_SEN_MIPI_MANUAL_12BIT = 12,     ///< mipi manual Pixel depth is 12 bits per pixel.
	CTL_SEN_MIPI_MANUAL_14BIT = 14,     ///< mipi manual Pixel depth is 14 bits per pixel.

	CTL_SEN_MIPI_MANUAL_YUV422 = 0x100, ///< mipi manual Pixel depth is YUV422.

	ENUM_DUMMY4WORD(CTL_SEN_MIPI_MANUAL_BIT)
} CTL_SEN_MIPI_MANUAL_BIT;

typedef struct {
	CTL_SEN_MIPI_MANUAL_BIT bit;    ///< manaul depack format (CSI_CONFIG_ID_MANUAL_FORMAT/..)
	UINT32 data_id;                 ///< receiving data id, range: 0x00~0xFF (CSI_CONFIG_ID_MANUAL_DATA_ID/..) or CTL_SEN_MIPI_PIXEL_DATA
} CTL_SEN_MIPI_MANUAL_INFO;

typedef struct {
	CTL_SEN_MODE mode;                                              ///< [in]
	CTL_SEN_CLKLANE clk_lane;                                       ///< [out] backward compatible, kdrv cover
	CTL_SEN_DATALANE data_lane;                                     ///< [out]
	CTL_SEN_MIPI_MANUAL_INFO manual_info[CTL_SEN_MIPI_MAX_MANUAL];  ///< [out]
	BOOL save_pwr;                                                  ///< [out]
	UINT32 sel_frm_id[CTL_SEN_MFRAME_MAX_NUM];                      ///< [out] HDR only
	UINT16 sel_bit_ofs;                                             ///< [out][option][CTL_SEN_VENDOR_OTHERS must be set]use SEN_BIT_OFS_X, select frame bit(LVDS range:4bits,MIPI range:4bits, SLVS-EC not used)
} CTL_SENDRV_GET_MODE_MIPI_PARAM;

typedef struct {
	CTL_SEN_MODE mode;              ///< [in]
	CTL_SEN_PARA_MUX_INFO mux_info; ///< [out], mux information
} CTL_SENDRV_GET_MODE_PARA_PARAM;

/**
    SLVSEC Operation Speed Selection

    The SLVSEC controller can only operate at two speeds: 2.304 or 1.152 Gbps.
    This is used at .set_slvsec_config(DAL_SSENIFSLVSEC_CFGID_SPEED) to
    tell the SLVSEC controller the sensor's output speed.
*/
typedef enum {
	CTL_SEN_SLVSEC_SPEED_2304,  ///< 2.304 Gbps
	CTL_SEN_SLVSEC_SPEED_1152,  ///< 1.152 Gbps

	ENUM_DUMMY4WORD(CTL_SEN_SLVSEC_SPEED)
} CTL_SEN_SLVSEC_SPEED;

typedef struct {
	CTL_SEN_MODE mode;                                              ///< [in]
	CTL_SEN_DATALANE data_lane;                                     ///< [out]
	CTL_SEN_SLVSEC_SPEED speed;                                     ///< [out]
	UINT32 output_pixel_order[CTL_SEN_SLVSEC_MAX_DATALANE];         ///< [out] output data pixel order, need to be filled by CTL_SEN_IGNORE
	UINT32 sel_frm_id[CTL_SEN_MFRAME_SLVSEC_MAX_NUM];               ///< [out] HDR only
} CTL_SENDRV_GET_MODE_SLVSEC_PARAM;


/**
    sensor dvi information
*/
typedef struct {
	CTL_SEN_MODE mode;                  ///< [in]
	CTL_SEN_DVI_FMT fmt;                ///< [out], sensor output fmt
	CTL_SEN_DVI_DATA_MODE data_mode;    ///< [out], sensor output data mode
} CTL_SENDRV_GET_MODE_DVI_PARAM;


/**
    sensor tge information
*/
typedef struct {
	CTL_SEN_MODE mode;             ///< [in]
	CTL_SEN_MODE_SIGNAL signal;    ///< [out], sensor need tge signal info
} CTL_SENDRV_GET_MODE_TGE_PARAM;

typedef struct {
	INT32 probe_rst;               ///< [out] probe result, return 0 for check probe sensor OK
} CTL_SENDRV_GET_PROBE_SEN_PARAM;

#define CTL_SEN_PLUG_PARAM_NUM 5
typedef struct {
	USIZE size;         ///< [out], current plug size
	UINT32 fps;         ///< [out], current plug fps
	BOOL    interlace;          ///< [out] TRUE:CTL_SEN_MODE_CCIR_INTERLACE, FALSE:CTL_SEN_MODE_CCIR
	UINT32 param[CTL_SEN_PLUG_PARAM_NUM];    ///< [out], current plug info
} CTL_SENDRV_GET_PLUG_INFO_PARAM;

typedef struct {
	CTL_SEN_MODE mode;                  ///< [in]
	UINT32 clane_ctl_mode;              ///< [out] 1: force high speed , 0: normal.
} CTL_SENDRV_GET_MODE_MIPI_CLANE_CMETHOD;

typedef INT32(*CTL_SEN_CSI_EN_CB)(CTL_SEN_ID id, BOOL en);
typedef struct {
	CTL_SEN_CSI_EN_CB csi_en_cb;    ///< [in] csi enable callback
} CTL_SENDRV_GET_MODE_MIPI_EN_USER;

typedef enum {
	CTL_SEN_MANUAL_IADJ_SEL_OFF,
	CTL_SEN_MANUAL_IADJ_SEL_IADJ,
	CTL_SEN_MANUAL_IADJ_SEL_DATARATE,

	ENUM_DUMMY4WORD(CTL_SEN_MANUAL_IADJ_SEL)
} CTL_SEN_MANUAL_IADJ_SEL;

typedef struct {
	CTL_SEN_MODE mode;  ///< [in]
	UINT32 row_time;    ///< [out] spend time (us) x10 per row, must be a fixed value.
	UINT32 row_time_step;  ///< [out] for AE, based on row_time
} CTL_SENDRV_GET_MODE_ROWTIME_PARAM;

typedef struct {
	CTL_SEN_MODE mode;              ///< [in]
	UINT32 frame_rate;              ///< [in] fps * 100
	UINT32 num;                     ///< [in] diff_row num
	UINT32 *diff_row;               ///< [out] unit:row, frame1 & frame2 diff / frame2 & frame3 diff / frame3 & frame4 diff / ...
	UINT32 *diff_row_vd;            ///< [out] unit:row, v-sync diff frame1 & frame2 diff / frame2 & frame3 diff / frame3 & frame4 diff / ...
} CTL_SENDRV_GET_MFR_OUTPUT_TIMING_PARAM;

typedef struct {
	CTL_SEN_MODE mode;              ///< [in]
	CTL_SEN_MANUAL_IADJ_SEL sel;    ///< [out]
	union {
		UINT32 iadj;    ///< for CTL_SEN_MANUAL_IADJ_SEL_IADJ
		///< rx comparator spare current control signal
		///< (0: i_source=20uA, 1: i_source=40uA, 2: i_source=60uA, 3: i_source=80uA)
		UINT32 data_rate;   ///< for CTL_SEN_MANUAL_IADJ_SEL_DATARATE
		///< mipi data rate, unit: Mbps, kdrv will calculate the corresponding IADJ value
	} val; ///< [out]
} CTL_SENDRV_GET_MODE_MANUAL_IADJ;

#define CTL_SEN_HSDATAOUT_BASE_CLK CTL_SEN_60M_HZ
typedef struct {
	CTL_SEN_MODE mode;              ///< [in]
	UINT32 delay;                   ///< [out] high speed data output delay, base on CTL_SEN_60M_HZ
} CTL_SENDRV_GET_MODE_MIPI_HSDATAOUT_DLY;

typedef enum {
	CTL_SEN_FLIP_NONE   = 0x00000000,
	CTL_SEN_FLIP_H      = 0x00000001,
	CTL_SEN_FLIP_V      = 0x00000002,
	CTL_SEN_FLIP_H_V    = 0x00000003,

	ENUM_DUMMY4WORD(CTL_SEN_FLIP)
} CTL_SEN_FLIP;

typedef enum {
	CTL_SEN_STATUS_STANDBY = 1,
	CTL_SEN_STATUS_DMA_ABORT = 2,

	ENUM_DUMMY4WORD(CTL_SEN_STATUS)
} CTL_SEN_STATUS;

typedef struct {
	UINT32 chip_id;     ///< [in][out] chip_id(chip idx) for current sensor id
	UINT32 vin_id;      ///< [in][out] vin_id(port) for current sensor id

	UINT32 reserved;
} CTL_SENDRV_AD_ID_MAP_PARAM;

typedef struct {
	UINT32 vin_id;      ///< [in] vin_id(ch) of image parameter
	UINT32 val;         ///< [in][out] set value / return value of image parameter

	UINT32 reserved;
} CTL_SENDRV_AD_IMAGE_PARAM;

typedef struct {
	CTL_SEN_MODE mode;              ///< [in]
	USIZE size;                     ///< [out] dvs sensor dram output size, unit : pixel
} CTL_SENDRV_DVS_INFO_PARAM;

typedef enum {
	CTL_SEN_VD_DLY_NONE,
	CTL_SEN_VD_DLY_AUTO,

	ENUM_DUMMY4WORD(CTL_SEN_VD_DLY)
} CTL_SEN_VD_DLY;

typedef struct {
	CTL_SEN_MODE mode;              ///< [in]
	UINT32 num;                     ///< [in] diff_dly num
	CTL_SEN_VD_DLY *dly;            ///< [out] vd delay frame1 / frame2 / frame3 / ...
} CTL_SENDRV_VD_DLY_PARAM;

typedef enum {
	/* set only*/
	CTL_SENDRV_CFGID_SET_BASE = 0x00000000,
	CTL_SENDRV_CFGID_SET_EXPT,          ///< void (user define), sensor exposure setting
	CTL_SENDRV_CFGID_SET_GAIN,          ///< void (user define), sensor gain setting
	CTL_SENDRV_CFGID_SET_FPS,           ///< UINT32: fps * 100
	CTL_SENDRV_CFGID_SET_STATUS,        ///< CTL_SEN_STATUS

	/* get only */
	CTL_SENDRV_CFGID_GET_BASE = 0x01000000,
	CTL_SENDRV_CFGID_GET_EXPT,          ///< void (user define),
	CTL_SENDRV_CFGID_GET_GAIN,          ///< void (user define),
	CTL_SENDRV_CFGID_GET_ATTR_BASIC,    ///< CTL_SENDRV_GET_ATTR_BASIC_PARAM,
	CTL_SENDRV_CFGID_GET_ATTR_SIGNAL,   ///< CTL_SENDRV_GET_ATTR_SIGNAL_PARAM
	CTL_SENDRV_CFGID_GET_ATTR_CMDIF,    ///< CTL_SENDRV_GET_ATTR_CMDIF_PARAM,
	CTL_SENDRV_CFGID_GET_ATTR_IF,       ///< CTL_SENDRV_GET_ATTR_IF_PARAM,
	CTL_SENDRV_CFGID_GET_TEMP,          ///< CTL_SENDRV_GET_TEMP_PARAM,
	CTL_SENDRV_CFGID_GET_FPS,           ///< CTL_SENDRV_GET_FPS_PARAM,
	CTL_SENDRV_CFGID_GET_SPEED,         ///< CTL_SENDRV_GET_SPEED_PARAM,
	CTL_SENDRV_CFGID_GET_MODESEL,       ///< CTL_SENDRV_GET_MODESEL_PARAM,
	CTL_SENDRV_CFGID_GET_MODE_BASIC,    ///< CTL_SENDRV_GET_MODE_BASIC_PARAM,
	CTL_SENDRV_CFGID_GET_MODE_LVDS,     ///< CTL_SENDRV_GET_MODE_LVDS_PARAM,
	CTL_SENDRV_CFGID_GET_MODE_MIPI,     ///< CTL_SENDRV_GET_MODE_MIPI_PARAM,
	CTL_SENDRV_CFGID_GET_MODE_PARA,     ///< CTL_SENDRV_GET_MODE_PARA_PARAM,
	CTL_SENDRV_CFGID_GET_MODE_SLVSEC,   ///< CTL_SENDRV_GET_MODE_SLVSEC_PARAM, 510/520/560 N.S.
	CTL_SENDRV_CFGID_GET_MODE_DVI,      ///< CTL_SENDRV_GET_MODE_DVI_PARAM,
	CTL_SENDRV_CFGID_GET_MODE_TGE,      ///< CTL_SENDRV_GET_MODE_TGE_PARAM,
	CTL_SENDRV_CFGID_GET_PLUG_INFO,     ///< CTL_SENDRV_GET_PLUG_INFO_PARAM
	CTL_SENDRV_CFGID_GET_PROBE_SEN,     ///< CTL_SENDRV_GET_PROBE_SEN_PARAM, need to get after sensor power on
	CTL_SENDRV_CFGID_GET_SENDRV_VER,    ///< UINT32, sensor driver version
	CTL_SENDRV_CFGID_GET_MODE_MIPI_CLANE_CMETHOD,   ///< CTL_SENDRV_GET_MODE_MIPI_CLANE_CMETHOD, clock lane control method
	CTL_SENDRV_CFGID_GET_MODE_MIPI_EN_USER,         ///< CTL_SENDRV_GET_MODE_MIPI_EN_USER, if return OK, csi need enable by sendrv
	CTL_SENDRV_CFGID_GET_MODE_MIPI_MANUAL_IADJ,     ///< CTL_SENDRV_GET_MODE_MANUAL_IADJ, rx comparator spare current control signal
	CTL_SENDRV_CFGID_GET_MODE_ROWTIME,              ///< CTL_SENDRV_GET_MODE_ROWTIME_PARAM
	CTL_SENDRV_CFGID_GET_MFR_OUTPUT_TIMING,         ///< CTL_SENDRV_GET_MFR_OUTPUT_TIMING_PARAM, sensor output frame timing
	CTL_SENDRV_CFGID_GET_MODE_MIPI_HSDATAOUT_DLY,   ///< CTL_SENDRV_GET_MODE_MIPI_HSDATAOUT_DLY, sensor high speed data output delay
	CTL_SENDRV_CFGID_GET_DVS_INFO,                  ///< CTL_SENDRV_DVS_INFO_PARAM dvs sensor information
	CTL_SENDRV_CFGID_GET_VD_DLY,                    ///< CTL_SENDRV_VD_DLY_PARAM, multi-frame sensor mode only
	CTL_SENDRV_CFGID_GET_MFR_VER,                   ///< UINT32, multi-frame version, 0: CTL_SEN_MODE not support FRMIDX, 1 : CTL_SEN_MODE support FRMIDX

	/* set & get */
	CTL_SENDRV_CFGID_BASE = 0x02000000,
	CTL_SENDRV_CFGID_FLIP_TYPE,         ///< CTL_SEN_FLIP
	CTL_SENDRV_CFGID_AD_ID_MAP,         ///< CTL_SENDRV_AD_ID_MAP_PARAM
	CTL_SENDRV_CFGID_AD_TYPE,           ///< CTL_SENDRV_AD_IMAGE_PARAM
	CTL_SENDRV_CFGID_AD_INIT,           ///< BOOL

	/* user define */
	CTL_SENDRV_CFGID_USER_BASE = 0x03000000,
	CTL_SENDRV_CFGID_USER_DEFINE1,      ///< user define 1 by sensor
	CTL_SENDRV_CFGID_USER_DEFINE2,      ///< user define 2 by sensor
	CTL_SENDRV_CFGID_USER_DEFINE3,      ///< user define 3 by sensor
	CTL_SENDRV_CFGID_USER_DEFINE4,      ///< user define 4 by sensor
	CTL_SENDRV_CFGID_USER_DEFINE5,      ///< user define 5 by sensor

	ENUM_DUMMY4WORD(CTL_SENDRV_CFGID)
} CTL_SENDRV_CFGID;

typedef struct {
	ER(*open)(CTL_SEN_ID id);                                               ///< initial sensor flow
	ER(*close)(CTL_SEN_ID id);                                              ///< un-initial sensor flow
	ER(*sleep)(CTL_SEN_ID id);                                              ///< enter sensor sleep mode
	ER(*wakeup)(CTL_SEN_ID id);                                             ///< exit sensor sleep mode
	ER(*write)(CTL_SEN_ID id, CTL_SEN_CMD *cmd);                            ///< write command
	ER(*read)(CTL_SEN_ID id, CTL_SEN_CMD *cmd);                             ///< read command
	ER(*chgmode)(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj);        ///< change mode flow
	ER(*chgfps)(CTL_SEN_ID id, UINT32 frame_rate);                          ///< change fps flow, frame_rate: fps * 100
	ER(*set_cfg)(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);   ///< set sensor information (if sensor driver not support feature, pls return E_NOSPT)
	ER(*get_cfg)(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);   ///< get sensor information (if sensor driver not support feature, pls return E_NOSPT)
} CTL_SEN_DRV_TAB;

typedef enum {
	CTL_SEN_PWR_CTRL_TURN_ON = 0,
	CTL_SEN_PWR_CTRL_TURN_OFF,
	CTL_SEN_PWR_CTRL_SAVE_BEGIN,
	CTL_SEN_PWR_CTRL_SAVE_END,
} CTL_SEN_PWR_CTRL_FLAG;

typedef enum {
	CTL_SEN_CLK_SEL_SIEMCLK,
	CTL_SEN_CLK_SEL_SIEMCLK2,
	CTL_SEN_CLK_SEL_SIEMCLK3,

	CTL_SEN_CLK_SEL_SPCLK,
	CTL_SEN_CLK_SEL_SPCLK2,

	CTL_SEN_CLK_SEL_MAX,

	CTL_SEN_CLK_SEL_SIEMCLK_IGNORE = CTL_SEN_IGNORE,
	ENUM_DUMMY4WORD(CTL_SEN_CLK_SEL)
} CTL_SEN_CLK_SEL;

typedef BOOL (*CTL_SEN_PLUG_IN)(CTL_SEN_ID id);
typedef void (*CTL_SEN_CLK_CB)(CTL_SEN_CLK_SEL clk_sel, BOOL en);
typedef void (*CTL_SEN_PWR_CTRL)(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb);

typedef struct {
	CTL_SEN_PWR_CTRL pwr_ctrl;
	CTL_SEN_PLUG_IN det_plug_in;
	CTL_SEN_DRV_TAB *drv_tab;
} CTL_SEN_REG_OBJ, *PCTL_SEN_REG_OBJ;

/**
    sensor chgmode information
    * sensor frame rate
      - input: user input/SEN_FPS_DFT
      - user input: frame rate * 100
      - user input: must be less than sensor driver default value
*/
typedef enum {
	CTL_SEN_MODESEL_AUTO = 0,   ///< auto select sensor mode by sensor driver
	CTL_SEN_MODESEL_MANUAL,     ///< user indicates sensor mode
} CTL_SEN_MODESEL;

typedef struct {
	UINT32 frame_rate;                      ///< fps * 100
	USIZE size;                             ///< sensor output size (pixel)
	UINT32 frame_num;                       ///< sensor output frame number (linear mode: 1, hdr mode: 2~CTL_SEN_MFRAME_MAX_NUM)
	CTL_SEN_DATA_FMT data_fmt;              ///< sensor output data format
	CTL_SEN_PIXDEPTH pixdepth;              ///< sensor output pixel depth, set CTL_SEN_IGNORE for auto (sensor driver need to cover CTL_SEN_IGNORE)
	CTL_SEN_MODESEL_CCIR ccir;              ///< only CTL_SEN_DATA_FMT_YUV, CTL_SEN_DATA_FMT_Y_ONLY need to set
	BOOL mux_singnal_en;                    ///< only support CTL_SEN_IF_TYPE_PARALLEL
	CTL_SEN_PARA_MUX_INFO mux_signal_info;  ///< only (mux_singnal_en == 1) need to set
	CTL_SEN_MODE_TYPE mode_type_sel;        ///< if set 0 or CTL_SEN_IGNORE (sendrv will ignore this parameters)
	CTL_SEN_DATALANE data_lane;             ///< if set 0 or CTL_SEN_IGNORE (sendrv will ignore this parameters)
} CTL_SEN_MODESEL_AUTO_INFO;

typedef struct {
	UINT32 frame_rate;  // fps * 100
	CTL_SEN_MODE sen_mode;
} CTL_SEN_MODESEL_MANUAL_INFO;

/**
    sensor chgmode obj
*/
typedef struct {
	CTL_SEN_MODESEL mode_sel;                   ///< [in] sensor mode select mode
	CTL_SEN_MODESEL_AUTO_INFO auto_info;        ///< [in] mode_sel select CTL_SEN_MODESEL_AUTO need to set
	CTL_SEN_MODESEL_MANUAL_INFO manual_info;    ///< [in] mode_sel select CTL_SEN_MODESEL_MANUAL need to set
	CTL_SEN_OUTPUT_DEST output_dest;            ///< [in] sensor output sie
} CTL_SEN_CHGMODE_OBJ;

typedef struct {
	CHAR name[CTL_SEN_NAME_LEN];        ///< [out]
	CTL_SEN_VENDOR vendor;              ///< [out]
	UINT32 max_senmode;                 ///< [out]
	CTL_SEN_CMDIF_TYPE cmdif_type;      ///< [out]
	CTL_SEN_IF_TYPE if_type;            ///< [out]
	CTL_SEN_DRVDEV drvdev;              ///< [out] KDRV controller id(CSI/LVDS/TGE) or ch(TGE) information
	CTL_SEN_SUPPORT_PROPERTY property;  ///< [out] sensor support property
	UINT32 sync_timing;                 ///< [out] sync timing for Exposure time & gain(VD)
	CTL_SEN_SIGNAL_TYPE signal_type;    ///< [out]
	CTL_SEN_SIGNAL_INFO signal_info;    ///< [out]
} CTL_SEN_GET_ATTR_PARAM;

typedef enum {
	CTL_SEN_VX1_IF_SEL_0,   ///< vx1 interface 0, support parallel sensor linear mode and mipi sensor linear mode
	CTL_SEN_VX1_IF_SEL_1,   ///< vx1 interface 1, support mipi sensor linear mode and hdr mode
	ENUM_DUMMY4WORD(CTL_SEN_VX1_IF_SEL)
} CTL_SEN_VX1_IF_SEL;

typedef enum {
	CTL_SEN_VX1_CTL_0,   ///< vx1 controller 0, mapping to KDRV_SSENIF_ENGINE_VX1_0, NT96680 only support SIE 1~5
	CTL_SEN_VX1_CTL_1,   ///< vx1 controller 1, mapping to KDRV_SSENIF_ENGINE_VX1_1, NT96680 only support SIE 4,6
	ENUM_DUMMY4WORD(CTL_SEN_VX1_CTL_SEL)
} CTL_SEN_VX1_CTL_SEL;

/*
    mapping to KDRV_SSENIFVX1_TXTYPE_THCV235/KDRV_SSENIFVX1_TXTYPE_THCV231/KDRV_SSENIFVX1_TXTYPE_THCV241
*/
typedef enum {
	CTL_SEN_VX1_TXTYPE_THCV235, ///< Vx1 Transmitter IC is THCV235.
	///< This only support Vx1 One-lane none-HDR parallel sensor.
	CTL_SEN_VX1_TXTYPE_THCV231, ///< Vx1 Transmitter IC is THCV231.
	///< This only support Vx1 One-lane none-HDR parallel sensor.

	CTL_SEN_VX1_TXTYPE_THCV241, ///< Vx1 Transmitter IC is THCV241. This is MIPI CSI input interface Vx1 Tx.
	///< Which can support 1 or 2 lanes Vx1 interface. HDR MIPI Sensor such as SONY_LI or OV/ON-SEMI is also supported.
	///< The maximum MIPI Sensor support spec is 4 lanes mipi with 1Gbps per lane by using Vx1 2 lanes.
	///< The maximum MIPI Sensor support spec is 4 lanes mipi with 500Mbps per lane by using Vx1 1 lanes.
	ENUM_DUMMY4WORD(CTL_SEN_VX1_TXTYPE)
} CTL_SEN_VX1_TXTYPE;

typedef struct {
	CTL_SEN_VX1_IF_SEL  if_sel;     ///< interface select
	CTL_SEN_VX1_TXTYPE tx_type;     ///< tx module select
} CTL_SEN_VX1_INFO;

typedef struct {
	CTL_SEN_CMDIF_TYPE type;///< [out]
	CTL_SEN_VX1_INFO vx1;   ///< [out], 510/520/560 N.S.
} CTL_SEN_GET_CMDIF_PARAM;

typedef struct {
	CTL_SEN_PARA_MUX_INFO mux_info;
} CTL_SEN_PARALLEL_INFO;

typedef struct {
	CTL_SEN_MODE mode;      ///< [in]
	CTL_SEN_IF_TYPE type;   ///< [out]
	union {
		CTL_SEN_PARALLEL_INFO parallel;
	} info; // [out]
} CTL_SEN_GET_IF_PARAM;

typedef struct {
	CTL_SEN_MODE mode;                  ///< [in]
	CTL_SEN_DVI_FMT fmt;                ///< [out], sensor output fmt
	CTL_SEN_DVI_DATA_MODE data_mode;    ///< [out], sensor output data mode
	BOOL msblsb_switch;                 ///< [out], only for sensor output HD (16bits)
} CTL_SEN_GET_DVI_PARAM;

typedef struct {
	UINT32 dft_fps;     ///< [out] sensor driver default (max) support frame rate (fps * 100)
	UINT32 chg_fps;     ///< [out] chgmode frame rate (fps * 100)
	UINT32 cur_fps;     ///< [out] current frame rate (fps * 100)
} CTL_SEN_GET_FPS_PARAM;


/*
    560 hw surpport list :

                480     320     PLL5    PLL6    PLL12
        MCLK1   O       O       O       X       O
        MCLK2   O       O       O       X       O
        MCLK3   O       O       O       X       O
        SPCLK1  O       X       O       O       X
        SPCLK1  O       X       O       O       X

*/
typedef enum {
	CTL_SEN_CLKSRC_480,         ///< clock source as 480MHz
	CTL_SEN_CLKSRC_320,         ///< clock source as 320MHz
	CTL_SEN_CLKSRC_PLL5,        ///< clock source as PLL5
	CTL_SEN_CLKSRC_PLL6,        ///< clock source as PLL6
	CTL_SEN_CLKSRC_PLL12,       ///< clock source as PLL12
	CTL_SEN_CLKSRC_MAX,
	ENUM_DUMMY4WORD(CTL_SEN_CLKSRC_SEL)
} CTL_SEN_CLKSRC_SEL;

typedef struct {
	CTL_SEN_MODE mode;              ///< [in]
	UINT32 mclk_freq;               ///< [out] mclk freq
	CTL_SEN_CLK_SEL mclk_sel;       ///< [out] mclk select (MCLK1/MCLK2/...)
	CTL_SEN_CLKSRC_SEL mclk_src_sel;///< [out] mclk source select (FIXED480/PLL5/PLL12/...)
	UINT32 data_rate;               ///< [out] sie or tge clk freq must larger than data_rate
	UINT32 pclk;                    ///< [out] pixel clock
} CTL_SEN_GET_CLK_INFO_PARAM;

typedef struct {
	UINT32 frame_rate;                      ///< [in] fps * 100
	USIZE size;                             ///< [in]
	UINT32 frame_num;                       ///< [in]
	CTL_SEN_DATA_FMT data_fmt;              ///< [in] [in] sensor output data format
	CTL_SEN_PIXDEPTH pixdepth;              ///< [in] sensor output pixel depth, set CTL_SEN_IGNORE for auto (sensor driver need to cover CTL_SEN_IGNORE)
	CTL_SEN_MODESEL_CCIR ccir;              ///< [in] only CTL_SEN_DATA_FMT_YUV, CTL_SEN_DATA_FMT_Y_ONLY need to set
	BOOL mux_singnal_en;                    ///< [in] only support CTL_SEN_IF_TYPE_PARALLEL
	CTL_SEN_PARA_MUX_INFO mux_signal_info;  ///< [in] only (mux_singnal_en == 1) need to set
	CTL_SEN_MODE_TYPE mode_type_sel;        ///< [in] if set 0 or CTL_SEN_IGNORE (sendrv will ignore this parameters)
	CTL_SEN_DATALANE data_lane;             ///< [in] if set 0 or CTL_SEN_IGNORE (sendrv will ignore this parameters)
	CTL_SEN_MODE mode;                      ///< [out]
} CTL_SEN_GET_MODESEL_PARAM;

typedef struct {
	CTL_SEN_MODE mode;              ///< [in]
	CTL_SEN_DATA_FMT data_fmt;      ///< [out]
	CTL_SEN_MODE_TYPE mode_type;    ///< [out]
	UINT32 dft_fps;                 ///< [out]sensor driver default (max) support frame rate (fps * 100)
	UINT32 frame_num;               ///< [out]
	CTL_SEN_STPIX stpix;            ///< [out]
	CTL_SEN_PIXDEPTH pixel_depth;   ///< [out]
	CTL_SEN_FMT fmt;                ///< [out] Sensor data type
	USIZE valid_size;                           ///< [out]the number of sample data, for serial interface
	URECT act_size[CTL_SEN_MFRAME_MAX_NUM];     ///< [out]the number of sample data, for sie act, must <= valid_size, [1]~[N] not valid in CTL_SEN_CFGID_GET_MFR_VER=1
	USIZE crp_size;                             ///< [out]the number of sample data, for sie crp (image data), must <= act_size
	CTL_SEN_MODE_SIGNAL signal_info;///< [out]
	UINT32 ratio_h_v;               ///< [out]sensor hv ratio (H:bit[31:16], V:bit[15:0])
	CTL_SEN_MODE_GAIN gain;         ///< [out] sensor gain X 1000
	UINT32 bining_ratio;            ///< [out] binning ratio X 100
	UINT32 row_time;                ///< [out] spend time (us) X10 per row, must be a fixed value.
	UINT32 row_time_step;           ///< [out] for AE, based on row_time
} CTL_SEN_GET_MODE_BASIC_PARAM;

typedef struct {
	INT32 probe_rst;               ///< [out] probe result, return 0 for check probe sensor OK
} CTL_SEN_GET_PROBE_SEN_PARAM;

typedef struct {
	CTL_SEN_MODE mode;              ///< [in]
	UINT32 frame_rate;              ///< [in] fps * 100
	UINT32 num;                     ///< [in] diff_row & diff_row_vd num
	UINT32 *diff_row;               ///< [out] unit:row, frame1 & frame2 diff / frame2 & frame3 diff / frame3 & frame4 diff / ...
	UINT32 *diff_row_vd;            ///< [out] unit:row, v-sync diff frame1 & frame2 diff / frame2 & frame3 diff / frame3 & frame4 diff / ...
} CTL_SEN_GET_MFR_OUTPUT_TIMING_PARAM;

typedef struct {
	USIZE size;         ///< [out], current plug size
	UINT32 fps;         ///< [out], current plug fps
	BOOL    interlace;          ///< [out] TRUE:CTL_SEN_MODE_CCIR_INTERLACE, FALSE:CTL_SEN_MODE_CCIR
	UINT32 param[5];    ///< [out], current plug info
} CTL_SEN_GET_PLUG_INFO_PARAM;

typedef struct {
	UINT32 pin;         ///< [SET-in][GET-in], NT96680 : pin=0 valid for THCV241, pin=1 valid for THCV241, pin=2 valid for THCV241
	///<    , pin=3 valid for THCV241/THCV231/THCV235, pin=4 valid for THCV231/THCV235
	BOOL value;         ///< [SET-in][GET-out], 0 for low, 1 for high
} CTL_SEN_CTL_VX1_GPIO_PARAM;

typedef void (*CTL_SEN_MIPI_CB)(UINT32 event);  // event pls reference CSI_CB_ID

#define CTL_SEN_AD_VCAP_ID_MAX 8

typedef struct {
	UINT32 chip_id;     ///< [in][out] chip_id(chip idx) for current sensor id
	UINT32 vin_id;      ///< [in][out] vin_id(ch) for current sensor id

	UINT32 reserved;
} CTL_SEN_AD_ID_MAP_PARAM;

typedef struct {
	UINT32 vin_id;      ///< [in] vin_id(ch) of image parameter
	UINT32 val;         ///< [in][out] set value / return value of image parameter

	UINT32 reserved;
} CTL_SEN_AD_IMAGE_PARAM;

typedef struct {
	CTL_SEN_MODE mode;              ///< [in]
	USIZE size;                     ///< [out] dvs sensor dram output size, unit : pixel
} CTL_SEN_DVS_INFO_PARAM;

typedef struct {
	CTL_SEN_MODE mode;              ///< [in]
	UINT32 num;                     ///< [in] dly num
	CTL_SEN_VD_DLY *dly;            ///< [out] vd delay frame1 / frame2 / frame3 / ...
} CTL_SEN_VD_DLY_PARAM;

typedef enum {
	/* set only*/
	CTL_SEN_CFGID_SET_BASE = 0x00000000,
	CTL_SEN_CFGID_SET_EXPT,             ///< [SET]void (user define), sensor exposure setting
	CTL_SEN_CFGID_SET_GAIN,             ///< [SET]void (user define), sensor gain setting
	CTL_SEN_CFGID_SET_FPS,              ///< [SET]UINT32 (fps*100)
	CTL_SEN_CFGID_SET_SIGNAL_RST,       ///< [SET]CTL_SEN_OUTPUT_DEST, only support slave sensor, user manual reset vd/hd signal
	CTL_SEN_CFGID_SET_STATUS,           ///< [SET]CTL_SEN_STATUS
	CTL_SEN_CFGID_SET_MAX,

	/* get only */
	CTL_SEN_CFGID_GET_BASE = 0x01000000,
	CTL_SEN_CFGID_GET_EXPT,             ///< [GET]void (user define)
	CTL_SEN_CFGID_GET_GAIN,             ///< [GET]void (user define)
	CTL_SEN_CFGID_GET_ATTR,             ///< [GET]CTL_SEN_GET_ATTR_PARAM
	CTL_SEN_CFGID_GET_CMDIF,            ///< [GET]CTL_SEN_GET_CMDIF_PARAM
	CTL_SEN_CFGID_GET_IF,               ///< [GET]CTL_SEN_GET_IF_PARAM
	CTL_SEN_CFGID_GET_DVI,              ///< [GET]CTL_SEN_GET_DVI_PARAM
	CTL_SEN_CFGID_GET_TEMP,             ///< [GET]UINT32
	CTL_SEN_CFGID_GET_FPS,              ///< [GET]CTL_SEN_GET_FPS_PARAM
	CTL_SEN_CFGID_GET_CLK_INFO,         ///< [GET]CTL_SEN_GET_CLK_INFO_PARAM, param. set for clk driver
	CTL_SEN_CFGID_GET_MODESEL,          ///< [GET]CTL_SEN_GET_MODESEL_PARAM
	CTL_SEN_CFGID_GET_MODE_BASIC,       ///< [GET]CTL_SEN_GET_MODE_BASIC_PARAM
	CTL_SEN_CFGID_GET_PLUG,             ///< [GET]BOOL
	CTL_SEN_CFGID_GET_PLUG_INFO,        ///< [GET]CTL_SEN_GET_PLUG_INFO_PARAM
	CTL_SEN_CFGID_GET_PROBE_SEN,        ///< [GET]CTL_SEN_GET_PROBE_SEN_PARAM, need to get after sensor power on
	CTL_SEN_CFGID_GET_SENDRV_VER,       ///< [GET]UINT32, sensor driver version
	CTL_SEN_CFGID_GET_MFR_OUTPUT_TIMING,///< [GET]CTL_SEN_GET_MFR_OUTPUT_TIMING_PARAM, sensor output frame timing
	CTL_SEN_CFGID_GET_VD_CNT,           ///< [GET]UINT32, ssenif vd count
	CTL_SEN_CFGID_GET_DVS_INFO,         ///< [GET]CTL_SEN_DVS_INFO_PARAM, dvs sensor information
	CTL_SEN_CFGID_GET_VD_DLY,           ///< [GET]CTL_SEN_VD_DLY_PARAM, multi-frame sensor mode only
	CTL_SEN_CFGID_GET_MFR_VER,          ///< [GET]UINT32, multi-frame version, 0: CTL_SEN_MODE not support FRMIDX, 1 : CTL_SEN_MODE support FRMIDX
	CTL_SEN_CFGID_GET_MAX,

	/* set & get */
	CTL_SEN_CFGID_BASE = 0x02000000,
	CTL_SEN_CFGID_FLIP_TYPE,            ///< [SET][GET]CTL_SEN_FLIP
	CTL_SEN_CFGID_AD_ID_MAP,            ///< [SET][GET]CTL_SEN_AD_ID_MAP_PARAM
	CTL_SEN_CFGID_AD_TYPE,              ///< [SET][GET]CTL_SEN_AD_IMAGE_PARAM
	CTL_SEN_CFGID_AD_INIT,              ///< [SET] void
	CTL_SEN_CFGID_MAX,

	/* user define */
	CTL_SEN_CFGID_USER_BASE = 0x03000000,
	CTL_SEN_CFGID_USER_DEFINE1,         ///< [SET][GET]user define 1 by sensor
	CTL_SEN_CFGID_USER_DEFINE2,         ///< [SET][GET]user define 2 by sensor
	CTL_SEN_CFGID_USER_DEFINE3,         ///< [SET][GET]user define 3 by sensor
	CTL_SEN_CFGID_USER_DEFINE4,         ///< [SET][GET]user define 4 by sensor
	CTL_SEN_CFGID_USER_DEFINE5,         ///< [SET][GET]user define 5 by sensor
	CTL_SEN_CFGID_USER_MAX,

	/* init obj (set & get) */
	CTL_SEN_CFGID_INIT_BASE = 0x04000000,
	CTL_SEN_CFGID_INIT_IF_MAP,          ///< Not support (Backward compatible)
	CTL_SEN_CFGID_INIT_IF_TIMEOUT_MS,   ///< [SET][GET]UINT32, ms, default: 1000ms, only CTL_SEN_IF_TYPE_LVDS, CTL_SEN_IF_TYPE_MIPI, CTL_SEN_IF_TYPE_SLVSEC support
	CTL_SEN_CFGID_INIT_SIGNAL_SYNC,     ///< [SET][GET]UINT32(1 << CTL_SEN_ID), default: 0, auto sync vd/hd output for sensor , only valid in slave sensor (tge_en = 1)
	///<  all of output destination sensor must be chgmode at the same time
	///<  (stop all sie -> chgmode all sensor -> start all sie)
	CTL_SEN_CFGID_INIT_MIPI_CB,         ///< [SET] CTL_SEN_MIPI_CB, only CTL_SEN_IF_TYPE_MIPI support
	CTL_SEN_CFGID_INIT_MAX,             ///<

	/* for sensor driver control vx1 , 510/520/560 N.S. */
	CTL_SEN_CFGID_VX1_BASE = 0x05000000,///< 510/520/560 N.S.
	CTL_SEN_CFGID_VX1_SLAVE_ADDR,       ///< [SET][GET]UINT32, Sensor I2C "7-bits" Slave address.(Not 8 bits)
	CTL_SEN_CFGID_VX1_ADDR_BYTE_CNT,    ///< [SET][GET]UINT32, Set Sensor I2C command address field byte count. valid setting value 1~4.
	CTL_SEN_CFGID_VX1_DATA_BYTE_CNT,    ///< [SET][GET]UINT32, Set Sensor I2C command    data field byte count. valid setting value 1~4.
	CTL_SEN_CFGID_VX1_I2C_WRITE,        ///< [SET]CTL_SEN_CMD, write i2c cmd
	CTL_SEN_CFGID_VX1_I2C_READ,         ///< [GET]CTL_SEN_CMD, read i2c cmd
	CTL_SEN_CFGID_VX1_GPIO,             ///< [SET][GET]CTL_SEN_CTL_VX1_GPIO_PARAM
	CTL_SEN_CFGID_VX1_PLUG,             ///< [GET]BOOL, Check Vx1 Cable is plug or not.
	CTL_SEN_CFGID_VX1_I2C_SPEED,        ///< [SET][GET]Specify i2c speed on the vx1 Tx board to sensor. Please use KDRV_SSENIFVX1_I2CSPEED as input parameter.
	CTL_SEN_CFGID_VX1_I2C_NACK_CHK,     ///< [SET][GET][OPTION]If "ENABLE":  The sensor I2C NACK status would be returned at .sensor_i2c_write(), but would down speed i2c transmission.
	CTL_SEN_CFGID_VX1_MAX,              ///<

	/* for sensor driver control tge */
	CTL_SEN_CFGID_TGE_BASE = 0x06000000,///<
	CTL_SEN_CFGID_TGE_VD_PERIOD,        ///< [SET][GET]UINT32
	CTL_SEN_CFGID_TGE_HD_PERIOD,        ///< [SET][GET]UINT32
	CTL_SEN_CFGID_TGE_VD_SYNC,          ///< [SET][GET]UINT32
	CTL_SEN_CFGID_TGE_HD_SYNC,          ///< [SET][GET]UINT32
	CTL_SEN_CFGID_TGE_MAX,              ///<

	ENUM_DUMMY4WORD(CTL_SEN_CFGID)
} CTL_SEN_CFGID;

typedef struct {
	CTL_SEN_IF_TYPE type;
	CTL_SEN_IF_TGE tge;          ///< tge information
	UINT32 mclksrc_sync;    // backward compatible, pls used PIN_SENSOR2_CFG_MCLK_SRC
} CTL_SEN_INIT_IF_CFG;

typedef struct {
	BOOL en;
	CTL_SEN_VX1_IF_SEL  if_sel;     ///< interface select
	CTL_SEN_VX1_TXTYPE tx_type;     ///< tx module select
} CTL_SEN_CMDIF_VX1;

typedef struct {
	CTL_SEN_CMDIF_VX1 vx1;          ///< vx1 information, 510/520/560 N.S.
} CTL_SEN_INIT_CMDIF_CFG;

#define CTL_SEN_PINMUX_MAX_NUM 10
typedef struct _CTL_SEN_PINMUX {
	UINT32  func;           ///< need mapping to top.h define (PIN_FUNC)
	UINT32  cfg;            ///< need mapping to top.h define (PIN_SENSOR_CFG/PIN_SENSOR2_CFG/.../PIN_SENSOR8_CFG/PIN_MIPI_LVDS_CFG/PIN_I2C_CFG/PIN_SIF_CFG/...)
	///< 1. GPIO->FUNCTION :
	///<    (1) PIN_I2C_CFG : after sendrv power on
	///<    (2) PIN_SENSORx_CFG(except cfg_mclk)/PIN_MIPI_LVDS_CFG/PIN_SIF_CFG : sensor_open
	///< 2. FUNCTION->GPIO :
	///<    (1) PIN_I2C_CFG : before sendrv power off
	UINT32  cfg_mclk;       ///< need mapping to top.h define (PIN_SENSOR_CFG/PIN_SENSOR2_CFG/.../PIN_SENSOR8_CFG)
	///< 1. GPIO->FUNCTION :
	///<    (1) before MCLK_ENABLE
	///< 2. FUNCTION->GPIO :
	///<    (1) after MCLK_DISABLE
	struct _CTL_SEN_PINMUX *pnext;
} CTL_SEN_PINMUX;

typedef struct {
	CTL_SEN_PINMUX pinmux;          ///< set gpio for function
	CTL_SEN_CLANE_SEL clk_lane_sel; ///< only LVDS and MIPI need to set, 510/520 N.S.
	UINT32 sen_2_serial_pin_map[CTL_SEN_SER_MAX_DATALANE];    ///< Serial (lvds/csi/slvsec) data pin 2 hw map
	BOOL ccir_msblsb_switch;        ///< only for ccir sensor output HD mode (16 bits)
	BOOL ccir_vd_hd_pin;            ///< CCIR601 need HW VD/HD output pin
	BOOL vx1_tx241_cko_pin;         ///< ENABLE / DISABLE the THCV241's CKO pin output. [limit]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only. 510/520/560 N.S.
	BOOL vx1_tx241_cfg_2lane_mode;  ///< set FALSE for 1 lane mode, set TRUE for 2 lanes mode. Only KDRV_SSENIFVX1_TXTYPE_THCV241 has 2 lanes mode. 510/520/560 N.S.
	///<        CTL_SEN_VX1_CTL_0 Legal setting is FALSE and TRUE.
	///<        CTL_SEN_VX1_CTL_1 Legal setting is FALSE.
} CTL_SEN_INIT_PIN_CFG;

typedef struct {
	CTL_SEN_AD_ID_MAP_PARAM ad_id_map; // for ad sendrv
} CTL_SEN_INIT_SENDRV_CFG;

typedef struct {
	CTL_SEN_INIT_PIN_CFG pin_cfg;
	CTL_SEN_INIT_IF_CFG if_cfg;
	CTL_SEN_INIT_CMDIF_CFG cmd_if_cfg;
	CTL_SEN_INIT_SENDRV_CFG sendrv_cfg;
	CTL_SEN_DRVDEV drvdev;
} CTL_SEN_INIT_CFG_OBJ;

/**
    CTL of Sensor Interface Object Definotions

    This is used by the CTL of Sensor Interface Get Object API ctl_sen_get_object() to get the specific module control object.

    must be ctl_sen_reg_sendrv() first
*/
typedef struct {

	/* Object Common items */
	CTL_SEN_ID   sen_id;                            ///< SENSOR Object ID.

	INT32(*init_cfg)(CHAR *name, CTL_SEN_INIT_CFG_OBJ *init_cfg_obj);  ///< name length: CTL_SEN_NAME_LEN

	INT32(*open)(void);                                ///< SENSOR Object open.
	INT32(*close)(void);                               ///< SENSOR Object close.
	BOOL (*is_opened)(void);                        ///< Check SENSOR Object is opened or not.

	INT32(*pwr_ctrl)(CTL_SEN_PWR_CTRL_FLAG flag);    ///< SENSOR Object power ctrl.

	INT32(*sleep)(void);                               ///< SENSOR Object sleep.
	INT32(*wakeup)(void);                              ///< SENSOR Object wakeup.

	INT32(*write_reg)(CTL_SEN_CMD *cmd);               ///< SENSOR Object write reg.
	INT32(*read_reg)(CTL_SEN_CMD *cmd);                ///< SENSOR Object read reg.

	INT32(*chgmode)(CTL_SEN_CHGMODE_OBJ chgmode_obj);  ///< SENSOR Object chgmode.

	INT32(*set_cfg)(CTL_SEN_CFGID cfg_id, void *value);///< Set SENSOR Object configuration.
	INT32(*get_cfg)(CTL_SEN_CFGID cfg_id, void *value);///< Get SENSOR Object configuration.

	/* for debug */
	CTL_SEN_INTE(*wait_interrupt)(CTL_SEN_INTE waited_flag);                    ///< SENSOR Object  wait_interrupt for debug.

	INT32(*dbg_info)(CTL_SEN_DBG_SEL dbg_sel, UINT32 param);     ///< SENSOR Object  dump debug information.

} CTL_SEN_OBJ, *PCTL_SEN_OBJ;

PCTL_SEN_OBJ ctl_sen_get_object(CTL_SEN_ID sen_id);
int kflow_ctl_sen_init(void);
int kflow_ctl_sen_uninit(void);

/**
    buffer ctrl
*/
UINT32 ctl_sen_buf_query(UINT32 num);
INT32 ctl_sen_init(UINT32 buf_addr, UINT32 buf_size);
INT32 ctl_sen_uninit(void);

/**
    sendrv register and unregister
*/
INT32 ctl_sen_reg_sendrv(CHAR *name, CTL_SEN_REG_OBJ *reg_obj);    ///< name length: CTL_SEN_NAME_LEN
INT32 ctl_sen_unreg_sendrv(CHAR *name);                            ///< name length: CTL_SEN_NAME_LEN

//@}
#endif
