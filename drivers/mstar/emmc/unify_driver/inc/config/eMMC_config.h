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
#ifndef __eMMC_CONFIG_H__
#define __eMMC_CONFIG_H__

#define UNIFIED_eMMC_DRIVER             1

//=====================================================
// select a HW platform:
//   - 1: enable, 0: disable.
//   - only one platform can be 1, others have to be 0.
//   - search and check all [FIXME] if need modify or not
//=====================================================

#ifndef U64
#define U64  unsigned long long
#endif
#ifndef U32
#define U32  unsigned int
#endif
#ifndef U16
#define U16  unsigned short
#endif
#ifndef U8
#define U8   unsigned char
#endif
#ifndef S64
#define S64  signed long long
#endif
#ifndef S32
#define S32  signed int
#endif
#ifndef S16
#define S16  signed short
#endif
#ifndef S8
#define S8   signed char
#endif

#if defined(CONFIG_ARCH_INFINITY3)
#define eMMC_DRV_INFINITY3_LINUX	 1
#endif

#if defined(eMMC_DRV_INFINITY3_LINUX) && eMMC_DRV_INFINITY3_LINUX
#include "eMMC_infinity3_linux.h"
#else
#error "Error! no platform selected."
#endif

//=====================================================
// misc. do NOT edit the following content.
//=====================================================
#define eMMC_DMA_RACING_PATCH           1
#define eMMC_DMA_PATCH_WAIT_TIME        DELAY_10ms_in_us
#define eMMC_DMA_RACING_PATTERN0        (((U32)'M'<<24)|((U32)0<<16)|((U32)'S'<<8)|(U32)1)
#define eMMC_DMA_RACING_PATTERN1        (((U32)'T'<<24)|((U32)6<<16)|((U32)'A'<<8)|(U32)8)

//===========================================================
// Time Dalay, do NOT edit the following content
//===========================================================
#if defined(eMMC_UPDATE_FIRMWARE) && (eMMC_UPDATE_FIRMWARE)
#define TIME_WAIT_DAT0_HIGH             (HW_TIMER_DELAY_1s*10)
#define TIME_WAIT_FCIE_RESET            (HW_TIMER_DELAY_1s*10)
#define TIME_WAIT_FCIE_RST_TOGGLE_CNT   (HW_TIMER_DELAY_1s*10)
#define TIME_WAIT_FIFOCLK_RDY           (HW_TIMER_DELAY_10ms*10)
#define TIME_WAIT_CMDRSP_END            (HW_TIMER_DELAY_10ms*10)
#define TIME_WAIT_1_BLK_END             (HW_TIMER_DELAY_1s*5)
#define TIME_WAIT_n_BLK_END             (HW_TIMER_DELAY_1s*10) // safe for 512 blocks
#else
#define TIME_WAIT_DAT0_HIGH             (HW_TIMER_DELAY_1s*10)
#define TIME_WAIT_ERASE_DAT0_HIGH       (HW_TIMER_DELAY_1s*10)
#define TIME_WAIT_FCIE_RESET            HW_TIMER_DELAY_500ms
#define TIME_WAIT_FCIE_RST_TOGGLE_CNT   HW_TIMER_DELAY_1us
#define TIME_WAIT_FIFOCLK_RDY           HW_TIMER_DELAY_10ms
#define TIME_WAIT_CMDRSP_END            HW_TIMER_DELAY_10ms
#define TIME_WAIT_1_BLK_END             (HW_TIMER_DELAY_1s*1)
#define TIME_WAIT_n_BLK_END             (HW_TIMER_DELAY_1s*2) // safe for 512 blocks
#endif

extern void mdelay_MacroToFun(u32 time);

#endif /* __eMMC_CONFIG_H__ */
