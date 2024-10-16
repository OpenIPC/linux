/*
 * Copyright (c) 2015, allwinnertech Communications R329.
 * Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/bitops.h>
#include <linux/rpmsg.h>
#include <linux/errno.h>
#include <linux/irqchip/chained_irq.h>
#include "rpmsg_internal.h"

#define MSGBOX_CTRL_REG0_OFFSET	      0
#define MSGBOX_CTRL_REG1_OFFSET	      0x0004
#define MSGBOX_VER_REG_OFFSET	      0x0010
#define MSGBOX_IRQ_EN_REG_OFFSET      0x0040
#define MSGBOX_IRQ_STATUS_REG_OFFSET  0x0050
#define MSGBOX_FIFO_STATUS_REG_OFFSET 0x0100
#define MSGBOX_MSG_STATUS_REG_OFFSET  0x0140
#define MSGBOX_MSG_REG_OFFSET	      0x0180
#define MSGBOX_DEBUG_REG_OFFSET       0x01c0

/*
 * there is 8 channel, but we use it dual channel
 * so there's only 4 channel for 4 * 2
 * 0, 2, 4, 6 is trans
 * 1, 3, 5, 7 is recv
 */
#define SUNXI_MSGBOX_MAX_EDP 4

#define TO_SUNXI_MSGBOX_ENDPOINT(x) \
	container_of(x, struct sunxi_msgbox_endpoint, edp)

#define TO_SUNXI_MSGBOX_DEVICE(x) \
	container_of(x, struct sunxi_msgbox_device, rpmsg_dev)

/*
 * sunxi_msgbox_endpoint - warp struct of rpmsg_endpoint
 * mtx_send: send mutex used to only on send
 * send_done: infor send finish
 * len: send length
 * idx: current should send
 * d: send buf
 */
struct sunxi_msgbox_endpoint {
	struct rpmsg_endpoint edp;
	struct mutex mtx_send;
	struct completion send_done;
	int len;
	int idx;
	uint8_t *d;
};

struct sunxi_msgbox {
	void __iomem *base;
	struct irq_domain *d;
	struct irq_chip_generic *gc;
	int irq;
	int rpdev_cnt;
};

/*
 * sunxi_msgbox_device - warp rpmsg_device of sunxi
 * edp_bitmap: used edp
 * base: base address of this messagebox
 * irq: irq descriptor of this messagebox
 * sunxi_edp: edp of this messagebox
 *
 * this rpmsg_dev has 8 msg_queue and group into 4 group
 * every group has 2 msg_queue.
 * so we have 4 rpmsg_endpoint every messagebox.
 * every rpmsg_endpoint use two msg_queue.
 * the little is for send the other is for rev.
 */
struct sunxi_msgbox_device {
	struct rpmsg_device rpmsg_dev;
	DECLARE_BITMAP(edp_bitmap, SUNXI_MSGBOX_MAX_EDP);
	void __iomem *base;
	uint32_t suspend_irq;
	int irq_send;
	int irq_rev;

	/* managed endpoint */
	struct sunxi_msgbox_endpoint sunxi_edp;
};

/*
 * sunxi_msgbox_start_send - this function start interrupt of send
 */
static int sunxi_msgbox_start_send(struct sunxi_msgbox_endpoint *medp)
{
	struct sunxi_msgbox_device *mdev =
		TO_SUNXI_MSGBOX_DEVICE(medp->edp.rpdev);

	/* enable interrupt */
	enable_irq(mdev->irq_send);

	return 0;
}

/*
 * sunxi_msgbox_stop_send - this function disable interrupt of send
 */
static int sunxi_msgbox_stop_send(struct sunxi_msgbox_endpoint *medp)
{
	struct sunxi_msgbox_device *mdev =
		TO_SUNXI_MSGBOX_DEVICE(medp->edp.rpdev);

	/* disable interrupt */
	disable_irq_nosync(mdev->irq_send);

	return 0;
}

/*
 * sunxi_msgbox_send - function of rpmsg_endpoint's callback
 */
static int sunxi_msgbox_send(struct rpmsg_endpoint *ept, void *data, int len)
{
	struct sunxi_msgbox_endpoint *medp = TO_SUNXI_MSGBOX_ENDPOINT(ept);
	unsigned long ret;

	dev_dbg(&ept->rpdev->dev, "send msg start\n");

	mutex_lock(&medp->mtx_send);

	medp->d = data;
	medp->len = len;
	medp->idx = 0;

	reinit_completion(&medp->send_done);
	sunxi_msgbox_start_send(medp);
	ret = wait_for_completion_timeout(&medp->send_done,
					  msecs_to_jiffies(15000));
	if (!ret) {
		dev_err(&ept->rpdev->dev, "send msg timeout\n");
		sunxi_msgbox_stop_send(medp);
		mutex_unlock(&medp->mtx_send);
		return -ERESTARTSYS;
	}
	mutex_unlock(&medp->mtx_send);

	return 0;
}

/*
 * sunxi_msgbox_destory_edp - destory rpmsg_endpoint
 */
static void sunxi_msgbox_destory_edp(struct rpmsg_endpoint *ept)
{
	struct sunxi_msgbox_device *mdev = TO_SUNXI_MSGBOX_DEVICE(ept->rpdev);

	clear_bit(ept->addr, mdev->edp_bitmap);

	if (ept->cb)
		ept->cb = NULL;
	if (ept->priv)
		ept->priv = NULL;
}

static struct rpmsg_endpoint_ops sunxi_msgbox_ep_ops = {
	.destroy_ept = sunxi_msgbox_destory_edp,
	.send = sunxi_msgbox_send,
};

/*
 * sunxi_msgbox_direction_set - set rpmsg_endpoint direction
 *
 * channel is double, so on channel use two msgbox_mq;
 * channel N used msgbox_mq is N * 2 and N * 2 + 1;
 * N * 2 is trans of cpu, and N * 2 + 1 is rec of cpu.
 */
static int sunxi_msgbox_direction_set(struct sunxi_msgbox_endpoint *medp)
{
	struct sunxi_msgbox_device *mdev =
		TO_SUNXI_MSGBOX_DEVICE(medp->edp.rpdev);
	int channel = medp->edp.addr;
	uint32_t *regs = mdev->base + (channel / 2 * 4);
	uint32_t data;
	uint32_t shift = (channel % 2) * 16;

	data = readl(regs);
	data &= ~(0xffff << shift);
	data |= (0x1001 << shift);
	writel(data, regs);

	return 0;
}

/*
 * sunxi_create_ept - used to create rpmsg_endpoint
 * rpdev: the rpmsg_device used to create rpmsg_endpoint
 * cb: callback of this create rpmsg_endpoint
 * priv: the cb add priv data
 * chinfo: used to create rpmsg_endpoint information
 *
 * we use chinfo's src address only
 */
static struct rpmsg_endpoint *sunxi_create_ept(struct rpmsg_device *rpdev,
					rpmsg_rx_cb_t cb, void *priv,
					struct rpmsg_channel_info chinfo)
{
	struct sunxi_msgbox_endpoint *sunxi_edp;
	struct sunxi_msgbox_device *mdev = TO_SUNXI_MSGBOX_DEVICE(rpdev);

	/* check is over max endpoint */
	if (chinfo.src >= SUNXI_MSGBOX_MAX_EDP) {
		dev_err(&mdev->rpmsg_dev.dev, "error channel src addr\n");
		return NULL;
	}

	sunxi_edp = &mdev->sunxi_edp;
	if (!sunxi_edp)
		return NULL;

	/* check it is ept is used */
	if (test_bit(chinfo.src, mdev->edp_bitmap)) {
		/* dev_err(&mdev->rpmsg_dev.dev, "channel is used\n"); */
		sunxi_edp->edp.cb = cb;
		sunxi_edp->edp.priv = priv;
		return &sunxi_edp->edp;
	}

	if (chinfo.src != rpdev->src) {
		dev_err(&mdev->rpmsg_dev.dev, "no channel with this device\n");
		return NULL;
	}

	sunxi_edp->edp.addr = chinfo.src;
	sunxi_edp->edp.rpdev = &mdev->rpmsg_dev;
	sunxi_edp->edp.cb = cb;
	sunxi_edp->edp.priv = priv;
	sunxi_edp->edp.ops = &sunxi_msgbox_ep_ops;

	kref_init(&sunxi_edp->edp.refcount);
	set_bit(chinfo.src, mdev->edp_bitmap);
	mutex_init(&sunxi_edp->mtx_send);
	init_completion(&sunxi_edp->send_done);

	sunxi_msgbox_direction_set(sunxi_edp);

	return &sunxi_edp->edp;
}

static struct rpmsg_device_ops sunxi_msgbox_dev_ops = {
	.create_ept = sunxi_create_ept,
};

/*
 * sunxi_msgbox_rec_int - receive interrupt handler
 */
static int sunxi_msgbox_rec_int(struct sunxi_msgbox_endpoint *sunxi_edp,
				int irq)
{
	struct sunxi_msgbox_device *mdev =
		container_of(sunxi_edp, struct sunxi_msgbox_device, sunxi_edp);
	int offset = (sunxi_edp->edp.addr * 2 + 1) * 4;
	uint32_t data;
	uint8_t d[4];
	void *reg_fifo = mdev->base + MSGBOX_MSG_REG_OFFSET + offset;
	void *reg_num = mdev->base + MSGBOX_MSG_STATUS_REG_OFFSET + offset;

	while (readl(reg_num)) {
		data = readl(reg_fifo);
		d[0] = data & 0xff;
		d[1] = (data >> 8) & 0xff;
		d[2] = (data >> 16) & 0xff;
		d[3] = (data >> 24) & 0xff;

		if (sunxi_edp->edp.cb) {
			sunxi_edp->edp.cb(&mdev->rpmsg_dev, d, 4,
					  sunxi_edp->edp.priv,
					  sunxi_edp->edp.addr);
		}
	}

	return 0;
}

/*
 * sunxi_msgbox_send_int - send interrupt handler
 */
static int sunxi_msgbox_send_int(struct sunxi_msgbox_endpoint *sunxi_edp,
				 int irq)
{
	struct sunxi_msgbox_device *mdev =
		container_of(sunxi_edp, struct sunxi_msgbox_device, sunxi_edp);
	int offset = (sunxi_edp->edp.addr * 2) * 4;
	uint32_t data;
	uint8_t d;
	int i;
	void *reg_full = mdev->base + MSGBOX_FIFO_STATUS_REG_OFFSET + offset;
	void *reg_fifo = mdev->base + MSGBOX_MSG_REG_OFFSET + offset;

	while (!readl(reg_full)) {
		if (sunxi_edp->idx >= sunxi_edp->len) {
			/* disable interrupt */
			disable_irq_nosync(mdev->irq_send);

			complete(&sunxi_edp->send_done);

			break;
		}

		data = 0;
		for (i = 0; i < sizeof(uint32_t); i++) {
			if (sunxi_edp->idx < sunxi_edp->len) {
				d = sunxi_edp->d[sunxi_edp->idx++];
				data |= d << (i * 8);
			}
		}
		writel(data, reg_fifo);
	}

	return 0;
}

static irqreturn_t sunxi_msgbox_irq_send(int irq, void *data)
{
	struct sunxi_msgbox_device *msgbox = data;

	/* send irq will enable it requested */
	if (msgbox->sunxi_edp.d)
		sunxi_msgbox_send_int(&msgbox->sunxi_edp, irq);
	else
		disable_irq_nosync(irq);

	return IRQ_HANDLED;
}

static irqreturn_t sunxi_msgbox_irq_rev(int irq, void *data)
{
	struct sunxi_msgbox_device *msgbox = data;

	sunxi_msgbox_rec_int(&msgbox->sunxi_edp, irq);

	return IRQ_HANDLED;
}

static void sunxi_msgbox_handler(struct irq_desc *desc)
{
	struct irq_domain *domain = irq_desc_get_handler_data(desc);
	struct irq_chip *chip = irq_desc_get_chip(desc);
	void __iomem *pbase = irq_get_domain_generic_chip(domain, 0)->reg_base;
	unsigned long r, r1;
	int set, virq;

	chained_irq_enter(chip, desc);
	r = readl(pbase + MSGBOX_IRQ_EN_REG_OFFSET);
	r1 = readl(pbase + MSGBOX_IRQ_STATUS_REG_OFFSET);
	r = r & r1;
	for_each_set_bit(set, &r, 32) {
		virq = irq_find_mapping(domain, set);
		if (virq)
			generic_handle_irq(virq);
	}
	chained_irq_exit(chip, desc);
}

/*
 * XXX: channel cnt shount from dts
 */
static struct sunxi_msgbox *sunxi_msgbox_alloc(struct platform_device *pdev)
{
	struct sunxi_msgbox *b;
	struct resource *res;
	unsigned int clr = IRQ_NOREQUEST | IRQ_NOPROBE | IRQ_NOAUTOEN;
	int ret;
	unsigned long addr;

	b = devm_kzalloc(&pdev->dev, sizeof(struct sunxi_msgbox), GFP_KERNEL);
	if (!b)
		return ERR_PTR(-ENOMEM);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "no memory resource\n");
		return ERR_PTR(-ENXIO);
	}
	b->base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!b->base) {
		dev_err(&pdev->dev, "not io map\n");
		return ERR_PTR(-ENXIO);
	}

	b->irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
	if (!b->irq) {
		dev_err(&pdev->dev, "no irq defined\n");
		return ERR_PTR(-ENXIO);
	}

	b->d = irq_domain_add_linear(pdev->dev.of_node, 16,
				     &irq_generic_chip_ops, NULL);
	if (!b->d) {
		dev_err(&pdev->dev, "can not create irq doamin\n");
		return ERR_PTR(-ENOMEM);
	}

	ret = irq_alloc_domain_generic_chips(b->d, 16, 1, KBUILD_MODNAME,
					     handle_fasteoi_irq, clr, 0,
					     IRQ_GC_INIT_MASK_CACHE);
	if (ret < 0) {
		dev_err(&pdev->dev, "can not alloc gc");
		goto no_gc;
	}

	b->gc = irq_get_domain_generic_chip(b->d, 0);

	b->gc->chip_types[0].type = IRQ_TYPE_LEVEL_MASK;
	b->gc->chip_types[0].chip.irq_mask = irq_gc_mask_clr_bit;
	b->gc->chip_types[0].chip.irq_unmask = irq_gc_mask_set_bit;
	b->gc->chip_types[0].chip.irq_eoi = irq_gc_ack_set_bit;
	b->gc->chip_types[0].chip.flags = IRQCHIP_EOI_THREADED |
					  IRQCHIP_EOI_IF_HANDLED |
					  IRQCHIP_SKIP_SET_WAKE;

	b->gc->reg_base = b->base;
	b->gc->chip_types[0].regs.mask = MSGBOX_IRQ_EN_REG_OFFSET;
	b->gc->chip_types[0].regs.ack = MSGBOX_IRQ_STATUS_REG_OFFSET;

	addr = (unsigned long)b->base + MSGBOX_IRQ_EN_REG_OFFSET;
	writel(0, (void *)addr);

	addr = (unsigned long)b->base + MSGBOX_IRQ_STATUS_REG_OFFSET;
	writel(0xffffffff, (void *)addr);

	irq_set_chained_handler_and_data(b->irq, sunxi_msgbox_handler, b->d);

	platform_set_drvdata(pdev, b);
	return b;
no_gc:
	irq_domain_remove(b->d);

	return ERR_PTR(ret);
}

static int sunxi_msgbox_rpdev_create(struct platform_device *pdev,
				     const char *rpmsg_id)
{
	struct sunxi_msgbox *b = platform_get_drvdata(pdev);
	struct sunxi_msgbox_device *msgbox;
	int ret;

	msgbox = devm_kzalloc(&pdev->dev, sizeof(struct sunxi_msgbox_device),
			      GFP_KERNEL);

	msgbox->base = b->base;

	msgbox->rpmsg_dev.dev.parent = &pdev->dev;
	msgbox->rpmsg_dev.dst = RPMSG_ADDR_ANY;
	msgbox->rpmsg_dev.src = b->rpdev_cnt++;

	strncpy(msgbox->rpmsg_dev.id.name, rpmsg_id, RPMSG_NAME_SIZE - 1);

	msgbox->irq_send = irq_create_mapping(b->d, msgbox->rpmsg_dev.src * 4 + 1);
	if (!msgbox->irq_send) {
		dev_err(&pdev->dev, "can not crate mapping\n");
		devm_kfree(&pdev->dev, msgbox);
		return -ENXIO;
	}

	msgbox->irq_rev = irq_create_mapping(b->d, msgbox->rpmsg_dev.src * 4 + 2);
	if (!msgbox->irq_rev) {
		dev_err(&pdev->dev, "can not crate mapping\n");
		devm_kfree(&pdev->dev, msgbox);
		return -ENXIO;
	}

	msgbox->rpmsg_dev.ops = &sunxi_msgbox_dev_ops;

	ret = devm_request_irq(&pdev->dev, msgbox->irq_send,
			       sunxi_msgbox_irq_send, 0, "sunxi-msgbox_send",
			       msgbox);
	if (ret < 0) {
		dev_err(&pdev->dev, "error get irq\n");
		return ret;
	}

	ret = devm_request_irq(&pdev->dev, msgbox->irq_rev,
			       sunxi_msgbox_irq_rev, 0, "sunxi-msgbox_rev",
			       msgbox);
	if (ret < 0) {
		dev_err(&pdev->dev, "error get irq\n");
		return ret;
	}

	ret = rpmsg_register_device(&msgbox->rpmsg_dev);
	if (ret)
		return ret;


	return 0;
}

static int sunxi_msgbox_probe(struct platform_device *pdev)
{
	struct sunxi_msgbox *b;
	const char *s;
	struct property *prop;

	b = sunxi_msgbox_alloc(pdev);
	if (IS_ERR(b))
		return PTR_ERR(b);

	of_property_for_each_string (pdev->dev.of_node, "rpmsg_id", prop, s) {
		sunxi_msgbox_rpdev_create(pdev, s);
	}

	return 0;
}

static int sunxi_msgbox_remove(struct platform_device *pdev)
{
	return 0;
}

static int msgbox_suspend(struct device *dev)
{
	return 0;
}

static int msgbox_resume(struct device *dev)
{
	return 0;
}

static SIMPLE_DEV_PM_OPS(sx_msg_pm, msgbox_suspend, msgbox_resume);
#ifdef CONFIG_PM_SLEEP
#define SX_MSGBOX_PM (&sx_msg_pm)
#else
#define SX_MSGBOX_PM NULL
#endif /* CONFIG_PM_SLEEP */

static const struct of_device_id sunxi_msgbox_table[] = {
	{ .compatible = "sunxi,msgbox" },
	{}
};

static struct platform_driver sunxi_msgbox_driver = {
	.probe = sunxi_msgbox_probe,
	.remove = sunxi_msgbox_remove,
	.driver = {
		.name = "sunxi-msgbox",
		.of_match_table = sunxi_msgbox_table,
		.pm = SX_MSGBOX_PM,
	},
};

static int __init sunxi_msgbox_init(void)
{
	return platform_driver_register(&sunxi_msgbox_driver);
}
subsys_initcall(sunxi_msgbox_init);

static void __exit sunxi_msgbox_exit(void)
{
	platform_driver_unregister(&sunxi_msgbox_driver);
}
module_exit(sunxi_msgbox_exit);

MODULE_AUTHOR("fuyao <fuyao@allwinnertech.com>");
MODULE_DESCRIPTION("msgbox communication channel");
MODULE_LICENSE("GPL v2");
