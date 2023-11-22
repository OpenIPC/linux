/*
 * usb_patch_sstar.h- Sigmastar
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
#ifndef _USB_PATCH_SSTAR_H_
#define _USB_PATCH_SSTAR_H_

#define SSTAR_MIU_BUS_BASE_NONE 0xFFFFFFFF

#define SSTAR_MIU0_BUS_BASE 0x20000000
#define SSTAR_MIU1_BUS_BASE SSTAR_MIU_BUS_BASE_NONE

//=========================== Module:USB=======================================

#ifdef CONFIG_USB_SSTAR_PATCH_DEBUG
#define USB_SSTAR_PATCH_DEBUG 1
#endif

//=========================== Module:USB=======================================
#ifdef CONFIG_SSTAR_USB_STR_PATCH
#define SSTAR_USB_STR_PATCH 1
#endif

#ifdef CONFIG_SSTAR_USB_STR_PATCH_DEBUG
#define SSTAR_USB_STR_PATCH_DEBUG 1
#endif

#endif /* _USB_PATCH_SSTAR_H_ */
