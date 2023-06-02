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

#define BOARD_NAME              "Grain-Media GM8136 series"
#define BOOT_PARAMETER_PA_OFFSET  0x100

//starts from 0xFE000000 if VMALLOC_END is 0xFF000000
#define IPTABLE_BASE            (VMALLOC_END - 0x1000000)

/*
 * list the virtual address of IPs for the iotable.
 */
/* PMU */
#define PMU_FTPMU010_PA_BASE        0x90C00000
#define PMU_FTPMU010_VA_BASE        IPTABLE_BASE
#define PMU_FTPMU010_VA_SIZE        SZ_4K

/* UART */
#define UART_FTUART010_0_PA_BASE	0x90700000
#define UART_FTUART010_0_VA_BASE    (PMU_FTPMU010_VA_BASE + PMU_FTPMU010_VA_SIZE)
#define UART_FTUART010_0_VA_SIZE    SZ_4K
#define UART_FTUART010_0_IRQ        21

#define UART_FTUART010_1_PA_BASE    0x90800000
#define UART_FTUART010_1_VA_BASE    (UART_FTUART010_0_VA_BASE + UART_FTUART010_0_VA_SIZE)
#define UART_FTUART010_1_VA_SIZE    SZ_4K
#define UART_FTUART010_1_IRQ        22

#define UART_FTUART010_2_PA_BASE    0x90900000
#define UART_FTUART010_2_VA_BASE    (UART_FTUART010_1_VA_BASE + UART_FTUART010_1_VA_SIZE)
#define UART_FTUART010_2_VA_SIZE    SZ_4K
#define UART_FTUART010_2_IRQ        25

/* TIMER */
#define TIMER_FTTMR010_PA_BASE	    0x90D00000
#define TIMER_FTTMR010_VA_BASE	    (UART_FTUART010_2_VA_BASE + UART_FTUART010_2_VA_SIZE)
#define TIMER_FTTMR010_VA_SIZE	    SZ_4K
#define TIMER_FTTMR010_IRQ_COUNT    3
#define TIMER_FTTMR010_IRQ0         14
#define TIMER_FTTMR010_IRQ1         14
#define TIMER_FTTMR010_IRQ2         14

/* INTC030 */
#define INTC_FTINTC030_PA_BASE      0x96000000
#define INTC_FTINTC030_VA_BASE      (TIMER_FTTMR010_VA_BASE + TIMER_FTTMR010_VA_SIZE)
#define INTC_FTINTC030_VA_SIZE	    SZ_4K

/* L2 Cache */
//#define L2CACHE_FTL2CC031_PA_BASE   0xE4B00000
//#define L2CACHE_FTL2CC031_VA_BASE   (INTC_FTINTC030_VA_BASE + INTC_FTINTC030_VA_SIZE)
//#define L2CACHE_FTL2CC031_VA_SIZE   SZ_4K
//#define L2CACHE_FTL2CC031_IRQ	    2
	
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
