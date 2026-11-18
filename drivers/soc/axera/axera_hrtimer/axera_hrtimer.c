/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/of_platform.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/list.h>
#include <linux/rbtree.h>
#include <linux/miscdevice.h>
#include <linux/timer.h>
#include <linux/freezer.h>
#include <linux/delay.h>
#include <linux/math64.h>
//#include <vdso/time64.h>
//#include <linux/timer64.h>
#include <linux/time64.h>
#include <uapi/linux/time.h>
#include <linux/ax_hrtimer.h>
#include <linux/clk.h>
#include <linux/reset.h>

#include <linux/delay.h>
#include <linux/math64.h>
//#include <vdso/time64.h>
#include <uapi/linux/time.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/sched/signal.h>
#include <asm-generic/div64.h>

#define TIME64_MIN			(-TIME64_MAX - 1)
#define KTIME_MAX			((s64)~((u64)1 << 63))
#define KTIME_MIN			(-KTIME_MAX - 1)
#define KTIME_SEC_MAX			(KTIME_MAX / NSEC_PER_SEC)
#define KTIME_SEC_MIN			(KTIME_MIN / NSEC_PER_SEC)

#define APBT_MIN_PERIOD         4
#define APBT_MIN_DELTA_USEC     200
#define APBTMR_N_LOAD_COUNT     0x00
#define APBTMR_N_CURRENT_VALUE      0x04
#define APBTMR_N_CONTROL        0x08
#define APBTMR_N_EOI            0x0c
#define APBTMR_N_INT_STATUS     0x10

#define APBTMRS_INT_STATUS      0xa0
#define APBTMRS_EOI         0xa4
#define APBTMRS_RAW_INT_STATUS      0xa8
#define APBTMRS_COMP_VERSION        0xac

#define APBTMR_CONTROL_ENABLE       (1 << 0)
/* 1: periodic, 0:free running. */
#define APBTMR_CONTROL_MODE_PERIODIC    (1 << 1)
#define APBTMR_CONTROL_INT      (1 << 2)

#define AX_HRTIMER_IOC_MAGIC  'u'
#define AX_HRTIMER_USLEEP    _IOWR(AX_HRTIMER_IOC_MAGIC, 1, unsigned long*)
#define HZ_PER_USEC     24
#define RETRY -1
#define MIN_TIME_DELAY 10
#define TASK_AWAKENED  1
#define TASK_SLEEP  0


#define  AX_HRTIMER_NORESTART  0      /* Timer is not restarted */
#define  AX_HRTIMER_RESTART  1        /* Timer must be restarted */

struct axera_dw_apb_timer {
	void __iomem *base;
	int irq;
	void (*eoi) (void);
	void *arg;
	int timer_status;
	spinlock_t info_lock;
	raw_spinlock_t ax_hrtimer_lock;
	atomic_t spinlock_count;
	struct reset_control *preset;
	struct reset_control *reset;
	struct clk *pclk;
	struct clk *clk;
};

static struct axera_dw_apb_timer *dw_timer;
static struct rb_root ax_hrtimer_rbroot = RB_ROOT;

void ax_raw_spin_lock_irqsave(raw_spinlock_t *lock, unsigned long *flags)
{
	unsigned long tmp;
	raw_spin_lock_irqsave(lock,tmp);
	*flags = tmp;
	atomic_inc(&dw_timer->spinlock_count);
	if (atomic_read(&dw_timer->spinlock_count) > 1) {
	//	printk("error ax apb timer spinlock nesting\n");
	}
}
void ax_raw_spin_unlock_irqrestore(raw_spinlock_t *lock, unsigned long *flags)
{
	unsigned long tmp;
	tmp = *flags;
	raw_spin_unlock_irqrestore(lock, tmp);
	atomic_dec(&dw_timer->spinlock_count);
}

static inline s64 ax_timespec64_to_us(void)
{
	struct timespec64 ts;
	u64 tm;
	ktime_get_real_ts64(&ts);
	/* Prevent multiplication overflow / underflow */
	if (ts.tv_sec >= KTIME_SEC_MAX)
		return KTIME_MAX;

	if (ts.tv_sec <= KTIME_SEC_MIN)
		return KTIME_MIN;
	tm = (ts.tv_sec * NSEC_PER_SEC) + ts.tv_nsec;
	do_div(tm, NSEC_PER_USEC);
	return tm;
}

struct ax_hrtimer *ax_hrtimer_rb_search(struct rb_root *root, int new)
{
	struct rb_node *node = root->rb_node;

	while (node) {
		struct ax_hrtimer *timer = container_of(node, struct ax_hrtimer, node);
		if (timer->delay > new)
			node = node->rb_left;
		else if (timer->delay < new)
			node = node->rb_right;
		else
			return timer;
	}
	return NULL;
}


static int ax_hrtimer_rb_insert(struct rb_root *root, struct ax_hrtimer *timer)
{

	struct rb_node **new = &(root->rb_node), *parent = NULL;

	/* Figure out where to put new node */
	while (*new) {
		struct ax_hrtimer *this = container_of(*new, struct ax_hrtimer, node);
		parent = *new;
		if (this->delay > timer->delay)
			new = &((*new)->rb_left);
		else if (this->delay < timer->delay)
			new = &((*new)->rb_right);
		else
			return -1;
	}

	/* Add new node and rebalance tree. */
	rb_link_node(&timer->node, parent, new);
	rb_insert_color(&timer->node, root);

	return 0;
}

static int ax_hrtimer_rb_erase(struct ax_hrtimer *timer)
{
	rb_erase(&timer->node, &ax_hrtimer_rbroot);
	return 0;
}

static void dw_apb_timer_disable(void)
{
	u32 ctrl = readl(dw_timer->base + APBTMR_N_CONTROL);
	ctrl |= APBTMR_CONTROL_INT;
	writel(ctrl, dw_timer->base + APBTMR_N_CONTROL);
}

void dw_apb_timer_timerpause(void)
{
	disable_irq(dw_timer->irq);
	dw_apb_timer_disable();
}

static void dw_apb_timer_eoi(void)
{
	readl_relaxed(dw_timer->base + APBTMR_N_EOI);
}

static int dw_apb_timer_start(unsigned long delta)
{
	u32 ctrl;
	/* Disable timer */
	ctrl = readl_relaxed(dw_timer->base + APBTMR_N_CONTROL);
	ctrl &= ~APBTMR_CONTROL_ENABLE;
	writel_relaxed(ctrl, dw_timer->base + APBTMR_N_CONTROL);
	/* write new count */
	writel_relaxed(delta, dw_timer->base + APBTMR_N_LOAD_COUNT);
	ctrl |= APBTMR_CONTROL_ENABLE;
	writel_relaxed(ctrl, dw_timer->base + APBTMR_N_CONTROL);
	/* enable, mask interrupt */
	ctrl &= ~APBTMR_CONTROL_MODE_PERIODIC;
	ctrl &= ~APBTMR_CONTROL_INT;
	ctrl |= APBTMR_CONTROL_ENABLE;
	writel(ctrl, dw_timer->base + APBTMR_N_CONTROL);
	smp_mb();
	return 0;
}

static void dw_apb_timer_end(void)
{
	/*
	 * start count down from 0xffff_ffff. this is done by toggling the
	 * enable bit then load initial load count to ~0.
	 */
	u32 ctrl = readl(dw_timer->base + APBTMR_N_CONTROL);
	ctrl &= ~APBTMR_CONTROL_ENABLE;
	writel(ctrl, dw_timer->base + APBTMR_N_CONTROL);
	smp_mb();
}

struct ax_hrtimer *ax_hrtimer_alloc(void)
{
	struct ax_hrtimer *timer;
	timer = kmalloc(sizeof(struct ax_hrtimer), GFP_ATOMIC);
	if (timer == NULL)
		return NULL;
	memset(timer, 0, sizeof(struct ax_hrtimer));
	return timer;
}
EXPORT_SYMBOL(ax_hrtimer_alloc);

static int ax_hrtimer_init(struct ax_hrtimer *timer)
{
	return ax_hrtimer_rb_insert(&ax_hrtimer_rbroot, timer);
}

static void ax_dw_apb_timer_start(struct ax_hrtimer *timer)
{
	dw_timer->arg = timer;
	dw_apb_timer_start(HZ_PER_USEC * timer->delay);
}

static void ax_hrtimer_cancel(struct ax_hrtimer *timer)
{
	do {
		if (atomic_read(&timer->timer_wake) == 1)
			return;
		else {
			udelay(10);
		}
	} while (!atomic_read(&timer->timer_wake));
}

void ax_hrtimer_destroy(struct ax_hrtimer *timer)
{
	if(IS_ERR_OR_NULL(timer)) {
		return;
	}
	kfree(timer);
}

EXPORT_SYMBOL(ax_hrtimer_destroy);


static long ax_hrtimer_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct ax_hrtimer *ptimer, *last_timer;
	unsigned long time_udelay = 0;
	unsigned long flags;
	unsigned long current_delay_us;
	unsigned long current_task_delay;
	s64 now;
	struct rb_node *node;
	void __user *argp = (void __user *)arg;
	if (copy_from_user(&time_udelay, argp, sizeof(time_udelay)))
		return -EFAULT;

	/*if (time_udelay < MIN_TIME_DELAY)
	   return -EINVAL; */

	switch (cmd) {
	case AX_HRTIMER_USLEEP:
		now = ax_timespec64_to_us();
		ptimer = ax_hrtimer_alloc();
		if (!ptimer)
			return -1;
		ptimer->task = current;
		ptimer->function = NULL;
		ptimer->delay = time_udelay;
		ptimer->start_time = now;
		ptimer->expires = time_udelay;
		atomic_set(&ptimer->timer_wake, TASK_SLEEP);
		ax_raw_spin_lock_irqsave(&dw_timer->ax_hrtimer_lock, &flags);

		node = rb_first(&ax_hrtimer_rbroot);
		if (node == NULL) {
			if (ax_hrtimer_init(ptimer) == RETRY) {
				printk(KERN_ERR "ax_hrtimer_init logic error %d\n", __LINE__);
			}
			ax_dw_apb_timer_start(ptimer);
		} else {
			last_timer = rb_entry(node, struct ax_hrtimer, node);
			current_delay_us = now - last_timer->start_time;
			if (current_delay_us >= last_timer->expires) {
				while (ax_hrtimer_init(ptimer) == RETRY) {
					ptimer->delay += 1;
				}
			} else if (current_delay_us < last_timer->expires) {
				current_task_delay = last_timer->expires - current_delay_us;
				if (time_udelay >= current_task_delay) {
					if (time_udelay == current_task_delay)
						ptimer->delay += 1;
					while (ax_hrtimer_init(ptimer) == RETRY) {
						ptimer->delay += 1;
					}
				} else if (time_udelay < current_task_delay) {
					dw_apb_timer_end();
					while (ax_hrtimer_init(ptimer) == RETRY) {
						if (ptimer->delay == 0) {
							kfree(ptimer);
							ax_dw_apb_timer_start(last_timer);
							ax_raw_spin_unlock_irqrestore(&dw_timer->ax_hrtimer_lock, &flags);
							return 0;
						} else {
							ptimer->delay -= 1;
                                        	};
					}
					ax_dw_apb_timer_start(ptimer);
				}
			}
		}

		__set_current_state(TASK_INTERRUPTIBLE);
		ax_raw_spin_unlock_irqrestore(&dw_timer->ax_hrtimer_lock, &flags);

		do {
			if (likely(ptimer->task))
				freezable_schedule();

			ax_hrtimer_cancel(ptimer);

		} while (ptimer->task && !signal_pending(current));
		__set_current_state(TASK_RUNNING);
		kfree(ptimer);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

long ax_usleep(unsigned long use)
{
	struct ax_hrtimer *ptimer, *last_timer;
	unsigned long time_udelay = 0;
	unsigned long flags;
	unsigned long current_delay_us;
	unsigned long current_task_delay;
	s64 now;
	struct rb_node *node;

	time_udelay = use;
	BUG_ON(in_interrupt());
	now = ax_timespec64_to_us();
	ptimer = ax_hrtimer_alloc();
	if (!ptimer)
		return -1;
	ptimer->task = current;
	ptimer->function = NULL;
	ptimer->delay = time_udelay;
	ptimer->start_time = now;
	ptimer->expires = time_udelay;
	atomic_set(&ptimer->timer_wake, TASK_SLEEP);
	ax_raw_spin_lock_irqsave(&dw_timer->ax_hrtimer_lock, &flags);

	node = rb_first(&ax_hrtimer_rbroot);
	if (node == NULL) {
		if (ax_hrtimer_init(ptimer) == RETRY) {
			printk(KERN_ERR "ax_hrtimer_init logic error %d\n", __LINE__);
		}
		ax_dw_apb_timer_start(ptimer);
	} else {
		last_timer = rb_entry(node, struct ax_hrtimer, node);
		current_delay_us = now - last_timer->start_time;
		if (current_delay_us >= last_timer->expires) {
			while (ax_hrtimer_init(ptimer) == RETRY) {
				ptimer->delay += 1;
			}
		} else if (current_delay_us < last_timer->expires) {
			current_task_delay = last_timer->expires - current_delay_us;
			if (time_udelay >= current_task_delay) {
				if (time_udelay == current_task_delay)
					ptimer->delay += 1;

				while (ax_hrtimer_init(ptimer) == RETRY) {
					ptimer->delay += 1;
				}
			} else if (time_udelay < current_task_delay) {
				dw_apb_timer_end();
				while (ax_hrtimer_init(ptimer) == RETRY) {
					ptimer->delay -= 1;
				}
				ax_dw_apb_timer_start(ptimer);
			}
		}
	}

	__set_current_state(TASK_INTERRUPTIBLE);
	ax_raw_spin_unlock_irqrestore(&dw_timer->ax_hrtimer_lock, &flags);

	do {
		if (likely(ptimer->task))
			freezable_schedule();

		ax_hrtimer_cancel(ptimer);

	} while (ptimer->task && !signal_pending(current));
	__set_current_state(TASK_RUNNING);
	kfree(ptimer);

	return 0;
}

EXPORT_SYMBOL(ax_usleep);

int ax_hrtimer_start(struct ax_hrtimer *timer)
{
	unsigned long flags;
	struct ax_hrtimer *last_timer;
	unsigned long time_udelay = 0;
	unsigned long current_delay_us;
	unsigned long current_task_delay;
	s64 now;
	struct rb_node *node;
	now = ax_timespec64_to_us();
	if(timer->expires != 0) {
		timer->delay = timer->expires;
        } else {
		timer->expires = timer->delay;
        }

	time_udelay = timer->expires;
	timer->task = NULL;
	timer->start_time = now;

	if(IS_ERR_OR_NULL(timer)) {
		return -1;
	}

	if((timer->function != NULL) && (timer->task != NULL)) {
		return -1;
	}

	ax_raw_spin_lock_irqsave(&dw_timer->ax_hrtimer_lock, &flags);

	node = rb_first(&ax_hrtimer_rbroot);
	if (node == NULL) {
		if (ax_hrtimer_init(timer) == RETRY) {
			printk(KERN_ERR "ax_hrtimer_init logic error %d\n", __LINE__);
		}
		ax_dw_apb_timer_start(timer);
	} else {
		last_timer = rb_entry(node, struct ax_hrtimer, node);
		current_delay_us = now - last_timer->start_time;
		if (current_delay_us >= last_timer->expires) {
			while (ax_hrtimer_init(timer) == RETRY) {
				timer->delay += 1;
			}
		} else if (current_delay_us < last_timer->expires) {
			current_task_delay = last_timer->expires - current_delay_us;
			if (time_udelay >= current_task_delay) {
				if (time_udelay == current_task_delay)
					timer->delay += 1;

				while (ax_hrtimer_init(timer) == RETRY) {
					timer->delay += 1;
				}
			} else if (time_udelay < current_task_delay) {
				dw_apb_timer_end();
				while (ax_hrtimer_init(timer) == RETRY) {
					timer->delay -= 1;
				}
				ax_dw_apb_timer_start(timer);
			}
		}
	}
	ax_raw_spin_unlock_irqrestore(&dw_timer->ax_hrtimer_lock, &flags);
	return 0;

}

EXPORT_SYMBOL(ax_hrtimer_start);

int ax_hrtimer_stop(struct ax_hrtimer *timer)
{
	unsigned long flags;
	struct ax_hrtimer *current_timer;
	struct ax_hrtimer *this_timer = NULL;
	s64 now;
	struct rb_node *node;
	unsigned long current_delay_us;

	ax_raw_spin_lock_irqsave(&dw_timer->ax_hrtimer_lock, &flags);

	node = rb_first(&ax_hrtimer_rbroot);
	for (; node != NULL; ) {
		this_timer = rb_entry(node, struct ax_hrtimer, node);
		node = rb_next(node);
		if (this_timer == timer) {
			break;
		} else {
			this_timer = NULL;
		}
	}

	if(this_timer == NULL) {
		printk(KERN_ERR "ax_hrtimer_stop not found timer %d\n", __LINE__);
		ax_raw_spin_unlock_irqrestore(&dw_timer->ax_hrtimer_lock, &flags);
		return -1;
	}


	if (dw_timer->arg != timer) {
		rb_erase(&timer->node, &ax_hrtimer_rbroot);
	} else if (dw_timer->arg == timer) {

		dw_apb_timer_end();
		rb_erase(&timer->node, &ax_hrtimer_rbroot);
		node = rb_first(&ax_hrtimer_rbroot);
		if (node == NULL) {
			ax_raw_spin_unlock_irqrestore(&dw_timer->ax_hrtimer_lock, &flags);
			return 0;// rb tree is empty
		}
		current_timer = rb_entry(node, struct ax_hrtimer, node);
		now = ax_timespec64_to_us();
		current_delay_us = now - current_timer->start_time;
		if (current_timer->expires > current_delay_us) {
			current_timer->delay = current_timer->expires - current_delay_us;
			ax_dw_apb_timer_start(current_timer);
		} else if (current_timer->expires <= current_delay_us) {
				/*Simply deal with some penalties for the current timer,
				 *otherwise you will have to continue traversing the rb trees
				 */
				current_timer->expires =+50;
				current_timer->delay = 50;//50 us
				ax_dw_apb_timer_start(current_timer);
		}
	}
	ax_raw_spin_unlock_irqrestore(&dw_timer->ax_hrtimer_lock, &flags);
	return 0;
}

EXPORT_SYMBOL(ax_hrtimer_stop);



static const struct file_operations ax_hrtimer_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = ax_hrtimer_ioctl,
};

static struct miscdevice ax_hrtimer_miscdev = {
	.name = "ax_hrtimer",
	.minor = MISC_DYNAMIC_MINOR,
	.fops = &ax_hrtimer_fops,
};

static irqreturn_t dw_apb_timer_irq(int irq, void *data)
{
	struct rb_node *node;
	unsigned long flags;
	s64 now;
	struct ax_hrtimer *timer,*next_timer;
	unsigned long current_delay_us;
	struct task_struct *task = NULL;

	timer = (struct ax_hrtimer *)dw_timer->arg;
	if (timer == NULL) {
		printk(KERN_ERR "ax hrtimer logic error\n");
		return IRQ_HANDLED;
	}

	disable_irq_nosync(dw_timer->irq);

	ax_raw_spin_lock_irqsave(&dw_timer->ax_hrtimer_lock, &flags);
	dw_apb_timer_end();
	if (dw_timer->eoi)
		dw_timer->eoi();

	task = timer->task;
	if (task) {
		ax_hrtimer_rb_erase(timer);
		timer->task = NULL;
		atomic_inc(&timer->timer_wake);
		wake_up_process(task);
	} else if (timer->function) {
		if(AX_HRTIMER_NORESTART == timer->function(timer->private)){
			rb_erase(&timer->node, &ax_hrtimer_rbroot);
		} else {//restart mode
			now = ax_timespec64_to_us();
			timer->delay = timer->expires;
			timer->task = NULL;
			timer->start_time = now;
		}
	}

	node = rb_first(&ax_hrtimer_rbroot);
	for (; node; ) {
		timer = rb_entry(node, struct ax_hrtimer, node);
		node = rb_next(node);
		now = ax_timespec64_to_us();
		current_delay_us = now - timer->start_time;
		if (timer->expires > current_delay_us) {
			timer->delay = timer->expires - current_delay_us;
		} else if (timer->expires <= current_delay_us) {
			task = timer->task;
			if (task) {
				rb_erase(&timer->node, &ax_hrtimer_rbroot);
				timer->task = NULL;
				atomic_inc(&timer->timer_wake);
				wake_up_process(task);
			} else if (timer->function) {
				if (AX_HRTIMER_NORESTART == timer->function(timer->private)) {
					rb_erase(&timer->node, &ax_hrtimer_rbroot);
				} else { //restart mode
					now = ax_timespec64_to_us();
					timer->delay = timer->expires;
					timer->task = NULL;
					timer->start_time = now;
				}
			}
		}
	}

	node = rb_first(&ax_hrtimer_rbroot);
	if (node != NULL) {
		next_timer = rb_entry(node, struct ax_hrtimer, node);
		if (next_timer != NULL) {
			ax_dw_apb_timer_start(next_timer);
		} else {
			ax_hrtimer_rbroot = RB_ROOT;
                }
	}
	ax_raw_spin_unlock_irqrestore(&dw_timer->ax_hrtimer_lock, &flags);
	enable_irq(dw_timer->irq);
	return IRQ_HANDLED;
}

static int axera_dw_timer_probe(struct platform_device *pdev)
{
	int err;
	int ret;
	dw_timer = devm_kzalloc(&pdev->dev, sizeof(*dw_timer), GFP_KERNEL);
	if (!dw_timer)
		return -ENOMEM;

	dw_timer->base = devm_ioremap_resource(&pdev->dev, platform_get_resource(pdev, IORESOURCE_MEM, 0));
	if (IS_ERR(dw_timer->base))
		return PTR_ERR(dw_timer->base);

	dw_timer->irq = platform_get_irq(pdev, 0);
	if (dw_timer->irq < 0) {
		pr_err("failed to request timer irq\n");
		return dw_timer->irq;
	}
	dw_timer->eoi = dw_apb_timer_eoi;
	err = devm_request_irq(&pdev->dev, dw_timer->irq, dw_apb_timer_irq, IRQF_SHARED, KBUILD_MODNAME, dw_timer);
	if (err) {
		return -ENOMEM;
	}

	dw_timer->preset = devm_reset_control_get_optional(&pdev->dev, "preset");
	if (IS_ERR_OR_NULL(dw_timer->preset)) {
		pr_err("ax get global reset failed\n");
		return PTR_ERR(dw_timer->preset);
	}

	reset_control_deassert(dw_timer->preset);

	dw_timer->reset = devm_reset_control_get_optional(&pdev->dev, "reset");
	if (IS_ERR_OR_NULL(dw_timer->reset)) {
		pr_err("ax get reset failed\n");
		return PTR_ERR(dw_timer->reset);
	}

	reset_control_deassert(dw_timer->reset);

	dw_timer->pclk = devm_clk_get(&pdev->dev, "pclk");

	if (IS_ERR_OR_NULL(dw_timer->pclk)) {
		pr_err("ax get pclk fail\n");
		return PTR_ERR(dw_timer->pclk);
	}

	dw_timer->clk = devm_clk_get(&pdev->dev, "clk");

	if (IS_ERR_OR_NULL(dw_timer->clk)) {
		pr_err("ax get clk fail\n");
		return PTR_ERR(dw_timer->clk);
	}

	clk_prepare_enable(dw_timer->pclk);
	clk_prepare_enable(dw_timer->clk);
	clk_set_rate(dw_timer->clk, 24*SZ_1M);

	raw_spin_lock_init(&dw_timer->ax_hrtimer_lock);
	atomic_set(&dw_timer->spinlock_count,0);
	ret = misc_register(&ax_hrtimer_miscdev);
	if (ret) {
		printk(KERN_ERR "ax hrtimer cannot register misc device.\n");
		return ret;
	}

	return 0;
}

static int axera_dw_timer_remove(struct platform_device *pdev)
{
	misc_deregister(&ax_hrtimer_miscdev);
	return 0;
}

static int __maybe_unused axera_dw_timer_suspend(struct device *dev)
{
	struct ax_hrtimer *timer, *curr_timer;
	unsigned long flags;

	struct rb_node *node;
	ax_raw_spin_lock_irqsave(&dw_timer->ax_hrtimer_lock, &flags);
	timer = (struct ax_hrtimer *)dw_timer->arg;
	node = rb_first(&ax_hrtimer_rbroot);
	if (node != NULL) {
		curr_timer = rb_entry(node, struct ax_hrtimer, node);
		if (curr_timer == timer) {
			dw_apb_timer_end();
		}
	} else {

	}
	ax_raw_spin_unlock_irqrestore(&dw_timer->ax_hrtimer_lock, &flags);
	return 0;
}

static int __maybe_unused axera_dw_timer_resume(struct device *dev)
{
	struct ax_hrtimer *timer, *curr_timer;
	unsigned long flags;

	struct rb_node *node;
	ax_raw_spin_lock_irqsave(&dw_timer->ax_hrtimer_lock, &flags);

	timer = (struct ax_hrtimer *)dw_timer->arg;
	node = rb_first(&ax_hrtimer_rbroot);
	if (node != NULL) {
		curr_timer = rb_entry(node, struct ax_hrtimer, node);
		if (curr_timer == timer) {
			ax_dw_apb_timer_start(curr_timer);
		}
	} else {

	}

	ax_raw_spin_unlock_irqrestore(&dw_timer->ax_hrtimer_lock, &flags);
	return 0;
}

static SIMPLE_DEV_PM_OPS(axera_dw_timer_pm_ops, axera_dw_timer_suspend, axera_dw_timer_resume);

static const struct of_device_id ax_dw_timer_of_id_table[] = {
	{.compatible = "ax,hrtimer"},
	{}
};

MODULE_DEVICE_TABLE(of, axera_dw_timer_of_id_table);

static struct platform_driver ax_dw_timer_driver = {
	.probe = axera_dw_timer_probe,
	.remove = axera_dw_timer_remove,
	.driver = {
		   .name = KBUILD_MODNAME,
		   .of_match_table = ax_dw_timer_of_id_table,
		   .pm     = &axera_dw_timer_pm_ops,
		   },
};

module_platform_driver(ax_dw_timer_driver);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ax hrtimer platform driver");
