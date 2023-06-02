#ifndef _MP_H265DECODER_H
#define _MP_H265DECODER_H


#include "kwrap/type.h"
#include "video_decode.h"
#ifdef __KERNEL__
#include <linux/module.h>
#endif

///#include "h265dec_api.h"

extern ER MP_H265Dec_init(MP_VDODEC_ID VidDecId, MP_VDODEC_INIT *pVidDecInit);
extern ER MP_H265Dec_close(MP_VDODEC_ID VidDecId);
extern ER MP_H265Dec_getInfo(MP_VDODEC_ID VidDecId, MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_H265Dec_setInfo(MP_VDODEC_ID VidDecId, MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
extern ER MP_H265Dec_decodeOne(MP_VDODEC_ID VidDecId, UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
extern ER MP_H265Dec_decodeOne(MP_VDODEC_ID VidDecId, UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif
extern ER MP_H265Dec_triggerDec(MP_VDODEC_ID VidDecId, MP_VDODEC_PARAM *pVidDecParam);
extern ER MP_H265Dec_waitDecReady(MP_VDODEC_ID VidDecId, MP_VDODEC_PARAM *pVidDecParam);

#endif //_MP_H265DECODER_H
