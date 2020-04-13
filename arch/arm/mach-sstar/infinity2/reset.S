/*------------------------------------------------------------------------------
    Function Code
-------------------------------------------------------------------------------*/

	.align	5
.globl cedric_reset_cpu
cedric_reset_cpu:

    mov   r3, #0x0000
    ldr   r1, =0xFD005C80
    str   r3, [r1]
    mov   r3, #0x0079
	ldr	  r1, =0xFD005CB8
	str   r3, [r1]
	nop
	nop
	nop
	nop

_loop_forever:
	b	_loop_forever
