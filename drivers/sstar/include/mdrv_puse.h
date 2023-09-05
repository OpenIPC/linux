/*
* mdrv_puse.h- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: richard.guo <richard.guo@sigmastar.com.tw>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#ifndef __MDRV_PUSE_H__
#define __MDRV_PUSE_H__

// Don't need to specify pad usage by which driver
#define MDRV_PUSE_NA                            0x00000000

// EMAC0 pad usage
#define MDRV_PUSE_EMAC0_LED                     0x00010000
#define MDRV_PUSE_EMAC0_LED_GPIO_GREEN          0x00010001
#define MDRV_PUSE_EMAC0_LED_GPIO_ORANGE         0x00010002

// EMAC1 pad usage
#define MDRV_PUSE_EMAC1_LED                     0x00020000
#define MDRV_PUSE_EMAC1_LED_GPIO_GREEN          0x00020001
#define MDRV_PUSE_EMAC1_LED_GPIO_ORANGE         0x00020002

// SDIO0
#define MDRV_PUSE_SDIO0_XXX                     0x00030000
#define MDRV_PUSE_SDIO0_YYY                     0x00030001
#define MDRV_PUSE_SDIO0_ZZZ                     0x00030002

// SDIO1
#define MDRV_PUSE_SDIO1_XXX                     0x00040000
#define MDRV_PUSE_SDIO1_YYY                     0x00040001
#define MDRV_PUSE_SDIO1_ZZZ                     0x00040002

#endif // #define __MDRV_PUSE_H__
