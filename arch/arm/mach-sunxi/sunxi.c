/*
 * Device Tree support for Allwinner A1X SoCs
 *
 * Copyright (C) 2012 Maxime Ripard
 *
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/clk-provider.h>
#include <linux/clocksource.h>
#include <linux/init.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/memblock.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mcpm.h>

#include "sunxi.h"
#include <linux/ion_sunxi.h>

void __iomem *sunxi_cpucfg_base;
void __iomem *sunxi_cpuscfg_base;
void __iomem *sunxi_sysctl_base;
void __iomem *sunxi_rtc_base;
void __iomem *sunxi_soft_entry_base;

static void __init sunxi_dt_cpufreq_init(void)
{
	platform_device_register_simple("cpufreq-dt", -1, NULL, 0);
}

static const char * const sunxi_board_dt_compat[] = {
	"allwinner,sun4i-a10",
	"allwinner,sun5i-a10s",
	"allwinner,sun5i-a13",
	"allwinner,sun5i-r8",
	NULL,
};

DT_MACHINE_START(SUNXI_DT, "Allwinner sun4i/sun5i Families")
	.dt_compat	= sunxi_board_dt_compat,
	.init_late	= sunxi_dt_cpufreq_init,
MACHINE_END

static const char * const sun6i_board_dt_compat[] = {
	"allwinner,sun6i-a31",
	"allwinner,sun6i-a31s",
	NULL,
};

extern void __init sun6i_reset_init(void);
static void __init sun6i_timer_init(void)
{
	of_clk_init(NULL);
	if (IS_ENABLED(CONFIG_RESET_CONTROLLER))
		sun6i_reset_init();
	clocksource_probe();
}

DT_MACHINE_START(SUN6I_DT, "Allwinner sun6i (A31) Family")
	.init_time	= sun6i_timer_init,
	.dt_compat	= sun6i_board_dt_compat,
	.init_late	= sunxi_dt_cpufreq_init,
MACHINE_END

static const char * const sun7i_board_dt_compat[] = {
	"allwinner,sun7i-a20",
	NULL,
};

DT_MACHINE_START(SUN7I_DT, "Allwinner sun7i (A20) Family")
	.dt_compat	= sun7i_board_dt_compat,
	.init_late	= sunxi_dt_cpufreq_init,
MACHINE_END

#define IO_ADDRESS(x)  ((x) + 0xf0000000)

static struct map_desc sunxi_io_desc[] __initdata = {
#ifdef CONFIG_ARCH_SUN8IW8P1
	{
		.virtual = (unsigned long) IO_ADDRESS(SUNXI_IO_PBASE),
		.pfn     = __phys_to_pfn(SUNXI_IO_PBASE),
		.length  = SUNXI_IO_SIZE,
		.type    = MT_DEVICE,
	},
#else
	{
		.virtual = (unsigned long) UARTIO_ADDRESS(SUNXI_UART_PBASE),
		.pfn     = __phys_to_pfn(SUNXI_UART_PBASE),
		.length  = SUNXI_UART_SIZE,
		.type    = MT_DEVICE,
	},
#endif
#ifdef CONFIG_ARCH_SUN8IW8P1
	{
		.virtual = (unsigned long)IO_ADDRESS(SUNXI_SRAM_A1_PBASE),
		.pfn     = __phys_to_pfn(SUNXI_SRAM_A1_PBASE),
		.length  = SUNXI_SRAM_A1_SIZE,
		.type    = MT_MEMORY_RWX_ITCM,
	},
	{
		.virtual = (unsigned long)IO_ADDRESS(SUNXI_SRAM_C_PBASE),
		.pfn     = __phys_to_pfn(SUNXI_SRAM_C_PBASE),
		.length  = SUNXI_SRAM_C_SIZE,
		.type    = MT_MEMORY_RWX_ITCM,
	},
#endif

#if defined(CONFIG_ARCH_SUN8IW10P1)
	{
		.virtual = (unsigned long)IO_ADDRESS(SUNXI_SRAM_A1_PBASE),
		.pfn     = __phys_to_pfn(SUNXI_SRAM_A1_PBASE),
		.length  = SUNXI_SRAM_A1_SIZE,
		.type    = MT_MEMORY_RWX_ITCM,
	},
	{
		.virtual = (unsigned long)IO_ADDRESS(SUNXI_SRAM_C_PBASE),
		.pfn     = __phys_to_pfn(SUNXI_SRAM_C_PBASE),
		.length  = SUNXI_SRAM_C_SIZE,
		.type    = MT_MEMORY_RWX_ITCM,
	},
#endif
#if defined(CONFIG_ARCH_SUN8IW6P1)
	{
		.virtual	= (unsigned long)IO_ADDRESS(SUNXI_SRAM_A1_PBASE),
		.pfn		= __phys_to_pfn(SUNXI_SRAM_A1_PBASE),
		.length		= SUNXI_SRAM_A1_SIZE,
		.type		= MT_MEMORY_RWX_ITCM,
	},

	{
		.virtual        = (unsigned long)IO_ADDRESS(SUNXI_SRAM_A2_PBASE),
		.pfn            = __phys_to_pfn(SUNXI_SRAM_A2_PBASE),
		.length         = SUNXI_SRAM_A2_SIZE,
		.type           = MT_DEVICE_NONSHARED,
	},

#endif
};

void __init sunxi_map_io(void)
{
	iotable_init(sunxi_io_desc, ARRAY_SIZE(sunxi_io_desc));
}

static struct platform_device sunxi_cpuidle = {
	.name = "sunxi_cpuidle",
};

static void __init sunxi_init_late(void)
{
	if (of_machine_is_compatible("allwinner,sun8iw11p1") ||
		of_machine_is_compatible("allwinner,sun8iw12p1") ||
		of_machine_is_compatible("allwinner,sun8iw15p1") ||
		of_machine_is_compatible("allwinner,sun8iw16p1") ||
		of_machine_is_compatible("allwinner,sun8iw17p1") ||
		of_machine_is_compatible("allwinner,sun8iw18p1") ||
		of_machine_is_compatible("allwinner,sun8iw19p1") ||
		of_machine_is_compatible("allwinner,sun8iw8p1") ||
		of_machine_is_compatible("allwinner,sun8iw7p1") ||
		of_machine_is_compatible("allwinner,sun8iw6p1"))
		platform_device_register(&sunxi_cpuidle);
}

static const char * const sun8i_board_dt_compat[] = {
	"allwinner,sun8i-a23",
	"allwinner,sun8i-a33",
	"allwinner,sun8i-h3",
	"allwinner,sun8iw11p1",
	"allwinner,sun8iw12p1",
	"allwinner,sun8iw15p1",
	"allwinner,sun8iw16p1",
	"allwinner,sun8iw17p1",
	"allwinner,sun8iw18p1",
	"allwinner,sun8iw19p1",
	"allwinner,sun8iw8p1",
	"allwinner,sun8iw7p1",
	"allwinner,sun8iw6p1",
	NULL,
};

#define PHYS_MEM_START 0x40000000
struct ion_mem_reserve_info ion_reserve_mem = {
	.size = 0,
	.start = 0,
};
static int __init ion_mem_reserve(char *p)
{
	char *endp;
	char *startp = p;
	char *endedp = p + strlen(p) - 1;
	struct memblock_region *reg;
	u32 mem_size = 0, reserve_size = 0;
	u32 total_pages = 0, phys_mem_size = 0;

	for_each_memblock(memory, reg) {
		total_pages += memblock_region_memory_end_pfn(reg) -
			memblock_region_memory_base_pfn(reg);
	}
	phys_mem_size = total_pages << PAGE_SHIFT;

	do {
		reserve_size = (u32)memparse(startp, &endp);
		if (*endp == '@') {
			startp = endp + 1;
			mem_size = (u32)memparse(startp, &endp);
		} else {
			/*reserve_size@dram_size*/
			pr_err("%s: err format for ion reserve list!\n", __func__);
			return 0;
		}
		startp = endp + 1;
		if (mem_size == phys_mem_size)
			break;
	} while (startp < endedp);

	if (mem_size == phys_mem_size) {
		ion_reserve_mem.size = reserve_size;
		ion_reserve_mem.start = PHYS_MEM_START + phys_mem_size - ion_reserve_mem.size;
		memblock_reserve(ion_reserve_mem.start, ion_reserve_mem.size);
		pr_info("%s: reserve %dMiB at 0x%x\n",
			   __func__, ion_reserve_mem.size >> 20, ion_reserve_mem.start);
	} else
		pr_err("%s: ion reserve list doesn't match current mem size!\n",
			   __func__);

	return 0;
}

static int __init early_ion_carveout_list(char *p)
{
	return ion_mem_reserve(p);
}
early_param("ion_carveout_list", early_ion_carveout_list);

DT_MACHINE_START(SUN8I_DT, CONFIG_SUNXI_SOC_NAME)
	.init_time	= sun6i_timer_init,
	.map_io		= sunxi_map_io,
#if defined (CONFIG_ARCH_SUN8IW6P1)
	.smp_init       = smp_init_ops(mcpm_smp_set_ops),
#endif
	.init_early	= NULL,
	.init_late	= sunxi_init_late,
	.dt_compat	= sun8i_board_dt_compat,
MACHINE_END

static const char * const sun9i_board_dt_compat[] = {
	"allwinner,sun9i-a80",
	NULL,
};

DT_MACHINE_START(SUN9I_DT, "Allwinner sun9i Family")
	.dt_compat	= sun9i_board_dt_compat,
MACHINE_END
