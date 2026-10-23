// SPDX-License-Identifier: GPL-2.0
/*
 * AX630C platform glue for the VeriSilicon VC8000E VCMD command-engine driver.
 *
 * The eswin EIC7X copy of the VeriSilicon VCMD driver (eswin/vc8000_vcmd_driver.c,
 * dual MIT/GPL) is split into a portable command-engine core and a
 * platform-specific probe layer (eswin's vc8000e_driver.c). That eswin probe
 * layer is bound to the EIC7700 device tree, its SMMU dynamic-SID scheme, and
 * its clk/reset/TBU/pm-runtime plumbing, none of which exist on the Axera
 * AX630C. This file is the AX630C replacement for that probe layer: it defines
 * the handful of symbols the VCMD core imports from the platform layer and
 * stands up the char-device instance.
 *
 * This is open-source driver-replacement work: it re-implements the AX630C's own
 * VCMD kernel driver from the public VeriSilicon lineage so the device can run a
 * blob-free encode-submission path in place of the vendor ax_venc.ko.
 *
 * WHAT THE MAINLINE PORT CHANGED (#83). On 4.19 this was a module_init that
 * called platform_device_register_simple() to invent a device, hand-set its
 * DMA masks (#63: a bare platform device gets arm64's dummy_dma_ops, so
 * dma_coerce_mask_and_coherent() FAILS silently), declared a coherent carveout
 * from module parameters, and found its clock and reset by walking the device
 * tree for a compatible string. All of that is gone. It is an ordinary
 * DT-bound platform driver now:
 *
 *   - the device comes from the DT node, so of_dma_configure() has run and the
 *     masks are real;
 *   - the command/status/register pools come from a `shared-dma-pool`
 *     reserved-memory node ("cmdbuf") attached with
 *     of_reserved_mem_device_init_by_name();
 *   - the frame-buffer carveout is a second, plain `no-map` reserved-memory
 *     node ("framebuf") whose base and size are handed to framebuf_alloc.c;
 *   - clocks and resets are `clocks =` / `resets =` phandles.
 *
 * The VENC block comes out of a cold boot IN RESET, and clock alone is not
 * enough -- with the clock on but reset asserted the whole 0x4010000 window
 * still reads the 0xDEADBEEF bus poison (device-proven 2026-08-31). So the
 * reset deassert below is load-bearing, and it is the FIRST use of the #80
 * reset provider's .deassert on this silicon.
 */

#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>
#include <linux/slab.h>

#include "vc8000_driver.h"	/* struct vcmd_config, extern venc_pdev, ABI */
#include "framebuf_alloc.h"	/* vcmd_fb_set_dev / vcmd_fb_set_region */

/* --- symbols the VCMD core (eswin/vc8000_vcmd_driver.c) imports --- */
struct platform_device *venc_pdev;
struct platform_device *venc_pdev_d1;	/* AX630C is single-die: always NULL */
EXPORT_SYMBOL(venc_pdev);
EXPORT_SYMBOL(venc_pdev_d1);

extern int venc_vcmd_core_num;			/* defined in the VCMD core */
extern struct vcmd_config vc8000e_vcmd_core_array[];	/* via vc8000_vcmd_cfg.h */

int vc8000e_vcmd_init(void);			/* vcmd_mem_init() + hantroenc_vcmd_init() */
int vc8000e_vcmd_cleanup(bool powered);

/*
 * Hold power for every open file as well as submitted commands. The file
 * reference covers ioctls, mapped pools and close-time abort/register access;
 * task references alone do not cover those paths. Gate only after the final
 * close has drained its commands, never between frames of a live stream.
 */
int enc_pm_runtime_get(u32 core_id)
{
	if (core_id || !venc_pdev)
		return -ENODEV;
	return pm_runtime_resume_and_get(&venc_pdev->dev);
}

int enc_pm_runtime_put(u32 core_id)
{
	if (core_id || !venc_pdev)
		return -ENODEV;
	pm_runtime_mark_last_busy(&venc_pdev->dev);
	return pm_runtime_put_autosuspend(&venc_pdev->dev);
}
int enc_reset_system(u32 core_id)   { (void)core_id; return 0; }
EXPORT_SYMBOL(enc_pm_runtime_get);
EXPORT_SYMBOL(enc_pm_runtime_put);
EXPORT_SYMBOL(enc_reset_system);

struct ax630c_vcmd {
	struct clk *clk;
	struct reset_control *rst;
	int irq;
};

static int ax630c_vcmd_runtime_suspend(struct device *dev)
{
	struct ax630c_vcmd *vc = dev_get_drvdata(dev);

	/* No file/command references remain. Drain a late ISR before gating. */
	disable_irq(vc->irq);
	clk_disable_unprepare(vc->clk);
	return 0;
}

static int ax630c_vcmd_runtime_resume(struct device *dev)
{
	struct ax630c_vcmd *vc = dev_get_drvdata(dev);
	int ret;

	/* Keep reset deasserted: VCMD queue/register state survives gating. */
	ret = clk_prepare_enable(vc->clk);
	if (ret)
		return ret;
	enable_irq(vc->irq);
	return 0;
}

static const struct dev_pm_ops ax630c_vcmd_pm_ops = {
	RUNTIME_PM_OPS(ax630c_vcmd_runtime_suspend,
		       ax630c_vcmd_runtime_resume, NULL)
};

static int ax630c_vcmd_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ax630c_vcmd *vc;
	struct device_node *rmem_np;
	struct reserved_mem *rmem;
	struct resource *res;
	int idx, irq, ret;

	if (venc_pdev)
		return -EBUSY;	/* the core is a singleton */

	vc = devm_kzalloc(dev, sizeof(*vc), GFP_KERNEL);
	if (!vc)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -EINVAL;

	/*
	 * The core's initial self-test cmdbuf completes in hardware, but
	 * without an IRQ nothing wakes the wait queue and probe hangs (proven
	 * on device).
	 */
	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;
	vc->irq = irq;

	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret)
		return dev_err_probe(dev, ret, "32-bit DMA mask\n");

	/*
	 * "cmdbuf": the three coherent VCMD pools (command / status /
	 * register) vcmd_mem_init() allocates with dma_alloc_attrs().
	 */
	ret = of_reserved_mem_device_init_by_name(dev, dev->of_node, "cmdbuf");
	if (ret)
		return dev_err_probe(dev, ret, "cmdbuf memory-region\n");

	/*
	 * "framebuf": encoder frame buffers. The kernel never maps it, so it
	 * is a plain no-map region and framebuf_alloc.c does the bookkeeping.
	 */
	idx = of_property_match_string(dev->of_node, "memory-region-names",
				       "framebuf");
	if (idx < 0) {
		ret = dev_err_probe(dev, idx, "framebuf memory-region\n");
		goto err_rmem;
	}
	rmem_np = of_parse_phandle(dev->of_node, "memory-region", idx);
	if (!rmem_np) {
		ret = -EINVAL;
		goto err_rmem;
	}
	rmem = of_reserved_mem_lookup(rmem_np);
	of_node_put(rmem_np);
	if (!rmem) {
		ret = -EINVAL;
		dev_err(dev, "framebuf region is not reserved memory\n");
		goto err_rmem;
	}
	vcmd_fb_set_region(rmem->base, rmem->size);
	dev_info(dev, "framebuf carveout %pa + %pa\n", &rmem->base, &rmem->size);

	vc->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(vc->clk)) {
		ret = dev_err_probe(dev, PTR_ERR(vc->clk), "venc clock\n");
		goto err_rmem;
	}
	ret = clk_prepare_enable(vc->clk);
	if (ret) {
		dev_err(dev, "venc clock enable failed: %d\n", ret);
		goto err_rmem;
	}

	vc->rst = devm_reset_control_get_exclusive(dev, NULL);
	if (IS_ERR(vc->rst)) {
		ret = dev_err_probe(dev, PTR_ERR(vc->rst), "venc reset\n");
		goto err_clk;
	}
	ret = reset_control_deassert(vc->rst);
	if (ret) {
		dev_err(dev, "venc reset deassert failed: %d\n", ret);
		goto err_clk;
	}

	platform_set_drvdata(pdev, vc);
	pm_runtime_set_active(dev);
	pm_runtime_get_noresume(dev);
	pm_runtime_set_autosuspend_delay(dev, 1000);
	pm_runtime_use_autosuspend(dev);
	pm_runtime_enable(dev);
	venc_pdev = pdev;
	vcmd_fb_set_dev(dev);	/* dma-buf imports attach here (#60) */

	/* Single VC8000E encoder core (JPEG/jenc is a separate device here). */
	venc_vcmd_core_num = 1;
	vc8000e_vcmd_core_array[0].vcmd_base_addr = (unsigned long)res->start;
	vc8000e_vcmd_core_array[0].vcmd_irq = irq;

	ret = vc8000e_vcmd_init();
	if (ret) {
		dev_err(dev, "vc8000e_vcmd_init failed: %d\n", ret);
		goto err_reset;
	}

	dev_info(dev, "VC8000E VCMD engine up (base %pa, irq %d)\n",
		 &res->start, irq);
	pm_runtime_mark_last_busy(dev);
	pm_runtime_put_autosuspend(dev);
	return 0;

err_reset:
	pm_runtime_disable(dev);
	pm_runtime_put_noidle(dev);
	pm_runtime_dont_use_autosuspend(dev);
	pm_runtime_set_suspended(dev);
	venc_pdev = NULL;
	vcmd_fb_set_dev(NULL);
	/*
	 * Leave the block DEASSERTED: that is the end state after a vendor
	 * encode too, so a later reload still finds live hardware.
	 */
err_clk:
	clk_disable_unprepare(vc->clk);
err_rmem:
	of_reserved_mem_device_release(dev);
	return ret;
}

static void ax630c_vcmd_remove(struct platform_device *pdev)
{
	struct ax630c_vcmd *vc = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	int ret;

	/* cleanup touches registers even when the last viewer left long ago. */
	ret = pm_runtime_resume_and_get(dev);
	if (ret < 0)
		dev_err(dev, "cannot resume VCMD for removal: %d\n", ret);
	pm_runtime_disable(dev);
	vc8000e_vcmd_cleanup(ret >= 0);
	vcmd_fb_set_dev(NULL);
	venc_pdev = NULL;
	if (ret >= 0) {
		clk_disable_unprepare(vc->clk);
		pm_runtime_put_noidle(dev);
	}
	pm_runtime_dont_use_autosuspend(dev);
	pm_runtime_set_suspended(dev);
	of_reserved_mem_device_release(dev);
}

static const struct of_device_id ax630c_vcmd_of_match[] = {
	{ .compatible = "axera,ax630c-vc8000e" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ax630c_vcmd_of_match);

static struct platform_driver ax630c_vcmd_driver = {
	.probe = ax630c_vcmd_probe,
	.remove = ax630c_vcmd_remove,
	.driver = {
		.name = "ax630c-venc-vcmd",
		.pm = pm_ptr(&ax630c_vcmd_pm_ops),
		.of_match_table = ax630c_vcmd_of_match,
	},
};
module_platform_driver(ax630c_vcmd_driver);

MODULE_LICENSE("GPL");
MODULE_IMPORT_NS("DMA_BUF");	/* HANTRO_IOCH_IMPORT_DMABUF, framebuf_alloc.c */
MODULE_AUTHOR("open-nanokvm-pro");
MODULE_DESCRIPTION("AX630C platform glue for the VeriSilicon VC8000E VCMD driver");
