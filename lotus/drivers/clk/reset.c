// SPDX-License-Identifier: GPL-2.0-only

#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include "reset.h"

#define	LOTUS_RESET_BIT_MASK	0x1f
#define	LOTUS_RESET_OFFSET_SHIFT	8
#define	LOTUS_RESET_OFFSET_MASK	0xffff00

struct lotus_reset_controller {
	spinlock_t	lock;
	void __iomem	*membase;
	struct reset_controller_dev	rcdev;
};


#define to_lotus_reset_controller(rcdev)  \
	container_of(rcdev, struct lotus_reset_controller, rcdev)

static int lotus_reset_of_xlate(struct reset_controller_dev *rcdev,
			const struct of_phandle_args *reset_spec)
{
	u32 offset;
	u8 bit;

	offset = (reset_spec->args[0] << LOTUS_RESET_OFFSET_SHIFT)
		& LOTUS_RESET_OFFSET_MASK;
	bit = reset_spec->args[1] & LOTUS_RESET_BIT_MASK;

	return (offset | bit);
}

static int lotus_reset_assert(struct reset_controller_dev *rcdev,
			      unsigned long id)
{
	struct lotus_reset_controller *rstc = to_lotus_reset_controller(rcdev);
	unsigned long flags;
	u32 offset, reg;
	u8 bit;

	offset = (id & LOTUS_RESET_OFFSET_MASK) >> LOTUS_RESET_OFFSET_SHIFT;
	bit = id & LOTUS_RESET_BIT_MASK;

	spin_lock_irqsave(&rstc->lock, flags);

	reg = readl(rstc->membase + offset);
	writel(reg | BIT(bit), rstc->membase + offset);

	spin_unlock_irqrestore(&rstc->lock, flags);

	return 0;
}

static int lotus_reset_deassert(struct reset_controller_dev *rcdev,
				unsigned long id)
{
	struct lotus_reset_controller *rstc = to_lotus_reset_controller(rcdev);
	unsigned long flags;
	u32 offset, reg;
	u8 bit;

	offset = (id & LOTUS_RESET_OFFSET_MASK) >> LOTUS_RESET_OFFSET_SHIFT;
	bit = id & LOTUS_RESET_BIT_MASK;

	spin_lock_irqsave(&rstc->lock, flags);

	reg = readl(rstc->membase + offset);
	writel(reg & ~BIT(bit), rstc->membase + offset);

	spin_unlock_irqrestore(&rstc->lock, flags);

	return 0;
}

static const struct reset_control_ops lotus_reset_ops = {
	.assert		= lotus_reset_assert,
	.deassert	= lotus_reset_deassert,
};

int __init lotus_reset_init(struct device_node *np,
		int nr_rsts)
{
	struct lotus_reset_controller *rstc;

	rstc = kzalloc(sizeof(*rstc), GFP_KERNEL);
	if (!rstc)
		return -ENOMEM;

	rstc->membase = of_iomap(np, 0);
	if (!rstc->membase) {
		kfree(rstc);
		return -EINVAL;
	}

	spin_lock_init(&rstc->lock);

	rstc->rcdev.owner = THIS_MODULE;
	rstc->rcdev.nr_resets = nr_rsts;
	rstc->rcdev.ops = &lotus_reset_ops;
	rstc->rcdev.of_node = np;
	rstc->rcdev.of_reset_n_cells = 2;
	rstc->rcdev.of_xlate = lotus_reset_of_xlate;

	return reset_controller_register(&rstc->rcdev);
}
