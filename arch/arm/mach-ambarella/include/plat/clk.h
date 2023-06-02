/*
 * arch/arm/plat-ambarella/include/plat/clk.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef __PLAT_AMBARELLA_CLK_H__
#define __PLAT_AMBARELLA_CLK_H__

#include <plat/rct.h>

/* ==========================================================================*/
enum PLL_CLK_HZ {
	PLL_CLK_10D1001MHZ	= 9990010,
	PLL_CLK_10MHZ		= 10000000,
	PLL_CLK_13D1001MHZ	= 12987012,
	PLL_CLK_13MHZ		= 13000000,
	PLL_CLK_13_5D1001MHZ	= 13486513,
	PLL_CLK_13_5MHZ		= 13500000,
	PLL_CLK_17_97515MHZ	= 17975154,
	PLL_CLK_17_97554MHZ	= 17975544,
	PLL_CLK_17_98201MHZ	= 17982018,
	PLL_CLK_18_44MHz	= 18440000,
	PLL_CLK_22_5MHZ		= 22500000,
	PLL_CLK_23_9772MHZ	= 23977223,
	PLL_CLK_23_9784MHZ	= 23978422,
	PLL_CLK_23_99MHZ	= 23998277,
	PLL_CLK_23_967MHZ	= 23967392,
	PLL_CLK_23_971MHZ	= 23971444,
	PLL_CLK_24D1001MHZ	= 23976024,
	PLL_CLK_23_996MHZ	= 23996483,
	PLL_CLK_24MHZ		= 24000000,
	PLL_CLK_24M1001MHZ	= 24024000,
	PLL_CLK_24_072MHZ	= 24072001,
	PLL_CLK_24_3MHZ		= 24300000,
	PLL_CLK_24_54MHZ	= 24545430,
	PLL_CLK_25MHZ		= 25000000,
	PLL_CLK_27D1001MHZ	= 26973027,
	PLL_CLK_26_9823MHZ	= 26982318,
	PLL_CLK_26_9485MHZ	= 26948544,
	PLL_CLK_26_9568MHZ	= 26956800,
	PLL_CLK_27MHZ		= 27000000,
	PLL_CLK_27_0432MHZ	= 27043200,
	PLL_CLK_27_0514MHZ	= 27051402,
	PLL_CLK_27M1001MHZ	= 27027000,
	PLL_CLK_30MHZ		= 30000000,
	PLL_CLK_27_1792MHZ	= 27179210,
	PLL_CLK_32_5D1001MHZ	= 32467532,
	PLL_CLK_36D1001MHZ	= 35964036,
	PLL_CLK_36MHZ		= 36000000,
	PLL_CLK_36_20MHZ	= 36197802,
	PLL_CLK_36_23MHZ	= 36234000,
	PLL_CLK_37_125D1001MHZ	= 37087912,
	PLL_CLK_37_125MHZ	= 37125000,
	PLL_CLK_42D1001MHZ	= 41958042,
	PLL_CLK_42MHZ		= 42000000,
	PLL_CLK_45MHZ		= 45000000,
	PLL_CLK_48D1001MHZ	= 47952048,
	PLL_CLK_48MHZ		= 48000000,
	PLL_CLK_48_6MHZ		= 48600000,
	PLL_CLK_49_5D1001MHZ	= 49450549,
	PLL_CLK_49_5MHZ		= 49500000,
	PLL_CLK_54MHZ		= 54000000,
	PLL_CLK_54M1001MHZ	= 54054000,
	PLL_CLK_59_4MHZ		= 59400000,
	PLL_CLK_60MHZ		= 60000000,
	PLL_CLK_60M1001MHZ	= 60060000,
	PLL_CLK_60_05MHz	= 60054545,
	PLL_CLK_60_16MHZ	= 60164835,
	PLL_CLK_60_18MHZ	= 60183566,
	PLL_CLK_60_29MHZ	= 60285794,
	PLL_CLK_60_33MHZ	= 60329670,
	PLL_CLK_60_35MHZ	= 60346080,
	PLL_CLK_60_39MHZ	= 60390000,
	PLL_CLK_60_48MHz	= 60480000,
	PLL_CLK_64D1001MHZ	= 63936064,
	PLL_CLK_64MHZ		= 64000000,
	PLL_CLK_65D1001MHZ	= 64935064,
	PLL_CLK_72D1001MHZ	= 71928072,
	PLL_CLK_72MHZ		= 72000000,
	PLL_CLK_74_25D1001MHZ	= 74175824,
	PLL_CLK_74_25MHZ	= 74250000,
	PLL_CLK_80MHZ		= 80000000,
	PLL_CLK_90D1001MHZ	= 89910090,
	PLL_CLK_90MHZ		= 90000000,
	PLL_CLK_90_62D1001MHZ	= 90525314,
	PLL_CLK_90_62MHZ	= 90615840,
	PLL_CLK_90_69D1001MHZ	= 90596763,
	PLL_CLK_90_69MHZ	= 90687360,
	PLL_CLK_95_993D1001MHZ	= 95896903,
	PLL_CLK_96D1001MHZ	= 95904096,
	PLL_CLK_96MHZ		= 96000000,
	PLL_CLK_99D1001MHZ	= 98901099,
	PLL_CLK_99MHZ		= 99000000,
	PLL_CLK_99_18D1001MHZ	= 99081439,
	PLL_CLK_99_18MHZ	= 99180720,
	PLL_CLK_108MHZ		= 108000000,
	PLL_CLK_148_5D1001MHZ	= 148351648,
	PLL_CLK_148_5MHZ	= 148500000,
	PLL_CLK_120MHZ		= 120000000,
	PLL_CLK_144MHZ		= 144000000,
	PLL_CLK_150MHZ		= 150000000,
	PLL_CLK_156MHZ		= 156000000,
	PLL_CLK_160MHZ		= 160000000,
	PLL_CLK_192MHZ		= 192000000,
	PLL_CLK_216MHZ		= 216000000,
	PLL_CLK_230_4MHZ	= 230400000,
	PLL_CLK_240MHZ		= 240000000,
	PLL_CLK_288MHZ		= 288000000,
	PLL_CLK_296_703MHZ	= 296703000,
	PLL_CLK_297MHZ		= 297000000,
	PLL_CLK_320MHZ		= 320000000,
	PLL_CLK_384MHZ		= 384000000,
	PLL_CLK_495MHZ		= 495000000,
	PLL_CLK_594MHZ		= 594000000,
};

struct clk;
struct clk_ops {
	int			(*enable)(struct clk *c);
	int			(*disable)(struct clk *c);
	unsigned long		(*get_rate)(struct clk *c);
	unsigned long		(*round_rate)(struct clk *c, unsigned long rate);
	int			(*set_rate)(struct clk *c, unsigned long rate);
	int			(*set_parent)(struct clk *c, struct clk *parent);
};

struct clk {
	struct list_head	list;
	struct clk		*parent;
	const char		*name;
	u64			rate;
	u32			frac_mode;
	u32			ctrl_reg;
	u32			pres_reg;
	u32			pres_val;
	u32			post_reg;
	u32			post_val;
	u32			frac_reg;
	u32			ctrl2_reg;
	u32			ctrl2_val;
	u32			ctrl3_reg;
	u32			ctrl3_val;
	u32			lock_reg;
	u32			lock_bit;
	u32			divider;
	u32			max_divider;
	u32			extra_scaler;
	struct pll_table	*table;
	u32			table_size;
	struct clk_ops		*ops;
};

struct pll_table {
	u64	multiplier; /* pll_out / (ref_clk / pre_scaler) */

	u32	intp;
	u32	sdiv;
	u32	sout;
	u32	post;
};

union ctrl_reg_u {
	struct {
		u32	write_enable		: 1;	/* [0] */
		u32	reserved1		: 1;	/* [1] */
		u32	bypass			: 1;	/* [2] */
		u32	frac_mode		: 1;	/* [3] */
		u32	force_reset		: 1;	/* [4] */
		u32	power_down		: 1;	/* [5] */
		u32	halt_vco		: 1;	/* [6] */
		u32	tristate		: 1;	/* [7] */
		u32	tout_async		: 4;	/* [11:8] */
		u32	sdiv			: 4;	/* [15:12] */
		u32	sout			: 4;	/* [19:16] */
		u32	force_lock		: 1;	/* [20] */
		u32	force_bypass		: 1;	/* [21] */
		u32	reserved2		: 2;	/* [23:22] */
		u32	intp			: 7;	/* [30:24] */
		u32	reserved3		: 1;	/* [31] */
	} s;
	u32	w;
};

union frac_reg_u {
	struct {
		u32	frac			: 31;	/* [30:0] */
		u32	nega			: 1;	/* [31] */
	} s;
	u32	w;
};

/* ==========================================================================*/
#ifndef __ASSEMBLER__

#define AMBCLK_DO_DIV(divident, divider)	do {	\
	do_div((divident), (divider));			\
	} while (0)

#define AMBCLK_DO_DIV_ROUND(divident, divider)	do {	\
	(divident) += ((divider) >> 1);			\
	do_div((divident), (divider));			\
	} while (0)

extern struct clk_ops ambarella_rct_pll_ops;
extern struct clk_ops ambarella_rct_scaler_ops;

#ifdef CONFIG_AMBARELLA_CALC_PLL
#define AMBARELLA_PLL_FRAC_TABLE_SIZE		(0)
extern struct pll_table ambarella_pll_frac_table[AMBARELLA_PLL_FRAC_TABLE_SIZE];
#define AMBARELLA_PLL_INT_TABLE_SIZE		(0)
extern struct pll_table ambarella_pll_int_table[AMBARELLA_PLL_INT_TABLE_SIZE];
#else
#define AMBARELLA_PLL_FRAC_TABLE_SIZE		(590)
extern struct pll_table ambarella_pll_frac_table[AMBARELLA_PLL_FRAC_TABLE_SIZE];
#define AMBARELLA_PLL_INT_TABLE_SIZE		(93)
extern struct pll_table ambarella_pll_int_table[AMBARELLA_PLL_INT_TABLE_SIZE];
#define AMBARELLA_PLL_VOUT_TABLE_SIZE		(797)
extern struct pll_table ambarella_pll_vout_table[AMBARELLA_PLL_VOUT_TABLE_SIZE];
#define AMBARELLA_PLL_VOUT2_TABLE_SIZE		(793)
extern struct pll_table ambarella_pll_vout2_table[AMBARELLA_PLL_VOUT2_TABLE_SIZE];
#endif

extern u32 ambarella_rct_find_pll_table_index(unsigned long rate,
		u32 pre_scaler, const struct pll_table *table, u32 table_size);

extern unsigned long ambarella_rct_clk_get_rate(struct clk *c);
extern int ambarella_rct_clk_set_rate(struct clk *c, unsigned long rate);
extern int ambarella_rct_clk_adj_rate(struct clk *c, unsigned long rate);
extern int ambarella_rct_clk_enable(struct clk *c);
extern int ambarella_rct_clk_disable(struct clk *c);

extern unsigned long ambarella_rct_scaler_get_rate(struct clk *c);
extern int ambarella_rct_scaler_set_rate(struct clk *c, unsigned long rate);

extern int ambarella_clk_init(void);
extern int ambarella_clk_add(struct clk *clk);
extern unsigned int ambarella_clk_get_ref_freq(void);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

