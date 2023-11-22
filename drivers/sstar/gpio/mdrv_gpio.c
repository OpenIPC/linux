/*
 * mdrv_gpio.c- Sigmastar
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
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <asm/io.h>

#include "mdrv_gpio.h"
#include "mhal_gpio.h"

//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
/// GPIO chiptop initialization
/// @return None
/// @note   Called only once at system initialization
//-------------------------------------------------------------------------------------------------
void MDrv_GPIO_Init(void)
{
    MHal_GPIO_Init();
}

//-------------------------------------------------------------------------------------------------
/// select one pad to set to GPIO mode
/// @param  u8IndexGPIO              \b IN:  pad index
/// @return None
/// @note
//-------------------------------------------------------------------------------------------------
void MDrv_GPIO_Pad_Set(U8 u8IndexGPIO)
{
    MHal_GPIO_Pad_Set(u8IndexGPIO);
}

//-------------------------------------------------------------------------------------------------
/// set the specified pad mode( a set of GPIO pad will be effected)
/// @param  u8PadMode              \b IN:  pad mode
/// @return 0: success; 1: fail or not supported
/// @note
//-------------------------------------------------------------------------------------------------
U8 MDrv_GPIO_PadGroupMode_Set(U32 u32PadMode)
{
    return MHal_GPIO_PadGroupMode_Set(u32PadMode);
}

//-------------------------------------------------------------------------------------------------
/// set a pad to the specified mode
/// @param  u8PadMode              \b IN:  pad mode
/// @return 0: success; 1: fail or not supported
/// @note
//-------------------------------------------------------------------------------------------------
U8 MDrv_GPIO_PadVal_Set(U8 u8IndexGPIO, U32 u32PadMode)
{
    return MHal_GPIO_PadVal_Set(u8IndexGPIO, u32PadMode);
}

//-------------------------------------------------------------------------------------------------
/// set a pad to the specified mode
/// @param  u8PadMode              \b IN:  pad mode
/// @return 0: success; 1: fail or not supported
/// @note
//-------------------------------------------------------------------------------------------------
U8 MDrv_GPIO_PadVal_Get(U8 u8IndexGPIO, U32* u32PadMode)
{
    return MHal_GPIO_PadVal_Get(u8IndexGPIO, u32PadMode);
}

//-------------------------------------------------------------------------------------------------
/// set a mode to IO voltage
///@param  u8Group  u32Mode
///@return 1
///@note only for I7 , 3.3V is not allowed when MODE = 1
//-------------------------------------------------------------------------------------------------
void MDrv_GPIO_VolVal_Set(U8 u8Group, U32 u32Mode)
{
    MHal_GPIO_VolVal_Set(u8Group, u32Mode);
}

//-------------------------------------------------------------------------------------------------
/// set a pad to the specified mode
/// @param  u8PadMode              \b IN:  pad mode
/// @return 0: success; 1: fail or not supported
/// @note
//-------------------------------------------------------------------------------------------------
U8 MDrv_GPIO_PadVal_Check(U8 u8IndexGPIO, U32 u32PadMode)
{
    return MHal_GPIO_PadVal_Check(u8IndexGPIO, u32PadMode);
}

//-------------------------------------------------------------------------------------------------
/// enable output for selected one pad
/// @param  u8IndexGPIO              \b IN:  pad index
/// @return 0: success, return 1: GPIO_Index out of bounds
/// @note
//-------------------------------------------------------------------------------------------------
U8 MDrv_GPIO_Pad_Oen(U8 u8IndexGPIO)
{
    return MHal_GPIO_Pad_Oen(u8IndexGPIO);
}

//-------------------------------------------------------------------------------------------------
/// enable input for selected one pad
/// @param  u8IndexGPIO              \b IN:  pad index
/// @return 0: success, return 1: GPIO_Index out of bounds
/// @note
//-------------------------------------------------------------------------------------------------
U8 MDrv_GPIO_Pad_Odn(U8 u8IndexGPIO)
{
    return MHal_GPIO_Pad_Odn(u8IndexGPIO);
}

//-------------------------------------------------------------------------------------------------
/// read data from selected one pad
/// @param  u8IndexGPIO              \b IN:  pad index
/// @return 0: success, return 1: fail, GPIO_Index out of bounds
/// @note
//-------------------------------------------------------------------------------------------------
U8 MDrv_GPIO_Pad_Read(U8 u8IndexGPIO, U8* u8PadLevel)
{
    return MHal_GPIO_Pad_Level(u8IndexGPIO, u8PadLevel);
}

//-------------------------------------------------------------------------------------------------
/// read pad direction for selected one pad
/// @param  u8IndexGPIO              \b IN:  pad index
/// @return 0: success, return 1: fail, GPIO_Index out of bounds
/// @note
//-------------------------------------------------------------------------------------------------
U8 MDrv_GPIO_Pad_InOut(U8 u8IndexGPIO, U8* u8PadInOut)
{
    return MHal_GPIO_Pad_InOut(u8IndexGPIO, u8PadInOut);
}

//-------------------------------------------------------------------------------------------------
/// output pull high for selected one pad
/// @param  u8IndexGPIO              \b IN:  pad index
/// @return 0: success, return 1: fail, GPIO_Index out of bounds
/// @note
//-------------------------------------------------------------------------------------------------
U8 MDrv_GPIO_Pull_High(U8 u8IndexGPIO)
{
    return MHal_GPIO_Pull_High(u8IndexGPIO);
}

//-------------------------------------------------------------------------------------------------
/// output pull low for selected one pad
/// @param  u8IndexGPIO              \b IN:  pad index
/// @return 0: success, return 1: fail, GPIO_Index out of bounds
/// @note
//-------------------------------------------------------------------------------------------------
U8 MDrv_GPIO_Pull_Low(U8 u8IndexGPIO)
{
    return MHal_GPIO_Pull_Low(u8IndexGPIO);
}

//-------------------------------------------------------------------------------------------------
/// output pull up for selected one pad
/// @param  u8IndexGPIO              \b IN:  pad index
/// @return 0 success, return 1: this GPIO unsupport pull up or GPIO_Index out of bounds
/// @note
//-------------------------------------------------------------------------------------------------
U8 MDrv_GPIO_Pull_Up(U8 u8IndexGPIO)
{
    return MHal_GPIO_Pull_Up(u8IndexGPIO);
}

//-------------------------------------------------------------------------------------------------
/// output pull down for selected one pad
/// @param  u8IndexGPIO              \b IN:  pad index
/// @return 0 success, return 1: this GPIO unsupport pull down or GPIO_Index out of bounds
/// @note
//-------------------------------------------------------------------------------------------------
U8 MDrv_GPIO_Pull_Down(U8 u8IndexGPIO)
{
    return MHal_GPIO_Pull_Down(u8IndexGPIO);
}

//-------------------------------------------------------------------------------------------------
/// output pull off for selected one pad
/// @param  u8IndexGPIO              \b IN:  pad index
/// @return 0 success, return 1: this GPIO unsupport pull off or GPIO_Index out of bounds
/// @note
//-------------------------------------------------------------------------------------------------
U8 MDrv_GPIO_Pull_Off(U8 u8IndexGPIO)
{
    return MHal_GPIO_Pull_Off(u8IndexGPIO);
}

//-------------------------------------------------------------------------------------------------
/// get the gpio pull status
/// @param  u8IndexGPIO              \b IN:  pad index
/// @return 0 success, return 1: this GPIO unsupport pull or GPIO_Index out of bounds
/// @note
//-------------------------------------------------------------------------------------------------
U8 MDrv_GPIO_Pull_Status(U8 u8IndexGPIO, U8* u8PullStatus)
{
    return MHal_GPIO_Pull_status(u8IndexGPIO, u8PullStatus);
}

//-------------------------------------------------------------------------------------------------
/// output set high for selected one pad
/// @param  u8IndexGPIO              \b IN:  pad index
/// @return 0 success, return 1: GPIO_Index out of bounds
/// @note
//-------------------------------------------------------------------------------------------------
U8 MDrv_GPIO_Set_High(U8 u8IndexGPIO)
{
    return MHal_GPIO_Set_High(u8IndexGPIO);
}

//-------------------------------------------------------------------------------------------------
/// output set low for selected one pad
/// @param  u8IndexGPIO              \b IN:  pad index
/// @return 0 success, return 1: GPIO_Index out of bounds
/// @note
//-------------------------------------------------------------------------------------------------
U8 MDrv_GPIO_Set_Low(U8 u8IndexGPIO)
{
    return MHal_GPIO_Set_Low(u8IndexGPIO);
}

//-------------------------------------------------------------------------------------------------
/// output set driving for selected one pad
/// @param  u8IndexGPIO              \b IN:  pad index
/// @return 0: success, return 1: this GPIO unsupport set driving or GPIO_Index out of bounds
/// @note
//-------------------------------------------------------------------------------------------------
U8 MDrv_GPIO_Drv_Set(U8 u8IndexGPIO, U8 u8Level)
{
    return MHal_GPIO_Drv_Set(u8IndexGPIO, u8Level);
}

//-------------------------------------------------------------------------------------------------
/// get driving for selected one pad
/// @param  u8IndexGPIO              \b IN:  pad index
/// @return 0: success, return 1: this GPIO unsupport set driving or GPIO_Index out of bounds
/// @note
//-------------------------------------------------------------------------------------------------
U8 MDrv_GPIO_Drv_Get(U8 u8IndexGPIO, U8* u8Level)
{
    return MHal_GPIO_Drv_Get(u8IndexGPIO, u8Level);
}

//-------------------------------------------------------------------------------------------------
/// enable GPIO int for selected one pad
/// @param  u8IndexGPIO              \b IN:  pad index
/// @return None
/// @note
//-------------------------------------------------------------------------------------------------
int MDrv_GPIO_To_Irq(U8 u8IndexGPIO)
{
    return MHal_GPIO_To_Irq(u8IndexGPIO);
}

//-------------------------------------------------------------------------------------------------
/// get GPIO check info count
/// @param  None                     \b IN:  None
/// @return GPIO check info count
/// @note
//-------------------------------------------------------------------------------------------------
U8 MDrv_GPIO_Get_CheckCount(void)
{
    return MHal_GPIO_Get_CheckCount();
}

//-------------------------------------------------------------------------------------------------
/// get GPIO check info point
/// @param  u8Index                  \b IN:  Check info index
/// @return GPIO check info point
/// @note
//-------------------------------------------------------------------------------------------------
void* MDrv_GPIO_Get_CheckInfo(U8 u8Index)
{
    return MHal_GPIO_Get_CheckInfo(u8Index);
}

//-------------------------------------------------------------------------------------------------
/// return GPIO number index according to pad name
/// @param  pu8Name                  \b IN:  pad name to transform
/// @return GPIO number index
/// @note
//-------------------------------------------------------------------------------------------------
U8 MDrv_GPIO_NameToNum(U8* pu8Name, U8* GpioIndex)
{
    return MHal_GPIO_NameToNum(pu8Name, GpioIndex);
}

//-------------------------------------------------------------------------------------------------
/// return pinmux mode index according to pad name
/// @param  pu8Name                  \b IN:  pad name to transform
/// @return pinmux mode index
/// @note
//-------------------------------------------------------------------------------------------------
U8 MDrv_GPIO_PadModeToVal(U8* pu8Mode, U8* u8Index)
{
    return MHal_GPIO_PadModeToVal(pu8Mode, u8Index);
}

//-------------------------------------------------------------------------------------------------
/// get pad index from pad mode
/// @param  U32 u32Mode              \b IN:  pad mode
/// @return pad array
/// @note
//-------------------------------------------------------------------------------------------------
U32* MDrv_GPIO_PadModeToPadIndex(U32 u32Mode)
{
    return MHal_GPIO_PadModeToPadIndex(u32Mode);
}

//-------------------------------------------------------------------------------------------------
/// set gpio debounce length in nanosecond
/// @param  u8IndexGPIO: pad index
///         debounce: Waveform width, in nanoseconds
/// @return pinmux mode index
/// @note
//-------------------------------------------------------------------------------------------------
int MDrv_GPIO_Set_Debounce(U8 u8IndexGPIO, U32 debounce)
{
    return MHal_GPIO_Set_Debounce(u8IndexGPIO, debounce);
}

EXPORT_SYMBOL(MDrv_GPIO_Init);
EXPORT_SYMBOL(MDrv_GPIO_Pad_Set);
EXPORT_SYMBOL(MDrv_GPIO_PadGroupMode_Set);
EXPORT_SYMBOL(MDrv_GPIO_PadVal_Set);
EXPORT_SYMBOL(MDrv_GPIO_PadVal_Get);
EXPORT_SYMBOL(MDrv_GPIO_VolVal_Set);
EXPORT_SYMBOL(MDrv_GPIO_PadVal_Check);
EXPORT_SYMBOL(MDrv_GPIO_Pad_Oen);
EXPORT_SYMBOL(MDrv_GPIO_Pad_Odn);
EXPORT_SYMBOL(MDrv_GPIO_Pad_Read);
EXPORT_SYMBOL(MDrv_GPIO_Pad_InOut);
EXPORT_SYMBOL(MDrv_GPIO_Pull_High);
EXPORT_SYMBOL(MDrv_GPIO_Pull_Low);
EXPORT_SYMBOL(MDrv_GPIO_Pull_Up);
EXPORT_SYMBOL(MDrv_GPIO_Pull_Down);
EXPORT_SYMBOL(MDrv_GPIO_Pull_Off);
EXPORT_SYMBOL(MDrv_GPIO_Pull_Status);
EXPORT_SYMBOL(MDrv_GPIO_Set_High);
EXPORT_SYMBOL(MDrv_GPIO_Set_Low);
EXPORT_SYMBOL(MDrv_GPIO_Drv_Set);
EXPORT_SYMBOL(MDrv_GPIO_Drv_Get);
EXPORT_SYMBOL(MDrv_GPIO_To_Irq);
EXPORT_SYMBOL(MDrv_GPIO_Get_CheckCount);
EXPORT_SYMBOL(MDrv_GPIO_Get_CheckInfo);
EXPORT_SYMBOL(MDrv_GPIO_NameToNum);
EXPORT_SYMBOL(MDrv_GPIO_PadModeToVal);
EXPORT_SYMBOL(MDrv_GPIO_PadModeToPadIndex);
EXPORT_SYMBOL(MDrv_GPIO_Set_Debounce);
