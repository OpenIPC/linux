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
#include <mach/fh_rtc_v1.h>
#include <mach/fh_sadc.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>

/* #define FH_RTC_DEBUG_PRINT */

#ifdef FH_RTC_DEBUG_PRINT
#define RTC_PRINT_DBG(fmt, args...) \
	printk(KERN_INFO "[FH_RTC_DEBUG]: "); \
	printk(fmt, ## args)
#else
#define RTC_PRINT_DBG(fmt, args...)  do { } while (0)
#endif

#define RTC_MAGIC	0x55555555
#define RTC_PHASE	0x03840384

#define SYNC_LOOP_COUNT 100

struct fh_rtc_controller {
	void * __iomem regs;
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

struct fh_rtc_controller *fh_rtc;

enum {

	TIME_FUNC = 0,
	ALARM_FUNC,

};

/* value of SADC channel for reference to get current temperature */
long SadcValue[28] = {
	260, 293, 332, 375, 426,
	483, 548, 621, 706, 800,
	906, 1022, 1149, 1287, 1435,
	1590, 1750, 1913, 2075, 2233,
	2385, 2527, 2656, 2772, 2873,
	2960, 3034, 3094
};

/* value of temperature for reference */
int Temperature1[28] = {
	95000, 90000, 85000, 80000, 75000,
	70000, 65000, 60000, 55000, 50000,
	45000, 40000, 35000, 30000, 25000,
	20000, 15000, 10000, 4000, 0,
	-5000, -10000, -15000, -20000, -25000,
	-30000, -35000, -40000
};

/* value of temperature for reference to get current deviation */
int Temperature2[136] = {
	-40000, -39000, -38000, -37000, -36000, -35000,
	-34000, -33000, -32000, -31000, -30000, -29000,
	-28000, -27000, -26000, -25000, -24000, -23000,
	-22000, -21000, -20000, -19000, -18000, -17000,
	-16000, -15000, -14000, -13000, -12000, -11000,
	-10000, -9000, -8000, -7000, -6000, -5000, -4000,
	-3000, -2000, -1000, 0, 1000, 2000, 3000, 4000,
	5000, 6000, 7000, 8000, 9000, 10000, 11000, 12000,
	13000, 14000, 15000, 16000, 17000, 18000, 19000,
	20000, 21000, 22000, 23000, 24000, 25000, 26000,
	27000, 28000, 29000, 30000, 31000, 32000, 33000,
	34000, 35000, 36000, 37000, 38000, 39000, 40000,
	41000, 42000, 43000, 44000, 45000, 46000, 47000,
	48000, 49000, 50000, 51000, 52000, 53000, 54000,
	55000, 56000, 57000, 58000, 59000, 60000, 61000,
	62000, 63000, 64000, 65000, 66000, 67000, 68000,
	69000, 70000, 71000, 72000, 73000, 74000, 75000,
	76000, 77000, 78000, 79000, 80000, 81000, 82000,
	83000, 84000, 85000, 86000, 87000, 88000, 89000,
	90000, 91000, 92000, 93000, 94000, 95000
};

/* the value of deviation to adjust rtc clock */
long Deviation[136] = {
	1690000, 1638400, 1587600, 1537600, 1488400, 1440000,
	1392400, 1345600, 1299600, 1254400, 1210000, 1166400,
	1123600, 1081600, 1040400, 1000000, 960400, 921600,
	883600, 846400, 810000, 774400, 739600, 705600, 672400,
	640000, 608400, 577600, 547600, 518400, 490000, 462400,
	435600, 409600, 384400, 360000, 336400, 313600, 291600,
	270400, 250000, 230400, 211600, 193600, 176400, 160000,
	144400, 129600, 115600, 102400, 90000, 78400, 67600, 57600,
	48400, 40000, 32400, 25600, 19600, 14400, 10000, 6400,
	3600, 1600, 400, 0, 400, 1600, 3600, 6400, 10000, 14400,
	19600, 25600, 32400, 40000, 48400, 57600, 67600, 78400,
	90000, 102400, 115600, 129600, 144400, 160000, 176400,
	193600, 211600, 230400, 250000, 270400, 291600, 313600,
	336400, 360000, 384400, 409600, 435600, 462400, 490000,
	518400, 547600, 577600, 608400, 640000, 672400, 705600,
	739600, 774400, 810000, 846400, 883600, 921600, 960400,
	1000000, 1040400, 1081600, 1123600, 1166400, 1210000,
	1254400, 1299600, 1345600, 1392400, 1440000, 1488400,
	1537600, 1587600, 1638400, 1690000, 1742400, 1795600,
	1849600, 1904400, 1960000
};

static int accelerate_second_rtc(int n)
{
	unsigned int reg;

	reg = readl(fh_rtc->regs + FH_RTC_OFFSET);
	reg &= ~(0x7000000);
	reg |= 0x30000000 | ((n & 0x7) << 24);
	writel(reg, fh_rtc->regs + FH_RTC_OFFSET);
	return 0;
}

static int accelerate_minute_rtc(int m)
{
	unsigned int reg;

	reg = readl(fh_rtc->regs + FH_RTC_OFFSET);
	reg &= ~(0x3F0000);
	reg |= 0x30000000 | ((m & 0x3f) << 16);
	writel(reg, fh_rtc->regs + FH_RTC_OFFSET);
	return 0;
}

static int slow_down_second_rtc(int n)
{
	unsigned int reg;

	reg = readl(fh_rtc->regs + FH_RTC_OFFSET);
	reg &= ~(0x7000000);
	reg |= 0x10000000 | ((n & 0x7) << 24);
	writel(reg, fh_rtc->regs + FH_RTC_OFFSET);
	return 0;
}

static int slow_down_minute_rtc(int m)
{
	unsigned int reg;

	reg = readl(fh_rtc->regs + FH_RTC_OFFSET);
	reg &= ~(0x3F0000);
	reg |= 0x10000000 | ((m & 0x3f) << 16);
	writel(reg, fh_rtc->regs + FH_RTC_OFFSET);
	return 0;
}

static unsigned int fh_rtc_get_hw_sec_data(unsigned int func_switch)
{

	unsigned int ret_sec, raw_value, sec_value;
	unsigned int min_value, hour_value, day_value;

	if (func_switch == TIME_FUNC)
		raw_value = fh_rtc_get_time(fh_rtc->regs);
	else
		raw_value = fh_rtc_get_alarm_time(fh_rtc->regs);

	sec_value = FH_GET_RTC_SEC(raw_value);
	min_value = FH_GET_RTC_MIN(raw_value);
	hour_value = FH_GET_RTC_HOUR(raw_value);
	day_value = FH_GET_RTC_DAY(raw_value);
	ret_sec = (day_value * 86400) + (hour_value * 3600)
			+ (min_value * 60) + sec_value;

	return ret_sec;

}

static void fh_rtc_set_hw_sec_data(struct rtc_time *rtc_tm,
		unsigned int func_switch) {

	unsigned int raw_value, sec_value, min_value;
	unsigned int hour_value, day_value;

	day_value = rtc_year_days(rtc_tm->tm_mday, rtc_tm->tm_mon,
			rtc_tm->tm_year+1900);
	day_value += (rtc_tm->tm_year-70)*365
			+ ELAPSED_LEAP_YEARS(rtc_tm->tm_year);

	hour_value = rtc_tm->tm_hour;
	min_value = rtc_tm->tm_min;
	sec_value = rtc_tm->tm_sec;

	raw_value = (day_value << DAY_BIT_START)
			| (hour_value << HOUR_BIT_START)
			| (min_value << MIN_BIT_START)
			| (sec_value << SEC_BIT_START);

	if (func_switch == TIME_FUNC)
		fh_rtc_set_time(fh_rtc->regs, raw_value);
	else
		fh_rtc_set_alarm_time(fh_rtc->regs, raw_value);

}

static int fh_rtc_exam_magic(void)
{
	unsigned int magic, status;
	int i;

	for (i = 0; i < 10; i++) {
		magic = GET_REG(fh_rtc->regs + FH_RTC_USER_REG);

		if (magic != RTC_MAGIC) {
			status = GET_REG(fh_rtc->regs+FH_RTC_SYNC);
			status &= 0x2;
			SET_REG(fh_rtc->regs+FH_RTC_SYNC, status);

			msleep(30);
		} else {
			return 0;
		}
	}

	printk(KERN_INFO "ERROR: read rtc failed: 0x%x\n", magic);

	return -EAGAIN;

}

static int fh_rtc_open(struct device *dev)
{
	return 0;
}

static void fh_rtc_release(struct device *dev)
{
	return;
}

static int fh_rtc_tm_compare(struct rtc_time *tm0, struct rtc_time *tm1)
{
	unsigned long read = 0, write = 0;

	rtc_tm_to_time(tm0, &read);
	rtc_tm_to_time(tm1, &write);

	if (write > read || write < read - 2) {
		RTC_PRINT_DBG("ERROR: read(%d-%d-%d %d:%d:%d) vs "
				"write(%d-%d-%d %d:%d:%d)\n",
			   tm0->tm_year + 1900,
			   tm0->tm_mon + 1,
			   tm0->tm_mday,
			   tm0->tm_hour,
			   tm0->tm_min,
			   tm0->tm_sec,
			   tm1->tm_year + 1900,
			   tm1->tm_mon + 1,
			   tm1->tm_mday,
			   tm1->tm_hour,
			   tm1->tm_min,
			   tm1->tm_sec);
		return -1;
	}
	return 0;
}

static int fh_rtc_gettime_nosync(struct device *dev, struct rtc_time *rtc_tm)
{
	unsigned int temp;

	temp = fh_rtc_get_hw_sec_data(TIME_FUNC);
	rtc_time_to_tm(temp, rtc_tm);
	RTC_PRINT_DBG("rtc read date:0x%x\n", temp);
	return 0;
}


static int fh_rtc_gettime_sync(struct device *dev, struct rtc_time *rtc_tm)
{
	unsigned int status;
	unsigned int loop_count;
	struct platform_device *pdev = to_platform_device(dev);
	struct fh_rtc_controller *fh_rtc = platform_get_drvdata(pdev);

	status = GET_REG(fh_rtc->regs+FH_RTC_SYNC);
	status &= 0x2;

	SET_REG(fh_rtc->regs+FH_RTC_SYNC, status);
	msleep(30);

	for (loop_count = 0;
			loop_count <= SYNC_LOOP_COUNT;
			loop_count++) {
		udelay(100);
		status = GET_REG(fh_rtc->regs+FH_RTC_SYNC);
		status &= 0x1;
		if(status == 1) {
			unsigned int temp;
			temp = fh_rtc_get_hw_sec_data(TIME_FUNC);
			rtc_time_to_tm(temp, rtc_tm);
			RTC_PRINT_DBG("rtc read date:0x%x\n", temp);
			return 0;
		}

	}

	printk(KERN_INFO "rtc read sync fail!\n");
	return -EAGAIN;
}

static int fh_rtc_settime(struct device *dev, struct rtc_time *tm)
{
	struct rtc_time rtc_tm_read0;
	unsigned int status;
	unsigned int loop_count;
	struct platform_device *pdev = to_platform_device(dev);
	struct fh_rtc_controller *fh_rtc = platform_get_drvdata(pdev);
	int cnt, ret, read_count = 0;

	RTC_PRINT_DBG("rtc write %d-%d-%d %d:%d:%d\n",
		 tm->tm_year + 1900,
		 tm->tm_mon + 1,
		 tm->tm_mday,
		 tm->tm_hour,
		 tm->tm_min,
		 tm->tm_sec);

	SET_REG(fh_rtc->regs + FH_RTC_USER_REG, RTC_MAGIC);
	msleep(3);

	for (cnt = 0; cnt < 5; cnt++) {
		int rewrite_count = 0;
REWRITE:
		ret = 0;

		fh_rtc_set_hw_sec_data(tm, TIME_FUNC);

		/*spin_lock_irqsave(&rtc_lock, flag);*/

		for (loop_count = 0;
				loop_count <= SYNC_LOOP_COUNT;
				loop_count++) {
			udelay(100);

			status = GET_REG(fh_rtc->regs+FH_RTC_SYNC);
			status &= 0x2;
			if (status == 0x2) {
				printk(KERN_INFO "rtc write loop_count :%d\n",
						loop_count);
				if(loop_count > 20) {
					RTC_PRINT_DBG("error: rewrite: %d, "
							"rtc write loop_count :%d\n",
							rewrite_count,
							loop_count);
					msleep(3);
					rewrite_count++;
					if (rewrite_count < 5) {
						goto REWRITE;
					} else {
						RTC_PRINT_DBG("rtc write retry exceed\n");
						msleep(3);
						break;
					}
				}
				/*spin_unlock_irqrestore(&rtc_lock, flag);*/
				msleep(3);
				break;
			}
		}

		if (loop_count >= SYNC_LOOP_COUNT) {
			printk(KERN_INFO "rtc write sync fail!\n");
			return -EAGAIN;
		}

		for (read_count = 0; read_count < 5; read_count++) {
			fh_rtc_gettime_sync(dev, &rtc_tm_read0);
			ret += fh_rtc_tm_compare(&rtc_tm_read0, tm);
		}

		if (!ret) {
			return 0;
		}

	}

	return -1;
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

	fh_rtc_set_hw_sec_data(rtc_tm, ALARM_FUNC);

	return 0;
}



static int fh_rtc_irq_enable(struct device *dev, unsigned int enabled)
{

	struct platform_device *pdev = to_platform_device(dev);
	struct fh_rtc_controller *fh_rtc = platform_get_drvdata(pdev);

	if (enabled) {
		fh_rtc_enable_interrupt(fh_rtc->regs,
				FH_RTC_ISR_SEC_POS | FH_RTC_ISR_ALARM_POS);
	}
	else{

		fh_rtc_disenable_interrupt(fh_rtc->regs,
				FH_RTC_ISR_SEC_POS | FH_RTC_ISR_ALARM_POS);
	}

	return 0;
}




static irqreturn_t fh_rtc_irq(int irq, void *dev_id)
{

	struct fh_rtc_controller *fh_rtc = (struct fh_rtc_controller *)dev_id;
	unsigned long events = 0;
	unsigned int isr_status;
	struct rtc_device *rtc = fh_rtc->rtc;

	isr_status = fh_rtc_get_enabled_interrupt(fh_rtc->regs);

	fh_rtc_clear_interrupt_status(fh_rtc->regs, isr_status);

	if (isr_status & FH_RTC_ISR_SEC_POS) {

		events |= RTC_IRQF | RTC_UF;
	}
	else if(isr_status & FH_RTC_ISR_ALARM_POS){
		events |= RTC_IRQF | RTC_AF;
	}
	else{
		pr_info("rtc unknown isr...\n");
		return IRQ_HANDLED;
	}
	rtc_update_irq(rtc, 1, events);

	return IRQ_HANDLED;

}

static const struct rtc_class_ops fh_rtcops = {
	.open		= fh_rtc_open,
	.release	= fh_rtc_release,
	.read_time	= fh_rtc_gettime_nosync,
	.set_time	= fh_rtc_settime,
	.read_alarm	= fh_rtc_getalarm,
	.set_alarm	= fh_rtc_setalarm,
	.alarm_irq_enable = fh_rtc_irq_enable,
};

/*get the read of SADC and adjust RTC clock*/
int fh_adjust_rtc(void)
{
	int m, n;	/*m:MinuteOffset, n:SecondOffset*/
	long T = 25000;
	int i, j, temp;
	long Ppm = 0;
	long value[7];
	int flag = 0;
	long sum = 0;
	long v;
	int num;

	for (i = 0; i < 7; i++) {
		value[i] = fh_sadc_get_value(fh_rtc->sadc_channel);
		if(!value[i])
		{
			printk("ERROR: %s, sadc value %lu is incorrect\n",
					__func__, value[i]);
			return -EIO;
		}
		mdelay(100);
	}
	for (i = 0; i < 7; i++) {
		for (j = i + 1; j < 7; j++) {
			if (value[j] < value[i]) {
				temp = value[i];
				value[i] = value[j];
				value[j] = temp;
			}
		}
	}
	sum = value[2] + value[3] + value[4];
	v = sum / 3;
	printk("the average value of SADC is:%ld\n", v);
	if(v >= 3094) {	/*if temperature is lower than -40℃,adjust by -40℃*/
		Ppm = 1690000;
		n = Ppm / 305176;
		Ppm -= 305176 * n;
		m = Ppm / 5086;
		printk("SecondOffset is: %d\n", n);
		printk("MinuteOffset is: %d\n", m);
		if ((n <= 7) && (m <= 63)) {
			accelerate_second_rtc(n);
			accelerate_minute_rtc(m);
			printk("rtc clock has been adjusted!\n");
		} else {
			printk("beyond range of adjust\n");
		}
		return 0;
	}
	if(v < 260) {	/*if temperature is higher than 95℃,adjust by 95℃*/
		Ppm = 1960000;
		n = Ppm / 305176;
		Ppm -= 305176 * n;
		m = Ppm / 5086;
		printk("SecondOffset is: %d\n", n);
		printk("MinuteOffset is: %d\n", m);
		if ((n <= 7) && (m <= 63)) {
			accelerate_second_rtc(n);
			accelerate_minute_rtc(m);
			printk("rtc clock has been adjusted!\n");
		} else {
			printk("beyond range of adjust\n");
		}

		return 0;
	}
	for (i = 0; i < 27; i++) {	/*calculate temperature by voltage*/
		if ((v >= SadcValue[i]) && (v < SadcValue[i+1])) {
			T = Temperature1[i] - ((Temperature1[i] - Temperature1[i+1]) *
					(SadcValue[i] - v) / (SadcValue[i] - SadcValue[i+1]));
		} else {
			//printk("the reading of SADC is beyond of voltage range\n");
			continue;
		}
	}
	for (i = 0; i < 135; i++) {	/*get deviation by temperature*/
		if ((T >= Temperature2[i]) && (T < Temperature2[i+1])) {
			num = i;
			flag = 1;
			if ((Temperature2[num+1] - T) <= 500) {
				T = Temperature2[num + 1];
				Ppm = Deviation[num + 1];
			} else if ((Temperature2[num+1] - T) > 500) {
				T = Temperature2[num];
				Ppm = Deviation[num];
			}
			printk("current temperature is: %ld\n", T);
			printk("current deviation of RTC crystal oscillator is: %ld\n", Ppm);
		}
	}
	if (flag == 1) {
		n = Ppm / 305176;
		Ppm -= 305176 * n;
		m = Ppm / 5086;
		printk("SecondOffset is: %d\n", n);
		printk("MinuteOffset is: %d\n", m);
		if ((n <= 7) && (m <= 63)) {
			accelerate_second_rtc(n);
			accelerate_minute_rtc(m);
			printk("rtc clock has been adjusted!\n");
		} else {
			printk("beyond range of adjust\n");
		}
	}
	return 0;
}

long get_rtc_temperature(void)
{
	long T = 0;
	int i, j, temp;
	long value[7];
	long sum = 0;
	long v;
	for (i = 0; i < 7; i++) {
		value[i] = fh_sadc_get_value(fh_rtc->sadc_channel);
		if(!value[i])
		{
			printk("ERROR: %s, sadc value %lu is incorrect\n",
					__func__, value[i]);
			return -EIO;
		}
		mdelay(100);
	}
	for (i = 0; i < 7; i++) {
		for (j = i + 1; j < 7; j++) {
			if (value[j] < value[i]) {
				temp = value[i];
				value[i] = value[j];
				value[j] = temp;
			}
		}
	}
	sum = value[2] + value[3] + value[4];
	v = sum / 3;
	printk("the average value of SADC is:%ld\n", v);
	for (i = 0; i < 27; i++) {
		if ((v >= SadcValue[i]) && (v < SadcValue[i+1])) {
			T = Temperature1[i] - ((Temperature1[i] - Temperature1[i+1]) *
					(SadcValue[i] - v) / (SadcValue[i] - SadcValue[i+1]));
		} else {
			//printk("the reading of SADC is beyond of voltage range\n");
			continue;
		}
	}
	printk("current temperature is: %ld\n", T);
	return 0;
}

void fh_rtc_self_adjustment(struct work_struct *work)
{
	fh_adjust_rtc();

	queue_delayed_work(fh_rtc->wq, &fh_rtc->self_adjust, 5000);
}


static void create_proc_rtc(struct fh_rtc_controller *rtc);
static void remove_proc(void);
static int __devinit fh_rtc_probe(struct platform_device *pdev)
{
	int err = 0;
	struct resource *ioarea;
	struct fh_rtc_platform_data * rtc_platform_info;
	struct rtc_device *rtc;
	struct resource *res;

	fh_rtc = kzalloc(sizeof(struct fh_rtc_controller), GFP_KERNEL);
	if (!fh_rtc)
		return -ENOMEM;

	memset(fh_rtc, 0, sizeof(struct fh_rtc_controller));

	/* board info below */
	rtc_platform_info = (struct fh_rtc_platform_data *)pdev->dev.platform_data;
	if(rtc_platform_info == NULL){
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
	ioarea = request_mem_region(res->start, resource_size(res),
			pdev->name);
	if(!ioarea) {
		dev_err(&pdev->dev, "rtc region already claimed\n");
		err = -EBUSY;
		goto err_nores;
	}

	fh_rtc->regs = ioremap(res->start, resource_size(res));
	if (!fh_rtc->regs) {
		dev_err(&pdev->dev, "rtc already mapped\n");
		err = -EINVAL;
		goto err_nores;
	}

	/* register RTC and exit */
	platform_set_drvdata(pdev, fh_rtc);
	rtc = rtc_device_register(rtc_platform_info->dev_name, &pdev->dev, &fh_rtcops,
				  THIS_MODULE);

	if (IS_ERR(rtc)) {
		dev_err(&pdev->dev, "cannot attach rtc\n");
		err = PTR_ERR(rtc);
		goto err_nores;
	}
	fh_rtc->rtc = rtc;

	err = request_irq(fh_rtc->irq , fh_rtc_irq, 0,
			  dev_name(&pdev->dev), fh_rtc);
	if (err) {
		dev_dbg(&pdev->dev, "request_irq failed, %d\n", err);
		goto err_nores;
	}

	create_proc_rtc(fh_rtc);

	SET_REG(fh_rtc->regs + FH_RTC_DEBUG, RTC_PHASE);

	if(fh_rtc->sadc_channel >= 0)
	{
		pr_info("RTC: start self adjustment\n");
		fh_rtc->wq = create_workqueue("rtc_wq");
		if(!fh_rtc->wq)
		{
			dev_err(&pdev->dev, "no memory to create rtc workqueue\n");
			return -ENOMEM;
		}
		INIT_DELAYED_WORK(&fh_rtc->self_adjust, fh_rtc_self_adjustment);

		queue_delayed_work(fh_rtc->wq, &fh_rtc->self_adjust, 5000);
	}

	err = fh_rtc_exam_magic();

	if(err)
		return -1;


err_nores:
	return err;

	return 0;
}

static int __devexit fh_rtc_remove(struct platform_device *dev)
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



static struct platform_driver fh_rtc_driver = {
	.probe		= fh_rtc_probe,
	.remove		= __devexit_p(fh_rtc_remove),
	.suspend	= fh_rtc_suspend,
	.resume		= fh_rtc_resume,
	.driver		= {
		.name	= "fh_rtc",
		.owner	= THIS_MODULE,
	},
};


static int __init fh_rtc_init(void) {

	return platform_driver_register(&fh_rtc_driver);
}

static void __exit fh_rtc_exit(void) {
	platform_driver_unregister(&fh_rtc_driver);
}

static void del_char(char *str, char ch)
{
	char *p = str;
	char *q = str;
	while (*q) {
		if (*q != ch) {
			*p++ = *q;
		}
		q++;
	}
	*p = '\0';
}

ssize_t proc_read(char *page, char **start, off_t off, int count,
		int *eof, struct fh_rtc_controller *data) {
	ssize_t len = 0;

	printk(KERN_INFO "------------- dump register -------------\n");
	printk(KERN_INFO "cnt:0x%x\n",fh_rtc_get_time(data->regs)  );
	printk(KERN_INFO "offset:0x%x\n",fh_rtc_get_offset(data->regs));
	printk(KERN_INFO "fail:0x%x\n",fh_rtc_get_power_fail(data->regs));
	printk(KERN_INFO "alarm_cnt:0x%x\n",fh_rtc_get_alarm_time(data->regs));
	printk(KERN_INFO "int stat:0x%x\n",fh_rtc_get_int_status(data->regs));
	printk(KERN_INFO "int en:0x%x\n",fh_rtc_get_enabled_interrupt(data->regs));
	printk(KERN_INFO "sync:0x%x\n",fh_rtc_get_sync(data->regs));
	printk(KERN_INFO "debug:0x%x\n",fh_rtc_get_debug(data->regs));
	printk(KERN_INFO "-------------------------------------------\n");

	return len;
}


static ssize_t fh_rtc_proc_write(struct file *filp, const char *buf, size_t len, loff_t *off)
{
	char message[32] = {0};
	char * const delim = ",";
	char *cur = message, *power_str;
	int power;
	len = (len > 32) ? 32 : len;
	if (copy_from_user(message, buf, len))
		return -EFAULT;
	power_str = strsep(&cur, delim);
	if (!power_str) {
		pr_err("%s: ERROR: parameter is empty\n", __func__);
		return -EINVAL;
	} else {
		del_char(power_str, ' ');
		del_char(power_str, '\n');
		power = (unsigned int)simple_strtoul(power_str, NULL, 10);
		if (power < 0) {
			pr_err("%s: ERROR: parameter is incorrect\n", __func__);
			return -EINVAL;
		}
		printk(KERN_INFO "the diff between rtc and sys is %d\n",
				power);
		if (power == 0)
			fh_adjust_rtc();
		else if (power == 1)
			get_rtc_temperature();
	}
	return len;
}

static void create_proc_rtc(struct fh_rtc_controller *rtc)
{
	fh_rtc->proc_rtc_entry =
			create_proc_entry(FH_RTC_PROC_FILE,
					S_IRUGO, NULL);

	if (!fh_rtc->proc_rtc_entry) {
		printk(KERN_ERR"create proc failed\n");
	} else {
		fh_rtc->proc_rtc_entry->read_proc =
				(read_proc_t *)proc_read;
		fh_rtc->proc_rtc_entry->write_proc =
				(write_proc_t *)fh_rtc_proc_write;
		fh_rtc->proc_rtc_entry->data = rtc;
	}
}

static void remove_proc(void) {
    remove_proc_entry(FH_RTC_PROC_FILE, NULL);
}

module_init(fh_rtc_init);
module_exit(fh_rtc_exit);

MODULE_DESCRIPTION("FH SOC RTC Driver");
MODULE_AUTHOR("yu.zhang <zhangy@fullhan.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:fh-rtc");
