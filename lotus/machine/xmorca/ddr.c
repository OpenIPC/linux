// SPDX-License-Identifier: GPL-2.0

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/printk.h>
#include <asm/neon.h>

#include <mach/platform.h>
#include <mach/common.h>

char ddr_type[8] = {0};
void __iomem *ddrc_base;
void __iomem *ddrphy_crg;
static DEFINE_MUTEX(ddr_mutex);

u32 ddrc_readl(u32 reg)
{
	WARN_ON(!ddrc_base);

	return readl(ddrc_base + reg);
}

void ddrc_writel(u32 v, u32 reg)
{
	WARN_ON(!ddrc_base);

	writel(v, ddrc_base + reg);
}

int get_ddr_type(void)
{
	void *ddrc_top_feature;
	u32 v;
	static int ddr_type = INIT_TYPE;

	if (ddr_type != INIT_TYPE)
		return ddr_type;


	ddrc_top_feature = ioremap(REG_DDRC_TOP_FEATURE, PAGE_SIZE);
	if (ddrc_top_feature == NULL) {
		pr_err("ddr ctrl map failed\n");
		return UNKNOWN_DDR_TYPE;
	}

	v = readl(ddrc_top_feature) & 0x7;
	iounmap(ddrc_top_feature);

	switch (v) {
	case 1: /* DDR3 */
		ddr_type = DDR3_TYPE;
		break;
	case 2: /* DDR4 */
		ddr_type = DDR4_TYPE;
		break;
	case 3: /* LPDDR4 */
		ddr_type = LPDDR4_TYPE;
		break;
	default:
		ddr_type = UNKNOWN_DDR_TYPE;
		break;
	}

	return ddr_type;
}
EXPORT_SYMBOL(get_ddr_type);

char *get_ddr_type_string(int *length)
{
	void *ddrc_top_feature = NULL;

	if (ddr_type[0] == 0) {
		u32 v;

		ddrc_top_feature = ioremap(REG_DDRC_TOP_FEATURE, PAGE_SIZE);
		if (ddrc_top_feature == NULL) {
			pr_err("ddr ctrl map failed\n");
			return NULL;
		}

		v = readl(ddrc_top_feature) & 0x7;
		iounmap(ddrc_top_feature);

		memset(ddr_type, 0, sizeof(ddr_type));
		switch (v) {
		case 1: /* DDR3 */
			strncpy(ddr_type, "DDR3", sizeof(ddr_type));
			break;
		case 2: /* DDR4 */
			strncpy(ddr_type, "DDR4", sizeof(ddr_type));
			break;
		case 3: /* LPDDR4 */
			strncpy(ddr_type, "LPDDR4", sizeof(ddr_type));
			break;
		default:
			strncpy(ddr_type, "Unknown", sizeof(ddr_type));
			break;
		}

		ddr_type[sizeof(ddr_type) - 1] = '\0';
	}

	if (length)
		*length = strnlen(ddr_type, sizeof(ddr_type));

	return ddr_type;
}
EXPORT_SYMBOL(get_ddr_type_string);

int get_ddr_freq(void)
{
	u32 v;
	u32 ddr_freq = 0;

	BUG_ON(!ddrphy_crg);
	v = (readl(ddrphy_crg) >> 8) & 0x7;

	switch (v) {
		case 0:
			ddr_freq = 24;
			break;
		case 1:
			ddr_freq = 525;
			break;
		case 2:
			ddr_freq = 496;
			break;
		case 3:
			ddr_freq = 450;
			break;
		case 4:
			ddr_freq = 350;
			break;
		case 5:
			ddr_freq = 263;
			break;
		case 6:
			ddr_freq = 198;
			break;
		case 7:
			ddr_freq = 99;
			break;
		default:
			break;
	}

	return (ddr_freq * 4);
}
EXPORT_SYMBOL(get_ddr_freq);

int get_ddr_bandwidth_util(u32 time_ms, int *integer, int *decimal)
{
	u32 v, eff_integer, eff_decimal, eff_time;
	u64 eff_out;
	u32 ddr_freq = get_ddr_freq();

	if (time_ms == 0)
		return -EINVAL;

#define EFF_TIME_MASK ((1 << 24) - 1)
	eff_time = (time_ms * 1000 * ddr_freq) / 4;
	if (eff_time > EFF_TIME_MASK)
		return -EINVAL;

	mutex_lock(&ddr_mutex);

	v = ddrc_readl(DFI_EFFI_TIME_CH0) & ~EFF_TIME_MASK;
	v |= eff_time & EFF_TIME_MASK;
	ddrc_writel(v, DFI_EFFI_TIME_CH0);

	v = ddrc_readl(DFI_EFFI_CTRL_CH0);
	v &= ~1;
	ddrc_writel(v, DFI_EFFI_CTRL_CH0);

	v = ddrc_readl(DFI_EFFI_CTRL_CH0);
	v |= 1;
	v &= ~(1 << 1);
	ddrc_writel(v, DFI_EFFI_CTRL_CH0);

	if (get_ddr_type() == LPDDR4_TYPE) {
		v = ddrc_readl(DFI_EFFI_TIME_CH1) & ~EFF_TIME_MASK;
		v |= eff_time & EFF_TIME_MASK;
		ddrc_writel(v, DFI_EFFI_TIME_CH1);

		v = ddrc_readl(DFI_EFFI_CTRL_CH1);
		v &= ~1;
		ddrc_writel(v, DFI_EFFI_CTRL_CH1);

		v = ddrc_readl(DFI_EFFI_CTRL_CH1);
		v |= 1;
		v &= ~(1 << 1);
		ddrc_writel(v, DFI_EFFI_CTRL_CH1);
	}

	msleep(time_ms);

	eff_out = ddrc_readl(DFI_EFFI_OUT_CH0) & EFF_TIME_MASK;
	if (get_ddr_type() == LPDDR4_TYPE) {
		eff_out += ddrc_readl(DFI_EFFI_OUT_CH1) & EFF_TIME_MASK;
		do_div(eff_out, 2);
	}

	mutex_unlock(&ddr_mutex);

#ifdef CONFIG_KERNEL_MODE_NEON
	kernel_neon_begin();
	calc_bandwidth_util((u32)eff_out, eff_time, &eff_integer, &eff_decimal);
	kernel_neon_end();
#else
	eff_integer = (u32)eff_out * 100 / eff_time;
	eff_decimal = 0;
#endif

	if (decimal)
		*decimal = eff_decimal;

	if (integer)
		*integer = eff_integer;

	return 0;

}
EXPORT_SYMBOL(get_ddr_bandwidth_util);

int ddrc_init(void)
{
	if (ddrc_base == NULL) {
		/* Map all DDR Controller registers */
		ddrc_base = ioremap(REG_DDRC_BASE, 32 * 1024);
		if (ddrc_base == NULL) {
			pr_err("ddrc map failed\n");
			return -ENOMEM;
		}

		ddrphy_crg = ioremap(REG_DDRPHY_CRG, PAGE_SIZE);
		if (ddrphy_crg == NULL) {
			pr_err("ddrphy crg map failed\n");
			return -ENOMEM;
		}
	}

	return 0;
}

void ddrc_exit(void)
{
	if (ddrc_base)
		iounmap(ddrc_base);
	if (ddrphy_crg)
		iounmap(ddrphy_crg);
}

