/*
    Copyright   Novatek Microelectronics Corp. 2005~2014.  All rights reserved.

    @file       ImageUnit_VdoIn.h
    @ingroup    mVdoIn

    @note       Nothing.

    @date       2017/10/01
*/

#ifndef IMAGEUNIT_VDOENC_H
#define IMAGEUNIT_VDOENC_H


#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#define _SECTION(sec)

#if defined(_BSP_NA51000_)
#define VDOENC_MAX_PATH_NUM	16
#endif
#if defined(_BSP_NA51023_)
#define VDOENC_MAX_PATH_NUM	8
#endif
#if defined(_BSP_NA51055_)
#if defined(CONFIG_NVT_SMALL_HDAL)
#define VDOENC_MAX_PATH_NUM	8
#else
#define VDOENC_MAX_PATH_NUM	16
#endif
#endif
#if defined(_BSP_NA51089_)
#if defined(CONFIG_NVT_SMALL_HDAL)
#define VDOENC_MAX_PATH_NUM	8
#else
#define VDOENC_MAX_PATH_NUM	16
#endif

#endif


typedef struct {
	UINT32 SrcOutYAddr;     ///< Src Out Y Addr
	UINT32 SrcOutUVAddr;	///< Src Out UV Addr
	UINT32 SrcOutWidth;     ///< Src Out Width
	UINT32 SrcOutHeight;    ///< Src Out Height
	UINT32 SrcOutYLoft;     ///< Src Out Y Line Offset
	UINT32 SrcOutUVLoft;    ///< Src Out UV Line Offset
} VDOENC_SOURCBS_INFO, *PVDOENC_SOURCBS_INFO;

typedef struct {
	UINT32 phyaddr;         ///< memory addr to put JPEG bitstream
	UINT32 size;            ///< memory size to put JPEG bitstream
	UINT32 image_quality;   ///< image quality
	INT32  timeout;         ///< timeout, unit: ms
	UINT64 timestamp;       ///< encoded YUV timestamp
} VDOENC_SNAP_INFO, *PVDOENC_SNAP_INFO;

typedef struct {
	UINT32 path_mask;       ///< path to check
	INT32  wait_ms;         ///< timeout, unit: ms
} VDOENC_POLL_LIST_BS_INFO, *PVDOENC_POLL_LIST_BS_INFO;

enum {
	VDOENC_PARAM_START = 0x0000F000,
	VDOENC_PARAM_CODEC = VDOENC_PARAM_START,
	VDOENC_PARAM_PROFILE,
	VDOENC_PARAM_TARGETRATE,
	VDOENC_PARAM_ENCODER_OBJ,
	VDOENC_PARAM_MAX_MEM_INFO,
	VDOENC_PARAM_ENCBUF_MS,						//unit: ms
	VDOENC_PARAM_ENCBUF_RESERVED_MS,			//unit: ms
	VDOENC_PARAM_FRAMERATE,
	VDOENC_PARAM_GOPTYPE,
	VDOENC_PARAM_GOPNUM,
	VDOENC_PARAM_RECFORMAT,
	VDOENC_PARAM_FILETYPE,
	VDOENC_PARAM_INITQP,
	VDOENC_PARAM_MINQP,
	VDOENC_PARAM_MAXQP,
	VDOENC_PARAM_MIN_I_RATIO,
	VDOENC_PARAM_MIN_P_RATIO,
	VDOENC_PARAM_MAX_FRAME_QUEUE,
	VDOENC_PARAM_DAR,
	VDOENC_PARAM_SKIP_FRAME,
	VDOENC_PARAM_SVC,
	VDOENC_PARAM_LTR,
	VDOENC_PARAM_MULTI_TEMPORARY_LAYER,
	VDOENC_PARAM_USRQP,
	VDOENC_PARAM_CBRINFO,
	VDOENC_PARAM_EVBRINFO,
	VDOENC_PARAM_VBRINFO,
	VDOENC_PARAM_FIXQPINFO,
	VDOENC_PARAM_AQINFO,
	VDOENC_PARAM_3DNR_CB,
	VDOENC_PARAM_IMM_CB,
	VDOENC_PARAM_FRAME_INTERVAL,
	VDOENC_PARAM_EVENT_CB,
	VDOENC_PARAM_ENCBUF_ADDR,
	VDOENC_PARAM_ENCBUF_SIZE,
	VDOENC_PARAM_ROI,
	VDOENC_PARAM_DIS,
	VDOENC_PARAM_TV_RANGE,
	VDOENC_PARAM_START_TIMER_BY_MANUAL,
	VDOENC_PARAM_START_TIMER,
	VDOENC_PARAM_STOP_TIMER,
	VDOENC_PARAM_START_SMART_ROI,
	VDOENC_PARAM_WAIT_SMART_ROI,
	VDOENC_PARAM_RESET_IFRAME,
	VDOENC_PARAM_RESET_SEC,
	VDOENC_PARAM_RAW_DATA_CB,
	VDOENC_PARAM_SRC_OUT_CB,
	VDOENC_PARAM_TIMELAPSE_TIME,
	VDOENC_PARAM_ALLOC_SNAPSHOT_BUF,
	VDOENC_PARAM_SNAPSHOT,
	VDOENC_PARAM_SNAPSHOT_PURE_LINUX,
	VDOENC_PARAM_SYNCSRC,
	VDOENC_PARAM_FPS_IMM,
	VDOENC_PARAM_FUNC,
	VDOENC_PARAM_PROC_CB,
	VDOENC_PARAM_BUFINFO_PHYADDR,       // work(max) buffer, physical addr
	VDOENC_PARAM_BUFINFO_SIZE,          // work(max) buffer, size
	VDOENC_PARAM_ISP_ID,
	VDOENC_PARAM_LEVEL_IDC,
	VDOENC_PARAM_ENTROPY,
	VDOENC_PARAM_GRAY,
	VDOENC_PARAM_DEBLOCK,
	VDOENC_PARAM_GDR,
	VDOENC_PARAM_VUI,
	VDOENC_PARAM_SLICE_SPLIT,
	VDOENC_PARAM_USR_QP_MAP,
	VDOENC_PARAM_JPG_QUALITY,            // jpeg encode quality
	VDOENC_PARAM_JPG_RESTART_INTERVAL,   // jpeg encode restart interval
	VDOENC_PARAM_JPG_BITRATE,            // jpeg encode bitrate
	VDOENC_PARAM_JPG_FRAMERATE,          // jpeg encode framerate
	VDOENC_PARAM_JPG_VBR_MODE,           // jpeg encode rc mode, 0: CBR , 1: VBR
	VDOENC_PARAM_POLL_LIST_BS,           // poll multi path to check bs buffer available
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	VDOENC_PARAM_ROWRC,
#endif
	VDOENC_PARAM_BS_RESERVED_SIZE,
	VDOENC_PARAM_RDO,
	VDOENC_PARAM_JND,
	VDOENC_PARAM_H26X_VBR_POLICY,        // h26x vbr policy, 0: original, 1: hisi-like
	VDOENC_PARAM_H26X_RC_GOPNUM,         // h26x rc gop
	VDOENC_PARAM_GOPNUM_ONSTART,         // h26x enc gop on start
	VDOENC_PARAM_JPG_YUV_TRAN_ENABLE,    // input YUV420, output 422 bs
	VDOENC_PARAM_JPG_RC_MIN_QUALITY,     // jpeg encode rc, min quality
	VDOENC_PARAM_JPG_RC_MAX_QUALITY,     // jpeg encode rc, max quality
	VDOENC_PARAM_HW_LIMIT,
	VDOENC_PARAM_COLMV_ENABLE,
	VDOENC_PARAM_MAXMEM_FIT_WORK_MEMORY,
	VDOENC_PARAM_LONG_START_CODE,
	VDOENC_PARAM_H26X_SVC_WEIGHT_MODE,  // h26x svc wright mode, 0: original, 1: svc weitht mode
	VDOENC_PARAM_QUALITY_BASE_MODE_ENABLE,
	VDOENC_PARAM_DMA_ABORT,
	VDOENC_PARAM_SMALL_HDAL,
	VDOENC_PARAM_COMM_RECFRM_ENABLE,
	VDOENC_PARAM_COMM_BASE_RECFRM,
	VDOENC_PARAM_COMM_SVC_RECFRM,
	VDOENC_PARAM_COMM_LTR_RECFRM,
	VDOENC_PARAM_BS_QUICK_ROLLBACK,
	VDOENC_PARAM_HW_PADDING_MODE,
	__VDOENC_PARAM_MAX__,

	// ImgCap
	VDOENC_PARAM_IMGCAP_THUMB,
	VDOENC_PARAM_IMGCAP_ACTION,
	VDOENC_PARAM_IMGCAP_MAX_W,
	VDOENC_PARAM_IMGCAP_MAX_H,
	VDOENC_PARAM_IMGCAP_JPG_BUFNUM,    // set jpg buffer number
	VDOENC_PARAM_IMGCAP_GET_JPG_ADDR,   // get jpg encoded addr
	VDOENC_PARAM_IMGCAP_GET_JPG_SIZE,   // get jpg encoded size
	VDOENC_PARAM_IMGCAP_MAX_MEM_INFO,
};


typedef void (IsfVdoEncEventCb)(CHAR *Name, UINT32 event_id, UINT32 value);
typedef UINT32 (*VDOENC_PROC_CB)(UINT32 id, UINT32 state);

extern ISF_UNIT isf_vdoenc;
extern ISF_PORT_CAPS **ISF_VdoEnc_InputPortList_Caps;
extern ISF_PORT_CAPS ISF_VdoEnc_Input_Caps;
extern ISF_PORT_CAPS **ISF_VdoEnc_OutputPortList_Caps;
extern ISF_PORT_CAPS ISF_VdoEnc_OutputPort_Caps;
extern ISF_STATE  *ISF_VdoEnc_OutputPort_State;
extern ISF_STATE **ISF_VdoEnc_OutputPortList_State;
extern ISF_INFO *isf_vdoenc_outputinfo_in;
extern ISF_INFO **isf_vdoenc_outputinfolist_in;
extern ISF_INFO *isf_vdoenc_outputinfo_out;
extern ISF_INFO **isf_vdoenc_outputinfolist_out;
extern ISF_PORT_PATH *ISF_VdoEnc_PathList;

#define VDOENC_FUNC_NOWAIT          ((UINT32)0x00000001 << 0)   //0x00000001   //do not wait for encode finish
#define VDOENC_FUNC_LOWLATENCY      ((UINT32)0x00000001 << 1)   //0x00000002   //ime-codec sync for low latency

extern void isf_vdoenc_install_id(void) _SECTION(".kercfg_text");
extern void isf_vdoenc_uninstall_id(void);
extern SEM_HANDLE _SECTION(".kercfg_data") ISF_VDOENC_SEM_ID[];
extern SEM_HANDLE _SECTION(".kercfg_data") ISF_VDOENC_COMMON_SEM_ID;


#endif //IMAGEUNIT_VDOENC_680_H

