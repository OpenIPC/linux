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
#ifndef DRV_SCLIRQ_ST_H
#define DRV_SCLIRQ_ST_H


//-------------------------------------------------------------------------------------------------
//  Defines & enum
//-------------------------------------------------------------------------------------------------
#define SCL_DELAYFRAME_FROM_ISP         (gu8DNRBufferReadyNum)               //0 or 1 or 2
#define SCLIRQ_MSK_SC_IN_FRM_END        (((MS_U64)1)<<0)
#define SCLIRQ_MSK_SC3_ENG_FRM_END      (((MS_U64)1)<<1)
#define SCLIRQ_MSK_SC1_ENG_FRM_END      (((MS_U64)1)<<2)
#define SCLIRQ_MSK_DISP_DMA_END         (((MS_U64)1)<<3)
#define SCLIRQ_MSK_VSYNC_IDCLK          (((MS_U64)1)<<4)
#define SCLIRQ_MSK_VSYNC_SC1_HVSP_FINISH (((MS_U64)1)<<6)
#define SCLIRQ_MSK_SC1_FRM_W_ACTIVE     (((MS_U64)1)<<7)
#define SCLIRQ_MSK_SC1_FRM_W_ACTIVE_N   (((MS_U64)1)<<8)
#define SCLIRQ_MSK_SC1_SNP_W_ACTIVE     (((MS_U64)1)<<9)
#define SCLIRQ_MSK_SC1_SNP_W_ACTIVE_N   (((MS_U64)1)<<10)
#define SCLIRQ_MSK_SC1_SNPI_W_ACTIVE    (((MS_U64)1)<<11)
#define SCLIRQ_MSK_SC1_SNPI_W_ACTIVE_N  (((MS_U64)1)<<12)
#define SCLIRQ_MSK_TRIG_LDC             (((MS_U64)1)<<13)
#define SCLIRQ_MSK_AFF_FULL             (((MS_U64)1)<<14)
#define SCLIRQ_MSK_VIP_HIST_DONE        (((MS_U64)1)<<15)
#define SCLIRQ_MSK_SC1_DBG_R_ACTIVE     (((MS_U64)1)<<17)
#define SCLIRQ_MSK_SC1_DBG_R_ACTIVE_N   (((MS_U64)1)<<18)
#define SCLIRQ_MSK_TTL_VS               (((MS_U64)1)<<19)
#define SCLIRQ_MSK_TTL_VDE              (((MS_U64)1)<<20)
#define SCLIRQ_MSK_TTL_VDE_N            (((MS_U64)1)<<21)
#define SCLIRQ_MSK_TRIG_DBG             (((MS_U64)1)<<22)
#define SCLIRQ_MSK_SC2_FRM_W_ACTIVE     (((MS_U64)1)<<23)
#define SCLIRQ_MSK_SC2_FRM_W_ACTIVE_N   (((MS_U64)1)<<24)
#define SCLIRQ_MSK_SC2_FRMI_W_ACTIVE    (((MS_U64)1)<<25)
#define SCLIRQ_MSK_SC2_FRMI_W_ACTIVE_N  (((MS_U64)1)<<26)
#define SCLIRQ_MSK_SC2_HVSP_FINISH      (((MS_U64)1)<<27)
#define SCLIRQ_MSK_VSYNC_FCLK2          (((MS_U64)1)<<28)
#define SCLIRQ_MSK_SC3_HVSP_FINISH      (((MS_U64)1)<<29)
#define SCLIRQ_MSK_TRIG_SC3_DMA_R       (((MS_U64)1)<<30)
#define SCLIRQ_MSK_SC3_DMA_R_ACTIVE     (((MS_U64)1)<<31)
#define SCLIRQ_MSK_SC3_DMA_R_ACTIVE_N   (((MS_U64)1)<<32)
#define SCLIRQ_MSK_SC3_DMA_W_ACTIVE     (((MS_U64)1)<<33)
#define SCLIRQ_MSK_SC3_DMA_W_ACTIVE_N   (((MS_U64)1)<<34)
#define SCLIRQ_MSK_VSYNC_FCLK_LDC       (((MS_U64)1)<<35)

#define SCLIRQ_SC_IN_FRM_END            0
#define SCLIRQ_SC3_ENG_FRM_END          1
#define SCLIRQ_SC1_ENG_FRM_END          2
#define SCLIRQ_DISP_DMA_END             3
#define SCLIRQ_VSYNC_IDCLK              4
#define SCLIRQ_VSYNC_FCLK               5
#define SCLIRQ_SC1_HVSP_FINISH          6
#define SCLIRQ_SC1_FRM_W_ACTIVE         7
#define SCLIRQ_SC1_FRM_W_ACTIVE_N       8
#define SCLIRQ_SC1_SNP_W_ACTIVE         9
#define SCLIRQ_SC1_SNP_W_ACTIVE_N       10
#define SCLIRQ_SC1_SNPI_W_ACTIVE        11
#define SCLIRQ_SC1_SNPI_W_ACTIVE_N      12
#define SCLIRQ_TRIG_LDC                 13
#define SCLIRQ_AFF_FULL                 14
#define SCLIRQ_VIP_HIST_DONE            15
#define SCLIRQ_RESERVED_00              16
#define SCLIRQ_SC1_DBG_R_ACTIVE         17
#define SCLIRQ_SC1_DBG_R_ACTIVE_N       18
#define SCLIRQ_TTL_VS                   19
#define SCLIRQ_TTL_VDE                  20
#define SCLIRQ_TTL_VDE_N                21
#define SCLIRQ_TRIG_DBG                 22
#define SCLIRQ_SC2_FRM_W_ACTIVE         23
#define SCLIRQ_SC2_FRM_W_ACTIVE_N       24
#define SCLIRQ_SC2_FRMI_W_ACTIVE        25
#define SCLIRQ_SC2_FRMI_W_ACTIVE_N      26
#define SCLIRQ_SC2_HVSP_FINISH          27
#define SCLIRQ_VSYNC_FCLK2              28
#define SCLIRQ_SC3_HVSP_FINISH          29
#define SCLIRQ_TRIG_SC3_DMA_R           30
#define SCLIRQ_SC3_DMA_R_ACTIVE         31
#define SCLIRQ_SC3_DMA_R_ACTIVE_N       32
#define SCLIRQ_SC3_DMA_W_ACTIVE         33
#define SCLIRQ_SC3_DMA_W_ACTIVE_N       34
#define SCLIRQ_VSYNC_FCLK_LDC           35
#define SCLIRQ_RESERVED                 48
#define SCLIRQ_NUM                      48  //I1 interrupt

#define SCLIRQ_EVENT_MSK_SC_IN_FRM_END        (((MS_U32)1)<<0)
#define SCLIRQ_EVENT_MSK_SC3_ENG_FRM_END      (((MS_U32)1)<<1)
#define SCLIRQ_EVENT_MSK_SC1_ENG_FRM_END      (((MS_U32)1)<<2)
#define SCLIRQ_EVENT_MSK_DISP_DMA_END         (((MS_U32)1)<<3)
#define SCLIRQ_EVENT_MSK_SC1_FRM_W_ACTIVE     (((MS_U32)1)<<4)
#define SCLIRQ_EVENT_MSK_SC1_FRM_W_ACTIVE_N   (((MS_U32)1)<<5)
#define SCLIRQ_EVENT_MSK_SC1_SNP_W_ACTIVE     (((MS_U32)1)<<6)
#define SCLIRQ_EVENT_MSK_SC1_SNP_W_ACTIVE_N   (((MS_U32)1)<<7)
#define SCLIRQ_EVENT_MSK_SC1_SNPI_W_ACTIVE    (((MS_U32)1)<<8)
#define SCLIRQ_EVENT_MSK_SC1_SNPI_W_ACTIVE_N  (((MS_U32)1)<<9)
#define SCLIRQ_EVENT_MSK_TRIG_LDC             (((MS_U32)1)<<10)
#define SCLIRQ_EVENT_MSK_SC1_DBG_R_ACTIVE     (((MS_U32)1)<<11)
#define SCLIRQ_EVENT_MSK_SC1_DBG_R_ACTIVE_N   (((MS_U32)1)<<12)
#define SCLIRQ_EVENT_MSK_SC2_FRM_W_ACTIVE     (((MS_U32)1)<<13)
#define SCLIRQ_EVENT_MSK_SC2_FRM_W_ACTIVE_N   (((MS_U32)1)<<14)
#define SCLIRQ_EVENT_MSK_SC2_FRMI_W_ACTIVE    (((MS_U32)1)<<15)
#define SCLIRQ_EVENT_MSK_SC2_FRMI_W_ACTIVE_N  (((MS_U32)1)<<16)
#define SCLIRQ_EVENT_MSK_SC3_DMA_R_ACTIVE     (((MS_U32)1)<<17)
#define SCLIRQ_EVENT_MSK_SC3_DMA_R_ACTIVE_N   (((MS_U32)1)<<18)
#define SCLIRQ_EVENT_MSK_SC3_DMA_W_ACTIVE     (((MS_U32)1)<<19)
#define SCLIRQ_EVENT_MSK_SC3_DMA_W_ACTIVE_N   (((MS_U32)1)<<20)
#define SCLIRQ_EVENT_MSK_VSYNC_FCLK_LDC       (((MS_U32)1)<<21)
#define SCLIRQ_EVENT_MSK_ALL                  (0xFFFFFFFF)


#define SCLIRQ_EVENT_SC_IN_FRM_END            0
#define SCLIRQ_EVENT_SC3_ENG_FRM_END          1
#define SCLIRQ_EVENT_SC1_ENG_FRM_END          2
#define SCLIRQ_EVENT_DISP_DMA_END             3
#define SCLIRQ_EVENT_SC1_FRM_W_ACTIVE         4
#define SCLIRQ_EVENT_SC1_FRM_W_ACTIVE_N       5
#define SCLIRQ_EVENT_SC1_SNP_W_ACTIVE         6
#define SCLIRQ_EVENT_SC1_SNP_W_ACTIVE_N       7
#define SCLIRQ_EVENT_SC1_SNPI_W_ACTIVE        8
#define SCLIRQ_EVENT_SC1_SNPI_W_ACTIVE_N      9
#define SCLIRQ_EVENT_TRIG_LDC                 10
#define SCLIRQ_EVENT_SC1_DBG_R_ACTIVE         11
#define SCLIRQ_EVENT_SC1_DBG_R_ACTIVE_N       12
#define SCLIRQ_EVENT_SC2_FRM_W_ACTIVE         13
#define SCLIRQ_EVENT_SC2_FRM_W_ACTIVE_N       14
#define SCLIRQ_EVENT_SC2_FRMI_W_ACTIVE        15
#define SCLIRQ_EVENT_SC2_FRMI_W_ACTIVE_N      16
#define SCLIRQ_EVENT_SC3_DMA_R_ACTIVE         17
#define SCLIRQ_EVENT_SC3_DMA_R_ACTIVE_N       18
#define SCLIRQ_EVENT_SC3_DMA_W_ACTIVE         19
#define SCLIRQ_EVENT_SC3_DMA_W_ACTIVE_N       20
#define SCLIRQ_EVENT_VSYNC_FCLK_LDC           21
#define SCLIRQ_EVENT_NUM                      22
#define SCLIRQ_ISTEVENT_BASE            0x10



#define PARSING_EVENT_MSK(x)   ( (x & SCLIRQ_EVENT_MSK_SC_IN_FRM_END)       == SCLIRQ_EVENT_MSK_SC_IN_FRM_END        ? "SCLIRQ_EVENT_MSK_SC_IN_FRM_END"       : \
                                 (x & SCLIRQ_EVENT_MSK_SC3_ENG_FRM_END)     == SCLIRQ_EVENT_MSK_SC3_ENG_FRM_END      ? "SCLIRQ_EVENT_MSK_SC3_ENG_FRM_END"     : \
                                 (x & SCLIRQ_EVENT_MSK_SC1_ENG_FRM_END)     == SCLIRQ_EVENT_MSK_SC1_ENG_FRM_END      ? "SCLIRQ_EVENT_MSK_SC1_ENG_FRM_END"     : \
                                 (x & SCLIRQ_EVENT_MSK_DISP_DMA_END)        == SCLIRQ_EVENT_MSK_DISP_DMA_END         ? "SCLIRQ_EVENT_MSK_DISP_DMA_END"        : \
                                 (x & SCLIRQ_EVENT_MSK_SC1_FRM_W_ACTIVE)    == SCLIRQ_EVENT_MSK_SC1_FRM_W_ACTIVE     ? "SCLIRQ_EVENT_MSK_SC1_FRM_W_ACTIVE"    : \
                                 (x & SCLIRQ_EVENT_MSK_SC1_FRM_W_ACTIVE_N)  == SCLIRQ_EVENT_MSK_SC1_FRM_W_ACTIVE_N   ? "SCLIRQ_EVENT_MSK_SC1_FRM_W_ACTIVE_N"  : \
                                 (x & SCLIRQ_EVENT_MSK_SC1_SNP_W_ACTIVE)    == SCLIRQ_EVENT_MSK_SC1_SNP_W_ACTIVE     ? "SCLIRQ_EVENT_MSK_SC1_SNP_W_ACTIVE"    : \
                                 (x & SCLIRQ_EVENT_MSK_SC1_SNP_W_ACTIVE_N)  == SCLIRQ_EVENT_MSK_SC1_SNP_W_ACTIVE_N   ? "SCLIRQ_EVENT_MSK_SC1_SNP_W_ACTIVE_N"  : \
                                 (x & SCLIRQ_EVENT_MSK_SC1_SNPI_W_ACTIVE)   == SCLIRQ_EVENT_MSK_SC1_SNPI_W_ACTIVE    ? "SCLIRQ_EVENT_MSK_SC1_SNPI_W_ACTIVE"   : \
                                 (x & SCLIRQ_EVENT_MSK_SC1_SNPI_W_ACTIVE_N) == SCLIRQ_EVENT_MSK_SC1_SNPI_W_ACTIVE_N  ? "SCLIRQ_EVENT_MSK_SC1_SNPI_W_ACTIVE_N" : \
                                 (x & SCLIRQ_EVENT_MSK_TRIG_LDC)            == SCLIRQ_EVENT_MSK_TRIG_LDC             ? "SCLIRQ_EVENT_MSK_TRIG_LDC"            : \
                                 (x & SCLIRQ_EVENT_MSK_SC1_DBG_R_ACTIVE)    == SCLIRQ_EVENT_MSK_SC1_DBG_R_ACTIVE     ? "SCLIRQ_EVENT_MSK_SC1_DBG_R_ACTIVE"    : \
                                 (x & SCLIRQ_EVENT_MSK_SC1_DBG_R_ACTIVE_N)  == SCLIRQ_EVENT_MSK_SC1_DBG_R_ACTIVE_N   ? "SCLIRQ_EVENT_MSK_SC1_DBG_R_ACTIVE_N"  : \
                                 (x & SCLIRQ_EVENT_MSK_SC2_FRM_W_ACTIVE)    == SCLIRQ_EVENT_MSK_SC2_FRM_W_ACTIVE     ? "SCLIRQ_EVENT_MSK_SC2_FRM_W_ACTIVE"    : \
                                 (x & SCLIRQ_EVENT_MSK_SC2_FRM_W_ACTIVE_N)  == SCLIRQ_EVENT_MSK_SC2_FRM_W_ACTIVE_N   ? "SCLIRQ_EVENT_MSK_SC2_FRM_W_ACTIVE_N"  : \
                                 (x & SCLIRQ_EVENT_MSK_SC2_FRMI_W_ACTIVE)   == SCLIRQ_EVENT_MSK_SC2_FRMI_W_ACTIVE    ? "SCLIRQ_EVENT_MSK_SC2_FRMI_W_ACTIVE"   : \
                                 (x & SCLIRQ_EVENT_MSK_SC2_FRMI_W_ACTIVE_N) == SCLIRQ_EVENT_MSK_SC2_FRMI_W_ACTIVE_N  ? "SCLIRQ_EVENT_MSK_SC2_FRMI_W_ACTIVE_N" : \
                                 (x & SCLIRQ_EVENT_MSK_SC3_DMA_R_ACTIVE)    == SCLIRQ_EVENT_MSK_SC3_DMA_R_ACTIVE     ? "SCLIRQ_EVENT_MSK_SC3_DMA_R_ACTIVE"    : \
                                 (x & SCLIRQ_EVENT_MSK_SC3_DMA_R_ACTIVE_N)  == SCLIRQ_EVENT_MSK_SC3_DMA_R_ACTIVE_N   ? "SCLIRQ_EVENT_MSK_SC3_DMA_R_ACTIVE_N"  : \
                                 (x & SCLIRQ_EVENT_MSK_SC3_DMA_W_ACTIVE)    == SCLIRQ_EVENT_MSK_SC3_DMA_W_ACTIVE     ? "SCLIRQ_EVENT_MSK_SC3_DMA_W_ACTIVE"    : \
                                 (x & SCLIRQ_EVENT_MSK_SC3_DMA_W_ACTIVE_N)  == SCLIRQ_EVENT_MSK_SC3_DMA_W_ACTIVE_N   ? "SCLIRQ_EVENT_MSK_SC3_DMA_W_ACTIVE_N"  : \
                                 (x & SCLIRQ_EVENT_MSK_VSYNC_FCLK_LDC)      == SCLIRQ_EVENT_MSK_VSYNC_FCLK_LDC       ? "SCLIRQ_EVENT_MSK_VSYNC_FCLK_LDC"      : \
                                                                                                                       "UNKNOWN")


#define PARSING_IRQ_MSK(x)     ( (x & SCLIRQ_MSK_SC_IN_FRM_END      ) ? "SCLIRQ_MSK_SC_IN_FRM_END         " : \
                                 (x & SCLIRQ_MSK_SC3_ENG_FRM_END    ) ? "SCLIRQ_MSK_SC3_ENG_FRM_END       " : \
                                 (x & SCLIRQ_MSK_SC1_ENG_FRM_END    ) ? "SCLIRQ_MSK_SC1_ENG_FRM_END       " : \
                                 (x & SCLIRQ_MSK_DISP_DMA_END       ) ? "SCLIRQ_MSK_DISP_DMA_END          " : \
                                 (x & SCLIRQ_MSK_SC1_FRM_W_ACTIVE   ) ? "SCLIRQ_MSK_SC1_FRM_W_ACTIVE      " : \
                                 (x & SCLIRQ_MSK_SC1_FRM_W_ACTIVE_N ) ? "SCLIRQ_MSK_SC1_FRM_W_ACTIVE_N    " : \
                                 (x & SCLIRQ_MSK_SC1_SNP_W_ACTIVE   ) ? "SCLIRQ_MSK_SC1_SNP_W_ACTIVE      " : \
                                 (x & SCLIRQ_MSK_SC1_SNP_W_ACTIVE_N ) ? "SCLIRQ_MSK_SC1_SNP_W_ACTIVE_N    " : \
                                 (x & SCLIRQ_MSK_SC1_SNPI_W_ACTIVE  ) ? "SCLIRQ_MSK_SC1_SNPI_W_ACTIVE     " : \
                                 (x & SCLIRQ_MSK_SC1_SNPI_W_ACTIVE_N) ? "SCLIRQ_MSK_SC1_SNPI_W_ACTIVE_N   " : \
                                 (x & SCLIRQ_MSK_TRIG_LDC           ) ? "SCLIRQ_MSK_TRIG_LDC              " : \
                                 (x & SCLIRQ_MSK_AFF_FULL           ) ? "SCLIRQ_MSK_AFF_FULL              " : \
                                 (x & SCLIRQ_MSK_VIP_HIST_DONE      ) ? "SCLIRQ_MSK_VIP_HIST_DONE         " : \
                                 (x & SCLIRQ_MSK_SC1_DBG_R_ACTIVE   ) ? "SCLIRQ_MSK_SC1_DBG_R_ACTIVE      " : \
                                 (x & SCLIRQ_MSK_SC1_DBG_R_ACTIVE_N ) ? "SCLIRQ_MSK_SC1_DBG_R_ACTIVE_N    " : \
                                 (x & SCLIRQ_MSK_TTL_VS             ) ? "SCLIRQ_MSK_TTL_VS                " : \
                                 (x & SCLIRQ_MSK_TTL_VDE            ) ? "SCLIRQ_MSK_TTL_VDE               " : \
                                 (x & SCLIRQ_MSK_TTL_VDE_N          ) ? "SCLIRQ_MSK_TTL_VDE_N             " : \
                                 (x & SCLIRQ_MSK_TRIG_DBG           ) ? "SCLIRQ_MSK_TRIG_DBG              " : \
                                 (x & SCLIRQ_MSK_SC2_FRM_W_ACTIVE   ) ? "SCLIRQ_MSK_SC2_FRM_W_ACTIVE      " : \
                                 (x & SCLIRQ_MSK_SC2_FRM_W_ACTIVE_N ) ? "SCLIRQ_MSK_SC2_FRM_W_ACTIVE_N    " : \
                                 (x & SCLIRQ_MSK_SC2_FRMI_W_ACTIVE  ) ? "SCLIRQ_MSK_SC2_FRMI_W_ACTIVE     " : \
                                 (x & SCLIRQ_MSK_SC2_FRMI_W_ACTIVE_N) ? "SCLIRQ_MSK_SC2_FRMI_W_ACTIVE_N   " : \
                                 (x & SCLIRQ_MSK_SC2_HVSP_FINISH    ) ? "SCLIRQ_MSK_SC2_HVSP_FINISH       " : \
                                 (x & SCLIRQ_MSK_VSYNC_FCLK2        ) ? "SCLIRQ_MSK_VSYNC_FCLK2           " : \
                                 (x & SCLIRQ_MSK_SC3_HVSP_FINISH    ) ? "SCLIRQ_MSK_SC3_HVSP_FINISH       " : \
                                 (x & SCLIRQ_MSK_TRIG_SC3_DMA_R     ) ? "SCLIRQ_MSK_TRIG_SC3_DMA_R        " : \
                                 (x & SCLIRQ_MSK_SC3_DMA_R_ACTIVE   ) ? "SCLIRQ_MSK_SC3_DMA_R_ACTIVE      " : \
                                 (x & SCLIRQ_MSK_SC3_DMA_R_ACTIVE_N ) ? "SCLIRQ_MSK_SC3_DMA_R_ACTIVE_N    " : \
                                 (x & SCLIRQ_MSK_SC3_DMA_W_ACTIVE   ) ? "SCLIRQ_MSK_SC3_DMA_W_ACTIVE      " : \
                                 (x & SCLIRQ_MSK_SC3_DMA_W_ACTIVE_N ) ? "SCLIRQ_MSK_SC3_DMA_W_ACTIVE_N    " : \
                                 (x & SCLIRQ_MSK_VSYNC_FCLK_LDC     ) ? "SCLIRQ_MSK_VSYNC_FCLK_LDC        " : \
                                                                        "UNKNOWN")
#define _IsFlagType(u8client,u8type)                     (gstScldmaInfo.bDmaflag[(u8client)] & (u8type))
#define _SetANDGetFlagType(u8client,u8Set,u8get)         (gstScldmaInfo.bDmaflag[(u8client)]=((gstScldmaInfo.bDmaflag[(u8client)]&(u8get))|(u8Set)))
#define _SetFlagType(u8client,u8type)                    (gstScldmaInfo.bDmaflag[(u8client)] |= (u8type))
#define _ReSetFlagType(u8client,u8type)                  (gstScldmaInfo.bDmaflag[(u8client)] &= ~(u8type))
#define _GetIdxType(u8client,u8type)                     (gstScldmaInfo.bDMAidx[(u8client)] & (u8type))
#define _SetANDGetIdxType(u8client,u8Set,u8get)         (gstScldmaInfo.bDMAidx[(u8client)]=((gstScldmaInfo.bDMAidx[(u8client)]&(u8get))|(u8Set)))
#define _SetIdxType(u8client,u8type)                     (gstScldmaInfo.bDMAidx[(u8client)] |= (u8type))
#define _ReSetIdxType(u8client,u8type)                   (gstScldmaInfo.bDMAidx[(u8client)] &= ~(u8type))

//-------------------------------------------------------------------------------------------------
//  Structure
//-------------------------------------------------------------------------------------------------
typedef enum
{
    E_SCLIRQ_EVENT_CLEAR      = 0x00000000, // no event
    E_SCLIRQ_EVENT_SC1FRM     = 0x00000001, // SC1FRM done (for scldma done,avoid flag to update but not poll,keep it status)
    E_SCLIRQ_EVENT_SC1SNP     = 0x00000002, // SC1SNP done
    E_SCLIRQ_EVENT_SC1        = (E_SCLIRQ_EVENT_SC1SNP|E_SCLIRQ_EVENT_SC1FRM), // SC1all done
    E_SCLIRQ_EVENT_SC2FRM     = 0x00000004, // SC2 FRM done
    E_SCLIRQ_EVENT_FRMEND     = (E_SCLIRQ_EVENT_SC2FRM|E_SCLIRQ_EVENT_SC1), // SC1 &SC2  done
    E_SCLIRQ_EVENT_IRQ        = 0x00000008, // any one FRM done
    E_SCLIRQ_EVENT_ISTSC1FRM  = 0x00000010, // SC1FRM done for IST
    E_SCLIRQ_EVENT_ISTSC1SNP  = 0x00000020, // SC1SNP done for IST
    E_SCLIRQ_EVENT_ISTSC1     = (E_SCLIRQ_EVENT_ISTSC1FRM|E_SCLIRQ_EVENT_ISTSC1SNP), // SC1all done for IST
    E_SCLIRQ_EVENT_ISTSC2FRM  = 0x00000080, // SC2 FRM done for IST
    E_SCLIRQ_EVENT_ISTFRMEND  = (E_SCLIRQ_EVENT_ISTSC2FRM|E_SCLIRQ_EVENT_ISTSC1), // SC1 &SC2  done for IST
    E_SCLIRQ_EVENT_SC1RESTART = 0x00000100, // SC1 FRM re-start
    E_SCLIRQ_EVENT_SC2RESTART = 0x00000200, // SC2 FRM re-start
    E_SCLIRQ_EVENT_ALLRESEART = (E_SCLIRQ_EVENT_SC2RESTART|E_SCLIRQ_EVENT_SC1RESTART), // SC1 SC2 restart
    E_SCLIRQ_EVENT_RESUME     = 0x00000400, // Suspend/Resume
    E_SCLIRQ_EVENT_HVSPST     = 0x00000800,
    E_SCLIRQ_EVENT_SC1RE      = (E_SCLIRQ_EVENT_SC1RESTART | E_SCLIRQ_EVENT_RESUME),
    E_SCLIRQ_EVENT_SC2RE      = (E_SCLIRQ_EVENT_SC2RESTART | E_SCLIRQ_EVENT_RESUME),
    E_SCLIRQ_EVENT_SC1POLL    = (E_SCLIRQ_EVENT_SC1|E_SCLIRQ_EVENT_SC1RESTART|E_SCLIRQ_EVENT_RESUME),
    E_SCLIRQ_EVENT_SC2POLL    = (E_SCLIRQ_EVENT_SC2FRM|E_SCLIRQ_EVENT_SC2RESTART|E_SCLIRQ_EVENT_RESUME),
    E_SCLIRQ_EVENT_ALLPOLL    = (E_SCLIRQ_EVENT_FRMEND|E_SCLIRQ_EVENT_ALLRESEART|E_SCLIRQ_EVENT_RESUME),
} SCLIRQTXEvent;

typedef enum
{
    E_SCLIRQ_SC3EVENT_CLEAR      = 0x00000000, // no event
    E_SCLIRQ_SC3EVENT_ACTIVE     = 0x00000001, // SC3FRM act
    E_SCLIRQ_SC3EVENT_ACTIVEN    = 0x00000002, // SC3FRM act_N
    E_SCLIRQ_SC3EVENT_END        = 0x00000003,
    E_SCLIRQ_SC3EVENT_DONE       = 0x00000004, // SC3FRM done
    E_SCLIRQ_SC3EVENT_IRQ        = 0x00000008, // any one ISR need to handle
    E_SCLIRQ_SC3EVENT_ISTACTIVE  = 0x00000010, // SC3 FRM act for IST
    E_SCLIRQ_SC3EVENT_ISTACTIVEN = 0x00000020, // SC3 done for IST
    E_SCLIRQ_SC3EVENT_ISTDONE    = 0x00000030, // SC3 done for IST
    E_SCLIRQ_SC3EVENT_RESTART    = 0x00000100, // SC3 FRM re-start
    E_SCLIRQ_SC3EVENT_RESUME     = 0x00000400, // Suspend/Resume
    E_SCLIRQ_SC3EVENT_HVSPST     = 0x00000800,
    E_SCLIRQ_SC3EVENT_ISTCLEAR   = (E_SCLIRQ_SC3EVENT_IRQ | E_SCLIRQ_SC3EVENT_ISTDONE | E_SCLIRQ_SC3EVENT_END),
    E_SCLIRQ_SC3EVENT_POLL = (E_SCLIRQ_SC3EVENT_HVSPST | E_SCLIRQ_SC3EVENT_DONE|E_SCLIRQ_SC3EVENT_RESTART|E_SCLIRQ_SC3EVENT_RESUME),
} SCLIRQSC3Event;

typedef enum
{
    E_SCLIRQ_EVENT_SYNC         = 0x00000001, // Ldc trig
    E_SCLIRQ_EVENT_FRMENDSYNC   = 0x00000002, // frame end
    E_SCLIRQ_EVENT_CMDQGOING    = 0x00000010, // CMDQ going
    E_SCLIRQ_EVENT_CMDQIDLE     = 0x00000020, // CMDQ idle
    E_SCLIRQ_EVENT_CMDQDONE     = 0x00000040, // CMDQ done
    E_SCLIRQ_EVENT_CMDQTIMEOUT  = 0x00000080, // timeout
    E_SCLIRQ_EVENT_CMDQ = (E_SCLIRQ_EVENT_CMDQGOING|E_SCLIRQ_EVENT_CMDQIDLE|E_SCLIRQ_EVENT_CMDQDONE|E_SCLIRQ_EVENT_CMDQTIMEOUT),
}SCLIRQRXEvent;

typedef enum
{
    E_SCLIRQ_FRM_IN_COUNT_NOW        = 0x1,  // FRAMEIN NOW
    E_SCLIRQ_FRM_IN_COUNT_DELAYONE   = 0x2,  // delay one
    E_SCLIRQ_FRM_IN_COUNT_DELAYTWO   = 0x4,  // delay two
}E_SCLIRQ_FRM_IN_COUNT_TYPE;
//-------------------------------------------------------------------------------------------------
//  Prototype
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//  extern Variable
//-------------------------------------------------------------------------------------------------
/////////////////
/// bclkforcemode
/// use in Drvscldma and Drvhvsp
/// if True ,can't dynamic set Clk
////////////////
extern unsigned char gbclkforcemode;
#endif
