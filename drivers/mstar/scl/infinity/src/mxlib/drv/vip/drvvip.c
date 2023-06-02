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
#include <linux/wait.h>
#include <linux/irqreturn.h>
#include <asm/div64.h>
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

//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define DRV_VIP_DBG 0
#define DRV_VIP_LDC_DMAP_align(x,align) ((x+align) & ~(align-1))
#define DMAPBLOCKUNIT       32
#define DMAPBLOCKUNITBYTE   4
#define DMAPBLOCKALIGN      4
#define DEFAULTLDCMD 0x1
#define SRAMNORMAL 0
#define SRAMFORSCALINGUP 1

#define DLCVariableSection 8
#define DLCCurveFitEnable 7
#define DLCCurveFitRGBEnable 13
#define DLCDitherEnable 5
#define DLCHistYRGBEnable 10
#define DLCStaticEnable 1
#define LDCFBRWDiff 4
#define LDCSWModeEnable 10
#define LDCAppointFBidx 2


//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------
MS_S32 _VIP_Mutex = -1;
//-------------------------------------------------------------------------------------------------
//  Loacl Functions
//-------------------------------------------------------------------------------------------------
MS_U16 _Drv_VIP_GetDLCEnableSetting(ST_VIP_DLC_HISTOGRAM_CONFIG stDLCCfg)
{
    MS_U16 u16valueForReg1Eh04;
    u16valueForReg1Eh04 = ((MS_U16)stDLCCfg.bVariable_Section<<DLCVariableSection)|
        ((MS_U16)stDLCCfg.bcurve_fit_en<<DLCCurveFitEnable)|
        ((MS_U16)stDLCCfg.bcurve_fit_rgb_en<<DLCCurveFitRGBEnable)|
        ((MS_U16)stDLCCfg.bDLCdither_en<<DLCDitherEnable)|
        ((MS_U16)stDLCCfg.bhis_y_rgb_mode_en<<DLCHistYRGBEnable)|
        ((MS_U16)stDLCCfg.bstatic<<DLCStaticEnable);
    return u16valueForReg1Eh04;
}
void _Drv_VIP_For_SetDLCHistRangeEachSection(ST_VIP_DLC_HISTOGRAM_CONFIG stDLCCfg)
{
    MS_U16 u16sec;
    MS_U16 u16tvalue;
    MS_U16 u16Mask;
    MS_U32 u32Reg;
    if(stDLCCfg.stFCfg.bEn)
    {
        for(u16sec=0;u16sec<VIP_DLC_HISTOGRAM_SECTION_NUM;u16sec++)
        {
            u32Reg = REG_VIP_DLC_0C_L+(((u16sec)/2)*2);
            if((u16sec%2) == 0)
            {
                u16Mask     = 0x00FF;
                u16tvalue   = ((MS_U16)stDLCCfg.u8Histogram_Range[u16sec]);

            }
            else
            {
                u16Mask     = 0xFF00;
                u16tvalue   = ((MS_U16)stDLCCfg.u8Histogram_Range[u16sec])<<8;
            }

            Drv_CMDQ_AssignFrameWriteCmd(u32Reg,(u16tvalue),u16Mask,stDLCCfg.stFCfg.u8framecnt);
        }
    }
    else
    {
        for(u16sec=0;u16sec<VIP_DLC_HISTOGRAM_SECTION_NUM;u16sec++)
        {
            Hal_VIP_DLCHistSetRange(stDLCCfg.u8Histogram_Range[u16sec],u16sec+1);
        }
    }
}
void _Drv_VIP_SetDLCHistogramCMDQConfig
    (ST_VIP_DLC_HISTOGRAM_CONFIG stDLCCfg,MS_U16 u16valueForReg1Eh04)
{
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_VIP_DLC_04_L,u16valueForReg1Eh04,0x25a2,stDLCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_VIP_DLC_03_L,stDLCCfg.u8HistSft,0x0007,stDLCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_VIP_MWE_15_L,stDLCCfg.bstat_MIU,0x0001,stDLCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_VIP_MWE_1C_L,stDLCCfg.u8trig_ref_mode<<2,0x0004,stDLCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_VIP_MWE_18_L,(MS_U16)stDLCCfg.u32StatBase[0]>>4,0xFFFF,stDLCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_VIP_MWE_19_L,(MS_U16)stDLCCfg.u32StatBase[0]>>20,0x07FF,stDLCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_VIP_MWE_1A_L,(MS_U16)stDLCCfg.u32StatBase[1]>>4,0xFFFF,stDLCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_VIP_MWE_1B_L,(MS_U16)stDLCCfg.u32StatBase[1]>>20,0x07FF,stDLCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_VIP_DLC_08_L,stDLCCfg.bRange<<7,0x0080,stDLCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_VIP_MWE_01_L,stDLCCfg.u16Vst,0x03FF,stDLCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_VIP_MWE_02_L,stDLCCfg.u16Vnd,0x03FF,stDLCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_VIP_MWE_03_L,stDLCCfg.u16Hst,0x01FF,stDLCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_VIP_MWE_04_L,stDLCCfg.u16Vnd,0x01FF,stDLCCfg.stFCfg.u8framecnt);

    _Drv_VIP_For_SetDLCHistRangeEachSection(stDLCCfg);
    Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,1);
}
void _Drv_VIP_SetDLCHistogramRIUConfig(ST_VIP_DLC_HISTOGRAM_CONFIG stDLCCfg,MS_U16 u16valueForReg1Eh04)
{
    Hal_VIP_DLCHistVarOnOff(u16valueForReg1Eh04);
    Hal_VIP_SetDLCstatMIU(stDLCCfg.bstat_MIU,stDLCCfg.u32StatBase[0],stDLCCfg.u32StatBase[1]);
    Hal_VIP_SetDLCshift(stDLCCfg.u8HistSft);
    HAl_VIP_SetDLCmode(stDLCCfg.u8trig_ref_mode);
    Hal_VIP_SetDLCActWin(stDLCCfg.bRange,stDLCCfg.u16Vst,stDLCCfg.u16Hst,stDLCCfg.u16Vnd,stDLCCfg.u16Hnd);
    _Drv_VIP_For_SetDLCHistRangeEachSection(stDLCCfg);
}
ST_VIP_DLC_HISTOGRAM_REPORT _Drv_VIP_GetDLCHistogramConfig(void)
{
    ST_VIP_DLC_HISTOGRAM_REPORT stdlc;
    stdlc.u32PixelCount  = Hal_VIP_DLCGetPC();
    stdlc.u32PixelWeight = Hal_VIP_DLCGetPW();
    stdlc.u8Baseidx      = Hal_VIP_DLCGetBaseidx();
    stdlc.u8MaxPixel     = Hal_VIP_DLCGetMaxP();
    stdlc.u8MinPixel     = Hal_VIP_DLCGetMinP();
    return stdlc;
}
MS_U8 _Drv_VIP_GetLDCFrameBufferSetting(ST_VIP_LDC_MD_CONFIG stLDCCfg)
{
    return (MS_U8)((stLDCCfg.u8FBrwdiff<<LDCFBRWDiff)|
        (stLDCCfg.bEnSWMode<<LDCSWModeEnable)|
        (stLDCCfg.u8FBidx<<LDCAppointFBidx));
}
MS_BOOL _Drv_VIP_SetLDCFrameBufferConfig(ST_VIP_LDC_MD_CONFIG stLDCCfg)
{
    MS_U8 u8LDCFBSetting;
    u8LDCFBSetting = _Drv_VIP_GetLDCFrameBufferSetting(stLDCCfg);
    Hal_VIP_SetLDCFrameBufferSetting(u8LDCFBSetting);
    Hal_VIP_SetLDCBypass(stLDCCfg.enbypass);
    return TRUE;
}
MS_BOOL _Drv_VIP_SetLDCFrameBufferConfigByCMDQ(ST_VIP_LDC_MD_CONFIG stLDCCfg)
{
    MS_U8 u8LDCFBSetting;
    u8LDCFBSetting = _Drv_VIP_GetLDCFrameBufferSetting(stLDCCfg);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_SCL_LDC_09_L, u8LDCFBSetting,BIT4|BIT3|BIT2|BIT5|BIT8,stLDCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_SCL_LDC_0F_L,((MS_U16)stLDCCfg.enbypass),0x0001,stLDCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,1);
    return TRUE;
}
void _Drv_VIP_SetNLMSRAMbyAutodownload(ST_VIP_NLM_SRAM_CONFIG stCfg)
{
    Hal_VIP_SetAutodownloadAddr(stCfg.u32baseadr,stCfg.u16iniaddr,VIP_NLM_AUTODOWNLOAD_CLIENT);
    Hal_VIP_SetAutodownloadReq(stCfg.u16depth,stCfg.u16reqlen,VIP_NLM_AUTODOWNLOAD_CLIENT);
    Hal_VIP_SetAutodownload(stCfg.bCLientEn,stCfg.btrigContinue,VIP_NLM_AUTODOWNLOAD_CLIENT);
#if DRV_VIP_DBG
    MS_U16 u16entry;
    for(u16entry=0;u16entry<VIP_NLM_ENTRY_NUM;u16entry++)
    {
        Hal_VIP_GetNLMSRAM(u16entry);
    }
#endif
}
void _Drv_VIP_SetNLMSRAMbyCPU(ST_VIP_NLM_SRAM_CONFIG stCfg)
{
    MS_U16 u16entry;
    MS_U32 u32value,u32addr;
    MS_U32 *pu32Addr = NULL;
    for(u16entry = 0;u16entry<VIP_NLM_ENTRY_NUM;u16entry++)
    {
        u32addr  = stCfg.u32viradr + u16entry * VIP_NLM_AUTODOWNLOAD_BASE_UNIT ;// 1entry cost 16 byte(128 bit)
        pu32Addr = (MS_U32 *)(u32addr);
        u32value = *pu32Addr;
        Hal_VIP_SetNLMSRAMbyCPU(u16entry,u32value);
#if DRV_VIP_DBG
        Hal_VIP_GetNLMSRAM(u16entry);
#endif
    }
}
MS_U32 _Drv_VIP_GetLDCDmapPitch(ST_VIP_LDC_DMAP_CONFIG stLDCCfg)
{
    MS_U32 u32DMAP_pitch;
    u32DMAP_pitch = ((stLDCCfg.u16DMAPWidth/DMAPBLOCKUNIT)+1);
    u32DMAP_pitch = DRV_VIP_LDC_DMAP_align(u32DMAP_pitch,DMAPBLOCKALIGN);
    u32DMAP_pitch = u32DMAP_pitch * DMAPBLOCKUNITBYTE;
    return u32DMAP_pitch;
}
void _Drv_VIP_Set_LDCDmapByCMDQ(ST_VIP_LDC_DMAP_CONFIG stLDCCfg,MS_U32 u32DMAP_pitch)
{
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_SCL_LDC_0A_L,((MS_U16)stLDCCfg.u32DMAPaddr),0xFFFF,stLDCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_SCL_LDC_0B_L,((MS_U16)(stLDCCfg.u32DMAPaddr>>16)),0xFFFF,stLDCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_SCL_LDC_0C_L,((MS_U16)u32DMAP_pitch),0xFFFF,stLDCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_SCL_LDC_0E_L,((MS_U16)stLDCCfg.bEnPowerSave)<<6|
        ((MS_U16)stLDCCfg.u8DMAPoffset),0x007F,stLDCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,1);
}
void _Drv_VIP_SetLDCDmap(ST_VIP_LDC_DMAP_CONFIG stLDCCfg,MS_U32 u32DMAP_pitch)
{
    Hal_VIP_SetLDCDmapBase(stLDCCfg.u32DMAPaddr);
    Hal_VIP_SetLDCDmapPitch(u32DMAP_pitch);
    Hal_VIP_SetLDCDmapOffset(stLDCCfg.u8DMAPoffset);
    Hal_VIP_SetLDCDmapPS(stLDCCfg.bEnPowerSave);
}
void _Drv_VIP_SetLDCSRAMCfgByCMDQ(ST_VIP_LDC_SRAM_CONFIG stLDCCfg)
{
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_SCL_LDC_22_L,((MS_U16)stLDCCfg.u16SRAMhoramount),0xFFFF,stLDCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_SCL_LDC_2A_L,((MS_U16)stLDCCfg.u16SRAMveramount),0xFFFF,stLDCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_SCL_LDC_24_L,((MS_U16)(stLDCCfg.u32loadhoraddr>>4)),0xFFFF,stLDCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_SCL_LDC_25_L,((MS_U16)(stLDCCfg.u32loadhoraddr>>20)),0xFFFF,stLDCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_SCL_LDC_2C_L,((MS_U16)(stLDCCfg.u32loadveraddr>>4)),0xFFFF,stLDCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_SCL_LDC_2D_L,((MS_U16)(stLDCCfg.u32loadveraddr>>20)),0xFFFF,stLDCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_SCL_LDC_23_L,((MS_U16)stLDCCfg.u16SRAMhorstr),0xFFFF,stLDCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_AssignFrameWriteCmd
        (REG_SCL_LDC_2B_L,((MS_U16)stLDCCfg.u16SRAMverstr),0xFFFF,stLDCCfg.stFCfg.u8framecnt);
    Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,1);
}
//-------------------------------------------------------------------------------------------------
//  Functions
//-------------------------------------------------------------------------------------------------
MS_BOOL Drv_VIP_Init(ST_VIP_INIT_CONFIG *pCfg)
{
    char word[] = {"_VIP_Mutex"};

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


    return TRUE;
}
void Drv_VIPLDCInit(void)
{
    SCL_DBG(SCL_DBG_LV_DRVVIP(), "[DRVVIP]%s: ", __FUNCTION__);
    Hal_VIP_SetLDC422_444_allMd(DEFAULTLDCMD);
}
void Drv_VIP_SRAM_Dump(void)
{
    Hal_VIP_SRAM_Dump(EN_VIP_SRAM_DUMP_IHC,SRAMNORMAL);
    Hal_VIP_SRAM_Dump(EN_VIP_SRAM_DUMP_ICC,SRAMNORMAL);
    Hal_VIP_SRAM_Dump(EN_VIP_SRAM_DUMP_HVSP,SRAMFORSCALINGUP);
    Hal_VIP_SRAM_Dump(EN_VIP_SRAM_DUMP_HVSP_1,SRAMNORMAL);
    Hal_VIP_SRAM_Dump(EN_VIP_SRAM_DUMP_HVSP_2,SRAMNORMAL);
}
MS_BOOL Drv_VIP_GetCMDQHWDone(void)
{
    return Drv_HVSP_GetCMDQDoneStatus();
}

wait_queue_head_t * Drv_VIP_GetWaitQueueHead(void)
{
    return Drv_HVSP_GetWaitQueueHead();
}

void Drv_VIP_SetDNRIPMRead(MS_BOOL bEn)
{
    Hal_VIP_SetDNRIPMRead(bEn);
}
unsigned char Drv_VIP_SetNLMSRAMConfig(ST_VIP_NLM_SRAM_CONFIG stCfg)
{
    if(stCfg.bCLientEn)
    {
        _Drv_VIP_SetNLMSRAMbyAutodownload(stCfg);
    }
    else
    {
        _Drv_VIP_SetNLMSRAMbyCPU(stCfg);
    }
    return TRUE;
}
MS_BOOL Drv_VIP_SetDLCHistogramConfig(ST_VIP_DLC_HISTOGRAM_CONFIG stDLCCfg)
{
    MS_U16 u16valueForReg1Eh04;
    SCL_DBG(SCL_DBG_LV_DRVVIP(), "[DRVVIP]%s: ", __FUNCTION__);
    u16valueForReg1Eh04 = _Drv_VIP_GetDLCEnableSetting(stDLCCfg);
    if(stDLCCfg.stFCfg.bEn)
    {
        _Drv_VIP_SetDLCHistogramCMDQConfig(stDLCCfg,u16valueForReg1Eh04);
    }
    else
    {
        _Drv_VIP_SetDLCHistogramRIUConfig(stDLCCfg,u16valueForReg1Eh04);
        SCL_DBG(SCL_DBG_LV_DRVVIP(),
        "[DRVVIP]u16valueForReg1Eh04:%hx ,stDLCCfg.u32StatBase[0]:%lx,stDLCCfg.u32StatBase[1]%lx,stDLCCfg.bstat_MIU:%hhd\n"
            ,u16valueForReg1Eh04,stDLCCfg.u32StatBase[0],stDLCCfg.u32StatBase[1],stDLCCfg.bstat_MIU);
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
ST_VIP_DLC_HISTOGRAM_REPORT Drv_VIP_GetDLCHistogramConfig(void)
{
    ST_VIP_DLC_HISTOGRAM_REPORT stdlc;
    stdlc = _Drv_VIP_GetDLCHistogramConfig();
    SCL_DBG(SCL_DBG_LV_DRVVIP(), "[DRVVIP]%s PixelCount:%lx,PixelWeight:%lx,Baseidx:%hhx \n"
        ,__FUNCTION__, stdlc.u32PixelCount,stdlc.u32PixelWeight,stdlc.u8Baseidx);
    return stdlc;
}
MS_BOOL Drv_VIP_SetLDCMdConfig(ST_VIP_LDC_MD_CONFIG stLDCCfg)
{
    SCL_DBG(SCL_DBG_LV_DRVVIP(), "[DRVVIP]%s \n", __FUNCTION__);
    if(stLDCCfg.stFCfg.bEn)
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
    Hal_VIP_SetLDCBypass(!bEn);
    Drv_HVSP_SetLDCONOFF(bEn);
    return TRUE;
}
MS_BOOL Drv_VIP_SetLDCDmapConfig(ST_VIP_LDC_DMAP_CONFIG stLDCCfg)
{
    MS_U32 u32DMAP_pitch =_Drv_VIP_GetLDCDmapPitch(stLDCCfg);
    SCL_DBG(SCL_DBG_LV_DRVVIP(),
        "[DRVVIP]%s DMAP_pitch:%lx,DMAPaddr:%lx \n",__FUNCTION__,u32DMAP_pitch,stLDCCfg.u32DMAPaddr);
    if(stLDCCfg.stFCfg.bEn)
    {
        _Drv_VIP_Set_LDCDmapByCMDQ(stLDCCfg,u32DMAP_pitch);
    }
    else
    {
        _Drv_VIP_SetLDCDmap(stLDCCfg,u32DMAP_pitch);
    }
    return TRUE;
}
MS_BOOL Drv_VIP_SetLDCSRAMConfig(ST_VIP_LDC_SRAM_CONFIG stLDCCfg)
{
    SCL_DBG(SCL_DBG_LV_DRVVIP(), "[DRVVIP]%s loadhoraddr:%lx,loadveraddr:%lx \n"
        ,__FUNCTION__,stLDCCfg.u32loadhoraddr,stLDCCfg.u32loadveraddr);
    if(stLDCCfg.stFCfg.bEn)
    {
        _Drv_VIP_SetLDCSRAMCfgByCMDQ(stLDCCfg);
    }
    else
    {
        Hal_VIP_SetLDCSramAmount(stLDCCfg.u16SRAMhoramount,stLDCCfg.u16SRAMveramount);
        Hal_VIP_SetLDCSramBase(stLDCCfg.u32loadhoraddr,stLDCCfg.u32loadveraddr);
        Hal_VIP_SetLDCSramStr(stLDCCfg.u16SRAMhorstr,stLDCCfg.u16SRAMverstr);
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
    Hal_VIP_VtrackEnable(u8FrameRate, bEnable);
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

        case EN_VIP_DRV_DNR_CONFIG:
            bRet = Hal_VIP_GetDNRBypass();
            break;

        case EN_VIP_DRV_SNR_CONFIG:
            bRet = Hal_VIP_GetSNRBypass();
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
#undef DRV_VIP_C
