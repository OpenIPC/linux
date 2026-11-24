// SPDX-License-Identifier: GPL-2.0-only
/*
 * Faraday FTSSP010 as an SPI host, polled.
 *
 * Frames are eight bits, so data keeps the order it has on the wire. The
 * GM8126's mask ROM and vendor software use 32-bit frames: what they wrote
 * to the flash reads back here with each word's bytes swapped.
 */

#include <linux/bits.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>

#define SSP_CR0			0x00
#define   CR0_FFMT_SPI		BIT(12)
#define   CR0_HOST		(BIT(5) | BIT(3) | BIT(2))
#define   CR0_SCLKPO		BIT(1)
#define   CR0_SCLKPH		BIT(0)
#define SSP_CR1			0x04
#define   CR1_SDL(bits)		(((bits) - 1) << 16)
#define   CR1_DIV_MAX		0xffff
#define SSP_CR2			0x08
#define   CR2_TXFCLR		BIT(3)
#define   CR2_RXFCLR		BIT(2)
#define   CR2_TXDOE		BIT(1)
#define   CR2_ENABLE		BIT(0)
#define SSP_STATUS		0x0c
#define   STATUS_RFVE(s)	(((s) >> 4) & 0x1f)
#define   STATUS_TFNF		BIT(1)
#define SSP_ICR			0x10
#define SSP_DATA		0x18

/* frames in flight: not more than the receive FIFO holds */
#define SSP_FIFO		16
#define SSP_TIMEOUT_US		100000

struct ftssp010 {
	void __iomem *base;
	unsigned long rate;
};

static int ftssp010_transfer_one(struct spi_controller *ctlr,
				 struct spi_device *spi,
				 struct spi_transfer *t)
{
	struct ftssp010 *ssp = spi_controller_get_devdata(ctlr);
	const u8 *out = t->tx_buf;
	u8 *in = t->rx_buf;
	unsigned int tx = 0, rx = 0, div;
	u32 cr0 = CR0_FFMT_SPI | CR0_HOST, st;

	if (spi->mode & SPI_CPHA)
		cr0 |= CR0_SCLKPH;
	if (spi->mode & SPI_CPOL)
		cr0 |= CR0_SCLKPO;
	/* SCLK = clk / 2 / (div + 1) */
	div = DIV_ROUND_UP(ssp->rate, 2 * t->speed_hz) - 1;
	div = min_t(unsigned int, div, CR1_DIV_MAX);
	t->effective_speed_hz = ssp->rate / 2 / (div + 1);

	writel(cr0, ssp->base + SSP_CR0);
	writel(CR1_SDL(8) | div, ssp->base + SSP_CR1);
	writel(CR2_ENABLE | CR2_TXDOE | CR2_RXFCLR | CR2_TXFCLR,
	       ssp->base + SSP_CR2);

	while (rx < t->len) {
		unsigned int n;

		if (readl_poll_timeout_atomic(ssp->base + SSP_STATUS, st,
				STATUS_RFVE(st) ||
				(tx < t->len && tx - rx < SSP_FIFO &&
				 (st & STATUS_TFNF)),
				0, SSP_TIMEOUT_US)) {
			dev_err(&ctlr->dev, "transfer timed out\n");
			return -ETIMEDOUT;
		}
		if (tx < t->len && tx - rx < SSP_FIFO && (st & STATUS_TFNF)) {
			writel(out ? out[tx] : 0, ssp->base + SSP_DATA);
			tx++;
		}
		for (n = STATUS_RFVE(st); n && rx < t->len; n--) {
			u8 v = readl(ssp->base + SSP_DATA);

			if (in)
				in[rx] = v;
			rx++;
		}
	}
	return 0;
}

static int ftssp010_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct spi_controller *ctlr;
	struct ftssp010 *ssp;
	struct clk *clk;

	ctlr = devm_spi_alloc_host(dev, sizeof(*ssp));
	if (!ctlr)
		return -ENOMEM;
	ssp = spi_controller_get_devdata(ctlr);
	ssp->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(ssp->base))
		return PTR_ERR(ssp->base);
	clk = devm_clk_get_enabled(dev, NULL);
	if (IS_ERR(clk))
		return dev_err_probe(dev, PTR_ERR(clk), "no clock\n");
	ssp->rate = clk_get_rate(clk);
	if (!ssp->rate)
		return dev_err_probe(dev, -EINVAL, "clock is stopped\n");

	ctlr->dev.of_node = dev->of_node;
	ctlr->mode_bits = SPI_CPHA | SPI_CPOL;
	ctlr->bits_per_word_mask = SPI_BPW_MASK(8);
	ctlr->max_speed_hz = ssp->rate / 2;
	ctlr->min_speed_hz = DIV_ROUND_UP(ssp->rate, 2 * (CR1_DIV_MAX + 1));
	ctlr->use_gpio_descriptors = true;
	ctlr->transfer_one = ftssp010_transfer_one;
	writel(0, ssp->base + SSP_ICR);

	return devm_spi_register_controller(dev, ctlr);
}

static const struct of_device_id ftssp010_of_match[] = {
	{ .compatible = "faraday,ftssp010" },
	{ }
};
MODULE_DEVICE_TABLE(of, ftssp010_of_match);

static struct platform_driver ftssp010_driver = {
	.probe = ftssp010_probe,
	.driver = {
		.name = "spi-ftssp010",
		.of_match_table = ftssp010_of_match,
	},
};
module_platform_driver(ftssp010_driver);

MODULE_DESCRIPTION("Faraday FTSSP010 SPI host driver");
MODULE_LICENSE("GPL");
