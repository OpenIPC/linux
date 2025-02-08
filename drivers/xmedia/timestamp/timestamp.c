/*
 *  Copyright (c) XMEDIA. All rights reserved.
 */
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/div64.h>
#include <linux/io.h>
#include <linux/export.h>
#include <linux/clk.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sched_clock.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/signal.h>
#include <linux/poll.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/usb.h>
#include <linux/usbdevice_fs.h>
#include <linux/usb/hcd.h>
#include <linux/cdev.h>
#include <linux/notifier.h>
#include <linux/security.h>
#include <linux/user_namespace.h>
#include <linux/scatterlist.h>
#include <linux/uaccess.h>
#include <linux/dma-mapping.h>
#include <asm/byteorder.h>
#include <linux/moduleparam.h>
#include <linux/xmedia/timestamp.h>

/* stopwatch start */
#define STOPWATCH_BUF_SIZE 500
#define MISC_CTRL62    0x120280F8
#define MISC_CTRL63    0x120280FC
#define MISC_CTRL64    0x12028100
#define MISC_CTRL65    0x12028104
#define MISC_CTRL66    0x12028108
#define TRIGGER_BIT    0
#define CLEAR_BIT      1
#define SELECT_BIT     2
#define SELECT_BIT_NUM 3
#ifdef CONFIG_ARCH_XM720XXX
#define RECORD_NUM     10
#define RUNNING_TRIGGER_BIT 16
#else
#define RECORD_NUM     5
#endif
#define REG_OPERATION_DELAY_US (10 * 1000)

u32 *reg_trigger = NULL;
u32 *reg_result  = NULL;
u32 *reg_result_h  = NULL;
#ifdef CONFIG_ARCH_XM720XXX
u32 *running_result = NULL;
u32 *running_result_h = NULL;
#endif
static struct proc_dir_entry *stopwatch_entry;
/* stopwatch end */

#define TIMESTAMP 1
#define TIMESTAMP_MINOR 124
#define CMD_TIMESTAMP_MARK   	_IOW(TIMESTAMP, 0x01 , timestamp_item)
#define CMD_TIMESTAMP_PRINT  	_IOWR(TIMESTAMP, 0x02, int)
#define CMD_TIMESTAMP_CLEAN     _IO(TIMESTAMP ,0x03)

#ifdef CONFIG_ARCH_XM720XXX
#define UNIT_STR "us"
#define TIMESTAMP_FMT "%-16llu"UNIT_STR
#else
#define UNIT_STR "ms"
#define TIMESTAMP_FMT "%-8u"UNIT_STR
#endif

#define SYS_CTRL_REG_BASE       0x12020000
#define REG_SYS_CTRL_REG_BASE   0x0000
#define CFG_TIMER_CTRL          0xC2
#define TIMER2_REG_BASE         0x12001000
#define TIME2_CLK_SEL           (1 << 18)
#define TIMER2_ENABLE           (1 << 7)
#define TIMER2_CIRCLE_MODE      (1 << 6)
#define TIMER2_GRADE8_PRESCALE  (1 << 3)
#define TIMER2_32_BIT_MODE      (1 << 1)
#define CONFIG_TIME_STAMP_ON 1
#define TIMESTAMP_MAX_CNT 200
#define TIMESTAMP_MIN_CNT 1

#define TIMESTAMP_CNT_ADDR             0x1202016C
#define TIMESTAMP_ITEM_START_ADDR      0x04019100
#define UBOOT_TIMESTAMP 0
#define KERNEL_TIMESTAMP 1
#define USER_TIMESTAMP 2


#if (defined(TIMER2_REG_BASE) && defined(SYS_CTRL_REG_BASE))
/* defined in SoC Data Sheet */
#define TIMER_REG_RELOAD (TIMER2_REG_BASE + 0x000)
#define TIMER_REG_VALUE (TIMER2_REG_BASE + 0x004)
#define TIMER_REG_CONTROL (TIMER2_REG_BASE + 0x008)
#endif

#ifndef TIMER_CLK_MASK
#define TIMER_CLK_MASK (1 << 18) /* timeren2ov */
#endif
#define BUS_CLK 50000000U  /* sysapb_cksel */
#define TIMER_CLK 3000000U /* timer default cksel */

#ifdef CONFIG_ARCH_XM720XXX
static timestamp_type stopwatch_get_running_timer(void);
#define GET_MS_CLOCK(value) (value)
#else
#define GET_MS_CLOCK(value) ((unsigned int)((u32)(value) * g_timestamp_ctx.timerpre / (g_timestamp_ctx.timer_clock / 1000U)))
#endif

void *reload_mem = NULL;
void *value_mem = NULL;
void *control_mem = NULL;
void *sysctl_mem =  NULL;
void *cnt_mem = NULL;
void *start_mem = NULL;

int init_flag = 0;
/* -------------------------Mutex Lock------------------------------ */
#define MUTEX_INIT_LOCK(mutex) do { \
    (void)mutex_init(&(mutex)); \
} while (0)
#define MUTEX_LOCK(mutex) do { \
    (void)mutex_lock(&(mutex)); \
} while (0)
#define MUTEX_UNLOCK(mutex) do { \
    (void)mutex_unlock(&(mutex)); \
} while (0)

void write_register_api(void *mem, unsigned int value);
void read_register_api(void *mem, unsigned int *value);
static int stopwatch_init(void);

#define DEFAULT_MD_LEN 128

/* TimeStamp Context */
typedef struct {
	unsigned int timer_clock;
	unsigned int timerpre;
	struct mutex mutex_lock;
	timestamp_item *items;
	unsigned int item_cur_cnt;
	unsigned int item_max_cnt;
} timestamp_context;

static timestamp_context g_timestamp_ctx = {
	.timer_clock = BUS_CLK,
	.timerpre = 256U, /* 256: default timerpre */
	.mutex_lock = NULL,
	.items = NULL,
	.item_cur_cnt = 0,
	.item_max_cnt = 0,
};


void *reg_iommap(void *phyaddr, unsigned int pagesize)
{
	unsigned char *p_pageaddr = NULL;

	p_pageaddr = ioremap(phyaddr, pagesize);

	if (p_pageaddr == NULL) {
		printk(KERN_ERR "ioremap error\n");
		return NULL;
	}

    return (void *)p_pageaddr;
}

void *reg_munmap(void *phyaddr)
{
	if(phyaddr == NULL) {
		printk(KERN_ERR "phy addr is NULL\n");
		return NULL;
	}
	iounmap(phyaddr);
}

void write_register_api(void *mem, unsigned int value)
{
	if (mem == NULL) {
		printk(KERN_ERR "sys mmap failed\n");
		return;
	}

	writel(value, mem);

	return;
}

void read_register_api(void *mem, unsigned int *value)
{
	if (value == NULL) {
		printk(KERN_ERR "value is null\n");
		return;
	}
	if (mem == NULL) {
		printk(KERN_ERR "read mem addr NULL\n");
		return;
	}
	*value = readl(mem);

	return;
}


void enable_timer2(void)
{
	unsigned int reg = 0;
	unsigned int reg1 = 0;

	write_register_api(control_mem, 0);
	write_register_api(reload_mem, ~0);

	write_register_api(control_mem, CFG_TIMER_CTRL);

	read_register_api(sysctl_mem, &reg);
	reg |= TIME2_CLK_SEL;

	write_register_api(sysctl_mem, reg);
	write_register_api(control_mem, 0);
	write_register_api(reload_mem, 0);

	reg = TIMER2_ENABLE | TIMER2_CIRCLE_MODE | TIMER2_GRADE8_PRESCALE | TIMER2_32_BIT_MODE;
	write_register_api(control_mem, reg);
	read_register_api(control_mem, &reg1);
}


static int timestamp_get_timer_attr(void)
{
	/* get timer control */
	unsigned int timerctrl;
	read_register_api(control_mem, &timerctrl);
	if ((timerctrl & (1 << 7)) == 0) { /* bit[7] of timerctrl is timer enable */
		enable_timer2();
		read_register_api(control_mem, &timerctrl);
		if ((timerctrl & (1 << 7)) == 0) { /* bit[7] of timerctrl is timer enable */
			printk(KERN_ERR "Timer  enable  failed\n");
			return -2;
		}
		init_flag = 1;
	}

	/* get timer clock */
	unsigned int timerenov;
	read_register_api(sysctl_mem, &timerenov);
	timerenov = timerenov & TIMER_CLK_MASK;
	g_timestamp_ctx.timer_clock = (timerenov == 0) ? TIMER_CLK : BUS_CLK;

	/* get timer prescaling factor configuration */
	switch (timerctrl & 0xc) {            /* 0xc: timerpre is bits[3:2] of timerctrl */
		case 0x0:                         /* 0x0: no prescaling */
			g_timestamp_ctx.timerpre = 1U; /* the clock frequency of the timer is divided by 1. */
			break;
		case 0x4:                          /* 0x4: 4-level prescaling */
			g_timestamp_ctx.timerpre = 16U; /* the clock frequency of the timer is divided by 16. */
			break;
		case 0x8: /* 0x8: 8-level prescaling */
		case 0xc: /* 0xc: undefined. If the bits are set to 11, 8-level prescaling is considered */
		default:
			g_timestamp_ctx.timerpre = 256U; /* the clock frequency of the timer is divided by 256 */
			break;
	}

    return 0;
}

static int timestamp_check_mem(unsigned int count)
{
	if (count < TIMESTAMP_MIN_CNT) {
		printk(KERN_ERR "count should greater than %u!\n", TIMESTAMP_MIN_CNT);
		return -1;
	} else if (count > TIMESTAMP_MAX_CNT) {
		printk(KERN_ERR "count should less than %u!\n", TIMESTAMP_MAX_CNT);
		return -1;
	}

    return 0;
}

static int timer2_reg_iommap(void)
{
	reload_mem = reg_iommap(TIMER_REG_RELOAD, PAGE_SIZE);
    if(reload_mem == NULL) {
		printk(KERN_ERR "reload mem iommap failed\n");
		return -1;
	}
	value_mem = reg_iommap(TIMER_REG_VALUE, PAGE_SIZE);
	if(value_mem == NULL) {
		printk(KERN_ERR "value mem iommap failed\n");
		return -1;
	}
	control_mem = reg_iommap(TIMER_REG_CONTROL, PAGE_SIZE);
	if(control_mem == NULL) {
		printk(KERN_ERR "control mem iommap failed\n");
		return -1;
	}
	sysctl_mem =  reg_iommap(SYS_CTRL_REG_BASE + REG_SYS_CTRL_REG_BASE, PAGE_SIZE);
	if(sysctl_mem == NULL) {
		printk(KERN_ERR "sysctl mem iommap failed\n");
		return -1;
	}
	cnt_mem = reg_iommap(TIMESTAMP_CNT_ADDR, PAGE_SIZE);
	if(cnt_mem == NULL) {
		printk(KERN_ERR "cnt_mem mem iommap failed\n");
		return -1;
	}
	start_mem = reg_iommap(TIMESTAMP_ITEM_START_ADDR, PAGE_SIZE);
	if(start_mem == NULL) {
		printk(KERN_ERR "cnt_mem mem iommap failed\n");
		return -1;
	}
	return 0;
}

static int get_uboot_timestamp(void)
{
	int cnt = 0;

	read_register_api(cnt_mem, &cnt);
	if((cnt > 0) && (cnt < TIMESTAMP_MAX_CNT)) {
		int i = 0;
		timestamp_item *item = g_timestamp_ctx.items;
		timestamp_item *uboot_items = (timestamp_item *)(start_mem);

		for (i; i < cnt; i++) {
			if(uboot_items[i].type == UBOOT_TIMESTAMP) {
				timestamp_type time = GET_MS_CLOCK(uboot_items[i].stamp);
				item[g_timestamp_ctx.item_cur_cnt].stamp = time;
				item[g_timestamp_ctx.item_cur_cnt].func = uboot_items[i].func;
				item[g_timestamp_ctx.item_cur_cnt].line = uboot_items[i].line;
				item[g_timestamp_ctx.item_cur_cnt].type = uboot_items[i].type;
				g_timestamp_ctx.item_cur_cnt++;
			}
		}
	}

	return cnt;
}


int timestamp_init(unsigned int count)
{
	int ret = timestamp_check_mem(count);
	if(ret != 0) {
		printk(KERN_ERR "ret failed ret%d\n",ret);
		return -1;
	}

	mutex_init(&(g_timestamp_ctx.mutex_lock));

	MUTEX_LOCK(g_timestamp_ctx.mutex_lock);
	if (g_timestamp_ctx.items != NULL) {
		if (count == g_timestamp_ctx.item_max_cnt) {
			MUTEX_UNLOCK(g_timestamp_ctx.mutex_lock);
			return 0;
		} else {
			MUTEX_UNLOCK(g_timestamp_ctx.mutex_lock);
			printk(KERN_ERR "time2 count init failed\n");
			return -2;
		}
	}

	if(timer2_reg_iommap() != 0) {
		printk(KERN_ERR "timer2 reg iommap failed\n");
		return -2;
	}
#ifndef CONFIG_ARCH_XM720XXX
	if (timestamp_get_timer_attr() != 0) {
		MUTEX_UNLOCK(g_timestamp_ctx.mutex_lock);
		printk(KERN_ERR "timer2 get timestamp failed\n");
		return -2;
	}
#endif
	g_timestamp_ctx.items = kmalloc(count * sizeof(timestamp_item), GFP_KERNEL);
	if (g_timestamp_ctx.items == NULL) {
		printk(KERN_ERR "xcam_zalloc failed\n");
		MUTEX_UNLOCK(g_timestamp_ctx.mutex_lock);
		return -1;
	}
	g_timestamp_ctx.item_max_cnt = count;

	MUTEX_UNLOCK(g_timestamp_ctx.mutex_lock);
	if(init_flag == 0) {
		//timestamp已经在uboot阶段初始化获取uboot阶段的时间戳数据 type=0
		get_uboot_timestamp();
		TIME_STAMP(KERNEL_TIMESTAMP);
	}

    return 0;
}
EXPORT_SYMBOL(timestamp_init);

void timestamp_deinit(void)
{
	MUTEX_LOCK(g_timestamp_ctx.mutex_lock);
	if(g_timestamp_ctx.items != NULL) {
		kfree(g_timestamp_ctx.items);
		g_timestamp_ctx.items = NULL;
		g_timestamp_ctx.item_cur_cnt = 0;
		g_timestamp_ctx.item_max_cnt = 0;
		if(value_mem != NULL) {
			reg_munmap(value_mem);
			value_mem = NULL;
		}
		if(control_mem != NULL) {
			reg_munmap(control_mem);
			control_mem = NULL;
		}
		if(reload_mem != NULL){
			reg_munmap(reload_mem);
			reload_mem = NULL;
		}
		if(sysctl_mem != NULL) {
			reg_munmap(sysctl_mem);
			sysctl_mem = NULL;
		}
	}
	MUTEX_UNLOCK(g_timestamp_ctx.mutex_lock);
}
EXPORT_SYMBOL(timestamp_deinit);

static void timestamp_get_time(timestamp_type *stamp)
{
#ifdef CONFIG_ARCH_XM720XXX
	timestamp_type value = stopwatch_get_running_timer();
	*stamp = GET_MS_CLOCK(value);
#else
	timestamp_type tick = 0;
	read_register_api(value_mem, &tick);
	tick = ~tick;
	u32 clock = g_timestamp_ctx.timer_clock / 1000U;
	u64 temp = (u64)tick * g_timestamp_ctx.timerpre;
	do_div(temp, clock);
	*stamp = temp;
#endif
}

static int timestamp_mark_api(const char *func, unsigned int line, unsigned int type)
{
	timestamp_type stamp;
	timestamp_get_time(&stamp);
	MUTEX_LOCK(g_timestamp_ctx.mutex_lock);
	if (g_timestamp_ctx.items == NULL) {
		MUTEX_UNLOCK(g_timestamp_ctx.mutex_lock);
		return -2;
	}
	if (g_timestamp_ctx.item_cur_cnt >= g_timestamp_ctx.item_max_cnt) {
		g_timestamp_ctx.item_cur_cnt = 0;
		printk(KERN_INFO "\n time_stamp is full!!! now clean the buffer to start from 0\n");
	}

	timestamp_item *item = g_timestamp_ctx.items;
	item[g_timestamp_ctx.item_cur_cnt].stamp = stamp;
	item[g_timestamp_ctx.item_cur_cnt].func = (char *)func;
	item[g_timestamp_ctx.item_cur_cnt].line = line;
	item[g_timestamp_ctx.item_cur_cnt].type = type;
	g_timestamp_ctx.item_cur_cnt++;
	MUTEX_UNLOCK(g_timestamp_ctx.mutex_lock);
	return 0;
}


int timestamp_mark(const char *func, unsigned int line, unsigned int type)
{
	int ret = -1;

	if(type != KERNEL_TIMESTAMP) {
		printk(KERN_ERR "mark type invalid,uboot type=0 kernel type=1 user type=2\n");
		return -1;
	}

	ret = timestamp_mark_api(func, line, type);

	return ret;
}
EXPORT_SYMBOL(timestamp_mark);

int timestamp_clean(void)
{
	MUTEX_LOCK(g_timestamp_ctx.mutex_lock);
	if (g_timestamp_ctx.items == NULL) {
		MUTEX_UNLOCK(g_timestamp_ctx.mutex_lock);
		return -2;
	}
	if ((g_timestamp_ctx.item_max_cnt != 0) && (UINT_MAX / g_timestamp_ctx.item_max_cnt > sizeof(timestamp_item))) {
		unsigned int size = sizeof(timestamp_item) * g_timestamp_ctx.item_max_cnt;
		(void)memset(g_timestamp_ctx.items, 0, size);
		g_timestamp_ctx.item_cur_cnt = 0;
	}
	MUTEX_UNLOCK(g_timestamp_ctx.mutex_lock);
	return 0;
}
EXPORT_SYMBOL(timestamp_clean);

int timestamp_print(unsigned int type)
{
	MUTEX_LOCK(g_timestamp_ctx.mutex_lock);
	if (g_timestamp_ctx.items == NULL) {
		MUTEX_UNLOCK(g_timestamp_ctx.mutex_lock);
		printk(KERN_ERR "TimeStamp is not init.\n");
		return -2;
	}
	timestamp_type time0 = 0;
	timestamp_type time = 0;
	timestamp_item *items = g_timestamp_ctx.items;
	timestamp_type stamp = 0;
	unsigned int i = 0;
	unsigned cnt_timestamp = 0;
	if(type == UBOOT_TIMESTAMP)
	{
		printk(KERN_INFO "uboot timestmap info:\n");
	}
	else if(type == KERNEL_TIMESTAMP)
	{
		printk(KERN_INFO "kernel timestmap info:\n");
	}
	else if(type == USER_TIMESTAMP)
	{
		printk(KERN_INFO "user timestmap info:\n");
	}
	else
	{
		printk(KERN_ERR "timestamp type invalid \n");
		return -1;
	}
	for (i; i < g_timestamp_ctx.item_cur_cnt; i++) {
		if (type == items[i].type) {
			time = items[i].stamp;
			char *func = items[i].func;
			unsigned int line = items[i].line;
			if ((func != NULL) && (type != UBOOT_TIMESTAMP)) {
				printk(KERN_INFO "time stamp[%-3u] = "TIMESTAMP_FMT" gap = "TIMESTAMP_FMT""
					" type: %-5u  line: %-5u func: %s\n",
					i + 1, time, time - stamp,
					items[i].type, line, func);
			} else {
				printk(KERN_INFO "time stamp[%-3u] = "TIMESTAMP_FMT" gap = "TIMESTAMP_FMT""
					" type: %-5u  line: %-5u\n",
					i + 1, time, time - stamp,
					items[i].type, line);
			}
			stamp = time;
			if (cnt_timestamp == 0) {
				time0 = time;
			}
			cnt_timestamp++;
		}
	}
	if(cnt_timestamp > 0)
		printk(KERN_INFO "timestamp type[%d] total time from stamp[%-3u] to [%-3u]  gap = "TIMESTAMP_FMT"\n\n", type, 1U, cnt_timestamp, time - time0);
	else
		printk(KERN_INFO "timestamp no data\n\n");
	MUTEX_UNLOCK(g_timestamp_ctx.mutex_lock);
	return 0;
}
EXPORT_SYMBOL(timestamp_print);

timestamp_type timestamp_get_ms(void)
{
	timestamp_type cur_time;
	timestamp_get_time(&cur_time);
	MUTEX_LOCK(g_timestamp_ctx.mutex_lock);
	if (g_timestamp_ctx.items == NULL) {
		MUTEX_UNLOCK(g_timestamp_ctx.mutex_lock);
		return 0;
	}
	MUTEX_UNLOCK(g_timestamp_ctx.mutex_lock);
	return cur_time;
}
EXPORT_SYMBOL(timestamp_get_ms);

static long timestamp_ioctl(struct file *file, unsigned int cmd, unsigned long pargs)
{
	int type;
	timestamp_item markparam;

	switch(cmd)
	{
		case CMD_TIMESTAMP_MARK:
			if(pargs == NULL) {
				printk(KERN_ERR "paramaters is invalid\n");
				return -1;
			}
			if (copy_from_user(&markparam, (void __user *)pargs, sizeof(timestamp_item))) {
				printk(KERN_ERR "copy from user param is failed\n");
				return -1;
			}
			if(markparam.type != USER_TIMESTAMP) {
				printk(KERN_ERR "mark type invalid,uboot type=0 kernel type=1 user type=2\n");
				return -1;
			}
			timestamp_mark_api(markparam.func, markparam.line, markparam.type);
			return 0;
		case CMD_TIMESTAMP_PRINT:
			if(pargs == NULL) {
				printk(KERN_ERR "paramaters is invalid\n");
				return -1;
			}
			if (copy_from_user(&type, (void __user *)pargs, sizeof(int))) {
				printk(KERN_ERR "copy from user param is failed\n");
				return -1;
			}
			timestamp_print(type);
			return 0;
		case CMD_TIMESTAMP_CLEAN:
			timestamp_clean();
			return 0;
		default:
			break;
	}

	return 0;
}


static int timestamp_open(struct inode *inode, struct file *file)
{
	int ret = 0;

#ifndef CONFIG_ARCH_XM720XXX
	//判断timer2是否打开
	unsigned int timerctrl;
	read_register_api(control_mem, &timerctrl);
	if ((timerctrl & (1 << 7)) == 0) {
		printk(KERN_ERR "timer2 is not enable\n");
		ret = -1;
	}
#endif
	return ret;
}

static int timestamp_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations timestamp_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = timestamp_ioctl,
	.open           = timestamp_open,
	.release        = timestamp_release,
};

static struct miscdevice timestamp_miscdev = {
	.minor  = TIMESTAMP_MINOR,
	.name   = "timestamp",
	.fops   = &timestamp_fops,
};


/* stopwatch */
static ssize_t stopwatch_write(struct file *file, const char __user *ubuf, size_t count,  loff_t *ppos)
{
	int in, ret;

	if (*ppos > 0)
		return 0;

	*ppos  += count;

	in = simple_strtoul(ubuf, NULL, 10);
	if (in == 1) {
		ret = stopwatch_trigger();
		if (ret)
			goto error;
	} else if (in == 2) {
		ret = stopwatch_clear();
		if (ret)
			goto error;
	} else {
		ret = -EINVAL;
		goto error;
	}

error:
	return count;
}

static ssize_t stopwatch_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
	int i, len;
	u64 record[RECORD_NUM] = {0};
	char buf[STOPWATCH_BUF_SIZE] = {0};
#ifdef CONFIG_ARCH_XM720XXX
	u64 current_record[2] = {0};
#endif

	if (*ppos > 0 || count < STOPWATCH_BUF_SIZE)
		return 0;

	if (reg_result == NULL)
		return -EBUSY;

	for (i = 0; i < RECORD_NUM; i++) {
		writel(i << SELECT_BIT, reg_trigger);
		record[i] = readl(reg_result);
#ifdef CONFIG_ARCH_XM720XXX
		record[i] += (u64)readl(reg_result_h) << 32;
#endif
	}

	len = 0;
	memset(buf, 0, sizeof(buf));
	len += snprintf(buf, sizeof(buf) - len, "--------------------------\n");
	len += snprintf(buf + len, sizeof(buf) - len, "STOPWATCH[0-%d]:\n", RECORD_NUM - 1);
	for (i = 0; i < RECORD_NUM; i++) {
#ifdef CONFIG_ARCH_XM720XXX
		len += snprintf(buf + len, sizeof(buf) - len, "%-3d  %-16lluus\n", i, record[i]);
#else
		len += snprintf(buf + len, sizeof(buf) - len, "%-3d  %-16llums\n", i, record[i]);
#endif
	}
#ifdef CONFIG_ARCH_XM720XXX
	writel(1 << RUNNING_TRIGGER_BIT, reg_trigger);
	udelay(4);
	current_record[0] = readl(running_result);
	current_record[1] = (u64)readl(running_result_h);
	len += snprintf(buf + len, sizeof(buf) - len, "CURRENT TIME: %lluus\n", current_record[0] + current_record[1]);
#endif
	len += snprintf(buf + len, sizeof(buf) - len, "--------------------------\n");

	if (copy_to_user(ubuf, buf, len))
		return -EFAULT;

	*ppos = len;

	return len;
}

static struct file_operations stopwatch_ops = {
	.owner = THIS_MODULE,
	.read  = stopwatch_read,
	.write = stopwatch_write,
};


static int stopwatch_init(void)
{
	reg_trigger = reg_iommap(MISC_CTRL62, PAGE_SIZE);
	if (reg_trigger == NULL) {
		printk(KERN_ERR "Fail to remap stopwatch reg\n");
		return -1;
	}
	reg_result   = reg_trigger + 1;
	reg_result_h = reg_trigger + 2;
#ifdef CONFIG_ARCH_XM720XXX
	running_result = reg_trigger + 3;
	running_result_h = reg_trigger +4;
#endif

	stopwatch_entry = proc_create("stopwatch", 0644, NULL, &stopwatch_ops);
	if (stopwatch_entry == NULL) {
		pr_err("Fail to create stopwatch proc file\n");
		return -1;
	}

	return 0;
}

static timestamp_type stopwatch_get(u32 point)
{
	writel(point << SELECT_BIT, reg_trigger);
	udelay(REG_OPERATION_DELAY_US);
	timestamp_type value = readl(reg_result);
#ifdef CONFIG_ARCH_XM720XXX
	value += (u64)readl(reg_result_h) << 32;
#endif
	return value;
}

#ifdef CONFIG_ARCH_XM720XXX
static timestamp_type stopwatch_get_running_timer(void)
{
	writel(1 << RUNNING_TRIGGER_BIT, reg_trigger);
	udelay(4);
	timestamp_type value = readl(running_result);
	value += (u64)readl(running_result_h) << 32;
	return value;
}
#endif

int stopwatch_trigger(void)
{
	u32 record;

	if (reg_trigger == NULL || reg_result == NULL) {
		return -EBUSY;
	}
	writel((RECORD_NUM - 1) << SELECT_BIT, reg_trigger);
	record = readl(reg_result);
	if (record > 0) {
		pr_err("No more slot\n");
		return -ENOMEM;
	}

	writel(1 << TRIGGER_BIT, reg_trigger);
	return 0;
}
EXPORT_SYMBOL(stopwatch_trigger);

int stopwatch_clear(void)
{
	if (reg_trigger == NULL) {
	   return -EBUSY;
	}
	writel(1 << CLEAR_BIT, reg_trigger);
	return 0;
}
EXPORT_SYMBOL(stopwatch_clear);

void stopwatch_print(void)
{
	int i, len;
	u64 record[RECORD_NUM] = {0};
	char buf[STOPWATCH_BUF_SIZE] = {0};

	if (reg_result == NULL)
		return -EBUSY;

	for (i = 0; i < RECORD_NUM; i++) {
		writel(i << SELECT_BIT, reg_trigger);
		record[i] = readl(reg_result);
#ifdef CONFIG_ARCH_XM720XXX
		record[i] += (u64)readl(reg_result_h) << 32;
#endif
	}

    len = 0;
    memset(buf, 0, sizeof(buf));
    len += snprintf(buf, sizeof(buf) - len, "--------------------------\n");
    len += snprintf(buf + len, sizeof(buf) - len, "STOPWATCH[0-%d]:\n", RECORD_NUM - 1);
    for (i = 0; i < RECORD_NUM; i++) {
#ifdef CONFIG_ARCH_XM720XXX
        len += snprintf(buf + len, sizeof(buf) - len, "%-3d  %-16lluus\n", i, record[i]);
#else
        len += snprintf(buf + len, sizeof(buf) - len, "%-3d  %-16llums\n", i, record[i]);
#endif
    }

	len += snprintf(buf + len, sizeof(buf) - len, "--------------------------\n");
	printk(KERN_INFO "%s\n", buf);

    return len;
}
EXPORT_SYMBOL(stopwatch_print);



static void  timestamps_exit(void)
{
	timestamp_deinit();
	proc_remove(stopwatch_entry);
	return 0;
}


static int  timestamps_init(void)
{
	int ret = -1;

	//stopwatch初始化
	ret = stopwatch_init();
	if(ret != 0) {
		printk(KERN_ERR "stopwatch init failed\n");
		return ret;
	}
	//timestamp 初始化
	ret = timestamp_init(TIMESTAMP_MAX_CNT);
	if(ret != 0) {
		printk(KERN_ERR "timestamp init failed\n");
		return ret;
	}
	//注册timestamp misc设备
	ret = misc_register(&timestamp_miscdev);
	if (ret) {
		printk(KERN_ERR "register timestamp misc device failed\n");
		return ret;
	}
	return 0;
}

module_init(timestamps_init);
module_exit(timestamps_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Xmedia");

