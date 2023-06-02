////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2011 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// ("MStar Confidential Information") by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _MDRV_VIP_H
#define _MDRV_VIP_H


//-------------------------------------------------------------------------------------------------
//  Defines & Structure
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Prototype
//-------------------------------------------------------------------------------------------------

#ifndef _MDRV_VIP_C
#define INTERFACE extern
#else
#define INTERFACE
#endif
INTERFACE unsigned char MDrv_VIP_Init(ST_MDRV_VIP_INIT_CONFIG *pCfg);
INTERFACE void MDrv_VIP_Resume(ST_MDRV_VIP_INIT_CONFIG *pCfg);
INTERFACE void MDrv_VIP_FreeMemory(void);
INTERFACE void MDrv_VIP_FillBasicStructSetPQCfg(EN_VIP_MDRV_CONFIG_TYPE enVIPtype,void *pPointToCfg,ST_MDRV_VIP_SETPQ_CONFIG *stSetPQCfg);
INTERFACE void MDrv_VIP_CheckEachIPByCMDQIST(void);
INTERFACE unsigned char MDrv_VIP_GetIsBlankingRegion(void);
INTERFACE unsigned char MDrv_VIP_Sys_Init(ST_MDRV_VIP_INIT_CONFIG *pCfg);
INTERFACE void MDrv_VIP_Delete(unsigned char bCloseISR);
INTERFACE void MDrv_VIP_Release(void);
INTERFACE void MDrv_VIP_Supend(void);
INTERFACE unsigned char MDrv_VIP_GetCMDQHWDone(void);
INTERFACE void * MDrv_VIP_GetWaitQueueHead(void);
INTERFACE void MDrv_VIP_SetPollWait
    (void *filp, void *pWaitQueueHead, void *pstPollQueue);
INTERFACE void MDrv_VIP_ResetEachIP(void);
INTERFACE void MDrv_VIP_SetAllVIPOneshot(ST_MDRV_VIP_SUSPEND_CONFIG *stvipCfg);
INTERFACE void MDrv_VIP_SuspendResetFlagInit(void);
INTERFACE unsigned char MDrv_VIP_SetPeakingConfig(void *pCfg);
INTERFACE unsigned char MDrv_VIP_SetHistogramConfig(void *pCfg);
INTERFACE unsigned char MDrv_VIP_GetDLCHistogramReport(void *pCfg);
INTERFACE unsigned char MDrv_VIP_SetDLCConfig(void *pCfg);
INTERFACE unsigned char MDrv_VIP_SetLCEConfig(void *pCfg);
INTERFACE unsigned char MDrv_VIP_SetUVCConfig(void *pCfg);
INTERFACE unsigned char MDrv_VIP_SetIHCConfig(void *pCfg);
INTERFACE unsigned char MDrv_VIP_SetICEConfig(void *pCfg);
INTERFACE unsigned char MDrv_VIP_SetIHCICCADPYConfig(void *pCfg);
INTERFACE unsigned char MDrv_VIP_SetIBCConfig(void *pCfg);
INTERFACE unsigned char MDrv_VIP_SetFCCConfig(void *pCfg);
INTERFACE unsigned char MDrv_VIP_CMDQWriteConfig(ST_MDRV_VIP_CMDQ_CONFIG *pCfg);
INTERFACE unsigned char MDrv_VIP_SetLDCConfig(void *pCfg);
INTERFACE unsigned char MDrv_VIP_SetLDCmdConfig(void *pCfg);
INTERFACE unsigned char MDrv_VIP_SetLDCDmapConfig(void *pCfg);
INTERFACE unsigned char MDrv_VIP_SetLDCSRAMConfig(void *pCfg);
INTERFACE unsigned char MDrv_VIP_SetACKConfig(void *pCfg);
INTERFACE unsigned char MDrv_VIP_SetNLMConfig(void *pCfg);
INTERFACE unsigned char MDrv_SetVIPOtherConfig(void *pCfg);
INTERFACE unsigned char MDrv_VIP_SetVIPBypassConfig(EN_VIP_MDRV_CONFIG_TYPE enVIPtype);
INTERFACE unsigned char MDrv_VIP_SetAIPBypassConfig(EN_VIP_MDRV_AIP_TYPE enAIPtype);
INTERFACE unsigned char MDrv_VIP_SetNLMSRAMConfig(ST_MDRV_VIP_NLM_SRAM_CONFIG *stSRAM);
INTERFACE void MDrv_VIP_PrepareStructToCheckRegister(EN_VIP_CMDQ_CHECK_TYPE enCheckType);
INTERFACE void MDrv_VIP_SetCheckMode(EN_VIP_CMDQ_CHECK_TYPE enCheckType);
INTERFACE void MDrv_VIP_CheckRegister(void);
INTERFACE void MDrv_VIP_CheckConsist(void);
INTERFACE unsigned char MDrv_VIP_VtrackSetUserDefindedSetting(unsigned char bUserDefinded, unsigned char *pu8Setting);
INTERFACE unsigned char MDrv_VIP_VtrackEnable( unsigned char u8FrameRate, EN_VIP_MDRV_VTRACK_ENABLE_TYPE bEnable);
INTERFACE unsigned char MDrv_VIP_VtrackSetPayloadData(unsigned short u16Timecode, unsigned char u8OperatorID);
INTERFACE unsigned char MDrv_VIP_VtrackSetKey(unsigned char bUserDefinded, unsigned char *pu8Setting);
INTERFACE unsigned char MDrv_VIP_VtrackInit(void);
INTERFACE ssize_t MDrv_VIP_ProcShow(char *buf);
INTERFACE unsigned long MDrv_VIP_GetSCLFrameEndCount(void);
INTERFACE unsigned char MDrv_VIP_SetAIPConfig(ST_MDRV_VIP_AIP_CONFIG *stCfg);
INTERFACE unsigned char MDrv_VIP_SetAIPSRAMConfig(ST_MDRV_VIP_AIP_SRAM_CONFIG *stCfg);
INTERFACE unsigned char MDrv_VIP_SetMCNRConfig(void *pstCfg);
INTERFACE ssize_t MDrv_VIP_VIPShow(char *buf);
INTERFACE void MDrv_VIP_VIPStore(const char *buf);
INTERFACE ssize_t MDrv_VIP_CMDQShow(char *buf);
INTERFACE ssize_t MDrv_VIP_VIPSetRuleShow(char *buf);

#undef INTERFACE


#endif
