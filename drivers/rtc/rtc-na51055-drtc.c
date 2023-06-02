/*
 *  driver/rtc/rtc-na51055-dtrc.c
 *
 *  Author:     howard_chang@novatek.com.tw
 *  Created:	Feb 26, 2019
 *  Copyright:	Novatek Inc.
 *
 */


#include <linux/module.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <mach/nvt-io.h>
#include <plat/drtc_reg.h>
#include <plat/drtc_int.h>
#include <plat/hardware.h>
#include <linux/clk.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/of.h>

#define DRV_VERSION		"1.00.000"

static struct semaphore drtc_sem;

unsigned int _REGIOBASE;

#define loc_cpu() down(&drtc_sem);
#define unl_cpu() up(&drtc_sem);

static const unsigned short drtc_ydays[2][13] = {
	/* Normal years */
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
	/* Leap years */
	{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};

struct nvt_drtc_priv {
	struct rtc_device *rtc;
	struct clk        *clk;
};

static void drtc_setreg(uint32_t offset, REGVALUE value)
{
	nvt_writel(value, _REGIOBASE + offset);
}

static REGVALUE drtc_getreg(uint32_t offset)
{
	return nvt_readl(_REGIOBASE + offset);
}

/*
//add by Y_S WU 2014.5.22  CTS default time
static struct rtc_time default_tm = {
	.tm_year = (2014 - 1900), // year 2014
	.tm_mon =  1,             // month 2
	.tm_mday = 5,             // day 5
	.tm_hour = 12,
	.tm_min = 0,
	.tm_sec = 0
};
*/
static void nvt_drtc_enable_configuration(void)
{
	uint32_t count = 0;
	union DRTC_CTRL_REG ctrl_reg;

	ctrl_reg.reg = drtc_getreg(DRTC_CTRL_REG_OFS);
	ctrl_reg.bit.time_en = 0;
	drtc_setreg(DRTC_CTRL_REG_OFS, ctrl_reg.reg);

	/* check hardware stop */
	do {
		ctrl_reg.reg = drtc_getreg(DRTC_CTRL_REG_OFS);
		if (ctrl_reg.bit.time_en)
			usleep_range(10, 100);
		else
			break;
	} while(count++ < 100);

	if (ctrl_reg.bit.time_en)
		pr_err("time counter stop timeout!\n");

	ctrl_reg.bit.time_en = 1;
	drtc_setreg(DRTC_CTRL_REG_OFS, ctrl_reg.reg);
}

#ifdef CONFIG_RTC_INTF_DEV

static int nvt_drtc_ioctl(struct device *dev, unsigned int cmd, \
	unsigned long arg)
{
/*
	switch(cmd) {
		case RTC_AIE_ON:
			pr_info("RTC_AIE_ON\n");
			break;
		case RTC_AIE_OFF:
			pr_info("RTC_AIE_OFF\n");
			break;
		case RTC_UIE_ON:
			pr_info("RTC_UIE_ON\n");
			break;
		case RTC_UIE_OFF:
			pr_info("RTC_UIE_OFF\n");
			break;
		case RTC_WIE_ON:
			pr_info("RTC_WIE_ON\n");
			break;
		case RTC_WIE_OFF:
			pr_info("RTC_WIE_OFF\n");
			break;
		case RTC_ALM_SET:
			pr_info("RTC_ALM_SET\n");
			break;
		case RTC_ALM_READ:
			pr_info("RTC_ALM_READ\n");
			break;
		case RTC_RD_TIME:
			pr_info("RTC_RD_TIME\n");
			break;
		case RTC_SET_TIME:
			pr_info("RTC_SET_TIME\n");
			break;
		case RTC_IRQP_READ:
			pr_info("RTC_IRQP_READ\n");
			break;
		case RTC_IRQP_SET:
			pr_info("RTC_IRQP_SET\n");
			break;
		case RTC_EPOCH_READ:
			pr_info("RTC_EPOCH_READ\n");
			break;
		case RTC_EPOCH_SET:
			pr_info("RTC_EPOCH_SET\n");
			break;
		case RTC_WKALM_SET:
			pr_info("RTC_WKALM_SET\n");
			break;
		case RTC_WKALM_RD:
			pr_info("RTC_WKALM_RD\n");
			break;
		case RTC_PLL_SET:
			pr_info("RTC_PLL_SET\n");
			break;
		case RTC_PLL_GET:
			pr_info("RTC_PLL_GET\n");
			break;
		default:
			pr_info("unknown rtc ioctl :0X%X\n",cmd);
	}
*/
	return 0;
}

#else
#define nvt_drtc_ioctl    NULL
#endif

#ifdef CONFIG_PROC_FS

static int nvt_drtc_proc(struct device *dev, struct seq_file *seq)
{
	return 0;
}

#else
#define nvt_drtc_proc     NULL
#endif


static int nvt_drtc_read_time(struct device *dev, struct rtc_time *tm)
{
	uint32_t days, months, years, month_days;
	union DRTC_TIMER_REG timer_reg;
	union DRTC_DAY_REG day_reg;

	timer_reg.reg = drtc_getreg(DRTC_TIMER_REG_OFS);
	day_reg.reg = drtc_getreg(DRTC_DAY_REG_OFS);
	days = day_reg.bit.day;

	for (years = 0; days >= drtc_ydays[is_leap_year(years + 1900)][12]; \
	years++) {
		days -= drtc_ydays[is_leap_year(years + 1900)][12];
	}

	for (months = 1; months < 13; months++) {
		if (days <= drtc_ydays[is_leap_year(years + 1900)][months]) {
			days -= drtc_ydays[is_leap_year(years + 1900)][months-1];
			months--;
			break;
		}
	}

	month_days = drtc_ydays[is_leap_year(years + 1900)][months+1] - \
		drtc_ydays[is_leap_year(years + 1900)][months];

	if (days == month_days) {
		months++;
		days = 1;
	} else
		days++; /*Align linux time format*/

	tm->tm_sec  = timer_reg.bit.sec;
	tm->tm_min  = timer_reg.bit.min;
	tm->tm_hour = timer_reg.bit.hour;
	tm->tm_mday = days;
	tm->tm_mon  = months;
	tm->tm_year = years;

	pr_debug("after read time: sec = %d, min = %d, hour = %d, mday = %d," \
	"mon = %d, year = %d, wday = %d, yday = %d," \
	"\n", tm->tm_sec, tm->tm_min, tm->tm_hour, tm->tm_mday, \
	tm->tm_mon, tm->tm_year, tm->tm_wday, tm->tm_yday);

	return rtc_valid_tm(tm);
}

static int nvt_drtc_set_time(struct device *dev, struct rtc_time *tm)
{
	int year_looper, ret;
	uint32_t days = 0;
	union DRTC_TIMER_REG timer_reg;
	union DRTC_DAY_REG day_reg;

	pr_debug("kernel set time: sec = %d, min = %d, hour = %d, mday = %d," \
	"mon = %d, year = %d, wday = %d, yday = %d," \
	"\n", tm->tm_sec, tm->tm_min, tm->tm_hour, tm->tm_mday, \
	tm->tm_mon, tm->tm_year, tm->tm_wday, tm->tm_yday);

	ret = rtc_valid_tm(tm);
	if (ret < 0)
		return ret;

	for (year_looper = 0; year_looper < tm->tm_year; year_looper++)
		days += drtc_ydays[is_leap_year(year_looper + 1900)][12];

	days += drtc_ydays[is_leap_year(year_looper + 1900)][tm->tm_mon];
	tm->tm_mday--; /*subtract the day which is not ended*/
	days += tm->tm_mday;

	loc_cpu();

	timer_reg.reg = 0;
	timer_reg.bit.sec = tm->tm_sec;
	timer_reg.bit.min = tm->tm_min;
	timer_reg.bit.hour = tm->tm_hour;
	drtc_setreg(DRTC_TIMER_REG_OFS, timer_reg.reg);

	day_reg.reg = drtc_getreg(DRTC_DAY_REG_OFS);
	day_reg.bit.day = days;
	drtc_setreg(DRTC_DAY_REG_OFS, day_reg.reg);

	nvt_drtc_enable_configuration();

	unl_cpu();

	return ret;
}

static int nvt_drtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct rtc_time *tm = &alrm->time;
	uint32_t days, months, years, month_days;
	union DRTC_ALARM_TIMER_REG alm_timer_reg;
	union DRTC_ALARM_DAY_REG alm_day_reg;

	alm_timer_reg.reg = drtc_getreg(DRTC_ALARM_TIMER_REG_OFS);
	alm_day_reg.reg = drtc_getreg(DRTC_ALARM_DAY_REG_OFS);

	days = alm_day_reg.bit.day;

	for (years = 0; days >= drtc_ydays[is_leap_year(years + 1900)][12]; \
	years++) {
		days -= drtc_ydays[is_leap_year(years + 1900)][12];
	}

	for (months = 1; months < 13; months++) {
		if (days <= drtc_ydays[is_leap_year(years + 1900)][months]) {
			days -= drtc_ydays[is_leap_year(years + 1900)][months-1];
			months--;
			break;
		}
	}

	month_days = drtc_ydays[is_leap_year(years + 1900)][months+1] - \
		drtc_ydays[is_leap_year(years + 1900)][months];

	if (days == month_days) {
		months++;
		days = 1;
	} else
		days++; /*Align linux time format*/

	tm->tm_sec  = alm_timer_reg.bit.sec;
	tm->tm_min  = alm_timer_reg.bit.min;
	tm->tm_hour = alm_timer_reg.bit.hour;
	tm->tm_mday = days;
	tm->tm_mon  = months;
	tm->tm_year = years;

	pr_debug("read alarm time: sec = %d, min = %d, hour = %d, mday = %d," \
	"mon = %d, year = %d, wday = %d, yday = %d," \
	"\n", tm->tm_sec, tm->tm_min, tm->tm_hour, tm->tm_mday, \
	tm->tm_mon, tm->tm_year, tm->tm_wday, tm->tm_yday);

	return rtc_valid_tm(tm);
}

static int nvt_drtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct rtc_time *tm = &alrm->time;
	int year_looper, ret;
	uint32_t current_days = 0, alarm_days = 0;
	union DRTC_ALARM_TIMER_REG alm_timer_reg;
	union DRTC_ALARM_DAY_REG alm_day_reg;
	union DRTC_DAY_REG day_reg;

	day_reg.reg = drtc_getreg(DRTC_DAY_REG_OFS);
	current_days = day_reg.bit.day;

	pr_debug("set alarm time: sec = %d, min = %d, hour = %d, mday = %d," \
	"mon = %d, year = %d, wday = %d, yday = %d," \
	"\n", tm->tm_sec, tm->tm_min, tm->tm_hour, tm->tm_mday, \
	tm->tm_mon, tm->tm_year, tm->tm_wday, tm->tm_yday);

	ret = rtc_valid_tm(tm);
	if (ret < 0)
		return ret;

	for (year_looper = 0; year_looper < tm->tm_year; year_looper++)
		alarm_days += drtc_ydays[is_leap_year(year_looper + 1900)][12];

	alarm_days += drtc_ydays[is_leap_year(year_looper + 1900)][tm->tm_mon];
	tm->tm_mday--; /*subtract the day which is not ended*/
	alarm_days += tm->tm_mday;

	/*Check date parameter for maximum register setting*/
	if (alarm_days < current_days) {
		pr_err("Invalid parameter!\n");
		return E_PAR;
	}

	alm_timer_reg.reg = 0;
	alm_timer_reg.bit.sec = tm->tm_sec;
	alm_timer_reg.bit.min = tm->tm_min;
	alm_timer_reg.bit.hour = tm->tm_hour;
	drtc_setreg(DRTC_ALARM_TIMER_REG_OFS, alm_timer_reg.reg);

	alm_day_reg.reg = 0;
	alm_day_reg.bit.day = alarm_days;
	drtc_setreg(DRTC_ALARM_DAY_REG_OFS, alm_day_reg.reg);

	unl_cpu();

	return ret;
}

static const struct rtc_class_ops nvt_drtc_ops = {
	.ioctl		= nvt_drtc_ioctl,
	.proc		= nvt_drtc_proc,
	.read_time	= nvt_drtc_read_time,
	.set_time	= nvt_drtc_set_time,
	.read_alarm	= nvt_drtc_read_alarm,
	.set_alarm	= nvt_drtc_set_alarm,
};

static int nvt_drtc_probe(struct platform_device *pdev)
{
	struct rtc_device *drtc;
	struct nvt_drtc_priv *priv;
	struct resource *memres = NULL;
	int ret = 0;

	/* setup resource */
	memres = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (unlikely(!memres)) {
		dev_err(&pdev->dev, "failed to get resource\n");
		return -ENXIO;
	}

	_REGIOBASE = (u32)  devm_ioremap_resource(&pdev->dev, memres);;
	if (unlikely(_REGIOBASE == 0)) {
		dev_err(&pdev->dev, "failed to get io memory\n");
		goto out;
	}

	priv = kzalloc(sizeof(struct nvt_drtc_priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	platform_set_drvdata(pdev, priv);

	device_init_wakeup(&pdev->dev, 1);

	drtc = rtc_device_register("nvt_drtc", &pdev->dev,
				  &nvt_drtc_ops, THIS_MODULE);

	if (IS_ERR(drtc)) {
		ret = PTR_ERR(drtc);
		goto out;
	}
	priv->rtc = drtc;

#ifdef NVT_DRTC_CLK_SUPPORT
	/* resource will auto free when device detached */
	priv->clk = devm_clk_get(&pdev->dev, dev_name(&pdev->dev));
	if (IS_ERR(priv->clk)) {
		dev_err(&pdev->dev, "failed to find drtc clock\n");
		return PTR_ERR(priv->clk);
	}
	clk_prepare_enable(priv->clk);
#endif

	sema_init(&drtc_sem, 1);

	return 0;

out:
	if (priv->rtc)
		rtc_device_unregister(priv->rtc);

	kfree(priv);
	return ret;
}

static int nvt_drtc_remove(struct platform_device *pdev)
{
	struct nvt_drtc_priv *priv = platform_get_drvdata(pdev);

	rtc_device_unregister(priv->rtc);
	kfree(priv);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id nvt_drtc_of_dt_ids[] = {
    { .compatible = "nvt,nvt_drtc", },
    {},
};
#endif

static struct platform_driver nvt_drtc_platform_driver = {
	.driver		= {
		.name	= "nvt_drtc",
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = nvt_drtc_of_dt_ids,
#endif
	},
	.probe		= nvt_drtc_probe,
	.remove		= nvt_drtc_remove,
};

static int __init nvt_drtc_init(void)
{
	int ret;
	ret = platform_driver_register(&nvt_drtc_platform_driver);
	return ret;
}

static void __exit nvt_drtc_exit(void)
{
	platform_driver_unregister(&nvt_drtc_platform_driver);
}

MODULE_AUTHOR("Novatek");
MODULE_DESCRIPTION("nvt DRTC driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
MODULE_ALIAS("platform:drtc-nvt");

module_init(nvt_drtc_init);
module_exit(nvt_drtc_exit);
