/*
 * arch/arm/mach-ambarella/clk.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/clk.h>
#include <asm/uaccess.h>
#include <mach/hardware.h>
#include <plat/iav_helper.h>
#include <plat/sd.h>
#include <plat/clk.h>

/* ==========================================================================*/
static LIST_HEAD(ambarella_all_clocks);
DEFINE_SPINLOCK(ambarella_clock_lock);
struct ambarella_service pll_service;

/* ==========================================================================*/
static unsigned int ambarella_clk_ref_freq = REF_CLK_FREQ;

unsigned int ambarella_clk_get_ref_freq(void)
{
	return ambarella_clk_ref_freq;
}
EXPORT_SYMBOL(ambarella_clk_get_ref_freq);

/* ==========================================================================*/

struct clk_ops ambarella_rct_scaler_ops = {
	.enable		= NULL,
	.disable	= NULL,
	.get_rate	= ambarella_rct_scaler_get_rate,
	.round_rate	= NULL,
	.set_rate	= ambarella_rct_scaler_set_rate,
	.set_parent	= NULL,
};
EXPORT_SYMBOL(ambarella_rct_scaler_ops);

struct clk_ops ambarella_rct_pll_ops = {
	.enable		= ambarella_rct_clk_enable,
	.disable	= ambarella_rct_clk_disable,
	.get_rate	= ambarella_rct_clk_get_rate,
	.round_rate	= NULL,
	.set_rate	= ambarella_rct_clk_set_rate,
	.set_parent	= NULL,
};
EXPORT_SYMBOL(ambarella_rct_pll_ops);

struct clk_ops ambarella_rct_adj_ops = {
	.enable		= ambarella_rct_clk_enable,
	.disable	= ambarella_rct_clk_disable,
	.get_rate	= ambarella_rct_clk_get_rate,
	.round_rate	= NULL,
	.set_rate	= ambarella_rct_clk_adj_rate,
	.set_parent	= NULL,
};
EXPORT_SYMBOL(ambarella_rct_adj_ops);

/* ==========================================================================*/

int ambarella_rct_clk_enable(struct clk *c)
{
	union ctrl_reg_u ctrl_reg;

	if (c->ctrl_reg == -1) {
		return -1;
	}

	ctrl_reg.w = amba_rct_readl(c->ctrl_reg);
	ctrl_reg.s.power_down = 0;
	ctrl_reg.s.halt_vco = 0;
	ctrl_reg.s.write_enable = 1;
	amba_rct_writel(c->ctrl_reg, ctrl_reg.w);

	ctrl_reg.s.write_enable	= 0;
	amba_rct_writel(c->ctrl_reg, ctrl_reg.w);

	c->rate = ambarella_rct_clk_get_rate(c);

	return 0;
}
EXPORT_SYMBOL(ambarella_rct_clk_enable);

int ambarella_rct_clk_disable(struct clk *c)
{
	union ctrl_reg_u ctrl_reg;

	if (c->ctrl_reg == -1) {
		return -1;
	}

	ctrl_reg.w = amba_rct_readl(c->ctrl_reg);
	ctrl_reg.s.power_down = 1;
	ctrl_reg.s.halt_vco = 1;
	ctrl_reg.s.write_enable = 1;
	amba_rct_writel(c->ctrl_reg, ctrl_reg.w);

	ctrl_reg.s.write_enable	= 0;
	amba_rct_writel(c->ctrl_reg, ctrl_reg.w);

	c->rate = ambarella_rct_clk_get_rate(c);

	return 0;
}
EXPORT_SYMBOL(ambarella_rct_clk_disable);


/* ==========================================================================*/
unsigned long ambarella_rct_scaler_get_rate(struct clk *c)
{
	u32 parent_rate, divider;

	if (!c->parent || !c->parent->ops || !c->parent->ops->get_rate)
		parent_rate = ambarella_clk_get_ref_freq();
	else
		parent_rate = c->parent->ops->get_rate(c->parent);

	if (c->divider) {
		c->rate = parent_rate / c->divider;
		return c->rate;
	} else if (c->post_reg != -1) {
		divider = amba_rct_readl(c->post_reg);
		if (c->extra_scaler == 1) {
			divider >>= 4;
			divider++;
		}
		if (divider) {
			c->rate = parent_rate / divider;
			return c->rate;
		}
	}

	return 0;
}
EXPORT_SYMBOL(ambarella_rct_scaler_get_rate);

int ambarella_rct_scaler_set_rate(struct clk *c, unsigned long rate)
{
	u32 parent_rate, divider, post_scaler;

	if (!rate)
		return -1;

	BUG_ON(c->post_reg == -1 || !c->max_divider);

	if (!c->parent || !c->parent->ops || !c->parent->ops->get_rate)
		parent_rate = ambarella_clk_get_ref_freq();
	else
		parent_rate = c->parent->ops->get_rate(c->parent);

	if (c->divider)
		rate *= c->divider;

	divider = (parent_rate + rate - 1) / rate;
	if (!divider)
		return -1;

	post_scaler = min(divider, c->max_divider);
	if (c->extra_scaler == 1) {
		post_scaler--;
		post_scaler <<= 4;
		amba_rct_writel_en(c->post_reg, post_scaler);
	} else {
		amba_rct_writel(c->post_reg, post_scaler);
	}

	c->rate = ambarella_rct_scaler_get_rate(c);

	return 0;
}
EXPORT_SYMBOL(ambarella_rct_scaler_set_rate);

unsigned long ambarella_rct_clk_get_rate(struct clk *c)
{
	u32 pre_scaler, post_scaler, intp, sdiv, sout;
	u64 dividend, divider, frac;
	union ctrl_reg_u ctrl_reg;
	union frac_reg_u frac_reg;

	BUG_ON(c->ctrl_reg == -1 || c->frac_reg == -1);

	ctrl_reg.w = amba_rct_readl(c->ctrl_reg);
	if ((ctrl_reg.s.power_down == 1) || (ctrl_reg.s.halt_vco == 1)) {
		c->rate = 0;
		return c->rate;
	}

	frac_reg.w = amba_rct_readl(c->frac_reg);

	if (c->pres_reg != -1) {
		pre_scaler = amba_rct_readl(c->pres_reg);
		if (c->extra_scaler == 1) {
			pre_scaler >>= 4;
			pre_scaler++;
		}
	} else {
		pre_scaler = 1;
	}

	if (c->post_reg != -1) {
		post_scaler = amba_rct_readl(c->post_reg);
		if (c->extra_scaler == 1) {
			post_scaler >>= 4;
			post_scaler++;
		}
	} else {
		post_scaler = 1;
	}

	if (ctrl_reg.s.bypass || ctrl_reg.s.force_bypass) {
		c->rate = ambarella_clk_get_ref_freq() / pre_scaler / post_scaler;
		return c->rate;
	}

	intp = ctrl_reg.s.intp + 1;
	sdiv = ctrl_reg.s.sdiv + 1;
	sout = ctrl_reg.s.sout + 1;

	dividend = (u64)ambarella_clk_get_ref_freq();
	dividend *= (u64)intp;
	dividend *= (u64)sdiv;
	if (ctrl_reg.s.frac_mode) {
		if (frac_reg.s.nega) {
			/* Negative */
			frac = (0x80000000 - frac_reg.s.frac);
			frac = (ambarella_clk_get_ref_freq() * frac * sdiv);
			frac >>= 32;
			dividend = dividend - frac;
		} else {
			/* Positive */
			frac = frac_reg.s.frac;
			frac = (ambarella_clk_get_ref_freq() * frac * sdiv);
			frac >>= 32;
			dividend = dividend + frac;
		}
	}

	divider = pre_scaler * sout * post_scaler;
	if (c->divider)
		divider *= c->divider;

	if (divider == 0) {
		c->rate = 0;
		return c->rate;
	}

	AMBCLK_DO_DIV(dividend, divider);
	c->rate = dividend;

	return c->rate;
}
EXPORT_SYMBOL(ambarella_rct_clk_get_rate);


int ambarella_rct_clk_adj_rate(struct clk *c, unsigned long rate)
{
	union ctrl_reg_u ctrl_reg;
	u32 curr_rate;
	u32 IntStepPllOut24MHz;
	u32 TargetIntp;

	/* get current PLL control registers' values */
	ctrl_reg.w = amba_rct_readl(c->ctrl_reg);

	if (ctrl_reg.s.sdiv >= ctrl_reg.s.sout) {
		while (ctrl_reg.s.sdiv > ctrl_reg.s.sout) {
			ctrl_reg.s.sdiv--;
			amba_rct_writel(c->ctrl_reg, ctrl_reg.w);
		}
		IntStepPllOut24MHz = 1;
	} else {
		IntStepPllOut24MHz = (ctrl_reg.s.sout + 1) / (ctrl_reg.s.sdiv + 1);
	}

	curr_rate = ambarella_rct_clk_get_rate(c);
	TargetIntp = rate * (ctrl_reg.s.sout + 1) / ((ctrl_reg.s.sdiv + 1) * REF_CLK_FREQ) - 1;

	if (curr_rate > rate) {
		/* decrease the frequency */
		while (curr_rate > rate && ctrl_reg.s.intp > 0) {
			if (ctrl_reg.s.intp - TargetIntp >= IntStepPllOut24MHz)
				ctrl_reg.s.intp -= IntStepPllOut24MHz;
			else
				ctrl_reg.s.intp--;

			amba_rct_writel_en(c->ctrl_reg, ctrl_reg.w);
			curr_rate = ambarella_rct_clk_get_rate(c);
		}
	} else {
		/* increase the frequency */
		if (TargetIntp > 123)
			TargetIntp = 123;

		while (curr_rate < rate && ctrl_reg.s.intp < 123) {
			if (TargetIntp - ctrl_reg.s.intp >= IntStepPllOut24MHz)
				ctrl_reg.s.intp += IntStepPllOut24MHz;
			else
				ctrl_reg.s.intp++;

			amba_rct_writel_en(c->ctrl_reg, ctrl_reg.w);
			curr_rate = ambarella_rct_clk_get_rate(c);
		}

		if (curr_rate > rate && ctrl_reg.s.intp > 6) {
			/* decrease the frequency so that is it is just below expected */
			ctrl_reg.s.intp--;
			amba_rct_writel_en(c->ctrl_reg, ctrl_reg.w);
		}
	}

	return 0;
}
EXPORT_SYMBOL(ambarella_rct_clk_adj_rate);

/* ==========================================================================*/
struct clk *clk_get_sys(const char *dev_id, const char *con_id)
{
	struct clk *p;
	struct clk *clk = ERR_PTR(-ENOENT);

	spin_lock(&ambarella_clock_lock);
	list_for_each_entry(p, &ambarella_all_clocks, list) {
		if (dev_id && (strcmp(p->name, dev_id) == 0)) {
			clk = p;
			break;
		}
		if (con_id && (strcmp(p->name, con_id) == 0)) {
			clk = p;
			break;
		}
	}
	spin_unlock(&ambarella_clock_lock);

	return clk;
}
EXPORT_SYMBOL(clk_get_sys);

struct clk *clk_get(struct device *dev, const char *id)
{
	struct clk *p;
	struct clk *clk = ERR_PTR(-ENOENT);

	if (id == NULL) {
		return clk;
	}

	spin_lock(&ambarella_clock_lock);
	list_for_each_entry(p, &ambarella_all_clocks, list) {
		if (strcmp(p->name, id) == 0) {
			clk = p;
			break;
		}
	}
	spin_unlock(&ambarella_clock_lock);

	return clk;
}
EXPORT_SYMBOL(clk_get);

void clk_put(struct clk *clk)
{
}
EXPORT_SYMBOL(clk_put);

int clk_enable(struct clk *clk)
{
	if (IS_ERR(clk) || (clk == NULL)) {
		return -EINVAL;
	}

	clk_enable(clk->parent);

	spin_lock(&ambarella_clock_lock);
	if (clk->ops && clk->ops->enable) {
		(clk->ops->enable)(clk);
	}
	spin_unlock(&ambarella_clock_lock);

	return 0;
}
EXPORT_SYMBOL(clk_enable);

void clk_disable(struct clk *clk)
{
	if (IS_ERR(clk) || (clk == NULL)) {
		return;
	}

	spin_lock(&ambarella_clock_lock);
	if (clk->ops && clk->ops->disable) {
		(clk->ops->disable)(clk);
	}
	spin_unlock(&ambarella_clock_lock);

	clk_disable(clk->parent);
}
EXPORT_SYMBOL(clk_disable);

unsigned long clk_get_rate(struct clk *clk)
{
	if (IS_ERR(clk) || (clk == NULL)) {
		return 0;
	}

	if (clk->ops != NULL && clk->ops->get_rate != NULL) {
		return (clk->ops->get_rate)(clk);
	}

	if (clk->parent != NULL) {
		return clk_get_rate(clk->parent);
	}

	return clk->rate;
}
EXPORT_SYMBOL(clk_get_rate);

long clk_round_rate(struct clk *clk, unsigned long rate)
{
	if (IS_ERR(clk) || (clk == NULL)) {
		return rate;
	}

	if (clk->ops && clk->ops->round_rate) {
		return (clk->ops->round_rate)(clk, rate);
	}

	return rate;
}
EXPORT_SYMBOL(clk_round_rate);

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	int ret;

	if (IS_ERR(clk) || (clk == NULL)) {
		return -EINVAL;
	}

	if ((clk->ops == NULL) || (clk->ops->set_rate == NULL)) {
		return -EINVAL;
	}

	spin_lock(&ambarella_clock_lock);
	ret = (clk->ops->set_rate)(clk, rate);
	spin_unlock(&ambarella_clock_lock);

	return ret;
}
EXPORT_SYMBOL(clk_set_rate);

struct clk *clk_get_parent(struct clk *clk)
{
	if (IS_ERR(clk) || (clk == NULL)) {
		return ERR_PTR(-EINVAL);
	}

	return clk->parent;
}
EXPORT_SYMBOL(clk_get_parent);

int clk_set_parent(struct clk *clk, struct clk *parent)
{
	int ret = 0;

	if (IS_ERR(clk) || (clk == NULL)) {
		return -EINVAL;
	}

	spin_lock(&ambarella_clock_lock);
	if (clk->ops && clk->ops->set_parent) {
		ret = (clk->ops->set_parent)(clk, parent);
	}
	spin_unlock(&ambarella_clock_lock);

	return ret;
}
EXPORT_SYMBOL(clk_set_parent);

int ambarella_clk_add(struct clk *clk)
{
	struct clk *p;

	if (IS_ERR(clk) || (clk == NULL))
		return -EINVAL;

	spin_lock(&ambarella_clock_lock);
	list_for_each_entry(p, &ambarella_all_clocks, list) {
		if (clk == p) {
			pr_err("clk %s is existed\n", clk->name);
			spin_unlock(&ambarella_clock_lock);
			return -EEXIST;
		}
	}
	list_add(&clk->list, &ambarella_all_clocks);
	spin_unlock(&ambarella_clock_lock);

	return 0;
}
EXPORT_SYMBOL(ambarella_clk_add);

/* ==========================================================================*/
#if defined(CONFIG_AMBARELLA_PLL_PROC)
static int ambarella_clock_proc_show(struct seq_file *m, void *v)
{
	int retlen = 0;
	struct clk *p;

	retlen += seq_printf(m, "\nClock Information:\n");
	spin_lock(&ambarella_clock_lock);
	list_for_each_entry_reverse(p, &ambarella_all_clocks, list) {
		retlen += seq_printf(m, "\t%s:\t%lu Hz\n",
			p->name, p->ops->get_rate(p));
	}
	spin_unlock(&ambarella_clock_lock);

	return retlen;
}

static int ambarella_clock_proc_write(struct file *file,
	const char __user *buffer, size_t count, loff_t *ppos)
{
	struct clk *gclk;
	char *buf, clk_name[32];
	int freq, rval = count;

	pr_warn("!!!DANGEROUS!!! You must know what you are doning!\n");

	buf = kmalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (copy_from_user(buf, buffer, count)) {
		rval = -EFAULT;
		goto exit;
	}

	sscanf(buf, "%s %d", clk_name, &freq);

	gclk = clk_get(NULL, clk_name);
	if (IS_ERR(gclk)) {
		pr_err("Invalid clk name\n");
		rval = -EINVAL;
		goto exit;
	}

	clk_set_rate(gclk, freq);

exit:
	kfree(buf);
	return rval;
}

static int ambarella_clock_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ambarella_clock_proc_show, PDE_DATA(inode));
}

static const struct file_operations proc_clock_fops = {
	.open = ambarella_clock_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = ambarella_clock_proc_write,
};
#endif

/* ==========================================================================*/
int __init ambarella_clk_init(void)
{
	int ret_val = 0;

#if defined(CONFIG_AMBARELLA_PLL_PROC)
	proc_create_data("clock", S_IRUGO, get_ambarella_proc_dir(),
		&proc_clock_fops, NULL);
#endif

	return ret_val;
}

/* ==========================================================================*/

static struct clk pll_out_core = {
	.parent		= NULL,
	.name		= "pll_out_core",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= PLL_CORE_CTRL_REG,
	.pres_reg	= -1,
	.post_reg	= -1,
	.frac_reg	= PLL_CORE_FRAC_REG,
	.ctrl2_reg	= PLL_CORE_CTRL2_REG,
	.ctrl3_reg	= PLL_CORE_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 6,
	.divider	= 0,
	.max_divider	= 0,
	.extra_scaler	= 0,
	.table		= ambarella_pll_int_table,
	.table_size	= ARRAY_SIZE(ambarella_pll_int_table),
	.ops		= &ambarella_rct_adj_ops,
};

static struct clk gclk_core = {
	.parent		= &pll_out_core,
	.name		= "gclk_core",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
#if ((CHIP_REV == A5S) || (CHIP_REV == S2) || (CHIP_REV == S2E))
	.post_reg	= SCALER_CORE_POST_REG,
#else
	.post_reg	= -1,
#endif
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
#if ((CHIP_REV == A5S) || (CHIP_REV == S2) || (CHIP_REV == S2E))
	.divider	= 0,
	.max_divider	= (1 << 4) - 1,
#else
	.divider	= 2,
	.max_divider	= 0,
#endif
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};

static struct clk gclk_ahb = {
	.parent		= &gclk_core,
	.name		= "gclk_ahb",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= -1,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
#if (CHIP_REV == A5S)
	.divider	= 1,
#else
	.divider	= 2,
#endif
	.max_divider	= 0,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};

static struct clk gclk_apb = {
	.parent		= &gclk_ahb,
	.name		= "gclk_apb",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= -1,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 2,
	.max_divider	= 0,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};

static struct clk gclk_ddr = {
	.parent		= NULL,
	.name		= "gclk_ddr",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= PLL_DDR_CTRL_REG,
	.pres_reg	= -1,
	.post_reg	= -1,
	.frac_reg	= PLL_DDR_FRAC_REG,
	.ctrl2_reg	= PLL_DDR_CTRL2_REG,
	.ctrl3_reg	= PLL_DDR_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 5,
	.divider	= 2,
	.max_divider	= 0,
	.extra_scaler	= 0,
	.table		= ambarella_pll_int_table,
	.table_size	= ARRAY_SIZE(ambarella_pll_int_table),
	.ops		= &ambarella_rct_adj_ops,
};

/* ==========================================================================*/
#if defined(CONFIG_PLAT_AMBARELLA_CORTEX)
static struct clk gclk_cortex = {
	.parent		= NULL,
	.name		= "gclk_cortex",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= PLL_CORTEX_CTRL_REG,
	.pres_reg	= -1,
	.post_reg	= -1,
	.frac_reg	= PLL_CORTEX_FRAC_REG,
	.ctrl2_reg	= PLL_CORTEX_CTRL2_REG,
	.ctrl3_reg	= PLL_CORTEX_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 2,
	.divider	= 0,
	.max_divider	= 0,
	.extra_scaler	= 0,
	.table		= ambarella_pll_int_table,
	.table_size	= ARRAY_SIZE(ambarella_pll_int_table),
	.ops		= &ambarella_rct_adj_ops,
};
static struct clk gclk_axi = {
	.parent		= &gclk_cortex,
	.name		= "gclk_axi",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= -1,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 3,
	.max_divider	= 0,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};
#if defined(CONFIG_HAVE_ARM_TWD)
static struct clk clk_smp_twd = {
	.parent		= &gclk_axi,
	.name		= "smp_twd",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= -1,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 1,
	.max_divider	= 0,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};
#endif
#endif

static struct clk gclk_idsp = {
	.parent		= NULL,
	.name		= "gclk_idsp",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= PLL_IDSP_CTRL_REG,
	.pres_reg	= -1,
	.post_reg	= SCALER_IDSP_POST_REG,
	.frac_reg	= PLL_IDSP_FRAC_REG,
	.ctrl2_reg	= PLL_IDSP_CTRL2_REG,
	.ctrl3_reg	= PLL_IDSP_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 4,
	.divider	= 0,
	.max_divider	= (1 << 4) - 1,
#if (CHIP_REV == S2L) || (CHIP_REV == S3) || (CHIP_REV == S3L)
	.extra_scaler	= 1,
#else
	.extra_scaler	= 0,
#endif
	.table		= ambarella_pll_int_table,
	.table_size	= ARRAY_SIZE(ambarella_pll_int_table),
	.ops		= &ambarella_rct_pll_ops,
};

#ifdef CONFIG_AMBARELLA_CALC_PLL
static struct clk gclk_so = {
	.parent		= NULL,
	.name		= "gclk_so",
	.rate		= 0,
	.frac_mode	= 1,
	.ctrl_reg	= PLL_SENSOR_CTRL_REG,
	.pres_reg	= SCALER_SENSOR_PRE_REG,
	.post_reg	= SCALER_SENSOR_POST_REG,
	.frac_reg	= PLL_SENSOR_FRAC_REG,
	.ctrl2_reg	= PLL_SENSOR_CTRL2_REG,
	.ctrl3_reg	= PLL_SENSOR_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 3,
	.divider	= 0,
	.max_divider	= (1 << 4) - 1,
#if (CHIP_REV == S2L) || (CHIP_REV == S3) || (CHIP_REV == S3L)
	.max_divider	= (1 << 4) - 1,
	.extra_scaler	= 1,
#else
	.max_divider	= (1 << 16) - 1,
	.extra_scaler	= 0,
#endif
	.ops		= &ambarella_rct_pll_ops,
};

static struct clk gclk_vo = {
	.parent		= NULL,
	.name		= "gclk_vo",
	.rate		= 0,
	.frac_mode	= 1,
	.ctrl_reg	= PLL_HDMI_CTRL_REG,
	.pres_reg	= SCALER_HDMI_PRE_REG,
	.post_reg	= -1,
	.frac_reg	= PLL_HDMI_FRAC_REG,
	.ctrl2_reg	= PLL_HDMI_CTRL2_REG,
#if (CHIP_REV == S2E) || (CHIP_REV == S3L)
	.ctrl2_val	= 0x3f770b00,
#endif
	.ctrl3_reg	= PLL_HDMI_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 8,
	.divider	= 10,
#if (CHIP_REV == S2L) || (CHIP_REV == S3) || (CHIP_REV == S3L)
	.max_divider	= (1 << 4) - 1,
	.extra_scaler	= 1,
#else
	.max_divider	= (1 << 16) - 1,
	.extra_scaler	= 0,
#endif
	.ops		= &ambarella_rct_pll_ops,
};

static struct clk gclk_vo2 = {
	.parent		= NULL,
	.name		= "gclk_vo2",
	.rate		= 0,
	.frac_mode	= 1,
	.ctrl_reg	= PLL_VIDEO2_CTRL_REG,
	.pres_reg	= SCALER_VIDEO2_PRE_REG,
	.post_reg	= SCALER_VIDEO2_POST_REG,
	.frac_reg	= PLL_VIDEO2_FRAC_REG,
	.ctrl2_reg	= PLL_VIDEO2_CTRL2_REG,
	.ctrl3_reg	= PLL_VIDEO2_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 0,
	.divider	= 0,
#if (CHIP_REV == S2L) || (CHIP_REV == S3) || (CHIP_REV == S3L)
	.max_divider	= (1 << 4) - 1,
	.extra_scaler	= 1,
#else
	.max_divider	= (1 << 16) - 1,
	.extra_scaler	= 0,
#endif
	.ops		= &ambarella_rct_pll_ops,
};
#endif

static struct clk gclk_uart = {
#if (CHIP_REV == S2E)
	.parent		= &gclk_idsp,
#else
	.parent		= NULL,
#endif
	.name		= "gclk_uart",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= CG_UART_REG,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 0,
	.max_divider	= (1 << 24) - 1,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};

static struct clk gclk_audio = {
	.parent		= NULL,
	.name		= "gclk_audio",
	.rate		= 0,
	.frac_mode	= 1,
	.ctrl_reg	= PLL_AUDIO_CTRL_REG,
	.pres_reg	= SCALER_AUDIO_PRE_REG,
	.post_reg	= SCALER_AUDIO_POST_REG,
	.frac_reg	= PLL_AUDIO_FRAC_REG,
	.ctrl2_reg	= PLL_AUDIO_CTRL2_REG,
	.ctrl3_reg	= PLL_AUDIO_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 7,
	.divider	= 0,
#if (CHIP_REV == S2L) || (CHIP_REV == S3) || (CHIP_REV == S3L)
	.max_divider	= (1 << 4) - 1,
	.extra_scaler	= 1,
#else
	.max_divider	= (1 << 16) - 1,
	.extra_scaler	= 0,
#endif
	.table		= ambarella_pll_frac_table,
	.table_size	= ARRAY_SIZE(ambarella_pll_frac_table),
	.ops		= &ambarella_rct_pll_ops,
};

#if (CHIP_REV == S2E) || (CHIP_REV == S2L) || (CHIP_REV == S3) || (CHIP_REV == S3L)
static struct clk pll_out_sd = {
	.parent		= NULL,
	.name		= "pll_out_sd",
	.rate		= 0,
	.frac_mode	= 1,
	.ctrl_reg	= PLL_SD_CTRL_REG,
	.pres_reg	= -1,
	.post_reg	= -1,
	.frac_reg	= PLL_SD_FRAC_REG,
	.ctrl2_reg	= PLL_SD_CTRL2_REG,
	.ctrl3_reg	= PLL_SD_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 12,
	.divider	= 0,
	.max_divider	= 0,
	.extra_scaler	= 0,
	.table		= ambarella_pll_int_table,
	.table_size	= ARRAY_SIZE(ambarella_pll_int_table),
	.ops		= &ambarella_rct_pll_ops,
};
#endif

#if (SD_SUPPORT_SDXC == 1)
static struct clk gclk_sdxc = {
	.parent		= &pll_out_sd,
	.name		= "gclk_sdxc",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= SCALER_SDXC_REG,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 0,
	.max_divider	= (1 << 16) - 1,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};
#endif

#if (SD_SUPPORT_SDIO == 1)
static struct clk gclk_sdio = {
#if (CHIP_REV == S2E) || (CHIP_REV == S2L) || (CHIP_REV == S3)
	.parent		= &pll_out_sd,
#else
	.parent		= &pll_out_core,
#endif
	.name		= "gclk_sdio",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= SCALER_SDIO_REG,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 0,
	.max_divider	= (1 << 16) - 1,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};
#endif

static struct clk gclk_sd = {
#if (CHIP_REV == S2E) || (CHIP_REV == S2L) || (CHIP_REV == S3) || (CHIP_REV == S3L)
	.parent		= &pll_out_sd,
#else
	.parent		= &pll_out_core,
#endif
	.name		= "gclk_sd",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= SCALER_SD48_REG,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 0,
	.max_divider	= (1 << 16) - 1,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};

static struct clk gclk_ir = {
	.parent		= NULL,
	.name		= "gclk_ir",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= CG_IR_REG,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 0,
	.max_divider	= (1 << 24) - 1,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};

static struct clk gclk_adc = {
	.parent		= NULL,
	.name		= "gclk_adc",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= SCALER_ADC_REG,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 2,
	.max_divider	= (1 << 16) - 1,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};

static struct clk gclk_ssi = {	/* for SSI master */
#if (CHIP_REV == A5S) || (CHIP_REV == S2) || (CHIP_REV == S2E)
	.parent		= &gclk_apb,
#else
	.parent		= &pll_out_core,
#endif
	.name		= "gclk_ssi",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= CG_SSI_REG,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 0,
	.max_divider	= (1 << 24) - 1,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};

static struct clk gclk_ssi2 = {	/* for SSI slave */
#if (CHIP_REV == A5S) || (CHIP_REV == S2) || (CHIP_REV == S2E)
	.parent		= &gclk_apb,
#else
	.parent		= &pll_out_core,
#endif
	.name		= "gclk_ssi2",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= CG_SSI2_REG,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 0,
	.max_divider	= (1 << 24) - 1,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};

#if (CHIP_REV == S2L) || (CHIP_REV == S3) || (CHIP_REV == S3L)
static struct clk gclk_ssi3 = {	/* for SPINOR */
	/* TODO: parent is determined by CLK_REF_SSI3_REG */
	.parent		= &pll_out_core,
	.name		= "gclk_ssi3",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= CG_SSI3_REG,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 0,
	.max_divider	= (1 << 24) - 1,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};
#endif

static struct clk gclk_pwm = {
	.parent		= &gclk_apb,
	.name		= "gclk_pwm",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= CG_PWM_REG,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 0,
	.max_divider	= (1 << 24) - 1,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};

static int ambarella_pll_service(void *arg, void *result)
{
	struct ambsvc_pll *pll_svc = arg;
	struct clk *clk;
	int rval = 0;

	BUG_ON(!pll_svc || !pll_svc->name);

	clk = clk_get(NULL, pll_svc->name);
	if (IS_ERR(clk)) {
		pr_err("%s: ERR get %s\n", __func__, pll_svc->name);
		return -EINVAL;
	}

	switch (pll_svc->svc_id) {
	case AMBSVC_PLL_GET_RATE:
		pll_svc->rate = clk_get_rate(clk);
		break;
	case AMBSVC_PLL_SET_RATE:
		clk_set_rate(clk, pll_svc->rate);
		break;

	default:
		pr_err("%s: Invalid pll service (%d)\n", __func__, pll_svc->svc_id);
		rval = -EINVAL;
		break;
	}

	return rval;
}

void ambarella_init_early(void)
{
	ambarella_clk_add(&pll_out_core);
#if (CHIP_REV == S2E) || (CHIP_REV == S2L) || (CHIP_REV == S3) || (CHIP_REV == S3L)
	ambarella_clk_add(&pll_out_sd);
#endif
#if defined(CONFIG_PLAT_AMBARELLA_CORTEX)
	ambarella_clk_add(&gclk_cortex);
	ambarella_clk_add(&gclk_axi);
#if defined(CONFIG_HAVE_ARM_TWD)
	ambarella_clk_add(&clk_smp_twd);
#endif
#endif
	ambarella_clk_add(&gclk_ddr);
	ambarella_clk_add(&gclk_core);
	ambarella_clk_add(&gclk_ahb);
	ambarella_clk_add(&gclk_apb);
	ambarella_clk_add(&gclk_idsp);
#ifdef CONFIG_AMBARELLA_CALC_PLL
	amba_rct_writel(CLK_SI_INPUT_MODE_REG, 0x0);
#if (CHIP_REV == S2E)
	amba_rct_setbitsl(HDMI_CLOCK_CTRL_REG, 0x1);
#endif
	ambarella_clk_add(&gclk_so);
	ambarella_clk_add(&gclk_vo2);	/* for lcd */
	ambarella_clk_add(&gclk_vo);	/* for tv */
#endif
#if (CHIP_REV == S2E)
	amba_rct_writel(UART_CLK_SRC_SEL_REG, UART_CLK_SRC_IDSP);
#endif
	ambarella_clk_add(&gclk_uart);
	ambarella_clk_add(&gclk_audio);
#if (SD_SUPPORT_SDXC == 1)
	ambarella_clk_add(&gclk_sdxc);
#endif
#if (SD_SUPPORT_SDIO == 1)
	ambarella_clk_add(&gclk_sdio);
#endif
	ambarella_clk_add(&gclk_sd);
	ambarella_clk_add(&gclk_ir);
	ambarella_clk_add(&gclk_adc);
	ambarella_clk_add(&gclk_ssi);
	ambarella_clk_add(&gclk_ssi2);
#if (CHIP_REV == S2L) || (CHIP_REV == S3) || (CHIP_REV == S3L)
	ambarella_clk_add(&gclk_ssi3);
#endif
	ambarella_clk_add(&gclk_pwm);

	/* register ambarella clk service for private operation */
	pll_service.service = AMBARELLA_SERVICE_PLL;
	pll_service.func = ambarella_pll_service;
	ambarella_register_service(&pll_service);
}

