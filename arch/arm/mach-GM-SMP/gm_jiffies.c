#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/proc_fs.h>
#include <linux/timer.h>
#include <mach/fttmr010.h>
#include <mach/gm_jiffies.h>

/*
 * Macro definitions
 */
#define HEART_BEAT_INTVAL   (10 * HZ)
#define time_disance(a, b)  ((long)(b) - (long)(a))

/*
 * Local variables
 */
static unsigned long __gm_jiffies = 0;  /* 1ms granularity */
static u64  __gm_jiffies_u64 = 0;       /* 1ms granularity */
static unsigned long clk_khz = 0;
static struct timer_list  gm_jiffies_tmr;
static spinlock_t spinlock;
static unsigned int refer_cnt = 0;
static struct fttmr010_clocksource *pGmClkSrc = NULL;
static struct proc_dir_entry *gm_jiffies_proc = NULL;

/*
 * Local functions
 */
static int proc_read_gm_jiffies(char *page, char **start, off_t off, int count, int *eof, void *data);

static unsigned int distance = 0;

/* get 1m jiffies */
unsigned long get_gm_jiffies(void)
{
    static unsigned long keep_clk_val = 0;
    unsigned long flags, new_clk_val;

    if (pGmClkSrc == NULL)
        return 0;

    /* lock */
    spin_lock_irqsave(&spinlock, flags);
    new_clk_val = pGmClkSrc->clocksource.read(&pGmClkSrc->clocksource);
    if (!keep_clk_val && !new_clk_val) {  /* not start yet */
        spin_unlock_irqrestore(&spinlock, flags);
        return 0;
    }

    distance += time_disance(keep_clk_val, new_clk_val);
    keep_clk_val = new_clk_val;

    if (distance >= clk_khz) {
        unsigned long value = distance / clk_khz;

        __gm_jiffies += value;
        __gm_jiffies_u64 += value;
        distance = distance % clk_khz;
    }

    /* unlock */
    spin_unlock_irqrestore(&spinlock, flags);

    refer_cnt ++;

    return __gm_jiffies;
}
EXPORT_SYMBOL(get_gm_jiffies);

/* get 1m jiffies */
u64 get_gm_jiffies_u64(void)
{
    get_gm_jiffies();

    return __gm_jiffies_u64;
}
EXPORT_SYMBOL(get_gm_jiffies_u64);

/*
 * A function to update __gm_jiffies periodically if there is no caller to call get_gm_jiffies()
 *   in a long interval.
 */
void gm_heatbeat_handler(unsigned long data)
{
    struct fttmr010_clocksource *pGmClkSrc = (struct fttmr010_clocksource *)data;


    /* in order to prevent __gm_jiffies not update in a long time */
    get_gm_jiffies();

    gm_jiffies_tmr.function = gm_heatbeat_handler;
    gm_jiffies_tmr.data = (unsigned long)pGmClkSrc;
    mod_timer(&gm_jiffies_tmr, jiffies + HEART_BEAT_INTVAL);

    if (gm_jiffies_proc == NULL) {
        gm_jiffies_proc = create_proc_entry("gm_jiffies", S_IRUGO, NULL);
        if (gm_jiffies_proc == NULL)
            panic("%s, error in create proc! \n", __func__);

        gm_jiffies_proc->read_proc = (read_proc_t *) proc_read_gm_jiffies;
        gm_jiffies_proc->write_proc = NULL;
    }
}

int __init gm_jiffies_init(void *data)
{
    pGmClkSrc = (struct fttmr010_clocksource *)data;
    if (pGmClkSrc == NULL)
        panic("%s, data is NULL! \n", __func__);

    clk_khz = pGmClkSrc->freq / 1000;

    /* sanity check */
    if ((HEART_BEAT_INTVAL / HZ) >= (0xffffffff / pGmClkSrc->freq))
        panic("%s, HEART_BEAT_INTVAL = %d out of range! \n", __func__, HEART_BEAT_INTVAL / HZ);

    spin_lock_init(&spinlock);
    init_timer(&gm_jiffies_tmr);
    gm_jiffies_tmr.function = gm_heatbeat_handler;
    gm_jiffies_tmr.data = (unsigned long)pGmClkSrc;
    mod_timer(&gm_jiffies_tmr, jiffies + HZ);
    printk("%s, system HZ: %d, pClk: %d \n", __func__, HZ, pGmClkSrc->freq);

    return 0;
}

static int proc_read_gm_jiffies(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;

    len += sprintf(page + len, "gm jiffies: 0x%x, HZ = %d \n", (u32)gm_jiffies, HZ);
    len += sprintf(page + len, "reference count: 0x%d \n", refer_cnt);

    return len;
}

MODULE_AUTHOR("Grain Media Corp.");
MODULE_LICENSE("GPL");
