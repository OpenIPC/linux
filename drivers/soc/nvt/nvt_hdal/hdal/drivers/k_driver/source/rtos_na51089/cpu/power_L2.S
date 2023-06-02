#ifndef ENTRY
#define ENTRY(name) \
        .globl name; \
name:
#endif

#ifndef WEAK
#define WEAK(name) \
        .weak name; \
name:
#endif

#ifndef END
#define END(name) \
        .size name, .-name
#endif

#define ENDPROC(name) \
        .type name, %function; \
        END(name)

.arch   armv7-a
.fpu    neon-fp-armv8
ENTRY(power_loop_l2)

    
    stmfd       sp!, {r0-r12, lr}
    mov     r10, r0 //buffer addr
    mov     r9, r1 // buffer offset
    mov     r5, r10
    add     r11, r5 , r9 
    ////
    /*
    mov     r1, r10 
    LDR     r0, =input_string
    BL printk
    mov     r1, r9 
    LDR     r0, =input_string
    BL printk
    LDR     r1, [r10] 
    LDR     r0, =input_string
    BL printk
    */
    mov     r7, r9
preload_loop:
    pld    [r10]
    pld    [r10, #0x40]
    pld    [r10, #0x80]
    pld    [r10, #0xc0]
    pld    [r10, #0x100]
    pld    [r10, #0x140]
    pld    [r10, #0x180]
    pld    [r10, #0x1c0]
    add     r10, r10, #0x200
    subs    r7, r7, #0x100   // for entire buffer, not buffer offset 
    bne     preload_loop
    mov     r0, #0
    mcr     p15, 2, r0, c0 ,c0 ,0
    isb
    mrc     p15, 1, r0 ,c0 ,c0 ,0
    ubfx    r0, r0, #13, # 15
    mov     r1, #0
inv_loop:
    orr     r2, r1, #0x40000000
    orr     r3, r1, #0x80000000
    orr     r4, r1, #0xC0000000
    mcr     p15, 0, r1, c7, c14, 2
    mcr     p15, 0, r2, c7, c14, 2
    mcr     p15, 0, r3, c7, c14, 2
    mcr     p15, 0, r4, c7, c14, 2
    add     r1, r1, #0x40
    subs    r0, r0, #1
    bcs     inv_loop
    mov     r10, r5  
mem_copy_loop:
    ldmia   r10, {r0, r1, r2, r3}
    stmia   r11, {r0, r1, r2, r3}
    
    LDR     r6  ,[r10]
    LDR     r7  ,[r11]
    /*
    mov     r1, r10
    LDR     r0, =parm_string
    BL printk
    mov     r1, r11
    LDR     r0, =parm_string
    BL printk
    mov     r1, r6
    LDR     r0, =parm_string
    BL printk
    mov     r1, r7
    LDR     r0, =parm_string
    BL printk
    */
    cmp     r6, r7
    bne     cmp_fail
    add     r10, r10, #4
    add     r11, r11, #4
    LDR     r6  ,[r10]
    LDR     r7  ,[r11]
    cmp     r6, r7
    bne     cmp_fail
    add     r10, r10, #4
    add     r11, r11, #4
    LDR     r6  ,[r10]
    LDR     r7  ,[r11]
    cmp     r6, r7
    bne     cmp_fail
    add     r10, r10, #4
    add     r11, r11, #4
    LDR     r6  ,[r10]
    LDR     r7  ,[r11]
    cmp     r6, r7
    bne     cmp_fail
    add     r10, r10, #4
    add     r11, r11, #4

    ldmia   r10, {r0, r1, r2, r3}
    stmia   r11, {r0, r1, r2, r3}
    LDR     r6  ,[r10]
    LDR     r7  ,[r11]
    cmp     r6, r7
    bne     cmp_fail
    add     r10, r10, #4
    add     r11, r11, #4
    LDR     r6  ,[r10]
    LDR     r7  ,[r11]
    cmp     r6, r7
    bne     cmp_fail
    add     r10, r10, #4
    add     r11, r11, #4
    LDR     r6  ,[r10]
    LDR     r7  ,[r11]
    cmp     r6, r7
    bne     cmp_fail
    add     r10, r10, #4
    add     r11, r11, #4
    LDR     r6  ,[r10]
    LDR     r7  ,[r11]
    cmp     r6, r7
    bne     cmp_fail
    add     r10, r10, #4
    add     r11, r11, #4
    ldmia   r10, {r0, r1, r2, r3}
    stmia   r11, {r0, r1, r2, r3}
    LDR     r6  ,[r10]
    LDR     r7  ,[r11]
    cmp     r6, r7
    bne     cmp_fail
    add     r10, r10, #4
    add     r11, r11, #4
    LDR     r6  ,[r10]
    LDR     r7  ,[r11]
    cmp     r6, r7
    bne     cmp_fail
    add     r10, r10, #4
    add     r11, r11, #4
    LDR     r6  ,[r10]
    LDR     r7  ,[r11]
    cmp     r6, r7
    bne     cmp_fail
    add     r10, r10, #4
    add     r11, r11, #4
    LDR     r6  ,[r10]
    LDR     r7  ,[r11]
    cmp     r6, r7
    bne     cmp_fail
    add     r10, r10, #4
    add     r11, r11, #4
    ldmia   r10, {r0, r1, r2, r3}
    stmia   r11, {r0, r1, r2, r3}
    LDR     r6  ,[r10]
    LDR     r7  ,[r11]
    cmp     r6, r7
    bne     cmp_fail
    add     r10, r10, #4
    add     r11, r11, #4
    LDR     r6  ,[r10]
    LDR     r7  ,[r11]
    cmp     r6, r7
    bne     cmp_fail
    add     r10, r10, #4
    add     r11, r11, #4
    LDR     r6  ,[r10]
    LDR     r7  ,[r11]
    cmp     r6, r7
    bne     cmp_fail
    add     r10, r10, #4
    add     r11, r11, #4
    LDR     r6  ,[r10]
    LDR     r7  ,[r11]
    cmp     r6, r7
    bne     cmp_fail
    add     r10, r10, #4
    add     r11, r11, #4
    subs    r9, r9, #0x40
    bne     mem_copy_loop
    //mov     r5, r0
    //mov     r7, r1
    //LDR     r0, =count_string
    //mov     r1, r8
    //BL      printk 
   // mov     r0, r5
   // mov     r1, r7
    mov     r2, #1
    str      r2,[r5]
    b       done

cmp_fail:
   /* 
    LDR     r0, =check_fail_string
    BL      printk
    */
    mov     r2,#0
    str     r2, [r5]

done:
    /*
    ldr     r1, [r5]
    LDR     r0, =done_string
    BL      printk
    */
    ldmfd       sp!, {r0-r12, pc}
ENDPROC(power_loop_l2)



.data
input_string:
    .asciz "input value %x\r\n"
parm_string:
    .asciz "value  %x\r\n"
count_string:
    .asciz "loop count %x\r\n"
check_fail_string:
    .asciz "==========\r\ncheck fail.............\r\n==========\r\n"
done_string:
    .asciz "test done %x\r\n"
