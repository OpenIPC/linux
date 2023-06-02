/**
    Header file of audio encoding codec library

    Exported header file of audio encoding codec library.

    @file       AudioEncode.h
    @ingroup    mIAVCODEC
    @note       Nothing.
    @version    V1.01.003
    @date       2010/05/05

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.
*/
#ifndef _AUDIOENCODE_H
#define _AUDIOENCODE_H

#ifdef __KERNEL__
#include "kwrap/type.h"
#else
#include "kwrap/type.h"
#include <string.h>
#endif

/**
    @addtogroup mIAVCODEC
*/
//@{

#define MP_AUDENC_SHOW_MSG 				0

#define MP_AUDENC_AAC_RAW_BLOCK  		1024 	//encode one aac frame needs 1024 samples

#define MP_AUDENC_TYPE_ENCLAST 			1		//enc last, struct = MP_AUDENC_CUSTOM_LAST_INFO//2012/07/09 Meg

/**
    Audio Encoder ID
*/
typedef enum {
	MP_AUDENC_ID_1,                			///< audio encoder ID 1
	MP_AUDENC_ID_2,                			///< audio encoder ID 2
	MP_AUDENC_ID_3,                			///< audio encoder ID 3
	MP_AUDENC_ID_4,                			///< audio encoder ID 4
	MP_AUDENC_ID_5,                			///< audio encoder ID 5
	MP_AUDENC_ID_6,                			///< audio encoder ID 6
	MP_AUDENC_ID_7,                			///< audio encoder ID 7
	MP_AUDENC_ID_8,                			///< audio encoder ID 8
	MP_AUDENC_ID_9,                			///< audio encoder ID 9
	MP_AUDENC_ID_10,               			///< audio encoder ID 10
	MP_AUDENC_ID_11,               			///< audio encoder ID 11
	MP_AUDENC_ID_12,               			///< audio encoder ID 12
	MP_AUDENC_ID_13,               			///< audio encoder ID 13
	MP_AUDENC_ID_14,               			///< audio encoder ID 14
	MP_AUDENC_ID_15,               			///< audio encoder ID 15
	MP_AUDENC_ID_16,               			///< audio encoder ID 16
	MP_AUDENC_ID_MAX,              			///< audio encoder ID maximum
	ENUM_DUMMY4WORD(MP_AUDENC_ID)
} MP_AUDENC_ID;

/**
    type of getting information from audio encoding codec library.
    format: ER (*GetInfo)(MP_AUDENC_GETINFO_TYPE type, UINT32 *pparam1, UINT32 *pparam2, UINT32 *pparam3);
*/
typedef enum {
	MP_AUDENC_GETINFO_PARAM,  				///< aud encoding parmater. p1: MP_AUDENC_PARAM pointer(out)
	MP_AUDENC_GETINFO_BLOCK,  				///< bytes to encode one audio BS. p1: samples per block (out), ex. 1024 samples for AAC
	ENUM_DUMMY4WORD(MP_AUDENC_GETINFO_TYPE)
} MP_AUDENC_GETINFO_TYPE;

/**
    type of setting information to audio encoding codec library.
    format: ER (*SetInfo)(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
*/
typedef enum {
	MP_AUDENC_SETINFO_RAWADDR, 				///< p1: address of input raw data(in)
	MP_AUDENC_SETINFO_SAMPLERATE, 			///< p1: sampleRate(in)
	MP_AUDENC_SETINFO_BITRATE, 				///< p1: audio encoding bit rate(in)
	MP_AUDENC_SETINFO_CHANNEL, 				///< p1: encoding channels (in)
	MP_AUDENC_SETINFO_SAVETYPE, 			///< p1: 1 one sec, 2 half sec
	MP_AUDENC_SETINFO_AAC_SET_CFG,			///< set aac config, no parameters
	MP_AUDENC_SETINFO_AAC_STOP_FREQ, 		///< p1: aac stop frequency
	ENUM_DUMMY4WORD(MP_AUDENC_SETINFO_TYPE)
} MP_AUDENC_SETINFO_TYPE;

/**
    Audio encoding parameters structure for EncodeOne()
*/
typedef struct {
	UINT32 encAddr;    	///< in:codec needed memory
	UINT32 encSize;    	///< in:size for codec memory
	UINT32 rawAddr;    	///< in:encoding audio raw address
	UINT32 sampleRate; 	///< in:input sample rate
	UINT32 bitRate;    	///< in:wanted bit rate (bits per second)
	UINT32 nChannels;  	///< in:input channels
	UINT32 saveType;   	///< in:save type, 1 one sec, 2 half sec
	UINT32 chunksize;  	///< out: size of one second audio raw data (bytes)
	UINT32 thisSize;   	///< out: size of encoding frame (bytes)
	UINT32 usedRawSize;	///< out: size of used raw data (bytes)
	UINT32 encOutSize; 	///< out: if half sec, half chunksize
	UINT32 stopFreq;    ///< in: stop frequency
} MP_AUDENC_PARAM;

typedef struct {
	UINT32 bsOutputAddr; ///< in: bitstream output addr
	UINT32 inputAddr;    ///< in: raw input addr
	UINT32 inputSize;    ///< in: raw input size
	UINT32 thisSize;     ///< out: size of encoding frame (bytes)
	UINT32 usedRawSize;  ///< out: size of used raw data (bytes)
} MP_AUDENC_CUSTOM_LAST_INFO;//2012/07/09 Meg

/**
    Struture of audio encoder
*/
typedef struct {
	ER(*Initialize)(void);   ///< initializing codec
	ER(*GetInfo)(MP_AUDENC_GETINFO_TYPE type, UINT32 *pparam1, UINT32 *pparam2, UINT32 *pparam3);   ///< get information from codec library
	ER(*SetInfo)(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);  ///< set parameters to codec library
	ER(*EncodeOne)(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);  ///< encoding one audio frame
	ER(*CustomizeFunc)(UINT32 type, void *pobj);  ///< codec customize function
	UINT32 checkID;     ///< check ID
} MP_AUDENC_ENCODER, *PMP_AUDENC_ENCODER;

#endif

//@}
