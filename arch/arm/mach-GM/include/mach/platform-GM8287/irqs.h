/*
 *  arch/arm/mach-GM/include/mach/platform-GM8287/irqs.h
 *  
 *  Faraday Platform Dependent IRQ Definitions
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
 *  ChangeLog
 *  
 *   Luke Lee 10/06/2005 Created.
 */

#ifndef __GM_PLATFORM_IRQS_HEADER__
#define __GM_PLATFORM_IRQS_HEADER__

#include <mach/platform/platform_io.h>

#define FIQ_START		    PLATFORM_FIQ_BASE
#define MAX_FTINTC010_NR	2
#define NR_IRQS             PLATFORM_INTERRUPTS

#endif /* __GM_PLATFORM_IRQS_HEADER__ */
