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
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/phy.h>
#include <linux/soc/axera/ax_boardinfo.h>

#include "stmmac.h"
#include "stmmac_platform.h"
#include "dwmac-axera.h"

int axera_dwmac_get_dts(
			struct platform_device *pdev,
			struct plat_stmmacenet_data *plat_dat,
			struct axera_eqos *eqos
			)
{
	int err;

	eqos->rmii_loopback_mode =
			of_property_read_bool(pdev->dev.of_node, "axera,rmii_loopback");

	eqos->out_rmii_mode =
			of_property_read_bool(pdev->dev.of_node, "axera,out_rmii");

	eqos->led0_en =
			of_property_read_bool(pdev->dev.of_node, "axera-ephy,led0-enable");

	eqos->led1_en =
			of_property_read_bool(pdev->dev.of_node, "axera-ephy,led1-enable");

	eqos->gpiod_reset = of_get_named_gpio(pdev->dev.of_node, "phy-rst-gpio", 0);
	if (gpio_is_valid(eqos->gpiod_reset)) {
		gpio_request(eqos->gpiod_reset, NULL);
		gpio_direction_output(eqos->gpiod_reset, 0);
	} else {
		dev_info(&pdev->dev, "phy-rst-gpio not configure\n");
	}

	// EPHY CFG
	if ((plat_dat->interface == PHY_INTERFACE_MODE_RMII) && !eqos->out_rmii_mode) {
		if (eqos->led0_en) {
			err = of_property_read_u32(pdev->dev.of_node, "axera-ephy,led0-mode",
					&eqos->led0_mode);
			if (err < 0)
				return err;
		}

		if (eqos->led1_en) {
			err = of_property_read_u32(pdev->dev.of_node, "axera-ephy,led1-mode",
					&eqos->led1_mode);
			if (err < 0)
				return err;
		}

		if (eqos->led0_en || eqos->led1_en) {
			err = of_property_read_u32(pdev->dev.of_node, "axera-ephy,led-polarity",
				&eqos->polarity);
			if (err < 0)
				return err;
		}
	}

	return 0;
}

static int dwmac_axera_probe(struct platform_device *pdev)
{
	struct plat_stmmacenet_data *plat_dat;
	struct stmmac_resources stmmac_res;
	int ret;
	struct axera_eqos *eqos;

	dev_info(&pdev->dev, "probe axera emac\n");
	eqos = devm_kzalloc(&pdev->dev, sizeof(*eqos), GFP_KERNEL);
	if (!eqos) {
		return -ENOMEM;
	}
	memset(eqos, 0, sizeof(*eqos));

	ret = stmmac_get_platform_resources(pdev, &stmmac_res);
	if (ret) {
		return ret;
	}

	if (pdev->dev.of_node) {
		plat_dat = stmmac_probe_config_dt(pdev, &stmmac_res.mac);
		if (IS_ERR(plat_dat)) {
			dev_err(&pdev->dev, "dts configuration failed\n");
			return PTR_ERR(plat_dat);
		}
	} else {
		dev_err(&pdev->dev, "no emac dts provided\n");
		return -EINVAL;
	}

	axera_dwmac_get_dts(pdev, plat_dat, eqos);

	/* TODO: need to improve, mdio_clk = csr_clk / 324 */
#ifdef CONFIG_DWMAC_AXERA_HAPS
	/* in haps, mdio_clk = csr_clk / 4 = 10M / 4 = 2.5M */
	plat_dat->clk_csr = 0x1000;
#else
	plat_dat->clk_csr = 0x6;
#endif

	eqos->dev = &pdev->dev;
	eqos->phy_interface = plat_dat->interface;
	eqos->plat = plat_dat;
	eqos->bus_id = plat_dat->bus_id;

	plat_dat->fix_mac_speed = axera_eqos_fix_speed;
	plat_dat->bsp_priv = eqos;
	plat_dat->init = plat_init;
	plat_dat->exit = plat_exit;

#ifdef CONFIG_DWMAC_AXERA_HAPS
	emac_clk_init(eqos);
#else
	ret = axera_dwmac_config_dt(pdev, plat_dat, eqos);
	if (ret) {
		goto err_remove_config_dt;
	}
#endif

#ifdef EMAC_SYS_REST_EN
	emac_get_reset_control(eqos);
#endif

	dev_info(&pdev->dev, "reset axera emac...\n");
	emac_sw_rst(eqos);
	select_phy_interface(plat_dat);

	// plat_dat->sph_disable = 1;

	ret = stmmac_dvr_probe(&pdev->dev, plat_dat, &stmmac_res);
	if (ret) {
		goto err_remove_config_dt;
	}

	return 0;

err_remove_config_dt:
	if (pdev->dev.of_node) {
		stmmac_remove_config_dt(pdev, plat_dat);
	}

	if (gpio_is_valid(eqos->gpiod_reset)) {
		gpio_free(eqos->gpiod_reset);
	}

	return ret;
}

static const struct of_device_id dwmac_axera_match[] = {
	{.compatible = "axera,dwmac-4.10a"},
	{}
};

MODULE_DEVICE_TABLE(of, dwmac_axera_match);

static struct platform_driver dwmac_axera_driver = {
	.probe = dwmac_axera_probe,
	.remove = stmmac_pltfr_remove,
	.driver = {
		.name = STMMAC_RESOURCE_NAME,
		.pm = &stmmac_pltfr_pm_ops,
		.of_match_table = of_match_ptr(dwmac_axera_match),
	},
};

module_platform_driver(dwmac_axera_driver);

MODULE_DESCRIPTION("axera dwmac driver");
MODULE_LICENSE("GPL v2");
