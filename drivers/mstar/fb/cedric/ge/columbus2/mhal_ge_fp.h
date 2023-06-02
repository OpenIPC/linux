////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2007 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (¡§MStar Confidential Information¡¨) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////


#ifndef DRV_FIXEDPOINT_H
#define DRV_FIXEDPOINT_H

#include "mhal_ge_reg.h"


extern U32 Divide2Fixed(U16 u16x, U16 u16y, U8 nInterger, U8 nFraction);

#if 0
extern void FixPLine_S1_11(MS_U16 u16x, MS_U16 u16y, LONG16_BYTE* pu16ret);
extern void FixPClr_S8_11(MS_S16 s8color, MS_U16 u16dis, LONG32_BYTE* pu32ret);
extern void FixPBlt_S1_15(MS_U16 u16src, MS_U16 u16dst, LONG16_BYTE* pu16ret);
#endif

#endif
