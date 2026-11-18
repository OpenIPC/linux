/*
 * DesignWare Real Time Clock Driver
 *
 * Copyright (C) 2019 AIChip, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/hrtimer.h>

#include "rtc-axera.h"

#define RTC_START_YEAR              1970
#define RTC_X32_CLK_SEL             1
#define CLK_SLV_ADDR                0x30
#define RTC_SLV_EB                  BIT(9)
#define CLK_CONFIG_ADDR             0x6C
#define RTC_CLOCK_EB                BIT(0)
#define RTC_PASSWORD                0x61696370
#define RTC_DEFAULT_CLOCK           4 // 4HZ

struct axera_rtc_dev {
	struct rtc_device *rtc;
	void __iomem *reg_base;
	void __iomem *clk_base;
	int alarm_irq;
	bool irq_enabled;
};

static int convert2days(u16 *days, struct rtc_time *tm)
{
	int i;
	*days = 0;

	/* epoch == 1900 */
	if (tm->tm_year < 70 || tm->tm_year > 199)
		return -EINVAL;

	for (i = 1970; i < 1900 + tm->tm_year; i++)
		*days += rtc_year_days(1, 12, i);

	*days += rtc_year_days(tm->tm_mday, tm->tm_mon, 1900 + tm->tm_year);

	return 0;
}

static void convertfromdays(u16 days, struct rtc_time *tm)
{
	int tmp_days, year, mon;

	for (year = 1970;; year++) {
		tmp_days = rtc_year_days(1, 12, year);
		if (days >= tmp_days)
			days -= tmp_days;
		else {
			for (mon = 0;; mon++) {
				tmp_days = rtc_month_days(mon, year);
				if (days >= tmp_days) {
					days -= tmp_days;
				} else {
					tm->tm_year = year - 1900;
					tm->tm_mon = mon;
					tm->tm_mday = days + 1;
					break;
				}
			}
			break;
		}
	}
}

static int axera_rtc_24mclk_enable(struct axera_rtc_dev *axera_rtcdev)
{
	u32 val;

	val = readl(axera_rtcdev->clk_base + CLK_CONFIG_ADDR);
	writel(val | RTC_CLOCK_EB, (axera_rtcdev->clk_base + CLK_CONFIG_ADDR));

	return 0;
}

static int axera_rtc_24mclk_disable(struct axera_rtc_dev *axera_rtcdev)
{
	u32 val;

	val = readl(axera_rtcdev->clk_base + CLK_CONFIG_ADDR);
	writel(val & (~RTC_CLOCK_EB), (axera_rtcdev->clk_base + CLK_CONFIG_ADDR));

	return 0;
}

static int axera_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	struct axera_rtc_dev *axera_rtcdev = dev_get_drvdata(dev);
	u16 days, cfg_valid_intr, wait_times = 0;
	time64_t new_time;

	/*
	 * The value written will be updated after 1 sec into the
	 * seconds read register, so we need to program time +1 sec
	 * to get the correct time on read.
	 */
	new_time = rtc_tm_to_time64(tm) + 1;
	rtc_time64_to_tm(new_time, tm);

	if (convert2days(&days, tm)) {
		printk("%s year %d invalid\n", __func__, tm->tm_year);
		return -EINVAL;
	}
	dev_dbg(dev, "%s: %ds %dm %dh %dmd %dwd %dm %dy\n", __func__, tm->tm_sec, tm->tm_min, tm->tm_hour, tm->tm_mday, tm->tm_wday, tm->tm_mon, tm->tm_year);

	writel(tm->tm_sec * RTC_DEFAULT_CLOCK, axera_rtcdev->reg_base + RTC_SET_SEC_ADDR);
	writel(tm->tm_min, axera_rtcdev->reg_base + RTC_SET_MIN_ADDR);
	writel(tm->tm_hour, axera_rtcdev->reg_base + RTC_SET_HOUR_ADDR);
	writel(days, axera_rtcdev->reg_base + RTC_SET_DAY_ADDR);
	writel(0xf, (axera_rtcdev->reg_base + RTC_SET_SW_ADDR));	//enable sec min hour day sw cfg

	while (wait_times++ < 50) {
		cfg_valid_intr = readl(axera_rtcdev->reg_base + RTC_CFG_VALID_RSTAT_ADDR);
		if (0xf == (cfg_valid_intr & 0xf)) {
			writel(0xf, axera_rtcdev->reg_base + RTC_CFG_VALID_CLR_INTR_ADDR);
			break;
		}
		msleep_interruptible(10);
	}
	return 0;
}

static int axera_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	u16 days;
	struct axera_rtc_dev *axera_rtcdev = dev_get_drvdata(dev);
	/*
	 * Time written in SET_TIME_WRITE has not yet updated into
	 * the seconds read register, so read the time from the
	 * SET_TIME_WRITE instead of CURRENT_TIME register.
	 * Since we add +1 sec while writing, we need to -1 sec while
	 * reading.
	 */
	tm->tm_sec = readl(axera_rtcdev->reg_base + RTC_SEC_CCVR_ADDR) / RTC_DEFAULT_CLOCK;
	tm->tm_min = readl(axera_rtcdev->reg_base + RTC_MIN_CCVR_ADDR);
	tm->tm_hour = readl(axera_rtcdev->reg_base + RTC_HOUR_CCVR_ADDR);
	days = readl(axera_rtcdev->reg_base + RTC_DAY_CCVR_ADDR);
	convertfromdays(days, tm);
	/* day of the week, 1970-01-01 was a Thursday */
	tm->tm_wday = (days + 4) % 7;

	if (days == 0 && tm->tm_hour < 8) {
		tm->tm_hour = 8;
	}

	dev_dbg(dev, "%s: %ds %dm %dh %dmd %dwd %dm %dy\n", __func__, tm->tm_sec, tm->tm_min, tm->tm_hour, tm->tm_mday, tm->tm_wday, tm->tm_mon, tm->tm_year);

	return rtc_valid_tm(tm);
}

static int axera_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct axera_rtc_dev *axera_rtcdev = dev_get_drvdata(dev);
	struct rtc_time *tm;
	u16 days;
	tm = &alrm->time;

	alrm->enabled = readl(axera_rtcdev->reg_base + RTC_ALARM_INTR_EN_ADDR) & 0x1;
	if (alrm->enabled) {
		tm->tm_sec = readl(axera_rtcdev->reg_base + RTC_SEC_MATCH_ADDR) / RTC_DEFAULT_CLOCK;
		tm->tm_min = readl(axera_rtcdev->reg_base + RTC_MIN_MATCH_ADDR);
		tm->tm_hour = readl(axera_rtcdev->reg_base + RTC_HOUR_MATCH_ADDR);
		days = readl(axera_rtcdev->reg_base + RTC_DAY_MATCH_ADDR);
	} else {
		tm->tm_sec = 0;
		tm->tm_min = 0;
		tm->tm_hour = 0;
		days = 0;
	}

	convertfromdays(days, tm);

	/* day of the week, 1970-01-01 was a Thursday */
	tm->tm_wday = (days + 4) % 7;

	if (days == 0 && tm->tm_hour < 8) {
		tm->tm_hour = 8;
	}

	dev_dbg(dev, "%s: %ds %dm %dh %dmd %dwd %dm %dy\n", __func__, tm->tm_sec, tm->tm_min, tm->tm_hour, tm->tm_mday, tm->tm_wday, tm->tm_mon, tm->tm_year);

	return rtc_valid_tm(tm);
}

static int axera_rtc_alarm_irq_enable(struct device *dev, u32 enabled)
{
	struct axera_rtc_dev *axera_rtcdev = dev_get_drvdata(dev);

	if (enabled) {
		writel(0x1, axera_rtcdev->reg_base + RTC_ALARM_INTR_EN_ADDR);	//enable alarm int
		writel(0x0, axera_rtcdev->reg_base + RTC_ALARM_INTR_MASK_ADDR);	//unmask mask int
		if (axera_rtcdev->irq_enabled == false){
			enable_irq(axera_rtcdev->alarm_irq);
			axera_rtcdev->irq_enabled = true;
		}
	} else {
		writel(0x0, axera_rtcdev->reg_base + RTC_ALARM_INTR_EN_ADDR);	//disable alarm int
		writel(0x1, axera_rtcdev->reg_base + RTC_ALARM_INTR_MASK_ADDR);	//mask mask int
	}
	msleep_interruptible(600);
	return 0;
}

static int axera_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct axera_rtc_dev *axera_rtcdev = dev_get_drvdata(dev);
	u16 days;
	struct rtc_time *tm = &alrm->time;

	writel(1, (axera_rtcdev->reg_base + RTC_ALARM_CLR_INTR_ADDR));	// clear alarm int
	writel(0, (axera_rtcdev->reg_base + RTC_ALARM_CLR_INTR_ADDR));	// clear alarm int
	writel(1, (axera_rtcdev->reg_base + RTC_ALARM_INTR_MASK_ADDR));	// mask alarm int

	if (convert2days(&days, tm)) {
		printk("%s year %d invalid\n", __func__, tm->tm_year);
		return -EINVAL;
	}

	writel((tm->tm_sec) * RTC_DEFAULT_CLOCK, axera_rtcdev->reg_base + RTC_SEC_MATCH_ADDR);
	writel(tm->tm_min, axera_rtcdev->reg_base + RTC_MIN_MATCH_ADDR);
	writel(tm->tm_hour, axera_rtcdev->reg_base + RTC_HOUR_MATCH_ADDR);
	writel(days, axera_rtcdev->reg_base + RTC_DAY_MATCH_ADDR);

	// start alram
	writel(1, axera_rtcdev->reg_base + RTC_MATCH_START_ADDR);
	msleep_interruptible(600);

	axera_rtc_alarm_irq_enable(dev, alrm->enabled);

	return 0;
}

static const struct rtc_class_ops axera_rtc_ops = {
	.set_time = axera_rtc_set_time,
	.read_time = axera_rtc_read_time,
	.read_alarm = axera_rtc_read_alarm,
	.set_alarm = axera_rtc_set_alarm,
	.alarm_irq_enable = axera_rtc_alarm_irq_enable,
};

static void axera_init_rtc(struct axera_rtc_dev *axera_rtcdev)
{
	u32 val;

	val = readl(axera_rtcdev->clk_base + CLK_SLV_ADDR);
	writel(val | RTC_SLV_EB, (axera_rtcdev->clk_base + CLK_SLV_ADDR));

	val = readl(axera_rtcdev->clk_base + CLK_CONFIG_ADDR);
	writel(val | RTC_CLOCK_EB, (axera_rtcdev->clk_base + CLK_CONFIG_ADDR));

	writel(RTC_PASSWORD, (axera_rtcdev->reg_base + RTC_PASSWORD_ADDR));

	/*
	*  REG_XTAL32K
	*/
	val = readl(axera_rtcdev->reg_base + RTC_X32K_XIN_ADDR);
	writel(val | 0x1D5, (axera_rtcdev->reg_base + RTC_X32K_XIN_ADDR));

	/*
	 * Rtc can be configured to use soc external and external 32k clocks.
	 * rtc registers use external clocks by default, and the configuration
	 * will not be lost after installing the battery.
	 */

	/*
	 * internal clk configured
	 writel(0, (axera_rtcdev->reg_base + RTC_X32K_XIN_EN_ADDR));
	 */
	writel(0, (axera_rtcdev->reg_base + RTC_CFG_VALID_INTR_EN_ADDR));
}

static irqreturn_t axera_rtc_interrupt(int irq, void *id)
{
	struct axera_rtc_dev *axera_rtcdev = (struct axera_rtc_dev *)id;
	unsigned int status;

	/*pr_debug("@axera_rtc_interrupt irq:%d, alarm status = %x time_status = %x\n", irq, readl(axera_rtcdev->reg_base + RTC_ALARM_INTR_ADDR),
	       readl(axera_rtcdev->reg_base + RTC_CFG_VALID_INTR_ADDR));*/

	disable_irq_nosync(irq);
	axera_rtcdev->irq_enabled = false;
	if (readl(axera_rtcdev->reg_base + RTC_CFG_VALID_INTR_ADDR) != 0x0) {
		status = readl(axera_rtcdev->reg_base + RTC_CFG_VALID_INTR_ADDR);
		writel(status, axera_rtcdev->reg_base + RTC_CFG_VALID_CLR_INTR_ADDR);
	}

	if ((readl(axera_rtcdev->reg_base + RTC_ALARM_INTR_ADDR) & 0x01) == 0x01) {
		status = readl(axera_rtcdev->reg_base + RTC_ALARM_INTR_ADDR);	//read alarm int status
		/* Check if interrupt asserted */
		if (!(status & 0x1)) {
			enable_irq(irq);
			return IRQ_NONE;
		}

		/* Read Clearinterrupt only */
		writel(status, axera_rtcdev->reg_base + RTC_ALARM_CLR_INTR_ADDR);
		rtc_update_irq(axera_rtcdev->rtc, 1, RTC_IRQF | RTC_AF);

	}

	return IRQ_HANDLED;
}

static int axera_rtc_probe(struct platform_device *pdev)
{
	struct axera_rtc_dev *axera_rtcdev;
	struct resource *res;
	int ret;

	axera_rtcdev = devm_kzalloc(&pdev->dev, sizeof(*axera_rtcdev), GFP_KERNEL);
	if (!axera_rtcdev) {
		printk("axera_rtcdev kzalloc failed\n");
		return -ENOMEM;
	}

	pdev->dev.driver_data = axera_rtcdev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "axera rtc no memory resource defined IORESOURCE_MEM 0\n");
		return -ENODEV;
	}

	axera_rtcdev->reg_base = (void *)ioremap(res->start, resource_size(res));
	if (IS_ERR(axera_rtcdev->reg_base)) {
		printk("ioremap reg_base failed\n");
		return PTR_ERR(axera_rtcdev->reg_base);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (res == NULL) {
		iounmap(axera_rtcdev->reg_base);
		dev_err(&pdev->dev, "axera rtc no memory resource defined IORESOURCE_MEM 1\n");
		return -ENODEV;
	}

	axera_rtcdev->clk_base = (void *)ioremap(res->start, resource_size(res));
	if (IS_ERR(axera_rtcdev->clk_base)) {
		printk("ioremap clk_base failed\n");
		iounmap(axera_rtcdev->reg_base);
		return PTR_ERR(axera_rtcdev->clk_base);
	}

	axera_rtcdev->alarm_irq = platform_get_irq(pdev, 0);
	axera_rtcdev->irq_enabled = true;
	if (axera_rtcdev->alarm_irq < 0) {
		printk("no irq resource\n");
		iounmap(axera_rtcdev->clk_base);
		iounmap(axera_rtcdev->reg_base);
		return axera_rtcdev->alarm_irq;
	}

	axera_init_rtc(axera_rtcdev);

	axera_rtcdev->rtc = devm_rtc_device_register(&pdev->dev, pdev->name, &axera_rtc_ops, THIS_MODULE);
	if (IS_ERR(axera_rtcdev->rtc)) {
		iounmap(axera_rtcdev->clk_base);
		iounmap(axera_rtcdev->reg_base);
		return PTR_ERR(axera_rtcdev->rtc);
	}

	ret = request_threaded_irq(axera_rtcdev->alarm_irq, axera_rtc_interrupt, NULL, 0, dev_name(&pdev->dev), axera_rtcdev);
	if (ret) {
		printk("request irq failed\n");
		iounmap(axera_rtcdev->clk_base);
		iounmap(axera_rtcdev->reg_base);
		return ret;
	}

	device_init_wakeup(&pdev->dev, 1);

	return PTR_ERR_OR_ZERO(axera_rtcdev->rtc);
}

static int axera_rtc_remove(struct platform_device *pdev)
{
	struct axera_rtc_dev *axera_rtcdev = pdev->dev.driver_data;

	axera_rtc_alarm_irq_enable(&pdev->dev, 0);
	device_init_wakeup(&pdev->dev, 0);
	free_irq(axera_rtcdev->alarm_irq, axera_rtcdev);
	iounmap(axera_rtcdev->clk_base);
	iounmap(axera_rtcdev->reg_base);
	kfree(axera_rtcdev);

	return 0;
}

static int __maybe_unused axera_rtc_suspend(struct device *dev)
{
	struct platform_device *pdev = container_of((dev), struct platform_device, dev);
	struct axera_rtc_dev *axera_rtcdev = pdev->dev.driver_data;

	if (device_may_wakeup(&pdev->dev))
		enable_irq_wake(axera_rtcdev->alarm_irq);
	else
		axera_rtc_alarm_irq_enable(dev, 0);

	axera_rtc_24mclk_disable(axera_rtcdev);

	return 0;
}

static int __maybe_unused axera_rtc_resume(struct device *dev)
{
	struct platform_device *pdev = container_of((dev), struct platform_device, dev);
	struct axera_rtc_dev *axera_rtcdev = pdev->dev.driver_data;

	axera_rtc_24mclk_enable(axera_rtcdev);

	if (device_may_wakeup(&pdev->dev))
		disable_irq_wake(axera_rtcdev->alarm_irq);
	else
		axera_rtc_alarm_irq_enable(dev, 1);

	return 0;
}

static SIMPLE_DEV_PM_OPS(axera_rtc_pm_ops, axera_rtc_suspend, axera_rtc_resume);

static const struct of_device_id axera_rtc_of_match[] = {
	{.compatible = "axera,axi-top-rtc"},
	{}
};

static struct platform_driver axera_rtc_driver = {
	.probe = axera_rtc_probe,
	.remove = axera_rtc_remove,
	.driver = {
		   .name = KBUILD_MODNAME,
		   .pm = &axera_rtc_pm_ops,
		   .of_match_table = axera_rtc_of_match,
		   },
};

static int __init axera_rtc_init(void)
{
	return platform_driver_register(&axera_rtc_driver);
}

static void axera_rtc_exit(void)
{
	platform_driver_unregister(&axera_rtc_driver);
}

module_init(axera_rtc_init);
module_exit(axera_rtc_exit);

MODULE_DESCRIPTION("Axera RTC driver");
MODULE_AUTHOR("Axera Inc.");
MODULE_LICENSE("GPL v2");
