/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include <linux/bcd.h>
#include <linux/bitops.h>
#include <linux/log2.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rtc.h>
#include <linux/version.h>
#include <linux/io.h>
#include <linux/delay.h>

#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

union u_spi_rw {
	struct {
		unsigned int spi_wdata : 8; /* [7:0] */
		unsigned int spi_rdata : 8; /* [15:8] */
		unsigned int spi_addr : 7; /* [22:16] */
		unsigned int spi_rw : 1; /* [23] */
		unsigned int spi_start : 1; /* [24] */
		unsigned int reserved : 6; /* [30:25] */
		unsigned int spi_busy : 1; /* [31] */
	} bits;
	unsigned int u32;
};

struct xmedia_time_str {
	unsigned char dayl;
	unsigned char dayh;
	unsigned char second;
	unsigned char minute;
	unsigned char hour;
};

#define SPI_CLK_DIV 0x000
#define SPI_RW 0x004

#define SPI_WRITE		0
#define SPI_READ		1


#if defined(CONFIG_ARCH_XM72050200) || defined(CONFIG_ARCH_XM72050300) || \
    defined(CONFIG_ARCH_XM72020300) || defined(CONFIG_ARCH_XM76050100) || \
    defined(CONFIG_ARCH_XM72050500) || defined(CONFIG_ARCH_XM720XXX)

#define SPI_RTC_TYPE

/* RTC REG */
#define RTC_10MS_COUN	0x00
#define RTC_S_COUNT		0x01
#define RTC_M_COUNT		0x02
#define RTC_H_COUNT		0x03
#define RTC_D_COUNT_L	0x04
#define RTC_D_COUNT_H	0x05

#define RTC_MR_10MS		0x06
#define RTC_MR_S		0x07
#define RTC_MR_M		0x08
#define RTC_MR_H		0x09
#define RTC_MR_D_L		0x0A
#define RTC_MR_D_H		0x0B

#define RTC_LR_10MS		0x0C
#define RTC_LR_S		0x0D
#define RTC_LR_M		0x0E
#define RTC_LR_H		0x0F
#define RTC_LR_D_L		0x10
#define RTC_LR_D_H		0x11

#define RTC_LORD		0x12

#define RTC_IMSC		0x13
#define RTC_INT_CLR		0x14
#define RTC_INT			0x15
#define RTC_INT_RAW		0x16

#define RTC_CLK			0x17
#define RTC_POR_N		0x18
#define RTC_SAR_CTRL	0x1A
#define RTC_CLK_CFG	    0x1B

#define RTC_FREQ_H		0x51
#define RTC_FREQ_L		0x52

#define RTC_REG_LOCK1	0x64
#define RTC_REG_LOCK2	0x65
#define RTC_REG_LOCK3	0x66
#define RTC_REG_LOCK4	0x67
#endif

#define FREQ_H_DEFAULT  0x8
#define FREQ_L_DEFAULT  0x1B

#define LV_CTL_DEFAULT  0x20
#define CLK_DIV_DEFAULT 0x4
#define INT_RST_DEFAULT 0x0
#define INT_MSK_DEFAULT 0x4

#define AIE_INT_MASK       BIT(0)
#define LV_INT_MASK        BIT(1)
#define REG_LOAD_STAT      BIT(0)
#define REG_LOCK_STAT      BIT(1)
#define REG_LOCK_BYPASS    BIT(2)

#define RTC_RW_RETRY_CNT    5
#define SPI_RW_RETRY_CNT    500
#define RTC_SLEEP_TIME_MS   20

#define date_to_sec(d, h, m, s)     ((s) + (m) * 60 + (h) * 60 * 60 + (d) * 24 * 60 * 60)
#define sec_to_day(s)            ((s) / (60 * 60 * 24))

struct xmedia_rtc {
	struct rtc_device *rtc_dev;
	void __iomem *regs;
	int                  rtc_irq;
};
#define xmedia_rtc_readl(x) (*((volatile unsigned char *)(x)))
#define xmedia_rtc_writel(v, x) (*((volatile unsigned char *)(x)) = (v))

#if defined(SPI_RTC_TYPE)
static int xmedia_spi_write(void *spi_reg, unsigned char reg,
			   unsigned char val)
{
	union u_spi_rw w_data, r_data;
	int cnt = SPI_RW_RETRY_CNT;

	r_data.u32 = 0;
	w_data.u32 = 0;

	w_data.bits.spi_wdata = val;
	w_data.bits.spi_addr = reg;
	w_data.bits.spi_rw = SPI_WRITE;
	w_data.bits.spi_start = 0x1;

	writel(w_data.u32, (spi_reg+SPI_RW));

	do
		r_data.u32 = readl(spi_reg+SPI_RW);
	while (r_data.bits.spi_busy && (--cnt));

	if (r_data.bits.spi_busy)
		return -EIO;

	return 0;
}

static int xmedia_spi_read(void *spi_reg, unsigned char reg,
			  unsigned char *val)
{
	union u_spi_rw w_data, r_data;
	int cnt = SPI_RW_RETRY_CNT;

	r_data.u32 = 0;
	w_data.u32 = 0;
	w_data.bits.spi_addr = reg;
	w_data.bits.spi_rw = SPI_READ;
	w_data.bits.spi_start = 0x1;

	writel(w_data.u32, (spi_reg+SPI_RW));

	do
		r_data.u32 = readl(spi_reg+SPI_RW);
	while (r_data.bits.spi_busy && (--cnt));

	if (r_data.bits.spi_busy)
		return -EIO;

	*val = r_data.bits.spi_rdata;

	return 0;
}
#else
static unsigned int xmedia_write_reg(void *spi_reg, unsigned long offset,
				    unsigned char val)
{
	xmedia_rtc_writel(val, (spi_reg + offset));
	return 0;
}

static unsigned int xmedia_read_reg(void *spi_reg, unsigned long offset,
				   unsigned char *val)
{
	*val = xmedia_rtc_readl(spi_reg + offset);
	return 0;
}
#endif


static unsigned int xmedia_rtc_write(void *spi_reg, unsigned long offset,
				    unsigned char val)
{
#if defined(SPI_RTC_TYPE)
	return xmedia_spi_write(spi_reg, offset, val);
#else
	return xmedia_write_reg(spi_reg, offset, val);
#endif
}

static unsigned int xmedia_rtc_read(void *spi_reg, unsigned long offset,
				   unsigned char *val)
{
#if defined(SPI_RTC_TYPE)
	return xmedia_spi_read(spi_reg, offset, val);
#else
	return xmedia_read_reg(spi_reg, offset, val);
#endif
}

static int xmedia_rtc_read_time(struct device *dev, struct rtc_time *time)
{
	struct xmedia_rtc *rtc = dev_get_drvdata(dev);
	struct xmedia_time_str time_str = {0};
	time64_t seconds = 0;
	unsigned int day;
	unsigned char raw_value = 0;
	int cnt = RTC_RW_RETRY_CNT;

	xmedia_rtc_read(rtc->regs, RTC_INT_RAW, &raw_value);

	if (raw_value & LV_INT_MASK)
		/* low voltage detected, date/time is not reliable. */
		xmedia_rtc_write(rtc->regs, RTC_INT_CLR, 1);

	xmedia_rtc_read(rtc->regs, RTC_LORD, &raw_value);
	if (raw_value & REG_LOCK_BYPASS)
		xmedia_rtc_write(rtc->regs, RTC_LORD,
				(~(REG_LOCK_BYPASS)) & raw_value);

	xmedia_rtc_read(rtc->regs, RTC_LORD, &raw_value);
	/* lock the time */
	xmedia_rtc_write(rtc->regs, RTC_LORD, (REG_LOCK_STAT) | raw_value);
	/* wait rtc load flag */
	do {
		xmedia_rtc_read(rtc->regs, RTC_LORD, &raw_value);
		msleep(RTC_SLEEP_TIME_MS);
	} while ((raw_value & REG_LOCK_STAT) && (--cnt));

	if (raw_value & REG_LOCK_STAT)
		return -EBUSY;

	xmedia_rtc_read(rtc->regs, RTC_S_COUNT, &time_str.second);
	xmedia_rtc_read(rtc->regs, RTC_M_COUNT, &time_str.minute);
	xmedia_rtc_read(rtc->regs, RTC_H_COUNT, &time_str.hour);
	xmedia_rtc_read(rtc->regs, RTC_D_COUNT_L, &time_str.dayl);
	xmedia_rtc_read(rtc->regs, RTC_D_COUNT_H, &time_str.dayh);

	day = (time_str.dayl | (time_str.dayh << 8)); /* Move to a high 8 bit. */
	seconds = date_to_sec(day, time_str.hour, time_str.minute, time_str.second);

	rtc_time64_to_tm(seconds, time);

	return rtc_valid_tm(time);
}

static int xmedia_rtc_set_time(struct device *dev, struct rtc_time *time)
{
	struct xmedia_rtc *rtc = dev_get_drvdata(dev);
	int days;
	time64_t seconds = 0;
	unsigned int cnt = RTC_RW_RETRY_CNT;
	unsigned char raw_value = 0;

	seconds = rtc_tm_to_time64(time);

	days = div_s64(seconds, (60 * 60 * 24));

	xmedia_rtc_write(rtc->regs, RTC_LR_10MS, 0);
	xmedia_rtc_write(rtc->regs, RTC_LR_S, time->tm_sec);
	xmedia_rtc_write(rtc->regs, RTC_LR_M, time->tm_min);
	xmedia_rtc_write(rtc->regs, RTC_LR_H, time->tm_hour);
	xmedia_rtc_write(rtc->regs, RTC_LR_D_L, (days & 0xFF));
	xmedia_rtc_write(rtc->regs, RTC_LR_D_H, (days >> 8));  /* Move to a Low 8 bit. */

	xmedia_rtc_write(rtc->regs, RTC_LORD,
			(raw_value | REG_LOAD_STAT));
	/* wait rtc load flag */
	do {
		xmedia_rtc_read(rtc->regs, RTC_LORD, &raw_value);
		msleep(RTC_SLEEP_TIME_MS);
	} while ((raw_value & REG_LOAD_STAT) && (--cnt));

	if (raw_value & REG_LOAD_STAT)
		return -EBUSY;

	return 0;
}

static int xmedia_rtc_read_alarm(struct device *dev,
				struct rtc_wkalrm *alrm)
{
	struct xmedia_rtc *rtc = dev_get_drvdata(dev);
	struct xmedia_time_str time_str = {0};
	time64_t seconds = 0;
	unsigned int day;
	unsigned char int_state = 0;

	memset(alrm, 0, sizeof(struct rtc_wkalrm));

	xmedia_rtc_read(rtc->regs, RTC_MR_S, &time_str.second);
	xmedia_rtc_read(rtc->regs, RTC_MR_M, &time_str.minute);
	xmedia_rtc_read(rtc->regs, RTC_MR_H, &time_str.hour);
	xmedia_rtc_read(rtc->regs, RTC_MR_D_L, &time_str.dayl);
	xmedia_rtc_read(rtc->regs, RTC_MR_D_H, &time_str.dayh);

	day = (unsigned int)(time_str.dayl | (time_str.dayh << 8)); /* Move to a high 8 bit. */
	seconds = date_to_sec(day, time_str.hour, time_str.minute, time_str.second);

	rtc_time64_to_tm(seconds, &alrm->time);

	xmedia_rtc_read(rtc->regs, RTC_IMSC, &int_state);

	alrm->enabled = !!(int_state & AIE_INT_MASK);
	alrm->pending = alrm->enabled;

	return 0;
}

static int xmedia_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct xmedia_rtc *rtc = dev_get_drvdata(dev);
	int days;
	time64_t seconds = 0;
	unsigned char val = 0;

	seconds = rtc_tm_to_time64(&alrm->time);

	days = div_s64(seconds, (60 * 60 * 24));

	xmedia_rtc_write(rtc->regs, RTC_MR_10MS, 0);
	xmedia_rtc_write(rtc->regs, RTC_MR_S, alrm->time.tm_sec);
	xmedia_rtc_write(rtc->regs, RTC_MR_M, alrm->time.tm_min);
	xmedia_rtc_write(rtc->regs, RTC_MR_H, alrm->time.tm_hour);
	xmedia_rtc_write(rtc->regs, RTC_MR_D_L, (days & 0xFF));
	xmedia_rtc_write(rtc->regs, RTC_MR_D_H, (days >> 8)); /* Move to a Low 8 bit. */

	xmedia_rtc_read(rtc->regs, RTC_IMSC, &val);
	if (alrm->enabled)
		xmedia_rtc_write(rtc->regs, RTC_IMSC, val | AIE_INT_MASK);
	else
		xmedia_rtc_write(rtc->regs, RTC_IMSC, val & ~AIE_INT_MASK);

	return 0;
}

static int xmedia_rtc_alarm_irq_enable(struct device *dev,
				      unsigned int enabled)
{
	struct xmedia_rtc *rtc = dev_get_drvdata(dev);
	unsigned char val = 0;

	xmedia_rtc_read(rtc->regs, RTC_IMSC, &val);
	if (enabled)
		xmedia_rtc_write(rtc->regs, RTC_IMSC, val | AIE_INT_MASK);
	else
		xmedia_rtc_write(rtc->regs, RTC_IMSC, val & ~AIE_INT_MASK);

	return 0;
}

/*
 * interrupt function
 * do nothing. left for future
 */
static irqreturn_t xmedia_rtc_alm_interrupt(int irq, void *data)
{
	struct xmedia_rtc *rtc = (struct xmedia_rtc *)data;
	unsigned char val = 0;

	xmedia_rtc_read(rtc->regs, RTC_INT, &val);
	xmedia_rtc_write(rtc->regs, RTC_INT_CLR, AIE_INT_MASK);

	if (val & AIE_INT_MASK)
		rtc_update_irq(rtc->rtc_dev, 1, RTC_AF | RTC_IRQF);

	return IRQ_HANDLED;
}

#define FREQ_MAX_VAL	    3277000
#define FREQ_MIN_VAL	    3276000

static int xmedia_rtc_ioctl(struct device *dev,
			   unsigned int cmd, unsigned long arg)
{
	struct xmedia_rtc *rtc = dev_get_drvdata(dev);

	switch (cmd) {
	case RTC_PLL_SET: {
		char freq_l, freq_h;
		struct rtc_pll_info pll_info = {0};

		if (copy_from_user(&pll_info, (struct rtc_pll_info *)(uintptr_t)arg,
				   sizeof(struct rtc_pll_info)))
			return -EFAULT;

		/* freq = 32700 + (freq /3052)*100 */
		if (pll_info.pll_value > FREQ_MAX_VAL ||
			pll_info.pll_value < FREQ_MIN_VAL)
			return -EINVAL;

		/* freq convert: (freq-3270000) * 3052 / 10000 */
		pll_info.pll_value = (pll_info.pll_value - 3270000) *
				     3052 / 10000;

		/* & 0xff Obtains the lower eight bits of data. */
		freq_l = (char)(pll_info.pll_value & 0xff);
		/* pll_info.pll_value >> 8 & 0xf  Obtains the last four bits of the higher eight bits. */
		freq_h = (char)((pll_info.pll_value >> 8) & 0xf);

		xmedia_rtc_write(rtc->regs, RTC_FREQ_H, freq_h);
		xmedia_rtc_write(rtc->regs, RTC_FREQ_L, freq_l);

		return 0;
	}
	case RTC_PLL_GET: {
		char freq_l = 0;
		char freq_h = 0;
		struct rtc_pll_info pll_info = {0};

		xmedia_rtc_read(rtc->regs, RTC_FREQ_H, &freq_h);
		xmedia_rtc_read(rtc->regs, RTC_FREQ_L, &freq_l);

		if ((void __user *)(uintptr_t)arg == NULL) {
			dev_err(dev, "IO err or user buf is NULL..\n");
			return -1;
		}

		/* freq_h & 0xf << 8 :Shifts leftwards by 8 bits and obtains the lower 4 bits. */
		pll_info.pll_value = (((unsigned)freq_h & 0xf) << 8) + freq_l;

		/* freq convert: 3270000 + (freq * 10000) / 3052 */
		pll_info.pll_value = 3270000 + ((unsigned int)pll_info.pll_value * 10000) / 3052;

		pll_info.pll_max = FREQ_MAX_VAL;
		pll_info.pll_min = FREQ_MIN_VAL;
		if (copy_to_user((void __user *)(uintptr_t)arg,
				 &pll_info, sizeof(struct rtc_pll_info)))
			return -EFAULT;

		return 0;
	}
	default:
		return -ENOIOCTLCMD;
	}
}

static const struct rtc_class_ops xmedia_rtc_ops = {
	.read_time = xmedia_rtc_read_time,
	.set_time = xmedia_rtc_set_time,
	.read_alarm = xmedia_rtc_read_alarm,
	.set_alarm = xmedia_rtc_set_alarm,
	.alarm_irq_enable = xmedia_rtc_alarm_irq_enable,
	.ioctl = xmedia_rtc_ioctl,
};

static int xmedia_rtc_init(struct xmedia_rtc *rtc)
{
	void *spi_reg = rtc->regs;
	unsigned char val = 0;

	/*
	 * clk div value = (apb_clk/spi_clk)/2-1,
	 *  apb clk = 100MHz, spi_clk = 10MHz,so value= 0x4
	 */
	writel(CLK_DIV_DEFAULT, (spi_reg + SPI_CLK_DIV));

	xmedia_rtc_write(spi_reg, RTC_IMSC, INT_MSK_DEFAULT);
	xmedia_rtc_write(spi_reg, RTC_SAR_CTRL, LV_CTL_DEFAULT);

	/* default driver capability */
	xmedia_rtc_write(spi_reg, RTC_REG_LOCK4, 0x5A); /* 0x5A:ctl order */
	xmedia_rtc_write(spi_reg, RTC_REG_LOCK3, 0x5A); /* 0x5A:ctl order */
	xmedia_rtc_write(spi_reg, RTC_REG_LOCK2, 0xAB); /* 0xAB:ctl order */
	xmedia_rtc_write(spi_reg, RTC_REG_LOCK1, 0xCD); /* 0xCD:ctl order */

	xmedia_rtc_write(spi_reg, RTC_CLK_CFG, 0x03);

	/* default FREQ COEF */
	xmedia_rtc_write(spi_reg, RTC_FREQ_H, FREQ_H_DEFAULT);
	xmedia_rtc_write(spi_reg, RTC_FREQ_L, FREQ_L_DEFAULT);

	xmedia_rtc_read(spi_reg, RTC_INT_RAW, &val);

	if (val & LV_INT_MASK)
		/* low voltage detected, date/time is not reliable. */
		xmedia_rtc_write(rtc->regs, RTC_INT_CLR, 1);

	return 0;
}

static int xmedia_rtc_probe(struct platform_device *pdev)
{
	struct resource  *mem = NULL;
	struct xmedia_rtc *rtc = NULL;
	int    ret;

	rtc = devm_kzalloc(&pdev->dev, sizeof(*rtc), GFP_KERNEL);
	if (!rtc)
		return -ENOMEM;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	rtc->regs = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR((const void *)rtc->regs)) {
		dev_err(&pdev->dev, "could not map I/O memory\n");
		return PTR_ERR((const void *)rtc->regs);
	}

	rtc->rtc_irq = platform_get_irq(pdev, 0);
	ret = devm_request_irq(&pdev->dev, rtc->rtc_irq,
			       xmedia_rtc_alm_interrupt, 0, pdev->name, rtc);
	if (ret) {
		dev_err(&pdev->dev, "could not request irq %d\n", rtc->rtc_irq);
		return ret;
	}

	platform_set_drvdata(pdev, rtc);
	rtc->rtc_dev = devm_rtc_device_register(&pdev->dev, pdev->name,
						&xmedia_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc->rtc_dev)) {
		dev_err(&pdev->dev, "could not register rtc device\n");
		return PTR_ERR(rtc->rtc_dev);
	}

	if (xmedia_rtc_init(rtc)) {
		dev_err(&pdev->dev, "xmedia_rtc_init failed.\n");
		return -EIO;
	}

	dev_info(&pdev->dev, "RTC driver for xmedia enabled\n");

	return 0;
}

static int xmedia_rtc_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id xmedia_rtc_match[] = {
	{ .compatible = "xmedia,rtc" },
	{},
};

static struct platform_driver xmedia_rtc_driver = {
	.probe  = xmedia_rtc_probe,
	.remove = xmedia_rtc_remove,
	.driver =  {
		.name = "xmedia_rtc",
		.of_match_table = xmedia_rtc_match,
	},
};

module_platform_driver(xmedia_rtc_driver);

#define OSDRV_MODULE_VERSION_STRING "XMEDIA_rtc @XMEDIA"

MODULE_AUTHOR("Xmedia");
MODULE_DESCRIPTION("Xmedia RTC driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("XMEDIA_VERSION=" OSDRV_MODULE_VERSION_STRING);
