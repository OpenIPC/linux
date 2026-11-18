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

#include "spi-axera-slv.h"

#define DRIVER_NAME "dw_slv_spi_mmio"

// #define SPI_SLV_USING_CLK_FRAME

#define MSCC_CPU_SYSTEM_CTRL_GENERAL_CTRL	0x24
#define OCELOT_IF_SI_OWNER_MASK			GENMASK(5, 4)
#define OCELOT_IF_SI_OWNER_OFFSET		4
#define MSCC_IF_SI_OWNER_SISL			0
#define MSCC_IF_SI_OWNER_SIBM			1
#define MSCC_IF_SI_OWNER_SIMC			2

#define MSCC_SPI_MST_SW_MODE			0x14
#define MSCC_SPI_MST_SW_MODE_SW_PIN_CTRL_MODE	BIT(13)
#define MSCC_SPI_MST_SW_MODE_SW_SPI_CS(x)	(x << 5)

#define FLASH_SYS_GLB_BASE        0x10030000
#define FLASH_SYS_GLB_CLK_EB0     (0x04)
#define FLASH_SYS_GLB_CLK_EB0_SET (0x4004)
#define FLASH_SYS_GLB_CLK_EB0_CLR (0x8004)
#define FLASH_SYS_GLB_CLK_EB1     (0x8)
#define FLASH_SYS_GLB_CLK_EB1_SET (0x4008)
#define FLASH_SYS_GLB_CLK_EB1_CLR (0x8008)

struct dw_spi_mmio {
	struct dw_spi  dws;
#ifdef SPI_SLV_USING_CLK_FRAME
	struct clk     *clk;
	struct clk     *hclk;
#endif
	void           *priv;
	struct reset_control *rstc;
	struct reset_control *hrstc;
};

extern unsigned int __clk_get_enable_count(struct clk *clk);
extern bool __clk_is_enabled(struct clk *clk);

struct dw_spi_mscc {
	struct regmap       *syscon;
	void __iomem        *spi_mst;
};

/*
 * The Designware SPI controller (referred to as master in the documentation)
 * automatically deasserts chip select when the tx fifo is empty. The chip
 * selects then needs to be either driven as GPIOs or, for the first 4 using the
 * the SPI boot controller registers. the final chip select is an OR gate
 * between the Designware SPI controller and the SPI boot controller.
 */
static void dw_spi_mscc_set_cs(struct spi_device *spi, bool enable)
{
	struct dw_spi *dws = spi_master_get_devdata(spi->master);
	struct dw_spi_mmio *dwsmmio = container_of(dws, struct dw_spi_mmio, dws);
	struct dw_spi_mscc *dwsmscc = dwsmmio->priv;
	u32 cs = spi->chip_select;

	if (cs < 4) {
		u32 sw_mode = MSCC_SPI_MST_SW_MODE_SW_PIN_CTRL_MODE;

		if (!enable)
			sw_mode |= MSCC_SPI_MST_SW_MODE_SW_SPI_CS(BIT(cs));

		writel(sw_mode, dwsmscc->spi_mst + MSCC_SPI_MST_SW_MODE);
	}

	dw_slv_spi_set_cs(spi, enable);
}

static int dw_spi_mscc_init(struct platform_device *pdev,
			    struct dw_spi_mmio *dwsmmio)
{
	struct dw_spi_mscc *dwsmscc;
	struct resource *res;

	dwsmscc = devm_kzalloc(&pdev->dev, sizeof(*dwsmscc), GFP_KERNEL);
	if (!dwsmscc)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	dwsmscc->spi_mst = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(dwsmscc->spi_mst)) {
		dev_err(&pdev->dev, "SPI_MST region map failed\n");
		return PTR_ERR(dwsmscc->spi_mst);
	}

	dwsmscc->syscon = syscon_regmap_lookup_by_compatible("mscc,ocelot-cpu-syscon");
	if (IS_ERR(dwsmscc->syscon))
		return PTR_ERR(dwsmscc->syscon);

	/* Deassert all CS */
	writel(0, dwsmscc->spi_mst + MSCC_SPI_MST_SW_MODE);

	/* Select the owner of the SI interface */
	regmap_update_bits(dwsmscc->syscon, MSCC_CPU_SYSTEM_CTRL_GENERAL_CTRL,
			   OCELOT_IF_SI_OWNER_MASK,
			   MSCC_IF_SI_OWNER_SIMC << OCELOT_IF_SI_OWNER_OFFSET);

	dwsmmio->dws.set_cs = dw_spi_mscc_set_cs;
	dwsmmio->priv = dwsmscc;

	return 0;
}

int axera_spi_slv_prepare_clk(struct dw_spi_mmio *dwsmmio, bool prepare)
{
#ifdef SPI_SLV_USING_CLK_FRAME
	int ret;
	if (prepare) {
		pr_debug("%s,[enabled]==> %d, %d,  hclk:%d, clk:%d\n",__func__, __clk_get_enable_count(dwsmmio->hclk), __clk_get_enable_count(dwsmmio->clk), __clk_is_enabled(dwsmmio->hclk), __clk_is_enabled(dwsmmio->clk));
		if (!__clk_is_enabled(dwsmmio->hclk)) {
			ret = clk_prepare_enable(dwsmmio->hclk);
			if (ret)
				pr_err("%s bus_num:%d set hclk failed\n", __func__, dwsmmio->dws.master->bus_num);
		}
		if (!__clk_is_enabled(dwsmmio->clk)) {
			ret = clk_prepare_enable(dwsmmio->clk);
			if (ret)
				pr_err("%s bus_num:%d set clk failed\n", __func__, dwsmmio->dws.master->bus_num);
		}
	} else {
		pr_debug("%s,[disabled]==> %d, %d,  hclk:%d, clk:%d\n",__func__, __clk_get_enable_count(dwsmmio->hclk), __clk_get_enable_count(dwsmmio->clk), __clk_is_enabled(dwsmmio->hclk), __clk_is_enabled(dwsmmio->clk));
		if (__clk_is_enabled(dwsmmio->hclk))
			clk_disable_unprepare(dwsmmio->hclk);
		if (__clk_is_enabled(dwsmmio->clk))
			clk_disable_unprepare(dwsmmio->clk);
	}
#else
	void __iomem *addr = NULL;
	addr = ioremap(FLASH_SYS_GLB_BASE, 0x8010);
	pr_debug("%s prepare:%d\n",__func__, prepare);
	if (prepare) {
		writel(BIT(11), addr + FLASH_SYS_GLB_CLK_EB0_SET);
		writel(BIT(12), addr + FLASH_SYS_GLB_CLK_EB1_SET);
	} else {
		writel(BIT(11), addr + FLASH_SYS_GLB_CLK_EB0_CLR);
		writel(BIT(12), addr + FLASH_SYS_GLB_CLK_EB1_CLR);
	}
	iounmap(addr);
#endif
	return 0;
}
static int dw_slv_spi_mmio_probe(struct platform_device *pdev)
{
	int (*init_func)(struct platform_device *pdev,
			 struct dw_spi_mmio *dwsmmio);
	struct dw_spi_mmio *dwsmmio;
	struct dw_spi *dws;
	struct resource *mem;
	int ret;
	int num_cs;
	void __iomem *addr = NULL;

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
	dws->paddr = mem->start;

	dws->irq = platform_get_irq(pdev, 0);
	if (dws->irq < 0) {
		dev_err(&pdev->dev, "no irq resource?\n");
		return dws->irq; /* -ENXIO */
	}
	dws->max_freq = 208000000;
	/* default chip design is open , now to close. */
	addr = ioremap(FLASH_SYS_GLB_BASE, 0x8010);
	writel(BIT(11), addr + FLASH_SYS_GLB_CLK_EB0_CLR);
	writel(BIT(12), addr + FLASH_SYS_GLB_CLK_EB1_CLR);
	iounmap(addr);

#ifdef SPI_SLV_USING_CLK_FRAME
	dwsmmio->hclk = devm_clk_get(&pdev->dev, "hclk");
	if (IS_ERR(dwsmmio->hclk)) {
		pr_err("%s: dwsmmio->hclk get fail\n", __FUNCTION__);
		return PTR_ERR(dwsmmio->hclk);
	}
	dwsmmio->clk = devm_clk_get(&pdev->dev, "ahb_ssi_s_clk");
	if (IS_ERR(dwsmmio->clk)) {
		pr_err("%s: dwsmmio->clk get fail\n", __FUNCTION__);
		return PTR_ERR(dwsmmio->clk);
	}
	dws->max_freq = clk_get_rate(dwsmmio->clk);
#endif
	pr_info("%s: dws->max_freq:%d\n",__func__,dws->max_freq);
	/* find an optional reset controller */
	dwsmmio->rstc = devm_reset_control_get_optional_exclusive(&pdev->dev, "rst");
	if (IS_ERR(dwsmmio->rstc)) {
		dev_err(&pdev->dev, "%s %d : dwsmmio->rstc\n", __func__,__LINE__);
		return PTR_ERR(dwsmmio->rstc);
	}
	reset_control_deassert(dwsmmio->rstc);

	dwsmmio->hrstc = devm_reset_control_get_optional_exclusive(&pdev->dev, "hrst");
	if (IS_ERR(dwsmmio->hrstc)) {
		dev_err(&pdev->dev, "%s %d : dwsmmio->hrstc\n", __func__,__LINE__);
		return PTR_ERR(dwsmmio->hrstc);
	}
	reset_control_deassert(dwsmmio->hrstc);

	dws->bus_num = pdev->id;
	axera_spi_slv_prepare_clk(dwsmmio, true);

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

	ret = dw_slv_spi_add_host(&pdev->dev, dws);
	if (ret)
		goto out;

	platform_set_drvdata(pdev, dwsmmio);
	pr_info("%s: successful\n",__func__);
	return 0;

out:
	axera_spi_slv_prepare_clk(dwsmmio, false);
	reset_control_assert(dwsmmio->hrstc);
	reset_control_assert(dwsmmio->rstc);
	return ret;
}

static int dw_slv_spi_mmio_remove(struct platform_device *pdev)
{
	struct dw_spi_mmio *dwsmmio = platform_get_drvdata(pdev);

	dw_slv_spi_remove_host(&dwsmmio->dws);
	axera_spi_slv_prepare_clk(dwsmmio, false);
	reset_control_assert(dwsmmio->hrstc);
	reset_control_assert(dwsmmio->rstc);

	return 0;
}

#ifdef CONFIG_PM

#ifdef CONFIG_PM_SLEEP
static int dw_ahb_spi_suspend(struct device *dev)
{
	int ret;
	struct platform_device *pdev = to_platform_device(dev);
	struct dw_spi_mmio *dwsmmio = platform_get_drvdata(pdev);
	ret = dw_slv_spi_suspend_host(&dwsmmio->dws);
	axera_spi_slv_prepare_clk(dwsmmio, false);
	reset_control_assert(dwsmmio->hrstc);
	reset_control_assert(dwsmmio->rstc);
	return ret;
}

static int dw_ahb_spi_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct dw_spi_mmio *dwsmmio = platform_get_drvdata(pdev);
	reset_control_deassert(dwsmmio->hrstc);
	reset_control_deassert(dwsmmio->rstc);
	axera_spi_slv_prepare_clk(dwsmmio, true);
	return dw_slv_spi_resume_host(&dwsmmio->dws);
}
#endif

static const struct dev_pm_ops dw_ahb_spi_slv_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(dw_ahb_spi_suspend, dw_ahb_spi_resume)
};
#define DW_AHB_SPI_SLV_PM_OPS	(&dw_ahb_spi_slv_pm_ops)
#else
#define DW_AHB_SPI_SLV_PM_OPS	NULL
#endif
static const struct of_device_id dw_slv_spi_mmio_of_match[] = {
	{ .compatible = "snps,dw-slv-ssi", },
	{ .compatible = "snps,dwc-ssi-slv-1.03a", },
	{ .compatible = "mscc,ocelot-spi", .data = dw_spi_mscc_init},
	{ /* end of table */}
};
MODULE_DEVICE_TABLE(of, dw_slv_spi_mmio_of_match);

static struct platform_driver dw_slv_spi_mmio_driver = {
	.probe		= dw_slv_spi_mmio_probe,
	.remove		= dw_slv_spi_mmio_remove,
	.driver		= {
		.name	= DRIVER_NAME,
		.pm	= DW_AHB_SPI_SLV_PM_OPS,
		.of_match_table = dw_slv_spi_mmio_of_match,
	},
};
module_platform_driver(dw_slv_spi_mmio_driver);

MODULE_AUTHOR("Jean-Hugues Deschenes <jean-hugues.deschenes@octasic.com>");
MODULE_DESCRIPTION("Memory-mapped I/O interface driver for DW SPI Core");
MODULE_LICENSE("GPL v2");
