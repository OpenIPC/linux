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
#include <linux/axera_dw_apb_timer.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/io.h>

#undef USE_LINUX_CLK

static struct axera_dw_apb_timer *timer_p[AXERA_APB_TIMER_COUNT];
struct axera_irq_data irq_data[AXERA_APB_TIMER_COUNT][PER_TIMER_CHANNEL_COUNT];

static inline u32 apbt_readl(struct axera_dw_apb_timer *timer, unsigned long offs)
{
	return readl(timer->base + offs);
}

static inline void apbt_writel(struct axera_dw_apb_timer *timer, u32 val,
	unsigned long offs)
{
	writel(val, timer->base + offs);
}

static inline u32 apbt_readl_relaxed(struct axera_dw_apb_timer *timer, unsigned long offs)
{
	return readl_relaxed(timer->base + offs);
}

static inline void apbt_writel_relaxed(struct axera_dw_apb_timer *timer, u32 val,
	unsigned long offs)
{
	writel_relaxed(val, timer->base + offs);
}

static void apbt_eoi(struct axera_dw_apb_timer *timer, unsigned int channel_id)
{
	u32 ctrl;

	apbt_readl_relaxed(timer, APBTMR_N_EOI(channel_id));
	ctrl = apbt_readl(timer, APBTMR_N_CONTROL(channel_id));
	ctrl |= APBTMR_CONTROL_INT;
	ctrl &= ~APBTMR_CONTROL_ENABLE;
	apbt_writel(timer, ctrl, APBTMR_N_CONTROL(channel_id));
}

static irqreturn_t dw_apb_timer_irq(int irq, void *data)
{
	int channel_id;
	struct axera_dw_apb_timer *dw_timer;
	struct axera_irq_data *irq_data;


	irq_data = (struct axera_irq_data*)data;

	dw_timer = irq_data->timer;

	channel_id = irq_data->channel_id;

	if (dw_timer->eoi)
		dw_timer->eoi(dw_timer, channel_id);

	schedule_work(&dw_timer->timer_work[channel_id].timer_work);
	return IRQ_HANDLED;
}

static int apbt_set_oneshot(struct axera_dw_apb_timer *timer, unsigned int channel_id)
{
	u32 ctrl;

	ctrl = apbt_readl(timer, APBTMR_N_CONTROL(channel_id));

	ctrl &= ~APBTMR_CONTROL_ENABLE;
	ctrl &= ~APBTMR_CONTROL_MODE_PERIODIC;

	apbt_writel(timer, ctrl, APBTMR_N_CONTROL(channel_id));

	apbt_writel(timer, ~0, APBTMR_N_LOAD_COUNT(channel_id));
	ctrl &= ~APBTMR_CONTROL_INT;
	ctrl |= APBTMR_CONTROL_ENABLE;
	apbt_writel(timer, ctrl, APBTMR_N_CONTROL(channel_id));
	return 0;
}

static int apbt_next_event(unsigned long delta,
	struct axera_dw_apb_timer *timer, unsigned int channel_id)
{
	u32 ctrl;

	/* Disable timer */
	ctrl = apbt_readl_relaxed(timer, APBTMR_N_CONTROL(channel_id));
	ctrl |= APBTMR_CONTROL_INT;
	ctrl &= ~APBTMR_CONTROL_ENABLE;
	apbt_writel_relaxed(timer, ctrl, APBTMR_N_CONTROL(channel_id));
	/* write new count */
	apbt_writel_relaxed(timer, delta, APBTMR_N_LOAD_COUNT(channel_id));
	ctrl &= ~APBTMR_CONTROL_INT;
	ctrl |= APBTMR_CONTROL_ENABLE;
	apbt_writel_relaxed(timer, ctrl, APBTMR_N_CONTROL(channel_id));

	return 0;
}

static void dw_apb_timer_start(struct axera_dw_apb_timer *timer, unsigned int channel_id)
{
	u32 ctrl = apbt_readl(timer, APBTMR_N_CONTROL(channel_id));

	ctrl &= ~APBTMR_CONTROL_ENABLE;
	apbt_writel(timer, ctrl, APBTMR_N_CONTROL(channel_id));
	apbt_writel(timer, ~0, APBTMR_N_LOAD_COUNT(channel_id));
	/* enable, mask interrupt */
	ctrl &= ~APBTMR_CONTROL_MODE_PERIODIC;
	ctrl |= (APBTMR_CONTROL_ENABLE | APBTMR_CONTROL_INT);
	apbt_writel(timer, ctrl, APBTMR_N_CONTROL(channel_id));
}

void apb_timer_enable(unsigned int dev_id)
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

void apb_timer_disable(unsigned int dev_id)
{
	unsigned int val;

	val = readl(timer_p[dev_id]->clk_p_eb_addr_offset + timer_p[dev_id]->clk_base);
	val &= ~(1 << timer_p[dev_id]->clk_p_eb_offset);
	writel(val, timer_p[dev_id]->clk_p_eb_addr_offset + timer_p[dev_id]->clk_base);

	val = readl(timer_p[dev_id]->clk_eb_addr_offset + timer_p[dev_id]->clk_base);
	val &= ~(1 << timer_p[dev_id]->clk_eb_offset);
	writel(val, timer_p[dev_id]->clk_eb_addr_offset + timer_p[dev_id]->clk_base);
}

u32 *timer_open(unsigned int dev_id, unsigned int channel_id)
{
	spin_lock(&timer_p[dev_id]->info_lock[channel_id]);
	if (timer_p[dev_id]->timer_status[channel_id] == 1) {
		pr_err("timer%d.channel[%d] is using\n", dev_id, channel_id);
		spin_unlock(&timer_p[dev_id]->info_lock[channel_id]);
		return NULL;
	}
	timer_p[dev_id]->timer_status[channel_id] = 1;
	spin_unlock(&timer_p[dev_id]->info_lock[channel_id]);
#ifdef USE_LINUX_CLK
	clk_prepare_enable(timer_p[dev_id]->pclk);
	clk_prepare_enable(timer_p[dev_id]->clk);
	clk_set_rate(timer_p[dev_id]->clk, CLK_SEL_FREQ);
#else
	apb_timer_enable(dev_id);
#endif
	dw_apb_timer_start(timer_p[dev_id], channel_id);
	apbt_set_oneshot(timer_p[dev_id], channel_id);
	timer_p[dev_id]->handle[channel_id].channel_id = channel_id;
	return &(timer_p[dev_id]->handle[channel_id].handle);
}
EXPORT_SYMBOL_GPL(timer_open);

int timer_set(u32 *timer_handle, u32 usec, void (*handler)(void *), void *arg)
{
	struct axera_dw_apb_timer *dw_timer;
	struct axera_timer_handle *handle;
	unsigned int channel_id;

	handle = container_of(timer_handle, struct axera_timer_handle, handle);
	channel_id = handle->channel_id;
	dw_timer = container_of(handle, struct axera_dw_apb_timer, handle[channel_id]);
	dw_timer->timer_handle[channel_id] = handler;
	dw_timer->arg[channel_id] = arg;
	apbt_next_event(HZ_PER_USEC * usec, dw_timer, channel_id);

	return 0;
}
EXPORT_SYMBOL_GPL(timer_set);

void timer_close(u32 *timer_handle)
{
	struct axera_dw_apb_timer *dw_timer;
	struct axera_timer_handle *handle;
	unsigned int channel_id;
	unsigned int ctrl = 0;
	unsigned int id = 0;

	handle = container_of(timer_handle, struct axera_timer_handle, handle);
	channel_id = handle->channel_id;
	dw_timer = container_of(handle, struct axera_dw_apb_timer, handle[channel_id]);

	spin_lock(&dw_timer->info_lock[channel_id]);
	if (dw_timer->timer_status[channel_id] == 0) {
		spin_unlock(&dw_timer->info_lock[channel_id]);
		return;
	}
	dw_timer->timer_status[channel_id] = 0;
	spin_unlock(&dw_timer->info_lock[channel_id]);

	ctrl = apbt_readl(dw_timer, APBTMR_N_CONTROL(channel_id));
	ctrl &= ~APBTMR_CONTROL_ENABLE;
	apbt_writel(dw_timer, ctrl, APBTMR_N_CONTROL(channel_id));

	for (id = 0; id < PER_TIMER_CHANNEL_COUNT; id++) {
		if (dw_timer->timer_status[id] != 0) {
			break;
		}
	}

	if (id == PER_TIMER_CHANNEL_COUNT) {
#ifdef USE_LINUX_CLK
		clk_disable_unprepare(dw_timer->pclk);
		clk_disable_unprepare(dw_timer->clk);
#else
	apb_timer_disable(dw_timer->dev_id);
#endif
	}

	return;
}
EXPORT_SYMBOL_GPL(timer_close);

void handle_channel(struct work_struct *timer_work)
{
	struct axera_work_struct *work;
	struct axera_dw_apb_timer *dw_timer;
	unsigned int channel_id = 0;

	work = container_of(timer_work, struct axera_work_struct, timer_work);
	channel_id = work->channel_id;
	dw_timer = container_of(work, struct axera_dw_apb_timer, timer_work[channel_id]);

	if (dw_timer->timer_handle[channel_id] != NULL) {
		dw_timer->timer_handle[channel_id](dw_timer->arg[channel_id]);
	}

	return;
}

#if 0
void test(void *arg)
{
	printk("arg is %s", (char*)arg);
}
#endif

static int axera_dw_timer_probe(struct platform_device *pdev)
{
	int err;
	int index = 0;
	int channel_id = 0;
	unsigned int val = 0;
	struct device_node *np = pdev->dev.of_node;
	char channel_name[10] = {0};
	struct resource *res_base;
	//u32 *handle;

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
	if (!(timer_p[index]->base))
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

	for(channel_id = 0; channel_id < PER_TIMER_CHANNEL_COUNT; channel_id++) {
		sprintf(channel_name,"channel%d",channel_id);
		timer_p[index]->irq[channel_id] = platform_get_irq_byname(pdev, channel_name);
		if (channel_id == 0) {
			timer_p[index]->start_irq = timer_p[index]->irq[0];
		}
		memset(channel_name, 0, sizeof(channel_name));
		if (timer_p[index]->irq[channel_id] < 0) {
			pr_err("failed to get channel irq.\n");
			return -ENODATA;
		}
	}

	timer_p[index]->eoi = apbt_eoi;

	for(channel_id = 0; channel_id < PER_TIMER_CHANNEL_COUNT; channel_id++) {
		irq_data[index][channel_id].channel_id = channel_id;
		irq_data[index][channel_id].timer = timer_p[index];
		err = devm_request_irq(&pdev->dev, timer_p[index]->irq[channel_id], dw_apb_timer_irq,
			IRQF_TRIGGER_HIGH, KBUILD_MODNAME, &irq_data[index][channel_id]);
		if (err) {
			return -ENOMEM;
		}
	}

	for(channel_id = 0; channel_id < PER_TIMER_CHANNEL_COUNT; channel_id++) {
		spin_lock_init(&timer_p[index]->info_lock[channel_id]);
		INIT_WORK(&timer_p[index]->timer_work[channel_id].timer_work, handle_channel);
		timer_p[index]->timer_work[channel_id].channel_id = channel_id;
	}
#if 0
	/* Test */
	handle = timer_open(index, 0);
	if (index == 0)
		timer_set(handle, 5000000, test, "dev 0 channel 0\r\n");
	else
		timer_set(handle, 5000000, test, "dev 1 channel 0\r\n");
	//timer_close(handle);

	handle = timer_open(index, 1);
	if (index == 0)
		timer_set(handle, 5000000, test, "dev 0 channel 1\r\n");
	else
		timer_set(handle, 5000000, test, "dev 1 channel 1\r\n");
	//timer_close(handle);

	handle = timer_open(index, 2);
	if (index == 0)
		timer_set(handle, 5000000, test, "dev 0 channel 2\r\n");
	else
		timer_set(handle, 5000000, test, "dev 1 channel 2\r\n");
	//timer_close(handle);

	handle = timer_open(index, 3);
	if (index == 0)
		timer_set(handle, 5000000, test, "dev 0 channel 3\r\n");
	else
		timer_set(handle, 5000000, test, "dev 1 channel 3\r\n");
	//timer_close(handle);
#endif

	return 0;
}

static int axera_dw_timer_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id axera_dw_timer_of_id_table[] = {
	{ .compatible = "axera,dw-apb-timer" },
	{}
};
MODULE_DEVICE_TABLE(of, axera_dw_timer_of_id_table);

static struct platform_driver axera_dw_timer_driver = {
	.probe  = axera_dw_timer_probe,
	.remove = axera_dw_timer_remove,
	.driver = {
		.name = KBUILD_MODNAME,
		.of_match_table = axera_dw_timer_of_id_table,
	},
};

module_platform_driver(axera_dw_timer_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("aichip dw timer platform driver");



