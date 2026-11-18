/*
 * (C) Copyright 2009 Intel Corporation
 * Author: Jacob Pan (jacob.jun.pan@intel.com)
 *
 * Shared with ARM platforms, Jamie Iles, Picochip 2011
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Support for the Synopsys DesignWare APB Timers.
 */
#include<linux/module.h>
#include<linux/init.h>
#include<linux/fs.h>
#include<linux/sched.h>

#include<linux/device.h>
#include<linux/string.h>
#include<linux/errno.h>
#include <linux/delay.h>
#include<linux/types.h>
#include<linux/slab.h>
#include<asm/uaccess.h>
#include <linux/of_platform.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>
#include <linux/ioctl.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/io.h>

#define CLK_SEL_FREQ	24000000

struct axera_periph_timer32 {
	void __iomem	*base;
	void __iomem	*clk_base;
	unsigned int	clk_sel_addr_offset;
	unsigned int	clk_sel_offset;
	unsigned int	clk_glb_eb_addr_offset;
	unsigned int	clk_glb_eb_offset;
	unsigned int	clk_eb_addr_offset;
	unsigned int	clk_eb_offset;
	unsigned int	clk_p_eb_addr_offset;
	unsigned int	clk_p_eb_offset;
	int		irq;
	int 		dev_id;
	unsigned int 	handle;
	unsigned int 	usec;
	spinlock_t	lock;
	unsigned int 	timer_status;
	void (*timer_handle)(void *);
	void (*eoi)(struct axera_periph_timer32 *);
	void *arg;
	struct work_struct timer_work;
	struct reset_control *preset;
	struct reset_control *reset;
	struct clk *pclk;
	struct clk *clk;
};

#define COUNT_PER_USEC			24

#define AXERA_PERIPH_TIMER32_COUNT 	4

#define TIMER32_CNT_CCVR		0x0
#define TIMER32_CMR				0x4
#define TIMER32_CMR_START		0x10
#define TIMER32_INTR_CTRL		0x1C
#define BIT_TIMER32_CMR_START	(1 << 0)
#define BIT_TIMER32_INTR_EN		(1 << 0)
#define BIT_TIMER32_INTR_EOI	(1 << 2)

static struct axera_periph_timer32 *timer_p[AXERA_PERIPH_TIMER32_COUNT];

#undef USE_LINUX_CLK

void periph_timer32_enable(unsigned int dev_id)
{
	unsigned int val;

	val = readl(timer_p[dev_id]->clk_p_eb_addr_offset + timer_p[dev_id]->clk_base);
	val &= ~(1 << timer_p[dev_id]->clk_p_eb_offset);
	writel(val, timer_p[dev_id]->clk_p_eb_addr_offset + timer_p[dev_id]->clk_base);

	val = readl(timer_p[dev_id]->clk_eb_addr_offset + timer_p[dev_id]->clk_base);
	val &= ~(1 << timer_p[dev_id]->clk_eb_offset);
	writel(val, timer_p[dev_id]->clk_eb_addr_offset + timer_p[dev_id]->clk_base);

	val = readl(timer_p[dev_id]->clk_sel_addr_offset + timer_p[dev_id]->clk_base);
	val |= (1 << timer_p[dev_id]->clk_sel_offset);
	writel(val, timer_p[dev_id]->clk_sel_addr_offset + timer_p[dev_id]->clk_base);

	val = readl(timer_p[dev_id]->clk_p_eb_addr_offset + timer_p[dev_id]->clk_base);
	val |= (1 << timer_p[dev_id]->clk_p_eb_offset);
	writel(val, timer_p[dev_id]->clk_p_eb_addr_offset + timer_p[dev_id]->clk_base);

	val = readl(timer_p[dev_id]->clk_eb_addr_offset + timer_p[dev_id]->clk_base);
	val |= (1 << timer_p[dev_id]->clk_eb_offset);
	writel(val, timer_p[dev_id]->clk_eb_addr_offset + timer_p[dev_id]->clk_base);

}

void periph_timer32_disable(unsigned int dev_id)
{
	unsigned int val;

	val = readl(timer_p[dev_id]->clk_p_eb_addr_offset + timer_p[dev_id]->clk_base);
	val &= ~(1 << timer_p[dev_id]->clk_p_eb_offset);
	writel(val, timer_p[dev_id]->clk_p_eb_addr_offset + timer_p[dev_id]->clk_base);

	val = readl(timer_p[dev_id]->clk_eb_addr_offset + timer_p[dev_id]->clk_base);
	val &= ~(1 << timer_p[dev_id]->clk_eb_offset);
	writel(val, timer_p[dev_id]->clk_eb_addr_offset + timer_p[dev_id]->clk_base);
}

u32* periph_timer32_open(unsigned int dev_id)
{
	spin_lock(&timer_p[dev_id]->lock);
	if (timer_p[dev_id]->timer_status == 1) {
		pr_err("timer is using\n");
		spin_unlock(&timer_p[dev_id]->lock);
		return NULL;
	}
	timer_p[dev_id]->timer_status = 1;
	spin_unlock(&timer_p[dev_id]->lock);
#ifdef USE_LINUX_CLK
	clk_prepare_enable(timer_p[dev_id]->pclk);
	clk_prepare_enable(timer_p[dev_id]->clk);
	clk_set_rate(timer_p[dev_id]->clk, CLK_SEL_FREQ);
#else
	periph_timer32_enable(dev_id);
#endif

	return &(timer_p[dev_id]->handle);
}

EXPORT_SYMBOL_GPL(periph_timer32_open);

int periph_timer32_set(u32 *timer_handle, u32 usec, void (*handle)(void *), void *arg)
{
	struct axera_periph_timer32 *timer;
	unsigned int value;

	timer = container_of(timer_handle, struct axera_periph_timer32, handle);
	timer->timer_handle = handle;
	timer->arg = arg;
	timer->usec = usec;

	value = readl(timer->base + TIMER32_CNT_CCVR);
	value += usec * COUNT_PER_USEC;
	writel(value, TIMER32_CMR + timer->base);

	//compare start
	writel(BIT_TIMER32_CMR_START, TIMER32_CMR_START + timer->base);

	//set int_en
	writel(BIT_TIMER32_INTR_EN, TIMER32_INTR_CTRL + timer->base);

	return 0;
}
EXPORT_SYMBOL_GPL(periph_timer32_set);

void periph_timer32_close(u32 *timer_handle)
{
	struct axera_periph_timer32 *timer;
	timer = container_of(timer_handle, struct axera_periph_timer32, handle);
	spin_lock(&timer->lock);
	if (timer->timer_status == 0) {
		spin_unlock(&timer->lock);
		return;
	}
	timer->timer_status = 0;
	spin_unlock(&timer->lock);

	/* disable int */
	writel(0, TIMER32_INTR_CTRL + timer->base);
	/* set cmr 0 */
	writel(0, TIMER32_CMR + timer->base);

#ifdef USE_LINUX_CLK
	clk_disable_unprepare(timer->pclk);
	clk_disable_unprepare(timer->clk);
#else
	periph_timer32_disable(timer->dev_id);
#endif

	return;
}
EXPORT_SYMBOL_GPL(periph_timer32_close);

static irqreturn_t axera_timer32_irq(int irq, void *data)
{

	struct axera_periph_timer32 *timer = (struct axera_periph_timer32 *)data;
	if (timer->eoi)
		timer->eoi(timer);

	schedule_work(&timer->timer_work);

	/* clear cmr_start before set cmr again */

	writel(0, TIMER32_CMR_START + timer->base);

	periph_timer32_set(&timer->handle, timer->usec, timer->timer_handle, timer->arg);

	return IRQ_HANDLED;
}

static void handle_eoi(struct axera_periph_timer32 *timer)
{
	int val;

	val = readl(TIMER32_INTR_CTRL + timer->base);
	val |= BIT_TIMER32_INTR_EOI;
	writel(val, TIMER32_INTR_CTRL + timer->base);
}

static void do_time_handle(struct work_struct *timer_work)
{
	struct axera_periph_timer32 *timer;
	timer = container_of(timer_work, struct axera_periph_timer32, timer_work);
	if (timer->timer_handle != NULL) {
		timer->timer_handle(timer->arg);
	}
	return;
}

#if 0
static void periph_test(void *arg)
{
	printk("arg is %s\n", (char*)arg);

}
#endif

static int axera_periph_timer32_probe(struct platform_device *pdev)
{
	int err;
	int index = 0;
	unsigned int val = 0;
	struct device_node *np = pdev->dev.of_node;
	struct resource *res_base;
	//unsigned int *handle;

	if (of_property_read_u32(np, "timer_index", &index)) {
		pr_err("get timer index failed.\n");
		return -ENODEV;
	}

	timer_p[index] = devm_kzalloc(&pdev->dev, sizeof(*timer_p[0]), GFP_KERNEL);
	timer_p[index]->dev_id = index;

	if (!timer_p[index])
		return -ENOMEM;

	res_base = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (!res_base) {
		pr_err("get res_base failed.\n");
		return -ENOMEM;
	}

	timer_p[index]->base = devm_ioremap_resource(&pdev->dev, res_base);
	if (IS_ERR(timer_p[index]->base))
		return PTR_ERR(timer_p[index]->base);

	timer_p[index]->preset = devm_reset_control_get_optional(&pdev->dev, "preset");
	if (IS_ERR(timer_p[index]->preset)) {
		pr_err("axera get global reset failed\n");
		return PTR_ERR(timer_p[index]->preset);
	}

	reset_control_deassert(timer_p[index]->preset);

	timer_p[index]->reset = devm_reset_control_get_optional(&pdev->dev, "reset");
	if (IS_ERR(timer_p[index]->reset)) {
		pr_err("axera get reset failed\n");
		return PTR_ERR(timer_p[index]->reset);
	}

	reset_control_deassert(timer_p[index]->reset);

#ifdef USE_LINUX_CLK
	timer_p[index]->pclk = devm_clk_get(&pdev->dev, "pclk");

	if (IS_ERR(timer_p[index]->pclk)) {
		pr_err("axera get pclk fail\n");
		return PTR_ERR(timer_p[index]->pclk);
	}

	timer_p[index]->clk = devm_clk_get(&pdev->dev, "clk");

	if (IS_ERR(timer_p[index]->clk)) {
		pr_err("axera get clk fail\n");
		return PTR_ERR(timer_p[index]->clk);
	}
#else
	res_base = platform_get_resource(pdev, IORESOURCE_MEM, 1);

	if (!res_base) {
		pr_err("get res_base failed.\n");
		return -ENOMEM;
	}

	timer_p[index]->clk_base = devm_ioremap(&pdev->dev, res_base->start, resource_size(res_base));
	if (IS_ERR(timer_p[index]->clk_base))
		return PTR_ERR(timer_p[index]->clk_base);

	if (of_property_read_u32(np, "clk-sel-addr-offset", &timer_p[index]->clk_sel_addr_offset)) {
		pr_err("get clk-sel-addr-offset failed.\n");
		return -ENODEV;
	}

	if (of_property_read_u32(np, "clk-sel-offset", &timer_p[index]->clk_sel_offset)) {
		pr_err("get clk-sel-offset failed.\n");
		return -ENODEV;
	}

	if (of_property_read_u32(np, "clk-glb-eb-addr-offset", &timer_p[index]->clk_glb_eb_addr_offset)) {
		pr_err("get clk-glb-eb-addr-offset failed.\n");
		return -ENODEV;
	}

	if (of_property_read_u32(np, "clk-glb-eb-offset", &timer_p[index]->clk_glb_eb_offset)) {
		pr_err("get clk-glb-eb-offset failed.\n");
		return -ENODEV;
	}

	if (of_property_read_u32(np, "clk-eb-addr-offset", &timer_p[index]->clk_eb_addr_offset)) {
		pr_err("get clk-eb-addr-offset failed.\n");
		return -ENODEV;
	}

	if (of_property_read_u32(np, "clk-eb-offset", &timer_p[index]->clk_eb_offset)) {
		pr_err("get clk_eb_offset failed.\n");
		return -ENODEV;
	}

	if (of_property_read_u32(np, "clk-p-eb-addr-offset", &timer_p[index]->clk_p_eb_addr_offset)) {
		pr_err("get clk_p_eb_addr_offset failed.\n");
		return -ENODEV;
	}

	if (of_property_read_u32(np, "clk-p-eb-offset", &timer_p[index]->clk_p_eb_offset)) {
		pr_err("get clk-p-eb-offset failed.\n");
		return -ENODEV;
	}

	val = readl(timer_p[index]->clk_glb_eb_addr_offset + timer_p[index]->clk_base);
	val |= (1 << timer_p[index]->clk_glb_eb_offset);
	writel(val, timer_p[index]->clk_glb_eb_addr_offset + timer_p[index]->clk_base);

#endif

	timer_p[index]->irq = platform_get_irq_byname(pdev, "timer32_int");
	if (timer_p[index]->irq < 0) {
		pr_err("failed to get irq.\n");
		return -ENODATA;
	}

	err = devm_request_irq(&pdev->dev, timer_p[index]->irq, axera_timer32_irq,
		IRQF_TRIGGER_HIGH, KBUILD_MODNAME, timer_p[index]);
	if (err) {
		return -ENOMEM;
	}

	timer_p[index]->eoi = handle_eoi;

	spin_lock_init(&timer_p[index]->lock);
	INIT_WORK(&timer_p[index]->timer_work, do_time_handle);

#if 0
	handle = periph_timer32_open(index);
	if (index == 0) {
		periph_timer32_set(handle, 5000000, periph_test, "periph timer32 0.\r\n");
	} else if(index == 1) {
		periph_timer32_set(handle, 5000000, periph_test, "periph timer32 1.\r\n");
	} else if (index == 2) {
		periph_timer32_set(handle, 5000000, periph_test, "periph timer32 2.\r\n");
	} else {
		periph_timer32_set(handle, 5000000, periph_test, "periph timer32 3.\r\n");
	}
#endif

	return 0;
}

static int axera_periph_timer32_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id axera_periph_timer32_of_id_table[] = {
	{ .compatible = "axera,periph-timer32" },
	{}
};
MODULE_DEVICE_TABLE(of, axera_periph_timer32_of_id_table);

static struct platform_driver axera_periph_timer32_driver = {
	.probe	= axera_periph_timer32_probe,
	.remove = axera_periph_timer32_remove,
	.driver = {
		.name = KBUILD_MODNAME,
		.of_match_table = axera_periph_timer32_of_id_table,
	},
};

module_platform_driver(axera_periph_timer32_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("axera periph timer32 driver");