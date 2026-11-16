#ifndef __PM_H__
#define __PM_H__

#include <soc/base.h>


/*
 *
 *	|-----------------------|
 *	|			|
 *	|			|
 *	|	RESUME_SP	|
 *	|			|
 *	|-----------------------|
 *	|			|
 *	|  	  PARAM		|
 *	|			|
 *	|-----------------------|
 *	|			|
 *	|			|
 *	|	SLEEP_TEXT      |
 *	|			|
 *	|			|
 *	|-----------------------|
 *	|			|
 *	|			|
 *	|	RESUME_TEXT	|
 *	|			|
 *	|			|
 *	|-----------------------|
 *	|			|
 *	|  RESUME_BOOTU_TEXTP	|
 *	|			|
 *	|-----------------------|     <------------ TCSM_START 0xb2400000
 *
 */
#define SRAM_MEMORY_START	        0xb2400000
#define SRAM_MEMORY_END		        0xb2407ff8
#define RTC_MEMORY_START	        0xb0004000
#define RTC_MEMORY_END              0xb0005000


#define NORMAL_RESUME_SP		    (SRAM_MEMORY_END - 4)
#define	NORMAL_RESUME_CODE1_ADDR	SRAM_MEMORY_START
#define NORMAL_RESUME_CODE1_LEN		64
#define	NORMAL_RESUME_CODE2_ADDR	(NORMAL_RESUME_CODE1_ADDR + NORMAL_RESUME_CODE1_LEN)
#define NORMAL_RESUME_CODE2_LEN		(4096)
#define NORMAL_SLEEP_CODE_ADDR		(NORMAL_RESUME_CODE2_ADDR + NORMAL_RESUME_CODE2_LEN)
#define NORMAL_SLEEP_CODE_LEN		(3072)
#define NORMAL_PARAM_ADDR		    (NORMAL_SLEEP_CODE_ADDR + NORMAL_SLEEP_CODE_LEN)
#define NORMAL_PARAM_LEN		    (sizeof(struct sleep_param))


#define FASTBOOT_RESUME_SP		    (FASTBOOT_DATA_ADDR - 4)
#define FASTBOOT_RESUME_CODE1_LEN	64
//RESUME code start addr
#define FASTBOOT_RESUME_CODE1_ADDR	RTC_MEMORY_START
//RESUME code end addr
#define FASTBOOT_RESUME_CODE2_ADDR	(FASTBOOT_RESUME_CODE1_ADDR + FASTBOOT_RESUME_CODE1_LEN)
//RESUME code length
#define FASTBOOT_RESUME_CODE_LEN	0xe00//4*1024 - 512

/*512 bytes*/
#define FASTBOOT_DATA_ADDR		    (FASTBOOT_RESUME_CODE1_ADDR + FASTBOOT_RESUME_CODE_LEN)
#define FASTBOOT_DATA_LEN		    (RTC_MEMORY_END - FASTBOOT_DATA_ADDR)
/*
 *Because RTC ram is not enough to put resume and suspend code, put the suspend code in the 128KB position behind ORAM.
 * */
#define FASTBOOT_SLEEP_CODE_ADDR	(0xb2620000)
#define FASTBOOT_SLEEP_CODE_LEN		(4 * 1024)




struct sleep_param {
	suspend_state_t state;
	unsigned int pdt;
	unsigned int dpd;
	unsigned int dlp;
	unsigned int autorefresh;
	unsigned int cpu_div;
	unsigned int uart_base;
	unsigned int sleep_level;
};

#define sleep_param ((struct sleep_param *)NORMAL_PARAM_ADDR)

enum {
	IDLE,
	IDLE_PD,
	SLEEP,
	FASTBOOT,
};



void load_func_to_tcsm(unsigned int tcsm_addr,unsigned int *f_addr,unsigned int size);


long long save_goto(unsigned int func);
int restore_goto(unsigned int func);



#define reg_ddr_phy(x)   (*(volatile unsigned int *)(0xb3011000 + ((x) << 2)))



extern int bc_idx;



/************************************************
 *		debug interface
 ***********************************************/

//#define DEBUG_PM
#define PRINT_DEBUG


#define U_IOBASE	    0xb0031000
#define UART_OFF        (0x1000)


#define OFF_TDR         (0x00)
#define OFF_LCR         (0x0C)
#define OFF_LSR         (0x14)
#define LSR_TDRQ        (1 << 5)
#define LSR_TEMT        (1 << 6)


#ifdef PRINT_DEBUG
#define TCSM_PCHAR(x)                                                   \
	*((volatile unsigned int*)(U_IOBASE+OFF_TDR)) = x;              \
while ((*((volatile unsigned int*)(U_IOBASE + OFF_LSR)) & (LSR_TDRQ | LSR_TEMT)) != (LSR_TDRQ | LSR_TEMT));
#else
#define TCSM_PCHAR(x)
#endif

#define TCSM_DELAY(x)						\
	do{							\
	register unsigned int i = x;				\
	while(i--)						\
		__asm__ volatile("nop\n\t");			\
	}while(0)

static inline void serial_put_hex(unsigned int x) {
	int i;
	unsigned int d;
	for(i = 7;i >= 0;i--) {
		d = (x  >> (i * 4)) & 0xf;
		if(d < 10) d += '0';
		else d += 'A' - 10;
		TCSM_PCHAR(d);
	}
	TCSM_PCHAR('\r');
	TCSM_PCHAR('\n');
}


#define WAIT_RTC_DONE(r,c,s)\
    do{ \
        while(r){ \
            c++; \
            if(c>5000){ \
                TCSM_PCHAR(s); \
                TCSM_DELAY(10); \
            } \
        } \
        c = 0; \
    }while(0);

static inline unsigned int  rtc_read_reg(unsigned int reg)
{
    int count = 0;
    WAIT_RTC_DONE((!((*(volatile unsigned int *)0xb0003000 >> 7) & 0x1)),count,'!');
    return *(volatile unsigned int *)reg;
}


static inline void rtc_write_reg(unsigned int reg, unsigned int val)
{
    int count = 0;
    do{
        WAIT_RTC_DONE((!((*(volatile unsigned int *)0xb0003000 >> 7) & 0x1)),count,'o');
        *(volatile unsigned int *)0xb000303c = 0xa55a;
        WAIT_RTC_DONE((!((*(volatile unsigned int *)0xb000303c >>31) & 0x1)),count,'v');
        WAIT_RTC_DONE((!((*(volatile unsigned int *)0xb0003000 >> 7) & 0x1)),count,'e');
        *(volatile unsigned int *)reg = val;
        WAIT_RTC_DONE((!((*(volatile unsigned int *)0xb0003000 >> 7) & 0x1)),count,'r');
    }while(val != rtc_read_reg(reg));
}
#endif/*PM*/
