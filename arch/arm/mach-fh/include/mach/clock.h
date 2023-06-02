/*
 *	Copyright (c) 2010 Shanghai Fullhan Microelectronics Co., Ltd.
 *				All Rights Reserved. Confidential.
 *
 *This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __ASM_ARCH_FH_CLOCK_H
#define __ASM_ARCH_FH_CLOCK_H

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



#define CLK_IOCTL_MAGIC 		'c'
#define ENABLE_CLK	 		_IOWR(CLK_IOCTL_MAGIC, 0, unsigned int)
#define DISABLE_CLK		 	_IOWR(CLK_IOCTL_MAGIC, 1, unsigned int)
#define SET_CLK_RATE			_IOWR(CLK_IOCTL_MAGIC, 2, unsigned int)
#define GET_CLK_RATE			_IOWR(CLK_IOCTL_MAGIC, 3, unsigned int)
#define SET_PMU				_IOWR(CLK_IOCTL_MAGIC, 4, unsigned int)
#define GET_PMU				_IOWR(CLK_IOCTL_MAGIC, 5, unsigned int)

#define CLK_IOCTL_MAXNR 		8


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


struct clk {
	struct list_head    list;
	const char          *name;
	unsigned long       frequency;
	unsigned int        flag;
	int                 select;
	struct clk         *parent[CLOCK_MAX_PARENT];
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

extern int clk_register(struct clk *clk);
extern void clk_unregister(struct clk *clk);

void clk_set_clk_sel(unsigned int reg);
unsigned int clk_get_clk_sel(void);

int fh_clk_init(void);
int fh_clk_procfs_init(void);
int fh_clk_misc_init(void);

extern struct clk_lookup fh_clks[];

#endif
