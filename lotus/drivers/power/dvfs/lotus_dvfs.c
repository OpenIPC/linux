// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/mutex.h>

//#define SUPPORT_CPU_594M
//#define SUPPORT_CPU_1000M

#define SVB_PWM1_BASE         0x12028308
#define SVB_PWM1_CTRL0 0x00
#define SVB_PWM1_CTRL1 0x04

#define HPM_CHECK_REG 0x1202015c
/*
 * bit[3:0] 为0xA时，Core使用boot表格配置的曲线，其他值使用默认曲线
 * bit[4] Core Vmin SVB开关，0-关闭，1-开启
 * 其他reserved
 */
#define SVB_VER_REG 0x12020168
#define SVB_MAGIC_WORD 0xA

/*
 * bit[15:0] low switch
 * bit[31:16] high switch
 */
#define CORE_HPM_SWITCH_REG  0x120200b4

#define HPM_VALUE_MIN 300
#define HPM_VALUE_MAX 410

#define duty_cpu_formula(val)  ((unsigned int)((1199 - (val)) * 373) >> 10)

struct dvfs_volt_cal_message {
	unsigned int freq;
	unsigned int volt; /* mv */
	unsigned int (*cpu_get_volt)(unsigned int hpm_value);
};

#define CPU_FREQ_594M  594000000
#define CPU_FREQ_917M  917000000
#define CPU_FREQ_1000M 1000000000
#define CPU_FREQ_1188M 1188000000

#ifdef SUPPORT_CPU_594M
/*
 * The orginal formula is Volt(mV) = 1321.3 - 1.22 * hpm.
 * To avoid float point calculation, make some conversion.
 */
#define volt_cpu_594m_formula(val) (1321 - ((1249 * (val)) >> 10))
#endif
/*
 * The orginal formula is Volt(mV) = 1321.3 - 1.22 * hpm.
 * To avoid float point calculation, make some conversion.
 */
#define volt_cpu_917m_formula(val) (1321 - ((1249 * (val)) >> 10))

#ifdef SUPPORT_CPU_1000M
/*
 * The orginal formula is Volt(mV) =???
 * To avoid float point calculation, make some conversion.
 */
#define volt_cpu_1000m_formula(val) (1321 - ((1249 * (val)) >> 10))
#endif
/*
 * The orginal formula is Volt(mV) = 1525.1 - 1.59 * hpm.
 * To avoid float point calculation, make some conversion.
 */
#define volt_cpu_1188m_formula(val) (1525 - ((1628 * (val)) >> 10))

static DEFINE_MUTEX(cpu_volt_mutex);
static void *g_pwm_base;
static int g_dvfs_volt;
static int g_comp_volt;
static unsigned short g_hpm_min = HPM_VALUE_MIN;
static unsigned short g_hpm_max = HPM_VALUE_MAX;

#ifdef SUPPORT_CPU_594M
static unsigned int get_cpu_594m_volt(unsigned int hpm_core_value)
{
	unsigned int volt;

	if (hpm_core_value < g_hpm_min)
		volt = volt_cpu_594m_formula(g_hpm_min);
	else if (hpm_core_value > g_hpm_max)
		volt = volt_cpu_594m_formula(g_hpm_max);
	else
		volt = volt_cpu_594m_formula(hpm_core_value);

	return volt;
}
#endif

static unsigned int get_cpu_917m_volt(unsigned int hpm_core_value)
{
	unsigned int volt;

	if (hpm_core_value < g_hpm_min)
		volt = volt_cpu_917m_formula(g_hpm_min);
	else if (hpm_core_value > g_hpm_max)
		volt = volt_cpu_917m_formula(g_hpm_max);
	else
		volt = volt_cpu_917m_formula(hpm_core_value);

	return volt;
}

#ifdef SUPPORT_CPU_1000M
static unsigned int get_cpu_1000m_volt(unsigned int hpm_core_value)
{
	unsigned int volt;

	if (hpm_core_value < g_hpm_min)
		volt = volt_cpu_1000m_formula(g_hpm_min);
	else if (hpm_core_value > g_hpm_max)
		volt = volt_cpu_1000m_formula(g_hpm_max);
	else
		volt = volt_cpu_1000m_formula(hpm_core_value);

	return volt;
}
#endif

static unsigned int get_cpu_1188m_volt(unsigned int hpm_core_value)
{
	unsigned int volt;

	if (hpm_core_value < g_hpm_min)
		volt = volt_cpu_1188m_formula(g_hpm_min);
	else if (hpm_core_value > g_hpm_max)
		volt = volt_cpu_1188m_formula(g_hpm_max);
	else
		volt = volt_cpu_1188m_formula(hpm_core_value);

	return volt;
}

static struct dvfs_volt_cal_message g_volt_cal_info[] = {
#ifdef SUPPORT_CPU_594M
	{ CPU_FREQ_594M,  0, get_cpu_594m_volt},
#endif
	{ CPU_FREQ_917M,  0, get_cpu_917m_volt},
#ifdef SUPPORT_CPU_1000M
	{ CPU_FREQ_1000M,  0, get_cpu_1000m_volt},
#endif
	{ CPU_FREQ_1188M, 0, get_cpu_1188m_volt},
};
static unsigned int g_info_count = ARRAY_SIZE(g_volt_cal_info);
unsigned int valid_volt = 0;

static void cpu_volt_update(void)
{
	unsigned int duty;
	int volt;

	volt = g_dvfs_volt + g_comp_volt;
	if ((volt > 0 ) && (g_dvfs_volt > 0)) {
		if (valid_volt != (unsigned int)volt)
			valid_volt = (unsigned int)volt;
		else
			return;

		duty = duty_cpu_formula(valid_volt);

		duty = duty - 1;
		duty = ((duty << 16) & 0XFFFF0000) + 0x0c71;
		writel(duty, g_pwm_base + SVB_PWM1_CTRL0);
		writel(1, g_pwm_base + SVB_PWM1_CTRL1);
	}

}

void lotus_cpufreq_set_volt(unsigned long freq)
{
	unsigned int volt;
	int i;

	for (i = 0; i < g_info_count; i++) {
		if (freq == g_volt_cal_info[i].freq) {
			volt = g_volt_cal_info[i].volt;
			break;
		}
	}
	if (i == g_info_count) {
		pr_info("Freq:%lu, DVFS get Cpu Volt failed !\n", freq);
		return;
	}

	mutex_lock(&cpu_volt_mutex);
	g_dvfs_volt = volt;
	cpu_volt_update();
	mutex_unlock(&cpu_volt_mutex);
}

void lotus_set_cpu_volt_comp(unsigned int volt)
{
	mutex_lock(&cpu_volt_mutex);
	g_comp_volt = volt;
	cpu_volt_update();
	mutex_unlock(&cpu_volt_mutex);
}
EXPORT_SYMBOL(lotus_set_cpu_volt_comp);

unsigned int lotus_get_cpu_volt(void)
{
	int volt;
	volt = (g_dvfs_volt + g_comp_volt);
	if (volt > 0)
		return (unsigned int)volt;
	else
		return 0;
}
EXPORT_SYMBOL(lotus_get_cpu_volt);

void lotus_init_dvfs_volt(void)
{
	void *hpm_check = (void *)ioremap(HPM_CHECK_REG, 0x4);
	unsigned int hpm_value = (readl(hpm_check) >> 16) & 0x1FF;
	void *hpm_switch_adjust = (void *)ioremap(SVB_VER_REG, 0x4);
	void *hpm_switch_reg =  (void *)ioremap(CORE_HPM_SWITCH_REG, 0x4);
	int reg_val;
	int i;

	reg_val = (readl(hpm_switch_adjust) & 0x1f);
	if (reg_val == SVB_MAGIC_WORD) {
		g_hpm_min = readl(hpm_switch_reg) & 0xffff;
		g_hpm_max = (readl(hpm_switch_reg) >> 16 ) & 0xffff;
	} else {
		g_hpm_min = HPM_VALUE_MIN;
		g_hpm_max = HPM_VALUE_MAX;
	}

	g_pwm_base = (void *)ioremap(SVB_PWM1_BASE, 0xf);

	for (i = 0; i < g_info_count; i++)
		g_volt_cal_info[i].volt = g_volt_cal_info[i].cpu_get_volt(hpm_value);

	iounmap(hpm_check);
	iounmap(hpm_switch_adjust);
	iounmap(hpm_switch_reg);
}

void lotus_exit_dvfs_volt(void)
{
	iounmap(g_pwm_base);
	g_pwm_base = NULL;
}

