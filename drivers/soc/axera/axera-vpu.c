// SPDX-License-Identifier: GPL-2.0-only
/*
 * Clock, reset, and frequency for the AX630C Hantro blocks.
 *
 * Live IDs after the vendor clock gate and reset (105, 6.18):
 *   venc@4010000  0x43421500 0x20221012  VCMD, build date 2022-10-12
 *   jenc@4000000  0x43421500 0x20221012  same VCMD wrapper
 *   vdec@4020000  0x6e645000              VC8000D core at offset 0
 *
 * Vendor ax_venc.ko (4.19.125, GPL, "VC8000 Vcmd driver") probes by the
 * resource name "ax_venc", allocates three AX_OSAL CMM pools (command,
 * status, registers), calls vcmd_arm_dma_init, then hantrovcmd_init.
 * That .c is not in the public SDK. ax_venc.ko also calls
 * pm_opp_of_add_table and devfreq; this driver does the clock half of
 * that. It does not program the command queue.
 */

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>

struct axera_vpu {
	void __iomem *base;
	struct clk *clk;
	struct reset_control *rst;
	bool opp;
};

static ssize_t regs_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	struct axera_vpu *vpu = dev_get_drvdata(dev);
	u32 w0, w1, w2, w3, c0, c1;
	int ret;
	ssize_t len;

	ret = pm_runtime_resume_and_get(dev);
	if (ret < 0)
		return ret;
	w0 = readl_relaxed(vpu->base + 0x0);
	w1 = readl_relaxed(vpu->base + 0x4);
	w2 = readl_relaxed(vpu->base + 0x8);
	w3 = readl_relaxed(vpu->base + 0xc);
	/* Public VC8000 VCMD puts the encoder core at 0x1000. */
	c0 = readl_relaxed(vpu->base + 0x1000);
	c1 = readl_relaxed(vpu->base + 0x1004);
	len = sysfs_emit(buf, "0000: %08x %08x %08x %08x\n1000: %08x %08x\n",
			 w0, w1, w2, w3, c0, c1);
	pm_runtime_mark_last_busy(dev);
	pm_runtime_put_autosuspend(dev);
	return len;
}
static DEVICE_ATTR_RO(regs);

static ssize_t rate_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	struct axera_vpu *vpu = dev_get_drvdata(dev);

	return sysfs_emit(buf, "%lu\n", clk_get_rate(vpu->clk));
}

static ssize_t rate_store(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct axera_vpu *vpu = dev_get_drvdata(dev);
	unsigned long rate;
	int ret;

	if (!vpu->opp)
		return -ENODEV;

	ret = kstrtoul(buf, 0, &rate);
	if (ret)
		return ret;

	ret = pm_runtime_resume_and_get(dev);
	if (ret < 0)
		return ret;
	ret = dev_pm_opp_set_rate(dev, rate);
	pm_runtime_mark_last_busy(dev);
	pm_runtime_put_autosuspend(dev);
	if (ret)
		return ret;

	return count;
}
static DEVICE_ATTR_RW(rate);

static ssize_t available_frequencies_show(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	unsigned long freq = 0;
	ssize_t len = 0;

	for (;;) {
		struct dev_pm_opp *opp;

		opp = dev_pm_opp_find_freq_ceil(dev, &freq);
		if (IS_ERR(opp))
			break;
		len += sysfs_emit_at(buf, len, "%lu ", freq);
		dev_pm_opp_put(opp);
		freq++;
	}

	if (len)
		buf[len - 1] = '\n';
	return len;
}
static DEVICE_ATTR_RO(available_frequencies);

static struct attribute *axera_vpu_attrs[] = {
	&dev_attr_regs.attr,
	&dev_attr_rate.attr,
	&dev_attr_available_frequencies.attr,
	NULL,
};

static const struct attribute_group axera_vpu_group = {
	.attrs = axera_vpu_attrs,
};

static int axera_vpu_runtime_suspend(struct device *dev)
{
	struct axera_vpu *vpu = dev_get_drvdata(dev);

	/*
	 * This diagnostic driver submits no jobs. Only register/rate accesses
	 * need a clock; preserve reset and register state across idle periods.
	 */
	clk_disable_unprepare(vpu->clk);
	return 0;
}

static int axera_vpu_runtime_resume(struct device *dev)
{
	struct axera_vpu *vpu = dev_get_drvdata(dev);

	return clk_prepare_enable(vpu->clk);
}

static const struct dev_pm_ops axera_vpu_pm_ops = {
	RUNTIME_PM_OPS(axera_vpu_runtime_suspend, axera_vpu_runtime_resume, NULL)
};

static int axera_vpu_probe(struct platform_device *pdev)
{
	struct axera_vpu *vpu;
	u32 hwid, date;
	int ret;

	vpu = devm_kzalloc(&pdev->dev, sizeof(*vpu), GFP_KERNEL);
	if (!vpu)
		return -ENOMEM;

	vpu->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(vpu->base))
		return PTR_ERR(vpu->base);

	vpu->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(vpu->clk))
		return dev_err_probe(&pdev->dev, PTR_ERR(vpu->clk), "clk\n");

	vpu->rst = devm_reset_control_get_exclusive(&pdev->dev, NULL);
	if (IS_ERR(vpu->rst))
		return dev_err_probe(&pdev->dev, PTR_ERR(vpu->rst), "reset\n");

	ret = clk_prepare_enable(vpu->clk);
	if (ret)
		return ret;

	ret = reset_control_deassert(vpu->rst);
	if (ret) {
		clk_disable_unprepare(vpu->clk);
		return ret;
	}

	ret = devm_pm_opp_of_add_table(&pdev->dev);
	if (ret)
		dev_warn(&pdev->dev, "OPP table: %d\n", ret);
	else
		vpu->opp = true;

	platform_set_drvdata(pdev, vpu);
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_get_noresume(&pdev->dev);
	pm_runtime_set_autosuspend_delay(&pdev->dev, 1000);
	pm_runtime_use_autosuspend(&pdev->dev);
	pm_runtime_enable(&pdev->dev);
	ret = device_add_group(&pdev->dev, &axera_vpu_group);
	if (ret) {
		pm_runtime_disable(&pdev->dev);
		pm_runtime_put_noidle(&pdev->dev);
		pm_runtime_dont_use_autosuspend(&pdev->dev);
		pm_runtime_set_suspended(&pdev->dev);
		reset_control_assert(vpu->rst);
		clk_disable_unprepare(vpu->clk);
		return ret;
	}

	hwid = readl(vpu->base);
	date = readl(vpu->base + 4);
	dev_info(&pdev->dev, "hwid %08x %08x, clock %lu Hz, reset deasserted\n",
		 hwid, date, clk_get_rate(vpu->clk));
	pm_runtime_mark_last_busy(&pdev->dev);
	pm_runtime_put_autosuspend(&pdev->dev);
	return 0;
}

static void axera_vpu_remove(struct platform_device *pdev)
{
	struct axera_vpu *vpu = platform_get_drvdata(pdev);
	int ret;

	device_remove_group(&pdev->dev, &axera_vpu_group);
	ret = pm_runtime_resume_and_get(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	if (ret >= 0) {
		reset_control_assert(vpu->rst);
		clk_disable_unprepare(vpu->clk);
		pm_runtime_put_noidle(&pdev->dev);
	}
	pm_runtime_dont_use_autosuspend(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);
}

static const struct of_device_id axera_vpu_of_match[] = {
	{ .compatible = "axera, venc-encoder" },
	{ .compatible = "axera,jpeg-encoder" },
	{ }
};
MODULE_DEVICE_TABLE(of, axera_vpu_of_match);

static struct platform_driver axera_vpu_driver = {
	.probe = axera_vpu_probe,
	.remove = axera_vpu_remove,
	.driver = {
		.name = "axera-vpu",
		.of_match_table = axera_vpu_of_match,
		.pm = pm_ptr(&axera_vpu_pm_ops),
	},
};
module_platform_driver(axera_vpu_driver);

MODULE_DESCRIPTION("Axera AX630C VPU clock, reset, and frequency");
MODULE_LICENSE("GPL");
