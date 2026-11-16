// SPDX-License-Identifier: GPL-2.0
/*
 * Ingenic T23 CPM reset controller
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/delay.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset-controller.h>
#include <dt-bindings/reset/ingenic,t23-reset.h>

#define T23_SRBC		0xc4
#define T23_ISP_ACK		BIT(20)
#define T23_ISP_STOP		BIT(21)
#define T23_ISP_RESET		BIT(22)
#define T23_HELIX_ACK		BIT(29)
#define T23_HELIX_STOP		BIT(30)
#define T23_HELIX_RESET		BIT(31)

struct t23_reset {
	struct reset_controller_dev rcdev;
	struct regmap *map;
	struct mutex lock;
};

static int t23_helix_reset(struct t23_reset *reset)
{
	unsigned int saved, value;
	unsigned int mask = T23_HELIX_STOP | T23_HELIX_RESET;
	int ret, restore_ret;

	ret = regmap_read(reset->map, T23_SRBC, &saved);
	if (ret)
		return ret;
	ret = regmap_write_bits(reset->map, T23_SRBC,
				T23_HELIX_STOP, T23_HELIX_STOP);
	if (ret)
		goto restore;
	ret = regmap_read_poll_timeout(reset->map, T23_SRBC, value,
				       value & T23_HELIX_ACK, 20, 655340);
	if (ret)
		goto restore;
	ret = regmap_write_bits(reset->map, T23_SRBC, mask,
				(saved & T23_HELIX_STOP) | T23_HELIX_RESET);
restore:
	restore_ret = regmap_write_bits(reset->map, T23_SRBC, mask, saved & mask);
	return ret ? ret : restore_ret;
}

static int t23_reset_reset(struct reset_controller_dev *rcdev,
			   unsigned long id)
{
	struct t23_reset *reset = container_of(rcdev, struct t23_reset, rcdev);
	unsigned int value, i;
	int ret;

	if (id != T23_RESET_ISP && id != T23_RESET_HELIX)
		return -EINVAL;
	mutex_lock(&reset->lock);
	if (id == T23_RESET_HELIX) {
		ret = t23_helix_reset(reset);
		goto unlock;
	}
	ret = regmap_write_bits(reset->map, T23_SRBC, T23_ISP_STOP,
				T23_ISP_STOP);
	if (ret)
		goto unlock;
	/* Stop new bus requests before pulsing the ISP reset. */
	for (i = 0; i < 500; i++) {
		ret = regmap_read(reset->map, T23_SRBC, &value);
		if (ret)
			goto release_stop;
		if (value & T23_ISP_ACK)
			break;
		msleep(2);
	}
	if (i == 500) {
		ret = -ETIMEDOUT;
		goto release_stop;
	}
	ret = regmap_write_bits(reset->map, T23_SRBC,
				T23_ISP_STOP | T23_ISP_RESET, T23_ISP_RESET);
	if (ret)
		goto release_stop;
	ret = regmap_write_bits(reset->map, T23_SRBC, T23_ISP_RESET, 0);
	goto unlock;

release_stop:
	regmap_write_bits(reset->map, T23_SRBC, T23_ISP_STOP, 0);
unlock:
	mutex_unlock(&reset->lock);
	return ret;
}

static const struct reset_control_ops t23_reset_ops = {
	.reset = t23_reset_reset,
};

static int t23_reset_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *parent;
	struct t23_reset *reset;

	reset = devm_kzalloc(dev, sizeof(*reset), GFP_KERNEL);
	if (!reset)
		return -ENOMEM;
	parent = of_get_parent(dev->of_node);
	reset->map = syscon_node_to_regmap(parent);
	of_node_put(parent);
	if (IS_ERR(reset->map))
		return PTR_ERR(reset->map);
	mutex_init(&reset->lock);
	reset->rcdev.owner = THIS_MODULE;
	reset->rcdev.ops = &t23_reset_ops;
	reset->rcdev.of_node = dev->of_node;
	reset->rcdev.nr_resets = 2;
	return devm_reset_controller_register(dev, &reset->rcdev);
}

static const struct of_device_id t23_reset_of_match[] = {
	{ .compatible = "ingenic,t23-reset" },
	{ }
};
MODULE_DEVICE_TABLE(of, t23_reset_of_match);

static struct platform_driver t23_reset_driver = {
	.probe = t23_reset_probe,
	.driver = {
		.name = "ingenic-t23-reset",
		.of_match_table = t23_reset_of_match,
	},
};
module_platform_driver(t23_reset_driver);

MODULE_DESCRIPTION("Ingenic T23 CPM reset controller");
MODULE_AUTHOR("Difan Zhang");
MODULE_LICENSE("GPL");
