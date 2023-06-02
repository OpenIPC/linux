#ifndef _H264ENC_WRAP_H_
#define _H264ENC_WRAP_H_

#include "h264enc_int.h"

//extern void h264Enc_wrapSeqCfg(H26XRegSet *pRegSet, H264ENC_CTX *pVdoCtx);
extern void h264Enc_wrapSeqCfg(H264ENC_CTX *pVdoCtx, H26XEncAddr *pAddr);
extern void h264Enc_wrapPicCfg(H26XRegSet *pRegSet, H264ENC_INFO *pInfo, H26XENC_VAR *pVar);
extern void h264Enc_wrapRdoCfg(H26XRegSet *pRegSet, H264ENC_CTX *pVdoCtx);
extern void h264Enc_wrapFroCfg(H26XRegSet *pRegSet, H264ENC_CTX *pVdoCtx);
extern void h264Enc_wrapRdoCfgUpdate(H26XRegSet *pRegSet, H264EncRdo *pRdo, BOOL bIsIfrm);
extern void h264Enc_setKPRefReg(H26XENC_VAR *pVar, UINT32 uiNaluAddr);
extern void h264Enc_setKPRefBsDma(H26XENC_VAR *pVar, UINT32 uiBsSize);
#endif	//_H264ENC_WRAP_H_

