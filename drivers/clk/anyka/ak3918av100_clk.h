#ifndef __AK3918AV100_CLK_H__
#define __AK3918AV100_CLK_H__

#define CLOCK_PLL1_CTRL                  (0x04)
#define CLOCK_PLL2_CTRL                  (0x08)
#define CLOCK_SDADDA_SPI0HS_SARADDA_CTRL (0x0C)
#define CLOCK_SD_ADC_DAC_HS_CTRL         (0x10)
#define CLOCK_PLL3_CTRL                  (0x14)
#define CLOCK_CSI_SCLK_MAC_OPCLK_CTRL    (0x18)
#define CLOCK_GATE_CTRL                  (0x1C)
#define CLOCK_SOFT_RESET                 (0x20)
#if defined(CONFIG_MACH_KM01A)
#define CLOCK_CSI_CTRL                   (0x24)
#define CLOCK_PDM_CTRL                   (0x28)
#elif defined(CONFIG_MACH_AK3918AV130)
#define CLOCK_CSI_CTRL                   (0x24)
#define CLOCK_PLL0_BANDWIDTH_ADJUST      (0xDC)
#endif
#define CLOCK_USB_I2S_CTRL               (0x58)
#define CLOCK_DAC_FADEOUT_CTRL           (0x70)
#define CLOCK_ISP_NPU_ENC_CTRL           (0x11c)

#define CLOCK_GCLK_VLD_MASK  (0x1)
#define CLOCK_GCLK_VLD_SHIFT (25)

#define CLOCK_GCLK_DIV_NUM_CFG_MASK  (0x3)
#define CLOCK_GCLK_DIV_NUM_CFG_SHIFT (23)
/*
 * CLOCK_SOFT_RESET
 */
#define CLOCK_SPI0HS_RST_CFG_SHIFT    (31)
#define CLOCK_SARADC_RST_CFG_SHIFT    (30)
#define CLOCK_SDADC_HSRST_CFG_SHIFT   (29)
#define CLOCK_SDDAC_HSRST_CFG_SHIFT   (28)
#define CLOCK_SDADC_RST_CFG_SHIFT     (27)
#define CLOCK_SDDAC_RST_CFG_SHIFT     (26)
#define CLOCK_DDRPHYCLK_RST_CFG_SHIFT (25)
#define CLOCK_DRAMCTRL_RST_CFG_SHIFT  (24)
#define GCLK_TWI2_RST_CFG_SHIFT       (22)
#define GCLK_MMC2_RST_CFG_SHIFT       (21)
#define GCLK_SPI2_RST_CFG_SHIFT       (20)
#define GCLK_MIPI1_RST_CFG_SHIFT      (19)
#define GCLK_UART2_RST_CFG_SHIFT      (18)
#define GCLK_TWI1_RST_CFG_SHIFT       (17)
#define GCLK_MIPI0_RST_CFG_SHIFT      (16)
#define GCLK_USB_RST_CFG_SHIFT        (15)
#define GCLK_ENCYPTO_CFG_SHIFT        (14)
#define GCLK_MAC_BUFFER_RST_CFG_SHIFT (13)
#define GCLK_GPIO_RST_CFG_SHIFT       (12)
#define GCLK_L2BUF1_RST_CFG_SHIFT     (11)
#define GCLK_TWI0_RST_CFG_SHIFT       (10)
#define GCLK_L2BUF0_RST_CFG_SHIFT     (9)
#define GCLK_UART1_RST_CFG_SHIFT      (8)
#define GCLK_UART0_RST_CFG_SHIFT      (7)
#define GCLK_SPI1_RST_CFG_SHIFT       (6)
#define GCLK_SPI0_RST_CFG_SHIFT       (5)
#define GCLK_SDDAC_RST_CFG_SHIFT      (4)
#define GCLK_SDADC_RST_CFG_SHIFT      (3)
#define GCLK_MMC1_RST_CFG_SHIFT       (2)
#define GCLK_MMC0_RST_CFG_SHIFT       (1)
#define GCLK_RST_CFG_SHIFT            (0)

/*
 * PLL1
 */
#define PLL1_CPU_1X_2X_SEL_CFG (1UL << 31)

#define PLL1_FRQ_ADJ_VLD_CFG_MASK  (0x01)
#define PLL1_FRQ_ADJ_VLD_CFG_SHIFT (30)
#define PLL2_FRQ_ADJ_VLD_CFG_MASK  (0x01)
#define PLL2_FRQ_ADJ_VLD_CFG_SHIFT (28)
#define PLL3_FRQ_ADJ_VLD_CFG_MASK  (0x01)
#define PLL3_FRQ_ADJ_VLD_CFG_SHIFT (29)

/*
 * CLOCK_SDADDA_SPI0HS_SARADDA_CTRL
 */
#define CLOCK_SPI0_HSDIV_VLD_CFG_MASK      (0x01)
#define CLOCK_SPI0_HSDIV_VLD_CFG_SHIFT     (19)
#define CLOCK_SPI0_HSCLK_EN_CFG_MASK       (0x01)
#define CLOCK_SPI0_HSCLK_EN_CFG_SHIFT      (18)
#define CLOCK_SPI0_HSCLK_DIV_NUM_CFG_MASK  (0xFF)
#define CLOCK_SPI0_HSCLK_DIV_NUM_CFG_SHIFT (10)

/*
 * CLOCK_SDADDA_SPI0HS_SARADDA_CTRL:
 * SD DAC
 */
#define CLOCK_SDDAC_DIV_VLD_CFG_SHIFT     (29)
#define CLOCK_SDDAC_CLK_EN_CFG_MASK       (0x01)
#define CLOCK_SDDAC_CLK_EN_CFG_SHIFT      (28)
#define CLOCK_SDDAC_CLK_DIV_NUM_CFG_MASK  (0xFF)
#define CLOCK_SDDAC_CLK_DIV_NUM_CFG_SHIFT (20)

#define CLOCK_SFC_PHY_CLK_EN_CFG_MASK       (0x01)
#define CLOCK_SFC_PHY_CLK_EN_CFG_SHIFT      (18)
#define CLOCK_SFC_PHY_CLK_DIV_NUM_CFG_MASK  (0xFF)
#define CLOCK_SFC_PHY_CLK_DIV_NUM_CFG_SHIFT (10)

/*
 * CLOCK_SDADDA_SPI0HS_SARADDA_CTRL:
 * DPHY_CFG
 */
#define CLOCK_DPHY_CFG_CLK_VLD_CFG_SHIFT     (9)
#define CLOCK_DPHY_CFG_CLK_EN_SHIFT          (8)
#define CLOCK_DPHY_CFG_CLK_EN_CFG_MASK       (0x01)
#define CLOCK_DPHY_CFG_CLK_DIV_NUM_CFG_MASK  (0xF)
#define CLOCK_DPHY_CFG_CLK_DIV_NUM_CFG_SHIFT (4)

/*
 * CLOCK_SDADDA_SPI0HS_SARADDA_CTRL:
 * SAR ADC
 */
#define CLOCK_SAR_ADC_CLK_EN_SHIFT          (3)
#define CLOCK_SAR_ADC_CLK_DIV_NUM_CFG_MASK  (0x7)
#define CLOCK_SAR_ADC_CLK_DIV_NUM_CFG_SHIFT (0)

/*
 * CLOCK_SD_ADC_DAC_HS_CTRL:
 * SD ADC HS
 * SD DAC HS
 * SD ADC
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

#define CLOCK_SDADC_DIV_VLD_CFG_MASK      (0x01)
#define CLOCK_SDADC_DIV_VLD_CFG_SHIFT     (9)
#define CLOCK_SDADC_CLK_EN_CFG_MASK       (0x01)
#define CLOCK_SDADC_CLK_EN_CFG_SHIFT      (8)
#define CLOCK_SDADC_CLK_DIV_NUM_CFG_MASK  (0xFF)
#define CLOCK_SDADC_CLK_DIV_NUM_CFG_SHIFT (0)

/*
 * CLOCK_CSI_SCLK_MAC_OPCLK_CTRL
 */
#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A)
#define CLOCK_CSI1_SCLK_EN_CFG_MASK       (0x01)
#define CLOCK_CSI1_SCLK_EN_CFG_SHIFT      (29)
#define CLOCK_CSI1_SCLK_MUX_NUM_CFG_MASK  (0x7)
#define CLOCK_CSI1_SCLK_MUX_NUM_CFG_SHIFT (26)
#define CLOCK_CSI1_SCLK_DIV_NUM_CFG_MASK  (0x3F)
#define CLOCK_CSI1_SCLK_DIV_NUM_CFG_SHIFT (20)
#endif

#define CLOCK_CSI0_SCLK_EN_CFG_MASK       (0x01)
#define CLOCK_CSI0_SCLK_EN_CFG_SHIFT      (19)
#define CLOCK_CSI0_SCLK_MUX_NUM_CFG_MASK  (0x7)
#define CLOCK_CSI0_SCLK_MUX_NUM_CFG_SHIFT (16)
#define CLOCK_CSI0_SCLK_DIV_NUM_CFG_MASK  (0x3F)
#define CLOCK_CSI0_SCLK_DIV_NUM_CFG_SHIFT (10)

#if !defined(CONFIG_MACH_KM01A) && !defined(CONFIG_MACH_AK3918AV130)
#define CLOCK_MAC_OPCLK_DIV_VLD_CFG_MASK  (0x01)
#define CLOCK_MAC_OPCLK_DIV_VLD_CFG_SHIFT (9)
#else
#define CLOCK_MAC_OPCLK_SOURCE_READY_MASK       (0x01)
#define CLOCK_MAC_OPCLK_SOURCE_READY_SHIFT      (31)
#define CLOCK_MAC_OPCLK_READ_SHIFT              (30)
#define CLOCK_MAC_OPCLK_READ_MASK               (0x01)
#define CLOCK_MAC_OPCLK_SOURCE_EN_CFG_MASK      (0x01)
#define CLOCK_MAC_OPCLK_SOURCE_EN_CFG_SHIFT     (9)
#define CLOCK_MAC_OPCLK_PLL_SEL_MASK            (0x3)
#define CLOCK_MAC_OPCLK_PLL_SEL_SHIFT           (6)
#endif
#define CLOCK_MAC_OPCLK_EN_CFG_MASK       (0x01)
#define CLOCK_MAC_OPCLK_EN_CFG_SHIFT      (8)
#define CLOCK_MAC_DIV_NUM_CFG_MASK        (0x3F)
#define CLOCK_MAC_DIV_NUM_CFG_SHIFT       (0)

/*
 * CLOCK SOURCE SELECTION
 */
#define CLOCK_CSI_SRC_SEL_PLL1   (0)
#define CLOCK_CSI_SRC_SEL_PLL2   (1)
#define CLOCK_CSI_SRC_SEL_PLL3   (2)
#define CLOCK_CSI_SRC_SEL_PLL4   (3)
#define CLOCK_CSI_SRC_SEL_OSC24M (4)

#define CLOCK_ISP_NPU_ENC_SRC_SEL_PLL1_DIVIDER (0)
#define CLOCK_ISP_NPU_ENC_SRC_SEL_PLL2_DIVIDER (1)
#define CLOCK_ISP_NPU_ENC_SRC_SEL_PLL3_DIVIDER (2)
#define CLOCK_ISP_NPU_ENC_SRC_SEL_PLL1         (4)
#define CLOCK_ISP_NPU_ENC_SRC_SEL_PLL2         (5)
#define CLOCK_ISP_NPU_ENC_SRC_SEL_PLL3         (6)

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
 * CLOCK_ISP_NPU_ENC_CTRL
 */

#define CLOCK_ISP_NPU_ENC_EN_CFG_MASK      (0x01)
#define CLOCK_ISP_NPU_ENC_MUX_NUM_CFG_MASK (0x7)
#define CLOCK_ISP_NPU_ENC_DIV_NUM_CFG_MASK (0x7)

#define CLOCK_ISP_CLK_RST_CFG_SHIFT     (7)
#define CLOCK_ISP_CLK_EN_CFG_SHIFT      (6)
#define CLOCK_ISP_CLK_MUX_NUM_CFG_SHIFT (3)
#define CLOCK_ISP_CLK_DIV_NUM_CFG_SHIFT (0)

#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A)
#define CLOCK_NPU_CLK_RST_CFG_SHIFT     (15)
#define CLOCK_NPU_CLK_EN_CFG_SHIFT      (14)
#define CLOCK_NPU_CLK_MUX_NUM_CFG_SHIFT (11)
#define CLOCK_NPU_CLK_DIV_NUM_CFG_SHIFT (8)
#endif

#define CLOCK_ENC_CLK_RST_CFG_SHIFT     (23)
#define CLOCK_ENC_CLK_EN_CFG_SHIFT      (22)
#define CLOCK_ENC_CLK_MUX_NUM_CFG_SHIFT (19)
#define CLOCK_ENC_CLK_DIV_NUM_CFG_SHIFT (16)


#define CLOCK_PP_CLK_RST_CFG_SHIFT     (31)
#define CLOCK_PP_CLK_EN_CFG_SHIFT      (30)
#define CLOCK_PP_CLK_MUX_NUM_CFG_SHIFT (27)
#define CLOCK_PP_CLK_DIV_NUM_CFG_SHIFT (24)


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
 * CLOCK_PLL3_CTRL
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

#if defined(CONFIG_MACH_KM01A)
#define CLOCK_PDM_MSCLK_EN_CFG_SHIFT         (19)
#define CLOCK_PDM_MSCLK_EN_CFG_MASK          (0x1)
#define CLOCK_PDM_MSCLK_DIV_VLD_CFG_SHIFT    (20)
#define CLOCK_PDM_MCLK_RESET_SHIFT           (21)
#define CLOCK_PDM_MSCLK_DIV_NUM_CFG_MASK     (0xFF)
#define CLOCK_PDM_MSCLK_DIV_NUM_CFG_SHIFT    (11)
#define CLOCK_PDM_HSCLK_EN_CFG_SHIFT         (8)
#define CLOCK_PDM_HSCLK_EN_CFG_MASK          (0x1)
#define CLOCK_PDM_HSCLK_DIV_VLD_CFG_SHIFT    (9)
#define CLOCK_PDM_HCLK_RESET_SHIFT           (10)
#define CLOCK_PDM_HSCLK_DIV_NUM_CFG_MASK     (0xFF)
#define CLOCK_PDM_HSCLK_DIV_NUM_CFG_SHIFT    (0)
#endif

#define MODULE_RESET_RELEASE (0)
#define MODULE_RESET_HOLD    (1)

#define CLOCK_FREQ_ADJ_WAITING_MAX_NUM (100000)

#define MHz 1000000UL
#define KHz 1000UL

#define TYPE_SDADC (1)
#define TYPE_SDDAC (0)

#define PLL1 (1)
#define PLL2 (2)
#define PLL3 (3)
struct ak_fixed_clk {
    int id;
    struct clk_hw hw;
    void __iomem* reg;
    unsigned long fixed_rate;
};

#define to_clk_ak_fixed(_hw) container_of(_hw, struct ak_fixed_clk, hw)

struct ak_gate_clk {
    int id;
    struct clk_hw hw;
    void __iomem* reg;
    int ctrlbit;
};

#define to_clk_ak_gate(_hw) container_of(_hw, struct ak_gate_clk, hw)

struct ak_factor_clk {
    int id;
    struct clk_hw hw;
    void __iomem* reg;
    int m;
    int n;
};

#define to_clk_ak_factor(_hw) container_of(_hw, struct ak_factor_clk, hw)
/*ak_get_pll2_clk*/
extern unsigned long ak_get_pll2_clk(void __iomem* reg);
/*ak_get_pll3_clk*/
extern unsigned long ak_get_pll3_clk(void __iomem* reg);
/*ak_module_reset_by_clock*/
extern void ak_module_reset_by_clock(void __iomem* res_reg, u32 rst_cfg_shift);
/*sdadc_get_clk*/
extern unsigned long sdadc_get_clk(
    struct ak_factor_clk* factor_clk, unsigned long parent_rate);
/*sddac_get_clk*/
extern unsigned long sddac_get_clk(
    struct ak_factor_clk* factor_clk, unsigned long parent_rate);
/*ak_clock_divider_adjust_complete*/
extern int ak_clock_divider_adjust_complete(
    void __iomem* res_reg, u32 shift, u32 mask);
/*ak_get_gclk*/
extern unsigned long ak_get_gclk(void __iomem* reg);

/*sfc_phy_get_clk*/
extern unsigned long sfc_phy_get_clk(struct ak_factor_clk* factor_clk, unsigned long parent_rate);

/*gclk_get_clk*/
extern unsigned long gclk_get_clk(struct ak_factor_clk* factor_clk, unsigned long parent_rate);

/*ess_vclk_get_clk*/
extern unsigned long ess_vclk_get_clk(struct ak_factor_clk* factor_clk,
    unsigned long parent_rate_pll1, unsigned long parent_rate_pll2,
    unsigned long parent_rate_pll3);

/*nss_vclk_get_clk*/
extern unsigned long nss_vclk_get_clk(struct ak_factor_clk* factor_clk,
    unsigned long parent_rate_pll1, unsigned long parent_rate_pll2,
    unsigned long parent_rate_pll3);

/*iss_vclk_get_clk*/
extern unsigned long iss_vclk_get_clk(struct ak_factor_clk* factor_clk,
    unsigned long parent_rate_pll1, unsigned long parent_rate_pll2,
    unsigned long parent_rate_pll3);

#endif
