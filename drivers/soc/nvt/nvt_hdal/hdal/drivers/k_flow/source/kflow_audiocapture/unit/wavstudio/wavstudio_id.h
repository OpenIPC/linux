#ifndef _WAVSTUDIOID_H
#define _WAVSTUDIOID_H

#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/semaphore.h"
#include "wavstudio_int.h"

#define PRI_WAVSTUD_PLAYTSK              3
#define STKSIZE_WAVSTUD_PLAYTSK          2048  //4096

#if WAVSTUD_FIX_BUF == ENABLE
#define PRI_WAVSTUD_RECORDTSK            1
#else
#define PRI_WAVSTUD_RECORDTSK            3
#endif
#define STKSIZE_WAVSTUD_RECORDTSK        2048  //4096

#define PRI_WAVSTUD_RECORDWRITETSK       8
#define STKSIZE_WAVSTUD_RECORDWRITETSK   2048  //4096

#define PRI_WAVSTUD_RECORDUPDATETSK       8
#define STKSIZE_WAVSTUD_RECORDUPDATETSK   2048  //4096


#if defined __UITRON || defined __ECOS
extern THREAD_HANDLE _SECTION(".kercfg_data") WAVSTUD_PLAYTSK_ID; ///< task id
extern THREAD_HANDLE _SECTION(".kercfg_data") WAVSTUD_RECORDTSK_ID; ///< task id
extern UINT32 _SECTION(".kercfg_data") FLG_ID_WAVSTUD_PLAY; ///< flag id
extern UINT32 _SECTION(".kercfg_data") FLG_ID_WAVSTUD_RECORD; ///< flag id
extern SEM_HANDLE _SECTION(".kercfg_data") SEM_ID_WAVSTUD_PLAY_QUE; ///< semaphore id
extern THREAD_HANDLE _SECTION(".kercfg_data") WAVSTUD_RECORDWRITETSK_ID; ///< task id
extern UINT32 _SECTION(".kercfg_data") FLG_ID_WAVSTUD_RECORDWRITE; ///< flag id
extern SEM_HANDLE _SECTION(".kercfg_data") SEM_ID_WAVSTUD_RECORD_QUE; ///< semaphore id
extern THREAD_HANDLE _SECTION(".kercfg_data") WAVSTUD_RECORDUPDATETSK_ID; ///< task id
extern UINT32 _SECTION(".kercfg_data") FLG_ID_WAVSTUD_RECORDUPDATE;
extern SEM_HANDLE _SECTION(".kercfg_data") SEM_ID_WAVSTUD_LOCK; ///< semaphore id
#else
extern THREAD_HANDLE WAVSTUD_PLAYTSK_ID; ///< task id
extern THREAD_HANDLE WAVSTUD_RECORDTSK_ID; ///< task id
extern ID FLG_ID_WAVSTUD_PLAY; ///< flag id
extern ID FLG_ID_WAVSTUD_RECORD; ///< flag id
extern SEM_HANDLE SEM_ID_WAVSTUD_PLAY_QUE; ///< semaphore id
extern THREAD_HANDLE WAVSTUD_RECORDWRITETSK_ID; ///< task id
extern ID FLG_ID_WAVSTUD_RECORDWRITE; ///< flag id
extern SEM_HANDLE SEM_ID_WAVSTUD_RECORD_QUE; ///< semaphore id
extern THREAD_HANDLE WAVSTUD_RECORDUPDATETSK_ID; ///< task id
extern ID FLG_ID_WAVSTUD_RECORDUPDATE;
extern SEM_HANDLE SEM_ID_WAVSTUD_LOCK; ///< semaphore id

extern THREAD_HANDLE WAVSTUD_PLAYTSK2_ID; ///< task id
extern ID FLG_ID_WAVSTUD_PLAY2; ///< flag id
extern SEM_HANDLE SEM_ID_WAVSTUD_PLAY2_QUE; ///< semaphore id
#endif

#endif //_WAVSTUDIOID_H
