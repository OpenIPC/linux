#define pr_fmt(fmt) KBUILD_MODNAME ":%s:%d: " fmt, __func__, __LINE__

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/watchdog.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/cpumask.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/poll.h>

#define WDOG_CONTROL_REG_EN                 0x00
#define WDOG_CONTROL_WDT_EN                 0x01
#define WDOG_CONTROL_WDT_DIS                0x00
#define WDOG_TIMEOUT_COUNT_REG              0x0C
#define WDOG_TIMEOUT_COUNT_CTRL_REG         0x18
#define WDOG_CONTROL_REG_INTR               0x54
#define WDOG_CONTROL_WDT_INTR_EN            0x01
#define WDOG_CONTROL_WDT_INTR_CLR           0x3C
#define WDOG_CURRENT_COUNT_REG              0x24
#define WDOG_COUNTER_RESTART_REG            0x30
#define WDOG_COUNTER_RESTART_KICK_VALUE     0x61696370

#define AX_MAX_COUNT                        0xFFFFFFFF
#define AX_WDT_DEFAULT_SECONDS              90
#define AX_PING_DELAY_US                    40
#define AX_WDT_CLK_32K                      32000
#define AX_WDT_CLK_24M                      24000000
#define PERI_SYS_BASE                       0x4870000
static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started "
		 "(default=" __MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

struct ax_wdt {
	void __iomem *regs;
	void __iomem *common;
	int irq;
	int suspend_status;
	u32 timeout;
	u32 udelay_time;
	u32 keep_alive;
	u32 rate;
	u32 clk_set[3];
	u32 aclk[3];
	u32 pclk[3];
	u32 arst[3];
	u32 prst[3];
	struct watchdog_device wdd;
	struct proc_dir_entry *pfile;
	int clk;
	int clk_wdt_select;
	char proc_name[32];
	struct mutex lock;
	/* Save/restore */
};

enum {
	SET_SET = 0,
	CLR_CLR,
	REG_SHIFT
};

static struct ax_wdt * g_ax_wdt[2];

#define to_ax_wdt(wdd)	container_of(wdd, struct ax_wdt, wdd)

static inline int ax_wdt_is_enabled(struct ax_wdt *ax_wdt)
{
	return readl(ax_wdt->regs + WDOG_CONTROL_REG_EN) & WDOG_CONTROL_WDT_EN;
}

static int ax_wdt_ping(struct watchdog_device *wdd)
{
	struct ax_wdt *ax_wdt = to_ax_wdt(wdd);
	writel(WDOG_COUNTER_RESTART_KICK_VALUE, ax_wdt->regs +
	       WDOG_COUNTER_RESTART_REG);
	udelay(ax_wdt->udelay_time);
	writel(0, ax_wdt->regs + WDOG_COUNTER_RESTART_REG);
	return 0;
}

static void ax_clk_set_rate(struct ax_wdt *ax_wdt, unsigned long int rate)
{
	if (rate == AX_WDT_CLK_32K) {
		ax_wdt->udelay_time = AX_PING_DELAY_US;
		writel(BIT(ax_wdt->clk_set[REG_SHIFT]), ax_wdt->common + ax_wdt->clk_set[CLR_CLR]);
	} else {
		ax_wdt->udelay_time = 1;
		writel(BIT(ax_wdt->clk_set[REG_SHIFT]), ax_wdt->common + ax_wdt->clk_set[SET_SET]);
	}
	ax_wdt->rate = rate;
	ax_wdt->wdd.min_timeout = DIV_ROUND_UP(0xFFFF, rate);
	ax_wdt->wdd.max_timeout = AX_MAX_COUNT / rate;
	ax_wdt->wdd.max_hw_heartbeat_ms = AX_MAX_COUNT / rate * 1000;
}

static void ax_wdt_reset_set(struct ax_wdt *ax_wdt, u8 en)
{
	if(en) {
		writel(BIT(ax_wdt->prst[REG_SHIFT]), ax_wdt->common + ax_wdt->prst[SET_SET]);
		writel(BIT(ax_wdt->arst[REG_SHIFT]), ax_wdt->common + ax_wdt->arst[SET_SET]);
	} else {
		writel(BIT(ax_wdt->arst[REG_SHIFT]), ax_wdt->common + ax_wdt->arst[CLR_CLR]);
		writel(BIT(ax_wdt->prst[REG_SHIFT]), ax_wdt->common + ax_wdt->prst[CLR_CLR]);
	}
}

// when clk disenable to en, 32K need delay 100us, 24M need delay 1us
static void ax_wdt_clk_set(struct ax_wdt *ax_wdt, u8 en, u32 rate)
{
	if(en) {
		writel(BIT(ax_wdt->pclk[REG_SHIFT]), ax_wdt->common + ax_wdt->pclk[SET_SET]);
		writel(BIT(ax_wdt->aclk[REG_SHIFT]), ax_wdt->common + ax_wdt->aclk[SET_SET]);
		if (rate == AX_WDT_CLK_32K)
			udelay(100);
		udelay(1);
	} else {
		writel(BIT(ax_wdt->aclk[REG_SHIFT]), ax_wdt->common + ax_wdt->aclk[CLR_CLR]);
		writel(BIT(ax_wdt->pclk[REG_SHIFT]), ax_wdt->common + ax_wdt->pclk[CLR_CLR]);
	}
}

static int ax_wdt_clk_enable(struct ax_wdt *ax_wdt, unsigned long int rate)
{
	ax_wdt_clk_set(ax_wdt, 0, rate);
	ax_clk_set_rate(ax_wdt, rate);
	ax_wdt_clk_set(ax_wdt, 1, rate);
	ax_wdt_ping(&ax_wdt->wdd);

	return 0;
}

static void ax_wdt_clk_disable(struct ax_wdt *ax_wdt)
{
	ax_wdt_clk_set(ax_wdt, 0, ax_wdt->rate);
	return;
}

static int __ax_wdt_set_timeout(struct ax_wdt *ax_wdt, unsigned long int rate,
				unsigned int timeout_s)
{
	unsigned long long count_val = 0;

	count_val = timeout_s * rate;
	count_val = count_val < AX_MAX_COUNT ? count_val >> 16 : AX_MAX_COUNT >> 16;
	writel(count_val, ax_wdt->regs + WDOG_TIMEOUT_COUNT_REG);
	writel(1, ax_wdt->regs + WDOG_TIMEOUT_COUNT_CTRL_REG);
	udelay(ax_wdt->udelay_time);
	writel(0, ax_wdt->regs + WDOG_TIMEOUT_COUNT_CTRL_REG);
	ax_wdt_ping(&ax_wdt->wdd);
	return 0;
}

static int ax_wdt_set_timeout(struct watchdog_device *wdd,
			      unsigned int timeout_s)
{
	struct ax_wdt *ax_wdt = to_ax_wdt(wdd);

	wdd->timeout = timeout_s;
	__ax_wdt_set_timeout(ax_wdt, ax_wdt->rate, timeout_s);

	return 0;
}

static irqreturn_t ax_wdt_interrupt(int irq, void *data)
{
	struct ax_wdt *ax_wdt = data;
	writel(1, ax_wdt->regs + WDOG_CONTROL_WDT_INTR_CLR);
	ax_wdt_ping(&ax_wdt->wdd);
	return IRQ_HANDLED;
}

static void ax_wdt_irq_enable(struct ax_wdt *ax_wdt)
{
	writel(WDOG_CONTROL_WDT_INTR_EN, ax_wdt->regs + WDOG_CONTROL_REG_INTR);
}

static int ax_wdt_enable(struct ax_wdt *ax_wdt, bool en)
{
	if (en) {
		writel(WDOG_CONTROL_WDT_EN, ax_wdt->regs + WDOG_CONTROL_REG_EN);
		ax_wdt_ping(&ax_wdt->wdd);
	} else {
		writel(WDOG_CONTROL_WDT_DIS, ax_wdt->regs + WDOG_CONTROL_REG_EN);
	}
	return 0;
}

static int ax_wdt_start(struct watchdog_device *wdd)
{
	struct ax_wdt *ax_wdt = to_ax_wdt(wdd);

	ax_wdt_set_timeout(wdd, wdd->timeout);
	ax_wdt_enable(ax_wdt, 1);

	return 0;
}

static int ax_wdt_stop(struct watchdog_device *wdd)
{
	struct ax_wdt *ax_wdt = to_ax_wdt(wdd);
	ax_wdt_enable(ax_wdt, 0);
	return 0;
}

static int ax_wdt_restart(struct watchdog_device *wdd,
			  unsigned long action, void *data)
{
	struct ax_wdt *ax_wdt = to_ax_wdt(wdd);
	//change clk to 24M, when reboot
	ax_wdt_clk_enable(ax_wdt, AX_WDT_CLK_24M);
	ax_wdt_set_timeout(wdd, 0);
	/* wait for reset to assert... */
	mdelay(500);
	return 0;
}

static unsigned int ax_wdt_get_timeleft(struct watchdog_device *wdd)
{
	struct ax_wdt *ax_wdt = to_ax_wdt(wdd);
	return readl(ax_wdt->regs + WDOG_CURRENT_COUNT_REG) / ax_wdt->rate;
}

static const struct watchdog_info ax_wdt_ident = {
	.options = WDIOF_KEEPALIVEPING | WDIOF_SETTIMEOUT | WDIOF_MAGICCLOSE,
	.identity = "Axera Watchdog",
};

static const struct watchdog_ops ax_wdt_ops = {
	.owner = THIS_MODULE,
	.start = ax_wdt_start,
	.stop = ax_wdt_stop,
	.ping = ax_wdt_ping,
	.set_timeout = ax_wdt_set_timeout,
	.get_timeleft = ax_wdt_get_timeleft,
	.restart = ax_wdt_restart,
};

#ifdef CONFIG_PM_SLEEP
static int ax_wdt_suspend(struct device *dev)
{
	struct ax_wdt *ax_wdt = dev_get_drvdata(dev);

	ax_wdt->suspend_status = readl(ax_wdt->regs + WDOG_CONTROL_REG_EN);
	if (ax_wdt->keep_alive) {
		__ax_wdt_set_timeout(ax_wdt, ax_wdt->rate, ax_wdt->keep_alive);
	} else {
		ax_wdt_enable(ax_wdt, 0);
	}
	return 0;
}

static int ax_wdt_resume(struct device *dev)
{
	struct ax_wdt *ax_wdt = dev_get_drvdata(dev);
	struct watchdog_device *wdd = &ax_wdt->wdd;

	__ax_wdt_set_timeout(ax_wdt, ax_wdt->rate, wdd->timeout);
	ax_wdt_enable(ax_wdt, !!ax_wdt->suspend_status);

	return 0;
}
#endif /* CONFIG_PM_SLEEP */
static SIMPLE_DEV_PM_OPS(ax_wdt_pm_ops, ax_wdt_suspend, ax_wdt_resume);

int ax_wdt_set_keep_alive_timeout(int wdt_id, unsigned int timeout)
{
	if (IS_ERR_OR_NULL(g_ax_wdt[wdt_id]))
		return -EINVAL;
	mutex_lock(&g_ax_wdt[wdt_id]->lock);
	g_ax_wdt[wdt_id]->keep_alive = timeout;
	mutex_unlock(&g_ax_wdt[wdt_id]->lock);
	return 0;
}
EXPORT_SYMBOL(ax_wdt_set_keep_alive_timeout);

static ssize_t ax_wdt_write(struct file *file, const char __user * userbuf,
			     size_t count, loff_t * data)
{
	char kbuf[16] = {0};
	unsigned long value;
	struct seq_file *seq = file->private_data;
	struct ax_wdt *ax_wdt = (struct ax_wdt *)seq->private;

	if (count > 11)
		return -EINVAL;
	if (copy_from_user(kbuf, userbuf, count)) {
		pr_err("copy_from_user fail\n");
		return -EFAULT;
	}
	kbuf[count] = '\0';
	if (kstrtoul(kbuf, 0, &value) != 0) {
		pr_err("kstrtoul fail value\n");
		return -EINVAL;
	}
	mutex_lock(&ax_wdt->lock);
	ax_wdt->keep_alive = value;
	mutex_unlock(&ax_wdt->lock);

	return count;
}

static int ax_wdt_stat_show(struct seq_file *seq, void *v)
{
	struct ax_wdt *ax_wdt = (struct ax_wdt *)seq->private;
	seq_printf(seq, "suspend timeout %d suspend reboot time %d\n", ax_wdt->keep_alive,
			ax_wdt->keep_alive * 2);
	return 0;
}

static int ax_wdt_stat_open(struct inode *inode, struct file *file)
{
	return single_open(file, ax_wdt_stat_show, PDE_DATA(inode));
}

static const struct file_operations ax_wdt_proc_ops = {
	.open = ax_wdt_stat_open,
	.read = seq_read,
	.release = single_release,
	.write = ax_wdt_write,
};

static int ax_wdt_drv_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct watchdog_device *wdd;
	struct ax_wdt *ax_wdt;
	struct resource *mem;
	int ret, flag;
	const struct cpumask *mask;
	static int id = 0;

	ax_wdt = devm_kzalloc(dev, sizeof(*ax_wdt), GFP_KERNEL);
	if (!ax_wdt) {
		pr_err("err %d\n", -ENOMEM);
		return -ENOMEM;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ax_wdt->regs = devm_ioremap_resource(dev, mem);
	if (IS_ERR(ax_wdt->regs)) {
		pr_err("err %ld\n", PTR_ERR(ax_wdt->regs));
		return PTR_ERR(ax_wdt->regs);
	}
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	ax_wdt->common = devm_ioremap(dev, mem->start, mem->end - mem->start);
	if (IS_ERR(ax_wdt->common)) {
		pr_err("err %ld\n", PTR_ERR(ax_wdt->common));
		return PTR_ERR(ax_wdt->common);
	}
	ax_wdt_enable(ax_wdt, 0);
	ret = device_property_read_u32(dev, "clock-frequency", &ax_wdt->rate);
	if (ret) {
		ax_wdt->rate = AX_WDT_CLK_32K;
	}

	if (unlikely(ax_wdt->rate != AX_WDT_CLK_24M))
		ax_wdt->udelay_time = AX_PING_DELAY_US;
	else
		ax_wdt->udelay_time = 1;

	sprintf(ax_wdt->proc_name, "ax_proc/wdt%d_keep_alive", id);
	ax_wdt->pfile = proc_create_data(ax_wdt->proc_name, 0644, NULL, &ax_wdt_proc_ops, ax_wdt);
	if (unlikely(!ax_wdt->pfile)) {
		pr_err("err: Create proc fail!\n");
		goto err;
	}
	ret =
	    of_property_read_u32_index(dev->of_node, "keep-alive", 0,
				       &ax_wdt->keep_alive);
	if (ret)
		ax_wdt->keep_alive = 0;

	ax_wdt->irq = platform_get_irq(pdev, 0);
	ret = device_property_read_u32(dev, "cpu_flag", &flag);
	if (ax_wdt->irq >= 0) {
		if (0 == ret) {
			ax_wdt_irq_enable(ax_wdt);
			mask = cpumask_of(flag);
			irq_set_affinity(ax_wdt->irq, mask);
		}
		ret = devm_request_irq(&pdev->dev, ax_wdt->irq, ax_wdt_interrupt,
			IRQF_SHARED, KBUILD_MODNAME, ax_wdt);
		if (ret) {
			pr_err("err %d\n", ret);
			watchdog_unregister_device(wdd);
			goto err_proc;
		}
	}
	ret = of_property_read_u32_array(dev->of_node, "clk_set", ax_wdt->clk_set, 3);

	if (ret) {
		pr_err("get clk_set err\n");
		goto err_proc;
	}

	ret = of_property_read_u32_array(dev->of_node, "aclk", ax_wdt->aclk, 3);
	if (!ax_wdt->aclk) {
		pr_err("get aclk err\n");
		goto err_proc;
	}

	ret = of_property_read_u32_array(dev->of_node, "pclk", ax_wdt->pclk, 3);
	if (!ax_wdt->pclk) {
		pr_err("get pclk err\n");
		goto err_proc;
	}

	ret = of_property_read_u32_array(dev->of_node, "arst", ax_wdt->arst, 3);
	if (!ax_wdt->arst) {
		pr_err("get arst err\n");
		goto err_proc;
	}

	ret = of_property_read_u32_array(dev->of_node, "prst", ax_wdt->prst, 3);
	if (!ax_wdt->prst) {
		pr_err("get prst err\n");
		goto err_proc;
	}

	wdd = &ax_wdt->wdd;
	wdd->info = &ax_wdt_ident;
	wdd->ops = &ax_wdt_ops;
	wdd->min_timeout = DIV_ROUND_UP(0xFFFF, ax_wdt->rate);
	wdd->max_timeout = AX_MAX_COUNT / ax_wdt->rate;
	wdd->max_hw_heartbeat_ms = AX_MAX_COUNT / ax_wdt->rate * 1000;
	wdd->parent = dev;

	watchdog_set_drvdata(wdd, ax_wdt);
	watchdog_set_nowayout(wdd, nowayout);

	wdd->timeout = AX_WDT_DEFAULT_SECONDS;
	watchdog_init_timeout(wdd, 0, dev);

	platform_set_drvdata(pdev, ax_wdt);

	watchdog_set_restart_priority(wdd, 128);

	ax_wdt_clk_enable(ax_wdt, ax_wdt->rate);
	ax_wdt_reset_set(ax_wdt, 0);
	ax_wdt_start(wdd);
	if (ax_wdt->irq < 0) {
		set_bit(WDOG_HW_RUNNING, &wdd->status);
	}

	ret = watchdog_register_device(wdd);
	if (ret) {
		pr_err("err %d\n", ret);
		goto err_disable_clk;
	}
	mutex_init(&ax_wdt->lock);
	g_ax_wdt[id] = ax_wdt;
	id++;
	pr_info("%s probe done!", dev_name(dev));
	return 0;

err_disable_clk:
	ax_wdt_clk_disable(ax_wdt);
err_proc:
	if (ax_wdt->pfile)
		remove_proc_entry(ax_wdt->proc_name, NULL);
err:
	pr_err("%s probe fail!\n", dev_name(dev));
	return ret;
}

static int ax_wdt_drv_remove(struct platform_device *pdev)
{
	struct ax_wdt *ax_wdt = platform_get_drvdata(pdev);

	watchdog_unregister_device(&ax_wdt->wdd);
	ax_wdt_clk_disable(ax_wdt);
	if (ax_wdt->pfile)
		remove_proc_entry(ax_wdt->proc_name, NULL);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id ax_wdt_of_match[] = {
	{.compatible = "axera,ax-wdt",},
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, ax_wdt_of_match);
#endif

static struct platform_driver ax_wdt_driver = {
	.probe = ax_wdt_drv_probe,
	.remove = ax_wdt_drv_remove,
	.driver = {
		   .name = "ax_wdt",
		   .of_match_table = of_match_ptr(ax_wdt_of_match),
		   .pm = &ax_wdt_pm_ops,
		   },
};

module_platform_driver(ax_wdt_driver);

MODULE_AUTHOR("axera");
MODULE_DESCRIPTION("Axera Watchdog Driver");
MODULE_LICENSE("GPL");
