#ifndef _XMSFC_H_
#define _XMSFC_H_


#include <mach/hardware.h>
/*****************************************************************************/

#define _1K     (0x400)
#define _2K     (0x800)

#define _4K     (0x1000)
#define _8K     (0x2000)
#define _16K        (0x4000)
#define _32K        (0x8000)

#define _64K        (0x10000)
#define _128K       (0x20000)
#define _256K       (0x40000)
#define _512K       (0x80000)

#define _1M     (0x100000)
#define _2M     (0x200000)
#define _4M     (0x400000)
#define _8M     (0x800000)

#define _16M        (0x1000000)
#define _32M        (0x2000000)
#define _64M        (0x4000000)

#define INFINITE    (0xFFFFFFFF)
/*****************************************************************************/

#define XMSFC_REG_BASE_ADDR     __io_address(0x100F0000)

#define XMSFC_REG_MODE			(XMSFC_REG_BASE_ADDR + 0x00)	
#define XMSFC_REG_ADDR			(XMSFC_REG_BASE_ADDR + 0x04)      /* Flash memoryµØÖ·¼Ä´æÆ÷; */                                                     
#define XMSFC_REG_OP			(XMSFC_REG_BASE_ADDR + 0x08)     /* Ö¸Áî¼Ä´æÆ÷;             */                                                     
#define XMSFC_REG_CMD			(XMSFC_REG_BASE_ADDR + 0x0C)     /* */                                                     
#define XMSFC_REG_SRW			(XMSFC_REG_BASE_ADDR + 0x10)     /* flash±êÖ¾Ð´¼Ä´æÆ÷;      */                                                     
#define XMSFC_REG_WRNUM			(XMSFC_REG_BASE_ADDR + 0x14)     /* Flash Ð´¸öÊý; */                                                     
#define XMSFC_REG_RW_SR_BSEL   	(XMSFC_REG_BASE_ADDR + 0x18)     /* Flash¶ÁÐ´×´Ì¬¼Ä´æÆ÷Î»¿íÑ¡Ôñ*/
#define XMSFC_REG_DOUT  		(XMSFC_REG_BASE_ADDR + 0x34)     /* ¶ÁflashÊý¾Ý¼Ä´æÆ÷;      */                                                     
#define XMSFC_REG_ST     		(XMSFC_REG_BASE_ADDR + 0x38)     /* Flash¿ØÖÆÆ÷×´Ì¬¼Ä´æÆ÷;  */                                                     
#define XMSFC_REG_SRR    		(XMSFC_REG_BASE_ADDR + 0x3C)     /* flash±êÖ¾¶Á¼Ä´æÆ÷;      */      
#define XMSFC_REG_MODE_ST		(XMSFC_REG_BASE_ADDR + 0x4C)
#define XMSFC_REG_CACHE_CMD     (XMSFC_REG_BASE_ADDR + 0x80)

#define XMSFC_REG_BUF			(XMSFC_REG_BASE_ADDR + 0x400)     /* */                                                     
#define XMSFC_REG_BUF_SIZE		256
#define XMSFC_REG_BASE_LEN      0x800

#define XMSF_BASE_ADDR		0x08000000
#define XMSF_BASE_LEN       0x01000000         /*16MB*/

#define	XMSFC_OP1_OK  0x01 //
#define	XMSFC_OP2_OK  0x02 //
#define	XMSFC_OP3_OK  0x04 //
#define	XMSFC_OP4_OK  0x08 //
#define	XMSFC_OP5_OK  0x10 //
#define XMSFC_OP6_OK  0x20 //

// XMSFCASH operation command
#define XMSFC_OP1  0x01 //
#define XMSFC_OP2  0x02 //
#define XMSFC_OP3  0x03 //
#define XMSFC_OP4  0x04 //
#define XMSFC_OP5  0x05 //
#define XMSFC_OP6  0x06 //

/*#define XMSFC_CMD_WRITE_DATA	0x02*/
/*#define XMSFC_CMD_READ_DATA	0x03*/
#define XMSFC_CMD_READ_JEDECID	0x9F
#define XMSFC_CMD_WRITE_ST		0x01
#define XMSFC_CMD_READ_ST2  	0x35


#define DBG_MSG(_fmt, arg...) printk(KERN_INFO "%s(%d): " _fmt, __FILE__, __LINE__, ##arg);  

#define MTD_TO_HOST(_mtd)       ((struct xmsfc_host *)(_mtd))

#define FLAG_QUAD_ONE_ADDR      0x01

struct xmsfc_host {
	struct mtd_info mtd[1];
	void __iomem     *iobase;
	struct mutex    lock; 
	unsigned char cmd_erase;
	unsigned char cmd_write;
	unsigned char cmd_read;
	unsigned char dummy_byte;
};

struct xmsfc_params {
	const char *name;
	unsigned int jedec;
	unsigned int size;
	unsigned int erasesize;
	unsigned char cmd_erase;
	unsigned char cmd_write;
	unsigned char cmd_read;
	unsigned char cmd_quad_write;
	unsigned char cmd_quad_read;
	unsigned char flags;
};


#endif
