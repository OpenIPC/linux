/**
 * drivers/uio/uio_video_codec.c
 *
 * Userspace I/O driver for anyka soc video hardware codec.
 * Based on uio_pdrv.c by Uwe Kleine-Koenig,
 *
 * Jacky Lau
 * 2011-07-05
 *
 * Copyright (C) 2011 by Anyka Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * AKUIO_SYSREG_WRITE / AKUIO_WAIT_IRQ / AKUIO_INVALIDATE_L2CACHE /
 * AKUIO_INVALIDATE_L1CACHE / AKUIO_UNBLOCK_CHANNEL / AKUIO_VIDEO_RESET
 * (include/linux/akuio_driver.h) are ABI-critical for the userspace
 * encoder libraries.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/uio_driver.h>
#include <linux/wait.h>
#include <asm/cacheflush.h>
#include <asm/uaccess.h>

#include <linux/akuio_driver.h>
#include <mach/map.h>

#define DRIVER_NAME "uio_vcodec"

#define init_MUTEX(sem)		sema_init(sem, 1)
#define init_MUTEX_LOCKED(sem)	sema_init(sem, 0)

/*
 * Two encoder cores sit behind this node, and the userspace library of each
 * expects a different contract from AKUIO_WAIT_IRQ.
 *
 * EV200 (Hantro 8290): the ioctl returns once the line has fired. The handler
 * masks it and the library reads and clears the core's own status.
 *
 * EV300 (Allegro, the AnyCloud39EV300 SDK's libakv_encode): the kernel reads
 * and acknowledges the core's interrupt register itself and hands the library
 * one interrupt number per call, in the order they fired, through the int the
 * ioctl points at. That is the SDK kernel's al5r_hardirq_handler/wait_irq
 * (drivers/uio/uio_video_codec.c of the 3.4.35 SDK), with two differences:
 * the wait can be interrupted by a signal, and a fixed ring replaces the
 * per-interrupt kmem_cache allocation in hard IRQ context.
 */
enum uio_vcodec_type {
	UIO_VCODEC_HANTRO,
	UIO_VCODEC_AL5,
};

/* Allegro interrupt mask and status, from the encoder register base */
#define AL5_INTERRUPT_MASK	0x8014
#define AL5_INTERRUPT		0x8018
#define AL5_INTERRUPT_NB	20

/*
 * The EV300 SDK pulses bit 20 of SYSCTRL 0x20 (MODULE_RESET_CON1,
 * AK39_SRESET_VIDEO) for 5 ms: arch/arm/mach-ak39/reset.c.
 */
#define AK_SYSCTRL_RESET_CON1	0x20
#define AK_SRESET_VIDEO_BIT	BIT(20)

#define AL5_IRQ_RING		64

/* platform data of this driver */
struct uio_platdata {
	struct uio_info uioinfo;
	struct clk *clk;
	int irq;
	enum uio_vcodec_type type;
	void __iomem *regs;
	/* open_count and the IRQ registration it guards: UIO calls open and
	 * release without serializing them */
	struct mutex open_lock;
	unsigned int open_count;

	/* EV200 */
	struct semaphore vcodec_sem;

	/* EV300 */
	wait_queue_head_t irq_queue;
	spinlock_t irq_lock;
	u8 irq_ring[AL5_IRQ_RING];
	unsigned int irq_head, irq_tail;
	unsigned int irq_dropped;
	bool unblock;
};

static DEFINE_SPINLOCK(sys_ctrl_reg_lock);

/*
 * Anyka system control register read-modify-write, bounded to the
 * SYSCTRL block.
 */
static void sys_ctrl_reg_set(unsigned long reg_phy_addr, unsigned long reg_mask,
			      unsigned long reg_val)
{
	unsigned long flags;
	unsigned long val;
	void __iomem *reg_virt_addr;

	if (reg_phy_addr < AK_PA_SYSCTRL || reg_phy_addr > (AK_PA_SYSCTRL + AK_SZ_SYSCTRL))
		return;

	spin_lock_irqsave(&sys_ctrl_reg_lock, flags);

	reg_virt_addr = (void __iomem *)((unsigned long)AK_VA_SYSCTRL + reg_phy_addr - AK_PA_SYSCTRL);
	val = __raw_readl(reg_virt_addr);
	val = (val & ~reg_mask) | (reg_val & reg_mask);
	__raw_writel(val, reg_virt_addr);

	spin_unlock_irqrestore(&sys_ctrl_reg_lock, flags);
}

static void al5_video_reset(void)
{
	unsigned long pa = AK_PA_SYSCTRL + AK_SYSCTRL_RESET_CON1;

	sys_ctrl_reg_set(pa, AK_SRESET_VIDEO_BIT, AK_SRESET_VIDEO_BIT);
	mdelay(5);
	sys_ctrl_reg_set(pa, AK_SRESET_VIDEO_BIT, 0);
}

static irqreturn_t uio_vcodec_irq_handler(int irq, void *dev_id)
{
	struct uio_platdata *pdata = dev_id;

	disable_irq_nosync(irq);
	up(&pdata->vcodec_sem);

	return IRQ_HANDLED;
}

static irqreturn_t al5_irq_handler(int irq, void *dev_id)
{
	struct uio_platdata *pdata = dev_id;
	u32 pending, fired;
	unsigned long flags;
	int i;

	pending = readl(pdata->regs + AL5_INTERRUPT);
	fired = pending & readl(pdata->regs + AL5_INTERRUPT_MASK);
	if (!fired)
		return IRQ_NONE;

	/* write-one-to-clear, then read back so the write lands first */
	writel(pending, pdata->regs + AL5_INTERRUPT);
	readl(pdata->regs + AL5_INTERRUPT);

	spin_lock_irqsave(&pdata->irq_lock, flags);
	for (i = 0; i < AL5_INTERRUPT_NB; i++) {
		if (!(fired & BIT(i)))
			continue;
		if (pdata->irq_head - pdata->irq_tail >= AL5_IRQ_RING) {
			pdata->irq_dropped++;
			continue;
		}
		pdata->irq_ring[pdata->irq_head++ % AL5_IRQ_RING] = i;
	}
	spin_unlock_irqrestore(&pdata->irq_lock, flags);

	wake_up_interruptible(&pdata->irq_queue);

	return IRQ_HANDLED;
}

static bool al5_ready(struct uio_platdata *pdata)
{
	unsigned long flags;
	bool ready;

	spin_lock_irqsave(&pdata->irq_lock, flags);
	ready = pdata->unblock || pdata->irq_head != pdata->irq_tail;
	spin_unlock_irqrestore(&pdata->irq_lock, flags);

	return ready;
}

static int al5_wait_irq(struct uio_platdata *pdata, unsigned long arg)
{
	unsigned long flags;
	unsigned int dropped;
	int nb, ret;

	ret = wait_event_interruptible(pdata->irq_queue, al5_ready(pdata));
	if (ret)
		return ret;

	spin_lock_irqsave(&pdata->irq_lock, flags);
	/*
	 * An unblock wins over a queued interrupt: the client is stopping, and
	 * every waiter returns -EINTR until the device is opened afresh.
	 */
	if (pdata->unblock || pdata->irq_head == pdata->irq_tail) {
		spin_unlock_irqrestore(&pdata->irq_lock, flags);
		return -EINTR;
	}
	nb = pdata->irq_ring[pdata->irq_tail++ % AL5_IRQ_RING];
	dropped = pdata->irq_dropped;
	pdata->irq_dropped = 0;
	spin_unlock_irqrestore(&pdata->irq_lock, flags);

	if (dropped)
		pr_warn_ratelimited(DRIVER_NAME ": %u encoder interrupts lost, the queue was full\n",
				    dropped);

	if (copy_to_user((void __user *)arg, &nb, sizeof(nb)))
		return -EFAULT;

	return 0;
}

static int uio_vcodec_ioctl(struct uio_info *uioinfo, unsigned int cmd, unsigned long arg)
{
	struct uio_platdata *pdata = uioinfo->priv;
	unsigned long flags;
	int err;

	switch (cmd) {
	case AKUIO_SYSREG_WRITE:
	{
		struct akuio_sysreg_write_t reg_write;

		if (copy_from_user(&reg_write, (void __user *)arg, sizeof(reg_write)))
			return -EFAULT;

		sys_ctrl_reg_set(reg_write.paddr, reg_write.mask, reg_write.val);

		err = 0;
	}
	break;

	case AKUIO_WAIT_IRQ:
		if (pdata->type == UIO_VCODEC_AL5) {
			err = al5_wait_irq(pdata, arg);
			break;
		}
		enable_irq(pdata->irq);
		down(&pdata->vcodec_sem);
		err = 0;
		break;

	case AKUIO_UNBLOCK_CHANNEL:
		if (pdata->type != UIO_VCODEC_AL5)
			return -EINVAL;
		spin_lock_irqsave(&pdata->irq_lock, flags);
		pdata->unblock = true;
		spin_unlock_irqrestore(&pdata->irq_lock, flags);
		wake_up_interruptible(&pdata->irq_queue);
		err = 0;
		break;

	case AKUIO_VIDEO_RESET:
		if (pdata->type != UIO_VCODEC_AL5)
			return -EINVAL;
		al5_video_reset();
		err = 0;
		break;

	case AKUIO_INVALIDATE_L2CACHE:
		flush_cache_all();
		err = 0;
		break;

	case AKUIO_INVALIDATE_L1CACHE:
		err = 0;
		break;

	default:
		err = -EINVAL;
		break;
	}

	return err;
}

static int uio_vcodec_open(struct uio_info *uioinfo, struct inode *inode)
{
	struct uio_platdata *pdata = uioinfo->priv;
	unsigned long flags;
	int ret = 0;

	mutex_lock(&pdata->open_lock);
	if (pdata->open_count++ > 0)
		goto out;

	if (pdata->type == UIO_VCODEC_AL5) {
		spin_lock_irqsave(&pdata->irq_lock, flags);
		pdata->irq_head = pdata->irq_tail = 0;
		pdata->irq_dropped = 0;
		pdata->unblock = false;
		spin_unlock_irqrestore(&pdata->irq_lock, flags);
		/* as the SDK does on open: the core starts from reset */
		al5_video_reset();
		if (request_irq(pdata->irq, al5_irq_handler, 0, "VIDEO HW CODEC", pdata)) {
			pdata->open_count--;
			ret = -EBUSY;
		}
		goto out;
	}

	init_MUTEX_LOCKED(&pdata->vcodec_sem);

	if (request_irq(pdata->irq, uio_vcodec_irq_handler, 0, "VIDEO HW CODEC", pdata)) {
		pdata->open_count--;
		ret = -EBUSY;
		goto out;
	}
	disable_irq_nosync(pdata->irq);

out:
	mutex_unlock(&pdata->open_lock);
	return ret;
}

static int uio_vcodec_release(struct uio_info *uioinfo, struct inode *inode)
{
	struct uio_platdata *pdata = uioinfo->priv;

	mutex_lock(&pdata->open_lock);
	if (--pdata->open_count == 0) {
		free_irq(pdata->irq, pdata);
		/*
		 * The last user is gone, and so may be the memory the core reads
		 * and writes. A picture abandoned with AKUIO_UNBLOCK_CHANNEL is
		 * still running on the core, so reset it here, not only on the
		 * next open.
		 */
		if (pdata->type == UIO_VCODEC_AL5)
			al5_video_reset();
	}
	mutex_unlock(&pdata->open_lock);

	return 0;
}

static const struct of_device_id uio_vcodec_of_match[];

static int uio_vcodec_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;
	struct uio_platdata *pdata;
	struct resource *res;
	void __iomem *base;
	int ret;

	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	match = of_match_device(uio_vcodec_of_match, &pdev->dev);
	pdata->type = match ? (enum uio_vcodec_type)match->data : UIO_VCODEC_HANTRO;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base))
		return PTR_ERR(base);
	pdata->regs = base;

	if (pdata->type == UIO_VCODEC_AL5 &&
	    resource_size(res) < AL5_INTERRUPT + sizeof(u32)) {
		dev_err(&pdev->dev, "register window %pR does not reach the interrupt status at +0x%x\n",
			res, AL5_INTERRUPT);
		return -EINVAL;
	}

	pdata->irq = platform_get_irq(pdev, 0);
	if (pdata->irq < 0)
		return pdata->irq;

	mutex_init(&pdata->open_lock);
	init_waitqueue_head(&pdata->irq_queue);
	spin_lock_init(&pdata->irq_lock);

	pdata->clk = devm_clk_get(&pdev->dev, NULL);
	if (!IS_ERR(pdata->clk))
		clk_prepare_enable(pdata->clk);

	pdata->uioinfo.name = "video_codec";
	pdata->uioinfo.version = "0.1.0";
	pdata->uioinfo.irq = UIO_IRQ_CUSTOM;
	pdata->uioinfo.mem[0].memtype = UIO_MEM_PHYS;
	pdata->uioinfo.mem[0].addr = res->start;
	pdata->uioinfo.mem[0].size = resource_size(res);
	pdata->uioinfo.open = uio_vcodec_open;
	pdata->uioinfo.release = uio_vcodec_release;
	pdata->uioinfo.ioctl = uio_vcodec_ioctl;
	pdata->uioinfo.priv = pdata;

	pdata->open_count = 0;

	ret = uio_register_device(&pdev->dev, &pdata->uioinfo);
	if (ret) {
		if (!IS_ERR(pdata->clk))
			clk_disable_unprepare(pdata->clk);
		return ret;
	}

	platform_set_drvdata(pdev, pdata);

	dev_info(&pdev->dev, "%s encoder at %pR, irq %d\n",
		 pdata->type == UIO_VCODEC_AL5 ? "Allegro" : "Hantro",
		 res, pdata->irq);

	return 0;
}

static int uio_vcodec_remove(struct platform_device *pdev)
{
	struct uio_platdata *pdata = platform_get_drvdata(pdev);

	uio_unregister_device(&pdata->uioinfo);
	if (!IS_ERR(pdata->clk))
		clk_disable_unprepare(pdata->clk);

	return 0;
}

static const struct of_device_id uio_vcodec_of_match[] = {
	{ .compatible = "anyka,ak39ev330-uio-vencoder",
	  .data = (void *)UIO_VCODEC_HANTRO },
	{ .compatible = "anyka,ak3918ev300-uio-vencoder",
	  .data = (void *)UIO_VCODEC_AL5 },
	{ }
};
MODULE_DEVICE_TABLE(of, uio_vcodec_of_match);

static struct platform_driver uio_vcodec = {
	.probe	= uio_vcodec_probe,
	.remove	= uio_vcodec_remove,
	.driver	= {
		.name		= DRIVER_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= uio_vcodec_of_match,
	},
};
module_platform_driver(uio_vcodec);

MODULE_AUTHOR("Jacky Lau");
MODULE_DESCRIPTION("Userspace driver for anyka video hw codec");
MODULE_LICENSE("GPL v2");
