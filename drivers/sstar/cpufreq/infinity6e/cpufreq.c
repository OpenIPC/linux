/*
* infinity6-cpufreq.c- Sigmastar
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
#ifdef CONFIG_CAM_CLK
#include "drv_camclk_Api.h"
#include "camclk.h"

void *pCpupll = NULL;
#endif


u32 sidd_th_100x = 1243;  //sidd threshold=12.74mA
static struct device *cpu;
static struct cpufreq_frequency_table *freq_table;
int g_sCurrentTemp = 35;
static int g_sCurrentTempThreshLo = 40;
static int g_sCurrentTempThreshHi = 60;
struct timer_list timer_temp;
static int g_SIDD = 0;
static int g_TT50P = 243;

static int ms_cpufreq_target_index(struct cpufreq_policy *policy, unsigned int index)
{
    struct cpufreq_freqs freqs;
    int ret = -1;
    struct dev_pm_opp *opp;
    unsigned long new_freq;
    int opp_voltage_mV;
#ifdef CONFIG_CAM_CLK
    CAMCLK_Set_Attribute stCfg = {.eRoundType = CAMCLK_ROUNDRATE_ROUND,
                                  .eSetType = CAMCLK_SET_ATTR_RATE};
#endif

    freqs.old = policy->cur;
    freqs.new = freq_table[index].frequency;
    new_freq = freqs.new * 1000;

    rcu_read_lock();
    opp = dev_pm_opp_find_freq_ceil(cpu, &new_freq);
    if (IS_ERR(opp)) {
        rcu_read_unlock();
        pr_err("[%s] %d not found in OPP\n", __func__, freqs.new);
        return -EINVAL;
    }
    rcu_read_unlock();

    opp_voltage_mV = (dev_pm_opp_get_voltage(opp)? dev_pm_opp_get_voltage(opp)/1000 : 0);

#ifdef CONFIG_SS_VOLTAGE_CTRL
    if (opp_voltage_mV > get_core_voltage())
    {
        set_core_voltage(VOLTAGE_DEMANDER_CPUFREQ, opp_voltage_mV);
        udelay(10);     //delay 10us to wait voltage stable (from low to high).
#ifdef CONFIG_CAM_CLK
        if (pCpupll)
        {
            stCfg.attribute.u32Rate = new_freq;
            ret = (CamClkAttrSet(pCpupll, &stCfg) == CAMCLK_RET_OK)? 0 : -1;;
        }
#else
        ret = clk_set_rate(policy->clk, new_freq);
#endif
    }
    else
    {
#ifdef CONFIG_CAM_CLK
        if (pCpupll)
        {
            stCfg.attribute.u32Rate = new_freq;
            ret = (CamClkAttrSet(pCpupll, &stCfg) == CAMCLK_RET_OK)? 0 : -1;;
        }
#else
        ret = clk_set_rate(policy->clk, new_freq);
#endif
        set_core_voltage(VOLTAGE_DEMANDER_CPUFREQ, opp_voltage_mV);
    }
#else
    ret = clk_set_rate(policy->clk, new_freq);
#endif

    return ret;
}

#ifdef CONFIG_CAM_CLK
static unsigned int ms_cpufreq_get(unsigned int cpu)
{
    CAMCLK_Get_Attribute stCfg = {0};

    if (pCpupll)
    {
        CamClkAttrGet(pCpupll, &stCfg);
    }

    return stCfg.u32Rate / 1000;
}
#endif

unsigned int get_cpufreq_testout(void)
{
    u16 reg_value = 0;

    if( (INREG8(BASE_REG_RIU_PA + (0x102216 << 1))&BIT0) != BIT0)
    {
        OUTREG8(BASE_REG_RIU_PA + (0x102216 << 1), 0x01);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEE << 1), 0x04);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEA << 1), 0x04);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEA << 1)+1, 0x40);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEC << 1), 0x01);
        OUTREG8(BASE_REG_RIU_PA + (0x101EE0 << 1)+1, 0x00);
        OUTREG8(BASE_REG_RIU_PA + (0x101EE0 << 1)+1, 0x80);
        udelay(100);
    }
    reg_value = (INREG8(BASE_REG_RIU_PA + (0x101EE2 << 1)) | (INREG8(BASE_REG_RIU_PA + (0x101EE2 << 1)+1)<<8));
    return reg_value * 48000;
}

static ssize_t show_cpufreq_testout(struct kobject *kobj, struct attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    u16 reg_value = 0;
    u32 freq = 0;

    if( (INREG8(BASE_REG_RIU_PA + (0x102216 << 1))&BIT0) != BIT0)
    {
        OUTREG8(BASE_REG_RIU_PA + (0x102216 << 1), 0x01);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEE << 1), 0x04);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEA << 1), 0x04);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEA << 1)+1, 0x40);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEC << 1), 0x01);
        OUTREG8(BASE_REG_RIU_PA + (0x101EE0 << 1)+1, 0x00);
        OUTREG8(BASE_REG_RIU_PA + (0x101EE0 << 1)+1, 0x80);
        udelay(100);
    }
    reg_value = (INREG8(BASE_REG_RIU_PA + (0x101EE2 << 1)) | (INREG8(BASE_REG_RIU_PA + (0x101EE2 << 1)+1)<<8));
    //freq = (reg_value * 4000)/83333;
    freq = reg_value * 48000;

    str += scnprintf(str, end - str, "%d\n", freq);

    return (str - buf);
}

static ssize_t store_cpufreq_testout(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
    u32 enable;
    if (sscanf(buf, "%d", &enable)<=0)
        return 0;

    if(enable)
    {
        pr_info("[CPUFREQ] Freq testout ON\n");
        OUTREG8(BASE_REG_RIU_PA + (0x102216 << 1), 0x01);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEE << 1), 0x04);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEA << 1), 0x04);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEA << 1)+1, 0x40);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEC << 1), 0x01);
        OUTREG8(BASE_REG_RIU_PA + (0x101EE0 << 1)+1, 0x00);
        OUTREG8(BASE_REG_RIU_PA + (0x101EE0 << 1)+1, 0x80);
    }
    else
    {
        pr_info("[CPUFREQ] Freq testout OFF\n");
        OUTREG8(BASE_REG_RIU_PA + (0x102216 << 1), 0x00);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEE << 1), 0x00);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEA << 1), 0x00);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEA << 1)+1, 0x00);
        OUTREG8(BASE_REG_RIU_PA + (0x101EEC << 1), 0x00);
        OUTREG8(BASE_REG_RIU_PA + (0x101EE0 << 1)+1, 0x00);
    }

    return count;
}
define_one_global_rw(cpufreq_testout);

static ssize_t show_temp_out(struct kobject *kobj, struct attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "Temp=%d\n", g_sCurrentTemp);

    return (str - buf);
}
define_one_global_ro(temp_out);

static ssize_t show_sidd_out(struct kobject *kobj, struct attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "SIDD=%d\n", g_SIDD);

    return (str - buf);
}
define_one_global_ro(sidd_out);


static ssize_t show_temp_adjust_TT50P_threshold(struct kobject *kobj, struct attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "%d\n", g_TT50P);

    return (str - buf);
}
static ssize_t store_temp_adjust_TT50P_threshold(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
    u32 value;
    if (sscanf(buf, "%d", &value)<=0)
        return 0;
    g_TT50P = value;
    return count;
}
define_one_global_rw(temp_adjust_TT50P_threshold);


static ssize_t show_temp_adjust_threshold_lo(struct kobject *kobj, struct attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "%d\n", g_sCurrentTempThreshLo);

    return (str - buf);
}
static ssize_t store_temp_adjust_threshold_lo(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
    u32 value;
    if (sscanf(buf, "%d", &value)<=0)
        return 0;
    g_sCurrentTempThreshLo = value;
    return count;
}
define_one_global_rw(temp_adjust_threshold_lo);

static ssize_t show_temp_adjust_threshold_hi(struct kobject *kobj, struct attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "%d\n", g_sCurrentTempThreshHi);

    return (str - buf);
}
static ssize_t store_temp_adjust_threshold_hi(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
    u32 value;
    if (sscanf(buf, "%d", &value)<=0)
        return 0;
    g_sCurrentTempThreshHi = value;
    return count;
}
define_one_global_rw(temp_adjust_threshold_hi);

int ms_get_temp(void)
{
    int vbe_code_ft;
    int vbe_code;

    CLRREG16(BASE_REG_PMSAR_PA + REG_ID_19, BIT6); //ch7 reference voltage select to 2.0V
    CLRREG16(BASE_REG_PMSAR_PA + REG_ID_10, BIT0); //reg_pm_dmy
    SETREG16(BASE_REG_PMSLEEP_PA + REG_ID_64, BIT10);
    SETREG16(BASE_REG_PMSLEEP_PA + REG_ID_2F, BIT2);
    OUTREG16(BASE_REG_PMSAR_PA + REG_ID_00, 0xA20);
    SETREG16(BASE_REG_PMSAR_PA + REG_ID_00, BIT14);
    vbe_code = INREG16(BASE_REG_PMSAR_PA + REG_ID_46);
    vbe_code_ft = (INREG16(BASE_REG_OTP_PA + REG_ID_59)>>6) & 0x3FF; //read bit[15:6]

    if (vbe_code_ft == 0)   // if no trim info
        vbe_code_ft = 400;

    //calculate temperature
    return (1370 * (vbe_code_ft - vbe_code))/1000 + 27; //25->27
}
EXPORT_SYMBOL(ms_get_temp);

static int monitor_temp_thread_handler(void *data)
{
    int resync = 0;

    while (!kthread_should_stop())
    {
        msleep_interruptible(1000);

        g_sCurrentTemp = ms_get_temp();

#ifdef CONFIG_SS_VOLTAGE_CTRL
        if(g_SIDD >= g_TT50P && g_sCurrentTemp > g_sCurrentTempThreshHi)
        {
            resync = sync_core_voltage_with_SIDD_and_TEMP(0);
            if (resync)
                set_core_voltage(VOLTAGE_DEMANDER_TEMPERATURE, 0);
        }
        if(g_SIDD >= g_TT50P && g_sCurrentTemp < g_sCurrentTempThreshLo)
        {
            resync = sync_core_voltage_with_SIDD_and_TEMP(1);
            if (resync)
                set_core_voltage(VOLTAGE_DEMANDER_TEMPERATURE, 0);
        }
#endif
    }

    return 0;
}

static int ms_cpufreq_init(struct cpufreq_policy *policy)
{
    int ret;
    struct task_struct *thr = NULL;
#ifdef CONFIG_CAM_CLK
    CAMCLK_Get_Attribute stCfg = {0};
#endif


    if (policy->cpu != 0)
        return -EINVAL;

#ifndef CONFIG_CAM_CLK
    policy->clk = devm_clk_get(cpu, 0);
    if (IS_ERR(policy->clk)) {
        pr_err("[%s] get cpu clk fail\n", __func__);
        return PTR_ERR(policy->clk);
    }
#endif

    ret = dev_pm_opp_init_cpufreq_table(cpu, &freq_table);
    if (ret) {
        pr_err("[%s] init OPP fail\n", __func__);
        return ret;
    }

    ret = cpufreq_generic_init(policy, freq_table, 100000);
    if (ret) {
        pr_err("[%s] init policy fail\n", __func__);
        goto fail;
    }

    policy->min = 800000;
    policy->max = 800000;

    g_SIDD = INREG16(BASE_REG_OTP_PA + REG_ID_42) & 0x3FF; //read bit[9:0]

    //create a thread for monitor temperature
    ms_get_temp(); //We will update temperature after 1sec. Drop first value due to one adc need cost 8ch*8.9usec.
    thr = kthread_run(monitor_temp_thread_handler, NULL, "monitor_temp");
    if (!thr) {
        pr_info("kthread_run fail");
    }

#ifdef CONFIG_CAM_CLK
    if (pCpupll)
    {
        CamClkAttrGet(pCpupll, &stCfg);
    }
    pr_info("[%s] Current clk=%u\n", __func__, stCfg.u32Rate);
#else
    pr_info("[%s] Current clk=%lu\n", __func__, clk_get_rate(policy->clk));
#endif
    return ret;

fail:
    dev_pm_opp_free_cpufreq_table(cpu, &freq_table);

    return ret;
}

static int ms_cpufreq_exit(struct cpufreq_policy *policy)
{
    dev_pm_opp_free_cpufreq_table(cpu, &freq_table);
#ifdef CONFIG_CAM_CLK
    CamClkUnregister(pCpupll);
    pCpupll = NULL;
#endif

    return 0;
}

static struct cpufreq_driver ms_cpufreq_driver = {
    .verify = cpufreq_generic_frequency_table_verify,
    .attr   = cpufreq_generic_attr,
    .target_index = ms_cpufreq_target_index,
#ifdef CONFIG_CAM_CLK
    .get    = ms_cpufreq_get,
#else
    .get    = cpufreq_generic_get,
#endif
    .init   = ms_cpufreq_init,
    .exit   = ms_cpufreq_exit,
    .name   = "Mstar cpufreq",
};

static int ms_cpufreq_probe(struct platform_device *pdev)
{
    int ret;
#ifdef CONFIG_CAM_CLK
    u32 clk_id = 0;

    of_property_read_u32_index(pdev->dev.of_node, "camclk", 0, &clk_id);
    if (!clk_id)
    {
        printk(KERN_DEBUG "[%s] Fail to get clk!\n", __func__);
    }
    else
    {
        CamClkRegister("CPUPLL", clk_id, &pCpupll);
    }
#endif

    cpu = get_cpu_device(0);
    if (dev_pm_opp_of_add_table(cpu)) {
         pr_err("[%s] add OPP fail\n", __func__);
         return -EINVAL;
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,4,0)
    ret = cpufreq_sysfs_create_file(&cpufreq_testout.attr);
    ret |= cpufreq_sysfs_create_file(&temp_adjust_threshold_lo.attr);
    ret |= cpufreq_sysfs_create_file(&temp_adjust_threshold_hi.attr);
    ret |= cpufreq_sysfs_create_file(&temp_out.attr);
    ret |= cpufreq_sysfs_create_file(&sidd_out.attr);
    ret |= cpufreq_sysfs_create_file(&temp_adjust_TT50P_threshold.attr);
#else
    ret = sysfs_create_file(cpufreq_global_kobject, &cpufreq_testout.attr);
    ret |= sysfs_create_file(cpufreq_global_kobject, &temp_adjust_threshold_lo.attr);
    ret |= sysfs_create_file(cpufreq_global_kobject, &temp_adjust_threshold_hi.attr);
    ret |= sysfs_create_file(cpufreq_global_kobject, &temp_out.attr);
    ret |= sysfs_create_file(cpufreq_global_kobject, &sidd_out.attr);
    ret |= sysfs_create_file(cpufreq_global_kobject, &temp_adjust_TT50P_threshold.attr);
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

static const struct of_device_id ms_cpufreq_of_match_table[] = {
    { .compatible = "sstar,infinity-cpufreq" },
    {}
};
MODULE_DEVICE_TABLE(of, ms_cpufreq_of_match_table);

static struct platform_driver ms_cpufreq_platdrv = {
    .driver = {
        .name  = "ms_cpufreq",
        .owner = THIS_MODULE,
        .of_match_table = ms_cpufreq_of_match_table,
    },
    .probe  = ms_cpufreq_probe,
    .remove = ms_cpufreq_remove,
};

module_platform_driver(ms_cpufreq_platdrv);
