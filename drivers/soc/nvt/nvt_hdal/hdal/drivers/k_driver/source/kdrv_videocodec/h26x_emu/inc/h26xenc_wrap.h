#ifndef _H26XENC_WRAP_H_
#define _H26XENC_WRAP_H_

#include "kwrap/type.h"

#include "h26x.h"

#include "h26xenc_int.h"

extern void h26xEnc_wrapUsrQpCfg(H26XRegSet *pRegSet, H26XUsrQp *pUsrQp);
extern void h26xEnc_wrapPSNRCfg(H26XRegSet *pRegSet, BOOL bEnable);
extern void h26xEnc_wrapSliceSplitCfg(H26XRegSet *pRegSet, H26XEncSliceSplit *pSliceSplit);
extern void h26xEnc_wrapVarCfg(H26XRegSet *pRegSet, H26XEncVarCfg *pVarCfg);
extern void h26xEnc_wrapMaskInitCfg(H26XRegSet *pRegSet, H26XEncMask *pMask);
extern void h26xEnc_wrapMaskWinCfg(H26XRegSet *pRegSet, H26XEncMask *pMask, UINT8 ucIdx);
extern void h26xEnc_wrapGdrCfg(H26XRegSet *pRegSet, H26XEncGdr *pGdr);
extern void h26xEnc_wrapRoiCfg(H26XRegSet *pRegSet, H26XEncRoi *pRoi, UINT8 ucIdx);
extern void h26xEnc_wrapRowRcCfg(H26XRegSet *pRegSet, H26XEncRowRc *pRowRc);
extern void h26xEnc_wrapRowRcPicUpdate(H26XRegSet *pRegSet, H26XEncRowRc *pRowRc, UINT32 uiPicCnt, BOOL bIsIFrm);
extern void h26xEnc_wrapAqCfg(H26XRegSet *pRegSet, H26XEncAq *pAq);
extern void h26xEnc_wrapLpmCfg(H26XRegSet *pRegSet, H26XEncLpm *pLpm);
extern void h26xEnc_wrapRndCfg(H26XRegSet *pRegSet, h26XEncRnd *pRnd);
extern void h26xEnc_wrapScdCfg(H26XRegSet *pRegSet, H26XEncScd *pScd);
extern void h26xEnc_wrapOsgRgbCfg(H26XRegSet *pRegSet, H26XEncOsgRgbCfg *pOsgRgb);
extern void h26xEnc_wrapOsgPalCfg(H26XRegSet *pRegSet, UINT8 ucIdx, H26XEncOsgPalCfg *pOsgPal);
extern void h26xEnc_wrapOsgWinCfg(H26XRegSet *pRegSet, UINT8 ucIdx, H26XEncOsgWinCfg *pOsgWin);
extern void h26xEnc_wrapMotionAqInitCfg(H26XRegSet *pRegSet, H26XEncMotionAq *pMAq);
extern void h26xEnc_wrapMotionAqCfg(H26XRegSet *pRegSet, H26XEncMotionAq *pMAq);
extern void h26xEnc_wrapJndCfg(H26XRegSet *pRegSet, H26XEncJnd *pJnd);
extern void h26xEnc_wrapSdeCfg(H26XRegSet *pRegSet, H26XEncSdec *pSde);
extern void h26xEnc_wrapBgrCfg(H26XRegSet *pRegSet, H26XEncBgr *pBgr);
extern void h26xEnc_wrapRmdCfg(H26XRegSet *pRegSet, H26XEncRmd *pIraMod);
extern void h26xEnc_wrapOsgChromaAlpha(H26XRegSet *pRegSet, UINT8 ucVal);
extern void h26xEnc_wrapTnrCfg(H26XRegSet *pRegSet, H26XEncTnr *pTnr);
extern void h26xEnc_wrapLambdaCfg(H26XRegSet *pRegSet, H26XEncLambda *pLambda);
extern void h26xEnc_wrapEarlySkipCfg(H26XRegSet *pRegSet, H26XEncEarlySkip *pEarlySkip);
extern void h26xEnc_wrapSpnCfg(H26XRegSet *pRegSet, H26XEncSpn *pSpn);

#endif	//_H26XENC_WRAP_H_

