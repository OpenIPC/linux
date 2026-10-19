/*
 * I2C master for the later Anyka controller (AK39EV330/EV331, and the
 * AK37D lineage the vendor calls "ak3790d").
 *
 * No source for this block exists in any SDK we have. Its programming
 * model is the one the vendor's prebuilt 4.4 ak_i2c.ko (AnyCloud V500,
 * version 1.0.01) uses, read out of that module's code, and the one
 * QEMU's ak-i2c (layout 2) implements from the same module:
 *
 *   0x00 CTRL  31:28 ?, 27 no acknowledge (read back), 26 start a read,
 *              24:22 set for reads, 23 start a write, 21:17 byte count
 *              (0 = 16), 16:15 SDA hold delay, 14 and 24 set at init,
 *              13 transfer done (cleared by the start write, set by the
 *              controller), 12:0 SCL divider
 *   0x08 ADDR  slave address byte (addr << 1 | R/W) | 0x300
 *   0x10..0x1C DATA0..3: up to 16 data bytes, little endian
 *
 * One transfer is START, the address byte, up to 16 data bytes, STOP;
 * there is no repeated start, so each message is a transaction of its
 * own, as in the vendor driver. The vendor driver polls with interrupts
 * off and no timeout; this one polls with a timeout and leaves them on.
 *
 * Licensed under the GPL-2.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#define AK_I2C2_CTRL		0x00
#define AK_I2C2_ADDR		0x08
#define AK_I2C2_DATA0		0x10

#define CTRL_NAK		BIT(27)
#define CTRL_READ_GO		BIT(26)
#define CTRL_READ_MODE		(BIT(24) | BIT(23) | BIT(22))
#define CTRL_WRITE_GO		BIT(23)
#define CTRL_COUNT_SHIFT	17
#define CTRL_COUNT_MASK		(0x1f << CTRL_COUNT_SHIFT)
#define CTRL_WRITE_CLEAR	(0x3f << CTRL_COUNT_SHIFT)	/* 22:17 */
#define CTRL_SDA_SHIFT		15
#define CTRL_SDA_MASK		(0x3 << CTRL_SDA_SHIFT)
#define CTRL_SDA_EN		BIT(16)
#define CTRL_DONE		BIT(13)
#define CTRL_DIV_MASK		0x1fff
#define CTRL_INIT		(BIT(24) | BIT(14))

#define ADDR_FLAGS		0x300

#define AK_I2C2_FIFO		16
#define AK_I2C2_TIMEOUT_US	100000

struct ak_i2c2 {
	struct i2c_adapter adap;
	struct device *dev;
	void __iomem *regs;
	struct clk *clk;
};

static int ak_i2c2_wait_done(struct ak_i2c2 *i2c)
{
	u32 ctrl;
	int ret;

	ret = readl_poll_timeout(i2c->regs + AK_I2C2_CTRL, ctrl,
				 ctrl & CTRL_DONE, 5, AK_I2C2_TIMEOUT_US);
	if (ret)
		dev_err(i2c->dev, "transfer timed out, CTRL %08x\n", ctrl);
	return ret;
}

static int ak_i2c2_write_chunk(struct ak_i2c2 *i2c, const u8 *buf,
			       unsigned int n)
{
	unsigned int i;
	u32 ctrl;

	for (i = 0; i < n; i += 4) {
		u32 word = 0;
		unsigned int j;

		for (j = 0; j < 4 && i + j < n; j++)
			word |= (u32)buf[i + j] << (8 * j);
		writel(word, i2c->regs + AK_I2C2_DATA0 + i);
	}

	ctrl = readl(i2c->regs + AK_I2C2_CTRL);
	ctrl &= ~(CTRL_WRITE_CLEAR | CTRL_DONE | CTRL_READ_GO);
	ctrl |= ((n & 0x1f) << CTRL_COUNT_SHIFT) | CTRL_WRITE_GO;
	writel(ctrl, i2c->regs + AK_I2C2_CTRL);

	/*
	 * The vendor driver looks at the acknowledge only after a read; a
	 * write to an absent device completes the same way, so this does too.
	 */
	return ak_i2c2_wait_done(i2c);
}

static int ak_i2c2_read_chunk(struct ak_i2c2 *i2c, u8 *buf, unsigned int n)
{
	unsigned int i;
	u32 ctrl;
	int ret;

	ctrl = readl(i2c->regs + AK_I2C2_CTRL);
	ctrl &= ~(CTRL_COUNT_MASK | CTRL_DONE);
	/* The vendor driver sets 24:22 and the SDA field to 3 for every read. */
	ctrl |= CTRL_READ_MODE | CTRL_SDA_MASK |
		((n & 0x1f) << CTRL_COUNT_SHIFT) | CTRL_READ_GO;
	writel(ctrl, i2c->regs + AK_I2C2_CTRL);

	ret = ak_i2c2_wait_done(i2c);
	if (ret)
		return ret;

	if (readl(i2c->regs + AK_I2C2_CTRL) & CTRL_NAK)
		return -ENXIO;

	for (i = 0; i < n; i += 4) {
		u32 word = readl(i2c->regs + AK_I2C2_DATA0 + i);
		unsigned int j;

		for (j = 0; j < 4 && i + j < n; j++)
			buf[i + j] = word >> (8 * j);
	}
	return 0;
}

static int ak_i2c2_xfer_msg(struct ak_i2c2 *i2c, struct i2c_msg *msg)
{
	bool read = msg->flags & I2C_M_RD;
	unsigned int done = 0;
	u8 ab;
	int ret;

	if (msg->flags & I2C_M_TEN)
		return -EOPNOTSUPP;
	/*
	 * The controller always moves at least one data byte, and the vendor
	 * driver silently skipped empty messages; refuse them instead, so
	 * that SMBus quick (and i2cdetect's default probe) is not answered
	 * with a success that never touched the bus.
	 */
	if (!msg->len)
		return -EOPNOTSUPP;

	ab = (msg->addr << 1) | (read ? 1 : 0);
	if (msg->flags & I2C_M_REV_DIR_ADDR)
		ab ^= 1;
	writel(ab | ADDR_FLAGS, i2c->regs + AK_I2C2_ADDR);

	/*
	 * More than 16 bytes go as consecutive transactions of up to 16, each
	 * with its own address phase. That is what the vendor driver does for
	 * writes; for reads it programs one 16-byte count and then re-reads
	 * the same data registers, so longer reads never worked there.
	 */
	while (done < msg->len) {
		unsigned int n = min_t(unsigned int, msg->len - done,
				       AK_I2C2_FIFO);

		if (read)
			ret = ak_i2c2_read_chunk(i2c, msg->buf + done, n);
		else
			ret = ak_i2c2_write_chunk(i2c, msg->buf + done, n);
		if (ret)
			return ret;
		done += n;
	}
	return 0;
}

static int ak_i2c2_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs,
			int num)
{
	struct ak_i2c2 *i2c = i2c_get_adapdata(adap);
	int i, ret;

	for (i = 0; i < num; i++) {
		ret = ak_i2c2_xfer_msg(i2c, &msgs[i]);
		if (ret)
			return ret;
	}
	return num;
}

static u32 ak_i2c2_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C |
	       (I2C_FUNC_SMBUS_EMUL & ~I2C_FUNC_SMBUS_QUICK);
}

static const struct i2c_algorithm ak_i2c2_algo = {
	.master_xfer	= ak_i2c2_xfer,
	.functionality	= ak_i2c2_func,
};

static void ak_i2c2_hw_init(struct ak_i2c2 *i2c, u32 bus_hz, u32 sda_delay)
{
	unsigned long clk_khz = clk_get_rate(i2c->clk) / 1000;
	unsigned long bus_khz = max_t(u32, bus_hz / 1000, 1);
	unsigned long div = clk_khz / bus_khz / 2;
	u32 ctrl = CTRL_INIT;

	ctrl |= (div ? div - 1 : 0) & CTRL_DIV_MASK;

	/*
	 * The SDA hold delay, in units of five clocks of a 1 MHz count, as
	 * the vendor driver computes it, capped at 3. Its encoding of the
	 * smaller values ORs them into the divider field instead; this puts
	 * them in 16:15 with bit 16 set, which equals the vendor's value
	 * whenever the delay reaches the cap.
	 */
	if (sda_delay) {
		unsigned long d = (clk_khz * sda_delay / 1000000 + 4) / 5;

		ctrl |= CTRL_SDA_EN | (min_t(unsigned long, d, 3) <<
				       CTRL_SDA_SHIFT);
	}
	writel(ctrl, i2c->regs + AK_I2C2_CTRL);
	dev_info(i2c->dev, "%lu kHz bus from a %lu kHz clock\n", bus_khz,
		 clk_khz);
}

static int ak_i2c2_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct ak_i2c2 *i2c;
	struct resource *res;
	u32 bus_hz = 100000, sda_delay = 100;
	int ret;

	i2c = devm_kzalloc(&pdev->dev, sizeof(*i2c), GFP_KERNEL);
	if (!i2c)
		return -ENOMEM;
	i2c->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	i2c->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(i2c->regs))
		return PTR_ERR(i2c->regs);

	i2c->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(i2c->clk)) {
		dev_err(&pdev->dev, "no clock\n");
		return PTR_ERR(i2c->clk);
	}
	ret = clk_prepare_enable(i2c->clk);
	if (ret)
		return ret;

	of_property_read_u32(np, "clock-frequency", &bus_hz);
	of_property_read_u32(np, "sda-delay", &sda_delay);
	ak_i2c2_hw_init(i2c, bus_hz, sda_delay);

	i2c->adap.owner = THIS_MODULE;
	i2c->adap.algo = &ak_i2c2_algo;
	i2c->adap.class = I2C_CLASS_HWMON;
	i2c->adap.dev.parent = &pdev->dev;
	i2c->adap.dev.of_node = np;
	i2c->adap.nr = of_alias_get_id(np, "i2c");
	strlcpy(i2c->adap.name, "AK39 I2C v2", sizeof(i2c->adap.name));
	i2c_set_adapdata(&i2c->adap, i2c);
	platform_set_drvdata(pdev, i2c);

	ret = i2c_add_numbered_adapter(&i2c->adap);
	if (ret) {
		clk_disable_unprepare(i2c->clk);
		return ret;
	}
	return 0;
}

static int ak_i2c2_remove(struct platform_device *pdev)
{
	struct ak_i2c2 *i2c = platform_get_drvdata(pdev);

	i2c_del_adapter(&i2c->adap);
	clk_disable_unprepare(i2c->clk);
	return 0;
}

static const struct of_device_id ak_i2c2_of_match[] = {
	{ .compatible = "anyka,ak39ev330-i2c0" },
	{ .compatible = "anyka,ak39ev330-i2c1" },
	{ .compatible = "anyka,ak39ev330-i2c2" },
	{ .compatible = "anyka,ak3790d-i2c0" },
	{ .compatible = "anyka,ak3790d-i2c1" },
	{ .compatible = "anyka,ak3790d-i2c2" },
	{ .compatible = "anyka,ak3790d-i2c3" },
	{ }
};
MODULE_DEVICE_TABLE(of, ak_i2c2_of_match);

static struct platform_driver ak_i2c2_driver = {
	.probe		= ak_i2c2_probe,
	.remove		= ak_i2c2_remove,
	.driver		= {
		.name	= "i2c-ak39-v2",
		.of_match_table = ak_i2c2_of_match,
	},
};

/* Before the sensors, as the old controller's driver does. */
static int __init ak_i2c2_init(void)
{
	return platform_driver_register(&ak_i2c2_driver);
}
subsys_initcall(ak_i2c2_init);

static void __exit ak_i2c2_exit(void)
{
	platform_driver_unregister(&ak_i2c2_driver);
}
module_exit(ak_i2c2_exit);

MODULE_DESCRIPTION("Anyka AK39EV330 (ak3790d) I2C master");
MODULE_LICENSE("GPL v2");
