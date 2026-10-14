// SPDX-License-Identifier: GPL-2.0

#include <linux/kernel.h>
#include <mach/platform.h>
#include <linux/io.h>
#include <linux/printk.h>
#include <asm/early_ioremap.h>
#include <linux/spinlock.h>

static unsigned int g_cpu_chipid = 0;
static DEFINE_SPINLOCK(chipid_lock);

unsigned int get_chipid(void)
{
	void *sysid = NULL;

	spin_lock(&chipid_lock);
	if (g_cpu_chipid != 0) {
		spin_unlock(&chipid_lock);
		return g_cpu_chipid;
	}
	spin_unlock(&chipid_lock);

	sysid = ioremap(REG_SC_CHIPID, PAGE_SIZE);
	if (sysid == NULL) {
		pr_err("ChipId reg: ioremap failed\n");
		return 0;
	}

	spin_lock(&chipid_lock);
	g_cpu_chipid = (unsigned int)readl(sysid);
	spin_unlock(&chipid_lock);

	iounmap(sysid);

	return g_cpu_chipid;
}
EXPORT_SYMBOL(get_chipid);


unsigned int __init early_get_chipid(void)
{
	void *sysid = NULL;

	spin_lock(&chipid_lock);
	if (g_cpu_chipid != 0) {
		spin_unlock(&chipid_lock);
		return g_cpu_chipid;
	}
	spin_unlock(&chipid_lock);

	sysid = early_ioremap(REG_SC_CHIPID, PAGE_SIZE);
	if (sysid == NULL) {
		pr_err("ChipId reg: ioremap failed\n");
		return 0;
	}

	spin_lock(&chipid_lock);
	g_cpu_chipid = (unsigned int)readl(sysid);
	spin_unlock(&chipid_lock);

	iounmap(sysid);

	return g_cpu_chipid;
}

