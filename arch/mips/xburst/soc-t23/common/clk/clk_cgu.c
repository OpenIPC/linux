#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <soc/cache.h>
#include <soc/cpm.h>
#include <soc/base.h>
#include <soc/extal.h>
#include <soc/ddr.h>
#include "clk.h"
static DEFINE_SPINLOCK(cpm_cgu_lock);
struct clk_selectors {
	unsigned int route[4];
};
enum {
	SELECTOR_A = 0,
	SELECTOR_2,
	SELECTOR_C,
	SELECTOR_3,
	SELECTOR_MSC_MUX,
	SELECTOR_F,
	SELECTOR_H
};
const struct clk_selectors selector[] = {
#define CLK(X)  CLK_ID_##X
/*
 *         bit31,bit30
 *          0   , 0       STOP
 *          0   , 1       SCLKA
 *          1   , 0       MPLL
 *          1   , 1       INVALID
 */
	[SELECTOR_A].route = {CLK(STOP),CLK(SCLKA),CLK(MPLL),CLK(INVALID)},
/*
 *         bit31,bit30
 *          0   , x       SCLKA
 *          0   , x       SCLKA
 *          1   , x       MPLL
 *          1   , x       MPLL
 */
	[SELECTOR_2].route  = {CLK(SCLKA),CLK(SCLKA),CLK(MPLL),CLK(MPLL)},
/*
 *         bit31,bit30
 *          0   , 0       EXT1
 *          0   , 1       EXT1
 *          1   , 0       SCLKA
 *          1   , 1       MPLL
 */
	[SELECTOR_C].route = {CLK(EXT1) ,CLK(EXT1),CLK(SCLKA),CLK(MPLL)},
/*
 *         bit31,bit30
 *          0   , 0       SCLKA
 *          0   , 1       MPLL
 *          1   , 0       INVALID
 *          1   , 1       INVALID
 */
	[SELECTOR_3].route = {CLK(SCLKA),CLK(MPLL),CLK(INVALID),CLK(INVALID)},

/*
 *         bit31,bit30
 *          0   , 0       MSC_MUX
 *          0   , 1       MSC_MUX
 *          1   , 0       MSC_MUX
 *          1   , 1       MSC_MUX
 */
	[SELECTOR_MSC_MUX].route = {CLK(SCLKA),CLK(SCLKA),CLK(MPLL),CLK(MPLL)},
/*
 *         bit31,bit30
 *          0   , x       SCLKA
 *          0   , x       SCLKA
 *          1   , x       OTGPHY
 *          1   , x       OTGPHY
 */
	[SELECTOR_F].route = {CLK(SCLKA),CLK(SCLKA),CLK(OTGPHY),CLK(OTGPHY)},
	/*
	 *		   bit31,bit30
	 *			0	, 0 	  SCLKA
	 *			0	, 1 	  MPLL
	 *			1	, 0 	  INVALID
	 *			1	, 1 	  INVALID
	 */
	[SELECTOR_H].route = {CLK(SCLKA),CLK(MPLL),CLK(INVALID),CLK(INVALID)},

#undef CLK
};

#define IS_CGU_CLK(x) (x&CLK_FLG_CGU)

struct cgu_clk {
	/* off: reg offset. ce_busy_stop: CE offset  + 1 is busy. coe : coe for div .div: div bit width */
	/* ext: extal/pll sel bit. sels: {select} */
	int off,ce_busy_stop,coe,div,sel,cache;
};
static struct cgu_clk cgu_clks[] = {
	[CGU_DDR] = 	{ CPM_DDRCDR, 27, 1, 4, SELECTOR_A},
	[CGU_VPU] = 	{ CPM_VPUCDR, 27, 1, 4, SELECTOR_H},
	[CGU_MACPHY] = 	{ CPM_MACCDR, 27, 1, 8, SELECTOR_H},
	[CGU_RSA] = 	{ CPM_RSACDR, 27, 1, 4, SELECTOR_H},
	[CGU_LPC] = 	{ CPM_LPCDR,  26, 1, 8, SELECTOR_H},
	[CGU_MSC_MUX]=	{ CPM_MSC0CDR, 27, 2, 0, SELECTOR_MSC_MUX},/*TODO:what does it mean?*/
	[CGU_MSC0] = 	{ CPM_MSC0CDR, 27, 2, 8, SELECTOR_H},
	[CGU_MSC1] = 	{ CPM_MSC1CDR, 27, 2, 8, SELECTOR_H},
	[CGU_I2S_SPK] = 	{ CPM_I2SSPKCDR, 29, 0, 20, SELECTOR_H}, /*TODO:check*/
	[CGU_I2S_MIC] = 	{ CPM_I2SMICCDR, 29, 0, 20, SELECTOR_H}, /*TODO:check*/
	[CGU_SSI] = 	{ CPM_SSICDR, 26, 1, 8, SELECTOR_H},
	[CGU_SFC] = 	{ CPM_SFCCDR, 26, 1, 8, SELECTOR_H},
	[CGU_CIM] =     { CPM_CIMCDR, 27, 1, 8, SELECTOR_H},
	[CGU_ISP] = 	{ CPM_ISPCDR, 27, 1, 4, SELECTOR_H},
};

static unsigned long cgu_get_rate(struct clk *clk)
{
	unsigned long x;
	unsigned long flags;
	int no = CLK_CGU_NO(clk->flags);

	if (!(strcmp(clk->name, "cgu_i2s_spk")) || !(strcmp(clk->name, "cgu_i2s_mic"))) {
		unsigned int reg_val = 0;
		int m = 0, n = 0;
		reg_val = cpm_inl(cgu_clks[no].off);
		n = reg_val & 0xfffff;
		m = (reg_val >> 20) & 0x1ff;

		printk(KERN_DEBUG"%s, parent = %ld, rate = %ld, m = %d, n = %d, reg val = 0x%08x\n",
				__func__, clk->parent->rate, clk->rate, m, n, cpm_inl(cgu_clks[no].off));
		return (clk->parent->rate * m) / n;
	}

	if(clk->parent == get_clk_from_id(CLK_ID_EXT1))
		return clk->parent->rate;
	if(no == CGU_MSC_MUX)
		return clk->parent->rate;
	if(cgu_clks[no].div == 0)
		return clk_get_rate(clk->parent);
	spin_lock_irqsave(&cpm_cgu_lock,flags);
	x = cpm_inl(cgu_clks[no].off);
	x &= (1 << cgu_clks[no].div) - 1;
	x = (x + 1) * cgu_clks[no].coe;

	spin_unlock_irqrestore(&cpm_cgu_lock,flags);
	return clk->parent->rate / x;
}
static int cgu_enable(struct clk *clk,int on)
{
	int no = CLK_CGU_NO(clk->flags);
	int reg_val;
	int ce,stop,busy;
	int prev_on;
	unsigned int mask;
	unsigned long flags;
	if(no == CGU_MSC_MUX)
		return 0;

	spin_lock_irqsave(&cpm_cgu_lock,flags);

	if (!(strcmp(clk->name, "cgu_i2s_spk")) || !(strcmp(clk->name, "cgu_i2s_mic"))) {
		reg_val = cpm_inl(cgu_clks[no].off);
		reg_val |= (1 << 29);
		cpm_outl(reg_val,cgu_clks[no].off);
		printk(KERN_DEBUG"%s,%s reg val = 0x%08x\n",
				__func__, clk->name, cpm_inl(cgu_clks[no].off));
		goto cgu_enable_finish;
	}

	reg_val = cpm_inl(cgu_clks[no].off);
	stop = cgu_clks[no].ce_busy_stop;
	busy = stop + 1;
	ce = stop + 2;
	prev_on = !(reg_val & (1 << stop));
	mask = (1 << cgu_clks[no].div) - 1;
	if(prev_on && on)
		goto cgu_enable_finish;
	if((!prev_on) && (!on))
		goto cgu_enable_finish;
	if(on){
		if(cgu_clks[no].cache && ((cgu_clks[no].cache & mask) != (reg_val & mask))) {
			unsigned int x = cgu_clks[no].cache;
			x = (x & ~(0x1 << stop)) | (0x1 << ce);
			cpm_outl(x,cgu_clks[no].off);
			while(cpm_test_bit(busy,cgu_clks[no].off)) {
				printk("wait stable.[%d][%s]\n",__LINE__,clk->name);
			}
			cpm_clear_bit(ce, cgu_clks[no].off);
			x &= (1 << cgu_clks[no].div) - 1;
			x = (x + 1) * cgu_clks[no].coe;
			clk->rate = clk->parent->rate / x;
			cgu_clks[no].cache = 0;
		} else {
			reg_val |= (1 << ce);
			reg_val &= ~(1 << stop);
			cpm_outl(reg_val,cgu_clks[no].off);
			cpm_clear_bit(ce,cgu_clks[no].off);
		}
	} else {
		reg_val |= (1 << ce);
		reg_val |= ( 1<< stop);
		cpm_outl(reg_val,cgu_clks[no].off);
		cpm_clear_bit(ce,cgu_clks[no].off);
	}

cgu_enable_finish:
	spin_unlock_irqrestore(&cpm_cgu_lock,flags);
	return 0;
}

static int cgu_set_rate(struct clk *clk, unsigned long rate)
{
	unsigned long x,tmp;
	int i,no = CLK_CGU_NO(clk->flags);
	int ce,stop,busy;

	unsigned int reg_val, mask;
	unsigned long flags;
	unsigned long long m, n;
	unsigned long long m_mul;
	unsigned long long tmp_value;
	unsigned long long tmp_rate = (unsigned long long)rate;
	unsigned char sig = 0;
	unsigned long long max = 0xfffff;
	unsigned long long min = 1;
	/* CLK_ID_CGU_I2S could be exten clk. */
	//if(clk->parent == get_clk_from_id(CLK_ID_EXT1) && (clk->CLK_ID != CLK_ID_CGU_I2S))
	//    //return -1;
	//
	if(no == CGU_MSC_MUX)
		return -1;
	if (!(strcmp(clk->name, "cgu_i2s_spk")) || !(strcmp(clk->name, "cgu_i2s_mic"))) {
		for(m=1;m<=0x1ff;m++)
		{
			m_mul = clk->parent->rate * m;
			max = 0xfffff;
			min = 1;
			while(max >= min){
				n = (max + min) >> 1;
				tmp_value = (unsigned long long)(n*tmp_rate);
				if( m_mul == tmp_value )
				{
					sig = 1;
					break;
				}else if (tmp_value > m_mul)
					max = n - 1;
				else
					min = n + 1;
			}
			if( sig )
			{
				clk->rate = rate;
				break;
			}
		}

		reg_val = cpm_inl(cgu_clks[no].off) & 0xe0000000;
		reg_val |= (m << 20) | (n << 0);
		cpm_outl(reg_val,cgu_clks[no].off);
		printk(KERN_DEBUG"%s, parent = %ld, rate = %ld, n = %lld, reg val = 0x%08x\n",
				__func__, clk->parent->rate, clk->rate, n, cpm_inl(cgu_clks[no].off));
		return 0;
	}
	spin_lock_irqsave(&cpm_cgu_lock,flags);
	mask = (1 << cgu_clks[no].div) - 1;
	tmp = clk->parent->rate / cgu_clks[no].coe;
	for (i = 1; i <= mask+1; i++) {
		if ((tmp / i) <= rate)
			break;
	}
	i--;
	if(i > mask)
		i = mask;
	reg_val = cpm_inl(cgu_clks[no].off);
	x = reg_val & ~mask;
	x |= i;
	stop = cgu_clks[no].ce_busy_stop;
	busy = stop + 1;
	ce = stop + 2;
	if(x & (1 << stop)) {
		if (cgu_clks[no].cache) {
			cgu_clks[no].cache &= ~mask;
			cgu_clks[no].cache |= i;
		} else {
			cgu_clks[no].cache = x;
		}
		clk->rate = tmp  / (i + 1);
	}
	else if((mask & reg_val) != i){

		x = (x & ~(0x1 << stop)) | (0x1 << ce);
		cpm_outl(x, cgu_clks[no].off);
		while(cpm_test_bit(busy,cgu_clks[no].off))
			printk("wait stable.[%d][%s]\n",__LINE__,clk->name);
		x &= ~(1 << ce);
		cpm_outl(x, cgu_clks[no].off);
		cgu_clks[no].cache = 0;
		clk->rate = tmp  / (i + 1);
	}
	spin_unlock_irqrestore(&cpm_cgu_lock,flags);
	return 0;
}

static struct clk * cgu_get_parent(struct clk *clk)
{
	unsigned int no,cgu,idx,pidx;

	no = CLK_CGU_NO(clk->flags);
	if(!(strcmp(clk->name, "cgu_i2s_mic"))){
		/* the operation uses for t31, because that the bits[30~31] of 0x10000084 is readonly */
		cgu = *(volatile unsigned int*)(0xb0000070); // the register is i2s speaker pll.
	}else{
		cgu = cpm_inl(cgu_clks[no].off);
	}
	idx = cgu >> 30;
	pidx = selector[cgu_clks[no].sel].route[idx];
	if (pidx == CLK_ID_STOP || pidx == CLK_ID_INVALID)
		return NULL;

	return get_clk_from_id(pidx);
}

static int cgu_set_parent(struct clk *clk, struct clk *parent)
{
	int i,tmp;
	int no = CLK_CGU_NO(clk->flags);
	unsigned int reg_val,cgu,mask;
	int ce,stop,busy;
	unsigned long flags;
	stop = cgu_clks[no].ce_busy_stop;
	busy = stop + 1;
	ce = stop + 2;
	mask = (1 << cgu_clks[no].div) - 1;

	for(i = 0;i < 4;i++) {
		if(selector[cgu_clks[no].sel].route[i] == get_clk_id(parent)){
			break;
		}
	}
	if(i >= 4)
		return -EINVAL;
	spin_lock_irqsave(&cpm_cgu_lock,flags);
	cgu = cpm_inl(cgu_clks[no].off);
	reg_val = cgu;
	if(cgu_clks[no].sel == SELECTOR_2) {
		if(i == 0)
			cgu &= ~(1 << 31);
		else
			cgu |= (1 << 31);
	}else {
		cgu &= ~(3 << 30);
		cgu |= i << 30;
	}

	tmp = parent->rate / cgu_clks[no].coe;
	for (i = 1; i <= mask+1; i++) {
		if ((tmp / i) <= clk->rate)
			break;
	}
	i--;
	mask = (1 << cgu_clks[no].div) - 1;
	cgu = (cgu & ~(0x1 << stop)) | (0x1 << ce);
	cgu = cgu & ~mask;
	cgu |= i;

	if(reg_val & (1 << stop))
		cgu_clks[no].cache = cgu;
	else if((mask & reg_val) != i){
		cpm_outl(cgu, cgu_clks[no].off);
		while(cpm_test_bit(busy,cgu_clks[no].off))
			printk("wait stable.[%d][%s]\n",__LINE__,clk->name);
		cgu &= ~(1 << ce);
		cpm_outl(cgu, cgu_clks[no].off);
		cgu_clks[no].cache = 0;
	}

	clk->parent = parent;

	spin_unlock_irqrestore(&cpm_cgu_lock,flags);
	return 0;
}

static int cgu_is_enabled(struct clk *clk) {
	int no = CLK_CGU_NO(clk->flags);
	int stop;
	stop = cgu_clks[no].ce_busy_stop;
	return !(cpm_inl(cgu_clks[no].off) & (1 << stop));
}
static struct clk_ops clk_cgu_ops = {
	.enable	= cgu_enable,
	.get_rate = cgu_get_rate,
	.set_rate = cgu_set_rate,
	.get_parent = cgu_get_parent,
	.set_parent = cgu_set_parent,
};

#define CONVERT_DDR_RATE (50 * 1000 * 1000)
#define UP_DIV 1
#define DOWN_DIV 2
#define REG32(x) *(volatile unsigned int *)(x)

#define OFF_TDR         (0x00)
#define OFF_LCR         (0x0C)
#define OFF_LSR         (0x14)

#define LSR_TDRQ        (1 << 5)
#define LSR_TEMT        (1 << 6)

#define U1_IOBASE (UART1_IOBASE + 0xa0000000)
#define TCSM_PCHAR(x)							\
	*((volatile unsigned int*)(U1_IOBASE+OFF_TDR)) = x;		\
	while ((*((volatile unsigned int*)(U1_IOBASE + OFF_LSR)) & (LSR_TDRQ | LSR_TEMT)) != (LSR_TDRQ | LSR_TEMT))

static inline void serial_put_hex(unsigned int x) {
	int i;
	unsigned int d;
	for(i = 7;i >= 0;i--) {
		d = (x  >> (i * 4)) & 0xf;
		if(d < 10) d += '0';
		else d += 'A' - 10;
		TCSM_PCHAR(d);
	}
}
static int ddr_set_rate(struct clk *clk, unsigned long rate)
{
	unsigned int ddr_cgu = cpm_inl(CPM_DDRCDR);
	unsigned int div = ddr_cgu & 0xf;
	unsigned int up = 0;
	unsigned int ret = 0;
	unsigned long flags,fail;
	printk("aaaaaaaaaaaaaaaaa\n");
	if(rate > CONVERT_DDR_RATE){
		if(clk->rate <= CONVERT_DDR_RATE) {
			up = UP_DIV;
			div >>= 1;
			clk->rate = 100 * 1000 * 1000;
		}
	}else{
		if(clk->rate > CONVERT_DDR_RATE) {
			up = DOWN_DIV;
			div <<= 1;
			clk->rate = 50 * 1000 * 1000;
		}
	}
	printk("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa div= %d\n",div);
	spin_lock_irqsave(&cpm_cgu_lock,flags);
	cache_prefetch(DDRCLAB1,200);
	fast_iob();
DDRCLAB1:
	REG32(0xb3012068) = 0;

	ddr_cgu |= (1 << 29) | (1 << 26);
	ddr_cgu &= ~(1 << 24);
	ddr_cgu &= ~0xf;
	TCSM_PCHAR('1');
	cpm_outl(ddr_cgu,CPM_DDRCDR);
	TCSM_PCHAR('a');
	ddr_cgu |= (1 << 25);
	ddr_cgu |= div;
	TCSM_PCHAR('2');
	cpm_outl(ddr_cgu,CPM_DDRCDR);
	fail = 0;
	TCSM_PCHAR('3');
	while(cpm_inl(CPM_DDRCDR) & 1 << 28) {
		serial_put_hex(cpm_inl(CPM_DDRCDR));
		TCSM_PCHAR('\r');
		TCSM_PCHAR('\n');
	}
	TCSM_PCHAR('4');
	cpm_clear_bit(25,CPM_DDRCDR);
	REG32(0xb3012068) = 0xf0000100;
        fail = REG32(0xa0000000);
	if(cpm_inl(CPM_DDRCDR) & 1 << 24) {
		printk("fail!\n");
	}else
		printk("ok!\n");

	//printk("REG32(0xb301206c) = %x\n",REG32(0xb301206c));

	spin_unlock_irqrestore(&cpm_cgu_lock,flags);
	return ret;

}
static struct clk_ops clk_ddr_ops = {
	.enable	= cgu_enable,
	.get_rate = cgu_get_rate,
	.set_rate = ddr_set_rate,
	.get_parent = cgu_get_parent,
	.set_parent = cgu_set_parent,

};
void __init init_cgu_clk(struct clk *clk)
{
	int no;
	int id;
	//printk("REG32(0xb3012088) = %x\n",REG32(0xb3012088));


	if (clk->flags & CLK_FLG_PARENT) {
		id = CLK_PARENT(clk->flags);
		clk->parent = get_clk_from_id(id);
	} else {
		clk->parent = cgu_get_parent(clk);
	}
	no = CLK_CGU_NO(clk->flags);
	cgu_clks[no].cache = 0;
	clk->rate = cgu_get_rate(clk);
	if(cgu_is_enabled(clk)) {
		clk->flags |= CLK_FLG_ENABLE;
	}

	if(no == CGU_MSC_MUX)
		clk->ops = NULL;
	else if(no == CGU_DDR){
		clk->ops = &clk_ddr_ops;
		if(ddr_readl(DDRP_PIR) & DDRP_PIR_DLLBYP){  /*TODO:zm*/
                    /**
                     * DDR request cpm to stop clk
                     * (0x9 << 28) DDR_CLKSTP_CFG (0x13012068)
                     * CPM response ddr stop clk request (1 << 26) (0x1000002c)
                     */
                    cpm_set_bit(26,CPM_DDRCDR);
                    REG32(0xb3012068) |= 0x9 << 28;
                }
		REG32(0xb3012088) |= 4 << 16;
	}
	else
		clk->ops = &clk_cgu_ops;

}
