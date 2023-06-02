/*
 * drivers/char/watchdog/davinci_wdt.c
 *
 * Watchdog driver for DaVinci DM644x/DM646x processors
 *
 * Copyright (C) 2006-2013 Texas Instruments.
 *
 * 2007 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/watchdog.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <plat/wdt_reg.h>

#define DRV_VERSION		"1.01.040"

/*The WDT range is from 1s to 89.478 s*/
#define NVT_WDT_DEFAULT_TIME 80 /*s*/
#define NVT_WDT_MAX_TIME 89     /*s*/

/*trigger sw timeout counter*/
#define WDT_SW_TIMEOUT	5

static int wdt_time = NVT_WDT_DEFAULT_TIME;
module_param(wdt_time, int, 0);
MODULE_PARM_DESC(wdt_time, "wdt default time in sec");

static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started (default="
			__MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

static int wdt_isr = 0;
module_param(wdt_isr, int, 0);
MODULE_PARM_DESC(wdt_time, "enable wdt isr mode");

struct nvt_wdt_device {
	void __iomem            *base;
	struct clk              *clk;
	struct watchdog_device  wdd;
	spinlock_t              wdt_lock;
};

static unsigned long lock_flags;
#define WDT_GETREG(wdt_dev, ofs)            readl((void __iomem *)(wdt_dev->base + ofs))
#define WDT_SETREG(wdt_dev, ofs, value)     writel(value, (void __iomem *)(wdt_dev->base + ofs))

static struct nvt_wdt_device *global_nvt_wdt = NULL;

static irqreturn_t wdt_irq(int irq, void *devid)
{
	struct nvt_wdt_device *nvt_wdt = devid;
	union WDT_STS_REG status_reg;

	status_reg.reg = WDT_GETREG(nvt_wdt, WDT_STS_REG_OFS);
	WDT_SETREG(nvt_wdt, WDT_STS_REG_OFS, status_reg.reg);

	return IRQ_HANDLED;
}

/*
 * Trigger Manual Reset.
 */
void nvt_trigger_manual_reset(void)
{
	union WDT_MANUAL_RST_REG manual_rst_reg;
	union WDT_CTRL_REG ctrl_reg;
	union WDT_STS_REG status_reg;
	int timeout;

	/*Enter critical section*/
	spin_lock_irqsave(&global_nvt_wdt->wdt_lock, lock_flags);

	/*Disable WDT First*/
	ctrl_reg.reg = WDT_GETREG(global_nvt_wdt, WDT_CTRL_REG_OFS);
	ctrl_reg.bit.enable = 0;
	ctrl_reg.bit.key_ctrl = WDT_KEY_VALUE;
	WDT_SETREG(global_nvt_wdt, WDT_CTRL_REG_OFS, ctrl_reg.reg);

	/*Wait for WDT is really disabled*/
	timeout = 0;
	while (timeout < WDT_SW_TIMEOUT) {
		status_reg.reg = WDT_GETREG(global_nvt_wdt, WDT_STS_REG_OFS);
		if (status_reg.bit.en_status == 0)
			break;
		udelay(1);
		timeout++;
	}
	if (timeout >= WDT_SW_TIMEOUT) {
		/*Leave critical section*/
		spin_unlock_irqrestore(&global_nvt_wdt->wdt_lock, lock_flags);
		printk("%s: wait WDT dis timeout\r\n", __func__);
		return;
	}

	/*Set external reset*/
	ctrl_reg.reg = WDT_GETREG(global_nvt_wdt, WDT_CTRL_REG_OFS);
	ctrl_reg.bit.mode       = WDT_MODE_RESET;
	ctrl_reg.bit.key_ctrl   = WDT_KEY_VALUE;
	ctrl_reg.bit.ext_reset  = 0x1;
	ctrl_reg.bit.msb = 0x1;
	WDT_SETREG(global_nvt_wdt, WDT_CTRL_REG_OFS, ctrl_reg.reg);

	/*wait 1T time before set WDT_EN*/
	udelay(80);

	/*Enable WDT*/
	ctrl_reg.reg = WDT_GETREG(global_nvt_wdt, WDT_CTRL_REG_OFS);
	ctrl_reg.bit.enable = 1;
	ctrl_reg.bit.key_ctrl = WDT_KEY_VALUE;
	WDT_SETREG(global_nvt_wdt, WDT_CTRL_REG_OFS, ctrl_reg.reg);

	/*Wait for WDT is really enabled*/
	timeout = 0;
	while (timeout < WDT_SW_TIMEOUT) {
		status_reg.reg = WDT_GETREG(global_nvt_wdt, WDT_STS_REG_OFS);
		if (status_reg.bit.en_status == 1)
			break;
		udelay(1);
		timeout++;
	}
	if (timeout >= WDT_SW_TIMEOUT) {
		/*Leave critical section*/
		spin_unlock_irqrestore(&global_nvt_wdt->wdt_lock, lock_flags);
		printk("%s: wait WDT en timeout\r\n", __func__);
		return;
	}

	manual_rst_reg.reg = WDT_GETREG(global_nvt_wdt, WDT_MANUAL_RST_REG_OFS);
	manual_rst_reg.bit.manual_rst = 0x1;
	WDT_SETREG(global_nvt_wdt, WDT_MANUAL_RST_REG_OFS, manual_rst_reg.reg);

	/*Leave critical section*/
	spin_unlock_irqrestore(&global_nvt_wdt->wdt_lock, lock_flags);
}

/*
 * Change the watchdog time interval.
 */
static int nvt_wdt_settimeout(struct nvt_wdt_device *nvt_wdt, int new_time)
{
	uint32_t clk_value;
	union WDT_CTRL_REG ctrl_reg;

	clk_value = (new_time * WDT_SOURCE_CLOCK) >> 12;
	if (clk_value > WDT_MSB_MAX) {
		pr_err("Invalid value (%d)\n", new_time);
		clk_value = WDT_MSB_MAX;
		nvt_wdt->wdd.timeout = wdt_time;
		return -EINVAL;
	}

	/*Enter critical section*/
	spin_lock_irqsave(&nvt_wdt->wdt_lock, lock_flags);

	ctrl_reg.reg = WDT_GETREG(nvt_wdt, WDT_CTRL_REG_OFS);
	ctrl_reg.bit.msb = clk_value;
	ctrl_reg.bit.key_ctrl = WDT_KEY_VALUE;
	WDT_SETREG(nvt_wdt, WDT_CTRL_REG_OFS, ctrl_reg.reg);

	/*Leave critical section*/
	spin_unlock_irqrestore(&nvt_wdt->wdt_lock, lock_flags);

	nvt_wdt->wdd.timeout = new_time;

	return 0;
}

/*
 * Disable the watchdog.
 */
static int nvt_wdt_stop(struct watchdog_device *wdd)
{
	struct nvt_wdt_device *nvt_wdt = watchdog_get_drvdata(wdd);
	union WDT_CTRL_REG ctrl_reg;
	union WDT_STS_REG status_reg;
	int timeout;

	/*Enter critical section*/
	spin_lock_irqsave(&nvt_wdt->wdt_lock, lock_flags);

	/*Enable WDT*/
	ctrl_reg.reg = WDT_GETREG(nvt_wdt, WDT_CTRL_REG_OFS);
	ctrl_reg.bit.enable = 0;
	ctrl_reg.bit.key_ctrl = WDT_KEY_VALUE;
	WDT_SETREG(nvt_wdt, WDT_CTRL_REG_OFS, ctrl_reg.reg);

	/*Wait for WDT is really enabled*/
	timeout = 0;
	while (timeout < WDT_SW_TIMEOUT) {
		status_reg.reg = WDT_GETREG(nvt_wdt, WDT_STS_REG_OFS);
		if (status_reg.bit.en_status == 0)
			break;
		udelay(1);
		timeout++;
	}
	if (timeout >= WDT_SW_TIMEOUT) {
		printk("%s: wait WDT dis timeout\r\n", __func__);
	}

	/*Leave critical section*/
	spin_unlock_irqrestore(&nvt_wdt->wdt_lock, lock_flags);

	return 0;
}

/*
 * Enable and reset the watchdog.
 */
static int nvt_wdt_start(struct watchdog_device *wdd)
{
	struct nvt_wdt_device *nvt_wdt = watchdog_get_drvdata(wdd);
	union WDT_CTRL_REG ctrl_reg;
	union WDT_STS_REG status_reg;
	int timeout;

	nvt_wdt_settimeout(nvt_wdt, wdd->timeout);

	/*Enter critical section*/
	spin_lock_irqsave(&nvt_wdt->wdt_lock, lock_flags);

	/*Enable WDT*/
	ctrl_reg.reg = WDT_GETREG(nvt_wdt, WDT_CTRL_REG_OFS);
	ctrl_reg.bit.enable = 1;
	ctrl_reg.bit.key_ctrl = WDT_KEY_VALUE;
	WDT_SETREG(nvt_wdt, WDT_CTRL_REG_OFS, ctrl_reg.reg);

	/*Wait for WDT is really enabled*/
	timeout = 0;
	while (timeout < WDT_SW_TIMEOUT) {
		status_reg.reg = WDT_GETREG(nvt_wdt, WDT_STS_REG_OFS);
		if (status_reg.bit.en_status == 1)
			break;
		udelay(1);
		timeout++;
	}
	if (timeout >= WDT_SW_TIMEOUT) {
		printk("%s: wait WDT en timeout\r\n", __func__);
	}

	/*Leave critical section*/
	spin_unlock_irqrestore(&nvt_wdt->wdt_lock, lock_flags);

	return 0;
}

/*
 * Reload the watchdog timer.  (ie, pat the watchdog)
 */
static int nvt_wdt_reload(struct watchdog_device *wdd)
{
	struct nvt_wdt_device *nvt_wdt = watchdog_get_drvdata(wdd);
	union WDT_TRIG_REG trig_reg;
	int timeout;

	/*Wait for previous trigger done*/
	timeout = 0;
	while (timeout < WDT_SW_TIMEOUT) {
		trig_reg.reg = WDT_GETREG(nvt_wdt, WDT_TRIG_REG_OFS);
		if (trig_reg.bit.trigger == 0)
			break;
		udelay(1);
		timeout++;
	}
	if (timeout >= WDT_SW_TIMEOUT) {
		printk("%s: wait trigger timeout\r\n", __func__);
		return -ETXTBSY;
	}

	/*Trigger WDT*/
	trig_reg.bit.trigger = 1;
	WDT_SETREG(nvt_wdt, WDT_TRIG_REG_OFS, trig_reg.reg);

	return 0;
}


static unsigned int nvt_wdt_get_timeleft(struct watchdog_device *wdd)
{
	u64 timer_counter;
	struct nvt_wdt_device *nvt_wdt = watchdog_get_drvdata(wdd);
	union WDT_STS_REG status_reg;

	status_reg.reg = WDT_GETREG(nvt_wdt, WDT_STS_REG_OFS);
	timer_counter = (u64)status_reg.bit.cnt;
	do_div(timer_counter, WDT_SOURCE_CLOCK);

	return timer_counter;
}

static int nvt_wdt_set_timeout(struct watchdog_device *wdd, unsigned int timeout)
{
	wdd->timeout = timeout;

	nvt_wdt_stop(wdd);

	nvt_wdt_start(wdd);

	return 0;
}

/* ......................................................................... */

static const struct watchdog_info nvt_wdt_info = {
	.identity = "Novatek IVOT Watchdog",
	.options = WDIOF_SETTIMEOUT |
			WDIOF_KEEPALIVEPING |
			WDIOF_MAGICCLOSE,
};

static const struct watchdog_ops nvt_wdt_ops = {
	.owner		= THIS_MODULE,
	.start		= nvt_wdt_start,
	.stop		= nvt_wdt_stop,
	.ping		= nvt_wdt_reload,
	.get_timeleft	= nvt_wdt_get_timeleft,
	.set_timeout	= nvt_wdt_set_timeout,
};

static void nvt_wdt_init(struct nvt_wdt_device *nvt_wdt)
{
	union WDT_CTRL_REG ctrl_reg;

	/*Enter critical section*/
	spin_lock_irqsave(&nvt_wdt->wdt_lock, lock_flags);

	/*Set the WDT as reset mode*/
	ctrl_reg.reg = 0;
	ctrl_reg.bit.mode       = WDT_MODE_RESET;
	ctrl_reg.bit.key_ctrl   = WDT_KEY_VALUE;
	ctrl_reg.bit.rst_num0_en = 1;
#ifdef CONFIG_NA51055_WATCHDOG_EXTERNAL_RESET
	ctrl_reg.bit.ext_reset  = 0x1;
#endif
	if (wdt_isr)
		ctrl_reg.bit.mode = WDT_MODE_INT;

	WDT_SETREG(nvt_wdt, WDT_CTRL_REG_OFS, ctrl_reg.reg);

	/*Leave critical section*/
	spin_unlock_irqrestore(&nvt_wdt->wdt_lock, lock_flags);
}

static int nvt_wdt_probe(struct platform_device *pdev)
{
	int res;
	struct device *dev = &pdev->dev;
	struct resource *memres = NULL;
	struct nvt_wdt_device *nvt_wdt;

	nvt_wdt = devm_kzalloc(dev, sizeof(*nvt_wdt), GFP_KERNEL);
	if (!nvt_wdt)
		return -ENOMEM;

	global_nvt_wdt = nvt_wdt;

	nvt_wdt->clk = devm_clk_get(dev, dev_name(dev));
	if (WARN_ON(IS_ERR(nvt_wdt->clk)))
		return PTR_ERR(nvt_wdt->clk);

	clk_prepare_enable(nvt_wdt->clk);

	platform_set_drvdata(pdev, nvt_wdt);

	memres = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	nvt_wdt->base = devm_ioremap_resource(dev, memres);
	if (IS_ERR(nvt_wdt->base))
		return PTR_ERR(nvt_wdt->base);

	nvt_wdt->wdd.info = &nvt_wdt_info;
	nvt_wdt->wdd.ops = &nvt_wdt_ops;
	nvt_wdt->wdd.min_timeout = 1;
	nvt_wdt->wdd.max_timeout = NVT_WDT_MAX_TIME;
	nvt_wdt->wdd.timeout = wdt_time;
	nvt_wdt->wdd.parent = &pdev->dev;

	spin_lock_init(&nvt_wdt->wdt_lock);

	nvt_wdt_init(nvt_wdt);

	if (wdt_isr) {
		int irq_num = 0;

		irq_num = platform_get_irq(pdev, 0);
		if (irq_num < 0) {
			dev_err(&pdev->dev, "no IRQ resource defined\n");
			return -ENXIO;
		}

		res = request_irq(irq_num, wdt_irq, IRQF_SHARED, pdev->name, nvt_wdt);
		if (res < 0) {
			dev_err(&pdev->dev, "failed to request IRQ\n");
			return -ENXIO;
		}
	}

	watchdog_init_timeout(&nvt_wdt->wdd, wdt_time, dev);
	watchdog_set_drvdata(&nvt_wdt->wdd, nvt_wdt);
	watchdog_set_nowayout(&nvt_wdt->wdd, nowayout);

	res = watchdog_register_device(&nvt_wdt->wdd);
	if (res < 0)
		dev_err(dev, "cannot register watchdog device\n");

	return res;
}

static int nvt_wdt_remove(struct platform_device *pdev)
{
	struct nvt_wdt_device *nvt_wdt = platform_get_drvdata(pdev);

	watchdog_unregister_device(&nvt_wdt->wdd);
	clk_disable_unprepare(nvt_wdt->clk);

	return 0;
}

static void nvt_wdt_shutdown(struct platform_device *pdev)
{
	struct nvt_wdt_device *nvt_wdt = platform_get_drvdata(pdev);

	if (watchdog_active(&nvt_wdt->wdd))
		nvt_wdt_stop(&nvt_wdt->wdd);
}

#ifdef CONFIG_PM

static int nvt_wdt_suspend(struct platform_device *pdev,
			      pm_message_t message)
{
	struct nvt_wdt_device *nvt_wdt = platform_get_drvdata(pdev);
	nvt_wdt_stop(&nvt_wdt->wdd);
	return 0;
}

static int nvt_wdt_resume(struct platform_device *pdev)
{
	struct nvt_wdt_device *nvt_wdt = platform_get_drvdata(pdev);
	nvt_wdt_start(&nvt_wdt->wdd);
	return 0;
}

#else
#define nvt_wdt_suspend NULL
#define nvt_wdt_resume	NULL
#endif

#ifdef CONFIG_OF
static const struct of_device_id nvt_wdt_match[] = {
	{ .compatible = "nvt,nvt_wdt" },
	{},
};

MODULE_DEVICE_TABLE(of, nvt_wdt_match);
#endif

static struct platform_driver nvt_wdt_driver = {
	.probe		= nvt_wdt_probe,
	.remove		= nvt_wdt_remove,
	.shutdown	= nvt_wdt_shutdown,
	.suspend	= nvt_wdt_suspend,
	.resume		= nvt_wdt_resume,
	.driver		= {
		.name	= "nvt_wdt",
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = nvt_wdt_match,
#endif
	},
};

module_platform_driver(nvt_wdt_driver);

MODULE_AUTHOR("Howard Chang @ Novatek");
MODULE_DESCRIPTION("Watchdog driver for Novatek");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
MODULE_ALIAS("platform:nvt_wdt");

