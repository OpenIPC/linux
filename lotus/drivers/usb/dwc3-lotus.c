// SPDX-License-Identifier: GPL-2.0
#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/usb/ch9.h>
#include <mach/platform.h>

#include "dwc3-lotus.h"

#define GUSB2PHYCFG_OFFSET	0xc200
#define GCTL_OFFSET		0xc110
#define GUCTL_OFFSET		0xc12C
#define GFLADJ_OFFSET		0xc630

#define U2_FREECLK_EXISTS	(0x1 << 30)
#define SOFITPSYNC		(0x1 << 10)
#define REFCLKPER_MASK		0xffc00000
#define REFCLKPER_VAL		0x29
#define set_refclkper(a)	(((a) << 22) & REFCLKPER_MASK)

#define PLS1			(0x1 << 31)
#define DECR_MASK		0x7f000000
#define DECR_VAL		0xa
#define set_decr(a)		(((a) << 24) & DECR_MASK)

#define LPM_SEL			(0x1 << 23)
#define FLADJ_MASK		0x003fff00
#define FLADJ_VAL		0x7f0
#define set_fladj(a)		(((a) << 8) & FLADJ_MASK)

#define CRG_OFFSET		0x140

static void control_free_clk_config(struct dwc3_host *host)
{
	unsigned int reg;

	if (host == NULL)
		return;

	reg = readl(host->ctrl_base + GUSB2PHYCFG_OFFSET);
	reg &= ~U2_FREECLK_EXISTS;
	writel(reg, host->ctrl_base + GUSB2PHYCFG_OFFSET);

	reg = readl(host->ctrl_base + GCTL_OFFSET);
	reg &= ~SOFITPSYNC;
	writel(reg, host->ctrl_base + GCTL_OFFSET);

	reg = readl(host->ctrl_base + GUCTL_OFFSET);
	reg &= ~REFCLKPER_MASK;
	reg |= set_refclkper(REFCLKPER_VAL);
	writel(reg, host->ctrl_base + GUCTL_OFFSET);

	reg = readl(host->ctrl_base + GFLADJ_OFFSET);
	reg &= ~PLS1;
	writel(reg, host->ctrl_base + GFLADJ_OFFSET);

	reg = readl(host->ctrl_base + GFLADJ_OFFSET);
	reg &= ~DECR_MASK;
	reg |= set_decr(DECR_VAL);
	writel(reg, host->ctrl_base + GFLADJ_OFFSET);

	reg = readl(host->ctrl_base + GFLADJ_OFFSET);
	reg |= LPM_SEL;
	writel(reg, host->ctrl_base + GFLADJ_OFFSET);

	reg = readl(host->ctrl_base + GFLADJ_OFFSET);
	reg &= ~FLADJ_MASK;
	reg |= set_fladj(FLADJ_VAL);
	writel(reg, host->ctrl_base + GFLADJ_OFFSET);
}

static int dwc3_lotus_iomap(struct device_node *np, struct dwc3_host *host)
{
	if ((np == NULL) || (host == NULL))
		return -EINVAL;

	host->ctrl_base = of_iomap(np, DEV_NODE_FLAG0);
	if (IS_ERR(host->ctrl_base)) {
		pr_err("%s: ctrl_base map fail\n", __func__);
		return -ENOMEM;
	}

	host->crg_base = of_iomap(np, DEV_NODE_FLAG1);
	if (IS_ERR(host->crg_base)) {
		iounmap(host->ctrl_base);
		pr_err("%s: crg_base map fail\n", __func__);
		return -ENOMEM;
	}

	host->misc_base = of_iomap(np, DEV_NODE_FLAG2);
	if (IS_ERR(host->misc_base)) {
		iounmap(host->ctrl_base);
		iounmap(host->crg_base);
		pr_err("%s: misc_base map fail\n", __func__);
		return -ENOMEM;
	}

	return 0;
}

/**
 * lotus_get_dr_mode - get USB CTRL dr_mode.
 *
 * @return:
 * 0: success to find dr_mode.
 * other: error.
 */
static int lotus_get_dr_mode(struct device *dev,
			struct dwc3_host *host)
{
	const char *dr_mode;
	struct device_node *np = dev->of_node;
	struct device_node *child_np;
	struct fwnode_handle *fwnode;
	int res;

	if ((dev == NULL) || (host == NULL)) {
		pr_err("%s: param invalid\n", __func__);
		return -EINVAL;
	}

	child_np = of_find_node_by_name(np, "dwc3");
	if (!child_np) {
		pr_err("%s: can't find child np\n", __func__);
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_OF) && dev->of_node) {
		fwnode = &(child_np->fwnode);
	} else {
		fwnode = dev->fwnode;
	}

	res = fwnode_property_read_string(fwnode, "dr_mode", &dr_mode);
	if (res < 0) {
		pr_err("%s: fwnode_property_read_string fail, ret=%d\n", __func__, res);
		return -EINVAL;
	}

	if (!strcmp(dr_mode, "host"))
		host->is_usb_host = 1;
	else if (!strcmp(dr_mode, "peripheral"))
		host->is_usb_host = 0;
	else
		return -EINVAL;

	return 0;
}
static int dwc3_lotus_probe(struct platform_device *pdev)
{
	struct dwc3_host *host = NULL;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	int ret;

	pr_debug("%s: enter\n", __func__);

	host = devm_kzalloc(dev, sizeof(*host), GFP_KERNEL);
	if (host == NULL) {
		ret = -ENOMEM;
		goto exit;
	}

	ret = lotus_get_dr_mode(dev, host);
	if (ret) {
		pr_err("%s: get dr_mode fail %d\n", __func__, ret);
		goto dwc3_free;
	}

	ret = dwc3_lotus_iomap(np, host);
	if (ret) {
		ret = -ENOMEM;
		goto dwc3_free;
	}

	platform_set_drvdata(pdev, host);
	host->dev = dev;

	lotus_u2_ctrl_misc_config(host);
	lotus_u2_ctrl_clk_init(host->crg_base);

	control_free_clk_config(host);

	udelay(200);

	ret = of_platform_populate(np, NULL, NULL, dev);
	if (ret) {
		pr_err("%s: of_platform_populate fail, ret=%d\n", __func__, ret);
		goto dwc3_disable_clk;
	}

	pr_debug("%s: ok\n", __func__);

	return 0;

dwc3_disable_clk:
	lotus_u2_ctrl_clk_exit(host->crg_base);

	iounmap(host->ctrl_base);
	iounmap(host->crg_base);
	iounmap(host->misc_base);

dwc3_free:
	devm_kfree(dev, host);
	host = NULL;

exit:
	return ret;
}

static int dwc3_lotus_remove(struct platform_device *pdev)
{
	struct dwc3_host *host = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	int ret = 0;

	if (!host) {
		ret = -EINVAL;
		pr_err("%s: dwc3_host is NULL\n", __func__);
		goto exit;
	}

	lotus_u2_ctrl_clk_exit(host->crg_base);

	of_platform_depopulate(dev);

	iounmap(host->ctrl_base);
	iounmap(host->crg_base);
	iounmap(host->misc_base);

	devm_kfree(dev, host);
	host = NULL;

exit:
	return ret;
}

#ifdef CONFIG_PM_SLEEP
static int lotus_usb_ctrl_suspend(struct device *dev)
{
	struct dwc3_host *host = dev_get_drvdata(dev);

	pr_debug("%s: enter!\n", __func__);

	if (!host)
		return 0;

	lotus_u2_ctrl_clk_exit(host->crg_base);

	pr_debug("%s: ok!\n", __func__);
	return 0;
}

static int lotus_usb_ctrl_resume(struct device *dev)
{
	struct dwc3_host *host = dev_get_drvdata(dev);

	pr_debug("%s: enter!\n", __func__);

	if (!host)
		return 0;

	lotus_u2_ctrl_misc_config(host);
	lotus_u2_ctrl_clk_resume(host->crg_base);
	control_free_clk_config(host);

	pr_debug("%s: ok!\n", __func__);

	return 0;
}
#endif /* CONFIG_PM_SLEEP */

static SIMPLE_DEV_PM_OPS(lotus_dwc3_pm_ops, lotus_usb_ctrl_suspend,
			lotus_usb_ctrl_resume);

static const struct of_device_id lotus_dwc3_match[] = {
	{ .compatible = "lotus,dwcusb2" },
	{ .compatible = "lotus,dwcusb3" },
	{},
};
MODULE_DEVICE_TABLE(of, lotus_dwc3_match);

static struct platform_driver dwc3_lotus_driver = {
	.probe = dwc3_lotus_probe,
	.remove = dwc3_lotus_remove,
	.driver = {
		.name = "lotus-dwc3",
		.pm = &lotus_dwc3_pm_ops,
		.of_match_table = lotus_dwc3_match,
	},
};
module_platform_driver(dwc3_lotus_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("DesignWare USB3 of Lotus");
MODULE_AUTHOR("Lotus Technologies Co., Ltd..>");
