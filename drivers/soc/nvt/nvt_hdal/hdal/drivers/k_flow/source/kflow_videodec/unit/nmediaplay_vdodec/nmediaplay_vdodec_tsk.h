/**
    Header file of Super Media Video Decode Task

    Exported header file of super media video decode task.

    @file       NMediaVdoDecTsk.h
    @ingroup    mIAPPMEDIAPLAY

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/

#ifndef _NMEDIAVDODECTSK_H
#define _NMEDIAVDODECTSK_H

#include "nmediaplay_vdodec.h"
#include "video_decode.h"
#include "kwrap/semaphore.h"
#include "kwrap/type.h"

/**
    NMP Video Decoder Status
*/
//@{
#define  NMP_VDODEC_STATUS_NOTCONFIGURE                 0        		///< NMP Video Decoder Status is not configured
#define  NMP_VDODEC_STATUS_CONFIGURE                    1        		///< NMP Video Decoder Status is configured
#define  NMP_VDODEC_STATUS_START                        2        		///< NMP Video Decoder Status is starting
#define  NMP_VDODEC_STATUS_STOP                         3        		///< NMP Video Decoder Status is stopped
#define  NMP_VDODEC_STATUS_PAUSE                        4        		///< NMP Video Decoder Status is pause
#define  NMP_VDODEC_STATUS_PLAYFINISH                   5        		///< NMP Video Decoder Status is finish playing
//@}

#define NMP_VDODEC_DYNAMIC_CONTEXT          1    // 1: alloc context dynamically, but debug command could only be used when running
                                                 // 0: static context, debug command could be used anytime

#define NMP_VDODEC_H264_CODEC_SIZE                    	0x6E0000		///< H264 Media Plug-in internal used memory size
#define NMP_VDODEC_H265_CODEC_SIZE                      0x6E0000      	///< H265 Media Plug-in internal used memory size

/**
    NMP Video Decoder RAW Data Block Number
*/
#define  NMP_VDODEC_RAW_BLOCK_NUMBER					4

/**
    NMP Video Decoder Bit-Stream Queue Size
*/
//@{
//#define NMP_VDODEC_BS_QUEUE_SIZE						4
#define NMP_VDODEC_BSQ_MAX                              4

#define NMP_VDODEC_WRAP_JOBQ_MAX         160
/**
    Struct definition of Media Video Decoder BS Queue information
*/
/*typedef struct {
	UINT32                      Front;									///< Front pointer
	UINT32                      Rear;									///< Rear pointer
	UINT32                      bFull;									///< Full flag
	PNMI_VDODEC_BS_SRC    		Queue;									///< Bit Stream Source Queue
} NMP_VDODEC_BSQ, *PNMP_VDODEC_BSQ;*/
typedef struct {
	UINT32                          Front;              ///< Front pointer
	UINT32                          Rear;               ///< Rear pointer
	UINT32                          bFull;              ///< Full flag
	NMI_VDODEC_BS_INFO              Queue[NMP_VDODEC_BSQ_MAX];
} NMP_VDODEC_BSQ, *PNMP_VDODEC_BSQ;

#ifdef VDODEC_LL
typedef struct {
	UINT32                      Front;                  ///< Front pointer
	UINT32                      Rear;                   ///< Rear pointer
	UINT32                      bFull;                  ///< Full flag
	UINT32                      Queue[NMP_VDODEC_WRAP_JOBQ_MAX];
} NMP_VDODEC_WRAPBS_JOBQ, *PNMP_VDODEC_WRAPBS_JOBQ;
#endif

/**
    Struct definition of Media Video Decode Configuration
*/
typedef struct {
	UINT32                      uiVideoDecType;							///< Dec type
	UINT32                      uiWidth;                                ///< Width (user assign for buffer allocation)
	UINT32                      uiHeight;                               ///< Height (user assign for buffer allocation)
	UINT32                      uiImageWidth;                           ///< Image width (codec parsing return)
	UINT32                      uiImageHeight;                          ///< Image height (codec parsing return)
	UINT32                      uiYLineOffset;                          ///< Image Y line offset (codec parsing return)
	UINT32                      uiUVLineOffset;                         ///< Image UV line offset (codec parsing return)
	UINT32                      uiDescAddr;                             ///< Desc addr
	UINT32                      uiDescSize;                             ///< Desc len
	UINT32                      uiDecoderBufAddr;                       ///< Buf addr of dec driver
	UINT32                      uiDecoderBufSize;                       ///< Buf size of dec driver
	UINT32                     	uiGOPNumber;							///< Gop
	UINT32                     	uiFrameRate;							///< Framerate
	UINT32                      uiRawStartAddr;							///< RAW data buffer start address
	UINT32                      uiRawBufSize;							///< RAW data buffer size
} NMP_VDODEC_CONFIG, *PNMP_VDODEC_CONFIG;

/**
    Struct definition of Media Video Decode Object
*/
typedef struct {
	UINT32                      Status;                                  ///< Current status
	PMP_VDODEC_DECODER          pMpVideoDecoder;                         ///< Media Plug-in object pointer
	NMP_VDODEC_BSQ              BsQueue;                                 ///< Bit-Stream Queue
	UINT64                      uiDecFrmNum;                             ///< Current decode frame number
	BOOL                        bStart;                                  ///< Start to decode
	NMI_VDODEC_CB               *CallBackFunc;                           ///< Event callback function
	MP_VDODEC_REFFRMCB          vRefFrmCb;                               ///< in: reference frame callback
#ifdef VDODEC_LL
	// VDODEC_LL
	UINT32                      uiYAddr;
	UINT32                      uiUVAddr;
	UINT32                      uiRawSize;
	BOOL                        bReleaseBsOnly;                          ///< release bs only
	NMI_VDODEC_RAW_INFO         VdoRawInfo;                              ///< video raw info
	MP_VDODEC_PARAM             mp_vdodec_param;
	NMI_VDODEC_BS_INFO          BsSrc;
	MP_VDODEC_CALLBACK_FUNC     Dec_CallBack;                            ///< decode callback
#endif
} NMP_VDODEC_OBJ, *PNMP_VDODEC_OBJ;

#ifdef VDODEC_LL
extern ER       NMP_VdoDec_TrigWrapRAW(void);
extern ER       NMP_VdoDec_Start_WrapRAW(void);
extern ER       NMP_VdoDec_Close_WrapRAW(void);
extern void     NMP_VdoDec_InitJobQ_WrapRAW(void);
extern ER       NMP_VdoDec_PutJob_WrapRAW(UINT32 pathID);
extern ER       NMP_VdoDec_GetJob_WrapRAW(UINT32 *pPathID);
extern UINT32   NMP_VdoDec_GetJobCount_WrapRAW(void);
#endif
extern void          nmp_vdodec_install_id(void) _SECTION(".kercfg_text");
extern void          nmp_vdodec_uninstall_id(void);
extern THREAD_HANDLE _SECTION(".kercfg_data") NMP_VDODEC_TSK_ID;
extern ID        _SECTION(".kercfg_data") FLG_ID_NMP_VDODEC;
extern SEM_HANDLE    _SECTION(".kercfg_data") NMP_VDODEC_SEM_ID[];
#ifdef VDODEC_LL
extern THREAD_HANDLE _SECTION(".kercfg_data") NMP_VDODEC_TSK_ID_WRAPRAW;
extern ID _SECTION(".kercfg_data") FLG_ID_NMP_VDODEC_WRAPRAW;
extern SEM_HANDLE _SECTION(".kercfg_data") NMP_VDODEC_TRIG_SEM_ID[];
#endif
#endif
