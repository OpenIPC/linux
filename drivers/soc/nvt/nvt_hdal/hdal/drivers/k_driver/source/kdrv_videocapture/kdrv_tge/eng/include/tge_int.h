/*
    Internal header file for TGE module.

    @file       tge_int.h
    @ingroup    mIIPPTGE

    Copyright   Novatek Microelectronics Corp. 2010.  All rights reserved.
*/

#ifndef _TGE_INT_H
#define _TGE_INT_H
//#include "nvtDrvProtected.h"
#include "kwrap/type.h"
#include "tge_lib.h"
#include "tge_dbg.h"

extern void tge_setBaseAddr(UINT32 uiAddr);
extern void tge_setReset(BOOL bReset);
//extern void tge_setVDReset(BOOL bReset);
extern void tge_setLoad(void);
extern void tge_setBasicSet(TGE_BASIC_SET_INFO *pBasic);
extern void tge_setModeSel(TGE_MODE_SEL_INFO *pMode);
extern void tge_setVdMode(TGE_MODE_SEL Mode);
extern void tge_setVd2Mode(TGE_MODE_SEL Mode);
extern void tge_setFlshMshMode(TGE_FLSH_MSH_VD_IN_SEL ModeSel);

extern void tge_setVdTimingPause(TGE_TIMING_PAUSE_INFO *pTimingPauseInfo);
extern void tge_setVd2TimingPause(TGE_TIMING_PAUSE_INFO *pTimingPauseInfo);

extern void tge_enableIntEnable(BOOL bEnable, UINT32 uiIntrp);
extern void tge_setIntEnable(UINT32 uiIntrp);
extern UINT32 tge_getIntEnable(void);

extern UINT32 tge_getIntrStatus(void);
extern void tge_clrIntrStatus(UINT32 uiIntrpStatus);
extern void tge_setAllVdRst(TGE_VD_RST_INFO *pVdRst);
extern void tge_setRst(TGE_VD_RST_INFO *pParam, TGE_CHANGE_RST_SEL FunSel);
extern void tge_setVdRst(TGE_VD_RST_SEL VdRst);
extern void tge_setVd2Rst(TGE_VD_RST_SEL VdRst);

extern void tge_setVdHdInfo(TGE_VD_RST_INFO *pVdRst, TGE_VDHD_SET *pVdHdInfo);
extern void tge_setVdHd(TGE_VDHD_INFO *pVdHd);
extern void tge_setVd2Hd2(TGE_VDHD_INFO *pVdHd);
extern void tge_setOutSwap(TGE_OUT_SWAP_INFO *pOutSwap);
extern void tge_setBreadPoint(TGE_VD_RST_INFO *pVdRst, TGE_BREAKPOINT_INFO *pBP);
extern void tge_setVdBreadPoint(UINT32 pBP);
extern void tge_setVd2BreadPoint(UINT32 pBP);

extern void tge_setVdInv(TGE_VD_INV *pInvSel);
extern void tge_setVd2Inv(TGE_VD_INV *pInvSel);

extern void tge_setFlshCtrlInv(BOOL bCtrlInv);
extern void tge_setMshACtrlInv(BOOL bCtrlInv);
extern void tge_setMshBCtrlInv(BOOL bCtrlInv);
extern void tge_setFlshExtInv(BOOL bCtrlInv);
extern void tge_setMshExtInv(BOOL bCtrlInv);
extern void tge_setMshASwap(BOOL bSwap);
extern void tge_setMshBSwap(BOOL bSwap);
extern void tge_setMshACtrlMode(BOOL bCtrlMode);
extern void tge_setMshBCtrlMode(BOOL bCtrlMode);

extern void tge_setVd2Sie(TGE_VDTOSIE_SEL VdSel);
extern void tge_setVd2Sie3(TGE_VDTOSIE3_SEL VdSel);

extern void tge_setVdPhase(TGE_VD_PHASE *pPhaseSel);
extern void tge_setVd2Phase(TGE_VD_PHASE *pPhaseSel);



extern void tge_getEngineStatus(TGE_ENGINE_STATUS_INFO *pEngineStatus);

extern void tge_setFlshTrig(TGE_CTRL_PIN_TRG_SEL TrigSel);
extern void tge_setFlshCtrl(TGE_CTRL_PIN_INFO *pCtrlPin);

extern void tge_setMshACloseTrig(TGE_CTRL_PIN_TRG_SEL TrigSel);
extern void tge_setMshACloseCtrl(TGE_CTRL_PIN_INFO *pCtrlPin);
extern void tge_setMshAOpenTrig(TGE_CTRL_PIN_TRG_SEL TrigSel);
extern void tge_setMshAOpenCtrl(TGE_CTRL_PIN_INFO *pCtrlPin);
extern void tge_setMshBCloseTrig(TGE_CTRL_PIN_TRG_SEL TrigSel);
extern void tge_setMshBCloseCtrl(TGE_CTRL_PIN_INFO *pCtrlPin);
extern void tge_setMshBOpenTrig(TGE_CTRL_PIN_TRG_SEL TrigSel);
extern void tge_setMshBOpenCtrl(TGE_CTRL_PIN_INFO *pCtrlPin);









#endif// _TGE_INT_H

