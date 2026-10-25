// SPDX-License-Identifier: GPL-2.0
/*
 * FH8626V100 standard-wire SSI master.
 * The Fullhan extension at 0xf4 is not generic DesignWare enhanced SPI
 * and is left untouched.
 * Copyright (C) 2026 Difan Zhang
 */
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/spi/spi.h>

#define FH_CTRL0		0x00
#define FH_ENABLE	0x08
#define FH_SELECT	0x10
#define FH_BAUD		0x14
#define FH_RX_LEVEL	0x24
#define FH_STATUS	0x28
#define FH_INT_MASK	0x2c
#define FH_DMA_CTRL	0x4c
#define FH_DATA		0x60
#define FH_RX_DELAY	0xf0
#define FH_FIFO_DEPTH	32U

struct fh_spi {
	void __iomem *base;
	struct clk *clk;
	int cs_gpio;
};

static void fh_spi_stop(struct fh_spi *fh)
{
	gpio_set_value_cansleep(fh->cs_gpio, 1);
	writel(0, fh->base + FH_SELECT);
	writel(0, fh->base + FH_ENABLE);
}

static int fh_spi_setup(struct spi_device *spi)
{
	struct fh_spi *fh = spi_master_get_devdata(spi->master);

	if (spi->chip_select || spi->cs_gpio != fh->cs_gpio ||
	    (spi->mode & ~(SPI_CPOL | SPI_CPHA)))
		return -EINVAL;
	return gpio_direction_output(fh->cs_gpio, 1);
}

static int fh_spi_prepare_message(struct spi_master *master,
				  struct spi_message *message)
{
	struct fh_spi *fh = spi_master_get_devdata(master);
	u32 ctrl = 7; /* 8-bit full duplex */

	/* Set clock polarity before the message core asserts GPIO CS. */
	if (message->spi->mode & SPI_CPHA)
		ctrl |= BIT(6);
	if (message->spi->mode & SPI_CPOL)
		ctrl |= BIT(7);
	writel(0, fh->base + FH_ENABLE);
	writel(ctrl, fh->base + FH_CTRL0);
	writel(1, fh->base + FH_RX_DELAY);
	writel(0, fh->base + FH_INT_MASK);
	writel(0, fh->base + FH_DMA_CTRL);
	return 0;
}

static int fh_spi_transfer_one(struct spi_master *master,
			       struct spi_device *spi,
			       struct spi_transfer *xfer)
{
	struct fh_spi *fh = spi_master_get_devdata(master);
	const u8 *tx = xfer->tx_buf;
	u8 *rx = xfer->rx_buf;
	unsigned long rate = clk_get_rate(fh->clk);
	unsigned int offset = 0;
	u32 div, value;
	int ret;

	if (xfer->bits_per_word != 8 || !xfer->speed_hz || !rate)
		return -EINVAL;
	div = DIV_ROUND_UP(rate, xfer->speed_hz);
	div = max_t(u32, 2, (div + 1) & ~1U);
	if (div > 65534)
		return -EINVAL;

	writel(0, fh->base + FH_ENABLE);
	writel(div, fh->base + FH_BAUD);
	writel(1, fh->base + FH_SELECT);
	writel(1, fh->base + FH_ENABLE);

	/*
	 * Never queue more than the FIFO depth, so the RX FIFO cannot
	 * overflow.
	 */
	while (offset < xfer->len) {
		unsigned int chunk = min_t(unsigned int, xfer->len - offset,
					   FH_FIFO_DEPTH);
		unsigned int i;

		for (i = 0; i < chunk; i++)
			writel(tx ? tx[offset + i] : 0xff, fh->base + FH_DATA);

		for (i = 0; i < chunk; i++) {
			ret = readl_poll_timeout(fh->base + FH_RX_LEVEL, value,
						 value != 0, 0, 100000);
			if (ret)
				goto fail;
			value = readl(fh->base + FH_DATA);
			if (rx)
				rx[offset + i] = value;
		}

		offset += chunk;
		cond_resched();
	}
	ret = readl_poll_timeout(fh->base + FH_STATUS, value,
				(value & 5) == 4, 0, 100000);
	if (!ret)
		return 0;
fail:
	fh_spi_stop(fh);
	return ret;
}

static int fh_spi_unprepare_message(struct spi_master *master,
				    struct spi_message *message)
{
	struct fh_spi *fh = spi_master_get_devdata(master);

	/* A final cs_change may retain external GPIO CS. */
	writel(0, fh->base + FH_SELECT);
	writel(0, fh->base + FH_ENABLE);
	return 0;
}

static int fh_spi_probe(struct platform_device *pdev)
{
	struct spi_master *master;
	struct fh_spi *fh;
	struct resource *res;
	enum of_gpio_flags flags;
	unsigned long rate;
	int ret;

	master = spi_alloc_master(&pdev->dev, sizeof(*fh));
	if (!master)
		return -ENOMEM;
	fh = spi_master_get_devdata(master);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	fh->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(fh->base)) {
		ret = PTR_ERR(fh->base);
		goto put_master;
	}
	if (of_gpio_named_count(pdev->dev.of_node, "cs-gpios") != 1) {
		ret = -EINVAL;
		goto put_master;
	}
	fh->cs_gpio = of_get_named_gpio_flags(pdev->dev.of_node,
					     "cs-gpios", 0, &flags);
	if (fh->cs_gpio < 0) {
		ret = fh->cs_gpio;
		goto put_master;
	}
	if (!(flags & OF_GPIO_ACTIVE_LOW) || gpio_cansleep(fh->cs_gpio)) {
		/*
		 * The SPI core drives cs-gpios with the non-sleeping
		 * gpio_set_value().
		 */
		ret = -EINVAL;
		goto put_master;
	}
	ret = devm_gpio_request_one(&pdev->dev, fh->cs_gpio,
				   GPIOF_OUT_INIT_HIGH, dev_name(&pdev->dev));
	if (ret)
		goto put_master;
	fh->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(fh->clk)) {
		ret = PTR_ERR(fh->clk);
		goto put_master;
	}
	ret = clk_prepare_enable(fh->clk);
	if (ret)
		goto put_master;
	rate = clk_get_rate(fh->clk);
	if (!rate) {
		ret = -EINVAL;
		goto disable_clock;
	}
	fh_spi_stop(fh);
	master->dev.of_node = pdev->dev.of_node;
	master->bus_num = -1;
	master->num_chipselect = 1;
	master->mode_bits = SPI_CPOL | SPI_CPHA;
	master->bits_per_word_mask = SPI_BPW_MASK(8);
	master->min_speed_hz = DIV_ROUND_UP(rate, 65534);
	master->max_speed_hz = min_t(unsigned long, rate / 2, 50000000);
	master->setup = fh_spi_setup;
	master->prepare_message = fh_spi_prepare_message;
	master->transfer_one = fh_spi_transfer_one;
	master->unprepare_message = fh_spi_unprepare_message;
	platform_set_drvdata(pdev, master);
	ret = spi_register_master(master);
	if (!ret)
		return 0;
disable_clock:
	clk_disable_unprepare(fh->clk);
put_master:
	spi_master_put(master);
	return ret;
}

static int fh_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master = platform_get_drvdata(pdev);
	struct fh_spi *fh = spi_master_get_devdata(master);

	spi_master_get(master);
	spi_unregister_master(master);
	fh_spi_stop(fh);
	clk_disable_unprepare(fh->clk);
	spi_master_put(master);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int fh_spi_suspend(struct device *dev)
{
	struct spi_master *master = dev_get_drvdata(dev);
	struct fh_spi *fh = spi_master_get_devdata(master);
	int ret = spi_master_suspend(master);

	if (ret)
		return ret;
	fh_spi_stop(fh);
	clk_disable_unprepare(fh->clk);
	return 0;
}

static int fh_spi_resume(struct device *dev)
{
	struct spi_master *master = dev_get_drvdata(dev);
	struct fh_spi *fh = spi_master_get_devdata(master);
	int ret = clk_prepare_enable(fh->clk);

	if (ret)
		return ret;
	fh_spi_stop(fh);
	ret = spi_master_resume(master);
	if (ret)
		clk_disable_unprepare(fh->clk);
	return ret;
}
#endif
static SIMPLE_DEV_PM_OPS(fh_spi_pm_ops, fh_spi_suspend, fh_spi_resume);

static const struct of_device_id fh_spi_match[] = {
	{ .compatible = "fullhan,fh8626v100-spi" },
	{ }
};
MODULE_DEVICE_TABLE(of, fh_spi_match);

static struct platform_driver fh_spi_driver = {
	.probe = fh_spi_probe,
	.remove = fh_spi_remove,
	.driver = {
		.name = "fh8626v100-spi",
		.of_match_table = fh_spi_match,
		.pm = &fh_spi_pm_ops,
	},
};
module_platform_driver(fh_spi_driver);
MODULE_DESCRIPTION("FH8626V100 standard-wire SSI master");
MODULE_LICENSE("GPL v2");
