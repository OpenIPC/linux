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
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#define COMMON_SYS_XTAL_SLEEP_BYP_ADDR_OFFSET	0x190
#define COMMON_SYS_EIC_EN_SET_ADDR_OFFSET	0x214
#define COMMON_SYS_SW_RST_0_ADDR_OFFSET	0x54
#define COMMON_SYS_CLK_MUX1_ADDR_OFFSET	0xC
#define COMMON_SYS_CLK_EB_0_ADDR_OFFSET	0x24
#define COMMON_SYS_CLK_EB_1_ADDR_OFFSET	0x30

#define PINMUX_G6_MISC_CLR_ADDR	0x23040AC
#define TIMER_COUNT_ADDR 	0x10FC
#define TIMER_EB_ADDR 	0x10F8

#define TIMER32_CNT_CCVR_OFFSET	0x0
#define TIMER32_CMR_OFFSET	0x4
#define TIMER32_CMR_START_OFFSET	0x10
#define TIMER32_INTR_CTRL_OFFSET	0x1C
#define TIMER32_INTR_STATUS_OFFSET	0x28

#define PINMUX_G6_CTRL_CLR_CLR	(1 << 0)
#define BIT_COMMON_TMR32_EIC_EN_SET	(1 << 1)
#define BIT_COMMON_SYS_TIMER32_SW_RST	(1 << 22)
#define BIT_COMMON_SYS_TIMER32_SW_PRST	(1 << 21)
#define BIT_COMMON_SYS_CLK_TIMER32_SEL_24M	(1 << 26)
#define BIT_COMMON_SYS_CLK_TIMER32_EB	(1 << 14)
#define BIT_COMMON_SYS_PCLK_TMR32_EB	(1 << 15)
#define BIT_TIMER32_INTR_EOI	(1 << 2)
#define BIT_TIMER32_INTR_MASK	(1 << 1)
#define BIT_TIMER32_INTR_EN	(1 << 0)
#define BIT_TIMER32_CMR_START	(1 << 0)

extern int ax_wdt_set_keep_alive_timeout(int wdt_id, unsigned int timeout);

static struct proc_dir_entry *wake_timer_root;
static unsigned int timers;
static unsigned int timer_eb = 0;
wait_queue_head_t wq;
atomic_t wq_status;


#define PROC_NODE_ROOT_NAME "ax_proc/wake_timer"
#define PROC_TIMER_SET	"timers"
#define PROC_EB_TIMER_SET	"enable_timers"

#define TIMER32_WAKEUP_NAME "timer32_wakeup"

static DEFINE_MUTEX(ax_wake_timer_mutex);

struct wake_timer {
	struct resource *sys_res;
	struct resource *tmr_res;
	void __iomem	*sys_base;
	void __iomem	*tmr_base;
	void __iomem	*pin_g6_base;
	void __iomem	*timer_count_addr;
	void __iomem	*timer_eb_addr;
	struct wakeup_source *ws;
	int major;
	struct class *timer32_class;
};

static struct wake_timer *wake_timer_res;

static irqreturn_t wake_int_handler(int irq, void *dev_id)
{
	int val = 0;

	pm_wakeup_ws_event(wake_timer_res->ws, 0, false);

	if (readl(wake_timer_res->tmr_base + TIMER32_INTR_STATUS_OFFSET)) {
		atomic_inc(&wq_status);
		wake_up_interruptible(&wq);
		/* stop compare */
		writel(0, wake_timer_res->tmr_base + TIMER32_CMR_START_OFFSET);

		val = readl(wake_timer_res->tmr_base + TIMER32_INTR_CTRL_OFFSET);
		val |= BIT_TIMER32_INTR_EOI;// set int eoi
		val |= BIT_TIMER32_INTR_MASK;// set int mask
		val &= ~BIT_TIMER32_INTR_EN;// disable int
		writel(val, wake_timer_res->tmr_base + TIMER32_INTR_CTRL_OFFSET);
	}

	return IRQ_HANDLED;
}

static int ax_wake_timer_show(struct seq_file *m, void *v)
{
	mutex_lock(&ax_wake_timer_mutex);
	seq_printf(m, "%u", readl( wake_timer_res->timer_count_addr));
	mutex_unlock(&ax_wake_timer_mutex);
	return 0;
}

static int ax_wake_timer_open(struct inode *inode, struct file *file)
{
	return single_open(file, ax_wake_timer_show, NULL);
}

static ssize_t ax_wake_timer_write(struct file *file, const char __user *buffer,
					  size_t count, loff_t *ppos)
{
	char kbuf[32] = { 0 };
	unsigned int wdt_times = 0;

	if (count > 32) {
		return -1;
	}

	if (copy_from_user(kbuf, buffer, count)) {
		return -EFAULT;
	}

	if (sscanf(kbuf, "%d", (unsigned int*)&timers) != 1) {
		return -1;
	}

	if (timers <= 4) {
		printk("wakeup timer count can't <= 4ms,at least 5ms\r\n");
		return -1;
	}

	mutex_lock(&ax_wake_timer_mutex);

	writel(timers, wake_timer_res->timer_count_addr);

	wdt_times = DIV_ROUND_UP(timers, 2000) + 16;

	pr_info("timer32 timeout is %d ms, wdt timeout is %d s, system reset timeout is %d s\r\n", timers, wdt_times, wdt_times * 2);

	ax_wdt_set_keep_alive_timeout(1, wdt_times);

	mutex_unlock(&ax_wake_timer_mutex);

	return count;
}

static const struct file_operations ax_wake_timer_fsops = {
	.open = ax_wake_timer_open,
	.read = seq_read,
	.write = ax_wake_timer_write,
	.release = single_release,
};

static int ax_wake_timer_eb_show(struct seq_file *m, void *v)
{
	mutex_lock(&ax_wake_timer_mutex);
	seq_printf(m, "%u", readl( wake_timer_res->timer_eb_addr));
	mutex_unlock(&ax_wake_timer_mutex);
	return 0;
}

static int ax_wake_timer_eb_open(struct inode *inode, struct file *file)
{
	return single_open(file, ax_wake_timer_eb_show, NULL);
}

static ssize_t ax_wake_timer_eb_write(struct file *file, const char __user *buffer,
					  size_t count, loff_t *ppos)
{
	char kbuf[32] = { 0 };

	if (count > 32) {
		return -1;
	}

	if (copy_from_user(kbuf, buffer, count)) {
		return -EFAULT;
	}

	if (sscanf(kbuf, "%d", (unsigned int*)&timer_eb) != 1) {
		return -1;
	}

	if (timer_eb < 0) {
		return -1;
	}

	if (timer_eb > 0) {
		timer_eb = 0x1;
	}

	mutex_lock(&ax_wake_timer_mutex);

	writel(timer_eb, wake_timer_res->timer_eb_addr);

	mutex_unlock(&ax_wake_timer_mutex);

	return count;
}

static const struct file_operations ax_wake_timer_eb_fsops = {
	.open = ax_wake_timer_eb_open,
	.read = seq_read,
	.write = ax_wake_timer_eb_write,
	.release = single_release,
};

static __poll_t timer32_poll(struct file *file, poll_table *pts)
{
	__poll_t mask = 0;

	poll_wait(file, &wq, pts);

	if (atomic_read(&wq_status)) {
		atomic_dec(&wq_status);
		mask = POLLIN | POLLRDNORM;
	} else
		mask = 0;

	return mask;
}

const struct file_operations timer32_fops = {
	.owner = THIS_MODULE,
	.poll = timer32_poll,
};

static int axera_wake_timer_probe(struct platform_device *pdev)
{
	int irq;
	int ret;
	struct device *timer32_dev;

	wake_timer_res = (struct wake_timer *)devm_kzalloc(&pdev->dev, sizeof(struct wake_timer), GFP_KERNEL);

	wake_timer_res->sys_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!wake_timer_res->sys_res)
		return -ENODATA;
	wake_timer_res->sys_base = devm_ioremap(&pdev->dev, wake_timer_res->sys_res->start, resource_size(wake_timer_res->sys_res));
	if (IS_ERR(wake_timer_res->sys_base)) {
		return PTR_ERR(wake_timer_res->sys_base);
	}

	wake_timer_res->tmr_res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!wake_timer_res->tmr_res)
		return -ENODATA;
	wake_timer_res->tmr_base = devm_ioremap(&pdev->dev, wake_timer_res->tmr_res->start, resource_size(wake_timer_res->tmr_res));
	if (IS_ERR(wake_timer_res->tmr_base)) {
		return PTR_ERR(wake_timer_res->tmr_base);
	}

	wake_timer_res->pin_g6_base = devm_ioremap(&pdev->dev, PINMUX_G6_MISC_CLR_ADDR, 0x4);
	if (IS_ERR(wake_timer_res->pin_g6_base)) {
		return PTR_ERR(wake_timer_res->pin_g6_base);
	}

	wake_timer_res->timer_count_addr = devm_ioremap(&pdev->dev, TIMER_COUNT_ADDR, 0x4);
	if (IS_ERR(wake_timer_res->timer_count_addr)) {
		return PTR_ERR(wake_timer_res->timer_count_addr);
	}

	wake_timer_res->timer_eb_addr = devm_ioremap(&pdev->dev, TIMER_EB_ADDR, 0x4);
	if (IS_ERR(wake_timer_res->timer_eb_addr)) {
		return PTR_ERR(wake_timer_res->timer_eb_addr);
	}

	irq = platform_get_irq_byname(pdev, "wake_int");
	if (irq < 0) {
		return -ENODATA;
	}

	ret = devm_request_irq(&pdev->dev, irq, wake_int_handler, IRQF_TRIGGER_HIGH, "wake_int", NULL);
	if (ret) {
		return -ENOMEM;
	}

	wake_timer_res->ws = wakeup_source_register("wake_timer");
	if (!wake_timer_res->ws) {
		pr_err("%s: create wake_timer wakeup source fail\n", __func__);
		return -ENODATA;
	}

	wake_timer_root = proc_mkdir(PROC_NODE_ROOT_NAME, NULL);
	if (wake_timer_root == NULL) {
		return -ENODATA;
	}

	proc_create_data(PROC_TIMER_SET, 0644, wake_timer_root,
			 &ax_wake_timer_fsops, NULL);

	proc_create_data(PROC_EB_TIMER_SET, 0644, wake_timer_root,
			 &ax_wake_timer_eb_fsops, NULL);

	wake_timer_res->major = register_chrdev(0, TIMER32_WAKEUP_NAME, &timer32_fops);
	if (wake_timer_res->major < 0)
		return -ENODEV;

	wake_timer_res->timer32_class = class_create(THIS_MODULE, TIMER32_WAKEUP_NAME);
	if (IS_ERR(wake_timer_res->timer32_class)) {
		unregister_chrdev(wake_timer_res->major, TIMER32_WAKEUP_NAME);
		return PTR_ERR(wake_timer_res->timer32_class);
	}

	init_waitqueue_head(&wq);
	atomic_set(&wq_status, 0);

	timer32_dev = device_create(wake_timer_res->timer32_class, NULL, MKDEV(wake_timer_res->major,0), NULL,
			"%s", TIMER32_WAKEUP_NAME);
	if (IS_ERR(timer32_dev)) {
		class_destroy(wake_timer_res->timer32_class);
		unregister_chrdev(wake_timer_res->major, TIMER32_WAKEUP_NAME);
		return PTR_ERR(timer32_dev);
	}

	return 0;
}

static int axera_wake_timer_remove(struct platform_device *pdev)
{
	remove_proc_entry(PROC_TIMER_SET, wake_timer_root);
	remove_proc_entry(PROC_NODE_ROOT_NAME, NULL);
	device_destroy(wake_timer_res->timer32_class, MKDEV(wake_timer_res->major, 0));
	class_destroy(wake_timer_res->timer32_class);
	unregister_chrdev(wake_timer_res->major, TIMER32_WAKEUP_NAME);

	return 0;
}

static const struct of_device_id axera_wake_timer_of_id_table[] = {
	{ .compatible = "axera,wake-timer" },
	{}
};
MODULE_DEVICE_TABLE(of, axera_wake_timer_of_id_table);

static struct platform_driver axera_wake_timer_driver = {
	.probe	= axera_wake_timer_probe,
	.remove = axera_wake_timer_remove,
	.driver = {
		.name = KBUILD_MODNAME,
		.of_match_table = axera_wake_timer_of_id_table,
	},
};

module_platform_driver(axera_wake_timer_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("axera wake timer driver");