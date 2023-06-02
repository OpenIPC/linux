/*
 * fh_simple_timer.h
 *
 *  Created on: Jan 22, 2017
 *      Author: duobao
 */

#ifndef FH_SIMPLE_TIMER_H_
#define FH_SIMPLE_TIMER_H_

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/timerqueue.h>
#include <mach/pmu.h>
#include <mach/timex.h>
#include <mach/io.h>
#include <mach/fh_predefined.h>


static inline unsigned int timern_base(int n)
{
	unsigned int base = 0;
	switch (n) {
	case 0:
	default:
		base = TIMER_REG_BASE;
		break;
	case 1:
		base = TIMER_REG_BASE + 0x14;
		break;
	case 2:
		base = TIMER_REG_BASE + 0x28;
		break;
	case 3:
		base = TIMER_REG_BASE + 0x3c;
		break;
	}
	return base;
}

#define TIMERN_REG_BASE(n)		(timern_base(n))

#define	REG_TIMER_LOADCNT(n)		(timern_base(n) + 0x00)
#define	REG_TIMER_CUR_VAL(n)		(timern_base(n) + 0x04)
#define	REG_TIMER_CTRL_REG(n)		(timern_base(n) + 0x08)
#define	REG_TIMER_EOI_REG(n)		(timern_base(n) + 0x0C)
#define	REG_TIMER_INTSTATUS(n)		(timern_base(n) + 0x10)

#define REG_TIMERS_INTSTATUS		(TIMER_REG_BASE + 0xa0)


enum simple_timer_state {
	SIMPLE_TIMER_STOP,
	SIMPLE_TIMER_START,
	SIMPLE_TIMER_ERROR,
};

struct fh_simple_timer
{
	struct timerqueue_node		node;
	ktime_t it_interval;	/* timer period */
	ktime_t it_value;	/* timer expiration */
	ktime_t it_delay;
	void (*function) (void *);
	void *param;
};


int fh_simple_timer_interrupt(void);
int fh_simple_timer_create(struct fh_simple_timer *tim);
int fh_timer_start(void);
int fh_simple_timer_init(void);
int fh_simple_timer_periodic_start(struct fh_simple_timer *tim);
int fh_simple_timer_periodic_stop(void);

#endif /* FH_SIMPLE_TIMER_H_ */
