/*
 * AK39ev330 clk driver
 *
 * Copyright (C) 2018 Anyka(Guangzhou) Microelectronics Technology Co., Ltd.
 *
 * Author: Feilong Dong <dong_feilong@anyka.com>
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

/**
 * DOC: AK39ev330 (clock resource manager)
 *
 * The clock tree on the AK39ev330 has several levels.  There's a root
 * oscillator running at 12Mhz. After the oscillator there are 3
 * PLLs, roughly divided as "CPU_PLL", "CORE_PLL" and "PERI_PLL".
 * Those 3 PLLs each can divide their output to produce several channels.
 * Finally, there is the level of clocks to be consumed by other hardware
 * components (like "isp" or "nethernet"), which divide off of some subset
 * of the PLL channels.
 *
 * All of the clocks in the tree are exposed in the DT, because the DT
 * may want to make assignments of the final layer of clocks to the
 * PLL channels, and some components of the hardware will actually
 * skip layers of the tree (for example, the sar_adc clock comes
 * directly from the root oscillator without using a clock divider
 * from PLL).
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

/* clock register offset */
#define CLOCK_CPU_PLL_CTRL     (0x04)
#define CLOCK_CORE_PLL_CTRL    (0x08)
#define CLOCK_ADC2_DAC_CTRL    (0x0C)
#define CLOCK_ADC2_DAC_HS_CTRL (0x10)
#define CLOCK_PERI_PLL_CTRL1   (0x14)
#define CLOCK_PERI_PLL_CTRL2   (0x18)
#define CLOCK_GATE_CTRL1       (0x1C)
#define CLOCK_SOFT_RESET       (0x20)
#define CLOCK_USB_I2S_CTRL     (0x58)
#define CLOCK_DAC_FADEOUT_CTRL (0x70)
#define ANALOG_CTRL_REG3       (0xA4)

/* ADC2/DAC clock control register */
#define AK_CLKCON_CLK_DAC  (1 << 28)
#define AK_CLKCON_CLK_ADC1 (1 << 3)

/* ADC2/DAC high speed clock control register */
#define AK_CLKCON_HCLK_ADC2 (1 << 28)
#define AK_CLKCON_HCLK_DAC  (1 << 18)
#define AK_CLKCON_CLK_ADC2  (1 << 8)

/* PERI PLL channel clock control register0 */
// 1: peri pll   0: external 12MHz
#define AK_CLKCON_CLK_PHY_SEL (1 << 19)
// 1: peri pll   0: external 25MHz
#define AK_CLKCON_CLK_MAC_SEL (1 << 18)
#define AK_CLKCON_CLK_12M     (1 << 17)
#define AK_CLKCON_CLK_25M     (1 << 16)
#define AK_CLKCON_CLK_25M_IN  (1 << 14)

/*  PERI PLL channel clock control register1
 *  This register is used to define CIS_PCLK, CIS_SCLK and OPCLK.
 *  Add: 0x0800,0018
 */
/* 1: positive clk  0: negative clk */
#define AK_CLKCON_PCLK_CIS  (1 << 28) // camera cis_pclk_en_cfg
#define AK_CLKCON_SCLK_CIS  (1 << 18) // sensor cis_sclk_en_cfg
#define AK_CLKCON_CLK_OPCLK (1 << 8) // MAC  opclk_en_cfg

#define AK39EV330_CLK_CPU2X4X_MODE    (1 << 24)
#define AK39EV330_CLK_CPU_DPHY_OPP_EN (1 << 26)
#define AK39EV330_CLK_DPHY1X2X_MODE   (1 << 30)

#define MHz 1000000UL

struct ak_fixed_clk {
    int id;
    struct clk_hw hw;
    void __iomem* reg;
    unsigned long fixed_rate;
};

struct ak_factor_clk {
    int id;
    struct clk_hw hw;
    void __iomem* reg;
    int m;
    int n;
};

struct ak_mux_clk {
    int id;
    struct clk_hw hw;
    void __iomem* reg;
    int select_index;
};

struct ak_gate_clk {
    int id;
    struct clk_hw hw;
    void __iomem* reg;
    int ctrlbit;
};

#define to_clk_ak_fixed(_hw)  container_of(_hw, struct ak_fixed_clk, hw)
#define to_clk_ak_factor(_hw) container_of(_hw, struct ak_factor_clk, hw)
#define to_clk_ak_mux(_hw)    container_of(_hw, struct ak_mux_clk, hw)
#define to_clk_ak_gate(_hw)   container_of(_hw, struct ak_gate_clk, hw)

#define SAMPLE_RATE_NUM  11
#define CORE_PLL_FRE_NUM 3

/*
***************************ADC******************************************
*采样率  core_pll_freq   adc             实际采样率         偏差 偏差比例
*                       clk_div osr
*8000   480000000       234     256     8012.820513     12.82051282     0.16%
*       560000000       136     512     8042.279412     42.27941176     0.53%
*       640000000       156     512     8012.820513     12.82051282     0.16%
*11025  480000000       170     256     11029.41176     4.411764706     0.04%
*       560000000       198     256     11047.9798      22.97979798     0.21%
*       640000000       226     256     11061.9469      36.94690265     0.34%
*12000  480000000       156     256     12019.23077     19.23076923     0.16%
*       560000000       182     256     12019.23077     19.23076923     0.16%
*       640000000       208     256     12019.23077     19.23076923     0.16%
*16000  480000000       117     256     16025.64103     25.64102564     0.16%
*       560000000       136     256     16084.55882     84.55882353     0.53%
*       640000000       156     256     16025.64103     25.64102564     0.16%
*22050  480000000       85      256     22058.82353     8.823529412     0.04%
*       560000000       99      256     22095.9596      45.95959596     0.21%
*       640000000       113     256     22123.89381     73.89380531     0.34%
*24000  480000000       78      256     24038.46154     38.46153846     0.16%
*       560000000       91      256     24038.46154     38.46153846     0.16%
*       640000000       104     256     24038.46154     38.46153846     0.16%
*32000  480000000       58      256     32327.58621     327.5862069     1.02%
*       560000000       68      256     32169.11765     169.1176471     0.53%
*       640000000       78      256     32051.28205     51.28205128     0.16%
*44100  480000000       42      256     44642.85714     542.8571429     1.23%
*       560000000       49      256     44642.85714     542.8571429     1.23%
*       640000000       56      256     44642.85714     542.8571429     1.23%
*48000  480000000       39      256     48076.92308     76.92307692     0.16%
*       560000000       45      256     48611.11111     611.1111111     1.27%
*       640000000       52      256     48076.92308     76.92307692     0.16%
*88202  480000000       21      256     89285.71429     1083.714286     1.23%
*       560000000       25      256     87500           -702            -0.80%
*       640000000       28      256     89285.71429     1083.714286     1.23%
*96000  480000000       19      256     98684.21053     2684.210526     2.80%
*       560000000       23      256     95108.69565     -891.3043478    -0.93%
*       640000000       26      256     96153.84615     153.8461538     0.16%
*
*
*****************************DAC******************************************
*采样率  core_pll_freq   dac
*                       clk_div osr     实际采样率         偏差 偏差比例 8000
*480000000       234     256     8012.820513     12.82051282     0.16% 560000000
*272     256     8042.279412     42.27941176     0.53% 640000000       312 256
*8012.820513     12.82051282     0.16% 11025  480000000       170     256
*11029.41176     4.411764706     0.04% 560000000       397     128 11020.15113
*-4.848866499    -0.04% 640000000       234     248 11028.39813     3.398125172
*0.03% 12000  480000000       147     272     12004.80192     4.801920768 0.04%
*       560000000       389     120     11996.57241     -3.427592117    -0.03%
*       640000000       202     264     12001.20012     1.200120012     0.01%
*16000  480000000       117     256     16025.64103     25.64102564     0.16%
*       560000000       136     256     16084.55882     84.55882353     0.53%
*       640000000       156     256     16025.64103     25.64102564     0.16%
*22050  480000000       85      256     22058.82353     8.823529412     0.04%
*       560000000       187     136     22019.50299     -30.49701164    -0.14%
*       640000000       117     248     22056.79625     6.796250345     0.03%
*24000  480000000       147     136     24009.60384     9.603841537     0.04%
*       560000000       94      248     24021.96294     21.96293754     0.09%
*       640000000       101     264     24002.40024     2.400240024     0.01%
*32000  480000000       125     120     32000   0       0.00%
*       560000000       73      240     31963.47032     -36.52968037    -0.11%
*       640000000       147     136     32012.80512     12.80512205     0.04%
*44100  480000000       40      272     44117.64706     17.64705882     0.04%
*       560000000       53      240     44025.15723     -74.8427673     -0.17%
*       640000000       55      264     44077.13499     -22.86501377    -0.05%
*48000  480000000       39      256     48076.92308     76.92307692     0.16%
*       560000000       47      248     48043.92588     43.92587509     0.09%
*       640000000       49      272     48019.20768     19.20768307     0.04%
*88202  480000000       20      272     88235.29412     33.29411765     0.04%
*       560000000       53      120     88050.31447     -151.6855346    -0.17%
*       640000000       57      128     87719.29825     -482.7017544    -0.55%
*96000  480000000       39      128     96153.84615     153.8461538     0.16%
*       560000000       43      136     95759.23393     -240.7660739    -0.25%
*       640000000       49      136     96038.41537     38.41536615     0.04%
*
*/

static unsigned short sdadc_DIV[CORE_PLL_FRE_NUM][SAMPLE_RATE_NUM] = {
    /*
     * 8000     11025   12000   16000   22050   24000   32000   44100   48000
     * 88202   96000
     */
    { 234, 170, 156, 117, 85, 78, 58, 42, 39, 21, 19 }, /*core_pll = 480M */
    { 136, 198, 182, 136, 99, 91, 68, 49, 45, 25, 23 }, /*core_pll = 560M */
    { 156, 226, 208, 156, 113, 104, 78, 56, 52, 28, 26 }, /*core_pll = 640M */
};
/*
 *  sddac_OSR only use 256, expect 8k sample @core_pll=560M and  8k sample
 * @core_pll=640M
 */
static unsigned short sdadc_OSR[CORE_PLL_FRE_NUM][SAMPLE_RATE_NUM] = {
    /*
     * 8000     11025   12000   16000   22050   24000   32000   44100   48000
     * 88202   96000
     */
    { 256, 256, 256, 256, 256, 256, 256, 256, 256, 256,
        256 }, /*core_pll = 480M */
    { 512, 256, 256, 256, 256, 256, 256, 256, 256, 256,
        256 }, /*core_pll = 560M */
    { 512, 256, 256, 256, 256, 256, 256, 256, 256, 256,
        256 }, /*core_pll = 640M */
};

static unsigned short sddac_DIV[CORE_PLL_FRE_NUM][SAMPLE_RATE_NUM] = {
    /*
     * 8000     11025   12000   16000   22050   24000   32000   44100   48000
     * 88202   96000
     */
    { 234, 170, 147, 117, 85, 147, 125, 40, 39, 20, 39 }, /*core_pll = 480M */
    { 272, 397, 389, 136, 187, 94, 73, 53, 47, 53, 43 }, /*core_pll = 560M */
    { 312, 234, 202, 156, 117, 101, 147, 55, 49, 57, 49 }, /*core_pll = 640M */
};

static unsigned short sddac_OSR[CORE_PLL_FRE_NUM][SAMPLE_RATE_NUM] = {
    /*
     * 8000     11025   12000   16000   22050   24000   32000   44100   48000
     * 88202   96000
     */
    { 256, 256, 272, 256, 256, 136, 120, 272, 256, 272,
        128 }, /*core_pll = 480M */
    { 256, 128, 120, 256, 136, 248, 240, 240, 248, 120,
        136 }, /*core_pll = 560M */
    { 256, 248, 264, 256, 248, 264, 136, 264, 272, 128,
        136 }, /*core_pll = 640M */
};

static unsigned int core_pll_freq[CORE_PLL_FRE_NUM] = {
    480,
    560,
    640,
};

static unsigned int sample_rate[SAMPLE_RATE_NUM] = { 8000, 11025, 12000, 16000,
    22050, 24000, 32000, 44100, 48000, 88202, 96000 };

/*
 * @BRIEF        ak_get_cpu_pll_clk
 * @PARAM[in]    reg
 * @RETURN       unsigned long
 * @RETVAL       cpu_pll_clk * MHz
 */
static unsigned long ak_get_cpu_pll_clk(void __iomem* reg)
{
    u32 pll_m, pll_n, pll_od;
    u32 cpu_pll_clk;
    u32 regval;

    regval = __raw_readl(reg + CLOCK_CPU_PLL_CTRL);
    pll_od = (regval & (0x3 << 12)) >> 12;
    pll_n = (regval & (0xf << 8)) >> 8;
    pll_m = regval & 0xff;

    cpu_pll_clk = 12 * pll_m / (pll_n * (1 << pll_od)); // clk unit: MHz

    return cpu_pll_clk * MHz;
}

/*
 * @BRIEF        ak_get_core_pll_clk
 * @PARAM[in]    reg
 * @RETURN       unsigned long
 * @RETVAL       core_pll_clk * MHz
 */
static unsigned long ak_get_core_pll_clk(void __iomem* reg)
{
    u32 pll_m, pll_n, pll_od;
    u32 core_pll_clk;
    u32 regval;

    regval = __raw_readl(reg + CLOCK_CORE_PLL_CTRL);
    pll_m = (regval & 0xff);
    pll_n = (regval >> 8) & 0xf;
    pll_od = (regval >> 12) & 0x3;
    core_pll_clk = 12 * pll_m / (pll_n * (1 << pll_od)); // clk unit: MHz

    pr_debug("pll_m=%d, pll_n=%d, pll_od=%d, core_pll_clk=%d MHz\n", pll_m,
        pll_n, pll_od, core_pll_clk);

    return core_pll_clk * MHz;
}

static unsigned long ak_get_vclk(void __iomem* reg)
{
    u32 regval;
    u32 vclk;
    regval = __raw_readl(reg + CLOCK_CORE_PLL_CTRL);
    regval = ((regval >> 17) & 0x7);
    if (regval == 0)
        regval = 1;
    vclk = (ak_get_core_pll_clk(reg) / MHz) >> (regval);

    return vclk * MHz;
}

static long ak_fixed_clk_round_rate(
    struct clk_hw* hw, unsigned long rate, unsigned long* parent_rate)
{
    struct ak_fixed_clk* fixed_clk = to_clk_ak_fixed(hw);

    pr_debug("%s %d id:%d, rate:%lu, parent_rate:%lu\n", __func__, __LINE__,
        fixed_clk->id, rate, *parent_rate);
    return rate;
}

static int ak_fixed_clk_set_rate(
    struct clk_hw* hw, unsigned long rate, unsigned long parent_rate)
{
    struct ak_fixed_clk* fixed_clk = to_clk_ak_fixed(hw);

    pr_debug("%s %d id:%d, rate:%lu, parent_rate:%lu\n", __func__, __LINE__,
        fixed_clk->id, rate, parent_rate);

    return 0;
}

static unsigned long ak_fixed_clk_recalc_rate(
    struct clk_hw* hw, unsigned long parent_rate)
{
    u32 regval;
    int div_j, div_d, div_phy;
    // get fixed clk rate from dts
    struct ak_fixed_clk* fixed_clk = to_clk_ak_fixed(hw);

    regval = __raw_readl(fixed_clk->reg + CLOCK_CPU_PLL_CTRL);
    if (regval & AK39EV330_CLK_CPU_DPHY_OPP_EN) {
        div_j = 0;
        div_d = 1;
        div_phy = 0;
    } else {
        if (regval & AK39EV330_CLK_CPU2X4X_MODE) {
            div_j = 0;
            div_d = 2;
        } else {
            div_j = 1;
            div_d = 2;
        }
        if (regval & AK39EV330_CLK_DPHY1X2X_MODE)
            div_phy = 1;
        else
            div_phy = 2;
    }
    pr_debug("div_j = %d, div_d = %d, div_phy = %d\n", div_j, div_d, div_phy);
    switch (fixed_clk->id) {
        case 0: // JCLK
            fixed_clk->fixed_rate = ak_get_cpu_pll_clk(fixed_clk->reg) >> div_j;
            pr_err("CPU(JCLK): %lu(Mhz) \n", fixed_clk->fixed_rate / MHz);
            break;
        case 1: // HCLK/DCLK
            fixed_clk->fixed_rate = ak_get_cpu_pll_clk(fixed_clk->reg) >> div_d;
            // pr_err("MEMCTRL(HCLK/DCLK): %lu(Mhz) \n",
            // fixed_clk->fixed_rate/MHz);
            break;
        case 2: // DPHY
            fixed_clk->fixed_rate
                = ak_get_cpu_pll_clk(fixed_clk->reg) >> div_phy;
            pr_err("MEMDDR2(DPHY): %lu(Mhz) \n", fixed_clk->fixed_rate / MHz);
            break;
        case 3: // CORE_PLL set in init func, just return
            fixed_clk->fixed_rate = ak_get_core_pll_clk(fixed_clk->reg);
            pr_debug("CORE_PLL: %lu(Mhz) \n", fixed_clk->fixed_rate / MHz);
        case 4: // PERI_PLL set in init func, just return
            break;
        default:
            pr_err("No this clk config in dts.\n");
            break;
    }

    return fixed_clk->fixed_rate;
}

const struct clk_ops ak_fixed_clk_ops = {
    .round_rate = ak_fixed_clk_round_rate,
    .set_rate = ak_fixed_clk_set_rate,
    .recalc_rate = ak_fixed_clk_recalc_rate,
};

static int sdadc_sddac_calc_div_osr(int is_sdadc, unsigned long rate,
    unsigned long* parent_rate, unsigned int* div, unsigned int* osr)
{
    int sample_index;
    int sample_num;
    int clk_freq_index;
    int clk_freq_num;

    pr_debug("%s %d %s rate:%lu, parent_rate:%lu\n", __func__, __LINE__,
        is_sdadc ? "sdadc" : "sddac", rate, *parent_rate);

    sample_num = sizeof(sample_rate) / sizeof(sample_rate[0]);

    for (sample_index = 0; sample_index < sample_num; sample_index++) {
        if (rate == sample_rate[sample_index]) {
            break;
        }
    }

    if (sample_index >= sample_num) {
        pr_err("%s %d rate:%lu failed\n", __func__, __LINE__, rate);
        return -1;
    }

    clk_freq_num = sizeof(core_pll_freq) / sizeof(core_pll_freq[0]);

    for (clk_freq_index = 0; clk_freq_index < clk_freq_num; clk_freq_index++) {
        if ((*parent_rate) / MHz == core_pll_freq[clk_freq_index]) {
            break;
        }
    }

    if (clk_freq_index >= clk_freq_num) {
        pr_err(
            "%s %d parent_rate:%lu failed\n", __func__, __LINE__, *parent_rate);
        return -1;
    }

    if (is_sdadc) {
        *div = sdadc_DIV[clk_freq_index][sample_index];
        *osr = sdadc_OSR[clk_freq_index][sample_index];
    } else {
        *div = sddac_DIV[clk_freq_index][sample_index];
        *osr = sddac_OSR[clk_freq_index][sample_index];
    }

    pr_info("[%s]core_pll=%lu, des_sample_rate=%lu, div=%d, osr=%d, "
            "actual_sample_rate=%lu\n",
        is_sdadc ? "sdadc" : "sddac", *parent_rate, rate, *div, *osr,
        (*parent_rate) / (*div) / (*osr));
    return 0;
}

static int sdadc_set_div_osr(struct ak_factor_clk* factor_clk,
    unsigned long rate, unsigned long* parent_rate)
{
    u32 div, osr, map_osr;
    int timeout;
    u32 regval;

    if (sdadc_sddac_calc_div_osr(1, rate, parent_rate, &div, &osr)) {
        return -1;
    }

    if (osr == 512)
        map_osr = 0x0;
    else if (osr == 256)
        map_osr = 0x1;
    else {
        pr_err("%s %d rate:%lu failed, div:%d, osr:%d\n", __func__, __LINE__,
            rate, div, osr);
        return -1;
    }

    /*reset adc from adc clk*/
    regval = __raw_readl(factor_clk->reg + CLOCK_SOFT_RESET);
    regval &= ~(1 << 27);
    __raw_writel(regval, factor_clk->reg + CLOCK_SOFT_RESET);

    /*disable sd_adc_clk*/
    regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);
    regval &= ~(1 << 8); /*sdadc_clk_en_cfg*/
    __raw_writel(regval, factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);

    /*change the div val*/
    regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);
    regval &= ~(0xff << 0); /*sdadc_clk_div_num_cfg[7:0]*/
    regval |= ((0xFF & (div - 1)) << 0);
    __raw_writel(regval, factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);

    /*set sdadc_div_vld_cfg*/
    regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);
    regval |= (1 << 9); /*sdadc_div_vld_cfg*/
    __raw_writel(regval, factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);

    /*wait the sddac_div_vld_cfg clear*/
    timeout = 100000;
    do {
        timeout--;
        regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL)
            & (1 << 9); /*sdadc_div_vld_cfg*/
    } while (regval && (timeout > 0));
    if (timeout <= 0) {
        pr_err("%s %d wait sddac_div timeout\n", __func__, __LINE__);
    }

    /*open adc fillter clk gate*/
    regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);
    regval |= (1 << 8); /*sdadc_clk_en _cfg*/
    __raw_writel(regval, factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);

    /*release the reset adc from adc clk*/
    regval = __raw_readl(factor_clk->reg + CLOCK_SOFT_RESET);
    regval |= (1 << 27);
    __raw_writel(regval, factor_clk->reg + CLOCK_SOFT_RESET);

    /*set osr*/
    regval = __raw_readl(factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);
    regval &= ~(0x1UL << 31);
    regval |= ((map_osr & 0x1UL) << 31);
    __raw_writel(regval, factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);

    return 0;
}

static int sddac_set_div_osr(struct ak_factor_clk* factor_clk,
    unsigned long rate, unsigned long* parent_rate)
{
    int div;
    int osr;
    int map_osr;
    int timeout;
    u32 regval;

    if (sdadc_sddac_calc_div_osr(0, rate, parent_rate, &div, &osr)) {
        return -1;
    }

    if (osr == 256)
        map_osr = 0x0;
    else if (osr == 272)
        map_osr = 0x1;
    else if (osr == 264)
        map_osr = 0x2;
    else if (osr == 248)
        map_osr = 0x3;
    else if (osr == 240)
        map_osr = 0x4;
    else if (osr == 136)
        map_osr = 0x5;
    else if (osr == 128)
        map_osr = 0x6;
    else if (osr == 120)
        map_osr = 0x7;
    else {
        pr_err("%s %d rate:%lu failed, div:%d, osr:%d\n", __func__, __LINE__,
            rate, div, osr);
        return -1;
    }

    /*close the dac_filter_en_cfg*/
    regval = __raw_readl(factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);
    regval &= ~(1 << 3); /*dac_filter_en*/
    __raw_writel(regval, factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);

    /*reset dac from dac clk*/
    regval = __raw_readl(factor_clk->reg + CLOCK_SOFT_RESET);
    regval &= ~(1 << 26); /*sddac_rst_cfg*/
    __raw_writel(regval, factor_clk->reg + CLOCK_SOFT_RESET);

    /*To disable sd_dac_clk*/
    regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_CTRL);
    regval &= ~(1 << 28); /*sddac_clk_en_cfg*/
    __raw_writel(regval, factor_clk->reg + CLOCK_ADC2_DAC_CTRL);

    /*change the div val*/
    regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_CTRL);

    /* bit[12:4] = sddac_div_num_int_cfg[8:0]; bit[24:13] =
     * sddac_div_num_int_cfg[11:0], let sddac_div_num_int_cfg[11:0] = 0
     */
    regval &= ~((0x1FF << 4) | (0xFFF << 13));
    regval |= ((0x1FF & (div - 1)) << 4);
    __raw_writel(regval, factor_clk->reg + CLOCK_ADC2_DAC_CTRL);

    /*set sddac_div_vld_cfg*/
    regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_CTRL);
    regval |= (1 << 29); /*bit[29] = sddac_div_vld_cfg*/
    __raw_writel(regval, factor_clk->reg + CLOCK_ADC2_DAC_CTRL);

    /*wait the sddac_div_vld_cfg  clear*/
    timeout = 100000;
    do {
        timeout--;
        regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_CTRL) & (1 << 29);
    } while (regval && (timeout > 0));
    if (timeout <= 0) {
        pr_err("%s %d wait sddac_div timeout\n", __func__, __LINE__);
    }

    /*To enable sd_dac_clk*/
    regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_CTRL);
    regval |= (1 << 28); /*bit[28] = sddac_clk_en_cfg*/
    __raw_writel(regval, factor_clk->reg + CLOCK_ADC2_DAC_CTRL);

    /*release the reset dac from dac clk*/
    regval = __raw_readl(factor_clk->reg + CLOCK_SOFT_RESET);
    regval |= (1 << 26);
    __raw_writel(regval, factor_clk->reg + CLOCK_SOFT_RESET);

    /*set osr*/
    regval = __raw_readl(factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);
    regval &= ~(0x7);
    regval |= map_osr & 0x7;
    __raw_writel(regval, factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);

    /*enable the dac_filter_en_cfg*/
    regval = __raw_readl(factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);
    regval |= (1 << 3); /*dac_filter_en*/
    __raw_writel(regval, factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);

    return 0;
}
/*end of func*/

static int adchs_set_div(struct ak_factor_clk* factor_clk, int factor)
{
    int timeout;
    u32 regval;

    /*reset adc from adc hsclk*/
    regval = __raw_readl(factor_clk->reg + CLOCK_SOFT_RESET);
    regval &= ~(1 << 29);
    __raw_writel(regval, factor_clk->reg + CLOCK_SOFT_RESET);

    /*wait the sdadc_hsdiv_vld_cfg  clear*/
    timeout = 100000;
    do {
        timeout--;
        regval
            = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL) & (1 << 29);
    } while (regval && (timeout > 0));
    if (timeout <= 0) {
        pr_err("%s %d wait sdadc hsdiv timeout\n", __func__, __LINE__);
    }

    /*disable the sd_adc_hsclk*/
    regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);
    regval &= ~(1 << 28);
    __raw_writel(regval, factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);

    /*change the div val*/
    regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);
    regval &= ~(0xff << 20);
    regval |= ((0xff & factor) << 20);
    __raw_writel(regval, factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);

    /*set sddac_hsdiv_vld_cfg*/
    regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);
    regval |= (1 << 29);
    __raw_writel(regval, factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);

    /*wait the sdadc_div_vld_cfg clear*/
    timeout = 100000;
    do {
        timeout--;
        regval
            = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL) & (1 << 29);
    } while (regval && (timeout > 0));
    if (timeout <= 0) {
        pr_err("%s %d wait sdadc div timeout\n", __func__, __LINE__);
    }

    /*enable the sd_adc_hsclk*/
    regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);
    regval |= (1 << 28);
    __raw_writel(regval, factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);

    /*release adc from adc hsclk*/
    regval = __raw_readl(factor_clk->reg + CLOCK_SOFT_RESET);
    regval |= (1 << 29);
    __raw_writel(regval, factor_clk->reg + CLOCK_SOFT_RESET);

    return 0;
}

static int dachs_set_div(struct ak_factor_clk* factor_clk, int factor)
{
    int timeout;
    u32 regval;

    /*close the dac_filter_en_cfg*/
    regval = __raw_readl(factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);
    regval &= ~(1 << 3);
    __raw_writel(regval, factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);

    /*reset dac from dac hsclk*/
    regval = __raw_readl(factor_clk->reg + CLOCK_SOFT_RESET);
    regval &= ~(1 << 28);
    __raw_writel(regval, factor_clk->reg + CLOCK_SOFT_RESET);

    /*wait the sddac_hsdiv_vld_cfg  clear*/
    timeout = 100000;
    do {
        timeout--;
        regval
            = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL) & (1 << 19);
    } while (regval && (timeout > 0));
    if (timeout <= 0) {
        pr_err("%s %d wait sddac hsdiv timeout\n", __func__, __LINE__);
    }

    /*disable the sd_dac_hsclk*/
    regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);
    regval &= ~(1 << 18);
    __raw_writel(regval, factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);

    /*change the div val*/
    regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);
    regval &= ~(0xff << 10);
    regval |= ((0xff & factor) << 10);
    __raw_writel(regval, factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);

    /*set sddac_hsdiv_vld_cfg*/
    regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);
    regval |= (1 << 19);
    __raw_writel(regval, factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);

    /*wait the sddac_div_vld_cfg clear*/
    timeout = 100000;
    do {
        timeout--;
        regval
            = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL) & (1 << 19);
    } while (regval && (timeout > 0));
    if (timeout <= 0) {
        pr_err("%s %d wait sddac div timeout\n", __func__, __LINE__);
    }

    /*enable the sd_dac_hsclk*/
    regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);
    regval |= (1 << 18);
    __raw_writel(regval, factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);

    /*release dac from dac hsclk*/
    regval = __raw_readl(factor_clk->reg + CLOCK_SOFT_RESET);
    regval |= (1 << 28);
    __raw_writel(regval, factor_clk->reg + CLOCK_SOFT_RESET);

    /*enable the dac_filter_en_cfg*/
    regval = __raw_readl(factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);
    regval |= (1 << 3); /*dac_filter_en*/
    __raw_writel(regval, factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);

    return 0;
}

static u32 dacchs_get_factor(unsigned long rate, unsigned long parent_rate)
{
    u32 factor;

    /* parmeter used by dachs calcurate */
    factor = parent_rate / rate;

    /*when the divider of the DAC CLK is even number,the DAC MODULE is more
     * steady*/
    factor &= ~1;

    if (0 != factor) {
        factor = factor - 1;
    }

    return factor;
}

static long ak_factor_clk_round_rate(
    struct clk_hw* hw, unsigned long rate, unsigned long* parent_rate)
{
    int ret = 0;
    u32 regval;
    u32 factor;
    unsigned long flags;

    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);

    pr_debug("%s %d id:%d\n", __func__, __LINE__, factor_clk->id);
    local_irq_save(flags);
    factor = (*parent_rate) / rate - 1;
    /* calculate the factor value, and set the proper reg bits */
    switch (factor_clk->id) {
        case 0: // adchs
            adchs_set_div(factor_clk, factor);
            break;

        case 1: // dachs
            dachs_set_div(factor_clk, dacchs_get_factor(rate, *parent_rate));
            break;

        case 2: // csi_sclk
            regval = __raw_readl(factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            regval &= ~(0x1 << 18);
            __raw_writel(regval, factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            regval = __raw_readl(factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            regval &= ~(0x3F << 10);
            regval |= ((0xFF & factor) << 10);
            __raw_writel(regval, factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            break;

        case 3: // csi_pclk
            regval = __raw_readl(factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            regval &= ~(0x1 << 28);
            __raw_writel(regval, factor_clk->reg + CLOCK_PERI_PLL_CTRL2);

            regval = __raw_readl(factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            regval &= ~(0x3F << 20);
            regval |= ((0x3F & factor) << 20);
            __raw_writel(regval, factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            break;
        case 4: // dsi_pclk
            /* config LCD PCLK in LCD driver, this function no use here */
            /* refer to 0x200100BC */
            break;
        case 5: // opclk
            /*opclk clk is fixed to 50MHz*/
            break;
        case 6: // usb_phy
            /* usb phy clk fixed to 12MHz */
            break;
        case 7: // sdadc
            if (sdadc_set_div_osr(factor_clk, rate, parent_rate))
                ret = -1;
            break;
        case 8: // sddac
            if (sddac_set_div_osr(factor_clk, rate, parent_rate))
                ret = -1;
            break;
        case 9: // saradc
            regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_CTRL);
            regval &= ~(0x1 << 3); /*bit[3] = saradc_clk_en_cfg*/
            __raw_writel(regval, factor_clk->reg + CLOCK_ADC2_DAC_CTRL);

            regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_CTRL);
            regval &= ~(0x7); /*bit[2:0] = saradc_clk_div_num_cfg */
            regval |= (0x7 & factor);
            __raw_writel(regval, factor_clk->reg + CLOCK_ADC2_DAC_CTRL);
            break;
        default:
            pr_err("No this factor clk config in dts.\n");
            break;
    }
    local_irq_restore(flags);

    return ret;
}

static int ak_factor_clk_set_rate(
    struct clk_hw* hw, unsigned long rate, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 regval;
    unsigned long flags;

    pr_debug(
        "%s %d id:%d, rate:%lu\n", __func__, __LINE__, factor_clk->id, rate);

    local_irq_save(flags);
    /* set the enable and valid config reg bits to enable the clk */
    switch (factor_clk->id) {
        case 0: // adchs
            break;
        case 1: // dachs
            break;

        case 2: // csi_sclk
            regval = __raw_readl(factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            regval |= (0x1 << 19);
            __raw_writel(regval, factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            regval |= (0x1 << 18);
            __raw_writel(regval, factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            break;

        case 3: // csi_pclk
            regval = __raw_readl(factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            regval |= (0x1 << 29);
            __raw_writel(regval, factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            regval |= (0x1 << 28);
            __raw_writel(regval, factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            break;
        case 4: // dsi_pclk
            /*config enable in LCD driver */
            break;
        case 5: // opclk
            /* config as fixed 50MHz */
            regval = __raw_readl(factor_clk->reg + CLOCK_PERI_PLL_CTRL1);
            regval &= ~(0x1 << 22); // first set mac interface select Rmii
            __raw_writel(regval, factor_clk->reg + CLOCK_PERI_PLL_CTRL1);
            regval |= (0x1 << 23); // mac_speed_cfg=1(100m)
            __raw_writel(regval, factor_clk->reg + CLOCK_PERI_PLL_CTRL1);
            regval |= (0x1 << 21); // set bit[21],enable generate 50m
            __raw_writel(regval, factor_clk->reg + CLOCK_PERI_PLL_CTRL1);
            regval |= (0x1 << 18
                | 0x1 << 28); // set bit[28],enable generate 50m, bit[18],
                              // select 25m clock of mac from pll div
            __raw_writel(regval, factor_clk->reg + CLOCK_PERI_PLL_CTRL1);
            break;
        case 6: // usb_phy
            /* config fixed to 12MHz */
            break;
        case 7: // sdadc

            break;
        case 8: // sddac

            break;
        case 9: // saradc
            regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_CTRL);
            regval |= (0x1 << 3); /*bit[3] = saradc_clk_en_cfg*/
            __raw_writel(regval, factor_clk->reg + CLOCK_ADC2_DAC_CTRL);
            break;
        default:
            pr_err("No this factor clk config in dts.\n");
            break;
    }
    local_irq_restore(flags);
    return 0;
}

static unsigned long sdadc_get_clk(
    struct ak_factor_clk* factor_clk, unsigned long parent_rate)
{
    u32 div, osr;
    unsigned int regval_div, regval_osr;
    unsigned long sdadc_clk;
    void __iomem* reg = factor_clk ? (factor_clk->reg) : AK_VA_SYSCTRL;

    regval_div = __raw_readl(reg + CLOCK_ADC2_DAC_HS_CTRL);
    div = (regval_div & 0xff) + 1; /* bit[7:0] = sdadc_clk_div_num_cfg[7:0] */

    regval_osr = __raw_readl(reg + CLOCK_DAC_FADEOUT_CTRL);
    osr = (regval_osr & (1UL << 31)) ? 256 : 512;

    sdadc_clk = parent_rate / div / osr;

    pr_debug("%s %d id:%d, parent_rate:%lu,"
             "regval_div:%x, regval_osr:%x, sddca_clk:%lu\n",
        __func__, __LINE__, factor_clk ? (factor_clk->id) : 0xffff, parent_rate,
        regval_div, regval_osr, sdadc_clk);
    return sdadc_clk;
}

static unsigned long sddac_get_clk(
    struct ak_factor_clk* factor_clk, unsigned long parent_rate)
{
    int div, osr, div_frac;
    unsigned int regval_div, regval_osr;
    unsigned long sddac_clk;
    void __iomem* reg = factor_clk ? (factor_clk->reg) : AK_VA_SYSCTRL;

    regval_div = __raw_readl(reg + CLOCK_ADC2_DAC_CTRL);
    div = ((regval_div >> 4) & 0x1ff)
        + 1; /*bit[12:4] = sddac_div_num_int_cfg[8:0]*/

    regval_div = __raw_readl(reg + CLOCK_ADC2_DAC_CTRL);
    div_frac = (regval_div >> 13)
        & 0xfff; /*bit[24:13] = sddac_div_num_int_cfg[11:0]*/
    div += (div_frac / 10);

    regval_osr = __raw_readl(reg + CLOCK_DAC_FADEOUT_CTRL);
    switch (regval_osr & 0x7) {
        case 0x0:
            osr = 256;
            break;

        case 0x1:
            osr = 272;
            break;

        case 0x2:
            osr = 264;
            break;

        case 0x3:
            osr = 248;
            break;

        case 0x4:
            osr = 240;
            break;

        case 0x5:
            osr = 136;
            break;

        case 0x6:
            osr = 128;
            break;

        case 0x7:
            osr = 120;
            break;

        default:
            osr = 256;
            break;
    }

    sddac_clk = parent_rate / div / osr;

    pr_debug("%s %d id:%d, parent_rate:%lu,"
             "regval_div:%x, regval_osr:%x, sddac_clk:%lu\n",
        __func__, __LINE__, factor_clk ? (factor_clk->id) : 0xffff, parent_rate,
        regval_div, regval_osr, sddac_clk);
    return sddac_clk;
}

static unsigned long ak_factor_clk_recalc_rate(
    struct clk_hw* hw, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 regval;
    u32 factor;

    pr_debug("%s %d id:%d, parent_rate:%lu\n", __func__, __LINE__,
        factor_clk->id, parent_rate);
    /* return the actural clk rate of the clk from the related reg bits */
    switch (factor_clk->id) {
        case 0: // adchs
            regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);
            factor = (regval >> 20) & 0xFF;
            break;
        case 1: // dachs
            regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_HS_CTRL);
            factor = (regval >> 10) & 0xFF;
            break;

        case 2: // csi_sclk
            regval = __raw_readl(factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            factor = (regval >> 10) & 0x3F;
            break;

        case 3: // csi_pclk
            regval = __raw_readl(factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            factor = (regval >> 20) & 0x3F;
            break;
        case 4: // dsi_pclk
            /*config enable in LCD driver */
            break;
        case 5: // opclk
            /* fixed to 50MHz */
            factor = 12 - 1;
            break;
        case 6: // usb_phy
            /* fixed to 12MHz */
            factor = 50 - 1;
            break;
        case 7: // sdadc
            return sdadc_get_clk(factor_clk, parent_rate);
            break;
        case 8: // sddac
            return sddac_get_clk(factor_clk, parent_rate);
            break;
        case 9: // saradc
            break;
        default:
            pr_err("No this factor clk config in dts.\n");
            break;
    }

    return parent_rate / (factor + 1);
}

const struct clk_ops ak_factor_clk_ops = {
    .round_rate = ak_factor_clk_round_rate,
    .set_rate = ak_factor_clk_set_rate,
    .recalc_rate = ak_factor_clk_recalc_rate,
};

static int ak_mux_clk_set_parent(struct clk_hw* hw, u8 index)
{
    // struct ak_client_clk *client_clk = to_clk_ak_mux(hw);
    /* to do */
    return 0;
}

static u8 ak_mux_clk_get_parent(struct clk_hw* hw)
{
    // struct ak_client_clk *client_clk = to_clk_ak_mux(hw);
    /* to do */
    return 0;
}

const struct clk_ops ak_mux_clk_ops = {
    .set_parent = ak_mux_clk_set_parent, .get_parent = ak_mux_clk_get_parent,
    //.determine_rate = __clk_mux_determine_rate,
};

static int ak_gc_enable(struct clk_hw* hw)
{
    struct ak_gate_clk* gate_clk = to_clk_ak_gate(hw);
    u32 con;
    unsigned long flags;

    local_irq_save(flags);
    if (gate_clk->reg == AK_VA_SYSCTRL) {
        con = __raw_readl(gate_clk->reg + CLOCK_GATE_CTRL1);
        con &= ~(1 << gate_clk->ctrlbit);
        __raw_writel(con, gate_clk->reg + CLOCK_GATE_CTRL1);

        con = __raw_readl(gate_clk->reg + CLOCK_SOFT_RESET);
        con |= (0x1 << gate_clk->ctrlbit);
        __raw_writel(con, gate_clk->reg + CLOCK_SOFT_RESET);
        mdelay(1);
        con = __raw_readl(gate_clk->reg + CLOCK_SOFT_RESET);
        con &= ~(0x1 << gate_clk->ctrlbit);
        __raw_writel(con, gate_clk->reg + CLOCK_SOFT_RESET);
    }

    local_irq_restore(flags);
    return 0;
}

static void ak_gc_disable(struct clk_hw* hw)
{
    struct ak_gate_clk* gate_clk = to_clk_ak_gate(hw);
    u32 con;
    unsigned long flags;

    local_irq_save(flags);
    if (gate_clk->reg == AK_VA_SYSCTRL) {
        con = __raw_readl(gate_clk->reg + CLOCK_GATE_CTRL1);
        con |= (1 << gate_clk->ctrlbit);
        __raw_writel(con, gate_clk->reg + CLOCK_GATE_CTRL1);
    }

    local_irq_restore(flags);
}

static unsigned long ak_gc_recalc_rate(
    struct clk_hw* hw, unsigned long parent_rate)
{
    struct ak_gate_clk* gate_clk = to_clk_ak_gate(hw);
    unsigned long rate = 0;
    u32 regval = __raw_readl(gate_clk->reg + CLOCK_CORE_PLL_CTRL);

    regval = ((regval >> 17) & 0x7);
    if (regval == 0)
        regval = 1;

    if (gate_clk->id >= 0 && gate_clk->id <= 28) {
        switch (gate_clk->id) {
            case 18:
            case 19:
            case 20:
            case 21:
            case 24:
                rate = parent_rate >> (regval);
                break;
            default:
                rate = parent_rate >> (regval);
                regval = __raw_readl(gate_clk->reg + CLOCK_CORE_PLL_CTRL);
                regval = ((regval >> 24) & 0x1);
                if (regval)
                    rate >>= 1;
                break;
        }
    }

    return rate;
}

const struct clk_ops ak_gate_clk_ops = {
    .enable = ak_gc_enable,
    .disable = ak_gc_disable,
    .recalc_rate = ak_gc_recalc_rate,
};

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

struct clk* __init ak_register_factor_clk(const char* name,
    const char* parent_name, void __iomem* res_reg, int id,
    const struct clk_ops* ops)
{
    struct ak_factor_clk* factor_clk;
    struct clk* clk;
    struct clk_init_data init = {};

    factor_clk = kzalloc(sizeof(*factor_clk), GFP_KERNEL);
    if (!factor_clk)
        return ERR_PTR(-ENOMEM);

    init.name = name;
    if (!strcmp(name, "clk_sdadc") || !strcmp(name, "clk_sddac"))
        init.flags = CLK_SET_RATE_PARENT;
    else
        init.flags = CLK_IGNORE_UNUSED;
    init.parent_names = parent_name ? &parent_name : NULL;
    init.num_parents = 1;
    init.ops = ops;

    factor_clk->reg = res_reg;
    factor_clk->id = id;
    factor_clk->hw.init = &init;

    clk = clk_register(NULL, &factor_clk->hw);
    if (IS_ERR(clk))
        kfree(factor_clk);

    return clk;
}

struct clk* __init ak_register_mux_clk(const char* name,
    const char* const* parent_names, int num_parents, void __iomem* res_reg,
    int id, const struct clk_ops* ops)
{
    struct ak_mux_clk* mux_clk;
    struct clk* clk;
    struct clk_init_data init = {};

    mux_clk = kzalloc(sizeof(*mux_clk), GFP_KERNEL);
    if (!mux_clk)
        return ERR_PTR(-ENOMEM);

    init.name = name;
    init.flags = CLK_SET_RATE_PARENT;
    init.parent_names = parent_names;
    init.num_parents = num_parents;
    init.ops = ops;

    mux_clk->id = id;
    mux_clk->reg = res_reg;
    mux_clk->hw.init = &init;

    clk = clk_register(NULL, &mux_clk->hw);
    if (IS_ERR(clk))
        kfree(mux_clk);

    return clk;
}

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

void core_pll_init(void __iomem* reg, u32 core_pll, u32 div_od, u32 div_n)
{
    u32 div_m;
    u32 regval;

    if ((div_n < 1) || (div_n > 12))
        panic("CORE pll frequency parameter Error");

    /* core_pll = 480M=2vclk; vclk=240M=4gclk; gclk=120M */
    div_m = ((core_pll / MHz) * (div_n * (1 << div_od))) / 12;
    if (div_m < 2)
        panic("CORE pll frequency parameter Error");
    /* set core pll frequency */
    regval = ((1 << 24) | (1 << 23) | (1 << 17) | (div_od << 12) | (div_n << 8)
        | (div_m));
    __raw_writel(regval, reg + CLOCK_CORE_PLL_CTRL);

    /* enable core pll freq change valid */
    regval = __raw_readl(reg + CLOCK_CPU_PLL_CTRL);
    regval |= (1 << 28);
    __raw_writel(regval, reg + CLOCK_CPU_PLL_CTRL);

    pr_err("VCLK: %lu(Mhz)\n", ak_get_vclk(reg) / MHz);
}

void peri_pll_init(void __iomem* reg, u32 peri_pll, u32 div_od, u32 div_n)
{
    u32 div_m;
    u32 regval;

    if ((div_n < 2) || (div_n > 6) || (div_od < 1) || (div_od > 3))
        panic("peril pll frequency parameter Error");

    /* peri_pll = 600M */
    div_m = ((peri_pll / MHz) * (div_n * (1 << div_od))) / 12;
    /* set peri pll frequency */
    regval = __raw_readl(reg + CLOCK_PERI_PLL_CTRL1);
    regval &= ~((0x3 << 12) | (0xf << 8) | 0xff);
    regval |= ((div_od << 12) | (div_n << 8) | (div_m));
    __raw_writel(regval, reg + CLOCK_PERI_PLL_CTRL1);

    /* enable peri pll freq change valid */
    regval = __raw_readl(reg + CLOCK_CPU_PLL_CTRL);
    regval |= (1 << 29);
    __raw_writel(regval, reg + CLOCK_CPU_PLL_CTRL);
}

static void __init of_ak_fixed_clk_init(struct device_node* np)
{
    struct clk_onecell_data* clk_data;
    const char* clk_name = np->name;
    const char* parent_name = of_clk_get_parent_name(np, 0);
    void __iomem* res_reg = AK_VA_SYSCTRL;
    u32 rate, div_od, div_n;
    int id, index, number;

    clk_data = kmalloc(sizeof(struct clk_onecell_data), GFP_KERNEL);
    if (!clk_data)
        return;

    number = of_property_count_u32_elems(np, "clock-id");
    clk_data->clks = kcalloc(number, sizeof(struct clk*), GFP_KERNEL);
    if (!clk_data->clks)
        goto err_free_data;

    of_property_read_u32(np, "clock-frequency", &rate);
    of_property_read_u32(np, "clock-div-od", &div_od);
    of_property_read_u32(np, "clock-div-n", &div_n);

    if (!strcmp(clk_name, "core_pll")) {
        core_pll_init(res_reg, rate, div_od, div_n);
    }
    if (!strcmp(clk_name, "peri_pll")) {
        peri_pll_init(res_reg, rate, div_od, div_n);
    }

    for (index = 0; index < number; index++) {
        of_property_read_string_index(
            np, "clock-output-names", index, &clk_name);
        of_property_read_u32_index(np, "clock-id", index, &id);
        pr_debug("of_ak_fixed_clk_init %s %s %d %d\n", clk_name, parent_name,
            rate, id);
        clk_data->clks[index] = ak_register_fixed_clk(
            clk_name, parent_name, res_reg, rate, id, &ak_fixed_clk_ops);
        if (IS_ERR(clk_data->clks[index])) {
            pr_err("of_ak_fixed_clk_init register fixed clk failed: "
                   "clk_name:%s, index = %d\n",
                clk_name, index);
            WARN_ON(true);
            continue;
        }
        clk_register_clkdev(clk_data->clks[index], clk_name, NULL);
    }

    clk_data->clk_num = number;
    if (number == 1)
        of_clk_add_provider(np, of_clk_src_simple_get, clk_data->clks[0]);
    else
        of_clk_add_provider(np, of_clk_src_onecell_get, clk_data);

    return;

err_free_data:
    kfree(clk_data);
}

static void __init of_ak_factor_clk_init(struct device_node* np)
{
    struct clk_onecell_data* clk_data;
    const char* clk_name = np->name;
    const char* parent_name = of_clk_get_parent_name(np, 0);
    void __iomem* res_reg = AK_VA_SYSCTRL;
    // u32 div_n, div_m;
    int id, index, number;

    clk_data = kmalloc(sizeof(struct clk_onecell_data), GFP_KERNEL);
    if (!clk_data)
        return;

    number = of_property_count_u32_elems(np, "clock-id");
    clk_data->clks = kcalloc(number, sizeof(struct clk*), GFP_KERNEL);
    if (!clk_data->clks)
        goto err_free_data;

    // of_property_read_u32(np, "clock-div-n", &div_n);
    // of_property_read_u32(np, "clock-div-m", &div_m);

    for (index = 0; index < number; index++) {
        of_property_read_string_index(
            np, "clock-output-names", index, &clk_name);
        of_property_read_u32_index(np, "clock-id", index, &id);
        pr_debug("of_ak_factor_clk_init %s %s %d %d\n", clk_name, parent_name,
            id, index);
        clk_data->clks[index] = ak_register_factor_clk(
            clk_name, parent_name, res_reg, id, &ak_factor_clk_ops);
        if (IS_ERR(clk_data->clks[index])) {
            pr_err("of_ak_factor_clk_init register gate clk failed: "
                   "clk_name:%s, err = %s\n",
                clk_name, (char*)PTR_ERR(clk_data->clks[index]));
            WARN_ON(true);
            continue;
        }
        // clk_register_clkdev(clk_data->clks[index], clk_name, NULL);
    }

    clk_data->clk_num = number;
    if (number == 1)
        of_clk_add_provider(np, of_clk_src_simple_get, clk_data->clks[0]);
    else
        of_clk_add_provider(np, of_clk_src_onecell_get, clk_data);

    return;

err_free_data:
    kfree(clk_data);
}

static void __init of_ak_mux_clk_init(struct device_node* np)
{
    struct clk* clk = NULL;
    const char* clk_name = np->name;
    const char* parent_name[2];
    void __iomem* res_reg = AK_VA_SYSCTRL;
    int id, number;

    number = of_clk_parent_fill(np, parent_name, 2);
    of_property_read_string(np, "clock-output-names", &clk_name);
    of_property_read_u32(np, "clock-id", &id);

    // pr_err("of_ak_mux_clk_init %s %s %s %d\n", clk_name, parent_name[0],
    // parent_name[1], id);
    clk = ak_register_mux_clk(
        clk_name, parent_name, number, res_reg, id, &ak_mux_clk_ops);
    if (!IS_ERR(clk)) {
        of_clk_add_provider(np, of_clk_src_simple_get, clk);
    } else
        pr_err("of_ak_mux_clk_init register mux clk failed: clk_name:%s %s \n",
            clk_name, (char*)PTR_ERR(clk));
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
    of_clk_add_provider(np, of_clk_src_onecell_get, clk_data);

    return;

err_free_data:
    kfree(clk_data);
}

static void __init of_ak_uv_det_pd(struct device_node* np)
{
    int ret;
    u32 regval;
    unsigned int enable = 0;
    unsigned int threshold = 0;
    void __iomem* reg = AK_VA_SYSCTRL;

    ret = of_property_read_u32(np, "enable", &enable);
    if (ret < 0) {
        pr_err("Could not read enable property of_uv_det_pd\n");
        return;
    }

    ret = of_property_read_u32(np, "threshold", &threshold);
    if (ret < 0) {
        pr_err("Could not read threshold property of_uv_det_pd\n");
        return;
    }

    switch (threshold) {
        case 0:
            threshold = 0B11;
            break;

        case 1:
            threshold = 0B01;
            break;

        case 2:
            threshold = 0B00;
            break;

        default:
            threshold = 0B11;
            break;
    }
    regval = __raw_readl(reg + ANALOG_CTRL_REG3);
    regval &= ~(0x3 << 20);
    regval |= threshold << 20;
    if (enable)
        regval &= ~(1 << 30);
    else
        regval |= 1 << 30;
    __raw_writel(regval, reg + ANALOG_CTRL_REG3);
}

CLK_OF_DECLARE(ak39ev330_core_fixed_clk, "anyka,ak39ev330-fixed-clk",
    of_ak_fixed_clk_init);
CLK_OF_DECLARE(
    ak39ev330_factor_clk, "anyka,ak39ev330-factor-clk", of_ak_factor_clk_init);
CLK_OF_DECLARE(
    ak39ev330_core_mux_clk, "anyka,ak39ev330-mux-clk", of_ak_mux_clk_init);
CLK_OF_DECLARE(
    ak39ev330_core_gate_clk, "anyka,ak39ev330-gate-clk", of_ak_gate_clk_init);
CLK_OF_DECLARE(
    ak39ev330_uv_det_pd, "anyka,ak39ev330-uv_det_pd", of_ak_uv_det_pd);

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
/*
 * /proc/clk
 */
static int proc_clk_show(struct seq_file* m, void* v)
{

    seq_printf(m,
        "core_pll:   %lu MHz\n"
        "vclk:       %lu MHz\n"
        "adc_rate:   %lu Hz\n"
        "dac_rate:   %lu Hz\n",
        ak_get_core_pll_clk(AK_VA_SYSCTRL) / MHz,
        ak_get_vclk(AK_VA_SYSCTRL) / MHz,
        sdadc_get_clk(NULL, ak_get_core_pll_clk(AK_VA_SYSCTRL)),
        sddac_get_clk(NULL, ak_get_core_pll_clk(AK_VA_SYSCTRL)));

    return 0;
}
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

MODULE_VERSION("1.0.00");