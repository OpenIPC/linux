// SPDX-License-Identifier: GPL-2.0
/*
 * Ingenic T-Series SFC v1 SPI Flash Controller (spi-mem) driver
 *
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi-mem.h>

#define SFC_GLB				0x0000
#define SFC_DEV_CONF			0x0004
#define SFC_DEV_STA_EXP			0x0008
#define SFC_DEV_STA_RT			0x000c
#define SFC_DEV_STA_MSK			0x0010
#define SFC_TRAN_CONF(n)		(0x0014 + ((n) * 4))
#define SFC_TRAN_LEN			0x002c
#define SFC_DEV_ADDR(n)			(0x0030 + ((n) * 4))
#define SFC_DEV_ADDR_PLUS(n)		(0x0048 + ((n) * 4))
#define SFC_MEM_ADDR			0x0060
#define SFC_TRIG			0x0064
#define SFC_SR				0x0068
#define SFC_SCR				0x006c
#define SFC_INTC			0x0070
#define SFC_CGE				0x0078
#define SFC_RM_DR			0x1000

#define GLB_TRAN_DIR			BIT(13)
#define GLB_THRESHOLD_MASK		GENMASK(12, 7)
#define GLB_OP_DMA			BIT(6)
#define GLB_PHASE_NUM_MASK		GENMASK(5, 3)
#define GLB_WP_EN			BIT(2)
#define GLB_BURST_MD_MASK		GENMASK(1, 0)

#define DEV_CONF_SMP_DELAY_MASK		GENMASK(17, 16)
#define DEV_CONF_HALF_CYCLE_DELAY	1
#define DEV_CONF_THOLD_MASK		GENMASK(12, 11)
#define DEV_CONF_TSETUP_MASK		GENMASK(10, 9)
#define DEV_CONF_TSH_MASK		GENMASK(8, 5)
#define DEV_CONF_CPHA			BIT(4)
#define DEV_CONF_CPOL			BIT(3)
#define DEV_CONF_CEDL			BIT(2)
#define DEV_CONF_HOLDDL			BIT(1)
#define DEV_CONF_WPDL			BIT(0)

#define TRAN_CONF_MODE_MASK		GENMASK(31, 29)
#define TRAN_SPI_STANDARD		0x0
#define TRAN_SPI_DUAL			0x1
#define TRAN_SPI_IO_DUAL		0x2
#define TRAN_SPI_QUAD			0x5
#define TRAN_SPI_IO_QUAD		0x6
#define TRAN_CONF_ADDR_WIDTH_MASK	GENMASK(28, 26)
#define TRAN_CONF_POLL_EN		BIT(25)
#define TRAN_CONF_CMD_EN		BIT(24)
#define TRAN_CONF_DMYBITS_MASK		GENMASK(22, 17)
#define TRAN_CONF_DATA_EN		BIT(16)
#define TRAN_CONF_CMD_MASK		GENMASK(15, 0)

#define TRIG_FLUSH			BIT(2)
#define TRIG_STOP			BIT(1)
#define TRIG_START			BIT(0)

#define SR_FIFO_NUM_MASK		GENMASK(22, 16)
#define SR_END				BIT(4)
#define SR_TREQ				BIT(3)
#define SR_RREQ				BIT(2)
#define SR_OVER				BIT(1)
#define SR_UNDER			BIT(0)
#define SR_IRQ_MASK			GENMASK(4, 0)

#define SFC_FIFO_THRESHOLD		31
#define SFC_PIO_MAX_BYTES		64
#define SFC_BOUNCE_SIZE			PAGE_SIZE
#define SFC_DMA_ADDR_MASK		0x1fffffffU

struct ingenic_sfc {
	void __iomem *base;
	struct device *dev;
	struct clk *clk;
	struct clk *clk_gate;
	int irq;
	struct completion done;
	struct mutex lock;
	void *bounce;
	dma_addr_t dma_phys;
	u32 cur_speed_hz;
	u32 status_err;
};

static void ingenic_sfc_quiesce(struct ingenic_sfc *sfc)
{
	writel(SR_IRQ_MASK, sfc->base + SFC_INTC);
	writel(TRIG_STOP | TRIG_FLUSH, sfc->base + SFC_TRIG);
	writel(SR_IRQ_MASK, sfc->base + SFC_SCR);
}

static irqreturn_t ingenic_sfc_irq(int irq, void *data)
{
	struct ingenic_sfc *sfc = data;
	u32 sr, mask;

	sr = readl(sfc->base + SFC_SR);
	mask = ~readl(sfc->base + SFC_INTC) & SR_IRQ_MASK;
	if (!(sr & mask))
		return IRQ_NONE;

	if (sr & (SR_OVER | SR_UNDER))
		sfc->status_err = sr & (SR_OVER | SR_UNDER);

	writel(SR_IRQ_MASK, sfc->base + SFC_INTC);
	writel(sr & SR_IRQ_MASK, sfc->base + SFC_SCR);
	complete(&sfc->done);

	return IRQ_HANDLED;
}

static int ingenic_sfc_op_mode(const struct spi_mem_op *op)
{
	if (op->cmd.buswidth != 1)
		return -EOPNOTSUPP;

	if (op->addr.nbytes && op->addr.buswidth != 1 &&
	    op->addr.buswidth != 2 && op->addr.buswidth != 4)
		return -EOPNOTSUPP;

	if (!op->data.nbytes)
		return (op->addr.buswidth <= 1) ? TRAN_SPI_STANDARD : -EOPNOTSUPP;

	switch (op->data.buswidth) {
	case 1:
		return (op->addr.buswidth <= 1) ? TRAN_SPI_STANDARD : -EOPNOTSUPP;
	case 2:
		if (op->addr.buswidth <= 1)
			return TRAN_SPI_DUAL;
		if (op->addr.buswidth == 2)
			return TRAN_SPI_IO_DUAL;
		return -EOPNOTSUPP;
	case 4:
		if (op->addr.buswidth <= 1)
			return TRAN_SPI_QUAD;
		if (op->addr.buswidth == 4)
			return TRAN_SPI_IO_QUAD;
		return -EOPNOTSUPP;
	default:
		return -EOPNOTSUPP;
	}
}

static bool ingenic_sfc_supports_op(struct spi_mem *mem,
				    const struct spi_mem_op *op)
{
	unsigned int dummy_cycles = 0;

	if (!spi_mem_default_supports_op(mem, op))
		return false;

	if (op->cmd.nbytes != 1 || op->addr.nbytes > 6)
		return false;

	if (op->dummy.nbytes) {
		if (!op->dummy.buswidth)
			return false;
		dummy_cycles = (op->dummy.nbytes * 8) / op->dummy.buswidth;
		if (dummy_cycles > 63)
			return false;
	}

	return ingenic_sfc_op_mode(op) >= 0;
}

static int ingenic_sfc_adjust_op_size(struct spi_mem *mem,
				      struct spi_mem_op *op)
{
	if (op->data.nbytes > SFC_BOUNCE_SIZE)
		op->data.nbytes = SFC_BOUNCE_SIZE;

	return 0;
}

static void ingenic_sfc_config_bus(struct ingenic_sfc *sfc,
				   struct spi_device *spi)
{
	u32 speed = spi->max_speed_hz ? spi->max_speed_hz : 50000000U;
	u32 dev_conf;
	unsigned long actual;

	if (speed != sfc->cur_speed_hz) {
		clk_set_rate(sfc->clk, speed);
		sfc->cur_speed_hz = speed;
	}

	actual = clk_get_rate(sfc->clk);
	dev_conf = DEV_CONF_CEDL | DEV_CONF_HOLDDL | DEV_CONF_WPDL;
	if (spi->mode & SPI_CPHA)
		dev_conf |= DEV_CONF_CPHA;
	if (spi->mode & SPI_CPOL)
		dev_conf |= DEV_CONF_CPOL;
	if (actual >= 100000000UL)
		dev_conf |= FIELD_PREP(DEV_CONF_SMP_DELAY_MASK,
				       DEV_CONF_HALF_CYCLE_DELAY);

	writel(dev_conf, sfc->base + SFC_DEV_CONF);
}

static int ingenic_sfc_pio_read(struct ingenic_sfc *sfc, u8 *buf, size_t len)
{
	size_t off = 0;
	u32 sr, words, word;
	int ret;

	while (off < len) {
		ret = readl_poll_timeout_atomic(sfc->base + SFC_SR, sr,
						FIELD_GET(SR_FIFO_NUM_MASK, sr) ||
						(sr & (SR_RREQ | SR_END | SR_OVER)),
						1, 200000);
		if (ret)
			return ret;
		if (sr & SR_OVER)
			return -EIO;

		words = FIELD_GET(SR_FIFO_NUM_MASK, sr);
		if (!words && (sr & (SR_RREQ | SR_END)))
			words = 1;

		while (words-- && off < len) {
			size_t chunk = min_t(size_t, 4, len - off);

			word = readl(sfc->base + SFC_RM_DR);
			memcpy(buf + off, &word, chunk);
			off += chunk;
		}
		writel(SR_RREQ, sfc->base + SFC_SCR);
	}

	return readl_poll_timeout_atomic(sfc->base + SFC_SR, sr,
					 sr & SR_END, 1, 200000);
}

static int ingenic_sfc_pio_write(struct ingenic_sfc *sfc, const u8 *buf,
				 size_t len)
{
	u32 sr;
	int ret;

	while (len) {
		unsigned int words = SFC_FIFO_THRESHOLD;

		ret = readl_poll_timeout_atomic(sfc->base + SFC_SR, sr,
						sr & (SR_TREQ | SR_UNDER),
						1, 200000);
		if (ret)
			return ret;
		if (sr & SR_UNDER)
			return -EIO;

		/* One request admits a threshold-sized burst of FIFO words. */
		writel(SR_TREQ, sfc->base + SFC_SCR);
		while (words-- && len) {
			u32 word = 0;
			size_t chunk = min(len, sizeof(word));

			memcpy(&word, buf, chunk);
			writel(word, sfc->base + SFC_RM_DR);
			buf += chunk;
			len -= chunk;
		}
	}

	ret = readl_poll_timeout_atomic(sfc->base + SFC_SR, sr,
					sr & (SR_END | SR_UNDER), 1, 200000);
	return ret ?: (sr & SR_UNDER ? -EIO : 0);
}

static int ingenic_sfc_exec_op(struct spi_mem *mem,
			       const struct spi_mem_op *op)
{
	struct ingenic_sfc *sfc = spi_controller_get_devdata(mem->spi->controller);
	int mode = ingenic_sfc_op_mode(op);
	unsigned int dummy_cycles = 0;
	size_t nbytes = op->data.nbytes;
	bool is_write = (op->data.dir == SPI_MEM_DATA_OUT);
	bool use_dma;
	u32 glb, tran_conf, sr;
	int ret = 0;

	if (mode < 0)
		return mode;
	if (nbytes > SFC_BOUNCE_SIZE)
		return -EINVAL;

	if (op->dummy.nbytes)
		dummy_cycles = (op->dummy.nbytes * 8) / op->dummy.buswidth;

	use_dma = nbytes > SFC_PIO_MAX_BYTES;

	mutex_lock(&sfc->lock);

	ingenic_sfc_quiesce(sfc);
	ingenic_sfc_config_bus(sfc, mem->spi);

	glb = FIELD_PREP(GLB_THRESHOLD_MASK, SFC_FIFO_THRESHOLD) |
	      FIELD_PREP(GLB_PHASE_NUM_MASK, 1) |
	      GLB_WP_EN |
	      FIELD_PREP(GLB_BURST_MD_MASK, 3);
	if (is_write)
		glb |= GLB_TRAN_DIR;
	if (use_dma)
		glb |= GLB_OP_DMA;

	tran_conf = FIELD_PREP(TRAN_CONF_MODE_MASK, (u32)mode) |
		    FIELD_PREP(TRAN_CONF_ADDR_WIDTH_MASK, op->addr.nbytes) |
		    TRAN_CONF_CMD_EN |
		    FIELD_PREP(TRAN_CONF_DMYBITS_MASK, dummy_cycles) |
		    FIELD_PREP(TRAN_CONF_CMD_MASK, op->cmd.opcode);
	if (nbytes)
		tran_conf |= TRAN_CONF_DATA_EN;

	writel(glb, sfc->base + SFC_GLB);
	writel(tran_conf, sfc->base + SFC_TRAN_CONF(0));
	writel(nbytes, sfc->base + SFC_TRAN_LEN);
	writel((u32)op->addr.val, sfc->base + SFC_DEV_ADDR(0));
	writel((u32)(op->addr.val >> 32), sfc->base + SFC_DEV_ADDR_PLUS(0));

	if (use_dma) {
		if (is_write) {
			memcpy(sfc->bounce, op->data.buf.out, nbytes);
			wmb(); /* flush bounce buffer writes before SFC DMA */
		}
		sfc->status_err = 0;
		reinit_completion(&sfc->done);
		writel((u32)sfc->dma_phys & SFC_DMA_ADDR_MASK,
		       sfc->base + SFC_MEM_ADDR);
		writel(~(SR_END | SR_OVER | SR_UNDER) & SR_IRQ_MASK,
		       sfc->base + SFC_INTC);
		writel(TRIG_START, sfc->base + SFC_TRIG);
		if (!wait_for_completion_timeout(&sfc->done,
						 msecs_to_jiffies(1000)))
			ret = -ETIMEDOUT;
		else if (sfc->status_err)
			ret = -EIO;

		if (!ret && !is_write) {
			rmb(); /* order DMA completion before bounce copy */
			memcpy(op->data.buf.in, sfc->bounce, nbytes);
		}
	} else {
		writel(TRIG_START, sfc->base + SFC_TRIG);
		if (!nbytes)
			ret = readl_poll_timeout_atomic(sfc->base + SFC_SR, sr,
							sr & SR_END, 1, 200000);
		else if (is_write)
			ret = ingenic_sfc_pio_write(sfc, op->data.buf.out, nbytes);
		else
			ret = ingenic_sfc_pio_read(sfc, op->data.buf.in, nbytes);
	}

	ingenic_sfc_quiesce(sfc);
	mutex_unlock(&sfc->lock);

	return ret;
}

static const struct spi_controller_mem_ops ingenic_sfc_mem_ops = {
	.supports_op = ingenic_sfc_supports_op,
	.adjust_op_size = ingenic_sfc_adjust_op_size,
	.exec_op = ingenic_sfc_exec_op,
};

static int ingenic_sfc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct spi_controller *ctlr;
	struct ingenic_sfc *sfc;
	int ret;

	ctlr = devm_spi_alloc_host(dev, sizeof(*sfc));
	if (!ctlr)
		return -ENOMEM;

	sfc = spi_controller_get_devdata(ctlr);
	sfc->dev = dev;
	mutex_init(&sfc->lock);
	init_completion(&sfc->done);

	sfc->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(sfc->base))
		return PTR_ERR(sfc->base);

	sfc->irq = platform_get_irq(pdev, 0);
	if (sfc->irq < 0)
		return sfc->irq;

	sfc->clk = devm_clk_get_enabled(dev, "sfc");
	if (IS_ERR(sfc->clk))
		return dev_err_probe(dev, PTR_ERR(sfc->clk),
				     "Failed to enable sfc clock\n");

	sfc->clk_gate = devm_clk_get_optional_enabled(dev, "gate");
	if (IS_ERR(sfc->clk_gate))
		return dev_err_probe(dev, PTR_ERR(sfc->clk_gate),
				     "Failed to enable sfc gate clock\n");

	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(29));
	if (ret)
		return ret;

	sfc->bounce = dmam_alloc_coherent(dev, SFC_BOUNCE_SIZE, &sfc->dma_phys,
					  GFP_KERNEL);
	if (!sfc->bounce)
		return -ENOMEM;

	if (sfc->dma_phys + SFC_BOUNCE_SIZE > (SFC_DMA_ADDR_MASK + 1ULL))
		return dev_err_probe(dev, -ERANGE,
				     "DMA bounce buffer outside 29-bit window\n");

	ingenic_sfc_quiesce(sfc);

	ret = devm_request_irq(dev, sfc->irq, ingenic_sfc_irq, 0,
			       dev_name(dev), sfc);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to request IRQ\n");

	ctlr->bus_num = -1;
	ctlr->num_chipselect = 1;
	ctlr->mode_bits = SPI_CPHA | SPI_CPOL |
			  SPI_RX_DUAL | SPI_RX_QUAD |
			  SPI_TX_DUAL | SPI_TX_QUAD;
	ctlr->mem_ops = &ingenic_sfc_mem_ops;
	ctlr->dev.of_node = dev->of_node;

	platform_set_drvdata(pdev, ctlr);

	return devm_spi_register_controller(dev, ctlr);
}

static const struct of_device_id ingenic_sfc_of_match[] = {
	{ .compatible = "ingenic,t23-sfc" },
	{ }
};
MODULE_DEVICE_TABLE(of, ingenic_sfc_of_match);

static struct platform_driver ingenic_sfc_driver = {
	.probe = ingenic_sfc_probe,
	.driver = {
		.name = "spi-ingenic-sfc",
		.of_match_table = ingenic_sfc_of_match,
	},
};
module_platform_driver(ingenic_sfc_driver);

MODULE_DESCRIPTION("Ingenic T-Series SFC v1 spi-mem controller driver");
MODULE_AUTHOR("Difan Zhang");
MODULE_LICENSE("GPL");
