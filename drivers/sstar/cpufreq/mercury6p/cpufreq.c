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

u32                                    sidd_th_100x = 1243; // sidd threshold=12.74mA
static struct device *                 cpu;
static struct cpufreq_frequency_table *freq_table;
int                                    g_sCurrentTemp         = 35;
static int                             g_sCurrentTempThreshLo = 40;
static int                             g_sCurrentTempThreshHi = 60;
struct timer_list                      timer_temp;
static int                             g_SIDD         = 0;
static int                             g_OSC          = 0;
static int                             g_OSCThreshold = 468;

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

    rcu_read_lock();
    opp = dev_pm_opp_find_freq_ceil(cpu, &new_freq);
    if (IS_ERR(opp))
    {
        rcu_read_unlock();
        pr_err("[%s] %d not found in OPP\n", __func__, freqs.new);
        return -EINVAL;
    }
    opp_voltage_mV = (dev_pm_opp_get_voltage(opp) ? dev_pm_opp_get_voltage(opp) / 1000 : 0);
    rcu_read_unlock();

#if defined(CONFIG_SS_VOLTAGE_CTRL) || defined(CONFIG_SS_VOLTAGE_IDAC_CTRL)
    if (!get_core_voltage("cpu_power", &voltage_mV) && opp_voltage_mV > voltage_mV)
    {
        set_core_voltage("cpu_power", VOLTAGE_DEMANDER_CPUFREQ, opp_voltage_mV);
        udelay(10); // delay 10us to wait voltage stable (from low to high).
        ret = clk_set_rate(policy->clk, new_freq);
    }
    else
    {
        ret = clk_set_rate(policy->clk, new_freq);
        set_core_voltage("cpu_power", VOLTAGE_DEMANDER_CPUFREQ, opp_voltage_mV);
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

    str += scnprintf(str, end - str, "Temp=%d\n", g_sCurrentTemp);

    return (str - buf);
}
define_one_global_ro(temp_out);

static ssize_t show_sidd_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "%d\n", g_SIDD);

    return (str - buf);
}
define_one_global_ro(sidd_out);

//#define __DEBUG_OSC__
#ifndef __DEBUG_OSC__
static ssize_t show_osc_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "%d\n", g_OSC);

    return (str - buf);
}
define_one_global_ro(osc_out);

#else

static ssize_t show_osc_out(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "%d\n", g_OSC);

    return (str - buf);
}

static ssize_t store_osc_out(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    u32 value;
    if (sscanf(buf, "%d", &value) <= 0)
        return 0;
    g_OSC = value;
    return count;
}

define_one_global_rw(osc_out);
#endif

static ssize_t show_osc_threshold(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "%d\n", g_OSCThreshold);

    return (str - buf);
}

static ssize_t store_osc_threshold(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    u32 value;
    if (sscanf(buf, "%d", &value) <= 0)
        return 0;
    g_OSCThreshold = value;
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

    CLRREG16(BASE_REG_PMSAR_PA + REG_ID_19, BIT6);        // ch7 reference voltage select to 2.0V
    CLRREG16(BASE_REG_PMSAR_PA + REG_ID_10, BIT0);        // reg_pm_dmy
    SETREG16(BASE_REG_PMSLEEP_PA + REG_ID_64, BIT10);     // reg_gcr_sel_ext_int, select current flow to sar or t_sen
    SETREG16(BASE_REG_PMSLEEP_PA + REG_ID_2F, BIT2);      // reg_en_tsen, enable current to sar or t_sensor
    OUTREG16(BASE_REG_PMSAR_PA + REG_ID_00, 0xA20);       // sar_freerun
    SETREG16(BASE_REG_PMSAR_PA + REG_ID_00, BIT14);       // enable load sar code
    vbe_code    = INREG16(BASE_REG_PMSAR_PA + REG_ID_46); // sar adc output 7
    vbe_code_ft = ((INREG16(BASE_REG_OTP3_PA + REG_ID_40) & 0xFFC0) >> 6);
    if (vbe_code_ft == 0) // if no trim info
        vbe_code_ft = 803;

    // calculate temperature
    return (670 * (vbe_code_ft - vbe_code)) / 1000 + 23;
}
EXPORT_SYMBOL(ms_get_temp);

static int monitor_temp_thread_handler(void *data)
{
#if defined(CONFIG_SS_VOLTAGE_CTRL) || defined(CONFIG_SS_VOLTAGE_IDAC_CTRL)
    int voltage_mV;
#endif
    while (!kthread_should_stop())
    {
        msleep_interruptible(1000);

        g_sCurrentTemp = ms_get_temp();

#if defined(CONFIG_SS_VOLTAGE_CTRL)
        if (g_OSC >= g_OSCThreshold)
        {
            if (!get_core_voltage("cpu_power", &voltage_mV) && voltage_mV > VOLTAGE_CORE_900
                && g_sCurrentTemp > g_sCurrentTempThreshHi)
                set_core_voltage("cpu_power", VOLTAGE_DEMANDER_TEMPERATURE, VOLTAGE_CORE_900);
            if (!get_core_voltage("cpu_power", &voltage_mV) && voltage_mV < VOLTAGE_CORE_1000
                && g_sCurrentTemp < g_sCurrentTempThreshLo)
                set_core_voltage("cpu_power", VOLTAGE_DEMANDER_TEMPERATURE, VOLTAGE_CORE_1000);

            if (!get_core_voltage("core_power", &voltage_mV) && voltage_mV > VOLTAGE_CORE_900
                && g_sCurrentTemp > g_sCurrentTempThreshHi)
                set_core_voltage("core_power", VOLTAGE_DEMANDER_TEMPERATURE, VOLTAGE_CORE_900);
            if (!get_core_voltage("core_power", &voltage_mV) && voltage_mV < VOLTAGE_CORE_1000
                && g_sCurrentTemp < g_sCurrentTempThreshLo)
                set_core_voltage("core_power", VOLTAGE_DEMANDER_TEMPERATURE, VOLTAGE_CORE_1000);
        }

#elif defined(CONFIG_SS_VOLTAGE_IDAC_CTRL)
        if (g_OSC >= g_OSCThreshold)
        {
            if (!get_core_voltage("cpu_power", &voltage_mV) && voltage_mV > get_lv_voltage("cpu_power")
                && g_sCurrentTemp > g_sCurrentTempThreshHi)
                sync_voltage_with_OSCandTemp("cpu_power", 0);
            if (!get_core_voltage("cpu_power", &voltage_mV) && voltage_mV < get_tt_voltage("cpu_power")
                && g_sCurrentTemp < g_sCurrentTempThreshLo)
                sync_voltage_with_OSCandTemp("cpu_power", 1);

            if (!get_core_voltage("core_power", &voltage_mV) && voltage_mV > get_lv_voltage("core_power")
                && g_sCurrentTemp > g_sCurrentTempThreshHi)
                sync_voltage_with_OSCandTemp("core_power", 0);
            if (!get_core_voltage("core_power", &voltage_mV) && voltage_mV < get_tt_voltage("core_power")
                && g_sCurrentTemp < g_sCurrentTempThreshLo)
                sync_voltage_with_OSCandTemp("core_power", 1);
        }
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

    g_SIDD = INREG16(BASE_REG_OTP2_PA + REG_ID_0A) & 0x3FF; // read bit[9:0]
    g_OSC  = INREG16(BASE_REG_OTP2_PA + REG_ID_0B) & 0x3FF; // read bit[9:0]

    g_OSCThreshold = 468;

    // create a thread for monitor temperature
    ms_get_temp(); // We will update temperature after 1sec. Drop first value due to one adc need cost 8ch*8.9usec.
    thr = kthread_run(monitor_temp_thread_handler, NULL, "monitor_temp");
    if (!thr)
    {
        pr_info("kthread_run fail");
    }

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
    int            ret;
    struct clk *   clk;
    struct clk_hw *hw_parent;

    cpu = get_cpu_device(0);
    if (dev_pm_opp_of_add_table(cpu))
    {
        pr_err("[%s] add OPP fail\n", __func__);
        return -EINVAL;
    }

    clk = of_clk_get(pdev->dev.of_node, 0);
    if (IS_ERR(clk))
    {
        ret = PTR_ERR(clk);
        return ret;
    }
    else
    {
        hw_parent = clk_hw_get_parent_by_index(__clk_get_hw(clk), 0); // select mux 0
        if (!hw_parent)
            return -ENXIO;

        ret = clk_set_parent(clk, hw_parent->clk);
        if (ret)
            return ret;

        ret = clk_prepare_enable(clk);
        if (ret)
            return ret;
    }

    platform_set_drvdata(pdev, clk);

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0)
    ret = cpufreq_sysfs_create_file(&cpufreq_testout.attr);
    ret |= cpufreq_sysfs_create_file(&temp_adjust_threshold_lo.attr);
    ret |= cpufreq_sysfs_create_file(&temp_adjust_threshold_hi.attr);
    ret |= cpufreq_sysfs_create_file(&temp_out.attr);
    ret |= cpufreq_sysfs_create_file(&sidd_out.attr);
    ret |= cpufreq_sysfs_create_file(&osc_out.attr);
    ret |= cpufreq_sysfs_create_file(&osc_threshold.attr);
#else
    ret = sysfs_create_file(cpufreq_global_kobject, &cpufreq_testout.attr);
    ret |= sysfs_create_file(cpufreq_global_kobject, &temp_adjust_threshold_lo.attr);
    ret |= sysfs_create_file(cpufreq_global_kobject, &temp_adjust_threshold_hi.attr);
    ret |= sysfs_create_file(cpufreq_global_kobject, &temp_out.attr);
    ret |= sysfs_create_file(cpufreq_global_kobject, &sidd_out.attr);
    ret |= sysfs_create_file(cpufreq_global_kobject, &osc_out.attr);
    ret |= sysfs_create_file(cpufreq_global_kobject, &osc_threshold.attr);
#endif

    if (ret)
    {
        pr_err("[%s] create file fail\n", __func__);
    }

    return cpufreq_register_driver(&ms_cpufreq_driver);
}

static int ms_cpufreq_remove(struct platform_device *pdev)
{
    struct clk *clk = platform_get_drvdata(pdev);

    clk_disable_unprepare(clk);

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0)
    cpufreq_sysfs_remove_file(&cpufreq_testout.attr);
    cpufreq_sysfs_remove_file(&temp_adjust_threshold_lo.attr);
    cpufreq_sysfs_remove_file(&temp_adjust_threshold_hi.attr);
    cpufreq_sysfs_remove_file(&temp_out.attr);
    cpufreq_sysfs_remove_file(&sidd_out.attr);
    cpufreq_sysfs_remove_file(&osc_out.attr);
    cpufreq_sysfs_remove_file(&osc_threshold.attr);
#else
    sysfs_remove_file(cpufreq_global_kobject, &cpufreq_testout.attr);
    sysfs_remove_file(cpufreq_global_kobject, &temp_adjust_threshold_lo.attr);
    sysfs_remove_file(cpufreq_global_kobject, &temp_adjust_threshold_hi.attr);
    sysfs_remove_file(cpufreq_global_kobject, &temp_out.attr);
    sysfs_remove_file(cpufreq_global_kobject, &sidd_out.attr);
    sysfs_remove_file(cpufreq_global_kobject, &osc_out.attr);
    sysfs_remove_file(cpufreq_global_kobject, &osc_threshold.attr);
#endif
    return cpufreq_unregister_driver(&ms_cpufreq_driver);
}

static const struct of_device_id ms_cpufreq_of_match_table[] = {{.compatible = "sstar,cpufreq"}, {}};
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
