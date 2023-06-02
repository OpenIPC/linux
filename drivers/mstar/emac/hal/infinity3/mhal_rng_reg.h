////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2009-2010 MStar Semiconductor, Inc.
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

///////////////////////////////////////////////////////////////////////////////////////////////////
///
/// file    regRNG.h
/// @brief  Random Number Generation Registers Definition
/// @author MStar Semiconductor Inc.
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _MHAL_RNG_REG_H_
#define _MHAL_RNG_REG_H_

// unit: ms< --> ( HZ / 1000 )
#define InputRNGJiffThreshold       (10 * ( HZ / 1000 ))
#define RIU_MAP                     0xFD200000
#define RIU                         ((unsigned short volatile *) RIU_MAP)
#define REG_MIPS_BASE               (0x1D00)
#define MIPS_REG(addr)              RIU[(addr<<1)+REG_MIPS_BASE]
#define REG_RNG_OUT                 0x0e

#endif
