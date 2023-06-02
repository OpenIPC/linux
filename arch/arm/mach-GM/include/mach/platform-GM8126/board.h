/*
 *  arch/arm/mach-GM/include/mach/platform/board.h
 *  
 *  GM board Independent Specification
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
 *   Harry  2012/3/6 03:15 created. 
 */
#ifndef __BOARD_H__
#define __BOARD_H__

#include <mach/platform/platform_io.h>
#include <asm/pgtable.h>
//#include <mach/platform/vmalloc.h>

#define BOARD_NAME              "Grain-Media GM8126"
#define BOOT_PARAMETER_PA_BASE  (CPU_MEM_PA_BASE + 0x100)

//starts from 0xFE000000 if VMALLOC_END is 0xFF000000
#define IPTABLE_BASE            (VMALLOC_END - 0x1000000)   

/*
 * list the virtual address of IPs for the iotable.
 */
/* PMU */
#define PMU_FTPMU010_PA_BASE        0x99000000
#define PMU_FTPMU010_VA_BASE        IPTABLE_BASE
#define PMU_FTPMU010_VA_SIZE        SZ_4K

/* UART */
#define UART_FTUART010_0_PA_BASE	0x98300000
#define UART_FTUART010_0_VA_BASE    (PMU_FTPMU010_VA_BASE + PMU_FTPMU010_VA_SIZE)
#define UART_FTUART010_0_VA_SIZE    SZ_4K
#define UART_FTUART010_0_IRQ        9

/* TIMER */
#define TIMER_FTTMR010_PA_BASE	    0x99100000
#define TIMER_FTTMR010_VA_BASE	    (UART_FTUART010_0_VA_BASE + UART_FTUART010_0_VA_SIZE)
#define TIMER_FTTMR010_VA_SIZE	    SZ_4K
#define TIMER_FTTMR010_IRQ_COUNT    3
#define TIMER_FTTMR010_IRQ0         14
#define TIMER_FTTMR010_IRQ1         14
#define TIMER_FTTMR010_IRQ2         14

/* INTC */
#define INTC_FTINTC010_PA_BASE      0x99500000
#define INTC_FTINTC010_VA_BASE      (TIMER_FTTMR010_VA_BASE + TIMER_FTTMR010_VA_SIZE)
#define INTC_FTINTC010_VA_SIZE	    SZ_4K

/* define the address used in machine-start */
#define PLATFORM_BOOTTIME_IO_PA_BASE    UART_FTUART010_0_PA_BASE
#define PLATFORM_BOOTTIME_IO_VA_BASE    UART_FTUART010_0_VA_BASE
/* for debug_macro.S */
#define DEBUG_LL_FTUART010_PA_BASE      UART_FTUART010_0_PA_BASE    
#define DEBUG_LL_FTUART010_VA_BASE      UART_FTUART010_0_VA_BASE


#ifndef __ASSEMBLY__ 
 #include <asm/setup.h>

/* declare the function prototype
 */
void board_get_memory(struct meminfo **p_memory);
void board_get_gmmemory(struct meminfo **p_memory);
#endif /* __ASSEMBLY__  */

#endif /* __BOARD_H__ */
