/*
 * Copyright (C) 2022 axrea Corporation
 *
 */
#include <linux/err.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include <linux/regmap.h>
#include <linux/types.h>
#include <linux/of_device.h>
#include <linux/module.h>
#include <linux/idr.h>

struct axera_reset_data {
	unsigned int dev_id;
	unsigned int reg;
	unsigned int flags;
	unsigned int rst_set_reg;
	unsigned int rst_clr_bit;
	unsigned int rst_clr_reg;

	unsigned int clk_reg;
	unsigned int clk_bit;
	unsigned int clk_set_reg;
	unsigned int clk_set_bit;
	unsigned int clk_clr_reg;
	unsigned int clk_clr_bit;
	unsigned int need_clk;
};

struct axera_reset_priv {
	struct reset_controller_dev rcdev;
	struct device *dev;
	struct regmap *regmap;
	struct axera_reset_data *data;
	struct idr idr;
};

#define to_axera_reset_priv(p)		\
	container_of((p), struct axera_reset_priv, rcdev)

static int axera_clk_close(struct reset_controller_dev *rcdev, unsigned long id)
{
	unsigned int val;
	int ret, asserted;
	struct axera_reset_priv *priv = to_axera_reset_priv(rcdev);
	struct axera_reset_data *p;
	unsigned int mask;

	p = idr_find(&priv->idr, id);
	ret = regmap_read(priv->regmap, p->clk_reg, &val);
	if (ret)
		return ret;

	asserted = ! !(val & BIT(p->clk_bit));
        /* need close clk */
	if (asserted == 1) {
		mask = BIT(p->clk_clr_bit);
		ret = regmap_write(priv->regmap, p->clk_clr_reg, mask);
		p->need_clk = 1;
	} else {
		p->need_clk = 0;
	}

	return ret;
}

static int axera_clk_restore(struct reset_controller_dev *rcdev,
			     unsigned long id)
{
	struct axera_reset_priv *priv = to_axera_reset_priv(rcdev);
	const struct axera_reset_data *p;
	int ret = 0;
	unsigned int mask;
	p = idr_find(&priv->idr, id);
        /* close clk--need open */
	if (p->need_clk == 1) {
		mask = BIT(p->clk_set_bit);
		ret = regmap_write(priv->regmap, p->clk_set_reg, mask);
	}
	return ret;
}

static int axera_reset_update(struct reset_controller_dev *rcdev,
			      unsigned long id, int flag)
{
	struct axera_reset_priv *priv = to_axera_reset_priv(rcdev);
	const struct axera_reset_data *p;
	unsigned int mask, val;

	p = idr_find(&priv->idr, id);
	if (!p)
		return -EINVAL;

	mask = BIT(p->dev_id);

	if (flag)
		val = mask;
	else
		val = ~mask;

	return regmap_write_bits(priv->regmap, p->reg, mask, val);
}

static int axera_reset_updatenew(struct reset_controller_dev *rcdev,
				 unsigned long id, int flag)
{
	struct axera_reset_priv *priv = to_axera_reset_priv(rcdev);
	const struct axera_reset_data *p;
	unsigned int mask;
	int ret;
	p = idr_find(&priv->idr, id);
	if (!p)
		return -EINVAL;

	if (flag == 1) {

		mask = BIT(p->dev_id);
		ret = regmap_write(priv->regmap, p->rst_set_reg, mask);
	} else {
		mask = BIT(p->rst_clr_bit);
		ret = regmap_write(priv->regmap, p->rst_clr_reg, mask);
	}
	return ret;
}

static int axera_reset_updatenewclk(struct reset_controller_dev *rcdev,
				    unsigned long id, int flag)
{
	struct axera_reset_priv *priv = to_axera_reset_priv(rcdev);
	const struct axera_reset_data *p;
	unsigned int mask;
	int ret;
	p = idr_find(&priv->idr, id);
	if (!p)
		return -EINVAL;

	if (flag == 1) {

		mask = BIT(p->dev_id);
		ret = regmap_write(priv->regmap, p->rst_set_reg, mask);
	} else {

		ret = axera_clk_close(rcdev, id);
		mask = BIT(p->rst_clr_bit);
		ret = regmap_write(priv->regmap, p->rst_clr_reg, mask);
		ret = axera_clk_restore(rcdev, id);
	}

	return ret;
}

static int axera_reset_assert(struct reset_controller_dev *rcdev,
			      unsigned long id)
{
	struct axera_reset_priv *priv = to_axera_reset_priv(rcdev);
	const struct axera_reset_data *p;
	int ret;
	p = idr_find(&priv->idr, id);
	if (!p)
		return -EINVAL;

	if (priv->rcdev.of_reset_n_cells == 10) {
		ret = axera_reset_updatenewclk(rcdev, id, 1);
	} else if (priv->rcdev.of_reset_n_cells == 3) {
		ret = axera_reset_update(rcdev, id, p->flags);
	} else if (priv->rcdev.of_reset_n_cells == 4) {
		ret = axera_reset_updatenew(rcdev, id, 1);
	}
	return ret;
}

static int axera_reset_deassert(struct reset_controller_dev *rcdev,
				unsigned long id)
{

	struct axera_reset_priv *priv = to_axera_reset_priv(rcdev);
	const struct axera_reset_data *pd;
	int ret;
	pd = idr_find(&priv->idr, id);
	if (!pd)
		return -EINVAL;

	if (priv->rcdev.of_reset_n_cells == 10) {
		ret = axera_reset_updatenewclk(rcdev, id, 0);
	} else if (priv->rcdev.of_reset_n_cells == 3) {
		ret = axera_reset_update(rcdev, id, pd->flags == 0 ? 1 : 0);
	} else if (priv->rcdev.of_reset_n_cells == 4) {
		ret = axera_reset_updatenew(rcdev, id, 0);
	}
	return ret;
}

static int axera_reset_of_xlate(struct reset_controller_dev *rcdev,
				const struct of_phandle_args *reset_spec)
{
	struct axera_reset_priv *priv = to_axera_reset_priv(rcdev);
	struct axera_reset_data *control;

	control = devm_kzalloc(priv->dev, sizeof(*control), GFP_KERNEL);

	if (!control)
		return -EINVAL;

	switch (reset_spec->args_count) {
	case 3:
		control->dev_id = reset_spec->args[0];
		control->reg = reset_spec->args[1];
		control->flags = reset_spec->args[2];
		break;
	case 4:
		control->dev_id = reset_spec->args[0];
		control->rst_set_reg = reset_spec->args[1];
		control->rst_clr_bit = reset_spec->args[2];
		control->rst_clr_reg = reset_spec->args[3];
		break;
	case 10:
		control->dev_id = reset_spec->args[0];
		control->rst_set_reg = reset_spec->args[1];
		control->rst_clr_bit = reset_spec->args[2];
		control->rst_clr_reg = reset_spec->args[3];
		control->clk_reg = reset_spec->args[4];
		control->clk_bit = reset_spec->args[5];
		control->clk_set_reg = reset_spec->args[6];
		control->clk_set_bit = reset_spec->args[7];
		control->clk_clr_reg = reset_spec->args[8];
		control->clk_clr_bit = reset_spec->args[9];
		break;

	default:
		return -EINVAL;
	}
	rcdev->of_reset_n_cells = reset_spec->args_count;

	return idr_alloc(&priv->idr, control, 0, 0, GFP_KERNEL);
}

static const struct reset_control_ops axera_reset_ops = {
	.assert = axera_reset_assert,
	.deassert = axera_reset_deassert,
};

static int axera_reset_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct axera_reset_priv *priv;
	struct regmap *regmap;
	priv = devm_kzalloc(dev, sizeof(struct axera_reset_priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	regmap = syscon_node_to_regmap(np);

	if (IS_ERR(regmap)) {
		dev_err(dev, "failed to get regmap (error %ld)\n",
			PTR_ERR(regmap));
		return PTR_ERR(regmap);
	}
	priv->rcdev.ops = &axera_reset_ops;
	priv->rcdev.owner = dev->driver->owner;
	priv->rcdev.of_node = dev->of_node;
	priv->dev = dev;
	priv->regmap = regmap;
	priv->rcdev.of_xlate = axera_reset_of_xlate;
	idr_init(&priv->idr);
	platform_set_drvdata(pdev, priv);
	return reset_controller_register(&priv->rcdev);
}

static const struct of_device_id axera_reset_match[] = {
	{.compatible = "axera,axera_reset_match",},
	{ /* axera */ }
};

MODULE_DEVICE_TABLE(of, axera_reset_match);

static struct platform_driver axera_reset_driver = {
	.probe = axera_reset_probe,
	.driver = {
		   .name = "axera-reset",
		   .of_match_table = axera_reset_match,
		   },
};

static int __init axera_reset_init(void)
{
        return platform_driver_register(&axera_reset_driver);
}

postcore_initcall(axera_reset_init);
MODULE_AUTHOR("axera <axera@axera-tech.com>");
MODULE_DESCRIPTION("axera Reset Controller Driver");
MODULE_LICENSE("GPL");
