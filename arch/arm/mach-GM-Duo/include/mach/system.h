/*
 *  arch/arm/mach-GM-Duo/include/mach/system.h
 *  
 *  Faraday Platform Independent System Definitions
 *  
 *  Copyright (C) 2005 Faraday Corp. (http://www.faraday-tech.com)
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
 *  
 * ChangeLog
 *
 *  Luke Lee  09/16/2005  Created.
 */

#ifndef __GM_PLATFORM_INDEPENDENT_SYSTEM_HEADER__
#define __GM_PLATFORM_INDEPENDENT_SYSTEM_HEADER__

/* Include platform *dependent* system definitions */
#include <mach/platform/system.h>

#include <asm/proc-fns.h>
#ifndef arch_idle
static inline void arch_idle(void)
{
	cpu_do_idle();
}
#endif

#ifndef arch_reset
extern inline void arch_reset(char mode, const char *cmd)
{
	/* NOP */
}
#endif

#endif /* __GM_PLATFORM_INDEPENDENT_SYSTEM_HEADER__ */

