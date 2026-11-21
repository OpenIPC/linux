/*
 * arch/arm/mach-anycloud/ak_adc1.c - ak ADC1 operation API
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>

#include <mach/ak_adc1.h>

static DEFINE_SPINLOCK(adc1_lock);

/*
 * Set under adc1_lock once the EV300 reader owns the converter (see
 * adc1_ev300_init()). The EV330 reader below reads 0 on that silicon, and
 * its register writes -- channel enables, power, a reset in adc1_init() --
 * would land in the middle of an EV300 conversion, so from then on it
 * leaves the hardware alone.
 */
static bool adc1_ev300_owned;

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
	if (adc1_ev300_owned) {
		spin_unlock_irqrestore(&adc1_lock, flags);
		pr_warn_once("ak_adc1: the EV330 reader does not work on an "
			     "EV300; read the IIO device instead\n");
		return 0;
	}

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
	if (adc1_ev300_owned) {
		spin_unlock_irqrestore(&adc1_lock, flags);
		return -EBUSY;
	}

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

/*
 * AK3918EV300.
 *
 * The EV330 reader above reads 0 mV on an EV300: on this silicon the
 * sample is not left in SAR_IF_SMP_DAT continuously, and the inputs have
 * an analog enable the EV330 code never touches. The sequence below is the
 * one a PE3020-W owner traced and tested on the camera (OpenIPC/builder#234),
 * reading 2999 mV lit and 38-42 mV with the photocell covered, as the stock
 * firmware's /sys/kernel/ain/ain0 does:
 *
 *  AD_DA_CLK1     +0x0C  bits 2:0 divider (7: 12 MHz / 8 = 1.5 MHz), bit 3
 *                        the converter clock
 *  RESET_CTRL     +0x20  bit 30 releases the converter
 *  SAR_IF_CFG     +0x5C  bit 0 starts a conversion, bits 7:5 select the
 *                        input (bit 5 AIN0), bits 10:8 format (1), bits
 *                        21:14 wait (1)
 *  SAR_TIMING     +0x60  1500 | 18 << 16
 *  SAR_IF_SMP_DAT +0x68  bits 11:0 the sample
 *  SAR_IF_STATUS  +0x6C  bit 0 ready; written 0 to clear
 *  ANALOG_CTRL2   +0xA0  bit 0 powers the analog block down, bit 3 off
 *                        selects the 3.0 V reference, bit 5 enables AIN0
 *
 * Register read-modify-writes take adc1_lock, as the EV330 path does; a
 * conversion holds adc1_ev300_mutex across its poll, which sleeps. Once
 * initialised, this path owns the converter: the EV330 reader and its
 * adc1_init() stand back (adc1_ev300_owned), and AD_DA_CLK1 bit 31, which
 * the EV330 reader closes after each sample, is opened before every
 * conversion.
 */
#define EV300_CLK_DIV_MASK	0x7
#define EV300_CLK_POWER_OFF	(1 << 31)	/* the EV330 reader's gate */
#define EV300_CLK_EN		(1 << 3)
#define EV300_ADC_RESET		(1 << 30)
#define EV300_CFG_TRIGGER	(1 << 0)
#define EV300_CFG_IRQ_MASK	(0xf << 1)
#define EV300_CFG_CHANNEL_MASK	(0x7 << 5)
#define EV300_CFG_AIN0		(1 << 5)
#define EV300_CFG_FORMAT_MASK	(0x7 << 8)
#define EV300_CFG_WAIT_MASK	(0xff << 14)
#define EV300_CFG_IDLE		((1 << 8) | (1 << 14))
#define EV300_TIMING_MASK	0x00ffffff
#define EV300_TIMING		(1500 | (18 << 16))
#define EV300_STATUS_READY	(1 << 0)
#define EV300_ACR_POWER_DOWN	(1 << 0)
#define EV300_ACR_REF_EXT	(1 << 3)
#define EV300_ACR_AIN0		(1 << 5)
#define EV300_POLL_US		250
#define EV300_TIMEOUT_US	50000

/* AK_ADDR() is a pointer on some machines and an integer on others. */
#define EV300_REG(r)		((void __iomem *)(r))

static DEFINE_MUTEX(adc1_ev300_mutex);

static void ev300_update(void __iomem *reg, u32 mask, u32 val)
{
	unsigned long flags;
	u32 v;

	spin_lock_irqsave(&adc1_lock, flags);
	v = __raw_readl(reg);
	__raw_writel((v & ~mask) | (val & mask), reg);
	spin_unlock_irqrestore(&adc1_lock, flags);
}

static void ev300_idle(void)
{
	ev300_update(EV300_REG(SAR_IF_CFG_REG), EV300_CFG_TRIGGER | EV300_CFG_CHANNEL_MASK,
		     0);
	ev300_update(EV300_REG(SAR_ANALOG_CTRL2_REG),
		     EV300_ACR_POWER_DOWN | EV300_ACR_AIN0, EV300_ACR_POWER_DOWN);
	ev300_update(EV300_REG(SAR_IF_INT_STATUS_REG), EV300_STATUS_READY, 0);
}

int adc1_ev300_init(void)
{
	unsigned long flags;

	mutex_lock(&adc1_ev300_mutex);
	spin_lock_irqsave(&adc1_lock, flags);
	adc1_ev300_owned = true;
	spin_unlock_irqrestore(&adc1_lock, flags);
	/* An EV330-style consumer that probed first leaves the gate shut. */
	ev300_update(EV300_REG(AD_DA_CLK1_REG), EV300_CLK_POWER_OFF, 0);
	/* The clock: off, the 1.5 MHz divider, on. */
	ev300_update(EV300_REG(AD_DA_CLK1_REG), EV300_CLK_EN, 0);
	ev300_update(EV300_REG(AD_DA_CLK1_REG), EV300_CLK_DIV_MASK, EV300_CLK_DIV_MASK);
	ev300_update(EV300_REG(AD_DA_CLK1_REG), EV300_CLK_EN, EV300_CLK_EN);
	ev300_update(EV300_REG(RESET_CTRL_REG), EV300_ADC_RESET, EV300_ADC_RESET);
	ev300_update(EV300_REG(SAR_TIMING_CFG_REG), EV300_TIMING_MASK, EV300_TIMING);
	ev300_update(EV300_REG(SAR_IF_CFG_REG),
		     EV300_CFG_TRIGGER | EV300_CFG_IRQ_MASK |
		     EV300_CFG_CHANNEL_MASK | EV300_CFG_FORMAT_MASK |
		     EV300_CFG_WAIT_MASK, EV300_CFG_IDLE);
	/* Powered down between conversions, on the 3.0 V reference. */
	ev300_update(EV300_REG(SAR_ANALOG_CTRL2_REG),
		     EV300_ACR_POWER_DOWN | EV300_ACR_REF_EXT | EV300_ACR_AIN0,
		     EV300_ACR_POWER_DOWN);
	ev300_update(EV300_REG(SAR_IF_INT_STATUS_REG), EV300_STATUS_READY, 0);
	mutex_unlock(&adc1_ev300_mutex);
	return 0;
}
EXPORT_SYMBOL(adc1_ev300_init);

int adc1_ev300_read(int channel, unsigned int *millivolts)
{
	unsigned int waited = 0;
	int ret = 0;
	u32 raw;

	if (channel != AK_ADC1_AD0 || !millivolts)
		return -EINVAL;

	mutex_lock(&adc1_ev300_mutex);
	ev300_update(EV300_REG(AD_DA_CLK1_REG), EV300_CLK_POWER_OFF, 0);
	ev300_update(EV300_REG(SAR_ANALOG_CTRL2_REG),
		     EV300_ACR_POWER_DOWN | EV300_ACR_AIN0, EV300_ACR_AIN0);
	ev300_update(EV300_REG(SAR_IF_CFG_REG), EV300_CFG_CHANNEL_MASK, EV300_CFG_AIN0);
	/* A ready left from before would be read as this conversion's. */
	ev300_update(EV300_REG(SAR_IF_INT_STATUS_REG), EV300_STATUS_READY, 0);
	ev300_update(EV300_REG(SAR_IF_CFG_REG), EV300_CFG_TRIGGER, EV300_CFG_TRIGGER);

	while (!(__raw_readl(SAR_IF_INT_STATUS_REG) & EV300_STATUS_READY)) {
		if (waited >= EV300_TIMEOUT_US) {
			ret = -ETIMEDOUT;
			break;
		}
		usleep_range(EV300_POLL_US, EV300_POLL_US + 50);
		waited += EV300_POLL_US;
	}
	raw = __raw_readl(SAR_IF_SMP_DAT_REG) & 0xfff;
	/* Down again whatever happened, so the next read starts clean. */
	ev300_idle();
	mutex_unlock(&adc1_ev300_mutex);

	if (!ret)
		*millivolts = raw * AK_ADC1_EV300_VREF / 4096;
	return ret;
}
EXPORT_SYMBOL(adc1_ev300_read);
