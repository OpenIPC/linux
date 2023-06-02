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
#include <mach/io.h>
#include <asm/bitops.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <mach/pmu.h>
#include <mach/clock.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include "soc.h"


#define PROC_FILE   "driver/clock"

static DEFINE_MUTEX(clocks_mutex);
static DEFINE_SPINLOCK(clocks_lock);

struct proc_dir_entry *proc_file;

struct fh_clk_divider {
	struct clk_hw	hw;
	u32         	reg;
	u32           prediv;
	u32         div_flag;
	u32     div_reg_mask;
	spinlock_t	   *lock;
};
struct fh_clk_pllpr {
	struct clk_hw	hw;
	u32         	reg_ctrl0;
	u32             reg_ctrl1;
	u32             covpr_mask;
	u32             flag;
	spinlock_t	   *lock;
};

struct fh_clk_phase {
	struct clk_hw	  hw;
	u32              reg;
	u32             mux;
	spinlock_t	   *lock;
};

/*#define FH_CLK_DEBUG*/
#define   DIVVCO_ONE_DEVISION          0x0
#define   DIVVCO_TWO_DEVISION          0x8
#define   DIVVCO_FOUR_DEVISION         0xc
#define   DIVVCO_EIGHT_DEVISION        0xd
#define   DIVVCO_SIXTEEN_DEVISION      0xe
#define   DIVVCO_THIRTYTWO_DEVISION    0xf

static unsigned long fh_clk_get_pll_pr_rate(struct clk_hw *hw,
						unsigned long parent_rate)
{
	unsigned int reg, m, n, p, r = 1;
	unsigned int clk_vco, divvcop = 1, shift;
	u32 rate;
	struct fh_clk_pllpr *pllpr = (struct fh_clk_pllpr *)hw;

	reg = fh_pmu_get_reg(pllpr->reg_ctrl0);

	m = reg & 0x7f;
	n = (reg >> 8) & 0x1f;
	p = (reg >> 16) & 0x3f;
	r = (reg >> 24) & 0x3f;

	/*pll databook*/
	if (m<4)
		m=128+m;

	if (m==0xb)
		m=0xa;

	shift = ffs(pllpr->covpr_mask)-1;
	reg = fh_pmu_get_reg(pllpr->reg_ctrl1);

	switch ((reg&pllpr->covpr_mask)>>shift){
	case DIVVCO_ONE_DEVISION:
		divvcop = 1;
		break;

	case DIVVCO_TWO_DEVISION:
		divvcop = 2;
		break;

	case DIVVCO_FOUR_DEVISION:
		divvcop = 4;
		break;

	case DIVVCO_EIGHT_DEVISION:
		divvcop = 8;
		break;

	case DIVVCO_SIXTEEN_DEVISION:
		divvcop = 16;
		break;

	case DIVVCO_THIRTYTWO_DEVISION:
		divvcop = 32;
		break;
	default:
		pr_err("divvcop error:%x\n", divvcop);
	}

	clk_vco = parent_rate * m / (n+1);
	if (pllpr->flag & CLOCK_PLL_P)
		rate = clk_vco / (p+1) / divvcop;
	else
		rate = clk_vco / (r+1) / divvcop;
	return rate;
}
static const struct clk_ops clk_pll_pr_ops = {
	.recalc_rate = fh_clk_get_pll_pr_rate,
};


static unsigned long clk_pll_recalc_rate(struct clk_hw *hw,
						unsigned long parent_rate)
{
		unsigned int m = 0, n = 0, no = 1;
		u32 reg;
		u32 rate;
		struct fh_clk_divider *div = (struct fh_clk_divider *)hw;

		/* Fetch the register value */
		reg = fh_pmu_get_reg(div->reg);
		m = reg & 0xff;
		n = ((reg & (0xf00)) >> 8);
		no = ((reg & (0x30000)) >> 16);
		rate = ((parent_rate) * m / n) >> no;
		return rate;

}

static long clk_pll_round_rate(struct clk_hw *hw, unsigned long rate,
					unsigned long *parent_rate)
	{
		unsigned int m = 0, n = 0, no = 1;
		u32 reg;
		struct fh_clk_divider *div = (struct fh_clk_divider *)hw;

		/* Fetch the register value */
		reg = fh_pmu_get_reg(div->reg);
		m = reg & 0xff;
		n = ((reg & (0xf00)) >> 8);
		no = ((reg & (0x30000)) >> 16);
		rate = ((*parent_rate) * m / n) >> no;
		return rate;

	}


static const struct clk_ops clk_pll_ops = {
	.recalc_rate = clk_pll_recalc_rate,
	.round_rate = clk_pll_round_rate,
};

int fh_pll_clk_register(struct fh_clk *fh_clk)
{
	struct clk *clk;
	const char *clk_name = fh_clk->name;
	const char *parents[CLOCK_MAX_PARENT];
	struct fh_clk_divider *div = NULL;
	int i = 0;
	
	/* if we have a mux, we will have >1 parents */
	while (i < CLOCK_MAX_PARENT && fh_clk->parent[i] != NULL) {
		parents[i] = fh_clk->parent[i]->name;
		i++;
	}
	div = kzalloc(sizeof(struct fh_clk_divider), GFP_KERNEL);
	if (!div)
		return 0;
	div->reg = fh_clk->div_reg_offset;
	clk = clk_register_composite(NULL, clk_name,
			parents, i,
			NULL, NULL,
			&div->hw, &clk_pll_ops,
			NULL, NULL, CLK_IGNORE_UNUSED);
	if (!IS_ERR(clk))
		clk_register_clkdev(clk, clk_name, NULL);
	return 1;

}

int fh_pll_pr_clk_register(struct fh_clk *fh_clk)
{
	struct clk *clk;
	const char *clk_name = fh_clk->name;
	const char *parents[CLOCK_MAX_PARENT];
	struct fh_clk_pllpr *pll_ctrl = NULL;
	int i = 0;

	/* if we have a mux, we will have >1 parents */
	while (i < CLOCK_MAX_PARENT && fh_clk->parent[i] != NULL) {
		parents[i] = fh_clk->parent[i]->name;
		i++;
	}
	pll_ctrl = kzalloc(sizeof(struct fh_clk_pllpr), GFP_KERNEL);
	if (!pll_ctrl)
		return 0;
	pll_ctrl->reg_ctrl0 = fh_clk->div_reg_offset;
	pll_ctrl->reg_ctrl1 = fh_clk->en_reg_offset;
	pll_ctrl->covpr_mask = fh_clk->en_reg_mask;
	pll_ctrl->flag       = fh_clk->flag;
	clk = clk_register_composite(NULL, clk_name,
			parents, i,
			NULL, NULL,
			&pll_ctrl->hw, &clk_pll_pr_ops,
			NULL, NULL, CLK_IGNORE_UNUSED);
	if (!IS_ERR(clk))
		clk_register_clkdev(clk, clk_name, NULL);
	return 1;

}

static int fh_clk_set_phase(struct clk_hw *hw,
						int degree)
{
	u32 reg;
	struct fh_clk_phase *phase = (struct fh_clk_phase *)hw;
	unsigned long flags = 0;
	u32 local_degree = 0;
	u32 shift = 0;

    /*printk("fh_clk_set_phase:%d\n",degree);*/
	if (phase->lock)
		spin_lock_irqsave(phase->lock, flags);

		/* Fetch the register value */
		reg = fh_pmu_get_reg(phase->reg);

		local_degree = degree;

		shift = ffs(phase->mux)-1;

		reg |= (local_degree << shift);

		/* Apply them now */
		fh_pmu_set_reg(phase->reg, reg);
	if (phase->lock)
		spin_unlock_irqrestore(phase->lock, flags);

	return 1;
}

static int fh_clk_get_phase(struct clk_hw *hw)
{
	u32 reg;
	struct fh_clk_phase *phase = (struct fh_clk_phase *)hw;
	unsigned long flags = 0;
	u32 local_degree = 0;
	u32 shift;

	if (phase->lock)
		spin_lock_irqsave(phase->lock, flags);

		/* Fetch the register value */
		reg = fh_pmu_get_reg(phase->reg);
		shift = ffs(phase->mux)-1;

		reg = reg&(phase->mux) >> shift;
		local_degree = reg;

	 /*printk("fh_clk_get_phase:%d\n",local_degree);*/

	if (phase->lock)
		spin_unlock_irqrestore(phase->lock, flags);

	return local_degree;
}


static const struct clk_ops fh_clk_phase_ops = {
	.set_phase = fh_clk_set_phase,
	.get_phase = fh_clk_get_phase,
};

int fh_phase_clk_register(struct fh_clk *fh_clk)
{
	struct clk *clk;
	const char *clk_name = fh_clk->name;
	const char *parents[CLOCK_MAX_PARENT];
	struct fh_clk_phase *clk_phase = NULL;
	int i = 0;
	struct clk_init_data *init = NULL;
	
	/* if we have a mux, we will have >1 parents */
	while (i < CLOCK_MAX_PARENT && fh_clk->parent[i] != NULL) {
		parents[i] = fh_clk->parent[i]->name;
		i++;
	}
	clk_phase = kzalloc(sizeof(struct fh_clk_phase), GFP_KERNEL);
	if (!clk_phase)
		return 0;
	clk_phase->mux = fh_clk->sel_reg_mask;
	init = kzalloc(sizeof(struct clk_init_data), GFP_KERNEL);

	/* set up gate properties */
	clk_phase->reg = fh_clk->sel_reg_offset;
	clk_phase->lock = &clocks_lock;
	init->ops = &fh_clk_phase_ops;
	init->parent_names = parents;
	init->num_parents = i;
	init->name = clk_name;
	clk_phase->hw.init = init;
	clk = clk_register(NULL, &clk_phase->hw);
	if (!IS_ERR(clk)) {
		clk_register_clkdev(clk, clk_name, NULL);
	}
	return 1;

}


int fh_fix_clk_register(struct fh_clk *fh_clk)
{
	struct clk *clk;
	u32 out_values = fh_clk->frequency;
	const char *clk_name = fh_clk->name;

	clk = clk_register_fixed_rate(NULL, clk_name,
				      NULL,
				      CLK_IS_ROOT,
				      out_values);
	if (!IS_ERR(clk)) {;
		clk_register_clkdev(clk, clk_name, NULL);
	}
	return 0;
}

static unsigned long fh_clk_divide_recalc_rate(struct clk_hw *hw,
						unsigned long parent_rate)
{
	u32 reg;
	u32 divide = 0;
	struct fh_clk_divider *factors = (struct fh_clk_divider *)hw;
	unsigned long rate;
	unsigned long shift;
	unsigned long flags = 0;

	if (factors->lock)
		spin_lock_irqsave(factors->lock, flags);
	if (factors->div_flag) {
		/* Fetch the register value */
		reg = fh_pmu_get_reg(factors->reg);

		/* Get each individual factor if applicable */
		shift = ffs(factors->div_reg_mask)-1;
		divide = (factors->div_reg_mask & reg)>> shift;

		/* Calculate the rate */
		rate = (parent_rate) / (divide+1) / factors->prediv;
	} else
		rate = parent_rate / factors->prediv;
	if (factors->lock)
		spin_unlock_irqrestore(factors->lock, flags);

	return rate;
}

static long fh_clk_divide_round_rate(struct clk_hw *hw, unsigned long rate,
					unsigned long *parent_rate)
{
	u32 reg;
	u32 rount_rate = 0, divide;
	struct fh_clk_divider *factors = (struct fh_clk_divider *)hw;
	unsigned long flags = 0;

	if (factors->lock)
		spin_lock_irqsave(factors->lock, flags);

	if (factors->div_flag) {

		/* Fetch the register value */
		reg = fh_pmu_get_reg(factors->reg);

		/* Calculate the rate */
		divide = (*parent_rate) / factors->prediv / rate;
		rount_rate = (*parent_rate) / divide / factors->prediv;
	} else
	pr_err("fh_clk_divide_round_rate not support divide\n");

	if (factors->lock)
		spin_unlock_irqrestore(factors->lock, flags);

	return rount_rate;
}

static int fh_clk_divide_set_rate(struct clk_hw *hw, unsigned long rate,
						unsigned long parent_rate)
{
	u32 reg;
	struct fh_clk_divider *factors = (struct fh_clk_divider *)hw;
	u32 divide = 0;
	unsigned long flags = 0;
	unsigned long shift = 0;

	if (factors->lock)
		spin_lock_irqsave(factors->lock, flags);


	if (factors->div_flag) {

		/* Fetch the register value */
		reg = fh_pmu_get_reg(factors->reg);

		divide = parent_rate/rate/factors->prediv - 1;
		/*printk("fh_clk_divide_set_rate:%x\n",divide);*/

		shift = ffs(factors->div_reg_mask)-1;


		reg &= ~(factors->div_reg_mask);
		reg |= ((divide << shift) & factors->div_reg_mask);

		/* Apply them now */
		fh_pmu_set_reg(factors->reg,reg);

	}
	if (factors->lock)
		spin_unlock_irqrestore(factors->lock, flags);

	return 0;
}


static const struct clk_ops fh_clk_divider_ops = {
	.recalc_rate = fh_clk_divide_recalc_rate,
	.round_rate = fh_clk_divide_round_rate,
	.set_rate = fh_clk_divide_set_rate,
};

int fh_complex_clk_register(struct fh_clk *fh_clk)
{
	struct  clk *clk;
	struct  clk_gate *gate = NULL;
	struct  clk_hw   *clk_mux_hw = NULL;
	struct  clk_mux  *mux = NULL;
	struct  clk_hw  *clk_gate_hw = NULL;
	struct  fh_clk_divider *div = NULL;
	struct  clk_hw *clk_divider_hw = NULL;
	const char *clk_name;
	const char *parents[CLOCK_MAX_PARENT];
	int i = 0;

	/* if we have a mux, we will have >1 parents */
	while (i < CLOCK_MAX_PARENT && fh_clk->parent[i] != NULL) {
		parents[i] = fh_clk->parent[i]->name;
		i++;
	}

	clk_name = fh_clk->name;
	/* Leaves can be fixed or configurable divisors */
	if ((!(fh_clk->flag & CLOCK_NODIV)) || (fh_clk->prediv > 1)) {
		div = kzalloc(sizeof(struct fh_clk_divider), GFP_KERNEL);
		if (!div)
			return 0;
		clk_divider_hw = &div->hw;
		div->reg = fh_clk->div_reg_offset;
		if(!(fh_clk->flag & CLOCK_NODIV))
			div->div_flag = 1;
	    div->div_reg_mask = fh_clk->div_reg_mask;
		div->prediv = fh_clk->prediv;
		div->lock = &clocks_lock;
	}

    /* Add a gate if this factor clock can be gated */
	if (!(fh_clk->flag & CLOCK_NOGATE)) {
		gate = kzalloc(sizeof(struct clk_gate), GFP_KERNEL);
		if (!gate) {
			kfree(div);
			return 0;
		}
		/* set up gate properties */
		gate->reg =  (void __iomem *)(fh_clk->en_reg_offset + VA_PMU_REG_BASE);
		gate->bit_idx =ffs(fh_clk->en_reg_mask)-1;
		gate->lock = &clocks_lock;
		gate->flags = CLK_GATE_SET_TO_DISABLE;
		clk_gate_hw = &gate->hw;
	}

	if (fh_clk->flag & CLOCK_MULTI_PARENT) {
		mux = kzalloc(sizeof(struct clk_mux), GFP_KERNEL);


		/* set up gate properties */
		mux->reg = (void __iomem *)(fh_clk->sel_reg_offset + VA_PMU_REG_BASE);
		mux->shift = ffs(fh_clk->sel_reg_mask)-1;
		mux->mask = fh_clk->sel_reg_mask>>mux->shift;
		mux->lock = &clocks_lock;
		clk_mux_hw = &mux->hw;
	}
	clk = clk_register_composite(NULL, clk_name,
			parents, i,
			clk_mux_hw, &clk_mux_ops,
			clk_divider_hw, &fh_clk_divider_ops,
			clk_gate_hw, &clk_gate_ops, CLK_IGNORE_UNUSED);
	if (!IS_ERR(clk)) {
		clk_register_clkdev(clk, clk_name, NULL);
	}
	return 1;
}


int fh_clk_register(struct fh_clk *clk)
{
	if (clk == NULL || IS_ERR(clk))
		return -EINVAL;
	
	if(clk->flag & CLOCK_PLL) {
		fh_pll_clk_register(clk);
		return 0;
		}
	if(clk->flag & (CLOCK_PLL_P|CLOCK_PLL_R)) {
		fh_pll_pr_clk_register(clk);
		return 0;
		}
	if(clk->flag & CLOCK_FIXED) {
		fh_fix_clk_register(clk);
		return 0;
		}
	if(clk->flag & CLOCK_PHASE) {
		fh_phase_clk_register(clk);
		return 0;
		}
	fh_complex_clk_register(clk);
	return 0;
}
EXPORT_SYMBOL(fh_clk_register);


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
	if (!clk || IS_ERR(clk)) {
		pr_err("%s: ERROR: clk %s is not found\n", __func__, param_str[0]);
		pr_err("[clk name], [enable/disable], [clk rate]\n");
		return -EINVAL;
	}

	param[2] = (u32)simple_strtoul(param_str[2], NULL, 10);
	if (param[2] < 0) {
		pr_err("ERROR: parameter[2] is incorrect\n");
		return -EINVAL;
	}

	if (!strcmp(param_str[1], "enable")) {
		clk_prepare_enable(clk);
		printk("clk %s enabled\n", param_str[0]);
	}
	else if (!strcmp(param_str[1], "disable")) {
		clk_disable_unprepare(clk);
		printk(KERN_ERR "clk %s disabled\n", param_str[0]);
		return len;
	} else {
		pr_err("%s: ERROR: parameter[1]:%s is incorrect\n",
				__func__, param_str[1]);
		pr_err("[clk name], [enable/disable], [clk rate]\n");
		return -EINVAL;
	}

	ret = clk_set_rate(clk, param[2]);
	if (ret)
		pr_err("set clk rate failed\n, ret=%d\n", ret);

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

	struct fh_clk *fh_clk;
	struct clk *clk;
	unsigned long rate;
	unsigned int reg;
	char gate[10] = {0};
	int i = 0;
	seq_printf(sfile, "\nPLL Information: \n");
	for (; fh_clks[i]; i++) {
		fh_clk = fh_clks[i];
		if (fh_clk->flag & CLOCK_HIDE)
			continue;
		clk = clk_get(NULL, fh_clk->name);
		if (clk == NULL || IS_ERR(clk))
			continue;
		rate = clk_get_rate(clk);
		if (!(fh_clk->flag & CLOCK_NOGATE)) {
			reg = fh_pmu_get_reg(fh_clk->en_reg_offset);
			reg &= fh_clk->en_reg_mask;
			if (reg) {
				if (fh_clk->flag & (CLOCK_PLL_P|CLOCK_PLL_R))
					strncpy(gate, "enable", sizeof(gate));
				else
					strncpy(gate, "disable", sizeof(gate));
			} else {
				if (fh_clk->flag & (CLOCK_PLL_P|CLOCK_PLL_R))
					strncpy(gate, "disable", sizeof(gate));
				else
					strncpy(gate, "enable", sizeof(gate));
			}
		} else {
			strncpy(gate, "nogate", sizeof(gate));
		}
		seq_printf(sfile, "\t%-20s \t%9luHZ \t%-10s\n",
				fh_clk->name, rate, gate);
		clk_put(clk);
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
	proc_file = proc_create(PROC_FILE, 0644, NULL, &fh_clk_proc_ops);
	if (!proc_file)
		pr_err("clock, create proc fs failed\n");

	return 0;
}
module_init(fh_clk_procfs_init);

int __init fh_clk_init(void)
{
	struct fh_clk *fh_clk;
	struct clk *clk;
	int i = 0;
	for (; fh_clks[i]; i++) {
		fh_clk = fh_clks[i];
		fh_clk_register(fh_clk);
		if (fh_clk->def_rate) {
			clk = clk_get(NULL, fh_clk->name);
			if(clk && !IS_ERR(clk)) {
				clk_set_rate(clk, fh_clk->def_rate);
				clk_put(clk);
			}
		}
	}
	return 0;
}
