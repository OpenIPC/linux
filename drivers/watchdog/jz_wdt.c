/*
   * linux/drivers/watchdog/jz_wdt.c - Ingenic Watchdog Controller driver
   *
   * Copyright (C) 2015 Ingenic Semiconductor Co., Ltd.
   * Written by Mick <dongyue.ye@ingenic.com>.
   *
   * This program is free software; you can redistribute it and/or modify
   * it under the terms of the GNU General Public License version 2 as
   * published by the Free Software Foundation.
   */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <jz_proc.h>
#include <soc/base.h>
#include <soc/tcu.h>

#define JZ_REG_WDT_TIMER_DATA     0x0
#define JZ_REG_WDT_COUNTER_ENABLE 0x4
#define JZ_REG_WDT_TIMER_COUNTER  0x8
#define JZ_REG_WDT_TIMER_CONTROL  0xC

#define JZ_WDT_CLOCK_PCLK 0x1
#define JZ_WDT_CLOCK_RTC  0x2
#define JZ_WDT_CLOCK_EXT  0x4

#define JZ_WDT_CLOCK_DIV_SHIFT   3

#define JZ_WDT_CLOCK_DIV_1    (0 << JZ_WDT_CLOCK_DIV_SHIFT)
#define JZ_WDT_CLOCK_DIV_4    (1 << JZ_WDT_CLOCK_DIV_SHIFT)
#define JZ_WDT_CLOCK_DIV_16   (2 << JZ_WDT_CLOCK_DIV_SHIFT)
#define JZ_WDT_CLOCK_DIV_64   (3 << JZ_WDT_CLOCK_DIV_SHIFT)
#define JZ_WDT_CLOCK_DIV_256  (4 << JZ_WDT_CLOCK_DIV_SHIFT)
#define JZ_WDT_CLOCK_DIV_1024 (5 << JZ_WDT_CLOCK_DIV_SHIFT)

#define DEFAULT_HEARTBEAT 5
#define MAX_HEARTBEAT     2048

static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout,
		 "Watchdog cannot be stopped once started (default="
		 __MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

static unsigned int heartbeat = DEFAULT_HEARTBEAT;
module_param(heartbeat, uint, 0);
MODULE_PARM_DESC(heartbeat,
		"Watchdog heartbeat period in seconds from 1 to "
		__MODULE_STRING(MAX_HEARTBEAT) ", default "
		__MODULE_STRING(DEFAULT_HEARTBEAT));

struct jz_wdt_drvdata {
	struct watchdog_device wdt;
	void __iomem *base;
	struct clk *rtc_clk;
};

static int jz_wdt_ping(struct watchdog_device *wdt_dev)
{
	struct jz_wdt_drvdata *drvdata = watchdog_get_drvdata(wdt_dev);

	writew(0x0, drvdata->base + JZ_REG_WDT_TIMER_COUNTER);
	return 0;
}

static int jz_wdt_set_timeout(struct watchdog_device *wdt_dev,
				    unsigned int new_timeout)
{
	struct jz_wdt_drvdata *drvdata = watchdog_get_drvdata(wdt_dev);
	unsigned int rtc_clk_rate;
	unsigned int timeout_value;
	unsigned short clock_div = JZ_WDT_CLOCK_DIV_1;

	/*rtc_clk_rate = clk_get_rate(drvdata->rtc_clk);*/
	rtc_clk_rate = 24000000 / 512;// becasue OPCR.ERCS select EXT/512

	timeout_value = rtc_clk_rate * new_timeout;
	while (timeout_value > 0xffff) {
		if (clock_div == JZ_WDT_CLOCK_DIV_1024) {
			/* Requested timeout too high;
			* use highest possible value. */
			timeout_value = 0xffff;
			break;
		}
		timeout_value >>= 2;
		clock_div += (1 << JZ_WDT_CLOCK_DIV_SHIFT);
	}
	/*printk("timeout = %d,timeout_value = %d,clock_div = %d,rtc_clk_rate = %d ----------\n",new_timeout,timeout_value,clock_div,rtc_clk_rate);*/

	writeb(0x0, drvdata->base + JZ_REG_WDT_COUNTER_ENABLE);
	writew(clock_div, drvdata->base + JZ_REG_WDT_TIMER_CONTROL);

	writew((u16)timeout_value, drvdata->base + JZ_REG_WDT_TIMER_DATA);
	writew(0x0, drvdata->base + JZ_REG_WDT_TIMER_COUNTER);
	writew(clock_div | JZ_WDT_CLOCK_RTC,
		drvdata->base + JZ_REG_WDT_TIMER_CONTROL);

	writeb(0x1, drvdata->base + JZ_REG_WDT_COUNTER_ENABLE);

	wdt_dev->timeout = new_timeout;
	return 0;
}

static int jz_wdt_start(struct watchdog_device *wdt_dev)
{
	outl(1 << 16,TCU_IOBASE + TCU_TSCR);
	jz_wdt_set_timeout(wdt_dev, wdt_dev->timeout);

	return 0;
}

static int jz_wdt_stop(struct watchdog_device *wdt_dev)
{
	struct jz_wdt_drvdata *drvdata = watchdog_get_drvdata(wdt_dev);

	outl(1 << 16,TCU_IOBASE + TCU_TSCR);
	writew(0,drvdata->base + JZ_REG_WDT_TIMER_COUNTER);		//counter
	writew(65535,drvdata->base + JZ_REG_WDT_TIMER_DATA);	//data
	outl(1 << 16,TCU_IOBASE + TCU_TSSR);

	return 0;
}

static const struct watchdog_info jz_wdt_info = {
	.options = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
	.identity = "jz Watchdog",
};

static const struct watchdog_ops jz_wdt_ops = {
	.owner = THIS_MODULE,
	.start = jz_wdt_start,
	.stop = jz_wdt_stop,
	.ping = jz_wdt_ping,
	.set_timeout = jz_wdt_set_timeout,
};

static ssize_t watchdog_cmd_set(struct file *file, const char __user *buffer, size_t count, loff_t *f_pos);
static int watchdog_cmd_open(struct inode *inode, struct file *file);
static const struct file_operations watchdog_cmd_fops ={
	.read = seq_read,
	.open = watchdog_cmd_open,
	.llseek = seq_lseek,
	.release = single_release,
	.write = watchdog_cmd_set,
};



static int jz_wdt_probe(struct platform_device *pdev)
{
	struct jz_wdt_drvdata *drvdata;
	struct watchdog_device *jz_wdt;
	struct resource	*res;
	struct proc_dir_entry *proc;
	int ret;

	drvdata = devm_kzalloc(&pdev->dev, sizeof(struct jz_wdt_drvdata),
			       GFP_KERNEL);
	if (!drvdata) {
		dev_err(&pdev->dev, "Unable to alloacate watchdog device\n");
		return -ENOMEM;
	}

	if (heartbeat < 1 || heartbeat > MAX_HEARTBEAT)
		heartbeat = DEFAULT_HEARTBEAT;

	jz_wdt = &drvdata->wdt;
	jz_wdt->info = &jz_wdt_info;
	jz_wdt->ops = &jz_wdt_ops;
	jz_wdt->timeout = heartbeat;
	jz_wdt->min_timeout = 1;
	jz_wdt->max_timeout = MAX_HEARTBEAT;
	watchdog_set_nowayout(jz_wdt, nowayout);
	watchdog_set_drvdata(jz_wdt, drvdata);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	drvdata->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(drvdata->base)) {
		ret = PTR_ERR(drvdata->base);
		goto err_out;
	}

	drvdata->rtc_clk = clk_get(NULL, "pclk");
	if (IS_ERR(drvdata->rtc_clk)) {
		dev_err(&pdev->dev, "cannot find RTC clock\n");
		ret = PTR_ERR(drvdata->rtc_clk);
		goto err_out;
	}

	ret = watchdog_register_device(&drvdata->wdt);
	if (ret < 0)
		goto err_disable_clk;

	platform_set_drvdata(pdev, drvdata);

	/* proc info */
	proc = jz_proc_mkdir("watchdog");
	if (!proc) {
		printk("create mdio info failed!\n");
	}
	proc_create_data("reset", S_IRUGO, proc, &watchdog_cmd_fops, NULL);

	return 0;

err_disable_clk:
	clk_put(drvdata->rtc_clk);
err_out:
	return ret;
}

static int jz_wdt_remove(struct platform_device *pdev)
{
	struct jz_wdt_drvdata *drvdata = platform_get_drvdata(pdev);

	jz_wdt_stop(&drvdata->wdt);
	watchdog_unregister_device(&drvdata->wdt);
	clk_put(drvdata->rtc_clk);

	return 0;
}

static struct platform_driver jz_wdt_driver = {
	.probe = jz_wdt_probe,
	.remove = jz_wdt_remove,
	.driver = {
		.name = "jz-wdt",
		.owner	= THIS_MODULE,
	},
};

module_platform_driver(jz_wdt_driver);

MODULE_DESCRIPTION("jz Watchdog Driver");
MODULE_LICENSE("GPL");



/* cmd */
#define WATCHDOG_CMD_BUF_SIZE 100
static uint8_t watchdog_cmd_buf[100];
static int watchdog_cmd_show(struct seq_file *m, void *v)
{
	int len = 0;
	len += seq_printf(m ,"%s\n", watchdog_cmd_buf);
	return len;
}

static ssize_t watchdog_cmd_set(struct file *file, const char __user *buffer, size_t count, loff_t *f_pos)
{
    int cmd_value = 0;
    unsigned int reg04 = 0,reglc = 0;

	char *buf = kzalloc((count+1), GFP_KERNEL);
	if(!buf)
		return -ENOMEM;
	if(copy_from_user(buf, buffer, count))
	{
		kfree(buf);
		return EFAULT;
	}
	cmd_value = simple_strtoull(buf, NULL, 0);

	mdelay(1000);
	if(1 == cmd_value) {
		reg04 = inl(TCU_IOBASE + 0x04);
		if(reg04 & 0x1) {
			outl(0 << 0, TCU_IOBASE + 0x4);
		}

		outl(0x1a, TCU_IOBASE + 0xc);

		reglc = inl(TCU_IOBASE + 0x1c);
		if(reglc & 0x10000) {
			outl(1 << 16, TCU_IOBASE + 0x3c);
			outl(1 << 16, TCU_IOBASE + 0x2c);
		}
		outl(0x0000, TCU_IOBASE + 0x0);
		outl(0x0000, TCU_IOBASE + 0x8);

		reg04 = inl(TCU_IOBASE + 0x04);
		if(!(reg04 & 0x1)) {
			outl(1 << 0, TCU_IOBASE + 0x4);
		}
		printk("watchdog reboot system!!!\n");
	}

	kfree(buf);
	return count;
}
static int watchdog_cmd_open(struct inode *inode, struct file *file)
{
	return single_open_size(file, watchdog_cmd_show, PDE_DATA(inode),8192);
}
