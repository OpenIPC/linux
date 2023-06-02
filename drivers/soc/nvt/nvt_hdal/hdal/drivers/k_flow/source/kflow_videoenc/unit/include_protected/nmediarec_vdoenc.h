#ifndef _NMEDIARECVDOENC_H
#define _NMEDIARECVDOENC_H

#include "nvt_media_interface.h"

#define VDOENC_MAX_RECONFRM_NUM 3

#if _TODO
#else
#define ISF_LATENCY_DEBUG   0
#endif

typedef enum {
	NMI_VDOENC_PARAM_ALLOC_SIZE,
	NMI_VDOENC_PARAM_ENCODER_OBJ,
	NMI_VDOENC_PARAM_MEM_RANGE,					//codec + enc buf range
	NMI_VDOENC_PARAM_ENCBUF_ADDR,				//enc buf addr
	NMI_VDOENC_PARAM_ENCBUF_SIZE,				//enc buf size
	NMI_VDOENC_PARAM_MAX_MEM_INFO,
	NMI_VDOENC_PARAM_ALLOC_SNAPSHOT_BUF,
	NMI_VDOENC_PARAM_SNAPSHOT,
	NMI_VDOENC_REG_CB,
	NMI_VDOENC_PARAM_RECFORMAT,                 //MEDIAREC_GOLFSHOT, MEDIAREC_TIMELAPSE
	NMI_VDOENC_PARAM_FILETYPE,
	NMI_VDOENC_PARAM_WIDTH,
	NMI_VDOENC_PARAM_HEIGHT,
	NMI_VDOENC_PARAM_FRAMERATE,
	NMI_VDOENC_PARAM_ENCBUF_MS,				    //enc buf length (unit: ms)
	NMI_VDOENC_PARAM_ENCBUF_RESERVED_MS,        //enc buf reserved length (unit: ms)
	NMI_VDOENC_PARAM_CODEC,
	NMI_VDOENC_PARAM_PROFILE,
	NMI_VDOENC_PARAM_GOPTYPE,
	NMI_VDOENC_PARAM_INITQP,
	NMI_VDOENC_PARAM_MINQP,
	NMI_VDOENC_PARAM_MAXQP,
	NMI_VDOENC_PARAM_MIN_I_RATIO,				//the ratio of I frame min size to bitrate
	NMI_VDOENC_PARAM_MIN_P_RATIO,				//the ratio of P frame min size to bitrate
	NMI_VDOENC_PARAM_MAX_FRAME_QUEUE,
	NMI_VDOENC_PARAM_BUILTIN_MAX_FRAME_QUEUE,
	NMI_VDOENC_PARAM_CBRINFO,
	NMI_VDOENC_PARAM_EVBRINFO,
	NMI_VDOENC_PARAM_VBRINFO,
	NMI_VDOENC_PARAM_FIXQPINFO,
	NMI_VDOENC_PARAM_AQINFO,
	NMI_VDOENC_PARAM_3DNR_CB,
	NMI_VDOENC_PARAM_TARGETRATE,
	NMI_VDOENC_PARAM_DAR,
	NMI_VDOENC_PARAM_ROI,
	NMI_VDOENC_PARAM_START_SMART_ROI,
	NMI_VDOENC_PARAM_WAIT_SMART_ROI,
	NMI_VDOENC_PARAM_SMART_ROI,
	NMI_VDOENC_PARAM_DIS,
	NMI_VDOENC_PARAM_TV_RANGE,
	NMI_VDOENC_PARAM_START_TIMER_BY_MANUAL,
	NMI_VDOENC_PARAM_START_TIMER,
	NMI_VDOENC_PARAM_STOP_TIMER,
	NMI_VDOENC_PARAM_DROP_FRAME,
	NMI_VDOENC_PARAM_SKIP_FRAME,
	NMI_VDOENC_PARAM_SVC,
	NMI_VDOENC_PARAM_LTR,
	NMI_VDOENC_PARAM_MULTI_TEMPORARY_LAYER,		//smart265 multi temporary layer
	NMI_VDOENC_PARAM_USRQP,
	NMI_VDOENC_PARAM_ROTATE,                    //rotate enc(0:disable, 1:counter clockwise, 2:clockwise)
	NMI_VDOENC_PARAM_TIMELAPSE_TIME,
	NMI_VDOENC_PARAM_IMM_CB,                    //register callback function for the events which should be processed immediately
	NMI_VDOENC_PARAM_GOPNUM,
	NMI_VDOENC_PARAM_FRAME_INTERVAL,
	NMI_VDOENC_PARAM_UNLOCK_BS_ADDR,
	NMI_VDOENC_PARAM_TRIGGER_MODE,				//trigger mode (timer, direct, or notify)
	NMI_VDOENC_PARAM_RESET_IFRAME,				//reset hw to start encoding i frame
	NMI_VDOENC_PARAM_RESET_SEC,					//reset sec info (for displaying recoding time)
	NMI_VDOENC_PARAM_TIMERRATE_IMM,
	NMI_VDOENC_PARAM_TIMERRATE2_IMM,
	NMI_VDOENC_PARAM_LEVEL_IDC,
	NMI_VDOENC_PARAM_ENTROPY,
	NMI_VDOENC_PARAM_GRAY,
	NMI_VDOENC_PARAM_DEBLOCK,
	NMI_VDOENC_PARAM_GDR,
	NMI_VDOENC_PARAM_VUI,
	NMI_VDOENC_PARAM_SLICE_SPLIT,
	NMI_VDOENC_PARAM_USR_QP_MAP,
	NMI_VDOENC_PARAM_SKIP_LOFF_CHECK,
	NMI_VDOENC_PARAM_IMGFMT,                     // yuv format for jpeg encode
	NMI_VDOENC_PARAM_JPG_QUALITY,                // encode quality for jpeg encode
	NMI_VDOENC_PARAM_JPG_RESTART_INTERVAL,       // encode restart interval for jpeg encode
	NMI_VDOENC_PARAM_JPG_BITRATE,                // encode bitrate for jpeg encode
	NMI_VDOENC_PARAM_JPG_FRAMERATE,              // encode framerate for jpeg encode
	NMI_VDOENC_PARAM_JPG_VBR_MODE,               // encode rc mode, 0: CBR , 1: VBR
	NMI_VDOENC_PARAM_ROWRC,
	NMI_VDOENC_PARAM_CANCEL_H26X_TASK,
	NMI_VDOENC_PARAM_LOWLATENCY,
	NMI_VDOENC_PARAM_IS_ENCODING,
	NMI_VDOENC_PARAM_PARTIAL_BS_ADDR,
	NMI_VDOENC_PARAM_DESC_LEN,
	NMI_VDOENC_PARAM_BS_RESERVED_SIZE,
	NMI_VDOENC_PARAM_ISP_CB,
	NMI_VDOENC_PARAM_RDO,
	NMI_VDOENC_PARAM_JND,
	NMI_VDOENC_PARAM_H26X_VBR_POLICY,            // H26X VBR policy, 0: original , 1: hisi-like
	NMI_VDOENC_PARAM_TRIGBSQ_SIZE,
	NMI_VDOENC_PARAM_BSQ_MEM,
	NMI_VDOENC_PARAM_JPG_YUV_TRAN_ENABLE,		// input YUV420, output 422 bs
	NMI_VDOENC_PARAM_H26X_SRCOUT_JOB,		    // h26x src_out snapshot, put_job + set_srcout
	NMI_VDOENC_PARAM_JPG_RC_MIN_QUALITY,
	NMI_VDOENC_PARAM_JPG_RC_MAX_QUALITY,
	NMI_VDOENC_PARAM_HW_LIMIT,
	NMI_VDOENC_PARAM_GOPNUM_ONSTART,
#ifdef VDOENC_LL
	NMI_VDOENC_PARAM_LL_MEM_SIZE,
	NMI_VDOENC_PARAM_LL_MEM_RANGE,
#endif
	NMI_VDOENC_PARAM_FASTBOOT_CODEC,
	NMI_VDOENC_PARAM_COLMV_ENABLE,
	NMI_VDOENC_PARAM_LONG_START_CODE,
	NMI_VDOENC_PARAM_H26X_SVC_WEIGHT_MODE,   // H26X svc weight mode, 0: original , 1: svc weight mode
	NMI_VDOENC_PARAM_QUALITY_BASE_MODE_ENABLE,
	NMI_VDOENC_PARAM_DMA_ABORT,
	NMI_VDOENC_PARAM_POSTSHARPEN_CB,
	NMI_VDOENC_PARAM_FASTBOOT_VPRC_SRC_DEV,
	NMI_VDOENC_PARAM_FASTBOOT_VPRC_SRC_PATH,
	NMI_VDOENC_PARAM_COMM_RECFRM_ENABLE,
	NMI_VDOENC_PARAM_RECON_FRAME_SIZE,
	NMI_VDOENC_PARAM_RECON_FRAME_NUM,
	NMI_VDOENC_PARAM_RECONFRM_RANGE,
	NMI_VDOENC_PARAM_BS_QUICK_ROLLBACK,
	NMI_VDOENC_PARAM_HW_PADDING_MODE,
	NMI_VDOENC_PARAM_FASTBOOT_ISP_ID,
} NMI_VDOENC_PARAM;

typedef enum {
	NMI_VDOENC_ACTION_START,
	NMI_VDOENC_ACTION_STOP,
	NMI_VDOENC_ACTION_MAX,
} NMI_VDOENC_ACTION;

typedef enum {
	NMI_VDOENC_EVENT_INFO_READY,				//notify video header ready
	NMI_VDOENC_EVENT_BS_CB,						//notify enc bs ready
	NMI_VDOENC_EVENT_FRAME_INTERVAL,			//notify every frame_rate*seamless_sec
	NMI_VDOENC_EVENT_3DNR_ENC_SETTING,			//get current 3dnr level from ap
	NMI_VDOENC_EVENT_STAMP_CB,					//notify to draw stamp
	NMI_VDOENC_EVENT_SEC_CB,					//notify to update displaying sec
	NMI_VDOENC_EVENT_NOTIFY_IPL,				//notify ipl to trigger
	NMI_VDOENC_EVENT_RAW_FRAME_COUNT,           //update SIE frame count
	NMI_VDOENC_EVENT_ENC_BEGIN,					//notify enc begin
	NMI_VDOENC_EVENT_ENC_END,					//notify enc end
	NMI_VDOENC_EVENT_SNAPSHOT_CB,				//notify snapshot bs ready
	NMI_VDOENC_EVENT_GET_BS_START_PA,			//get bs_start pa
	NMI_VDOENC_EVENT_MAX,
	ENUM_DUMMY4WORD(NMI_VDOENC_EVENT)
} NMI_VDOENC_EVENT;

typedef enum {
	NMI_VDOENC_TRIGGER_TIMER,					//for DVCam by internal timer
	NMI_VDOENC_TRIGGER_DIRECT,					//for IPCam direct input trigger
	NMI_VDOENC_TRIGGER_NOTIFY,					//for IPCam with two buf
	NMI_VDOENC_TRIGGER_MAX,
	ENUM_DUMMY4WORD(NMI_VDOENC_TRIGGER)
} NMI_VDOENC_TRIGGER;

typedef enum {
	NMI_VDOENC_TOP_MD_TIME,    				//for set md info time
	NMI_VDOENC_TOP_SW_TIME,    				//for kdriver sw time
	NMI_VDOENC_TOP_HW_TIME,    				//for hw time
	NMI_VDOENC_TOP_OSD_INPUT_TIME,    			//for osd input time
	NMI_VDOENC_TOP_OSD_FINISH_TIME,    			//for osd finish time
	NMI_VDOENC_TOP_MAX,
	ENUM_DUMMY4WORD(NMI_VDOENC_TOP)
} NMI_VDOENC_TOP;

typedef struct {
	UINT32 Addr;    							//memory buffer starting address
	UINT32 Size;    							//memory buffer size
} NMI_VDOENC_MEM_RANGE, *PNMI_VDOENC_MEM_RANGE;

typedef struct {
	UINT32 Addr;    							//memory buffer starting address
	UINT32 Size;    							//memory buffer size
	UINT32 Idx;    								//memory buffer index
} NMI_VDOENC_RECBUF_RANGE, *PNMI_VDOENC_RECBUF_RANGE;

typedef struct {
	UINT32 PathID;          					//path id
	UINT32 BufID;           					//memory block id
	UINT32 IsKey;           					//video is key frame or not
	UINT32 IsIDR2Cut;       					//video is idr to cut here
	UINT32 FrameType;       					//video frame type (0:P, 1:B, 2:I, 3:IDR, 4:KP)
	UINT32 Addr;            					//1st output bit-stream address
	UINT32 Size;            					//1st output bit-stream size
	UINT32 Addr2;            					//2nd output bit-stream address
	UINT32 Size2;            					//2nd output bit-stream address
	UINT32 RawYAddr;        					//raw y address
	UINT32 SVCSize;         					//svc header size
	UINT32 TemporalId;      					//svc temporal layer id (0, 1, 2)
	UINT32 Occupied;        					//raw yuv frame need to be encoded again
	UINT64 TimeStamp;       					//timestamp
	UINT32 FrameCount;      					//frame count
	UINT32 SrcOutYAddr;     					//src out y addr
	UINT32 SrcOutUVAddr;						//src out uv addr
	UINT32 SrcOutWidth;     					//src out width
	UINT32 SrcOutHeight;    					//src out height
	UINT32 SrcOutYLoft;     					//src out y line offset
	UINT32 SrcOutUVLoft;    					//src out uv line offset
	UINT32 uiBaseQP;						//base QP
	UINT32 uiMotionRatio;                       //motion ratio
	UINT32 y_mse;								//y mean square error
	UINT32 u_mse;								//u mean square error
	UINT32 v_mse;								//v mean square error
	UINT32 nalu_info_addr;                      //nalu_info_addr(va)
#if ISF_LATENCY_DEBUG
	//Check latency og stream
	UINT64 enc_timestamp_start;
	UINT64 enc_timestamp_end;
#endif
} NMI_VDOENC_BS_INFO, *PNMI_VDOENC_BS_INFO;

typedef enum {
	NMR_VDOENC_UNKNOWN,
	NMR_VDOENC_MJPG,
	NMR_VDOENC_H264,
	NMR_VDOENC_H265,
	ENUM_DUMMY4WORD(NMR_VDOENC_CODEC)
} NMR_VDOENC_CODEC;

typedef enum {
	NMR_VDOENC_P_SLICE = 0,
	NMR_VDOENC_B_SLICE = 1,
	NMR_VDOENC_I_SLICE = 2,
	NMR_VDOENC_IDR_SLICE = 3,
	NMR_VDOENC_KP_SLICE = 4
} NMR_VDOENC_SLICE_TYPE;

typedef struct {
	UINT32 				uiCodec;
	UINT32				uiWidth;
	UINT32				uiHeight;
	UINT32				uiTargetByterate;
	UINT32				uiEncBufMs;
	UINT32				uiRecFormat;
	UINT32				uiSVCLayer;
	UINT32				uiLTRInterval;
	UINT32				uiRotate;
	BOOL				bAllocSnapshotBuf;
	BOOL				bRelease;				//release max buf
	UINT32				uiSnapShotSize;			//out: snapshot size
	UINT32				uiCodecsize;			//out: codec size
	UINT32				uiEncsize;				//out: enc buf size
	BOOL				bColmvEn;
	BOOL				bCommRecFrmEn;
	BOOL				bFitWorkMemory;         //TRUE: just calculate by setting, FALSE: check all (h264/h265) - (low_latency) - (rotate) combination to pick max memory
	BOOL				bQualityBaseMode;
} NMI_VDOENC_MAX_MEM_INFO, *PNMI_VDOENC_MAX_MEM_INFO;


typedef UINT32 (NMI_VDOENC_CB)(UINT32 event, UINT32 param);
typedef void (NMI_VDOENC_IMM_CB)(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3);

extern void             NMR_VdoEnc_AddUnit(void);
extern void             NMR_VdoEnc_Top(int (*dump)(const char *fmt, ...));

#endif //_NMEDIARECVDOENC_H
