/*
 * drivers/cpufreq/ambarella-cpufreq.c
 *
 * CPU Frequency Scaling for Ambarella platform
 *
 * Copyright (C) 2015 ST Ambarella Shanghai Co., Ltd.
 * XianqingZheng <xqzheng@ambarella.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <plat/event.h>
#include <linux/io.h>
#include <plat/rct.h>

//#define amba_cpufreq_debug	//used at debug mode

#ifdef amba_cpufreq_debug
#define amba_cpufreq_prt printk
#else
#define amba_cpufreq_prt(format, arg...) do {} while (0)
#endif

/* Ambarella CPUFreq driver data structure */
static struct {
	struct clk *core_clk;
	struct clk *cortex_clk;
	unsigned int transition_latency;
	struct cpufreq_frequency_table *cortex_clktbl;
	struct cpufreq_frequency_table *core_clktbl;
} amba_cpufreq;

static int amba_cpufreq_verify(struct cpufreq_policy *policy)
{
	return cpufreq_frequency_table_verify(policy, amba_cpufreq.cortex_clktbl);
}

static unsigned int amba_cpufreq_get(unsigned int cpu)
{
	return clk_get_rate(amba_cpufreq.cortex_clk) / 1000;
}

static int amba_cpufreq_target(struct cpufreq_policy *policy,
		unsigned int target_freq, unsigned int relation)
{
	struct cpufreq_freqs freqs;
	unsigned long cortex_newfreq, core_newfreq;
	int index, ret;
	unsigned int cur_freq;

	if(target_freq < 96000) {
		printk("Target frequency is too low, cortex is not stable, it should be more than 96000\n");
		return -EINVAL;
	}

	if (cpufreq_frequency_table_target(policy, amba_cpufreq.cortex_clktbl,
				target_freq, relation, &index))
		return -EINVAL;

	freqs.old = amba_cpufreq_get(0);
	cortex_newfreq = amba_cpufreq.cortex_clktbl[index].frequency * 1000;
	core_newfreq = amba_cpufreq.core_clktbl[index].frequency * 1000;
	freqs.new = cortex_newfreq / 1000;

	amba_cpufreq_prt(KERN_INFO "prepare to switch the frequency from %d KHz to %d KHz\n"
				,freqs.old, freqs.new);

	cpufreq_notify_transition(policy, &freqs, CPUFREQ_PRECHANGE);
	ambarella_set_event(AMBA_EVENT_PRE_CPUFREQ, NULL);

	ret = clk_set_rate(amba_cpufreq.cortex_clk, cortex_newfreq);
	/* Get current rate after clk_set_rate, in case of failure */
	if (ret) {
		pr_err("CPU Freq: cpu clk_set_rate failed: %d\n", ret);
		freqs.new = clk_get_rate(amba_cpufreq.cortex_clk) / 1000;
	}

	ret = clk_set_rate(amba_cpufreq.core_clk, core_newfreq);
	/* Get current rate after clk_set_rate, in case of failure */
	if (ret) {
		pr_err("CPU Freq: cpu clk_set_rate failed: %d\n", ret);
	}

#if (CHIP_REV == S2L) || (CHIP_REV == S3L)
	if(amba_cpufreq.core_clktbl[index].frequency <= 96000) {
		/*Disable IDSP/VDSP to Save Power*/
		amba_writel(CKEN_CLUSTER_REG, 0x540);
	} else {
		/*Enable IDSP/VDSP to Save Power*/
		amba_writel(CKEN_CLUSTER_REG, 0x3fff);
	}
#endif
	cpufreq_notify_transition(policy, &freqs, CPUFREQ_POSTCHANGE);
	ambarella_set_event(AMBA_EVENT_POST_CPUFREQ, NULL);

	cur_freq = clk_get_rate(amba_cpufreq.cortex_clk) / 1000;
	amba_cpufreq_prt(KERN_INFO "current frequency of cortex clock is:%d KHz\n",
				 cur_freq);


	return ret;
}

static int amba_cpufreq_init(struct cpufreq_policy *policy)
{
	int ret;

	ret = cpufreq_frequency_table_cpuinfo(policy, amba_cpufreq.cortex_clktbl);
	if (ret) {
		pr_err("cpufreq_frequency_table_cpuinfo() failed");
		return ret;
	}

	cpufreq_frequency_table_get_attr(amba_cpufreq.cortex_clktbl, policy->cpu);
	policy->cpuinfo.transition_latency = amba_cpufreq.transition_latency;
	policy->cur = amba_cpufreq_get(0);

	cpumask_setall(policy->cpus);

	return 0;
}

static int amba_cpufreq_exit(struct cpufreq_policy *policy)
{
	cpufreq_frequency_table_put_attr(policy->cpu);
	return 0;
}

static struct freq_attr *amba_cpufreq_attr[] = {
	 &cpufreq_freq_attr_scaling_available_freqs,
	 NULL,
};

static struct cpufreq_driver amba_cpufreq_driver = {
	.name		= "amba-cpufreq",
	.flags		= CPUFREQ_STICKY,
	.verify		= amba_cpufreq_verify,
	.target		= amba_cpufreq_target,
	.get		= amba_cpufreq_get,
	.init		= amba_cpufreq_init,
	.exit		= amba_cpufreq_exit,
	.attr		= amba_cpufreq_attr,
};

static int amba_cpufreq_driver_init(void)
{
	struct device_node *np;
	const struct property *prop;
	struct cpufreq_frequency_table *cortex_freqtbl;
	struct cpufreq_frequency_table *core_freqtbl;
	const __be32 *val;
	int cnt, i, ret, clk_div;

	printk("ambarella cpufreq driver init\n");
	np = of_find_node_by_path("/cpus");
	if (!np) {
		pr_err("No cpu node found");
		return -ENODEV;
	}

	ret = of_property_read_u32(np, "amb,core-div", &clk_div);
	if (ret != 0 || !((clk_div == 1 || clk_div == 2))) {
		/*Default is that the pll_out_core is twice gclk_core */
		clk_div = 2;
	}

	if (of_property_read_u32(np, "clock-latency",
				&amba_cpufreq.transition_latency))
		amba_cpufreq.transition_latency = CPUFREQ_ETERNAL;

	prop = of_find_property(np, "cpufreq_tbl", NULL);
	if (!prop || !prop->value) {
		pr_err("Invalid cpufreq_tbl");
		ret = -ENODEV;
		goto out_put_node;
	}

	cnt = prop->length / sizeof(u32) / 2;
	val = prop->value;

	cortex_freqtbl = kmalloc(sizeof(*cortex_freqtbl) * (cnt + 2), GFP_KERNEL);
	if (!cortex_freqtbl) {
		ret = -ENOMEM;
		goto out_put_node;
	}

	core_freqtbl = kmalloc(sizeof(*core_freqtbl) * (cnt + 2), GFP_KERNEL);
	if (!core_freqtbl) {
		ret = -ENOMEM;
		goto out_put_node;
	}

	for (i = 0; i < cnt; i++) {
		core_freqtbl[i].index = i;
		/*pll_out_core = clk_div * gclk_core;*/
		core_freqtbl[i].frequency = be32_to_cpup(val) * clk_div;

		cortex_freqtbl[i].index = i;
		cortex_freqtbl[i].frequency = be32_to_cpup((val + 1));
		val = val + 2;

	}

	cortex_freqtbl[i].index = i;
	cortex_freqtbl[i].frequency = clk_get_rate(clk_get(NULL, "gclk_cortex")) / 1000;

	core_freqtbl[i].index = i;
	core_freqtbl[i].frequency = clk_get_rate(clk_get(NULL, "gclk_core")) / 1000 * clk_div;

	i++;
	cortex_freqtbl[i].index = i;
	cortex_freqtbl[i].frequency = CPUFREQ_TABLE_END;

	core_freqtbl[i].index = i;
	core_freqtbl[i].frequency = CPUFREQ_TABLE_END;

	amba_cpufreq.cortex_clktbl = cortex_freqtbl;
	amba_cpufreq.core_clktbl = core_freqtbl;

	of_node_put(np);

	amba_cpufreq.core_clk = clk_get(NULL, "pll_out_core");
	if (IS_ERR(amba_cpufreq.core_clk)) {
		pr_err("Unable to get core clock\n");
		ret = PTR_ERR(amba_cpufreq.core_clk);
		goto out_put_mem;
	}

	amba_cpufreq.cortex_clk = clk_get(NULL, "gclk_cortex");
	if (IS_ERR(amba_cpufreq.cortex_clk)) {
		pr_err("Unable to get cotex clock\n");
		ret = PTR_ERR(amba_cpufreq.cortex_clk);
		goto out_put_mem;
	}

	ret = cpufreq_register_driver(&amba_cpufreq_driver);
	if (!ret)
		return 0;

	pr_err("failed register driver: %d\n", ret);
	clk_put(amba_cpufreq.core_clk);
	clk_put(amba_cpufreq.cortex_clk);

out_put_mem:
	kfree(cortex_freqtbl);
	kfree(core_freqtbl);
	return ret;

out_put_node:
	of_node_put(np);
	return ret;
}

module_init(amba_cpufreq_driver_init);

static void __exit amba_cpufreq_driver_exit(void)
{
	cpufreq_unregister_driver(&amba_cpufreq_driver);
	if(amba_cpufreq.cortex_clktbl != NULL) {
		kfree(amba_cpufreq.cortex_clktbl);
		amba_cpufreq.cortex_clktbl = NULL;
	}

	if(amba_cpufreq.core_clktbl != NULL) {
		kfree(amba_cpufreq.core_clktbl);
		amba_cpufreq.core_clktbl = NULL;
	}

}
module_exit(amba_cpufreq_driver_exit);


MODULE_AUTHOR("XianqingZheng <xqzheng@ambarella.com>");
MODULE_DESCRIPTION("Ambarella CPUFreq driver");
MODULE_LICENSE("GPL");
