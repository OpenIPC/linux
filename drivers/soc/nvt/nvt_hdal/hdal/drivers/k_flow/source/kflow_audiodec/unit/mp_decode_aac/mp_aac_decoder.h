/*
    @file       mp_aac_decoder.h

    @brief      header file of aac decoder
    @version    V1.01.001
    @date       2018/09/26

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _MP_AACDECODER_H
#define _MP_AACDECODER_H

#include "kwrap/type.h"
#include "audio_decode.h"

#define AAC_DECODEID        0x74776f73//twos
#define AAC_DECODE_BLOCK    1024

typedef struct {
	UINT32 startAddr;
	UINT32 endAddr;
	UINT32 maxAddr;
	UINT32 nowAddr;     //addr of raw output
	UINT32 usedAddr;    //addr of getting from audio
	UINT32 outputRawSize;//start from usedAddr
	UINT32 maxRawSize;//max output raw audio data //2010/08/03 Meg Lin
} AAC_ADDRINFO;

extern ER MP_AACDec_init(MP_AUDDEC_ID AudDecId, AUDIO_PLAYINFO *pobj);
extern ER MP_AACDec_getInfo(MP_AUDDEC_ID AudDecId, MP_AUDDEC_GETINFO_TYPE type, UINT32 *pparam1, UINT32 *pparam2, UINT32 *pparam3);
extern ER MP_AACDec_setInfo(MP_AUDDEC_ID AudDecId, MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACDec_decodeOne(MP_AUDDEC_ID AudDecId, UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_AACDec_waitDecodeDone(MP_AUDDEC_ID AudDecId, UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
//extern ER MP_AACDec_parseADTS(UINT32 bsAddr, ADTS_HEADER *pHeader);




#endif //_MP_AACDECODER_H

