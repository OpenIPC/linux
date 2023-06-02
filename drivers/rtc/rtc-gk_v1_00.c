#include <linux/bcd.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

#include <mach/hardware.h>
#include <mach/sd.h>
#include <mach/io.h>
#include <mach/gpio.h>
#include <mach/event.h>
#include <plat/cache.h>

#define ADDR_SHIFT              2

#define PWRMGR_BASE             GK_VA_PMU_RTC

#define MCU_RTC_BASE            (PWRMGR_BASE + (0x0000 << ADDR_SHIFT))
#define MCU_SYS_BASE            (PWRMGR_BASE + (0x2800 << ADDR_SHIFT))

/* System configure 17 register */
#define SYS_REG_CFG17               (MCU_SYS_BASE + (0x0011 << ADDR_SHIFT))
#define CFG17_RTC_CLK_SEL_32K       (1<<0)
#define CFG17_RTC_CLK_SEL_24M       (0<<0)
#define CFG17_RTC_SOFT_RESET        (1<<1)

/********************************/
/*     RTC Register List        */
/********************************/
#define RTC_SYS_CONFIG          (MCU_RTC_BASE + (0x00 << ADDR_SHIFT))
#define RTC_IRQ_ENABLE          (1<<5)
#define RTC_PAUSE_ENABLE        (1<<4)
#define RTC_STOP_ENABLE         (1<<3)
#define RTC_ENABLE              (1<<0)

#define RTC_P_SCALER_DIV0       (MCU_RTC_BASE + (0x01 << ADDR_SHIFT))
#define RTC_P_SCALER_DIV1       (MCU_RTC_BASE + (0x02 << ADDR_SHIFT))
#define RTC_P_SCALER_DIV2       (MCU_RTC_BASE + (0x03 << ADDR_SHIFT))
#define RTC_P_SCALER_DIV3       (MCU_RTC_BASE + (0x04 << ADDR_SHIFT))

#define RTC_PRE_LOAD_HOUR       (MCU_RTC_BASE + (0x05 << ADDR_SHIFT))
#define RTC_PRE_LOAD_MIN        (MCU_RTC_BASE + (0x06 << ADDR_SHIFT))
#define RTC_PRE_LOAD_SEC        (MCU_RTC_BASE + (0x07 << ADDR_SHIFT))

#define RTC_IRQ_CLEAR           (MCU_RTC_BASE + (0x08 << ADDR_SHIFT))
#define RTC_IRQ_CLR             (1<<0)

#define RTC_PRE_LOAD_DAY_L      (MCU_RTC_BASE + (0x09 << ADDR_SHIFT))
#define RTC_PRE_LOAD_DAY_H      (MCU_RTC_BASE + (0x0A << ADDR_SHIFT))

#define RTC_IRQ_STATUS          (MCU_RTC_BASE + (0x1A << ADDR_SHIFT))
#define RTC_IRQ_GEN             (1<<0)

#define RTC_HOUR_VALUE          (MCU_RTC_BASE + (0x1D << ADDR_SHIFT))
#define RTC_MIN_VALUE           (MCU_RTC_BASE + (0x1C << ADDR_SHIFT))
#define RTC_SEC_VALUE           (MCU_RTC_BASE + (0x1B << ADDR_SHIFT))

#define RTC_DAY_L_VALUE         (MCU_RTC_BASE + (0x1E << ADDR_SHIFT))
#define RTC_DAY_H_VALUE         (MCU_RTC_BASE + (0x1F << ADDR_SHIFT))

static unsigned short gk_rtc_get_day(void);

static void gk_rtc_set_clock(void)
{
#ifdef CONFIG_CLOCK_32K
    // 32.768KHz clock
    gk_mcu_writel( SYS_REG_CFG17, CFG17_RTC_CLK_SEL_32K);
    gk_mcu_writel( RTC_P_SCALER_DIV3, 0x00);
    gk_mcu_writel( RTC_P_SCALER_DIV2, 0x00);
    gk_mcu_writel( RTC_P_SCALER_DIV1, 0x7f);
    gk_mcu_writel( RTC_P_SCALER_DIV0, 0xff);
#endif

#ifdef CONFIG_CLOCK_24M
    gk_mcu_writel( SYS_REG_CFG17, CFG17_RTC_CLK_SEL_24M);
    gk_mcu_writel( RTC_P_SCALER_DIV3, 0x01);
    gk_mcu_writel( RTC_P_SCALER_DIV2, 0x6e);
    gk_mcu_writel( RTC_P_SCALER_DIV1, 0x35);
    gk_mcu_writel( RTC_P_SCALER_DIV0, 0xff);
#endif
}

static void gk_rtc_stop(void)
{
    /* clear out the hardware. */
    gk_mcu_writel( RTC_SYS_CONFIG, 9);

}

static void gk_rtc_reset(void)
{
    unsigned char syscfg17;

    // config
    gk_mcu_writel( RTC_SYS_CONFIG, 0x0);
    gk_mcu_writel( RTC_SYS_CONFIG, (RTC_IRQ_ENABLE | RTC_ENABLE));

    //reset rtc
    syscfg17 = gk_mcu_readl( SYS_REG_CFG17);
    syscfg17 |= CFG17_RTC_SOFT_RESET;
    gk_mcu_writel( SYS_REG_CFG17, syscfg17);
}

static void gk_rtc_start(void)
{
    unsigned char syscfg17;

    gk_rtc_set_clock();
    printk("rtc start...\n");
    //if(gk_rtc_get_day() != 0) return;

    // config
    gk_mcu_writel( RTC_SYS_CONFIG, 0x0);
    gk_mcu_writel( RTC_SYS_CONFIG, (RTC_ENABLE));

    syscfg17 = gk_mcu_readl( SYS_REG_CFG17);
    syscfg17 |= CFG17_RTC_SOFT_RESET;
    gk_mcu_writel( SYS_REG_CFG17, syscfg17);
}

static unsigned short gk_rtc_get_day(void)
{
    unsigned char dayh;
    unsigned char dayl;

    dayh = gk_mcu_readl(RTC_DAY_H_VALUE);
    dayl = gk_mcu_readl(RTC_DAY_L_VALUE);

    //printk("read day = %d - %d \n", dayh, dayl);

    return  (((unsigned short)dayh << 8)|(unsigned short)dayl);
}

static unsigned long gk_rtc_get_time(void)
{
    int h,m,s;

    h = gk_mcu_readl(RTC_HOUR_VALUE);
    m = gk_mcu_readl(RTC_MIN_VALUE );
    s = gk_mcu_readl(RTC_SEC_VALUE );

    //printk("read time: h=%d m=%d s=%d \n", h, m, s);

    return (unsigned long)((h*3600) + m*60 + s);
}

static void gk_rtc_set_day(unsigned short day)
{
    unsigned char dayh;
    unsigned char dayl;

    dayh = ((day >> 8) & 0xff);
    dayl = ((day >> 0) & 0xff);

    //printk("set dayh=%d dayl=%d \n", dayh, dayl);

    gk_mcu_writel(RTC_PRE_LOAD_DAY_H, dayh);
    gk_mcu_writel(RTC_PRE_LOAD_DAY_L, dayl);
}

static void gk_rtc_write_time(unsigned char hour, unsigned char min, unsigned char sec)
{
    //printk("set time: h=%d m=%d s=%d \n", hour, min, sec);
    gk_mcu_writel(RTC_PRE_LOAD_HOUR, hour);
    gk_mcu_writel(RTC_PRE_LOAD_MIN,  min);
    gk_mcu_writel(RTC_PRE_LOAD_SEC,  sec);
}

static int gk_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
    unsigned long t;
    unsigned short day;
    unsigned long sec;
#ifdef CONFIG_PMU_ALWAYS_RUNNING
    // get pmu controller
    gk_gpio_config(GPIO_12, GPIO_TYPE_OUTPUT_1);
    msleep(1);
    gk_gpio_config(GPIO_12, GPIO_TYPE_OUTPUT_0);
    msleep(1);
#endif
    day = gk_rtc_get_day();
    sec = gk_rtc_get_time();

    t = day * 3600 * 24 + sec;
    printk("os read tm: t=%ld \n", t);
    rtc_time_to_tm(t, tm);
#ifdef CONFIG_PMU_ALWAYS_RUNNING
    // release pmu controller & reset pmu
    gk_mcu_writel(GK_VA_PMU + 0xA05C, PMU_ALWAYS_RUNNING);
    gk_mcu_writel(GK_VA_PMU + 0xA02C, 0xB2);    // 1011 0010
    gk_mcu_writel(GK_VA_PMU + 0xA02C, 0x63);    // 0110 0011
#endif
    return rtc_valid_tm(tm);
}

static int gk_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
    unsigned long t;
    unsigned short day;
    unsigned long left;
    unsigned char h,m,s;

#ifdef CONFIG_PMU_ALWAYS_RUNNING
    // get pmu controller
    gk_gpio_config(GPIO_12, GPIO_TYPE_OUTPUT_1);
    msleep(1);
    gk_gpio_config(GPIO_12, GPIO_TYPE_OUTPUT_0);
    msleep(1);
#endif
    rtc_tm_to_time(tm, &t);

    day = t / (3600*24);
    left = t % (3600*24);

    h = left / 3600;
    left = left % 3600;

    m = left /60;
    s = left % 60;

    printk("os set time: day=%d h=%d m=%d s=%d t=%ld\n", day, h, m, s, t);

    gk_rtc_stop();
    gk_rtc_set_day(day);
    gk_rtc_write_time(h, m, s);
    // Steven Yu: use start replace reset.
    //gk_rtc_reset();
    gk_rtc_start();

#ifdef CONFIG_PMU_ALWAYS_RUNNING
    // release pmu controller & reset pmu
    gk_mcu_writel(GK_VA_PMU + 0xA05C, PMU_ALWAYS_RUNNING);
    gk_mcu_writel(GK_VA_PMU + 0xA02C, 0xB2);    // 1011 0010
    gk_mcu_writel(GK_VA_PMU + 0xA02C, 0x63);    // 0110 0011
#endif
    return 0;
}

static struct rtc_class_ops gk_rtc_ops =
{
    .read_time  = gk_rtc_read_time,
    .set_time   = gk_rtc_set_time,
};



static int __devinit gk_rtc_probe(struct platform_device *pdev)
{
    struct rtc_device *rtcdev;

    int ret;


    printk("rtc base: 0x%x \n", MCU_RTC_BASE);

    //gk_rtc_stop();

    // Steven Yu: do not reset rtc, otherwise the time would be reset or stop.
    // start rtc in gk_rtc_set_time
    //gk_rtc_start();

    rtcdev = rtc_device_register("gk-rtc", &pdev->dev, &gk_rtc_ops, THIS_MODULE);
    if (IS_ERR(rtcdev))
    {
        ret = PTR_ERR(rtcdev);
        goto out_err;
    }

    platform_set_drvdata(pdev, rtcdev);

    return 0;

out_err:
    return ret;
}

static int __devexit gk_rtc_remove(struct platform_device *pdev)
{
    struct rtc_device *rtcdev = platform_get_drvdata(pdev);

    rtc_device_unregister(rtcdev);
    platform_set_drvdata(pdev, NULL);

    return 0;
}

MODULE_ALIAS("platform:gk_rtc");
static struct platform_driver gk_rtc_driver =
{
    .remove     = __devexit_p(gk_rtc_remove),
    .driver     =
    {
        .name   = "gk-rtc",
        .owner  = THIS_MODULE,
    },
};

static int __init gk_rtc_init(void)
{
    printk("gk rtc init...\n");
    return platform_driver_probe(&gk_rtc_driver, gk_rtc_probe);
}
module_init(gk_rtc_init);

static void __exit gk_rtc_exit(void)
{
    platform_driver_unregister(&gk_rtc_driver);
}
module_exit(gk_rtc_exit);

MODULE_AUTHOR("louis <liangyou@gokemicro.com>");
MODULE_DESCRIPTION("driver for gk internal RTC");
MODULE_LICENSE("GPL");

