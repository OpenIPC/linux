/*
 * Copyright (c) 2017 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
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

#include "dwc3-hisi.h"

#define USB3_CTRL           0x190
#define REG_SYS_STAT        0x8c
#define PCIE_USB3_MODE_MASK (0x3 << 12)
#define USB3_PCLK_OCC_SEL   (0x1 << 30)

#define PERI_USB3_GTXTHRCFG 0x2310000

#define REG_GUSB3PIPECTL0 0xc2c0
#define GTXTHRCFG         0xc108

#define PCS_SSP_SOFT_RESET  (0x1 << 31)
#define SUSPEND_USB3_SS_PHY (0x1 << 17)

#define GUSB2PHYCFG_OFFSET 0xc200
#define GCTL_OFFSET        0xc110
#define GUCTL_OFFSET       0xc12C
#define GFLADJ_OFFSET      0xc630

#define U2_FREECLK_EXISTS (0x1 << 30)
#define SOFITPSYNC        (0x1 << 10)
#define REFCLKPER_MASK    0xffc00000
#define REFCLKPER_VAL(a)  (((a) << 22) & REFCLKPER_MASK)

#define PLS1        (0x1 << 31)
#define DECR_MASK   0x7f000000
#define DECR_VAL(a) (((a) << 24) & DECR_MASK)

#define LPM_SEL      (0x1 << 23)
#define FLADJ_MASK   0x003fff00
#define FLADJ_VAL(a) (((a) << 8) & FLADJ_MASK)

/* hi3559a */
#define DOUBLE_PCIE_MODE    0x0
#define P0_PCIE_ADD_P1_USB3 (0x1 << 12)
#define DOUBLE_USB3         (0x2 << 12)

/* hi3556a,hi3519a */
#define PCIE_X1_MODE (0x0 << 12)
#define USB3_MODE    (0x1 << 12)

static struct hi_priv *priv;

#if defined(CONFIG_ARCH_HI3559AV100)
static int speed_adapt_for_hi3559a(struct device_node *np)
{
	unsigned int ret;
	unsigned int reg;

	priv->speed_id = -1;

	reg = readl(priv->sys_ctrl + REG_SYS_STAT);
	reg &= PCIE_USB3_MODE_MASK;

	switch (reg) {
	case DOUBLE_PCIE_MODE:
		ret = USB_SPEED_HIGH;
		break;
	case P0_PCIE_ADD_P1_USB3:
		if (of_property_read_u32(np, "port_speed", &priv->speed_id))
			ret = USB_SPEED_UNKNOWN;

		if (priv->speed_id == 0)
			ret = USB_SPEED_HIGH;
		else if (priv->speed_id == 1)
			ret = USB_SPEED_SUPER;
		else
			ret = USB_SPEED_UNKNOWN;

		break;
	case DOUBLE_USB3:
		ret = USB_SPEED_SUPER;
		break;
	default:
		ret = USB_SPEED_UNKNOWN;
	}

	return ret;
}
#endif

#if defined(CONFIG_ARCH_HI3556AV100) || defined(CONFIG_ARCH_HI3519AV100)
static int speed_adapt_for_hi3556a(struct device *dev)
{
	unsigned int ret;
	unsigned int reg;

	reg = readl(priv->sys_ctrl + REG_SYS_STAT);
	reg &= PCIE_USB3_MODE_MASK;

	if (reg == PCIE_X1_MODE)
		ret = USB_SPEED_HIGH;
	else
		ret = usb_get_maximum_speed(dev);

	return ret;
}
#endif

int usb_get_max_speed(struct device *dev)
{
	unsigned int ret;
	struct device_node *np = dev->of_node;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->peri_crg = of_iomap(np, 1);
	if (IS_ERR(priv->peri_crg)) {
		ret = PTR_ERR(priv->peri_crg);
		goto err;
	}

	priv->sys_ctrl = of_iomap(np, 2);
	if (IS_ERR(priv->sys_ctrl)) {
		ret = PTR_ERR(priv->sys_ctrl);
		goto err;
	}

#if defined(CONFIG_ARCH_HI3559AV100)
	return speed_adapt_for_hi3559a(np);
#endif
#if defined(CONFIG_ARCH_HI3556AV100) || defined(CONFIG_ARCH_HI3519AV100)
	return speed_adapt_for_hi3556a(dev);
#endif
	return usb_get_maximum_speed(dev);
err:
	kfree(priv);
	return usb_get_maximum_speed(dev);
}
EXPORT_SYMBOL(usb_get_max_speed);

void hisi_dwc3_exited(void)
{
	kfree(priv);
}
EXPORT_SYMBOL(hisi_dwc3_exited);

static int set_ctrl_crg_val(struct device_node *np, struct dwc3_hisi *hisi)
{
	unsigned int ret;
	unsigned int reg;

	/* get usb ctrl crg para */
	ret = of_property_read_u32(np, "crg_offset", &hisi->crg_offset);
	if (ret)
		return ret;

	ret = of_property_read_u32(np, "crg_ctrl_def_mask", &hisi->crg_ctrl_def_mask);
	if (ret)
		return ret;

	ret = of_property_read_u32(np, "crg_ctrl_def_val", &hisi->crg_ctrl_def_val);
	if (ret)
		return ret;

	/* write usb ctrl crg default value */
	reg = readl(hisi->crg_base + hisi->crg_offset);
	reg &= ~hisi->crg_ctrl_def_mask;
	reg |= hisi->crg_ctrl_def_val;
	writel(reg, hisi->crg_base + hisi->crg_offset);

	return 0;
}

static int dwc3_hisi_clk_init(struct dwc3_hisi *hisi, int count)
{
	struct device *dev = hisi->dev;
	struct device_node *np = dev->of_node;
	int i;

	hisi->num_clocks = count;
	if (!count)
		return 0;

	hisi->clks = devm_kcalloc(dev, hisi->num_clocks, sizeof(struct clk *), GFP_KERNEL);
	if (!hisi->clks)
		return -ENOMEM;

	for (i = 0; i < hisi->num_clocks; i++) {
		struct clk *clk;
		int ret;

		clk = of_clk_get(np, i);
		if (IS_ERR(clk)) {
			while (--i >= 0)
				clk_put(hisi->clks[i]);

			return PTR_ERR(clk);
		}

		ret = clk_prepare_enable(clk);
		if (ret < 0) {
			while (--i >= 0) {
				clk_disable_unprepare(hisi->clks[i]);
				clk_put(hisi->clks[i]);
			}
			clk_put(clk);

			return ret;
		}

		hisi->clks[i] = clk;
	}

	return 0;
}

static int control_free_clk_config(struct dwc3_hisi *hisi)
{
	unsigned int reg;

	reg = readl(hisi->ctrl_base + GUSB2PHYCFG_OFFSET);
	reg &= ~U2_FREECLK_EXISTS;
	writel(reg, hisi->ctrl_base + GUSB2PHYCFG_OFFSET);

	reg = readl(hisi->ctrl_base + GCTL_OFFSET);
	reg &= ~SOFITPSYNC;
	writel(reg, hisi->ctrl_base + GCTL_OFFSET);

	reg = readl(hisi->ctrl_base + GUCTL_OFFSET);
	reg &= ~REFCLKPER_MASK;
	reg |= REFCLKPER_VAL(41);
	writel(reg, hisi->ctrl_base + GUCTL_OFFSET);

	reg = readl(hisi->ctrl_base + GFLADJ_OFFSET);
	reg &= ~PLS1;
	writel(reg, hisi->ctrl_base + GFLADJ_OFFSET);

	reg = readl(hisi->ctrl_base + GFLADJ_OFFSET);
	reg &= ~DECR_MASK;
	reg |= DECR_VAL(10);
	writel(reg, hisi->ctrl_base + GFLADJ_OFFSET);

	reg = readl(hisi->ctrl_base + GFLADJ_OFFSET);
	reg |= LPM_SEL;
	writel(reg, hisi->ctrl_base + GFLADJ_OFFSET);

	reg = readl(hisi->ctrl_base + GFLADJ_OFFSET);
	reg &= ~FLADJ_MASK;
	reg |= FLADJ_VAL(2032);
	writel(reg, hisi->ctrl_base + GFLADJ_OFFSET);

	return 0;
}

static int dwc3_hisi_probe(struct platform_device *pdev)
{
	struct dwc3_hisi *hisi;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	int ret, i;

	hisi = devm_kzalloc(dev, sizeof(*hisi), GFP_KERNEL);
	if (!hisi)
		return -ENOMEM;

	platform_set_drvdata(pdev, hisi);

	hisi->dev = dev;

	hisi->port_rst = devm_reset_control_get(dev, "vcc_reset");
	if (IS_ERR_OR_NULL(hisi->port_rst)) {
		dev_err(dev, "get crg_rst failed.\n");
		return PTR_ERR(hisi->port_rst);
	}

	hisi->ctrl_base = of_iomap(np, 0);
	if (!hisi->ctrl_base)
		return -ENOMEM;

	hisi->crg_base = of_iomap(np, 1);
	if (!hisi->crg_base) {
		iounmap(hisi->ctrl_base);
		return -ENOMEM;
	}

	set_ctrl_crg_val(np, hisi);

	reset_control_assert(hisi->port_rst);

	ret = dwc3_hisi_clk_init(hisi, of_clk_get_parent_count(np));
	if (ret) {
		goto err_0;
	}

	reset_control_deassert(hisi->port_rst);

	control_free_clk_config(hisi);

	udelay(200);

	ret = of_platform_populate(np, NULL, NULL, dev);
	if (ret) {
		dev_err(dev, "failed to register dwc3 device\n");
		for (i = 0; i < hisi->num_clocks; i++) {
			clk_disable_unprepare(hisi->clks[i]);
			clk_put(hisi->clks[i]);
		}
		goto err_0;
	}

err_0:
	iounmap(hisi->ctrl_base);
	iounmap(hisi->crg_base);
	return ret;
}

static int dwc3_hisi_remove(struct platform_device *pdev)
{
	struct dwc3_hisi *hisi = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	int i;

	for (i = 0; i < hisi->num_clocks; i++) {
		clk_disable_unprepare(hisi->clks[i]);
		clk_put(hisi->clks[i]);
	}

	reset_control_assert(hisi->port_rst);

	of_platform_depopulate(dev);

	return 0;
}

static const struct of_device_id hisi_dwc3_match[] = {
	{ .compatible = "hisi,dwusb2" },
	{ .compatible = "hisi,dwusb3" },
	{},
};
MODULE_DEVICE_TABLE(of, hisi_dwc3_match);

static struct platform_driver dwc3_hisi_driver = {
	.probe = dwc3_hisi_probe,
	.remove = dwc3_hisi_remove,
	.driver = {
		.name = "hisi-dwc3",
		.of_match_table = hisi_dwc3_match,
	},
};
module_platform_driver(dwc3_hisi_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("DesignWare USB3 of HiSi");
MODULE_AUTHOR("HiSilicon Technologies Co., Ltd..>");
