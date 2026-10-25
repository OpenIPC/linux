// SPDX-License-Identifier: GPL-2.0
/*
 * Fullhan FH8626 eFuse controller, read-only access to the 64-byte array
 *
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/nvmem-provider.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#define FH_EFUSE_REG_CMD		0x00
#define FH_EFUSE_REG_CONFIG		0x04
#define FH_EFUSE_REG_MATCH_KEY		0x08
#define FH_EFUSE_REG_DOUT		0x28
#define FH_EFUSE_REG_STATUS0		0x2c
#define FH_EFUSE_REG_STATUS1		0x30
#define FH_EFUSE_REG_STATUS2		0x34

#define FH_EFUSE_CONFIG_POWER_ON	BIT(27)

#define FH_EFUSE_CMD_WFLAG_AUTO		8
#define FH_EFUSE_CMD_LOAD_USER		1
#define FH_EFUSE_CMD_READ		3
/*
 * Completion is reported in STATUS0 at the bit whose number equals the
 * command code just issued.
 */
#define FH_EFUSE_STATUS_WFLAG		8

#define FH_EFUSE_STATUS_COMPLETE(bit)	BIT(bit)
#define FH_EFUSE_TOTAL_SIZE		64

struct fh8626_efuse {
	struct device *dev;
	void __iomem *base;
	struct clk *clk;
	struct mutex lock;
};

static int fh8626_efuse_wait_complete(struct fh8626_efuse *efuse,
				      unsigned int bit)
{
	unsigned int count = 1000;
	u32 val;

	do {
		val = readl_relaxed(efuse->base + FH_EFUSE_REG_STATUS0);
		/* Preserve the controller's sample and completion delays. */
		udelay(10);
		if (val & FH_EFUSE_STATUS_COMPLETE(bit)) {
			udelay(10);
			return 0;
		}
	} while (--count);

	dev_err(efuse->dev,
		"timed out waiting for command %u (status0: 0x%08x)\n",
		bit, val);
	return -ETIMEDOUT;
}

static int fh8626_efuse_refresh(struct fh8626_efuse *efuse)
{
	int ret;

	writel_relaxed(FH_EFUSE_CMD_WFLAG_AUTO, efuse->base + FH_EFUSE_REG_CMD);
	ret = fh8626_efuse_wait_complete(efuse, FH_EFUSE_STATUS_WFLAG);
	if (ret)
		return ret;

	writel_relaxed(FH_EFUSE_CMD_LOAD_USER, efuse->base + FH_EFUSE_REG_CMD);
	return fh8626_efuse_wait_complete(efuse, FH_EFUSE_CMD_LOAD_USER);
}

static int fh8626_efuse_read(void *context, unsigned int offset,
			     void *val, size_t bytes)
{
	struct fh8626_efuse *efuse = context;
	u8 *buf = val;
	int ret = 0;
	size_t i;

	if (!bytes)
		return 0;

	if (offset >= FH_EFUSE_TOTAL_SIZE ||
	    bytes > FH_EFUSE_TOTAL_SIZE - offset)
		return -EINVAL;

	mutex_lock(&efuse->lock);

	ret = clk_prepare_enable(efuse->clk);
	if (ret) {
		dev_err(efuse->dev, "failed to enable efuse clock: %d\n", ret);
		goto out_unlock;
	}

	writel_relaxed(readl_relaxed(efuse->base + FH_EFUSE_REG_CONFIG) |
		       FH_EFUSE_CONFIG_POWER_ON,
		       efuse->base + FH_EFUSE_REG_CONFIG);
	udelay(10);

	ret = fh8626_efuse_refresh(efuse);
	if (ret)
		goto out_power_off;

	for (i = 0; i < bytes; i++) {
		unsigned int entry = offset + i;
		u32 cmd = (entry << 4) | FH_EFUSE_CMD_READ;

		/* The match key is zero for plain reads. */
		writel_relaxed(0, efuse->base + FH_EFUSE_REG_MATCH_KEY);
		writel_relaxed(cmd, efuse->base + FH_EFUSE_REG_CMD);

		ret = fh8626_efuse_wait_complete(efuse, FH_EFUSE_CMD_READ);
		if (ret)
			break;

		buf[i] = readl_relaxed(efuse->base + FH_EFUSE_REG_DOUT) & 0xff;
	}

out_power_off:
	writel_relaxed(readl_relaxed(efuse->base + FH_EFUSE_REG_CONFIG) &
		       ~FH_EFUSE_CONFIG_POWER_ON,
		       efuse->base + FH_EFUSE_REG_CONFIG);

	clk_disable_unprepare(efuse->clk);

out_unlock:
	mutex_unlock(&efuse->lock);
	return ret;
}

static const struct nvmem_config fh8626_nvmem_config = {
	.name = "fh8626-efuse",
	.read_only = true,
	.word_size = 1,
	.stride = 1,
	.size = FH_EFUSE_TOTAL_SIZE,
	.reg_read = fh8626_efuse_read,
};

static int fh8626_efuse_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct fh8626_efuse *efuse;
	struct nvmem_device *nvmem;
	struct nvmem_config config;
	int ret;

	efuse = devm_kzalloc(dev, sizeof(*efuse), GFP_KERNEL);
	if (!efuse)
		return -ENOMEM;

	efuse->dev = dev;
	mutex_init(&efuse->lock);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	efuse->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(efuse->base))
		return PTR_ERR(efuse->base);

	efuse->clk = devm_clk_get(dev, "efuse");
	if (IS_ERR(efuse->clk)) {
		ret = PTR_ERR(efuse->clk);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "failed to get efuse clock: %d\n", ret);
		return ret;
	}

	config = fh8626_nvmem_config;
	config.dev = dev;
	config.priv = efuse;

	nvmem = devm_nvmem_register(dev, &config);
	if (IS_ERR(nvmem)) {
		ret = PTR_ERR(nvmem);
		dev_err(dev, "failed to register nvmem device: %d\n", ret);
		return ret;
	}

	return 0;
}

static const struct of_device_id fh8626_efuse_of_match[] = {
	{ .compatible = "fullhan,fh8626v100-efuse" },
	{ .compatible = "fullhan,fh8626-efuse" },
	{ }
};
MODULE_DEVICE_TABLE(of, fh8626_efuse_of_match);

static struct platform_driver fh8626_efuse_driver = {
	.probe = fh8626_efuse_probe,
	.driver = {
		.name = "fh8626-efuse",
		.of_match_table = fh8626_efuse_of_match,
	},
};
module_platform_driver(fh8626_efuse_driver);

MODULE_DESCRIPTION("Fullhan FH8626 eFuse / OTP NVMEM driver");
MODULE_AUTHOR("Difan Zhang");
MODULE_LICENSE("GPL v2");
