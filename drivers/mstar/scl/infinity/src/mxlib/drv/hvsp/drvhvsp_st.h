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
#ifndef _DRV_HVSP_ST_H
#define _DRV_HVSP_ST_H

#define DRV_HVSP_CROP_1     0
#define DRV_HVSP_CROP_2     1
#define DRV_HVSP_CROP_NUM   2
//-------------------------------------------------------------------------------------------------
//  Defines & enum
//-------------------------------------------------------------------------------------------------

typedef enum
{
    E_HVSP_CROP_ID_1,
    E_HVSP_CROP_ID_2,
    E_HVSP_CROP_ID_MAX,     // I1 has 2 crop IP
}EN_HVSP_CROP_ID_TYPE;

typedef enum
{
    E_HVSP_IP_MUX_BT656 ,   //I1 HW only has BT656 and ISP input source
    E_HVSP_IP_MUX_ISP ,
    E_HVSP_IP_MUX_HVSP ,    //SW used in HVSP 2
    E_HVSP_IP_MUX_PAT_TGEN, // SW used in debug pat
    E_HVSP_IP_MUX_MAX,      // 4 main type, 2 hw type, 2 sw type
}EN_HVSP_IP_MUX_TYPE;

typedef enum
{
    EN_HVSP_MONITOR_CROPCHECK  = 1,    ///< crop
    EN_HVSP_MONITOR_DMA1FRMCHECK = 2,    ///< dma1frm
    EN_HVSP_MONITOR_DMA1SNPCHECK = 3,    ///< dma1frm
    EN_HVSP_MONITOR_DMA2FRMCHECK = 4,    ///< dma1frm
    EN_HVSP_MONITOR_DMA3FRMCHECK = 5,    ///< dma1frm
}EN_HVSP_MONITOR_TYPE;
typedef enum
{
    EN_DRV_HVSP_OSD_LOC_AFTER  = 0,    ///< after hvsp
    EN_DRV_HVSP_OSD_LOC_BEFORE = 1,    ///< before hvsp
}EN_DRV_HVSP_OSD_LOC_TYPE;


typedef enum
{
    E_HVSP_ID_1,
    E_HVSP_ID_2,
    E_HVSP_ID_3,
    E_HVSP_ID_MAX,      //I1 has 3 HVSP
}EN_HVSP_ID_TYPE;

typedef enum
{
    E_HVSP_CMD_TRIG_NONE,           //RIU
    E_HVSP_CMD_TRIG_POLL_LDC_SYNC,  //polling blanking region RIU
    E_HVSP_CMD_TRIG_CMDQ_FRMCNT,    //wait framecnt CMDQ
    E_HVSP_CMD_TRIG_CMDQ_LDC_SYNC,  //used CMDQ in blanking region
    E_HVSP_CMD_TRIG_MAX,            //HVSP has 4 type to set register,2 RIU,2 CMDQ
}EN_HVSP_CMD_TRIG_TYPE;
typedef enum
{
    E_HVSP_EVENT_RUN = 0x00000001,
    E_HVSP_EVENT_IRQ = 0x00000002,
} MDrvHVSPTXEvent;
typedef enum
{
    EN_DRV_HVSP_FBMG_SET_LDCPATH_ON      = 0x1,
    EN_DRV_HVSP_FBMG_SET_LDCPATH_OFF     = 0x2,
    EN_DRV_HVSP_FBMG_SET_DNR_Read_ON     = 0x4,
    EN_DRV_HVSP_FBMG_SET_DNR_Read_OFF    = 0x8,
    EN_DRV_HVSP_FBMG_SET_DNR_Write_ON    = 0x10,
    EN_DRV_HVSP_FBMG_SET_DNR_Write_OFF   = 0x20,
    EN_DRV_HVSP_FBMG_SET_DNR_BUFFER_1    = 0x40,
    EN_DRV_HVSP_FBMG_SET_DNR_BUFFER_2    = 0x80,
    EN_DRV_HVSP_FBMG_SET_UNLOCK          = 0x100,
    EN_DRV_HVSP_FBMG_SET_DNR_COMDE_ON    = 0x200,
    EN_DRV_HVSP_FBMG_SET_DNR_COMDE_OFF   = 0x400,
    EN_DRV_HVSP_FBMG_SET_DNR_COMDE_265OFF   = 0x800,
}EN_DRV_HVSP_FBMG_SET_TYPE;

//-------------------------------------------------------------------------------------------------
//  Structure
//-------------------------------------------------------------------------------------------------

typedef struct
{
    MS_U32 u32RIUBase;      // I1 :0x1F000000
    MS_U32 u32IRQNUM;       // scl irq num by device tree
    MS_U32 u32CMDQIRQNUM;   // cmdq irq
}ST_HVSP_INIT_CONFIG;

typedef struct
{
    MS_U32 u32IRQNUM;
    MS_U32 u32CMDQIRQNUM;   // cmdq irq
}ST_HVSP_SUSPEND_RESUME_CONFIG;

typedef struct
{
    MSOS_ST_CLK* idclk; // I1 scl clk 1.idclk(for before crop)
    MSOS_ST_CLK* fclk1; // 2. fclk1(scl main clk)
    MSOS_ST_CLK* fclk2; // 3.fclk2(scl hvsp3 only)
    MSOS_ST_CLK* odclk; // 4.odclk(ttl only )
}ST_HVSP_CLK_CONFIG;
typedef struct
{
    MS_BOOL bRead;      // DNR IP can Read from Buffer (if can read then DNR availability)
    MS_BOOL bWrite;     // DNR IP can Write to Buffer
    MS_U32  u32BaseAddr;// DNR Buffer phycal address(unit:Byte)
    MS_U16  u16Vsize;   // DNR Frame Height size
    MS_U16  u16Fetch;   // DNR Frame Width size
    MS_U32  u32MemSize; // DNR buffer size
}ST_HVSP_IPM_CONFIG;


typedef struct
{
    EN_HVSP_CMD_TRIG_TYPE enType;
    MS_U8             u8Fmcnt;
}ST_HVSP_CMD_TRIG_CONFIG;

typedef struct
{
    ST_HVSP_CMD_TRIG_CONFIG stCmdTrigCfg;
    MS_U16 u16Src_Width;                      // Input source width(post crop1)
    MS_U16 u16Src_Height;                     // Input source height(post crop1)
    MS_BOOL bCropEn[DRV_HVSP_CROP_NUM];       // crop enable
    MS_U16 u16Crop_X[DRV_HVSP_CROP_NUM];      // crop x
    MS_U16 u16Crop_Y[DRV_HVSP_CROP_NUM];      // crop y
    MS_U16 u16Crop_Width[DRV_HVSP_CROP_NUM];  // post crop2 width
    MS_U16 u16Crop_Height[DRV_HVSP_CROP_NUM]; // post crop2 height
    MS_U16 u16Dsp_Width;                      // After scl display width
    MS_U16 u16Dsp_Height;                     // After scl display height
}ST_HVSP_SCALING_CONFIG;

typedef struct
{
    MS_BOOL bSet;
    MS_U16 u16Width;
    MS_U16 u16Height;
}ST_HVSP_SIZE_CONFIG;

typedef struct
{
    MS_BOOL             bCropEn;

    ST_HVSP_SIZE_CONFIG stSizeAfterCrop;
    ST_HVSP_SIZE_CONFIG stSizeAfterScaling;
    MS_U32              u32ScalingRatio_H;
    MS_U32              u32ScalingRatio_V;
}ST_HVSP_SCALING_INFO;

typedef struct
{
    unsigned short u16Width;    // LDC FB width
    unsigned short u16Height;   // LDC FB Heigh
    unsigned long u32FBaddr;    // DNR Buffer
    unsigned char u8FBidx;      // idx assign by sw
    unsigned char u8FBrwdiff;   // delay frame from DNR buffer
    unsigned char bEnSWMode;    // enable idx assign sw mode
    unsigned char bEnDNR;       // DNR enable?
}ST_HVSP_LDC_FRAMEBUFFER_CONFIG;

typedef struct
{
    MS_U16 u16Vtt;
    MS_U16 u16VBackPorch;
    MS_U16 u16VSyncWidth;
    MS_U16 u16VFrontPorch;
    MS_U16 u16VActive;
    MS_U16 u16Htt;
    MS_U16 u16HBackPorch;
    MS_U16 u16HSyncWidth;
    MS_U16 u16HFrontPorch;
    MS_U16 u16HActive;
}ST_DRV_HVSP_PAT_TGEN_CONFIG;

typedef struct
{
    unsigned short u16X;            //isp crop x
    unsigned short u16Y;            // isp crop y
    unsigned short u16Width;        // display width
    unsigned short u16Height;       // display height
    unsigned short u16crop2inWidth; // after isp crop width
    unsigned short u16crop2inHeight;// after isp crop height
    unsigned short u16crop2OutWidth; // after isp crop width
    unsigned short u16crop2OutHeight;// after isp crop height
    unsigned char  bEn;             //crop En
}ST_DRV_HVSP_SCINFORM_CONFIG;
typedef struct
{
    unsigned short u16Width;        // display width
    unsigned short u16Height;       // display height
    unsigned short u16inWidth; // after hvsp width
    unsigned short u16inHeight;// after hvsp height
    unsigned char  bEn;             //function En
}ST_DRV_HVSP_HVSPINFORM_CONFIG;
typedef struct
{
    EN_HVSP_IP_MUX_TYPE enMux;        // display width
    unsigned short u16inWidth; // isp width
    unsigned short u16inHeight;// isp height
    unsigned short u16inWidthcount; // isp width
    unsigned short u16inHeightcount;// isp height
}ST_DRV_HVSP_INPUTINFORM_CONFIG;
typedef struct
{
    EN_DRV_HVSP_FBMG_SET_TYPE enSet;
}ST_DRV_HVSP_SET_FB_MANAGE_CONFIG;
typedef struct
{
    unsigned char bOSDEn;    ///< OSD en
    unsigned char bOSDBypass;    ///< OSD en
    unsigned char bWTMBypass;    ///< OSD en
}ST_DRV_HVSP_OSD_ONOFF_CONFIG;

typedef struct
{
    EN_DRV_HVSP_OSD_LOC_TYPE enOSD_loc;    ///< OSD locate
    ST_DRV_HVSP_OSD_ONOFF_CONFIG stOsdOnOff;
}ST_DRV_HVSP_OSD_CONFIG;
//-------------------------------------------------------------------------------------------------
//  Prototype
//-------------------------------------------------------------------------------------------------

#endif
