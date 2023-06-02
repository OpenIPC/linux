/*------------------------------------------------------------------------------
    Function Code
-------------------------------------------------------------------------------*/

    .align 5
.globl infinity_restart

infinity_restart:
    mov   r3, #0x00FF
    ldr   r1, =0xFD001CB8
    str   r3, [r1]
    mov   r3, #0x0079
    str   r3, [r1]
    nop
    nop
    nop
    nop

_loop_forever:
    b    _loop_forever
