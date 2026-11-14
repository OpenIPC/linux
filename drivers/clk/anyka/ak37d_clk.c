/*
 * AK37D clk driver
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
 * DOC: AK37D (clock resource manager)
 *
 * The clock tree on the AK37D has several levels.  There's a root
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
#define CLOCK_CPU_PLL_CTRL        (0x04)
#define CLOCK_CORE_PLL_CTRL       (0x08)
#define CLOCK_ADC2_DAC_CTRL       (0x0C)
#define CLOCK_ADC2_DAC_HS_CTRL    (0x10)
#define CLOCK_PERI_PLL_CTRL1      (0x14)
#define CLOCK_PERI_PLL_CTRL2      (0x18)
#define CLOCK_GATE_CTRL1          (0x1C)
#define CLOCK_SOFT_RESET          (0x20)
#define CLOCK_USB_I2S_CTRL        (0x58)
#define CLOCK_DAC_FADEOUT_CTRL    (0x70)
#define ANALOG_CTRL_REG3          (0xA4)
#define CLOCK_SPI_HIGH_SPEED_CTRL (0x100)
#define CLOCK_AUDIO_PLL_CTRL      (0x1B8)
#define CLOCK_SD_ADC_DAC_CTRL     (0x1BC)

#define CLOCK_GATE_RESET_CTRL2 (0xFC)

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

/* PERI PLL channel clock control register1 */
/* 1: positive clk  0: negative clk */
#define AK_CLKCON_PCLK_CIS  (1 << 28) // camera
#define AK_CLKCON_SCLK_CIS  (1 << 18) // sensor
#define AK_CLKCON_CLK_OPCLK (1 << 8) // MAC

#define AK37D_CLK_CPU2X4X_MODE    (1 << 24)
#define AK37D_CLK_CPU_DPHY_OPP_EN (1 << 26)
#define AK37D_CLK_DPHY1X2X_MODE   (1 << 30)

/*
 * CLOCK_AUDIO_PLL_CTRL
 */
#define CLOCK_AUDIO_PLL_DISABLE_MASK       (0x1)
#define CLOCK_AUDIO_PLL_DISABLE_SHIFT      (15)
#define CLOCK_AUDIO_PLL_FREQ_ADJ_CFG_SHIFT (14)
#define CLOCK_AUDIO_PLL_FB_DIV_N_MASK      (0x1FF)
#define CLOCK_AUDIO_PLL_FB_DIV_N_SHIFT     (5)
#define CLOCK_AUDIO_PLL_REF_DIV_M_MASK     (0x1F)
#define CLOCK_AUDIO_PLL_REF_DIV_M_SHIFT    (0)

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

/*
    pll（160mhz-320mhz）      1   1   0   0
采样率   PLL（HZ） PLL_M       PLL_N   DIV OSR spr_compute error(%)
    8k  184320000   24          95      90  256 8000.000000     0
    11.025k 282240000 24        146     100 256 11025.000000    0
    12k 168960000   24          87      55  256 12000.000000    0
note：MCLK要为12.288mhz，则pll要为184320000 16k 184320000   24          95 45
256 16000.000000    0 22.05k  282240000   24      146     50  256 22050.000000 0
    24k 184320000   24          95      30  256 24000.000000    0
    32k 204800000   14          63      25  256 32000.000000    0
note：MCLK要为12.288mhz，则pll要为245760000，DIV为30 44.1k   282240000   24 146
25  256 44100.000000    0 48k 184320000   24          95      15  256
48000.000000    0 88.2k   293538461   25      158     13  256 88202.662560
0.0030187   note:可以使用（pll）282240000／（osr）128 96k 245760000   24 127 10
256 96000.000000    0
*/

static unsigned long pll4[]
    = { 204800000, 282240000, 168960000, 204800000, 282240000, 184320000,
          204800000, 282240000, 184320000, 293538461, 245760000 };
static unsigned char pll4_m[]
    = { 14, 24, 24, 14, 24, 24, 14, 24, 24, 25, 24 }; /*width:5bits*/
static unsigned short pll4_n[]
    = { 63, 146, 87, 63, 146, 95, 63, 146, 95, 158, 127 }; /*width:9bits*/

static unsigned short sddac_DIV[]
    = { 100, 100, 55, 50, 50, 30, 25, 25, 15, 13, 10 };
static unsigned short sdadc_DIV[]
    = { 100, 100, 55, 50, 50, 30, 25, 25, 15, 13, 10 };
static unsigned short sddac_OSR[]
    = { 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256 };
static unsigned short sdadc_OSR[]
    = { 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256 };
static unsigned int sample_rate[] = { 8000, 11025, 12000, 16000, 22050, 24000,
    32000, 44100, 48000, 88202, 96000 };

/*
 * @BRIEF       ak_get_cpu_pll_clk
 * @PARAM[in]   reg
 * @RETURN      unsigned long
 * @RETVAL      cpu_pll_clk * MHz;
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
 * @BRIEF       ak get core pll clk
 * @PARAM[in]   reg
 * @RETURN      unsigned long
 * @RETVAL      core_pll_clk * MHz
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

/*
 * @BRIEF       ak_get_audio_pll_freq
 * @PARAM[in]   reg
 * @RETURN      unsigned long
 * @RETVAL      audio_clk
 */
static unsigned long ak_get_audio_pll_freq(void __iomem* reg)
{
    u32 regval, audio_m, audio_n;
    unsigned long audio_clk;

    regval = __raw_readl(reg + CLOCK_AUDIO_PLL_CTRL);

    /*
     * CLOCK_AUDIO_PLL_STATUS: AUDIO_PLL is active or not. 1 means not active
     */
    if ((regval >> CLOCK_AUDIO_PLL_DISABLE_SHIFT)
        & CLOCK_AUDIO_PLL_DISABLE_MASK) {
        return 0;
    }

    audio_m = (regval >> CLOCK_AUDIO_PLL_REF_DIV_M_SHIFT)
        & CLOCK_AUDIO_PLL_REF_DIV_M_MASK;
    audio_n = (regval >> CLOCK_AUDIO_PLL_FB_DIV_N_SHIFT)
        & CLOCK_AUDIO_PLL_FB_DIV_N_MASK;

    audio_clk = (48000000UL * (audio_n + 1)) / (audio_m + 1);

    pr_info("Now AUDIO PLL %lu Hz m %d n %d\n", audio_clk, audio_m, audio_n);

    return audio_clk;
}

/*
 * @BRIEF       ak_get_vclk
 * @PARAM[in]   reg
 * @RETURN      unsigned long
 * @RETVAL      vclk * MHz
 */
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

/*
 * @BRIEF       ak_fixed_clk_round_rate
 * @PARAM[in]   hw
 * @PARAM[in]   rate
 * @PARAM[in]   parent_rate
 * @RETURN      long
 * @RETVAL      rate
 */
static long ak_fixed_clk_round_rate(
    struct clk_hw* hw, unsigned long rate, unsigned long* parent_rate)
{
    struct ak_fixed_clk* fixed_clk = to_clk_ak_fixed(hw);

    pr_debug("%s %d id:%d, rate:%lu, parent_rate:%lu\n", __func__, __LINE__,
        fixed_clk->id, rate, *parent_rate);
    return rate;
}

/*
 * @BRIEF       ak_set_audio_pll_freq
 * @PARAM[in]   reg
 * @PARAM[in]   rate
 * @RETURN      int
 * @RETVAL      0/1
 */
static int ak_set_audio_pll_freq(void __iomem* reg, unsigned long rate)
{
    u32 regval;
    int i, num;
    int timeout;

    num = sizeof(pll4) / sizeof(pll4[0]);

    for (i = 0; i < num; i++) {
        if (rate == pll4[i]) {
            break;
        }
    }

    if (i >= num) {
        pr_err("audio pll not support %lu\n", rate);
        return -1;
    }

    /*open audio pll*/
    regval = __raw_readl(reg + CLOCK_AUDIO_PLL_CTRL);
    regval &= ~(0x1 << 15);
    regval |= 0x1 << 16;
    __raw_writel(regval, reg + CLOCK_AUDIO_PLL_CTRL);

    /*set audio pll frequence*/
    regval = __raw_readl(reg + CLOCK_AUDIO_PLL_CTRL);
    regval &= ~(0x1f << 0);
    regval &= ~(0x1ff << 5);
    regval |= (pll4_m[i] & 0x1f) << 0;
    regval |= (pll4_n[i] & 0x1ff) << 5;
    regval |= 1 << 14;
    __raw_writel(regval, reg + CLOCK_AUDIO_PLL_CTRL);

    /*wait pll adjust finish*/
    timeout = 100000;
    do {
        timeout--;
        regval = __raw_readl(reg + CLOCK_AUDIO_PLL_CTRL) & (0x1 << 14);
    } while (regval && (timeout > 0));
    if (timeout <= 0) {
        pr_err("audio_pll update @%lu m %d n %d timeout\n", rate, pll4_m[i],
            pll4_n[i]);
        return -1;
    }

    pr_info("audio_pll update @%lu m %d n %d\n", rate, pll4_m[i], pll4_n[i]);

    return 0;
}

/*
 * @BRIEF        ak_fixed_clk_set_rate
  * @PARAM[in]   hw
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       int
 * @RETVAL       0
 */
static int ak_fixed_clk_set_rate(
    struct clk_hw* hw, unsigned long rate, unsigned long parent_rate)
{
    struct ak_fixed_clk* fixed_clk = to_clk_ak_fixed(hw);

    pr_debug("%s %d id:%d, rate:%lu, parent_rate:%lu\n", __func__, __LINE__,
        fixed_clk->id, rate, parent_rate);

    return 0;
}


/*
 * @BRIEF        ak_fixed_clk_recalc_rate
  * @PARAM[in]   hw
 * @PARAM[in]    parent_rate
 * @RETURN       unsigned long
 * @RETVAL       pll4_clk
 */
static unsigned long ak_fixed_clk_recalc_rate(
    struct clk_hw* hw, unsigned long parent_rate)
{
    u32 regval;
    int div_j, div_d, div_phy;
    // get fixed clk rate from dts
    struct ak_fixed_clk* fixed_clk = to_clk_ak_fixed(hw);

    if (fixed_clk->id == 12) {
        int pll4_m, pll4_n;
        unsigned long pll4_clk;

        regval = __raw_readl(fixed_clk->reg + CLOCK_AUDIO_PLL_CTRL);
        pll4_m = regval & 0x1f;
        pll4_n = (regval >> 5) & 0x1ff;

        pll4_clk = 48000000 / (pll4_m + 1) * (pll4_n + 1);

        pr_debug("%s %d id:%d parent rate:%lu,"
                 "regval:0x%08x, pll4_clk:%lu, m:%d, n:%d\n",
            __func__, __LINE__, fixed_clk->id, parent_rate, regval, pll4_clk,
            pll4_m, pll4_n);

        return pll4_clk;
    }

    regval = __raw_readl(fixed_clk->reg + CLOCK_CPU_PLL_CTRL);
    if (regval & AK37D_CLK_CPU_DPHY_OPP_EN) {
        div_j = 0;
        div_d = 1;
        div_phy = 0;
    } else {
        if (regval & AK37D_CLK_CPU2X4X_MODE) {
            div_j = 0;
            div_d = 2;
        } else {
            div_j = 1;
            div_d = 2;
        }
        if (regval & AK37D_CLK_DPHY1X2X_MODE)
            div_phy = 1;
        else
            div_phy = 2;
    }

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
    unsigned long* parent_rate, int* div, int* osr)
{
    int i;
    int num;

    pr_debug("%s %d %s rate:%lu, parent_rate:%lu\n", __func__, __LINE__,
        is_sdadc ? "sdadc" : "sddac", rate, *parent_rate);

    num = sizeof(pll4) / sizeof(pll4[0]);

    for (i = 0; i < num; i++) {
        if (rate == sample_rate[i]) {
            break;
        }
    }

    if (i >= num) {
        pr_err("%s %d rate:%lu failed\n", __func__, __LINE__, rate);
        return -1;
    }

    *parent_rate = pll4[i];
    if (is_sdadc) {
        *div = sdadc_DIV[i];
        *osr = sdadc_OSR[i];
    } else {
        *div = sddac_DIV[i];
        *osr = sddac_OSR[i];
    }

    return 0;
}

/*
 * @BRIEF        sdadc_set_div_osr
 * @PARAM[in]    factor_clk
 * @PARAM[in]    parent_rate
 * @PARAM[in]    rate
 * @RETURN       unsigned long
 * @RETVAL       0/1
 */
static int sdadc_set_div_osr(struct ak_factor_clk* factor_clk,
    unsigned long rate, unsigned long* parent_rate)
{
    unsigned int div, osr, map_osr;
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

    /*set adc_clk generated from audio_pll_clk*/
    regval = __raw_readl(factor_clk->reg + CLOCK_SD_ADC_DAC_CTRL);
    regval |= 1 << 20;
    __raw_writel(regval, factor_clk->reg + CLOCK_SD_ADC_DAC_CTRL);

    /*reset adc from adc clk*/
    regval = __raw_readl(factor_clk->reg + CLOCK_SOFT_RESET);
    regval &= ~(1 << 27);
    __raw_writel(regval, factor_clk->reg + CLOCK_SOFT_RESET);

    /*close adc fillter clk gate*/
    regval = __raw_readl(factor_clk->reg + CLOCK_SD_ADC_DAC_CTRL);
    regval &= ~(1 << 8);
    __raw_writel(regval, factor_clk->reg + CLOCK_SD_ADC_DAC_CTRL);

    /*change the div val*/
    regval = __raw_readl(factor_clk->reg + CLOCK_SD_ADC_DAC_CTRL);
    regval &= ~(0xff << 0);
    regval |= ((0xFF & (div - 1)) << 0);
    __raw_writel(regval, factor_clk->reg + CLOCK_SD_ADC_DAC_CTRL);

    /*set sdadc_div_vld_cfg*/
    regval = __raw_readl(factor_clk->reg + CLOCK_SD_ADC_DAC_CTRL);
    regval |= (1 << 9);
    __raw_writel(regval, factor_clk->reg + CLOCK_SD_ADC_DAC_CTRL);

    /*wait the sddac_div_vld_cfg clear*/
    timeout = 100000;
    do {
        timeout--;
        regval
            = __raw_readl(factor_clk->reg + CLOCK_SD_ADC_DAC_CTRL) & (1 << 9);
    } while (regval && (timeout > 0));
    if (timeout <= 0) {
        pr_err("%s %d wait sddac_div timeout\n", __func__, __LINE__);
    }

    /*open adc fillter clk gate*/
    regval = __raw_readl(factor_clk->reg + CLOCK_SD_ADC_DAC_CTRL);
    regval |= (1 << 8);
    __raw_writel(regval, factor_clk->reg + CLOCK_SD_ADC_DAC_CTRL);

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

/*
 * @BRIEF        sddac_set_div_osr
 * @PARAM[in]    factor_clk
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       int
 * @RETVAL        fail:-1 success:0
 */
static int sddac_set_div_osr(struct ak_factor_clk* factor_clk,
    unsigned long rate, unsigned long* parent_rate)
{
    int div, osr, map_osr;
    int timeout;
    u32 regval;

    if (sdadc_sddac_calc_div_osr(0, rate, parent_rate, &div, &osr)) {
        return -1;
    }

    /*map osr*/
    switch (osr) {
        case 256:
            map_osr = 0x0;
            break;

        case 272:
            map_osr = 0x1;
            break;

        case 264:
            map_osr = 0x2;
            break;

        case 248:
            map_osr = 0x3;
            break;

        case 240:
            map_osr = 0x4;
            break;

        case 136:
            map_osr = 0x5;
            break;

        case 128:
            map_osr = 0x6;
            break;

        case 120:
            map_osr = 0x7;
            break;

        default:
            pr_err("%s %d rate:%lu failed, set osr fail. div:%d, osr:%d\n",
                __func__, __LINE__, rate, div, osr);
            return -1;
            break;
    }

    /*sele audio_pll generated dac_clk*/
    regval = __raw_readl(factor_clk->reg + CLOCK_SD_ADC_DAC_CTRL);
    regval |= 1 << 21;
    __raw_writel(regval, factor_clk->reg + CLOCK_SD_ADC_DAC_CTRL);

    /*close the dac_filter_en_cfg*/
    regval = __raw_readl(factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);
    regval &= ~(1 << 3);
    __raw_writel(regval, factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);

    /*reset dac from dac clk*/
    regval = __raw_readl(factor_clk->reg + CLOCK_SOFT_RESET);
    regval &= ~(1 << 26);
    __raw_writel(regval, factor_clk->reg + CLOCK_SOFT_RESET);

    /*disable the sd_dac_clk*/
    regval = __raw_readl(factor_clk->reg + CLOCK_SD_ADC_DAC_CTRL);
    regval &= ~(1 << 18);
    __raw_writel(regval, factor_clk->reg + CLOCK_SD_ADC_DAC_CTRL);

    /*change the div val*/
    regval = __raw_readl(factor_clk->reg + CLOCK_SD_ADC_DAC_CTRL);
    regval &= ~(0xFF << 10);
    regval |= ((0xFF & (div - 1)) << 10);
    __raw_writel(regval, factor_clk->reg + CLOCK_SD_ADC_DAC_CTRL);

    /*set sddac_div_vld_cfg*/
    regval = __raw_readl(factor_clk->reg + CLOCK_SD_ADC_DAC_CTRL);
    regval |= (1 << 19);
    __raw_writel(regval, factor_clk->reg + CLOCK_SD_ADC_DAC_CTRL);

    /*wait the sddac_div_vld_cfg  clear*/
    timeout = 100000;
    do {
        timeout--;
        regval
            = __raw_readl(factor_clk->reg + CLOCK_SD_ADC_DAC_CTRL) & (1 << 19);
    } while (regval && (timeout > 0));
    if (timeout <= 0) {
        pr_err("%s %d wait sddac_div timeout\n", __func__, __LINE__);
    }

    /*enable the sd_dac_clk*/
    regval = __raw_readl(factor_clk->reg + CLOCK_SD_ADC_DAC_CTRL);
    regval |= (1 << 18);
    __raw_writel(regval, factor_clk->reg + CLOCK_SD_ADC_DAC_CTRL);

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
    regval |= (1 << 3);
    __raw_writel(regval, factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);

    return 0;
}
/*end of func*/

/*
 * @BRIEF        adchs_set_div
 * @PARAM[in]    factor_clk
 * @PARAM[in]    factor
 * @RETURN       int
 * @RETVAL       success:0
 */
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

/*
 * @BRIEF        dachs_set_div
 * @PARAM[in]    factor_clk
 * @PARAM[in]    factor
 * @RETURN       int
 * @RETVAL       success:0
 */
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

/*
 * @BRIEF        ak_get_audio_pll_by_sample_rate
 * @PARAM[in]    target_rate
 * @PARAM[in]    rate
 * @RETURN       int
 * @RETVAL       success:0
 */
static int ak_get_audio_pll_by_sample_rate(
    unsigned long target_rate, unsigned long* rate)
{
    int i, num = sizeof(sample_rate) / sizeof(sample_rate[0]);

    for (i = 0; i < num; i++) {
        if (target_rate == sample_rate[i]) {
            break;
        }
    }

    if (i >= num) {
        pr_err("audio pll not support %lu\n", target_rate);
        return -EINVAL;
    }

    (*rate) = pll4[i];

    return 0;
}

/*
 * @BRIEF        ak_factor_clk_round_rate
 * @PARAM[in]    hw
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       long
 * @RETVAL       ret
 */
static long ak_factor_clk_round_rate(
    struct clk_hw* hw, unsigned long rate, unsigned long* parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    long ret = 0;
    u32 factor;
    u32 regval;
    unsigned long flags;
    unsigned long actual_rate = 0, match_rate = 0;

    pr_debug("%s %d id:%d\n", __func__, __LINE__, factor_clk->id);

    local_irq_save(flags);

    factor = (*parent_rate) / rate - 1;
    /* calculate the factor value, and set the proper reg bits */
    switch (factor_clk->id) {
        case 0: // adchs
            adchs_set_div(factor_clk, factor);
            break;
        case 1: // dachs
            dachs_set_div(factor_clk, factor);
            break;
        case 2: // spi0_clk
            /* spi hsclk = 2* spi_bus_clock(80M) = 160M*/
            factor = (*parent_rate) / (rate * 2) - 1;

            /* hsclk must be disable,before adjustment clock divider parameter.
             */
            regval = __raw_readl(factor_clk->reg + CLOCK_SPI_HIGH_SPEED_CTRL);
            regval &= ~(0x1 << 8);
            __raw_writel(regval, factor_clk->reg + CLOCK_SPI_HIGH_SPEED_CTRL);

            regval = __raw_readl(factor_clk->reg + CLOCK_SPI_HIGH_SPEED_CTRL);
            regval &= ~(0xFF);
            regval |= (0xFF & factor);
            __raw_writel(regval, factor_clk->reg + CLOCK_SPI_HIGH_SPEED_CTRL);
            break;
        case 3: // sclk0
            /* hsclk must be disable,before adjustment clock divider parameter.
             */
            regval = __raw_readl(factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            regval &= ~(0x1 << 18);
            __raw_writel(regval, factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            regval = __raw_readl(factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            regval &= ~(0x3F << 10);
            regval |= ((0xFF & factor) << 10);
            __raw_writel(regval, factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            break;
        case 4: // sclk1
            /* hsclk must be disable,before adjustment clock divider parameter.
             */
            regval = __raw_readl(factor_clk->reg + CLOCK_SPI_HIGH_SPEED_CTRL);
            regval &= ~(0x1 << 18);
            __raw_writel(regval, factor_clk->reg + CLOCK_SPI_HIGH_SPEED_CTRL);
            regval = __raw_readl(factor_clk->reg + CLOCK_SPI_HIGH_SPEED_CTRL);
            regval &= ~(0x3F << 10);
            regval |= ((0xFF & factor) << 10);
            __raw_writel(regval, factor_clk->reg + CLOCK_SPI_HIGH_SPEED_CTRL);
            break;
        case 5: // csi_pclk
            /* hsclk must be disable,before adjustment clock divider parameter.
             */
            regval = __raw_readl(factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            regval &= ~(0x1 << 28);
            __raw_writel(regval, factor_clk->reg + CLOCK_PERI_PLL_CTRL2);

            regval = __raw_readl(factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            regval &= ~(0x3F << 20);
            regval |= ((0x3F & factor) << 20);
            __raw_writel(regval, factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            break;
        case 6: // dsi_pclk
            /* config LCD PCLK in LCD driver, this function no use here */
            /* refer to 0x200100BC */
            break;
        case 7: // opclk
            /*opclk clk is fixed to 50MHz*/
            break;
        case 8: // usb_phy
            /* usb phy clk fixed to 12MHz */
            break;
        case 9: // sdadc
        case 10: // sddac
            if (ak_get_audio_pll_by_sample_rate(rate, &match_rate)) {
                ret = 0;
            } else {
                /*
                 * Update the audio pll as we want
                 */
                actual_rate = ak_get_audio_pll_freq(factor_clk->reg);
                pr_debug("audio actual_rate %lu match_rate %lu\n", actual_rate,
                    match_rate);
                if (actual_rate != match_rate) {
                    ak_set_audio_pll_freq(factor_clk->reg, match_rate);
                }
                ret = rate;
                *parent_rate = match_rate;
            }
            break;
        case 11: // saradc
            regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_CTRL);
            regval &= ~(0x1 << 3); /*sar_adc_clk_en*/
            __raw_writel(regval, factor_clk->reg + CLOCK_ADC2_DAC_CTRL);

            regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_CTRL);
            regval &= ~(0x7); /*sar_adc_clk_div_num_cfg*/
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
/*end of func*/

/*
 * @BRIEF        ak_factor_clk_set_rate
 * @PARAM[in]    hw
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       int
 * @RETVAL       0
 */
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
        case 2: // spi0_clk
            regval = __raw_readl(factor_clk->reg + CLOCK_SPI_HIGH_SPEED_CTRL);
            regval |= (0x1 << 9);
            __raw_writel(regval, factor_clk->reg + CLOCK_SPI_HIGH_SPEED_CTRL);
            regval |= (0x1 << 8);
            __raw_writel(regval, factor_clk->reg + CLOCK_SPI_HIGH_SPEED_CTRL);
            break;
        case 3: // sclk0
            regval = __raw_readl(factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            regval |= (0x1 << 19);
            __raw_writel(regval, factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            regval |= (0x1 << 18);
            __raw_writel(regval, factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            break;
        case 4: // sclk1
            regval = __raw_readl(factor_clk->reg + CLOCK_SPI_HIGH_SPEED_CTRL);
            regval |= (0x1 << 19);
            __raw_writel(regval, factor_clk->reg + CLOCK_SPI_HIGH_SPEED_CTRL);
            regval |= (0x1 << 18);
            __raw_writel(regval, factor_clk->reg + CLOCK_SPI_HIGH_SPEED_CTRL);
            break;
        case 5: // csi_pclk
            regval = __raw_readl(factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            regval |= (0x1 << 29);
            __raw_writel(regval, factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            regval |= (0x1 << 28);
            __raw_writel(regval, factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            break;
        case 6: // dsi_pclk
            /*config enable in LCD driver */
            break;
        case 7: // opclk
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
        case 8: // usb_phy
            /* config fixed to 12MHz */
            break;
        case 9: // sdadc
            parent_rate = ak_get_audio_pll_freq(factor_clk->reg);
            sdadc_set_div_osr(factor_clk, rate, &parent_rate);
            break;
        case 10: // sddac
            parent_rate = ak_get_audio_pll_freq(factor_clk->reg);
            sddac_set_div_osr(factor_clk, rate, &parent_rate);
            break;
        case 11: // saradc
            regval = __raw_readl(factor_clk->reg + CLOCK_ADC2_DAC_CTRL);
            regval |= (0x1 << 3); /*sar_adc_clk_en*/
            __raw_writel(regval, factor_clk->reg + CLOCK_ADC2_DAC_CTRL);
            break;

        default:
            pr_err("No this factor clk config in dts.\n");
            break;
    }
    local_irq_restore(flags);

    return 0;
}
/*end of func*/

/*
 * @BRIEF        sdadc_get_clk
 * @PARAM[in]    factor_clk
 * @PARAM[in]    parent_rate
 * @RETURN       unsigned long
 * @RETVAL       sdadc_clk
 */
static unsigned long sdadc_get_clk(
    struct ak_factor_clk* factor_clk, unsigned long parent_rate)
{
    int div, osr;
    unsigned int regval_div, regval_osr, reg_val;
    unsigned long sdadc_clk;
    void __iomem* reg = factor_clk ? (factor_clk->reg) : AK_VA_SYSCTRL;

    reg_val = __raw_readl(reg + CLOCK_SD_ADC_DAC_CTRL);
    if (reg_val & (1 << 20)) {
        parent_rate = ak_get_audio_pll_freq(reg);
    }

    regval_div = __raw_readl(reg + CLOCK_SD_ADC_DAC_CTRL);
    div = (regval_div & 0xFF) + 1;

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
    int div, osr;
    unsigned int regval_div, regval_osr, reg_val;
    unsigned long sddac_clk;
    void __iomem* reg = factor_clk ? (factor_clk->reg) : AK_VA_SYSCTRL;

    reg_val = __raw_readl(reg + CLOCK_SD_ADC_DAC_CTRL);
    if (reg_val & (1 << 21)) {
        parent_rate = ak_get_audio_pll_freq(reg);
    }

    regval_div = __raw_readl(reg + CLOCK_SD_ADC_DAC_CTRL);
    div = ((regval_div >> 10) & 0xff) + 1;

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
        case 2: // spi0_clk
            regval = __raw_readl(factor_clk->reg + CLOCK_SPI_HIGH_SPEED_CTRL);
            factor = regval & 0xFF;
            break;
        case 3: // sclk0
            regval = __raw_readl(factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            factor = (regval >> 10) & 0x3F;
            break;
        case 4: // sclk1
            regval = __raw_readl(factor_clk->reg + CLOCK_SPI_HIGH_SPEED_CTRL);
            factor = (regval >> 10) & 0x3F;
            break;
        case 5: // csi_pclk
            regval = __raw_readl(factor_clk->reg + CLOCK_PERI_PLL_CTRL2);
            factor = (regval >> 20) & 0x3F;
            break;
        case 6: // dsi_pclk
            /*config enable in LCD driver */
            break;
        case 7: // opclk
            /* fixed to 50MHz */
            factor = 12 - 1;
            break;
        case 8: // usb_phy
            /* fixed to 12MHz */
            factor = 50 - 1;
            break;
        case 9: // sdadc
            parent_rate = ak_get_audio_pll_freq(factor_clk->reg);
            return sdadc_get_clk(factor_clk, parent_rate);
            break;
        case 10: // sddac
            parent_rate = ak_get_audio_pll_freq(factor_clk->reg);
            return sddac_get_clk(factor_clk, parent_rate);
            break;
        case 11: // saradc
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
    } else if (gate_clk->reg == AK_VA_SYSCTRL + CLOCK_GATE_RESET_CTRL2) {
        con = __raw_readl(gate_clk->reg);
        con &= ~(1 << gate_clk->ctrlbit);
        __raw_writel(con, gate_clk->reg);

        con = __raw_readl(gate_clk->reg);
        con |= (0x1 << (gate_clk->ctrlbit + 16));
        __raw_writel(con, gate_clk->reg);
        mdelay(1);
        con = __raw_readl(gate_clk->reg);
        con &= ~(0x1 << (gate_clk->ctrlbit + 16));
        __raw_writel(con, gate_clk->reg);
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
    } else if (gate_clk->reg == AK_VA_SYSCTRL + CLOCK_GATE_RESET_CTRL2) {
        con = __raw_readl(gate_clk->reg);
        con |= (1 << gate_clk->ctrlbit);
        __raw_writel(con, gate_clk->reg);
    }
    local_irq_restore(flags);
}

static unsigned long ak_gc_recalc_rate(
    struct clk_hw* hw, unsigned long parent_rate)
{
    struct ak_gate_clk* gate_clk = to_clk_ak_gate(hw);
    unsigned long rate = 0;
    u32 regval = __raw_readl(AK_VA_SYSCTRL + CLOCK_CORE_PLL_CTRL);

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
                regval = __raw_readl(AK_VA_SYSCTRL + CLOCK_CORE_PLL_CTRL);
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
    u32 rd_div_m;
    u32 rd_div_n;
    u32 rd_div_od;
    u32 regval;

    if ((div_n < 1) || (div_n > 12))
        panic("CORE pll frequency parameter Error");

    regval = __raw_readl(reg + CLOCK_CORE_PLL_CTRL);
    rd_div_m = (regval & 0xff);
    rd_div_n = (regval >> 8) & 0xf;
    rd_div_od = (regval >> 12) & 0x3;
    if ((rd_div_m != 0xa0) && (rd_div_n != 0x2) && (rd_div_od != 0x3)) {
        printk(KERN_ERR "CORE pll has inited!\n");
        return;
    }
    /* core_pll = 480M=2vclk=4gclk;  vclk = 240M; gclk=120M */
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
    u32 rd_div_m;
    u32 rd_div_n;
    u32 rd_div_od;

    if ((div_n < 2) || (div_n > 6) || (div_od < 1) || (div_od > 3))
        panic("peri pll frequency parameter Error");

    regval = __raw_readl(reg + CLOCK_PERI_PLL_CTRL1);
    rd_div_m = (regval & 0xff);
    rd_div_n = (regval >> 8) & 0xf;
    rd_div_od = (regval >> 12) & 0x3;
    if ((rd_div_m == 0xc8) && (rd_div_n == 0x2) && (rd_div_od == 0x1)) {
        printk(KERN_ERR "peril pll has inited!\n");
        return;
    }
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
        if (ctrlbit >= 32) {
            ctrlbit %= 32;
            reg = AK_VA_SYSCTRL + CLOCK_GATE_RESET_CTRL2;
        }

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

CLK_OF_DECLARE(
    ak37d_core_fixed_clk, "anyka,ak37d-fixed-clk", of_ak_fixed_clk_init);
CLK_OF_DECLARE(
    ak37d_factor_clk, "anyka,ak37d-factor-clk", of_ak_factor_clk_init);
CLK_OF_DECLARE(ak37d_core_mux_clk, "anyka,ak37d-mux-clk", of_ak_mux_clk_init);
CLK_OF_DECLARE(
    ak37d_core_gate_clk, "anyka,ak37d-gate-clk", of_ak_gate_clk_init);
CLK_OF_DECLARE(ak37d_uv_det_pd, "anyka,ak37d-uv_det_pd", of_ak_uv_det_pd);

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
