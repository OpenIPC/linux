/*
    @file       nmediaplay_auddec_tsk.h

    @brief      header file of media audio decoder
    @note
    @date       2018/09/25

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _NMEDIAAUDDECTSK_H
#define _NMEDIAAUDDECTSK_H

#ifdef __KERNEL__
#include "kwrap/type.h"
#include "nmediaplay_auddec.h"
#include "audio_decode.h"
#else
#include "kwrap/type.h"
#include "nmediaplay_auddec.h"
#include "audio_decode.h"
#define msecs_to_jiffies(x)	0
#endif

#define NMP_AUDDEC_BS_QUEUE_SEC             6
#define NMP_AUDDEC_RAW_QUEUE_SEC            1
#define NMP_AUDDEC_MAX_SECENTRY             47
#define NMP_AUDDEC_JOBQ_MAX                 160
#define NMP_AUDDEC_BSQ_MAX                  (NMP_AUDDEC_BS_QUEUE_SEC*NMP_AUDDEC_MAX_SECENTRY)
#define NMP_AUDDEC_RAWQ_MAX                 (NMP_AUDDEC_RAW_QUEUE_SEC*NMP_AUDDEC_MAX_SECENTRY)

/**
    NMP Audio Decoder Status
*/
//@{
#define  NMP_AUDDEC_STATUS_NOTCONFIGURE                 0        ///< NMP Audio Decoder Status is not configured
#define  NMP_AUDDEC_STATUS_CONFIGURE                    1        ///< NMP Audio Decoder Status is configured
#define  NMP_AUDDEC_STATUS_START                        2        ///< NMP Audio Decoder Status is starting
#define  NMP_AUDDEC_STATUS_STOP                         3        ///< NMP Audio Decoder Status is stopped
#define  NMP_AUDDEC_STATUS_PAUSE                        4        ///< NMP Audio Decoder Status is pause
#define  NMP_AUDDEC_STATUS_PLAYFINISH                   5        ///< NMP Audio Decoder Status is finish playing
//@}

/**
    NMP Audio Decoder Maximum Channel
*/
#define  NMP_AUDDEC_MAX_CHANNEL							2

/**
    NMP Audio Decoder Maximum Size Of One Raw Data Block
*/
#define  NMP_AUDDEC_RAW_BLOCK_MAXSIZE					(NMI_AUDDEC_MAX_SAMPLERATE*NMP_AUDDEC_MAX_CHANNEL*2/NMI_AUDDEC_DECODE_BLOCK_CNT)

/**
    NMP Audio Decoder Maximum in Seconds
*/
#define	 NMP_AUDDEC_MAXIMUM_IN_SECONDS					1		///< seconds

/**
    NMP Audio Decoder RAW Data Block Number
*/
#define  NMP_AUDDEC_RAW_BLOCK_NUMBER					(NMI_AUDDEC_DECODE_BLOCK_CNT * NMP_AUDDEC_MAXIMUM_IN_SECONDS)

/**
    NMP Audio Decoder Bit-Stream Queue Size
*/
//@{
#define NMP_AUDDEC_BSSET_QUEUE_SIZE						NMP_AUDDEC_RAW_BLOCK_NUMBER//(NMI_AUDDEC_DECODE_BLOCK_CNT * NMP_AUDDEC_MAXIMUM_IN_SECONDS)

#define NMP_AUDDEC_DYNAMIC_CONTEXT          1    // 1: alloc context dynamically, but debug command could only be used when running
                                                 // 0: static context, debug command could be used anytime

/**
    Struct definition of Media Audio Decoder BS Queue information
*/
typedef struct {
	UINT32                      Front;							///< Front pointer
	UINT32                      Rear;							///< Rear pointer
	UINT32                      bFull;							///< Full flag
	NMI_AUDDEC_BS_INFO          Queue[NMP_AUDDEC_BSQ_MAX];
} NMP_AUDDEC_BSQ, *PNMP_AUDDEC_BSQ;

typedef struct {
	UINT32                      Front;              ///< Front pointer
	UINT32                      Rear;               ///< Rear pointer
	UINT32                      bFull;              ///< Full flag
	NMI_AUDDEC_MEM_RANGE        Queue[NMP_AUDDEC_RAWQ_MAX];
} NMP_AUDDEC_RAWQ, *PNMP_AUDDEC_RAWQ;

typedef struct {
	UINT32                      Front;                          ///< Front pointer
	UINT32                      Rear;                           ///< Rear pointer
	UINT32                      bFull;                          ///< Full flag
	UINT32                      Queue[NMP_AUDDEC_JOBQ_MAX];
} NMP_AUDDEC_JOBQ, *PNMP_AUDDEC_JOBQ;

/**
    Struct definition of Media Audio Decode Object
*/
typedef struct {
	UINT32                      Status;							///< Current status
	MP_AUDDEC_DECODER           *pDecoder;			            ///< decoder obj
	MP_AUDDEC_DECODER           Decoder;				        ///< dec ctrl
	NMP_AUDDEC_BSQ              bsQueue;                        ///< bs queue
	NMP_AUDDEC_RAWQ             rawQueue;                       ///< raw queue
	UINT32                      uiCodec;                        ///< audio codec type
	BOOL						bStart;							///< Start to decode
	NMI_AUDDEC_CB				*CallBackFunc;					///< Event callback function
} NMP_AUDDEC_OBJ, *PNMP_AUDDEC_OBJ;

extern void          nmp_auddec_install_id(void) _SECTION(".kercfg_text");
extern THREAD_HANDLE _SECTION(".kercfg_data") NMP_AUDDEC_TSK_ID;
extern ID            _SECTION(".kercfg_data") FLG_ID_NMP_AUDDEC;
extern SEM_HANDLE    _SECTION(".kercfg_data") NMP_AUDDEC_SEM_ID[];

#endif
