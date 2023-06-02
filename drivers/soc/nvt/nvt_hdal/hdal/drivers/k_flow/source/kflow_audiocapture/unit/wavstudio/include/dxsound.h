#ifndef _DXSOUND_H_
#define _DXSOUND_H_

#if defined(__UITRON)
#include "Type.h"
#include "Audio.h"
#else
#include "audio_common/include/Audio.h"
#include "kwrap/type.h"
#include "kwrap/platform.h"
#endif

#define DXSOUND_VER                 0x00010000
#define DX_OK                       0x0000 ///< Success

typedef enum _DXSOUND_CAP_ {
	DXSOUND_CAP_SET_DEVINITDATA,
	DXSOUND_CAP_SET_CODEC,
	DXSOUND_CAP_SET_TOTAL_VOLUME_LEVEL,
	DXSOUND_CAP_VOLUME,
	DXSOUND_CAP_SAMPLERATE,
	DXSOUND_CAP_CHANNEL,
	DXSOUND_CAP_GAIN,
	DXSOUND_CAP_RESAMPLE,
	DXSOUND_CAP_GET_HDMI_SAMPLERATE,
	DXSOUND_CAP_GET_AUDOUTDEV,
	DXSOUND_CAP_SET_OUTDEV,
	DXSOUND_CAP_MAX
} DXSOUND_CAP;

typedef struct _DXSOUND_AUD_CH_ {
	AUDIO_CH audch;
	UINT32 monoExpand;
} DXSOUND_AUD_CH, *PDXSOUND_AUD_CH;

typedef struct _DXSOUND_AUD_RESAMPLE_INFO_ {
	UINT32 audSR;
	UINT32 uiWorkBufAddr;
	UINT32 uiWorkBufSize;
} DXSOUND_AUD_RESAMPLE_INFO, *PDXSOUND_AUD_RESAMPLE_INFO;

typedef enum {
	DRVSOUND_VOUT_LCD,
	DRVSOUND_VOUT_TV,
	DRVSOUND_VOUT_HDMI,
	DRVSOUND_VOUT_LCDTV,
	DRVSOUND_VOUT_LCDHDMI,
	ENUM_DUMMY4WORD(DRVSOUND_VOUT)
} DRVSOUND_VOUT;

///////////////////////////////////////////////////////////////////////////////
// Audio
///////////////////////////////////////////////////////////////////////////////
extern AUDIO_OUTPUT GPIOMap_DetAudioOutDevice(void);
extern void GPIOMap_TurnOnExtrAmp(BOOL bEn);
extern void DrvSound_SetAudioDevInitData(AUDIO_DEVICE_OBJ  *pAudioDevice, AUDIO_SETTING *pAudioSetting);
extern void DrvSound_SetAudioFeature(void);
extern UINT32 DrvSound_Init(void *pInitParam);
extern ER DrvSound_SetCodec(PAUDIO_CODEC_FUNC codecFunc, AUDIO_CODECSEL audCodecSel);


extern ER   DrvSound_SetOutDevConfig(UINT32 outDevIdx, PAUDTS_OBJ pAudPlayObj);
extern ER   DrvSound_SetVol(UINT32 vol);// 0 ~ 100
extern ER   DrvSound_SetSR(UINT32 audSR, PAUDTS_OBJ pAudPlayObj);
//extern ER   DrvSound_SetCh(AUDIO_CH audCh, UINT32 monoExpand, PAUDTS_OBJ pAudPlayObj);
ER DrvSound_SetCh(PDXSOUND_AUD_CH pDxSndAudCh, PAUDTS_OBJ pAudObj);
extern UINT32 DrvSound_GetVol(void);
extern UINT32 DrvSound_GetHDMIAudSR(void);
extern ER   DrvSound_SetAudGain(UINT32 vol);
extern ER DrvSound_SetSR_Resample(PDXSOUND_AUD_RESAMPLE_INFO pAudResampleInfo, PAUDTS_OBJ pAudObj);
extern void DrvSound_SetHDMIConfigInit(void);


#endif //_DXSOUND_H_

