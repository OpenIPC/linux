#ifndef __ASM_ARCH_IO_H
#define __ASM_ARCH_IO_H

/*
 * We don't actually have real ISA nor PCI buses, but there is so many
 * drivers out there that might just work if we fake them...
 */
#define __mem_pci(a)		(a)
#define __mem_isa(a)		(a)

#define FH_VIRT		0xFE000000

#define		VA_INTC_REG_BASE	(FH_VIRT + 0x00000)
#define		VA_TIMER_REG_BASE	(FH_VIRT + 0x10000)
#define		VA_UART0_REG_BASE	(FH_VIRT + 0x20000)
#define		VA_PMU_REG_BASE		(FH_VIRT + 0x90000)
#define		VA_UART1_REG_BASE	(FH_VIRT + 0xa0000)
#define		VA_UART2_REG_BASE	(FH_VIRT + 0xb0000)
#define		VA_UART3_REG_BASE	(FH_VIRT + 0xc0000)

#define		VA_RAM_REG_BASE		(FH_VIRT + 0xd0000)
#define		VA_DDRC_REG_BASE	(FH_VIRT + 0xe0000)

#define VA_CONSOLE_REG_BASE		VA_UART0_REG_BASE

#endif /* __ASM_ARCH_IO_H */
