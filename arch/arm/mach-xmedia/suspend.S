#include <linux/linkage.h>

.data
/*
 * this code will be copied from
 * ddr to sram for system resumeing.
 * so it is ".data section".
 */
	.align	4
ENTRY(xmedia_sys_suspend)
	nop
	nop
	nop
	dsb
	isb
	wfi
	mov pc, lr
ENDPROC(xmedia_sys_suspend)

ENTRY(xmedia_sys_suspend_sz)
	.word   . - xmedia_sys_suspend
