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
#define _MDRV_VIP_C

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include "MsCommon.h"
#include "MsTypes.h"
#include "MsOS.h"
#include "drvvip.h"
#include "drvCMDQ.h"
#include "drvPQ_Define.h"
#include "drvPQ_Declare.h"
#include "drvPQ.h"
#include "mdrv_vip_io_st.h"
#include "mdrv_scl_dbg.h"
#include "mdrv_vip.h"



//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define MDRV_CMDQ_MUTEX_LOCK(enIP,bEn)         (bEn ? Drv_CMDQ_GetModuleMutex(enIP,1) : 0)
#define MDRV_CMDQ_MUTEX_UNLOCK(enIP,bEn)       (bEn ? Drv_CMDQ_GetModuleMutex(enIP,0) : 0)
#define _IsDNRBufferAllocatedReady()        (gu8DNRBufferReadyNum)
#define _IsCMDQNeedToReturnOrigin()       (gbVIPCheckCMDQorPQ == EN_VIP_CMDQ_CHECK_RETURN_ORI)
#define _IsCMDQAlreadySetting()           (gbVIPCheckCMDQorPQ == EN_VIP_CMDQ_CHECK_ALREADY_SETINNG)
#define _IsSetCMDQ()                        (gbVIPCheckCMDQorPQ >= EN_VIP_CMDQ_CHECK_ALREADY_SETINNG)
#define _IsAutoSetting()                  (gbVIPCheckCMDQorPQ == EN_VIP_CMDQ_CHECK_AUTOSETTING\
    || gbVIPCheckCMDQorPQ == EN_VIP_CMDQ_CHECK_PQ)
#define _Set_SuspendResetFlag(u32flag)         (gstSupCfg.bresetflag |= u32flag)
#define _IsSuspendResetFlag(enType)         (gstSupCfg.bresetflag &enType)
#define _IsCheckPQorCMDQmode()      (gbVIPCheckCMDQorPQ)
#define _IsNotToCheckPQorCMDQmode()      (!gbVIPCheckCMDQorPQ)
#define _IsOpenVIPBypass()          (gu32OpenBypass)
#define _IsNotOpenVIPBypass()       (!gu32OpenBypass)
//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------
/////////////////
/// gstSupCfg
/// save data for suspend/resume and test Qmap.
////////////////
ST_IOCTL_VIP_SUSPEND_CONFIG gstSupCfg;
/////////////////
/// gbVIPCheckCMDQorPQ
/// if True ,is check PQ mode.
////////////////
unsigned char gbVIPCheckCMDQorPQ = 0;
unsigned long gu32OpenBypass = 0;


//-------------------------------------------------------------------------------------------------
//  Local Function
//-------------------------------------------------------------------------------------------------
unsigned short _MDrv_VIP_ChioceStructSizeFromPQType(MS_U8 u8PQIPIdx)
{
    MS_U16 u16DataSize;
    switch(u8PQIPIdx)
    {
        case PQ_IP_PreSNR_Main:
            u16DataSize = PQ_IP_PreSNR_Size;
            break;
        case PQ_IP_DNR_Main:
            u16DataSize = PQ_IP_DNR_Size;
            break;
        case PQ_IP_DNR_Y_Main:
            u16DataSize = PQ_IP_DNR_Y_Size;
            break;
        case PQ_IP_DNR_C_Main:
            u16DataSize = PQ_IP_DNR_C_Size;
            break;
        case PQ_IP_LDC_Main:
            u16DataSize = PQ_IP_LDC_Size;
            break;
        case PQ_IP_LDC_422_444_422_Main:
            u16DataSize = PQ_IP_LDC_422_444_422_Size;
            break;
        case PQ_IP_NLM_Main:
            u16DataSize = PQ_IP_NLM_Size;
            break;
        case PQ_IP_422to444_Main:
            u16DataSize = PQ_IP_422to444_Size;
            break;
        case PQ_IP_VIP_Main:
            u16DataSize = PQ_IP_VIP_Size;
            break;
        case PQ_IP_VIP_LineBuffer_Main:
            u16DataSize = PQ_IP_VIP_LineBuffer_Size;
            break;
        case PQ_IP_VIP_HLPF_Main:
            u16DataSize = PQ_IP_VIP_HLPF_Size;
            break;
        case PQ_IP_VIP_HLPF_dither_Main:
            u16DataSize = PQ_IP_VIP_HLPF_dither_Size;
            break;
        case PQ_IP_VIP_VLPF_coef1_Main:
        case PQ_IP_VIP_VLPF_coef2_Main:
            u16DataSize = PQ_IP_VIP_VLPF_coef1_Size;
            break;
        case PQ_IP_VIP_VLPF_dither_Main:
            u16DataSize = PQ_IP_VIP_VLPF_dither_Size;
            break;
        case PQ_IP_VIP_Peaking_Main:
            u16DataSize = PQ_IP_VIP_Peaking_Size;
            break;
        case PQ_IP_VIP_Peaking_band_Main:
            u16DataSize = PQ_IP_VIP_Peaking_band_Size;
            break;
        case PQ_IP_VIP_Peaking_adptive_Main:
            u16DataSize = PQ_IP_VIP_Peaking_adptive_Size;
            break;
        case PQ_IP_VIP_Peaking_Pcoring_Main:
            u16DataSize = PQ_IP_VIP_Peaking_Pcoring_Size;
            break;
        case PQ_IP_VIP_Peaking_Pcoring_ad_Y_Main:
            u16DataSize = PQ_IP_VIP_Peaking_Pcoring_ad_Y_Size;
            break;
        case PQ_IP_VIP_Peaking_gain_Main:
            u16DataSize = PQ_IP_VIP_Peaking_gain_Size;
            break;
        case PQ_IP_VIP_Peaking_gain_ad_Y_Main:
            u16DataSize = PQ_IP_VIP_Peaking_gain_ad_Y_Size;
            break;
        case PQ_IP_VIP_YC_gain_offset_Main:
            u16DataSize = PQ_IP_VIP_YC_gain_offset_Size;
            break;
        case PQ_IP_VIP_LCE_Main:
            u16DataSize = PQ_IP_VIP_LCE_Size;
            break;
        case PQ_IP_VIP_LCE_dither_Main:
            u16DataSize = PQ_IP_VIP_LCE_dither_Size;
            break;
        case PQ_IP_VIP_LCE_setting_Main:
            u16DataSize = PQ_IP_VIP_LCE_setting_Size;
            break;
        case PQ_IP_VIP_LCE_curve_Main:
            u16DataSize = PQ_IP_VIP_LCE_curve_Size;
            break;
        case PQ_IP_VIP_DLC_Main:
            u16DataSize = PQ_IP_VIP_DLC_Size;
            break;
        case PQ_IP_VIP_DLC_dither_Main:
            u16DataSize = PQ_IP_VIP_DLC_dither_Size;
            break;
        case PQ_IP_VIP_DLC_His_range_Main:
            u16DataSize = PQ_IP_VIP_DLC_His_range_Size;
            break;
        case PQ_IP_VIP_DLC_His_rangeH_Main:
            u16DataSize = PQ_IP_VIP_DLC_His_rangeH_Size;
            break;
        case PQ_IP_VIP_DLC_His_rangeV_Main:
            u16DataSize = PQ_IP_VIP_DLC_His_rangeV_Size;
            break;
        case PQ_IP_VIP_DLC_PC_Main:
            u16DataSize = PQ_IP_VIP_DLC_PC_Size;
            break;
        case PQ_IP_VIP_UVC_Main:
            u16DataSize = PQ_IP_VIP_UVC_Size;
            break;
        case PQ_IP_VIP_FCC_full_range_Main:
            u16DataSize = PQ_IP_VIP_FCC_full_range_Size;
            break;
        case PQ_IP_VIP_FCC_T1_Main:
        case PQ_IP_VIP_FCC_T2_Main:
        case PQ_IP_VIP_FCC_T3_Main:
        case PQ_IP_VIP_FCC_T4_Main:
        case PQ_IP_VIP_FCC_T5_Main:
        case PQ_IP_VIP_FCC_T6_Main:
        case PQ_IP_VIP_FCC_T7_Main:
        case PQ_IP_VIP_FCC_T8_Main:
            u16DataSize = PQ_IP_VIP_FCC_T1_Size;
            break;
        case PQ_IP_VIP_FCC_T9_Main:
            u16DataSize = PQ_IP_VIP_FCC_T9_Size;
            break;
        case PQ_IP_VIP_IHC_Main:
            u16DataSize = PQ_IP_VIP_IHC_Size;
            break;
        case PQ_IP_VIP_IHC_Ymode_Main:
            u16DataSize = PQ_IP_VIP_IHC_Ymode_Size;
            break;
        case PQ_IP_VIP_IHC_dither_Main:
            u16DataSize = PQ_IP_VIP_IHC_dither_Size;
            break;
        case PQ_IP_VIP_IHC_SETTING_Main:
            u16DataSize = PQ_IP_VIP_IHC_SETTING_Size;
            break;
        case PQ_IP_VIP_ICC_Main:
            u16DataSize = PQ_IP_VIP_ICC_Size;
            break;
        case PQ_IP_VIP_ICC_Ymode_Main:
            u16DataSize = PQ_IP_VIP_ICC_Ymode_Size;
            break;
        case PQ_IP_VIP_ICC_dither_Main:
            u16DataSize = PQ_IP_VIP_ICC_dither_Size;
            break;
        case PQ_IP_VIP_ICC_SETTING_Main:
            u16DataSize = PQ_IP_VIP_ICC_SETTING_Size;
            break;
        case PQ_IP_VIP_Ymode_Yvalue_ALL_Main:
            u16DataSize = PQ_IP_VIP_Ymode_Yvalue_ALL_Size;
            break;
        case PQ_IP_VIP_Ymode_Yvalue_SETTING_Main:
            u16DataSize = PQ_IP_VIP_Ymode_Yvalue_SETTING_Size;
            break;
        case PQ_IP_VIP_IBC_Main:
            u16DataSize = PQ_IP_VIP_IBC_Size;
            break;
        case PQ_IP_VIP_IBC_dither_Main:
            u16DataSize = PQ_IP_VIP_IBC_dither_Size;
            break;
        case PQ_IP_VIP_IBC_SETTING_Main:
            u16DataSize = PQ_IP_VIP_IBC_SETTING_Size;
            break;
        case PQ_IP_VIP_ACK_Main:
            u16DataSize = PQ_IP_VIP_ACK_Size;
            break;
        case PQ_IP_VIP_YCbCr_Clip_Main:
            u16DataSize = PQ_IP_VIP_YCbCr_Clip_Size;
            break;
        default:
            u16DataSize = 0;

            break;
    }
    return u16DataSize;
}
ST_MDRV_VIP_SETPQ_CONFIG _MDrv_VIP_FillPQCfgByType(MS_U8 u8PQIPIdx, MS_U8 *pData, MS_U16 u16DataSize)
{
    ST_MDRV_VIP_SETPQ_CONFIG stSetPQCfg;
    stSetPQCfg.enPQIPType    = u8PQIPIdx;
    stSetPQCfg.pPointToCfg   = pData;
    stSetPQCfg.u32StructSize = u16DataSize;
    return stSetPQCfg;
}

void _MDrv_VIP_FillstFCfgBelongGlobal(EN_VIP_CONFIG_TYPE enVIPtype,unsigned char bEn,unsigned char u8framecnt)
{
    switch(enVIPtype)
    {
        case EN_VIP_DNR_CONFIG:
                gstSupCfg.stdnr.stFCfg.bEn = bEn ;
                gstSupCfg.stdnr.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_ACK_CONFIG:
                gstSupCfg.stack.stFCfg.bEn = bEn ;
                gstSupCfg.stack.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_IBC_CONFIG:
                gstSupCfg.stibc.stFCfg.bEn = bEn ;
                gstSupCfg.stibc.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_IHCICC_CONFIG:
                gstSupCfg.stihcicc.stFCfg.bEn = bEn ;
                gstSupCfg.stihcicc.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_ICC_CONFIG:
                gstSupCfg.sticc.stFCfg.bEn = bEn ;
                gstSupCfg.sticc.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_IHC_CONFIG:
                gstSupCfg.stihc.stFCfg.bEn = bEn ;
                gstSupCfg.stihc.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_FCC_CONFIG:
                gstSupCfg.stfcc.stFCfg.bEn = bEn ;
                gstSupCfg.stfcc.stFCfg.u8framecnt = u8framecnt;
            break;
        case EN_VIP_UVC_CONFIG:
                gstSupCfg.stuvc.stFCfg.bEn = bEn ;
                gstSupCfg.stuvc.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_DLC_CONFIG:
                gstSupCfg.stdlc.stFCfg.bEn = bEn ;
                gstSupCfg.stdlc.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_DLC_HISTOGRAM_CONFIG:
                gstSupCfg.sthist.stFCfg.bEn = bEn ;
                gstSupCfg.sthist.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_LCE_CONFIG:
                gstSupCfg.stlce.stFCfg.bEn = bEn ;
                gstSupCfg.stlce.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_PEAKING_CONFIG:
                gstSupCfg.stpk.stFCfg.bEn = bEn ;
                gstSupCfg.stpk.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_LDC_MD_CONFIG:
                gstSupCfg.stldcmd.stFCfg.bEn = bEn ;
                gstSupCfg.stldcmd.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_LDC_DMAP_CONFIG:
                gstSupCfg.stldcdmap.stFCfg.bEn = bEn ;
                gstSupCfg.stldcdmap.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_LDC_SRAM_CONFIG:
                gstSupCfg.stldcsram.stFCfg.bEn = bEn ;
                gstSupCfg.stldcsram.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_NLM_CONFIG:
                gstSupCfg.stnlm.stFCfg.bEn = bEn ;
                gstSupCfg.stnlm.stFCfg.u8framecnt = u8framecnt;
            break;
        case EN_VIP_SNR_CONFIG:
                gstSupCfg.stsnr.stFCfg.bEn = bEn ;
                gstSupCfg.stsnr.stFCfg.u8framecnt = u8framecnt;
            break;
        case EN_VIP_LDC_CONFIG:
                gstSupCfg.stldc.stFCfg.bEn = bEn ;
                gstSupCfg.stldc.stFCfg.u8framecnt = u8framecnt;
            break;
        case EN_VIP_CONFIG:
                gstSupCfg.stack.stFCfg.bEn = bEn ;
                gstSupCfg.stack.stFCfg.u8framecnt = u8framecnt;
            break;
        default:
                gstSupCfg.stvip.stFCfg.bEn = 0 ;
                gstSupCfg.stvip.stFCfg.u8framecnt = 0;
            break;

    }
}
void _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_CONFIG_TYPE enVIPtype,void *pCfg)
{
    ST_MDRV_VIP_SETPQ_CONFIG stSetPQCfg;
    if(_IsNotToCheckPQorCMDQmode())
    {
        stSetPQCfg = MDrv_VIP_FillBasicStructSetPQCfg(enVIPtype,pCfg);
        MsOS_Memcpy(stSetPQCfg.pGolbalStructAddr, stSetPQCfg.pPointToCfg,stSetPQCfg.u32StructSize);
        _MDrv_VIP_FillstFCfgBelongGlobal(enVIPtype,0,0);
        _Set_SuspendResetFlag(enVIPtype);
    }
}
void _MDrv_VIP_ResetAlreadySetting(ST_MDRV_VIP_SETPQ_CONFIG stSetPQCfg)
{
    void * pvPQSetParameter;
    pvPQSetParameter = MsOS_Memalloc(stSetPQCfg.u32StructSize, GFP_KERNEL);
    MsOS_Memset(pvPQSetParameter, 0, stSetPQCfg.u32StructSize);
    stSetPQCfg.pfForSet(pvPQSetParameter);
    MsOS_MemFree(pvPQSetParameter);
}
void _MDrv_VIP_FillAutoSetStruct(ST_MDRV_VIP_SETPQ_CONFIG stSetPQCfg)
{
    void * pvPQSetParameter;
    pvPQSetParameter = MsOS_Memalloc(stSetPQCfg.u32StructSize, GFP_KERNEL);
    MsOS_Memset(pvPQSetParameter, 0xFF, stSetPQCfg.u32StructSize);
    MsOS_Memcpy(stSetPQCfg.pGolbalStructAddr, pvPQSetParameter, stSetPQCfg.u32StructSize);
    MsOS_MemFree(pvPQSetParameter);
}
void _MDrv_VIP_PrepareCheckSetting(EN_VIP_CONFIG_TYPE enVIPtype)
{
    ST_MDRV_VIP_SETPQ_CONFIG stSetPQCfg;
    unsigned char bEn,u8framecount;
    bEn = (_IsSetCMDQ()) ? 1 : 0 ;
    u8framecount = 0;
    stSetPQCfg = MDrv_VIP_FillBasicStructSetPQCfg(enVIPtype,NULL);
    if(_IsCMDQAlreadySetting())
    {
        _MDrv_VIP_ResetAlreadySetting(stSetPQCfg);
    }
    else if(_IsAutoSetting())
    {
        _MDrv_VIP_FillAutoSetStruct(stSetPQCfg);
        SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPSUP, "[VIP]%x:addr:%lx\n",enVIPtype,(unsigned long)stSetPQCfg.pGolbalStructAddr);
    }
    _MDrv_VIP_FillstFCfgBelongGlobal(enVIPtype,bEn,u8framecount);
}
void _MDrv_VIP_For_PrepareCheckSetting(void)
{
    EN_VIP_CONFIG_TYPE enVIPtype;
    for(enVIPtype =EN_VIP_ACK_CONFIG;enVIPtype<=EN_VIP_CONFIG;(enVIPtype*=2))
    {
        if(_IsSuspendResetFlag(enVIPtype))
        {
            _MDrv_VIP_PrepareCheckSetting(enVIPtype);
        }
    }
}
void _MDrv_VIP_SetCMDQAfterPollTimeoutSkip(unsigned char bskip)
{
    Drv_CMDQ_SetSkipPollWhenWaitTimeOut(EN_CMDQ_TYPE_IP0,bskip);
}
void _MDrv_VIP_WaitForCMDQDone(void)
{
    int u32Time;
    u32Time = MsOS_GetSystemTime();
    while(1)
    {
        if(Drv_CMDQ_CheckIPAlreadyDone(EN_CMDQ_TYPE_IP0))
        {
            _MDrv_VIP_SetCMDQAfterPollTimeoutSkip(0);//close no wait
            break;
        }
        else if(MsOS_Timer_DiffTimeFromNow(u32Time)>1000)
        {
            printf("[VIP]!!!!Timeout\n");
            break;
        }
    }
}
void _MDrv_VIP_For_SetEachIP(void)
{
    EN_VIP_CONFIG_TYPE enVIPtype;
    ST_MDRV_VIP_SETPQ_CONFIG stSetPQCfg;
    for(enVIPtype =EN_VIP_ACK_CONFIG;enVIPtype<=EN_VIP_CONFIG;(enVIPtype*=2))
    {
        if(_IsSuspendResetFlag(enVIPtype))
        {
            stSetPQCfg = MDrv_VIP_FillBasicStructSetPQCfg(enVIPtype,NULL);
            stSetPQCfg.pfForSet((void *)stSetPQCfg.pGolbalStructAddr);
            SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPSUP, "[MDRVVIP]%s %d \n", __FUNCTION__,enVIPtype);
        }
    }
}
void _MDrv_VIP_ResumeDumpSRAM(void)
{
    if(_IsSuspendResetFlag(EN_VIP_NLM_CONFIG))
    {
        if(gstSupCfg.stnlm.stSRAM.bEn)
        {
            MDrv_VIP_SetNLMSRAMConfig(gstSupCfg.stnlm.stSRAM);
        }
        else if(!gstSupCfg.stnlm.stSRAM.bEn && gstSupCfg.stnlm.stSRAM.u32viradr)
        {
            MDrv_VIP_SetNLMSRAMConfig(gstSupCfg.stnlm.stSRAM);
        }

    }
    Drv_VIP_SRAM_Dump();
}
void _MDrv_VIP_PrepareBypassFunctionStruct(unsigned char bBypass,ST_MDRV_VIP_SETPQ_CONFIG stSetBypassCfg)
{
    if(bBypass)
    {
        MsOS_Memset(stSetBypassCfg.pPointToCfg, 0,stSetBypassCfg.u32StructSize);
    }
    else
    {
        if(stSetBypassCfg.bSetConfigFlag)
        {
            MsOS_Memcpy(stSetBypassCfg.pPointToCfg,stSetBypassCfg.pGolbalStructAddr ,stSetBypassCfg.u32StructSize);
        }
    }
}
void _MDrv_VIP_SetCMDQStatus(unsigned char bFire,unsigned char bEn,unsigned char u8framecnt)
{
    MDRv_PQ_Set_CmdqCfg(0,bEn,u8framecnt,bFire);
    SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPSUP, "[MVIP]CMDQ:%hhd,framecnt:%hhd\n"
        ,bEn,u8framecnt);
}
void _MDrv_VIP_SetPQParameter(ST_MDRV_VIP_SETPQ_CONFIG stSetBypassCfg)
{
    MDrv_PQ_LoadSettingByData(0,stSetBypassCfg.enPQIPType,stSetBypassCfg.pPointToCfg,stSetBypassCfg.u32StructSize);
    SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[MVIP]enPQIPType:%ld,u32StructSize:%ld\n"
        ,stSetBypassCfg.enPQIPType,stSetBypassCfg.u32StructSize);
}
void _MDrv_VIP_SetPQByType(MS_U8 u8PQIPIdx, MS_U8 *pData)
{
    ST_MDRV_VIP_SETPQ_CONFIG stSetPQCfg;
    stSetPQCfg = _MDrv_VIP_FillPQCfgByType(u8PQIPIdx,pData,_MDrv_VIP_ChioceStructSizeFromPQType(u8PQIPIdx));
    _MDrv_VIP_SetPQParameter(stSetPQCfg);
}
void _MDrv_VIP_For_SetEachPQTypeByIP
    (unsigned char u8FirstType,unsigned char u8LastType,ST_IOCTL_VIP_FC_CONFIG stFCfg,MS_U8 ** pPointToData)
{
    unsigned char u8PQType;
    for(u8PQType = u8FirstType;u8PQType<=u8LastType;u8PQType++)
    {
        if(u8PQType == u8FirstType)
        {
            _MDrv_VIP_SetCMDQStatus(0,stFCfg.bEn,stFCfg.u8framecnt);
        }
        else if(u8PQType == u8LastType)
        {
            _MDrv_VIP_SetCMDQStatus(1,stFCfg.bEn,stFCfg.u8framecnt);
        }
        _MDrv_VIP_SetPQByType(u8PQType,pPointToData[u8PQType-u8FirstType]);
    }
}
unsigned char _MDrv_VIP_SetBypassIP(unsigned long bBypass,EN_VIP_CONFIG_TYPE enVIPtype)
{
    ST_MDRV_VIP_SETPQ_CONFIG stSetBypassCfg;
    stSetBypassCfg.bSetConfigFlag = _IsSuspendResetFlag(enVIPtype);
    switch(enVIPtype)
    {
        case EN_VIP_DNR_CONFIG:
                stSetBypassCfg.u32StructSize = sizeof(ST_IOCTL_VIP_DNR_ONOFF_CONFIG);
                stSetBypassCfg.pGolbalStructAddr = (unsigned char *)&gstSupCfg.stdnr.stOnOff;
                stSetBypassCfg.enPQIPType = PQ_IP_DNR_Main;
                stSetBypassCfg.pPointToCfg = MsOS_Memalloc(stSetBypassCfg.u32StructSize, GFP_KERNEL);
                _MDrv_VIP_PrepareBypassFunctionStruct(bBypass,stSetBypassCfg);
                if(stSetBypassCfg.bSetConfigFlag)
                {
                    _MDrv_VIP_SetPQParameter(stSetBypassCfg);
                }
                MsOS_MemFree(stSetBypassCfg.pPointToCfg);
            break;
        case EN_VIP_NLM_CONFIG:
                stSetBypassCfg.u32StructSize = sizeof(ST_IOCTL_VIP_NLM_MAIN_CONFIG);
                stSetBypassCfg.pGolbalStructAddr = (unsigned char *)&gstSupCfg.stnlm.stNLM;
                stSetBypassCfg.enPQIPType = PQ_IP_NLM_Main;
                stSetBypassCfg.pPointToCfg = MsOS_Memalloc(stSetBypassCfg.u32StructSize, GFP_KERNEL);
                _MDrv_VIP_PrepareBypassFunctionStruct(bBypass,stSetBypassCfg);
                if(stSetBypassCfg.bSetConfigFlag)
                {
                    _MDrv_VIP_SetPQParameter(stSetBypassCfg);
                }
                MsOS_MemFree(stSetBypassCfg.pPointToCfg);
            break;
        case EN_VIP_SNR_CONFIG:
                stSetBypassCfg.u32StructSize = sizeof(ST_IOCTL_VIP_SNR_MAIN_CONFIG);
                stSetBypassCfg.pGolbalStructAddr = (unsigned char *)&gstSupCfg.stsnr.stSNR;
                stSetBypassCfg.enPQIPType = PQ_IP_PreSNR_Main;
                stSetBypassCfg.pPointToCfg = MsOS_Memalloc(stSetBypassCfg.u32StructSize, GFP_KERNEL);
                _MDrv_VIP_PrepareBypassFunctionStruct(bBypass,stSetBypassCfg);
                if(stSetBypassCfg.bSetConfigFlag)
                {
                    _MDrv_VIP_SetPQParameter(stSetBypassCfg);
                }
                MsOS_MemFree(stSetBypassCfg.pPointToCfg);
            break;
        case EN_VIP_LDC_CONFIG:
                Drv_VIP_SetLDCOnConfig(!bBypass);
            break;
        case EN_VIP_CONFIG:
                MDRv_PQ_Set_CmdqCfg(0,0,0,0);
                stSetBypassCfg.u32StructSize = sizeof(ST_IOCTL_VIP_BYPASS_CONFIG);
                stSetBypassCfg.enPQIPType = PQ_IP_VIP_Main;
                stSetBypassCfg.pPointToCfg = MsOS_Memalloc(stSetBypassCfg.u32StructSize, GFP_KERNEL);
                MsOS_Memset(stSetBypassCfg.pPointToCfg, (bBypass) ? 0x1 :0,stSetBypassCfg.u32StructSize);
                _MDrv_VIP_SetPQParameter(stSetBypassCfg);
                MsOS_MemFree(stSetBypassCfg.pPointToCfg);
            break;
        default:
                stSetBypassCfg.u32StructSize = 0;
                stSetBypassCfg.pGolbalStructAddr = NULL;
                stSetBypassCfg.bSetConfigFlag = 0;
                stSetBypassCfg.pPointToCfg = NULL;
                stSetBypassCfg.enPQIPType = PQ_IP_SC_End_Main;
            return 0;

    }
    return 1;
}
//-------------------------------------------------------------------------------------------------
//  Function
//-------------------------------------------------------------------------------------------------
ST_MDRV_VIP_SETPQ_CONFIG MDrv_VIP_FillBasicStructSetPQCfg(EN_VIP_CONFIG_TYPE enVIPtype,void *pPointToCfg)
{
    ST_MDRV_VIP_SETPQ_CONFIG stSetPQCfg;
    stSetPQCfg.bSetConfigFlag = _IsSuspendResetFlag(enVIPtype);
    stSetPQCfg.pPointToCfg = pPointToCfg;
    switch(enVIPtype)
    {
        case EN_VIP_DNR_CONFIG:
                stSetPQCfg.u32StructSize = sizeof(ST_IOCTL_VIP_DNR_CONFIG);
                stSetPQCfg.pGolbalStructAddr = (unsigned char *)&gstSupCfg.stdnr;
                stSetPQCfg.pfForSet = MDrv_VIP_SetDNRConfig;
            break;

        case EN_VIP_ACK_CONFIG:
                stSetPQCfg.u32StructSize = sizeof(ST_IOCTL_VIP_ACK_CONFIG);
                stSetPQCfg.pGolbalStructAddr = (unsigned char *)&gstSupCfg.stack;
                stSetPQCfg.pfForSet = MDrv_VIP_SetACKConfig;
            break;

        case EN_VIP_IBC_CONFIG:
                stSetPQCfg.u32StructSize = sizeof(ST_IOCTL_VIP_IBC_CONFIG);
                stSetPQCfg.pGolbalStructAddr = (unsigned char *)&gstSupCfg.stibc;
                stSetPQCfg.pfForSet = MDrv_VIP_SetIBCConfig;
            break;

        case EN_VIP_IHCICC_CONFIG:
                stSetPQCfg.u32StructSize = sizeof(ST_IOCTL_VIP_IHCICC_CONFIG);
                stSetPQCfg.pGolbalStructAddr = (unsigned char *)&gstSupCfg.stihcicc;
                stSetPQCfg.pfForSet = MDrv_VIP_SetIHCICCADPYConfig;
            break;

        case EN_VIP_ICC_CONFIG:
                stSetPQCfg.u32StructSize = sizeof(ST_IOCTL_VIP_ICC_CONFIG);
                stSetPQCfg.pGolbalStructAddr = (unsigned char *)&gstSupCfg.sticc;
                stSetPQCfg.pfForSet = MDrv_VIP_SetICEConfig;
            break;

        case EN_VIP_IHC_CONFIG:
                stSetPQCfg.u32StructSize = sizeof(ST_IOCTL_VIP_IHC_CONFIG);
                stSetPQCfg.pGolbalStructAddr = (unsigned char *)&gstSupCfg.stihc;
                stSetPQCfg.pfForSet = MDrv_VIP_SetIHCConfig;
            break;

        case EN_VIP_FCC_CONFIG:
                stSetPQCfg.u32StructSize = sizeof(ST_IOCTL_VIP_FCC_CONFIG);
                stSetPQCfg.pGolbalStructAddr = (unsigned char *)&gstSupCfg.stfcc;
                stSetPQCfg.pfForSet = MDrv_VIP_SetFCCConfig;
            break;
        case EN_VIP_UVC_CONFIG:
                stSetPQCfg.u32StructSize = sizeof(ST_IOCTL_VIP_UVC_CONFIG);
                stSetPQCfg.pGolbalStructAddr = (unsigned char *)&gstSupCfg.stuvc;
                stSetPQCfg.pfForSet = MDrv_VIP_SetUVCConfig;
            break;

        case EN_VIP_DLC_CONFIG:
                stSetPQCfg.u32StructSize = sizeof(ST_IOCTL_VIP_DLC_CONFIG);
                stSetPQCfg.pGolbalStructAddr = (unsigned char *)&gstSupCfg.stdlc;
                stSetPQCfg.pfForSet = MDrv_VIP_SetDLCConfig;
            break;

        case EN_VIP_DLC_HISTOGRAM_CONFIG:
                stSetPQCfg.u32StructSize = sizeof(ST_IOCTL_VIP_DLC_HISTOGRAM_CONFIG);
                stSetPQCfg.pGolbalStructAddr = (unsigned char *)&gstSupCfg.sthist;
                stSetPQCfg.pfForSet = MDrv_VIP_SetHistogramConfig;
            break;

        case EN_VIP_LCE_CONFIG:
                stSetPQCfg.u32StructSize = sizeof(ST_IOCTL_VIP_LCE_CONFIG);
                stSetPQCfg.pGolbalStructAddr = (unsigned char *)&gstSupCfg.stlce;
                stSetPQCfg.pfForSet = MDrv_VIP_SetLCEConfig;
            break;

        case EN_VIP_PEAKING_CONFIG:
                stSetPQCfg.u32StructSize = sizeof(ST_IOCTL_VIP_PEAKING_CONFIG);
                stSetPQCfg.pGolbalStructAddr = (unsigned char *)&gstSupCfg.stpk;
                stSetPQCfg.pfForSet = MDrv_VIP_SetPeakingConfig;
            break;

        case EN_VIP_LDC_MD_CONFIG:
                stSetPQCfg.u32StructSize = sizeof(ST_IOCTL_VIP_LDC_MD_CONFIG);
                stSetPQCfg.pGolbalStructAddr = (unsigned char *)&gstSupCfg.stldcmd;
                stSetPQCfg.pfForSet = MDrv_VIP_SetLDCmdConfig;
            break;

        case EN_VIP_LDC_DMAP_CONFIG:
                stSetPQCfg.u32StructSize = sizeof(ST_IOCTL_VIP_LDC_DMAP_CONFIG);
                stSetPQCfg.pGolbalStructAddr = (unsigned char *)&gstSupCfg.stldcdmap;
                stSetPQCfg.pfForSet = MDrv_VIP_SetLDCDmapConfig;
            break;

        case EN_VIP_LDC_SRAM_CONFIG:
                stSetPQCfg.u32StructSize = sizeof(ST_IOCTL_VIP_LDC_SRAM_CONFIG);
                stSetPQCfg.pGolbalStructAddr = (unsigned char *)&gstSupCfg.stldcsram;
                stSetPQCfg.pfForSet = MDrv_VIP_SetLDCSRAMConfig;
            break;

        case EN_VIP_NLM_CONFIG:
                stSetPQCfg.u32StructSize = sizeof(ST_IOCTL_VIP_NLM_CONFIG);
                stSetPQCfg.pGolbalStructAddr = (unsigned char *)&gstSupCfg.stnlm;
                stSetPQCfg.pfForSet = MDrv_VIP_SetNLMConfig;
            break;
        case EN_VIP_SNR_CONFIG:
                stSetPQCfg.u32StructSize = sizeof(ST_IOCTL_VIP_SNR_CONFIG);
                stSetPQCfg.pGolbalStructAddr = (unsigned char *)&gstSupCfg.stsnr;
                stSetPQCfg.pfForSet = MDrv_VIP_SetSNRConfig;
            break;
        case EN_VIP_LDC_CONFIG:
                stSetPQCfg.u32StructSize = sizeof(ST_IOCTL_VIP_LDC_CONFIG);
                stSetPQCfg.pGolbalStructAddr = (unsigned char *)&gstSupCfg.stldc;
                stSetPQCfg.pfForSet = MDrv_VIP_SetLDCConfig;
            break;
        case EN_VIP_CONFIG:
                stSetPQCfg.u32StructSize = sizeof(ST_IOCTL_VIP_CONFIG);
                stSetPQCfg.pGolbalStructAddr = (unsigned char *)&gstSupCfg.stvip;
                stSetPQCfg.pfForSet = MDrv_SetVIPOtherConfig;
            break;
        default:
                stSetPQCfg.u32StructSize = 0;
                stSetPQCfg.pGolbalStructAddr = NULL;
                stSetPQCfg.bSetConfigFlag = 0;
                stSetPQCfg.pfForSet = NULL;
            break;

    }
    return stSetPQCfg;
}


unsigned char MDrv_VIP_Init(ST_MDRV_VIP_INIT_CONFIG *pCfg)
{
    ST_VIP_INIT_CONFIG stIniCfg;
    ST_VIP_OPEN_CONFIG stCMDQIniCfg;
    MS_PQ_Init_Info    stPQInitInfo;
    unsigned char      ret = FALSE;
    stIniCfg.u32RiuBase         = pCfg->u32RiuBase;
    stCMDQIniCfg.u32RiuBase     = pCfg->u32RiuBase;
    stCMDQIniCfg.u32CMDQ_Phy    = pCfg->CMDQCfg.u32CMDQ_Phy;
    stCMDQIniCfg.u32CMDQ_Size   = pCfg->CMDQCfg.u32CMDQ_Size;
    stCMDQIniCfg.u32CMDQ_Vir    = pCfg->CMDQCfg.u32CMDQ_Vir;
    if(Drv_VIP_Init(&stIniCfg) == 0)
    {
        SCL_ERR( "[MDRVVIP]%s, Fail\n", __FUNCTION__);
        return FALSE;
    }
    MDrv_PQ_init_RIU(pCfg->u32RiuBase);
    Drv_CMDQ_Init(stCMDQIniCfg.u32CMDQ_Phy, stCMDQIniCfg.u32CMDQ_Vir, stCMDQIniCfg.u32CMDQ_Size, stCMDQIniCfg.u32RiuBase);
    MsOS_Memset(&stPQInitInfo, 0, sizeof(MS_PQ_Init_Info));
    gu32OpenBypass = 0;
    // Init PQ
    stPQInitInfo.u16PnlWidth    = 1920;
    stPQInitInfo.u8PQBinCnt     = 0;
    stPQInitInfo.u8PQTextBinCnt = 0;
    if(MDrv_PQ_Init(&stPQInitInfo))
    {
        MDrv_PQ_DesideSrcType(PQ_MAIN_WINDOW, PQ_INPUT_SOURCE_ISP);
        //MDrv_PQ_LoadSettings(PQ_MAIN_WINDOW);
        ret = TRUE;
    }
    else
    {
        ret = FALSE;
    }
    Drv_VIP_SRAM_Dump();
    return ret;
}
void MDrv_VIP_Delete(void)
{
    Drv_CMDQ_Delete();
}
void MDrv_VIP_Release(void)
{
    Drv_CMDQ_release();
}
unsigned char MDrv_VIP_GetCMDQHWDone(void)
{
    return Drv_VIP_GetCMDQHWDone();
}

wait_queue_head_t * MDrv_VIP_GetWaitQueueHead(void)
{
    return Drv_VIP_GetWaitQueueHead();
}
void MDrv_VIP_SetPollWait
    (void *filp, void *pWaitQueueHead, void *pstPollQueue)
{
    MsOS_SetPollWait(filp, pWaitQueueHead, pstPollQueue);
}

void MDrv_VIP_SuspendResetFlagInit(void)
{
    gstSupCfg.bresetflag = 0;
}
void MDrv_VIP_SetAllVIPOneshot(ST_IOCTL_VIP_SUSPEND_CONFIG *stvipCfg)
{
    EN_VIP_CONFIG_TYPE enVIPtype;
    ST_MDRV_VIP_SETPQ_CONFIG stSetPQCfg;
    unsigned long u32StructSize = sizeof(unsigned long);
    gstSupCfg.bresetflag = stvipCfg->bresetflag;
    gstSupCfg.bresetflag &= 0x3FFFF;
    for(enVIPtype =EN_VIP_ACK_CONFIG;enVIPtype<EN_VIP_CONFIG;(enVIPtype*=2))
    {
        stSetPQCfg = MDrv_VIP_FillBasicStructSetPQCfg(enVIPtype,(void *)((unsigned long)stvipCfg+u32StructSize));
        if(_IsSuspendResetFlag(enVIPtype))
        {
            SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPSUP,
                "enVIPtype :%d pGolbalStructAddr: %lx,pPointToCfg: %lx,size:%lx\n",enVIPtype,
                (unsigned long)stSetPQCfg.pGolbalStructAddr,(unsigned long)stSetPQCfg.pPointToCfg,stSetPQCfg.u32StructSize);
            MsOS_Memcpy(stSetPQCfg.pGolbalStructAddr,stSetPQCfg.pPointToCfg,stSetPQCfg.u32StructSize);
        }
        u32StructSize += stSetPQCfg.u32StructSize;
    }
    _MDrv_VIP_For_SetEachIP();
}
void MDrv_VIP_Resume(void)
{
    _MDrv_VIP_For_SetEachIP();
    _MDrv_VIP_ResumeDumpSRAM();
}
unsigned char MDrv_VIP_Sys_Init(ST_MDRV_VIP_INIT_CONFIG *pCfg)
{
    SCL_DBG(SCL_DBG_LV_VIP(), "[MDRVVIP]%s %d\n", __FUNCTION__,__LINE__);
    Drv_CMDQ_Enable(1,0);
    Drv_VIPLDCInit();
    MDrv_VIP_VtrackInit();
    return TRUE;
}
void MDrv_VIP_SetDNRConpressForDebug(unsigned char bEn)
{
    ST_IOCTL_VIP_DNR_ONOFF_CONFIG stOnOff;
    if(bEn)
    {
        MsOS_Memcpy(&stOnOff, &gstSupCfg.stdnr.stOnOff,sizeof(ST_IOCTL_VIP_DNR_ONOFF_CONFIG));
        gu32OpenBypass = 0;
    }
    else
    {
        MsOS_Memcpy(&stOnOff, &gstSupCfg.stdnr.stOnOff,sizeof(ST_IOCTL_VIP_DNR_ONOFF_CONFIG));
        stOnOff.bEncode = 0;
        stOnOff.bDecode = 0;
        gu32OpenBypass = 1;
    }
    MDrv_PQ_LoadSettingByData(0,PQ_IP_DNR_Main,(MS_U8 *)&stOnOff,sizeof(ST_IOCTL_VIP_DNR_ONOFF_CONFIG));
}

unsigned char MDrv_VIP_SetDNRConfig(void *pvCfg)
{
    ST_IOCTL_VIP_DNR_CONFIG *pCfg = pvCfg;
    unsigned char u8PQType;
    MS_U8 *p8PQType[(PQ_IP_DNR_C_Main-PQ_IP_DNR_Main+1)];
    for(u8PQType=0;u8PQType<(PQ_IP_DNR_C_Main-PQ_IP_DNR_Main+1);u8PQType++)
    {
        p8PQType[u8PQType] = NULL;
    }

    if(_IsDNRBufferAllocatedReady())
    {
        p8PQType[2] = (MS_U8*)&(pCfg->stC);
        p8PQType[1] = (MS_U8*)&(pCfg->stY);
        p8PQType[0] = (MS_U8*)&(pCfg->stOnOff);

        MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);

        if(_IsNotOpenVIPBypass())
        {
            _MDrv_VIP_For_SetEachPQTypeByIP(PQ_IP_DNR_Main,PQ_IP_DNR_C_Main,pCfg->stFCfg,p8PQType);
            Drv_VIP_SetDNRIPMRead(pCfg->stOnOff.bEn);
        }

        MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);

        _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_DNR_CONFIG,(void *)pCfg);

        SCL_DBG(SCL_DBG_LV_VIP(),
            "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&(gstSupCfg.stdnr),gstSupCfg.bresetflag);
        return TRUE;
    }
    else
    {
        SCL_ERR( "[MDRVVIP]%s,DNR buffer not alloc \n", __FUNCTION__);
        return FALSE;
    }
}


unsigned char MDrv_VIP_SetPeakingConfig(void *pvCfg)
{
    ST_IOCTL_VIP_PEAKING_CONFIG *pCfg = pvCfg;
    MS_U8 *p8PQType[(PQ_IP_VIP_Peaking_gain_ad_Y_Main-PQ_IP_VIP_HLPF_Main+1)];
    p8PQType[0]     = (MS_U8*)&(pCfg->stHLPF);
    p8PQType[1]     = (MS_U8*)&(pCfg->stHLPFDith);
    p8PQType[2]     = (MS_U8*)&(pCfg->stVLPFcoef1);
    p8PQType[3]     = (MS_U8*)&(pCfg->stVLPFcoef2);
    p8PQType[4]     = (MS_U8*)&(pCfg->stVLPFDith);
    p8PQType[5]     = (MS_U8*)&(pCfg->stOnOff);
    p8PQType[6]     = (MS_U8*)&(pCfg->stBand);
    p8PQType[7]     = (MS_U8*)&(pCfg->stAdp);
    p8PQType[8]     = (MS_U8*)&(pCfg->stPcor);
    p8PQType[9]     = (MS_U8*)&(pCfg->stAdpY);
    p8PQType[10]    = (MS_U8*)&(pCfg->stGain);
    p8PQType[11]    = (MS_U8*)&(pCfg->stGainAdpY);

    MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
    _MDrv_VIP_For_SetEachPQTypeByIP(PQ_IP_VIP_HLPF_Main,PQ_IP_VIP_Peaking_gain_ad_Y_Main,pCfg->stFCfg,p8PQType);

    MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_PEAKING_CONFIG,(void *)pCfg);
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stpk,gstSupCfg.bresetflag);
    return TRUE;
}

unsigned char MDrv_VIP_SetHistogramConfig(void *pvCfg)
{
    ST_VIP_DLC_HISTOGRAM_CONFIG stDLCCfg;
    MS_U16 i;
    ST_IOCTL_VIP_DLC_HISTOGRAM_CONFIG *pCfg = pvCfg;
    stDLCCfg.bVariable_Section  = pCfg->bVariable_Section;
    stDLCCfg.bstat_MIU          = pCfg->bstat_MIU;
    stDLCCfg.bcurve_fit_en      = pCfg->bcurve_fit_en;
    stDLCCfg.bcurve_fit_rgb_en  = pCfg->bcurve_fit_rgb_en;
    stDLCCfg.bDLCdither_en      = pCfg->bDLCdither_en;
    stDLCCfg.bhis_y_rgb_mode_en = pCfg->bhis_y_rgb_mode_en;
    stDLCCfg.bstatic            = pCfg->bstatic;
    stDLCCfg.bRange             = pCfg->bRange;
    stDLCCfg.u16Vst             = pCfg->u16Vst;
    stDLCCfg.u16Hst             = pCfg->u16Hst;
    stDLCCfg.u16Vnd             = pCfg->u16Vnd;
    stDLCCfg.u16Hnd             = pCfg->u16Hnd;
    stDLCCfg.u8HistSft          = pCfg->u8HistSft;
    stDLCCfg.u8trig_ref_mode    = pCfg->u8trig_ref_mode;
    stDLCCfg.u32StatBase[0]     = pCfg->u32StatBase[0];
    stDLCCfg.u32StatBase[1]     = pCfg->u32StatBase[1];
    stDLCCfg.stFCfg.bEn         = pCfg->stFCfg.bEn;
    stDLCCfg.stFCfg.u8framecnt  = pCfg->stFCfg.u8framecnt;

    for(i=0;i<VIP_DLC_HISTOGRAM_SECTION_NUM;i++)
    {
        stDLCCfg.u8Histogram_Range[i] = pCfg->u8Histogram_Range[i];
    }

    MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);

    if(Drv_VIP_SetDLCHistogramConfig(stDLCCfg) == 0)
    {
        SCL_DBG(SCL_DBG_LV_VIP(), "[MDRVVIP]%s, Fail\n", __FUNCTION__);
        MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
        return FALSE;
    }

    MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_DLC_HISTOGRAM_CONFIG,(void *)pCfg);
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.sthist,gstSupCfg.bresetflag);
    return TRUE;
}

unsigned char MDrv_VIP_GetDLCHistogramReport(void *pvCfg)
{
    ST_VIP_DLC_HISTOGRAM_REPORT stDLCCfg;
    MS_U16 i;
    ST_IOCTL_VIP_DLC_HISTOGRAM_REPORT *pCfg = pvCfg;
    stDLCCfg             = Drv_VIP_GetDLCHistogramConfig();
    pCfg->u32PixelWeight = stDLCCfg.u32PixelWeight;
    pCfg->u32PixelCount  = stDLCCfg.u32PixelCount;
    pCfg->u8MaxPixel     = stDLCCfg.u8MaxPixel;
    pCfg->u8MinPixel     = stDLCCfg.u8MinPixel;
    pCfg->u8Baseidx      = stDLCCfg.u8Baseidx;
    for(i=0;i<VIP_DLC_HISTOGRAM_REPORT_NUM;i++)
    {
        stDLCCfg.u32Histogram[i]    = Drv_VIP_GetDLCHistogramReport(i);
        pCfg->u32Histogram[i]       = stDLCCfg.u32Histogram[i];
    }

    return TRUE;
}

unsigned char MDrv_VIP_SetDLCConfig(void *pvCfg)
{
    ST_IOCTL_VIP_DLC_CONFIG *pCfg = pvCfg;
    MS_U8 *p8PQType[(PQ_IP_VIP_YC_gain_offset_Main-PQ_IP_VIP_DLC_His_range_Main+1)];//add PQ_IP_VIP_YC_gain_offset_Main
    p8PQType[0] = (MS_U8*)&(pCfg->sthist);
    p8PQType[1] = (MS_U8*)&(pCfg->stEn);
    p8PQType[2] = (MS_U8*)&(pCfg->stDither);
    p8PQType[3] = (MS_U8*)&(pCfg->stHistH);
    p8PQType[4] = (MS_U8*)&(pCfg->stHistV);
    p8PQType[5] = (MS_U8*)&(pCfg->stPC);
    p8PQType[6] = (MS_U8*)&(pCfg->stGainOffset);
    MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);

    _MDrv_VIP_For_SetEachPQTypeByIP(PQ_IP_VIP_DLC_His_range_Main,PQ_IP_VIP_YC_gain_offset_Main,pCfg->stFCfg,p8PQType);

    MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_DLC_CONFIG,(void *)pCfg);
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stdlc,gstSupCfg.bresetflag);
    return TRUE;
}

unsigned char MDrv_VIP_SetLCEConfig(void *pvCfg)
{
    ST_IOCTL_VIP_LCE_CONFIG *pCfg = pvCfg;
    MS_U8 *p8PQType[(PQ_IP_VIP_LCE_curve_Main-PQ_IP_VIP_LCE_Main+1)];
    p8PQType[0] = (MS_U8*)&(pCfg->stOnOff);
    p8PQType[1] = (MS_U8*)&(pCfg->stDITHER);
    p8PQType[2] = (MS_U8*)&(pCfg->stSet);
    p8PQType[3] = (MS_U8*)&(pCfg->stCurve);

    MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
    _MDrv_VIP_For_SetEachPQTypeByIP(PQ_IP_VIP_LCE_Main,PQ_IP_VIP_LCE_curve_Main,pCfg->stFCfg,p8PQType);

    MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_LCE_CONFIG,(void *)pCfg);
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stlce,gstSupCfg.bresetflag);
    return TRUE;
}

unsigned char MDrv_VIP_SetUVCConfig(void *pvCfg)
{
    MS_U8 *pUVC = NULL;
    ST_IOCTL_VIP_UVC_CONFIG *pCfg = pvCfg;
    pUVC = (MS_U8*)&(pCfg->stUVC);
    MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);

    _MDrv_VIP_SetCMDQStatus(1,pCfg->stFCfg.bEn,pCfg->stFCfg.u8framecnt);
    _MDrv_VIP_SetPQByType(PQ_IP_VIP_UVC_Main,pUVC);

    MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_UVC_CONFIG,(void *)pCfg);
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stuvc,gstSupCfg.bresetflag);
    return TRUE;
}


unsigned char MDrv_VIP_SetIHCConfig(void *pvCfg)
{
    ST_IOCTL_VIP_IHC_CONFIG *pCfg = pvCfg;
    MS_U8 *p8PQType[(PQ_IP_VIP_IHC_dither_Main-PQ_IP_VIP_IHC_Main+2)];
    p8PQType[0] = (MS_U8*)&(pCfg->stOnOff);
    p8PQType[1] = (MS_U8*)&(pCfg->stYmd);
    p8PQType[2] = (MS_U8*)&(pCfg->stDither);
    p8PQType[3] = (MS_U8*)&(pCfg->stset);
    MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
    _MDrv_VIP_SetCMDQStatus(0,pCfg->stFCfg.bEn,pCfg->stFCfg.u8framecnt);
    _MDrv_VIP_SetPQByType(PQ_IP_VIP_IHC_SETTING_Main,p8PQType[3]);
    _MDrv_VIP_For_SetEachPQTypeByIP(PQ_IP_VIP_IHC_Main,PQ_IP_VIP_IHC_dither_Main,pCfg->stFCfg,p8PQType);

    MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_IHC_CONFIG,(void *)pCfg);
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stihc,gstSupCfg.bresetflag);
    return TRUE;
}

unsigned char MDrv_VIP_SetICEConfig(void *pvCfg)
{
    ST_IOCTL_VIP_ICC_CONFIG *pCfg = pvCfg;
    MS_U8 *p8PQType[(PQ_IP_VIP_ICC_SETTING_Main-PQ_IP_VIP_ICC_dither_Main+2)];
    p8PQType[0] = (MS_U8*)&(pCfg->stEn);
    p8PQType[1] = (MS_U8*)&(pCfg->stYmd);
    p8PQType[2] = (MS_U8*)&(pCfg->stDither);
    p8PQType[3] = (MS_U8*)&(pCfg->stSet);
    MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
    _MDrv_VIP_SetCMDQStatus(0,pCfg->stFCfg.bEn,pCfg->stFCfg.u8framecnt);
    _MDrv_VIP_SetPQByType(PQ_IP_VIP_ICC_SETTING_Main,p8PQType[3]);
    _MDrv_VIP_For_SetEachPQTypeByIP(PQ_IP_VIP_ICC_Main,PQ_IP_VIP_ICC_dither_Main,pCfg->stFCfg,p8PQType);

    MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_ICC_CONFIG,(void *)pCfg);
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.sticc,gstSupCfg.bresetflag);
    return TRUE;
}


unsigned char MDrv_VIP_SetIHCICCADPYConfig(void *pvCfg)
{
    ST_IOCTL_VIP_IHCICC_CONFIG *pCfg = pvCfg;
    MS_U8 *p8PQType[(PQ_IP_VIP_Ymode_Yvalue_SETTING_Main-PQ_IP_VIP_Ymode_Yvalue_ALL_Main+1)];
    p8PQType[0] = (MS_U8*)&(pCfg->stYmdall);
    p8PQType[1] = (MS_U8*)&(pCfg->stYmdset);
    MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);

    _MDrv_VIP_For_SetEachPQTypeByIP(PQ_IP_VIP_Ymode_Yvalue_ALL_Main,PQ_IP_VIP_Ymode_Yvalue_SETTING_Main,pCfg->stFCfg,p8PQType);

    MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_IHCICC_CONFIG,(void *)pCfg);
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stihcicc,gstSupCfg.bresetflag);
    return TRUE;
}

unsigned char MDrv_VIP_SetIBCConfig(void *pvCfg)
{
    ST_IOCTL_VIP_IBC_CONFIG *pCfg = pvCfg;
    MS_U8 *p8PQType[(PQ_IP_VIP_IBC_SETTING_Main-PQ_IP_VIP_IBC_Main+1)];
    p8PQType[0] = (MS_U8*)&(pCfg->stEn);
    p8PQType[1] = (MS_U8*)&(pCfg->stDither);
    p8PQType[2] = (MS_U8*)&(pCfg->stSet);
    MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
    _MDrv_VIP_For_SetEachPQTypeByIP(PQ_IP_VIP_IBC_Main,PQ_IP_VIP_IBC_SETTING_Main,pCfg->stFCfg,p8PQType);

    MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_IBC_CONFIG,(void *)pCfg);
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stibc,gstSupCfg.bresetflag);
    return TRUE;
}

unsigned char MDrv_VIP_SetFCCConfig(void *pvCfg)
{
    ST_IOCTL_VIP_FCC_CONFIG *pCfg = pvCfg;
    MS_U8 *p8PQType[(PQ_IP_VIP_FCC_T9_Main-PQ_IP_VIP_FCC_T1_Main+2)];
    p8PQType[9] = (MS_U8*)&(pCfg->stfr);
    p8PQType[0] = (MS_U8*)&(pCfg->stT[0]);
    p8PQType[1] = (MS_U8*)&(pCfg->stT[1]);
    p8PQType[2] = (MS_U8*)&(pCfg->stT[2]);
    p8PQType[3] = (MS_U8*)&(pCfg->stT[3]);
    p8PQType[4] = (MS_U8*)&(pCfg->stT[4]);
    p8PQType[5] = (MS_U8*)&(pCfg->stT[5]);
    p8PQType[6] = (MS_U8*)&(pCfg->stT[6]);
    p8PQType[7] = (MS_U8*)&(pCfg->stT[7]);
    p8PQType[8] = (MS_U8*)&(pCfg->stT9);
    MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
    _MDrv_VIP_SetCMDQStatus(0,pCfg->stFCfg.bEn,pCfg->stFCfg.u8framecnt);
    _MDrv_VIP_SetPQByType(PQ_IP_VIP_FCC_full_range_Main,p8PQType[9]);
    _MDrv_VIP_For_SetEachPQTypeByIP(PQ_IP_VIP_FCC_T1_Main,PQ_IP_VIP_FCC_T9_Main,pCfg->stFCfg,p8PQType);

    MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_FCC_CONFIG,(void *)pCfg);
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stfcc,gstSupCfg.bresetflag);
    return TRUE;
}


unsigned char MDrv_VIP_SetACKConfig(void *pvCfg)
{
    ST_IOCTL_VIP_ACK_CONFIG *pCfg = pvCfg;
    MS_U8 *p8PQType[(PQ_IP_VIP_YCbCr_Clip_Main-PQ_IP_VIP_ACK_Main+1)];
    p8PQType[0] = (MS_U8*)&(pCfg->stACK);
    p8PQType[1] = (MS_U8*)&(pCfg->stclip);
    MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
    _MDrv_VIP_For_SetEachPQTypeByIP(PQ_IP_VIP_ACK_Main,PQ_IP_VIP_YCbCr_Clip_Main,pCfg->stFCfg,p8PQType);

    MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_ACK_CONFIG,(void *)pCfg);
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stack,gstSupCfg.bresetflag);
    return TRUE;
}

unsigned char MDrv_VIP_SetNLMConfig(void *pvCfg)
{
    MS_U8 *stNLM = NULL;
    ST_IOCTL_VIP_NLM_CONFIG *pCfg = pvCfg;
    stNLM  = (MS_U8*)&(pCfg->stNLM);
    MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);

    if(_IsNotOpenVIPBypass())
    {
        _MDrv_VIP_SetCMDQStatus(1,pCfg->stFCfg.bEn,pCfg->stFCfg.u8framecnt);
        _MDrv_VIP_SetPQByType(PQ_IP_NLM_Main,stNLM);
    }

    MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_NLM_CONFIG,(void *)pCfg);
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stnlm,gstSupCfg.bresetflag);
    return TRUE;
}

unsigned char MDrv_VIP_SetNLMSRAMConfig(ST_IOCTL_VIP_NLM_SRAM_CONFIG stSRAM)
{
    ST_VIP_NLM_SRAM_CONFIG stCfg;
    stCfg.u32baseadr    = stSRAM.u32Baseadr;
    stCfg.bCLientEn     = stSRAM.bEn;
    stCfg.u32viradr     = stSRAM.u32viradr;
    stCfg.btrigContinue = 0;                    //single
    stCfg.u16depth      = VIP_NLM_ENTRY_NUM;    // entry
    stCfg.u16reqlen     = VIP_NLM_ENTRY_NUM;
    stCfg.u16iniaddr    = 0;
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s, flag:%hhx ,addr:%lx,addr:%lx \n", __FUNCTION__,stSRAM.bEn,stSRAM.u32viradr,stSRAM.u32Baseadr);
    Drv_VIP_SetNLMSRAMConfig(stCfg);
    return TRUE;
}

unsigned char MDrv_VIP_SetSNRConfig(void *pvCfg)
{
    MS_U8 *stSNR = NULL;
    ST_IOCTL_VIP_SNR_CONFIG *pCfg = pvCfg;
    stSNR  = (MS_U8*)&(pCfg->stSNR);
    MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);

    if(_IsNotOpenVIPBypass())
    {
        _MDrv_VIP_SetCMDQStatus(1,pCfg->stFCfg.bEn,pCfg->stFCfg.u8framecnt);
        _MDrv_VIP_SetPQByType(PQ_IP_PreSNR_Main,stSNR);
    }

    MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_SNR_CONFIG,(void *)pCfg);
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stsnr,gstSupCfg.bresetflag);
    return TRUE;
}

unsigned char MDrv_SetVIPOtherConfig(void *pvCfg)
{
    ST_IOCTL_VIP_CONFIG *pCfg = pvCfg;
    MS_U8 *p8PQType[(PQ_IP_VIP_Main-PQ_IP_422to444_Main+2)];
    p8PQType[0] = (MS_U8*)&(pCfg->st422_444);
    p8PQType[1] = (MS_U8*)&(pCfg->stBypass);
    p8PQType[2] = (MS_U8*)&(pCfg->stLB);
    MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
    _MDrv_VIP_SetCMDQStatus(0,pCfg->stFCfg.bEn,pCfg->stFCfg.u8framecnt);
    _MDrv_VIP_SetPQByType(PQ_IP_VIP_LineBuffer_Main,p8PQType[2]);
    _MDrv_VIP_For_SetEachPQTypeByIP(PQ_IP_422to444_Main,PQ_IP_VIP_Main,pCfg->stFCfg,p8PQType);

    MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_CONFIG,(void *)pCfg);
    return TRUE;
}
void MDrv_VIP_CheckRegister(void)
{
    SCL_ERR( "[MDRVVIP]%s  \n", __FUNCTION__);
    MDRv_PQ_Check_Type(0,PQ_CHECK_REG);
    MDrv_VIP_Resume();
    MDRv_PQ_Check_Type(0,PQ_CHECK_OFF);
}

void MDrv_VIP_CheckConsist(void)
{
    SCL_ERR( "[MDRVVIP]%s  \n", __FUNCTION__);
    MDRv_PQ_Check_Type(0,PQ_CHECK_SIZE);
    MDrv_VIP_Resume();
    MDRv_PQ_Check_Type(0,PQ_CHECK_OFF);
}
void MDrv_VIP_PrepareStructToCheckRegister(EN_VIP_CMDQ_CHECK_TYPE enCheckType)
{
    SCL_ERR( "[MDRVVIP]%s CHECK_TYPE:%hhd \n", __FUNCTION__,enCheckType);
    gbVIPCheckCMDQorPQ = enCheckType;
    if(_IsAutoSetting())
    {
        _Set_SuspendResetFlag(0x3FFFF);
    }
    _MDrv_VIP_For_PrepareCheckSetting();


    if(_IsCMDQNeedToReturnOrigin())
    {
        _MDrv_VIP_WaitForCMDQDone();
    }
    else
    {
        _MDrv_VIP_SetCMDQAfterPollTimeoutSkip(1);//not wait
    }
}
unsigned char MDrv_VIP_SetVIPBypassConfig(EN_VIP_CONFIG_TYPE enVIPtype)
{
    if(enVIPtype)
    {
        gu32OpenBypass |= enVIPtype;
        if(enVIPtype&EN_VIP_CONFIG)
        {
            _MDrv_VIP_SetBypassIP(enVIPtype&EN_VIP_CONFIG, EN_VIP_CONFIG);
        }
        if(enVIPtype&EN_VIP_DNR_CONFIG)
        {
            _MDrv_VIP_SetBypassIP(enVIPtype&EN_VIP_DNR_CONFIG, EN_VIP_DNR_CONFIG);
        }
        if(enVIPtype&EN_VIP_SNR_CONFIG)
        {
            _MDrv_VIP_SetBypassIP(enVIPtype&EN_VIP_SNR_CONFIG, EN_VIP_SNR_CONFIG);
        }
        if(enVIPtype&EN_VIP_NLM_CONFIG)
        {
            _MDrv_VIP_SetBypassIP(enVIPtype&EN_VIP_NLM_CONFIG, EN_VIP_NLM_CONFIG);
        }
        if(enVIPtype&EN_VIP_LDC_CONFIG)
        {
            _MDrv_VIP_SetBypassIP(enVIPtype&EN_VIP_LDC_CONFIG, EN_VIP_LDC_CONFIG);
        }
    }
    else
    {
        gu32OpenBypass = enVIPtype;
        _MDrv_VIP_SetBypassIP(enVIPtype&EN_VIP_CONFIG, EN_VIP_CONFIG);
        _MDrv_VIP_SetBypassIP(enVIPtype&EN_VIP_DNR_CONFIG, EN_VIP_DNR_CONFIG);
        _MDrv_VIP_SetBypassIP(enVIPtype&EN_VIP_SNR_CONFIG, EN_VIP_SNR_CONFIG);
        _MDrv_VIP_SetBypassIP(enVIPtype&EN_VIP_NLM_CONFIG, EN_VIP_NLM_CONFIG);
        _MDrv_VIP_SetBypassIP(enVIPtype&EN_VIP_LDC_CONFIG, EN_VIP_LDC_CONFIG);
    }
    return TRUE;
}

unsigned char MDrv_VIP_CMDQWriteConfig(ST_IOCTL_VIP_CMDQ_CONFIG *pCfg)
{
    MS_U32 u32Addr;
    MS_U16 u16Data,u16Mask;
    MS_U8 u8framecnt;
    MS_U8 bfire,bUseFrameCntCMD;

    bfire           = pCfg->bfire;
    bUseFrameCntCMD = pCfg->bCnt;
    u32Addr         = pCfg->u32Addr;
    u16Data         = pCfg->u16Data;
    u16Mask         = pCfg->u16Mask;
    u8framecnt      = pCfg->u8framecnt;
    if(bfire)
    {
        Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,bfire);
        SCL_DBG(SCL_DBG_LV_VIP(), "[MDRVVIP]%s, fire\n", __FUNCTION__);
    }
    else if(bUseFrameCntCMD)
    {
        if(Drv_CMDQ_AssignFrameWriteCmd(u32Addr,u16Data,u16Mask,u8framecnt) == 0)
        {
            SCL_DBG(SCL_DBG_LV_VIP(), "[MDRVVIP]%s,Write Fail\n", __FUNCTION__);
            return FALSE;
        }
    }
    else
    {
        if(Drv_CMDQ_WriteCmd(u32Addr,u16Data,u16Mask,0) == 0)
        {
            SCL_DBG(SCL_DBG_LV_VIP(), "[MDRVVIP]%s,No frame cmd Write Fail\n", __FUNCTION__);
            return FALSE;
        }
    }
    return TRUE;

}
unsigned char MDrv_VIP_SetLDCConfig(void *pvCfg)
{
    ST_IOCTL_VIP_LDC_CONFIG *pCfg = pvCfg;
    if(_IsDNRBufferAllocatedReady())
    {
        MS_U8 *stEn = NULL,*stmd = NULL;
        stEn = (MS_U8*)&(pCfg->stEn);
        stmd = (MS_U8*)&(pCfg->stmd);
        MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);

        _MDrv_VIP_SetCMDQStatus(1,pCfg->stFCfg.bEn,pCfg->stFCfg.u8framecnt);
        _MDrv_VIP_SetPQByType(PQ_IP_LDC_422_444_422_Main,stmd);
        Drv_VIP_SetLDCOnConfig(pCfg->stEn.bEn_ldc);

        MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
        _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_LDC_CONFIG,(void *)pCfg);
        SCL_DBG(SCL_DBG_LV_VIP(),
            "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stldc,gstSupCfg.bresetflag);
        return TRUE;
    }
    else
    {
        SCL_ERR( "[MDRVVIP]%s,DNR buffer not alloc \n", __FUNCTION__);
        return FALSE;
    }
}

unsigned char MDrv_VIP_SetLDCmdConfig(void *pvCfg)
{
    ST_IOCTL_VIP_LDC_MD_CONFIG *pCfg = pvCfg;
    if(_IsDNRBufferAllocatedReady())
    {
        ST_VIP_LDC_MD_CONFIG stLDCCfg;
        stLDCCfg.stFCfg.bEn         = pCfg->stFCfg.bEn;
        stLDCCfg.stFCfg.u8framecnt  = pCfg->stFCfg.u8framecnt;
        stLDCCfg.bEnSWMode          = pCfg->bEnSWMode;
        stLDCCfg.u8FBidx            = pCfg->u8FBidx;
        stLDCCfg.u8FBrwdiff         = pCfg->u8FBrwdiff;
        stLDCCfg.enbypass           = pCfg->enbypass;
        MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);

        if(Drv_VIP_SetLDCMdConfig(stLDCCfg) == 0)
        {
            SCL_DBG(SCL_DBG_LV_VIP(), "[MDRVVIP]%s, Fail\n", __FUNCTION__);
            MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
            return FALSE;
        }

        MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
        _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_LDC_MD_CONFIG,(void *)pCfg);
        SCL_DBG(SCL_DBG_LV_VIP(),
            "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stldcmd,gstSupCfg.bresetflag);
        return TRUE;
    }
    else
    {
        SCL_ERR( "[MDRVVIP]%s,DNR buffer not alloc \n", __FUNCTION__);
        return FALSE;
    }
}

unsigned char MDrv_VIP_SetLDCDmapConfig(void *pvCfg)
{
    ST_IOCTL_VIP_LDC_DMAP_CONFIG *pCfg = pvCfg;
    if(_IsDNRBufferAllocatedReady())
    {
        ST_VIP_LDC_DMAP_CONFIG stLDCCfg;
        stLDCCfg.bEnPowerSave       = pCfg->bEnPowerSave;
        stLDCCfg.u32DMAPaddr        = pCfg->u32DMAPaddr;
        stLDCCfg.u16DMAPWidth       = pCfg->u16DMAPWidth;
        stLDCCfg.u8DMAPoffset       = pCfg->u8DMAPoffset;
        stLDCCfg.stFCfg.bEn         = pCfg->stFCfg.bEn;
        stLDCCfg.stFCfg.u8framecnt  = pCfg->stFCfg.u8framecnt;
        MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);

        if(Drv_VIP_SetLDCDmapConfig(stLDCCfg) == 0)
        {
            SCL_DBG(SCL_DBG_LV_VIP(), "[MDRVVIP]%s, Fail\n", __FUNCTION__);
            MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
            return FALSE;
        }

        MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
        _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_LDC_DMAP_CONFIG,(void *)pCfg);
        SCL_DBG(SCL_DBG_LV_VIP(),
            "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stldcdmap,gstSupCfg.bresetflag);
        return TRUE;
    }
    else
    {
        SCL_ERR( "[MDRVVIP]%s,DNR buffer not alloc \n", __FUNCTION__);
        return FALSE;
    }
}

unsigned char MDrv_VIP_SetLDCSRAMConfig(void *pvCfg)
{
    ST_IOCTL_VIP_LDC_SRAM_CONFIG *pCfg = pvCfg;
    if(_IsDNRBufferAllocatedReady())
    {
        ST_VIP_LDC_SRAM_CONFIG stLDCCfg;
        stLDCCfg.u16SRAMhorstr      = pCfg->u16SRAMhorstr;
        stLDCCfg.u16SRAMverstr      = pCfg->u16SRAMverstr;
        stLDCCfg.u32loadhoraddr     = pCfg->u32loadhoraddr;
        stLDCCfg.u32loadveraddr     = pCfg->u32loadveraddr;
        stLDCCfg.u16SRAMhoramount   = pCfg->u16SRAMhoramount;
        stLDCCfg.u16SRAMveramount   = pCfg->u16SRAMveramount;
        stLDCCfg.stFCfg.bEn         = pCfg->stFCfg.bEn;
        stLDCCfg.stFCfg.u8framecnt  = pCfg->stFCfg.u8framecnt;
        MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);

        if(Drv_VIP_SetLDCSRAMConfig(stLDCCfg) == 0)
        {
            SCL_DBG(SCL_DBG_LV_VIP(), "[MDRVVIP]%s, Fail\n", __FUNCTION__);
            MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
            return FALSE;
        }

        MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
        _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_LDC_SRAM_CONFIG,(void *)pCfg);
        SCL_DBG(SCL_DBG_LV_VIP(),
            "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stldcsram,gstSupCfg.bresetflag);
        return TRUE;
    }
    else
    {
        SCL_ERR( "[MDRVVIP]%s,DNR buffer not alloc \n", __FUNCTION__);
        return FALSE;
    }
}

MS_BOOL MDrv_VIP_VtrackEnable( unsigned char u8FrameRate, EN_VIP_IOCTL_VTRACK_ENABLE_TYPE bEnable)
{
    Drv_VIP_VtrackEnable(u8FrameRate, bEnable);
    return 1;
}
MS_BOOL MDrv_VIP_VtrackSetPayloadData(unsigned short u16Timecode, unsigned char u8OperatorID)
{
    Drv_VIP_VtrackSetPayloadData(u16Timecode,u8OperatorID);
    return 1;
}
MS_BOOL MDrv_VIP_VtrackSetKey(unsigned char bUserDefinded, unsigned char *pu8Setting)
{
    Drv_VIP_VtrackSetKey(bUserDefinded,pu8Setting);
    return 1;
}

MS_BOOL MDrv_VIP_VtrackSetUserDefindedSetting(unsigned char bUserDefinded, unsigned char *pu8Setting)
{
    Drv_VIP_VtrackSetUserDefindedSetting(bUserDefinded,pu8Setting);
    return 1;
}
MS_BOOL MDrv_VIP_VtrackInit(void)
{
    Drv_VIP_VtrackSetUserDefindedSetting(0,NULL);
    Drv_VIP_VtrackSetPayloadData(0,0);
    Drv_VIP_VtrackSetKey(0,NULL);
    return 1;
}
ssize_t MDrv_VIP_ProcShow(char *buf)
{
	char *str = buf;
	char *end = buf + PAGE_SIZE;
    str += scnprintf(str, end - str, "========================SCL VIP STATUS======================\n");
    str += scnprintf(str, end - str, "Command       IP      Status\n");
    str += scnprintf(str, end - str, "----------------------------------------------\n");
    str += scnprintf(str, end - str, "   0      Normal ALL     %s\n",(gu32OpenBypass) ? "No" :"Yes");
    str += scnprintf(str, end - str, "   1      Bypass ALL     %s\n",(gu32OpenBypass==0x7FFFF) ? "ON" :"OFF");
    str += scnprintf(str, end - str, "   2      VIP Bypass     %s\n", Drv_VIP_GetBypassStatus(EN_VIP_CONFIG) ? "ON" :"OFF");
    str += scnprintf(str, end - str, "   3          DNR        %s\n", Drv_VIP_GetBypassStatus(EN_VIP_DNR_CONFIG) ? "Bypass" :"ON");
    str += scnprintf(str, end - str, "   4          SNR        %s\n", Drv_VIP_GetBypassStatus(EN_VIP_SNR_CONFIG) ? "Bypass" :"ON");
    str += scnprintf(str, end - str, "   5          NLM        %s\n", Drv_VIP_GetBypassStatus(EN_VIP_NLM_CONFIG) ? "Bypass" :"ON");
    str += scnprintf(str, end - str, "   6          LDC        %s\n", Drv_VIP_GetBypassStatus(EN_VIP_LDC_CONFIG) ? "Bypass" :"ON");
    str += scnprintf(str, end - str, "----------------------------------------------\n");
    str += scnprintf(str, end - str, "========================SCL VIP STATUS======================\n");
	return (str - buf);
}
ssize_t MDrv_VIP_CMDQShow(char *buf)
{
	char *str = buf;
	char *end = buf + PAGE_SIZE;
    unsigned long long u64DramMsg;
    unsigned short u16Count;
    unsigned short idx,reserve = 0;
    unsigned char bRotate;
    MS_CMDQ_Info stCMDQinfo = Drv_CMDQ_GetCMDQInformation(EN_CMDQ_TYPE_IP0);
    if(stCMDQinfo.u16WPoint >= stCMDQinfo.u16LPoint)
    {
        u16Count = stCMDQinfo.u16WPoint - stCMDQinfo.u16LPoint;
    }
    else
    {
        u16Count = stCMDQinfo.u16WPoint + stCMDQinfo.u16MaxCmdCnt-stCMDQinfo.u16LPoint;
        bRotate = 1;
    }
    str += scnprintf(str, end - str, "========================CMDQ STATUS======================\n");
    str += scnprintf(str, end - str, "---------------------- SW INFORMATION -------------------\n");
    str += scnprintf(str, end - str, "CMDQ Current CMD End Pointer      :%hd\n",stCMDQinfo.u16WPoint);
    str += scnprintf(str, end - str, "CMDQ Last Time Trigger Pointer    :%hd\n",stCMDQinfo.u16RPoint);
    str += scnprintf(str, end - str, "CMDQ Last Time Trigger COUNT      :%hd\n",u16Count);
    str += scnprintf(str, end - str, "CMDQ Fire Pointer                 :%hd\n",stCMDQinfo.u16FPoint);
    str += scnprintf(str, end - str, "CMDQ assign CMD Pointer           :%hd\n",stCMDQinfo.u16assFrame_Point);
    str += scnprintf(str, end - str, "---------------------- HW INFORMATION -------------------\n");
    str += scnprintf(str, end - str, "CMDQ Start                        :%lx\n",stCMDQinfo.PhyAddr);
    str += scnprintf(str, end - str, "CMDQ End                          :%lx\n",stCMDQinfo.PhyAddrEnd);
    str += scnprintf(str, end - str, "CMDQ Max CMD One shot             :%hd\n",stCMDQinfo.u16MaxCmdCnt);
    str += scnprintf(str, end - str, "CMDQ Non-FrameCnt CMD Buffer      :%hd\n",stCMDQinfo.u16BufCmdCnt);
    str += scnprintf(str, end - str, "---------------------- DRAM INFORMATION -----------------\n");
    for(idx=0;idx<u16Count;idx++)
    {
        if(bRotate)
        {
            if((stCMDQinfo.u16LPoint+idx+1)<=stCMDQinfo.u16MaxCmdCnt)
            {
                u64DramMsg = Drv_CMDQ_GetCMDFromPoint(stCMDQinfo.u16LPoint+idx);
            }
            else
            {
                u64DramMsg = Drv_CMDQ_GetCMDFromPoint(reserve);
                reserve++;
            }
            str += scnprintf(str, end - str, "%hd:%llx => %lx\n",idx,u64DramMsg,Drv_CMDQ_GetCMDBankFromCMD(u64DramMsg));
        }
        else
        {
            u64DramMsg = Drv_CMDQ_GetCMDFromPoint(stCMDQinfo.u16LPoint+idx);
            str += scnprintf(str, end - str, "%hd:%llx => %lx\n",idx,u64DramMsg,Drv_CMDQ_GetCMDBankFromCMD(u64DramMsg));
        }
    }
    str += scnprintf(str, end - str, "========================CMDQ STATUS======================\n");
	return (str - buf);
}
ssize_t MDrv_VIP_VIPShow(char *buf)
{
	char *str = buf;
	char *end = buf + PAGE_SIZE;
    str += scnprintf(str, end - str, "========================SCL VIP STATUS======================\n");
    str += scnprintf(str, end - str, "     IP      Status\n");
    str += scnprintf(str, end - str, "---------------------------------------\n");
    str += scnprintf(str, end - str, "  Normal ALL     %s\n",(gu32OpenBypass) ? "No" :"Yes");
    str += scnprintf(str, end - str, "  Bypass ALL     %s\n",(gu32OpenBypass==0x7FFFF) ? "ON" :"OFF");
    str += scnprintf(str, end - str, "  VIP Bypass     %s\n", Drv_VIP_GetBypassStatus(EN_VIP_CONFIG) ? "ON" :"OFF");
    str += scnprintf(str, end - str, "     DNR         %s\n", Drv_VIP_GetBypassStatus(EN_VIP_DNR_CONFIG) ? "Bypass" :"ON");
    str += scnprintf(str, end - str, "     SNR         %s\n", Drv_VIP_GetBypassStatus(EN_VIP_SNR_CONFIG) ? "Bypass" :"ON");
    str += scnprintf(str, end - str, "     NLM         %s\n", Drv_VIP_GetBypassStatus(EN_VIP_NLM_CONFIG) ? "Bypass" :"ON");
    str += scnprintf(str, end - str, "     LDC         %s\n", Drv_VIP_GetBypassStatus(EN_VIP_LDC_CONFIG) ? "Bypass" :"ON");
    str += scnprintf(str, end - str, "     ACK         %s\n", gstSupCfg.stack.stACK.backen ? "ON" :"OFF");
    str += scnprintf(str, end - str, "     IBC         %s\n", gstSupCfg.stibc.stEn.bIBC_en ? "ON" :"OFF");
    str += scnprintf(str, end - str, "     ICC         %s\n", gstSupCfg.sticc.stEn.bICC_en ? "ON" :"OFF");
    str += scnprintf(str, end - str, "     IHC         %s\n", gstSupCfg.stihc.stOnOff.bIHC_en? "ON" :"OFF");
    str += scnprintf(str, end - str, "     FCC1        %s\n", gstSupCfg.stfcc.stT[0].bEn ? "ON" :"OFF");
    str += scnprintf(str, end - str, "     FCC2        %s\n", gstSupCfg.stfcc.stT[1].bEn ? "ON" :"OFF");
    str += scnprintf(str, end - str, "     FCC3        %s\n", gstSupCfg.stfcc.stT[2].bEn ? "ON" :"OFF");
    str += scnprintf(str, end - str, "     FCC4        %s\n", gstSupCfg.stfcc.stT[3].bEn ? "ON" :"OFF");
    str += scnprintf(str, end - str, "     FCC5        %s\n", gstSupCfg.stfcc.stT[4].bEn ? "ON" :"OFF");
    str += scnprintf(str, end - str, "     FCC6        %s\n", gstSupCfg.stfcc.stT[5].bEn ? "ON" :"OFF");
    str += scnprintf(str, end - str, "     FCC7        %s\n", gstSupCfg.stfcc.stT[6].bEn ? "ON" :"OFF");
    str += scnprintf(str, end - str, "     FCC8        %s\n", gstSupCfg.stfcc.stT[7].bEn ? "ON" :"OFF");
    str += scnprintf(str, end - str, "     FCC9        %s\n", gstSupCfg.stfcc.stT9.bEn ? "ON" :"OFF");
    str += scnprintf(str, end - str, "     UVC         %s\n", gstSupCfg.stuvc.stUVC.buvc_en? "ON" :"OFF");
    str += scnprintf(str, end - str, " DLC CURVEFITPW  %s\n", gstSupCfg.stdlc.stEn.bcurve_fit_var_pw_en? "ON" :"OFF");
    str += scnprintf(str, end - str, " DLC  CURVEFIT   %s\n", gstSupCfg.stdlc.stEn.bcurve_fit_en? "ON" :"OFF");
    str += scnprintf(str, end - str, " DLC  STATISTIC  %s\n", gstSupCfg.stdlc.stEn.bstatistic_en? "ON" :"OFF");
    str += scnprintf(str, end - str, "     LCE         %s\n", gstSupCfg.stlce.stOnOff.bLCE_En ? "ON" :"OFF");
    str += scnprintf(str, end - str, "     PK          %s\n", gstSupCfg.stpk.stOnOff.bpost_peaking_en ? "ON" :"OFF");
    str += scnprintf(str, end - str, "========================SCL VIP STATUS======================\n");
	return (str - buf);
}
