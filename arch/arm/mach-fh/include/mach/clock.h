#ifndef __FH_CLOCK_H__
#define __FH_CLOCK_H__

#include <linux/list.h>
#include <linux/clkdev.h>

#define CLOCK_MAX_PARENT    4

#define OSC_FREQUENCY		(24000000)

#define CLOCK_FIXED         	(1<<0)
#define CLOCK_NOGATE        	(1<<1)
#define CLOCK_NODIV         	(1<<2)
#define CLOCK_NORESET       	(1<<3)
#define CLOCK_MULTI_PARENT  	(1<<4)
#define CLOCK_PLL				(1<<5)
#define CLOCK_PLL_P				(1<<6)
#define CLOCK_PLL_R				(1<<7)
#define CLOCK_HIDE				(1<<8)
#define CLOCK_CIS				(1<<9)
#define CLOCK_PHASE				(1<<10)


#define CLK(dev, con, ck) 	\
		{					\
			.dev_id = dev,	\
			.con_id = con,	\
			.clk = ck,		\
		}

struct clk_usr {
	char			*name;
	unsigned long       	frequency;
};


struct fh_clk {
	const char          *name;
	unsigned long       frequency;
	unsigned int        flag;
	int                 select;
	struct fh_clk         *parent[CLOCK_MAX_PARENT];
	int                 prediv;
	int                 divide;
	unsigned int        div_reg_offset;
	unsigned int        div_reg_mask;
	unsigned int        en_reg_offset;
	unsigned int        en_reg_mask;
	unsigned int        rst_reg_offset;
	unsigned int        rst_reg_mask;
	unsigned int        sel_reg_offset;
	unsigned int        sel_reg_mask;
	unsigned int        def_rate;
};

extern struct fh_clk *fh_clks[];
extern int __init fh_clk_init(void);

#endif
