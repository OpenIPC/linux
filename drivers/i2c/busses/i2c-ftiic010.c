// SPDX-License-Identifier: GPL-2.0-only
/*
 * Faraday FTIIC010 I2C controller, master.
 *
 * Registers from the Grain Media GM8126 driver (i2c-ftiic010.c, 2.6.28).
 * A byte moves for each write of the control register with TB_EN; the
 * status register's events clear when read.
 */

#include <linux/bits.h>
#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#define IIC_CR			0x00
#define   CR_RST		BIT(0)
#define   CR_EN			BIT(1)
#define   CR_SCL_EN		BIT(2)
#define   CR_START		BIT(4)
#define   CR_STOP		BIT(5)
#define   CR_NAK		BIT(6)
#define   CR_TB_EN		BIT(7)
#define   CR_DTI_EN		BIT(8)
#define   CR_DRI_EN		BIT(9)
#define   CR_BERRI_EN		BIT(10)
#define   CR_ALI_EN		BIT(13)
#define IIC_SR			0x04
#define   SR_DT			BIT(4)
#define   SR_DR			BIT(5)
#define   SR_BERR		BIT(6)
#define   SR_AL			BIT(10)
#define   SR_EVENTS		(SR_DT | SR_DR | SR_BERR | SR_AL)
#define IIC_CDR			0x08
#define   CDR_MAX		0x3ffff
#define IIC_DR			0x0c
#define IIC_TGSR		0x14
#define   TGSR(tsr, gsr)	((tsr) | (gsr) << 10)

/* the vendor's hold time and glitch filter */
#define IIC_TSR			27
#define IIC_GSR			2

struct ftiic010 {
	void __iomem *base;
	struct i2c_adapter adapter;
	struct completion done;
	u32 status;
	u32 cdr;
};

static int ftiic010_init(struct ftiic010 *iic)
{
	u32 cr;

	writel(CR_RST, iic->base + IIC_CR);
	if (readl_poll_timeout_atomic(iic->base + IIC_CR, cr, !(cr & CR_RST),
				      1, 1000))
		return -ETIMEDOUT;
	writel(TGSR(IIC_TSR, IIC_GSR), iic->base + IIC_TGSR);
	writel(iic->cdr, iic->base + IIC_CDR);
	return 0;
}

static irqreturn_t ftiic010_interrupt(int irq, void *dev_id)
{
	struct ftiic010 *iic = dev_id;
	u32 sr = readl(iic->base + IIC_SR) & SR_EVENTS;

	if (!sr)
		return IRQ_NONE;
	iic->status |= sr;
	complete(&iic->done);
	return IRQ_HANDLED;
}

/* Move one byte; flags: CR_START, CR_STOP, CR_NAK. */
static int ftiic010_byte(struct ftiic010 *iic, u32 flags, u32 event)
{
	iic->status = 0;
	reinit_completion(&iic->done);
	writel(CR_EN | CR_SCL_EN | CR_TB_EN | CR_DTI_EN | CR_DRI_EN |
	       CR_BERRI_EN | CR_ALI_EN | flags, iic->base + IIC_CR);
	if (!wait_for_completion_timeout(&iic->done, iic->adapter.timeout))
		return -ETIMEDOUT;
	if (iic->status & SR_AL)
		return -EAGAIN;
	if (iic->status & SR_BERR)
		return -ENXIO;
	return iic->status & event ? 0 : -EIO;
}

static int ftiic010_msg(struct ftiic010 *iic, struct i2c_msg *msg, bool last)
{
	int i, ret;

	writel(i2c_8bit_addr_from_msg(msg), iic->base + IIC_DR);
	ret = ftiic010_byte(iic, CR_START | (last && !msg->len ? CR_STOP : 0),
			    SR_DT);
	for (i = 0; !ret && i < msg->len; i++) {
		bool end = i + 1 == msg->len;
		u32 flags = end && last ? CR_STOP : 0;

		if (msg->flags & I2C_M_RD) {
			ret = ftiic010_byte(iic, flags | (end ? CR_NAK : 0),
					    SR_DR);
			if (!ret)
				msg->buf[i] = readl(iic->base + IIC_DR);
		} else {
			writel(msg->buf[i], iic->base + IIC_DR);
			ret = ftiic010_byte(iic, flags, SR_DT);
		}
	}
	return ret;
}

static int ftiic010_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs,
			 int num)
{
	struct ftiic010 *iic = i2c_get_adapdata(adap);
	int i, ret;

	for (i = 0; i < num; i++) {
		ret = ftiic010_msg(iic, &msgs[i], i == num - 1);
		if (ret) {
			/* also what ends a transfer that was not acknowledged */
			ftiic010_init(iic);
			return ret;
		}
	}
	return num;
}

static u32 ftiic010_functionality(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | (I2C_FUNC_SMBUS_EMUL & ~I2C_FUNC_SMBUS_QUICK);
}

static const struct i2c_algorithm ftiic010_algo = {
	.xfer = ftiic010_xfer,
	.functionality = ftiic010_functionality,
};

/* a read of no bytes cannot be ended */
static const struct i2c_adapter_quirks ftiic010_quirks = {
	.flags = I2C_AQ_NO_ZERO_LEN_READ,
};

static int ftiic010_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ftiic010 *iic;
	struct clk *clk;
	u32 hz = I2C_MAX_STANDARD_MODE_FREQ;
	int irq, ret;

	iic = devm_kzalloc(dev, sizeof(*iic), GFP_KERNEL);
	if (!iic)
		return -ENOMEM;
	iic->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(iic->base))
		return PTR_ERR(iic->base);
	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;
	clk = devm_clk_get_enabled(dev, NULL);
	if (IS_ERR(clk))
		return dev_err_probe(dev, PTR_ERR(clk), "no clock\n");

	of_property_read_u32(dev->of_node, "clock-frequency", &hz);
	if (!hz || hz > I2C_MAX_FAST_MODE_FREQ)
		return dev_err_probe(dev, -EINVAL, "bad clock-frequency\n");
	/* SCL = clk / (2 * (CDR + 2) + GSR) */
	iic->cdr = min_t(u32, (DIV_ROUND_UP(clk_get_rate(clk), hz) - IIC_GSR) /
			      2 - 2, CDR_MAX);
	init_completion(&iic->done);
	ret = ftiic010_init(iic);
	if (ret)
		return dev_err_probe(dev, ret, "reset failed\n");
	ret = devm_request_irq(dev, irq, ftiic010_interrupt, 0, dev_name(dev),
			       iic);
	if (ret)
		return ret;

	iic->adapter.owner = THIS_MODULE;
	iic->adapter.algo = &ftiic010_algo;
	iic->adapter.quirks = &ftiic010_quirks;
	iic->adapter.timeout = HZ / 10;
	iic->adapter.dev.parent = dev;
	iic->adapter.dev.of_node = dev->of_node;
	strscpy(iic->adapter.name, "ftiic010", sizeof(iic->adapter.name));
	i2c_set_adapdata(&iic->adapter, iic);

	return devm_i2c_add_adapter(dev, &iic->adapter);
}

static const struct of_device_id ftiic010_of_match[] = {
	{ .compatible = "faraday,ftiic010" },
	{ }
};
MODULE_DEVICE_TABLE(of, ftiic010_of_match);

static struct platform_driver ftiic010_driver = {
	.probe = ftiic010_probe,
	.driver = {
		.name = "i2c-ftiic010",
		.of_match_table = ftiic010_of_match,
	},
};
module_platform_driver(ftiic010_driver);

MODULE_DESCRIPTION("Faraday FTIIC010 I2C driver");
MODULE_LICENSE("GPL");
