//<MStar Software>
//******************************************************************************
// MStar Software
// Copyright (c) 2010 - 2012 MStar Semiconductor, Inc. All rights reserved.
// All software, firmware and related documentation herein ("MStar Software") are
// intellectual property of MStar Semiconductor, Inc. ("MStar") and protected by
// law, including, but not limited to, copyright law and international treaties.
// Any use, modification, reproduction, retransmission, or republication of all
// or part of MStar Software is expressly prohibited, unless prior written
// permission has been granted by MStar.
//
// By accessing, browsing and/or using MStar Software, you acknowledge that you
// have read, understood, and agree, to be bound by below terms ("Terms") and to
// comply with all applicable laws and regulations:
//
// 1. MStar shall retain any and all right, ownership and interest to MStar
//    Software and any modification/derivatives thereof.
//    No right, ownership, or interest to MStar Software and any
//    modification/derivatives thereof is transferred to you under Terms.
//
// 2. You understand that MStar Software might include, incorporate or be
//    supplied together with third party`s software and the use of MStar
//    Software may require additional licenses from third parties.
//    Therefore, you hereby agree it is your sole responsibility to separately
//    obtain any and all third party right and license necessary for your use of
//    such third party`s software.
//
// 3. MStar Software and any modification/derivatives thereof shall be deemed as
//    MStar`s confidential information and you agree to keep MStar`s
//    confidential information in strictest confidence and not disclose to any
//    third party.
//
// 4. MStar Software is provided on an "AS IS" basis without warranties of any
//    kind. Any warranties are hereby expressly disclaimed by MStar, including
//    without limitation, any warranties of merchantability, non-infringement of
//    intellectual property rights, fitness for a particular purpose, error free
//    and in conformity with any international standard.  You agree to waive any
//    claim against MStar for any loss, damage, cost or expense that you may
//    incur related to your use of MStar Software.
//    In no event shall MStar be liable for any direct, indirect, incidental or
//    consequential damages, including without limitation, lost of profit or
//    revenues, lost or damage of data, and unauthorized system use.
//    You agree that this Section 4 shall still apply without being affected
//    even if MStar Software has been modified by MStar in accordance with your
//    request or instruction for your use, except otherwise agreed by both
//    parties in writing.
//
// 5. If requested, MStar may from time to time provide technical supports or
//    services in relation with MStar Software to you for your use of
//    MStar Software in conjunction with your or your customer`s product
//    ("Services").
//    You understand and agree that, except otherwise agreed by both parties in
//    writing, Services are provided on an "AS IS" basis and the warranty
//    disclaimer set forth in Section 4 above shall apply.
//
// 6. Nothing contained herein shall be construed as by implication, estoppels
//    or otherwise:
//    (a) conferring any license or right to use MStar name, trademark, service
//        mark, symbol or any other identification;
//    (b) obligating MStar or any of its affiliates to furnish any person,
//        including without limitation, you and your customers, any assistance
//        of any kind whatsoever, or any information; or
//    (c) conferring any license or right under any intellectual property right.
//
// 7. These terms shall be governed by and construed in accordance with the laws
//    of Taiwan, R.O.C., excluding its conflict of law rules.
//    Any and all dispute arising out hereof or related hereto shall be finally
//    settled by arbitration referred to the Chinese Arbitration Association,
//    Taipei in accordance with the ROC Arbitration Law and the Arbitration
//    Rules of the Association by three (3) arbitrators appointed in accordance
//    with the said Rules.
//    The place of arbitration shall be in Taipei, Taiwan and the language shall
//    be English.
//    The arbitration award shall be final and binding to both parties.
//
//******************************************************************************
//<MStar Software>
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2008-2009 MStar Semiconductor, Inc.
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
#define DRV_VIP_C

#ifdef MSOS_TYPE_LINUX_KERNEL
//#include <linux/wait.h>
//#include <linux/irqreturn.h>
//#include <asm/div64.h>
#endif

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
#include "MsCommon.h"
#include "MsTypes.h"
#include "MsOS.h"
#include "hwreg.h"
#include "drvvip.h"
#include "halvip.h"
#include "mdrv_scl_dbg.h"
#include "drvCMDQ.h"
#include "drvhvsp_st.h"
#include "drvhvsp.h"
#include "drvsclirq_st.h"
#include "drvsclirq.h"
#include "drvPQ_Define.h"
#include "drvPQ_Declare.h"
#include "drvPQ.h"

//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define DRV_VIP_DBG 0
#define DRV_VIP_LDC_DMAP_align(x,align) ((x+align) & ~(align-1))
#define DMAPBLOCKUNIT       32
#define DMAPBLOCKUNITBYTE   4
#define DMAPBLOCKALIGN      4
#define DEFAULTLDCMD 0x1
#define SRAMNORMAL NULL

#define DLCVariableSection 8
#define DLCCurveFitEnable 7
#define DLCCurveFitRGBEnable 13
#define DLCDitherEnable 5
#define DLCHistYRGBEnable 10
#define DLCStaticEnable 1
#define LDCFBRWDiff 4
#define LDCSWModeEnable 10
#define LDCAppointFBidx 2
#define FHD_Width   1920
#define FHD_Height  1080
#define DRV_VIP_MUTEX_LOCK()            MsOS_ObtainMutex(_VIP_Mutex,MSOS_WAIT_FOREVER)
#define DRV_VIP_MUTEX_UNLOCK()          MsOS_ReleaseMutex(_VIP_Mutex)


//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------
MS_S32 _VIP_Mutex = -1;
void *gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_NUM];
MS_BOOL gbSRAMCkeckPass = 1;
//-------------------------------------------------------------------------------------------------
//  Loacl Functions
//-------------------------------------------------------------------------------------------------
MS_U16 _Drv_VIP_GetDLCEnableSetting(ST_VIP_DLC_HISTOGRAM_CONFIG *stDLCCfg)
{
    MS_U16 u16valueForReg1Eh04;
    u16valueForReg1Eh04 = ((MS_U16)stDLCCfg->bVariable_Section<<DLCVariableSection)|
        ((MS_U16)stDLCCfg->bcurve_fit_en<<DLCCurveFitEnable)|
        ((MS_U16)stDLCCfg->bcurve_fit_rgb_en<<DLCCurveFitRGBEnable)|
        ((MS_U16)stDLCCfg->bDLCdither_en<<DLCDitherEnable)|
        ((MS_U16)stDLCCfg->bhis_y_rgb_mode_en<<DLCHistYRGBEnable)|
        ((MS_U16)stDLCCfg->bstatic<<DLCStaticEnable);
    return u16valueForReg1Eh04;
}
void _Drv_VIP_For_SetDLCHistRangeEachSection(ST_VIP_DLC_HISTOGRAM_CONFIG *stDLCCfg)
{
    MS_U16 u16sec;
    MS_U16 u16tvalue;
    MS_U16 u16Mask;
    MS_U32 u32Reg;
    MS_CMDQ_CMDReg stCfg;
    MsOS_Memset(&stCfg,0,sizeof(MS_CMDQ_CMDReg));
    if(stDLCCfg->stFCfg.bEn)
    {
        for(u16sec=0;u16sec<VIP_DLC_HISTOGRAM_SECTION_NUM;u16sec++)
        {
            u32Reg = REG_VIP_DLC_0C_L+(((u16sec)/2)*2);
            if((u16sec%2) == 0)
            {
                u16Mask     = 0x00FF;
                u16tvalue   = ((MS_U16)stDLCCfg->u8Histogram_Range[u16sec]);

            }
            else
            {
                u16Mask     = 0xFF00;
                u16tvalue   = ((MS_U16)stDLCCfg->u8Histogram_Range[u16sec])<<8;
            }
            Drv_CMDQ_FillCmd(&stCfg,u32Reg,(u16tvalue),u16Mask);
            Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
        }
    }
    else
    {
        for(u16sec=0;u16sec<VIP_DLC_HISTOGRAM_SECTION_NUM;u16sec++)
        {
            Hal_VIP_DLCHistSetRange(stDLCCfg->u8Histogram_Range[u16sec],u16sec+1);
        }
    }
}
void _Drv_VIP_SetDLCHistogramCMDQConfig
    (ST_VIP_DLC_HISTOGRAM_CONFIG *stDLCCfg,MS_U16 u16valueForReg1Eh04)
{
    MS_CMDQ_CMDReg stCfg;
    MsOS_Memset(&stCfg,0,sizeof(MS_CMDQ_CMDReg));
    Drv_CMDQ_FillCmd(&stCfg,REG_VIP_DLC_04_L,(u16valueForReg1Eh04),0x25a2);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_VIP_DLC_03_L,(stDLCCfg->u8HistSft),0x0007);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_VIP_MWE_15_L,(stDLCCfg->bstat_MIU),0x0001);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_VIP_MWE_1C_L,((MS_U16)stDLCCfg->u8trig_ref_mode)<<2,0x0004);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_VIP_MWE_18_L,((MS_U16)stDLCCfg->u32StatBase[0]>>4),0xFFFF);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_VIP_MWE_19_L,(MS_U16)(stDLCCfg->u32StatBase[0]>>20),0x07FF);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_VIP_MWE_1A_L,(MS_U16)stDLCCfg->u32StatBase[1]>>4,0xFFFF);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_VIP_MWE_1B_L,(MS_U16)((MS_U32)stDLCCfg->u32StatBase[1]>>20),0x07FF);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_VIP_DLC_08_L,((MS_U16)stDLCCfg->bRange)<<7,0x0080);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_VIP_MWE_01_L,stDLCCfg->u16Vst,0x03FF);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_VIP_MWE_02_L,stDLCCfg->u16Vnd,0x03FF);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_VIP_MWE_03_L,stDLCCfg->u16Hst,0x01FF);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_VIP_MWE_04_L,stDLCCfg->u16Vnd,0x01FF);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);

    _Drv_VIP_For_SetDLCHistRangeEachSection(stDLCCfg);
    Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,1);
}
void _Drv_VIP_SetDLCHistogramRIUConfig(ST_VIP_DLC_HISTOGRAM_CONFIG *stDLCCfg,MS_U16 u16valueForReg1Eh04)
{
    Hal_VIP_DLCHistVarOnOff(u16valueForReg1Eh04);
    Hal_VIP_SetDLCstatMIU(stDLCCfg->bstat_MIU,stDLCCfg->u32StatBase[0],stDLCCfg->u32StatBase[1]);
    Hal_VIP_SetDLCshift(stDLCCfg->u8HistSft);
    HAl_VIP_SetDLCmode(stDLCCfg->u8trig_ref_mode);
    Hal_VIP_SetDLCActWin(stDLCCfg->bRange,stDLCCfg->u16Vst,stDLCCfg->u16Hst,stDLCCfg->u16Vnd,stDLCCfg->u16Hnd);
    _Drv_VIP_For_SetDLCHistRangeEachSection(stDLCCfg);
}
void _Drv_VIP_GetDLCHistogramConfig(ST_VIP_DLC_HISTOGRAM_REPORT *stdlc)
{
    stdlc->u32PixelCount  = Hal_VIP_DLCGetPC();
    stdlc->u32PixelWeight = Hal_VIP_DLCGetPW();
    stdlc->u8Baseidx      = Hal_VIP_DLCGetBaseidx();
    stdlc->u8MaxPixel     = Hal_VIP_DLCGetMaxP();
    stdlc->u8MinPixel     = Hal_VIP_DLCGetMinP();
}
MS_BOOL _Drv_VIP_SetLDCFrameBufferConfig(ST_VIP_LDC_MD_CONFIG *stLDCCfg)
{
    Hal_VIP_SetLDCBypass(stLDCCfg->enbypass);
    Hal_VIP_SetLDCHWrwDiff(((MS_U16)stLDCCfg->u8FBrwdiff));
    Hal_VIP_SetLDCSW_Idx(stLDCCfg->u8FBidx);
    Hal_VIP_SetLDCSW_Mode(stLDCCfg->bEnSWMode);
    if(!stLDCCfg->enbypass)
    {
        //HW issue: 64pixel mode bypass can't use;
        Drv_VIP_SetLDCBank_Mode(stLDCCfg->enLDCType);
    }
    else
    {
        Drv_VIP_SetLDCBank_Mode(EN_DRV_VIP_LDCLCBANKMODE_128);
    }
    return TRUE;
}
void Drv_VIP_SetLDCBank_Mode(EN_DRV_VIP_LDCLCBANKMODE_TYPE enType)
{
    Hal_VIP_SetLDCBank_Mode(enType);
}
MS_BOOL _Drv_VIP_SetLDCFrameBufferConfigByCMDQ(ST_VIP_LDC_MD_CONFIG *stLDCCfg)
{
    MS_CMDQ_CMDReg stCfg;
    MsOS_Memset(&stCfg,0,sizeof(MS_CMDQ_CMDReg));
    Drv_CMDQ_FillCmd(&stCfg,REG_SCL_LDC_10_L,stLDCCfg->u8FBidx,BIT1|BIT0);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_SCL_LDC_11_L,stLDCCfg->u8FBrwdiff,BIT1|BIT0);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_SCL_LDC_12_L,stLDCCfg->bEnSWMode,BIT0);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_SCL_LDC_0F_L,((MS_U16)stLDCCfg->enbypass),0x0001);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    if(!stLDCCfg->enbypass)
    {
        //HW issue: 64pixel mode bypass can't use;
        Drv_CMDQ_FillCmd(&stCfg,REG_SCL_LDC_09_L,
        ((MS_U16)stLDCCfg->enLDCType == EN_DRV_HVSP_LDCLCBANKMODE_64)? BIT0 :0,BIT0);
        Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    }
    else
    {
        Drv_CMDQ_FillCmd(&stCfg,REG_SCL_LDC_09_L,0,BIT0);
        Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    }
    Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,1);
    return TRUE;
}
void _Drv_VIP_SetNLMSRAMbyAutodownload(ST_VIP_NLM_SRAM_CONFIG *stCfg)
{
    Hal_VIP_SetAutodownloadAddr(stCfg->u32baseadr,stCfg->u16iniaddr,VIP_NLM_AUTODOWNLOAD_CLIENT);
    Hal_VIP_SetAutodownloadReq(stCfg->u16depth,stCfg->u16reqlen,VIP_NLM_AUTODOWNLOAD_CLIENT);
    Hal_VIP_SetAutodownload(stCfg->bCLientEn,stCfg->btrigContinue,VIP_NLM_AUTODOWNLOAD_CLIENT);
#if DRV_VIP_DBG
    MS_U16 u16entry;
    for(u16entry=0;u16entry<VIP_NLM_ENTRY_NUM;u16entry++)
    {
        Hal_VIP_GetNLMSRAM(u16entry);
    }
#endif
}
void _Drv_VIP_SetNLMSRAMbyCPU(ST_VIP_NLM_SRAM_CONFIG *stCfg)
{
    MS_U16 u16entry;
    MS_U32 u32value,u32addr;
    MS_U32 *pu32Addr = NULL;
    for(u16entry = 0;u16entry<VIP_NLM_ENTRY_NUM;u16entry++)
    {
        u32addr  = stCfg->u32viradr + u16entry * VIP_NLM_AUTODOWNLOAD_BASE_UNIT ;// 1entry cost 16 byte(128 bit)
        pu32Addr = (MS_U32 *)(u32addr);
        u32value = *pu32Addr;
        Hal_VIP_SetNLMSRAMbyCPU(u16entry,u32value);
#if DRV_VIP_DBG
        Hal_VIP_GetNLMSRAM(u16entry);
#endif
    }
}
MS_U32 _Drv_VIP_GetLDCDmapPitch(ST_VIP_LDC_DMAP_CONFIG *stLDCCfg)
{
    MS_U32 u32DMAP_pitch;
    u32DMAP_pitch = ((stLDCCfg->u16DMAPWidth/DMAPBLOCKUNIT)+1);
    u32DMAP_pitch = DRV_VIP_LDC_DMAP_align(u32DMAP_pitch,DMAPBLOCKALIGN);
    u32DMAP_pitch = u32DMAP_pitch * DMAPBLOCKUNITBYTE;
    return u32DMAP_pitch;
}
void _Drv_VIP_Set_LDCDmapByCMDQ(ST_VIP_LDC_DMAP_CONFIG *stLDCCfg,MS_U32 u32DMAP_pitch)
{
    MS_CMDQ_CMDReg stCfg;
    MsOS_Memset(&stCfg,0,sizeof(MS_CMDQ_CMDReg));
    Drv_CMDQ_FillCmd(&stCfg,REG_SCL_LDC_0A_L,((MS_U16)stLDCCfg->u32DMAPaddr),0xFFFF);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_SCL_LDC_0B_L,((MS_U16)(stLDCCfg->u32DMAPaddr>>16)),0xFFFF);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_SCL_LDC_0C_L,((MS_U16)u32DMAP_pitch),0xFFFF);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_SCL_LDC_0E_L,(((MS_U16)stLDCCfg->bEnPowerSave)<<6|((MS_U16)stLDCCfg->u8DMAPoffset)),0x007F);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,1);
}
void _Drv_VIP_SetLDCDmap(ST_VIP_LDC_DMAP_CONFIG *stLDCCfg,MS_U32 u32DMAP_pitch)
{
    Hal_VIP_SetLDCDmapBase(stLDCCfg->u32DMAPaddr);
    Hal_VIP_SetLDCDmapPitch(u32DMAP_pitch);
    Hal_VIP_SetLDCDmapOffset(stLDCCfg->u8DMAPoffset);
    Hal_VIP_SetLDCDmapPS(stLDCCfg->bEnPowerSave);
}
void _Drv_VIP_SetLDCSRAMCfgByCMDQ(ST_VIP_LDC_SRAM_CONFIG *stLDCCfg)
{
    MS_CMDQ_CMDReg stCfg;
    MsOS_Memset(&stCfg,0,sizeof(MS_CMDQ_CMDReg));
    Drv_CMDQ_FillCmd(&stCfg,REG_SCL_LDC_22_L,((MS_U16)stLDCCfg->u16SRAMhoramount),0xFFFF);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_SCL_LDC_2A_L,((MS_U16)stLDCCfg->u16SRAMveramount),0xFFFF);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_SCL_LDC_24_L,((MS_U16)(stLDCCfg->u32loadhoraddr>>4)),0xFFFF);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_SCL_LDC_25_L,((MS_U16)(stLDCCfg->u32loadhoraddr>>20)),0xFFFF);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_SCL_LDC_2C_L,((MS_U16)(stLDCCfg->u32loadveraddr>>4)),0xFFFF);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_SCL_LDC_2D_L,((MS_U16)(stLDCCfg->u32loadveraddr>>20)),0xFFFF);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_SCL_LDC_23_L,((MS_U16)stLDCCfg->u16SRAMhorstr),0xFFFF);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_FillCmd(&stCfg,REG_SCL_LDC_2B_L,((MS_U16)stLDCCfg->u16SRAMverstr),0xFFFF);
    Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
    Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,1);
}
//-------------------------------------------------------------------------------------------------
//  Functions
//-------------------------------------------------------------------------------------------------
void Drv_VIP_Exit(void)
{
    int i;
    if(_VIP_Mutex != -1)
    {
        MsOS_DeleteMutex(_VIP_Mutex);
        _VIP_Mutex = -1;
    }
    for(i=0;i<EN_VIP_DRV_AIP_SRAM_NUM;i++)
    {
        MsOS_VirMemFree(gpvSRAMBuffer[i]);
        gpvSRAMBuffer[i] = NULL;
    }
    Hal_VIP_Exit();
}
MS_U16 _Drv_VIP_GetSRAMBufferSize(EN_VIP_DRV_AIP_SRAM_TYPE enAIPType)
{
    MS_U16 u16StructSize;
    switch(enAIPType)
    {
        case EN_VIP_DRV_AIP_SRAM_GAMMA_Y:
            u16StructSize = PQ_IP_YUV_Gamma_tblY_SRAM_SIZE_Main;
            break;
        case EN_VIP_DRV_AIP_SRAM_GAMMA_U:
            u16StructSize = PQ_IP_YUV_Gamma_tblU_SRAM_SIZE_Main;
            break;
        case EN_VIP_DRV_AIP_SRAM_GAMMA_V:
            u16StructSize = PQ_IP_YUV_Gamma_tblV_SRAM_SIZE_Main;
            break;
        case EN_VIP_DRV_AIP_SRAM_GM10to12_R:
            u16StructSize = PQ_IP_ColorEng_GM10to12_Tbl_R_SRAM_SIZE_Main;
            break;
        case EN_VIP_DRV_AIP_SRAM_GM10to12_G:
            u16StructSize = PQ_IP_ColorEng_GM10to12_Tbl_G_SRAM_SIZE_Main;
            break;
        case EN_VIP_DRV_AIP_SRAM_GM10to12_B:
            u16StructSize = PQ_IP_ColorEng_GM10to12_Tbl_B_SRAM_SIZE_Main;
            break;
        case EN_VIP_DRV_AIP_SRAM_GM12to10_R:
            u16StructSize = PQ_IP_ColorEng_GM12to10_CrcTbl_R_SRAM_SIZE_Main;
            break;
        case EN_VIP_DRV_AIP_SRAM_GM12to10_G:
            u16StructSize = PQ_IP_ColorEng_GM12to10_CrcTbl_G_SRAM_SIZE_Main;
            break;
        case EN_VIP_DRV_AIP_SRAM_GM12to10_B:
            u16StructSize = PQ_IP_ColorEng_GM12to10_CrcTbl_B_SRAM_SIZE_Main;
            break;
        case EN_VIP_DRV_AIP_SRAM_WDR:
            u16StructSize = (81*2 *8);
            break;
        default:
            u16StructSize = 0;
            break;
    }
    return u16StructSize;
}

MS_BOOL Drv_VIP_Init(ST_VIP_INIT_CONFIG *pCfg)
{
    char word[] = {"_VIP_Mutex"};
    int idx;
    if(_VIP_Mutex != -1)
    {
        SCL_DBG(SCL_DBG_LV_DRVVIP(), "[DRVVIP]%s already done\n", __FUNCTION__);
        return TRUE;
    }

    _VIP_Mutex = MsOS_CreateMutex(E_MSOS_FIFO, word, MSOS_PROCESS_SHARED);

    if (_VIP_Mutex == -1)
    {
        SCL_DBG(SCL_DBG_LV_DRVVIP(), "[DRVVIP]%s create mutex fail\n", __FUNCTION__);
        return FALSE;
    }

    Hal_VIP_Set_Riu_Base(pCfg->u32RiuBase);
    Drv_VIP_SRAM_Dump();
    for(idx=0;idx<EN_VIP_DRV_AIP_SRAM_NUM;idx++)
    {
        gpvSRAMBuffer[(EN_VIP_DRV_AIP_SRAM_TYPE)idx] =
            MsOS_VirMemalloc(_Drv_VIP_GetSRAMBufferSize((EN_VIP_DRV_AIP_SRAM_TYPE)idx));
        if(!gpvSRAMBuffer[(EN_VIP_DRV_AIP_SRAM_TYPE)idx])
        {
            SCL_ERR("[DRVSCLDMA]%s(%d) Init gpvSRAMBuffer Fail\n", __FUNCTION__, __LINE__);
            return 0;
        }
    }


    return TRUE;
}
void _Drv_VIPLDCInit(void)
{
    SCL_DBG(SCL_DBG_LV_DRVVIP(), "[DRVVIP]%s: ", __FUNCTION__);
    Hal_VIP_SetLDC422_444_Md(DEFAULTLDCMD);
    Hal_VIP_SetLDC444_422_Md(0);
    Hal_VIP_LDCECO();
}
void Drv_VIP_Open(void)
{
    Hal_VIP_InitY2R();
}
MS_BOOL Drv_VIP_GetIsBlankingRegion(void)
{
    //unsigned long u32Events;
    if(Drv_SCLIRQ_GetIsVIPBlankingRegion())
    {
        return 1;
    }
    else
    {
        //MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 200); // get status: FRM END
        //return Drv_SCLIRQ_GetIsBlankingRegion();
        return 0;
    }
}
MS_BOOL Drv_VIP_GetEachDMAEn(void)
{
    return Drv_SCLIRQ_GetEachDMAEn();
}
void Drv_VIP_HWInit(void)
{
    _Drv_VIPLDCInit();
    Hal_VIP_MCNRInit();
    Hal_VIP_AIPDB(0);
}
EN_VIP_SRAM_DUMP_TYPE _Drv_VIP_GetSRAMType(EN_VIP_DRV_AIP_SRAM_TYPE enAIPType)
{
    EN_VIP_SRAM_DUMP_TYPE enType;
    switch(enAIPType)
    {
        case EN_VIP_DRV_AIP_SRAM_GAMMA_Y:
            enType = EN_VIP_SRAM_DUMP_GAMMA_Y;
            break;
        case EN_VIP_DRV_AIP_SRAM_GAMMA_U:
            enType = EN_VIP_SRAM_DUMP_GAMMA_U;
            break;
        case EN_VIP_DRV_AIP_SRAM_GAMMA_V:
            enType = EN_VIP_SRAM_DUMP_GAMMA_V;
            break;
        case EN_VIP_DRV_AIP_SRAM_GM10to12_R:
            enType = EN_VIP_SRAM_DUMP_GM10to12_R;
            break;
        case EN_VIP_DRV_AIP_SRAM_GM10to12_G:
            enType = EN_VIP_SRAM_DUMP_GM10to12_G;
            break;
        case EN_VIP_DRV_AIP_SRAM_GM10to12_B:
            enType = EN_VIP_SRAM_DUMP_GM10to12_B;
            break;
        case EN_VIP_DRV_AIP_SRAM_GM12to10_R:
            enType = EN_VIP_SRAM_DUMP_GM12to10_R;
            break;
        case EN_VIP_DRV_AIP_SRAM_GM12to10_G:
            enType = EN_VIP_SRAM_DUMP_GM12to10_G;
            break;
        case EN_VIP_DRV_AIP_SRAM_GM12to10_B:
            enType = EN_VIP_SRAM_DUMP_GM12to10_B;
            break;
        case EN_VIP_DRV_AIP_SRAM_WDR:
            enType = EN_VIP_SRAM_DUMP_WDR;
            break;
        default:
            enType = EN_VIP_SRAM_DUMP_NUM;
            break;
    }
    return enType;
}
void * Drv_VIP_GeSRAMGlobal(EN_VIP_DRV_AIP_SRAM_TYPE enAIPType)
{
    return gpvSRAMBuffer[enAIPType];
}

void * Drv_VIP_SetAIPSRAMConfig(void * pvPQSetParameter, EN_VIP_DRV_AIP_SRAM_TYPE enAIPType)
{
    EN_VIP_SRAM_DUMP_TYPE enType;
    MS_BOOL bRet;
    //unsigned long u32Events;
    void * pvPQSetPara = NULL;
    enType =  _Drv_VIP_GetSRAMType(enAIPType);
    if(pvPQSetParameter == NULL)
    {
        Hal_VIP_SRAM_Dump(enType,SRAMNORMAL);
        gbSRAMCkeckPass = 1;
    }
    else
    {
        //if(!Drv_SCLIRQ_GetIsBlankingRegion())
        //{
        //    MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 200); // get status: FRM END
        //}
        bRet = Hal_VIP_SRAM_Dump(enType,pvPQSetParameter);
        pvPQSetPara = pvPQSetParameter;
        DRV_VIP_MUTEX_LOCK();
        MsOS_Memcpy(gpvSRAMBuffer[enAIPType],pvPQSetParameter,_Drv_VIP_GetSRAMBufferSize(enAIPType));
        //gpvSRAMBuffer[enAIPType] = pvPQSetParameter;
        gbSRAMCkeckPass = bRet;
        DRV_VIP_MUTEX_UNLOCK();
    }
    return pvPQSetPara;
}
MS_BOOL Drv_VIP_GetSRAMCheckPass(void)
{
    return gbSRAMCkeckPass;
}
void Drv_VIP_SRAM_Dump(void)
{
    Hal_VIP_SRAM_Dump(EN_VIP_SRAM_DUMP_IHC,SRAMNORMAL);
    Hal_VIP_SRAM_Dump(EN_VIP_SRAM_DUMP_ICC,SRAMNORMAL);
    Drv_VIP_SetAIPSRAMConfig(gpvSRAMBuffer[EN_VIP_SRAM_DUMP_WDR],EN_VIP_DRV_AIP_SRAM_WDR);
    Drv_VIP_SetAIPSRAMConfig(gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GAMMA_Y],EN_VIP_DRV_AIP_SRAM_GAMMA_Y);
    Drv_VIP_SetAIPSRAMConfig(gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GAMMA_U],EN_VIP_DRV_AIP_SRAM_GAMMA_U);
    Drv_VIP_SetAIPSRAMConfig(gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GAMMA_V],EN_VIP_DRV_AIP_SRAM_GAMMA_V);
    Drv_VIP_SetAIPSRAMConfig(gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM10to12_R],EN_VIP_DRV_AIP_SRAM_GM10to12_R);
    Drv_VIP_SetAIPSRAMConfig(gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM10to12_G],EN_VIP_DRV_AIP_SRAM_GM10to12_G);
    Drv_VIP_SetAIPSRAMConfig(gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM10to12_B],EN_VIP_DRV_AIP_SRAM_GM10to12_B);
    Drv_VIP_SetAIPSRAMConfig(gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM12to10_R],EN_VIP_DRV_AIP_SRAM_GM12to10_R);
    Drv_VIP_SetAIPSRAMConfig(gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM12to10_G],EN_VIP_DRV_AIP_SRAM_GM12to10_G);
    Drv_VIP_SetAIPSRAMConfig(gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM12to10_B],EN_VIP_DRV_AIP_SRAM_GM12to10_B);
    gbSRAMCkeckPass = 1;

}
MS_BOOL Drv_VIP_GetCMDQHWDone(void)
{
    return Drv_HVSP_GetCMDQDoneStatus(E_HVSP_POLL_ID_VIP);
}

void * Drv_VIP_GetWaitQueueHead(void)
{
    return Drv_HVSP_GetWaitQueueHead();
}

void Drv_VIP_SetMCNRIPMRead(MS_BOOL bEn)
{
    Hal_VIP_SeMCNRIPMRead(bEn);
}
void Drv_VIP_SetIPMConpress(MS_BOOL bEn)
{
    Hal_VIP_SetIPMConpress(bEn);
}
void Drv_VIP_SetCIIRRead(MS_BOOL bEn)
{
    Hal_VIP_SeCIIRRead(bEn);
}
void Drv_VIP_SetCIIRWrite(MS_BOOL bEn)
{
    Hal_VIP_SeCIIRWrite(bEn);
}
unsigned char Drv_VIP_SetNLMSRAMConfig(ST_VIP_NLM_SRAM_CONFIG *stCfg)
{
    MsOS_WaitForCPUWriteToDMem();
    if(stCfg->bCLientEn)
    {
        _Drv_VIP_SetNLMSRAMbyAutodownload(stCfg);
    }
    else
    {
        _Drv_VIP_SetNLMSRAMbyCPU(stCfg);
    }
    return TRUE;
}
MS_BOOL Drv_VIP_SetDLCHistogramConfig(ST_VIP_DLC_HISTOGRAM_CONFIG *stDLCCfg)
{
    MS_U16 u16valueForReg1Eh04;
    SCL_DBG(SCL_DBG_LV_DRVVIP(), "[DRVVIP]%s: ", __FUNCTION__);
    u16valueForReg1Eh04 = _Drv_VIP_GetDLCEnableSetting(stDLCCfg);
    if(stDLCCfg->stFCfg.bEn)
    {
        _Drv_VIP_SetDLCHistogramCMDQConfig(stDLCCfg,u16valueForReg1Eh04);
    }
    else
    {
        _Drv_VIP_SetDLCHistogramRIUConfig(stDLCCfg,u16valueForReg1Eh04);
        SCL_DBG(SCL_DBG_LV_DRVVIP(),
        "[DRVVIP]u16valueForReg1Eh04:%hx ,stDLCCfg->u32StatBase[0]:%lx,stDLCCfg->u32StatBase[1]%lx,stDLCCfg->bstat_MIU:%hhd\n"
            ,u16valueForReg1Eh04,stDLCCfg->u32StatBase[0],stDLCCfg->u32StatBase[1],stDLCCfg->bstat_MIU);
    }

    return TRUE;
}

MS_U32 Drv_VIP_GetDLCHistogramReport(MS_U16 u16range)
{
    MS_U32 u32value;
    u32value = Hal_VIP_DLC_Hist_GetRange(u16range);
    SCL_DBG(SCL_DBG_LV_DRVVIP(), "[DRVVIP]%s Histogram_%hd:%lx \n", __FUNCTION__,u16range,u32value);
    return u32value;
}
void Drv_VIP_GetDLCHistogramConfig(ST_VIP_DLC_HISTOGRAM_REPORT *stdlc)
{
    _Drv_VIP_GetDLCHistogramConfig(stdlc);
    SCL_DBG(SCL_DBG_LV_DRVVIP(), "[DRVVIP]%s PixelCount:%lx,PixelWeight:%lx,Baseidx:%hhx \n"
        ,__FUNCTION__, stdlc->u32PixelCount,stdlc->u32PixelWeight,stdlc->u8Baseidx);
}
MS_BOOL Drv_VIP_SetLDCMdConfig(ST_VIP_LDC_MD_CONFIG *stLDCCfg)
{
    SCL_DBG(SCL_DBG_LV_DRVVIP(), "[DRVVIP]%s \n", __FUNCTION__);
    MsOS_WaitForCPUWriteToDMem();
    if(stLDCCfg->stFCfg.bEn)
    {
        _Drv_VIP_SetLDCFrameBufferConfigByCMDQ(stLDCCfg);
    }
    else
    {
        _Drv_VIP_SetLDCFrameBufferConfig(stLDCCfg);
    }
    return TRUE;
}
MS_BOOL Drv_VIP_SetLDCOnConfig(MS_BOOL bEn)
{
    SCL_DBG(SCL_DBG_LV_DRVVIP(), "[DRVVIP]%s: bEn:%hhd\n", __FUNCTION__,bEn);
    MsOS_WaitForCPUWriteToDMem();
    if(!bEn)
    {
        //HW issue: 64pixel mode bypass can't use;
        Drv_VIP_SetLDCBank_Mode(EN_DRV_VIP_LDCLCBANKMODE_128);
    }
    Hal_VIP_SetLDCBypass(!bEn);
    Drv_HVSP_SetLDCONOFF(bEn);
    return TRUE;
}
MS_BOOL Drv_VIP_SetLDCDmapConfig(ST_VIP_LDC_DMAP_CONFIG *stLDCCfg)
{
    MS_U32 u32DMAP_pitch =_Drv_VIP_GetLDCDmapPitch(stLDCCfg);
    SCL_DBG(SCL_DBG_LV_DRVVIP(),
        "[DRVVIP]%s DMAP_pitch:%lx,DMAPaddr:%lx \n",__FUNCTION__,u32DMAP_pitch,stLDCCfg->u32DMAPaddr);
    if(stLDCCfg->stFCfg.bEn)
    {
        _Drv_VIP_Set_LDCDmapByCMDQ(stLDCCfg,u32DMAP_pitch);
    }
    else
    {
        _Drv_VIP_SetLDCDmap(stLDCCfg,u32DMAP_pitch);
    }
    return TRUE;
}
MS_BOOL Drv_VIP_CheckIPMResolution(void)
{
    MS_BOOL bRet = 0;
    ST_HVSP_IPM_CONFIG stInformCfg;
    MsOS_Memset(&stInformCfg,0,sizeof(ST_HVSP_IPM_CONFIG));
    Drv_HVSP_GetFrameBufferAttribute(E_HVSP_ID_1, &stInformCfg);
    if(stInformCfg.u16Fetch<=FHD_Width && stInformCfg.u16Vsize<=FHD_Width)
    {
        bRet = 1;
    }
    return bRet;
}
MS_BOOL Drv_VIP_SetLDCSRAMConfig(ST_VIP_LDC_SRAM_CONFIG *stLDCCfg)
{
    SCL_DBG(SCL_DBG_LV_DRVVIP(), "[DRVVIP]%s loadhoraddr:%lx,loadveraddr:%lx \n"
        ,__FUNCTION__,stLDCCfg->u32loadhoraddr,stLDCCfg->u32loadveraddr);
    if(stLDCCfg->stFCfg.bEn)
    {
        _Drv_VIP_SetLDCSRAMCfgByCMDQ(stLDCCfg);
    }
    else
    {
        Hal_VIP_SetLDCSramAmount(stLDCCfg->u16SRAMhoramount,stLDCCfg->u16SRAMveramount);
        Hal_VIP_SetLDCSramBase(stLDCCfg->u32loadhoraddr,stLDCCfg->u32loadveraddr);
        Hal_VIP_SetLDCSramStr(stLDCCfg->u16SRAMhorstr,stLDCCfg->u16SRAMverstr);
    }
    return TRUE;
}
MS_BOOL Drv_VIP_VtrackSetPayloadData(MS_U16 u16Timecode, MS_U8 u8OperatorID)
{
    SCL_DBG(SCL_DBG_LV_DRVVIP(), "[DRVVIP]%s \n", __FUNCTION__);
    Hal_VIP_VtrackSetPayloadData(u16Timecode, u8OperatorID);
    return 1;
}
MS_BOOL Drv_VIP_VtrackSetKey(MS_BOOL bUserDefinded, MS_U8 *pu8Setting)
{
    SCL_DBG(SCL_DBG_LV_DRVVIP(), "[DRVVIP]%s \n", __FUNCTION__);
    Hal_VIP_VtrackSetKey(bUserDefinded, pu8Setting);
    return 1;
}
MS_BOOL Drv_VIP_VtrackSetUserDefindedSetting(MS_BOOL bUserDefinded, MS_U8 *pu8Setting)
{
    SCL_DBG(SCL_DBG_LV_DRVVIP(), "[DRVVIP]%s \n", __FUNCTION__);
    Hal_VIP_VtrackSetUserDefindedSetting(bUserDefinded, pu8Setting);
    return 1;
}
MS_BOOL Drv_VIP_VtrackEnable( MS_U8 u8FrameRate, EN_VIP_DRVVTRACK_ENABLE_TYPE bEnable)
{
    SCL_DBG(SCL_DBG_LV_DRVVIP(), "[DRVVIP]%s \n", __FUNCTION__);
    Hal_VIP_VtrackEnable(u8FrameRate, (EN_VIP_VTRACK_ENABLE_TYPE)bEnable);
    return 1;
}
MS_BOOL Drv_VIP_GetBypassStatus(EN_VIP_DRV_CONFIG_TYPE enIPType)
{
    MS_BOOL bRet;
    switch(enIPType)
    {
        case EN_VIP_DRV_CONFIG:
            bRet = Hal_VIP_GetVIPBypass();
            break;

        case EN_VIP_DRV_MCNR_CONFIG:
            bRet = Hal_VIP_GetMCNRBypass();
            break;

        case EN_VIP_DRV_LDC_CONFIG:
            bRet = Hal_VIP_GetLDCBypass();
            break;

        case EN_VIP_DRV_NLM_CONFIG:
            bRet = Hal_VIP_GetNLMBypass();
            break;
        default:
            bRet = 0;
            break;
    }
    return bRet;
}
void Drv_VIP_CheckVIPSRAM(MS_U32 u32Type)
{
    MS_U32 u32Events;
    if(!Drv_SCLIRQ_GetIsBlankingRegion())
    {
        MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
    }
    DRV_VIP_MUTEX_LOCK();
    switch(u32Type)
    {
        case E_SCLIRQ_EVENT_GAMMA_Y:
            if((VIPSETRULE() == E_VIPSetRule_CMDQCheck)||(VIPSETRULE() == E_VIPSetRule_CMDQAllCheck)
                ||(VIPSETRULE() == E_VIPSetRule_CMDQAllONLYSRAMCheck))
            {
                if(Hal_VIP_GetSRAMDumpGammaYUV(EN_VIP_SRAM_DUMP_GAMMA_Y,
                gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GAMMA_Y],REG_VIP_SCNR_41_L))
                {
                    break;
                }
                else
                {

                    MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                    Hal_VIP_GetSRAMDumpGammaYUV(EN_VIP_SRAM_DUMP_GAMMA_Y,
                gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GAMMA_Y],REG_VIP_SCNR_41_L);
                    break;
                }
            }
            else
            {
                if(Hal_VIP_GetSRAMDumpGammaYUVCallback(EN_VIP_SRAM_DUMP_GAMMA_Y,
                gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GAMMA_Y],REG_VIP_SCNR_41_L))
                {
                    break;
                }
                else
                {

                    MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                    Hal_VIP_GetSRAMDumpGammaYUVCallback(EN_VIP_SRAM_DUMP_GAMMA_Y,
                gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GAMMA_Y],REG_VIP_SCNR_41_L);
                    break;
                }
            }

        case E_SCLIRQ_EVENT_GAMMA_U:
            if((VIPSETRULE() == E_VIPSetRule_CMDQCheck)||(VIPSETRULE() == E_VIPSetRule_CMDQAllCheck)
                ||(VIPSETRULE() == E_VIPSetRule_CMDQAllONLYSRAMCheck))
            {
                if(Hal_VIP_GetSRAMDumpGammaYUV(EN_VIP_SRAM_DUMP_GAMMA_U,
                gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GAMMA_U],REG_VIP_SCNR_41_L))
                {
                    break;
                }
                else
                {

                    MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                    Hal_VIP_GetSRAMDumpGammaYUV(EN_VIP_SRAM_DUMP_GAMMA_U,
                gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GAMMA_U],REG_VIP_SCNR_41_L);
                    break;
                }
            }
            else
            {
                if(Hal_VIP_GetSRAMDumpGammaYUVCallback(EN_VIP_SRAM_DUMP_GAMMA_U,
                gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GAMMA_U],REG_VIP_SCNR_41_L))
                {
                    break;
                }
                else
                {

                    MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                    Hal_VIP_GetSRAMDumpGammaYUVCallback(EN_VIP_SRAM_DUMP_GAMMA_U,
                gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GAMMA_U],REG_VIP_SCNR_41_L);
                    break;
                }
            }
        case E_SCLIRQ_EVENT_GAMMA_V:
            if((VIPSETRULE() == E_VIPSetRule_CMDQCheck)||(VIPSETRULE() == E_VIPSetRule_CMDQAllCheck)
                ||(VIPSETRULE() == E_VIPSetRule_CMDQAllONLYSRAMCheck))
            {
                if(Hal_VIP_GetSRAMDumpGammaYUV(EN_VIP_SRAM_DUMP_GAMMA_V,
                gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GAMMA_V],REG_VIP_SCNR_41_L))
                {
                    break;
                }
                else
                {

                    MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                    Hal_VIP_GetSRAMDumpGammaYUV(EN_VIP_SRAM_DUMP_GAMMA_V,
                gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GAMMA_V],REG_VIP_SCNR_41_L);
                    break;
                }
            }
            else
            {
                if(Hal_VIP_GetSRAMDumpGammaYUVCallback(EN_VIP_SRAM_DUMP_GAMMA_V,
                gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GAMMA_V],REG_VIP_SCNR_41_L))
                {
                    break;
                }
                else
                {

                    MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                    Hal_VIP_GetSRAMDumpGammaYUVCallback(EN_VIP_SRAM_DUMP_GAMMA_V,
                gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GAMMA_V],REG_VIP_SCNR_41_L);
                    break;
                }
            }

        case E_SCLIRQ_EVENT_GM10to12_R:
            if((VIPSETRULE() == E_VIPSetRule_CMDQCheck)||(VIPSETRULE() == E_VIPSetRule_CMDQAllCheck)
                ||(VIPSETRULE() == E_VIPSetRule_CMDQAllONLYSRAMCheck))
            {
                if(Hal_VIP_GetSRAMDumpGammaRGB(EN_VIP_SRAM_DUMP_GM10to12_R,
                    gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM10to12_R],REG_SCL_HVSP1_7A_L))
                {
                    break;
                }
                else
                {

                    MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                    Hal_VIP_GetSRAMDumpGammaRGB(EN_VIP_SRAM_DUMP_GM10to12_R,
                        gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM10to12_R],REG_SCL_HVSP1_7A_L);
                    break;
                }
            }
            else
            {
                if(Hal_VIP_GetSRAMDumpGammaRGBCallback(EN_VIP_SRAM_DUMP_GM10to12_R,
                    gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM10to12_R],REG_SCL_HVSP1_7A_L))
                {
                    break;
                }
                else
                {
                    do
                    {
                        MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                    }
                    while(!Hal_VIP_GetSRAMDumpGammaRGBCallback(EN_VIP_SRAM_DUMP_GM10to12_R,
                        gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM10to12_R],REG_SCL_HVSP1_7A_L));
                    break;
                }
            }

        case E_SCLIRQ_EVENT_GM10to12_G:
            if((VIPSETRULE() == E_VIPSetRule_CMDQCheck)||(VIPSETRULE() == E_VIPSetRule_CMDQAllCheck)
                ||(VIPSETRULE() == E_VIPSetRule_CMDQAllONLYSRAMCheck))
            {
                if(Hal_VIP_GetSRAMDumpGammaRGB(EN_VIP_SRAM_DUMP_GM10to12_G,
                    gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM10to12_G],REG_SCL_HVSP1_7A_L))
                {
                    break;
                }
                else
                {

                    MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                    Hal_VIP_GetSRAMDumpGammaRGB(EN_VIP_SRAM_DUMP_GM10to12_G,
                        gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM10to12_G],REG_SCL_HVSP1_7A_L);
                    break;
                }
            }
            else
            {
                if(Hal_VIP_GetSRAMDumpGammaRGBCallback(EN_VIP_SRAM_DUMP_GM10to12_G,
                    gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM10to12_G],REG_SCL_HVSP1_7A_L))
                {
                    break;
                }
                else
                {
                    do
                    {
                        MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                    }
                    while(!Hal_VIP_GetSRAMDumpGammaRGBCallback(EN_VIP_SRAM_DUMP_GM10to12_G,
                        gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM10to12_G],REG_SCL_HVSP1_7A_L));
                    break;
                }
            }

        case E_SCLIRQ_EVENT_GM10to12_B:
            if((VIPSETRULE() == E_VIPSetRule_CMDQCheck)||(VIPSETRULE() == E_VIPSetRule_CMDQAllCheck)
                ||(VIPSETRULE() == E_VIPSetRule_CMDQAllONLYSRAMCheck))
            {
                if(Hal_VIP_GetSRAMDumpGammaRGB(EN_VIP_SRAM_DUMP_GM10to12_B,
                    gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM10to12_B],REG_SCL_HVSP1_7A_L))
                {
                    break;
                }
                else
                {

                    MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                    Hal_VIP_GetSRAMDumpGammaRGB(EN_VIP_SRAM_DUMP_GM10to12_B,
                        gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM10to12_B],REG_SCL_HVSP1_7A_L);
                    break;
                }
            }
            else
            {
                if(Hal_VIP_GetSRAMDumpGammaRGBCallback(EN_VIP_SRAM_DUMP_GM10to12_B,
                    gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM10to12_B],REG_SCL_HVSP1_7A_L))
                {
                    break;
                }
                else
                {
                    do
                    {
                        MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                    }
                    while(!Hal_VIP_GetSRAMDumpGammaRGBCallback(EN_VIP_SRAM_DUMP_GM10to12_B,
                        gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM10to12_B],REG_SCL_HVSP1_7A_L));
                    break;
                }
            }

        case E_SCLIRQ_EVENT_GM12to10_R:
            if((VIPSETRULE() == E_VIPSetRule_CMDQCheck)||(VIPSETRULE() == E_VIPSetRule_CMDQAllCheck)
                ||(VIPSETRULE() == E_VIPSetRule_CMDQAllONLYSRAMCheck))
            {
                if(Hal_VIP_GetSRAMDumpGammaRGB(EN_VIP_SRAM_DUMP_GM12to10_R,
                    gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM12to10_R],REG_SCL_HVSP1_7D_L))
                {
                    break;
                }
                else
                {

                    MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                    Hal_VIP_GetSRAMDumpGammaRGB(EN_VIP_SRAM_DUMP_GM12to10_R,
                        gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM12to10_R],REG_SCL_HVSP1_7D_L);
                    break;
                }
            }
            else
            {
                if(Hal_VIP_GetSRAMDumpGammaRGBCallback(EN_VIP_SRAM_DUMP_GM12to10_R,
                    gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM12to10_R],REG_SCL_HVSP1_7D_L))
                {
                    break;
                }
                else
                {
                    do
                    {
                        MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                    }
                    while(!Hal_VIP_GetSRAMDumpGammaRGBCallback(EN_VIP_SRAM_DUMP_GM12to10_R,
                        gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM12to10_R],REG_SCL_HVSP1_7D_L));
                    break;
                }
            }

        case E_SCLIRQ_EVENT_GM12to10_G:
            if((VIPSETRULE() == E_VIPSetRule_CMDQCheck)||(VIPSETRULE() == E_VIPSetRule_CMDQAllCheck)
                ||(VIPSETRULE() == E_VIPSetRule_CMDQAllONLYSRAMCheck))
            {
                if(Hal_VIP_GetSRAMDumpGammaRGB(EN_VIP_SRAM_DUMP_GM12to10_G,
                    gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM12to10_G],REG_SCL_HVSP1_7D_L))
                {
                    break;
                }
                else
                {

                    MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                    Hal_VIP_GetSRAMDumpGammaRGB(EN_VIP_SRAM_DUMP_GM12to10_G,
                        gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM12to10_G],REG_SCL_HVSP1_7D_L);
                    break;
                }
            }
            else
            {
                if(Hal_VIP_GetSRAMDumpGammaRGBCallback(EN_VIP_SRAM_DUMP_GM12to10_G,
                    gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM12to10_G],REG_SCL_HVSP1_7D_L))
                {
                    break;
                }
                else
                {
                    do
                    {
                        MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                    }
                    while(!Hal_VIP_GetSRAMDumpGammaRGBCallback(EN_VIP_SRAM_DUMP_GM12to10_G,
                        gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM12to10_G],REG_SCL_HVSP1_7D_L));
                    break;
                }
            }

        case E_SCLIRQ_EVENT_GM12to10_B:
            if((VIPSETRULE() == E_VIPSetRule_CMDQCheck)||(VIPSETRULE() == E_VIPSetRule_CMDQAllCheck)
                ||(VIPSETRULE() == E_VIPSetRule_CMDQAllONLYSRAMCheck))
            {
                if(Hal_VIP_GetSRAMDumpGammaRGB(EN_VIP_SRAM_DUMP_GM12to10_B,
                    gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM12to10_B],REG_SCL_HVSP1_7D_L))
                {
                    break;
                }
                else
                {

                    MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                    Hal_VIP_GetSRAMDumpGammaRGB(EN_VIP_SRAM_DUMP_GM12to10_B,
                        gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM12to10_B],REG_SCL_HVSP1_7D_L);
                    break;
                }
            }
            else
            {
                if(Hal_VIP_GetSRAMDumpGammaRGBCallback(EN_VIP_SRAM_DUMP_GM12to10_B,
                    gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM12to10_B],REG_SCL_HVSP1_7D_L))
                {
                    break;
                }
                else
                {
                    do
                    {
                        MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                    }
                    while(!Hal_VIP_GetSRAMDumpGammaRGBCallback(EN_VIP_SRAM_DUMP_GM12to10_B,
                        gpvSRAMBuffer[EN_VIP_DRV_AIP_SRAM_GM12to10_B],REG_SCL_HVSP1_7D_L));
                    break;
                }
            }

        default:
            break;
    }
    DRV_VIP_MUTEX_UNLOCK();
}

#undef DRV_VIP_C
