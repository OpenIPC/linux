/*
 * drv_adc-keys.c- Sigmastar
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

#include <linux/err.h>
//#include <linux/iio/consumer.h>
//#include <linux/iio/types.h>
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/slab.h>

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/of_platform.h>
#include <linux/delay.h>
#include "ms_types.h"
#include <linux/reboot.h>
// drv_rtcpwc is in use.
#include <hal_rtcpwc.h>
#include <hal_rtcpwcreg.h>
#include <cam_os_wrapper.h>

#define LONG_PRESS_HOLD_TIMES  (15)
#define SHORT_PRESS_HOLD_TIMES (2)

struct adc_keys_button
{
    u32 voltage;
    u32 keycode;
};

struct sstar_rtc_info
{
    u8                      rtc_inited;
    struct platform_device *rtc_pdev;
    struct rtc_device *     rtc_dev;
    struct hal_rtcpwc_t     rtc_hal;
    spinlock_t              mutex;
    u32                     auto_wakeup_delay_seconds;
};
void __iomem *g_prtc_base = NULL;

static int _virtual_key = 0;
static int _vkey_status = 1;

struct adc_keys_state
{
    // struct iio_channel *channel;
    int                           chan;
    u32                           num_keys;
    u32                           last_key;
    u32                           hold_times;
    u32                           keyup_voltage;
    void __iomem *                reg_base;
    const struct adc_keys_button *map;
};

#define ISO_S0                         0x00
#define ISO_S1                         0x01
#define ISO_S2                         0x03
#define ISO_S3                         0x07
#define ISO_S4                         0x05
#define ISO_S5                         0x01
#define ISO_ACK_RETRY_TIME             20
#define RTC_CHECK_STATUS_DELAY_TIME_US 100

extern int  ms_sar_get(int ch);
extern void ms_sar_hw_init(void);

static int hal_rtc_has_1k_clk(void)
{
    u16 reg     = 0;
    u8  ubCheck = 33; /// delay 22 * 100 = 2200 us

    do
    {
        reg = readw(g_prtc_base + (RTCPWC_DIG2PWC_RTC_TESTBUS << 2));
        if (reg & RTCPWC_CLK_1K)
        {
            return 1;
        }
        CamOsUsDelay(100);
        ubCheck--;
    } while (ubCheck);
    return 0;
}

bool hal_rtc_iso_ctrl_ex(void)
{
    u16 reg     = 0;
    u8  ubCheck = ISO_ACK_RETRY_TIME;

    writew(RTCPWC_ISO_TRIG_BIT, g_prtc_base + (RTCPWC_ISO_TRIG << 2));
    reg = readw(g_prtc_base + (RTCPWC_ISO_EN_ACK << 2));
    reg = reg & RTCPWC_ISO_EN_ACK_BIT;

    while ((!reg) && (--ubCheck))
    {
        CamOsUsDelay(RTC_CHECK_STATUS_DELAY_TIME_US);
        reg = readw(g_prtc_base + (RTCPWC_ISO_EN_ACK << 2));
        reg = reg & RTCPWC_ISO_EN_ACK_BIT;
    }
    if (ubCheck == 0)
        return FALSE;

    writew(RTCPWC_ISO_EN_CLR_BIT, g_prtc_base + (RTCPWC_ISO_EN_CLR << 2));

    reg     = readw(g_prtc_base + (RTCPWC_ISO_EN_ACK << 2));
    reg     = reg & RTCPWC_ISO_EN_ACK_BIT;
    ubCheck = ISO_ACK_RETRY_TIME;
    while ((reg) && (--ubCheck))
    {
        CamOsUsDelay(RTC_CHECK_STATUS_DELAY_TIME_US);
        reg = readw(g_prtc_base + (RTCPWC_ISO_EN_ACK << 2));
        reg = reg & RTCPWC_ISO_EN_ACK_BIT;
    }
    if (ubCheck == 0)
        return FALSE;

    return TRUE;
}

static void ms_RtcPwr_ISOCTL_EX(void)
{
    static int warn_once = 0;
    u8         ubCheck   = ISO_ACK_RETRY_TIME;

    if (0 == hal_rtc_has_1k_clk())
    {
        if (!warn_once)
        {
            warn_once = 1;
            printk("[%s][%d] RTCPWC fail to enter correct state and possibly caused by no power supplied\n",
                   __FUNCTION__, __LINE__);
        }
        return;
    }

    while (!hal_rtc_iso_ctrl_ex() && (--ubCheck))
    {
        CamOsUsDelay(100); // RTC_CHECK_STATUS_DELAY_TIME_US
    }
}

#if defined(SYS_POWEROFF_DIRECT_BY_DRIVER)
static void ms_poweroff_Notify(void)
{
    int   ret    = -1;
    char  path[] = "/usr/bin/killall";
    char *argv[] = {path, "cardv", NULL};
    char *envp[] = {"HOME=/", "PATH=/sbin:/bin:/user/bin", NULL};

    printk(KERN_CRIT "-------%s-------\n", __func__);
    ret = call_usermodehelper(path, argv, envp, UMH_WAIT_PROC);
    ssleep(3);
    kernel_power_off();
}
#endif

static void ms_poweroff_enter(void)
{
    if (g_prtc_base)
    {
        // Enable WOS,connect to Gsen_POC
        writew(0x03, (g_prtc_base + (0x21 << 2))); // set reg_vh_sel_poc =3'b011 ' VIH: AVDD x 75% (default)
        writew(0x02, (g_prtc_base + (0x22 << 2))); // set reg_wos_clr =1'b0 ' HIZ select, wait sound input
        // software control of PWC Power Enable.
        writew(0x00, (g_prtc_base + (0x0f << 2)));
        printk(KERN_CRIT "[%s]\n", __func__);
        ms_RtcPwr_ISOCTL_EX();
    }
}

static ssize_t ms_panic_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int ret;
    ret = sprintf(buf, "Use to save Segmentation fault to mtd\n");
    return ret;
}

static ssize_t ms_panic_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int value, ret;
    ret = kstrtoint(buf, 10, &value);
    panic("***********test************");
    return count;
}

//#define KEY_UP		103
//#define KEY_LEFT		105
//#define KEY_RIGHT		106
//#define KEY_DOWN		108
//#define KEY_ENTER		28
//#define KEY_MENU		139	/* Menu (show menu) */
// CMD Prompt:"echo 139 0 > /sys/devices/soc0/soc/soc:adc-keys/virtual_input"
static ssize_t ms_virtual_key_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    char *opt;

    // printk("%s\n", buf);
    _virtual_key = simple_strtoul(buf, &opt, 10);
    if (*opt == ' ')
        _vkey_status = simple_strtoul(opt + 1, NULL, 10);

    return count;
}

static ssize_t ms_virtual_key_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int ret;
    ret = sprintf(buf,
                  "Prompt:echo 139 0 > /sys/devices/soc0/soc/soc:adc-keys/virtual_input\n 0->rel\n 1->press\n "
                  "2->Lpress\n 255->pre&rel\n");
    return ret;
}

// Segmentation fault
static DEVICE_ATTR(save_seg_fault, 0660, ms_panic_show, ms_panic_store);
static DEVICE_ATTR(virtual_input, 0660, ms_virtual_key_show, ms_virtual_key_store);
static const struct attribute *     uadc_attrs[] = {&dev_attr_save_seg_fault.attr, &dev_attr_virtual_input.attr, NULL};
static const struct attribute_group adc_attr_grp = {
    .attrs = (struct attribute **)uadc_attrs,
};

static void adc_keys_poll(struct input_polled_dev *dev)
{
    struct adc_keys_state *st = dev->private;
    int                    i, value;
    u32                    diff, closest = 0xffffffff;
    int                    keycode = 0;

    // ret = iio_read_channel_processed(st->channel, &value);
    value = ms_sar_get(st->chan);
    value = (value * 325) / 100;
    if (unlikely(value < 0))
    {
        /* Forcibly release key if any was pressed */
        value = st->keyup_voltage;
    }
    else
    {
        for (i = 0; i < st->num_keys; i++)
        {
            diff = abs(st->map[i].voltage - value);
            if (diff < closest)
            {
                closest = diff;
                keycode = st->map[i].keycode;
            }
        }
    }

    if (abs(st->keyup_voltage - value) < closest)
        keycode = 0;

    if (keycode == 0 && st->reg_base)
    {
        u16 reg = 0;
        reg     = readw(st->reg_base + 0x48);
        keycode = (reg & 0x04) ? KEY_POWER : 0;
        if (keycode == KEY_POWER)
        {
// printk(KERN_CRIT "keycode = %d  value=%d  \n", keycode,reg);
#if defined(SYS_POWEROFF_DIRECT_BY_DRIVER)
            ms_poweroff_Notify();
#endif
        }
    }

    if (st->last_key && st->last_key != keycode)
    {
        // printk(" free keycode = %d new = %d value=%d\n", st->last_key, keycode, value);
        // input_report_key(dev->input, st->last_key, 0);
        input_event(dev->input, EV_KEY, st->last_key, 0); // release
        st->hold_times = 0;
    }

    if (st->last_key && st->last_key == keycode)
    {
        st->hold_times++;
        if (st->hold_times == LONG_PRESS_HOLD_TIMES)
        {
            // printk(" long press keycode = %d  value=%d\n", keycode, value);
            input_event(dev->input, EV_KEY, keycode, 2); // long press
        }
        else if (st->hold_times < SHORT_PRESS_HOLD_TIMES)
        {
            input_event(dev->input, EV_KEY, keycode, 1);
        }
    }
    else if (keycode)
    {
        // printk(" press keycode = %d  value=%d\n", keycode, value);
        // input_event(dev->input, EV_KEY, keycode, 1); // press
        st->hold_times = 0;
    }

    if (_virtual_key)
    {
        if (_vkey_status == 255)
        {
            input_event(dev->input, EV_KEY, _virtual_key, 1);
            input_event(dev->input, EV_KEY, _virtual_key, 0);
        }
        else if (_vkey_status >= 0 && _vkey_status <= 3)
        {
            input_event(dev->input, EV_KEY, _virtual_key, _vkey_status);
        }
        _virtual_key = 0;
        _vkey_status = 1;
    }

    input_sync(dev->input);
    st->last_key = keycode;
}

static int adc_keys_load_keymap(struct device *dev, struct adc_keys_state *st)
{
    struct adc_keys_button *map;
    struct fwnode_handle *  child;
    int                     i;

    st->num_keys = device_get_child_node_count(dev);
    if (st->num_keys == 0)
    {
        dev_err(dev, "keymap is missing\n");
        return -EINVAL;
    }

    map = devm_kmalloc_array(dev, st->num_keys, sizeof(*map), GFP_KERNEL);
    if (!map)
        return -ENOMEM;

    i = 0;
    device_for_each_child_node(dev, child)
    {
        if (fwnode_property_read_u32(child, "press-threshold-microvolt", &map[i].voltage))
        {
            dev_err(dev, "Key with invalid or missing voltage\n");
            fwnode_handle_put(child);
            return -EINVAL;
        }
        map[i].voltage /= 1000;

        if (fwnode_property_read_u32(child, "linux,code", &map[i].keycode))
        {
            dev_err(dev, "Key with invalid or missing linux,code\n");
            fwnode_handle_put(child);
            return -EINVAL;
        }

        i++;
    }

    st->map = map;
    return 0;
}

static int adc_keys_probe(struct platform_device *pdev)
{
    struct device *          dev = &pdev->dev;
    struct adc_keys_state *  st;
    struct input_polled_dev *poll_dev;
    struct input_dev *       input;
    int                      i, value, chan;
    int                      error;
    char                     compatible_name[64];
    struct device_node *     dev_node;
    struct platform_device * pRtcdev;
    struct sstar_rtc_info *  _pinfo = NULL;

    st = devm_kzalloc(dev, sizeof(*st), GFP_KERNEL);
    if (!st)
    {
        return -ENOMEM;
    }

    sprintf(compatible_name, "sstar,infinity-rtcpwc");
    dev_node = of_find_compatible_node(NULL, NULL, compatible_name);

    if (!dev_node)
    {
        dev_err(dev, "[%s]: failed to find of_find_compatible_node\n", __func__);
        // return 0;
    }
    else
    {
        pRtcdev = of_find_device_by_node(dev_node);
        _pinfo  = platform_get_drvdata(pRtcdev);

        if (!_pinfo)
        {
            dev_err(dev, "[%s]: failed to get platform_get_drvdata\n", __func__);
            st->reg_base = g_prtc_base = NULL;
            // return 0;
        }
        else
        {
            st->reg_base = (void __iomem *)_pinfo->rtc_hal.rtc_base;
            g_prtc_base  = (void __iomem *)_pinfo->rtc_hal.rtc_base;
            dev_err(dev, "adc_keys_probe reg_base=0x%p success\n", st->reg_base);
        }
    }

    if (!device_property_read_u32(dev, "io-channels", &chan))
        st->chan = chan;
    else
    {
        // set default
        st->chan = 0;
    }

    if (device_property_read_u32(dev, "keyup-threshold-microvolt", &st->keyup_voltage))
    {
        dev_err(dev, "Invalid or missing keyup voltage\n");
        return -EINVAL;
    }
    st->keyup_voltage /= 1000;

    error = adc_keys_load_keymap(dev, st);
    if (error)
        return error;

    platform_set_drvdata(pdev, st);

    poll_dev = devm_input_allocate_polled_device(dev);
    if (!poll_dev)
    {
        dev_err(dev, "failed to allocate input device\n");
        return -ENOMEM;
    }

    if (!device_property_read_u32(dev, "poll-interval", &value))
        poll_dev->poll_interval = value;

    poll_dev->poll    = adc_keys_poll;
    poll_dev->private = st;

    input = poll_dev->input;

    input->name = pdev->name;
    input->phys = "adc-keys/input0";

    input->id.bustype = BUS_HOST;
    input->id.vendor  = 0x0001;
    input->id.product = 0x0001;
    input->id.version = 0x0100;

    __set_bit(EV_KEY, input->evbit);
    for (i = 0; i < st->num_keys; i++)
        __set_bit(st->map[i].keycode, input->keybit);

    if (device_property_read_bool(dev, "autorepeat"))
        __set_bit(EV_REP, input->evbit);

    error = input_register_polled_device(poll_dev);
    if (error)
    {
        dev_err(dev, "Unable to register input device: %d\n", error);
        return error;
    }
    ms_sar_hw_init();

    if (0 != sysfs_create_group(&dev->kobj, &adc_attr_grp))
    {
        pr_warn("adc sysfs_create_group failed...\n");
    }

    pm_power_off = ms_poweroff_enter;
    return 0;
}

static int adc_keys_remove(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    sysfs_remove_group(&dev->kobj, &adc_attr_grp);
    printk("TODO:%s", __func__);
    return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id adc_keys_of_match[] = {{
                                                            .compatible = "adc-keys",
                                                        },
                                                        {}};
MODULE_DEVICE_TABLE(of, adc_keys_of_match);
#endif

static struct platform_driver __refdata adc_keys_driver = {
    .driver =
        {
            .name           = "adc_keys",
            .of_match_table = of_match_ptr(adc_keys_of_match),
        },
    .probe  = adc_keys_probe,
    .remove = adc_keys_remove,
};
module_platform_driver(adc_keys_driver);

MODULE_AUTHOR("Alexandre Belloni <alexandre.belloni@free-electrons.com>");
MODULE_DESCRIPTION("Input driver for resistor ladder connected on ADC");
MODULE_LICENSE("GPL v2");
