/*
Offset 0x20 ==> command			0x33333333->0x06260626 => 626 has boot and run
Offset 0x24 ==> boot pc			0xFFFFFFFF
Offset 0x28 ==> response		0x0 -> 0x35737888
 */
 
reset:  b start_boot
undef:  b undef
swi:    b swi
pref:   b pref
dabt:   b dabt
resv:   b resv
irq:    b irq
fiq:    b fiq

_offset_command:            @ command at 0x20
	.word 0x33333333        @ default value

_offset_bootpc:             @ boot address at 0x24, FA726 will update it.
	.word 0xFFFFFFFF        @ jump to address XXX

_offset_response:           @ response at 0x28 to notice both ends
	.word 0x0

.globl _start,_end
.globl _boot_start
_boot_start:
	.word _start

.globl _boot_end
_boot_end:
	.word _end

start_boot:
    mov     r0, #0
    mov     r1, #0
    mov     r2, #0
    mov     r3, #0
    mov     r4, #0
    mov     r5, #0
    mov     r6, #0
    mov     r7, #0      @reserved for counter
    mov     r8, #0
    mov     r9, #0
    mov     r10, #0 
    mov     r11, #0

delay:    
    //mov	r10, lr
    
    //delay for a while
    ldr     r0, =0x10000
    ldr     r1, =0x0
myloop1:
    sub     r0, r0, #1
    cmp     r0, r1
    bne     myloop1
    //mov	    pc, r10
    
    //UART divisor to 38400
    //ldr     r1, =0x98400000
    //ldr     r0, =0x83
    //str     r0, [r1,#0xc]
    //ldr     r0, =0x14
    //str     r0, [r1]
    //ldr     r0, =0x3
    //str     r0, [r1,#0xc]
    //ldr     r0, =0x43
    //str     r0, [r1]    
    
    //update the command to inform fa726, I am ok.
    ldr     r2, =0x06260626
    adr     r3, _offset_command
    str     r2, [r3]
    
    ldr     r4, =0x35737888
wait_response:
    ldr     r5, _offset_response
    cmp     r4, r5
    beq     get_bootpc
    //bl      delay
    nop
    nop
    nop    
    b       wait_response
        
/* jump to specified address */
get_bootpc:
    ldr     r2, =0x11111111
    adr     r3, _offset_command
    str     r2, [r3]
    
    ldr     r2, _offset_bootpc     @boot address
    mov     pc, r2
    nop
    nop
    nop
    nop
    nop