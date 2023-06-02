/*
    @file       WavStudioInt.h
    @ingroup    mILIBWAVSTUDIO

    @brief      Internel definition of WavStudio task.

                Internel definition of WavStudio task.

    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.
*/

/**
    @addtogroup mILIBWAVSTUDIO
*/
//@{

#ifndef _WAVSTUDIOINT_H
#define _WAVSTUDIOINT_H

#if defined(__KERNEL__)
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include "kwrap/stdio.h"
#include "kwrap/sxcmd.h"
#include <kwrap/spinlock.h>
#include "kflow_audiocapture/wavstudio_tsk.h"
#include "comm/hwclock.h"
#include "kdrv_builtin/kdrv_builtin.h"
#include "kdrv_builtin/audcap_builtin.h"
#include <linux/string.h>

#define kent_tsk()
#define Perf_GetCurrent(x) 0
#else
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include "kwrap/stdio.h"
#include "kwrap/sxcmd.h"
#include <kwrap/spinlock.h>
#include "kflow_audiocapture/wavstudio_tsk.h"
#include "comm/hwclock.h"
#include "kdrv_builtin/kdrv_builtin.h"
#include "kdrv_builtin/audcap_builtin.h"
#include <string.h>

#define kent_tsk()
#endif

///////////////////////////////////////////////////////////////////////////////
#define THIS_DBGLVL         NVT_DBG_WRN
#define __MODULE__          wavstudio
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass
#include "kwrap/debug.h"

extern unsigned int wavstudio_debug_level;
///////////////////////////////////////////////////////////////////////////////

#define _WAVSTUD_SAME_MODE_FMT_     ENABLE
#define _WAVSTUD_AIR_MODE_          DISABLE

/**
    @name Wave studio callback
*/
//@{
#define WAVSTUD_PLAY_CB(p1, p2)   {if (gWavStudObj.pWavStudCB[WAVSTUD_ACT_PLAY]){gWavStudObj.pWavStudCB[WAVSTUD_ACT_PLAY](WAVSTUD_ACT_PLAY, p1, p2);}}
#define WAVSTUD_REC_CB(p1, p2)    {if (gWavStudObj.pWavStudCB[WAVSTUD_ACT_REC]){gWavStudObj.pWavStudCB[WAVSTUD_ACT_REC](WAVSTUD_ACT_REC, p1, p2);}}
#define WAVSTUD_REC2_CB(p1, p2)   {if (gWavStudObj.pWavStudCB[WAVSTUD_ACT_REC]){gWavStudObj.pWavStudCB[WAVSTUD_ACT_REC](WAVSTUD_ACT_REC2, p1, p2);}}
#define WAVSTUD_PLAY2_CB(p1, p2)   {if (gWavStudObj.pWavStudCB[WAVSTUD_ACT_PLAY2]){gWavStudObj.pWavStudCB[WAVSTUD_ACT_PLAY2](WAVSTUD_ACT_PLAY2, p1, p2);}}
//@}

/**
    @name Wave studio command flag pattern
*/
//@{
#define FLG_ID_WAVSTUD_CMD_START        0x00000001
#define FLG_ID_WAVSTUD_CMD_STOP         0x00000002
#define FLG_ID_WAVSTUD_CMD_PAUSE        0x00000004
#define FLG_ID_WAVSTUD_CMD_RESUME       0x00000008
#define FLG_ID_WAVSTUD_CMD_RESTART      0x00000010
//@}

/**
    @name Wave studio event flag pattern
*/
//@{
#define FLG_ID_WAVSTUD_EVT_TCHIT        0x00000100
#define FLG_ID_WAVSTUD_EVT_BUF_READY    0x00000200
#define FLG_ID_WAVSTUD_EVT_DISK_FULL    0x00000400
#define FLG_ID_WAVSTUD_EVT_BUF_FULL     0x00000800
#define FLG_ID_WAVSTUD_EVT_BUF_EMPTY    0x00001000
#define FLG_ID_WAVSTUD_EVT_RWERR        0x00002000
#define FLG_ID_WAVSTUD_EVT_BUF_READY2   0x00004000
#define FLG_ID_WAVSTUD_EVT_BUF_FULL2    0x00008000
#define FLG_ID_WAVSTUD_EVT_BUF_EMPTY2   0x00010000
#define FLG_ID_WAVSTUD_EVT_RWERR2       0x00020000
#define FLG_ID_WAVSTUD_EVT_RW           0x00040000
#define FLG_ID_WAVSTUD_EVT_RW2          0x00080000
#define FLG_ID_WAVSTUD_EVT_RWIDLE       0x00100000
#define FLG_ID_WAVSTUD_EVT_CLOSE        0x00200000
#define FLG_ID_WAVSTUD_EVT_STREAMERR    0x00400000
#define FLG_ID_WAVSTUD_EVT_QUE_READY    0x00800000
#define FLG_ID_WAVSTUD_EVT_LB_BUF_READY 0x10000000
#define FLG_ID_WAVSTUD_EVT_LB_BUF_FULL  0x20000000
//@}

/**
    @name Wave studio status flag pattern
*/
//@{
#define FLG_ID_WAVSTUD_STS_GOING        0x01000000
#define FLG_ID_WAVSTUD_STS_STOPPED      0x02000000
#define FLG_ID_WAVSTUD_STS_PAUSED       0x04000000
#define FLG_ID_WAVSTUD_STS_IDLE         0x08000000
#define FLG_ID_WAVSTUD_STS_WAIT_PRELOAD 0x40000000
//@}

#define FLG_ID_WAVSTUD_CMD              (FLG_ID_WAVSTUD_CMD_START | \
		FLG_ID_WAVSTUD_CMD_STOP  | \
		FLG_ID_WAVSTUD_CMD_PAUSE | \
		FLG_ID_WAVSTUD_CMD_RESUME| \
		FLG_ID_WAVSTUD_CMD_RESTART)

#define FLG_ID_WAVSTUD_EVT              (FLG_ID_WAVSTUD_EVT_TCHIT    | \
		FLG_ID_WAVSTUD_EVT_BUF_READY|FLG_ID_WAVSTUD_EVT_BUF_READY2| \
		FLG_ID_WAVSTUD_EVT_DISK_FULL| \
		FLG_ID_WAVSTUD_EVT_BUF_FULL |FLG_ID_WAVSTUD_EVT_BUF_FULL2| \
		FLG_ID_WAVSTUD_EVT_BUF_EMPTY|FLG_ID_WAVSTUD_EVT_BUF_EMPTY2| \
		FLG_ID_WAVSTUD_EVT_RWERR|FLG_ID_WAVSTUD_EVT_RWERR2        | \
		FLG_ID_WAVSTUD_EVT_LB_BUF_READY|FLG_ID_WAVSTUD_EVT_LB_BUF_FULL | \
		FLG_ID_WAVSTUD_EVT_QUE_READY | FLG_ID_WAVSTUD_EVT_CLOSE)

#define FLG_ID_WAVSTUD_WAIT             (FLG_ID_WAVSTUD_CMD | FLG_ID_WAVSTUD_EVT)

#define FLG_ID_WAVSTUD_STS              (FLG_ID_WAVSTUD_STS_GOING   | \
		FLG_ID_WAVSTUD_STS_STOPPED | \
		FLG_ID_WAVSTUD_STS_PAUSED  | \
		FLG_ID_WAVSTUD_STS_IDLE    | \
		FLG_ID_WAVSTUD_STS_WAIT_PRELOAD)

#define FLG_ID_WAVSTUD_WRITE            (FLG_ID_WAVSTUD_EVT_RW  | \
		FLG_ID_WAVSTUD_EVT_RW2 | \
		FLG_ID_WAVSTUD_EVT_RWIDLE)



#define FLG_ID_WAVSTUD_ALL              (FLG_ID_WAVSTUD_CMD | \
		FLG_ID_WAVSTUD_EVT | \
		FLG_ID_WAVSTUD_STS | \
		FLG_ID_WAVSTUD_WRITE)

typedef enum {
	AUDIO_EVENT_TCLATCH             =   0x00000001,     ///< Time code latch event
	AUDIO_EVENT_TCHIT               =   0x00000002,     ///< Time code hit event
	AUDIO_EVENT_DMADONE             =   0x00000010,     ///< DMA finish transfering one buffer event
	AUDIO_EVENT_BUF_FULL            =   0x00000020,     ///< Buffer queue full event (in record mode)
	AUDIO_EVENT_BUF_EMPTY           =   0x00000040,     ///< Buffer queue empty event (in playback mode)
	AUDIO_EVENT_DONEBUF_FULL        =   0x00000080,     ///< Done buffer queue full event
	AUDIO_EVENT_FIFO_ERROR          =   0x00000100,     ///< FIFO Error
	AUDIO_EVENT_BUF_RTEMPTY         =   0x00000200,     ///< Buffer queue runtime empty event (in playback mode)

	AUDIO_EVENT_DMADONE2            =   0x00010000,     ///< DMA finish transfering one buffer event.
														///< Valid for Dual Mono Record mode only.
	AUDIO_EVENT_BUF_FULL2           =   0x00020000,     ///< Buffer queue full event (in record mode)
														///< Valid for Dual Mono Record mode only.
	AUDIO_EVENT_DONEBUF_FULL2       =   0x00040000,     ///< Done buffer queue full event
														///< Valid for Dual Mono Record mode only.
	AUDIO_EVENT_FIFO_ERROR2         =   0x00080000,     ///< FIFO Error. Valid for Dual Mono Record mode only.

	ENUM_DUMMY4WORD(AUDIO_EVENT)
} AUDIO_EVENT;



#define WAVSTUD_SAMPLE_RATE_MAX         AUDIO_SR_48000  ///< 48000
#define WAVSTUD_BPS_MAX                 2               ///< Bytes Per Second (16bit)
#define WAVSTUD_CHANNEL_CNT_MAX         2               ///< Stereo
#define WAVSTUD_RESRVD_SECOND           8               ///< Reserved second

#define WAVSTUD_TMP_DATA_BUF_CNT        2
#define WAVSTUD_QUEUE_VAR_SIZE          WAVSTUD_RESRVD_SECOND * sizeof(AUDIO_BUF_QUEUE) //128
#define WAVSTUD_TMP_DATA_BUF_SIZE      (WAVSTUD_SINGLE_ACT_DATA_SIZE * WAVSTUD_TMP_DATA_BUF_CNT) //192k*2=384k //ping pong
#define WAVSTUD_HEADER_BUF_SIZE         0x00000200      ///<  512 Bytes

#define WAVSTUD_SINGLE_ACT_DATA_SIZE   (WAVSTUD_SAMPLE_RATE_MAX * WAVSTUD_BPS_MAX * WAVSTUD_CHANNEL_CNT_MAX)//192k
#define WAVSTUD_SINGLE_ACT_MEMSEIZE    (WAVSTUD_HEADER_BUF_SIZE * WAVSTUD_ACT_MAX +\
										WAVSTUD_TMP_DATA_BUF_SIZE + WAVSTUD_QUEUE_VAR_SIZE +\
										WAVSTUD_SINGLE_ACT_DATA_SIZE*WAVSTUD_RESRVD_SECOND)  //0x1E0680 = 1967744 = 1921.7k

#define WAVSTUD_RECORD_SR_DEFAULT       AUDIO_SR_48000

#define WAVSTUD_PROCSIZE                1024

#define WAVSTUD_FIX_BUF                 ENABLE

#define WAVSTUD_PB_PRELOAD_ZERO         ENABLE

/**
    Wave studio modes
*/
typedef enum _WAVSTUD_MODE {
	WAVSTUD_MODE_1P,
	WAVSTUD_MODE_1R,
	WAVSTUD_MODE_2R,
	WAVSTUD_MODE_MAX
} WAVSTUD_MODE;

typedef struct _WAVSTUD_AUDQUE {
	AUDIO_BUF_QUEUE    *pAudQue;
	UINT64             timestamp;
} WAVSTUD_AUDQUE, *PWAVSTUD_AUDQUE;

typedef struct {
	UINT32                          Front;              ///< Front pointer
	UINT32                          Rear;               ///< Rear pointer
	UINT32                          bFull;              ///< Full flag
	BOOL                            *pDoneQueLock;
	PWAVSTUD_AUDQUE                 pDoneQueue;
} WAVSTUD_AUDQ, *PWAVSTUD_AUDQ;

/**
    Wave studio act object
*/
typedef struct _WAVSTUD_ACT_OBJ {
	WAVSTUD_MEM         mem;
	WAVSTUD_MEM         memTmpData[WAVSTUD_TMP_DATA_BUF_CNT]; ///< For decode, encode
	PAUDIO_BUF_QUEUE    pAudBufQueFirst;
	PAUDIO_BUF_QUEUE    pAudBufQueNext;
	PAUDIO_BUF_QUEUE    pAudBufQueTmp; //temp queue for no idle buffer
	UINT32              audBufQueCnt;
	UINT32              unitSize;
	UINT32              audVol;
	WAVSTUD_MODE        mode;
	////////////////////////////////////
	WAVSTUD_ACT         actId;
	WAVSTUD_AUD_INFO    audInfo;
	UINT32              currentCount;   ///< current sample count
	UINT64              stopCount;      ///< stop sample count
	BOOL (*fpDataCB)(PAUDIO_BUF_QUEUE p_aud_bufque, PAUDIO_BUF_QUEUE p_aud_bufque_aec, UINT32 id, UINT64 timestamp);
	PAUDIO_BUF_QUEUE    pAudBufQueHead;
	PAUDIO_BUF_QUEUE    pAudBufQueTail;
	UINT32              queEntryNum;
	UINT32              remain_entry_num;
	UINT32              currentBufRemain;
	WAVSTUD_UNIT_TIME   unitTime;
	BOOL                opened;
	WAVSTUD_DATA_MODE   dataMode;
	UINT32              procUnitSize;
	UINT32              audChs;
	UINT32              lock_addr;
	WAVSTUD_CB          pWavStudEvtCB;
	WAVSTUD_AUDQ        wavAudQue;
	UINT32              buf_sample_cnt;
	WAVSTUD_FILTCB      filt_cb;
	BOOL                wait_push;
	UINT32              done_size;
	BOOL                global_vol;
	UINT32              p0_vol;
	UINT32              p1_vol;
} WAVSTUD_ACT_OBJ, *PWAVSTUD_ACT_OBJ;


/**
    Wave studio memory allocation (full duplex)
*/
typedef struct _WAVSTUD_OBJ {
	WAVSTUD_CB           pWavStudCB[WAVSTUD_ACT_MAX]; ///< Wave studio record/playback callback
	UINT32               playOutDev;
	WAVSTUD_ACT_OBJ      actObj[WAVSTUD_ACT_MAX];
	WAVSTUD_CB           pWavStudRecvCB;
	PWAVSTUD_AUD_AEC_OBJ paecObj;
	AUDIO_CH			 lb_ch;
	WAVSTUD_AUD_CODEC    codec;
	BOOL                 aec_en;
	UINT32               default_set;
	INT32                ng_thd;
	INT32                alc_en;
	UINT32               rec_src;
} WAVSTUD_OBJ, *PWAVSTUD_OBJ;

/**
    Make buffer queue.

    Make buffer queue.

    @param[in]  pActObj      Act object.
    @return     Count of buffer.
*/
extern UINT32 WavStud_MakeAudBufQue(PWAVSTUD_ACT_OBJ pActObj);

/**
    Wav studio play task.

    Wav studio play task.
*/
extern THREAD_DECLARE(WavStudio_PlayTsk, arglist);

/**
    Wav studio record task.

    Wav studio record task.
*/
extern THREAD_DECLARE(WavStudio_RecordTsk, arglist);

/**
    Wav studio record write task.

    Wav studio record write task.
*/
extern THREAD_DECLARE(WavStudio_RecordWriteTsk, arglist);

extern THREAD_DECLARE(WavStudio_PlayTsk2, arglist);

/**
    Get buffer unit size.

    Get buffer unit size.

    @param[in]  audSR       Sample rate.
    @param[in]  audCh       Channel.
    @param[in]  bps         Bit per second.
    @return     Unit size.
*/
extern UINT32 WavStud_GetUnitSize(AUDIO_SR audSR, UINT32 ch_num, WAVSTUD_BITS_PER_SAM bps, UINT32 sample_cnt);

/**
    Dump Wav studio module info.

    Dump Wav studio module info.
*/
extern void WavStud_DmpModuleInfo(void);

extern UINT32 WavStud_BufToIdx(WAVSTUD_ACT actType, PAUDIO_BUF_QUEUE pAudCurBuf);

extern PAUDIO_BUF_QUEUE WavStud_IdxToBuf(WAVSTUD_ACT actType, UINT32 idx);

extern UINT32 WavStud_AddrToIdx(WAVSTUD_ACT actType, UINT32 addr);

extern THREAD_DECLARE(WavStudio_RecordUpdateTsk, arglist);

extern UINT64 wavstudio_do_div(UINT64 dividend, UINT64 divisor);

#if WAVSTUD_FIX_BUF == ENABLE
extern BOOL WavStud_AddDoneQue(WAVSTUD_ACT actType, PAUDIO_BUF_QUEUE pAudQue);
extern PWAVSTUD_AUDQUE WavStud_GetDoneQue(WAVSTUD_ACT actType);
extern void WavStud_InitQ(WAVSTUD_ACT actType);
extern UINT32 WavStud_HowManyinQ(WAVSTUD_ACT actType);
extern void WavStud_LockQ(WAVSTUD_ACT actType, PAUDIO_BUF_QUEUE pAudCurBuf);
extern void WavStud_UnlockQ(WAVSTUD_ACT actType, UINT32 Addr);
extern BOOL WavStud_IsQLock(WAVSTUD_ACT actType, PAUDIO_BUF_QUEUE pAudCurBuf);
extern void WavStud_InitLockQ(WAVSTUD_ACT actType);
#endif

extern INT32 wavstud_play_buf_cb(VOID *callback_info, VOID *user_data);
extern INT32 wavstud_rec_buf_cb(VOID *callback_info, VOID *user_data);
extern INT32 wavstud_lb_buf_cb(VOID *callback_info, VOID *user_data);
extern INT32 wavstud_play2_buf_cb(VOID *callback_info, VOID *user_data);
#endif

//@}
