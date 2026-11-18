/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>
#include "core.h"
#include "dwc3-axera.h"
#include "io.h"

struct dwc3_of_simple {
	struct device	*dev;
	struct clk	*bus_clk;
	struct clk	*ref_clk;
	struct clk	*ref_alt_clk;
	struct reset_control	*rst;
	struct reset_control	*prst;
};

//#define AXERA_CLK_AND_RST_EN

/* clock and reset */
#define FLASH_SYS_GLB_BASE_ADDR             0x10030000
#define FLASH_SYS_GLB_CLK_MUX0_SET          (FLASH_SYS_GLB_BASE_ADDR + 0x4000)
#define FLASH_SYS_GLB_CLK_MUX0_CLR          (FLASH_SYS_GLB_BASE_ADDR + 0x8000)
#define FLASH_SYS_GLB_CLK_EB0_SET           (FLASH_SYS_GLB_BASE_ADDR + 0x4004)
#define FLASH_SYS_GLB_CLK_EB0_CLR           (FLASH_SYS_GLB_BASE_ADDR + 0x8004)
#define FLASH_SYS_GLB_CLK_EB1_SET           (FLASH_SYS_GLB_BASE_ADDR + 0x4008)
#define FLASH_SYS_GLB_CLK_EB1_CLR           (FLASH_SYS_GLB_BASE_ADDR + 0x8008)
#define FLASH_SYS_GLB_SW_RST0_SET           (FLASH_SYS_GLB_BASE_ADDR + 0x4014)
#define FLASH_SYS_GLB_SW_RST0_CLR           (FLASH_SYS_GLB_BASE_ADDR + 0x8014)
#define FLASH_SYS_GLB_USB2_CTRL_SET         (FLASH_SYS_GLB_BASE_ADDR + 0x4040)
#define FLASH_SYS_GLB_USB2_CTRL_CLR         (FLASH_SYS_GLB_BASE_ADDR + 0x8040)

#define USB2_PHY_SW_RST         24
#define USB2_VCC_SW_RST         25
#define CLK_USB2_REF_EB         12
#define CLK_USB2_REF_ALT_CLK_EB 14
#define CLK_BUS_CLK_USB2_EB      5
#define VBUSVALID                6

#define DEVICE_MODE 1
#define HOST_MODE   0

/* bus clock freq */
#define BUS_CLK_24M     24000000
#define BUS_CLK_100M    100000000
#define BUS_CLK_156M    156000000
#define BUS_CLK_208M    208000000
#define BUS_CLK_250M    250000000
#define BUS_CLK_312M    312000000

#define CURRENT_BUS_CLK    BUS_CLK_312M

/* ref clk adj value */
#define GFLADJ_REFCLK_240MHZ_DECR   0xa
#define GFLADJ_REFCLK_FLADJ         0x7f0
#define GUCTL_REFCLKPER             0x29

static void usb_sw_rst_init(void)
{
	void __iomem *addr;

	addr = ioremap(FLASH_SYS_GLB_SW_RST0_SET, 4);
	writel(BIT(USB2_PHY_SW_RST), addr);
	writel(BIT(USB2_VCC_SW_RST), addr);
	iounmap(addr);

	addr = ioremap(FLASH_SYS_GLB_SW_RST0_CLR, 4);
	writel(BIT(USB2_PHY_SW_RST), addr);
	writel(BIT(USB2_VCC_SW_RST), addr);
	iounmap(addr);
}

static void usb_sw_rst(int flag)
{
	void __iomem *addr;

	if (flag == 1) {
		addr = ioremap(FLASH_SYS_GLB_SW_RST0_SET, 4);
		writel(BIT(USB2_PHY_SW_RST), addr);
		writel(BIT(USB2_VCC_SW_RST), addr);
		iounmap(addr);
	} else if (flag == 0) {
		addr = ioremap(FLASH_SYS_GLB_SW_RST0_CLR, 4);
		writel(BIT(USB2_PHY_SW_RST), addr);
		writel(BIT(USB2_VCC_SW_RST), addr);
		iounmap(addr);
	} else {
		printk("usb sw rst flag invalid!\n");
	}
}

static void usb_clk_init(void)
{
	void __iomem *addr;

	addr = ioremap(FLASH_SYS_GLB_CLK_EB0_SET, 4);
	writel(BIT(CLK_USB2_REF_EB), addr);
	writel(BIT(CLK_USB2_REF_ALT_CLK_EB), addr);
	iounmap(addr);

	addr = ioremap(FLASH_SYS_GLB_CLK_EB1_SET, 4);
	writel(BIT(CLK_BUS_CLK_USB2_EB), addr);
	iounmap(addr);

	/* bus_clk freq (bit6..8):
	 * 000:24M,
	 * 001:200M,
	 * 010:156M,
	 * 011:208M,
	 * 100:250M,
	 * 101:312M */
	addr = ioremap(FLASH_SYS_GLB_CLK_MUX0_CLR, 4);
	writel(BIT(6), addr);
	writel(BIT(7), addr);
	writel(BIT(8), addr);
	iounmap(addr);
	addr = ioremap(FLASH_SYS_GLB_CLK_MUX0_SET, 4);
	writel(BIT(6), addr);
	writel(BIT(8), addr);
	iounmap(addr);
}

static void usb_clk_enable(void)
{
	void __iomem *addr;

	addr = ioremap(FLASH_SYS_GLB_CLK_EB0_SET, 4);
	writel(BIT(CLK_USB2_REF_EB), addr);
	writel(BIT(CLK_USB2_REF_ALT_CLK_EB), addr);
	iounmap(addr);

	addr = ioremap(FLASH_SYS_GLB_CLK_EB1_SET, 4);
	writel(BIT(CLK_BUS_CLK_USB2_EB), addr);
	iounmap(addr);
}

static void usb_clk_disable(void)
{
	void __iomem *addr;

	addr = ioremap(FLASH_SYS_GLB_CLK_EB0_CLR, 4);
	writel(BIT(CLK_USB2_REF_EB), addr);
	writel(BIT(CLK_USB2_REF_ALT_CLK_EB), addr);
	iounmap(addr);

	addr = ioremap(FLASH_SYS_GLB_CLK_EB1_CLR, 4);
	writel(BIT(CLK_BUS_CLK_USB2_EB), addr);
	iounmap(addr);
}

static void usb_vbus_init(struct dwc3 *dwc, char usb_mode)
{
	void __iomem *addr;

	if (usb_mode == DEVICE_MODE)
		addr = ioremap(FLASH_SYS_GLB_USB2_CTRL_SET, 4);
	else
		addr = ioremap(FLASH_SYS_GLB_USB2_CTRL_CLR, 4);

	writel(BIT(VBUSVALID), addr);
	iounmap(addr);
}

#ifdef AXERA_CLK_AND_RST_EN
static int axera_usb_sw_rst_init(struct dwc3_of_simple *simple)
{
	simple->rst = devm_reset_control_get_optional_exclusive(simple->dev, NULL);
	if (IS_ERR(simple->rst)) {
		dev_err(simple->dev, "Cannot get rst descriptor!\n");
		goto err_reset;
	}

	simple->prst = devm_reset_control_get_optional_exclusive(simple->dev, "preset");
	if (IS_ERR(simple->prst)) {
		dev_err(simple->dev, "Cannot get preset descriptor!\n");
		goto err_reset;
	}

	return 0;

err_reset:
	reset_control_assert(simple->rst);

	return -1;
}

static int axera_usb_sw_rst(struct dwc3_of_simple *simple, int flag)
{
	if (flag == 1) {
		reset_control_assert(simple->rst);
		reset_control_assert(simple->prst);
	} else if (flag == 0) {
		reset_control_deassert(simple->rst);
		reset_control_deassert(simple->prst);
	} else {
		dev_err(simple->dev, "flag invalid!\n");
		return -EINVAL;
	}

	return 0;
}

static int axera_usb_clk_init(struct dwc3_of_simple *simple)
{
	int ret = -1;

	simple->bus_clk = devm_clk_get(simple->dev, "bus_clk");
	if (IS_ERR(simple->bus_clk)) {
		dev_err(simple->dev, "can not get usb bus clk.\n");
		goto err_bus_clk;
	}
	clk_prepare_enable(simple->bus_clk);

	ret = clk_set_rate(simple->bus_clk, CURRENT_BUS_CLK);
	if (ret) {
		dev_err(simple->dev, "can not set usb bus clk rate.\n");
		goto err_bus_clk;
	}
	dev_dbg(simple->dev, "usb bus clock: %ldMHz\n", clk_get_rate(simple->bus_clk) / 1000000);

	simple->ref_clk = devm_clk_get(simple->dev, "ref_clk");
	if (IS_ERR(simple->ref_clk)) {
		dev_err(simple->dev, "can not get usb ref clk.\n");
		goto err_ref_clk;
	}
	clk_prepare_enable(simple->ref_clk);

	simple->ref_alt_clk = devm_clk_get(simple->dev, "ref_alt_clk");
	if (IS_ERR(simple->ref_alt_clk)) {
		dev_err(simple->dev, "can not get usb ref alt clk.\n");
		goto err_ref_alt_clk;
	}
	clk_prepare_enable(simple->ref_alt_clk);

	dev_dbg(simple->dev, "axera usb clk set succeed.\n");

	return 0;

err_ref_alt_clk:
	clk_disable_unprepare(simple->ref_alt_clk);

err_ref_clk:
	clk_disable_unprepare(simple->ref_clk);

err_bus_clk:
	clk_disable_unprepare(simple->bus_clk);

	return ret;
}
#endif

#ifdef CONFIG_TYPEC_SGM7220
struct list_head dwc3_list;

void put_dwc3_into_list(struct dwc3 *dwc)
{
	struct dwc3_dev_list *list;
	static int is_head_init;

	list = (struct dwc3_dev_list *)kmalloc(sizeof(struct dwc3_dev_list), GFP_KERNEL);
	list->dwc = dwc;

	if (!is_head_init) {
		INIT_LIST_HEAD(&dwc3_list);
		is_head_init = 1;
	}
	list_add_tail(&list->list_node, &dwc3_list);
}

struct list_head *get_dwc3_list(void)
{
	return &dwc3_list;
}
#endif

static void axera_usb_adj_ref_clk(struct dwc3 *dwc)
{
	u32 value;

	//dev_info(dwc->dev,"axera_usb_adj_ref_clk\n");

	/* DWC3_GFLADJ: 0xc630 */
	value = dwc3_readl(dwc->regs, DWC3_GFLADJ);
	value &= ~(DWC3_GFLADJ_REFCLK_FLADJ_MASK << 8);
	value &= ~(DWC3_GFLADJ_REFCLK_240MHZ_DECR_MASK << 24);
	value |= GFLADJ_REFCLK_240MHZ_DECR << 24 |
			 DWC3_GFLADJ_REFCLK_LPM_SEL 	|
			 GFLADJ_REFCLK_FLADJ << 8;
	dwc3_writel(dwc->regs, DWC3_GFLADJ, value);

	/* DWC3_GUCTL: 0xc12c */
	value = dwc3_readl(dwc->regs, DWC3_GUCTL);
	value &= ~(DWC3_GUCTL_REFCLKPER_MASK << 22);
	value |= GUCTL_REFCLKPER << 22;
	dwc3_writel(dwc->regs, DWC3_GUCTL, value);
}

void axera_usb_global_init(struct dwc3 *dwc)
{
#ifdef CONFIG_TYPEC_SGM7220
	put_dwc3_into_list(dwc);
#endif
	axera_usb_adj_ref_clk(dwc);
}
EXPORT_SYMBOL(axera_usb_global_init);

void axera_usb_host_init(struct dwc3 *dwc)
{
	dev_info(dwc->dev, "axera usb host init\n");
	usb_vbus_init(dwc, HOST_MODE);
}
EXPORT_SYMBOL(axera_usb_host_init);

void axera_usb_device_init(struct dwc3 *dwc)
{
	dev_info(dwc->dev, "axera usb gadget init\n");
	usb_vbus_init(dwc, DEVICE_MODE);
}
EXPORT_SYMBOL(axera_usb_device_init);

static int dwc3_of_simple_probe(struct platform_device *pdev)
{
	struct dwc3_of_simple	*simple;
	struct device		*dev = &pdev->dev;
	struct device_node	*np = dev->of_node;

	int	ret;

	dev_info(dev, "axera usb probe\n");

	simple = devm_kzalloc(dev, sizeof(*simple), GFP_KERNEL);
	if (!simple)
		return -ENOMEM;

	platform_set_drvdata(pdev, simple);
	simple->dev = dev;

#ifndef AXERA_CLK_AND_RST_EN
	usb_clk_init();
#else
	ret = axera_usb_clk_init(simple);
	if (ret) {
		dev_err(dev, "axera usb clk init failed!\n");
		return ret;
	}
#endif

#ifndef AXERA_CLK_AND_RST_EN
	usb_sw_rst_init();
#else
	ret = axera_usb_sw_rst_init(simple);
	if (ret) {
		dev_err(dev, "axera usb reset failed!\n");
		goto err_rst;
	}
	axera_usb_sw_rst(simple, 1);
	axera_usb_sw_rst(simple, 0);
#endif

	ret = of_platform_populate(np, NULL, NULL, dev);
	if (ret) {
		dev_err(dev, "failed to add dwc3 core\n");
#ifdef AXERA_CLK_AND_RST_EN
		goto err_clk;
#else
		return ret;
#endif
	}

	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);
	pm_runtime_get_sync(dev);

	return 0;

#ifdef AXERA_CLK_AND_RST_EN
err_clk:
	clk_disable_unprepare(simple->ref_alt_clk);
	clk_disable_unprepare(simple->ref_clk);
	clk_disable_unprepare(simple->bus_clk);

err_rst:
	reset_control_assert(simple->prst);
	reset_control_assert(simple->rst);
#endif

	return ret;

}

static void __dwc3_of_simple_teardown(struct dwc3_of_simple *simple)
{
	of_platform_depopulate(simple->dev);
#ifdef AXERA_CLK_AND_RST_EN
	axera_usb_sw_rst(simple, 1);

	clk_disable_unprepare(simple->ref_alt_clk);
	clk_disable_unprepare(simple->ref_clk);
	clk_disable_unprepare(simple->bus_clk);
#endif
	pm_runtime_disable(simple->dev);
	pm_runtime_put_noidle(simple->dev);
	pm_runtime_set_suspended(simple->dev);
}

static int dwc3_of_simple_remove(struct platform_device *pdev)
{
	struct dwc3_of_simple	*simple = platform_get_drvdata(pdev);

	__dwc3_of_simple_teardown(simple);

	return 0;
}

static void dwc3_of_simple_shutdown(struct platform_device *pdev)
{
	struct dwc3_of_simple	*simple = platform_get_drvdata(pdev);

	__dwc3_of_simple_teardown(simple);
}

static int __maybe_unused dwc3_of_simple_runtime_suspend(struct device *dev)
{
#ifdef AXERA_CLK_AND_RST_EN
	struct dwc3_of_simple	*simple = dev_get_drvdata(dev);

	clk_disable_unprepare(simple->ref_alt_clk);
	clk_disable_unprepare(simple->ref_clk);
	clk_disable_unprepare(simple->bus_clk);
#else
	usb_clk_disable();
#endif

	return 0;
}

static int __maybe_unused dwc3_of_simple_runtime_resume(struct device *dev)
{
#ifdef AXERA_CLK_AND_RST_EN
	struct dwc3_of_simple	*simple = dev_get_drvdata(dev);

	clk_prepare_enable(simple->ref_alt_clk);
	clk_prepare_enable(simple->ref_clk);
	clk_prepare_enable(simple->bus_clk);
#else
	usb_clk_enable();
#endif

	return 0;
}

static int __maybe_unused dwc3_of_simple_suspend(struct device *dev)
{
#ifdef AXERA_CLK_AND_RST_EN
	struct dwc3_of_simple *simple = dev_get_drvdata(dev);

	clk_disable_unprepare(simple->ref_alt_clk);
	clk_disable_unprepare(simple->ref_clk);
	clk_disable_unprepare(simple->bus_clk);

	axera_usb_sw_rst(simple, 1);
#else
	usb_clk_disable();
	usb_sw_rst(1);
#endif

	return 0;
}

static int __maybe_unused dwc3_of_simple_resume(struct device *dev)
{
#ifdef AXERA_CLK_AND_RST_EN
	struct dwc3_of_simple *simple = dev_get_drvdata(dev);

	axera_usb_sw_rst(simple, 0);

	clk_prepare_enable(simple->ref_alt_clk);
	clk_prepare_enable(simple->ref_clk);
	clk_prepare_enable(simple->bus_clk);
#else
	usb_sw_rst(0);
	usb_clk_enable();
#endif

	return 0;
}

static const struct dev_pm_ops dwc3_of_simple_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(dwc3_of_simple_suspend, dwc3_of_simple_resume)
	SET_RUNTIME_PM_OPS(dwc3_of_simple_runtime_suspend,
			dwc3_of_simple_runtime_resume, NULL)
};


static const struct of_device_id of_dwc3_simple_match[] = {
	{ .compatible = "axera,dwc3" },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, of_dwc3_simple_match);

static struct platform_driver dwc3_of_simple_driver = {
	.probe		= dwc3_of_simple_probe,
	.remove		= dwc3_of_simple_remove,
	.shutdown	= dwc3_of_simple_shutdown,
	.driver		= {
		.name	= "axera dwc3",
		.of_match_table = of_dwc3_simple_match,
		.pm	= &dwc3_of_simple_dev_pm_ops,
	},
};

module_platform_driver(dwc3_of_simple_driver);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("DesignWare USB3 OF Simple Glue Layer");
MODULE_AUTHOR("axera");
