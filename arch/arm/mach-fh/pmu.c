#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/delay.h>
#include <mach/io.h>
#include <linux/io.h>
#include "chip.h"
#include "soc.h"

static int s_pmu_inited = 0;
static void __iomem	*fh_pmu_regs = NULL;

#ifndef INTERNAL_PHY
#define INTERNAL_PHY	0x55
#endif
#ifndef EXTERNAL_PHY
#define	EXTERNAL_PHY	0xaa
#endif

void fh_pmu_set_reg(unsigned int offset, unsigned int data)
{

	if (offset > FH_PMU_REG_SIZE) {
		pr_err("fh_pmu_set_reg: offset is out of range");
		return;
	}

	writel(data, fh_pmu_regs + offset);
}
EXPORT_SYMBOL(fh_pmu_set_reg);

unsigned int fh_pmu_get_reg(unsigned int offset)
{

	if (offset > FH_PMU_REG_SIZE) {
		pr_err("fh_pmu_get_reg: offset is out of range");
		return 0;
	}
	return readl(fh_pmu_regs + offset);
}
EXPORT_SYMBOL(fh_pmu_get_reg);

void fh_pmu_set_reg_m(unsigned int offset, unsigned int data, unsigned int mask)
{
	fh_pmu_set_reg(offset, (fh_pmu_get_reg(offset) & (~(mask))) |
				       ((data) & (mask)));
}
EXPORT_SYMBOL(fh_pmu_set_reg_m);

void fh_get_chipid(unsigned int *plat_id, unsigned int *chip_id)
{
	unsigned int _plat_id = 0;

	_plat_id = fh_pmu_get_reg(REG_PMU_CHIP_ID);
	if (plat_id != NULL)
		*plat_id = _plat_id;

	if (chip_id != NULL)
		*chip_id = fh_pmu_get_reg(REG_PMU_IP_VER);
}
EXPORT_SYMBOL(fh_get_chipid);

unsigned int fh_pmu_get_ptsl(void)
{
	fh_pmu_set_reg(REG_PMU_PTSLO, 0x01);
	return fh_pmu_get_reg(REG_PMU_PTSLO);
}
EXPORT_SYMBOL(fh_pmu_get_ptsl);

unsigned int fh_pmu_get_ptsh(void)
{
	fh_pmu_set_reg(REG_PMU_PTSLO, 0x01);
	return fh_pmu_get_reg(REG_PMU_PTSHI);
}
EXPORT_SYMBOL(fh_pmu_get_ptsh);

unsigned long long fh_get_pts64(void)
{
	unsigned int high, low;
	unsigned long long pts64;

	fh_pmu_set_reg(REG_PMU_PTSLO, 0x01);
	high = fh_pmu_get_reg(REG_PMU_PTSHI);
	low = fh_pmu_get_reg(REG_PMU_PTSLO);
	pts64 = (((unsigned long long)high)<<32)|((unsigned long long)low);
	return pts64;
}
EXPORT_SYMBOL(fh_get_pts64);

void fh_pmu_wdt_pause(void)
{
	unsigned int reg;

	reg = fh_pmu_get_reg(REG_PMU_WDT_CTRL);
	reg |= 0x100;
	fh_pmu_set_reg(REG_PMU_WDT_CTRL, reg);
}
EXPORT_SYMBOL_GPL(fh_pmu_wdt_pause);

void fh_pmu_wdt_resume(void)
{
	unsigned int reg;

	reg = fh_pmu_get_reg(REG_PMU_WDT_CTRL);
	reg &= ~(0x100);
	fh_pmu_set_reg(REG_PMU_WDT_CTRL, reg);
}
EXPORT_SYMBOL_GPL(fh_pmu_wdt_resume);

void fh_pmu_usb_utmi_rst(void)
{
	unsigned int pmu_reg;

	pmu_reg = fh_pmu_get_reg(REG_PMU_SWRST_MAIN_CTRL);
	pmu_reg &= ~(USB_UTMI_RST_BIT);
	fh_pmu_set_reg(REG_PMU_SWRST_MAIN_CTRL, pmu_reg);
	pmu_reg = fh_pmu_get_reg(REG_PMU_SWRST_MAIN_CTRL);
	mdelay(1);
	pmu_reg |= USB_UTMI_RST_BIT;
	fh_pmu_set_reg(REG_PMU_SWRST_MAIN_CTRL, pmu_reg);
	pmu_reg = fh_pmu_get_reg(REG_PMU_SWRST_MAIN_CTRL);
	msleep(20);
}
EXPORT_SYMBOL_GPL(fh_pmu_usb_utmi_rst);

void fh_pmu_usb_phy_rst(void)
{
	unsigned int pmu_reg;

	pmu_reg = fh_pmu_get_reg(REG_PMU_USB_SYS1);
	pmu_reg &= (~USB_IDDQ_PWR_BIT);
	fh_pmu_set_reg(REG_PMU_USB_SYS1, pmu_reg);
	mdelay(1);
	pmu_reg = fh_pmu_get_reg(REG_PMU_USB_SYS);
	pmu_reg |= (USB_PHY_RST_BIT);
	fh_pmu_set_reg(REG_PMU_USB_SYS, pmu_reg);
	mdelay(1);
	pmu_reg = fh_pmu_get_reg(REG_PMU_USB_SYS);
	pmu_reg &= (~USB_PHY_RST_BIT);
	fh_pmu_set_reg(REG_PMU_USB_SYS, pmu_reg);
}
EXPORT_SYMBOL_GPL(fh_pmu_usb_phy_rst);

void fh_pmu_usb_resume(void)
{
	unsigned int pmu_reg;

	pmu_reg = fh_pmu_get_reg(REG_PMU_USB_SYS);
	pmu_reg |= (USB_SLEEP_MODE_BIT);
	fh_pmu_set_reg(REG_PMU_USB_SYS, pmu_reg);
	mdelay(1);
}
EXPORT_SYMBOL_GPL(fh_pmu_usb_resume);

void _pmu_main_reset(unsigned int reg, unsigned int retry, unsigned int udelay)
{
	fh_pmu_set_reg(REG_PMU_SWRST_MAIN_CTRL, reg);

	while (fh_pmu_get_reg(REG_PMU_SWRST_MAIN_CTRL) != 0xffffffff) {
		if (retry-- <= 0)
			return;

		udelay(udelay);
	}
}

#if defined(CONFIG_MACH_FH8856) || \
	defined(CONFIG_MACH_FH8852) || \
	defined(CONFIG_MACH_FH8626V100)

void _pmu_axi_reset(unsigned int reg, unsigned int retry, unsigned int udelay)
{
	fh_pmu_set_reg(REG_PMU_SWRST_AXI_CTRL, reg);

	while (fh_pmu_get_reg(REG_PMU_SWRST_AXI_CTRL) != 0xffffffff) {
		if (retry-- <= 0)
			return;

		udelay(udelay);
	}
}
#endif

void _pmu_ahb_reset(unsigned int reg, unsigned int retry, unsigned int udelay)
{
	fh_pmu_set_reg(REG_PMU_SWRST_AHB_CTRL, reg);

	while (fh_pmu_get_reg(REG_PMU_SWRST_AHB_CTRL) != 0xffffffff) {
		if (retry-- <= 0)
			return;

		udelay(udelay);
	}
}

void _pmu_apb_reset(unsigned int reg, unsigned int retry, unsigned int udelay)
{
	fh_pmu_set_reg(REG_PMU_SWRST_APB_CTRL, reg);

	while (fh_pmu_get_reg(REG_PMU_SWRST_APB_CTRL) != 0xffffffff) {
		if (retry-- <= 0)
			return;

		udelay(udelay);
	}
}

void _pmu_nsr_reset(unsigned int reg, unsigned int reset_time)
{
	fh_pmu_set_reg(REG_PMU_SWRSTN_NSR, reg);
	udelay(reset_time);
	fh_pmu_set_reg(REG_PMU_SWRSTN_NSR, 0xFFFFFFFF);
}

void fh_pmu_sdc_reset(int slot_id)
{
	unsigned int reg = 0;

	if (slot_id == 1)
		reg = ~(1 << SDC1_HRSTN_BIT);
	else if (slot_id == 0)
		reg = ~(1 << SDC0_HRSTN_BIT);
	else
		reg = 0;

	_pmu_ahb_reset(reg, 1000, 1);
}
EXPORT_SYMBOL_GPL(fh_pmu_sdc_reset);

void fh_pmu_mipi_reset(void)
{
	_pmu_apb_reset(~(1 << MIPIW_PRSTN_BIT | 1 << MIPIC_PRSTN_BIT), 1000, 1);
}
EXPORT_SYMBOL_GPL(fh_pmu_mipi_reset);

void fh_pmu_enc_reset(void)
{
#if !defined(CONFIG_ARCH_FH8626V100) \
	&& !defined(CONFIG_ARCH_FH8636) \
	&& !defined(CONFIG_ARCH_FH8852V101)
	_pmu_ahb_reset(~(1 << VCU_HRSTN_BIT), 100, 10);
#endif
}
EXPORT_SYMBOL_GPL(fh_pmu_enc_reset);

void fh_pmu_dwi2s_set_clk(unsigned int div_i2s, unsigned int div_mclk)
{
#if !defined(CONFIG_ARCH_FH8636) && !defined(CONFIG_ARCH_FH8852V101)
	unsigned int reg;

	reg = fh_pmu_get_reg(PMU_DWI2S_CLK_DIV_REG);
	reg &= ~(0xffff << PMU_DWI2S_CLK_DIV_SHIFT);
	reg |= ((div_i2s-1) << 8 | (div_mclk-1)) << PMU_DWI2S_CLK_DIV_SHIFT;
	fh_pmu_set_reg(PMU_DWI2S_CLK_DIV_REG, reg);

	/* i2s_clk switch to PLLVCO */
	reg = fh_pmu_get_reg(PMU_DWI2S_CLK_SEL_REG);
	reg &= ~(1 << PMU_DWI2S_CLK_SEL_SHIFT);
	reg |= 1 << PMU_DWI2S_CLK_SEL_SHIFT;
	fh_pmu_set_reg(PMU_DWI2S_CLK_SEL_REG, reg);
#endif
}
EXPORT_SYMBOL_GPL(fh_pmu_dwi2s_set_clk);

void fh_pmu_eth_set_speed(unsigned int speed)
{
	unsigned int reg;

	reg = fh_pmu_get_reg(PMU_RMII_SPEED_MODE);
	if (speed == 10)
		reg &= ~(FH_GMAC_SPEED_100M);
	else if (speed == 100)
		reg |= FH_GMAC_SPEED_100M;
	else
		printk(KERN_ERR"ERROR: wrong param for emac set speed, %d\n",
			speed);

	fh_pmu_set_reg(PMU_RMII_SPEED_MODE, reg);
}
EXPORT_SYMBOL_GPL(fh_pmu_eth_set_speed);

void fh_pmu_eth_reset(void)
{
	_pmu_ahb_reset(~(1 << EMAC_HRSTN_BIT), 1000, 1);
}
EXPORT_SYMBOL_GPL(fh_pmu_eth_reset);

void fh_pmu_restart(void)
{
	fh_pmu_set_reg(REG_PMU_SWRST_MAIN_CTRL, 0x7fffffff);
}


unsigned int fh_pmu_get_tsensor_init_data(void)
{
#if defined(CONFIG_ARCH_FH885xV200) || \
defined(CONFIG_ARCH_FH865x) || \
defined(CONFIG_ARCH_FH8636) || \
defined(CONFIG_ARCH_FH8852V101)
	return fh_pmu_get_reg(REG_PMU_RTC_PARAM);
#else
	return 0;
#endif
}
EXPORT_SYMBOL_GPL(fh_pmu_get_tsensor_init_data);

unsigned int fh_pmu_get_ddrsize(void)
{
#if defined(CONFIG_ARCH_FH885xV200) || \
defined(CONFIG_ARCH_FH865x) || \
defined(CONFIG_ARCH_FH8636) || \
defined(CONFIG_ARCH_FH8852V101)
	return fh_pmu_get_reg(REG_PMU_DDR_SIZE);
#else
	return 0;
#endif
}
EXPORT_SYMBOL_GPL(fh_pmu_get_ddrsize);

#if defined(CONFIG_ARCH_FH885xV200) || \
defined(CONFIG_ARCH_FH865x)

#define REFIX_TRAIN_DATA_OFFSET	(0)
#define MIN_TRAINING_DATA	-8
#define MAX_TRAINING_DATA	7
int __refix_train_data(int *pdata, int offset)
{
	int temp_data;

	temp_data = *pdata;
	temp_data &= INSIDE_PHY_TRAINING_MASK;

	if (temp_data & 0x08)
		temp_data  = (~INSIDE_PHY_TRAINING_MASK) | temp_data;

	temp_data += offset;

	if (temp_data < MIN_TRAINING_DATA || temp_data > MAX_TRAINING_DATA) {
		pr_err("cal data [%d] should limit in [%d , %d]\n",
		temp_data, MIN_TRAINING_DATA, MAX_TRAINING_DATA);
		return -1;
	}
	temp_data &= INSIDE_PHY_TRAINING_MASK;

	*pdata = temp_data;
	return 0;
}

void refix_train_data(int *pdata)
{
	__refix_train_data(pdata, REFIX_TRAIN_DATA_OFFSET);
}
#endif

int fh_pmu_internal_ephy_reset(void)
{
#if defined CONFIG_FH_EPHY
	pr_err("fh_pmu_internal_ephy_reset v2 get in...\n");
	//add internal phy reset code below...
#elif defined(CONFIG_ARCH_FH885xV200) || \
defined(CONFIG_ARCH_FH865x)
	int train_data;
	/*reset sunplus phy */
	/*
	 * ephy shutdown: ---____________________________
	 * ephy rst         : ___|<=L_10ms=>|--|<=H_12us=>|--
	 * ephy clkin       : _-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
	 */
	/* pull up shut down first.. */
	fh_pmu_set_reg_m(REG_PMU_ETHPHY_REG0,
	1 << INSIDE_PHY_SHUTDOWN_BIT_POS,
	1 << INSIDE_PHY_SHUTDOWN_BIT_POS);

	udelay(5);
	/* pull down shut down and rst for 10ms. */
	fh_pmu_set_reg_m(REG_PMU_ETHPHY_REG0, 0,
	1 << INSIDE_PHY_SHUTDOWN_BIT_POS);

	fh_pmu_set_reg_m(REG_PMU_SWRSTN_NSR, 0,
	1 << INSIDE_PHY_RST_BIT_POS);

	mdelay(10);
	/* pull up rst for 12us. */
	fh_pmu_set_reg_m(REG_PMU_SWRSTN_NSR,
	1 << INSIDE_PHY_RST_BIT_POS,
	1 << INSIDE_PHY_RST_BIT_POS);

	udelay(12);

	train_data = fh_pmu_get_reg(REG_PMU_EPHY_PARAM);
	if (train_data & (1 << TRAINING_EFUSE_ACTIVE_BIT_POS)) {
		/* train_data */
		refix_train_data(&train_data);
		pr_info("ephy: training data is :%x\n",
		train_data);
		fh_pmu_set_reg_m(REG_PMU_ETHPHY_REG0,
		(train_data & INSIDE_PHY_TRAINING_MASK)
		<< INSIDE_PHY_TRAINING_BIT_POS,
		INSIDE_PHY_TRAINING_MASK << INSIDE_PHY_TRAINING_BIT_POS);
	} else{
		/* todo set default value...*/
		train_data = 0;
		pr_info("ephy: no training data, use default:%x\n",
			train_data);
		/* train_data */
		fh_pmu_set_reg_m(REG_PMU_ETHPHY_REG0,
		train_data << INSIDE_PHY_TRAINING_BIT_POS,
		INSIDE_PHY_TRAINING_MASK << INSIDE_PHY_TRAINING_BIT_POS);
	}
#endif
	return 0;
}
EXPORT_SYMBOL(fh_pmu_internal_ephy_reset);

void fh_pmu_ephy_sel(__u32 phy_sel)
{
#if defined CONFIG_FH_EPHY
	pr_err("fh_pmu_ephy_sel v2 get in...\n");
	//add multi phy sel code below...

#elif defined(CONFIG_ARCH_FH885xV200) || \
defined(CONFIG_ARCH_FH865x)
	fh_pmu_set_reg_m(REG_PMU_CLK_SEL,
	1 << CLK_SCAN_BIT_POS, 1 << CLK_SCAN_BIT_POS);

	if (phy_sel == EXTERNAL_PHY) {
		/* close inside phy */
		fh_pmu_set_reg_m(REG_PMU_ETHPHY_REG0,
		0 << INSIDE_PHY_ENABLE_BIT_POS, 1 << INSIDE_PHY_ENABLE_BIT_POS);
		/* gate inside phy */
		fh_pmu_set_reg_m(REG_PMU_CLK_GATE1,
		1 << INSIDE_CLK_GATE_BIT_POS,
		1 << INSIDE_CLK_GATE_BIT_POS);
		/* set div */
		fh_pmu_set_reg_m(REG_PMU_CLK_DIV6,
		1 << MAC_REF_CLK_DIV_BIT_POS,
		MAC_REF_CLK_DIV_MASK << MAC_REF_CLK_DIV_BIT_POS);
		/* set pad */
		/* eth ref clk out Ungate */
		fh_pmu_set_reg_m(REG_PMU_CLK_GATE,
		0 << ETH_REF_CLK_OUT_GATE_BIT_POS,
		1 << ETH_REF_CLK_OUT_GATE_BIT_POS);
		/* eth rmii clk Ungate */
		fh_pmu_set_reg_m(REG_PMU_CLK_GATE,
		0 << ETH_RMII_CLK_OUT_GATE_BIT_POS,
		1 << ETH_RMII_CLK_OUT_GATE_BIT_POS);
		/* switch mac clk in */
		fh_pmu_set_reg_m(REG_PMU_CLK_SEL,
		1 << IN_OR_OUT_PHY_SEL_BIT_POS,
		1 << IN_OR_OUT_PHY_SEL_BIT_POS);
	} else {
		/* inside phy enable */
		fh_pmu_set_reg_m(REG_PMU_ETHPHY_REG0,
		1 << INSIDE_PHY_ENABLE_BIT_POS,
		1 << INSIDE_PHY_ENABLE_BIT_POS);
		/* set pad no need */
		/* eth ref clk out gate */
		fh_pmu_set_reg_m(REG_PMU_CLK_GATE,
		1 << ETH_REF_CLK_OUT_GATE_BIT_POS,
		1 << ETH_REF_CLK_OUT_GATE_BIT_POS);
		/* eth rmii clk gate */
		fh_pmu_set_reg_m(REG_PMU_CLK_GATE,
		1 << ETH_RMII_CLK_OUT_GATE_BIT_POS,
		1 << ETH_RMII_CLK_OUT_GATE_BIT_POS);
		/* inside phy clk Ungate */
		fh_pmu_set_reg_m(REG_PMU_CLK_GATE1,
		0 << INSIDE_CLK_GATE_BIT_POS,
		1 << INSIDE_CLK_GATE_BIT_POS);
		/* int rmii refclk mux */
		fh_pmu_set_reg_m(REG_PMU_CLK_SEL,
		0 << IN_OR_OUT_PHY_SEL_BIT_POS,
		1 << IN_OR_OUT_PHY_SEL_BIT_POS);
	}
#else
	return;
#endif
}
EXPORT_SYMBOL(fh_pmu_ephy_sel);

void fh_pmu_set_sdc1_funcsel(unsigned int val)
{
#if defined(CONFIG_ARCH_FH885xV200) || \
defined(CONFIG_ARCH_FH865x) || \
defined(CONFIG_ARCH_FH8636) || \
defined(CONFIG_ARCH_FH8852V101)
	fh_pmu_set_reg(REG_PMU_SD1_FUNC_SEL, val);
#endif
}
EXPORT_SYMBOL_GPL(fh_pmu_set_sdc1_funcsel);

void fh_pmu_arxc_write_A625_INT_RAWSTAT(unsigned int val)
{
	fh_pmu_set_reg(PMU_A625_INT_RAWSTAT, val);
}
EXPORT_SYMBOL_GPL(fh_pmu_arxc_write_A625_INT_RAWSTAT);

unsigned int fh_pmu_arxc_read_ARM_INT_RAWSTAT(void)
{
	return fh_pmu_get_reg(PMU_ARM_INT_RAWSTAT);
}
EXPORT_SYMBOL_GPL(fh_pmu_arxc_read_ARM_INT_RAWSTAT);

void fh_pmu_arxc_write_ARM_INT_RAWSTAT(unsigned int val)
{
	fh_pmu_set_reg(PMU_ARM_INT_RAWSTAT, val);
}
EXPORT_SYMBOL_GPL(fh_pmu_arxc_write_ARM_INT_RAWSTAT);

unsigned int fh_pmu_arxc_read_ARM_INT_STAT(void)
{
	return fh_pmu_get_reg(PMU_ARM_INT_STAT);
}
EXPORT_SYMBOL_GPL(fh_pmu_arxc_read_ARM_INT_STAT);

void fh_pmu_arxc_reset(unsigned long phy_addr)
{
	unsigned int arc_addr;

	/*ARC Reset*/
	fh_pmu_set_reg(REG_PMU_SWRSTN_NSR, ~(1<<ARC_RSTN_BIT));

	arc_addr = ((phy_addr & 0xffff) << 16) | (phy_addr >> 16);

	fh_pmu_set_reg(REG_PMU_A625BOOT0, 0x7940266B);
	/* Configure ARC Bootcode start address */
	fh_pmu_set_reg(REG_PMU_A625BOOT1, arc_addr);
	fh_pmu_set_reg(REG_PMU_A625BOOT2, 0x0F802020);
	fh_pmu_set_reg(REG_PMU_A625BOOT3, arc_addr);

	/*clear ARC ready flag*/
	fh_pmu_arxc_write_ARM_INT_RAWSTAT(0);

	/* don't let ARC run when release ARC */
	fh_pmu_set_reg(REG_PMU_A625_START_CTRL, 0);
	udelay(2);

	/* ARC reset released */
	fh_pmu_set_reg(REG_PMU_SWRSTN_NSR, 0xFFFFFFFF);
}
EXPORT_SYMBOL_GPL(fh_pmu_arxc_reset);

void fh_pmu_arxc_kickoff(void)
{
	//start ARC625
	fh_pmu_set_reg(REG_PMU_A625_START_CTRL, 0x10);
}
EXPORT_SYMBOL_GPL(fh_pmu_arxc_kickoff);

#ifdef CONFIG_USE_OF
static const struct of_device_id fh_pmu_match[] = {
	{
			.compatible = "fh,fh-pmu",
	},
	{},
};
#endif

int fh_pmu_init(void)
{

#ifdef CONFIG_USE_OF
	struct device_node *np;
#endif

	if (s_pmu_inited)
		return 0;

#ifdef CONFIG_USE_OF
	np = of_find_matching_node(NULL, fh_pmu_match);

	if (np) {
		fh_pmu_regs = of_iomap(np, 0);
	} else {
		pr_err("ERROR: cannot get pmu regs from dts\n");
		return -1;
	}
#else
	fh_pmu_regs = (void __iomem *)VA_PMU_REG_BASE;
#endif

	s_pmu_inited = 1;
	return 0;
}
