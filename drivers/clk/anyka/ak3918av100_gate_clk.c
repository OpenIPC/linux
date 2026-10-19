#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <mach/map.h>
#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_KM01A)
#include <dt-bindings/clock/ak3918av100_clock.h>
#elif defined(CONFIG_MACH_AK3918EV300L)
#include <dt-bindings/clock/ak3918ev300l_clock.h>
#elif defined(CONFIG_MACH_AK3918AV130)
#include <dt-bindings/clock/ak3918av130_clock.h>
#endif
#include "ak3918av100_clk.h"

void ak_module_reset_by_clock(void __iomem* res_reg, u32 rst_cfg_shift)
{
    u32 regval;

    pr_info("reset_module:0x%p @ %d\n", res_reg, rst_cfg_shift);

    regval = __raw_readl(res_reg);
    regval |= (0x1 << rst_cfg_shift);
    __raw_writel(regval, res_reg);

    mdelay(1);

    regval = __raw_readl(res_reg);
    regval &= ~(0x1 << rst_cfg_shift);
    __raw_writel(regval, res_reg);

    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
}

static int ak_gate_clk_enable(struct clk_hw* hw)
{
    struct ak_gate_clk* gate_clk = to_clk_ak_gate(hw);
    u32 con;
    unsigned long flags;

    local_irq_save(flags);
    if (gate_clk->reg == AK_VA_SYSCTRL) {
        con = __raw_readl(gate_clk->reg + CLOCK_GATE_CTRL);
        con &= ~(1 << gate_clk->ctrlbit);
        __raw_writel(con, gate_clk->reg + CLOCK_GATE_CTRL);

        con = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
    }

    // ak_module_reset_by_clock(gate_clk->reg + CLOCK_SOFT_RESET,
    // gate_clk->ctrlbit);

    local_irq_restore(flags);
    return 0;
}

static void ak_gate_clk_disable(struct clk_hw* hw)
{
    struct ak_gate_clk* gate_clk = to_clk_ak_gate(hw);
    u32 con;
    unsigned long flags;

    local_irq_save(flags);
    if (gate_clk->reg == AK_VA_SYSCTRL) {
        con = __raw_readl(gate_clk->reg + CLOCK_GATE_CTRL);
        con |= (1 << gate_clk->ctrlbit);
        __raw_writel(con, gate_clk->reg + CLOCK_GATE_CTRL);

        con = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
    }

    local_irq_restore(flags);
}

const struct clk_ops ak_gate_clk_ops = {
#ifndef CONFIG_MACH_AK3918AV100_FPGA
    .enable = ak_gate_clk_enable,
    .disable = ak_gate_clk_disable,
#endif
};

struct clk* __init ak_clk_register_gate(const char* name,
    const char* parent_name, void __iomem* res_reg, u8 ctrlbit,
    const struct clk_ops* ops)
{
    struct ak_gate_clk* gate_clk;
    struct clk* clk;
    struct clk_init_data init = {};

    gate_clk = kzalloc(sizeof(*gate_clk), GFP_KERNEL);
    if (!gate_clk)
        return ERR_PTR(-ENOMEM);

    init.name = name;
    init.flags = CLK_SET_RATE_PARENT;
    init.parent_names = parent_name ? &parent_name : NULL;
    init.num_parents = parent_name ? 1 : 0;
    init.ops = ops;

    gate_clk->id = ctrlbit;
    gate_clk->reg = res_reg;
    gate_clk->ctrlbit = ctrlbit;

    gate_clk->hw.init = &init;

    clk = clk_register(NULL, &gate_clk->hw);
    if (IS_ERR(clk))
        kfree(gate_clk);

    return clk;
}

static void __init of_ak_gate_clk_init(struct device_node* np)
{
    struct clk_onecell_data* clk_data;
    const char* clk_name = np->name;
    const char* parent_name = of_clk_get_parent_name(np, 0);
    void __iomem* reg = AK_VA_SYSCTRL;
    u32 ctrlbit;
    int number, index;

    clk_data = kmalloc(sizeof(struct clk_onecell_data), GFP_KERNEL);
    if (!clk_data)
        return;

    number = of_property_count_u32_elems(np, "clock-ctrlbit");

    clk_data->clks = kcalloc(number, sizeof(struct clk*), GFP_KERNEL);
    if (!clk_data->clks)
        goto err_free_data;

    for (index = 0; index < number; index++) {
        of_property_read_string_index(
            np, "clock-output-names", index, &clk_name);
        of_property_read_u32_index(np, "clock-ctrlbit", index, &ctrlbit);

        pr_debug(
            "of_ak_gate_clk_init %s %s %d\n", clk_name, parent_name, ctrlbit);

        clk_data->clks[index] = ak_clk_register_gate(
            clk_name, parent_name, reg, ctrlbit, &ak_gate_clk_ops);

        if (IS_ERR(clk_data->clks[index])) {
            pr_err("of_ak_gate_clk_init register gate clk failed: clk_name:%s, "
                   "index = %d\n",
                clk_name, index);
            WARN_ON(true);
            continue;
        }
    }

    clk_data->clk_num = number;

    if (number == 1)
        of_clk_add_provider(np, of_clk_src_simple_get, clk_data->clks[0]);
    else
        of_clk_add_provider(np, of_clk_src_onecell_get, clk_data);

    return;

err_free_data:
    kfree(clk_data);

/* // To check
    struct clk_onecell_data* clk_data;
    const char* clk_name = np->name;
    const char* parent_name = of_clk_get_parent_name(np, 0);
    ;
    u32 ctrlbit;
    int number, index;

    clk_data = kmalloc(sizeof(struct clk_onecell_data), GFP_KERNEL);
    if (!clk_data)
        return;

    number = of_property_count_u32_elems(np, "clock-ctrlbit");

    clk_data->clks = kcalloc(number, sizeof(struct clk*), GFP_KERNEL);
    if (!clk_data->clks)
        goto err_free_data;

    for (index = 0; index < number; index++) {
        of_property_read_string_index(
            np, "clock-output-names", index, &clk_name);
        of_property_read_u32_index(np, "clock-ctrlbit", index, &ctrlbit);

        pr_debug(
            "of_ak_gate_clk_init %s %s %d\n", clk_name, parent_name, ctrlbit);

        clk_data->clks[index] = clk_register_gate(NULL, clk_name, parent_name,
            0, AK_VA_SYSCTRL + CLOCK_GATE_CTRL, ctrlbit, 0,
            &ak3918av100_clk_lock);

        if (IS_ERR(clk_data->clks[index])) {
            pr_err("of_ak_gate_clk_init register gate clk failed: clk_name:%s, "
                   "index = %d\n",
                clk_name, index);
            WARN_ON(true);
            continue;
        }
    }
    clk_data->clk_num = number;
    of_clk_add_provider(np, of_clk_src_onecell_get, clk_data);

    return;

err_free_data:
    kfree(clk_data);

*/
}
/*end of func*/

#if defined(CONFIG_MACH_AK3918AV100)
CLK_OF_DECLARE(
    ak3918av100_gate_clk, "anyka,ak3918av100-gate-clk", of_ak_gate_clk_init);
#elif defined(CONFIG_MACH_AK3918EV300L)
CLK_OF_DECLARE(
    ak3918ev300l_gate_clk, "anyka,ak3918ev300l-gate-clk", of_ak_gate_clk_init);
#elif defined(CONFIG_MACH_AK3918AV130)
CLK_OF_DECLARE(
    ak3918av130_gate_clk, "anyka,ak3918av130-gate-clk", of_ak_gate_clk_init);    
#elif defined(CONFIG_MACH_KM01A)
CLK_OF_DECLARE(
    km01a_gate_clk, "anyka,km01a-gate-clk", of_ak_gate_clk_init);  
#endif
