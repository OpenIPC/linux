#ifndef _XMSFC_V2_H_
#define _XMSFC_V2_H_


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

#define XM_SFC_V2_CTRL          __io_address(0x100F4000)	
#define XM_SFC_V2_CFG1          __io_address(0x100F4004)	
#define XM_SFC_V2_CFG2          __io_address(0x100F4008)	
#define XM_SFC_V2_ADDR          __io_address(0x100F400c)	
#define XM_SFC_V2_OP            __io_address(0x100F4010)	
#define XM_SFC_V2_CMD           __io_address(0x100F4014)	
#define XM_SFC_V2_SRW           __io_address(0x100F4018)	
#define XM_SFC_V2_RW_NUM        __io_address(0x100F401c)	
#define XM_SFC_V2_PRE_CMD       __io_address(0x100F4020)	
#define XM_SFC_V2_PST_CMD       __io_address(0x100F4024)	
#define XM_SFC_V2_TDEL          __io_address(0x100F4028)	
#define XM_SFC_V2_ST            __io_address(0x100F402c)	
#define XM_SFC_V2_R_SR_ID       __io_address(0x100F4030)	
#define XM_SFC_V2_RINT          __io_address(0x100F4034)	
#define XM_SFC_V2_INT_MSK       __io_address(0x100F4038)	
#define XM_SFC_V2_INT_ST        __io_address(0x100F403c)	
#define XM_SFC_V2_MODE_ST       __io_address(0x100F4040)	
#define XM_SFC_V2_CACHE_CMD     __io_address(0x100F4044)	
#define XM_SFC_V2_CACHE_BBT     __io_address(0x100F4048)	
#define XM_SFC_V2_RAM			__io_address(0x100F6000)


#define XM_SFC_V2_BUF_SIZE		256
#define XM_SFC_V2_BASE_LEN      0x800

#define XMSF_BASE_ADDR			0x60000000
#define XMSF_BASE_LEN   	    0x01000000         /*16MB*/

#define SPI_NOR_CMD_WRITE_ST     	0x01
#define SPI_NOR_CMD_WRITE_DATA   	0x02
#define SPI_NOR_CMD_READ_DATA    	0x03
#define SPI_NOR_CMD_READ_ST      	0x05
#define SPI_NOR_CMD_READ_ST2     	0x35
#define SPI_NOR_CMD_READ_JEDECID	0x9F            /*读ID*/

#define FL_OP1  0x01
#define FL_OP2  0x02
#define FL_OP3  0x03
#define FL_OP4  0x04
#define FL_OP5  0x05
#define FL_OP6  0x06
#define FL_OP7  0x07
#define FL_OP8  0x08
#define FL_OP9  0x09
#define FL_OP10 0x0A

#define FL_OP1_OK  0x01
#define FL_OP2_OK  0x02
#define FL_OP3_OK  0x04
#define FL_OP4_OK  0x08
#define FL_OP5_OK  0x10
#define FL_OP6_OK  0x20
#define FL_OP7_OK  0x40
#define FL_OP8_OK  0x80
#define FL_OP9_OK  0x100
#define FL_OP10_OK 0x200


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
