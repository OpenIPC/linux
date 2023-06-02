		.macro	addruart, rp, rv
		mov	\rp,	#0x00030000
		ldr	\rv,	=0xFE030000	@ virtual base
		orr	\rp, \rp, #0x10000000	@ physical base
		.endm

#include <asm/hardware/debug-pl01x.S>
