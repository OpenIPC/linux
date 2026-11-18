/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sched.h>

#include <linux/device.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/of_platform.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>
#include <linux/ioctl.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/ax_timestamp.h>
#include <asm/div64.h>
#include <linux/dma-mapping.h>
#ifdef CONFIG_ARM64
#include <linux/syscore_ops.h>
#endif

#define PROC_NODE_ROOT_NAME	"ax_proc/slp_stat"
#define SLP_STAT 	"state"
#define TIMESTAMP_ROOT_NAME		"ax_proc/timestamp"
#define TIMESTAMP_STATE	"state"

#define AX_SLEEP_IOC_MAGIC	'u'
#define CMD_ENTER_SLEEP_TIME	_IOWR(AX_SLEEP_IOC_MAGIC, 1, unsigned long*)
#define CMD_ENTER_WAKRUP_TIME	_IOWR(AX_SLEEP_IOC_MAGIC, 2, unsigned long*)
#define CMD_CLR_SLEEP_WAKRUP_TIME	_IO(AX_SLEEP_IOC_MAGIC, 3)

#define AX_TIMESTAMP_MAGIC	't'
#define CMD_GET_TIMESTAMP_CONFIG	_IOWR(AX_TIMESTAMP_MAGIC, 1, timestamp_header_t*)
#define CMD_GET_TIMESTAMP_ARR_ADDR	_IOWR(AX_TIMESTAMP_MAGIC, 2, unsigned long*)

#define TIMER64_ADDR_LOW_ADDR	(0x2250000)
#ifdef CONFIG_ARM64
#define TIMESTAMP_START_STORE_IRAM_ADDR		0x4100
#endif
#define PERSISTENT_TIMER_FREQ (24000000)
#define MAX_SUBID 10
#define MAX_STAMP_ARRAY_NUM 128
#ifdef CONFIG_PM_SLEEP
#define SLEEP_STAGE_STORE_ADDR	0x4200
static void __iomem *slp_failed_addr = NULL;
extern unsigned long k_suspend_wakeup_times;
extern unsigned long k_suspend_wakeup_max_duration;
extern unsigned long k_suspend_wakeup_min_duration;
extern unsigned long k_suspend_wakeup_average_duration;
extern unsigned long k_suspend_wakeup_total_duration;
extern unsigned long k_suspend_wakeup_duration;
static struct proc_dir_entry *slp_stat_root;
static struct proc_dir_entry *timestamp_root;
static unsigned long user_sleep_wake_times = 0;
static unsigned long user_sleep_time = 0;
static unsigned long user_wakeup_time = 0;
static unsigned long user_last_duration = 0;
static unsigned long user_max_duration = 0;
static unsigned long user_min_duration = ((unsigned long)-1);
static unsigned long user_total_duration = 0;
static unsigned long user_average_duration = 0;
static void __iomem *timer64_addr_low = NULL;
#ifdef CONFIG_ARM64
static void __iomem *timestamp_atf_arr_iram_addr = NULL;
#endif
static void *timestamp_virt_addr = NULL;
static dma_addr_t timestamp_phys_addr;
static int timestamp_number = 0;
static u8 valid_mod_id[] = {7,8,9,17,18,37,46};
static DEFINE_MUTEX(ax_slp_stat_mutex);
typedef struct {
	u64 wakeup_tmr64_cnt;
	u64 timestamp_phys_addr;
	/*pingpong buffer used index*/
	u32 store_index;
	u32 max_number;
} timestamp_header_t;
typedef struct {
	u32 data[ARRAY_SIZE(valid_mod_id)][MAX_SUBID];
}timestamp_data_t;
#ifdef CONFIG_ARM64
typedef struct {
	u64 wakeup_tmr64_cnt;
	u32 store_index;
} timestamp_atf_header_t;
typedef struct {
	u32 data[MAX_SUBID];
}timestamp_atf_data_t;
#endif

enum {
	AX_SLEEP_STAGE_00 = 0xAF00,
	AX_SLEEP_STAGE_01,
	AX_SLEEP_STAGE_02,
	AX_SLEEP_STAGE_03,
	AX_SLEEP_STAGE_04,
	AX_SLEEP_STAGE_05,
	AX_SLEEP_STAGE_06,
	AX_SLEEP_STAGE_07,
	AX_SLEEP_STAGE_08,
	AX_SLEEP_STAGE_09,
	AX_SLEEP_STAGE_0A,
	AX_SLEEP_STAGE_0B,
	AX_WAKEUP_STAGE_0C,
	AX_WAKEUP_STAGE_0D,
	AX_WAKEUP_STAGE_0E,
	AX_WAKEUP_STAGE_0F,
	AX_WAKEUP_STAGE_10,
	AX_WAKEUP_STAGE_11,
	AX_WAKEUP_STAGE_12,
	AX_WAKEUP_STAGE_13,
	AX_WAKEUP_STAGE_14,
	AX_WAKEUP_STAGE_15,
	AX_WAKEUP_STAGE_16,
	AX_WAKEUP_STAGE_17,
	AX_WAKEUP_STAGE_18,
	AX_WAKEUP_STAGE_19,
	AX_WAKEUP_STAGE_1A,
	AX_WAKEUP_STAGE_1B,
	AX_WAKEUP_STAGE_1C,
	AX_WAKEUP_STAGE_1D,
	AX_WAKEUP_STAGE_1E,
	AX_WAKEUP_STAGE_1F,
	AX_KERNEL_SLEEP_STAGE_00 = 0xFF00,
	AX_KERNEL_SLEEP_STAGE_01,
	AX_KERNEL_SLEEP_STAGE_02,
	AX_KERNEL_SLEEP_STAGE_03,
	AX_KERNEL_SLEEP_STAGE_04,
	AX_KERNEL_SLEEP_STAGE_05,
	AX_KERNEL_SLEEP_STAGE_06,
	AX_KERNEL_SLEEP_STAGE_07,
	AX_KERNEL_SLEEP_STAGE_08,
	AX_KERNEL_SLEEP_STAGE_09,
	AX_KERNEL_SLEEP_STAGE_0A,
	AX_KERNEL_SLEEP_STAGE_0B,
	AX_KERNEL_SLEEP_STAGE_0C,
	AX_KERNEL_SLEEP_STAGE_0D,
	AX_KERNEL_WAKEUP_STAGE_0E,
	AX_KERNEL_WAKEUP_STAGE_0F,
	AX_KERNEL_WAKEUP_STAGE_10,
	AX_KERNEL_WAKEUP_STAGE_11,
	AX_KERNEL_WAKEUP_STAGE_12,
	AX_KERNEL_WAKEUP_STAGE_13,
	AX_KERNEL_WAKEUP_STAGE_14,
	AX_KERNEL_WAKEUP_STAGE_15,
	AX_KERNEL_WAKEUP_STAGE_16,
	AX_KERNEL_WAKEUP_STAGE_17,
	AX_KERNEL_WAKEUP_STAGE_18,
	AX_KERNEL_WAKEUP_STAGE_19,
	AX_KERNEL_WAKEUP_STAGE_1A,
	AX_KERNEL_WAKEUP_STAGE_1B,
	AX_KERNEL_WAKEUP_STAGE_1C,
	AX_KERNEL_WAKEUP_STAGE_1D,
	AX_KERNEL_WAKEUP_STAGE_1E,
	AX_KERNEL_WAKEUP_STAGE_1F,
};

static u64 read_tmr64_val(void)
{
	if(timer64_addr_low)
		return *((unsigned long long*)timer64_addr_low);
	return 0;
}
static int find_valid_mod_id(int modid)
{
	int i;
	for(i = 0; i < ARRAY_SIZE(valid_mod_id); i++) {
		if(valid_mod_id[i] == modid)
			return i;
	}
	return -1;
}
static void store_timestamp(int modid, int subid, timestamp_data_t *data)
{
	int idx;
	u64 val;
	timestamp_header_t *header;
	header = timestamp_virt_addr;
	idx = find_valid_mod_id(modid);
	if((idx < 0))
		return;
	val = read_tmr64_val();
	/*only save low 32 bit*/
	data->data[idx][subid] = (val - header->wakeup_tmr64_cnt) & 0xffffffff;
}
unsigned int ax_sys_sleeptimestamp(int modid, unsigned int subid)
{
	timestamp_header_t *header;
	timestamp_data_t *data;
	if (!timestamp_virt_addr || !timer64_addr_low || (subid >= MAX_SUBID))
		return -1;
	header = timestamp_virt_addr;
	if((modid == AX_ID_KERNEL) && (subid == AX_SUB_ID_RESUME_START)) {
#ifndef CONFIG_ARM64
		header->wakeup_tmr64_cnt = read_tmr64_val();
		header->store_index++;
		header->store_index %= timestamp_number;
		data = (timestamp_data_t *)((void *)timestamp_virt_addr + sizeof(timestamp_header_t));
		memset(&data[header->store_index], 0, sizeof(timestamp_data_t));
#else
		pr_err("arm64 wakeup start point at atf.\r\n");
		return -1;
#endif
	}
	data = (timestamp_data_t *)((void *)timestamp_virt_addr + sizeof(timestamp_header_t));
	store_timestamp(modid, subid, &data[header->store_index]);
	return 0;
}

EXPORT_SYMBOL(ax_sys_sleeptimestamp);

void ax_sys_sleeptimestamp_print(void)
{
}

EXPORT_SYMBOL(ax_sys_sleeptimestamp_print);
static int __timestamp_show(struct seq_file *m, timestamp_data_t *data)
{
	u32 i, j;
	unsigned int modId = 0;

	for(i = 0; i < ARRAY_SIZE(valid_mod_id); i++) {
		modId = valid_mod_id[i];
		for (j = 0; j < MAX_SUBID; j ++) {
			if (data->data[i][j] != 0x0) {
				seq_printf(m, "%d\t%d\t%u\t\r\n", modId, j, data->data[i][j] / 24);
			}
		}
	}
	return 0;
}

static int timestamp_show(struct seq_file *m, void *v)
{
	u32 i;
	timestamp_data_t *data;
	timestamp_header_t *header;
	if(!timestamp_virt_addr) {
		seq_printf(m, "not enable, please echo value > /proc/ax_proc/timestamp/state \n");
		return 0;
	}

	header = timestamp_virt_addr;
	data = (timestamp_data_t *)((void *)timestamp_virt_addr + sizeof(timestamp_header_t));
	seq_printf(m, "ModId\tSubId\tTimeStamp(us)\n");
	for(i = (header->store_index + 1); i < (header->store_index + timestamp_number); i++) {
		seq_printf(m, "-------------------------------\n");
		__timestamp_show(m, &data[i % timestamp_number]);
	}
	seq_printf(m, "\n\n");
	return 0;
}

static int timestamp_open(struct inode *inode, struct file *file)
{
	return single_open(file, timestamp_show, NULL);
}

static long timestamp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch(cmd) {
	case CMD_GET_TIMESTAMP_CONFIG:
		if(!timestamp_virt_addr || copy_to_user((void*)arg, timestamp_virt_addr, sizeof(timestamp_header_t)))
			return -1;
		break;
	case CMD_GET_TIMESTAMP_ARR_ADDR:
		if(copy_to_user((void*)arg, &timestamp_phys_addr, sizeof(unsigned long)))
			return -1;
		break;
	default:
		return -1;
	}

	return 0;
}

static ssize_t timestamp_write(struct file *file , const char __user *buf, size_t count, loff_t *pos)
{
	int ret;
	char kbuf[8] = { 0 };
	int size;
	timestamp_header_t *header;
	if ((count > 4) || (timestamp_virt_addr))
		return -1;

	if (copy_from_user(kbuf, buf, count))
		return -EFAULT;
	ret = kstrtoint(kbuf, 10, &timestamp_number);
	if (ret)
		return ret;
	timestamp_number++;
	if(timestamp_number > MAX_STAMP_ARRAY_NUM) {
		timestamp_number = MAX_STAMP_ARRAY_NUM;
	}
	size = timestamp_number * sizeof(timestamp_data_t) + sizeof(timestamp_header_t);
	timestamp_virt_addr = kzalloc(size, GFP_KERNEL);
	if(!timestamp_virt_addr) {
		pr_err("malloc failed\n");
		return 0;
	}
	header = timestamp_virt_addr;
	header->timestamp_phys_addr = virt_to_phys(timestamp_virt_addr);
	header->max_number = timestamp_number;
#ifdef CONFIG_ARM64
	size = sizeof(timestamp_atf_data_t) + sizeof(timestamp_atf_header_t);
	timestamp_atf_arr_iram_addr = ioremap_nocache(TIMESTAMP_START_STORE_IRAM_ADDR, size);
	if (!timestamp_atf_arr_iram_addr) {
		pr_err("malloc atf arr iram addr failed\n");
		return 0;
	}
#endif
	return count;
}

static const struct file_operations timestamp_fsops = {
	.open = timestamp_open,
	.read = seq_read,
	.write = timestamp_write,
	.unlocked_ioctl = timestamp_ioctl,
	.release = single_release,
};

static int ax_slp_stat_show(struct seq_file *m, void *v)
{
	mutex_lock(&ax_slp_stat_mutex);
	seq_printf(m, "\t\t------ User Space Sleep Wakeup Time Statistics ------\n\n");
	seq_printf(m, "Total Times\tMax Duration\tMin Duration\tAverage Duration\tLast Duration\n");
	seq_printf(m,"%lu\t\t%lu ms\t\t%lu ms\t\t%lu ms\t\t\t%lu ms\n", user_sleep_wake_times, user_max_duration,
		user_min_duration, user_average_duration, user_last_duration);
	seq_printf(m, "\t\t------ Kernel Space Sleep Wakeup Time Statistics ------\n\n");
	seq_printf(m, "Total Times\tMax Duration\tMin Duration\tAverage Duration\tLast Duration\n");
	seq_printf(m,"%lu\t\t%lu ms\t\t%lu ms\t\t%lu ms\t\t\t%lu ms\n", k_suspend_wakeup_times, k_suspend_wakeup_max_duration,
		k_suspend_wakeup_min_duration, k_suspend_wakeup_average_duration, k_suspend_wakeup_duration);
	seq_printf(m, "\n\n");
	mutex_unlock(&ax_slp_stat_mutex);
	return 0;
}

static int ax_slp_stat_open(struct inode *inode, struct file *file)
{
	return single_open(file, ax_slp_stat_show, NULL);
}

static long ax_slp_stat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned long k_arg;

	mutex_lock(&ax_slp_stat_mutex);
	switch(cmd) {
	case CMD_ENTER_SLEEP_TIME:
		if(copy_from_user(&k_arg, (void*)arg, sizeof(k_arg))) {
			mutex_unlock(&ax_slp_stat_mutex);
			return -1;
		}
		user_sleep_time = k_arg;
		if (user_sleep_wake_times > 0) {
			user_last_duration = (user_sleep_time - user_wakeup_time);
			if ((user_sleep_time - user_wakeup_time) > user_max_duration)
				user_max_duration = (user_sleep_time - user_wakeup_time);
			if ((user_sleep_time - user_wakeup_time) < user_min_duration) {
				user_min_duration = (user_sleep_time - user_wakeup_time);
			}
			user_total_duration += (user_sleep_time - user_wakeup_time);
			user_average_duration = (user_total_duration / user_sleep_wake_times);
		}
		user_sleep_wake_times++;
		break;
	case CMD_ENTER_WAKRUP_TIME:
		if(copy_from_user(&k_arg, (void*)arg, sizeof(k_arg))) {
			mutex_unlock(&ax_slp_stat_mutex);
			return -1;
		}
		user_wakeup_time = k_arg;
		break;
	case CMD_CLR_SLEEP_WAKRUP_TIME:
		user_sleep_wake_times = 0;
		user_max_duration = 0;
		user_min_duration = ((unsigned long)-1);
		user_total_duration = 0;
		user_average_duration = 0;
		k_suspend_wakeup_times = 0;
		k_suspend_wakeup_max_duration = 0;
		k_suspend_wakeup_min_duration = ((unsigned long)-1);
		k_suspend_wakeup_total_duration = 0;
		k_suspend_wakeup_average_duration = 0;
		break;
	default:
		mutex_unlock(&ax_slp_stat_mutex);
		return -1;
	}

	mutex_unlock(&ax_slp_stat_mutex);
	return 0;
}

static const struct file_operations ax_slp_stat_fsops = {
	.open = ax_slp_stat_open,
	.read = seq_read,
	.unlocked_ioctl = ax_slp_stat_ioctl,
	.release = single_release,
};
#ifndef CONFIG_ARM64
#include <asm/mach/time.h>
extern int register_persistent_clock(clock_access_fn read_persistent);
#endif

static struct timespec64 persistent_ts;
static unsigned int persistent_mult, persistent_shift;
static void ax_read_persistent_clock64(struct timespec64 *ts)
{
	u64 nsecs;
	u64 last_cycles;
	static u64 cycles;
	last_cycles = cycles;
	cycles = read_tmr64_val();

	nsecs = clocksource_cyc2ns(cycles - last_cycles, persistent_mult, persistent_shift);
	timespec64_add_ns(&persistent_ts, nsecs);

	*ts = persistent_ts;
}
static int ax_persistent_clocksource_init(void)
{
	clocks_calc_mult_shift(&persistent_mult, &persistent_shift,
		PERSISTENT_TIMER_FREQ, NSEC_PER_SEC, 120000);
#ifndef CONFIG_ARM64
	register_persistent_clock(ax_read_persistent_clock64);
#endif
	return 0;
}
#ifdef CONFIG_ARM64
void read_persistent_clock64(struct timespec64 *ts)
{
	ax_read_persistent_clock64(ts);
}
#endif
#ifdef CONFIG_ARM64
static struct syscore_ops slp_stat_pm_syscore_ops;
static int axera_slp_stat_syscore_suspend(void)
{
	return 0;
}

static void axera_slp_stat_syscore_resume(void)
{
	timestamp_header_t *header;
	timestamp_atf_header_t *atf_header;
	timestamp_data_t *data;
	timestamp_atf_data_t *atf_data;
	u32 index = 0;
	u32 data_index = 0;

	if (!timestamp_virt_addr || !timestamp_atf_arr_iram_addr)
		return;
	/* copy atf timestamp point to kernel */
	header = timestamp_virt_addr;
	atf_header = timestamp_atf_arr_iram_addr;
	header->store_index++;
	header->store_index %= timestamp_number;
	header->wakeup_tmr64_cnt = atf_header->wakeup_tmr64_cnt;
	data = (timestamp_data_t *)((void *)timestamp_virt_addr + sizeof(timestamp_header_t));
	atf_data = (timestamp_atf_data_t*)((void*)timestamp_atf_arr_iram_addr + sizeof(timestamp_atf_header_t));
	for(index = 0; index < MAX_SUBID; index++) {
		if (atf_data->data[index] != 0) {
			data_index = header->store_index ? header->store_index : timestamp_number;
			data[data_index - 1].data[find_valid_mod_id(AX_ID_KERNEL)][index] = atf_data->data[index];
		}
	}
	return;
}
#endif


static void print_sleep_record(void)
{
	unsigned int err_num = 0;
	err_num = readl(slp_failed_addr);
	pr_info("slp failed number: %d\r\n", err_num);
	writel(0, slp_failed_addr);
}

static int axera_slp_stat_probe(struct platform_device *pdev)
{
	slp_stat_root = proc_mkdir(PROC_NODE_ROOT_NAME, NULL);
	if (slp_stat_root == NULL)
		goto err0;

	proc_create_data(SLP_STAT, 0644, slp_stat_root,
			 &ax_slp_stat_fsops, NULL);

	timer64_addr_low = ioremap_nocache(TIMER64_ADDR_LOW_ADDR, 0x8);
	if (NULL == timer64_addr_low)
		goto err1;

	timestamp_root = proc_mkdir(TIMESTAMP_ROOT_NAME, NULL);
	if (timestamp_root == NULL)
		goto err2;

	proc_create_data(TIMESTAMP_STATE, 0644, timestamp_root,
			 &timestamp_fsops, NULL);

	ax_persistent_clocksource_init();
#ifdef CONFIG_ARM64
	slp_stat_pm_syscore_ops.suspend = axera_slp_stat_syscore_suspend;
	slp_stat_pm_syscore_ops.resume = axera_slp_stat_syscore_resume;
	register_syscore_ops(&slp_stat_pm_syscore_ops);
#endif

	slp_failed_addr = ioremap(SLEEP_STAGE_STORE_ADDR, 0x4);
	if (!slp_failed_addr)
		goto err2;

	print_sleep_record();

	printk("axera_slp_stat_probe ok\r\n");

	return 0;

err2:
	if (timer64_addr_low)
		iounmap(timer64_addr_low);
err1:
	remove_proc_entry(SLP_STAT, slp_stat_root);
	remove_proc_entry(PROC_NODE_ROOT_NAME, NULL);
err0:
	return -ENODATA;
}

static int axera_slp_stat_remove(struct platform_device *pdev)
{
	if (slp_stat_root) {
		remove_proc_entry(SLP_STAT, slp_stat_root);
		remove_proc_entry(PROC_NODE_ROOT_NAME, NULL);
	}

	if (timestamp_root) {
		remove_proc_entry(TIMESTAMP_STATE, timestamp_root);
		remove_proc_entry(TIMESTAMP_ROOT_NAME, NULL);
	}

	if (timestamp_virt_addr)
		kfree(timestamp_virt_addr);

	if (timer64_addr_low)
		iounmap(timer64_addr_low);

#ifdef CONFIG_ARM64
	if (timestamp_atf_arr_iram_addr)
		kfree(timestamp_atf_arr_iram_addr);
#endif

	if (slp_failed_addr)
		iounmap(slp_failed_addr);

	return 0;
}

static const struct of_device_id axera_slp_stat_of_id_table[] = {
	{ .compatible = "ax_slp_stat" },
	{}
};
MODULE_DEVICE_TABLE(of, axera_slp_stat_of_id_table);

static struct platform_driver axera_slp_stat_driver = {
	.probe	= axera_slp_stat_probe,
	.remove = axera_slp_stat_remove,
	.driver = {
		.name = KBUILD_MODNAME,
		.of_match_table = axera_slp_stat_of_id_table,
	},
};

module_platform_driver(axera_slp_stat_driver);
#else
unsigned int ax_sys_sleeptimestamp(int modid, unsigned int subid)
{
	return 0;
}
EXPORT_SYMBOL(ax_sys_sleeptimestamp);
#endif

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("axera slp stat driver");
