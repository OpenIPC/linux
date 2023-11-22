/*
 * drv_rtcpwc.c- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/rtc.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/of_irq.h>

#include <ms_platform.h>
#include <ms_types.h>
#include <ms_msys.h>
#include <hal_rtcpwc.h>

#define DTS_DEFAULT_DATE "default-date"

struct sstar_rtc_info
{
    u8                      rtc_inited;
    struct platform_device *rtc_pdev;
    struct rtc_device *     rtc_dev;
    struct hal_rtcpwc_t     rtc_hal;
    struct mutex            mutex;
};

static struct sstar_rtc_info *sstar_rtc_point;

static ssize_t count_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "iso   count  : %d\n", sstar_rtc_point->rtc_hal.fail_count.iso_fail);
    str += scnprintf(str, end - str, "read  count  : %d\n", sstar_rtc_point->rtc_hal.fail_count.read_fail);
    str += scnprintf(str, end - str, "clock count  : %d\n", sstar_rtc_point->rtc_hal.fail_count.clock_fail);
    return (str - buf);
}
DEVICE_ATTR(count_status, 0444, count_status_show, NULL);

#ifdef CONFIG_RTC_WITH_PWC
static ssize_t wakeup_event_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *src;
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    mutex_lock(&sstar_rtc_point->mutex);
    src = hal_rtc_get_wakeup_name(&sstar_rtc_point->rtc_hal);
    mutex_unlock(&sstar_rtc_point->mutex);

    str += scnprintf(str, end - str, "wakeup event: %s\n", src);
    return (str - buf);
}
DEVICE_ATTR(wakeup_event, 0444, wakeup_event_show, NULL);

static ssize_t event_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int    i;
    u16    state;
    char **name;
    char * str = buf;
    char * end = buf + PAGE_SIZE;

    mutex_lock(&sstar_rtc_point->mutex);
    name  = hal_rtc_get_event_name(&sstar_rtc_point->rtc_hal);
    state = hal_rtc_get_event_state(&sstar_rtc_point->rtc_hal);
    mutex_unlock(&sstar_rtc_point->mutex);

    str += scnprintf(str, end - str, "event state:\n");

    for (i = 0; i < RTC_STATE_MAX; i++)
    {
        if (name[i])
        {
            if (state & (1 << i))
            {
                str += scnprintf(str, end - str, "%s : ON\n", name[i]);
            }
            else
            {
                str += scnprintf(str, end - str, "%s : OFF\n", name[i]);
            }
        }
    }

    return (str - buf);
}
DEVICE_ATTR(event_state, 0444, event_state_show, NULL);
#endif

static ssize_t alarm_timer_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    if (NULL != buf)
    {
        size_t      len;
        const char *str     = buf;
        u32         seconds = 0;
        while (*str && !isspace(*str))
            str++;
        len = str - buf;
        if (len)
        {
            seconds = simple_strtoul(buf, NULL, 10);
            mutex_lock(&sstar_rtc_point->mutex);
            hal_rtc_set_alarm(&sstar_rtc_point->rtc_hal, seconds);
            hal_rtc_alarm_enable(&sstar_rtc_point->rtc_hal, 1);
            mutex_unlock(&sstar_rtc_point->mutex);
            return n;
        }
        return -EINVAL;
    }
    return -EINVAL;
}
static ssize_t alarm_timer_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int           rest    = 0;
    unsigned long run_sec = 0;
    char *        str     = buf;
    char *        end     = buf + PAGE_SIZE;

    mutex_lock(&sstar_rtc_point->mutex);
    run_sec = hal_rtc_get_rddata(&sstar_rtc_point->rtc_hal);
    mutex_unlock(&sstar_rtc_point->mutex);
    rest = (sstar_rtc_point->rtc_hal.alarm_time > run_sec) ? (sstar_rtc_point->rtc_hal.alarm_time - run_sec) : 0;

    if (rest < 0)
        rest = 0;

    str += scnprintf(str, end - str, "countdown time = %d\n", rest);
    return (str - buf);
}
DEVICE_ATTR(alarm_timer, 0644, alarm_timer_show, alarm_timer_store);

static ssize_t alarm_interrupt_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    if (NULL != buf)
    {
        size_t      len;
        const char *str = buf;
        while (*str && !isspace(*str))
            str++;
        len = str - buf;
        if (len && strcmp(buf, "clear"))
        {
            mutex_lock(&sstar_rtc_point->mutex);
            hal_rtc_clear_alarm_int(&sstar_rtc_point->rtc_hal);
            mutex_unlock(&sstar_rtc_point->mutex);
            return n;
        }
        return -EINVAL;
    }
    return -EINVAL;
}

static ssize_t alarm_interrupt_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u32   ret = 0;
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    mutex_lock(&sstar_rtc_point->mutex);
    ret = hal_rtc_get_alarm_int(&sstar_rtc_point->rtc_hal);
    mutex_unlock(&sstar_rtc_point->mutex);

    if (ret)
    {
        str += scnprintf(str, end - str, "alarm interrupt actived\n");
    }
    else
    {
        str += scnprintf(str, end - str, "alarm interrupt inactived\n");
    }
    return (str - buf);
}
DEVICE_ATTR(alarm_interrupt, 0644, alarm_interrupt_show, alarm_interrupt_store);

static ssize_t offset_count_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    int ret    = 0;
    s16 offset = 0;

    if (NULL != buf)
    {
        size_t      len;
        const char *str = buf;
        while (*str && !isspace(*str))
            str++;
        len = str - buf;
        if (len)
        {
            ret = kstrtos16(buf, 10, &offset);
            if (!ret)
            {
                if (offset > -256 && offset < 256)
                {
                    mutex_lock(&sstar_rtc_point->mutex);
                    hal_rtc_set_offset(&sstar_rtc_point->rtc_hal, offset);
                    mutex_unlock(&sstar_rtc_point->mutex);
                    return n;
                }
                return -ERANGE;
            }

            return ret;
        }
        return -EINVAL;
    }
    return -EINVAL;
}

static ssize_t offset_count_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    s16   offset = 0;
    char *str    = buf;
    char *end    = buf + PAGE_SIZE;

    mutex_lock(&sstar_rtc_point->mutex);
    offset = hal_rtc_get_offset(&sstar_rtc_point->rtc_hal);
    mutex_unlock(&sstar_rtc_point->mutex);

    str += scnprintf(str, end - str, "offset count %d\n", offset);

    return (str - buf);
}
DEVICE_ATTR(offset_count, 0644, offset_count_show, offset_count_store);

#ifdef CONFIG_RTC_WITH_PWC
char *sstar_rtc_get_wakeup_source(void)
{
    if (!sstar_rtc_point)
        return NULL;

    if (!sstar_rtc_point->rtc_inited)
        return NULL;

    return hal_rtc_get_wakeup_name(&sstar_rtc_point->rtc_hal);
}
EXPORT_SYMBOL(sstar_rtc_get_wakeup_source);
#endif

static int sstar_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
    u32                    seconds = 0;
    struct sstar_rtc_info *info    = dev_get_drvdata(dev);

    if (!info->rtc_inited)
        return 0;

    mutex_lock(&info->mutex);

    hal_rtc_read_time(&info->rtc_hal, &seconds);

    rtc_time64_to_tm(seconds, tm);

    RTC_DBG("[%d,%d,%d,%d,%d,%d]\n", tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    mutex_unlock(&info->mutex);
    return rtc_valid_tm(tm);
}

static int sstar_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
    unsigned long          seconds;
    struct sstar_rtc_info *info = dev_get_drvdata(dev);

    if (!info->rtc_inited)
        return 0;

    mutex_lock(&info->mutex);
    RTC_DBG("[%d,%d,%d,%d,%d,%d]\n", tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    seconds = rtc_tm_to_time64(tm);

    hal_rtc_set_time(&info->rtc_hal, seconds);

    mutex_unlock(&info->mutex);
    return 0;
}

static int sstar_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
    u32                    seconds;
    struct sstar_rtc_info *info = dev_get_drvdata(dev);

    if (!info->rtc_inited)
        return 0;

    mutex_lock(&info->mutex);

    hal_rtc_get_alarm(&info->rtc_hal, &seconds);

    RTC_DBG("seconds = %#lx\r\n", seconds);

    rtc_time64_to_tm(seconds, &alarm->time);

    RTC_DBG("[%d,%d,%d,%d,%d,%d]\n", alarm->time.tm_year, alarm->time.tm_mon, alarm->time.tm_mday, alarm->time.tm_hour,
            alarm->time.tm_min, alarm->time.tm_sec);

    mutex_unlock(&info->mutex);
    return rtc_valid_tm(&alarm->time);
}

static int sstar_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
    u32                    now;
    u32                    seconds;
    struct sstar_rtc_info *info = dev_get_drvdata(dev);

    if (!info->rtc_inited)
        return 0;

    mutex_lock(&info->mutex);
    RTC_DBG("[%d,%d,%d,%d,%d,%d]\n", alarm->time.tm_year, alarm->time.tm_mon, alarm->time.tm_mday, alarm->time.tm_hour,
            alarm->time.tm_min, alarm->time.tm_sec);

    hal_rtc_alarm_enable(&info->rtc_hal, alarm->enabled);

    seconds = rtc_tm_to_time64(&alarm->time);

    hal_rtc_read_time(&info->rtc_hal, &now);

    seconds -= now;

    RTC_DBG("seconds = %#lx\r\n", seconds);

    hal_rtc_set_alarm(&info->rtc_hal, seconds);

    mutex_unlock(&info->mutex);
    return 0;
}

static int sstar_rtc_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
    struct sstar_rtc_info *info = dev_get_drvdata(dev);

    if (!info->rtc_inited)
        return -EIO;

    mutex_lock(&info->mutex);
    hal_rtc_alarm_enable(&info->rtc_hal, enabled);
    mutex_unlock(&info->mutex);

    return 0;
}

static irqreturn_t sstar_rtc_interrupt(int irq, void *dev_id)
{
    struct sstar_rtc_info *info = (struct sstar_rtc_info *)dev_id;

    RTC_DBG("rtc interrupt is trigger\n");

    hal_rtc_interrupt(&info->rtc_hal);
    rtc_update_irq(info->rtc_dev, 1, RTC_AF);

    return IRQ_HANDLED;
}

static void sstar_rtc_init(struct platform_device *pdev, struct sstar_rtc_info *info)
{
    int ret     = 0;
    int num     = 0;
    s32 offset  = 0;
    u32 irq_num = 0;
#ifdef CONFIG_RTC_WITH_PWC
    u32 val      = 0;
    u32 array[2] = {0};
#endif
    struct rtc_time tm = {0};

    info->rtc_hal.default_base = 0; // 1970/1/1 00:00:00
    if (0 < (num = of_property_count_elems_of_size(pdev->dev.of_node, DTS_DEFAULT_DATE, sizeof(int))))
    {
        if (!of_property_read_u32_array(pdev->dev.of_node, DTS_DEFAULT_DATE, (u32 *)&tm, num))
        {
            if (!rtc_valid_tm(&tm))
            {
                info->rtc_hal.default_base = rtc_tm_to_time64(&tm);
            }
            else
            {
                RTC_ERR("Please check " DTS_DEFAULT_DATE " in dtsi\r\n");
            }
        }
    }

#ifdef CONFIG_RTC_WITH_PWC
    ret = of_property_read_bool(pdev->dev.of_node, "io0-hiz");
    if (ret)
    {
        info->rtc_hal.pwc_io0_hiz = 1;
        RTC_DBG("io0-hiz (%d)\n", 1);
    }

    ret = of_property_read_u32(pdev->dev.of_node, "io2-wos", &val);
    if (ret)
    {
        RTC_DBG("of_property_read_u32 fail (io2-wos) %d\n", ret);
    }
    else
    {
        info->rtc_hal.pwc_io2_valid = TRUE;
        info->rtc_hal.pwc_io2_cmp   = val;
        RTC_DBG("io2-wos (%d)\n", val);

        ret = of_property_read_u32_array(pdev->dev.of_node, "io2-wos-v", array, 2);
        if (ret)
        {
            RTC_DBG("of_property_read_u32_array fail (io2-wos-v) %d\n", ret);
        }
        else
        {
            info->rtc_hal.pwc_io2_vlsel = array[0];
            info->rtc_hal.pwc_io2_vhsel = array[1];
            RTC_DBG("io2-wos-v (%d %d)\n", array[0], array[1]);
        }
    }

    ret = of_property_read_bool(pdev->dev.of_node, "io3-pu");
    if (ret)
    {
        info->rtc_hal.pwc_io3_pu = 1;
        RTC_DBG("io3-pu (%d)\n", 1);
    }

    ret = of_property_read_u32(pdev->dev.of_node, "io4-enable", &val);
    if (ret)
    {
        RTC_DBG("of_property_read_u32 fail (io4-enable) %d\n", ret);
    }
    else
    {
        info->rtc_hal.pwc_io4_valid = TRUE;
        info->rtc_hal.pwc_io4_value = val;
        RTC_DBG("io4-enable (%d)\n", val);
    }

    ret = of_property_read_u32(pdev->dev.of_node, "io5-enable", &val);
    if (ret)
    {
        RTC_DBG("of_property_read_u32 fail (io5-enable) %d\n", ret);
    }
    else
    {
        info->rtc_hal.pwc_io5_valid = TRUE;
        info->rtc_hal.pwc_io5_value = val;
        RTC_DBG("io5-enable (%d)\n", val);
    }

    ret = of_property_read_bool(pdev->dev.of_node, "alarm-init-on");
    if (ret)
    {
        info->rtc_hal.pwc_alm_init_on = 1;
        RTC_DBG("alarm-init-on (%d)\n", 1);
    }

#endif
    ret = of_property_read_bool(pdev->dev.of_node, "alarm-enable");
    if (ret)
    {
        info->rtc_hal.pwc_alm_enable = 1;
        RTC_DBG("alarm-enable (%d)\n", 1);
    }

    if (info->rtc_hal.pwc_alm_enable)
    {
        irq_num = irq_of_parse_and_map(pdev->dev.of_node, 0);
        if (irq_num == 0)
        {
            RTC_ERR("[%s]: can't find interrupts property\n", __func__);
        }
        ret = request_irq(irq_num, sstar_rtc_interrupt, IRQF_TRIGGER_RISING, "RTCPWC", (void *)info);
        if (ret)
        {
            RTC_ERR("[%s]: interrupt register failed\n", __func__);
        }
#ifdef CONFIG_PM_SLEEP
        enable_irq_wake(irq_num);
#endif
    }

    ret = of_property_read_bool(pdev->dev.of_node, "iso-auto-regen");
    if (ret)
    {
        info->rtc_hal.iso_auto_regen = 1;
        RTC_DBG("iso-auto-regen (%d)\n", 1);
    }

    ret = of_property_read_u32(pdev->dev.of_node, "offset-count", &offset);
    if (ret)
    {
        RTC_DBG("of_property_read_s32 fail (offset-count) %d\n", ret);
    }
    else
    {
        ret = of_property_read_bool(pdev->dev.of_node, "offset-nagative");
        if (ret)
        {
            offset = -offset;
        }

        if (offset > -256 && offset < 256)
        {
            info->rtc_hal.offset_count = (s16)offset;
        }
        else
        {
            info->rtc_hal.offset_count = 0;
        }
        RTC_DBG("offset-count (%d)\n", val);
    }
}

static const struct rtc_class_ops sstar_rtcpwc_ops = {
    .read_time        = sstar_rtc_read_time,
    .set_time         = sstar_rtc_set_time,
    .read_alarm       = sstar_rtc_read_alarm,
    .set_alarm        = sstar_rtc_set_alarm,
    .alarm_irq_enable = sstar_rtc_alarm_irq_enable,
};

#ifdef CONFIG_RTC_WITH_PWC
static void sstar_rtcpwc_poweroff(void)
{
    hal_rtc_power_off(&sstar_rtc_point->rtc_hal);
}
#endif

static int sstar_rtcpwc_remove(struct platform_device *pdev)
{
    struct device *        dev  = &pdev->dev;
    struct sstar_rtc_info *info = dev_get_drvdata(dev);

    if (info)
    {
        devm_kfree(dev, info);
        sstar_rtc_point = 0;
    }
    return 0;
}

static int sstar_rtcpwc_probe(struct platform_device *pdev)
{
    struct sstar_rtc_info *info;
    struct resource *      res;
    struct device *        rtc_dev;
    dev_t                  dev;
    int                    ret = 0;
    void __iomem *         rtc_base;

    info = devm_kzalloc(&pdev->dev, sizeof(struct sstar_rtc_info), GFP_KERNEL);
    if (!info)
        return -ENOMEM;
    RTC_DBG("RTC initial\n");
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res)
    {
        RTC_ERR("[%s]: failed to get IORESOURCE_MEM\n", __func__);
        return -ENODEV;
    }
    rtc_base = devm_ioremap_resource(&pdev->dev, res);

    if (IS_ERR(rtc_base))
        return PTR_ERR(rtc_base);

    info->rtc_hal.rtc_base = (unsigned long)rtc_base;
    info->rtc_pdev         = pdev;

    platform_set_drvdata(pdev, info);
    info->rtc_pdev = pdev;
    mutex_init(&info->mutex);

    sstar_rtc_init(pdev, info);

    hal_rtc_init(&info->rtc_hal);

    sstar_rtc_point  = info;
    info->rtc_inited = 1;

#ifdef CONFIG_PM_SLEEP
    device_set_wakeup_capable(&pdev->dev, true);
    device_wakeup_enable(&pdev->dev);
#endif

    info->rtc_dev = devm_rtc_device_register(&pdev->dev, dev_name(&pdev->dev), &sstar_rtcpwc_ops, THIS_MODULE);

    if (IS_ERR(info->rtc_dev))
    {
        ret = PTR_ERR(info->rtc_dev);
        RTC_ERR("[%s]: unable to register device (err=%d).\n", __func__, ret);
        return ret;
    }

    // Note: is it needed?
    // device_set_wakeup_capable(&pdev->dev, 1);
    // device_wakeup_enable(&pdev->dev);

    // init rtc
    RTC_DBG("[%s]: hardware initialize\n", __func__);

    if (0 != (ret = alloc_chrdev_region(&dev, 0, 1, "rtcpwc")))
        return ret;

    rtc_dev = device_create(msys_get_sysfs_class(), NULL, dev, NULL, "rtcpwc");

    if (info->rtc_hal.pwc_alm_enable)
    {
        device_create_file(rtc_dev, &dev_attr_alarm_timer);
        device_create_file(rtc_dev, &dev_attr_alarm_interrupt);
    }
    device_create_file(rtc_dev, &dev_attr_count_status);
#ifdef CONFIG_RTC_WITH_PWC
    device_create_file(rtc_dev, &dev_attr_wakeup_event);
    device_create_file(rtc_dev, &dev_attr_event_state);
#endif
    device_create_file(rtc_dev, &dev_attr_offset_count);

#ifdef CONFIG_RTC_WITH_PWC
    pm_power_off = sstar_rtcpwc_poweroff;
#endif

    return ret;
}

static const struct of_device_id sstar_rtcpwc_of_match_table[] = {{.compatible = "sstar,rtcpwc"}, {}};
MODULE_DEVICE_TABLE(of, sstar_rtcpwc_of_match_table);

static struct platform_driver sstar_rtcpwc_driver = {
    .remove = sstar_rtcpwc_remove,
    .probe  = sstar_rtcpwc_probe,
    .driver =
        {
            .name           = "sstar,rtcpwc",
            .owner          = THIS_MODULE,
            .of_match_table = sstar_rtcpwc_of_match_table,
        },
};

module_platform_driver(sstar_rtcpwc_driver);

MODULE_AUTHOR("SSTAR");
MODULE_DESCRIPTION("SStar RTCPWC Driver");
MODULE_LICENSE("GPL v2");
