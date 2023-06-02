/* 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/export.h>

#include <asm/io.h>
#include <asm/mach/time.h>

#include <mach/map.h>

#include <mach/timer.h>
#include <linux/delay.h>

#define AK39_TIMER1_CTRL1		(AK_VA_SYSCTRL + 0xB4)
#define AK39_TIMER1_CTRL2		(AK_VA_SYSCTRL + 0xB8)
#define AK39_TIMER2_CTRL1		(AK_VA_SYSCTRL + 0xBC)
#define AK39_TIMER2_CTRL2		(AK_VA_SYSCTRL + 0xC0)
#define AK39_TIMER3_CTRL1		(AK_VA_SYSCTRL + 0xC4)
#define AK39_TIMER3_CTRL2		(AK_VA_SYSCTRL + 0xC8)
#define AK39_TIMER4_CTRL1		(AK_VA_SYSCTRL + 0xCC)
#define AK39_TIMER4_CTRL2		(AK_VA_SYSCTRL + 0xD0)
#define AK39_TIMER5_CTRL1		(AK_VA_SYSCTRL + 0xD4)
#define AK39_TIMER5_CTRL2		(AK_VA_SYSCTRL + 0xD8)

#define AK39_TIMER_CTRL1		AK39_TIMER2_CTRL1
#define AK39_TIMER_CTRL2		AK39_TIMER2_CTRL2
#define IRQ_TIMER				IRQ_TIMER2

#define TIMER_CNT				(12000000/HZ)
#define TIMER_USEC_SHIFT		16
#define TIMER_CNT_MASK			(0x3F<<26)

//define timer register bits
#define TIMER_CLEAR_BIT			(1<<30)
#define TIMER_FEED_BIT			(1<<29)
#define TIMER_ENABLE_BIT		(1<<28)
#define TIMER_STATUS_BIT		(1<<27)
#define TIMER_READ_SEL_BIT		(1<<26)

//define pwm/pwm mode
#define MODE_AUTO_RELOAD_TIMER	0x0
#define MODE_ONE_SHOT_TIMER		0x1
#define MODE_PWM				0x2   

#if 1
#define PDEBUG(fmt, args...) printk( KERN_ERR fmt,## args)
#else
#define PDEBUG(fmt, args...) 
#endif

struct aktimer {
	volatile unsigned int __force *ctrl1;
	volatile unsigned int __force *ctrl2;

	int timer_bit;
	int irq;
	timer_handler handler;
	void *data;
};
static unsigned long flags;

/* copy from plat-s3c/time.c
 *
 *  timer_mask_usec_ticks
 *
 * given a clock and divisor, make the value to pass into timer_ticks_to_usec
 * to scale the ticks into usecs
*/
static inline unsigned long
timer_mask_usec_ticks(unsigned long scaler, unsigned long pclk)
{
	unsigned long den = pclk / 1000;

	return ((1000 << TIMER_USEC_SHIFT) * scaler + (den >> 1)) / den;
}

static inline void ak39_timer_setup(struct aktimer *ptimer)
{
	unsigned long regval;

	/* clear timeout puls, reload */
    regval = __raw_readl(ptimer->ctrl2);
    __raw_writel(regval | TIMER_CLEAR_BIT, ptimer->ctrl2);
}

/*
 * IRQ handler for the timer
 */
static irqreturn_t ak39_timer_interrupt(int irq, void *dev_id)
{
	struct aktimer *ptimer = dev_id;

	if (__raw_readl(ptimer->ctrl2) & TIMER_STATUS_BIT) {

		ptimer->handler(ptimer->data);

		ak39_timer_setup(ptimer);
	}

	return IRQ_HANDLED;
}

void * ak39_timer_probe(int which_timer)
{
	int ret;
	int timer_bit;
	struct aktimer *ptimer;

	ptimer = kmalloc(sizeof(struct aktimer), GFP_KERNEL);
	if (ptimer == NULL) {
		PDEBUG("%s kmalloc failed.\n", __func__);
		goto err1;
	}

	memset(ptimer, 0, sizeof(*ptimer));

	switch (which_timer) {
	case 1:
		ptimer->ctrl1 = AK39_TIMER1_CTRL1;
		ptimer->ctrl2 = AK39_TIMER1_CTRL2;
		ptimer->irq = IRQ_TIMER1;
		break;
	case 2:
		ptimer->ctrl1 = AK39_TIMER2_CTRL1;
		ptimer->ctrl2 = AK39_TIMER2_CTRL2;
		ptimer->irq = IRQ_TIMER2;
		break;
	case 3:
		ptimer->ctrl1 = AK39_TIMER3_CTRL1;
		ptimer->ctrl2 = AK39_TIMER3_CTRL2;
		ptimer->irq = IRQ_TIMER3;
		break;
	case 4:
		ptimer->ctrl1 = AK39_TIMER4_CTRL1;
		ptimer->ctrl2 = AK39_TIMER4_CTRL2;
		ptimer->irq = IRQ_TIMER4;
		break;
	case 5:
		ptimer->ctrl1 = AK39_TIMER5_CTRL1;
		ptimer->ctrl2 = AK39_TIMER5_CTRL2;
		ptimer->irq = IRQ_TIMER5;
		break;
	default:
		PDEBUG("error ak39 only 5 timers.\n");
		goto err2;
		break;
	}

	timer_bit = 1<<(which_timer - 1);
	ptimer->timer_bit = timer_bit;

	/* setup irq handler for IRQ_TIMER */
	ret = request_irq(ptimer->irq, ak39_timer_interrupt, IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL, "timer for ptz", ptimer);
	if (ret) {
		PDEBUG("request irq for timer failed.\n");
		goto err2;
	}
	return ptimer;

err2:
	kfree(ptimer);
err1:
	return NULL;
}
EXPORT_SYMBOL(ak39_timer_probe);

int ak39_timer_remove(void *priv)
{
	struct aktimer *ptimer = priv;

	ak39_timer_stop(priv);
	free_irq(ptimer->irq, ptimer);
	kfree(ptimer);

	return 0;
}
EXPORT_SYMBOL(ak39_timer_remove);

/*
 *which_timer:	1~5, total five timers.
 * */
int ak39_timer_start(timer_handler handler, void *data, void *priv, int hz)
{
	int ret = 0;
	unsigned long timecnt = (12000000/hz) - 1;
	struct aktimer *ptimer = priv;
	int timer_bit = ptimer->timer_bit;

	if (handler == NULL) {
		PDEBUG("%s handler NULL", __func__);
		ret = -EINVAL;
		goto err1;
	}

#if 0
	if (test_bit(timer_bit, &flags)) {
		PDEBUG("The timer be used.\n");
		ret = -EBUSY;
		goto err1;
	}
#endif

	set_bit(timer_bit, &flags);

	ptimer->handler = handler;
	ptimer->data = data;

	__raw_writel(timecnt, ptimer->ctrl1);
	__raw_writel((TIMER_ENABLE_BIT | TIMER_FEED_BIT | (MODE_AUTO_RELOAD_TIMER << 24)), 
		ptimer->ctrl2);

err1:
	return ret;
}
EXPORT_SYMBOL(ak39_timer_start);

int ak39_timer_stop(void *priv)
{
	struct aktimer *ptimer = priv;

	__raw_writel(~TIMER_ENABLE_BIT, ptimer->ctrl2);
	clear_bit(ptimer->timer_bit, &flags);

	return 0;
}
EXPORT_SYMBOL(ak39_timer_stop);