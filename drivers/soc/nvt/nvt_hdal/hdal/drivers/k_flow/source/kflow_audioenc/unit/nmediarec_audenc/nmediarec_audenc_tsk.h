/*
    @file       nmediarec_audenc_tsk.h

    @brief      header file of media audio recorder
    @note
    @date       2018/09/05

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _NMEDIARECAUDENCTSK_H
#define _NMEDIARECAUDENCTSK_H

#ifdef __KERNEL__
#include "kwrap/type.h"
#include "nmediarec_audenc.h"
#include "audio_encode.h"
#else
#include "kwrap/type.h"
#include "nmediarec_audenc.h"
#include "audio_encode.h"
#define module_param_named(a, b, c, d)
#define MODULE_PARM_DESC(a, b)
#define msecs_to_jiffies(x)	0
#endif

#define NMR_AUDENC_RAW_QUEUE_SEC            1
#define NMR_AUDENC_ROLLBACK_SEC             6
#define NMR_AUDENC_MAX_SECENTRY             47
#define NMR_AUDENC_RAWQ_MAX                 (NMR_AUDENC_RAW_QUEUE_SEC*NMR_AUDENC_MAX_SECENTRY)
#define NMR_AUDENC_JOBQ_MAX                 160
#define NMR_AUDENC_BSQ_MAX                  (NMR_AUDENC_ROLLBACK_SEC*NMR_AUDENC_MAX_SECENTRY)
#define NMR_AUDENC_RESERVED_MIN_MS          (2 * 1000)
#define NMR_AUDENC_RESERVED_MAX_MS          (10 * 1000)

#define NMR_AUDENC_DYNAMIC_CONTEXT          1    // 1: alloc context dynamically, but debug command could only be used when running
                                                 // 0: static context, debug command could be used anytime

#define NMR_AUDENC_MSG_ENCTIME     			((UINT32)0x00000001 << 0)   //0x00000001
#define NMR_AUDENC_MSG_RING_BUF				((UINT32)0x00000001 << 6)   //0x00000040

typedef struct {
	UINT32                          uiRawBlocksize;     //in: raw blocksize, bytes for one block (1024*chs*2bytes)
	UINT32                          uiChunkSize;        //in: uiBytesPerSec 4-byte alignment
	UINT32                          uiAudBSBufMs;       //in: audio bit-stream buffer length in millisecond
	UINT32                          uiBSStart;          //in: start address of audio BS data
	UINT32                      	uiBSEnd;            //in: end address of audio BS data
	UINT32                          uiBSMax;            //in: max address of audio BS data
	UINT32                          uiPrevBSAddr;       //out: previous AAC Bit-stream address
	UINT32                          uiLastLeftAddr;     //out: last g.711 addr (for g.711 fix sample)
	UINT32                          uiLastLeftSize;     //out: left g.711 data (for g.711 fix sample)
} NMR_AUDENC_MEMINFO;

typedef struct {
	UINT32                          Front;              ///< Front pointer
	UINT32                          Rear;               ///< Rear pointer
	UINT32                          bFull;              ///< Full flag
	NMI_AUDENC_RAW_INFO             Queue[NMR_AUDENC_RAWQ_MAX];
} NMR_AUDENC_AUDQ, *PNMR_AUDENC_AUDQ;

typedef struct {
	UINT32                          Front;                  ///< Front pointer
	UINT32                          Rear;                   ///< Rear pointer
	UINT32                          bFull;                  ///< Full flag
	UINT32                          Queue[NMR_AUDENC_JOBQ_MAX];
} NMR_AUDENC_JOBQ, *PNMR_AUDENC_JOBQ;

typedef struct {
	UINT32                          Front;              ///< Front pointer
	UINT32                          Rear;               ///< Rear pointer
	UINT32                          bFull;              ///< Full flag
	NMI_AUDENC_MEM_RANGE            Queue[NMR_AUDENC_BSQ_MAX];
} NMR_AUDENC_BSQ, *PNMR_AUDENC_BSQ;

typedef struct {
	NMR_AUDENC_MEMINFO              memInfo;            //in: memory info
	NMI_AUDENC_CB                   *CallBackFunc;      //in: event inform callback
	MP_AUDENC_ENCODER               *pEncoder;			//in: encoder obj
	MP_AUDENC_ENCODER               Encoder;            //in: enc ctrl
	MP_AUDENC_PARAM          		*pEncParam;         //in: enc param
	NMR_AUDENC_AUDQ                 rawQueue;           //in: raw queue
	NMR_AUDENC_BSQ                  bsQueue;            //in: bs queue
	UINT32                          filetype;           //in: file format: MEDIAREC_MP4, MEDIAREC_TS
	UINT32                          uiCodec;         	//in: audio codec type, MEDIAREC_ENC_PCM
	UINT32                          uiRecFormat;        //in: MEDIAREC_TIMELAPSE, MEDIAREC_LIVEVIEW ...
	UINT32                          uiSampleRate;       //in: audio sample rate
	UINT32                          uiBits;          	//in: bits per sample, 8 or 16
	UINT32                          uiChannels;      	//in: channels, 1 or 2
	UINT32                          uiMaxFrameQueue;	//in: max frame queue count in ring buf
	UINT32                          uiFixSamples;	    //in: fixed enc samples
	UINT32							uiMsgLevel;			//in: msg level
	UINT32                          uStopFreq;      	//out: audio stop frequency (for AAC)
	UINT32                          uiBitRate;       	//out: bitRate after encoding (bits per second)
	UINT32                          uiADTSHeader;    	//out: audio adts header (for AAC)
	BOOL                            bStart;             //Start to enc
	UINT32                          uiBsReservedSize;   //in: reserved bs header size for each frame
	UINT32                          uiAacVer;           //in: aac mpeg version
} NMR_AUDENC_OBJ;

extern void 	     NMR_AudEnc_ShowMsg(UINT32 pathID, UINT32 msgLevel);

extern void          nmr_audenc_install_id(void) _SECTION(".kercfg_text");
extern void          nmr_audenc_install_id(void);
extern THREAD_HANDLE _SECTION(".kercfg_data") NMR_AUDENC_TSK_ID;
extern ID            _SECTION(".kercfg_data") FLG_ID_NMR_AUDENC;
extern SEM_HANDLE    _SECTION(".kercfg_data") NMR_AUDENC_SEM_ID[];

#endif //_NMEDIARECAUDENCTSK_H
