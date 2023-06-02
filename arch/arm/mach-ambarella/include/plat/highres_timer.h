/*
 * arch/arm/plat-ambarella/include/plat/highres_timer.h
 *
 * Author: Louis Sun <lysun@ambarella.com>
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
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

#ifndef __PLAT_AMBARELLA_HIGHRES_TIMER_H
#define __PLAT_AMBARELLA_HIGHRES_TIMER_H

extern void highres_timer_init(struct hrtimer *timer, clockid_t clock_id, enum hrtimer_mode mode);
extern int highres_timer_start(struct hrtimer *timer, ktime_t tim, const enum hrtimer_mode mode);
extern int highres_timer_cancel(struct hrtimer *timer);

#endif

