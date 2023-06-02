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

#define CPU_FA726   0x1
#define CPU_FA626   0x2
#define PCI_HOST    0x40
#define PCI_DEV     0x80

#ifdef CONFIG_PLATFORM_GM8210
/* return x0:726, x1:626, 1x:device */
static unsigned int mode_detect(void)
{
	u32 ret = 0;
	u32 value;

    value = (readl(PMU_FTPMU010_PA_BASE + 0x04) >> 19) & 0x3;
    if (value == 0x1)
		ret |= PCI_DEV;
    else
        ret |= PCI_HOST;

	__asm__ __volatile__ ("mrc p15, 0, %0, c0, c0, 0\t\n": "=r"(value));

	if (((value >> 0x4) & 0xFFF) == 0x626)
     	ret |= CPU_FA626;
    else
        ret |= CPU_FA726;

	return ret;
}
#endif

#ifdef CONFIG_PLATFORM_GM8210
static void gm_putc(int c)
{
    unsigned long base;

    base = (mode_detect() & CPU_FA726) ? UART_FTUART010_0_PA_BASE : UART_FTUART010_1_PA_BASE;

    while ((readl(base + SERIAL_LSR) & SERIAL_LSR_THRE) == 0)
        barrier();

    writel(c, base + SERIAL_THR);
}

#define putc(ch)	gm_putc(ch)
#endif /* CONFIG_PLATFORM_GM8210 */

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

#ifdef CONFIG_PLATFORM_GM8210
#include "start8210.c"
unsigned int boot_addr = 0;
#endif /* CONFIG_PLATFORM_GM8210 */

extern int do_decompress(u8 *input, int len, u8 *output, void (*error)(char *x));

#define BOOT_IMAGE_SIZE     0x200
#define FC7500_ADDR_LIMIT   0x1000000
#define FA626_SIGNATURE     0x06260626
#define FA626_BOOT_PC       0x18100040  //changeable, 256+128MB+0x40 uImage header
#define FA626_GO            0x35737888

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

	putstr("Uncompressing Linux...");
	ret = do_decompress(input_data, input_data_end - input_data,
			    output_data, error);
	if (ret)
		error("decompressor returned an error");
	else
		putstr(" done, booting the kernel.\n");

#ifdef CONFIG_FA626
    /*
     * 1. copy the bootcode to the memory address 0x0
     * 2. copy linux image of FA626 to the designate address.
     * 3. let the FA626 CPU go.
     * FA626 boot flow:
     *  a. FA726 waits FA626 to update 0x20(command offset) to FA626_SIGNATURE
     *  b. FA626 starts to wait 0x28 to be FA626_GO
     *  c. FA726 updates 0x24 to FA626_BOOT_PC and then updates 0x28 to FA626_GO
     *  d. FA626 loads FA626_BOOT_PC from 0x24 and jumps to specific PC.
     *
     * Note: in __setup_mmu of head.S, the zero address is mapped to NCNB in MMU already and whole
     *       4G memory is created to 1:1 mapping.
     */
    if (mode_detect() & CPU_FA726) {
        volatile unsigned int val, i, ttbr;

        /* work around to turn on write buffer */
        __asm__ __volatile__ ("mrc p15, 0, %0, c15, c0, 0\t\n":"=r"(val));
        val |= (0x1 << 20);
        __asm__ __volatile__ ("mcr p15, 0, %0, c15, c0, 0\t\n":"=r"(val));

       /*
        * copy the target image to the designate address and then decompress the image
        * Note: the MMU was established in __setup_mmu of head.S and the memory address from 0 ~
        * 0x01800000 is created in NCNB(see mov	r9, r0, lsr #18 and mov	r9, r9, lsl #18 of head.S),
        * so we don't need to consider cache sync problem. But for safety, we still do the sanity
        * check.
        */

#ifdef CONFIG_FC7500
	    /* copy the start code and linux
	     */
	    putstr("start to copy FC7500 core boot image at 0x18100000 and wait response... \n");

	    /* because memory address 0 contains FC7500 image, so we need to
	     * copy the front part of FC7500 to specific memory address for backup. FA626 will move it
	     * back.
	     */
		memcpy((char *)(FC7500_ADDR_LIMIT - BOOT_IMAGE_SIZE), (char *)boot_addr, BOOT_IMAGE_SIZE);
#endif
	    /* copy boot image */
	    memcpy((char *)boot_addr, inner, sizeof(inner));

	    /* for safety only, actually it is unnecessary because of 0-16M is NCNB in head.S */
	    __asm__ __volatile__ ("mrc p15, 0, %0, c2, c0, 0\t\n":"=r"(ttbr));
	    val = *((unsigned int *)ttbr);  //get first 1M for boot_addr
	    if (val & 0xc) {
	        /* for safety only, actually it is unnecessary */
	        __asm__ __volatile__ ("mov r3, #0\n"
	                              "mcr p15, 0, r3, c7, c10, 3\n"    /* Test and Clean DCache */
	                              "mcr p15, 0, r3, c7, c10, 4");	/* drain WB */
	       putstr("The start address in MMU is unexpected! \n");
	    }

	    //delay for a while to retire DDR write buffer
	    for (i = 0; i < 0x1000; i ++) {}

        //copy flash image for 626.

        /* step 1: release the fa626 reset
         */
        *(unsigned int *)(PMU_FTPMU010_PA_BASE + 0x4) |= (1 << 17);

        /* step 2: turn on fclk
         */
        val = *(unsigned int *)(PMU_FTPMU010_PA_BASE + 0xC);
        val &= ~(0x1 << 6);
        *(unsigned int *)(PMU_FTPMU010_PA_BASE + 0xC) = val;

        /* wait FA6 to update the response flag. 0x20 default value is 0x33333333
         */
        do {
            val = *(volatile unsigned int *)(boot_addr + 0x20);
        } while (val != FA626_SIGNATURE);

        putstr("wait FA6 core response ok \n");
        *(unsigned int *)(boot_addr + 0x24) = FA626_BOOT_PC;
        *(unsigned int *)(boot_addr + 0x28) = FA626_GO;  //start to boot FA626

        /* before moving pc to boot address, FA626 will update the staus again */
        do {
            val = *(volatile unsigned int *)(boot_addr + 0x20);
        } while (val == FA626_SIGNATURE);
        putstr("done, FA7 core now is booting. \n");

    }
#endif /* CONFIG_FA626 */

#ifdef CONFIG_FC7500
    /* only FA626 can copy FC7500 images to specific address */
	if ((mode_detect() & CPU_FA626) == CPU_FA626) {
        volatile unsigned int i, tmp_size = BOOT_IMAGE_SIZE, src_base = FC7500_ADDR_LIMIT - BOOT_IMAGE_SIZE;

        /* copy the start code and linux
         */
        putstr("start to copy FC7500 core boot image... \n");

        memcpy((char *)boot_addr, (char *)src_base, tmp_size);

        for (i = 0; i < 0x1000; i ++) {}

        /* pmu go
         */
        *(unsigned int *)(PMU_FTPMU010_PA_BASE + 0x4) |= (1 << 18);

        putstr("done, FA6 core now is booting. \n");
    }
#endif

}
