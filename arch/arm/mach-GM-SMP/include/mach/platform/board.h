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

#define BOARD_NAME              "Grain-Media GM8220 series"
#define BOOT_PARAMETER_PA_OFFSET  0x100

//starts from 0xFE000000 if VMALLOC_END is 0xFF000000
#define IPTABLE_BASE            (VMALLOC_END - 0x1000000)

/*
 * list the virtual address of IPs for the iotable.
 */
#ifdef CONFIG_FPGA /*----------------------------- FPGA --------------------------------------*/
/* PMU */
#define PMU_FTPMU010_PA_BASE        0xa0200000//use DMA base to do
#define PMU_FTPMU010_VA_BASE        IPTABLE_BASE
#define PMU_FTPMU010_VA_SIZE        SZ_4K

/* GIC_DIST */
#define PLATFORM_GIC_DIST_PA_BASE   (PLATFORM_GIC_PA_BASE + 0x1000)//use DMA base to do
#define PLATFORM_GIC_DIST_VA_BASE   (PMU_FTPMU010_VA_BASE + PMU_FTPMU010_VA_SIZE)
#define PLATFORM_GIC_DIST_VA_SIZE   SZ_4K

/* GIC_CPU */
#define PLATFORM_GIC_CPU_PA_BASE    (PLATFORM_GIC_PA_BASE + 0x2000)//use DMA base to do
#define PLATFORM_GIC_CPU_VA_BASE    (PLATFORM_GIC_DIST_VA_BASE + PLATFORM_GIC_DIST_VA_SIZE)
#define PLATFORM_GIC_CPU_VA_SIZE    SZ_4K

/* UART */
#define UART_FTUART010_0_PA_BASE	0xa0800000
#define UART_FTUART010_0_VA_BASE    (PLATFORM_GIC_CPU_VA_BASE + PLATFORM_GIC_CPU_VA_SIZE)//0xfa800000//(PMU_FTPMU010_VA_BASE + PMU_FTPMU010_VA_SIZE)
#define UART_FTUART010_0_VA_SIZE    SZ_4K
#define UART_FTUART010_0_IRQ        (IRQ_GIC_START + 4)

#define UART_FTUART010_1_PA_BASE    0xa0900000
#define UART_FTUART010_1_VA_BASE    (UART_FTUART010_0_VA_BASE + UART_FTUART010_0_VA_SIZE)//0xfa810000//(UART_FTUART010_0_VA_BASE + UART_FTUART010_0_VA_SIZE)
#define UART_FTUART010_1_VA_SIZE    SZ_4K
#define UART_FTUART010_1_IRQ        (IRQ_GIC_START + 6)

#define UART_FTUART010_2_PA_BASE    0x90900000
#define UART_FTUART010_2_VA_BASE    (UART_FTUART010_1_VA_BASE + UART_FTUART010_1_VA_SIZE)
#define UART_FTUART010_2_VA_SIZE    SZ_4K
#define UART_FTUART010_2_IRQ        (IRQ_GIC_START + 7)

/* TIMER */
#ifdef CONFIG_FTTMR010
#define TIMER_FTTMR010_PA_BASE	    0xa0a00000
#define TIMER_FTTMR010_VA_BASE	    0xfa820000
#define TIMER_FTTMR010_VA_SIZE	    SZ_4K
#define TIMER_FTTMR010_IRQ_COUNT    3
#define TIMER_FTTMR010_IRQ0         (IRQ_GIC_START + 1)
#define TIMER_FTTMR010_IRQ1         (IRQ_GIC_START + 2)
#define TIMER_FTTMR010_IRQ2         (IRQ_GIC_START + 3)
#endif

#else /*-------------------------------- GM8220 -------------------------------------*/

/* PMU */
#define PMU_FTPMU010_PA_BASE        0xfe000000
#define PMU_FTPMU010_VA_BASE        IPTABLE_BASE
#define PMU_FTPMU010_VA_SIZE        SZ_4K

/* UART */
#define UART_FTUART010_0_PA_BASE	  0xfe200000
#define UART_FTUART010_0_VA_BASE    (PMU_FTPMU010_VA_BASE + PMU_FTPMU010_VA_SIZE)
#define UART_FTUART010_0_VA_SIZE    SZ_4K
#define UART_FTUART010_0_IRQ        (IRQ_GIC_START + 38)

#define UART_FTUART010_1_PA_BASE    0xfe220000
#define UART_FTUART010_1_VA_BASE    (UART_FTUART010_0_VA_BASE + UART_FTUART010_0_VA_SIZE)
#define UART_FTUART010_1_VA_SIZE    SZ_4K
#define UART_FTUART010_1_IRQ        (IRQ_GIC_START + 39)

#define UART_FTUART010_2_PA_BASE    0xfe240000
#define UART_FTUART010_2_VA_BASE    (UART_FTUART010_1_VA_BASE + UART_FTUART010_1_VA_SIZE)
#define UART_FTUART010_2_VA_SIZE    SZ_4K
#define UART_FTUART010_2_IRQ        (IRQ_GIC_START + 43)

#define UART_FTUART010_3_PA_BASE	  0xfe260000
#define UART_FTUART010_3_VA_BASE    (UART_FTUART010_2_VA_BASE + UART_FTUART010_2_VA_SIZE)
#define UART_FTUART010_3_VA_SIZE    SZ_4K
#define UART_FTUART010_3_IRQ        (IRQ_GIC_START + 44)

#define UART_FTUART010_4_PA_BASE    0xfe280000
#define UART_FTUART010_4_VA_BASE    (UART_FTUART010_3_VA_BASE + UART_FTUART010_3_VA_SIZE)
#define UART_FTUART010_4_VA_SIZE    SZ_4K
#define UART_FTUART010_4_IRQ        (IRQ_GIC_START + 45)

#define UART_FTUART010_5_PA_BASE    0xfe2a0000
#define UART_FTUART010_5_VA_BASE    (UART_FTUART010_4_VA_BASE + UART_FTUART010_4_VA_SIZE)
#define UART_FTUART010_5_VA_SIZE    SZ_4K
#define UART_FTUART010_5_IRQ        (IRQ_GIC_START + 46)

#define UART_FTUART010_6_PA_BASE    0xfe2c0000
#define UART_FTUART010_6_VA_BASE    (UART_FTUART010_5_VA_BASE + UART_FTUART010_5_VA_SIZE)
#define UART_FTUART010_6_VA_SIZE    SZ_4K
#define UART_FTUART010_6_IRQ        (IRQ_GIC_START + 47)

#define UART_FTUART010_7_PA_BASE    0xfe2e0000
#define UART_FTUART010_7_VA_BASE    (UART_FTUART010_6_VA_BASE + UART_FTUART010_6_VA_SIZE)
#define UART_FTUART010_7_VA_SIZE    SZ_4K
#define UART_FTUART010_7_IRQ        (IRQ_GIC_START + 48)

/* TIMER */
#ifdef CONFIG_FTTMR010
#define TIMER_FTTMR010_PA_BASE	    0xfe8c0000
#define TIMER_FTTMR010_VA_BASE	    (UART_FTUART010_7_VA_BASE + UART_FTUART010_7_VA_SIZE)
#define TIMER_FTTMR010_VA_SIZE	    SZ_4K
#define TIMER_FTTMR010_IRQ_COUNT    3
#define TIMER_FTTMR010_IRQ0         (IRQ_GIC_START + 26)
#define TIMER_FTTMR010_IRQ1         (IRQ_GIC_START + 26)
#define TIMER_FTTMR010_IRQ2         (IRQ_GIC_START + 26)

#endif

/* TIMER */
#if 0//def CONFIG_ARMGENERICTIMER
#define TIMER_ARMGTIMER_PA_BASE	    0xA0547000//???
#define TIMER_ARMGTIMER_VA_BASE	    (UART_FTUART010_7_VA_BASE + UART_FTUART010_7_VA_SIZE)
#define TIMER_ARMGTIMER_VA_SIZE	    SZ_4K
#define TIMER_ARMGTIMER_IRQ         27
#endif

#endif
	
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
