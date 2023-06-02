/*
 * Novatek tmr010 driver.
 *
 * Copyright (C) 2019 Novatek MicroElectronics Corp.
 *
 * Updated by JJ Chen Jan 2020
 *
 * ----------------------------------------------------------------------------
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
 * ----------------------------------------------------------------------------
 *
 */

/*
 * Timer
 */
#ifndef __NVTTMR010_H
#define __NVTTMR010_H

#define NVTTMR010_OFFSET_COUNTER	0x00
#define NVTTMR010_OFFSET_LOAD		0x04
#define NVTTMR010_OFFSET_MATCH1		0x08
#define NVTTMR010_OFFSET_MATCH2		0x0c
#define NVTTMR010_OFFSET_TIMER(x)	((x) * 0x10)
#define NVTTMR010_OFFSET_CR			0x30
#define NVTTMR010_OFFSET_INTR_STATE	0x34
#define NVTTMR010_OFFSET_INTR_MASK	0x38

/*
 * Timer Control Register
 */
#define NVTTMR010_TM3_UPDOWN	(1 << 11)
#define NVTTMR010_TM2_UPDOWN	(1 << 10)
#define NVTTMR010_TM1_UPDOWN	(1 << 9)
#define NVTTMR010_TM3_OFENABLE	(1 << 8)
#define NVTTMR010_TM3_CLOCK		(1 << 7)
#define NVTTMR010_TM3_ENABLE	(1 << 6)
#define NVTTMR010_TM2_OFENABLE	(1 << 5)
#define NVTTMR010_TM2_CLOCK		(1 << 4)
#define NVTTMR010_TM2_ENABLE	(1 << 3)
#define NVTTMR010_TM1_OFENABLE	(1 << 2)
#define NVTTMR010_TM1_CLOCK		(1 << 1)
#define NVTTMR010_TM1_ENABLE	(1 << 0)

/*
 * Timer Interrupt State & Mask Registers
 */
#define NVTTMR010_TM3_OVERFLOW	(1 << 8)
#define NVTTMR010_TM3_MATCH2	(1 << 7)
#define NVTTMR010_TM3_MATCH1	(1 << 6)
#define NVTTMR010_TM2_OVERFLOW	(1 << 5)
#define NVTTMR010_TM2_MATCH2	(1 << 4)
#define NVTTMR010_TM2_MATCH1	(1 << 3)
#define NVTTMR010_TM1_OVERFLOW	(1 << 2)
#define NVTTMR010_TM1_MATCH2	(1 << 1)
#define NVTTMR010_TM1_MATCH1	(1 << 0)

#include <linux/interrupt.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>

struct nvttmr010_clockevent {
	struct clock_event_device	clockevent;
	struct irqaction		irqaction;
	void __iomem			*base;
	unsigned int			id;	/* one of 3 counters */
	unsigned int			reload;
	unsigned int			freq;
};

typedef struct nvttmr010_clocksource{
	struct clocksource	clocksource;
	void __iomem		*base;
	unsigned int		id;	/* one of 3 counters */
	unsigned int		freq;
}*nvttmr010_clocksource;

nvttmr010_clocksource nvttmr010_get_clocksource(void);
void __init nvttmr010_init(void __iomem *base, int irq, unsigned long clk_freq);
//void __init nvttmr010_clockevent_init(struct nvttmr010_clockevent *nvttmr010);
//void __init nvttmr010_clocksource_init(struct nvttmr010_clocksource *nvttmr010);

#endif	/* __NVTTMR010_H */
