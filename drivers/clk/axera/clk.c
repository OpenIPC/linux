/*
 * Axera clock driver
 *
 * Copyright (c) 2019-2020 Axera Technology Co., Ltd.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/slab.h>

#include "clk-axera.h"
static DEFINE_SPINLOCK(axera_clk_lock);
struct axera_clock_data *axera_clk_alloc(struct platform_device *pdev, int nr_clks)
{
    struct axera_clock_data *clk_data;
    struct resource *res;
    struct clk **clk_table;
    clk_data = devm_kmalloc(&pdev->dev, sizeof(*clk_data), GFP_KERNEL);
    if (!clk_data)
        return NULL;
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res)
        return NULL;
    clk_data->base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
    if (!clk_data->base)
        return NULL;
    clk_table = devm_kmalloc_array(&pdev->dev, nr_clks, sizeof(*clk_table), GFP_KERNEL);
    if (!clk_table)
        return NULL;
    clk_data->clk_data.clks = clk_table;
    clk_data->clk_data.clk_num = nr_clks;
    return clk_data;
}

EXPORT_SYMBOL_GPL(axera_clk_alloc);
struct axera_clock_data *axera_clk_init(struct device_node *np,
                                        int nr_clks)
{
    struct axera_clock_data *clk_data;
    struct clk **clk_table;
    void __iomem *base;
    base = of_iomap(np, 0);
    if (!base) {
        pr_err("%s: failed to map clock registers\n", __func__);
        goto err;
    }
    clk_data = kzalloc(sizeof(*clk_data), GFP_KERNEL);
    if (!clk_data)
        goto err;
    clk_data->base = base;
    clk_table = kcalloc(nr_clks, sizeof(*clk_table), GFP_KERNEL);
    if (!clk_table)
        goto err_data;
    clk_data->clk_data.clks = clk_table;
    clk_data->clk_data.clk_num = nr_clks;
    of_clk_add_provider(np, of_clk_src_onecell_get, &clk_data->clk_data);
    return clk_data;
err_data:
    kfree(clk_data);
err:
    return NULL;
}

EXPORT_SYMBOL_GPL(axera_clk_init);
int axera_clk_register_fixed_rate(const struct axera_fixed_rate_clock
                                  *clks, int nums,
                                  struct axera_clock_data *data)
{
    struct clk *clk;
    int i;
    for (i = 0; i < nums; i++) {
        clk = clk_register_fixed_rate(NULL, clks[i].name,
                                      clks[i].parent_name,
                                      clks[i].flags,
                                      clks[i].fixed_rate);
        if (IS_ERR(clk)) {
            pr_err("%s: failed to register clock %s\n", __func__,
                   clks[i].name);
            goto err;
        }
        data->clk_data.clks[clks[i].id] = clk;
    }
    return 0;
err:
    while (i--)
        clk_unregister_fixed_rate(data->clk_data.clks[clks[i].id]);
    return PTR_ERR(clk);
}

EXPORT_SYMBOL_GPL(axera_clk_register_fixed_rate);
int axera_clk_register_fixed_factor(const struct axera_fixed_factor_clock
                                    *clks, int nums,
                                    struct axera_clock_data *data)
{
    struct clk *clk;
    int i;
    for (i = 0; i < nums; i++) {
        clk = clk_register_fixed_factor(NULL, clks[i].name,
                                        clks[i].parent_name,
                                        clks[i].flags, clks[i].mult,
                                        clks[i].div);
        if (IS_ERR(clk)) {
            pr_err("%s: failed to register clock %s\n", __func__,
                   clks[i].name);
            goto err;
        }
        data->clk_data.clks[clks[i].id] = clk;
    }
    return 0;
err:
    while (i--)
        clk_unregister_fixed_factor(data->clk_data.clks[clks[i].id]);
    return PTR_ERR(clk);
}

EXPORT_SYMBOL_GPL(axera_clk_register_fixed_factor);
int axera_clk_register_mux(const struct axera_mux_clock *clks, int nums,
                           struct axera_clock_data *data)
{
    struct clk *clk;
    void __iomem *base = data->base;
    int i;
    for (i = 0; i < nums; i++) {
        u32 mask = BIT(clks[i].width) - 1;
        clk =
            clk_register_mux_table(NULL, clks[i].name,
                                   clks[i].parent_names,
                                   clks[i].num_parents, clks[i].flags,
                                   base + clks[i].offset,
                                   clks[i].shift, mask,
                                   clks[i].mux_flags, clks[i].table,
                                   &axera_clk_lock);
        if (IS_ERR(clk)) {
            pr_err("%s: failed to register clock %s\n", __func__,
                   clks[i].name);
            goto err;
        }
        if (clks[i].alias)
            clk_register_clkdev(clk, clks[i].alias, NULL);
        data->clk_data.clks[clks[i].id] = clk;
    }
    return 0;
err:
    while (i--)
        clk_unregister_mux(data->clk_data.clks[clks[i].id]);
    return PTR_ERR(clk);
}

EXPORT_SYMBOL_GPL(axera_clk_register_mux);
#define div_mask(width) ((1 << (width)) - 1)


struct axera_clk_divider {
    struct clk_hw   hw;
    void __iomem    *reg;
    void __iomem    *reg_update;
    u8      update;
    u8      shift;
    u8      width;
    u32     mask;
    const struct clk_div_table *table;
    spinlock_t  *lock;
};

#define to_axera_clk_divider(_hw)   \
    container_of(_hw, struct axera_clk_divider, hw)

static unsigned long axera_clkdiv_recalc_rate(struct clk_hw *hw,
        unsigned long parent_rate)
{
    unsigned int val;
    struct axera_clk_divider *dclk = to_axera_clk_divider(hw);

    val = readl_relaxed(dclk->reg) >> dclk->shift;
    val &= div_mask(dclk->width);

    return divider_recalc_rate(hw, parent_rate, val, dclk->table,
                               CLK_DIVIDER_ROUND_CLOSEST, dclk->width);
}

static long axera_clkdiv_round_rate(struct clk_hw *hw, unsigned long rate,
                                    unsigned long *prate)
{
    struct axera_clk_divider *dclk = to_axera_clk_divider(hw);

    return divider_round_rate(hw, rate, prate, dclk->table,
                              dclk->width, CLK_DIVIDER_ROUND_CLOSEST);
}

static int axera_clkdiv_set_rate(struct clk_hw *hw, unsigned long rate,
                                 unsigned long parent_rate)
{
    int value;
    u32 data;
    struct axera_clk_divider *dclk = to_axera_clk_divider(hw);

    value = divider_get_val(rate, parent_rate, dclk->table,
                            dclk->width, CLK_DIVIDER_ROUND_CLOSEST);

    pr_debug("axera_clkdiv_set_rate, rate: %ld, parent_rate: %ld, value: %d", rate, parent_rate, value);
    if (dclk->lock)
        spin_lock(dclk->lock);

    data = readl_relaxed(dclk->reg);
    data &= ~(div_mask(dclk->width) << dclk->shift);
    data |= value << dclk->shift;
    writel_relaxed(data, dclk->reg);
    data = readl_relaxed(dclk->reg_update);
    data |= 1 << dclk->update;
    writel_relaxed(data, dclk->reg_update);
    mdelay(1);
    data &= ~(1 << dclk->update);
    writel_relaxed(data, dclk->reg_update);

    if (dclk->lock)
        spin_unlock(dclk->lock);

    return 0;
}

static const struct clk_ops axera_clkdiv_ops = {
    .recalc_rate = axera_clkdiv_recalc_rate,
    .round_rate = axera_clkdiv_round_rate,
    .set_rate = axera_clkdiv_set_rate,
};

struct clk *axera_register_clkdiv(struct device *dev, const char *name,
                                  const char *parent_name, unsigned long flags,
                                  void __iomem *reg, void __iomem *reg_update,
                                  u8 shift, u8 width, u32 update, spinlock_t *lock)
{
    struct axera_clk_divider *div;
    struct clk *clk;
    struct clk_init_data init;
    struct clk_div_table *table;
    u32 max_div, min_div;
    int i;

    /* allocate the divider */
    div = kzalloc(sizeof(*div), GFP_KERNEL);
    if (!div)
        return ERR_PTR(-ENOMEM);

    /* Init the divider table */
    max_div = div_mask(width) + 1;
    min_div = 1;

    table = kcalloc(max_div + 1, sizeof(*table), GFP_KERNEL);
    if (!table) {
        kfree(div);
        return ERR_PTR(-ENOMEM);
    }
    table[0].div = 1;
    table[0].val = 0;
    for (i = 1; i < max_div; i++) {
        table[i].div = i * 2;
        table[i].val = i;
    }

    init.name = name;
    init.ops = &axera_clkdiv_ops;
    init.flags = flags;
    init.parent_names = parent_name ? &parent_name : NULL;
    init.num_parents = parent_name ? 1 : 0;

    div->reg = reg;
    div->reg_update = reg_update;
    div->shift = shift;
    div->width = width;
    div->lock = lock;
    div->hw.init = &init;
    div->table = table;
    div->update = update;

    /* register the clock */
    clk = clk_register(dev, &div->hw);
    if (IS_ERR(clk)) {
        kfree(table);
        kfree(div);
    }

    return clk;
}
int axera_clk_register_divider(const struct axera_divider_clock *clks,
                               int nums, struct axera_clock_data *data)
{
    struct clk *clk;
    void __iomem *base = data->base;
    int i;

    for (i = 0; i < nums; i++) {
        clk = axera_register_clkdiv(NULL, clks[i].name,
                                    clks[i].parent_name,
                                    clks[i].flags,
                                    base + clks[i].offset,
                                    base + clks[i].update_offset,
                                    clks[i].shift,
                                    clks[i].width,
                                    clks[i].update,
                                    &axera_clk_lock);
        if (IS_ERR(clk)) {
            pr_err("%s: failed to register clock %s\n",
                   __func__, clks[i].name);
            goto err;
        }

        if (clks[i].alias)
            clk_register_clkdev(clk, clks[i].alias, NULL);

        data->clk_data.clks[clks[i].id] = clk;
    }
    return 0;
err:
    while (i--)
        clk_unregister_divider(data->clk_data.clks[clks[i].id]);
    return PTR_ERR(clk);

}

EXPORT_SYMBOL_GPL(axera_clk_register_divider);
static int axera_clkgate_enable(struct clk_hw *hw)
{
    struct clkgate_separated *sclk;
    unsigned long flags = 0;
    u32 reg;
    sclk = container_of(hw, struct clkgate_separated, hw);
    pr_debug("axera_clkgate_enable, %s, %lx, %d, %lx, %d, %lx, %d\n",
             sclk->name, (unsigned long)sclk->enable, sclk->bit_idx,
             (unsigned long)sclk->set, sclk->set_bit,
             (unsigned long)sclk->clr, sclk->clr_bit);
    if (sclk->flags == 1) {
        if (sclk->lock)
            spin_lock_irqsave(sclk->lock, flags);
        reg = readl_relaxed(sclk->enable);
        reg |= BIT(sclk->bit_idx);
        writel_relaxed(reg, sclk->enable);
        if (sclk->lock)
            spin_unlock_irqrestore(sclk->lock, flags);
    } else if (sclk->flags == 2) {
        reg = BIT(sclk->clr_bit);
        writel_relaxed(reg, sclk->clr);
    } else {
        reg = BIT(sclk->set_bit);
        writel_relaxed(reg, sclk->set);
    }
    pr_debug("axera_clkgate_enable %x\n", readl_relaxed(sclk->enable));
    return 0;
}

static void axera_clkgate_disable(struct clk_hw *hw)
{
    struct clkgate_separated *sclk;
    unsigned long flags = 0;
    u32 reg;
    sclk = container_of(hw, struct clkgate_separated, hw);
    pr_debug("axera_clkgate_disable, %s, %lx, %d, %lx, %d, %lx, %d\n",
             sclk->name, (unsigned long)sclk->enable, sclk->bit_idx,
             (unsigned long)sclk->set, sclk->set_bit,
             (unsigned long)sclk->clr, sclk->clr_bit);
    if (sclk->flags == 1) {
        if (sclk->lock)
            spin_lock_irqsave(sclk->lock, flags);
        reg = readl_relaxed(sclk->enable);
        reg &= ~BIT(sclk->bit_idx);
        writel_relaxed(reg, sclk->enable);
        if (sclk->lock)
            spin_unlock_irqrestore(sclk->lock, flags);
    } else if (sclk->flags == 2) {
        reg = BIT(sclk->set_bit);
        writel_relaxed(reg, sclk->set);
    } else {
        reg = BIT(sclk->clr_bit);
        writel_relaxed(reg, sclk->clr);
    }
    reg = readl_relaxed(sclk->enable);
    pr_debug("axera_clkgate_disable %x\n", readl_relaxed(sclk->enable));
}

static int axera_clkgate_is_enabled(struct clk_hw *hw)
{
    struct clkgate_separated *sclk;
    u32 reg;
    sclk = container_of(hw, struct clkgate_separated, hw);
    pr_debug("axera_clkgate_is_enabled, %s, %lx, %d, %lx, %d, %lx, %d\n",
             sclk->name, (unsigned long)sclk->enable, sclk->bit_idx,
             (unsigned long)sclk->set, sclk->set_bit,
             (unsigned long)sclk->clr, sclk->clr_bit);
    reg = readl_relaxed(sclk->enable);
    reg &= BIT(sclk->bit_idx);
    if (sclk->flags == 2)
        reg = !reg;
    pr_debug("axera_clkgate_is_enabled %x\n", readl_relaxed(sclk->enable));
    return reg ? 1 : 0;
}

static const struct clk_ops clkgate_separated_ops = {
    .enable = axera_clkgate_enable,
    .disable = axera_clkgate_disable,
    .is_enabled = axera_clkgate_is_enabled,
};
#define CLK_IS_BASIC_A 0 //..
struct clk *axera_register_clkgate(struct device *dev, const char *name,
                                   const char *parent_name,
                                   unsigned long flags,
                                   void __iomem *reg, u8 bit_idx,
                                   u8 clk_gate_flags,
                                   void __iomem *set_reg, u8 set_bit,
                                   void __iomem *clr_reg, u8 clr_bit,
                                   spinlock_t *lock)
{
    struct clkgate_separated *sclk;
    struct clk *clk;
    struct clk_init_data init;
    sclk = kzalloc(sizeof(*sclk), GFP_KERNEL);
    if (!sclk)
        return ERR_PTR(-ENOMEM);
    init.name = name;
    init.ops = &clkgate_separated_ops;
    init.flags = flags | CLK_IS_BASIC_A;
    init.parent_names = (parent_name ? &parent_name : NULL);
    init.num_parents = (parent_name ? 1 : 0);
    sclk->enable = reg;
    sclk->bit_idx = bit_idx;
    sclk->flags = clk_gate_flags;
    sclk->set = set_reg;
    sclk->set_bit = set_bit;
    sclk->clr = clr_reg;
    sclk->clr_bit = clr_bit;
    sclk->hw.init = &init;
    sclk->lock = lock;
    sclk->name = name;
    pr_debug("axera_register_clkgate: %lx, %s\n", (unsigned long)sclk->enable, init.name);
    clk = clk_register(dev, &sclk->hw);
    if (IS_ERR(clk))
        kfree(sclk);
    return clk;
}

void axera_clk_register_gate(const struct axera_gate_clock *clks,
                             int nums, struct axera_clock_data *data)
{
    struct clk *clk;
    void __iomem *base = data->base;
    int i;
    for (i = 0; i < nums; i++) {
        clk = axera_register_clkgate(NULL, clks[i].name,
                                     clks[i].parent_name,
                                     clks[i].flags,
                                     base + clks[i].offset,
                                     clks[i].bit_idx,
                                     clks[i].gate_flags,
                                     base + clks[i].set_offset,
                                     clks[i].set_bit,
                                     base + clks[i].clr_offset,
                                     clks[i].clr_bit,
                                     &axera_clk_lock);
        if (IS_ERR(clk)) {
            pr_err("%s: failed to register clock %s\n", __func__, clks[i].name);
            continue;
        }
        if (clks[i].alias)
            clk_register_clkdev(clk, clks[i].alias, NULL);
        data->clk_data.clks[clks[i].id] = clk;
    }
}

EXPORT_SYMBOL_GPL(axera_clk_register_gate);

#define ON_BIT_OFF 0
#define LOCK_STS_BIT 0
#define FBK_INT_OFF 0
#define FBK_FRA_OFF 0x2
#define PRE_DIV_OFF 17
#define LDO_STB_X2_EN_OFF 14
#define POST_DIV_OFF 23


static u32 clk_power(u32 base, u32 exponent)
{
	int i;
    u32 result = 1;
    if (exponent >= 0)
    {
        for (i = 0; i < exponent; i++)
        {
            result *= base;
        }
    }
    return result;
}

static void pll_set(struct axera_pll_cfg *clk, u32 rate)
{
    u32 val;
    int i;
    u64 rate_u64;
    struct axera_pll_cfg *pll_cfg = clk;
    int try_count = 10000;

    u32 pre_div;
    u32 ldo_stb_x2_en;
    u32 fbk_int;
    //u64 fbk_fra;
    u32 post_div;
    pre_div = 0;
    ldo_stb_x2_en = 1;
    rate_u64 = rate;

    post_div = (readl_relaxed(pll_cfg->cfg1_reg) & GENMASK(24, 23)) >> 23;
    post_div = clk_power(2,post_div);
    fbk_int = rate / (pll_cfg->ref_rate/post_div);
/*
    fbk_fra = do_div(rate_u64,(pll_cfg->ref_rate));

    fbk_fra = fbk_fra * (1 << 24);
    do_div(fbk_fra,(pll_cfg->ref_rate));
*/
    spin_lock(&axera_clk_lock);
    /*re_open set to 0 */
    writel_relaxed(1 << pll_cfg->re_open_bit, pll_cfg->re_open_reg + 8);
    /*on set to 0 */
    writel_relaxed(1 << ON_BIT_OFF, pll_cfg->on_reg + 8);

    if(post_div == 0)
    	writel_relaxed(BIT(23) | BIT(14) | (BIT(17) | BIT(18) | GENMASK(8, 0)), pll_cfg->cfg1_reg + 8);
    else
    	writel_relaxed(BIT(14) | (BIT(17) | BIT(18) | GENMASK(8, 0)), pll_cfg->cfg1_reg + 8);

    val = (0 << POST_DIV_OFF) | (fbk_int << FBK_INT_OFF) | (pre_div << PRE_DIV_OFF) |
          (ldo_stb_x2_en << LDO_STB_X2_EN_OFF);

    writel_relaxed(val, pll_cfg->cfg1_reg + 4);
    val = readl_relaxed(pll_cfg->cfg1_reg);
    writel_relaxed(GENMASK(28, 2), pll_cfg->cfg0_reg + 8);

/*
    val = fbk_fra << FBK_FRA_OFF;
    writel_relaxed(val, pll_cfg->cfg0_reg + 4);
*/
    /*on set to 1 */

    writel_relaxed(1 << ON_BIT_OFF, pll_cfg->on_reg + 4);
    udelay(50);
    /*wait LOCKED */
    i = 0;
    while (i++ < try_count) {
        if (readl_relaxed(pll_cfg->lock_sts_reg) & (1 << LOCK_STS_BIT)) {
            break;
        }
    }
    if (i == try_count)
        printk(KERN_ERR "PLL wait LOCKED Error!\n");
    /*wait rdy */
    i = 0;
    while (i++ < try_count) {
        if (readl_relaxed(pll_cfg->rdy_sts_reg) & (1 << pll_cfg->re_open_bit)) {
            break;
        }
    }
    if (i == try_count)
        printk(KERN_ERR "PLL wait rdy Error!\n");
    /*re_open set to 1 */
    writel_relaxed(1 << pll_cfg->re_open_bit, pll_cfg->re_open_reg + 4);
    spin_unlock(&axera_clk_lock);
}

#define to_pll_clk(_hw) container_of(_hw, struct axera_pll_cfg, hw)

static int clk_pll_set_rate(struct clk_hw *hw,unsigned long rate,unsigned long parent_rate)
{
    struct axera_pll_cfg *clk = to_pll_clk(hw);
    pll_set(clk, rate);
    return 0;
}

static unsigned long clk_pll_recalc_rate(struct clk_hw *hw,unsigned long parent_rate)
{
    struct axera_pll_cfg *pll_cfg = to_pll_clk(hw);
    u64 rate_int;
    u32 fbk_int;
    u32 fbk_fra;
    u32 post_div;

    fbk_fra = (readl_relaxed(pll_cfg->cfg0_reg) & GENMASK(25, 2)) >> 2;
    fbk_int = readl_relaxed(pll_cfg->cfg1_reg) & GENMASK(8, 0);
    post_div = (readl_relaxed(pll_cfg->cfg1_reg) & GENMASK(24, 23)) >> 23;
    post_div = clk_power(2,post_div);
    //rate = fbk_int * (pll_cfg->ref_rate / post_div) + ((u64)fbk_fra * (pll_cfg->ref_rate/post_div) + (1 << 23)) / (1 << 24);
    //tmp = ((u64)fbk_fra * (pll_cfg->ref_rate/post_div) + (1 << 23)) / (1 << 24);
    rate_int = fbk_int * (pll_cfg->ref_rate / post_div);
    return rate_int;
}

static long clk_pll_round_rate(struct clk_hw *hw, unsigned long rate,
                               unsigned long *prate)
{
    struct axera_pll_cfg *pll_cfg = to_pll_clk(hw);

    if (rate < pll_cfg->min_rate)
        rate = pll_cfg->min_rate;
    else if (rate > pll_cfg->max_rate)
        rate = pll_cfg->max_rate;
     return rate;
}
static const struct clk_ops ax_clk_pll_ops = {
    .set_rate = clk_pll_set_rate,
    .round_rate = clk_pll_round_rate,
    .recalc_rate = clk_pll_recalc_rate,
};

void axera_clk_register_pll(struct axera_pll_cfg_reg *clks, int nums, struct axera_clock_data *data)
{
    void __iomem *base = data->base;
    struct axera_pll_cfg *p_clk = NULL;
    struct clk *clk = NULL;
    struct clk_init_data init;
    int i;

    p_clk = kzalloc(sizeof(*p_clk) * nums, GFP_KERNEL);
    if (!p_clk)
        return;

    for (i = 0; i < nums; i++) {
        init.name = clks[i].name;
        init.flags = 0;
        init.parent_names =
            (clks[i].parent_name ? &clks[i].parent_name : NULL);
        init.num_parents = (clks[i].parent_name ? 1 : 0);
        init.ops = &ax_clk_pll_ops;

        p_clk->id = clks[i].id;
        p_clk->re_open_reg = base + clks[i].re_open_reg;
        p_clk->on_reg = base + clks[i].on_reg;
        p_clk->cfg0_reg = base + clks[i].cfg0_reg;
        p_clk->cfg1_reg = base + clks[i].cfg1_reg;
        p_clk->lock_sts_reg = base + clks[i].lock_sts_reg;
        p_clk->rdy_sts_reg = base + clks[i].rdy_sts_reg;
        p_clk->re_open_bit = clks[i].re_open_bit;
        p_clk->hw.init = &init;
        p_clk->ref_rate = clks[i].ref_rate;
        p_clk->min_rate = clks[i].min_rate;
        p_clk->max_rate = clks[i].max_rate;

        clk = clk_register(NULL, &p_clk->hw);
        if (IS_ERR(clk)) {
            kfree(p_clk);
            printk(KERN_ERR "%s: failed to register clock %s\n",
                   __func__, clks[i].name);
            continue;
        }

        clk_hw_set_rate_range(&p_clk->hw, p_clk->min_rate, p_clk->max_rate);
        data->clk_data.clks[clks[i].id] = clk;
        p_clk++;
    }
}

EXPORT_SYMBOL(axera_clk_register_pll);
