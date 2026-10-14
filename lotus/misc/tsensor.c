// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/atomic.h>
#include <mach/platform.h>
#include <linux/pm.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/freezer.h>

/* #define _TSENSOR_DEBUG */

#define TSENOR_M_PERIOD_NUM	0x1
#define TSENOR_M_PERIOD		2 //ms
#define REG_SIZE		(4 * 7)


/* Tsensor value otp_address*/
#define OTP_TSENSOR_VALUE_REG		0x12020414
#define OTP_SENSOR_T0_SHIFT		0
#define OTP_SENSOR_T0_MASK		GENMASK(11, OTP_SENSOR_T0_SHIFT)
#define OTP_SENSOR_Y0_SHIFT		12
#define OTP_SENSOR_Y0_MASK		GENMASK(22, OTP_SENSOR_Y0_SHIFT)

#define _DEFAULT_Y0 512
#define _DEFAULT_T0 4764

static int _Y0 = _DEFAULT_Y0;
static int _T0 = _DEFAULT_T0;

static atomic_t g_tsensor_init = ATOMIC_INIT(0);

static void __iomem *g_tsensor_addr;

static unsigned int record_sim = 0;
module_param(record_sim, uint, 0644);
MODULE_PARM_DESC(record_sim, "Simulator value for tsensor record");

static unsigned int record_sim_enable = 0;
module_param(record_sim_enable, uint, 0644);
MODULE_PARM_DESC(record_sim_enable, "Tsensor Simulator Enable");

int get_chip_temperature(void)
{
	unsigned int value = 0;
	int temp;
	int timeout = 500;

	if (atomic_read(&g_tsensor_init) != 1) {
		pr_err("Tsensor NOT init\n");
		return -100;
	}

	timeout = 500;
	while (timeout --) {
		if (record_sim_enable == 0x5AC3A53C && record_sim != 0)
			value = record_sim & 0x3ff;
		else {
			value = readl(g_tsensor_addr + MISC_CTRL47_OFFSET);
			value = value & 0x3ff;
		}
		if (value != 0) {
			/* T = (Y - Y0)*0.303 + T0 */
			temp = ((((int)value - _Y0) * 1241) + _T0 * 4096 / 100) >> 12;
			return temp;
		} else {
			pr_info("Waiting Tsensor ready\n");
			freezable_schedule_timeout(msecs_to_jiffies(TSENOR_M_PERIOD * TSENOR_M_PERIOD_NUM));
		}
	}

	return -100;

}
EXPORT_SYMBOL(get_chip_temperature);

void dump_tsensor_reg(void)
{
	pr_info("Tsensor CTRL0(0x%08X): %08x\n", REG_TSENSOR_BASE, readl(g_tsensor_addr));
	pr_info("Tsensor CTRL1(0x%08X): %08x\n", REG_TSENSOR_BASE + MISC_CTRL46_OFFSET, readl(g_tsensor_addr + MISC_CTRL46_OFFSET));
	{
		int timeout = 10000;
		u32 value;

		do {
			value = readl(g_tsensor_addr + MISC_CTRL47_OFFSET);
			pr_info("Tsensor RECORD(0x%X): %08x\n", REG_TSENSOR_BASE + MISC_CTRL47_OFFSET, value);
			mdelay(10);
			timeout--;
			if (timeout < 9995)
				pr_info("Tsensor timeout!\n");
		} while (!(value & 0x3ff) && (timeout > 0));
	}
}


int init_tsensor_formula(void)
{
	static void __iomem *otp_addr;
	unsigned int data;

	otp_addr = ioremap(OTP_TSENSOR_VALUE_REG, 0x4);
	if (otp_addr == NULL)
		return -ENOMEM;

	data = readl(otp_addr);
	_Y0 = (data & OTP_SENSOR_Y0_MASK) >> OTP_SENSOR_Y0_SHIFT;
	_T0 = (data & OTP_SENSOR_T0_MASK) >> OTP_SENSOR_T0_SHIFT;

	pr_info("OTP Y0:%d, T0:%d\n", _Y0, _T0);

	if ((_Y0 == 0) || (_T0 == 0)) {
		_Y0 = _DEFAULT_Y0;
		_T0 = _DEFAULT_T0;
		pr_info("Y0: %d, T0 %d\n", _Y0, _T0);
	}

	iounmap(otp_addr);

	return 0;
}

static void reg_write32(unsigned long value, unsigned long mask, void __iomem *addr)
{
	unsigned long t;

	t = readl(addr);
	t &= ~mask;
	t |= value & mask;
	writel(t, addr);
}

static int proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "Temperature      : %d\n", get_chip_temperature());

	return 0;
}


static int lotus_tsensor_hw_init(void)
{
	if (g_tsensor_addr == NULL) {
		return -EFAULT;
	}

	pr_info("Tsensor hw init\n");

	reg_write32(0x0, 0xffffffff, g_tsensor_addr);

	/* 2.048ms, en bypass */
	reg_write32(TSENOR_M_PERIOD_NUM << 20, 0xff << 20, g_tsensor_addr);

	udelay(2);

	/* set mode: oneshot circulation */
	reg_write32(0x1 << 30, 0x1 << 30, g_tsensor_addr);

	/* bypass averaging */
	reg_write32(1 << 29, 1 << 29, g_tsensor_addr);

	/* en tsensor */
	reg_write32(0x1 << 31, 0x1 << 31, g_tsensor_addr);

#ifdef _TSENSOR_DEBUG
	dump_tsensor_reg();
#endif
	return 0;
}


static void lotus_tsensor_exit(void)
{
	if ((atomic_read(&g_tsensor_init) == 0) || (g_tsensor_addr == NULL)) {
		return;
	}

	atomic_set(&g_tsensor_init, 0);

	iounmap(g_tsensor_addr);
	g_tsensor_addr = NULL;

	remove_proc_entry("temperature", NULL);
	return;
}

static int lotus_tsensor_probe(struct platform_device *pdev)
{
	int ret;

	g_tsensor_addr = ioremap(REG_TSENSOR_BASE, REG_SIZE);
	if (g_tsensor_addr == NULL) {
		pr_err("ioremap failed\n");
		return -ENOMEM;
	}

	proc_create_single("temperature", 0, NULL, proc_show);

	ret = init_tsensor_formula();
	if (ret != 0)
		goto err;

	ret = lotus_tsensor_hw_init();
	if (ret != 0)
		goto err;

	atomic_set(&g_tsensor_init, 1);

	return ret;
err:
	iounmap(g_tsensor_addr);
	g_tsensor_addr = NULL;

	return ret;
}
static int lotus_tsensor_remove(struct platform_device *pdev)
{
	lotus_tsensor_exit();

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int lotus_tsensor_suspend(struct device *dev)
{
	if (g_tsensor_addr == NULL)
		return -EFAULT;

	/* disable tsensor */
	reg_write32(0x0, 0x1 << 31, g_tsensor_addr);

	atomic_set(&g_tsensor_init, 0);

	return 0;
}

static int lotus_tsensor_resume(struct device *dev)
{

	lotus_tsensor_hw_init();

	atomic_set(&g_tsensor_init, 1);

	return 0;

}
static const struct dev_pm_ops tsensor_pm_ops = {
	.suspend_late        = lotus_tsensor_suspend,
	.resume_early        = lotus_tsensor_resume,
};
#endif


static const struct of_device_id lotus_tsensor_match[] = {
	{ .compatible = "lotus,lotus-tsensor" },
	{},
};
MODULE_DEVICE_TABLE(of, lotus_tsensor_match);

static struct platform_driver lotus_tsensor_driver = {
	.driver = {
		.name	= "lotus-tsensor",
		.of_match_table = lotus_tsensor_match,
#ifdef CONFIG_PM_SLEEP
		.pm	= &tsensor_pm_ops,
#endif
	},
	.probe = lotus_tsensor_probe,
	.remove		= lotus_tsensor_remove,
};

module_platform_driver(lotus_tsensor_driver);

MODULE_AUTHOR("Lotus");
MODULE_DESCRIPTION("LOTUS IPC Tesnsor driver");
MODULE_LICENSE("GPL v2");

