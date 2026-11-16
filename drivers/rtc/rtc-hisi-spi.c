// SPDX-License-Identifier: GPL-2.0-only
/*
 * HiSilicon SPI-bridged RTC
 *
 * The RTC macro lives in its own, battery backed power domain. It holds a
 * 100 Hz counter split into 10 ms, seconds, minutes, hours and a 16 bit day
 * count, plus a match (alarm) comparator for the same fields. None of these
 * registers is on the APB bus: the APB window only has SPI_CLK_DIV and SPI_RW,
 * a tiny SPI master wired to the macro, and every access to an internal
 * 8 bit register goes through SPI_RW.
 *
 * The day counter is used as the number of days since 1970-01-01, so the time
 * is simply the Unix time split into days and a time of day. It wraps at
 * 65536 days, i.e. in 2149.
 *
 * The APB side and the access protocol are the same on all SoCs. The internal
 * register map and a few timing rules differ in editions, selected by the
 * compatible string:
 *
 *  - V1: counter registers always readable, no wait around the load
 *  - V1 with lock (Hi3516A/D V100): the counter registers update only after
 *    a lock request, 5 ms wait before the load
 *  - V2 and V3: lock request. V3 runs the SPI bridge at 5 MHz at most, has
 *    a global interrupt enable and wants 1 ms between the lock and the read
 *
 * State kept in the battery domain survives the boot loader, a reboot and
 * the driver. The driver does not rely on it where that matters for
 * timekeeping: at probe and resume it programs the bridge clock divider
 * (read back), the lock mode and the interrupt registers, and at probe the
 * test clock output, the match value and, on the editions that have it, the
 * temperature compensation mode (TEMP_SEL, to its reset value), and it checks
 * that the counter runs.
 *
 * Left alone on purpose:
 *
 *  - The correction table (LUT1 ... LUT47), TOT_OFFSET, TEMP_OFFSET and
 *    Sdm_coef: they are the trim of the crystal and the battery domain is the
 *    only place a calibration of the board can live in. The reset values
 *    suit a typical crystal.
 *  - The routing of the interrupt line. On some SoCs it is shared with
 *    another source, and on Hi3518 and Hi3516C V100 a system controller bit
 *    (PERI_CTRL0 bit 31) selects it. The driver leaves that to the board
 *    description.
 */

#include <linux/bitfield.h>
#include <linux/bits.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/jiffies.h>
#include <linux/ktime.h>
#include <linux/math.h>
#include <linux/minmax.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/pm_wakeirq.h>
#include <linux/pm_wakeup.h>
#include <linux/property.h>
#include <linux/rtc.h>
#include <linux/time64.h>

/* APB registers */
#define HISI_SPI_RTC_CLK_DIV		0x00
#define  CLK_DIV_MASK			GENMASK(7, 0)
#define HISI_SPI_RTC_RW			0x04
#define  SPI_RW_BUSY			BIT(31)
#define  SPI_RW_START			BIT(24)
#define  SPI_RW_READ			BIT(23)
#define  SPI_RW_ADDR			GENMASK(22, 16)
#define  SPI_RW_RDATA			GENMASK(15, 8)
#define  SPI_RW_WDATA			GENMASK(7, 0)

#define HISI_SPI_RTC_XFER_TIMEOUT_US	1000

/* Internal registers, addressed through SPI_RW */
#define RTC_10MS_COUNT			0x00
#define  RTC_10MS_COUNT_MASK		GENMASK(6, 0)
#define RTC_S_COUNT			0x01	/* then M, H, D_L, D_H */
#define RTC_MR_10MS			0x06	/* match (alarm) value, 10 ms ... */
#define RTC_MR_S			0x07
#define RTC_LR_10MS			0x0c	/* load value, 10 ms ... */
#define RTC_LORD			0x12
#define  RTC_LORD_BYPASS		BIT(2)	/* lock editions: counter always readable */
#define  RTC_LORD_LOCK			BIT(1)	/* lock editions: latch the counter */
#define  RTC_LORD_LOAD			BIT(0)	/* load the load value into the counter */
#define RTC_IMSC			0x13
#define  RTC_IMSC_TIME			BIT(0)
#define RTC_INT_CLR			0x14
#define RTC_MSC_INT			0x15	/* masked interrupt status */
#define  RTC_MSC_INT_TIME		BIT(0)
#define RTC_CLK				0x17	/* test clock output select */
#define RTC_TEMP_SEL			0x21	/* V1: temperature compensation mode */
#define  RTC_TEMP_SEL_MASK		GENMASK(2, 0)	/* sdm_sel, fix_mode, temp_sel */

#define HISI_SPI_RTC_LOAD_TIMEOUT_MS	100
#define HISI_SPI_RTC_READ_RETRIES	3
#define HISI_SPI_RTC_LOAD_RETRIES	3
#define HISI_SPI_RTC_LOAD_GAP_MS	50
/*
 * A loaded time starts to run about 25 ms after the load request (measured
 * on the Hi3520D V100: the seconds then change 5 ms late with 20 ms), with
 * the 10 ms count at 0, so the first seconds increment follows one second
 * later.
 */
#define HISI_SPI_RTC_LOAD_DELAY_MS	25
#define HISI_SPI_RTC_TICK_POLLS		10
#define HISI_SPI_RTC_TICK_POLL_MS	20

/**
 * struct hisi_spi_rtc_data - differences between the editions of the RTC
 * @spi_max_hz:		highest SPI bridge clock allowed by the data sheet
 * @has_lock:		the counter registers only update after RTC_LORD.lock was
 *			written and cleared by the hardware
 * @lock_settle_us:	delay between the hardware clearing RTC_LORD.lock and
 *			reading the counter registers
 * @has_tcomp:		TEMP_SEL is present (temperature compensation registers)
 * @load_wait_before_ms:	delay between writing the load value and setting
 *			RTC_LORD.load
 * @imsc_enable:	value written to RTC_IMSC to enable the alarm interrupt
 *			(editions with a global interrupt enable in bit 2 need
 *			it set, the under-voltage enable in bit 1 stays clear)
 */
struct hisi_spi_rtc_data {
	unsigned long spi_max_hz;
	bool has_lock;
	unsigned int lock_settle_us;
	bool has_tcomp;
	unsigned int load_wait_before_ms;
	u8 imsc_enable;
};

static const struct hisi_spi_rtc_data hisi_spi_rtc_v1 = {
	.spi_max_hz = 12000000,
	.has_tcomp = true,
	.imsc_enable = RTC_IMSC_TIME,
};

static const struct hisi_spi_rtc_data hisi_spi_rtc_v1_lock = {
	.spi_max_hz = 12000000,
	.has_lock = true,
	.has_tcomp = true,
	.load_wait_before_ms = 5,
	.imsc_enable = RTC_IMSC_TIME,
};

static const struct hisi_spi_rtc_data hisi_spi_rtc_v2 = {
	.spi_max_hz = 12000000,
	.has_lock = true,
	.imsc_enable = RTC_IMSC_TIME,
};

static const struct hisi_spi_rtc_data hisi_spi_rtc_v3 = {
	.spi_max_hz = 5000000,
	.has_lock = true,
	.lock_settle_us = 1000,
	.imsc_enable = BIT(2) | RTC_IMSC_TIME,
};

struct hisi_spi_rtc {
	struct device *dev;
	struct rtc_device *rtc;
	void __iomem *base;
	struct clk *clk;
	const struct hisi_spi_rtc_data *data;
	u32 div;		/* SPI_CLK_DIV value for the bus clock */
	struct mutex lock;	/* serializes SPI_RW transfers */
	ktime_t last_load;	/* monotonic time of the last load request */
};

static int hisi_spi_rtc_xfer(struct hisi_spi_rtc *hr, u32 cmd, u32 *val)
{
	u32 v;
	int ret;

	/*
	 * The lock keeps the busy/start/busy sequence of one transfer from
	 * being interleaved with another one. The polls are short (tens of
	 * microseconds), so they stay busy-waits under the mutex.
	 */
	mutex_lock(&hr->lock);

	ret = readl_poll_timeout_atomic(hr->base + HISI_SPI_RTC_RW, v,
					!(v & SPI_RW_BUSY), 1,
					HISI_SPI_RTC_XFER_TIMEOUT_US);
	if (ret)
		goto out;

	writel(cmd | SPI_RW_START, hr->base + HISI_SPI_RTC_RW);

	ret = readl_poll_timeout_atomic(hr->base + HISI_SPI_RTC_RW, v,
					!(v & SPI_RW_BUSY), 1,
					HISI_SPI_RTC_XFER_TIMEOUT_US);
	if (!ret && val)
		*val = FIELD_GET(SPI_RW_RDATA, v);
out:
	mutex_unlock(&hr->lock);
	/* -ETIMEDOUT is kept for the waits on the RTC itself */
	return ret ? -EIO : 0;
}

static int hisi_spi_rtc_read_reg(struct hisi_spi_rtc *hr, u8 reg, u8 *val)
{
	u32 v;
	int ret;

	ret = hisi_spi_rtc_xfer(hr, SPI_RW_READ | FIELD_PREP(SPI_RW_ADDR, reg),
				&v);
	if (!ret)
		*val = v;

	return ret;
}

static int hisi_spi_rtc_write_reg(struct hisi_spi_rtc *hr, u8 reg, u8 val)
{
	return hisi_spi_rtc_xfer(hr, FIELD_PREP(SPI_RW_ADDR, reg) |
				     FIELD_PREP(SPI_RW_WDATA, val), NULL);
}

/*
 * Read the seconds ... days registers starting at @s_reg (the counter or the
 * match value) and return them as seconds since the epoch.
 */
static int hisi_spi_rtc_read_fields(struct hisi_spi_rtc *hr, u8 s_reg,
				    u64 *secs)
{
	u8 s, m, h, dl, dh, s2;
	int ret;

	/* S, M, H, D_L, D_H are consecutive in the counter and match banks */
	ret = hisi_spi_rtc_read_reg(hr, s_reg, &s);
	ret = ret ?: hisi_spi_rtc_read_reg(hr, s_reg + 1, &m);
	ret = ret ?: hisi_spi_rtc_read_reg(hr, s_reg + 2, &h);
	ret = ret ?: hisi_spi_rtc_read_reg(hr, s_reg + 3, &dl);
	ret = ret ?: hisi_spi_rtc_read_reg(hr, s_reg + 4, &dh);
	ret = ret ?: hisi_spi_rtc_read_reg(hr, s_reg, &s2);
	if (ret)
		return ret;

	if (s != s2)
		return -EAGAIN;	/* a carry happened while reading */

	*secs = ((u64)(dl | dh << 8) * 24 + (h & 0x1f)) * 3600 +
		(m & 0x3f) * 60 + (s & 0x3f);
	return 0;
}

static int hisi_spi_rtc_write_fields(struct hisi_spi_rtc *hr, u8 ms_reg,
				     u64 secs)
{
	u32 tod, days = div_u64_rem(secs, 86400, &tod);
	int ret;

	ret = hisi_spi_rtc_write_reg(hr, ms_reg, 0);
	ret = ret ?: hisi_spi_rtc_write_reg(hr, ms_reg + 1, tod % 60);
	ret = ret ?: hisi_spi_rtc_write_reg(hr, ms_reg + 2, tod / 60 % 60);
	ret = ret ?: hisi_spi_rtc_write_reg(hr, ms_reg + 3, tod / 3600);
	ret = ret ?: hisi_spi_rtc_write_reg(hr, ms_reg + 4, days & 0xff);
	ret = ret ?: hisi_spi_rtc_write_reg(hr, ms_reg + 5, days >> 8);

	return ret;
}

/* Wait for the hardware to clear the @mask bits of RTC_LORD */
static int hisi_spi_rtc_wait_lord(struct hisi_spi_rtc *hr, u8 mask)
{
	unsigned long timeout =
		jiffies + msecs_to_jiffies(HISI_SPI_RTC_LOAD_TIMEOUT_MS);
	bool expired;
	u8 v;
	int ret;

	do {
		/* sample the clock first: the last check follows the sleep */
		expired = time_after(jiffies, timeout);
		ret = hisi_spi_rtc_read_reg(hr, RTC_LORD, &v);
		if (ret)
			return ret;
		if (!(v & mask))
			return 0;
		if (!expired)
			usleep_range(500, 1000);
	} while (!expired);

	return -ETIMEDOUT;
}

/* Latch the counter into the readable registers (editions with a lock) */
static int hisi_spi_rtc_latch(struct hisi_spi_rtc *hr)
{
	int ret;

	ret = hisi_spi_rtc_write_reg(hr, RTC_LORD, RTC_LORD_LOCK);
	if (ret)
		return ret;

	ret = hisi_spi_rtc_wait_lord(hr, RTC_LORD_LOCK);
	if (!ret && hr->data->lock_settle_us)
		usleep_range(hr->data->lock_settle_us,
			     hr->data->lock_settle_us + 500);

	return ret;
}

static int hisi_spi_rtc_read_secs(struct hisi_spi_rtc *hr, u64 *secs)
{
	int i, ret = -EAGAIN;

	if (hr->data->has_lock) {
		ret = hisi_spi_rtc_latch(hr);
		if (ret)
			return ret;
		ret = -EAGAIN;
	}

	for (i = 0; i < HISI_SPI_RTC_READ_RETRIES && ret == -EAGAIN; i++)
		ret = hisi_spi_rtc_read_fields(hr, RTC_S_COUNT, secs);

	return ret;
}

static int hisi_spi_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct hisi_spi_rtc *hr = dev_get_drvdata(dev);
	u64 secs;
	int ret;

	ret = hisi_spi_rtc_read_secs(hr, &secs);
	if (ret)
		return ret;

	rtc_time64_to_tm(secs, tm);
	return 0;
}

/*
 * RTC_LORD.load clears before the counter shows the loaded value, which
 * takes about 25 ms. Wait until the counter registers show it, so that a
 * read after set_time returns the new time.
 */
static int hisi_spi_rtc_wait_loaded(struct hisi_spi_rtc *hr, u64 want)
{
	unsigned long timeout =
		jiffies + msecs_to_jiffies(HISI_SPI_RTC_LOAD_TIMEOUT_MS);
	bool expired;
	u64 secs;
	int ret;

	do {
		expired = time_after(jiffies, timeout);
		ret = hisi_spi_rtc_read_secs(hr, &secs);
		if (ret && ret != -EAGAIN)
			return ret;
		if (!ret && secs >= want && secs <= want + 2)
			return 0;
		if (!expired)
			usleep_range(1000, 2000);
	} while (!expired);

	return -ETIMEDOUT;
}

static int hisi_spi_rtc_load(struct hisi_spi_rtc *hr, u64 secs)
{
	int ret;

	/* A previous load must have completed, the hardware clears LORD */
	ret = hisi_spi_rtc_wait_lord(hr, RTC_LORD_LOAD);
	if (ret)
		return ret;

	/* The counter and the load value share the field layout */
	ret = hisi_spi_rtc_write_fields(hr, RTC_LR_10MS, secs);
	if (ret)
		return ret;

	if (hr->data->load_wait_before_ms)
		usleep_range(hr->data->load_wait_before_ms * 1000,
			     hr->data->load_wait_before_ms * 1000 + 500);

	ret = hisi_spi_rtc_write_reg(hr, RTC_LORD, RTC_LORD_LOAD);
	ret = ret ?: hisi_spi_rtc_wait_lord(hr, RTC_LORD_LOAD);
	hr->last_load = ktime_get();
	if (ret)
		return ret;

	return hisi_spi_rtc_wait_loaded(hr, secs);
}

/* Keep a load request at least HISI_SPI_RTC_LOAD_GAP_MS after the previous one */
static void hisi_spi_rtc_load_gap(struct hisi_spi_rtc *hr)
{
	s64 since = ktime_ms_delta(ktime_get(), hr->last_load);

	if (since < HISI_SPI_RTC_LOAD_GAP_MS)
		msleep(HISI_SPI_RTC_LOAD_GAP_MS - since);
}

static int hisi_spi_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	struct hisi_spi_rtc *hr = dev_get_drvdata(dev);
	u64 secs = rtc_tm_to_time64(tm);
	int i, ret = -ETIMEDOUT;

	/*
	 * A load request that follows the previous one too closely is
	 * dropped silently (RTC_LORD.load clears, the counter keeps its
	 * value). Keep the requests apart and repeat a dropped one.
	 */
	for (i = 0; i < HISI_SPI_RTC_LOAD_RETRIES && ret == -ETIMEDOUT; i++) {
		hisi_spi_rtc_load_gap(hr);
		ret = hisi_spi_rtc_load(hr, secs);
	}

	return ret;
}

static int hisi_spi_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct hisi_spi_rtc *hr = dev_get_drvdata(dev);
	u8 imsc, st;
	u64 secs;
	int ret;

	ret = hisi_spi_rtc_read_fields(hr, RTC_MR_S, &secs);
	ret = ret ?: hisi_spi_rtc_read_reg(hr, RTC_IMSC, &imsc);
	ret = ret ?: hisi_spi_rtc_read_reg(hr, RTC_MSC_INT, &st);
	if (ret)
		return ret;

	rtc_time64_to_tm(secs, &alrm->time);
	alrm->enabled = !!(imsc & RTC_IMSC_TIME);
	alrm->pending = !!(st & RTC_MSC_INT_TIME);
	return 0;
}

static int hisi_spi_rtc_alarm_irq_enable(struct device *dev,
					 unsigned int enabled)
{
	struct hisi_spi_rtc *hr = dev_get_drvdata(dev);

	return hisi_spi_rtc_write_reg(hr, RTC_IMSC,
				      enabled ? hr->data->imsc_enable : 0);
}

static int hisi_spi_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct hisi_spi_rtc *hr = dev_get_drvdata(dev);
	int ret;

	ret = hisi_spi_rtc_write_reg(hr, RTC_IMSC, 0);
	ret = ret ?: hisi_spi_rtc_write_fields(hr, RTC_MR_10MS,
					       rtc_tm_to_time64(&alrm->time));
	ret = ret ?: hisi_spi_rtc_write_reg(hr, RTC_INT_CLR, 1);
	if (ret)
		return ret;

	return hisi_spi_rtc_write_reg(hr, RTC_IMSC,
				      alrm->enabled ? hr->data->imsc_enable : 0);
}

static const struct rtc_class_ops hisi_spi_rtc_ops = {
	.read_time = hisi_spi_rtc_read_time,
	.set_time = hisi_spi_rtc_set_time,
	.read_alarm = hisi_spi_rtc_read_alarm,
	.set_alarm = hisi_spi_rtc_set_alarm,
	.alarm_irq_enable = hisi_spi_rtc_alarm_irq_enable,
};

/*
 * Threaded: the status check and the acknowledge are SPI transfers. The RTC
 * core holds ops_lock around set_alarm and alarm_irq_enable, which write
 * IMSC, the match value and INT_CLR; take it here too before the interrupt
 * is masked and cleared, so that the handler cannot do that in the middle of
 * a new alarm being set up. The lock order is ops_lock, then the transfer
 * lock, in both paths. The first status read does not need ops_lock: a
 * foreign interrupt on a shared line must not wait for a set_time, which
 * holds it for a few hundred milliseconds.
 */
static irqreturn_t hisi_spi_rtc_irq(int irq, void *data)
{
	struct hisi_spi_rtc *hr = data;
	bool fired;
	u8 st;
	int ret;

	ret = hisi_spi_rtc_read_reg(hr, RTC_MSC_INT, &st);
	if (ret || !(st & RTC_MSC_INT_TIME))
		return IRQ_NONE;

	mutex_lock(&hr->rtc->ops_lock);

	/*
	 * A set_alarm that ran in between has cleared the event and replaced
	 * the alarm: leave its interrupt mask alone.
	 */
	ret = hisi_spi_rtc_read_reg(hr, RTC_MSC_INT, &st);
	fired = !ret && (st & RTC_MSC_INT_TIME);
	if (fired) {
		/*
		 * The match condition holds for one 10 ms tick, so mask the
		 * interrupt before clearing it to keep it from latching again:
		 * the alarm is one-shot, the next set_alarm re-enables it.
		 */
		hisi_spi_rtc_write_reg(hr, RTC_IMSC, 0);
		hisi_spi_rtc_write_reg(hr, RTC_INT_CLR, 1);
	}

	mutex_unlock(&hr->rtc->ops_lock);

	if (fired)
		rtc_update_irq(hr->rtc, 1, RTC_IRQF | RTC_AF);

	return IRQ_HANDLED;
}

/*
 * Program the divider of the SPI bridge: FSPICLK = FAPBCLK / (2 * (div + 1)).
 * Does not sleep, so it can run in the noirq resume phase.
 */
static int hisi_spi_rtc_set_divider(struct hisi_spi_rtc *hr)
{
	u32 rb;

	writel(hr->div, hr->base + HISI_SPI_RTC_CLK_DIV);
	rb = readl(hr->base + HISI_SPI_RTC_CLK_DIV);
	if (FIELD_GET(CLK_DIV_MASK, rb) != hr->div) {
		dev_err(hr->dev, "SPI_CLK_DIV reads back 0x%x, wrote 0x%x\n",
			rb, hr->div);
		return -EIO;
	}

	return 0;
}

/*
 * Bring the counter lock mode into a known state: the counter registers must
 * be read through the lock request, so clear the lock bypass bit and a lock
 * request left pending. A load that is still running has to finish first.
 */
static int hisi_spi_rtc_setup_lock(struct hisi_spi_rtc *hr)
{
	u8 v;
	int ret;

	if (!hr->data->has_lock)
		return 0;

	ret = hisi_spi_rtc_wait_lord(hr, RTC_LORD_LOAD);
	ret = ret ?: hisi_spi_rtc_write_reg(hr, RTC_LORD, 0);
	ret = ret ?: hisi_spi_rtc_read_reg(hr, RTC_LORD, &v);
	if (ret) {
		dev_err(hr->dev, "RTC does not answer on the SPI bridge or does not complete a load: %d\n",
			ret);
		return ret;
	}

	if (v & RTC_LORD_BYPASS) {
		dev_err(hr->dev, "RTC_LORD stuck at 0x%02x, lock bypass cannot be cleared\n",
			v);
		return -EIO;
	}

	return 0;
}

/*
 * Quiet comparator, test clock output off, match value cleared and the
 * temperature compensation of the crystal in its reset mode (compensation on,
 * input from the internal sensor).
 */
static int hisi_spi_rtc_setup(struct hisi_spi_rtc *hr)
{
	u8 v;
	int ret;

	ret = hisi_spi_rtc_setup_lock(hr);
	if (ret)
		return ret;

	ret = hisi_spi_rtc_write_reg(hr, RTC_IMSC, 0);
	ret = ret ?: hisi_spi_rtc_write_reg(hr, RTC_INT_CLR, 1);
	ret = ret ?: hisi_spi_rtc_write_reg(hr, RTC_CLK, 0);
	/* the match value is not defined after a power-up of the macro */
	ret = ret ?: hisi_spi_rtc_write_fields(hr, RTC_MR_10MS, 0);
	if (ret) {
		dev_err(hr->dev, "RTC does not answer on the SPI bridge: %d\n", ret);
		return ret;
	}

	if (!hr->data->has_tcomp)
		return 0;

	/*
	 * Reset value 0: compensation on, input from the internal sensor. The
	 * other settings switch the compensation off (the clock is then the
	 * fixed divided one) or take the temperature from OUTSIDE_TEMP, which
	 * nothing feeds. sdm_sel does not exist on the Hi3518 and Hi3516C V100
	 * and reads back as 0 there.
	 */
	ret = hisi_spi_rtc_read_reg(hr, RTC_TEMP_SEL, &v);
	ret = ret ?: hisi_spi_rtc_write_reg(hr, RTC_TEMP_SEL, 0);
	if (ret) {
		dev_err(hr->dev, "RTC does not answer on the SPI bridge: %d\n", ret);
		return ret;
	}
	if (v & RTC_TEMP_SEL_MASK)
		dev_info(hr->dev, "TEMP_SEL was 0x%02lx, set to the reset value 0\n",
			 v & RTC_TEMP_SEL_MASK);

	ret = hisi_spi_rtc_read_reg(hr, RTC_TEMP_SEL, &v);
	if (ret) {
		dev_err(hr->dev, "RTC does not answer on the SPI bridge: %d\n", ret);
		return ret;
	}
	if (v & RTC_TEMP_SEL_MASK)
		dev_warn(hr->dev, "TEMP_SEL reads back 0x%02x after writing 0, the temperature compensation mode of the crystal is not the reset one\n",
			 v);

	return 0;
}

/* Read the 10 ms counter, through the lock request where needed */
static int hisi_spi_rtc_read_tick(struct hisi_spi_rtc *hr, u8 *ms)
{
	int ret = 0;

	if (hr->data->has_lock)
		ret = hisi_spi_rtc_latch(hr);

	return ret ?: hisi_spi_rtc_read_reg(hr, RTC_10MS_COUNT, ms);
}

/*
 * A board without the 32 kHz crystal (or with the RTC supply unpowered)
 * answers on the bridge but its counter never advances. Refuse to register
 * rather than hand out a clock that always reads the same time.
 */
static int hisi_spi_rtc_check_running(struct hisi_spi_rtc *hr)
{
	u8 first, now;
	int i, ret;

	ret = hisi_spi_rtc_read_tick(hr, &first);
	for (i = 0; !ret && i < HISI_SPI_RTC_TICK_POLLS; i++) {
		msleep(HISI_SPI_RTC_TICK_POLL_MS);
		ret = hisi_spi_rtc_read_tick(hr, &now);
		if (!ret && (now ^ first) & RTC_10MS_COUNT_MASK)
			return 0;
	}

	/* a lock request that never completes is the same symptom */
	if (!ret || ret == -ETIMEDOUT) {
		dev_err(hr->dev, "RTC counter does not advance, crystal or RTC supply missing\n");
		return -ENODEV;
	}

	dev_err(hr->dev, "RTC does not answer on the SPI bridge: %d\n", ret);
	return ret;
}

static void hisi_spi_rtc_mask_irq(void *data)
{
	struct hisi_spi_rtc *hr = data;

	hisi_spi_rtc_write_reg(hr, RTC_IMSC, 0);
}

static int hisi_spi_rtc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hisi_spi_rtc *hr;
	unsigned long rate, div;
	bool alarm = false;
	int irq, ret;

	hr = devm_kzalloc(dev, sizeof(*hr), GFP_KERNEL);
	if (!hr)
		return -ENOMEM;

	hr->dev = dev;
	hr->data = device_get_match_data(dev);
	if (!hr->data)
		return -ENODEV;
	hr->last_load = ktime_sub_ms(ktime_get(), HISI_SPI_RTC_LOAD_GAP_MS);
	mutex_init(&hr->lock);
	platform_set_drvdata(pdev, hr);

	hr->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(hr->base))
		return PTR_ERR(hr->base);

	hr->clk = devm_clk_get_enabled(dev, NULL);
	if (IS_ERR(hr->clk))
		return dev_err_probe(dev, PTR_ERR(hr->clk), "cannot get clock\n");

	rate = clk_get_rate(hr->clk);
	if (!rate)
		return dev_err_probe(dev, -EINVAL,
				     "bus clock rate unknown, cannot set the bridge divider\n");

	/* valid range 1 ... 255, the read back of the register checks the upper end */
	div = max(DIV_ROUND_UP(rate, 2 * hr->data->spi_max_hz) - 1, 1UL);
	hr->div = div;

	hr->rtc = devm_rtc_allocate_device(dev);
	if (IS_ERR(hr->rtc))
		return PTR_ERR(hr->rtc);

	hr->rtc->ops = &hisi_spi_rtc_ops;
	hr->rtc->range_min = 0;
	hr->rtc->range_max = 65536ULL * 86400 - 1;
	/*
	 * The time handed to set_time starts to run when the load request is
	 * issued, which follows the call by the wait before the load (a few
	 * SPI transfers otherwise), and starts to run 25 ms later; its seconds
	 * increment one second after that.
	 */
	hr->rtc->set_offset_nsec = NSEC_PER_SEC +
		(HISI_SPI_RTC_LOAD_DELAY_MS + hr->data->load_wait_before_ms) *
		NSEC_PER_MSEC;

	ret = hisi_spi_rtc_set_divider(hr);
	ret = ret ?: hisi_spi_rtc_setup(hr);
	if (ret)
		return ret;

	ret = devm_add_action_or_reset(dev, hisi_spi_rtc_mask_irq, hr);
	if (ret)
		return ret;

	ret = hisi_spi_rtc_check_running(hr);
	if (ret)
		return ret;

	irq = platform_get_irq_optional(pdev, 0);
	if (irq < 0 && irq != -ENXIO)
		return dev_err_probe(dev, irq, "cannot get the interrupt\n");

	if (irq > 0) {
		/*
		 * Level interrupt without a hard handler: needs IRQF_ONESHOT.
		 * The line is shared with another source on some SoCs; the
		 * handler returns IRQ_NONE when the RTC did not raise it. A
		 * handler of the other source that is not oneshot makes the
		 * request fail with -EBUSY: the clock still works without
		 * the alarm, so do not fail the probe for it.
		 */
		ret = devm_request_threaded_irq(dev, irq, NULL, hisi_spi_rtc_irq,
						IRQF_ONESHOT | IRQF_SHARED,
						dev_name(dev), hr);
		if (ret && ret != -EBUSY)
			return dev_err_probe(dev, ret, "cannot get IRQ\n");

		if (ret)
			dev_warn(dev, "interrupt %d is shared with an incompatible handler, no alarm and no wakeup\n",
				 irq);
		else
			alarm = true;
	}

	if (alarm && device_property_read_bool(dev, "wakeup-source")) {
		ret = devm_device_init_wakeup(dev);
		ret = ret ?: devm_pm_set_wake_irq(dev, irq);
		if (ret)
			return dev_err_probe(dev, ret, "cannot set up wakeup\n");
	}

	if (!alarm)
		clear_bit(RTC_FEATURE_ALARM, hr->rtc->features);

	return devm_rtc_register_device(hr->rtc);
}

/*
 * The APB side may have been reset or gated in the sleep state. The divider
 * is restored before the interrupts are enabled, so that a wake interrupt
 * handled right after the noirq phase can already talk to the RTC; the rest
 * follows in the normal resume phase.
 */
static int hisi_spi_rtc_resume_noirq(struct device *dev)
{
	return hisi_spi_rtc_set_divider(dev_get_drvdata(dev));
}

static int hisi_spi_rtc_resume(struct device *dev)
{
	/*
	 * Only the lock mode: an armed alarm and a pending interrupt must
	 * survive for the handler that the wake interrupt schedules.
	 */
	return hisi_spi_rtc_setup_lock(dev_get_drvdata(dev));
}

static const struct dev_pm_ops hisi_spi_rtc_pm_ops = {
	NOIRQ_SYSTEM_SLEEP_PM_OPS(NULL, hisi_spi_rtc_resume_noirq)
	SYSTEM_SLEEP_PM_OPS(NULL, hisi_spi_rtc_resume)
};

static const struct of_device_id hisi_spi_rtc_of_match[] = {
	{ .compatible = "hisilicon,hisi-spi-rtc-v1", .data = &hisi_spi_rtc_v1 },
	{ .compatible = "hisilicon,hisi-spi-rtc-v2", .data = &hisi_spi_rtc_v2 },
	{ .compatible = "hisilicon,hisi-spi-rtc-v3", .data = &hisi_spi_rtc_v3 },
	{ .compatible = "hisilicon,hisi-spi-rtc-v1-lock", .data = &hisi_spi_rtc_v1_lock },
	{ }
};
MODULE_DEVICE_TABLE(of, hisi_spi_rtc_of_match);

static struct platform_driver hisi_spi_rtc_driver = {
	.probe = hisi_spi_rtc_probe,
	.driver = {
		.name = "rtc-hisi-spi",
		.of_match_table = hisi_spi_rtc_of_match,
		.pm = pm_sleep_ptr(&hisi_spi_rtc_pm_ops),
	},
};
module_platform_driver(hisi_spi_rtc_driver);

MODULE_AUTHOR("Dylan Corrales <deathcamel58@gmail.com>");
MODULE_DESCRIPTION("HiSilicon SPI-bridged RTC driver");
MODULE_LICENSE("GPL");
