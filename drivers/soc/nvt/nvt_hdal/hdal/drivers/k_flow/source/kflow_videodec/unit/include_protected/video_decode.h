/**
    Header file of video decoding codec library

    Exported header file of video decoding codec library.

    @file       VideoDecode.h
    @ingroup    mIAVCODEC
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _VIDEODECODE_H
#define _VIDEODECODE_H

#include "dal_vdodec.h"  // TODO: use kdrv later
#include "kdrv_videodec/kdrv_videodec.h"

#define MP_VDODEC_SHOW_MSG 				0

/**
    Definition of video pre-decode frame buffer number.
*/
//@{
#define MP_VDODEC_PREDEC_FRMNUM			3			///< Video Pre-decode Frame Number
//@}

typedef void (*MEDIAPLAY_REFFRMCB)(UINT32 pathID, UINT32 uiYAddr, BOOL bIsRef);

/**
    Video Decoder ID
*/
typedef enum {
	MP_VDODEC_ID_1,                			///< video decoder ID 1
	MP_VDODEC_ID_2,                			///< video decoder ID 2
	MP_VDODEC_ID_3,                			///< video decoder ID 3
	MP_VDODEC_ID_4,                			///< video decoder ID 4
	MP_VDODEC_ID_5,                			///< video decoder ID 5
	MP_VDODEC_ID_6,                			///< video decoder ID 6
	MP_VDODEC_ID_7,                			///< video decoder ID 7
	MP_VDODEC_ID_8,                			///< video decoder ID 8
	MP_VDODEC_ID_9,                			///< video decoder ID 9
	MP_VDODEC_ID_10,               			///< video decoder ID 10
	MP_VDODEC_ID_11,               			///< video decoder ID 11
	MP_VDODEC_ID_12,               			///< video decoder ID 12
	MP_VDODEC_ID_13,               			///< video decoder ID 13
	MP_VDODEC_ID_14,               			///< video decoder ID 14
	MP_VDODEC_ID_15,               			///< video decoder ID 15
	MP_VDODEC_ID_16,               			///< video decoder ID 16
	MP_VDODEC_ID_MAX,              			///< video decoder ID maximum
	ENUM_DUMMY4WORD(MP_VDODEC_ID)
} MP_VDODEC_ID;

/**
    type for CustomizeFunc() in MP_VDODEC_DECODER.
*/
//@{
typedef enum {
	MP_VDODEC_CUSTOM_DECODE1ST,						///< p1:(MP_VDODEC_1STV_INFO *) object(out)
	MP_VDODEC_CUSTOM_STREAMINIT,					///< p1:(MP_VDODEC_STREAM_INIT *) object(in)
	MP_VDODEC_CUSTOM_STREAMDECODE,					///< p1:(MP_VDODEC_STREAM_INFO *) object(in)
	MP_VDODEC_CUSTOM_STREAMWAITDONE,				///< p1:(UINT32 *) ER_Code(out)
	ENUM_DUMMY4WORD(MP_VDODEC_CUSTOM_TYPE)
} MP_VDODEC_CUSTOM_TYPE;
//@}

/**
    Video Decoding Slice Type
*/
typedef enum {
	MP_VDODEC_P_SLICE = 0,
	MP_VDODEC_B_SLICE = 1,
	MP_VDODEC_I_SLICE = 2,
	MP_VDODEC_IDR_SLICE = 3,
	MP_VDODEC_KP_SLICE = 4,
	ENUM_DUMMY4WORD(MP_VDODEC_SLICE_TYPE)
} MP_VDODEC_SLICE_TYPE;

/**
    Type of getting information from video decoding codec library.
*/
typedef enum {
	MP_VDODEC_GETINFO_JPEG_FREQ,
	MP_VDODEC_GETINFO_MEM_SIZE,
	MP_VDODEC_GETINFO_VUI_INFO,
	MP_VDODEC_GETINFO_JPEG_INFO,
	MP_VDODEC_GET_H26X_INTERRUPT,
	ENUM_DUMMY4WORD(MP_VDODEC_GETINFO_TYPE)
} MP_VDODEC_GETINFO_TYPE;

/**
    Type of setting information to video decoding codec library.
*/
typedef enum {
	MP_VDODEC_SETINFO_INIT, 						///< set init decode info
	MP_VDODEC_SETINFO_CODEC,						///< set channel codec type
	MP_VDODEC_SETINFO_JPEG_FREQ,						///< set JPEG engine frequency
	MP_VDODEC_SETINFO_CLOSE,						///< set video decode channe close
	MP_VDODEC_SETINFO_NXT_BS,						///< set next bitstream buffer
	ENUM_DUMMY4WORD(MP_VDODEC_SETINFO_TYPE)
} MP_VDODEC_SETINFO_TYPE;

typedef KDRV_VDODEC_REFFRMCB MP_VDODEC_REFFRMCB;

/**
    Type of 1st video frame decode information.
*/
typedef struct {
	UINT32 addr;									///< [in] Video frame addr
	UINT32 size;									///< [in] Video frame size
	UINT32 decodeBuf;								///< [in] Output raw addr
	UINT32 decodeBufSize;							///< [in] Max size for outputing raw
	UINT32 y_Addr;									///< [out] Output Y addr
	UINT32 cb_Addr;									///< [out] Output cb addr
	UINT32 cr_Addr;									///< [out] Output cr addr
	UINT32 width;									///< [out] Video frame width
	UINT32 height;									///< [out] Video frame height
	UINT32 DescAddr;								///< [in] Description addr
	UINT32 DescLen;									///< [in] Description length
	UINT32 jpgType;									///< [out] Jpg type, JPG_FMT_YUV420 or others
	UINT32 decfrmnum;								///< [in] Decode Frame numeber
	UINT32 frmaddr[MP_VDODEC_PREDEC_FRMNUM];		///< [in] Video frames position
	UINT32 frmsize[MP_VDODEC_PREDEC_FRMNUM];		///< [in] Video frames size
} MP_VDODEC_1STV_INFO;

/**
    Type defintion struture of video frame data YCbCr buffer information.
*/
typedef struct {
	UINT32 yAddr;									///< Y address
	UINT32 cbAddr;									///< CB address
	UINT32 crAddr;									///< CR address
} MP_VDODEC_YCBCR_INFO;

/**
    Type defintion struture of video display setting information.
*/
typedef struct {
	UINT32 firstFramePos;							///<[in] First video frame position
	UINT32 firstFrameSize;							///<[in] First video frame size
	UINT32 rawAddr;									///<[in] After decoded, Y addr (cb, cr counted)
	UINT32 rawSize;									///<[in] Size for YCbCr
	UINT32 rawWidth;								///<[in] Raw width
	UINT32 rawHeight;								///<[in] Raw height
	UINT32 rawType;									///<[in] Raw type, (jdcfg.fileformat)
	UINT32 rawCbAddr;								///<[out] After decoded, cb addr
	UINT32 rawCrAddr;								///<[out] After decoded, cr addr
	UINT32 DescAddr;								///<[in] Description addr
	UINT32 DescLen;									///<[in] Description length
	UINT32 jpgType;									///<[in] Jpg type
} MP_VDODEC_DISPLAY_INFO;

/**
	Type defintion struture of video stream decoder init parameter.
*/
typedef struct {
	UINT32 workbuf;									///<[in] workbuf Addr for codec H/W internal use
	UINT32 workbufsize;								///<[in] workbuf size for codec H/W internal use
	UINT32 rawWidth;								///<[in] Raw width
	UINT32 rawHeight;								///<[in] Raw height
	UINT32 DescAddr;								///<[in] Description addr
	UINT32 DescLen;									///<[in] Description length
} MP_VDODEC_STREAM_INIT;

/**
	Type defintion struture of video stream decode parameter.
*/
typedef struct {
	UINT32 bs_addr;									///<[in] bitstream addr
	UINT32 bs_size;									///<[in] bitstream size
	UINT32 y_addr;									///<[out] After decoded, Y addr
	UINT32 uv_addr;									///<[out] After decoded, UV addr
	UINT32 frameType;								///<[in] slice type for current decode frame
} MP_VDODEC_STREAM_INFO;

/**
    Init structure
*/
typedef KDRV_VDODEC_INIT MP_VDODEC_INIT;

/**
    Decode structure
*/
typedef KDRV_VDODEC_PARAM MP_VDODEC_PARAM;

typedef KDRV_VDODEC_VUI_INFO MP_VDODEC_VUI_INFO;

typedef DAL_VDODEC_JPGDECINFO MP_VDODEC_JPGDECINFO;

typedef KDRV_VDODEC_MEM_INFO MP_VDODEC_MEMINFO;

typedef KDRV_VDODEC_RECYUV_WH MP_VDODEC_RECYUV_WH;

#ifdef VDODEC_LL
typedef KDRV_CALLBACK_FUNC MP_VDODEC_CALLBACK_FUNC;

typedef KDRV_VDODEC_LL_MEM_INFO MP_VDODEC_LL_MEM_INFO;

typedef KDRV_VDODEC_LL_MEM MP_VDODEC_LL_MEM;
#endif
/**
	Type defintion struture of video decoder object functions.
*/
/*typedef struct {
	ER(*Initialize)(UINT32 fileAddr, MP_VDODEC_DISPLAY_INFO *pobj);									///< Initialize decoder
	ER(*Close)(void);   																			///< close decoder
	ER(*GetInfo)(MP_VDODEC_GETINFO_TYPE type, UINT32 *pparam1, UINT32 *pparam2, UINT32 *pparam3);	///< Get information from decoder
	ER(*SetInfo)(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);			///< Set information to decoder
	ER(*DecodeOne)(UINT32 type, UINT32 fileAddr, UINT32 p2, UINT32 *p3);							///< Decode one frame, p1: frame addr(in), p2: frame size(in), p3: frame number for H264 (in)
	ER(*WaitDecodeDone)(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);							///< Wait decoding finished, p1: E_OK or not(out)
	ER(*CustomizeFunc)(MP_VDODEC_CUSTOM_TYPE type, void *pobj);										///< customize function, if needed
	UINT32 checkID;																					///< check ID for decoders
} MP_VDODEC_DECODER, *PMP_VDODEC_DECODER;*/
typedef struct {
	ER(*Initialize)(MP_VDODEC_INIT *ptr);  													        ///< initialize decoder
	ER(*Close)(void);   																			///< close decoder
	ER(*GetInfo)(MP_VDODEC_GETINFO_TYPE type, UINT32 *pparam1, UINT32 *pparam2, UINT32 *pparam3);   ///< get info from codec library
	ER(*SetInfo)(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);  		///< set parameters to codec library
#ifdef VDODEC_LL
	ER(*DecodeOne)(UINT32 type, MP_VDODEC_PARAM *ptr, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
	ER(*DecodeOne)(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *ptr);  			///< decoding one video frame and wait ready
#endif
	ER(*TriggerDec)(MP_VDODEC_PARAM *ptr);  														///< trigger decoding one video frame but not wait ready (only for direct path)
	ER(*WaitDecReady)(MP_VDODEC_PARAM *ptr);							                            ///< wait for decoding ready
	ER(*AdjustBPS)(UINT32 type, UINT32 param1, UINT32 param2, UINT32 param3);  						///< Not used
	ER(*CustomizeFunc)(UINT32 type, void *pobj);										            ///< customize function, if needed
	UINT32 checkID;																					///< check ID for decoders
} MP_VDODEC_DECODER, *PMP_VDODEC_DECODER;


//@}

#endif //_VIDEODECODE_H

