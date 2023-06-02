/**
    Internal header file of NvtAec

    Internal header file of NvtAec.

    @file       NvtAec_int.h
    @ingroup    mIAPPNVTAEC
    @version    V1.00.000
    @date       2016/02/26

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/

#ifndef _NVTAEC_H
#define _NVTAEC_H

#if defined(__FREERTOS) || defined __UITRON || defined __ECOS
#include <string.h>
#include "kwrap/error_no.h"
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "../include/nvtaec_api.h"
#else
#include "kwrap/error_no.h"
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "../include/nvtaec_api.h"
#endif

#define FILEWRITE           DISABLE//ENABLE

/**
    @addtogroup mIAPPNVTAEC
*/
//@{

/**
    @name NvtAec SC Queue Maximum

    NvtAec Sample Count Queue Maximum
*/
//@{
#define NVTAEC_SCQUE_MAX       128
//@}

//Max sample count for played data buffer
#define NVTAEC_MAX_SAMPLE_COUNT (20480)
#define AEC_PRELOAD_FORE_BUFSIZE  (8192) // 48KHz Stereo case.sizeof(short)
#define AEC_PRELOAD_BACK_BUFSIZE (16384) // 48KHz Stereo case.sizeof(int)

typedef struct _NVTAEC_AUDIO_SC_QUE {
	UINT32 uiSampleCount[NVTAEC_SCQUE_MAX];
	UINT32 uiHeadIdx;
	UINT32 uiTailIdx;
} NVTAEC_AUDIO_SC_QUE;

typedef struct {
	// Info Config by User
	INT32   iECLvl;
	INT32   iNRLvl;
	BOOL    bLeakEstiEn;
	INT32   iLeakEstiVal;
	INT32   iMicCH;
	INT32   iSpkCH;
	INT32   iSampleRate;
	INT32   iFilterLen;
	INT32   iFrameSize;
	INT32   iNotchRadius;
	INT16   iPreLoadEn;
	UINT32  uiForeAddr;
	UINT32  uiBackAddr;
	UINT32  uiForeSize;
	UINT32  uiBackSize;
	BOOL    uiARModuleEn;
	UINT32  uiInternalAddr;
	UINT32  uiInternalSize;
	INT32   iSpkNum;
} NVTAEC_INFO;

void xNvtAec_InstallCmd(void);
//@}


#endif
