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
    Type defintion struture of getting audio library information

    Type defintion struture of getting information from audio decoding codec library.

    @note format: ER (*GetInfo)(AUDIDEC_GETINFO_TYPE type, UINT32 *pparam1, UINT32 *pparam2, UINT32 *pparam3);
*/
typedef enum {
	MEDIAAUD_GETINFO_NOWOUTRAWADDR   = 0x1,  ///< p1:now addr of output audio RAW data(out)
	MEDIAAUD_GETINFO_SECOND2FN       = 0x2,  ///< p1:sec(in), p2:audioByteOffset to sec(in), p3:related audio frame number(out)
	MEDIAAUD_GETINFO_RAWSTARTADDR    = 0x3,  ///< p1:starting address of raw data(out)
	//#NT#2012/02/08#Hideo Lin -begin
	//#NT#Add the parameter for getting audio frame number in next second start
	MEDIAAUD_GETINFO_NEXTSECOND2FN   = 0x4,  ///< p1:current audio frame number(in), p2:audio frame number for next second start(out)
	//#NT#2012/02/08#Hideo Lin -end
	ENUM_DUMMY4WORD(AUDCODEC_GETINFO_TYPE)
} AUDCODEC_GETINFO_TYPE;

/**
    Type defintion struture of setting audio library information

    Type defintion struture of setting information to audio decoding codec library.

    @note format: ER (*SetInfo)(UINT32 type, UINT32 param1, UINT32 param2, UINT32 param3);
*/
typedef enum {
	MEDIAAUD_SETINFO_USEDADDR   = 0x1,        ///< p1:address to use(in), p2:size(in)
	MEDIAAUD_SETINFO_RESETRAW   = 0x2,        ///< reset raw to starting
	MEDIAAUD_SETINFO_FAKEDECODE = 0x3,        ///< start fake decoding
	MEDIAAUD_SETINFO_REALDECODE = 0x4,        ///< start real decoding
	//#NT#2012/12/18#Calvin Chang#Shift the start position of audio raw data out address -begin
	MEDIAAUD_SETINFO_USEDADDR_SHIFT = 0x05,   ///< p1: byte shift of start address to use(in)
	//#NT#2012/12/18#Calvin Chang -end
	//#NT#2014/03/14#Calvin Chang#Raw Data Buffer handle by application/project -begin
	MEDIAAUD_SETINFO_OUTPUTRAWMODE  = 0x06,   ///< p1:enable/disable output RAW mode
	MEDIAAUD_SETINFO_RAWDATAADDR    = 0x07,   ///< p1:RAW data buffer addr(in)
	//#NT#2014/03/14#Calvin Chang -end
	ENUM_DUMMY4WORD(AUDCODEC_SETINFO_TYPE)
} AUDCODEC_SETINFO_TYPE;

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
	ER(*Initialize)(UINT32 fileAddr, AUDIO_PLAYINFO *pobj);                                       ///< initialize decoder
	ER(*GetInfo)(AUDCODEC_GETINFO_TYPE type, UINT32 *pparam1, UINT32 *pparam2, UINT32 *pparam3);  ///< get information from decoder
	ER(*SetInfo)(AUDCODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);        ///< set information to decoder
	ER(*DecodeOne)(UINT32 type, UINT32 fileAddr, UINT32 p2, UINT32 *p3);                          ///< decode one frame
	ER(*WaitDecodeDone)(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);                         ///< wait decoding finished, p1: E_OK or not (out), p2:(AUDIO_OUTRAW *) object(out)
	ER(*CustomizeFunc)(UINT32 type, void *pobj);                                                  ///< customize function
	UINT32 checkID;                                                                               ///< check id of decoder
} AUDIO_DECODER, *PAUDIO_DECODER;

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
