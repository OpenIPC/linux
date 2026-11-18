// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2016 Socionext Inc.
 *   Author: Masahiro Yamada <yamada.masahiro@socionext.com>
 */

#include <linux/bitfield.h>
#include <linux/bits.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/reset.h>
#include <linux/pm_runtime.h>
#include "sdhci-pltfm.h"
#include <linux/gpio.h>
#include <linux/of_gpio.h>

/* #define SUPPORT_CLK_AUTOGATE */
// #define USING_CLK_FRAME

#define DEASSERT 0
#define ASSERT 1

/* HRS - Host Register Set (specific to Cadence) */
#define SDHCI_CDNS_HRS04                    0x10		/* PHY access port */
#define SDHCI_CDNS_HRS04_ACK                BIT(26)
#define SDHCI_CDNS_HRS04_RD                 BIT(25)
#define SDHCI_CDNS_HRS04_WR                 BIT(24)
#define SDHCI_CDNS_HRS04_RDATA              GENMASK(23, 16)
#define SDHCI_CDNS_HRS04_WDATA              GENMASK(15, 8)
#define SDHCI_CDNS_HRS04_ADDR               GENMASK(5, 0)

#define SDHCI_CDNS_HRS06                    0x18		/* eMMC control */
#define SDHCI_CDNS_HRS06_TUNE_UP            BIT(15)
#define SDHCI_CDNS_HRS06_TUNE               GENMASK(13, 8)
#define SDHCI_CDNS_HRS06_MODE               GENMASK(2, 0)
#define SDHCI_CDNS_HRS06_MODE_SD            0x0
#define SDHCI_CDNS_HRS06_MODE_MMC_LEGACY    0x1
#define SDHCI_CDNS_HRS06_MODE_MMC_SDR       0x2
#define SDHCI_CDNS_HRS06_MODE_MMC_DDR       0x3
#define SDHCI_CDNS_HRS06_MODE_MMC_HS200     0x4
#define SDHCI_CDNS_HRS06_MODE_MMC_HS400     0x5
#define SDHCI_CDNS_HRS06_MODE_MMC_HS400ES   0x6

/* SRS - Slot Register Set (SDHCI-compatible) */
#define SDHCI_CDNS_SRS_BASE		0x200

/* PHY */
#define SDHCI_CDNS_PHY_DLY_SD_HS        0x00
#define SDHCI_CDNS_PHY_DLY_SD_DEFAULT	0x01
#define SDHCI_CDNS_PHY_DLY_UHS_SDR12	0x02
#define SDHCI_CDNS_PHY_DLY_UHS_SDR25	0x03
#define SDHCI_CDNS_PHY_DLY_UHS_SDR50	0x04
#define SDHCI_CDNS_PHY_DLY_UHS_DDR50	0x05
#define SDHCI_CDNS_PHY_DLY_EMMC_LEGACY	0x06
#define SDHCI_CDNS_PHY_DLY_EMMC_SDR     0x07
#define SDHCI_CDNS_PHY_DLY_EMMC_DDR     0x08
#define SDHCI_CDNS_PHY_LOCK_VALUE       0x09
#define SDHCI_CDNS_PHY_DLY_SDCLK        0x0b
#define SDHCI_CDNS_PHY_DLY_HSMMC        0x0c
#define SDHCI_CDNS_PHY_DLY_STROBE       0x0d
#define SDHCI_CDNS_PHY_DLL_RESET        0x0f
/*
 * The tuned val register is 6 bit-wide, but not the whole of the range is
 * available.  The range 0-42 seems to be available (then 43 wraps around to 0)
 * but I am not quite sure if it is official.  Use only 0 to 39 for safety.
 */
#define SDHCI_CDNS_MAX_TUNING_LOOP	40

#define SDHCI_AXERA_200M_CLK (200000000UL)

#define EMMC_BOOT_4BIT_25M_768K 0x6
#define EMMC_BOOT_4BIT_25M_128K 0x4
#define TOP_CHIPMODE_GLB        0x2390000
#define TOP_CHIPMODE_GLB_SW     (TOP_CHIPMODE_GLB + 0xC)
/* chip_mode[3:1] */
#define FLASH_BOOT_MASK         (0x7 << 1)

#define PIN_MUX_G9_BASE			0x104F1000
#define PIN_MUX_G9_VDET_RO0		(PIN_MUX_G9_BASE + 0x58)	//bit0
#define PIN_MUX_G9_PINCTRL_SET	        (PIN_MUX_G9_BASE + 0x4)
#define PIN_MUX_G9_PINCTRL_CLR	        (PIN_MUX_G9_BASE + 0x8)

#define PIN_MUX_G12_BASE			0x104F2000
#define PIN_MUX_G12_VDET_RO0		(PIN_MUX_G12_BASE + 0x58)	//bit0
#define PIN_MUX_G12_PINCTRL_SET	        (PIN_MUX_G12_BASE + 0x4)
#define PIN_MUX_G12_PINCTRL_CLR	        (PIN_MUX_G12_BASE + 0x8)

#define FLASH_SYS_GLB_BASE  0x10030000
#define CLK_EB_1  (FLASH_SYS_GLB_BASE + 0x8)

#ifndef USING_CLK_FRAME
#define FLASH_SYS_GLB_BASE                  0x10030000
#define FLASH_SYS_GLB_CLK_MUX0_SET          0x4000
#define FLASH_SYS_GLB_CLK_MUX0_CLR          0x8000
#define FLASH_SYS_GLB_CLK_EB0_SET           0x4004
#define FLASH_SYS_GLB_CLK_EB0_CLR           0x8004
#define FLASH_SYS_GLB_CLK_EB1_SET           0x4008
#define FLASH_SYS_GLB_CLK_EB1_CLR           0x8008
#define FLASH_SYS_GLB_CLK_DIV0_SET          0x400C
#define FLASH_SYS_GLB_CLK_DIV0_CLR          0x800C
#define FLASH_SYS_GLB_CLK_DIV1_SET          0x4010
#define FLASH_SYS_GLB_CLK_DIV1_CLR          0x8010
#define FLASH_SYS_GLB_SW_RST0_SET           0x4014
#define FLASH_SYS_GLB_SW_RST0_CLR           0x8014

#define CPU_SYS_GLB               0x1900000
#define CPU_SYS_GLB_CLK_MUX0_SET  0x1000
#define CPU_SYS_GLB_CLK_MUX0_CLR  0x2000
#define CPU_SYS_GLB_CLK_EB0_SET   0x1004
#define CPU_SYS_GLB_CLK_EB0_CLR   0x2004
#define CPU_SYS_GLB_CLK_DIV0_SET  0x100C
#define CPU_SYS_GLB_CLK_DIV0_CLR  0x200C
#define CPU_SYS_GLB_SW_RST0_SET   0x1010
#define CPU_SYS_GLB_SW_RST0_CLR   0x2010

#define CLK_EMMC_CLK_CARD_SEL(x)    (((x) & 0x3) << 5)
#define CLK_EMMC_CLK_CARD_DIV(x)    (((x) & 0x3F) << 0)
#define EMMC_CLK_DIV_UPDATE         (0x1 << 6)

#define CLK_SD_CLK_CARD_SEL(x)    (((x) & 0x3) << 16)
#define CLK_SD_CLK_CARD_DIV(x)    (((x) & 0x3F) << 20)
#define CLK_SDIO_CLK_CARD_SEL(x)    (((x) & 0x3) << 18)
#define CLK_SDIO_CLK_CARD_DIV(x)    (((x) & 0x3F) << 0)
#define SD_CLK_DIV_UPDATE         (0x1 << 26)
#define SDIO_CLK_DIV_UPDATE         (0x1 << 6)
#define PCLK_SDIO_EB_SET          (0x1 << 18)
#define PCLK_SD_EB_SET          (0x1 << 17)
#define ACLK_SDIO_EB_SET          (0x1 << 4)
#define ACLK_SD_EB_SET          (0x1 << 3)
#endif

#define EMMC_BASE_ADDR  0x01b40000
#define SD_BASE_ADDR    0x104e0000
#define SDIO_BASE_ADDR  0x104d0000

extern bool __clk_is_enabled(struct clk *clk);
extern unsigned int __clk_get_enable_count(struct clk *clk);

struct sdhci_cdns_phy_param {
	u8 addr;
	u8 data;
};

struct sdhci_axera_priv {
	struct platform_device *pdev;
	struct sdhci_host *host;
	void __iomem *hrs_addr;
	bool enhanced_strobe;
	unsigned int nr_phy_params;
	struct clk *aclk;
	struct clk *pclk;
	struct clk *cardclk;
	struct reset_control *arst;
	struct reset_control *prst;
	struct reset_control *cardrst;
	int hw_reset_gpio;
	int sdio_voltage_sw;
	unsigned long io_addr;
	struct sdhci_cdns_phy_param phy_params[];
};

struct sdhci_cdns_phy_cfg {
	const char *property;
	u8 addr;
};

static const struct sdhci_cdns_phy_cfg sdhci_cdns_phy_cfgs[] = {
	{ "cdns,phy-input-delay-sd-highspeed", SDHCI_CDNS_PHY_DLY_SD_HS, },
	{ "cdns,phy-input-delay-legacy", SDHCI_CDNS_PHY_DLY_SD_DEFAULT, },
	{ "cdns,phy-input-delay-sd-uhs-sdr12", SDHCI_CDNS_PHY_DLY_UHS_SDR12, },
	{ "cdns,phy-input-delay-sd-uhs-sdr25", SDHCI_CDNS_PHY_DLY_UHS_SDR25, },
	{ "cdns,phy-input-delay-sd-uhs-sdr50", SDHCI_CDNS_PHY_DLY_UHS_SDR50, },
	{ "cdns,phy-input-delay-sd-uhs-ddr50", SDHCI_CDNS_PHY_DLY_UHS_DDR50, },
	{ "cdns,phy-input-delay-mmc-highspeed", SDHCI_CDNS_PHY_DLY_EMMC_SDR, },
	{ "cdns,phy-input-delay-mmc-ddr", SDHCI_CDNS_PHY_DLY_EMMC_DDR, },
	{ "cdns,phy-dll-delay-sdclk", SDHCI_CDNS_PHY_DLY_SDCLK, },
	{ "cdns,phy-dll-delay-sdclk-hsmmc", SDHCI_CDNS_PHY_DLY_HSMMC, },
	{ "cdns,phy-dll-delay-strobe", SDHCI_CDNS_PHY_DLY_STROBE, },
};

static int sdhci_cdns_write_phy_reg(struct sdhci_axera_priv *priv,
				    u8 addr, u8 data)
{
	void __iomem *reg = priv->hrs_addr + SDHCI_CDNS_HRS04;
	u32 tmp;
	int ret;

	ret = readl_poll_timeout(reg, tmp, !(tmp & SDHCI_CDNS_HRS04_ACK),
				 0, 10);
	if (ret)
		return ret;

	tmp = FIELD_PREP(SDHCI_CDNS_HRS04_WDATA, data) |
	      FIELD_PREP(SDHCI_CDNS_HRS04_ADDR, addr);
	writel(tmp, reg);

	tmp |= SDHCI_CDNS_HRS04_WR;
	writel(tmp, reg);

	ret = readl_poll_timeout(reg, tmp, tmp & SDHCI_CDNS_HRS04_ACK, 0, 10);
	if (ret)
		return ret;

	tmp &= ~SDHCI_CDNS_HRS04_WR;
	writel(tmp, reg);

	ret = readl_poll_timeout(reg, tmp, !(tmp & SDHCI_CDNS_HRS04_ACK),
				 0, 10);

	return ret;
}

static int sdhci_cdns_read_phy_reg(struct sdhci_axera_priv *priv,
				    u32 addr)
{
	void __iomem *reg = priv->hrs_addr + SDHCI_CDNS_HRS04;
	u32 tmp;
	int ret;

	tmp = readl(reg);
	tmp = tmp & 0xffffff00;
	tmp = tmp | addr;
	/* set address */
	writel(tmp, reg);

	tmp |= SDHCI_CDNS_HRS04_RD;
	/* send read request */
	writel(tmp, reg);

	ret = readl_poll_timeout(reg, tmp, tmp & SDHCI_CDNS_HRS04_ACK, 0, 10);
	if (ret)
		return ret;

	tmp &= ~SDHCI_CDNS_HRS04_RD;
	/* clear read request */
	writel(0, reg);
	tmp = tmp >> 16;
	/* Lock or not. */
	if((addr == 0x09) && (tmp & 0x80)){
		pr_info("%s: phy lock successful\n",__func__);
	}else{
		pr_info("%s: phy lock failed\n",__func__);
	}
	return tmp;
}
static unsigned int sdhci_cdns_phy_param_count(struct device_node *np)
{
	unsigned int count = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(sdhci_cdns_phy_cfgs); i++)
		if (of_property_read_bool(np, sdhci_cdns_phy_cfgs[i].property))
			count++;

	return count;
}

static void sdhci_cdns_phy_param_parse(struct device_node *np,
				       struct sdhci_axera_priv *priv)
{
	struct sdhci_cdns_phy_param *p = priv->phy_params;
	u32 val;
	int ret, i;

	for (i = 0; i < ARRAY_SIZE(sdhci_cdns_phy_cfgs); i++) {
		ret = of_property_read_u32(np, sdhci_cdns_phy_cfgs[i].property,
					   &val);
		if (ret)
			continue;

		p->addr = sdhci_cdns_phy_cfgs[i].addr;
		p->data = val;
		p++;
	}
}

static int sdhci_cdns_phy_init(struct sdhci_axera_priv *priv)
{
	int ret, i;

	/* when low the dll does into start point of locking mechanism.
	*  After de-assertion,the master DLL begins searching for lock.
	*  It is recommended to assert this signal low when changing the sdmclk clock frequency.
	*/
	sdhci_cdns_write_phy_reg(priv,SDHCI_CDNS_PHY_DLL_RESET,0);
	sdhci_cdns_write_phy_reg(priv,SDHCI_CDNS_PHY_DLL_RESET,1);
	for (i = 0; i < priv->nr_phy_params; i++) {
		ret = sdhci_cdns_write_phy_reg(priv, priv->phy_params[i].addr,
					       priv->phy_params[i].data);
		if (ret)
			return ret;
	}

	return 0;
}

static void *sdhci_axera_priv(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);

	return sdhci_pltfm_priv(pltfm_host);
}

static unsigned int sdhci_cdns_get_timeout_clock(struct sdhci_host *host)
{
	/*
	 * Cadence's spec says the Timeout Clock Frequency is the same as the
	 * Base Clock Frequency.
	 */
	return host->max_clk;
}

static void sdhci_cdns_set_emmc_mode(struct sdhci_axera_priv *priv, u32 mode)
{
	u32 tmp;

	/* The speed mode for eMMC is selected by HRS06 register */
	tmp = readl(priv->hrs_addr + SDHCI_CDNS_HRS06);
	tmp &= ~SDHCI_CDNS_HRS06_MODE;
	tmp |= FIELD_PREP(SDHCI_CDNS_HRS06_MODE, mode);
	writel(tmp, priv->hrs_addr + SDHCI_CDNS_HRS06);
}

static u32 sdhci_cdns_get_emmc_mode(struct sdhci_axera_priv *priv)
{
	u32 tmp;

	tmp = readl(priv->hrs_addr + SDHCI_CDNS_HRS06);
	return FIELD_GET(SDHCI_CDNS_HRS06_MODE, tmp);
}

static int sdhci_cdns_set_tune_val(struct sdhci_host *host, unsigned int val)
{
	struct sdhci_axera_priv *priv = sdhci_axera_priv(host);
	void __iomem *reg = priv->hrs_addr + SDHCI_CDNS_HRS06;
	u32 tmp;
	int i, ret;

	if (WARN_ON(!FIELD_FIT(SDHCI_CDNS_HRS06_TUNE, val)))
		return -EINVAL;

	tmp = readl(reg);
	tmp &= ~SDHCI_CDNS_HRS06_TUNE;
	tmp |= FIELD_PREP(SDHCI_CDNS_HRS06_TUNE, val);

	/*
	 * Workaround for IP errata:
	 * The IP6116 SD/eMMC PHY design has a timing issue on receive data
	 * path. Send tune request twice.
	 */
	for (i = 0; i < 2; i++) {
		tmp |= SDHCI_CDNS_HRS06_TUNE_UP;
		writel(tmp, reg);

		ret = readl_poll_timeout(reg, tmp,
					 !(tmp & SDHCI_CDNS_HRS06_TUNE_UP),
					 0, 1);
		if (ret)
			return ret;
	}

	return 0;
}

/*
 * In SD mode, software must not use the hardware tuning and instead perform
 * an almost identical procedure to eMMC.
 */
static int sdhci_cdns_execute_tuning(struct sdhci_host *host, u32 opcode)
{
	int cur_streak = 0;
	int max_streak = 0;
	int end_of_streak = 0;
	int i;

	/*
	 * Do not execute tuning for UHS_SDR50 or UHS_DDR50.
	 * The delay is set by probe, based on the DT properties.
	 */
	if (host->timing != MMC_TIMING_MMC_HS200 &&
	    host->timing != MMC_TIMING_UHS_SDR104)
		return 0;

	for (i = 0; i < SDHCI_CDNS_MAX_TUNING_LOOP; i++) {
		if (sdhci_cdns_set_tune_val(host, i) ||
		    mmc_send_tuning(host->mmc, opcode, NULL)) { /* bad */
			cur_streak = 0;
		} else { /* good */
			cur_streak++;
			if (cur_streak > max_streak) {
				max_streak = cur_streak;
				end_of_streak = i;
			}
		}
	}

	if (!max_streak) {
		dev_err(mmc_dev(host->mmc), "no tuning point found\n");
		return -EIO;
	}

	dev_dbg(mmc_dev(host->mmc), "tuning success, tuning point found: %d\n", end_of_streak - max_streak / 2);
	return sdhci_cdns_set_tune_val(host, end_of_streak - max_streak / 2);
}


static void sdhci_cdns_set_uhs_signaling(struct sdhci_host *host,
					 unsigned int timing)
{
	struct sdhci_axera_priv *priv = sdhci_axera_priv(host);
	u32 mode;
	if ((host->mmc->caps2 & MMC_CAP2_NO_SDIO) &&
                  (host->mmc->caps2 & MMC_CAP2_NO_SD)) {
		switch (timing) {
		case MMC_TIMING_MMC_HS:
			mode = SDHCI_CDNS_HRS06_MODE_MMC_SDR;
			break;
		case MMC_TIMING_MMC_DDR52:
			mode = SDHCI_CDNS_HRS06_MODE_MMC_DDR;
			break;
		case MMC_TIMING_MMC_HS200:
			mode = SDHCI_CDNS_HRS06_MODE_MMC_HS200;
			break;
		case MMC_TIMING_MMC_HS400:
			if (priv->enhanced_strobe)
				mode = SDHCI_CDNS_HRS06_MODE_MMC_HS400ES;
			else
				mode = SDHCI_CDNS_HRS06_MODE_MMC_HS400;
			break;
		default:
			mode = SDHCI_CDNS_HRS06_MODE_MMC_LEGACY;
			break;
		}

		sdhci_cdns_set_emmc_mode(priv, mode);
	} else {
	/* For SD, fall back to the default handler */
		sdhci_set_uhs_signaling(host, timing);
	}
}

static inline int mmc_host_support_uhs(struct mmc_host *host)
{
	return host->caps &
		(MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
		 MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104 |
		 MMC_CAP_UHS_DDR50) &&
	       host->caps & MMC_CAP_4_BIT_DATA;
}

static void sdhci_axera_voltage_switch(struct sdhci_host *host)
{
	u32 val;
	void __iomem *addr = NULL;
	struct sdhci_axera_priv *priv = sdhci_axera_priv(host);

	if (!mmc_host_support_uhs(host->mmc))
		return;

	if ((host->mmc->caps2 & MMC_CAP2_NO_MMC) && (host->mmc->caps2 & MMC_CAP2_NO_SDIO)) {
		pr_debug("sd voltage switch\n");

		if(host->mmc->ios.signal_voltage == MMC_SIGNAL_VOLTAGE_330) {
			addr = ioremap(PIN_MUX_G9_PINCTRL_CLR, 0x4);
			writel(GENMASK(8, 7), addr);
			iounmap(addr);
			pr_debug("%s voltage switch to 3.3V\n", host->hw_name);
			return;
		}

		usleep_range(15000, 15500);

		//enbale pinmux for vdet
		addr = ioremap(CLK_EB_1, 0x4);
		val = readl(addr);
		writel(val | (1 << 16), addr);
		iounmap(addr);

		/* CMD & DATA pad switch to 1.8V */
		addr = ioremap(PIN_MUX_G9_VDET_RO0, 0x4);
		val = readl(addr);
		iounmap(addr);
		pr_debug("vdet val: %x\n", val);

		if (!(val & BIT(0))) { //1.8v
			addr = ioremap(PIN_MUX_G9_PINCTRL_SET, 0x4);
			writel(GENMASK(8, 7), addr);
			iounmap(addr);
			pr_info("%s voltage switch to 1.8V\n", host->hw_name);
		}
	}

	if ((host->mmc->caps2 & MMC_CAP2_NO_MMC) && (host->mmc->caps2 & MMC_CAP2_NO_SD)) {
		pr_debug("sdio voltage switch\n");

		if (!gpio_is_valid(priv->sdio_voltage_sw)) {
			pr_err("no sdio voltage gpio\n");
			return;
		}

		if (host->mmc->ios.signal_voltage == MMC_SIGNAL_VOLTAGE_330) {
			addr = ioremap(PIN_MUX_G12_PINCTRL_CLR, 0x4);
			writel(GENMASK(8, 7), addr);
			iounmap(addr);
			pr_debug("%s voltage switch to 3.3V\n", host->hw_name);

			gpio_direction_output(priv->sdio_voltage_sw, 0);
		} else {
			pr_info("sdio switch to 1.8v\n");
			gpio_direction_output(priv->sdio_voltage_sw, 1);
			usleep_range(15000, 15500);

			//enbale pinmux for vdet
			addr = ioremap(CLK_EB_1, 0x4);
			val = readl(addr);
			writel(val | (1 << 16), addr);
			iounmap(addr);

			/* CMD & DATA pad switch to 1.8V */
			addr = ioremap(PIN_MUX_G12_VDET_RO0, 0x4);
			val = readl(addr);
			iounmap(addr);
			pr_debug("vdet val: %x\n", val);

			if (!(val & BIT(0))) { //1.8v
				addr = ioremap(PIN_MUX_G12_PINCTRL_SET, 0x4);
				writel(GENMASK(8, 7), addr);
				iounmap(addr);
				pr_info("%s voltage switch to 1.8V\n", host->hw_name);
			}
		}
	}

	return;
}


static void get_mmc_hw_reset_gpio(struct sdhci_host *host, struct sdhci_axera_priv *priv, struct device *dev)
{
	int ret;
	struct device_node *pp = dev->of_node;
	struct mmc_host *mmc = host->mmc;

	if (mmc->caps & MMC_CAP_HW_RESET) {
		/* Get GPIO from device tree */
		priv->hw_reset_gpio = of_get_named_gpio(pp, "hw-reset", 0);
		if (priv->hw_reset_gpio < 0) {
			dev_err(dev,
				"Failed to get emmc hw-reset gpio from dts.\n");
		}
		/* GPIO request and configuration */
		ret = devm_gpio_request_one(dev, priv->hw_reset_gpio,
				GPIOF_OUT_INIT_HIGH, "eMMC HW RESET");
		if (ret) {
			dev_err(dev, "Failed to request hw-reset pin\n");
		}
	}

}

static void sdhci_axera_hw_reset(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host;
	struct sdhci_axera_priv *priv;
	struct mmc_host *mmc = host->mmc;

	if (mmc->caps & MMC_CAP_HW_RESET) {
		pltfm_host = sdhci_priv(host);
		priv = sdhci_pltfm_priv(pltfm_host);
		if (priv->hw_reset_gpio >= 0) {
			gpio_set_value(priv->hw_reset_gpio, 0);
			udelay(10);
			gpio_set_value(priv->hw_reset_gpio, 1);
		}
		sdhci_cdns_phy_init(priv);
	}
}

static const struct sdhci_ops sdhci_cdns_ops = {
	.set_clock = sdhci_set_clock,
	.get_timeout_clock = sdhci_cdns_get_timeout_clock,
	.set_bus_width = sdhci_set_bus_width,
	.reset = sdhci_reset,
	.hw_reset = sdhci_axera_hw_reset,
	.platform_execute_tuning = sdhci_cdns_execute_tuning,
	.set_uhs_signaling = sdhci_cdns_set_uhs_signaling,
	.voltage_switch = sdhci_axera_voltage_switch,
};

static const struct sdhci_pltfm_data sdhci_cdns_uniphier_pltfm_data = {
	.ops = &sdhci_cdns_ops,
	.quirks2 = SDHCI_QUIRK2_PRESET_VALUE_BROKEN,
};

static const struct sdhci_pltfm_data sdhci_cdns_pltfm_data = {
	.ops = &sdhci_cdns_ops,
};

static void sdhci_cdns_hs400_enhanced_strobe(struct mmc_host *mmc,
					     struct mmc_ios *ios)
{
	struct sdhci_host *host = mmc_priv(mmc);
	struct sdhci_axera_priv *priv = sdhci_axera_priv(host);
	u32 mode;

	priv->enhanced_strobe = ios->enhanced_strobe;

	mode = sdhci_cdns_get_emmc_mode(priv);

	if (mode == SDHCI_CDNS_HRS06_MODE_MMC_HS400 && ios->enhanced_strobe)
		sdhci_cdns_set_emmc_mode(priv,
					 SDHCI_CDNS_HRS06_MODE_MMC_HS400ES);

	if (mode == SDHCI_CDNS_HRS06_MODE_MMC_HS400ES && !ios->enhanced_strobe)
		sdhci_cdns_set_emmc_mode(priv,
					 SDHCI_CDNS_HRS06_MODE_MMC_HS400);
}

static int axera_get_clk_reset(struct platform_device *pdev, struct sdhci_axera_priv *priv)
{
	priv->arst = devm_reset_control_get_optional(&pdev->dev, "arst");
	if (IS_ERR(priv->arst)) {
		pr_err("couldn't get the priv->arst\n");
		return -1;
	}

	priv->prst = devm_reset_control_get_optional(&pdev->dev, "prst");
	if (IS_ERR(priv->prst)) {
		pr_err("couldn't get the priv->prst\n");
		return -1;
	}

	priv->cardrst = devm_reset_control_get_optional(&pdev->dev, "cardrst");
	if (IS_ERR(priv->cardrst)) {
		pr_err("couldn't get the priv->cardrst\n");
		return -1;
	}
	return 0;
}

static int axera_clk_reset_control(struct sdhci_axera_priv *priv, int flag)
{
	if (IS_ERR(priv->arst))
		return PTR_ERR(priv->arst);
	if (IS_ERR(priv->prst))
		return PTR_ERR(priv->prst);
	if (IS_ERR(priv->cardrst))
		return PTR_ERR(priv->cardrst);

	if (flag == DEASSERT) {
		reset_control_deassert(priv->arst);
		reset_control_deassert(priv->prst);
		reset_control_deassert(priv->cardrst);
	}
	if (flag == ASSERT) {
		reset_control_assert(priv->cardrst);
		reset_control_assert(priv->arst);
		reset_control_assert(priv->prst);
	}
	return 0;
}
#ifdef USING_CLK_FRAME
static int axera_get_clk(struct platform_device *pdev, struct sdhci_axera_priv *priv)
{
	priv->aclk = devm_clk_get(&pdev->dev, "aclk");
	if (IS_ERR(priv->aclk)) {
		pr_err("%s: priv->hrs_addr:%p, priv->aclk get fail\n", __func__, priv->hrs_addr);
		return -1;
	}
	priv->pclk = devm_clk_get(&pdev->dev, "pclk");
	if (IS_ERR(priv->pclk)) {
		pr_debug("%s: priv->hrs_addr:%p, priv->pclk get fail, eMMC ignores it\n", __func__, priv->hrs_addr);
	}

	priv->cardclk = devm_clk_get(&pdev->dev, "cardclk");
	if (IS_ERR(priv->cardclk)) {
		pr_err("%s: priv->hrs_addr:%p, priv->cardclk get fail\n", __func__, priv->hrs_addr);
		return -1;
	}

	return 0;
}
#endif

#ifndef USING_CLK_FRAME
void ax_set_mmc_clk(struct sdhci_host *host)
{
	void __iomem *addr = NULL;
	int clk_sel = 0, div = 0;
	struct sdhci_axera_priv *priv = sdhci_axera_priv(host);

	if (!(host->mmc->caps2 & MMC_CAP2_NO_MMC)) {
		pr_info("set emmc clk to 200M\n");
		clk_sel = 0x3; //source npll_400m
		div = 0x1; //200M supply card

		addr = ioremap(CPU_SYS_GLB, 0x2020);

		writel(BIT(2), addr + CPU_SYS_GLB_CLK_EB0_CLR);//close clk_emmc_card_eb
		udelay(2);
		writel(CLK_EMMC_CLK_CARD_SEL(0x3), addr + CPU_SYS_GLB_CLK_MUX0_CLR);//clr clk source bit[6:5]
		writel(CLK_EMMC_CLK_CARD_SEL(clk_sel), addr + CPU_SYS_GLB_CLK_MUX0_SET);//Select clk source bit[6:5]
		writel(BIT(2), addr + CPU_SYS_GLB_CLK_EB0_SET);//open clk_emmc_card_eb

		writel(GENMASK(5, 0), addr + CPU_SYS_GLB_CLK_DIV0_CLR);//clear bit[5:0]
		writel(CLK_EMMC_CLK_CARD_DIV(div), addr + CPU_SYS_GLB_CLK_DIV0_SET);//set div
		writel(EMMC_CLK_DIV_UPDATE, addr + CPU_SYS_GLB_CLK_DIV0_SET);
		udelay(2);
		writel(EMMC_CLK_DIV_UPDATE, addr + CPU_SYS_GLB_CLK_DIV0_CLR);

		iounmap(addr);
	}

	if (priv->io_addr == SD_BASE_ADDR) {
		pr_info("set sd clk to 200M\n");
		clk_sel = 0x3; //source npll_400m
		div = 0x1; //200M supply card

		addr = ioremap(FLASH_SYS_GLB_BASE, 0x8020);

		writel(BIT(9), addr + FLASH_SYS_GLB_CLK_EB0_CLR);//close clk_sd_card_eb
		udelay(2);
		writel(CLK_SD_CLK_CARD_SEL(0x3), addr + FLASH_SYS_GLB_CLK_MUX0_CLR);//clr clk source bit[17:16]
		writel(CLK_SD_CLK_CARD_SEL(clk_sel), addr + FLASH_SYS_GLB_CLK_MUX0_SET);//Select clk source bit[17:16]
		writel(BIT(9), addr + FLASH_SYS_GLB_CLK_EB0_SET);//open clk_sd_card_eb

		writel(GENMASK(25, 20), addr + FLASH_SYS_GLB_CLK_DIV0_CLR);//clear bit[25:20]
		writel(CLK_SD_CLK_CARD_DIV(div), addr + FLASH_SYS_GLB_CLK_DIV0_SET);//set div
		writel(SD_CLK_DIV_UPDATE, addr + FLASH_SYS_GLB_CLK_DIV0_SET);
		udelay(2);
		writel(SD_CLK_DIV_UPDATE, addr + FLASH_SYS_GLB_CLK_DIV0_CLR);

		iounmap(addr);
	}

	if (priv->io_addr == SDIO_BASE_ADDR) {
		pr_info("set sdio clk to 200M\n");
		clk_sel = 0x3; //source npll_400m
		div = 0x1; //200M supply card

		addr = ioremap(FLASH_SYS_GLB_BASE, 0x8020);

		writel(BIT(10), addr + FLASH_SYS_GLB_CLK_EB0_CLR);//close clk_sd_card_eb
		udelay(2);
		writel(CLK_SDIO_CLK_CARD_SEL(0x3), addr + FLASH_SYS_GLB_CLK_MUX0_CLR);//clr clk source bit[17:16]
		writel(CLK_SDIO_CLK_CARD_SEL(clk_sel), addr + FLASH_SYS_GLB_CLK_MUX0_SET);//Select clk source bit[17:16]
		writel(BIT(10), addr + FLASH_SYS_GLB_CLK_EB0_SET);//open clk_sd_card_eb

		writel(GENMASK(5, 0), addr + FLASH_SYS_GLB_CLK_DIV1_CLR);//clear bit[25:20]
		writel(CLK_SDIO_CLK_CARD_DIV(div), addr + FLASH_SYS_GLB_CLK_DIV1_SET);//set div
		writel(SDIO_CLK_DIV_UPDATE, addr + FLASH_SYS_GLB_CLK_DIV1_SET);
		udelay(2);
		writel(SDIO_CLK_DIV_UPDATE, addr + FLASH_SYS_GLB_CLK_DIV1_CLR);

		iounmap(addr);
	}
}
#endif

static void set_mmc_div(struct sdhci_axera_priv *priv, int enable)
{
	struct sdhci_host *host;
	void __iomem *addr = NULL;
	host = priv->host;

	if ((host->mmc->caps2 & MMC_CAP2_NO_SDIO) &&
		(host->mmc->caps2 & MMC_CAP2_NO_SD)) {
		if(host->mmc->caps2 & MMC_CAP2_HS200_1_8V_SDR || host->mmc->caps2 & (MMC_CAP2_HS400_1_8V | MMC_CAP2_HS200_1_8V_SDR) ||
				 host->mmc->caps2 & MMC_CAP2_HS400_ES) {
			addr = ioremap(CPU_SYS_GLB, 0x2020);
			if (enable) {
				writel(CLK_EMMC_CLK_CARD_DIV(0x1), addr + CPU_SYS_GLB_CLK_DIV0_SET);
				writel(EMMC_CLK_DIV_UPDATE, addr + CPU_SYS_GLB_CLK_DIV0_SET);
				udelay(2);
				writel(EMMC_CLK_DIV_UPDATE, addr + CPU_SYS_GLB_CLK_DIV0_CLR);
			} else {
				writel(CLK_EMMC_CLK_CARD_DIV(0x3f), addr + CPU_SYS_GLB_CLK_DIV0_CLR);
				writel(EMMC_CLK_DIV_UPDATE, addr + CPU_SYS_GLB_CLK_DIV0_SET);
				udelay(2);
				writel(EMMC_CLK_DIV_UPDATE, addr + CPU_SYS_GLB_CLK_DIV0_CLR);
			}
			iounmap(addr);
		}
	}

	if (priv->io_addr == SD_BASE_ADDR) {
		addr = ioremap(FLASH_SYS_GLB_BASE, 0x8020);
		if (enable) {
			writel(CLK_SD_CLK_CARD_DIV(0x1), addr + FLASH_SYS_GLB_CLK_DIV0_SET);
			writel(SD_CLK_DIV_UPDATE, addr + FLASH_SYS_GLB_CLK_DIV0_SET);
			udelay(2);
			writel(SD_CLK_DIV_UPDATE, addr + FLASH_SYS_GLB_CLK_DIV0_CLR);
		} else {
			writel(CLK_SD_CLK_CARD_DIV(0x3f), addr + FLASH_SYS_GLB_CLK_DIV0_CLR);
			writel(SD_CLK_DIV_UPDATE, addr + FLASH_SYS_GLB_CLK_DIV0_SET);
			udelay(2);
			writel(SD_CLK_DIV_UPDATE, addr + FLASH_SYS_GLB_CLK_DIV0_CLR);
		}
		iounmap(addr);
	}

	if (priv->io_addr == SDIO_BASE_ADDR) {
		addr = ioremap(FLASH_SYS_GLB_BASE, 0x8020);
		if (enable) {
			writel(CLK_SDIO_CLK_CARD_DIV(0x1), addr + FLASH_SYS_GLB_CLK_DIV1_SET);
			writel(SDIO_CLK_DIV_UPDATE, addr + FLASH_SYS_GLB_CLK_DIV1_SET);
			udelay(2);
			writel(SDIO_CLK_DIV_UPDATE, addr + FLASH_SYS_GLB_CLK_DIV1_CLR);
		} else {
			writel(CLK_SDIO_CLK_CARD_DIV(0x3f), addr + FLASH_SYS_GLB_CLK_DIV1_CLR);
			writel(SDIO_CLK_DIV_UPDATE, addr + FLASH_SYS_GLB_CLK_DIV1_SET);
			udelay(2);
			writel(SDIO_CLK_DIV_UPDATE, addr + FLASH_SYS_GLB_CLK_DIV1_CLR);
		}
		iounmap(addr);
	}
}

static int axera_prepare_clk(struct sdhci_axera_priv *priv, bool prepare)
{
#ifdef USING_CLK_FRAME
	int ret;

	if (IS_ERR(priv->aclk))
		return PTR_ERR(priv->aclk);
	if (IS_ERR(priv->cardclk))
		return PTR_ERR(priv->cardclk);
	pr_debug("%s: prepare:%d, aclk enable_count:%d,cardclk enable_count:%d,alk_enabled:%d,cardclk_enabled:%d\n",__func__, \
		prepare, __clk_get_enable_count(priv->aclk), __clk_get_enable_count(priv->cardclk), __clk_is_enabled(priv->aclk), __clk_is_enabled(priv->cardclk));

	if (prepare) {
		if (!__clk_is_enabled(priv->aclk)) {
			ret = clk_prepare_enable(priv->aclk);
			if (ret)
				return ret;
		}
		if (!IS_ERR(priv->pclk) && !__clk_is_enabled(priv->pclk)) {
			ret = clk_prepare_enable(priv->pclk);
			if (ret)
				return ret;
		}
		if (!__clk_is_enabled(priv->cardclk)) {
			set_mmc_div(priv, true);

			clk_prepare_enable(priv->cardclk);
			clk_disable_unprepare(priv->cardclk);
			clk_set_rate(priv->cardclk, 400000000); //400M
			clk_prepare_enable(priv->cardclk);
			ret = clk_set_rate(priv->cardclk, SDHCI_AXERA_200M_CLK);
			if (ret)
				return ret;
			pr_debug("sd base clk: %ldMHz\n", clk_get_rate(priv->cardclk)/1000000);
		}

		return ret;
	}

	if (__clk_is_enabled(priv->cardclk)) {
		set_mmc_div(priv, false);
		clk_disable_unprepare(priv->cardclk);
	}
	if (__clk_is_enabled(priv->aclk))
		clk_disable_unprepare(priv->aclk);
	if (!IS_ERR(priv->pclk) && __clk_is_enabled(priv->pclk))
		clk_disable_unprepare(priv->pclk);
#else
	void __iomem *addr = NULL;
	if (prepare) {
		if (!(priv->host->mmc->caps2 & MMC_CAP2_NO_MMC)) {
			ax_set_mmc_clk(priv->host);
		}
		if (priv->io_addr == SD_BASE_ADDR) {
			pr_info("enable sd clk\n");
			// set pck & ack
			addr = ioremap(FLASH_SYS_GLB_BASE, 0x8020);
			writel(PCLK_SD_EB_SET | ACLK_SD_EB_SET, addr + FLASH_SYS_GLB_CLK_EB1_SET);
			iounmap(addr);
			ax_set_mmc_clk(priv->host);
		}
		if (priv->io_addr == SDIO_BASE_ADDR) {
			pr_info("enable sdio clk\n");
			// set pck & ack
			addr = ioremap(FLASH_SYS_GLB_BASE, 0x8020);
			writel(PCLK_SDIO_EB_SET | ACLK_SDIO_EB_SET, addr + FLASH_SYS_GLB_CLK_EB1_SET);
			iounmap(addr);
			ax_set_mmc_clk(priv->host);
		}
	} else {
		if (!(priv->host->mmc->caps2 & MMC_CAP2_NO_MMC)) {
			set_mmc_div(priv, false);
			//close clk_emmc_card_eb
			addr = ioremap(CPU_SYS_GLB, 0x2020);
			writel(BIT(2), addr + CPU_SYS_GLB_CLK_EB0_CLR);
			iounmap(addr);
		}
		if (priv->io_addr == SD_BASE_ADDR) {
			pr_info("disable sd clk\n");
			set_mmc_div(priv, false);
			addr = ioremap(FLASH_SYS_GLB_BASE, 0x8020);
			//close clk_sd_card_eb
			writel(BIT(9), addr + FLASH_SYS_GLB_CLK_EB0_CLR);
			// disable pck & ack
			writel(PCLK_SD_EB_SET | ACLK_SD_EB_SET, addr + FLASH_SYS_GLB_CLK_EB1_CLR);
			iounmap(addr);
		}
		if (priv->io_addr == SDIO_BASE_ADDR) {
			pr_info("disable sdio clk\n");
			set_mmc_div(priv, false);
			//close clk_sdio_card_eb
			addr = ioremap(FLASH_SYS_GLB_BASE, 0x8020);
			writel(BIT(10), addr + FLASH_SYS_GLB_CLK_EB0_CLR);
			// disable pck & ack
			writel(PCLK_SDIO_EB_SET | ACLK_SDIO_EB_SET, addr + FLASH_SYS_GLB_CLK_EB1_CLR);
			iounmap(addr);
		}
	}
#endif
	return 0;
}

static void emmc_is_set_4bit(struct sdhci_host *host)
{
	u32 val;
	if ((host->mmc->caps2 & MMC_CAP2_NO_SDIO) &&
		(host->mmc->caps2 & MMC_CAP2_NO_SD)) {
		void __iomem *addr = NULL;
		addr = ioremap(TOP_CHIPMODE_GLB_SW, 0x4);
		val = (readl(addr) & FLASH_BOOT_MASK) >> 1;
		if ((val == EMMC_BOOT_4BIT_25M_768K) || (val == EMMC_BOOT_4BIT_25M_128K)) {
			host->mmc->caps &= ~MMC_CAP_8_BIT_DATA;
			host->mmc->caps2 &= ~(MMC_CAP2_HS400_ES | MMC_CAP2_HS400);
		}
		iounmap(addr);
	}
}

static struct mmc_host *sdio_host = NULL;

void axera_sdio_rescan(void)
{
	if (!sdio_host) {
		pr_err("axera sdio_host is NULL\n");
		return;
	}

	pr_debug("axera sdio_host rescan begin\n");

	mmc_detect_change(sdio_host, 0);
}
EXPORT_SYMBOL_GPL(axera_sdio_rescan);

static int sdhci_cdns_probe(struct platform_device *pdev)
{
	struct sdhci_host *host;
	const struct sdhci_pltfm_data *data;
	struct sdhci_pltfm_host *pltfm_host;
	struct sdhci_axera_priv *priv;
	unsigned int nr_phy_params;
	int ret;
	struct device *dev = &pdev->dev;
	static u16 version = 3 << SDHCI_SPEC_VER_SHIFT;
	struct resource *res;

	dev_info(dev, "axera sdhci probe\n");

	data = of_device_get_match_data(dev);
	if (!data)
		data = &sdhci_cdns_pltfm_data;

	nr_phy_params = sdhci_cdns_phy_param_count(dev->of_node);
	host = sdhci_pltfm_init(pdev, data,
				struct_size(priv, phy_params, nr_phy_params));
	if (IS_ERR(host)) {
		ret = PTR_ERR(host);
		goto ret_err;
	}

	pltfm_host = sdhci_priv(host);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	priv = sdhci_pltfm_priv(pltfm_host);
	priv->io_addr = res->start;
	priv->hrs_addr = host->ioaddr;
	priv->pdev = pdev;
	priv->host = host;
	priv->nr_phy_params = nr_phy_params;
	priv->host = host;
	priv->hrs_addr = host->ioaddr;
	priv->enhanced_strobe = false;
	host->ioaddr += SDHCI_CDNS_SRS_BASE;
	host->mmc_host_ops.hs400_enhanced_strobe =
				sdhci_cdns_hs400_enhanced_strobe;

	sdhci_get_of_property(pdev);
	ret = mmc_of_parse(host->mmc);
	if (ret)
		goto free;

	if (priv->io_addr == SD_BASE_ADDR) {
		host->mmc->caps &= ~MMC_CAP_AGGRESSIVE_PM;
		host->mmc->pm_flags |= MMC_PM_KEEP_POWER;
	}
	else if ((priv->io_addr == EMMC_BASE_ADDR) && (host->mmc->pm_caps & MMC_PM_KEEP_POWER)) {
		dev_info(&pdev->dev, "emmc pm_caps=0x%x\n", host->mmc->pm_caps);
		host->quirks2 |= SDHCI_QUIRK2_HOST_OFF_CARD_ON;
		host->mmc->pm_flags |= MMC_PM_KEEP_POWER;
	}
	ret = axera_get_clk_reset(pdev, priv);

	if (ret)
		goto free;

#ifdef USING_CLK_FRAME
	ret = axera_get_clk(pdev, priv);
	if (ret)
		goto free;
	clk_prepare_enable(priv->cardclk);
	clk_disable_unprepare(priv->cardclk);

	ret = axera_clk_reset_control(priv, DEASSERT);
	if (ret)
		goto free;

	clk_set_rate(priv->cardclk, 400000000); //400M
	clk_prepare_enable(priv->cardclk);
	clk_set_rate(priv->cardclk, SDHCI_AXERA_200M_CLK);
	if(!IS_ERR(priv->pclk))
		clk_prepare_enable(priv->pclk);
	clk_prepare_enable(priv->aclk);
#else
	axera_prepare_clk(priv, false);
	ret = axera_clk_reset_control(priv, DEASSERT);
	if (ret)
		goto free;
	axera_prepare_clk(priv, true);
#endif
	host->mmc->ocr_avail = MMC_VDD_32_33 | MMC_VDD_31_32 | MMC_VDD_30_31 | MMC_VDD_29_30 | MMC_VDD_28_29 | MMC_VDD_27_28 | MMC_VDD_165_195;
	sdhci_enable_v4_mode(host);
	__sdhci_read_caps(host, &version, NULL, NULL);

	emmc_is_set_4bit(host);

	get_mmc_hw_reset_gpio(host, priv, dev);
	sdhci_cdns_phy_param_parse(dev->of_node, priv);

	ret = sdhci_cdns_phy_init(priv);
	if (ret)
		goto disable_clk;
#ifdef SUPPORT_CLK_AUTOGATE
	pm_runtime_get_noresume(&pdev->dev);
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);
	pm_runtime_set_autosuspend_delay(&pdev->dev, 50);
	pm_runtime_use_autosuspend(&pdev->dev);
#endif
	ret = sdhci_add_host(host);
	if (ret)
		goto disable_clk;

	if (!(host->mmc->caps2 & MMC_CAP2_NO_SDIO)) {
		printk("axera sdio config\n");
		sdio_host = host->mmc;

		priv->sdio_voltage_sw = of_get_named_gpio(pdev->dev.of_node, "vol-sw-gpio", 0);
		if (gpio_is_valid(priv->sdio_voltage_sw)) {
			gpio_request(priv->sdio_voltage_sw, NULL);
			gpio_direction_output(priv->sdio_voltage_sw, 0); //3.3v
		} else {
			dev_info(&pdev->dev, "can't get sdio voltage switch gpio\n");
		}
	}

	/* Confirm the value of lock. */
	sdhci_cdns_read_phy_reg(priv,SDHCI_CDNS_PHY_LOCK_VALUE);
#ifdef SUPPORT_CLK_AUTOGATE
	pm_runtime_mark_last_busy(&pdev->dev);
	pm_runtime_put_autosuspend(&pdev->dev);
#endif
	return 0;

disable_clk:
#ifdef SUPPORT_CLK_AUTOGATE
	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);
	pm_runtime_put_noidle(&pdev->dev);
#endif
	axera_clk_reset_control(priv, ASSERT);
	axera_prepare_clk(priv, false);

free:
	sdhci_pltfm_free(pdev);

ret_err:
	return ret;
}

#ifdef SUPPORT_CLK_AUTOGATE
static int sdhci_axera_resume(struct device *dev)
{
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_axera_priv *priv = sdhci_pltfm_priv(pltfm_host);
	int ret;
	ret = axera_prepare_clk(priv, true);
	if (ret)
		return ret;
	return 0;
}

int sdhci_axera_suspend(struct device *dev)
{
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_axera_priv *priv = sdhci_pltfm_priv(pltfm_host);
	int ret;
	ret = axera_prepare_clk(priv, false);
	if (ret)
		return ret;
	return 0;
}

static const struct dev_pm_ops sdhci_cdns_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
	SET_RUNTIME_PM_OPS(sdhci_axera_suspend,
			   sdhci_axera_resume,
			   NULL)
};
#else
#ifdef CONFIG_PM_SLEEP
static int sdhci_axera_resume(struct device *dev)
{
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_axera_priv *priv = sdhci_pltfm_priv(pltfm_host);
	int ret;

	ret = axera_prepare_clk(priv, true);
	if (ret)
		return ret;

	ret = sdhci_cdns_phy_init(priv);
	if (ret)
		goto disable_clk;

	ret = sdhci_resume_host(host);
	if (ret)
		goto disable_clk;

	return 0;

disable_clk:
	axera_prepare_clk(priv, false);

	return ret;
}

int sdhci_axera_suspend(struct device *dev)
{
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_axera_priv *priv = sdhci_pltfm_priv(pltfm_host);
	int ret;
	if (host->tuning_mode != SDHCI_TUNING_MODE_3)
		mmc_retune_needed(host->mmc);

	ret = sdhci_suspend_host(host);
	if (ret)
		return ret;

	axera_prepare_clk(priv, false);

	return 0;
}
#endif
static const struct dev_pm_ops sdhci_cdns_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sdhci_axera_suspend, sdhci_axera_resume)
};
#endif

static const struct of_device_id sdhci_cdns_match[] = {
	{
		.compatible = "axera,sdhc",
		.data = &sdhci_cdns_uniphier_pltfm_data,
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sdhci_cdns_match);

static struct platform_driver sdhci_cdns_driver = {
	.driver = {
		.name = "sdhci-axera",
		.probe_type = PROBE_PREFER_ASYNCHRONOUS,
		.pm = &sdhci_cdns_pm_ops,
		.of_match_table = sdhci_cdns_match,
	},
	.probe = sdhci_cdns_probe,
	.remove = sdhci_pltfm_unregister,
};
module_platform_driver(sdhci_cdns_driver);

MODULE_AUTHOR("Masahiro Yamada <yamada.masahiro@socionext.com>");
MODULE_DESCRIPTION("Cadence SD/SDIO/eMMC Host Controller Driver");
MODULE_LICENSE("GPL");
