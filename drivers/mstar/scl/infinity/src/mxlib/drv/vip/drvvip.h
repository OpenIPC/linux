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
// (!¡±MStar Confidential Information!¡L) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////
//==============================================================================
#ifndef _DRV_VIP_H
#define _DRV_VIP_H
//=============================================================================
// Defines
//=============================================================================
//
#define VIP_DLC_HISTOGRAM_SECTION_NUM   7
#define VIP_DLC_HISTOGRAM_REPORT_NUM    8
#define VIP_NLM_ENTRY_NUM 1104
#define VIP_NLM_AUTODOWNLOAD_BASE_UNIT 16
#define VIP_NLM_AUTODOWNLOAD_CLIENT 9
//
typedef enum
{
    E_VIP_LDC_Drv_MENULOAD,
    E_VIP_LDC_Drv_BYPASS,
    E_VIP_LDC_BYPASS_Drv_TYPE_NUM,
}EN_VIP_LDC_BYPASS_Drv_TYPE;
//
typedef enum
{
    EN_VIP_DRVVTRACK_ENABLE_ON,
    EN_VIP_DRVVTRACK_ENABLE_OFF,
    EN_VIP_DRVVTRACK_ENABLE_DEBUG,
}EN_VIP_DRVVTRACK_ENABLE_TYPE;
typedef enum
{
    EN_VIP_DRV_ACK_CONFIG           = 0x1,      ///< ACK
    EN_VIP_DRV_IBC_CONFIG           = 0x2,      ///< IBC
    EN_VIP_DRV_IHCICC_CONFIG        = 0x4,      ///< ICCIHC
    EN_VIP_DRV_ICC_CONFIG           = 0x8,      ///< ICE
    EN_VIP_DRV_IHC_CONFIG           = 0x10,     ///< IHC
    EN_VIP_DRV_FCC_CONFIG           = 0x20,     ///< FCC
    EN_VIP_DRV_UVC_CONFIG           = 0x40,     ///< UVC
    EN_VIP_DRV_DLC_CONFIG           = 0x80,     ///< DLC
    EN_VIP_DRV_DLC_HISTOGRAM_CONFIG = 0x100,    ///< HIST
    EN_VIP_DRV_LCE_CONFIG           = 0x200,    ///< LCE
    EN_VIP_DRV_PEAKING_CONFIG       = 0x400,    ///< PK
    EN_VIP_DRV_NLM_CONFIG           = 0x800,    ///< NLM
    EN_VIP_DRV_LDC_MD_CONFIG        = 0x1000,   ///< LDCMD
    EN_VIP_DRV_LDC_DMAP_CONFIG      = 0x2000,   ///< LDCDMAP
    EN_VIP_DRV_LDC_SRAM_CONFIG      = 0x4000,   ///< LDC SRAM
    EN_VIP_DRV_LDC_CONFIG           = 0x8000,   ///< LDC
    EN_VIP_DRV_DNR_CONFIG           = 0x10000,  ///< DNR
    EN_VIP_DRV_SNR_CONFIG           = 0x20000,  ///< SNR
    EN_VIP_DRV_CONFIG               = 0x40000,  ///< 19 bit to control 19 IOCTL
}EN_VIP_DRV_CONFIG_TYPE;

//-------------------------------------------------------------------------------------------------
//  Defines & Structure
//-------------------------------------------------------------------------------------------------
// IOCTL_VIP_SET_DNR_CONFIG
typedef struct
{
    unsigned char bEn;
    unsigned char u8framecnt;
}ST_VIP_FC_CONFIG;
typedef struct
{
    MS_U32 u32RiuBase;
}ST_VIP_INIT_CONFIG;
typedef struct
{
    MS_U32 u32RiuBase;
    MS_U32 u32CMDQ_Phy;
    MS_U32 u32CMDQ_Size;
    MS_U32 u32CMDQ_Vir;
}ST_VIP_OPEN_CONFIG;

typedef struct
{
    MS_U32  u32baseadr;
    MS_U16  u16depth;
    MS_U16  u16reqlen;
    MS_BOOL bCLientEn;
    MS_BOOL btrigContinue;
    MS_U16  u16iniaddr;
    MS_U32  u32viradr;
}ST_VIP_NLM_SRAM_CONFIG;


typedef struct
{
    ST_VIP_FC_CONFIG stFCfg;
    unsigned char u8FBidx;
    unsigned char u8FBrwdiff;
    unsigned char bEnSWMode;
    EN_VIP_LDC_BYPASS_Drv_TYPE enbypass;
}ST_VIP_LDC_MD_CONFIG;

typedef struct
{
    ST_VIP_FC_CONFIG stFCfg;
    unsigned long u32DMAPaddr;
    unsigned short u16DMAPWidth;
    unsigned char u8DMAPoffset;
    unsigned char bEnPowerSave;
}ST_VIP_LDC_DMAP_CONFIG;

typedef struct
{
    ST_VIP_FC_CONFIG stFCfg;
    unsigned long u32loadhoraddr;
    unsigned short u16SRAMhorstr;
    unsigned short u16SRAMhoramount;
    unsigned long u32loadveraddr;
    unsigned short u16SRAMverstr;
    unsigned short u16SRAMveramount;
}ST_VIP_LDC_SRAM_CONFIG;

typedef struct
{
    ST_VIP_FC_CONFIG stFCfg;
    unsigned char bVariable_Section;//1E04
    unsigned char bstatic; //1E04
    unsigned char bcurve_fit_en;//1E04
    unsigned char bhis_y_rgb_mode_en;//1E04
    unsigned char bcurve_fit_rgb_en;//1E04
    unsigned char bDLCdither_en;//1E04
    unsigned char u8Histogram_Range[VIP_DLC_HISTOGRAM_SECTION_NUM];
    unsigned char bstat_MIU;
    unsigned char bRange;
    unsigned short u16Vst;
    unsigned short u16Vnd;
    unsigned short u16Hst;
    unsigned short u16Hnd;
    unsigned char u8HistSft;
    unsigned char u8trig_ref_mode;
    unsigned long u32StatBase[2];
}ST_VIP_DLC_HISTOGRAM_CONFIG;

//IOCTL_VIP_GET_DLC_HISTOGRAM_REPORT
typedef struct
{
    unsigned int u32Histogram[VIP_DLC_HISTOGRAM_REPORT_NUM];
    unsigned long u32PixelWeight;
    unsigned long u32PixelCount;
    unsigned char u8MinPixel;
    unsigned char u8MaxPixel;
    unsigned char u8Baseidx;
}ST_VIP_DLC_HISTOGRAM_REPORT;

//IOCTL_VIP_SET_FCC_ADP_YWIN_CONFIG


typedef struct
{
    ST_VIP_FC_CONFIG stFCfg;
    unsigned long u32Addr;
    unsigned short u16Data;
    unsigned short u16Mask;
    unsigned char u8framecnt;
    unsigned char bfire;
    unsigned char bCnt;
}ST_VIP_CMDQ_CONFIG;


//-------------------------------------------------------------------------------------------------
//  Prototype
//-------------------------------------------------------------------------------------------------
#ifndef DRV_VIP_C
#define INTERFACE extern
#else
#define INTERFACE
#endif

INTERFACE MS_BOOL Drv_VIP_SetLDCOnConfig(MS_BOOL bEn);
INTERFACE ST_VIP_DLC_HISTOGRAM_REPORT Drv_VIP_GetDLCHistogramConfig(void);
INTERFACE MS_BOOL Drv_VIP_Init(ST_VIP_INIT_CONFIG *pCfg);
INTERFACE MS_BOOL Drv_VIP_SetDLCHistogramConfig(ST_VIP_DLC_HISTOGRAM_CONFIG stDLCCfg);
INTERFACE MS_U32 Drv_VIP_GetDLCHistogramReport(MS_U16 u16range);
INTERFACE MS_BOOL Drv_VIP_SetLDCMdConfig(ST_VIP_LDC_MD_CONFIG stLDCCfg);
INTERFACE MS_BOOL Drv_VIP_SetLDCDmapConfig(ST_VIP_LDC_DMAP_CONFIG stLDCCfg);
INTERFACE MS_BOOL Drv_VIP_SetLDCSRAMConfig(ST_VIP_LDC_SRAM_CONFIG stLDCCfg);
INTERFACE void    Drv_VIPLDCInit(void);
INTERFACE void Drv_VIP_DNR_Init(void);
INTERFACE void Drv_VIP_PK_Init(void);
INTERFACE void Drv_VIP_DLC_Init(void);
INTERFACE void Drv_VIP_UVC_Init(void);
INTERFACE void Drv_VIP_ICE_Init(void);
INTERFACE void Drv_VIP_IHC_Init(void);
INTERFACE void Drv_VIP_FCC_Init(void);
INTERFACE void Drv_VIP_LCE_Init(void);
INTERFACE unsigned char Drv_VIP_SetNLMSRAMConfig(ST_VIP_NLM_SRAM_CONFIG stCfg);
INTERFACE void Drv_VIP_SRAM_Dump(void);
INTERFACE void Drv_VIP_SetDNRIPMRead(MS_BOOL bEn);
INTERFACE MS_BOOL Drv_VIP_VtrackSetPayloadData(MS_U16 u16Timecode, MS_U8 u8OperatorID);
INTERFACE MS_BOOL Drv_VIP_VtrackSetKey(MS_BOOL bUserDefinded, MS_U8 *pu8Setting);
INTERFACE MS_BOOL Drv_VIP_VtrackSetUserDefindedSetting(MS_BOOL bUserDefinded, MS_U8 *pu8Setting);
INTERFACE MS_BOOL Drv_VIP_VtrackEnable( MS_U8 u8FrameRate, EN_VIP_DRVVTRACK_ENABLE_TYPE bEnable);
INTERFACE MS_BOOL Drv_VIP_GetBypassStatus(EN_VIP_DRV_CONFIG_TYPE enIPType);
INTERFACE wait_queue_head_t * Drv_VIP_GetWaitQueueHead(void);
INTERFACE MS_BOOL Drv_VIP_GetCMDQHWDone(void);

#undef INTERFACE

#endif
