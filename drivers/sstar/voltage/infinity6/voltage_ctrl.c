/*
* ms_complex_clk.c- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: karl.xiao <karl.xiao@sigmastar.com.tw>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/module.h>
#include "registers.h"
#include "ms_platform.h"
#include "voltage_ctrl.h"

struct device dev_core;

static struct bus_type voltage_subsys = {
    .name = "voltage",
    .dev_name = "voltage",
};

#if defined (CONFIG_MS_CPU_FREQ) && defined (CONFIG_MS_GPIO)
u8 enable_scaling_voltage = 0;
#include "gpio.h"
#include "../../gpio/mdrv_gpio.h"
#else
u8 enable_scaling_voltage = 0;
#define MDrv_GPIO_Set_High(x) {}
#define MDrv_GPIO_Set_Low(x) {}
#define MDrv_GPIO_Pad_Set(x) {}
#endif

#define VOLCTRL_DEBUG  0

#if VOLCTRL_DEBUG
#define VOLCTRL_DBG(fmt, arg...) printk(KERN_INFO fmt, ##arg)
#else
#define VOLCTRL_DBG(fmt, arg...)
#endif
#define VOLCTRL_ERR(fmt, arg...) printk(KERN_ERR fmt, ##arg)

unsigned int gpio_vid_width = 0;
unsigned int *gpio_vid_pins = NULL;
unsigned int *gpio_vid_voltages = NULL;

int _gCurrentVoltageCore = 0;

int _gVoltageDemanderRequestValue[VOLTAGE_DEMANDER_MAX] = {0};
const char *_gVoltageDemanderName[] = {
    FOREACH_DEMANDER(GENERATE_STRING)
};

int gVoltageCoreGpioInited = 0;
DEFINE_MUTEX(core_voltage_mutex);

static int check_voltage_valid(int mV)
{
    int voltage_lv_cnt = 0;
    int vcore_total_lv = 1 << gpio_vid_width;

    // Check core voltage setting(mV) valid or not.
    for (voltage_lv_cnt = 0; voltage_lv_cnt < vcore_total_lv; voltage_lv_cnt++)
    {
        if (mV == gpio_vid_voltages[voltage_lv_cnt])
        {
            break;
        }
    }

    if (voltage_lv_cnt == vcore_total_lv)  // If core voltage setting(mV) is invalid, Try to find ceiling setting.
    {
        for (voltage_lv_cnt = 0; voltage_lv_cnt < vcore_total_lv; voltage_lv_cnt++)
        {
            if (mV < gpio_vid_voltages[voltage_lv_cnt])
            {
                VOLCTRL_ERR("[Core Voltage] %s: Not support %dmV, use %dmV\n",
                            __FUNCTION__, mV, gpio_vid_voltages[voltage_lv_cnt]);
                mV = gpio_vid_voltages[voltage_lv_cnt];
                break;
            }
        }

        if (voltage_lv_cnt == vcore_total_lv)  // If no ceiling setting, use highest setting.
        {
            voltage_lv_cnt--;
            VOLCTRL_ERR("[Core Voltage] %s: Not support %dmV, use %dmV\n",
                        __FUNCTION__, mV, gpio_vid_voltages[voltage_lv_cnt]);
            mV = gpio_vid_voltages[voltage_lv_cnt];
        }
    }

    return mV;
}

static void sync_core_voltage(void)
{
    int voltage_lv_cnt = 0;
    int gpio_lv_cnt = 0;
    int i = 0;
    int vcore_max = 0;
    int vcore_total_lv = 1 << gpio_vid_width;

    mutex_lock(&core_voltage_mutex);

    if(gVoltageCoreGpioInited && enable_scaling_voltage)
    {
        for (i=0; i<VOLTAGE_DEMANDER_MAX; i++)
        {
            vcore_max = max(vcore_max, _gVoltageDemanderRequestValue[i]);
        }
        vcore_max = check_voltage_valid(vcore_max);
        VOLCTRL_DBG("[Core Voltage] %s: maximum request is %dmV\n", __FUNCTION__, vcore_max);

        if(gpio_vid_width > 0)
        {
            for (voltage_lv_cnt = 0; voltage_lv_cnt < vcore_total_lv; voltage_lv_cnt++)
            {
                if (vcore_max == gpio_vid_voltages[voltage_lv_cnt])
                {
                    for (gpio_lv_cnt=0; gpio_lv_cnt<gpio_vid_width; gpio_lv_cnt++)
                    {
                        if ((voltage_lv_cnt >> gpio_lv_cnt) & 0x1)
                        {
                            MDrv_GPIO_Set_High(gpio_vid_pins[gpio_lv_cnt]);
                            VOLCTRL_DBG("[Core Voltage] %s: gpio%d set high\n", __FUNCTION__, gpio_vid_pins[gpio_lv_cnt]);
                        }
                        else
                        {
                            MDrv_GPIO_Set_Low(gpio_vid_pins[gpio_lv_cnt]);
                            VOLCTRL_DBG("[Core Voltage] %s: gpio%d set low\n", __FUNCTION__, gpio_vid_pins[gpio_lv_cnt]);
                        }
                    }

                    _gCurrentVoltageCore = gpio_vid_voltages[voltage_lv_cnt];
                    break;
                }
            }
        }
    }
    else
    {
        VOLCTRL_DBG("[Core Voltage] %s: voltage scaling not enable\n", __FUNCTION__);
    }

    VOLCTRL_DBG("[Core Voltage] %s: current core voltage %dmV\n", __FUNCTION__, _gCurrentVoltageCore);

    mutex_unlock(&core_voltage_mutex);
}

void set_core_voltage(VOLTAGE_DEMANDER_E demander, int mV)
{
    int voltage_lv_cnt = 0;
    int gpio_lv_cnt = 0;
    int i = 0;
    int vcore_max = 0;
    int vcore_total_lv = 1 << gpio_vid_width;

    if (demander >= VOLTAGE_DEMANDER_MAX)
    {
        VOLCTRL_ERR("[Core Voltage] %s: demander number out of range (%d)\n", __FUNCTION__, demander);
        return;
    }

    mutex_lock(&core_voltage_mutex);

    _gVoltageDemanderRequestValue[demander] = mV;
    VOLCTRL_DBG("[Core Voltage] %s: %s request %dmV\n", __FUNCTION__, _gVoltageDemanderName[demander], mV);

    if(gVoltageCoreGpioInited && enable_scaling_voltage)
    {
        for (i=0; i<VOLTAGE_DEMANDER_MAX; i++)
        {
            vcore_max = max(vcore_max, _gVoltageDemanderRequestValue[i]);
        }
        vcore_max = check_voltage_valid(vcore_max);
        VOLCTRL_DBG("[Core Voltage] %s: maximum request is %dmV\n", __FUNCTION__, vcore_max);

        if(gpio_vid_width > 0)
        {
            for (voltage_lv_cnt = 0; voltage_lv_cnt < vcore_total_lv; voltage_lv_cnt++)
            {
                if (vcore_max == gpio_vid_voltages[voltage_lv_cnt])
                {
                    for (gpio_lv_cnt=0; gpio_lv_cnt<gpio_vid_width; gpio_lv_cnt++)
                    {
                        if ((voltage_lv_cnt >> gpio_lv_cnt) & 0x1)
                        {
                            MDrv_GPIO_Set_High(gpio_vid_pins[gpio_lv_cnt]);
                            VOLCTRL_DBG("[Core Voltage] %s: gpio%d set high\n", __FUNCTION__, gpio_vid_pins[gpio_lv_cnt]);
                        }
                        else
                        {
                            MDrv_GPIO_Set_Low(gpio_vid_pins[gpio_lv_cnt]);
                            VOLCTRL_DBG("[Core Voltage] %s: gpio%d set low\n", __FUNCTION__, gpio_vid_pins[gpio_lv_cnt]);
                        }
                    }

                    _gCurrentVoltageCore = gpio_vid_voltages[voltage_lv_cnt];
                    break;
                }
            }
        }
    }
    else
    {
        VOLCTRL_DBG("[Core Voltage] %s: voltage scaling not enable\n", __FUNCTION__);
    }

    VOLCTRL_DBG("[Core Voltage] %s: current core voltage %dmV\n", __FUNCTION__, _gCurrentVoltageCore);

    mutex_unlock(&core_voltage_mutex);
}
EXPORT_SYMBOL(set_core_voltage);

int get_core_voltage(void)
{
    return _gCurrentVoltageCore;
}
EXPORT_SYMBOL(get_core_voltage);

static void core_voltage_get_gpio(void)
{
    struct device_node *np = NULL;

    mutex_lock(&core_voltage_mutex);

    if((np=of_find_node_by_name(NULL, "core_voltage")))
    {
        if (0 != of_property_read_u32(np, "vid_width", &gpio_vid_width) || gpio_vid_width < 1)
            goto core_voltage_get_gpio_err;

        gpio_vid_pins = vzalloc(sizeof(unsigned int) * gpio_vid_width);
        if (!gpio_vid_pins)
            goto core_voltage_get_gpio_err;

        gpio_vid_voltages = vzalloc(sizeof(unsigned int) * (1 << gpio_vid_width));
        if (!gpio_vid_voltages)
            goto core_voltage_get_gpio_err;

        if (gpio_vid_width != of_property_read_variable_u32_array(np, "vid_gpios", gpio_vid_pins, 0, gpio_vid_width))
            goto core_voltage_get_gpio_err;
        if ((1 << gpio_vid_width) != of_property_read_variable_u32_array(np, "vid_voltages", gpio_vid_voltages, 0, (1 << gpio_vid_width)))
            goto core_voltage_get_gpio_err;
    }
    else
    {
        VOLCTRL_ERR("[Core Voltage] %s: can't get core_voltage node\n", __FUNCTION__);
    }

    gVoltageCoreGpioInited = 1;

    mutex_unlock(&core_voltage_mutex);
    return;

core_voltage_get_gpio_err:
    gpio_vid_width = 0;
    if (gpio_vid_pins)
    {
        vfree(gpio_vid_pins);
        gpio_vid_pins = NULL;
    }
    if (gpio_vid_voltages)
    {
        vfree(gpio_vid_voltages);
        gpio_vid_voltages = NULL;
    }

    mutex_unlock(&core_voltage_mutex);
    return;
}

int core_voltage_available(unsigned int **voltages, unsigned int *num)
{
    if (voltages && num)
    {
        *num = 1 << gpio_vid_width;
        *voltages = gpio_vid_voltages;
        return 0;
    }
    else
    {
        return -1;
    }
}
EXPORT_SYMBOL(core_voltage_available);

int core_voltage_pin(unsigned int **pins, unsigned int *num)
{
    if (pins && num)
    {
        *num = gpio_vid_width;
        *pins = gpio_vid_pins;
        return 0;
    }
    else
    {
        return -1;
    }
}
EXPORT_SYMBOL(core_voltage_pin);

static ssize_t show_scaling_voltage(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    mutex_lock(&core_voltage_mutex);
    str += scnprintf(str, end - str, "%d\n", enable_scaling_voltage);
    mutex_unlock(&core_voltage_mutex);

    return (str - buf);
}

static ssize_t store_scaling_voltage(struct device *dev,  struct device_attribute *attr, const char *buf, size_t count)
{
    u32 enable;
    if (sscanf(buf, "%d", &enable)<=0)
        return 0;

    mutex_lock(&core_voltage_mutex);
    if(enable)
    {
        enable_scaling_voltage=1;
        VOLCTRL_DBG("[Core Voltage] %s: scaling ON\n", __FUNCTION__);
    }
    else
    {
        enable_scaling_voltage=0;
        VOLCTRL_DBG("[Core Voltage] %s: scaling OFF\n", __FUNCTION__);
    }
    mutex_unlock(&core_voltage_mutex);
    sync_core_voltage();

    return count;
}
DEVICE_ATTR(scaling_voltage, 0644, show_scaling_voltage, store_scaling_voltage);

static ssize_t show_voltage_available(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    unsigned int i = 0;
    unsigned int *voltages = NULL;
    unsigned int voltage_num = 0;

    mutex_lock(&core_voltage_mutex);
    if(core_voltage_available(&voltages, &voltage_num) == 0)
    {
        for (i = 0; i < voltage_num; i++)
        {
            str += scnprintf(str, end - str, "%d ", voltages[i]);
        }
    }
    mutex_unlock(&core_voltage_mutex);

    str += scnprintf(str, end - str, "\n");

    return (str - buf);
}
DEVICE_ATTR(voltage_available, 0444, show_voltage_available, NULL);

static ssize_t show_voltage_current(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    unsigned int i;

    str += scnprintf(str, end - str, "%d\n", get_core_voltage());

    for (i=0; i<VOLTAGE_DEMANDER_MAX; i++)
    {
        if (_gVoltageDemanderRequestValue[i])
            str += scnprintf(str, end - str, "    %-32s%d\n", _gVoltageDemanderName[i], _gVoltageDemanderRequestValue[i]);
        else
            str += scnprintf(str, end - str, "    %-32s-\n", _gVoltageDemanderName[i]);
    }

    return (str - buf);
}

static ssize_t store_voltage_current(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    u32 voltage = 0;
    if (sscanf(buf, "%d", &voltage)<=0)
        return 0;

    if (enable_scaling_voltage)
        set_core_voltage(VOLTAGE_DEMANDER_USER, voltage);
    else
        printk("[Core Voltage] voltage scaling not enable\n");

    return count;
}
DEVICE_ATTR(voltage_current, 0644, show_voltage_current, store_voltage_current);

static ssize_t show_vid_gpio_map(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    unsigned int i = 0;
    unsigned int *pins = NULL;
    unsigned int pin_num = 0;

    if(core_voltage_pin(&pins, &pin_num) == 0)
    {
        for (i = 0; i < pin_num; i++)
        {
            str += scnprintf(str, end - str, "vid_%d=%d ", i, pins[i]);
        }
    }

    str += scnprintf(str, end - str, "\n");

    return (str - buf);
}
DEVICE_ATTR(vid_gpio_map, 0444, show_vid_gpio_map, NULL);

int voltage_control_init(void)
{
    int ret = 0;
    unsigned int miupll_freq_Mhz = 0;
    unsigned int miu_data_rate = 0;

    dev_core.kobj.name = "core";
    dev_core.bus = &voltage_subsys;

    core_voltage_get_gpio();

    VOLCTRL_DBG("[Core Voltage] %s: register sub system\n", __FUNCTION__);

    subsys_system_register(&voltage_subsys, NULL);
    if (ret) {
        printk(KERN_ERR "Failed to register voltage sub system!! %d\n",ret);
        return -1;
    }

    ret = device_register(&dev_core);
    if (ret) {
        printk(KERN_ERR "Failed to register voltage core device!! %d\n",ret);
        return -1;
    }

    device_create_file(&dev_core, &dev_attr_scaling_voltage);
    device_create_file(&dev_core, &dev_attr_voltage_available);
    device_create_file(&dev_core, &dev_attr_voltage_current);
    device_create_file(&dev_core, &dev_attr_vid_gpio_map);


    // Check MIUPLL and determine core voltage request
    miupll_freq_Mhz = 24 * INREGMSK16(BASE_REG_MIUPLL_PA + REG_ID_03, 0x00FF) / ((INREGMSK16(BASE_REG_MIUPLL_PA + REG_ID_03, 0x0700) >> 8) + 2);
    miu_data_rate = 1 << (INREGMSK16(BASE_REG_MIU_PA + REG_ID_01, 0x0300) >> 8);
    if(miupll_freq_Mhz > 433 || (miu_data_rate <=4 && miupll_freq_Mhz > 216))
        set_core_voltage(VOLTAGE_DEMANDER_MIU, VOLTAGE_CORE_1000);
    else
        set_core_voltage(VOLTAGE_DEMANDER_MIU, VOLTAGE_CORE_900);

    // Enable core voltage scaling
    VOLCTRL_DBG("[Core Voltage] %s: turn-on core voltage scaling\n", __FUNCTION__);
    enable_scaling_voltage = 1;
    sync_core_voltage();

    return 0;
}

device_initcall(voltage_control_init);
