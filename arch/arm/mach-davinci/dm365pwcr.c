#include <linux/io.h>
#include "dm365pwcr.h"
/* Alexander V. Shadrin, Virt2Real
 * This file is intended to support PRTCSS module
 */
#define DM365_PRTCSS_BASE	0x01c69000

#define DM365_PRTCIF_PID			0x00
#define DM365_PRTCIF_CTLR			0x04
#define DM365_PRTCIF_LDATA			0x08
#define DM365_PRTCIF_UDATA			0x0C
#define DM365_PRTCIF_INTEN			0x10
#define DM365_PRTCIF_INTFLG			0x14


#define DM365_PRTCSS_GO_OUT         0x00   //output pin output data register Section 4.2.1
#define DM365_PRTCSS_GIO_OUT        0x01   // input/output pin output data register Section 4.2.2
#define DM365_PRTCSS_GIO_DIR        0x02   //input/output pin direction register Section 4.2.3
#define DM365_PRTCSS_GIO_IN         0x03   //input/output pin input data register Section 4.2.4
#define DM365_PRTCSS_GIO_FUNC       0x04  //input/output pin function register

/* DAVINCI_PRTCIF_CTLR bit fields */
#define DM365_PRTCIF_CTLR_BUSY		    (1<<31)
#define DM365_PRTCIF_CTLR_SIZE		    (1<<25)
#define DM365_PRTCIF_CTLR_DIR			(1<<24)
#define DM365_PRTCIF_CTLR_BENU_MSB		(1<<23)
#define DM365_PRTCIF_CTLR_BENU_LSB		(1<<20)
#define DM365_PRTCIF_CTLR_BENU_MASK		(0x00F00000)
#define DM365_PRTCIF_CTLR_BENL_MSB		(1<<19)
#define DM365_PRTCIF_CTLR_BENL_LSB		(1<<16)
#define DM365_PRTCIF_CTLR_BENL_MASK		(0x000F0000)
#define DM365_PRTCSS_GO2_FUNC_32K       (0x01)
#define DM365_PRTCSS_GO2_MASK           (0x07)

static void prtcif_write(u32 val, u32 addr){
	volatile void* base = (volatile void*)DM365_PRTCSS_BASE;
	writel(val, base + addr);
}

static u32 prtcif_read(u32 addr){
	volatile void* base = (volatile void*)DM365_PRTCSS_BASE;
	return readl(base + addr);
}

static void prtcif_wait(void){
    while (prtcif_read(DM365_PRTCIF_CTLR) & DM365_PRTCIF_CTLR_BUSY){};
}

static void prtcss_write(unsigned long val, u8 addr)
{
	prtcif_wait();
	prtcif_write(DM365_PRTCIF_CTLR_BENL_LSB | addr, DM365_PRTCIF_CTLR);
	prtcif_write(val, DM365_PRTCIF_LDATA);
	prtcif_wait();
}

static inline u8 prtcss_read(u8 addr)
{
	prtcif_wait();
	prtcif_write(DM365_PRTCIF_CTLR_DIR|DM365_PRTCIF_CTLR_BENL_LSB|addr, DM365_PRTCIF_CTLR);
	prtcif_wait();
	return prtcif_read(DM365_PRTCIF_LDATA);
}

void configure_prtcss_32k(void){
     unsigned long go_function = prtcss_read(DM365_PRTCSS_GIO_FUNC);
     go_function &= ~(DM365_PRTCSS_GO2_MASK); //Clear all possible go_func2 functions
     go_function |= DM365_PRTCSS_GO2_FUNC_32K;//Setting 32K jutput
     prtcss_write(go_function, DM365_PRTCSS_GIO_FUNC);
}
