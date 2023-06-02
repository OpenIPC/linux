#ifndef _MP_MJPGDECODER_H
#define _MP_MJPGDECODER_H


#include "kwrap/type.h"
#define _SECTION(sec)


extern ER MP_MjpgDec_init(MP_VDODEC_ID VidDecId, MP_VDODEC_INIT *pVidDecInit);
extern ER MP_MjpgDec_close(MP_VDODEC_ID VidDecId);
extern ER MP_MjpgDec_getInfo(MP_VDODEC_ID VidDecId, MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_MjpgDec_setInfo(MP_VDODEC_ID VidDecId, MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
extern ER MP_MjpgDec_decodeOne(MP_VDODEC_ID VidDecId, UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
extern ER MP_MjpgDec_decodeOne(MP_VDODEC_ID VidDecId, UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *ptr);
#endif

#endif //_MP_MJPGDECODER_H

