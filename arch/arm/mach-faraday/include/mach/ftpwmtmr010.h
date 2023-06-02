/*
 *  arch/arm/mach-faraday/include/mach/ftpwmtmr010.h
 *
 *  Faraday FTPWMTMR010 Timer
 *
 *  Copyright (C) 2009 Faraday Technology
 *  Copyright (C) 2009 Po-Yu Chuang <ratbert@faraday-tech.com>
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

#ifndef __FTPWMTMR010_H
#define __FTPWMTMR010_H

#define FTPWMTMR010_OFFSET_INTSTAT	0x00
#define FTPWMTMR010_OFFSET_TIMER(x)	((x) * 0x10)
#define FTPWMTMR010_OFFSET_CTRL		0x00
#define FTPWMTMR010_OFFSET_LOAD		0x04
#define FTPWMTMR010_OFFSET_CMP		0x08
#define FTPWMTMR010_OFFSET_CNT		0x0c

/*
 * Timer Control Register
 */
#define FTPWMTMR010_CTRL_SRC		(1 << 0)
#define FTPWMTMR010_CTRL_START		(1 << 1)
#define FTPWMTMR010_CTRL_UPDATE		(1 << 2)
#define FTPWMTMR010_CTRL_OUT_INV	(1 << 3)
#define FTPWMTMR010_CTRL_AUTO		(1 << 4)
#define FTPWMTMR010_CTRL_INT_EN		(1 << 5)
#define FTPWMTMR010_CTRL_INT_MODE	(1 << 6)
#define FTPWMTMR010_CTRL_DMA_EN		(1 << 7)
#define FTPWMTMR010_CTRL_PWM_EN		(1 << 8)
#define FTPWMTMR010_CTRL_DZ(x)		(((x) & 0xff) << 24)

#include <linux/interrupt.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>

struct ftpwmtmr010_clockevent {
	struct clock_event_device	clockevent;
	struct irqaction		irqaction;
	void __iomem			*base;
	unsigned int			id;	/* one of 8 counters */
	unsigned int			reload;
	unsigned int			freq;
};

struct ftpwmtmr010_clocksource {
	struct clocksource	clocksource;
	void __iomem		*base;
	unsigned int		id;	/* one of 8 counters */
	unsigned int		freq;
};

void __init ftpwmtmr010_clockevent_init(struct ftpwmtmr010_clockevent *ftpwmtmr010);
void __init ftpwmtmr010_clocksource_init(struct ftpwmtmr010_clocksource *ftpwmtmr010);

#endif	/* __FTPWMTMR010_H */
