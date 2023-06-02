/*
 * FH timer subsystem
 *
 * Copyright (C) 2014 Fullhan Microelectronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef __ASM_ARCH_TIMEX_H
#define __ASM_ARCH_TIMEX_H
#include "mach/chip.h"

#define PAE_PTS_CLK			(1000000)

#define CLOCK_TICK_RATE			TIMER_CLK

extern struct sys_timer fh_timer;

#endif /* __ASM_ARCH_TIMEX_H__ */
