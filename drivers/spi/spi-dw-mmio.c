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

#include "spi-dw.h"

#define DRIVER_NAME "dw_spi_mmio"

// #define SPI_USING_CLK_FRAME

#define MSCC_CPU_SYSTEM_CTRL_GENERAL_CTRL	0x24
#define OCELOT_IF_SI_OWNER_MASK			GENMASK(5, 4)
#define OCELOT_IF_SI_OWNER_OFFSET		4
#define MSCC_IF_SI_OWNER_SISL			0
#define MSCC_IF_SI_OWNER_SIBM			1
#define MSCC_IF_SI_OWNER_SIMC			2

#define MSCC_SPI_MST_SW_MODE			0x14
#define MSCC_SPI_MST_SW_MODE_SW_PIN_CTRL_MODE	BIT(13)
#define MSCC_SPI_MST_SW_MODE_SW_SPI_CS(x)	(x << 5)

#define PERI_SYS_GLB_BASE        0x4870000
#define PERI_SYS_GLB_CLK_EB0     (0x04)
#define PERI_SYS_GLB_CLK_EB0_SET (0xB0)
#define PERI_SYS_GLB_CLK_EB0_CLR (0xB4)
#define PERI_SYS_GLB_CLK_EB3     (0x10)
#define PERI_SYS_GLB_CLK_EB3_SET (0xC8)
#define PERI_SYS_GLB_CLK_EB3_CLR (0xCC)

#define SPI0_DEV_NAME "6070000.spi"
#define SPI1_DEV_NAME "6071000.spi"
#define SPI2_DEV_NAME "6072000.spi"

extern unsigned int __clk_get_enable_count(struct clk *clk);
extern bool __clk_is_enabled(struct clk *clk);

static int get_spi_id(struct platform_device *pdev)
{
	u32 spi_id;
	if(pdev == NULL) {
		return -1;
	}
	if(strncmp(dev_name(&pdev->dev), SPI0_DEV_NAME, 11) == 0) {
		spi_id = 0;
	}
	else if(strncmp(dev_name(&pdev->dev), SPI1_DEV_NAME, 11) == 0) {
		spi_id = 1;
	}
	else if(strncmp(dev_name(&pdev->dev), SPI2_DEV_NAME, 11) == 0) {
		spi_id = 2;
	}
	else {
		spi_id = -1;
	}
	return spi_id;
}

int axera_spi_prepare_clk(struct dw_spi_mmio *dwsmmio, bool prepare, int spi_id)
{
#ifdef SPI_USING_CLK_FRAME
	int ret;
	if (prepare) {
		pr_debug("%s, %d, %d,  pclk:%d, clk:%d\n",__func__, __clk_get_enable_count(dwsmmio->pclk), __clk_get_enable_count(dwsmmio->clk), __clk_is_enabled(dwsmmio->pclk), __clk_is_enabled(dwsmmio->clk));
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
	} else {
		pr_debug("%s, %d, %d,  pclk:%d, clk:%d\n",__func__, __clk_get_enable_count(dwsmmio->pclk), __clk_get_enable_count(dwsmmio->clk), __clk_is_enabled(dwsmmio->pclk), __clk_is_enabled(dwsmmio->clk));
		if (__clk_is_enabled(dwsmmio->pclk))
			clk_disable_unprepare(dwsmmio->pclk);
		if (__clk_is_enabled(dwsmmio->clk))
			clk_disable_unprepare(dwsmmio->clk);
	}
#else
	pr_debug("%s prepare:%d\n",__func__, prepare);

	if (IS_ERR(dwsmmio->peri_sys_glb_base))
		return -1;

	if (prepare) {
		writel(BIT(6 + spi_id), dwsmmio->peri_sys_glb_base + PERI_SYS_GLB_CLK_EB0_SET);
		writel(BIT(2 + spi_id), dwsmmio->peri_sys_glb_base + PERI_SYS_GLB_CLK_EB3_SET);
	} else {
		writel(BIT(6 + spi_id), dwsmmio->peri_sys_glb_base + PERI_SYS_GLB_CLK_EB0_CLR);
		writel(BIT(2 + spi_id), dwsmmio->peri_sys_glb_base + PERI_SYS_GLB_CLK_EB3_CLR);
	}
#endif
	return 0;
}

static int dw_spi_mmio_probe(struct platform_device *pdev)
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
	dws->max_freq = 208000000;
	/* default chip design is open , now to close. */
	dwsmmio->spi_id = get_spi_id(pdev);
	dwsmmio->peri_sys_glb_base = ioremap(PERI_SYS_GLB_BASE, 0x100);
	if (!dwsmmio->peri_sys_glb_base) {
		pr_err("clk base ioremap failed.\n");
		return -EBUSY;
	}
	writel(BIT(6 + dwsmmio->spi_id), dwsmmio->peri_sys_glb_base + PERI_SYS_GLB_CLK_EB0_CLR);
	writel(BIT(2 + dwsmmio->spi_id), dwsmmio->peri_sys_glb_base + PERI_SYS_GLB_CLK_EB3_CLR);
#ifdef SPI_USING_CLK_FRAME
	dwsmmio->clk = devm_clk_get(&pdev->dev, "apb_ssi_clk");
	if (IS_ERR(dwsmmio->clk))
		return PTR_ERR(dwsmmio->clk);

	/* Optional clock needed to access the registers */
	dwsmmio->pclk = devm_clk_get(&pdev->dev, "pclk");
	if (IS_ERR(dwsmmio->pclk)) {
		return PTR_ERR(dwsmmio->pclk);
	}
	dws->max_freq = clk_get_rate(dwsmmio->clk);
#endif
	pr_info("%s: dws->max_freq:%d\n",__func__,dws->max_freq);
	dws->bus_num = pdev->id;
	dwsmmio->spi_id = get_spi_id(pdev);

	/* before reset to close clk. */
	axera_spi_prepare_clk(dwsmmio, false, dwsmmio->spi_id);

	/* find an optional reset controller */
	dwsmmio->rstc = devm_reset_control_get_optional_exclusive(&pdev->dev, "rst");
	if (IS_ERR(dwsmmio->rstc)) {
		dev_err(&pdev->dev, "%s: dwsmmio->rstc failed\n",__func__);
		return PTR_ERR(dwsmmio->rstc);
	}
	reset_control_deassert(dwsmmio->rstc);
	dwsmmio->prstc = devm_reset_control_get_optional_exclusive(&pdev->dev, "prst");
	if (IS_ERR(dwsmmio->prstc)) {
		dev_err(&pdev->dev, "%s: dwsmmio->prstc failed\n",__func__);
		return PTR_ERR(dwsmmio->prstc);
	}
	reset_control_deassert(dwsmmio->prstc);

	axera_spi_prepare_clk(dwsmmio, true, dwsmmio->spi_id);
	pr_info("%s: get_spi_id :%d\n",__func__,get_spi_id(pdev));

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

	ret = dw_spi_add_host(&pdev->dev, dws);
	if (ret)
		goto out;

	platform_set_drvdata(pdev, dwsmmio);
	return 0;

out:
	axera_spi_prepare_clk(dwsmmio, false, dwsmmio->spi_id);
	reset_control_assert(dwsmmio->rstc);
	reset_control_assert(dwsmmio->prstc);
	if (dwsmmio->peri_sys_glb_base)
		iounmap(dwsmmio->peri_sys_glb_base);
	return ret;
}

static int dw_spi_mmio_remove(struct platform_device *pdev)
{
	struct dw_spi_mmio *dwsmmio = platform_get_drvdata(pdev);

	dw_spi_remove_host(&dwsmmio->dws);
	axera_spi_prepare_clk(dwsmmio, false, dwsmmio->spi_id);
	reset_control_assert(dwsmmio->rstc);
	reset_control_assert(dwsmmio->prstc);
	if (dwsmmio->peri_sys_glb_base)
		iounmap(dwsmmio->peri_sys_glb_base);

	return 0;
}

#ifdef CONFIG_PM

#ifdef CONFIG_PM_SLEEP
int dw_spi_suspend(struct device *dev)
{
	int ret;
	struct platform_device *pdev = to_platform_device(dev);
	struct dw_spi_mmio *dwsmmio = platform_get_drvdata(pdev);

	ret = dw_spi_suspend_host(&dwsmmio->dws);
	axera_spi_prepare_clk(dwsmmio, false, dwsmmio->spi_id);
	return ret;
}

int dw_spi_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct dw_spi_mmio *dwsmmio = platform_get_drvdata(pdev);
	axera_spi_prepare_clk(dwsmmio, true, dwsmmio->spi_id);
	return dw_spi_resume_host(&dwsmmio->dws);
}
#endif
static const struct dev_pm_ops dw_spi_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(dw_spi_suspend, dw_spi_resume)
};
#define DW_SPI_PM_OPS	(&dw_spi_pm_ops)
#else
#define DW_SPI_PM_OPS	NULL
#endif
static const struct of_device_id dw_spi_mmio_of_match[] = {
	{ .compatible = "snps,dw-apb-ssi", },
	{ /* end of table */}
};
MODULE_DEVICE_TABLE(of, dw_spi_mmio_of_match);
static struct platform_driver dw_spi_mmio_driver = {
	.probe		= dw_spi_mmio_probe,
	.remove		= dw_spi_mmio_remove,
	.driver		= {
		.name	= DRIVER_NAME,
		.pm	= DW_SPI_PM_OPS,
		.of_match_table = dw_spi_mmio_of_match,
	},
};
module_platform_driver(dw_spi_mmio_driver);

MODULE_AUTHOR("Jean-Hugues Deschenes <jean-hugues.deschenes@octasic.com>");
MODULE_DESCRIPTION("Memory-mapped I/O interface driver for DW SPI Core");
MODULE_LICENSE("GPL v2");
