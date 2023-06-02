/*
 *   @file   vg_log.c
 *
 *   @brief  The Log System to record debug information.
 *
 *   Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
 */

#define _LOG_C_
#include <linux/module.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <linux/synclink.h>
#include <linux/miscdevice.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/proc_fs.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/workqueue.h>
#include <linux/kallsyms.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/time.h>
#include <asm/io.h>
#include <asm/statfs.h>
#include <linux/uaccess.h>
#include <asm/delay.h>
#include <linux/thread_info.h>
#include <linux/slab.h>
#include <mach/fmem.h>
#include <comm/log.h>
#include <kwrap/type.h>
#include <kwrap/file.h>
#include "vg_log_pif.h"
#include "vg_log_core.h"
#include <kwrap/dev.h>

#define MODULE_NAME "log_vg"
#define MODULE_MINOR_COUNT 1

#if 0
#include "info.h"
#include "vg_memcpy.h"
#endif

#define BIT_BUSY_WRITE      0
#define BIT_PANIC           1
#define BIT_BUSY_DUMP_LOG   2
#define BIT_BUSY_DUMP_BUF   3
static unsigned long log_state[1] ={0}; //bit 0:idle/busy  bit 1:panic

#define FUNC_LOG        0
#define FUNC_DUMPBUF    1
static unsigned int write_function = FUNC_LOG;

static struct proc_dir_entry *videoproc, *debugproc, *modeproc, *dump2proc, *affinityproc;
static struct proc_dir_entry *delay1proc, *delay2proc, *gmlib_setting_proc, *gmlib_flow_proc;
static struct proc_dir_entry *gmlib_err_proc, *threadproc;
static struct proc_dir_entry *hdal_proc, *hdal_setting_proc, *hdal_flow_proc, *hdal_version_proc;


struct timer_list   log_timer;
struct task_struct  *write_task, *notify_task, *monitor_task;
static struct timespec log_mod_insert_time;
static int is_already_write = 0;

#define LOOP_DETECTION  0x1
#define CPUID_LOG       0x2
static unsigned int feature = 0; //0x1:loop detect   0x2:cpuID+ISR

module_param(log_ksize, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(log_ksize, "log_ksize");

module_param(feature, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(feature, "feature");

module_param(mode, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mode, "mode");


static char dump_path[MAX_PATH_WIDTH] = "/mnt/nfs";

#define LOG_VERSION "v1.4"

#define MAX_VERSION_LEN 10
static char version[MAX_VERSION_LEN] = "v0.4";


struct panic_func_t {
	int registered;
	int (*panic_notifier)(int);
};

struct printout_func_t {
	int registered;
	int (*printout_notifier)(int);
};

#define MAX_CB  20
struct panic_func_t     log_panic_cb[MAX_CB];
struct printout_func_t  log_printout_cb[MAX_CB];
struct printout_func_t log_master_print_cb;
struct printout_func_t log_hdal_proc_cb;
char hdal_cmd_buffer[64];

wait_queue_head_t log_wq;

#define LOG_NOTIFY_NONE     0
#define LOG_NOTIFY_START    1
#define LOG_NOTIFY_DONE     2
static int log_notify_state = LOG_NOTIFY_NONE; //1:start  2:done
static int panic_notify_state = LOG_NOTIFY_NONE; //1:start  2:done
unsigned int is_panic = 0;

void write_process(unsigned long data);

#if 0
static void *seq_gmlib_err_start(struct seq_file *s, loff_t *pos);
static void *seq_gmlib_flow_start(struct seq_file *s, loff_t *pos);
static void *seq_gmlib_setting_start(struct seq_file *s, loff_t *pos);
#endif
#if 0
static void *seq_hdal_setting_start(struct seq_file *s, loff_t *pos);
#endif
static void *seq_hdal_flow_start(struct seq_file *s, loff_t *pos);

void *seq_next(struct seq_file *s, void *v, loff_t *pos);
void seq_stop(struct seq_file *s, void *v);
int seq_show(struct seq_file *s, void *v);

#if 0
static struct seq_operations seq_gmlib_err_ops = {
	.start = seq_gmlib_err_start,
	.next  = seq_next,
	.stop  = seq_stop,
	.show  = seq_show
};
static struct seq_operations seq_gmlib_flow_ops = {
	.start = seq_gmlib_flow_start,
	.next  = seq_next,
	.stop  = seq_stop,
	.show  = seq_show
};
static struct seq_operations seq_gmlib_setting_ops = {
	.start = seq_gmlib_setting_start,
	.next  = seq_next,
	.stop  = seq_stop,
	.show  = seq_show
};
#endif

#if 0
static struct seq_operations seq_hdal_setting_ops = {
	.start = seq_hdal_setting_start,
	.next  = seq_next,
	.stop  = seq_stop,
	.show  = seq_show
};
#endif

static struct seq_operations seq_hdal_flow_ops = {
	.start = seq_hdal_flow_start,
	.next  = seq_next,
	.stop  = seq_stop,
	.show  = seq_show
};

//seq_iteration_read
#define MAX_ITERATION_PRINT_SIZE 4096
typedef struct {
	char *print_buf;
	unsigned int remain_size;
} proc_print_buf_t;



/**
 * move the iterator forward to the next position in the sequence
 */
void *seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	proc_print_buf_t *v_in;

	(*pos)++;

	v_in = (proc_print_buf_t *) v;
	if (v_in->remain_size >= MAX_ITERATION_PRINT_SIZE) { //next->show
		v_in->print_buf = v_in->print_buf + MAX_ITERATION_PRINT_SIZE;
		v_in->remain_size =  v_in->remain_size - MAX_ITERATION_PRINT_SIZE;
	} else { //last one
		v_in->print_buf = NULL;
		v_in->remain_size = 0;
		return NULL; //prepare to do end process
	}

	return (void *) v_in;
}

/**
 * stop() is called when iteration is complete (clean up)
 */
void seq_stop(struct seq_file *s, void *v)
{
	if (v) {
		kfree(v);
	}
}

/**
 * success return 0, otherwise return error code
 */
int seq_show(struct seq_file *s, void *v)
{
	proc_print_buf_t *v_in;

	v_in = (proc_print_buf_t *)v;
	if (v_in->remain_size >= MAX_ITERATION_PRINT_SIZE) {
		seq_write(s, (void *) v_in->print_buf, MAX_ITERATION_PRINT_SIZE);
	} else {
		seq_write(s, (void *) v_in->print_buf, v_in->remain_size);
	}

	return 0; //continue to print
}

//return only 0->1
int test_and_wait_bit(int bitmap, unsigned long *state)
{
	int timeout = 1000;
	while (test_and_set_bit(bitmap, state) && (timeout > 0)) { //while if bit is already set
		if (in_interrupt()) {
			udelay(10);
		} else {
			msleep(1);
		}
		timeout--;
	}
	if (timeout <= 0) {
		return 1;
	}
	return 0; //bit is 0, and successful to set the bit
}



unsigned int in_busyloop = 0;
#define PRINTM_PER_LOOPS 100
/* busyloop condiction:
    during 2s printm 160000 (under 800MIPS) ==> printm 100 loops/MIPS,second
 */
inline void busyloop_detection(char *module_char)
{
	static unsigned int start_jiffies = 0, end_jiffies = 0, printm_count = 0;
	unsigned int loops = (unsigned int)(loops_per_jiffy / (500000 / HZ));
	unsigned int diff_ms;

	if ((feature & LOOP_DETECTION) != 0x1) {
		return;
	}
	if (start_jiffies == 0) {
		start_jiffies = get_gm_jiffies();
		return;
	}
	end_jiffies = get_gm_jiffies();
	printm_count++;

	diff_ms = (int)end_jiffies - (int)start_jiffies;

#define DETECT_MS 5000
	if (diff_ms > DETECT_MS) {
		//printk("printm_count %u %u\n",printm_count, (loops * PRINTM_PER_LOOPS));
		if (printm_count > (loops * PRINTM_PER_LOOPS * (DETECT_MS / 1000))) {
			unsigned int start, size;
			in_busyloop = 1;
			printk("printm busyloop detected! (%ds count %u over %u at %uMIPS)\n",
				   DETECT_MS / 1000, printm_count, (loops * PRINTM_PER_LOOPS * (DETECT_MS / 1000)), loops);
			dump_stack();
			calculate_log(&start, &size);
			printk("==========================================================================\n");
			prepare_dump_console(start, size);
			dump_stack();
			panic("printm busyloop detected! (%ds count %u over %u at %uMIPS)\n",
				  DETECT_MS / 1000, printm_count, (loops * PRINTM_PER_LOOPS * (DETECT_MS / 1000)), loops);
		}
		start_jiffies = printm_count = 0;
	}
}

//
void master_print(const char *fmt, ...)
{
	int len = 0;
	va_list args;
	char log[MAX_CHAR];

	if (log_master_print_cb.registered == 0) {
		return;
	}

	va_start(args, fmt);
	len = vsnprintf(log, sizeof(log), fmt, args);
	va_end(args);
	log_master_print_cb.printout_notifier((int) &log[0]);
}

void wait_for_notify(void)
{
	if (write_function == FUNC_LOG) {
		printk("[LOG] Notifying...\n");
		if (is_panic) {
			panic_notify_state = LOG_NOTIFY_START;
		}
		log_notify_state = LOG_NOTIFY_START;
		wake_up_process(notify_task);
	}

	if (write_function == FUNC_LOG) {
		wait_event_timeout(log_wq, (panic_notify_state == LOG_NOTIFY_NONE) &&
						   (log_notify_state == LOG_NOTIFY_NONE), msecs_to_jiffies(1000));
		if ((panic_notify_state != LOG_NOTIFY_DONE) && (panic_notify_state != LOG_NOTIFY_NONE)) {
			printk("Error to wait panic notifier function return well (1000ms timeout)!\n");
			panic_notify_state = LOG_NOTIFY_NONE;
		}
		if ((log_notify_state != LOG_NOTIFY_DONE) && (log_notify_state != LOG_NOTIFY_NONE)) {
			printk("Error to wait notifier function return well (1000ms timeout)!\n");
			log_notify_state = LOG_NOTIFY_NONE;
		}
	}
}

void wake_up_log_write(void)
{
	sprintf(log_path[0], "%s/log.txt", dump_path);
	write_function = FUNC_LOG;

	printk("[LOG] Notifying...\n");
	if (is_panic) {
		panic_notify_state = LOG_NOTIFY_START;
	}
	log_notify_state = LOG_NOTIFY_START;
	wake_up_process(notify_task);
	wake_up_process(write_task);
}

void wake_up_file_write(unsigned int va, dump_info_t *dump_info, unsigned int counts, char *path)
{
	unsigned int i, offset = 0;
	log_slice_ptr[0] = va;
	for (i = 0; i < counts; i++) {
		log_slice_ptr[i] = va + offset;
		log_size[i] = dump_info[i].size;
		sprintf(log_path[i], "%s/%s", path, (strlen(dump_info[i].filename) != 0) ? dump_info[i].filename : "buffer.dat");
		offset += dump_info[i].size;
	}
	write_function = FUNC_DUMPBUF;
	wake_up_process(write_task);
}

void write_process(unsigned long data)
{
	int ret = 0, i;
	mm_segment_t fs;
	unsigned long long offset = 0;
	int filp[10];
	unsigned int start = 0, size = 0;

	if (write_function == FUNC_LOG) {
		wait_event_timeout(log_wq, (panic_notify_state == LOG_NOTIFY_NONE) &&
						   (log_notify_state == LOG_NOTIFY_NONE), msecs_to_jiffies(1000));
		if ((panic_notify_state != LOG_NOTIFY_DONE) && (panic_notify_state != LOG_NOTIFY_NONE)) {
			printk("Error to wait panic notifier function return well (1000ms timeout)!\n");
			panic_notify_state = LOG_NOTIFY_NONE;
		}
		if ((log_notify_state != LOG_NOTIFY_DONE) && (log_notify_state != LOG_NOTIFY_NONE)) {
			printk("Error to wait notifier function return well (1000ms timeout)!\n");
			log_notify_state = LOG_NOTIFY_NONE;
		}
		//calculate pointer
		calculate_log(&start, &size);
		if (mode == MODE_STORAGE) {
			prepare_dump_storage(start, size);
		} else {
			prepare_dump_console(start, size);
		}
		printk("[LOG] Dumping log %d bytes...\n", log_size[0]);
	} else { //FUNC_DUMPBUF
		for (i = 0; i < MAX_DUMP_FILE; i++) {
			if (log_size[i] == 0) {
				break;
			}
			printk("[LOG] Dumping buffer %d bytes... va(%#x)\n", log_size[i], log_slice_ptr[i]);
		}
	}
	if (test_and_set_bit(BIT_BUSY_WRITE, log_state)) { //fail
		printk("[LOG]already busy write process!\n");
		return;
	}
	printk("[LOG] Log Writing...\n");
	fs = get_fs();
	set_fs(KERNEL_DS);
	if ((log_slice_ptr[0] == 0) || (log_size[0] == 0) || (strlen(log_path[0]) == 0)) {
		printk("---NO Log Message---\n");
		goto returnit;
	}

	if (write_function == FUNC_LOG) {
		filp[0] = vos_file_open(log_path[0], O_WRONLY | O_CREAT, 0777);
		if (filp[0] == 0) {
			printk("Error to open %s\n", log_path[0]);
			goto returnit;
		}
		ret = vos_file_write(filp[0], (unsigned char *)log_slice_ptr[0], log_size[0]);
		if (log_size[1] && (ret > 0)) {
			ret = vos_file_write(filp[0], (unsigned char *)log_slice_ptr[1], log_size[1]);
		}
		vos_file_close(filp[0]);
	}
	if (write_function == FUNC_DUMPBUF) {
		for (i = 0; i < MAX_DUMP_FILE; i++) {
			if (log_size[i] == 0) {
				break;
			}
			filp[i] = vos_file_open(log_path[i], O_WRONLY | O_CREAT, 0777);
			if (filp[i] == 0) {
				printk("Error to open %s\n", log_path[i]);
				goto returnit;
			}
			offset = 0;
			ret = vos_file_write(filp[i], (unsigned char *)log_slice_ptr[i], log_size[i]);
			vos_file_close(filp[i]);
			if (ret < 0) {
				break;
			}
		}
	}
	if (write_function == FUNC_LOG) {
		printk("Write Log 0x%x(%d bytes) && 0x%x(0x%x) to %s\n",
			   log_slice_ptr[0], log_size[0], log_slice_ptr[1], log_size[1], log_path[0]);
	} else if (write_function == FUNC_DUMPBUF) {
		for (i = 0; i < MAX_DUMP_FILE; i++) {
			if (log_size[i] == 0) {
				break;
			}
			printk("Write Buffer 0x%x(%d bytes) to %s\n", log_slice_ptr[i], log_size[i], log_path[i]);
		}
	}
	set_fs(fs);

	if (ret) {
		printk("\n======================\n Write DONE!!!!!!\n======================\n");
	} else {
		printk("\n======================\n Write FAIL!!!!!!\n======================\n");
	}

returnit:
	clear_bit(BIT_BUSY_WRITE, log_state);
	clear_bit(BIT_BUSY_DUMP_BUF, log_state);
	clear_bit(BIT_BUSY_DUMP_LOG, log_state);
}

#if 0
static int write_thread(void *__cwq)
{
	set_current_state(TASK_INTERRUPTIBLE);
	set_user_nice(current, -20);
	while (!kthread_should_stop()) {
		schedule();
		__set_current_state(TASK_RUNNING);
		write_process(0);
		set_current_state(TASK_INTERRUPTIBLE);
	}
	__set_current_state(TASK_RUNNING);
	return 0;
}
#endif


#if 0
#define TIMEOUT_VALUE_MS    100
static int thread_nice = 10, timeout_threadhold = 20;
static int cpu_state = 0; //0:normal 1:busy 2:blocking
static int monitor_thread(void *__cwq)
{
	unsigned int start_jiffies, end_jiffies, over_counter = 0, current_diff;
	set_current_state(TASK_INTERRUPTIBLE);
	set_user_nice(current, thread_nice);

	while (!kthread_should_stop()) {
		start_jiffies = get_gm_jiffies();
		set_current_state(TASK_INTERRUPTIBLE);

		schedule_timeout(msecs_to_jiffies(TIMEOUT_VALUE_MS));
		__set_current_state(TASK_RUNNING);
		end_jiffies = get_gm_jiffies();

		current_diff = (int)end_jiffies - (int)start_jiffies;

		if (current_diff > TIMEOUT_VALUE_MS + (timeout_threadhold * 2)) {
			cpu_state = 2;
			over_counter++;
		} else if (current_diff > TIMEOUT_VALUE_MS + timeout_threadhold) {
			cpu_state = 1;
			over_counter++;
		} else if (over_counter) {
			over_counter--;
		}

		if (over_counter == 0) {
			cpu_state = 0;
		}

		//printk("diff=%d state %d counter %d\n",current_diff, cpu_state, over_counter);
	}
	__set_current_state(TASK_RUNNING);
	return 0;
}

int get_cpu_state(void)
{
	return cpu_state;
}
#endif

#if 0
static int notify_thread(void *__cwq)
{
	int i;
	set_current_state(TASK_INTERRUPTIBLE);
	set_user_nice(current, -20);
	while (!kthread_should_stop()) {
		if ((log_notify_state != LOG_NOTIFY_START) && (panic_notify_state != LOG_NOTIFY_START)) {
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(msecs_to_jiffies(100));
		}
		__set_current_state(TASK_RUNNING);

		if (panic_notify_state == LOG_NOTIFY_START) {
			for (i = 0; i < MAX_CB; i++) {
				if (log_panic_cb[i].registered) {
					print_symbol("[LOG] Damnit calling (%s)",
								 (int)log_panic_cb[i].panic_notifier);
					printk("  at 0x%x\n", (int)get_gm_jiffies() & 0xffff);
					log_panic_cb[i].panic_notifier(0);
				}
			}
			panic_notify_state = LOG_NOTIFY_DONE;
		}
		if (log_notify_state == LOG_NOTIFY_START) {
			for (i = 0; i < MAX_CB; i++) {
				if (log_printout_cb[i].registered) {
					print_symbol("[LOG] Log printout calling (%s)",
								 (int)log_printout_cb[i].printout_notifier);
					printk("  at 0x%x\n", (int)get_gm_jiffies() & 0xffff);
					log_printout_cb[i].printout_notifier(0);
				}
			}
			log_notify_state = LOG_NOTIFY_DONE;
		}
		set_current_state(TASK_INTERRUPTIBLE);
	}
	__set_current_state(TASK_RUNNING);
	return 0;
}
#endif

void register_version(char *ver)
{
	if (strlen(ver) < MAX_VERSION_LEN) {
		strcpy(version, ver);
	}
}

/*
 * @brief register the hdal proc notify function callback
 *
 * @function int register_hdal_proc_notifier(callback)
 * @param callback when it need to print message on Master console.
 * @return 1 on success, !0 on error
*/
int register_hdal_proc_notifier(int (*func)(int))
{
	if (log_hdal_proc_cb.registered == 1) {
		printk("Double regiter to master print notifier(%p, %p)!\n",
			   func, log_hdal_proc_cb.printout_notifier);
		return -1;
	}
	log_hdal_proc_cb.registered = 1;
	log_hdal_proc_cb.printout_notifier = func;
	return 1;
}

/*
 * @brief register the master print function callback
 *
 * @function int register_master_print_notifier(callback)
 * @param callback when it need to print message on Master console.
 * @return 1 on success, !0 on error
*/
int register_master_print_notifier(int (*func)(int))
{
	if (log_master_print_cb.registered == 1) {
		printk("Double regiter to master print notifier(%p, %p)!\n",
			   func, log_master_print_cb.printout_notifier);
		return -1;
	}
	log_master_print_cb.registered = 1;
	log_master_print_cb.printout_notifier = func;
	return 1;
}

/*
 * @brief register the panic function callback
  *
 * @function int register_panic_notifier(callback)
 * @param callback when panic occured callback will be triggered
 * @return 1 on success, !0 on error
*/
int register_panic_notifier(int (*func)(int))
{
	int i;
	for (i = 0; i < MAX_CB; i++) {
		if (log_panic_cb[i].registered == 0) {
			log_panic_cb[i].registered = 1;
			log_panic_cb[i].panic_notifier = func;
			break;
		}
	}
	return 1;
}

/*
 * @brief unregister the panic function callback
  *
 * @function int unregister_panic_notifier(callback)
 * @param callback when panic occured callback will be triggered
 * @return 1 on success, !0 on error
*/
int unregister_panic_notifier(int (*func)(int))
{
	int i;
	for (i = 0; i < MAX_CB; i++) {
		if (log_panic_cb[i].panic_notifier == func) {
			log_panic_cb[i].registered = 0;
			log_panic_cb[i].panic_notifier = NULL;
			break;
		}
	}
	return 1;
}

/*
 * @brief register the printout function callback
  *
 * @function int register_printout_notifier(callback)
 * @param callback when printout occured callback will be triggered
 * @return 1 on success, !0 on error
*/
int register_printout_notifier(int (*func)(int))
{
	int i;
	for (i = 0; i < MAX_CB; i++) {
		if (log_printout_cb[i].registered == 0) {
			log_printout_cb[i].registered = 1;
			log_printout_cb[i].printout_notifier = func;
			break;
		}
	}
	return 1;
}

/*
 * @brief unregister the printout function callback
  *
 * @function int unregister_printout_notifier(callback)
 * @param callback when printout occured callback will be triggered
 * @return 1 on success, !0 on error
*/
int unregister_printout_notifier(int (*func)(int))
{
	int i;
	for (i = 0; i < MAX_CB; i++) {
		if (log_printout_cb[i].printout_notifier == func) {
			log_printout_cb[i].registered = 0;
			log_printout_cb[i].printout_notifier = NULL;
			break;
		}
	}
	return 1;
}

/*
 * @brief dump log without damnit
 *
 * @function int dumplog(char *module)
 * @param module two characters module name
 * @return 1 on success, !0 on error
*/
int dumplog(char *module)
{
	struct timespec ts;

	if (is_already_write == 1) {
		printk("[LOG] Log exists and skip dumplog.\n");
		return 0;
	}

	getnstimeofday(&ts);
	printm("LG", "LOG1: system start H/M/S(%.2lu:%.2lu:%.2lu), log at H/M/S(%.2lu:%.2lu:%.2lu)\n",
		   (log_mod_insert_time.tv_sec / 3600) % (24), (log_mod_insert_time.tv_sec / 60) % (60),
		   log_mod_insert_time.tv_sec % 60, (ts.tv_sec / 3600) % (24), (ts.tv_sec / 60) % (60),
		   ts.tv_sec % 60);
	dump_stack();

	if (test_and_set_bit(BIT_BUSY_DUMP_LOG, log_state)) { //fail
		printk("[LOG]dump buffer already busy write1!\n");
		return 0;
	}
	printk("\nlog1 %s (%s %s) log...\n", LOG_VERSION, __DATE__, __TIME__);
	wake_up_log_write();
	is_already_write = 1;
	return 0;
}

/* This macro allows us to keep printk typechecking */
static __printf(1, 2)
void __klog_check_printsym_format(const char *fmt, ...)
{
}

static void __klog_print_symbol(const char *fmt, unsigned long address)
{
	char buffer[KSYM_SYMBOL_LEN];

	sprint_symbol(buffer, address);

	printk(fmt, buffer);
}
static inline void klog_print_symbol(const char *fmt, unsigned long addr)
{
	__klog_check_printsym_format(fmt, "");
	__klog_print_symbol(fmt, (unsigned long)
		       __builtin_extract_return_addr((void *)addr));
}


/*
 * @brief enter damnit to notify panic
 *
 * @function int damnit(char *module)
 * @param module two characters module name
 * @return 1 on success, !0 on error
*/
int damnit(char *module)
{
	struct timespec ts;
	unsigned long pfun = (unsigned int)__builtin_return_address(0);

	if (is_already_write == 1) {
		printk("[LOG] Log exists and skip dumplog.\n");
		return 0;
	}
	printm(module, "###Error### damnit videograph v%s (%s %s) from 0x%x\n", version, __DATE__,
		   __TIME__, pfun);

	getnstimeofday(&ts);
	printm("LG", "LOG2: system start H/M/S(%.2lu:%.2lu:%.2lu), log at H/M/S(%.2lu:%.2lu:%.2lu)\n",
		   (log_mod_insert_time.tv_sec / 3600) % (24), (log_mod_insert_time.tv_sec / 60) % (60),
		   log_mod_insert_time.tv_sec % 60, (ts.tv_sec / 3600) % (24), (ts.tv_sec / 60) % (60),
		   ts.tv_sec % 60);

	if (test_and_set_bit(BIT_PANIC, log_state)) {
		klog_print_symbol("Damnit from (%s), PANIC already\n", pfun);
		return 0;
	} else if (test_and_set_bit(BIT_BUSY_DUMP_LOG, log_state) ||
			   test_bit(BIT_BUSY_DUMP_BUF, log_state)) {
		klog_print_symbol("Damnit from (%s), BUSY already\n", pfun);
		return 0;
	} else {
		klog_print_symbol("Damnit from (%s)\n", pfun);
	}
	dump_stack();
	is_panic = 1;
	wake_up_log_write();
	is_already_write = 1;
	return 1;
}

#if 0
void dumpbuf_pa(dump_info_t *dump_info, unsigned int counts, char *path)
{
	unsigned int total_size = 0, i, offset = 0;
	if (test_and_set_bit(BIT_BUSY_DUMP_BUF, log_state)) {
		printk("[LOG]dump buffer already busy write!\n");
		return;
	}
	for (i = 0; i < counts; i++) {
		total_size += dump_info[i].size;
	}
	if (log_bsize < (total_size)) {
		printk("Error log_ksize=%dK, need %dK is enough\n", log_bsize / 1024, total_size / 1024);
		return;
	}
	fmem_dcache_sync((void *)log_base_start, total_size, DMA_TO_DEVICE);
	for (i = 0; i < counts; i++) {
		int ret;
		dump_info[i].size = ((dump_info[i].size + 3) >> 2) << 2;
		ret = vg_memcpy(log_base_ddr, __pa(log_base_start + offset), dump_info[i].ddr_id, dump_info[i].pa, dump_info[i].size);
		if (ret < 0) {
			printk("Error to do vg_memcpy return value %d\n", ret);
		}
		offset += dump_info[i].size;
	}
	fmem_dcache_sync((void *)log_base_start, total_size, DMA_FROM_DEVICE);
	wake_up_file_write(log_base_start, dump_info, counts, path);
}

void dumpbuf_va(int ddr_id, unsigned int va, unsigned int size, char *filename, char *path)
{
	dump_info_t dump_info;
	if (test_and_set_bit(BIT_BUSY_DUMP_BUF, log_state)) {
		printk("[LOG]dump buffer already busy write!\n");
		return;
	}
	dump_info.ddr_id = ddr_id;
	dump_info.pa = __pa(va);
	dump_info.size = size;
	sprintf(dump_info.filename, "%s", filename);
	wake_up_file_write(va, &dump_info, 1, path);
}

static int proc_affinity_seq_show(struct seq_file *s, void *v)
{
	seq_printf(s, "\necho irq_number cpu_id(0,1,2...) > affinity\n");
	return 0;
}

#include <linux/kallsyms.h>
typedef int (*irq_set_affinity_ptr)(unsigned int irq, const struct cpumask *mask);
static int proc_affinity_write_mode(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	char ker_buffer[64];
	int irq_num = 0, cpu_num = 0;
	irq_set_affinity_ptr irq_set_affinity = (irq_set_affinity_ptr)kallsyms_lookup_name("__irq_set_affinity");

	if (count > sizeof(ker_buffer)) {
		printk("count %d > max %d\n", count, sizeof(ker_buffer));
		return -EINVAL;
	}

	if (copy_from_user((void *)ker_buffer, buffer, count)) {
		return -EINVAL;
	}

	sscanf(ker_buffer, "%d %d\n", &irq_num, &cpu_num);
	printk("Set IRQ %d to CPU%d\n", irq_num, cpu_num);

	if (irq_set_affinity) {
		printk("Set IRQ %d to CPU%d\n", irq_num, cpu_num);
     	irq_set_affinity(irq_num, get_cpu_mask(cpu_num));
	}
	return count;
}
#endif

static int proc_mode_seq_show(struct seq_file *s, void *v)
{
	seq_printf(s, "\nDebug Mode %d (0:storage  1:dump console  2:direct print)\n", mode);
	return 0;
}

static int proc_mode_write_mode(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	char ker_buffer[64];
	int mode_set = 0;

	if (count > sizeof(ker_buffer)) {
		printk("count %d > max %d\n", count, sizeof(ker_buffer));
		return -EINVAL;
	}

	if (copy_from_user((void *)ker_buffer, buffer, count)) {
		return -EINVAL;
	}

	sscanf(ker_buffer, "%d\n", &mode_set);
	printk("Set to Mode %d (-1:disable 0:storage  1:dump console  2:direct print(forever)\n", mode_set);
	mode = mode_set;

	return count;
}

static int proc_dumplog_seq_show(struct seq_file *s, void *v)
{
	unsigned int start, size;
	calculate_log(&start, &size);
	prepare_dump_console(start, size);
#if 0
	struct timespec ts;

	getnstimeofday(&ts);
	printm("LG", "LOG3: system start H/M/S(%.2lu:%.2lu:%.2lu), log at H/M/S(%.2lu:%.2lu:%.2lu)\n",
		   (log_mod_insert_time.tv_sec / 3600) % (24), (log_mod_insert_time.tv_sec / 60) % (60),
		   log_mod_insert_time.tv_sec % 60, (ts.tv_sec / 3600) % (24), (ts.tv_sec / 60) % (60),
		   ts.tv_sec % 60);
	if (test_and_set_bit(BIT_BUSY_DUMP_LOG, log_state)) { //fail
		seq_printf(s, "[LOG]dump buffer already busy write1!\n");
		return 0;
	}
	printk("\nlog3 %s (%s %s) log...\n", LOG_VERSION, __DATE__, __TIME__);
	wake_up_log_write();
#endif
	return 0;
}


static int proc_dumplog_write_mode(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	if (count > MAX_PATH_WIDTH) {
		printk("Over path size %d\n", MAX_PATH_WIDTH);
		return 0;
	}
	if (copy_from_user(dump_path, buffer, count)) {
		return 0;
	}
	if (count < sizeof(dump_path)) {
		if (count == 0) {
			dump_path[0] = '\0';
		} else {
			dump_path[count - 1] = '\0';
		}
	}
	return count;
}

#if 0
static int proc_delay_non_preemptive_seq_show(struct seq_file *s, void *v)
{
	seq_printf(s, "Active a delay with non-preemptive:\n    #echo [delay msec] > delay_non_preemptive\n");
	return 0;
}


static spinlock_t delay_lock;
static int proc_delay1_write_mode(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	char ker_buffer[64];
	unsigned int delay;
	unsigned int start_jiffies = jiffies, end_jiffies = 0;

	if (count > sizeof(ker_buffer)) {
		printk("count %d > max %d\n", count, sizeof(ker_buffer));
		return -EINVAL;
	}

	if (copy_from_user((void *)ker_buffer, buffer, count)) {
		return -EINVAL;
	}

	sscanf(ker_buffer, "%d", &delay);
	spin_lock(&delay_lock); //preemptive disable

	end_jiffies = start_jiffies + msecs_to_jiffies(delay);
	while (jiffies != end_jiffies) //busy loop
		;
	spin_unlock(&delay_lock);
	return count;
}

static int proc_delay_irq_disabled_seq_show(struct seq_file *s, void *v)
{
	seq_printf(s, "Active a delay without ISR:\n    #echo [delay msec] > delay_irq_disabled\n");
	return 0;
}


static int proc_thread_write_mode(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	char ker_buffer[64];

	if (count > sizeof(ker_buffer)) {
		printk("count %d > max %d\n", count, sizeof(ker_buffer));
		return -EINVAL;
	}

	if (copy_from_user((void *)ker_buffer, buffer, count)) {
		return -EINVAL;
	}

	sscanf(ker_buffer, "%d", &timeout_threadhold);
	printk("timeout_threadhold=%d\n", timeout_threadhold);
	return count;
}
#endif

#if 0
static int proc_thread_realtime_test_seq_show(struct seq_file *s, void *v)
{
	seq_printf(s, "CPU:%d threshold:%d\n", cpu_state, timeout_threadhold);
	return 0;
}
#endif

#if 0
/* <----- setting message ----> <len(4bytes) - flow message>  */
static void *seq_gmlib_setting_start(struct seq_file *s, loff_t *pos)
{
	proc_print_buf_t *v;
	unsigned int start_ptr;
	unsigned int total_print_size;
	unsigned int offset;
	static char *msg = NULL;
	static int msg_len = 0;

	if (*pos == 0) {
		char *gmlib_setting_msg;
		unsigned int gmlib_setting_len;

		if (!mmap_msg) {
			if (msg) {
				kfree(msg);
				msg = NULL;
			}
			return NULL;
		}

		gmlib_setting_msg = (char *) mmap_msg;
		gmlib_setting_len = strlen((char *)mmap_msg);
		msg_len = gmlib_setting_len;
		msg = kmalloc(msg_len, GFP_KERNEL);
		memcpy((void *) msg, (void *) gmlib_setting_msg, msg_len);
	}

	start_ptr = (unsigned int) msg;  //fill start print pointer
	total_print_size = msg_len;      //fill print size

	offset = (*pos) * MAX_ITERATION_PRINT_SIZE;

	if (offset >= total_print_size) {
		if (msg) {
			kfree(msg);
			msg = NULL;
		}
		return NULL; //end of print
	}

	v = (proc_print_buf_t *) kzalloc(sizeof(proc_print_buf_t), GFP_KERNEL);
	if (!v) {
		if (msg) {
			kfree(msg);
			msg = NULL;
		}
		return NULL; //end of print
	}

	v->print_buf = (char *)(start_ptr + offset);
	v->remain_size = total_print_size - offset;

	return v;
}

static void *seq_gmlib_flow_start(struct seq_file *s, loff_t *pos)
{
	proc_print_buf_t *v;
	unsigned int start_ptr;
	unsigned int total_print_size;
	unsigned int offset;
	static char *msg = NULL;
	static int msg_len = 0;
	char *flow_msg;
	unsigned int flow_len;
	unsigned int flow_offset;

	if (*pos == 0) {
		if (!mmap_msg) {
			if (msg) {
				kfree(msg);
				msg = NULL;
			}
			return NULL;
		}

		flow_msg = (char *) (mmap_msg + SETTING_MSG_SIZE + MSG_LENGTH_SIZE + MSG_OFFSET_SIZE);
		flow_len = *(unsigned int *)(mmap_msg + SETTING_MSG_SIZE);
		flow_offset = *(unsigned int *)(mmap_msg + SETTING_MSG_SIZE + MSG_LENGTH_SIZE);
		msg_len = flow_len;
		msg = kmalloc(msg_len, GFP_KERNEL);
		if (flow_offset == 0) {
			memcpy((void *)msg, (void *)(flow_msg), msg_len);
		} else {
			memcpy((void *)msg, (void *)(flow_msg + flow_offset), (msg_len - flow_offset));
			memcpy((void *)(msg + msg_len - flow_offset), (void *)flow_msg, flow_offset);
		}
	}

	start_ptr = (unsigned int) msg;  //fill start print pointer
	total_print_size = msg_len;      //fill print size

	offset = (*pos) * MAX_ITERATION_PRINT_SIZE;

	if (offset >= total_print_size) {
		if (msg) {
			kfree(msg);
			msg = NULL;
		}
		return NULL; //end of print
	}

	v = (proc_print_buf_t *) kzalloc(sizeof(proc_print_buf_t), GFP_KERNEL);
	if (!v) {
		if (msg) {
			kfree(msg);
			msg = NULL;
		}
		return NULL; //end of print
	}

	v->print_buf = (char *)(start_ptr + offset);
	v->remain_size = total_print_size - offset;

	return v;
}

static void *seq_gmlib_err_start(struct seq_file *s, loff_t *pos)
{
	proc_print_buf_t *v;
	unsigned int start_ptr;
	unsigned int total_print_size;
	unsigned int offset;
	static char *msg = NULL;
	static int msg_len = 0;
	char *err_msg;
	unsigned int err_len;
	unsigned int err_offset;

	if (*pos == 0) {

		if (!mmap_msg) {
			if (msg) {
				kfree(msg);
				msg = NULL;
			}
			return NULL;
		}

		err_msg = (char *)(mmap_msg + SETTING_MSG_SIZE + FLOW_MSG_SIZE + MSG_LENGTH_SIZE + MSG_OFFSET_SIZE);
		err_len = *(unsigned int *)(mmap_msg + SETTING_MSG_SIZE + FLOW_MSG_SIZE);
		err_offset = *(unsigned int *)(mmap_msg + SETTING_MSG_SIZE + FLOW_MSG_SIZE + MSG_LENGTH_SIZE);

		msg_len = err_len;
		msg = kmalloc(msg_len, GFP_KERNEL);
		if (err_offset == 0) {
			memcpy((void *)msg, (void *)(err_msg), msg_len);
		} else {
			memcpy((void *)msg, (void *)(err_msg + err_offset), (msg_len - err_offset));
			memcpy((void *)(msg + msg_len - err_offset), (void *)err_msg, err_offset);
		}
	}

	start_ptr = (unsigned int) msg;  //fill start print pointer
	total_print_size = msg_len;      //fill print size

	offset = (*pos) * MAX_ITERATION_PRINT_SIZE;

	if (offset >= total_print_size) {
		if (msg) {
			kfree(msg);
			msg = NULL;
		}
		return NULL; //end of print
	}

	v = (proc_print_buf_t *) kzalloc(sizeof(proc_print_buf_t), GFP_KERNEL);
	if (!v) {
		if (msg) {
			kfree(msg);
			msg = NULL;
		}
		return NULL; //end of print
	}

	v->print_buf = (char *)(start_ptr + offset);
	v->remain_size = total_print_size - offset;

	return v;
}
#endif
/* <----- hdal setting message ----> <len(4bytes) - flow message>  */
#if 0
static void *seq_hdal_setting_start(struct seq_file *s, loff_t *pos)
{
	proc_print_buf_t *v;
	unsigned int start_ptr;
	unsigned int total_print_size;
	unsigned int offset;
	static char *msg = NULL;
	static int msg_len = 0;
	char help_string[] = "Usage: echo [module_type] [all | device_id] > hdal_setting\n"
						 "       module_type: graph, videocap, videoproc, videoout, videodec, videoenc, audiocap, audioenc, audiodec, audioout\n";

	if (*pos == 0) {
		char *hdal_setting_msg;
		unsigned int hdal_setting_len;

		if (!mmap_msg) {
			if (msg) {
				kfree(msg);
				msg = NULL;
			}
			return NULL;
		}
		if (log_hdal_proc_cb.registered != 0) {
			log_hdal_proc_cb.printout_notifier((int) hdal_cmd_buffer);
			set_current_state(TASK_UNINTERRUPTIBLE);
			schedule_timeout(msecs_to_jiffies(300));
		}
		hdal_setting_msg = (char *) (mmap_msg + SETTING_MSG_SIZE + FLOW_MSG_SIZE + ERR_MSG_SIZE);
		hdal_setting_len = strlen(hdal_setting_msg) + strlen(help_string);
		msg_len = hdal_setting_len;
		msg = kmalloc(msg_len, GFP_KERNEL);
		if (msg == NULL) {
			return NULL;
		}
		memcpy((void *) msg, (void *) help_string, strlen(help_string));
		memcpy((void *) msg + strlen(help_string), (void *) hdal_setting_msg, msg_len);
	}

	start_ptr = (unsigned int) msg;  //fill start print pointer
	total_print_size = msg_len;      //fill print size

	offset = (*pos) * MAX_ITERATION_PRINT_SIZE;

	if (offset >= total_print_size) {
		if (msg) {
			kfree(msg);
			msg = NULL;
		}
		return NULL; //end of print
	}

	v = (proc_print_buf_t *) kzalloc(sizeof(proc_print_buf_t), GFP_KERNEL);
	if (!v) {
		if (msg) {
			kfree(msg);
			msg = NULL;
		}
		return NULL; //end of print
	}

	v->print_buf = (char *)(start_ptr + offset);
	v->remain_size = total_print_size - offset;

	return v;
}
#endif
#if 0
static int hdal_setting_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	if (count > sizeof(hdal_cmd_buffer) - 1) {
		printk("count %d > max %d\n", count, sizeof(hdal_cmd_buffer));
		return -EINVAL;
	}

	if (copy_from_user(hdal_cmd_buffer, buffer, count)) {
		return 0;
	}
	hdal_cmd_buffer[count] = '\0';

	return count;
}
#endif

static void *seq_hdal_flow_start(struct seq_file *s, loff_t *pos)
{
	proc_print_buf_t *v;
	unsigned int start_ptr;
	unsigned int total_print_size;
	unsigned int offset;
	static char *msg = NULL;
	static int msg_len = 0;
	char *hdal_msg, *start_addr;
	unsigned int hdal_len;
	unsigned int hdal_offset;

	if (*pos == 0) {
		if (!mmap_msg) {
			if (msg) {
				kfree(msg);
				msg = NULL;
			}
			return NULL;
		}

		start_addr = (char *)(mmap_msg + SETTING_MSG_SIZE + FLOW_MSG_SIZE + ERR_MSG_SIZE + HDAL_SETTING_MSG_SIZE);
		hdal_len = *(unsigned int *)(start_addr);
		hdal_offset = *(unsigned int *)(start_addr + MSG_LENGTH_SIZE);
		hdal_msg = start_addr + MSG_LENGTH_SIZE + MSG_OFFSET_SIZE;

		msg_len = hdal_len;
		msg = kmalloc(msg_len, GFP_KERNEL);
		if (msg == NULL) {
			return NULL;
		}
		if (hdal_offset == 0) {
			memcpy((void *)msg, (void *)(hdal_msg), msg_len);
		} else {
			memcpy((void *)msg, (void *)(hdal_msg + hdal_offset), (msg_len - hdal_offset));
			memcpy((void *)(msg + msg_len - hdal_offset), (void *)hdal_msg, hdal_offset);
		}
	}

	start_ptr = (unsigned int) msg;  //fill start print pointer
	total_print_size = msg_len;      //fill print size

	offset = (*pos) * MAX_ITERATION_PRINT_SIZE;

	if (offset >= total_print_size) {
		if (msg) {
			kfree(msg);
			msg = NULL;
		}
		return NULL; //end of print
	}

	v = (proc_print_buf_t *) kzalloc(sizeof(proc_print_buf_t), GFP_KERNEL);
	if (!v) {
		if (msg) {
			kfree(msg);
			msg = NULL;
		}
		return NULL; //end of print
	}

	v->print_buf = (char *)(start_ptr + offset);
	v->remain_size = total_print_size - offset;

	return v;
}
static int proc_hdal_version_show(struct seq_file *s, void *v)
{
	int start, size;
	if (s) {
		seq_printf(s, "HDAL: Version: v%x.%x.%x\n",
				(hdal_version & 0xF00000) >> 20,
				(hdal_version & 0x0FFFF0) >> 4,
				(hdal_version & 0x00000F));
		seq_printf(s, "HDAL: IMPL Version: v%x.%x.%x\n",
				(impl_version & 0xF00000) >> 20,
				(impl_version & 0x0FFFF0) >> 4,
				(impl_version & 0x00000F));
	} else {
		printk("HDAL: Version: v%x.%x.%x\n",
				(hdal_version & 0xF00000) >> 20,
				(hdal_version & 0x0FFFF0) >> 4,
				(hdal_version & 0x00000F));
		printk("HDAL: IMPL Version: v%x.%x.%x\n",
				(impl_version & 0xF00000) >> 20,
				(impl_version & 0x0FFFF0) >> 4,
				(impl_version & 0x00000F));
	}

	calculate_log(&start, &size);
	printk("==========================================================================\n");
	prepare_dump_console(start, size);
	return 0;
}
#if 0
extern unsigned long loops_per_jiffy;
static int proc_delay2_write_mode(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	char ker_buffer[64];
	unsigned int delay, loop;
	unsigned long flags;

	if (count > sizeof(ker_buffer)) {
		printk("count %d > max %d\n", count, sizeof(ker_buffer));
		return -EINVAL;
	}

	if (copy_from_user((void *)ker_buffer, buffer, count)) {
		return -EINVAL;
	}

	sscanf(ker_buffer, "%d", &delay);

	loop = msecs_to_jiffies(delay * loops_per_jiffy * 1000);
	spin_lock_irqsave(&delay_lock, flags);
	while ((loop--) > 0) //busy loop
		;
	spin_unlock_irqrestore(&delay_lock, flags);
	return count;
}
#endif

/* pa2ddrid function: It is only for RC, EP side physical address not support!
   parameter rc_pa:  RC physical address
   return value: RC ddr_id
*/
int pa2ddrid(unsigned int rc_pa)
{
#if 0
	int ddr_id;
	frammap_ddrinfo_t info;

	frm_get_ddrinfo(&info);
	for (ddr_id = 0; ddr_id < info.nr_banks; ddr_id++) {
		if (info.bank[ddr_id].chip != (frm_pcie_num_t)PCIE_RC) {
			continue;
		}
		if ((rc_pa >= info.bank[ddr_id].start) &&
			(rc_pa < info.bank[ddr_id].start + info.bank[ddr_id].size - 1)) {

			return ddr_id;
		}
	}
#endif
	return 0; // for 680, always 0
}

static int log_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int log_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static long log_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	char log_string[MAX_STRING_LEN];

	switch (cmd) {
	case IOCTL_PRINTM:
		memset(log_string, 0, sizeof(log_string));
		if (copy_from_user((void *)&log_string, (void __user *)arg, sizeof(char) * MAX_STRING_LEN)) {
			return -EFAULT;
		}
		printm("UR", "(%d)%s", task_pid_nr(current), log_string);
		break;

	case IOCTL_PRINTM_WITH_PANIC:
		memset(log_string, 0, sizeof(log_string));
		if (copy_from_user((void *)&log_string, (void __user *)arg, sizeof(char) * MAX_STRING_LEN)) {
			return -EFAULT;
		}
		printm("UR", "(%d)%s", task_pid_nr(current), log_string);
		damnit("UR");
		break;
	case IOCTL_SET_HDAL_VERSION:
		memset(&hdal_version, 0, sizeof(unsigned int));
		if (copy_from_user((void *)&hdal_version, (void __user *)arg, sizeof(unsigned int))) {
			return -EFAULT;
		}
		break;
	case IOCTL_SET_IMPL_VERSION:
		memset(&impl_version, 0, sizeof(unsigned int));
		if (copy_from_user((void *)&impl_version, (void __user *)arg, sizeof(unsigned int))) {
			return -EFAULT;
		}
		break;
	default:
		return -EFAULT;
	}
	return 0;
}

int log_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long size = vma->vm_end - vma->vm_start;
	unsigned int pfn;

	if (!mmap_msg) {
		return -EFAULT;
	}

	pfn = __pa(mmap_msg) >> PAGE_SHIFT;
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot)) {
		printk("Error to remap mmap\n");
		return -EFAULT;
	}
	return 0;
}

#if 0
static int affinity_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_affinity_seq_show, NULL);
}
#endif

static int mode_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_mode_seq_show, NULL);
}

static int dumplog_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_dumplog_seq_show, NULL);

}

#if 0
static int delay_non_preemptive_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_delay_non_preemptive_seq_show, NULL);
}

static int delay_irq_disabled_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_delay_irq_disabled_seq_show, NULL);
}

static int gmlib_setting_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &seq_gmlib_setting_ops);
}

static int gmlib_flow_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &seq_gmlib_flow_ops);
}

static int gmlib_err_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &seq_gmlib_err_ops);
}
#endif

#if 0
static int hdal_setting_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &seq_hdal_setting_ops);
}
#endif

static int hdal_flow_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &seq_hdal_flow_ops);
}

static int hdal_version_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_hdal_version_show, NULL);
}

#if 0
static int thread_realtime_test_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_thread_realtime_test_seq_show, NULL);
}
#endif


#if 0
static struct file_operations affinity_proc_ops = {
	.owner   = THIS_MODULE,
	.open    = affinity_proc_open,
	.read    = seq_read,
	.write   = proc_affinity_write_mode,
	.llseek  = seq_lseek,
	.release = seq_release
};
#endif

static struct file_operations mode_proc_ops = {
	.owner   = THIS_MODULE,
	.open    = mode_proc_open,
	.read    = seq_read,
	.write   = proc_mode_write_mode,
	.llseek  = seq_lseek,
	.release = seq_release
};

static struct file_operations dumplog_proc_ops = {
	.owner   = THIS_MODULE,
	.open    = dumplog_proc_open,
	.read    = seq_read,
	.write   = proc_dumplog_write_mode,
	.llseek  = seq_lseek,
	.release = seq_release
};

#if 0
static struct file_operations delay_non_preemptive_proc_ops = {
	.owner   = THIS_MODULE,
	.open    = delay_non_preemptive_proc_open,
	.read    = seq_read,
	.write   = proc_delay1_write_mode,
	.llseek  = seq_lseek,
	.release = seq_release
};

static struct file_operations delay_irq_disabled_proc_ops = {
	.owner   = THIS_MODULE,
	.open    = delay_irq_disabled_proc_open,
	.read    = seq_read,
	.write   = proc_delay2_write_mode,
	.llseek  = seq_lseek,
	.release = seq_release
};

static struct file_operations gmlib_setting_proc_ops = {
	.owner   = THIS_MODULE,
	.open    = gmlib_setting_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};

static struct file_operations gmlib_flow_proc_ops = {
	.owner   = THIS_MODULE,
	.open    = gmlib_flow_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};

static struct file_operations gmlib_err_proc_ops = {
	.owner   = THIS_MODULE,
	.open    = gmlib_err_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};
#endif

#if 0
static struct file_operations hdal_setting_proc_ops = {
	.owner   = THIS_MODULE,
	.open    = hdal_setting_proc_open,
	.read    = seq_read,
	.write   = hdal_setting_write,
	.llseek  = seq_lseek,
	.release = seq_release
};
#endif


static struct file_operations hdal_flow_proc_ops = {
	.owner   = THIS_MODULE,
	.open    = hdal_flow_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};

static struct file_operations hdal_version_proc_ops = {
	.owner   = THIS_MODULE,
	.open    = hdal_version_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};

#if 0
static struct file_operations thread_realtime_test_proc_ops = {
	.owner   = THIS_MODULE,
	.open    = thread_realtime_test_proc_open,
	.read    = seq_read,
	.write   = proc_thread_write_mode,
	.llseek  = seq_lseek,
	.release = seq_release
};
#endif


struct file_operations log_fops = {
owner:
	THIS_MODULE,
unlocked_ioctl:
	log_ioctl,
mmap:
	log_mmap,
open:
	log_open,
release:
	log_release,
};

#if 0
struct miscdevice log_dev = {
minor:
	MISC_DYNAMIC_MINOR,
name: "log_vg"
	,
fops:
	&log_fops,
};
#else
typedef struct vglog_drv_info {
	struct class *pmodule_class;
	struct device *pdevice[MODULE_MINOR_COUNT];
	//struct resource* presource[MODULE_REG_NUM];
	struct cdev cdev;
	dev_t dev_id;
} VGLOG_DRV_INFO, *PVGLOG_DRV_INFO;
static VGLOG_DRV_INFO m_pdrv_info;
#endif

#if defined(_GROUP_KO_)
#undef __init
#undef __exit
#undef module_init
#undef module_exit
#define __init
#define __exit
#define module_init(x)
#define module_exit(x)
#endif

extern int vg_info_init(void);
extern int vg_info_exit(void);
int __init log_init(void)
{
	int i;
	int ret = 0;
	unsigned char ucloop;
	VGLOG_DRV_INFO* pdrv_info = &m_pdrv_info;

	log_init_mem();
	log_base_start_pa = __pa(log_base_start);
	log_base_ddr = pa2ddrid(log_base_start_pa);
	if (!mmap_msg) {
		panic("Error to allocate mmap buffer\n");
	}

	printk("log.ko %s: %s %s (mmap 0x%x size 0x%x)\n", LOG_VERSION, __DATE__, __TIME__, (int)mmap_msg, MMAP_MSG_LEN);
	memset((char *)log_base_start, 0, log_bsize);
	if (log_base_start == 0) {
		printk("Error to allocate debug buffer!\n");
		return -1;
	}
	memset(log_slice_ptr, 0, sizeof(int) * MAX_DUMP_FILE);
	memset(log_size, 0, sizeof(int) * MAX_DUMP_FILE);
	memset(log_path, 0, sizeof(char) * MAX_DUMP_FILE * MAX_PATH_WIDTH);
	printk("\nLOG base 0x%x(ddr%d) size %dK (start pointer 0x%x)\n", log_base_start, log_base_ddr,
		   log_ksize, (int)&log_start_ptr);
	printk("PAGE_OFFSET(0x%x) VMALLOC START(0x%x) HZ(%d)\n",
		   (int)PAGE_OFFSET, (int)VMALLOC_START, HZ);

	for (i = 0; i < MAX_CB; i++) {
		log_panic_cb[i].registered = 0;
		log_printout_cb[i].registered = 0;
	}
	log_master_print_cb.registered = 0;
	log_hdal_proc_cb.registered = 0;

#if 0
	videoproc = proc_mkdir("videograph", NULL);
	if (videoproc == NULL) {
		return -EIO;
	}

	debugproc = proc_mkdir("debug", videoproc);
	if (debugproc == NULL) {
		return -EIO;
	}

	affinityproc = proc_create("affinity", 0, debugproc, &affinity_proc_ops);
	if (affinityproc == NULL) {
		return -EIO;
	}

	modeproc = proc_create("mode", 0, videoproc, &mode_proc_ops);
	if (modeproc == NULL) {
		return -EIO;
	}

	dump2proc = proc_create("dumplog", 0, videoproc, &dumplog_proc_ops);
	if (dump2proc == NULL) {
		return -EIO;
	}

	delay1proc = proc_create("delay_non_preemptive", 0, debugproc, &delay_non_preemptive_proc_ops);
	if (delay1proc == NULL) {
		return -EIO;
	}

	delay2proc = proc_create("delay_irq_disabled", 0, debugproc, &delay_irq_disabled_proc_ops);
	if (delay2proc == NULL) {
		return -EIO;
	}

	gmlib_setting_proc = proc_create("gmlib_setting", 0, videoproc, &gmlib_setting_proc_ops);
	if (gmlib_setting_proc == NULL) {
		return -EIO;
	}

	gmlib_flow_proc = proc_create("gmlib_flow", 0, videoproc, &gmlib_flow_proc_ops);
	if (gmlib_flow_proc == NULL) {
		return -EIO;
	}

	gmlib_err_proc = proc_create("gmlib_err", 0, videoproc, &gmlib_err_proc_ops);
	if (gmlib_err_proc == NULL) {
		return -EIO;
	}
#endif
	hdal_proc = proc_mkdir("hdal", NULL);
	if (hdal_proc == NULL) {
		return -EIO;
	}

#if 0
	hdal_setting_proc = proc_create("setting", 0, hdal_proc, &hdal_setting_proc_ops);
	if (hdal_setting_proc == NULL) {
		return -EIO;
	}
#endif

	hdal_flow_proc = proc_create("flow", 0, hdal_proc, &hdal_flow_proc_ops);
	if (hdal_flow_proc == NULL) {
		return -EIO;
	}

	hdal_version_proc = proc_create("version", 0, hdal_proc, &hdal_version_proc_ops);
	if (hdal_version_proc == NULL) {
		return -EIO;
	}

	modeproc = proc_create("log_mode", 0, hdal_proc, &mode_proc_ops);
	if (modeproc == NULL) {
		return -EIO;
	}

	dump2proc = proc_create("log_dump", 0, hdal_proc, &dumplog_proc_ops);
	if (dump2proc == NULL) {
		return -EIO;
	}

#if 0
	threadproc = proc_create("thread_realtime_test", 0, debugproc, &thread_realtime_test_proc_ops);
	if (threadproc == NULL) {
		return -EIO;
	}

	init_timer(&log_timer);
	write_task = kthread_create(write_thread, 0, "log_thread");
	if (IS_ERR(write_task)) {
		return -EFAULT;
	}
	wake_up_process(write_task);

	monitor_task = kthread_create(monitor_thread, 0, "threadmon");
	if (IS_ERR(monitor_task)) {
		return -EFAULT;
	}
	wake_up_process(monitor_task);

	notify_task = kthread_create(notify_thread, 0, "log_notify");
	if (IS_ERR(notify_task)) {
		return -EFAULT;
	}
	wake_up_process(notify_task);

	init_waitqueue_head(&log_wq);
	spin_lock_init(&delay_lock);
#endif

	//vg_info_init();

	getnstimeofday(&log_mod_insert_time);
	printm_pif_init();
#if 0
	return misc_register(&log_dev);
#else
	//Dynamic to allocate Device ID
	if (vos_alloc_chrdev_region(&pdrv_info->dev_id, MODULE_MINOR_COUNT, MODULE_NAME)) {
		pr_err("Can't get device ID\n");
		return -ENODEV;
	}

	/* Register character device for the volume */
	cdev_init(&pdrv_info->cdev, &log_fops);
	pdrv_info->cdev.owner = THIS_MODULE;

	if (cdev_add(&pdrv_info->cdev, pdrv_info->dev_id, MODULE_MINOR_COUNT)) {
		pr_err("Can't add cdev\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	pdrv_info->pmodule_class = class_create(THIS_MODULE, MODULE_NAME);
	if(IS_ERR(pdrv_info->pmodule_class)) {
		pr_err("failed in creating class.\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	/* register your own device in sysfs, and this will cause udev to create corresponding device node */
	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT ) ; ucloop++) {
		pdrv_info->pdevice[ucloop] = device_create(pdrv_info->pmodule_class, NULL
			, MKDEV(MAJOR(pdrv_info->dev_id), (ucloop + MINOR(pdrv_info->dev_id))), NULL
			, MODULE_NAME);

		if(IS_ERR(pdrv_info->pdevice[ucloop])) {

			pr_err("failed in creating device%d.\n", ucloop);

			if (ucloop == 0) {
				device_unregister(pdrv_info->pdevice[ucloop]);
			}

			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}
	return ret;

FAIL_CLASS:
	class_destroy(pdrv_info->pmodule_class);

FAIL_CDEV:
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MODULE_MINOR_COUNT);

	return ret;
#endif
}

void __exit log_clearnup(void)
{
	unsigned char ucloop;
	VGLOG_DRV_INFO* pdrv_info = &m_pdrv_info;

	printm_pif_uninit();
	if (write_task) {
		kthread_stop(write_task);
	}
	if (monitor_task) {
		kthread_stop(monitor_task);
	}
	if (notify_task) {
		kthread_stop(notify_task);
	}

	log_uninit_mem();
	//vg_info_exit();

	if (affinityproc != 0) {
		remove_proc_entry("affinity", videoproc);
	}
	if (modeproc != 0) {
		remove_proc_entry("mode", videoproc);
	}
	if (dump2proc != 0) {
		remove_proc_entry("dumplog", videoproc);
	}
	if (delay1proc != 0) {
		remove_proc_entry("delay_non_preemptive", debugproc);
	}
	if (delay2proc != 0) {
		remove_proc_entry("delay_irq_disabled", debugproc);
	}
	if (threadproc != 0) {
		remove_proc_entry("thread_realtime_test", debugproc);
	}
	if (gmlib_setting_proc != 0) {
		remove_proc_entry("gmlib_setting", videoproc);
	}
	if (gmlib_flow_proc != 0) {
		remove_proc_entry("gmlib_flow", videoproc);
	}
	if (gmlib_err_proc != 0) {
		remove_proc_entry("gmlib_err", videoproc);
	}
	if (debugproc != 0) {
		remove_proc_entry("debug", videoproc);
	}
	if (videoproc != 0) {
		remove_proc_entry("videograph", NULL);
	}
	if (hdal_setting_proc != 0) {
		remove_proc_entry("setting", hdal_proc);
	}
	if (hdal_flow_proc != 0) {
		remove_proc_entry("flow", hdal_proc);
	}
	if (hdal_version_proc != 0) {
		remove_proc_entry("version", hdal_proc);
	}
	if (hdal_proc != 0) {
		remove_proc_entry("hdal", NULL);
	}
	//misc_deregister(&log_dev);

	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT ) ; ucloop++)
		device_unregister(pdrv_info->pdevice[ucloop]);

	class_destroy(pdrv_info->pmodule_class);
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MODULE_MINOR_COUNT);
}

void *get_hdal_proc_dir_entry(void)
{
	return (void *)hdal_proc;
}


module_init(log_init);
module_exit(log_clearnup);

MODULE_AUTHOR("Novatek Microelectronics Corp.");
MODULE_LICENSE("GPL");

#if 0
EXPORT_SYMBOL(register_panic_notifier);
EXPORT_SYMBOL(register_printout_notifier);
EXPORT_SYMBOL(register_master_print_notifier);
EXPORT_SYMBOL(register_hdal_proc_notifier);
EXPORT_SYMBOL(damnit);
EXPORT_SYMBOL(dumplog);
EXPORT_SYMBOL(printm);
EXPORT_SYMBOL(master_print);
EXPORT_SYMBOL(dumpbuf_pa);
EXPORT_SYMBOL(dumpbuf_va);
EXPORT_SYMBOL(register_version);
EXPORT_SYMBOL(unregister_printout_notifier);
EXPORT_SYMBOL(unregister_panic_notifier);
EXPORT_SYMBOL(get_cpu_state);
EXPORT_SYMBOL(pa2ddrid);
#endif
EXPORT_SYMBOL(get_hdal_proc_dir_entry);
