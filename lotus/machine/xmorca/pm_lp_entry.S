
#include <linux/lotus/timestamp.h>

//#define CRC16_CHECK_SUPPORT  1
#define CRC_RESULT_SAVE_ADDR	0x401bbe0
#undef SHOW_TIMER0_VALUE

.global suspend_fun_size
.global xmorca_suspend

.text

.macro reg_write reg, val
	ldr	r0, =\reg
	ldr	r1, =\val
	str	r1, [r0]
	dsb
.endm

.macro reg_read reg
	ldr	r1, =\reg
	ldr	r0, [r1]
	dsb
.endm
#ifdef CRC16_CHECK_SUPPORT
/*
 * multinomial:0x04C11DB7
 * r0: pointer to data
 * r1: data len
 * out: r0, crc16
 */
.macro crc16_calc
	ldr	r2, =0xFFFF     @ 初始 CRC 值
	ldr	r3, =crc16_table @ 加载查找表地址
	cmp	r1, #0          @ 检查长度
	beq	.done_table

.loop_table:
	ldrb	r12, [r0], #1   @ 加载一个字节
	eor	r12, r2, r12    @ CRC ^ data
	and	r12, r12, #0xFF @ 取低8位
	ldrh	r4, [r3, r12, lsl #1] @ 查表(每项2字节)
	eor	r2, r4, r2, lsr #8 @ 更新CRC

	subs	r1, r1, #1      @ 计数器减1
	bne	.loop_table     @ 继续循环

.done_table:
	mov	r0, r2          @ 返回结果
.endm
#endif

.align
xmorca_suspend:
	STOPWATCH_TRIGGER
#ifdef SHOW_TIMER0_VALUE
	reg_read 0x12000004
	mov	r10, r0
#endif
 	dsb
 	isb

enable_cache:
	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #(1 << 12)
	//orr r0, r0, #(1 << 2)
	mcr	p15, 0, r0, c1, c0, 0
	isb

#ifdef CRC16_CHECK_SUPPORT
	/* */
	ldr	r0, =0x120d0000
	ldr	r1, [r0]
	and	r1, r1, #0x7
	cmp	r1, #1
	bne	ddr_64m_crc
ddr_128m_crc:
	ldr	r1, =0x8000000
	b	do_crc
ddr_64m_crc:
	ldr	r1, =0x4000000
do_crc:
	ldr	r0, =0x40000000
	crc16_calc
crc_end:
	/* crc save in lpds_param ddr_suspend_crc */
	ldr	r1, =CRC_RESULT_SAVE_ADDR
	str	r0, [r1]

disable_cache:
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #(1 << 12)
	bic	r0, r0, #(1 << 2)
	mcr	p15, 0, r0, c1, c0, 0
	isb
#endif

/*
 * first set ddr to low power mode
 */
ddr3_lp_entry:
ddr2_lp_entry:
	/*
	 * set ddr register
	 * set ddr to self flush mode
	 */

	reg_write 0x120d315c, 0x7200a0

	reg_write 0x120d313c, 0x1000005

/*
 * check if ddr is idle
 */
	bl	timer0_init
	bl	timer0_start
	/* set timeout is 10ms, timer0's clk is 24M HZ */
	ldr	r2, =240000
ddr_idle:
	ldr	r1, ddr_qos_empty_reg
	ldr	r0, [r1]
	cmp	r0, #1
	beq	ddr_power_down
	bl	timer0_get_val
	cmp	r0, r2
	bcc	ddr_idle
ddr_idle_timeout:
	adr	r0, _str_ddr_idle_timeout
	bl	serial_puts

	/* return to kernel */
	ldr	r0, =0x120f0050
	ldr	r1, [r0]
	mov	pc, r1

ddr_power_down:
	STOPWATCH_TRIGGER

	reg_write 0x120d3138, 0x1

	reg_write 0x120d3138, 0x0

	/*
	 * delay 1 us
	 */

	mov	r0, #1
	bl	delay_us

	/*
	 * power down ddr io
	 */

power_off:
	/*
	 * entry str mode
	 */

	/* ioctrl rt en */

	reg_write 0x120f0008, 0

	/*delay 50us */
	mov	r0, #50
	bl	delay_us

	STOPWATCH_TRIGGER
#ifdef SHOW_TIMER0_VALUE
	reg_read 0x12000004
	mov	r11, r0

	mov	r0, r11
	bl	serial_put_hex
	adr	r0, _str_space
	bl	serial_puts
	mov	r0, r10
	bl	serial_put_hex
	adr	r0, _str_backslash_r_n
	bl	serial_puts
#endif

dump_stopwatch:
	/* Check dump flag */
	reg_read 0x120200bc
	cmp	r0, 0xA
	bne	clr_rtc_int
	/* Dump all stopwatch */
	adr	r0, _str_stopwatch
	bl	serial_puts
	ldr	r5, =0
_stopwatch_loop:
	mov	r0, r5
	bl	serial_put_hex
	adr	r0, _str_space
	bl	serial_puts
	ldr	r1, =REG_STOPWATCH_SEL
	str	r5, [r1]
	mov	r0, #4
	bl	delay_us
	adr	r0, _str_0x
	bl	serial_puts
	reg_read REG_STOPWATCH_RESULT
	bl	serial_put_hex
	adr	r0, _str_backslash_r_n
	bl	serial_puts
	add	r5, r5, #1
	cmp	r5, #STOPWATCH_NUM
	bne	_stopwatch_loop
	ldr	r0, =10000
	bl	delay_us

clr_rtc_int:
#ifdef _RTC_AOV_DEBUG
	/* show rtc int */
	reg_write 0x120E0004, 0x01960000
_rtc_busy1:
	reg_read  0x120E0004
	mov	r5, r0
	lsr	r0, r0, #31
	cmp	r0, #0
	bne	_rtc_busy1
	adr	r0, _str_rtc_int
	bl	serial_puts
	adr	r0, _str_0x
	bl	serial_puts
	lsr	r5, r5, #8
	and	r5, r5, #0xff
	mov	r0, r5
	bl	serial_put_hex
	adr	r0, _str_backslash_r_n
	bl	serial_puts
	ldr	r0, =10000
	bl	delay_us
	/* clear rtc int */
	adr	r0, _str_clear_rtc_int
	bl	serial_puts
#endif
	reg_write 0x120E0004, 0x01150001
	ldr	r0, =42
	bl	delay_us
_check_rtc_busy:
	reg_read  0x120E0004
	lsr	r0, r0, #31
	cmp	r0, #0
	beq	start_standby
	adr	r0, _str_rtc_busy
	bl	serial_puts
	b	_check_rtc_busy
start_standby:
	adr	r0, _str_enter_standby
	bl	serial_puts
	dsb
	isb
	/* start standby */
	/* reg_write 0x120f0010, 0x100161 */
	ldr	r1, pmc_base_reg
	ldr	r0, [r1, 0x10]
	bic	r0, r0, #0x1    //not standby mode
	orr	r0, r0, #0x2	//powerdown mode
	str	r0, [r1, 0x10]

	/*
	 * arm entry wfi
	 */
arm_suspend:
	dsb
	isb
	wfi
wait:
	B	wait

/*
 * void serial_put_hex(int hex);
 */
.align  2
.global serial_put_hex
.type   serial_put_hex, %function
serial_put_hex:
	mov	r2, #28
wait2:
	ldr	r3, [r1, #24]
	tst	r3, #32
	bne	wait2

	mov	r3, #0xF
	lsr	r1, r0, r2
	and	r3, r3, r1
	cmp	r3, #9
	addle	r3, r3, #0x30   @ r3 = r3 + '0'
	addgt	r3, r3, #55     @ r3 = r3 - 10 + 'A'
	ldr	r1, uart_base
	str	r3, [r1, #0]
	cmp	r2, #0
	beq	exit2
	sub	r2, r2, #4
	b	wait2
exit2:
bx      lr

/*
 * void serial_puts(const char *ss);
 */
.align  2
.global serial_puts
.type   serial_puts, %function
serial_puts:
	ldr	r1, uart_base
	b	next_char
output:
	ldr	r3, [r1, #24]
	tst	r3, #32
	bne	output
	str	r2, [r1, #0]
	add	r0, r0, #1
next_char:
	ldrb	r2, [r0]
	cmp	r2, #0
	bne	output
	bx	lr

/*
 * void timer0_init(void)
 */
.align	2
.global timer0_init
.type	timer0_init, %function
timer0_init:
	dsb	sy
	reg_write 0x12000000, 0x0
	reg_write 0x12000008, 0xc2
	bx	lr

/*
 * void timer0_deinit(void)
 */
.align	2
.global timer0_deinit
.type	timer0_deinit, %function
timer0_deinit:
	dsb	sy
	reg_write 0x12000008, 0x0
	bx	lr

/*
 * void timer0_start(void)
 */
.align	2
.global timer0_start
.type	timer0_start, %function
timer0_start:
	dsb sy
	reg_write 0x12000000, 0xffffffff
	bx	lr

/*
 * unsigned int timer0_get_val(void)
 */
.align	2
.global timer0_get_val
.type	timer0_get_val, %function
timer0_get_val:
	reg_read 0x12000004
	mvn	r0, r0
	bx	lr

/*
 * void delay_us(unsigned int us)
 */
.align	2
.global delay_us
.type	delay_us, %function
delay_us:
	mov	r1, #24
	mul	r2, r1, r0
	reg_write 0x12000000, 0xffffffff
delay_wait:
	reg_read 0x12000004
	mvn	r0, r0
	cmp	r0, r2
	bcc	delay_wait
	bx	lr

.ltorg

#ifdef CRC16_CHECK_SUPPORT
crc16_table:
	.hword \
		0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241, \
		0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440, \
		0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40, \
		0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841, \
		0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40, \
		0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41, \
		0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641, \
		0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040, \
		0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240, \
		0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441, \
		0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41, \
		0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840, \
		0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41, \
		0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40, \
		0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640, \
		0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041, \
		0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240, \
		0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441, \
		0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41, \
		0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840, \
		0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41, \
		0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40, \
		0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640, \
		0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041, \
		0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241, \
		0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440, \
		0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40, \
		0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841, \
		0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40, \
		0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41, \
		0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641, \
		0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
#endif

pmc_base_reg:
	.word	0x120f0000
ddr_qos_empty_reg:
	.word	0x120d2900
dfi_base_reg:
	.word	0x120d3100
dfi_cfg5c_data:
	.word	0x7200a0
dfi_cfg3c_data:
	.word	0x1000005
uart_base:
	.word CONSOLE_UART_BASE
_str_stopwatch:
	.ascii "STOPWATCH:\r\n\0"
_str_0x:
	.ascii "0x\0"
_str_space:
	.ascii " \0"
_str_backslash_r_n:
	.ascii "\r\n\0"

_str_ddr_idle_timeout:
	.ascii "ddr idle timeout\r\n\0"
_str_enter_standby:
	.ascii "PD\r\n\0"
_str_clear_rtc_int:
	.ascii "Clear RTC Int\r\n\0"
_str_rtc_busy:
	.ascii "RTC BUSY!\r\n\0"
_str_rtc_int:
	.ascii "RTC INT: \0"
suspend_fun_size:
	.word	. - xmorca_suspend

.align 2
.global xmorca_dismmu_jump
.type   xmorca_dismmu_jump, %function
xmorca_dismmu_jump:
	dsb
	isb
	mov r1, r0
	mrc p15, 0, r0, c1, c0, 0
	bic r0, r0, #0x1
	mcr p15, 0, r0, c1, c0, 0
	mov pc, r1
