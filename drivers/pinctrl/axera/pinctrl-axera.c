// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2022 AXERA Technology Co., Ltd.
 *
 * Adapted for Linux 6.18 generic pin group/function APIs.
 */

#include <linux/io.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>

#include "../core.h"
#include "../pinmux.h"
#include "../pinctrl-utils.h"
#include "pinctrl-axera.h"
#include "pinctrl-ax620e.h"

#define AX_DPHYTX_MIPI_EN	0x10c

struct ax_pinctrl {
	struct pinctrl_dev *pctldev;
	struct device *dev;
	void __iomem *base;
	void __iomem *base2;
	struct regmap *dphytx;
	struct reset_control *dphytx_reset;
	spinlock_t lock;
	struct axera_pinctrl_soc_info *info;
};

static u32 ax_pinctrl_readl(struct ax_pinctrl *axpctl, u32 offset)
{
	if (offset < SECOND_OFFSET)
		return readl(axpctl->base + offset);
	return readl(axpctl->base2 + offset - SECOND_OFFSET);
}

static void ax_pinctrl_writel(struct ax_pinctrl *axpctl, u32 offset, u32 val)
{
	if (offset < SECOND_OFFSET)
		writel(val, axpctl->base + offset);
	else
		writel(val, axpctl->base2 + offset - SECOND_OFFSET);
}

static int ax_dt_node_to_map(struct pinctrl_dev *pctldev,
			     struct device_node *np_config,
			     struct pinctrl_map **map, unsigned int *num_maps)
{
	return pinconf_generic_dt_node_to_map(pctldev, np_config, map,
					      num_maps, PIN_MAP_TYPE_INVALID);
}

static const struct pinctrl_ops ax_pinctrl_ops = {
	.dt_node_to_map = ax_dt_node_to_map,
	.dt_free_map = pinctrl_utils_free_map,
	.get_groups_count = pinctrl_generic_get_group_count,
	.get_group_name = pinctrl_generic_get_group_name,
	.get_group_pins = pinctrl_generic_get_group_pins,
};

static int ax_set_mux(struct pinctrl_dev *pctldev, unsigned int func_selector,
		      unsigned int group_selector)
{
	struct ax_pinctrl *axpctl = pinctrl_dev_get_drvdata(pctldev);
	struct axera_pinctrl_soc_info *info = axpctl->info;
	const struct pinctrl_pin_desc *pindesc = info->pins + group_selector;
	struct axera_pin_data *data = pindesc->drv_data;
	const struct function_desc *func;
	struct axera_mux_desc *mux;
	unsigned long flags;
	int ret;
	u32 val;

	if (!data)
		return -EINVAL;

	func = pinmux_generic_get_function(pctldev, func_selector);
	if (!func || !func->func)
		return -EINVAL;

	for (mux = data->muxes; mux->name; mux++) {
		if (!strcmp(mux->name, func->func->name))
			break;
	}
	if (!mux->name)
		return -EINVAL;

	/*
	 * CDTX pads bypass the ordinary mux while the MIPI transmitter is
	 * enabled. Follow the AX620E SDK sequence before selecting a digital
	 * function: hold D-PHY TX in reset, disable MIPI, then program the pad.
	 * Do not change the PHY for unrelated pads or its native function.
	 */
	if (data->offset >= CDTX_L0N_OFFSET &&
	    data->offset <= CDTX_L4P_OFFSET && mux->muxval) {
		if (!axpctl->dphytx || !axpctl->dphytx_reset)
			return -ENODEV;
		ret = reset_control_assert(axpctl->dphytx_reset);
		if (ret)
			return ret;
		ret = regmap_write(axpctl->dphytx, AX_DPHYTX_MIPI_EN, 0);
		if (ret)
			return ret;
	}

	spin_lock_irqsave(&axpctl->lock, flags);
	val = ax_pinctrl_readl(axpctl, data->offset);
	val &= ~FUNCTION_SELECT_BIT_CLEAR;
	val |= mux->muxval << FUNCTION_SELECT;
	ax_pinctrl_writel(axpctl, data->offset, val);
	spin_unlock_irqrestore(&axpctl->lock, flags);
	return 0;
}

static const struct pinmux_ops ax_pinmux_ops = {
	.get_functions_count = pinmux_generic_get_function_count,
	.get_function_name = pinmux_generic_get_function_name,
	.get_function_groups = pinmux_generic_get_function_groups,
	.set_mux = ax_set_mux,
	.strict = true,
};

static int ax_pin_config_get(struct pinctrl_dev *pctldev, unsigned int pin,
			     unsigned long *config)
{
	struct ax_pinctrl *axpctl = pinctrl_dev_get_drvdata(pctldev);
	struct axera_pinctrl_soc_info *info = axpctl->info;
	const struct pinctrl_pin_desc *pindesc = info->pins + pin;
	struct axera_pin_data *data = pindesc->drv_data;
	enum pin_config_param param = pinconf_to_config_param(*config);
	u32 val;
	u32 arg = 0;

	if (!data)
		return -EINVAL;

	val = ax_pinctrl_readl(axpctl, data->offset);
	switch (param) {
	case PIN_CONFIG_BIAS_PULL_DOWN:
		if ((val & AX_PULL_DOWN) != AX_PULL_DOWN)
			return -EINVAL;
		arg = 1;
		break;
	case PIN_CONFIG_BIAS_PULL_UP:
		if ((val & AX_PULL_UP) != AX_PULL_UP)
			return -EINVAL;
		arg = 1;
		break;
	case PIN_CONFIG_BIAS_DISABLE:
		if (val & (0x3 << AX_PULL_DOWN_BIT))
			return -EINVAL;
		arg = 0;
		break;
	case PIN_CONFIG_DRIVE_STRENGTH:
		arg = val & AX_DRIVE_STRENGTH;
		break;
	case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
		arg = !!(val & AX_SCHMITT_ENABLE);
		break;
	default:
		return -ENOTSUPP;
	}

	*config = pinconf_to_config_packed(param, arg);
	return 0;
}

static int ax_pin_config_set(struct pinctrl_dev *pctldev, unsigned int pin,
			     unsigned long *configs, unsigned int num_configs)
{
	struct ax_pinctrl *axpctl = pinctrl_dev_get_drvdata(pctldev);
	struct axera_pinctrl_soc_info *info = axpctl->info;
	const struct pinctrl_pin_desc *pindesc = info->pins + pin;
	struct axera_pin_data *data = pindesc->drv_data;
	unsigned long flags;
	u32 val;
	unsigned int i;

	if (!data)
		return -EINVAL;

	spin_lock_irqsave(&axpctl->lock, flags);
	val = ax_pinctrl_readl(axpctl, data->offset);
	for (i = 0; i < num_configs; i++) {
		enum pin_config_param param = pinconf_to_config_param(configs[i]);
		u32 arg = pinconf_to_config_argument(configs[i]);

		switch (param) {
		case PIN_CONFIG_BIAS_PULL_DOWN:
			val &= ~AX_PULL_UP;
			val |= AX_PULL_DOWN;
			break;
		case PIN_CONFIG_BIAS_PULL_UP:
			val &= ~AX_PULL_DOWN;
			val |= AX_PULL_UP;
			break;
		case PIN_CONFIG_BIAS_DISABLE:
			val &= ~(AX_PULL_DOWN | AX_PULL_UP);
			break;
		case PIN_CONFIG_DRIVE_STRENGTH:
			val &= ~AX_DRIVE_STRENGTH;
			val |= arg & AX_DRIVE_STRENGTH;
			break;
		case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
			val &= ~AX_SCHMITT_ENABLE;
			if (arg)
				val |= AX_SCHMITT_ENABLE;
			break;
		default:
			spin_unlock_irqrestore(&axpctl->lock, flags);
			return -ENOTSUPP;
		}
	}
	ax_pinctrl_writel(axpctl, data->offset, val);
	spin_unlock_irqrestore(&axpctl->lock, flags);
	return 0;
}

static const struct pinconf_ops ax_pinconf_ops = {
	.pin_config_set = ax_pin_config_set,
	.pin_config_get = ax_pin_config_get,
	.is_generic = true,
};

static int ax_pinctrl_build_state(struct platform_device *pdev)
{
	struct ax_pinctrl *axpctl = platform_get_drvdata(pdev);
	struct axera_pinctrl_soc_info *info = axpctl->info;
	struct pinctrl_dev *pctldev = axpctl->pctldev;
	int i;

	for (i = 0; i < info->npins; i++) {
		const struct pinctrl_pin_desc *pindesc = info->pins + i;
		struct axera_pin_data *data = pindesc->drv_data;
		struct axera_mux_desc *mux;
		int ret;

		ret = pinctrl_generic_add_group(pctldev, pindesc->name,
						&pindesc->number, 1, NULL);
		if (ret < 0)
			return ret;

		if (!data)
			continue;

		for (mux = data->muxes; mux->name; mux++) {
			const char **groups;

			groups = devm_kzalloc(&pdev->dev, sizeof(*groups), GFP_KERNEL);
			if (!groups)
				return -ENOMEM;
			groups[0] = pindesc->name;
			ret = pinmux_generic_add_function(pctldev, mux->name,
							  groups, 1, NULL);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

int axera_pinctrl_init(struct platform_device *pdev,
		       struct axera_pinctrl_soc_info *info)
{
	struct pinctrl_desc *pctldesc;
	struct ax_pinctrl *axpctl;
	int ret;

	axpctl = devm_kzalloc(&pdev->dev, sizeof(*axpctl), GFP_KERNEL);
	if (!axpctl)
		return -ENOMEM;

	spin_lock_init(&axpctl->lock);
	axpctl->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(axpctl->base))
		return PTR_ERR(axpctl->base);
	axpctl->base2 = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(axpctl->base2))
		return PTR_ERR(axpctl->base2);

	axpctl->dphytx = syscon_regmap_lookup_by_phandle_optional(
		pdev->dev.of_node, "axera,dphytx-syscon");
	if (IS_ERR(axpctl->dphytx))
		return dev_err_probe(&pdev->dev, PTR_ERR(axpctl->dphytx),
				     "failed to get D-PHY TX syscon\n");
	axpctl->dphytx_reset = devm_reset_control_get_optional_exclusive(
		&pdev->dev, "dphytx");
	if (IS_ERR(axpctl->dphytx_reset))
		return dev_err_probe(&pdev->dev, PTR_ERR(axpctl->dphytx_reset),
				     "failed to get D-PHY TX reset\n");

	axpctl->dev = &pdev->dev;
	axpctl->info = info;

	pctldesc = devm_kzalloc(&pdev->dev, sizeof(*pctldesc), GFP_KERNEL);
	if (!pctldesc)
		return -ENOMEM;

	pctldesc->name = dev_name(&pdev->dev);
	pctldesc->owner = THIS_MODULE;
	pctldesc->pins = info->pins;
	pctldesc->npins = info->npins;
	pctldesc->pctlops = &ax_pinctrl_ops;
	pctldesc->pmxops = &ax_pinmux_ops;
	pctldesc->confops = &ax_pinconf_ops;

	axpctl->pctldev = devm_pinctrl_register(&pdev->dev, pctldesc, axpctl);
	if (IS_ERR(axpctl->pctldev))
		return PTR_ERR(axpctl->pctldev);

	platform_set_drvdata(pdev, axpctl);

	ret = ax_pinctrl_build_state(pdev);
	if (ret) {
		dev_err(&pdev->dev, "failed to build pinctrl state: %d\n", ret);
		return ret;
	}

	dev_info(&pdev->dev, "initialized AX620E pinctrl\n");
	return 0;
}
EXPORT_SYMBOL_GPL(axera_pinctrl_init);
