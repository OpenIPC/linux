
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
#if defined(CONFIG_MACH_AK3918AV100)
#include <dt-bindings/clock/ak3918av100_clock.h>
#elif defined(CONFIG_MACH_AK3918EV300L)
#include <dt-bindings/clock/ak3918ev300l_clock.h>
#elif defined(CONFIG_MACH_AK3918AV130)
#include <dt-bindings/clock/ak3918av130_clock.h>
#elif defined(CONFIG_MACH_KM01A)
#include <dt-bindings/clock/ak3918av100_clock.h>
#endif
#include "ak3918av100_clk.h"

#ifdef CONFIG_MACH_KM01A
static unsigned long pll3[] = {266240000, 266240000, 266240000, 266240000,
  266240000, 266240000, 266240000, 266240000, 266240000, 266240000, 266240000};
static unsigned char pll3_nr[] = {25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25};
static unsigned short pll3_od[] = {6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6};
static unsigned short pll3_nf[] = {1664, 1664, 1664, 1664, 1664, 1664, 1664,
    1664, 1664, 1664, 1664};
static unsigned short sddac_DIV[] = {130, 201, 185, 130, 89, 42, 65,
    47, 21, 25, 23};
static unsigned short sdadc_DIV[] = {130, 94, 87, 65, 47, 43, 33,
    24, 22, 12, 11};
static unsigned short sddac_OSR[] = {256, 120, 120, 128, 136, 264, 128,
    128, 264, 120, 120};
static unsigned short sdadc_OSR[] = {256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256};
static unsigned int adc_dac_sample_rate[] = {8000, 11025, 12000, 16000, 22050,
    24000, 32000, 44100, 48000, 88200, 96000};
#elif defined(CONFIG_MACH_AK3918AV130)
static unsigned long pll3[]
= { 307200000, 307200000, 307200000, 307200000, 307200000, 307200000,
    307200000, 307200000, 307200000, 307200000, 307200000 };
static unsigned char pll3_nr[] = { 5, 5, 5, 5, 5, 5 };
static unsigned short pll3_od[] = { 2, 2, 2, 2, 2, 2 };
static unsigned short pll3_nf[]
= { 128, 128, 128, 128, 128, 128 };
static unsigned short sddac_DIV[]
= { 150, 125, 100, 75, 50, 25 };
static unsigned short sdadc_DIV[]
= { 150, 125, 100, 75, 50, 25 };
static unsigned short sddac_OSR[]
= { 256, 256, 256, 256, 256, 256 };
static unsigned short sdadc_OSR[]
= { 256, 256, 256, 256, 256, 256 };
static unsigned int adc_dac_sample_rate[] = { 8000, 9600, 12000, 16000,
    24000, 48000 };
#else
static unsigned long pll3[]
= { 348160000, 348160000, 348160000, 348160000, 348160000, 348160000,
    348160000, 348160000, 348160000, 348160000, 348160000 };
static unsigned char pll3_nr[] = { 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25};
static unsigned short pll3_od[] = { 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6 };
static unsigned short pll3_nf[]
= { 2176, 2176, 2176, 2176, 2176, 2176, 2176, 2176, 2176, 2176, 2176 };
static unsigned short sddac_DIV[]
= { 160, 247, 117, 170, 116, 121, 85, 58, 57, 29, 30 };
static unsigned short sdadc_DIV[]
= { 170, 123, 113, 85, 62, 57, 43, 31, 28, 15, 14 };
static unsigned short sddac_OSR[]
= { 272, 128, 248, 128, 136, 120, 128, 136, 128, 136, 120 };
static unsigned short sdadc_OSR[]
= { 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256 };
static unsigned int adc_dac_sample_rate[] = { 8000, 11025, 12000, 16000, 22050,
    24000, 32000, 44100, 48000, 88200, 96000 };
#endif

//芯片SIGNOFF频率是350Mhz,但是AV130项目会分到352Mhz.所以放宽这个条件
#define ISP_FREQ_MAX  (353 * MHz)
#define ISP_FREQ_MAX_DIV_FACTOR (0x7)
#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_KM01A)
#define VENC_FREQ_MAX (270 * MHz)
#elif defined(CONFIG_MACH_AK3918EV300L)
#define VENC_FREQ_MAX (255 * MHz)
#elif defined(CONFIG_MACH_AK3918AV130)
#define VENC_FREQ_MAX (270 * MHz)
#endif
#define VENC_FREQ_MAX_DIV_FACTOR (0x7)
#ifdef CONFIG_MACH_KM01A
#define NPU_FREQ_MAX  (300 * MHz)
#else
#define NPU_FREQ_MAX  (405 * MHz)
#endif
#define NPU_FREQ_MAX_DIV_FACTOR (0x7)
#ifdef CONFIG_MACH_KM01A
#define SFC_FREQ_MAX  (200 * MHz)
#else
#define SFC_FREQ_MAX  (100 * MHz)
#endif

static int ak_adjust_clk_int_divider_freq(void __iomem* res_reg,
        u32 div_vld_cfg_shift, u32 en_cfg_shift, u32 div_num_cfg,
        u32 div_num_cfg_shift, u32 div_num_cfg_mask);
static void ak_factor_pll2_clk_disable(struct clk_hw* hw);
static void ak_factor_pll3_clk_disable(struct clk_hw* hw);
static void ak_factor_osc24m_clk_disable(struct clk_hw* hw);
static void ak_factor_pll1_clk_disable(struct clk_hw* hw);

#ifdef AK_CLK_DUMP_INFO
void clk_dump_registers(void)
{
    void __iomem* res = AK_VA_SYSCTRL;

    pr_info("ANYKA CLK Register Dumping Begin:\n");
    pr_info("  PLL1(0x04) = 0x%08X, PLL2(0x08)    = 0x%0X\n",
            __raw_readl(res + CLOCK_PLL1_CTRL),
            __raw_readl(res + CLOCK_PLL2_CTRL));
    pr_info("  SDADDA_SPI0HS_SARADDA(0x0C) = 0x%08X, SD_ADC_DAC_HS(0x10)  = "
            "0x%0X\n",
            __raw_readl(res + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL),
            __raw_readl(res + CLOCK_SD_ADC_DAC_HS_CTRL));
    pr_info("  PLL3(0x14) = 0x%08X, CSI_SCLK_MAC_OPCLK(0x18) = 0x%0X\n",
            __raw_readl(res + CLOCK_PLL3_CTRL),
            __raw_readl(res + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL));
    pr_info("  GATE_CTRL(0x1C) = 0x%08X, SOFT_RESET(0x20) = 0x%0X\n",
            __raw_readl(res + CLOCK_GATE_CTRL),
            __raw_readl(res + CLOCK_SOFT_RESET));
    pr_info("  USB_I2S(0x58) = 0x%08X, DAC_FADEOUT(0x70) = 0x%0X\n",
            __raw_readl(res + CLOCK_USB_I2S_CTRL),
            __raw_readl(res + CLOCK_DAC_FADEOUT_CTRL));
    pr_info("  ISP_NPU_ENC(0x11c) = 0x%08X\n",
            __raw_readl(res + CLOCK_ISP_NPU_ENC_CTRL));
    pr_info("ANYKA CLK Register Dumping End.\n");
}
#endif

/*
 * @BRIEF        ak_clock_divider_adjust_complete
 * @PARAM[in]    res_reg
 * @PARAM[in]    shift
 * @PARAM[in]    mask
 * @RETURN       int
 * @RETVAL       sucess:0 fail:-1
 */
int ak_clock_divider_adjust_complete(
        void __iomem* res_reg, u32 shift, u32 mask)
{
#ifdef CONFIG_SYS_FAST_LAUNCH
    int timeout = 10;
#else
    int timeout = CLOCK_FREQ_ADJ_WAITING_MAX_NUM;
#endif
    u32 regval;

    do {
        timeout--;
        regval = ((__raw_readl(res_reg)) >> shift) & mask;
    } while (regval && (timeout > 0));

    if (timeout <= 0) {
        pr_err("Waiting 0x%p:(0x%x) shift %d mask 0x%x timeout\n", res_reg,
                __raw_readl(res_reg), shift, mask);
        return -1;
    }

    return 0;
}

/*
 * @BRIEF        ak_get_pll3_by_sample_rate
 * @PARAM[in]    target_rate
 * @PARAM[in]    rate
 * @RETURN       int
 * @RETVAL       sucess:0 fail:-EINVAL
 */
static int ak_get_pll3_by_sample_rate(
        unsigned long target_rate, unsigned long* rate)
{
    int i, num = sizeof(adc_dac_sample_rate) / sizeof(adc_dac_sample_rate[0]);

    for (i = 0; i < num; i++) {
        if (target_rate == adc_dac_sample_rate[i]) {
            break;
        }
    }

    if (i >= num) {
        pr_err("audio pll not support %lu\n", target_rate);
        return -EINVAL;
    }

    (*rate) = pll3[i];

    return 0;
}

/*
 * @BRIEF        ak_set_pll3_freq
 * @PARAM[in]    reg
 * @PARAM[in]    rate
 * @RETURN       int
 * @RETVAL       ret
 */
static int ak_set_pll3_freq(void __iomem* reg, unsigned long rate)
{
    u32 regval;
    int i, num;
    u32 pll_nf, pll_nr, pll_od;
    int ret;

    num = sizeof(pll3) / sizeof(pll3[0]);

    for (i = 0; i < num; i++) {
        if (rate == pll3[i]) {
            break;
        }
    }

    if (i >= num) {
        pr_err("pll3 not support %lu\n", rate);
        ret = -1;
        return ret;
    }

    pll_nf = pll3_nf[i];
    pll_nr = pll3_nr[i];
    pll_od = pll3_od[i];

    pll_nf--;
    pll_nr--;
    pll_od--;

    pll_nf &= 0x1fff; /* pll_clknf_cfg    0x0800,0008 or 0x0800,0014 bit  [12:0]
    */
    pll_nr
        &= 0x3f; /* pll_clknr_cfg    0x0800,0008 or 0x0800,0014 bit  [18:13] */
    pll_od
        &= 0xf; /* pll_clkod_cfg    0x0800,0008 or 0x0800,0014 bit  [22:19] */

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
    regval |= (1 << 29);
    __raw_writel(regval, reg + CLOCK_PLL1_CTRL);

    ret = ak_clock_divider_adjust_complete(reg + CLOCK_PLL1_CTRL,
            PLL3_FRQ_ADJ_VLD_CFG_SHIFT, PLL3_FRQ_ADJ_VLD_CFG_MASK);
    if (ret < 0) {
        pr_err("pll3 update @%lu (nf %d ,nr %d od %d timeout)\n", rate,
                pll3_nf[i], pll3_nr[i], pll3_od[i]);
        return ret;
    }

    pr_debug("pll3 update @%lu (nf %d ,nr %d od %d )\n", rate, pll3_nf[i],
            pll3_nr[i], pll3_od[i]);
    return ret;
}

#if defined(CONFIG_MACH_KM01A)

#define PDM_CTRL_REG (0xd0000)
#define PDM_CTRL_DECIMATION_RATIO_CFG_MASK  (0x07)
#define PDM_CTRL_DECIMATION_RATIO_CFG_SHIFT (13)

/*
 * PDM采样率
 * pdm_sr = pdm_clk / pdm_osr, pdm_osr = fourty_8_sel * 4
 * pdm_io 时钟信号的频率
 * pdm_clk = pdm_i2sm_clk / 2
 * pdm_i2sm_clk
 * pdm_i2sm_clk = audio_pll_clk / (pdm_i2sm_clk_div_num_cfg + 1)M
 * @rate -> pdm_sr
 */
static unsigned int pdm_calc_div_ratio(unsigned long pll, unsigned int rate,
        unsigned int *div, unsigned int *decimation_ratio_cfg_val)
{
    unsigned int decimation_ratio_list[] = {64, 48, 32, 24, 0, 0, 16, 12};
    unsigned int decimation_ratio, decimation_ratio_sel;
    unsigned int i;

    if (!decimation_ratio_cfg_val)
        return 0;

    for (i = 0; i < ARRAY_SIZE(decimation_ratio_list); i++) {
        decimation_ratio_sel = ARRAY_SIZE(decimation_ratio_list) - i;
        if (decimation_ratio_list[decimation_ratio_sel] == 0)
            continue;

        /*find the pdm_osr*/
        decimation_ratio = decimation_ratio_list[decimation_ratio_sel] * 4;
        *div = pll % (rate * decimation_ratio * 2);

        if (*div != 0)
            continue;

        *div = pll / (rate * decimation_ratio * 2);
        pr_info("%s pll#%lu pdm_i2sm_clk %lu pdm_clk %lu pdm_sr %lu\n",
            __func__, pll, pll/(*div), pll/(*div * 2),
            pll/(*div * 2 * decimation_ratio_list[decimation_ratio_sel] * 4));
        break;
    }

    if (i == ARRAY_SIZE(decimation_ratio_list)) {
        pr_err("%s invaild decimation_ratio\n", __func__);
        return -EINVAL;
    }

    *decimation_ratio_cfg_val = decimation_ratio_sel;

    return 0;
}

/*
 * pdm_set_div_ratio
 *
 */
static int pdm_set_div_ratio(struct ak_factor_clk *factor_clk,
        unsigned long rate, unsigned long parent_rate)
{
    int div, ratio;
    unsigned int reg_val;
    void __iomem *reg = factor_clk->reg;

    if (rate == 0) {
        reg_val = __raw_readl(reg + CLOCK_PDM_CTRL);
        reg_val &=
            ~(CLOCK_PDM_MSCLK_EN_CFG_MASK << CLOCK_PDM_MSCLK_EN_CFG_SHIFT);
        __raw_writel(reg_val, (reg + CLOCK_PDM_CTRL));

        reg_val = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
        return 0;
    }

    if (pdm_calc_div_ratio(parent_rate, rate, &div, &ratio)) {
        return -EINVAL;
    }

    ak_adjust_clk_int_divider_freq((reg + CLOCK_PDM_CTRL),
            CLOCK_PDM_MSCLK_DIV_VLD_CFG_SHIFT, CLOCK_PDM_MSCLK_EN_CFG_SHIFT,
            (div - 1), CLOCK_PDM_MSCLK_DIV_NUM_CFG_SHIFT,
            CLOCK_PDM_MSCLK_DIV_NUM_CFG_MASK);
    ak_module_reset_by_clock(
            factor_clk->reg + CLOCK_PDM_CTRL, CLOCK_PDM_MCLK_RESET_SHIFT);

    reg_val = __raw_readl(AK_VA_SUBCTRL + PDM_CTRL_REG);
    reg_val &= ~(PDM_CTRL_DECIMATION_RATIO_CFG_MASK <<
            PDM_CTRL_DECIMATION_RATIO_CFG_SHIFT);
    reg_val |= ((ratio & PDM_CTRL_DECIMATION_RATIO_CFG_MASK)
            << PDM_CTRL_DECIMATION_RATIO_CFG_SHIFT);
    __raw_writel(reg_val, AK_VA_SUBCTRL + PDM_CTRL_REG);

    return 0;
}

/*
 * pdm_set_hsclk
 *
 *
 */
static int pdm_set_hsclk(struct ak_factor_clk *factor_clk,
        unsigned long rate, unsigned long parent_rate)
{
    unsigned int regval, factor;
    void __iomem *reg = factor_clk->reg;

    if (rate == 0) {
        regval = __raw_readl(reg + CLOCK_PDM_CTRL);
        regval &=
            ~(CLOCK_PDM_HSCLK_EN_CFG_MASK << CLOCK_PDM_HSCLK_EN_CFG_SHIFT);
        __raw_writel(regval, (reg + CLOCK_PDM_CTRL));

        regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
        return 0;
    }

    /*
     * hsclk need to bigger than the i2m_clk
     * here make the max hsclk no matter which sample rate pdm use.
     */
    factor = 0;
    ak_adjust_clk_int_divider_freq((reg + CLOCK_PDM_CTRL),
        CLOCK_PDM_HSCLK_DIV_VLD_CFG_SHIFT, CLOCK_PDM_HSCLK_EN_CFG_SHIFT, factor,
        CLOCK_PDM_HSCLK_DIV_NUM_CFG_SHIFT, CLOCK_PDM_HSCLK_DIV_NUM_CFG_MASK);
    ak_module_reset_by_clock(
            factor_clk->reg + CLOCK_PDM_CTRL, CLOCK_PDM_HCLK_RESET_SHIFT);

    return 0;
}

static unsigned long pdm_get_clk(struct ak_factor_clk *factor_clk,
        unsigned long parent_rate)
{
    u32 regval, div, ratio;
    unsigned long pdm_clk;
    unsigned int pdm_ratio_list[] = {64, 48, 32, 24, 0, 0, 16, 12};
    void __iomem *reg = factor_clk ? (factor_clk->reg): AK_VA_SYSCTRL;

    regval = __raw_readl(reg + CLOCK_PDM_CTRL);

    if (!((regval >> CLOCK_PDM_MSCLK_EN_CFG_SHIFT)
                & CLOCK_PDM_MSCLK_EN_CFG_MASK))
        return 0;

    div = ((regval >> CLOCK_PDM_MSCLK_DIV_NUM_CFG_SHIFT)
            & CLOCK_PDM_MSCLK_DIV_NUM_CFG_MASK) + 1;

    regval = __raw_readl(AK_VA_SUBCTRL + PDM_CTRL_REG);
    ratio = (regval >> PDM_CTRL_DECIMATION_RATIO_CFG_SHIFT)
        & PDM_CTRL_DECIMATION_RATIO_CFG_MASK;
    ratio = pdm_ratio_list[ratio] * 4;

    pdm_clk = parent_rate / (div * 2) / ratio;

    return pdm_clk;
}
#endif

/*
 * @BRIEF        ak_factor_pll3_clk_round_rate
 * @PARAM[in]    hw
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       long
 * @RETVAL       round_rate
 */
static long ak_factor_pll3_clk_round_rate(
        struct clk_hw* hw, unsigned long rate, unsigned long* parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    unsigned long round_rate = rate;
    unsigned long actual_rate = 0, match_rate = 0;
    unsigned long flags;

    local_irq_save(flags);
    /*
     * Here:
     * 1. factor clock related to pll3
     * (PLL3_FACTOR_SDADC_CLK/PLL3_FACTOR_SDDAC_CLK) need to be modify by
     * different rate. here change the parent_rate.
     * TODO:How about PLL3_FACTOR_PDM_HSCLK and PLL3_FACTOR_PDM_I2SM_CLK.
     * !!!Please NOTICE audio only support the sample rate as previous!!!
     */
    switch (factor_clk->id) {
        case PLL3_FACTOR_SDADC_CLK:
        case PLL3_FACTOR_SDDAC_CLK:
#ifdef CONFIG_MACH_KM01A
        case PLL3_FACTOR_PDM_MSCLK:
#endif
            if (ak_get_pll3_by_sample_rate(rate, &match_rate)) {
                round_rate = 0;
            } else {
                /*
                 * Update the  pll3 as we want
                 */
                actual_rate = ak_get_pll3_clk(factor_clk->reg);
                pr_debug("audio actual_rate %lu match_rate %lu\n", actual_rate,
                        match_rate);
                if (actual_rate != match_rate) {
                    ak_set_pll3_freq(factor_clk->reg, match_rate);
                }
            }
            break;
        default:
            break;
    }
    local_irq_restore(flags);

    pr_debug("audio round_rate %s(%d) rate %lu parent %luMHz\n",
            hw->init->name,
            factor_clk->id, round_rate, (match_rate) / MHz);

    return round_rate;
}

/*
 * @BRIEF        ak_group_clock_reset_module
 * @PARAM[in]    reg
 * @PARAM[in]    reset_val
 * @PARAM[in]    ops
 * @RETURN       int
 * @RETVAL       0
 */
static int ak_group_clock_reset_module(
        void __iomem* reg, u32 reset_val, int ops)
{
    u32 regval;

    if (ops == MODULE_RESET_HOLD) {
        regval = __raw_readl(reg);
        regval |= (reset_val);
        __raw_writel(regval, reg);
    } else if (ops == MODULE_RESET_RELEASE) {
        regval = __raw_readl(reg);
        regval &= ~(reset_val);
        __raw_writel(regval, reg);
    }
    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000

    return 0;
}

/*
 * ak_adjust_clk_even_divider_freq:clock with even divider
 * like mac1_opclk:
 * 1. disable module driven by mac1_opclk
 * 2. enable the output clock when mac1_opclk_div_vld_cfg is 0 by settting
 * mac1_opclk_en_cfg to 1
 * 3. change the mac1_opclk_div_num_cfg and mac1_opclk_div_vld_cfg to be 1.
 *    wait mac1_opclk_div_vld_cfg to 0
 * 4. enable the module driven by mac1_opclk
 */

static int ak_adjust_clk_even_divider_freq(void __iomem* res_reg,
        u32 div_vld_cfg_shift, u32 en_cfg_shift, u32 div_num_cfg,
        u32 div_num_cfg_shift, u32 div_num_cfg_mask)
{
    u32 regval;

    ak_clock_divider_adjust_complete(res_reg, div_vld_cfg_shift, 0x01);
    regval = __raw_readl(res_reg);
    regval |= ((0x01) << en_cfg_shift);
    __raw_writel(regval, res_reg);

    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000

    if (div_num_cfg_shift >= 0) {
        regval = __raw_readl(res_reg);
        regval &= ~(div_num_cfg_mask << div_num_cfg_shift);
        regval |= ((div_num_cfg & div_num_cfg_mask) << div_num_cfg_shift);
        regval |= ((0x01) << div_vld_cfg_shift);
        __raw_writel(regval, res_reg);
        ak_clock_divider_adjust_complete(res_reg, div_vld_cfg_shift, 0x01);
    }
    return 0;
}

/*
 * ak_adjust_clk_int_divider_freq
 * like sdadc_clk:
 * 1.disable the sd-adc module
 * 2.set sdadc_clk_en_cfg to 0 to disable output when sdadc_clk_div_vld_cfg is
 * 0
 * 3. change the sdadc_clk_div_num_cfg and set sdadc_clk_div_vld_cfg to 1.
 *    wait sdadc_clk_div_vld_cfg to 0
 * 4. set sdadc_clk_en_cfg to 1 t enable the output clock.
 * 5. enable the sd-adc module
 */
static int ak_adjust_clk_int_divider_freq(void __iomem* res_reg,
        u32 div_vld_cfg_shift, u32 en_cfg_shift, u32 div_num_cfg,
        u32 div_num_cfg_shift, u32 div_num_cfg_mask)
{
    u32 regval;

    regval = __raw_readl(res_reg);
    regval &= ~((0x1) << en_cfg_shift);
    __raw_writel(regval, res_reg);

    regval = __raw_readl(res_reg);
    regval &= ~(div_num_cfg_mask << div_num_cfg_shift);
    regval |= ((div_num_cfg & div_num_cfg_mask) << div_num_cfg_shift);
    __raw_writel(regval, res_reg);

    if (div_vld_cfg_shift >= 0) {
        regval = __raw_readl(res_reg);
        regval |= ((0x01) << div_vld_cfg_shift);
        __raw_writel(regval, res_reg);
        ak_clock_divider_adjust_complete(res_reg, div_vld_cfg_shift, 0x01);
    }

    regval = __raw_readl(res_reg);
    regval |= ((0x1) << en_cfg_shift);
    __raw_writel(regval, res_reg);

    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
    return 0;
}

/*
 * ak_clk_mux_and_int_divider_freq
 *
 *
 */
static int ak_clk_mux_and_int_divider_freq(void __iomem* res_reg,
        u32 mux_num_cfg, u32 mux_num_cfg_shift, u32 mux_num_cfg_mask,
        u32 div_vld_cfg_shift, u32 en_cfg_shift, u32 div_num_cfg,
        u32 div_num_cfg_shift, u32 div_num_cfg_mask)
{
    u32 regval;

    regval = __raw_readl(res_reg);
    regval &= ~((0x1) << en_cfg_shift);
    __raw_writel(regval, res_reg);

#if defined (CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130)
    //h322ls pll有优化，分频=0选择output clk，分频大于0选择div clk
    mux_num_cfg = div_num_cfg ? mux_num_cfg : mux_num_cfg + 4 ;
#endif
    regval = __raw_readl(res_reg);
    regval &= ~(mux_num_cfg_mask << mux_num_cfg_shift);
    regval |= ((mux_num_cfg & mux_num_cfg_mask) << mux_num_cfg_shift);
    __raw_writel(regval, res_reg);

    regval = __raw_readl(res_reg);
    regval &= ~(div_num_cfg_mask << div_num_cfg_shift);
    regval |= ((div_num_cfg & div_num_cfg_mask) << div_num_cfg_shift);
    __raw_writel(regval, res_reg);

    if (div_vld_cfg_shift >= 0) {
        regval = __raw_readl(res_reg);
        regval |= ((0x01) << div_vld_cfg_shift);
        __raw_writel(regval, res_reg);
        ak_clock_divider_adjust_complete(res_reg, div_vld_cfg_shift, 0x01);
    }

    regval = __raw_readl(res_reg);
    regval |= ((0x1) << en_cfg_shift);
    __raw_writel(regval, res_reg);

    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
    return 0;
}

/*
 * @BRIEF        sdadc_sddac_calc_div_osr
 * @PARAM[in]    is_sdadc
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @PARAM[in]    div
 * @PARAM[in]    osr
 * @RETURN       int
 * @RETVAL       success:0 fail:-EINVAL
 */
static int sdadc_sddac_calc_div_osr(int is_sdadc, unsigned long rate,
        unsigned long* parent_rate, int* div, int* osr)
{
    int i;
    int num;

    pr_debug("%s %s rate %lu parent %lu\n", __func__,
            is_sdadc ? "sdadc" : "sddac", rate, *parent_rate);

    num = sizeof(pll3) / sizeof(pll3[0]);

    for (i = 0; i < num; i++) {
        if (rate == adc_dac_sample_rate[i]) {
            break;
        }
    }

    if (i >= num) {
        pr_err("%s %s rate:%lu failed\n", __func__,
                is_sdadc ? "sdadc" : "sddac", rate);
        return -EINVAL;
    }

    *parent_rate = pll3[i];

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
 * @BRIEF        sdadc_sddac_calc_div_osr
 * @PARAM[in]    factor_clk
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       int
 * @RETVAL       success:0 fail:-EINVAL
 */
static int sdadc_set_div_osr(struct ak_factor_clk* factor_clk,
        unsigned long rate, unsigned long parent_rate)
{
    unsigned long final_parent_rate;
    int div, osr, map_osr;
    u32 regval;

    if (sdadc_sddac_calc_div_osr(
                TYPE_SDADC, rate, &final_parent_rate, &div, &osr)) {
        return -EINVAL;
    }

    if (osr == 512) {
        map_osr = 0x0;
    } else if (osr == 256) {
        map_osr = 0x1;
    } else {
        pr_err("%s rate:%lu failed div %d osr %d\n", __func__, rate, div, osr);
    }

    ak_group_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
            ((0x1 << CLOCK_SDADC_RST_CFG_SHIFT)
             | (0x1 << CLOCK_SDADC_HSRST_CFG_SHIFT)),
            MODULE_RESET_HOLD);

    ak_adjust_clk_int_divider_freq(
        (factor_clk->reg + CLOCK_SD_ADC_DAC_HS_CTRL),
        CLOCK_SDADC_DIV_VLD_CFG_SHIFT, CLOCK_SDADC_CLK_EN_CFG_SHIFT, (div - 1),
        CLOCK_SDADC_CLK_DIV_NUM_CFG_SHIFT, CLOCK_SDADC_CLK_DIV_NUM_CFG_MASK);

    ak_group_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
            ((0x1 << CLOCK_SDADC_RST_CFG_SHIFT)
             | (0x1 << CLOCK_SDADC_HSRST_CFG_SHIFT)),
            MODULE_RESET_RELEASE);

    regval = __raw_readl(factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);
    regval &= ~(
        (CLOCK_DAC_48K_MODE_EN_CFG_MASK) << CLOCK_DAC_48K_MODE_EN_CFG_SHIFT);
    regval |= ((map_osr & CLOCK_DAC_48K_MODE_EN_CFG_MASK)
            << CLOCK_DAC_48K_MODE_EN_CFG_SHIFT);
    __raw_writel(regval, factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);

    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000

    return 0;
}

/*
 * @BRIEF        sddac_set_div_osr
 * @PARAM[in]    factor_clk
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       int
 * @RETVAL       success:0 fail:-EINVAL
 */
static int sddac_set_div_osr(struct ak_factor_clk* factor_clk,
        unsigned long rate, unsigned long parent_rate)
{
    unsigned long final_parent_rate;
    int div, osr;
    u32 map_osr, regval;

    if (sdadc_sddac_calc_div_osr(
                TYPE_SDDAC, rate, &final_parent_rate, &div, &osr)) {
        return -EINVAL;
    }

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
            pr_err("%s rate:%lu osr fail div %d osr %d\n", __func__, rate, div,
                    osr);
            return -EINVAL;
            break;
    }

    regval = __raw_readl(factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);
    regval
        &= ~((CLOCK_DAC_FILTER_EN_CFG_MASK) << CLOCK_DAC_FILTER_EN_CFG_SHIFT);
    __raw_writel(regval, factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);

    ak_group_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
            ((0x1 << CLOCK_SDDAC_RST_CFG_SHIFT)
             | (0x1 << CLOCK_SDDAC_HSRST_CFG_SHIFT)),
            MODULE_RESET_HOLD);

    ak_adjust_clk_int_divider_freq(
        (factor_clk->reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL),
        CLOCK_SDDAC_DIV_VLD_CFG_SHIFT, CLOCK_SDDAC_CLK_EN_CFG_SHIFT, (div - 1),
        CLOCK_SDDAC_CLK_DIV_NUM_CFG_SHIFT, CLOCK_SDDAC_CLK_DIV_NUM_CFG_MASK);

    ak_group_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
            ((0x1 << CLOCK_SDDAC_RST_CFG_SHIFT)
             | (0x1 << CLOCK_SDDAC_HSRST_CFG_SHIFT)),
            MODULE_RESET_RELEASE);

    regval = __raw_readl(factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);
    regval &= ~((CLOCK_DAC_OSR_CFG_MASK) << CLOCK_DAC_OSR_CFG_SHIFT);
    regval |= ((map_osr & CLOCK_DAC_OSR_CFG_MASK) << CLOCK_DAC_OSR_CFG_SHIFT);
    __raw_writel(regval, factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);

    regval = __raw_readl(factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);
    regval |= ((CLOCK_DAC_FILTER_EN_CFG_MASK)<<CLOCK_DAC_FILTER_EN_CFG_SHIFT);
    __raw_writel(regval, factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);

    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
    return 0;
}

/*
 * @BRIEF        adchs_set_div
 * @PARAM[in]    factor_clk
 * @PARAM[in]    factor
 * @RETURN       int
 * @RETVAL       success:0 
 */
static int adchs_set_div(struct ak_factor_clk* factor_clk, int factor)
{
    ak_group_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
            ((0x1 << CLOCK_SDADC_RST_CFG_SHIFT)
             | (0x1 << CLOCK_SDADC_HSRST_CFG_SHIFT)),
            MODULE_RESET_HOLD);

    ak_adjust_clk_int_divider_freq(
            (factor_clk->reg + CLOCK_SD_ADC_DAC_HS_CTRL),
            CLOCK_SDADC_HS_DIV_VLD_CFG_SHIFT, CLOCK_SDADC_HS_CLK_EN_CFG_SHIFT,
            factor, CLOCK_SDADC_HS_CLK_DIV_NUM_CFG_SHIFT,
            CLOCK_SDADC_HS_CLK_DIV_NUM_CFG_MASK);

    ak_group_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
            ((0x1 << CLOCK_SDADC_RST_CFG_SHIFT)
             | (0x1 << CLOCK_SDADC_HSRST_CFG_SHIFT)),
            MODULE_RESET_RELEASE);

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
    u32 regval;

    ak_group_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
            ((0x1 << CLOCK_SDDAC_RST_CFG_SHIFT)
             | (0x1 << CLOCK_SDDAC_HSRST_CFG_SHIFT)),
            MODULE_RESET_HOLD);

    regval = __raw_readl(factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);
    regval
        &= ~((CLOCK_DAC_FILTER_EN_CFG_MASK) << CLOCK_DAC_FILTER_EN_CFG_SHIFT);
    __raw_writel(regval, factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);

    ak_adjust_clk_int_divider_freq(
            (factor_clk->reg + CLOCK_SD_ADC_DAC_HS_CTRL),
            CLOCK_SDDAC_HS_DIV_VLD_CFG_SHIFT, CLOCK_SDDAC_HS_CLK_EN_CFG_SHIFT,
            factor, CLOCK_SDDAC_HS_CLK_DIV_NUM_CFG_SHIFT,
            CLOCK_SDDAC_HS_CLK_DIV_NUM_CFG_MASK);

    ak_group_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
            ((0x1 << CLOCK_SDDAC_RST_CFG_SHIFT)
             | (0x1 << CLOCK_SDDAC_HSRST_CFG_SHIFT)),
            MODULE_RESET_RELEASE);

    regval = __raw_readl(factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);
    regval |= ((CLOCK_DAC_FILTER_EN_CFG_MASK)<<CLOCK_DAC_FILTER_EN_CFG_SHIFT);
    __raw_writel(regval, factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);

    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
    return 0;
}

/*
 * @BRIEF        i2s0_mclk_sdadc_set_div
 * @PARAM[in]    factor_clk
 * @PARAM[in]    factor
 * @RETURN       int
 * @RETVAL       success:0 
 */
static int i2s0_mclk_sdadc_set_div(
        struct ak_factor_clk* factor_clk, int factor)
{
    unsigned int regval;
    void __iomem* reg = factor_clk->reg;

    /*
     * set sdadc_clk -> mclk
     */
    regval = __raw_readl(reg + CLOCK_USB_I2S_CTRL);
    regval &= ~(CLOCK_I2S_MCLK_SEL_MASK << CLOCK_I2S_MCLK_SEL_SHIFT);
    regval |= (CLOCK_I2S_MCLK_SEL_SDADC_CLK_CFG << CLOCK_I2S_MCLK_SEL_SHIFT);
    __raw_writel(regval, (reg + CLOCK_USB_I2S_CTRL));

    ak_group_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
            ((0x1 << CLOCK_SDADC_RST_CFG_SHIFT)
             | (0x1 << CLOCK_SDADC_HSRST_CFG_SHIFT)),
            MODULE_RESET_HOLD);

    ak_adjust_clk_int_divider_freq(
        (factor_clk->reg + CLOCK_SD_ADC_DAC_HS_CTRL),
        CLOCK_SDADC_DIV_VLD_CFG_SHIFT, CLOCK_SDADC_CLK_EN_CFG_SHIFT, factor,
        CLOCK_SDADC_CLK_DIV_NUM_CFG_SHIFT, CLOCK_SDADC_CLK_DIV_NUM_CFG_MASK);

    ak_group_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
            ((0x1 << CLOCK_SDADC_RST_CFG_SHIFT)
             | (0x1 << CLOCK_SDADC_HSRST_CFG_SHIFT)),
            MODULE_RESET_RELEASE);

    return 0;
}

/*
 * @BRIEF        i2s0_b_lr_sdadc_set_div
 * @PARAM[in]    factor_clk
 * @PARAM[in]    factor
 * @RETURN       int
 * @RETVAL       success:0 
 */
static int i2s0_b_lr_sdadc_set_div(
        struct ak_factor_clk* factor_clk, int factor)
{
    unsigned int regval;

    ak_group_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
            ((0x1 << CLOCK_SDDAC_RST_CFG_SHIFT)
             | (0x1 << CLOCK_SDDAC_HSRST_CFG_SHIFT)),
            MODULE_RESET_HOLD);

    regval = __raw_readl(factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);
    regval
        &= ~((CLOCK_DAC_FILTER_EN_CFG_MASK) << CLOCK_DAC_FILTER_EN_CFG_SHIFT);
    __raw_writel(regval, factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);

    ak_adjust_clk_int_divider_freq(
        (factor_clk->reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL),
        CLOCK_SDDAC_DIV_VLD_CFG_SHIFT, CLOCK_SDDAC_CLK_EN_CFG_SHIFT, factor,
        CLOCK_SDDAC_CLK_DIV_NUM_CFG_SHIFT, CLOCK_SDDAC_CLK_DIV_NUM_CFG_MASK);

    ak_group_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
            ((0x1 << CLOCK_SDDAC_RST_CFG_SHIFT)
             | (0x1 << CLOCK_SDDAC_HSRST_CFG_SHIFT)),
            MODULE_RESET_RELEASE);

    regval = __raw_readl(factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);
    regval |= ((CLOCK_DAC_FILTER_EN_CFG_MASK)<<CLOCK_DAC_FILTER_EN_CFG_SHIFT);
    __raw_writel(regval, factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);

    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
    return 0;
}

/*
 * @BRIEF        sdadc_get_clk
 * @PARAM[in]    factor_clk
 * @PARAM[in]    parent_rate
 * @RETURN       unsigned long
 * @RETVAL       sdadc_clk
 */
unsigned long sdadc_get_clk(
        struct ak_factor_clk* factor_clk, unsigned long parent_rate)
{
    u32 div, osr;
    u32 regval_div, regval_osr;
    unsigned long sdadc_clk;
    void __iomem* reg = factor_clk ? (factor_clk->reg) : AK_VA_SYSCTRL;

    regval_div = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
    if (!((regval_div >> CLOCK_SDADC_CLK_EN_CFG_SHIFT)
                & CLOCK_SDADC_CLK_EN_CFG_MASK))
        return 0;

    regval_div = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
    div = ((regval_div >> CLOCK_SDADC_CLK_DIV_NUM_CFG_SHIFT)
            & CLOCK_SDADC_CLK_DIV_NUM_CFG_MASK)
        + 1;

    regval_osr = __raw_readl(reg + CLOCK_DAC_FADEOUT_CTRL);
    osr = (regval_osr
            & (CLOCK_DAC_48K_MODE_EN_CFG_MASK
                << CLOCK_DAC_48K_MODE_EN_CFG_SHIFT))
        ? 256
        : 512;

    sdadc_clk = parent_rate / div / osr;

    pr_debug("parent:%lu,"
            "div:%d osr:%d sdadc_clk:%lu\n",
            parent_rate, div, osr, sdadc_clk);
    return sdadc_clk;
}

/*
 * @BRIEF        sddac_get_clk
 * @PARAM[in]    factor_clk
 * @PARAM[in]    parent_rate
 * @RETURN       unsigned long
 * @RETVAL       sddac_clk
 */
#if defined(CONFIG_MACH_AK3918AV130)
unsigned long sddac_get_clk(
        struct ak_factor_clk* factor_clk, unsigned long parent_rate)
{
    void __iomem* reg = factor_clk ? (factor_clk->reg) : AK_VA_SYSCTRL;
    u32 regval;
    int div, osr;
    u32 regval_osr;
    unsigned long rate;

    regval = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);

    if (!((regval >> CLOCK_SDDAC_CLK_EN_CFG_SHIFT)
                & CLOCK_SDDAC_CLK_EN_CFG_MASK))
        return 0;

    div = ((regval >> CLOCK_SDDAC_CLK_DIV_NUM_CFG_SHIFT)
            & CLOCK_SDDAC_CLK_DIV_NUM_CFG_MASK) + 1;

    regval_osr = __raw_readl(reg + CLOCK_DAC_FADEOUT_CTRL);
    regval_osr = (regval_osr >> CLOCK_DAC_OSR_CFG_SHIFT) & CLOCK_DAC_OSR_CFG_MASK;
    switch (regval_osr) {
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

    rate = parent_rate / div / osr;

    return rate;   
}
#else
unsigned long sddac_get_clk(
        struct ak_factor_clk* factor_clk, unsigned long parent_rate)
{
    int div, osr;
    u32 regval_div, regval_osr;
    unsigned long sddac_clk;
    void __iomem* reg = factor_clk ? (factor_clk->reg) : AK_VA_SYSCTRL;

    regval_div = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
    if (!((regval_div >> CLOCK_SDDAC_CLK_EN_CFG_SHIFT)
                & CLOCK_SDDAC_CLK_EN_CFG_MASK))
        return 0;

    regval_div = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
    div = ((regval_div >> CLOCK_SDDAC_CLK_DIV_NUM_CFG_SHIFT)
            & CLOCK_SDDAC_CLK_DIV_NUM_CFG_MASK)
        + 1;

    regval_osr = __raw_readl(reg + CLOCK_DAC_FADEOUT_CTRL);
    regval_osr
        = (regval_osr >> CLOCK_DAC_OSR_CFG_SHIFT) & CLOCK_DAC_OSR_CFG_MASK;
    switch (regval_osr) {
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

    pr_debug("parent:%lu,"
            "div:%d osr:%d sddac_clk:%lu\n",
            parent_rate, div, regval_osr, sddac_clk);

    return sddac_clk;
}
#endif

unsigned long sfc_phy_get_clk(
    struct ak_factor_clk* factor_clk, unsigned long parent_rate)
{
    void __iomem* reg = factor_clk ? (factor_clk->reg) : AK_VA_SYSCTRL;
    u32 regval;
    int div;
    unsigned long rate;

    regval = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);

    if (!((regval >> CLOCK_SFC_PHY_CLK_EN_CFG_SHIFT)
            & CLOCK_SFC_PHY_CLK_EN_CFG_MASK))
        return 0;

    div    = ((regval >> CLOCK_SFC_PHY_CLK_DIV_NUM_CFG_SHIFT)
              & CLOCK_SFC_PHY_CLK_DIV_NUM_CFG_MASK)
        + 1;

    rate = parent_rate / div;

    return rate;
}

unsigned long gclk_get_clk(
    struct ak_factor_clk* factor_clk, unsigned long parent_rate)
{
    void __iomem* reg = factor_clk ? (factor_clk->reg) : AK_VA_SYSCTRL;
    u32 regval;
    int div;
    unsigned long rate;

    regval = __raw_readl(reg + CLOCK_PLL2_CTRL);
    div    = ((regval >> CLOCK_GCLK_DIV_NUM_CFG_SHIFT)
              & CLOCK_GCLK_DIV_NUM_CFG_MASK)
        + 1;

    rate = parent_rate / div / 2;

    return rate;
}

unsigned long ess_vclk_get_clk(struct ak_factor_clk* factor_clk,
    unsigned long parent_rate_pll1, unsigned long parent_rate_pll2,
    unsigned long parent_rate_pll3)
{
    void __iomem* reg = factor_clk ? (factor_clk->reg) : AK_VA_SYSCTRL;
    u32 regval;
    int sel;
    int div;
    unsigned long rate;
    unsigned long parent_rate;

    regval = __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL);

    if (!((regval >> CLOCK_ENC_CLK_EN_CFG_SHIFT)
            & CLOCK_ISP_NPU_ENC_EN_CFG_MASK))
        return 0;

    sel = (regval >> CLOCK_ENC_CLK_MUX_NUM_CFG_SHIFT)
        & CLOCK_ISP_NPU_ENC_MUX_NUM_CFG_MASK;
    if (sel == 0)
        parent_rate = parent_rate_pll1;
    else if (sel == 1)
        parent_rate = parent_rate_pll2;
    else if (sel == 2)
        parent_rate = parent_rate_pll3;

    div = ((regval >> CLOCK_ENC_CLK_DIV_NUM_CFG_SHIFT)
              & CLOCK_ISP_NPU_ENC_DIV_NUM_CFG_MASK)
        + 1;

    rate = parent_rate / div;

    return rate;
}

unsigned long nss_vclk_get_clk(struct ak_factor_clk* factor_clk,
    unsigned long parent_rate_pll1, unsigned long parent_rate_pll2,
    unsigned long parent_rate_pll3)
{
    void __iomem* reg = factor_clk ? (factor_clk->reg) : AK_VA_SYSCTRL;
    u32 regval;
    int sel;
    int div;
    unsigned long rate;
    unsigned long parent_rate;

    regval = __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL);

    if (!((regval >> CLOCK_NPU_CLK_EN_CFG_SHIFT)
            & CLOCK_ISP_NPU_ENC_EN_CFG_MASK))
        return 0;

    sel = (regval >> CLOCK_NPU_CLK_MUX_NUM_CFG_SHIFT)
        & CLOCK_ISP_NPU_ENC_MUX_NUM_CFG_MASK;
    if (sel == 0)
        parent_rate = parent_rate_pll1;
    else if (sel == 1)
        parent_rate = parent_rate_pll2;
    else if (sel == 2)
        parent_rate = parent_rate_pll3;

    div = ((regval >> CLOCK_NPU_CLK_DIV_NUM_CFG_SHIFT)
              & CLOCK_ISP_NPU_ENC_DIV_NUM_CFG_MASK)
        + 1;

    rate = parent_rate / div;

    return rate;
}

unsigned long iss_vclk_get_clk(struct ak_factor_clk* factor_clk,
    unsigned long parent_rate_pll1, unsigned long parent_rate_pll2,
    unsigned long parent_rate_pll3)
{
    void __iomem* reg = factor_clk ? (factor_clk->reg) : AK_VA_SYSCTRL;
    u32 regval;
    int sel;
    int div;
    unsigned long rate;
    unsigned long parent_rate;

    regval = __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL);

    if (!((regval >> CLOCK_ISP_CLK_EN_CFG_SHIFT)
            & CLOCK_ISP_NPU_ENC_EN_CFG_MASK))
        return 0;

    sel = (regval >> CLOCK_ISP_CLK_MUX_NUM_CFG_SHIFT)
        & CLOCK_ISP_NPU_ENC_MUX_NUM_CFG_MASK;
    if (sel == 0)
        parent_rate = parent_rate_pll1;
    else if (sel == 1)
        parent_rate = parent_rate_pll2;
    else if (sel == 2)
        parent_rate = parent_rate_pll3;

    div = ((regval >> CLOCK_ISP_CLK_DIV_NUM_CFG_SHIFT)
              & CLOCK_ISP_NPU_ENC_DIV_NUM_CFG_MASK)
        + 1;

    rate = parent_rate / div;

    return rate;
}

/*
   static unsigned long sdadc_hs_get_clk(
   struct ak_factor_clk* factor_clk, unsigned long parent_rate)
   {
   int div;
   u32 regval_div;
   unsigned long sdadc_hs_clk;
   void __iomem* reg = factor_clk ? (factor_clk->reg) : AK_VA_SYSCTRL;

   regval_div = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
   if (!((regval_div >> CLOCK_SDADC_HS_CLK_EN_CFG_SHIFT)
   & CLOCK_SDADC_HS_CLK_EN_CFG_MASK))
   return 0;

   regval_div = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
   div = ((regval_div >> CLOCK_SDADC_HS_CLK_DIV_NUM_CFG_SHIFT)
   & CLOCK_SDADC_HS_CLK_DIV_NUM_CFG_MASK);

   sdadc_hs_clk = parent_rate / (div + 1);

   pr_debug("parent:%lu,"
   "div:%d sdadc_hs_clk:%lu\n",
   parent_rate, div, sdadc_hs_clk);

   return sdadc_hs_clk;
   }

   static unsigned long sddac_hs_get_clk(
   struct ak_factor_clk* factor_clk, unsigned long parent_rate)
   {
   int div;
   u32 regval_div;
   unsigned long sddac_hs_clk;
   void __iomem* reg = factor_clk ? (factor_clk->reg) : AK_VA_SYSCTRL;

   regval_div = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
   if (!((regval_div >> CLOCK_SDDAC_HS_CLK_EN_CFG_SHIFT)
   & CLOCK_SDDAC_HS_CLK_EN_CFG_MASK))
   return 0;

   regval_div = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);

   div = ((regval_div >> CLOCK_SDDAC_HS_CLK_DIV_NUM_CFG_SHIFT)
   & CLOCK_SDDAC_HS_CLK_DIV_NUM_CFG_MASK);

   sddac_hs_clk = parent_rate / (div + 1);

   pr_debug("parent:%lu,"
   "div:%d sddac_hs_clk:%lu\n",
   parent_rate, div, sddac_hs_clk);

   return sddac_hs_clk;
   }
   */
/* for isp pllx */
static int ak_factor_pllx_isp_set_rate(
        int pllid, struct clk_hw* hw, unsigned long rate,
        unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 factor;
#if defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A)
    int pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL3_DIVIDER;
#endif
    void __iomem* reg = factor_clk->reg;
    pr_debug("enter %s, parent_rate = %lu\n", __func__, parent_rate);

    if (parent_rate > rate)
        factor = parent_rate / rate - 1;
    else
        factor = 0;
#if defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A)
    if (1 == pllid)
        pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL1_DIVIDER;
    else if (2 == pllid)
        pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL2_DIVIDER;
    else if (3 == pllid)
        pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL3_DIVIDER;

    while (parent_rate/(factor+1) > ISP_FREQ_MAX) {
        pr_err("%s Error:preset ISP frequency %lu is larger than %ld,"
                "now freq_div_factor(%d)+1 = %d, actual frequency is %ld!\n",
                __func__,parent_rate / (factor+1),
                ISP_FREQ_MAX, factor,factor+1,parent_rate/(factor+1+1));
        factor++;
        if (factor >= ISP_FREQ_MAX_DIV_FACTOR) {
            factor = ISP_FREQ_MAX_DIV_FACTOR;
            pr_err("ISP freq max div factor is %d,"
                    "actual frequency is %ld\n",ISP_FREQ_MAX_DIV_FACTOR,
                    parent_rate/(factor+1));
            break;
        }
    }

    ak_clk_mux_and_int_divider_freq((reg + CLOCK_ISP_NPU_ENC_CTRL),
        pll_divider, CLOCK_ISP_CLK_MUX_NUM_CFG_SHIFT,
        CLOCK_ISP_NPU_ENC_MUX_NUM_CFG_MASK, -1, CLOCK_ISP_CLK_EN_CFG_SHIFT,
        factor, CLOCK_ISP_CLK_DIV_NUM_CFG_SHIFT,
        CLOCK_ISP_NPU_ENC_DIV_NUM_CFG_MASK);
#else
    //ensure that preset frequency is less than the MAX frequency
    while (parent_rate/(factor+1) > ISP_FREQ_MAX) {
        pr_err("%s Error:preset ISP frequency %lu is larger than %ld,"
                "now freq_div_factor(%d)+1 = %d, actual frequency is %ld!\n",
                __func__,parent_rate / (factor+1),
                ISP_FREQ_MAX, factor,factor+1,parent_rate/(factor+1+1));
        factor++;
        if (factor >= ISP_FREQ_MAX_DIV_FACTOR) {
            factor = ISP_FREQ_MAX_DIV_FACTOR;
            pr_err("ISP freq max div factor is %d,"
                    "actual frequency is %ld\n",ISP_FREQ_MAX_DIV_FACTOR,
                    parent_rate/(factor+1));
            break;
        }
    }

    ak_adjust_clk_int_divider_freq((reg + CLOCK_ISP_NPU_ENC_CTRL), -1,
        CLOCK_ISP_CLK_EN_CFG_SHIFT, factor, CLOCK_ISP_CLK_DIV_NUM_CFG_SHIFT,
        CLOCK_ISP_NPU_ENC_DIV_NUM_CFG_MASK);

#endif	
    pr_debug("%s: ISP CLK %luMHz->%luMhz(%u)\n", __func__, parent_rate / MHz,
            rate / MHz, factor);
    pr_debug("%s: CSI_SCLK_MAC_OPCLK_CTRL 0x%x\n", __func__,
            __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL));

    return 0;
}

#if defined (CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130)
/*
 * ak_factor_pllx_pp_set_rate
 *
 *
 */
static int __maybe_unused ak_factor_pllx_pp_set_rate(
        int pllid, struct clk_hw* hw,
        unsigned long rate, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 factor;
    void __iomem* reg = factor_clk->reg;
    int pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL3_DIVIDER;
    pr_info("enter %s, parent_rate = %lu\n", __func__, parent_rate);

    if (parent_rate > rate)
        factor = parent_rate / rate - 1;
    else
        factor = 0;

    if (1 == pllid)
        pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL1_DIVIDER;
    else if (2 == pllid)
        pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL2_DIVIDER;
    else if (3 == pllid)
        pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL3_DIVIDER;

    ak_clk_mux_and_int_divider_freq((reg + CLOCK_ISP_NPU_ENC_CTRL),
            pll_divider, CLOCK_PP_CLK_MUX_NUM_CFG_SHIFT,
            CLOCK_ISP_NPU_ENC_MUX_NUM_CFG_MASK, -1, CLOCK_PP_CLK_EN_CFG_SHIFT,
            factor, CLOCK_PP_CLK_DIV_NUM_CFG_SHIFT,
            CLOCK_ISP_NPU_ENC_DIV_NUM_CFG_MASK);

    // ak_adjust_clk_int_divider_freq((reg + CLOCK_ISP_NPU_ENC_CTRL), -1,
    //   CLOCK_PP_CLK_EN_CFG_SHIFT, factor, CLOCK_PP_CLK_DIV_NUM_CFG_SHIFT,
    //   CLOCK_ISP_NPU_ENC_DIV_NUM_CFG_MASK);
    pr_info("%s: PP CLK %luMHz->%luMhz(%u)\n", __func__, parent_rate / MHz,
            rate / MHz, factor);
    pr_info("%s: 0x11c 0x%x,factor:%d\n", __func__,
            __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL), factor);

    return 0;
}

#define CSI_CLK_SEL_SHIFT(x) ((x) * 4)
#define CSI_CLK_OPEN_SHIFT(x) ((x)*4 + 2)
#define CSI_CLK_READY_SHIFT(x) ((x)*4 + 3)
#define CSI_CLK_READY_MASK (1)
#define CSI_CLK_OPEN_MASK  (1)
#define CSI_CLK_SEL_MASK   (3)
#endif

/* for csi pllx */
/*
 * @BRIEF        ak_factor_pllx_csi0_set_rate
 * @PARAM[in]    pllid
 * @PARAM[in]    hw
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       int
 * @RETVAL       0
 */
static int ak_factor_pllx_csi0_set_rate(
        int pllid, struct clk_hw* hw,
        unsigned long rate, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 factor;
#if defined(CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130)
    u32 regval;
#endif
    void __iomem* reg = factor_clk->reg;
    int pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL3_DIVIDER;
#if defined(CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130)
    int timeout = CLOCK_FREQ_ADJ_WAITING_MAX_NUM;
#endif

    pr_debug("enter %s, parent_rate = %lu\n", __func__, parent_rate);

    if (parent_rate > rate)
        factor = parent_rate / rate - 1;
    else
        factor = 0;

    if (1 == pllid)
        pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL1_DIVIDER;
    else if (2 == pllid)
        pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL2_DIVIDER;
    else if (3 == pllid)
        pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL3_DIVIDER;

    ak_clk_mux_and_int_divider_freq(
        (reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL), pll_divider,
        CLOCK_CSI0_SCLK_MUX_NUM_CFG_SHIFT, CLOCK_CSI0_SCLK_MUX_NUM_CFG_MASK,-1,
        CLOCK_CSI0_SCLK_EN_CFG_SHIFT, factor,CLOCK_CSI0_SCLK_DIV_NUM_CFG_SHIFT,
        CLOCK_CSI0_SCLK_DIV_NUM_CFG_MASK);

#if defined(CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130)
    regval = __raw_readl(reg + CLOCK_CSI_CTRL);
    regval &= ~(CSI_CLK_OPEN_MASK << CSI_CLK_OPEN_SHIFT(0));
    __raw_writel(regval, reg + CLOCK_CSI_CTRL);

    regval = __raw_readl(reg + CLOCK_CSI_CTRL);
    regval &= ~(CSI_CLK_SEL_MASK << CSI_CLK_SEL_SHIFT(0));
    regval |= (1 << CSI_CLK_SEL_SHIFT(0));
    __raw_writel(regval, reg + CLOCK_CSI_CTRL);

    do {
        timeout--;
        regval = ((__raw_readl(reg + CLOCK_CSI_CTRL))>>CSI_CLK_READY_SHIFT(0))
            & CSI_CLK_READY_MASK;
    } while ((regval == 0) && (timeout > 0));

    if (timeout <= 0) {
        pr_err("Waiting CSI%d ready timeout!\n", 0);
    }

    regval = __raw_readl(reg + CLOCK_CSI_CTRL);
    regval |= (CSI_CLK_OPEN_MASK << CSI_CLK_OPEN_SHIFT(0));
    __raw_writel(regval, reg + CLOCK_CSI_CTRL);

    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
#endif

    pr_debug("%s: CSI CLK %luMHz->%luMhz(%u)\n", __func__, parent_rate / MHz,
            rate / MHz, factor);
    pr_debug("%s: CLOCK_CSI_SCLK_MAC_OPCLK_CTRL 0x%x\n", __func__,
            __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL));

    return 0;
}

#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918AV130) \
    || defined(CONFIG_MACH_KM01A)

/*
 * @BRIEF        ak_factor_pllx_csi1_set_rate
 * @PARAM[in]    pllid
 * @PARAM[in]    hw
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       int
 * @RETVAL       0
 */
static int ak_factor_pllx_csi1_set_rate(
        int pllid, struct clk_hw* hw,
        unsigned long rate, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 factor;
#if defined(CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130)
    u32 regval;
    int timeout = CLOCK_FREQ_ADJ_WAITING_MAX_NUM;
#endif
    void __iomem* reg = factor_clk->reg;
    int pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL3_DIVIDER;
    pr_debug("enter %s, parent_rate = %lu\n", __func__, parent_rate);

    if (parent_rate > rate)
        factor = parent_rate / rate - 1;
    else
        factor = 0;

    if (1 == pllid)
        pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL1_DIVIDER;
    else if (2 == pllid)
        pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL2_DIVIDER;
    else if (3 == pllid)
        pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL3_DIVIDER;

    ak_clk_mux_and_int_divider_freq(
        (reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL), pll_divider,
        CLOCK_CSI1_SCLK_MUX_NUM_CFG_SHIFT, CLOCK_CSI1_SCLK_MUX_NUM_CFG_MASK,-1,
        CLOCK_CSI1_SCLK_EN_CFG_SHIFT, factor,CLOCK_CSI1_SCLK_DIV_NUM_CFG_SHIFT,
        CLOCK_CSI1_SCLK_DIV_NUM_CFG_MASK);

#if defined(CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130)
    regval = __raw_readl(reg + CLOCK_CSI_CTRL);
    regval &= ~(CSI_CLK_OPEN_MASK << CSI_CLK_OPEN_SHIFT(1));
    __raw_writel(regval, reg + CLOCK_CSI_CTRL);

    regval = __raw_readl(reg + CLOCK_CSI_CTRL);
    regval &= ~(CSI_CLK_SEL_MASK << CSI_CLK_SEL_SHIFT(1));
    regval |= (2 << CSI_CLK_SEL_SHIFT(1));
    __raw_writel(regval, reg + CLOCK_CSI_CTRL);

    do {
        timeout--;
        regval = ((__raw_readl(reg + CLOCK_CSI_CTRL))>>CSI_CLK_READY_SHIFT(1))
            & CSI_CLK_READY_MASK;
    } while ((regval == 0) && (timeout > 0));

    if (timeout <= 0) {
        pr_err("Waiting CSI%d ready timeout!\n", 0);
    }

    regval = __raw_readl(reg + CLOCK_CSI_CTRL);
    regval |= (CSI_CLK_OPEN_MASK << CSI_CLK_OPEN_SHIFT(1));
    __raw_writel(regval, reg + CLOCK_CSI_CTRL);

    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
#endif

    pr_debug("%s: CSI1 SCLK %luMHz->%luMhz(%u)\n", __func__, parent_rate / MHz,
            rate / MHz, factor);
    pr_debug("%s: CSI_SCLK_MAC_OPCLK_CTRL 0x%x\n", __func__,
            __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL));

    return 0;
}
#endif

#if defined(CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130) 
/*
 * @BRIEF        ak_factor_pllx_csi1_set_rate
 * @PARAM[in]    pllid
 * @PARAM[in]    hw
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       int
 * @RETVAL       0
 */
static int ak_factor_pllx_opclk_set_rate(
        int pllid, struct clk_hw* hw,
        unsigned long rate, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 factor;
    u32 regval;
    int timeout = CLOCK_FREQ_ADJ_WAITING_MAX_NUM;
    void __iomem* reg = factor_clk->reg;
    int pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL3_DIVIDER;

    if (parent_rate > rate)
        factor = parent_rate / rate - 1;
    else
        factor = 0;

    if (1 == pllid)
        pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL1_DIVIDER;
    else if (2 == pllid)
        pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL2_DIVIDER;
    else if (3 == pllid)
        pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL3_DIVIDER;

    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
    regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
    regval &= ~((CLOCK_MAC_OPCLK_PLL_SEL_MASK << CLOCK_MAC_OPCLK_PLL_SEL_SHIFT) |
                (CLOCK_MAC_DIV_NUM_CFG_MASK << CLOCK_MAC_DIV_NUM_CFG_SHIFT));
    regval |= (pll_divider << CLOCK_MAC_OPCLK_PLL_SEL_SHIFT) | (factor << CLOCK_MAC_DIV_NUM_CFG_SHIFT);
    __raw_writel(regval, reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
    do {
        timeout--;
        regval = ((__raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL)) >> CLOCK_MAC_OPCLK_SOURCE_READY_SHIFT)
            & CLOCK_MAC_OPCLK_SOURCE_READY_MASK;
    } while ((regval == 0) && (timeout > 0));

    if (timeout <= 0) {
        pr_err("Waiting opclk source ready timeout!\n");
    }

    regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
    regval |= (1 << CLOCK_MAC_OPCLK_SOURCE_EN_CFG_SHIFT);
    __raw_writel(regval, reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
    do {
        timeout--;
        regval = ((__raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL)) >> CLOCK_MAC_OPCLK_READ_SHIFT)
            & CLOCK_MAC_OPCLK_READ_MASK;
    } while ((regval == 0) && (timeout > 0));

    if (timeout <= 0) {
        pr_err("Waiting opclk ready timeout!\n");
    }

    return 0;
}

#endif

/*
 * @BRIEF        ak_factor_pllx_enc_set_rate
 * @PARAM[in]    pllid
 * @PARAM[in]    hw
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       int
 * @RETVAL       0
 */
static int ak_factor_pllx_enc_set_rate(
        int pllid, struct clk_hw* hw,
        unsigned long rate, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 factor;
    void __iomem* reg = factor_clk->reg;
    int pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL3_DIVIDER;
    pr_debug("enter %s, parent_rate = %lu\n", __func__, parent_rate);

    if (parent_rate > rate)
        factor = parent_rate / rate - 1;
    else
        factor = 0;

    //ensure that preset frequency is less than the MAX frequency
    while (parent_rate/(factor+1) > VENC_FREQ_MAX) {
        pr_err("%s Error:preset VENC frequency %lu is larger than %ld,"
                "now freq_div_factor(%d)+1 = %d, actual frequency is %ld!\n",
                __func__,parent_rate / (factor+1),
                VENC_FREQ_MAX, factor,factor+1,parent_rate/(factor+1+1));
        factor++;
        if (factor >= VENC_FREQ_MAX_DIV_FACTOR) {
            factor = VENC_FREQ_MAX_DIV_FACTOR;
            pr_err("VENC freq max div factor is %d,"
                    "actual frequency is %ld\n",VENC_FREQ_MAX_DIV_FACTOR,
                    parent_rate/(factor+1));
            break;
        }
    }

    if (1 == pllid)
        pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL1_DIVIDER;
    else if (2 == pllid)
        pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL2_DIVIDER;
    else if (3 == pllid)
        pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL3_DIVIDER;

    ak_clk_mux_and_int_divider_freq((reg + CLOCK_ISP_NPU_ENC_CTRL),
            pll_divider, CLOCK_ENC_CLK_MUX_NUM_CFG_SHIFT,
            CLOCK_ISP_NPU_ENC_MUX_NUM_CFG_MASK, -1,CLOCK_ENC_CLK_EN_CFG_SHIFT,
            factor, CLOCK_ENC_CLK_DIV_NUM_CFG_SHIFT,
            CLOCK_ISP_NPU_ENC_DIV_NUM_CFG_MASK);

    pr_debug("%s: ENC CLK %luMHz->%luMhz(%u)\n", __func__, parent_rate / MHz,
            rate / MHz, factor);
    pr_debug("%s: CLOCK_ISP_NPU_ENC_CTRL 0x%x\n", __func__,
            __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL));

    return 0;
}

#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918AV130) \
    || defined(CONFIG_MACH_KM01A)

/*
 * @BRIEF        ak_factor_pllx_npu_set_rate
 * @PARAM[in]    pllid
 * @PARAM[in]    hw
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       int
 * @RETVAL       0
 */
static int ak_factor_pllx_npu_set_rate(
        int pllid, struct clk_hw* hw,
        unsigned long rate, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 factor;
    void __iomem* reg = factor_clk->reg;
    int pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL3_DIVIDER;
    pr_debug("enter %s, parent_rate = %lu\n", __func__, parent_rate);

    if (parent_rate > rate)
        factor = parent_rate / rate - 1;
    else
        factor = 0;

    //ensure that preset frequency is less than the MAX frequency
    while (parent_rate/(factor+1) > NPU_FREQ_MAX) {
        pr_err("%s Error:preset NPU frequency %lu is larger than %ld,"
                "now freq_div_factor(%d)+1 = %d, actual frequency is %ld!\n",
                __func__,parent_rate / (factor+1),
                NPU_FREQ_MAX, factor,factor+1,parent_rate/(factor+1+1));
        factor++;
        if (factor >= NPU_FREQ_MAX_DIV_FACTOR) {
            factor = NPU_FREQ_MAX_DIV_FACTOR;
            pr_err("NPU freq max div factor is %d,"
                    "actual frequency is %ld\n",NPU_FREQ_MAX_DIV_FACTOR,
                    parent_rate/(factor+1));
            break;
        }
    }

    if (1 == pllid)
        pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL1_DIVIDER;
    else if (2 == pllid)
        pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL2_DIVIDER;
    else if (3 == pllid)
        pll_divider = CLOCK_ISP_NPU_ENC_SRC_SEL_PLL3_DIVIDER;

    ak_clk_mux_and_int_divider_freq((reg + CLOCK_ISP_NPU_ENC_CTRL),
        pll_divider, CLOCK_NPU_CLK_MUX_NUM_CFG_SHIFT,
        CLOCK_ISP_NPU_ENC_MUX_NUM_CFG_MASK, -1, CLOCK_NPU_CLK_EN_CFG_SHIFT,
        factor, CLOCK_NPU_CLK_DIV_NUM_CFG_SHIFT,
        CLOCK_ISP_NPU_ENC_DIV_NUM_CFG_MASK);

    pr_debug("%s: NPU CLK %luMHz->%luMhz(%u)\n", __func__, parent_rate / MHz,
            rate / MHz, factor);
    pr_debug("%s: CLOCK_ISP_NPU_ENC_CTRL 0x%x\n", __func__,
            __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL));

    return 0;
}
#endif

#if defined(CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130)
/*
 * @BRIEF       ak_get_pll_clk
 * @PARAM[in]   reg
 * @RETURN      unsigned long
 * @RETVAL      pll_clk * KHz
 */
static unsigned long ak_get_pll_clk(void __iomem *reg)
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
 * ak_get_pllx_clk
 *
 *
 */
static unsigned long ak_get_pllx_clk(void __iomem *reg, unsigned int pllid)
{
    switch (pllid) {
        case 1:
            return ak_get_pll_clk(reg + CLOCK_PLL1_CTRL);
        case 2:
            return ak_get_pll_clk(reg + CLOCK_PLL2_CTRL);
        case 3:
            return ak_get_pll_clk(reg + CLOCK_PLL3_CTRL);
        default:
            return 0;
    }
}

/*
 * @BRIEF        ak_factor_pllx_csix_recalc_rate
 * @PARAM[in]    hw
 * @PARAM[in]    parent_rate
 * @RETURN       unsigned long
 * @RETVAL       recalc_rate
 */
static unsigned long ak_factor_pllx_csix_recalc_rate(
        struct clk_hw* hw, unsigned long parent_rate, unsigned int csi_id)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    void __iomem* reg = factor_clk->reg;
    u32 regval, factor = 0, pllid;
    unsigned long recalc_rate = 0;

    regval = __raw_readl(reg + CLOCK_CSI_CTRL);
    if (regval & (CSI_CLK_OPEN_MASK << CSI_CLK_OPEN_SHIFT(csi_id))) {
        pllid = (regval >> CSI_CLK_SEL_SHIFT(csi_id)) & CSI_CLK_SEL_MASK;
        switch (pllid) {
            case 0:
                recalc_rate = 24 * MHz;
                break;
            case 1:
            case 2:
                regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
                regval = (regval >> CLOCK_CSI0_SCLK_EN_CFG_SHIFT)
                    & CLOCK_CSI0_SCLK_EN_CFG_MASK;
                if (regval) {
                    regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
                    factor = (regval >> CLOCK_CSI0_SCLK_MUX_NUM_CFG_SHIFT)
                        & CLOCK_CSI0_SCLK_MUX_NUM_CFG_MASK;
                    recalc_rate = ak_get_pllx_clk(reg, pllid) / (factor + 1);
                }
            default:
                pr_err("CLOCK_CSI_CTRL clock%d selection not invalid!\n",
                        csi_id);
        }
    }
    return recalc_rate;
}
#endif

/*
 * @BRIEF        ak_factor_pllx_csi0_recalc_rate
 * @PARAM[in]    hw
 * @PARAM[in]    parent_rate
 * @RETURN       unsigned long
 * @RETVAL       recalc_rate
 */
static unsigned long ak_factor_pllx_csi0_recalc_rate(
        struct clk_hw* hw, unsigned long parent_rate)
{
#if !defined(CONFIG_MACH_KM01A) && !defined(CONFIG_MACH_AK3918AV130)
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    void __iomem* reg = factor_clk->reg;
    u32 regval, factor = 0;
    unsigned long recalc_rate;

    regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
    regval = (regval >> CLOCK_CSI0_SCLK_EN_CFG_SHIFT)
        & CLOCK_CSI0_SCLK_EN_CFG_MASK;
    if (regval) {
        regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
        regval = (regval >> CLOCK_CSI0_SCLK_MUX_NUM_CFG_SHIFT)
            & CLOCK_CSI0_SCLK_MUX_NUM_CFG_MASK;

        if (regval != CLOCK_CSI_SRC_SEL_OSC24M) {
            // Use the PLL as the source clock of the sensor0 SCLK
            regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
            factor = (regval >> CLOCK_CSI0_SCLK_DIV_NUM_CFG_SHIFT)
                & CLOCK_CSI0_SCLK_DIV_NUM_CFG_MASK;
            recalc_rate = parent_rate / (factor + 1);
        } else {
            // Use the external 24MHz as the source clock of the sensor0 SCLK
            recalc_rate = 24 * MHz;
        }
    } else {
        recalc_rate = 0;
    }
    return recalc_rate;
#else
    return ak_factor_pllx_csix_recalc_rate(hw, parent_rate, 0);
#endif
}

#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918AV130) \
    || defined(CONFIG_MACH_KM01A)

/*
 * @BRIEF        ak_factor_pllx_csi1_recalc_rate
 * @PARAM[in]    hw
 * @PARAM[in]    parent_rate
 * @RETURN       unsigned long
 * @RETVAL       recalc_rate
 */
static unsigned long ak_factor_pllx_csi1_recalc_rate(
        struct clk_hw* hw, unsigned long parent_rate)
{
#if !defined(CONFIG_MACH_KM01A) && !defined(CONFIG_MACH_AK3918AV130)
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    void __iomem* reg = factor_clk->reg;
    u32 regval, factor = 0;
    unsigned long recalc_rate;

    regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
    regval = (regval >> CLOCK_CSI1_SCLK_EN_CFG_SHIFT)
        & CLOCK_CSI1_SCLK_EN_CFG_MASK;
    if (regval) {
        regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
        regval = (regval >> CLOCK_CSI1_SCLK_MUX_NUM_CFG_SHIFT)
            & CLOCK_CSI1_SCLK_MUX_NUM_CFG_MASK;

        if (regval != CLOCK_CSI_SRC_SEL_OSC24M) {
            // Use the PLL as the source clock of the sensor0 SCLK
            regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
            factor = (regval >> CLOCK_CSI1_SCLK_DIV_NUM_CFG_SHIFT)
                & CLOCK_CSI1_SCLK_DIV_NUM_CFG_MASK;
            recalc_rate = parent_rate / (factor + 1);
        } else {
            // Use the external 24MHz as the source clock of the sensor0 SCLK
            recalc_rate = 24 * MHz;
        }

    } else {
        recalc_rate = 0;
    }
    return recalc_rate;
#else
    return ak_factor_pllx_csix_recalc_rate(hw, parent_rate, 1);
#endif
}
#endif

/*
 * @BRIEF        ak_factor_pllx_enc_recalc_rate
 * @PARAM[in]    hw
 * @PARAM[in]    parent_rate
 * @RETURN       unsigned long
 * @RETVAL       recalc_rate
 */
static unsigned long ak_factor_pllx_enc_recalc_rate(
        struct clk_hw* hw, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    void __iomem* reg = factor_clk->reg;
    u32 regval, factor = 0;
    unsigned long recalc_rate;

    regval = __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL);
    regval = (regval >> CLOCK_ENC_CLK_EN_CFG_SHIFT)
        & CLOCK_ISP_NPU_ENC_EN_CFG_MASK;
    if (regval) {
        regval = __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL);
        regval = (regval >> CLOCK_ENC_CLK_MUX_NUM_CFG_SHIFT)
            & CLOCK_ISP_NPU_ENC_MUX_NUM_CFG_MASK;

        if (regval < 3) {
            // Use the PLLx_DIVIDER
            regval = __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL);
            factor = (regval >> CLOCK_ENC_CLK_DIV_NUM_CFG_SHIFT)
                & CLOCK_ISP_NPU_ENC_DIV_NUM_CFG_MASK;
            recalc_rate = parent_rate / (factor + 1);
        } else {
            // Use the PLLx
            recalc_rate = parent_rate;
        }

    } else {
        recalc_rate = 0;
    }
    return recalc_rate;
}

#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918AV130) \
    || defined(CONFIG_MACH_KM01A)

/*
 * @BRIEF        ak_factor_pllx_npu_recalc_rate
 * @PARAM[in]    hw
 * @PARAM[in]    parent_rate
 * @RETURN       unsigned long
 * @RETVAL       recalc_rate
 */
static unsigned long ak_factor_pllx_npu_recalc_rate(
        struct clk_hw* hw, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    void __iomem* reg = factor_clk->reg;
    u32 regval, factor = 0;
    unsigned long recalc_rate;

    regval = __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL);
    regval = (regval >> CLOCK_NPU_CLK_EN_CFG_SHIFT)
        & CLOCK_ISP_NPU_ENC_EN_CFG_MASK;
    if (regval) {
        regval = __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL);
        regval = (regval >> CLOCK_NPU_CLK_MUX_NUM_CFG_SHIFT)
            & CLOCK_ISP_NPU_ENC_MUX_NUM_CFG_MASK;

        if (regval < 3) {
            // Use the PLLx_DIVIDER
            regval = __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL);
            factor = (regval >> CLOCK_NPU_CLK_DIV_NUM_CFG_SHIFT)
                & CLOCK_ISP_NPU_ENC_DIV_NUM_CFG_MASK;
            recalc_rate = parent_rate / (factor + 1);
        } else {
            // Use the PLLx
            recalc_rate = parent_rate;
        }
    } else {
        recalc_rate = 0;
    }

    return recalc_rate;
}
#endif

/*
 * @BRIEF        ak_factor_pllx_isp_recalc_rate
 * @PARAM[in]    hw
 * @PARAM[in]    parent_rate
 * @RETURN       unsigned long
 * @RETVAL       recalc_rate
 */
static unsigned long ak_factor_pllx_isp_recalc_rate(
        struct clk_hw* hw, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    void __iomem* reg = factor_clk->reg;
    u32 regval, factor = 0;
    unsigned long recalc_rate;

    regval = __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL);
    regval = (regval >> CLOCK_ISP_CLK_EN_CFG_SHIFT)
        & CLOCK_ISP_NPU_ENC_EN_CFG_MASK;
    if (regval) {
        regval = __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL);
        regval = (regval >> CLOCK_ISP_CLK_MUX_NUM_CFG_SHIFT)
            & CLOCK_ISP_NPU_ENC_MUX_NUM_CFG_MASK;

        if (regval < 3) {
            // Use the PLLx_DIVIDER
            regval = __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL);
            factor = (regval >> CLOCK_ISP_CLK_DIV_NUM_CFG_SHIFT)
                & CLOCK_ISP_NPU_ENC_DIV_NUM_CFG_MASK;
            recalc_rate = parent_rate / (factor + 1);
        } else {
            // Use the PLLx
            return parent_rate;
        }

    } else {
        recalc_rate = 0;
    }

    return recalc_rate;
}

#if defined (CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130)
/*
 * @BRIEF		 ak_factor_pllx_pp_recalc_rate
 * @PARAM[in]	 hw
 * @PARAM[in]	 parent_rate
 * @RETURN		 unsigned long
 * @RETVAL		 recalc_rate
 */
static unsigned long __maybe_unused ak_factor_pllx_pp_recalc_rate(
        struct clk_hw* hw, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    void __iomem* reg = factor_clk->reg;
    u32 regval, factor = 0;
    unsigned long recalc_rate;

    regval = __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL);
    regval = (regval >> CLOCK_PP_CLK_EN_CFG_SHIFT)
        & CLOCK_ISP_NPU_ENC_EN_CFG_MASK;
    if (regval) {
        regval = __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL);
        regval = (regval >> CLOCK_PP_CLK_MUX_NUM_CFG_SHIFT)
            & CLOCK_ISP_NPU_ENC_MUX_NUM_CFG_MASK;

        if (regval < 3) {
            // Use the PLLx_DIVIDER
            regval = __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL);
            factor = (regval >> CLOCK_PP_CLK_DIV_NUM_CFG_SHIFT)
                & CLOCK_ISP_NPU_ENC_DIV_NUM_CFG_MASK;
            recalc_rate = parent_rate / (factor + 1);
        } else {
            // Use the PLLx
            return parent_rate;
        }

    } else {
        recalc_rate = 0;
    }

    return recalc_rate;
}
#endif

/*
 * @BRIEF        ak_factor_pllx_isp_disable
 * @PARAM[in]    hw
 * @RETURN       void
 */
static void ak_factor_pllx_isp_disable(struct clk_hw* hw)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 regval;
    void __iomem* reg = factor_clk->reg;
    regval = __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL);
    regval &= ~(CLOCK_ISP_NPU_ENC_EN_CFG_MASK << CLOCK_ISP_CLK_EN_CFG_SHIFT);
    __raw_writel(regval, reg + CLOCK_ISP_NPU_ENC_CTRL);

    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
}

/*
 * @BRIEF        ak_factor_pllx_isp_enable
 * @PARAM[in]    hw
 * @RETURN       int
 * @RETVAL       0
 */
static int ak_factor_pllx_isp_enable(struct clk_hw* hw)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 regval;
    void __iomem* reg = factor_clk->reg;
    regval = __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL);
    regval |= (CLOCK_ISP_NPU_ENC_EN_CFG_MASK << CLOCK_ISP_CLK_EN_CFG_SHIFT);
    __raw_writel(regval, reg + CLOCK_ISP_NPU_ENC_CTRL);

    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
    return 0;
}

#if defined (CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130)
/*
 * @BRIEF        ak_factor_pllx_pp_disable
 * @PARAM[in]    hw
 * @RETURN       void
 */
static void __maybe_unused ak_factor_pllx_pp_disable(struct clk_hw* hw)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 regval;
    void __iomem* reg = factor_clk->reg;
    regval = __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL);
    regval &= ~(CLOCK_ISP_NPU_ENC_EN_CFG_MASK << CLOCK_PP_CLK_EN_CFG_SHIFT);
    __raw_writel(regval, reg + CLOCK_ISP_NPU_ENC_CTRL);

    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
}

/*
 * @BRIEF        ak_factor_pllx_isp_enable
 * @PARAM[in]    hw
 * @RETURN       int
 * @RETVAL       0
 */
static int __maybe_unused ak_factor_pllx_pp_enable(struct clk_hw* hw)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 regval;
    void __iomem* reg = factor_clk->reg;
    regval = __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL);
    regval |= (CLOCK_ISP_NPU_ENC_EN_CFG_MASK << CLOCK_PP_CLK_EN_CFG_SHIFT);
    __raw_writel(regval, reg + CLOCK_ISP_NPU_ENC_CTRL);

    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
    return 0;
}
#endif

/*
 * @BRIEF        ak_factor_pllx_enc_disable
 * @PARAM[in]    hw
 * @RETURN       void
 */
static void ak_factor_pllx_enc_disable(struct clk_hw* hw)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 regval;
    void __iomem* reg = factor_clk->reg;

    regval = __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL);
    regval &= ~(CLOCK_ISP_NPU_ENC_EN_CFG_MASK << CLOCK_ENC_CLK_EN_CFG_SHIFT);
    __raw_writel(regval, reg + CLOCK_ISP_NPU_ENC_CTRL);

    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
}

/*
 * @BRIEF        ak_factor_pllx_enc_enable
 * @PARAM[in]    hw
 * @RETURN       int
 * @RETVAL       0
 */
static int ak_factor_pllx_enc_enable(struct clk_hw* hw)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 regval;
    void __iomem* reg = factor_clk->reg;

    regval = __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL);
    regval |= (CLOCK_ISP_NPU_ENC_EN_CFG_MASK << CLOCK_ENC_CLK_EN_CFG_SHIFT);
    __raw_writel(regval, reg + CLOCK_ISP_NPU_ENC_CTRL);

    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
    return 0;
}

#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918AV130) \
    || defined(CONFIG_MACH_KM01A)

/*
 * @BRIEF        ak_factor_pllx_npu_disable
 * @PARAM[in]    hw
 * @RETURN       void
 */
static void ak_factor_pllx_npu_disable(struct clk_hw* hw)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 regval;
    void __iomem* reg = factor_clk->reg;

    regval = __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL);
    regval &= ~(CLOCK_ISP_NPU_ENC_EN_CFG_MASK << CLOCK_NPU_CLK_EN_CFG_SHIFT);
    __raw_writel(regval, reg + CLOCK_ISP_NPU_ENC_CTRL);

    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
}

/*
 * @BRIEF        ak_factor_pllx_npu_enable
 * @PARAM[in]    hw
 * @RETURN       int
 * @RETVAL       0
 */
static int ak_factor_pllx_npu_enable(struct clk_hw* hw)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 regval;
    void __iomem* reg = factor_clk->reg;

    regval = __raw_readl(reg + CLOCK_ISP_NPU_ENC_CTRL);
    regval |= (CLOCK_ISP_NPU_ENC_EN_CFG_MASK << CLOCK_NPU_CLK_EN_CFG_SHIFT);
    __raw_writel(regval, reg + CLOCK_ISP_NPU_ENC_CTRL);

    regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
    return 0;
}
#endif

/*
 * @BRIEF        ak_factor_pll2_clk_round_rate
 * @PARAM[in]    hw
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       long
 * @RETVAL       round_rate
 */
static long ak_factor_pll2_clk_round_rate(
        struct clk_hw* hw, unsigned long rate, unsigned long* parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    unsigned long round_rate = rate;

    pr_debug("factor round_rate %s(%d) rate %lu parent %luMHz\n",
            hw->init->name, factor_clk->id, round_rate, (*parent_rate) / MHz);

    return round_rate;
}

/*
 * @BRIEF        ak_factor_pll2_clk_set_rate
 * @PARAM[in]    hw
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       int
 * @RETVAL       0
 */
static int ak_factor_pll2_clk_set_rate(
        struct clk_hw* hw, unsigned long rate, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 factor;
    void __iomem* reg = factor_clk->reg;
    unsigned long flags;
    int ret;

    pr_debug("enter %s, parent_rate = %lu\n", __func__, parent_rate);

    if (rate == 0) {
        ak_factor_pll2_clk_disable(hw);
        return 0;
    }

    local_irq_save(flags);

    if (parent_rate > rate)
        factor = parent_rate / rate - 1;
    else
        factor = 0;

    switch (factor_clk->id) {
        case PLL2_FACTOR_CSI0_SCLK:
            ret = ak_factor_pllx_csi0_set_rate(PLL2, hw, rate, parent_rate);
            break;
#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918AV130) \
            || defined(CONFIG_MACH_KM01A)
        case PLL2_FACTOR_CSI1_SCLK:
            ret = ak_factor_pllx_csi1_set_rate(PLL2, hw, rate, parent_rate);
            break;
#endif
        case PLL2_FACTOR_MAC_OPCLK:
#if defined(CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130)
            ret = ak_factor_pllx_opclk_set_rate(PLL2, hw, rate, parent_rate);
#else
            ak_adjust_clk_even_divider_freq(
                (reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL),
                CLOCK_MAC_OPCLK_DIV_VLD_CFG_SHIFT,CLOCK_MAC_OPCLK_EN_CFG_SHIFT,
                factor, CLOCK_MAC_DIV_NUM_CFG_SHIFT,
                CLOCK_MAC_DIV_NUM_CFG_MASK);
#endif
            break;

        case PLL2_FACTOR_SPI0_CLK:
            if (rate > SFC_FREQ_MAX) {
                pr_err("SFC frequency %lu is larger than %ld"
                        ", use %ld instead!\n",
                        rate, SFC_FREQ_MAX, SFC_FREQ_MAX);
                rate = SFC_FREQ_MAX;
                if (parent_rate % (rate * 2)) {
                    factor = parent_rate / (rate * 2) + 1 - 1;
                } else
                    factor = parent_rate / (rate * 2) - 1;
            } else {

                // TODO need to reset spi or not.
                /*
                 * Please refer to PG SPI0 clock
                 * ASIC PLL -- spi_hsclk_div_num_cfg - /2 then be SPI_CLK
                 * NOTICE the "/2"
                 */
                factor = parent_rate / (rate * 2) - 1;
            }            

            ak_adjust_clk_int_divider_freq(
                (reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL),
                CLOCK_SPI0_HSDIV_VLD_CFG_SHIFT, CLOCK_SPI0_HSCLK_EN_CFG_SHIFT,
                factor, CLOCK_SPI0_HSCLK_DIV_NUM_CFG_SHIFT,
                CLOCK_SPI0_HSCLK_DIV_NUM_CFG_MASK);

            // ak_module_reset_by_clock(factor_clk->reg + CLOCK_SOFT_RESET,
            // CLOCK_SPI0HS_RST_CFG_SHIFT);
            pr_debug("SPI0 CLOCK_SDADDA_SPI0HS_SARADDA_CTRL 0x%x,rate = %lu\n",
                    __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL), rate);
            break;

        case PLL2_FACTOR_ENC:
            ret = ak_factor_pllx_enc_set_rate(PLL2, hw, rate, parent_rate);
            break;

#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918AV130) \
            || defined(CONFIG_MACH_KM01A)
        case PLL2_FACTOR_NPU:
            ret = ak_factor_pllx_npu_set_rate(PLL2, hw, rate, parent_rate);
            break;
#endif

        case PLL2_FACTOR_ISP:
            ret = ak_factor_pllx_isp_set_rate(2, hw, rate, parent_rate);
            break;

        case PLL2_FACTOR_DPHY_CFGCLK:
            ak_adjust_clk_even_divider_freq(
                (reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL),
                CLOCK_DPHY_CFG_CLK_VLD_CFG_SHIFT, CLOCK_DPHY_CFG_CLK_EN_SHIFT,
                factor, CLOCK_DPHY_CFG_CLK_DIV_NUM_CFG_SHIFT,
                CLOCK_DPHY_CFG_CLK_DIV_NUM_CFG_MASK);

            break;

        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            break;
    }
    local_irq_restore(flags);

    pr_debug("factor set_rate %s(%d) rate %lu parent %lu\n", hw->init->name,
            factor_clk->id, rate, parent_rate);

    return 0;
}
/*end of func*/

/*
 * @BRIEF        ak_factor_pll2_clk_recalc_rate
 * @PARAM[in]    hw
 * @PARAM[in]    parent_rate
 * @RETURN       unsigned long
 * @RETVAL       recalc_rate
 */
static unsigned long ak_factor_pll2_clk_recalc_rate(
        struct clk_hw* hw, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    void __iomem* reg = factor_clk->reg;
    u32 regval, factor = 0;
    unsigned long recalc_rate;
    unsigned long flags;

    pr_debug("%s %s(%d) parent %lu\n", __func__, hw->init->name, factor_clk->id,
            parent_rate);

    local_irq_save(flags);

    switch (factor_clk->id) {
        case PLL2_FACTOR_CSI0_SCLK:
            recalc_rate = ak_factor_pllx_csi0_recalc_rate(hw, parent_rate);
            break;
#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918AV130) \
            || defined(CONFIG_MACH_KM01A)
        case PLL2_FACTOR_CSI1_SCLK:
            recalc_rate = ak_factor_pllx_csi1_recalc_rate(hw, parent_rate);
            break;
#endif
        case PLL2_FACTOR_MAC_OPCLK:
            regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
            factor = (regval >> CLOCK_MAC_DIV_NUM_CFG_SHIFT)
                & CLOCK_MAC_DIV_NUM_CFG_MASK;
            recalc_rate = parent_rate / (factor + 1);
            break;

        case PLL2_FACTOR_SPI0_CLK:
            regval = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
            factor = (regval >> CLOCK_SPI0_HSCLK_DIV_NUM_CFG_SHIFT)
                & CLOCK_SPI0_HSCLK_DIV_NUM_CFG_MASK;
            recalc_rate = parent_rate / (2 * (factor + 1));
            break;
        case PLL2_FACTOR_ENC:
            recalc_rate = ak_factor_pllx_enc_recalc_rate(hw, parent_rate);
            break;
#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918AV130) \
            || defined(CONFIG_MACH_KM01A)
        case PLL2_FACTOR_NPU:
            recalc_rate = ak_factor_pllx_npu_recalc_rate(hw, parent_rate);
            break;
#endif
        case PLL2_FACTOR_ISP:
            recalc_rate = ak_factor_pllx_isp_recalc_rate(hw, parent_rate);
            break;

        case PLL2_FACTOR_DPHY_CFGCLK:
            regval = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
            factor = (regval >> CLOCK_DPHY_CFG_CLK_DIV_NUM_CFG_SHIFT)
                & CLOCK_DPHY_CFG_CLK_DIV_NUM_CFG_MASK;
            recalc_rate = parent_rate / (factor + 1);

            break;
        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            recalc_rate = 0;
            break;
    }
    local_irq_restore(flags);

    pr_debug("%s %s(%d) recalc_rate %lu\n", __func__, hw->init->name,
            factor_clk->id, recalc_rate);

    return recalc_rate;
}


/*
 * ak_factor_pll2_clk_disable
 *
 *
 */
static void ak_factor_pll2_clk_disable(struct clk_hw* hw)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 regval;
    void __iomem* reg = factor_clk->reg;
    unsigned long flags;

    pr_debug("%s %s(%d)\n", __func__, hw->init->name, factor_clk->id);

    local_irq_save(flags);

    switch (factor_clk->id) {
        case PLL2_FACTOR_CSI0_SCLK:
            regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
            regval &= ~(
                CLOCK_CSI0_SCLK_EN_CFG_MASK << CLOCK_CSI0_SCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;
#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918AV130) \
            || defined(CONFIG_MACH_KM01A)
        case PLL2_FACTOR_CSI1_SCLK:
            regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
            regval &= ~(
                CLOCK_CSI1_SCLK_EN_CFG_MASK << CLOCK_CSI1_SCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;
#endif
        case PLL2_FACTOR_MAC_OPCLK:
            regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
            regval &= ~(
                CLOCK_MAC_OPCLK_EN_CFG_MASK << CLOCK_MAC_OPCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;
        case PLL2_FACTOR_SPI0_CLK:
            regval = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
            regval &= ~(
                CLOCK_SPI0_HSCLK_EN_CFG_MASK << CLOCK_SPI0_HSCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;

        case PLL2_FACTOR_ENC:
            ak_factor_pllx_enc_disable(hw);
            break;
#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918AV130) \
            || defined(CONFIG_MACH_KM01A)
        case PLL2_FACTOR_NPU:
            ak_factor_pllx_npu_disable(hw);
            break;
#endif
        case PLL2_FACTOR_ISP:
            ak_factor_pllx_isp_disable(hw);
            break;

        case PLL2_FACTOR_DPHY_CFGCLK:
            regval = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
            regval &= ~(
                CLOCK_DPHY_CFG_CLK_EN_CFG_MASK << CLOCK_DPHY_CFG_CLK_EN_SHIFT);
            __raw_writel(regval, reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;

        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            break;
    }

    local_irq_restore(flags);
    return;
}

/*
 * @BRIEF        ak_factor_pll2_clk_enable
 * @PARAM[in]    hw
 * @RETURN       int
 * @RETVAL       ret
 */
static int ak_factor_pll2_clk_enable(struct clk_hw* hw)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 regval;
    void __iomem* reg = factor_clk->reg;
    int ret = 0;
    unsigned long flags;

    pr_debug("%s %s(%d)\n", __func__, hw->init->name, factor_clk->id);

    local_irq_save(flags);

    switch (factor_clk->id) {
        case PLL2_FACTOR_CSI0_SCLK:
            regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
            regval |= (CLOCK_CSI0_SCLK_EN_CFG_MASK
                    << CLOCK_CSI0_SCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;
#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918AV130) \
            || defined(CONFIG_MACH_KM01A)
        case PLL2_FACTOR_CSI1_SCLK:
            regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
            regval |= (CLOCK_CSI1_SCLK_EN_CFG_MASK
                    << CLOCK_CSI1_SCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;
#endif
        case PLL2_FACTOR_MAC_OPCLK:
            regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
            regval |= (CLOCK_MAC_OPCLK_EN_CFG_MASK
                    << CLOCK_MAC_OPCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;
        case PLL2_FACTOR_SPI0_CLK:
            regval = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
            regval |= (CLOCK_SPI0_HSCLK_EN_CFG_MASK
                    << CLOCK_SPI0_HSCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;

        case PLL2_FACTOR_ENC:
            ret = ak_factor_pllx_enc_enable(hw);
            break;
#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918AV130) \
            || defined(CONFIG_MACH_KM01A)
        case PLL2_FACTOR_NPU:
            ret = ak_factor_pllx_npu_enable(hw);
            break;
#endif
        case PLL2_FACTOR_ISP:
            ret = ak_factor_pllx_isp_enable(hw);
            break;

        case PLL2_FACTOR_DPHY_CFGCLK:
            regval = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
            regval |= (CLOCK_DPHY_CFG_CLK_EN_CFG_MASK
                    << CLOCK_DPHY_CFG_CLK_EN_SHIFT);
            __raw_writel(regval, reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;
        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            break;
    }

    local_irq_restore(flags);
    return ret;
}

/*
 * @BRIEF        ak_factor_pll3_clk_set_rate
 * @PARAM[in]    hw
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       int
 * @RETVAL       ret
 */
static int ak_factor_pll3_clk_set_rate(
        struct clk_hw* hw, unsigned long rate, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 factor;
    void __iomem* reg = factor_clk->reg;
    unsigned long flags;
    int ret = 0;
    u32 regval;

    parent_rate = ak_get_pll3_clk(reg);
    pr_debug("enter %s, parent_rate = %lu\n", __func__, parent_rate);

    if (rate == 0) {
        ak_factor_pll3_clk_disable(hw);
        return ret;
    }

    local_irq_save(flags);

    if (rate && (parent_rate > rate))
        factor = parent_rate / rate - 1;
    else
        factor = 0;

    switch (factor_clk->id) {
        case PLL3_FACTOR_SDADC_HSCLK:
            if (rate == 0) {
                regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
                regval &= ~(CLOCK_SDADC_HS_CLK_EN_CFG_MASK
                        << CLOCK_SDADC_HS_CLK_EN_CFG_SHIFT);
                __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_HS_CTRL);

                regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            } else {
                adchs_set_div(factor_clk, factor);
            }
            break;
        case PLL3_FACTOR_SDDAC_HSCLK:
            if (rate == 0) {
                regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
                regval &= ~(CLOCK_SDDAC_HS_CLK_EN_CFG_MASK
                        << CLOCK_SDDAC_HS_CLK_EN_CFG_SHIFT);
                __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_HS_CTRL);

                regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            } else {
                dachs_set_div(factor_clk, factor);
            }
            break;
        case PLL3_FACTOR_SDADC_CLK:
            if (rate == 0) {
                regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
                regval &= ~(CLOCK_SDADC_CLK_EN_CFG_MASK
                        << CLOCK_SDADC_CLK_EN_CFG_SHIFT);
                __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_HS_CTRL);

                regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            } else {
                sdadc_set_div_osr(factor_clk, rate, parent_rate);
            }
            break;
        case PLL3_FACTOR_SDDAC_CLK:
            if (rate == 0) {
                regval = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
                regval &= ~(CLOCK_SDDAC_CLK_EN_CFG_MASK
                        << CLOCK_SDDAC_CLK_EN_CFG_SHIFT);
                __raw_writel(regval, reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);

                regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            } else {
                sddac_set_div_osr(factor_clk, rate, parent_rate);
            }
            break;
#if defined(CONFIG_MACH_KM01A)
        case PLL3_FACTOR_PDM_HSCLK:
            if (rate == 0) {
                regval = __raw_readl(reg + CLOCK_PDM_CTRL);
                regval &= ~(CLOCK_PDM_HSCLK_EN_CFG_MASK
                        << CLOCK_PDM_HSCLK_EN_CFG_SHIFT);
                __raw_writel(regval, reg + CLOCK_PDM_CTRL);

                regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            } else {
                pdm_set_hsclk(factor_clk, rate, parent_rate);
            }
            break;
        case PLL3_FACTOR_PDM_MSCLK:
            if (rate == 0) {
                regval = __raw_readl(reg + CLOCK_PDM_CTRL);
                regval &= ~(CLOCK_PDM_MSCLK_EN_CFG_MASK
                        << CLOCK_PDM_MSCLK_EN_CFG_SHIFT);
                __raw_writel(regval, reg + CLOCK_PDM_CTRL);

                regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            } else {
                pdm_set_div_ratio(factor_clk, rate, parent_rate);
            }
            break;
#endif
        case PLL3_FACTOR_I2S0_MCLK:
            if (rate == 0) {
                regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
                regval &= ~(CLOCK_SDADC_CLK_EN_CFG_MASK
                        << CLOCK_SDADC_CLK_EN_CFG_SHIFT);
                __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_HS_CTRL);

                regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            } else {
                i2s0_mclk_sdadc_set_div(factor_clk, factor);
            }
            break;
        case PLL3_FACTOR_I2S0_B_LR_CLK:
            if (rate == 0) {
                regval = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
                regval &= ~(CLOCK_SDDAC_CLK_EN_CFG_MASK
                        << CLOCK_SDDAC_CLK_EN_CFG_SHIFT);
                __raw_writel(regval, reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);

                regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            } else {
                i2s0_b_lr_sdadc_set_div(factor_clk, factor);
            }
            break;

        case PLL3_FACTOR_ISP:
            ret = ak_factor_pllx_isp_set_rate(PLL3, hw, rate, parent_rate);
            break;

        case PLL3_FACTOR_CSI0_SCLK:
            ret = ak_factor_pllx_csi0_set_rate(PLL3, hw, rate, parent_rate);
            break;
#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918AV130) \
            || defined(CONFIG_MACH_KM01A)
        case PLL3_FACTOR_NPU:
            ret = ak_factor_pllx_npu_set_rate(PLL3, hw, rate, parent_rate);
            break;
#endif
        case PLL3_FACTOR_ENC:
            ret = ak_factor_pllx_enc_set_rate(PLL3, hw, rate, parent_rate);
            break;
        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            break;
    }
    local_irq_restore(flags);

    pr_debug("%s set_rate %s(%d) rate %lu parent %lu\n", __func__,
            hw->init->name, factor_clk->id, rate, parent_rate);

    return ret;
}
/*end of func*/

/*
 * @BRIEF        ak_factor_pll3_clk_recalc_rate
 * @PARAM[in]    hw
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       unsigned long
 * @RETVAL       recalc_rate
 */
static unsigned long ak_factor_pll3_clk_recalc_rate(
        struct clk_hw* hw, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    void __iomem* reg = factor_clk->reg;
    u32 regval, factor = 0;
    unsigned long recalc_rate;
    unsigned long flags;

    parent_rate = ak_get_pll3_clk(reg);

    pr_debug("%s %s(%d) parent %lu\n", __func__,
            hw->init->name, factor_clk->id,
            parent_rate);

    local_irq_save(flags);

    switch (factor_clk->id) {
        case PLL3_FACTOR_SDADC_HSCLK:
            regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
            if ((regval >> CLOCK_SDADC_HS_CLK_EN_CFG_SHIFT)
                    & CLOCK_SDADC_HS_CLK_EN_CFG_MASK) {
                regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
                factor = (regval >> CLOCK_SDADC_HS_CLK_DIV_NUM_CFG_SHIFT)
                    & CLOCK_SDADC_HS_CLK_DIV_NUM_CFG_MASK;
                recalc_rate = parent_rate / (factor + 1);
            } else {
                recalc_rate = 0;
            }
            break;
        case PLL3_FACTOR_SDDAC_HSCLK:
            regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
            if ((regval >> CLOCK_SDDAC_HS_CLK_EN_CFG_SHIFT)
                    & CLOCK_SDDAC_HS_CLK_EN_CFG_MASK) {
                regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
                factor = (regval >> CLOCK_SDDAC_HS_CLK_DIV_NUM_CFG_SHIFT)
                    & CLOCK_SDDAC_HS_CLK_DIV_NUM_CFG_MASK;
                recalc_rate = parent_rate / (factor + 1);
            } else {
                recalc_rate = 0;
            }
            break;
        case PLL3_FACTOR_SDADC_CLK:
            recalc_rate = sdadc_get_clk(factor_clk, parent_rate);
            break;
        case PLL3_FACTOR_SDDAC_CLK:
            recalc_rate = sddac_get_clk(factor_clk, parent_rate);
            break;
#if defined(CONFIG_MACH_KM01A)
        case PLL3_FACTOR_PDM_HSCLK:
            regval = __raw_readl(reg + CLOCK_PDM_CTRL);
            factor = (regval >> CLOCK_PDM_HSCLK_DIV_NUM_CFG_SHIFT)
                & CLOCK_PDM_HSCLK_DIV_NUM_CFG_MASK;
            recalc_rate = parent_rate/(factor + 1);
            break;
        case PLL3_FACTOR_PDM_MSCLK:
            recalc_rate = pdm_get_clk(factor_clk, parent_rate);
            break;
#endif
        case PLL3_FACTOR_I2S0_MCLK:
            regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
            if ((regval >> CLOCK_SDADC_CLK_EN_CFG_SHIFT)
                    & CLOCK_SDADC_CLK_EN_CFG_MASK) {
                regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
                factor = (regval >> CLOCK_SDADC_CLK_DIV_NUM_CFG_SHIFT)
                    & CLOCK_SDADC_CLK_DIV_NUM_CFG_MASK;
                recalc_rate = parent_rate / (factor + 1);
            } else {
                recalc_rate = 0;
            }
            break;
        case PLL3_FACTOR_I2S0_B_LR_CLK:
            regval = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
            if ((regval >> CLOCK_SDDAC_CLK_EN_CFG_SHIFT)
                    & CLOCK_SDDAC_CLK_EN_CFG_MASK) {
                regval = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
                factor = (regval >> CLOCK_SDDAC_CLK_DIV_NUM_CFG_SHIFT)
                    & CLOCK_SDDAC_CLK_DIV_NUM_CFG_MASK;
                recalc_rate = parent_rate / (factor + 1);
            } else {
                recalc_rate = 0;
            }
            break;

        case PLL3_FACTOR_ISP:
            recalc_rate = ak_factor_pllx_isp_recalc_rate(hw, parent_rate);
            break;

        case PLL3_FACTOR_CSI0_SCLK:
            recalc_rate = ak_factor_pllx_csi0_recalc_rate(hw, parent_rate);
            break;
#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918AV130) \
            || defined(CONFIG_MACH_KM01A)
        case PLL3_FACTOR_NPU:
            recalc_rate = ak_factor_pllx_npu_recalc_rate(hw, parent_rate);
            break;
#endif
        case PLL3_FACTOR_ENC:
            recalc_rate = ak_factor_pllx_enc_recalc_rate(hw, parent_rate);
            break;

        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            break;
    }
    local_irq_restore(flags);
    return recalc_rate;
}
/*end of func*/

/*
 * ak_factor_pll3_clk_disable
 *
 *
 */
static void ak_factor_pll3_clk_disable(struct clk_hw* hw)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 regval;
    void __iomem* reg = factor_clk->reg;
    unsigned long flags;

    pr_debug("%s %s(%d)\n", __func__, hw->init->name, factor_clk->id);

    local_irq_save(flags);
    switch (factor_clk->id) {
        case PLL3_FACTOR_SDADC_HSCLK:
            regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
            regval &= ~(CLOCK_SDADC_HS_CLK_EN_CFG_MASK
                    << CLOCK_SDADC_HS_CLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_HS_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;
        case PLL3_FACTOR_SDDAC_HSCLK:
            regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
            regval &= ~(CLOCK_SDDAC_HS_CLK_EN_CFG_MASK
                    << CLOCK_SDDAC_HS_CLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_HS_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;
        case PLL3_FACTOR_SDADC_CLK:
            regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
            regval &= ~(
                CLOCK_SDADC_CLK_EN_CFG_MASK << CLOCK_SDADC_CLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_HS_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;
        case PLL3_FACTOR_SDDAC_CLK:
            regval = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
            regval &= ~(
                CLOCK_SDDAC_CLK_EN_CFG_MASK << CLOCK_SDDAC_CLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;
#if defined(CONFIG_MACH_KM01A)
        case PLL3_FACTOR_PDM_HSCLK:
            regval = __raw_readl(reg + CLOCK_PDM_CTRL);
            regval &= ~(CLOCK_PDM_HSCLK_EN_CFG_MASK
                    << CLOCK_PDM_HSCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_PDM_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;
        case PLL3_FACTOR_PDM_MSCLK:
            regval = __raw_readl(reg + CLOCK_PDM_CTRL);
            regval &= ~(CLOCK_PDM_MSCLK_EN_CFG_MASK
                    << CLOCK_PDM_MSCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_PDM_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;
#endif
        case PLL3_FACTOR_I2S0_MCLK:
            regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
            regval &= ~(
                CLOCK_SDADC_CLK_EN_CFG_MASK << CLOCK_SDADC_CLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_HS_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;
        case PLL3_FACTOR_I2S0_B_LR_CLK:
            regval = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
            regval &= ~(
                CLOCK_SDDAC_CLK_EN_CFG_MASK << CLOCK_SDDAC_CLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
            regval = __raw_readl(reg + CLOCK_DAC_FADEOUT_CTRL);
            regval &= ~((CLOCK_DAC_FILTER_EN_CFG_MASK)
                    << CLOCK_DAC_FILTER_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_DAC_FADEOUT_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;

        case PLL3_FACTOR_ISP:
            ak_factor_pllx_isp_disable(hw);
            break;
#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918AV130) \
            || defined(CONFIG_MACH_KM01A)
        case PLL3_FACTOR_NPU:
            ak_factor_pllx_npu_disable(hw);
            break;
#endif
        case PLL3_FACTOR_ENC:
            ak_factor_pllx_enc_disable(hw);
            break;

        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            break;
    }
    local_irq_restore(flags);
    return;
}

/*
 * @BRIEF        ak_factor_pll3_clk_enable
 * @PARAM[in]    hw
 * @RETURN       int
 * @RETVAL       ret
 */
static int ak_factor_pll3_clk_enable(struct clk_hw* hw)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 regval;
    void __iomem* reg = factor_clk->reg;
    int ret;
    unsigned long flags;

    pr_debug("%s %s(%d)\n", __func__, hw->init->name, factor_clk->id);

    local_irq_save(flags);
    switch (factor_clk->id) {
        case PLL3_FACTOR_SDADC_HSCLK:
            regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
            regval |= (CLOCK_SDADC_HS_CLK_EN_CFG_MASK
                    << CLOCK_SDADC_HS_CLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_HS_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;
        case PLL3_FACTOR_SDDAC_HSCLK:
            regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
            regval |= (CLOCK_SDDAC_HS_CLK_EN_CFG_MASK
                    << CLOCK_SDDAC_HS_CLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_HS_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;
        case PLL3_FACTOR_SDADC_CLK:
            regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
            regval |= (CLOCK_SDADC_CLK_EN_CFG_MASK
                    << CLOCK_SDADC_CLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_HS_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;
        case PLL3_FACTOR_SDDAC_CLK:
            regval = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
            regval |= (CLOCK_SDDAC_CLK_EN_CFG_MASK
                    << CLOCK_SDDAC_CLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;
        case PLL3_FACTOR_I2S0_MCLK:
            regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
            regval |= (CLOCK_SDADC_CLK_EN_CFG_MASK
                    << CLOCK_SDADC_CLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_HS_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;
        case PLL3_FACTOR_I2S0_B_LR_CLK:
            regval = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
            regval |= (CLOCK_SDDAC_CLK_EN_CFG_MASK
                    << CLOCK_SDDAC_CLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
            regval = __raw_readl(reg + CLOCK_DAC_FADEOUT_CTRL);
            regval |= ((CLOCK_DAC_FILTER_EN_CFG_MASK)
                    << CLOCK_DAC_FILTER_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_DAC_FADEOUT_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;

        case PLL3_FACTOR_ISP:
            ret = ak_factor_pllx_isp_enable(hw);
            break;
#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918AV130) \
            || defined(CONFIG_MACH_KM01A)
        case PLL3_FACTOR_NPU:
            ret = ak_factor_pllx_npu_enable(hw);
            break;
#endif
        case PLL3_FACTOR_ENC:
            ret = ak_factor_pllx_enc_enable(hw);
            break;
        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            break;
    }
    local_irq_restore(flags);
    return ret;
}

/*
 * @BRIEF        ak_factor_osc24m_clk_round_rate
 * @PARAM[in]    hw
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       long
 * @RETVAL       round_rate
 */
static long ak_factor_osc24m_clk_round_rate(
        struct clk_hw* hw, unsigned long rate, unsigned long* parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    unsigned long round_rate = rate;

    pr_debug("factor round_rate %s(%d) rate %lu parent %luMHz\n",
            hw->init->name, factor_clk->id, round_rate, (*parent_rate) / MHz);

    return round_rate;
}

/*
 * ak_factor_osc24m_clk_set_rate
 *
 */
static int ak_factor_osc24m_clk_set_rate(
        struct clk_hw* hw, unsigned long rate, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 factor, regval;
    void __iomem* reg = factor_clk->reg;
    unsigned long flags;
    pr_debug("enter %s, parent_rate = %lu\n", __func__, parent_rate);

    if (rate == 0) {
        ak_factor_osc24m_clk_disable(hw);
        return 0;
    }

    local_irq_save(flags);

    switch (factor_clk->id) {
        case OSC24_FACTOR_SAR_ADC_CLK:
            factor = parent_rate / rate - 1;

            regval = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
            regval &= ~((0x1) << CLOCK_SAR_ADC_CLK_EN_SHIFT);
            __raw_writel(regval, (reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL));

            regval = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
            regval &= ~(CLOCK_SAR_ADC_CLK_DIV_NUM_CFG_MASK
                    << CLOCK_SAR_ADC_CLK_DIV_NUM_CFG_SHIFT);
            regval |= ((CLOCK_SAR_ADC_CLK_DIV_NUM_CFG_MASK & factor)
                    << CLOCK_SAR_ADC_CLK_DIV_NUM_CFG_SHIFT);
            __raw_writel(regval, (reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL));

            regval = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
            regval |= ((0x1) << CLOCK_SAR_ADC_CLK_EN_SHIFT);
            __raw_writel(regval, (reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL));

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;
        case OSC24_FACTOR_CSI0_SCLK:

#if !defined(CONFIG_MACH_KM01A) && !defined(CONFIG_MACH_AK3918AV130)
            regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
            regval &= ~((0x1) << CLOCK_CSI0_SCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);

            regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
            regval &= ~(CLOCK_CSI0_SCLK_MUX_NUM_CFG_MASK
                    << CLOCK_CSI0_SCLK_MUX_NUM_CFG_SHIFT);
            regval |= ((CLOCK_CSI_SRC_SEL_OSC24M
                        & CLOCK_CSI0_SCLK_MUX_NUM_CFG_MASK)
                    << CLOCK_CSI0_SCLK_MUX_NUM_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);

            regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
            regval |= ((0x1) << CLOCK_CSI0_SCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
#else
            {
                int timeout = CLOCK_FREQ_ADJ_WAITING_MAX_NUM;
                regval = __raw_readl(reg + CLOCK_CSI_CTRL);
                regval &= ~(CSI_CLK_OPEN_MASK << CSI_CLK_OPEN_SHIFT(0));
                __raw_writel(regval, reg + CLOCK_CSI_CTRL);

                regval = __raw_readl(reg + CLOCK_CSI_CTRL);
                regval &= ~(CSI_CLK_SEL_MASK << CSI_CLK_SEL_SHIFT(0));
                regval |= (0 << CSI_CLK_SEL_SHIFT(0));
                __raw_writel(regval, reg + CLOCK_CSI_CTRL);

                do {
                    timeout--;
                    regval = ((__raw_readl(reg + CLOCK_CSI_CTRL))
                            >> CSI_CLK_READY_SHIFT(0))
                        & CSI_CLK_READY_MASK;
                } while ((regval == 0) && (timeout > 0));

                if (timeout <= 0) {
                    pr_err("Waiting CSI%d ready timeout!\n", 0);
                }

                regval = __raw_readl(reg + CLOCK_CSI_CTRL);
                regval |= (CSI_CLK_OPEN_MASK << CSI_CLK_OPEN_SHIFT(0));
                __raw_writel(regval, reg + CLOCK_CSI_CTRL);

                regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            }
#endif
            break;

#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918AV130) \
            || defined(CONFIG_MACH_KM01A)
        case OSC24_FACTOR_CSI1_SCLK:
#if !defined(CONFIG_MACH_KM01A) && !defined(CONFIG_MACH_AK3918AV130)
            regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
            regval &= ~((0x1) << CLOCK_CSI1_SCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);

            regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
            regval &= ~(CLOCK_CSI1_SCLK_MUX_NUM_CFG_MASK
                    << CLOCK_CSI1_SCLK_MUX_NUM_CFG_SHIFT);
            regval |= ((CLOCK_CSI_SRC_SEL_OSC24M
                        & CLOCK_CSI1_SCLK_MUX_NUM_CFG_MASK)
                    << CLOCK_CSI1_SCLK_MUX_NUM_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);

            regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
            regval |= ((0x1) << CLOCK_CSI1_SCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
#else
            {
                int timeout = CLOCK_FREQ_ADJ_WAITING_MAX_NUM;
                regval = __raw_readl(reg + CLOCK_CSI_CTRL);
                regval &= ~(CSI_CLK_OPEN_MASK << CSI_CLK_OPEN_SHIFT(1));
                __raw_writel(regval, reg + CLOCK_CSI_CTRL);

                regval = __raw_readl(reg + CLOCK_CSI_CTRL);
                regval &= ~(CSI_CLK_SEL_MASK << CSI_CLK_SEL_SHIFT(1));
                regval |= (0 << CSI_CLK_SEL_SHIFT(1));
                __raw_writel(regval, reg + CLOCK_CSI_CTRL);

                do {
                    timeout--;
                    regval = ((__raw_readl(reg + CLOCK_CSI_CTRL))
                            >> CSI_CLK_READY_SHIFT(1))
                        & CSI_CLK_READY_MASK;
                } while ((regval == 0) && (timeout > 0));

                if (timeout <= 0) {
                    pr_err("Waiting CSI%d ready timeout!\n", 0);
                }

                regval = __raw_readl(reg + CLOCK_CSI_CTRL);
                regval |= (CSI_CLK_OPEN_MASK << CSI_CLK_OPEN_SHIFT(1));
                __raw_writel(regval, reg + CLOCK_CSI_CTRL);

                regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            }
#endif
            break;
#endif
        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            break;
    }
    local_irq_restore(flags);

    pr_debug("%s %s(%d) rate %lu parent %lu\n", __func__, hw->init->name,
            factor_clk->id, rate, parent_rate);

    return 0;
}

/*
 * ak_factor_osc24m_clk_recalc_rate
 *
 */
static unsigned long ak_factor_osc24m_clk_recalc_rate(
        struct clk_hw* hw, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    void __iomem* reg = factor_clk->reg;
    u32 regval, factor = 0;
    unsigned long recalc_rate;
    unsigned long flags;

    pr_debug("%s %s(%d) parent %lu\n",
            __func__, hw->init->name, factor_clk->id,
            parent_rate);

    local_irq_save(flags);
    switch (factor_clk->id) {
        case OSC24_FACTOR_SAR_ADC_CLK:
            regval = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
            factor = (regval >> CLOCK_SAR_ADC_CLK_DIV_NUM_CFG_SHIFT)
                & CLOCK_SAR_ADC_CLK_DIV_NUM_CFG_MASK;
            recalc_rate = parent_rate / (factor + 1);
            break;

        case OSC24_FACTOR_CSI0_SCLK:
            recalc_rate = ak_factor_pllx_csi0_recalc_rate(hw, parent_rate);
            break;

#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918AV130) \
            || defined(CONFIG_MACH_KM01A)
        case OSC24_FACTOR_CSI1_SCLK:
            recalc_rate = ak_factor_pllx_csi1_recalc_rate(hw, parent_rate);
            break;
#endif

        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            break;
    }

    local_irq_restore(flags);
    return recalc_rate;
}

/*
 * ak_factor_osc24m_clk_disable
 *
 */
static void ak_factor_osc24m_clk_disable(struct clk_hw* hw)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    void __iomem* reg = factor_clk->reg;
    u32 regval;
    unsigned long flags;

    local_irq_save(flags);
    switch (factor_clk->id) {
        case OSC24_FACTOR_SAR_ADC_CLK:
            regval = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
            regval &= ~((0x1) << CLOCK_SAR_ADC_CLK_EN_SHIFT);
            __raw_writel(regval, (reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL));

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;

        case OSC24_FACTOR_CSI0_SCLK:
#if !defined(CONFIG_MACH_KM01A) && !defined(CONFIG_MACH_AK3918AV130)
            regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
            regval &= ~(
                CLOCK_CSI0_SCLK_EN_CFG_MASK << CLOCK_CSI0_SCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
#else
            regval = __raw_readl(reg + CLOCK_CSI_CTRL);
            regval &= ~(CSI_CLK_OPEN_MASK << CSI_CLK_OPEN_SHIFT(0));
            __raw_writel(regval, reg + CLOCK_CSI_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
#endif
            break;

#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918AV130) \
            || defined(CONFIG_MACH_KM01A)
        case OSC24_FACTOR_CSI1_SCLK:
#if !defined(CONFIG_MACH_KM01A) && !defined(CONFIG_MACH_AK3918AV130)
            regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
            regval &= ~(
                CLOCK_CSI1_SCLK_EN_CFG_MASK << CLOCK_CSI1_SCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
#else
            regval = __raw_readl(reg + CLOCK_CSI_CTRL);
            regval &= ~(CSI_CLK_OPEN_MASK << CSI_CLK_OPEN_SHIFT(1));
            __raw_writel(regval, reg + CLOCK_CSI_CTRL);

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
#endif
            break;
#endif

        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            break;
    }
    local_irq_restore(flags);
}

/*
 * ak_factor_osc24m_clk_enable
 *
 */
static int ak_factor_osc24m_clk_enable(struct clk_hw* hw)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 regval;
    void __iomem* reg = factor_clk->reg;
    int ret = 0;
    unsigned long flags;

    pr_debug("%s %s(%d)\n", __func__, hw->init->name, factor_clk->id);

    local_irq_save(flags);
    switch (factor_clk->id) {
        case OSC24_FACTOR_SAR_ADC_CLK:
            regval = __raw_readl(reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL);
            regval |= ((0x1) << CLOCK_SAR_ADC_CLK_EN_SHIFT);
            __raw_writel(regval, (reg + CLOCK_SDADDA_SPI0HS_SARADDA_CTRL));

            regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            break;

        case OSC24_FACTOR_CSI0_SCLK:
#if !defined(CONFIG_MACH_KM01A) && !defined(CONFIG_MACH_AK3918AV130)
            regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
            regval |= (CLOCK_CSI0_SCLK_EN_CFG_MASK
                    << CLOCK_CSI0_SCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
#else
            {
                regval = __raw_readl(reg + CLOCK_CSI_CTRL);
                regval |= (CSI_CLK_OPEN_MASK << CSI_CLK_OPEN_SHIFT(0));
                __raw_writel(regval, reg + CLOCK_CSI_CTRL);

                regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            }
#endif
            break;

#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918AV130) \
            || defined(CONFIG_MACH_KM01A)
        case OSC24_FACTOR_CSI1_SCLK:
#if !defined(CONFIG_MACH_KM01A) && !defined(CONFIG_MACH_AK3918AV130)
            regval = __raw_readl(reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
            regval |= (CLOCK_CSI1_SCLK_EN_CFG_MASK
                    << CLOCK_CSI1_SCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_CSI_SCLK_MAC_OPCLK_CTRL);
#else
            {
                regval = __raw_readl(reg + CLOCK_CSI_CTRL);
                regval |= (CSI_CLK_OPEN_MASK << CSI_CLK_OPEN_SHIFT(1));
                __raw_writel(regval, reg + CLOCK_CSI_CTRL);

                regval = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
            }
#endif
            break;
#endif

        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            break;
    }

    local_irq_restore(flags);
    return ret;
}

/*
 * ak_factor_pll1_clk_round_rate
 *
 */
static long ak_factor_pll1_clk_round_rate(
        struct clk_hw* hw, unsigned long rate, unsigned long* parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    unsigned long round_rate = rate;

    pr_debug("factor round_rate %s(%d) rate %lu parent %luMHz\n",
            hw->init->name, factor_clk->id, round_rate, (*parent_rate) / MHz);

    return round_rate;
}

/*
 * ak_factor_pll1_clk_set_rate
 *
 */
static int ak_factor_pll1_clk_set_rate(
        struct clk_hw* hw, unsigned long rate, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    unsigned long flags;
    int ret = 0;
    pr_debug("enter %s, parent_rate = %lu id:%d\n",
            __func__, parent_rate,factor_clk->id);

    if (rate == 0) {
        ak_factor_pll1_clk_disable(hw);
        return ret;
    }

    local_irq_save(flags);

    switch (factor_clk->id) {
        case PLL1_FACTOR_ISP:
            ret = ak_factor_pllx_isp_set_rate(PLL1, hw, rate, parent_rate);
            break;
#ifdef CONFIG_MACH_KM01A
        case PLL1_FACTOR_PP:
            ret = ak_factor_pllx_pp_set_rate(PLL1, hw, rate, parent_rate);
            break;
#endif
        case PLL1_FACTOR_ENC:
            ret = ak_factor_pllx_enc_set_rate(PLL1, hw, rate, parent_rate);
            break;

        case PLL1_FACTOR_CSI0_SCLK:
            ret = ak_factor_pllx_csi0_set_rate(PLL1, hw, rate, parent_rate);
            break;
#if defined(CONFIG_MACH_KM01A)
        case PLL1_FACTOR_NPU:
            ret = ak_factor_pllx_npu_set_rate(PLL1, hw, rate, parent_rate);
            break;
#endif
        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            break;
    }
    local_irq_restore(flags);

    pr_debug("factor set_rate %s(%d) rate %lu parent %lu\n", hw->init->name,
            factor_clk->id, rate, parent_rate);

    return ret;
}

/*
 * ak_factor_pll1_clk_recalc_rate
 *
 */
static unsigned long ak_factor_pll1_clk_recalc_rate(
        struct clk_hw* hw, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    unsigned long recalc_rate;
    unsigned long flags;

    pr_debug("%s %s(%d) parent %lu\n",
            __func__, hw->init->name, factor_clk->id,
            parent_rate);

    local_irq_save(flags);
    switch (factor_clk->id) {
        case PLL1_FACTOR_ISP:
            recalc_rate = ak_factor_pllx_isp_recalc_rate(hw, parent_rate);
            break;
#ifdef CONFIG_MACH_KM01A
        case PLL1_FACTOR_PP:
            recalc_rate = ak_factor_pllx_pp_recalc_rate(hw, parent_rate);
            break;
#endif
        case PLL1_FACTOR_ENC:
            recalc_rate = ak_factor_pllx_enc_recalc_rate(hw, parent_rate);
            break;

        case PLL1_FACTOR_CSI0_SCLK:
            recalc_rate = ak_factor_pllx_csi0_recalc_rate(hw, parent_rate);
            break;

#if defined(CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130)
        case PLL1_FACTOR_CSI1_SCLK:
            recalc_rate = ak_factor_pllx_csi1_recalc_rate(hw, parent_rate);
            break;
#endif
#if defined(CONFIG_MACH_KM01A)
        case PLL1_FACTOR_NPU:
            recalc_rate = ak_factor_pllx_npu_recalc_rate(hw, parent_rate);
            break;
#endif
        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            break;
    }

    local_irq_restore(flags);
    return recalc_rate;
}

/*
 * ak_factor_pll1_clk_disable
 */
static void ak_factor_pll1_clk_disable(struct clk_hw* hw)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    unsigned long flags;

    pr_debug("%s %s(%d)\n", __func__, hw->init->name, factor_clk->id);

    local_irq_save(flags);
    switch (factor_clk->id) {
        case PLL1_FACTOR_ISP:
            ak_factor_pllx_isp_disable(hw);
            break;
#ifdef CONFIG_MACH_KM01A
        case PLL1_FACTOR_PP:
            ak_factor_pllx_pp_disable(hw);
            break;
#endif
        case PLL1_FACTOR_ENC:
            ak_factor_pllx_enc_disable(hw);
            break;
#if defined(CONFIG_MACH_KM01A)
        case PLL1_FACTOR_NPU:
            ak_factor_pllx_npu_disable(hw);
            break;
#endif
        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            break;
    }
    local_irq_restore(flags);
}

/*
 * ak_factor_pll1_clk_enable
 *
 */
static int ak_factor_pll1_clk_enable(struct clk_hw* hw)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    int ret = 0;
    unsigned long flags;

    pr_debug("%s %s(%d)\n", __func__, hw->init->name, factor_clk->id);

    local_irq_save(flags);
    switch (factor_clk->id) {
        case PLL1_FACTOR_ENC:
            ret = ak_factor_pllx_enc_enable(hw);
            break;
        case PLL1_FACTOR_ISP:
            ret = ak_factor_pllx_isp_enable(hw);
            break;
#if defined(CONFIG_MACH_KM01A)
        case PLL1_FACTOR_PP:
            ret = ak_factor_pllx_pp_enable(hw);
            break;
        case PLL1_FACTOR_NPU:
            ret = ak_factor_pllx_npu_enable(hw);
            break;
#endif
        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            break;
    }
    local_irq_restore(flags);

    return ret;
}

const struct clk_ops ak_factor_pll1_clk_ops = {
#ifndef CONFIG_MACH_AK3918AV100_FPGA
    .round_rate = ak_factor_pll1_clk_round_rate,
    .set_rate = ak_factor_pll1_clk_set_rate,
    .recalc_rate = ak_factor_pll1_clk_recalc_rate,
    .disable = ak_factor_pll1_clk_disable,
    .enable = ak_factor_pll1_clk_enable,
#endif
};

const struct clk_ops ak_factor_pll2_clk_ops = {
#ifndef CONFIG_MACH_AK3918AV100_FPGA
    .round_rate = ak_factor_pll2_clk_round_rate,
    .set_rate = ak_factor_pll2_clk_set_rate,
    .recalc_rate = ak_factor_pll2_clk_recalc_rate,
    .disable = ak_factor_pll2_clk_disable,
    .enable = ak_factor_pll2_clk_enable,
#endif
};

const struct clk_ops ak_factor_pll3_clk_ops = {
#ifndef CONFIG_MACH_AK3918AV100_FPGA
    .round_rate = ak_factor_pll3_clk_round_rate,
    .set_rate = ak_factor_pll3_clk_set_rate,
    .recalc_rate = ak_factor_pll3_clk_recalc_rate,
    .disable = ak_factor_pll3_clk_disable,
    .enable = ak_factor_pll3_clk_enable,
#endif
};

const struct clk_ops ak_factor_osc24m_clk_ops = {
#ifndef CONFIG_MACH_AK3918AV100_FPGA
    .disable = ak_factor_osc24m_clk_disable,
    .enable = ak_factor_osc24m_clk_enable,
    .round_rate = ak_factor_osc24m_clk_round_rate,
    .recalc_rate = ak_factor_osc24m_clk_recalc_rate,
    .set_rate = ak_factor_osc24m_clk_set_rate,
#endif
};

/*
 * ak_register_factor_clk
 *
 */
static struct clk* __init ak_register_factor_clk(const char* name,
        const char* parent_name, void __iomem* res_reg, int id,
        const struct clk_ops* ops)
{
    struct ak_factor_clk* factor_clk;
    struct clk* clk = NULL;
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

/*
 * of_ak_factor_clk_init
 *
 */
static void __init of_ak_factor_clk_init(struct device_node* np)
{
    struct clk_onecell_data* clk_data;
    const char* clk_name = np->name;
    const char* parent_name = of_clk_get_parent_name(np, 0);
    void __iomem* res_reg = AK_VA_SYSCTRL;
    int clock_id, index, number;
    const struct clk_ops* ak_factor_clk_ops;

    clk_data = kmalloc(sizeof(struct clk_onecell_data), GFP_KERNEL);
    if (!clk_data)
        return;

    number = of_property_count_u32_elems(np, "clock-id");
    clk_data->clks = kcalloc(number, sizeof(struct clk*), GFP_KERNEL);
    if (!clk_data->clks)
        goto err_free_data;

    if (strncmp(parent_name, "pll1", sizeof("pll1")) == 0)
        ak_factor_clk_ops = &ak_factor_pll1_clk_ops;
    else if (strncmp(parent_name, "pll2", sizeof("pll2")) == 0)
        ak_factor_clk_ops = &ak_factor_pll2_clk_ops;
    else if (strncmp(parent_name, "pll3", sizeof("pll3")) == 0)
        ak_factor_clk_ops = &ak_factor_pll3_clk_ops;
    else if (strncmp(parent_name, "osc24M", sizeof("osc24M")) == 0)
        ak_factor_clk_ops = &ak_factor_osc24m_clk_ops;
    else {
        pr_err("ak_factor_clk_ops not exit!\n");
        WARN_ON(true);
    }

    for (index = 0; index < number; index++) {
        of_property_read_string_index(
                np, "clock-output-names", index, &clk_name);
        of_property_read_u32_index(np, "clock-id", index, &clock_id);

        if (clock_id >= number) {
            pr_err("register factor clk failed: id %d >= number %d\n",
                    clock_id , number);
            WARN_ON(true);
            continue;
        }
        clk_data->clks[clock_id] = ak_register_factor_clk(
                clk_name, parent_name, res_reg, clock_id, ak_factor_clk_ops);
        if (IS_ERR(clk_data->clks[clock_id])) {
            pr_err("register factor clk failed: clk_name:%s, err = %s\n",
                    clk_name, (char*)PTR_ERR(clk_data->clks[clock_id]));
            WARN_ON(true);
            continue;
        } else
            pr_debug("clk[%p] %s, parent:%s, id:%d , idx:%d\n",
                    clk_data->clks[clock_id], clk_name, parent_name, clock_id,
                    index);
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

#if defined(CONFIG_MACH_AK3918AV100)
CLK_OF_DECLARE(ak3918av100_factor_pll1_clk,"anyka,ak3918av100-pll1-factor-clk",
        of_ak_factor_clk_init);
CLK_OF_DECLARE(ak3918av100_factor_pll2_clk,"anyka,ak3918av100-pll2-factor-clk",
        of_ak_factor_clk_init);
CLK_OF_DECLARE(ak3918av100_factor_pll3_clk,"anyka,ak3918av100-pll3-factor-clk",
        of_ak_factor_clk_init);
CLK_OF_DECLARE(ak3918av100_factor_osc24m_clk,
        "anyka,ak3918av100-osc24-factor-clk", of_ak_factor_clk_init);
#elif defined(CONFIG_MACH_AK3918EV300L)
CLK_OF_DECLARE(ak3918ev300l_factor_pll1_clk,
        "anyka,ak3918ev300l-pll1-factor-clk", of_ak_factor_clk_init);
CLK_OF_DECLARE(ak3918ev300l_factor_pll2_clk,
        "anyka,ak3918ev300l-pll2-factor-clk", of_ak_factor_clk_init);
CLK_OF_DECLARE(ak3918ev300l_factor_pll3_clk,
        "anyka,ak3918ev300l-pll3-factor-clk", of_ak_factor_clk_init);
CLK_OF_DECLARE(ak3918ev300l_factor_osc24m_clk,
        "anyka,ak3918ev300l-osc24-factor-clk", of_ak_factor_clk_init);
#elif defined(CONFIG_MACH_AK3918AV130)
CLK_OF_DECLARE(ak3918av130_factor_pll1_clk,
        "anyka,ak3918av130-pll1-factor-clk",
        of_ak_factor_clk_init);
CLK_OF_DECLARE(ak3918av130_factor_pll2_clk,
        "anyka,ak3918av130-pll2-factor-clk",
        of_ak_factor_clk_init);
CLK_OF_DECLARE(ak3918av130_factor_pll3_clk,
        "anyka,ak3918av130-pll3-factor-clk",
        of_ak_factor_clk_init);
CLK_OF_DECLARE(ak3918av130_factor_osc24m_clk,
        "anyka,ak3918av130-osc24-factor-clk", of_ak_factor_clk_init);
#elif defined(CONFIG_MACH_KM01A)
CLK_OF_DECLARE(km01a_factor_pll1_clk, "anyka,km01a-pll1-factor-clk",
        of_ak_factor_clk_init);
CLK_OF_DECLARE(km01a_factor_pll2_clk, "anyka,km01a-pll2-factor-clk",
        of_ak_factor_clk_init);
CLK_OF_DECLARE(km01a_factor_pll3_clk, "anyka,km01a-pll3-factor-clk",
        of_ak_factor_clk_init);
CLK_OF_DECLARE(km01a_factor_osc24m_clk,
        "anyka,km01a-osc24-factor-clk", of_ak_factor_clk_init);
#endif
