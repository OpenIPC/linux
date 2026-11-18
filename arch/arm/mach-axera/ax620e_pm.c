/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/regmap.h>
#include <linux/suspend.h>
#include <linux/mfd/syscon.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>
#include <asm/suspend.h>
#include <asm/system_info.h>
#include <asm/cputype.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include <linux/proc_fs.h>
#include "ax620e_pm.h"
#include "ax620e_pm_reg.h"
#include <linux/ax_timestamp.h>

#define func_2_iram	__aligned(4)

typedef enum {
	SLP_EXIT = 0,
	SLP_ENTER = 1,
} slp_state_t;

typedef enum MODULES_ENUM {
	MODULE_GROUP1_START = 0,
	MODULE_CPU = MODULE_GROUP1_START,
	MODULE_DDR,
	MODULE_FLASH,
	MODULE_ISP,
	MODULE_MM,
	MODULE_NPU,
	MODULE_VPU,
	MODULE_PERIPH,
} MODULES_ENUM;

typedef enum SLP_EN_ENUM {
	SLP_EN_SET = 1,
	SLP_EN_CLR = 2,
} SLP_EN_ENUM;

typedef enum PWR_STATE_ENUM {
	PWR_STATE_ON = 0,
	PWR_STATE_OFF = 5,
} PWR_STATE_ENUM;

static void __iomem *pmu_glb_base;
static void __iomem *comm_sys_glb_base;
static void __iomem *pinmux_g6_base;
static void __iomem *cpu_sys_glb_base;
static void __iomem *dbg_sys_glb_base;
static void __iomem *iram_base;
static void __iomem *ddrc_base;
static void __iomem *flash_sys_glb_base;
static void __iomem *isp_sys_glb_base;
static void __iomem *mm_sys_glb_base;
static void __iomem *npu_sys_glb_base;
static void __iomem *vpu_sys_glb_base;
static void __iomem *periph_sys_glb_base;
static void __iomem *comm_cpu_glb_base;
static void __iomem *pllc_glb_base;
static void __iomem *dpll_ctrl_glb_base;
static void __iomem *ddr_sys_glb_base;
static void __iomem *sleep_stage_addr;
static struct proc_dir_entry *pll_choose_root;
static pgd_t *pm_idmap_pgd;
static unsigned int a64_warmreset_a32_code[] = {
	0xD5033F9F, 0xD5033FDF, 0xD2800040, 0xD51EC040,
	0xD53800A1, 0x92400C21, 0xF100003F, 0x540000A1,
	0x58000102, 0xB9400041, 0x11000421, 0xB9000041,
	0xD5033FDF, 0xD503207F, 0xD503201F, 0x14000000,
	0x0000401C, 0x00000000, 0x00000000, 0x00000000
};
static unsigned int pll_state = 0;

extern unsigned int ax620e_slp_resume_code_sz;
extern void ax620e_slp_cpu_resume(void);
extern void ax620e_cpu_sleep_enter(void);

#define PLL_CHOOSE_ROOT_NAME	"ax_proc/pll_choose"
#define PLL_STATE 	"pll_always_on"
#define TIMER32_COUNT_PER_SEC		32768
#define SYS_PWR_STATE_PER_GRP_MAX	8
#define SYS_PWR_STATE_LEN		4
#define SYS_PWR_STATE_MASK		0xF
#define SLEEP_STAGE_STORE_ADDR	0x4200

static inline int pmu_get_module_state(void *pmu_base, MODULES_ENUM module, unsigned int *state)
{
	int i;
	unsigned int value;
	int ret = -1;

	MODULES_ENUM grp1_modules[SYS_PWR_STATE_PER_GRP_MAX] =
	{MODULE_CPU, MODULE_DDR, MODULE_FLASH, MODULE_ISP, MODULE_MM,
	 MODULE_NPU, MODULE_VPU, MODULE_PERIPH};

	for (i = 0; i < SYS_PWR_STATE_PER_GRP_MAX; i++) {
		if (grp1_modules[i] == module) {
			ret = 0;
			break;
		}
	}
	if (!ret) {
		value = readl(pmu_base + PMU_GLB_PWR_STATE_ADDR);
		*state = (value >> (i * SYS_PWR_STATE_LEN)) & SYS_PWR_STATE_MASK;
	}

	return ret;
}

static inline int pmu_module_sleep_en(void *pmu_base, MODULES_ENUM module, SLP_EN_ENUM en_set_clr)
{
	int ret = 0;

	switch (module) {
	case MODULE_PERIPH:
	case MODULE_VPU:
	case MODULE_NPU:
	case MODULE_MM:
	case MODULE_ISP:
	case MODULE_FLASH:
	case MODULE_DDR:
	case MODULE_CPU:
		if (SLP_EN_SET == en_set_clr) {
			writel((1 << (module - MODULE_GROUP1_START)), pmu_base + PMU_GLB_SLP_EN_SET_ADDR);
		} else if (SLP_EN_CLR == en_set_clr) {
			writel((1 << (module - MODULE_GROUP1_START)), pmu_base + PMU_GLB_SLP_EN_CLR_ADDR);
		}
		break;
	default:
		ret = -1;
	}

	return ret;
}

static inline int pmu_module_wakeup_set_clr(void *pmu_base, MODULES_ENUM module, int set)
{
	int ret = 0;

	switch (module) {
	case MODULE_CPU:
	case MODULE_DDR:
	case MODULE_FLASH:
	case MODULE_ISP:
	case MODULE_MM:
	case MODULE_NPU:
	case MODULE_VPU:
	case MODULE_PERIPH:
		if (set) {
			writel((1 << (module - MODULE_GROUP1_START)), pmu_base + PMU_GLB_WAKUP_SET_ADDR);
		} else {
			writel((1 << (module - MODULE_GROUP1_START)), pmu_base + PMU_GLB_WAKUP_CLR_ADDR);
		}
		break;
	default:
		ret = -1;
	}

	return ret;
}

static inline int pmu_module_wakeup(void *pmu_base, MODULES_ENUM module)
{
	unsigned int state = 0x5;
	int ret;
	int i;
	unsigned int value;

	MODULES_ENUM grp1_modules[SYS_PWR_STATE_PER_GRP_MAX] =
	{MODULE_CPU, MODULE_DDR, MODULE_FLASH, MODULE_ISP, MODULE_MM,
	 MODULE_NPU, MODULE_VPU, MODULE_PERIPH};

	ret = pmu_module_wakeup_set_clr(pmu_base, module, 1);
	if (ret)
		goto RET;

	for (i = 0; i < SYS_PWR_STATE_PER_GRP_MAX; i++) {
		if (grp1_modules[i] == module) {
			ret = 0;
			break;
		}
	}
	while(state != PWR_STATE_ON) {
		value = readl(pmu_base + PMU_GLB_PWR_STATE_ADDR);
		state = (value >> (i * SYS_PWR_STATE_LEN)) & SYS_PWR_STATE_MASK;
	}

	/* wake_up_clr */
	ret = pmu_module_wakeup_set_clr(pmu_base, module, 0);

RET:
	return ret;
}

static void ax620e_pmu_init(void __iomem *pmu_base)
{
	writel(0, pmu_base + PMU_GLB_PWR_WAIT0_ADDR);
	writel(0, pmu_base + PMU_GLB_PWR_WAIT1_ADDR);
	writel(0x0, pmu_base + PMU_GLB_PWR_WAITON0_ADDR);
	writel(0x0, pmu_base + PMU_GLB_PWR_WAITON1_ADDR);
	writel(0xFFFF, pmu_base + PMU_GLB_INT_CLR_SET_ADDR);
}

static void ax620e_chip_top_init(void)
{
	/* mask all wakeup source here , will be enabled in wakeup source drivers */
	writel(EIC_EN_MASK_ALL, comm_sys_glb_base + EIC_EN_CLR);

	writel(0, comm_sys_glb_base + COMMON_SYS_PAD_SLEEP_BYP_ADDR);
	writel(0, comm_sys_glb_base + COMMON_SYS_PLL_SLEEP_BYP_ADDR);
	writel(1, comm_sys_glb_base + COMMON_SYS_XTAL_SLEEP_BYP_ADDR);

	/* cpll force on */
	writel(BIT_PLL_GRP_PLL_FRC_EN_SW_SET, pllc_glb_base + PLL_GRP_PLL_FRC_EN_SW_SET_ADDR);
	writel(BIT_PLL_GRP_PLL_FRC_EN_SET, pllc_glb_base + PLL_GRP_PLL_FRC_EN_SET_ADDR);
	/* set cpll wait cycle to minimum */
	writel(BITS_PLL_CPLL_ON_CFG_CLR, pllc_glb_base + PLL_CPLL_ON_CFG_CLR_ADDR);
	/* set cpupll wait ref clock to minimum */
	writel(BITS_PLL_CPUPLL_ON_CFG_CLR, pllc_glb_base + PLL_CPUPLL_ON_CFG_CLR_ADDR);
	/* set epll wait ref clock to minimum */
	writel(BITS_PLL_EPLL_ON_CFG_CLR, pllc_glb_base + PLL_EPLL_ON_CFG_CLR_ADDR);
	/* set hpll wait ref clock to minimum */
	writel(BITS_PLL_HPLL_ON_CFG_CLR, pllc_glb_base + PLL_HPLL_ON_CFG_CLR_ADDR);
	/* set npll wait ref clock to minimum */
	writel(BITS_PLL_NPLL_ON_CFG_CLR, pllc_glb_base + PLL_NPLL_ON_CFG_CLR_ADDR);
	/* set vpll0 wait ref clock to minimum */
	writel(BITS_PLL_VPLL0_ON_CFG_CLR, pllc_glb_base + PLL_VPLL0_ON_CFG_CLR_ADDR);
	/* set vpll1 wait ref clock to minimum */
	writel(BITS_PLL_VPLL1_ON_CFG_CLR, pllc_glb_base + PLL_VPLL1_ON_CFG_CLR_ADDR);
	/* set dpll wait ref clock to minimum */
	writel(BITS_DPLL_ON_CFG_CLR, dpll_ctrl_glb_base + DPLL_ON_CFG_CLR_ADDR);

	//writel(PINMUX_G6_CTRL_SET_SET, pinmux_g6_base + PINMUX_G6_MISC_SET_ADDR);

	//writel(BIT_COMMON_SYS_EIC_MASK_ENABLE_SET, comm_sys_glb_base + COMMON_SYS_EIC_MASK_ENABLE_SET_ADDR);

	/* mask all deb gpio int */
	writel(BIT_COMMON_SYS_DEB_GPIO_MASK_ALL, comm_sys_glb_base + COMMON_SYS_DEB_GPIO_31_0_INT_CLR_SET_ADDR);
	writel(BIT_COMMON_SYS_DEB_GPIO_MASK_ALL, comm_sys_glb_base + COMMON_SYS_DEB_GPIO_31_0_INT_MASK_SET_ADDR);
	writel(BIT_COMMON_SYS_DEB_GPIO_MASK_ALL, comm_sys_glb_base + COMMON_SYS_DEB_GPIO_63_32_INT_CLR_SET_ADDR);
	writel(BIT_COMMON_SYS_DEB_GPIO_MASK_ALL, comm_sys_glb_base + COMMON_SYS_DEB_GPIO_63_32_INT_MASK_SET_ADDR);
	writel(BIT_COMMON_SYS_DEB_GPIO_MASK_ALL, comm_sys_glb_base + COMMON_SYS_DEB_GPIO_89_64_INT_CLR_SET_ADDR);
	writel(BIT_COMMON_SYS_DEB_GPIO_MASK_ALL, comm_sys_glb_base + COMMON_SYS_DEB_GPIO_89_64_INT_MASK_SET_ADDR);

	/* clear usb wakeup interrupt here or usb int will affect flash sys sleep */
	writel(BIT_COMMON_SYS_USB_WAKE_UP_INT_CLR_SET, comm_sys_glb_base + COMMON_SYS_USB_INT_CTRL_SET_ADDR);
	writel(BIT_COMMON_SYS_USB_WAKE_UP_INT_MASK_SET, comm_sys_glb_base + COMMON_SYS_USB_INT_CTRL_SET_ADDR);

	/* settings for auto gate */
	writel(MISC_CTRL_VAL, comm_sys_glb_base + MISC_CTRL);
	writel(CPU_SYS_FAB_GT_CTRL0_VAL, cpu_sys_glb_base + CPU_SYS_FAB_GT_CTRL0_ADDR);
	writel(DEBUG_SYS_AUTO_GT_EN_VAL, dbg_sys_glb_base + DEBUG_SYS_AUTO_GT_EN);

	/* set core0/1 wakeup addr */
	writel(0, iram_base + A64_TO_A32_CNT_ADDR);  /* addr record sleep cnt */
	writel(((long long)A64_WARMRST_A32_ADDR >> 32), comm_cpu_glb_base + CA53_CFG_RVBARADDR0_H);
	writel((A64_WARMRST_A32_ADDR & 0xFFFFFFFF), comm_cpu_glb_base + CA53_CFG_RVBARADDR0_L);
	writel(((long long)A64_WARMRST_A32_ADDR >> 32), comm_cpu_glb_base + CA53_CFG_RVBARADDR1_H);
	writel((A64_WARMRST_A32_ADDR & 0xFFFFFFFF), comm_cpu_glb_base + CA53_CFG_RVBARADDR1_L);

	writel(0x0, iram_base + PLL_ALWAYS_ON_ADDR);
	*((volatile unsigned long long*)(iram_base + WAKEUP_START_TIMESTAMP_ADDR)) = 0x0;
}

static int ax620e_chip_lpmode_enter(unsigned long arg)
{
	void (*sleep_fn)(void);

	flush_cache_all();

	cpu_switch_mm(pm_idmap_pgd, &init_mm);
	local_flush_bp_all();
	local_flush_tlb_all();

	sleep_fn = (void (*)(void))(IRAM_BASE_PHY_ADDR +
							(unsigned int)ax620e_cpu_sleep_enter -
							(unsigned int)ax620e_slp_cpu_resume);
	sleep_fn();

	pr_err("%s: Failed to suspend\n", __func__);

	/* should never here */
	BUG();

	return 1;
}

static int axera_get_resource_byname(struct device_node *np, char *rname, struct resource *res)
{
	int index, ret;

	index = of_property_match_string(np, "reg-names", rname);
	if (index < 0) {
		pr_err("%s get %s property index failed\n", __func__, rname);
		return index;
	}

	ret = of_address_to_resource(np, index, res);
	if (ret < 0) {
		pr_err("%s get %s of index:%d resource failed\n", __func__, rname, index);
		return ret;
	}

	return 0;
}

static void __iomem *axera_get_iomem_byname(struct device_node *np, char *rname)
{
	int ret;
	struct resource res;

	ret = axera_get_resource_byname(np, rname, &res);
	if (ret)
		return NULL;

	return ioremap(res.start, resource_size(&res));
}

static void flash_sys_sleep(slp_state_t state)
{
	static unsigned int flash_sys_clk_eb_0_reserve = 0;
	static unsigned int flash_sys_clk_eb_1_reserve = 0;
	static unsigned int flash_sys_clk_mux_0_reserve = 0;
	static unsigned int flash_sys_clk_div_0_reserve = 0;
	static unsigned int flash_sys_clk_div_1_reserve = 0;
	int ret = -1;
	unsigned int pwr_state;

	switch (state) {
	case SLP_ENTER:
		flash_sys_clk_mux_0_reserve = readl(flash_sys_glb_base + FLASH_SYS_CLK_MUX_0_ADDR);
		flash_sys_clk_eb_0_reserve = readl(flash_sys_glb_base + FLASH_SYS_CLK_EB_0_ADDR);
		flash_sys_clk_eb_1_reserve = readl(flash_sys_glb_base + FLASH_SYS_CLK_EB_1_ADDR);
		flash_sys_clk_div_0_reserve = readl(flash_sys_glb_base + FLASH_SYS_CLK_DIV_0_ADDR);
		flash_sys_clk_div_1_reserve = readl(flash_sys_glb_base + FLASH_SYS_CLK_DIV_1_ADDR);

		writel(BIT_COMMON_SYS_USB_WAKE_UP_INT_MASK_SET, comm_sys_glb_base + COMMON_SYS_USB_INT_CTRL_SET_ADDR);
		writel(BIT_COMMON_SYS_USB_WAKE_UP_INT_CLR_SET, comm_sys_glb_base + COMMON_SYS_USB_INT_CTRL_SET_ADDR);

		writel(BIT_FLASH_LPC_CFG_BUS_IDLE_EN_SET, flash_sys_glb_base + FLASH_SYS_LPC_CFG_SET_ADDR);
		writel(BIT_FLASH_LPC_DATA_BUS_IDLE_EN_SET, flash_sys_glb_base + FLASH_SYS_LPC_DATA_SET_ADDR);
		writel(BIT_FLASH_LPC_DATA_IDLE_EN_SET, flash_sys_glb_base + FLASH_SYS_LPC_SET_ADDR);
		writel(BIT_FLASH_LPC_BP_CLK_EB_SET, flash_sys_glb_base + FLASH_SYS_LPC_SET_ADDR);

		while(readl(flash_sys_glb_base + FLASH_SYS_FAB_BUSY_ADDR) != 0x0);

		writel(BITS_FLASH_SYS_CLK_EB_0, flash_sys_glb_base + FLASH_SYS_CLK_EB_0_ADDR);
		writel(BITS_FLASH_SYS_CLK_EB_1, flash_sys_glb_base + FLASH_SYS_CLK_EB_1_ADDR);

		writel(BIT_FLASH_LPC_DATA_BUS_FRC_WORK, flash_sys_glb_base + FLASH_SYS_LPC_DATA_CLR_ADDR);

		writel(BIT_FLASH_LPC_SLP_EN_SET, flash_sys_glb_base + FLASH_SYS_LPC_SET_ADDR);

		pmu_module_sleep_en(pmu_glb_base, MODULE_FLASH, SLP_EN_SET);

		while (!(ret = pmu_get_module_state(pmu_glb_base, MODULE_FLASH, &pwr_state)) && (pwr_state != PWR_STATE_OFF));
		pr_info("flash sleep state is 0x%x\r\n", pwr_state);
		break;
	case SLP_EXIT:
		pmu_module_sleep_en(pmu_glb_base, MODULE_FLASH, SLP_EN_CLR);
		ret = pmu_module_wakeup(pmu_glb_base, MODULE_FLASH);

		/* special module not power down so sleep en should clear */
		writel(BIT_FLASH_LPC_SLP_EN_CLR, flash_sys_glb_base + FLASH_SYS_LPC_CLR_ADDR);
		/* special module not power down so idle en should clear */
		writel(BIT_FLASH_LPC_CFG_BUS_IDLE_EN_CLR, flash_sys_glb_base + FLASH_SYS_LPC_CFG_CLR_ADDR);
		writel(BIT_FLASH_LPC_DATA_BUS_IDLE_EN_CLR, flash_sys_glb_base + FLASH_SYS_LPC_DATA_CLR_ADDR);
		writel(BIT_FLASH_LPC_DATA_IDLE_EN_CLR, flash_sys_glb_base + FLASH_SYS_LPC_CLR_ADDR);

		writel(BIT_FLASH_LPC_DATA_BUS_FRC_WORK, flash_sys_glb_base + FLASH_SYS_LPC_DATA_SET_ADDR);

		ret = pmu_get_module_state(pmu_glb_base, MODULE_FLASH, &pwr_state);
		if (!ret)
			pr_info("flash sys wakeup state is 0x%x\r\n", pwr_state);

		writel(flash_sys_clk_mux_0_reserve, flash_sys_glb_base + FLASH_SYS_CLK_MUX_0_ADDR);
		writel(flash_sys_clk_eb_0_reserve, flash_sys_glb_base + FLASH_SYS_CLK_EB_0_ADDR);
		writel(flash_sys_clk_eb_1_reserve, flash_sys_glb_base + FLASH_SYS_CLK_EB_1_ADDR);
		writel(flash_sys_clk_div_0_reserve, flash_sys_glb_base + FLASH_SYS_CLK_DIV_0_ADDR);
		writel(flash_sys_clk_div_1_reserve, flash_sys_glb_base + FLASH_SYS_CLK_DIV_1_ADDR);
		break;
	default:
		pr_err("%s param input is invalid:%d", __func__, state);
		break;
	}
}

static void isp_sys_sleep(slp_state_t state)
{
	static unsigned int isp_sys_clk_mux_0_reserve = 0;
	static unsigned int isp_sys_clk_eb_0_reserve = 0;
	static unsigned int isp_sys_clk_eb_1_reserve = 0;
	unsigned int pwr_state = 0;
	int ret;

	switch (state) {
	case SLP_ENTER:
		isp_sys_clk_mux_0_reserve = readl(isp_sys_glb_base + ISP_SYS_CLK_MUX_0_ADDR);
		isp_sys_clk_eb_0_reserve = readl(isp_sys_glb_base + ISP_SYS_CLK_EB_0_ADDR);
		isp_sys_clk_eb_1_reserve = readl(isp_sys_glb_base + ISP_SYS_CLK_EB_1_ADDR);

		/* lpc_cfg_bus_idle_en & lpc_data_bus_idle_en */
		writel(BIT_ISP_LPC_DATA_BUS_IDLE_EN_SET, isp_sys_glb_base + ISP_SYS_LPC_DATA_SET_ADDR);
		writel(BIT_ISP_LPC_CFG_BUS_IDLE_EN_SET, isp_sys_glb_base + ISP_SYS_LPC_CFG_SET_ADDR);
		writel(BIT_ISP_LPC_DATA_IDLE_EN_SET, isp_sys_glb_base + ISP_SYS_LPC_DATA_SET_ADDR);
		writel(BIT_ISP_SYS_LPC_SW_RST_CLR, isp_sys_glb_base + ISP_SYS_SW_RST_1_CLR_ADDR);

		/* clear clk_eb */
		writel(BITS_ISP_CLK_EB_0, isp_sys_glb_base + ISP_SYS_CLK_EB_0_ADDR); /*close clk except 24M */
		writel(BITS_ISP_CLK_EB_1, isp_sys_glb_base + ISP_SYS_CLK_EB_1_ADDR); /*close clk except lpc */

		/* enable sys sleep enable and pmu sys sleep en */
		writel(BIT_ISP_SLEEP_EN_SET, isp_sys_glb_base + ISP_SYS_SLEEP_EN_SET_ADDR);
		pmu_module_sleep_en(pmu_glb_base, MODULE_ISP, SLP_EN_SET);

		/* check sys power state till off */
		while (!(ret = pmu_get_module_state(pmu_glb_base, MODULE_ISP, &pwr_state)) && (pwr_state != PWR_STATE_OFF));
		pr_info("isp sleep state is 0x%x\r\n", pwr_state);
		break;
	case SLP_EXIT:
		pmu_module_sleep_en(pmu_glb_base, MODULE_ISP, SLP_EN_CLR);

		ret = pmu_module_wakeup(pmu_glb_base, MODULE_ISP);

		writel(BIT_ISP_LPC_DATA_BUS_IDLE_EN_CLR, isp_sys_glb_base + ISP_SYS_LPC_DATA_CLR_ADDR);
		writel(BIT_ISP_LPC_CFG_BUS_IDLE_EN_CLR, isp_sys_glb_base + ISP_SYS_LPC_CFG_CLR_ADDR);
		writel(BIT_ISP_LPC_DATA_IDLE_EN_CLR, isp_sys_glb_base + ISP_SYS_LPC_DATA_CLR_ADDR);
		writel(BIT_ISP_SLEEP_EN_CLR, isp_sys_glb_base + ISP_SYS_SLEEP_EN_CLR_ADDR);

		ret = pmu_get_module_state(pmu_glb_base, MODULE_ISP, &pwr_state);
		pr_info("isp wakeup state is 0x%x\r\n", pwr_state);

		writel(isp_sys_clk_mux_0_reserve, isp_sys_glb_base + ISP_SYS_CLK_MUX_0_ADDR);
		writel(isp_sys_clk_eb_0_reserve, isp_sys_glb_base + ISP_SYS_CLK_EB_0_ADDR);
		writel(isp_sys_clk_eb_1_reserve, isp_sys_glb_base + ISP_SYS_CLK_EB_1_ADDR);
		break;
	default:
		pr_err("%s param input is invalid:%d", __func__, state);
		break;
	}
}

static void mm_sys_sleep(slp_state_t state)
{
	static unsigned int mm_sys_clk_mux_0_reserve = 0;
	static unsigned int mm_sys_clk_eb_0_reserve = 0;
	static unsigned int mm_sys_clk_eb_1_reserve = 0;
	static unsigned int mm_sys_clk_div_0_reserve = 0;
	unsigned int pwr_state;
	int ret;

	switch (state) {
	case SLP_ENTER:
		mm_sys_clk_mux_0_reserve = readl(mm_sys_glb_base + MM_SYS_CLK_MUX_0_ADDR);
		mm_sys_clk_eb_0_reserve = readl(mm_sys_glb_base + MM_SYS_CLK_EB_0_ADDR);
		mm_sys_clk_eb_1_reserve = readl(mm_sys_glb_base + MM_SYS_CLK_EB_1_ADDR);
		mm_sys_clk_div_0_reserve = readl(mm_sys_glb_base + MM_SYS_CLK_DIV_0_ADDR);

		/* lpc_cfg_bus_idle_en & lpc_data_bus_idle_en */
		writel(BIT_MM_LPC_CFG_BUS_IDLE_EN_SET, mm_sys_glb_base + MM_SYS_LPC1_SET_ADDR);
		writel(BIT_MM_LPC_DATA_BUS_IDLE_EN_SET, mm_sys_glb_base + MM_SYS_LPC2_SET_ADDR);
		writel(BIT_MM_DATA_IDLE_EN_SET, mm_sys_glb_base + MM_SYS_LPC0_SET_ADDR);
		writel(BIT_MM_SYS_LPC_SW_RST_CLR, mm_sys_glb_base + MM_SYS_SW_RST_0_CLR_ADDR);

		/* clear clk_eb */
		writel(BITS_MM_SYS_CLK_EB_1, mm_sys_glb_base + MM_SYS_CLK_EB_1_ADDR);
		writel(BITS_MM_SYS_CLK_EB_0, mm_sys_glb_base + MM_SYS_CLK_EB_0_ADDR);

		/* enable sys sleep enable and pmu sys sleep en */
		writel(BIT_MM_LPC_SLP_EN_SET, mm_sys_glb_base + MM_SYS_LPC0_SET_ADDR);
		pmu_module_sleep_en(pmu_glb_base, MODULE_MM, SLP_EN_SET);

		/* check sys power state till off */
		while (!(ret = pmu_get_module_state(pmu_glb_base, MODULE_MM, &pwr_state)) && (pwr_state != PWR_STATE_OFF));

		pr_info("mm sleep state is 0x%x\r\n", pwr_state);
		break;
	case SLP_EXIT:
		pmu_module_sleep_en(pmu_glb_base, MODULE_MM, SLP_EN_CLR);

		ret = pmu_module_wakeup(pmu_glb_base, MODULE_MM);

		writel(BIT_MM_LPC_CFG_BUS_IDLE_EN_CLR, mm_sys_glb_base + MM_SYS_LPC1_CLR_ADDR);
		writel(BIT_MM_LPC_DATA_BUS_IDLE_EN_CLR, mm_sys_glb_base + MM_SYS_LPC2_CLR_ADDR);
		writel(BIT_MM_DATA_IDLE_EN_CLR, mm_sys_glb_base + MM_SYS_LPC0_CLR_ADDR);
		writel(BIT_MM_LPC_SLP_EN_CLR, mm_sys_glb_base + MM_SYS_LPC0_CLR_ADDR);

		ret = pmu_get_module_state(pmu_glb_base, MODULE_MM, &pwr_state);
		pr_info("mm sys wakeup state is 0x%x\r\n", pwr_state);

		writel(mm_sys_clk_mux_0_reserve, mm_sys_glb_base + MM_SYS_CLK_MUX_0_ADDR);
		writel(mm_sys_clk_eb_0_reserve, mm_sys_glb_base + MM_SYS_CLK_EB_0_ADDR);
		writel(mm_sys_clk_eb_1_reserve, mm_sys_glb_base + MM_SYS_CLK_EB_1_ADDR);
		writel(mm_sys_clk_div_0_reserve, mm_sys_glb_base + MM_SYS_CLK_DIV_0_ADDR);
		break;
	default:
		pr_err("%s param input is invalid:%d", __func__, state);
		break;
	}
}

static void npu_sys_sleep(slp_state_t state)
{
	static unsigned int npu_sys_clk_mux_0_reserve = 0;
	static unsigned int npu_sys_clk_eb_0_reserve = 0;
	static unsigned int npu_sys_clk_eb_1_reserve = 0;
	int ret;
	unsigned int pwr_state;

	switch (state) {
	case SLP_ENTER:
		npu_sys_clk_mux_0_reserve = readl(npu_sys_glb_base + NPU_SYS_CLK_MUX_0_ADDR);
		npu_sys_clk_eb_0_reserve = readl(npu_sys_glb_base + NPU_SYS_CLK_EB_0_ADDR);
		npu_sys_clk_eb_1_reserve = readl(npu_sys_glb_base + NPU_SYS_CLK_EB_1_ADDR);

		/* lpc_cfg_bus_idle_en & lpc_data_bus_idle_en */
		writel(BIT_NPU_SYS_CFG_BUS_IDLE_EN_SET, npu_sys_glb_base + NPU_SYS_LPC_CFG_SETTING_SET_ADDR);
		writel(BIT_NPU_SYS_DATA_BUS_IDLE_EN_SET, npu_sys_glb_base + NPU_SYS_LPC_DATA_SETTING_SET_ADDR);
		writel(BIT_NPU_SYS_DATA_IDLE_EB_SET, npu_sys_glb_base + NPU_SYS_LPC_SETTING_SET_ADDR);
		writel(BIT_NPU_SYS_LPC_SW_RST_CLR, npu_sys_glb_base + NPU_SYS_SW_RST_0_CLR_ADDR);

		/* wait until eu_idle_sts = 1 */
		while (readl(npu_sys_glb_base + NPU_SYS_EU_IDLE_STS_ADDR) != 0x1);

		/* clear clk_eb except clk_lpc_eb */
		writel(BITS_NPU_SYS_CLK_EB_0, npu_sys_glb_base + NPU_SYS_CLK_EB_0_ADDR);
		writel(BITS_NPU_SYS_CLK_EB_1, npu_sys_glb_base + NPU_SYS_CLK_EB_1_ADDR);

		/* enable sys sleep enable and pmu sys sleep en */
		writel(BIT_NPU_SYS_SLP_EN_SET, npu_sys_glb_base + NPU_SYS_LPC_SETTING_SET_ADDR);
		pmu_module_sleep_en(pmu_glb_base, MODULE_NPU, SLP_EN_SET);

		/* check sys power state till off */
		while (!(ret = pmu_get_module_state(pmu_glb_base, MODULE_NPU, &pwr_state)) && (pwr_state != PWR_STATE_OFF));

		pr_info("npu sys sleep state is 0x%x\r\n", pwr_state);
		break;
	case SLP_EXIT:
		pmu_module_sleep_en(pmu_glb_base, MODULE_NPU, SLP_EN_CLR);
		ret = pmu_module_wakeup(pmu_glb_base, MODULE_NPU);

		writel(BIT_NPU_SYS_CFG_BUS_IDLE_EN_CLR, npu_sys_glb_base + NPU_SYS_LPC_CFG_SETTING_CLR_ADDR);
		writel(BIT_NPU_SYS_DATA_BUS_IDLE_EN_CLR, npu_sys_glb_base + NPU_SYS_LPC_DATA_SETTING_CLR_ADDR);
		writel(BIT_NPU_SYS_DATA_IDLE_EB_CLR, npu_sys_glb_base + NPU_SYS_LPC_SETTING_CLR_ADDR);
		writel(BIT_NPU_SYS_SLP_EN_CLR, npu_sys_glb_base + NPU_SYS_LPC_SETTING_CLR_ADDR);

		ret = pmu_get_module_state(pmu_glb_base, MODULE_NPU, &pwr_state);

		pr_info("npu sys wakeup state is 0x%x\r\n", pwr_state);

		writel(npu_sys_clk_mux_0_reserve, npu_sys_glb_base + NPU_SYS_CLK_MUX_0_ADDR);
		writel(npu_sys_clk_eb_0_reserve, npu_sys_glb_base + NPU_SYS_CLK_EB_0_ADDR);
		writel(npu_sys_clk_eb_1_reserve, npu_sys_glb_base + NPU_SYS_CLK_EB_1_ADDR);
		break;
	default:
		pr_err("%s param input is invalid:%d", __func__, state);
		break;
	}
}

static void vpu_sys_sleep(slp_state_t state)
{
	static unsigned int vpu_sys_clk_mux_0_reserve = 0;
	static unsigned int vpu_sys_clk_eb_0_reserve = 0;
	static unsigned int vpu_sys_clk_eb_1_reserve = 0;
	int ret;
	unsigned int pwr_state;

	switch (state) {
	case SLP_ENTER:
		vpu_sys_clk_mux_0_reserve = readl(vpu_sys_glb_base + VPU_SYS_CLK_MUX_0_ADDR);
		vpu_sys_clk_eb_0_reserve = readl(vpu_sys_glb_base + VPU_SYS_CLK_EB_0_ADDR);
		vpu_sys_clk_eb_1_reserve = readl(vpu_sys_glb_base + VPU_SYS_CLK_EB_1_ADDR);

		/* lpc_cfg_bus_idle_en & lpc_data_bus_idle_en */
		writel(BIT_VPU_LPC_CFG_BUS_IDLE_EN_SET, vpu_sys_glb_base + VPU_SYS_VENC1_LPC1_SET_ADDR);
		writel(BIT_VPU_LPC_DATA_BUS_IDLE_EN_SET, vpu_sys_glb_base + VPU_SYS_VENC1_LPC2_SET_ADDR);
		writel(BIT_VPU_DATA_IDLE_EN_SET, vpu_sys_glb_base + VPU_SYS_VENC1_LPC0_SET_ADDR);
		writel(BIT_VPU_SYS_LPC_SW_RST_CLR, vpu_sys_glb_base + VPU_SYS_SW_RST_0_CLR_ADDR);

		/* clear clk_eb */
		writel(BITS_VPU_SYS_CLK_EB_1, vpu_sys_glb_base + VPU_SYS_CLK_EB_1_ADDR);
		writel(BITS_VPU_SYS_CLK_EB_0, vpu_sys_glb_base + VPU_SYS_CLK_EB_0_ADDR);

		writel(BIT_VPU_LPC_SLP_EN_SET, vpu_sys_glb_base + VPU_SYS_VENC1_LPC0_SET_ADDR);
		pmu_module_sleep_en(pmu_glb_base, MODULE_VPU, SLP_EN_SET);

		/* check sys power state till off */
		while (!(ret = pmu_get_module_state(pmu_glb_base, MODULE_VPU, &pwr_state)) && (pwr_state != PWR_STATE_OFF));

		pr_info("vpu sys sleep state is 0x%x\r\n", pwr_state);
		break;
	case SLP_EXIT:
		pmu_module_sleep_en(pmu_glb_base, MODULE_VPU, SLP_EN_CLR);
		ret = pmu_module_wakeup(pmu_glb_base, MODULE_VPU);

		writel(BIT_VPU_LPC_SLP_EN_CLR, vpu_sys_glb_base + VPU_SYS_VENC1_LPC0_CLR_ADDR);
		writel(BIT_VPU_LPC_CFG_BUS_IDLE_EN_CLR, vpu_sys_glb_base + VPU_SYS_VENC1_LPC1_CLR_ADDR);
		writel(BIT_VPU_DATA_IDLE_EN_CLR, vpu_sys_glb_base + VPU_SYS_VENC1_LPC0_CLR_ADDR);
		writel(BIT_VPU_LPC_DATA_BUS_IDLE_EN_CLR, vpu_sys_glb_base + VPU_SYS_VENC1_LPC2_CLR_ADDR);

		ret = pmu_get_module_state(pmu_glb_base, MODULE_VPU, &pwr_state);

		pr_info("vpu sys wakeup state is 0x%x\r\n", pwr_state);

		writel(vpu_sys_clk_mux_0_reserve, vpu_sys_glb_base + VPU_SYS_CLK_MUX_0_ADDR);
		writel(vpu_sys_clk_eb_0_reserve, vpu_sys_glb_base + VPU_SYS_CLK_EB_0_ADDR);
		writel(vpu_sys_clk_eb_1_reserve, vpu_sys_glb_base + VPU_SYS_CLK_EB_1_ADDR);
		break;
	default:
		pr_err("%s param input is invalid:%d", __func__, state);
		break;
	}
}

static void peri_sys_sleep(slp_state_t state)
{
	static unsigned int peri_sys_clk_mux_0_reserve = 0;
	static unsigned int peri_sys_clk_eb_0_reserve = 0;
	static unsigned int peri_sys_clk_eb_1_reserve = 0;
	static unsigned int peri_sys_clk_eb_2_reserve = 0;
	static unsigned int peri_sys_clk_eb_3_reserve = 0;
	static unsigned int peri_sys_clk_div_0_reserve = 0;
	int ret;
	unsigned int pwr_state;
	unsigned int val;

	switch (state) {
	case SLP_ENTER:

		/* in order to enable wdt2 during deep sleep, we set periph sys clk frc en here. */
		val = readl(pmu_glb_base + PMU_GLB_CLK_FRC_EN_ADDR);
		val |= BIT_PMU_GLB_PERIPH_SYS_CLK_FRC_EN;
		writel(val, pmu_glb_base + PMU_GLB_CLK_FRC_EN_ADDR);

		/* in order to enable wdt2 during deep sleep, we set periph sys clk frc sw here. */
		val = readl(pmu_glb_base + PMU_GLB_CLK_FRC_SW_ADDR);
		val |= BIT_PMU_GLB_PERIPH_SYS_CLK_FRC_EN;
		writel(val, pmu_glb_base + PMU_GLB_CLK_FRC_SW_ADDR);

		peri_sys_clk_mux_0_reserve = readl(periph_sys_glb_base + PERIPH_SYS_CLK_MUX_0_ADDR);
		peri_sys_clk_eb_0_reserve = readl(periph_sys_glb_base + PERIPH_SYS_CLK_EB_0_ADDR);
		peri_sys_clk_eb_1_reserve = readl(periph_sys_glb_base + PERIPH_SYS_CLK_EB_1_ADDR);
		peri_sys_clk_eb_2_reserve = readl(periph_sys_glb_base + PERIPH_SYS_CLK_EB_2_ADDR);
		peri_sys_clk_eb_3_reserve = readl(periph_sys_glb_base + PERIPH_SYS_CLK_EB_3_ADDR);
		peri_sys_clk_div_0_reserve = readl(periph_sys_glb_base + PERIPH_SYS_CLK_DIV_0_ADDR);

		/* in order to enable wdt2 during deep sleep, set lpc bypass here */
		writel(BIT_PERIPH_SYS_LPC_BP_CLK_EB_SET, periph_sys_glb_base + PERIPH_SYS_LPC_SET_ADDR);

		writel(BIT_PERIPH_SYS_LPC_DATA_BUS_FRC_WORK_CLR ,periph_sys_glb_base + PERIPH_SYS_LPC_DATA_CLR_ADDR);

		/* lpc_cfg_bus_idle_en & lpc_data_bus_idle_en */
		writel(BIT_PERIPH_SYS_LPC_CFG_BUS_IDLE_EN_SET, periph_sys_glb_base + PERIPH_SYS_LPC_CFG_SET_ADDR);
		writel(BIT_PERIPH_SYS_LPC_DATA_BUS_IDLE_EN_SET, periph_sys_glb_base + PERIPH_SYS_LPC_DATA_SET_ADDR);
		writel(BIT_PERIPH_SYS_LPC_DATA_IDLE_EN_SET, periph_sys_glb_base + PERIPH_SYS_LPC_SET_ADDR);
		writel(BIT_PERIPH_SYS_LPC_SW_RST_CLR, periph_sys_glb_base + PERIPH_SYS_SW_RST_1_CLR_ADDR);

		/* dont't clr clk_periph_24m and clk_wdt2_eb */
		writel(BITS_PERIPH_SYS_CLK_EB_0, periph_sys_glb_base + PERIPH_SYS_CLK_EB_0_CLR_ADDR);
		writel(BITS_PERIPH_SYS_CLK_EB_1, periph_sys_glb_base + PERIPH_SYS_CLK_EB_1_ADDR);
		writel(BITS_PERIPH_SYS_CLK_EB_2, periph_sys_glb_base + PERIPH_SYS_CLK_EB_2_ADDR);
		writel(BITS_PERIPH_SYS_CLK_EB_3, periph_sys_glb_base + PERIPH_SYS_CLK_EB_3_ADDR);

		writel(BIT_PERIPH_SYS_LPC_SLP_EN_SET, periph_sys_glb_base + PERIPH_SYS_LPC_SET_ADDR);

		pmu_module_sleep_en(pmu_glb_base, MODULE_PERIPH, SLP_EN_SET);

		/* check sys power state till off */
		while (!(ret = pmu_get_module_state(pmu_glb_base, MODULE_PERIPH, &pwr_state)) && (pwr_state != PWR_STATE_OFF));
		break;
	case SLP_EXIT:
		pmu_module_sleep_en(pmu_glb_base, MODULE_PERIPH, SLP_EN_CLR);
		ret = pmu_module_wakeup(pmu_glb_base, MODULE_PERIPH);

		writel(BIT_PERIPH_SYS_LPC_SLP_EN_CLR, periph_sys_glb_base + PERIPH_SYS_LPC_CLR_ADDR);
		writel(BIT_PERIPH_SYS_LPC_CFG_BUS_IDLE_EN_CLR, periph_sys_glb_base + PERIPH_SYS_LPC_CFG_CLR_ADDR);
		writel(BIT_PERIPH_SYS_LPC_DATA_BUS_IDLE_EN_CLR, periph_sys_glb_base + PERIPH_SYS_LPC_DATA_CLR_ADDR);
		writel(BIT_PERIPH_SYS_LPC_DATA_IDLE_EN_CLR, periph_sys_glb_base + PERIPH_SYS_LPC_CLR_ADDR);
		writel(BIT_PERIPH_SYS_LPC_DATA_BUS_FRC_WORK_SET, periph_sys_glb_base + PERIPH_SYS_LPC_DATA_SET_ADDR);

		writel(peri_sys_clk_mux_0_reserve, periph_sys_glb_base + PERIPH_SYS_CLK_MUX_0_ADDR);
		writel(peri_sys_clk_eb_0_reserve, periph_sys_glb_base + PERIPH_SYS_CLK_EB_0_ADDR);
		writel(peri_sys_clk_eb_1_reserve, periph_sys_glb_base + PERIPH_SYS_CLK_EB_1_ADDR);
		writel(peri_sys_clk_eb_2_reserve, periph_sys_glb_base + PERIPH_SYS_CLK_EB_2_ADDR);
		writel(peri_sys_clk_eb_3_reserve, periph_sys_glb_base + PERIPH_SYS_CLK_EB_3_ADDR);
		writel(peri_sys_clk_div_0_reserve, periph_sys_glb_base + PERIPH_SYS_CLK_DIV_0_ADDR);

		ret = pmu_get_module_state(pmu_glb_base, MODULE_PERIPH, &pwr_state);
		break;
	default:
		pr_err("%s param input is invalid:%d", __func__, state);
		break;
	}
}

static void copy_again(void);

static void other_sys_sleep(void)
{
	unsigned int val;

	val = readl(ddrc_base + DDRC_DDRMC_PORT0_CFG0_ADDR);
	val |= BIT_DDRC_RF_AUTO_SLP_EN_PORT0;
	writel(val, ddrc_base + DDRC_DDRMC_PORT0_CFG0_ADDR);

	val = readl(ddrc_base + DDRC_DDRMC_PORT1_CFG0_ADDR);
	val |= BIT_DDRC_RF_AUTO_SLP_EN_PORT1;
	writel(val, ddrc_base + DDRC_DDRMC_PORT1_CFG0_ADDR);

	val = readl(ddrc_base + DDRC_DDRMC_PORT2_CFG0_ADDR);
	val |= BIT_DDRC_RF_AUTO_SLP_EN_PORT2;
	writel(val, ddrc_base + DDRC_DDRMC_PORT2_CFG0_ADDR);

	val = readl(ddrc_base + DDRC_DDRMC_PORT3_CFG0_ADDR);
	val |= BIT_DDRC_RF_AUTO_SLP_EN_PORT3;
	writel(val, ddrc_base + DDRC_DDRMC_PORT3_CFG0_ADDR);

	val = readl(ddrc_base + DDRC_DDRMC_PORT4_CFG0_ADDR);
	val |= BIT_DDRC_RF_AUTO_SLP_EN_PORT4;
	writel(val, ddrc_base + DDRC_DDRMC_PORT4_CFG0_ADDR);

	val = readl(ddrc_base + DDRC_DDRMC_CFG22_ADDR);
	val |= BIT_DDRC_RF_AUTO_SLP_EN;
	writel(val, ddrc_base + DDRC_DDRMC_CFG22_ADDR);

	writel(AX_KERNEL_SLEEP_STAGE_00, sleep_stage_addr);
	flash_sys_sleep(SLP_ENTER);

	writel(AX_KERNEL_SLEEP_STAGE_01, sleep_stage_addr);
	isp_sys_sleep(SLP_ENTER);

	writel(AX_KERNEL_SLEEP_STAGE_02, sleep_stage_addr);
	mm_sys_sleep(SLP_ENTER);

	writel(AX_KERNEL_SLEEP_STAGE_03, sleep_stage_addr);
	npu_sys_sleep(SLP_ENTER);

	writel(AX_KERNEL_SLEEP_STAGE_04, sleep_stage_addr);
	vpu_sys_sleep(SLP_ENTER);

	writel(AX_KERNEL_SLEEP_STAGE_05, sleep_stage_addr);
	ax_sys_sleeptimestamp(AX_ID_KERNEL, AX_SUB_ID_SUSPEND_END);
	ax_sys_sleeptimestamp_print();
	peri_sys_sleep(SLP_ENTER);
	writel(AX_KERNEL_SLEEP_STAGE_06, sleep_stage_addr);

	/* check flash isp mm npu vpu peri status */
	while(readl(pmu_glb_base + PMU_GLB_PWR_STATE_ADDR) != 0x55555500);
	copy_again();
	writel(AX_KERNEL_SLEEP_STAGE_07, sleep_stage_addr);
}

static void other_sys_wakeup(void)
{
	ax_sys_sleeptimestamp(AX_ID_KERNEL, AX_SUB_ID_RESUME_START);
	writel(AX_KERNEL_WAKEUP_STAGE_15, sleep_stage_addr);
	peri_sys_sleep(SLP_EXIT);
	writel(AX_KERNEL_WAKEUP_STAGE_16, sleep_stage_addr);
	flash_sys_sleep(SLP_EXIT);
	writel(AX_KERNEL_WAKEUP_STAGE_17, sleep_stage_addr);
	npu_sys_sleep(SLP_EXIT);
	writel(AX_KERNEL_WAKEUP_STAGE_18, sleep_stage_addr);
	isp_sys_sleep(SLP_EXIT);
	writel(AX_KERNEL_WAKEUP_STAGE_19, sleep_stage_addr);
	mm_sys_sleep(SLP_EXIT);
	writel(AX_KERNEL_WAKEUP_STAGE_1A, sleep_stage_addr);
	vpu_sys_sleep(SLP_EXIT);
	writel(AX_KERNEL_WAKEUP_STAGE_1B, sleep_stage_addr);
}

static void clk_aux_disabled(void)
{
	writel(0x10, comm_sys_glb_base + COMMON_SYS_AUX_CFG1_CLR_ADDR);
}

static void clk_aux_enabled(void)
{
	writel(0x10, comm_sys_glb_base + COMMON_SYS_AUX_CFG1_SET_ADDR);
}

int ax620e_suspend_enter(suspend_state_t state)
{
	int ret;

	/* eic riscv en clr */
	writel(0x0, comm_sys_glb_base + EIC_RISCV_EN);

	writel(BIT_COMMON_SYS_EIC_MASK_ENABLE_SET, comm_sys_glb_base + COMMON_SYS_EIC_MASK_ENABLE_SET_ADDR);

	if (IS_ENABLED(CONFIG_CLK_AUX_SUSPEND_DISABLE))
		clk_aux_disabled();

	other_sys_sleep();

	ret = cpu_suspend(0, ax620e_chip_lpmode_enter);
	if (ret)
		pr_warn("ax620e sleep occurs some unexpected\n");

	other_sys_wakeup();

	if (IS_ENABLED(CONFIG_CLK_AUX_SUSPEND_DISABLE))
		clk_aux_enabled();

	return 0;
}

int ax620e_suspend_prepare(void)
{
	return 0;
}

void ax620e_suspend_finish(void)
{

}


#ifdef CONFIG_ARM_LPAE
static void idmap_add_pmd(pud_t *pud, unsigned long addr, unsigned long end,
	unsigned long prot)
{
	pmd_t *pmd;
	unsigned long next;

	if (pud_none_or_clear_bad(pud) || (pud_val(*pud) & L_PGD_SWAPPER)) {
		pmd = pmd_alloc_one(&init_mm, addr);
		if (!pmd) {
			pr_warn("Failed to allocate identity pmd.\n");
			return;
		}
		/*
		 * Copy the original PMD to ensure that the PMD entries for
		 * the kernel image are preserved.
		 */
		if (!pud_none(*pud))
			memcpy(pmd, pmd_offset(pud, 0),
			       PTRS_PER_PMD * sizeof(pmd_t));
		pud_populate(&init_mm, pud, pmd);
		pmd += pmd_index(addr);
	} else
		pmd = pmd_offset(pud, addr);

	do {
		next = pmd_addr_end(addr, end);
		*pmd = __pmd((addr & PMD_MASK) | prot);
		flush_pmd_entry(pmd);
	} while (pmd++, addr = next, addr != end);
}
#else	/* !CONFIG_ARM_LPAE */
static void idmap_add_pmd(pud_t *pud, unsigned long addr, unsigned long end,
	unsigned long prot)
{
	pmd_t *pmd = pmd_offset(pud, addr);

	addr = (addr & PMD_MASK) | prot;
	pmd[0] = __pmd(addr);
	addr += SECTION_SIZE;
	pmd[1] = __pmd(addr);
	flush_pmd_entry(pmd);
}
#endif	/* CONFIG_ARM_LPAE */

static void idmap_add_pud(pgd_t *pgd, unsigned long addr, unsigned long end,
	unsigned long prot)
{
	pud_t *pud = pud_offset(pgd, addr);
	unsigned long next;

	do {
		next = pud_addr_end(addr, end);
		idmap_add_pmd(pud, addr, next, prot);
	} while (pud++, addr = next, addr != end);
}

static void identity_mapping_add(pgd_t *pgd, unsigned long text_start,
				 unsigned long text_end, unsigned long prot)
{
	unsigned long addr = text_start;
	unsigned long end = text_end;
	unsigned long next;

	pr_info("ax620e pm setting up static identity map for 0x%lx - 0x%lx\n", addr, end);

	prot |= PMD_TYPE_SECT | PMD_SECT_AP_WRITE | PMD_SECT_AF;

	pgd += pgd_index(addr);
	do {
		next = pgd_addr_end(addr, end);
		idmap_add_pud(pgd, addr, next, prot);
	} while (pgd++, addr = next, addr != end);
}

static int ax620e_pm_setup_idmap(unsigned long start, unsigned long end)
{
	pm_idmap_pgd = pgd_alloc(&init_mm);
	if (!pm_idmap_pgd)
		return -ENOMEM;

	identity_mapping_add(pm_idmap_pgd, start, end, 0);

	/* Flush L1 for the hardware to see this page table content */
	flush_cache_louis();

	return 0;
}

static inline void timer32_wakeup_config(unsigned int wait_count)
{
	unsigned long long val = 0;
	unsigned long long delta_stamp = 0;
	unsigned int delta = 0;
	unsigned long long wakeup_start = 0;
	unsigned long long sleep_end = 0;


	if (__raw_readl((void *)TIMER_EB_ADDR) == 0)
		return;

	val = __raw_readl((void *)TIMER_COUNT_ADDR);
	if (val != 0) {
		val *= 1000;
		val *= TIMER32_COUNT_PER_SEC;	//wait_count = ms * 1000 * 32K / 1000000.
		do_div(val, 1000000);
		wait_count = val;
	}

flag1:
	__raw_writel(0, (void *)TIMER32_BASE_PHY_ADDR + TIMER32_CMR_START);//stop compare

	val = __raw_readl((void *)TIMER32_BASE_PHY_ADDR + TIMER32_INTR_CTRL);
	val |= BIT_TIMER32_INTR_EOI;// set int eoi
	val |= BIT_TIMER32_INTR_MASK;// set int mask
	val &= ~BIT_TIMER32_INTR_EN; //disable int
	__raw_writel(val, (void *)TIMER32_BASE_PHY_ADDR + TIMER32_INTR_CTRL);

	/* need clk */
	__raw_writel(1, (void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_XTAL_SLEEP_BYP_ADDR);
	//__raw_writel(PINMUX_G6_CTRL_CLR_CLR, (void *)PINMUX_G6_BASE_PHY_ADDR + PINMUX_G6_MISC_CLR_ADDR);

	/* eic enable timer32 for wakeup */
	val = __raw_readl((void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_EIC_EN_SET_ADDR);
	val |= BIT_COMMON_TMR32_EIC_EN_SET;
	__raw_writel(val, (void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_EIC_EN_SET_ADDR);

	/*prst and rst */
	val = __raw_readl((void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_SW_RST_0_ADDR);
	val |= (BIT_COMMON_SYS_TIMER32_SW_RST | BIT_COMMON_SYS_TIMER32_SW_PRST);
	__raw_writel(val, (void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_SW_RST_0_ADDR);

	val = __raw_readl((void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_SW_RST_0_ADDR);
	val &= ~(BIT_COMMON_SYS_TIMER32_SW_RST | BIT_COMMON_SYS_TIMER32_SW_PRST);
	__raw_writel(val, (void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_SW_RST_0_ADDR);

	/* select 32K */
	val = __raw_readl((void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_CLK_MUX1_ADDR);
	val &= ~BIT_COMMON_SYS_CLK_TIMER32_SEL_24M;
	__raw_writel(val, (void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_CLK_MUX1_ADDR);

	/*clk channel enable */
	val = __raw_readl((void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_CLK_EB_0_ADDR);
	val |= BIT_COMMON_SYS_CLK_TIMER32_EB;
	__raw_writel(val, (void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_CLK_EB_0_ADDR);

	/* global clk enable */
	val = __raw_readl((void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_CLK_EB_1_ADDR);
	val |= BIT_COMMON_SYS_PCLK_TMR32_EB;
	__raw_writel(val, (void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_CLK_EB_1_ADDR);

	/* sleep end timer64 value */
	sleep_end = *((volatile unsigned long long*)TIMER64_CNT_LOW_ADDR);

	wakeup_start = *((volatile unsigned long long*)WAKEUP_START_TIMESTAMP_ADDR);

	delta_stamp = sleep_end - wakeup_start;
	if (delta_stamp > 0) {
		do_div(delta_stamp, 24);
		delta_stamp *= TIMER32_COUNT_PER_SEC;
		do_div(delta_stamp, 1000000);
		delta = delta_stamp;
		if (wait_count > delta)
			wait_count = wait_count - delta;
	}

	/* when the timer32 wait count < 10ms, we directly set it to 10ms in case of the timer32 interrupt block sleep. */
	if (wait_count < TIMER32_COUNT_PER_SEC / 100)
		wait_count = TIMER32_COUNT_PER_SEC / 100;

	val = __raw_readl((void *)TIMER32_BASE_PHY_ADDR + TIMER32_CNT_CCVR);
	val += wait_count;
	if (val >= 0xfffffff0) {
		goto flag1;
	} else {
		__raw_writel(val, (void *)TIMER32_BASE_PHY_ADDR + TIMER32_CMR);
	}

	//compare start
	__raw_writel(BIT_TIMER32_CMR_START, (void *)TIMER32_BASE_PHY_ADDR + TIMER32_CMR_START);

	//set int_en
	__raw_writel(BIT_TIMER32_INTR_EN, (void *)TIMER32_BASE_PHY_ADDR + TIMER32_INTR_CTRL);
}

static inline void reg_mask_set(void *addr, unsigned int mask_data, unsigned int bits_set)
{
	unsigned int reg_data;
	reg_data = __raw_readl(addr);
	reg_data &= ~(mask_data);
	reg_data |= bits_set;
	__raw_writel(reg_data, addr);
}

static void noinline func_2_iram ddr_sys_sleep_in_iram(slp_state_t state)
{
	unsigned int val = 0;
	unsigned int pwr_state = 0;
	unsigned char type;
	type = __raw_readl((void *)IRAM_CHIP_TYPE_ADDR);
	switch (state) {
	case SLP_ENTER:
		__raw_writel(AX_KERNEL_SLEEP_STAGE_08, (void*)SLEEP_STAGE_STORE_ADDR);
		if(IRAM_CHIP_TYPE_Q == type) {
			reg_mask_set((void *)D_DDRPHYB_AC_CFG7_ADDR, 0, D_DDRIOMUX_CKE_OE | D_DDRIOMUX_CKE_OUT);
			reg_mask_set((void *)D_DDRPHYB_AC_CFG6_ADDR, D_DDRIOMUX_CKE_IE, D_DDRIOMUX_CKE_SEL);
			reg_mask_set((void *)D_DDRPHYB_AC_CFG9_ADDR, 0, D_DDRIOMUX_CLK_OE | D_DDRIOMUX_CLK_OUT);
			reg_mask_set((void *)D_DDRPHYB_AC_CFG8_ADDR, D_DDRIOMUX_CLK_IE, D_DDRIOMUX_CLK_SEL);
			reg_mask_set((void *)D_DDRMC_TMG18_F0_ADDR, D_DDRMC_CA_CS_CLK_OE, 0);
			reg_mask_set((void *)D_DDRMC_TMG18_F1_ADDR, D_DDRMC_CA_CS_CLK_OE, 0);
			reg_mask_set((void *)D_DDRMC_CFG18_ADDR, D_DDRMC_AUTO_GATE_EN_PHY, 0);
		}
		__raw_writel(BIT_DDR_SYS_AXICLK_OFF_HW_EN_SET, (void *)DDR_SYS_GLB_BASE_PHY_ADDR + DDR_SYS_AXICLK_OFF_SET_ADDR);
		__raw_writel(BIT_DDR_SYS_CORECLK_OFF_HW_EN_SET, (void *)DDR_SYS_GLB_BASE_PHY_ADDR + DDR_SYS_CORECLK_OFF_SET_ADDR);

		//ddr sys go into light sleep
		__raw_writel(BIT_COMMON_SYS_COMMON2_DDR_BUS_IDLE_SW, (void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_COMMON2_DDR_BUS_IDLE_SW_ADDR);
		__raw_writel(BIT_COOMON_SYS_COMMON2_DDR_BUS_IDLE_MASK, (void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_COMMON2_DDR_BUS_IDLE_MASK_ADDR);
		__raw_writel(BIT_DDR_SYS_SLP_IGNORE_CPU_EN_SET, (void *)DDR_SYS_GLB_BASE_PHY_ADDR + DDR_SYS_SLP_IGNORE_CPU_SET_ADDR);
		__raw_writel(1, (void *)PMU_GLB_BASE_PHY_ADDR + PMU_GLB_LIGHT_SLEEP_FRC_ADDR);
		__raw_writel(1, (void *)PMU_GLB_BASE_PHY_ADDR + PMU_GLB_LIGHT_SLEEP_SW_ADDR);

		//wait ddr sys go into light sleep
		while((__raw_readl((void *)DDR_SYS_GLB_BASE_PHY_ADDR + DDR_SYS_LP_DDRC_ADDR) & BITS_DDR_SYS_LP_DDRC_STATE) != DDR_SYS_LP_DDRC_ENTER_LIGHT_SLP);
		__raw_writel(AX_KERNEL_SLEEP_STAGE_09, (void*)SLEEP_STAGE_STORE_ADDR);

		if(IRAM_CHIP_TYPE_Q == type) {
			reg_mask_set((void *)D_DDRPHYB_AC_CFG7_ADDR, D_DDRIOMUX_CKE_OUT, 0);
			reg_mask_set((void *)D_DDRPHYB_AC_CFG9_ADDR, D_DDRIOMUX_CLK_OUT, 0);
		}
		__raw_writel(BIT_DDR_SYS_LPC_DATA_BUS_FORCE_IDLE_SET_LSB, (void *)DDR_SYS_GLB_BASE_PHY_ADDR + DDR_SYS_LPC_DATA_SET_ADDR);

		/* enable pmu ddr and loop state till off */
		__raw_writel(1 << MODULE_DDR, (void *)PMU_GLB_BASE_PHY_ADDR + PMU_GLB_SLP_EN_SET_ADDR);
		while(1) {
			val = __raw_readl((void *)PMU_GLB_BASE_PHY_ADDR + PMU_GLB_PWR_STATE_ADDR);
			pwr_state = (val >> (MODULE_DDR * SYS_PWR_STATE_LEN)) & SYS_PWR_STATE_MASK;
			if (pwr_state == PWR_STATE_OFF)
				break;
		}
		__raw_writel(AX_KERNEL_SLEEP_STAGE_0A, (void*)SLEEP_STAGE_STORE_ADDR);
		break;
	case SLP_EXIT:
		__raw_writel(AX_KERNEL_WAKEUP_STAGE_10, (void*)SLEEP_STAGE_STORE_ADDR);
		/* wakeup pmu ddr sys loop state till on */
		__raw_writel(1 << MODULE_DDR, (void *)PMU_GLB_BASE_PHY_ADDR + PMU_GLB_SLP_EN_CLR_ADDR);
		__raw_writel(1 << MODULE_DDR, (void *)PMU_GLB_BASE_PHY_ADDR + PMU_GLB_WAKUP_SET_ADDR);
		while(1) {
			val = __raw_readl((void *)PMU_GLB_BASE_PHY_ADDR + PMU_GLB_PWR_STATE_ADDR);
			pwr_state = (val >> (MODULE_DDR * SYS_PWR_STATE_LEN)) & SYS_PWR_STATE_MASK;
			if (pwr_state == PWR_STATE_ON)
				break;
		}
		__raw_writel(AX_KERNEL_WAKEUP_STAGE_11, (void*)SLEEP_STAGE_STORE_ADDR);

		__raw_writel(BIT_DDR_SYS_LPC_DATA_BUS_FORCE_IDLE_CLR_LSB, (void *)DDR_SYS_GLB_BASE_PHY_ADDR + DDR_SYS_LPC_DATA_CLR_ADDR);
		__raw_writel(1 << MODULE_DDR, (void *)PMU_GLB_BASE_PHY_ADDR + PMU_GLB_WAKUP_CLR_ADDR);

		//cpu will close dpll. TODO and wait 1ms
		while((__raw_readl((void *)DPLL_BASE_PHY_ADDR + DDR_SYS_PLL_RDY_STS_ADDR) & BIT_DDR_STS_DPLL_RSY) != 0x1);
		__raw_writel(AX_KERNEL_WAKEUP_STAGE_12, (void*)SLEEP_STAGE_STORE_ADDR);

		//ddr sys exit light sleep
		if(IRAM_CHIP_TYPE_Q == type) {
			reg_mask_set((void *)D_DDRPHYB_AC_CFG9_ADDR, 0, D_DDRIOMUX_CLK_OUT);
			reg_mask_set((void *)D_DDRPHYB_AC_CFG7_ADDR, 0, D_DDRIOMUX_CKE_OUT);
			reg_mask_set((void *)D_DDRMC_CFG18_ADDR, 0, D_DDRMC_AUTO_GATE_EN_PHY);
		}
		__raw_writel(0, (void *)PMU_GLB_BASE_PHY_ADDR + PMU_GLB_LIGHT_SLEEP_SW_ADDR);
		while((__raw_readl((void *)DDR_SYS_GLB_BASE_PHY_ADDR + DDR_SYS_LP_DDRC_ADDR) & 0xf) != DDR_SYS_LP_DDRC_EXIT_LIGHT_SLP);
		__raw_writel(AX_KERNEL_WAKEUP_STAGE_13, (void*)SLEEP_STAGE_STORE_ADDR);

		__raw_writel(BIT_DDR_SYS_AXICLK_OFF_HW_EN_CLR, (void *)DDR_SYS_GLB_BASE_PHY_ADDR + DDR_SYS_AXICLK_OFF_CLR_ADDR);
		__raw_writel(BIT_DDR_SYS_CORECLK_OFF_HW_EN_CLR, (void *)DDR_SYS_GLB_BASE_PHY_ADDR + DDR_SYS_CORECLK_OFF_CLR_ADDR);

		__raw_writel(0, (void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_COMMON2_DDR_BUS_IDLE_SW_ADDR);
		__raw_writel(0, (void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_COMMON2_DDR_BUS_IDLE_MASK_ADDR);
		__raw_writel(AX_KERNEL_WAKEUP_STAGE_14, (void*)SLEEP_STAGE_STORE_ADDR);
		break;
	default:

		break;
	}
}
static void noinline ddr_sys_sleep_in_iram_end(void) {}

static void noinline func_2_iram cpu_sys_sleep_in_iram(slp_state_t state)
{
	unsigned int cpu_clk_mux_0_reserve;
	unsigned int cpu_clk_eb_0_reserve;
	unsigned int cpu_clk_eb_1_reserve;
	unsigned int cpu_clk_div_0_reserve;
	unsigned int common_clk_mux_0_reserve;
	unsigned int common_clk_mux_2_reserve;
	unsigned int val;
	unsigned long long timer64_val = 0;

	switch (state) {
	case SLP_ENTER:
		__raw_writel(AX_KERNEL_SLEEP_STAGE_0B, (void*)SLEEP_STAGE_STORE_ADDR);
		/* backup cpu clk regs to iram backup address */
		cpu_clk_mux_0_reserve = __raw_readl((void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_CLK_MUX_0_ADDR);
		cpu_clk_eb_0_reserve = __raw_readl((void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_CLK_EB_0_ADDR);
		cpu_clk_eb_1_reserve = __raw_readl((void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_CLK_EB_1_ADDR);
		cpu_clk_div_0_reserve = __raw_readl((void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_CLK_DIV_0_ADDR);
		common_clk_mux_0_reserve = __raw_readl((void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_CLK_MUX_0_ADDR);
		common_clk_mux_2_reserve = __raw_readl((void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_CLK_MUX_2_ADDR);

		__raw_writel(cpu_clk_mux_0_reserve, (void *)CPU_CLK_MUX_0_BAK_ADDR);
		__raw_writel(cpu_clk_eb_0_reserve, (void *)CPU_CLK_EB_0_BAK_ADDR);
		__raw_writel(cpu_clk_eb_1_reserve, (void *)CPU_CLK_EB_1_BAK_ADDR);
		__raw_writel(cpu_clk_div_0_reserve, (void *)CPU_CLK_DIV_0_BAK_ADDR);
		__raw_writel(common_clk_mux_0_reserve, (void *)COMMON_CLK_MUX_0_BAK_ADDR);
		__raw_writel(common_clk_mux_2_reserve, (void *)COMMON_CLK_MUX_2_BAK_ADDR);

		if (__raw_readl((void*)PLL_ALWAYS_ON_ADDR) == 0x0) {
			/* set cpu clk to cpll_208M */
			val = cpu_clk_mux_0_reserve;
			val &= ~(0x7 << 2);
			val |= (0x1 << 2);
			__raw_writel(val, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_CLK_MUX_0_ADDR);

			/* set aclk cpu top to cpll_208M */
			val = __raw_readl((void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_CLK_MUX_0_ADDR);
			val &= ~(0x7 << 24);
			val |= (0x1 << 24);
			__raw_writel(val, (void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_CLK_MUX_0_ADDR);

			/* set pclk top to cpll_208M */
			__raw_writel((0x3 << 0), (void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_CLK_MUX_2_ADDR);
		}

		__raw_writel(BIT_CPU_SYS_CA53_CLUSTER_INT_DISABLE_SET | BIT_CPU_SYS_CA53_CPU_INT_DISABLE_SET, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_INT_MSK_SET_ADDR);
		__raw_writel(BIT_CPU_SYS_PCLK_CPU_DBGMNR_EB_CLR, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_CLK_EB_1_CLR_ADDR);
		__raw_writel(BIT_CPU_SYS_ACLK_CPU_DBGMNR_EB_CLR, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_CLK_EB_1_CLR_ADDR);
		__raw_writel(BIT_CPU_SYS_PCLK_CPU_PERFMNR_EB_CLR, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_CLK_EB_1_CLR_ADDR);
		__raw_writel(BIT_CPU_SYS_ACLK_CPU_PERFMNR_EB_CLR, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_CLK_EB_1_CLR_ADDR);
		__raw_writel(BIT_CPU_SYS_CLK_PERFMNR_24M_EB_CLR, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_CLK_EB_1_CLR_ADDR);
		__raw_writel(BIT_CPU_SYS_HCLK_SPI_EB_CLR, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_CLK_EB_1_CLR_ADDR);
		__raw_writel(BIT_CPU_SYS_CLK_EMMC_EB_CLR, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_CLK_EB_1_CLR_ADDR);
		__raw_writel(BIT_CPU_SYS_CLK_ROSC_EB_CLR, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_CLK_EB_1_CLR_ADDR);
		__raw_writel(BIT_CPU_SYS_CLK_BROM_EB_CLR, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_CLK_EB_0_CLR_ADDR);
		__raw_writel(BIT_CPU_SYS_CLK_EMMC_CARD_EB_CLR, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_CLK_EB_0_CLR_ADDR);
		__raw_writel(BIT_CPU_SYS_CLK_H_SSI_EB_CLR, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_CLK_EB_0_CLR_ADDR);

		__raw_writel(BIT_CPU_SYS_DATA_IDLE_EN_SET, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_LPC_CFG_2_SET_ADDR);
		__raw_writel(BIT_CPU_SYS_DATA_BUS_IDLE_EN_SET, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_LPC_CFG_1_SET_ADDR);
		__raw_writel(BIT_CPU_SYS_DATA_NOC_TIMEOUT_EN_SET, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_LPC_CFG_1_SET_ADDR);

		__raw_writel(BIT_CPU_SYS_CFG_BUS_IDLE_EN_SET, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_LPC_CFG_0_SET_ADDR);
		__raw_writel(BIT_CPU_SYS_CFG_NOC_TIMEOUT_EN_SET, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_LPC_CFG_0_SET_ADDR);

		/* clear all interrupt */
		__raw_writel(BITS_CPU_ERR_INT_CLEAR_SET, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_RSP2INT_CTRL_SET_ADDR);

		__raw_writel(BIT_CPU_SYS_SLP_EN_SET, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_LPC_CFG_2_SET_ADDR);

		/* cpu slp en in pmu and cpu reset config */
		__raw_writel(1 << MODULE_CPU, (void *)PMU_GLB_BASE_PHY_ADDR + PMU_GLB_SLP_EN_SET_ADDR);
		val = __raw_readl((void *)PMU_GLB_BASE_PHY_ADDR + PMU_GLB_CPU_SYS_SLP_REQ_ADDR);
		val |= BIT_PMU_GLB_CPU_SYS_SLP_REQ_MASK;
		__raw_writel(val, (void *)PMU_GLB_BASE_PHY_ADDR + PMU_GLB_CPU_SYS_SLP_REQ_ADDR);
		/*inorder to use ca53_cfg_rvbaraddr0_h register to set cpu reset addr,
		we must clear below bits here or we must set it to 1.
		*/
		val = __raw_readl((void *)PMU_GLB_BASE_PHY_ADDR + PMU_GLB_PWR_BYPASS_ADDR);
		val &= ~(0x1 << 0);
		__raw_writel(val, (void *)PMU_GLB_BASE_PHY_ADDR + PMU_GLB_PWR_BYPASS_ADDR);

		if (__raw_readl((void*)PLL_ALWAYS_ON_ADDR) == 0x0) {
			/* clear pll clock gating eb , except cpll */
			__raw_writel(0x7e, (void *)PLLC_GLB_REG_BASE + PLL_GRP_PLL_RE_OPEN_CLR_ADDR);
			__raw_writel(0x0, (void *)DPLL_CTRL_GLB_BASE_ADDR + DPLL_PLL_RE_OPEN_ADDR);
		}
		__raw_writel(AX_KERNEL_SLEEP_STAGE_0C, (void*)SLEEP_STAGE_STORE_ADDR);
		timer32_wakeup_config(TIMER32_COUNT_PER_SEC); /* default 1s */
		__raw_writel(AX_KERNEL_SLEEP_STAGE_0D, (void*)SLEEP_STAGE_STORE_ADDR);
		break;
	case SLP_EXIT:
		__raw_writel(AX_KERNEL_WAKEUP_STAGE_0E, (void*)SLEEP_STAGE_STORE_ADDR);
		/* wakeup timer64 start */
		*((volatile unsigned long long*)WAKEUP_START_TIMESTAMP_ADDR) = 0x0;
		timer64_val = *((volatile unsigned long long*)TIMER64_CNT_LOW_ADDR);
		*((volatile unsigned long long*)WAKEUP_START_TIMESTAMP_ADDR) = timer64_val;

		if (__raw_readl((void*)PLL_ALWAYS_ON_ADDR) == 0x0) {
			/* set pll clock gating eb */
			__raw_writel(0x7e, (void *)PLLC_GLB_REG_BASE + PLL_GRP_PLL_RE_OPEN_SET_ADDR);
			__raw_writel(0x1, (void *)DPLL_CTRL_GLB_BASE_ADDR + DPLL_PLL_RE_OPEN_ADDR);
		}

		/* cpu sys wakeup and clock restore */
		cpu_clk_mux_0_reserve = __raw_readl((void *)CPU_CLK_MUX_0_BAK_ADDR);
		cpu_clk_eb_0_reserve = __raw_readl((void *)CPU_CLK_EB_0_BAK_ADDR);
		cpu_clk_eb_1_reserve = __raw_readl((void *)CPU_CLK_EB_1_BAK_ADDR);
		cpu_clk_div_0_reserve = __raw_readl((void *)CPU_CLK_DIV_0_BAK_ADDR);
		common_clk_mux_0_reserve =  __raw_readl((void *)COMMON_CLK_MUX_0_BAK_ADDR);
		common_clk_mux_2_reserve =  __raw_readl((void *)COMMON_CLK_MUX_2_BAK_ADDR);
		__raw_writel(cpu_clk_mux_0_reserve, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_CLK_MUX_0_ADDR);
		__raw_writel(cpu_clk_eb_0_reserve, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_CLK_EB_0_ADDR);
		__raw_writel(cpu_clk_eb_1_reserve, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_CLK_EB_1_ADDR);
		__raw_writel(cpu_clk_div_0_reserve, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_CLK_DIV_0_ADDR);
		__raw_writel(common_clk_mux_0_reserve, (void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_CLK_MUX_0_ADDR);
		__raw_writel(common_clk_mux_2_reserve, (void *)COMMON_SYS_GLB_PHY_BASE + COMMON_SYS_CLK_MUX_2_ADDR);

		__raw_writel(BIT_CPU_SYS_CA53_CLUSTER_INT_DISABLE_CLR | BIT_CPU_SYS_CA53_CPU_INT_DISABLE_CLR, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_INT_MSK_CLR_ADDR);
		__raw_writel(BIT_CPU_SYS_DATA_IDLE_EN_CLR, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_LPC_CFG_2_CLR_ADDR);
		__raw_writel(BIT_CPU_SYS_DATA_BUS_IDLE_EN_CLR, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_LPC_CFG_1_CLR_ADDR);
		__raw_writel(BIT_CPU_SYS_DATA_NOC_TIMEOUT_EN_CLR, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_LPC_CFG_1_CLR_ADDR);

		__raw_writel(BIT_CPU_SYS_CFG_BUS_IDLE_EN_CLR, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_LPC_CFG_0_CLR_ADDR);
		__raw_writel(BIT_CPU_SYS_CFG_NOC_TIMEOUT_EN_CLR, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_LPC_CFG_0_CLR_ADDR);

		__raw_writel(BIT_CPU_SYS_SLP_EN_CLR, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_LPC_CFG_2_CLR_ADDR);

		__raw_writel(1 << MODULE_CPU, (void *)PMU_GLB_BASE_PHY_ADDR + PMU_GLB_SLP_EN_CLR_ADDR);
		__raw_writel(BIT_CPU_SYS_DATA_IDLE_EN_CLR, (void *)CPU_SYS_GLB_BASE_PHY_ADDR + CPU_SYS_LPC_CFG_2_CLR_ADDR);

		/* gtimer config & enable */
		__raw_writel(0x0, (void *)GTMR_BASE_PHY_ADDR + GTMR_CNTCVL);
		__raw_writel(0x0, (void *)GTMR_BASE_PHY_ADDR + GTMR_CNTCVU);
		__raw_writel(GTMR_FREQ, (void *)GTMR_BASE_PHY_ADDR + GTMR_CNTFID0);
		__raw_writel(BITS_ENABLE_GTMR, (void *)GTMR_BASE_PHY_ADDR + GTMR_CNTCR);
		__raw_writel(AX_KERNEL_WAKEUP_STAGE_0F, (void*)SLEEP_STAGE_STORE_ADDR);
		break;
	default:

		break;
	}
}
static void noinline cpu_sys_sleep_in_iram_end(void) {}

static DEFINE_MUTEX(pll_state_mutex);

static int pll_state_show(struct seq_file *m, void *v)
{
	mutex_lock(&pll_state_mutex);
	seq_printf(m, "%d\r\n", pll_state);
	mutex_unlock(&pll_state_mutex);
	return 0;
}

static int pll_state_open(struct inode *inode, struct file *file)
{
	return single_open(file, pll_state_show, NULL);
}

ssize_t pll_state_write(struct file *file, const char __user * buffer, size_t count, loff_t *pos)
{
	char kbuf[32] = { 0 };

	if (count > 32)
		return -1;

	if (copy_from_user(kbuf, buffer, count))
		return -EFAULT;

	if (sscanf(kbuf, "%d", (unsigned int*)&pll_state) != 1)
		return -1;

	mutex_lock(&pll_state_mutex);
	if (pll_state) {
		writel(1, comm_sys_glb_base + COMMON_SYS_PAD_SLEEP_BYP_ADDR);
		writel(1, comm_sys_glb_base + COMMON_SYS_PLL_SLEEP_BYP_ADDR);
		writel(1, comm_sys_glb_base + COMMON_SYS_XTAL_SLEEP_BYP_ADDR);
		writel(0x6DB6E, ddr_sys_glb_base); /* set ddr clk mux to 416M */
		writel(0x1, iram_base + PLL_ALWAYS_ON_ADDR); /* pll alway open way */
	}
	mutex_unlock(&pll_state_mutex);

	return count;
}

static void copy_again(void)
{
	unsigned int a64_rst_a32_code_sz, cpu_sys_slp_code_sz, ddr_sys_slp_code_sz;
#ifdef CONFIG_THUMB2_KERNEL
	unsigned long vaddr;
#endif

	a64_rst_a32_code_sz = sizeof(a64_warmreset_a32_code);
	cpu_sys_slp_code_sz = (void *)cpu_sys_sleep_in_iram_end - (void *)cpu_sys_sleep_in_iram;
	ddr_sys_slp_code_sz = (void *)ddr_sys_sleep_in_iram_end - (void *)ddr_sys_sleep_in_iram;
	BUG_ON(ax620e_slp_resume_code_sz > IRAM_SLP_WAKE_CODE_SZ_MAX);
	BUG_ON(a64_rst_a32_code_sz > (CPU_SYS_SLEEP_CODE_ADDR - A64_WARMRST_A32_ADDR));
	BUG_ON(cpu_sys_slp_code_sz > (DDR_SYS_SLEEP_CODE_ADDR - CPU_SYS_SLEEP_CODE_ADDR));
	BUG_ON(ddr_sys_slp_code_sz > (CPU_CLK_MUX_0_BAK_ADDR - DDR_SYS_SLEEP_CODE_ADDR));
#ifndef CONFIG_THUMB2_KERNEL
	memcpy(iram_base, ax620e_slp_cpu_resume, ax620e_slp_resume_code_sz);
	memcpy(iram_base + A64_WARMRST_A32_ADDR, (void *)a64_warmreset_a32_code, sizeof(a64_warmreset_a32_code));
	memcpy(iram_base + CPU_SYS_SLEEP_CODE_ADDR, cpu_sys_sleep_in_iram, (void *)cpu_sys_sleep_in_iram_end - (void *)cpu_sys_sleep_in_iram);
	memcpy(iram_base + DDR_SYS_SLEEP_CODE_ADDR, ddr_sys_sleep_in_iram, (void *)ddr_sys_sleep_in_iram_end - (void *)ddr_sys_sleep_in_iram);
#else
	vaddr = (unsigned long)ax620e_slp_cpu_resume & -2;
	memcpy(iram_base, (void *)vaddr, ax620e_slp_resume_code_sz);
	memcpy(iram_base + A64_WARMRST_A32_ADDR, (void *)a64_warmreset_a32_code, sizeof(a64_warmreset_a32_code));
	vaddr = (unsigned long)cpu_sys_sleep_in_iram & -2;
	memcpy(iram_base + CPU_SYS_SLEEP_CODE_ADDR, (void *)vaddr, (void *)cpu_sys_sleep_in_iram_end - (void *)cpu_sys_sleep_in_iram);
	vaddr = (unsigned long)ddr_sys_sleep_in_iram & -2;
	memcpy(iram_base + DDR_SYS_SLEEP_CODE_ADDR, (void *)vaddr, (void *)ddr_sys_sleep_in_iram_end - (void *)ddr_sys_sleep_in_iram);
#endif
	flush_cache_all();

}

static const struct file_operations pll_state_fsops = {
	.open = pll_state_open,
	.read = seq_read,
	.write = pll_state_write,
	.release = single_release,
};

int ax620e_suspend_init(struct device_node *np)
{
	struct resource iram_res;
	int ret, len;
	phys_addr_t physical_addr;
	unsigned int a64_rst_a32_code_sz, cpu_sys_slp_code_sz, ddr_sys_slp_code_sz;
#ifdef CONFIG_THUMB2_KERNEL
	unsigned long vaddr;
#endif
	const char *auto_tmr_wake;

	pmu_glb_base = axera_get_iomem_byname(np, "pmu_glb");
	if (NULL == pmu_glb_base) {
		pr_err("%s: could not map pmu resource\n", __func__);
		goto iomap_err;
	}

	comm_sys_glb_base = axera_get_iomem_byname(np, "comm_sys_glb");
	if (NULL == comm_sys_glb_base) {
		pr_err("%s: could not map comm_sys_glb resource\n", __func__);
		goto iomap_err;
	}

	pinmux_g6_base = axera_get_iomem_byname(np, "pinmux_g6");
	if (NULL == pinmux_g6_base) {
		pr_err("%s: could not map pinmux_g6 resource\n", __func__);
		goto iomap_err;
	}

	cpu_sys_glb_base = axera_get_iomem_byname(np, "cpu_sys_glb");
	if (NULL == cpu_sys_glb_base) {
		pr_err("%s: could not map cpu_sys_glb resource\n", __func__);
		goto iomap_err;
	}

	dbg_sys_glb_base = axera_get_iomem_byname(np, "dbg_sys_glb");
	if (NULL == dbg_sys_glb_base) {
		pr_err("%s: could not map dbg_sys_glb_base resource\n", __func__);
		goto iomap_err;
	}

	iram_base = axera_get_iomem_byname(np, "iram_base");
	if (NULL == iram_base) {
		pr_err("%s: could not map iram resource\n", __func__);
		goto iomap_err;
	}

	ddrc_base = axera_get_iomem_byname(np, "ddrc_base");
	if (NULL == ddrc_base) {
		pr_err("%s: could not map ddrc resource\n", __func__);
		goto iomap_err;
	}

	flash_sys_glb_base = axera_get_iomem_byname(np, "flash_sys_glb");
	if (NULL == flash_sys_glb_base) {
		pr_err("%s: could not map flash_sys_glb_base resource\n", __func__);
		goto iomap_err;
	}

	isp_sys_glb_base = axera_get_iomem_byname(np, "isp_sys_glb");
	if (NULL == isp_sys_glb_base) {
		pr_err("%s: could not map isp_sys_glb_base resource\n", __func__);
		goto iomap_err;
	}

	mm_sys_glb_base = axera_get_iomem_byname(np, "mm_sys_glb");
	if (NULL == mm_sys_glb_base) {
		pr_err("%s: could not map mm_sys_glb_base resource\n", __func__);
		goto iomap_err;
	}

	npu_sys_glb_base = axera_get_iomem_byname(np, "npu_sys_glb");
	if (NULL == npu_sys_glb_base) {
		pr_err("%s: could not map npu_sys_glb_base resource\n", __func__);
		goto iomap_err;
	}

	vpu_sys_glb_base = axera_get_iomem_byname(np, "vpu_sys_glb");
	if (NULL == vpu_sys_glb_base) {
		pr_err("%s: could not map vpu_sys_glb_base resource\n", __func__);
		goto iomap_err;
	}

	periph_sys_glb_base = axera_get_iomem_byname(np, "periph_sys_glb");
	if (NULL == periph_sys_glb_base) {
		pr_err("%s: could not map periph_sys_glb_base resource\n", __func__);
		goto iomap_err;
	}

	comm_cpu_glb_base = axera_get_iomem_byname(np, "comm_cpu_glb");
	if (NULL == comm_cpu_glb_base) {
		pr_err("%s: could not map comm_cpu_glb_base resource\n", __func__);
		goto iomap_err;
	}

	pllc_glb_base = axera_get_iomem_byname(np, "pllc_glb");
	if (NULL == pllc_glb_base) {
		pr_err("%s: could not map pllc_glb_base resource\n", __func__);
		goto iomap_err;
	}

	dpll_ctrl_glb_base = axera_get_iomem_byname(np, "dpll_ctrl_glb");
	if (NULL == dpll_ctrl_glb_base) {
		pr_err("%s: could not map dpll_ctrl_glb_base resource\n", __func__);
		goto iomap_err;
	}

	ddr_sys_glb_base = axera_get_iomem_byname(np, "ddr_sys_glb");
	if (NULL == ddr_sys_glb_base) {
		pr_err("%s: could not map ddr_sys_glb_base resource\n", __func__);
		goto iomap_err;
	}

	sleep_stage_addr = ioremap(SLEEP_STAGE_STORE_ADDR, 0x4);
	if (NULL == sleep_stage_addr) {
		pr_err("%s: could not map sleep_stage_addr\n", __func__);
		goto iomap_err;
	}

	ret = axera_get_resource_byname(np, "iram_base", &iram_res);
	if (ret)
		goto iomap_err;

	ret = ax620e_pm_setup_idmap(iram_res.start, iram_res.end + 1);
	if (ret) {
		pr_err("%s: could not remap iram to page table\n", __func__);
		goto iomap_err;
	}

	auto_tmr_wake = of_get_property(np, "auto_tmr_wake", &len);
	if ((auto_tmr_wake != NULL) && (len > 0) &&
		(!strcmp(auto_tmr_wake, "true") || !strcmp(auto_tmr_wake, "TRUE"))) {
		writel(1, iram_base + TIMER_EB_ADDR);
	} else {
		pr_info("%s auto_tmr_wake property not exist\n", __func__);
		writel(0, iram_base + TIMER_EB_ADDR);
	}

	physical_addr = virt_to_phys((void *)cpu_resume);
	writel(physical_addr, iram_base + CPU_RESUME_PHY_ADDR);
#ifdef CONFIG_SMP
	physical_addr = virt_to_phys((void *)&pen_release);
	writel(physical_addr, iram_base + PEN_RELEASE_PHY_ADDR);
	physical_addr = virt_to_phys((void *)secondary_startup);
	writel(physical_addr, iram_base + SECONDARY_STARTUP_PHY_ADDR);
#endif

	a64_rst_a32_code_sz = sizeof(a64_warmreset_a32_code);
	cpu_sys_slp_code_sz = (void *)cpu_sys_sleep_in_iram_end - (void *)cpu_sys_sleep_in_iram;
	ddr_sys_slp_code_sz = (void *)ddr_sys_sleep_in_iram_end - (void *)ddr_sys_sleep_in_iram;
	BUG_ON(ax620e_slp_resume_code_sz > IRAM_SLP_WAKE_CODE_SZ_MAX);
	BUG_ON(a64_rst_a32_code_sz > (CPU_SYS_SLEEP_CODE_ADDR - A64_WARMRST_A32_ADDR));
	BUG_ON(cpu_sys_slp_code_sz > (DDR_SYS_SLEEP_CODE_ADDR - CPU_SYS_SLEEP_CODE_ADDR));
	BUG_ON(ddr_sys_slp_code_sz > (CPU_CLK_MUX_0_BAK_ADDR - DDR_SYS_SLEEP_CODE_ADDR));
#ifndef CONFIG_THUMB2_KERNEL
	memcpy(iram_base, ax620e_slp_cpu_resume, ax620e_slp_resume_code_sz);
	memcpy(iram_base + A64_WARMRST_A32_ADDR, (void *)a64_warmreset_a32_code, sizeof(a64_warmreset_a32_code));
	memcpy(iram_base + CPU_SYS_SLEEP_CODE_ADDR, cpu_sys_sleep_in_iram, (void *)cpu_sys_sleep_in_iram_end - (void *)cpu_sys_sleep_in_iram);
	memcpy(iram_base + DDR_SYS_SLEEP_CODE_ADDR, ddr_sys_sleep_in_iram, (void *)ddr_sys_sleep_in_iram_end - (void *)ddr_sys_sleep_in_iram);
#else
	vaddr = (unsigned long)ax620e_slp_cpu_resume & -2;
	memcpy(iram_base, (void *)vaddr, ax620e_slp_resume_code_sz);
	memcpy(iram_base + A64_WARMRST_A32_ADDR, (void *)a64_warmreset_a32_code, sizeof(a64_warmreset_a32_code));
	vaddr = (unsigned long)cpu_sys_sleep_in_iram & -2;
	memcpy(iram_base + CPU_SYS_SLEEP_CODE_ADDR, (void *)vaddr, (void *)cpu_sys_sleep_in_iram_end - (void *)cpu_sys_sleep_in_iram);
	vaddr = (unsigned long)ddr_sys_sleep_in_iram & -2;
	memcpy(iram_base + DDR_SYS_SLEEP_CODE_ADDR, (void *)vaddr, (void *)ddr_sys_sleep_in_iram_end - (void *)ddr_sys_sleep_in_iram);
#endif
	flush_cache_all();

	ax620e_pmu_init(pmu_glb_base);
	ax620e_chip_top_init();

	pll_choose_root = proc_mkdir(PLL_CHOOSE_ROOT_NAME, NULL);
	if (pll_choose_root == NULL) {
		goto iomap_err;
	}

	proc_create_data(PLL_STATE, 0644, pll_choose_root,
			 &pll_state_fsops, NULL);

	printk("ax620e suspend init finished\n");
	return 0;

iomap_err:

	if (comm_cpu_glb_base)
		iounmap(comm_cpu_glb_base);

	if (periph_sys_glb_base)
		iounmap(periph_sys_glb_base);

	if (vpu_sys_glb_base)
		iounmap(vpu_sys_glb_base);

	if (npu_sys_glb_base)
		iounmap(npu_sys_glb_base);

	if (mm_sys_glb_base)
		iounmap(mm_sys_glb_base);

	if (isp_sys_glb_base)
		iounmap(isp_sys_glb_base);

	if (flash_sys_glb_base)
		iounmap(flash_sys_glb_base);

	if (ddrc_base)
		iounmap(ddrc_base);

	if (iram_base)
		iounmap(iram_base);

	if (dbg_sys_glb_base)
		iounmap(dbg_sys_glb_base);

	if (cpu_sys_glb_base)
		iounmap(cpu_sys_glb_base);

	if (pinmux_g6_base)
		iounmap(pinmux_g6_base);

	if (comm_sys_glb_base)
		iounmap(comm_sys_glb_base);

	if (pmu_glb_base)
		iounmap(pmu_glb_base);

	if (pllc_glb_base)
		iounmap(pllc_glb_base);

	if (dpll_ctrl_glb_base)
		iounmap(dpll_ctrl_glb_base);

	if (ddr_sys_glb_base)
		iounmap(ddr_sys_glb_base);

	if (sleep_stage_addr)
		iounmap(sleep_stage_addr);

	return -1;
}
