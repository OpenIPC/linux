/*
 *  arch/arm/mach-GM-Duo/include/mach/platform/memory.h
 *  
 *  GM Platform Dependent Memory Configuration
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
 */
#ifndef __MEMORY_H__
#define __MEMORY_H__

#ifndef __ASSEMBLY__

#define PHYS_OFFSET     CPU_MEM_PA_BASE//0x0

#endif /* __ASSEMBLY__ */
/* The memory size DDR0_FMEM_SIZE / DDR1_FMEM_SIZE is allocated for Framamp management. 
 *
 * Users can adjust the memory size based on the real memory size requirement.  
 * This definition is applied to DDR0 only.
 */
#define DDR0_FMEM_SIZE     0x1000000

/* The memory size for Framamp management. Users can adjust the memory size based on the real
 * memory size requirement. 
 *
 * This definition is applied to DDR1. -1 means to allocate almost whole memory.
 */
#define DDR1_FMEM_SIZE     -1


/* HARRY: DO NOT CHANGE THIS VALUE. 
 * The memory boundary is 16M alginment 
 *
 *
 * Two definitions are required for sparsemem:
 *
 * MAX_PHYSMEM_BITS: The number of physical address bits required
 *   to address the last byte of memory.
 *
 * SECTION_SIZE_BITS: The number of physical address bits to cover
 *   the maximum amount of memory in a section.
 *
 * Eg, if you have 2 banks of up to 64MB at 0x80000000, 0x84000000,
 * then MAX_PHYSMEM_BITS is 32, SECTION_SIZE_BITS is 26.
 */
#define SECTION_SIZE_BITS   24  /* 16M */
#define MAX_PHYSMEM_BITS    27  /* 128M */


#endif /* __MEMORY_H__ */

