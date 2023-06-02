/*
 * arch/arm/mach-ambarella/pll_calc.c
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/rational.h>
#include <mach/init.h>
#include <plat/clk.h>
#include <plat/fio.h>
#include <plat/sd.h>
#include <plat/spi.h>

/* these two tables are actually not used */
struct pll_table ambarella_pll_frac_table[AMBARELLA_PLL_FRAC_TABLE_SIZE];
struct pll_table ambarella_pll_int_table[AMBARELLA_PLL_INT_TABLE_SIZE];

int ambarella_rct_clk_set_rate(struct clk *c, unsigned long rate)
{
	unsigned long rate_int, pre_scaler = 1, post_scaler = 1;
	unsigned long intp, sdiv = 1, sout = 1;
	u32 ctrl2, ctrl3, fix_divider = c->divider ? c->divider : 1;
	u64 dividend, divider, diff;
	union ctrl_reg_u ctrl_reg;
	union frac_reg_u frac_reg;

	BUG_ON(c->ctrl_reg == -1 || c->ctrl2_reg == -1 || c->ctrl3_reg == -1);

	rate *= fix_divider;

	if (rate < REF_CLK_FREQ && c->post_reg != -1) {
		rate *= 16;
		post_scaler = 16;
	}

	if (rate < REF_CLK_FREQ) {
		pr_err("Error: target rate is too slow: %ld!\n", rate);
		return -1;
	}

	//TODO: temporary solution for s3l 4Kp30 hdmi vout
	if(rate == (PLL_CLK_296_703MHZ * fix_divider)){
		pre_scaler = 4;
		intp = 0x63;
		sdiv = 5;
		sout = 1;
		c->frac_mode = 0;
	}else{
                /* fvco should be less than 1.5GHz, so we use 64 as max_numerator,
                 * although the actual bit width of pll_intp is 7 */
                rational_best_approximation(rate, REF_CLK_FREQ, 64, 16, &intp, &sout);

                /* fvco should be faster than 700MHz, otherwise pll out is not stable */
                while (REF_CLK_FREQ / 1000000 * intp * sdiv / pre_scaler < 700) {
                        if (sout > 8 || intp > 64)
                                break;
                        intp *= 2;
                        sout *= 2;
                }

                BUG_ON(pre_scaler > 16 || post_scaler > 16);
                BUG_ON(intp > 64 || sdiv > 16 || sout > 16 || sdiv > 16);
        }

	if (c->pres_reg != -1) {
		if (c->extra_scaler == 1)
			amba_rct_writel_en(c->pres_reg, (pre_scaler - 1) << 4);
		else
			amba_rct_writel(c->pres_reg, pre_scaler);
	}

	if (c->post_reg != -1) {
		if (c->extra_scaler == 1)
			amba_rct_writel_en(c->post_reg, (post_scaler - 1) << 4);
		else
			amba_rct_writel(c->post_reg, post_scaler);
	}

	ctrl_reg.w = amba_rct_readl(c->ctrl_reg);
	ctrl_reg.s.intp = intp - 1;
	ctrl_reg.s.sdiv = sdiv - 1;
	ctrl_reg.s.sout = sout - 1;
	ctrl_reg.s.bypass = 0;
	ctrl_reg.s.frac_mode = 0;
	ctrl_reg.s.force_reset = 0;
	ctrl_reg.s.power_down = 0;
	ctrl_reg.s.halt_vco = 0;
	ctrl_reg.s.tristate = 0;
	ctrl_reg.s.force_lock = 1;
	ctrl_reg.s.force_bypass = 0;
	ctrl_reg.s.write_enable = 0;
	amba_rct_writel_en(c->ctrl_reg, ctrl_reg.w);

	if (c->frac_mode) {
		rate_int = ambarella_rct_clk_get_rate(c) * fix_divider * post_scaler;
		if (rate_int <= rate)
			diff = rate - rate_int;
		else
			diff = rate_int - rate;

		if (diff) {
			dividend = diff * pre_scaler * sout;
			dividend = dividend << 32;
			divider = (u64)sdiv * REF_CLK_FREQ;
			dividend = DIV_ROUND_CLOSEST_ULL(dividend, divider);
			if (rate_int <= rate) {
				frac_reg.s.nega	= 0;
				frac_reg.s.frac	= dividend;
			} else {
				frac_reg.s.nega	= 1;
				frac_reg.s.frac	= 0x80000000 - dividend;
			}
			amba_rct_writel(c->frac_reg, frac_reg.w);

			ctrl_reg.s.frac_mode = 1;
		}

		amba_rct_writel_en(c->ctrl_reg, ctrl_reg.w);
	}

	ctrl2 = c->ctrl2_val ? c->ctrl2_val : 0x3f770000;
	ctrl3 = c->ctrl3_val ? c->ctrl3_val : ctrl_reg.s.frac_mode ? 0x00069300 : 0x00068300;
	if(rate == (PLL_CLK_296_703MHZ * fix_divider)){
		ctrl2 = 0x3f700e00;
		ctrl3 = 0x00068306;
	}
	amba_rct_writel(c->ctrl2_reg, ctrl2);
	amba_rct_writel(c->ctrl3_reg, ctrl3);

	c->rate = ambarella_rct_clk_get_rate(c);

	/* check if result rate is precise or not */
	if (abs(c->rate - rate / fix_divider / post_scaler) > 10) {
		pr_warn("%s: rate is not very precise: %lld, %ld\n",
			c->name, c->rate, rate / fix_divider / post_scaler);
	}

	return 0;
}
EXPORT_SYMBOL(ambarella_rct_clk_set_rate);

