#ifndef _NMEDIARECVDOENCAPI_H
#define _NMEDIARECVDOENCAPI_H

#include "video_encode.h"

extern void             NMR_VdoEnc_GetAQ(UINT32 pathID, MP_VDOENC_AQ_INFO *pAqInfo);
extern void             NMR_VdoEnc_GetCBR(UINT32 pathID, MP_VDOENC_CBR_INFO *pCbrInfo);
extern void             NMR_VdoEnc_GetEVBR(UINT32 pathID, MP_VDOENC_EVBR_INFO *pEVbrInfo);
extern void				NMR_VdoEnc_GetFixQP(UINT32 pathID, MP_VDOENC_FIXQP_INFO *pFixQpInfo);
extern void             NMR_VdoEnc_GetVBR(UINT32 pathID, MP_VDOENC_VBR_INFO *pVbrInfo);
extern void				NMR_VdoEnc_GetRC(UINT32 pathID, MP_VDOENC_RC_INFO *pRcInfo);
extern void				NMR_VdoEnc_GetRCStr(UINT32 pathID, char *Buf, UINT32 BufLen);
extern void             NMR_VdoEnc_SetAQ(UINT32 pathID, MP_VDOENC_AQ_INFO *pAqInfo);
extern void             NMR_VdoEnc_SetCBR(UINT32 pathID, MP_VDOENC_CBR_INFO *pCbrInfo);
extern void             NMR_VdoEnc_SetEVBR(UINT32 pathID, MP_VDOENC_EVBR_INFO *pEVbrInfo);
extern void				NMR_VdoEnc_SetFixQP(UINT32 pathID, MP_VDOENC_FIXQP_INFO *pFixQpInfo);
extern void             NMR_VdoEnc_SetVBR(UINT32 pathID, MP_VDOENC_VBR_INFO *pVbrInfo);
extern void             NMR_VdoEnc_SetRowRc(UINT32 pathID, MP_VDOENC_ROWRC_INFO *pRowRcInfo);
extern void             NMR_VdoEnc_GetBuiltinBsData(UINT32 pathID);

#endif //_NMEDIARECVDOENCAPI_H
