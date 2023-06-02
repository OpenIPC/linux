#ifndef _IVE_INT_H
#define _IVE_INT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "kdrv_type.h"
#include "ive_reg.h"//header for IVE: reg struct

#define EDGE_COEFF_NUM 9

#define CEIL(a, n) ((((a)+((1<<(n))-1))>>(n))<<(n))

//-----------------------------------------------------------------
/*
    internal function declaration
*/
VOID ive_setResetReg(BOOL bReset);
VOID ive_setStartReg(BOOL bStart);
VOID ive_setIntrEnableReg(UINT32 uiIntrOp);
VOID ive_clrIntrStatusReg(UINT32 uiIntrStatus);
VOID ive_setGenFiltEnReg(BOOL bEn);
VOID ive_setMednFiltEnReg(BOOL bEn);
VOID ive_setMednFiltModeReg(UINT32 uiFiltMode);
VOID ive_setEdgeFiltEnReg(BOOL bEn);
VOID ive_setEdgeFiltModeReg(UINT32 uiFiltMode);
VOID ive_setEdgeAngSlpFactReg(UINT32 uiSlpFact);
VOID ive_setEdgeShiftReg(UINT32 uiShiftBit);
VOID ive_setNonMaxSupEnReg(BOOL bEn);
VOID ive_setThresLutEnReg(BOOL bEn);
VOID ive_setMorphFiltEnReg(BOOL bEn);
VOID ive_setMorphInSelReg(UINT32 uiMorphInSel);
VOID ive_setMorphOpReg(UINT32 uiMorphOp);
VOID ive_setIntegralEnReg(BOOL bEn);
VOID ive_setIntegralFmtReg(UINT32 uiIglFmtSel);
VOID ive_setOutDataReg(UINT32 uiOutDataSel);
VOID ive_setInAddrReg(UINT32 uiInAddr);
VOID ive_setLLAddrReg(UINT32 uiInAddr);
VOID ive_setInLofstReg(UINT32 uiInLofst);
VOID ive_setOutAddrReg(UINT32 uiOutAddr);
VOID ive_setOutLofstReg(UINT32 uiOutLofst);
VOID ive_setInImgWidthReg(UINT32 uiWidth);
VOID ive_setInImgHeightReg(UINT32 uiHeight);
VOID ive_setGenFiltCoeffReg(UINT32 *puiCoeff);
VOID ive_setEdge1CoeffReg(UINT32 *puiEdgeCoeff);
VOID ive_setEdge2CoeffReg(UINT32 *puiEdgeCoeff);
VOID ive_setThresLutReg(UINT32 *puiThresLut);
VOID ive_setEdgeMagThReg(UINT32 uiEdgeMagTh);
VOID ive_setMorphNeighEnReg(BOOL *pbMorphNeighEn);
VOID ive_setInBurstLengthReg(UINT32 InBurstSel);
VOID ive_setOutBurstLengthReg(UINT32 OutBurstSel);
VOID ive_setDbgPortReg(UINT32 DbgPortSel);
UINT32 ive_getIntrEnableReg(VOID);
UINT32 ive_getIntrStatusReg(VOID);
UINT32 ive_getInAddrReg(VOID);
UINT32 ive_getInLofstReg(VOID);
UINT32 ive_getOutAddrReg(VOID);
UINT32 ive_getOutLofstReg(VOID);
ER ive_lock(VOID);
ER ive_setReset(VOID);
VOID ive_setIrvEnReg(BOOL bEn);
VOID ive_setIrvHistModeSelReg(BOOL bHistModeSel);
VOID ive_setIrvInvalidValReg(BOOL bInvalidVal);
VOID ive_setIrvThrSReg(UINT32 uiIrvThrS);
VOID ive_setIrvThrHReg(UINT32 uiIrvThrH);
VOID ive_setIrvMednInvalThReg(UINT32 uiIrvMednInvalTh);

/*
    internal function declaration
*/
//engine control API
ER ive_attach(VOID);
ER ive_detach(VOID);
ER ive_enable(BOOL bStartOp);
void ive_ll_enable(BOOL bStart);
void ive_ll_terminate(BOOL isterminate);
ER ive_lock(VOID);
ER ive_unlock(VOID);
ER ive_readnChkQtableValue(VOID);
BOOL ive_isOpen(VOID);
VOID ive_setClockRate(UINT32 uiRdeClockRate);
ER ive_setOutRandInit(UINT32 uiInit0, UINT32 uiInit1);
ER ive_setQTable(UINT32 *puiQTbl);
ER ive_getDegammaTable(UINT32 *puiDgmaTbl);

//polling mode
UINT32 ive_getFrmEndRdyReg(VOID);
UINT32 ive_getLLFrmEndRdyReg(VOID);

VOID ive_set_dma_disable(BOOL disable);
BOOL ive_get_dma_disable(VOID);
BOOL ive_get_engine_idle(VOID);
#ifdef __cplusplus
}
#endif

#endif
