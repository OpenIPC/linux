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

//#include <linux/module.h>
//#include <linux/kernel.h>
//#include <linux/delay.h>
//#include <linux/slab.h>
//#include <asm/uaccess.h>
#include "MsCommon.h"
#include "MsTypes.h"
#include "MsOS.h"
#include "drvvip.h"
#include "drvCMDQ.h"
#include "drvPQ_Define.h"
#include "drvPQ_Declare.h"
#include "drvPQ.h"
#include "ms_platform.h"
#include "mdrv_scl_dbg.h"
#include "mdrv_vip_st.h"
#include "mdrv_vip.h"
#include "drvsclirq_st.h"
#include "drvsclirq.h"
#include "hwreg.h"



//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define MDRV_CMDQ_MUTEX_LOCK(enIP,bEn)         (bEn ? Drv_CMDQ_GetModuleMutex(enIP,1) : 0)
#define MDRV_CMDQ_MUTEX_UNLOCK(enIP,bEn)       (bEn ? Drv_CMDQ_GetModuleMutex(enIP,0) : 0)
#define MDRV_VIP_MUTEX_LOCK()            MsOS_ObtainMutex(_MVIP_Mutex,MSOS_WAIT_FOREVER)
#define MDRV_VIP_MUTEX_UNLOCK()          MsOS_ReleaseMutex(_MVIP_Mutex)
#define _IsFrameBufferAllocatedReady()        (gu8FrameBufferReadyNum &0x3)
#define _IsCIIRBufferAllocatedReady()        (gu8FrameBufferReadyNum &0xC)
#define _IsCMDQNeedToReturnOrigin()       (gbVIPCheckCMDQorPQ == EN_VIP_CMDQ_CHECK_RETURN_ORI)
#define _IsCMDQAlreadySetting()           (gbVIPCheckCMDQorPQ == EN_VIP_CMDQ_CHECK_ALREADY_SETINNG)
#define _IsSetCMDQ()                        (gbVIPCheckCMDQorPQ >= EN_VIP_CMDQ_CHECK_ALREADY_SETINNG)
#define _IsAutoSetting()                  (gbVIPCheckCMDQorPQ == EN_VIP_CMDQ_CHECK_AUTOSETTING\
    || gbVIPCheckCMDQorPQ == EN_VIP_CMDQ_CHECK_PQ)
#define _Set_SuspendResetFlag(u32flag)         (gstSupCfg.bresetflag |= u32flag)
#define _IsSuspendResetFlag(enType)         ((gstSupCfg.bresetflag &enType)>0)
#define _Set_SuspendAIPResetFlag(u32flag)         (gstSupCfg.bAIPreflag |= (0x1 << u32flag))
#define _IsSuspendAIPResetFlag(enType)         (gstSupCfg.bAIPreflag &(0x1 << enType))
#define _Set_AIPBypassFlag(u32flag)         (gu32AIPOpenBypass |= (0x1 << u32flag))
#define _Is_AIPBypassFlag(u32flag)         (gu32AIPOpenBypass &(0x1 << u32flag))
#define _IsCheckPQorCMDQmode()      (gbVIPCheckCMDQorPQ)
#define _IsNotToCheckPQorCMDQmode()      (!gbVIPCheckCMDQorPQ)
#define _IsOpenVIPBypass()          (gu32OpenBypass)
#define _IsNotOpenVIPBypass(enType)       (!(gu32OpenBypass & enType))
#define AIPOffset PQ_IP_YEE_Main
#define _GetAIPOffset(u32Type) (u32Type +AIPOffset)
#define _IsVIPCMDQSetFlag(enType)         (gbVIPCMDQSet &enType)
#define _IsAIPCMDQSetFlag(enType)         (gbAIPCMDQSet &(0x1 << enType))
#define _Set_VIPCMDQSetFlag(u32flag)         (gbVIPCMDQSet |= u32flag)
#define _Set_AIPCMDQSetFlag(u32flag)         (gbAIPCMDQSet |= (0x1 << u32flag))
#define _ReSet_VIPCMDQSetFlag(u32flag)         (gbVIPCMDQSet &= ~(u32flag))
#define _ReSet_AIPCMDQSetFlag(u32flag)         (gbAIPCMDQSet &= ~(0x1 << (u32flag)))


//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------
/////////////////
/// gstSupCfg
/// save data for suspend/resume and test Qmap.
////////////////
ST_MDRV_VIP_SUSPEND_CONFIG gstSupCfg;
/////////////////
/// gbVIPCheckCMDQorPQ
/// if True ,is check PQ mode.
////////////////
unsigned char gbVIPCheckCMDQorPQ = 0;
unsigned long gu32OpenBypass = 0;
unsigned long gu32AIPOpenBypass = 0;
unsigned long gbVIPCMDQSet;
unsigned long gbAIPCMDQSet;
unsigned char gbAlreadyOpenGamma = 0;
VIPDAZAEvent genVipDazaEvent;
MS_S32 _MVIP_Mutex = -1;
MS_S32 _MVIP_TASK = -1;
MS_BOOL b_MVIP_TASKEn = 1;

//-------------------------------------------------------------------------------------------------
//  Local Function
//-------------------------------------------------------------------------------------------------
void _MDrv_VIP_SetDAZATaskWork(VIPDAZAEvent enEvent)
{
    genVipDazaEvent |= enEvent;
    MsOS_SetTaskWork(_MVIP_TASK);
    //SCL_DBGERR("[VIP]%s :task:%hhx\n",__FUNCTION__,bret);
}

unsigned short _MDrv_VIP_ChioceStructSizeFromPQType(MS_U8 u8PQIPIdx)
{
    MS_U16 u16DataSize;
    switch(u8PQIPIdx)
    {
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
void _MDrv_VIP_FillPQCfgByType
    (MS_U8 u8PQIPIdx, MS_U8 *pData, MS_U16 u16DataSize,ST_MDRV_VIP_SETPQ_CONFIG *stSetPQCfg)
{
    stSetPQCfg->enPQIPType    = u8PQIPIdx;
    stSetPQCfg->pPointToCfg   = pData;
    stSetPQCfg->u32StructSize = u16DataSize;
}
unsigned char MDrv_VIP_GetIsBlankingRegion(void)
{
    return Drv_VIP_GetIsBlankingRegion();
}

void _MDrv_VIP_FillstFCfgBelongGlobal(EN_VIP_MDRV_CONFIG_TYPE enVIPtype,unsigned char bEn,unsigned char u8framecnt)
{
    switch(enVIPtype)
    {
        case EN_VIP_MDRV_MCNR_CONFIG:
                gstSupCfg.stmcnr.stFCfg.bEn = bEn ;
                gstSupCfg.stmcnr.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_MDRV_ACK_CONFIG:
                gstSupCfg.stack.stFCfg.bEn = bEn ;
                gstSupCfg.stack.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_MDRV_IBC_CONFIG:
                gstSupCfg.stibc.stFCfg.bEn = bEn ;
                gstSupCfg.stibc.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_MDRV_IHCICC_CONFIG:
                gstSupCfg.stihcicc.stFCfg.bEn = bEn ;
                gstSupCfg.stihcicc.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_MDRV_ICC_CONFIG:
                gstSupCfg.sticc.stFCfg.bEn = bEn ;
                gstSupCfg.sticc.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_MDRV_IHC_CONFIG:
                gstSupCfg.stihc.stFCfg.bEn = bEn ;
                gstSupCfg.stihc.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_MDRV_FCC_CONFIG:
                gstSupCfg.stfcc.stFCfg.bEn = bEn ;
                gstSupCfg.stfcc.stFCfg.u8framecnt = u8framecnt;
            break;
        case EN_VIP_MDRV_UVC_CONFIG:
                gstSupCfg.stuvc.stFCfg.bEn = bEn ;
                gstSupCfg.stuvc.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_MDRV_DLC_CONFIG:
                gstSupCfg.stdlc.stFCfg.bEn = bEn ;
                gstSupCfg.stdlc.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_MDRV_DLC_HISTOGRAM_CONFIG:
                gstSupCfg.sthist.stFCfg.bEn = bEn ;
                gstSupCfg.sthist.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_MDRV_LCE_CONFIG:
                gstSupCfg.stlce.stFCfg.bEn = bEn ;
                gstSupCfg.stlce.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_MDRV_PEAKING_CONFIG:
                gstSupCfg.stpk.stFCfg.bEn = bEn ;
                gstSupCfg.stpk.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_MDRV_LDC_MD_CONFIG:
                gstSupCfg.stldcmd.stFCfg.bEn = bEn ;
                gstSupCfg.stldcmd.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_MDRV_LDC_DMAP_CONFIG:
                gstSupCfg.stldcdmap.stFCfg.bEn = bEn ;
                gstSupCfg.stldcdmap.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_MDRV_LDC_SRAM_CONFIG:
                gstSupCfg.stldcsram.stFCfg.bEn = bEn ;
                gstSupCfg.stldcsram.stFCfg.u8framecnt = u8framecnt;
            break;

        case EN_VIP_MDRV_NLM_CONFIG:
                gstSupCfg.stnlm.stFCfg.bEn = bEn ;
                gstSupCfg.stnlm.stFCfg.u8framecnt = u8framecnt;
            break;
        case EN_VIP_MDRV_LDC_CONFIG:
                gstSupCfg.stldc.stFCfg.bEn = bEn ;
                gstSupCfg.stldc.stFCfg.u8framecnt = u8framecnt;
            break;
        case EN_VIP_MDRV_CONFIG:
                gstSupCfg.stack.stFCfg.bEn = bEn ;
                gstSupCfg.stack.stFCfg.u8framecnt = u8framecnt;
            break;
        default:
                gstSupCfg.stvip.stFCfg.bEn = 0 ;
                gstSupCfg.stvip.stFCfg.u8framecnt = 0;
            break;

    }
}
void _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_MDRV_CONFIG_TYPE enVIPtype,void *pCfg)
{
    ST_MDRV_VIP_SETPQ_CONFIG stSetPQCfg;
    MsOS_Memset(&stSetPQCfg,0,sizeof(ST_MDRV_VIP_SETPQ_CONFIG));
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDrv_VIP_FillBasicStructSetPQCfg(enVIPtype,pCfg,&stSetPQCfg);
        MsOS_Memcpy(stSetPQCfg.pGolbalStructAddr, stSetPQCfg.pPointToCfg,stSetPQCfg.u32StructSize);
        _MDrv_VIP_FillstFCfgBelongGlobal(enVIPtype,0,0);
        _Set_SuspendResetFlag(enVIPtype);
    }
}
void _MDrv_VIP_ResetAlreadySetting(ST_MDRV_VIP_SETPQ_CONFIG *stSetPQCfg)
{
    void * pvPQSetParameter;
    pvPQSetParameter = MsOS_Memalloc(stSetPQCfg->u32StructSize, GFP_KERNEL);
    if(!pvPQSetParameter)
    {
        SCL_ERR("[MDRVIP]%s(%d) Init pvPQSetParameter Fail\n", __FUNCTION__, __LINE__);
        return ;
    }
    MsOS_Memset(pvPQSetParameter, 0, stSetPQCfg->u32StructSize);
    stSetPQCfg->pfForSet(pvPQSetParameter);
    MsOS_MemFree(pvPQSetParameter);
}
void _MDrv_VIP_ModifyAIPAlreadySetting(EN_VIP_MDRV_AIP_TYPE enAIPtype, EN_VIP_RESET_TYPE enRe)
{
    ST_MDRV_VIP_AIP_CONFIG staip;
    unsigned char bSet;
    unsigned long u32viraddr;
    MsOS_Memset(&staip,0,sizeof(ST_MDRV_VIP_AIP_CONFIG));
    bSet = (enRe == EN_VIP_RESET_ZERO) ? 0 : 0xFF;
    staip = gstSupCfg.staip[enAIPtype];
    staip.u32Viraddr =(unsigned long)MsOS_VirMemalloc(MDrv_PQ_GetIPRegCount(_GetAIPOffset(enAIPtype)));
    if(!staip.u32Viraddr)
    {
        SCL_ERR("[MDRVIP]%s(%d) Init staip.u32Viraddr Fail\n", __FUNCTION__, __LINE__);
        return ;
    }
    MsOS_Memset((void*)staip.u32Viraddr, bSet, MDrv_PQ_GetIPRegCount(_GetAIPOffset(enAIPtype)));
    if(enRe == EN_VIP_RESET_ZERO)
    {
        MDrv_VIP_SetAIPConfig(&staip);
    }
    else if(enRe == EN_VIP_RESET_ALREADY)
    {
        if(gstSupCfg.staip[enAIPtype].u32Viraddr)
        {
            MDrv_VIP_SetAIPConfig(&gstSupCfg.staip[enAIPtype]);
        }
    }
    else if (enRe == EN_VIP_FILL_GOLBAL_FULL)
    {
        u32viraddr = gstSupCfg.staip[enAIPtype].u32Viraddr;
        gstSupCfg.staip[enAIPtype].u32Viraddr = staip.u32Viraddr;
        gstSupCfg.staip[enAIPtype].u16AIPType = enAIPtype;
        staip.u32Viraddr = u32viraddr;
    }
    MsOS_VirMemFree((void *)staip.u32Viraddr);
}
void _MDrv_VIP_FillAutoSetStruct(ST_MDRV_VIP_SETPQ_CONFIG *stSetPQCfg)
{
    void * pvPQSetParameter;
    pvPQSetParameter = MsOS_Memalloc(stSetPQCfg->u32StructSize, GFP_KERNEL);
    if(!pvPQSetParameter)
    {
        SCL_ERR("[MDRVIP]%s(%d) Init pvPQSetParameter Fail\n", __FUNCTION__, __LINE__);
        return ;
    }
    MsOS_Memset(pvPQSetParameter, 0xFF, stSetPQCfg->u32StructSize);
    MsOS_Memcpy(stSetPQCfg->pGolbalStructAddr, pvPQSetParameter, stSetPQCfg->u32StructSize);
    MsOS_MemFree(pvPQSetParameter);
}
void _Mdrv_VIP_ModifyMCNRAlreadySetting(EN_VIP_RESET_TYPE enRe)
{
    ST_MDRV_VIP_MCNR_CONFIG stmcnr;
    unsigned char bSet;
    unsigned long u32viraddr;
    SCL_ERR("[MVIP]%s",__FUNCTION__);
    MsOS_Memset(&stmcnr,0,sizeof(ST_MDRV_VIP_MCNR_CONFIG));
    bSet = (enRe == EN_VIP_RESET_ZERO) ? 0 : 0xFF;
    stmcnr = gstSupCfg.stmcnr;
    stmcnr.u32Viraddr = (unsigned long)MsOS_VirMemalloc(MDrv_PQ_GetIPRegCount(PQ_IP_MCNR_Main));
    if(!stmcnr.u32Viraddr)
    {
        SCL_ERR("[MDRVIP]%s(%d) Init stmcnr.u32Viraddr Fail\n", __FUNCTION__, __LINE__);
        return ;
    }
    stmcnr.bEnMCNR = (enRe == EN_VIP_RESET_ZERO) ? 0 :
                     (enRe == EN_VIP_FILL_GOLBAL_FULL) ? 1 :
                        gstSupCfg.stmcnr.bEnMCNR;

    stmcnr.bEnCIIR = (enRe == EN_VIP_RESET_ZERO) ? 0 :
                     (enRe == EN_VIP_FILL_GOLBAL_FULL) ? 1 :
                        gstSupCfg.stmcnr.bEnCIIR;
    stmcnr.stFCfg.bEn = 0;
    stmcnr.stFCfg.u8framecnt = 0;
    MsOS_Memset((void*)stmcnr.u32Viraddr,  bSet, MDrv_PQ_GetIPRegCount(PQ_IP_MCNR_Main));
    MDRV_VIP_MUTEX_LOCK();
    gu32OpenBypass |= EN_VIP_MDRV_MCNR_CONFIG;
    MDRV_VIP_MUTEX_UNLOCK();
    if(enRe == EN_VIP_RESET_ZERO)
    {
        MDrv_VIP_SetMCNRConfig(&stmcnr);
    }
    else if(enRe == EN_VIP_RESET_ALREADY)
    {
        MDrv_VIP_SetMCNRConfig(&gstSupCfg.stmcnr);
    }
    else if (enRe == EN_VIP_FILL_GOLBAL_FULL)
    {
        u32viraddr = gstSupCfg.stmcnr.u32Viraddr;
        gstSupCfg.stmcnr.u32Viraddr = stmcnr.u32Viraddr;
        stmcnr.u32Viraddr = u32viraddr;
        gstSupCfg.stmcnr.bEnMCNR = stmcnr.bEnMCNR;
        gstSupCfg.stmcnr.bEnCIIR = stmcnr.bEnCIIR;
    }
    MsOS_VirMemFree((void *)stmcnr.u32Viraddr);
}
void _MDrv_VIP_PrepareCheckSetting(EN_VIP_MDRV_CONFIG_TYPE enVIPtype)
{
    ST_MDRV_VIP_SETPQ_CONFIG stSetPQCfg;
    unsigned char bEn,u8framecount;
    MsOS_Memset(&stSetPQCfg,0,sizeof(ST_MDRV_VIP_SETPQ_CONFIG));
    bEn = (_IsSetCMDQ()) ? 1 : 0 ;
    u8framecount = 0;
    MDrv_VIP_FillBasicStructSetPQCfg(enVIPtype,NULL,&stSetPQCfg);
    if(_IsCMDQAlreadySetting())
    {
        if(enVIPtype ==EN_VIP_MDRV_MCNR_CONFIG)
        {
            _Mdrv_VIP_ModifyMCNRAlreadySetting(EN_VIP_RESET_ZERO);
        }
        else
        {
            _MDrv_VIP_ResetAlreadySetting(&stSetPQCfg);
        }
    }
    else if(_IsAutoSetting())
    {
        if(enVIPtype ==EN_VIP_MDRV_MCNR_CONFIG)
        {
            _Mdrv_VIP_ModifyMCNRAlreadySetting(EN_VIP_FILL_GOLBAL_FULL);
        }
        else
        {
            _MDrv_VIP_FillAutoSetStruct(&stSetPQCfg);
            SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPSUP,
                "[VIP]%x:addr:%lx\n",enVIPtype,(unsigned long)stSetPQCfg.pGolbalStructAddr);
        }
    }
    _MDrv_VIP_FillstFCfgBelongGlobal(enVIPtype,bEn,u8framecount);
}
void _MDrv_VIP_PrepareAIPCheckSetting(EN_VIP_MDRV_AIP_TYPE enAIPtype)
{
    unsigned char bEn,u8framecount;
    bEn = (_IsSetCMDQ()) ? 1 : 0 ;
    u8framecount = 0;
    if(_IsCMDQAlreadySetting())
    {
        _MDrv_VIP_ModifyAIPAlreadySetting(enAIPtype,EN_VIP_RESET_ZERO);
    }
    else if(_IsAutoSetting())
    {
        _MDrv_VIP_ModifyAIPAlreadySetting(enAIPtype,EN_VIP_FILL_GOLBAL_FULL);
    }
    gstSupCfg.staip[enAIPtype].stFCfg.bEn = bEn;
    gstSupCfg.staip[enAIPtype].stFCfg.u8framecnt = u8framecount;
}
void _MDrv_VIP_For_PrepareCheckSetting(void)
{
    EN_VIP_MDRV_CONFIG_TYPE enVIPtype;
    EN_VIP_MDRV_AIP_TYPE enAIPType;
    for(enVIPtype =EN_VIP_MDRV_ACK_CONFIG;enVIPtype<=EN_VIP_MDRV_MCNR_CONFIG;(enVIPtype*=2))
    {
        if(_IsSuspendResetFlag(enVIPtype))
        {
            _MDrv_VIP_PrepareCheckSetting(enVIPtype);
        }
    }
    for(enAIPType =EN_VIP_MDRV_AIP_YEE;enAIPType<EN_VIP_MDRV_AIP_NUM;(enAIPType++))
    {
        if(_IsSuspendAIPResetFlag(enAIPType))
        {
            _MDrv_VIP_PrepareAIPCheckSetting(enAIPType);
            SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPSUP, "[MDRVVIP]AIP: %d \n",enAIPType);
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
    EN_VIP_MDRV_CONFIG_TYPE enVIPtype;
    EN_VIP_MDRV_AIP_TYPE enAIPType;
    ST_MDRV_VIP_SETPQ_CONFIG stSetPQCfg;
    MsOS_Memset(&stSetPQCfg,0,sizeof(ST_MDRV_VIP_SETPQ_CONFIG));
    for(enVIPtype =EN_VIP_MDRV_ACK_CONFIG;enVIPtype<=EN_VIP_MDRV_MCNR_CONFIG;(enVIPtype*=2))
    {
        if(_IsSuspendResetFlag(enVIPtype))
        {
            MDrv_VIP_FillBasicStructSetPQCfg(enVIPtype,NULL,&stSetPQCfg);
            stSetPQCfg.pfForSet((void *)stSetPQCfg.pGolbalStructAddr);
            SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPSUP, "[MDRVVIP]%s %d \n", __FUNCTION__,enVIPtype);
        }
    }
    for(enAIPType =EN_VIP_MDRV_AIP_YEE;enAIPType<EN_VIP_MDRV_AIP_NUM;(enAIPType++))
    {
        if(_IsSuspendAIPResetFlag(enAIPType))
        {
            MDrv_VIP_SetAIPConfig(&gstSupCfg.staip[enAIPType]);
            SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPSUP, "[MDRVVIP]AIP: %d \n",enAIPType);
        }
    }
}
void MDrv_VIP_CheckEachIPByCMDQIST(void)
{
    EN_VIP_MDRV_CONFIG_TYPE enVIPtype;
    EN_VIP_MDRV_AIP_TYPE enAIPType;
    ST_MDRV_VIP_SETPQ_CONFIG stSetPQCfg;
    MS_U32 u32Time = 0,u32Time2 = 0;
    MsOS_Memset(&stSetPQCfg,0,sizeof(ST_MDRV_VIP_SETPQ_CONFIG));
    for(enVIPtype =EN_VIP_MDRV_ACK_CONFIG;enVIPtype<=EN_VIP_MDRV_MCNR_CONFIG;(enVIPtype*=2))
    {
        if(_IsVIPCMDQSetFlag(enVIPtype))
        {
            MDRV_VIP_MUTEX_LOCK();
            u32Time = ((MS_U32)MsOS_GetSystemTimeStamp());
            MDrv_VIP_SetCheckMode(EN_VIP_CMDQ_CHECK_PQ);
            MDRv_PQ_Check_Type(PQ_MAIN_WINDOW,PQ_CHECK_REG);
            MDrv_VIP_FillBasicStructSetPQCfg(enVIPtype,NULL,&stSetPQCfg);
            stSetPQCfg.pfForSet((void *)stSetPQCfg.pGolbalStructAddr);
            SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISR, "[MDRVVIP]%s %d \n", __FUNCTION__,enVIPtype);
            _ReSet_VIPCMDQSetFlag(enVIPtype);
            MDRv_PQ_Check_Type(PQ_MAIN_WINDOW,PQ_CHECK_OFF);
            MDrv_VIP_SetCheckMode(EN_VIP_CMDQ_CHECK_RETURN_ORI);
            u32Time2 = ((MS_U32)MsOS_GetSystemTimeStamp());
            MDRV_VIP_MUTEX_UNLOCK();
            SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISR, "[DRVSCLIRQ]check diff:%lu(ns)\n",(u32Time2-u32Time));
        }
    }
    for(enAIPType =EN_VIP_MDRV_AIP_YEE;enAIPType<EN_VIP_MDRV_AIP_NUM;(enAIPType++))
    {
        if(_IsAIPCMDQSetFlag(enAIPType))
        {
            MDRV_VIP_MUTEX_LOCK();
            u32Time = ((MS_U32)MsOS_GetSystemTimeStamp());
            MDrv_VIP_SetCheckMode(EN_VIP_CMDQ_CHECK_PQ);
            MDRv_PQ_Check_Type(PQ_MAIN_WINDOW,PQ_CHECK_REG);
            MDrv_VIP_SetAIPConfig(&gstSupCfg.staip[enAIPType]);
            SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISR, "[MDRVVIP]AIP: %d \n",enAIPType);
            _ReSet_AIPCMDQSetFlag(enAIPType);
            MDRv_PQ_Check_Type(PQ_MAIN_WINDOW,PQ_CHECK_OFF);
            MDrv_VIP_SetCheckMode(EN_VIP_CMDQ_CHECK_RETURN_ORI);
            u32Time2 = ((MS_U32)MsOS_GetSystemTimeStamp());
            MDRV_VIP_MUTEX_UNLOCK();
            SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISR, "[DRVSCLIRQ]check diff:%lu(ns)\n",(u32Time2-u32Time));
        }
    }
}

void _MDrv_VIP_ResumeDumpSRAM(void)
{
    if(_IsSuspendResetFlag(EN_VIP_MDRV_NLM_CONFIG))
    {
        if(gstSupCfg.stnlm.stSRAM.bEn)
        {
            MDrv_VIP_SetNLMSRAMConfig(&gstSupCfg.stnlm.stSRAM);
        }
        else if(!gstSupCfg.stnlm.stSRAM.bEn && gstSupCfg.stnlm.stSRAM.u32viradr)
        {
            MDrv_VIP_SetNLMSRAMConfig(&gstSupCfg.stnlm.stSRAM);
        }

    }
    Drv_VIP_SRAM_Dump();
}
void _MDrv_VIP_PrepareBypassFunctionStruct(unsigned char bBypass,ST_MDRV_VIP_SETPQ_CONFIG *stSetBypassCfg)
{
    if(bBypass)
    {
        MsOS_Memset(stSetBypassCfg->pPointToCfg, 0,stSetBypassCfg->u32StructSize);
    }
    else
    {
        if(stSetBypassCfg->bSetConfigFlag)
        {
            MsOS_Memcpy(stSetBypassCfg->pPointToCfg,stSetBypassCfg->pGolbalStructAddr ,stSetBypassCfg->u32StructSize);
        }
    }
}
unsigned char _MDrv_VIP_SetCMDQStatus(unsigned char bFire,unsigned char bEn,unsigned char u8framecnt)
{
    static unsigned char stu8ISPCnt = 0;
    static unsigned char bEnCMDQ = 0;
    unsigned char bRet = 0;
    // TODO: CMDQ
    if(_IsNotToCheckPQorCMDQmode() && (VIPSETRULE()== E_VIPSetRule_CMDQAll || VIPSETRULE()== E_VIPSetRule_CMDQAllCheck
        ||(VIPSETRULE() == E_VIPSetRule_CMDQAllONLYSRAMCheck)))
    {
        if(Drv_VIP_GetEachDMAEn())
        {
            stu8ISPCnt = Drv_CMDQ_GetISPHWCnt();
            MDRv_PQ_Set_CmdqCfg(PQ_MAIN_WINDOW,1,stu8ISPCnt,bFire);
            if(!bFire)
            {
                bEnCMDQ = 1;
            }
            else
            {
                bEnCMDQ = 0;
                stu8ISPCnt = 0;
            }
            bRet = 1;
            SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_NORMAL, "[MVIP]ALL CMDQ framecnt:%hhd\n",Drv_CMDQ_GetISPHWCnt());
        }
        else if(bFire && bEnCMDQ)
        {
            MDRv_PQ_Set_CmdqCfg(PQ_MAIN_WINDOW,bEnCMDQ,stu8ISPCnt,bFire);
            bEnCMDQ = 0;
            stu8ISPCnt = 0;
            bRet = 1;
        }
        else
        {
            MDRv_PQ_Set_CmdqCfg(PQ_MAIN_WINDOW,bEn,u8framecnt,bFire);
            SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPSUP, "[MVIP]CMDQ:%hhd,framecnt:%hhd\n"
                ,bEn,u8framecnt);
            if(bEn)
            {
                bRet = 1;
            }
        }
    }
    else if(_IsNotToCheckPQorCMDQmode() && VIPSETRULE())
    {
        if((!MDrv_VIP_GetIsBlankingRegion())&&(!bEn))
        {
            stu8ISPCnt = Drv_CMDQ_GetISPHWCnt();
            MDRv_PQ_Set_CmdqCfg(PQ_MAIN_WINDOW,1,stu8ISPCnt,bFire);
            if(!bFire)
            {
                bEnCMDQ = 1;
            }
            else
            {
                bEnCMDQ = 0;
                stu8ISPCnt = 0;
            }
            bRet = 1;
            SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_NORMAL, "[MVIP]framecnt:%hhd\n",Drv_CMDQ_GetISPHWCnt());
        }
        else if(bFire && bEnCMDQ)
        {
            MDRv_PQ_Set_CmdqCfg(PQ_MAIN_WINDOW,bEnCMDQ,stu8ISPCnt,bFire);
            bEnCMDQ = 0;
            stu8ISPCnt = 0;
            bRet = 1;
        }
        else
        {
            MDRv_PQ_Set_CmdqCfg(PQ_MAIN_WINDOW,bEn,u8framecnt,bFire);
            SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPSUP, "[MVIP]CMDQ:%hhd,framecnt:%hhd\n"
                ,bEn,u8framecnt);
            if(bEn)
            {
                bRet = 1;
            }
        }
    }
    else
    {
        MDRv_PQ_Set_CmdqCfg(PQ_MAIN_WINDOW,bEn,u8framecnt,bFire);
    }
    return bRet;
}
void _MDrv_VIP_SetPQParameter(ST_MDRV_VIP_SETPQ_CONFIG *stSetBypassCfg)
{
    MDrv_PQ_LoadSettingByData(PQ_MAIN_WINDOW,stSetBypassCfg->enPQIPType,stSetBypassCfg->pPointToCfg,stSetBypassCfg->u32StructSize);
    SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[MVIP]enPQIPType:%ld,u32StructSize:%ld\n"
        ,stSetBypassCfg->enPQIPType,stSetBypassCfg->u32StructSize);
}
void _MDrv_VIP_SetPQByType(MS_U8 u8PQIPIdx, MS_U8 *pData)
{
    ST_MDRV_VIP_SETPQ_CONFIG stSetPQCfg;
    MS_U16 u16Structsize;
    MsOS_Memset(&stSetPQCfg,0,sizeof(ST_MDRV_VIP_SETPQ_CONFIG));
    if(u8PQIPIdx>=AIPOffset || u8PQIPIdx==PQ_IP_MCNR_Main)
    {
        u16Structsize = MDrv_PQ_GetIPRegCount(u8PQIPIdx);
    }
    else
    {
        u16Structsize = _MDrv_VIP_ChioceStructSizeFromPQType(u8PQIPIdx);
    }
    _MDrv_VIP_FillPQCfgByType(u8PQIPIdx,pData,u16Structsize,&stSetPQCfg);
    SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[MVIP]Struct size:%hd,%hd\n"
        ,u16Structsize,MDrv_PQ_GetIPRegCount(u8PQIPIdx));
    _MDrv_VIP_SetPQParameter(&stSetPQCfg);
}
void _MDrv_VIP_SetMCNR(ST_MDRV_VIP_MCNR_CONFIG *pstCfg)
{
    Drv_VIP_SetMCNRIPMRead(pstCfg->bEnMCNR);
    Drv_VIP_SetIPMConpress(pstCfg->bEnMCNR);
    if(MsOS_GetEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID())&E_SCLIRQ_EVENT_BCLOSECIIR)
    {
        Drv_VIP_SetCIIRRead(0);
        Drv_VIP_SetCIIRWrite(0);
    }
    else if(_IsCIIRBufferAllocatedReady())
    {
        Drv_VIP_SetCIIRRead(pstCfg->bEnCIIR);
        Drv_VIP_SetCIIRWrite(pstCfg->bEnCIIR);
    }
    else
    {
        //SCL_DBGERR( "[MDRVVIP]%s,CIIR buffer not alloc \n", __FUNCTION__);
    }
    _MDrv_VIP_SetPQByType((MS_U8)PQ_IP_MCNR_Main,(unsigned char *)pstCfg->u32Viraddr);
}
static int VIP_DazaIST(void *arg)
{
    MS_U32 u32Events;
    VIPDAZAEvent enDAZAEvent;
    unsigned short u16AIPsheet;
    while(b_MVIP_TASKEn)
    {
        SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_NORMAL, "[DRVVIP]%s:going event:%x @:%lu\n"
            ,__FUNCTION__,genVipDazaEvent,((MS_U32)MsOS_GetSystemTimeStamp()));
        if(genVipDazaEvent)
        {
            if((MsOS_GetEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID())&(E_SCLIRQ_EVENT_ISPFRMEND|E_SCLIRQ_EVENT_FRMENDSYNC))
                !=(E_SCLIRQ_EVENT_ISPFRMEND|E_SCLIRQ_EVENT_FRMENDSYNC))
            {
                MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_ISPFRMEND, &u32Events, E_OR, 2000); // get status: FRM END
                MsOS_DelayTaskUs(100);
            }
            MDRV_VIP_MUTEX_LOCK();
            enDAZAEvent = genVipDazaEvent;
            MDRV_VIP_MUTEX_UNLOCK();
            if(enDAZAEvent &E_VIP_DAZA_BMCNR)
            {
                MDRV_VIP_MUTEX_LOCK();
                if(Drv_SCLIRQ_GetIsBlankingRegion())
                {
                    SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_NORMAL, "[DRVVIP]E_VIP_DAZA_BMCNR\n");
                    MDRv_PQ_Set_CmdqCfg(PQ_MAIN_WINDOW,0,0,1);
                    _MDrv_VIP_SetMCNR(&gstSupCfg.stmcnr);
                    genVipDazaEvent &= (~E_VIP_DAZA_BMCNR);
                }
                MDRV_VIP_MUTEX_UNLOCK();
            }
            // avoid one blanking region can't set all cmd,double switch.
            else if(enDAZAEvent &E_VIP_DAZA_BGMA2C)
            {
                u16AIPsheet = _GetAIPOffset(EN_VIP_MDRV_AIP_GM10TO12);
                MDRV_VIP_MUTEX_LOCK();
                if(Drv_SCLIRQ_GetIsBlankingRegion())
                {
                    SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_NORMAL, "[DRVVIP]E_VIP_DAZA_BGMA2C\n");
                    MDRv_PQ_Set_CmdqCfg(PQ_MAIN_WINDOW,0,0,1);
                    _Drv_CMDQ_WriteRegWithMaskDirect(REG_SCL0_01_L, BIT3, BIT3);
                    _MDrv_VIP_SetPQByType((MS_U8)u16AIPsheet,(MS_U8 *)gstSupCfg.staip[EN_VIP_MDRV_AIP_GM10TO12].u32Viraddr);
                    _Drv_CMDQ_WriteRegWithMaskDirect(REG_SCL0_01_L, 0, BIT3);
                    genVipDazaEvent &= (~E_VIP_DAZA_BGMA2C);
                }
                MDRV_VIP_MUTEX_UNLOCK();
            }
            if(enDAZAEvent &E_VIP_DAZA_BNLM)
            {
                MDRV_VIP_MUTEX_LOCK();
                if(Drv_SCLIRQ_GetIsBlankingRegion())
                {
                    SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_NORMAL, "[DRVVIP]E_VIP_DAZA_BNLM\n");
                    MDRv_PQ_Set_CmdqCfg(PQ_MAIN_WINDOW,0,0,1);
                    _MDrv_VIP_SetPQByType((MS_U8)PQ_IP_NLM_Main,(MS_U8 *)&gstSupCfg.stnlm.stNLM);
                    genVipDazaEvent &= (~E_VIP_DAZA_BNLM);
                }
                MDRV_VIP_MUTEX_UNLOCK();
            }
            else if(enDAZAEvent &E_VIP_DAZA_BGMC2A)
            {
                u16AIPsheet = _GetAIPOffset(EN_VIP_MDRV_AIP_GM12TO10);
                MDRV_VIP_MUTEX_LOCK();
                if(Drv_SCLIRQ_GetIsBlankingRegion())
                {
                    SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_NORMAL, "[DRVVIP]E_VIP_DAZA_BGMC2A\n");
                    MDRv_PQ_Set_CmdqCfg(PQ_MAIN_WINDOW,0,0,1);
                    _Drv_CMDQ_WriteRegWithMaskDirect(REG_SCL0_01_L, BIT3, BIT3);
                    _MDrv_VIP_SetPQByType((MS_U8)u16AIPsheet,(MS_U8 *)gstSupCfg.staip[EN_VIP_MDRV_AIP_GM12TO10].u32Viraddr);
                    _Drv_CMDQ_WriteRegWithMaskDirect(REG_SCL0_01_L, 0, BIT3);
                    genVipDazaEvent &= (~E_VIP_DAZA_BGMC2A);
                }
                MDRV_VIP_MUTEX_UNLOCK();
            }
            if(enDAZAEvent &E_VIP_DAZA_BXNR)
            {
                MDRV_VIP_MUTEX_LOCK();
                if(Drv_SCLIRQ_GetIsBlankingRegion())
                {
                    SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_NORMAL, "[DRVVIP]E_VIP_DAZA_BXNR\n");
                    u16AIPsheet = _GetAIPOffset(EN_VIP_MDRV_AIP_XNR);
                    MDRv_PQ_Set_CmdqCfg(PQ_MAIN_WINDOW,0,0,1);
                    _MDrv_VIP_SetPQByType((MS_U8)u16AIPsheet,(MS_U8 *)gstSupCfg.staip[EN_VIP_MDRV_AIP_XNR].u32Viraddr);
                    genVipDazaEvent &= (~E_VIP_DAZA_BXNR);
                }
                MDRV_VIP_MUTEX_UNLOCK();
            }
        }
        else
        {
            MsOS_SleepTaskWork(_MVIP_TASK);
        }
    }
    return 0;
}
unsigned char _MDrv_VIP_For_SetEachPQTypeByIP
    (unsigned char u8FirstType,unsigned char u8LastType,ST_MDRV_VIP_FC_CONFIG *stFCfg,MS_U8 ** pPointToData)
{
    unsigned char u8PQType;
    unsigned char bRet = 0;
    for(u8PQType = u8FirstType;u8PQType<=u8LastType;u8PQType++)
    {
        if(u8PQType == u8FirstType)
        {
            bRet = _MDrv_VIP_SetCMDQStatus(0,stFCfg->bEn,stFCfg->u8framecnt);
            MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,bRet);
        }
        else if(u8PQType == u8LastType)
        {
            bRet = _MDrv_VIP_SetCMDQStatus(1,stFCfg->bEn,stFCfg->u8framecnt);
        }
        _MDrv_VIP_SetPQByType(u8PQType,pPointToData[u8PQType-u8FirstType]);
        if(u8PQType == u8LastType)
        {
            MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,bRet);
        }
    }
    return bRet;
}
unsigned char _MDrv_VIP_SetBypassIP(unsigned long bBypass,EN_VIP_MDRV_CONFIG_TYPE enVIPtype)
{
    ST_MDRV_VIP_SETPQ_CONFIG stSetBypassCfg;
    MsOS_Memset(&stSetBypassCfg,0,sizeof(ST_MDRV_VIP_SETPQ_CONFIG));
    stSetBypassCfg.bSetConfigFlag = _IsSuspendResetFlag(enVIPtype);
    switch(enVIPtype)
    {
        case EN_VIP_MDRV_MCNR_CONFIG:
                MDrv_VIP_FillBasicStructSetPQCfg(enVIPtype,NULL,&stSetBypassCfg);
                if(bBypass)
                {
                    _Mdrv_VIP_ModifyMCNRAlreadySetting(EN_VIP_RESET_ZERO);
                }
                else
                {
                    stSetBypassCfg.pfForSet((void *)stSetBypassCfg.pGolbalStructAddr);
                }
            break;
        case EN_VIP_MDRV_NLM_CONFIG:
                stSetBypassCfg.u32StructSize = sizeof(ST_MDRV_VIP_NLM_MAIN_CONFIG);
                stSetBypassCfg.pGolbalStructAddr = (unsigned char *)&gstSupCfg.stnlm.stNLM;
                stSetBypassCfg.enPQIPType = PQ_IP_NLM_Main;
                stSetBypassCfg.pPointToCfg = MsOS_Memalloc(stSetBypassCfg.u32StructSize, GFP_KERNEL);
                if(!stSetBypassCfg.pPointToCfg)
                {
                    SCL_ERR("[MDRVIP]%s(%d) Init stSetBypassCfg.pPointToCfg Fail\n", __FUNCTION__, __LINE__);
                    return 0;
                }
                _MDrv_VIP_PrepareBypassFunctionStruct(bBypass,&stSetBypassCfg);
                if(stSetBypassCfg.bSetConfigFlag)
                {
                    _MDrv_VIP_SetPQParameter(&stSetBypassCfg);
                }
                MsOS_MemFree(stSetBypassCfg.pPointToCfg);
            break;
        case EN_VIP_MDRV_LDC_CONFIG:
                Drv_VIP_SetLDCOnConfig(!bBypass);
            break;
        case EN_VIP_MDRV_CONFIG:
                MDRv_PQ_Set_CmdqCfg(PQ_MAIN_WINDOW,0,0,0);
                stSetBypassCfg.u32StructSize = sizeof(ST_MDRV_VIP_BYPASS_CONFIG);
                stSetBypassCfg.enPQIPType = PQ_IP_VIP_Main;
                stSetBypassCfg.pPointToCfg = MsOS_Memalloc(stSetBypassCfg.u32StructSize, GFP_KERNEL);
                if(!stSetBypassCfg.pPointToCfg)
                {
                    SCL_ERR("[MDRVIP]%s(%d) Init stSetBypassCfg.pPointToCfg Fail\n", __FUNCTION__, __LINE__);
                    return 0;
                }
                MsOS_Memset(stSetBypassCfg.pPointToCfg, (bBypass) ? 0x1 :0,stSetBypassCfg.u32StructSize);
                _MDrv_VIP_SetPQParameter(&stSetBypassCfg);
                MsOS_MemFree(stSetBypassCfg.pPointToCfg);
            break;
        default:
            return 0;

    }
    return 1;
}
unsigned long _MDrv_VIP_FillSettingBuffer(MS_U16 u16PQIPIdx, MS_U32 u32pstViraddr, MS_U32 *u32gpViraddr)
{
    if(_IsNotToCheckPQorCMDQmode())
    {
        MS_U16 u16StructSize;
        MS_U32 u32ViraddrOri;
        MS_U32 u32Viraddr;
        u16StructSize = MDrv_PQ_GetIPRegCount(u16PQIPIdx);
        u32ViraddrOri = u32pstViraddr;
        if(*u32gpViraddr)
        {
            u32Viraddr = *u32gpViraddr;
        }
        else
        {
            *u32gpViraddr = (unsigned long)MsOS_VirMemalloc(u16StructSize);
            if(!*u32gpViraddr)
            {
                SCL_ERR("[MDRVIP]%s(%d) Init *u32gpViraddr Fail\n", __FUNCTION__, __LINE__);
                return 0;
            }
            MsOS_Memset((void *)*u32gpViraddr,0,u16StructSize);
            u32Viraddr = *u32gpViraddr;
        }
        if(u32Viraddr != u32ViraddrOri)
        {
            if(MsOS_copy_from_user((void *)u32Viraddr, (__user void  *)u32ViraddrOri, u16StructSize))
            {
                if((u32ViraddrOri&MIU0Vir_BASE)==MIU0Vir_BASE)
                {
                    MsOS_Memcpy((void *)u32Viraddr, (void *)u32ViraddrOri, u16StructSize);
                }
                else
                {
                    SCL_ERR( "[VIP] copy buffer error Ori:%lx ,%lx\n",u32ViraddrOri,u32Viraddr);
                    return 0;
                }
            }
        }
        return u32Viraddr;
    }
    else
    {
        return u32pstViraddr;
    }
}
//-------------------------------------------------------------------------------------------------
//  Function
//-------------------------------------------------------------------------------------------------
void MDrv_VIP_FillBasicStructSetPQCfg(EN_VIP_MDRV_CONFIG_TYPE enVIPtype,void *pPointToCfg,ST_MDRV_VIP_SETPQ_CONFIG *stSetPQCfg)
{
    stSetPQCfg->bSetConfigFlag = _IsSuspendResetFlag(enVIPtype);
    stSetPQCfg->pPointToCfg = pPointToCfg;
    switch(enVIPtype)
    {
        case EN_VIP_MDRV_MCNR_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(ST_MDRV_VIP_MCNR_CONFIG);
                stSetPQCfg->pGolbalStructAddr = (unsigned char *)&gstSupCfg.stmcnr;
                stSetPQCfg->pfForSet = MDrv_VIP_SetMCNRConfig;
            break;

        case EN_VIP_MDRV_ACK_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(ST_MDRV_VIP_ACK_CONFIG);
                stSetPQCfg->pGolbalStructAddr = (unsigned char *)&gstSupCfg.stack;
                stSetPQCfg->pfForSet = MDrv_VIP_SetACKConfig;
            break;

        case EN_VIP_MDRV_IBC_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(ST_MDRV_VIP_IBC_CONFIG);
                stSetPQCfg->pGolbalStructAddr = (unsigned char *)&gstSupCfg.stibc;
                stSetPQCfg->pfForSet = MDrv_VIP_SetIBCConfig;
            break;

        case EN_VIP_MDRV_IHCICC_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(ST_MDRV_VIP_IHCICC_CONFIG);
                stSetPQCfg->pGolbalStructAddr = (unsigned char *)&gstSupCfg.stihcicc;
                stSetPQCfg->pfForSet = MDrv_VIP_SetIHCICCADPYConfig;
            break;

        case EN_VIP_MDRV_ICC_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(ST_MDRV_VIP_ICC_CONFIG);
                stSetPQCfg->pGolbalStructAddr = (unsigned char *)&gstSupCfg.sticc;
                stSetPQCfg->pfForSet = MDrv_VIP_SetICEConfig;
            break;

        case EN_VIP_MDRV_IHC_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(ST_MDRV_VIP_IHC_CONFIG);
                stSetPQCfg->pGolbalStructAddr = (unsigned char *)&gstSupCfg.stihc;
                stSetPQCfg->pfForSet = MDrv_VIP_SetIHCConfig;
            break;

        case EN_VIP_MDRV_FCC_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(ST_MDRV_VIP_FCC_CONFIG);
                stSetPQCfg->pGolbalStructAddr = (unsigned char *)&gstSupCfg.stfcc;
                stSetPQCfg->pfForSet = MDrv_VIP_SetFCCConfig;
            break;
        case EN_VIP_MDRV_UVC_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(ST_MDRV_VIP_UVC_CONFIG);
                stSetPQCfg->pGolbalStructAddr = (unsigned char *)&gstSupCfg.stuvc;
                stSetPQCfg->pfForSet = MDrv_VIP_SetUVCConfig;
            break;

        case EN_VIP_MDRV_DLC_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(ST_MDRV_VIP_DLC_CONFIG);
                stSetPQCfg->pGolbalStructAddr = (unsigned char *)&gstSupCfg.stdlc;
                stSetPQCfg->pfForSet = MDrv_VIP_SetDLCConfig;
            break;

        case EN_VIP_MDRV_DLC_HISTOGRAM_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(ST_MDRV_VIP_DLC_HISTOGRAM_CONFIG);
                stSetPQCfg->pGolbalStructAddr = (unsigned char *)&gstSupCfg.sthist;
                stSetPQCfg->pfForSet = MDrv_VIP_SetHistogramConfig;
            break;

        case EN_VIP_MDRV_LCE_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(ST_MDRV_VIP_LCE_CONFIG);
                stSetPQCfg->pGolbalStructAddr = (unsigned char *)&gstSupCfg.stlce;
                stSetPQCfg->pfForSet = MDrv_VIP_SetLCEConfig;
            break;

        case EN_VIP_MDRV_PEAKING_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(ST_MDRV_VIP_PEAKING_CONFIG);
                stSetPQCfg->pGolbalStructAddr = (unsigned char *)&gstSupCfg.stpk;
                stSetPQCfg->pfForSet = MDrv_VIP_SetPeakingConfig;
            break;

        case EN_VIP_MDRV_LDC_MD_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(ST_MDRV_VIP_LDC_MD_CONFIG);
                stSetPQCfg->pGolbalStructAddr = (unsigned char *)&gstSupCfg.stldcmd;
                stSetPQCfg->pfForSet = MDrv_VIP_SetLDCmdConfig;
            break;

        case EN_VIP_MDRV_LDC_DMAP_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(ST_MDRV_VIP_LDC_DMAP_CONFIG);
                stSetPQCfg->pGolbalStructAddr = (unsigned char *)&gstSupCfg.stldcdmap;
                stSetPQCfg->pfForSet = MDrv_VIP_SetLDCDmapConfig;
            break;

        case EN_VIP_MDRV_LDC_SRAM_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(ST_MDRV_VIP_LDC_SRAM_CONFIG);
                stSetPQCfg->pGolbalStructAddr = (unsigned char *)&gstSupCfg.stldcsram;
                stSetPQCfg->pfForSet = MDrv_VIP_SetLDCSRAMConfig;
            break;

        case EN_VIP_MDRV_NLM_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(ST_MDRV_VIP_NLM_CONFIG);
                stSetPQCfg->pGolbalStructAddr = (unsigned char *)&gstSupCfg.stnlm;
                stSetPQCfg->pfForSet = MDrv_VIP_SetNLMConfig;
            break;
        case EN_VIP_MDRV_LDC_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(ST_MDRV_VIP_LDC_CONFIG);
                stSetPQCfg->pGolbalStructAddr = (unsigned char *)&gstSupCfg.stldc;
                stSetPQCfg->pfForSet = MDrv_VIP_SetLDCConfig;
            break;
        case EN_VIP_MDRV_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(ST_MDRV_VIP_CONFIG);
                stSetPQCfg->pGolbalStructAddr = (unsigned char *)&gstSupCfg.stvip;
                stSetPQCfg->pfForSet = MDrv_SetVIPOtherConfig;
            break;
        default:
                stSetPQCfg->u32StructSize = 0;
                stSetPQCfg->pGolbalStructAddr = NULL;
                stSetPQCfg->bSetConfigFlag = 0;
                stSetPQCfg->pfForSet = NULL;
            break;

    }
}
void _MDrv_VIP_InitSupCfg(void)
{
    MsOS_Memset(&gstSupCfg, 0, sizeof(ST_MDRV_VIP_SUSPEND_CONFIG));
}
void MDrv_VIP_FreeMemory(void)
{
    EN_VIP_MDRV_AIP_TYPE enVIPtype;
    if(gstSupCfg.stmcnr.u32Viraddr)
    {
        MsOS_VirMemFree((void *)gstSupCfg.stmcnr.u32Viraddr);
        gstSupCfg.stmcnr.u32Viraddr = NULL;
    }
    for(enVIPtype = EN_VIP_MDRV_AIP_YEE;enVIPtype<EN_VIP_MDRV_AIP_NUM;enVIPtype++)
    {
        if(gstSupCfg.staip[enVIPtype].u32Viraddr)
        {
            MsOS_VirMemFree((void *)gstSupCfg.staip[enVIPtype].u32Viraddr);
            gstSupCfg.staip[enVIPtype].u32Viraddr = NULL;
        }
    }
}

void MDrv_VIP_Resume(ST_MDRV_VIP_INIT_CONFIG *pCfg)
{
    ST_VIP_OPEN_CONFIG stCMDQIniCfg;
    MsOS_Memset(&stCMDQIniCfg,0,sizeof(ST_VIP_OPEN_CONFIG));
    stCMDQIniCfg.u32RiuBase     = pCfg->u32RiuBase;
    stCMDQIniCfg.u32CMDQ_Phy[0]    = pCfg->CMDQCfg.u32CMDQ_Phy;
    stCMDQIniCfg.u32CMDQ_Size[0]   = pCfg->CMDQCfg.u32CMDQ_Size;
    stCMDQIniCfg.u32CMDQ_Vir[0]    = pCfg->CMDQCfg.u32CMDQ_Vir;
    Drv_VIP_HWInit();
    MDrv_VIP_VtrackInit();
    Drv_CMDQ_Init(&stCMDQIniCfg);
    MDRV_VIP_MUTEX_LOCK();
    gu32OpenBypass = 0;
    gu32AIPOpenBypass = 0;
    gbVIPCMDQSet = 0;
    gbAIPCMDQSet = 0;
    gbAlreadyOpenGamma = 0;
    MDRV_VIP_MUTEX_UNLOCK();
    Drv_VIP_SRAM_Dump();
}

unsigned char MDrv_VIP_Init(ST_MDRV_VIP_INIT_CONFIG *pCfg)
{
    ST_VIP_INIT_CONFIG stIniCfg;
    ST_VIP_OPEN_CONFIG stCMDQIniCfg;
    MS_PQ_Init_Info    stPQInitInfo;
    unsigned char      ret = FALSE;
    char word[] = {"_MVIP_Mutex"};
    _MVIP_Mutex = MsOS_CreateMutex(E_MSOS_FIFO, word, MSOS_PROCESS_SHARED);
    if (_MVIP_Mutex == -1)
    {
        SCL_ERR("[MDRVVIP]%s create mutex fail\n", __FUNCTION__);
        return FALSE;
    }
    b_MVIP_TASKEn = 1;
    _MVIP_TASK  = MsOS_CreateTask((TaskEntry)VIP_DazaIST,(MS_U32)NULL,TRUE,(const char*)"VIPDazaTask");
    if (_MVIP_TASK == -1)
    {
        SCL_ERR("[MDRVVIP]%s create task fail\n", __FUNCTION__);
        return FALSE;
    }
    MsOS_Memset(&stCMDQIniCfg,0,sizeof(ST_VIP_OPEN_CONFIG));
    MsOS_Memset(&stIniCfg,0,sizeof(ST_VIP_INIT_CONFIG));
    stIniCfg.u32RiuBase         = pCfg->u32RiuBase;
    stCMDQIniCfg.u32RiuBase     = pCfg->u32RiuBase;
    stCMDQIniCfg.u32CMDQ_Phy[0]    = pCfg->CMDQCfg.u32CMDQ_Phy;
    stCMDQIniCfg.u32CMDQ_Size[0]   = pCfg->CMDQCfg.u32CMDQ_Size;
    stCMDQIniCfg.u32CMDQ_Vir[0]    = pCfg->CMDQCfg.u32CMDQ_Vir;
    if(Drv_VIP_Init(&stIniCfg) == 0)
    {
        SCL_ERR( "[MDRVVIP]%s, Fail\n", __FUNCTION__);
        return FALSE;
    }
    Drv_VIP_HWInit();
    MDrv_VIP_VtrackInit();
    MDrv_PQ_init_RIU(pCfg->u32RiuBase);
    Drv_CMDQ_Init(&stCMDQIniCfg);
    MsOS_Memset(&stPQInitInfo, 0, sizeof(MS_PQ_Init_Info));
    MDRV_VIP_MUTEX_LOCK();
    gu32OpenBypass = 0;
    gu32AIPOpenBypass = 0;
    gbVIPCMDQSet = 0;
    gbAIPCMDQSet = 0;
    gbAlreadyOpenGamma = 0;
    MDRV_VIP_MUTEX_UNLOCK();
    // Init PQ
    stPQInitInfo.u16PnlWidth    = 1920;
    stPQInitInfo.u8PQBinCnt     = 0;
    stPQInitInfo.u8PQTextBinCnt = 0;
    if(MDrv_PQ_Init(&stPQInitInfo))
    {
        MDrv_PQ_DesideSrcType(PQ_MAIN_WINDOW, PQ_INPUT_SOURCE_ISP);
        MDrv_PQ_LoadSettings(PQ_MAIN_WINDOW);
        ret = TRUE;
    }
    else
    {
        ret = FALSE;
    }
    _MDrv_VIP_InitSupCfg();
    return ret;
}
void MDrv_VIP_Delete(unsigned char bCloseISR)
{
    b_MVIP_TASKEn = 0;
    MsOS_DeleteTask(_MVIP_TASK);
    _MVIP_TASK = -1;
    if (_MVIP_Mutex != -1)
    {
        MsOS_DeleteMutex(_MVIP_Mutex);
        _MVIP_Mutex = -1;
    }
    if(bCloseISR)
    {
        Drv_SCLIRQ_Exit();
    }
    Drv_VIP_Exit();
    Drv_CMDQ_Delete(EN_CMDQ_TYPE_IP0);
}
void MDrv_VIP_Supend(void)
{
    Drv_CMDQ_Delete(EN_CMDQ_TYPE_IP0);
}
void MDrv_VIP_Release(void)
{
    unsigned long u32Events;
    Drv_CMDQ_release(EN_CMDQ_TYPE_IP0);
    MDRV_VIP_MUTEX_LOCK();
    gbVIPCMDQSet = 0;
    gbAIPCMDQSet = 0;
    gbAlreadyOpenGamma = 0;
    MDRV_VIP_MUTEX_UNLOCK();
    if(!MDrv_VIP_GetIsBlankingRegion())
    {
        MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 200); // get status: FRM END
    }
    Drv_VIP_SetIPMConpress(0);
    MDrv_PQ_LoadSettings(PQ_MAIN_WINDOW);
}
unsigned char MDrv_VIP_GetCMDQHWDone(void)
{
    return Drv_VIP_GetCMDQHWDone();
}

void * MDrv_VIP_GetWaitQueueHead(void)
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
void MDrv_VIP_SetAllVIPOneshot(ST_MDRV_VIP_SUSPEND_CONFIG *stvipCfg)
{
    EN_VIP_MDRV_CONFIG_TYPE enVIPtype;
    EN_VIP_MDRV_AIP_TYPE enAIPtype;
    ST_MDRV_VIP_SETPQ_CONFIG stSetPQCfg;
    unsigned long u32StructSize = sizeof(unsigned long);
    MsOS_Memset(&stSetPQCfg,0,sizeof(ST_MDRV_VIP_SETPQ_CONFIG));
    gstSupCfg.bresetflag = stvipCfg->bresetflag;
    gstSupCfg.bresetflag &= 0x2FFFF;
    gstSupCfg.bAIPreflag = stvipCfg->bAIPreflag;
    for(enVIPtype =EN_VIP_MDRV_ACK_CONFIG;enVIPtype<=EN_VIP_MDRV_MCNR_CONFIG;(enVIPtype*=2))
    {
        MDrv_VIP_FillBasicStructSetPQCfg(enVIPtype,(void *)((unsigned long)stvipCfg+u32StructSize),&stSetPQCfg);
        if(_IsSuspendResetFlag(enVIPtype))
        {
            SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPSUP,
                "enVIPtype :%d pGolbalStructAddr: %lx,pPointToCfg: %lx,size:%lx\n",enVIPtype,
                (unsigned long)stSetPQCfg.pGolbalStructAddr,(unsigned long)stSetPQCfg.pPointToCfg,stSetPQCfg.u32StructSize);
            MsOS_Memcpy(stSetPQCfg.pGolbalStructAddr,stSetPQCfg.pPointToCfg,stSetPQCfg.u32StructSize);
        }
        u32StructSize += stSetPQCfg.u32StructSize;
    }
    for(enAIPtype =EN_VIP_MDRV_AIP_YEE;enAIPtype<EN_VIP_MDRV_AIP_NUM;(enAIPtype++))
    {
        if(_IsSuspendAIPResetFlag(enAIPtype))
        {
            _MDrv_VIP_FillSettingBuffer
                (_GetAIPOffset(enAIPtype),stvipCfg->staip[enAIPtype].u32Viraddr,&gstSupCfg.staip[enAIPtype].u32Viraddr);
            gstSupCfg.staip[enAIPtype].stFCfg.bEn = stvipCfg->staip[enAIPtype].stFCfg.bEn;
            gstSupCfg.staip[enAIPtype].stFCfg.u8framecnt = stvipCfg->staip[enAIPtype].stFCfg.u8framecnt;
            gstSupCfg.staip[enAIPtype].u16AIPType = stvipCfg->staip[enAIPtype].u16AIPType;
        }
    }
    _MDrv_VIP_For_SetEachIP();
}
void MDrv_VIP_ResetEachIP(void)
{
    _MDrv_VIP_For_SetEachIP();
    _MDrv_VIP_ResumeDumpSRAM();
    if(gbVIPCheckCMDQorPQ >= EN_VIP_CMDQ_CHECK_ALREADY_SETINNG)
    {
        Drv_CMDQ_BeTrigger(EN_CMDQ_TYPE_IP0,1);
    }
}

unsigned char MDrv_VIP_Sys_Init(ST_MDRV_VIP_INIT_CONFIG *pCfg)
{
    SCL_DBG(SCL_DBG_LV_VIP(), "[MDRVVIP]%s %d\n", __FUNCTION__,__LINE__);
    Drv_CMDQ_Enable(1,EN_CMDQ_TYPE_IP0);
    Drv_VIP_Open();
    return TRUE;
}
void MDrv_VIP_SetMCNRConpressForDebug(unsigned char bEn)
{
    if(gstSupCfg.stmcnr.bEnMCNR)
    {
        Drv_VIP_SetIPMConpress(bEn);
    }
}
void _MDrv_VIP_CopyAIPConfigToGlobal(EN_VIP_MDRV_AIP_TYPE enVIPtype, void *pCfg, unsigned long u32StructSize)
{
    if(_IsNotToCheckPQorCMDQmode())
    {
        MsOS_Memcpy(&gstSupCfg.staip[enVIPtype], pCfg,u32StructSize);
        gstSupCfg.staip[enVIPtype].stFCfg.bEn = 0 ;
        gstSupCfg.staip[enVIPtype].stFCfg.u8framecnt = 0;
        gstSupCfg.staip[enVIPtype].u16AIPType= enVIPtype;
        _Set_SuspendAIPResetFlag(enVIPtype);
    }
}
unsigned char MDrv_VIP_SetMCNRConfig(void *pCfg)
{
    ST_MDRV_VIP_MCNR_CONFIG *pstCfg = pCfg;
    static MS_BOOL bbypass = 0;
    if(_IsFrameBufferAllocatedReady())
    {
        if(_IsNotToCheckPQorCMDQmode())
        {
            MDRV_VIP_MUTEX_LOCK();
        }

        if(_IsNotOpenVIPBypass(EN_VIP_MDRV_MCNR_CONFIG))
        {
            bbypass = 0;
            pstCfg->u32Viraddr =
                _MDrv_VIP_FillSettingBuffer(PQ_IP_MCNR_Main,pstCfg->u32Viraddr,&gstSupCfg.stmcnr.u32Viraddr);
            if(!pstCfg->u32Viraddr)
            {
                return 0;
            }
            //_MDrv_VIP_SetCMDQStatus(1,pstCfg->stFCfg.bEn,pstCfg->stFCfg.u8framecnt);
            if(((MsOS_GetEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID())&(E_SCLIRQ_EVENT_ISPFRMEND|E_SCLIRQ_EVENT_FRMENDSYNC))
                !=(E_SCLIRQ_EVENT_ISPFRMEND|E_SCLIRQ_EVENT_FRMENDSYNC))&&(_IsNotToCheckPQorCMDQmode()))
            {
                _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_MDRV_MCNR_CONFIG,(void *)pstCfg);
                _MDrv_VIP_SetDAZATaskWork(E_VIP_DAZA_BMCNR);
            }
            else
            {
                MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,pstCfg->stFCfg.bEn);
                MDRv_PQ_Set_CmdqCfg(PQ_MAIN_WINDOW,pstCfg->stFCfg.bEn,pstCfg->stFCfg.u8framecnt,1);
                _MDrv_VIP_SetMCNR(pstCfg);
                MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pstCfg->stFCfg.bEn);
                _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_MDRV_MCNR_CONFIG,(void *)pstCfg);
            }
        }
        else if(bbypass==0)
        {
            MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,pstCfg->stFCfg.bEn);
            MDRv_PQ_Set_CmdqCfg(PQ_MAIN_WINDOW,pstCfg->stFCfg.bEn,pstCfg->stFCfg.u8framecnt,1);
            _MDrv_VIP_SetMCNR(pstCfg);
            MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pstCfg->stFCfg.bEn);
            bbypass = 1;
        }
        else
        {
            _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_MDRV_MCNR_CONFIG,(void *)pstCfg);
        }
        if(_IsNotToCheckPQorCMDQmode())
        {
            MDRV_VIP_MUTEX_UNLOCK();
        }
        SCL_DBG(SCL_DBG_LV_VIP(),
            "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&(gstSupCfg.stmcnr),gstSupCfg.bresetflag);
        return TRUE;
    }
    else
    {
        SCL_ERR( "[MDRVVIP]%s,MCNR buffer not alloc \n", __FUNCTION__);
        return FALSE;
    }
    return 1;
}


unsigned char MDrv_VIP_SetPeakingConfig(void *pvCfg)
{
    ST_MDRV_VIP_PEAKING_CONFIG *pCfg = pvCfg;
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
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_LOCK();
    }
    if(_MDrv_VIP_For_SetEachPQTypeByIP((MS_U8)PQ_IP_VIP_HLPF_Main,(MS_U8)PQ_IP_VIP_Peaking_gain_ad_Y_Main,&pCfg->stFCfg,p8PQType))
    {
        _Set_VIPCMDQSetFlag(EN_VIP_MDRV_PEAKING_CONFIG);
    }

    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_MDRV_PEAKING_CONFIG,(void *)pCfg);
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_UNLOCK();
    }
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stpk,gstSupCfg.bresetflag);
    return TRUE;
}

unsigned char MDrv_VIP_SetHistogramConfig(void *pvCfg)
{
    ST_VIP_DLC_HISTOGRAM_CONFIG stDLCCfg;
    MS_U16 i;
    ST_MDRV_VIP_DLC_HISTOGRAM_CONFIG *pCfg = pvCfg;
    MsOS_Memset(&stDLCCfg,0,sizeof(ST_VIP_DLC_HISTOGRAM_CONFIG));
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
    stDLCCfg.u32StatBase[0]     = _Phys2Miu(pCfg->u32StatBase[0]);
    stDLCCfg.u32StatBase[1]     = _Phys2Miu(pCfg->u32StatBase[1]);
    stDLCCfg.stFCfg.bEn         = pCfg->stFCfg.bEn;
    stDLCCfg.stFCfg.u8framecnt  = pCfg->stFCfg.u8framecnt;

    for(i=0;i<VIP_DLC_HISTOGRAM_SECTION_NUM;i++)
    {
        stDLCCfg.u8Histogram_Range[i] = pCfg->u8Histogram_Range[i];
    }
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_LOCK();
    }
    MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,stDLCCfg.stFCfg.bEn);

    if(Drv_VIP_SetDLCHistogramConfig(&stDLCCfg) == 0)
    {
        SCL_DBG(SCL_DBG_LV_VIP(), "[MDRVVIP]%s, Fail\n", __FUNCTION__);
        MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,stDLCCfg.stFCfg.bEn);
        MDRV_VIP_MUTEX_UNLOCK();
        return FALSE;
    }
    MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,stDLCCfg.stFCfg.bEn);
    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_MDRV_DLC_HISTOGRAM_CONFIG,(void *)pCfg);
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_UNLOCK();
    }
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.sthist,gstSupCfg.bresetflag);
    return TRUE;
}

unsigned char MDrv_VIP_GetDLCHistogramReport(void *pvCfg)
{
    ST_VIP_DLC_HISTOGRAM_REPORT stDLCCfg;
    MS_U16 i;
    ST_MDRV_VIP_DLC_HISTOGRAM_REPORT *pCfg = pvCfg;
    MsOS_Memset(&stDLCCfg,0,sizeof(ST_VIP_DLC_HISTOGRAM_REPORT));
    Drv_VIP_GetDLCHistogramConfig(&stDLCCfg);
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
    ST_MDRV_VIP_DLC_CONFIG *pCfg = pvCfg;
    MS_U8 *p8PQType[(PQ_IP_VIP_YC_gain_offset_Main-PQ_IP_VIP_DLC_His_range_Main+1)];//add PQ_IP_VIP_YC_gain_offset_Main
    p8PQType[0] = (MS_U8*)&(pCfg->sthist);
    p8PQType[1] = (MS_U8*)&(pCfg->stEn);
    p8PQType[2] = (MS_U8*)&(pCfg->stDither);
    p8PQType[3] = (MS_U8*)&(pCfg->stHistH);
    p8PQType[4] = (MS_U8*)&(pCfg->stHistV);
    p8PQType[5] = (MS_U8*)&(pCfg->stPC);
    p8PQType[6] = (MS_U8*)&(pCfg->stGainOffset);
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_LOCK();
    }

    if(_MDrv_VIP_For_SetEachPQTypeByIP((MS_U8)PQ_IP_VIP_DLC_His_range_Main,(MS_U8)PQ_IP_VIP_YC_gain_offset_Main,&pCfg->stFCfg,p8PQType))
    {
        _Set_VIPCMDQSetFlag(EN_VIP_MDRV_DLC_CONFIG);
    }

    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_MDRV_DLC_CONFIG,(void *)pCfg);
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_UNLOCK();
    }
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stdlc,gstSupCfg.bresetflag);
    return TRUE;
}

unsigned char MDrv_VIP_SetLCEConfig(void *pvCfg)
{
    ST_MDRV_VIP_LCE_CONFIG *pCfg = pvCfg;
    MS_U8 *p8PQType[(PQ_IP_VIP_LCE_curve_Main-PQ_IP_VIP_LCE_Main+1)];
    p8PQType[0] = (MS_U8*)&(pCfg->stOnOff);
    p8PQType[1] = (MS_U8*)&(pCfg->stDITHER);
    p8PQType[2] = (MS_U8*)&(pCfg->stSet);
    p8PQType[3] = (MS_U8*)&(pCfg->stCurve);
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_LOCK();
    }
    if(_MDrv_VIP_For_SetEachPQTypeByIP((MS_U8)PQ_IP_VIP_LCE_Main,(MS_U8)PQ_IP_VIP_LCE_curve_Main,&pCfg->stFCfg,p8PQType))
    {
        _Set_VIPCMDQSetFlag(EN_VIP_MDRV_LCE_CONFIG);
    }

    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_MDRV_LCE_CONFIG,(void *)pCfg);
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_UNLOCK();
    }
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stlce,gstSupCfg.bresetflag);
    return TRUE;
}

unsigned char MDrv_VIP_SetUVCConfig(void *pvCfg)
{
    MS_U8 *pUVC = NULL;
    MS_U8 bRet = 0;
    ST_MDRV_VIP_UVC_CONFIG *pCfg = pvCfg;
    pUVC = (MS_U8*)&(pCfg->stUVC);
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_LOCK();
    }
    if(_MDrv_VIP_SetCMDQStatus(1,pCfg->stFCfg.bEn,pCfg->stFCfg.u8framecnt))
    {
        MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,1);
        _Set_VIPCMDQSetFlag(EN_VIP_MDRV_UVC_CONFIG);
        bRet = 1;
    }
    _MDrv_VIP_SetPQByType((MS_U8)PQ_IP_VIP_UVC_Main,pUVC);
    MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,bRet);
    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_MDRV_UVC_CONFIG,(void *)pCfg);
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_UNLOCK();
    }
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stuvc,gstSupCfg.bresetflag);
    return TRUE;
}


unsigned char MDrv_VIP_SetIHCConfig(void *pvCfg)
{
    ST_MDRV_VIP_IHC_CONFIG *pCfg = pvCfg;
	MS_U8 bRet = 0;
    MS_U8 *p8PQType[(PQ_IP_VIP_IHC_dither_Main-PQ_IP_VIP_IHC_Main+2)];
    p8PQType[0] = (MS_U8*)&(pCfg->stOnOff);
    p8PQType[1] = (MS_U8*)&(pCfg->stYmd);
    p8PQType[2] = (MS_U8*)&(pCfg->stDither);
    p8PQType[3] = (MS_U8*)&(pCfg->stset);
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_LOCK();
    }
    bRet = _MDrv_VIP_SetCMDQStatus(0,pCfg->stFCfg.bEn,pCfg->stFCfg.u8framecnt);
    MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,bRet);
    _MDrv_VIP_SetPQByType((MS_U8)PQ_IP_VIP_IHC_SETTING_Main,p8PQType[3]);
    MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,bRet);
    if(_MDrv_VIP_For_SetEachPQTypeByIP((MS_U8)PQ_IP_VIP_IHC_Main,(MS_U8)PQ_IP_VIP_IHC_dither_Main,&pCfg->stFCfg,p8PQType))
    {
        _Set_VIPCMDQSetFlag(EN_VIP_MDRV_IHC_CONFIG);
    }

    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_MDRV_IHC_CONFIG,(void *)pCfg);
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_UNLOCK();
    }
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stihc,gstSupCfg.bresetflag);
    return TRUE;
}

unsigned char MDrv_VIP_SetICEConfig(void *pvCfg)
{
    ST_MDRV_VIP_ICC_CONFIG *pCfg = pvCfg;
	MS_U8 bRet = 0;
    MS_U8 *p8PQType[(PQ_IP_VIP_ICC_SETTING_Main-PQ_IP_VIP_ICC_dither_Main+2)];
    p8PQType[0] = (MS_U8*)&(pCfg->stEn);
    p8PQType[1] = (MS_U8*)&(pCfg->stYmd);
    p8PQType[2] = (MS_U8*)&(pCfg->stDither);
    p8PQType[3] = (MS_U8*)&(pCfg->stSet);
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_LOCK();
    }
    bRet = _MDrv_VIP_SetCMDQStatus(0,pCfg->stFCfg.bEn,pCfg->stFCfg.u8framecnt);
    MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,bRet);
    _MDrv_VIP_SetPQByType((MS_U8)PQ_IP_VIP_ICC_SETTING_Main,p8PQType[3]);
    MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,bRet);
    if(_MDrv_VIP_For_SetEachPQTypeByIP((MS_U8)PQ_IP_VIP_ICC_Main,(MS_U8)PQ_IP_VIP_ICC_dither_Main,&pCfg->stFCfg,p8PQType))
    {
        _Set_VIPCMDQSetFlag(EN_VIP_MDRV_ICC_CONFIG);
    }

    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_MDRV_ICC_CONFIG,(void *)pCfg);
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_UNLOCK();
    }
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.sticc,gstSupCfg.bresetflag);
    return TRUE;
}


unsigned char MDrv_VIP_SetIHCICCADPYConfig(void *pvCfg)
{
    ST_MDRV_VIP_IHCICC_CONFIG *pCfg = pvCfg;
    MS_U8 *p8PQType[(PQ_IP_VIP_Ymode_Yvalue_SETTING_Main-PQ_IP_VIP_Ymode_Yvalue_ALL_Main+1)];
    p8PQType[0] = (MS_U8*)&(pCfg->stYmdall);
    p8PQType[1] = (MS_U8*)&(pCfg->stYmdset);
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_LOCK();
    }

    if(_MDrv_VIP_For_SetEachPQTypeByIP((MS_U8)PQ_IP_VIP_Ymode_Yvalue_ALL_Main,(MS_U8)PQ_IP_VIP_Ymode_Yvalue_SETTING_Main,&pCfg->stFCfg,p8PQType))
    {
        _Set_VIPCMDQSetFlag(EN_VIP_MDRV_IHCICC_CONFIG);
    }

    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_MDRV_IHCICC_CONFIG,(void *)pCfg);
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_UNLOCK();
    }
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stihcicc,gstSupCfg.bresetflag);
    return TRUE;
}

unsigned char MDrv_VIP_SetIBCConfig(void *pvCfg)
{
    ST_MDRV_VIP_IBC_CONFIG *pCfg = pvCfg;
    MS_U8 *p8PQType[(PQ_IP_VIP_IBC_SETTING_Main-PQ_IP_VIP_IBC_Main+1)];
    p8PQType[0] = (MS_U8*)&(pCfg->stEn);
    p8PQType[1] = (MS_U8*)&(pCfg->stDither);
    p8PQType[2] = (MS_U8*)&(pCfg->stSet);
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_LOCK();
    }
    if(_MDrv_VIP_For_SetEachPQTypeByIP((MS_U8)PQ_IP_VIP_IBC_Main,(MS_U8)PQ_IP_VIP_IBC_SETTING_Main,&pCfg->stFCfg,p8PQType))
    {
        _Set_VIPCMDQSetFlag(EN_VIP_MDRV_IBC_CONFIG);
    }

    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_MDRV_IBC_CONFIG,(void *)pCfg);
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_UNLOCK();
    }
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stibc,gstSupCfg.bresetflag);
    return TRUE;
}

unsigned char MDrv_VIP_SetFCCConfig(void *pvCfg)
{
    ST_MDRV_VIP_FCC_CONFIG *pCfg = pvCfg;
	MS_U8 bRet = 0;
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
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_LOCK();
    }
    bRet = _MDrv_VIP_SetCMDQStatus(0,pCfg->stFCfg.bEn,pCfg->stFCfg.u8framecnt);
    MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,bRet);
    _MDrv_VIP_SetPQByType((MS_U8)PQ_IP_VIP_FCC_full_range_Main,p8PQType[9]);
    MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,bRet);
    if(_MDrv_VIP_For_SetEachPQTypeByIP((MS_U8)PQ_IP_VIP_FCC_T1_Main,(MS_U8)PQ_IP_VIP_FCC_T9_Main,&pCfg->stFCfg,p8PQType))
    {
        _Set_VIPCMDQSetFlag(EN_VIP_MDRV_FCC_CONFIG);
    }

    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_MDRV_FCC_CONFIG,(void *)pCfg);
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_UNLOCK();
    }
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stfcc,gstSupCfg.bresetflag);
    return TRUE;
}


unsigned char MDrv_VIP_SetACKConfig(void *pvCfg)
{
    ST_MDRV_VIP_ACK_CONFIG *pCfg = pvCfg;
    MS_U8 *p8PQType[(PQ_IP_VIP_YCbCr_Clip_Main-PQ_IP_VIP_ACK_Main+1)];
    p8PQType[0] = (MS_U8*)&(pCfg->stACK);
    p8PQType[1] = (MS_U8*)&(pCfg->stclip);
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_LOCK();
    }
    if(_MDrv_VIP_For_SetEachPQTypeByIP((MS_U8)PQ_IP_VIP_ACK_Main,(MS_U8)PQ_IP_VIP_YCbCr_Clip_Main,&pCfg->stFCfg,p8PQType))
    {
        _Set_VIPCMDQSetFlag(EN_VIP_MDRV_ACK_CONFIG);
    }

    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_MDRV_ACK_CONFIG,(void *)pCfg);
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_UNLOCK();
    }
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stack,gstSupCfg.bresetflag);
    return TRUE;
}

unsigned char MDrv_VIP_SetNLMConfig(void *pvCfg)
{
    MS_U8 *stNLM = NULL;
    ST_MDRV_VIP_NLM_CONFIG *pCfg = pvCfg;
    stNLM  = (MS_U8*)&(pCfg->stNLM);
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_LOCK();
    }

    if(_IsNotOpenVIPBypass(EN_VIP_MDRV_NLM_CONFIG))
    {
        //_MDrv_VIP_SetCMDQStatus(1,pCfg->stFCfg.bEn,pCfg->stFCfg.u8framecnt);
        if(((MsOS_GetEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID())&(E_SCLIRQ_EVENT_ISPFRMEND|E_SCLIRQ_EVENT_FRMENDSYNC))
            !=(E_SCLIRQ_EVENT_ISPFRMEND|E_SCLIRQ_EVENT_FRMENDSYNC))&&(_IsNotToCheckPQorCMDQmode()))
        {
            _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_MDRV_NLM_CONFIG,(void *)pCfg);
            _MDrv_VIP_SetDAZATaskWork(E_VIP_DAZA_BNLM);
        }
        else
        {
            MDRv_PQ_Set_CmdqCfg(PQ_MAIN_WINDOW,pCfg->stFCfg.bEn,pCfg->stFCfg.u8framecnt,1);
            MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
            _MDrv_VIP_SetPQByType((MS_U8)PQ_IP_NLM_Main,stNLM);
            MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,pCfg->stFCfg.bEn);
            _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_MDRV_NLM_CONFIG,(void *)pCfg);
        }
    }
    else
    {
        _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_MDRV_NLM_CONFIG,(void *)pCfg);
    }
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_UNLOCK();
    }
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stnlm,gstSupCfg.bresetflag);
    return TRUE;
}

unsigned char MDrv_VIP_SetNLMSRAMConfig(ST_MDRV_VIP_NLM_SRAM_CONFIG *stSRAM)
{
    ST_VIP_NLM_SRAM_CONFIG stCfg;
    MsOS_Memset(&stCfg,0,sizeof(ST_VIP_NLM_SRAM_CONFIG));
    stCfg.u32baseadr    = _Phys2Miu(stSRAM->u32Baseadr);
    stCfg.bCLientEn     = stSRAM->bEn;
    stCfg.u32viradr     = stSRAM->u32viradr;
    stCfg.btrigContinue = 0;                    //single
    stCfg.u16depth      = VIP_NLM_ENTRY_NUM;    // entry
    stCfg.u16reqlen     = VIP_NLM_ENTRY_NUM;
    stCfg.u16iniaddr    = 0;
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s, flag:%hhx ,addr:%lx,addr:%lx \n", __FUNCTION__,stSRAM->bEn,stSRAM->u32viradr,stSRAM->u32Baseadr);
    Drv_VIP_SetNLMSRAMConfig(&stCfg);
    return TRUE;
}

unsigned char MDrv_SetVIPOtherConfig(void *pvCfg)
{
    ST_MDRV_VIP_CONFIG *pCfg = pvCfg;
	MS_U8 bRet = 0;
    MS_U8 *p8PQType[2];
    p8PQType[0] = (MS_U8*)&(pCfg->st422_444);
    p8PQType[1] = (MS_U8*)&(pCfg->stBypass);
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_LOCK();
    }
    bRet = _MDrv_VIP_SetCMDQStatus(0,pCfg->stFCfg.bEn,pCfg->stFCfg.u8framecnt);
    if(_MDrv_VIP_For_SetEachPQTypeByIP((MS_U8)PQ_IP_422to444_Main,(MS_U8)PQ_IP_VIP_Main,&pCfg->stFCfg,p8PQType))
    {
        _Set_VIPCMDQSetFlag(EN_VIP_MDRV_CONFIG);
    }

    _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_MDRV_CONFIG,(void *)pCfg);
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_UNLOCK();
    }
    return TRUE;
}
void MDrv_VIP_CheckRegister(void)
{
    SCL_ERR( "[MDRVVIP]%s  \n", __FUNCTION__);
    MDRv_PQ_Check_Type(PQ_MAIN_WINDOW,PQ_CHECK_REG);
    MDrv_VIP_ResetEachIP();
    MDRv_PQ_Check_Type(PQ_MAIN_WINDOW,PQ_CHECK_OFF);
}

void MDrv_VIP_CheckConsist(void)
{
    SCL_ERR( "[MDRVVIP]%s  \n", __FUNCTION__);
    MDRv_PQ_Check_Type(PQ_MAIN_WINDOW,PQ_CHECK_SIZE);
    MDrv_VIP_ResetEachIP();
    MDRv_PQ_Check_Type(PQ_MAIN_WINDOW,PQ_CHECK_OFF);
}
void MDrv_VIP_PrepareStructToCheckRegister(EN_VIP_CMDQ_CHECK_TYPE enCheckType)
{
    EN_VIP_MDRV_AIP_TYPE enIdx;
    SCL_ERR( "[MDRVVIP]%s CHECK_TYPE:%hhd \n", __FUNCTION__,enCheckType);
    MDrv_VIP_SetCheckMode(enCheckType);
    if(_IsAutoSetting())
    {
        _Set_SuspendResetFlag(0x2FFFF);
        for(enIdx = EN_VIP_MDRV_AIP_YEE;enIdx<EN_VIP_MDRV_AIP_NUM;enIdx++)
        {
            _Set_SuspendAIPResetFlag(enIdx);
        }
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
void MDrv_VIP_SetCheckMode(EN_VIP_CMDQ_CHECK_TYPE enCheckType)
{
    gbVIPCheckCMDQorPQ = enCheckType;
}
unsigned char MDrv_VIP_SetAIPBypassConfig(EN_VIP_MDRV_AIP_TYPE enAIPtype)
{
    EN_VIP_MDRV_AIP_TYPE entype;
    if(enAIPtype != EN_VIP_MDRV_AIP_NUM)
    {
        _MDrv_VIP_ModifyAIPAlreadySetting(enAIPtype,EN_VIP_RESET_ZERO);
        _Set_AIPBypassFlag(enAIPtype);
    }
    else
    {
        for(entype=EN_VIP_MDRV_AIP_YEE;entype<EN_VIP_MDRV_AIP_NUM;entype++)
        {
            if(_Is_AIPBypassFlag(entype))
            {
                _MDrv_VIP_ModifyAIPAlreadySetting(entype,EN_VIP_RESET_ALREADY);
            }
        }
        MDRV_VIP_MUTEX_LOCK();
        gu32AIPOpenBypass = 0;
        MDRV_VIP_MUTEX_UNLOCK();
    }
    return TRUE;
}
unsigned char MDrv_VIP_SetVIPBypassConfig(EN_VIP_MDRV_CONFIG_TYPE enVIPtype)
{
    if(enVIPtype)
    {
        if(enVIPtype&EN_VIP_MDRV_CONFIG)
        {
            _MDrv_VIP_SetBypassIP(enVIPtype&EN_VIP_MDRV_CONFIG, EN_VIP_MDRV_CONFIG);
        }
        if(enVIPtype&EN_VIP_MDRV_MCNR_CONFIG)
        {
            _MDrv_VIP_SetBypassIP(enVIPtype&EN_VIP_MDRV_MCNR_CONFIG, EN_VIP_MDRV_MCNR_CONFIG);
        }
        if(enVIPtype&EN_VIP_MDRV_NLM_CONFIG)
        {
            _MDrv_VIP_SetBypassIP(enVIPtype&EN_VIP_MDRV_NLM_CONFIG, EN_VIP_MDRV_NLM_CONFIG);
        }
        if(enVIPtype&EN_VIP_MDRV_LDC_CONFIG)
        {
            _MDrv_VIP_SetBypassIP(enVIPtype&EN_VIP_MDRV_LDC_CONFIG, EN_VIP_MDRV_LDC_CONFIG);
        }
        MDRV_VIP_MUTEX_LOCK();
        gu32OpenBypass |= enVIPtype;
        MDRV_VIP_MUTEX_UNLOCK();
    }
    else
    {
        if(gu32OpenBypass&EN_VIP_MDRV_CONFIG)
        {
            _MDrv_VIP_SetBypassIP(0, EN_VIP_MDRV_CONFIG);
        }
        if(gu32OpenBypass&EN_VIP_MDRV_MCNR_CONFIG)
        {
            _MDrv_VIP_SetBypassIP(0, EN_VIP_MDRV_MCNR_CONFIG);
        }
        if(gu32OpenBypass&EN_VIP_MDRV_NLM_CONFIG)
        {
            _MDrv_VIP_SetBypassIP(0, EN_VIP_MDRV_NLM_CONFIG);
        }
        if(gu32OpenBypass&EN_VIP_MDRV_LDC_CONFIG)
        {
            _MDrv_VIP_SetBypassIP(0, EN_VIP_MDRV_LDC_CONFIG);
        }
        MDRV_VIP_MUTEX_LOCK();
        gu32OpenBypass = enVIPtype;
        MDRV_VIP_MUTEX_UNLOCK();
    }
    return TRUE;
}

unsigned char MDrv_VIP_CMDQWriteConfig(ST_MDRV_VIP_CMDQ_CONFIG *pCfg)
{
    MS_U32 u32Addr;
    MS_U16 u16Data,u16Mask;
    MS_U8 bfire;
    MS_CMDQ_CMDReg stCfg;
    bfire           = pCfg->bfire;
    u32Addr         = pCfg->u32Addr;
    u16Data         = pCfg->u16Data;
    u16Mask         = pCfg->u16Mask;
    if(bfire)
    {
        Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,bfire);
        SCL_DBG(SCL_DBG_LV_VIP(), "[MDRVVIP]%s, fire\n", __FUNCTION__);
    }
    else
    {
        Drv_CMDQ_FillCmd(&stCfg, u32Addr, u16Data, u16Mask);
        if(Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0) == 0)
        {
            SCL_DBG(SCL_DBG_LV_VIP(), "[MDRVVIP]%s,No frame cmd Write Fail\n", __FUNCTION__);
            return FALSE;
        }
    }
    return TRUE;

}
unsigned char MDrv_VIP_SetLDCConfig(void *pvCfg)
{
    ST_MDRV_VIP_LDC_CONFIG *pCfg = pvCfg;
	MS_U8 bRet = 0;
    if(_IsFrameBufferAllocatedReady())
    {
        MS_U8 *stEn = NULL,*stmd = NULL;
        stEn = (MS_U8*)&(pCfg->stEn);
        stmd = (MS_U8*)&(pCfg->stmd);
        if(_IsNotToCheckPQorCMDQmode())
        {
            MDRV_VIP_MUTEX_LOCK();
        }
        if(Drv_VIP_CheckIPMResolution())
        {

            if(_MDrv_VIP_SetCMDQStatus(1,pCfg->stFCfg.bEn,pCfg->stFCfg.u8framecnt))
            {
                _Set_VIPCMDQSetFlag(EN_VIP_MDRV_LDC_CONFIG);
                MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,1);
                bRet = 1;
            }
            _MDrv_VIP_SetPQByType((MS_U8)PQ_IP_LDC_422_444_422_Main,stmd);
            Drv_VIP_SetLDCBank_Mode((EN_DRV_VIP_LDCLCBANKMODE_TYPE)pCfg->enLDCType);
            Drv_VIP_SetLDCOnConfig(pCfg->stEn.bEn_ldc);
            MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,bRet);

        }
        else
        {
            SCL_ERR("[MDRVVIP]LDC Resolution Over Spec \n");
        }
        _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_MDRV_LDC_CONFIG,(void *)pCfg);
        if(_IsNotToCheckPQorCMDQmode())
        {
            MDRV_VIP_MUTEX_UNLOCK();
        }
        SCL_DBG(SCL_DBG_LV_VIP(),
            "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stldc,gstSupCfg.bresetflag);
        return TRUE;
    }
    else
    {
        SCL_ERR( "[MDRVVIP]%s,MCNR buffer not alloc \n", __FUNCTION__);
        return FALSE;
    }
}

unsigned char MDrv_VIP_SetLDCmdConfig(void *pvCfg)
{
    ST_MDRV_VIP_LDC_MD_CONFIG *pCfg = pvCfg;
    if(_IsFrameBufferAllocatedReady())
    {
        ST_VIP_LDC_MD_CONFIG stLDCCfg;
        stLDCCfg.stFCfg.bEn         = pCfg->stFCfg.bEn;
        stLDCCfg.stFCfg.u8framecnt  = pCfg->stFCfg.u8framecnt;
        stLDCCfg.bEnSWMode          = pCfg->bEnSWMode;
        stLDCCfg.u8FBidx            = pCfg->u8FBidx;
        stLDCCfg.u8FBrwdiff         = pCfg->u8FBrwdiff;
        stLDCCfg.enbypass           = (EN_VIP_LDC_BYPASS_Drv_TYPE)pCfg->enbypass;
        stLDCCfg.enLDCType          = (EN_DRV_VIP_LDCLCBANKMODE_TYPE)pCfg->enLDCType;
        if(_IsNotToCheckPQorCMDQmode())
        {
            MDRV_VIP_MUTEX_LOCK();
        }

        if(Drv_VIP_SetLDCMdConfig(&stLDCCfg) == 0)
        {
            SCL_DBG(SCL_DBG_LV_VIP(), "[MDRVVIP]%s, Fail\n", __FUNCTION__);
            return FALSE;
        }

        _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_MDRV_LDC_MD_CONFIG,(void *)pCfg);
        if(_IsNotToCheckPQorCMDQmode())
        {
            MDRV_VIP_MUTEX_UNLOCK();
        }
        SCL_DBG(SCL_DBG_LV_VIP(),
            "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stldcmd,gstSupCfg.bresetflag);
        return TRUE;
    }
    else
    {
        SCL_ERR( "[MDRVVIP]%s,MCNR buffer not alloc \n", __FUNCTION__);
        return FALSE;
    }
}

unsigned char MDrv_VIP_SetLDCDmapConfig(void *pvCfg)
{
    ST_MDRV_VIP_LDC_DMAP_CONFIG *pCfg = pvCfg;
    if(_IsFrameBufferAllocatedReady())
    {
        ST_VIP_LDC_DMAP_CONFIG stLDCCfg;
        stLDCCfg.bEnPowerSave       = pCfg->bEnPowerSave;
        stLDCCfg.u32DMAPaddr        = _Phys2Miu(pCfg->u32DMAPaddr);
        stLDCCfg.u16DMAPWidth       = pCfg->u16DMAPWidth;
        stLDCCfg.u8DMAPoffset       = pCfg->u8DMAPoffset;
        stLDCCfg.stFCfg.bEn         = pCfg->stFCfg.bEn;
        stLDCCfg.stFCfg.u8framecnt  = pCfg->stFCfg.u8framecnt;
        if(_IsNotToCheckPQorCMDQmode())
        {
            MDRV_VIP_MUTEX_LOCK();
        }

        if(Drv_VIP_SetLDCDmapConfig(&stLDCCfg) == 0)
        {
            SCL_DBG(SCL_DBG_LV_VIP(), "[MDRVVIP]%s, Fail\n", __FUNCTION__);
            return FALSE;
        }

        _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_MDRV_LDC_DMAP_CONFIG,(void *)pCfg);
        if(_IsNotToCheckPQorCMDQmode())
        {
            MDRV_VIP_MUTEX_UNLOCK();
        }
        SCL_DBG(SCL_DBG_LV_VIP(),
            "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stldcdmap,gstSupCfg.bresetflag);
        return TRUE;
    }
    else
    {
        SCL_ERR( "[MDRVVIP]%s,MCNR buffer not alloc \n", __FUNCTION__);
        return FALSE;
    }
}

unsigned char MDrv_VIP_SetLDCSRAMConfig(void *pvCfg)
{
    ST_MDRV_VIP_LDC_SRAM_CONFIG *pCfg = pvCfg;
    if(_IsFrameBufferAllocatedReady())
    {
        ST_VIP_LDC_SRAM_CONFIG stLDCCfg;
        stLDCCfg.u16SRAMhorstr      = pCfg->u16SRAMhorstr;
        stLDCCfg.u16SRAMverstr      = pCfg->u16SRAMverstr;
        stLDCCfg.u32loadhoraddr     = _Phys2Miu(pCfg->u32loadhoraddr);
        stLDCCfg.u32loadveraddr     = _Phys2Miu(pCfg->u32loadveraddr);
        stLDCCfg.u16SRAMhoramount   = pCfg->u16SRAMhoramount;
        stLDCCfg.u16SRAMveramount   = pCfg->u16SRAMveramount;
        stLDCCfg.stFCfg.bEn         = pCfg->stFCfg.bEn;
        stLDCCfg.stFCfg.u8framecnt  = pCfg->stFCfg.u8framecnt;
        if(_IsNotToCheckPQorCMDQmode())
        {
            MDRV_VIP_MUTEX_LOCK();
        }

        if(Drv_VIP_SetLDCSRAMConfig(&stLDCCfg) == 0)
        {
            SCL_DBG(SCL_DBG_LV_VIP(), "[MDRVVIP]%s, Fail\n", __FUNCTION__);
            return FALSE;
        }

        _MDrv_VIP_CopyIPConfigToGlobal(EN_VIP_MDRV_LDC_SRAM_CONFIG,(void *)pCfg);
        if(_IsNotToCheckPQorCMDQmode())
        {
            MDRV_VIP_MUTEX_UNLOCK();
        }
        SCL_DBG(SCL_DBG_LV_VIP(),
            "[MDRVVIP]%s,addr:%lx flag:%lx \n", __FUNCTION__,(MS_U32)&gstSupCfg.stldcsram,gstSupCfg.bresetflag);
        return TRUE;
    }
    else
    {
        SCL_ERR( "[MDRVVIP]%s,MCNR buffer not alloc \n", __FUNCTION__);
        return FALSE;
    }
}

MS_BOOL MDrv_VIP_VtrackEnable( unsigned char u8FrameRate, EN_VIP_MDRV_VTRACK_ENABLE_TYPE bEnable)
{
    Drv_VIP_VtrackEnable(u8FrameRate, (EN_VIP_DRVVTRACK_ENABLE_TYPE)bEnable);
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
void _MDrv_VIP_AIPSettingDebugMessage
    (ST_MDRV_VIP_AIP_CONFIG *stCfg,void * pvPQSetParameter,MS_U16 u16StructSize)
{
    unsigned short u16AIPsheet;
    unsigned char word1,word2;
    unsigned short u16idx;
    u16AIPsheet = _GetAIPOffset(stCfg->u16AIPType);
    SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[MDRVVIP]Sheet:%hd,size:%hd\n",u16AIPsheet,u16StructSize);
    SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[MDRVVIP]copy addr:%lx,vir addr:%lx\n"
        ,(unsigned long)pvPQSetParameter,(unsigned long)stCfg->u32Viraddr);
    if(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPLOG)
    {
        for(u16idx =0 ;u16idx<u16StructSize;u16idx++)
        {
            word1 = *((unsigned char *)pvPQSetParameter+u16idx);
            word2 = *((unsigned char *)stCfg->u32Viraddr+u16idx);
            SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[MDRVVIP]idx :%hd, copy value:%hhx,vir value:%hhx\n"
                ,u16idx,(unsigned char)(word1),(unsigned char)(word2));
        }
    }
}
void _MDrv_VIP_AIPSRAMSettingDebugMessage
    (ST_MDRV_VIP_AIP_SRAM_CONFIG *stCfg,void * pvPQSetParameter,MS_U16 u16StructSize)
{
    unsigned char word1,word2;
    unsigned short u16idx;
    SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[MDRVVIP]%s\n",__FUNCTION__);
    SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[MDRVVIP]enType:%hd,size:%hd\n",stCfg->enAIPType,u16StructSize);
    SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[MDRVVIP]copy addr:%lx,vir addr:%lx\n"
        ,(unsigned long)pvPQSetParameter,(unsigned long)stCfg->u32Viraddr);
    if(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPLOG)
    {
        for(u16idx =0 ;u16idx<u16StructSize;u16idx++)
        {
            word1 = *((unsigned char *)pvPQSetParameter+u16idx);
            word2 = *((unsigned char *)stCfg->u32Viraddr+u16idx);
            SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[MDRVVIP]idx :%hd, copy value:%hhx,vir value:%hhx\n"
                ,u16idx,(unsigned char)(word1),(unsigned char)(word2));
        }
    }
}
unsigned char MDrv_VIP_SetAIPConfig(ST_MDRV_VIP_AIP_CONFIG *stCfg)
{
    void * pvPQSetParameter;
    MS_U16 u16StructSize;
    unsigned short u16AIPsheet;
	MS_U8 bRet = 0;
    MS_U32 u32Events;
    if(!_Is_AIPBypassFlag(stCfg->u16AIPType))
    {
        u16AIPsheet = _GetAIPOffset(stCfg->u16AIPType);
        u16StructSize = MDrv_PQ_GetIPRegCount(u16AIPsheet);
        if(_IsNotToCheckPQorCMDQmode())
        {
            MDRV_VIP_MUTEX_LOCK();
        }
        pvPQSetParameter = (void *)
        _MDrv_VIP_FillSettingBuffer(u16AIPsheet,stCfg->u32Viraddr,&gstSupCfg.staip[stCfg->u16AIPType].u32Viraddr);
        if(!pvPQSetParameter)
        {
            return 0;
        }
        _MDrv_VIP_AIPSettingDebugMessage(stCfg,pvPQSetParameter,u16StructSize);
        if(stCfg->u16AIPType!=EN_VIP_MDRV_AIP_XNR &&
            stCfg->u16AIPType!=EN_VIP_MDRV_AIP_GM10TO12 &&
            stCfg->u16AIPType!=EN_VIP_MDRV_AIP_GM12TO10 &&
            stCfg->u16AIPType!=EN_VIP_MDRV_AIP_WDR_LOC)
        {
            if(_MDrv_VIP_SetCMDQStatus(1,stCfg->stFCfg.bEn,stCfg->stFCfg.u8framecnt))
            {
                _Set_AIPCMDQSetFlag(stCfg->u16AIPType);
                MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,1);
                bRet = 1;
            }
            _MDrv_VIP_SetPQByType((MS_U8)u16AIPsheet,(MS_U8 *)pvPQSetParameter);
            MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,bRet);
            if(gstSupCfg.staip[stCfg->u16AIPType].u32Viraddr != stCfg->u32Viraddr)
            {
                stCfg->u32Viraddr = (unsigned long)pvPQSetParameter;
                _MDrv_VIP_CopyAIPConfigToGlobal((EN_VIP_MDRV_AIP_TYPE)stCfg->u16AIPType,(void *)stCfg,sizeof(ST_MDRV_VIP_AIP_CONFIG));
            }
        }
        else if(stCfg->u16AIPType==EN_VIP_MDRV_AIP_WDR_LOC)
        {
            if(WDR_USE_CMDQ())
            {
                if(_MDrv_VIP_SetCMDQStatus(1,stCfg->stFCfg.bEn,stCfg->stFCfg.u8framecnt))
                {
                    _Set_AIPCMDQSetFlag(stCfg->u16AIPType);
                    MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,1);
                    bRet = 1;
                }
            }
            else
            {
                if(!Drv_SCLIRQ_GetIsBlankingRegion())
                {
                    MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                }
                MDRv_PQ_Set_CmdqCfg(PQ_MAIN_WINDOW,stCfg->stFCfg.bEn,stCfg->stFCfg.u8framecnt,1);
                MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,stCfg->stFCfg.bEn);
            }
            _MDrv_VIP_SetPQByType((MS_U8)u16AIPsheet,(MS_U8 *)pvPQSetParameter);
            MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,bRet);
            if(gstSupCfg.staip[stCfg->u16AIPType].u32Viraddr != stCfg->u32Viraddr)
            {
                stCfg->u32Viraddr = (unsigned long)pvPQSetParameter;
                _MDrv_VIP_CopyAIPConfigToGlobal((EN_VIP_MDRV_AIP_TYPE)stCfg->u16AIPType,(void *)stCfg,sizeof(ST_MDRV_VIP_AIP_CONFIG));
            }
        }
        else if (stCfg->u16AIPType==EN_VIP_MDRV_AIP_XNR)
        {
            SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_VIP, "[MDRVVIP]%s @:%lu\n",__FUNCTION__,(MS_U32)MsOS_GetSystemTimeStamp());
            if(((MsOS_GetEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID())&(E_SCLIRQ_EVENT_ISPFRMEND|E_SCLIRQ_EVENT_FRMENDSYNC))
                !=(E_SCLIRQ_EVENT_ISPFRMEND|E_SCLIRQ_EVENT_FRMENDSYNC))&&(_IsNotToCheckPQorCMDQmode()))
            {
                if(gstSupCfg.staip[stCfg->u16AIPType].u32Viraddr != stCfg->u32Viraddr)
                {
                    stCfg->u32Viraddr = (unsigned long)pvPQSetParameter;
                    _MDrv_VIP_CopyAIPConfigToGlobal((EN_VIP_MDRV_AIP_TYPE)stCfg->u16AIPType,(void *)stCfg,sizeof(ST_MDRV_VIP_AIP_CONFIG));
                }
                _MDrv_VIP_SetDAZATaskWork(E_VIP_DAZA_BXNR);
            }
            else
            {
                MDRv_PQ_Set_CmdqCfg(PQ_MAIN_WINDOW,stCfg->stFCfg.bEn,stCfg->stFCfg.u8framecnt,1);
                MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,stCfg->stFCfg.bEn);
                _MDrv_VIP_SetPQByType((MS_U8)u16AIPsheet,(MS_U8 *)pvPQSetParameter);
                MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,stCfg->stFCfg.bEn);
                if(gstSupCfg.staip[stCfg->u16AIPType].u32Viraddr != stCfg->u32Viraddr)
                {
                    stCfg->u32Viraddr = (unsigned long)pvPQSetParameter;
                    _MDrv_VIP_CopyAIPConfigToGlobal((EN_VIP_MDRV_AIP_TYPE)stCfg->u16AIPType,(void *)stCfg,sizeof(ST_MDRV_VIP_AIP_CONFIG));
                }
            }
        }
        else if (((stCfg->u16AIPType==EN_VIP_MDRV_AIP_GM10TO12&& !(gbAlreadyOpenGamma&EN_VIP_MDRV_GAMMA_ENABLE_GM10TO12))
            ||(stCfg->u16AIPType==EN_VIP_MDRV_AIP_GM12TO10 && !(gbAlreadyOpenGamma&EN_VIP_MDRV_GAMMA_ENABLE_GM12TO10)))&&
            MDrv_VIP_GetSCLFrameEndCount()<40)
        {
            // for hw bug : gamma can't setting in blanking region
            if(stCfg->u16AIPType==EN_VIP_MDRV_AIP_GM10TO12)
            {
                gbAlreadyOpenGamma |= EN_VIP_MDRV_GAMMA_ENABLE_GM10TO12;
            }
            else if(stCfg->u16AIPType==EN_VIP_MDRV_AIP_GM12TO10)
            {
                gbAlreadyOpenGamma |= EN_VIP_MDRV_GAMMA_ENABLE_GM12TO10;
            }
            // for XNR protect
            if(((MsOS_GetEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID())&(E_SCLIRQ_EVENT_ISPFRMEND|E_SCLIRQ_EVENT_FRMENDSYNC))
                !=(E_SCLIRQ_EVENT_ISPFRMEND|E_SCLIRQ_EVENT_FRMENDSYNC))&&(_IsNotToCheckPQorCMDQmode()))
            {
                if(gstSupCfg.staip[stCfg->u16AIPType].u32Viraddr != stCfg->u32Viraddr)
                {
                    stCfg->u32Viraddr = (unsigned long)pvPQSetParameter;
                    _MDrv_VIP_CopyAIPConfigToGlobal((EN_VIP_MDRV_AIP_TYPE)stCfg->u16AIPType,(void *)stCfg,sizeof(ST_MDRV_VIP_AIP_CONFIG));
                }
                if(stCfg->u16AIPType==EN_VIP_MDRV_AIP_GM10TO12)
                {
                    _MDrv_VIP_SetDAZATaskWork(E_VIP_DAZA_BGMA2C);
                }
                else if(stCfg->u16AIPType==EN_VIP_MDRV_AIP_GM12TO10)
                {
                    _MDrv_VIP_SetDAZATaskWork(E_VIP_DAZA_BGMC2A);
                }
            }
            else
            {
                MDRv_PQ_Set_CmdqCfg(PQ_MAIN_WINDOW,stCfg->stFCfg.bEn,stCfg->stFCfg.u8framecnt,1);
                MDRV_CMDQ_MUTEX_LOCK(EN_CMDQ_TYPE_IP0,stCfg->stFCfg.bEn);
                _Drv_CMDQ_WriteRegWithMaskDirect(REG_SCL0_01_L, BIT3, BIT3);
                _MDrv_VIP_SetPQByType((MS_U8)u16AIPsheet,(MS_U8 *)pvPQSetParameter);
                _Drv_CMDQ_WriteRegWithMaskDirect(REG_SCL0_01_L, 0, BIT3);
                MDRV_CMDQ_MUTEX_UNLOCK(EN_CMDQ_TYPE_IP0,stCfg->stFCfg.bEn);
                if(gstSupCfg.staip[stCfg->u16AIPType].u32Viraddr != stCfg->u32Viraddr)
                {
                    stCfg->u32Viraddr = (unsigned long)pvPQSetParameter;
                    _MDrv_VIP_CopyAIPConfigToGlobal((EN_VIP_MDRV_AIP_TYPE)stCfg->u16AIPType,(void *)stCfg,sizeof(ST_MDRV_VIP_AIP_CONFIG));
                }
            }
        }
        else
        {
            if(gstSupCfg.staip[stCfg->u16AIPType].u32Viraddr != stCfg->u32Viraddr)
            {
                stCfg->u32Viraddr = (unsigned long)pvPQSetParameter;
                _MDrv_VIP_CopyAIPConfigToGlobal((EN_VIP_MDRV_AIP_TYPE)stCfg->u16AIPType,(void *)stCfg,sizeof(ST_MDRV_VIP_AIP_CONFIG));
            }
        }
        if(_IsNotToCheckPQorCMDQmode())
        {
            MDRV_VIP_MUTEX_UNLOCK();
        }
    }
    else
    {
        SCL_DBGERR("[AIP]Bypass AIP:%d\n",stCfg->u16AIPType);
    }
    //MsOS_VirMemFree(pvPQSetParameter);
    return 1;
}

MS_U16 _MDrv_VIP_GetSRAMBufferSize(EN_VIP_MDRV_AIP_SRAM_TYPE enAIPType)
{
    MS_U16 u16StructSize;
    switch(enAIPType)
    {
        case EN_VIP_MDRV_AIP_SRAM_GAMMA_Y:
            u16StructSize = PQ_IP_YUV_Gamma_tblY_SRAM_SIZE_Main;
            break;
        case EN_VIP_MDRV_AIP_SRAM_GAMMA_U:
            u16StructSize = PQ_IP_YUV_Gamma_tblU_SRAM_SIZE_Main;
            break;
        case EN_VIP_MDRV_AIP_SRAM_GAMMA_V:
            u16StructSize = PQ_IP_YUV_Gamma_tblV_SRAM_SIZE_Main;
            break;
        case EN_VIP_MDRV_AIP_SRAM_GM10to12_R:
            u16StructSize = PQ_IP_ColorEng_GM10to12_Tbl_R_SRAM_SIZE_Main;
            break;
        case EN_VIP_MDRV_AIP_SRAM_GM10to12_G:
            u16StructSize = PQ_IP_ColorEng_GM10to12_Tbl_G_SRAM_SIZE_Main;
            break;
        case EN_VIP_MDRV_AIP_SRAM_GM10to12_B:
            u16StructSize = PQ_IP_ColorEng_GM10to12_Tbl_B_SRAM_SIZE_Main;
            break;
        case EN_VIP_MDRV_AIP_SRAM_GM12to10_R:
            u16StructSize = PQ_IP_ColorEng_GM12to10_CrcTbl_R_SRAM_SIZE_Main;
            break;
        case EN_VIP_MDRV_AIP_SRAM_GM12to10_G:
            u16StructSize = PQ_IP_ColorEng_GM12to10_CrcTbl_G_SRAM_SIZE_Main;
            break;
        case EN_VIP_MDRV_AIP_SRAM_GM12to10_B:
            u16StructSize = PQ_IP_ColorEng_GM12to10_CrcTbl_B_SRAM_SIZE_Main;
            break;
        case EN_VIP_MDRV_AIP_SRAM_WDR:
            u16StructSize = (81*2 *8);
            break;
        default:
            u16StructSize = 0;
            break;
    }
    return u16StructSize;
}

unsigned char MDrv_VIP_SetAIPSRAMConfig(ST_MDRV_VIP_AIP_SRAM_CONFIG *stCfg)
{
    void * pvPQSetParameter;
    void * pvPQSetPara;
    MS_U16 u16StructSize;
    EN_VIP_DRV_AIP_SRAM_TYPE enAIPType;
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_LOCK();
    }
    u16StructSize = _MDrv_VIP_GetSRAMBufferSize(stCfg->enAIPType);
    pvPQSetParameter = MsOS_VirMemalloc(u16StructSize);
    if(!pvPQSetParameter)
    {
        SCL_ERR("[MDRVIP]%s(%d) Init pvPQSetParameter Fail\n", __FUNCTION__, __LINE__);
        return 0;
    }
    MsOS_Memset(pvPQSetParameter,0,u16StructSize);
    if(MsOS_copy_from_user(pvPQSetParameter, (__user void  *)stCfg->u32Viraddr, u16StructSize))
    {
        if((stCfg->u32Viraddr&MIU0Vir_BASE)==MIU0Vir_BASE)
        {
            MsOS_Memcpy(pvPQSetParameter, (void *)stCfg->u32Viraddr, u16StructSize);
        }
        else
        {
            SCL_ERR( "[VIP] copy buffer error\n");
            MsOS_VirMemFree(pvPQSetParameter);
            return 0;
        }
    }
    enAIPType = ((EN_VIP_DRV_AIP_SRAM_TYPE)stCfg->enAIPType);
    _MDrv_VIP_AIPSRAMSettingDebugMessage(stCfg,pvPQSetParameter,u16StructSize);
    // NOt need to lock CMDQ ,because hal will do it.
    pvPQSetPara = Drv_VIP_SetAIPSRAMConfig(pvPQSetParameter,enAIPType);
    if(pvPQSetPara!=NULL)
    {
        MsOS_VirMemFree(pvPQSetPara);
    }
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDRV_VIP_MUTEX_UNLOCK();
    }
    return Drv_VIP_GetSRAMCheckPass();
}

unsigned long MDrv_VIP_GetSCLFrameEndCount(void)
{
    ST_SCLIRQ_SCINTS_TYPE *stints;
    stints = Drv_SCLIRQ_GetSCLInts();
    if(stints->u8CountReset)
    {
        return 100;
    }
    else
    {
        return stints->u32SCLMainDoneCount;
    }
}

ssize_t MDrv_VIP_ProcShow(char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    str += MsOS_scnprintf(str, end - str, "========================SCL VIP STATUS======================\n");
    str += MsOS_scnprintf(str, end - str, "Command       IP      Status\n");
    str += MsOS_scnprintf(str, end - str, "----------------------------------------------\n");
    str += MsOS_scnprintf(str, end - str, "   0      Normal ALL     %s\n",(gu32OpenBypass) ? "No" :"Yes");
    str += MsOS_scnprintf(str, end - str, "   1      Bypass ALL     %s\n",(gu32OpenBypass==0x7FFFF) ? "ON" :"OFF");
    str += MsOS_scnprintf(str, end - str, "   2      VIP Bypass     %s\n", Drv_VIP_GetBypassStatus((EN_VIP_DRV_CONFIG_TYPE)EN_VIP_MDRV_CONFIG) ? "ON" :"OFF");
    str += MsOS_scnprintf(str, end - str, "   3          MCNR       %s\n", Drv_VIP_GetBypassStatus((EN_VIP_DRV_CONFIG_TYPE)EN_VIP_MDRV_MCNR_CONFIG) ? "Bypass" :"ON");
    str += MsOS_scnprintf(str, end - str, "   5          NLM        %s\n", Drv_VIP_GetBypassStatus((EN_VIP_DRV_CONFIG_TYPE)EN_VIP_MDRV_NLM_CONFIG) ? "Bypass" :"ON");
    str += MsOS_scnprintf(str, end - str, "   6          LDC        %s\n", Drv_VIP_GetBypassStatus((EN_VIP_DRV_CONFIG_TYPE)EN_VIP_MDRV_LDC_CONFIG) ? "Bypass" :"ON");
    str += MsOS_scnprintf(str, end - str, "-------------------AIP------------------------\n");
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_YEE].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "   4          EE         %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_YEE].u32Viraddr)+71)) ? "ON" :"OFF");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_WDR_GLOB].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "   7      WDR GLOB       %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_WDR_GLOB].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_WDR_LOC].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "   8      WDR LOCAL      %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_WDR_LOC].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_MXNR].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "   9          MXNR       %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_MXNR].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_UVADJ].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "   A          YUVADJ     %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_UVADJ].u32Viraddr)+1)) ? "ON" :"OFF");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_XNR].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "   B          XNR        %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_XNR].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_YCUVM].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "   C          YCUVM      %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_YCUVM].u32Viraddr)+0)) ? "Bypass" :"ON");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_COLORTRAN].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "   D      COLOR TRAN     %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_COLORTRAN].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_GAMMA].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "   E      YUV GAMMA      %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_GAMMA].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    str += MsOS_scnprintf(str, end - str, "---------------COLOR ENGINE--------------------\n");
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_YUVTORGB].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "   F           Y2R       %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_YUVTORGB].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_GM10TO12].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "   G      GM10to12       %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_GM10TO12].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_CCM].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "   H           CCM       %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_CCM].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_HSV].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "   I           HSV       %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_HSV].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_GM12TO10].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "   J      GM12to10       %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_GM12TO10].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_RGBTOYUV].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "   K           R2Y       %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_RGBTOYUV].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    str += MsOS_scnprintf(str, end - str, "----------------------------------------------\n");
    str += MsOS_scnprintf(str, end - str, "========================SCL VIP STATUS======================\n");
    return (str - buf);
}
ssize_t MDrv_VIP_VIPSetRuleShow(char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    str += MsOS_scnprintf(str, end - str, "========================SCL VIPSetRule======================\n");
    str += MsOS_scnprintf(str, end - str, "NOW Rule:%d\n",MsOS_GetVIPSetRule());
    str += MsOS_scnprintf(str, end - str, "echo 0 > VIPSetRule :Default (RIU+assCMDQ)\n");
    str += MsOS_scnprintf(str, end - str, "echo 1 > VIPSetRule :CMDQ in active (RIU blanking+CMDQ active)\n");
    str += MsOS_scnprintf(str, end - str, "echo 2 > VIPSetRule :CMDQ & checking(like 1,but add IST to check)\n");
    str += MsOS_scnprintf(str, end - str, "echo 3 > VIPSetRule :ALLCMDQ (Rule 4)\n");
    str += MsOS_scnprintf(str, end - str, "echo 4 > VIPSetRule :ALLCMDQ & checking(like 4,but add IST to checkSRAM)\n");
    str += MsOS_scnprintf(str, end - str, "echo 5 > VIPSetRule :ALLCMDQ & checking(like 4,but add IST to check)\n");
    str += MsOS_scnprintf(str, end - str, "========================SCL VIPSetRule======================\n");
    return (str - buf);
}

ssize_t MDrv_VIP_CMDQShow(char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    unsigned long long u64DramMsg;
    unsigned short u16Count;
    unsigned short idx,reserve = 0;
    unsigned char bRotate = 0;
    MS_CMDQ_Info *stCMDQinfo = Drv_CMDQ_GetCMDQInformation(EN_CMDQ_TYPE_IP0);
    if(stCMDQinfo->u16WPoint >= stCMDQinfo->u16LPoint)
    {
        u16Count = stCMDQinfo->u16WPoint - stCMDQinfo->u16LPoint;
    }
    else
    {
        u16Count = stCMDQinfo->u16WPoint + stCMDQinfo->u16MaxCmdCnt-stCMDQinfo->u16LPoint;
        bRotate = 1;
    }
    str += MsOS_scnprintf(str, end - str, "========================CMDQ STATUS======================\n");
    str += MsOS_scnprintf(str, end - str, "---------------------- SW INFORMATION -------------------\n");
    str += MsOS_scnprintf(str, end - str, "CMDQ Current CMD End Pointer      :%hd\n",stCMDQinfo->u16WPoint);
    str += MsOS_scnprintf(str, end - str, "CMDQ Last Time Trigger Pointer    :%hd\n",stCMDQinfo->u16RPoint);
    str += MsOS_scnprintf(str, end - str, "CMDQ Last Time Trigger COUNT      :%hd\n",u16Count);
    str += MsOS_scnprintf(str, end - str, "CMDQ Fire Pointer                 :%hd\n",stCMDQinfo->u16FPoint);
    str += MsOS_scnprintf(str, end - str, "---------------------- HW INFORMATION -------------------\n");
    str += MsOS_scnprintf(str, end - str, "CMDQ Start                        :%lx\n",stCMDQinfo->PhyAddr);
    str += MsOS_scnprintf(str, end - str, "CMDQ End                          :%lx\n",stCMDQinfo->PhyAddrEnd);
    str += MsOS_scnprintf(str, end - str, "CMDQ Max CMD One shot             :%hu\n",stCMDQinfo->u16MaxCmdCnt);
    str += MsOS_scnprintf(str, end - str, "---------------------- DRAM INFORMATION -----------------\n");
    for(idx=0;idx<u16Count;idx++)
    {
        if(bRotate)
        {
            if((stCMDQinfo->u16LPoint+idx+1)<=stCMDQinfo->u16MaxCmdCnt)
            {
                u64DramMsg = Drv_CMDQ_GetCMDFromPoint(EN_CMDQ_TYPE_IP0,stCMDQinfo->u16LPoint+idx);
            }
            else
            {
                u64DramMsg = Drv_CMDQ_GetCMDFromPoint(EN_CMDQ_TYPE_IP0,reserve);
                reserve++;
            }
            SCL_DBGERR("%hd:%llx => %lx\n",idx,u64DramMsg,Drv_CMDQ_GetCMDBankFromCMD(u64DramMsg));
            //str += MsOS_scnprintf(str, end - str, "%hd:%llx => %lx\n",idx,u64DramMsg,Drv_CMDQ_GetCMDBankFromCMD(u64DramMsg));
        }
        else
        {
            u64DramMsg = Drv_CMDQ_GetCMDFromPoint(EN_CMDQ_TYPE_IP0,stCMDQinfo->u16LPoint+idx);

            SCL_DBGERR("%hd:%llx => %lx\n",idx,u64DramMsg,Drv_CMDQ_GetCMDBankFromCMD(u64DramMsg));
            //str += MsOS_scnprintf(str, end - str, "%hd:%llx => %lx\n",idx,u64DramMsg,Drv_CMDQ_GetCMDBankFromCMD(u64DramMsg));
        }
    }
    str += MsOS_scnprintf(str, end - str, "========================CMDQ STATUS======================\n");
    return (str - buf);
}
ssize_t MDrv_VIP_VIPShow(char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    str += MsOS_scnprintf(str, end - str, "========================SCL VIP STATUS======================\n");
    str += MsOS_scnprintf(str, end - str, "     IP      Status\n");
    str += MsOS_scnprintf(str, end - str, "---------------VIP---------------------\n");
    str += MsOS_scnprintf(str, end - str, "  Normal ALL     %s\n",(gu32OpenBypass) ? "No" :"Yes");
    str += MsOS_scnprintf(str, end - str, "  Bypass ALL     %s\n",(gu32OpenBypass==0x7FFFF) ? "ON" :"OFF");
    str += MsOS_scnprintf(str, end - str, "  VIP Bypass     %s\n", Drv_VIP_GetBypassStatus((EN_VIP_DRV_CONFIG_TYPE)EN_VIP_MDRV_CONFIG) ? "ON" :"OFF");
    str += MsOS_scnprintf(str, end - str, "     MCNR         %s\n", Drv_VIP_GetBypassStatus((EN_VIP_DRV_CONFIG_TYPE)EN_VIP_MDRV_MCNR_CONFIG) ? "Bypass" :"ON");
    str += MsOS_scnprintf(str, end - str, "     NLM         %s\n", Drv_VIP_GetBypassStatus((EN_VIP_DRV_CONFIG_TYPE)EN_VIP_MDRV_NLM_CONFIG) ? "Bypass" :"ON");
    str += MsOS_scnprintf(str, end - str, "     LDC         %s\n", Drv_VIP_GetBypassStatus((EN_VIP_DRV_CONFIG_TYPE)EN_VIP_MDRV_LDC_CONFIG) ? "Bypass" :"ON");
    str += MsOS_scnprintf(str, end - str, "     ACK         %s\n", gstSupCfg.stack.stACK.backen ? "ON" :"OFF");
    str += MsOS_scnprintf(str, end - str, "     IBC         %s\n", gstSupCfg.stibc.stEn.bIBC_en ? "ON" :"OFF");
    str += MsOS_scnprintf(str, end - str, "     ICC         %s\n", gstSupCfg.sticc.stEn.bICC_en ? "ON" :"OFF");
    str += MsOS_scnprintf(str, end - str, "     IHC         %s\n", gstSupCfg.stihc.stOnOff.bIHC_en? "ON" :"OFF");
    str += MsOS_scnprintf(str, end - str, "     FCC1        %s\n", gstSupCfg.stfcc.stT[0].bEn ? "ON" :"OFF");
    str += MsOS_scnprintf(str, end - str, "     FCC2        %s\n", gstSupCfg.stfcc.stT[1].bEn ? "ON" :"OFF");
    str += MsOS_scnprintf(str, end - str, "     FCC3        %s\n", gstSupCfg.stfcc.stT[2].bEn ? "ON" :"OFF");
    str += MsOS_scnprintf(str, end - str, "     FCC4        %s\n", gstSupCfg.stfcc.stT[3].bEn ? "ON" :"OFF");
    str += MsOS_scnprintf(str, end - str, "     FCC5        %s\n", gstSupCfg.stfcc.stT[4].bEn ? "ON" :"OFF");
    str += MsOS_scnprintf(str, end - str, "     FCC6        %s\n", gstSupCfg.stfcc.stT[5].bEn ? "ON" :"OFF");
    str += MsOS_scnprintf(str, end - str, "     FCC7        %s\n", gstSupCfg.stfcc.stT[6].bEn ? "ON" :"OFF");
    str += MsOS_scnprintf(str, end - str, "     FCC8        %s\n", gstSupCfg.stfcc.stT[7].bEn ? "ON" :"OFF");
    str += MsOS_scnprintf(str, end - str, "     FCC9        %s\n", gstSupCfg.stfcc.stT9.bEn ? "ON" :"OFF");
    str += MsOS_scnprintf(str, end - str, "     UVC         %s\n", gstSupCfg.stuvc.stUVC.buvc_en? "ON" :"OFF");
    str += MsOS_scnprintf(str, end - str, " DLC CURVEFITPW  %s\n", gstSupCfg.stdlc.stEn.bcurve_fit_var_pw_en? "ON" :"OFF");
    str += MsOS_scnprintf(str, end - str, " DLC  CURVEFIT   %s\n", gstSupCfg.stdlc.stEn.bcurve_fit_en? "ON" :"OFF");
    str += MsOS_scnprintf(str, end - str, " DLC  STATISTIC  %s\n", gstSupCfg.stdlc.stEn.bstatistic_en? "ON" :"OFF");
    str += MsOS_scnprintf(str, end - str, "     LCE         %s\n", gstSupCfg.stlce.stOnOff.bLCE_En ? "ON" :"OFF");
    str += MsOS_scnprintf(str, end - str, "     PK          %s\n", gstSupCfg.stpk.stOnOff.bpost_peaking_en ? "ON" :"OFF");
    str += MsOS_scnprintf(str, end - str, "---------------AIP--------------------\n");

    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_YEE].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "     EE          %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_YEE].u32Viraddr)+71)) ? "ON" :"OFF");
        str += MsOS_scnprintf(str, end - str, "     EYEE        %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_YEE].u32Viraddr)+70)) ? "ON" :"OFF");
        str += MsOS_scnprintf(str, end - str, "     YEE Merge   %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_YEE].u32Viraddr)+204)) ? "ON" :"OFF");
    }

    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_YEE_AC_LUT].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "     YC SEC From %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_YEE_AC_LUT].u32Viraddr)+64)==0) ? "YEE" :
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_YEE_AC_LUT].u32Viraddr)+64)==0x1) ? "2DPK" :
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_YEE_AC_LUT].u32Viraddr)+64)==0x2) ? "MIX" :"Debug mode" );
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_WDR_GLOB].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "     WDR GLOB    %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_WDR_GLOB].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_WDR_LOC].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "     WDR LOCAL   %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_WDR_LOC].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_MXNR].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "     MXNR        %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_MXNR].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_UVADJ].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "     YUVADJ      %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_UVADJ].u32Viraddr)+1)) ? "ON" :"OFF");
        str += MsOS_scnprintf(str, end - str, "     UVADJbyY    %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_UVADJ].u32Viraddr)+0)) ? "ON" :"OFF");
        str += MsOS_scnprintf(str, end - str, "     UVADJbyS    %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_UVADJ].u32Viraddr)+2)) ? "ON" :"OFF");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_XNR].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "     XNR         %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_XNR].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_YCUVM].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "     YCUVM       %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_YCUVM].u32Viraddr)+0)) ? "Bypass" :"ON");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_COLORTRAN].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "     COLOR TRAN  %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_COLORTRAN].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_GAMMA].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "     YUV GAMMA   %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_GAMMA].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    str += MsOS_scnprintf(str, end - str, "---------------COLOR ENGINE--------------------\n");
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_YUVTORGB].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "     Y2R         %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_YUVTORGB].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_GM10TO12].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "     GM10to12    %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_GM10TO12].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_CCM].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "     CCM         %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_CCM].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_HSV].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "     HSV         %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_HSV].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_GM12TO10].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "     GM12to10    %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_GM12TO10].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstSupCfg.staip[EN_VIP_MDRV_AIP_RGBTOYUV].u32Viraddr)
    {
        str += MsOS_scnprintf(str, end - str, "     R2Y         %s\n",
            (*((unsigned char *)(gstSupCfg.staip[EN_VIP_MDRV_AIP_RGBTOYUV].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    str += MsOS_scnprintf(str, end - str, "---------------SRAM--------------------\n");
    str += MsOS_scnprintf(str, end - str, "-1-----------Y GAMMA--------------------\n");
    str += MsOS_scnprintf(str, end - str, "-2-----------U GAMMA--------------------\n");
    str += MsOS_scnprintf(str, end - str, "-3-----------V GAMMA--------------------\n");
    str += MsOS_scnprintf(str, end - str, "-4-----------R GAMMAA2C--------------------\n");
    str += MsOS_scnprintf(str, end - str, "-5-----------G GAMMAA2C--------------------\n");
    str += MsOS_scnprintf(str, end - str, "-6-----------B GAMMAA2C--------------------\n");
    str += MsOS_scnprintf(str, end - str, "-7-----------R GAMMAC2A--------------------\n");
    str += MsOS_scnprintf(str, end - str, "-8-----------G GAMMAC2A--------------------\n");
    str += MsOS_scnprintf(str, end - str, "-9-----------B GAMMAC2A--------------------\n");
    str += MsOS_scnprintf(str, end - str, "-A-----------WDR--------------------\n");
    str += MsOS_scnprintf(str, end - str, "========================SCL VIP STATUS======================\n");
    return (str - buf);
}
void MDrv_VIP_VIPStore(const char *buf)
{
    const char *str = buf;
    if(NULL != buf)
    {
        if((int)*str == 49)    //input 1  echo 1 >
        {
            Drv_CMDQ_CheckVIPSRAM(E_SCLIRQ_EVENT_GAMMA_Y);
        }
        else if((int)*str == 50)  //input 2  echo 2 >
        {
            Drv_CMDQ_CheckVIPSRAM(E_SCLIRQ_EVENT_GAMMA_U);
        }
        else if((int)*str == 51)  //input 3  echo 3 >
        {
            Drv_CMDQ_CheckVIPSRAM(E_SCLIRQ_EVENT_GAMMA_V);
        }
        else if((int)*str == 52)  //input 4  echo 4 >
        {
            Drv_CMDQ_CheckVIPSRAM(E_SCLIRQ_EVENT_GM10to12_R);
        }
        else if((int)*str == 53)  //input 5  echo 5 >
        {
            Drv_CMDQ_CheckVIPSRAM(E_SCLIRQ_EVENT_GM10to12_G);
        }
        else if((int)*str == 54)  //input 6  echo 6 >
        {
            Drv_CMDQ_CheckVIPSRAM(E_SCLIRQ_EVENT_GM10to12_B);
        }
        else if((int)*str == 55)  //input 7  echo 7 >
        {
            Drv_CMDQ_CheckVIPSRAM(E_SCLIRQ_EVENT_GM12to10_R);
        }
        else if((int)*str == 56)  //input 8  echo 8 >
        {
            Drv_CMDQ_CheckVIPSRAM(E_SCLIRQ_EVENT_GM12to10_G);
        }
        else if((int)*str == 57)  //input 9  echo 9 >
        {
            Drv_CMDQ_CheckVIPSRAM(E_SCLIRQ_EVENT_GM12to10_B);
        }
        else if((int)*str == 65)  //input A  echo A >
        {
        }

    }
}
