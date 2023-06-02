/**
    NVT IO mapping
    To handle static io mapping
    @file       io.c
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <asm/mach/map.h>
#include <plat/hardware.h>
#include <mach/nvt-info.h>

static struct map_desc nvt_ivot_io_desc[] __initdata = {
	{
		.virtual	= (unsigned long) NVT_PERIPHERAL_VIRT_BASE,
		.pfn		= __phys_to_pfn(NVT_PERIPHERAL_PHYS_BASE),
		.length		= NVT_PERIPHERAL_SIZE,
		.type		= MT_DEVICE,
	},
#ifdef CONFIG_NVT_IVOT_PLAT_NA51068
	{
		.virtual	= (unsigned long) NVT_PERIPHERAL_1_VIRT_BASE,
		.pfn		= __phys_to_pfn(NVT_PERIPHERAL_1_PHYS_BASE),
		.length		= NVT_PERIPHERAL_1_SIZE,
		.type		= MT_DEVICE,
	},
	{
		.virtual	= (unsigned long) NVT_PERIPHERAL_2_VIRT_BASE,
		.pfn		= __phys_to_pfn(NVT_PERIPHERAL_2_PHYS_BASE),
		.length		= NVT_PERIPHERAL_2_SIZE,
		.type		= MT_DEVICE,
	},
#endif
};

/* physical address to virtual address space mapping */
void __init nvt_ivot_map_io(void)
{
	debug_ll_io_init();
	iotable_init(nvt_ivot_io_desc, ARRAY_SIZE(nvt_ivot_io_desc));

#ifdef CONFIG_NVT_INFO
	//assume to be the earliest kernel tag
	nvt_bootts_add_ts("ker");
#endif
}
