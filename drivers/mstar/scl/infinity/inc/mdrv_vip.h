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
typedef enum
{
    EN_VIP_CMDQ_CHECK_RETURN_ORI        = 0,     // reset before test
    EN_VIP_CMDQ_CHECK_PQ                = 1,     // autotest PQ
    EN_VIP_CMDQ_CHECK_ALREADY_SETINNG   = 2,     // test CMDQ
    EN_VIP_CMDQ_CHECK_AUTOSETTING       = 3,     // autotest CMDQ (value = mask)
}EN_VIP_CMDQ_CHECK_TYPE;
typedef enum
{
    PQ_IP_PreSNR_Size = sizeof(ST_IOCTL_VIP_SNR_MAIN_CONFIG),  //0
    PQ_IP_DNR_Size = sizeof(ST_IOCTL_VIP_DNR_ONOFF_CONFIG),  //1
    PQ_IP_DNR_Y_Size = sizeof(ST_IOCTL_VIP_DNR_Y_CONFIG),  //2
    PQ_IP_DNR_C_Size = sizeof(ST_IOCTL_VIP_DNR_C_CONFIG),  //3
    PQ_IP_LDC_Size = sizeof(ST_IOCTL_VIP_LDC_OnOffCONFIG),  //4
    PQ_IP_LDC_422_444_422_Size = sizeof(ST_IOCTL_VIP_LDC_422_444_CONFIG),  //5
    PQ_IP_NLM_Size = sizeof(ST_IOCTL_VIP_NLM_MAIN_CONFIG),  //6
    PQ_IP_422to444_Size = sizeof(ST_IOCTL_VIP_422_444_CONFIG),  //7
    PQ_IP_VIP_Size = sizeof(ST_IOCTL_VIP_BYPASS_CONFIG),  //8
    PQ_IP_VIP_pseudo_Size = 0,  //9
    PQ_IP_VIP_LineBuffer_Size = sizeof(ST_IOCTL_VIP_LINEBUFFER_CONFIG),  //10
    PQ_IP_VIP_HLPF_Size = sizeof(ST_IOCTL_VIP_HLPF_CONFIG),  //11
    PQ_IP_VIP_HLPF_dither_Size = sizeof(ST_IOCTL_VIP_HLPF_DITHER_CONFIG),  //12
    PQ_IP_VIP_VLPF_coef1_Size = sizeof(ST_IOCTL_VIP_VLPF_COEF_CONFIG),  //13
    PQ_IP_VIP_VLPF_coef2_Size = sizeof(ST_IOCTL_VIP_VLPF_COEF_CONFIG),  //14
    PQ_IP_VIP_VLPF_dither_Size = sizeof(ST_IOCTL_VIP_VLPF_DITHER_CONFIG),  //15
    PQ_IP_VIP_Peaking_Size = sizeof(ST_IOCTL_VIP_PEAKING_ONOFFCONFIG),  //16
    PQ_IP_VIP_Peaking_band_Size = sizeof(ST_IOCTL_VIP_PEAKING_BAND_CONFIG),  //17
    PQ_IP_VIP_Peaking_adptive_Size = sizeof(ST_IOCTL_VIP_PEAKING_ADPTIVE_CONFIG),  //18
    PQ_IP_VIP_Peaking_Pcoring_Size = sizeof(ST_IOCTL_VIP_PEAKING_PCORING_CONFIG),  //19
    PQ_IP_VIP_Peaking_Pcoring_ad_Y_Size = sizeof(ST_IOCTL_VIP_PEAKING_ADP_Y_CONFIG),  //20
    PQ_IP_VIP_Peaking_gain_Size = sizeof(ST_IOCTL_VIP_PEAKING_GAIN_CONFIG),  //21
    PQ_IP_VIP_Peaking_gain_ad_Y_Size = sizeof(ST_IOCTL_VIP_PEAKING_GAIN_ADP_Y_CONFIG),  //22
    PQ_IP_VIP_LCE_Size = sizeof(ST_IOCTL_VIP_LCE_ONOFF_CONFIG),  //24
    PQ_IP_VIP_LCE_dither_Size = sizeof(ST_IOCTL_VIP_LCE_DITHER_CONFIG),  //25
    PQ_IP_VIP_LCE_setting_Size = sizeof(ST_IOCTL_VIP_LCE_SETTING_CONFIG),  //26
    PQ_IP_VIP_LCE_curve_Size = sizeof(ST_IOCTL_VIP_LCE_CRUVE_CONFIG),  //27
    PQ_IP_VIP_DLC_His_range_Size = sizeof(ST_IOCTL_VIP_DLC_HISTOGRAM_EN_CONFIG),  //30
    PQ_IP_VIP_DLC_Size = sizeof(ST_IOCTL_VIP_DLC_ENABLE_CONFIG),  //28
    PQ_IP_VIP_DLC_dither_Size = sizeof(ST_IOCTL_VIP_DLC_DITHER_CONFIG),  //29
    PQ_IP_VIP_DLC_His_rangeH_Size = sizeof(ST_IOCTL_VIP_DLC_HISTOGRAM_H_CONFIG),  //31
    PQ_IP_VIP_DLC_His_rangeV_Size = sizeof(ST_IOCTL_VIP_DLC_HISTOGRAM_V_CONFIG),  //32
    PQ_IP_VIP_DLC_PC_Size = sizeof(ST_IOCTL_VIP_DLC_PC_CONFIG),  //33
    PQ_IP_VIP_YC_gain_offset_Size = sizeof(ST_IOCTL_VIP_YC_GAIN_OFFSET_CONFIG),  //23
    PQ_IP_VIP_UVC_Size = sizeof(ST_IOCTL_VIP_UVC_MAIN_CONFIG),  //34
    PQ_IP_VIP_FCC_full_range_Size = sizeof(ST_IOCTL_VIP_FCC_FULLRANGE_CONFIG),  //35
    PQ_IP_VIP_FCC_bdry_dist_Size = 0,  //36
    PQ_IP_VIP_FCC_T1_Size = sizeof(ST_IOCTL_VIP_FCC_T_CONFIG),  //37
    PQ_IP_VIP_FCC_T2_Size = sizeof(ST_IOCTL_VIP_FCC_T_CONFIG),  //38
    PQ_IP_VIP_FCC_T3_Size = sizeof(ST_IOCTL_VIP_FCC_T_CONFIG),  //39
    PQ_IP_VIP_FCC_T4_Size = sizeof(ST_IOCTL_VIP_FCC_T_CONFIG),  //40
    PQ_IP_VIP_FCC_T5_Size = sizeof(ST_IOCTL_VIP_FCC_T_CONFIG),  //41
    PQ_IP_VIP_FCC_T6_Size = sizeof(ST_IOCTL_VIP_FCC_T_CONFIG),  //42
    PQ_IP_VIP_FCC_T7_Size = sizeof(ST_IOCTL_VIP_FCC_T_CONFIG),  //43
    PQ_IP_VIP_FCC_T8_Size = sizeof(ST_IOCTL_VIP_FCC_T_CONFIG),  //44
    PQ_IP_VIP_FCC_T9_Size = sizeof(ST_IOCTL_VIP_FCC_T9_CONFIG),  //45
    PQ_IP_VIP_IHC_Size = sizeof(ST_IOCTL_VIP_IHC_ONOFF_CONFIG),  //46
    PQ_IP_VIP_IHC_Ymode_Size = sizeof(ST_IOCTL_VIP_IHC_Ymode_CONFIG),  //47
    PQ_IP_VIP_IHC_dither_Size = sizeof(ST_IOCTL_VIP_IHC_DITHER_CONFIG),  //48
    PQ_IP_VIP_IHC_CRD_SRAM_Size = 0,  //49
    PQ_IP_VIP_IHC_SETTING_Size = sizeof(ST_IOCTL_VIP_IHC_SETTING_CONFIG),  //50
    PQ_IP_VIP_ICC_Size = sizeof(ST_IOCTL_VIP_ICC_ENABLE_CONFIG),  //51
    PQ_IP_VIP_ICC_Ymode_Size = sizeof(ST_IOCTL_VIP_ICC_Ymode_CONFIG),  //52
    PQ_IP_VIP_ICC_dither_Size = sizeof(ST_IOCTL_VIP_ICC_DITHER_CONFIG),  //53
    PQ_IP_VIP_ICC_CRD_SRAM_Size = 0,  //54
    PQ_IP_VIP_ICC_SETTING_Size = sizeof(ST_IOCTL_VIP_ICC_SETTING_CONFIG),  //55
    PQ_IP_VIP_Ymode_Yvalue_ALL_Size = sizeof(ST_IOCTL_VIP_Ymode_Yvalue_ALL_CONFIG),  //56
    PQ_IP_VIP_Ymode_Yvalue_SETTING_Size = sizeof(ST_IOCTL_VIP_Ymode_Yvalue_SETTING_CONFIG),  //57
    PQ_IP_VIP_IBC_Size = sizeof(ST_IOCTL_VIP_IBC_ENABLE_CONFIG),  //58
    PQ_IP_VIP_IBC_dither_Size = sizeof(ST_IOCTL_VIP_IBC_DITHER_CONFIG),  //59
    PQ_IP_VIP_IBC_SETTING_Size = sizeof(ST_IOCTL_VIP_IBC_SETTING_CONFIG),  //60
    PQ_IP_VIP_ACK_Size = sizeof(ST_IOCTL_VIP_ACK_MAIN_CONFIG),  //61
    PQ_IP_VIP_YCbCr_Clip_Size = sizeof(ST_IOCTL_VIP_YCbCr_Clip_MAIN_CONFIG),  //62
}PQ_IPTYPE_Size;

typedef struct
{
    unsigned long u32CMDQ_Phy;
    unsigned long u32CMDQ_Size;
    unsigned long u32CMDQ_Vir;

}ST_VIP_CMDQ_INIT_CONFIG;
typedef struct
{
    unsigned long u32RiuBase;
    ST_VIP_CMDQ_INIT_CONFIG CMDQCfg;
}ST_MDRV_VIP_INIT_CONFIG;

typedef struct
{
    unsigned long u32StructSize;
    unsigned char *pGolbalStructAddr;
    unsigned char bSetConfigFlag;
    unsigned char *pPointToCfg;
    unsigned long enPQIPType;
    unsigned char (*pfForSet)(void *);
}ST_MDRV_VIP_SETPQ_CONFIG;

typedef struct
{
    unsigned int u32StructSize;
    unsigned int *pVersion;
    unsigned int u32VersionSize;
}ST_MDRV_VIP_VERSIONCHK_CONFIG;

//-------------------------------------------------------------------------------------------------
//  Prototype
//-------------------------------------------------------------------------------------------------

#ifndef _MDRV_VIP_C
#define INTERFACE extern
#else
#define INTERFACE
#endif
INTERFACE unsigned char MDrv_VIP_Init(ST_MDRV_VIP_INIT_CONFIG *pCfg);
INTERFACE ST_MDRV_VIP_SETPQ_CONFIG MDrv_VIP_FillBasicStructSetPQCfg(EN_VIP_CONFIG_TYPE enVIPtype,void *pPointToCfg);
INTERFACE unsigned char MDrv_VIP_Sys_Init(ST_MDRV_VIP_INIT_CONFIG *pCfg);
INTERFACE void MDrv_VIP_Delete(void);
INTERFACE void MDrv_VIP_Release(void);
INTERFACE unsigned char MDrv_VIP_GetCMDQHWDone(void);
INTERFACE wait_queue_head_t * MDrv_VIP_GetWaitQueueHead(void);
INTERFACE void MDrv_VIP_SetPollWait
    (void *filp, void *pWaitQueueHead, void *pstPollQueue);
INTERFACE void MDrv_VIP_Resume(void);
INTERFACE void MDrv_VIP_SetAllVIPOneshot(ST_IOCTL_VIP_SUSPEND_CONFIG *stvipCfg);
INTERFACE void MDrv_VIP_SuspendResetFlagInit(void);
INTERFACE unsigned char MDrv_VIP_SetDNRConfig(void *pCfg);
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
INTERFACE unsigned char MDrv_VIP_CMDQWriteConfig(ST_IOCTL_VIP_CMDQ_CONFIG *pCfg);
INTERFACE unsigned char MDrv_VIP_SetLDCConfig(void *pCfg);
INTERFACE unsigned char MDrv_VIP_SetLDCmdConfig(void *pCfg);
INTERFACE unsigned char MDrv_VIP_SetLDCDmapConfig(void *pCfg);
INTERFACE unsigned char MDrv_VIP_SetLDCSRAMConfig(void *pCfg);
INTERFACE unsigned char MDrv_VIP_SetACKConfig(void *pCfg);
INTERFACE unsigned char MDrv_VIP_SetNLMConfig(void *pCfg);
INTERFACE unsigned char MDrv_VIP_SetSNRConfig(void *pCfg);
INTERFACE unsigned char MDrv_SetVIPOtherConfig(void *pCfg);
INTERFACE unsigned char MDrv_VIP_SetVIPBypassConfig(EN_VIP_CONFIG_TYPE enVIPtype);
INTERFACE unsigned char MDrv_VIP_SetNLMSRAMConfig(ST_IOCTL_VIP_NLM_SRAM_CONFIG stSRAM);
INTERFACE void MDrv_VIP_PrepareStructToCheckRegister(EN_VIP_CMDQ_CHECK_TYPE enCheckType);
INTERFACE void MDrv_VIP_CheckRegister(void);
INTERFACE void MDrv_VIP_CheckConsist(void);
INTERFACE unsigned char MDrv_VIP_VtrackSetUserDefindedSetting(unsigned char bUserDefinded, unsigned char *pu8Setting);
INTERFACE unsigned char MDrv_VIP_VtrackEnable( unsigned char u8FrameRate, EN_VIP_IOCTL_VTRACK_ENABLE_TYPE bEnable);
INTERFACE unsigned char MDrv_VIP_VtrackSetPayloadData(unsigned short u16Timecode, unsigned char u8OperatorID);
INTERFACE unsigned char MDrv_VIP_VtrackSetKey(unsigned char bUserDefinded, unsigned char *pu8Setting);
INTERFACE unsigned char MDrv_VIP_VtrackInit(void);
INTERFACE ssize_t MDrv_VIP_ProcShow(char *buf);
INTERFACE ssize_t MDrv_VIP_VIPShow(char *buf);
INTERFACE ssize_t MDrv_VIP_CMDQShow(char *buf);

#undef INTERFACE


#endif
