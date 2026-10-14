// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/pm_opp.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/lotus/lotus_dvfs.h>
#include "lotus_dvfs.h"

#define LOTUS_CPU_CONUT 2
static struct platform_device *g_dvfs_platdev;
static struct clk *cpu_clk[LOTUS_CPU_CONUT];
extern unsigned int valid_volt;

static int pm_cpufreq_target(struct cpufreq_policy *policy, unsigned int index)
{
	int ret;
	unsigned int old_freq;
	unsigned long new_freq;
	struct cpufreq_frequency_table *freq_table = policy->freq_table;

	new_freq = freq_table[index].frequency * 1000;
	old_freq = clk_get_rate(policy->clk);

	if (new_freq == old_freq)
		return 0;

	if (new_freq > old_freq) {
		lotus_cpufreq_set_volt(new_freq);
		msleep(1);
		ret = clk_set_rate(policy->clk, new_freq);
		if (ret) {
			pr_err("%s: set rate %lu failed\n", __func__, new_freq);
			return ret;
		}
	} else {
		ret = clk_set_rate(policy->clk, new_freq);
		if (ret) {
			pr_err("%s: set rate %lu failed\n", __func__, new_freq);
			return ret;
		}
		msleep(1);
		lotus_cpufreq_set_volt(new_freq);
	}

	return 0;
}

static unsigned int pm_cpufreq_getspeed(unsigned int cpu)
{
	struct cpufreq_policy *policy = NULL;
	unsigned long rate;

	policy = cpufreq_cpu_get_raw(cpu);
	if ((policy == NULL) || (policy->clk == NULL))
		return 0;

	rate = clk_get_rate(policy->clk);

	return rate / 1000;
}

static int pm_cpufreq_cpu_init(struct cpufreq_policy *policy)
{
	struct device *mpu_dev;
	int ret;
	struct cpufreq_frequency_table *freq_table = NULL;

	mpu_dev = get_cpu_device(policy->cpu);
	if (IS_ERR(mpu_dev)) {
		pr_err("failed to get cpu%d device\n", policy->cpu);
		return PTR_ERR(mpu_dev);
	}

	ret = dev_pm_opp_init_cpufreq_table(mpu_dev, &freq_table);
	if (ret) {
		pr_err("%s: opp init cpufreq table failed, %d\n", __func__, ret);
		return ret;
	}

	cpufreq_generic_init(policy, freq_table, 0);
	policy->clk = cpu_clk[policy->cpu];
	policy->min = policy->cpuinfo.min_freq;
	policy->max = policy->cpuinfo.max_freq;
	policy->cur = policy->max;

	dev_pm_opp_of_register_em(mpu_dev, policy->cpus);

	/* Support turbo/boost mode */
	if (policy_has_boost_freq(policy)) {
		/* This gets disabled by core on driver unregister */
		ret = cpufreq_enable_boost_support();
		if (ret) {
			dev_pm_opp_free_cpufreq_table(mpu_dev, &freq_table);
			return ret;
		}
	}

	return 0;
}

static int pm_cpufreq_cpu_exit(struct cpufreq_policy *policy)
{
	dev_pm_opp_free_cpufreq_table(get_cpu_device(policy->cpu), &policy->freq_table);

	return 0;
}

unsigned long suspend_freq;

static int pm_cpufreq_suspend(struct cpufreq_policy *policy)
{

	suspend_freq = clk_get_rate(policy->clk);

	return 0;
}

static int pm_cpufreq_resume(struct cpufreq_policy *policy)
{
	unsigned long now_freq;
	int ret;

	valid_volt = 0;
	now_freq = clk_get_rate(policy->clk);
	if (now_freq == suspend_freq) {
		lotus_cpufreq_set_volt(now_freq);
		return 0;
	}

	if (now_freq < suspend_freq) {
		lotus_cpufreq_set_volt(suspend_freq);
		msleep(1);
		ret = clk_set_rate(policy->clk, suspend_freq);
		if (ret) {
			pr_err("%s: set rate %lu failed\n", __func__, suspend_freq);
			return ret;
		}
	} else {
		ret = clk_set_rate(policy->clk, suspend_freq);
		if (ret) {
			pr_err("%s: set rate %lu failed\n", __func__, suspend_freq);
			return ret;
		}
		msleep(1);
		lotus_cpufreq_set_volt(suspend_freq);
	}

	return 0;
}


static struct cpufreq_driver lotus_cpufreq_driver = {
	.verify = cpufreq_generic_frequency_table_verify,
	.target_index = pm_cpufreq_target,
	.get    = pm_cpufreq_getspeed,
	.init   = pm_cpufreq_cpu_init,
	.exit   = pm_cpufreq_cpu_exit,
	.suspend = pm_cpufreq_suspend,
	.resume = pm_cpufreq_resume,
	.name   = "lotus-cpufreq",
	.attr   = cpufreq_generic_attr,
};

static int add_opp_table(struct device *cpu_dev)
{
	int ret;
	struct cpumask cpus;

	ret = dev_pm_opp_of_get_sharing_cpus(cpu_dev, &cpus);
	if (ret) {
		pr_err("%s: pm opp of get sharing cpus failed, %d\n", __func__, ret);
		return ret;
	}

	ret = dev_pm_opp_of_cpumask_add_table(&cpus);
	if (ret) {
		pr_err("%s: pm opp of cpumask add table failed, %d\n", __func__, ret);
		return ret;
	}

	ret = dev_pm_opp_get_opp_count(cpu_dev);
	if (ret <= 0) {
		pr_err("%s: dev_pm_opp_get_opp_count failed, %d\n", __func__, ret);
		dev_pm_opp_of_cpumask_remove_table(&cpus);
		return -EPROBE_DEFER;
	}

	return 0;
}

#ifdef SUPPORT_CPU_1000M
void apll_1000_clk_set(void)
{
	unsigned int val;
	void *crg_base;

	crg_base = (void *)ioremap(0x12010000, 0xf);
	val = readl(crg_base + 0x4);
	writel( val | (0x1 << 29), crg_base + 0x4);

	writel(0x555555, crg_base);
	writel(0x20800053, crg_base + 0x4);

	val = readl(crg_base + 0x8);
	val &= 0xfffffff0;
	val |= 0x1;
	writel(val, crg_base + 0x8);

	val = readl(crg_base + 0xc);
	val |= 0x4;
	writel(val, crg_base + 0xc);

	writel(0x00800053, crg_base + 0x4);
	iounmap(crg_base);

}
#endif

static int dvfs_probe(struct platform_device *pdev)
{
	int ret;
	unsigned int cpu;
	struct device *cpu_dev;

#ifdef SUPPORT_CPU_1000M
	apll_1000_clk_set();
#endif

	for_each_possible_cpu(cpu) {
		cpu_dev = get_cpu_device(cpu);
		if (IS_ERR(cpu_dev)) {
			pr_err("failed to get cpu%d device\n", cpu);
			return PTR_ERR(cpu_dev);
		}

		cpu_clk[cpu] = clk_get(cpu_dev, NULL);
		if (IS_ERR_OR_NULL(cpu_clk[cpu]))
			return PTR_ERR(cpu_clk[cpu]);

		ret = clk_prepare_enable(cpu_clk[cpu]);
		if (ret) {
			clk_put(cpu_clk[cpu]);
			return ret;
		}

		ret = add_opp_table(cpu_dev);
		if (ret) {
			pr_err("%s: add opp table failed, %d\n", __func__, ret);
			clk_disable_unprepare(cpu_clk[cpu]);
			clk_put(cpu_clk[cpu]);
			return ret;
		}
	}

	return cpufreq_register_driver(&lotus_cpufreq_driver);
}

static int dvfs_remove(struct platform_device *pdev)
{
	unsigned int i;

	for (i = 0; i < LOTUS_CPU_CONUT; i++) {
		clk_disable_unprepare(cpu_clk[i]);
		clk_put(cpu_clk[i]);
	}
	return cpufreq_unregister_driver(&lotus_cpufreq_driver);
}

static struct platform_driver g_dvfs_platdrv = {
	.probe    = dvfs_probe,
	.remove   = dvfs_remove,
	.driver.name = "lotus_cpufreq",
	.driver.pm   = NULL,
};

static int __init lotus_cpufreq_init(void)
{
	int ret;

	lotus_init_dvfs_volt();
	ret = platform_driver_register(&g_dvfs_platdrv);
	if (ret) {
		pr_err("%s_init: platform_driver_register failed\n", __func__);
		return ret;
	}

	g_dvfs_platdev = platform_device_register_simple("lotus_cpufreq", -1, NULL, 0);
	if (IS_ERR(g_dvfs_platdev)) {
		pr_err("%s_init: platform_device_register failed\n", __func__);
		platform_driver_unregister(&g_dvfs_platdrv);
		return PTR_ERR(g_dvfs_platdev);
	}

	device_enable_async_suspend(&g_dvfs_platdev->dev);
	return 0;
}

static void __exit lotus_cpufreq_exit(void)
{
	platform_device_unregister(g_dvfs_platdev);
	platform_driver_unregister(&g_dvfs_platdrv);
	lotus_exit_dvfs_volt();
}

module_init(lotus_cpufreq_init);
module_exit(lotus_cpufreq_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Lotus");
MODULE_DESCRIPTION("Lotus cpufreq driver");
