

#ifndef _WAVSTUDIO_AUDINTF_H
#define _WAVSTUDIO_AUDINTF_H

#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kflow_audiocapture/wavstudio_tsk.h"
#include "wavstudio_int.h"

ER WavStud_AudDrvInit(WAVSTUD_ACT actType, PWAVSTUD_AUD_INFO audInfo, UINT32 audVol, UINT32 fpEventCB);
ER WavStud_AudLBDrvInit(AUDIO_SR audSR, AUDIO_CH audCh, UINT32 fpEventCB);
void WavStud_AudDrvStop(WAVSTUD_ACT actType);
void WavStud_AudDrvPause(WAVSTUD_ACT actType);
void WavStud_AudDrvResume(WAVSTUD_ACT actType);
BOOL WavStud_AudDrvIsBuffQueFull(WAVSTUD_ACT actType, AUDIO_CH audCh);
BOOL WavStud_AudDrvAddBuffQue(WAVSTUD_ACT actType, AUDIO_CH audCh, PAUDIO_BUF_QUEUE pAudBufQue, BOOL trigger);
PAUDIO_BUF_QUEUE WavStud_AudDrvGetDoneBufQue(WAVSTUD_ACT actType, AUDIO_CH audCh);
PAUDIO_BUF_QUEUE WavStud_AudDrvGetCurrBufQue(WAVSTUD_ACT actType, AUDIO_CH audCh);
UINT32 WavStud_AudDrvGetCurrDMA(WAVSTUD_ACT actType, AUDIO_CH audCh);
//__inline UINT32 WavStud_AudDrvGetTCValue(WAVSTUD_ACT actType);
void WavStud_AudDrvSetTCTrigger(WAVSTUD_ACT actType, UINT32 tcTrigVal);
ER WavStud_AudDrvPlay(WAVSTUD_ACT act_type);
ER WavStud_AudDrvRec(WAVSTUD_ACT act_type);
void WavStud_AudDrvLBRec(void);
UINT32 WavStud_GetAudObj(WAVSTUD_ACT actType);
ER WavStud_AudDrvSetVol(WAVSTUD_ACT act_type, WAVSTUD_PORT port, UINT32 vol);

#endif

