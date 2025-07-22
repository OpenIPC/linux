#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/bsearch.h>
#include <linux/string.h>

#include <soc/cpm.h>
#include <soc/base.h>
#include <soc/extal.h>

#include "clk.h"

static DEFINE_SPINLOCK(cpm_pll_lock);

static u32 frac_to_value(u32 frac)
{
	u32 t = frac * 125;
	u32 v = t / 0x200000 + (((t % 0x200000) * 2 >= 0x200000) ? 1 : 0);

	return v;
}

struct pll_rate_setting {
	unsigned long rate;
	int m,n,od1,od0;
};

struct pll_rate_setting* cal_pll_setting(unsigned long rate)
{
	struct pll_rate_setting *p;
	static struct pll_rate_setting tmp_rate;
	unsigned long pllfreq = 0;
	unsigned int extal = 0;
	unsigned nr = 1, nf = 16 ,od1 = 7, od0 = 1;
	p = &tmp_rate;

    pllfreq = rate / 1000000;
	extal = (get_clk_from_id(CLK_ID_EXT1)->rate) / 1000000;

	/*Unset*/
	if (rate < 25000000 || rate > 5000000000UL){/* Now we only support 25M ~ 5000M */
		printk("kernel clk_pll freq  not in range \n");
		return NULL;
	}

	/*Align to extal clk*/
	if (pllfreq%extal >= extal/2) {
		pllfreq += (extal - pllfreq%extal);
	} else {
		pllfreq -= pllfreq%extal;
	}

	/*caculate nf*/
	do {
		nr++;
		nf = (pllfreq*nr)/extal;
	} while ((nf * extal != nr * pllfreq || nf > 2500) && nr < 63);

	/*caculate od1*/
	while ((nr%od1) && od1 > 2) {
		od1--;
	}
	nr = nr/od1;

	/*caculate od0*/
	od0 = od1;
	while((nr%od0) && od0 > 1) {
		od0--;
	}
	nr = nr/od0;

	if(891000000 == rate) {
		nf  = 297;
		nr  = 4;
		od1 = 2;
		od0 = 1;
	}else if(1188000000 == rate) {
		nf  = 297;
		nr  = 3;
		od1 = 2;
		od0 = 1;
	}else if(1485000000 == rate) {
		nf  = 495;
		nr  = 4;
		od1 = 2;
		od0 = 1;
	}else if(1200000000 == rate) {
		nf  = 100;
		nr  = 1;
		od1 = 2;
		od0 = 1;
	}

	p->rate = nf * (extal / nr / od0 / od1);
	p->m = nf;
	p->n = nr;
	p->od0 = od0;
	p->od1 = od1;

	if(rate <= 800000000) {
		p->m *= 2;
		p->n *= 2;
	}

	/*printk("m=%d n = %d od0 = %d od1 = %d\n",nf,nr,od0,od1);*/

	return p;
}

static int pll_set_rate(struct clk *clk,unsigned long rate)
{
	int ret = -1;
	unsigned int cpxpcr,cpccr;
	struct pll_rate_setting *p=NULL;
	unsigned long flags;
	unsigned int timeout = 0x1ffff;

	if(strcmp(clk->name, "mpll") == 0){
		printk("\033[31m mpll not support set!!!\033[37m\n");
		return 0;
	}
	spin_lock_irqsave(&cpm_pll_lock,flags);

	cpxpcr = cpm_inl(CLK_PLL_NO(clk->flags));
	if(rate == 0) {
		cpxpcr &= ~(1 << 0);
		cpm_outl(cpxpcr,CLK_PLL_NO(clk->flags));
		clk->rate = 0;
		ret = 0;
		goto PLL_SET_RATE_FINISH;
	} else if(rate <= clk_get_rate(clk->parent)) {
		ret = -1;
	} else {
		if(0x10 == CLK_PLL_NO(clk->flags)) {
			cpccr = cpm_inl(0x0);
			cpccr &=~(3<<30);
			cpccr |=1<<30;
			cpm_outl(cpccr,0x0);
		}
		p = cal_pll_setting(rate);
		if(p) {
			cpxpcr &= ~1;
			cpm_outl(cpxpcr,CLK_PLL_NO(clk->flags));

			cpxpcr &= ~((0xfff << 20) | (0x3f << 14) | (0x7 << 11) | (0x7 << 8));
			cpxpcr |= ((p->m) << 20) | ((p->n) << 14) |
				((p->od1) << 11) | ((p->od0) << 8) ;
			cpm_outl(cpxpcr,CLK_PLL_NO(clk->flags));

			cpxpcr |= 1;
			cpm_outl(cpxpcr,CLK_PLL_NO(clk->flags));

			ret = 0;
			clk->rate = p->rate * 1000 * 1000;

			while(!(cpm_inl(CLK_PLL_NO(clk->flags)) & (1 << 3)) && timeout--);
			if (timeout == 0) {
				printk("wait pll stable timeout!");
				ret = -1;
			}
		} else {
			printk("no support this rate [%ld]\n",rate);
		}
		if(0x10 == CLK_PLL_NO(clk->flags)) {
			cpccr = cpm_inl(0x0);
			cpccr &=~(3<<30);
			cpccr |=2<<30;
			cpm_outl(cpccr,0x0);
		}
	}

PLL_SET_RATE_FINISH:
	spin_unlock_irqrestore(&cpm_pll_lock,flags);
	return ret;
}

static unsigned long pll_get_rate(struct clk *clk) {
	unsigned long cpxpcr;
	unsigned long m,n,od1,od0;
	unsigned long rate;
	unsigned long flags;
	unsigned long frac, frac_value;

	spin_lock_irqsave(&cpm_pll_lock,flags);

	cpxpcr = cpm_inl(CLK_PLL_NO(clk->flags));
	if(cpxpcr & 1){
		clk->flags |= CLK_FLG_ENABLE;
		m = ((cpxpcr >> 20) & 0xfff);
		n = ((cpxpcr >> 14) & 0x3f);
		od1 = ((cpxpcr >> 11) & 0x07);
		od0 = ((cpxpcr >> 8) & 0x07);
		frac = cpm_inl(CLK_PLL_NO(clk->flags) + 8);
		frac_value = frac_to_value(frac);
		rate = ((clk->parent->rate / 4000) * m / n / od1 / od0) * 4000
			+ ((clk->parent->rate / 4000) * frac_value / n / od1 / od0) * 4;
	}else  {
		clk->flags &= ~(CLK_FLG_ENABLE);
		rate = 0;
	}
	spin_unlock_irqrestore(&cpm_pll_lock,flags);
	return rate;
}
static struct clk_ops clk_pll_ops = {
	.get_rate = pll_get_rate,
	.set_rate = pll_set_rate,
};
void __init init_ext_pll(struct clk *clk)
{
	switch (get_clk_id(clk)) {
	case CLK_ID_EXT0:
		clk->rate = JZ_EXTAL_RTC;
		clk->flags |= CLK_FLG_ENABLE;
		break;
	case CLK_ID_EXT1:
		clk->rate = JZ_EXTAL;
		clk->flags |= CLK_FLG_ENABLE;
		break;
	case CLK_ID_OTGPHY:
		clk->rate = 48 * 1000 * 1000;
		clk->flags |= CLK_FLG_ENABLE;
		break;
	default:
		clk->parent = get_clk_from_id(CLK_ID_EXT1);
		clk->rate = pll_get_rate(clk);
		clk->ops = &clk_pll_ops;
		break;
	}
}
