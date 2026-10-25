// SPDX-License-Identifier: GPL-2.0
/*
 * Fullhan FH8626V100 I2C controller
 *
 * The controller is a Synopsys DesignWare APB I2C core, used here as an
 * interrupt-driven master.
 *
 * Copyright (C) 2026 Difan Zhang
 * Licensed under the GNU General Public License version 2.
 */

#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm.h>

#define DW_IC_CON			0x00
#define DW_IC_TAR			0x04
#define DW_IC_DATA_CMD			0x10
#define DW_IC_SS_SCL_HCNT		0x14
#define DW_IC_SS_SCL_LCNT		0x18
#define DW_IC_FS_SCL_HCNT		0x1c
#define DW_IC_FS_SCL_LCNT		0x20
#define DW_IC_INTR_STAT			0x2c
#define DW_IC_INTR_MASK			0x30
#define DW_IC_RAW_INTR_STAT		0x34
#define DW_IC_RX_TL			0x38
#define DW_IC_TX_TL			0x3c
#define DW_IC_CLR_INTR			0x40
#define DW_IC_CLR_RX_UNDER		0x44
#define DW_IC_CLR_RX_OVER		0x48
#define DW_IC_CLR_TX_OVER		0x4c
#define DW_IC_CLR_RD_REQ		0x50
#define DW_IC_CLR_TX_ABRT		0x54
#define DW_IC_CLR_RX_DONE		0x58
#define DW_IC_CLR_ACTIVITY		0x5c
#define DW_IC_CLR_STOP_DET		0x60
#define DW_IC_CLR_START_DET		0x64
#define DW_IC_CLR_GEN_CALL		0x68
#define DW_IC_ENABLE			0x6c
#define DW_IC_STATUS			0x70
#define DW_IC_TXFLR			0x74
#define DW_IC_RXFLR			0x78
#define DW_IC_SDA_HOLD			0x7c
#define DW_IC_TX_ABRT_SOURCE		0x80
#define DW_IC_COMP_PARAM_1		0xf4

#define DW_IC_CON_MASTER_MODE		BIT(0)
#define DW_IC_CON_SPEED_STD		BIT(1)
#define DW_IC_CON_SPEED_FAST		BIT(2)
#define DW_IC_CON_10BITADDR_SLAVE	BIT(3)
#define DW_IC_CON_10BITADDR_MASTER	BIT(4)
#define DW_IC_CON_RESTART_EN		BIT(5)
#define DW_IC_CON_SLAVE_DISABLE		BIT(6)

#define DW_IC_TAR_10BITADDR_MASTER	BIT(12)

#define DW_IC_CMD_READ			BIT(8)
#define DW_IC_CMD_STOP			BIT(9)
#define DW_IC_CMD_RESTART		BIT(10)

#define DW_IC_STATUS_ACTIVITY		BIT(0)
#define DW_IC_STATUS_TFNF		BIT(1)	/* Tx FIFO Not Full */
#define DW_IC_STATUS_TFE		BIT(2)	/* Tx FIFO Empty */
#define DW_IC_STATUS_RFNE		BIT(3)	/* Rx FIFO Not Empty */
#define DW_IC_STATUS_RFF		BIT(4)	/* Rx FIFO Full */
#define DW_IC_STATUS_MST_ACTIVITY	BIT(5)	/* Master FSM Activity */

#define DW_IC_INTR_RX_UNDER		BIT(0)
#define DW_IC_INTR_RX_OVER		BIT(1)
#define DW_IC_INTR_RX_FULL		BIT(2)
#define DW_IC_INTR_TX_OVER		BIT(3)
#define DW_IC_INTR_TX_EMPTY		BIT(4)
#define DW_IC_INTR_RD_REQ		BIT(5)
#define DW_IC_INTR_TX_ABRT		BIT(6)
#define DW_IC_INTR_RX_DONE		BIT(7)
#define DW_IC_INTR_ACTIVITY		BIT(8)
#define DW_IC_INTR_STOP_DET		BIT(9)
#define DW_IC_INTR_START_DET		BIT(10)
#define DW_IC_INTR_GEN_CALL		BIT(11)

#define DW_IC_INTR_DEFAULT_MASK		(DW_IC_INTR_RX_FULL | \
					 DW_IC_INTR_TX_EMPTY | \
					 DW_IC_INTR_TX_ABRT | \
					 DW_IC_INTR_STOP_DET)

struct fh_i2c_dev {
	struct device		*dev;
	void __iomem		*base;
	struct completion	cmd_complete;
	struct clk		*clk;
	struct i2c_adapter	adapter;
	struct mutex		lock;
	int			irq;
	u32			tx_fifo_depth;
	u32			rx_fifo_depth;

	struct i2c_msg		*msgs;
	int			msgs_num;
	int			msg_write_idx;
	u32			msg_buf_idx;
	int			msg_read_idx;
	u32			rx_buf_idx;
	u32			rx_outstanding;
	u32			rx_remaining;
	int			cmd_err;
	u32			abort_source;
	u32			bus_freq;
	bool			stop_detected;
};

static inline u32 fh_i2c_read(struct fh_i2c_dev *i2c, unsigned int reg)
{
	return readl(i2c->base + reg);
}

static inline void fh_i2c_write(struct fh_i2c_dev *i2c, unsigned int reg,
				u32 val)
{
	writel(val, i2c->base + reg);
}

static int fh_i2c_wait_bus_not_busy(struct fh_i2c_dev *i2c)
{
	int count = 200;

	while ((fh_i2c_read(i2c, DW_IC_STATUS) & DW_IC_STATUS_MST_ACTIVITY) &&
	       count--)
		usleep_range(20, 50);

	if (count <= 0) {
		dev_warn(i2c->dev, "I2C bus timeout waiting for master idle\n");
		return -ETIMEDOUT;
	}
	return 0;
}

static int fh_i2c_disable(struct fh_i2c_dev *i2c)
{
	int ret = fh_i2c_wait_bus_not_busy(i2c);

	/* Request a stop on timeout, but do not permit reconfiguration. */
	fh_i2c_write(i2c, DW_IC_ENABLE, 0);
	fh_i2c_write(i2c, DW_IC_INTR_MASK, 0);
	return ret;
}

static int fh_i2c_init_hw(struct fh_i2c_dev *i2c)
{
	unsigned long rate = clk_get_rate(i2c->clk);
	u32 ss_hcnt, ss_lcnt, fs_hcnt, fs_lcnt;
	u32 con;
	int ret;

	ret = fh_i2c_disable(i2c);
	if (ret)
		return ret;

	/* Standard 100kHz speed counts: ss_hcnt = 212, ss_lcnt = 284 @ 50MHz */
	ss_hcnt = max_t(unsigned long, (rate / 100000) * 43 / 100, 11) - 3;
	ss_lcnt = max_t(unsigned long, (rate / 100000) * 57 / 100, 9) - 1;

	/*
	 * Fast 400kHz speed counts: fs_hcnt = 42, fs_lcnt = 79 @ 50MHz.
	 * The fast-mode counts stay at their minimum on a slow input clock.
	 */
	fs_hcnt = max_t(unsigned long, (rate / 400000) * 36 / 100, 11) - 3;
	fs_lcnt = max_t(unsigned long, (rate / 400000) * 64 / 100, 9) - 1;

	fh_i2c_write(i2c, DW_IC_SS_SCL_HCNT, ss_hcnt);
	fh_i2c_write(i2c, DW_IC_SS_SCL_LCNT, ss_lcnt);
	fh_i2c_write(i2c, DW_IC_FS_SCL_HCNT, fs_hcnt);
	fh_i2c_write(i2c, DW_IC_FS_SCL_LCNT, fs_lcnt);

	/* RX_TL = 0 raises RX_FULL as soon as one byte is in the RX FIFO. */
	fh_i2c_write(i2c, DW_IC_TX_TL, 0);
	fh_i2c_write(i2c, DW_IC_RX_TL, 0);

	con = DW_IC_CON_MASTER_MODE |
	      DW_IC_CON_RESTART_EN |
	      DW_IC_CON_SLAVE_DISABLE;
	if (i2c->bus_freq <= 100000)
		con |= DW_IC_CON_SPEED_STD;
	else
		con |= DW_IC_CON_SPEED_FAST;

	fh_i2c_write(i2c, DW_IC_CON, con);
	return 0;
}

static int fh_i2c_xfer_init(struct fh_i2c_dev *i2c)
{
	struct i2c_msg *msg = &i2c->msgs[0];
	u32 con, tar = msg->addr;
	int ret;

	ret = fh_i2c_disable(i2c);
	if (ret)
		return ret;

	/* 10-bit targets select the extended target address. */
	if (msg->flags & I2C_M_TEN)
		tar |= DW_IC_TAR_10BITADDR_MASTER;
	fh_i2c_write(i2c, DW_IC_TAR, tar);

	con = fh_i2c_read(i2c, DW_IC_CON);
	if (msg->flags & I2C_M_TEN)
		con |= DW_IC_CON_10BITADDR_MASTER;
	else
		con &= ~DW_IC_CON_10BITADDR_MASTER;
	fh_i2c_write(i2c, DW_IC_CON, con);

	fh_i2c_read(i2c, DW_IC_CLR_INTR);

	fh_i2c_write(i2c, DW_IC_ENABLE, 1);
	fh_i2c_write(i2c, DW_IC_INTR_MASK, DW_IC_INTR_DEFAULT_MASK);
	return 0;
}

static int fh_i2c_read_rx_fifo(struct fh_i2c_dev *i2c)
{
	struct i2c_msg *msg;

	while (fh_i2c_read(i2c, DW_IC_STATUS) & DW_IC_STATUS_RFNE) {
		while (i2c->msg_read_idx < i2c->msgs_num &&
		       !(i2c->msgs[i2c->msg_read_idx].flags & I2C_M_RD))
			i2c->msg_read_idx++;
		if (i2c->msg_read_idx >= i2c->msgs_num ||
		    !i2c->rx_outstanding || !i2c->rx_remaining)
			return -EIO;

		msg = &i2c->msgs[i2c->msg_read_idx];
		if (i2c->rx_buf_idx >= msg->len)
			return -EIO;
		msg->buf[i2c->rx_buf_idx++] =
			(u8)(fh_i2c_read(i2c, DW_IC_DATA_CMD) & 0xff);
		i2c->rx_outstanding--;
		i2c->rx_remaining--;

		if (i2c->rx_buf_idx >= msg->len) {
			i2c->msg_read_idx++;
			i2c->rx_buf_idx = 0;
		}
	}
	return 0;
}

static void fh_i2c_fill_tx_fifo(struct fh_i2c_dev *i2c)
{
	struct i2c_msg *msg;
	u32 cmd;

	while (i2c->msg_write_idx < i2c->msgs_num) {
		msg = &i2c->msgs[i2c->msg_write_idx];

		if (!(fh_i2c_read(i2c, DW_IC_STATUS) & DW_IC_STATUS_TFNF))
			break;

		if ((msg->flags & I2C_M_RD) &&
		    (i2c->rx_outstanding >= i2c->rx_fifo_depth))
			break;

		cmd = 0;

		if (i2c->msg_buf_idx == 0 && i2c->msg_write_idx > 0)
			cmd |= DW_IC_CMD_RESTART;

		if (i2c->msg_write_idx == i2c->msgs_num - 1 &&
		    i2c->msg_buf_idx == msg->len - 1)
			cmd |= DW_IC_CMD_STOP;

		if (msg->flags & I2C_M_RD) {
			cmd |= DW_IC_CMD_READ;
			fh_i2c_write(i2c, DW_IC_DATA_CMD, cmd);
			i2c->rx_outstanding++;
		} else {
			cmd |= (msg->buf[i2c->msg_buf_idx] & 0xff);
			fh_i2c_write(i2c, DW_IC_DATA_CMD, cmd);
		}

		i2c->msg_buf_idx++;
		if (i2c->msg_buf_idx >= msg->len) {
			i2c->msg_write_idx++;
			i2c->msg_buf_idx = 0;
		}
	}

	if (i2c->msg_write_idx >= i2c->msgs_num) {
		u32 intr_mask = fh_i2c_read(i2c, DW_IC_INTR_MASK);

		intr_mask &= ~DW_IC_INTR_TX_EMPTY;
		fh_i2c_write(i2c, DW_IC_INTR_MASK, intr_mask);
	}
}

static irqreturn_t fh_i2c_isr(int irq, void *dev_id)
{
	struct fh_i2c_dev *i2c = dev_id;
	u32 stat;

	stat = fh_i2c_read(i2c, DW_IC_INTR_STAT);
	if (!stat)
		return IRQ_NONE;

	if (stat & DW_IC_INTR_TX_ABRT) {
		i2c->abort_source = fh_i2c_read(i2c, DW_IC_TX_ABRT_SOURCE);
		fh_i2c_read(i2c, DW_IC_CLR_TX_ABRT);
		i2c->cmd_err = -EIO;
		fh_i2c_write(i2c, DW_IC_INTR_MASK, 0);
		complete(&i2c->cmd_complete);
		return IRQ_HANDLED;
	}

	if (stat & DW_IC_INTR_STOP_DET) {
		fh_i2c_read(i2c, DW_IC_CLR_STOP_DET);
		i2c->stop_detected = true;
	}

	if ((stat & DW_IC_INTR_RX_FULL) || i2c->stop_detected)
		i2c->cmd_err = fh_i2c_read_rx_fifo(i2c);

	/* A STOP must terminate the whole transfer, including every RX byte. */
	if (i2c->stop_detected &&
	    (i2c->msg_write_idx != i2c->msgs_num ||
	     i2c->rx_outstanding || i2c->rx_remaining))
		i2c->cmd_err = -EIO;

	if (i2c->cmd_err || i2c->stop_detected) {
		fh_i2c_write(i2c, DW_IC_INTR_MASK, 0);
		complete(&i2c->cmd_complete);
		return IRQ_HANDLED;
	}

	if (stat & DW_IC_INTR_TX_EMPTY)
		fh_i2c_fill_tx_fifo(i2c);

	return IRQ_HANDLED;
}

static int fh_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	struct fh_i2c_dev *i2c = i2c_get_adapdata(adap);
	u32 rx_remaining = 0;
	int i, ret, stop_ret;

	if (!num)
		return 0;
	if (num < 0)
		return -EINVAL;

	/* No address-only command exists in this FIFO transfer engine. */
	for (i = 0; i < num; i++) {
		if (!msgs[i].len)
			return -EOPNOTSUPP;
		if (!msgs[i].buf)
			return -EINVAL;
		if (msgs[i].flags & ~(I2C_M_RD | I2C_M_TEN))
			return -EOPNOTSUPP;
		/* TAR is fixed for a combined transfer. */
		if (msgs[i].addr != msgs[0].addr ||
		    ((msgs[i].flags ^ msgs[0].flags) & I2C_M_TEN))
			return -EOPNOTSUPP;
		if (msgs[i].flags & I2C_M_RD) {
			if (rx_remaining > U32_MAX - msgs[i].len)
				return -EOVERFLOW;
			rx_remaining += msgs[i].len;
		}
	}

	mutex_lock(&i2c->lock);

	i2c->msgs = msgs;
	i2c->msgs_num = num;
	i2c->msg_write_idx = 0;
	i2c->msg_buf_idx = 0;
	i2c->msg_read_idx = 0;
	i2c->rx_buf_idx = 0;
	i2c->rx_outstanding = 0;
	i2c->rx_remaining = rx_remaining;
	i2c->cmd_err = 0;
	i2c->abort_source = 0;
	i2c->stop_detected = false;

	reinit_completion(&i2c->cmd_complete);

	ret = fh_i2c_xfer_init(i2c);
	if (ret)
		goto out_no_recovery;

	ret = wait_for_completion_timeout(&i2c->cmd_complete,
					  msecs_to_jiffies(1000));
	if (ret == 0) {
		dev_err(i2c->dev, "I2C transfer timed out\n");
		ret = -ETIMEDOUT;
		goto out;
	}

	if (i2c->cmd_err) {
		dev_dbg(i2c->dev, "I2C transfer aborted: 0x%08x\n",
			i2c->abort_source);
		ret = i2c->cmd_err;
		goto out;
	}

	ret = num;
out:
	fh_i2c_write(i2c, DW_IC_INTR_MASK, 0);
	fh_i2c_read(i2c, DW_IC_INTR_MASK);
	synchronize_irq(i2c->irq);
	stop_ret = fh_i2c_disable(i2c);
	i2c->msgs = NULL;
	if (!stop_ret && ret < 0)
		stop_ret = fh_i2c_init_hw(i2c);
	if (stop_ret)
		ret = stop_ret;
	mutex_unlock(&i2c->lock);
	return ret;

out_no_recovery:
	/* The controller was already busy; leave its configuration alone. */
	fh_i2c_write(i2c, DW_IC_INTR_MASK, 0);
	fh_i2c_read(i2c, DW_IC_INTR_MASK);
	synchronize_irq(i2c->irq);
	i2c->msgs = NULL;
	mutex_unlock(&i2c->lock);
	return ret;
}

static u32 fh_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C |
	       I2C_FUNC_10BIT_ADDR |
	       (I2C_FUNC_SMBUS_EMUL & ~I2C_FUNC_SMBUS_QUICK);
}

static const struct i2c_algorithm fh_i2c_algo = {
	.master_xfer	= fh_i2c_xfer,
	.functionality	= fh_i2c_func,
};

static int fh_i2c_probe(struct platform_device *pdev)
{
	struct fh_i2c_dev *i2c;
	struct resource *res;
	u32 comp_param;
	int ret;

	i2c = devm_kzalloc(&pdev->dev, sizeof(*i2c), GFP_KERNEL);
	if (!i2c)
		return -ENOMEM;

	i2c->dev = &pdev->dev;
	mutex_init(&i2c->lock);
	init_completion(&i2c->cmd_complete);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	i2c->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(i2c->base))
		return PTR_ERR(i2c->base);

	i2c->irq = platform_get_irq(pdev, 0);
	if (i2c->irq < 0) {
		dev_err(&pdev->dev, "failed to get IRQ: %d\n", i2c->irq);
		return i2c->irq;
	}

	i2c->clk = devm_clk_get(&pdev->dev, "i2c");
	if (IS_ERR(i2c->clk)) {
		dev_err(&pdev->dev, "failed to get clock: %ld\n",
			PTR_ERR(i2c->clk));
		return PTR_ERR(i2c->clk);
	}

	ret = clk_prepare_enable(i2c->clk);
	if (ret) {
		dev_err(&pdev->dev, "failed to enable clock: %d\n", ret);
		return ret;
	}
	if (!clk_get_rate(i2c->clk)) {
		dev_err(&pdev->dev, "I2C input clock has no usable rate\n");
		ret = -EINVAL;
		goto err_clk;
	}

	comp_param = fh_i2c_read(i2c, DW_IC_COMP_PARAM_1);
	i2c->tx_fifo_depth = ((comp_param >> 16) & 0xff) + 1;
	i2c->rx_fifo_depth = ((comp_param >> 8) & 0xff) + 1;
	if (i2c->tx_fifo_depth == 1 || i2c->tx_fifo_depth > 256)
		i2c->tx_fifo_depth = 16;
	if (i2c->rx_fifo_depth == 1 || i2c->rx_fifo_depth > 256)
		i2c->rx_fifo_depth = 16;
	ret = of_property_read_u32(pdev->dev.of_node, "clock-frequency",
				   &i2c->bus_freq);
	if (ret) {
		dev_err(&pdev->dev, "failed to read clock-frequency: %d\n",
			ret);
		ret = -EINVAL;
		goto err_clk;
	}

	ret = fh_i2c_init_hw(i2c);
	if (ret)
		goto err_clk;

	ret = devm_request_irq(&pdev->dev, i2c->irq, fh_i2c_isr,
			       IRQF_SHARED, dev_name(&pdev->dev), i2c);
	if (ret) {
		dev_err(&pdev->dev, "failed to request IRQ %d: %d\n",
			i2c->irq, ret);
		goto err_clk;
	}

	i2c_set_adapdata(&i2c->adapter, i2c);
	i2c->adapter.owner = THIS_MODULE;
	i2c->adapter.class = I2C_CLASS_DEPRECATED;
	i2c->adapter.algo = &fh_i2c_algo;
	i2c->adapter.dev.parent = &pdev->dev;
	i2c->adapter.dev.of_node = pdev->dev.of_node;
	strlcpy(i2c->adapter.name, "Fullhan FH8626 I2C adapter",
		sizeof(i2c->adapter.name));

	if (pdev->dev.of_node)
		i2c->adapter.nr = pdev->id >= 0 ? pdev->id :
			of_alias_get_id(pdev->dev.of_node, "i2c");
	else
		i2c->adapter.nr = pdev->id;

	platform_set_drvdata(pdev, i2c);

	if (i2c->adapter.nr >= 0)
		ret = i2c_add_numbered_adapter(&i2c->adapter);
	else
		ret = i2c_add_adapter(&i2c->adapter);

	if (ret) {
		dev_err(&pdev->dev, "failed to add I2C adapter: %d\n", ret);
		goto err_irq;
	}

	return 0;

err_irq:
	fh_i2c_disable(i2c);
	devm_free_irq(&pdev->dev, i2c->irq, i2c);
err_clk:
	clk_disable_unprepare(i2c->clk);
	return ret;
}

static int fh_i2c_remove(struct platform_device *pdev)
{
	struct fh_i2c_dev *i2c = platform_get_drvdata(pdev);

	i2c_del_adapter(&i2c->adapter);
	fh_i2c_disable(i2c);
	devm_free_irq(&pdev->dev, i2c->irq, i2c);
	clk_disable_unprepare(i2c->clk);
	return 0;
}

static const struct of_device_id fh_i2c_of_match[] = {
	{ .compatible = "fullhan,fh8626v100-i2c" },
	{ .compatible = "snps,designware-i2c" },
	{ }
};
MODULE_DEVICE_TABLE(of, fh_i2c_of_match);

static struct platform_driver fh_i2c_driver = {
	.probe = fh_i2c_probe,
	.remove = fh_i2c_remove,
	.driver = {
		.name = "fh8626-i2c",
		.of_match_table = fh_i2c_of_match,
	},
};
module_platform_driver(fh_i2c_driver);

MODULE_DESCRIPTION("Fullhan FH8626V100 I2C Controller Driver");
MODULE_AUTHOR("Difan Zhang");
MODULE_LICENSE("GPL v2");
