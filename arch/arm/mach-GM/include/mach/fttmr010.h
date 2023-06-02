/*
 * Copyright (C) 2009 Po-Yu Chuang
 * Copyright (C) 2009 Faraday Technology
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Timer
 */
#ifndef __FTTMR010_H
#define __FTTMR010_H

#define FTTMR010_OFFSET_COUNTER		0x00
#define FTTMR010_OFFSET_LOAD		0x04
#define FTTMR010_OFFSET_MATCH1		0x08
#define FTTMR010_OFFSET_MATCH2		0x0c
#define FTTMR010_OFFSET_TIMER(x)	((x) * 0x10)
#define FTTMR010_OFFSET_CR		0x30
#define FTTMR010_OFFSET_INTR_STATE	0x34
#define FTTMR010_OFFSET_INTR_MASK	0x38

/*
 * Timer Control Register
 */
#define FTTMR010_TM3_UPDOWN	(1 << 11)
#define FTTMR010_TM2_UPDOWN	(1 << 10)
#define FTTMR010_TM1_UPDOWN	(1 << 9)
#define FTTMR010_TM3_OFENABLE	(1 << 8)
#define FTTMR010_TM3_CLOCK	(1 << 7)
#define FTTMR010_TM3_ENABLE	(1 << 6)
#define FTTMR010_TM2_OFENABLE	(1 << 5)
#define FTTMR010_TM2_CLOCK	(1 << 4)
#define FTTMR010_TM2_ENABLE	(1 << 3)
#define FTTMR010_TM1_OFENABLE	(1 << 2)
#define FTTMR010_TM1_CLOCK	(1 << 1)
#define FTTMR010_TM1_ENABLE	(1 << 0)

/*
 * Timer Interrupt State & Mask Registers
 */
#define FTTMR010_TM3_OVERFLOW	(1 << 8)
#define FTTMR010_TM3_MATCH2	(1 << 7)
#define FTTMR010_TM3_MATCH1	(1 << 6)
#define FTTMR010_TM2_OVERFLOW	(1 << 5)
#define FTTMR010_TM2_MATCH2	(1 << 4)
#define FTTMR010_TM2_MATCH1	(1 << 3)
#define FTTMR010_TM1_OVERFLOW	(1 << 2)
#define FTTMR010_TM1_MATCH2	(1 << 1)
#define FTTMR010_TM1_MATCH1	(1 << 0)

#include <linux/interrupt.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>

struct fttmr010_clockevent {
	struct clock_event_device	clockevent;
	struct irqaction		irqaction;
	void __iomem			*base;
	unsigned int			id;	/* one of 3 counters */
	unsigned int			reload;
	unsigned int			freq;
};

struct fttmr010_clocksource {
	struct clocksource	clocksource;
	void __iomem		*base;
	unsigned int		id;	/* one of 3 counters */
	unsigned int		freq;
};

void __init fttmr010_clockevent_init(struct fttmr010_clockevent *fttmr010);
void __init fttmr010_clocksource_init(struct fttmr010_clocksource *fttmr010);

#endif	/* __FTTMR010_H */
