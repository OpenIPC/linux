/*
 * cpufreq.c- Sigmastar
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/pm_opp.h>
#include <linux/kthread.h>

#include "ms_types.h"
#include "ms_platform.h"
#include "registers.h"
#include "voltage_ctrl.h"
#include "../../opp/opp.h"

//#define __DEBUG_OSC__
//#define __DEBUG_TEMP__
//#define __DEBUG_IPU__
#ifdef __DEBUG_IPU__
#include <linux/of_platform.h>
#endif

// u32                                    sidd_th_100x = 1243; // sidd threshold=12.74mA
static struct device *                 cpu;
static struct cpufreq_frequency_table *freq_table;
int                                    g_sCurrentTemp         = 35;
static int                             g_sCurrentTempThreshLo = 40;
static int                             g_sCurrentTempThreshHi = 60;
#ifdef __DEBUG_TEMP__
static int g_sManualTemp = 0;
#endif
#ifdef __DEBUG_IPU__
static struct
{
    unsigned long  curfreq;
    struct device *dev;
} m_ipufreq = {
    0,
};
#endif

struct timer_list timer_temp;
const char *      power_name = "core_power";

typedef enum
{
    E_OSC_0, // ss
    E_OSC_1, // ff
} OSC_TYPE_E;

typedef struct
{
    unsigned int g_SIDD;
    unsigned int g_OSC;
    unsigned int g_OSCThreshold;
    unsigned int g_SIDDThreshold;
    unsigned int g_OSCTYPE;
} SS_VID_INFO;
SS_VID_INFO vidInfo = {0, 0, 240, 275, 0};

unsigned long dev_pm_opp_get_voltageMin(struct dev_pm_opp *opp)
{
    if (IS_ERR_OR_NULL(opp))
    {
        pr_err("%s: Invalid parameters\n", __func__);
        return 0;
    }

    return opp->supplies[0].u_volt_min;
}

static int ms_cpufreq_target_index(struct cpufreq_policy *policy, unsigned int index)
{
    struct cpufreq_freqs freqs;
    int                  ret;
    struct dev_pm_opp *  opp;
    unsigned long        new_freq;
    int                  opp_voltage_mV;
#if defined(CONFIG_SS_VOLTAGE_CTRL) || defined(CONFIG_SS_VOLTAGE_IDAC_CTRL)
    int voltage_mV;
#endif

    freqs.old = policy->cur;
    freqs.new = freq_table[index].frequency;
    new_freq  = freqs.new * 1000;

    opp = dev_pm_opp_find_freq_ceil(cpu, &new_freq);
    if (IS_ERR(opp))
    {
        pr_err("[%s] %d not found in OPP\n", __func__, freqs.new);
        return -EINVAL;
    }

    if (vidInfo.g_OSCTYPE) // ff
        opp_voltage_mV = (dev_pm_opp_get_voltageMin(opp) ? dev_pm_opp_get_voltageMin(opp) / 1000 : 0);
    else
        opp_voltage_mV = (dev_pm_opp_get_voltage(opp) ? dev_pm_opp_get_voltage(opp) / 1000 : 0);

#if defined(CONFIG_SS_VOLTAGE_CTRL) || defined(CONFIG_SS_VOLTAGE_IDAC_CTRL)
    if (!get_core_voltage(power_name, &voltage_mV) && opp_voltage_mV > voltage_mV)
    {
        set_core_voltage(power_name, VOLTAGE_DEMANDER_CPUFREQ, opp_voltage_mV);
        udelay(10); // delay 10us to wait voltage stable (from low to high).
        ret = clk_set_rate(policy->clk, new_freq);
    }
    else
    {
        ret = clk_set_rate(policy->clk, new_freq);
        set_core_voltage(power_name, VOLTAGE_DEMANDER_CPUFREQ, opp_voltage_mV);
    }
#else
    ret = clk_set_rate(policy->clk, new_freq);
#endif

    return ret;
}

static ssize_t show_cpufreq_testout(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    char *str       = buf;
    char *end       = buf + PAGE_SIZE;
    u16   reg_value = 0;
    u32   freq      = 0;

    if ((INREG8(BASE_REG_RIU_PA + (0x102216 << 1)) & BIT0) != BIT0)
    {
        OUTREG8(BASE_REG_RIU_PA + (0x102216 << 1), 0x01);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEE << 1), 0x04);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEA << 1), 0x04);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEA << 1) + 1, 0x40);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEC << 1), 0x01);
        OUTREG8(BASE_REG_RIU_PA + (0x101EE0 << 1) + 1, 0x00);
        OUTREG8(BASE_REG_RIU_PA + (0x101EE0 << 1) + 1, 0x80);
        udelay(100);
    }
    reg_value = (INREG8(BASE_REG_RIU_PA + (0x101EE2 << 1)) | (INREG8(BASE_REG_RIU_PA + (0x101EE2 << 1) + 1) << 8));
    // freq = (reg_value * 4000)/83333;
    freq = reg_value * 48000;

    str += scnprintf(str, end - str, "%d\n", freq);

    return (str - buf);
}

static ssize_t store_cpufreq_testout(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    u32 enable;
    if (sscanf(buf, "%d", &enable) <= 0)
        return 0;

    if (enable)
    {
        pr_info("[CPUFREQ] Freq testout ON\n");
        OUTREG8(BASE_REG_RIU_PA + (0x102216 << 1), 0x01);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEE << 1), 0x04);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEA << 1), 0x04);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEA << 1) + 1, 0x40);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEC << 1), 0x01);
        OUTREG8(BASE_REG_RIU_PA + (0x101EE0 << 1) + 1, 0x00);
        OUTREG8(BASE_REG_RIU_PA + (0x101EE0 << 1) + 1, 0x80);
    }
    else
    {
        pr_info("[CPUFREQ] Freq testout OFF\n");
        OUTREG8(BASE_REG_RIU_PA + (0x102216 << 1), 0x00);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEE << 1), 0x00);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEA << 1), 0x00);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEA << 1) + 1, 0x00);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEC << 1), 0x00);
        OUTREG8(BASE_REG_RIU_PA + (0x101EE0 << 1) + 1, 0x00);
    }

    return count;
}
define_one_global_rw(cpufreq_testout);

static ssize_t show_temp_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int   vbe_code;

    vbe_code = INREG16(BASE_REG_PMSAR_PA + REG_ID_46);

#ifdef __DEBUG_TEMP__
    str += scnprintf(str, end - str, "Temp=%d  VBE=%d\n", g_sManualTemp ? g_sManualTemp : g_sCurrentTemp, vbe_code);
#else
    str += scnprintf(str, end - str, "Temp=%d  VBE=%d\n", g_sCurrentTemp, vbe_code);
#endif
    return (str - buf);
}
#ifdef __DEBUG_TEMP__
static ssize_t store_temp_out(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    if (sscanf(buf, "%d", &g_sManualTemp) <= 0)
        return 0;
    return count;
}
define_one_global_rw(temp_out);
#else
define_one_global_ro(temp_out);
#endif

static ssize_t show_sidd_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "%d\n", vidInfo.g_SIDD);

    return (str - buf);
}
define_one_global_ro(sidd_out);

static ssize_t show_osc_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "%d, %s\n", vidInfo.g_OSC, vidInfo.g_OSCTYPE ? "FF" : "SS");

    return (str - buf);
}
#ifdef __DEBUG_OSC__
/* since we can change corner type in run-time during debugging,
 * the Vmin according to CPU freq will be different between FF && SS.
 * here force to update CPUFREQ demander voltage while changing corner */
static void osc_sync_voltage(void)
{
    struct cpufreq_policy *policy;
    unsigned long          cur_freq;
    struct dev_pm_opp *    opp;
    int                    voltage_mV;

    policy = cpufreq_cpu_get(0);
    if (!policy)
        return;

    cur_freq = (unsigned long)policy->cur * 1000;
    opp      = dev_pm_opp_find_freq_ceil(cpu, &cur_freq);
    if (IS_ERR(opp))
        return;

    if (vidInfo.g_OSCTYPE) // ff
        voltage_mV = (dev_pm_opp_get_voltageMin(opp) ? dev_pm_opp_get_voltageMin(opp) / 1000 : 0);
    else
        voltage_mV = (dev_pm_opp_get_voltage(opp) ? dev_pm_opp_get_voltage(opp) / 1000 : 0);

    set_core_voltage(power_name, VOLTAGE_DEMANDER_CPUFREQ, voltage_mV);
    if (vidInfo.g_OSCTYPE == E_OSC_0) // ignore TEMPERATURE demander for SS
        set_core_voltage(power_name, VOLTAGE_DEMANDER_TEMPERATURE, 0);
}

static ssize_t store_osc_out(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    u32 value;
    if (sscanf(buf, "%d", &value) <= 0)
        return 0;

    if (vidInfo.g_OSCTYPE != value)
    {
        vidInfo.g_OSCTYPE = value;
        osc_sync_voltage();
    }
    return count;
}
define_one_global_rw(osc_out);
#else
define_one_global_ro(osc_out);
#endif

#ifdef __DEBUG_IPU__
static ssize_t show_ipu_freq(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "%d\n", (u32)m_ipufreq.curfreq);

    return (str - buf);
}

static ssize_t store_ipu_freq(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    u32                value;
    struct dev_pm_opp *opp;
    unsigned long      m_volt;

    if (sscanf(buf, "%d", &value) <= 0)
        return 0;
    m_ipufreq.curfreq = value * 1000;

    opp = dev_pm_opp_find_freq_ceil(m_ipufreq.dev, &m_ipufreq.curfreq);
    if (IS_ERR(opp))
    {
        pr_err("ipu opp ceil not found - %ld!\n", m_ipufreq.curfreq);
        return -EINVAL;
    }

    m_volt = dev_pm_opp_get_voltage(opp);
    m_volt = m_volt ? m_volt / 1000 : 0;
    printk("ipu freq %ld <-> volt %ld\n", m_ipufreq.curfreq, m_volt);
    set_core_voltage(power_name, VOLTAGE_DEMANDER_IPUFREQ, m_volt);

    return count;
}
define_one_global_rw(ipu_freq);
#endif

static ssize_t show_osc_threshold(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "%d\n", vidInfo.g_OSCThreshold);

    return (str - buf);
}

static ssize_t store_osc_threshold(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    u32 value;
    if (sscanf(buf, "%d", &value) <= 0)
        return 0;
    vidInfo.g_OSCThreshold = value;
    return count;
}

define_one_global_rw(osc_threshold);

static ssize_t show_temp_adjust_threshold_lo(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "%d\n", g_sCurrentTempThreshLo);

    return (str - buf);
}
static ssize_t store_temp_adjust_threshold_lo(struct kobject *kobj, struct kobj_attribute *attr, const char *buf,
                                              size_t count)
{
    u32 value;
    if (sscanf(buf, "%d", &value) <= 0)
        return 0;
    g_sCurrentTempThreshLo = value;
    return count;
}
define_one_global_rw(temp_adjust_threshold_lo);

static ssize_t show_temp_adjust_threshold_hi(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "%d\n", g_sCurrentTempThreshHi);

    return (str - buf);
}
static ssize_t store_temp_adjust_threshold_hi(struct kobject *kobj, struct kobj_attribute *attr, const char *buf,
                                              size_t count)
{
    u32 value;
    if (sscanf(buf, "%d", &value) <= 0)
        return 0;
    g_sCurrentTempThreshHi = value;
    return count;
}
define_one_global_rw(temp_adjust_threshold_hi);

int ms_get_temp(void)
{
    int vbe_code_ft;
    int vbe_code;

    CLRREG16(BASE_REG_PMSAR_PA + REG_ID_19, BIT6); // ch7 reference voltage select to 2.0V
    CLRREG16(BASE_REG_PMSAR_PA + REG_ID_10, BIT0); // reg_pm_dmy
    SETREG16(BASE_REG_PMSLEEP_PA + REG_ID_64, BIT10);
    SETREG16(BASE_REG_PMSLEEP_PA + REG_ID_2F, BIT2);
    OUTREG16(BASE_REG_PMSAR_PA + REG_ID_00, 0xA20);
    SETREG16(BASE_REG_PMSAR_PA + REG_ID_00, BIT14);
    vbe_code    = INREG16(BASE_REG_PMSAR_PA + REG_ID_46);
    vbe_code_ft = ((INREG16(BASE_REG_OTP3_PA + REG_ID_40) & 0x7FE0) >> 5);
    if (vbe_code_ft == 0) // if no trim info
        vbe_code_ft = 399;

    // calculate temperature
    return (1350 * (vbe_code_ft - vbe_code)) / 1000 + 27;
    // return 35;
}
EXPORT_SYMBOL(ms_get_temp);

static int monitor_temp_thread_handler(void *data)
{
#if defined(CONFIG_SS_VOLTAGE_CTRL) || defined(CONFIG_SS_VOLTAGE_IDAC_CTRL)
    int voltage_mV;
    int vUp = 0;
#endif

    while (!kthread_should_stop())
    {
        msleep_interruptible(1000);

#ifdef __DEBUG_OSC__
        if (vidInfo.g_OSCTYPE == E_OSC_0)
            continue;
#endif

        g_sCurrentTemp = ms_get_temp();
#ifdef __DEBUG_TEMP__
        if (g_sManualTemp)
            g_sCurrentTemp = g_sManualTemp;
#endif

#if defined(CONFIG_SS_VOLTAGE_CTRL) || defined(CONFIG_SS_VOLTAGE_IDAC_CTRL)
        if (!get_core_voltage(power_name, &voltage_mV) && (g_sCurrentTemp > g_sCurrentTempThreshHi))
        {
            // under high temperature, TEMPERATURE demander doesn't impact the final Vcore then.
            set_core_voltage(power_name, VOLTAGE_DEMANDER_TEMPERATURE, 0);
        }
        else if (!get_core_voltage(power_name, &voltage_mV) && (g_sCurrentTemp < g_sCurrentTempThreshLo))
        {
            get_core_lt_voltage(power_name, &vUp);
            set_core_voltage(power_name, VOLTAGE_DEMANDER_TEMPERATURE, vUp);
        }

        // pr_info("%s %d Temp: %d Cpufreq: %d Voltage: %d vUp %d vDown %d width %d cpuinfo.max_freq %d\r\n",
        // __FUNCTION__,
        //         __LINE__, g_sCurrentTemp, current_freq, voltage_mV, vUp, vDown, width, policy.cpuinfo.max_freq);

#endif
    }

    return 0;
}

static int ms_cpufreq_init(struct cpufreq_policy *policy)
{
    int                 ret;
    struct task_struct *thr = NULL;

    if (policy->cpu != 0)
        return -EINVAL;

    policy->clk = devm_clk_get(cpu, 0);
    if (IS_ERR(policy->clk))
    {
        pr_err("[%s] get cpu clk fail\n", __func__);
        return PTR_ERR(policy->clk);
    }

    ret = dev_pm_opp_init_cpufreq_table(cpu, &freq_table);
    if (ret)
    {
        pr_err("[%s] init OPP fail\n", __func__);
        return ret;
    }

    cpufreq_generic_init(policy, freq_table, 100000);

    vidInfo.g_SIDD = INREG16(BASE_REG_OTP2_PA + REG_ID_0A) & 0x3FF;
    vidInfo.g_OSC  = INREG16(BASE_REG_OTP2_PA + REG_ID_0B) & 0x3FF;

    pr_info("%s %d SIDD: 0x%x OSC:0x%x \r\n", __FUNCTION__, __LINE__, vidInfo.g_SIDD, vidInfo.g_OSC);
    pr_info("%s %d SIDD Thres: 0x%x OSC Thres:0x%x \r\n", __FUNCTION__, __LINE__, vidInfo.g_SIDDThreshold,
            vidInfo.g_OSCThreshold);
    // create a thread for monitor temperature
    ms_get_temp(); // We will update temperature after 1sec. Drop first value due to one adc need cost 8ch*8.9usec.

    if ((vidInfo.g_OSC > vidInfo.g_OSCThreshold) && (vidInfo.g_SIDD > vidInfo.g_SIDDThreshold))
    {
        vidInfo.g_OSCTYPE = E_OSC_1;
#if 0 // ndef __DEBUG_OSC__
        thr = kthread_run(monitor_temp_thread_handler, NULL, "monitor_temp");
        if (!thr)
        {
            pr_info("kthread_run fail");
        }
#endif
    }
    else
    {
        vidInfo.g_OSCTYPE = E_OSC_0;
    }
#if 1 // def __DEBUG_OSC__
    thr = kthread_run(monitor_temp_thread_handler, NULL, "monitor_temp");
    if (!thr)
    {
        pr_info("kthread_run fail");
    }
#endif

    pr_info("[%s] Current clk=%lu\n", __func__, clk_get_rate(policy->clk));
    return ret;
}

static int ms_cpufreq_exit(struct cpufreq_policy *policy)
{
    dev_pm_opp_free_cpufreq_table(cpu, &freq_table);

    return 0;
}

static struct cpufreq_driver ms_cpufreq_driver = {
    .verify       = cpufreq_generic_frequency_table_verify,
    .attr         = cpufreq_generic_attr,
    .target_index = ms_cpufreq_target_index,
    .get          = cpufreq_generic_get,
    .init         = ms_cpufreq_init,
    .exit         = ms_cpufreq_exit,
    .name         = "Mstar cpufreq",
};

static int ms_cpufreq_probe(struct platform_device *pdev)
{
    int ret;
#ifdef __DEBUG_IPU__
    struct device_node *    ipu_node;
    struct platform_device *ipu_pdev;
#endif

    cpu = get_cpu_device(0);
    if (dev_pm_opp_of_add_table(cpu))
    {
        pr_err("[%s] add OPP fail\n", __func__);
        return -EINVAL;
    }

#ifdef __DEBUG_IPU__
    ipu_node = of_find_compatible_node(NULL, NULL, "sstar,ipu");
    if (!ipu_node)
    {
        return -1;
    }
    ipu_pdev = of_find_device_by_node(ipu_node);
    if (!ipu_pdev)
    {
        pr_err("fail to find ipu device node\n");
        return -1;
    }
    m_ipufreq.dev = &ipu_pdev->dev;
    if (dev_pm_opp_of_add_table(&ipu_pdev->dev))
    {
        pr_err("fail to add ipu opp table\n");
        return -1;
    }
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0)
    ret = cpufreq_sysfs_create_file(&cpufreq_testout.attr);
    ret |= cpufreq_sysfs_create_file(&temp_adjust_threshold_lo.attr);
    ret |= cpufreq_sysfs_create_file(&temp_adjust_threshold_hi.attr);
    ret |= cpufreq_sysfs_create_file(&temp_out.attr);
    ret |= cpufreq_sysfs_create_file(&sidd_out.attr);
    ret |= cpufreq_sysfs_create_file(&osc_out.attr);
    ret |= cpufreq_sysfs_create_file(&osc_threshold.attr);
#ifdef __DEBUG_IPU__
    ret |= sysfs_create_file(cpufreq_global_kobject, &ipu_freq.attr);
#endif
#else
    ret = sysfs_create_file(cpufreq_global_kobject, &cpufreq_testout.attr);
    ret |= sysfs_create_file(cpufreq_global_kobject, &temp_adjust_threshold_lo.attr);
    ret |= sysfs_create_file(cpufreq_global_kobject, &temp_adjust_threshold_hi.attr);
    ret |= sysfs_create_file(cpufreq_global_kobject, &temp_out.attr);
    ret |= sysfs_create_file(cpufreq_global_kobject, &sidd_out.attr);
    ret |= sysfs_create_file(cpufreq_global_kobject, &osc_out.attr);
    ret |= sysfs_create_file(cpufreq_global_kobject, &osc_threshold.attr);
#ifdef __DEBUG_IPU__
    ret |= sysfs_create_file(cpufreq_global_kobject, &ipu_freq.attr);
#endif
#endif

    if (ret)
    {
        pr_err("[%s] create file fail\n", __func__);
    }

    return cpufreq_register_driver(&ms_cpufreq_driver);
}

static int ms_cpufreq_remove(struct platform_device *pdev)
{
    return cpufreq_unregister_driver(&ms_cpufreq_driver);
}

static const struct of_device_id ms_cpufreq_of_match_table[] = {{.compatible = "sstar,infinity-cpufreq"}, {}};
MODULE_DEVICE_TABLE(of, ms_cpufreq_of_match_table);

static struct platform_driver ms_cpufreq_platdrv = {
    .driver =
        {
            .name           = "ms_cpufreq",
            .owner          = THIS_MODULE,
            .of_match_table = ms_cpufreq_of_match_table,
        },
    .probe  = ms_cpufreq_probe,
    .remove = ms_cpufreq_remove,
};

module_platform_driver(ms_cpufreq_platdrv);
