/**
    Header file of vdoenc library

    Exported header file of vdoenc library.

    @file       dal_vdoenc.h
    @ingroup    mIAVCODEC
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/
#ifndef _DAL_VDOENC_H
#define _DAL_VDOENC_H

#if defined __UITRON || defined __ECOS
#include <stdio.h>
#include <string.h>
#include "Type.h"
#include "kernel.h"
#else
#ifdef __KERNEL__
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include <linux/module.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#define debug_msg        printk
#else
#endif
#endif

#define DAL_VDOENC_SHOW_MSG								ENABLE
#define DAL_VDOENC_QP_MIN 								0
#define DAL_VDOENC_QP_MAX 								51
#define DAL_VDOENC_ROI_NUM_MAX							10
#define DAL_VDOENC_MD_INFO_SIZE							4112


/**
    Video Encoder ID
*/
typedef enum {
	DAL_VDOENC_ID_1,                			        ///< video encoder ID 1
	DAL_VDOENC_ID_2,                			        ///< video encoder ID 2
	DAL_VDOENC_ID_3,                			        ///< video encoder ID 3
	DAL_VDOENC_ID_4,                			        ///< video encoder ID 4
	DAL_VDOENC_ID_5,                			        ///< video encoder ID 5
	DAL_VDOENC_ID_6,                			        ///< video encoder ID 6
	DAL_VDOENC_ID_7,                			        ///< video encoder ID 7
	DAL_VDOENC_ID_8,                			        ///< video encoder ID 8
	DAL_VDOENC_ID_9,                			        ///< video encoder ID 9
	DAL_VDOENC_ID_10,               			        ///< video encoder ID 10
	DAL_VDOENC_ID_11,               			        ///< video encoder ID 11
	DAL_VDOENC_ID_12,               			        ///< video encoder ID 12
	DAL_VDOENC_ID_13,               			        ///< video encoder ID 13
	DAL_VDOENC_ID_14,               			        ///< video encoder ID 14
	DAL_VDOENC_ID_15,               			        ///< video encoder ID 15
	DAL_VDOENC_ID_16,               			        ///< video encoder ID 16
	DAL_VDOENC_ID_MAX,              			        ///< video encoder ID maximum
	ENUM_DUMMY4WORD(DAL_VDOENC_ID)
} DAL_VDOENC_ID;

/**
    Video Display Aspect Ratio
*/
typedef enum {
	DAL_VDOENC_DAR_DEFAULT,         			        ///< default video display aspect ratio (the same as encoded image)
	DAL_VDOENC_DAR_16_9,            			        ///< 16:9 video display aspect ratio
	DAL_VDOENC_DAR_MAX,             			        ///< maximum video display aspect ratio
	ENUM_DUMMY4WORD(DAL_VDOENC_DAR)
} DAL_VDOENC_DAR;

/**
    Video Encoding Path
*/
typedef enum {
	DAL_VDOENC_PATH_D2D,            			        ///< encoding path: D2D
	DAL_VDOENC_PATH_IMEDIRECT,      			        ///< encoding path: IME/Direct
	DAL_VDOENC_PATH_ALLDIRECT,      			        ///< encoding path: All direct
	DAL_VDOENC_PATH_MAX,            			        ///< maximum encoding path value
	ENUM_DUMMY4WORD(DAL_VDOENC_PATH)
} DAL_VDOENC_PATH;

/**
    Video Encoding Profile
*/
typedef enum {
	DAL_VDOENC_PROFILE_BASELINE,
	DAL_VDOENC_PROFILE_MAIN,
	DAL_VDOENC_PROFILE_HIGH,
	DAL_VDOENC_PROFILE_MAX,
	ENUM_DUMMY4WORD(DAL_VDOENC_PROFILE)
} DAL_VDOENC_PROFILE;

/**
    Video Encoding entropy method
*/
typedef enum {
	DAL_VDOENC_CAVLC = 0,
	DAL_VDOENC_CABAC,
	ENUM_DUMMY4WORD(DAL_VDOENC_ENTROPY)
}DAL_VDOENC_ENTROPY;

/**
    SVC settings
*/
typedef enum {
	DAL_VDOENC_SVC_DISABLE,         			        ///< disable SVC
	DAL_VDOENC_SVC_LAYER1,          			        ///< enable SVC with GOP number 15
	DAL_VDOENC_SVC_LAYER2,          			        ///< enable SVC with GOP number 15
	DAL_VDOENC_SVC_MAX,             			        ///< SVC setting maximum
	ENUM_DUMMY4WORD(DAL_VDOENC_SVC)
} DAL_VDOENC_SVC;

/**
    type of getting info from video encoder
*/
typedef enum {
	DAL_VDOENC_GET_MEM_SIZE,							///< get codec needed size
	DAL_VDOENC_GET_DESC, 								///< get header description
	DAL_VDOENC_GET_ISIFRAME, 							///< check whether is I frame
	DAL_VDOENC_GET_GOPNUM, 								///< get gop number
	DAL_VDOENC_GET_CBR, 								///< get CBR info
	DAL_VDOENC_GET_EVBR, 								///< get EVBR info
	DAL_VDOENC_GET_VBR, 								///< get VBR info
	DAL_VDOENC_GET_FIXQP, 								///< get FIXQP info
	DAL_VDOENC_GET_AQ, 									///< get AQ info
	DAL_VDOENC_GET_RC, 									///< get RC info
	DAL_VDOENC_GET_3DNR, 								///< get 3DNR info
	DAL_VDOENC_GET_GDR, 								///< get GDR info
	ENUM_DUMMY4WORD(DAL_VDOENC_GET_ITEM)
} DAL_VDOENC_GET_ITEM;

/**
    type of setting info to video encoder
*/
typedef enum {
	DAL_VDOENC_SET_CBR, 								///< set CBR info
	DAL_VDOENC_SET_EVBR, 								///< set EVBR info
	DAL_VDOENC_SET_VBR, 								///< set VBR info
	DAL_VDOENC_SET_FIXQP, 								///< set FIXQP info
	DAL_VDOENC_SET_ROWRC, 								///< set ROWRC info
	DAL_VDOENC_SET_QPMAP, 								///< set QP Map
	DAL_VDOENC_SET_AQ, 									///< set AQ info
	DAL_VDOENC_SET_3DNR,								///< set 3DNR info
	DAL_VDOENC_SET_3DNRCB, 								///< set 3DNR setting callback
	DAL_VDOENC_SET_ROI, 								///< set ROI info
	DAL_VDOENC_SET_SMART_ROI, 							///< set Smart ROI info (from OD)
	DAL_VDOENC_SET_MD, 									///< set MD info
	DAL_VDOENC_SET_SLICESPLIT,							///< set Slice Split info
	DAL_VDOENC_SET_GDR,									///< set GDR
	DAL_VDOENC_SET_RESET_IFRAME, 						///< reset I frame
	DAL_VDOENC_SET_DUMPINFO, 							///< enable/disable dump info (e.x. brc ...)
	DAL_VDOENC_SET_RESET, 								///< reset settings when encoding h.264/h.265 simultaneously
	ENUM_DUMMY4WORD(DAL_VDOENC_SET_ITEM)
} DAL_VDOENC_SET_ITEM;

typedef enum {
	DAL_VDOENC_ROTATE_DISABLE,				            ///< no rotate
	DAL_VDOENC_ROTATE_CCW, 					            ///< rotate counter clockwise
	DAL_VDOENC_ROTATE_CW, 					            ///< rotate clockwise
	ENUM_DUMMY4WORD(DAL_VDOENC_ROTATE_TYPE)
} DAL_VDOENC_ROTATE_TYPE;

typedef struct {
	INT32  nr_3d_mode;                      // 0 = nr off, 1 = nr on
	INT32  tnr_osd_mode;
	INT32  mctf_p2p_pixel_blending;
	INT32  tnr_p2p_sad_mode;
	INT32  tnr_mctf_sad_mode;
	INT32  tnr_mctf_bias_mode;
	INT32  nr_3d_adp_th_p2p[3];
	INT32  nr_3d_adp_weight_p2p[3];
	INT32  tnr_p2p_border_check_th;
	INT32  tnr_p2p_border_check_sc;
	INT32  tnr_p2p_input;
	INT32  tnr_p2p_mctf_motion_th;
	INT32  tnr_p2p_mctf_motion_wt[4];
	INT32  nr3d_temporal_spatial_y[3];
	INT32  nr3d_temporal_spatial_c[3];
	INT32  nr3d_temporal_range_y[3];
	INT32  nr3d_temporal_range_c[3];
	INT32  nr3d_clampy_th;
	INT32  nr3d_clampy_div;
	INT32  nr3d_clampc_th;
	INT32  nr3d_clampc_div;
	INT32  nr3d_temporal_spatial_y_mctf[3];
	INT32  nr3d_temporal_spatial_c_mctf[3];
	INT32  nr3d_temporal_range_y_mctf[3];
	INT32  nr3d_temporal_range_c_mctf[3];
	INT32  nr3d_clampy_th_mctf;
	INT32  nr3d_clampy_div_mctf;
	INT32  nr3d_clampc_th_mctf;
	INT32  nr3d_clampc_div_mctf;
	INT32  nr3d_temporal_spatial_y_mctf_1[3];
	INT32  nr3d_temporal_spatial_c_mctf_1[3];
	INT32  nr3d_temporal_spatial_y_1[3];
	INT32  nr3d_temporal_spatial_c_1[3];
	INT32  nr3d_twr_in_key_frame_en;
} DAL_VDOENC_3DNR_INFO;

typedef struct {
	void (*VdoEnc_3DNRDo)(UINT32 pathID, UINT32 config);
	UINT32 id;
} DAL_VDOENC_3DNRCB;

typedef struct {
	INT32  iAqEnable;                   	            ///< 0 = aq off, 1 = aq on
	UINT32 uiIStr;                     	            	///< I aq strength: 1~8
	UINT32 uiPStr;										///< P aq strength: 1~8
	INT32  iSraqInitAslog2;             	            ///< initial i_ASlog2 for sraq: 0~63 (default: 36)
	INT32  iMaxSraq;                    	            ///< max ssim aq: 0~ 8 (default:  8)
	INT32  iMinSraq;                    	            ///< min ssim aq: 0~-8 (default: -8)
	INT32  iSraqConstAslog2;				            ///< 0:update aslog2 by each frame, 1:use iSraqInitAslog2 for each frame, default: 0
} DAL_VDOENC_AQ_INFO;

typedef struct {
	UINT32 uiAddr;               				        ///< Buf Addr
	UINT32 uiSize;               				        ///< Buf Size
} DAL_VDOENC_BUF;

typedef struct {
	UINT32 uiEnable;
	UINT32 uiStaticTime;
	UINT32 uiByteRate;
	UINT32 uiFrameRate;
	UINT32 uiGOP;
	UINT32 uiInitIQp;									///< value: 0 - 51
	UINT32 uiMinIQp;									///< value: 0 - 51
	UINT32 uiMaxIQp;									///< value: 0 - 51
	UINT32 uiInitPQp;									///< value: 0 - 51
	UINT32 uiMinPQp;									///< value: 0 - 51
	UINT32 uiMaxPQp;									///< value: 0 - 51
	INT32  iIPWeight;									///< value: -100 - 100
	UINT32 uiRowRcEnalbe;
	UINT32 uiRowRcQpRange;
	UINT32 uiRowRcQpStep;
} DAL_VDOENC_CBR_INFO;

typedef struct {
	UINT32 uiEnable;
	UINT32 uiStaticTime;
	UINT32 uiByteRate;
	UINT32 uiFrameRate;
	UINT32 uiGOP;
    UINT32 uiKeyPPeriod;
	UINT32 uiInitIQp;
	UINT32 uiMinIQp;
	UINT32 uiMaxIQp;
	UINT32 uiInitPQp;
	UINT32 uiMinPQp;
	UINT32 uiMaxPQp;
	INT32  iIPWeight;
    INT32  iKeyPWeight;
    INT32  iMotionAQStrength;
	UINT32 uiRowRcEnalbe;
	UINT32 uiRowRcQpRange;
	UINT32 uiRowRcQpStep;
    UINT32 uiStillFrameCnd;
    UINT32 uiMotionRatioThd;
	UINT32 uiIPsnrCnd;
	UINT32 uiPPsnrCnd;
	UINT32 uiKeyPPsnrCnd;
} DAL_VDOENC_EVBR_INFO;

typedef struct {
	UINT32 uiByteRate;  					            ///< no use, only sync prj settings
	UINT32 uiFrameRate;						            ///< no use, only sync prj settings
	UINT32 uiGOP;							            ///< no use, only sync prj settings
	UINT32 uiEnable;
	UINT32 uiIFixQP;
	UINT32 uiPFixQP;
} DAL_VDOENC_FIXQP_INFO;

typedef struct {
	UINT32 uiLTRInterval;   				            ///< long-term reference frame interval. range : 0 ~ 4095
	UINT32 uiLTRPreRef;    					            ///< long-term reference setting. 0 : all long-term reference to IDR frame, 1: reference latest long-term reference frame.
} DAL_VDOENC_LTR_INFO;

typedef struct {
	UINT32 uiMbWidthNum;
	UINT32 uiMbHeightNum;
	UINT32 uiMbWidth;
	UINT32 uiMbHeight;
	UINT8  *pMDBitMap;  					            ///< size: UINT8*8160
} DAL_VDOENC_MD_INFO;

typedef struct {
	UINT32 uiWidth;
	UINT32 uiHeight;
	UINT32 uiSVCLayer;
	UINT32 uiLTRInterval;
	UINT32 uiSize;									///< return mem size
} DAL_VDOENC_MEM_INFO;

typedef struct {
	UINT32 uiEnable;
	UINT8  *pQpMap;
	UINT32	qp_map_size;		///<[w] [520]user qp map size
	UINT32	qp_map_loft;		///<[w] [520]user qp map lineoffset
} DAL_VDOENC_QPMAP_INFO;

typedef struct {
	UINT32 uiSliceType;
	UINT32 uiFrameRate;
	UINT32 uiGOPSize;
	UINT32 uiQp;
	UINT32 uiStaticFrame;
	UINT32 uiFrameByte;
    UINT32 uiMotionRatio;
    UINT32 uiEVBRState;
} DAL_VDOENC_RC_INFO;

typedef struct {
	BOOL uiEnable;
	UINT8 uiQPRange;
	UINT8 uiQPStep;
	// [520]	//
	UINT8           p_qp_range;         ///< [r/w] qp range of P frame for row-level rata control. default: 4, range: 0~15
	UINT8           p_qp_step;          ///< [r/w] qp step  of P frame for row-level rata control. default: 1, range: 0~15
	UINT8           min_i_qp;           ///< [r/w] min qp of I frame for row-level rata control. default:  1, range: 0~51
	UINT8           max_i_qp;           ///< [r/w] max qp of I frame for row-level rata control. default: 51, range: 0~51
	UINT8           min_p_qp;           ///< [r/w] min qp of P frame for row-level rata control. default:  1, range: 0~51
	UINT8           max_p_qp;           ///< [r/w] max qp of P frame for row-level rata control. default: 51, range: 0~51
	UINT8			i_pred_wt;			///< [r/w] [520]predict weight of I frame for row-level rata control. (TODO:default: 51, range: 0~51)
	UINT8			p_pred_wt;			///< [r/w] [520]predict weight of P frame for row-level rata control. (TODO:default: 51, range: 0~51)
} DAL_VDOENC_ROWRC_INFO;

typedef struct {
	BOOL uiEnable;
	INT8 iQP;											///< fixed: 0 ~ 51, delta: -32 ~ 31
	UINT16 uiCoord_X;
	UINT16 uiCoord_Y;
	UINT16 uiWidth;
	UINT16 uiHeight;
	UINT8			qp_mode;			///< [r/w] [520]qp mode , TODO
} _DAL_VDOENC_ROI_INFO;

typedef struct {
	_DAL_VDOENC_ROI_INFO roi[DAL_VDOENC_ROI_NUM_MAX];
	UINT32 uiDeltaQp;									///< 0 = all fixed qp, 1 = all delta qp
	UINT32 uiRoiCount;
} DAL_VDOENC_ROI_INFO;

typedef struct {
	UINT32  uiSarW;
	UINT32  uiSarH;
} DAL_VDOENC_SAR;

typedef struct {
	UINT32 uiClass;
	UINT32 uiScore;
	UINT32 uiCoord_X;
	UINT32 uiCoord_Y;
	UINT32 uiWidth;
	UINT32 uiHeight;
} _DAL_VDOENC_SMART_ROI_INFO;

typedef struct {
	UINT32 uiRoiNum;
	UINT32 uiRange;
    UINT64 TimeStamp;
	_DAL_VDOENC_SMART_ROI_INFO RoiInfo[DAL_VDOENC_ROI_NUM_MAX];
} DAL_VDOENC_SMART_ROI_INFO;

typedef struct {
	BOOL uiEnable;        								///< enable multiple slice , range : 0 ~ 1 //
	UINT32 uiSliceRowNum;   							///< number of macroblock rows occupied by a slice , range : 1 ~ SIZE16X(picture_height)
} DAL_VDOENC_SLICESPLIT_INFO;

typedef struct {
	BOOL uiEnable;
	UINT32 uiPeriod;
	UINT32 uiRowNumber;
} DAL_VDOENC_GDR_INFO;

typedef struct {
	UINT32 uiVirSrcDecSideInfoAddr;
	UINT32 uiSrcDecktable0;
	UINT32 uiSrcDecktable1;
	UINT32 uiSrcDecktable2;
	UINT32 uiSrcDecStripNum;
	UINT32 uiSrcDecStrip01Size;
	UINT32 uiSrcDecStrip23Size;
} DAL_VDOENC_SRC_COMPRESS_INFO;

typedef struct {
	UINT32 uiEnable;
	UINT32 uiStaticTime;
	UINT32 uiByteRate;
	UINT32 uiFrameRate;
	UINT32 uiGOP;
	UINT32 uiInitIQp;
	UINT32 uiMinIQp;
	UINT32 uiMaxIQp;
	UINT32 uiInitPQp;
	UINT32 uiMinPQp;
	UINT32 uiMaxPQp;
	INT32  iIPWeight;
	UINT32 uiChangePos;
	UINT32 uiRowRcEnalbe;
	UINT32 uiRowRcQpRange;
	UINT32 uiRowRcQpStep;
} DAL_VDOENC_VBR_INFO;

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
	DAL_VDOENC_SRC_COMPRESS_INFO SrcCompressInfo;		///< Source compression Info
	UINT32  uiMdAddr;									///< Md address
	UINT32  uiProcessTime;                  			///< Process time for debugging
	UINT32	uiThumbnail;								///< 1: thumbnail
	UINT32	uiEncoded;									///< has been encoded or not
	UINT32  uiRawCount;                                 ///< SIE frame count
	UINT32  uiInputTime;                                ///< YUV input timestamp
	INT32   iInputTimeout;                              ///< YUV input process timeout
} DAL_VDOENC_YUV_SRC;

/**
    Init structure
*/
typedef struct {
	UINT32 uiEncAddr;                                 	///< codec addr
	UINT32 uiEncSize;                                   ///< codec size
	UINT32 uiWidth;                                     ///< frame uiWidth
	UINT32 uiHeight;                                    ///< frame uiHeight
	UINT32 uiByteRate;                                	///< byte rate
	UINT32 uiFrameRate;                                 ///< frame rate
	UINT32 uiGop;										///< GOP
	UINT32 uiInitIQp;                                   ///< I Qp initial value
	UINT32 uiMinIQp;                                    ///< I Qp min value
	UINT32 uiMaxIQp;                                    ///< I Qp max value
	UINT32 uiInitPQp;                                   ///< P Qp initial value
	UINT32 uiMinPQp;                                    ///< P Qp min value
	UINT32 uiMaxPQp;                                    ///< P Qp max value
	UINT32 uiUsrQp; 								    ///< use user Qp (0:default, 1:enable)
	UINT32 uiStaticTime;       							///< rc static time (unit: sec)
	INT32  iIPWeight;									///< rc I/P weight
	UINT32 uiRowRcEnalbe;								///< rowrc enable
	UINT32 uiRowRcQpRange;								///< rowrc Qp range
	UINT32 uiRowRcQpStep;								///< rowrc Qp step
	DAL_VDOENC_DAR uiDAR;                               ///< video display aspect ratio
	DAL_VDOENC_SVC uiSVC;                               ///< SVC Layer
	UINT32 uiLTRInterval;      							///< long-term reference frame interval (range: 0 ~ 4095)
	UINT32 uiLTRPreRef;                                 ///< long-term reference setting (0:all long-term reference to IDR frame, 1:reference latest long-term reference frame)
	UINT32 uiRotate;                                    ///< rotate (0:disable, 1:CCW, 2:CW)
	UINT32 uiYuvFormat;                                 ///< YUV format for jpeg
	UINT32 uiFastSearch; 								///< fast Search (0:small range, 1:big range)
	DAL_VDOENC_PROFILE uiProfile;						///< profile
	UINT32 uiMultiTLayer;								///< multi temporary layer
	UINT32 uiColorRange; 								///< color range (0:tv range (15-235), 1:full range (0-255))
	UINT32 uiPrjMode;									///< prj mode for brc (0:IPCam, 1:DVCam)
	UINT32 uiSEIIdfEn;									///< add SEI data (0: disable, 1:enable)

	DAL_VDOENC_ENTROPY eEntropy;						///< entropy mode (0 : cavlc (H264 only), 1 : cabac)
	UINT8  ucLevelIdc;									///< level idc
	UINT32 uiSarWidth;									///< Sample Aspect Ratio width, ex: 1920 x 1080, width = 1920
	UINT32 uiSarHeight;									///< Sample Aspect Ratio height, ex: 1920 x 1080, height = 1080 (not 1088)
	BOOL   bGrayEn;										///< encode gray enable (0 : disable , 1 : enable)
	UINT8  ucDisableDB;									///< disable_filter_idc (0=Filter, 1=No Filter, 2=Slice Mode)
	INT8   cDBAlpha; 		  							///< deblocking Alpha & C0 offset div. 2, {-6, -5, ... 0, +1, .. +6}
	INT8   cDBBeta;			  							///< deblocking Beta offset div. 2, {-6, -5, ... 0, +1, .. +6}
	// VUI //
	BOOL   bVUIEn;										///< encode VUI header(0:disable, 1 : enable)
	UINT8  ucMatrixCoef;								///< matrix_coef of VUI header
	UINT8  ucTransferChar;								///< transfer_characteristics of VUI header
	UINT8  ucColorPrimaries;							///< colour_primaries of VUI header
	UINT8  ucVideoFormat;								///< video_format of VUI header
	BOOL   bTimingPresetFlag;							///< time_present_flag of VUI header, if frame_rate = 0, default disable time_present_flag
	// 680
	UINT32 uiRecCompressEn;								///< [NOT USE, PLEASE USE bFBCEn]H26x reconstruct compression (ECLS)
	// [520] //
	BOOL bFBCEn;										///< reconstruct compression control (0:disalbe, 1:enable)
	INT8 chrm_qp_idx;
	INT8 sec_chrm_qp_idx;
	BOOL hw_padding_en;
} DAL_VDOENC_INIT;

/**
    Encode structure
*/
typedef struct {
	UINT32 uiYAddr;                                     ///< encoding y address
	UINT32 uiUVAddr;                                    ///< encoding cb address
	UINT32 uiYLineOffset;                               ///< Y line offset
	UINT32 uiUVLineOffset;                              ///< UV line offset
	UINT32 uiBsStartAddr;                               ///< output bit-stream start address if enc space not enough
	UINT32 uiBsAddr;                                    ///< 1st output bit-stream address
	UINT32 uiBsSize;                                    ///< 1st output bit-stream size
	UINT32 uiBsAddr2;                                   ///< 2nd output bit-stream address
	UINT32 uiBsSize2;                                   ///< 2nd output bit-stream size
	UINT32 uiBsEndAddr;                                 ///< output bit-stream end address
	UINT32 uiSrcOutEnable;                              ///< Enable/disable src output
	UINT32 uiSrcOutYAddr;                            	///< src output Y address
	UINT32 uiSrcOutUVAddr;                              ///< src output UV address
	UINT32 uiSrcOutYLoft;                            	///< src output Y line offset
	UINT32 uiSrcOutUVLoft;                              ///< src output UV line offset
	UINT32 uiBsShift;                                   ///< I frame need VPS(not 4-byte align), BS should shift
	UINT32 uiSrcCompression;                            ///< src compression
	DAL_VDOENC_SRC_COMPRESS_INFO SrcCompressInfo;   	///< src compression info
	UINT32 uiQuality;                                   ///< initial quality value for jpeg
	UINT32 uiEncYaddr;                                  ///< RAW Yaddr
	UINT32 uiSVCSize;                                  	///< SVC header size
	UINT32 uiTemporalId;                                ///< SVC temporal layer ID (0, 1, 2)
	UINT32 uiFrameType; 								///< (0:P, 1:B, 2:I, 3:IDR, 4:KP)
	UINT32 uiReEncode;									///< output: re-encode or not
	UINT32 uiNextFrameType; 							///< (0:P, 1:B, 2:I, 3:IDR, 4:KP)
	UINT32 uiBaseQP;                                    ///< BaseQP
	UINT32 uiRestartInterval;                           ///< JPEG restart interval. default: 0, range: 0~65536
	UINT32 uiHwEncTime;                                 ///< HW encode time(unit: us)
} DAL_VDOENC_PARAM;

#endif
