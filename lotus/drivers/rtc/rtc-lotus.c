// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) LOTUS. All rights reserved.
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

struct lotus_time_str {
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


#if defined(CONFIG_ARCH_XMORCA) || defined(CONFIG_ARCH_XMFALCON)

#define SPI_RTC_TYPE

/* RTC REG */
#define RTC_10MS_COUN	0x01
#define RTC_S_COUNT		0x02
#define RTC_M_COUNT		0x03
#define RTC_H_COUNT		0x04
#define RTC_D_COUNT_L	0x05
#define RTC_D_COUNT_H	0x06

#define RTC_MR_10MS		0x07
#define RTC_MR_S		0x08
#define RTC_MR_M		0x09
#define RTC_MR_H		0x0A
#define RTC_MR_D_L		0x0B
#define RTC_MR_D_H		0x0C

#define RTC_LR_10MS		0x0D
#define RTC_LR_S		0x0E
#define RTC_LR_M		0x0F
#define RTC_LR_H		0x10
#define RTC_LR_D_L		0x11
#define RTC_LR_D_H		0x12

#define RTC_LORD		0x13

#define RTC_IMSC		0x14
#define RTC_INT_CLR		0x15
#define RTC_INT			0x16
#define RTC_INT_RAW		0x17

#define RTC_CLK			0x18
#define RTC_POR_N		0x19
#define RTC_SAR_CTRL	0x1A
#define RTC_CLK_CFG	    0x1B
#define RTC_VOLT_AVDD		0x1C
#define RTC_VOLT_BG		0x1D

#define RTC_FREQ_H		0x50
#define RTC_FREQ_L		0x51

#define RTC_REG_LOCK1	0x64
#define RTC_REG_LOCK2	0x65
#define RTC_REG_LOCK3	0x66
#define RTC_REG_LOCK4	0x67
#endif

#define FREQ_H_DEFAULT  0x0
#define FREQ_L_DEFAULT  0x20

#define LV_CTL_DEFAULT  0x20
#define CLK_DIV_DEFAULT 0x4
#define INT_RST_DEFAULT 0x0
#define INT_MSK_DEFAULT 0x4

#define AIE_INT_MASK       BIT(0)
#define LV_INT_MASK        BIT(1)
#define REG_LOAD_STAT      BIT(0)
#define REG_LOCK_STAT      BIT(1)
#define REG_LOCK_BYPASS    BIT(2)

#define RTC_RW_RETRY_CNT    100*10
#define SPI_RW_RETRY_CNT    500
#define RTC_SLEEP_TIME_MS   1
#define RTC_SLEEP_TIME_US   100
#define RTC_WRITE_TIME_MS   15

#define date_to_sec(d, h, m, s)     ((s) + (m) * 60 + (h) * 60 * 60 + (d) * 24 * 60 * 60)
#define sec_to_day(s)            ((s) / (60 * 60 * 24))

#ifdef _RTC_AOV_DEBUG
void __iomem *rtc_reg_base;
#endif

struct lotus_rtc {
	struct rtc_device *rtc_dev;
	void __iomem *regs;
	int                  rtc_irq;
	unsigned int alarm_time;
	unsigned char alarm_enable;
	struct rtc_time resume_time;
	struct rtc_time suspend_time;
	unsigned char resume_ms;
	unsigned char suspend_ms;
	bool rtc_first_suspend;
};

#ifndef SPI_RTC_TYPE
#define lotus_rtc_readl(x) readl((x))
#define lotus_rtc_writel(v, x) writel((v), (x))
#endif

#if defined(SPI_RTC_TYPE)
static int lotus_spi_write(void *spi_reg, unsigned char reg,
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

	do {
		r_data.u32 = readl(spi_reg+SPI_RW);
		udelay(1);
	} while (r_data.bits.spi_busy && (--cnt));

	if (r_data.bits.spi_busy) {
		printk("rtc spi write failed\n");
		return -EIO;
	}
	return 0;
}

static int lotus_spi_read(void *spi_reg, unsigned char reg,
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

	do {
		r_data.u32 = readl(spi_reg+SPI_RW);
		udelay(1);
	} while (r_data.bits.spi_busy && (--cnt));

	if (r_data.bits.spi_busy) {
		printk("rtc spi read failed\n");
		return -EIO;
	}
	*val = r_data.bits.spi_rdata;

	return 0;
}
#else
static unsigned int lotus_write_reg(void *spi_reg, unsigned long offset,
				    unsigned char val)
{
	lotus_rtc_writel(val, (spi_reg + offset));
	return 0;
}

static unsigned int lotus_read_reg(void *spi_reg, unsigned long offset,
				   unsigned char *val)
{
	*val = lotus_rtc_readl(spi_reg + offset);
	return 0;
}
#endif


static unsigned int lotus_rtc_write(void *spi_reg, unsigned long offset,
				    unsigned char val)
{
#if defined(SPI_RTC_TYPE)
	return lotus_spi_write(spi_reg, offset, val);
#else
	return lotus_write_reg(spi_reg, offset, val);
#endif
}

static unsigned int lotus_rtc_read(void *spi_reg, unsigned long offset,
				   unsigned char *val)
{
#if defined(SPI_RTC_TYPE)
	return lotus_spi_read(spi_reg, offset, val);
#else
	return lotus_read_reg(spi_reg, offset, val);
#endif
}

int lotus_rtc_aov_read_time(struct device *dev, struct rtc_time *time, unsigned char *ms)
{
	struct lotus_rtc *rtc = dev_get_drvdata(dev);
	struct lotus_time_str time_str = {0};
	time64_t seconds = 0;
	unsigned int day;
	unsigned char raw_value = 0;
	int cnt = RTC_RW_RETRY_CNT;

	lotus_rtc_read(rtc->regs, RTC_INT_RAW, &raw_value);

	if (raw_value & LV_INT_MASK)
		/* low voltage detected, date/time is not reliable. */
		lotus_rtc_write(rtc->regs, RTC_INT_CLR, 1);

	lotus_rtc_read(rtc->regs, RTC_LORD, &raw_value);
	if (raw_value & REG_LOCK_BYPASS)
		lotus_rtc_write(rtc->regs, RTC_LORD,
				(~(REG_LOCK_BYPASS)) & raw_value);

	lotus_rtc_read(rtc->regs, RTC_LORD, &raw_value);
	/* lock the time */
	lotus_rtc_write(rtc->regs, RTC_LORD, (REG_LOCK_STAT) | raw_value);
	/* wait rtc load flag */
	do {
		udelay(RTC_SLEEP_TIME_US);
		lotus_rtc_read(rtc->regs, RTC_LORD, &raw_value);
	} while ((raw_value & REG_LOCK_STAT) && (--cnt));

	if (raw_value & REG_LOCK_STAT)
		return -EBUSY;

	lotus_rtc_read(rtc->regs, RTC_S_COUNT, &time_str.second);
	lotus_rtc_read(rtc->regs, RTC_M_COUNT, &time_str.minute);
	lotus_rtc_read(rtc->regs, RTC_H_COUNT, &time_str.hour);
	lotus_rtc_read(rtc->regs, RTC_D_COUNT_L, &time_str.dayl);
	lotus_rtc_read(rtc->regs, RTC_D_COUNT_H, &time_str.dayh);

	day = (time_str.dayl | (time_str.dayh << 8)); /* Move to a high 8 bit. */
	seconds = date_to_sec(day, time_str.hour, time_str.minute, time_str.second);

	rtc_time64_to_tm(seconds, time);
	if (ms) {
		lotus_rtc_read(rtc->regs,RTC_10MS_COUN, ms);
	}
#ifdef _RTC_AOV_DEBUG
	dev_info(dev, "AOV read time: %d%ddays-%dh:%dm:%ds.%dms\n", time_str.dayh, time_str.dayl, time_str.hour, time_str.minute, time_str.second, *ms * 10);
#endif
	return rtc_valid_tm(time);
}
EXPORT_SYMBOL(lotus_rtc_aov_read_time);

int lotus_rtc_aov_set_alarm(struct device *dev, struct rtc_wkalrm *alrm, unsigned char ms)
{
	struct lotus_rtc *rtc = dev_get_drvdata(dev);
	int days;
	time64_t seconds = 0;
	unsigned char val = 0;

	if (alrm->enabled) {
		lotus_rtc_read(rtc->regs, RTC_IMSC, &val);
		lotus_rtc_write(rtc->regs, RTC_IMSC, val & ~AIE_INT_MASK);
		lotus_rtc_write(rtc->regs, RTC_INT_CLR, AIE_INT_MASK);
	}

	seconds = rtc_tm_to_time64(&alrm->time);

	days = div_s64(seconds, (60 * 60 * 24));

	if (ms < 0 || ms > 99) {
		lotus_rtc_write(rtc->regs, RTC_MR_10MS, 0);
	} else {
		lotus_rtc_write(rtc->regs, RTC_MR_10MS, ms);
	}
	lotus_rtc_write(rtc->regs, RTC_MR_S, alrm->time.tm_sec);
	lotus_rtc_write(rtc->regs, RTC_MR_M, alrm->time.tm_min);
	lotus_rtc_write(rtc->regs, RTC_MR_H, alrm->time.tm_hour);
	lotus_rtc_write(rtc->regs, RTC_MR_D_L, (days & 0xFF));
	lotus_rtc_write(rtc->regs, RTC_MR_D_H, (days >> 8)); /* Move to a Low 8 bit. */

#ifdef _RTC_AOV_DEBUG
	if (alrm->enabled)
		dev_info(dev, "AOV alarm time(to set): %ddays-%dh:%dm:%ds.%dms\n",
			days, alrm->time.tm_hour, alrm->time.tm_min, alrm->time.tm_sec, ms * 10);
#endif

	lotus_rtc_read(rtc->regs, RTC_IMSC, &val);
	if (alrm->enabled) {
#ifdef _RTC_AOV_DEBUG
		unsigned char r_ms, r_sec, r_min, r_hour, r_dl, r_dh, val_imsc, val_int_masked, val_int_raw;
#endif
		lotus_rtc_write(rtc->regs, RTC_IMSC, val | AIE_INT_MASK);

#ifdef _RTC_AOV_DEBUG
		lotus_rtc_read(rtc->regs, RTC_IMSC, &val_imsc);
		lotus_rtc_read(rtc->regs, RTC_INT, &val_int_masked);
		lotus_rtc_read(rtc->regs, RTC_INT_RAW, &val_int_raw);
		lotus_rtc_read(rtc->regs, RTC_MR_10MS, &r_ms);
		lotus_rtc_read(rtc->regs, RTC_MR_S, &r_sec);
		lotus_rtc_read(rtc->regs, RTC_MR_M, &r_min);
		lotus_rtc_read(rtc->regs, RTC_MR_H, &r_hour);
		lotus_rtc_read(rtc->regs, RTC_MR_D_L, &r_dl);
		lotus_rtc_read(rtc->regs, RTC_MR_D_H, &r_dh);
		dev_info(dev, "AOV alarm time(set): %d%ddays-%dh:%dm:%ds.%dms\n",
				r_dh, r_dl, r_hour, r_min, r_sec, r_ms * 10);
		dev_info(dev, "RTC IMSC:0x%x, INT_MASKED: 0x%x, INT_RAW: 0x%x,\n",
				val_imsc, val_int_masked, val_int_raw);
#endif
	} else {
		lotus_rtc_write(rtc->regs, RTC_IMSC, val & ~AIE_INT_MASK);
	}
	return 0;
}
EXPORT_SYMBOL(lotus_rtc_aov_set_alarm);

// alarm_time:��λΪ10ms
int lotus_rtc_aov_set_alarm_time(struct device *dev, unsigned int alarm_time, unsigned char alarm_enable)
{
	struct lotus_rtc *rtc = dev_get_drvdata(dev);

	rtc->alarm_time = alarm_time;
	rtc->alarm_enable = alarm_enable;
	rtc->rtc_first_suspend = true;

	return 0;
}
EXPORT_SYMBOL(lotus_rtc_aov_set_alarm_time);

int lotus_rtc_aov_get_resume_time(struct device *dev, struct rtc_time *time, unsigned char *ms)
{
	struct lotus_rtc *rtc = dev_get_drvdata(dev);

	*time = rtc->resume_time;
	*ms = rtc->resume_ms;
	return 0;
}
EXPORT_SYMBOL(lotus_rtc_aov_get_resume_time);

int lotus_rtc_aov_get_suspend_time(struct device *dev, struct rtc_time *time, unsigned char *ms)
{
	struct lotus_rtc *rtc = dev_get_drvdata(dev);

	*time = rtc->suspend_time;
	*ms = rtc->suspend_ms;
	return 0;
}
EXPORT_SYMBOL(lotus_rtc_aov_get_suspend_time);

int lotus_rtc_aov_get_current_time(struct device *dev, struct rtc_time *time, unsigned char *ms)
{
	struct rtc_time linuxTime = {0};
	unsigned char cur_ms;
	int ret = 0;

	ret = lotus_rtc_aov_read_time(dev, &linuxTime, &cur_ms);
	if (ret != 0) {
		printk("get rtc current time failed\n");
		return 0;
	}

	*time = linuxTime;
	*ms = cur_ms;
	return 0;
}
EXPORT_SYMBOL(lotus_rtc_aov_get_current_time);

static int lotus_rtc_read_time(struct device *dev, struct rtc_time *time)
{
	struct lotus_rtc *rtc = dev_get_drvdata(dev);
	struct lotus_time_str time_str = {0};
	time64_t seconds = 0;
	unsigned int day;
	unsigned char raw_value = 0;
	int cnt = RTC_RW_RETRY_CNT;

	lotus_rtc_read(rtc->regs, RTC_INT_RAW, &raw_value);

	if (raw_value & LV_INT_MASK)
		/* low voltage detected, date/time is not reliable. */
		lotus_rtc_write(rtc->regs, RTC_INT_CLR, 1);

	lotus_rtc_read(rtc->regs, RTC_LORD, &raw_value);
	if (raw_value & REG_LOCK_BYPASS)
		lotus_rtc_write(rtc->regs, RTC_LORD,
				(~(REG_LOCK_BYPASS)) & raw_value);

	lotus_rtc_read(rtc->regs, RTC_LORD, &raw_value);
	/* lock the time */
	lotus_rtc_write(rtc->regs, RTC_LORD, (REG_LOCK_STAT) | raw_value);
	/* wait rtc load flag */
	do {
		udelay(RTC_SLEEP_TIME_US);
		lotus_rtc_read(rtc->regs, RTC_LORD, &raw_value);
	} while ((raw_value & REG_LOCK_STAT) && (--cnt));

	if (raw_value & REG_LOCK_STAT)
		return -EBUSY;

	lotus_rtc_read(rtc->regs, RTC_S_COUNT, &time_str.second);
	lotus_rtc_read(rtc->regs, RTC_M_COUNT, &time_str.minute);
	lotus_rtc_read(rtc->regs, RTC_H_COUNT, &time_str.hour);
	lotus_rtc_read(rtc->regs, RTC_D_COUNT_L, &time_str.dayl);
	lotus_rtc_read(rtc->regs, RTC_D_COUNT_H, &time_str.dayh);

	day = (time_str.dayl | (time_str.dayh << 8)); /* Move to a high 8 bit. */
	seconds = date_to_sec(day, time_str.hour, time_str.minute, time_str.second);

	rtc_time64_to_tm(seconds, time);

	return rtc_valid_tm(time);
}

static int lotus_rtc_set_time(struct device *dev, struct rtc_time *time)
{
	struct lotus_rtc *rtc = dev_get_drvdata(dev);
	int days;
	time64_t seconds = 0;
	unsigned int cnt = RTC_RW_RETRY_CNT;
	unsigned char raw_value = 0;

	seconds = rtc_tm_to_time64(time);

	days = div_s64(seconds, (60 * 60 * 24));

	lotus_rtc_write(rtc->regs, RTC_LR_10MS, 0);
	lotus_rtc_write(rtc->regs, RTC_LR_S, time->tm_sec);
	lotus_rtc_write(rtc->regs, RTC_LR_M, time->tm_min);
	lotus_rtc_write(rtc->regs, RTC_LR_H, time->tm_hour);
	lotus_rtc_write(rtc->regs, RTC_LR_D_L, (days & 0xFF));
	lotus_rtc_write(rtc->regs, RTC_LR_D_H, (days >> 8));  /* Move to a Low 8 bit. */

	lotus_rtc_write(rtc->regs, RTC_LORD,
			(raw_value | REG_LOAD_STAT));
	/* wait rtc load flag */
	do {
		udelay(RTC_SLEEP_TIME_US);
		lotus_rtc_read(rtc->regs, RTC_LORD, &raw_value);
	} while ((raw_value & REG_LOAD_STAT) && (--cnt));

	if (raw_value & REG_LOAD_STAT)
		return -EBUSY;

	return 0;
}

static int lotus_rtc_read_alarm(struct device *dev,
				struct rtc_wkalrm *alrm)
{
	struct lotus_rtc *rtc = dev_get_drvdata(dev);
	struct lotus_time_str time_str = {0};
	time64_t seconds = 0;
	unsigned int day;
	unsigned char int_state = 0;

	memset(alrm, 0, sizeof(struct rtc_wkalrm));

	lotus_rtc_read(rtc->regs, RTC_MR_S, &time_str.second);
	lotus_rtc_read(rtc->regs, RTC_MR_M, &time_str.minute);
	lotus_rtc_read(rtc->regs, RTC_MR_H, &time_str.hour);
	lotus_rtc_read(rtc->regs, RTC_MR_D_L, &time_str.dayl);
	lotus_rtc_read(rtc->regs, RTC_MR_D_H, &time_str.dayh);

	day = (unsigned int)(time_str.dayl | (time_str.dayh << 8)); /* Move to a high 8 bit. */
	seconds = date_to_sec(day, time_str.hour, time_str.minute, time_str.second);

	rtc_time64_to_tm(seconds, &alrm->time);

	lotus_rtc_read(rtc->regs, RTC_IMSC, &int_state);

	alrm->enabled = !!(int_state & AIE_INT_MASK);
	alrm->pending = alrm->enabled;

	return 0;
}

static int lotus_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct lotus_rtc *rtc = dev_get_drvdata(dev);
	int days;
	time64_t seconds = 0;
	unsigned char val = 0;

	if (alrm->enabled) {
		lotus_rtc_read(rtc->regs, RTC_IMSC, &val);
		lotus_rtc_write(rtc->regs, RTC_IMSC, val & ~AIE_INT_MASK);
		lotus_rtc_write(rtc->regs, RTC_INT_CLR, AIE_INT_MASK);
	}

	seconds = rtc_tm_to_time64(&alrm->time);

	days = div_s64(seconds, (60 * 60 * 24));

	lotus_rtc_write(rtc->regs, RTC_MR_10MS, 0);
	lotus_rtc_write(rtc->regs, RTC_MR_S, alrm->time.tm_sec);
	lotus_rtc_write(rtc->regs, RTC_MR_M, alrm->time.tm_min);
	lotus_rtc_write(rtc->regs, RTC_MR_H, alrm->time.tm_hour);
	lotus_rtc_write(rtc->regs, RTC_MR_D_L, (days & 0xFF));
	lotus_rtc_write(rtc->regs, RTC_MR_D_H, (days >> 8)); /* Move to a Low 8 bit. */

	mdelay(RTC_WRITE_TIME_MS);
	lotus_rtc_write(rtc->regs, RTC_INT_CLR, AIE_INT_MASK);
	lotus_rtc_read(rtc->regs, RTC_IMSC, &val);
	if (alrm->enabled)
		lotus_rtc_write(rtc->regs, RTC_IMSC, val | AIE_INT_MASK);
	else
		lotus_rtc_write(rtc->regs, RTC_IMSC, val & ~AIE_INT_MASK);

	return 0;
}

static int lotus_rtc_alarm_irq_enable(struct device *dev,
				      unsigned int enabled)
{
	struct lotus_rtc *rtc = dev_get_drvdata(dev);
	unsigned char val = 0;

	lotus_rtc_read(rtc->regs, RTC_IMSC, &val);
	if (enabled)
		lotus_rtc_write(rtc->regs, RTC_IMSC, val | AIE_INT_MASK);
	else
		lotus_rtc_write(rtc->regs, RTC_IMSC, val & ~AIE_INT_MASK);

	return 0;
}

void dump_rtc_reg(void)
{
#ifdef _RTC_AOV_DEBUG
	unsigned char val = 0;

	lotus_rtc_read(rtc_reg_base, RTC_IMSC, &val);
	pr_info("RTC_IMSC: 0x%x\n", val);
	lotus_rtc_read(rtc_reg_base, RTC_INT, &val);
	pr_info("RTC_INT: 0x%x\n", val);
	lotus_rtc_read(rtc_reg_base, RTC_INT_RAW, &val);
	pr_info("RTC_INT_RAW: 0x%x\n", val);
	lotus_rtc_read(rtc_reg_base, RTC_MR_D_H, &val);
	pr_info("RTC_MR_D_H: 0x%x\n", val);
	lotus_rtc_read(rtc_reg_base, RTC_MR_D_L, &val);
	pr_info("RTC_MR_D_L: 0x%x\n", val);
	lotus_rtc_read(rtc_reg_base, RTC_MR_H, &val);
	pr_info("RTC_MR_H: 0x%x\n", val);
	lotus_rtc_read(rtc_reg_base, RTC_MR_M, &val);
	pr_info("RTC_MR_M: 0x%x\n", val);
	lotus_rtc_read(rtc_reg_base, RTC_MR_S, &val);
	pr_info("RTC_MR_S: 0x%x\n", val);
	lotus_rtc_read(rtc_reg_base, RTC_MR_10MS, &val);
	pr_info("RTC_MR_10MS: 0x%x\n", val);
#endif
}
/*
 * interrupt function
 * do nothing. left for future
 */
static irqreturn_t lotus_rtc_alm_interrupt(int irq, void *data)
{
	struct lotus_rtc *rtc = (struct lotus_rtc *)data;
	unsigned char val = 0;

	lotus_rtc_read(rtc->regs, RTC_INT, &val);
	lotus_rtc_write(rtc->regs, RTC_INT_CLR, AIE_INT_MASK);

	if (val & AIE_INT_MASK) {
#ifdef _RTC_AOV_DEBUG
		pr_info("enter rtc alarm interrupt, INT:0x%x\n", val);
#endif
		rtc_update_irq(rtc->rtc_dev, 1, RTC_AF | RTC_IRQF);
		lotus_rtc_read(rtc->regs, RTC_IMSC, &val);
		lotus_rtc_write(rtc->regs, RTC_IMSC, val & ~AIE_INT_MASK);
	}

	return IRQ_HANDLED;
}

#define FREQ_MAX_VAL	    32799
#define FREQ_MIN_VAL	    32700

static int lotus_rtc_ioctl(struct device *dev,
			   unsigned int cmd, unsigned long arg)
{
	struct lotus_rtc *rtc = dev_get_drvdata(dev);

	switch (cmd) {
	case RTC_PLL_SET: {
		char freq_l;
		struct rtc_pll_info pll_info = {0};

		if (copy_from_user(&pll_info, (struct rtc_pll_info *)(uintptr_t)arg,
				   sizeof(struct rtc_pll_info)))
			return -EFAULT;

		/* freq = 32700 + b */
		if (pll_info.pll_value > FREQ_MAX_VAL ||
			pll_info.pll_value < FREQ_MIN_VAL)
			return -EINVAL;

		/* freq convert: freq - 32700 */
		pll_info.pll_value = pll_info.pll_value - 32700;

		/* & 0xff Obtains the lower eight bits of data, higher eight bits not used */
		freq_l = (char)(pll_info.pll_value & 0xff);

		lotus_rtc_write(rtc->regs, RTC_FREQ_L, freq_l);

		return 0;
	}
	case RTC_PLL_GET: {
		char freq_l = 0;
		struct rtc_pll_info pll_info = {0};

		lotus_rtc_read(rtc->regs, RTC_FREQ_L, &freq_l);

		if ((void __user *)(uintptr_t)arg == NULL) {
			dev_err(dev, "IO err or user buf is NULL..\n");
			return -1;
		}

		/* freq_l is lower eight bits of data, higher eight bits not used */
		pll_info.pll_value = freq_l;

		/* freq convert: 32700 + freq_l */
		pll_info.pll_value = 32700 + (unsigned int)pll_info.pll_value;

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

static const struct rtc_class_ops lotus_rtc_ops = {
	.read_time = lotus_rtc_read_time,
	.set_time = lotus_rtc_set_time,
	.read_alarm = lotus_rtc_read_alarm,
	.set_alarm = lotus_rtc_set_alarm,
	.alarm_irq_enable = lotus_rtc_alarm_irq_enable,
	.ioctl = lotus_rtc_ioctl,
};

static int lotus_rtc_init(struct lotus_rtc *rtc)
{
	void *spi_reg = rtc->regs;
	unsigned char val = 0;

	/*
	 * clk div value = (apb_clk/spi_clk)/2-1,
	 * asic apb clk = 50MHz, spi_clk = 5MHz,so value= 0x4
	 * fpga apb clk = 30MHz, spi_clk = 3MHz,so value= 0x4
	 */
	writel(CLK_DIV_DEFAULT, (spi_reg + SPI_CLK_DIV));

	lotus_rtc_write(spi_reg, RTC_IMSC, INT_MSK_DEFAULT);

	/* enable low voltage detected */
	lotus_rtc_read(spi_reg, RTC_SAR_CTRL, &val);
	val = val & (~BIT(5));
	lotus_rtc_write(spi_reg, RTC_SAR_CTRL, val);

	/* default driver capability */
	lotus_rtc_write(spi_reg, RTC_REG_LOCK4, 0x5A); /* 0x5A:ctl order */
	lotus_rtc_write(spi_reg, RTC_REG_LOCK3, 0x5A); /* 0x5A:ctl order */
	lotus_rtc_write(spi_reg, RTC_REG_LOCK2, 0xAB); /* 0xAB:ctl order */
	lotus_rtc_write(spi_reg, RTC_REG_LOCK1, 0xCD); /* 0xCD:ctl order */

	/* set oscillating current max*/
	lotus_rtc_write(spi_reg, RTC_CLK_CFG, 0x03);

	/* freq_l (0-99), freq_h not used */
	//lotus_rtc_write(spi_reg, RTC_FREQ_H, FREQ_H_DEFAULT);
	lotus_rtc_write(spi_reg, RTC_FREQ_L, FREQ_L_DEFAULT);

	lotus_rtc_read(spi_reg, RTC_INT_RAW, &val);

	if (val & LV_INT_MASK)
		/* low voltage detected, date/time is not reliable. */
		lotus_rtc_write(rtc->regs, RTC_INT_CLR, 1);

	return 0;
}

static int lotus_rtc_probe(struct platform_device *pdev)
{
	struct resource  *mem = NULL;
	struct lotus_rtc *rtc = NULL;
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

#ifdef _RTC_AOV_DEBUG
	rtc_reg_base = rtc->regs;
#endif

	rtc->rtc_irq = platform_get_irq(pdev, 0);
	ret = devm_request_irq(&pdev->dev, rtc->rtc_irq,
			       lotus_rtc_alm_interrupt, 0, pdev->name, rtc);
	if (ret) {
		dev_err(&pdev->dev, "could not request irq %d\n", rtc->rtc_irq);
		return ret;
	}

	platform_set_drvdata(pdev, rtc);
	rtc->rtc_dev = devm_rtc_device_register(&pdev->dev, pdev->name,
						&lotus_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc->rtc_dev)) {
		dev_err(&pdev->dev, "could not register rtc device\n");
		return PTR_ERR(rtc->rtc_dev);
	}

	if (lotus_rtc_init(rtc)) {
		dev_err(&pdev->dev, "lotus_rtc_init failed.\n");
		return -EIO;
	}

	rtc->rtc_first_suspend = true;
	dev_info(&pdev->dev, "RTC driver for lotus enabled\n");

	return 0;
}

static int lotus_rtc_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_PM
static int lotus_rtc_suspend(struct device *dev)
{
	struct rtc_wkalrm alarm = {0};
	struct rtc_time linuxTime = {0};
	time64_t time;
	time64_t time1;
	unsigned char ms,ms1;

	struct lotus_rtc *rtc = dev_get_drvdata(dev);
	lotus_rtc_aov_read_time(dev, &linuxTime, &ms);
	rtc->suspend_time = linuxTime;
	rtc->suspend_ms = ms;
	if (!rtc->alarm_enable || !rtc->rtc_first_suspend)
		return 0;

	time1 = rtc_tm_to_time64(&linuxTime);
	time = time1 + (rtc->alarm_time)/100;
	ms1 = ms + ((rtc->alarm_time) % 100 )+ 1;
	if (ms1 > 99) {
		time += 1;
		ms1 -= 100;
	}
	rtc_time64_to_tm(time, &alarm.time);
	alarm.enabled = rtc->alarm_enable;
	lotus_rtc_aov_set_alarm(dev, &alarm, ms1);

	mdelay(10);
	rtc->rtc_first_suspend = false;
	return 0;
}

static int lotus_rtc_resume(struct device *dev)
{
	struct rtc_wkalrm alarm = {0};
	struct rtc_time linuxTime = {0};
	time64_t time;
	time64_t time1;
	unsigned char ms,ms1;

	struct lotus_rtc *rtc = dev_get_drvdata(dev);
	if (rtc) {
		/* spi clk div reg value will be lost when cpu power down. we reload it in resume */
		writel(CLK_DIV_DEFAULT, (rtc->regs + SPI_CLK_DIV));
	}

	if (!rtc->alarm_enable)
		return 0;

	lotus_rtc_aov_read_time(dev, &linuxTime, &ms);
	rtc->resume_time = linuxTime;
	rtc->resume_ms = ms;
	time1 = rtc_tm_to_time64(&linuxTime);
	time = time1 + (rtc->alarm_time)/100;
	ms1 = ms + ((rtc->alarm_time) % 100 )+ 1;
	if (ms1 > 99) {
		time += 1;
		ms1 -= 100;
	}
	rtc_time64_to_tm(time, &alarm.time);
	alarm.enabled = rtc->alarm_enable;
	lotus_rtc_aov_set_alarm(dev, &alarm, ms1);

	return 0;
}

const struct dev_pm_ops __maybe_unused lotus_rtc_rtc_pm = {
	.suspend_late = lotus_rtc_suspend,
	.resume_early = lotus_rtc_resume,
};
#endif

static const struct of_device_id lotus_rtc_match[] = {
	{ .compatible = "lotus,rtc" },
	{},
};

static struct platform_driver lotus_rtc_driver = {
	.probe  = lotus_rtc_probe,
	.remove = lotus_rtc_remove,
	.driver =  {
		.name = "lotus_rtc",
		.of_match_table = lotus_rtc_match,
#ifdef CONFIG_PM
		.pm = &lotus_rtc_rtc_pm,
#endif
	},
};

module_platform_driver(lotus_rtc_driver);

#define OSDRV_MODULE_VERSION_STRING "LOTUS_rtc @LOTUS"

MODULE_AUTHOR("Lotus");
MODULE_DESCRIPTION("Lotus RTC driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("LOTUS_VERSION=" OSDRV_MODULE_VERSION_STRING);
