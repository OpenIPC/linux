#include <linux/linkage.h>
#include <asm/assembler.h>

ENTRY(float_test)
    scvtf   d1, x0
    fmov    d0, #1.000000000000000000e+00
    fmov    d6, #2.000000000000000000e+00
    fmov    d2, #5.000000000000000000e-01
    fmadd   d0, d1, d7, d0
    fsub    d0, d0, d6
    fmul    d0, d0, d5
    fdiv    d0, d0, d4
    fmadd   d0, d0, d3, d2
    fcvtzs  x0, d0
    str x0, [x1]
	ret
ENDPROC(float_test)
