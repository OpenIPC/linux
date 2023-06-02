/**
    NVT clock management module
    To handle Linux clock framework API interface
    @file nvt-im-clk.c
    @ingroup
    @note
    Copyright Novatek Microelectronics Corp. 2019. All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/delay.h>

#include "nvt-im-clk.h"
#define div_mask(width)	((1 << (width)) - 1)

void __iomem *cg_base = NULL;

#define CG_SETREG(ofs, value)	writel((value), (cg_base+(ofs)))
#define CG_GETREG(ofs)		readl(cg_base+(ofs))

void nvt_cg_base_remap(void __iomem *remap_base)
{
	cg_base = remap_base;
}

int nvt_fixed_rate_clk_register(struct nvt_fixed_rate_clk fixed_rate_clks[],
				int array_size)
{
	struct clk *clk;
	int i, ret = 0;

	for (i = 0; i < array_size; i++) {
		clk = clk_register_fixed_rate(NULL, fixed_rate_clks[i].name, NULL, 0,
		                              fixed_rate_clks[i].fixed_rate);
		if (IS_ERR(clk)) {
			pr_err("%s: failed to register fixed rate clock \"%s\"\n", __func__, fixed_rate_clks[i].name);
			ret = -EPERM;
			goto err;
		}

		if (clk_register_clkdev(clk, fixed_rate_clks[i].name, NULL)) {
			pr_err("%s: failed to register lookup %s!\n", __func__, fixed_rate_clks[i].name);
			ret = -EPERM;
			goto err;
		} else {
			ret = clk_prepare_enable(clk);
			if (ret < 0)
				pr_err("Fixed clock prepare & enable failed!\n");
		}
	}

err:
	return ret;
}

static int nvt_pll_clk_enable(struct clk_hw *hw)
{
	unsigned int reg_val;
	unsigned long flags;
	int timeout = 100;
	struct nvt_pll_clk *pll = container_of(hw, struct nvt_pll_clk, pll_hw);

	/* race condition protect. enter critical section */
	spin_lock_irqsave(pll->lock, flags);
	reg_val = CG_GETREG(pll->gate_reg_offset);
	reg_val |= (0x1 << pll->gate_bit_idx);
	CG_SETREG(pll->gate_reg_offset, reg_val);

	/* Wait PLLx_READY becomes 1 */
	while (timeout-- != 0) {
		if (CG_GETREG(pll->status_reg_offset) &
		    (0x1 << pll->gate_bit_idx))
			break;
	}

	if (timeout == 0)
		pr_err("%s is not ready!\n", pll->name);

	/* race condition protect. leave critical section */
	spin_unlock_irqrestore(pll->lock, flags);

	return 0;
}

static void nvt_pll_clk_disable(struct clk_hw *hw)
{
	unsigned int reg_val;
	unsigned long flags;
	struct nvt_pll_clk *pll = container_of(hw, struct nvt_pll_clk, pll_hw);

	/* race condition protect. enter critical section */
	spin_lock_irqsave(pll->lock, flags);
	reg_val = CG_GETREG(pll->gate_reg_offset);
	reg_val &= ~(0x1 << pll->gate_bit_idx);
	CG_SETREG(pll->gate_reg_offset, reg_val);
	/* race condition protect. leave critical section */
	spin_unlock_irqrestore(pll->lock, flags);
}

static int nvt_pll_clk_is_enabled(struct clk_hw *hw)
{
	unsigned long flags;
	bool is_enabled;
	struct nvt_pll_clk *pll = container_of(hw, struct nvt_pll_clk, pll_hw);

	/* race condition protect. enter critical section */
	spin_lock_irqsave(pll->lock, flags);
	is_enabled = CG_GETREG(pll->gate_reg_offset) & (0x1 << pll->gate_bit_idx);
	/* race condition protect. leave critical section */
	spin_unlock_irqrestore(pll->lock, flags);

	return is_enabled;
}

static long nvt_pll_clk_round_rate(struct clk_hw *hw, unsigned long rate,
				   unsigned long *parent_rate)
{
	struct nvt_pll_clk *pll = container_of(hw, struct nvt_pll_clk, pll_hw);
	unsigned long long temp_rate, ratio;

	temp_rate = rate;

	ratio = (temp_rate * pll_cal_factor);

	do_div(ratio, pll->parent_rate);

	if (!ratio)
		pll->pll_ratio = 1;
	else
		pll->pll_ratio = ratio;


	temp_rate = pll->pll_ratio * pll->parent_rate;

	do_div(temp_rate, pll_cal_factor);

	pll->current_rate = temp_rate;

	return pll->current_rate;
}

static unsigned long nvt_pll_clk_recalc_rate(struct clk_hw *hw,
					     unsigned long parent_rate)
{
	struct nvt_pll_clk *pll = container_of(hw, struct nvt_pll_clk, pll_hw);
	unsigned long long temp_rate;
	unsigned int ratio0, ratio1, ratio2;

	ratio0 = CG_GETREG(pll->rate_reg_offset + 0x20) & 0xFF;
	ratio1 = CG_GETREG(pll->rate_reg_offset + 0x24) & 0xFF;
	ratio2 = CG_GETREG(pll->rate_reg_offset + 0x28) & 0xFF;

	pll->pll_ratio = ratio0 | (ratio1 << 8) | (ratio2 << 16);

	temp_rate = pll->pll_ratio * pll->parent_rate;

	do_div(temp_rate, pll_cal_factor);

	pll->current_rate = temp_rate;

	return pll->current_rate;
}

static int nvt_pll_clk_set_rate(struct clk_hw *hw, unsigned long rate,
				unsigned long parent_rate)
{
	struct nvt_pll_clk *pll = container_of(hw, struct nvt_pll_clk, pll_hw);
	unsigned long flags;

	spin_lock_irqsave(pll->lock, flags);
	CG_SETREG(pll->rate_reg_offset + 0x20, pll->pll_ratio & 0xFF);
	CG_SETREG(pll->rate_reg_offset + 0x24, (pll->pll_ratio >> 8) & 0xFF);
	CG_SETREG(pll->rate_reg_offset + 0x28, (pll->pll_ratio >> 16) & 0xFF);
	spin_unlock_irqrestore(pll->lock, flags);

	return 0;
}

static const struct clk_ops nvt_pll_clk_ops = {
	.enable = nvt_pll_clk_enable,
	.disable = nvt_pll_clk_disable,
	.is_enabled = nvt_pll_clk_is_enabled,
	.round_rate = nvt_pll_clk_round_rate,
	.recalc_rate = nvt_pll_clk_recalc_rate,
	.set_rate = nvt_pll_clk_set_rate,
};

int nvt_pll_clk_register(struct nvt_pll_clk pll_clks[], int array_size,
			 unsigned long pll_div_value, spinlock_t *cg_spinlock)
{
	struct clk *clk;
	struct clk_init_data init;
	struct nvt_pll_clk *p_pll_clk;
	const char *parent_name;
	char *pstr;
	int i, ret = 0;

	pstr = kzalloc(CLK_NAME_STR_SIZE, GFP_KERNEL);
	if (!pstr) {
		ret = -EPERM;
		pr_err("%s: failed to alloc string buf!\n", __func__);
		goto err;
	}

	for (i = 0; i < array_size; i++) {
		snprintf(pstr, CLK_NAME_STR_SIZE, "%s_factor",
			 pll_clks[i].name);

		p_pll_clk = NULL;
		p_pll_clk = kzalloc(sizeof(struct nvt_pll_clk), GFP_KERNEL);
		if (!p_pll_clk) {
			pr_err("%s: failed to alloc pll_clk!\n", __func__);
			kfree(pstr);
			return -EPERM;
		}

		clk = clk_register_fixed_factor(NULL, pstr, "osc_in", 0,
		                                pll_clks[i].pll_ratio,
		                                pll_div_value);
		p_pll_clk->fixed_factor = __clk_get_hw(clk);
		p_pll_clk->parent_rate = clk_hw_get_rate(clk_hw_get_parent(__clk_get_hw(clk)));
		p_pll_clk->current_rate = clk_hw_get_rate(__clk_get_hw(clk));

		memcpy(p_pll_clk->name, pll_clks[i].name, CLK_NAME_STR_SIZE);
		p_pll_clk->pll_ratio = pll_clks[i].pll_ratio;
		p_pll_clk->rate_reg_offset = pll_clks[i].rate_reg_offset;
		p_pll_clk->gate_reg_offset = pll_clks[i].gate_reg_offset;
		p_pll_clk->gate_bit_idx = pll_clks[i].gate_bit_idx;
		p_pll_clk->lock = cg_spinlock;

		init.name = pll_clks[i].name;
		init.ops = &nvt_pll_clk_ops;
		init.flags = CLK_IGNORE_UNUSED;
		init.num_parents = 1;
		parent_name = pstr;
		init.parent_names = &parent_name;
		p_pll_clk->pll_hw.init = &init;

		clk = clk_register(NULL, &p_pll_clk->pll_hw);
		if (IS_ERR(clk)) {
			pr_err("%s: failed to register clk %s!\n", __func__, p_pll_clk->name);
			ret = -EPERM;
			goto err;
		}
		if (clk_register_clkdev(clk, p_pll_clk->name, NULL)) {
			pr_err("%s: failed to register lookup %s!\n", __func__, p_pll_clk->name);
			ret = -EPERM;
			goto err;
		} else {
			if (__clk_is_enabled(clk)) {
				ret = clk_prepare_enable(clk);
				if (ret < 0)
					pr_err("%s prepare & enable failed!\n", p_pll_clk->name);
			}
		}
	}

	kfree(pstr);

err:
	return ret;
}

static int nvt_composite_mux_clk_prepare(struct clk_hw *hw)
{
	unsigned int reg_val;
	unsigned long flags;
	struct clk_mux *cmux = container_of(hw, struct clk_mux, hw);
	struct nvt_composite_mux_clk *mux =
	    container_of(cmux, struct nvt_composite_mux_clk, mux);

	if (mux->do_reset) {
		if (mux->reset_reg_offset != 0) {
			/* race condition protect. enter critical section */
			spin_lock_irqsave(mux->lock, flags);
			reg_val = CG_GETREG(mux->reset_reg_offset);
			reg_val &= ~(0x1 << mux->reset_bit_idx);
			CG_SETREG(mux->reset_reg_offset, reg_val);
			/* race condition protect. leave critical section */
			spin_unlock_irqrestore(mux->lock, flags);
			udelay(10);
		}
		if (mux->apb_reset_reg_offset != 0) {
			/* race condition protect. enter critical section */
			spin_lock_irqsave(mux->lock, flags);
			reg_val = CG_GETREG(mux->apb_reset_reg_offset);
			reg_val &= ~(0x1 << mux->apb_reset_bit_idx);
			CG_SETREG(mux->apb_reset_reg_offset, reg_val);
			/* race condition protect. leave critical section */
			spin_unlock_irqrestore(mux->lock, flags);
			udelay(10);
		}
	}

	if (mux->reset_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(mux->lock, flags);
		reg_val = CG_GETREG(mux->reset_reg_offset);
		if ((reg_val & (0x1 << mux->reset_bit_idx)) == 0) {
			reg_val |= (0x1 << mux->reset_bit_idx);
			CG_SETREG(mux->reset_reg_offset, reg_val);
			udelay(10);
		}
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(mux->lock, flags);
	}
	if (mux->apb_reset_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(mux->lock, flags);
		reg_val = CG_GETREG(mux->apb_reset_reg_offset);
		if ((reg_val & (0x1 << mux->apb_reset_bit_idx)) == 0) {
			reg_val |= (0x1 << mux->apb_reset_bit_idx);
			CG_SETREG(mux->apb_reset_reg_offset, reg_val);
			udelay(10);
		}
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(mux->lock, flags);
	}

	return 0;
}

static void nvt_composite_mux_clk_unprepare(struct clk_hw *hw)
{
	/* Do nothing */
}

static int nvt_composite_mux_clk_is_prepared(struct clk_hw *hw)
{
	unsigned int reg_val, apb_reg_val;
	unsigned long flags;
	struct clk_mux *cmux = container_of(hw, struct clk_mux, hw);
	struct nvt_composite_mux_clk *mux =
	    container_of(cmux, struct nvt_composite_mux_clk, mux);

	if ((mux->reset_reg_offset != 0) && (mux->apb_reset_reg_offset != 0)) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(mux->lock, flags);
		/* Check if RSTN bit is released */
		reg_val = CG_GETREG(mux->reset_reg_offset);
		apb_reg_val = CG_GETREG(mux->apb_reset_reg_offset);
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(mux->lock, flags);
		return ((reg_val & (0x1 << mux->reset_bit_idx)) &&
		        (apb_reg_val & (0x1 << mux->apb_reset_bit_idx)));
	} else if (mux->reset_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(mux->lock, flags);
		/* Check if RSTN bit is released */
		reg_val = CG_GETREG(mux->reset_reg_offset);
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(mux->lock, flags);
		return (reg_val & (0x1 << mux->reset_bit_idx));
	} else if (mux->apb_reset_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(mux->lock, flags);
		/* Check if RSTN bit is released */
		apb_reg_val = CG_GETREG(mux->apb_reset_reg_offset);
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(mux->lock, flags);
		return (apb_reg_val & (0x1 << mux->apb_reset_bit_idx));
	} else {
		return 0;
	}
}

static int nvt_composite_mux_clk_enable(struct clk_hw *hw)
{
	unsigned int reg_val;
	unsigned long flags;
	struct clk_mux *cmux = container_of(hw, struct clk_mux, hw);
	struct nvt_composite_mux_clk *mux =
	    container_of(cmux, struct nvt_composite_mux_clk, mux);

	if (mux->gate_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(mux->lock, flags);
		reg_val = CG_GETREG(mux->gate_reg_offset);
		if (strncmp("eth0_extphy_clk", mux->name, sizeof(mux->name)) == 0) {
			reg_val |= (0x1 << mux->gate_bit_idx);
		} else if (strncmp("eth1_extphy_clk", mux->name, sizeof(mux->name)) == 0) {
			reg_val |= (0x1 << mux->gate_bit_idx);
		} else {
			reg_val &= ~(0x1 << mux->gate_bit_idx);
		}
		CG_SETREG(mux->gate_reg_offset, reg_val);
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(mux->lock, flags);
	}
	if (mux->apb_gate_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(mux->lock, flags);
		reg_val = CG_GETREG(mux->apb_gate_reg_offset);
		if (strncmp("eth0_extphy_clk", mux->name, sizeof(mux->name)) == 0) {
			reg_val |= (0x1 << mux->apb_gate_bit_idx);
		} else if (strncmp("eth1_extphy_clk", mux->name, sizeof(mux->name)) == 0) {
			reg_val |= (0x1 << mux->apb_gate_bit_idx);
		} else {
			reg_val &= ~(0x1 << mux->apb_gate_bit_idx);
		}
		CG_SETREG(mux->apb_gate_reg_offset, reg_val);
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(mux->lock, flags);
	}

	return 0;
}

static void nvt_composite_mux_clk_disable(struct clk_hw *hw)
{
	unsigned int reg_val;
	unsigned long flags;
	struct clk_mux *cmux = container_of(hw, struct clk_mux, hw);
	struct nvt_composite_mux_clk *mux =
	    container_of(cmux, struct nvt_composite_mux_clk, mux);

	if (mux->gate_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(mux->lock, flags);
		reg_val = CG_GETREG(mux->gate_reg_offset);
		if (strncmp("eth0_extphy_clk", mux->name, sizeof(mux->name)) == 0) {
			reg_val &= ~(0x1 << mux->gate_bit_idx);
		} else if (strncmp("eth1_extphy_clk", mux->name, sizeof(mux->name)) == 0) {
			reg_val &= ~(0x1 << mux->gate_bit_idx);
		} else {
			reg_val |= (0x1 << mux->gate_bit_idx);
		}
		CG_SETREG(mux->gate_reg_offset, reg_val);

		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(mux->lock, flags);
	}
	if (mux->apb_gate_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(mux->lock, flags);
		reg_val = CG_GETREG(mux->apb_gate_reg_offset);
		if (strncmp("eth0_extphy_clk", mux->name, sizeof(mux->name)) == 0) {
			reg_val &= ~(0x1 << mux->apb_gate_bit_idx);
		} else if (strncmp("eth1_extphy_clk", mux->name, sizeof(mux->name)) == 0) {
			reg_val &= ~(0x1 << mux->apb_gate_bit_idx);
		} else {
			reg_val |= (0x1 << mux->apb_gate_bit_idx);
		}
		CG_SETREG(mux->apb_gate_reg_offset, reg_val);
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(mux->lock, flags);
	}
}

static int nvt_composite_mux_clk_is_enabled(struct clk_hw *hw)
{
	unsigned int reg_val, apb_reg_val;
	unsigned long flags;
	struct clk_mux *cmux = container_of(hw, struct clk_mux, hw);
	struct nvt_composite_mux_clk *mux =
	    container_of(cmux, struct nvt_composite_mux_clk, mux);

	if ((mux->gate_reg_offset != 0) && (mux->apb_gate_reg_offset != 0)) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(mux->lock, flags);
		/* Check if RSTN bit is released */
		reg_val = CG_GETREG(mux->gate_reg_offset);
		apb_reg_val = CG_GETREG(mux->apb_gate_reg_offset);
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(mux->lock, flags);
		if (strncmp("eth0_extphy_clk", mux->name, sizeof(mux->name)) == 0) {
			return ((reg_val & (0x1 << mux->gate_bit_idx)) &&
				(apb_reg_val & (0x1 << mux->apb_gate_bit_idx)));
		} else if (strncmp("eth1_extphy_clk", mux->name, sizeof(mux->name)) == 0) {
			return ((reg_val & (0x1 << mux->gate_bit_idx)) &&
				(apb_reg_val & (0x1 << mux->apb_gate_bit_idx)));
		} else {
			return (~(reg_val & (0x1 << mux->gate_bit_idx)) &&
				~(apb_reg_val & (0x1 << mux->apb_gate_bit_idx)));
		}
	} else if (mux->gate_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(mux->lock, flags);
		/* Check if RSTN bit is released */
		reg_val = CG_GETREG(mux->gate_reg_offset);
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(mux->lock, flags);
		if (strncmp("eth0_extphy_clk", mux->name, sizeof(mux->name)) == 0) {
			return (reg_val & (0x1 << mux->gate_bit_idx));
		} else if (strncmp("eth1_extphy_clk", mux->name, sizeof(mux->name)) == 0) {
			return (reg_val & (0x1 << mux->gate_bit_idx));
		} else {
			return ~(reg_val & (0x1 << mux->gate_bit_idx));
		}
	} else if (mux->apb_gate_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(mux->lock, flags);
		/* Check if RSTN bit is released */
		apb_reg_val = CG_GETREG(mux->apb_gate_reg_offset);
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(mux->lock, flags);
		if (strncmp("eth0_extphy_clk", mux->name, sizeof(mux->name)) == 0) {
			return (apb_reg_val & (0x1 << mux->apb_gate_bit_idx));
		} else if (strncmp("eth1_extphy_clk", mux->name, sizeof(mux->name)) == 0) {
			return (apb_reg_val & (0x1 << mux->apb_gate_bit_idx));
		} else {
			return ~(apb_reg_val & (0x1 << mux->apb_gate_bit_idx));
		}
	} else {
		return 0;
	}
}

static long nvt_composite_mux_clk_round_rate(struct clk_hw *hw,
					     unsigned long rate,
					     unsigned long *prate)
{
	struct clk_mux *cmux = container_of(hw, struct clk_mux, hw);
	struct nvt_composite_mux_clk *mux =
	    container_of(cmux, struct nvt_composite_mux_clk, mux);
	u32 best_div, max_div;

	if (!(mux->div_bit_width))
		return mux->current_rate;

	max_div = div_mask(mux->div_bit_width);

	best_div = DIV_ROUND_UP(*prate, rate);

	if (best_div < 1)
		best_div = 1;
	if (best_div > max_div)
		best_div = max_div;

	mux->current_rate = *prate/best_div;

	return mux->current_rate;
}

static unsigned long nvt_composite_mux_clk_recalc_rate(struct clk_hw
						       *hw,
						       unsigned long
						       parent_rate)
{
	struct clk_mux *cmux = container_of(hw, struct clk_mux, hw);
	struct nvt_composite_mux_clk *mux =
	    container_of(cmux, struct nvt_composite_mux_clk, mux);
	unsigned long flags;
	u32 divider, reg_val;

	if (mux->div_bit_width & SWDIVX) {
		/* SWDIVX to correct software value */
		mux->current_rate = parent_rate / (mux->div_bit_width & 0xF);
	} else if ((!(mux->div_reg_offset)) || (!(mux->div_bit_width))) {
		/* If there is no divider, parent rate equals to current rate */
		mux->current_rate = parent_rate;
	} else {
		/* Calling clk_set_parent will update parent_idx and parent_rate, and current_rate should be updated via recalc_rate ops */
		spin_lock_irqsave(mux->lock, flags);
		reg_val = CG_GETREG(mux->div_reg_offset);
		reg_val &= (div_mask(mux->div_bit_width) << mux->div_bit_idx);
		divider = (reg_val >> mux->div_bit_idx);
		spin_unlock_irqrestore(mux->lock, flags);

		mux->current_rate = parent_rate/(divider+1);
	}

	return mux->current_rate;
}

static int nvt_composite_mux_clk_set_rate(struct clk_hw *hw,
					  unsigned long rate,
					  unsigned long parent_rate)
{
	struct clk_mux *cmux = container_of(hw, struct clk_mux, hw);
	struct nvt_composite_mux_clk *mux =
	    container_of(cmux, struct nvt_composite_mux_clk, mux);
	unsigned long flags;
	u32 divider, reg_val;

	if (!(mux->div_reg_offset))
		return 0;

	divider = (parent_rate / rate);
	if (!divider)
		divider = 1;

	spin_lock_irqsave(mux->lock, flags);
	reg_val = CG_GETREG(mux->div_reg_offset);
	reg_val &= ~(div_mask(mux->div_bit_width) << mux->div_bit_idx);
	reg_val |= ((divider - 1) << mux->div_bit_idx);
	CG_SETREG(mux->div_reg_offset, reg_val);
	spin_unlock_irqrestore(mux->lock, flags);

	return 0;
}

static int nvt_composite_mux_clk_set_parent(struct clk_hw *hw, u8 index)
{
	struct clk_mux *cmux = container_of(hw, struct clk_mux, hw);
	struct nvt_composite_mux_clk *mux =
	    container_of(cmux, struct nvt_composite_mux_clk, mux);
	unsigned long flags;
	u32 reg_val;

	if (mux->mux_reg_offset != 0) {
		spin_lock_irqsave(mux->lock, flags);
		reg_val = CG_GETREG(mux->mux_reg_offset);
		reg_val &= ~(div_mask(mux->mux_bit_width) << mux->mux_bit_idx);
		reg_val |= (index << mux->mux_bit_idx);
		CG_SETREG(mux->mux_reg_offset, reg_val);
		spin_unlock_irqrestore(mux->lock, flags);
	}

	return 0;
}

static u8 nvt_composite_mux_clk_get_parent(struct clk_hw *hw)
{
	struct clk_mux *cmux = container_of(hw, struct clk_mux, hw);
	struct nvt_composite_mux_clk *mux =
	    container_of(cmux, struct nvt_composite_mux_clk, mux);
	u32 reg_val, index;

	reg_val = CG_GETREG(mux->mux_reg_offset);
	index = reg_val & (div_mask(mux->mux_bit_width) << mux->mux_bit_idx);

	return index >> mux->mux_bit_idx;
}

static const struct clk_ops nvt_composite_mux_clk_ops = {
	.prepare = nvt_composite_mux_clk_prepare,
	.unprepare = nvt_composite_mux_clk_unprepare,
	.is_prepared = nvt_composite_mux_clk_is_prepared,
	.enable = nvt_composite_mux_clk_enable,
	.disable = nvt_composite_mux_clk_disable,
	.is_enabled = nvt_composite_mux_clk_is_enabled,
	.round_rate = nvt_composite_mux_clk_round_rate,
	.recalc_rate = nvt_composite_mux_clk_recalc_rate,
	.set_rate = nvt_composite_mux_clk_set_rate,
	.set_parent = nvt_composite_mux_clk_set_parent,
	.get_parent = nvt_composite_mux_clk_get_parent,
};

int nvt_composite_mux_clk_register(struct nvt_composite_mux_clk
				   composite_mux_clks[], int array_size,
				   spinlock_t *cg_spinlock)
{
	struct clk *clk;
	struct clk_init_data init;
	struct nvt_composite_mux_clk *p_cmux_clk;
	int i, ret = 0;

	for (i = 0; i < array_size; i++) {
		p_cmux_clk = NULL;
		p_cmux_clk = kzalloc(sizeof(struct nvt_composite_mux_clk), GFP_KERNEL);
		if (!p_cmux_clk) {
			pr_err("%s: failed to alloc cmux_clk\n", __func__);
			return -EPERM;
		}

		memcpy(p_cmux_clk->name, composite_mux_clks[i].name, CLK_NAME_STR_SIZE);

		p_cmux_clk->mux_reg_offset = composite_mux_clks[i].mux_reg_offset;
		p_cmux_clk->mux_bit_idx = composite_mux_clks[i].mux_bit_idx;
		p_cmux_clk->mux_bit_width = composite_mux_clks[i].mux_bit_width;

		p_cmux_clk->div_reg_offset = composite_mux_clks[i].div_reg_offset;
		p_cmux_clk->div_bit_idx = composite_mux_clks[i].div_bit_idx;
		p_cmux_clk->div_bit_width = composite_mux_clks[i].div_bit_width;

		p_cmux_clk->gate_reg_offset = composite_mux_clks[i].gate_reg_offset;
		p_cmux_clk->gate_bit_idx = composite_mux_clks[i].gate_bit_idx;
		p_cmux_clk->apb_gate_reg_offset = composite_mux_clks[i].apb_gate_reg_offset;
		p_cmux_clk->apb_gate_bit_idx = composite_mux_clks[i].apb_gate_bit_idx;
		p_cmux_clk->do_enable = composite_mux_clks[i].do_enable;

		p_cmux_clk->reset_reg_offset = composite_mux_clks[i].reset_reg_offset;
		p_cmux_clk->reset_bit_idx = composite_mux_clks[i].reset_bit_idx;
		p_cmux_clk->apb_reset_reg_offset = composite_mux_clks[i].apb_reset_reg_offset;
		p_cmux_clk->apb_reset_bit_idx = composite_mux_clks[i].apb_reset_bit_idx;
		p_cmux_clk->do_reset = composite_mux_clks[i].do_reset;

		p_cmux_clk->lock = cg_spinlock;

		init.name = composite_mux_clks[i].name;
		init.ops = &nvt_composite_mux_clk_ops;
		init.flags = CLK_IGNORE_UNUSED | CLK_GET_RATE_NOCACHE;
		init.num_parents = composite_mux_clks[i].num_parents;
		init.parent_names = composite_mux_clks[i].parent_names;

		/* struct clk_mux assignments */
		p_cmux_clk->mux.reg = cg_base + composite_mux_clks[i].mux_reg_offset;
		p_cmux_clk->mux.shift = composite_mux_clks[i].mux_bit_idx;
		p_cmux_clk->mux.mask = (u32)(BIT(composite_mux_clks[i].mux_bit_width) - 1);
		p_cmux_clk->mux.flags = composite_mux_clks[i].mux_flags;
		p_cmux_clk->mux.lock = cg_spinlock;
		p_cmux_clk->mux.table = NULL;
		p_cmux_clk->mux.hw.init = &init;

		clk = clk_register(NULL, &(p_cmux_clk->mux.hw));
		if (IS_ERR(clk)) {
			pr_err("%s: failed to register clk %s\n", __func__, p_cmux_clk->name);
			ret = -EPERM;
			goto err;
		}

		if (clk_register_clkdev(clk, p_cmux_clk->name, NULL)) {
			pr_err("%s: failed to register lookup %s\n", __func__, p_cmux_clk->name);
			ret = -EPERM;
			goto err;
		} else {
			if (composite_mux_clks[i].do_enable) {
				ret = clk_prepare_enable(clk);
				if (ret < 0)
					pr_err("%s prepare & enable failed!\n", p_cmux_clk->name);
			}
		}

		clk_set_parent(clk, __clk_lookup(composite_mux_clks[i].parent_names[composite_mux_clks[i].parent_idx]));

		p_cmux_clk->parent_rate = clk_hw_get_rate(clk_hw_get_parent(__clk_get_hw(clk)));

		if (composite_mux_clks[i].div_bit_width) {
			clk_set_rate(clk, composite_mux_clks[i].current_rate);
			p_cmux_clk->current_rate = clk_hw_get_rate(__clk_get_hw(clk));
		} else
			p_cmux_clk->current_rate = composite_mux_clks[i].current_rate;
	}

err:
	return ret;
}
