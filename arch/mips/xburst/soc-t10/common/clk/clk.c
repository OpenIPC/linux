/*
 * JZSOC Clock and Power Manager
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006 Ingenic Semiconductor Inc.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/syscore_ops.h>
#include <linux/vmalloc.h>
#include <linux/suspend.h>
#include <linux/seq_file.h>
#include <linux/cpufreq.h>

#include <soc/cpm.h>
#include <soc/base.h>
#include <soc/extal.h>
#include <jz_proc.h>

#include "clk.h"
/*
 *  unit: kHz
 */
static unsigned long set_cpu_freqs[] = {
	12000,
	24000  ,60000 ,120000,
	200000 ,300000 ,600000,
	792000, 1008000,1200000,
	CPUFREQ_TABLE_END
};

static int clk_suspend(void)
{
	printk("clk suspend!\n");
	return 0;
}

static void clk_resume(void)
{
	printk("clk resume!\n");
}
static int clk_sleep_pm_callback(struct notifier_block *nfb,unsigned long action,void *ignored)
{
	switch (action) {
	case PM_SUSPEND_PREPARE:
		printk("clk_sleep_pm_callback PM_SUSPEND_PREPARE\n");
		break;
	case PM_POST_SUSPEND:
		printk("clk_sleep_pm_callback PM_POST_SUSPEND\n");
		break;
	}
	return NOTIFY_OK;
}
static struct notifier_block clk_sleep_pm_notifier = {
	.notifier_call = clk_sleep_pm_callback,
	.priority = 0,
};
struct syscore_ops clk_pm_ops = {
	.suspend = clk_suspend,
	.resume = clk_resume,
};
static void init_clk_parent(struct clk *p) {
	int init = 0;
	if(!p)
		return;
	if(p->init_state) {
		p->count = 1;
		p->init_state = 0;
		init = 1;
	}
	if(p->count == 0) {
		printk("%s clk should be opened!\n",p->name);
		p->count = 1;
	}
	if(!init)
		p->count++;
}

void __init init_all_clk(void)
{
	int i;
	struct clk *clk_srcs = get_clk_from_id(0);
	int clk_srcs_size = get_clk_sources_size();
	for(i = 0; i < clk_srcs_size; i++) {
		if(clk_srcs[i].flags & CLK_FLG_CPCCR) {
			init_cpccr_clk(&clk_srcs[i]);
		}
		if(clk_srcs[i].flags & CLK_FLG_CGU) {
			init_cgu_clk(&clk_srcs[i]);
		}
		if(clk_srcs[i].flags & CLK_FLG_PLL) {
			init_ext_pll(&clk_srcs[i]);
		}
		if(clk_srcs[i].flags & CLK_FLG_NOALLOC) {
			init_ext_pll(&clk_srcs[i]);
		}
		if(clk_srcs[i].flags & CLK_FLG_GATE) {
			init_gate_clk(&clk_srcs[i]);
		}
		if(clk_srcs[i].flags & CLK_FLG_ENABLE)
			clk_srcs[i].init_state = 1;

		if(clk_srcs[i].flags & CLK_FLG_PWC) {
			init_pwc_clk(&clk_srcs[i]);
		}
		clk_srcs[i].CLK_ID = i;
	}
	for(i = 0; i < clk_srcs_size; i++) {
		if(clk_srcs[i].parent && clk_srcs[i].init_state)
			init_clk_parent(clk_srcs[i].parent);
	}
	register_syscore_ops(&clk_pm_ops);
	register_pm_notifier(&clk_sleep_pm_notifier);
	printk("CCLK:%luMHz L2CLK:%luMhz H0CLK:%luMHz H2CLK:%luMhz PCLK:%luMhz\n",
			clk_srcs[CLK_ID_CCLK].rate/1000/1000,
			clk_srcs[CLK_ID_L2CLK].rate/1000/1000,
			clk_srcs[CLK_ID_H0CLK].rate/1000/1000,
			clk_srcs[CLK_ID_H2CLK].rate/1000/1000,
			clk_srcs[CLK_ID_PCLK].rate/1000/1000);
}
struct clk *clk_get(struct device *dev, const char *id)
{
	int i;
	struct clk *retval = NULL;
	struct clk *clk_srcs = get_clk_from_id(0);
	struct clk *parent_clk = NULL;
	for(i = 0; i < get_clk_sources_size(); i++) {
		if(id && clk_srcs[i].name && !strcmp(id,clk_srcs[i].name)) {
			if(clk_srcs[i].flags & CLK_FLG_NOALLOC)
				return &clk_srcs[i];
			retval = kzalloc(sizeof(struct clk),GFP_KERNEL);
			if(!retval)
				return ERR_PTR(-ENODEV);
			memcpy(retval,&clk_srcs[i],sizeof(struct clk));
			retval->source = &clk_srcs[i];
			if(CLK_FLG_RELATIVE & clk_srcs[i].flags)
			{
				parent_clk = get_clk_from_id(CLK_RELATIVE(clk_srcs[i].flags));
				parent_clk->child = NULL;
			}
			retval->count = 0;
			return retval;
		}
	}
	return ERR_PTR(-EINVAL);
}
EXPORT_SYMBOL(clk_get);

int clk_enable(struct clk *clk)
{
	if(!clk)
		return -EINVAL;

	if (!(strcmp(clk->name, "cgu_i2s"))) {
			clk->source->init_state = 0;
	}

	if(clk->source) {
		clk->count = 1;
		clk = clk->source;

		if(clk->init_state) {
			clk->count = 1;
			clk->init_state = 0;
			return 0;
		}
	}
	if(clk->count == 0) {
		if(clk->parent)
			clk_enable(clk->parent);
		if(clk->ops && clk->ops->enable) {
			clk->ops->enable(clk,1);
			if(clk->child)
				cpm_pwc_enable_ctrl(clk->child,1);
		}
		clk->flags |= CLK_FLG_ENABLE;
	}
	clk->count++;
	return 0;
}
EXPORT_SYMBOL(clk_enable);

int clk_is_enabled(struct clk *clk)
{
	if(clk->source)
		clk = clk->source;
	return !!(clk->flags & CLK_FLG_ENABLE);
}
EXPORT_SYMBOL(clk_is_enabled);
void clk_disable(struct clk *clk)
{
	if(!clk)
		return;
	if(clk->count == 0)
		return;
	if(clk->source) {
		clk->count = 0;
		clk = clk->source;
	}
	clk->count--;
	if(clk->count > 0) {
		return;
	}else
		clk->count = 0;
	if(!clk->count) {
		if(clk->child)
			cpm_pwc_enable_ctrl(clk->child,0);
		if(clk->ops && clk->ops->enable)
			clk->ops->enable(clk,0);
		clk->flags &= ~CLK_FLG_ENABLE;
		if(clk->parent)
			clk_disable(clk->parent);
	}
}
EXPORT_SYMBOL(clk_disable);

unsigned long clk_get_rate(struct clk *clk)
{
	if (!clk)
		return -EINVAL;
	if(clk->source)
		clk = clk->source;
	return clk? clk->rate: 0;
}
EXPORT_SYMBOL(clk_get_rate);

void clk_put(struct clk *clk)
{
	struct clk *parent_clk;
	if(clk && !(clk->flags & CLK_FLG_NOALLOC)) {
		if(clk->source && clk->count && clk->source->count > 0) {
			clk->source->count--;
			if(clk->source->count == 0)
				clk->source->init_state = 1;

		}
		if(CLK_FLG_RELATIVE & clk->source->flags)
		{
			parent_clk = get_clk_from_id(CLK_RELATIVE(clk->source->flags));
			parent_clk->child = clk->source;
		}
		kfree(clk);
	}
}
EXPORT_SYMBOL(clk_put);

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	int ret = 0;
	if (!clk)
		return -EINVAL;
	if(clk->source)
		clk = clk->source;
	if (!clk->ops || !clk->ops->set_rate)
		return -EINVAL;
	if(clk->rate != rate)
		ret = clk->ops->set_rate(clk, rate);
	return ret;
}
EXPORT_SYMBOL(clk_set_rate);

int clk_set_parent(struct clk *clk, struct clk *parent)
{
	int err;

	if (!clk)
		return -EINVAL;
	if(clk->source)
		clk = clk->source;
	if (!clk->ops || !clk->ops->set_rate)
		return -EINVAL;

	err = clk->ops->set_parent(clk, parent);
	clk->rate = clk->ops->get_rate(clk);
	return err;
}
EXPORT_SYMBOL(clk_set_parent);

struct clk *clk_get_parent(struct clk *clk)
{
	if (!clk)
		return NULL;
	return clk->source->parent;
}
EXPORT_SYMBOL(clk_get_parent);

static int clocks_show(struct seq_file *m, void *v)
{
	int i,len=0;
	struct clk *clk_srcs = get_clk_from_id(0);
	if(m->private != NULL) {
		len += seq_printf(m ,"CLKGR\t: %08x\n",cpm_inl(CPM_CLKGR));
		len += seq_printf(m ,"CLKGR1\t: %08x\n",cpm_inl(CPM_CLKGR1));
		len += seq_printf(m ,"LCR1\t: %08x\n",cpm_inl(CPM_LCR));
		len += seq_printf(m ,"PGR\t: %08x\n",cpm_inl(CPM_PGR));
		len += seq_printf(m ,"SPCR0\t: %08x\n",cpm_inl(CPM_SPCR0));
	} else {
		len += seq_printf(m,"ID NAME       FRE        stat       count     parent\n");
		for(i = 0; i < get_clk_sources_size(); i++) {
			if (clk_srcs[i].name == NULL) {
				len += seq_printf(m ,"--------------------------------------------------------\n");
			} else {
				unsigned int mhz = clk_srcs[i].rate / 1000;
				len += seq_printf(m,"%2d %-10s %4d.%03dMHz %3sable   %d %s\n",i,clk_srcs[i].name
						, mhz/1000, mhz%1000
						, clk_srcs[i].flags & CLK_FLG_ENABLE? "en": "dis"
						, clk_srcs[i].count
						, clk_srcs[i].parent? clk_srcs[i].parent->name: "root");
			}
		}
	}
	return len;
}
static int enable_write(struct file *file, const char __user *buffer,size_t count, loff_t *data)
{

	struct clk *clk = ((struct seq_file *)file->private_data)->private;
	if(clk) {
        struct clk *tmp = clk_get(NULL,clk->name);
		if(count && (buffer[0] == '1')){
			clk_enable(tmp);
		}
		else if(count && (buffer[0] == '0'))
			clk_disable(tmp);
		else
			printk("To control the state of an Ingenic SOC clock, use:\n"
				"\"echo 0 > /proc/jz/clock/[CLOCK_NAME]/enable\" to disable the clock\n"
				"\"echo 1 > /proc/jz/clock/[CLOCK_NAME]/enable\" to enable the clock\n");
		clk_put(tmp);
	}
	return count;
}

static int rate_write(struct file *file, const char __user *buffer,size_t count, loff_t *data)
{
	struct clk *clk = ((struct seq_file *)file->private_data)->private;
	long rate;
	if(clk) {
		if(kstrtol_from_user(buffer,count,0,&rate) >= 0) {
			struct clk *tmp = clk_get(NULL,clk->name);
			clk_set_rate(tmp,rate);
			clk_put(tmp);
		} else
			printk("\"usage : echo 100000000 > rate");
	}
	return count;
}

static int enable_show(struct seq_file *m, void *v)
{
	struct clk *clk = m->private;
	return seq_printf(m,"%s\n",clk_is_enabled(clk) ? "enabled" : "disabled");
}

static int rate_show(struct seq_file *m, void *v)
{
	struct clk *clk = m->private;
	return seq_printf(m,"rate: %ld\n",clk_get_rate(clk));
}

static int clocks_open(struct inode *inode, struct file *file)
{
	return single_open_size(file, clocks_show, PDE_DATA(inode),8192);
}

static int enable_open(struct inode *inode, struct file *file)
{
	return single_open(file, enable_show, PDE_DATA(inode));
}

static int rate_open(struct inode *inode, struct file *file)
{
	return single_open(file, rate_show, PDE_DATA(inode));
}

static const struct file_operations clocks_proc_fops ={
	.read = seq_read,
	.open = clocks_open,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations enable_fops ={
	.read = seq_read,
	.open = enable_open,
	.write = enable_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations rate_fops ={
	.read = seq_read,
        .open = rate_open,
	.write = rate_write,
	.llseek = seq_lseek,
	.release = single_release,
};
struct  freq_udelay_jiffy *freq_udelay_jiffys;
#ifdef CONFIG_CPU_FREQ
struct cpufreq_frequency_table *init_freq_table(unsigned int max_freq,
		     unsigned int min_freq)
{
	int i, j = 0;
	struct cpufreq_frequency_table *freq_table;
	unsigned int num = ARRAY_SIZE(set_cpu_freqs);

	if(max_freq <= 0 || min_freq <= 0)
		return NULL;
	freq_table = (struct cpufreq_frequency_table *)kzalloc(sizeof(struct cpufreq_frequency_table)
							       * num, GFP_KERNEL);
	if(!freq_table) {
		pr_err("%s: cpufreq_frequency_table kzalloc fail!!!\n", __func__);
		return NULL;
	}
	for(i = 0;i < num && set_cpu_freqs[i] <= max_freq; i++) {
		if(set_cpu_freqs[i] < min_freq)
			continue;
		freq_table[j].index = j;
		freq_table[j].frequency = set_cpu_freqs[i];
		j++;
	}
	if(j < num) {
		freq_table[j].index = j;
		freq_table[j].frequency = CPUFREQ_TABLE_END;
	}
	return freq_table;
}
EXPORT_SYMBOL(init_freq_table);
#endif
static void init_freq_udelay_jiffys(struct clk *cpu_clk, unsigned int num)
{
	int i;
	unsigned int rate;

	rate = clk_get_rate(cpu_clk)/1000;
	for(i = 0; i < num; i++) {
		freq_udelay_jiffys[i].cpufreq = set_cpu_freqs[i];
		freq_udelay_jiffys[i].udelay_val = cpufreq_scale(cpu_data[0].udelay_val,
								 rate, set_cpu_freqs[i]);
		freq_udelay_jiffys[i].loops_per_jiffy = cpufreq_scale(loops_per_jiffy,
								      rate, set_cpu_freqs[i]);
	}
	freq_udelay_jiffys[0].max_num = num;
	printk("freq_udelay_jiffys[0].max_num = %d\n", freq_udelay_jiffys[0].max_num);

	printk("cpufreq \tudelay \tloops_per_jiffy\t\n");
	for(i = 0; i < num; i++) {
		printk("%u\t %u\t %u\t\n", freq_udelay_jiffys[i].cpufreq,
		       freq_udelay_jiffys[i].udelay_val, freq_udelay_jiffys[i].loops_per_jiffy);
	}
}
static int __init init_clk_proc(void)
{
	int i=0;
	struct proc_dir_entry *p;
	struct proc_dir_entry *sub;
	struct clk *clk_srcs = get_clk_from_id(0);
	struct clk *cpu_clk;
	unsigned int num = ARRAY_SIZE(set_cpu_freqs) - 1;

	freq_udelay_jiffys = (struct freq_udelay_jiffy *)kzalloc(sizeof(struct freq_udelay_jiffy) *
								 num, GFP_KERNEL);
	if(!freq_udelay_jiffys) {
		pr_err("%s: freq_udelay_jiffys kzalloc fail!!!\n", __func__);
		return -1;
	}
	cpu_clk = clk_get(NULL, "cclk");
	if (IS_ERR(cpu_clk)) {
		printk("ERROR:cclk request fail!\n");
		return -1;
	}
	init_freq_udelay_jiffys(cpu_clk, num);
	clk_put(cpu_clk);

	p = jz_proc_mkdir("clock");
	if (!p) {
		pr_warning("create_proc_entry for common clock failed.\n");
		return -ENODEV;
	}
	proc_create_data("clocks", 0600,p,&clocks_proc_fops,0);
	proc_create_data("misc", 0600,p,&clocks_proc_fops,(void *)1);

	for(i = 0; i < get_clk_sources_size(); i++) {
		if (clk_srcs[i].name == NULL)
			continue;
		sub = proc_mkdir(clk_srcs[i].name,p);
		proc_create_data("enable", 0600,sub,&enable_fops,&clk_srcs[i]);
		proc_create_data("rate", 0600,sub,&rate_fops,&clk_srcs[i]);
	}
	return 0;
}

module_init(init_clk_proc);
