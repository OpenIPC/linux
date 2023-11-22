/*
 * mhal_gpio.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
#ifndef _MHAL_GPIO_H_
#define _MHAL_GPIO_H_

#include <asm/types.h>
#include "mdrv_types.h"

//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------

extern void  MHal_GPIO_Init(void);
extern void  MHal_GPIO_Pad_Set(U8 u8IndexGPIO);
extern int   MHal_GPIO_PadGroupMode_Set(U32 u32PadMode);
extern int   MHal_GPIO_PadVal_Set(U8 u8IndexGPIO, U32 u32PadMode);
extern void  MHal_GPIO_VolVal_Set(U8 u8Group, U32 u32Mode);
extern int   MHal_GPIO_PadVal_Check(U8 u8IndexGPIO, U32 u32PadMode);
extern void  MHal_GPIO_Pad_Oen(U8 u8IndexGPIO);
extern void  MHal_GPIO_Pad_Odn(U8 u8IndexGPIO);
extern U8    MHal_GPIO_Pad_Level(U8 u8IndexGPIO);
extern U8    MHal_GPIO_Pad_InOut(U8 u8IndexGPIO);
extern void  MHal_GPIO_Pull_High(U8 u8IndexGPIO);
extern void  MHal_GPIO_Pull_Low(U8 u8IndexGPIO);
extern U8    MHal_GPIO_Pull_Up(U8 u8IndexGPIO);
extern U8    MHal_GPIO_Pull_Down(U8 u8IndexGPIO);
extern U8    MHal_GPIO_Pull_Off(U8 u8IndexGPIO);
extern void  MHal_GPIO_Set_High(U8 u8IndexGPIO);
extern void  MHal_GPIO_Set_Low(U8 u8IndexGPIO);
extern U8    MHal_GPIO_Drv_Set(U8 u8IndexGPIO, U8 u8Level);
extern U8    MHal_GPIO_Drv_Get(U8 u8IndexGPIO, U8* u8Level);
extern void  MHal_Enable_GPIO_INT(U8 u8IndexGPIO);
extern int   MHal_GPIO_To_Irq(U8 u8IndexGPIO);
extern void  MHal_GPIO_Set_POLARITY(U8 u8IndexGPIO, U8 reverse);
extern void  MHal_GPIO_PAD_32K_OUT(U8 u8Enable);
extern U8    MHal_GPIO_Get_CheckCount(void);
extern void* MHal_GPIO_Get_CheckInfo(U8 u8Index);
extern int   MHal_GPIO_NameToNum(U8* pu8Name);
extern int   MHal_GPIO_PadModeToVal(U8* pu8Mode);
extern int   GPI_GPIO_To_Irq(U8 u8IndexGPIO);
extern U32*  MHal_GPIO_PadModeToPadIndex(U32 u32Mode);

#endif // _MHAL_GPIO_H_
