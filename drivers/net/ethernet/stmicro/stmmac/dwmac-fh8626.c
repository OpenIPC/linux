// SPDX-License-Identifier: GPL-2.0
/* Fullhan FH8626V100 Synopsys 3.x GMAC integration.
 * The PMU enables the RMII clocks, selects the link speed and resets
 * the MAC.
 * Copyright (C) 2026 Difan Zhang
 * Licensed under the GNU General Public License version 2.
 */
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_mdio.h>
#include <linux/of_net.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/soc/fullhan/fh8626-pmu.h>
#include "stmmac.h"
#include "stmmac_platform.h"

struct fh8626_dwmac {
	struct device *dev;
	struct clk *mac_clk;
	struct clk *rmii_clk;
	bool clocks_enabled;
};

static void fh8626_dwmac_fix_speed(void *priv, unsigned int speed)
{
	struct fh8626_dwmac *dwmac = priv;
	int ret;

	if (speed != SPEED_10 && speed != SPEED_100) {
		dev_err(dwmac->dev, "unsupported RMII speed %u\n", speed);
		return;
	}
	ret = fh8626_pmu_update_bits(0x28, BIT(2),
				     speed == SPEED_100 ? BIT(2) : 0);
	if (ret)
		dev_err(dwmac->dev, "failed to select RMII speed: %d\n", ret);
}

static int fh8626_dwmac_init(struct platform_device *pdev, void *priv)
{
	struct fh8626_dwmac *dwmac = priv;
	int ret;

	if (dwmac->clocks_enabled)
		return 0;
	ret = clk_prepare_enable(dwmac->mac_clk);
	if (ret)
		return ret;
	ret = clk_prepare_enable(dwmac->rmii_clk);
	if (ret)
		goto err_mac;
	ret = fh8626_pmu_update_bits(0x28, BIT(2), BIT(2));
	if (ret)
		goto err_rmii;
	ret = fh8626_pmu_reset(10);
	if (!ret) {
		dwmac->clocks_enabled = true;
		return 0;
	}
	dev_err(&pdev->dev, "failed to reset GMAC: %d\n", ret);
err_rmii:
	clk_disable_unprepare(dwmac->rmii_clk);
err_mac:
	clk_disable_unprepare(dwmac->mac_clk);
	return ret;
}

static void fh8626_dwmac_exit(struct platform_device *pdev, void *priv)
{
	struct fh8626_dwmac *dwmac = priv;

	if (!dwmac->clocks_enabled)
		return;
	clk_disable_unprepare(dwmac->rmii_clk);
	clk_disable_unprepare(dwmac->mac_clk);
	dwmac->clocks_enabled = false;
}

#ifdef CONFIG_PM_SLEEP
static int fh8626_dwmac_suspend(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	int ret;

	if (device_may_wakeup(dev))
		return -EOPNOTSUPP;
	ret = stmmac_suspend(dev);
	if (ret)
		return ret;
	fh8626_dwmac_exit(to_platform_device(dev), priv->plat->bsp_priv);
	return 0;
}

static int fh8626_dwmac_resume(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct platform_device *pdev = to_platform_device(dev);
	int ret;

	ret = fh8626_dwmac_init(pdev, priv->plat->bsp_priv);
	if (ret)
		return ret;
	ret = stmmac_resume(dev);
	if (ret)
		fh8626_dwmac_exit(pdev, priv->plat->bsp_priv);
	return ret;
}
#endif

static SIMPLE_DEV_PM_OPS(fh8626_dwmac_pm_ops, fh8626_dwmac_suspend,
			fh8626_dwmac_resume);

static int fh8626_dwmac_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct stmmac_resources res;
	struct plat_stmmacenet_data *plat;
	struct fh8626_dwmac *dwmac;
	u32 value;
	int ret;

	if (!of_find_property(dev->of_node, "phy-handle", NULL) &&
	    !of_phy_is_fixed_link(dev->of_node)) {
		dev_err(dev, "missing phy-handle or fixed-link\n");
		return -ENODEV;
	}
	ret = fh8626_pmu_read(0x28, &value);
	if (ret)
		return ret;
	dwmac = devm_kzalloc(dev, sizeof(*dwmac), GFP_KERNEL);
	if (!dwmac)
		return -ENOMEM;
	dwmac->dev = dev;
	dwmac->mac_clk = devm_clk_get(dev, "stmmaceth");
	if (IS_ERR(dwmac->mac_clk))
		return PTR_ERR(dwmac->mac_clk);
	dwmac->rmii_clk = devm_clk_get(dev, "rmii");
	if (IS_ERR(dwmac->rmii_clk))
		return PTR_ERR(dwmac->rmii_clk);
	if (!clk_get_rate(dwmac->mac_clk))
		return -EINVAL;
	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret)
		return ret;
	ret = stmmac_get_platform_resources(pdev, &res);
	if (ret)
		return ret;
	plat = stmmac_probe_config_dt(pdev, &res.mac);
	if (IS_ERR(plat))
		return PTR_ERR(plat);
	plat->has_gmac = 1;
	plat->maxmtu = ETH_DATA_LEN;
	if (plat->max_speed < 0 || plat->max_speed > SPEED_100)
		plat->max_speed = SPEED_100;
	plat->bsp_priv = dwmac;
	plat->fix_mac_speed = fh8626_dwmac_fix_speed;
	plat->init = fh8626_dwmac_init;
	plat->exit = fh8626_dwmac_exit;
	ret = fh8626_dwmac_init(pdev, dwmac);
	if (ret)
		goto err_dt;
	ret = stmmac_dvr_probe(dev, plat, &res);
	if (!ret)
		return 0;
	fh8626_dwmac_exit(pdev, dwmac);
err_dt:
	stmmac_remove_config_dt(pdev, plat);
	return ret;
}

static const struct of_device_id fh8626_dwmac_match[] = {
	{ .compatible = "fullhan,fh8626v100-gmac" },
	{ }
};
MODULE_DEVICE_TABLE(of, fh8626_dwmac_match);
static struct platform_driver fh8626_dwmac_driver = {
	.probe = fh8626_dwmac_probe,
	.remove = stmmac_pltfr_remove,
	.driver = {
		.name = "fh8626-dwmac",
		.of_match_table = fh8626_dwmac_match,
		.pm = &fh8626_dwmac_pm_ops,
	},
};
module_platform_driver(fh8626_dwmac_driver);
MODULE_DESCRIPTION("Fullhan FH8626V100 DWMAC integration");
MODULE_LICENSE("GPL v2");
