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

#ifndef _LPLL_TBL_H_
#define _LPLL_TBL_H_

#define LPLL_REG_NUM    17

typedef enum
{
    E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_40TO80MHZ,          //0
    E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_20TO40MHZ,          //1
    E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_10TO20MHZ,          //2
    E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_5TO10MHZ,          //3
    E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_5TO5MHZ,          //4

    E_PNL_SUPPORTED_LPLL_MAX,          //5
} E_PNL_SUPPORTED_LPLL_TYPE;

typedef struct
{
    MS_U8  address;
    MS_U16 value;
    MS_U16 mask;
}TBLStruct,*pTBLStruct;

TBLStruct LPLLSettingTBL[E_PNL_SUPPORTED_LPLL_MAX][LPLL_REG_NUM]=
{
    { //E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_40TO80MHZ    NO.0
      //Address,Value,Mask
        {0x40,0x0000,0x8000},//reg_lpll_ext_pd
        {0x40,0x0001,0x0007},//reg_lpll_ext_ictrl
        {0x41,0x0000,0x0003},//reg_lpll_ext_input_div_fst
        {0x41,0x0020,0x0030},//reg_lpll_ext_loop_div_fst
        {0x41,0x0300,0x0F00},//reg_lpll_ext_loop_div_sec
        {0x42,0x0002,0x0003},//reg_lpll_ext_scalar_div_fst
        {0x42,0x0050,0x00F0},//reg_lpll_ext_scalar_div_sec
        {0x43,0x0007,0x0007},//reg_lpll_ext_skew_div
        {0x42,0x0000,0x0700},//reg_lpll_ext_fifo_div
        {0x43,0x0010,0x0010},//reg_lpll_ext_skew_en_fixclk
        {0x40,0x0000,0x0800},//reg_lpll_ext_dual_lp_en
        {0x40,0x0000,0x0100},//reg_lpll_ext_sdiv2p5_en
        {0x40,0x2000,0x2000},//reg_lpll_ext_en_mini
        {0x40,0x0000,0x0400},//reg_lpll_ext_en_fifo
        {0x40,0x0200,0x0200},//reg_lpll_ext_en_scalar
        {0x40,0x0000,0x1000},//reg_lpll_ext_fifo_div5_en
        {0x40,0x0000,0x0008},//reg_lpll_ext_sdiv3p5_en
    },

    { //E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_20TO40MHZ    NO.1
      //Address,Value,Mask
        {0x40,0x0000,0x8000},//reg_lpll_ext_pd
        {0x40,0x0001,0x0007},//reg_lpll_ext_ictrl
        {0x41,0x0000,0x0003},//reg_lpll_ext_input_div_fst
        {0x41,0x0020,0x0030},//reg_lpll_ext_loop_div_fst
        {0x41,0x0300,0x0F00},//reg_lpll_ext_loop_div_sec
        {0x42,0x0003,0x0003},//reg_lpll_ext_scalar_div_fst
        {0x42,0x0050,0x00F0},//reg_lpll_ext_scalar_div_sec
        {0x43,0x0007,0x0007},//reg_lpll_ext_skew_div
        {0x42,0x0000,0x0700},//reg_lpll_ext_fifo_div
        {0x43,0x0010,0x0010},//reg_lpll_ext_skew_en_fixclk
        {0x40,0x0000,0x0800},//reg_lpll_ext_dual_lp_en
        {0x40,0x0000,0x0100},//reg_lpll_ext_sdiv2p5_en
        {0x40,0x2000,0x2000},//reg_lpll_ext_en_mini
        {0x40,0x0000,0x0400},//reg_lpll_ext_en_fifo
        {0x40,0x0200,0x0200},//reg_lpll_ext_en_scalar
        {0x40,0x0000,0x1000},//reg_lpll_ext_fifo_div5_en
        {0x40,0x0000,0x0008},//reg_lpll_ext_sdiv3p5_en
    },

    { //E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_10TO20MHZ    NO.2
      //Address,Value,Mask
        {0x40,0x0000,0x8000},//reg_lpll_ext_pd
        {0x40,0x0001,0x0007},//reg_lpll_ext_ictrl
        {0x41,0x0000,0x0003},//reg_lpll_ext_input_div_fst
        {0x41,0x0020,0x0030},//reg_lpll_ext_loop_div_fst
        {0x41,0x0300,0x0F00},//reg_lpll_ext_loop_div_sec
        {0x42,0x0003,0x0003},//reg_lpll_ext_scalar_div_fst
        {0x42,0x00A0,0x00F0},//reg_lpll_ext_scalar_div_sec
        {0x43,0x0007,0x0007},//reg_lpll_ext_skew_div
        {0x42,0x0000,0x0700},//reg_lpll_ext_fifo_div
        {0x43,0x0010,0x0010},//reg_lpll_ext_skew_en_fixclk
        {0x40,0x0000,0x0800},//reg_lpll_ext_dual_lp_en
        {0x40,0x0000,0x0100},//reg_lpll_ext_sdiv2p5_en
        {0x40,0x2000,0x2000},//reg_lpll_ext_en_mini
        {0x40,0x0000,0x0400},//reg_lpll_ext_en_fifo
        {0x40,0x0200,0x0200},//reg_lpll_ext_en_scalar
        {0x40,0x0000,0x1000},//reg_lpll_ext_fifo_div5_en
        {0x40,0x0000,0x0008},//reg_lpll_ext_sdiv3p5_en
    },

    { //E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_5TO10MHZ    NO.3
      //Address,Value,Mask
        {0x40,0x0000,0x8000},//reg_lpll_ext_pd
        {0x40,0x0001,0x0007},//reg_lpll_ext_ictrl
        {0x41,0x0000,0x0003},//reg_lpll_ext_input_div_fst
        {0x41,0x0020,0x0030},//reg_lpll_ext_loop_div_fst
        {0x41,0x0300,0x0F00},//reg_lpll_ext_loop_div_sec
        {0x42,0x0003,0x0003},//reg_lpll_ext_scalar_div_fst
        {0x42,0x0080,0x00F0},//reg_lpll_ext_scalar_div_sec
        {0x43,0x0007,0x0007},//reg_lpll_ext_skew_div
        {0x42,0x0000,0x0700},//reg_lpll_ext_fifo_div
        {0x43,0x0010,0x0010},//reg_lpll_ext_skew_en_fixclk
        {0x40,0x0000,0x0800},//reg_lpll_ext_dual_lp_en
        {0x40,0x0100,0x0100},//reg_lpll_ext_sdiv2p5_en
        {0x40,0x2000,0x2000},//reg_lpll_ext_en_mini
        {0x40,0x0000,0x0400},//reg_lpll_ext_en_fifo
        {0x40,0x0200,0x0200},//reg_lpll_ext_en_scalar
        {0x40,0x0000,0x1000},//reg_lpll_ext_fifo_div5_en
        {0x40,0x0000,0x0008},//reg_lpll_ext_sdiv3p5_en
    },

    { //E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_5TO5MHZ    NO.4
      //Address,Value,Mask
        {0x40,0x0000,0x8000},//reg_lpll_ext_pd
        {0x40,0x0001,0x0007},//reg_lpll_ext_ictrl
        {0x41,0x0000,0x0003},//reg_lpll_ext_input_div_fst
        {0x41,0x0020,0x0030},//reg_lpll_ext_loop_div_fst
        {0x41,0x0300,0x0F00},//reg_lpll_ext_loop_div_sec
        {0x42,0x0003,0x0003},//reg_lpll_ext_scalar_div_fst
        {0x42,0x0080,0x00F0},//reg_lpll_ext_scalar_div_sec
        {0x43,0x0007,0x0007},//reg_lpll_ext_skew_div
        {0x42,0x0000,0x0700},//reg_lpll_ext_fifo_div
        {0x43,0x0010,0x0010},//reg_lpll_ext_skew_en_fixclk
        {0x40,0x0000,0x0800},//reg_lpll_ext_dual_lp_en
        {0x40,0x0100,0x0100},//reg_lpll_ext_sdiv2p5_en
        {0x40,0x2000,0x2000},//reg_lpll_ext_en_mini
        {0x40,0x0000,0x0400},//reg_lpll_ext_en_fifo
        {0x40,0x0200,0x0200},//reg_lpll_ext_en_scalar
        {0x40,0x0000,0x1000},//reg_lpll_ext_fifo_div5_en
        {0x40,0x0000,0x0008},//reg_lpll_ext_sdiv3p5_en
    },

};
MS_U16 u16LoopGain[E_PNL_SUPPORTED_LPLL_MAX]=
{
    12,           //E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_40TO80MHZ    NO.0
    12,           //E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_20TO40MHZ    NO.1
    12,           //E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_10TO20MHZ    NO.2
    12,           //E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_5TO10MHZ    NO.3
    12,           //E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_5TO5MHZ    NO.4
};
MS_U16 u16LoopDiv[E_PNL_SUPPORTED_LPLL_MAX]=
{
    20,           //E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_40TO80MHZ    NO.0
    40,           //E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_20TO40MHZ    NO.1
    80,           //E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_10TO20MHZ    NO.2
    160,           //E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_5TO10MHZ    NO.3
    160,           //E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_5TO5MHZ    NO.4
};

#endif //_LPLL_TBL_H_
