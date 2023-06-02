/*
    @file       NMediaRecVdoEncTsk.h
    @ingroup    mIAPPMEDIAREC

    @brief      header file of media recorder
    @note
    @date       2013/03/18

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/
#ifndef _NMEDIARECVDOENCTSK_H
#define _NMEDIARECVDOENCTSK_H

#include "nmediarec_vdoenc.h"
#include "video_encode.h"
#include "kwrap/semaphore.h"
#include "kwrap/type.h"
#include "kwrap/file.h"

#define NMR_VDOENC_MAX_PATH     			16
#define NMR_VDOENC_SRCOUT_USE_REF_BUF     	0
#define NMR_VDOENC_MD_MAP_MAX_SIZE			8196  //8KB for 4K video (3840*2176)/(32*32)
#define NMR_VDOENC_MIN_I_RATIO				150
#define NMR_VDOENC_MIN_P_RATIO				100

#define NMR_VDOENC_DYNAMIC_CONTEXT          0    // 1: alloc context dynamically, but debug command could only be used when running
                                                 // 0: static context, debug command could be used anytime

#define FLG_NMR_VDOENC_IDLE          		FLGPTN_BIT(0)   //0x00000001
#define FLG_NMR_VDOENC_ENCODE           	FLGPTN_BIT(1)   //0x00000002
#define FLG_NMR_VDOENC_PRELOAD      		FLGPTN_BIT(2)   //0x00000004
#define FLG_NMR_VDOENC_STOP					FLGPTN_BIT(3)   //0x00000008
#define FLG_NMR_VDOENC_STOP_DONE			FLGPTN_BIT(4)   //0x00000010
#ifdef VDOENC_LL
#define FLG_NMR_VDOENC_ALLCB_DONE			FLGPTN_BIT(5)   //0x00000020
#endif

//msg level for debugging
#define NMR_VDOENC_MSG_ENCTIME     			((UINT32)0x00000001 << 0)   //0x00000001
#define NMR_VDOENC_MSG_INPUT       			((UINT32)0x00000001 << 1)   //0x00000002
#define NMR_VDOENC_MSG_OUTPUT      			((UINT32)0x00000001 << 2)   //0x00000004
#define NMR_VDOENC_MSG_PROCESS				((UINT32)0x00000001 << 3)   //0x00000008
#define NMR_VDOENC_MSG_MD					((UINT32)0x00000001 << 4)   //0x00000010
#define NMR_VDOENC_MSG_SMART_ROI			((UINT32)0x00000001 << 5)   //0x00000020
#define NMR_VDOENC_MSG_RING_BUF				((UINT32)0x00000001 << 6)   //0x00000040
#define NMR_VDOENC_MSG_DROP				    ((UINT32)0x00000001 << 7)   //0x00000080
#define NMR_VDOENC_MSG_YUV_TMOUT		    ((UINT32)0x00000001 << 8)   //0x00000100
#define NMR_VDOENC_MSG_SRCOUT      			((UINT32)0x00000001 << 9)   //0x00000200
#define NMR_VDOENC_MSG_OSD      			((UINT32)0x00000001 << 10)  //0x00000400

#define NMR_VDOENC_RC_CBR					1
#define NMR_VDOENC_RC_VBR					2
#define NMR_VDOENC_RC_FIXQP					3
#define NMR_VDOENC_RC_EVBR					4

/*
    recording information structure
*/
typedef enum {
	NMR_VDOENC_TIMER_STATUS_NORMAL,
	NMR_VDOENC_TIMER_STATUS_RESET,
	NMR_VDOENC_TIMER_STATUS_ALREADY_RESET,
	NMR_VDOENC_TIMER_STATUS_RESET2,
	ENUM_DUMMY4WORD(NMR_VDOENC_TIMER_STATUS)
} NMR_VDOENC_TIMER_STATUS;

typedef enum {
    NMR_VDOENC_PADDING_MODE_ZERO = 0,  // padding with zero (default)
    NMR_VDOENC_PADDING_MODE_COPY = 1,  // padding with copied bounday-line pixels
	ENUM_DUMMY4WORD(NMR_VDOENC_PADDING_MODE)
} NMR_VDOENC_PADDING_MODE;

typedef struct {
	UINT32                      uiEncBufMs;             //in: enc buf length (unit: ms)
	UINT32                      uiEncBufReservedMs;     //in: enc buf reserved length (unit: ms)
	UINT32                      uiSnapshotAddr;         //in: snapshot addr
	UINT32                      uiSnapshotSize;         //in: snapshot size
	UINT32                      uiVCodecAddr;           //in: video codec addr
	UINT32                      uiVCodecSize;           //in: video codec size
	UINT32                      uiBSStart;              //in: video bs start addr
	UINT32                      uiBSEnd;                //in: video bs end addr
	UINT32                      uiBSMax;                //in: video bs reserved addr
	UINT32                      uiMinIRatio;            //in: min I ratio
	UINT32                      uiMinPRatio;            //in: min P ratio
	UINT32                      uiMinISize;				//in: min I size
	UINT32                      uiMinPSize;				//in: min P size
	UINT32                      uiTempBuf;              //out: temp bs output buffer when direct
	UINT32                      uiTempSize;             //out: temp bs output size when direct
	UINT32                      uiRecBuffAddr[VDOENC_MAX_RECONFRM_NUM];       //in: video reconstruct buffer addr
	UINT32                      uiRecBuffSize;          //in: video reconstruct buffer size
	UINT32                      uiRecBuffNum;           //in: video reconstruct buffer number
} NMR_VDOENC_MEMINFO;

typedef struct {
	UINT32                      uiCur[4];
	UINT32                      uiMin;
	UINT32                      uiMax;
} NMR_VDOENC_TOPINFO;

typedef struct {
	NMR_VDOENC_MEMINFO          MemInfo;                //in: buf info
	MP_VDOENC_INIT         		InitInfo;               //in: init info
	MP_VDOENC_CBR_INFO          CbrInfo;				//in: cbr info
	MP_VDOENC_EVBR_INFO         EVbrInfo;				//in: evbr info
	MP_VDOENC_VBR_INFO          VbrInfo;				//in: vbr info
	MP_VDOENC_FIXQP_INFO        FixQpInfo;				//in: fix qp info
	MP_VDOENC_ROWRC_INFO        RowRcInfo;				//in: rowrc info
	MP_VDOENC_AQ_INFO           AqInfo;					//in: aq info
	MP_VDOENC_ROI_INFO			RoiInfo;  				//in: set ROI Info (max count: 10)
	MP_VDOENC_MD_INFO			MdInfo;  				//in: set MD Info
	MP_VDOENC_3DNRCB          	v3dnrCb;                //in: 3NDR setting callback
	MP_VDOENC_ISPCB             ispCb;                  //in: ISP ratio callback
	MP_VDOENC_SPNCB             vspnCb;                 //in: postsharpen setting callback
	MP_VDOENC_GDR_INFO          GdrInfo;                //in: gdr info
	MP_VDOENC_QPMAP_INFO        QpMapInfo;              //in: QP map info
	MP_VDOENC_SLICESPLIT_INFO   SliceSplitInfo;         //in: slice split info
	MP_VDOENC_ENCODER           *pEncoder;				//in: encoder obj
#ifdef VDOENC_LL
	MP_VDOENC_YUV_SRC           *pYuvSrc;				//in: encode yuv source
#endif
	NMI_VDOENC_CB               *CallBackFunc;          //in: event inform callback
	NMI_VDOENC_CB               *IPLTriggerCB;          //in: callback to trigger IPL direct path
	NMI_VDOENC_IMM_CB           *ImmProc;               //in: immediately process//2012/09/12 Meg
	UINT32                      uiVidCodec;             //in: codec
	UINT32                      uiGopType;              //in: Gop type (I/P only, I/P/B, ...)
	UINT32                      uiRecFormat;            //in: MEDIAREC_TIMELAPSE, MEDIAREC_GOLFSHOT ...
	UINT32                      uiFileType;             //in: file format: MEDIAREC_MP4, MEDIAREC_TS
	UINT32                      uiBsReservedSize;       //in: reserved bs header size for each frame
	UINT32						uiMaxFrameQueue;		//in: max frame queue count in ring buf
	UINT32                      uiFrameInterval;        //in: frame internal
	UINT32						uiSkipFrame; 			//in: skip frame
	UINT32 						uiTriggerMode;			//in: trigger mode (timer, direct, notify)
	UINT32                      uiTimelapseTime;        //in: timelapse time
	INT32                      	iTimerInterval;         //in: timer interval (unit: us)
	UINT64                      uiCurrentTime64;		//in: current time (unit: us)
	UINT64                      uiLastTime64;			//in: last time (unit: us)
	UINT32                      uiLastClock;			//in: last clock (unit: us)
	NMR_VDOENC_TIMER_STATUS		uiTimerStatus;			//in: timer status
	UINT32						uiTimerResetCount;		//in: timer reset count
	UINT32                      uiTimerRate;         	//in: trigger rate of timer
	BOOL                        bDis;					//in: enable digital image stabilzier
	BOOL                        bStartTimerByManual;    //in: start timer by manual
	BOOL                        bStartSmartRoi;         //in: enable smart roi
	BOOL                        bWaitSmartRoi;          //in: wait smart roi
	BOOL                        bAllocSnapshotBuf;      //in: alloc snapshot buf
	BOOL                        bSnapshot;              //in: start snapshot
	BOOL                        bStart;                 //in: start to enc
	BOOL 						bTimelapseTrigger;		//in: timelapse trigger to encode
	BOOL                        bResetIFrame;           //in: reset to start encoding I frame
	BOOL                        bResetEncoder;          //in: reset encoder if gop, framerate or byterate of Cbr/Vbr info have been changed
	BOOL                        bResetSec;          	//in: reset sec
	BOOL                        bTvRange;				//in: YUV color space in tv range
	BOOL                        bSkipLoffCheck;			//in: skip LineOffset & Width relation check
	UINT32						uiMsgLevel;				//in: msg level
	UINT32                      uiSetRCMode;			//in: set rc mode (0: none, 1: cbr, 2: vbr, 3:evbr)
	UINT32                      uiSnapShotSize;			//in: snapshot buf size
	UINT32                      uiCodecSize;			//in: codec buf size
	UINT32                      uiEncSize;				//in: enc buf size
	BOOL                        bQualityBase;			//in: quality base mode enable
	UINT32                      uiLongStartCode;			//in: long start code
	UINT32                      uiDescAddr;             //out: desc addr
	UINT32                      uiDescSize;             //out: desc size
	UINT64                      uiPreEncDropCount;		//out: previous enc drop count
	UINT64                      uiPreEncInCount;		//out: previous enc input count
	UINT64                      uiPreEncErrCount;		//out: previous enc output count
	UINT64                      uiEncDropCount;			//out: enc drop count
	UINT64                      uiEncInCount;			//out: enc input count
	UINT64                      uiEncOutCount;			//out: enc output count
	UINT64                      uiEncReCount;			//out: re-enc count
	UINT64                      uiEncErrCount;			//out: enc error count
	UINT32                      uiBSQAddr;
	NMR_VDOENC_TOPINFO                      uiEncTopTime[NMI_VDOENC_TOP_MAX];
	UINT32                      uiVidFastbootCodec;			//in: fastboot video codec type
	BOOL                        bBsQuickRollback;			//in: bs buffer quick rollback
	NMR_VDOENC_PADDING_MODE     uiEncPaddingMode;			//in: hw padding mode
	UINT32                      uiVidFastbootVprcSrcDev;		//in: fastboot video yuv from which vproc device
	UINT32                      uiVidFastbootVprcSrcPath;		//in: fastboot video yuv from which vproc path
	UINT32                      uiVidFastbootGdc;			    //in: fastboot video gdc
	UINT32                      uiVidFastbootQualityLv;		    //in: fastboot video quality level
	UINT32                      uiVidFastbootISPId; 			//in: fastboot isp id
} NMR_VDOENC_OBJ;

extern void 			NMR_VdoEnc_FpsTimerTrigger(UINT32 pathID);
extern void 			NMR_VdoEnc_TimerTrigger(UINT32 pathID);
extern void             NMR_VdoEnc_EncInfo(UINT32 pathID);
extern UINT32           NMR_VdoEnc_GetBytesPerSecond(UINT32 pathID);
extern void             NMR_VdoEnc_GetDesc(UINT32 pathID, UINT32 *pAddr, UINT32 *pSize);
extern UINT32           NMR_VdoEnc_GetVidInputPathNum(void);
extern void             NMR_VdoEnc_FpsInfo(UINT32 pathID, UINT32 Enable);
extern void             NMR_VdoEnc_RcInfo(UINT32 pathID, UINT32 Enable);
extern void 			NMR_VdoEnc_SetDesc(UINT32 pathID);
extern void             NMR_VdoEnc_SetEncodeParam(UINT32 pathID);
extern void 			NMR_VdoEnc_SetSmartROI(UINT32 pathID, UINT32 Enable);
extern void 			NMR_VdoEnc_SetROI(UINT32 pathID, MP_VDOENC_ROI_INFO *pRoiInfo);
extern void 			NMR_VdoEnc_ShowMsg(UINT32 pathID, UINT32 msgLevel);
extern void             nmr_vdoenc_install_id(void) _SECTION(".kercfg_text");
extern void             nmr_vdocodec_install_id(void) _SECTION(".kercfg_text");

extern THREAD_HANDLE _SECTION(".kercfg_data") NMR_VDOTRIG_D2DTSK_ID_H26X;
extern THREAD_HANDLE _SECTION(".kercfg_data") NMR_VDOTRIG_D2DTSK_ID_H26X_SNAPSHOT;
extern THREAD_HANDLE _SECTION(".kercfg_data") NMR_VDOTRIG_D2DTSK_ID_JPEG;
extern ID _SECTION(".kercfg_data") FLG_ID_NMR_VDOTRIG_H26X;
extern ID _SECTION(".kercfg_data") FLG_ID_NMR_VDOTRIG_H26X_SNAPSHOT;
extern ID _SECTION(".kercfg_data") FLG_ID_NMR_VDOTRIG_JPEG;
extern SEM_HANDLE _SECTION(".kercfg_data") NMR_VDOENC_SEM_ID[];
extern SEM_HANDLE _SECTION(".kercfg_data") NMR_VDOENC_COMMON_SEM_ID;
extern SEM_HANDLE _SECTION(".kercfg_data") NMR_VDOENC_YUV_SEM_ID[];

#endif //_NMEDIARECVDOENCTSK_H

