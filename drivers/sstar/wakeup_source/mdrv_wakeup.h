/*
 * mdrv_wakeup.h- Sigmastar
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
#ifndef __MDRV_WAKEUP_H
#define __MDRV_WAKEUP_H

#include "ms_types.h"
#include "ms_platform.h"

//#define BASE_REG_RIU_PA     (0x1F000000)
#define BASE_REG_CHIPTOP_PA (0x1F203C00)

#define BK_REG(reg) ((reg) << 2)

#define XTAL_GATE                 BK_REG(0x30)
#define REG_GATE_XTAL24_CORE      (BIT0)
#define REG_GATE_XTAL24_NODIE     (BIT1)
#define REG_GATE_XTAL12_CORE      (BIT2)
#define REG_GATE_XTAL12_NODIE     (BIT3)
#define REG_WAKEUP_XTAL_IRQ_CLR   (BIT5)
#define REG_WAKEUP_XTAL_IRQ       (BIT6)
#define REG_WAKEUP_XTAL_IRQ_EN    (BIT7)
#define REG_WAKEUP_XTAL_USB_EN    (BIT8)
#define REG_WAKEUP_XTAL_USB_EVENT (BIT12)

#define REG_WAKEUP_XTAL_GPIO_EN    BK_REG(0x32) // 0x32~0x36
#define REG_WAKEUP_XTAL_GPIO_EVENT BK_REG(0x38) // 0x38~0x3c
#endif
