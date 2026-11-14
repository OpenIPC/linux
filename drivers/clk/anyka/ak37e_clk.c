/*
 * AK37E clk driver
 *
 * Copyright (C) 2018 Anyka(Guangzhou) Microelectronics Technology Co., Ltd.
 *
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
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

#include <dt-bindings/clock/ak37e-clock.h>

#include <mach/map.h>

#define AK_CLK_DUMP_INFO

/*
 * todo_List:
 * 1. 3.2.3.2.1 USB clock. How is the USB PHY(30MHz).
 */

#define CLOCK_CPU_PLL_CTRL          (0x04)
#define CLOCK_ASIC_PLL_CTRL         (0x08)
#define CLOCK_SAR_ADC_CTRL          (0x0C)
#define CLOCK_SD_ADC_DAC_HS_CTRL    (0x10)
#define CLOCK_IMAGE_CAPTURE_CTRL    (0x14)
#define CLOCK_MAC_OPCLK_CTRL        (0x18)
#define CLOCK_GATE_CTRL             (0x1C)
#define CLOCK_SOFT_RESET            (0x20)
#define CLOCK_GATE_SOFT_RESET_CTRL  (0xFC)
#define CLOCK_USB_I2S_CTRL          (0x58)
#define CLOCK_DAC_FADEOUT_CTRL      (0x70)
#define CLOCK_SPI_HS_CTRL           (0x100)
#define CLOCK_AUDIO_PLL_CTRL        (0x1B8)
#define CLOCK_SD_ADC_DAC_AUDIO_CTRL (0x1BC)

#define CLOCK_LCD_PCLK_CTRL (0xBC)

#define ANALOG_CTRL_REG3 (0xA4)

#define PDM_CTRL_REG (0xE0000)
/*
 * CLOCK_CPU_PLL_CTRL
 */
#define CPU_PLL_DPHYCLK_1X2X_CFG_MASK  (0x1)
#define CPU_PLL_DPHYCLK_1X2X_CFG_SHIFT (30)
#define ASIC_PLL_FREQ_ADJ_CFG_MASK     (0x1)
#define ASIC_PLL_FREQ_ADJ_CFG_SHIFT    (28)
#define CPU_PLL_ST_MODE_SHIFT          (27)
#define CPU_PLL_CPU_5X_SEL_CFG_MASK    (0x1)
#define CPU_PLL_CPU_5X_SEL_CFG_SHIFT   (26)
#define CPU_PLL_CLK_5X_EN_CFG_SHIFT    (25)
#define CPU_PLL_EVEN_DIV_VLD_CFG_SHIFT (17)
#define CPU_PLL_EVEN_DIV_NUM_CFG_MASK  (0x3)
#define CPU_PLL_EVEN_DIV_NUM_CFG_SHIFT (15)
#define CPU_PLL_FREQ_ADJ_CFG_SHIFT     (14)
#define CPU_PLL_OD_CFG_MASK            (0x3)
#define CPU_PLL_OD_CFG_SHIFT           (12)
#define CPU_PLL_N_CFG_MASK             (0xF)
#define CPU_PLL_N_CFG_SHIFT            (8)
#define CPU_PLL_M_CFG_MASK             (0xFF)
#define CPU_PLL_M_CFG_SHIFT            (0)

/*
 * CLOCK_ASIC_PLL_CTRL
 */
#define CLOCK_ASIC_GCLK_SEL_MASK          (0x1)
#define CLOCK_ASIC_GCLK_SEL_SHIFT         (24)
#define CLOCK_ASIC_VCLK_DIV_VLD_CFG_SHIFT (23)
#define CLOCK_ASIC_VCLK_DIV_NUM_CFG_MASK  (0x3)
#define CLOCK_ASIC_VCLK_DIV_NUM_CFG_SHIFT (17)
#define CLOCK_ASIC_PLL_OD_CFG_MASK        (0x3)
#define CLOCK_ASIC_PLL_OD_CFG_SHIFT       (12)
#define CLOCK_ASIC_PLL_N_CFG_MASK         (0xF)
#define CLOCK_ASIC_PLL_N_CFG_SHIFT        (8)
#define CLOCK_ASIC_PLL_M_CFG_MASK         (0xFF)
#define CLOCK_ASIC_PLL_M_CFG_SHIFT        (0)

/*
 * CLOCK_SAR_ADC_CTRL
 */
#define CLOCK_SAR_ADC_CLK_EN_SHIFT          (3)
#define CLOCK_SAR_ADC_CLK_DIV_NUM_CFG_MASK  (0x7)
#define CLOCK_SAR_ADC_CLK_DIV_NUM_CFG_SHIFT (0)

/*
 * CLOCK_SD_ADC_DAC_HS_CTRL
 */
#define CLOCK_SDADC_HS_DIV_VLD_CFG_MASK      (0x01)
#define CLOCK_SDADC_HS_DIV_VLD_CFG_SHIFT     (29)
#define CLOCK_SDADC_HS_CLK_EN_CFG_MASK       (0x01)
#define CLOCK_SDADC_HS_CLK_EN_CFG_SHIFT      (28)
#define CLOCK_SDADC_HS_CLK_DIV_NUM_CFG_MASK  (0xFF)
#define CLOCK_SDADC_HS_CLK_DIV_NUM_CFG_SHIFT (20)
#define CLOCK_SDDAC_HS_DIV_VLD_CFG_MASK      (0x01)
#define CLOCK_SDDAC_HS_DIV_VLD_CFG_SHIFT     (19)
#define CLOCK_SDDAC_HS_CLK_EN_CFG_MASK       (0x01)
#define CLOCK_SDDAC_HS_CLK_EN_CFG_SHIFT      (18)
#define CLOCK_SDDAC_HS_CLK_DIV_NUM_CFG_MASK  (0xFF)
#define CLOCK_SDDAC_HS_CLK_DIV_NUM_CFG_SHIFT (10)

/*
 * CLOCK_IMAGE_CAPTURE_CTRL
 */
#define CLOCK_CSI_DVP_SENSOR_SEL_CFG_SHIFT   (31)
#define CLOCK_CSI_DUAL_SENSOR_MODE_CFG_SHIFT (29)
#define CLOCK_CSI1_SCLK_SEL_CFG_SHIFT        (27)
#define CLOCK_CSI_PRST_CFG_SHIFT             (25)
#define CLOCK_CSI0_SCLK_SEL_CFG_SHIFT        (24)
#define CLOCK_MAC0_SPEED_CFG_SHIFT           (23)
#define CLOCK_MAC0_INTF_SEL_CFG_SHIFT        (22)
#define CLOCK_MAC1_SPEED_CFG_SHIFT           (21)
#define CLOCK_MAC1_INTF_SEL_CFG_SHIFT        (20)
#define CLOCK_IMAGE_CAPTURE_CTRL_CFG_MASK    (0x1)

/*
 * CLOCK_MAC_OPCLK_CTRL
 */
#define CLOCK_CSI1_PCLK_POL_SHIFT          (31)
#define CLOCK_CSI0_PCLK_POL_SHIFT          (30)
#define CLOCK_MAC1_OPCLK_DIV_VLD_CFG_MASK  (0x01)
#define CLOCK_MAC1_OPCLK_DIV_VLD_CFG_SHIFT (29)
#define CLOCK_MAC1_OPCLK_EN_CFG_MASK       (0x01)
#define CLOCK_MAC1_OPCLK_EN_CFG_SHIFT      (28)
#define CLOCK_MAC1_DIV_NUM_CFG_MASK        (0x3F)
#define CLOCK_MAC1_DIV_NUM_CFG_SHIFT       (20)
#define CLOCK_CSI0_SCLK_DIV_VLD_CFG_MASK   (0x01)
#define CLOCK_CSI0_SCLK_DIV_VLD_CFG_SHIFT  (19)
#define CLOCK_CSI0_SCLK_EN_CFG_SHIFT       (18)
#define CLOCK_CSI0_SCLK_EN_CFG_MASK        (0x01)
#define CLOCK_CSI0_SCLK_DIV_NUM_CFG_MASK   (0x3F)
#define CLOCK_CSI0_SCLK_DIV_NUM_CFG_SHIFT  (10)
#define CLOCK_MAC0_OPCLK_DIV_VLD_CFG_MASK  (0x01)
#define CLOCK_MAC0_OPCLK_DIV_VLD_CFG_SHIFT (9)
#define CLOCK_MAC0_OPCLK_EN_CFG_MASK       (0x01)
#define CLOCK_MAC0_OPCLK_EN_CFG_SHIFT      (8)
#define CLOCK_MAC0_DIV_NUM_CFG_MASK        (0x3F)
#define CLOCK_MAC0_DIV_NUM_CFG_SHIFT       (0)

/*
 * CLOCK_GATE_CTRL
 * NOTE: ctrlbit will be configured from dts files.
 */
#define CLOCK_DSI_CONTROLLER_SHIFT     (31)
#define CLOCK_DPHY_CLK_GATE_CFG_SHIFT  (30)
#define CLOCK_DCLK_GATE_CFG_SHIFT      (24)
#define CLOCK_GUI_CFG_SHIFT            (23)
#define CLOCK_LCD_CONTROLLER_CFG_SHIFT (22)
#define CLOCK_VIDEO_DECODER_CFG_SHIFT  (21)
#define CLOCK_JPEG_CODEC_CFG_SHIFT     (20)
#define CLOCK_IMAGE_CAPTURE_CFG_SHIFT  (19)

/*
 * CLOCK_SOFT_RESET
 */
#define CLOCK_MIPI_DIS_RST_CFG_SHIFT      (31)
#define CLOCK_SARADC_RST_CFG_SHIFT        (30)
#define CLOCK_SDADC_HSRST_CFG_SHIFT       (29)
#define CLOCK_SDDAC_HSRST_CFG_SHIFT       (28)
#define CLOCK_SDADC_RST_CFG_SHIFT         (27)
#define CLOCK_SDDAC_RST_CFG_SHIFT         (26)
#define CLOCK_DPHYCLK_RST_CFG_SHIFT       (25)
#define CLOCK_DCLK_RST_CFG_SHIFT          (24)
#define VCLK_GUI_RST_CFG_SHIFT            (23)
#define VCLK_LCD_CONTROLLER_RST_CFG_SHIFT (22)
#define VCLK_VIDEO_DECODER_RST_CFG_SHIFT  (21)
#define VCLK_JPEG_CODEC_RST_CFG_SHIFT     (20)
#define VCLK_IMAGE_CAPTURE_RST_CFG_SHIFT  (19)
#define VCLK_RST_CFG_SHIFT                (18)
#define GCLK_TWI1_RST_CFG_SHIFT           (17)
#define GCLK_MAC1_RST_CFG_SHIFT           (16)
#define GCLK_USB_RST_CFG_SHIFT            (15)
#define GCLK_PDM_CONTROLLER_RST_CFG_SHIFT (14)
#define GCLK_MAC0_BUFFER_RST_CFG_SHIFT    (13)
#define GCLK_GPIO_RST_CFG_SHIFT           (12)
#define GCLK_L2BUF1_RST_CFG_SHIFT         (11)
#define GCLK_TWI0_RST_CFG_SHIFT           (10)
#define GCLK_L2BUF0_RST_CFG_SHIFT         (9)
#define GCLK_UART1_RST_CFG_SHIFT          (8)
#define GCLK_UART0_RST_CFG_SHIFT          (7)
#define GCLK_SPI1_RST_CFG_SHIFT           (6)
#define GCLK_SPI0_RST_CFG_SHIFT           (5)
#define GCLK_SDDAC_RST_CFG_SHIFT          (4)
#define GCLK_SDADC_RST_CFG_SHIFT          (3)
#define GCLK_MMC1_RST_CFG_SHIFT           (2)
#define GCLK_MMC0_RST_CFG_SHIFT           (1)
#define GCLK_RST_CFG_SHIFT                (0)

/*
 * CLOCK_GATE_SOFT_RESET_CTRL
 */
#define CLOCK_PDM_I2SM_CLK_RST_CFG (31)
#define CLOCK_PDM_HS_CLK_RST_CFG   (30)
#define GCLK_TWI3_RST_CFG_SHIFT    (21)
#define GCLK_TWI2_RST_CFG_SHIFT    (20)
#define GCLK_MMC2_RST_CFG_SHIFT    (19)
#define GCLK_SPI2_RST_CFG_SHIFT    (18)
#define GCLK_UART3_RST_CFG_SHIFT   (17)
#define GCLK_UART2_RST_CFG_SHIFT   (16)

/*
 * CLOCK_USB_I2S_CTRL
 */
#define CLOCK_I2S_MCLK_SEL_SDADC_CLK_CFG  (0x0)
#define CLOCK_I2S_MCLK_SEL_SDDAC_CLK_CFG  (0x1)
#define CLOCK_I2S_MCLK_SEL_MASK           (0x1)
#define CLOCK_I2S_MCLK_SEL_SHIFT          (30)
#define CLOCK_I2SM_ADC_MODE_ENABLED_SHIFT (29)
#define CLOCK_I2SM_MODE_ENABLED_SHIFT     (28)
#define CLOCK_I2SSR_MODE_ENABLED_SHIFT    (27)
#define CLOCK_I2SST_MODE_ENABLED_SHIFT    (26)
#define CLOCK_I2SSI_MODE_ENABLED_SHIFT    (25)
#define USB_LINE_STATE_WP_MASK            (0x3)
#define USB_LINE_STATE_WP_SHIFT           (15)
#define USB_DP_PU_EN_SHIFT                (14)
#define USB_ID_CFG_MASK                   (0x3)
#define USB_ID_CFG_SHIFT                  (12)
#define USB_PHY_CFG_MASK                  (0x3F)
#define USB_PHY_CFG_SHIFT                 (6)
#define USB_PHY_PON_RST_SHIFT             (3)
#define USB_SUS_CFG_SHIFT                 (2)
#define CLOCK_USB_PHY_PLL_EN_SHIFT        (1)
#define CLOCK_USB_PHY_RST_SHIFT           (0)

/*
 * CLOCK_DAC_FADEOUT_CTRL
 */
#define CLOCK_DAC_48K_MODE_EN_CFG_MASK  (0x01UL)
#define CLOCK_DAC_48K_MODE_EN_CFG_SHIFT (31)
#define CLOCK_DAC_FILTER_EN_CFG_MASK    (0x1)
#define CLOCK_DAC_FILTER_EN_CFG_SHIFT   (3)
#define CLOCK_DAC_OSR_CFG_MASK          (0x07)
#define CLOCK_DAC_OSR_CFG_SHIFT         (0)

/*
 * CLOCK_SPI_HS_CTRL
 */
#define CLOCK_SPI0_HSCLK_RST_CFG_SHIFT    (31)
#define CLOCK_LCD_PCLK_RST_CFG_SHIFT      (30)
#define CLOCK_CSI1_SCLK_DIV_VLD_CFG_MASK  (0x01)
#define CLOCK_CSI1_SCLK_DIV_VLD_CFG_SHIFT (19)
#define CLOCK_CSI1_SCLK_EN_CFG_SHIFT      (18)
#define CLOCK_CSI1_SCLK_EN_CFG_MASK       (0x01)
#define CLOCK_CSI1_SCLK_DIV_NUM_CFG_MASK  (0x3F)
#define CLOCK_CSI1_SCLK_DIV_NUM_CFG_SHIFT (10)
#define CLOCK_SPI_HSDIV_VLD_CFG_MASK      (0x01)
#define CLOCK_SPI_HSDIV_VLD_CFG_SHIFT     (9)
#define CLOCK_SPI_HSCLK_EN_CFG_MASK       (0x01)
#define CLOCK_SPI_HSCLK_EN_CFG_SHIFT      (8)
#define CLOCK_SPI_HSCLK_DIV_NUM_CFG_MASK  (0xFF)
#define CLOCK_SPI_HSCLK_DIV_NUM_CFG_SHIFT (0)

/*
 * CLOCK_AUDIO_PLL_CTRL
 */
#define CLOCK_PDM_I2SM_CLK_DIV_VLD_CFG_SHIFT (29)
#define CLOCK_PDM_I2SM_CLK_EN_CFG_MASK       (0x01)
#define CLOCK_PDM_I2SM_CLK_EN_CFG_SHIFT      (28)
#define CLOCK_PDM_I2SM_CLK_DIV_NUM_CFG_MASK  (0xFF)
#define CLOCK_PDM_I2SM_CLK_DIV_NUM_CFG_SHIFT (20)
#define CLOCK_AUDIO_PLL_STATUS_MASK          (0x1)
#define CLOCK_AUDIO_PLL_STATUS_SHIFT         (17)
#define CLOCK_AUDIO_PLL_REF_CLK_EN_SHIFT     (16)
#define CLOCK_AUDIO_PLL_DISABLE_MASK         (0x1)
#define CLOCK_AUDIO_PLL_DISABLE_SHIFT        (15)
#define CLOCK_AUDIO_PLL_FREQ_ADJ_CFG_SHIFT   (14)
#define CLOCK_AUDIO_PLL_FB_DIV_N_MASK        (0x1FF)
#define CLOCK_AUDIO_PLL_FB_DIV_N_SHIFT       (5)
#define CLOCK_AUDIO_PLL_REF_DIV_M_MASK       (0x1F)
#define CLOCK_AUDIO_PLL_REF_DIV_M_SHIFT      (0)

/*
 * CLOCK_SD_ADC_DAC_AUDIO_CTRL
 */
#define CLOCK_PDM_HS_DIV_VLD_CFG_SHIFT     (29)
#define CLOCK_PDM_HS_CLK_EN_CFG_MASK       (0x01)
#define CLOCK_PDM_HS_CLK_EN_CFG_SHIFT      (28)
#define CLOCK_PDM_HS_CLK_DIV_NUM_CFG_MASK  (0xFF)
#define CLOCK_PDM_HS_CLK_DIV_NUM_CFG_SHIFT (20)
#define CLOCK_SDDAC_DIV_VLD_CFG_SHIFT      (19)
#define CLOCK_SDDAC_CLK_EN_CFG_MASK        (0x01)
#define CLOCK_SDDAC_CLK_EN_CFG_SHIFT       (18)
#define CLOCK_SDDAC_CLK_DIV_NUM_CFG_MASK   (0xFF)
#define CLOCK_SDDAC_CLK_DIV_NUM_CFG_SHIFT  (10)
#define CLOCK_SDADC_DIV_VLD_CFG_MASK       (0x01)
#define CLOCK_SDADC_DIV_VLD_CFG_SHIFT      (9)
#define CLOCK_SDADC_CLK_EN_CFG_MASK        (0x01)
#define CLOCK_SDADC_CLK_EN_CFG_SHIFT       (8)
#define CLOCK_SDADC_CLK_DIV_NUM_CFG_MASK   (0xFF)
#define CLOCK_SDADC_CLK_DIV_NUM_CFG_SHIFT  (0)

/*
 * CLOCK_LCD_PCLK_CTRL
 */
#define CLOCK_LCD_PCLK_EN_CFG_MASK       (0x01)
#define CLOCK_LCD_PCLK_EN_CFG_SHIFT      (8)
#define CLOCK_LCD_PCLK_DIV_NUM_CFG_MASK  (0x7F)
#define CLOCK_LCD_PCLK_DIV_NUM_CFG_SHIFT (1)
#define CLOCK_LCD_PCLK_DIV_VLD_CFG_SHIFT (0)

/*
 * PDM_CTRL_REG
 */
#define PDM_CTRL_DECIMATION_RATIO_CFG_MASK  (0x07)
#define PDM_CTRL_DECIMATION_RATIO_CFG_SHIFT (13)

#define AK_CLOCK_GATE_GROUP (32)

#define MHz (1000000UL)

#define CLOCK_FREQ_ADJ_WAITING_MAX_NUM (100000)

#define TYPE_SDADC (1)
#define TYPE_SDDAC (0)

#define MODULE_RESET_RELEASE (0)
#define MODULE_RESET_HOLD    (1)

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

struct ak_gate_clk {
    int id;
    struct clk_hw hw;
    void __iomem* reg;
    int ctrlbit;
};

#define to_clk_ak_fixed(_hw)  container_of(_hw, struct ak_fixed_clk, hw)
#define to_clk_ak_factor(_hw) container_of(_hw, struct ak_factor_clk, hw)
#define to_clk_ak_gate(_hw)   container_of(_hw, struct ak_gate_clk, hw)

static unsigned long audio_pll[]
    = { 204800000, 282240000, 168960000, 204800000, 282240000, 184320000,
          204800000, 282240000, 184320000, 293538461, 245760000 };
static unsigned char audio_pll_m[]
    = { 14, 24, 24, 14, 24, 24, 14, 24, 24, 25, 24 }; /*width:5bits*/
static unsigned short audio_pll_n[]
    = { 63, 146, 87, 63, 146, 95, 63, 146, 95, 158, 127 }; /*width:9bits*/

static unsigned short sddac_DIV[]
    = { 100, 100, 55, 50, 50, 30, 25, 25, 15, 13, 10 };
static unsigned short sdadc_DIV[]
    = { 100, 100, 55, 50, 50, 30, 25, 25, 15, 13, 10 };
static unsigned short sddac_OSR[]
    = { 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256 };
static unsigned short sdadc_OSR[]
    = { 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256 };
static unsigned int adc_dac_sample_rate[] = { 8000, 11025, 12000, 16000, 22050,
    24000, 32000, 44100, 48000, 88202, 96000 };

#ifdef AK_CLK_DUMP_INFO
void clk_dump_registers(void)
{
    void __iomem* res = AK_VA_SYSCTRL;

    pr_info("ANYKA CLK Register Dumping Begin:\n");
    pr_info("  CPU_PLL(04) = 0x%08X, ASIC_PLL(08)    = 0x%0X\n",
        __raw_readl(res + CLOCK_CPU_PLL_CTRL),
        __raw_readl(res + CLOCK_ASIC_PLL_CTRL));
    pr_info("  SAR_ADC(0C) = 0x%08X, ADC_DAC_HS(10)  = 0x%0X\n",
        __raw_readl(res + CLOCK_SAR_ADC_CTRL),
        __raw_readl(res + CLOCK_SD_ADC_DAC_HS_CTRL));
    pr_info("  IMG_CAP(14) = 0x%08X, OPCLK(18) = 0x%0X\n",
        __raw_readl(res + CLOCK_IMAGE_CAPTURE_CTRL),
        __raw_readl(res + CLOCK_MAC_OPCLK_CTRL));
    pr_info("  GATE_CTRL(1C) = 0x%08X, SOFT_RST(20) = 0x%0X\n",
        __raw_readl(res + CLOCK_GATE_CTRL),
        __raw_readl(res + CLOCK_SOFT_RESET));
    pr_info("  GATE_RST(FC) = 0x%08X, USB_I2S(58) = 0x%0X\n",
        __raw_readl(res + CLOCK_GATE_SOFT_RESET_CTRL),
        __raw_readl(res + CLOCK_USB_I2S_CTRL));
    pr_info("  DAC_FADEOUT(70) = 0x%08X, SPI_HS(100) = 0x%0X\n",
        __raw_readl(res + CLOCK_DAC_FADEOUT_CTRL),
        __raw_readl(res + CLOCK_SPI_HS_CTRL));
    pr_info("  AUDIO_PLL(1B8) = 0x%08X, ADC_DAC_AUDIO(1BC) = 0x%0X\n",
        __raw_readl(res + CLOCK_AUDIO_PLL_CTRL),
        __raw_readl(res + CLOCK_SD_ADC_DAC_AUDIO_CTRL));
    pr_info("ANYKA CLK Register Dumping End.\n");
}
#endif

static int ak_clock_divider_adjust_complete(
    void __iomem* res_reg, u32 shift, u32 mask)
{
    int timeout = CLOCK_FREQ_ADJ_WAITING_MAX_NUM;
    u32 regval;

    do {
        timeout--;
        regval = ((__raw_readl(res_reg)) >> shift) & mask;
    } while (regval && (timeout > 0));

    if (timeout <= 0) {
        pr_err("Waiting 0x%p:(0x%x) shift %d mask 0x%x timeout\n", res_reg,
            __raw_readl(res_reg), shift, mask);
    }

    return 0;
}

/*
 * asic_pll_clk = 24 * M/(N*2^OD)
 */
static unsigned long ak_get_asic_pll_clk(void __iomem* reg)
{
    u32 pll_m, pll_n, pll_od;
    u32 asic_pll_clk;
    u32 regval;

    regval = __raw_readl(reg + CLOCK_ASIC_PLL_CTRL);
    pll_od
        = (regval >> CLOCK_ASIC_PLL_OD_CFG_SHIFT) & CLOCK_ASIC_PLL_OD_CFG_MASK;
    pll_n = (regval >> CLOCK_ASIC_PLL_N_CFG_SHIFT) & CLOCK_ASIC_PLL_N_CFG_MASK;
    pll_m = (regval >> CLOCK_ASIC_PLL_M_CFG_SHIFT) & CLOCK_ASIC_PLL_M_CFG_MASK;

    asic_pll_clk = 24 * pll_m / (pll_n * (1 << pll_od));

    return asic_pll_clk * MHz;
}

/*
 * @BRIEF        ak_get_vclk
 * @PARAM[in]    reg
 * @RETURN       unsigned long
 * @RETVAL       vclk * MHz
 */
static unsigned long ak_get_vclk(void __iomem* reg)
{
    u32 regval;
    u32 vclk;

    regval = __raw_readl(reg + CLOCK_ASIC_PLL_CTRL);
    regval = ((regval >> CLOCK_ASIC_VCLK_DIV_NUM_CFG_SHIFT)
        & CLOCK_ASIC_VCLK_DIV_NUM_CFG_MASK);

    vclk = (ak_get_asic_pll_clk(reg) / MHz) / (2 * (regval + 1));

    return vclk * MHz;
}

/*
 * @BRIEF        ak_fixed_clk_round_rate
 * @PARAM[in]    hw
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       long
 * @RETVAL       rate
 */
static long ak_fixed_clk_round_rate(
    struct clk_hw* hw, unsigned long rate, unsigned long* parent_rate)
{
    struct ak_fixed_clk* fixed_clk = to_clk_ak_fixed(hw);

    /*
     * NOTE: we have 5 fixed clock:
     * CPU_PLL_JCLK/CPU_PLL_HCLK/CPU_PLL_DPHYCLK: kernel could not to mofity
     * them ASIC_PLL_CLK: configured from the dts. AUDIO_PLL_CLK: configured due
     * different audio sample rate. So here round rate only return the target
     * rate, DONOT change the parent_rate(They parent always fixed like osc24M)
     */

    pr_debug("fixed round_rate %s(%d) rate %lu parent %luMHz\n", hw->init->name,
        fixed_clk->id, rate, (*parent_rate) / MHz);

    return rate;
}

/*
 * @BRIEF        ak_get_audio_pll_freq
 * @PARAM[in]    reg
 * @RETURN       unsigned long
 * @RETVAL       audio_clk
 */
static unsigned long ak_get_audio_pll_freq(void __iomem* reg)
{
    u32 regval, audio_m, audio_n;
    unsigned long audio_clk;

    regval = __raw_readl(reg + CLOCK_AUDIO_PLL_CTRL);

    /*
     * CLOCK_AUDIO_PLL_STATUS: AUDIO_PLL is active or not. 1 means not active
     */
    if (!(regval >> CLOCK_AUDIO_PLL_DISABLE_SHIFT)
        & CLOCK_AUDIO_PLL_DISABLE_MASK) {
        return 0;
    }

    audio_m = (regval >> CLOCK_AUDIO_PLL_REF_DIV_M_SHIFT)
        & CLOCK_AUDIO_PLL_REF_DIV_M_MASK;
    audio_n = (regval >> CLOCK_AUDIO_PLL_FB_DIV_N_SHIFT)
        & CLOCK_AUDIO_PLL_FB_DIV_N_MASK;

    audio_clk = (48000000UL / (audio_m + 1)) * (audio_n + 1);

    pr_debug("Now AUDIO PLL %lu Hz m %d n %d\n", audio_clk, audio_m, audio_n);

    return audio_clk;
}

/*
 * audio_pll_clk = 48 * (audio_pll_fb_div_n + 1) / (audio_pll_ref_div_m + 1)
 * 0.8125 <= 24/(audio_pll_ref_div_m +1 ) <= 13
 * |---1.84 <= audio_pll_ref_div_m <=28 only integer
 * 160MHz <= audio_pll_clk <= 320MHz
 * audio_pll_fb_div_n need audio_pll_freq_adj_cfg to be 1/CLOCK_AUDIO_PLL_CTRL
 * Bit[14]
 */
static int ak_set_audio_pll_freq(void __iomem* reg, unsigned long rate)
{
    u32 regval;
    int i, num = sizeof(audio_pll) / sizeof(audio_pll[0]);

    for (i = 0; i < num; i++) {
        if (rate == audio_pll[i]) {
            break;
        }
    }

    if (i >= num) {
        pr_err("audio pll not support %lu\n", rate);
        return -EINVAL;
    }

    regval = __raw_readl(reg + CLOCK_AUDIO_PLL_CTRL);
    regval |= ((0x01) << CLOCK_AUDIO_PLL_REF_CLK_EN_SHIFT);
    regval &= ~((0x01) << CLOCK_AUDIO_PLL_DISABLE_SHIFT);
    __raw_writel(regval, reg + CLOCK_AUDIO_PLL_CTRL);

    regval = __raw_readl(reg + CLOCK_AUDIO_PLL_CTRL);
    regval &= ~(
        (CLOCK_AUDIO_PLL_REF_DIV_M_MASK) << CLOCK_AUDIO_PLL_REF_DIV_M_SHIFT);
    regval
        &= ~((CLOCK_AUDIO_PLL_FB_DIV_N_MASK) << CLOCK_AUDIO_PLL_FB_DIV_N_SHIFT);
    regval |= (audio_pll_m[i] & CLOCK_AUDIO_PLL_REF_DIV_M_MASK)
        << CLOCK_AUDIO_PLL_REF_DIV_M_SHIFT;
    regval |= (audio_pll_n[i] & CLOCK_AUDIO_PLL_FB_DIV_N_MASK)
        << CLOCK_AUDIO_PLL_FB_DIV_N_SHIFT;
    regval |= ((0x1) << CLOCK_AUDIO_PLL_FREQ_ADJ_CFG_SHIFT);
    __raw_writel(regval, reg + CLOCK_AUDIO_PLL_CTRL);

    ak_clock_divider_adjust_complete(
        reg + CLOCK_AUDIO_PLL_CTRL, CLOCK_AUDIO_PLL_FREQ_ADJ_CFG_SHIFT, 0x01);

    pr_info("audio_pll update @%lu m %d n %d\n", rate, audio_pll_m[i],
        audio_pll_n[i]);

    return 0;
}

/*
 * @BRIEF        ak_get_audio_pll_by_sample_rate
 * @PARAM[in]    sample_rate
 * @PARAM[in]    rate
 * @RETURN       int
 * @RETVAL       0
 */
static int ak_get_audio_pll_by_sample_rate(
    unsigned long sample_rate, unsigned long* rate)
{
    int i, num = sizeof(adc_dac_sample_rate) / sizeof(adc_dac_sample_rate[0]);

    for (i = 0; i < num; i++) {
        if (sample_rate == adc_dac_sample_rate[i]) {
            break;
        }
    }

    if (i >= num) {
        pr_err("audio pll not support %lu\n", sample_rate);
        return -EINVAL;
    }

    (*rate) = audio_pll[i];

    return 0;
}

/*
 * @BRIEF        ak_fixed_clk_set_rate
 * @PARAM[in]    hw
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       int
 * @RETVAL       rate
 */
static int ak_fixed_clk_set_rate(
    struct clk_hw* hw, unsigned long rate, unsigned long parent_rate)
{
    struct ak_fixed_clk* fixed_clk = to_clk_ak_fixed(hw);
    unsigned long now_rate;

    pr_debug("fixed set_rate %s(%d) rate %lu parent %luMHz\n", hw->init->name,
        fixed_clk->id, rate, parent_rate / MHz);

    /*
     * NOTE: we have 5 fixed clock:
     * CPU_PLL_JCLK/CPU_PLL_HCLK/CPU_PLL_DPHYCLK: kernel could not to mofity
     * them ASIC_PLL_CLK: configured from the dts.Donot change in code.
     * AUDIO_PLL_CLK: configured due different audio sample rate.Update it in
     * children's round rate So here round rate only return the target rate,
     * DONOT change the parent_rate(They parent always fixed like osc24M)
     */
    switch (fixed_clk->id) {
        case AUDIO_PLL_CLK:
            now_rate = ak_get_audio_pll_freq(fixed_clk->reg);
            if (now_rate != rate)
                ak_set_audio_pll_freq(fixed_clk->reg, rate);
            break;
        default:
            break;
    }

    return rate;
}

/*
 * cpu_pll_clk = 24 * M/(N*2^OD)
 */
static unsigned long ak_get_cpu_pll_clk(void __iomem* reg)
{
    u32 pll_m, pll_n, pll_od;
    u32 cpu_pll_clk;
    u32 regval;

    regval = __raw_readl(reg + CLOCK_CPU_PLL_CTRL);
    pll_od = (regval >> CPU_PLL_OD_CFG_SHIFT) & CPU_PLL_OD_CFG_MASK;
    pll_n = (regval >> CPU_PLL_N_CFG_SHIFT) & CPU_PLL_N_CFG_MASK;
    pll_m = (regval >> CPU_PLL_M_CFG_SHIFT) & CPU_PLL_M_CFG_MASK;

    cpu_pll_clk = 24 * pll_m / (pll_n * (1 << pll_od));

    return cpu_pll_clk * MHz;
}

/*
 * @BRIEF        ak_get_dphyclk
 * @PARAM[in]    reg
 * @PARAM[in]    cpu_pll_clk
 * @RETURN       unsigned long
 * @RETVAL       pll_clk
 */
static unsigned long ak_get_dphyclk(
    void __iomem* reg, unsigned long cpu_pll_clk)
{
    unsigned long pll_clk;
    u32 cpu_pll_even_div_num_cfg = 0;
    u32 regval = __raw_readl(reg + CLOCK_CPU_PLL_CTRL);

    if ((regval >> CPU_PLL_DPHYCLK_1X2X_CFG_SHIFT)
        & CPU_PLL_DPHYCLK_1X2X_CFG_MASK) {
        // dphyclk_1x_2x_cfg:1 DDR2 work in 2x mode.dphyclk = cpu_pll_clk
        pll_clk = cpu_pll_clk;
    } else {
        // dphyclk_1x_2x_cfg:0 DDR2 work in 1x mode.DPHY clock mode defined by
        // cpu5x_sel_cfg
        if ((regval >> CPU_PLL_CPU_5X_SEL_CFG_SHIFT)
            & CPU_PLL_CPU_5X_SEL_CFG_MASK) {
            // cpu5x_sel_cfg:1 jclk = cpu_pll_clk
            // hclk = dclk = dphyclk = 1/5 cpu_pll_clk
            pll_clk = cpu_pll_clk / 5;
        } else {
            // cpu5x_sel_cfg:0 jclk = cpu_pll_clk
            // hclk = dclk = dphyclk = cpu_pll_even_div_clk
            cpu_pll_even_div_num_cfg
                = (regval >> CPU_PLL_EVEN_DIV_NUM_CFG_SHIFT)
                & CPU_PLL_EVEN_DIV_NUM_CFG_MASK;
            pll_clk = cpu_pll_clk / (2 * (cpu_pll_even_div_num_cfg + 1));
        }
    }

    return pll_clk;
}

/*
 * @BRIEF        ak_get_hclk_dclk
 * @PARAM[in]    reg
 * @PARAM[in]    cpu_pll_clk
 * @RETURN       unsigned long
 * @RETVAL       pll_clk
 */
static unsigned long ak_get_hclk_dclk(
    void __iomem* reg, unsigned long cpu_pll_clk)
{
    unsigned long pll_clk;
    u32 cpu_pll_even_div_num_cfg = 0;
    u32 regval = __raw_readl(reg + CLOCK_CPU_PLL_CTRL);

    if ((regval >> CPU_PLL_DPHYCLK_1X2X_CFG_SHIFT)
        & CPU_PLL_DPHYCLK_1X2X_CFG_MASK) {
        // dphyclk_1x_2x_cfg:1 DDR2 work in 2x mode.hclk = dclk = 1/2
        // cpu_pll_clk
        pll_clk = cpu_pll_clk / 2;
    } else {
        // dphyclk_1x_2x_cfg:0 DDR2 work in 1x mode.DPHY clock mode defined by
        // cpu5x_sel_cfg
        if ((regval >> CPU_PLL_CPU_5X_SEL_CFG_SHIFT)
            & CPU_PLL_CPU_5X_SEL_CFG_MASK) {
            // cpu5x_sel_cfg:1 jclk = cpu_pll_clk
            // hclk = dclk = dphyclk = 1/5 cpu_pll_clk
            pll_clk = cpu_pll_clk / 5;
        } else {
            // cpu5x_sel_cfg:0 jclk = cpu_pll_clk
            // hclk = dclk = dphyclk = cpu_pll_even_div_clk
            cpu_pll_even_div_num_cfg
                = (regval >> CPU_PLL_EVEN_DIV_NUM_CFG_SHIFT)
                & CPU_PLL_EVEN_DIV_NUM_CFG_MASK;
            pll_clk = cpu_pll_clk / (2 * (cpu_pll_even_div_num_cfg + 1));
        }
    }

    return pll_clk;
}

/*
 * @BRIEF        ak_fixed_clk_recalc_rate
 * @PARAM[in]    hw
 * @PARAM[in]    parent_rate
 * @RETURN       unsigned long
 * @RETVAL       fixed_clk->fixed_rate
 */
static unsigned long ak_fixed_clk_recalc_rate(
    struct clk_hw* hw, unsigned long parent_rate)
{
    struct ak_fixed_clk* fixed_clk = to_clk_ak_fixed(hw);
    unsigned long cpu_clk = ak_get_cpu_pll_clk(fixed_clk->reg);

    pr_debug("%s %s(%d) parent %luMHz\n", __func__, hw->init->name,
        fixed_clk->id, parent_rate / MHz);

    switch (fixed_clk->id) {
        case CPU_PLL_JCLK:
            fixed_clk->fixed_rate = cpu_clk;
            pr_info("CPU(JCLK): %lu(Mhz)\n", cpu_clk / MHz);
            break;
        case CPU_PLL_HCLK:
            fixed_clk->fixed_rate = ak_get_hclk_dclk(fixed_clk->reg, cpu_clk);
            pr_info("CPU(HCLK): %lu(Mhz)\n", fixed_clk->fixed_rate / MHz);
            break;
        case CPU_PLL_DPHYCLK:
            fixed_clk->fixed_rate = ak_get_dphyclk(fixed_clk->reg, cpu_clk);
            pr_info("MEMDDR2(DPHY): %lu(Mhz)\n", fixed_clk->fixed_rate / MHz);
            break;
        case ASIC_PLL_CLK:
            fixed_clk->fixed_rate = ak_get_asic_pll_clk(fixed_clk->reg);
            break;
        case AUDIO_PLL_CLK:
            fixed_clk->fixed_rate = ak_get_audio_pll_freq(fixed_clk->reg);
            break;
        default:
            break;
    }

    pr_debug("%s (%d) recalc_rate parent %luMHz rate %luMHz\n", hw->init->name,
        fixed_clk->id, parent_rate / MHz, fixed_clk->fixed_rate / MHz);

    return fixed_clk->fixed_rate;
}

const struct clk_ops ak_fixed_clk_ops = {
    .round_rate = ak_fixed_clk_round_rate,
    .set_rate = ak_fixed_clk_set_rate,
    .recalc_rate = ak_fixed_clk_recalc_rate,
};

static int sdadc_sddac_calc_div_osr(int is_sdadc, unsigned long rate,
    unsigned long* parent_rate, int* div, int* osr);

static long ak_factor_asic_clk_round_rate(
    struct clk_hw* hw, unsigned long rate, unsigned long* parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    unsigned long round_rate = rate;

    /*
     * Here:
     * 1. ASIC_FACTOR_CIS0_SCLK/ASIC_FACTOR_CIS1_SCLKASIC_FACTOR_MAC0_OPCLK
     * ASIC_FACTOR_MAC1_OPCLK/ASIC_FACTOR_SPI0_CLK:
     * there parent is asic pll. and is fixed. DONOT change the parent_rate
     */

    pr_debug("factor round_rate %s(%d) rate %lu parent %luMHz\n",
        hw->init->name, factor_clk->id, round_rate, (*parent_rate) / MHz);

    return round_rate;
}

static long ak_factor_audio_clk_round_rate(
    struct clk_hw* hw, unsigned long rate, unsigned long* parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    unsigned long round_rate = rate;
    unsigned long actual_rate = 0, match_rate = 0;

    /*
     * Here:
     * 1. factor clock related to auduo
     * pll(AUDIO_FACTOR_SDADC_CLK/AUDIO_FACTOR_SDDAC_CLK) need to be modify by
     * different rate. here change the parent_rate.
     * TODO:How about AUDIO_FACTOR_PDM_HSCLK and AUDIO_FACTOR_PDM_I2SM_CLK.
     * !!!Please NOTICE audio only support the sample rate as previous!!!
     */
    switch (factor_clk->id) {
        case AUDIO_FACTOR_SDADC_CLK:
        case AUDIO_FACTOR_SDDAC_CLK:
        case AUDIO_FACTOR_PDM_I2SM_CLK:
            if (ak_get_audio_pll_by_sample_rate(rate, &match_rate)) {
                round_rate = 0;
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
            }
            break;
        default:
            break;
    }

    pr_debug("audio round_rate %s(%d) rate %lu parent %luMHz\n", hw->init->name,
        factor_clk->id, round_rate, (match_rate) / MHz);

    return round_rate;
}

// static int ak_clock_reset_module(void __iomem *reg,
// u32 reset_shift, int ops)
// {
// u32 regval;

// if (ops == MODULE_RESET_HOLD) {
// regval = __raw_readl(reg);
// regval |= (0x1 << reset_shift);
// __raw_writel(regval, reg);
// } else if (ops == MODULE_RESET_RELEASE) {
// regval = __raw_readl(reg);
// regval &= ~(0x1 << reset_shift);
// __raw_writel(regval, reg);
// }

// return 0;
// }

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

    regval = __raw_readl(res_reg);
    regval &= ~(div_num_cfg_mask << div_num_cfg_shift);
    regval |= ((div_num_cfg & div_num_cfg_mask) << div_num_cfg_shift);
    regval |= ((0x01) << div_vld_cfg_shift);
    __raw_writel(regval, res_reg);
    ak_clock_divider_adjust_complete(res_reg, div_vld_cfg_shift, 0x01);

    return 0;
}

/*
 * ak_adjust_clock_int_divider_freq
 * like sdadc_clk:
 * 1. disable the sd-adc module
 * 2. set sdadc_clk_en_cfg to 0 to disable output when sdadc_clk_div_vld_cfg is
 * 0
 * 3. change the sdadc_clk_div_num_cfg and set sdadc_clk_div_vld_cfg to 1.
 *    wait sdadc_clk_div_vld_cfg to 0
 * 4. set sdadc_clk_en_cfg to 1 t enable the output clock.
 * 5. enable the sd-adc module
 */
static int ak_adjust_clock_int_divider_freq(void __iomem* res_reg,
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

    regval = __raw_readl(res_reg);
    regval |= ((0x01) << div_vld_cfg_shift);
    __raw_writel(regval, res_reg);
    ak_clock_divider_adjust_complete(res_reg, div_vld_cfg_shift, 0x01);

    regval = __raw_readl(res_reg);
    regval |= ((0x1) << en_cfg_shift);
    __raw_writel(regval, res_reg);

    return 0;
}

/*
 * @BRIEF        ak_module_reset_by_clock
 * @PARAM[in]    res_reg
 * @PARAM[in]    rst_cfg_shift
 * @RETURN       void
 */
static void ak_module_reset_by_clock(void __iomem* res_reg, u32 rst_cfg_shift)
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
}

/*
 * @BRIEF        sdadc_sddac_calc_div_osr
 * @PARAM[in]    is_sdadc
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @PARAM[in]    div
 * @PARAM[in]    osr
 * @RETURN       int
 * @RETVAL       fail:-22 sucess:0
 */
static int sdadc_sddac_calc_div_osr(int is_sdadc, unsigned long rate,
    unsigned long* parent_rate, int* div, int* osr)
{
    int i;
    int num;

    pr_debug("%s %s rate %lu parent %lu\n", __func__,
        is_sdadc ? "sdadc" : "sddac", rate, *parent_rate);

    num = sizeof(audio_pll) / sizeof(audio_pll[0]);

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

    *parent_rate = audio_pll[i];

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
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       int
 * @RETVAL       fail:-EINVAL sucess:0
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

    // ak_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
    //      CLOCK_SDADC_RST_CFG_SHIFT, MODULE_RESET_HOLD);
    ak_group_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
        ((0x1 << CLOCK_SDADC_RST_CFG_SHIFT)
            | (0x1 << CLOCK_SDADC_HSRST_CFG_SHIFT)),
        MODULE_RESET_HOLD);

    ak_adjust_clock_int_divider_freq(
        (factor_clk->reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL),
        CLOCK_SDADC_DIV_VLD_CFG_SHIFT, CLOCK_SDADC_CLK_EN_CFG_SHIFT, (div - 1),
        CLOCK_SDADC_CLK_DIV_NUM_CFG_SHIFT, CLOCK_SDADC_CLK_DIV_NUM_CFG_MASK);

    // ak_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
    //      CLOCK_SDADC_RST_CFG_SHIFT, MODULE_RESET_RELEASE);
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

    return 0;
}

/*
 * @BRIEF        sddac_set_div_osr
 * @PARAM[in]    factor_clk
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       int
 * @RETVAL       fail:-EINVAL sucess:0
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

    // ak_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
    //  CLOCK_SDDAC_RST_CFG_SHIFT, MODULE_RESET_HOLD);
    ak_group_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
        ((0x1 << CLOCK_SDDAC_RST_CFG_SHIFT)
            | (0x1 << CLOCK_SDDAC_HSRST_CFG_SHIFT)),
        MODULE_RESET_HOLD);

    ak_adjust_clock_int_divider_freq(
        (factor_clk->reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL),
        CLOCK_SDDAC_DIV_VLD_CFG_SHIFT, CLOCK_SDDAC_CLK_EN_CFG_SHIFT, (div - 1),
        CLOCK_SDDAC_CLK_DIV_NUM_CFG_SHIFT, CLOCK_SDDAC_CLK_DIV_NUM_CFG_MASK);

    // ak_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
    //  CLOCK_SDDAC_RST_CFG_SHIFT, MODULE_RESET_RELEASE);
    ak_group_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
        ((0x1 << CLOCK_SDDAC_RST_CFG_SHIFT)
            | (0x1 << CLOCK_SDDAC_HSRST_CFG_SHIFT)),
        MODULE_RESET_RELEASE);

    regval = __raw_readl(factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);
    regval &= ~((CLOCK_DAC_OSR_CFG_MASK) << CLOCK_DAC_OSR_CFG_SHIFT);
    regval |= ((map_osr & CLOCK_DAC_OSR_CFG_MASK) << CLOCK_DAC_OSR_CFG_SHIFT);
    __raw_writel(regval, factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);

    regval = __raw_readl(factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);
    regval |= ((CLOCK_DAC_FILTER_EN_CFG_MASK) << CLOCK_DAC_FILTER_EN_CFG_SHIFT);
    __raw_writel(regval, factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);

    return 0;
}

/*
 * @BRIEF        adchs_set_div
 * @PARAM[in]    factor_clk
 * @PARAM[in]    factor
 * @RETURN       int
 * @RETVAL       sucess:0
 */
static int adchs_set_div(struct ak_factor_clk* factor_clk, int factor)
{
    // ak_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
    //  CLOCK_SDADC_HSRST_CFG_SHIFT, MODULE_RESET_HOLD);
    ak_group_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
        ((0x1 << CLOCK_SDADC_RST_CFG_SHIFT)
            | (0x1 << CLOCK_SDADC_HSRST_CFG_SHIFT)),
        MODULE_RESET_HOLD);

    ak_adjust_clock_int_divider_freq(
        (factor_clk->reg + CLOCK_SD_ADC_DAC_HS_CTRL),
        CLOCK_SDADC_HS_DIV_VLD_CFG_SHIFT, CLOCK_SDADC_HS_CLK_EN_CFG_SHIFT,
        factor, CLOCK_SDADC_HS_CLK_DIV_NUM_CFG_SHIFT,
        CLOCK_SDADC_HS_CLK_DIV_NUM_CFG_MASK);

    // ak_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
    //  CLOCK_SDADC_HSRST_CFG_SHIFT, MODULE_RESET_RELEASE);
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
 * @RETVAL       sucess:0
 */
static int dachs_set_div(struct ak_factor_clk* factor_clk, int factor)
{
    u32 regval;

    // ak_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
    //  CLOCK_SDDAC_HSRST_CFG_SHIFT, MODULE_RESET_HOLD);
    ak_group_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
        ((0x1 << CLOCK_SDDAC_RST_CFG_SHIFT)
            | (0x1 << CLOCK_SDDAC_HSRST_CFG_SHIFT)),
        MODULE_RESET_HOLD);

    regval = __raw_readl(factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);
    regval
        &= ~((CLOCK_DAC_FILTER_EN_CFG_MASK) << CLOCK_DAC_FILTER_EN_CFG_SHIFT);
    __raw_writel(regval, factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);

    ak_adjust_clock_int_divider_freq(
        (factor_clk->reg + CLOCK_SD_ADC_DAC_HS_CTRL),
        CLOCK_SDDAC_HS_DIV_VLD_CFG_SHIFT, CLOCK_SDDAC_HS_CLK_EN_CFG_SHIFT,
        factor, CLOCK_SDDAC_HS_CLK_DIV_NUM_CFG_SHIFT,
        CLOCK_SDDAC_HS_CLK_DIV_NUM_CFG_MASK);

    // ak_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
    //  CLOCK_SDDAC_HSRST_CFG_SHIFT, MODULE_RESET_RELEASE);
    ak_group_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
        ((0x1 << CLOCK_SDDAC_RST_CFG_SHIFT)
            | (0x1 << CLOCK_SDDAC_HSRST_CFG_SHIFT)),
        MODULE_RESET_RELEASE);

    regval = __raw_readl(factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);
    regval |= ((CLOCK_DAC_FILTER_EN_CFG_MASK) << CLOCK_DAC_FILTER_EN_CFG_SHIFT);
    __raw_writel(regval, factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);

    return 0;
}

/*
 * @BRIEF        ak_factor_asic_clk_set_rate
 * @PARAM[in]    hw
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       int
 * @RETVAL       sucess:0
 */
static int ak_factor_asic_clk_set_rate(
    struct clk_hw* hw, unsigned long rate, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 factor, regval;
    void __iomem* reg = factor_clk->reg;
    unsigned long flags;

    local_irq_save(flags);

    if (parent_rate > rate)
        factor = parent_rate / rate - 1;
    else
        factor = 0;

    switch (factor_clk->id) {
        case ASIC_FACTOR_CSI0_SCLK:
            parent_rate = ak_get_asic_pll_clk(factor_clk->reg);
            if (parent_rate > rate)
                factor = parent_rate / rate - 1;
            else
                factor = 0;
            ak_adjust_clock_int_divider_freq((reg + CLOCK_MAC_OPCLK_CTRL),
                CLOCK_CSI0_SCLK_DIV_VLD_CFG_SHIFT, CLOCK_CSI0_SCLK_EN_CFG_SHIFT,
                factor, CLOCK_CSI0_SCLK_DIV_NUM_CFG_SHIFT,
                CLOCK_CSI0_SCLK_DIV_NUM_CFG_MASK);

            regval = __raw_readl(reg + CLOCK_IMAGE_CAPTURE_CTRL);
            regval |= ((0x01 << CLOCK_CSI0_SCLK_SEL_CFG_SHIFT));
            __raw_writel(regval, reg + CLOCK_IMAGE_CAPTURE_CTRL);

            regval = __raw_readl(reg + CLOCK_IMAGE_CAPTURE_CTRL);
            regval &= (~(0x01 << CLOCK_CSI_PRST_CFG_SHIFT));
            __raw_writel(regval, reg + CLOCK_IMAGE_CAPTURE_CTRL);

            regval = __raw_readl(reg + CLOCK_IMAGE_CAPTURE_CTRL);
            regval &= (~(0x01 << CLOCK_CSI_DUAL_SENSOR_MODE_CFG_SHIFT));
            __raw_writel(regval, reg + CLOCK_IMAGE_CAPTURE_CTRL);

            regval = __raw_readl(reg + CLOCK_IMAGE_CAPTURE_CTRL);
            regval &= (~(0x01UL << CLOCK_CSI_DVP_SENSOR_SEL_CFG_SHIFT));
            __raw_writel(regval, reg + CLOCK_IMAGE_CAPTURE_CTRL);

            ak_module_reset_by_clock(factor_clk->reg + CLOCK_IMAGE_CAPTURE_CTRL,
                CLOCK_CSI_PRST_CFG_SHIFT);
            pr_info("%s: CSI0 SCLK %luMHz->%luMhz(%u)\n", __func__,
                parent_rate / MHz, rate / MHz, factor);
            pr_info("%s: IMG CFG1 0x%x CFG2 0x%x\n", __func__,
                __raw_readl(reg + CLOCK_IMAGE_CAPTURE_CTRL),
                __raw_readl(reg + CLOCK_MAC_OPCLK_CTRL));
            break;
        case ASIC_FACTOR_CSI1_SCLK:
            // TODO Later need to check with module onwers
            if (rate >= (24 * MHz)) {
                regval = __raw_readl(reg + CLOCK_IMAGE_CAPTURE_CTRL);
                regval |= (0x1 << CLOCK_CSI1_SCLK_SEL_CFG_SHIFT);
                __raw_writel(regval, reg + CLOCK_IMAGE_CAPTURE_CTRL);
            } else {
                ak_adjust_clock_int_divider_freq((reg + CLOCK_SPI_HS_CTRL),
                    CLOCK_CSI1_SCLK_DIV_VLD_CFG_SHIFT,
                    CLOCK_CSI1_SCLK_EN_CFG_SHIFT, factor,
                    CLOCK_CSI1_SCLK_DIV_NUM_CFG_SHIFT,
                    CLOCK_CSI1_SCLK_DIV_NUM_CFG_MASK);
            }
            break;
        case ASIC_FACTOR_MAC0_OPCLK:
            ak_adjust_clk_even_divider_freq((reg + CLOCK_MAC_OPCLK_CTRL),
                CLOCK_MAC0_OPCLK_DIV_VLD_CFG_SHIFT,
                CLOCK_MAC0_OPCLK_EN_CFG_SHIFT, factor,
                CLOCK_MAC0_DIV_NUM_CFG_SHIFT, CLOCK_MAC0_DIV_NUM_CFG_MASK);
            break;
        case ASIC_FACTOR_MAC1_OPCLK:
            ak_adjust_clk_even_divider_freq((reg + CLOCK_MAC_OPCLK_CTRL),
                CLOCK_MAC1_OPCLK_DIV_VLD_CFG_SHIFT,
                CLOCK_MAC1_OPCLK_EN_CFG_SHIFT, factor,
                CLOCK_MAC1_DIV_NUM_CFG_SHIFT, CLOCK_MAC1_DIV_NUM_CFG_MASK);
            break;
        case ASIC_FACTOR_LCD_PCLK:
            /*
             * Please refer to PG 3.2.3.2.3 LCD controller clock
             * ASIC PLL -- lcd_pclk_div_num_cfg - then the lcd_pdk_sel need care
             * about
             */
            ak_adjust_clk_even_divider_freq((reg + CLOCK_LCD_PCLK_CTRL),
                CLOCK_LCD_PCLK_DIV_VLD_CFG_SHIFT, CLOCK_LCD_PCLK_EN_CFG_SHIFT,
                factor, CLOCK_LCD_PCLK_DIV_VLD_CFG_SHIFT,
                CLOCK_LCD_PCLK_DIV_NUM_CFG_MASK);
            ak_module_reset_by_clock(factor_clk->reg + CLOCK_SPI_HS_CTRL,
                CLOCK_LCD_PCLK_RST_CFG_SHIFT);
            break;
        case ASIC_FACTOR_SPI0_CLK:
            // TODO need to reset spi or not.
            /*
             * Please refer to PG 3.2.3.2.2 SPIO clock
             * ASIC PLL -- spi_hsclk_div_num_cfg - /2 then be SPI_CLK
             * NOTICE the "/2"
             */
            factor = parent_rate / (rate * 2) - 1;
            ak_adjust_clock_int_divider_freq((reg + CLOCK_SPI_HS_CTRL),
                CLOCK_SPI_HSDIV_VLD_CFG_SHIFT, CLOCK_SPI_HSCLK_EN_CFG_SHIFT,
                factor, CLOCK_SPI_HSCLK_DIV_NUM_CFG_SHIFT,
                CLOCK_SPI_HSCLK_DIV_NUM_CFG_MASK);
            // ak_module_reset_by_clock(factor_clk->reg + CLOCK_SPI_HS_CTRL,
            // CLOCK_SPI0_HSCLK_RST_CFG_SHIFT);
            pr_debug("SPI0 CLOCK_SPI_HS_CTRL 0x%x\n",
                __raw_readl(reg + CLOCK_SPI_HS_CTRL));
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
 * PDM采样率
 * pdm_sr = pdm_clk / pdm_osr, pdm_osr = fourty_8_sel * 4
 * pdm_io 时钟信号的频率
 * pdm_clk = pdm_i2sm_clk / 2
 * pdm_i2sm_clk
 * pdm_i2sm_clk = audio_pll_clk / (pdm_i2sm_clk_div_num_cfg + 1)M
 * @rate -> pdm_sr
 */
static unsigned int pdm_calc_div_ratio(unsigned long pll, unsigned int rate,
    unsigned int* div, unsigned int* decimation_ratio_cfg_val)
{
    unsigned int decimation_ratio_list[] = { 64, 48, 32, 24, 0, 0, 16, 12 };
    unsigned int decimation_ratio, decimation_ratio_sel;
    unsigned int i;

    if (!decimation_ratio_cfg_val)
        return 0;

    for (i = 0; i < sizeof(decimation_ratio_list) / sizeof(unsigned int); i++) {
        decimation_ratio_sel
            = sizeof(decimation_ratio_list) / sizeof(unsigned int) - i;
        if (decimation_ratio_list[decimation_ratio_sel] == 0)
            continue;

        /*find the pdm_osr*/
        decimation_ratio = decimation_ratio_list[decimation_ratio_sel] * 4;
        *div = pll % (rate * decimation_ratio * 2);

        if (*div != 0)
            continue;

        *div = pll / (rate * decimation_ratio * 2);
        pr_info("%s pll#%lu pdm_i2sm_clk %lu pdm_clk %lu pdm_sr %lu\n",
            __func__, pll, pll / (*div), pll / (*div * 2),
            pll / (*div * 2 * decimation_ratio_list[decimation_ratio_sel] * 4));

        break;
    }

    if (i == sizeof(decimation_ratio_list) / sizeof(unsigned int)) {
        printk(KERN_ERR "%s invaild decimation_ratio\n", __func__);
        return -EINVAL;
    }

    *decimation_ratio_cfg_val = decimation_ratio_sel;

    return 0;
}

/*
 * @BRIEF        pdm_set_div_ratio
 * @PARAM[in]    factor_clk
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       int
 * @RETVAL       fail:-EINVAL sucess:0
 */
static int pdm_set_div_ratio(struct ak_factor_clk* factor_clk,
    unsigned long rate, unsigned long parent_rate)
{
    int div, ratio;
    unsigned int reg_val;
    void __iomem* reg = factor_clk->reg;

    if (rate == 0) {
        reg_val = __raw_readl(reg + CLOCK_AUDIO_PLL_CTRL);
        reg_val &= ~(
            CLOCK_PDM_I2SM_CLK_EN_CFG_MASK << CLOCK_PDM_I2SM_CLK_EN_CFG_SHIFT);
        __raw_writel(reg_val, (reg + CLOCK_AUDIO_PLL_CTRL));
        return 0;
    }

    if (pdm_calc_div_ratio(parent_rate, rate, &div, &ratio)) {
        return -EINVAL;
    }

    ak_adjust_clock_int_divider_freq((reg + CLOCK_AUDIO_PLL_CTRL),
        CLOCK_PDM_I2SM_CLK_DIV_VLD_CFG_SHIFT, CLOCK_PDM_I2SM_CLK_EN_CFG_SHIFT,
        (div - 1), CLOCK_PDM_I2SM_CLK_DIV_NUM_CFG_SHIFT,
        CLOCK_PDM_I2SM_CLK_DIV_NUM_CFG_MASK);
    ak_module_reset_by_clock(factor_clk->reg + CLOCK_GATE_SOFT_RESET_CTRL,
        CLOCK_PDM_I2SM_CLK_RST_CFG);

    reg_val = __raw_readl(AK_VA_SUBCTRL + PDM_CTRL_REG);
    reg_val &= ~(PDM_CTRL_DECIMATION_RATIO_CFG_MASK
        << PDM_CTRL_DECIMATION_RATIO_CFG_SHIFT);
    reg_val |= ((ratio & PDM_CTRL_DECIMATION_RATIO_CFG_MASK)
        << PDM_CTRL_DECIMATION_RATIO_CFG_SHIFT);
    __raw_writel(reg_val, AK_VA_SUBCTRL + PDM_CTRL_REG);

    return 0;
}

/*
 * @BRIEF        pdm_set_hsclk
 * @PARAM[in]    factor_clk
 * @PARAM[in]    rate
 * @PARAM[in]    parent_rate
 * @RETURN       int
 * @RETVAL       sucess:0
 */
static int pdm_set_hsclk(struct ak_factor_clk* factor_clk, unsigned long rate,
    unsigned long parent_rate)
{
    unsigned int regval, factor;
    void __iomem* reg = factor_clk->reg;

    if (rate == 0) {
        regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
        regval
            &= ~(CLOCK_PDM_HS_CLK_EN_CFG_MASK << CLOCK_PDM_HS_CLK_EN_CFG_SHIFT);
        __raw_writel(regval, (reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL));
        return 0;
    }

    /*
     * hsclk need to bigger than the i2m_clk
     * here make the max hsclk no matter which sample rate pdm use.
     */
    factor = 0;
    ak_adjust_clock_int_divider_freq((reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL),
        CLOCK_PDM_HS_DIV_VLD_CFG_SHIFT, CLOCK_PDM_HS_CLK_EN_CFG_SHIFT, factor,
        CLOCK_PDM_HS_CLK_DIV_NUM_CFG_SHIFT, CLOCK_PDM_HS_CLK_DIV_NUM_CFG_MASK);
    ak_module_reset_by_clock(
        factor_clk->reg + CLOCK_GATE_SOFT_RESET_CTRL, CLOCK_PDM_HS_CLK_RST_CFG);

    return 0;
}

/*
 * @BRIEF        i2s0_mclk_sdadc_set_div
 * @PARAM[in]    factor_clk
 * @PARAM[in]    factor
 * @PARAM[in]    parent_rate
 * @RETURN       int
 * @RETVAL       sucess:0
 */
static int i2s0_mclk_sdadc_set_div(struct ak_factor_clk* factor_clk, int factor)
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

    ak_adjust_clock_int_divider_freq(
        (factor_clk->reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL),
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
 * @RETVAL       sucess:0
 */
static int i2s0_b_lr_sdadc_set_div(struct ak_factor_clk* factor_clk, int factor)
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

    ak_adjust_clock_int_divider_freq(
        (factor_clk->reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL),
        CLOCK_SDDAC_DIV_VLD_CFG_SHIFT, CLOCK_SDDAC_CLK_EN_CFG_SHIFT, factor,
        CLOCK_SDDAC_CLK_DIV_NUM_CFG_SHIFT, CLOCK_SDDAC_CLK_DIV_NUM_CFG_MASK);

    ak_group_clock_reset_module(factor_clk->reg + CLOCK_SOFT_RESET,
        ((0x1 << CLOCK_SDDAC_RST_CFG_SHIFT)
            | (0x1 << CLOCK_SDDAC_HSRST_CFG_SHIFT)),
        MODULE_RESET_RELEASE);

    regval = __raw_readl(factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);
    regval |= ((CLOCK_DAC_FILTER_EN_CFG_MASK) << CLOCK_DAC_FILTER_EN_CFG_SHIFT);
    __raw_writel(regval, factor_clk->reg + CLOCK_DAC_FADEOUT_CTRL);

    return 0;
}

/*
 * @BRIEF        i2s0_b_lr_sdadc_set_div
 * @PARAM[in]    factor_clk
 * @PARAM[in]    factor
 * @PARAM[in]    parent_rate
 * @RETURN       int
 * @RETVAL       sucess:0
 */
static int ak_factor_audio_clk_set_rate(
    struct clk_hw* hw, unsigned long rate, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 factor;
    void __iomem* reg = factor_clk->reg;
    unsigned long flags;
    u32 regval;

    local_irq_save(flags);

    parent_rate = ak_get_audio_pll_freq(reg);

    if (rate && (parent_rate > rate))
        factor = parent_rate / rate - 1;
    else
        factor = 0;

    switch (factor_clk->id) {
        case AUDIO_FACTOR_SDADC_HSCLK:
            if (rate == 0) {
                regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
                regval &= ~(CLOCK_SDADC_HS_CLK_EN_CFG_MASK
                    << CLOCK_SDADC_HS_CLK_EN_CFG_SHIFT);
                __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_HS_CTRL);
            } else {
                adchs_set_div(factor_clk, factor);
            }
            break;
        case AUDIO_FACTOR_SDDAC_HSCLK:
            if (rate == 0) {
                regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
                regval &= ~(CLOCK_SDDAC_HS_CLK_EN_CFG_MASK
                    << CLOCK_SDDAC_HS_CLK_EN_CFG_SHIFT);
                __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_HS_CTRL);
            } else {
                dachs_set_div(factor_clk, factor);
            }
            break;
        case AUDIO_FACTOR_SDADC_CLK:
            if (rate == 0) {
                regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
                regval &= ~(CLOCK_SDADC_CLK_EN_CFG_MASK
                    << CLOCK_SDADC_CLK_EN_CFG_SHIFT);
                __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
            } else {
                sdadc_set_div_osr(factor_clk, rate, parent_rate);
            }
            break;
        case AUDIO_FACTOR_SDDAC_CLK:
            if (rate == 0) {
                regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
                regval &= ~(CLOCK_SDDAC_CLK_EN_CFG_MASK
                    << CLOCK_SDDAC_CLK_EN_CFG_SHIFT);
                __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
            } else {
                sddac_set_div_osr(factor_clk, rate, parent_rate);
            }
            break;
        case AUDIO_FACTOR_PDM_HSCLK:
            if (rate == 0) {
                regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
                regval &= ~(CLOCK_PDM_HS_CLK_EN_CFG_MASK
                    << CLOCK_PDM_HS_CLK_EN_CFG_SHIFT);
                __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
            } else {
                pdm_set_hsclk(factor_clk, rate, parent_rate);
            }
            break;
        case AUDIO_FACTOR_PDM_I2SM_CLK:
            if (rate == 0) {
                regval = __raw_readl(reg + CLOCK_AUDIO_PLL_CTRL);
                regval &= ~(CLOCK_PDM_I2SM_CLK_EN_CFG_MASK
                    << CLOCK_PDM_I2SM_CLK_EN_CFG_SHIFT);
                __raw_writel(regval, reg + CLOCK_AUDIO_PLL_CTRL);
            } else {
                pdm_set_div_ratio(factor_clk, rate, parent_rate);
            }
            break;
        case AUDIO_FACTOR_I2S0_MCLK:
            if (rate == 0) {
                regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
                regval &= ~(CLOCK_SDADC_CLK_EN_CFG_MASK
                    << CLOCK_SDADC_CLK_EN_CFG_SHIFT);
                __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
            } else {
                i2s0_mclk_sdadc_set_div(factor_clk, factor);
            }
            break;
        case AUDIO_FACTOR_I2S0_B_LR_CLK:
            if (rate == 0) {
                regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
                regval &= ~(CLOCK_SDDAC_CLK_EN_CFG_MASK
                    << CLOCK_SDDAC_CLK_EN_CFG_SHIFT);
                __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
            } else {
                i2s0_b_lr_sdadc_set_div(factor_clk, factor);
            }
            break;
        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            break;
    }
    local_irq_restore(flags);

    pr_debug("audio set_rate %s(%d) rate %lu parent %lu\n", hw->init->name,
        factor_clk->id, rate, parent_rate);

    return 0;
}
/*end of func*/

/*
 * @BRIEF        sdadc_get_clk
 * @PARAM[in]    factor_clk
 * @PARAM[in]    parent_rate
 * @RETURN       unsigned long
 * @RETVAL       0 \sdadc_clk
 */
static unsigned long sdadc_get_clk(
    struct ak_factor_clk* factor_clk, unsigned long parent_rate)
{
    u32 div, osr;
    u32 regval_div, regval_osr;
    unsigned long sdadc_clk;
    void __iomem* reg = factor_clk ? (factor_clk->reg) : AK_VA_SYSCTRL;

    regval_div = __raw_readl(reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
    if (!((regval_div >> CLOCK_SDADC_CLK_EN_CFG_SHIFT)
            & CLOCK_SDADC_CLK_EN_CFG_MASK))
        return 0;

    regval_div = __raw_readl(reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
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
 * @BRIEF        sdadc_get_clk
 * @PARAM[in]    factor_clk
 * @PARAM[in]    parent_rate
 * @RETURN       unsigned long
 * @RETVAL       0 \sddac_clk
 */
static unsigned long sddac_get_clk(
    struct ak_factor_clk* factor_clk, unsigned long parent_rate)
{
    int div, osr;
    u32 regval_div, regval_osr;
    unsigned long sddac_clk;
    void __iomem* reg = factor_clk ? (factor_clk->reg) : AK_VA_SYSCTRL;

    regval_div = __raw_readl(reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
    if (!((regval_div >> CLOCK_SDDAC_CLK_EN_CFG_SHIFT)
            & CLOCK_SDDAC_CLK_EN_CFG_MASK))
        return 0;

    regval_div = __raw_readl(reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
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

static unsigned long pdm_get_clk(
    struct ak_factor_clk* factor_clk, unsigned long parent_rate)
{
    u32 regval, div, ratio;
    unsigned long pdm_clk;
    unsigned int pdm_ratio_list[] = { 64, 48, 32, 24, 0, 0, 16, 12 };
    void __iomem* reg = factor_clk ? (factor_clk->reg) : AK_VA_SYSCTRL;

    regval = __raw_readl(reg + CLOCK_AUDIO_PLL_CTRL);

    if (!((regval >> CLOCK_PDM_I2SM_CLK_EN_CFG_SHIFT)
            & CLOCK_PDM_I2SM_CLK_EN_CFG_MASK))
        return 0;

    div = ((regval >> CLOCK_PDM_I2SM_CLK_DIV_NUM_CFG_SHIFT)
              & CLOCK_PDM_I2SM_CLK_DIV_NUM_CFG_MASK)
        + 1;

    regval = __raw_readl(AK_VA_SUBCTRL + PDM_CTRL_REG);
    ratio = (regval >> PDM_CTRL_DECIMATION_RATIO_CFG_SHIFT)
        & PDM_CTRL_DECIMATION_RATIO_CFG_MASK;
    ratio = pdm_ratio_list[ratio] * 4;

    pdm_clk = parent_rate / (div * 2) / ratio;

    return pdm_clk;
}

static unsigned long ak_factor_asic_clk_recalc_rate(
    struct clk_hw* hw, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    void __iomem* reg = factor_clk->reg;
    u32 regval, factor = 0;
    unsigned long recalc_rate;

    pr_debug("%s %s(%d) parent %lu\n", __func__, hw->init->name, factor_clk->id,
        parent_rate);

    switch (factor_clk->id) {
        case ASIC_FACTOR_CSI0_SCLK:
            regval = __raw_readl(reg + CLOCK_MAC_OPCLK_CTRL);
            regval = (regval >> CLOCK_CSI0_SCLK_EN_CFG_SHIFT)
                & CLOCK_CSI0_SCLK_EN_CFG_MASK;
            if (regval) {
                regval = __raw_readl(reg + CLOCK_IMAGE_CAPTURE_CTRL);
                regval = (regval >> CLOCK_CSI0_SCLK_SEL_CFG_SHIFT)
                    & CLOCK_IMAGE_CAPTURE_CTRL_CFG_MASK;
                if (regval) {
                    // Use the PLL as the source clock of the sensor0 SCLK
                    regval = __raw_readl(reg + CLOCK_MAC_OPCLK_CTRL);
                    factor = (regval >> CLOCK_CSI0_SCLK_DIV_NUM_CFG_SHIFT)
                        & CLOCK_CSI0_SCLK_DIV_NUM_CFG_MASK;
                    recalc_rate = parent_rate / (factor + 1);
                } else {
                    // Use the external 24MHz as the source clock of the sensor0
                    // SCLK
                    return 24 * MHz;
                }
            } else {
                recalc_rate = 0;
            }
            break;
        case ASIC_FACTOR_CSI1_SCLK:
            regval = __raw_readl(reg + CLOCK_SPI_HS_CTRL);
            regval = (regval >> CLOCK_CSI1_SCLK_EN_CFG_SHIFT)
                & CLOCK_CSI1_SCLK_EN_CFG_MASK;
            if (regval) {
                regval = __raw_readl(reg + CLOCK_IMAGE_CAPTURE_CTRL);
                regval = (regval >> CLOCK_CSI1_SCLK_SEL_CFG_SHIFT)
                    & CLOCK_IMAGE_CAPTURE_CTRL_CFG_MASK;
                if (regval) {
                    regval = __raw_readl(reg + CLOCK_SPI_HS_CTRL);
                    factor = (regval >> CLOCK_CSI1_SCLK_DIV_NUM_CFG_SHIFT)
                        & CLOCK_CSI1_SCLK_DIV_NUM_CFG_MASK;
                    recalc_rate = parent_rate / (factor + 1);
                } else {
                    // Use the external 24MHz as the source clock of the sensor1
                    // SCLK
                    return 24 * MHz;
                }
            }
            break;
        case ASIC_FACTOR_MAC0_OPCLK:
            regval = __raw_readl(reg + CLOCK_MAC_OPCLK_CTRL);
            factor = (regval >> CLOCK_MAC0_DIV_NUM_CFG_SHIFT)
                & CLOCK_MAC0_DIV_NUM_CFG_MASK;
            recalc_rate = parent_rate / (factor + 1);
            break;
        case ASIC_FACTOR_MAC1_OPCLK:
            regval = __raw_readl(reg + CLOCK_MAC_OPCLK_CTRL);
            factor = (regval >> CLOCK_MAC1_DIV_NUM_CFG_SHIFT)
                & CLOCK_MAC1_DIV_NUM_CFG_MASK;
            recalc_rate = parent_rate / (factor + 1);
            break;
        case ASIC_FACTOR_LCD_PCLK:
            regval = __raw_readl(reg + CLOCK_LCD_PCLK_CTRL);
            factor = (regval >> CLOCK_LCD_PCLK_DIV_NUM_CFG_SHIFT)
                & CLOCK_LCD_PCLK_DIV_NUM_CFG_MASK;
            recalc_rate = parent_rate / (factor + 1);
            break;
        case ASIC_FACTOR_SPI0_CLK:
            regval = __raw_readl(reg + CLOCK_SPI_HS_CTRL);
            factor = (regval >> CLOCK_SPI_HSCLK_DIV_NUM_CFG_SHIFT)
                & CLOCK_SPI_HSCLK_DIV_NUM_CFG_MASK;
            recalc_rate = parent_rate / (2 * (factor + 1));
            break;
        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            break;
    }

    return recalc_rate;
}
/*end of func*/

static unsigned long ak_factor_audio_clk_recalc_rate(
    struct clk_hw* hw, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    void __iomem* reg = factor_clk->reg;
    u32 regval, factor = 0;
    unsigned long recalc_rate;

    parent_rate = ak_get_audio_pll_freq(reg);

    pr_debug("%s %s(%d) parent %lu\n", __func__, hw->init->name, factor_clk->id,
        parent_rate);

    switch (factor_clk->id) {
        case AUDIO_FACTOR_SDADC_HSCLK:
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
        case AUDIO_FACTOR_SDDAC_HSCLK:
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
        case AUDIO_FACTOR_SDADC_CLK:
            recalc_rate = sdadc_get_clk(factor_clk, parent_rate);
            break;
        case AUDIO_FACTOR_SDDAC_CLK:
            recalc_rate = sddac_get_clk(factor_clk, parent_rate);
            break;
        case AUDIO_FACTOR_PDM_HSCLK:
            regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
            factor = (regval >> CLOCK_PDM_HS_CLK_DIV_NUM_CFG_SHIFT)
                & CLOCK_PDM_HS_CLK_DIV_NUM_CFG_MASK;
            recalc_rate = parent_rate / (factor + 1);
            break;
        case AUDIO_FACTOR_PDM_I2SM_CLK:
            recalc_rate = pdm_get_clk(factor_clk, parent_rate);
            break;
        case AUDIO_FACTOR_I2S0_MCLK:
            regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
            if ((regval >> CLOCK_SDADC_CLK_EN_CFG_SHIFT)
                & CLOCK_SDADC_CLK_EN_CFG_MASK) {
                regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
                factor = (regval >> CLOCK_SDADC_CLK_DIV_NUM_CFG_SHIFT)
                    & CLOCK_SDADC_CLK_DIV_NUM_CFG_MASK;
                recalc_rate = parent_rate / (factor + 1);
            } else {
                recalc_rate = 0;
            }
            break;
        case AUDIO_FACTOR_I2S0_B_LR_CLK:
            regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
            if ((regval >> CLOCK_SDDAC_CLK_EN_CFG_SHIFT)
                & CLOCK_SDDAC_CLK_EN_CFG_MASK) {
                regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
                factor = (regval >> CLOCK_SDDAC_CLK_DIV_NUM_CFG_SHIFT)
                    & CLOCK_SDDAC_CLK_DIV_NUM_CFG_MASK;
                recalc_rate = parent_rate / (factor + 1);
            } else {
                recalc_rate = 0;
            }
            break;
        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            break;
    }

    return recalc_rate;
}

static void ak_factor_asic_clk_disable(struct clk_hw* hw)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 regval;
    void __iomem* reg = factor_clk->reg;

    pr_debug("%s %s(%d)\n", __func__, hw->init->name, factor_clk->id);

    switch (factor_clk->id) {
        case ASIC_FACTOR_CSI0_SCLK:
            regval = __raw_readl(reg + CLOCK_MAC_OPCLK_CTRL);
            regval &= ~(
                CLOCK_CSI0_SCLK_EN_CFG_MASK << CLOCK_CSI0_SCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_MAC_OPCLK_CTRL);
            break;
        case ASIC_FACTOR_CSI1_SCLK:
            regval = __raw_readl(reg + CLOCK_SPI_HS_CTRL);
            regval &= ~(
                CLOCK_CSI1_SCLK_EN_CFG_MASK << CLOCK_CSI1_SCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_SPI_HS_CTRL);
            break;
        case ASIC_FACTOR_MAC0_OPCLK:
            regval = __raw_readl(reg + CLOCK_MAC_OPCLK_CTRL);
            regval &= ~(
                CLOCK_MAC0_OPCLK_EN_CFG_MASK << CLOCK_MAC0_OPCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_MAC_OPCLK_CTRL);
            break;
        case ASIC_FACTOR_MAC1_OPCLK:
            regval = __raw_readl(reg + CLOCK_MAC_OPCLK_CTRL);
            regval &= ~(
                CLOCK_MAC1_OPCLK_EN_CFG_MASK << CLOCK_MAC1_OPCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_MAC_OPCLK_CTRL);
            break;
        case ASIC_FACTOR_LCD_PCLK:
            regval = __raw_readl(reg + CLOCK_LCD_PCLK_CTRL);
            regval
                &= ~(CLOCK_LCD_PCLK_EN_CFG_MASK << CLOCK_LCD_PCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_LCD_PCLK_CTRL);
            break;
        case ASIC_FACTOR_SPI0_CLK:
            regval = __raw_readl(reg + CLOCK_SPI_HS_CTRL);
            regval &= ~(
                CLOCK_SPI_HSCLK_EN_CFG_MASK << CLOCK_SPI_HSCLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_SPI_HS_CTRL);
            break;
        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            break;
    }
}

static void ak_factor_audio_clk_disable(struct clk_hw* hw)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 regval;
    void __iomem* reg = factor_clk->reg;

    pr_debug("%s %s(%d)\n", __func__, hw->init->name, factor_clk->id);

    switch (factor_clk->id) {
        case AUDIO_FACTOR_SDADC_HSCLK:
            regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
            regval &= ~(CLOCK_SDADC_HS_CLK_EN_CFG_MASK
                << CLOCK_SDADC_HS_CLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_HS_CTRL);
            break;
        case AUDIO_FACTOR_SDDAC_HSCLK:
            regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
            regval &= ~(CLOCK_SDDAC_HS_CLK_EN_CFG_MASK
                << CLOCK_SDDAC_HS_CLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_HS_CTRL);
            break;
        case AUDIO_FACTOR_SDADC_CLK:
            regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
            regval &= ~(
                CLOCK_SDADC_CLK_EN_CFG_MASK << CLOCK_SDADC_CLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
            break;
        case AUDIO_FACTOR_SDDAC_CLK:
            regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
            regval &= ~(
                CLOCK_SDDAC_CLK_EN_CFG_MASK << CLOCK_SDDAC_CLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
            break;
        case AUDIO_FACTOR_PDM_HSCLK:
            regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
            regval &= ~(
                CLOCK_PDM_HS_CLK_EN_CFG_MASK << CLOCK_PDM_HS_CLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
            break;
        case AUDIO_FACTOR_PDM_I2SM_CLK:
            regval = __raw_readl(reg + CLOCK_AUDIO_PLL_CTRL);
            regval &= ~(CLOCK_PDM_I2SM_CLK_EN_CFG_MASK
                << CLOCK_PDM_I2SM_CLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_AUDIO_PLL_CTRL);
            break;
        case AUDIO_FACTOR_I2S0_MCLK:
            regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
            regval &= ~(
                CLOCK_SDADC_CLK_EN_CFG_MASK << CLOCK_SDADC_CLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
            break;
        case AUDIO_FACTOR_I2S0_B_LR_CLK:
            regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
            regval &= ~(
                CLOCK_SDDAC_CLK_EN_CFG_MASK << CLOCK_SDDAC_CLK_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
            regval = __raw_readl(reg + CLOCK_DAC_FADEOUT_CTRL);
            regval &= ~((CLOCK_DAC_FILTER_EN_CFG_MASK)
                << CLOCK_DAC_FILTER_EN_CFG_SHIFT);
            __raw_writel(regval, reg + CLOCK_DAC_FADEOUT_CTRL);
            break;
        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            break;
    }
}

const struct clk_ops ak_factor_asic_clk_ops = {
    .round_rate = ak_factor_asic_clk_round_rate,
    .set_rate = ak_factor_asic_clk_set_rate,
    .recalc_rate = ak_factor_asic_clk_recalc_rate,
    .disable = ak_factor_asic_clk_disable,
};

const struct clk_ops ak_factor_audio_clk_ops = {
    .round_rate = ak_factor_audio_clk_round_rate,
    .set_rate = ak_factor_audio_clk_set_rate,
    .recalc_rate = ak_factor_audio_clk_recalc_rate,
    .disable = ak_factor_audio_clk_disable,
};

static long ak_factor_sar_adc_clk_round_rate(
    struct clk_hw* hw, unsigned long rate, unsigned long* parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    unsigned long round_rate = rate;

    pr_debug("factor round_rate %s(%d) rate %lu parent %luMHz\n",
        hw->init->name, factor_clk->id, round_rate, (*parent_rate) / MHz);

    return round_rate;
}

static int ak_factor_sar_adc_clk_set_rate(
    struct clk_hw* hw, unsigned long rate, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    u32 factor, regval;
    void __iomem* reg = factor_clk->reg;
    unsigned long flags;

    local_irq_save(flags);

    switch (factor_clk->id) {
        case SAR_FACTOR_ADC_CLK:
            factor = parent_rate / rate - 1;

            regval = __raw_readl(reg + CLOCK_SAR_ADC_CTRL);
            regval &= ~((0x1) << CLOCK_SAR_ADC_CLK_EN_SHIFT);
            __raw_writel(regval, (reg + CLOCK_SAR_ADC_CTRL));

            regval = __raw_readl(reg + CLOCK_SAR_ADC_CTRL);
            regval &= ~(CLOCK_SAR_ADC_CLK_DIV_NUM_CFG_MASK
                << CLOCK_SAR_ADC_CLK_DIV_NUM_CFG_SHIFT);
            regval |= ((CLOCK_SAR_ADC_CLK_DIV_NUM_CFG_MASK & factor)
                << CLOCK_SAR_ADC_CLK_DIV_NUM_CFG_SHIFT);
            __raw_writel(regval, (reg + CLOCK_SAR_ADC_CTRL));

            regval = __raw_readl(reg + CLOCK_SAR_ADC_CTRL);
            regval |= ((0x1) << CLOCK_SAR_ADC_CLK_EN_SHIFT);
            __raw_writel(regval, (reg + CLOCK_SAR_ADC_CTRL));
            break;
        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            break;
    }
    local_irq_restore(flags);

    pr_debug("factor set_sar_rate %s(%d) rate %lu parent %lu\n", hw->init->name,
        factor_clk->id, rate, parent_rate);

    return 0;
}

static unsigned long akfactor_sar_adc_clk_recalc_rate(
    struct clk_hw* hw, unsigned long parent_rate)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    void __iomem* reg = factor_clk->reg;
    u32 regval, factor = 0;
    unsigned long recalc_rate;

    pr_debug("%s %s(%d) parent %lu\n", __func__, hw->init->name, factor_clk->id,
        parent_rate);

    switch (factor_clk->id) {
        case SAR_FACTOR_ADC_CLK:
            regval = __raw_readl(reg + CLOCK_SAR_ADC_CTRL);
            factor = (regval >> CLOCK_SAR_ADC_CLK_DIV_NUM_CFG_SHIFT)
                & CLOCK_SAR_ADC_CLK_DIV_NUM_CFG_MASK;
            recalc_rate = parent_rate / (factor + 1);
            break;
        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            break;
    }

    return recalc_rate;
}

static void ak_factor_sar_adc_clk_disable(struct clk_hw* hw)
{
    struct ak_factor_clk* factor_clk = to_clk_ak_factor(hw);
    void __iomem* reg = factor_clk->reg;
    u32 regval;

    switch (factor_clk->id) {
        case SAR_FACTOR_ADC_CLK:
            regval = __raw_readl(reg + CLOCK_SAR_ADC_CTRL);
            regval &= ~((0x1) << CLOCK_SAR_ADC_CLK_EN_SHIFT);
            __raw_writel(regval, (reg + CLOCK_SAR_ADC_CTRL));
            break;
        default:
            pr_err("%s: unknow ID: %d\n", __func__, factor_clk->id);
            break;
    }
}

const struct clk_ops ak_factor_sar_adc_clk_ops = {
    .disable = ak_factor_sar_adc_clk_disable,
    .round_rate = ak_factor_sar_adc_clk_round_rate,
    .set_rate = ak_factor_sar_adc_clk_set_rate,
    .recalc_rate = akfactor_sar_adc_clk_recalc_rate,
};

static int ak_gate_clk_enable(struct clk_hw* hw)
{
    struct ak_gate_clk* gate_clk = to_clk_ak_gate(hw);
    u32 con;
    unsigned long flags;
    void __iomem* reg;
    int ctrlbit;

    if (gate_clk->id == ASIC_GCLK_BRIDGE_MODULE) {
        /*
         * NOTE:special clock.software cannot config it.Ignore.
         */
        return 0;
    }

    local_irq_save(flags);
    ctrlbit = gate_clk->ctrlbit % AK_CLOCK_GATE_GROUP;
    if (gate_clk->ctrlbit < AK_CLOCK_GATE_GROUP) {
        reg = gate_clk->reg + CLOCK_GATE_CTRL;
    } else {
        reg = gate_clk->reg + CLOCK_GATE_SOFT_RESET_CTRL;
    }

    con = __raw_readl(reg);
    con &= ~(1UL << ctrlbit);
    __raw_writel(con, reg);

    ctrlbit = -1;
    switch (gate_clk->id) {
        case ASIC_GCLK_MMC_0:
            ctrlbit = GCLK_MMC0_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_MMC_1:
            ctrlbit = GCLK_MMC1_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_SD_ADC:
            ctrlbit = GCLK_SDADC_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_SD_DAC:
            ctrlbit = GCLK_SDDAC_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_SPI0:
            ctrlbit = GCLK_SPI0_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_SPI1:
            ctrlbit = GCLK_SPI1_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_UART0:
            ctrlbit = GCLK_UART0_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_UART1:
            ctrlbit = GCLK_UART1_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_L2BUF0:
            ctrlbit = GCLK_L2BUF0_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_TWI0:
            ctrlbit = GCLK_TWI0_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_L2BUF1:
            ctrlbit = GCLK_L2BUF1_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_GPIO:
            /*
             * Due to LCD use the gpio pin to control power
             * IF reset the GPIO controller the make the GPIO73
             * to pull up. Then make the LCD display unnormally.
             * HERE not reset the GPIO controller.
             */
            break;
        case ASIC_GCLK_MAC0:
            ctrlbit = GCLK_MAC0_BUFFER_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_PDM:
            ctrlbit = GCLK_PDM_CONTROLLER_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_USB:
            ctrlbit = GCLK_USB_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_MAC1:
            ctrlbit = GCLK_MAC1_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_TWI1:
            ctrlbit = GCLK_TWI1_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_UART2:
            ctrlbit = GCLK_UART2_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_UART3:
            ctrlbit = GCLK_UART3_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_SPI2:
            ctrlbit = GCLK_SPI2_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_MMC_2:
            ctrlbit = GCLK_MMC2_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_TWI2:
            ctrlbit = GCLK_TWI2_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_TWI3:
            ctrlbit = GCLK_TWI3_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_IMAGE_CAPTURE:
            ctrlbit = VCLK_IMAGE_CAPTURE_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_JPEG_CODEC:
            ctrlbit = VCLK_JPEG_CODEC_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_VIDEO_DECODER:
            ctrlbit = VCLK_VIDEO_DECODER_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_LCD_CONTROLLER:
            ctrlbit = VCLK_LCD_CONTROLLER_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_GUI:
            ctrlbit = VCLK_GUI_RST_CFG_SHIFT;
            break;
        case ASIC_GCLK_DSI:
            ctrlbit = CLOCK_MIPI_DIS_RST_CFG_SHIFT;
            break;
    }

    reg = gate_clk->reg + CLOCK_SOFT_RESET;
    if ((gate_clk->id >= ASIC_GCLK_UART2) && (gate_clk->id <= ASIC_GCLK_TWI3)) {
        reg = gate_clk->reg + CLOCK_GATE_SOFT_RESET_CTRL;
    }

    if (ctrlbit < 0)
        goto exit_gc_enable;

    ak_module_reset_by_clock(reg, ctrlbit);

    pr_debug("gate %d enable(ctrl %d) rst %d@0x%p\n", gate_clk->id,
        gate_clk->ctrlbit, ctrlbit, reg);

exit_gc_enable:
    local_irq_restore(flags);
    return 0;
}
/*end of func*/

static void ak_gate_clk_disable(struct clk_hw* hw)
{
    struct ak_gate_clk* gate_clk = to_clk_ak_gate(hw);
    u32 con;
    unsigned long flags;
    void __iomem* reg;
    int ctrlbit;

    if (gate_clk->id == ASIC_GCLK_BRIDGE_MODULE) {
        /*
         * NOTE:special clock.software cannot config it.Ignore.
         */
        return;
    }

    local_irq_save(flags);
    ctrlbit = gate_clk->ctrlbit % AK_CLOCK_GATE_GROUP;
    if (gate_clk->ctrlbit < AK_CLOCK_GATE_GROUP) {
        reg = gate_clk->reg + CLOCK_GATE_CTRL;
    } else {
        reg = gate_clk->reg + CLOCK_GATE_SOFT_RESET_CTRL;
    }

    con = __raw_readl(reg);
    con |= (0x1UL << ctrlbit);
    __raw_writel(con, reg);

    pr_debug("gate %d disable(ctrl %d)@0x%p\n", gate_clk->id, gate_clk->ctrlbit,
        reg);

    local_irq_restore(flags);
}

static unsigned long ak_gate_clk_recalc_rate(
    struct clk_hw* hw, unsigned long parent_rate)
{
    struct ak_gate_clk* gate_clk = to_clk_ak_gate(hw);
    unsigned long rate;
    u32 regval;
    int ctrlbit;

    ctrlbit = gate_clk->ctrlbit % AK_CLOCK_GATE_GROUP;
    if (gate_clk->ctrlbit < AK_CLOCK_GATE_GROUP) {
        regval = __raw_readl(gate_clk->reg + CLOCK_GATE_CTRL);
    } else {
        regval = __raw_readl(gate_clk->reg + CLOCK_GATE_SOFT_RESET_CTRL);
    }
    if ((regval >> ctrlbit) & 0x01) {
        pr_debug("gate %d rate 0\n", gate_clk->id);
        return 0;
    }

    regval = __raw_readl(gate_clk->reg + CLOCK_ASIC_PLL_CTRL);
    regval = ((regval >> CLOCK_ASIC_VCLK_DIV_NUM_CFG_SHIFT)
        & CLOCK_ASIC_VCLK_DIV_NUM_CFG_MASK);
    rate = parent_rate / (2 * (regval + 1));

    if ((gate_clk->id >= ASIC_GCLK_MMC_0) && (gate_clk->id <= ASIC_GCLK_TWI3)) {
        regval = __raw_readl(gate_clk->reg + CLOCK_ASIC_PLL_CTRL);
        regval = ((regval >> CLOCK_ASIC_GCLK_SEL_SHIFT)
            & CLOCK_ASIC_GCLK_SEL_MASK);
        if (regval)
            rate = rate / 2;
    }

    pr_debug("gate %d rate %lu\n", gate_clk->id, rate);

    return rate;
}

const struct clk_ops ak_gate_clk_ops = {
    .enable = ak_gate_clk_enable,
    .disable = ak_gate_clk_disable,
    .recalc_rate = ak_gate_clk_recalc_rate,
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
    init.flags = CLK_IGNORE_UNUSED;
    if (!strcmp(name, "pdm_i2sm") || !strcmp(name, "i2s0_mclk")
        || !strcmp(name, "i2s0_b_lr_clk"))
        init.flags |= CLK_GET_RATE_NOCACHE;
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

struct clk* __init ak_clk_register_gate(const char* name,
    const char* parent_name, void __iomem* reg_base, int id, u32 ctrlbit,
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
    init.flags = CLK_GET_RATE_NOCACHE;

    gate_clk->id = id;
    gate_clk->ctrlbit = ctrlbit;
    gate_clk->reg = reg_base;

    gate_clk->hw.init = &init;

    clk = clk_register(NULL, &gate_clk->hw);
    if (IS_ERR(clk))
        kfree(gate_clk);

    return clk;
}

/******************************************************************************
 * !! NOTE !!
 * CPU PLL:
 * 1 <= N <=24, 2<= M, OD >=1
 * 500MHz <= 24*M/N <= 1500MHz
 * 62.5MHz <= cpu_pll_clk <= 800MHz.Maximun depends on MODE AND VDD
 *
 ******************************************************************************/

/*
 * asic_pll_init
 * default asci pll is 120MHz
 * asic_pll_clk = 24*M/(N*2^OD).
 * NOTE: need asic_freq_adj_cfg CLOCK_CPU_PLL_CTRL
 * Bit[ASIC_PLL_FREQ_ADJ_CFG_SHIFT] as 1 1 <= N <= 24 M >= 2 OD >= 1 500MHz <=
 * 24*M/N <= 1500MHz 62.5MHz <= asic_pll_clk <= 500MHz NOTE: the maximun value
 * depends on the MODE AND VDD CPU/DDR2=800/400MHZ
 * ASIC_CLK/VCLK/GCLK=480/240/120MHZ
 * vclk = asic_pll_clk/(2*(vclk_div_num_cfg+1))
 */
static void asic_pll_init(
    void __iomem* reg, u32 asic_pll, u32 div_od, u32 div_n)
{
    u32 div_m;
    u32 rd_div_m;
    u32 rd_div_n;
    u32 rd_div_od;
    u32 regval;
    int timeout;

    /*
     * NOTE:close gate clock which not need be opened by default
     */
    regval = __raw_readl(reg + CLOCK_GATE_CTRL);
    regval |= (1 << CLOCK_VIDEO_DECODER_CFG_SHIFT);
    regval |= (1 << CLOCK_JPEG_CODEC_CFG_SHIFT);
    regval |= (1 << CLOCK_IMAGE_CAPTURE_CFG_SHIFT);
    __raw_writel(regval, reg + CLOCK_GATE_CTRL);

    /*
     * NOTICE that wait the asic_pll_freq_adj_cfg to 0 then update
     * the m/n/od of the ASIC PLL
     * THEN set the asic_pll_freq_adj_cfg to 1.
     */

    if ((div_od < 1) || (div_n < 1) || (div_n > 24))
        panic("ASIC pll frequency parameter Error");

    regval = __raw_readl(reg + CLOCK_ASIC_PLL_CTRL);
    rd_div_m
        = (regval >> CLOCK_ASIC_PLL_M_CFG_SHIFT) & CLOCK_ASIC_PLL_M_CFG_MASK;
    rd_div_n
        = (regval >> CLOCK_ASIC_PLL_N_CFG_SHIFT) & CLOCK_ASIC_PLL_N_CFG_MASK;
    rd_div_od
        = (regval >> CLOCK_ASIC_PLL_OD_CFG_SHIFT) & CLOCK_ASIC_PLL_OD_CFG_MASK;

    div_m = ((asic_pll / MHz) * (div_n * (1 << div_od))) / 24;

    /*
     * Since we use mipi to display in uboot. Here we reconfig the asic clock
     * will make the display unnormally.
     */
    if ((div_m == rd_div_m) && (rd_div_n == div_n) && (rd_div_od == div_od)) {
        pr_info("asic pll has inited!\n");
        goto exit;
    }

    if (div_m < 2)
        panic("ASIC pll frequency parameter Error");

    // pr_info("m 0x%x n 0x%x od 0x%x\n", rd_div_m, rd_div_n, rd_div_od);

    /*
     * 1. First wait the asic_pll_freq_adj_cfg to 0
     */
    timeout = CLOCK_FREQ_ADJ_WAITING_MAX_NUM;
    do {
        timeout--;
        regval = ((__raw_readl(reg + CLOCK_CPU_PLL_CTRL))
                     >> ASIC_PLL_FREQ_ADJ_CFG_SHIFT)
            & ASIC_PLL_FREQ_ADJ_CFG_MASK;
    } while (regval && (timeout > 0));
    if (timeout <= 0) {
        pr_err(
            "%s:%d waiting asic pll freq adjust timeout\n", __func__, __LINE__);
    }

    /*
     * 2. Update the configuration for asic_pll
     */
    regval = ((0x1 << CLOCK_ASIC_GCLK_SEL_SHIFT)
        | (0x1 << CLOCK_ASIC_VCLK_DIV_VLD_CFG_SHIFT)
        | ((0x0 & CLOCK_ASIC_VCLK_DIV_NUM_CFG_MASK)
            << CLOCK_ASIC_VCLK_DIV_NUM_CFG_SHIFT)
        | (div_od << CLOCK_ASIC_PLL_OD_CFG_SHIFT)
        | (div_n << CLOCK_ASIC_PLL_N_CFG_SHIFT)
        | (div_m << CLOCK_ASIC_PLL_M_CFG_SHIFT));
    __raw_writel(regval, reg + CLOCK_ASIC_PLL_CTRL);

    /*
     * 3. SET asic_pll_freq_adj_cfg to 1 to make valid
     */
    regval = __raw_readl(reg + CLOCK_CPU_PLL_CTRL);
    regval |= ((0x1) << ASIC_PLL_FREQ_ADJ_CFG_SHIFT);
    __raw_writel(regval, reg + CLOCK_CPU_PLL_CTRL);

    timeout = CLOCK_FREQ_ADJ_WAITING_MAX_NUM;
    do {
        timeout--;
        regval = ((__raw_readl(reg + CLOCK_CPU_PLL_CTRL))
                     >> ASIC_PLL_FREQ_ADJ_CFG_SHIFT)
            & ASIC_PLL_FREQ_ADJ_CFG_MASK;
    } while (regval && (timeout > 0));
    if (timeout <= 0) {
        pr_err(
            "%s:%d waiting asic pll freq adjust timeout\n", __func__, __LINE__);
    }

exit:
    pr_info("ASIC: %lu(Mhz)\n", ak_get_asic_pll_clk(reg) / MHz);
    pr_info("VCLK: %lu(Mhz)\n", ak_get_vclk(reg) / MHz);
}
/*end of func*/

static void audio_pll_init(void __iomem* reg)
{
    u32 regval;

    ak_set_audio_pll_freq(reg, audio_pll[0]);

    regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_HS_CTRL);
    regval
        &= ~(CLOCK_SDADC_HS_CLK_EN_CFG_MASK << CLOCK_SDADC_HS_CLK_EN_CFG_SHIFT);
    regval
        &= ~(CLOCK_SDDAC_HS_CLK_EN_CFG_MASK << CLOCK_SDDAC_HS_CLK_EN_CFG_SHIFT);
    __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_HS_CTRL);

    regval = __raw_readl(reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
    regval &= ~(CLOCK_SDADC_CLK_EN_CFG_MASK << CLOCK_SDADC_CLK_EN_CFG_SHIFT);
    regval &= ~(CLOCK_SDDAC_CLK_EN_CFG_MASK << CLOCK_SDDAC_CLK_EN_CFG_SHIFT);
    regval &= ~(CLOCK_PDM_HS_CLK_EN_CFG_MASK << CLOCK_PDM_HS_CLK_EN_CFG_SHIFT);
    regval
        &= ~(CLOCK_PDM_I2SM_CLK_EN_CFG_MASK << CLOCK_PDM_I2SM_CLK_EN_CFG_SHIFT);
    __raw_writel(regval, reg + CLOCK_SD_ADC_DAC_AUDIO_CTRL);
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

    if (!strcmp(clk_name, "asic_pll")) {
        asic_pll_init(res_reg, rate, div_od, div_n);
    }

    if (!strcmp(clk_name, "audio_pll")) {
        audio_pll_init(res_reg);
    }

    for (index = 0; index < number; index++) {
        of_property_read_string_index(
            np, "clock-output-names", index, &clk_name);
        of_property_read_u32_index(np, "clock-id", index, &id);
        pr_debug("clk %s(%s) rate %d id %d\n", clk_name, parent_name, rate, id);
        clk_data->clks[index] = ak_register_fixed_clk(
            clk_name, parent_name, res_reg, rate, id, &ak_fixed_clk_ops);
        if (IS_ERR(clk_data->clks[index])) {
            pr_err("register fix clk failed: clk_name:%s, index = %d\n",
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

static void __init of_ak_factor_audio_clk_init(struct device_node* np)
{
    struct clk_onecell_data* clk_data;
    const char* clk_name = np->name;
    const char* parent_name = of_clk_get_parent_name(np, 0);
    void __iomem* res_reg = AK_VA_SYSCTRL;
    int id, index, number;

    clk_data = kmalloc(sizeof(struct clk_onecell_data), GFP_KERNEL);
    if (!clk_data)
        return;

    number = of_property_count_u32_elems(np, "clock-id");
    clk_data->clks = kcalloc(number, sizeof(struct clk*), GFP_KERNEL);
    if (!clk_data->clks)
        goto err_free_data;

    for (index = 0; index < number; index++) {
        of_property_read_string_index(
            np, "clock-output-names", index, &clk_name);
        of_property_read_u32_index(np, "clock-id", index, &id);
        pr_debug(
            "audio clk %s %s id %d idx %d\n", clk_name, parent_name, id, index);
        clk_data->clks[index] = ak_register_factor_clk(
            clk_name, parent_name, res_reg, id, &ak_factor_audio_clk_ops);
        if (IS_ERR(clk_data->clks[index])) {
            pr_err("register factor clk failed: clk_name:%s, err = %s\n",
                clk_name, (char*)PTR_ERR(clk_data->clks[index]));
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
}

static void __init of_ak_factor_aisc_clk_init(struct device_node* np)
{
    struct clk_onecell_data* clk_data;
    const char* clk_name = np->name;
    const char* parent_name = of_clk_get_parent_name(np, 0);
    void __iomem* res_reg = AK_VA_SYSCTRL;
    int id, index, number;

    clk_data = kmalloc(sizeof(struct clk_onecell_data), GFP_KERNEL);
    if (!clk_data)
        return;

    number = of_property_count_u32_elems(np, "clock-id");
    clk_data->clks = kcalloc(number, sizeof(struct clk*), GFP_KERNEL);
    if (!clk_data->clks)
        goto err_free_data;

    for (index = 0; index < number; index++) {
        of_property_read_string_index(
            np, "clock-output-names", index, &clk_name);
        of_property_read_u32_index(np, "clock-id", index, &id);
        if (id == ASIC_FACTOR_LCD_PCLK) {
            res_reg = AK_VA_LCD;
        } else {
            res_reg = AK_VA_SYSCTRL;
        }
        pr_debug(
            "aisc clk %s %s id %d idx %d\n", clk_name, parent_name, id, index);
        clk_data->clks[index] = ak_register_factor_clk(
            clk_name, parent_name, res_reg, id, &ak_factor_asic_clk_ops);
        if (IS_ERR(clk_data->clks[index])) {
            pr_err("register factor clk failed: clk_name:%s, err = %s\n",
                clk_name, (char*)PTR_ERR(clk_data->clks[index]));
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
}

static void __init of_ak_factor_sar_adc_clk_init(struct device_node* np)
{
    struct clk_onecell_data* clk_data;
    const char* clk_name = np->name;
    const char* parent_name = of_clk_get_parent_name(np, 0);
    void __iomem* res_reg = AK_VA_SYSCTRL;
    int id, index, number;

    clk_data = kmalloc(sizeof(struct clk_onecell_data), GFP_KERNEL);
    if (!clk_data)
        return;

    number = of_property_count_u32_elems(np, "clock-id");
    clk_data->clks = kcalloc(number, sizeof(struct clk*), GFP_KERNEL);
    if (!clk_data->clks)
        goto err_free_data;

    for (index = 0; index < number; index++) {
        of_property_read_string_index(
            np, "clock-output-names", index, &clk_name);
        of_property_read_u32_index(np, "clock-id", index, &id);
        pr_debug(
            "sar clk %s %s id %d idx %d\n", clk_name, parent_name, id, index);
        clk_data->clks[index] = ak_register_factor_clk(
            clk_name, parent_name, res_reg, id, &ak_factor_sar_adc_clk_ops);
        if (IS_ERR(clk_data->clks[index])) {
            pr_err("register sar clk failed: clk_name:%s, err = %s\n", clk_name,
                (char*)PTR_ERR(clk_data->clks[index]));
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
}

static void __init of_ak_gate_clk_init(struct device_node* np)
{
    struct clk_onecell_data* clk_data;
    const char* clk_name = np->name;
    const char* parent_name = of_clk_get_parent_name(np, 0);
    void __iomem* reg = AK_VA_SYSCTRL;
    int number, index;
    u32 id, ctrlbit;

    clk_data = kmalloc(sizeof(struct clk_onecell_data), GFP_KERNEL);
    if (!clk_data)
        return;

    number = of_property_count_u32_elems(np, "clock-id");

    clk_data->clks = kcalloc(number, sizeof(struct clk*), GFP_KERNEL);
    if (!clk_data->clks)
        goto err_free_data;

    for (index = 0; index < number; index++) {
        of_property_read_string_index(
            np, "clock-output-names", index, &clk_name);
        of_property_read_u32_index(np, "clock-id", index, &id);
        of_property_read_u32_index(np, "clock-ctrlbit", index, &ctrlbit);
        pr_debug("gclk %s(%s) idx %d id %d ctrlbit %d\n", clk_name, parent_name,
            index, id, ctrlbit);

        clk_data->clks[index] = ak_clk_register_gate(
            clk_name, parent_name, reg, id, ctrlbit, &ak_gate_clk_ops);
        if (IS_ERR(clk_data->clks[index])) {
            pr_err("register gate clk failed: clk_name:%s, index = %d\n",
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
    /*
     * [21:20]:reset_cfg
     * Threshold control of reset
     * 3: reset when VDDIO < 2.48V
     * 2/1: reset when VDDIO < 2.86V
     * 0: reset when VDDIO < 2.72V
     */
    regval = __raw_readl(reg + ANALOG_CTRL_REG3);
    regval &= ~(0x3 << 20);
    regval |= threshold << 20;
    if (enable)
        regval &= ~(1 << 30);
    else
        regval |= 1 << 30;
    __raw_writel(regval, reg + ANALOG_CTRL_REG3);
    /*
     * Bit30 as same as the 0x0800,0218 Bit17
     */
}

CLK_OF_DECLARE(ak37e_fixed_clk, "anyka,ak37e-fixed-clk", of_ak_fixed_clk_init);
CLK_OF_DECLARE(ak37e_factor_asic_clk, "anyka,ak37e-factor-asic-clk",
    of_ak_factor_aisc_clk_init);
CLK_OF_DECLARE(ak37e_factor_audio_clk, "anyka,ak37e-factor-audio-clk",
    of_ak_factor_audio_clk_init);
CLK_OF_DECLARE(ak37e_factor_sar_adc_clk, "anyka,ak37e-factor-sar-adc-clk",
    of_ak_factor_sar_adc_clk_init);
CLK_OF_DECLARE(ak37e_gate_clk, "anyka,ak37e-gate-clk", of_ak_gate_clk_init);
CLK_OF_DECLARE(ak37e_uv_det_pd, "anyka,ak37e-uv_det_pd", of_ak_uv_det_pd);

/*
 * /proc/clk
 */
static int proc_clk_show(struct seq_file* m, void* v)
{
#ifdef AK_CLK_DUMP_INFO
    clk_dump_registers();
#endif
    seq_printf(m,
        "aisc_pll: %lu MHz\n"
        "vclk: %lu MHz\n"
        "Audio PLL %luMHz\n"
        "adc_rate: %lu Hz\n"
        "dac_rate: %lu Hz\n"
        "adc_hs: %lu MHz\n"
        "dac_hs: %lu MHz\n",
        ak_get_asic_pll_clk(AK_VA_SYSCTRL) / MHz,
        ak_get_vclk(AK_VA_SYSCTRL) / MHz,
        ak_get_audio_pll_freq(AK_VA_SYSCTRL) / MHz,
        sdadc_get_clk(NULL, ak_get_audio_pll_freq(AK_VA_SYSCTRL)),
        sddac_get_clk(NULL, ak_get_audio_pll_freq(AK_VA_SYSCTRL)),
        sdadc_hs_get_clk(NULL, ak_get_audio_pll_freq(AK_VA_SYSCTRL)) / MHz,
        sddac_hs_get_clk(NULL, ak_get_audio_pll_freq(AK_VA_SYSCTRL)) / MHz);

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
