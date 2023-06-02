/**
    NVT reset function
    This file will setup reset function interface
    @file       nvt-reset.c
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#include <linux/pm.h>
#include <linux/reboot.h>
#include <linux/delay.h>

#include <asm/io.h>

#include <mach/nvt-io.h>
#include <plat/hardware.h>
#include <plat/rtc_int.h>

#define WDT_CG_EN_OFS     0x74
#define WDT_CG_RESET_OFS  0x84
#define WDT_CG_BIT        17
#define WDT_MAN_OFS       0xC
static void nvt_trigger_wdt_external(void)
{
	uint32_t reg;

	reg = nvt_readl(NVT_CG_BASE_VIRT + WDT_CG_RESET_OFS);
	reg &= ~(0x1 << WDT_CG_BIT);
	nvt_writel(reg, NVT_CG_BASE_VIRT + WDT_CG_RESET_OFS);

	udelay(50);

	reg = nvt_readl(NVT_CG_BASE_VIRT + WDT_CG_RESET_OFS);
	reg |= (0x1 << WDT_CG_BIT);
	nvt_writel(reg, NVT_CG_BASE_VIRT + WDT_CG_RESET_OFS);

	reg = nvt_readl(NVT_CG_BASE_VIRT + WDT_CG_EN_OFS);
	reg |= (0x1 << WDT_CG_BIT);
	nvt_writel(reg, NVT_CG_BASE_VIRT + WDT_CG_EN_OFS);

	nvt_writel(0x5a960112, NVT_WDT_BASE_VIRT);

	udelay(80);

	nvt_writel(0x5a960113, NVT_WDT_BASE_VIRT);

	nvt_writel(0x1, NVT_WDT_BASE_VIRT + WDT_MAN_OFS);
}

static int nvt_notify_reboot(struct notifier_block *nb, unsigned long action, void *data)
{
	if (action == SYS_RESTART) {
		pr_info("restarting.....\n");
#if defined(CONFIG_RTC_DRV_NA51055) && !defined(CONFIG_NVT_IM_LVD_RESET)
		nvt_rtc_power_control(3);
#else
		nvt_trigger_wdt_external();
#endif
	}

	return NOTIFY_DONE;
}

static void nvt_power_off_prepare(void)
{
#ifdef CONFIG_RTC_DRV_NA51055
	nvt_rtc_power_control(0);
#endif
}

static struct notifier_block nvt_reboot_notifier = {
	.notifier_call  = nvt_notify_reboot,
	.next       = NULL,
	.priority   = INT_MAX,
};

static void nvt_power_off_done(void)
{
	pr_info(" Power off \n");
}

static int __init nvt_poweroff_setup(void)
{
	pr_info("*** %s \n", __func__);

	pm_power_off = nvt_power_off_done;
        pm_power_off_prepare = nvt_power_off_prepare;
	register_reboot_notifier(&nvt_reboot_notifier);

	return 0;
}

/* It will be executed when restart done */
void nvt_ivot_restart(enum reboot_mode mode, const char *cmd)
{
	pr_info("done\n");
	/* Waiting for core0 rtc flow ready */
	mdelay(5000);

	nvt_trigger_wdt_external();
}

arch_initcall(nvt_poweroff_setup);
