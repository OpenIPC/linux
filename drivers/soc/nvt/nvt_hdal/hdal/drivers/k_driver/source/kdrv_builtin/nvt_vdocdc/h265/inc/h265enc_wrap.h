#ifndef _H265ENC_WRAP_H_
#define _H265ENC_WRAP_H_

#include "h265enc_int.h"

extern void h265Enc_wrapSeqCfg(H26XRegSet *pRegSet, H265ENC_CTX *pVdoCtx, UINT32 uiPaBsdmaAddr);
extern INT32 h265Enc_wrapPicCfg(H26XRegSet *pRegSet, H265ENC_INFO *pInfo, H26XENC_VAR *pVar);
extern void h265Enc_wrapRdo(H26XRegSet *pRegSet, H265ENC_CTX *pVdoCtx);
extern void h265Enc_wrapRdoCfg(H26XRegSet *pRegSet, H265ENC_CTX *pVdoCtx);
extern void h265Enc_wrapFroCfg(H26XRegSet *pRegSet, H265ENC_CTX *pVdoCtx);
extern void h265Enc_wrapQpRelatedCfg(H26XRegSet *pRegSet, H265EncQpRelatedCfg *pQpCfg);
extern void h265Enc_setKPRefReg(H26XENC_VAR *pVar, UINT32 uiNaluAddr);
extern void h265Enc_setKPRefBsDma(H26XENC_VAR *pVar, UINT32 uiBsSize);


#endif	//_H265ENC_WRAP_H_

