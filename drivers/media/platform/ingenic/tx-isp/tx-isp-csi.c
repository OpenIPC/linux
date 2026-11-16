// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2017-2023 Ingenic Semiconductor Co., Ltd. */
/* Copyright (C) 2026 Difan Zhang */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/delay.h>

#include "tx-isp-device.h"

struct tx_isp_csi_device {
	struct tx_isp_subdev sd;
	void __iomem *phy;
	unsigned int lanes;
	unsigned int clock_mhz;
	bool configured;
	bool running;
};

static struct tx_isp_csi_device *g_csi_dev;

int tx_isp_csi_configure(struct tx_isp_subdev *sd, unsigned int lanes,
			 unsigned int clock_mhz)
{
	struct tx_isp_csi_device *csi = sd ? sd->dev_priv : NULL;

	if (!csi)
		return -EINVAL;
	if (csi->running)
		return -EBUSY;
	csi->configured = false;
	if (lanes < 1 || lanes > 2 || clock_mhz < 80 || clock_mhz >= 1500)
		return -EINVAL;
	csi->lanes = lanes;
	csi->clock_mhz = clock_mhz;
	csi->configured = true;
	return 0;
}

static u32 csi_settle_range(unsigned int clock)
{
	static const unsigned int upper[] = {
		110, 150, 200, 250, 300, 400, 500, 600, 700, 800, 1000,
	};
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(upper); i++)
		if (clock < upper[i])
			return i;
	return 11;
}

void tx_isp_csi_select_rate(struct tx_isp_subdev *sd, unsigned int clock_mhz)
{
	struct tx_isp_csi_device *csi = sd->dev_priv;
	u32 value;

	if (csi->clock_mhz == clock_mhz)
		return;
	value = (readl(csi->phy + DPHY_CLK_SETTLE) & ~0xfU) |
		csi_settle_range(clock_mhz);
	writel(value, csi->phy + DPHY_CLK_SETTLE);
	writel(value, csi->phy + DPHY_DATA0_SETTLE);
	writel(value, csi->phy + DPHY_DATA1_SETTLE);
	readl(csi->phy + DPHY_DATA1_SETTLE);
	csi->clock_mhz = clock_mhz;
}

int csi_phy_start(struct tx_isp_subdev *sd)
{
	struct tx_isp_csi_device *csi = sd ? sd->dev_priv : NULL;
	u32 value;

	if (!csi || !sd->base || !csi->phy)
		return -EINVAL;
	if (!csi->configured)
		return -ENOLINK;
	if (csi->running)
		return 0;
	writel(csi->lanes - 1, sd->base + CSI2_N_LANES);
	writel(readl(sd->base + CSI2_PHY_SHUTDOWNZ) & ~CSI2_PHY_SHUTDOWNZ_BIT,
	       sd->base + CSI2_PHY_SHUTDOWNZ);
	writel(0, sd->base + CSI2_DPHY_RSTZ);
	msleep(1);
	writel(readl(sd->base + CSI2_RESETN) & ~CSI2_RESETN_BIT,
	       sd->base + CSI2_RESETN);
	msleep(1);
	writel(CSI2_DPHY_RSTZ_BIT, sd->base + CSI2_DPHY_RSTZ);
	msleep(1);
	writel(CSI2_TIMEOUT_DEFAULT, sd->base + CSI2_TIMEOUT_REG);
	value = (readl(csi->phy + DPHY_CLK_SETTLE) & ~DPHY_SETTLE_MASK) |
		csi_settle_range(csi->clock_mhz);
	writel(value, csi->phy + DPHY_CLK_SETTLE);
	writel(value, csi->phy + DPHY_DATA0_SETTLE);
	writel(value, csi->phy + DPHY_DATA1_SETTLE);
	writel(DPHY_PWR_ON, csi->phy + DPHY_PWR_REG);
	writel(DPHY_RST_RELEASE, csi->phy + DPHY_RST_REG);
	writel(CSI2_RESETN_BIT, sd->base + CSI2_RESETN);
	msleep(10);
	csi->running = true;
	return 0;
}

int csi_phy_stop(struct tx_isp_subdev *sd)
{
	struct tx_isp_csi_device *csi = sd ? sd->dev_priv : NULL;

	if (!csi || !sd->base)
		return -EINVAL;
	if (!csi->running)
		return 0;
	writel(readl(sd->base + CSI2_PHY_SHUTDOWNZ) & ~CSI2_PHY_SHUTDOWNZ_BIT,
	       sd->base + CSI2_PHY_SHUTDOWNZ);
	writel(readl(sd->base + CSI2_DPHY_RSTZ) & ~CSI2_DPHY_RSTZ_BIT,
	       sd->base + CSI2_DPHY_RSTZ);
	writel(readl(sd->base + CSI2_RESETN) & ~CSI2_RESETN_BIT,
	       sd->base + CSI2_RESETN);
	csi->running = false;
	return 0;
}

static struct tx_isp_subdev_ops csi_subdev_ops = {};

int tx_isp_csi_probe(struct platform_device *pdev)
{
	struct tx_isp_csi_device *csi;
	struct resource *res;
	int ret;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "phy");
	if (!res || resource_size(res) < 0x264) {
		dev_err(&pdev->dev, "missing or invalid phy resource\n");
		return -EINVAL;
	}
	csi = kzalloc(sizeof(*csi), GFP_KERNEL);
	if (!csi)
		return -ENOMEM;
	csi->phy = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(csi->phy)) {
		ret = PTR_ERR(csi->phy);
		dev_err(&pdev->dev, "failed to ioremap phy resource: %d\n", ret);
		kfree(csi);
		return ret;
	}
	ret = tx_isp_subdev_init(pdev, &csi->sd, &csi_subdev_ops);
	if (ret) {
		dev_err(&pdev->dev, "failed to initialize csi subdev: %d\n", ret);
		kfree(csi);
		return ret;
	}
	csi->sd.dev_priv = csi;
	g_csi_dev = csi;
	return 0;
}

int tx_isp_csi_remove(struct platform_device *pdev)
{
	struct tx_isp_csi_device *csi = g_csi_dev;

	if (csi) {
		csi_phy_stop(&csi->sd);
		tx_isp_subdev_deinit(&csi->sd);
		kfree(csi);
		g_csi_dev = NULL;
	}
	return 0;
}
