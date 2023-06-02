/**
    Header file of video encoding codec library

    Exported header file of video encoding codec library.

    @file       VideoEncode.h
    @ingroup    mIAVCODEC
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.
*/
#ifndef _VIDEOENCODE_H
#define _VIDEOENCODE_H

#include "dal_vdoenc.h"  // TODO: use kdrv later
#include "kdrv_videoenc/kdrv_videoenc.h"
#ifdef VDOENC_LL
#include "kdrv_type.h"
#endif

#define MP_VDOENC_SHOW_MSG 			1

/**
    Each bitstream buffer size for N frames encoding simultaneously
*/
#define MP_VDOENC_BSBUFSIZE  				(1024*1024) 	///< 1MB for BS temp buffer as simultaneously encode
#define MP_VDOENC_REFSIZE    				(2*1024*1024) 	///< 2MB for MV, RcRef, SideInfo ... (max enc limit: 5120*5120)


#define MP_VDOENC_QP_MIN 					0
#define MP_VDOENC_QP_MAX 					51
#define MP_VDOENC_ROI_NUM_MAX				10
#define MP_VDOENC_BSTR_VF_NUM_MAX  			60  							///< input video bitstream test pattern max frame numbers
#define MP_VDOENC_FIXED_YUV_NUM_MAX    		8   							///< max number of fixed YUV test patterns
#define MP_VDOENC_MD_INFO_SIZE				4112

typedef void (*MEDIAREC_3DNRCB)(UINT32 pathID, UINT32 config);
typedef void (*MEDIAREC_ISPCB)(UINT32 path_id, UINT32 config);
typedef void (*MEDIAREC_SPNCB)(UINT32 pathID, UINT32 config);

/**
    Video Encoder ID
*/
typedef enum {
	MP_VDOENC_ID_1,                			///< video encoder ID 1
	MP_VDOENC_ID_2,                			///< video encoder ID 2
	MP_VDOENC_ID_3,                			///< video encoder ID 3
	MP_VDOENC_ID_4,                			///< video encoder ID 4
	MP_VDOENC_ID_5,                			///< video encoder ID 5
	MP_VDOENC_ID_6,                			///< video encoder ID 6
	MP_VDOENC_ID_7,                			///< video encoder ID 7
	MP_VDOENC_ID_8,                			///< video encoder ID 8
	MP_VDOENC_ID_9,                			///< video encoder ID 9
	MP_VDOENC_ID_10,               			///< video encoder ID 10
	MP_VDOENC_ID_11,               			///< video encoder ID 11
	MP_VDOENC_ID_12,               			///< video encoder ID 12
	MP_VDOENC_ID_13,               			///< video encoder ID 13
	MP_VDOENC_ID_14,               			///< video encoder ID 14
	MP_VDOENC_ID_15,               			///< video encoder ID 15
	MP_VDOENC_ID_16,               			///< video encoder ID 16
	MP_VDOENC_ID_MAX,              			///< video encoder ID maximum
	ENUM_DUMMY4WORD(MP_VDOENC_ID)
} MP_VDOENC_ID;

/**
    Video Display Aspect Ratio
*/
typedef enum {
	MP_VDOENC_DAR_DEFAULT,         			///< default video display aspect ratio (the same as encoded image)
	MP_VDOENC_DAR_16_9,            			///< 16:9 video display aspect ratio
	MP_VDOENC_DAR_MAX,             			///< maximum video display aspect ratio
	ENUM_DUMMY4WORD(MP_VDOENC_DAR)
} MP_VDOENC_DAR;

/**
    Video Encoding Path
*/
typedef enum {
	MP_VDOENC_PATH_D2D,            			///< encoding path: D2D
	MP_VDOENC_PATH_IMEDIRECT,      			///< encoding path: IME/Direct
	MP_VDOENC_PATH_ALLDIRECT,      			///< encoding path: All direct
	MP_VDOENC_PATH_MAX,            			///< maximum encoding path value
	ENUM_DUMMY4WORD(MP_VDOENC_PATH)
} MP_VDOENC_PATH;

/**
    Video Encoding Profile
*/
typedef enum {
	MP_VDOENC_PROFILE_BASELINE,
	MP_VDOENC_PROFILE_MAIN,
	MP_VDOENC_PROFILE_HIGH,
	MP_VDOENC_PROFILE_MAX,
	ENUM_DUMMY4WORD(MP_VDOENC_PROFILE)
} MP_VDOENC_PROFILE;

/**
    SVC settings
*/
typedef enum {
	MP_VDOENC_SVC_DISABLE,         			///< disable SVC
	MP_VDOENC_SVC_LAYER1,          			///< enable SVC with GOP number 15
	MP_VDOENC_SVC_LAYER2,          			///< enable SVC with GOP number 15
	MP_VDOENC_SVC_MAX,             			///< SVC setting maximum
	ENUM_DUMMY4WORD(MP_VDOENC_SVC)
} MP_VDOENC_SVC;

/**
    Video Encoding Slice Type
*/
typedef enum {
	MP_VDOENC_P_SLICE = 0,
	MP_VDOENC_B_SLICE = 1,
	MP_VDOENC_I_SLICE = 2,
	MP_VDOENC_IDR_SLICE = 3,
	MP_VDOENC_KP_SLICE = 4,
	ENUM_DUMMY4WORD(MP_VDOENC_SLICE_TYPE)
} MP_VDOENC_SLICE_TYPE;

/**
    type of getting info from video encoding codec library.
    format: ER (*GetInfo)(MP_VDOENC_GETINFO_TYPE type, UINT32 *pparam1, UINT32 *pparam2, UINT32 *pparam3);
*/
typedef KDRV_VDOENC_GET_PARAM_ID MP_VDOENC_GETINFO_TYPE;

/**
    type of setting info to video encoding codec library.
    format: ER (*SetInfo)(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
*/
typedef KDRV_VDOENC_SET_PARAM_ID MP_VDOENC_SETINFO_TYPE;


typedef enum {
	MP_VDOENC_ROTATE_DISABLE,				///< no rotate
	MP_VDOENC_ROTATE_CCW, 					///< rotate counter clockwise
	MP_VDOENC_ROTATE_CW, 					///< rotate clockwise
	MP_VDOENC_ROTATE_180, 					///< rotate 180 degree
	ENUM_DUMMY4WORD(MP_VDOENC_ROTATE_TYPE)
} MP_VDOENC_ROTATE_TYPE;

typedef struct {
	UINT32 uiLTRInterval;   				///< long-term reference frame interval. range : 0 ~ 4095
	UINT32 uiLTRPreRef;    					///< long-term reference setting. 0 : all long-term reference to IDR frame, 1: reference latest long-term reference frame.
} MP_VDOENC_LTR_INFO;

typedef struct {
	UINT32  uiSarW;
	UINT32  uiSarH;
} MP_VDOENC_SAR;     // _TODO : remove it? seems no one use it

typedef struct {
	UINT32 enable;
	UINT32 strp_num;
	UINT32 strp_size[3];
} MP_VDOENC_LOWLATENCY;

typedef struct {
	UINT32  uiWidth;                      	            ///< Width
	UINT32  uiHeight;                                   ///< Height
	UINT32  uiYAddr;                                    ///< Y address
	UINT32  uiCbAddr;                                   ///< Cb address
	UINT32  uiCrAddr;                                   ///< Cr address
	UINT32  uiYLineOffset;               	            ///< Y line offset
	UINT32  uiUVLineOffset;                             ///< UV line offset
	UINT32  uiVidPathID;                                ///< vidpathid, start from 0,2015/03/09
	UINT32  uiBufID;                                    ///< bufid for release
	UINT64  TimeStamp;                                  ///< TimeStamp
	UINT32  uiSrcCompression;                           ///< Source compression (0: disable, 1:enable)
	KDRV_VDOENC_SDC SrcCompressInfo;                    ///< Source compression Info
	UINT32  uiMdAddr;                                   ///< Md address
	UINT32  uiProcessTime;                              ///< Process time for debugging
	UINT32	uiThumbnail;                                ///< 1: thumbnail
	UINT32	uiEncoded;                                  ///< has been encoded or not
	UINT32  uiRawCount;                                 ///< SIE frame count
	UINT32  uiInputTime;                                ///< YUV input timestamp
	INT32   iInputTimeout;                              ///< YUV input process timeout
	MP_VDOENC_LOWLATENCY LowLatencyInfo;                ///< Low latency Info
	KDRV_VDOENC_MD_INFO MDInfo;                         ///< MD Info
} MP_VDOENC_YUV_SRC;

typedef struct {
	UINT32  path_id;                                    ///< path_id
	UINT32  width;                      	            ///< width
	UINT32  height;                                     ///< height
	UINT32  y_addr;                                     ///< y address
	UINT32  c_addr;                                     ///< uv address
	UINT32  y_line_offset;               	            ///< y line offset
	UINT32  c_line_offset;                              ///< uv line offset
	UINT32  quality;                                    ///< jpeg quality
	UINT32  snap_addr;                                  ///< bs addr
	UINT32  snap_size;                                  ///< bs buffer size, encoded bs size
	UINT64  timestamp;                                  ///< YUV timestamp
} MP_VDOENC_YUV_SRCOUT;

typedef struct {
	UINT32 min_width;
	UINT32 min_height;
	UINT32 max_width;
	UINT32 max_height;
} MP_VDOENC_W_H_LIMIT;

typedef struct {
	MP_VDOENC_W_H_LIMIT h264e;
	MP_VDOENC_W_H_LIMIT h265e;
	MP_VDOENC_W_H_LIMIT jpege;
	UINT32 h264e_rotate_max_height;
} MP_VDOENC_HW_LMT;

typedef DAL_VDOENC_3DNR_INFO MP_VDOENC_3DNR_INFO;  // TODO : 520 doesn't use 3DNR, wait kdrv to support new TNR struct

typedef DAL_VDOENC_3DNRCB MP_VDOENC_3DNRCB; // TODO

typedef KDRV_VDOENC_ISPCB MP_VDOENC_ISPCB;

typedef KDRV_VDOENC_SPNCB MP_VDOENC_SPNCB;

typedef KDRV_VDOENC_AQ MP_VDOENC_AQ_INFO;

typedef KDRV_VDOENC_DESC MP_VDOENC_BUF;

typedef KDRV_VDOENC_CBR MP_VDOENC_CBR_INFO;

typedef KDRV_VDOENC_VBR MP_VDOENC_VBR_INFO;

typedef KDRV_VDOENC_EVBR MP_VDOENC_EVBR_INFO;

typedef KDRV_VDOENC_FIXQP MP_VDOENC_FIXQP_INFO;

typedef KDRV_VDOENC_MD_INFO MP_VDOENC_MD_INFO;

typedef KDRV_VDOENC_MEM_INFO MP_VDOENC_MEM_INFO;

typedef KDRV_VDOENC_USR_QP MP_VDOENC_QPMAP_INFO;

typedef DAL_VDOENC_RC_INFO MP_VDOENC_RC_INFO; // TODO : wait kdrv support it

typedef KDRV_VDOENC_ROW_RC MP_VDOENC_ROWRC_INFO;

typedef KDRV_VDOENC_ROI MP_VDOENC_ROI_INFO;

typedef DAL_VDOENC_SMART_ROI_INFO MP_VDOENC_SMART_ROI_INFO; // TODO : kdrv diff with dal

typedef KDRV_VDOENC_SDC MP_VDOENC_SRC_COMPRESS_INFO;

typedef KDRV_VDOENC_GDR MP_VDOENC_GDR_INFO;

typedef KDRV_VDOENC_SLICE_SPLIT MP_VDOENC_SLICESPLIT_INFO;

typedef KDRV_VDOENC_RDO MP_VDOENC_RDO_INFO;

typedef KDRV_VDOENC_JND MP_VDOENC_JND_INFO;

#ifdef VDOENC_LL
typedef KDRV_CALLBACK_FUNC MP_VDOENC_CALLBACK_FUNC;

typedef KDRV_VDOENC_LL_MEM_INFO MP_VDOENC_LL_MEM_INFO;

typedef KDRV_VDOENC_LL_MEM MP_VDOENC_LL_MEM;
#endif
/**
    Init structure
*/
typedef KDRV_VDOENC_INIT MP_VDOENC_INIT;

/**
    Encode structure
*/
typedef KDRV_VDOENC_PARAM MP_VDOENC_PARAM;

/**
    Structure of video encoder
*/
typedef struct {
	ER(*Initialize)(MP_VDOENC_INIT *ptr);  													///< initialize encoder
	ER(*Close)(void);   																			///< close encoder
	ER(*GetInfo)(MP_VDOENC_GETINFO_TYPE type, UINT32 *pparam1, UINT32 *pparam2, UINT32 *pparam3);   ///< get info from codec library
	ER(*SetInfo)(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);  		///< set parameters to codec library
#ifdef VDOENC_LL
	ER(*EncodeOne)(UINT32 type, MP_VDOENC_PARAM *ptr, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);  			///< encoding one video frame and wait ready
#else
	ER(*EncodeOne)(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *ptr);  			///< encoding one video frame and wait ready
#endif
	ER(*TriggerEnc)(MP_VDOENC_PARAM *ptr);  														///< trigger encoding one video frame but not wait ready (only for direct path)
	ER(*WaitEncReady)(MP_VDOENC_PARAM *ptr);  														///< wait for encoding ready
	ER(*AdjustBPS)(UINT32 type, UINT32 param1, UINT32 param2, UINT32 param3);  						///< adjusting BPS
	ER(*CustomizeFunc)(UINT32 type, void *pobj);  													///< codec customize function
	UINT32 checkID;     																			///< check ID
} MP_VDOENC_ENCODER, *PMP_VDOENC_ENCODER;

#endif
