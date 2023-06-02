#ifndef __XM_CHIP_REGS_H__
#define __XM_CHIP_REGS_H__

#include <mach/io.h>

/*#define DDR_BASE		0x80000000*/
/**//*#define DDRC_BASE		0x20110000*/
/**//*#define IOCONFIG_BASE		0x200F0000*/
#define UART0_BASE		0x10030000
#define UART1_BASE		0x10040000
#define UART2_BASE		0x10050000
#define SYS_CTRL_BASE	0x20000000
#define ARM_INTNL_BASE  0x20200000 
#define REG_BASE_L2CACHE 0x20210000

#define PLL_PLLA_CTRL                0x20000008     
#define PLL_CPUCLK_CTRL              0x20000028

/*#define WDG_BASE		0x20040000*/
/*#define CRG_REG_BASE		0x20030000*/

#define TIMER0_REG_BASE         0x100C0000
#define TIMER1_REG_BASE         0x100C0020 
#define TIMER2_REG_BASE         0x100D0000 
#define TIMER3_REG_BASE         0x100D0020 

#define GMAC_BASE       0x10010000
#define GPIO_BASE       0x10020000
#define INTC_BASE		0x20010000
#define DMAC_BASE		0x20020000

#define SDIO0_BASE      0x50000000
#define SDIO1_BASE      0x50100000


#define REG_A5_PERI_SCU     0x0000     
#define REG_A5_PERI_PRI_TIMER_WDT 0x0600
#define A5_GIC_OFFSET       0x100
#define A5_GIC_DIST     0x1000
#define CFG_GIC_CPU_BASE    (ARM_INTNL_BASE + A5_GIC_OFFSET)
#define CFG_GIC_DIST_BASE   (ARM_INTNL_BASE + A5_GIC_DIST)



#define REG_INTC_IRQSTATUS	0x000
#define REG_INTC_FIQSTATUS	0x004
#define REG_INTC_RAWSTATUS	0x008
#define REG_INTC_INTSELECT	0x00C
#define REG_INTC_INTENABLE	0x010
#define REG_INTC_INTENCLEAR	0x014
#define REG_INTC_SOFTINT	0x018
#define REG_INTC_SOFTINTCLEAR	0x01C

#define INTNR_IRQ_START		0
#define INTNR_IRQ_END		31

#define REG_TIMER_RELOAD	0x000
#define REG_TIMER_VALUE		0x004
#define REG_TIMER_CONTROL	0x008
#define REG_TIMER_INTCLR	0x00C
#define REG_TIMER_RIS		0x010
#define REG_TIMER_MIS		0x014
#define REG_TIMER_BGLOAD	0x018

#define REG_TIMER1_RELOAD	0x020
#define REG_TIMER1_VALUE	0x024
#define REG_TIMER1_CONTROL	0x028
#define REG_TIMER1_INTCLR	0x02C
#define REG_TIMER1_RIS		0x030
#define REG_TIMER1_MIS		0x034
#define REG_TIMER1_BGLOAD	0x038


#define REG_SC_REMAP        0x0204

#define REG_SYS_SOFT_RSTEN  0x80  
#define REG_SYS_SOFT_RST	0x84
#define REG_CPU1_SOFT_RST	0x8C

#define GPIO_MUX1_EN       	(1 << 1)
#define GPIO_MUX2_EN       	(1 << 2)
#define GPIO_MUX3_EN       	(1 << 3)
#define GPIO_OUT_EN        	(1 << 10)
#define GPIO_OUT_HIGH      	(1 << 11)
#define GPIO_OUT_LOW       	(0 << 11)
#define GPIO_IN_EN         	(1 << 12)
#define GPIO_DRIVE_2MA      (0 << 6)
#define GPIO_DRIVE_4MA      (1 << 6)
#define GPIO_DRIVE_8MA      (2 << 6)
#define GPIO_DRIVE_12MA     (3 << 6)

#define GPIO_FLASH_CLK		9
#define GPIO_FLASH_CSN		10
#define GPIO_FLASH_DI		11
#define GPIO_FLASH_DO		12
#define GPIO_FLASH_WPN		13
#define GPIO_FLASH_HOLD		14

#define SMP_COREX_START_ADDR_REG        0x20000F00/* see bootloader */

#endif
