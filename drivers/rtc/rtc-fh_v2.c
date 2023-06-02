#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/clk.h>
#include <linux/log2.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>
#include <linux/miscdevice.h>
#include <linux/fh_rtc_v2.h>
#ifdef CONFIG_USE_OF
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#else
#include <linux/irqdomain.h>
#endif
#include <mach/pmu.h>
#include <linux/fh_efuse.h>
#include <mach/fh_rtc_plat.h>

/*
 * Registers offset
 */
#define  FH_RTC_INT_STATUS			   0x0
#define  FH_RTC_INT_EN				   0x4
#define  FH_RTC_DEBUG0				   0x8
#define  FH_RTC_DEBUG1				   0xC
#define  FH_RTC_DEBUG2				   0x10
#define  FH_RTC_CMD					   0x14
#define  FH_RTC_RD_DATA				   0x18
#define  FH_RTC_WR_DATA				   0x1C

#define  FH_RTC_CMD_COUNTER            (0<<4)
#define  FH_RTC_CMD_OFFSET             (1<<4)
#define  FH_RTC_CMD_ALARM_CFG          (2<<4)
#define  FH_RTC_CMD_TEMP_INFO          (0x3<<4)
#define  FH_RTC_CMD_TEMP_CFG           (0x4<<4)
#define  FH_RTC_CMD_ANA_CFG            (0x5<<4)
#define  FH_RTC_CMD_INT_STATUS         (0x6<<4)
#define  FH_RTC_CMD_INT_EN             (0x7<<4)
#define  FH_RTC_CMD_DEBUG              (0x8<<4)
#define  FH_RTC_CMD_OFFSET_LUT         (0x9<<4)

#define  OFFSET_EN                     (1<<0)
#define  OFFSET_ATUTO                  (1<<1)
#define  OFFSET_IDX                    (1<<2)
#define  OFFSET_BK_EN                  (1<<8)
#define  OFFSET_BK_AUTO                (1<<9)
#define  OFFSET_BK_IDX                 (1<<10)
#define  OFFSET_CURRENT                (1<<16)
#define  LP_MODE                       (1<<31)

#define  RTC_READ 1
#define  RTC_WRITE 2
#define  RTC_TEMP 3
#define  FH_RTC_INT_STATUS_RX_CRC_ERR   (1<<0)
#define  FH_RTC_INT_STATUS_RX_COM_ERR   (1<<1)
#define  FH_RTC_INT_STATUS_RX_LEN_ERR   (1<<2)
#define  FH_RTC_INT_STATUS_CNT_THL	    (1<<3)
#define  FH_RTC_INT_STATUS_CNT_THH	    (1<<4)
#define  FH_RTC_INT_STATUS_CORE_IDLE	(1<<5)
#define  FH_RTC_INT_STATUS_CORE		    (1<<6)
#define  FH_RTC_INT_STATUS_WRAPPER_BUSY (1<<8)
#define  FH_RTC_INT_STATUS_CORE_BUSY    (1<<16)

#define  FH_RTC_INT_RX_CRC_ERR_EN        (1<<0)
#define  FH_RTC_INT_RX_COM_ERR_EN        (1<<1)
#define  FH_RTC_INT_RX_LEN_ERR_EN        (1<<2)
#define  FH_RTC_INT_CNT_THL_ERR_EN       (1<<3)
#define  FH_RTC_INT_CNT_THH_ERR_EN       (1<<4)
#define  FH_RTC_INT_CORE_IDLE_ERR_EN     (1<<5)
#define  FH_RTC_INT_CORE_INT_ERR_EN      (1<<6)
#define  FH_RTC_INT_RX_CRC_ERR_MASK      (1<<16)
#define  FH_RTC_INT_RX_COM_ERR_MASK      (1<<17)
#define  FH_RTC_INT_RX_LEN_ERR_MASK      (1<<18)
#define  FH_RTC_INT_CNT_THL_ERR_MASK     (1<<19)
#define  FH_RTC_INT_CNT_THH_ERR_MASK     (1<<20)
#define  FH_RTC_INT_CORE_IDLE_ERR_MASK   (1<<21)
#define  FH_RTC_INT_CORE_INT_ERR_MASK    (1<<22)
#define  FH_RTC_INT_CORE_INT_ERR_MASK_COV    0xffbfffff
#define  FH_RTC_INT_CORE_INT_STATUS_COV    0xffffff3f
#define  FH_RTC_INT_CORE_INT_ALL_COV    0xffffffff

#define FH_RTC_CORE_INT_EN_SEC_INT       (0x1<<0)
#define FH_RTC_CORE_INT_EN_MIN_INT       (0x1<<1)
#define FH_RTC_CORE_INT_EN_HOU_INT       (0x1<<2)
#define FH_RTC_CORE_INT_EN_DAY_INT       (0x1<<3)
#define FH_RTC_CORE_INT_EN_ALM_INT       (0x1<<4)
#define FH_RTC_CORE_INT_EN_POW_INT       (0x1<<5)


#define FH_RTC_CORE_INT_EN_SEC_MAS       (0x1<<16)
#define FH_RTC_CORE_INT_EN_MIN_MAS       (0x1<<17)
#define FH_RTC_CORE_INT_EN_HOU_MAS       (0x1<<18)
#define FH_RTC_CORE_INT_EN_DAY_MAS       (0x1<<19)
#define FH_RTC_CORE_INT_EN_ALM_MAS       (0x1<<20)
#define FH_RTC_CORE_INT_EN_POE_MAS       (0x1<<21)

/* #define FH_RTC_DEBUG_PRINT */

#ifdef FH_RTC_DEBUG_PRINT
#define RTC_PRINT_DBG(fmt, args...) \
	do {
	pr_info("[FH_RTC_DEBUG]: ");
	printk(fmt, ## args)
	} while (0)
#else
#define RTC_PRINT_DBG(fmt, args...)  do { } while (0)
#endif

#ifdef CONFIG_USE_TSENSOR
#define USE_TSENSOR

#ifdef CONFIG_USE_TSENSOR_OFFSET
#define USE_TSENSOR_OFFSET
#endif

#endif

struct fh_rtc_controller {
	void * __iomem regs;
	unsigned int v_addr;
	unsigned int irq;
	unsigned int paddr;
	unsigned int base_year;
	unsigned int base_month;
	unsigned int base_day;
	struct rtc_device *rtc;
	struct clk *clk;
	int sadc_channel;

	struct workqueue_struct *wq;
	struct delayed_work self_adjust;
	int tsensor_delta_val;
	struct fh_rtc_plat_data *plat_data;
};
struct fh_rtc_core_int_status {
	unsigned int core_int_en;
	unsigned int core_int_status;
};
static struct fh_rtc_controller *fh_rtc;
struct fh_rtc_core_int_status fh_core_int;
enum {

	TIME_FUNC = 0, ALARM_FUNC,

};
#ifdef USE_TSENSOR

unsigned int TSENSOR_LUT[12] = {0x1b1e2023,
								0x11131618,
								0x090b0d0f,
								0x03040607,
								0x00010202,
								0x01000000,
								0x04030201,
								0x0b090706,
								0x1713100e,
								0x27221e1a,
								0x3e37322c,
								0x5b534c44};
#endif



static int fh_rtc_get_core(unsigned int base_addr, unsigned int reg_num);
#ifdef USE_TSENSOR_OFFSET

int tsensor_adjust_lut(void)
{
	unsigned int tsensor_init_data;
	unsigned int tsensor_12bit;
	int temp_diff;
	int tsensor_out_value_diff;
	int lut_cof = fh_rtc->plat_data->lut_cof;
#if defined(CONFIG_ARCH_FH885xV200) || \
defined(CONFIG_ARCH_FH865x)
	tsensor_init_data = fh_pmu_get_tsensor_init_data();
#else
	unsigned int low_data, high_data;

	low_data =  efuse_read_debug_entry(60);
	high_data =  efuse_read_debug_entry(61);
	tsensor_init_data = (high_data<<8)|low_data;
#endif
	tsensor_12bit = tsensor_init_data&0xfff;
	tsensor_out_value_diff = tsensor_12bit
		- fh_rtc->plat_data->tsensor_cp_default_out;
	fh_rtc->tsensor_delta_val = tsensor_out_value_diff;

	temp_diff = tsensor_out_value_diff*lut_cof/4096;

	return temp_diff;
}
#endif
#ifdef USE_TSENSOR
static int fh_rtc_temp_cfg_coef_offset(unsigned int coef, unsigned int offset);
static int fh_rtc_temp_cfg_thl_thh(unsigned int thl, unsigned int thh);
#endif
static int fh_rtc_set_core(unsigned int base_addr, unsigned int reg_num,
		unsigned int value);
#ifdef USE_TSENSOR
static void rtc_adjust(void)
{
	int i;
	int offset_index;
	struct fh_rtc_plat_data *pd = fh_rtc->plat_data;

	char offset_lut[48];

	for (i = 0; i < 12; i++)
		fh_rtc_set_core(fh_rtc->v_addr,
			FH_RTC_CMD_OFFSET_LUT+(i<<4),
			TSENSOR_LUT[i]);

	memcpy(offset_lut, TSENSOR_LUT, sizeof(offset_lut));

	offset_index = 0;
	for (i = 0; i < 46; i++) {
		if (offset_lut[i] > offset_lut[i+1])
			offset_index = i + 1;
		else
			offset_lut[i+1] = offset_lut[i];
	}

#ifdef USE_TSENSOR_OFFSET
	i = tsensor_adjust_lut();
	if ((i < 0) || (i > 5))
		i = 0;
	fh_rtc_temp_cfg_coef_offset(pd->lut_cof, pd->lut_offset-i);
	pr_info("tsensor diff value : %d\n", i);
#else
	fh_rtc_temp_cfg_coef_offset(pd->lut_cof, pd->lut_offset);
#endif
	fh_rtc_temp_cfg_thl_thh(0, 47);
	fh_rtc_set_core(fh_rtc->v_addr,
		FH_RTC_CMD_OFFSET,
		(offset_index<<2) | (offset_index<<10)|
		OFFSET_BK_EN|OFFSET_BK_AUTO);
}
#endif

static inline void rtc_udelay(int us)
{
#ifdef CONFIG_HIGH_RES_TIMERS
	usleep_range(us, us+20);
#else
	udelay(us);
#endif
}

static int fh_rtc_core_idle(unsigned int base_addr)
{
	unsigned int status;

	status = readl(fh_rtc->regs+FH_RTC_INT_STATUS);
	if (status & FH_RTC_INT_STATUS_CORE_IDLE)
		return 0;
	else
		return -1;
}

static int fh_rtc_core_wr(unsigned int base_addr)
{
	int reg;

	reg = readl(fh_rtc->regs+FH_RTC_INT_STATUS);
	reg &= (~FH_RTC_INT_STATUS_CORE_IDLE);
	writel(reg, fh_rtc->regs+FH_RTC_INT_STATUS);

	return 0;
}

static int fh_rtc_get_time(unsigned int base_addr)
{

	int reg, count, status;

	status = fh_rtc_core_idle(base_addr);
	if (status < 0) {
		pr_info("fh rtc get time error %x\n", status);
		return -1;
	}
	fh_rtc_core_wr(base_addr);
	writel(RTC_READ, fh_rtc->regs+FH_RTC_CMD);
	for (count = 0; count < 250; count++) {
		status = fh_rtc_core_idle(base_addr);
		if (status == 0) {
			reg = readl((void * __iomem)base_addr+FH_RTC_RD_DATA);
			return reg;
		}
		rtc_udelay(100);

	}
	pr_info("rtc core busy can't get time\n");

	return -1;
}

static int fh_rtc_set_core(unsigned int base_addr, unsigned int reg_num,
		unsigned int value)
{
	int count, status;

	if (fh_rtc_core_idle(base_addr) < 0) {
		pr_info("rtc get time:rtc core busy\n");
		return -1;
	}
	fh_rtc_core_wr(base_addr);

	writel(value, (void * __iomem)base_addr+FH_RTC_WR_DATA);
	writel(reg_num|RTC_WRITE, (void * __iomem)base_addr+FH_RTC_CMD);
	for (count = 0; count < 25; count++) {
		status = fh_rtc_core_idle(base_addr);
		if (status == 0)
			return 0;

		mdelay(1);

	}
	pr_info("rtc SET CORE REG TIMEOUT\n");

	return -1;
}

static int fh_rtc_get_core(unsigned int base_addr, unsigned int reg_num)
{
	int reg, count, status;

	if (fh_rtc_core_idle(base_addr) < 0) {
		pr_err("rtc get time:rtc core busy %d\n", __LINE__);
		return -1;
	}
	fh_rtc_core_wr(base_addr);
	writel(reg_num|RTC_READ, (void * __iomem)base_addr+FH_RTC_CMD);
	for (count = 0; count < 150; count++) {
		status = fh_rtc_core_idle(base_addr);
		if (status == 0) {
			reg = readl((void * __iomem)base_addr+FH_RTC_RD_DATA);
			return reg;
		}
		rtc_udelay(100);

	}
	pr_info("rtc GET CORE REG TIMEOUT line %d\n", __LINE__);

	return -1;
}

#if defined(CONFIG_MACH_FH8856) || defined(CONFIG_MACH_FH8852)
static int fh_rtc_get_temp(unsigned int base_addr)
{
	return 0;
}
#else
static int fh_rtc_get_temp(unsigned int base_addr)
{
	int reg, count, status;

	if (fh_rtc_core_idle(base_addr) < 0) {
		pr_info("rtc get time:rtc core busy %d\n", __LINE__);
		return -1;
	}
	fh_rtc_core_wr(base_addr);
	writel(RTC_TEMP, (void * __iomem)base_addr+FH_RTC_CMD);
	for (count = 0; count < 150; count++) {
		status = fh_rtc_core_idle(base_addr);
		if (status == 0) {
			reg = readl((void * __iomem)base_addr+FH_RTC_RD_DATA);
			return reg;
		}
		rtc_udelay(100);

	}
	pr_err("rtc GET CORE REG TIMEOUT line %d\n", __LINE__);

	return -1;
}
#endif

/* the temp[out] is 10 times of actually temperature */
int fh_rtc_get_tsensor_data(int *data, int *temp)
{
	struct fh_rtc_plat_data *pd = fh_rtc->plat_data;
	int raw_data = -1, t, status, count;
	int offset = (signed char)pd->lut_offset;

	fh_rtc_get_temp(fh_rtc->v_addr);

	if (fh_rtc_core_idle(fh_rtc->v_addr) < 0) {
		pr_err("rtc get time:rtc core busy %d\n", __LINE__);
		return -1;
	}
	fh_rtc_core_wr(fh_rtc->v_addr);
	writel(FH_RTC_CMD_TEMP_INFO|RTC_READ,
			(void * __iomem)fh_rtc->v_addr+FH_RTC_CMD);
	for (count = 0; count < 30; count++) {
		status = fh_rtc_core_idle(fh_rtc->v_addr);
		if (status == 0) {
			raw_data = readl((void * __iomem)
					fh_rtc->v_addr+FH_RTC_RD_DATA);
			break;
		}
		mdelay(1);
	}

	if (raw_data < 0)
		return raw_data;

	if (data)
		*data = raw_data;
	raw_data += fh_rtc->tsensor_delta_val;
	/* data * COF >> 12 + OFFSET = lut_index
	 *		= (temp - (-50)) * 47 / (120 - (-50))
	 */
	t = (raw_data * pd->lut_cof * 10 / 4096 + offset * 10) * 170 / 47 - 500;
	if (temp)
		*temp = t;
	return 0;
}
EXPORT_SYMBOL(fh_rtc_get_tsensor_data);

static int fh_rtc_set_time(unsigned int base_addr, unsigned int value)
{
	int count;

	if (fh_rtc_core_idle(base_addr) < 0) {
		pr_err("set time :rtc core busy\n");
		return -1;
	}
	fh_rtc_core_wr(base_addr);
	writel(value, (void * __iomem)base_addr+FH_RTC_WR_DATA);
	writel(RTC_WRITE, (void * __iomem)base_addr+FH_RTC_CMD);
	for (count = 0; count < 150; count++) {
		if (fh_rtc_core_idle(base_addr) == 0)
			return 0;
		rtc_udelay(100);
	}
	pr_err("rtc core busy can't set time\n");

	return -1;
}
#ifdef DRIVER_TEST
static int fh_rtc_set_time_first(unsigned int base_addr, unsigned int value)
{

	writel(value, base_addr+FH_RTC_WR_DATA);
	writel(RTC_WRITE, base_addr+FH_RTC_CMD);

}
#endif

static int fh_rtc_alarm_enable_interrupt(unsigned int base_addr)
{
	int status;

	status = fh_rtc_get_core(fh_rtc->v_addr,
		FH_RTC_CMD_INT_STATUS);

	fh_rtc_set_core(fh_rtc->v_addr,
		FH_RTC_CMD_INT_STATUS,
		status & (~FH_RTC_ISR_ALARM_POS));

	status = fh_rtc_get_core(fh_rtc->v_addr,
		FH_RTC_CMD_INT_EN);
	fh_rtc_set_core(fh_rtc->v_addr,
		FH_RTC_CMD_INT_EN,
			(status | FH_RTC_CORE_INT_EN_ALM_INT));

	status = readl((void *)fh_rtc->v_addr+FH_RTC_INT_STATUS);
	writel(status & (~FH_RTC_INT_STATUS_CORE),
			(void *)fh_rtc->v_addr+FH_RTC_INT_STATUS);

	status = readl((void *)fh_rtc->v_addr+FH_RTC_INT_EN);
	writel(status &	(~FH_RTC_INT_CORE_INT_ERR_MASK),
			(void *)fh_rtc->v_addr+FH_RTC_INT_EN);
	return 0;
}

static int fh_rtc_alarm_disable_interrupt(unsigned int base_addr,
	unsigned int value)
{
	int rtc_core_status;

	rtc_core_status = fh_rtc_get_core(fh_rtc->v_addr,
	FH_RTC_CMD_INT_EN);
	fh_rtc_set_core(fh_rtc->v_addr,
	FH_RTC_CMD_INT_EN, (rtc_core_status & (~FH_RTC_CORE_INT_EN_ALM_INT)));

	return 0;
}
static int fh_rtc_set_alarm_time(unsigned int base_addr, unsigned int value)
{
	int status;

#ifdef USE_DEBUG_REGISTER
	int rtc_core_status, cys_count;

	rtc_core_status = fh_rtc_get_core(fh_rtc->v_addr,
		FH_RTC_CMD_COUNTER);
	if (rtc_core_status > value) {
		pr_info("alarm time > now time\n");
		return -ETIME;
	}
#endif
	status =  fh_rtc_set_core(base_addr,
		FH_RTC_CMD_ALARM_CFG, value);
	if (status < 0) {
		pr_info("set alarm time failed\n");

		return -1;
	}
#ifdef USE_DEBUG_REGISTER
	if (rtc_core_status == value) {
		cys_count = fh_rtc_get_core(fh_rtc->v_addr,
				FH_RTC_CMD_DEBUG);
		if (cys_count > 0x3e00)
			return -ETIME;
	}
#endif
	return 0;
}

static int fh_rtc_get_alarm_time(unsigned int base_addr)
{
	int data;

	data = fh_rtc_get_core(base_addr, FH_RTC_CMD_ALARM_CFG);

	return data;
}
static unsigned int fh_rtc_get_hw_sec_data(unsigned int func_switch)
{

	unsigned int ret_sec, raw_value, sec_value;
	unsigned int min_value, hour_value, day_value;

	if (func_switch == TIME_FUNC)
		raw_value = fh_rtc_get_time((unsigned int)fh_rtc->regs);
	else
		raw_value = fh_rtc_get_alarm_time((unsigned int)fh_rtc->regs);

	sec_value = FH_GET_RTC_SEC(raw_value);
	min_value = FH_GET_RTC_MIN(raw_value);
	hour_value = FH_GET_RTC_HOUR(raw_value);
	day_value = FH_GET_RTC_DAY(raw_value);
	ret_sec = (day_value * 86400) + (hour_value * 3600) + (min_value * 60)
		+ sec_value;

	return ret_sec;
}

#ifdef DRIVER_TEST
#define TEMP_DATA 0x203
static unsigned int fh_rtc_get_tmp(void)
{
	unsigned int value;

	value = GET_REG((unsigned int)fh_rtc->regs + FH_RTC_CMD_TEMP_INFO);

	return value;
}
static void fh_rtc_tmp_en(void)
{
	fh_rtc_set_core((unsigned int)fh_rtc->regs, FH_RTC_CMD_OFFSET,
		TEMP_DATA);
}
#endif
static int fh_rtc_set_hw_sec_data(struct rtc_time *rtc_tm,
	unsigned int func_switch)
{
	unsigned int raw_value, sec_value, min_value;
	unsigned int hour_value, day_value;
	unsigned long now;
	int status;

	if (func_switch == ALARM_FUNC) {
		rtc_tm_to_time(rtc_tm, &now);
		now--;
		rtc_time_to_tm(now, rtc_tm);
	}

	day_value = rtc_year_days(rtc_tm->tm_mday, rtc_tm->tm_mon,
		rtc_tm->tm_year + 1900);
	day_value += (rtc_tm->tm_year - 70)
		* 365 + ELAPSED_LEAP_YEARS(rtc_tm->tm_year);

	hour_value = rtc_tm->tm_hour;
	min_value = rtc_tm->tm_min;
	sec_value = rtc_tm->tm_sec;

	raw_value = (day_value << DAY_BIT_START) |
		(hour_value << HOUR_BIT_START)
		| (min_value << MIN_BIT_START) |
		(sec_value << SEC_BIT_START);

	if (func_switch == TIME_FUNC) {
		status = fh_rtc_set_time((unsigned int)fh_rtc->regs, raw_value);
	} else {
		status = fh_rtc_set_alarm_time((unsigned int)fh_rtc->regs,
				raw_value);
	}
	return status;
}

static int fh_rtc_open(struct device *dev)
{
	return 0;
}

static void fh_rtc_release(struct device *dev)
{
}

#ifdef DRIVER_TEST
static int fh_rtc_tm_compare(struct rtc_time *tm0, struct rtc_time *tm1)
{
	unsigned long read = 0, write = 0;

	rtc_tm_to_time(tm0, &read);
	rtc_tm_to_time(tm1, &write);

	if (write > read || write < read - 2) {
		RTC_PRINT_DBG(
			"ERROR: read(%d-%d-%d %d:%d:%d) vs write(%d-%d-%d %d:%d:%d)\n",
			tm0->tm_year + 1900, tm0->tm_mon + 1,
			tm0->tm_mday,
			tm0->tm_hour, tm0->tm_min,
			tm0->tm_sec, tm1->tm_year + 1900,
			tm1->tm_mon + 1, tm1->tm_mday,
			tm1->tm_hour, tm1->tm_min,
			tm1->tm_sec);
		return -1;
	}

	return 0;
}
#endif
static int fh_rtc_gettime_nosync(struct device *dev, struct rtc_time *rtc_tm)
{
	unsigned int temp;

	temp = fh_rtc_get_hw_sec_data(TIME_FUNC);
	rtc_time_to_tm(temp, rtc_tm);
	RTC_PRINT_DBG("rtc read date:0x%x\n", temp);

	return 0;
}

static int fh_rtc_settime(struct device *dev, struct rtc_time *tm)
{
	RTC_PRINT_DBG("rtc write %d-%d-%d %d:%d:%d\n", tm->tm_year + 1900,
		tm->tm_mon + 1, tm->tm_mday, tm->tm_hour,
		tm->tm_min, tm->tm_sec);

	fh_rtc_set_hw_sec_data(tm, TIME_FUNC);

	return 0;
}

static int fh_rtc_getalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct rtc_time *rtc_tm = &alrm->time;

	rtc_time_to_tm(fh_rtc_get_hw_sec_data(ALARM_FUNC), rtc_tm);

	return 0;
}

static int fh_rtc_setalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct rtc_time *rtc_tm = &alrm->time;
	int status;

	status = fh_rtc_set_hw_sec_data(rtc_tm, ALARM_FUNC);
	if (status)
		return status;
	if (alrm->enabled)
		fh_rtc_alarm_enable_interrupt(fh_rtc->v_addr);
	else
		fh_rtc_alarm_disable_interrupt(fh_rtc->v_addr,
			 FH_RTC_ISR_ALARM_POS);
	return 0;
}

#ifdef DRIVER_TEST
static int dump_interrput_status(unsigned int base_addr)
{
	int status;

	status = fh_rtc_get_core(fh_rtc->v_addr,
		FH_RTC_CMD_INT_STATUS);
	pr_info("core int status : %x\n", status);

	status = fh_rtc_get_core(fh_rtc->v_addr,
		FH_RTC_CMD_INT_EN);
	pr_info("core int en : %x\n", status);
	status = readl(fh_rtc->v_addr+FH_RTC_INT_STATUS);
	pr_info("wrapper int status : 0x%x\n", status);
	status = readl(fh_rtc->v_addr+FH_RTC_INT_EN);
	pr_info("wrapper int en : 0x%x\n", status);
	return 0;
}
#endif
static int fh_rtc_irq_enable(struct device *dev, unsigned int enabled)
{

	struct platform_device *pdev = to_platform_device(dev);
	struct fh_rtc_controller *fh_rtc = platform_get_drvdata(pdev);
	int status;

	if (enabled) {
		status = fh_rtc_get_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS);

		fh_rtc_set_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS,
			status & (~FH_RTC_ISR_ALARM_POS));

		status = fh_rtc_get_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_EN);
		fh_rtc_set_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_EN,
			(status | FH_RTC_CORE_INT_EN_ALM_INT));

		status = readl((void * __iomem)fh_rtc->v_addr+
				FH_RTC_INT_STATUS);
		writel(status&
			(~FH_RTC_INT_STATUS_CORE),
			(void * __iomem)fh_rtc->v_addr+
			FH_RTC_INT_STATUS);

		status = readl((void * __iomem)fh_rtc->v_addr +
			FH_RTC_INT_EN);
		writel(status &
			(~FH_RTC_INT_CORE_INT_ERR_MASK),
			(void * __iomem)fh_rtc->v_addr +
			FH_RTC_INT_EN);

	} else
		fh_rtc_alarm_disable_interrupt(fh_rtc->v_addr,
			 FH_RTC_ISR_ALARM_POS);

	return 0;
}

static void fh_rtc_clear_interrupt_status(unsigned int base_addr,
		unsigned int value)
{
	unsigned int int_en, int_status;

	int_en = readl((void * __iomem)base_addr+FH_RTC_INT_EN);
	int_en &= 0x7f;
	writel((((value)
		& 0x7f)<<16)|int_en|
		FH_RTC_INT_CORE_IDLE_ERR_MASK|
		FH_RTC_INT_CORE_INT_ERR_MASK,
		(void * __iomem)base_addr+FH_RTC_INT_EN);
	int_status = value & 0x10a0;
	writel(int_status, (void * __iomem)base_addr+
		FH_RTC_INT_STATUS);
}
static struct work_struct rtc_int_wq;

static void rtc_core_int_handler(struct work_struct *work)
{
	unsigned long events = 0;
	int rtc_int_status, rtc_int_en;
	unsigned int wr_int_status;
	unsigned int wr_int_status_clr;
	int rtc_core_status;
	struct rtc_device *rtc = fh_rtc->rtc;

	/* get core interrupt status */
	rtc_int_en = fh_rtc_get_core(fh_rtc->v_addr,
		FH_RTC_CMD_INT_EN);
	wr_int_status_clr = readl((void * __iomem)fh_rtc->v_addr +
		FH_RTC_INT_STATUS);
	rtc_int_status = fh_rtc_get_core(fh_rtc->v_addr,
		FH_RTC_CMD_INT_STATUS);

	/* get core interrupt failed */
	if (rtc_int_en < 0) {
		pr_err("can't get core int en\n");
		return;
	}
	if (rtc_int_status < 0) {
		pr_err("can't get core int status\n");
		return;
	}
	/* do interrupt */
	if (rtc_int_en & rtc_int_status & FH_RTC_ISR_ALARM_POS) {
		pr_info("ALARM INT\n");
		/* clear core alarm interrupt status */
		rtc_core_status = fh_rtc_get_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS);
		fh_rtc_set_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS,
			rtc_core_status &
			(~FH_RTC_ISR_ALARM_POS));
		/* eanble interrupt */
		wr_int_status = readl((void * __iomem)fh_rtc->v_addr+
			FH_RTC_INT_EN);
		writel(wr_int_status&
			(FH_RTC_INT_CORE_INT_ALL_COV),
			(void * __iomem)fh_rtc->v_addr+FH_RTC_INT_EN);
		/* updata to RTC CORE alarm work */
		events |= RTC_IRQF | RTC_AF;
		rtc_update_irq(rtc, 1, events);

	} else if (rtc_int_en & rtc_int_status &
			FH_RTC_ISR_SEC_POS) {

		pr_info("FH_RTC_ISR_SEC_POS\n");
		rtc_core_status = fh_rtc_get_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS);
		fh_rtc_set_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS,
			rtc_core_status & (~FH_RTC_ISR_SEC_POS));
		wr_int_status = readl((void * __iomem)fh_rtc->v_addr+
			FH_RTC_INT_EN);

		writel(wr_int_status&
			(FH_RTC_INT_CORE_INT_STATUS_COV),
			(void * __iomem)fh_rtc->v_addr + FH_RTC_INT_EN);

		events |= RTC_IRQF | RTC_UF;
		rtc_update_irq(rtc, 1, events);

	} else if (rtc_int_en & rtc_int_status &
			FH_RTC_ISR_MIN_POS) {

		pr_info("FH_RTC_ISR_MIN_POS\n");
		rtc_core_status = fh_rtc_get_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS);

		fh_rtc_set_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS,
			rtc_core_status & (~FH_RTC_ISR_MIN_POS));
		wr_int_status = readl((void * __iomem)fh_rtc->regs+
			FH_RTC_INT_EN);

		writel(wr_int_status&
			(FH_RTC_INT_CORE_INT_ERR_MASK_COV),
			(void * __iomem)fh_rtc->v_addr + FH_RTC_INT_EN);

		writel(wr_int_status_clr&
			(FH_RTC_INT_CORE_INT_STATUS_COV),
			(void * __iomem)fh_rtc->v_addr + FH_RTC_INT_STATUS);

	} else if (rtc_int_en & rtc_int_status & FH_RTC_ISR_HOUR_POS) {
		pr_info("FH_RTC_ISR_HOUR_POS\n");
		rtc_core_status = fh_rtc_get_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS);

		fh_rtc_set_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS,
			rtc_core_status & (~FH_RTC_ISR_HOUR_POS));
		wr_int_status = readl((void * __iomem)fh_rtc->v_addr+
			FH_RTC_INT_EN);

		writel(wr_int_status&
			(FH_RTC_INT_CORE_INT_ERR_MASK_COV),
			(void * __iomem)fh_rtc->v_addr + FH_RTC_INT_EN);

		writel(wr_int_status_clr&
			(FH_RTC_INT_CORE_INT_STATUS_COV),
			(void * __iomem)fh_rtc->v_addr + FH_RTC_INT_STATUS);
	} else if (rtc_int_en & rtc_int_status & FH_RTC_ISR_DAY_POS) {
		pr_info("FH_RTC_ISR_DAY_POS\n");
		rtc_core_status = fh_rtc_get_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS);

		fh_rtc_set_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS,
			rtc_core_status & (~FH_RTC_ISR_DAY_POS));
		wr_int_status = readl(fh_rtc->regs+
			FH_RTC_INT_EN);

		writel(wr_int_status&
			(FH_RTC_INT_CORE_INT_ERR_MASK_COV),
			(void * __iomem)fh_rtc->regs + FH_RTC_INT_EN);

		writel(wr_int_status_clr&
			(FH_RTC_INT_CORE_INT_STATUS_COV),
			(void * __iomem)fh_rtc->regs + FH_RTC_INT_STATUS);
	} else if (rtc_int_en & rtc_int_status &
			FH_RTC_ISR_POWERFAIL_POS) {
		pr_info("FH_RTC_ISR_POWERFAIL_POS\n");
		rtc_core_status = fh_rtc_get_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS);
		fh_rtc_set_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS,
			rtc_core_status &
			(~FH_RTC_ISR_POWERFAIL_POS));

		wr_int_status = readl((void * __iomem)fh_rtc->regs +
			FH_RTC_INT_EN);

		writel(wr_int_status_clr&
			(FH_RTC_INT_CORE_INT_STATUS_COV),
			(void * __iomem)fh_rtc->v_addr + FH_RTC_INT_STATUS);

	} else if (rtc_int_en & rtc_int_status & FH_RTC_ISR_RX_CRC_ERR_INT) {
		pr_info("FH_RTC_ISR_RX_CRC_ERR_INT\n");
	} else if (rtc_int_en & rtc_int_status & FH_RTC_ISR_RX_COM_ERR_INT) {
		pr_info("FH_RTC_ISR_RX_COM_ERR_INT\n");
	} else if (rtc_int_en & rtc_int_status & FH_RTC_LEN_ERR_INT) {
		pr_info("FH_RTC_LEN_ERR_INT\n");
	} else {
		pr_info("unused isr\n");
		fh_rtc_set_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS, rtc_int_en & (~rtc_int_status));
	}


}

static irqreturn_t fh_rtc_irq(int irq, void *dev_id)
{
	struct fh_rtc_controller *fh_rtc = (struct fh_rtc_controller *) dev_id;
	unsigned int isr_status;

	isr_status = readl((void * __iomem)fh_rtc->v_addr+FH_RTC_INT_STATUS);
	fh_rtc_clear_interrupt_status(fh_rtc->v_addr, isr_status);

	if (isr_status & FH_RTC_INT_STATUS_CORE) {
		schedule_work(&rtc_int_wq);
	} else if (isr_status & FH_RTC_INT_STATUS_RX_CRC_ERR)
		pr_info("FH_RTC_INT_STATUS_RX_CRC_ERR\n");
	else if (isr_status & FH_RTC_INT_STATUS_RX_COM_ERR) {
		pr_info("FH_RTC_INT_STATUS_RX_COM_ERR\n");
	} else if (isr_status & FH_RTC_INT_STATUS_RX_LEN_ERR) {
		pr_info("FH_RTC_INT_STATUS_RX_LEN_ERR\n");
	} else if (isr_status & FH_RTC_INT_STATUS_CNT_THL) {
		pr_info("FH_RTC_INT_STATUS_CNT_THL\n");
	} else if (isr_status & FH_RTC_INT_STATUS_CNT_THH) {
		pr_info("FH_RTC_INT_STATUS_CNT_THH\n");
	} else if (isr_status & FH_RTC_INT_STATUS_CORE_IDLE) {
		pr_info("FH_RTC_INT_STATUS_CORE_IDLE\n");
	} else if (isr_status & FH_RTC_INT_STATUS_WRAPPER_BUSY) {
		pr_info("FH_RTC_INT_STATUS_WRAPPER_BUSY\n");
	} else if (isr_status & FH_RTC_INT_STATUS_CORE_BUSY) {
		pr_info("FH_RTC_INT_STATUS_CORE_BUSY\n");
	} else {
		pr_info("rtc unknown isr...\n");
		return IRQ_NONE;
	}

	return IRQ_HANDLED;

}

static const struct rtc_class_ops fh_rtcops = {
	.open = fh_rtc_open,
	.release = fh_rtc_release,
	.read_time = fh_rtc_gettime_nosync,
	.set_time = fh_rtc_settime,
	.read_alarm = fh_rtc_getalarm,
	.set_alarm = fh_rtc_setalarm,
	.alarm_irq_enable = fh_rtc_irq_enable,
};

/*get the read of SADC and adjust RTC clock*/
static struct miscdevice fh_rtc_misc;


static struct of_device_id const fh_rtc_of_match[] = {
	{ .compatible = "fh,fh_rtc" },
	{}
};
#ifdef CONFIG_EMULATION
static int fh_rtc_probe(struct platform_device *pdev)
{
	return 0;
}
#else
static int fh_rtc_probe(struct platform_device *pdev)
{
	int err = 0;
	struct rtc_device *rtc;
	int rtc_core_status;
	struct fh_rtc_plat_data *plat_data;
#ifdef CONFIG_USE_OF
	const struct of_device_id *match;
	unsigned int paddr;
	struct device_node *np = pdev->dev.of_node;
#else
	struct resource *res;
#endif

	fh_rtc = kzalloc(sizeof(struct fh_rtc_controller), GFP_KERNEL);
	if (!fh_rtc)
		return -ENOMEM;
	memset(fh_rtc, 0, sizeof(struct fh_rtc_controller));

#ifdef CONFIG_USE_OF
	match = of_match_device(fh_rtc_of_match, &pdev->dev);
	if (!match) {
		pr_err("failed to find rtc controller\n");
		return -ENODEV;
	}
	plat_data = devm_kzalloc(&pdev->dev, sizeof(*plat_data), GFP_KERNEL);
	if (plat_data == NULL)
		return -ENOMEM;

	fh_rtc->regs = of_iomap(np, 0);
		if (fh_rtc->regs == NULL) {
			err = -ENXIO;
			return err;
		}

	err = of_property_read_u32(np, "lut_cof", &plat_data->lut_cof);
	err |= of_property_read_u32(np, "lut_offset", &plat_data->lut_offset);
	err |= of_property_read_u32(np, "tsensor_cp_default_out",
		&plat_data->tsensor_cp_default_out);

	if (err) {
		dev_err(&pdev->dev, "need lut/tsensor params\n");
		return -EINVAL;
	}

	if (of_find_property(np, "clocks", NULL))
		fh_rtc->clk = of_clk_get(np, 0);

	fh_rtc->irq = irq_of_parse_and_map(np, 0);
	if (fh_rtc->irq < 0) {
		dev_err(&pdev->dev, "no irq resource\n");
		goto err_nores;
	}
	if (of_property_read_u32(np, "reg", &paddr))
		//goto err_nores;
		dev_err(&pdev->dev, "get rtc reg failed\n");
	fh_rtc->paddr = paddr;
#else
	plat_data = (struct fh_rtc_plat_data *)dev_get_platdata(&pdev->dev);

	if (plat_data->clk_name)
		fh_rtc->clk = clk_get(NULL, plat_data->clk_name);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	fh_rtc->regs = devm_ioremap(&pdev->dev, res->start,
			resource_size(res));
	fh_rtc->irq = irq_create_mapping(NULL, platform_get_irq(pdev, 0));
	fh_rtc->paddr = res->start;
#endif
	fh_rtc->plat_data = plat_data;

	if (fh_rtc->clk) {
		if (IS_ERR(fh_rtc->clk)) {
			err = PTR_ERR(fh_rtc->clk);
			dev_err(&pdev->dev, "no clk resource\n");
			goto err_nores;
		} else
			clk_prepare_enable(fh_rtc->clk);
	}

	err = request_irq(fh_rtc->irq,
		fh_rtc_irq, IRQF_NO_THREAD, "fh_rtc", fh_rtc);
	if (err) {
		dev_err(&pdev->dev, "failure requesting irq %i\n", fh_rtc->irq);
		goto err_nores;
	}
	fh_rtc->v_addr = (unsigned int)fh_rtc->regs;

	writel(
		FH_RTC_INT_CORE_INT_ERR_EN|
		FH_RTC_INT_CORE_IDLE_ERR_EN |
		FH_RTC_INT_CORE_INT_ERR_MASK |
		FH_RTC_INT_CORE_IDLE_ERR_MASK,
		(fh_rtc->regs+FH_RTC_INT_EN)
		);
	writel(
		FH_RTC_INT_STATUS_CORE |
		FH_RTC_INT_STATUS_CORE_IDLE,
		fh_rtc->regs+FH_RTC_INT_STATUS);

	writel(0x20, fh_rtc->regs+FH_RTC_INT_STATUS);
	mdelay(1);
	rtc_core_status = fh_rtc_get_core(fh_rtc->v_addr,
		FH_RTC_CMD_INT_STATUS);
	if (rtc_core_status & FH_RTC_ISR_POWERFAIL_POS) {
		RTC_PRINT_DBG("rtc powerfailed !\n");
		fh_rtc_set_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS,
			rtc_core_status &
			(~FH_RTC_ISR_POWERFAIL_POS));
	}
	/* clear all core's irq status an enable */
	fh_rtc_set_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_EN,
			rtc_core_status &
			(~FH_RTC_INT_CORE_INT_ALL_COV));
	fh_rtc_set_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS,
			rtc_core_status &
			(~FH_RTC_INT_CORE_INT_ALL_COV));
	fh_rtc_get_temp(fh_rtc->v_addr);

	platform_set_drvdata(pdev, fh_rtc);
	rtc = devm_rtc_device_register(&pdev->dev, "fh_rtc", &fh_rtcops,
		THIS_MODULE);
	INIT_WORK(&rtc_int_wq, rtc_core_int_handler);

	if (IS_ERR(rtc)) {
		dev_err(&pdev->dev, "cannot attach rtc\n");
		err = PTR_ERR(rtc);
		goto err_nores;
	}
	fh_rtc->rtc = rtc;

	err = misc_register(&fh_rtc_misc);
	if (err < 0)
		dev_dbg(&pdev->dev, "register rtc misc device error\n");

#ifdef USE_TSENSOR
	rtc_adjust();
#endif

	return 0;

err_nores:
	kfree(fh_rtc);
	return err;
}
#endif

static int  fh_rtc_remove(struct platform_device *dev)
{
	struct fh_rtc_controller *fh_rtc = platform_get_drvdata(dev);

	free_irq(fh_rtc->irq, fh_rtc);
	rtc_device_unregister(fh_rtc->rtc);
	iounmap(fh_rtc->regs);
	platform_set_drvdata(dev, NULL);
	kfree(fh_rtc);

	return 0;
}

static int fh_rtc_misc_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int fh_rtc_misc_release(struct inode *inode, struct file *filp)
{
	return 0;
}

long fh_rtc_misc_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	int current_tsensor_data, current_offset, current_idx;
	int status;
	int count;
	int lut[12] = { 0 };
	int reg[2] = {0};

	switch (cmd) {
	case GET_TSENSOR_DATA:
		current_tsensor_data = fh_rtc_get_core(fh_rtc->v_addr,
			FH_RTC_CMD_TEMP_INFO);
		status = copy_to_user((int __user *)(arg),
			&current_tsensor_data, 4);
		if (status < 0)
			pr_info("copy to user failed\n");
		break;
	case GET_CURRENT_OFFSET_DATA:
		current_offset = fh_rtc_get_core(fh_rtc->v_addr,
			FH_RTC_CMD_OFFSET);
		current_offset = current_offset&(0xff<<16);
		current_offset = current_offset>>16;
		status = copy_to_user((int __user *)(arg),
			&current_offset, 4);
		if (status < 0)
			pr_info("copy to user failed\n");
		break;
	case GET_CURRENT_OFFSET_IDX:
		current_offset = fh_rtc_get_core(fh_rtc->v_addr,
			FH_RTC_CMD_OFFSET);
		current_idx = current_offset&(0x3f<<10);
		current_idx = current_idx>>10;
		status = copy_to_user((int __user *)(arg),
			&current_idx,
								4);
		if (status < 0)
			pr_info("copy to user failed\n");
		break;
	case RTC_GET_LUT:
		for (count = 0; count < 12; count++)
			lut[count] = fh_rtc_get_core(fh_rtc->v_addr,
				FH_RTC_CMD_OFFSET_LUT +
				(count<<4));
		status = copy_to_user((int __user *)(arg), &lut[0],
				count*4);
		if (status < 0)
			pr_info("copy to user failed\n");
		break;
	case RTC_SET_LUT:
		status = copy_from_user(&lut[0], (int __user *)(arg),
								48);
		if (status < 0)
			pr_info("copy from user failed\n");
		for (count = 0; count < 12; count++)
			fh_rtc_set_core(fh_rtc->v_addr,
				FH_RTC_CMD_OFFSET_LUT +
				(count<<4),
				lut[count]);
		break;
	case GET_REG_VALUE:
		for (count = 0; count < 9; count++)
			lut[count] = fh_rtc_get_core(fh_rtc->v_addr,
				(count&0xf)<<4);

		status = copy_to_user((int __user *)(arg), &lut[0],
			count*4);
		if (status < 0)
			pr_info("copy to user failed\n");
		break;
	case SET_REG_VALUE:
		status = copy_from_user(&reg[0], (int __user *)(arg),
			8);
		if (status < 0)
			pr_info("copy from user failed\n");

		fh_rtc_set_core(fh_rtc->v_addr,
			reg[0], reg[1]);
		pr_info("set reg addr :%x, value: %x\n",
			reg[0], reg[1]);
		break;
	case GET_TEMP_VALUE:
		status = fh_rtc_get_tsensor_data(&lut[0], NULL);
		if (status) {
			pr_err("GET_TEMP_VALUE error %d\n", status);
			return -EIO;
		}
		pr_info("temp value is %x\n", lut[0]);
		status = copy_to_user((int __user *)(arg),
			&lut[0], 4);
		if (status < 0)
			pr_info("copy to user failed\n");
		break;
	default:
		pr_info("val is invalied\n");
	break;
	}
		return 0;

}
static const struct file_operations fh_rtc_misc_fops = {
	.owner = THIS_MODULE,
	.open = fh_rtc_misc_open,
	.release = fh_rtc_misc_release,
	.unlocked_ioctl = fh_rtc_misc_ioctl,
};

static struct miscdevice fh_rtc_misc = {
	.minor = RTC_MINOR,
	.name = FH_RTC_MISC_DEVICE_NAME,
	/*.nodename = FH_EFUSE_MISC_DEVICE_NODE_NAME,*/
	.fops = &fh_rtc_misc_fops,
};


static struct platform_driver fh_rtc_driver = {
	.driver	= {
		.name = "fh_rtc",
		.owner = THIS_MODULE,
		.of_match_table = fh_rtc_of_match,
	},
	.probe		= fh_rtc_probe,
	.remove		= fh_rtc_remove,
};
static int __init fh_rtc_init(void)
{
	return platform_driver_register(&fh_rtc_driver);
}

static void __exit fh_rtc_exit(void)
{
	platform_driver_unregister(&fh_rtc_driver);
}

#ifdef USE_TSENSOR

static int fh_rtc_temp_cfg_coef_offset(unsigned int coef, unsigned int offset)
{
	unsigned int temp_cfg;
	int status;

	if (coef > 0xff) {
		pr_err("coef invalid\n");
		return -1;
	}
	if (offset > 0xffff) {
		pr_err("offset invalid\n");
		return -1;
	}
	temp_cfg = fh_rtc_get_core(fh_rtc->v_addr,
		FH_RTC_CMD_TEMP_CFG);
	temp_cfg &= 0xffff0000;
	temp_cfg |= coef;
	temp_cfg |= (offset<<8);

	status = fh_rtc_set_core(fh_rtc->v_addr,
		FH_RTC_CMD_TEMP_CFG,
		temp_cfg);
	return status;
}

static int fh_rtc_temp_cfg_thl_thh(unsigned int thl, unsigned int thh)
{
	unsigned int temp_cfg;
	int status;

	if (thl > 0x3f)	{
		pr_err("thl invalid\n");
		return -1;
	}
	if (thh > 0x3f)	{
		pr_err("thh invalid\n");
		return -1;
	}

	temp_cfg = fh_rtc_get_core(fh_rtc->v_addr,
		FH_RTC_CMD_TEMP_CFG);
	temp_cfg &= 0xf000ffff;
	temp_cfg |= (thl<<16);
	temp_cfg |= (thh<<22);

	status = fh_rtc_set_core(fh_rtc->v_addr,
		FH_RTC_CMD_TEMP_CFG,
		temp_cfg);

	return status;
}

#endif

#ifdef CONFIG_DEFERRED_INIICALLS_RTC
deferred_module_init(fh_rtc_init);
#else
module_init(fh_rtc_init);
#endif
module_exit(fh_rtc_exit);


MODULE_DESCRIPTION("FH SOC RTC Driver");
MODULE_AUTHOR("yu.zhang <zhangy@fullhan.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:fh-rtc");
