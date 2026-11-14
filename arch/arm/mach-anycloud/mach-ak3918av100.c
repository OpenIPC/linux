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

#define AK_CPU_ID           (AK_VA_SYSCTRL + 0x00)

#define AKCPU_VALUE         0x535335
#define AKCPU_VALUE_C       0x53533501
#define AKCPU_TYPE          "AK3918AV100-A"
#define AKCPU_TYPE_C        "AK3918AV100-C"
#define AKCPU_VAL_SHIFT     (8)
#define AKCPU_VAL_MASK      (0xFFFFFF)

#define IODESC_ENT(x)                           \
{                                               \
    .virtual = (unsigned long)AK_VA_##x,        \
    .pfn     = __phys_to_pfn(AK_PA_##x),        \
    .length  = AK_SZ_##x,                       \
    .type    = MT_DEVICE                        \
}

static struct map_desc ak3918av100_iodesc[] __initdata = {
    IODESC_ENT(SYSCTRL),
    IODESC_ENT(SUBCTRL),
    IODESC_ENT(L2MEM),
    IODESC_ENT(L2_1_MEM),
    IODESC_ENT(RESERVED_MEM),
    IODESC_ENT(RAM_CONTROLLER),
};

void __init ak3918av100_map_io(void)
{
    unsigned long regval = 0x0;

    /* initialise the io descriptors we need for initialisation */
    iotable_init(ak3918av100_iodesc, ARRAY_SIZE(ak3918av100_iodesc));

    /* determine the chip version*/
    regval = __raw_readl(AK_CPU_ID);
    if(regval == AKCPU_VALUE_C){
        regval = (regval >> AKCPU_VAL_SHIFT) & AKCPU_VAL_MASK;
        pr_info("ANYKA CPU %s (ID 0x%lx)\n", AKCPU_TYPE_C, regval);
    }else{
        regval = (regval >> AKCPU_VAL_SHIFT) & AKCPU_VAL_MASK;
        if (regval == AKCPU_VALUE)
            pr_info("ANYKA CPU %s (ID 0x%lx)\n", AKCPU_TYPE, regval);
        else
            pr_info("Unknown ANYKA CPU ID: 0x%lx\n", regval);
    }
}

void wdt_enable(void);
void wdt_keepalive(unsigned int heartbeat);

static void ak3918av100_restart(enum reboot_mode mode, const char *cmd)
{
#if defined CONFIG_AK_WATCHDOG_TOP
    //wdt_enable();
    //wdt_keepalive(2);
    pr_err("l2_exec_buf ...\n");
    local_irq_disable();
    SPECIFIC_L2BUF_EXEC(standby, 0,0,0,0);
#endif
}

static void __init ak3918av100_init(void)
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


static const char * const ak3918av100_dt_compat[] = {
    "anyka,ak3918av100",
    NULL
};

DT_MACHINE_START(AK3918AV100, "AK3918AV100")
/* Maintainer: Anyka(Guangzhou) Microelectronics Technology Co., Ltd */
    .dt_compat  = ak3918av100_dt_compat,
    .map_io = ak3918av100_map_io,
    .init_time = NULL,
    .init_machine = ak3918av100_init,
    .init_early = NULL,
    .reserve = NULL,
    .restart = ak3918av100_restart,
MACHINE_END
