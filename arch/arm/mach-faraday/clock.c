/*
 *  linux/arch/arm/mach-faraday/clock.c
 *
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

#include <linux/clk.h>
#include <linux/module.h>
#include <linux/spinlock.h>

#include <linux/clkdev.h>

#include "clock.h"

static DEFINE_SPINLOCK(clk_lock);

void __clk_enable(struct clk *clk)
{
	if (clk->parent)
		__clk_enable(clk->parent);
	if (clk->users++ == 0 && clk->mode)
		clk->mode(clk, 1);
}

int clk_enable(struct clk *clk)
{
	unsigned long	flags;

	spin_lock_irqsave(&clk_lock, flags);
	__clk_enable(clk);
	spin_unlock_irqrestore(&clk_lock, flags);
	return 0;
}
EXPORT_SYMBOL(clk_enable);

void __clk_disable(struct clk *clk)
{
	BUG_ON(clk->users == 0);
	if (--clk->users == 0 && clk->mode)
		clk->mode(clk, 0);
	if (clk->parent)
		__clk_disable(clk->parent);
}

void clk_disable(struct clk *clk)
{
	unsigned long	flags;

	spin_lock_irqsave(&clk_lock, flags);
	__clk_disable(clk);
	spin_unlock_irqrestore(&clk_lock, flags);
}
EXPORT_SYMBOL(clk_disable);

unsigned long __clk_get_rate(struct clk *clk)
{
	unsigned long rate;

	rate = clk->rate;
	if (rate == 0) {
		if (clk->get_rate)
			rate = clk->get_rate(clk);
		else if (clk->parent)
			rate = __clk_get_rate(clk->parent);
	}

	return rate;
}

unsigned long clk_get_rate(struct clk *clk)
{
	unsigned long flags;
	unsigned long rate;

	spin_lock_irqsave(&clk_lock, flags);
	rate = __clk_get_rate(clk);
	spin_unlock_irqrestore(&clk_lock, flags);
	return rate;
}
EXPORT_SYMBOL(clk_get_rate);

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	unsigned long flags;
	int ret = -EINVAL;

	if (clk == NULL || clk->set_rate == NULL || rate == 0)
		return ret;

	spin_lock_irqsave(&clk_lock, flags);
	ret = clk->set_rate(clk, rate);
	spin_unlock_irqrestore(&clk_lock, flags);

	return ret;
}
EXPORT_SYMBOL(clk_set_rate);
