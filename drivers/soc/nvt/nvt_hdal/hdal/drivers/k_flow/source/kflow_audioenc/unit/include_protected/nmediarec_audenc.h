#ifndef _NMEDIARECAUDENC_H
#define _NMEDIARECAUDENC_H

#include "nvt_media_interface.h"

typedef enum {
	NMI_AUDENC_PARAM_ALLOC_SIZE,
	NMI_AUDENC_PARAM_ENCODER_OBJ,
	NMI_AUDENC_PARAM_MEM_RANGE,
	NMI_AUDENC_PARAM_MAX_MEM_INFO,
	NMI_AUDENC_REG_CB,
	NMI_AUDENC_PARAM_FILETYPE,
	NMI_AUDENC_PARAM_RECFORMAT,
	NMI_AUDENC_PARAM_CODEC,
	NMI_AUDENC_PARAM_CHS,
	NMI_AUDENC_PARAM_SAMPLERATE,
	NMI_AUDENC_PARAM_BITS,
	NMI_AUDENC_PARAM_UNLOCK_BS_ADDR,
	NMI_AUDENC_PARAM_AAC_ADTS_HEADER,
	NMI_AUDENC_PARAM_FIXED_SAMPLE,
	NMI_AUDENC_PARAM_MAX_FRAME_QUEUE,
	NMI_AUDENC_PARAM_DROP_FRAME,
	NMI_AUDENC_PARAM_ENCBUF_MS,
	NMI_AUDENC_PARAM_BS_RESERVED_SIZE,
	NMI_AUDENC_PARAM_AAC_VER,
} NMI_AUDENC_AUDENC_PARAM;

typedef enum {
	NMI_AUDENC_ACTION_START,
	NMI_AUDENC_ACTION_STOP,
	NMI_AUDENC_ACTION_MAX,
} NMI_AUDENC_ACTION;

typedef enum {
	NMI_AUDENC_EVENT_BS_CB,
	NMI_AUDENC_EVENT_MAX,
	ENUM_DUMMY4WORD(NMI_AUDENC_CB_EVENT)
} NMI_AUDENC_CB_EVENT;

typedef struct {
	UINT32 Addr;            ///< Memory buffer starting address
	UINT32 Size;            ///< Memory buffer size
	UINT32 BufID;         	///< Buffer ID
	UINT64 TimeStamp;       ///< TimeStamp
} NMI_AUDENC_RAW_INFO, *PNMI_AUDENC_RAW_INFO;

typedef struct {
	UINT32 Addr;            ///< Memory buffer starting address
	UINT32 Size;            ///< Memory buffer size
	UINT32 Rel;             ///< Not released in sequence
} NMI_AUDENC_MEM_RANGE, *PNMI_AUDENC_MEM_RANGE;

typedef struct {
	UINT32 PathID;          ///< Path ID
	UINT32 BufID;           ///< Memory block ID
	UINT32 Addr;            ///< Memory buffer starting address
	UINT32 Size;            ///< Memory buffer size
	UINT32 RawAddr;         ///< Raw PCM address
	UINT32 Occupied;        ///< Has remaining raw PCM need to be encoded
	UINT64 TimeStamp;       ///< Timestamp
} NMI_AUDENC_BS_INFO, *PNMI_AUDENC_BS_INFO;

typedef struct {
	UINT32 uiAudCodec;      //in: audio codec type, MEDIAREC_ENC_PCM
	UINT32 uiRecFormat;     //in: MEDIAREC_TIMELAPSE, MEDIAREC_LIVEVIEW ...
	UINT32 uiAudChannels;   //in: channels, 1 or 2
	UINT32 uiAudioSR;       //in: audio sample rate
	UINT32 uiAudBits;       //in: bits per sample, 8 or 16
	BOOL   bRelease;
	UINT32 uiEncsize;		//out: enc buf size
} NMI_AUDENC_MAX_MEM_INFO, *PNMI_AUDENC_MAX_MEM_INFO;

typedef void (NMI_AUDENC_CB)(UINT32 event, UINT32 param);

extern void NMR_AudEnc_AddUnit(void);

extern UINT32 g_audenc_path_max_count;
#define PATH_MAX_COUNT g_audenc_path_max_count

#endif //_NMEDIARECAUDENC_H
