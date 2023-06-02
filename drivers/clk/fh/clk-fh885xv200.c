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
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/bitops.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <mach/pmu.h>

static DEFINE_SPINLOCK(clk_lock);

/* Maximum number of parents our clocks have */
#define FH_MAX_PARENTS	4

#define PROC_FILE   "driver/clock"

#define to_clk_factors(_hw) container_of(_hw, struct clk_factors, hw)


#define SETMASK(len, pos)       (((1U << (len)) - 1) << (pos))
#define CLRMASK(len, pos)       (~(SETMASK(len, pos)))

#define FACTOR_GET(bit, len, reg)   (((reg) & SETMASK(len, bit)) >> (bit))
#define FACTOR_SET(bit, len, reg, val) \
	    (((reg) & CLRMASK(len, bit)) | (val << (bit)))

#define CONFIG_ISP_CLK_RATE   240000000
#define CONFIG_JPEG_CLK_RATE  200000000
#define CONFIG_VEU_CLK_RATE   300000000

/* init clk base addr */
void __iomem *base_addr;

/* Matches for pll clocks */
struct clk_factors {
	struct clk_hw hw;
	void __iomem *reg0;
	void __iomem *reg1;
	u32 m;
	u32 n;
	u32 pr;
	u32 divcop;
	spinlock_t *lock;
};

struct fh_clk_divider {
	struct clk_hw	hw;
	void __iomem	*reg;
	u32           prediv;
	u32         div_flag;
	u32     div_reg_mask;
	spinlock_t	   *lock;
};

struct fh_clk_phase {
	struct clk_hw	  hw;
	void __iomem	*reg;
	u32             mux;
	spinlock_t	   *lock;
};

struct fh_clk_config {
	char	name[32];
	u32	    rate;
};



static struct of_device_id clk_fix_match[] __initdata = {
	{.compatible = "fixed-clock", .data = NULL,},
	{}
};

/* Matches for pll clocks */
static struct of_device_id clk_factors_match[]  = {
	{.compatible = "fh pll-ddr-rclk", .data = NULL,},
	{.compatible = "fh pll-cpu-pclk", .data = NULL,},
	{.compatible = "fh pll-cpu-rclk", .data = NULL,},
	{.compatible = "fh pll-sys-pclk", .data = NULL,},
	{.compatible = "fh pll-sys-rclk", .data = NULL,},
	{}
};

/* Matches for divided outputs */
static struct of_device_id clk_divs_match[]  = {
	{.compatible = "fh fh-arm-clk", .data = NULL,},
	{.compatible = "fh fh-arc-clk", .data = NULL,},
	{.compatible = "fh fh-ahb-clk", .data = NULL,},
	{.compatible = "fh sysp-div12-clk", .data = NULL,},
	{.compatible = "fh fh-ddr-clk", .data = NULL,},
	{.compatible = "fh fh-ispa-clk", .data = NULL,},
	{.compatible = "fh fh-ispb-clk", .data = NULL,},
	{.compatible = "fh fh-vpu-clk", .data = NULL,},
	{.compatible = "fh fh-pix-clk", .data = NULL,},
	{.compatible = "fh fh-jpeg-clk", .data = NULL,},
	{.compatible = "fh fh-bgm-clk", .data = NULL,},
	{.compatible = "fh fh-jpeg-adapt-clk", .data = NULL,},
	{.compatible = "fh fh-sdc0-clk", .data = NULL,},
	{.compatible = "fh fh-sdc1-clk", .data = NULL,},
	{.compatible = "fh fh-spi0-clk", .data = NULL,},
	{.compatible = "fh fh-spi1-clk", .data = NULL,},
	{.compatible = "fh fh-spi2-clk", .data = NULL,},
	{.compatible = "fh fh-veu-clk", .data = NULL,},
	{.compatible = "fh fh-veu-adapt-clk", .data = NULL,},
	{.compatible = "fh fh-cis-clk-out", .data = NULL,},
	{.compatible = "fh fh-ac-mclk", .data = NULL,},
	{.compatible = "fh fh-usb-clk", .data = NULL,},
	{.compatible = "fh fh-eth-clk", .data = NULL,},
	{.compatible = "fh fh-ethrmii-clk", .data = NULL,},
	{.compatible = "fh fh-uart0-clk", .data = NULL,},
	{.compatible = "fh fh-uart1-clk", .data = NULL,},
	{.compatible = "fh fh-uart2-clk", .data = NULL,},
	{.compatible = "fh fh-i2c0-clk", .data = NULL,},
	{.compatible = "fh fh-i2c1-clk", .data = NULL,},
	{.compatible = "fh fh-i2c2-clk", .data = NULL,},
	{.compatible = "fh fh-pwm-clk", .data = NULL,},
	{.compatible = "fh fh-pts-clk", .data = NULL,},
	{.compatible = "fh fh-efuse-clk", .data = NULL,},
	{.compatible = "fh fh-tmr0-clk", .data = NULL,},
	{.compatible = "fh fh-ac-clk", .data = NULL,},
	{.compatible = "fh fh-i2s-clk", .data = NULL,},
	{.compatible = "fh fh-sadc-clk", .data = NULL,},
	{.compatible = "fh fh-wdt-clk", .data = NULL,},
	{.compatible = "fh fh-gpio0-dbclk", .data = NULL,},
	{.compatible = "fh fh-gpio1-dbclk", .data = NULL,},
	{.compatible = "fh fh-emc-hclk", .data = NULL,},
	{.compatible = "fh fh-rtc-pclk", .data = NULL,},
	{.compatible = "fh fh-aes-hclk", .data = NULL,},
	{.compatible = "fh fh-mipi-dphy-clk", .data = NULL,},
	{.compatible = "fh fh-mipi-wrap-clk", .data = NULL,},
	{.compatible = "fh fh-rtc-hclk", .data = NULL,},
	{.compatible = "fh fh-emac-hclk", .data = NULL,},
	{.compatible = "fh fh-usb-clk", .data = NULL,},
	{.compatible = "fh fh-aes-hclk", .data = NULL,},
	{.compatible = "fh fh-ephy-clk", .data = NULL,},
	{.compatible = "fh fh-sdc08x-clk", .data = NULL,},
	{.compatible = "fh fh-sdc18x-clk", .data = NULL,},
	{.compatible = "fh fh-mipic-pclk", .data = NULL,},
	{.compatible = "fh fh-gpio0-pclk", .data = NULL,},
	{.compatible = "fh fh-gpio1-pclk", .data = NULL,},
	{.compatible = "fh fh-isp-hclk", .data = NULL,},
	{.compatible = "fh fh-veu-hclk", .data = NULL,},
	{.compatible = "fh fh-bgm-hclk", .data = NULL,},
	{.compatible = "fh fh-adapt-hclk", .data = NULL,},
	{.compatible = "fh fh-jpg-hclk", .data = NULL,},
	{.compatible = "fh fh-jpg-adapt-clk", .data = NULL,},
	{.compatible = "fh fh-vpu-hclk", .data = NULL,},
	{}
};

/* Matches for phase clocks */

static struct of_device_id clk_phase_match[] __initdata = {
	{.compatible = "fh fh-sdc0-clk_sample", .data = NULL,},
	{.compatible = "fh fh-sdc0-clk_drv", .data = NULL,},
	{.compatible = "fh fh-sdc1-clk_sample", .data = NULL,},
	{.compatible = "fh fh-sdc1-clk_drv", .data = NULL,},
	{}
};

/* init clocks base*/
static struct of_device_id clk_init_match[] __initdata = {
	{.compatible = "fh,fh-clk", .data = NULL,},
	{}
};


void __init fh_fixed_clk_setup(struct device_node *node,
						void *name)
{
	struct clk *clk;
	u32 out_values;
	const char *clk_name;

	of_property_read_string(node, "clock-output-names", &clk_name);
	strcpy(name, clk_name);
	of_property_read_u32_array(node,
			       "clock-frequency", &out_values,
			       1);
	clk = clk_register_fixed_rate(NULL, clk_name,
				      NULL,
				      CLK_IS_ROOT,
				      out_values);
	if (!IS_ERR(clk)) {
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
		clk_register_clkdev(clk, clk_name, NULL);
	}
}
#define   DIVVCO_ONE_DEVISION          0x0
#define   DIVVCO_TWO_DEVISION          0x8
#define   DIVVCO_FOUR_DEVISION         0xc
#define   DIVVCO_EIGHT_DEVISION        0xd
#define   DIVVCO_SIXTEEN_DEVISION      0xe
#define   DIVVCO_THIRTYTWO_DEVISION    0xf

static unsigned long clk_pll_recalc_rate(struct clk_hw *hw,
						unsigned long parent_rate)
{
	unsigned int m = 0, n = 0, pr = 1;
	unsigned int clk_vco, divvcop = 1, shift;
	u32 reg;
	unsigned long rate;
	struct clk_factors *factors = to_clk_factors(hw);
	unsigned int divcop = 0;
	/* Fetch the register value */
	reg = readl(factors->reg0);
	shift = ffs(factors->m)-1;
	m = (reg&factors->m) >> shift;

	shift = ffs(factors->n)-1;
	n = (reg&factors->n) >> shift;

	/*pll databook*/
	if (m < 4)
	    m = 128+m;

	if (m == 0xb)
	    m = 0xa;

	shift = ffs(factors->pr)-1;
	pr = (reg&factors->pr) >> shift;

	reg = readl(factors->reg1);
    shift = ffs(factors->divcop)-1;
	divcop = ((reg&factors->divcop) >> shift);


	/* Calculate the rate */
	switch (divcop){
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
		printk("divvcop error:%x\n",divvcop);
	}

	clk_vco = parent_rate * m / (n+1);
	rate = clk_vco/ (pr+1)/divvcop;
	return rate;

}

static long clk_pll_round_rate(struct clk_hw *hw, unsigned long rate,
					unsigned long *parent_rate)
	{
		unsigned int m = 0, n = 0, pr = 1;
		unsigned int clk_vco, divvcop = 1, shift;
		u32 reg;
		struct clk_factors *factors = to_clk_factors(hw);
		unsigned int divcop = 0;

		/* Fetch the register value */
		reg = readl(factors->reg0);

		shift = ffs(factors->m)-1;
		m = (reg&factors->m) >> shift;

		shift = ffs(factors->n)-1;
		n = (reg&factors->n) >> shift;


		/*pll databook*/
		if(m < 4)
			m = 128+m;

		if(m == 0xb)
			m = 0xa;

		shift = ffs(factors->pr)-1;
		pr = (reg&factors->pr) >> shift;

		reg = readl(factors->reg1);
		shift = ffs(factors->divcop)-1;
		divcop = (reg&factors->divcop) >> shift;


		/* Calculate the rate */
		switch (divcop){
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
			printk("divvcop error:%x\n",divvcop);
		}

		clk_vco = (*parent_rate) * m / (n+1);
		rate = clk_vco / (pr+1) / divvcop;
		return rate;

	}


static const struct clk_ops clk_pll_ops = {
	.recalc_rate = clk_pll_recalc_rate,
	.round_rate = clk_pll_round_rate,
};

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
		reg = readl(factors->reg);

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
		reg = readl(factors->reg);

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
		reg = readl(factors->reg);

		divide = parent_rate/rate/factors->prediv - 1;
		/*printk("fh_clk_divide_set_rate:%x\n",divide);*/

		shift = ffs(factors->div_reg_mask)-1;


		reg &= ~(factors->div_reg_mask);
		reg |= ((divide << shift) & factors->div_reg_mask);

		/* Apply them now */
		clk_writel(reg, factors->reg);

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

static int  __init fh_divs_clk_setup(struct device_node *node,
			void *name)
{
	struct  clk *clk;
	struct  clk_gate *gate = NULL;
	struct  clk_hw   *clk_mux_hw = NULL;
	struct  clk_mux  *mux = NULL;
	struct  clk_hw *clk_gate_hw = NULL;
	struct  clk_hw *clk_divider_hw = NULL;
	struct  fh_clk_divider *fh_divider = NULL;
	const char *clk_name;
	const char *parents[FH_MAX_PARENTS];
	void __iomem *reg = NULL;
	int i = 0;
	u32 div_mask = 0;
	u32 gate_mask = 0;
	u32 mux_mask = 0;
	u32 prediv = 0;

	if (!base_addr)
		return 0;

	/* if we have a mux, we will have >1 parents */
	while (i < FH_MAX_PARENTS &&
			(parents[i] = of_clk_get_parent_name(node, i)) != NULL)
		i++;

	fh_divider = kzalloc(sizeof(*fh_divider), GFP_KERNEL);
	if (!fh_divider)
		return 0;

	fh_divider->lock = &clk_lock;
	of_property_read_string(node, "clock-output-names", &clk_name);
	strcpy(name, clk_name);

	if(!of_property_read_u32(node, "prediv", &prediv)) {
		fh_divider->prediv = prediv;
		clk_divider_hw = &fh_divider->hw;
		} else
		fh_divider->prediv = 1;


	/* Leaves can be fixed or configurable divisors */
	if (!of_property_read_u32(node, "div", &div_mask)) {
		reg = of_iomap(node, 0);
		clk_divider_hw = &fh_divider->hw;
		fh_divider->reg = reg;
		fh_divider->div_flag = 1;
	    fh_divider->div_reg_mask= div_mask;
	}

    /* Add a gate if this factor clock can be gated */
	if (!of_property_read_u32(node, "gate", &gate_mask)) {
		gate = kzalloc(sizeof(struct clk_gate), GFP_KERNEL);
		if (!gate) {
			kfree(fh_divider);
			return 0;
		}

		/* set up gate properties */
		reg = of_iomap(node, 1);
		gate->reg = reg;
		gate->bit_idx =ffs(gate_mask)-1;
		gate->lock = &clk_lock;
		gate->flags = CLK_GATE_SET_TO_DISABLE;
		clk_gate_hw = &gate->hw;
	}

	if (!of_property_read_u32(node, "mux", &mux_mask)) {
		mux = kzalloc(sizeof(struct clk_mux), GFP_KERNEL);


		/* set up gate properties */
		reg = of_iomap(node, 2);
		mux->reg = reg;
		mux->shift = ffs(mux_mask)-1;
		mux->mask = mux_mask>>mux->shift;
		mux->lock = &clk_lock;
		clk_mux_hw = &mux->hw;
	}
	#if 1
	clk = clk_register_composite(NULL, clk_name,
			parents, i,
			clk_mux_hw, &clk_mux_ops,
			clk_divider_hw, &fh_clk_divider_ops,
			clk_gate_hw, &clk_gate_ops, CLK_IGNORE_UNUSED);
	if (!IS_ERR(clk)) {
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
		clk_register_clkdev(clk, clk_name, NULL);
	}
	#endif
	return 1;
}


static int __init fh_pll_clk_setup(struct device_node *node,
			void *name)
{
	struct clk *clk;
	struct clk_factors *factors;
	const char *clk_name = node->name;
	const char *parents[FH_MAX_PARENTS];
	void __iomem *reg0;
	void __iomem *reg1;
	int i = 0;
	u32 pr = 0;
	u32 divcop = 0;
	u32 m = 0;
	u32 n = 0;

	if (!base_addr)
		return 0;

	reg0 = of_iomap(node, 0);
	reg1 = of_iomap(node, 1);
	/* if we have a mux, we will have >1 parents */
	while (i < FH_MAX_PARENTS &&
			(parents[i] = of_clk_get_parent_name(node, i)) != NULL)
		i++;
	of_property_read_string(node, "clock-output-names", &clk_name);
	strcpy(name, clk_name);
	factors = kzalloc(sizeof(struct clk_factors), GFP_KERNEL);
	if (!factors)
		return 0;

	if (of_property_read_u32(node, "m", &m)) {
		pr_err("%s:get node attr m error\n", __func__);
		return -1;
	}
	factors->m = m;

	if (of_property_read_u32(node, "n", &n)) {
		pr_err("%s get node attr n error\n", __func__);
		return -1;
	}
	factors->n = n;

	if (of_property_read_u32(node, "pr", &pr)) {
		pr_err("%s get node attr pr error\n", __func__);
		return -1;
	}
	factors->pr = pr;

	if (of_property_read_u32(node, "divcop", &divcop)) {
		pr_err("%s get node attr dicop error\n", __func__);
		return -1;
	}
	factors->divcop = divcop;

	/* set up factors properties */
	factors->reg0 = reg0;
	factors->reg1 = reg1;
	factors->lock = &clk_lock;
	clk = clk_register_composite(NULL, clk_name,
			parents, i,
			NULL, NULL,
			&factors->hw, &clk_pll_ops,
			NULL, NULL, CLK_IGNORE_UNUSED);
	if (!IS_ERR(clk)) {
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
		clk_register_clkdev(clk, clk_name, NULL);
	}
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
		reg = readl(phase->reg);

		local_degree = degree;

		shift = ffs(phase->mux)-1;

		reg |= (local_degree << shift);

		/* Apply them now */
		writel(reg, phase->reg);
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
		reg = readl(phase->reg);
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

static int __init fh_phase_clk_setup(struct device_node *node,
						void*data)
{
	struct clk *clk;
	const char *clk_name = node->name;
	const char *parents[FH_MAX_PARENTS];
	void __iomem *reg;
	struct fh_clk_phase *clk_phase = NULL;
	int i = 0;
	struct clk_init_data *init = NULL;
	u32 mux_mask = 0;

	while (i < FH_MAX_PARENTS &&
			(parents[i] = of_clk_get_parent_name(node, i)) != NULL)
		i++;

	of_property_read_string(node, "clock-output-names", &clk_name);

	if(of_property_read_u32(node, "mux", &mux_mask)) {
		pr_err("%s get node attr mux error\n", __func__);
		return -1;
	}

	clk_phase = kzalloc(sizeof(struct fh_clk_phase), GFP_KERNEL);
	if (!clk_phase)
		return 0;
	clk_phase->mux = mux_mask;
	init = kzalloc(sizeof(struct clk_init_data), GFP_KERNEL);

	reg = of_iomap(node, 0);
	/* set up gate properties */
	clk_phase->reg = reg;
	clk_phase->lock = &clk_lock;
	init->ops = &fh_clk_phase_ops;
	init->parent_names = parents;
	init->num_parents = i;
	init->name = clk_name;
	clk_phase->hw.init = init;
	clk = clk_register(NULL, &clk_phase->hw);

	if (clk) {
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
		clk_register_clkdev(clk, clk_name, NULL);
	}

	return 1;
}


static void __init of_fh_table_clock_setup(const struct of_device_id *clk_match,
								void *function)
{
	struct device_node *np;
	const char *name;
	const struct of_device_id *match;
	int (*setup_function)(struct device_node *, const void *) = function;

	for_each_matching_node_and_match(np, clk_match, &match) {
		name = match->name;
		setup_function(np, name);
	}
}

static int __init fh_clk_setup(struct device_node *node,
				void *data)
{
	if (!base_addr)
		base_addr = of_iomap(node, 0);
	return 1;
}

static void __init fh_init_setup(struct fh_clk_config clocks[], int nclocks)
{
	int i = 0;
	/* init clocks base*/
	of_fh_table_clock_setup(clk_init_match, fh_clk_setup);

	/* Register fixed clocks */
	of_fh_table_clock_setup(clk_fix_match, fh_fixed_clk_setup);

	/* Register factor clocks */
	of_fh_table_clock_setup(clk_factors_match, fh_pll_clk_setup);

	/* Register divided output clocks */
	of_fh_table_clock_setup(clk_divs_match, fh_divs_clk_setup);

	/* Register phase clocks */
	of_fh_table_clock_setup(clk_phase_match, fh_phase_clk_setup);

	/* Protect the clocks that needs to stay on */
	for (i = 0; i < nclocks; i++) {
		struct clk *clk = clk_get(NULL, clocks[i].name);

		if (!IS_ERR(clk)) {
			clk_set_rate(clk,clocks[i].rate);
			clk_put(clk);
		}

	}
}
static void del_char(char *str, char ch)
{
	char *p = str;
	char *q = str;

	while (*q) {
		if (*q != ch)
			*p++ = *q;
		q++;
	}
	*p = '\0';
}

static ssize_t fh_clk_proc_write(struct file *filp, const char *buf,
	size_t len, loff_t *off)
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
			pr_err("%s: ERROR: parameter[%d] is empty\n",
				__func__, i);
			pr_err("[clk name], [enable/disable], [clk rate]\n");
			return -EINVAL;
		}
		del_char(param_str[i], ' ');
		del_char(param_str[i], '\n');
	}

	clk = clk_get(NULL, param_str[0]);
	if (!clk) {
		pr_err("%s: ERROR: clk %s is not found\n",
			__func__, param_str[0]);
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
		pr_err("clk %s enabled\n", param_str[0]);
	} else if (!strcmp(param_str[1], "disable")) {
		clk_disable_unprepare(clk);
		pr_err("clk %s disabled\n", param_str[0]);
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
	*pos = 0;
	return NULL;
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
	struct of_device_id *match;
	struct clk *clk = NULL;
	unsigned long rate;
	char gate[32];

	seq_printf(sfile, "\nPLL Information:\n");
	for (match = clk_factors_match; match->name[0]; match++) {
		clk = clk_get(NULL, match->name);
		rate = clk_get_rate(clk);

		if (__clk_is_enabled(clk))
			memcpy(gate, "enable", sizeof("enable"));
		else
			memcpy(gate, "disable", sizeof("disable"));
		seq_printf(sfile, "\t%-20s \t%9luHZ \t%-10s\n",
				match->name, rate, gate);
		clk_put(clk);
	}
	for (match = clk_divs_match; match->compatible[0]; match++) {
		if (!match->name[0])
			continue;
		clk = NULL;
		clk = clk_get(NULL, match->name);
		if (IS_ERR(clk))
			continue;
		rate = clk_get_rate(clk);

		if (__clk_is_enabled(clk))
			memcpy(gate, "enable", sizeof("enable"));
		else
			memcpy(gate, "disable", sizeof("disable"));
		seq_printf(sfile, "\t%-20s \t%9luHZ \t%-10s\n",
				match->name, rate, gate);
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

static const struct file_operations fh_clk_proc_ops = {
	.owner		= THIS_MODULE,
	.open		= fh_clk_proc_open,
	.read		= seq_read,
	.write		= fh_clk_proc_write,
	.release	= seq_release,
};

int  fh_clk_procfs_init(void)
{
	struct proc_dir_entry *proc_file;

	proc_file = proc_create(PROC_FILE, 0644, NULL, &fh_clk_proc_ops);
	if (!proc_file)
		pr_err("clock, create proc fs failed\n");

	return 0;
}

static struct fh_clk_config fh_init_rate_clocks[] __initconst = {
	{.name = "uart1_clk", .rate = 16666666,},
	{.name = "uart2_clk", .rate = 16666666,},
	{.name = "isp_aclk", .rate = CONFIG_ISP_CLK_RATE,},
	{.name = "veu_clk", .rate = CONFIG_VEU_CLK_RATE,},
	{.name = "jpeg_clk", .rate = CONFIG_JPEG_CLK_RATE,},
};

static void __init fh_init_clocks(struct device_node *node)
{
	fh_init_setup(fh_init_rate_clocks,
			ARRAY_SIZE(fh_init_rate_clocks));
}

CLK_OF_DECLARE(fh_clk_init, "fh,fh-clk", fh_init_clocks);
module_init(fh_clk_procfs_init);


