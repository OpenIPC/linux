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

unsigned int get_cpu_freq(void)
{
	static void *cpu_clk_reg = NULL;
	unsigned int cpu_clk;

	if (!cpu_clk_reg) {
		cpu_clk_reg = ioremap(REG_CPU_CLK_SEL, PAGE_SIZE);
		if (cpu_clk_reg == NULL) {
			pr_err("CPU clk reg: ioremap failed\n");
			return 24000000;
		}
	}

	switch((unsigned int)readl(cpu_clk_reg) & 0x7) {
		case CPU_FREQ_24M:
			cpu_clk = 24;
			break;
		case CPU_FREQ_1188M:
			cpu_clk = 1188;
			break;
		case CPU_FREQ_993M:
			cpu_clk = 993;
			break;
		case CPU_FREQ_900M:
			cpu_clk = 900;
			break;
		case CPU_FREQ_700M:
			cpu_clk = 700;
			break;
		case CPU_FREQ_594M:
			cpu_clk = 594;
			break;
		case CPU_FREQ_396M:
			cpu_clk = 396;
			break;
		case CPU_FREQ_350M:
			cpu_clk = 350;
			break;
		default:
			cpu_clk = 24;
			break;
	}

	return cpu_clk * 1000 * 1000;

}
EXPORT_SYMBOL(get_cpu_freq);

