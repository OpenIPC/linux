/*
    @file       NMediaRecVdoTrigTsk.h
    @ingroup    mIAPPMEDIAREC

    @brief      header file of media recorder
    @note
    @date       2013/03/18

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/
#ifndef _NMEDIARECVDOTRIGTSK_H
#define _NMEDIARECVDOTRIGTSK_H

#include "video_encode.h"
#include "nmediarec_vdoenc.h"


#define NMR_VDOTRIG_YUVQ_MAX            2              ///< Video Raw Queue Maximum number
#define NMR_VDOTRIG_SRCOUTQ_MAX         1              ///< Video SrcOut Queue Maximum number
#define NMR_VDOTRIG_JOBQ_MAX            160
#define NMR_VDOTRIG_BSQ_MAX             90             //30 fps * 3 secs
#define NMR_VDOTRIG_SMARTROIQ_MAX       3

#define NMR_VDOTRIG_D2DTSK_PRI          3               // task priority, SIE(2) > IPP(3) = VDOENC(3)

/**
    Struct definition of Media Video Stream YUV Queue information
*/
typedef struct {
	UINT32                      Front;                  ///< Front pointer
	UINT32                      Rear;                   ///< Rear pointer
	UINT32                      bFull;                  ///< Full flag
	MP_VDOENC_YUV_SRC     Queue[NMR_VDOTRIG_YUVQ_MAX];
} NMR_VDOTRIG_YUVQ, *PNMR_VDOTRIG_YUVQ;

typedef struct {
	UINT32                      Front;                  ///< Front pointer
	UINT32                      Rear;                   ///< Rear pointer
	UINT32                      bFull;                  ///< Full flag
	MP_VDOENC_YUV_SRCOUT        Queue[NMR_VDOTRIG_SRCOUTQ_MAX];
} NMR_VDOTRIG_SRCOUTQ, *PNMR_VDOTRIG_SRCOUTQ;

typedef struct {
	UINT32                      Front;                  ///< Front pointer
	UINT32                      Rear;                   ///< Rear pointer
	UINT32                      bFull;                  ///< Full flag
	UINT32                      Queue[NMR_VDOTRIG_JOBQ_MAX];
} NMR_VDOTRIG_JOBQ, *PNMR_VDOTRIG_JOBQ;

typedef struct {
	UINT32                      Front;                  ///< Front pointer
	UINT32                      Rear;                   ///< Rear pointer
	UINT32                      bFull;                  ///< Full flag
	NMI_VDOENC_MEM_RANGE        *Queue;
} NMR_VDOTRIG_BSQ, *PNMR_VDOTRIG_BSQ;

typedef struct {
	UINT32                      Front;                  ///< Front pointer
	UINT32                      Rear;                   ///< Rear pointer
	UINT32                      bFull;                  ///< Full flag
	MP_VDOENC_SMART_ROI_INFO    Queue[NMR_VDOTRIG_SMARTROIQ_MAX];
} NMR_VDOTRIG_SMARTROIQ, *PNMR_VDOTRIG_SMARTROIQ;

typedef enum {
	NMR_VDOENC_ENC_STATUS_IDLE=0,
	NMR_VDOENC_ENC_STATUS_CHECK_VALID,
	NMR_VDOENC_ENC_STATUS_ENCODE_TRIG,
	NMR_VDOENC_ENC_STATUS_ENCODE_OK_DONE,
	NMR_VDOENC_ENC_STATUS_ENCODE_FAIL_DONE,
	NMR_VDOENC_ENC_STATUS_BS_CB_DONE,
	ENUM_DUMMY4WORD(NMR_VDOENC_ENC_STATUS)
} NMR_VDOENC_ENC_STATUS;

/**
    Struct definition of Media Video Stream Object information
*/
typedef struct {
	MP_VDOENC_ENCODER           Encoder;
	MP_VDOENC_PARAM       		EncParam;
	NMR_VDOTRIG_YUVQ            rawQueue;
	NMR_VDOTRIG_SRCOUTQ         srcoutQueue;
	NMR_VDOTRIG_BSQ             bsQueue;
    NMR_VDOTRIG_SMARTROIQ       smartroiQueue;
	MP_VDOENC_RDO_INFO          rdo_info;
	BOOL                        rdo_en;
	MP_VDOENC_JND_INFO          jnd_info;
	BOOL                        jnd_en;
	//BisStream info
	UINT32                      BsStart;                ///< in:bitstream starting addr
	UINT32                      BsEnd;                  ///< in:bitstream end addr
	UINT32                      BsMax;                  ///< in:addr for bitstream rollback
	UINT32                      BsNow;                  ///< out: current bs addr
	UINT32                      BsSize;        		    ///< out: current bs size
	UINT32                      FrameCount;             ///< out: vid frame count
	UINT32                      SyncFrameN;             ///< out: sync frame num, usually as vfr
	UINT32                      WantRollback;           ///< out: if reach max, next i-frame should rollback
	NMR_VDOENC_ENC_STATUS       uiEncStatus;            ///< out: enc task status
#ifdef VDOENC_LL
	// VdoEnc LL
	UINT32                      QueueCount;             ///< in:yuv queue count
	UINT32                      EncTime;                ///< out: video encode time
	MP_VDOENC_CALLBACK_FUNC     Enc_CallBack;
	NMI_VDOENC_BS_INFO          vidBSinfo;
#endif
} NMR_VDOTRIG_OBJ, *PNMR_VDOTRIG_OBJ;


extern THREAD_DECLARE(NMR_VdoTrig_D2DTsk_H26X, p1);
extern THREAD_DECLARE(NMR_VdoTrig_D2DTsk_JPEG, p1);
extern ER       NMR_VdoTrig_InitParam(UINT32 fileID);
extern BOOL     NMR_VdoTrig_PutYuv(UINT32 fileID, MP_VDOENC_YUV_SRC *pYuvSrc);
extern BOOL     NMR_VdoTrig_GetYuv(UINT32 fileID, MP_VDOENC_YUV_SRC **pYuvSrc, BOOL bKeepElement);
extern UINT32   NMR_VdoTrig_GetYuvCount(UINT32 pathID);
extern BOOL     NMR_VdoTrig_PutSrcOut(UINT32 pathID, MP_VDOENC_YUV_SRCOUT *pSrcOut);
extern BOOL     NMR_VdoTrig_GetSrcOut(UINT32 pathID, MP_VDOENC_YUV_SRCOUT **pSrcOut);
extern BOOL     NMR_VdoTrig_PutSmartRoi(UINT32 pathID, MP_VDOENC_SMART_ROI_INFO *pSmartRoi);
extern BOOL     NMR_VdoTrig_GetSmartRoi(UINT32 pathID, MP_VDOENC_SMART_ROI_INFO *pSmartRoi);
extern UINT32   NMR_VdoTrig_GetSmartRoiCount(UINT32 pathID);
extern PMP_VDOENC_ENCODER NMR_VdoTrig_GetVidEncoder(UINT32 fileid);
extern ER       NMR_VdoTrig_TrigEncode(UINT32 pathID);
extern ER       NMR_VdoTrig_TrigSnapshotEncode(void);
extern ER       NMR_VdoTrig_Start_H26X(void);
extern ER       NMR_VdoTrig_Start_JPEG(void);
extern ER       NMR_VdoTrig_Close_H26X(void);
extern ER       NMR_VdoTrig_Close_JPEG(void);
extern ER       NMR_VdoTrig_CreatePluginObject(UINT32 pathID);
extern void     NMR_VdoTrig_InitJobQ_H26X(void);
extern void     NMR_VdoTrig_InitJobQ_H26X_Snapshot(void);
extern void     NMR_VdoTrig_InitJobQ_JPEG(void);
extern ER       NMR_VdoTrig_PutJob(UINT32 pathID);
extern ER       NMR_VdoTrig_PutJob_H26X_Snapshot(UINT32 pathID);
extern ER       NMR_VdoTrig_GetJob_H26X(UINT32 *pPathID);
extern ER       NMR_VdoTrig_GetJob_H26X_Snapshot(UINT32 *pPathID);
extern ER       NMR_VdoTrig_GetJob_JPEG(UINT32 *pPathID);
extern UINT32   NMR_VdoTrig_GetJobCount_H26X(void);
extern UINT32   NMR_VdoTrig_GetJobCount_H26X_Snapshot(void);
extern UINT32   NMR_VdoTrig_GetJobCount_JPEG(void);
extern UINT32   NMR_VdoTrig_GetBSCount(UINT32 pathID);
extern BOOL     NMR_VdoTrig_CheckBS(UINT32 pathID, UINT32 *uiAddr, UINT32 *uiSize, UINT32 *uiStartAddr);
extern BOOL     NMR_VdoTrig_LockBS(UINT32 pathID, UINT32 uiAddr, UINT32 uiSize);
extern BOOL     NMR_VdoTrig_UnlockBS(UINT32 pathID, UINT32 uiAddr);
extern void     NMR_VdoTrig_Stop(UINT32 pathID);
extern void     NMR_VdoTrig_CancelH26xTask(void);
extern void     NMR_VdoTrig_WaitTskIdle(UINT32 pathID);
extern UINT32  _NMR_VdoTrig_YUV_TMOUT_ResourceUnlock(UINT32 pathID);
#endif //_NMEDIARECVDOTRIGTSK_H


