/*
 * Copyright (c) XMEDIA. All rights reserved.
 */
#include "sdhci-xmedia.h"

static unsigned int sdhci_xmedia_get_max_clk(struct sdhci_host *host)
{
	struct sdhci_xmedia_priv *xmedia_priv = sdhci_get_pltfm_priv(host);

	return xmedia_priv->f_max;
}

static int sdhci_xmedia_parse_dt(struct sdhci_host *host)
{
	struct sdhci_xmedia_priv *xmedia_priv = sdhci_get_pltfm_priv(host);
	struct device_node *np = host->mmc->parent->of_node;
	int ret, len;

	ret = mmc_of_parse(host->mmc);
	if (ret)
		return ret;

	if (of_property_read_u32(np, "max-frequency", &xmedia_priv->f_max))
		xmedia_priv->f_max = MAX_FREQ;

	if (of_find_property(np, "mmc-cmd-queue", &len))
		host->mmc->caps2 |= MMC_CAP2_CMD_QUEUE;

	if (of_find_property(np, "mmc-broken-cmd23", &len) ||
		(host->mmc->caps2 & MMC_CAP2_CMD_QUEUE))
		host->quirks2 |= SDHCI_QUIRK2_HOST_NO_CMD23;

	return 0;
}

static void xmedia_mmc_crg_init(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xmedia_priv *xmedia_priv = sdhci_pltfm_priv(pltfm_host);

	clk_prepare_enable(pltfm_host->clk);
	reset_control_assert(xmedia_priv->crg_rst);
	reset_control_assert(xmedia_priv->dll_rst);
	if (xmedia_priv->sampl_rst)
		reset_control_assert(xmedia_priv->sampl_rst);

	udelay(25); /* delay 25us */
	reset_control_deassert(xmedia_priv->crg_rst);
	udelay(10); /* delay 10us */
}

static void xmedia_set_drv_phase(struct sdhci_host *host, unsigned int phase)
{
	struct sdhci_xmedia_priv *xmedia_priv = sdhci_get_pltfm_priv(host);
	unsigned int devid = xmedia_priv->devid;
	unsigned int offset[] = {
		REG_EMMC_DRV_DLL_CTRL,
		REG_SDIO0_DRV_DLL_CTRL,
		REG_SDIO1_DRV_DLL_CTRL,
		REG_SDIO2_DRV_DLL_CTRL
	};

	regmap_write_bits(xmedia_priv->crg_regmap, offset[devid],
			SDIO_DRV_PHASE_SEL_MASK, sdio_drv_sel(phase));
}

static void xmedia_set_sampl_phase(struct sdhci_host *host, unsigned int phase)
{
	unsigned int reg;

	reg = sdhci_readl(host, SDHCI_AT_STAT);
	reg &= ~SDHCI_PHASE_SEL_MASK;
	reg |= phase;
	sdhci_writel(host, reg, SDHCI_AT_STAT);
}

static void xmedia_disable_card_clk(struct sdhci_host *host)
{
	u16 clk;

	clk = sdhci_readw(host, SDHCI_CLOCK_CONTROL);
	clk &= ~SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, clk, SDHCI_CLOCK_CONTROL);
}

static void xmedia_enable_card_clk(struct sdhci_host *host)
{
	u16 clk;

	clk = sdhci_readw(host, SDHCI_CLOCK_CONTROL);
	clk |= SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, clk, SDHCI_CLOCK_CONTROL);
}

static void xmedia_disable_inter_clk(struct sdhci_host *host)
{
	u16 clk;

	clk = sdhci_readw(host, SDHCI_CLOCK_CONTROL);
	clk &= ~SDHCI_CLOCK_INT_EN;
	sdhci_writew(host, clk, SDHCI_CLOCK_CONTROL);
}

static void xmedia_enable_sampl_dll_slave(struct sdhci_host *host)
{
	struct sdhci_xmedia_priv *xmedia_priv = sdhci_get_pltfm_priv(host);
	unsigned int devid = xmedia_priv->devid;
	unsigned int offset[] = {
		REG_EMMC_SAMPL_DLL_CTRL,
		REG_SDIO0_SAMPL_DLL_CTRL,
		REG_SDIO1_SAMPL_DLL_CTRL,
		REG_SDIO2_SAMPL_DLL_CTRL
	};

	regmap_write_bits(xmedia_priv->crg_regmap, offset[devid],
			SDIO_SAMPL_DLL_SLAVE_EN, SDIO_SAMPL_DLL_SLAVE_EN);
}

static void xmedia_wait_drv_dll_lock(struct sdhci_host *host)
{
	struct sdhci_xmedia_priv *xmedia_priv = sdhci_get_pltfm_priv(host);
	unsigned int devid = xmedia_priv->devid;
	unsigned int reg;
	unsigned int timeout = 20;
	unsigned int offset[] = {
		REG_EMMC_DRV_DLL_STATUS,
		REG_SDIO0_DRV_DLL_STATUS,
		REG_SDIO1_DRV_DLL_STATUS,
		REG_SDIO2_DRV_DLL_STATUS
	};

	do {
		reg = 0;
		regmap_read(xmedia_priv->crg_regmap, offset[devid], &reg);
		if (reg & SDIO_DRV_DLL_LOCK)
			return;

		mdelay(1);
		timeout--;
	} while (timeout > 0);

	pr_err("%s: DRV DLL master not locked.\n", mmc_hostname(host->mmc));
}

static void xmedia_wait_sampl_dll_slave_ready(struct sdhci_host *host)
{
	struct sdhci_xmedia_priv *xmedia_priv = sdhci_get_pltfm_priv(host);
	unsigned int devid = xmedia_priv->devid;
	unsigned int reg;
	unsigned int timeout = 20;
	unsigned int offset[] = {
		REG_EMMC_SAMPL_DLL_STATUS,
		REG_SDIO0_SAMPL_DLL_STATUS,
		REG_SDIO1_SAMPL_DLL_STATUS,
		REG_SDIO2_SAMPL_DLL_STATUS
	};

	do {
		reg = 0;
		regmap_read(xmedia_priv->crg_regmap, offset[devid], &reg);
		if (reg & SDIO_SAMPL_DLL_SLAVE_READY)
			return;

		mdelay(1);
		timeout--;
	} while (timeout > 0);

	pr_err("%s: SAMPL DLL slave not ready.\n", mmc_hostname(host->mmc));
}

static void xmedia_enable_sample(struct sdhci_host *host)
{
	unsigned int reg;

	reg = sdhci_readl(host, SDHCI_AT_CTRL);
	reg |= SDHCI_SAMPLE_EN;
	sdhci_writel(host, reg, SDHCI_AT_CTRL);
}

static void sdhci_xmedia_set_clock(struct sdhci_host *host, unsigned int clock)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xmedia_priv *xmedia_priv = sdhci_pltfm_priv(pltfm_host);
	unsigned long timeout;
	u16 clk;

	host->mmc->actual_clock = 0;
	xmedia_disable_card_clk(host);
	udelay(25); /* delay 25us */
	xmedia_disable_inter_clk(host);
	if (clock == 0)
		return;

	reset_control_assert(xmedia_priv->dll_rst);
	if (xmedia_priv->sampl_rst)
		reset_control_assert(xmedia_priv->sampl_rst);
	udelay(25); /* delay 25us */

	clk_set_rate(pltfm_host->clk, clock);
	host->mmc->actual_clock = clk_get_rate(pltfm_host->clk);

	xmedia_get_phase(host);
	xmedia_set_drv_phase(host, xmedia_priv->drv_phase);
	xmedia_enable_sample(host);
	xmedia_set_sampl_phase(host, xmedia_priv->sampl_phase);
	udelay(25); /* delay 25us */

	if (host->mmc->actual_clock > MMC_HIGH_52_MAX_DTR) {
		xmedia_enable_sampl_dll_slave(host);
		reset_control_deassert(xmedia_priv->dll_rst);
		if (xmedia_priv->sampl_rst)
			reset_control_deassert(xmedia_priv->sampl_rst);
	}

	clk = sdhci_readw(host, SDHCI_CLOCK_CONTROL);
	clk |= SDHCI_CLOCK_INT_EN | SDHCI_CLOCK_PLL_EN;
	sdhci_writew(host, clk, SDHCI_CLOCK_CONTROL);
	timeout = 20; /* default timeout 20ms */
	clk = sdhci_readw(host, SDHCI_CLOCK_CONTROL);
	while (!(clk & SDHCI_CLOCK_INT_STABLE)) {
		clk = sdhci_readw(host, SDHCI_CLOCK_CONTROL);
		if (timeout == 0) {
			pr_err("%s: Internal clock never stabilised.\n",
					mmc_hostname(host->mmc));
			return;
		}
		timeout--;
		mdelay(1);
	}

	if (host->mmc->actual_clock > MMC_HIGH_52_MAX_DTR) {
		xmedia_wait_drv_dll_lock(host);
		xmedia_wait_sampl_dll_slave_ready(host);
	}

	if (host->mmc->ios.timing == MMC_TIMING_MMC_HS400)
		xmedia_wait_ds_180_dll_ready(host);

	clk |= SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, clk, SDHCI_CLOCK_CONTROL);
	udelay(100); /* delay 100us */

	if (host->mmc->ios.timing == MMC_TIMING_MMC_HS400) {
		xmedia_wait_ds_dll_lock(host);
		xmedia_set_ds_dll_delay(host);
	}
}

static void xmedia_select_sampl_phase(struct sdhci_host *host, unsigned int phase)
{
	xmedia_disable_card_clk(host);
	xmedia_set_sampl_phase(host, phase);
	xmedia_wait_sampl_dll_slave_ready(host);
	xmedia_enable_card_clk(host);
	udelay(1);
}

static int xmedia_send_tuning(struct sdhci_host *host, u32 opcode)
{
	int count, err;

	count = 0;
	do {
		err = mmc_send_tuning(host->mmc, opcode, NULL);
		if (err)
			break;

		count++;
	} while (count < MAX_TUNING_NUM);

	return err;
}

static void xmedia_pre_tuning(struct sdhci_host *host)
{
	sdhci_writel(host, host->ier | SDHCI_INT_DATA_AVAIL, SDHCI_INT_ENABLE);
	sdhci_writel(host, host->ier | SDHCI_INT_DATA_AVAIL,
		SDHCI_SIGNAL_ENABLE);

	xmedia_wait_drv_dll_lock(host);
	xmedia_enable_sampl_dll_slave(host);
	xmedia_enable_sample(host);
	host->is_tuning = 1;
}

static void xmedia_post_tuning(struct sdhci_host *host)
{
	unsigned short ctrl;

	ctrl = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	ctrl |= SDHCI_CTRL_TUNED_CLK;
	sdhci_writew(host, ctrl, SDHCI_HOST_CONTROL2);

	sdhci_writel(host, host->ier, SDHCI_INT_ENABLE);
	sdhci_writel(host, host->ier, SDHCI_SIGNAL_ENABLE);
	host->is_tuning = 0;
}

#ifndef SDHCI_XMEDIA_EDGE_TUNING
static int xmedia_get_best_sampl(u32 candidates)
{
	int rise = NOT_FOUND;
	int fall = NOT_FOUND;
	int win_max_r = NOT_FOUND;
	int win_max_f = NOT_FOUND;
	int end_fall = NOT_FOUND;
	int found = NOT_FOUND;
	int win_max = 0;
	int i, win;

	for (i = 0; i < PHASE_SCALE; i++) {
		if ((candidates & 0x3) == 0x2)
			rise = (i + 1) % PHASE_SCALE;

		if ((candidates & 0x3) == 0x1) {
			fall = i;
			if (rise != NOT_FOUND) {
				win = fall - rise + 1;
				if (win > win_max) {
					win_max = win;
					found = (fall + rise) / 2; /* Get window center by devide 2 */
					win_max_r = rise;
					win_max_f = fall;
					rise = NOT_FOUND;
					fall = NOT_FOUND;
				}
			} else {
				end_fall = fall;
			}
		}
		candidates = ror32(candidates, 1);
	}

	if (end_fall != NOT_FOUND && rise != NOT_FOUND) {
		fall = end_fall;
		if (end_fall < rise)
			end_fall += PHASE_SCALE;

		win = end_fall - rise + 1;
		if (win > win_max) {
			found = (rise + (win / 2)) % PHASE_SCALE; /* Get window center by devide 2 */
			win_max_r = rise;
			win_max_f = fall;
		}
	}

	if (found != NOT_FOUND)
		pr_info("valid phase shift [%d, %d] Final Phase:%d\n",
				win_max_r, win_max_f, found);

	return found;
}

static int sdhci_xmedia_exec_tuning(struct sdhci_host *host, u32 opcode)
{
	struct sdhci_xmedia_priv *xmedia_priv = sdhci_get_pltfm_priv(host);
	unsigned int sampl;
	unsigned int candidates = 0;
	int phase, err;

	xmedia_pre_tuning(host);

	for (sampl = 0; sampl < PHASE_SCALE; sampl++) {
		xmedia_select_sampl_phase(host, sampl);

		err = xmedia_send_tuning(host, opcode);
		if (err)
			pr_debug("send tuning CMD%u fail! phase:%d err:%d\n",
					opcode, sampl, err);
		else
			candidates |= (0x1 << sampl);
	}

	pr_info("%s: tuning done! candidates 0x%X: ",
			mmc_hostname(host->mmc), candidates);

	phase = xmedia_get_best_sampl(candidates);
	if (phase == NOT_FOUND) {
		phase = xmedia_priv->sampl_phase;
		pr_err("no valid phase shift! use default %d\n", phase);
	}

	xmedia_priv->tuning_phase = phase;
	xmedia_select_sampl_phase(host, phase);
	xmedia_post_tuning(host);

	return 0;
}

#else
static void xmedia_enable_edge_tuning(struct sdhci_host *host)
{
	struct sdhci_xmedia_priv *xmedia_priv = sdhci_get_pltfm_priv(host);
	unsigned int devid = xmedia_priv->devid;
	unsigned int samplb_offset[] = {
		REG_EMMC_SAMPLB_DLL_CTRL,
		REG_SDIO0_SAMPLB_DLL_CTRL,
		REG_SDIO1_SAMPLB_DLL_CTRL,
		REG_SDIO2_SAMPLB_DLL_CTRL
	};
	unsigned int reg;

#ifdef CONFIG_XM_FPGA
	/* fpga: 1 -> 45 degree */
	regmap_write_bits(xmedia_priv->crg_regmap, samplb_offset[devid],
			SDIO_SAMPLB_DLL_CLK_MASK, sdio_samplb_sel(2)); /* 8 for 180 degree */

#else
	/* asic: 1-> 11.25 degree  */
	regmap_write_bits(xmedia_priv->crg_regmap, samplb_offset[devid],
			SDIO_SAMPLB_DLL_CLK_MASK, sdio_samplb_sel(8)); /* 8 for 180 degree */
#endif

	reg = sdhci_readl(host, SDHCI_MULTI_CYCLE);
	reg |= SDHCI_EDGE_DETECT_EN;
	sdhci_writel(host, reg, SDHCI_MULTI_CYCLE);
}

static void xmedia_disable_edge_tuning(struct sdhci_host *host)
{
	unsigned int reg;

	reg = sdhci_readl(host, SDHCI_MULTI_CYCLE);
	reg &= ~SDHCI_EDGE_DETECT_EN;
	sdhci_writel(host, reg, SDHCI_MULTI_CYCLE);
}

static int sdhci_xmedia_exec_edge_tuning(struct sdhci_host *host, u32 opcode)
{
	struct sdhci_xmedia_priv *xmedia_priv = sdhci_get_pltfm_priv(host);
	unsigned int index, val;
	unsigned int edge_p2f, edge_f2p, start, end, phase;
	unsigned int fall, rise, fall_updat_flag;
	unsigned int found = 0;
	unsigned int prev_found = 0;
	int prev_err = 0;
	int err;

	xmedia_pre_tuning(host);
	xmedia_enable_edge_tuning(host);

	start = 0;
	end = PHASE_SCALE / EDGE_TUNING_PHASE_STEP;

	edge_p2f = start;
	edge_f2p = end;
	for (index = 0; index <= end; index++) {
		xmedia_select_sampl_phase(host, index * EDGE_TUNING_PHASE_STEP);

		err = mmc_send_tuning(host->mmc, opcode, NULL);
		if (!err) {
			val = sdhci_readl(host, SDHCI_MULTI_CYCLE);
			found = val & SDHCI_FOUND_EDGE;
		} else {
			found = 1;
		}

		if (prev_found && !found)
			edge_f2p = index;
		else if (!prev_found && found)
			edge_p2f = index;

		if ((edge_p2f != start) && (edge_f2p != end))
			break;

		prev_found = found;
		found = 0;
	}

	if ((edge_p2f == start) && (edge_f2p == end)) {
		pr_err("%s: tuning failed! can not found edge!\n",
				mmc_hostname(host->mmc));
		return -1;
	}

	xmedia_disable_edge_tuning(host);

	start = edge_p2f * EDGE_TUNING_PHASE_STEP;
	end = edge_f2p * EDGE_TUNING_PHASE_STEP;
	if (end <= start)
		end += PHASE_SCALE;

	fall = start;
	rise = end;
	fall_updat_flag = 0;
	for (index = start; index <= end; index++) {
		xmedia_select_sampl_phase(host, index % PHASE_SCALE);

		err = xmedia_send_tuning(host, opcode);
		if (err)
			pr_debug("send tuning CMD%u fail! phase:%d err:%d\n",
					opcode, index, err);

		if (err && index == start) {
			if (!fall_updat_flag) {
				fall_updat_flag = 1;
				fall = start;
			}
		} else {
			if (!prev_err && err) {
				if (!fall_updat_flag) {
					fall_updat_flag = 1;
					fall = index;
				}
			}
		}


		if (prev_err && !err)
			rise = index;

		if (err && index == end)
			rise = end;


		prev_err = err;
	}

	phase = ((fall + rise) / 2 + PHASE_SCALE / 2) % PHASE_SCALE; /* Get window center by divide 2 */

	pr_info("%s: tuning done! valid phase shift [%d, %d] Final Phase:%d\n",
			mmc_hostname(host->mmc), rise % PHASE_SCALE,
			fall % PHASE_SCALE, phase);

	xmedia_priv->tuning_phase = phase;
	xmedia_select_sampl_phase(host, phase);
	xmedia_post_tuning(host);

	return 0;
}
#endif

static void sdhci_xmedia_set_uhs_signaling(struct sdhci_host *host, unsigned timing)
{
	sdhci_set_uhs_signaling(host, timing);
	host->timing = timing;

	/* Xmedia add set io config here to set pin drv strength */
	xmedia_set_io_config(host);
}

static void sdhci_xmedia_hw_reset(struct sdhci_host *host)
{
	sdhci_writel(host, 0x0, SDHCI_EMMC_HW_RESET);
	udelay(10); /* delay 10us */
	sdhci_writel(host, 0x1, SDHCI_EMMC_HW_RESET);
	udelay(200); /* delay 200us */
}

/*
 * This api is for wifi driver rescan the sdio device,
 * ugly but it is needed
 */
int xmedia_sdio_rescan(int slot)
{
	struct mmc_host *mmc = mci_host[slot];

	if (mmc == NULL) {
		pr_err("invalid mmc, please check the argument\n");
		return -EINVAL;
	}

	mmc_detect_change(mmc, 0);
	return 0;
}
EXPORT_SYMBOL_GPL(xmedia_sdio_rescan);

static const struct sdhci_ops sdhci_xmedia_ops = {
	.get_max_clock  = sdhci_xmedia_get_max_clk,
	.set_clock = sdhci_xmedia_set_clock,
	.set_bus_width = sdhci_set_bus_width,
	.reset = sdhci_reset,
	.set_uhs_signaling = sdhci_xmedia_set_uhs_signaling,
	.hw_reset = sdhci_xmedia_hw_reset,

#ifdef SDHCI_XMEDIA_EDGE_TUNING
	.platform_execute_tuning = sdhci_xmedia_exec_edge_tuning,
#else
	.platform_execute_tuning = sdhci_xmedia_exec_tuning,
#endif
	.pre_init = xmedia_mmc_crg_init,
	.extra_init = xmedia_host_extra_init,
};

static const struct sdhci_pltfm_data sdhci_xmedia_pdata = {
	.ops = &sdhci_xmedia_ops,
	.quirks = SDHCI_QUIRK_DATA_TIMEOUT_USES_SDCLK |
		  SDHCI_QUIRK_INVERTED_WRITE_PROTECT |
		  SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN |
		  SDHCI_QUIRK_BROKEN_TIMEOUT_VAL,
	.quirks2 = SDHCI_QUIRK2_PRESET_VALUE_BROKEN,
};
static int sdhci_xmedia_probe(struct platform_device *pdev)
{
	struct sdhci_host *host;
	struct sdhci_pltfm_host *pltfm_host = NULL;
	int ret;

	host = sdhci_pltfm_init(pdev, &sdhci_xmedia_pdata,
			sizeof(struct sdhci_xmedia_priv));
	if (IS_ERR(host))
		return PTR_ERR(host);

	ret = sdhci_xmedia_pltfm_init(pdev, host);
	if (ret)
		goto err_sdhci_add;

	if (xmedia_support_runtime_pm(host)) {
		pm_runtime_get_noresume(&pdev->dev);
		pm_runtime_set_autosuspend_delay(&pdev->dev,
			XMEDIA_MMC_AUTOSUSPEND_DELAY_MS);
		pm_runtime_use_autosuspend(&pdev->dev);
		pm_runtime_set_active(&pdev->dev);
		pm_runtime_enable(&pdev->dev);
	}

	ret = sdhci_add_host(host);
	if (ret)
		goto pm_runtime_disable;

	if (xmedia_support_runtime_pm(host)) {
		pm_runtime_mark_last_busy(&pdev->dev);
		pm_runtime_put_autosuspend(&pdev->dev);
	}

	return 0;

pm_runtime_disable:
	if (xmedia_support_runtime_pm(host)) {
		pm_runtime_disable(&pdev->dev);
		pm_runtime_set_suspended(&pdev->dev);
		pm_runtime_put_noidle(&pdev->dev);
	}

err_sdhci_add:
	pltfm_host = sdhci_priv(host);
	clk_disable_unprepare(pltfm_host->clk);
	sdhci_pltfm_free(pdev);
	return ret;
}

static int sdhci_xmedia_remove(struct platform_device *pdev)
{
	struct sdhci_host *host = platform_get_drvdata(pdev);

	if (xmedia_support_runtime_pm(host)) {
		pm_runtime_get_sync(&pdev->dev);
		pm_runtime_disable(&pdev->dev);
		pm_runtime_put_noidle(&pdev->dev);
	}
	return sdhci_pltfm_unregister(pdev);
}

#ifdef CONFIG_PM
static int sdhci_xmedia_runtime_suspend(struct device *dev)
{
	struct sdhci_host *host = dev_get_drvdata(dev);

	xmedia_disable_card_clk(host);
	return 0;
}

static int sdhci_xmedia_runtime_resume(struct device *dev)
{
	struct sdhci_host *host = dev_get_drvdata(dev);

	xmedia_enable_card_clk(host);
	return 0;
}
#endif

static const struct of_device_id sdhci_xmedia_match[] = {
	{ .compatible = "xmedia,sdhci" },
	{ }
};
MODULE_DEVICE_TABLE(of, sdhci_xmedia_match);

static const struct dev_pm_ops sdhci_xmedia_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sdhci_pltfm_suspend,
				sdhci_pltfm_resume)

	SET_RUNTIME_PM_OPS(sdhci_xmedia_runtime_suspend,
			   sdhci_xmedia_runtime_resume,
			   NULL)
};

static struct platform_driver sdhci_xmedia_driver = {
	.driver		= {
		.name	= "sdhci-xmedia",
		.of_match_table = sdhci_xmedia_match,
		.pm	= &sdhci_xmedia_pm_ops,
	},
	.probe		= sdhci_xmedia_probe,
	.remove		= sdhci_xmedia_remove,
};

static int __init sdhci_xmedia_init(void)
{
	int ret;

	ret = platform_driver_register(&sdhci_xmedia_driver);
	if (ret)
		return ret;

	ret = mci_proc_init();
	if (ret)
		platform_driver_unregister(&sdhci_xmedia_driver);

	return ret;
}

static void __exit sdhci_xmedia_exit(void)
{
	mci_proc_shutdown();

	platform_driver_unregister(&sdhci_xmedia_driver);
}

module_init(sdhci_xmedia_init);
module_exit(sdhci_xmedia_exit);

MODULE_DESCRIPTION("SDHCI driver for xmedia");
MODULE_LICENSE("GPL v2");
