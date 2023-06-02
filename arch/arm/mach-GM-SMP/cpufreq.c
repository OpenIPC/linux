/*
 * linux/arch/arm/mach-GM/cpufreq.c
 *
 * Copyright (C) 2014 Grain Media
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/slab.h>
#include <mach/ftpmu010.h>

static int cpu_freq_fd;
static unsigned int cpu_frequency;
static unsigned int cpu_max_speed;

struct gm_freq_info {
	unsigned int cpufreq_mhz;
	unsigned int membus;
	unsigned int cccr;
	unsigned int div2;
};

static struct cpufreq_frequency_table *gm_freq_table;

static pmuReg_t  pmu_reg[] = {
    /* off, bitmask,   lockbit,    init val,   init mask */
    {0x14, (0xFFFFFFFF << 0), (0xFFFFFFFF << 0), (0x0 << 0), (0xFFFFFFFF << 0)},   /* AHB clock gate */
};

static pmuRegInfo_t	pmu_reg_info = {
    "CPU_Freq",
    ARRAY_SIZE(pmu_reg),
    ATTR_TYPE_NONE,
    pmu_reg
};

static void update_pwm_freq(unsigned int clk)
{
	unsigned int value = 0xFFFF;

    if (clk == gm_freq_table[0].frequency)
        value = 0xFFFF;
    else if (clk == gm_freq_table[1].frequency)
        value = 0xAAAA;
    else if (clk == gm_freq_table[2].frequency)
        value = 0x1111;
    else if (clk == gm_freq_table[3].frequency)
        value = 0x0101;
    else
    	printk("Not this kind of clock definition\n");

#ifdef CONFIG_PLATFORM_GM8220
 	ftpmu010_write_reg(cpu_freq_fd, 0x14, (value << 15), (0xFFFF << 15));
#else
 	ftpmu010_write_reg(cpu_freq_fd, 0x14, (value << 16), (0xFFFF << 16));
#endif 	
	ftpmu010_write_reg(cpu_freq_fd, 0x14, (0x3 << 0), (0x3 << 0));
}

static int gm_cpufreq_verify(struct cpufreq_policy *policy)
{
    return cpufreq_frequency_table_verify(policy, gm_freq_table);
}

static unsigned int gm_cpufreq_get(unsigned int cpu)
{
	return cpu_frequency;//REG_RD(clkgen, regi_clkgen, rw_clk_ctrl);, clk_ctrl.pll ? 200000 : 6000; ???
}

static int gm_cpufreq_set(struct cpufreq_policy *policy,
			      unsigned int target_freq,
			      unsigned int relation)
{
	struct cpufreq_freqs freqs;
	int idx;

	if (policy->cpu != 0)
		return -EINVAL;

	/* Lookup the next frequency */
	if (cpufreq_frequency_table_target(policy, gm_freq_table,
				target_freq, relation, &idx))
		return -EINVAL;

	freqs.old = policy->cur;
	freqs.new = cpu_frequency = gm_freq_table[idx].frequency;
	freqs.cpu = policy->cpu;
#if 0
	printk(KERN_DEBUG "CPU freq %d to %d MHz%s\n",
			freqs.old, freqs.new,
			(freqs.old == freqs.new) ? " (skip)" : "");
#endif
	if (freqs.old == target_freq)
		return 0;

	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);

	//local_irq_save(flags);
	update_pwm_freq(cpu_frequency);
	//local_irq_restore(flags);
	
	cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);

	return 0;
}

static int gm_cpufreq_init(struct cpufreq_policy *policy)
{
	int ret = -EINVAL;
	int i, j, num, new_speed;
	
	printk("GM CPU frequency driver\n");

    cpu_freq_fd = ftpmu010_register_reg(&pmu_reg_info);
    if (cpu_freq_fd < 0)
        printk("cpu freq register PMU fail");

	/* set default policy and cpuinfo */
	cpu_max_speed = ftpmu010_get_attr(ATTR_TYPE_CPU) / 1000000;
	//printk("cpu_speed = %d\n", cpu_max_speed);
	cpu_frequency = cpu_max_speed;
	policy->max = policy->cpuinfo.max_freq = cpu_max_speed;
	
	if(cpu_max_speed > 700)
        num = 4;
	else
        num = 3;

    gm_freq_table = kzalloc((num + 1) * sizeof(*gm_freq_table), GFP_KERNEL);
    if (gm_freq_table == NULL)
    	return -ENOMEM;
    
    for (i = 0; i < num; i++) {
        new_speed = cpu_max_speed;
        
        for(j = 0; j < i; j++)
            new_speed /= 2;
            
    	gm_freq_table[i].index = i;
    	gm_freq_table[i].frequency = new_speed;
    	//printk("a%d = %d,%d\n", i, gm_freq_table[i].index, gm_freq_table[i].frequency);
    }
    		
	policy->min = policy->cpuinfo.min_freq = 100000;
	policy->cpuinfo.transition_latency = 1000; /* FIXME: 1 ms, assumed */
	policy->cur = policy->min = policy->max;
	
	gm_freq_table[num].index = 0;
	gm_freq_table[num].frequency = CPUFREQ_TABLE_END;
    
	ret = cpufreq_frequency_table_cpuinfo(policy, gm_freq_table);

	if (ret) {
		pr_err("failed to setup frequency table\n");
		return ret;
	}
    cpufreq_frequency_table_get_attr(gm_freq_table, policy->cpu);
	pr_info("CPUFREQ support for gm initialized\n");
	return 0;
}

static int gm_cpufreq_exit(struct cpufreq_policy *policy)
{
    cpufreq_frequency_table_put_attr(policy->cpu);
    
    return 0;
}

static struct freq_attr *gm_cpufreq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};

static struct cpufreq_driver gm_cpufreq_driver = {	
	.flags		= CPUFREQ_STICKY,	
	.verify		= gm_cpufreq_verify,
	.target		= gm_cpufreq_set,
	.init		= gm_cpufreq_init,
	.exit       = gm_cpufreq_exit,
	.get		= gm_cpufreq_get,
	.name		= "GM-cpufreq",
	.attr		= gm_cpufreq_attr,
};

static int __init cpufreq_init(void)
{
	return cpufreq_register_driver(&gm_cpufreq_driver);
}
module_init(cpufreq_init);

static void __exit cpufreq_exit(void)
{
	cpufreq_unregister_driver(&gm_cpufreq_driver);
}
module_exit(cpufreq_exit);

MODULE_DESCRIPTION("CPU frequency scaling driver for GM");
MODULE_LICENSE("GPL");
