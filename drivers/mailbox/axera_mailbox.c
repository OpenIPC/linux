// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2023-2024 Axera

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mailbox_controller.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/ax_mailbox.h>
#include "mailbox.h"

#define MBOX_TX			0x1
#define NUM_CHANS		32
#define MBOX_MSG_LEN		8
#define MAILBOX_MAX_DATA_SIZE (8*4)

#define INT_STATS		(0x300 & (~(0xf << 4))) //int status reg
#define INT_CLR			(0x304 & (~(0xf << 4)))  //int clr reg
#define REG_STATUS_SEARCH_REG	(0x308 & (~(0xf << 4))) //channel search reg
#define REG_UNLOCK_REG		(0x30C & (~(0xf << 4))) //channel unlock reg

/*NORMAL_INT_MASK：Normal transmission is interrupted, when the correct information is written, an interrupt is sent to the slave specified in the information
ERR_SLV_INT_MASK：The slave number written in information is greater than MST_NUM
RD_EMPTY_DATA_INT_MAST:When the amount of data read from the slot is greater than the amount of data written to the slot,
an interrupt is sent to the slave indicated by the information in the current slot
WR_FULL_DATA_INT_MASK:When the data written to the slot is greater than 8, an interrupt is sent to the master that locks the current slot*/
#define NORMAL_INT_MASK        1
#define ERR_SLV_INT_MASK       2
#define RD_EMPTY_DATA_INT_MAST 4
#define WR_FULL_DATA_INT_MASK  8

#define INFOR_REG	0x100 //master reg
#define FLASH_SYS_BASE	0x10030000
static void *ax_pri_data;
static mbox_callback_t *g_notify;
static void __iomem *mb_base;

/**
 * axera mailbox channel information
 *
 * A channel can be used for TX or RX, it can trigger remote
 * processor interrupt to notify remote processor and can receive
 * interrupt if has incoming message.
 *
 * @send_id:	as a sender
 * @receive_id:	as a receiver
 */
struct ax_chan_info {
	unsigned int send_id;
	unsigned int receive_id;
};

/**
 * axera mailbox controller data
 *
 * Mailbox controller includes 32 channels and can allocate
 * channel for message transferring.
 *
 * @dev:	Device to which it is attached
 * @base:	Base address of the register mapping region
 * @chan:	Representation of channels in mailbox controller
 * @mchan:	Representation of channel info
 * @controller:	Representation of a communication channel controller
 */
struct ax_mbox {
	struct device *dev;
	void __iomem *base;
	struct mbox_chan chan[NUM_CHANS];
	struct ax_chan_info mchan[NUM_CHANS];
	struct mbox_controller controller;
	int irq;
};

void ax_mailbox_set_callback(mbox_callback_t *callback, void *pri_data)
{
	g_notify = callback;
	ax_pri_data = pri_data;
}
EXPORT_SYMBOL(ax_mailbox_set_callback);

static struct ax_mbox *to_ax_mbox(struct mbox_controller *mbox)
{
	return container_of(mbox, struct ax_mbox, controller);
}

static int ax_mbox_startup(struct mbox_chan *chan)
{
	if(chan == NULL)
		return -ETIMEDOUT;
	chan->txdone_method = TXDONE_BY_IRQ;

	return 0;
}

static irqreturn_t ax_mbox_irq(int irq, void *dev_id)
{
	return IRQ_WAKE_THREAD;
}

irqreturn_t ax_threaded_function(int irq, void *dev_id)
{
	unsigned int int_sts;
	u32 i;
	u32 msg[MBOX_MSG_LEN];
	mbox_msg_t *re_message;
	/* Only examine channels that are currently enabled. */
	int_sts = readl(mb_base + (INT_STATS | (AX_MAILBOX_MASTERID_ARM0 << 4)));
	if ((int_sts & 0xf) != 1) {
		printk(KERN_ERR "%s Int Error 0x%x\n", __func__, int_sts);
		return IRQ_HANDLED;
	}

	writel(int_sts & 0xf, mb_base + (INT_CLR | (AX_MAILBOX_MASTERID_ARM0 << 4))); //..0xe

	readl(mb_base + INFOR_REG + (((int_sts >> 16) & 0x1f) << 2));
	for (i = 0; i < MBOX_MSG_LEN; i++){
		msg[i] = readl(mb_base + (((int_sts >> 16) & 0x1f) << 2));
	}
	re_message = (mbox_msg_t *)msg;
	/*mbox_chan_received_data(mbox->chan, (void *)msg);*/
	if (g_notify)
		g_notify(re_message, ax_pri_data);
	return IRQ_HANDLED;
}

static int ax_mbox_send_data(struct mbox_chan *chan, void *msg)
{
	unsigned long ch = (unsigned long)chan->con_priv;
	struct ax_mbox *mbox = to_ax_mbox(chan->mbox);
	struct ax_chan_info *mchan = &mbox->mchan[ch];

	u32 *buf = msg;
	unsigned int i;
	u32 infor_data;

	for (i = 0; i < MBOX_MSG_LEN; i++){
		writel_relaxed(buf[i], ((mbox->base) + (ch << 2)));
	}
	infor_data  = (mchan->receive_id << 28) | (mchan->send_id << 24) | (ch << 16) | (i * 4);
	writel_relaxed(infor_data, (mbox->base) + INFOR_REG + (ch << 2));

	return 0;
}

static const struct mbox_chan_ops ax_mbox_ops = {
	.startup	= ax_mbox_startup,
	.send_data	= ax_mbox_send_data,
};

int ax_mailbox_send_message(unsigned int send_masterid, unsigned int receive_masterid, mbox_msg_t *data)
{
	u32 ch, i;
	u32 *buf = (u32 *)data;
	u32 infor_data;
	ch = readl(mb_base + (REG_STATUS_SEARCH_REG | (send_masterid << 4)));
	if (ch == 0xffffffff)
		printk("REQUEST CHANNEL FAILED\n");
	for (i = 0; i < MBOX_MSG_LEN; i++){
		writel_relaxed(buf[i], (mb_base + (ch << 2)));
	}
	infor_data  = (receive_masterid << 28) | (send_masterid << 24) | (ch << 16) | (i * 4);
	writel_relaxed(infor_data, mb_base + INFOR_REG + (ch << 2));
	return 0;
}
EXPORT_SYMBOL(ax_mailbox_send_message);
static struct mbox_chan *ax_mbox_xlate(struct mbox_controller *controller,
					   const struct of_phandle_args *spec)
{
	struct ax_mbox *mbox = to_ax_mbox(controller);

	struct mbox_chan *chan = mbox->chan;
	int send_id, receive_id;
	unsigned long ch;

	send_id = spec->args[0];
	receive_id = spec->args[1];

	ch = readl((mbox->base) + (REG_STATUS_SEARCH_REG | (send_id << 4)));
	if (ch == 0xffffffff) {
		return NULL;
	} else {
		chan = &chan[ch];
		mbox->mchan[ch].send_id	= send_id;
		mbox->mchan[ch].receive_id = receive_id;
		return &mbox->chan[ch];
	}
}
static void ax_mailbox_enable(void)
{
	void __iomem *base;

	base = ioremap(FLASH_SYS_BASE, 0x10000);
	writel(1 << 15, base + 0x4008);
	writel(1 << 13, base + 0x8014);
	iounmap(base);
}

static void ax_mailbox_disable(void)
{
	void __iomem *base;
	base = ioremap(FLASH_SYS_BASE, 0x10000);
	writel(1 << 13, base + 0x4014);
	writel(1 << 15, base + 0x8008);
	iounmap(base);
}
static int ax_mbox_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ax_mbox *mbox;
	struct mbox_chan *chan;
	struct resource *res;
	unsigned long ch;
	int err, ret;

	mbox = devm_kzalloc(dev, sizeof(*mbox), GFP_KERNEL);
	if (!mbox)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mbox->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(mbox->base))
		return PTR_ERR(mbox->base);
	mbox->dev = dev;
	mb_base = mbox->base;
	mbox->irq = platform_get_irq(pdev, 0);
	if (mbox->irq < 0) {
		dev_err(dev, "ax620e_mailbox_probe error\n");
		return mbox->irq;
	}

	ax_mailbox_enable();

	mbox->dev = dev;
	mbox->controller.dev = dev;
	mbox->controller.chans = mbox->chan;
	mbox->controller.num_chans = NUM_CHANS;
	mbox->controller.ops = &ax_mbox_ops;
	mbox->controller.of_xlate = ax_mbox_xlate;
	mbox->controller.txdone_irq = true;
	/* Initialize mailbox channel data */
	chan = mbox->chan;
	for (ch = 0; ch < NUM_CHANS; ch++)
		chan[ch].con_priv = (void *)ch;

 	ret = request_threaded_irq(mbox->irq, ax_mbox_irq, ax_threaded_function,
					IRQF_ONESHOT, dev_name(dev), dev);
	if (ret) {
		dev_err(dev, "failed to request inbox IRQ: %d\n", ret);
		return ret;
	}

	err = mbox_controller_register(&mbox->controller);
	if (err) {
		dev_err(dev, "Failed to register mailbox %d\n", err);
		return err;
	}

	platform_set_drvdata(pdev, mbox);
	dev_set_drvdata(&pdev->dev, mbox);
	printk("Mailbox enabled\n");
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int ax_mailbox_suspend(struct device *dev)
{
	ax_mailbox_disable();
	return 0;
}

static int ax_mailbox_resume(struct device *dev)
{
	ax_mailbox_enable();

	return 0;
}
#endif
static SIMPLE_DEV_PM_OPS(ax_mailbox_pm_ops, ax_mailbox_suspend, ax_mailbox_resume);
static int ax_mbox_remove(struct platform_device *pdev)
{
	struct ax_mbox *mbox = platform_get_drvdata(pdev);

	mbox_controller_unregister(&mbox->controller);
	/* See the comment in ax_mbox_probe about the reset line. */
	ax_mailbox_disable();

	return 0;
}

static const struct of_device_id ax_mbox_of_match[] = {
	{ .compatible = "axera,mailbox", },
	{},
};
MODULE_DEVICE_TABLE(of, ax_mbox_of_match);

static struct platform_driver ax_mbox_driver = {
	.driver = {
		.name = "axera-mbox",
		.pm = &ax_mailbox_pm_ops,
		.of_match_table = ax_mbox_of_match,
	},
	.probe  = ax_mbox_probe,
	.remove = ax_mbox_remove,
};
static int __init ax620e_mbox_init(void)
{
	return platform_driver_register(&ax_mbox_driver);
}
core_initcall(ax620e_mbox_init);

static void __exit ax620e_mbox_exit(void)
{
	platform_driver_unregister(&ax_mbox_driver);
}
module_exit(ax620e_mbox_exit);
MODULE_AUTHOR("Axera");
MODULE_DESCRIPTION("Axera Message Box");
MODULE_LICENSE("GPL v2");
