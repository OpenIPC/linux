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
#ifndef _DRV_HVSP_H
#define _DRV_HVSP_H


//-------------------------------------------------------------------------------------------------
//  Defines & Structure
//-------------------------------------------------------------------------------------------------



//-------------------------------------------------------------------------------------------------
//  Prototype
//-------------------------------------------------------------------------------------------------
#ifndef DRV_HVSP_C
#define INTERFACE extern
#else
#define INTERFACE
#endif
#define HVSP_irq                    0
INTERFACE void    Drv_HVSP_SetMemoryAllocateReady(MS_BOOL bEn);
INTERFACE MS_BOOL Drv_HVSP_Init(ST_HVSP_INIT_CONFIG *pInitCfg);
INTERFACE void Drv_HVSP_SetBufferNum(MS_U8 u8Num);
INTERFACE MS_BOOL Drv_HVSP_GetBufferNum(void);
INTERFACE MS_BOOL Drv_HVSP_Set_IPM_Config(ST_HVSP_IPM_CONFIG stCfg);
INTERFACE void Drv_HVSP_SetFbManageConfig(ST_DRV_HVSP_SET_FB_MANAGE_CONFIG stCfg);
INTERFACE MS_BOOL Drv_HVSP_SetScaling(EN_HVSP_ID_TYPE enHVSP_ID, ST_HVSP_SCALING_CONFIG stCfg, ST_HVSP_CLK_CONFIG* stclk);
INTERFACE MS_BOOL Drv_HVSP_SetInputMux(EN_HVSP_IP_MUX_TYPE enIP,ST_HVSP_CLK_CONFIG* stclk);
INTERFACE MS_BOOL Drv_HVSP_SetRegisterForce(MS_U32 u32Reg, MS_U8 u8Val, MS_U8 u8Msk);
INTERFACE MS_BOOL Drv_HVSP_SetLDCFrameBuffer_Config(ST_HVSP_LDC_FRAMEBUFFER_CONFIG stLDCCfg);
INTERFACE MS_BOOL Drv_HVSP_SetPatTgen(MS_BOOL bEn, ST_DRV_HVSP_PAT_TGEN_CONFIG *pCfg);
INTERFACE MS_BOOL Drv_HVSP_GetSCLInform(EN_HVSP_ID_TYPE enID,ST_DRV_HVSP_SCINFORM_CONFIG *stInformCfg);
INTERFACE void Drv_HVSP_SetOSDConfig(EN_HVSP_ID_TYPE enID, ST_DRV_HVSP_OSD_CONFIG stOSdCfg);
INTERFACE void Drv_HVSP_IDCLKRelease(ST_HVSP_CLK_CONFIG* stclk);
INTERFACE unsigned long Drv_HVSP_CropCheck(void);
INTERFACE unsigned long Drv_HVSP_DMACheck(EN_HVSP_MONITOR_TYPE enMonitorType);
INTERFACE MS_U8 Drv_HVSP_CheckInputVSync(void);
INTERFACE void Drv_HVSP_GetCrop12Inform(ST_DRV_HVSP_INPUTINFORM_CONFIG *stInformCfg);
INTERFACE MS_BOOL Drv_HVSP_GetFrameBufferAttribute(EN_HVSP_ID_TYPE enID,ST_HVSP_IPM_CONFIG *stInformCfg);
INTERFACE MS_BOOL Drv_HVSP_GetHVSPAttribute(EN_HVSP_ID_TYPE enID,ST_DRV_HVSP_HVSPINFORM_CONFIG *stInformCfg);
INTERFACE void Drv_HVSP_GetOSDAttribute(EN_HVSP_ID_TYPE enID,ST_DRV_HVSP_OSD_CONFIG *stOsdCfg);
INTERFACE MS_BOOL Drv_HVSP_Suspend(ST_HVSP_SUSPEND_RESUME_CONFIG *pCfg);
INTERFACE MS_BOOL Drv_HVSP_Resume(ST_HVSP_SUSPEND_RESUME_CONFIG *pCfg);
INTERFACE void Drv_HVSP_Release(EN_HVSP_ID_TYPE HVSP_IP);
INTERFACE MS_BOOL Drv_HVSP_SetCMDQTrigType(ST_HVSP_CMD_TRIG_CONFIG stCmdTrigCfg);
INTERFACE ST_HVSP_CMD_TRIG_CONFIG Drv_HVSP_SetCMDQTrigTypeByRIU(void);
INTERFACE void Drv_HVSP_SetCLKForcemode(unsigned char bEn);
INTERFACE MS_BOOL Drv_HVSP_GetCLKForcemode(void);
INTERFACE void Drv_HVSP_SetCLKRate(unsigned char u8Idx);
INTERFACE unsigned long Drv_HVSP_CMDQStatusReport(void);
INTERFACE void Drv_HVSP_SetLDCONOFF(MS_BOOL bEn);
INTERFACE wait_queue_head_t * Drv_HVSP_GetWaitQueueHead(void);
INTERFACE MS_BOOL Drv_HVSP_GetCMDQDoneStatus(void);
INTERFACE void Drv_HVSP_SetCropWindowSize(void);
INTERFACE void Drv_HVSP_SetInputSrcSize(MS_U16 u16Height,MS_U16 u16Width);

#undef INTERFACE

#endif
