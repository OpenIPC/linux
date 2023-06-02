/*
    Audio Echo Cancellation library internal header file

    This file is the global header of Audio Echo Cancellation library.

    @file       Aec_int.h
    @ingroup    mIAudEC
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2015.  All rights reserved.
*/
#ifndef   _AEC_INT_H
#define   _AEC_INT_H
#ifdef __KERNEL__
#include   <mach/fmem.h>
#define dma_getNonCacheAddr(parm) parm
#define dma_getPhyAddr(parm) frm_va2pa(parm)
#define dma_flushWriteCache(parm,parm2)
#define dma_flushReadCache(parm,parm2)
#endif

#define AEC_MSW_VERSION_CODE    0x0502

#define AEC_ALIGN_BUF_EN		DISABLE
#define AEC_BUF_FIXED_MAP		DISABLE

#define AEC_INTERNAL_BUFSIZE    409600 // 400KB for 48KHz Stereo
#define AEC_ALIGN_BUFSIZE         4096 // 1024 samples for 48KHz Stereo
#define AEC_AMPLIFIED_RATIO          2 // Use 1 or 2 only.
#define AEC_PRELOAD_FORE_BUFSIZE  8192 // 48KHz Stereo case.sizeof(short)
#define AEC_PRELOAD_BACK_BUFSIZE 16384 // 48KHz Stereo case.sizeof(int)

typedef struct
{
    // Info Config by User
    INT32   iSampleRate;
    INT32   iMicCH,iSpkrCH,iSpkrNo;
    INT32   iNRLvl,iECLvl;
    INT16   iNREn,iNLPEn,iPreLoadEn;
    UINT32  uiAlignRx,uiAlignCntRx;
    UINT32  uiAlignTx,uiAlignCntTx;
    UINT32  uiForeAddr,uiBackAddr;
    UINT32  uiForeSize,uiBackSize;


    // Info for Lib internal
    INT32   iFrameSize;
    INT32   iRxBufSize,iPlayBufSize,iOutBufSize;
    BOOL    bAutoCacheable;
    BOOL    DbgPrint;


}AEC_INFO;


typedef struct
{
    // Info Config by User
    INT32   iSampleRate;
    INT32   iCHANNEL;
    INT32   iNRLvl;

    // Info for Lib internal
    INT32   iFrameSize;
    INT32   iBufUnitSize;
    BOOL    bAutoCacheable;
    BOOL    DbgPrint;


}AUDNS_INFO;


#endif
