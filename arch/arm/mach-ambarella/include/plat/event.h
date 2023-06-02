/*
 * arch/arm/plat-ambarella/include/plat/event.h
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

#ifndef __PLAT_AMBARELLA_EVENT_H
#define __PLAT_AMBARELLA_EVENT_H

/* ==========================================================================*/
#define AMBA_EVENT_PRE			(0x80000000)
#define AMBA_EVENT_POST			(0x40000000)
#define AMBA_EVENT_CHECK		(0x20000000)

#define AMBA_EVENT_ID_CPUFREQ		(1)
#define AMBA_EVENT_ID_PM		(2)
#define AMBA_EVENT_ID_TOSS		(3)
#define AMBA_EVENT_ID_GIVEUP_DSP	(4)
#define AMBA_EVENT_ID_TAKEOVER_DSP	(5)
#define AMBA_EVENT_ID_VIN_LOSS		(6)

#define AMBA_EVENT_PRE_CPUFREQ		(AMBA_EVENT_ID_CPUFREQ | AMBA_EVENT_PRE)
#define AMBA_EVENT_POST_CPUFREQ		(AMBA_EVENT_ID_CPUFREQ | AMBA_EVENT_POST)
#define AMBA_EVENT_CHECK_CPUFREQ	(AMBA_EVENT_ID_CPUFREQ | AMBA_EVENT_CHECK)
#define AMBA_EVENT_PRE_PM		(AMBA_EVENT_ID_PM | AMBA_EVENT_PRE)
#define AMBA_EVENT_POST_PM		(AMBA_EVENT_ID_PM | AMBA_EVENT_POST)
#define AMBA_EVENT_CHECK_PM		(AMBA_EVENT_ID_PM | AMBA_EVENT_CHECK)
#define AMBA_EVENT_PRE_TOSS		(AMBA_EVENT_ID_TOSS | AMBA_EVENT_PRE)
#define AMBA_EVENT_POST_TOSS		(AMBA_EVENT_ID_TOSS | AMBA_EVENT_POST)
#define AMBA_EVENT_CHECK_TOSS		(AMBA_EVENT_ID_TOSS | AMBA_EVENT_CHECK)

#define AMBA_EVENT_PRE_GIVEUP_DSP	(AMBA_EVENT_ID_GIVEUP_DSP | AMBA_EVENT_PRE)
#define AMBA_EVENT_POST_GIVEUP_DSP	(AMBA_EVENT_ID_GIVEUP_DSP | AMBA_EVENT_POST)
#define AMBA_EVENT_GIVEUP_DSP		(AMBA_EVENT_ID_GIVEUP_DSP | AMBA_EVENT_CHECK)
#define AMBA_EVENT_PRE_TAKEOVER_DSP	(AMBA_EVENT_ID_TAKEOVER_DSP | AMBA_EVENT_PRE)
#define AMBA_EVENT_POST_TAKEOVER_DSP	(AMBA_EVENT_ID_TAKEOVER_DSP | AMBA_EVENT_POST)
#define AMBA_EVENT_TAKEOVER_DSP		(AMBA_EVENT_ID_TAKEOVER_DSP | AMBA_EVENT_CHECK)

#define AMBA_EVENT_POST_VIN_LOSS	(AMBA_EVENT_ID_VIN_LOSS | AMBA_EVENT_POST)

/* ==========================================================================*/
#ifndef __ASSEMBLER__

/* ==========================================================================*/

/* ==========================================================================*/
extern int ambarella_register_event_notifier(void *nb);
extern int ambarella_unregister_event_notifier(void *nb);
extern int ambarella_set_event(unsigned long val, void *v);
extern int ambarella_register_raw_event_notifier(void *nb);
extern int ambarella_unregister_raw_event_notifier(void *nb);
extern int ambarella_set_raw_event(unsigned long val, void *v);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

