/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       isf_audout_int.h

    @brief      isf_audout internal header file

    @version    V1.00.000
    @author     Novatek FW Team
    @date       2018/06/04
*/
#ifndef _ISF_AUDOUT_INT_H
#define _ISF_AUDOUT_INT_H

#ifdef __KERNEL__
#include "kwrap/error_no.h"
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/semaphore.h"
#include "kwrap/util.h"
#include "kwrap/task.h"
#include "comm/hwclock.h"
#include "kflow_common/nvtmpp.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#include "kflow_audioout/isf_audout.h"
#include "kflow_audiocapture/wavstudio_tsk.h"
#include "kflow_audiocapture/ctl_aud.h"
#include "../include/isf_audout_dbg.h"

#define debug_msg                      vk_printk
#else
#include "kwrap/error_no.h"
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/semaphore.h"
#include "kwrap/util.h"
#include "kwrap/task.h"
#include "comm/hwclock.h"
#include "kflow_common/nvtmpp.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#include "kflow_audioout/isf_audout.h"
#include "kflow_audiocapture/wavstudio_tsk.h"
#include "kflow_audiocapture/ctl_aud.h"
#include <string.h>
#include "../include/isf_audout_dbg.h"

#define debug_msg                      vk_printk
#define msecs_to_jiffies(x)	0
#endif

typedef enum _AUDOUT_EVT {
	AUDOUT_EVT_TCHIT = 0,           ///< Time code hit
	AUDOUT_EVT_STARTED,             ///< Started
	AUDOUT_EVT_STOPPED,             ///< Stopped
	AUDOUT_EVT_PAUSED,              ///< Paused
	AUDOUT_EVT_RESUMED,             ///< Resumed
	AUDOUT_EVT_BUF_FULL,            ///< Buffer full
	AUDOUT_EVT_BUF_EMPTY,           ///< Buffer empty
	AUDOUT_EVT_BUF_DONE,            ///< Buffer done
	AUDOUT_EVT_PRELOAD_DONE,        ///< Preload done
	AUDOUT_EVT_MAX
} AUDOUT_EVT;

typedef struct {
	PWAVSTUD_INFO_SET pAudInfoSet;
	BOOL              bRelease;
} AUDOUT_MAX_MEM_INFO, *PAUDOUT_MAX_MEM_INFO;

typedef struct _AUDOUT_CONTEXT_DEV {
	WAVSTUD_APPOBJ wso;
	WAVSTUD_INFO_SET wsis;
	WAVSTUD_INFO_SET wsis_max;

	ISF_DATA g_isf_audout_memblk;
	UINT32 g_volume;
	BOOL   g_isf_audout_allocmem;
	BOOL   g_isf_audout_lpf_en;

	AUDIO_CH isf_audout_mono_ch;
	UINT32 g_isf_audout_max_frame_sample;
	UINT32 g_isf_audout_buf_cnt;

	AUDIO_SR   isf_audout_resample_max;
	AUDIO_SR   isf_audout_resample;
	AUDIO_SR   isf_audout_resample_update;
	BOOL       isf_audout_resample_dirty;
	UINT32     isf_audout_resample_in_frame;
	UINT32     isf_audout_resample_out_frame;
	BOOL       isf_audout_resample_en;
	UINT32     isf_audout_resample_last_remain_in;
	UINT32     isf_audout_resample_last_remain_out;
	int        resample_handle_play;
	MEM_RANGE  g_isf_audout_srcmem;
	MEM_RANGE  g_isf_audout_srcbufmem;
	ISF_DATA   isf_audout_srcmemblk;
	ISF_DATA   isf_audout_srcbufmemblk;
	UINT32     isf_audout_started;
	UINT32     isf_audout_opened;
	SEM_HANDLE ISF_AUDOUT_BUF_SEM_ID;
	SEM_HANDLE ISF_AUDOUT_SIZE_SEM_ID;
	BOOL       wait_push;
	BOOL       preload_done;
	UINT32     played_size;
	UINT32     isf_audout_tdm_ch;
} AUDOUT_CONTEXT_DEV;

typedef struct _AUDOUT_CTX_MEM {
	ISF_DATA        ctx_memblk;
	UINT32          ctx_addr;
	UINT32          isf_audout_size;
	UINT32          wavstudio_size;
} AUDOUT_CTX_MEM;

typedef struct _AUDOUT_CONTEXT_COMMON {
	// CONTEXT
	AUDOUT_CTX_MEM         ctx_mem;   // alloc context memory for isf_audenc + nmedia
} AUDOUT_CONTEXT_COMMON;

typedef struct _AUDOUT_CONTEXT {
	AUDOUT_CONTEXT_COMMON comm;
	AUDOUT_CONTEXT_DEV *dev;
} AUDOUT_CONTEXT;

extern void isf_audout_install_id(void);
extern void isf_audout_uninstall_id(void);
extern SEM_HANDLE ISF_AUDOUT_PROC_SEM_ID;

#define DEV_NUM     2
#define DEV_ID_0    0
#define DEV_ID_1    1

extern UINT32 g_audout_max_count;
#define DEV_MAX_COUNT	g_audout_max_count
extern ISF_UNIT *g_audout_list[DEV_NUM]; //list of all videoproc device
#define DEV_UNIT(did)	g_audout_list[(did)]

extern ISF_RV _isf_audout_bindinput(struct _ISF_UNIT *p_thisunit, UINT32 iport, struct _ISF_UNIT *p_srcunit, UINT32 oport);
extern ISF_RV _isf_audout_setportparam(UINT32 id, ISF_UNIT  *p_thisunit, UINT32 nport, UINT32 param, UINT32 value);
extern UINT32 _isf_audout_getportparam(UINT32 id, ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param);
extern ISF_RV _isf_audout_setportstruct(UINT32 id, struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32* p_struct, UINT32 size);
extern ISF_RV _isf_audout_getportstruct(UINT32 id, struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32* p_struct, UINT32 size);
extern ISF_RV isf_audout_inputport_push_imgbuf_to_dest(UINT32 id,ISF_PORT *p_port, ISF_DATA *p_data, INT32 wait_ms);
extern ISF_RV _isf_audout_updateport(UINT32 id,ISF_UNIT  *p_thisunit, UINT32 oport, ISF_PORT_CMD cmd);
extern ISF_RV _isf_audout_do_command(UINT32 cmd, UINT32 p0, UINT32 p1, UINT32 p2);

//debug
extern void isf_audout_dump_status(int (*dump)(const char *fmt, ...), ISF_UNIT *p_thisunit);

#endif

