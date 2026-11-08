// SPDX-License-Identifier: GPL-2.0
/*
 * Fullhan FH8626V100 ARC coprocessor remoteproc driver
 *
 * The ARC runs an ELF image from a reserved region of system memory and
 * shares virtio rings and buffers with the ARM through a second reserved
 * region. The coprocessor is held in reset with its clock gated whenever no
 * firmware runs.
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/remoteproc.h>
#include <linux/reset.h>
#include <linux/soc/fullhan/fh8626-pmu.h>

#include "remoteproc_internal.h"

#define FH8626_ARC_FIRMWARE		"fh8626-arc.elf"

/* PMU registers */
#define FH8626_PMU_DB_TO_ARC		0x1e4
#define FH8626_PMU_DB_TO_ARM		0x1f0
#define FH8626_PMU_ARC_BOOT		0x2000
#define FH8626_PMU_ARC_CTRL		0x2010
#define FH8626_ARC_CTRL_RUN		BIT(4)

/* Doorbell bits set by the ARC in FH8626_PMU_DB_TO_ARM */
#define FH8626_ARC_DB_VRING		BIT(0)
#define FH8626_ARC_DB_CRASH		BIT(1)
#define FH8626_ARC_DB_MASK	(FH8626_ARC_DB_VRING | FH8626_ARC_DB_CRASH)

/*
 * Out of reset the ARC executes four words at FH8626_PMU_ARC_BOOT. Point the
 * interrupt vector base at the firmware entry and jump to it:
 *
 *	sr	entry, [INT_VECTOR_BASE]	0x266b7940, entry
 *	j	entry				0x20200f80, entry
 *
 * ARCompact stores 32-bit instructions and long immediates as two halfwords,
 * the most significant first, so each word is written rotated by 16 bits.
 */
#define FH8626_ARC_INSN_SR_VECBASE	0x266b7940
#define FH8626_ARC_INSN_J		0x20200f80

struct fh8626_arc {
	struct rproc *rproc;
	struct clk *clk;
	struct reset_control *reset;
	void *mem;
	phys_addr_t mem_base;
	size_t mem_size;
	struct rproc_subdev vrings;
	bool vrings_added;
	struct mutex lock;	/* guards the core, running and vrings_up */
	bool running;
	bool vrings_up;
};

static void fh8626_arc_isolate(struct fh8626_arc *arc)
{
	if (!arc->running)
		return;

	reset_control_assert(arc->reset);
	fh8626_pmu_update_bits(FH8626_PMU_ARC_CTRL, FH8626_ARC_CTRL_RUN, 0);
	clk_disable_unprepare(arc->clk);
	arc->running = false;
}

static void *fh8626_arc_da_to_va(struct rproc *rproc, u64 da, int len)
{
	struct fh8626_arc *arc = rproc->priv;

	if (len <= 0 || len > arc->mem_size || da < arc->mem_base ||
	    da - arc->mem_base > arc->mem_size - len)
		return NULL;

	return arc->mem + (da - arc->mem_base);
}

/* Deliver vring notifications; the caller holds arc->lock */
static void fh8626_arc_notify_vrings(struct fh8626_arc *arc)
{
	struct rproc *rproc = arc->rproc;
	int id;

	if (!arc->vrings_up)
		return;

	for (id = 0; id <= rproc->max_notifyid; id++)
		rproc_vq_interrupt(rproc, id);
}

/*
 * Vring notifications are delivered only while this subdevice runs. It is
 * added after the virtio devices on each start, so it stops before they do
 * and no notification reaches a virtqueue that is being deleted.
 */
static int fh8626_arc_vrings_start(struct rproc_subdev *subdev)
{
	struct fh8626_arc *arc = container_of(subdev, struct fh8626_arc, vrings);

	mutex_lock(&arc->lock);
	arc->vrings_up = true;
	/* Pick up anything the ARC posted while the virtqueues were set up */
	fh8626_arc_notify_vrings(arc);
	mutex_unlock(&arc->lock);

	return 0;
}

static void fh8626_arc_vrings_stop(struct rproc_subdev *subdev, bool crashed)
{
	struct fh8626_arc *arc = container_of(subdev, struct fh8626_arc, vrings);

	mutex_lock(&arc->lock);
	arc->vrings_up = false;
	mutex_unlock(&arc->lock);
}

static int fh8626_arc_start(struct rproc *rproc)
{
	struct fh8626_arc *arc = rproc->priv;
	u32 entry = rproc->bootaddr;
	const u32 boot[] = {
		ror32(FH8626_ARC_INSN_SR_VECBASE, 16), ror32(entry, 16),
		ror32(FH8626_ARC_INSN_J, 16), ror32(entry, 16),
	};
	int i, ret;

	if (!fh8626_arc_da_to_va(rproc, entry, sizeof(u32)))
		return -EINVAL;

	mutex_lock(&arc->lock);

	for (i = 0; i < ARRAY_SIZE(boot); i++) {
		ret = fh8626_pmu_write(FH8626_PMU_ARC_BOOT + 4 * i, boot[i]);
		if (ret)
			goto unlock;
	}

	/* Drop doorbells left over from a previous run */
	fh8626_pmu_write(FH8626_PMU_DB_TO_ARC, 0);
	fh8626_pmu_update_bits(FH8626_PMU_DB_TO_ARM, FH8626_ARC_DB_MASK, 0);

	ret = clk_prepare_enable(arc->clk);
	if (ret)
		goto unlock;

	ret = reset_control_deassert(arc->reset);
	if (ret)
		goto disable_clk;

	/* Let the core come out of reset before it is allowed to run */
	usleep_range(100, 200);

	ret = fh8626_pmu_update_bits(FH8626_PMU_ARC_CTRL, FH8626_ARC_CTRL_RUN,
				     FH8626_ARC_CTRL_RUN);
	if (ret)
		goto assert_reset;

	arc->running = true;
	mutex_unlock(&arc->lock);

	/* The core calls start and stop under its lock */
	rproc_add_subdev(rproc, &arc->vrings);
	arc->vrings_added = true;

	return 0;

assert_reset:
	reset_control_assert(arc->reset);
disable_clk:
	clk_disable_unprepare(arc->clk);
unlock:
	mutex_unlock(&arc->lock);
	return ret;
}

static int fh8626_arc_stop(struct rproc *rproc)
{
	struct fh8626_arc *arc = rproc->priv;

	mutex_lock(&arc->lock);
	fh8626_arc_isolate(arc);
	mutex_unlock(&arc->lock);

	if (arc->vrings_added) {
		rproc_remove_subdev(rproc, &arc->vrings);
		arc->vrings_added = false;
	}

	return 0;
}

static void fh8626_arc_kick(struct rproc *rproc, int vqid)
{
	fh8626_pmu_write(FH8626_PMU_DB_TO_ARC, 1);
}

static const struct rproc_ops fh8626_arc_ops = {
	.start = fh8626_arc_start,
	.stop = fh8626_arc_stop,
	.kick = fh8626_arc_kick,
	.da_to_va = fh8626_arc_da_to_va,
};

static irqreturn_t fh8626_arc_irq(int irq, void *data)
{
	struct fh8626_arc *arc = data;
	struct rproc *rproc = arc->rproc;
	u32 pending;

	if (fh8626_pmu_read(FH8626_PMU_DB_TO_ARM, &pending))
		return IRQ_NONE;

	pending &= FH8626_ARC_DB_MASK;
	if (!pending)
		return IRQ_NONE;

	/*
	 * The ARC sets its bits with a read-modify-write of this plain
	 * register, so clearing can drop a bit it sets meanwhile. The vrings
	 * are scanned only after the clear, which then sees whatever the ARC
	 * posted before a dropped vring bit. Firmware keeps the crash bit set
	 * until it is stopped, setting it again if it was cleared.
	 */
	fh8626_pmu_update_bits(FH8626_PMU_DB_TO_ARM, pending, 0);
	mb();

	mutex_lock(&arc->lock);
	if (pending & FH8626_ARC_DB_CRASH) {
		if (arc->running) {
			fh8626_arc_isolate(arc);
			rproc_report_crash(rproc, RPROC_FATAL_ERROR);
		}
	} else {
		fh8626_arc_notify_vrings(arc);
	}
	mutex_unlock(&arc->lock);

	return IRQ_HANDLED;
}

static void fh8626_arc_free(void *data)
{
	rproc_free(data);
}

static void fh8626_arc_release_dma(void *data)
{
	of_reserved_mem_device_release(data);
}

static int fh8626_arc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct reserved_mem *rmem;
	struct device_node *np;
	struct fh8626_arc *arc;
	struct rproc *rproc;
	int irq, ret;

	rproc = rproc_alloc(dev, dev_name(dev), &fh8626_arc_ops,
			    FH8626_ARC_FIRMWARE, sizeof(*arc));
	if (!rproc)
		return -ENOMEM;

	ret = devm_add_action_or_reset(dev, fh8626_arc_free, rproc);
	if (ret)
		return ret;

	rproc->auto_boot = false;
	arc = rproc->priv;
	arc->rproc = rproc;
	arc->vrings.start = fh8626_arc_vrings_start;
	arc->vrings.stop = fh8626_arc_vrings_stop;
	mutex_init(&arc->lock);
	platform_set_drvdata(pdev, rproc);

	arc->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(arc->clk))
		return PTR_ERR(arc->clk);

	arc->reset = devm_reset_control_get_exclusive(dev, NULL);
	if (IS_ERR(arc->reset))
		return PTR_ERR(arc->reset);

	/*
	 * Isolate the ARC until firmware is started. The clock is cycled
	 * because it may have been left ungated without a reference.
	 */
	ret = reset_control_assert(arc->reset);
	if (ret)
		return ret;

	ret = fh8626_pmu_update_bits(FH8626_PMU_ARC_CTRL,
				     FH8626_ARC_CTRL_RUN, 0);
	if (ret)
		return ret;

	ret = clk_prepare_enable(arc->clk);
	if (ret)
		return ret;
	clk_disable_unprepare(arc->clk);

	np = of_parse_phandle(dev->of_node, "memory-region", 0);
	if (!np)
		return -EINVAL;
	rmem = of_reserved_mem_lookup(np);
	of_node_put(np);
	if (!rmem)
		return -EINVAL;

	arc->mem_base = rmem->base;
	arc->mem_size = rmem->size;
	arc->mem = devm_memremap(dev, rmem->base, rmem->size, MEMREMAP_WC);
	if (IS_ERR(arc->mem))
		return PTR_ERR(arc->mem);

	/* Virtio rings and buffers come from the second region */
	ret = of_reserved_mem_device_init_by_idx(dev, dev->of_node, 1);
	if (ret)
		return ret;

	ret = devm_add_action_or_reset(dev, fh8626_arc_release_dma, dev);
	if (ret)
		return ret;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	ret = devm_request_threaded_irq(dev, irq, NULL, fh8626_arc_irq,
					IRQF_ONESHOT, dev_name(dev), arc);
	if (ret)
		return ret;

	return rproc_add(rproc);
}

static int fh8626_arc_remove(struct platform_device *pdev)
{
	struct rproc *rproc = platform_get_drvdata(pdev);
	struct fh8626_arc *arc = rproc->priv;
	int power;

	/* A crash must not restart the firmware from here on */
	rproc->recovery_disabled = true;

	/*
	 * rproc_del() only shuts down processors booted by rproc_add(). Drop
	 * every boot reference, including the one a failed recovery leaves.
	 */
	while ((power = atomic_read(&rproc->power)) > 0) {
		rproc_shutdown(rproc);
		if (atomic_read(&rproc->power) == power)
			break;
	}

	rproc_del(rproc);

	/* A start racing with the shutdown above must not outlive the driver */
	mutex_lock(&arc->lock);
	fh8626_arc_isolate(arc);
	mutex_unlock(&arc->lock);

	/* An isolated core reports no crash; finish any reported before */
	cancel_work_sync(&rproc->crash_handler);

	return 0;
}

static const struct of_device_id fh8626_arc_of_match[] = {
	{ .compatible = "fullhan,fh8626v100-arc" },
	{ }
};
MODULE_DEVICE_TABLE(of, fh8626_arc_of_match);

static struct platform_driver fh8626_arc_driver = {
	.probe = fh8626_arc_probe,
	.remove = fh8626_arc_remove,
	.driver = {
		.name = "fh8626-arc",
		.of_match_table = fh8626_arc_of_match,
	},
};
module_platform_driver(fh8626_arc_driver);

MODULE_DESCRIPTION("Fullhan FH8626V100 ARC remoteproc driver");
MODULE_LICENSE("GPL v2");
