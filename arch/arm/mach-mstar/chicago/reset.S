/*------------------------------------------------------------------------------
    Function Code
-------------------------------------------------------------------------------*/

	.align	5
.globl chicago_reset_cpu
chicago_reset_cpu:

    mov   r3, #0x00FF
    ldr   r1, =0xFD003C18
    str   r3, [r1]
    mov   r3, #0x0079
    ldr   r1, =0xFD003C18
    str   r3, [r1]
	nop
	nop
	nop
	nop

_loop_forever:
	b	_loop_forever
