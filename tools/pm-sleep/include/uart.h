#ifndef __UART_H__
#define __UART_H__
#define TCSM_DELAY(x)								\
	do{												\
		register unsigned int i = x;				\
		while(i--)									\
			__asm__ volatile(".set push      \n\t"	\
							 ".set mips32\n\t"		\
							 "nop\n\t"				\
							 ".set mips32\n\t"		\
		                     ".set pop      \n\t");	\
	}while(0)

void serial_putc(char x);
void serial_put_hex(unsigned int x);
void serial_setid(int uart_id);
#endif	/* __UART_H__ */
