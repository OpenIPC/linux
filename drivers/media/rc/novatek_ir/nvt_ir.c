/*
 * Novatek IR remote receiver - interface and resource allocation
 *
 * Copyright Novatek Microelectronics Corp. 2020. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/clk.h>

#include <media/rc-core.h>

#include "nvt_ir.h"

#include <plat/top.h>

#define DRIVER_VERSION		"2.0.2"

#define DRIVER_NAME		"nvt_ir"

/**
 * This function gets called two different ways, one way is from
 * rc_register_device, for initial protocol selection/setup, and the other is
 * via a userspace-initiated protocol change request, either by direct sysfs
 * prodding or by something like ir-keytable.
 */
static int nvt_ir_change_protocol(struct rc_dev *rc, u64 *rc_type)
{
	struct nvt_ir *ir = rc->priv;

	spin_lock(&ir->lock);

	if (*rc_type && !(*rc_type & rc->allowed_protocols)) {
		dev_err(&ir->rc->dev, "Looks like you're trying to use an IR protocol this module does not support, rc_type(%llx)\n", *rc_type);
		return -EINVAL;
	} else {
		ir->rc_type = *rc_type;
	}

	nvt_ir_protocol_config(ir);

	spin_unlock(&ir->lock);

	return 0;
}

static irqreturn_t nvt_ir_irq(int irqno, void *dev_id)
{
	struct nvt_ir *ir = dev_id;

	spin_lock(&ir->lock);

	nvt_ir_irq_cb(ir);

	spin_unlock(&ir->lock);

	return IRQ_HANDLED;
}

static int nvt_ir_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct resource *res;
	const char *map_name;
	struct nvt_ir *ir;
	int ret;

	ir = devm_kzalloc(dev, sizeof(struct nvt_ir), GFP_KERNEL);
	if (!ir)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ir->reg = devm_ioremap_resource(dev, res);
	if (IS_ERR(ir->reg)) {
		dev_err(dev, "failed to map registers\n");
		return PTR_ERR(ir->reg);
	}

	if (nvt_get_chip_id() == CHIP_NA51084) {
		ir->clk = clk_get(NULL, "remote_528");
		if (IS_ERR(ir->clk)) {
			dev_err(dev, "clk remote_528 not found\n");
			return PTR_ERR(ir->clk);
		}
	} else {
		ir->clk = clk_get(&pdev->dev, dev_name(&pdev->dev));
		if (IS_ERR(ir->clk)) {
			dev_err(dev, "clk %s not found\n", dev_name(&pdev->dev));
			return PTR_ERR(ir->clk);
		}
	}

	ir->irq = platform_get_irq(pdev, 0);
	if (ir->irq < 0) {
		dev_err(dev, "no irq resource\n");
		return ir->irq;
	}

	ir->rc = devm_rc_allocate_device(dev, RC_DRIVER_SCANCODE);
	if (!ir->rc) {
		dev_err(dev, "failed to allocate rc device\n");
		return -ENOMEM;
	}

	ir->rc->priv = ir;
	ir->rc->device_name = DRIVER_NAME;
	ir->rc->input_phys = DRIVER_NAME "/input0";
	ir->rc->input_id.bustype = BUS_HOST;
	map_name = of_get_property(node, "linux,rc-map-name", NULL);
	ir->rc->map_name = map_name ? map_name : RC_MAP_EMPTY;
	if (nvt_get_chip_id() == CHIP_NA51084) {
		ir->rc->allowed_protocols = (RC_PROTO_BIT_UNKNOWN |
			RC_PROTO_BIT_OTHER |
			RC_PROTO_BIT_NEC | RC_PROTO_BIT_NECX | RC_PROTO_BIT_NEC32 |
			RC_PROTO_BIT_JVC |
			RC_PROTO_BIT_SHARP |
			RC_PROTO_BIT_RC5 | RC_PROTO_BIT_RC5X_20);
	} else {
		ir->rc->allowed_protocols = (RC_PROTO_BIT_UNKNOWN |
			RC_PROTO_BIT_NEC | RC_PROTO_BIT_NECX | RC_PROTO_BIT_NEC32 |
			RC_PROTO_BIT_JVC |
			RC_PROTO_BIT_SHARP);
	}

	ir->rc->change_protocol = nvt_ir_change_protocol;
	ir->rc->driver_name = DRIVER_NAME;

	spin_lock_init(&ir->lock);
	spin_lock_init(&ir->write_lock);

	nvt_ir_platform_init(ir);

	platform_set_drvdata(pdev, ir);

	ret = devm_rc_register_device(dev, ir->rc);
	if (ret) {
		dev_err(dev, "failed to register rc device\n");
		return ret;
	}

	ret = clk_prepare_enable(ir->clk);
	if (ret) {
		dev_err(dev, "failed to enable clk\n");
		return ret;
	}

	ret = devm_request_irq(dev, ir->irq, nvt_ir_irq, 0, "nvt_ir", ir);
	if (ret) {
		dev_err(dev, "failed to request irq\n");
		return ret;
	}

	dev_info(dev, "platform driver probed\n");

	return 0;
}

static int nvt_ir_remove(struct platform_device *pdev)
{
	struct nvt_ir *ir = platform_get_drvdata(pdev);
	unsigned long flags;

	/* Disable the raw receiver */
	spin_lock_irqsave(&ir->lock, flags);

	nvt_ir_disable(ir);

	spin_unlock_irqrestore(&ir->lock, flags);

	rc_unregister_device(ir->rc);

	return 0;
}

static const struct of_device_id nvt_ir_match[] = {
	{ .compatible = "nvt,nvt_irdet" },
	{ .compatible = "nvt,nvt_remote" },
	{ },
};

static struct platform_driver nvt_ir_driver = {
	.probe		= nvt_ir_probe,
	.remove		= nvt_ir_remove,
	.driver = {
		.name		= DRIVER_NAME,
		.of_match_table	= nvt_ir_match,
	},
};

module_platform_driver(nvt_ir_driver);

MODULE_DESCRIPTION("Novatek IR remote receiver driver");
MODULE_VERSION(DRIVER_VERSION);
MODULE_AUTHOR("Shawn Chou <shawn_chou@novatek.com.tw>");
MODULE_LICENSE("GPL v2");
