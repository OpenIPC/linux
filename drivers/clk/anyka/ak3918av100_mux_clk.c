/*
 * ak3918av100 clk driver
 *
 * Copyright (C) 2021 Anyka(Guangzhou) Microelectronics Technology Co., Ltd.
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
 */

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

#define MAX_CLOCK_SOURCE 8

static DEFINE_SPINLOCK(ak_clk_lock);

struct ak_mux_clk_priv {
    unsigned long reg_offset;
    u8 shift;
    u8 width;
#define CLK_MUX_FLAGS_NONE 0
    /*
     *  #define CLK_MUX_INDEX_ONE       BIT(0)
     *  #define CLK_MUX_INDEX_BIT       BIT(1)
     *  #define CLK_MUX_HIWORD_MASK     BIT(2)
     *  #define CLK_MUX_READ_ONLY       BIT(3)  //mux can't be changed
     *  #define CLK_MUX_ROUND_CLOSEST       BIT(4)
     */
    u8 mux_flags;
    u32* table;
};

static u32 mux_index_to_regval_table[] = { 0, 1, 2 };

static struct ak_mux_clk_priv ak_mux_clk_priv[] = {
    /* isp  0x0800,011c bit[5:3] */
    [MUX_ISP_CLK]
    = { 0x11c, 3, 3, CLK_MUX_FLAGS_NONE, mux_index_to_regval_table },
#ifdef CONFIG_MACH_AK3918AV100
    /* npu  0x0800,011c bit[13:11] */
    [MUX_NPU_CLK]
    = { 0x11c, 11, 3, CLK_MUX_FLAGS_NONE, mux_index_to_regval_table },
#endif
    /* enc  0x0800,011c bit[21:19] */
    [MUX_ENC_CLK]
    = { 0x18, 19, 3, CLK_MUX_FLAGS_NONE, mux_index_to_regval_table },

    /* csi0  0x0800,0018    bit[18:16] */
    [MUX_CSI0_CLK]
    = { 0x18, 16, 3, CLK_MUX_FLAGS_NONE, mux_index_to_regval_table },
#ifdef CONFIG_MACH_AK3918AV100
    /* csi1  0x0800,0018    bit[28:26] */
    [MUX_CSI1_CLK]
    = { 0x18, 26, 3, CLK_MUX_FLAGS_NONE, mux_index_to_regval_table },
#endif
};

static void __init of_ak_mux_clk_init(struct device_node* np)
{
#if 0 // one clock id version
    struct clk *clk = NULL;
    const char *clk_name = np->name;
    const char *parent_names[MAX_CLOCK_SOURCE];
    void __iomem *res_reg = AK_VA_SYSCTRL;
    int id, num_parents;

    num_parents = of_clk_get_parent_count(np);
    if (num_parents <= 0 || num_parents > MAX_CLOCK_SOURCE)
        return;
        
    num_parents = of_clk_parent_fill(np, parent_names, num_parents);
    
    of_property_read_string(np, "clock-output-names", &clk_name);
    of_property_read_u32(np, "clock-id", &id);

    clk = ak_register_mux_clk(clk_name, parent_names, num_parents,
                        res_reg, id, &ak_mux_clk_ops);
    if (!IS_ERR(clk)) {
        of_clk_add_provider(np, of_clk_src_simple_get, clk);
    } else
        pr_err("of_ak_mux_clk_init register mux clk failed: clk_name:%s %s \n",
                clk_name, (char *)PTR_ERR(clk));
    retrun ;

#else // mutli clock id version

    const char* parent_names[MAX_CLOCK_SOURCE];
    int num_parents, num_clk;
    int clock_id, index;
    const char* clk_name = np->name;
    void __iomem* reg = AK_VA_SYSCTRL;
    struct clk_onecell_data* clk_data;
    u32 mask;

    pr_debug("enter %s, np[%p]\n", __func__, np);

    num_parents = of_clk_get_parent_count(np);
    if (num_parents <= 0 || num_parents > MAX_CLOCK_SOURCE) {
        pr_err("%s num_parents =%d, invalid\n", __func__, num_parents);
        return;
    }

    clk_data = kmalloc(sizeof(struct clk_onecell_data), GFP_KERNEL);
    if (!clk_data) {
        pr_err("%s clk_data null\n", __func__);
        return;
    }
    num_parents = of_clk_parent_fill(np, parent_names, num_parents);

    num_clk = of_property_count_u32_elems(np, "clock-id");

    clk_data->clks = kcalloc(num_clk, sizeof(struct clk*), GFP_KERNEL);
    if (!clk_data->clks) {
        pr_err("%s clk_data->clks null\n", __func__);
        goto err_free_data;
    }

    pr_debug("num_parents = %d, num_clk =%d\n", num_parents, num_clk);

    for (index = 0; index < num_parents; index++) {
        pr_debug(
            "%s parent_names[%d] = %s\n", __func__, index, parent_names[index]);
    }

    for (index = 0; index < num_clk; index++) {

        of_property_read_string_index(
            np, "clock-output-names", index, &clk_name);
        of_property_read_u32_index(np, "clock-id", index, &clock_id);

        pr_debug("%s clk_register_mux_table for clock %s ( clock_id = %d)\n",
            __func__, clk_name, clock_id);
#if 0
        clk_data->clks[index] = ak_register_mux_clk(clk_name, parent_names,
         num_parents, res_reg, clock_id, &ak_mux_clk_ops);
#endif
        mask = BIT(ak_mux_clk_priv[clock_id].width) - 1;
        clk_data->clks[index]
            = clk_register_mux_table(NULL, clk_name, parent_names, num_parents,
                /*
                 * CLK_SET_PARENT_GATE:
                 * must be gated across re-parent.
                 * clk_core_set_parent check that we are allowed to
                 * re-parent if the clock is in use
                 *
                 * CLK_SET_RATE_PARENT:
                 * propagate rate change up one level.
                 * pass rate changes to the parent clock.
                 */
                CLK_SET_PARENT_GATE | CLK_SET_RATE_PARENT,
                reg + ak_mux_clk_priv[clock_id].reg_offset,
                ak_mux_clk_priv[clock_id].shift, mask,
                ak_mux_clk_priv[clock_id].mux_flags,
                ak_mux_clk_priv[clock_id].table, &ak_clk_lock);
        if (IS_ERR(clk_data->clks[index])) {
            pr_err("of_ak_gate_clk_init register gate clk failed: clk_name:%s, "
                   "index = %d\n",
                clk_name, index);
            WARN_ON(true);
            continue;
        }
        clk_register_clkdev(clk_data->clks[index], clk_name, NULL); // fix
                                                                    // latter
    }

    clk_data->clk_num = num_clk;

    if (num_clk == 1) {
        of_clk_add_provider(np, of_clk_src_simple_get, clk_data->clks[0]);
        kfree(clk_data->clks);
        kfree(clk_data);
    } else
        of_clk_add_provider(np, of_clk_src_onecell_get, clk_data);

    return;

err_free_data:
    kfree(clk_data);
    return;

#endif
}
/*end of func*/

#if defined(CONFIG_MACH_AK3918AV100)
CLK_OF_DECLARE(
    ak3918av100_mux_clk, "anyka,ak3918av100-mux-clk", of_ak_mux_clk_init);
#elif defined(CONFIG_MACH_AK3918EV300L)
CLK_OF_DECLARE(
    ak3918ev300l_mux_clk, "anyka,ak3918ev300l-mux-clk", of_ak_mux_clk_init);
#elif defined(CONFIG_MACH_AK3918AV130)
CLK_OF_DECLARE(
    ak3918av130_mux_clk, "anyka,ak3918av130-mux-clk", of_ak_mux_clk_init);  
#elif defined(CONFIG_MACH_KM01A)
CLK_OF_DECLARE(
    km01a_mux_clk, "anyka,km01a-mux-clk", of_ak_mux_clk_init);
#endif
