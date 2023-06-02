/*
 * FH Power Management Routines
 *
 * Copyright (C) 2014 Fullhan Microelectronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/pm.h>
#include <linux/suspend.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/spinlock.h>
#include <linux/io.h>

#include <asm/cacheflush.h>
#include <asm/delay.h>

#include <mach/sram.h>
#include <mach/system.h>
#include <mach/io.h>
#include <mach/gpio.h>
#include <mach/ddrc.h>
#include <mach/pmu.h>

#ifdef CONFIG_PM
static u32 old_clk_gate = 0;

static void (*fh_sram_suspend)(void);

static inline void fh_pm_pll0_to_xtal(void)
{
	u32 reg;

	reg = fh_pmu_get_reg(REG_PMU_SYS_CTRL);
	reg &= ~(0x1);
	fh_pmu_set_reg(REG_PMU_SYS_CTRL, reg);
}

static inline void fh_pm_xtal_to_pll0(void)
{
	u32 reg;

	reg = fh_pmu_get_reg(REG_PMU_SYS_CTRL);
	reg |= 0x1;
	fh_pmu_set_reg(REG_PMU_SYS_CTRL, reg);
}

static inline void fh_pm_gate_clocks(void)
{
	u32 reg;
	old_clk_gate = fh_pmu_get_reg(REG_PMU_CLK_GATE);
	reg = fh_pmu_get_reg(REG_PMU_CLK_GATE);
	reg |= 0x7fff3fb1;
	fh_pmu_set_reg(REG_PMU_CLK_GATE, reg);
}

static inline void fh_pm_ungate_clocks(void)
{
	u32 reg;

	reg = old_clk_gate;
	fh_pmu_set_reg(REG_PMU_CLK_GATE, reg);
}


static void fh_sram_push(void *dest, void *src, unsigned int size)
{
	memcpy(dest, src, size);
	flush_icache_range((unsigned long)dest, (unsigned long)(dest + size));
}

static int fh_pm_valid_state(suspend_state_t state)
{
	switch (state) {
	case PM_SUSPEND_ON:
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
		return 1;

	default:
		return 0;
	}
}

static void fh_pm_suspend_to_ram(void)
{
	fh_pm_pll0_to_xtal();
	fh_pm_gate_clocks();

	fh_sram_suspend();

	fh_pm_ungate_clocks();
	fh_pm_xtal_to_pll0();
}

static inline void fh_ddrc_selfrefresh_enable(void)
{
	u32 reg;

	/*
	 * Ensure that the Cadence DDR Controller is idle,
	 * that is when the controller_busy signal is low.
	 */
	do {
		reg = readl(VA_DDRC_REG_BASE + OFFSET_DENAL_CTL_57);
	} while (reg & DDRC_CONTROLLER_BUSY);

	/*
	 * Put the memories into self-refresh mode
	 * by issuing one of the self-refresh entry commands
	 * through the Low Power Control Module
	 */
	writel(DDRC_LPI_SR_WAKEUP_TIME | DDRC_LP_CMD_SELFREFRESH | DDRC_CKSRX_DELAY,
	       VA_DDRC_REG_BASE + OFFSET_DENAL_CTL_31);

	//wait no more
	/*
	do
	{
	    reg = readl(VA_DDRC_REG_BASE + OFFSET_DENAL_CTL_97);
	}
	while(reg & DDRC_CKE_STATUS);
	*/
}

static inline void fh_ddrc_selfrefresh_disable(void)
{
	//Exit any low power state
	writel(DDRC_LPI_SR_WAKEUP_TIME | DDRC_LP_CMD_EXITLOWPOWER | DDRC_CKSRX_DELAY,
	       VA_DDRC_REG_BASE + OFFSET_DENAL_CTL_31);
}

static void fh_pm_suspend_to_cache(void)
{
	asm volatile("mov r1, #0\n\t"
		     "mcr p15, 0, r1, c7, c10, 4\n\t"
		     "mcr p15, 0, r1, c7, c0, 4\n\t"
		     : /* no output */
		     : /* no input */
		     : "r1");

	fh_ddrc_selfrefresh_enable();

	asm volatile("mov r1, #0\n\t"
		     "mcr p15, 0, r1, c7, c10, 4\n\t"
		     "mcr p15, 0, r1, c7, c0, 4\n\t"
		     : /* no output */
		     : /* no input */
		     : "r1");
	fh_ddrc_selfrefresh_disable();
}

static int fh_pm_enter(suspend_state_t state)
{
	int ret = 0;

	fh_irq_suspend();
	fh_gpio_irq_suspend();

	switch (state) {
	case PM_SUSPEND_ON:
		cpu_do_idle();
		break;
	case PM_SUSPEND_STANDBY:
		fh_pm_suspend_to_cache();
		break;
	case PM_SUSPEND_MEM:
		fh_pm_suspend_to_ram();
		break;
	default:
		ret = -EINVAL;
	}

	fh_gpio_irq_resume();
	fh_irq_resume();

	return ret;
}

static const struct platform_suspend_ops fh_pm_ops = {
	.enter		= fh_pm_enter,
	.valid		= fh_pm_valid_state,
};

static int __init fh_pm_probe(struct platform_device *pdev)
{
	fh_sram_suspend = sram_alloc(fh_cpu_suspend_sz, NULL);
	if (!fh_sram_suspend) {
		dev_err(&pdev->dev, "cannot allocate SRAM memory\n");
		return -ENOMEM;
	}
	fh_sram_push(fh_sram_suspend, fh_cpu_suspend,
		       fh_cpu_suspend_sz);
	suspend_set_ops(&fh_pm_ops);

	return 0;
}

static int __exit fh_pm_remove(struct platform_device *pdev)
{
	sram_free(fh_sram_suspend, fh_cpu_suspend_sz);
	return 0;
}

static struct platform_driver fh_pm_driver = {
	.driver =
	{
		.name	 = "pm-fh",
		.owner	 = THIS_MODULE,
	},
	.remove = __exit_p(fh_pm_remove),
};

static int __init fh_pm_init(void)
{
	return platform_driver_probe(&fh_pm_driver, fh_pm_probe);
}
late_initcall(fh_pm_init);
#endif
