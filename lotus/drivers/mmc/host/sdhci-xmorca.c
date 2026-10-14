// SPDX-License-Identifier: GPL-2.0-or-later
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/reset.h>
#include <linux/mmc/host.h>
#include <linux/pm_runtime.h>
#include <linux/lotus/soc_chip.h>
#include <mach/platform.h>
#include <linux/of_gpio.h>
#include "sdhci-pltfm.h"
#include "sdhci-lotus.h"


#ifdef CONFIG_FPGA_SUPPORT
#define PHASE_SCALE	8
#define EDGE_TUNING_PHASE_STEP	1
#else
#define PHASE_SCALE	32
#define EDGE_TUNING_PHASE_STEP	4
#endif
#define NOT_FOUND	(-1)
#define MAX_TUNING_NUM	1
#define MAX_FREQ	200000000

#define LOTUS_MMC_AUTOSUSPEND_DELAY_MS 50

#define REG_EMMC_DRV_DLL_CTRL		0x1fc
#define REG_SDIO0_DRV_DLL_CTRL		0x220
#define REG_SDIO1_DRV_DLL_CTRL		0x234
#define REG_SDIO2_DRV_DLL_CTRL          /* no sdio2 */
#define SDIO_DRV_PHASE_SEL_MASK		(0x1f << 24)
#define sdio_drv_sel(phase)		((phase) << 24)

#define REG_EMMC_DRV_DLL_STATUS		0x210
#define REG_SDIO0_DRV_DLL_STATUS	0x228
#define REG_SDIO1_DRV_DLL_STATUS	0x23c
#define REG_SDIO2_DRV_DLL_STATUS        /* no sdio2 */
#define SDIO_DRV_DLL_LOCK		BIT(15)
#define SDIO_DRV_DLL_READY		BIT(14)

#define REG_EMMC_SAMPL_DLL_STATUS	0x208
#define REG_SDIO0_SAMPL_DLL_STATUS	0x224
#define REG_SDIO1_SAMPL_DLL_STATUS	0x238
#define REG_SDIO2_SAMPL_DLL_STATUS      /* no sdio2 */
#define SDIO_SAMPL_DLL_SLAVE_READY	BIT(0)

#define REG_EMMC_SAMPL_DLL_CTRL		0x1f4
#define REG_SDIO0_SAMPL_DLL_CTRL	0x22C
#define REG_SDIO1_SAMPL_DLL_CTRL	0x240
#define REG_SDIO2_SAMPL_DLL_CTRL        /* no sdio2 */
#define SDIO_SAMPL_DLL_SLAVE_EN		BIT(16)

#define REG_EMMC_SAMPLB_DLL_CTRL	0x1f8
#define EMMC_SAMPLB_DLL_CLK_MASK	(0x1f << 0)
#define emmc_samplb_sel(phase)		((phase) << 0)
#define REG_SDIO0_SAMPLB_DLL_CTRL	0x21C
#define REG_SDIO1_SAMPLB_DLL_CTRL	0x230
#define REG_SDIO2_SAMPLB_DLL_CTRL       /* no sdio2 */
#define SDIO_SAMPLB_DLL_CLK_MASK	(0x1f << 24)
#define sdio_samplb_sel(phase)		((phase) << 24)

#define REG_EMMC_DS_DLL_CTRL		0x200
#define EMMC_DS_DLL_MODE_SSEL		BIT(13)
#define EMMC_DS_DLL_SSEL_MASK		0x7f

#define REG_EMMC_DS180_DLL_CTRL		0x204
#define EMMC_DS180_DLL_BYPASS		BIT(15)
#define REG_EMMC_DS180_DLL_STATUS	0x218
#define EMMC_DS180_DLL_READY		BIT(0)

#define REG_EMMC_DS_DLL_STATUS		0x214
#define EMMC_DS_DLL_READY		BIT(0)

#define REG_EMMC_CLK_CTRL		0x1f4
#define REG_SDIO0_CLK_CTRL		0x22c
#define REG_SDIO1_CLK_CTRL		0x240
#define REG_SDIO2_CLK_CTRL              /* no sdio2 */
#define SDIO_CLK_DRV_DLL_RST		BIT(29)
#define SDIO_CLK_CRG_RST		BIT(27)

#define IO_CFG_PIN_MUX_MASK		(0xf << 0)
#define io_cfg_pin_mux_sel(type)	((type) << 0)
#define IO_CFG_PIN_MUX_TYPE_CLK_EMMC	0x0
#define IO_CFG_PIN_MUX_TYPE_CLK_SD	0x1

#define REG_MISC_CTRL71		0x8380
#define PS_POWER_SEL		1

#define REG_MISC_CTRL76		0x8800
#define IO_SDIO0_MS		1

#define REG_MISC_CTRL80		0x8810
#define IO_FLASH_MS		0x10


struct sdhci_lotus_priv {
	struct reset_control *crg_rst;
	struct reset_control *dll_rst;
	struct reset_control *sampl_rst; /* Not used */
	struct regmap *crg_regmap;
	struct regmap *iocfg_regmap;
	struct regmap *sysctrl_regmap;
	struct regmap *iocfg3_regmap;
	unsigned int f_max;
	unsigned int devid;
	unsigned int drv_phase;
	unsigned int sampl_phase;
	unsigned int tuning_phase;
	unsigned int reset_gpio_dev;
};

static void lotus_mmc_crg_init(struct sdhci_host *host);
static void sdhci_lotus_hs400_enhanced_strobe(struct mmc_host *mmc, struct mmc_ios *ios);
static int sdhci_lotus_parse_dt(struct sdhci_host *host);

static inline void *sdhci_get_pltfm_priv(struct sdhci_host *host)
{
	return sdhci_pltfm_priv(sdhci_priv(host));
}

static void sdhci_lotus_hs400_enhanced_strobe(struct mmc_host *mmc, struct mmc_ios *ios)
{
	u32 ctrl;
	struct sdhci_host *host = mmc_priv(mmc);

	ctrl = sdhci_readl(host, SDHCI_EMMC_CTRL);
	if (ios->enhanced_strobe)
		ctrl |= SDHCI_ENH_STROBE_EN;
	else
		ctrl &= ~SDHCI_ENH_STROBE_EN;

	sdhci_writel(host, ctrl, SDHCI_EMMC_CTRL);
}

static int sdhci_lotus_gpio_dev_reset(struct platform_device *pdev, struct sdhci_host *host)
{
	int ret;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_lotus_priv *lotus_priv = sdhci_pltfm_priv(pltfm_host);
	u32 flags = GPIOF_OUT_INIT_LOW | GPIOF_ACTIVE_LOW;

	ret = devm_gpio_request_one(&pdev->dev, lotus_priv->reset_gpio_dev, flags, "sdio-reset-dev");
	if (ret == -EBUSY) {
		devm_gpio_free(&pdev->dev, lotus_priv->reset_gpio_dev);
		ret = devm_gpio_request_one(&pdev->dev, lotus_priv->reset_gpio_dev, flags, "sdio-reset-dev");
	}

	if (ret) {
		pr_err("request gpio device failed\n");
		return ret;
	}

	mdelay(10); /* hold reset gpio */

	/* release reset */
	gpio_set_value(lotus_priv->reset_gpio_dev, 0x1);

	devm_gpio_free(&pdev->dev, lotus_priv->reset_gpio_dev);

	return 0;
}

static int sdhci_lotus_pltfm_init(struct platform_device *pdev, struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_lotus_priv *lotus_priv = sdhci_pltfm_priv(pltfm_host);
	struct device_node *np = pdev->dev.of_node;
	struct clk *clk = NULL;
	int ret;

	lotus_priv->crg_rst = devm_reset_control_get(&pdev->dev, "crg_reset");
	if (IS_ERR_OR_NULL(lotus_priv->crg_rst)) {
		dev_err(&pdev->dev, "get crg_rst failed.\n");
		return PTR_ERR(lotus_priv->crg_rst);
	}

	lotus_priv->dll_rst = devm_reset_control_get(&pdev->dev, "dll_reset");
	if (IS_ERR_OR_NULL(lotus_priv->dll_rst)) {
		dev_err(&pdev->dev, "get dll_rst failed.\n");
		return PTR_ERR(lotus_priv->dll_rst);
	}

	lotus_priv->sampl_rst = NULL;

	lotus_priv->crg_regmap = syscon_regmap_lookup_by_phandle(np, "crg_regmap");
	if (IS_ERR(lotus_priv->crg_regmap)) {
		dev_err(&pdev->dev, "get crg regmap failed.\n");
		return PTR_ERR(lotus_priv->crg_regmap);
	}

	lotus_priv->iocfg_regmap = syscon_regmap_lookup_by_phandle(np, "iocfg_regmap");
	if (IS_ERR(lotus_priv->iocfg_regmap)) {
		dev_err(&pdev->dev, "get iocfg regmap failed.\n");
		return PTR_ERR(lotus_priv->iocfg_regmap);
	}

	lotus_priv->sysctrl_regmap = syscon_regmap_lookup_by_phandle(np, "sysctrl_regmap");
	if (IS_ERR(lotus_priv->sysctrl_regmap))
		dev_info(&pdev->dev, "no sysctrl regmap.\n");

	lotus_priv->iocfg3_regmap = syscon_regmap_lookup_by_phandle(np, "iocfg3_regmap");
	if (IS_ERR(lotus_priv->iocfg3_regmap))
		dev_info(&pdev->dev, "no iocfg3 regmap.\n");

	if (!of_property_read_u32(np, "reset-gpio-dev", &lotus_priv->reset_gpio_dev)) {
		dev_info(&pdev->dev, "find reset-gpio(%d) for device\n", lotus_priv->reset_gpio_dev);
		if (sdhci_lotus_gpio_dev_reset(pdev, host))
			dev_err(&pdev->dev, "failed to reset sdio device\n");
	}

	if (of_property_read_u32(np, "devid", &lotus_priv->devid))
		return -EINVAL;

	clk = devm_clk_get(mmc_dev(host->mmc), "mmc_clk");
	if (IS_ERR_OR_NULL(clk)) {
		dev_err(mmc_dev(host->mmc), "get clk err\n");
		return -EINVAL;
	}

	pltfm_host->clk = clk;

	clk_prepare_enable(pltfm_host->clk);

	lotus_mmc_crg_init(host);
	ret = sdhci_lotus_parse_dt(host);
	if (ret)
		return ret;

	/*
	 * Only eMMC has a hw reset, and now eMMC signaling
	 * is fixed to 180
	 */
	if (host->mmc->caps & MMC_CAP_HW_RESET) {
		host->flags &= ~SDHCI_SIGNALING_330;
		host->flags |= SDHCI_SIGNALING_180;
	}

	/*
	 * We parse the support timings from dts, so we read the
	 * host capabilities early and clear the timing capabilities,
	 * SDHCI_QUIRK_MISSING_CAPS is set so that sdhci driver would
	 * not read it again
	 */
	host->caps = sdhci_readl(host, SDHCI_CAPABILITIES);
	host->caps &= ~(SDHCI_CAN_DO_HISPD | SDHCI_CAN_VDD_300);
	host->caps1 = sdhci_readl(host, SDHCI_CAPABILITIES_1);
	host->caps1 &= ~(SDHCI_SUPPORT_SDR50 | SDHCI_SUPPORT_SDR104 |
				SDHCI_SUPPORT_DDR50 | SDHCI_CAN_DO_ADMA3);
	host->quirks |= SDHCI_QUIRK_MISSING_CAPS |
			SDHCI_QUIRK_NO_ENDATTR_IN_NOPDESC |
			SDHCI_QUIRK_SINGLE_POWER_WRITE |
			SDHCI_QUIRK_FORCE_1_BIT_DATA;

	if ((host->mmc->caps & MMC_CAP_NEEDS_POLL) && (lotus_priv->devid == 1))
		host->quirks |= SDHCI_QUIRK_BROKEN_CARD_DETECTION;

	host->mmc_host_ops.hs400_enhanced_strobe =
			sdhci_lotus_hs400_enhanced_strobe;

	mci_host[host->mmc->index] = host->mmc;

	return 0;
}

static void lotus_wait_ds_dll_lock(struct sdhci_host *host)
{
	/* Do nothing */
}

static void lotus_wait_ds_180_dll_ready(struct sdhci_host *host)
{
	struct sdhci_lotus_priv *lotus_priv = sdhci_get_pltfm_priv(host);
	unsigned int reg;
	unsigned int timeout = 20;

	do {
		reg = 0;
		regmap_read(lotus_priv->crg_regmap,
				REG_EMMC_DS180_DLL_STATUS, &reg);
		if (reg & EMMC_DS180_DLL_READY)
			return;

		mdelay(1);
		timeout--;
	} while (timeout > 0);

	pr_err("%s: DS 180 DLL master not ready.\n", mmc_hostname(host->mmc));
}

static void lotus_set_ds_dll_delay(struct sdhci_host *host)
{
	/* Do nothing */
}

static void lotus_host_extra_init(struct sdhci_host *host)
{
	unsigned short ctrl;
	unsigned int mbiiu_ctrl, val;

	ctrl = sdhci_readw(host, SDHCI_MSHC_CTRL);
	ctrl &= ~SDHCI_CMD_CONFLIT_CHECK;
	sdhci_writew(host, ctrl, SDHCI_MSHC_CTRL);

	mbiiu_ctrl = sdhci_readl(host, SDHCI_AXI_MBIIU_CTRL);
	mbiiu_ctrl &= ~(SDHCI_GM_WR_OSRC_LMT_MASK | SDHCI_GM_RD_OSRC_LMT_MASK |
			SDHCI_UNDEFL_INCR_EN);
	mbiiu_ctrl |= (SDHCI_GM_WR_OSRC_LMT_SEL(0x7) | /* set write outstanding 8 (lmt + 1) */
		       SDHCI_GM_RD_OSRC_LMT_SEL(0x7)); /* set read outstanding 8 (lmt + 1) */
	sdhci_writel(host, mbiiu_ctrl, SDHCI_AXI_MBIIU_CTRL);

	val = sdhci_readl(host, SDHCI_MULTI_CYCLE);
	val &= ~SDHCI_CMD_DLY_EN;
	val |= SDHCI_EDGE_DETECT_EN | SDHCI_DATA_DLY_EN;
	val &= ~SDHCI_DOUT_EN_F_EDGE;

	sdhci_writel(host, val, SDHCI_MULTI_CYCLE);
	host->error_count = 0;

	if (host->mmc->caps & MMC_CAP_HW_RESET) {
		sdhci_writel(host, 0x0, SDHCI_EMMC_HW_RESET);
		udelay(10); /* delay 10us */
		sdhci_writel(host, 0x1, SDHCI_EMMC_HW_RESET);
		udelay(200); /* delay 200us */
	}
}

static void lotus_set_emmc_ctrl(struct sdhci_host *host)
{
	unsigned int reg;

	reg = sdhci_readl(host, SDHCI_EMMC_CTRL);
	reg |= SDHCI_CARD_IS_EMMC;
	sdhci_writel(host, reg, SDHCI_EMMC_CTRL);
}
static void lotus_set_mmc_drv(struct sdhci_host *host)
{
	lotus_set_emmc_ctrl(host);
}

static void lotus_set_io_config(struct sdhci_host *host)
{
	struct sdhci_lotus_priv *lotus_priv = sdhci_get_pltfm_priv(host);
	unsigned int devid = lotus_priv->devid;

	if (devid == 0) {
		/* For mmc0: eMMC */
		lotus_set_mmc_drv(host);
	}

	if ((host->timing == MMC_TIMING_UHS_DDR50) ||
		(host->timing == MMC_TIMING_MMC_DDR52)) {
		unsigned int reg;

		reg = sdhci_readw(host, SDHCI_MULTI_CYCLE);
		reg &= ~SDHCI_DATA_DLY_EN;
		sdhci_writew(host, reg, SDHCI_MULTI_CYCLE);
	}
}

static void lotus_set_io_voltage(struct sdhci_host *host, unsigned char signal_voltage)
{

}

static void lotus_get_phase(struct sdhci_host *host)
{
	struct sdhci_lotus_priv *lotus_priv = sdhci_get_pltfm_priv(host);
	unsigned int devid = lotus_priv->devid;

#ifdef CONFIG_FPGA_SUPPORT
	/* The drv_phase and sample_phase in fpga are 1/4 in asic.
	 * 1 -> 45 degree in fpag, 1 -> 45 degree in asic
	 */
	unsigned int fix_num = 4;
#else
	unsigned int fix_num = 1;
#endif

	if ((devid == 0) || (devid == 1)) {
		/* For eMMC and SD card */
		if (host->mmc->ios.timing == MMC_TIMING_MMC_HS400) {
			lotus_priv->drv_phase = 8/fix_num;   /* 8 for 90 degree */
			lotus_priv->sampl_phase = lotus_priv->tuning_phase/fix_num;
		} else if (host->mmc->ios.timing == MMC_TIMING_MMC_HS200 ||
			(host->mmc->ios.timing == MMC_TIMING_UHS_SDR104)) {
			lotus_priv->drv_phase = 20/fix_num;   /* 20 for 225 degree */
			lotus_priv->sampl_phase = lotus_priv->tuning_phase/fix_num;
		} else if (host->mmc->ios.timing == MMC_TIMING_UHS_SDR50) {
			lotus_priv->drv_phase = 16/fix_num;   /* 16 for 180 degree */
			lotus_priv->sampl_phase = lotus_priv->tuning_phase/fix_num;
		} else if (host->mmc->ios.timing == MMC_TIMING_MMC_DDR52 ||
			(host->mmc->ios.timing == MMC_TIMING_UHS_DDR50)) {
			lotus_priv->drv_phase = 8/fix_num;    /* 8 for 90 degree */
			lotus_priv->sampl_phase = 4/fix_num; /* 4 for 45 degree */
		} else if (host->mmc->ios.timing == MMC_TIMING_MMC_HS) {
			lotus_priv->drv_phase = 16/fix_num;   /* 16 for 180 degree */
			lotus_priv->sampl_phase = 4/fix_num; /* 4 for 45 degree */
		} else if (host->mmc->ios.timing == MMC_TIMING_UHS_SDR25 ||
			(host->mmc->ios.timing == MMC_TIMING_SD_HS)) {
			lotus_priv->drv_phase = 16/fix_num;   /* 16 for 180 degree */
			lotus_priv->sampl_phase = 4/fix_num; /* 4 for 45 degree */
		} else {
			lotus_priv->drv_phase = 16/fix_num;  /* 16 for 180 degree */
			lotus_priv->sampl_phase = 0; /* 0 for 0 degree */
		}
	} else {
		/* For SDIO device */
		if (host->mmc->ios.timing == MMC_TIMING_UHS_SDR104) {
			lotus_priv->drv_phase = 20/fix_num;   /* 20 for 225 degree */
			lotus_priv->sampl_phase = lotus_priv->tuning_phase/fix_num;
		} else if (host->mmc->ios.timing == MMC_TIMING_UHS_SDR50) {
			lotus_priv->drv_phase = 16/fix_num;   /* 16 for 180 degree */
			lotus_priv->sampl_phase = lotus_priv->tuning_phase/fix_num;
		} else if (host->mmc->ios.timing == MMC_TIMING_UHS_DDR50) {
			lotus_priv->drv_phase = 8/fix_num;	  /* 8 for 90 degree */
			lotus_priv->sampl_phase = 4/fix_num; /* 4 for 45 degree */
		} else if (host->mmc->ios.timing == MMC_TIMING_UHS_SDR25 ||
			(host->mmc->ios.timing == MMC_TIMING_SD_HS)) {
			lotus_priv->drv_phase = 16/fix_num;   /* 16 for 180 degree */
			lotus_priv->sampl_phase = 4/fix_num; /* 4 for 45 degree */
		} else {
			lotus_priv->drv_phase = 16/fix_num;  /* 16 for 180 degree */
			lotus_priv->sampl_phase = 0; /* 0 for 0 degree */
		}

	}
}

static int lotus_support_runtime_pm(struct sdhci_host *host)
{
	struct sdhci_lotus_priv *lotus_priv = sdhci_get_pltfm_priv(host);
	unsigned int devid = lotus_priv->devid;

	/* Only enable for mmc0 eMMC and SD card */
	if ((devid == 0) || (devid == 1))
		return 1;
	else
		return 0;
}

#include "sdhci-lotus-common.c"
