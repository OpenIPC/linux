/*
 * Clock and PLL control for FH devices
 *
 * Copyright (C) 2014 Fullhan Microelectronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <asm/bitops.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <mach/hardware.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>
#include <mach/clock.h>
#include <mach/clock.h>
#include <linux/platform_device.h>
#include <mach/pmu.h>

#define PROC_FILE   "driver/clock"

static LIST_HEAD(clocks);
static DEFINE_MUTEX(clocks_mutex);
static DEFINE_SPINLOCK(clocks_lock);

struct proc_dir_entry *proc_file;


#define   DIVVCO_ONE_DEVISION          0x0
#define   DIVVCO_TWO_DEVISION          0x8
#define   DIVVCO_FOUR_DEVISION         0xc
#define   DIVVCO_EIGHT_DEVISION        0xd
#define   DIVVCO_SIXTEEN_DEVISION      0xe
#define   DIVVCO_THIRTYTWO_DEVISION    0xf


/*#define FH_CLK_DEBUG*/

#if defined(FH_CLK_DEBUG)
	#define PRINT_CLK_DBG(fmt, args...)   \
		do {                               \
			printk("FH_CLK_DEBUG: ");     \
			printk(fmt, ##args);          \
		} while (0)
#else
	#define PRINT_CLK_DBG(fmt, args...) \
		do {                              \
		} while (0)
#endif


void clk_set_clk_sel(unsigned int reg)
{
	fh_pmu_set_reg(REG_PMU_CLK_SEL, reg);
}
EXPORT_SYMBOL(clk_set_clk_sel);

unsigned int clk_get_clk_sel(void)
{
	return fh_pmu_get_reg(REG_PMU_CLK_SEL);
}
EXPORT_SYMBOL(clk_get_clk_sel);

#ifdef FH_CLOCK_DEBUG
static void __clk_sel_ddr_clk(int source)
{
	unsigned int clk_sel;
	int shift = 24;
	clk_sel = clk_get_clk_sel();
	clk_sel &= ~(0x1 << shift);
	clk_sel |= (source & 0x1) << shift;
	clk_set_clk_sel(clk_sel);
}

static void __clk_sel_pix_clk(int source)
{
	unsigned int clk_sel;
	int shift = 4;
	clk_sel = clk_get_clk_sel();
	clk_sel &= ~(0x3 << shift);
	clk_sel |= (source & 0x3) << shift;
	clk_set_clk_sel(clk_sel);
}

static void __clk_sel_ac_clk(int source)
{
	unsigned int clk_sel;
	int shift = 0;
	clk_sel = clk_get_clk_sel();
	clk_sel &= ~(0x1 << shift);
	clk_sel |= (source & 0x1) << shift;
	clk_set_clk_sel(clk_sel);
}
#endif


static void fh_clk_enable(struct clk *clk)
{
	unsigned int reg;

	if (clk->flag & CLOCK_NOGATE) {
		PRINT_CLK_DBG("%s, %s has no gate register\n", __func__, clk->name);
		return;
	}

	reg = fh_pmu_get_reg(clk->en_reg_offset);
	PRINT_CLK_DBG("%s, clk: %s, reg: 0x%x\n", __func__, clk->name, reg);
	reg &= ~(clk->en_reg_mask);
	fh_pmu_set_reg(clk->en_reg_offset, reg);
	PRINT_CLK_DBG("%s, clk: %s, after mask: 0x%x\n", __func__, clk->name, reg);
}

static void fh_clk_disable(struct clk *clk)
{
	unsigned int reg;

	if (clk->flag & CLOCK_NOGATE) {
		PRINT_CLK_DBG("%s, %s has no gate register\n", __func__, clk->name);
		return;
	}

	reg = fh_pmu_get_reg(clk->en_reg_offset);
	reg |= clk->en_reg_mask;
	fh_pmu_set_reg(clk->en_reg_offset, reg);

	PRINT_CLK_DBG("%s, clk: %s, reg: 0x%x\n", __func__, clk->name, reg);
}

static int fh_clk_get_sel(struct clk *clk)
{
	unsigned int reg, shift;
	int ret;

	if (!(clk->flag & CLOCK_MULTI_PARENT))
		return 0;

	shift = ffs(clk->sel_reg_mask) - 1;
	reg = fh_pmu_get_reg(clk->sel_reg_offset);
	reg &= clk->sel_reg_mask;
	ret = reg >> shift;
	PRINT_CLK_DBG("%s, clk: %s, sel: %d\n", __func__, clk->name, ret);

	return ret;
}

static void fh_clk_set_sel(struct clk *clk, int sel)
{
	unsigned int reg, shift;

	if (!(clk->flag & CLOCK_MULTI_PARENT)) {
		PRINT_CLK_DBG("%s, clk: %s has only one parent\n", __func__, clk->name);
		return;
	}

	clk->select = sel;
	shift = ffs(clk->sel_reg_mask) - 1;
	reg = fh_pmu_get_reg(clk->sel_reg_offset);
	reg &= ~(clk->sel_reg_mask);
	reg |= (sel << shift);
	fh_pmu_set_reg(clk->sel_reg_offset, reg);
	PRINT_CLK_DBG("%s, clk: %s, select: %d, reg: 0x%x\n", __func__, clk->name, sel,
		      reg);
}

static unsigned long fh_clk_get_pll_rate(struct clk *clk)
{
	unsigned int reg, m, n, od, no = 1, i;

	reg = fh_pmu_get_reg(clk->div_reg_offset);
	m = reg & 0xff;
	n = (reg >> 8) & 0xf;
	od = (reg >> 16) & 0x3;

	for(i=0; i<od; i++)
		no *= 2;

	clk->frequency = OSC_FREQUENCY * m / n / no;

	return clk->frequency;
}

static unsigned long fh_clk_get_pll_p_rate(struct clk *clk)
{
	unsigned int reg, m, n, p, r = 1;
	unsigned int clk_vco, divvcop = 1, shift;

	reg = fh_pmu_get_reg(clk->div_reg_offset);

	m = reg & 0x7f;
	n = (reg >> 8) & 0x1f;
	p = (reg >> 16) & 0x3f;
	r = (reg >> 24) & 0x3f;

	/*pll databook*/
	if (m<4)
		m=128+m;

	if (m==0xb)
		m=0xa;

	shift = ffs(clk->en_reg_mask)-1;
	reg = fh_pmu_get_reg(clk->en_reg_offset);

	switch ((reg&clk->en_reg_mask)>>shift){
	case DIVVCO_ONE_DEVISION:
		divvcop=1;
		break;

	case DIVVCO_TWO_DEVISION:
		divvcop=2;
		break;

	case DIVVCO_FOUR_DEVISION:
		divvcop=4;
		break;

	case DIVVCO_EIGHT_DEVISION:
		divvcop=8;
		break;

	case DIVVCO_SIXTEEN_DEVISION:
		divvcop=16;
		break;

	case DIVVCO_THIRTYTWO_DEVISION:
		divvcop=32;
		break;
	default:
		printk("divvcop error:%x\n",divvcop);
	}

	clk_vco = OSC_FREQUENCY * m / (n+1);
	clk->frequency = clk_vco/ (p+1)/divvcop;
	return clk->frequency;
}

static unsigned long fh_clk_get_pll_r_rate(struct clk *clk)
{
	unsigned int reg, m, n, p, r = 1;
	unsigned int clk_vco,divvcor=1,shift;

	reg = fh_pmu_get_reg(clk->div_reg_offset);

	m = reg & 0x7f;
	n = (reg >> 8) & 0x1f;
	p = (reg >> 16) & 0x3f;
	r = (reg >> 24) & 0x3f;

	/*pll databook*/
		if(m<4)
			m=128+m;

		if(m==0xb)
			m=0xa;

	shift = ffs(clk->en_reg_mask)-1;
	reg = fh_pmu_get_reg(clk->en_reg_offset);

	switch ((reg&clk->en_reg_mask)>>shift){
	case DIVVCO_ONE_DEVISION:
		divvcor=1;
		break;

	case DIVVCO_TWO_DEVISION:
		divvcor=2;
		break;

	case DIVVCO_FOUR_DEVISION:
		divvcor=4;
		break;

	case DIVVCO_EIGHT_DEVISION:
		divvcor=8;
		break;

	case DIVVCO_SIXTEEN_DEVISION:
		divvcor=16;
		break;

	case DIVVCO_THIRTYTWO_DEVISION:
		divvcor=32;
		break;
	default:
		printk("divvcop error:%x\n", divvcor);
	}


	clk_vco = OSC_FREQUENCY * m / (n+1);
	clk->frequency = clk_vco/ (r+1)/divvcor;
	return clk->frequency;
}

static int fh_clk_get_div(struct clk *clk)
{
	unsigned int reg, shift;
	int ret;

	if (clk->flag & (CLOCK_NODIV | CLOCK_FIXED))
		return 0;

	shift = ffs(clk->div_reg_mask) - 1;
	reg = fh_pmu_get_reg(clk->div_reg_offset);
	PRINT_CLK_DBG("%s, clk: %s, reg: 0x%x\n", __func__, clk->name, reg);
	reg &= clk->div_reg_mask;
	PRINT_CLK_DBG("%s, clk: %s, shift: %d, after mask: 0x%x\n", __func__, clk->name,
		      shift, reg);
	ret = reg >> shift;
	PRINT_CLK_DBG("%s, clk: %s, div: %d\n", __func__, clk->name, ret);
	PRINT_CLK_DBG("%s, clk: %s, div_mask: 0x%x, div_offset: 0x%x\n",
		      __func__, clk->name, clk->div_reg_mask, clk->div_reg_offset);

	return ret;
}

static void fh_clk_set_div(struct clk *clk, int div)
{
	unsigned int reg, shift;

	if (clk->flag & CLOCK_NODIV) {
		PRINT_CLK_DBG("%s, clk: %s has no divide\n", __func__, clk->name);
		return;
	}

	shift = ffs(clk->div_reg_mask) - 1;

	if(div > clk->div_reg_mask >> shift) {
		pr_err("%s, clk: %s, curr div %d is too big, max is %d\n",
				__func__, clk->name, div, clk->div_reg_mask >> shift);
		return;
	}

	clk->divide = div;

	reg = fh_pmu_get_reg(clk->div_reg_offset);
	PRINT_CLK_DBG("%s, clk: %s, reg: 0x%x\n", __func__, clk->name, reg);
	reg &= ~(clk->div_reg_mask);
	reg |= (div << shift);
	PRINT_CLK_DBG("%s, clk: %s, shift: %d, after mask: 0x%x\n", __func__, clk->name,
		      shift, reg);
	fh_pmu_set_reg(clk->div_reg_offset, reg);
	PRINT_CLK_DBG("%s, clk: %s, div: %d, reg: 0x%x\n", __func__, clk->name, div,
		      reg);
	PRINT_CLK_DBG("%s, clk: %s, div_mask: 0x%x, div_offset: 0x%x\n",
		      __func__, clk->name, clk->div_reg_mask, clk->div_reg_offset);

}

unsigned long fh_clk_get_rate(struct clk *clk)
{
	if (clk->flag & CLOCK_FIXED) {
		PRINT_CLK_DBG("%s, clk: %s is fixed clock, rate: %lu\n", __func__, clk->name,
			      clk->frequency);
		return clk->frequency;
	}

	if (clk->flag & CLOCK_PLL) {
		PRINT_CLK_DBG("%s, clk: %s is a PLL clock\n", __func__, clk->name);
		return fh_clk_get_pll_rate(clk);
	}

	if (clk->flag & CLOCK_PLL_P) {
		PRINT_CLK_DBG("%s, clk: %s is a PLL clock\n", __func__, clk->name);
		return fh_clk_get_pll_p_rate(clk);
	}

	if (clk->flag & CLOCK_PLL_R) {
		PRINT_CLK_DBG("%s, clk: %s is a PLL clock\n", __func__, clk->name);
		return fh_clk_get_pll_r_rate(clk);
	}

	if (clk->flag & CLOCK_CIS && (0 == fh_clk_get_sel(clk))) {
		clk->frequency = OSC_FREQUENCY;
		return clk->frequency;
	}

	clk->select = fh_clk_get_sel(clk);
	clk->divide = fh_clk_get_div(clk) + 1;

	if (clk->select > CLOCK_MAX_PARENT) {
		pr_err("ERROR, %s, clk: %s, select is not correct, clk->select: %d\n", __func__,
		       clk->name, clk->select);
		return 0;
	}

	if (!clk->parent[clk->select]) {
		pr_err("ERROR, %s, clk: %s has no parent and is not a fixed clock\n", __func__,
		       clk->name);
		return 0;
	}

	clk->frequency = clk->parent[clk->select]->frequency / clk->prediv;
	clk->frequency /= clk->divide;

	PRINT_CLK_DBG("%s, clk: %s, rate: %lu\n", __func__, clk->name, clk->frequency);

	return clk->frequency;
}

void fh_clk_set_rate(struct clk *clk, unsigned long rate)
{
	if (clk->flag & CLOCK_FIXED) {
		pr_err("%s, clk: %s is fixed clock, rate: %lu\n", __func__, clk->name,
		       clk->frequency);
		return;
	}

	if (clk->flag & (CLOCK_PLL | CLOCK_PLL_R | CLOCK_PLL_P)) {
		pr_err("%s, clk: %s is a PLL clock, changing frequency is not recommended\n",
				__func__, clk->name);
		return;
	}
	if (clk->select > CLOCK_MAX_PARENT) {
		pr_err("ERROR, %s, clk: %s, select is not correct, clk->select: %d\n", __func__,
		       clk->name, clk->select);
		return;
	}

	if (!clk->parent[clk->select]) {
		pr_err("ERROR, %s, clk: %s has no parent and is not a fixed clock\n", __func__,
		       clk->name);
		return;
	}
	if (clk->flag & CLOCK_CIS) {

		if (rate == OSC_FREQUENCY) {
			fh_clk_set_sel(clk, 0);
			clk->frequency = rate;
			return;
		} else
			fh_clk_set_sel(clk, 1);
	}

	clk->frequency = clk->parent[clk->select]->frequency / clk->prediv;
	clk->divide = clk->frequency / rate;
	PRINT_CLK_DBG("%s, clk: %s, set rate: %lu, divide: %d\n", __func__, clk->name,
		      rate, clk->divide);
	fh_clk_set_div(clk, clk->divide - 1);

	clk->frequency = rate;

	PRINT_CLK_DBG("%s, clk: %s, rate: %lu\n", __func__, clk->name, clk->frequency);
}

void fh_clk_reset(struct clk *clk)
{
	unsigned int reg;

	if (clk->flag & CLOCK_NORESET) {
		pr_err("%s, clk: %s has no reset\n", __func__, clk->name);
		return;
	}

	reg = 0xffffffff & ~(clk->rst_reg_mask);

	fh_pmu_set_reg(clk->rst_reg_offset, reg);
	while (fh_pmu_get_reg(clk->rst_reg_offset) != 0xffffffff) {

	}
	PRINT_CLK_DBG("%s, clk: %s has been reset\n", __func__, clk->name);
}

int clk_enable(struct clk *clk)
{
	unsigned long flags;

	if (clk == NULL || IS_ERR(clk))
		return -EINVAL;

	spin_lock_irqsave(&clocks_lock, flags);
	if(clk->flag & (CLOCK_PLL_R | CLOCK_PLL_P))
		fh_clk_disable(clk);
	else
		fh_clk_enable(clk);
	spin_unlock_irqrestore(&clocks_lock, flags);

	return 0;
}
EXPORT_SYMBOL(clk_enable);

void clk_disable(struct clk *clk)
{
	unsigned long flags;

	if (clk == NULL || IS_ERR(clk))
		return;

	spin_lock_irqsave(&clocks_lock, flags);
	if (clk->flag & (CLOCK_PLL_R | CLOCK_PLL_P))
		fh_clk_enable(clk);
	else
		fh_clk_disable(clk);
	spin_unlock_irqrestore(&clocks_lock, flags);
}
EXPORT_SYMBOL(clk_disable);


unsigned long clk_get_rate(struct clk *clk)
{
	unsigned long flags, rate;
	if (clk == NULL || IS_ERR(clk))
		return -EINVAL;

	spin_lock_irqsave(&clocks_lock, flags);

	rate = fh_clk_get_rate(clk);

	spin_unlock_irqrestore(&clocks_lock, flags);

	return rate;
}
EXPORT_SYMBOL(clk_get_rate);


int clk_set_rate(struct clk *clk, unsigned long rate)
{
	unsigned long flags, real_rate;
	int ret = -EINVAL;

	if (clk == NULL || IS_ERR(clk))
		return ret;

	spin_lock_irqsave(&clocks_lock, flags);
	fh_clk_set_rate(clk, rate);
	real_rate = clk_get_rate(clk);
	if (rate != real_rate)
		printk("WARN: set clk %s to %ld, but get %ld\n", clk->name, rate, real_rate);

	spin_unlock_irqrestore(&clocks_lock, flags);

	return 0;
}
EXPORT_SYMBOL(clk_set_rate);

void clk_reset(struct clk *clk)
{
	unsigned long flags;

	if (clk == NULL || IS_ERR(clk))
		return;

	spin_lock_irqsave(&clocks_lock, flags);
	fh_clk_reset(clk);
	spin_unlock_irqrestore(&clocks_lock, flags);
}
EXPORT_SYMBOL(clk_reset);

void clk_change_parent(struct clk *clk, int select)
{
	unsigned long flags;

	if (clk == NULL || IS_ERR(clk))
		return;

	spin_lock_irqsave(&clocks_lock, flags);
	fh_clk_set_sel(clk, select);
	spin_unlock_irqrestore(&clocks_lock, flags);
}
EXPORT_SYMBOL(clk_change_parent);

int clk_register(struct clk *clk)
{
	if (clk == NULL || IS_ERR(clk))
		return -EINVAL;

	if (WARN(clk->parent[clk->select] && !clk->parent[clk->select]->frequency,
		 "CLK: %s parent %s has no rate!\n",
		 clk->name, clk->parent[clk->select]->name))
		return -EINVAL;

	clk_get_rate(clk);

	PRINT_CLK_DBG("clk: %s has been registered, div: %d, sel: %d\n",
		      clk->name, clk->divide, clk->select);

	mutex_lock(&clocks_mutex);
	list_add_tail(&clk->list, &clocks);
	mutex_unlock(&clocks_mutex);

	return 0;
}
EXPORT_SYMBOL(clk_register);

void clk_unregister(struct clk *clk)
{
	if (clk == NULL || IS_ERR(clk))
		return;

	mutex_lock(&clocks_mutex);
	list_del(&clk->list);
	mutex_unlock(&clocks_mutex);
}
EXPORT_SYMBOL(clk_unregister);


static void del_char(char* str, char ch)
{
	char *p = str;
	char *q = str;
	while (*q) {
		if (*q != ch) {
			*p++ = *q;
		}
		q++;
	}
	*p = '\0';
}

static ssize_t fh_clk_proc_write(struct file *filp, const char *buf, size_t len, loff_t *off)
{
	int i, ret;
	char message[64] = {0};
	char * const delim = ",";
	char *cur = message;
	char *param_str[4];
	unsigned int param[4];
	struct clk *clk;

	len = (len > 64) ? 64 : len;

	if (copy_from_user(message, buf, len))
		return -EFAULT;

	for (i = 0; i < 3; i++) {
		param_str[i] = strsep(&cur, delim);
		if (!param_str[i]) {
			pr_err("%s: ERROR: parameter[%d] is empty\n", __func__, i);
			pr_err("[clk name], [enable/disable], [clk rate]\n");
			return -EINVAL;
		} else {
			del_char(param_str[i], ' ');
			del_char(param_str[i], '\n');
		}
	}

	clk = clk_get(NULL, param_str[0]);
	if (!clk) {
		pr_err("%s: ERROR: clk %s is not found\n", __func__, param_str[0]);
		pr_err("[clk name], [enable/disable], [clk rate]\n");
		return -EINVAL;
	}

	param[2] = (u32)simple_strtoul(param_str[2], NULL, 10);
	if (param[2] < 0) {
		pr_err("ERROR: parameter[2] is incorrect\n");
		return -EINVAL;
	}

	ret = clk_set_rate(clk, param[2]);
	if (ret) {
		pr_err("set clk rate failed\n, ret=%d\n", ret);
	}

	if (!strcmp(param_str[1], "enable")) {
		clk_enable(clk);
		printk("clk %s enabled\n", param_str[0]);
	}
	else if (!strcmp(param_str[1], "disable")) {
		clk_disable(clk);
		printk(KERN_ERR "clk %s disabled\n", param_str[0]);
	} else {
		pr_err("%s: ERROR: parameter[1]:%s is incorrect\n",
				__func__, param_str[1]);
		pr_err("[clk name], [enable/disable], [clk rate]\n");
		return -EINVAL;
	}

	return len;
}


static void *v_seq_start(struct seq_file *s, loff_t *pos)
{
	static unsigned long counter = 0;
	if (*pos == 0)
		return &counter;
	else {
		*pos = 0;
		return NULL;
	}
}

static void *v_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	(*pos)++;
	return NULL;
}

static void v_seq_stop(struct seq_file *s, void *v)
{

}

static int v_seq_show(struct seq_file *sfile, void *v)
{

	struct clk_lookup *clock_lookup;
	struct clk *clk;
	unsigned long rate;
	unsigned int reg;
	char gate[10] = {0};
	seq_printf(sfile, "\nPLL Information: \n");
	for (clock_lookup = fh_clks; clock_lookup->clk; clock_lookup++) {
		clk = clock_lookup->clk;

		if (clk->flag & CLOCK_HIDE)
			continue;

		rate = clk_get_rate(clk);
		if (!(clk->flag & CLOCK_NOGATE)) {
			reg = fh_pmu_get_reg(clk->en_reg_offset);
			reg &= clk->en_reg_mask;
			if (reg) {
				if (clk->flag & (CLOCK_PLL_P|CLOCK_PLL_R))
					strncpy(gate, "enable", sizeof(gate));
				else
					strncpy(gate, "disable", sizeof(gate));
			} else {
				if (clk->flag & (CLOCK_PLL_P|CLOCK_PLL_R))
					strncpy(gate, "disable", sizeof(gate));
				else
					strncpy(gate, "enable", sizeof(gate));
			}
		} else {
			strncpy(gate, "nogate", sizeof(gate));
		}
		seq_printf(sfile, "\t%-20s \t%9luHZ \t%-10s\n",
				clk->name, rate, gate);
}
	return 0;
}

static const struct seq_operations fh_clk_seq_ops = {
	.start = v_seq_start,
	.next = v_seq_next,
	.stop = v_seq_stop,
	.show = v_seq_show
};

static int fh_clk_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &fh_clk_seq_ops);
}


static struct file_operations fh_clk_proc_ops = {
	.owner		= THIS_MODULE,
	.open		= fh_clk_proc_open,
	.read		= seq_read,
	.write		= fh_clk_proc_write,
	.release	= seq_release,
};

int __init fh_clk_procfs_init(void)
{
	proc_file = create_proc_entry(PROC_FILE, 0644, NULL);
	if (proc_file)
		proc_file->proc_fops = &fh_clk_proc_ops;
	else
		pr_err("clock, create proc fs failed\n");

	return 0;
}

int __init fh_clk_init(void)
{
	struct clk_lookup *clock_lookup;
	struct clk *clk;
	size_t num_clocks = 0;

	for (clock_lookup = fh_clks; clock_lookup->clk; clock_lookup++) {
		clk = clock_lookup->clk;
		num_clocks++;
		clk_register(clk);
		if (clk->def_rate)
			clk_set_rate(clk, clk->def_rate);
	}
	clkdev_add_table(fh_clks, num_clocks);
	return 0;
}
