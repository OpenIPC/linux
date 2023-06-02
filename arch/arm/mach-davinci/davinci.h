/*
 * This file contains the processor specific definitions
 * of the TI DM644x, DM355, DM365, and DM646x.
 *
 * Copyright (C) 2011 Texas Instruments Incorporated
 * Copyright (c) 2007 Deep Root Systems, LLC
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __DAVINCI_H
#define __DAVINCI_H

#include <linux/clk.h>
#include <linux/videodev2.h>
#include <linux/davinci_emac.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/platform_data/davinci_asp.h>
#include <linux/platform_data/edma.h>
#include <linux/platform_data/keyscan-davinci.h>
#include <mach/hardware.h>

#include <media/davinci/vpfe_types.h>
#include <media/davinci/vpfe_capture.h>
#include <video/davinci_osd.h>
#include <video/davinci_vpbe.h>


//#define DAVINCI_SYSTEM_MODULE_BASE	0x01c40000
#define SYSMOD_VDAC_CONFIG		0x2c
#define SYSMOD_VIDCLKCTL		0x38
#define SYSMOD_VPSS_CLKCTL		0x44
#define SYSMOD_VDD3P3VPWDN		0x48
#define SYSMOD_VSCLKDIS			0x6c
#define SYSMOD_PUPDCTL1			0x7c

/* VPSS CLKCTL bit definitions */
#define VPSS_MUXSEL_EXTCLK_ENABLE	BIT(1)
#define VPSS_VENCCLKEN_ENABLE		BIT(3)
#define VPSS_DACCLKEN_ENABLE		BIT(4)
#define VPSS_PLLC2SYSCLK5_ENABLE	BIT(5)

extern void __iomem *davinci_sysmod_base;
#define DAVINCI_SYSMOD_VIRT(x)	(davinci_sysmod_base + (x))
void davinci_map_sysmod(void);

/* DM355 base addresses */
#define DM355_ASYNC_EMIF_CONTROL_BASE	0x01e10000
#define DM355_ASYNC_EMIF_DATA_CE0_BASE	0x02000000

#define ASP1_TX_EVT_EN	1
#define ASP1_RX_EVT_EN	2

/* DM365 base addresses */
#define DM365_ASYNC_EMIF_CONTROL_BASE	0x01d10000
#define DM365_ASYNC_EMIF_DATA_CE0_BASE	0x02000000
#define DM365_ASYNC_EMIF_DATA_CE1_BASE	0x04000000





/* DM365 function declarations */
void __init dm365_init(void);
void __init dm365_init_asp(struct snd_platform_data *pdata);
void __init dm365_init_vc(struct snd_platform_data *pdata);
void __init dm365_init_rtc(void);


void dm365_set_vpfe_config(struct vpfe_config *cfg);
void __init davinci_setup_usb(unsigned mA, unsigned potpgt_ms);
#endif /* __ASM_ARCH_DM365_H */
