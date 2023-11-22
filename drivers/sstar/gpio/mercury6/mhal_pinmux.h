/*
 * mhal_pinmux.h- Sigmastar
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
#ifndef __MHAL_PINMUX_H__
#define __MHAL_PINMUX_H__

#include "mdrv_types.h"

extern S32   HalPadSetVal(U32 u32PadID, U32 u32Mode);
extern S32   HalPadCheckVal(U32 u32PadID, U32 u32Mode);
extern S32   HalPadSetMode(U32 u32Mode);
extern U8    HalPadCheckInfoCount(void);
extern void* HalPadCheckInfoGet(U8 u8Index);
extern int   HalPadModeToVal(U8* pu8Mode);
extern void  HalGPIOSetVol(U32 u32Group, U32 u32Mode);
extern U32*  HalPadModeToPadIndex(U32 u32Mode);

#endif // __MHAL_PINMUX_H__
