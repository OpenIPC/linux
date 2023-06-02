#ifndef _MP_MJPGENCODER_H
#define _MP_MJPGENCODER_H


#include "kwrap/type.h"
#define _SECTION(sec)


extern ER MP_MjpgEnc_init(MP_VDOENC_ID VidEncId, MP_VDOENC_INIT *pVidEncInit);
extern ER MP_MjpgEnc_close(MP_VDOENC_ID VidEncId);
extern ER MP_MjpgEnc_getInfo(MP_VDOENC_ID VidEncId, MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_MjpgEnc_setInfo(MP_VDOENC_ID VidEncId, MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
extern ER MP_MjpgEnc_encodeOne(MP_VDOENC_ID VidEncId, UINT32 type, MP_VDOENC_PARAM *ptr, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
extern ER MP_MjpgEnc_encodeOne(MP_VDOENC_ID VidEncId, UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *ptr);
#endif

#endif //_MP_MJPGENCODER_H

