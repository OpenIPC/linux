/**
    NVT clock management module
    To handle Linux clock framework API interface
    @file nvt-im-clk.c
    @ingroup
    @note
    Copyright Novatek Microelectronics Corp. 2017. All rights reserved.

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
#include <plat/top.h>
#include <plat/cg-reg.h>
#define div_mask(width)	((1 << (width)) - 1)

static int log_index_cgate = 0;
static int log_index_cmux = 0;

void __iomem *cg_base = NULL;
int fastboot = 0x0;
int logoboot = 0x0;

#define CG_SETREG(ofs, value)	writel((value), (cg_base+(ofs)))
#define CG_GETREG(ofs)		readl(cg_base+(ofs))

void nvt_fastboot_detect(int fastboot_init)
{
	fastboot = fastboot_init;
}

void nvt_logoboot_detect(int logo_init)
{
	logoboot = logo_init;
}

void nvt_cg_base_remap(void __iomem *remap_base)
{
	cg_base = remap_base;
}

void nvt_init_clk_auto_gating(struct nvt_clk_gating_init nvt_clk_gating_init[], 
				int array_size, spinlock_t *lock)
{
	unsigned long flags;
	int i = 0;

	spin_lock_irqsave(lock, flags);
	for (i = 0; i < array_size; i++)
		CG_SETREG(nvt_clk_gating_init[i].autogating_reg_offset, \
			nvt_clk_gating_init[i].autogating_value);
	spin_unlock_irqrestore(lock, flags);
}

int nvt_fixed_rate_clk_register(struct nvt_fixed_rate_clk fixed_rate_clks[],
				int array_size)
{
	struct clk *clk;
	int i, ret = 0;

	for (i = 0; i < array_size; i++) {
		clk =
		    clk_register_fixed_rate(NULL, fixed_rate_clks[i].name, NULL, 0,
					    fixed_rate_clks[i].fixed_rate);
		if (IS_ERR(clk)) {
			pr_err
			    ("%s: failed to register fixed rate clock \"%s\"\n",
			     __func__, fixed_rate_clks[i].name);
			ret = -EPERM;
			goto err;
		}
		if (clk_register_clkdev(clk, fixed_rate_clks[i].name, NULL)) {
			pr_err("%s: failed to register lookup %s!\n", __func__,
			    fixed_rate_clks[i].name);
			ret = -EPERM;
			goto err;
		}
		else {
			ret = clk_prepare_enable(clk);
			if (ret < 0)
				pr_err
				    ("Fixed clock prepare & enable failed!\n");
		}
	}

err:
	return ret;
}

static int nvt_pll_clk_enable(struct clk_hw *hw)
{
	unsigned int reg_val;
	unsigned long flags;
	struct nvt_pll_clk *pll = container_of(hw, struct nvt_pll_clk, pll_hw);

	/* race condition protect. enter critical section */
	spin_lock_irqsave(pll->lock, flags);
	reg_val = CG_GETREG(pll->gate_reg_offset);
	reg_val |= (0x1 << pll->gate_bit_idx);
	CG_SETREG(pll->gate_reg_offset, reg_val);

	if (!(CG_GETREG(pll->status_reg_offset) & (0x1 << pll->gate_bit_idx)))
		mdelay(3);

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
	is_enabled =
	    CG_GETREG(pll->gate_reg_offset) & (0x1 << pll->gate_bit_idx);
	/* race condition protect. leave critical section */
	spin_unlock_irqrestore(pll->lock, flags);

	return is_enabled;
}

static long nvt_pll_clk_round_rate(struct clk_hw *hw, unsigned long rate,
				   unsigned long *parent_rate)
{
	struct nvt_pll_clk *pll = container_of(hw, struct nvt_pll_clk, pll_hw);
#ifndef CONFIG_NVT_FPGA_EMULATION
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
#endif
	return pll->current_rate;
}

static unsigned long nvt_pll_clk_recalc_rate(struct clk_hw *hw,
					     unsigned long parent_rate)
{
	struct nvt_pll_clk *pll = container_of(hw, struct nvt_pll_clk, pll_hw);
#ifndef CONFIG_NVT_FPGA_EMULATION
	unsigned long long temp_rate;
	unsigned int ratio0, ratio1, ratio2;

	ratio0 = CG_GETREG(pll->rate_reg_offset) & 0xFF;
	ratio1 = CG_GETREG(pll->rate_reg_offset + 0x4) & 0xFF;
	ratio2 = CG_GETREG(pll->rate_reg_offset + 0x8) & 0xFF;

	pll->pll_ratio = ratio0 | (ratio1 << 8) | (ratio2 << 16);

	temp_rate = pll->pll_ratio * pll->parent_rate;

	do_div(temp_rate, pll_cal_factor);

	pll->current_rate = temp_rate;
#endif
	return pll->current_rate;
}

static int nvt_pll_clk_set_rate(struct clk_hw *hw, unsigned long rate,
				unsigned long parent_rate)
{
#ifndef CONFIG_NVT_FPGA_EMULATION
	struct nvt_pll_clk *pll = container_of(hw, struct nvt_pll_clk, pll_hw);
	unsigned long flags;

	spin_lock_irqsave(pll->lock, flags);
	if ((nvt_get_chip_id() == CHIP_NA51084) && (pll->gate_bit_idx == PLL_CPU_BIT)) {
		/* set mux_sel_sw to 0 for choose synthesizer output as MPLL/CPU_CK */
		CG_SETREG(CG528_ARMPLL_CONTROL0_REG_OFFSET, 0x2);
		/* assert resetb_armpll to 0 */
		CG_SETREG(CG528_ARMPLL_CONTROL1_REG_OFFSET, 0x0);
	}

	CG_SETREG(pll->rate_reg_offset, pll->pll_ratio & 0xFF);
	CG_SETREG(pll->rate_reg_offset + 0x4, (pll->pll_ratio >> 8) & 0xFF);
	CG_SETREG(pll->rate_reg_offset + 0x8, (pll->pll_ratio >> 16) & 0xFF);

	if ((nvt_get_chip_id() == CHIP_NA51084) && (pll->gate_bit_idx == PLL_CPU_BIT)) {
		unsigned long pll_rate = pll->current_rate * 8;
		u32 reg;

		udelay(50);

		reg = CG_GETREG(CG528_ARMPLL_CONTROL2_REG_OFFSET) & (~(0x3 << 5));
		if (pll_rate < 1000000000)
			reg |= (0x1 << 5);
		else if (pll_rate < 1400000000)
			reg |= (0x2 << 5);
		else
			reg |= (0x3 << 5);

		CG_SETREG(CG528_ARMPLL_CONTROL2_REG_OFFSET, reg);
		/* assert resetb_armpll to 1 */
		CG_SETREG(CG528_ARMPLL_CONTROL1_REG_OFFSET, 0x10);

		/*Wait 500us for ARMPLL stable*/
		udelay(500);

		/* set mux_sel_sw to 1 for choose synthesizer output as MPLL/CPU_CK */
		CG_SETREG(CG528_ARMPLL_CONTROL0_REG_OFFSET, 0x3);
	}
	spin_unlock_irqrestore(pll->lock, flags);
#endif
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

		clk =
		    clk_register_fixed_factor(NULL, pstr, "osc_in", 0,
					      pll_clks[i].pll_ratio,
					      pll_div_value);
		p_pll_clk->fixed_factor = __clk_get_hw(clk);
		p_pll_clk->parent_rate = clk_get_rate(clk_get_parent(clk));
#ifndef CONFIG_NVT_FPGA_EMULATION
		p_pll_clk->current_rate = clk_get_rate(clk);
#else
		p_pll_clk->current_rate = pll_clks[i].pll_ratio;
#endif
		memcpy(p_pll_clk->name, pll_clks[i].name, CLK_NAME_STR_SIZE);
		p_pll_clk->pll_ratio = pll_clks[i].pll_ratio;
		p_pll_clk->rate_reg_offset = pll_clks[i].rate_reg_offset;
		p_pll_clk->gate_reg_offset = pll_clks[i].gate_reg_offset;
		p_pll_clk->gate_bit_idx = pll_clks[i].gate_bit_idx;
		p_pll_clk->status_reg_offset = pll_clks[i].status_reg_offset;
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
			pr_err("%s: failed to register clk %s!\n", __func__,
			       p_pll_clk->name);
			ret = -EPERM;
			goto err;
		}
		if (clk_register_clkdev(clk, p_pll_clk->name, NULL)) {
			pr_err("%s: failed to register lookup %s!\n", __func__,
			       p_pll_clk->name);
			ret = -EPERM;
			goto err;
		} else {
			if (__clk_is_enabled(clk)) {
				ret = clk_prepare_enable(clk);
				if (ret < 0)
					pr_err("%s prepare & enable failed!\n",
						p_pll_clk->name);
			}
		}
	}

	kfree(pstr);

err:
	return ret;
}

static int nvt_composite_gate_clk_prepare(struct clk_hw *hw)
{
	unsigned int reg_val;
	unsigned long flags;
	struct nvt_composite_gate_clk *gate =
	    container_of(hw, struct nvt_composite_gate_clk, cgate_hw);

	if (gate->do_reset) {
		if (gate->reset_reg_offset != 0) {
			/* race condition protect. enter critical section */
			spin_lock_irqsave(gate->lock, flags);
			reg_val = CG_GETREG(gate->reset_reg_offset);
			reg_val &= ~(0x1 << gate->reset_bit_idx);
			CG_SETREG(gate->reset_reg_offset, reg_val);
			/* race condition protect. leave critical section */
			spin_unlock_irqrestore(gate->lock, flags);
			udelay(10);
		}
	}
	if (gate->reset_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(gate->lock, flags);
		reg_val = CG_GETREG(gate->reset_reg_offset);
		if ((reg_val & (0x1 << gate->reset_bit_idx)) == 0) {
			reg_val |= 0x1 << gate->reset_bit_idx;
			CG_SETREG(gate->reset_reg_offset, reg_val);
		}
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(gate->lock, flags);
	}
	return 0;
}

static void nvt_composite_gate_clk_unprepare(struct clk_hw *hw)
{
	/* Do nothing */
}

static int nvt_composite_gate_clk_is_prepared(struct clk_hw *hw)
{
	unsigned int reg_val;
	unsigned long flags;
	struct nvt_composite_gate_clk *gate =
	    container_of(hw, struct nvt_composite_gate_clk, cgate_hw);

	if (gate->reset_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(gate->lock, flags);
		/* Check if RSTN bit is released */
		reg_val = CG_GETREG(gate->reset_reg_offset);
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(gate->lock, flags);
		return (reg_val && (0x1 << gate->reset_bit_idx));
	} else
		return 0;
}

static int nvt_composite_gate_clk_enable(struct clk_hw *hw)
{
	unsigned int reg_val;
	unsigned long flags;
	struct nvt_composite_gate_clk *gate =
	    container_of(hw, struct nvt_composite_gate_clk, cgate_hw);

	if ((gate->max_rate) && (gate->current_rate > gate->max_rate)) {
		pr_err("!!!! %s: %s current_freq(%ld) exceed max_freq(%ld), return !!!!\n",
			__func__, gate->name, gate->current_rate, gate->max_rate);
		return -1;
	}

	if (gate->gate_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(gate->lock, flags);
		reg_val = CG_GETREG(gate->gate_reg_offset);
		reg_val |= (0x1 << gate->gate_bit_idx);
		CG_SETREG(gate->gate_reg_offset, reg_val);
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(gate->lock, flags);
	}

	return 0;
}

static void nvt_composite_gate_clk_disable(struct clk_hw *hw)
{
	unsigned int reg_val;
	unsigned long flags;
	struct nvt_composite_gate_clk *gate =
	    container_of(hw, struct nvt_composite_gate_clk, cgate_hw);

	if (gate->gate_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(gate->lock, flags);
		reg_val = CG_GETREG(gate->gate_reg_offset);
		reg_val &= ~(0x1 << gate->gate_bit_idx);
		CG_SETREG(gate->gate_reg_offset, reg_val);
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(gate->lock, flags);
	}
}

static int nvt_composite_gate_clk_is_enabled(struct clk_hw *hw)
{
	unsigned long flags;
	bool is_enabled;
	struct nvt_composite_gate_clk *gate =
	    container_of(hw, struct nvt_composite_gate_clk, cgate_hw);

	/* race condition protect. enter critical section */
	spin_lock_irqsave(gate->lock, flags);
	is_enabled =
	    CG_GETREG(gate->gate_reg_offset) & (0x1 << gate->gate_bit_idx);
	/* race condition protect. leave critical section */
	spin_unlock_irqrestore(gate->lock, flags);

	return is_enabled;
}

static long nvt_composite_gate_clk_round_rate(struct clk_hw *hw,
					      unsigned long rate,
					      unsigned long *parent_rate)
{
	struct nvt_composite_gate_clk *gate =
	    container_of(hw, struct nvt_composite_gate_clk, cgate_hw);

	if ((gate->max_rate) && (gate->current_rate > gate->max_rate) && nvt_composite_gate_clk_is_enabled(hw) && log_index_cgate) {
		pr_err("!!!! %s: %s current_freq(%ld) exceed max_freq(%ld) !!!!\n",
			__func__, gate->name, gate->current_rate, gate->max_rate);
	}

	if (gate->divider == NULL)
		return rate;
	else
		return clk_divider_ops.round_rate(gate->divider, rate,
						  &gate->parent_rate);
}

static unsigned long nvt_composite_gate_clk_recalc_rate(struct clk_hw *hw,
							unsigned long
							parent_rate)
{
	struct nvt_composite_gate_clk *gate =
	    container_of(hw, struct nvt_composite_gate_clk, cgate_hw);

	if ((gate->max_rate) && (gate->current_rate > gate->max_rate) && nvt_composite_gate_clk_is_enabled(hw) && log_index_cgate) {
		pr_err("!!!! %s: %s current_freq(%ld) exceed max_freq(%ld) !!!!\n",
			__func__, gate->name, gate->current_rate, gate->max_rate);
	}

	return gate->current_rate;
}

static int nvt_composite_gate_clk_set_rate(struct clk_hw *hw,
					   unsigned long rate,
					   unsigned long parent_rate)
{
	int ret;
	struct nvt_composite_gate_clk *gate =
	    container_of(hw, struct nvt_composite_gate_clk, cgate_hw);

	if (gate->divider == NULL)
		return -EPERM;

	if ((gate->max_rate) && (gate->current_rate > gate->max_rate)) {
		pr_err("!!!! %s: %s current_freq(%ld) exceed max_freq(%ld), return !!!!\n",
			__func__, gate->name, gate->current_rate, gate->max_rate);
		return -1;
	}

	/* race condition protect. enter critical section */
	ret = clk_divider_ops.set_rate(gate->divider, rate, gate->parent_rate);
	/* race condition protect. leave critical section */
	if (ret == 0)
		gate->current_rate = rate;
	return ret;
}

static int nvt_pll_clk_set_ss_rate(struct clk_hw *hw, int steps)
{
	struct nvt_composite_gate_clk *gate =
	    container_of(hw, struct nvt_composite_gate_clk, cgate_hw);
	unsigned long flags;
	unsigned int reg_val;
	unsigned long ssc_step = 0x2;
	unsigned long ssc_period_value = 1 << 7;

	spin_lock_irqsave(gate->lock, flags);
	/*Disable PLLx*/
	reg_val = CG_GETREG(0x0);
	reg_val &= ~(0x1 << gate->gate_bit_idx);
	CG_SETREG(0x0, reg_val);

	/* Setup period vlaue */
	CG_SETREG(gate->autogating_reg_offset, ssc_period_value);
	/* Setup MULFAC */
	CG_SETREG(gate->reset_reg_offset, steps);
	/* Setup SET_SEL */

	if ((nvt_get_chip_id() == CHIP_NA51055)) {
		reg_val = CG_GETREG(gate->gate_reg_offset);
		reg_val &= ~(0x3 << SSC_STEP);
		reg_val |= (ssc_step << SSC_STEP);
		CG_SETREG(gate->gate_reg_offset, reg_val);
		/*Write 1 to SSC_NEW_MODE, DSSC*/
		reg_val &= ~((0x1 << SSC_NEW_MODE) | (0x1 << SSC_DSSC));
		reg_val |= (0x1 << SSC_NEW_MODE) | (0x1 << SSC_DSSC);
		CG_SETREG(gate->gate_reg_offset, reg_val);
		/*Write 1 to SSC_EN*/
		reg_val = CG_GETREG(gate->div_reg_offset);
		if (steps == 0) {
			reg_val &= ~0x1;
		} else {
			reg_val |= 0x1;
		}
		CG_SETREG(gate->div_reg_offset, reg_val);
	} else {
		reg_val = CG_GETREG(gate->div_reg_offset);
		reg_val &= ~(0x3 << SSC_528_STEP);
		reg_val |= (ssc_step << SSC_528_STEP);
		CG_SETREG(gate->div_reg_offset, reg_val);
		/*Write 1 to SSC_NEW_MODE, DSSC*/
		reg_val &= ~((0x1 << SSC_528_NEW_MODE) | (0x1 << SSC_528_DSSC));
		reg_val |= (0x1 << SSC_528_NEW_MODE) | (0x1 << SSC_528_DSSC);
		CG_SETREG(gate->div_reg_offset, reg_val);
		/*Write 1 to SSC_EN*/
		reg_val = CG_GETREG(gate->div_reg_offset);
		if (steps == 0) {
			reg_val &= ~0x1;
		} else {
			reg_val |= 0x1;
		}
		CG_SETREG(gate->div_reg_offset, reg_val);
	}

	/*Enable PLLx*/
	reg_val = CG_GETREG(0x0);
	reg_val |= (0x1 << gate->gate_bit_idx);
	CG_SETREG(0x0, reg_val);

	spin_unlock_irqrestore(gate->lock, flags);
	return 0;
}

static int nvt_composite_gate_clk_set_phase(struct clk_hw *hw, int enable)
{
	unsigned int reg_val;
	unsigned long flags;
	struct nvt_composite_gate_clk *gate =
	    container_of(hw, struct nvt_composite_gate_clk, cgate_hw);

	if (gate->gate_reg_offset > SSC_BOUND) {
		nvt_pll_clk_set_ss_rate(hw, enable);
	} else if (gate->autogating_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(gate->lock, flags);
		reg_val = CG_GETREG(gate->autogating_reg_offset);

		if (enable == 0)
			reg_val &= ~(0x1 << gate->autogating_bit_idx);
		else
			reg_val |= (0x1 << gate->autogating_bit_idx);

		CG_SETREG(gate->autogating_reg_offset, reg_val);
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(gate->lock, flags);
	}

	return 0;
}

static int nvt_composite_gate_clk_get_phase(struct clk_hw *hw)
{
	unsigned long flags;
	int is_enabled = 0;
	struct nvt_composite_gate_clk *gate =
	    container_of(hw, struct nvt_composite_gate_clk, cgate_hw);

	if (gate->autogating_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(gate->lock, flags);
		is_enabled =
		    (CG_GETREG(gate->autogating_reg_offset) &
		    (0x1 << gate->autogating_bit_idx)) ? 1 : 0;
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(gate->lock, flags);
	}

	return is_enabled;
}

static const struct clk_ops nvt_composite_gate_clk_ops = {
	.prepare = nvt_composite_gate_clk_prepare,
	.unprepare = nvt_composite_gate_clk_unprepare,
	.is_prepared = nvt_composite_gate_clk_is_prepared,
	.enable = nvt_composite_gate_clk_enable,
	.disable = nvt_composite_gate_clk_disable,
	.is_enabled = nvt_composite_gate_clk_is_enabled,
	.round_rate = nvt_composite_gate_clk_round_rate,
	.recalc_rate = nvt_composite_gate_clk_recalc_rate,
	.set_rate = nvt_composite_gate_clk_set_rate,
	.set_phase = nvt_composite_gate_clk_set_phase,
	.get_phase = nvt_composite_gate_clk_get_phase,
};

int nvt_composite_gate_clk_register(struct nvt_composite_gate_clk
				    composite_gate_clks[], int array_size,
				    spinlock_t *cg_spinlock)
{
	struct clk *clk;
	struct clk_init_data init;
	struct nvt_composite_gate_clk *p_cgate_clk;
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
		snprintf(pstr, CLK_NAME_STR_SIZE, "%s_div",
			 composite_gate_clks[i].name);

		p_cgate_clk = NULL;
		p_cgate_clk =
		    kzalloc(sizeof(struct nvt_composite_gate_clk), GFP_KERNEL);
		if (!p_cgate_clk) {
			pr_err("%s: failed to alloc cgate_clk!\n", __func__);
			kfree(pstr);
			return -EPERM;
		}

		memcpy(p_cgate_clk->name, composite_gate_clks[i].name,
		       CLK_NAME_STR_SIZE);
		p_cgate_clk->div_reg_offset =
		    composite_gate_clks[i].div_reg_offset;
		p_cgate_clk->div_bit_idx = composite_gate_clks[i].div_bit_idx;
		p_cgate_clk->div_bit_width =
		    composite_gate_clks[i].div_bit_width;
		p_cgate_clk->gate_reg_offset =
		    composite_gate_clks[i].gate_reg_offset;
		p_cgate_clk->gate_bit_idx = composite_gate_clks[i].gate_bit_idx;
		p_cgate_clk->do_enable = composite_gate_clks[i].do_enable;
		p_cgate_clk->reset_reg_offset =
		    composite_gate_clks[i].reset_reg_offset;
		p_cgate_clk->reset_bit_idx =
		    composite_gate_clks[i].reset_bit_idx;
		p_cgate_clk->do_reset = composite_gate_clks[i].do_reset;
		p_cgate_clk->autogating_reg_offset =
		    composite_gate_clks[i].autogating_reg_offset;
		p_cgate_clk->autogating_bit_idx =
		    composite_gate_clks[i].autogating_bit_idx;
		p_cgate_clk->do_autogating = composite_gate_clks[i].do_autogating;
		p_cgate_clk->lock = cg_spinlock;

		if (composite_gate_clks[i].div_bit_width == 0) {
			/* No clock divider */
			snprintf(pstr, CLK_NAME_STR_SIZE, "%s",
				 composite_gate_clks[i].parent_name);
			p_cgate_clk->divider = NULL;
		} else {
			clk =
			    clk_register_divider(NULL, pstr,
						 composite_gate_clks
						 [i].parent_name,
						 CLK_GET_RATE_NOCACHE,
						 cg_base +
						 composite_gate_clks
						 [i].div_reg_offset,
						 composite_gate_clks
						 [i].div_bit_idx,
						 composite_gate_clks
						 [i].div_bit_width,
						 composite_gate_clks
						 [i].div_flags, cg_spinlock);
			p_cgate_clk->divider = __clk_get_hw(clk);
			p_cgate_clk->parent_rate =
			    clk_get_rate(clk_get_parent(clk));
		}

		if (p_cgate_clk->divider != NULL) {
			if (!fastboot) {
				/* race condition protect. enter critical section */
				clk_divider_ops.set_rate(p_cgate_clk->divider,
							composite_gate_clks
							[i].current_rate,
							p_cgate_clk->parent_rate);
				/* race condition protect. leave critical section */
			}

			p_cgate_clk->current_rate =
				composite_gate_clks[i].current_rate;
		}

		init.name = composite_gate_clks[i].name;
		init.ops = &nvt_composite_gate_clk_ops;
		init.flags = CLK_IGNORE_UNUSED | CLK_GET_RATE_NOCACHE;
		init.num_parents = 1;
		parent_name = pstr;
		init.parent_names = &parent_name;
		p_cgate_clk->cgate_hw.init = &init;
		clk = clk_register(NULL, &(p_cgate_clk->cgate_hw));
		if (IS_ERR(clk)) {
			pr_err("%s: failed to register clk %s!\n", __func__,
			       p_cgate_clk->name);
			ret = -EPERM;
			goto err;
		}
		if (clk_register_clkdev(clk, p_cgate_clk->name, NULL)) {
			pr_err("%s: failed to register lookup %s!\n", __func__,
			       p_cgate_clk->name);
			ret = -EPERM;
			goto err;
		} else {
			if (p_cgate_clk->divider == NULL) {
				p_cgate_clk->parent_rate =
				    p_cgate_clk->current_rate =
				    clk_get_rate(clk_get_parent(clk));
			}
			if (composite_gate_clks[i].do_enable) {
				ret = clk_prepare_enable(clk);
				if (ret < 0)
					pr_err("%s prepare & enable failed!\n",
					       p_cgate_clk->name);
			}

			ret = clk_set_phase(clk, composite_gate_clks[i].do_autogating);
			if (ret < 0)
				pr_err("%s enable autogating failed!\n",
				       p_cgate_clk->name);
		}
	}

	log_index_cgate = 1;

	kfree(pstr);

err:
	return ret;
}

static int nvt_composite_group_pwm_clk_prepare(struct clk_hw *hw)
{
	unsigned int reg_val;
	unsigned long flags;
	struct nvt_composite_group_pwm_clk *gpwm =
	    container_of(hw, struct nvt_composite_group_pwm_clk, cgpwm_hw);

	if (gpwm->reset_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(gpwm->lock, flags);
		reg_val = CG_GETREG(gpwm->reset_reg_offset);
		if ((reg_val & (0x1 << gpwm->reset_bit_idx)) == 0) {
			reg_val |= 0x1 << gpwm->reset_bit_idx;
			CG_SETREG(gpwm->reset_reg_offset, reg_val);
		}
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(gpwm->lock, flags);
	}

	return 0;
}

static void nvt_composite_group_pwm_clk_unprepare(struct clk_hw *hw)
{
	/* Do nothing */
}

static int nvt_composite_group_pwm_clk_is_prepared(struct clk_hw *hw)
{
	unsigned int reg_val;
	unsigned long flags;
	struct nvt_composite_group_pwm_clk *gpwm =
	    container_of(hw, struct nvt_composite_group_pwm_clk, cgpwm_hw);

	if (gpwm->reset_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(gpwm->lock, flags);
		/* Check if RSTN bit is released */
		reg_val = CG_GETREG(gpwm->reset_reg_offset);
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(gpwm->lock, flags);
		return (reg_val && (0x1 << gpwm->reset_bit_idx));
	} else
		return 0;
}

static int nvt_composite_group_pwm_clk_enable(struct clk_hw *hw)
{
	unsigned int reg_val;
	unsigned long flags;
	struct nvt_composite_group_pwm_clk *gpwm =
	    container_of(hw, struct nvt_composite_group_pwm_clk, cgpwm_hw);

	/* race condition protect. enter critical section */
	spin_lock_irqsave(gpwm->lock, flags);
	reg_val = CG_GETREG(gpwm->gate_reg_offset);
	reg_val |= (0x1 << gpwm->gate_bit_idx);
	CG_SETREG(gpwm->gate_reg_offset, reg_val);
	/* race condition protect. leave critical section */
	spin_unlock_irqrestore(gpwm->lock, flags);

	return 0;
}

static void nvt_composite_group_pwm_clk_disable(struct clk_hw *hw)
{
	unsigned int reg_val;
	unsigned long flags;
	struct nvt_composite_group_pwm_clk *gpwm =
	    container_of(hw, struct nvt_composite_group_pwm_clk, cgpwm_hw);
	/* race condition protect. enter critical section */
	spin_lock_irqsave(gpwm->lock, flags);
	reg_val = CG_GETREG(gpwm->gate_reg_offset);
	reg_val &= ~(0x1 << gpwm->gate_bit_idx);
	CG_SETREG(gpwm->gate_reg_offset, reg_val);
	/* race condition protect. leave critical section */
	spin_unlock_irqrestore(gpwm->lock, flags);
}

static int nvt_composite_group_pwm_clk_is_enabled(struct clk_hw *hw)
{
	unsigned long flags;
	bool is_enabled;
	struct nvt_composite_group_pwm_clk *gpwm =
	    container_of(hw, struct nvt_composite_group_pwm_clk, cgpwm_hw);
	/* race condition protect. enter critical section */
	spin_lock_irqsave(gpwm->lock, flags);
	is_enabled =
	    CG_GETREG(gpwm->gate_reg_offset) & (0x1 << gpwm->gate_bit_idx);
	/* race condition protect. leave critical section */
	spin_unlock_irqrestore(gpwm->lock, flags);

	return is_enabled;
}

static long nvt_composite_group_pwm_clk_round_rate(struct clk_hw *hw,
						   unsigned long rate,
						   unsigned long *parent_rate)
{
	struct nvt_composite_group_pwm_clk *gpwm =
	    container_of(hw, struct nvt_composite_group_pwm_clk, cgpwm_hw);

	if (gpwm->divider == NULL)
		return rate;
	else
		return clk_divider_ops.round_rate(gpwm->divider, rate,
						  &gpwm->parent_rate);
}

static unsigned long nvt_composite_group_pwm_clk_recalc_rate(struct clk_hw *hw,
							     unsigned long
							     parent_rate)
{
	struct nvt_composite_group_pwm_clk *gpwm =
	    container_of(hw, struct nvt_composite_group_pwm_clk, cgpwm_hw);
	return gpwm->current_rate;
}

static int nvt_composite_group_pwm_clk_set_rate(struct clk_hw *hw,
						unsigned long rate,
						unsigned long parent_rate)
{
	int ret;
	struct nvt_composite_group_pwm_clk *gpwm =
	    container_of(hw, struct nvt_composite_group_pwm_clk, cgpwm_hw);

	if (gpwm->divider == NULL)
		return -EPERM;
	/* race condition protect. enter critical section */
	ret = clk_divider_ops.set_rate(gpwm->divider, rate, gpwm->parent_rate);
	/* race condition protect. leave critical section */
	if (ret == 0)
		gpwm->current_rate = rate;
	return ret;
}

static const struct clk_ops nvt_composite_group_pwm_clk_ops = {
	.prepare = nvt_composite_group_pwm_clk_prepare,
	.unprepare = nvt_composite_group_pwm_clk_unprepare,
	.is_prepared = nvt_composite_group_pwm_clk_is_prepared,
	.enable = nvt_composite_group_pwm_clk_enable,
	.disable = nvt_composite_group_pwm_clk_disable,
	.is_enabled = nvt_composite_group_pwm_clk_is_enabled,
	.round_rate = nvt_composite_group_pwm_clk_round_rate,
	.recalc_rate = nvt_composite_group_pwm_clk_recalc_rate,
	.set_rate = nvt_composite_group_pwm_clk_set_rate,
};

int nvt_composite_group_pwm_clk_register(struct nvt_composite_group_pwm_clk
					 composite_group_pwm_clks[],
					 int array_size,
					 spinlock_t *cg_spinlock)
{
	struct clk *clk, *div_clk;
	struct clk_init_data init;
	struct nvt_composite_group_pwm_clk *p_cgpwm_clk;
	const char *parent_name;
	char *pstr1, *pstr2;
	int i, j, ret = 0;

	pstr1 = kzalloc(CLK_NAME_STR_SIZE, GFP_KERNEL);
	if (!pstr1) {
		ret = -EPERM;
		pr_err("%s: failed to alloc string buf!\n", __func__);
		goto err;
	}
	pstr2 = kzalloc(CLK_NAME_STR_SIZE, GFP_KERNEL);
	if (!pstr2) {
		ret = -EPERM;
		pr_err("%s: failed to alloc string buf!\n", __func__);
		goto err;
	}

	for (i = 0; i < array_size; i++) {
		if (composite_group_pwm_clks[i].div_bit_width == 0) {
			pr_err("%s: PWM clock should have a divider HW!\n",
			       __func__);
			ret = -EPERM;
			goto err;
		}
		if (composite_group_pwm_clks[i].gate_bit_start_idx >
		    composite_group_pwm_clks[i].gate_bit_end_idx) {
			pr_err
			    ("%s: PWM start idx should be less than end idx!\n",
			     __func__);
			ret = -EPERM;
			goto err;
		}
		snprintf(pstr1, CLK_NAME_STR_SIZE, "pwm_clk%d-%d_div",
			 composite_group_pwm_clks[i].gate_bit_start_idx,
			 composite_group_pwm_clks[i].gate_bit_end_idx);

		div_clk =
		    clk_register_divider(NULL, pstr1,
					 composite_group_pwm_clks
					 [i].parent_name,
					 CLK_GET_RATE_NOCACHE,
					 cg_base +
					 composite_group_pwm_clks
					 [i].div_reg_offset,
					 composite_group_pwm_clks
					 [i].div_bit_idx,
					 composite_group_pwm_clks
					 [i].div_bit_width,
					 composite_group_pwm_clks
					 [i].div_flags, cg_spinlock);

		for (j = composite_group_pwm_clks[i].gate_bit_start_idx;
		     j <= composite_group_pwm_clks[i].gate_bit_end_idx; j++) {

			p_cgpwm_clk = NULL;
			p_cgpwm_clk =
			    kzalloc(sizeof(struct nvt_composite_group_pwm_clk),
				    GFP_KERNEL);
			if (!p_cgpwm_clk) {
				pr_err("%s: failed to alloc cgpwm_clk!\n",
				       __func__);
				kfree(pstr1);
				ret = -EPERM;
				goto err;
			}

			p_cgpwm_clk->div_reg_offset =
			    composite_group_pwm_clks[i].div_reg_offset;
			p_cgpwm_clk->div_bit_idx =
			    composite_group_pwm_clks[i].div_bit_idx;
			p_cgpwm_clk->div_bit_width =
			    composite_group_pwm_clks[i].div_bit_width;
			p_cgpwm_clk->gate_reg_offset =
			    composite_group_pwm_clks[i].gate_reg_offset;
			p_cgpwm_clk->gate_bit_start_idx =
			    composite_group_pwm_clks[i].gate_bit_start_idx;
			p_cgpwm_clk->gate_bit_end_idx =
			    composite_group_pwm_clks[i].gate_bit_end_idx;
			p_cgpwm_clk->do_enable =
			    composite_group_pwm_clks[i].do_enable;
			p_cgpwm_clk->reset_reg_offset =
			    composite_group_pwm_clks[i].reset_reg_offset;
			p_cgpwm_clk->reset_bit_idx =
			    composite_group_pwm_clks[i].reset_bit_idx;
			p_cgpwm_clk->gate_bit_idx = j;
			p_cgpwm_clk->lock = cg_spinlock;

			p_cgpwm_clk->divider = __clk_get_hw(div_clk);
			p_cgpwm_clk->parent_rate =
			    clk_get_rate(clk_get_parent(div_clk));
			p_cgpwm_clk->current_rate =
			    composite_group_pwm_clks[i].current_rate;

			snprintf(pstr2, CLK_NAME_STR_SIZE, "pwm_clk.%d", j);
			init.name = pstr2;
			init.ops = &nvt_composite_group_pwm_clk_ops;
			init.flags = CLK_IGNORE_UNUSED | CLK_GET_RATE_NOCACHE;
			init.num_parents = 1;
			parent_name = pstr1;
			init.parent_names = &parent_name;

			p_cgpwm_clk->cgpwm_hw.init = &init;
			clk = clk_register(NULL, &(p_cgpwm_clk->cgpwm_hw));
			if (IS_ERR(clk)) {
				pr_err("%s: failed to register clk %s!",
				       __func__, pstr2);
				ret = -EPERM;
				goto err;
			}
			if (clk_register_clkdev(clk, pstr2, NULL)) {
				pr_err("%s: failed to register lookup %s!",
				       __func__, pstr2);
				ret = -EPERM;
				goto err;
			} else {
				if (composite_group_pwm_clks[i].do_enable) {
					ret = clk_prepare_enable(clk);
					if (ret < 0)
						pr_err
						    ("%s prepare & enable failed!",
						     pstr2);
				}
			}
		}
	}
	kfree(pstr1);
	kfree(pstr2);
err:
	return ret;
}

static int nvt_composite_mux_clk_is_enabled(struct clk_hw *hw)
{
	unsigned long flags;
	bool is_enabled;
	struct clk_mux *cmux = container_of(hw, struct clk_mux, hw);
	struct nvt_composite_mux_clk *mux =
	    container_of(cmux, struct nvt_composite_mux_clk, mux);

	/* race condition protect. enter critical section */
	spin_lock_irqsave(mux->lock, flags);
	is_enabled =
	    CG_GETREG(mux->gate_reg_offset) & (0x1 << mux->gate_bit_idx);
	/* race condition protect. leave critical section */
	spin_unlock_irqrestore(mux->lock, flags);

	return is_enabled;
}

static int nvt_composite_mux_clk_prepare(struct clk_hw *hw)
{
	unsigned int reg_val;
	unsigned long flags;
	int keep_do_reset = 1;
	struct clk_mux *cmux = container_of(hw, struct clk_mux, hw);
	struct nvt_composite_mux_clk *mux =
	    container_of(cmux, struct nvt_composite_mux_clk, mux);

	if (fastboot && nvt_composite_mux_clk_is_enabled(hw))
		keep_do_reset = 0;

	if (mux->do_reset && keep_do_reset) {
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
	}

	if (mux->reset_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(mux->lock, flags);
		reg_val = CG_GETREG(mux->reset_reg_offset);
		if ((reg_val & (0x1 << mux->reset_bit_idx)) == 0) {
			reg_val |= 0x1 << mux->reset_bit_idx;
			CG_SETREG(mux->reset_reg_offset, reg_val);
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
	unsigned int reg_val;
	unsigned long flags;
	struct clk_mux *cmux = container_of(hw, struct clk_mux, hw);
	struct nvt_composite_mux_clk *mux =
	    container_of(cmux, struct nvt_composite_mux_clk, mux);

	if (mux->reset_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(mux->lock, flags);
		/* Check if RSTN bit is released */
		reg_val = CG_GETREG(mux->reset_reg_offset);
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(mux->lock, flags);
		return (reg_val && (0x1 << mux->reset_bit_idx));
	} else
		return 0;
}

static int nvt_composite_mux_clk_enable(struct clk_hw *hw)
{
	unsigned int reg_val;
	unsigned long flags;
	struct clk_mux *cmux = container_of(hw, struct clk_mux, hw);
	struct nvt_composite_mux_clk *mux =
	    container_of(cmux, struct nvt_composite_mux_clk, mux);

	if ((mux->max_rate) && (mux->current_rate > mux->max_rate)) {
		pr_err("!!!! %s: %s current_freq(%ld) exceed max_freq(%ld), return !!!!\n",
			__func__, mux->name, mux->current_rate, mux->max_rate);
		return -1;
	}

	if (mux->gate_reg_offset == SIEMCLK_COMM_CLK_EN_REG0_OFFSET) {
		unsigned long offset = mux->gate_reg_offset & SIEMCLK_COMM_MASK;

		/* race condition protect. enter critical section */
		spin_lock_irqsave(mux->lock, flags);
		reg_val = CG_GETREG(offset);
		reg_val |= (0x1 << mux->gate_bit_idx);
		reg_val |= (0x1 << (mux->gate_bit_idx + SIEMCLK_COMM_EN_SHIFT));
		CG_SETREG(offset, reg_val);
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(mux->lock, flags);

		return 0;
	}

	if (mux->gate_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(mux->lock, flags);
		reg_val = CG_GETREG(mux->gate_reg_offset);
		reg_val |= (0x1 << mux->gate_bit_idx);
		CG_SETREG(mux->gate_reg_offset, reg_val);
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

	if (mux->gate_reg_offset == SIEMCLK_COMM_CLK_EN_REG0_OFFSET) {
		unsigned long offset = mux->gate_reg_offset & SIEMCLK_COMM_MASK;

		/* race condition protect. enter critical section */
		spin_lock_irqsave(mux->lock, flags);
		reg_val = CG_GETREG(offset);
		reg_val &= ~(0x1 << mux->gate_bit_idx);
		reg_val &= ~(0x1 << (mux->gate_bit_idx + SIEMCLK_COMM_EN_SHIFT));
		CG_SETREG(offset, reg_val);
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(mux->lock, flags);
		return;
	}

	if (mux->gate_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(mux->lock, flags);
		reg_val = CG_GETREG(mux->gate_reg_offset);
		reg_val &= ~(0x1 << mux->gate_bit_idx);
		CG_SETREG(mux->gate_reg_offset, reg_val);
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(mux->lock, flags);
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

	if ((mux->max_rate) && (mux->current_rate > mux->max_rate) && nvt_composite_mux_clk_is_enabled(hw) && log_index_cmux) {
		pr_err("!!!! %s: %s current_freq(%ld) exceed max_freq(%ld) !!!!\n",
			__func__, mux->name, mux->current_rate, mux->max_rate);
	}

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

	if (mux->div_reg_offset == SIEMCLK_COMM_CLK_DIV_REG0_OFFSET) {
		unsigned long offset = mux->div_reg_offset & SIEMCLK_COMM_MASK;
		unsigned long siemclk1_rate, siemclk2_rate;

		/*Update current rate with hardware runtime value*/
		spin_lock_irqsave(mux->lock, flags);
		reg_val = CG_GETREG(offset);
		reg_val &= (div_mask(mux->div_bit_width) << mux->div_bit_idx);
		divider = (reg_val >> mux->div_bit_idx);
		spin_unlock_irqrestore(mux->lock, flags);

		siemclk1_rate = parent_rate/(divider+1);

		spin_lock_irqsave(mux->lock, flags);
		reg_val = CG_GETREG(offset);
		reg_val &= (div_mask(mux->div_bit_width) << 
				(mux->div_bit_idx + SIEMCLK_COMM_DIV_SHIFT));
		divider = (reg_val >> 
				(mux->div_bit_idx + SIEMCLK_COMM_DIV_SHIFT));
		spin_unlock_irqrestore(mux->lock, flags);

		siemclk2_rate = parent_rate/(divider+1);

		mux->current_rate = (siemclk1_rate == siemclk2_rate) ? 
					siemclk1_rate : 0;

		return mux->current_rate;
	}

	/*If there is no divider, parent rate equals to current rate*/
	if (!(mux->div_bit_width))
		mux->current_rate = parent_rate;
	else {
		/*Update current rate with hardware runtime value*/
		spin_lock_irqsave(mux->lock, flags);
		reg_val = CG_GETREG(mux->div_reg_offset);
		reg_val &= (div_mask(mux->div_bit_width) << mux->div_bit_idx);
		divider = (reg_val >> mux->div_bit_idx);
		spin_unlock_irqrestore(mux->lock, flags);

		mux->current_rate = parent_rate/(divider+1);
	}

	if ((mux->max_rate) && (mux->current_rate > mux->max_rate) && nvt_composite_mux_clk_is_enabled(hw) && log_index_cmux) {
		pr_err("!!!! %s: %s current_freq(%ld) exceed max_freq(%ld) !!!!\n",
			__func__, mux->name, mux->current_rate, mux->max_rate);
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

	if ((mux->max_rate) && (mux->current_rate > mux->max_rate)) {
		pr_err("!!!! %s: %s current_freq(%ld) exceed max_freq(%ld), return !!!!\n",
			__func__, mux->name, mux->current_rate, mux->max_rate);
		return -1;
	}

	divider = (parent_rate / rate);
	if (!divider)
		divider = 1;

	if (mux->div_reg_offset == SIEMCLK_COMM_CLK_DIV_REG0_OFFSET) {
		unsigned long offset = mux->div_reg_offset & SIEMCLK_COMM_MASK;

		spin_lock_irqsave(mux->lock, flags);
		reg_val = CG_GETREG(offset);
		reg_val &= ~(div_mask(mux->div_bit_width) << mux->div_bit_idx);
		reg_val &= ~(div_mask(mux->div_bit_width) <<
				(mux->div_bit_idx + SIEMCLK_COMM_DIV_SHIFT));
		CG_SETREG(offset, reg_val);
		udelay(2);
		reg_val |= ((divider - 1) << mux->div_bit_idx);
		reg_val |= ((divider - 1) << 
				(mux->div_bit_idx + SIEMCLK_COMM_DIV_SHIFT));
		CG_SETREG(offset, reg_val);
		spin_unlock_irqrestore(mux->lock, flags);

		return 0;
	}


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

	if (mux->mux_reg_offset == SIEMCLK_COMM_CLK_RATE_REG0_OFFSET) {
		unsigned long offset = mux->mux_reg_offset & SIEMCLK_COMM_MASK;

		spin_lock_irqsave(mux->lock, flags);
		reg_val = CG_GETREG(offset);
		reg_val &= ~(div_mask(mux->mux_bit_width) << mux->mux_bit_idx);
		reg_val &= ~(div_mask(mux->mux_bit_width) << 
				(mux->mux_bit_idx + SIEMCLK_COMM_RATE_SHIFT));
		reg_val |= (index << mux->mux_bit_idx);
		reg_val |= (index << (mux->mux_bit_idx + SIEMCLK_COMM_RATE_SHIFT));
		CG_SETREG(offset, reg_val);
		spin_unlock_irqrestore(mux->lock, flags);

		return 0;
	}

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

	if (mux->mux_reg_offset == SIEMCLK_COMM_CLK_RATE_REG0_OFFSET) {
		unsigned long offset = mux->mux_reg_offset & SIEMCLK_COMM_MASK;

		reg_val = CG_GETREG(offset);
		index = reg_val & (div_mask(mux->mux_bit_width) << mux->mux_bit_idx);

		return index >> mux->mux_bit_idx;
	}

	reg_val = CG_GETREG(mux->mux_reg_offset);
	index = reg_val & (div_mask(mux->mux_bit_width) << mux->mux_bit_idx);

	return index >> mux->mux_bit_idx;
}

static int nvt_composite_mux_clk_set_phase(struct clk_hw *hw, int enable)
{
	unsigned int reg_val;
	unsigned long flags;
	struct clk_mux *cmux = container_of(hw, struct clk_mux, hw);
	struct nvt_composite_mux_clk *mux =
	    container_of(cmux, struct nvt_composite_mux_clk, mux);

	if (mux->autogating_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(mux->lock, flags);
		reg_val = CG_GETREG(mux->autogating_reg_offset);

		if (enable == 0)
			reg_val &= ~(0x1 << mux->autogating_bit_idx);
		else
			reg_val |= (0x1 << mux->autogating_bit_idx);

		CG_SETREG(mux->autogating_reg_offset, reg_val);
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(mux->lock, flags);
	}

	return 0;
}

static int nvt_composite_mux_clk_get_phase(struct clk_hw *hw)
{
	unsigned long flags;
	int is_enabled = 0;
	struct clk_mux *cmux = container_of(hw, struct clk_mux, hw);
	struct nvt_composite_mux_clk *mux =
	    container_of(cmux, struct nvt_composite_mux_clk, mux);

	if (mux->autogating_reg_offset != 0) {
		/* race condition protect. enter critical section */
		spin_lock_irqsave(mux->lock, flags);
		is_enabled =
		    (CG_GETREG(mux->autogating_reg_offset) & \
			(0x1 << mux->autogating_bit_idx)) ? 1 : 0;
		/* race condition protect. leave critical section */
		spin_unlock_irqrestore(mux->lock, flags);
	}

	return is_enabled;
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
	.set_phase = nvt_composite_mux_clk_set_phase,
	.get_phase = nvt_composite_mux_clk_get_phase,
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
		p_cmux_clk =
		    kzalloc(sizeof(struct nvt_composite_mux_clk), GFP_KERNEL);
		if (!p_cmux_clk) {
			pr_err("%s: failed to alloc cmux_clk\n", __func__);
			return -EPERM;
		}

		memcpy(p_cmux_clk->name, composite_mux_clks[i].name,
		       CLK_NAME_STR_SIZE);
		p_cmux_clk->mux_reg_offset =
		    composite_mux_clks[i].mux_reg_offset;
		p_cmux_clk->mux_bit_idx = composite_mux_clks[i].mux_bit_idx;
		p_cmux_clk->mux_bit_width = composite_mux_clks[i].mux_bit_width;
		p_cmux_clk->div_reg_offset =
		    composite_mux_clks[i].div_reg_offset;
		p_cmux_clk->div_bit_idx = composite_mux_clks[i].div_bit_idx;
		p_cmux_clk->div_bit_width = composite_mux_clks[i].div_bit_width;
		p_cmux_clk->gate_reg_offset =
		    composite_mux_clks[i].gate_reg_offset;
		p_cmux_clk->gate_bit_idx = composite_mux_clks[i].gate_bit_idx;
		p_cmux_clk->do_enable = composite_mux_clks[i].do_enable;
		p_cmux_clk->reset_reg_offset =
		    composite_mux_clks[i].reset_reg_offset;
		p_cmux_clk->reset_bit_idx = composite_mux_clks[i].reset_bit_idx;
		p_cmux_clk->do_reset = composite_mux_clks[i].do_reset;
		p_cmux_clk->autogating_reg_offset =
		    composite_mux_clks[i].autogating_reg_offset;
		p_cmux_clk->autogating_bit_idx = composite_mux_clks[i].autogating_bit_idx;
		p_cmux_clk->do_autogating = composite_mux_clks[i].do_autogating;
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
			pr_err("%s: failed to register clk %s\n",
			       __func__, p_cmux_clk->name);
			ret = -EPERM;
			goto err;
		}
		if (clk_register_clkdev(clk, p_cmux_clk->name, NULL)) {
			pr_err("%s: failed to register lookup %s\n",
			       __func__, p_cmux_clk->name);
			ret = -EPERM;
			goto err;
		} else {
			if (composite_mux_clks[i].do_enable) {
				ret = clk_prepare_enable(clk);
				if (ret < 0)
					pr_err("%s prepare & enable failed!\n",
					       p_cmux_clk->name);
			}

			ret = clk_set_phase(clk, composite_mux_clks[i].do_autogating);
			if (ret < 0)
				pr_err("%s enable autogating failed!\n",
				       p_cmux_clk->name);

			if (__clk_is_enabled(clk) && fastboot) {
				ret = clk_prepare_enable(clk);
				if (ret < 0)
					pr_err("%s prepare & enable failed!\n",
						p_cmux_clk->name);
			}
		}

		if (logoboot && ((strncmp(p_cmux_clk->name, "f0800000.ide", 16) == 0) || (strncmp(p_cmux_clk->name, "f0800000.ideif", 16) == 0))) {
			p_cmux_clk->parent_rate = clk_get_rate(clk_get_parent(clk));
			p_cmux_clk->current_rate = clk_get_rate(clk);
		} else {
			if (fastboot == 0) {
				clk_set_parent(clk,
					__clk_lookup(composite_mux_clks[i].parent_names
						[composite_mux_clks
						[i].parent_idx]));
			}

			p_cmux_clk->parent_rate = clk_get_rate(clk_get_parent(clk));

			if ((composite_mux_clks[i].div_bit_width) && (fastboot == 0) && !composite_mux_clks[i].keep_rate)
				clk_set_rate(clk, composite_mux_clks[i].current_rate);

			p_cmux_clk->current_rate = clk_get_rate(clk);
		}
	}

	log_index_cmux = 1;
err:
	return ret;
}
