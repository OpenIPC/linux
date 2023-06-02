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

#ifndef _HAL_GPIO_H_
#define _HAL_GPIO_H_

#include <asm/types.h>
#include "mdrv_types.h"

//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------
//#define MASK(x)     (((1<<(x##_BITS))-1) << x##_SHIFT)
//#define BIT(_bit_)                  (1 << (_bit_))
#define BIT_(x)                     BIT(x) //[OBSOLETED] //TODO: remove it later
#define BITS(_bits_, _val_)         ((BIT(((1)?_bits_)+1)-BIT(((0)?_bits_))) & (_val_<<((0)?_bits_)))
#define BMASK(_bits_)               (BIT(((1)?_bits_)+1)-BIT(((0)?_bits_)))


//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------
//the functions of this section set to initialize
extern void MHal_GPIO_Init(void);
extern void MHal_GPIO_Pad_Set(U8 u8IndexGPIO);
extern void MHal_GPIO_Pad_Oen(U8 u8IndexGPIO);
extern void MHal_GPIO_Pad_Odn(U8 u8IndexGPIO);
extern U8 MHal_GPIO_Pad_Level(U8 u8IndexGPIO);
extern U8 MHal_GPIO_Pad_InOut(U8 u8IndexGPIO);
extern void MHal_GPIO_Pull_High(U8 u8IndexGPIO);
extern void MHal_GPIO_Pull_Low(U8 u8IndexGPIO);
extern void MHal_GPIO_Set_High(U8 u8IndexGPIO);
extern void MHal_GPIO_Set_Low(U8 u8IndexGPIO);
extern void MHal_Enable_GPIO_INT(U8 u8IndexGPIO);
extern int MHal_GPIO_To_Irq(U8 u8IndexGPIO);
extern void MHal_GPIO_Set_POLARITY(U8 u8IndexGPIO,U8 reverse);
extern void MHal_GPIO_PAD_32K_OUT(U8 u8Enable);

#endif // _HAL_GPIO_H_

