////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2008-2009 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// ("MStar Confidential Information") by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _HAL_CHIP_H_
#define _HAL_CHIP_H_



#ifdef __cplusplus
extern "C"
{
#endif

//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------
#define ARM_CLOCK_FREQ             900000000
#define AEON_CLOCK_FREQ            240000000
#define XTAL_CLOCK_FREQ            12000000

#define HAL_MIU1_BASE               0x60000000 // 1512MB
#define HAL_MIU1_BUS_BASE           0xA0000000 // MIU1 Low 256MB
//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------
//void    CHIP_InitISR(void);
//MS_BOOL CHIP_InISRContext(void);
//MS_BOOL CHIP_AttachISR(InterruptNum eIntNum, InterruptCb pIntCb);
//MS_BOOL CHIP_DetachISR(InterruptNum eIntNum);
//MS_BOOL CHIP_EnableIRQ(InterruptNum eIntNum);
//MS_BOOL CHIP_DisableIRQ(InterruptNum eIntNum);
//MS_BOOL CHIP_EnableAllInterrupt(void);
//MS_BOOL CHIP_DisableAllInterrupt(void);



#ifdef __cplusplus
}
#endif

#endif // _HAL_CHIP_H_
