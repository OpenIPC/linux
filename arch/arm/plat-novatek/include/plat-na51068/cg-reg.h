/*
	Register definition header file for CG module of NA51055.

	This file is the header file that define the address offset
	definition of registers of CG module.

	@file       cg-reg.h

	Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
	published by the Free Software Foundation.
*/

#ifndef _CG_REG_H
#define _CG_REG_H

#include "hardware.h"

/* PLL */
#define CG_PLL_EN_REG_OFFSET                   0x00
#define CG_PLL_STAT_REG_OFFSET                 0x04

/* Module reset control */
#define CG_CPU_DSP_RESET_REG_OFFSET            0x4C
#define CG_MODULE_RESET_REG0_OFFSET            0x50
#define CG_MODULE_RESET_REG1_OFFSET            0x54

/* APB reset control */
#define CG_APB_RESET_REG0_OFFSET               0x58
#define CG_APB_RESET_REG1_OFFSET               0x5C

/* Bus clock enable/disable control */
#define CG_BUS_CLK_GATING_REG0_OFFSET          0x60
#define CG_BUS_CLK_GATING_REG1_OFFSET          0x64
#define CG_BUS_CLK_GATING_REG2_OFFSET          0x68
#define CG_BUS_CLK_GATING_REG3_OFFSET          0x6C

/* APB clock enable/disable control */
#define CG_APB_CLK_GATING_REG0_OFFSET          0x70
#define CG_APB_CLK_GATING_REG1_OFFSET          0x74
#define CG_APB_CLK_GATING_REG2_OFFSET          0x78

/* MAC Cofiguration */
#define CG_MAC_CONFIG_REG_OFFSET	       0xE4

/* Clock source and rate control */
#define CG_CPU_CKCTRL_REG_OFFSET               0x100
#define CG_H264D_CKCTRL_REG_OFFSET             0x158
#define CG_OSG_CKCTRL_REG_OFFSET               0x15C
#define CG_SSCA_CKCTRL_REG_OFFSET              0x160
#define CG_CRYPTO_CKCTRL_REG_OFFSET            0x164
#define CG_H265D_CKCTRL_REG_OFFSET             0x168
#define CG_VPE536_CKCTRL_REG_OFFSET            0x16C
#define CG_EXT_CKCTRL_REG_OFFSET               0x170
#define CG_LCD_CKCTRL_REG_OFFSET               0x174
#define CG_ETH_CKCTRL_REG_OFFSET               0x178
#define CG_VENC_CKCTRL_REG_OFFSET              0x17C
#define CG_HDMI_SPI_CKCTRL_REG_OFFSET          0x180
#define CG_SSP_CKCTRL_REG0_OFFSET              0x184
#define CG_SSP_CKCTRL_REG1_OFFSET              0x188
#define CG_SDC_UART_SSP_CKCTRL_REG_OFFSET      0x18C
#define CG_VCAP_CKCTRL_REG_OFFSET              0x190
#define CG_VPE316_CKCTRL_REG_OFFSET            0x194

/* PLL clock rate control */
#define CG_PLL0_CLK_RATE0_REG_OFFSET           0x10400
#define CG_PLL1_CLK_RATE0_REG_OFFSET           0x10440
#define CG_PLL2_CLK_RATE0_REG_OFFSET           0x10480
#define CG_PLL3_CLK_RATE0_REG_OFFSET           0x104C0
#define CG_PLL4_CLK_RATE0_REG_OFFSET           0x10500
#define CG_PLL5_CLK_RATE0_REG_OFFSET           0x10800
#define CG_PLL6_CLK_RATE0_REG_OFFSET           0x10540
#define CG_PLL7_CLK_RATE0_REG_OFFSET           0x10580
#define CG_PLL8_CLK_RATE0_REG_OFFSET           0x105C0
#define CG_PLL9_CLK_RATE0_REG_OFFSET           0x10600
#define CG_PLL10_CLK_RATE0_REG_OFFSET          0x10640
#define CG_PLL11_CLK_RATE0_REG_OFFSET          0x10680
#define CG_PLL12_CLK_RATE0_REG_OFFSET          0x106C0
#define CG_PLL13_CLK_RATE0_REG_OFFSET          0x10700
#define CG_PLL14_CLK_RATE0_REG_OFFSET          0x10740
#define CG_PLL15_CLK_RATE0_REG_OFFSET          0x10780
#define CG_PLL16_CLK_RATE0_REG_OFFSET          0x107C0

#endif
