#include <linux/regmap.h>
#include <linux/mm.h>
#include <asm/cacheflush.h>
#include <asm/set_memory.h>

#include <linux/cpu.h>
#include <linux/err.h>
#include <linux/genalloc.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/sizes.h>
#include <linux/sram.h>
#include <linux/suspend.h>
#include <asm/suspend.h>
#include <linux/lotus/misc.h>

#define AUX_SAVE_ADDR					0x4013200
#define UBOOT_HEADER_PARAM_LEN			0x2800
#define AUX_AREA_SIZE					0x5000
#define AUX_SIZE						(AUX_AREA_SIZE + UBOOT_HEADER_PARAM_LEN)

#define LP_ENTRY_ADDR					0x4010000
#define DDR_PARAM_SAVE_ADDR				0x401b800
#define DDR_PARAM_SAVE_SIZE				0x800

#define BASE_PMC_PHY_ADDR				0x120f0000
#define PMC_REG_SIZE					0x1000
#define BASE_DDRPHY_PHY_ADDR			0x120d8000
#define DDRPHY_REG_SIZE					0x1000
#define BASE_DFI_PHY_ADDR				0x120d3000
#define DFI_REG_SIZE					0x1000
#define BASE_DDRC_PHY_ADDR				0x120d0000
#define DDRC_REG_SIZE					0x1000

#define REG_STR_FLAG					0xC3321142
#define REG_RAM_OFF_STR_FLAG			0xC3321140
#define REG_STR_FLAG_OFFSET				0x48

#define REG_RAM_OFF_OFFSET				0x58
#define RAM_OFF_SET						0x1
#define RAM_ON_SET						0x0

#define REG_RAM_OFF_LINUX_ENTRY_OFFSET	0x50

#define REG_PWR_TIME_SEQUENCE_OFFSET	0x14

struct xmorca_pm_param {
	unsigned long arm_resume_addr;
	unsigned long early_resume_addr;
	unsigned long aux_dst_phy_addr;
	unsigned long aux_size;
	unsigned long lp_entry_addr;
	void __iomem *lp_entry_vir;
	unsigned long ddr_param_addr;
	unsigned long str_mode;
	void __iomem *base_pmc_vir;
	void __iomem *base_ddrphy_vir;
	void __iomem *base_ddrc_vir;
	void __iomem *ddr_param_vir;
	void __iomem *aux_src_vir;
	void __iomem *aux_dst_vir;
};

#include "ddr_cali_save.c"

extern unsigned int suspend_fun_size;
extern void xmorca_suspend(void);
extern void xmorca_dismmu_jump(unsigned int phyaddr);

static bool ram_power_off = 1;
module_param(ram_power_off, bool, 0644);
/*
 * here are phy addr
 */
static struct xmorca_pm_param local_xmorca_pm_param = {
	.aux_dst_phy_addr = AUX_SAVE_ADDR,
	.aux_size = AUX_SIZE,
	.lp_entry_addr = LP_ENTRY_ADDR,
	.ddr_param_addr = DDR_PARAM_SAVE_ADDR,
};

int pm_set_aux_src(void __iomem *aux_src, unsigned long size)
{
	local_xmorca_pm_param.aux_src_vir = aux_src;
	local_xmorca_pm_param.aux_size = size;

	return 0;
}
EXPORT_SYMBOL(pm_set_aux_src);

int pm_ddr_param_get(struct lpds_param *p_lpds_param)
{
	ddr_param_read(&local_xmorca_pm_param, p_lpds_param);

	return 0;
}
EXPORT_SYMBOL(pm_ddr_param_get);


static int xmorca_pm_latest_suspend(void)
{
	return 0;
}

static int xmorca_pm_earliest_resume(void)
{
	printk("xmorca resume\n");
	return 0;
}

static void disable_cache(void)
{
	/*
	 * disable icache and dcache
	 * get cp15 register to r0
	 * clear icache bit12
	 * clear dcache bit2
	 * write r0 to cp15 register
	 */
	__asm__ __volatile__(
        "mrc p15, 0, r0, c1, c0, 0\n"
        "bic r0, r0, #(1 << 12)\n"
        "bic r0, r0, #(1 << 2)\n"
        "mcr p15, 0, r0, c1, c0, 0\n"
        "isb\n"
        ::: "r0", "memory"
    );
}

/* this wait maybe in sram */
static int wait_ddr_idle(void)
{
	return 0;
}

extern void dump_rtc_reg(void);

static int xmorca_str_enter(unsigned long arg)
{
	disable_cache();
	flush_cache_all();

	/* wait ddr do not write */
	if (wait_ddr_idle()) {
		pr_err("ddr is busy, can not entry str\n");
		return 1;
	}

	printk("xmorca ready to str ...\n");

	dump_reg("===PMC REG===", local_xmorca_pm_param.base_pmc_vir, BASE_PMC_PHY_ADDR, 0x70, "===END===");

	dump_rtc_reg();

	xmorca_dismmu_jump(local_xmorca_pm_param.lp_entry_addr);

	pr_err("%s: Failed to suspend\n", __func__);
	return 1;
}

static int xmorca_pm_suspend(suspend_state_t suspend_state)
{
	xmorca_pm_latest_suspend();
	cpu_suspend(0, xmorca_str_enter);

	dump_reg("===PMC REG===", local_xmorca_pm_param.base_pmc_vir, BASE_PMC_PHY_ADDR, 0x70, "===END===");

	/*
	 * resume
	 */
	xmorca_pm_earliest_resume();

	return 0;
}

static int xmorca_pm_enter(suspend_state_t suspend_state)
{
	int ret = 0;

	switch (suspend_state) {
	case PM_SUSPEND_MEM:
		ret = xmorca_pm_suspend(suspend_state);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int xmorca_pm_valid(suspend_state_t state)
{
	switch (state) {
	case PM_SUSPEND_MEM:
		return 1;
	default:
		return 0;
	}
}

static int xmorca_pwr_time_sequence_cfg(struct xmorca_pm_param *pm_param)
{
	if (ram_power_off)
		writel(0, pm_param->base_pmc_vir + REG_PWR_TIME_SEQUENCE_OFFSET);
	else
		writel(0xa0000, pm_param->base_pmc_vir + REG_PWR_TIME_SEQUENCE_OFFSET);

	return 0;
}

static int xmorca_set_str_flag(struct xmorca_pm_param *pm_param)
{
	if (ram_power_off) {
		printk("ram off str\n");
		writel(REG_RAM_OFF_STR_FLAG, pm_param->base_pmc_vir + REG_STR_FLAG_OFFSET);
		writel(RAM_OFF_SET, pm_param->base_pmc_vir + REG_RAM_OFF_OFFSET);
	} else {
		writel(REG_STR_FLAG, pm_param->base_pmc_vir + REG_STR_FLAG_OFFSET);
		writel(RAM_ON_SET, pm_param->base_pmc_vir + REG_RAM_OFF_OFFSET);
	}

	return 0;
}

static int xmorca_set_arm_resume_addr(struct xmorca_pm_param *pm_param)
{
	struct lpds_param *p_lpds_param = (struct lpds_param *)pm_param->ddr_param_vir;

	if (ram_power_off)
		writel(pm_param->arm_resume_addr, pm_param->base_pmc_vir + REG_RAM_OFF_LINUX_ENTRY_OFFSET);
	else {
		printk("ram on str\n");
		p_lpds_param->resume_addr = pm_param->arm_resume_addr;
		p_lpds_param->resume_addr_backup = pm_param->arm_resume_addr;
	}

	return 0;
}

/* copy aux and boot header from ddr to ram */
static int copy_early_resume_code(struct xmorca_pm_param *pm_param)
{
	if (ram_power_off)
		return 0;

	else {

		if (pm_param->aux_src_vir == NULL || pm_param->aux_size == 0)
			return -1;

		memcpy(pm_param->aux_dst_vir, pm_param->aux_src_vir, pm_param->aux_size);
		return 0;
	}
}

static int copy_lp_entry_code(struct xmorca_pm_param *pm_param)
{
	memcpy((void *)pm_param->lp_entry_vir, (void *)xmorca_suspend, suspend_fun_size);

	return 0;
}

static int save_ddr_paramter(struct xmorca_pm_param *pm_param)
{
	if (!ram_power_off)
		ddr_phy_save_calib(pm_param);

	return 0;
}

static int xmorca_pm_prepare(void)
{
	int ret;

	printk("%s entry\n", __func__);
	/*
	 * set resume address, we use cpu_resume's phy address
	 */
	xmorca_set_arm_resume_addr(&local_xmorca_pm_param);

	/* set str mode to register */
	/* save ddr paramter */
	save_ddr_paramter(&local_xmorca_pm_param);

	/*
	 * copy resume code to ram
	 * set resume entry
	 * cpu will set resume entry to register afer set ddr to low power mode
	 */
	ret = copy_early_resume_code(&local_xmorca_pm_param);
	if (ret)
		return ret;

	/*
	 * copy low power entry code of risc-v to tcm
	 * set risc-v low power entry
	 */
	copy_lp_entry_code(&local_xmorca_pm_param);

	xmorca_set_str_flag(&local_xmorca_pm_param);

	xmorca_pwr_time_sequence_cfg(&local_xmorca_pm_param);

	printk("%s success\n", __func__);
	return 0;
}

static struct platform_suspend_ops xmorca_pm_ops =
{
	.prepare = xmorca_pm_prepare,
	.enter = xmorca_pm_enter,
	.valid = xmorca_pm_valid,
};

static int xmorca_pm_probe(struct platform_device *pdev)
{
	local_xmorca_pm_param.base_pmc_vir = ioremap(BASE_PMC_PHY_ADDR, PMC_REG_SIZE);
	if (!local_xmorca_pm_param.base_pmc_vir) {
		printk("pmc ioremap error\n");
		return -ENOMEM;
	}

	local_xmorca_pm_param.base_ddrphy_vir = ioremap(BASE_DDRPHY_PHY_ADDR, DDRPHY_REG_SIZE);
	if (!local_xmorca_pm_param.base_ddrphy_vir) {
		printk("ddr phy ioremap error\n");
		iounmap(local_xmorca_pm_param.base_pmc_vir);
		return -ENOMEM;
	}

	local_xmorca_pm_param.base_ddrc_vir = ioremap(BASE_DDRC_PHY_ADDR, DDRC_REG_SIZE);
	if (!local_xmorca_pm_param.base_ddrc_vir) {
		printk("ddrc ioremap error\n");
		iounmap(local_xmorca_pm_param.base_ddrphy_vir);
		iounmap(local_xmorca_pm_param.base_pmc_vir);
		return -ENOMEM;
	}

	local_xmorca_pm_param.ddr_param_vir = ioremap(DDR_PARAM_SAVE_ADDR, DDR_PARAM_SAVE_SIZE);
	if (!local_xmorca_pm_param.ddr_param_vir) {
		printk("ddr param ioremap error\n");
		iounmap(local_xmorca_pm_param.base_ddrc_vir);
		iounmap(local_xmorca_pm_param.base_ddrphy_vir);
		iounmap(local_xmorca_pm_param.base_pmc_vir);
		return -ENOMEM;
	}

	local_xmorca_pm_param.aux_dst_vir = ioremap(local_xmorca_pm_param.aux_dst_phy_addr, local_xmorca_pm_param.aux_size);
	if (!local_xmorca_pm_param.aux_dst_vir) {
		printk("aux dst ioremap error\n");
		iounmap(local_xmorca_pm_param.base_ddrc_vir);
		iounmap(local_xmorca_pm_param.ddr_param_vir);
		iounmap(local_xmorca_pm_param.base_pmc_vir);
		iounmap(local_xmorca_pm_param.base_ddrphy_vir);
		return -ENOMEM;
	}

	local_xmorca_pm_param.lp_entry_vir = ioremap(local_xmorca_pm_param.lp_entry_addr, suspend_fun_size);
	if (!local_xmorca_pm_param.lp_entry_vir) {
		printk("lp entry ioremap error\n");
		iounmap(local_xmorca_pm_param.base_ddrc_vir);
		iounmap(local_xmorca_pm_param.ddr_param_vir);
		iounmap(local_xmorca_pm_param.base_pmc_vir);
		iounmap(local_xmorca_pm_param.base_ddrphy_vir);
		iounmap(local_xmorca_pm_param.aux_dst_vir);
		return -ENOMEM;
	}

	set_memory_x((unsigned long)local_xmorca_pm_param.lp_entry_vir, 1);

	local_xmorca_pm_param.arm_resume_addr = __pa_symbol(cpu_resume);
	suspend_set_ops(&xmorca_pm_ops);

	printk("%s probe success\n", __func__);
	return 0;
}

static int xmorca_pm_remove(struct platform_device *pdev)
{
	suspend_set_ops(NULL);

	return 0;
}

static struct platform_driver xmorca_pm_driver = {
	.driver = {
		.name   = "xmorca",
	},
	.probe = xmorca_pm_probe,
	.remove = xmorca_pm_remove,
};
module_platform_driver(xmorca_pm_driver);

static struct platform_device xmorca_pm_dev = {
    .name = "xmorca",
    .id = -1,
};

static int __init pm_dev_init(void)
{
    return platform_device_register(&xmorca_pm_dev);
}

static void __exit pm_dev_exit(void)
{
    platform_device_unregister(&xmorca_pm_dev);
}

module_init(pm_dev_init);
module_exit(pm_dev_exit);

MODULE_ALIAS("platform:pm xmorca");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("xmorca power management driver");

