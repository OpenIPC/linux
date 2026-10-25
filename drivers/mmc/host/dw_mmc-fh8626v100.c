// SPDX-License-Identifier: GPL-2.0
/*
 * FH8626V100 integration of the Synopsys MSHC controller.
 * The controller is reset through the PMU before use.
 * Copyright (C) 2026 Difan Zhang
 */
#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/soc/fullhan/fh8626-pmu.h>

#include "dw_mmc.h"
#include "dw_mmc-pltfm.h"

#define FH8626_PMU_SDC0_RESET_BIT	12

/*
 * SDC0 card clock phase select in the PMU. The drive clock is delayed by
 * two steps and the sample clock is not shifted; this is the setting the
 * controller is characterised with at 50 MHz in 1-bit high-speed mode.
 */
#define FH8626_PMU_SDC_PHASE		0x28
#define FH8626_SDC0_SAMPLE_PHASE	GENMASK(10, 9)
#define FH8626_SDC0_DRIVE_PHASE		GENMASK(8, 7)
#define FH8626_SDC0_SAMPLE_STEP		0
#define FH8626_SDC0_DRIVE_STEP		2

static int dw_mci_fh8626_init(struct dw_mci *host)
{
	int ret;

	if (IS_ERR(host->biu_clk))
		return PTR_ERR(host->biu_clk);
	if (IS_ERR(host->ciu_clk))
		return PTR_ERR(host->ciu_clk);

	ret = fh8626_pmu_reset(FH8626_PMU_SDC0_RESET_BIT);
	if (ret)
		return ret;

	return fh8626_pmu_update_bits(FH8626_PMU_SDC_PHASE,
			FH8626_SDC0_SAMPLE_PHASE | FH8626_SDC0_DRIVE_PHASE,
			FIELD_PREP(FH8626_SDC0_SAMPLE_PHASE,
				   FH8626_SDC0_SAMPLE_STEP) |
			FIELD_PREP(FH8626_SDC0_DRIVE_PHASE,
				   FH8626_SDC0_DRIVE_STEP));
}

static const struct dw_mci_drv_data fh8626_drv_data = {
	.init = dw_mci_fh8626_init,
};

static int fh_mci_probe(struct platform_device *pdev)
{
	struct clk *clk;
	int ret;

	/* Propagate clock deferral before dw_mci touches the hardware. */
	clk = devm_clk_get(&pdev->dev, "biu");
	if (IS_ERR(clk))
		return PTR_ERR(clk);
	clk = devm_clk_get(&pdev->dev, "ciu");
	if (IS_ERR(clk))
		return PTR_ERR(clk);
	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	if (ret)
		return ret;
	return dw_mci_pltfm_register(pdev, &fh8626_drv_data);
}

#ifdef CONFIG_PM_SLEEP
static int fh_mci_suspend(struct device *dev)
{
	return pm_runtime_force_suspend(dev);
}

static int fh_mci_resume(struct device *dev)
{
	return pm_runtime_force_resume(dev);
}
#endif

static const struct dev_pm_ops fh_mci_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(fh_mci_suspend, fh_mci_resume)
	SET_RUNTIME_PM_OPS(dw_mci_runtime_suspend,
			   dw_mci_runtime_resume,
			   NULL)
};

static const struct of_device_id fh_mci_match[] = {
	{ .compatible = "fullhan,fh8626v100-mmc" },
	{ }
};
MODULE_DEVICE_TABLE(of, fh_mci_match);

static struct platform_driver fh_mci_driver = {
	.probe = fh_mci_probe,
	.remove = dw_mci_pltfm_remove,
	.driver = {
		.name = "dwmmc_fh8626v100",
		.of_match_table = fh_mci_match,
		.pm = &fh_mci_pm_ops,
	},
};
module_platform_driver(fh_mci_driver);

MODULE_DESCRIPTION("Fullhan FH8626V100 DesignWare MSHC glue");
MODULE_LICENSE("GPL v2");
