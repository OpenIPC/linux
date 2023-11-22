/*
 * sstar_irq_debug_trace.c- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
#include <linux/kernel.h>
#include <linux/dma-mapping.h> /* for dma_alloc_coherent */
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include "registers.h"
#include "mdrv_msys_io_st.h"
#include <linux/ctype.h>
#include <linux/kthread.h>
#include <asm/irq_regs.h>
#include <linux/seq_file.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/io.h>
#include "mdrv_gpio_io.h"
#include "mdrv_gpio.h"
#include <linux/proc_fs.h>

extern unsigned long long __weak sched_clock(void);

//--------------------------------------------------------------------------------------------------
//  Global Variable and Functions
//--------------------------------------------------------------------------------------------------
struct device *girqdebug_class_dev;

int irqts_enable           = 1;
int irqoffHint             = 0;
int irqoff_enable[NR_CPUS] = {
    0,
};
int irqoffCnt[NR_CPUS] = {
    0,
};

int irqTH     = 5000000; // ns 5ms
int sirqTH    = 5000000;
int irqOffTH  = 5000000;
int sirqOffTH = 5000000;

#define IRQ_TRACE_TEST (0)

#define IRQ_LIST    (1000)
#define IRQOFF_LIST (1000)

#define KEPT_IRQ_LATENCY_SIZE (50)

struct list_head kept_irq_head;
struct list_head kept_sirq_head;
struct list_head kept_irqOff_head;

int sirq_head_initialized = 0;

static DEFINE_SPINLOCK(irq_latency_lock);
static DEFINE_SPINLOCK(irq_lock);

static unsigned int       iIrqLatencyCount       = 0;
static unsigned long long mIrqDisableTs[NR_CPUS] = {
    0,
};

static unsigned long long mIrqEnableTs[NR_CPUS] = {
    0,
};

int                        IRQ_COUNTER    = 0;
int                        SIRQ_COUNTER   = 0;
int                        IRQOFF_COUNTER = 0;
struct SSTAR_IRQ_INFO_LIST kept_irq_latency[KEPT_IRQ_LATENCY_SIZE];

#ifdef CONFIG_CPU_CP15
#define get_cpuid()                                                  \
    (                                                                \
        {                                                            \
            unsigned int __val;                                      \
            asm("mrc p15, 0, %0, c0, c0, 5" : "=r"(__val) : : "cc"); \
            __val & 0x3;                                             \
        })

#else

#define get_cpuid() smp_processor_id()
#endif

#define IRQ_SIZE 1024 // SPI hw_irq number rang in GIC_V3: 32~1019
unsigned int count[IRQ_SIZE] = {0};

#if IRQ_TRACE_TEST
struct task_struct *kreset_task;
static spinlock_t   irq_lock;
static ssize_t      irq_trace_test_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int   i;
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "Debug IRQ being disabled for long time:\n");

    for (i = 0; i < NR_CPUS; i++)
    {
        str += scnprintf(str, end - str, "CPU_%d: debug enable: %d, happen count: %d\n", i, irqoff_enable[i],
                         irqoffCnt[i]);
    }

    return (str - buf);
}
static int irq_trace_test_thread(void *data)
{
    unsigned long flags;

    for (;;)
    {
        spin_lock_irqsave(&irq_lock, flags);
        camdriver_gpio_set(NULL, 64, 1);
        mdelay(100);
        camdriver_gpio_set(NULL, 64, 0);
        spin_unlock_irqrestore(&irq_lock, flags);

        msleep(5000); // 500ms

        if (kthread_should_stop())
            break;
    }

    return 0;
}

extern void camdriver_gpio_set(struct gpio_chip *chip, unsigned offset, int value);
int         camdriver_gpio_direction_output(struct gpio_chip *chip, unsigned offset, int value);

static irqreturn_t irq_trace_test_interrupt(int irq, void *dev_id)
{
    camdriver_gpio_set(NULL, 62, 1);
    mdelay(2);
    camdriver_gpio_set(NULL, 62, 0);
    return IRQ_HANDLED;
}

static ssize_t irq_trace_test_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    int                 i = 0, enable = 0;
    int                 iIrqNum        = 0;
    char                compatible[16] = "sstar,timer2";
    struct device_node *dev_node;

    if (NULL != buf)
    {
        enable = ('1' == buf[0]) ? 1 : 0;
    }

    spin_lock_init(&irq_lock);
    kreset_task = kthread_create(irq_trace_test_thread, NULL, "TEST_Thread");
    wake_up_process(kreset_task);

    /*TEST*/
    i = gpio_request(62, "IRQ-test"); // 62

    camdriver_gpio_direction_output(NULL, 62, 0);
    camdriver_gpio_direction_output(NULL, 64, 0);

    camdriver_gpio_set(NULL, 62, 0);
    camdriver_gpio_set(NULL, 64, 0);
    dev_node = of_find_compatible_node(NULL, NULL, compatible);

    if (!dev_node)
    {
        printk("@@@error\r\n");
    }

    /* Register interrupt handler */
    iIrqNum = irq_of_parse_and_map(dev_node, 0);
    printk("iIrqNum %d\r\n", iIrqNum);
    i = request_irq(iIrqNum, irq_trace_test_interrupt, IRQF_SHARED, "timer2", dev);

    return n;
}
#endif

void sstar_irq_records_count(int irq_num)
{
    if (irq_num < IRQ_SIZE)
        count[irq_num]++;
}
void sstar_irq_dump_count(void)
{
    int i;
    printk("ms_dump_irq_count\n");
    for (i = 0; i < IRQ_SIZE; i++)
    {
        if (count[i] > 0)
        {
            printk("irq:%03d count:%8d\n", i, count[i]);
        }
    }
}

static bool get_trace(struct pt_regs *regs, bool hardirq, SSTAR_IRQOFF_INFO *irqoff_info)
{
    unsigned long back_trace[10];

    strlcpy(irqoff_info->comms, current->comm, TASK_COMM_LEN);
    irqoff_info->pids = current->pid;

    stack_trace_save(back_trace, ARRAY_SIZE(back_trace), 1);
    memcpy(irqoff_info->entries, back_trace, sizeof(unsigned long) * PER_TRACE_ENTRIES_AVERAGE);
    return true;
}

void sstar_irqoff_records(SSTAR_IRQOFF_INFO *irq_info)
{
    struct SSTAR_IRQ_OFF_LIST *new;

    if (IRQOFF_COUNTER < IRQOFF_LIST)
    {
        get_trace(get_irq_regs(), 1, irq_info);

        new = (struct SSTAR_IRQ_OFF_LIST *)kmalloc(sizeof(struct SSTAR_IRQ_OFF_LIST), GFP_KERNEL);

        if (new != NULL)
        {
            memcpy(&new->irqoff_info, irq_info, sizeof(SSTAR_IRQOFF_INFO));
            list_add(&new->irqoff_list, &kept_irqOff_head);
            IRQOFF_COUNTER += 1;
        }
    }
}

void sstar_irq_disable_debug(void)
{
    u8 id = get_cpuid();

    if ((id >= NR_CPUS) || (irqoff_enable[id] == 0))
    {
        return;
    }

    /* record the timestamp of irq disabling if debug is enabled */
    mIrqDisableTs[id] = sched_clock();
}
EXPORT_SYMBOL(sstar_irq_disable_debug);

void sstar_irq_enable_debug(void)
{
    u8 id = get_cpuid();

    SSTAR_IRQOFF_INFO irq_info;

    if ((id >= NR_CPUS) || (mIrqDisableTs[id] == 0) || (irqoff_enable[id] == 0))
    {
        return;
    }

    mIrqEnableTs[id] = sched_clock();

    if ((mIrqEnableTs[id] > mIrqDisableTs[id]))
    {
        unsigned long diff = (unsigned long)(mIrqEnableTs[id] - mIrqDisableTs[id]);

        if (diff >= irqOffTH)
        { // 2ms
            irqoffCnt[id]++;
            irq_info.pids      = current->pid; // current->comm ;
            irq_info.timeStart = mIrqDisableTs[id];
            irq_info.timeEnd   = mIrqEnableTs[id];
            irq_info.cpuid     = id;

            sstar_irqoff_records(&irq_info);

            if (irqoffHint)
                WARN_ON(irqoffCnt[id]);
        }
    }
}
EXPORT_SYMBOL(sstar_irq_enable_debug);

void sstar_irq_debug_ignore(void)
{
    u8 id = get_cpuid();

    if (id >= NR_CPUS)
    {
        return;
    }

    /* set timestap of irq disabling to zero, then irq_enable_debug will ignore one time */
    mIrqDisableTs[id] = 0;
}

void sstar_irq_dump_latency_info(void)
{
    int           i       = 0;
    unsigned long cost    = 0;
    int           counter = 0;

    printk("Irq latency larger than 2ms: %u\n", iIrqLatencyCount);

    for (i = 0; i < KEPT_IRQ_LATENCY_SIZE; i++)
    {
        cost = kept_irq_latency[i].irq_info.timeEnd - kept_irq_latency[i].irq_info.timeStart;

        counter += 1;

        if (kept_irq_latency[i].irq_info.IRQNumber <= 32)
            break;

        printk("No: %d, IRQ: %d, cost: %lu us\n", counter, kept_irq_latency[i].irq_info.IRQNumber,
               (unsigned long)cost >> 10);
    }
}

void sstar_irq_record_large_latency_in_top(SSTAR_IRQ_INFO *irq_info)
{
    unsigned int i = 0;

    spin_lock(&irq_latency_lock);
    i = iIrqLatencyCount % KEPT_IRQ_LATENCY_SIZE;
    iIrqLatencyCount++;
    memcpy(&kept_irq_latency[i].irq_info, irq_info, sizeof(SSTAR_IRQ_INFO));
    spin_unlock(&irq_latency_lock);
}

/*
 * This API is for recording hard irq.
 * Could be used just in hard irq context.
 */
void sstar_irq_records(SSTAR_IRQ_INFO *irq_info)
{
    struct SSTAR_IRQ_INFO_LIST *new;
    spin_lock(&irq_lock);

    if (IRQ_COUNTER < IRQ_LIST)
    {
        new = (struct SSTAR_IRQ_INFO_LIST *)kmalloc(sizeof(struct SSTAR_IRQ_INFO_LIST),
                                                    GFP_ATOMIC); // need avoid sleep here

        if (new != NULL)
        {
            memcpy(&new->irq_info, irq_info, sizeof(SSTAR_IRQ_INFO));
            list_add(&new->list, &kept_irq_head);
            IRQ_COUNTER += 1;
        }
    }

    spin_unlock(&irq_lock);
}

/*
 * This API is for recording soft irq.
 * Could be used just in soft irq context.
 */
void sstar_sirq_records(SSTAR_IRQ_INFO *irq_info)
{
    struct SSTAR_IRQ_INFO_LIST *new;
    unsigned long flags;

    spin_lock_irqsave(&irq_lock, flags);

    if (SIRQ_COUNTER < IRQ_LIST)
    {
        new = (struct SSTAR_IRQ_INFO_LIST *)kmalloc(sizeof(struct SSTAR_IRQ_INFO_LIST), GFP_ATOMIC);

        if (new != NULL)
        {
            memcpy(&new->irq_info, irq_info, sizeof(SSTAR_IRQ_INFO));
            list_add(&new->list, &kept_sirq_head);
            SIRQ_COUNTER += 1;
        }
    }

    spin_unlock_irqrestore(&irq_lock, flags);
}

static void sstar_irqoff_dump_info(struct seq_file *s)
{
    struct list_head *         ptr;
    struct SSTAR_IRQ_OFF_LIST *entry;
    unsigned long long         start;
    unsigned long long         endtime;
    unsigned long long         cost;
    int                        i = 0;

    seq_printf(s, "IRQOFF_NR: %d \n", IRQOFF_COUNTER);

    list_for_each(ptr, &kept_irqOff_head)
    {
        entry = list_entry(ptr, struct SSTAR_IRQ_OFF_LIST, irqoff_list);
        seq_printf(s, "CPU: %d \n", entry->irqoff_info.cpuid);
        cost    = (entry->irqoff_info.timeEnd - entry->irqoff_info.timeStart) / 1000000;
        start   = (entry->irqoff_info.timeStart / 1000000);
        endtime = (entry->irqoff_info.timeEnd / 1000000);

        seq_printf(s, "COMMAND: %s PID: %d LATENCY: %llu(ms) TIMESTEMP: %llu~%llu (ms)\n", entry->irqoff_info.comms,
                   entry->irqoff_info.pids, cost, start, endtime);

        for (i = 0; i < PER_TRACE_ENTRIES_AVERAGE; i++)
        {
            seq_printf(s, "    ");
            seq_printf(s, "[<%p>] %pS\n", (void *)entry->irqoff_info.entries[i], (void *)entry->irqoff_info.entries[i]);
        }
    }
}

void sstar_irq_dump_info(void)
{
    struct list_head *          ptr;
    struct SSTAR_IRQ_INFO_LIST *entry;
    unsigned long               nanosec_rem     = 0;
    unsigned long               nanosec_end_rem = 0;
    unsigned long long          start           = 0;
    unsigned long long          end             = 0;
    unsigned long               cost;

    int counter = 0;
    printk("IRQ_COUNTER:%d \n", IRQ_COUNTER);

    list_for_each(ptr, &kept_irq_head)
    {
        entry           = list_entry(ptr, struct SSTAR_IRQ_INFO_LIST, list);
        start           = entry->irq_info.timeStart;
        end             = entry->irq_info.timeEnd;
        cost            = (unsigned long)((entry->irq_info.timeEnd - entry->irq_info.timeStart) / 1000000);
        nanosec_rem     = do_div(start, 1000000000);
        nanosec_end_rem = do_div(end, 1000000000);
        counter += 1;
        printk("No: %03d, IRQ: %02d, cost: (%lu)ms, timestamp: (%5lu.%06lu) - (%5lu.%06lu) \n", counter,
               entry->irq_info.IRQNumber, (unsigned long)cost, (unsigned long)start, nanosec_rem / 1000,
               (unsigned long)end, nanosec_end_rem / 1000);
    }
}

void sstar_sirq_dump_info(void)
{
    struct list_head *          ptr;
    struct SSTAR_IRQ_INFO_LIST *entry;
    unsigned long               nanosec_rem     = 0;
    unsigned long               nanosec_end_rem = 0;
    unsigned long long          start           = 0;
    unsigned long long          end             = 0;
    unsigned long               cost;

    int counter = 0;
    printk("Soft IRQ_COUNTER:%d \n", SIRQ_COUNTER);

    list_for_each(ptr, &kept_sirq_head)
    {
        entry = list_entry(ptr, struct SSTAR_IRQ_INFO_LIST, list);
        cost  = entry->irq_info.timeEnd - (unsigned long)entry->irq_info.timeStart;
        start = entry->irq_info.timeStart;
        end   = entry->irq_info.timeEnd;

        nanosec_rem     = do_div(start, 1000000000);
        nanosec_end_rem = do_div(end, 1000000000);
        counter += 1;
        printk("No: %03d, vec_nr: %02d, cost: %lu ns, action: %p\n", counter, entry->irq_info.IRQNumber,
               (unsigned long)cost, entry->irq_info.action);
    }
}

static ssize_t threshold_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "Set IRQ_OFF threshold %d(ms)\n", irqOffTH / 1000000);
    str += scnprintf(str, end - str, "Set SIRQ_OFF threshold %d(ms)\n", sirqOffTH / 1000000);
    str += scnprintf(str, end - str, "Set IRQ_TS threshold %d(ms)\n", irqTH / 1000000);
    str += scnprintf(str, end - str, "Set SIRQ_TS threshold %d(ms)\n", sirqTH / 1000000);
    return (str - buf);
}

static ssize_t threshold_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    unsigned int  threshold = 0;
    unsigned char token[16];
    sscanf(buf, "%s", token);
    //    IRQ_OFF: io IRQ_TS: it SIRQ_TS: st SIRQ_OFF: so

    if (0 == strcasecmp(token, "io"))
    {
        sscanf(buf, "%s %d ", token, &threshold);
        irqOffTH = threshold * 1000000;
        printk("Set IRQ_OFF threshold %d(ms)\n", threshold);
    }

    else if (0 == strcasecmp(token, "st"))
    {
        sscanf(buf, "%s %d ", token, &threshold);
        sirqTH = threshold * 1000000;
        printk("Set SIRQ_TS threshold %d(ms)\n", threshold);
    }

    else if (0 == strcasecmp(token, "it"))
    {
        sscanf(buf, "%s %d ", token, &threshold);
        irqTH = threshold * 1000000;
        printk("Set IRQ_TS threshold %d(ms)\n", threshold);
    }

    else if (0 == strcasecmp(token, "so"))
    {
        sscanf(buf, "%s %d ", token, &threshold);
        sirqOffTH = threshold * 1000000;
        printk("Set SIRQ_OFF threshold %d(ms)\n", threshold);
    }
    else
    {
        printk("echo [op] [ms] > /sys/class/mstar/irq/threshold \r\n");
        printk("set irq off more than 3 ms \r\n");
        printk("echo io 3 > /sys/class/mstar/irq/threshold \r\n");
        printk("IRQ_OFF: io IRQ_TS: it SIRQ_TS: st SIRQ_OFF: so \r\n");
    }
    return (n);
}

static ssize_t irqoff_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int   i   = 0;

    for (i = 0; i < NR_CPUS; i++)
    {
        str += scnprintf(str, end - str, "cpu:[%d] irqoff_enable %d\n", i, irqoff_enable[i]);
    }

    return (str - buf);
}

static ssize_t irqoff_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    unsigned int irqoffEnable = 0, i = 0;
    sscanf(buf, "%d ", &irqoffEnable);

    if ((0 == irqoffEnable) || (1 == irqoffEnable))
    {
        printk("irq %d \n", irqoffEnable);
    }
    else
    {
        printk("echo 1 > irq_enable \n");
    }

    for (i = 0; i < NR_CPUS; i++)
    {
        irqoff_enable[i] = irqoffEnable;
        irqoffCnt[i]     = 0;
    }
    return n;
}

static ssize_t irq_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "irq_enable %d\n", irqts_enable);

    return (str - buf);
}

static ssize_t irq_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    unsigned int irqTimeSpentOn = 0;
    sscanf(buf, "%d ", &irqTimeSpentOn);

    if ((0 == irqTimeSpentOn) || (1 == irqTimeSpentOn))
    {
        printk("irq %d \n", irqTimeSpentOn);
    }
    else
    {
        printk("echo 1 > irq_enable \n");
    }
    irqts_enable = irqTimeSpentOn;
    return n;
}

static ssize_t irq_off_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "!! cat /proc/irq_detect/irq_off !!\n");

    return (str - buf);
}

static ssize_t irq_off(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    unsigned int               clear     = 0;
    int                        i         = 0;
    struct SSTAR_IRQ_OFF_LIST *old_entry = NULL;
    struct list_head *         ptr;

    sscanf(buf, "%d ", &clear);

    if (clear == 1)
    {
        printk("clear \n");

        for (i = 0; i <= IRQOFF_COUNTER; i++)
        {
            list_for_each_prev(ptr, &kept_irqOff_head)
            {
                old_entry = list_entry(ptr, struct SSTAR_IRQ_OFF_LIST, irqoff_list);
                list_del_init(&old_entry->irqoff_list);
                kfree(old_entry);

                if (list_empty(&old_entry->irqoff_list))
                    break;
            }
        }
        IRQOFF_COUNTER = 0;
    }
    else
    {
        printk("echo 1 > irq_off to clear log \n");
    }
    return n;
}

static ssize_t irq_timespent_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "Dump irq timespent list \n");
    sstar_irq_dump_info();

    return (str - buf);
}

static ssize_t irq_timespent(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    unsigned int                clear, i = 0;
    struct SSTAR_IRQ_INFO_LIST *old_entry = NULL;
    struct list_head *          ptr;
    sscanf(buf, "%d ", &clear);

    if (clear == 1)
    {
        printk("clear \n");

        for (i = 0; i <= IRQ_COUNTER; i++)
        {
            list_for_each_prev(ptr, &kept_irq_head)
            {
                old_entry = list_entry(ptr, struct SSTAR_IRQ_INFO_LIST, list);
                list_del_init(&old_entry->list);
                kfree(old_entry);

                if (list_empty(&old_entry->list))
                    break;
            }
        }
        IRQ_COUNTER = 0;
    }
    else
    {
        printk("echo 1 > irq_timespent to clear log \n");
    }
    return n;
}

static ssize_t sirq_off_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "Dump sirq off list \n");

    return (str - buf);
}

static ssize_t sirq_off(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    unsigned int clear = 0;
    sscanf(buf, "%d ", &clear);

    if (clear == 1)
    {
        printk("clear \n");
    }
    else
    {
        printk("echo 1 > sirq_off to clear log \n");
    }
    return n;
}

static ssize_t sirq_timespent_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "Dump sirq timespent list \n");
    sstar_sirq_dump_info();

    return (str - buf);
}

static ssize_t sirq_timespent(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    unsigned int                clear     = 0, i;
    struct SSTAR_IRQ_INFO_LIST *old_entry = NULL;
    struct list_head *          ptr;
    sscanf(buf, "%d ", &clear);

    if (clear == 1)
    {
        printk("clear \n");

        for (i = 0; i <= SIRQ_COUNTER; i++)
        {
            list_for_each_prev(ptr, &kept_sirq_head)
            {
                old_entry = list_entry(ptr, struct SSTAR_IRQ_INFO_LIST, list);
                list_del_init(&old_entry->list);
                kfree(old_entry);

                if (list_empty(&old_entry->list))
                    break;
            }
        }
    }
    else
    {
        printk("echo 1 > sirq_timespent to clear log \n");
    }
    return n;
}

static ssize_t irqoff_hint(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    unsigned int irqoffHintEn = 0;
    sscanf(buf, "%d ", &irqoffHintEn);

    if ((0 == irqoffHintEn) || (1 == irqoffHintEn))
    {
        printk("irqoff_hint %d \n", irqoffHintEn);
    }
    else
    {
        printk("echo 1 > irqoff_hint \n");
    }
    irqoffHint = irqoffHintEn;
    return n;
}

static ssize_t irqoff_hint_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "irq_enable %d\n", irqts_enable);
    return (str - buf);
}

extern struct class *msys_get_sysfs_class(void);

DEVICE_ATTR(sirq_timespent, 0644, sirq_timespent_show, sirq_timespent);
DEVICE_ATTR(threshold, 0644, threshold_show, threshold_set);
DEVICE_ATTR(sirqoff, 0644, sirq_off_show, sirq_off);
DEVICE_ATTR(irqoff, 0644, irq_off_show, irq_off);
DEVICE_ATTR(irq_timespent, 0644, irq_timespent_show, irq_timespent);
DEVICE_ATTR(irqoff_enable, 0644, irqoff_enable_show, irqoff_set);
DEVICE_ATTR(irqoff_hint, 0644, irqoff_hint_show, irqoff_hint);
DEVICE_ATTR(irqts_enable, 0644, irq_enable_show, irq_set);

#if IRQ_TRACE_TEST
DEVICE_ATTR(irqdebug, 0644, irq_trace_test_show, irq_trace_test_set);
#endif

static int irq_off_dump(struct seq_file *seq, void *offset)
{
    sstar_irqoff_dump_info(seq);
    return 0;
}

static int irq_off_Open(struct inode *inode, struct file *file)
{
    return single_open(file, irq_off_dump, NULL);
}

static const struct proc_ops irqoff_status_fops = {
    .proc_open = irq_off_Open, .proc_read = seq_read, .proc_lseek = seq_lseek, .proc_release = seq_release};

static char            irqoff_proc_name[1][16] = {[0] = {"irq_off"}};
struct proc_dir_entry *proc_reg_dir            = NULL;
#define IRQ_PROC_DIR "irq_detect"
static struct proc_dir_entry *proc_entry[1] = {NULL};

int sstar_irq_stat_init(void)
{
    sirq_head_initialized = 1;
    INIT_LIST_HEAD(&kept_irq_head);
    INIT_LIST_HEAD(&kept_sirq_head);
    INIT_LIST_HEAD(&kept_irqOff_head);

    girqdebug_class_dev = device_create(msys_get_sysfs_class(), NULL, MKDEV(241, 2), NULL, "irq");
    device_create_file(girqdebug_class_dev, &dev_attr_irq_timespent);
    device_create_file(girqdebug_class_dev, &dev_attr_irqoff);
    device_create_file(girqdebug_class_dev, &dev_attr_sirq_timespent);
    device_create_file(girqdebug_class_dev, &dev_attr_sirqoff);
    device_create_file(girqdebug_class_dev, &dev_attr_irqts_enable);
    device_create_file(girqdebug_class_dev, &dev_attr_irqoff_enable);
    device_create_file(girqdebug_class_dev, &dev_attr_threshold);
    device_create_file(girqdebug_class_dev, &dev_attr_irqoff_hint);
#if IRQ_TRACE_TEST
    device_create_file(girqdebug_class_dev, &dev_attr_irqdebug);
#endif
    if (!proc_reg_dir)
        proc_reg_dir = proc_mkdir(IRQ_PROC_DIR, NULL);

    proc_entry[0] = proc_create(irqoff_proc_name[0], 0, proc_reg_dir, &irqoff_status_fops);
    if (!proc_entry[0])
        printk("!! FAIL to create %s PROC !!\n", irqoff_proc_name[0]);
    return 0;
}

device_initcall(sstar_irq_stat_init);
