/*
* ms_usb.c- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: raul.wang <raul.wang@sigmastar.com.tw>
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
#include <generated/autoconf.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/resource.h>
#include <linux/platform_device.h>
#include <linux/platform_data/ms_usb.h>
#include <cedric/irqs.h>
#include "ms_platform.h"
#include "cedric/irqs.h"
#include <mstar/mpatch_macro.h>

#if defined(CONFIG_MSTAR_AMBER3) || defined(CONFIG_MSTAR_CEDRIC)
#define ENABLE_THIRD_EHC
#endif

#define UTMI_BASE_ADDRESS_START		0xFD207500
#define UTMI_BASE_ADDRESS_END		0xFD2075FC
#define USB_HOST20_ADDRESS_START	0xFD204800
#define USB_HOST20_ADDRESS_END		0xFD2049FC

#define SECOND_UTMI_BASE_ADDRESS_START	0xFD207400
#define SECOND_UTMI_BASE_ADDRESS_END	0xFD2074FC
#define SECOND_USB_HOST20_ADDRESS_START	0xFD201A00
#define SECOND_USB_HOST20_ADDRESS_END	0xFD201BFC

#define THIRD_UTMI_BASE_ADDRESS_START	0xFD207200
#define THIRD_UTMI_BASE_ADDRESS_END		0xFD2072FC
#define THIRD_USB_HOST20_ADDRESS_START	0xFD227200
#define THIRD_USB_HOST20_ADDRESS_END	0xFD2273FC

static struct resource Sstar_usb_ehci_resources[] = {
	[0] = {
		.start		= UTMI_BASE_ADDRESS_START,
		.end		= UTMI_BASE_ADDRESS_END,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= USB_HOST20_ADDRESS_START,
		.end		= USB_HOST20_ADDRESS_END,
		.flags		= IORESOURCE_MEM,
	},
	[2] = {
		.start		= INT_IRQ_UHC,
		.end		= INT_IRQ_UHC,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct resource Second_Sstar_usb_ehci_resources[] = {
	[0] = {
		.start		= SECOND_UTMI_BASE_ADDRESS_START,
		.end		= SECOND_UTMI_BASE_ADDRESS_END,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= SECOND_USB_HOST20_ADDRESS_START,
		.end		= SECOND_USB_HOST20_ADDRESS_END,
		.flags		= IORESOURCE_MEM,
	},
	[2] = {
		.start		= INT_IRQ_UHC1,
		.end		= INT_IRQ_UHC1,
		.flags		= IORESOURCE_IRQ,
	},
};

#ifdef ENABLE_THIRD_EHC
static struct resource Third_Sstar_usb_ehci_resources[] = {
	[0] = {
		.start		= THIRD_UTMI_BASE_ADDRESS_START,
		.end		= THIRD_UTMI_BASE_ADDRESS_END,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= THIRD_USB_HOST20_ADDRESS_START,
		.end		= THIRD_USB_HOST20_ADDRESS_END,
		.flags		= IORESOURCE_MEM,
	},
	[2] = {
		.start		= INT_IRQ_UHC2,
		.end		= INT_IRQ_UHC2,
		.flags		= IORESOURCE_IRQ,
	},
};
#endif

/* The dmamask must be set for EHCI to work */
static u64 ehci_dmamask = ~(u32)0;

static struct platform_device Sstar_usb_ehci_device = {
	.name           = "Sstar-ehci-1",
	.id             = 0,
	.dev = {
		.dma_mask		= &ehci_dmamask,
		.coherent_dma_mask	= 0xffffffff, // for limit DMA range
	},
	.num_resources	= ARRAY_SIZE(Sstar_usb_ehci_resources),
	.resource	= Sstar_usb_ehci_resources,
};

static struct platform_device Second_Sstar_usb_ehci_device = {
	.name		= "Sstar-ehci-2",
	.id		= 1,
	.dev = {
		.dma_mask		= &ehci_dmamask,
		.coherent_dma_mask	= 0xffffffff,    // for limit DMA range
	},
	.num_resources	= ARRAY_SIZE(Second_Sstar_usb_ehci_resources),
	.resource	= Second_Sstar_usb_ehci_resources,
};

#ifdef ENABLE_THIRD_EHC
static struct platform_device Third_Sstar_usb_ehci_device = {
	.name		= "Sstar-ehci-3",
	.id		= 2,
	.dev = {
		.dma_mask		= &ehci_dmamask,
		.coherent_dma_mask	= 0xffffffff,    // for limit DMA range
	},
	.num_resources	= ARRAY_SIZE(Third_Sstar_usb_ehci_resources),
	.resource	= Third_Sstar_usb_ehci_resources,
};
#endif

static struct platform_device *Sstar_platform_devices[] = {
	&Sstar_usb_ehci_device,
	&Second_Sstar_usb_ehci_device,   // for 2st EHCI
#ifdef ENABLE_THIRD_EHC
	&Third_Sstar_usb_ehci_device,
#endif
};

int Sstar_ehc_platform_init(void)
{
	printk("[USB] add host platform dev....\n");
	return platform_add_devices(Sstar_platform_devices, ARRAY_SIZE(Sstar_platform_devices));
}

/* The following code is for OTG implementation */
#define USB_USBC_ADDRESS_START	0xFD200E00
#define USB_USBC_ADDRESS_END	0xFD200EFC
#define USB_MOTG_ADDRESS_START	0xFD2A2000
#define USB_MOTG_ADDRESS_END	0xFD2A23FC

static struct resource ms_otg_device_resource[] =
{
	[0] = {
		.start = USB_USBC_ADDRESS_START,
		.end   = USB_USBC_ADDRESS_END,
		.name  = "usbc-base",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = USB_HOST20_ADDRESS_START,
		.end   = USB_HOST20_ADDRESS_END,
		.name  = "uhc-base",
		.flags = IORESOURCE_MEM,
	},
	[2] = {
		.start = USB_MOTG_ADDRESS_START,
		.end   = USB_MOTG_ADDRESS_END,
		.name  = "motg-base",
		.flags = IORESOURCE_MEM,
	},
	[3] = {
		.start = INT_IRQ_USB,
		.end   = INT_IRQ_USB,
		.name  = "usb-int",
		.flags = IORESOURCE_IRQ,
	},
	[4] = {
		.start = UTMI_BASE_ADDRESS_START,
		.end   = UTMI_BASE_ADDRESS_END,
		.name  = "utmi-base",
		.flags = IORESOURCE_MEM,
	},
	[5] = {
		.start = INT_IRQ_UHC,
		.end   = INT_IRQ_UHC,
		.name  = "uhc-int",
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device Sstar_otg_device =
{
	.name             = "mstar-otg",
	.id               = -1,
	.num_resources    = ARRAY_SIZE(ms_otg_device_resource),
	.resource         = ms_otg_device_resource,
};

/* MST154A-D02A0-S utilize GPIO3 to control VBUS */
#define MSTAR_CEDRIC_RIU_BASE 0xFD200000
int ms_BD154A_set_vbus(unsigned int on)
{
	printk("[OTG] ms_BD154A_set_vbus %d\n", on);

	*((u16 volatile*)(MSTAR_CEDRIC_RIU_BASE+(0x1A13<<1))) = *((u16 volatile*)(MSTAR_CEDRIC_RIU_BASE+(0x1A13<<1))) & (u16)0xfff7;
	*((u16 volatile*)(MSTAR_CEDRIC_RIU_BASE+(0x1A15<<1))) = *((u16 volatile*)(MSTAR_CEDRIC_RIU_BASE+(0x1A15<<1))) & (u16)0xfff7;
	*((u16 volatile*)(MSTAR_CEDRIC_RIU_BASE+(0x2B0C<<1))) = *((u16 volatile*)(MSTAR_CEDRIC_RIU_BASE+(0x2B0C<<1))) & (u16)0xfffd;

	if (on)
		*((u16 volatile*)(MSTAR_CEDRIC_RIU_BASE+(0x2B0C<<1))) = *((u16 volatile*)(MSTAR_CEDRIC_RIU_BASE+(0x2B0C<<1))) | (u16)0x0001;
	else
		*((u16 volatile*)(MSTAR_CEDRIC_RIU_BASE+(0x2B0C<<1))) = *((u16 volatile*)(MSTAR_CEDRIC_RIU_BASE+(0x2B0C<<1))) & (u16)0xfffe;
	return on;
}

struct ms_usb_platform_data ms_otg_platform_data =
{
	.mode = MS_USB_MODE_OTG,
	.set_vbus = ms_BD154A_set_vbus,
};

int Sstar_otg_platform_init(void)
{
	printk("[OTG] Sstar_otg_device_init\n");
	Sstar_otg_device.dev.platform_data = &ms_otg_platform_data;

	return platform_device_register(&Sstar_otg_device);
}

arch_initcall(Sstar_ehc_platform_init);
arch_initcall(Sstar_otg_platform_init);


