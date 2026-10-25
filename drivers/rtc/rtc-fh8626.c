// SPDX-License-Identifier: GPL-2.0
/*
 * Fullhan FH8626 real-time clock
 *
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/bitops.h>
#include <linux/bits.h>
#include <linux/iopoll.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>

#define FH_RTC_CONTROL			0x00
#define FH_RTC_COMMAND			0x14
#define FH_RTC_READ_DATA		0x18
#define FH_RTC_WRITE_DATA		0x1c

#define FH_RTC_READY			BIT(5)
#define FH_RTC_CMD_READ			1
#define FH_RTC_CMD_WRITE		2

#define FH_RTC_SEC_MASK			GENMASK(5, 0)
#define FH_RTC_MIN_MASK			GENMASK(11, 6)
#define FH_RTC_MIN_SHIFT		6
#define FH_RTC_HOUR_MASK		GENMASK(16, 12)
#define FH_RTC_HOUR_SHIFT		12
#define FH_RTC_DAYS_SHIFT		17
#define FH_RTC_DAYS_MAX			0x7fff

#define FH_RTC_POLL_US			1000
#define FH_RTC_READ_TIMEOUT_US		25000
#define FH_RTC_WRITE_TIMEOUT_US		15000

struct fh8626_rtc {
	void __iomem		*base;
};

static int fh8626_rtc_wait(struct fh8626_rtc *rtc, unsigned int timeout_us)
{
	u32 val;

	return readl_poll_timeout(rtc->base + FH_RTC_CONTROL, val,
				  val & FH_RTC_READY, FH_RTC_POLL_US,
				  timeout_us);
}

static int fh8626_rtc_read_word(struct fh8626_rtc *rtc, u32 *word)
{
	u32 control;
	int ret;

	control = readl(rtc->base + FH_RTC_CONTROL);
	if (!(control & FH_RTC_READY))
		return -ENODEV;

	writel(control & ~FH_RTC_READY, rtc->base + FH_RTC_CONTROL);
	writel(FH_RTC_CMD_READ, rtc->base + FH_RTC_COMMAND);
	ret = fh8626_rtc_wait(rtc, FH_RTC_READ_TIMEOUT_US);
	if (ret)
		return ret;

	*word = readl(rtc->base + FH_RTC_READ_DATA);
	return 0;
}

static int fh8626_rtc_write_word(struct fh8626_rtc *rtc, u32 word)
{
	u32 control;

	control = readl(rtc->base + FH_RTC_CONTROL);
	if (!(control & FH_RTC_READY))
		return -ENODEV;

	writel(control & ~FH_RTC_READY, rtc->base + FH_RTC_CONTROL);
	writel(word, rtc->base + FH_RTC_WRITE_DATA);
	writel(FH_RTC_CMD_WRITE, rtc->base + FH_RTC_COMMAND);
	return fh8626_rtc_wait(rtc, FH_RTC_WRITE_TIMEOUT_US);
}

static int fh8626_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct fh8626_rtc *rtc = dev_get_drvdata(dev);
	u32 word, sec, min, hour, days;
	int ret;

	ret = fh8626_rtc_read_word(rtc, &word);
	if (ret)
		return ret;

	sec = word & FH_RTC_SEC_MASK;
	min = (word & FH_RTC_MIN_MASK) >> FH_RTC_MIN_SHIFT;
	hour = (word & FH_RTC_HOUR_MASK) >> FH_RTC_HOUR_SHIFT;
	days = word >> FH_RTC_DAYS_SHIFT;
	if (sec > 59 || min > 59 || hour > 23)
		return -EINVAL;

	rtc_time64_to_tm((time64_t)days * 86400 + hour * 3600 +
			 min * 60 + sec, tm);
	return 0;
}

static int fh8626_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	struct fh8626_rtc *rtc = dev_get_drvdata(dev);
	time64_t secs = rtc_tm_to_time64(tm);
	u32 days, tod, word;

	if (secs < 0)
		return -EINVAL;

	days = div_u64_rem((u64)secs, 86400, &tod);
	if (days > FH_RTC_DAYS_MAX)
		return -EINVAL;

	word = (days << FH_RTC_DAYS_SHIFT) |
	       ((tod / 3600) << FH_RTC_HOUR_SHIFT) |
	       (((tod % 3600) / 60) << FH_RTC_MIN_SHIFT) |
	       (tod % 60);
	return fh8626_rtc_write_word(rtc, word);
}

static const struct rtc_class_ops fh8626_rtc_ops = {
	.read_time = fh8626_rtc_read_time,
	.set_time = fh8626_rtc_set_time,
};

static int fh8626_rtc_probe(struct platform_device *pdev)
{
	struct fh8626_rtc *rtc;
	struct rtc_device *rdev;
	struct resource *res;

	rtc = devm_kzalloc(&pdev->dev, sizeof(*rtc), GFP_KERNEL);
	if (!rtc)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	rtc->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(rtc->base))
		return PTR_ERR(rtc->base);

	if (!(readl(rtc->base + FH_RTC_CONTROL) & FH_RTC_READY)) {
		dev_err(&pdev->dev, "RTC is not ready\n");
		return -ENODEV;
	}

	platform_set_drvdata(pdev, rtc);

	rdev = devm_rtc_allocate_device(&pdev->dev);
	if (IS_ERR(rdev))
		return PTR_ERR(rdev);

	rdev->ops = &fh8626_rtc_ops;
	rdev->range_min = 0;
	rdev->range_max = (u64)FH_RTC_DAYS_MAX * 86400 + 86399;
	return rtc_register_device(rdev);
}

static const struct of_device_id fh8626_rtc_of_match[] = {
	{ .compatible = "fullhan,fh8626v100-rtc" },
	{ }
};
MODULE_DEVICE_TABLE(of, fh8626_rtc_of_match);

static struct platform_driver fh8626_rtc_driver = {
	.probe = fh8626_rtc_probe,
	.driver = {
		.name = "fh8626-rtc",
		.of_match_table = fh8626_rtc_of_match,
	},
};
module_platform_driver(fh8626_rtc_driver);

MODULE_DESCRIPTION("Fullhan FH8626 real-time clock");
MODULE_AUTHOR("Difan Zhang");
MODULE_LICENSE("GPL v2");
