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

///////////////////////////////////////////////////////////////////////////////////////////////////
///
/// @file   mdrv_gpio.h
/// @brief  GPIO Driver Interface
/// @author MStar Semiconductor Inc.
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _DRV_GPIO_H_
#define _DRV_GPIO_H_

#include <asm/types.h>
#include "mdrv_types.h"

//-------------------------------------------------------------------------------------------------
//  Driver Capability
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------
void MDrv_GPIO_Init(void);
void MDrv_GPIO_Pad_Set(U8 u8IndexGPIO);
void MDrv_GPIO_Pad_Oen(U8 u8IndexGPIO);
void MDrv_GPIO_Pad_Odn(U8 u8IndexGPIO);
U8 MDrv_GPIO_Pad_Read(U8 u8IndexGPIO);
U8 MDrv_GPIO_Pad_InOut(U8 u8IndexGPIO);
void MDrv_GPIO_Pull_High(U8 u8IndexGPIO);
void MDrv_GPIO_Pull_Low(U8 u8IndexGPIO);
void MDrv_GPIO_Set_High(U8 u8IndexGPIO);
void MDrv_GPIO_Set_Low(U8 u8IndexGPIO);
void MDrv_Enable_GPIO_INT(U8 u8IndexGPIO);
int MDrv_GPIO_To_Irq(U8 u8IndexGPIO);
void MDrv_GPIO_PAD_32K_OUT(U8 u8Enable);
void MDrv_GPIO_Set_POLARITY(U8 u8IndexGPIO,U8 reverse);

#endif // _DRV_GPIO_H_

