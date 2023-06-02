#ifndef _MP_H265ENCODER_H
#define _MP_H265ENCODER_H


#include "kwrap/type.h"
#include "video_encode.h"

///#include "h265enc_api.h"

extern ER MP_H265Enc_init(MP_VDOENC_ID VidEncId, MP_VDOENC_INIT *pVidEncInit);
extern ER MP_H265Enc_close(MP_VDOENC_ID VidEncId);
extern ER MP_H265Enc_getInfo(MP_VDOENC_ID VidEncId, MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_H265Enc_setInfo(MP_VDOENC_ID VidEncId, MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
extern ER MP_H265Enc_encodeOne(MP_VDOENC_ID VidEncId, UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
extern ER MP_H265Enc_encodeOne(MP_VDOENC_ID VidEncId, UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif
extern ER MP_H265Enc_triggerEnc(MP_VDOENC_ID VidEncId, MP_VDOENC_PARAM *pVidEncParam);
extern ER MP_H265Enc_waitEncReady(MP_VDOENC_ID VidEncId, MP_VDOENC_PARAM *pVidEncParam);

#endif //_MP_H265ENCODER_H
