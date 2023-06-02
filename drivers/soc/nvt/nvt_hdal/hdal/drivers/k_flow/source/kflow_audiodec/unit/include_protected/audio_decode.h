/**
    Header file of audio decoding codec library

    Exported header file of audio decoding codec library.

    @file       AudioDecode.h
    @ingroup    mIAVCODEC
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _AUDIODECODE_H
#define _AUDIODECODE_H

#ifdef __KERNEL__
#include <kwrap/type.h>
#define _SECTION(sec)
#else
#include <kwrap/type.h>
#define _SECTION(sec)
#endif
/**
    @addtogroup mIAVCODEC
*/
//@{

/**
    @name Audio pseudo play definition

    Definition of audio decode.

    @note For MediaPseudoAudioPlayTsk().
*/
//@{
#define ZERO_PAD_SAMPLERATE 8000       ///< Sample rate for pseudo audio play
#define ZERO_PAD_CHUNKSIZE  16000      ///< Data chunk size for pseudo audio play
#define ZERO_PAD_BLOCK      5          ///< Data block for pseudo audio play
#define ZERO_PAD_SEC        10         ///< Zero block pad seconds
//@}

/**
    Audio Encoder ID
*/
typedef enum {
	MP_AUDDEC_ID_1,                			///< audio decoder ID 1
	MP_AUDDEC_ID_2,                			///< audio decoder ID 2
	MP_AUDDEC_ID_3,                			///< audio decoder ID 3
	MP_AUDDEC_ID_4,                			///< audio decoder ID 4
	MP_AUDDEC_ID_5,                			///< audio decoder ID 5
	MP_AUDDEC_ID_6,                			///< audio decoder ID 6
	MP_AUDDEC_ID_7,                			///< audio decoder ID 7
	MP_AUDDEC_ID_8,                			///< audio decoder ID 8
	MP_AUDDEC_ID_9,                			///< audio decoder ID 9
	MP_AUDDEC_ID_10,               			///< audio decoder ID 10
	MP_AUDDEC_ID_11,               			///< audio decoder ID 11
	MP_AUDDEC_ID_12,               			///< audio decoder ID 12
	MP_AUDDEC_ID_13,               			///< audio decoder ID 13
	MP_AUDDEC_ID_14,               			///< audio decoder ID 14
	MP_AUDDEC_ID_15,               			///< audio decoder ID 15
	MP_AUDDEC_ID_16,               			///< audio decoder ID 16
	MP_AUDDEC_ID_MAX,              			///< audio decoder ID maximum
	ENUM_DUMMY4WORD(MP_AUDDEC_ID)
} MP_AUDDEC_ID;

/**
    Type defintion struture of getting audio library information

    Type defintion struture of getting information from audio decoding codec library.

    @note format: ER (*GetInfo)(AUDIDEC_GETINFO_TYPE type, UINT32 *pparam1, UINT32 *pparam2, UINT32 *pparam3);
*/
typedef enum {
	MP_AUDDEC_GETINFO_NOWOUTRAWADDR   = 0x1,  ///< p1:now addr of output audio RAW data(out)
	MP_AUDDEC_GETINFO_SECOND2FN       = 0x2,  ///< p1:sec(in), p2:audioByteOffset to sec(in), p3:related audio frame number(out)
	MP_AUDDEC_GETINFO_RAWSTARTADDR    = 0x3,  ///< p1:starting address of raw data(out)
	//#NT#2012/02/08#Hideo Lin -begin
	//#NT#Add the parameter for getting audio frame number in next second start
	MP_AUDDEC_GETINFO_NEXTSECOND2FN   = 0x4,  ///< p1:current audio frame number(in), p2:audio frame number for next second start(out)
	//#NT#2012/02/08#Hideo Lin -end
	ENUM_DUMMY4WORD(MP_AUDDEC_GETINFO_TYPE)
} MP_AUDDEC_GETINFO_TYPE;

/**
    Type defintion struture of setting audio library information

    Type defintion struture of setting information to audio decoding codec library.

    @note format: ER (*SetInfo)(UINT32 type, UINT32 param1, UINT32 param2, UINT32 param3);
*/
typedef enum {
	MP_AUDDEC_SETINFO_USEDADDR   = 0x1,        ///< p1:address to use(in), p2:size(in)
	MP_AUDDEC_SETINFO_RESETRAW   = 0x2,        ///< reset raw to starting
	MP_AUDDEC_SETINFO_FAKEDECODE = 0x3,        ///< start fake decoding
	MP_AUDDEC_SETINFO_REALDECODE = 0x4,        ///< start real decoding
	//#NT#2012/12/18#Calvin Chang#Shift the start position of audio raw data out address -begin
	MP_AUDDEC_SETINFO_USEDADDR_SHIFT = 0x05,   ///< p1: byte shift of start address to use(in)
	//#NT#2012/12/18#Calvin Chang -end
	//#NT#2014/03/14#Calvin Chang#Raw Data Buffer handle by application/project -begin
	MP_AUDDEC_SETINFO_OUTPUTRAWMODE  = 0x06,   ///< p1:enable/disable output RAW mode
	MP_AUDDEC_SETINFO_RAWDATAADDR    = 0x07,   ///< p1:RAW data buffer addr(in)
	MP_AUDDEC_SETINFO_ADTS_EN   = 0x08,        ///< p1:enable/disable adts header
	//#NT#2014/03/14#Calvin Chang -end
	ENUM_DUMMY4WORD(MP_AUDDEC_SETINFO_TYPE)
} MP_AUDDEC_SETINFO_TYPE;

/**
    Type defintion struture of audio playing information.

    Type defintion struture of audio playing information from audio decoding codec library.

    @note for initialize() function
*/
typedef struct {
	UINT32 sampleRate;                        ///< [in] Audio sample rate
	UINT32 channels;                          ///< [in] Audio channels
	UINT32 filesize;                          ///< [in] Filesize
	UINT32 chunksize;                         ///< [out] Chunksize, in bytes
} AUDIO_PLAYINFO;

/**
    Type defintion struture of audio output raw data.

    Type defintion struture of audio output raw data buffer and size.

    @note for WaitDecodeDone() function
*/
typedef struct {
	UINT32 outRawAddr;                        ///< [out] Audio raw address
	UINT32 outRawSize;                        ///< [out] Audio raw data (in bytes)
} AUDIO_OUTRAW;


/*
    Type defintion struture of audio decoder object

    Type defintion struture of audio decoder object functions
*/
typedef struct {
	ER(*Init)(AUDIO_PLAYINFO *pobj);                                       ///< initialize decoder
	ER(*GetInfo)(MP_AUDDEC_GETINFO_TYPE type, UINT32 *pparam1, UINT32 *pparam2, UINT32 *pparam3);  ///< get information from decoder
	ER(*SetInfo)(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);        ///< set information to decoder
	ER(*DecodeOne)(UINT32 type, UINT32 BsAddr, UINT32 BsSize);                          ///< decode one frame
	ER(*WaitDecodeDone)(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);                         ///< wait decoding finished, p1: E_OK or not (out), p2:(AUDIO_OUTRAW *) object(out)
	ER(*CustomizeFunc)(UINT32 type, void *pobj);                                                  ///< customize function
	UINT32 checkID;                                                                               ///< check id of decoder
} MP_AUDDEC_DECODER, *PMP_AUDDEC_DECODER;

/*
    Public funtions to get audio decoding object

    Public funtions to get audio decoding object.
*/
//@{
//extern PAUDIO_DECODER MP_PCMDec_getAudioDecodeObject(void);
//extern PAUDIO_DECODER MP_AACDec_getAudioDecodeObject(void);
//extern PAUDIO_DECODER MP_ZeroDec_getAudioDecodeObject(void);
//@}

//@}

#endif //_AUDIODECODE_H
