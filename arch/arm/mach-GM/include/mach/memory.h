/*
 *  arch/arm/mach-GM/include/mach/memory.h
 *  
 *  Faraday Platform Independent Memory Configuration
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
 *  Harry Hsu 04/06/2010 add "isolate high memory" function
 */

#ifndef __GM_PLATFORM_INDEPENDENT_MEMORY_HEADER__
#define __GM_PLATFORM_INDEPENDENT_MEMORY_HEADER__

#include <asm/sizes.h>
#include <mach/platform/memory.h>
#include <mach/platform/platform_io.h>

#define MEM_SIZE        SZ_256M
#define END_MEM		    (CPU_MEM_PA_LIMIT + 1)

#define CONSISTENT_DMA_SIZE  (12 << 20)	/* 12M NISH_20121015 */

#ifndef __ASSEMBLY__
extern unsigned long fmem_virt_to_phys(unsigned int vaddr);
extern unsigned int fmem_phys_to_virt(unsigned long phys);

#define __virt_to_phys(x)	fmem_virt_to_phys((unsigned int)(x))  //((x) - PAGE_OFFSET + PHYS_OFFSET)
#define __phys_to_virt(x)	fmem_phys_to_virt((unsigned long)(x))  //((x) - PHYS_OFFSET + PAGE_OFFSET)
#endif /* __ASSEMBLY__ */

#endif /* __GM_PLATFORM_INDEPENDENT_MEMORY_HEADER__ */
