/*
    Public header file for RDE module.

    This file is the header file that define the API and data type for SIE
    module.

    @file       sie1_lib.h
    @ingroup    mIDrvIPPSIE
    @note

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#ifndef _SIE_LIB_H
#define _SIE_LIB_H

#include "siep_lib.h"

//extern UINT32 g_uiSie1Out0Cnt,g_uiSie2Out0Cnt;
//extern UINT32 g_uiSie1BccOvflCnt,g_uiSie2BccOvflCnt;
//extern UINT32 g_uiSie1VdCnt,g_uiSie2VdCnt;
//extern UINT32 g_uiSie1ActStCnt,g_uiSie2ActStCnt;
//extern UINT32 g_uiSie1Out5Cnt,g_uiSie2Out5Cnt;

/**
    SIE Entire Configuration.

    Structure of entire SIE configuration.

    This is for emulation only.
*/
//@{
typedef struct {
	// No run-time change
	SIE_MAIN_INPUT_INFO         MainInInfo;
	SIE_DVI_INFO                DviInfo;
	SIE_PATGEN_INFO             PatGenInfo;
    SIE_DRAM_OUT_CTRL           DramOutMode;
	SIE_VDHD_INTERVAL_INFO      VdHdIntervalInfo;

	// run-time change-able
	UINT32  uiFuncEn;
	UINT32  uiIntrpEn;
	SIE_BREAKPOINT_INFO BreakPointInfo;
	SIE_SRC_WIN_INFO    SrcWinInfo;
	SIE_ACT_WIN_INFO    ActWinInfo;
	SIE_CRP_WIN_INFO    CrpWinInfo;

	SIE_DRAM_IN0_INFO   DramIn0Info;
	SIE_DRAM_IN1_INFO   DramIn1Info;
	SIE_DRAM_IN2_INFO   DramIn2Info;
	SIE_DRAM_OUT0_INFO  DramOut0Info;
	SIE_DRAM_OUT1_INFO  DramOut1Info;
	SIE_DRAM_OUT2_INFO  DramOut2Info;
	//NA//SIE_DRAM_OUT3_INFO  DramOut3Info;
//NA//    SIE_DRAM_OUT4_INFO  DramOut4Info;
	//NA//SIE_DRAM_OUT5_INFO  DramOut5Info;
	SIE_DRAM_BURST_INFO DramBurstInfo;// would be removed soon

	SIE_OB_DT_INFO          ObDtInfo;
	SIE_OB_DT_RSLT_INFO     ObDtRsltInfo;// would be removed soon
	SIE_OB_OFS_INFO         ObOfsInfo;
//NA//    SIE_OB_FRAME_INFO       ObFrmInfo;
//NA//    SIE_OB_PLANE_INFO       ObPlnInfo;

	SIE_DPC_INFO        DefectInfo;
	SIE_MAIN_GAMMA_INFO MainGmaInfo;
	SIE_ECS_INFO        EcsInfo;
	SIE_DGAIN_INFO      DgainInfo;
    SIE_DECOMPANDING_INFO DeCompandingInfo;
	SIE_COMPANDING_INFO CompandingInfo;

	SIE_BS_H_INFO       BshInfo;
	SIE_BS_V_INFO       BsvInfo;
	SIE_CG_INFO         CGainInfo;
	//NA//SIE_GRIDLINE_INFO   GridLineInfo;
	//NA//SIE_YOUT_INFO       YoutInfo;

	//NA//SIE_IR_INFO         IrInfo;
	//NA//SIE_RCCB_INFO       RccbInfo;


	SIE_BCC_PARAM_INFO  BccParamInfo;

	SIE_STCS_PATH_INFO  StcsPathInfo;
	SIE_STCS_CALASIZE_GRP_INFO  StcsCaLaSzGrpInfo;
	SIE_VIG_INFO        StcsVigInfo;
	SIE_CA_TH_INFO      StcsCaThInfo;
	SIE_LA_CG_INFO      StcsLaCgInfo;
	SIE_LA_GMA_INFO     StcsLaGma1Info;
	SIE_DRAM_SINGLE_OUT DramSingleOutEn;
    SIE_MD_INFO         mdInfo;

	//SIE_STCS_CALASIZE_GRP_INFO//SIE_LA_WIN_INFO     StcsLaWinInfo;
//NA//    SIE_VA_CG_INFO      StcsVaCgInfo;
//NA//    SIE_VA_CROP_INFO    StcsVaCrpInfo;
//NA//    SIE_VA_GMA_INFO     StcsVaGma2Info;
//NA//    SIE_VA_FLTR_GROUP_INFO    StcsVaFltrG1;
//NA//    SIE_VA_FLTR_GROUP_INFO    StcsVaFltrG2;
//NA//    SIE_VA_WIN_INFO     StcsVaWinInfo;
//NA//    SIE_VA_INDEP_WIN_INFO StcsVaIndepWinInfo[5];
//NA//    SIE_ETH_INFO          StcsEthInfo;

//NA//    SIE_PFPC_INFO       PfpcInfo;
} SIE_MODE_PARAM;
//@}


typedef void (*SIE_DRV_ISR_FP)(UINT32 int_sts);
/**
SIE API

@name   SIE_API
*/
//@{
extern ER sie_open(SIE_ENGINE_ID eng_id, SIE_OPENOBJ *pObjCB);
extern BOOL sie_isOpened(SIE_ENGINE_ID eng_id);
extern ER sie_close(SIE_ENGINE_ID eng_id);
extern ER sie_pause(SIE_ENGINE_ID eng_id);
extern ER sie_start(SIE_ENGINE_ID eng_id);
extern ER sie_setMode(SIE_ENGINE_ID eng_id, SIE_MODE_PARAM *pSieParam);
extern ER sie_chgParam(SIE_ENGINE_ID eng_id, void *pParam, SIE_CHANGE_FUN_PARAM_SEL FunSel);
extern ER sie_chgFuncEn(SIE_ENGINE_ID eng_id, SIE_FUNC_SWITCH_SEL FuncSwitch, UINT32 uiFuncSel);
extern ER sie_waitEvent(SIE_ENGINE_ID eng_id, SIE_WAIT_EVENT_SEL WaitEvent, BOOL bClrFlag);

extern void sie_getLAGamma(SIE_ENGINE_ID eng_id, UINT32 *puiGammaLut);
//NA//extern void sie_calcObPlnScl(SIE_OB_PLANE_INFO *pObpParam, SIE_OB_PLANE_ADJ_INFO *pObpAdjParam);
extern void sie_calcBSHScl(SIE_BS_H_INFO *pBshParam, SIE_BS_H_ADJ_INFO *pBshAdjParam);
extern void sie_calcBSVScl(SIE_BS_V_INFO *pBsvParam, SIE_BS_V_ADJ_INFO *pBsvAdjParam);
extern void sie_calcECSScl(SIE_ECS_INFO *pEcsParam, SIE_ECS_ADJ_INFO *pEcsAdjParam);
extern void sie_chgLoadSrc(SIE_ENGINE_ID id, SIE_LOAD_SRC load_src);
extern void sie_set_dmaaobrt(SIE_ENGINE_ID eng_id);
extern void sie_set_vdhddelay(SIE_ENGINE_ID eng_id, SIE_VDHD_DELAY_INFO *timing_delayInfo);
extern void sie_calcCaLaSize(SIE_ENGINE_ID eng_id, SIE_STCS_CALASIZE_GRP_INFO *pVaSzGrpInfo, SIE_STCS_CALASIZE_ADJ_INFO *pVaAdjInfo);
//NA//extern void sie_calcYoutInfo(SIE_YOUT_GRP_INFO *pSzGrpInfo, SIE_YOUT_ADJ_INFO *pSzAdjInfo);
extern void sie_get_ca_rslt(SIE_ENGINE_ID eng_id, SIE_STCS_CA_RSLT_INFO *CaRsltInfo, SIE_CA_WIN_INFO *CaWinInfo, UINT32 uiBuffAddr);
extern void sie_get_histo(SIE_ENGINE_ID eng_id, UINT16 *puiHisto);
extern void sie_get_la_rslt(SIE_ENGINE_ID eng_id, UINT16 *puiBufLa1, UINT16 *puiBufLa2, SIE_LA_WIN_INFO *LaWinInfo, UINT32 uiBuffAddr);
extern void sie_get_dram_single_out(SIE_ENGINE_ID eng_id, SIE_DRAM_SINGLE_OUT *pDOut);
extern void sie_get_sys_debug_info(SIE_ENGINE_ID eng_id, SIE_SYS_DEBUG_INFO *pDbugInfo);
extern void sie_set_checksum_en(SIE_ENGINE_ID eng_id, UINT32 uiEn);
extern void sie_get_sat_gain_info(SIE_ENGINE_ID eng_id, UINT32 *gain);
extern void sie_reg_isr_cb(SIE_ENGINE_ID eng_id, SIE_DRV_ISR_FP fp);
extern void sie_reg_vdlatch_isr_cb(SIE_ENGINE_ID eng_id, SIE_DRV_ISR_FP fp);
extern void sie_get_mdrslt(SIE_ENGINE_ID eng_id, SIE_MD_RESULT_INFO *pMdInfo);
extern void sie_calc_ir_level(SIE_ENGINE_ID eng_id, SIE_STCS_CA_RSLT_INFO *ca_rst, SIE_CA_WIN_INFO ca_win_info, SIE_CA_IRSUB_INFO ca_ir_sub_info);
extern UINT32 sie_get_ir_level(SIE_ENGINE_ID eng_id);
extern UINT32 sie_get_ccir656_matchStatus(SIE_ENGINE_ID eng_id);

extern ER sie_chgBurstLengthByMode(SIE_ENGINE_ID eng_id, SIE_DRAM_BURST_MODE_SEL BurstMdSel);
extern ER sie_attach(SIE_ENGINE_ID eng_id, BOOL do_enable);
extern ER sie_detach(SIE_ENGINE_ID eng_id, BOOL do_disable);

extern UINT32 sie_getClockRate(SIE_ENGINE_ID eng_id);
extern ER   sie1_SIE_CLK_openClock(void);
extern ER   sie1_SIE_CLK_closeClock(void);
extern BOOL sie1_SIE_CLK_isClockOpened(void);
extern ER   sie1_SIE_CLK_chgClockSource(SIE_CLKSRC_SEL ClkSrc);
extern ER   sie1_SIE_CLK_chgClockRate(UINT32 uiClkRate);
extern ER sie1_measureFrameTime(void);
extern INT32 sie_chk_limitation(SIE_ENGINE_ID eng_id, UINT32 flag);

#endif//_SIE_LIB_H
//@}

//@}


