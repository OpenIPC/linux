/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/scatterlist.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/property.h>
#include <linux/regmap.h>
#include <linux/kfifo.h>
#include <linux/sched.h>
#include <uapi/linux/sched/types.h>
#include <linux/reset.h>
#include <linux/ax_printk.h>
#include <linux/miscdevice.h>
#include "spi-axera-slv-raw.h"
#include <linux/iopoll.h>
#include <linux/kthread.h>
#include <linux/ioctl.h>
#include "spi-axera-uapi.h"

#define DRIVER_NAME "dw_spi_slv"
#define AX_RX_LEN 32
#define AX_TX_LEN 32

static unsigned char input_buf[AX_RX_LEN * 1024] = { 0 };
static unsigned char output_buf[DW_SPI_FIFO_LEN] = { 0 };

static int kfifo_size;

#ifdef SPI_SLAVE_DEBUG
long long int volatile max;
long long int volatile timerval1;
long int volatile timerval2;
long long int volatile timerval3;
#endif

static DECLARE_KFIFO(slave_rx_fifo, unsigned char, (AX_RX_LEN * 1024));

struct dw_spi_slv {
	u32 magic;
	void __iomem *regs;
	unsigned long paddr;
	int irq;
	u32 fifo_len;		/* depth of the FIFO buffer */
	u32 tx_len;
	u32 rx_len;
	u32 max_freq;		/* max bus freq supported */
	struct clk *clk;
	struct clk *hclk;
	struct reset_control *rstc;
	struct reset_control *prstc;
	void *priv;
	wait_queue_head_t rx_waitq;
	wait_queue_head_t tx_waitq;
	u32 rx_waitflags;
	u32 tx_waitflags;
	struct completion xfer_completion;
};

struct dw_poll_priv {
	struct platform_device *pdev;
	struct miscdevice miscdev;
	struct dw_spi_slv *dws_slv;
	u32 tx_entries;
	u32 rx_entries;
	void *tx;
	void *rx;
	u32 fifowidth;
	int stream_tx_gpio;
	int stream_rx_gpio;
	struct mutex stream_mutex;
};

static u32 slv_fifowidth = DFS_8_BIT;

static void slv_rx_handshk_gpio_init(struct dw_poll_priv *priv)
{
	gpio_direction_output(priv->stream_rx_gpio, 0);
}

static void slv_tx_handshk_gpio_init(struct dw_poll_priv *priv)
{
	gpio_direction_output(priv->stream_tx_gpio, 0);
}

static int slv_rx_handshk_gpio_invert(struct dw_poll_priv *priv, int enable)
{
	if (enable) {
		gpio_set_value(priv->stream_rx_gpio, 1);
	} else {
		gpio_set_value(priv->stream_rx_gpio, 0);
	}
	return 0;
}

static int slv_tx_handshk_gpio_invert(struct dw_poll_priv *priv, int enable)
{
	if (enable) {
		gpio_set_value(priv->stream_tx_gpio, 1);
	} else {
		gpio_set_value(priv->stream_tx_gpio, 0);
	}
	return 0;
}

static inline u32 dw_slv_read(struct dw_spi_slv *dws_slv, u32 offset)
{
	u32 val;
	val = __raw_readl(dws_slv->regs + offset);
	return val;
}

static inline void dw_slv_write(struct dw_spi_slv *dws_slv, u32 offset, u32 val)
{
	__raw_writel(val, dws_slv->regs + offset);
}

static void spi_slv_enable_chip(struct dw_spi_slv *dws_slv, int enable)
{
	dw_slv_write(dws_slv, DW_SPI_SSIENR, (enable ? 1 : 0));
}

static void dw_slv_reader(struct dw_poll_priv *priv)
{
	/* get the max entries we should read out of rx fifo */
	int dw = priv->fifowidth >> 3;
	int rx_left = priv->rx_entries;
	int rx_avail = dw_slv_read(priv->dws_slv, DW_SPI_RXFLR);
	int rx_max = rx_left > rx_avail ? rx_avail : rx_left;
	priv->rx_entries = rx_left - rx_max;
	while (rx_max-- > 0 && priv->rx) {
		if (priv->fifowidth == DFS_8_BIT)
			*(u8 *) (priv->rx) = dw_slv_read(priv->dws_slv, DW_SPI_DR);
		else
			*(u32 *) (priv->rx) = dw_slv_read(priv->dws_slv, DW_SPI_DR);
		priv->rx += dw;
	}
	priv->dws_slv->rx_len = priv->rx_entries;
}

static void dw_slv_writer(struct dw_poll_priv *priv)
{
	/* get the max entries we can fill into tx fifo */
	int dw = priv->fifowidth >> 3;
	int tx_left = priv->tx_entries;
	int tx_avail = DW_SPI_FIFO_LEN / dw - dw_slv_read(priv->dws_slv, DW_SPI_TXFLR);
	int tx_max = tx_left > tx_avail ? tx_avail : tx_left;
	priv->tx_entries = tx_left - tx_max;
	while (tx_max-- && priv->tx) {
		if (priv->fifowidth == DFS_8_BIT)
			dw_slv_write(priv->dws_slv, DW_SPI_DR, (*(u8 *) (priv->tx)));
		else
			dw_slv_write(priv->dws_slv, DW_SPI_DR, (*(u32 *) (priv->tx)));
		priv->tx += dw;
	}
	priv->dws_slv->tx_len = priv->tx_entries;
}

/* Disable IRQ bits */
static inline void spi_s_mask_intr(struct dw_spi_slv *dws_slv, u32 mask)
{
	u32 new_mask;

	new_mask = dw_slv_read(dws_slv, DW_SPI_IMR) & ~mask;
	dw_slv_write(dws_slv, DW_SPI_IMR, new_mask);
}

/* Enable IRQ bits */
static inline void spi_s_umask_intr(struct dw_spi_slv *dws_slv, u32 mask)
{
	u32 new_mask;

	new_mask = dw_slv_read(dws_slv, DW_SPI_IMR) | mask;
	dw_slv_write(dws_slv, DW_SPI_IMR, new_mask);
}


static void spi_slv_switch_mode(struct dw_spi_slv *dws_slv, unsigned int xfer_mode)
{
	u32 cr0;

	switch (xfer_mode) {
	case SPI_TMOD_RO:
		cr0 = dw_slv_read(dws_slv, DW_SPI_CTRL0);
		cr0 &= ~(SPI_TMOD_MASK | SPI_SLOVE_MASK);
		cr0 |= (SPI_TMOD_RO << SPI_TMOD_OFFSET) | (SPI_SLOVE_DISABLE << SPI_SLOVE_OFFSET);
		break;
	case SPI_TMOD_TO:
		cr0 = dw_slv_read(dws_slv, DW_SPI_CTRL0);
		cr0 &= ~(SPI_TMOD_MASK | SPI_SLOVE_MASK);
		cr0 |= (SPI_TMOD_TO << SPI_TMOD_OFFSET) | (SPI_SLOVE_ENABLE << SPI_SLOVE_OFFSET);
		break;
	case SPI_TMOD_TR:
		cr0 = dw_slv_read(dws_slv, DW_SPI_CTRL0);
		cr0 &= ~(SPI_TMOD_MASK | SPI_SLOVE_MASK);
		cr0 |= (SPI_TMOD_TR << SPI_TMOD_OFFSET);
		break;
	default:
		break;
	}

	spi_slv_enable_chip(dws_slv, 0);
	dw_slv_write(dws_slv, DW_SPI_CTRL0, cr0);
	spi_s_mask_intr(dws_slv, 0xff);
	spi_slv_enable_chip(dws_slv, 1);
}

int dw_spi_s_check_status(struct dw_spi_slv *dws_slv, bool raw)
{
	u32 irq_status, mask;
	int ret = 0;

	if (raw)
		irq_status = dw_slv_read(dws_slv, DW_SPI_RISR);
	else
		irq_status = dw_slv_read(dws_slv, DW_SPI_ISR);

	if (irq_status & SPI_INT_RXOI) {
		pr_err("RX FIFO overflow detected\n");
		ret = -EIO;
	}

	if (irq_status & SPI_INT_RXUI) {
		pr_err("RX FIFO underflow detected\n");
		ret = -EIO;
	}

	if (irq_status & SPI_INT_TXOI) {
		pr_err("TX FIFO overflow detected\n");
		ret = -EIO;
	}

	if (irq_status & SPI_INT_RXFI) {
		pr_debug("RX FIFO FULL\n");

	}
	if (irq_status & SPI_INT_TXEI) {
		pr_debug("TX FIFO empty\n");
	}

	/* Generically handle the erroneous situation */
	if (ret) {
		spi_slv_enable_chip(dws_slv, 0);
		mask = dw_slv_read(dws_slv, DW_SPI_IMR);
		mask &= ~0xff;
		dw_slv_write(dws_slv, DW_SPI_IMR, mask);
		spi_slv_enable_chip(dws_slv, 1);
	}

	return ret;
}

static void dw_spi_s_irq_setup(struct dw_poll_priv *priv_slv)
{
	u16 level;
	u32 mask = 0;
	struct dw_spi_slv *dws_slv = priv_slv->dws_slv;
	/*
	 * Originally Tx and Rx data lengths match. Rx FIFO Threshold level
	 * will be adjusted at the final stage of the IRQ-based SPI transfer
	 * execution so not to lose the leftover of the incoming data.
	 */
	mask = dw_slv_read(dws_slv, DW_SPI_IMR);
	if (priv_slv->rx) {
		level = min_t(u16, dws_slv->fifo_len / 2, priv_slv->rx_entries);
		dw_slv_write(dws_slv, DW_SPI_RXFTLR, level - 1);
		mask |= (SPI_INT_RXUI | SPI_INT_RXOI | SPI_INT_RXFI);
	} else {		//tx
		mask |= (SPI_INT_TXOI | SPI_INT_TXEI);
	}
	dw_slv_write(dws_slv, DW_SPI_IMR, mask);
}

static int dw_spi_slv_xfer(struct dw_poll_priv *priv_slv, unsigned int nbytes, const void *dout, void *din)
{
	u8 *rx = din;
	const u8 *tx = (void *)dout;
	int ret = 0;
	if (tx) {
		spi_slv_switch_mode(priv_slv->dws_slv, SPI_TMOD_TO);
		priv_slv->tx_entries = nbytes / (slv_fifowidth >> 3);
		priv_slv->dws_slv->tx_len = priv_slv->tx_entries;
	}

	if (rx) {
		spi_slv_switch_mode(priv_slv->dws_slv, SPI_TMOD_RO);
		priv_slv->rx_entries = nbytes / (slv_fifowidth >> 3);
		priv_slv->dws_slv->rx_len = priv_slv->rx_entries;
	}
	priv_slv->fifowidth = slv_fifowidth;
	priv_slv->tx = (void *)tx;
	priv_slv->rx = (void *)rx;
	return ret;
}

static int spi_slv_rx_tx_init(struct dw_poll_priv *priv_slv, unsigned int nbytes, void *dout, void *din)
{
	int ret;
	ret = dw_spi_slv_xfer(priv_slv, nbytes, dout, din);
	if (ret < 0) {
		pr_err("%s: ret=%d\n", __FUNCTION__, ret);
		return ret;
	}
	return nbytes;
}

static irqreturn_t dw_spi_s_xfer_handler(struct dw_poll_priv *priv_slv)
{
	u32 new_mask;
	u16 irq_status;
	struct dw_spi_slv *dws_slv = priv_slv->dws_slv;
	int read_finish_flag = 0;
#ifdef SPI_SLAVE_DEBUG
	timerval1 = local_clock();
#endif
	irq_status = dw_slv_read(dws_slv, DW_SPI_ISR);
	if (dw_spi_s_check_status(dws_slv, false)) {
		return IRQ_HANDLED;
	}

	/*
	 * Read data from the Rx FIFO every time we've got a chance executing
	 * this method. If there is nothing left to receive, terminate the
	 * procedure. Otherwise adjust the Rx FIFO Threshold level if it's a
	 * final stage of the transfer. By doing so we'll get the next IRQ
	 * right when the leftover incoming data is received.
	 */
	if (irq_status & SPI_INT_RXFI) {
		dw_slv_reader(priv_slv);
		if (!dws_slv->rx_len) {
			spi_s_mask_intr(dws_slv, SPI_INT_RXUI | SPI_INT_RXOI | SPI_INT_RXFI);
			read_finish_flag = 1;
		} else if (dws_slv->rx_len <= dw_slv_read(dws_slv, DW_SPI_RXFTLR)) {
			dw_slv_write(dws_slv, DW_SPI_RXFTLR, dws_slv->rx_len - 1);
		}
	}

	if (irq_status & SPI_INT_TXEI) {
		// spi_s_mask_intr(dws_slv, (SPI_INT_TXOI | SPI_INT_TXEI));
		new_mask = dw_slv_read(dws_slv, DW_SPI_IMR) & ~(SPI_INT_TXOI | SPI_INT_TXEI);
		dw_slv_write(dws_slv, DW_SPI_IMR, new_mask);
		dws_slv->tx_waitflags = 1;
		slv_tx_handshk_gpio_invert(priv_slv, 0);
		wake_up_interruptible(&dws_slv->tx_waitq);
	}
#ifdef SPI_SLAVE_DEBUG
	timerval2 = local_clock();
#endif

	if (read_finish_flag == 1) {
		if (AX_RX_LEN == kfifo_in(&slave_rx_fifo, input_buf, AX_RX_LEN)) {
			dws_slv->rx_waitflags = 1;
			read_finish_flag = 0;
			wake_up_interruptible(&dws_slv->rx_waitq);
		} else {
			ax_printk(0, "spi", 0, "%s: kfifo full \n", __func__);
		}
	}
#ifdef SPI_SLAVE_DEBUG
	timerval3 = local_clock();
	if (max < (timerval3 - timerval1)) {
		max = timerval3 - timerval1;
		ax printk("max %lld self %lld axirq %lld\n", max, timerval3 - timerval2, timerval2 - timerval1).
	}
#endif
	return IRQ_HANDLED;
}

static irqreturn_t dw_spi_s_irq(int irq, void *dev_id)
{
	struct dw_poll_priv *priv_slv = (struct dw_poll_priv *)dev_id;

	u16 irq_status = dw_slv_read(priv_slv->dws_slv, DW_SPI_ISR) & 0x3f;
	if (!irq_status)
		return IRQ_NONE;

	dw_spi_s_xfer_handler(priv_slv);
	return IRQ_HANDLED;
}

static void spi_slv_hw_init(struct dw_spi_slv *dws_slv)
{
	u32 cr0;
	spi_slv_enable_chip(dws_slv, 0);
	dw_slv_write(dws_slv, DW_SPI_IMR, 0);
	cr0 = (slv_fifowidth - 1) << SPI_DFS_OFFSET;
	cr0 |= SPI_FRF_SPI << SPI_FRF_OFFSET;
	cr0 |= SPI_TMOD_RO << SPI_TMOD_OFFSET;
	// cr0 |= SPI_SLOVE_DISABLE << SPI_SLOVE_OFFSET;
	cr0 |= SPI_FRF_SPI_STANDARD << SPI_SPI_FRF_OFFSET;
	dw_slv_write(dws_slv, DW_SPI_CTRL0, cr0);
	spi_slv_enable_chip(dws_slv, 1);
}

static int spi_slv_open(struct inode *inode, struct file *filp)
{
	struct dw_poll_priv *priv_slv;
	priv_slv = container_of(filp->private_data, struct dw_poll_priv, miscdev);

	filp->private_data = priv_slv;
	return 0;
}

static long spi_slv_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0, rxsize, txsize, time = 0;
	int remain_size, tx_len, len;
	unsigned long timeout;
	struct dw_poll_priv *priv_slv;
	void __user *argp;
	struct dw_spi_slv *dws_slv;
	spi_data_t spi_data_u;
	priv_slv = filp->private_data;
	dws_slv = priv_slv->dws_slv;
	argp = (void __user *)arg;

	switch (cmd) {
	case SET_SPI_TX:
		mutex_lock(&priv_slv->stream_mutex);
		ret = copy_from_user(&spi_data_u, argp, sizeof(spi_data_t));
		if (ret != 0) {
			mutex_unlock(&priv_slv->stream_mutex);
			return -EFAULT;
		}
		txsize = spi_data_u.tx_size;
		remain_size = txsize;

		while (remain_size) {
			tx_len = AX_TX_LEN;
			if (remain_size > 0 && remain_size < AX_TX_LEN) {
				tx_len = remain_size;
			}

			ret = copy_from_user(output_buf, spi_data_u.tx_buf + (time * AX_TX_LEN), tx_len);
			if (ret != 0) {
				mutex_unlock(&priv_slv->stream_mutex);
				return -EFAULT;
			}

			/* open tx fifo empty interrupt */
			spi_slv_rx_tx_init(priv_slv, tx_len, (void *)output_buf, NULL);
			dw_slv_writer(priv_slv);
			dw_spi_s_irq_setup(priv_slv);
			slv_tx_handshk_gpio_invert(priv_slv, 1);
			if (wait_event_interruptible(priv_slv->dws_slv->tx_waitq, priv_slv->dws_slv->tx_waitflags)) {
				pr_err("%s wait event Failed!", __func__);
				priv_slv->dws_slv->tx_waitflags = 0;
			}
			priv_slv->dws_slv->tx_waitflags = 0;
			remain_size -= tx_len;
			time++;
		}
		mutex_unlock(&priv_slv->stream_mutex);
		break;

	case SET_SPI_RX:
		mutex_lock(&priv_slv->stream_mutex);
		timeout = msecs_to_jiffies(5000);
		ret = copy_from_user(&spi_data_u, argp, sizeof(spi_data_t));
		if (ret != 0) {
			mutex_unlock(&priv_slv->stream_mutex);
			return -EFAULT;
		}
		rxsize = spi_data_u.rx_size;
		if (rxsize > sizeof(input_buf)) {
			mutex_unlock(&priv_slv->stream_mutex);
			return -EFAULT;
		}
		spi_slv_rx_tx_init(priv_slv, AX_RX_LEN, NULL, (void *)input_buf);
		dw_spi_s_irq_setup(priv_slv);
retry:
		if (kfifo_len(&slave_rx_fifo) >= rxsize) {
			slv_rx_handshk_gpio_invert(priv_slv, 1);
			len = kfifo_out(&slave_rx_fifo, input_buf, rxsize);
			if ((len != rxsize) && (len != 0)) {
				pr_err(" %s: %d != %d\n", __func__, len, rxsize);
				mutex_unlock(&priv_slv->stream_mutex);
				return -EFAULT;
			} else {
				ret = copy_to_user(((spi_data_t *) argp)->rx_buf, input_buf, rxsize);
				if (ret != 0) {
					ret = -EFAULT;
				}
				slv_rx_handshk_gpio_invert(priv_slv, 0);
				dw_spi_s_irq_setup(priv_slv);
				mutex_unlock(&priv_slv->stream_mutex);
				return ret;
			}
		}
		slv_rx_handshk_gpio_invert(priv_slv, 1);
		if (wait_event_interruptible_timeout(priv_slv->dws_slv->rx_waitq, priv_slv->dws_slv->rx_waitflags, timeout) <= 0) {
			pr_err("%s wait event timeout!", __func__);
			kfifo_reset(&slave_rx_fifo);
			priv_slv->dws_slv->rx_waitflags = 0;
			mutex_unlock(&priv_slv->stream_mutex);
			return -EFAULT;
		}
		priv_slv->dws_slv->rx_waitflags = 0;
		goto retry;

	case GET_SPI_INPUT_KFIFO_SIZE:
		kfifo_size = kfifo_len(&slave_rx_fifo);
		ret = copy_to_user(argp, &kfifo_size, sizeof(kfifo_size));
		if (ret != 0) {
			ret = -EFAULT;
		}
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static const struct file_operations spi_slv_fops = {
	.owner = THIS_MODULE,
	.open = spi_slv_open,
	.unlocked_ioctl = spi_slv_ioctl,
};

static int register_spi_slv_dev(struct dw_poll_priv *priv_slv, struct platform_device *pdev)
{
	int ret;

	priv_slv->pdev = pdev;
	priv_slv->miscdev.minor = MISC_DYNAMIC_MINOR;
	priv_slv->miscdev.name = "axera_spi_slv";
	priv_slv->miscdev.fops = &spi_slv_fops;
	priv_slv->miscdev.parent = &pdev->dev;

	ret = misc_register(&priv_slv->miscdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register misc device\n");
		return ret;
	}
	platform_set_drvdata(pdev, priv_slv);
	return 0;
}

static int unregister_spi_slv_dev(struct platform_device *pdev)
{
	struct dw_poll_priv *priv_slv = platform_get_drvdata(pdev);
	misc_deregister(&priv_slv->miscdev);
	return 0;
}

static int dw_spi_slv_probe(struct platform_device *pdev)
{
	struct resource *mem;
	int ret;
	struct dw_poll_priv *priv_slv;
	struct dw_spi_slv *dws_slv;

	priv_slv = devm_kzalloc(&pdev->dev, sizeof(struct dw_poll_priv), GFP_KERNEL);
	if (!priv_slv)
		return -ENOMEM;

	priv_slv->dws_slv = devm_kzalloc(&pdev->dev, sizeof(struct dw_spi_slv), GFP_KERNEL);
	if (!priv_slv->dws_slv) {
		ret = -ENOMEM;
		goto err_free1;
	}

	dws_slv = priv_slv->dws_slv;
	dws_slv->magic = 0x5a5a;

	/* Get basic io resource and map it */
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dws_slv->regs = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(dws_slv->regs)) {
		dev_err(&pdev->dev, "SPI region map failed\n");
		ret = PTR_ERR(dws_slv->regs);
		goto err_free2;
	}

	dws_slv->paddr = mem->start;
	dws_slv->irq = platform_get_irq(pdev, 0);
	if (dws_slv->irq < 0) {
		dev_err(&pdev->dev, "no irq resource?\n");
		ret = dws_slv->irq;	/* -ENXIO */
		goto err_free2;
	}

	dws_slv->hclk = devm_clk_get(&pdev->dev, "hclk");
	if (IS_ERR(dws_slv->hclk)) {
		pr_err("%s: dws_slv->hclk get fail\n", __FUNCTION__);
		ret = PTR_ERR(dws_slv->hclk);
		goto err_free2;
	}
	ret = clk_prepare_enable(dws_slv->hclk);
	if (ret) {
		goto err_free2;
	}

	dws_slv->clk = devm_clk_get(&pdev->dev, "ahb_ssi_s_clk");
	if (IS_ERR(dws_slv->clk)) {
		pr_err("%s: dws_slv->clk get fail\n", __FUNCTION__);
		ret = PTR_ERR(dws_slv->clk);
		goto free_hclk;
	}

	clk_set_rate(dws_slv->clk, 208000000);

	ret = clk_prepare_enable(dws_slv->clk);
	if (ret) {
		goto free_hclk;
	}

	dws_slv->max_freq = clk_get_rate(dws_slv->clk);

	/* find an optional reset controller */
	dws_slv->rstc = devm_reset_control_get_optional_exclusive(&pdev->dev, "rst");
	if (IS_ERR(dws_slv->rstc)) {
		dev_err(&pdev->dev, "%s: dws_slv->rstc failed\n", __func__);
		ret = PTR_ERR(dws_slv->rstc);
		goto free_clk;
	}
	reset_control_deassert(dws_slv->rstc);

	dws_slv->prstc = devm_reset_control_get_optional_exclusive(&pdev->dev, "prst");
	if (IS_ERR(dws_slv->prstc)) {
		dev_err(&pdev->dev, "%s: dws_slv->prstc failed\n", __func__);
		ret = PTR_ERR(dws_slv->prstc);
		goto free_rst;
	}
	reset_control_deassert(dws_slv->prstc);

	priv_slv->stream_tx_gpio = of_get_named_gpio_flags(pdev->dev.of_node, "stream-tx-gpio", 0, NULL);
	ret = gpio_request(priv_slv->stream_tx_gpio, "stream_gpio_tx");
	if (ret < 0) {
		pr_err("stream-gpio-tx gpio request failed\n");
		goto free_prst;
	}

	priv_slv->stream_rx_gpio = of_get_named_gpio_flags(pdev->dev.of_node, "stream-rx-gpio", 0, NULL);
	ret = gpio_request(priv_slv->stream_rx_gpio, "stream_gpio_rx");
	if (ret < 0) {
		pr_err("stream-gpio-rx gpio request failed\n");
		goto err_gpio_tx;
	}

	slv_tx_handshk_gpio_init(priv_slv);
	slv_rx_handshk_gpio_init(priv_slv);
	gpio_export(priv_slv->stream_tx_gpio, false);
	gpio_export(priv_slv->stream_rx_gpio, false);

	dws_slv->fifo_len = DW_SPI_FIFO_LEN;
	INIT_KFIFO(slave_rx_fifo);
	init_waitqueue_head(&dws_slv->rx_waitq);
	init_waitqueue_head(&dws_slv->tx_waitq);

	register_spi_slv_dev(priv_slv, pdev);
	mutex_init(&priv_slv->stream_mutex);
	spi_slv_hw_init(dws_slv);
	spi_slv_rx_tx_init(priv_slv, AX_RX_LEN, NULL, (void *)input_buf);
	dw_spi_s_irq_setup(priv_slv);
	ret = request_irq(dws_slv->irq, dw_spi_s_irq, IRQF_SHARED, "ax_spi_s", priv_slv);
	if (ret < 0) {
		dev_err(&pdev->dev, "can not get IRQ\n");
		goto err_gpio;
	}
	dev_err(&pdev->dev, "probe successful\n");
	return 0;

err_gpio:
	gpio_free(priv_slv->stream_rx_gpio);
err_gpio_tx:
	gpio_free(priv_slv->stream_tx_gpio);
free_prst:
	reset_control_assert(dws_slv->prstc);
free_rst:
	reset_control_assert(dws_slv->rstc);
free_clk:
	clk_disable_unprepare(dws_slv->clk);
free_hclk:
	clk_disable_unprepare(dws_slv->hclk);
err_free2:
	devm_kfree(&pdev->dev, priv_slv->dws_slv);
err_free1:
	devm_kfree(&pdev->dev, priv_slv);
	return ret;
}

static int dw_spi_slv_remove(struct platform_device *pdev)
{
	struct dw_poll_priv *priv_slv = dev_get_drvdata(&pdev->dev);
	struct dw_spi_slv *dws_slv = priv_slv->dws_slv;
	free_irq(dws_slv->irq, &dws_slv);
	unregister_spi_slv_dev(pdev);
	gpio_free(priv_slv->stream_tx_gpio);
	gpio_free(priv_slv->stream_rx_gpio);
	reset_control_assert(dws_slv->rstc);
	reset_control_assert(dws_slv->prstc);
	clk_disable_unprepare(dws_slv->clk);
	clk_disable_unprepare(dws_slv->hclk);
	devm_kfree(&pdev->dev, dws_slv);
	devm_kfree(&pdev->dev, priv_slv);
	return 0;
}

/*
 * This disables the SPI controller, interrupts, clears the interrupts status
 * and CS, then re-enables the controller back. Transmit and receive FIFO
 * buffers are cleared when the device is disabled.
 */

static inline void spi_slv_reset_chip(struct dw_spi_slv *dws)
{
	spi_slv_enable_chip(dws, 0);
	spi_s_mask_intr(dws, 0xff);
	dw_slv_read(dws, DW_SPI_ICR);
	dw_slv_write(dws, DW_SPI_SER, 0);
	spi_slv_enable_chip(dws, 1);
}

static inline void spi_slv_shutdown_chip(struct dw_spi_slv *dws)
{
	spi_slv_enable_chip(dws, 0);
}

#ifdef CONFIG_PM_SLEEP
extern unsigned int __clk_get_enable_count(struct clk *clk);
extern bool __clk_is_enabled(struct clk *clk);
int dw_spi_slv_suspend(struct device *dev)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct dw_poll_priv *priv_slv = dev_get_drvdata(&pdev->dev);
	struct dw_spi_slv *dws_slv = priv_slv->dws_slv;

	pr_debug("%s, %d, %d,  hclk:%d, clk:%d\n",__func__, __clk_get_enable_count(dws_slv->hclk), __clk_get_enable_count(dws_slv->clk), __clk_is_enabled(dws_slv->hclk), __clk_is_enabled(dws_slv->clk));
	if (__clk_is_enabled(dws_slv->hclk))
		clk_disable_unprepare(dws_slv->hclk);
	if (__clk_is_enabled(dws_slv->clk))
		clk_disable_unprepare(dws_slv->clk);

	spi_slv_shutdown_chip(dws_slv);
	return 0;
}

int dw_spi_slv_resume(struct device *dev)
{
	int ret;
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct dw_poll_priv *priv_slv = dev_get_drvdata(&pdev->dev);
	struct dw_spi_slv *dws_slv = priv_slv->dws_slv;

	pr_debug("%s, %d, %d,  hclk:%d, clk:%d\n",__func__, __clk_get_enable_count(dws_slv->hclk), __clk_get_enable_count(dws_slv->clk), __clk_is_enabled(dws_slv->hclk), __clk_is_enabled(dws_slv->clk));
	if (!__clk_is_enabled(dws_slv->hclk)) {
		ret = clk_prepare_enable(dws_slv->hclk);
		if (ret)
			pr_err("%s spi_slv set hclk failed\n", __func__);
	}
	if (!__clk_is_enabled(dws_slv->clk)) {
		ret = clk_prepare_enable(dws_slv->clk);
		if (ret)
			pr_err("%s spi_slv set clk failed\n", __func__);
	}
	spi_slv_reset_chip(dws_slv);
	spi_slv_hw_init(dws_slv);
	spi_slv_rx_tx_init(priv_slv, AX_RX_LEN, NULL, (void *)input_buf);
	dw_spi_s_irq_setup(priv_slv);
	return 0;
}
#endif

static const struct dev_pm_ops dw_spi_slv_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(dw_spi_slv_suspend, dw_spi_slv_resume)
};

static const struct of_device_id dw_spi_slv_of_match[] = {
	{.compatible = "snps,dwc-ssi-slv-1.03a",},
	{ /* end of table */ }
};

MODULE_DEVICE_TABLE(of, dw_spi_slv_of_match);

static struct platform_driver dw_spi_slv_driver = {
	.probe = dw_spi_slv_probe,
	.remove = dw_spi_slv_remove,
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = dw_spi_slv_of_match,
		.pm	= &dw_spi_slv_pm_ops,
	},
};

module_platform_driver(dw_spi_slv_driver);

MODULE_AUTHOR("Wu Qianlong <wuqianlong@axera-tech.com>");
MODULE_DESCRIPTION("Memory-mapped I/O interface driver for DW SPI Core");
MODULE_LICENSE("GPL v2");
