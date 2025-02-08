/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include "reset.h"

#define	XMEDIA_RESET_BIT_MASK	0x1f
#define	XMEDIA_RESET_OFFSET_SHIFT	8
#define	XMEDIA_RESET_OFFSET_MASK	0xffff00

struct xmedia_reset_controller {
	spinlock_t	lock;
	void __iomem	*membase;
	struct reset_controller_dev	rcdev;
};


#define to_xmedia_reset_controller(rcdev)  \
	container_of(rcdev, struct xmedia_reset_controller, rcdev)

static int xmedia_reset_of_xlate(struct reset_controller_dev *rcdev,
			const struct of_phandle_args *reset_spec)
{
	u32 offset;
	u8 bit;

	offset = (reset_spec->args[0] << XMEDIA_RESET_OFFSET_SHIFT)
		& XMEDIA_RESET_OFFSET_MASK;
	bit = reset_spec->args[1] & XMEDIA_RESET_BIT_MASK;

	return (offset | bit);
}

static int xmedia_reset_assert(struct reset_controller_dev *rcdev,
			      unsigned long id)
{
	struct xmedia_reset_controller *rstc = to_xmedia_reset_controller(rcdev);
	unsigned long flags;
	u32 offset, reg;
	u8 bit;

	offset = (id & XMEDIA_RESET_OFFSET_MASK) >> XMEDIA_RESET_OFFSET_SHIFT;
	bit = id & XMEDIA_RESET_BIT_MASK;

	spin_lock_irqsave(&rstc->lock, flags);

	reg = readl(rstc->membase + offset);
	writel(reg | BIT(bit), rstc->membase + offset);

	spin_unlock_irqrestore(&rstc->lock, flags);

	return 0;
}

static int xmedia_reset_deassert(struct reset_controller_dev *rcdev,
				unsigned long id)
{
	struct xmedia_reset_controller *rstc = to_xmedia_reset_controller(rcdev);
	unsigned long flags;
	u32 offset, reg;
	u8 bit;

	offset = (id & XMEDIA_RESET_OFFSET_MASK) >> XMEDIA_RESET_OFFSET_SHIFT;
	bit = id & XMEDIA_RESET_BIT_MASK;

	spin_lock_irqsave(&rstc->lock, flags);

	reg = readl(rstc->membase + offset);
	writel(reg & ~BIT(bit), rstc->membase + offset);

	spin_unlock_irqrestore(&rstc->lock, flags);

	return 0;
}

static const struct reset_control_ops xmedia_reset_ops = {
	.assert		= xmedia_reset_assert,
	.deassert	= xmedia_reset_deassert,
};

int __init xmedia_reset_init(struct device_node *np,
		int nr_rsts)
{
	struct xmedia_reset_controller *rstc;

	rstc = kzalloc(sizeof(*rstc), GFP_KERNEL);
	if (!rstc)
		return -ENOMEM;

	rstc->membase = of_iomap(np, 0);
	if (!rstc->membase){
		kfree(rstc);
		return -EINVAL;
	}

	spin_lock_init(&rstc->lock);

	rstc->rcdev.owner = THIS_MODULE;
	rstc->rcdev.nr_resets = nr_rsts;
	rstc->rcdev.ops = &xmedia_reset_ops;
	rstc->rcdev.of_node = np;
	rstc->rcdev.of_reset_n_cells = 2;
	rstc->rcdev.of_xlate = xmedia_reset_of_xlate;

	return reset_controller_register(&rstc->rcdev);
}
EXPORT_SYMBOL_GPL(xmedia_reset_init);
