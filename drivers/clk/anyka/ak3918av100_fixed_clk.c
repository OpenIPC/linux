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
#include <linux/syscore_ops.h>

#elif defined(CONFIG_MACH_AK3918EV300L)
#include <dt-bindings/clock/ak3918ev300l_clock.h>
#elif defined(CONFIG_MACH_AK3918AV130)
#include <dt-bindings/clock/ak3918av130_clock.h>
#endif
#include "ak3918av100_clk.h"

#define GCLK_FREQ_MAX (200 * MHz)
#ifdef CONFIG_MACH_AK3918AV130
#define PLL1_FREQ_MAX (1200 * MHz)
#define JCLK_FREQ_MAX (1200 * MHz)
#define HCLK_FREQ_MAX (600 * MHz)
#else
#define PLL1_FREQ_MAX (1000 * MHz)
#define JCLK_FREQ_MAX (900 * MHz)
#define HCLK_FREQ_MAX (500 * MHz)
#endif

#define PLL2_FREQ_MAX_MHZ (800)
#define PLL3_FREQ_MAX_MHZ (800)

static u32 gclk_freq;
#if defined(CONFIG_PM) && defined(CONFIG_MACH_KM01A)
struct ak_pll_data {
    u32 id;
    u32 freq;
    u32 od;
    u32 nr;
};

static struct ak_pll_data ak_pll_data_tables[2];
#endif

extern void ak_early_serial_send(unsigned int line,
        const char *s, unsigned int count);

/*
 * ak_early_serial_printk
 */
static void ak_early_serial_printk(unsigned int line, const char *fmt, ...)
{
    va_list args;
    char buf[256];
    int i;

    va_start(args, fmt);
    i = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    ak_early_serial_send(line, buf, i);
    printk(KERN_ERR"%s", buf);
}

/*
 * ak_register_fixed_clk
 */
struct clk* __init ak_register_fixed_clk(const char* name,
        const char* parent_name, void __iomem* res_reg, u32 fixed_rate, int id,
        const struct clk_ops* ops)
{
    struct ak_fixed_clk* fixed_clk;
    struct clk* clk;
    struct clk_init_data init = {};

    fixed_clk = kzalloc(sizeof(*fixed_clk), GFP_KERNEL);
    if (!fixed_clk)
        return ERR_PTR(-ENOMEM);

    init.name = name;
    init.flags = CLK_IS_BASIC;
    init.parent_names = parent_name ? &parent_name : NULL;
    init.num_parents = parent_name ? 1 : 0;
    init.ops = ops;

    fixed_clk->reg = res_reg;
    fixed_clk->fixed_rate = fixed_rate;
    fixed_clk->id = id;

    fixed_clk->hw.init = &init;

    clk = clk_register(NULL, &fixed_clk->hw);
    if (IS_ERR(clk))
        kfree(fixed_clk);

    return clk;
}

/*
 * @BRIEF       ak_get_pll_clk
 * @PARAM[in]   reg
 * @RETURN      unsigned long
 * @RETVAL      pll_clk * KHz
 */
static unsigned long ak_get_pll_clk(void __iomem* reg)
{
    u32 pll_nf, pll_nr, pll_od;
    u32 pll_clk;
    u32 regval;

    regval = __raw_readl(reg);
    pll_nf = (regval & 0x1fff);
    pll_nr = (regval >> 13) & 0x3f;
    pll_od = (regval >> 19) & 0xf;

    pll_nf++;
    pll_nr++;
    pll_od++;

    pll_clk = 24000 * pll_nf / pll_nr / pll_od; // clk unit: kHz

    pr_debug("pll_nf=%d, pll_nr=%d, pll_od=%d, pll_clk=%d kHz\n", pll_nf,
            pll_nr, pll_od, pll_clk);

    return pll_clk * KHz;
}

/*
 * @BRIEF       ak_get_pll1_clk
 * @PARAM[in]   reg
 * @RETURN      unsigned long
 * @RETVAL      80 * MHz/ak_get_pll_clk(reg + CLOCK_PLL1_CTRL)
 */
static unsigned long ak_get_pll1_clk(void __iomem* reg)
{
#ifdef CONFIG_MACH_AK3918AV100_FPGA
    // FPGA use default clock
    return 80 * MHz;
#else
    return ak_get_pll_clk(reg + CLOCK_PLL1_CTRL);
#endif
}

/*
 * ak_get_pll2_clk
 */
unsigned long ak_get_pll2_clk(void __iomem* reg)
{
#ifdef CONFIG_MACH_AK3918AV100_FPGA
    // FPGA use default clock
    return 60 * MHz;
#else
    return ak_get_pll_clk(reg + CLOCK_PLL2_CTRL);
#endif
}

/*
 * ak_get_pll3_clk
 */
unsigned long ak_get_pll3_clk(void __iomem* reg)
{
#ifdef CONFIG_MACH_AK3918AV100_FPGA
    // FPGA use default clock
    return 60 * MHz;
#else
    return ak_get_pll_clk(reg + CLOCK_PLL3_CTRL);
#endif
}

/*
 * ak_get_gclk
 */
unsigned long ak_get_gclk(void __iomem* reg)
{
#ifdef CONFIG_MACH_AK3918AV100_FPGA
    return get_pll2_clk();
#else
    unsigned long regval;
    unsigned long gclk_div_num_cfg;

    regval = __raw_readl(reg + CLOCK_PLL2_CTRL);
    gclk_div_num_cfg = (regval & (0x3 << 23)) >> 23;

    return ak_get_pll2_clk(reg) / (2 * (gclk_div_num_cfg + 1));
#endif
}

/*
 * ak_pll1_fixed_clk_recalc_rate
 */
static unsigned long ak_pll1_fixed_clk_recalc_rate(
        struct clk_hw* hw, unsigned long parent_rate)
{
    struct ak_fixed_clk* fixed_clk = to_clk_ak_fixed(hw);
    int cpu_1x_2x_sel_cfg = 0;
    u32 regval;

    regval = __raw_readl(fixed_clk->reg + CLOCK_PLL1_CTRL);
    if (regval & PLL1_CPU_1X_2X_SEL_CFG)
        cpu_1x_2x_sel_cfg = 1; // CPU 1X mode

    /*
     *  PLL1 is configed in boot setting, and can't be configed in kernel.
     *  ReadOnly
     *
     *  DPHY:   DDR PHY source clock frequency
     *  JCLK:   CPU opreating clock
     *  DDR2:   DDR2 clock frequency
     *  HCLK:   CPU bus frequency
     *  DCLK:   memory controller working clock, equal to HCLK
     */
    switch (fixed_clk->id) {
        case PLL1_CLK: //
            fixed_clk->fixed_rate = ak_get_pll1_clk(fixed_clk->reg);
            pr_info("PLL1: %lu(Mhz) \n", fixed_clk->fixed_rate / MHz);
            break;
        case PLL1_DPHY: // DPHY is equal to PLL1
            fixed_clk->fixed_rate = ak_get_pll1_clk(fixed_clk->reg);
            break;

        case PLL1_JCLK: // JCLK
            fixed_clk->fixed_rate
                = ak_get_pll1_clk(fixed_clk->reg) >> cpu_1x_2x_sel_cfg;
            pr_info("CPU(JCLK): %lu(Mhz) \n", fixed_clk->fixed_rate / MHz);
            break;

        case PLL1_DDR2: // DDR2 is equal to PLL1/2
            fixed_clk->fixed_rate = ak_get_pll1_clk(fixed_clk->reg) / 2;
            pr_info("DDR2: %lu(Mhz) \n", fixed_clk->fixed_rate / MHz);
            break;

        case PLL1_HCLK: // HCLK
        case PLL1_DCLK: // DCLK
            // HCLK/DCLK is equal to PLL1/2
            fixed_clk->fixed_rate = ak_get_pll1_clk(fixed_clk->reg) / 2;
            // pr_info("HCLK/DCLK: %lu(Mhz) \n", fixed_clk->fixed_rate/MHz);
            break;
        default:
            pr_err("No CLK CONFIG IN DTS.\n");
            break;
    }

    return fixed_clk->fixed_rate;
}

/*
 * @BRIEF       ak_pll2_fixed_clk_recalc_rate
 * @PARAM[in]   hw
 * @PARAM[in]   parent_rate
 * @RETURN      unsigned long
 * @RETVAL      fixed_clk->fixed_rate
 */
static unsigned long ak_pll2_fixed_clk_recalc_rate(
        struct clk_hw* hw, unsigned long parent_rate)
{
    u32 regval;
    u32 factor;
    struct ak_fixed_clk* fixed_clk = to_clk_ak_fixed(hw);

    switch (fixed_clk->id) {
        case PLL2_CLK:
            fixed_clk->fixed_rate = ak_get_pll2_clk(fixed_clk->reg);
            pr_debug("PLL2: %lu(Mhz) \n", fixed_clk->fixed_rate / MHz);
            break;

        case PLL2_GCLK:
            fixed_clk->fixed_rate = ak_get_gclk(fixed_clk->reg);
            pr_info("PLL2_GCLK: %lu(Mhz) \n", fixed_clk->fixed_rate / MHz);
            break;

        case PLL2_DPHY_CFGCLK:
            regval = __raw_readl(fixed_clk->reg + CLOCK_PLL2_CTRL);
            factor = (regval >> CLOCK_DPHY_CFG_CLK_DIV_NUM_CFG_SHIFT)
                & CLOCK_DPHY_CFG_CLK_DIV_NUM_CFG_MASK;
            fixed_clk->fixed_rate = parent_rate / (factor + 1);
            break;
        default:
            pr_err("No CLK CONFIG IN DTS.\n");
            break;
    }

    return fixed_clk->fixed_rate;
}

/*
 * ak_pll3_fixed_clk_recalc_rate
 */
static unsigned long ak_pll3_fixed_clk_recalc_rate(
        struct clk_hw* hw, unsigned long parent_rate)
{
    struct ak_fixed_clk* fixed_clk = to_clk_ak_fixed(hw);

    switch (fixed_clk->id) {
        case PLL3_CLK:
            fixed_clk->fixed_rate = ak_get_pll3_clk(fixed_clk->reg);
            break;
        default:
            pr_err("No CLK CONFIG IN DTS.\n");
            break;
    }

    return fixed_clk->fixed_rate;
}

static struct clk_ops ak_pll1_fixed_clk_ops = {
    .recalc_rate = ak_pll1_fixed_clk_recalc_rate,
};
static struct clk_ops ak_pll2_fixed_clk_ops = {
    .recalc_rate = ak_pll2_fixed_clk_recalc_rate,
};
static struct clk_ops ak_pll3_fixed_clk_ops = {
    .recalc_rate = ak_pll3_fixed_clk_recalc_rate,
};

struct clk_ops* ak_fixed_clk_ops[] = {
    &ak_pll1_fixed_clk_ops,
    &ak_pll2_fixed_clk_ops,
    &ak_pll3_fixed_clk_ops,
};

/*
 * gclk_init
 */
void gclk_init(void __iomem* reg, u32 pll_freq)
{
    u32 regval;
    u32 factor = pll_freq / gclk_freq / 2;

    pr_debug("%s, pll_freq = %d, gclk_freq =%d, factor =%d\n", __func__,
            pll_freq, gclk_freq, factor);

    if ((pll_freq / (factor * 2)) > GCLK_FREQ_MAX) {
        gclk_freq = GCLK_FREQ_MAX;
        if (pll_freq % (gclk_freq * 2)) {
            factor = pll_freq / gclk_freq / 2 + 1;
        } else {
            factor = pll_freq / gclk_freq / 2;
        }
    }

    /*
     * 1. First wait the gclk_div_vld_cfg to 0
     */
    ak_clock_divider_adjust_complete(
            reg + CLOCK_PLL2_CTRL, CLOCK_GCLK_VLD_SHIFT, CLOCK_GCLK_VLD_MASK);

    /*
     * 2. enable gclk
     * gclk is always open. Do nothing
     */

    /*
     * 3. set factor
     */
    regval = __raw_readl(reg + CLOCK_PLL2_CTRL);
    regval &= ~(CLOCK_GCLK_DIV_NUM_CFG_MASK << CLOCK_GCLK_DIV_NUM_CFG_SHIFT);
    regval |= (((factor - 1) & CLOCK_GCLK_DIV_NUM_CFG_MASK)
            << CLOCK_GCLK_DIV_NUM_CFG_SHIFT);
    __raw_writel(regval, reg + CLOCK_PLL2_CTRL);

    /*
     * 4. set gclk_div_vld_cfg to 1 to make valid
     */
    regval = __raw_readl(reg + CLOCK_PLL2_CTRL);
    regval |= ((0x1) << CLOCK_GCLK_VLD_SHIFT);
    __raw_writel(regval, reg + CLOCK_PLL2_CTRL);

    /*
     * 5. wait the gclk_div_vld_cfg to 0 again.
     */
    ak_clock_divider_adjust_complete(
            reg + CLOCK_PLL2_CTRL, CLOCK_GCLK_VLD_SHIFT, CLOCK_GCLK_VLD_MASK);

    pr_debug("%s: CLOCK_PLL2_CTRL 0x%x\n", __func__,
            __raw_readl(reg + CLOCK_PLL2_CTRL));
} /* end of func */

/*
 * pll_init
 */
void pll_init(
        int pll_id, void __iomem* reg, u32 pll_freq, u32 pll_od, u32 pll_nr)
{
    u32 regval;
    u32 pll_nf;

    if (1 == pll_id)
        return;

    pll_nf = pll_freq / KHz * pll_od * pll_nr / 24000; // clk unit: kHz

    /* since 500MHz <= 24* NF / NR <= 2500 MHz */
    if ((24 * pll_nf / pll_nr) < 500 || (24 * pll_nf / pll_nr) > 2500) {
        ak_early_serial_printk(0, "PLL Frequency parameter Error: "
                "pll_freq %d pll_od %d pll_nr %d pll_nf %d\n",
                pll_freq, pll_od, pll_nr, pll_nf);
        panic(" ");
    }

    if ((pll_id == 2) && ((24 * pll_nf/pll_nr/pll_od)>PLL2_FREQ_MAX_MHZ)) {
        ak_early_serial_printk(0,
                "PLL2 frequency %uMHz larger than %d MHz!",
                24 * pll_nf / pll_nr, PLL2_FREQ_MAX_MHZ);
        panic(" ");
    }
    if ((pll_id == 3) && ((24 * pll_nf/pll_nr/pll_od)>PLL3_FREQ_MAX_MHZ)) {
        ak_early_serial_printk(0,
                "PLL3 frequency %uMHz larger than %d MHz!",
                24 * pll_nf / pll_nr, PLL3_FREQ_MAX_MHZ);
        panic(" ");
    }

    pr_debug("@%s, pll_id = %d, pll_freq = %d\n", __func__, pll_id, pll_freq);
    pr_debug("@%s, pll_nf:%d, pll_nr =%d, pll_od =%d\n", __func__, pll_nf,
            pll_nr, pll_od);

    pll_nf--;
    pll_nr--;
    pll_od--;

    pll_nf &= 0x1fff; /* pll_clknf_cfg  0x0800,0008 or 0x0800,0014 bit  [12:0]
    */
    pll_nr
        &= 0x3f; /* pll_clknr_cfg  0x0800,0008 or 0x0800,0014 bit  [18:13] */
    pll_od
        &= 0xf; /* pll_clkod_cfg   0x0800,0008 or 0x0800,0014 bit  [22:19] */

    /* for pll2 */
    if (pll_id == 2) {
        ak_clock_divider_adjust_complete(reg + CLOCK_PLL1_CTRL,
                PLL2_FRQ_ADJ_VLD_CFG_SHIFT, PLL2_FRQ_ADJ_VLD_CFG_MASK);

        regval = __raw_readl(reg + CLOCK_PLL2_CTRL);
        regval &= ~0x7fffff;
        /* set core pll frequency */
        regval |= (pll_od << 19) | (pll_nr << 13) | (pll_nf);
        __raw_writel(regval, reg + CLOCK_PLL2_CTRL);

        /* 0x0800,00e0*/
        regval = __raw_readl(reg + 0xe0);
        regval &= ~(0xfff); // pll2 bwadj_cfg[11:0]
        // regval |= ((pll_nf+1)/4-1);   // NB=NF/4
        regval |= ((pll_nf + 1) / 6
                - 1); // NB=NF/6. when NF=100, NR=1, OD=6,  let NB=16
        __raw_writel(regval, reg + 0xe0);

        /* pll2_freq_adj_cfg  0x0800,0004 bit [28]*/
        regval = __raw_readl(reg + CLOCK_PLL1_CTRL);
        regval |= (1 << PLL2_FRQ_ADJ_VLD_CFG_SHIFT);
        __raw_writel(regval, reg + CLOCK_PLL1_CTRL);

        ak_clock_divider_adjust_complete(reg + CLOCK_PLL1_CTRL,
                PLL2_FRQ_ADJ_VLD_CFG_SHIFT, PLL2_FRQ_ADJ_VLD_CFG_MASK);
        /* init gclk */
        gclk_init(reg, pll_freq);
    } else if (pll_id == 3) {
        ak_clock_divider_adjust_complete(reg + CLOCK_PLL1_CTRL,
                PLL3_FRQ_ADJ_VLD_CFG_SHIFT, PLL3_FRQ_ADJ_VLD_CFG_MASK);

        regval = __raw_readl(reg + CLOCK_PLL3_CTRL);
        regval &= ~0x7fffff;
        /* set core pll frequency */
        regval |= (pll_od << 19) | (pll_nr << 13) | (pll_nf);
        __raw_writel(regval, reg + CLOCK_PLL3_CTRL);

        regval = __raw_readl(reg + 0xe0);
        regval &= ~(0xfff << 12); // pll3 bwadj_cfg[11:0]
        regval |= ((pll_nf + 1) / 2 - 1) << 12;
        __raw_writel(regval, reg + 0xe0);

        /* pll3_freq_adj_cfg  0x0800,0004 bit [29]*/
        regval = __raw_readl(reg + CLOCK_PLL1_CTRL);
        regval |= (1 << PLL3_FRQ_ADJ_VLD_CFG_SHIFT);
        __raw_writel(regval, reg + CLOCK_PLL1_CTRL);

        ak_clock_divider_adjust_complete(reg + CLOCK_PLL1_CTRL,
                PLL3_FRQ_ADJ_VLD_CFG_SHIFT, PLL3_FRQ_ADJ_VLD_CFG_MASK);

    } else {
        ak_early_serial_printk(0, "PLL Frequency parameter Error");
        panic(" ");
    }

    return;
}
/*end of func*/

#if defined(CONFIG_PM) && defined(CONFIG_MACH_KM01A)

static u32 ak_fixed_pm_flag = 0;

/*
 * ak_fixed_clk_suspend
 */
static int ak_fixed_clk_suspend(void)
{
    return 0;
}

/*
 * ak_fixed_clk_resume
 */
static void ak_fixed_clk_resume(void)
{
#ifndef CONFIG_MACH_AK3918AV100_FPGA
    pll_init(ak_pll_data_tables[0].id, AK_VA_SYSCTRL,
            ak_pll_data_tables[0].freq,
            ak_pll_data_tables[0].od, ak_pll_data_tables[0].nr);

    pll_init(ak_pll_data_tables[1].id, AK_VA_SYSCTRL,
            ak_pll_data_tables[1].freq,
            ak_pll_data_tables[1].od, ak_pll_data_tables[1].nr);
#endif
}

static struct syscore_ops ak_fixed_clk_syscore_ops = {
    .suspend = ak_fixed_clk_suspend,
    .resume = ak_fixed_clk_resume,
};
#endif

/*
 * of_ak_fixed_clk_init
 */
static void __init of_ak_fixed_clk_init(struct device_node* np)
{
    struct clk_onecell_data* clk_data;
    const char* clk_name = np->name;
    const char* parent_name = of_clk_get_parent_name(np, 0);
    void __iomem* res_reg = AK_VA_SYSCTRL;
    u32 rate = -1;
    u32 div_od, div_nr;
    int clock_id, index, number;
    int pll_id;

    clk_data = kmalloc(sizeof(struct clk_onecell_data), GFP_KERNEL);
    if (!clk_data)
        return;

    number = of_property_count_u32_elems(np, "clock-id");
    clk_data->clks = kcalloc(number, sizeof(struct clk*), GFP_KERNEL);
    if (!clk_data->clks)
        goto err_free_data;

    of_property_read_u32(np, "clock-frequency", &rate);
    of_property_read_u32(np, "clock-div-od", &div_od);
    of_property_read_u32(np, "clock-div-nr", &div_nr);
    of_property_read_u32(np, "gclk-frequency", &gclk_freq);

    pll_id = of_alias_get_id(np, "pll");
#ifndef CONFIG_MACH_AK3918AV100_FPGA
#if defined(CONFIG_PM) && defined(CONFIG_MACH_KM01A)
    if (pll_id == 2) {
        ak_pll_data_tables[0].id = pll_id;
        ak_pll_data_tables[0].freq = rate;
        ak_pll_data_tables[0].od = div_od;
        ak_pll_data_tables[0].nr = div_nr;
    } else if (pll_id == 3) {
        ak_pll_data_tables[1].id = pll_id;
        ak_pll_data_tables[1].freq = rate;
        ak_pll_data_tables[1].od = div_od;
        ak_pll_data_tables[1].nr = div_nr;
    }
#endif
#if !defined(CONFIG_ANYKA_FASTSYS)
    pll_init(pll_id, res_reg, rate, div_od, div_nr);
#endif
    if (pll_id == 1) {
        int cpu_1x_2x_sel_cfg = 0;
        u32 regval;
        unsigned long pll1_freq, jclk_freq, hclk_dclk_ddr2_freq;

        pll1_freq = ak_get_pll1_clk(res_reg);
        if (pll1_freq > PLL1_FREQ_MAX) {
            ak_early_serial_printk(0,
                    "PLL1 frequency %lu larger than %ld MHz!",
                    pll1_freq, PLL1_FREQ_MAX);
            panic(" ");
        }

        regval = __raw_readl(res_reg + CLOCK_PLL1_CTRL);
        if (regval & PLL1_CPU_1X_2X_SEL_CFG)
            cpu_1x_2x_sel_cfg = 1; // CPU 1X mode

        jclk_freq = pll1_freq >> cpu_1x_2x_sel_cfg;
        if (jclk_freq > JCLK_FREQ_MAX) {
            ak_early_serial_printk(0,
                    "JCLK frequency %lu larger than %ld MHz!",
                    jclk_freq, JCLK_FREQ_MAX);
            panic(" ");
        }

        hclk_dclk_ddr2_freq = pll1_freq / 2;
        if (hclk_dclk_ddr2_freq > HCLK_FREQ_MAX) {
            ak_early_serial_printk(0,
                    "HCLK/DCLK/DDR2 frequency %lu larger than %ld MHz!",
                    hclk_dclk_ddr2_freq, HCLK_FREQ_MAX);
            panic(" ");
        }
    }
#endif
    for (index = 0; index < number; index++) {
        of_property_read_string_index(
                np, "clock-output-names", index, &clk_name);
        of_property_read_u32_index(np, "clock-id", index, &clock_id);
        pr_debug("[pll%d]%s %s %s %d %d\n", pll_id, __func__, clk_name,
                parent_name, rate, clock_id);

        clk_data->clks[index] = ak_register_fixed_clk(clk_name, parent_name,
                res_reg, rate, clock_id, ak_fixed_clk_ops[pll_id - 1]);
        if (IS_ERR(clk_data->clks[index])) {
            pr_err("%s register fixed clk failed: clk_name:%s, index = %d\n",
                    __func__, clk_name, index);
            WARN_ON(true);
            continue;
        }
        clk_register_clkdev(clk_data->clks[index], clk_name, NULL);
    }
    clk_data->clk_num = number;
    if (number == 1) {
        of_clk_add_provider(np, of_clk_src_simple_get, clk_data->clks[0]);
        kfree(clk_data->clks);
        kfree(clk_data);
    } else
        of_clk_add_provider(np, of_clk_src_onecell_get, clk_data);

#if defined(CONFIG_PM) && defined(CONFIG_MACH_KM01A)
    if (!ak_fixed_pm_flag) {
        register_syscore_ops(&ak_fixed_clk_syscore_ops);
        ak_fixed_pm_flag = 1;
    }
#endif

    return;

err_free_data:
    kfree(clk_data);
} /* end of func */

#if defined(CONFIG_MACH_AK3918AV100)
CLK_OF_DECLARE(ak3918av100_fixed_clk,
        "anyka,ak3918av100-fixed-clk", of_ak_fixed_clk_init);
#elif defined(CONFIG_MACH_AK3918EV300L)
CLK_OF_DECLARE(ak3918ev300l_fixed_clk, "anyka,ak3918ev300l-fixed-clk",
        of_ak_fixed_clk_init);
#elif defined(CONFIG_MACH_AK3918AV130)
CLK_OF_DECLARE(ak3918av130_fixed_clk, "anyka,ak3918av130-fixed-clk",
        of_ak_fixed_clk_init);
#elif defined(CONFIG_MACH_KM01A)
CLK_OF_DECLARE(km01a_fixed_clk, "anyka,km01a-fixed-clk",
        of_ak_fixed_clk_init);
#endif

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
/*
 * /proc/clk
 */
#if defined(CONFIG_MACH_AK3918AV130)
static int proc_clk_show(struct seq_file* m, void* v)
{
    seq_printf(m, "------PLL------\n");    
    seq_printf(m, "%5s %10s\n", "pll", "freq(MHz)");
    seq_printf(m, "%5d %10lu\n", 1, ak_get_pll1_clk(AK_VA_SYSCTRL)/MHz);
    seq_printf(m, "%5d %10lu\n", 2, ak_get_pll2_clk(AK_VA_SYSCTRL)/MHz);
    seq_printf(m, "%5d %10lu\n", 3, ak_get_pll3_clk(AK_VA_SYSCTRL)/MHz);

    seq_printf(m, "------Module Clock------\n");    
    seq_printf(m, "%10s %10s\n","name" , "freq(Hz)");
    seq_printf(m, "%10s %10lu\n","SD_DAC" , sddac_get_clk(NULL, ak_get_pll2_clk(AK_VA_SYSCTRL)));
    seq_printf(m, "%10s %10lu\n","SFC_PHY" , sfc_phy_get_clk(NULL, ak_get_pll1_clk(AK_VA_SYSCTRL)));
    seq_printf(m, "%10s %10lu\n","GCLK" , gclk_get_clk(NULL, ak_get_pll2_clk(AK_VA_SYSCTRL)));
    seq_printf(m, "%10s %10lu\n", "ESS_VCLK",
        ess_vclk_get_clk(NULL, ak_get_pll1_clk(AK_VA_SYSCTRL),
            ak_get_pll2_clk(AK_VA_SYSCTRL), ak_get_pll3_clk(AK_VA_SYSCTRL)));
    seq_printf(m, "%10s %10lu\n", "NSS_VCLK",
        nss_vclk_get_clk(NULL, ak_get_pll1_clk(AK_VA_SYSCTRL),
            ak_get_pll2_clk(AK_VA_SYSCTRL), ak_get_pll3_clk(AK_VA_SYSCTRL)));
    seq_printf(m, "%10s %10lu\n", "ISS_VCLK",
        iss_vclk_get_clk(NULL, ak_get_pll1_clk(AK_VA_SYSCTRL),
            ak_get_pll2_clk(AK_VA_SYSCTRL), ak_get_pll3_clk(AK_VA_SYSCTRL)));

    return 0;
}
#else
static int proc_clk_show(struct seq_file* m, void* v)
{

    seq_printf(m,
            "pll2:   %lu MHz\n"
            "adc_rate:   %lu Hz\n"
            "dac_rate:   %lu Hz\n"
            "pll3:   %lu MHz\n",
            ak_get_pll2_clk(AK_VA_SYSCTRL) / MHz,
            sdadc_get_clk(NULL, ak_get_pll2_clk(AK_VA_SYSCTRL)),
            sddac_get_clk(NULL, ak_get_pll2_clk(AK_VA_SYSCTRL)),
            ak_get_pll3_clk(AK_VA_SYSCTRL) / MHz);

    return 0;
}
#endif

/*
 * clk_info_open
 */
static int clk_info_open(struct inode* inode, struct file* filp)
{
    return single_open(filp, proc_clk_show, NULL);
}

static const struct file_operations proc_clk_operations = {
    .open = clk_info_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = seq_release,
};

static int __init proc_clk_init(void)
{
    proc_create("clk", 0, NULL, &proc_clk_operations);
    return 0;
}
fs_initcall(proc_clk_init);
