#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>

#include <mach/io.h>
#include <mach/pmu.h>

#include "chip.h"

static void __iomem *fh_pmu_regs;

void fh_pmu_set_reg(unsigned int offset, unsigned int data)
{
	writel(data, fh_pmu_regs + offset);
}
EXPORT_SYMBOL(fh_pmu_set_reg);

unsigned int fh_pmu_get_reg(unsigned int offset)
{
	return readl(fh_pmu_regs + offset);
}
EXPORT_SYMBOL(fh_pmu_get_reg);

void fh_pmu_set_reg_m(unsigned int offset, unsigned int data,
		      unsigned int mask)
{
	unsigned int value;

	value = fh_pmu_get_reg(offset);
	value &= ~mask;
	value |= data & mask;
	fh_pmu_set_reg(offset, value);
}
EXPORT_SYMBOL(fh_pmu_set_reg_m);

void fh_get_chipid(unsigned int *plat_id, unsigned int *chip_id)
{
	if (plat_id)
		*plat_id = fh_pmu_get_reg(REG_PMU_CHIP_ID);

	if (chip_id)
		*chip_id = fh_pmu_get_reg(REG_PMU_IP_VER);
}
EXPORT_SYMBOL(fh_get_chipid);

unsigned int fh_pmu_get_ddrsize(void)
{
	return 0;
}
EXPORT_SYMBOL(fh_pmu_get_ddrsize);

int fh_pmu_init(void)
{
	if (!fh_pmu_regs)
		fh_pmu_regs = (void __iomem *)VA_PMU_REG_BASE;

	return 0;
}

static void fh_pmu_ahb_reset(unsigned int value, unsigned int retry,
			     unsigned int delay_us)
{
	fh_pmu_set_reg(REG_PMU_SWRST_AHB_CTRL, value);

	while (fh_pmu_get_reg(REG_PMU_SWRST_AHB_CTRL) != 0xffffffff) {
		if (!retry--)
			return;
		udelay(delay_us);
	}
}

void fh_pmu_sdc_reset(int slot_id)
{
	unsigned int reset_value = 0;

	if (slot_id == 0)
		reset_value = ~FH8626V100_SDC0_RESET_MASK;
	else if (slot_id == 1)
		reset_value = ~FH8626V100_SDC1_RESET_MASK;

	fh_pmu_ahb_reset(reset_value, 1000, 1);
}
EXPORT_SYMBOL_GPL(fh_pmu_sdc_reset);

/*
 * Latch and read the common PMU presentation timestamp counter.  Audio DMA
 * records this value on capture completion; the offsets and latch sequence
 * match the exact FH8626V100 stock implementation.
 */
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

	fh_pmu_set_reg(REG_PMU_PTSLO, 0x01);
	high = fh_pmu_get_reg(REG_PMU_PTSHI);
	low = fh_pmu_get_reg(REG_PMU_PTSLO);

	return ((unsigned long long)high << 32) | low;
}
EXPORT_SYMBOL(fh_get_pts64);

void fh_pmu_dwi2s_set_clk(unsigned int div_i2s, unsigned int div_mclk)
{
	unsigned int value;

	value = fh_pmu_get_reg(PMU_DWI2S_CLK_DIV_REG);
	value &= ~(0xffff << PMU_DWI2S_CLK_DIV_SHIFT);
	value |= (((div_i2s - 1) << 8) | (div_mclk - 1))
		 << PMU_DWI2S_CLK_DIV_SHIFT;
	fh_pmu_set_reg(PMU_DWI2S_CLK_DIV_REG, value);

	/* Select the stock audio PLL source after programming both divisors. */
	value = fh_pmu_get_reg(PMU_DWI2S_CLK_SEL_REG);
	value |= 1 << PMU_DWI2S_CLK_SEL_SHIFT;
	fh_pmu_set_reg(PMU_DWI2S_CLK_SEL_REG, value);
}
EXPORT_SYMBOL_GPL(fh_pmu_dwi2s_set_clk);

void fh_pmu_eth_set_speed(unsigned int speed)
{
	unsigned int value;

	value = fh_pmu_get_reg(REG_PMU_ETH_CTRL);

	if (speed == 10)
		value &= ~FH8626V100_ETH_SPEED_100M;
	else if (speed == 100)
		value |= FH8626V100_ETH_SPEED_100M;
	else
		return;

	fh_pmu_set_reg(REG_PMU_ETH_CTRL, value);
}
EXPORT_SYMBOL_GPL(fh_pmu_eth_set_speed);

void fh_pmu_eth_reset(void)
{
	unsigned int retry = 1000;

	fh_pmu_set_reg(REG_PMU_SWRST_AHB_CTRL,
		       ~FH8626V100_EMAC_RESET_MASK);

	while (fh_pmu_get_reg(REG_PMU_SWRST_AHB_CTRL) != 0xffffffff) {
		if (!retry--)
			break;
		udelay(1);
	}
}
EXPORT_SYMBOL_GPL(fh_pmu_eth_reset);

void fh_pmu_ephy_sel(__u32 phy_sel)
{
	(void)phy_sel;
}
EXPORT_SYMBOL(fh_pmu_ephy_sel);

void fh_pmu_restart(void)
{
	fh_pmu_set_reg(REG_PMU_SWRST_APB_CTRL, 0x7fffffff);

	for (;;)
		cpu_relax();
}

void fh_pmu_wdt_pause(void)
{
	unsigned int reg;

	reg = fh_pmu_get_reg(REG_PMU_WDT_CTRL);
	reg |= 0x100;
	fh_pmu_set_reg(REG_PMU_WDT_CTRL, reg);
}

void fh_pmu_wdt_resume(void)
{
	unsigned int reg;

	reg = fh_pmu_get_reg(REG_PMU_WDT_CTRL);
	reg &= ~0x100;
	fh_pmu_set_reg(REG_PMU_WDT_CTRL, reg);
}

void fh_pmu_arxc_write_A625_INT_RAWSTAT(unsigned int value)
{
	fh_pmu_set_reg(REG_PMU_A625_INT_RAWSTAT, value);
}
EXPORT_SYMBOL_GPL(fh_pmu_arxc_write_A625_INT_RAWSTAT);

unsigned int fh_pmu_arxc_read_ARM_INT_RAWSTAT(void)
{
	return fh_pmu_get_reg(REG_PMU_ARM_INT_RAWSTAT);
}
EXPORT_SYMBOL_GPL(fh_pmu_arxc_read_ARM_INT_RAWSTAT);

void fh_pmu_arxc_write_ARM_INT_RAWSTAT(unsigned int value)
{
	fh_pmu_set_reg(REG_PMU_ARM_INT_RAWSTAT, value);
}
EXPORT_SYMBOL_GPL(fh_pmu_arxc_write_ARM_INT_RAWSTAT);

void fh_pmu_arxc_reset(unsigned long physical_address)
{
	unsigned int arc_address;

	fh_pmu_set_reg(REG_PMU_ARC_RESET, ~FH8626V100_ARC_RESET_MASK);
	arc_address = (physical_address << 16) | (physical_address >> 16);
	fh_pmu_set_reg(REG_PMU_A625BOOT0, 0x7940266b);
	fh_pmu_set_reg(REG_PMU_A625BOOT1, arc_address);
	fh_pmu_set_reg(REG_PMU_A625BOOT2, 0x0f802020);
	fh_pmu_set_reg(REG_PMU_A625BOOT3, arc_address);
	fh_pmu_arxc_write_ARM_INT_RAWSTAT(0);
	fh_pmu_set_reg(REG_PMU_A625_START_CTRL, 0);
	udelay(2);
	fh_pmu_set_reg(REG_PMU_ARC_RESET, 0xffffffff);
}
EXPORT_SYMBOL_GPL(fh_pmu_arxc_reset);

void fh_pmu_arxc_kickoff(void)
{
	fh_pmu_set_reg(REG_PMU_A625_START_CTRL, 0x10);
}
EXPORT_SYMBOL_GPL(fh_pmu_arxc_kickoff);

void fh_pmu_usb_phy_rst(void)
{
	unsigned int value;

	value = fh_pmu_get_reg(REG_PMU_USB_SYS);
	value &= ~FH8626V100_USB_PHY_IDDQ_MASK;
	fh_pmu_set_reg(REG_PMU_USB_SYS, value);
	mdelay(1);

	value = fh_pmu_get_reg(REG_PMU_USB_SYS);
	value |= FH8626V100_USB_PHY_RESET_MASK;
	fh_pmu_set_reg(REG_PMU_USB_SYS, value);
	mdelay(1);

	value = fh_pmu_get_reg(REG_PMU_USB_SYS);
	value &= ~FH8626V100_USB_PHY_RESET_MASK;
	fh_pmu_set_reg(REG_PMU_USB_SYS, value);
}
EXPORT_SYMBOL_GPL(fh_pmu_usb_phy_rst);

void fh_pmu_usb_utmi_rst(void)
{
	unsigned int value;

	value = fh_pmu_get_reg(REG_PMU_SWRST_APB_CTRL);
	value &= ~FH8626V100_USB_UTMI_RESET_MASK;
	fh_pmu_set_reg(REG_PMU_SWRST_APB_CTRL, value);
	mdelay(1);

	value = fh_pmu_get_reg(REG_PMU_SWRST_APB_CTRL);
	value |= FH8626V100_USB_UTMI_RESET_MASK;
	fh_pmu_set_reg(REG_PMU_SWRST_APB_CTRL, value);
	msleep(20);
}
EXPORT_SYMBOL_GPL(fh_pmu_usb_utmi_rst);

void fh_pmu_usb_resume(void)
{
	unsigned int value;

	value = fh_pmu_get_reg(REG_PMU_USB_SYS);
	value |= FH8626V100_USB_SLEEP_MODE_MASK;
	fh_pmu_set_reg(REG_PMU_USB_SYS, value);
	mdelay(1);
}
EXPORT_SYMBOL_GPL(fh_pmu_usb_resume);
