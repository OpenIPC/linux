/*
    @file       mp_alaw_encoder.h

    @brief      header file of aLaw encoder
    @version    V1.00.000
    @date       2018/08/30

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _MP_ALAWENCODER_H
#define _MP_ALAWENCODER_H

#include "kwrap/type.h"
#include "audio_encode.h"

extern ER MP_aLawEnc_init(MP_AUDENC_ID AudEncId);
extern ER MP_aLawEnc_getInfo(MP_AUDENC_ID AudEncId, MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawEnc_setInfo(MP_AUDENC_ID AudEncId, MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawEnc_encodeOne(MP_AUDENC_ID AudEncId, UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

#endif //_MP_ALAWENCODER_H
