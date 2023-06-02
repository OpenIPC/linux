#include <linux/module.h>
#include <mach/fh_simple_timer.h>

//#define  FH_TIMER_DEBUG
#ifdef FH_TIMER_DEBUG
#define PRINT_DBG(fmt,args...)  printk(fmt,##args)
#else
#define PRINT_DBG(fmt,args...)  do{} while(0)
#endif

enum SIMPLE_TIMER_WORKMODE {
	SIMPLE_TIMER_SEQ,
	SIMPLE_TIMER_PERIOD
};

struct simple_time_base
{
	struct timerqueue_head	simple_timer_queue;
	int state;
	int workmode;
};
struct fh_simple_timer periodic_timer;


struct simple_time_base base;

static void fh_timer_enable(void)
{
	SET_REG(VTIMER(REG_TIMER_CTRL_REG(SIMPLE_TIMER_BASE)), 0x3);
}

static void fh_timer_disable(void)
{
	SET_REG(VTIMER(REG_TIMER_CTRL_REG(SIMPLE_TIMER_BASE)), 0x0);
}

static void fh_timer_clearirq(void)
{
	GET_REG(VTIMER(REG_TIMER_EOI_REG(SIMPLE_TIMER_BASE)));
}

void fh_simple_timer_set_next(long cycles)
{
	long curr_val;
	int sync_cnt =  0;


	PRINT_DBG("cycles: %lu\n", cycles);

	if (cycles < 0) {
		pr_err("ERROR: cycles is invaild: %lu\n", cycles);
		fh_timer_clearirq();
		fh_timer_disable();
		base.state = SIMPLE_TIMER_ERROR;
		return;
	}

	SET_REG_M(VTIMER(REG_TIMER_CTRL_REG(SIMPLE_TIMER_BASE)), 0x00, 0x1);
	/* zy/ticket/100 : update apb Timer LOADCNT */
	/* CURRENTVALE could,t be start from new LOADCOUNT */
	/* cause is timer clk 1M hz and apb is 150M hz */
	/* check current cnt for it is disabled */
	while (GET_REG(VTIMER(REG_TIMER_CUR_VAL(1))) != 0) {
		sync_cnt++;
		if (sync_cnt >= 50) {
			/* typical cnt is 5 when in 1M timer clk */
			/* so here use 50 to check whether it is err */
			pr_err("timer problem,can't disable");
		}
	}
	SET_REG(VTIMER(REG_TIMER_LOADCNT(SIMPLE_TIMER_BASE)), cycles);
	SET_REG_M(VTIMER(REG_TIMER_CTRL_REG(SIMPLE_TIMER_BASE)), 0x01, 0x1);
#ifdef CONFIG_USE_PTS_AS_CLOCKSOURCE
#if defined(CONFIG_ARCH_FH8632) || defined(CONFIG_ARCH_FH8833) || defined(CONFIG_ARCH_FH8810)
	curr_val = GET_REG(VTIMER(REG_TIMER_CUR_VAL(SIMPLE_TIMER_BASE))) ;
	if (curr_val >  0x80000000) { ///0xffff0000)
		panic("timer curr %lu, want cycles %lu\n", curr_val, cycles);

		SET_REG_M(VTIMER(REG_TIMER_CTRL_REG(SIMPLE_TIMER_BASE)), 0x01, 0x1);
		SET_REG(VTIMER(REG_TIMER_LOADCNT(SIMPLE_TIMER_BASE)), cycles);

		//pmu reset
		fh_pmu_set_reg(REG_PMU_SWRST_MAIN_CTRL, 0xfffbffff);
		while (fh_pmu_get_reg(REG_PMU_SWRST_MAIN_CTRL) != 0xffffffff) {

		}
	}

	fh_pmu_set_reg(REG_PMU_SWRST_MAIN_CTRL, 0xfffbffff);
	while (fh_pmu_get_reg(REG_PMU_SWRST_MAIN_CTRL) != 0xffffffff) {

	}
#endif
#endif

}

int fh_simple_timer_create(struct fh_simple_timer* new)
{
	timerqueue_init(&new->node);
	new->node.expires = new->it_value;
	timerqueue_add(&base.simple_timer_queue, &new->node);
	return 0;
}
EXPORT_SYMBOL_GPL(fh_simple_timer_create);

int fh_timer_start(void)
{
	struct fh_simple_timer *timer = NULL;
	struct timerqueue_node *node;

	if (base.state == SIMPLE_TIMER_START)
		return 0;


	node = timerqueue_getnext(&base.simple_timer_queue);

	if(node == NULL)
	{
		pr_err("ERROR: timequeue is empty\n");
		return -1;
	}
	base.workmode = SIMPLE_TIMER_SEQ;
	timer = container_of(node, struct fh_simple_timer, node);

	base.state = SIMPLE_TIMER_START;
	fh_timer_enable();
	fh_simple_timer_set_next(ktime_to_us(ktime_sub(timer->it_value, timer->it_delay)));
	return 0;
}
EXPORT_SYMBOL_GPL(fh_timer_start);

int fh_simple_timer_interrupt_seq(void)
{
	ktime_t diff;
	struct fh_simple_timer *curr = NULL, *next = NULL;
	struct timerqueue_node *node;

	node = timerqueue_getnext(&base.simple_timer_queue);

	if(node == NULL)
	{
		pr_err("ERROR: timequeue is empty\n");
		fh_timer_clearirq();
		fh_timer_disable();
		base.state = SIMPLE_TIMER_ERROR;
		return -1;
	}

	curr = container_of(node, struct fh_simple_timer, node);

	timerqueue_del(&base.simple_timer_queue, &curr->node);

	curr->function(curr->param);

	node = timerqueue_getnext(&base.simple_timer_queue);

	if(node == NULL)
	{
		PRINT_DBG("finished all timers, close device\n");
		fh_timer_clearirq();
		fh_timer_disable();
		base.state = SIMPLE_TIMER_STOP;
		return 0;
	}

	next = container_of(node, struct fh_simple_timer, node);

	PRINT_DBG("sec: %lu, nsec: %lu\n",
			ktime_to_timespec(next->it_value).tv_sec,
			ktime_to_timespec(next->it_value).tv_nsec);

	diff = ktime_sub(next->it_value, curr->it_value);

	fh_simple_timer_set_next(ktime_to_us(ktime_sub(diff, next->it_delay)));
	fh_timer_clearirq();
	return 0;
}
int fh_simple_timer_interrupt_period(void)
{

	periodic_timer.function(periodic_timer.param);
	fh_timer_clearirq();
	return 0;
}

int fh_simple_timer_interrupt(void)
{
	if (base.workmode == SIMPLE_TIMER_SEQ)
		return fh_simple_timer_interrupt_seq();
	else
		return fh_simple_timer_interrupt_period();
}


int fh_simple_timer_init(void)
{
	base.state = SIMPLE_TIMER_STOP;
	timerqueue_init_head(&base.simple_timer_queue);
	memset(&periodic_timer, 0, sizeof(periodic_timer));
	fh_timer_disable();
	return 0;
}
EXPORT_SYMBOL_GPL(fh_simple_timer_init);


int fh_simple_timer_periodic_start(struct fh_simple_timer *tim)
{

	if (base.state == SIMPLE_TIMER_START)
		return 0;

	if (tim == NULL)
		return 0;

	periodic_timer = *tim;



	base.state = SIMPLE_TIMER_START;
	base.workmode = SIMPLE_TIMER_PERIOD;
	fh_timer_enable();
	fh_simple_timer_set_next(ktime_to_us(ktime_sub(periodic_timer.it_value,
		periodic_timer.it_delay)));

	return 0;
}
EXPORT_SYMBOL_GPL(fh_simple_timer_periodic_start);
int fh_simple_timer_periodic_stop(void)
{
	fh_timer_disable();
	base.state = SIMPLE_TIMER_STOP;

	return 0;
}
EXPORT_SYMBOL_GPL(fh_simple_timer_periodic_stop);

