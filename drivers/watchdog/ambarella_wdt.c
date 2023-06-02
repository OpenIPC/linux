/*
 * linux/drivers/mmc/host/ambarella_sd.c
 *
 * Copyright (C) 2006-2007, Ambarella, Inc.
 *  Anthony Ginger, <hfjiang@ambarella.com>
 *
 * Ambarella Media Processor Watch Dog Timer
 *
 */

#include <linux/module.h>
#include <linux/watchdog.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <plat/wdt.h>
#include <plat/rct.h>


#define AMBARELLA_WDT_MAX_CYCLE		0xffffffff

static int heartbeat = -1; /* -1 means get value from FDT */
module_param(heartbeat, int, 0);
MODULE_PARM_DESC(heartbeat, "Initial watchdog heartbeat in seconds");

static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started (default="
				__MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

struct ambarella_wdt {
	void __iomem	 		*regbase;
	struct clk			*clk;
	int				irq;
	struct watchdog_device		wdd;
	bool				enabled;
	u32				timeout; /* in cycle */
};

static int ambarella_wdt_start(struct watchdog_device *wdd)
{
	struct ambarella_wdt *ambwdt;
	u32 ctrl_val;

	ambwdt = watchdog_get_drvdata(wdd);
	ambwdt->enabled = true;

	ctrl_val = ambwdt->irq > 0 ? WDOG_CTR_INT_EN : WDOG_CTR_RST_EN;
	ctrl_val |= WDOG_CTR_EN;

	if (ambwdt->irq > 0)
		amba_writel(ambwdt->regbase + WDOG_RST_WD_OFFSET, 0x00);
	else
		amba_writel(ambwdt->regbase + WDOG_RST_WD_OFFSET, 0xFF);

	amba_writel(ambwdt->regbase + WDOG_CONTROL_OFFSET, ctrl_val);

	return 0;
}

static int ambarella_wdt_stop(struct watchdog_device *wdd)
{
	struct ambarella_wdt *ambwdt;

	ambwdt = watchdog_get_drvdata(wdd);
	ambwdt->enabled = false;

	amba_writel(ambwdt->regbase + WDOG_CONTROL_OFFSET, 0);

	return 0;
}

static int ambarella_wdt_keepalive(struct watchdog_device *wdd)
{
	struct ambarella_wdt *ambwdt = watchdog_get_drvdata(wdd);

	amba_writel(ambwdt->regbase + WDOG_RELOAD_OFFSET, ambwdt->timeout);
	amba_writel(ambwdt->regbase + WDOG_RESTART_OFFSET, WDT_RESTART_VAL);

	return 0;
}

static int ambarella_wdt_set_timeout(struct watchdog_device *wdd, u32 time)
{
	struct ambarella_wdt *ambwdt = watchdog_get_drvdata(wdd);
	u32 freq;

	wdd->timeout = time;

	freq = clk_get_rate(clk_get(NULL, "gclk_apb"));
	ambwdt->timeout = time * freq;
	amba_writel(ambwdt->regbase + WDOG_RELOAD_OFFSET, ambwdt->timeout);

	return 0;
}

static irqreturn_t ambarella_wdt_irq(int irq, void *devid)
{
	struct ambarella_wdt *ambwdt = devid;

	amba_writel(ambwdt->regbase + WDOG_CLR_TMO_OFFSET, 0x01);

	dev_info(ambwdt->wdd.dev, "Watchdog timer expired!\n");

	return IRQ_HANDLED;
}

static const struct watchdog_info ambwdt_info = {
	.options = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
	.identity = "Ambarella Watchdog",
};

static const struct watchdog_ops ambwdt_ops = {
	.owner		= THIS_MODULE,
	.start		= ambarella_wdt_start,
	.stop		= ambarella_wdt_stop,
	.ping		= ambarella_wdt_keepalive,
	.set_timeout	= ambarella_wdt_set_timeout,
};

static int ambarella_wdt_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct ambarella_wdt *ambwdt;
	struct resource *mem;
	void __iomem *reg;
	int max_timeout, rval = 0;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem == NULL) {
		dev_err(&pdev->dev, "Get WDT mem resource failed!\n");
		return -ENXIO;
	}

	reg = devm_ioremap(&pdev->dev, mem->start, resource_size(mem));
	if (!reg) {
		dev_err(&pdev->dev, "devm_ioremap() failed\n");
		return -ENOMEM;
	}

	ambwdt = devm_kzalloc(&pdev->dev, sizeof(*ambwdt), GFP_KERNEL);
	if (ambwdt == NULL) {
		dev_err(&pdev->dev, "Out of memory!\n");
		return -ENOMEM;
	}

	ambwdt->regbase = reg;

	/* irq < 0 means to disable interrupt mode */
	ambwdt->irq = platform_get_irq(pdev, 0);
	if (ambwdt->irq > 0) {
		rval = devm_request_irq(&pdev->dev, ambwdt->irq,
					ambarella_wdt_irq, IRQF_TRIGGER_RISING,
					dev_name(&pdev->dev), ambwdt);
		if (rval < 0) {
			dev_err(&pdev->dev, "Request IRQ failed!\n");
			return -ENXIO;
		}
	}

	ambwdt->clk = clk_get(NULL, "gclk_apb");
	if (IS_ERR(ambwdt->clk)) {
		dev_err(&pdev->dev, "no clk for wdt!\n");
		return -ENODEV;
	}

	max_timeout = AMBARELLA_WDT_MAX_CYCLE / clk_get_rate(ambwdt->clk);

	ambwdt->wdd.info = &ambwdt_info;
	ambwdt->wdd.ops = &ambwdt_ops;
	ambwdt->wdd.min_timeout = 0;
	ambwdt->wdd.max_timeout = max_timeout;
	ambwdt->wdd.bootstatus = amba_readl(WDT_RST_L_REG) ? 0 : WDIOF_CARDRESET;

	if (!of_find_property(np, "amb,non-bootstatus", NULL)) {
		/* WDT_RST_L_REG cannot be restored by "reboot" command,
		 * so reset it manually */
		amba_setbitsl(UNLOCK_WDT_RST_L_REG, UNLOCK_WDT_RST_L_VAL);
		amba_writel(WDT_RST_L_REG, 0x1);
		amba_clrbitsl(UNLOCK_WDT_RST_L_REG, UNLOCK_WDT_RST_L_VAL);
	}

	watchdog_init_timeout(&ambwdt->wdd, heartbeat, &pdev->dev);
	watchdog_set_nowayout(&ambwdt->wdd, nowayout);
	watchdog_set_drvdata(&ambwdt->wdd, ambwdt);

	ambarella_wdt_set_timeout(&ambwdt->wdd, ambwdt->wdd.timeout);
	ambarella_wdt_stop(&ambwdt->wdd);

	rval = watchdog_register_device(&ambwdt->wdd);
	if (rval < 0) {
		dev_err(&pdev->dev, "failed to register wdt!\n");
		return rval;
	}

	platform_set_drvdata(pdev, ambwdt);

	dev_notice(&pdev->dev, "Ambarella Watchdog Timer Probed.\n");

	return 0;
}

static int ambarella_wdt_remove(struct platform_device *pdev)
{
	struct ambarella_wdt *ambwdt;
	int rval = 0;

	ambwdt = platform_get_drvdata(pdev);

	ambarella_wdt_stop(&ambwdt->wdd);
	watchdog_unregister_device(&ambwdt->wdd);

	dev_notice(&pdev->dev, "Remove Ambarella Watchdog Timer.\n");

	return rval;
}

static void ambarella_wdt_shutdown(struct platform_device *pdev)
{
	struct ambarella_wdt *ambwdt;

	ambwdt = platform_get_drvdata(pdev);
	ambarella_wdt_stop(&ambwdt->wdd);

	dev_info(&pdev->dev, "%s @ %d.\n", __func__, system_state);
}

#ifdef CONFIG_PM

static int ambarella_wdt_suspend(struct platform_device *pdev,
			pm_message_t state)
{
	struct ambarella_wdt *ambwdt;
	int rval = 0;

	ambwdt = platform_get_drvdata(pdev);

	if (watchdog_active(&ambwdt->wdd))
		ambarella_wdt_stop(&ambwdt->wdd);

	dev_dbg(&pdev->dev, "%s exit with %d @ %d\n",
				__func__, rval, state.event);

	return rval;
}

static int ambarella_wdt_resume(struct platform_device *pdev)
{
	struct ambarella_wdt *ambwdt;
	int rval = 0;

	ambwdt = platform_get_drvdata(pdev);

	if (watchdog_active(&ambwdt->wdd)) {
		ambarella_wdt_start(&ambwdt->wdd);
		ambarella_wdt_keepalive(&ambwdt->wdd);
	}

	dev_dbg(&pdev->dev, "%s exit with %d\n", __func__, rval);

	return rval;
}
#endif

static const struct of_device_id ambarella_wdt_dt_ids[] = {
	{.compatible = "ambarella,wdt", },
	{},
};
MODULE_DEVICE_TABLE(of, ambarella_wdt_dt_ids);

static struct platform_driver ambarella_wdt_driver = {
	.probe		= ambarella_wdt_probe,
	.remove		= ambarella_wdt_remove,
	.shutdown	= ambarella_wdt_shutdown,
#ifdef CONFIG_PM
	.suspend	= ambarella_wdt_suspend,
	.resume		= ambarella_wdt_resume,
#endif
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "ambarella-wdt",
		.of_match_table = ambarella_wdt_dt_ids,
	},
};

module_platform_driver(ambarella_wdt_driver);

MODULE_DESCRIPTION("Ambarella Media Processor Watch Dog Timer");
MODULE_AUTHOR("Anthony Ginger, <hfjiang@ambarella.com>");
MODULE_LICENSE("GPL");

