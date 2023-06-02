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
#include <mach/fh_rtc_v2.h>
#include <mach/fh_sadc.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>
#include <linux/irqreturn.h>
/* #define FH_RTC_DEBUG_PRINT */

#ifdef FH_RTC_DEBUG_PRINT
#define RTC_PRINT_DBG(fmt, args...) \
	do {
	printk(KERN_INFO "[FH_RTC_DEBUG]: "); \
	printk(fmt, ## args)
	} while (0)
#else
#define RTC_PRINT_DBG(fmt, args...)  do { } while (0)
#endif

unsigned int rtc_use_efuse_viture_addr;

#ifdef CONFIG_USE_TSENSOR
#define USE_TSENSOR
#ifdef CONFIG_USE_TSENSOR_OFFSET
#define USE_TSENSOR_OFFSET
#endif
#endif
/*
#define USE_TSENSOR_OFFSET
#define USE_TSENSOR
*/
/* #define USE_DEBUG_REGISTER */

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
	struct proc_dir_entry *proc_rtc_entry;
	int sadc_channel;

	struct workqueue_struct *wq;
	struct delayed_work self_adjust;
};
struct fh_rtc_core_int_status {
	unsigned int core_int_en;
	unsigned int core_int_status;
};
struct fh_rtc_controller *fh_rtc;
struct fh_rtc_core_int_status fh_core_int;
enum {

	TIME_FUNC = 0, ALARM_FUNC,

};
#ifdef USE_TSENSOR
#define LUT_COF        58
#define LUT_OFFSET     0xff
#define TSENSOR_OFFSET 55
#define TSENSOR_COF    58
#define TSENSOR_STEP   3
#define TEMP_CP        25
#define TSENSOR_CP_DEFAULT_OUT 0x993

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
#define		RTC_USE_EFUSE_CMD				(0x0000)
#define		RTC_USE_EFUSE_CONFIG			(0x0004)
#define		RTC_USE_EFUSE_MATCH_KEY			(0x0008)
#define		RTC_USE_EFUSE_TIMING0			(0x000C)
#define		RTC_USE_EFUSE_TIMING1			(0x0010)
#define		RTC_USE_EFUSE_TIMING2			(0x0014)
#define		RTC_USE_EFUSE_TIMING3			(0x0018)
#define		RTC_USE_EFUSE_TIMING4			(0x001C)
#define		RTC_USE_EFUSE_TIMING5			(0x0020)
#define		RTC_USE_EFUSE_TIMING6			(0x0024)
#define		RTC_USE_EFUSE_DOUT				(0x0028)
#define		RTC_USE_EFUSE_STATUS0			(0x002C)
#define		RTC_USE_EFUSE_STATUS1			(0x0030)
#define		RTC_USE_EFUSE_STATUS2			(0x0034)
#define		RTC_USE_EFUSE_STATUS3			(0x0038)
#define		RTC_USE_EFUSE_STATUS4			(0x003C)
#define		RTC_USE_EFUSE_MEM_INFO			(0x0100)
static int tsensor_detect_complete(unsigned int i)
{
	unsigned int rdata;
	unsigned int loop_count;

	rdata = 0
	loop_count = 0;
	while ((rdata&(1<<i)) != 1<<i) {
		rdata = readl(rtc_use_efuse_viture_addr +
				RTC_USE_EFUSE_STATUS3);

		if (rdata != 0x0) {

			pr_info("detect complet status 0x%x\n", rdata);
			break;
		}
		rdata = readl(rtc_use_efuse_viture_addr +
				RTC_USE_EFUSE_STATUS0);
		loop_count++;
		usleep_range(1000, 2000);
		if (loop_count >= 100) {

			pr_err("RTC: can't detect completed!\n");
			return -EBUSY;
		}
	}

	return 0;
}
int tsensor_load_usrcmd(void)
{
	unsigned rdata;
	int status;

	status = 0;
	rdata = readl(rtc_use_efuse_viture_addr + RTC_USE_EFUSE_STATUS0);
	if ((rdata&(1<<31)) != 1<<31) {
		writel(0x1, rtc_use_efuse_viture_addr+RTC_USE_EFUSE_CMD);
		status = tsensor_detect_complete(1);
		if (status < 0) {
			pr_err("RTC:load usrcmd error!\n");
			return -EBUSY;
		}

		rdata = readl(rtc_use_efuse_viture_addr+RTC_USE_EFUSE_DOUT);
	}

	return 0;
}
int tsensor_efuse_read(int entry)
{
	unsigned rdata;
	int status = 0;

	status = tsensor_load_usrcmd();
	if (status < 0) {
		pr_err("RTC:tsensor_efuse_read load usrcmd error!\n");
		return -EBUSY;
	}
	writel((entry<<4)+0x3, rtc_use_efuse_viture_addr+RTC_USE_EFUSE_CMD);
	status = 0;
	status = tsensor_detect_complete(3);
	if (status < 0) {
		pr_err("RTC:tsensor_efuse_read load data error!\n");
		return -EBUSY;
	}
	rdata = readl(rtc_use_efuse_viture_addr+RTC_USE_EFUSE_DOUT);

	return rdata;
}
int tsensor_adjust_lut(void)
{
	unsigned int tsensor_init_data;
	unsigned int tsensor_12bit;
	unsigned int low_data, high_data;
	int temp_diff;
	int tsensor_out_value_diff;

	low_data =  tsensor_efuse_read(60);
	high_data =  tsensor_efuse_read(61);
	tsensor_init_data = (high_data<<8)|low_data;
	tsensor_12bit = tsensor_init_data&0xfff;
	tsensor_out_value_diff = tsensor_12bit - TSENSOR_CP_DEFAULT_OUT;

	temp_diff = tsensor_out_value_diff*LUT_COF/4096;

	return temp_diff;
}
#endif
#ifdef USE_TSENSOR
static int fh_rtc_temp_cfg_coef_offset(unsigned int coef, unsigned int offset);
static int fh_rtc_temp_cfg_thl_thh(unsigned int thl, unsigned int thh);
#if 0
static int fh_rtc_temp_cfg_update_time(unsigned int time);
#endif
#endif
static int fh_rtc_set_core(unsigned int base_addr, unsigned int reg_num,\
		unsigned int value);
#ifdef USE_TSENSOR
static void rtc_adjust(void)
{
	int i;
	int offset_index;
	char offset_lut[48];

	for (i = 0; i < 12; i++)
		fh_rtc_set_core(fh_rtc->v_addr,
				FH_RTC_CMD_OFFSET_LUT+(i<<4),
				TSENSOR_LUT[i]);

	for (i = 0; i < 12; i++) {
		offset_lut[i*4] = TSENSOR_LUT[i]&0xff;
		offset_lut[i*4+1] = (TSENSOR_LUT[i]>>4);
		offset_lut[i*4+2] = (TSENSOR_LUT[i]>>8);
		offset_lut[i*4+3] = (TSENSOR_LUT[i]>>12);
	}

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
	fh_rtc_temp_cfg_coef_offset(LUT_COF, LUT_OFFSET-i);
	pr_info("tsensor diff value : %d\n", i);
#else
	fh_rtc_temp_cfg_coef_offset(LUT_COF, LUT_OFFSET);
#endif
	fh_rtc_temp_cfg_thl_thh(0, 47);
	fh_rtc_set_core(fh_rtc->v_addr,
		FH_RTC_CMD_OFFSET,
		(offset_index<<2) | (offset_index<<10)|
		OFFSET_BK_EN|OFFSET_BK_AUTO);
}
#endif

static int fh_rtc_core_idle(unsigned int base_addr)
{
	unsigned int status;

	status = GET_REG(base_addr+FH_RTC_INT_STATUS);
	if (status & FH_RTC_INT_STATUS_CORE_IDLE)
		return 0;
	else
		return -1;
}

static int fh_rtc_core_wr(unsigned int base_addr)
{
	int reg;

	reg = GET_REG(base_addr+FH_RTC_INT_STATUS);
	reg &= (~FH_RTC_INT_STATUS_CORE_IDLE);
	SET_REG(base_addr+FH_RTC_INT_STATUS, reg);

	return 0;
}

static int fh_rtc_get_time(unsigned int base_addr)
{
	int reg, count, status;

	if (fh_rtc_core_idle(base_addr) < 0) {
		pr_info("fh rtc get time error\n");
		return -1;
	}
	fh_rtc_core_wr(base_addr);
	SET_REG(base_addr+FH_RTC_CMD, RTC_READ);
	for (count = 0; count < 250; count++) {
		status = fh_rtc_core_idle(base_addr);
		if (status == 0) {
			reg = GET_REG(base_addr+FH_RTC_RD_DATA);
			return reg;
		}
		udelay(100);

	}
	pr_info("rtc core busy can't get time\n");

	return -1;
}

static int fh_rtc_set_core(unsigned int base_addr, unsigned int reg_num,\
		unsigned int value)
{
	int count, status;

	if (fh_rtc_core_idle(base_addr) < 0) {
		pr_info("rtc get time:rtc core busy\n");
		return -1;
	}
	fh_rtc_core_wr(base_addr);

	SET_REG(base_addr+FH_RTC_WR_DATA, value);
	SET_REG(base_addr+FH_RTC_CMD, reg_num|RTC_WRITE);
	for (count = 0; count < 250; count++) {
		status = fh_rtc_core_idle(base_addr);
		if (status == 0)
			return 0;

		udelay(100);

	}
	pr_info("rtc SET CORE REG TIMEOUT\n");

	return -1;
}

static int fh_rtc_get_core(unsigned int base_addr, unsigned int reg_num)
{
	int reg, count, status;

	if (fh_rtc_core_idle(base_addr) < 0) {
		pr_info("rtc get time:rtc core busy %d\n", __LINE__);
		return -1;
	}
	fh_rtc_core_wr(base_addr);
	SET_REG(base_addr+FH_RTC_CMD, reg_num|RTC_READ);
	for (count = 0; count < 150; count++) {
		status = fh_rtc_core_idle(base_addr);
		if (status == 0) {
			reg = GET_REG(base_addr+FH_RTC_RD_DATA);
			return reg;
		}
		udelay(100);

	}
	pr_info("rtc GET CORE REG TIMEOUT line %d\n", __LINE__);

	return -1;
}

static int fh_rtc_get_temp(unsigned int base_addr)
{
	int reg, count, status;

	if (fh_rtc_core_idle(base_addr) < 0) {
		pr_info("rtc get time:rtc core busy %d\n", __LINE__);

		return -1;
	}
	fh_rtc_core_wr(base_addr);
	SET_REG(base_addr+FH_RTC_CMD, RTC_TEMP);
	for (count = 0; count < 150; count++) {
		status = fh_rtc_core_idle(base_addr);
		if (status == 0) {
			reg = GET_REG(base_addr+FH_RTC_RD_DATA);

			return reg;
		}
		udelay(100);

	}
	pr_info("rtc GET CORE REG TIMEOUT line %d\n", __LINE__);

	return -1;
}

static int fh_rtc_set_time(unsigned int base_addr, unsigned int value)
{
	int count;

	if (fh_rtc_core_idle(base_addr) < 0) {
		pr_info("set time :rtc core busy\n");

		return -1;
	}
	fh_rtc_core_wr(base_addr);
	SET_REG(base_addr+FH_RTC_WR_DATA, value);
	SET_REG(base_addr+FH_RTC_CMD, RTC_WRITE);
	for (count = 0; count < 150; count++) {
		if (fh_rtc_core_idle(base_addr) == 0)
			return 0;

		udelay(100);
	}
	pr_info("rtc core busy can't set time\n");

	return -1;
}
#ifdef DRIVER_TEST
static int fh_rtc_set_time_first(unsigned int base_addr, unsigned int value)
{

	SET_REG(base_addr+FH_RTC_WR_DATA, value);
	SET_REG(base_addr+FH_RTC_CMD, RTC_WRITE);

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

	status = GET_REG(fh_rtc->v_addr+FH_RTC_INT_STATUS);
	SET_REG(fh_rtc->v_addr+FH_RTC_INT_STATUS, status&
			(~FH_RTC_INT_STATUS_CORE));

	status = GET_REG(fh_rtc->v_addr+FH_RTC_INT_EN);
	SET_REG(fh_rtc->v_addr+FH_RTC_INT_EN, status &
			(~FH_RTC_INT_CORE_INT_ERR_MASK));
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
	ret_sec = (day_value * 86400) + (hour_value * 3600) + (min_value * 60)\
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

	raw_value = (day_value << DAY_BIT_START) | \
		(hour_value << HOUR_BIT_START)\
		| (min_value << MIN_BIT_START) | \
		(sec_value << SEC_BIT_START);

	if (func_switch == TIME_FUNC) {
		fh_rtc_set_time((unsigned int)fh_rtc->regs, raw_value);
		return 0;
	} else {
		status = fh_rtc_set_alarm_time((unsigned int)fh_rtc->regs,
				raw_value);
		return status;
	}
}

static int fh_rtc_open(struct device *dev)
{
	return 0;
}

static void fh_rtc_release(struct device *dev)
{
	return;
}

#ifdef DRIVER_TEST
static int fh_rtc_tm_compare(struct rtc_time *tm0, struct rtc_time *tm1)
{
	unsigned long read = 0, write = 0;

	rtc_tm_to_time(tm0, &read);
	rtc_tm_to_time(tm1, &write);

	if (write > read || write < read - 2) {
		RTC_PRINT_DBG(
			"ERROR: read(%d-%d-%d %d:%d:%d) vs "\
			"write(%d-%d-%d %d:%d:%d)\n",\
			tm0->tm_year + 1900, tm0->tm_mon + 1, \
			tm0->tm_mday,\
			tm0->tm_hour, tm0->tm_min, \
			tm0->tm_sec, tm1->tm_year + 1900,\
			tm1->tm_mon + 1, tm1->tm_mday, \
			tm1->tm_hour, tm1->tm_min,\
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
		tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, \
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
	status = GET_REG(fh_rtc->v_addr+FH_RTC_INT_STATUS);
	pr_info("wrapper int status : 0x%x\n", status);
	status = GET_REG(fh_rtc->v_addr+FH_RTC_INT_EN);
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

		status = GET_REG(fh_rtc->v_addr+FH_RTC_INT_STATUS);
		SET_REG(fh_rtc->v_addr+FH_RTC_INT_STATUS, status&
				(~FH_RTC_INT_STATUS_CORE));

		status = GET_REG(fh_rtc->v_addr+FH_RTC_INT_EN);
		SET_REG(fh_rtc->v_addr+FH_RTC_INT_EN, status &
				(~FH_RTC_INT_CORE_INT_ERR_MASK));

	} else
		fh_rtc_alarm_disable_interrupt(fh_rtc->v_addr,
			 FH_RTC_ISR_ALARM_POS);

	return 0;
}

static void fh_rtc_clear_interrupt_status(unsigned int base_addr,
		unsigned int value)
{
	unsigned int int_en, int_status;

	int_en = GET_REG((unsigned int)base_addr+FH_RTC_INT_EN);
	int_en &= 0x7f;
	SET_REG(base_addr+FH_RTC_INT_EN,\
		(((value)\
		& 0x7f)<<16)|int_en|\
		FH_RTC_INT_CORE_IDLE_ERR_MASK|FH_RTC_INT_CORE_INT_ERR_MASK);
	int_status = value & 0x10a0;
	SET_REG(base_addr+FH_RTC_INT_STATUS, int_status);
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
	wr_int_status_clr = GET_REG(fh_rtc->v_addr +
		FH_RTC_INT_STATUS);
	rtc_int_status = fh_rtc_get_core(fh_rtc->v_addr,
		FH_RTC_CMD_INT_STATUS);

	/* get core interrupt failed */
	if (rtc_int_en < 0) {
		pr_info("can't get core int en\n");
		return;
	}
	if (rtc_int_status < 0) {
		pr_info("can't get core int status\n");
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
		wr_int_status = GET_REG(fh_rtc->v_addr+
			FH_RTC_INT_EN);
		SET_REG(fh_rtc->v_addr+FH_RTC_INT_EN,\
			wr_int_status&
			(FH_RTC_INT_CORE_INT_ALL_COV));
		/* updata to RTC CORE alarm work */
		events |= RTC_IRQF | RTC_AF;
		rtc_update_irq(rtc, 1, events);

	} else if (rtc_int_en & rtc_int_status & FH_RTC_ISR_SEC_POS) {

		pr_info("FH_RTC_ISR_SEC_POS\n");
		rtc_core_status = fh_rtc_get_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS);
		fh_rtc_set_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS,
			rtc_core_status & (~FH_RTC_ISR_SEC_POS));
		wr_int_status = GET_REG(fh_rtc->v_addr+
			FH_RTC_INT_EN);

		SET_REG(fh_rtc->v_addr+FH_RTC_INT_EN,\
			wr_int_status&
			(FH_RTC_INT_CORE_INT_STATUS_COV));

		events |= RTC_IRQF | RTC_UF;
		rtc_update_irq(rtc, 1, events);

	} else if (rtc_int_en & rtc_int_status & FH_RTC_ISR_MIN_POS) {

		pr_info("FH_RTC_ISR_MIN_POS\n");
		rtc_core_status = fh_rtc_get_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS);

		fh_rtc_set_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS,
			rtc_core_status & (~FH_RTC_ISR_MIN_POS));
		wr_int_status = GET_REG(fh_rtc->regs+
			FH_RTC_INT_EN);

		SET_REG(fh_rtc->v_addr+FH_RTC_INT_EN,
			wr_int_status&\
			(FH_RTC_INT_CORE_INT_ERR_MASK_COV));

		SET_REG(fh_rtc->v_addr+FH_RTC_INT_STATUS,
			wr_int_status_clr&\
			(FH_RTC_INT_CORE_INT_STATUS_COV));

	} else if (rtc_int_en & rtc_int_status & FH_RTC_ISR_HOUR_POS) {
		pr_info("FH_RTC_ISR_HOUR_POS\n");
		rtc_core_status = fh_rtc_get_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS);

		fh_rtc_set_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS,
			rtc_core_status & (~FH_RTC_ISR_HOUR_POS));
		wr_int_status = GET_REG(fh_rtc->v_addr+
			FH_RTC_INT_EN);

		SET_REG(fh_rtc->v_addr+FH_RTC_INT_EN,
			wr_int_status&\
			(FH_RTC_INT_CORE_INT_ERR_MASK_COV));

		SET_REG(fh_rtc->v_addr+FH_RTC_INT_STATUS,
			wr_int_status_clr&\
			(FH_RTC_INT_CORE_INT_STATUS_COV));
	} else if (rtc_int_en & rtc_int_status & FH_RTC_ISR_DAY_POS) {
		pr_info("FH_RTC_ISR_DAY_POS\n");
		rtc_core_status = fh_rtc_get_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS);

		fh_rtc_set_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS,
			rtc_core_status & (~FH_RTC_ISR_DAY_POS));
		wr_int_status = GET_REG(fh_rtc->v_addr+
			FH_RTC_INT_EN);

		SET_REG(fh_rtc->v_addr+FH_RTC_INT_EN,
			wr_int_status&\
			(FH_RTC_INT_CORE_INT_ERR_MASK_COV));

		SET_REG(fh_rtc->v_addr+FH_RTC_INT_STATUS,
			wr_int_status_clr&\
			(FH_RTC_INT_CORE_INT_STATUS_COV));
	} else if (rtc_int_en & rtc_int_status & FH_RTC_ISR_POWERFAIL_POS) {
		pr_info("FH_RTC_ISR_POWERFAIL_POS\n");
		rtc_core_status = fh_rtc_get_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS);
		fh_rtc_set_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS,
			rtc_core_status &
			(~FH_RTC_ISR_POWERFAIL_POS));

		wr_int_status = GET_REG(fh_rtc->v_addr+
			FH_RTC_INT_EN);

		SET_REG(fh_rtc->v_addr+FH_RTC_INT_STATUS,
			wr_int_status_clr&\
			(FH_RTC_INT_CORE_INT_STATUS_COV));

	} else if (rtc_int_en & rtc_int_status & FH_RTC_ISR_RX_CRC_ERR_INT) {
		pr_info("FH_RTC_ISR_RX_CRC_ERR_INT\n");
	} else if (rtc_int_en & rtc_int_status & FH_RTC_ISR_RX_COM_ERR_INT) {
		pr_info("FH_RTC_ISR_RX_COM_ERR_INT\n");
	} else if (rtc_int_en & rtc_int_status & FH_RTC_LEN_ERR_INT) {
		pr_info("FH_RTC_LEN_ERR_INT\n");
	} else {
		pr_info("unexpect isr\n");
		fh_rtc_set_core(fh_rtc->v_addr,
			FH_RTC_CMD_INT_STATUS, rtc_int_en & (~rtc_int_status));
	}


}

static irqreturn_t fh_rtc_irq(int irq, void *dev_id)
{

	struct fh_rtc_controller *fh_rtc = (struct fh_rtc_controller *) dev_id;
	unsigned int isr_status;
	/*
	 * 1.clear wrapper interrput status
	 * 2.mask core int
	 * 3.schedule interrput work
	*/
	isr_status = GET_REG(fh_rtc->v_addr+FH_RTC_INT_STATUS);
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
static void create_proc_rtc(struct fh_rtc_controller *rtc);
static void remove_proc(void);
static int  fh_rtc_probe(struct platform_device *pdev)
{
	int err = 0;
	struct fh_rtc_platform_data *rtc_platform_info;
	struct rtc_device *rtc;
	struct resource *res;
	int rtc_core_status;

	/* malloc mem region */
	fh_rtc = kzalloc(sizeof(struct fh_rtc_controller), GFP_KERNEL);
	if (!fh_rtc)
		return -ENOMEM;

	memset(fh_rtc, 0, sizeof(struct fh_rtc_controller));

	/* board info below */
	rtc_platform_info = (struct fh_rtc_platform_data *)\
			pdev->dev.platform_data;
	if (rtc_platform_info == NULL) {
		dev_err(&pdev->dev, "%s, rtc platform error.\n",
			__func__);
		err = -ENODEV;
		goto err_nores;
	}
	fh_rtc->base_year = rtc_platform_info->base_year;
	fh_rtc->base_month = rtc_platform_info->base_month;
	fh_rtc->base_day = rtc_platform_info->base_day;
	fh_rtc->sadc_channel = rtc_platform_info->sadc_channel;

	/* find the IRQs */
	fh_rtc->irq = platform_get_irq(pdev, 0);
	if (fh_rtc->irq < 0) {
		dev_err(&pdev->dev, "%s, rtc irq error.\n",
			__func__);
		err = fh_rtc->irq;
		goto err_nores;
	}

	/* get the memory region */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "failed to get memory region resource\n");
		err = -ENOENT;
		goto err_nores;
	}

	fh_rtc->paddr = res->start;

	fh_rtc->regs = ioremap(res->start, resource_size(res));
	fh_rtc->v_addr = (unsigned int)fh_rtc->regs;
	if (!fh_rtc->regs) {
		dev_err(&pdev->dev, "rtc already mapped\n");
		err = -EINVAL;
		goto err_nores;
	}

	fh_rtc->clk = clk_get(&pdev->dev, "rtc_pclk_gate");

	if (IS_ERR(fh_rtc->clk))
		printk(KERN_INFO "cannot get rtc_pclk\n");
	else
		clk_enable(fh_rtc->clk);

	/* efuse mem map */
	rtc_use_efuse_viture_addr = (unsigned int)ioremap(EFUSE_REG_BASE,
		resource_size(res));

	/* init int status */
	SET_REG(fh_rtc->v_addr+FH_RTC_INT_EN,
		FH_RTC_INT_CORE_INT_ERR_EN|\
		FH_RTC_INT_CORE_IDLE_ERR_EN | \
		FH_RTC_INT_CORE_INT_ERR_MASK |\
		FH_RTC_INT_CORE_IDLE_ERR_MASK
		);
	SET_REG(fh_rtc->v_addr+FH_RTC_INT_STATUS,
		FH_RTC_INT_STATUS_CORE |\
		FH_RTC_INT_STATUS_CORE_IDLE);


	SET_REG(fh_rtc->v_addr+FH_RTC_INT_STATUS, 0x20);
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

	platform_set_drvdata(pdev, fh_rtc);

	/* register RTC and exit */
	rtc = rtc_device_register(rtc_platform_info->dev_name, \
		&pdev->dev, &fh_rtcops, THIS_MODULE);
	if (IS_ERR(rtc)) {
		dev_err(&pdev->dev, "cannot attach rtc\n");
		err = PTR_ERR(rtc);
		goto err_nores;
	}
	fh_rtc->rtc = rtc;

	INIT_WORK(&rtc_int_wq, rtc_core_int_handler);
	/* register RTC IRQs */
	err = request_irq(fh_rtc->irq , fh_rtc_irq, 0,
		dev_name(&pdev->dev), fh_rtc);
	if (err) {
		RTC_PRINT_DBG("request_irq failed, %d\n", err);
		goto err_nores;
	}

	/* register RTC MISC device */
	err = misc_register(&fh_rtc_misc);
	if (err < 0)
		RTC_PRINT_DBG("register rtc misc device error\n");
	/* register RTC PROC FILE */
	create_proc_rtc(fh_rtc);

#ifdef USE_TSENSOR
	rtc_adjust();
#endif

	return 0;

err_nores:
	return err;
}

static int fh_rtc_remove(struct platform_device *dev)
{
	struct fh_rtc_controller *fh_rtc = platform_get_drvdata(dev);

	remove_proc();
	free_irq(fh_rtc->irq, fh_rtc);
	rtc_device_unregister(fh_rtc->rtc);
	iounmap(fh_rtc->regs);
	platform_set_drvdata(dev, NULL);
	kfree(fh_rtc);

	return 0;
}

#ifdef CONFIG_PM

/* RTC Power management control */

static int fh_rtc_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int fh_rtc_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define fh_rtc_suspend NULL
#define fh_rtc_resume  NULL
#endif
#define FH_RTC_MISC_DEVICE_NAME			"fh_rtc_misc"

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
	/* get temperature data from tsensor */
	case GET_TSENSOR_DATA:
		current_tsensor_data = fh_rtc_get_core(fh_rtc->v_addr,
			FH_RTC_CMD_TEMP_INFO);
		status = copy_to_user((int __user *)(arg),
			&current_tsensor_data,
			4);
		if (status < 0)
			pr_info("copy to user failed\n");
		break;
	/* get current used offset data */
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
	/* get current used offset index of lut */
	case GET_CURRENT_OFFSET_IDX:
		current_offset = fh_rtc_get_core(fh_rtc->v_addr,
			FH_RTC_CMD_OFFSET);
		current_idx = current_offset&(0x3f<<10);
		current_idx = current_idx>>10;
		status = copy_to_user((int __user *)(arg),
			&current_idx, 4);

		if (status < 0)
			pr_info("copy to user failed\n");
		break;
	/* updata all of lut */
	case RTC_GET_LUT:
		for (count = 0; count < 12; count++)
			lut[count] = fh_rtc_get_core(fh_rtc->v_addr,
				FH_RTC_CMD_OFFSET_LUT + (count<<4));

		status = copy_to_user((int __user *)(arg), &lut[0],
			count*4);
		if (status < 0)
			pr_info("copy to user failed\n");
		break;
	/* modify lut */
	case RTC_SET_LUT:
		status = copy_from_user(&lut[0], (int __user *)(arg),
								48);
		if (status < 0)
			pr_info("copy from user failed\n");
		for (count = 0; count < 12; count++)
			fh_rtc_set_core(fh_rtc->v_addr,
				FH_RTC_CMD_OFFSET_LUT + (count<<4), lut[count]);
		break;
	/* get RTC core reg value */
	case GET_REG_VALUE:
		for (count = 0; count < 9; count++)
			lut[count] = fh_rtc_get_core(fh_rtc->v_addr,
				(count&0xf)<<4);

		status = copy_to_user((int __user *)(arg), &lut[0],
			count*4);
		if (status < 0)
			pr_info("copy to user failed\n");
		break;
	/* set RTC core reg value */
	case SET_REG_VALUE:
		status = copy_from_user(&reg[0], (int __user *)(arg), 8);
		if (status < 0)
			pr_info("copy from user failed\n");

		fh_rtc_set_core(fh_rtc->v_addr,
								reg[0],
								reg[1]);
		pr_info("set reg addr :%x, value: %x\n",
				reg[0], reg[1]);
		break;
	/* get temperature value */
	case GET_TEMP_VALUE:
		status = fh_rtc_get_temp(fh_rtc->v_addr);
		lut[0] = fh_rtc_get_core(fh_rtc->v_addr,
				FH_RTC_CMD_TEMP_INFO);
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
	.minor = MISC_DYNAMIC_MINOR,
	.name = FH_RTC_MISC_DEVICE_NAME,
	/*.nodename = FH_EFUSE_MISC_DEVICE_NODE_NAME,*/
	.fops = &fh_rtc_misc_fops,
};
static struct platform_driver fh_rtc_driver = {
	.probe = fh_rtc_probe,
	.remove = __devexit_p(fh_rtc_remove),
	.suspend = fh_rtc_suspend,
	.resume = fh_rtc_resume,
	.driver = {
		.name = "fh_rtc",
		.owner = THIS_MODULE,
	},
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
		FH_RTC_CMD_TEMP_CFG, temp_cfg);

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
#if 0
static int fh_rtc_temp_cfg_update_time(unsigned int time)
{
	unsigned int temp_cfg;
	int status;

	if (time > 5)	{
		pr_err("update time invalid\n");
		return -1;
	}

	temp_cfg = fh_rtc_get_core(fh_rtc->v_addr,
		FH_RTC_CMD_TEMP_CFG);
	temp_cfg &= 0xf0000000;
	temp_cfg |= time;

	status = fh_rtc_set_core(fh_rtc->v_addr,
		FH_RTC_CMD_TEMP_CFG, temp_cfg);

	return status;
}
#endif
#endif
ssize_t zy_rtc_proc_read(char *page, char **start, off_t off, \
		int count, int *eof, struct fh_rtc_controller *data)
{
#ifdef USE_TSENSOR
	int current_offset, current_idx, current_tsensor_data, current_cfg;

	current_offset = fh_rtc_get_core(fh_rtc->v_addr,
		FH_RTC_CMD_OFFSET);
	current_cfg = fh_rtc_get_core(fh_rtc->v_addr,
		FH_RTC_CMD_TEMP_CFG);
	current_offset = current_offset&(0xff<<16);
	current_idx = current_offset&(0x3f<<10);
	current_tsensor_data = fh_rtc_get_core(fh_rtc->v_addr,
		FH_RTC_CMD_TEMP_INFO);
	current_tsensor_data = current_tsensor_data*210;
	current_tsensor_data = (int)(current_tsensor_data/4096);
	pr_info("[RTC]:current offset:%x\n", current_offset>>16);
	pr_info("[RTC]:current temp:%d\n", current_tsensor_data-56);
#endif

	return 0;
}

static ssize_t fh_rtc_proc_write(struct file *filp, \
	const char *buf, size_t len,\
loff_t *off)
{
	return 0;
}

static void create_proc_rtc(struct fh_rtc_controller *rtc)
{
	fh_rtc->proc_rtc_entry = create_proc_entry(FH_RTC_PROC_FILE,\
		S_IRUGO, NULL);

	if (!fh_rtc->proc_rtc_entry) {
		pr_err("create proc failed\n");
	} else {
		fh_rtc->proc_rtc_entry->read_proc = \
			(read_proc_t *) zy_rtc_proc_read;
		fh_rtc->proc_rtc_entry->write_proc = \
			(write_proc_t *) fh_rtc_proc_write;
		fh_rtc->proc_rtc_entry->data = rtc;
	}
}

static void remove_proc(void)
{
	remove_proc_entry(FH_RTC_PROC_FILE, NULL);
}

module_init(fh_rtc_init);
module_exit(fh_rtc_exit);


MODULE_DESCRIPTION("FH SOC RTC Driver");
MODULE_AUTHOR("yu.zhang <zhangy@fullhan.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:fh-rtc");
