#include <linux/spinlock.h>
#include <linux/wakelock.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/sort.h>
#include <linux/bsearch.h>

#include <soc/base.h>
#include <soc/cpm.h>

#include "clk.h"
struct cpm_pwc
{
	struct timer_list timer;
	unsigned int reg_offset;
	unsigned int ctrl_bit;
	unsigned int wait_bit;
	unsigned int delay_ms;
	const char *name;
};
struct cpm_pwc_ctrl {
	spinlock_t spin_lock;
	struct  wake_lock  pwc_wakelock;
	unsigned int is_suspend;
}cpm_pwc_ctrl;
static struct cpm_pwc cpm_pwc_srcs[] = {
#define PWC_SRC(ID,offset,ctrl,wait,delay)	\
	[ID] = {				\
		.reg_offset = (offset),		\
		.ctrl_bit = (ctrl),		\
		.wait_bit = (wait),		\
		.delay_ms = (delay),		\
	}
	/* No pwc in T15 */
#undef PWC_SRC
};

int cpm_pwc_is_enabled(struct cpm_pwc * pwc) {
	int t;
	unsigned long flags;
	spin_lock_irqsave(&cpm_pwc_ctrl.spin_lock,flags);
	t = cpm_test_bit(pwc->ctrl_bit,pwc->reg_offset); //t == 0 is power on
	spin_unlock_irqrestore(&cpm_pwc_ctrl.spin_lock,flags);
	return (!t);
}
static void cpm_pwc_poweroff(unsigned long data)
{
	int t;
	unsigned long flags;
	struct cpm_pwc *pwc = (struct cpm_pwc *)data;

	if(strcmp(pwc->name, "pwc_lcd") == 0)
		return;

	spin_lock_irqsave(&cpm_pwc_ctrl.spin_lock,flags);
	t = cpm_test_bit(pwc->ctrl_bit,pwc->reg_offset); //t == 0 is power on
	if(!t) {
		cpm_set_bit(pwc->ctrl_bit,pwc->reg_offset);
		while(!cpm_test_bit(pwc->wait_bit,pwc->reg_offset));
	}
	spin_unlock_irqrestore(&cpm_pwc_ctrl.spin_lock,flags);
}
static void cpm_pwc_poweron(unsigned long data)
{
	int t;
	unsigned long flags;
	struct cpm_pwc *pwc = (struct cpm_pwc *)data;
	spin_lock_irqsave(&cpm_pwc_ctrl.spin_lock,flags);
	t = cpm_test_bit(pwc->ctrl_bit,pwc->reg_offset); //t == 0 is power on
	if(t) {
		cpm_clear_bit(pwc->ctrl_bit,pwc->reg_offset);
		while(cpm_test_bit(pwc->wait_bit,pwc->reg_offset));
	}
	spin_unlock_irqrestore(&cpm_pwc_ctrl.spin_lock,flags);
}
static int cpm_pwc_enable(struct clk *clk,int on) {
	struct cpm_pwc *pwc = &cpm_pwc_srcs[CLK_PWC_NO(clk->flags)];

	if(!!on) {
		cpm_pwc_poweron((unsigned long)pwc);
		clk->flags |= CLK_FLG_ENABLE;
	}else {
		clk->flags &= ~CLK_FLG_ENABLE;
		cpm_pwc_poweroff((unsigned long)pwc);
	}
	return 0;
}
int cpm_pwc_enable_ctrl(struct clk *clk,int on) {
	struct cpm_pwc *pwc = &cpm_pwc_srcs[CLK_PWC_NO(clk->flags)];
	if(!!on) {
		del_timer_sync(&pwc->timer);
		cpm_pwc_poweron((unsigned long)pwc);
		clk->flags |= CLK_FLG_ENABLE;
	} else {
		if(cpm_pwc_ctrl.is_suspend == 0 && pwc->delay_ms){
			mod_timer(&pwc->timer,jiffies + msecs_to_jiffies(pwc->delay_ms));
			wake_lock_timeout(&cpm_pwc_ctrl.pwc_wakelock,msecs_to_jiffies(pwc->delay_ms));
		}
		else
			cpm_pwc_poweroff((unsigned long)pwc);
		clk->flags &= ~CLK_FLG_ENABLE;
	}
	return 0;
}
void cpm_pwc_suspend(void)
{
	unsigned long flags;
	spin_lock_irqsave(&cpm_pwc_ctrl.spin_lock,flags);
	cpm_pwc_ctrl.is_suspend = 1;
	spin_unlock_irqrestore(&cpm_pwc_ctrl.spin_lock,flags);
}
void cpm_pwc_resume(void)
{
	unsigned long flags;
	spin_lock_irqsave(&cpm_pwc_ctrl.spin_lock,flags);
	cpm_pwc_ctrl.is_suspend = 0;
	spin_unlock_irqrestore(&cpm_pwc_ctrl.spin_lock,flags);
}

void __init cpm_pwc_init(void)
{
	//set power switch timing
	cpm_outl(0,CPM_PSWC0ST);
	cpm_outl(16,CPM_PSWC1ST);
	cpm_outl(24,CPM_PSWC2ST);
	cpm_outl(8,CPM_PSWC3ST);
	spin_lock_init(&cpm_pwc_ctrl.spin_lock);
	cpm_pwc_ctrl.is_suspend = 1;
}

static struct clk_ops clk_pwc_ops = {
	.enable	= cpm_pwc_enable,
};
void __init init_pwc_clk(struct clk *clk)
{
	struct clk *p;
	struct cpm_pwc *pwc;
	unsigned int id;

	id = CLK_PWC_NO(clk->flags);
	pwc = &cpm_pwc_srcs[CLK_PWC_NO(clk->flags)];
	clk->rate = 0;
	pwc->name = clk->name;
	clk->ops = &clk_pwc_ops;
	if (clk->flags & CLK_FLG_RELATIVE) {
		id = CLK_RELATIVE(clk->flags);
		p = get_clk_from_id(id);
		p->child = clk;
		clk->parent = clk;
		if(p->flags & CLK_FLG_ENABLE)
			cpm_pwc_enable(clk,1);
		else
			cpm_pwc_enable(clk,0);

	}
	clk->parent = NULL;
}

static int __init cpm_pwc_dev_init(void)
{
	int i;
	cpm_pwc_ctrl.is_suspend = 0;
	wake_lock_init(&cpm_pwc_ctrl.pwc_wakelock,WAKE_LOCK_SUSPEND,"pwc wakelock");
	for(i = 0;i < ARRAY_SIZE(cpm_pwc_srcs);i++) {
		setup_timer(&cpm_pwc_srcs[i].timer,cpm_pwc_poweroff,(unsigned long)&cpm_pwc_srcs[i]);
	}
	return 0;
}

module_init(cpm_pwc_dev_init);
