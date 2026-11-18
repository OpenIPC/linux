/*
 * Memory-mapped interface driver for DW SPI Core
 *
 * Copyright (c) 2010, Octasic semiconductor.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/scatterlist.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/property.h>
#include <linux/regmap.h>
#include <linux/reset.h>

#include "spi-axera.h"

#define DRIVER_NAME "dw_ahb_spi_mmio"

struct dw_spi_mmio {
	struct dw_spi  dws;
	struct clk     *clk;
	struct clk     *pclk;
	void           *priv;
	struct reset_control *rstc;
	struct reset_control *prstc;
};

#define MSCC_CPU_SYSTEM_CTRL_GENERAL_CTRL	0x24
#define OCELOT_IF_SI_OWNER_MASK			GENMASK(5, 4)
#define OCELOT_IF_SI_OWNER_OFFSET		4
#define MSCC_IF_SI_OWNER_SISL			0
#define MSCC_IF_SI_OWNER_SIBM			1
#define MSCC_IF_SI_OWNER_SIMC			2

#define MSCC_SPI_MST_SW_MODE			0x14
#define MSCC_SPI_MST_SW_MODE_SW_PIN_CTRL_MODE	BIT(13)
#define MSCC_SPI_MST_SW_MODE_SW_SPI_CS(x)	(x << 5)

static int dw_ahb_spi_mmio_probe(struct platform_device *pdev)
{
	int (*init_func)(struct platform_device *pdev,
			 struct dw_spi_mmio *dwsmmio);
	struct dw_spi_mmio *dwsmmio;
	struct dw_spi *dws;
	struct resource *mem;
	int ret;
	int num_cs;

	dwsmmio = devm_kzalloc(&pdev->dev, sizeof(struct dw_spi_mmio),
			GFP_KERNEL);
	if (!dwsmmio)
		return -ENOMEM;

	dws = &dwsmmio->dws;

	/* Get basic io resource and map it */
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dws->regs = devm_ioremap_resource(&pdev->dev, mem);
	dws->paddr = mem->start;

	if (IS_ERR(dws->regs)) {
		dev_err(&pdev->dev, "SPI region map failed\n");
		return PTR_ERR(dws->regs);
	}

	dws->irq = platform_get_irq(pdev, 0);
	if (dws->irq < 0) {
		dev_err(&pdev->dev, "no irq resource?\n");
		return dws->irq; /* -ENXIO */
	}

	dwsmmio->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(dwsmmio->clk))
		return PTR_ERR(dwsmmio->clk);
	ret = clk_prepare_enable(dwsmmio->clk);
	if (ret)
		return ret;

	/* Optional clock needed to access the registers */
	dwsmmio->pclk = devm_clk_get(&pdev->dev, "pclk");
	if (IS_ERR(dwsmmio->pclk)) {
		ret = PTR_ERR(dwsmmio->pclk);
		goto out_clk;
	}
	ret = clk_prepare_enable(dwsmmio->pclk);
	if (ret)
		goto out_clk;

	/* find an optional reset controller */
	dwsmmio->rstc = devm_reset_control_get_optional_exclusive(&pdev->dev, "rst");
	if (IS_ERR(dwsmmio->rstc)) {
		dev_err(&pdev->dev, "%s: dwsmmio->rstc failed\n",__func__);
		ret = PTR_ERR(dwsmmio->rstc);
		goto out_clk;
	}
	reset_control_deassert(dwsmmio->rstc);

	dwsmmio->prstc = devm_reset_control_get_optional_exclusive(&pdev->dev, "prst");
	if (IS_ERR(dwsmmio->prstc)) {
		dev_err(&pdev->dev, "%s: dwsmmio->prstc failed\n",__func__);
		ret = PTR_ERR(dwsmmio->prstc);
		goto out_clk;
	}
	reset_control_deassert(dwsmmio->prstc);

	dws->bus_num = pdev->id;

	dws->max_freq = clk_get_rate(dwsmmio->clk);

	device_property_read_u32(&pdev->dev, "reg-io-width", &dws->reg_io_width);

	num_cs = 4;

	device_property_read_u32(&pdev->dev, "num-cs", &num_cs);

	dws->num_cs = num_cs;

	if (pdev->dev.of_node) {
		int i;

		for (i = 0; i < dws->num_cs; i++) {
			int cs_gpio = of_get_named_gpio(pdev->dev.of_node,
					"cs-gpios", i);

			if (cs_gpio == -EPROBE_DEFER) {
				ret = cs_gpio;
				goto out;
			}

			if (gpio_is_valid(cs_gpio)) {
				ret = devm_gpio_request(&pdev->dev, cs_gpio,
						dev_name(&pdev->dev));
				if (ret)
					goto out;
			}
		}
	}

	init_func = device_get_match_data(&pdev->dev);
	if (init_func) {
		ret = init_func(pdev, dwsmmio);
		if (ret)
			goto out;
	}

	ret = dw_ahb_spi_add_host(&pdev->dev, dws);
	if (ret)
		goto out;

	platform_set_drvdata(pdev, dwsmmio);
	return 0;

out:
	clk_disable_unprepare(dwsmmio->pclk);
out_clk:
	clk_disable_unprepare(dwsmmio->clk);
	reset_control_assert(dwsmmio->rstc);
	reset_control_assert(dwsmmio->prstc);
	return ret;
}

static int dw_ahb_spi_mmio_remove(struct platform_device *pdev)
{
	struct dw_spi_mmio *dwsmmio = platform_get_drvdata(pdev);

	dw_ahb_spi_remove_host(&dwsmmio->dws);
	clk_disable_unprepare(dwsmmio->pclk);
	clk_disable_unprepare(dwsmmio->clk);
	reset_control_assert(dwsmmio->rstc);
	reset_control_assert(dwsmmio->prstc);

	return 0;
}

#ifdef CONFIG_PM

#ifdef CONFIG_PM_SLEEP
static int dw_ahb_spi_suspend(struct device *dev)
{
	int ret;
	struct platform_device *pdev = to_platform_device(dev);
	struct dw_spi_mmio *dwsmmio = platform_get_drvdata(pdev);

	ret = dw_ahb_spi_suspend_host(&dwsmmio->dws);

	if (__clk_is_enabled(dwsmmio->pclk))
		clk_disable_unprepare(dwsmmio->pclk);
	if (__clk_is_enabled(dwsmmio->clk))
		clk_disable_unprepare(dwsmmio->clk);

	return ret;
}

static int dw_ahb_spi_resume(struct device *dev)
{
	int ret;
	struct platform_device *pdev = to_platform_device(dev);
	struct dw_spi_mmio *dwsmmio = platform_get_drvdata(pdev);

	if (!__clk_is_enabled(dwsmmio->pclk)) {
		ret = clk_prepare_enable(dwsmmio->pclk);
		if (ret)
			pr_err("%s bus_num:%d set pclk failed\n", __func__, dwsmmio->dws.master->bus_num);
	}
	if (!__clk_is_enabled(dwsmmio->clk)) {
		ret = clk_prepare_enable(dwsmmio->clk);
		if (ret)
			pr_err("%s bus_num:%d set clk failed\n", __func__, dwsmmio->dws.master->bus_num);
	}

	return dw_ahb_spi_resume_host(&dwsmmio->dws);
}
#endif

static const struct dev_pm_ops dw_ahb_spi_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(dw_ahb_spi_suspend, dw_ahb_spi_resume)
};
#define DW_AHB_SPI_PM_OPS	(&dw_ahb_spi_pm_ops)
#else
#define DW_AHB_SPI_PM_OPS	NULL
#endif

static const struct of_device_id dw_ahb_spi_mmio_of_match[] = {
	{ .compatible = "snps,dwc-ssi-1.03a", },
	{ /* end of table */}
};
MODULE_DEVICE_TABLE(of, dw_ahb_spi_mmio_of_match);

static struct platform_driver dw_ahb_spi_mmio_driver = {
	.probe		= dw_ahb_spi_mmio_probe,
	.remove		= dw_ahb_spi_mmio_remove,
	.driver		= {
		.name	= DRIVER_NAME,
		.pm	= DW_AHB_SPI_PM_OPS,
		.of_match_table = dw_ahb_spi_mmio_of_match,
	},
};
module_platform_driver(dw_ahb_spi_mmio_driver);

MODULE_AUTHOR("Jean-Hugues Deschenes <jean-hugues.deschenes@octasic.com>");
MODULE_DESCRIPTION("Memory-mapped I/O interface driver for DW SPI Core");
MODULE_LICENSE("GPL v2");
