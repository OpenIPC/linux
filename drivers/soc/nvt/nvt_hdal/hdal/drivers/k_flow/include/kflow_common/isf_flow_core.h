/**
    Stream Framework.

    A framework to integrate image data flow of all related application tasks.

    @file       isf_flow_core.h
    @ingroup    mIisf_stream
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/

#ifndef _ISF_FLOW_CORE_H_
#define _ISF_FLOW_CORE_H_

#include "kwrap/platform.h"
#include "kwrap/type.h"
#include "kflow_common/type_vdo.h"
#include "kflow_common/type_aud.h"

extern void isf_flow_install_id(void) _SECTION(".kercfg_text");
extern void isf_flow_uninstall_id(void) _SECTION(".kercfg_text");

/**
     @name Signature value
*/
//@{
#define ISF_SIGN_STREAM     MAKEFOURCC('I','S','F','S')
#define ISF_SIGN_UNIT       MAKEFOURCC('I','S','F','U')
#define ISF_SIGN_PORT       MAKEFOURCC('I','S','F','P')
#define ISF_SIGN_DATA       MAKEFOURCC('I','S','F','D')
#define ISF_SIGN_QUEUE      MAKEFOURCC('I','S','F','Q')
//@}

/**
     Port command
*/
typedef enum {
	////////////////////////////////////////////////////////////////////////////////////////////////
	// begin cmd (forward)
	ISF_PORT_CMD_CONNECT            = 0x00000001, ///< connect -> off
	ISF_PORT_CMD_OPEN               = 0x00000002, ///< off -> ready
	ISF_PORT_CMD_START              = 0x00000004, ///< ready -> run
	ISF_PORT_CMD_RESUME             = 0x00000008, ///< wait -> run
	ISF_PORT_CMD_WAKEUP             = 0x00000010, ///< sleep -> run
	ISF_PORT_CMD_S1                 = 0x00000020, ///<
	ISF_PORT_CMD_S0                 = 0x00000040, ///<
	ISF_PORT_CMD_RESET              = 0x00000080, ///< reset parameters to default
	////////////////////////////////////////////////////////////////////////////////////////////////
	// sync cmd (forward)
	ISF_PORT_CMD_RUNTIME_UPDATE     = 0x00000100, ///< run -> run (update change)
	// sync cmd (backward)
	ISF_PORT_CMD_OFFTIME_SYNC       = 0x00000200, ///< off -> off (off-time property is dirty)
	ISF_PORT_CMD_READYTIME_SYNC     = 0x00000400, ///< ready -> ready (ready-time property is dirty)
	ISF_PORT_CMD_RUNTIME_SYNC       = 0x00000800, ///< run -> run (run-time property is dirty)
	////////////////////////////////////////////////////////////////////////////////////////////////
	// end cmd (backward)
	ISF_PORT_CMD_DISCONNECT         = 0x00001000, ///< off -> disconnect
	ISF_PORT_CMD_CLOSE              = 0x00002000, ///< stop -> off
	ISF_PORT_CMD_STOP               = 0x00004000, ///< run -> stop
	ISF_PORT_CMD_SLEEP              = 0x00008000, ///< run -> sleep
	ISF_PORT_CMD_PAUSE              = 0x00010000, ///< run -> wait
	ISF_PORT_MASK_STATE             = 0x0001ffff, ///< state mask
	ISF_PORT_CMD_QUERY              = 0x00020000, ///< query device or output state
	////////////////////////////////////////////////////////////////////////////////////////////////
	ISF_PORT_CMD_OUTPUT             = 0x01000000, ///< output is dirty
	ISF_PORT_CMD_STATE              = 0x02000000, ///< state is dirty
	ISF_PORT_CMD_INPUT_INFO         = 0x10000000, ///< property is dirty (input port info)
	ISF_PORT_CMD_OUTPUT_INFO        = 0x20000000, ///< property is dirty (output port info)
	//ISF_PORT_CMD_INPUT_PARAM        = 0x40000000, ///< property is dirty (input port param)
	//ISF_PORT_CMD_OUTPUT_PARAM       = 0x80000000, ///< property is dirty (output port param)
	ENUM_DUMMY4WORD(ISF_PORT_CMD)
} ISF_PORT_CMD;


#define ISF_IS_OPORT(p_unit, nport)     ((((UINT32)(nport)) < ISF_IN_BASE) && (((UINT32)(nport)) < (p_unit)->port_out_count))

#define ISF_IS_IPORT(p_unit, nport)     ((((UINT32)(nport)) >= ISF_IN_BASE) && (((UINT32)(nport))-ISF_IN_BASE < (p_unit)->port_in_count))

#define ISF_VDO_HEADER_SIZE    1024   ///< size of VRAW internal header in BYTES, include ISF_DATA + VDO_FRAME + (internal info)

struct _ISF_BASE_UNIT;
struct _ISF_UNIT;
struct _ISF_STREAM;


/**
     Stream Data Class.
*/
typedef struct _ISF_DATA_CLASS {
	UINT32 this;
	UINT32 base;
	//data
	UINT32 (*do_new)(struct _ISF_DATA *p_data, struct _ISF_UNIT *p_thisunit, UINT32 pool, UINT32 buf_size, UINT32 ddr); ///< CB to new "custom data handle", or NULL if h_data is "nvtmpp blk_id"
	ISF_RV (*do_lock)(struct _ISF_DATA *p_data, struct _ISF_UNIT *p_thisunit, UINT32 h_data); ///< CB to lock "custom data handle", or NULL if h_data is "nvtmpp blk_id"
	ISF_RV (*do_unlock)(struct _ISF_DATA *p_data, struct _ISF_UNIT *p_thisunit, UINT32 h_data); ///< CB to unlock "custom data handle", or NULL if h_data is "nvtmpp blk_id"
	ISF_RV (*do_init)(struct _ISF_DATA *p_data, UINT32 version); //init
	ISF_RV (*do_verify)(struct _ISF_DATA *p_data); //verify
	ISF_RV (*do_load)(struct _ISF_DATA *p_data, struct _ISF_UNIT *p_thisunit, INT32 pd, INT32 pi); //load from file
	ISF_RV (*do_save)(struct _ISF_DATA *p_data, struct _ISF_UNIT *p_thisunit, INT32 pd, INT32 pi); //save to file
	ISF_RV (*do_probe)(struct _ISF_DATA *p_data, struct _ISF_UNIT *p_thisunit, INT32 pd, INT32 pi, CHAR* act); //log
	ISF_RV (*do_perf)(struct _ISF_DATA *p_data, struct _ISF_UNIT *p_thisunit, INT32 pd, INT32 pi); //perf
} ISF_DATA_CLASS;


/**
    Stream state.
*/
typedef struct _ISF_STATE_ {
	struct _ISF_STREAM *p_stream; ///< config for stream
	UINT32 list_id;
	ISF_PORT_STATE statecfg; ///< new port state
	ISF_PORT_STATE state; ///< current port state
	UINT32 dirty; ///< dirty cmd
} ISF_STATE;

#define ISF_PORT_INFO_SIZE  24  ///< size of port-info in WORDS

/**
    Stream port.

    NOTE: if get any push or pull pDataBuf pointer, user must verify data format by desc[0] FOURCC, then cast it to related struct pointer:

	if (p_data->desc[0] == MAKEFOURCC('V','F','R','M'))
        	=> video yuv         	=> cast to VDO_FRAME* and if (VDO_PXLFMT_CLASS(pxlfmt) == VDO_PXLFMT_CLASS_YUV)
		=> video yuv compress => cast to VDO_FRAME* and if (VDO_PXLFMT_CLASS(pxlfmt) == VDO_PXLFMT_CLASS_NVX)
    		=> video raw   		=> cast to VDO_FRAME* and if (VDO_PXLFMT_CLASS(pxlfmt) == VDO_PXLFMT_CLASS_RAW)
    		=> video raw compress => cast to VDO_FRAME* and if (VDO_PXLFMT_CLASS(pxlfmt) == VDO_PXLFMT_CLASS_NRX)

	if (p_data->desc[0] == MAKEFOURCC('A','F','R','M'))
		=> audio pcm        	=> cast to VDO_FRAME*

	if (p_data->desc[0] == MAKEFOURCC('V','S','T','M'))
        => video stream      	=> cast to VDO_BITSTREAM*

	if (p_data->desc[0] == MAKEFOURCC('A','S','T','M'))
        => audio stream      	=> cast to AUD_BITSTREAM*
*/
typedef struct _ISF_PORT_ {
	UINT32 sign; // ///< signature, equal to ISF_SIGN_PORT
	ISF_CONNECT_TYPE connecttype; ///< connect type
	UINT32 iport; ///< input-port ID of p_destunit
	struct _ISF_UNIT *p_destunit; ///< pointer to dest unit (cannot be NULL)
	UINT32 oport; ///< output-port ID of p_srcunit
	struct _ISF_UNIT *p_srcunit; ///< pointer to src unit (NULL if disconnect)
	//UINT32 info[ISF_PORT_INFO_SIZE]; ///< vdo/aud info of this port
	UINT32 flagcfg; ///< ctrl flag (config), default 0
	UINT32 flag; ///< ctrl flag, default 0
	UINT32 attr; ///< reserved
	// push interface for transfer image buffer
	ISF_RV (*do_push)(struct _ISF_PORT_* p_port, ISF_DATA *p_data, INT32 wait_ms); ///< CB to push data to dest unit (for PUSH connect type)(provided by port-caps of p_destunit, called by p_srcunit)
	// pull interface for transfer image buffer
	ISF_RV (*do_notify)(struct _ISF_PORT_* p_port, ISF_DATA *p_syncdata, INT32 wait_ms); ///< CB to send notify to dest unit (for PULL connect type)
	ISF_RV (*do_pull)(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms);  ///< CB to pull data from src unit (for PULL connect type)(provided by port-caps of p_srcunit, called by p_destunit)
}
ISF_PORT;

//framework level debug msg API

//Class mask of do_debug() and "isf debug" cmd
//Class mask of do_trace() and "isf trace" cmd
#define ISF_OP_COMMAND			0x1000 //dispatch bind/unbind, open/close, start/stop
#define ISF_OP_STATE				0x2000 //do open/start/stop/close
#define ISF_OP_BIND				0x4000 //do bind/unbind, set bind type
#define ISF_OP_PARAM_CTRL			0x0100 //set ctrl port parameters: enable, mode, device mode, buffer, queue depth
#define ISF_OP_PARAM_CTRL_IMM		0x0200 //set ctrl port IMM parameters: ?
#define ISF_OP_PARAM_GENERAL		0x0400 //set i/o port general parameters: enable, mode, device mode, buffer, queue depth
#define ISF_OP_PARAM_GENERAL_IMM	0x0800 //set i/o port general IMM parameters: ?
#define ISF_OP_PARAM_VDOFRM		0x0010 //set i/o port video frame parameters: size, pxlfmt, window, direct
#define ISF_OP_PARAM_VDOFRM_IMM	0x0020 //set i/o port video frame IMM parameters: frc, crop
#define ISF_OP_PARAM_VDOBS			0x0040 //set i/o port video bitstream parameters: codec and related
#define ISF_OP_PARAM_VDOBS_IMM		0x0080 //set i/o port video bitstream IMM parameters: codec and related
#define ISF_OP_PARAM_AUDFRM		0x0001 //set i/o port audio frame parameters: bps, channel
#define ISF_OP_PARAM_AUDFRM_IMM	0x0002 //set i/o port audio frame IMM parameters: samplerate
#define ISF_OP_PARAM_AUDBS			0x0004 //set i/o port audio bitstream parameters: codec and related
#define ISF_OP_PARAM_AUDBS_IMM		0x0008 //set i/o port audio bitstream IMM parameters: codec and related

//Class mask of "isf probe" cmd
#define ISF_TS_LAST	0
#define ISF_TS_THIS	1
#define ISF_TS_DIFF	2
#define ISF_TS_SECOND	3
#define ISF_TS_MAX	4

//IN: PUSH-EXT-PROC-REL
#define ISF_IN_PROBE_PUSH			0x1000 // begin push from src / end of push
#define ISF_IN_PROBE_PUSH_DROP     	0x2000 // while push, drop with acceptable reason  (force drop, or just new fail)
#define ISF_IN_PROBE_PUSH_WRN     	0x4000 // while push, drop with abnormal data (require to fix input data)
#define ISF_IN_PROBE_PUSH_ERR     	0x8000 // while push, drop with serious error (require to fix state or param from hdal)
#define ISF_IN_PROBE_EXT			0x0100 // begin ext / end of ext
#define ISF_IN_PROBE_EXT_DROP     	0x0200 // while ext, drop with acceptable reason
#define ISF_IN_PROBE_EXT_WRN     	0x0400 // while ext, drop with abnormal data
#define ISF_IN_PROBE_EXT_ERR     	0x0800 // while ext, drop with serious error
#define ISF_IN_PROBE_PROC			0x0010 // begin proc / end of proc
#define ISF_IN_PROBE_PROC_DROP		0x0020 // while proc, drop with acceptable reason
#define ISF_IN_PROBE_PROC_WRN		0x0040 // while proc, drop with abnormal data
#define ISF_IN_PROBE_PROC_ERR		0x0080 // while proc, drop with serious error
#define ISF_IN_PROBE_REL			0x0001 // begin release / end of release

//(common)
//OUT: NEW-PROC-EXT-PUSH-REL
//OUT: NEW-PROC-EXT-PUSH-PULL-REL
//(private)
//OUT: NEW-PROC-EXT-PUSH-REL
//OUT: NEW-PROC-EXT-PUSH-PULL-REL

//OUT: NEW-PROC-EXT-PUSH
#define ISF_OUT_PROBE_NEW			0x1000 // begin new from pool / end of new
#define ISF_OUT_PROBE_NEW_DROP     	0x2000 // while new, drop with acceptable reason
#define ISF_OUT_PROBE_NEW_WRN     	0x4000 // while new, with abnormal data
#define ISF_OUT_PROBE_NEW_ERR     	0x8000 // while new, drop with serious error
#define ISF_OUT_PROBE_PROC			0x0100 // begin proc / end of proc
#define ISF_OUT_PROBE_PROC_DROP	0x0200 // while proc, drop with acceptable reason
#define ISF_OUT_PROBE_PROC_WRN		0x0400 // while proc, drop with abnormal data
#define ISF_OUT_PROBE_PROC_ERR		0x0800 // while proc, drop with serious error
#define ISF_OUT_PROBE_EXT			0x0010 // begin ext / end of ext
#define ISF_OUT_PROBE_EXT_DROP     	0x0020 // while ext, drop with acceptable reason
#define ISF_OUT_PROBE_EXT_WRN     	0x0040 // while ext, drop with abnormal data
#define ISF_OUT_PROBE_EXT_ERR     	0x0080 // while ext, drop with serious error
#define ISF_OUT_PROBE_PUSH			0x0001 // begin push to next / end of push
#define ISF_OUT_PROBE_PUSH_DROP    	0x0002 // while push, drop with acceptable reason (not bind, or queue depth =0)
#define ISF_OUT_PROBE_PUSH_WRN     	0x0004 // while push, drop with abnormal data
#define ISF_OUT_PROBE_PUSH_ERR     	0x0008 // while push, drop with serious error

//USER: PULL--------REL
#define ISF_USER_PROBE_PULL		0x00100000 // begin pull / end of pull
#define ISF_USER_PROBE_PULL_SKIP   	0x00200000 // while pull, skip with acceptable reason
#define ISF_USER_PROBE_PULL_WRN    	0x00400000 // while pull, skip with abnormal data
#define ISF_USER_PROBE_PULL_ERR    	0x00800000 // while pull, skip with serious error
#define ISF_USER_PROBE_REL			0x00010000 // begin release


typedef struct _ISF_IN_STATUS_ {
	UINT32 mode; //0: auto, 1: manual
	UINT32 resv;
	UINT64 ts_push[ISF_TS_MAX];
	UINT64 ts_ext[ISF_TS_MAX];
	UINT64 ts_proc[ISF_TS_MAX];
	UINT64 ts_rel[ISF_TS_MAX];
	UINT16 this_cnt[16];
	UINT16 total_cnt[16];
} ISF_IN_STATUS;

typedef struct _ISF_OUT_STATUS_ {
	UINT32 mode; //0: auto, 1: manual
	UINT32 resv;
	UINT64 ts_new[ISF_TS_MAX];
	UINT64 ts_proc[ISF_TS_MAX];
	UINT64 ts_ext[ISF_TS_MAX];
	UINT64 ts_push[ISF_TS_MAX];
	UINT64 ts_pull[ISF_TS_MAX]; //user
	UINT64 ts_rel[ISF_TS_MAX]; //user + bind
	UINT16 this_cnt[24];
	UINT16 total_cnt[24];
} ISF_OUT_STATUS;

#define ISF_USER_STATUS ISF_OUT_STATUS

typedef struct _ISF_INFO_ {
	UINT32 info[ISF_PORT_INFO_SIZE]; ///< vdo/aud info of this port
	union {
		ISF_IN_STATUS in;
		ISF_OUT_STATUS out;
	} status;
}
ISF_INFO;

/**
     @name param for "dirty of port"
*/
#define ISF_PARAM_PORT_DIRTY		0x8000000f

/**
     @name dirty of port
*/
#define ISF_INFO_CLASS_VDO			0x00010000
#define ISF_INFO_CLASS_AUD			0x00020000
#define ISF_INFO_CLASS_VBS			0x00040000
#define ISF_INFO_CLASS_ABS			0x00080000
#define ISF_INFO_DIRTY_MASK		0x0000ffff
#define ISF_INFO_DIRTY_VDOMAX		0x00000001
#define ISF_INFO_DIRTY_VDOFUNC		0x00000002
#define ISF_INFO_DIRTY_IMGSIZE		0x00000010
#define ISF_INFO_DIRTY_IMGFMT		0x00000020
#define ISF_INFO_DIRTY_DIRECT		0x00000040
#define ISF_INFO_DIRTY_VDOFRAME	0x000000f0
#define ISF_INFO_DIRTY_FRAMERATE	0x00000100
#define ISF_INFO_DIRTY_VDOFPS		0x00000f00
#define ISF_INFO_DIRTY_ASPECT		0x00001000
#define ISF_INFO_DIRTY_WINDOW		0x00002000
#define ISF_INFO_DIRTY_VDOWIN		0x00000f00
//----------------------------------------------
#define ISF_INFO_DIRTY_AUDMAX		0x00000001
#define ISF_INFO_DIRTY_AUDFUNC		0x00000002
#define ISF_INFO_DIRTY_AUDBITS		0x00000010
#define ISF_INFO_DIRTY_AUDCH		0x00000020
#define ISF_INFO_DIRTY_AUDSAMPLE	0x00000040
#define ISF_INFO_DIRTY_AUDFRAME	0x000000f0
#define ISF_INFO_DIRTY_SAMPLERATE	0x00000100
#define ISF_INFO_DIRTY_AUDFPS		0x00000f00

/**
     Port Info: Video.
*/
typedef struct _ISF_VDO_INFO_ {
	//--------------------------------------------------- 2
	UINT32 dirty; ///< dirty mask
	UINT32 sync; ///< sync mask (from binding side)
	//ISF_VDO_FUNC -------------------------------------- 2
	UINT32 src;
	UINT32 func;
	//ISF_VDO_MAX --------------------------------------- 6
	UINT32 buffercount; ///< (config) frame buffer count   ...ISF_INFO_DIRTY_VDOMAX
	UINT32 max_imgfmt; ///< (config) maximum               ...ISF_INFO_DIRTY_VDOMAX
	USIZE max_imgsize; ///< (config) maximum               ...ISF_INFO_DIRTY_VDOMAX
	//ISF_VDO_FRAME ------------------------------------- 6
	UINT32 direct; ///< direction                          ...ISF_INFO_DIRTY_DIRECT or ISF_INFO_DIRTY_VDOFRAME
	UINT32 imgfmt; ///< image format                       ...ISF_INFO_DIRTY_IMGFMT or ISF_INFO_DIRTY_VDOFRAME
	USIZE imgsize; ///< image size                         ...ISF_INFO_DIRTY_IMGSIZE or ISF_INFO_DIRTY_VDOFRAME
	//ISF_VDO_WIN --------------------------------------- 6
	USIZE imgaspect; ///< image aspect size                ...ISF_INFO_DIRTY_ASPECT or ISF_INFO_DIRTY_VDOWIN
	URECT window; ///< crop or cast window range           ...ISF_INFO_DIRTY_WINDOW or ISF_INFO_DIRTY_VDOWIN
	//ISF_VDO_FPS --------------------------------------- 1
	UINT32 framepersecond; ///< frame per second           ...ISF_INFO_DIRTY_FRAMERATE or ISF_INFO_DIRTY_VDOFPS
	UINT32 resv2[4];
	//ISF_ATTR ------------------------------------------ 1
	UINT32 attr; ///< attr flag, default 0
}
ISF_VDO_INFO;
STATIC_ASSERT(sizeof(ISF_VDO_INFO) / sizeof(UINT32) == ISF_PORT_INFO_SIZE);


/**
     Port Info: Audio.
*/
typedef struct _ISF_AUD_INFO_ {
	//--------------------------------------------------- 2
	UINT32 dirty; ///< dirty mask
	UINT32 sync; ///< sync mask (from binding side)
	//ISF_AUD_FUNC -------------------------------------- 2
	UINT32 src;
	UINT32 func;
	//ISF_AUD_MAX --------------------------------------- 4
	UINT32 buffercount; ///< (config) frame buffer count   ...ISF_INFO_DIRTY_AUDMAX
	UINT32 max_bitpersample; ///< (config) maximum         ...ISF_INFO_DIRTY_AUDMAX
	UINT32 max_channelcount; ///< (config) maximum         ...ISF_INFO_DIRTY_AUDMAX
	UINT32 max_samplepersecond; ///< (config) maximum      ...ISF_INFO_DIRTY_AUDMAX
	UINT32 resv2[2];
	//ISF_AUD_FRAME ------------------------------------- 2
	UINT32 bitpersample; ///< sample bits                  ...ISF_INFO_DIRTY_AUDBITS or ISF_INFO_DIRTY_AUDFRAME
	UINT32 channelcount; ///< channel count                ...ISF_INFO_DIRTY_AUDCH or ISF_INFO_DIRTY_AUDFRAME
	UINT32 samplecount; ///< audio sample count            ...ISF_INFO_DIRTY_AUDSAMPLE or ISF_INFO_DIRTY_AUDFRAME
	UINT32 resv3[3];
	//--------------------------------------------------- 0
	UINT32 resv4[6];
	//ISF_AUD_FPS --------------------------------------- 1
	UINT32 samplepersecond; ///< sample rate               ...ISF_INFO_DIRTY_SAMPLERATE or ISF_INFO_DIRTY_AUDFPS
	//ISF_ATTR ------------------------------------------ 1
	UINT32 attr; ///< attr flag, default 0
} ISF_AUD_INFO;
STATIC_ASSERT(sizeof(ISF_AUD_INFO) / sizeof(UINT32) == ISF_PORT_INFO_SIZE);


/**
     Caps of Stream port.
*/
typedef struct _ISF_PORT_CAPS {
	ISF_CONNECT_TYPE connecttype_caps; ///< caps of connect type
	// push interface for transfer image buffer
	ISF_RV (*do_push)(struct _ISF_PORT_* p_port, ISF_DATA *p_data, INT32 wait_ms); ///< CB to push data to dest unit (for PUSH connect type)
	// pull interface for transfer image buffer
	ISF_RV (*do_notify)(struct _ISF_PORT_* p_port, ISF_DATA *p_syncdata, INT32 wait_ms); ///< CB to send notify to dest unit (for PULL connect type)
	ISF_RV (*do_pull)(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms);  ///< CB to pull data from src unit (for PULL connect type)
} ISF_PORT_CAPS;

/**
    Stream i/o path (from input port to output port of a unit).
*/
typedef struct _ISF_PORT_PATH_ {
	struct _ISF_UNIT *p_unit; ///< pointer to this unit (default should not NULL)
	UINT32 iport; ///< input port
	UINT32 oport; ///< output port
	UINT32 p_stream; ///< pointer to working stream (default should be NULL)
	UINT32 flag; ///< select
	UINT32 resv; ///< reserved
} ISF_PORT_PATH;

/**
     Stream Base Unit.
*/
typedef struct _ISF_BASE_UNIT {
	ISF_RV (*reg_data)(ISF_DATA_CLASS* p_data_class);
	UINT32 (*init_data)(ISF_DATA *p_data, ISF_DATA_CLASS* p_thisclass, UINT32 base_class, UINT32 version);
	ISF_PORT* (*iport)(struct _ISF_UNIT *p_thisunit, UINT32 iport);
	ISF_PORT* (*oport)(struct _ISF_UNIT *p_thisunit, UINT32 oport);
	ISF_PORT* (*get_port)(struct _ISF_UNIT *p_unit, UINT32 nport);
	ISF_PORT* (*get_bind_port)(struct _ISF_UNIT *p_unit, UINT32 nport);
	ISF_INFO* (*get_info)(struct _ISF_UNIT *p_unit, UINT32 nport);
	ISF_INFO* (*get_bind_info)(struct _ISF_UNIT *p_unit, UINT32 nport);
	UINT32 (*do_create)(struct _ISF_UNIT *p_thisunit, ISF_PORT *p_srcport, CHAR* pool_name, UINT32 blk_size, UINT32 blk_cnt);  ///< CB to create pool
	ISF_RV (*do_destory)(struct _ISF_UNIT *p_thisunit, ISF_PORT *p_srcport, UINT32 pool);  ///< CB to destory pool
	UINT32 (*do_new_i)(struct _ISF_UNIT *p_thisunit, ISF_PORT *p_srcport, CHAR* pool_name, UINT32 size, ISF_DATA *p_data);  ///< CB to create pool and new data
	ISF_RV (*do_release_i)(struct _ISF_UNIT *p_thisunit, ISF_DATA *p_data, ISF_RV result);  ///< CB to free data and destory pool
	UINT32 (*do_new)(struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 pool, UINT32 attr, UINT32 size, ISF_DATA *p_data, UINT32 probe);  ///< CB to new data and reference =1
	ISF_RV (*do_add)(struct _ISF_UNIT *p_thisunit, UINT32 nport, ISF_DATA *p_data, UINT32 probe);  ///< CB to reference++
	ISF_RV (*do_release)(struct _ISF_UNIT *p_thisunit, UINT32 nport, ISF_DATA *p_data, UINT32 probe);  ///< CB to reference--, and free data if reference ==0
	ISF_RV (*do_push)(struct _ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data, INT32 wait_ms);  ///< CB to push data
	ISF_RV (*do_pull)(struct _ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data, INT32 wait_ms);  ///< CB to pull data
	ISF_RV (*do_notify)(struct _ISF_UNIT *p_thisunit, ISF_PORT *p_srcport, ISF_DATA *p_data, INT32 wait_ms, ISF_RV result, ISF_PORT *p_destport);  ///< CB to notify & sync data
	BOOL (*get_is_allow_push)(struct _ISF_UNIT *p_thisunit, ISF_PORT *p_destport);
	BOOL (*get_is_allow_pull)(struct _ISF_UNIT *p_thisunit, ISF_PORT *p_srcport);
	BOOL (*get_is_allow_notify)(struct _ISF_UNIT *p_thisunit, ISF_PORT *p_srcport);
	void (*do_probe)(struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 probe, ISF_RV result);
	void (*do_trace)(struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 opclass, const char *fmtstr, ...);
	void (*do_debug)(struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 opclass, const char *fmtstr, ...);
	ISF_RV (*do_clear_ctx)(struct _ISF_UNIT *p_thisunit, UINT32 oport);
	ISF_RV (*do_clear_bind)(struct _ISF_UNIT *p_thisunit, UINT32 oport);
	ISF_RV (*do_clear_state)(struct _ISF_UNIT *p_thisunit, UINT32 oport);

} ISF_BASE_UNIT;



/**
     Stream Unit.
*/
typedef struct _ISF_UNIT {
	UINT32 sign; ///< signature, equal to ISF_SIGN_UINT
	CHAR *unit_name; ///< name string
	UINT64 unit_module; ///< nvtmpp module name
	UINT32 list_id; ///< id of stream working list
	UINT32 port_in_count; ///< input-port count
	UINT32 port_out_count; ///< output-port count
	UINT32 port_path_count; ///< input-output path count
	ISF_PORT **port_in; ///< input-port pointer array (active)
	ISF_PORT **port_out; ///< output-port pointer array (active)
	ISF_PORT **port_outcfg; ///< output-port pointer array (config)
	ISF_PORT_CAPS **port_incaps; ///< input-port-caps pointer array
	ISF_PORT_CAPS **port_outcaps; ///< output-port-caps pointer array
	ISF_INFO **port_ininfo; ///< input-port-info pointer array
	ISF_INFO **port_outinfo; ///< output-port-info pointer array
	ISF_STATE **port_outstate; ///< output-port-info pointer array (active)
	ISF_PORT_PATH *port_path; ///< input-output path array
	UINT32 flagcfg; ///< flag (config)
	UINT32 flag; ///< flag
	UINT32 lock; ///< lock state
	UINT32 dirty; ///< dirty
	UINT32 attr; ///< attr of CTRL port
	void *refdata; ///< reference data pointer
	ISF_BASE_UNIT *p_base; ///< base interface
	// interface for command
	ISF_RV (*do_command)(UINT32 cmd, UINT32 p0, UINT32 p1, UINT32 p2);  ///< CB to command
	// interface for device init/uninit
	ISF_RV (*do_init)(struct _ISF_UNIT *p_thisunit, void* p_ctx);  ///< CB to init
	ISF_RV (*do_uninit)(struct _ISF_UNIT *p_thisunit);  ///< CB to uninit
	// interface for connect/disconnect
	ISF_RV (*do_bindinput)(struct _ISF_UNIT *p_thisunit, UINT32 iport, struct _ISF_UNIT *p_srcunit, UINT32 oport);  ///< CB to check input port
	ISF_RV (*do_bindoutput)(struct _ISF_UNIT *p_thisunit, UINT32 oport, struct _ISF_UNIT *p_destunit, UINT32 iport);  ///< CB to check output port
	// interface after connect
	#if 0
	ISF_RV(*do_setparam)(struct _ISF_UNIT *p_thisunit, UINT32 param, UINT32 value);  ///< CB to set parameter of ISF_CTRL port
	UINT32(*do_getparam)(struct _ISF_UNIT *p_thisunit, UINT32 param);  ///< CB to get parameter of ISF_CTRL port
	#endif
	ISF_RV (*do_setportparam)(struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32 value);  ///< CB to set parameter of ISF_IN() or ISF_OUT() port
	UINT32 (*do_getportparam)(struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param);  ///< CB to get parameter of ISF_IN() or ISF_OUT() port
	ISF_RV (*do_setportstruct)(struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32* p_struct, UINT32 size);  ///< CB to set parameter of ISF_IN() or ISF_OUT() port
	ISF_RV (*do_getportstruct)(struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32* p_struct, UINT32 size);  ///< CB to get parameter of ISF_IN() or ISF_OUT() port
	ISF_RV (*do_update)(struct _ISF_UNIT *p_thisunit, UINT32 oport, ISF_PORT_CMD cmd);  ///< CB to update a ISF_OUT() port
	BOOL (*debug)(CHAR *strCmd);
} ISF_UNIT;



///////////////////////////////////////////////////////////////////////////////
// unit operation
extern ISF_PORT *_isf_unit_in(ISF_UNIT *p_unit, UINT32 iport);
extern ISF_PORT *_isf_unit_out(ISF_UNIT *p_unit, UINT32 oport);
extern ISF_PORT *_isf_unit_port(ISF_UNIT *p_unit, UINT32 nport);
extern ISF_PORT *_isf_bind_port(ISF_UNIT *p_unit, UINT32 nport);
extern ISF_INFO *_isf_unit_info(ISF_UNIT *p_unit, UINT32 nport);
extern ISF_INFO *_isf_bind_info(ISF_UNIT *p_unit, UINT32 nport);

extern ISF_PORT_STATE _isf_unit_state(ISF_UNIT *p_unit, UINT32 oport);

extern void _isf_unit_begin(ISF_UNIT *p_unit, UINT32 flag);
extern void _isf_unit_end(void);
extern ISF_RV _isf_unit_set_param(ISF_UNIT *p_unit, UINT32 nport, UINT32 param, UINT32* p_value);
extern ISF_RV _isf_unit_set_struct_param(ISF_UNIT *p_unit, UINT32 nport, UINT32 param, UINT32* p_value, UINT32 size);
extern ISF_RV _isf_unit_get_param(ISF_UNIT *p_unit, UINT32 nport, UINT32 param, UINT32* p_value);
extern ISF_RV _isf_unit_get_struct_param(ISF_UNIT *p_unit, UINT32 nport, UINT32 param, UINT32* p_value, UINT32 size);
extern ISF_RV _isf_unit_clear_context(ISF_UNIT *p_unit, UINT32 oport);

extern ISF_RV _isf_unit_set_bind(ISF_UNIT *p_unit, UINT32 oport, ISF_UNIT *p_destunit, UINT32 iport);
extern ISF_RV _isf_unit_get_bind(ISF_UNIT *p_unit, UINT32 nport, ISF_UNIT **pp_destunit, UINT32* p_nport);
extern ISF_RV _isf_unit_clear_bind(ISF_UNIT *p_unit, UINT32 oport);
extern ISF_RV _isf_unit_set_state(ISF_UNIT *p_unit, UINT32 oport, ISF_PORT_CMD cmd);
extern ISF_RV _isf_unit_get_state(ISF_UNIT *p_unit, UINT32 oport, ISF_PORT_STATE* p_state);
extern ISF_RV _isf_unit_clear_state(ISF_UNIT *p_unit, UINT32 oport);


///////////////////////////////////////////////////////////////////////////////
// data operation
extern UINT32 _isf_unit_init_data(ISF_DATA *p_data, ISF_DATA_CLASS* p_thisclass, UINT32 base_class, UINT32 version);
extern UINT32 _isf_unit_new_data(UINT32 size, ISF_DATA *p_data);
extern ISF_RV _isf_unit_add_data(ISF_DATA *p_data);
extern ISF_RV _isf_unit_release_data_from_user(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data);
extern BOOL _isf_unit_is_allow_push(ISF_PORT *p_destport);
extern ISF_RV _isf_unit_push_data_from_user(ISF_UNIT *p_thisunit, UINT32 iport, ISF_DATA *p_data, INT32 wait_ms);
extern BOOL _isf_unit_is_allow_pull(ISF_PORT *p_srcport);
extern ISF_RV _isf_unit_pull_data_from_user(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data, INT32 wait_ms);
extern BOOL _isf_unit_is_allow_notify(ISF_PORT *p_srcport);
extern ISF_RV _isf_unit_notify(ISF_PORT *p_srcport, ISF_DATA* p_syncdata);
extern ISF_RV _isf_unit_send_event(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 event, UINT32 param1, UINT32 param2, UINT32 param3);


/////////////////////////////////////////////////////////////
// util

extern UINT32 _isf_div64(UINT64 dividend, UINT64 divisor);


#define ISF_FRC_UNIT       		100000

#define ISF_SAMPLE_OFF			0 //never trigger
#define ISF_SAMPLE_CAPTURE			1 //trigger n frame, then stop
#define ISF_SAMPLE_PREVIEW			2 //always trigger, do frc
#define ISF_SAMPLE_ALL			3 //always trigger, no frc

typedef struct _ISF_FRC {
	UINT32		sample_mode;
	UINT32		rate;
	UINT32		frm_counter;
	UINT32		rate_counter;
	UINT32		output_counter;
	UINT32		framepersecond;
	UINT32		framepersecond_new;
} ISF_FRC;

extern BOOL _isf_frc_start(ISF_UNIT* p_unit, UINT32 nport, ISF_FRC *p_frc, UINT32 framepersecond);
extern BOOL _isf_frc_stop(ISF_UNIT* p_unit, UINT32 nport, ISF_FRC *p_frc);
extern BOOL _isf_frc_update(ISF_UNIT* p_unit, UINT32 nport, ISF_FRC *p_frc, UINT32 framepersecond);
extern BOOL _isf_frc_update_imm(ISF_UNIT* p_unit, UINT32 nport, ISF_FRC *p_frc, UINT32 framepersecond_new);
extern UINT32 _isf_frc_is_select(ISF_UNIT* p_unit, UINT32 nport, ISF_FRC *p_frc);

typedef struct _ISF_DATA_QUEUE {
	UINT32 		sign; ///< signature, equal to ISF_SIGN_QUEUE
	CHAR *		unit_name; ///< name string
	UINT32 		id; ///< id of this module
	UINT32		en;
	UINT32		head;	///< head pointer
	UINT32		tail;	///< tail pointer
	UINT32		is_full;	///< full flag
	UINT32		cnt; ///< current count
	UINT32		max;		///< max count
	ISF_DATA*	data;
	void*		p_sem;
	void*     	p_sem_q;
} ISF_DATA_QUEUE, *PISF_DATA_QUEUE;

extern ISF_RV _isf_queue2_open(PISF_DATA_QUEUE p_queue, void* p_sem, void* p_sem_q);
extern ISF_RV _isf_queue2_pre_start(PISF_DATA_QUEUE p_queue);
extern ISF_RV _isf_queue2_start(PISF_DATA_QUEUE p_queue, UINT32 n, ISF_DATA* p_data);
extern ISF_RV _isf_queue2_pre_stop(PISF_DATA_QUEUE p_queue); //reject put, allow get
extern ISF_RV _isf_queue2_stop(PISF_DATA_QUEUE p_queue);
extern ISF_RV _isf_queue2_force_stop(PISF_DATA_QUEUE p_queue);
extern BOOL _isf_queue2_is_empty(PISF_DATA_QUEUE p_queue);
extern BOOL _isf_queue2_is_full(PISF_DATA_QUEUE p_queue);
extern ISF_RV _isf_queue2_put_data(PISF_DATA_QUEUE p_queue, ISF_DATA *data_in);
extern ISF_RV _isf_queue2_get_data(PISF_DATA_QUEUE p_queue, ISF_DATA *data_out, INT32 wait_ms);
extern ISF_RV _isf_queue2_close(PISF_DATA_QUEUE p_queue);


//@}


#define DBG_FPS(p_thisunit, port_str, i, action_str) \
	{ \
		static UINT64 fps_t1, fps_t2; \
		static INT32 fps_diff,fps_count=0; \
	    if (fps_count == 0) \
			fps_t1 = HwClock_GetLongCounter(); \
	    fps_count++; \
	    fps_t2 =  HwClock_GetLongCounter(); \
	    fps_diff = GET_HI_UINT32(fps_t2)-GET_HI_UINT32(fps_t1); \
	    if (fps_diff > 0) { \
			float fps = (((float)(fps_count-1))*1000000)/(((float)fps_diff)*1000000 + GET_LO_UINT32(fps_t2) - GET_LO_UINT32(fps_t1)); \
			DBG_DUMP("^C\"%s\"."port_str"[%d] "action_str"! -- %f Frame/sec\r\n", (p_thisunit)->unit_name,(i), fps);  \
		    fps_count = 0; \
	    } \
	}

#if _TODO
//cmd
extern void isf_stream_install_cmd(void);
#endif

///////////////////////////////////////////////////////////////////////////////

extern ISF_RV isf_reg_unit(UINT32 unit_id, ISF_UNIT* p_unit); //only EXPORT this API for unit!
extern ISF_UNIT* isf_unit_ptr(UINT32 unit_id);

extern ISF_RV isf_data_regclass(ISF_DATA_CLASS* p_data_class);
extern ISF_DATA_CLASS* isf_data_class(UINT32 fourcc);
//extern ISF_RV isf_init(UINT32 h_isf, UINT32 p0, UINT32 p1, UINT32 p2);
//extern ISF_RV isf_exit(UINT32 h_isf, UINT32 p0, UINT32 p1, UINT32 p2);

extern ISF_RV _isf_probe_is_ready(ISF_UNIT *p_thisunit, UINT32 nport);


extern ISF_DATA_CLASS _common_base_data;
extern ISF_DATA_CLASS _vdofm_base_data;
extern ISF_DATA_CLASS _audfm_base_data;
extern ISF_DATA_CLASS _vdobs_base_data;
extern ISF_DATA_CLASS _audbs_base_data;


#endif //_ISF_STREAM_CORE_H_
