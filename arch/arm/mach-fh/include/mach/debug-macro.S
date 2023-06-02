/* linux/arch/arm/mach-fh/include/mach/debug-macro.S
 *
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

/* pull in the relevant register and map files. */

	/* note, for the boot process to work we have to keep the UART
	 * virtual address aligned to an 1MiB boundary for the L1
	 * mapping the head code makes. We keep the UART virtual address
	 * aligned and add in the offset when we load the value here.
	 */


#include <linux/serial_reg.h>
#include <mach/hardware.h>
#include <asm/memory.h>
#include <mach/chip.h>

#include <mach/io.h>

	.macro addruart, rp, rv
		ldr	\rp, =CONSOLE_REG_BASE
		ldr	\rv, =VA_CONSOLE_REG_BASE
	.endm

 	.macro senduart,data,addr
        	strb      \data, [\addr, #(0x00)]           @ Write to Transmitter Holding Register
 	.endm

  	.macro waituart,data,addr
1001:   	ldr \data, [\addr, #(0x14)]                 @ Read Status Register
        	tst \data, #(0x40)                               @when TX FIFO Full, then wait
		beq 1001b
  	.endm

 	.macro busyuart,data,addr
@		stmfd r13!, {r4}
1002:
		ldr	\data, [\addr, #(0x14)]
		tst	\data, #(0x40)
		beq	1002b
	.endm



