/*
 * arch/arm/mach-anycloud/ak_adc1.c - ak ADC1 operation API
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/spinlock.h>

#include <mach/ak_adc1.h>

static DEFINE_SPINLOCK(adc1_lock);

/*
 * config saradc clk
 * div: SARADC CLK = 12MHz/(div+1); div is always 2 in practice.
 */
static void adc1_clk_cfg(unsigned long div)
{
	unsigned long saradc_chief_driven, saradc_module_driven;
	u32 val;

	val = __raw_readl(SAR_IF_CFG_REG);
	saradc_chief_driven = val & (0x1 << 0);
	saradc_module_driven = val & (0x7 << 5);

	/* disable module chief driven by sar adc clk */
	val = __raw_readl(SAR_IF_CFG_REG);
	val &= ~(1 << 0);
	__raw_writel(val, SAR_IF_CFG_REG);

	/* disable Ain0_sampling Ain1_sampling Bat_sampling */
	val = __raw_readl(SAR_IF_CFG_REG);
	val &= ~(0x7 << 5);
	__raw_writel(val, SAR_IF_CFG_REG);

	/* close sar adc clk */
	val = __raw_readl(AD_DA_CLK1_REG);
	val &= ~(0x1 << 3);
	__raw_writel(val, AD_DA_CLK1_REG);

	/* config div */
	val = __raw_readl(AD_DA_CLK1_REG);
	val &= ~0x7;
	val |= (div & 0x7);
	__raw_writel(val, AD_DA_CLK1_REG);

	/* open sar adc clk */
	val = __raw_readl(AD_DA_CLK1_REG);
	val |= (0x1 << 3);
	__raw_writel(val, AD_DA_CLK1_REG);

	/* restore the driven cfg */
	val = __raw_readl(SAR_IF_CFG_REG);
	val |= (saradc_chief_driven | saradc_module_driven);
	__raw_writel(val, SAR_IF_CFG_REG);
}

static void power_on_adc1(void)
{
	u32 val = __raw_readl(AD_DA_CLK1_REG);

	val &= ~(1 << 31);
	__raw_writel(val, AD_DA_CLK1_REG);
}

static void power_off_adc1(void)
{
	u32 val = __raw_readl(AD_DA_CLK1_REG);

	val |= (1 << 31);
	__raw_writel(val, AD_DA_CLK1_REG);
}

static void enable_adc1_channel(int channel)
{
	u32 val = __raw_readl(SAR_IF_CFG_REG);

	val &= ~(1 << 0);
	val |= (1 << (channel + 5));
	val |= (1 << 0);
	__raw_writel(val, SAR_IF_CFG_REG);
}

static void disable_adc1_channel(int channel)
{
	u32 val = __raw_readl(SAR_IF_CFG_REG);

	val &= ~(1 << 0);
	val &= ~(1 << (channel + 5));
	val |= (1 << 0);
	__raw_writel(val, SAR_IF_CFG_REG);
}

/*
 * Read AD0/AD1/BAT voltage, mV. Not IRQ-safe (uses mdelay()).
 */
unsigned long adc1_read_channel(int channel)
{
	int count;
	unsigned long val = 0;
	unsigned long flags;

	spin_lock_irqsave(&adc1_lock, flags);

	power_on_adc1();
	enable_adc1_channel(channel);
	mdelay(1);

	val = (__raw_readl(SAR_IF_SMP_DAT_REG) >> (channel * 10)) & 0x3ff;
	if (channel == AK_ADC1_BAT) {
		for (count = 0; count < 4; count++) {
			mdelay(1);
			val += (__raw_readl(SAR_IF_SMP_DAT_REG) >> (channel * 10)) & 0x3ff;
		}
		val /= 5;
	}
	val = (val * AK_AVCC) >> 10;

	disable_adc1_channel(channel);
	power_off_adc1();

	spin_unlock_irqrestore(&adc1_lock, flags);

	return val;
}
EXPORT_SYMBOL(adc1_read_channel);

/*
 * ADC1 initialization: sets up clock/sample-rate, leaves ADC1 powered off.
 * Each consumer (ADC keys, battery, the IIO driver) calls it at probe, so it
 * takes the readers' lock: a reset in the middle of another consumer's
 * sample would hand that consumer a wrong value.
 */
int adc1_init(void)
{
	unsigned long samplerate, adc1_clk, clkdiv;
	unsigned long spl_cycle, spl_hold, spl_wait;
	unsigned long flags;
	u32 val;

	spin_lock_irqsave(&adc1_lock, flags);

	/* reset adc1 */
	val = __raw_readl(RESET_CTRL_REG);
	val &= ~(1 << 30);
	__raw_writel(val, RESET_CTRL_REG);

	/* config adc1 clk, default 1.5MHz (BAT channel only works there) */
	clkdiv = (ADC1_MAIN_CLK / ADC1_DEFAULT_CLK - 1) & 0x7;
	adc1_clk_cfg(clkdiv);

	/* release reset */
	val = __raw_readl(RESET_CTRL_REG);
	val |= (1 << 30);
	__raw_writel(val, RESET_CTRL_REG);

	/* disable all sample */
	val = __raw_readl(SAR_IF_CFG_REG);
	val &= ~((1 << 0) | (0x7 << 5));
	__raw_writel(val, SAR_IF_CFG_REG);

	/* clear all adc1 interrupt state */
	__raw_writel(0x0, SAR_IF_INT_STATUS_REG);

	/* mask all adc1 interrupt */
	val = __raw_readl(SAR_IF_CFG_REG);
	val &= ~(0xf << 1);
	__raw_writel(val, SAR_IF_CFG_REG);

	power_on_adc1();

	/* select AVCC */
	val = __raw_readl(SAR_ADC_CFG_REG);
	val &= ~((1 << 16) | (1 << 22));
	__raw_writel(val, SAR_ADC_CFG_REG);

	/* one channel one time, default samplerate is 5000 */
	adc1_clk = ADC1_MAIN_CLK / (clkdiv + 1);
	samplerate = DEFAULT_SAMPLE;
	spl_cycle = adc1_clk / samplerate;
	spl_wait = 1;
	spl_hold = spl_wait + 16 + 1;

	val = __raw_readl(SAR_IF_CFG_REG);
	val &= ~(0xff << 14);
	val |= (spl_wait << 14);
	__raw_writel(val, SAR_IF_CFG_REG);

	__raw_writel(0, SAR_TIMING_CFG_REG);
	__raw_writel(spl_cycle | (spl_hold << 16), SAR_TIMING_CFG_REG);

	val = __raw_readl(SAR_IF_CFG_REG);
	val &= ~(0x7 << 8);
	val |= (1 << 8);
	__raw_writel(val, SAR_IF_CFG_REG);

	/* disable the bat div ratio */
	val = __raw_readl(SAR_ADC_CFG_REG);
	val &= ~(1 << 1);
	__raw_writel(val, SAR_ADC_CFG_REG);

	/* leave powered off; readers power on/off around each sample */
	power_off_adc1();

	spin_unlock_irqrestore(&adc1_lock, flags);
	return 0;
}
EXPORT_SYMBOL(adc1_init);
