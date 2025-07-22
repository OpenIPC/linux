#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/cpufreq.h>
#include <linux/bsearch.h>

#include <asm/cacheops.h>
#include <soc/cpm.h>
#include <soc/cache.h>
#include <soc/base.h>
#include <soc/extal.h>
#include <jz_notifier.h>
#include "clk.h"
#define USE_PLL
static DEFINE_SPINLOCK(cpm_cpccr_lock);
struct cpccr_clk {
	short off,sel,ce;
};
static struct cpccr_clk cpccr_clks[] = {
#define CPCCR_CLK(N,O,D,E)			\
	[N] = { .off = O, .sel = D, .ce = E}
	CPCCR_CLK(CDIV, 0, 28,22),
	CPCCR_CLK(L2CDIV, 4, 28,22),
	CPCCR_CLK(H0DIV, 8, 26,21),
	CPCCR_CLK(H2DIV, 12, 24,20),
	CPCCR_CLK(PDIV, 16, 24,20),
	CPCCR_CLK(SCLKA,-1, -1,30),
#undef CPCCR_CLK
};
static unsigned int cpccr_selector[4] = {0,CLK_ID_SCLKA,CLK_ID_MPLL,0};

#define MHz (1000 * 1000)

#define AHB_MIN	  (100*MHz)
#define l2div_policy(rate,div) ({               \
            unsigned int l2div;                 \
            if(rate > 800*MHz)                  \
                l2div = div * 4;                \
            else if(rate > 200*MHz)             \
                l2div = div * 3;                \
            else                                \
                l2div = div * 2;                \
            l2div - 1;})

static int cclk_set_rate_nopll(struct clk *clk,unsigned long rate,struct clk *parentclk,unsigned int cpccr) {
	unsigned int parent_rate;
	unsigned int target_rate;
	unsigned int div;
	int l2div,cdiv;
	int ret = -1;
	parent_rate = parentclk->rate;
	for(div = 1;div < 15;div++) {
		target_rate = parent_rate / div;
		if(target_rate <= rate)
			break;
	}
	if(div >= 15){
		dump_stack();
		printk("%s don't find the rate[%ld]\n",clk->name,rate);
		goto SET_CPCCR_RATE_ERR;
	}
	cdiv = div - 1;
	l2div = l2div_policy(rate,div);
	if(cdiv < 0 || l2div > 14 ) {
		printk("%s don't comply rules cdiv[%d] l2div[%d]\n",clk->name,cdiv,l2div);
		goto SET_CPCCR_RATE_ERR;
	}

	cache_prefetch(LAB5,64);
	fast_iob();
LAB5:
	cpccr &= ~((0xf << cpccr_clks[CDIV].off) | (0xf << cpccr_clks[L2CDIV].off));
	cpccr |= ((cdiv << cpccr_clks[CDIV].off) | (l2div << cpccr_clks[L2CDIV].off));
	cpccr |= (1 << cpccr_clks[CDIV].ce) | (1 << cpccr_clks[L2CDIV].ce);
	cpm_outl(cpccr,CPM_CPCCR);
	/* wait not busy */
	while(cpm_inl(CPM_CPCSR) & 1);

	cpccr &= ~((1 << cpccr_clks[CDIV].ce) | (1 << cpccr_clks[L2CDIV].ce));
	cpm_outl(cpccr,CPM_CPCCR);
	clk->rate = parentclk->rate / (cdiv + 1);
	get_clk_from_id(CLK_ID_L2CLK)->rate = parentclk->rate / (l2div + 1);
	ret = 0;
SET_CPCCR_RATE_ERR:
	return ret;
}
static int get_cpccr_div(unsigned int prate,unsigned int rate)
{
	unsigned int div = prate / rate;
	if((prate % rate) && (div > 1))
		div--;
	if(div >= 15)
		div = 15;
	return div;
}
static inline void set_cpccr_h0div(struct clk *clk,unsigned int rate)
{
	unsigned int cpccr = cpm_inl(CPM_CPCCR);
	int sel=(cpccr >> cpccr_clks[H0DIV].sel) & 3;
	struct clk *parentclk = get_clk_from_id(cpccr_selector[sel]);
	unsigned int hdiv;
	unsigned int pclk_rate = clk_get_rate(parentclk);
	hdiv = get_cpccr_div(pclk_rate,rate) - 1;
	if(clk->rate != pclk_rate / hdiv){
		cpccr &= ~(0xf << cpccr_clks[H0DIV].off);
		cpccr |= (hdiv << cpccr_clks[H0DIV].off);
		cpccr |= (1 << cpccr_clks[H0DIV].ce);
		cpm_outl(cpccr,CPM_CPCCR);
		/* wait not busy */
		while(cpm_inl(CPM_CPCSR) & 2);

		cpccr &= ~(1 << cpccr_clks[H0DIV].ce);
		cpm_outl(cpccr,CPM_CPCCR);
		clk->rate = pclk_rate / (hdiv + 1);
	}
}
//#define APB_RATE_MAX   (150 * 1000 * 1000)
//#define APB_RATE_MIN   (96 * 1000 * 1000)
static inline void set_cpccr_h2div(struct clk *clk,unsigned int rate)
{
	unsigned int cpccr = cpm_inl(CPM_CPCCR);
	int sel=(cpccr >> cpccr_clks[H2DIV].sel) & 3;
	struct clk *parentclk = get_clk_from_id(cpccr_selector[sel]);
	unsigned int hdiv,pdiv;
	unsigned int pclk_rate = clk_get_rate(parentclk);
	struct clk *relativeclk = get_clk_from_id(CLK_ID_PCLK);
	hdiv = get_cpccr_div(pclk_rate,rate);
	pdiv = hdiv * 2;
	if(pdiv >= 15)
		pdiv = hdiv;
	hdiv--;
	pdiv--;
	if(clk->rate != pclk_rate / hdiv){
		cpccr &= ~((0xf << cpccr_clks[H2DIV].off) | (0xf << cpccr_clks[PDIV].off));
		cpccr |= (hdiv << cpccr_clks[H2DIV].off) | (pdiv << cpccr_clks[PDIV].off);
		cpccr |= (1 << cpccr_clks[H2DIV].ce);
//		udelay(1000);
		cpm_outl(cpccr,CPM_CPCCR);
		/* wait not busy */
		while(cpm_inl(CPM_CPCSR) & 4);
		cpccr &= ~(1 << cpccr_clks[H2DIV].ce);
		cpm_outl(cpccr,CPM_CPCCR);
		clk->rate = pclk_rate / (hdiv + 1);
		relativeclk->rate = pclk_rate / (pdiv + 1);
	}
}
static inline void sw_ahb_from_l2cache(void)
{
	struct clk *ahb0 = get_clk_from_id(CLK_ID_H0CLK);
	struct clk *ahb2 = get_clk_from_id(CLK_ID_H2CLK);
	unsigned int rate = get_clk_from_id(CLK_ID_L2CLK)->rate;
	struct clk *msc = get_clk_from_id(CLK_ID_MSC);

	if(clk_is_enabled(msc))
		return;
	if(rate >= 300*1000*1000)
	{
		set_cpccr_h0div(ahb0,200*1000*1000);
		set_cpccr_h2div(ahb2,200*1000*1000);
	}else if(rate >= 150*1000*1000){
		set_cpccr_h0div(ahb0,150*1000*1000);
		set_cpccr_h2div(ahb2,150*1000*1000);
	}else if(rate >= 50*1000*1000){
		set_cpccr_h0div(ahb0,100*1000*1000);
		set_cpccr_h2div(ahb2,100*1000*1000);
	}else{
		set_cpccr_h0div(ahb0,60*1000*1000);
		set_cpccr_h2div(ahb2,60*1000*1000);
	}
}
static int ahb_change_notify(struct jz_notifier *notify,void *v)
{
	unsigned int val = (unsigned int)v;
	unsigned int on = val & 0x80000000;
	unsigned int clk_id = val & (~0x80000000);
	unsigned long flags;
	struct clk *ahb0 = get_clk_from_id(CLK_ID_H0CLK);
	struct clk *ahb2 = get_clk_from_id(CLK_ID_H2CLK);

	if(on && (clk_id == CLK_ID_MSC)) {
		if(clk_get_rate(ahb2) == 200000000)
			return NOTIFY_OK;
		spin_lock_irqsave(&cpm_cpccr_lock,flags);
		set_cpccr_h0div(ahb0,200*1000*1000);
		set_cpccr_h2div(ahb2,200*1000*1000);
		spin_unlock_irqrestore(&cpm_cpccr_lock,flags);
	}
	return NOTIFY_OK;
}
/*
 * Note that loops_per_jiffy is not updated on SMP systems in
 * cpufreq driver. So, update the per-CPU loops_per_jiffy value
 * on frequency transition. We need to update all dependent CPUs.
 */
extern struct  freq_udelay_jiffy *freq_udelay_jiffys;
static int cpufreq_setting_cmp(const void *key,const void *elt) {
	unsigned int *d = (unsigned int*)key;
	struct freq_udelay_jiffy *p = (struct freq_udelay_jiffy *)elt;
	if(*d > p->cpufreq)
		return 1;
	else if(*d < p->cpufreq)
		return -1;
	return 0;
}
static struct freq_udelay_jiffy* search_cpufrq_setting(unsigned int rate) {
	struct freq_udelay_jiffy *p = NULL;
	unsigned int num;

	num = freq_udelay_jiffys[0].max_num;
	p = (struct freq_udelay_jiffy *)bsearch((const void*)&rate,(const void*)freq_udelay_jiffys,
						num, sizeof(struct freq_udelay_jiffy),cpufreq_setting_cmp);
	if(!p) {
		dump_stack();
		printk("warning!!!, new %d freq not found\n", rate);
		while(1);
	}
	return p;
}


#define before_change_udelay_hz(lock, freqs_old, freqs_new) do {	\
		struct freq_udelay_jiffy *p;				\
		if (freqs_old < freqs_new) {				\
			if (!lock) {					\
				p = search_cpufrq_setting(freqs_new);	\
				cpu_data[0].udelay_val = p->udelay_val;	\
				loops_per_jiffy = p->loops_per_jiffy;	\
			} else {					\
				spin_lock_irqsave(&cpm_cpccr_lock,flags); \
				p = search_cpufrq_setting(freqs_new);	\
				cpu_data[0].udelay_val = p->udelay_val;	\
				loops_per_jiffy = p->loops_per_jiffy;	\
				spin_unlock_irqrestore(&cpm_cpccr_lock,flags); \
			}						\
	}								\
	}while(0)

#define after_change_udelay_hz(freqs_old, freqs_new) do {		\
		struct freq_udelay_jiffy *p;				\
		if (freqs_new < freqs_old) {				\
			p = search_cpufrq_setting(freqs_new);		\
			cpu_data[0].udelay_val = p->udelay_val;		\
			loops_per_jiffy = p->loops_per_jiffy;		\
		}							\
	 }while(0)

static int cpccr_set_rate(struct clk *clk,unsigned long rate) {
	int sel;
	unsigned int cpccr = cpm_inl(CPM_CPCCR);
	int clkid = &cpccr_clks[CLK_CPCCR_NO(clk->flags)] - &cpccr_clks[0];
	int ret = -1;
	struct clk *parentclk = NULL;
	unsigned long flags;
	unsigned long prate = clk_get_rate(get_clk_from_id(CLK_ID_EXT1));
	if(clkid == CDIV)
	{
		struct clk_notify_data dn;
		dn.current_rate = clk->rate;
		dn.target_rate = rate;
		jz_notifier_call(NOTEFY_PROI_HIGH, JZ_CLK_PRECHANGE,&dn);
	}
	spin_lock_irqsave(&cpm_cpccr_lock,flags);
	switch(clkid) {
	case SCLKA:
	{
		unsigned int cpccr_sel_src;
		cpccr_sel_src = cpccr >> 30;
		if(rate > prate) {
			clk->parent = get_clk_from_id(CLK_ID_APLL);
			spin_unlock_irqrestore(&cpm_cpccr_lock,flags);
			ret = clk_set_rate(clk->parent,rate);
			spin_lock_irqsave(&cpm_cpccr_lock,flags);
			cpccr &= ~(3 << 30);
			cpccr |= 2 << 30;
			cpm_outl(cpccr,CPM_CPCCR);
			if(!ret)
				clk->rate = clk->parent->rate;
		} else {
			cpccr &= ~(3 << 30);
			cpccr |= 1 << 30;
			cpm_outl(cpccr,CPM_CPCCR);
			/* don't close Apll */
			//	if(clk->parent != get_clk_from_id(CLK_ID_EXT1))
			//		clk_set_rate(clk->parent,0);
			clk->parent = get_clk_from_id(CLK_ID_EXT1);
			clk->rate = prate;
			// no success ret = -1;
		}
		break;
	}
	case CDIV:
		sel=(cpccr >> cpccr_clks[clkid].sel) & 3;
		parentclk = get_clk_from_id(cpccr_selector[sel]);
#ifdef USE_PLL
		{
			unsigned int ddrcgu = cpm_inl(CPM_DDRCDR);
			unsigned int ddrsel = ddrcgu >> 30;
			unsigned int h2sel = (cpccr >> 24) & 0x3;
			unsigned int h0sel = (cpccr >> 26) & 0x3;
			unsigned int csel = (cpccr >> 28) & 0x3;
			unsigned int cpccr_temp,tdiv,l2div;
			unsigned int ddr_pll_rate;
			if(get_clk_from_id(CLK_ID_CGU_DDR)->parent) {
				ddr_pll_rate = clk_get_rate(get_clk_from_id(CLK_ID_CGU_DDR)->parent);
			} else {
				printk("ddr cgu clk set failure!\n");
				BUG_ON(1);
			}

			if((h2sel != h0sel) || (h2sel != ddrsel) || (h0sel != ddrsel)) {
				printk("h2sel[%d] h0sel[%d] ddrsel[%d] should be from the same pll!\n",h2sel,h0sel,ddrsel);
				break;
			}
			if(csel == ddrsel) {
				printk("csel[%d] ddrsel[%d],this config no support pll switch!\n",csel,ddrsel);
				break;
			}
			/*
			 *    1. switch to ddrsel & switch to 200M cclk
			 *    2. set pll freq
			 *    3. switch to csel
			 */
			/*
			 *    1. switch to ddrsel & switch to 200M cclk
			 */
			before_change_udelay_hz(0, clk->rate / 1000, 200000);
			cache_prefetch(LAB1,64);
			fast_iob();
		LAB1:
                        tdiv = (ddr_pll_rate + 200*MHz - 1) / (200*MHz);
			tdiv = tdiv - 1;
			cpccr_temp = cpccr & ~((3 << 28) | (0xf << cpccr_clks[CDIV].off) | (0xf << cpccr_clks[L2CDIV].off));
			cpccr_temp |= cpccr_temp | (ddrsel << 28) | (tdiv << cpccr_clks[CDIV].off) | (tdiv << cpccr_clks[L2CDIV].off) ;
			cpccr_temp |= (1 << cpccr_clks[CDIV].ce) | (1 << cpccr_clks[L2CDIV].ce);
			cpm_outl(cpccr_temp,CPM_CPCCR);
			while(cpm_inl(CPM_CPCSR) & 1);
			//cpm_inl(CPM_CPCCR);
			after_change_udelay_hz(clk->rate / 1000, 200000);

			spin_unlock_irqrestore(&cpm_cpccr_lock,flags);
			{
				struct clk_notify_data dn;
				dn.current_rate = clk->rate;
				dn.target_rate = rate;
				jz_notifier_call(NOTEFY_PROI_HIGH, JZ_CLK_CHANGING,&dn);
			}
			// 2. set pll freq
			before_change_udelay_hz(1, 200000, rate/1000);
			ret = clk_set_rate(parentclk,rate);
			jz_notifier_call(NOTEFY_PROI_HIGH, JZ_CLK_CHANGED,NULL);
			spin_lock_irqsave(&cpm_cpccr_lock,flags);

			if(ret != 0) {
				/*
				 *  3. switch to csel
				 */
				cpccr_temp = cpm_inl(CPM_CPCCR);
				cpccr_temp &= ~(3 << 28);
				cpccr_temp |= csel << 28;
				parentclk = get_clk_from_id(cpccr_selector[sel]);
				ret = cclk_set_rate_nopll(clk,rate,parentclk,cpccr_temp);
				after_change_udelay_hz(200000, rate/1000);
				if(ret) {
					printk("cpccr set rate fail!\n");
				}else{
					sw_ahb_from_l2cache();
					break;
				}
			}else {
				/*
				 *  3. switch to csel
				 */
				after_change_udelay_hz(200000, rate/1000);
				cache_prefetch(LAB3,64);
				fast_iob();
			LAB3:
				cpccr = cpm_inl(CPM_CPCCR);     // reread cpccr
				cpccr_temp = cpccr | (1 << cpccr_clks[CDIV].ce) | (1 << cpccr_clks[L2CDIV].ce);
				cpccr_temp &= ~(3 << 28 | (0xf << cpccr_clks[CDIV].off) |(0xf << cpccr_clks[L2CDIV].off));
				l2div = l2div_policy(parentclk->rate,1);
				cpccr_temp |= (csel << 28 | l2div << cpccr_clks[L2CDIV].off) | (0 << cpccr_clks[CDIV].off);
				cpm_outl(cpccr_temp,CPM_CPCCR);
				cpccr_temp  &=  ~((1 << cpccr_clks[CDIV].ce) | (1 << cpccr_clks[L2CDIV].ce));
				cpm_outl(cpccr_temp,CPM_CPCCR);
				while(cpm_inl(CPM_CPCSR) & 1);
				clk->rate = parentclk->rate;
				get_clk_from_id(CLK_ID_L2CLK)->rate = parentclk->rate / (l2div + 1);
				sw_ahb_from_l2cache();
			}

		}
#else
		{
			unsigned int prev_rate = clk->rate;
			struct clk_notify_data dn;
			dn.current_rate = prev_rate;
			dn.target_rate = rate;
			ret = cclk_set_rate_nopll(clk,rate,parentclk,cpccr);
			sw_ahb_from_l2cache();
			spin_unlock_irqrestore(&cpm_cpccr_lock,flags);

			jz_notifier_call(NOTEFY_PROI_HIGH, JZ_CLK_CHANGING,&dn);
			jz_notifier_call(NOTEFY_PROI_HIGH, JZ_CLK_CHANGED,NULL);

			spin_lock_irqsave(&cpm_cpccr_lock,flags);
		}
#endif
		break;
	case H0DIV:
		set_cpccr_h0div(clk,rate);
		break;
	case H2DIV:
		set_cpccr_h2div(clk,rate);
		break;
	default:
		printk("no support set %s clk!\n",clk->name);

	}
	spin_unlock_irqrestore(&cpm_cpccr_lock,flags);
	return ret;
}
static unsigned long cpccr_get_rate(struct clk *clk)
{
	int sel;
	unsigned long cpccr = cpm_inl(CPM_CPCCR);
	unsigned int rate;
	int v;
	if(CLK_CPCCR_NO(clk->flags) == SCLKA)
	{
		int clka_sel[4] = {0,CLK_ID_EXT1,CLK_ID_APLL,0};
		sel = cpm_inl(CPM_CPCCR) >> 30;
		if(clka_sel[sel] == 0) {
			rate = 0;
			clk->flags &= ~CLK_FLG_ENABLE;
		}else {
			clk->parent = get_clk_from_id(clka_sel[sel]);
			rate = clk->parent->rate;
			clk->flags |= CLK_FLG_ENABLE;
		}

	}else{

		v = (cpccr >> cpccr_clks[CLK_CPCCR_NO(clk->flags)].off) & 0xf;
		sel = (cpccr >> (cpccr_clks[CLK_CPCCR_NO(clk->flags)].sel)) & 0x3;
		rate = get_clk_from_id(cpccr_selector[sel])->rate;
		rate = rate / (v + 1);
	}
	return rate;
}
static struct clk_ops clk_cpccr_ops = {
	.get_rate = cpccr_get_rate,
	.set_rate = cpccr_set_rate,
};
static struct jz_notifier ahb_change;
void __init init_cpccr_clk(struct clk *clk)
{
	int sel;	//check
	unsigned long cpccr = cpm_inl(CPM_CPCCR);
	if(CLK_CPCCR_NO(clk->flags) != SCLKA) {
		sel = (cpccr >> cpccr_clks[CLK_CPCCR_NO(clk->flags)].sel) & 0x3;
		if(cpccr_selector[sel] != 0) {
			clk->parent = get_clk_from_id(cpccr_selector[sel]);
			clk->flags |= CLK_FLG_ENABLE;
		}else {
			clk->parent = NULL;
			clk->flags &= ~CLK_FLG_ENABLE;
		}
	}
	clk->rate = cpccr_get_rate(clk);
	clk->ops = &clk_cpccr_ops;
	if(ahb_change.jz_notify == NULL) {
		ahb_change.jz_notify = ahb_change_notify;
		ahb_change.level = NOTEFY_PROI_NORMAL;
		ahb_change.msg = JZ_CLKGATE_CHANGE;
		jz_notifier_register(&ahb_change, NOTEFY_PROI_HIGH);
	}
}
