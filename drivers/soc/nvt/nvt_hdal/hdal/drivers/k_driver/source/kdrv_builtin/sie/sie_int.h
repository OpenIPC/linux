/*
    Internal header file for SIE module.

    @file       sie_int.h
    @ingroup    mIIPPSIE

    Copyright   Novatek Microelectronics Corp. 2010.  All rights reserved.
*/

#ifndef _SIE_INT_H
#define _SIE_INT_H
#include "kwrap/type.h"
#include "sie_lib.h"

#define ECS_MAX_MAP_SZ 65
#define ECS_MAX_MAP_BUF_SZ (ECS_MAX_MAP_SZ*ECS_MAX_MAP_SZ*4)
#define SIE_DPC_MAX_PXCNT 4096
#define SIE_DPC_MAX_BUF_SZ (SIE_DPC_MAX_PXCNT * 4)
#define TIME_MEASURE 0

extern void sie_setBaseAddr(UINT32 id, UINT32 uiAddr);
extern void sie_setReset(UINT32 id, BOOL bReset);
extern BOOL sie_getReset(UINT32 id);
extern void sie_setDmaabort(UINT32 id);
extern void sie_setGlobalLoad(UINT32 id, UINT32 value);
extern void sie_setLoad(UINT32 id);
extern void sie_setActEn(UINT32 id, BOOL bActEn);

extern UINT32 sie_getIntrStatus(UINT32 id);
extern UINT32 sie_getFunction(UINT32 id);
extern void sie_getDramSingleOut(UINT32 id, SIE_DRAM_SINGLE_OUT *pDOut);
extern void sie_getSysDbgInfo(UINT32 id, SIE_SYS_DEBUG_INFO*pDbugInfo);
extern void sie_setChecksumEn(UINT32 id,UINT32 uiEn);
extern void sie_getActiveWindow(UINT32 id, SIE_ACT_WIN_INFO *pActWin);
extern void sie_getCropWindow(UINT32 id, SIE_CRP_WIN_INFO *pCrpWin);
extern void sie_getEngineStatus(UINT32 id, SIE_ENGINE_STATUS_INFO *pEngineStatus);
extern void sie_clrIntrStatus(UINT32 id, UINT32 uiIntrpStatus);
extern UINT32 sie_getIntEnable(UINT32 id);
extern void sie_getEngineStatus(UINT32 id, SIE_ENGINE_STATUS_INFO *pEngineStatus);
extern void sie_setDramOutMode(UINT32 id, SIE_DRAM_OUT_CTRL OutMode);
extern void sie_setVdHdInterval(UINT32 id, SIE_VDHD_INTERVAL_INFO *IntervalInfo);
extern void sie_setMainInput(UINT32 id, SIE_MAIN_INPUT_INFO *pMainInInfo);
extern void sie_getMainInput(UINT32 id, SIE_MAIN_INPUT_INFO *pMainInInfo);

extern void sie_setVdHdDelay(UINT32 id, SIE_VDHD_DELAY_INFO *delay_info);
extern void sie_setDramInStart(UINT32 id);
extern BOOL sie_getDramInStart(UINT32 id);
extern void sie_setRwOBP(UINT32 id, BOOL bEnable);

extern ER sie_verifyFunction(UINT32 uiFunction);
extern void sie_enableFunction(UINT32 id, BOOL bEnable, UINT32 uiFunction);
extern void sie_setFunction(UINT32 id, UINT32 uiFunction);
extern UINT32 sie_getFunction(UINT32 id);
extern void sie_setSramPwrSave(UINT32 id, BOOL bBsAuto, BOOL bVaAuto);

extern void sie_enableIntEnable(UINT32 id, BOOL bEnable, UINT32 uiIntrp);
extern void sie_setIntEnable(UINT32 id, UINT32 uiIntrp);



extern void sie_clrEngineStatus(UINT32 id, SIE_ENGINE_STATUS_INFO *pEngineStatus);
extern void sie_setSourceWindow(UINT32 id, SIE_SRC_WIN_INFO *pSrcWin);
extern void sie_getSourceWindow(UINT32 id, SIE_SRC_WIN_INFO *pSrcWin);

extern void sie_setBreakPoint(UINT32 id, SIE_BREAKPOINT_INFO *pBP);
extern void sie_setBreakPoint1(UINT32 id, SIE_BREAKPOINT_INFO *pBP);
extern void sie_setBreakPoint2(UINT32 id, SIE_BREAKPOINT_INFO *pBP);
extern void sie_setBreakPoint3(UINT32 id, SIE_BREAKPOINT_INFO *pBP);

extern void sie_setActiveWindow(UINT32 id, SIE_ACT_WIN_INFO *pActWin);
extern void sie_getActiveWindow(UINT32 id, SIE_ACT_WIN_INFO *pActWin);

extern void sie_setCropWindow(UINT32 id, SIE_CRP_WIN_INFO *pCrpWin);
extern void sie_getCropWindow(UINT32 id, SIE_CRP_WIN_INFO *pCrpWin);
extern void sie_setDvsWindow(UINT32 id, SIE_DVS_WIN_INFO *pDvsWin);
extern void sie_getDvsWindow(UINT32 id, SIE_DVS_WIN_INFO *pDvsWin);
extern void sie_setDvsCode(UINT32 id, SIE_DVS_CODE_INFO *pDvsCode);
extern void sie_getDvsCode(UINT32 id, SIE_DVS_CODE_INFO *pDvsCode);
extern void sie_calcDvsWindow(UINT32 id, SIE_DVS_WIN_INFO *pDvsWin);
extern void sie_getActBlankingInfo(UINT32 id, SIE_PIX_RATE_INFO *pPixRateInfo);
extern void sie_setDramIn0(UINT32 id, SIE_DRAM_IN0_INFO *pDIn0);
extern void sie_getDramIn0(UINT32 id, SIE_DRAM_IN0_INFO *pDIn0);
extern void sie_setDramIn1(UINT32 id, SIE_DRAM_IN1_INFO *pDIn1);
extern void sie_setDramIn2(UINT32 id, SIE_DRAM_IN2_INFO *pDIn2);
extern void sie_setDramSingleOut(UINT32 id, SIE_DRAM_SINGLE_OUT *pDOut);
extern void sie_getDramSingleOut(UINT32 id, SIE_DRAM_SINGLE_OUT *pDOut);
extern void sie_setDramOut0(UINT32 id, SIE_DRAM_OUT0_INFO *pDOut0);
extern void sie_setDramOut0Flip(UINT32 id, SIE_DRAM_OUT0_INFO *pDOut0);
extern void sie_setDramOut1(UINT32 id, SIE_DRAM_OUT1_INFO *pDOut1);
extern void sie_setDramOut1Flip(UINT32 id, SIE_DRAM_OUT1_INFO *pDOut1);
extern void sie_setDramOut2(UINT32 id, SIE_DRAM_OUT2_INFO *pDOut2);
extern void sie_chgBurstLength(UINT32 id, SIE_BURST_LENGTH *pBurstLen);
extern void sie_getBurstLength(UINT32 id, SIE_BURST_LENGTH *pBurstLen);
extern void sie_setDramBurst(UINT32 id, SIE_DRAM_BURST_INFO *pDramBurst);// would be removed soon
extern void sie_setFlip(UINT32 id, SIE_FLIP_INFO *pFlipParm);
extern void sie1_getFlip(UINT32 id, SIE_FLIP_INFO *pFlipParm);
extern void sie_getDramOut0(UINT32 id, SIE_DRAM_OUT0_INFO *pDOut0);
extern void sie1_getDramOut0Flip(SIE_DRAM_OUT0_INFO *pDOut0);
extern void sie_getDramOut1(UINT32 id, SIE_DRAM_OUT1_INFO *pDOut1);
extern void sie1_getDramOut1Flip(SIE_DRAM_OUT1_INFO *pDOut1);
extern void sie_getDramOut2(UINT32 id, SIE_DRAM_OUT2_INFO *pDOut2);

extern void sie_setCCIR656Header(UINT32 id, UINT16 value);
extern void sie_getCCIR656HeaderStatus(UINT32 id, UINT32 value);
extern void sie_setDVI(UINT32 id, SIE_DVI_INFO *pDviParam);
extern void sie_getDVI(UINT32 id, SIE_DVI_INFO *pDviParam);

extern void sie_setPatGen(UINT32 id, SIE_PATGEN_INFO *pPatGen);
extern void sie_getPatGen(UINT32 id, SIE_PATGEN_INFO *pPatGen);
extern void sie_setObDt(UINT32 id, SIE_OB_DT_INFO *pObDt);
extern void sie_getObDtRslt(UINT32 id, SIE_OB_DT_RSLT_INFO *pObDtRslt);
extern void sie_setObOfs(UINT32 id, SIE_OB_OFS_INFO *pObDtOfs);

extern void sie_setDpc(UINT32 id, SIE_DPC_INFO *pDpc);
extern void sie_setDecompanding(UINT32 id, SIE_DECOMPANDING_INFO *pDecomp);
extern void sie_setCompanding(UINT32 id, SIE_COMPANDING_INFO *pComp);
extern void sie1_setMainGma(SIE_MAIN_GAMMA_INFO *pMainGma);

extern void sie_setECS(UINT32 id, SIE_ECS_INFO *pEcs);
extern void sie_getECS(UINT32 id, SIE_ECS_INFO *pEcs);
extern void sie_setDGain(UINT32 id, SIE_DGAIN_INFO *pDGain);

extern void sie_setBSH(UINT32 id, SIE_BS_H_INFO *pBsh);
extern void sie_getBSH(UINT32 id, SIE_BS_H_INFO *pBsh);
extern void sie_setBSV(UINT32 id, SIE_BS_V_INFO *pBsv);
extern void sie_getBSV(UINT32 id, SIE_BS_V_INFO *pBsv);
extern void sie_setCGain(UINT32 id, SIE_CG_INFO *pCGain);
extern void sie_getCGain(UINT32 id, SIE_CG_INFO *pCGain);
//NA//extern void sie_setGridLine(UINT32 id, SIE_GRIDLINE_INFO *pGridLine);
//NA//extern void sie_setYoutScl(UINT32 id, SIE_YOUT_SCAL_INFO *pYoutScl);
//NA//extern void sie_setYoutWin(UINT32 id, SIE_YOUT_WIN_INFO *pYoutWin);
//NA//extern void sie_setYoutAccm(UINT32 id, SIE_YOUT_ACCM_INFO *pYoutWin);
//NA//extern void sie_getYoutWin(UINT32 id, SIE_YOUT_WIN_INFO *pYoutWin);
//NA//extern void sie_setYoutCg(UINT32 id, SIE_YOUT_CG_INFO *pYoutCg);

extern void sie_setStcsPath(UINT32 id, SIE_STCS_PATH_INFO *pSetting);
extern void sie_getStcsPath(UINT32 id, SIE_STCS_PATH_INFO *pSetting);
extern void sie_setVIG(UINT32 id, SIE_VIG_INFO *pVig);
extern void sie_setStcsOb(UINT32 id, SIE_STCS_OB_INFO *pSetting);
extern void sie_getStcsOb(UINT32 id, SIE_STCS_OB_INFO *pSetting);
extern void sie_setCaCrp(UINT32 id, SIE_CA_CROP_INFO *pCaCrp);
extern void sie_getCaCrp(UINT32 id, SIE_CA_CROP_INFO *pCaCrp);
extern void sie_setCaScl(UINT32 id, SIE_CA_SCAL_INFO *pCaScl);
extern void sie_setCaIrSub(UINT32 id, SIE_CA_IRSUB_INFO *pCaIrSubInfo);
extern void sie_setCaTh(UINT32 id, SIE_CA_TH_INFO *pCaTh);
extern void sie_setCaWin(UINT32 id, SIE_CA_WIN_INFO *pSetting);
extern void sie_getCaWin(UINT32 id, SIE_CA_WIN_INFO *pSetting);
extern void sie_setLaIrSub(UINT32 id, SIE_LA_IRSUB_INFO *pLaIrSubInfo);
extern void sie_setLaCrp(UINT32 id, SIE_LA_CROP_INFO *pSetting);
extern void sie_setLaGma1(UINT32 id, SIE_LA_GMA_INFO *pSetting);
extern void sie_setLaCg(UINT32 id, SIE_LA_CG_INFO *pSetting);
extern void sie_getLaCg(UINT32 id, SIE_LA_CG_INFO *pSetting);
extern void sie_setLaTh(UINT32 id, SIE_LA_TH *pSetting);
extern void sie_setLaWin(UINT32 id, SIE_LA_WIN_INFO *pSetting);
extern void sie_getLaWin(UINT32 id, SIE_LA_WIN_INFO *pSetting);
extern UINT32 sie_getLaWinSum(UINT32 id);
//NA//extern void sie_setVaCg(UINT32 id, SIE_VA_CG_INFO *pSetting);
//NA//extern void sie_setVaCrp(UINT32 id, SIE_VA_CROP_INFO *pSetting);
//NA//extern void sie_getVaCrp(UINT32 id, SIE_VA_CROP_INFO *pSetting);
//NA//extern void sie_setVaGma2(UINT32 id, SIE_VA_GMA_INFO *pSetting);
//NA//extern void sie_setVaFltrG1(UINT32 id, SIE_VA_FLTR_GROUP_INFO *pSetting);
//NA//extern void sie_setVaFltrG2(UINT32 id, SIE_VA_FLTR_GROUP_INFO *pSetting);
//NA//extern void sie_setVaWin(UINT32 id, SIE_VA_WIN_INFO *pSetting);
//NA//extern void sie_getVaWin(UINT32 id, SIE_VA_WIN_INFO *pSetting);
//NA//extern void sie_setVaIndepWin(UINT32 id, SIE_VA_INDEP_WIN_INFO *pSetting, UINT32 uiIdx);
//NA//extern void sie_getVaIndepWinRslt(UINT32 id, SIE_VA_INDEP_WIN_RSLT_INFO *pSetting, UINT32 uiIdx);
//NA//extern void sie_setEth(UINT32 id, SIE_ETH_INFO *pEth);
//NA//extern void sie_getEth(UINT32 id, SIE_ETH_INFO *pEth);


//NA//extern void sie_setPFPC(UINT32 id, SIE_PFPC_INFO *pPfpcParam);














//NA//extern void sie_setObFrm(UINT32 id, SIE_OB_FRAME_INFO *pObFrm);
//NA//extern void sie_getObFrmRslt(UINT32 id, SIE_OB_FRAME_RSLT_INFO *pObFrmRslt);
//NA//extern void sie_setObPln(UINT32 id, SIE_OB_PLANE_INFO *pObPln);
extern void sie_setmd(UINT32 id, SIE_MD_INFO *pSetting);














extern UINT32 sie_getOutAdd(UINT32 id, UINT32 uiOutChIdx, SIE_PINGPONG_BUFF_SEL uiBufSel);
extern UINT32 sie_getOutAdd_Flip(UINT32 id, UINT32 uiOutChIdx, SIE_PINGPONG_BUFF_SEL uiBufSel);
extern void sie_getCAResult(UINT32 id, SIE_STCS_CA_RSLT_INFO *CaRsltInfo);
extern void sie_getSatgainInfo(UINT32 id, UINT32 *satgain);

//NA//extern void sie1_getCAFlipResult(UINT16 *puiBufR, UINT16 *puiBufG, UINT16 *puiBufB, UINT16 *puiAccCnt, SIE_FLIP_SEL pFlipSel);
extern void sie_getCAResultManual(UINT32 id, SIE_STCS_CA_RSLT_INFO *CaRsltInfo, SIE_CA_WIN_INFO *CaWinInfo, UINT32 uiBuffAddr);
//NA//extern void sie1_getCAFlipResultManual(UINT16 *puiBufR, UINT16 *puiBufG, UINT16 *puiBufB, UINT16 *puiAccCnt, SIE_CA_WIN_INFO *CaWinInfo, UINT32 uiBuffAddr, SIE_FLIP_SEL pFlipSel);
extern void sie_getHisto(UINT32 id, UINT16 *puiHisto);
extern void sie_getLAResult(UINT32 id, UINT16 *puiBufLa1, UINT16 *puiBufLa2);
//NA//extern void sie1_getLAFlipResult(UINT16 *puiBufLa1, UINT16 *puiBufLa2, SIE_FLIP_SEL pFlipSel);
extern void sie_getLAResultManual(UINT32 id, UINT16 *puiBufLa1, UINT16 *puiBufLa2, SIE_LA_WIN_INFO *LaWinInfo, UINT32 uiBuffAddr);
extern void sie_getMdResult(UINT32 id, SIE_MD_RESULT_INFO *pMdInfo);


extern ER   sie1_SIE_CLK_openClock(void);
extern ER   sie1_SIE_CLK_closeClock(void);
extern BOOL sie1_SIE_CLK_isClockOpened(void);
extern ER   sie1_SIE_CLK_chgClockSource(SIE_CLKSRC_SEL ClkSrc);
extern ER   sie1_SIE_CLK_chgClockRate(UINT32 uiClkRate);

extern void sie_setChgInAddr(UINT32 id, SIE_CHG_IN_ADDR_INFO *pChgIoAddrInfo);
extern void sie_setChgOutAddr(UINT32 id, SIE_CHG_OUT_ADDR_INFO *pChgIoAddrInfo);
extern void sie_setChgRingBuf(UINT32 id, SIE_CHG_RINGBUF_INFO *pChgIoAddrInfo);
extern void sie_setChgOutAddr_Flip(UINT32 id, SIE_CHG_OUT_ADDR_INFO *pChgIoAddrInfo);
extern void sie_setChgIOLofs(UINT32 id, SIE_CHG_IO_LOFS_INFO *pChgIoLofsInfo);
extern void sie_setChgOutSrc(UINT32 id, SIE_CHG_OUT_SRC_INFO *pChgIoSrcInfo);
extern void sie_setOutBitDepth(UINT32 id, SIE_PACKBUS_SEL ChgOut0PackBus);
extern void sie_setOutFormat(UINT32 id, SIE_DRAM_OUT0_FMT *Out0Fmt);
extern void sie_getGammaLut_Reg(UINT32 id, UINT32 *puiGmaLut);

//BCC APIs
extern void sie_setBccParam(UINT32 id, SIE_BCC_PARAM_INFO *pRawEncParam);
extern VOID sie_setBccEn(UINT32 id, BOOL bBccEn);
extern VOID sie_setBccGammaEnReg(UINT32 id, BOOL bGammaEn);
extern VOID sie_setBccSegBitNumReg(UINT32 id, UINT32 uiSegBitNum);
extern VOID sie_setBccGammaTblReg(UINT32 id, UINT32 *p_uiBccGammaTbl);
extern VOID sie_setBccIniParm(SIE_BCC_PARAM_INFO *pBccParam, UINT32 rate);


#endif// _SIE_INT_H
