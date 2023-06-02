/*
 * misc.c
 *
 * This is a collection of several routines from gzip-1.0.3
 * adapted for Linux.
 *
 * malloc by Hannu Savolainen 1993 and Matthias Urlichs 1994
 *
 * Modified for ARM Linux by Russell King
 *
 * Nicolas Pitre <nico@visuaide.com>  1999/04/14 :
 *  For this code to run directly from Flash, all constant variables must
 *  be marked with 'const' and all other variables initialized at run-time
 *  only.  This way all non constant variables will end up in the bss segment,
 *  which should point to addresses in RAM and cleared to 0 on start.
 *  This allows for a much quicker boot time.
 */

unsigned int __machine_arch_type;

#include <linux/compiler.h>	/* for inline */
#include <linux/types.h>
#include <linux/linkage.h>

static void putstr(const char *ptr);
extern void error(char *x);

#include <mach/uncompress.h>

/* avoid EXPORT_SYMBOL warning. */
#define EXPORT_SYMBOL(x)

#if GPIO_HAL_MODE
HAL_IO(gpio)
#else
DIR_IO(gpio)
#endif
#if RCT_HAL_MODE
HAL_IO(rct)
#else
DIR_IO(rct)
#endif
#if UART_HAL_MODE
HAL_IO(uart)
#else
DIR_IO(uart)
#endif
#ifdef CONFIG_DEBUG_ICEDCC

#if defined(CONFIG_CPU_V6) || defined(CONFIG_CPU_V6K) || defined(CONFIG_CPU_V7)

static void icedcc_putc(int ch)
{
	int status, i = 0x4000000;

	do {
		if (--i < 0)
			return;

		asm volatile ("mrc p14, 0, %0, c0, c1, 0" : "=r" (status));
	} while (status & (1 << 29));

	asm("mcr p14, 0, %0, c0, c5, 0" : : "r" (ch));
}


#elif defined(CONFIG_CPU_XSCALE)

static void icedcc_putc(int ch)
{
	int status, i = 0x4000000;

	do {
		if (--i < 0)
			return;

		asm volatile ("mrc p14, 0, %0, c14, c0, 0" : "=r" (status));
	} while (status & (1 << 28));

	asm("mcr p14, 0, %0, c8, c0, 0" : : "r" (ch));
}

#else

static void icedcc_putc(int ch)
{
	int status, i = 0x4000000;

	do {
		if (--i < 0)
			return;

		asm volatile ("mrc p14, 0, %0, c0, c0, 0" : "=r" (status));
	} while (status & 2);

	asm("mcr p14, 0, %0, c1, c0, 0" : : "r" (ch));
}

#endif

#define putc(ch)	icedcc_putc(ch)
#endif

static void putstr(const char *ptr)
{
	char c;

	while ((c = *ptr++) != '\0') {
		if (c == '\n')
			putc('\r');
		putc(c);
	}

	flush();
}

/*
 * gzip declarations
 */
extern char input_data[];
extern char input_data_end[];

unsigned char *output_data;

unsigned long free_mem_ptr;
unsigned long free_mem_end_ptr;

#ifndef arch_error
#define arch_error(x)
#endif

void error(char *x)
{
	arch_error(x);

	putstr("\n\n");
	putstr(x);
	putstr("\n\n -- System halted");

	while(1);	/* Halt */
}

asmlinkage void __div0(void)
{
	error("Attempting division by 0!");
}

extern int do_decompress(u8 *input, int len, u8 *output, void (*error)(char *x));

static inline void clk_setting(void)
{
	u32 type;
    u32 ARMFreq;

	type = *(volatile u32 *)CONFIG_U2K_PHY_TYPE;

	if(type != 0){
#ifdef CONFIG_PHY_USE_SD_CLK
		gk_gpio_setbitsl(GK_PA_GPIO0 + 0x3c, 0x00000001);
		gk_rct_clrbitsl(GK_PA_RCT + 0x108, 0x00000018);
		gk_rct_writel(GK_PA_RCT + 0x014, 0x0011202D);
		gk_rct_writel(GK_PA_RCT + 0x01c, 0x00D55555);
		gk_rct_writel(GK_PA_RCT + 0x018, 0x00000016);
#endif
#ifdef CONFIG_PHY_USE_AO_MCLK
		gk_rct_writel(GK_PA_RCT + 0x024, 0x00124021);
		gk_rct_writel(GK_PA_RCT + 0x078, 0x00555555);
		if(type == 2)//50MHz
 		   gk_rct_writel(GK_PA_RCT + 0x084, 0x00000002);
 	    else//25MHz
           gk_rct_writel(GK_PA_RCT + 0x084, 0x00000004);
		gk_rct_writel(GK_PA_RCT + 0x080, 0x00000001);
#endif
    }
    else
    {
		gk_gpio_clrbitsl(GK_PA_GPIO0 + 0x3c, 0x00000001);
#ifdef CONFIG_PHY_USE_SD_CLK
		gk_rct_setbitsl(GK_PA_RCT + 0x108, 0x00000018);
		gk_rct_writel(GK_PA_RCT + 0x014, 0x0011202E);
		gk_rct_writel(GK_PA_RCT + 0x01c, 0x00000000);
		gk_rct_writel(GK_PA_RCT + 0x018, 0x00000017);
#endif

#ifdef CONFIG_PHY_USE_AO_MCLK
		gk_rct_writel(GK_PA_RCT + 0x024, 0x00124020);
		gk_rct_writel(GK_PA_RCT + 0x078, 0x00000000);
		gk_rct_writel(GK_PA_RCT + 0x084, 0x00000004);
		gk_rct_writel(GK_PA_RCT + 0x080, 0x00000001);
#endif
    }

//misc clock configure
    // SFLASH ioctrl
	gk_rct_writel(GK_PA_RCT + 0x0198, 0x00000011);

    // Sensor ioctrl
    gk_rct_writel(GK_PA_RCT + 0x019C, 0x00000012);
    // modify idsp arm freq
    //[25:20]REFDIV
    //[18:16]PSTDIV2
    //[14:12]PSTDIV1
    //[11:0]FBDIV
    // Fout=(Fin / REFDIV)*FBDIV/ POSTDIV1/ POSTDIV2
    // 0x53 = 1G
    // 0x4B = 900M
    // 0x43 = 800M
    // 0x32 = 600M
    // 0x21 = 400M
    // 0x29 = 500M
    //putstr("modify idsp arm freq\n");
    ARMFreq = *(volatile u32 *)CONFIG_U2K_ARM_FREQ;
	gk_rct_writel(GK_PA_RCT + 0x8c, ARMFreq);
}


void
decompress_kernel(unsigned long output_start, unsigned long free_mem_ptr_p,
		unsigned long free_mem_ptr_end_p,
		int arch_id)
{
	int ret;

	output_data		= (unsigned char *)output_start;
	free_mem_ptr		= free_mem_ptr_p;
	free_mem_end_ptr	= free_mem_ptr_end_p;
	__machine_arch_type	= arch_id;

	arch_decomp_setup();

	clk_setting();

	putstr("Uncompressing Linux...");
	ret = do_decompress(input_data, input_data_end - input_data,
			    output_data, error);
	if (ret)
		error("decompressor returned an error");
	else
		putstr(" done, booting the kernel.\n");
}
