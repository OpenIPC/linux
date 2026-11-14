/*
 * /linux/arch/arm/mach-anycloud/mach-ak39ev330.c
 *
 * Copyright (C) 2020 Anyka(Guangzhou) Microelectronics Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/of_platform.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach-types.h>
#include <asm/irq.h>
#include <asm/sizes.h>

#include <mach/ak_l2.h>
#include <mach/map.h>

#include <linux/input.h>
#include <linux/irqchip.h>

#define AK_CPU_ID           (AK_VA_SYSCTRL + 0x00)

#define AKCPU_VALUE         0x20160101
#define AKCPU_TYPE          "AK39XXEV330"

#define IODESC_ENT(x)                           \
{                                               \
    .virtual = (unsigned long)AK_VA_##x,        \
    .pfn     = __phys_to_pfn(AK_PA_##x),        \
    .length  = AK_SZ_##x,                       \
    .type    = MT_DEVICE                        \
}

static struct map_desc ak39ev330_iodesc[] __initdata = {
    IODESC_ENT(SYSCTRL),
    IODESC_ENT(CAMERA),
    IODESC_ENT(SUBCTRL),
    IODESC_ENT(MIPI1),
    IODESC_ENT(L2MEM),
    IODESC_ENT(RESERVED_MEM),
};

void __init ak39ev330_map_io(void)
{
    unsigned long regval = 0x0;
    
    /* initialise the io descriptors we need for initialisation */
    iotable_init(ak39ev330_iodesc, ARRAY_SIZE(ak39ev330_iodesc));

    regval = __raw_readl(AK_CPU_ID);
    if (regval == AKCPU_VALUE) 
        pr_info("ANYKA CPU %s (ID 0x%lx)\n", AKCPU_TYPE, regval);
    else
        pr_info("Unknown ANYKA CPU ID: 0x%lx\n", regval);
        
}

void wdt_enable(void);
void wdt_keepalive(unsigned int heartbeat);

static void ak39ev330_restart(enum reboot_mode mode, const char *cmd)
{
#if defined CONFIG_AK_WATCHDOG_TOP
    wdt_enable();
    wdt_keepalive(2);
#endif
}

static void __init ak39ev330_init(void)
{   
    int ret;
    l2_init();

    ret = of_platform_populate(NULL, of_default_bus_match_table, NULL,
                   NULL);
    if (ret) {
        pr_err("of_platform_populate failed: %d\n", ret);
        BUG();
    }    
    
    return;
}


static const char * const ak39ev330_dt_compat[] = {
    "anyka,ak3916ev330",
    "anyka,ak3918ev330",
    "anyka,ak3919ev330",
    "anyka,ak39ev330",
    "anyka,ak3916ev331",
    "anyka,ak3918ev331",
    "anyka,ak3919ev331",
    "anyka,ak39ev331",
    NULL
};

DT_MACHINE_START(AK39xxEV330, "AK39EV330")
/* Maintainer: Anyka(Guangzhou) Microelectronics Technology Co., Ltd */
    .dt_compat  = ak39ev330_dt_compat,
    .map_io = ak39ev330_map_io,
    .init_time = NULL,
    .init_machine = ak39ev330_init,
    .init_early = NULL,
    .reserve = NULL,
    .restart = ak39ev330_restart,
MACHINE_END
