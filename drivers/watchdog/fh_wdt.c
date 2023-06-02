/*
 * Copyright 2010-2011 Picochip Ltd., Jamie Iles
 * http://www.picochip.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This file implements a driver for the Synopsys DesignWare watchdog device
 * in the many ARM subsystems. The watchdog has 16 different timeout periods
 * and these are a function of the input clock frequency.
 *
 * The DesignWare watchdog cannot be stopped once it has been started so we
 * use a software timer to implement a ping that will keep the watchdog alive.
 * If we receive an expected close for the watchdog then we keep the timer
 * running, otherwise the timer is stopped and the watchdog will expire.
 */
#define pr_fmt(fmt) "fh_wdt: " fmt

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/watchdog.h>
#include <linux/interrupt.h>
#include <mach/pmu.h>
#include <mach/fh_wdt.h>

#define WDT_RESPONSE_MODE

#define WDT_TIMER_MODE

#ifndef WDT_TIMER_MODE
#define WDT_NOTIMER_MODE
#endif

#define WDOG_CONTROL_REG_OFFSET					0x00
#define WDOG_CONTROL_REG_WDT_EN_MASK			0x01
#define WDOG_CONTROL_REG_RMOD_MASK				0x02
#define WDOG_TIMEOUT_RANGE_REG_OFFSET			0x04
#define WDOG_CURRENT_COUNT_REG_OFFSET			0x08
#define WDOG_COUNTER_RESTART_REG_OFFSET			0x0c
#define WDOG_COUNTER_RESTART_KICK_VALUE			0x76

/* Hardware timeout in seconds */
#define WDT_HW_TIMEOUT 2
/* User land timeout */
#define WDT_HEARTBEAT 15
static int heartbeat = WDT_HEARTBEAT;

/* The maximum TOP (timeout period) value that can be set in the watchdog. */
#define FH_WDT_MAX_TOP		15

static int curr_clk_rate;
static int nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, int, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started "
			"(default=" __MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

#define WDT_TIMEOUT		(HZ / 2)

struct fh_wdt_t fh_wdt;

static inline int fh_wdt_is_enabled(void)
{
	return readl(fh_wdt.regs + WDOG_CONTROL_REG_OFFSET) &
		WDOG_CONTROL_REG_WDT_EN_MASK;
}

#define WDT_CLOCK clk_get_rate(fh_wdt.clk)

static inline int fh_wdt_top_in_seconds(unsigned top)
{
	/*
	 * There are 16 possible timeout values in 0..15 where the number of
	 * cycles is 2 ^ (16 + i) and the watchdog counts down.
	 */
	return (1 << (16 + top)) / WDT_CLOCK;
}

static inline void fh_wdt_set_next_heartbeat(void)
{
	unsigned long flags;

	spin_lock_irqsave(&fh_wdt.lock, flags);
	fh_wdt.next_heartbeat = jiffies + heartbeat * HZ;
	spin_unlock_irqrestore(&fh_wdt.lock, flags);
}

static int fh_wdt_set_top(unsigned top_s)
{
	int i, val, top_val = FH_WDT_MAX_TOP;
	unsigned long flags;
	/*
	 * Iterate over the timeout values until we find the closest match. We
	 * always look for >=.
	 */
	spin_lock_irqsave(&fh_wdt.lock, flags);
	for (i = 0; i <= FH_WDT_MAX_TOP; ++i)
		if (fh_wdt_top_in_seconds(i) >= top_s) {
			top_val = i;
			break;
		}

	/* Set the new value in the watchdog. */
	printk("[wdt] set topval: %d", top_val);
	writel(top_val, fh_wdt.regs + WDOG_TIMEOUT_RANGE_REG_OFFSET);

	fh_wdt_set_next_heartbeat();

	val = fh_wdt_top_in_seconds(top_val);
	spin_unlock_irqrestore(&fh_wdt.lock, flags);
	return val;
}

#ifndef WDT_TIMER_MODE
static int fh_wdt_get_top(void)
{
	unsigned int val;

	val = readl(fh_wdt.regs + WDOG_TIMEOUT_RANGE_REG_OFFSET);
	val = (1 << (16 + val)) / WDT_CLOCK;
	return val;
}
#endif

static void fh_wdt_keepalive(void)
{
	unsigned long flags;

	spin_lock_irqsave(&fh_wdt.lock, flags);
	writel(WDOG_COUNTER_RESTART_KICK_VALUE, fh_wdt.regs +
			WDOG_COUNTER_RESTART_REG_OFFSET);
	spin_unlock_irqrestore(&fh_wdt.lock, flags);
}

static void fh_wdt_ping(unsigned long data)
{
	if (time_before(jiffies, fh_wdt.next_heartbeat) ||
		(!nowayout && !fh_wdt.in_use)) {
		fh_wdt_keepalive();
		mod_timer(&fh_wdt.timer, jiffies + WDT_TIMEOUT);
	} else
		pr_crit("keepalive missed, machine will reset\n");
}

static int fh_wdt_open(struct inode *inode, struct file *filp)
{
	unsigned long flags;

	if (test_and_set_bit(0, &fh_wdt.in_use))
		return -EBUSY;

	/* Make sure we don't get unloaded. */
	__module_get(THIS_MODULE);

	spin_lock_irqsave(&fh_wdt.lock, flags);

	if(fh_wdt.plat_data && fh_wdt.plat_data->resume)
		fh_wdt.plat_data->resume();

	fh_wdt_set_top(WDT_HW_TIMEOUT);
	if (!fh_wdt_is_enabled())
	{
		/*
		 * The watchdog is not currently enabled. Set the timeout to
		 * the maximum and then start it.
		 */
		u32 value;
		value = WDOG_CONTROL_REG_WDT_EN_MASK;
#ifdef WDT_RESPONSE_MODE
		value |= WDOG_CONTROL_REG_RMOD_MASK;
#endif
		writel(value, fh_wdt.regs + WDOG_CONTROL_REG_OFFSET);
		fh_wdt_keepalive();
	}
#ifdef WDT_TIMER_MODE
	fh_wdt_set_next_heartbeat();
#else
	del_timer(&fh_wdt.timer);
#endif
	spin_unlock_irqrestore(&fh_wdt.lock, flags);

	return nonseekable_open(inode, filp);
}

ssize_t fh_wdt_write(struct file *filp, const char __user *buf, size_t len,
			loff_t *offset)
{
	if (!len)
		return 0;

	if (!nowayout) {
		size_t i;

		fh_wdt.expect_close = 0;

		for (i = 0; i < len; ++i) {
			char c;

			if (get_user(c, buf + i))
				return -EFAULT;

			if (c == 'V') {
				fh_wdt.expect_close = 1;
				break;
			}
		}
	}

	fh_wdt_set_next_heartbeat();
	mod_timer(&fh_wdt.timer, jiffies + WDT_TIMEOUT);

	return len;
}

#ifndef WDT_TIMER_MODE
static u32 fh_wdt_time_left(void)
{
	return readl(fh_wdt.regs + WDOG_CURRENT_COUNT_REG_OFFSET) /
			WDT_CLOCK;
}
#endif

static const struct watchdog_info fh_wdt_ident = {
	.options	= WDIOF_KEEPALIVEPING | WDIOF_SETTIMEOUT |
			WDIOF_MAGICCLOSE,
	.identity	= "Synopsys DesignWare Watchdog",
};

static long fh_wdt_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned long val;

	switch (cmd) {
	case WDIOC_GETSUPPORT:
		return copy_to_user((struct watchdog_info *)arg, &fh_wdt_ident,
					sizeof(fh_wdt_ident)) ? -EFAULT : 0;

	case WDIOC_GETSTATUS:
	case WDIOC_GETBOOTSTATUS:
		return put_user(0, (int *)arg);

	case WDIOC_KEEPALIVE:
#ifdef WDT_TIMER_MODE
		fh_wdt_set_next_heartbeat();
#else
		fh_wdt_keepalive();
#endif
		return 0;

	case WDIOC_SETTIMEOUT:
		if (get_user(val, (int __user *)arg))
			return -EFAULT;
#ifdef WDT_TIMER_MODE
		pr_debug("[wdt] settime value %lu", val);
		heartbeat =  val;
		fh_wdt_keepalive();
		fh_wdt_set_next_heartbeat();
#else
		fh_wdt_set_top(val);
		fh_wdt_keepalive();
#endif
		return put_user(val , (int __user *)arg);

	case WDIOC_GETTIMEOUT:
#ifdef WDT_TIMER_MODE
		return put_user(heartbeat, (int __user *)arg);
#else
		if (get_user(val, (int __user *)arg))
			return -EFAULT;
		return put_user(fh_wdt_time_left(), (int __user *)arg);
#endif

	case WDIOC_GETTIMELEFT:
#ifdef WDT_TIMER_MODE
		val = (fh_wdt.next_heartbeat - fh_wdt.timer.expires) / HZ;
		return put_user(val, (int __user *)arg);
#else
	/* Get the time left in hardware if use hardware-mode*/
		if (get_user(val, (int __user *)arg))
			return -EFAULT;
		return put_user(fh_wdt_get_top(), (int __user *)arg);
#endif

	case WDIOC_SETOPTIONS:
		if (get_user(val, (int __user *)arg))
			return -EFAULT;

		if (val & WDIOS_DISABLECARD) {
			if(fh_wdt.plat_data && fh_wdt.plat_data->pause)
				fh_wdt.plat_data->pause();
			else
				return -EPERM;
		}

		if (val & WDIOS_ENABLECARD) {
			if(fh_wdt.plat_data && fh_wdt.plat_data->resume)
				fh_wdt.plat_data->resume();
			else
				return -EPERM;
		}

		return 0;

	default:
		return -ENOTTY;
	}
}

#ifdef WDT_RESPONSE_MODE
static irqreturn_t fh_wdt_interrupt(int this_irq, void *dev_id)
{
	struct fh_wdt_t *fh_wdt = (struct fh_wdt_t *)dev_id;

	if (fh_wdt && fh_wdt->plat_data->intr)
		return fh_wdt->plat_data->intr(fh_wdt);
	else
		return IRQ_HANDLED;
}
#endif

static int fh_wdt_release(struct inode *inode, struct file *filp)
{
	clear_bit(0, &fh_wdt.in_use);
#ifdef WDT_TIMER_MODE
	if (!fh_wdt.expect_close) {
		del_timer(&fh_wdt.timer);
		if (!nowayout)
			pr_crit("unexpected close, system will reboot soon\n");
		else
			pr_crit("watchdog cannot be disabled, system will reboot soon\n");
	}

	fh_wdt.expect_close = 0;
#endif
	return 0;
}

#ifdef CONFIG_PM
static int fh_wdt_suspend(struct device *dev)
{
	clk_disable(fh_wdt.clk);
	writel(FH_WDT_MAX_TOP, fh_wdt.regs + WDOG_TIMEOUT_RANGE_REG_OFFSET);
	curr_clk_rate = WDT_CLOCK;
	fh_wdt_keepalive();

	return 0;
}

static int fh_wdt_resume(struct device *dev)
{
	int err;

	clk_set_rate(fh_wdt.clk, curr_clk_rate);
	err = clk_enable(fh_wdt.clk);

	if (err)
	{
		pr_err("an error occured during wdt resume, error no: %d\n", err);
		return err;
	}

	fh_wdt_keepalive();

	return 0;
}

static const struct dev_pm_ops fh_wdt_pm_ops = {
	.suspend	= fh_wdt_suspend,
	.resume		= fh_wdt_resume,
};
#endif /* CONFIG_PM */

static const struct file_operations wdt_fops = {
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.open		= fh_wdt_open,
	.write		= fh_wdt_write,
	.unlocked_ioctl	= fh_wdt_ioctl,
	.release	= fh_wdt_release
};

static struct miscdevice fh_wdt_miscdev = {
	.fops		= &wdt_fops,
	.name		= "watchdog",
	.minor		= WATCHDOG_MINOR,
};

static int __devinit fh_wdt_drv_probe(struct platform_device *pdev)
{
	int ret, irq;
	struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (!mem)
		return -EINVAL;

	if (!devm_request_mem_region(&pdev->dev, mem->start, resource_size(mem),
					"fh_wdt"))
		return -ENOMEM;

	fh_wdt.regs = devm_ioremap(&pdev->dev, mem->start, resource_size(mem));
	if (!fh_wdt.regs)
		return -ENOMEM;
#ifdef WDT_RESPONSE_MODE
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "no irq resource\n");
		return -ENXIO;
	}

	ret = request_irq(irq, fh_wdt_interrupt, IRQF_DISABLED,\
			dev_name(&pdev->dev),
			&fh_wdt);
	if (ret) {
		dev_err(&pdev->dev, "failure requesting irq %i\n", irq);
		return -ENXIO;
	}
#endif

	fh_wdt.plat_data = dev_get_platdata(&pdev->dev);
	fh_wdt.clk = clk_get( NULL,"wdt_clk");
	if (IS_ERR(fh_wdt.clk))
		return PTR_ERR(fh_wdt.clk);

	clk_set_rate(fh_wdt.clk, 1000000);

	ret = clk_enable(fh_wdt.clk);
	if (ret)
		goto out_put_clk;

	spin_lock_init(&fh_wdt.lock);

	ret = misc_register(&fh_wdt_miscdev);
	if (ret)
		goto out_disable_clk;

	fh_wdt_set_next_heartbeat();
	setup_timer(&fh_wdt.timer, fh_wdt_ping, 0);
	mod_timer(&fh_wdt.timer, jiffies + WDT_TIMEOUT);

	return 0;

out_disable_clk:
	clk_disable(fh_wdt.clk);
out_put_clk:
	clk_put(fh_wdt.clk);

	return ret;
}

static int __devexit fh_wdt_drv_remove(struct platform_device *pdev)
{
	misc_deregister(&fh_wdt_miscdev);

	clk_disable(fh_wdt.clk);
	clk_put(fh_wdt.clk);

	return 0;
}

static struct platform_driver fh_wdt_driver = {
	.probe		= fh_wdt_drv_probe,
	.remove		= __devexit_p(fh_wdt_drv_remove),
	.driver		= {
		.name	= "fh_wdt",
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= &fh_wdt_pm_ops,
#endif /* CONFIG_PM */
	},
};

static int __init fh_wdt_watchdog_init(void)
{
	return platform_driver_register(&fh_wdt_driver);
}
module_init(fh_wdt_watchdog_init);

static void __exit fh_wdt_watchdog_exit(void)
{
	platform_driver_unregister(&fh_wdt_driver);
}
module_exit(fh_wdt_watchdog_exit);

MODULE_AUTHOR("fullhan");
MODULE_DESCRIPTION("Synopsys DesignWare Watchdog Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
