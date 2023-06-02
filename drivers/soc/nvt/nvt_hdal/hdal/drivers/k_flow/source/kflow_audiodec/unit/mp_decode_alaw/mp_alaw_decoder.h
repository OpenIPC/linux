/*
    @file       mp_alaw_decoder.h

    @brief      header file of aLaw decoder
    @version    V1.00.000
    @date       2018/09/26

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _MP_ALAWDECODER_H
#define _MP_ALAWDECODER_H

#include "kwrap/type.h"
#include "audio_decode.h"

typedef struct {
	UINT32 startAddr;
	UINT32 endAddr;
	UINT32 maxAddr;
	UINT32 nowAddr; //addr of raw output
	UINT32 usedAddr;//addr of getting from audio
	UINT32 outputRawSize;//start from usedAddr, unused audio size
	UINT32 maxRawSize;//max unused audio size //2010/08/03 Meg Lin

} ALAW_ADDRINFO;

extern ER MP_aLawDec_init(MP_AUDDEC_ID AudDecId, AUDIO_PLAYINFO *pobj);
extern ER MP_aLawDec_getInfo(MP_AUDDEC_ID AudDecId, MP_AUDDEC_GETINFO_TYPE type, UINT32 *pparam1, UINT32 *pparam2, UINT32 *pparam3);
extern ER MP_aLawDec_setInfo(MP_AUDDEC_ID AudDecId, MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawDec_decodeOne(MP_AUDDEC_ID AudDecId, UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_aLawDec_waitDecodeDone(MP_AUDDEC_ID AudDecId, UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

#endif //_MP_ALAWDECODER_H

