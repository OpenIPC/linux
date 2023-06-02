/*
 *  Copyright (C) 2010 Faraday Technology
 *  Copyright (C) 2010 Po-Yu Chuang <ratbert@faraday-tech.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __ARCH_ARM_FARADAY_CLOCK_H
#define __ARCH_ARM_FARADAY_CLOCK_H

struct clk {
	struct clk	*parent;
	int		users;
	unsigned long	rate;
	void __iomem	*base;	/* base address of controller if any */
	unsigned long	(*get_rate)(struct clk *clk);
	int		(*set_rate)(struct clk *clk, unsigned long rate);
	int		(*mode)(struct clk *, int on);
	void		*params;
};

/*
 * These functions assume that lock is already held.
 */
void __clk_enable(struct clk *clk);
void __clk_disable(struct clk *clk);
unsigned long __clk_get_rate(struct clk *clk);

#endif
