// SPDX-License-Identifier: GPL-2.0
/*
 * Fullhan FH8626V100 SoC support
 *
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/bitops.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/reboot.h>
#include <linux/sizes.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/system_misc.h>

#define FH8626_INTC_PHYS_BASE		0xe0200000
#define FH8626_TIMER_PHYS_BASE		0xf0c00000
#define FH8626_PMU_PHYS_BASE		0xf0000000
#define FH8626_UART0_PHYS_BASE		0xf0700000
#define FH8626_UART1_PHYS_BASE		0xf0800000
#define FH8626_UART2_PHYS_BASE		0xf1300000
#define FH8626_GPIO0_PHYS_BASE		0xf0300000
#define FH8626_GPIO1_PHYS_BASE		0xf4000000

#define FH8626_INTC_VIRT_BASE		0xfe000000
#define FH8626_TIMER_VIRT_BASE		0xfe010000
#define FH8626_UART0_VIRT_BASE		0xfe020000
#define FH8626_PMU_VIRT_BASE		0xfe090000
#define FH8626_UART1_VIRT_BASE		0xfe0a0000
#define FH8626_UART2_VIRT_BASE		0xfe0b0000
#define FH8626_GPIO0_VIRT_BASE		0xfe0c0000
#define FH8626_GPIO1_VIRT_BASE		0xfe0d0000

/* DesignWare GPIO port A data and direction */
#define FH8626_GPIO_SWPORTA_DR		0x00
#define FH8626_GPIO_SWPORTA_DDR		0x04

#define FH8626_PMU_SW_RESET_REG		0x4c
#define FH8626_PMU_SW_RESET_VAL		0x7fffffff

/*
 * The PMU freeze bit stops the watchdog counter but leaves its registers
 * writable.
 */
#define FH8626_PMU_WDT_CTRL_REG		0x1fc
#define FH8626_PMU_WDT_PAUSE		BIT(8)

static struct map_desc fh8626_io_desc[] __initdata = {
	{
		.virtual	= FH8626_INTC_VIRT_BASE,
		.pfn		= __phys_to_pfn(FH8626_INTC_PHYS_BASE),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	},
	{
		.virtual	= FH8626_TIMER_VIRT_BASE,
		.pfn		= __phys_to_pfn(FH8626_TIMER_PHYS_BASE),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	},
	{
		.virtual	= FH8626_UART0_VIRT_BASE,
		.pfn		= __phys_to_pfn(FH8626_UART0_PHYS_BASE),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	},
	{
		.virtual	= FH8626_PMU_VIRT_BASE,
		.pfn		= __phys_to_pfn(FH8626_PMU_PHYS_BASE),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	},
	{
		.virtual	= FH8626_UART1_VIRT_BASE,
		.pfn		= __phys_to_pfn(FH8626_UART1_PHYS_BASE),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	},
	{
		.virtual	= FH8626_UART2_VIRT_BASE,
		.pfn		= __phys_to_pfn(FH8626_UART2_PHYS_BASE),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	},
	{
		.virtual	= FH8626_GPIO0_VIRT_BASE,
		.pfn		= __phys_to_pfn(FH8626_GPIO0_PHYS_BASE),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	},
	{
		.virtual	= FH8626_GPIO1_VIRT_BASE,
		.pfn		= __phys_to_pfn(FH8626_GPIO1_PHYS_BASE),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	},
};

static void __init fullhan_map_io(void)
{
	/* UART0's static window also supplies DEBUG_FULLHAN_UART0. */
	iotable_init(fh8626_io_desc, ARRAY_SIZE(fh8626_io_desc));
}

/*
 * The boot ROM picks its boot device from GPIO0 pins 12-14 as it finds them
 * (GPIO13 high: SPI NOR), and the PMU software reset does not reset the GPIO
 * controllers. A board that drives any of those pins at run time can make the
 * ROM look for the wrong boot device after a reboot, so give both GPIO ports
 * back the data and direction the bootloader left before resetting.
 */
static void __iomem *const fh8626_gpio_base[] = {
	(void __iomem *)FH8626_GPIO0_VIRT_BASE,
	(void __iomem *)FH8626_GPIO1_VIRT_BASE,
};
static u32 fh8626_boot_gpio[ARRAY_SIZE(fh8626_gpio_base)][2];

static void __init fh8626_save_boot_gpio(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(fh8626_gpio_base); i++) {
		fh8626_boot_gpio[i][0] = readl_relaxed(fh8626_gpio_base[i] +
						       FH8626_GPIO_SWPORTA_DR);
		fh8626_boot_gpio[i][1] = readl_relaxed(fh8626_gpio_base[i] +
						       FH8626_GPIO_SWPORTA_DDR);
	}
}

static void fh8626_restore_boot_gpio(void)
{
	void __iomem *base;
	int i;

	for (i = 0; i < ARRAY_SIZE(fh8626_gpio_base); i++) {
		base = fh8626_gpio_base[i];
		writel_relaxed(fh8626_boot_gpio[i][0],
			       base + FH8626_GPIO_SWPORTA_DR);
		writel_relaxed(fh8626_boot_gpio[i][1],
			       base + FH8626_GPIO_SWPORTA_DDR);
	}
}

static void fullhan_restart(enum reboot_mode mode, const char *cmd)
{
	void __iomem *pmu_base = (void __iomem *)FH8626_PMU_VIRT_BASE;

	fh8626_restore_boot_gpio();
	dsb();
	writel_relaxed(FH8626_PMU_SW_RESET_VAL,
		       pmu_base + FH8626_PMU_SW_RESET_REG);
	dsb();

	while (1)
		cpu_relax();
}

static void fh8626_idle(void)
{
	/* Do not issue WFI in idle on FH8626V100. */
}

static void __init fullhan_init_early(void)
{
	/* Before any driver touches the GPIO ports */
	fh8626_save_boot_gpio();
	arm_pm_restart = fullhan_restart;
	arm_pm_idle = fh8626_idle;
}

static void __init fullhan_init_machine(void)
{
	void __iomem *pmu_base = (void __iomem *)FH8626_PMU_VIRT_BASE;
	u32 val;

	/* Release the watchdog freeze before the watchdog driver probes. */
	val = readl_relaxed(pmu_base + FH8626_PMU_WDT_CTRL_REG);
	writel_relaxed(val & ~FH8626_PMU_WDT_PAUSE,
		       pmu_base + FH8626_PMU_WDT_CTRL_REG);

	of_platform_default_populate(NULL, NULL, NULL);
}

static const char * const fullhan_dt_compat[] __initconst = {
	"fullhan,fh8626v100",
	"fullhan,fh8626",
	NULL,
};

DT_MACHINE_START(FULLHAN_FH8626, "Fullhan FH8626")
	.dt_compat	= fullhan_dt_compat,
	.map_io		= fullhan_map_io,
	.init_early	= fullhan_init_early,
	.init_machine	= fullhan_init_machine,
	.restart	= fullhan_restart,
MACHINE_END
