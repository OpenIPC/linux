#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <plat/nvttmr010.h>
#include <plat/nvt_jiffies.h>

/*
 * Macro definitions
 */
#define HEART_BEAT_INTVAL   (10 * HZ)
#define time_disance(a, b)  ((long)(b) - (long)(a))

#define MAX_CMD_LENGTH 30
#define MAX_ARG_NUM     6

/*
 * Local variables
 */
static unsigned long __nvt_jiffies = 0;  /* 1ms granularity */
static u64  __nvt_jiffies_u64 = 0;       /* 1ms granularity */
static unsigned long clk_khz = 0;
static struct timer_list  nvt_jiffies_tmr;
//static spinlock_t spinlock;
DEFINE_SPINLOCK(spinlock);
static unsigned int refer_cnt = 0;
static struct nvttmr010_clocksource *nvt_clk_src = NULL, clocksource;
static struct proc_dir_entry *nvt_jiffies_proc = NULL;

/*
 * Local functions
 */
static unsigned int distance = 0;

/* Proc function
 */
static int proc_read_nvt_jiffies(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "nvt jiffies: 0x%x, HZ = %d \n", (u32)nvt_jiffies, HZ);
    seq_printf(sfile, "reference count: 0x%d \n", refer_cnt);

    return 0;
}

static int nvt_jiffies_proc_cmd_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_read_nvt_jiffies, NULL);
}

static void nvt_jiffies_test(void)
{
	unsigned long jiffies_0 = 0, jiffies_1 = 0;
	int delay_ms = 0;

	// delay 500 ms
	delay_ms = 500;
	jiffies_0 = get_nvt_jiffies();
	printk("Test jiffies_0: %d\n", (int)jiffies_0);
	mdelay(delay_ms);
	jiffies_1 = get_nvt_jiffies();
	printk("Test jiffies_1: %d\n", (int)jiffies_1);
	printk("jiffies_1 - jiffies_0: %d ms , expected spend time: %d ms \n", (int)(jiffies_1 -jiffies_0),(int)delay_ms);

	// delay 5000 ms
	delay_ms = 5000;
	jiffies_0 = get_nvt_jiffies();
	printk("Test jiffies_0: %d\n", (int)jiffies_0);
	mdelay(delay_ms);
	jiffies_1 = get_nvt_jiffies();
	printk("Test jiffies_1: %d\n", (int)jiffies_1);
	printk("jiffies_1 - jiffies_0: %d ms , expected spend time: %d ms \n", (int)(jiffies_1 -jiffies_0),(int)delay_ms);

}

static ssize_t nvt_jiffies_proc_cmd_write(struct file *file, const char __user *buf,	 size_t size, loff_t *off)
{
	int len = size;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char ucargc = 0;

	// check command length
	if (len > (MAX_CMD_LENGTH - 1)) {
		pr_err("Command length is too long!\n");
		goto ERR_OUT;
	}

	// copy command string from user space
	if (copy_from_user(cmd_line, buf, len)) {
		goto ERR_OUT;
	}

	if (len == 0) {
		cmd_line[0] = '\0';
	} else {
		cmd_line[len - 1] = '\0';
	}

	// parse command string
	for (ucargc = 0; ucargc < MAX_ARG_NUM; ucargc++) {
		argv[ucargc] = strsep(&cmdstr, delimiters);

		if (argv[ucargc] == NULL) {
			break;
		}
	}

	// dispatch command handler
	if (strncmp(argv[0], "t", 2) == 0) {
		nvt_jiffies_test();
	} else {
		goto ERR_OUT;
	}

	return size;

ERR_OUT:
	return -1;

}

static const struct file_operations nvt_jiffies_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= nvt_jiffies_proc_cmd_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.write		= nvt_jiffies_proc_cmd_write
};


/* get 1m jiffies */
unsigned long get_nvt_jiffies(void)
{
    static unsigned long keep_clk_val = 0;
    unsigned long flags, new_clk_val;

    if (nvt_clk_src == NULL)
        return 0;

    /* lock */
    spin_lock_irqsave(&spinlock, flags);
    new_clk_val = nvt_clk_src->clocksource.read(&nvt_clk_src->clocksource);
    if (!keep_clk_val && !new_clk_val) {  /* not start yet */
        spin_unlock_irqrestore(&spinlock, flags);
        return 0;
    }

    distance += time_disance(keep_clk_val, new_clk_val);
    keep_clk_val = new_clk_val;

    if (distance >= clk_khz) {
        unsigned long value = distance / clk_khz;

        __nvt_jiffies += value;
        __nvt_jiffies_u64 += value;
        distance = distance % clk_khz;
    }

    /* unlock */
    spin_unlock_irqrestore(&spinlock, flags);

    refer_cnt ++;

    return __nvt_jiffies;
}
EXPORT_SYMBOL(get_nvt_jiffies);

/* get 1m jiffies */
u64 get_nvt_jiffies_u64(void)
{
    get_nvt_jiffies();

    return __nvt_jiffies_u64;
}
EXPORT_SYMBOL(get_nvt_jiffies_u64);


/*
 * A function to update __nvt_jiffies periodically if there is no caller to call get_nvt_jiffies()
 *   in a long interval.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0)
void nvt_heatbeat_handler(unsigned long data)
{
    struct nvttmr010_clocksource *nvt_clk_src = (struct nvttmr010_clocksource *)data;

    /* in order to prevent __nvt_jiffies not update in a long time */
    get_nvt_jiffies();

    nvt_jiffies_tmr.function = nvt_heatbeat_handler;
    nvt_jiffies_tmr.data = (unsigned long)nvt_clk_src;
    mod_timer(&nvt_jiffies_tmr, jiffies + HEART_BEAT_INTVAL);
}
#else
void nvt_heatbeat_handler(struct timer_list *timer)
{
    /* in order to prevent __nvt_jiffies not update in a long time */
    get_nvt_jiffies();

	timer_setup(&nvt_jiffies_tmr, nvt_heatbeat_handler, 0);
    mod_timer(&nvt_jiffies_tmr, jiffies + HEART_BEAT_INTVAL);
}
#endif

int __init nvt_jiffies_init(void)
{
	nvt_clk_src = nvttmr010_get_clocksource();
    if (nvt_clk_src == NULL)
        panic("%s, data is NULL! \n", __func__);

    clk_khz = nvt_clk_src->freq / 1000;

    /* sanity check */
    if ((HEART_BEAT_INTVAL / HZ) >= (0xffffffff / nvt_clk_src->freq))
        panic("%s, HEART_BEAT_INTVAL = %d out of range! \n", __func__, HEART_BEAT_INTVAL / HZ);

    memcpy(&clocksource, nvt_clk_src, sizeof(struct nvttmr010_clocksource));

    return 0;
}

/* post init. In nvt_jiffies_init(), if we enable timer the kernel will pop out warning message. Thus
 * we move the remaining code from nvt_jiffies_init() to here.
 */
int __init nvt_jiffies_postinit(void)
{
	struct proc_dir_entry *pmodule_root = NULL;

	nvt_jiffies_init();

	if (nvt_clk_src == NULL) {
        printk("%s, nvt_clk_src is NULL! It may timer010 be disabled!\n", __func__);
        return -1;
    }

	pmodule_root = proc_mkdir("nvt_jiffies", NULL);
	if (pmodule_root == NULL) {
		pr_err("failed to create Module root\n");
		return -1;
	}

    nvt_jiffies_proc = proc_create("nvt_jiffies_cmd", S_IRUGO, pmodule_root, &nvt_jiffies_proc_fops);
    if (nvt_jiffies_proc == NULL)
        panic("%s, error in create proc! \n", __func__);

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0)
    init_timer(&nvt_jiffies_tmr);
    nvt_jiffies_tmr.function = nvt_heatbeat_handler;
    nvt_jiffies_tmr.data = (unsigned long)nvt_clk_src;
#else
    timer_setup(&nvt_jiffies_tmr, nvt_heatbeat_handler, 0);
#endif
    mod_timer(&nvt_jiffies_tmr, jiffies + HZ);
    printk("nvt_jiffies: system HZ: %d, pClk: %d \n", HZ, nvt_clk_src->freq);

    return 0;
}
core_initcall(nvt_jiffies_postinit);

MODULE_AUTHOR("Novatek Microelectronics Corp.");
MODULE_LICENSE("GPL");
