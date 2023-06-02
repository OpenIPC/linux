/*
 *  driver/rtc/rtc-na51068.c
 *
 *  Author:     h_h_chuang@novatek.com.tw
 *  Created:	Oct 28, 2019
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
#include <plat/rtc_reg.h>
#include <plat/rtc_int.h>
#include <plat/hardware.h>
#include <linux/clk.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/semaphore.h>

#define DRV_VERSION		"1.00.000"

static struct completion cset_completion;
static struct semaphore rtc_sem;

unsigned int _REGIOBASE;

#define loc_cpu() down(&rtc_sem);
#define unl_cpu() up(&rtc_sem);

//void rtc_wait_pwr_alarm_done(void);
void rtc_trigger_cset(void);

static const unsigned short rtc_ydays[2][13] = {
	/* Normal years */
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
	/* Leap years */
	{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};

struct nvt_rtc_priv {
	struct rtc_device	*rtc;
	struct clk		*clk;
};

static void rtc_setreg(uint32_t offset, REGVALUE value)
{
	nvt_writel(value, _REGIOBASE + offset);
}

static REGVALUE rtc_getreg(uint32_t offset)
{
	return nvt_readl(_REGIOBASE + offset);
}

#if 0
static void nvt_rtc_add_one_sec(u32* sec, u32* min, u32* hour, u32* days)
{
	if (*sec == 59) {
		if (*min == 59) {
			*hour += 1;
			*min = 0;
			*sec = 0;
		} else {
			*min += 1;
			*sec = 0;
		}
	} else {
		*sec += 1;
	}

	if (*hour > 23) {
		*hour = 0;
		*days += 1;
	}
}
#endif


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

static irqreturn_t rtc_update_handler(int irq, void *data)
{
	union RTC_STATUS_REG status_reg;
	union RTC_PWBC_REG pwbc_reg;
	union RTC_PWBCSTS_REG pwbcsts_reg;

	status_reg.reg = rtc_getreg(RTC_STATUS_REG_OFS);

	if (status_reg.bit.alarm_sts || status_reg.bit.cset_sts) {
		rtc_setreg(RTC_STATUS_REG_OFS, status_reg.reg);

		/*CSET done*/
		if (status_reg.bit.cset_sts) {
			complete(&cset_completion);
		}
	}

	pwbcsts_reg.reg = rtc_getreg(RTC_PWBCSTS_REG_OFS);
	pwbc_reg.reg = rtc_getreg(RTC_PWBC_REG_OFS);

	pwbcsts_reg.reg &= pwbc_reg.reg;
	if (pwbcsts_reg.reg)
		rtc_setreg(RTC_PWBCSTS_REG_OFS, pwbcsts_reg.reg);

	return IRQ_HANDLED;
}

void rtc_trigger_cset(void)
{
	union RTC_STATUS_REG status_reg;
	union RTC_CTRL_REG ctrl_reg;

	/*Wait for RTC SRST done*/
	do {
		status_reg.reg = rtc_getreg(RTC_STATUS_REG_OFS);
	} while (status_reg.bit.srst_sts == 1);

	/*Wait for RTC is ready for next CSET*/
	do {
		ctrl_reg.reg = rtc_getreg(RTC_CTRL_REG_OFS);
	} while (ctrl_reg.bit.cset == 1);

	/*Trigger CSET*/
	ctrl_reg.reg = rtc_getreg(RTC_CTRL_REG_OFS);
	ctrl_reg.bit.cset = 1;
	ctrl_reg.bit.cset_inten = 1;
	rtc_setreg(RTC_CTRL_REG_OFS, ctrl_reg.reg);
}

#if 0
void rtc_wait_pwr_alarm_done(void)
{
	union RTC_PWBC_REG pwbc_reg;
	do
	{
		pwbc_reg.reg = rtc_getreg(RTC_PWBC_REG_OFS);
	} while ((pwbc_reg.bit.pwralarm_en == 1) || \
			(pwbc_reg.bit.pwralarm_dis == 1));
}
#endif

#ifdef CONFIG_RTC_INTF_DEV

static int nvt_rtc_ioctl(struct device *dev, unsigned int cmd, \
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
#define nvt_rtc_ioctl    NULL
#endif

#ifdef CONFIG_PROC_FS

static int nvt_rtc_proc(struct device *dev, struct seq_file *seq)
{
	return 0;
}

#else
#define nvt_rtc_proc     NULL
#endif

static int nvt_rtc_reset(struct device *dev)
{
	ulong timeout;
	union RTC_STATUS_REG status_reg;
	union RTC_CTRL_REG ctrl_reg;

	// Wait for previous SRST done
	do {
		status_reg.reg = rtc_getreg(RTC_STATUS_REG_OFS);
	} while (status_reg.bit.srst_sts);

	// Wait for RTC is ready for next CSET
	do {
		ctrl_reg.reg = rtc_getreg(RTC_CTRL_REG_OFS);
	} while (ctrl_reg.bit.cset);

	// Do software reset
	ctrl_reg.reg = 0;
	ctrl_reg.bit.srst = 1;
	rtc_setreg(RTC_CTRL_REG_OFS, ctrl_reg.reg);

	// Wait for RTC SRST finished, RTC can do other operations except SRST
	timeout = jiffies + msecs_to_jiffies(1000);
	do {
		ctrl_reg.reg = rtc_getreg(RTC_CTRL_REG_OFS);

		if (time_after_eq(jiffies, timeout)) {
			dev_err(dev, "RTC s/w reset timeout\n");
			return -ETXTBSY;
		}
	} while(ctrl_reg.bit.srst);

	return 0;
}

static int nvt_rtc_chk_power(struct device *dev)
{
	int ret = 0;
	union RTC_DAYKEY_REG daykey_reg;
	union RTC_CTRL_REG ctrl_reg;
	union RTC_OSCAN_REG oscan_reg;

	daykey_reg.reg = rtc_getreg(RTC_DAYKEY_REG_OFS);
	if (daykey_reg.bit.key & RTC_INT_KEY_POR) {
		printk("%s: RTC power lost detected\r\n", __func__);
		ret = nvt_rtc_reset(dev);
		if (ret) return ret;
	} else {
		return 0;
	}

	loc_cpu();

	/*wait for cset done*/
	wait_for_completion(&cset_completion);

	ctrl_reg.reg = rtc_getreg(RTC_CTRL_REG_OFS);
	ctrl_reg.bit.cset = 0;
	ctrl_reg.bit.key_sel = 1;
	ctrl_reg.bit.pwralarmday_sel = 1;
	rtc_setreg(RTC_CTRL_REG_OFS, ctrl_reg.reg);

	daykey_reg.reg = rtc_getreg(RTC_DAYKEY_REG_OFS);
	daykey_reg.bit.key = RTC_INT_KEY;
	rtc_setreg(RTC_DAYKEY_REG_OFS, daykey_reg.reg);

	/*Set OSC analog parameter*/
	oscan_reg.reg = rtc_getreg(RTC_OSCAN_REG_OFS);
	oscan_reg.bit.osc_analogcfg = RTC_INT_OSC_ANALOG_CFG;
	rtc_setreg(RTC_OSCAN_REG_OFS, oscan_reg.reg);

	rtc_trigger_cset();

	unl_cpu();

	return ret;
}

static int nvt_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	uint32_t days, months, years, month_days;
	union RTC_TIMER_REG timer_reg;
	union RTC_DAYKEY_REG daykey_reg;

	timer_reg.reg = rtc_getreg(RTC_TIMER_REG_OFS);
	daykey_reg.reg = rtc_getreg(RTC_DAYKEY_REG_OFS);
	days = daykey_reg.bit.day;

	for (years = 0; days >= rtc_ydays[is_leap_year(years + 1900)][12]; \
	years++) {
		days -= rtc_ydays[is_leap_year(years + 1900)][12];
	}

	for (months = 1; months < 13; months++) {
		if (days <= rtc_ydays[is_leap_year(years + 1900)][months]) {
			days -= rtc_ydays[is_leap_year(years + 1900)][months-1];
			months--;
			break;
		}
	}

	month_days = rtc_ydays[is_leap_year(years + 1900)][months+1] - \
		rtc_ydays[is_leap_year(years + 1900)][months];

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

static int nvt_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	int year_looper, ret;
	uint32_t days = 0;
	union RTC_TIMER_REG timer_reg;
	union RTC_DAYKEY_REG daykey_reg;
	union RTC_CTRL_REG ctrl_reg;

	pr_debug("kernel set time: sec = %d, min = %d, hour = %d, mday = %d," \
	"mon = %d, year = %d, wday = %d, yday = %d," \
	"\n", tm->tm_sec, tm->tm_min, tm->tm_hour, tm->tm_mday, \
	tm->tm_mon, tm->tm_year, tm->tm_wday, tm->tm_yday);

	ret = rtc_valid_tm(tm);
	if (ret < 0)
		return ret;

	for (year_looper = 0; year_looper < tm->tm_year; year_looper++)
		days += rtc_ydays[is_leap_year(year_looper + 1900)][12];

	days += rtc_ydays[is_leap_year(year_looper + 1900)][tm->tm_mon];
	tm->tm_mday--; /*subtract the day which is not ended*/
	days += tm->tm_mday;

	loc_cpu();

	/*wait for cset done*/
	wait_for_completion(&cset_completion);

	ctrl_reg.reg = rtc_getreg(RTC_CTRL_REG_OFS);
	ctrl_reg.bit.cset = 0;
	ctrl_reg.bit.day_sel = 1;
	ctrl_reg.bit.time_sel = 1;
	rtc_setreg(RTC_CTRL_REG_OFS, ctrl_reg.reg);

	timer_reg.reg = 0;
	timer_reg.bit.sec = tm->tm_sec;
	timer_reg.bit.min = tm->tm_min;
	timer_reg.bit.hour = tm->tm_hour;
	rtc_setreg(RTC_TIMER_REG_OFS, timer_reg.reg);

	daykey_reg.reg = rtc_getreg(RTC_DAYKEY_REG_OFS);
	daykey_reg.bit.day = days;
	rtc_setreg(RTC_DAYKEY_REG_OFS, daykey_reg.reg);

	rtc_trigger_cset();

	unl_cpu();

	return ret;
}

static int nvt_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct rtc_time *tm = &alrm->time;
	uint32_t days, alarm_days, months, years, month_days;
	union RTC_PWRALM_REG pwralarm_reg;
	union RTC_DAYKEY_REG daykey_reg;

	pwralarm_reg.reg = rtc_getreg(RTC_PWRALM_REG_OFS);
	daykey_reg.reg = rtc_getreg(RTC_DAYKEY_REG_OFS);

	alarm_days = pwralarm_reg.bit.day;
	days = daykey_reg.bit.day;

	if ((days & RTC_INT_MAX_PWRALARM_DAY) > alarm_days) {
		days = ((days & ~RTC_INT_MAX_PWRALARM_DAY) | alarm_days) \
			+ (1 << RTC_INT_PRWALARM_DAY_SHIFT);
	} else
		days = ((days & ~RTC_INT_MAX_PWRALARM_DAY) | alarm_days);

	for (years = 0; days >= rtc_ydays[is_leap_year(years + 1900)][12]; \
	years++) {
		days -= rtc_ydays[is_leap_year(years + 1900)][12];
	}

	for (months = 1; months < 13; months++) {
		if (days <= rtc_ydays[is_leap_year(years + 1900)][months]) {
			days -= rtc_ydays[is_leap_year(years + 1900)][months-1];
			months--;
			break;
		}
	}

	month_days = rtc_ydays[is_leap_year(years + 1900)][months+1] - \
		rtc_ydays[is_leap_year(years + 1900)][months];

	if (days == month_days) {
		months++;
		days = 1;
	} else
		days++; /*Align linux time format*/

	tm->tm_sec  = pwralarm_reg.bit.sec;
	tm->tm_min  = pwralarm_reg.bit.min;
	tm->tm_hour = pwralarm_reg.bit.hour;
	tm->tm_mday = days;
	tm->tm_mon  = months;
	tm->tm_year = years;

	pr_debug("read alarm time: sec = %d, min = %d, hour = %d, mday = %d," \
	"mon = %d, year = %d, wday = %d, yday = %d," \
	"\n", tm->tm_sec, tm->tm_min, tm->tm_hour, tm->tm_mday, \
	tm->tm_mon, tm->tm_year, tm->tm_wday, tm->tm_yday);

	return rtc_valid_tm(tm);
}

static int nvt_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct rtc_time *tm = &alrm->time;
	int year_looper, ret;
	uint32_t current_days = 0, alarm_days = 0;
	union RTC_DAYKEY_REG daykey_reg;
	union RTC_CTRL_REG ctrl_reg;
	union RTC_PWRALM_REG pwralarm_reg;
	union RTC_PWBC_REG pwbc_reg;
	union RTC_OSCAN_REG oscan_reg;

	daykey_reg.reg = rtc_getreg(RTC_DAYKEY_REG_OFS);
	current_days = daykey_reg.bit.day;

	pr_debug("set alarm time: sec = %d, min = %d, hour = %d, mday = %d," \
	"mon = %d, year = %d, wday = %d, yday = %d," \
	"\n", tm->tm_sec, tm->tm_min, tm->tm_hour, tm->tm_mday, \
	tm->tm_mon, tm->tm_year, tm->tm_wday, tm->tm_yday);

	ret = rtc_valid_tm(tm);
	if (ret < 0)
		return ret;

	for (year_looper = 0; year_looper < tm->tm_year; year_looper++)
		alarm_days += rtc_ydays[is_leap_year(year_looper + 1900)][12];

	alarm_days += rtc_ydays[is_leap_year(year_looper + 1900)][tm->tm_mon];
	tm->tm_mday--; /*subtract the day which is not ended*/
	alarm_days += tm->tm_mday;

	/*Check date parameter for maximum register setting*/
	if ((alarm_days < current_days) || ((alarm_days - current_days) > \
	RTC_INT_MAX_PWRALARM_DAY)) {
		pr_err("Invalid parameter!\n");
		return E_PAR;
	}

	wait_for_completion(&cset_completion);

	ctrl_reg.reg = rtc_getreg(RTC_CTRL_REG_OFS);
	ctrl_reg.bit.cset = 0;
//	ctrl_reg.bit.pwralarmtime_sel = 1;
	ctrl_reg.bit.pwralarmday_sel = 1;
	ctrl_reg.bit.alarm_inten = 1;
	rtc_setreg(RTC_CTRL_REG_OFS, ctrl_reg.reg);

	pwralarm_reg.reg = 0;
	pwralarm_reg.bit.sec = tm->tm_sec;
	pwralarm_reg.bit.min = tm->tm_min;
	pwralarm_reg.bit.hour = tm->tm_hour;
	pwralarm_reg.bit.day = alarm_days;
	rtc_setreg(RTC_PWRALM_REG_OFS, pwralarm_reg.reg);

	/*Set OSC analog parameter*/
	oscan_reg.reg = rtc_getreg(RTC_OSCAN_REG_OFS);
	oscan_reg.bit.osc_analogcfg = RTC_INT_OSC_ANALOG_CFG;
	rtc_setreg(RTC_OSCAN_REG_OFS, oscan_reg.reg);

	rtc_trigger_cset();

//	rtc_wait_pwr_alarm_done();

	pwbc_reg.reg = rtc_getreg(RTC_PWBC_REG_OFS);
	pwbc_reg.bit.pwralarm_en = 1;
	rtc_setreg(RTC_PWBC_REG_OFS, oscan_reg.reg);

	return ret;
}

static const struct rtc_class_ops nvt_rtc_ops = {
	.ioctl		= nvt_rtc_ioctl,
	.proc		= nvt_rtc_proc,
	.read_time	= nvt_rtc_read_time,
	.set_time	= nvt_rtc_set_time,
	.read_alarm	= nvt_rtc_read_alarm,
	.set_alarm	= nvt_rtc_set_alarm,
};

static int nvt_rtc_probe(struct platform_device *pdev)
{
	struct rtc_device *rtc;
	struct nvt_rtc_priv *priv;
	struct device *dev = &pdev->dev;
	struct resource *memres = NULL;
	int ret = 0, irq = 0;
#ifdef CONFIG_NVT_FPGA_EMULATION
	union RTC_OSCAN_REG oscan_reg;
#endif

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

	priv = kzalloc(sizeof(struct nvt_rtc_priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->clk = devm_clk_get(dev, dev_name(dev));
        if (WARN_ON(IS_ERR(priv->clk)))
                return PTR_ERR(priv->clk);

        clk_prepare_enable(priv->clk);

#ifdef CONFIG_NVT_FPGA_EMULATION
	printk("%s: try rtc\r\n", __func__);
	oscan_reg.reg = rtc_getreg(RTC_OSCAN_REG_OFS);
	if (oscan_reg.reg == 0) {
		printk("%s: reg 0x%x\r\n", __func__, oscan_reg.reg);
		printk("%s: this FPGA doesn't have RTC\r\n", __func__);
		ret = PTR_ERR(rtc);
		goto out;
	}
#endif

	platform_set_drvdata(pdev, priv);

	device_init_wakeup(&pdev->dev, 1);

	rtc = rtc_device_register("nvt_rtc", &pdev->dev,
				  &nvt_rtc_ops, THIS_MODULE);

	if (IS_ERR(rtc)) {
		ret = PTR_ERR(rtc);
		goto out;
	}
	priv->rtc = rtc;

	/*Set default HW configuration*/
	/*Define what data type will be used, RCW_DEF or manual define*/

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "No IRQ resource!\n");
		goto out;
	}

	ret = devm_request_irq(&pdev->dev, irq, rtc_update_handler, 0, pdev->name,
			NULL);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to request IRQ: #%d: %d\n", irq, ret);
		return ret;
	}

	init_completion(&cset_completion);
	sema_init(&rtc_sem, 1);
	complete(&cset_completion);

	nvt_rtc_chk_power(&pdev->dev);

	return 0;

out:
	if (priv->rtc)
		rtc_device_unregister(priv->rtc);

	kfree(priv);
	return ret;
}

static int nvt_rtc_remove(struct platform_device *pdev)
{
	struct nvt_rtc_priv *priv = platform_get_drvdata(pdev);

	rtc_device_unregister(priv->rtc);
	clk_disable_unprepare(priv->clk);
	kfree(priv);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id nvt_rtc_of_dt_ids[] = {
    { .compatible = "nvt,nvt_rtc", },
    {},
};
#endif

static struct platform_driver nvt_rtc_platform_driver = {
	.driver		= {
		.name	= "nvt_rtc",
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = nvt_rtc_of_dt_ids,
#endif
	},
	.probe		= nvt_rtc_probe,
	.remove		= nvt_rtc_remove,
};

static int __init nvt_rtc_init(void)
{
	int ret;
	ret = platform_driver_register(&nvt_rtc_platform_driver);
	return ret;
}

static void __exit nvt_rtc_exit(void)
{
	platform_driver_unregister(&nvt_rtc_platform_driver);
}

MODULE_AUTHOR("Novatek");
MODULE_DESCRIPTION("nvt RTC driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
MODULE_ALIAS("platform:rtc-nvt");

module_init(nvt_rtc_init);
module_exit(nvt_rtc_exit);
