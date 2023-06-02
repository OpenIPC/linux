#include <mach/platform.h>

	.macro  addruart, rp, rv, tmp
	ldr	\rp, =UART0_BASE @physical base
	ldr	\rv, =(UART0_BASE + IO_IOCH1_OFFSET)
	.endm

#include <asm/hardware/debug-pl01x.S>
