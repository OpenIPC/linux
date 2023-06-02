/*
    Audio Echo Cancellation library internal header file

    This file is the global header of Audio Echo Cancellation library.

    @file       Aec_int.h
    @ingroup    mIAudEC
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2015.  All rights reserved.
*/
#ifndef   _AGC_INT_H
#define   _AGC_INT_H

#include "kwrap/type.h"


#define AGC_ABS(x)	(((x)>0)?(x):-(x))



#define AGC_TARGET_TOLERANCE		350


typedef struct {
	BOOL	bStereo;
	INT32   iSampleRate;
	INT16   iMaxGain,iMinGain;
	INT32   iTargetLvl,iNgTHD;


	UINT8	pk_up_thd,pk_up_curL,pk_up_curR,pk_idx;
	UINT16  peakL,peakL_next,peakL_update;
	UINT16  peakR,peakR_next,peakR_update;

	INT32	iPeakLDB,iPeakRDB;
	INT32   iCurGainL,iCurGainR;//0.5dB per step
	INT32   iActiveGainL,iActiveGainR;
	INT32   AccSumL,AccSumR;


} AGC_INFO;





#endif
