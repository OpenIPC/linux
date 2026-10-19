/*
 * /linux/arch/arm/mach-anycloud/mach-av3918av100.c
 *
 * Copyright (C) 2021 Guangzhou Anyka Microelectronics Co., Ltd.
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
#include <mach/ak_cache_count.h>

#include <linux/input.h>
#include <linux/irqchip.h>
#include <mach/ak_l2_exebuf.h>

#define AK_CPU_ID           (AK_VA_SYSCTRL + 0x3FC)

#define AKCPU_VALUE_A       0x56573500
#define AKCPU_VALUE_B       0x56573502
#define AKCPU_TYPE_A        "AK3918AV130_A"
#define AKCPU_TYPE_B        "AK3918AV130_B"

#define IODESC_ENT(x)                           \
{                                               \
    .virtual = (unsigned long)AK_VA_##x,        \
    .pfn     = __phys_to_pfn(AK_PA_##x),        \
    .length  = AK_SZ_##x,                       \
    .type    = MT_DEVICE                        \
}

static struct map_desc ak3918av130_iodesc[] __initdata = {
    IODESC_ENT(SYSCTRL),
    IODESC_ENT(SUBCTRL),
    IODESC_ENT(L2MEM),
    IODESC_ENT(L2_1_MEM),
    IODESC_ENT(RESERVED_MEM),
    IODESC_ENT(RAM_CONTROLLER),
    IODESC_ENT(INT_TIMER),
    IODESC_ENT(CPUL_MEM),
};

void __init ak3918av130_map_io(void)
{
    unsigned long regval = 0x0;

    /* initialise the io descriptors we need for initialisation */
    iotable_init(ak3918av130_iodesc, ARRAY_SIZE(ak3918av130_iodesc));

    regval = __raw_readl(AK_CPU_ID);
    if (regval == AKCPU_VALUE_A)
        pr_info("ANYKA CPU %s (ID 0x%lx)\n", AKCPU_TYPE_A, regval);
    else if (regval == AKCPU_VALUE_B)
        pr_info("ANYKA CPU %s (ID 0x%lx)\n", AKCPU_TYPE_B, regval);
    else
        pr_info("Unknown ANYKA CPU ID: 0x%lx\n", regval);

}

void wdt_enable(void);
void wdt_keepalive(unsigned int heartbeat);

static void ak3918av130_restart(enum reboot_mode mode, const char *cmd)
{
#if defined CONFIG_AK_WATCHDOG_TOP
    // wdt_enable();
    // wdt_keepalive(2);
    printk(KERN_ERR"%s.,line:%d..\n", __func__,__LINE__);
    SPECIFIC_L2BUF_EXEC(standby,0,1,0,0,0);
#endif
}

static void __init ak3918av130_init(void)
{
    int ret;

    ret = of_platform_populate(NULL, of_default_bus_match_table, NULL,
                   NULL);
    if (ret) {
        pr_err("of_platform_populate failed: %d\n", ret);
        BUG();
    }

    ak_cache_hit_miss_counting_init();

    l2_init();
    return;
}


static const char * const ak3918av130_dt_compat[] = {
    "anyka,ak3918av130",
    NULL
};

DT_MACHINE_START(AK3918av130, "AK3918AV130")
/* Maintainer: Anyka(Guangzhou) Microelectronics Technology Co., Ltd */
    .dt_compat  = ak3918av130_dt_compat,
    .map_io = ak3918av130_map_io,
    .init_time = NULL,
    .init_machine = ak3918av130_init,
    .init_early = NULL,
    .reserve = NULL,
    .restart = ak3918av130_restart,
MACHINE_END
