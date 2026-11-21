// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Hisilicon Reset Controller Driver
 *
 * Copyright (c) 2015-2016 HiSilicon Technologies Co., Ltd.
 */

#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include "reset.h"

#define	HISI_RESET_BIT_MASK	0x1f
#define	HISI_RESET_OFFSET_SHIFT	8
#define	HISI_RESET_OFFSET_MASK	0xffff00

struct hisi_reset_controller {
	spinlock_t	own_lock;	/* used when the caller passes no lock */
	spinlock_t	*lock;		/* protects the reset register updates */
	void __iomem	*membase;
	const struct hisi_reset_bits *bits;	/* valid resets, NULL: any */
	size_t		num_bits;
	struct reset_controller_dev	rcdev;
};


#define to_hisi_reset_controller(rcdev)  \
	container_of(rcdev, struct hisi_reset_controller, rcdev)

/*
 * A controller with a table of valid resets only accepts a specifier that
 * names one of them exactly: the cells are compared before they are masked
 * into the reset ID, so an out of range value cannot alias a valid one.
 */
static bool hisi_reset_valid(const struct hisi_reset_controller *rstc,
			     u32 offset, u32 bit)
{
	size_t i;

	if (!rstc->bits)
		return true;

	for (i = 0; i < rstc->num_bits; i++)
		if (rstc->bits[i].offset == offset)
			return bit < 32 && (rstc->bits[i].mask & BIT(bit));

	return false;
}

static int hisi_reset_of_xlate(struct reset_controller_dev *rcdev,
			const struct of_phandle_args *reset_spec)
{
	struct hisi_reset_controller *rstc = to_hisi_reset_controller(rcdev);
	u32 offset;
	u8 bit;

	if (!hisi_reset_valid(rstc, reset_spec->args[0], reset_spec->args[1]))
		return -EINVAL;

	offset = (reset_spec->args[0] << HISI_RESET_OFFSET_SHIFT)
		& HISI_RESET_OFFSET_MASK;
	bit = reset_spec->args[1] & HISI_RESET_BIT_MASK;

	return (offset | bit);
}

static int hisi_reset_assert(struct reset_controller_dev *rcdev,
			      unsigned long id)
{
	struct hisi_reset_controller *rstc = to_hisi_reset_controller(rcdev);
	unsigned long flags;
	u32 offset, reg;
	u8 bit;

	offset = (id & HISI_RESET_OFFSET_MASK) >> HISI_RESET_OFFSET_SHIFT;
	bit = id & HISI_RESET_BIT_MASK;

	spin_lock_irqsave(rstc->lock, flags);

	reg = readl(rstc->membase + offset);
	writel(reg | BIT(bit), rstc->membase + offset);

	spin_unlock_irqrestore(rstc->lock, flags);

	return 0;
}

static int hisi_reset_deassert(struct reset_controller_dev *rcdev,
				unsigned long id)
{
	struct hisi_reset_controller *rstc = to_hisi_reset_controller(rcdev);
	unsigned long flags;
	u32 offset, reg;
	u8 bit;

	offset = (id & HISI_RESET_OFFSET_MASK) >> HISI_RESET_OFFSET_SHIFT;
	bit = id & HISI_RESET_BIT_MASK;

	spin_lock_irqsave(rstc->lock, flags);

	reg = readl(rstc->membase + offset);
	writel(reg & ~BIT(bit), rstc->membase + offset);

	spin_unlock_irqrestore(rstc->lock, flags);

	return 0;
}

static const struct reset_control_ops hisi_reset_ops = {
	.assert		= hisi_reset_assert,
	.deassert	= hisi_reset_deassert,
};

/**
 * hisi_reset_init_checked - register a reset controller that validates resets
 * @pdev: platform device of the CRG
 * @lock: lock that also protects the other users of the same registers (the
 *	  gate and mux clocks), or NULL to use a private one
 * @bits: the resets that exist, one entry per register, or NULL to accept any
 *	  register offset and bit like hisi_reset_init_shared()
 * @num_bits: number of entries in @bits
 *
 * Reset bits share registers with clock gates, so both must update them under
 * the same lock. With @bits, a reset specifier whose register offset or bit is
 * not listed is rejected with -EINVAL when the consumer looks it up, so a
 * device tree cannot make the controller write a register or a bit that is
 * not a reset, or lies outside the register block.
 *
 * Return: the controller, or an ERR_PTR() with the reason of the failure.
 */
struct hisi_reset_controller *hisi_reset_init_checked(struct platform_device *pdev,
						      spinlock_t *lock,
						      const struct hisi_reset_bits *bits,
						      size_t num_bits)
{
	struct hisi_reset_controller *rstc;
	struct resource *res;
	size_t i;
	int ret;

	rstc = devm_kzalloc(&pdev->dev, sizeof(*rstc), GFP_KERNEL);
	if (!rstc)
		return ERR_PTR(-ENOMEM);

	rstc->membase = devm_platform_get_and_ioremap_resource(pdev, 0, &res);
	if (IS_ERR(rstc->membase))
		return ERR_CAST(rstc->membase);

	for (i = 0; bits && i < num_bits; i++) {
		if ((u64)bits[i].offset + sizeof(u32) > resource_size(res)) {
			dev_err(&pdev->dev, "reset register %#x is outside the %pR block\n",
				bits[i].offset, res);
			return ERR_PTR(-EINVAL);
		}
	}

	if (!lock) {
		spin_lock_init(&rstc->own_lock);
		lock = &rstc->own_lock;
	}
	rstc->lock = lock;
	rstc->bits = bits;
	rstc->num_bits = bits ? num_bits : 0;
	rstc->rcdev.owner = THIS_MODULE;
	rstc->rcdev.ops = &hisi_reset_ops;
	rstc->rcdev.of_node = pdev->dev.of_node;
	rstc->rcdev.of_reset_n_cells = 2;
	rstc->rcdev.of_xlate = hisi_reset_of_xlate;
	ret = reset_controller_register(&rstc->rcdev);
	if (ret) {
		dev_err(&pdev->dev, "failed to register the reset controller: %d\n", ret);
		return ERR_PTR(ret);
	}

	return rstc;
}
EXPORT_SYMBOL_GPL(hisi_reset_init_checked);

/**
 * hisi_reset_init_shared - register a reset controller on the CRG of @pdev
 * @pdev: platform device of the CRG
 * @lock: lock that also protects the other users of the same registers (the
 *	  gate and mux clocks), or NULL to use a private one
 *
 * Accepts any register offset and bit in a reset specifier, see
 * hisi_reset_init_checked() for a controller that validates them.
 *
 * Return: the controller, or an ERR_PTR() with the reason of the failure.
 */
struct hisi_reset_controller *hisi_reset_init_shared(struct platform_device *pdev,
						     spinlock_t *lock)
{
	return hisi_reset_init_checked(pdev, lock, NULL, 0);
}
EXPORT_SYMBOL_GPL(hisi_reset_init_shared);

struct hisi_reset_controller *hisi_reset_init(struct platform_device *pdev)
{
	struct hisi_reset_controller *rstc = hisi_reset_init_shared(pdev, NULL);

	return IS_ERR(rstc) ? NULL : rstc;
}
EXPORT_SYMBOL_GPL(hisi_reset_init);

void hisi_reset_exit(struct hisi_reset_controller *rstc)
{
	reset_controller_unregister(&rstc->rcdev);
}
EXPORT_SYMBOL_GPL(hisi_reset_exit);

#ifdef CONFIG_ARCH_HISI_BVT
#include "reset.h"

int __init hibvt_reset_init(struct device_node *np, int nr_rsts)
{
	struct hisi_reset_controller *rstc;

	rstc = kzalloc(sizeof(*rstc), GFP_KERNEL);
	if (!rstc)
		return -ENOMEM;

	rstc->rcdev.owner = THIS_MODULE;
	rstc->rcdev.ops = &hisi_reset_ops;
	rstc->rcdev.of_node = np;
	rstc->rcdev.of_reset_n_cells = 2;
	rstc->rcdev.of_xlate = hisi_reset_of_xlate;
	rstc->rcdev.nr_resets = nr_rsts;

	rstc->membase = of_iomap(np, 0);
	if (!rstc->membase) {
		kfree(rstc);
		return -EINVAL;
	}

	spin_lock_init(&rstc->own_lock);
	rstc->lock = &rstc->own_lock;

	return reset_controller_register(&rstc->rcdev);
}
EXPORT_SYMBOL_GPL(hibvt_reset_init);
#endif
