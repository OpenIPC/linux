// SPDX-License-Identifier: GPL-2.0
/*
 * Ingenic T23 Low-Power RISC-V Coprocessor Platform Core Driver
 *
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/mfd/syscon.h>
#include <linux/io.h>
#include "t23_coproc.h"
#include "remoteproc_internal.h"

static irqreturn_t t23_coproc_isr(int irq, void *dev_id)
{
	struct t23_coproc_priv *priv = dev_id;
	u32 status;

	status = readl(priv->aux_base + REG_HOST_IRQ_STATUS);
	if (!status && !readl(priv->aux_base + REG_HOST_DOORBELL))
		return IRQ_NONE;

	writel(status, priv->aux_base + REG_HOST_IRQ_CLEAR);
	writel(0, priv->aux_base + REG_HOST_DOORBELL);

	return IRQ_HANDLED;
}

static int t23_coproc_probe(struct platform_device *pdev)
{
	const char *fw_name = "ingenic/t23_riscv.elf";
	struct device *dev = &pdev->dev;
	struct t23_coproc_priv *priv;
	struct rproc *rproc;
	struct resource *res;
	int ret;

	ret = rproc_of_parse_firmware(dev, 0, &fw_name);
	if (ret < 0 && ret != -EINVAL)
		return ret;

	rproc = rproc_alloc(dev, dev_name(dev), &t23_coproc_rproc_ops,
			    fw_name, sizeof(*priv));
	if (!rproc) {
		dev_err(dev, "failed to allocate remoteproc context\n");
		return -ENOMEM;
	}

	priv = rproc->priv;
	priv->dev = dev;
	priv->rproc = rproc;
	rproc->auto_boot = false;
	mutex_init(&priv->lock);

	/* AUX control aperture */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "aux");
	if (!res)
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->aux_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(priv->aux_base)) {
		ret = PTR_ERR(priv->aux_base);
		goto err_free_rproc;
	}

	/* TCSM shared memory aperture */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "tcsm");
	if (!res)
		res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	priv->tcsm_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(priv->tcsm_base)) {
		ret = PTR_ERR(priv->tcsm_base);
		goto err_free_rproc;
	}

	priv->clk = devm_clk_get(dev, "coproc");
	if (IS_ERR(priv->clk))
		priv->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(priv->clk)) {
		ret = PTR_ERR(priv->clk);
		dev_err(dev, "failed to acquire CLK_RISCV: %d\n", ret);
		goto err_free_rproc;
	}

	priv->cpm_map = syscon_regmap_lookup_by_phandle(dev->of_node, "ingenic,cpm");
	if (IS_ERR(priv->cpm_map)) {
		dev_warn(dev, "CPM syscon regmap not found, relying on direct register access\n");
		priv->cpm_map = NULL;
	}

	priv->irq = platform_get_irq(pdev, 0);
	if (priv->irq < 0) {
		ret = priv->irq;
		dev_err(dev, "failed to get IRQ: %d\n", ret);
		goto err_free_rproc;
	}

	ret = devm_request_irq(dev, priv->irq, t23_coproc_isr, 0, dev_name(dev), priv);
	if (ret) {
		dev_err(dev, "failed to request IRQ %d: %d\n", priv->irq, ret);
		goto err_free_rproc;
	}
	disable_irq(priv->irq);

	platform_set_drvdata(pdev, priv);

	ret = rproc_add(rproc);
	if (ret) {
		dev_err(dev, "failed to add remoteproc: %d\n", ret);
		goto err_free_rproc;
	}

	dev_dbg(dev, "Ingenic T23 Low-Power Coprocessor driver initialized\n");
	return 0;

err_free_rproc:
	rproc_free(rproc);
	return ret;
}

static void t23_coproc_remove(struct platform_device *pdev)
{
	struct t23_coproc_priv *priv = platform_get_drvdata(pdev);

	if (!priv)
		return;

	if (priv->running)
		rproc_shutdown(priv->rproc);
	rproc_del(priv->rproc);
	rproc_free(priv->rproc);
	dev_dbg(&pdev->dev, "Ingenic T23 Coprocessor unregistered\n");
}

static const struct of_device_id t23_coproc_of_match[] = {
	{ .compatible = "ingenic,t23-coprocessor" },
	{ .compatible = "ingenic,t23-riscv" },
	{ }
};
MODULE_DEVICE_TABLE(of, t23_coproc_of_match);

static struct platform_driver t23_coproc_driver = {
	.probe = t23_coproc_probe,
	.remove = t23_coproc_remove,
	.driver = {
		.name = "ingenic-t23-coproc",
		.of_match_table = t23_coproc_of_match,
	},
};
module_platform_driver(t23_coproc_driver);

MODULE_AUTHOR("Ingenic Semiconductor Co., Ltd.");
MODULE_AUTHOR("Difan Zhang");
MODULE_DESCRIPTION("Ingenic T23 low-power coprocessor remoteproc platform driver");
MODULE_LICENSE("GPL");
