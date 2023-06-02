/*
 * Copyright 2015 ambarella, Inc.
 *
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Maintainer: Jorney Tu <qtu@ambarella.com>
 */

#include <linux/cpuidle.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/suspend.h>
#include <asm/cpuidle.h>
#include <asm/proc-fns.h>
#include <asm/smp_scu.h>
#include <linux/interrupt.h>
#include <linux/percpu.h>
#include <linux/cpu.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include "../../kernel/time/tick-internal.h"

#define DEFAULT_FREQENCY_INDEX  1

#define MIN(x,y)   ( (x) < (y) ? (x) : (y) )

#define CPUIDLE_DBG  0

#if CPUIDLE_DBG
#define dbg_print(fmt, args...)   printk(fmt, ##args)
#else
#define dbg_print(fmt, args...)   do{}while(0)
#endif


struct ambarella_cpuidle_info {
	unsigned int core_old_freqency ;
	unsigned int cortex_old_freqency ;

	unsigned int freq_tbl_cnt;
	struct cpufreq_frequency_table *freq_tbl;

	struct clk *core_clk ;   // pll_out_core !!!
	struct clk *cortex_clk ;
};

static struct ambarella_cpuidle_info *info = NULL;

static int ambarella_cpufreq_probe(void)
{
	const struct property *prop;
	struct cpufreq_frequency_table *freqs;
	struct device_node *np;
	int i, count;
	int ret = 0;
	const __be32 *value;

	info = kmalloc(sizeof(struct ambarella_cpuidle_info), GFP_KERNEL);
	if (!info) {
		pr_err("kmalloc ambarella_cpuidle_info failed !\n");
		return -ENOMEM;
	}

	np = of_find_node_by_path("/cpus");
	if (!np) {
		pr_err("No cpu node found");
		ret = -EFAULT;
		goto info_mem;
	}

	prop = of_find_property(np, "cpufreq_tbl", NULL);
	if (!prop || !prop->value) {
		pr_err("Invalid cpufreq_tbl");
		ret = -ENODEV;
		goto node_put;
	}

	count = prop->length / sizeof(u32);
	value = prop->value;

	freqs = kmalloc(sizeof(struct cpufreq_frequency_table) * count,GFP_KERNEL);
	if (!freqs) {
		ret = -ENOMEM;
		goto node_put;
	}


	for (i = 0; i < count; i++) {
		freqs[i].index = i;
		freqs[i].frequency = be32_to_cpup(value++);
	}

	info->freq_tbl_cnt = count;
	info->freq_tbl = freqs;

	info->core_clk = clk_get(NULL, "pll_out_core");
	if (IS_ERR(info->core_clk)){
		pr_err("unable get core clk\n");
		ret = -EFAULT;
		goto freq_mem;
	}

	info->cortex_clk = clk_get(NULL, "gclk_cortex");
	if (IS_ERR(info->cortex_clk)){
		pr_err("unable get cortex clk\n");
		ret = -EFAULT;
		goto freq_mem;
	}

	of_node_put(np);
	return ret;

freq_mem:
	kfree(freqs);
node_put:
	of_node_put(np);
info_mem:
	kfree(info);
	return ret;
}

static unsigned int ambarella_cur_freq(struct clk *clk)
{
	return clk_get_rate(clk) / 1000;
}

static int ambarella_cpufreq_switch(unsigned int index)
{
	int ret ;
	unsigned int core_new_freqency = 0;
	unsigned int cortex_new_freqency = 0;

	if(index < 0 || 2 * index > info->freq_tbl_cnt)
		index = 0;

	info->core_old_freqency = ambarella_cur_freq(info->core_clk) / 2;
	info->cortex_old_freqency = ambarella_cur_freq(info->cortex_clk);

	core_new_freqency = info->freq_tbl[index * 2].frequency;
	cortex_new_freqency = info->freq_tbl[index * 2  + 1].frequency;

	ret = clk_set_rate(info->core_clk, core_new_freqency * 1000 * 2);
	if (ret){
		pr_err("Core clock set failed\n");
		return -1;
	}

	ret = clk_set_rate(info->cortex_clk, cortex_new_freqency * 1000);
	if (ret){
		pr_err("Cortex clock set failed\n");
		return -1;
	}

	dbg_print("Switch Core frequency from %d to %d\n", info->core_old_freqency, core_new_freqency);
	dbg_print("Switch Cortex frequency from %d to %d\n", info->cortex_old_freqency, cortex_new_freqency);
	return 0;
}

static int ambarella_cpufreq_recover(void)
{
	int ret ;
	unsigned int core_new_freqency = 0;
	unsigned int cortex_new_freqency = 0;

	core_new_freqency = info->core_old_freqency;
	cortex_new_freqency = info->cortex_old_freqency;

	info->core_old_freqency = ambarella_cur_freq(info->core_clk) / 2;
	info->cortex_old_freqency = ambarella_cur_freq(info->cortex_clk);

	ret = clk_set_rate(info->core_clk, core_new_freqency * 1000 * 2);
	if (ret){
		pr_err("Core clock set failed\n");
		return -1;
	}

	ret = clk_set_rate(info->cortex_clk, cortex_new_freqency * 1000);
	if (ret){
		pr_err("Cortex clock set failed\n");
		return -1;
	}

	dbg_print("Recover Core frequency from %d to %d\n", info->core_old_freqency, core_new_freqency);
	dbg_print("Recover Cortex frequency from %d to %d\n", info->cortex_old_freqency, cortex_new_freqency);
	return 0;
}

static int disable_percpu_timer(void)
{

	struct clock_event_device  *dev;
	struct tick_device *td;
	int cpu;

	cpu = smp_processor_id();
	td = &per_cpu(tick_cpu_device, cpu);
	dev = td->evtdev;
	clockevents_set_mode(dev, CLOCK_EVT_MODE_SHUTDOWN);

	return 0;
}

static int enable_percpu_timer(void)
{

	struct clock_event_device  *dev;
	struct tick_device *td;
	int cpu;

	cpu = smp_processor_id();
	td = &per_cpu(tick_cpu_device, cpu);
	dev = td->evtdev;
	clockevents_set_mode(dev, CLOCK_EVT_MODE_ONESHOT);

	return 0;
}

static void ambarella_idle_exec(unsigned int freq_index)
{

	ambarella_cpufreq_switch(freq_index);

	disable_percpu_timer();

	cpu_do_idle();

	enable_percpu_timer();

	ambarella_cpufreq_recover();
}

static int ambarella_pwrdown_idle(struct cpuidle_device *dev,
				struct cpuidle_driver *drv,
				int index)
{
	ambarella_idle_exec(DEFAULT_FREQENCY_INDEX);

	return index;
}

static struct cpuidle_driver ambarella_idle_driver = {
	.name = "ambarella_idle",
	.states = {
		ARM_CPUIDLE_WFI_STATE,
		{
			.name = "FG",
			.desc = "FREQ Gate",
			.flags = CPUIDLE_FLAG_TIMER_STOP,
			.exit_latency = 30,
			.power_usage = 50,
			.target_residency = 400000,      // for judging cpu idle time
			.enter = ambarella_pwrdown_idle,
		},
	},
	.state_count = 2,
};

static int __init ambarella_cpuidle_init(void)
{

	if (ambarella_cpufreq_probe())
		return -EFAULT;

	return cpuidle_register(&ambarella_idle_driver, NULL);
}
module_init(ambarella_cpuidle_init);
