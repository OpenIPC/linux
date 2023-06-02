/*
 * drivers/rtc/ambarella_rtc.c
 *
 * History:
 *	2008/04/01 - [Cao Rongrong] Support pause and resume
 *	2009/01/22 - [Anthony Ginger] Port to 2.6.28
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/clk.h>
#include <asm/uaccess.h>
#include <mach/hardware.h>
#include <plat/rtc.h>

#define AMBRTC_TIME		0
#define AMBRTC_ALARM		1

struct ambarella_rtc {
	struct rtc_device 	*rtc;
	void __iomem		*reg;
	struct device		*dev;

	/* limitation for old rtc:
	 * 1. cannot detect power lost
	 * 2. the msb 2bits are reserved. */
	bool			is_limited;
	int			irq;
};


static inline void ambrtc_reset_rtc(struct ambarella_rtc *ambrtc)
{
	u32 delay = ambrtc->is_limited ? 3 : 1;

	amba_writel(ambrtc->reg + RTC_RESET_OFFSET, 0x01);
	msleep(delay);
	amba_writel(ambrtc->reg + RTC_RESET_OFFSET, 0x00);
	msleep(delay);
}

static int ambrtc_get_alarm_or_time(struct ambarella_rtc *ambrtc,
		int time_alarm)
{
	u32 reg_offs, val_sec;

	if (time_alarm == AMBRTC_TIME)
		reg_offs = RTC_CURT_OFFSET;
	else
		reg_offs = RTC_ALAT_OFFSET;

	val_sec = amba_readl(ambrtc->reg + reg_offs);
	/* because old rtc cannot use the msb 2bits, we add 0x40000000
	 * here, this is a pure software workaround. And the result is that
	 * the time must be started at least from 2004.01.10 13:38:00 */
	if (ambrtc->is_limited)
		val_sec |= 0x40000000;

	return val_sec;
}

static int ambrtc_set_alarm_or_time(struct ambarella_rtc *ambrtc,
		int time_alarm, unsigned long secs)
{
	u32 time_val, alarm_val;

	if (ambrtc->is_limited && secs < 0x40000000) {
		dev_err(ambrtc->dev,
			"Invalid date[0x%lx](2004.01.10 13:38:00 ~ )\n", secs);
		return -EINVAL;
	}

	if (time_alarm == AMBRTC_TIME) {
		time_val = secs;
		alarm_val = amba_readl(ambrtc->reg + RTC_ALAT_OFFSET);
	} else {
		alarm_val = secs;
		time_val = amba_readl(ambrtc->reg + RTC_CURT_OFFSET);
                // only for wakeup ambarella internal PWC
                amba_writel(ambrtc->reg + RTC_PWC_SET_STATUS_OFFSET, 0x28);
	}

	if (ambrtc->is_limited) {
		time_val &= 0x3fffffff;
		alarm_val &= 0x3fffffff;
	}

	amba_writel(ambrtc->reg + RTC_POS0_OFFSET, 0x80);
	amba_writel(ambrtc->reg + RTC_POS1_OFFSET, 0x80);
	amba_writel(ambrtc->reg + RTC_POS2_OFFSET, 0x80);

	/* reset time and alarm to 0 first */
	amba_writel(ambrtc->reg + RTC_PWC_ALAT_OFFSET, 0x0);
	amba_writel(ambrtc->reg + RTC_PWC_CURT_OFFSET, 0x0);
	ambrtc_reset_rtc(ambrtc);

	/* now write the required value to time or alarm */
	amba_writel(ambrtc->reg + RTC_PWC_CURT_OFFSET, time_val);
	amba_writel(ambrtc->reg + RTC_PWC_ALAT_OFFSET, alarm_val);
	ambrtc_reset_rtc(ambrtc);

	return 0;
}

static int ambrtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct ambarella_rtc *ambrtc;
	u32 time_sec;

	ambrtc = dev_get_drvdata(dev);

	time_sec = ambrtc_get_alarm_or_time(ambrtc, AMBRTC_TIME);

	rtc_time_to_tm(time_sec, tm);

	return 0;
}

static int ambrtc_set_mmss(struct device *dev, unsigned long secs)
{
	struct ambarella_rtc *ambrtc;

	ambrtc = dev_get_drvdata(dev);

	return ambrtc_set_alarm_or_time(ambrtc, AMBRTC_TIME, secs);
}


static int ambrtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct ambarella_rtc *ambrtc;
	u32 alarm_sec, time_sec;
	u32 rtc_status;

	ambrtc = dev_get_drvdata(dev);

	alarm_sec = ambrtc_get_alarm_or_time(ambrtc, AMBRTC_ALARM);
	rtc_time_to_tm(alarm_sec, &alrm->time);

	time_sec = ambrtc_get_alarm_or_time(ambrtc, AMBRTC_TIME);
	alrm->enabled = alarm_sec > time_sec;

	rtc_status = amba_readl(ambrtc->reg + RTC_STATUS_OFFSET);
	alrm->pending = !!(rtc_status & RTC_STATUS_ALA_WK);

	return 0;
}

static int ambrtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct ambarella_rtc *ambrtc;
	unsigned long alarm_sec;

	ambrtc = dev_get_drvdata(dev);

	rtc_tm_to_time(&alrm->time, &alarm_sec);

	return ambrtc_set_alarm_or_time(ambrtc, AMBRTC_ALARM, alarm_sec);
}

static int ambrtc_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
	return 0;
}

static irqreturn_t ambrtc_alarm_irq(int irq, void *dev_id)
{
	struct ambarella_rtc *ambrtc = (struct ambarella_rtc *)dev_id;

	if(ambrtc->rtc)
		rtc_update_irq(ambrtc->rtc, 1, RTC_IRQF | RTC_AF);

	return IRQ_HANDLED;
}

static int ambrtc_ioctl(struct device *dev, unsigned int cmd,
			     unsigned long arg)
{
	struct ambarella_rtc *ambrtc;
	int lbat, rval = 0;

	ambrtc = dev_get_drvdata(dev);

	if (ambrtc->is_limited)
		return -ENOIOCTLCMD;

	switch (cmd) {
	case RTC_VL_READ:
		lbat = !!amba_readl(ambrtc->reg + RTC_PWC_LBAT_OFFSET);
		rval = put_user(lbat, (int __user *)arg);
		break;
	default:
		rval = -ENOIOCTLCMD;
		break;
	}

	return rval;
}

static const struct rtc_class_ops ambarella_rtc_ops = {
	.ioctl		= ambrtc_ioctl,
	.read_time	= ambrtc_read_time,
	.set_mmss	= ambrtc_set_mmss,
	.read_alarm	= ambrtc_read_alarm,
	.set_alarm	= ambrtc_set_alarm,
	.alarm_irq_enable = ambrtc_alarm_irq_enable,
};

static void ambrtc_check_power_lost(struct ambarella_rtc *ambrtc)
{
	u32 status, need_rst, time_sec;

	if (ambrtc->is_limited) {
		status = amba_readl(ambrtc->reg + RTC_STATUS_OFFSET);
		need_rst = !(status & RTC_STATUS_PC_RST);
	} else {
		status = amba_readl(ambrtc->reg + RTC_PWC_REG_STA_OFFSET);
		need_rst = !(status & RTC_PWC_LOSS_MASK);
		amba_setbitsl(ambrtc->reg + RTC_PWC_SET_STATUS_OFFSET,
							RTC_PWC_LOSS_MASK);
	}

	if (need_rst) {
		dev_warn(ambrtc->dev, "=====RTC ever lost power=====\n");
		time_sec = ambrtc->is_limited ? 0x40000000 : 0;
		ambrtc_set_alarm_or_time(ambrtc, AMBRTC_TIME, time_sec);
	}
}


static int ambrtc_probe(struct platform_device *pdev)
{
	struct ambarella_rtc *ambrtc;
	struct resource *mem;
	void __iomem *reg;
	int ret;
	unsigned int wakeup_support;
	struct device_node *np = pdev->dev.of_node;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem == NULL) {
		dev_err(&pdev->dev, "Get mem resource failed!\n");
		return -ENXIO;
	}

	reg = devm_ioremap(&pdev->dev, mem->start, resource_size(mem));
	if (!reg) {
		dev_err(&pdev->dev, "devm_ioremap() failed\n");
		return -ENOMEM;
	}

	ambrtc = devm_kzalloc(&pdev->dev, sizeof(*ambrtc), GFP_KERNEL);
	if (mem == NULL) {
		dev_err(&pdev->dev, "no memory!\n");
		return -ENOMEM;
	}

	ambrtc->irq = platform_get_irq(pdev, 0);
	if (ambrtc->irq < 0) {
		ambrtc->irq = -1;
	} else {
		ret = devm_request_irq(&pdev->dev, ambrtc->irq, ambrtc_alarm_irq, IRQF_SHARED,
					"rtc alarm", ambrtc);
		if (ret) {
			dev_err(&pdev->dev, "could not request irq %d for rtc alarm\n", ambrtc->irq);
			return ret;
		}
	}

	ambrtc->reg = reg;
	ambrtc->dev = &pdev->dev;
	ambrtc->is_limited = !!of_find_property(pdev->dev.of_node,
				"amb,is-limited", NULL);
	platform_set_drvdata(pdev, ambrtc);
	ambrtc_check_power_lost(ambrtc);

	wakeup_support = !!of_get_property(np, "rtc,wakeup", NULL);
	if (wakeup_support)
		device_set_wakeup_capable(&pdev->dev, 1);

	ambrtc->rtc = devm_rtc_device_register(&pdev->dev, "rtc-ambarella",
				     &ambarella_rtc_ops, THIS_MODULE);
	if (IS_ERR(ambrtc->rtc)) {
		dev_err(&pdev->dev, "devm_rtc_device_register fail.\n");
		return PTR_ERR(ambrtc->rtc);
	}

	ambrtc->rtc->uie_unsupported = 1;

	return 0;
}

static int ambrtc_remove(struct platform_device *pdev)
{
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id ambarella_rtc_dt_ids[] = {
	{.compatible = "ambarella,rtc", },
	{},
};
MODULE_DEVICE_TABLE(of, ambarella_rtc_dt_ids);

#ifdef CONFIG_PM_SLEEP
static int ambarella_rtc_suspend(struct device *dev)
{
	struct ambarella_rtc *ambrtc = dev_get_drvdata(dev);

	if (device_may_wakeup(dev)){
		if (ambrtc->irq != -1)
			enable_irq_wake(ambrtc->irq);
	}

	return 0;
}

static int ambarella_rtc_resume(struct device *dev)
{
	struct ambarella_rtc *ambrtc = dev_get_drvdata(dev);

	if (device_may_wakeup(dev)) {
		if (ambrtc->irq != -1)
			disable_irq_wake(ambrtc->irq);
	}
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(ambarella_rtc_pm_ops, ambarella_rtc_suspend, ambarella_rtc_resume);

static struct platform_driver ambarella_rtc_driver = {
	.probe		= ambrtc_probe,
	.remove		= ambrtc_remove,
	.driver		= {
		.name	= "ambarella-rtc",
		.owner	= THIS_MODULE,
		.of_match_table = ambarella_rtc_dt_ids,
		.pm	= &ambarella_rtc_pm_ops,
	},
};

module_platform_driver(ambarella_rtc_driver);

MODULE_DESCRIPTION("Ambarella Onchip RTC Driver");
MODULE_AUTHOR("Cao Rongrong <rrcao@ambarella.com>");
MODULE_LICENSE("GPL");

