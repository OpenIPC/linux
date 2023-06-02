/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       isf_audcap_int.h

    @brief      isf_audcap internal header file

    @version    V1.00.000
    @author     Novatek FW Team
    @date       2018/06/04
*/
#ifndef _ISF_AUDCAP_INT_H
#define _ISF_AUDCAP_INT_H

#ifdef __KERNEL__
#include "kwrap/error_no.h"
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include <kwrap/spinlock.h>
#include "kwrap/util.h"
#include "kwrap/cpu.h"
#include "kflow_common/nvtmpp.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#include "kflow_audiocapture/isf_audcap.h"
#include "kflow_audiocapture/wavstudio_tsk.h"
#include "kflow_audiocapture/ctl_aud.h"
#include "kdrv_builtin/kdrv_builtin.h"
#include "kdrv_builtin/audcap_builtin.h"
#include "../include/isf_audcap_dbg.h"
#include <linux/string.h>

#define debug_msg 			vk_printk
#else
#include "kwrap/error_no.h"
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include <kwrap/spinlock.h>
#include "kwrap/util.h"
#include "kwrap/cpu.h"
#include "kflow_common/nvtmpp.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#include "kflow_audiocapture/isf_audcap.h"
#include "kflow_audiocapture/wavstudio_tsk.h"
#include "kflow_audiocapture/ctl_aud.h"
#include "../include/isf_audcap_dbg.h"
#include <string.h>

#define debug_msg 			vk_printk
#define msecs_to_jiffies(x)	0
#endif

#define ISF_AUDCAP_PULL_Q_MAX       60
#define ISF_AUDCAP_IN_NUM           1
#define ISF_AUDCAP_OUT_NUM          2
#define ISF_AUDCAP_BSDATA_BLK_MAX   150

typedef struct _AUDCAP_PULL_QUEUE {
	UINT32          Front;                  ///< Front pointer
	UINT32          Rear;                   ///< Rear pointer
	UINT32          bFull;                  ///< Full flag
	ISF_DATA        Queue[ISF_AUDCAP_PULL_Q_MAX];
} AUDCAP_PULL_QUEUE, *PAUDCAP_PULL_QUEUE;

typedef struct _AUDCAP_MMAP_MEM_INFO {
	UINT32          addr_virtual;           ///< memory start addr (virtual)
	UINT32          addr_physical;          ///< memory start addr (physical)
	UINT32          size;                   ///< size
} AUDCAP_MMAP_MEM_INFO, *PAUDCAP_MMAP_MEM_INFO;

typedef enum {
	AUDCAP_AUD_CODEC_EMBEDDED,
	AUDCAP_AUD_CODEC_EXTERNAL,
	ENUM_DUMMY4WORD(AUDCAP_AUD_CODEC)
} AUDCAP_AUD_CODEC;

/**
    AudFilter object
*/
typedef struct _AUDCAP_AUDFILT_OBJ {
	BOOL(*open)(UINT32 , BOOL);
	BOOL(*close)(void);
	BOOL(*apply)(UINT32, UINT32, UINT32);
	void(*design)(void);
} AUDCAP_AUDFILT_OBJ, *PAUDCAP_AUDFILT_OBJ;

typedef struct {
	PWAVSTUD_INFO_SET pAudInfoSet;
	BOOL              bRelease;
} AUDCAP_MAX_MEM_INFO, *PAUDCAP_MAX_MEM_INFO;

typedef struct {
	UINT32           blk_id;
	UINT32           buf_addr;
	UINT32           occupy;
} AUDCAP_BUF_INFO;

typedef struct {
	UINT64           frame_time;
	UINT64           block_time;
} AUDCAP_TIMESTAMP;

typedef struct {
	UINT32           refCnt;
	UINT32           hData;
	MEM_RANGE        mem;
	UINT32           blk_id;
	void            *pnext_bsdata;
	AUDCAP_BUF_INFO  *p_buf_info;
	AUDCAP_BUF_INFO  *p_remain_buf_info;
} AUDCAP_BSDATA;

typedef struct _AUDCAP_CONTEXT_PORT {
	AUDCAP_BSDATA          *g_p_audcap_bsdata_link_head;
	AUDCAP_BSDATA          *g_p_audcap_bsdata_link_tail;
	BOOL                   aec_en;
	BOOL                   anr_en;
	MEM_RANGE              g_isf_audcap_pathmem;
	AUDIO_CH               isf_audout_ch;
	AUDIO_CH               isf_audout_ch_num;
	AUDIO_SR               isf_audcap_resample_max;
	AUDIO_SR               isf_audcap_resample;
	AUDIO_SR               isf_audcap_resample_update;
	BOOL                   isf_audcap_resample_dirty;
	UINT32                 isf_audcap_resample_frame;
	BOOL                   isf_audcap_resample_en;
	UINT32                 isf_audcap_started;
	UINT32                 isf_audcap_opened;
	MEM_RANGE              isf_audcap_output_mem;
	AUDCAP_AGC_CONFIG      isf_audcap_agc_cfg;
	KDRV_AUDIO_CAP_DEFSET  isf_audcap_defset;
	INT32                  isf_audcap_ng_thd;
	AUDCAP_PULL_QUEUE      isf_audcap_pull_que;
	AUDCAP_MMAP_MEM_INFO   isf_audcap_mmap_info;
	SEM_HANDLE             ISF_AUDCAP_SEM_ID;
	SEM_HANDLE             ISF_AUDCAP_PULLQ_SEM_ID;
	BOOL                   put_pullq;
	UINT32                 volume;
	BOOL                   isf_audcap_lb_en;
} AUDCAP_CONTEXT_PORT;

typedef struct _AUDCAP_CTX_MEM {
	ISF_DATA        ctx_memblk;
	UINT32          ctx_addr;
	UINT32          isf_audcap_size;
	UINT32          wavstudio_size;
} AUDCAP_CTX_MEM;

typedef struct _AUDCAP_CONTEXT_COMMON {
	// CONTEXT
	AUDCAP_CTX_MEM         ctx_mem;   // alloc context memory for isf_audenc + nmedia
} AUDCAP_CONTEXT_COMMON;

typedef struct _AUDCAP_CONTEXT {
	AUDCAP_CONTEXT_COMMON comm;
	AUDCAP_CONTEXT_PORT *port;
} AUDCAP_CONTEXT;

extern void isf_audcap_install_id(void);
extern void isf_audcap_uninstall_id(void);
//extern SEM_HANDLE ISF_AUDCAP_SEM_ID[ISF_AUDCAP_OUT_NUM];
//extern SEM_HANDLE ISF_AUDCAP_PULLQ_SEM_ID[ISF_AUDCAP_OUT_NUM];
extern SEM_HANDLE ISF_AUDCAP_PROC_SEM_ID;

extern UINT32 g_audcap_max_count;
#define PATH_MAX_COUNT  g_audcap_max_count

//debug
extern void isf_audcap_dump_status(int (*dump)(const char *fmt, ...), ISF_UNIT *p_thisunit);
extern void isf_audcap_get_audinfo(PWAVSTUD_INFO_SET info);
extern UINT32 isf_audcap_get_vol(UINT32 path);
extern UINT32 isf_audcap_get_recsrc(void);

#endif

