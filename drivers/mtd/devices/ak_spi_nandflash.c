 /**
 *  @file      /driver/mtd/devices/ak_spi_nand.c
 *  @brief     SPI Flash driver for Anyka AK39e platform.
 *  Copyright C 2016 Anyka CO.,LTD
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *  @author    luoyongchuang 
 *  @date      2016-05-17
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/math64.h>
#include <linux/sched.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/delay.h>
//nclude <mach-anyka/fha.h>
//nclude <mach-anyka/fha_asa.h>
#include <mach-anyka/nand_list.h>
#include <mach-anyka/partition_init.h> 
#include <mach-anyka/partition_lib.h> 
#include <mach-anyka/ak_partition_table.h> 
#include <mach-anyka/spinand_badblock.h> 
#include <mach-anyka/anyka_types.h>
#include <linux/mm.h>


//#define SPINAND_DEBUG  
      
#ifdef SPINAND_DEBUG
#define PDEBUG(fmt, args...) printk( KERN_ALERT fmt,## args)
#define DEBUG(n, args...) printk(KERN_ALERT args)
#else
#define PDEBUG(fmt, args...)
#define DEBUG(n, args...) 
#endif


#define UPDATE_NULL  ""       // NO UPDATA
#define UPDATE_ING   "U_ING"  //UPDATE ING
#define UPDATE_END   "U_END"  //UPDATE FINISH
#define UPDATE_ERR   "U_ERR"  //UPDATE ERROR, is not to update




#define FHA_SUCCESS  				0
#define FHA_FAIL     				-1



#define FEATURE_ECC_EN  (1<<4)  //(1<<5)



//#define FLASH_BUF_SIZE			(32*1024)
#define FLASH_BUF_SIZE			(flash->info.page_size)

#define SPI_FLASH_READ		1
#define SPI_FLASH_WRITE		2
#define BAD_BLOCK_MARK_OFFSET 4
//#define CONFIG_SPINAND_USE_FAST_READ 1

/*mtd layer allocate memory use for 'vmalloc' interface, need to convert.*/
//#define SPINAND_USE_MTD_BLOCK_LAYER  


#define OPCODE_RESET						0xff   
#define OPCODE_WREN							0x06    /* Write Enable */ 
#define OPCODE_WRDI							0x04    /* Write Disable */ 

#define OPCODE_RDSR1    				0x0f    /* Read Status Register1 */
#define OPCODE_RDSR2     				0x35    /* Read Status Register2 */ 
#define OPCODE_RDSR3     				0x15    /* Read Status Register3 */ 
#define OPCODE_WRSR1          	0x1f    /* Write Status Register */ 
#define OPCODE_WRSR2          	0x31    /* Write Status2 Register*/ 
#define OPCODE_WRSR3          	0x11    /* Write Status3 Register*/ 

#define OPCODE_READ_TO_CACHE  	0x13
#define OPCODE_NORM_READ     		0x03    /* Read Data Bytes */ 
#define OPCODE_FAST_READ      	0x0b    /* Read Data Bytes at Higher Speed */ 
#define OPCODE_FAST_D_READ     	0x3b    /* Read Data Bytes at Dual output */ 
#define OPCODE_FAST_Q_READ     	0x6b    /* Read Data Bytes at Quad output */ 
#define OPCODE_FAST_D_IO     		0xbb    /* Read Data Bytes at Dual i/o */ 
#define OPCODE_FAST_Q_IO     		0xeb    /* Read Data Bytes at Quad i/o */ 

#define OPCODE_PP            		0x02    /* Page Program */
#define OPCODE_PP_DUAL					0x12		/* Dual Page Program*/
#define OPCODE_PP_QUAD					0x32		/* Quad Page Program*/
#define OPCODE_2IO_PP						0x18		/* 2I/O Page Program (tmp)*/
#define OPCODE_4IO_PP						0x38		/* 4I/O Page Program*/
#define OPCODE_PP_EXEC 					0x10

#define OPCODE_BE_4K						0x20    /* Sector (4K) Erase */ 
#define OPCODE_BE_32K       		0x52    /* Block (32K) Erase */
#define OPCODE_BE_64K          	0xd8    /* Block (64K) Erase */ 
#define	OPCODE_SE								0xd8		/* Sector erase (usually 64KiB) */
#define OPCODE_ERASE_BLOCK 			0xd8    /* block Erase */ 
#define	OPCODE_RDID							0x9f		/* Read JEDEC ID */
#define OPCODE_DP         	  	0xb9    /* Deep Power-down */ 
#define OPCODE_RES         		 	0xab    /* Release from DP, and Read Signature */ 


#define SPI_STATUS_REG1	1
#define SPI_STATUS_REG2	2


/* Define max times to check status register before we give up. */
#define	MAX_READY_WAIT_JIFFIES	(40 * HZ)	/* 40s max chip erase */

#define	CMD_SIZE		(1)
#define ADDR_SIZE		(2)
#define CMD_ADDR_SIZE	(CMD_SIZE + ADDR_SIZE)
#define MAX_DUMMY_SIZE	(4)

#define MTD_PART_NAME_LEN (4)

#ifdef CONFIG_SPINAND_USE_FAST_READ
#define OPCODE_READ 	OPCODE_FAST_READ
#define FAST_READ_DUMMY_BYTE 1
#else
#define OPCODE_READ 	OPCODE_NORM_READ
#define FAST_READ_DUMMY_BYTE 0
#endif

#define SPINAND_OOB_LEN(info) 	\
	((info.oob_up_skiplen + info.oob_seglen + info.oob_down_skiplen)*info.oob_seg_perpage)

#define ALIGN_DOWN(a, b)  (((a) / (b)) * (b))

#define SPINAND_BIN_PAGE_START 	(62)
/****************************************************************************/
struct partitions
{
	char name[MTD_PART_NAME_LEN]; 		   
	unsigned long long size;
	unsigned long long offset;         
	unsigned int mask_flags;
}__attribute__((packed));

typedef struct
{
    T_U32 BinPageStart; /*bin data start addr*/
    T_U32 PageSize;     /*spi page size*/
    T_U32 PagesPerBlock;/*page per block*/
    T_U32 BinInfoStart;
    T_U32 FSPartStart;
}
T_SPI_BURN_INIT_INFO;



T_PARTITION_INFO g_partition_info = {0};
T_PARTITION_TABLE_CONFIG g_part_tab;
static unsigned long kernel_start_page = 0;
static unsigned long kernel_end_page = 0;

static unsigned long rtf_start_page = 0;
static unsigned long rtf_end_page = 0;


int ak_check_error_to_update_flag_and_reboot(unsigned long page);




/*
 * SPI device driver setup and teardown
 */
struct flash_info {
	char		*name;

	/* JEDEC id zero means "no ID" (most older chips); otherwise it has
	 * a high byte of zero plus three data bytes: the manufacturer id,
	 * then a two byte device id.
	 */
	u32			jedec_id;
	u16			ext_id;

	u32 		page_size;
	u32 		page_per_block;

	/* The size listed here is what works with OPCODE_SE, which isn't
	 * necessarily called a "sector" by the vendor.
	 */
	unsigned	block_size;
	u16			n_blocks;

	/*|--------------------64bytes------------------------------|*/
	/*|---12---|-4--|---12----|-4--|---12---|-4--|---12----|-4--|*/
	/*|-seglen-|skip|-segllen-|skip|-seglen-|skip|-segllen-|skip|*/
	u32 		oob_size;        
	u16 		oob_up_skiplen;
	u16 		oob_seglen;
	u16 		oob_down_skiplen;	
	u16 		oob_seg_perpage;
	u16			oob_vail_data_offset;

	/**
	 *  chip character bits:
	 *  bit 0: under_protect flag, the serial flash under protection or not when power on
	 *  bit 1: fast read flag, the serial flash support fast read or not(command 0Bh)
	 *  bit 2: AAI flag, the serial flash support auto address increment word programming
	 *  bit 3: support dual write or no
	 *  bit 4: support dual read or no
	 *  bit 5: support quad write or no
	 *  bit 6: support quad read or no
	 *  bit 7: the second status command (35h) flag,if use 4-wire(quad) mode,the bit must be is enable
	 */
	u16			flags;
#define	SFLAG_UNDER_PROTECT			(1<<0)
#define SFLAG_FAST_READ           	(1<<1)
#define SFLAG_AAAI                	(1<<2)
#define SFLAG_COM_STATUS2         	(1<<3)

#define SFLAG_DUAL_IO_READ         	(1<<4)
#define SFLAG_DUAL_READ           	(1<<5)
#define SFLAG_QUAD_IO_READ         	(1<<6)
#define SFLAG_QUAD_READ           	(1<<7)

#define SFLAG_DUAL_IO_WRITE        	(1<<8)
#define SFLAG_DUAL_WRITE          	(1<<9)
#define SFLAG_QUAD_IO_WRITE        	(1<<10)
#define SFLAG_QUAD_WRITE          	(1<<11)

#define SFLAG_SECT_4K       		(1<<12)

};

/**
  *because of some spi nand is difference of status register difinition.
  *this structure use mapping the status reg function and corresponding.
*/
struct flash_status_reg
{
	u32		jedec_id;	
	u16		ext_id;
	unsigned b_wip:4;		/*write in progress*/
	unsigned b_wel:4;		/*wrute ebabke latch*/
	unsigned b_bp0:4;		/*block protected 0*/
	unsigned b_bp1:4;		/*block protected 1*/
	unsigned b_bp2:4;		/*block protected 2*/
	unsigned b_bp3:4;		/*block protected 3*/
	unsigned b_bp4:4;		/*block protected 4*/
	unsigned b_srp0:4;		/*status register protect 0*/
	
	unsigned b_srp1:4;		/*status register protect 1*/
	unsigned b_qe:4;		/*quad enable*/
	unsigned b_lb:4;		/*write protect control and status to the security reg.*/
/*
	unsigned b_reserved0:4;
	unsigned b_reserved1:4;
	unsigned b_reserved2:4;
*/
	unsigned b_cmp:4;		/*conjunction bp0-bp4 bit*/
	unsigned b_sus:4;		/*exec an erase/program suspend cmd_buf*/
	unsigned b_efail:4;		/**/
	unsigned b_pfail:4;		/**/
};

struct ak_spinand {
	struct spi_device	*spi;
	struct mutex		lock;
	struct flash_info	info;
	struct mtd_info		mtd;
	unsigned			partitioned:1;
	
	u8		bus_width;
	unsigned char 		*buf;
	u8		cmd_buf[CMD_ADDR_SIZE + MAX_DUMMY_SIZE];
	u8		dummy_len;

	u8		erase_opcode;
	u8		tx_opcode;
	u8		rx_opcode;
	u8		txd_bus_width;
	u8		rxd_bus_width;
	
	u8		txa_bus_width;
	u8		rxa_bus_width;	
	u32 	page_shift;
	struct flash_status_reg stat_reg;
};

static struct mtd_info *ak_mtd_info;


//extern char __initdata saved_root_name[64];

/*
 * feature cmd list ,reference by spec.
 * */
static int feature_cmd[3] = {0xC0, 0xB0, 0xA0};

static inline struct ak_spinand *mtd_to_spiflash(struct mtd_info *mtd)
{
	return container_of(mtd, struct ak_spinand, mtd);
}


static 	struct flash_status_reg __devinitdata  status_reg_list[] = {
		/*normal status reg define*/
		{
			.jedec_id = 0xc8f4c8f4,	.ext_id = 0,
			.b_wip = 0,	.b_wel = 1,	.b_efail= 2,	.b_pfail= 3,
			.b_bp2 = 4,	.b_bp3 = 5,	.b_bp4 = 6,	.b_srp0 = 7,

			.b_qe = 8,	.b_srp1 = 9,.b_lb = 10,	.b_cmp = 14,
			.b_sus = 15,
		},
		/*normal status reg define*/
		{
			.jedec_id = 0xc8f2c8f2,	.ext_id = 0,
			.b_wip = 0,	.b_wel = 1,	.b_efail= 2,	.b_pfail= 3,
			.b_bp2 = 4,	.b_bp3 = 5,	.b_bp4 = 6,	.b_srp0 = 7,

			.b_qe = 8,	.b_srp1 = 9,.b_lb = 10,	.b_cmp = 14,
			.b_sus = 15,
		},
		/*normal status reg define*/
		{
			.jedec_id = 0xc8f1c8f1,	.ext_id = 0,
			.b_wip = 0,	.b_wel = 1,	.b_efail= 2,	.b_pfail= 3,
			.b_bp2 = 4,	.b_bp3 = 5,	.b_bp4 = 6,	.b_srp0 = 7,

			.b_qe = 8,	.b_srp1 = 9,.b_lb = 10,	.b_cmp = 14,
			.b_sus = 15,
		},
		/*normal status reg define*/
		{
			.jedec_id = 0xc8d1c8d1,	.ext_id = 0,
			.b_wip = 0,	.b_wel = 1,	.b_efail= 2,	.b_pfail= 3,
			.b_bp2 = 4,	.b_bp3 = 5,	.b_bp4 = 6,	.b_srp0 = 7,

			.b_qe = 8,	.b_srp1 = 9,.b_lb = 10,	.b_cmp = 14,
			.b_sus = 15,
		},	
		/*normal status reg define*/
		{
			.jedec_id = 0xc8d2c8d2,	.ext_id = 0,
			.b_wip = 0,	.b_wel = 1,	.b_efail= 2,	.b_pfail= 3,
			.b_bp2 = 4,	.b_bp3 = 5,	.b_bp4 = 6,	.b_srp0 = 7,

			.b_qe = 8,	.b_srp1 = 9,.b_lb = 10,	.b_cmp = 14,
			.b_sus = 15,
		},				
		/*normal status reg define*/
		{
			.jedec_id = 0xb148c8b1,	.ext_id = 0,
			.b_wip = 0,	.b_wel = 1,	.b_efail= 2,	.b_pfail= 3,
			.b_bp2 = 4,	.b_bp3 = 5,	.b_bp4 = 6,	.b_srp0 = 7,

			.b_qe = 8,	.b_srp1 = 9,.b_lb = 10,	.b_cmp = 14,
			.b_sus = 15,
		},
		/*normal status reg define*/
		{
			.jedec_id = 0xc8217f7f,	.ext_id = 0,
			.b_wip = 0,	.b_wel = 1,	.b_efail= 2,	.b_pfail= 3,
			.b_bp2 = 4,	.b_bp3 = 5,	.b_bp4 = 6,	.b_srp0 = 7,

			.b_qe = 8,	.b_srp1 = 9,.b_lb = 10,	.b_cmp = 14,
			.b_sus = 15,
		},
		/*normal status reg define*/
		{
			.jedec_id = 0xc831C831,	.ext_id = 0,
			.b_wip = 0,	.b_wel = 1,	.b_efail= 2,	.b_pfail= 3,
			.b_bp2 = 4,	.b_bp3 = 5,	.b_bp4 = 6,	.b_srp0 = 7,

			.b_qe = 8,	.b_srp1 = 9,.b_lb = 10,	.b_cmp = 14,
			.b_sus = 15,
		},	
		/*normal status reg define*/
		{
			.jedec_id = 0xa1e1a1e1,	.ext_id = 0,
			.b_wip = 0,	.b_wel = 1,	.b_efail= 2,	.b_pfail= 3,
			.b_bp2 = 4,	.b_bp3 = 5,	.b_bp4 = 6,	.b_srp0 = 7,

			.b_qe = 8,	.b_srp1 = 9,.b_lb = 10,	.b_cmp = 14,
			.b_sus = 15,
		},
		{
			.jedec_id = 0xc212c212,	.ext_id = 0,
			.b_wip = 0,	.b_wel = 1,	.b_efail= 2,	.b_pfail= 3,
			.b_bp2 = 4,	.b_bp3 = 5,	.b_bp4 = 6,	.b_srp0 = 7,

			.b_qe = 8,	.b_srp1 = 9,.b_lb = 10,	.b_cmp = 14,
			.b_sus = 15,
		},		
		{
			.jedec_id = 0xefaa2100,	.ext_id = 0,
			.b_wip = 0,	.b_wel = 1,	.b_efail= 2,	.b_pfail= 3,
			.b_bp2 = 4,	.b_bp3 = 5,	.b_bp4 = 6,	.b_srp0 = 7,

			.b_qe = 8,	.b_srp1 = 9,.b_lb = 10,	.b_cmp = 14,
			.b_sus = 15,
		},	
		/*normal status reg define*/
		{
			.jedec_id = 0,	.ext_id = 0,
			.b_wip = 0,	.b_wel = 1,	.b_bp0 = 2,	.b_bp1 = 3,
			.b_bp2 = 4,	.b_bp3 = 5,	.b_bp4 = 6,	.b_srp0 = 7,
			
			.b_srp1 = 8,.b_qe = 9,	.b_lb = 10,	.b_cmp = 14,
			.b_sus = 15,
		},
};


/* NOTE: double check command sets and memory organization when you add
 * more flash chips.  This current list focusses on newer chips, which
 * have been converging on command sets which including JEDEC ID.
 */
static struct flash_info __devinitdata ak_spinand_supportlist [] = {
  { "GD5G1GQ4U", 0xc8f1c8f1, 0, 2048, 64, 128*1024, 1024, 64, 0, 12, 4, 4, 4, SFLAG_DUAL_READ|SFLAG_QUAD_READ|SFLAG_QUAD_WRITE,},
  { "GD5F1GQ4UB", 0xc8d1c8d1, 0, 2048, 64, 128*1024, 1024, 64, 0, 12, 4, 4, 4, SFLAG_DUAL_READ|SFLAG_QUAD_READ|SFLAG_QUAD_WRITE,},  
	{ "GD5G2GQ4U", 0xc8f2c8f2, 0, 2048, 64, 128*1024, 2048, 64, 0, 12, 4, 4, 4, SFLAG_DUAL_READ|SFLAG_QUAD_READ|SFLAG_QUAD_WRITE,},
  { "GD5F2GQ4UB", 0xc8d2c8d2, 0, 2048, 64, 128*1024, 2048, 64, 0, 16, 0, 4, 4, SFLAG_DUAL_READ|SFLAG_QUAD_READ|SFLAG_QUAD_WRITE,},
	{ "GD5G4GQ4U", 0xc8f4c8f4, 0, 2048, 64, 128*1024, 4096, 64, 0, 12, 4, 4, 4, SFLAG_DUAL_READ|SFLAG_QUAD_READ|SFLAG_QUAD_WRITE,},	
	{ "PSU1GQ4U", 0xc8217f7f, 0, 2048, 64, 128*1024, 1024, 64, 8, 8, 0, 4, 8, SFLAG_DUAL_READ|SFLAG_QUAD_READ|SFLAG_QUAD_WRITE,},
	{ "AFS1GQ4UAAWC2", 0xc831C831, 0, 2048, 128, 256*1024, 512, 64, 0, 6, 10, 4, 4, SFLAG_DUAL_READ|SFLAG_QUAD_READ|SFLAG_QUAD_WRITE,},	
  { "PN26G01AWSIUG", 0xa1e1a1e1, 0, 2048, 64, 128*1024, 1024, 64, 0, 8, 0, 4, 6, SFLAG_DUAL_READ|SFLAG_QUAD_READ|SFLAG_QUAD_WRITE,},
  	{"MX35LF1GE4AB", 0xc212c212, 0, 2048, 64, 128*1024, 1024, 64, 0, 16, 0, 4, 4, SFLAG_DUAL_READ|SFLAG_QUAD_READ|SFLAG_QUAD_WRITE,},
  	{ "W25N01GV", 0xefaa2100, 0, 2048, 64, 128*1024, 1024, 64, 0, 4, 12, 4, 4, SFLAG_DUAL_READ|SFLAG_QUAD_READ|SFLAG_QUAD_WRITE,},
};


#ifdef SPINAND_USE_MTD_BLOCK_LAYER
/**
* @brief: because of the _read() function call by mtd block layer, the buffer be
* allocate by vmalloc() in mtd layer, spi driver layer may use this buffer that 
* intents of use for DMA transfer, so, add this function to transition buffer.
* call this function at before real read/write data.
* 
* @author lixinhai
* @date 2013-04-10
* @param[in] flash  spiflash handle.
* @param[in] buffer.
* @param[in] buffer len
* @param[in] read/write
* @retval return the transition buffer
*/
static void *flash_buf_bounce_pre(struct ak_spinand *flash,
				void *buf, u32 len, int dir)
{
	if(!is_vmalloc_addr(buf)) {
		return buf;
	}

	if(dir == SPI_FLASH_WRITE) {
		memcpy(flash->buf, buf, len);
	}
	return flash->buf;
}

/**
* @brief: because of the _read() function call by mtd block layer, the buffer be
* allocate by vmalloc() in mtd layer, spi driver layer may use this buffer that 
* intents of use for DMA transfer, so, add this function to transition buffer.
* call this function at after real read/write data
* 
* @author lixinhai
* @date 2013-04-10
* @param[in] flash  spiflash handle.
* @param[in] buffer.
* @param[in] buffer len
* @param[in] read/write
* @retval return the transition buffer
*/
static void flash_buf_bounce_post(struct ak_spinand *flash,
				void *buf, u32 len, int dir)
{
	if(!is_vmalloc_addr(buf)) {
		return;
	}

	if(dir == SPI_FLASH_READ) {
		memcpy(buf, flash->buf, len);
	}
}
#else
static inline void *flash_buf_bounce_pre(struct ak_spinand *flash,
				void *buf, u32 len, int dir)
{
	return buf;
}

static inline void flash_buf_bounce_post(struct ak_spinand *flash,
				void *buf, u32 len, int dir)
{
}
#endif

/*
 * Internal helper functions
 */

/**
* @brief Read the status register.
* 
*  returning its value in the location
* @author lixinhai
* @date 2014-03-20
* @param[in] spiflash handle.
* @return int Return the status register value.
*/
static int read_sr(struct ak_spinand *flash, u32 addr)
{

	u8 			st_tmp= 0;
	int			status;
	struct spi_message	message;
	struct spi_transfer	x[2];

	spi_message_init(&message);
	memset(x, 0, sizeof x);
	
	flash->cmd_buf[0]= OPCODE_RDSR1;
	flash->cmd_buf[1]= addr;	
	
	x[0].tx_buf = flash->cmd_buf;
	x[0].len = 2;
	spi_message_add_tail(&x[0], &message);


	x[1].rx_buf = flash->cmd_buf + 2;
	x[1].len = 1;
	spi_message_add_tail(&x[1], &message);

	/* do the i/o */
	status = spi_sync(flash->spi, &message);
	if (status == 0)
		memcpy(&st_tmp, x[1].rx_buf, 1);

	return st_tmp;	
	
	/*
	ssize_t retval;
	u32 status;
	u8 st_tmp= 0;
//printk("%s\n",__func__);
	flash->cmd_buf[0]= OPCODE_RDSR1;
	flash->cmd_buf[1]= addr;

	if((retval = spi_write_then_read(flash->spi, flash->cmd_buf, 2, &st_tmp, 1))<0)
		return retval;

	status = st_tmp;
	return status;
	*/
}


/**
* @brief Write status register
* 
*  Write status register 1 byte.
* @author lixinhai
* @date 2014-03-20
* @param[in] flash  spiflash handle.
* @param[in] val  register value to be write.
* @return int return write success or failed
* @retval returns zero on success
* @retval return a negative error code if failed
*/
static int write_sr(struct ak_spinand *flash, u32 addr, u16 val)
{
	struct spi_transfer t[1];
	struct spi_message m;
	spi_message_init(&m);
	memset(t, 0, (sizeof t));
		
	flash->cmd_buf[0] = OPCODE_WRSR1;
	flash->cmd_buf[1] = addr;
	flash->cmd_buf[2] = val;
	
	t[0].tx_buf = flash->cmd_buf;
	t[0].len = 3;	
	spi_message_add_tail(&t[0], &m);
	
	return spi_sync(flash->spi, &m);
}

static inline int sflash_reset(struct ak_spinand *flash)
{
	u8	code = OPCODE_RESET;
	int ret;

	ret = spi_write_then_read(flash->spi, &code, 1, NULL, 0);
	ret |= write_sr(flash, 0xa0, 0x0);
	return ret;
}

/**
* @brief Set write enable latch.
* 
*  Set write enable latch with Write Enable command.
* @author lixinhai
* @date 2014-03-20
* @param[in] flash  spiflash handle.
* @return int return write success or failed
* @retval returns zero on success
* @retval return a negative error code if failed
*/
static inline int write_enable(struct ak_spinand *flash)
{
	
//	u8 			st_tmp= 0;
	int			status;
	struct spi_message	message;
	struct spi_transfer	x[2];

	spi_message_init(&message);
	memset(x, 0, sizeof x);
	
	flash->cmd_buf[0]= OPCODE_WREN;

	
	x[0].tx_buf = flash->cmd_buf;
	x[0].len = 1;
	spi_message_add_tail(&x[0], &message);

	/* do the i/o */
	status = spi_sync(flash->spi, &message);


	return status;		

/*	
	u8	code = OPCODE_WREN;
	
//printk("%s\n",__func__);
	return spi_write_then_read(flash->spi, &code, 1, NULL, 0);
*/
}


/**
* @brief Set write disble
* 
*  Set write disble instruction to the chip.
* @author lixinhai
* @date 2014-03-20
* @param[in] flash	spiflash handle.
* @return int return write success or failed
* @retval returns zero on success
* @retval return a negative error code if failed
*/
static inline int write_disable(struct ak_spinand *flash)
{
	u8	code = OPCODE_WRDI;

	return spi_write_then_read(flash->spi, &code, 1, NULL, 0);
}

/**
* @brief  Wait for SPI flash ready.
* 
*  Service routine to read status register until ready, or timeout occurs.
* @author lixinhai
* @date 2014-03-20
* @param[in] flash	spiflash handle.
* @return int return write success or failed
* @retval returns zero on success
* @retval return a non-zero error code if failed
*/
static int wait_till_ready(struct ak_spinand *flash)
{
	unsigned long deadline;
	int idx, shift;
	u32 sr;
	u8 addr;
	struct flash_status_reg *fsr = &flash->stat_reg;

	deadline = jiffies + MAX_READY_WAIT_JIFFIES;

	shift = fsr->b_wip / 8;
	idx = fsr->b_wip % 8;
	addr = feature_cmd[shift];
	do {
		if ((sr = read_sr(flash, addr)) < 0)
			break;
		else if (!(sr & (1<<(fsr->b_wip%8))))
			return 0;

		cond_resched();

	} while (!time_after_eq(jiffies, deadline));

	return 1;
}


static int check_ecc_status(struct ak_spinand *flash)
{
	unsigned long deadline;
	int idx, shift;
	u32 sr;
	u8 addr;
	struct flash_status_reg *fsr = &flash->stat_reg;

	deadline = jiffies + MAX_READY_WAIT_JIFFIES;

	shift = fsr->b_wip / 8;
	idx = fsr->b_wip % 8;
	addr = feature_cmd[shift];
	do {
		if ((sr = read_sr(flash, addr)) < 0)
		{
			printk(KERN_ERR "read_sr fail\n" );
			break;
		}

		if(((sr >> 4) & 0x3) == 2)
		{
			printk(KERN_ERR "ecc error sr:%d\n", sr );
			return 1;
		}
		else
		{
			return 0;
		}

		cond_resched();

	} while (!time_after_eq(jiffies, deadline));

	return 1;

}



/**
* @brief: enable 4 wire transfer mode.
* 
* @author lixinhai
* @date 2014-04-10
* @param[in] flash  spiflash handle.
*/
static int quad_mode_enable(struct ak_spinand *flash)
{
	int ret, idx, shift;
	u32 regval;
	u8 addr;
	struct flash_status_reg *fsr = &flash->stat_reg;

	shift = fsr->b_qe / 8;
	idx = fsr->b_qe % 8;
	
	addr = feature_cmd[shift];
	ret = wait_till_ready(flash);
	if (ret)
		return -EBUSY;

	write_enable(flash);
	
	regval = read_sr(flash, addr);
	regval |= 1<<(fsr->b_qe % 8);
	write_sr(flash, addr, regval);


	regval = read_sr(flash, addr);

	write_disable(flash);
	return 0;
}

/**
* @brief: disable 4 wire transfer mode.
* 
* @author lixinhai
* @date 2014-04-10
* @param[in] flash  spiflash handle.
*/
static int quad_mode_disable(struct ak_spinand *flash)
{
	int ret, idx, shift;
	u32 regval;
	u8 addr;
	struct flash_status_reg *fsr = &flash->stat_reg;

	shift = fsr->b_qe / 8;
	idx = fsr->b_qe % 8; 
	addr = feature_cmd[shift];
	ret = wait_till_ready(flash);
	if (ret)
		return -EBUSY;
	
	write_enable(flash);
	
	regval = read_sr(flash, addr);
	regval &= ~(1<<(fsr->b_qe%8));
	write_sr(flash, addr, regval);


	regval = read_sr(flash, addr);
	write_disable(flash);
	return 0;
}


/**
* @brief  Erase sector
* 
*  Erase a sector specialed by user.
* @author lixinhai
* @date 2014-03-20
* @param[in] flash	    spiflash handle.
* @param[in] offset    which is any address within the sector which should be erased.
* @return int return write success or failed
* @retval returns zero on success
* @retval return a non-zero error code if failed
*/
static int erase_block(struct ak_spinand *flash, u32 offset)
{
	u32 row;	
	struct spi_transfer t[1];
	struct spi_message m;
	
	spi_message_init(&m);
	memset(t, 0, (sizeof t));
		
	DEBUG(MTD_DEBUG_LEVEL3, "%s: %s %dKiB at 0x%08x\n",
			dev_name(&flash->spi->dev), __func__,
			flash->mtd.erasesize / 1024, offset);

	row = ((offset>>flash->page_shift) & 0xffffff);

	/* Wait until finished previous write command. */
	if (wait_till_ready(flash))
	{
		printk(KERN_ERR "kernel: error erase_block previous write command fail\n" );
		return -EBUSY;
	}

	/* Send write enable, then erase commands. */
	write_enable(flash);

	/* Set up command buffer. */
	flash->cmd_buf[0] = flash->erase_opcode;
	flash->cmd_buf[1] = row >> 16;
	flash->cmd_buf[2] = row >> 8;
	flash->cmd_buf[3] = row;

	t[0].tx_buf = flash->cmd_buf;
	t[0].len = 4;	
	spi_message_add_tail(&t[0], &m);
	
  spi_sync(flash->spi, &m);


//	spi_write(flash->spi, flash->cmd_buf, 4);

	if (wait_till_ready(flash)) {
		printk(KERN_ERR "kernel: error erase_block write command fail\n" );
		/* REVISIT status return?? */		
		return -EBUSY;
	}

	return 0;
}


/**
* @brief  MTD Erase
* 
* Erase an address range on the flash chip.
* @author luoyongchuang
* @date 2015-05-17
* @param[in] mtd    mtd info handle.
* @param[in] instr   erase info.
* @return int return write success or failed
* @retval returns zero on success
* @retval return a non-zero error code if failed
*/
static int ak_spinand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct ak_spinand *flash = mtd_to_spiflash(mtd);
	u32 addr,len;
	uint32_t rem;


	DEBUG(MTD_DEBUG_LEVEL2, "%s: %s %s 0x%llx, len %lld\n",
	      dev_name(&flash->spi->dev), __func__, "at",
	      (long long)instr->addr, (long long)instr->len);

	/* sanity checks */
	if (instr->addr + instr->len > mtd->size)
	{
		printk(KERN_ERR "ak_spinand_erase:instr->addr[0x%llx] + instr->len[%lld] > mtd->size[%lld]\n",
			instr->addr, instr->len, mtd->size );
		return -EINVAL;
	}
	div_u64_rem(instr->len, mtd->erasesize, &rem);
	if (rem != 0)
	{
		printk(KERN_ERR "ak_spinand_erase:rem!=0 [%u]\n", rem );
		return -EINVAL;
	}

	addr = instr->addr;
	len = instr->len;

	mutex_lock(&flash->lock);

	//printk("ak_spinand_erase:%d:%d\n", addr, len);
	while (len) {
		if (erase_block(flash, addr)) {
			instr->state = MTD_ERASE_FAILED;
			mutex_unlock(&flash->lock);
			return -EIO;
		}

		addr += mtd->erasesize;
		len -= mtd->erasesize;
	}

	mutex_unlock(&flash->lock);

	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);

	return 0;
}

/**
* @brief initilize spi nand flash read/write param. 
* 
* @author lixinhai
* @date 2014-04-20
* @param[in] spiflash info handle.
* @return int return config success or failed
* @retval returns zero on success
* @retval return a non-zero error code if failed
*/
static int init_spiflash_rw_info(struct ak_spinand *flash)
{
	/**default param.*/
	flash->rx_opcode = OPCODE_READ;
	flash->rxd_bus_width = XFER_1DATAWIRE;
	flash->rxa_bus_width = XFER_1DATAWIRE;
	flash->tx_opcode = OPCODE_PP;
	flash->txd_bus_width = XFER_1DATAWIRE;
	flash->txa_bus_width = XFER_1DATAWIRE;
	flash->dummy_len = 1;

	if(flash->bus_width & FLASH_BUS_WIDTH_2WIRE){
		if(flash->info.flags & SFLAG_DUAL_READ) {
			flash->rx_opcode = OPCODE_FAST_D_READ;
			flash->rxd_bus_width = XFER_2DATAWIRE;
			flash->rxa_bus_width = XFER_1DATAWIRE;
			flash->dummy_len = 1;
		} else if (flash->info.flags & SFLAG_DUAL_IO_READ) {
			flash->rx_opcode = OPCODE_FAST_D_IO;
			flash->rxd_bus_width = XFER_2DATAWIRE;
			flash->rxa_bus_width = XFER_2DATAWIRE;
			flash->dummy_len = 1;
		}

		if(flash->info.flags & SFLAG_DUAL_WRITE) {
			flash->tx_opcode = OPCODE_PP_DUAL;
			flash->txd_bus_width = XFER_2DATAWIRE;
			flash->txa_bus_width = XFER_1DATAWIRE;
		} else if(flash->info.flags & SFLAG_DUAL_IO_WRITE) {
			flash->tx_opcode = OPCODE_2IO_PP;
			flash->txd_bus_width = XFER_2DATAWIRE;
			flash->txa_bus_width = XFER_2DATAWIRE;
		}	
	}

	if(flash->bus_width & FLASH_BUS_WIDTH_4WIRE){
		if(flash->info.flags & SFLAG_QUAD_READ) {
			flash->rx_opcode = OPCODE_FAST_Q_READ;
			flash->rxd_bus_width = XFER_4DATAWIRE;
			flash->rxa_bus_width = XFER_1DATAWIRE;
			flash->dummy_len = 1;
		}else if(flash->info.flags & SFLAG_QUAD_IO_READ){
			flash->rx_opcode = OPCODE_FAST_Q_IO;
			flash->rxd_bus_width = XFER_4DATAWIRE;
			flash->rxa_bus_width = XFER_4DATAWIRE;
			flash->dummy_len = 3;
		}

		if(flash->info.flags & SFLAG_QUAD_WRITE) {
			flash->tx_opcode = OPCODE_PP_QUAD;
			flash->txd_bus_width = XFER_4DATAWIRE;			
			flash->txa_bus_width = XFER_1DATAWIRE;
		}else if(flash->info.flags & SFLAG_QUAD_IO_WRITE) {
			flash->tx_opcode = OPCODE_4IO_PP;
			flash->txd_bus_width = XFER_4DATAWIRE;
			flash->txa_bus_width = XFER_4DATAWIRE;
		}
	
	}
	return 0;
}


#if 0

bool ak_spinand_get_protect(void)
{
	struct ak_spinand *flash;
	u32 regval; 

	if(ak_mtd_info == NULL) 
		return false;
		
	flash = mtd_to_spiflash(ak_mtd_info);
	if(flash == NULL) 
		return false;
		
	mutex_lock(&flash->lock);
	regval = read_sr(flash, 0xA0);
	mutex_unlock(&flash->lock);

    if((regval & (0x7<<3)) == 0)
        return false;
    else
        return true;
}



//0: none, 1:1/64, 2:1/32, 3:1/16, 4:1/8, 5:1/4, 6:1/2
int ak_spinand_cfg_protect(bool protect)
{
	struct ak_spinand *flash;
	const unsigned char protect_bit[7] = {0, 0x0C, 0x14, 0x1C, 0x24, 0x2C, 0x34};
    const unsigned char mode = 6;
	u8 addr = 0xA0;
	u32 regval; 

	if(ak_mtd_info == NULL) 
		return -1;

	flash = mtd_to_spiflash(ak_mtd_info);
	if(flash == NULL) 
		return -1;
		
	mutex_lock(&flash->lock);

	if(protect)
    	regval = protect_bit[mode];
	else
    	regval = protect_bit[0];
	
	if(wait_till_ready(flash)) {
        mutex_unlock(&flash->lock);
		return -EBUSY;
    }
    
	write_enable(flash);
	write_sr(flash, addr, regval);
	regval = read_sr(flash, addr);
	printk(KERN_ERR "[protect regval: %x]\n", regval);
	write_disable(flash);

	mutex_unlock(&flash->lock);
	
	return 0;		
}
#endif


/**
* @brief configure spi nandflash transfer mode according to flags. 
* 
* @author lixinhai
* @date 2014-04-20
* @param[in] spiflash info handle.
* @return int return config success or failed
* @retval returns zero on success
* @retval return a non-zero error code if failed
*/
static int ak_spinand_cfg_quad_mode(struct ak_spinand *flash)
{
	int ret = 0;
	
	if((flash->bus_width & FLASH_BUS_WIDTH_4WIRE) && 
		(flash->info.flags & (SFLAG_QUAD_WRITE|SFLAG_QUAD_IO_WRITE|
			SFLAG_DUAL_READ|SFLAG_DUAL_IO_READ))) {		
		ret = quad_mode_enable(flash);
		if(ret) {
			flash->bus_width &= ~FLASH_BUS_WIDTH_4WIRE;
			printk("config the spiflash quad enable fail. transfer use 1 wire.\n");
		}
	}
	else
		quad_mode_disable(flash);

	return ret;
}


#define FILL_CMD(c, val) do{c[0] = (val);}while(0)
#define FILL_ADDR(c, val) do{	\
		c[CMD_SIZE] = ((val) >> 8) & 0xf;	\
		c[CMD_SIZE+1] = ((val) & 0xff);	\
		}while(0)
		
#define FILL_DUMMY_DATA(c, val) do{	\
			c[CMD_ADDR_SIZE] = val >> 16;	\
			c[CMD_ADDR_SIZE+1] = 0;	\
			c[CMD_ADDR_SIZE+2] = 0;	\
			c[CMD_ADDR_SIZE+3] = 0;	\
			}while(0)

/**
* @brief configure spi nandflash transfer mode according to flags. 
* 
* @author lixinhai
* @date 2014-04-20
* @param[in] mtd info handle.
* @param[in] row address.
* @param[in] column address.
* @param[in] transfer len.
* @param[out] transfer result len.
* @param[out] result buffer.
* @return int return config success or failed
* @retval returns zero on success
* @retval return a non-zero error code if failed
*/
static int spinand_read(struct mtd_info *mtd, int row, int col, size_t len,
		size_t *retlen, u_char *buf)
{
	struct ak_spinand *flash = mtd_to_spiflash(mtd);
	struct spi_transfer t[4];
	struct spi_message m;
	struct spi_message m1;	
	void *bounce_buf;
  
  
	spi_message_init(&m);
	spi_message_init(&m1);	
	memset(t, 0, (sizeof t));
	
	mutex_lock(&flash->lock);
	bounce_buf = flash_buf_bounce_pre(flash, buf, len, SPI_FLASH_READ);

	/*fill command*/
	flash->cmd_buf[0] = OPCODE_READ_TO_CACHE;
	flash->cmd_buf[1] = (row >> 16) & 0xff;
	flash->cmd_buf[2] = (row >> 8) & 0xff;
	flash->cmd_buf[3] = row & 0xff;
	t[3].tx_buf = flash->cmd_buf;
	t[3].len = 4;	
	spi_message_add_tail(&t[3], &m1);
	spi_sync(flash->spi, &m1);
	
//	spi_write(flash->spi, flash->cmd_buf, 4);

	t[0].tx_buf = flash->cmd_buf;
	t[0].len = CMD_SIZE;
	spi_message_add_tail(&t[0], &m);

	t[1].tx_buf = &flash->cmd_buf[CMD_SIZE];
	t[1].len = ADDR_SIZE + flash->dummy_len;
	t[1].xfer_mode = flash->rxa_bus_width;
	spi_message_add_tail(&t[1], &m);

	t[2].rx_buf = bounce_buf;
	t[2].len = len;	
	t[2].cs_change = 1;	
	t[2].xfer_mode = flash->rxd_bus_width;

	spi_message_add_tail(&t[2], &m);

	/* Byte count starts at zero. */
	if (retlen)
		*retlen = 0;

	/* Wait till previous write/erase is done. */
	if (wait_till_ready(flash)) {
		/* REVISIT status return?? */		
		mutex_unlock(&flash->lock);
		printk(KERN_ERR"kernel: error spinand_read fail, row:%d\n", row);
		return -EBUSY;
	}

	/* Set up the write data buffer. */
	FILL_CMD(flash->cmd_buf, flash->rx_opcode);
	FILL_ADDR(flash->cmd_buf, col);
	FILL_DUMMY_DATA(flash->cmd_buf, 0x00);

	spi_sync(flash->spi, &m);

	*retlen = m.actual_length - CMD_ADDR_SIZE - flash->dummy_len;
	if (check_ecc_status(flash)) {
		/* REVISIT status return?? */		
		mutex_unlock(&flash->lock);
		printk(KERN_ERR"kernel: check_ecc_status, row:%d\n", row);
		return -EBUSY;
	}
	
	flash_buf_bounce_post(flash, buf, len, SPI_FLASH_READ);
	
	mutex_unlock(&flash->lock);

	return 0;
}

#if 0

static int ecc_ctl(struct ak_spinand *flash, bool enable)
{
	u8 addr = 0xB0;
	int regval; 
	
	if(wait_till_ready(flash)) {
		return -1;
	}
	
	write_enable(flash);

	regval = read_sr(flash, addr);

	if(enable)
        regval |= FEATURE_ECC_EN;
    else
        regval &= ~FEATURE_ECC_EN;
	
	write_sr(flash, addr, regval);
	//regval = read_sr(flash, addr);
	//printk(KERN_ERR "[ecc enable regval: %x]\n", regval);
	write_disable(flash);

	return 0;		
}


static int spinand_read_noecc(struct mtd_info *mtd, int row, int col, size_t len,
		size_t *retlen, u_char *buf)
{
	struct ak_spinand *flash = mtd_to_spiflash(mtd);
	struct spi_transfer t[4];
	struct spi_message m;
	struct spi_message m1;	
	void *bounce_buf;
	int ret = 0;
  
  
	spi_message_init(&m);
	spi_message_init(&m1);	
	memset(t, 0, (sizeof t));
	
	mutex_lock(&flash->lock);

	//set to no ecc
    if (-1 == ecc_ctl(flash, false))
    {
        printk(KERN_ERR"ecc_ctl fail\r\n");  
        mutex_unlock(&flash->lock);
        return -EBUSY;
    }

	
	bounce_buf = flash_buf_bounce_pre(flash, buf, len, SPI_FLASH_READ);

	/*fill command*/
	flash->cmd_buf[0] = OPCODE_READ_TO_CACHE;
	flash->cmd_buf[1] = (row >> 16) & 0xff;
	flash->cmd_buf[2] = (row >> 8) & 0xff;
	flash->cmd_buf[3] = row & 0xff;
	t[3].tx_buf = flash->cmd_buf;
	t[3].len = 4;	
	spi_message_add_tail(&t[3], &m1);
	spi_sync(flash->spi, &m1);
	
//	spi_write(flash->spi, flash->cmd_buf, 4);

	t[0].tx_buf = flash->cmd_buf;
	t[0].len = CMD_SIZE;
	spi_message_add_tail(&t[0], &m);

	t[1].tx_buf = &flash->cmd_buf[CMD_SIZE];
	t[1].len = ADDR_SIZE + flash->dummy_len;
	t[1].xfer_mode = flash->rxa_bus_width;
	spi_message_add_tail(&t[1], &m);

	t[2].rx_buf = bounce_buf;
	t[2].len = len; 
	t[2].cs_change = 1; 
	t[2].xfer_mode = flash->rxd_bus_width;

	spi_message_add_tail(&t[2], &m);

	/* Byte count starts at zero. */
	if (retlen)
		*retlen = 0;

	/* Wait till previous write/erase is done. */
	if (wait_till_ready(flash)) {
		/* REVISIT status return?? */		
		printk(KERN_ERR"kernel: error spinand_read fail\n");
		ret = -EBUSY;
		goto OUT;
	}

	/* Set up the write data buffer. */
	FILL_CMD(flash->cmd_buf, flash->rx_opcode);
	FILL_ADDR(flash->cmd_buf, col);
	FILL_DUMMY_DATA(flash->cmd_buf, 0x00);

	spi_sync(flash->spi, &m);

	*retlen = m.actual_length - CMD_ADDR_SIZE - flash->dummy_len;
	
	flash_buf_bounce_post(flash, buf, len, SPI_FLASH_READ);

OUT:
	if (-1 == ecc_ctl(flash, true))
    {
    	mutex_unlock(&flash->lock);
        printk(KERN_ERR"spi_nand_read_noecc ecc_ctl fail\r\n");
        return -EBUSY;
    }
	
	mutex_unlock(&flash->lock);

	return ret;
}ak_spi_nand
#endif


/**
* @brief configure spi nandflash transfer mode according to flags. 
* 
* @author lixinhai
* @date 2014-04-20
* @param[in] mtd info handle.
* @param[in] from: address.
* @param[in] transfer len.
* @param[out] transfer result len.
* @param[out] result buffer.
* @return int return config success or failed
* @retval returns zero on success
* @retval return a non-zero error code if failed
*/
static int ak_spinand_read(struct mtd_info *mtd, loff_t from, size_t len,
		size_t *retlen, u_char *buf)
{
	int ret = 0;
	size_t rlen = 0;
	u32 xfer_len;
	u32 offset = 0;
	u32 count = len;
	int row, column;
	struct ak_spinand *flash = mtd_to_spiflash(mtd);

	*retlen = 0;

	/*decode row/column in address param*/
	row = ((from>>flash->page_shift) & 0xffffff);
	column = (from & 0x7ff);
  
	while(count > 0) {
		xfer_len = (count > FLASH_BUF_SIZE) ? FLASH_BUF_SIZE : count;

		/*transfer len not greater than page size*/
		if(xfer_len > flash->info.page_size)
			xfer_len = ALIGN_DOWN(xfer_len, flash->info.page_size);
		if(xfer_len+column >= flash->info.page_size)
			xfer_len = flash->info.page_size - column;
    
		ret = spinand_read(mtd, row, column, xfer_len, &rlen, buf + offset);
		if(unlikely(ret)) {
			ret = -EBUSY;
			goto out;
		}
		row++;
		column = 0;
		*retlen += rlen;
		count -= rlen;		
		offset += rlen;
	}	
out:
	return ret;
}


//此函数暂不实���?

static int ak_spinand_read_noecc(struct mtd_info *mtd, loff_t from, size_t len,size_t *retlen, u_char *buf)
{
	printk("%s: no support\n",__func__);
	return 0;
}


static int spinand_do_read_page(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops)
{
	int ret = 0;
	size_t rlen = 0;
	u32 xfer_len,read_len,read_len_oob,oob_size,oob_xfer_len,oob_add_len,oob_seglen_len;
	u32 offset = 0;
	u32 oob_offset = 0;	
	u32 count = ops->len;
	int row, column;
	uint8_t buftmp[2112]={0};
	uint8_t *buf = NULL;	
	uint8_t *oobbuf = ops->oobbuf;
	uint8_t *datbuf = ops->datbuf;
	struct ak_spinand *flash = mtd_to_spiflash(mtd);
	
//printk("%d\n",count);	
	/*decode row/column in address param*/
	row = ((from>>flash->page_shift) & 0xffffff);
	column = (from & 0x7ff);
  
	read_len_oob = ops->ooblen;
	oob_seglen_len = flash->info.oob_up_skiplen + flash->info.oob_seglen + flash->info.oob_down_skiplen;
	oob_size  = flash->info.oob_size;
	oob_xfer_len = flash->info.oob_seglen;

	while(count > 0) {
		xfer_len = (count > FLASH_BUF_SIZE) ? FLASH_BUF_SIZE : count;

		/*transfer len not greater than page size*/
		if(xfer_len > flash->info.page_size)
			xfer_len = ALIGN_DOWN(xfer_len, flash->info.page_size);
		if(xfer_len+column >= flash->info.page_size)
			xfer_len = flash->info.page_size - column;
    
    	read_len = xfer_len + oob_size;

    
		ret = spinand_read(mtd, row, column, read_len, &rlen, buftmp/**/);
		if(unlikely(ret)) {
			ret = -EBUSY;
			goto out;
		}
		memcpy(datbuf + offset, buftmp, xfer_len);
		
	  	//buf	 = buftmp + rlen - oob_size;
	  	//skip bad block mark.
	  	buf	 = buftmp + rlen - oob_size + flash->info.oob_vail_data_offset;
	  oob_add_len = flash->info.oob_up_skiplen;
		

   if(read_len_oob > 0)
   	{
		while(read_len_oob > 0)
			{	
							 							 	
			 	oob_xfer_len = (read_len_oob > flash->info.oob_seglen) ? flash->info.oob_seglen : read_len_oob;

				memcpy(oobbuf+oob_offset, buf+oob_add_len, oob_xfer_len);
				
			 	oob_add_len += oob_seglen_len;							
				read_len_oob -= oob_xfer_len;		
				oob_offset += oob_xfer_len;
				ops->oobretlen += oob_xfer_len;			 
		  }
		}
	 else
	 	{
	 		oob_size = 0 ; 		
	 	} 
		row++;
		column = 0;
		ops->retlen += (rlen - oob_size);
		count -= (rlen - oob_size);		
		offset += (rlen-oob_size);	
  }
out:
	return ret;//ops->retlen;
}

/**
* @brief   MTD write
* 
* Write an address range to the flash chip.
* @author lixinhai
* @date 2014-03-20
* @param[in] mtd	mtd info handle.
* @param[in] to 	write start address.
* @param[in] len	write length.
* @param[out] retlen  write length at actually.
* @param[out] buf	   the pointer to write data.
* @return int return write success or failed
* @retval returns zero on success
* @retval return a non-zero error code if failed
*/
static int spinand_write(struct mtd_info *mtd, int row, int col, size_t len,
	size_t *retlen, const u_char *buf)
{
	struct ak_spinand *flash = mtd_to_spiflash(mtd);
	struct spi_transfer t[4];
	struct spi_message m;
	struct spi_message m1;	
	void *bounce_buf;
	//u_char buf_read[128]={0};
	//size_t read_rlen = 0;
	//int ret = 0, i = 0;

	DEBUG(MTD_DEBUG_LEVEL2, "%s: %s %s 0x%08x, len %zd\n",
			dev_name(&flash->spi->dev), __func__, "to(page_shift)",
			(u32)row, len);

	if (retlen)
		*retlen = 0;

	/* sanity checks */
	if (!len)
		return(0);

	spi_message_init(&m);
	spi_message_init(&m1);	
	memset(t, 0, (sizeof t));
	
	mutex_lock(&flash->lock);
	bounce_buf = flash_buf_bounce_pre(flash, (void*)buf, len, SPI_FLASH_WRITE);

	t[0].tx_buf = flash->cmd_buf;
	t[0].len = CMD_SIZE;
	spi_message_add_tail(&t[0], &m);

	t[1].tx_buf = &flash->cmd_buf[CMD_SIZE];
	t[1].len = ADDR_SIZE;
	t[1].xfer_mode = flash->txa_bus_width;
	spi_message_add_tail(&t[1], &m);

	t[2].tx_buf = bounce_buf;
	t[2].cs_change = 1;
	t[2].xfer_mode = flash->txd_bus_width;

	spi_message_add_tail(&t[2], &m);

	/* Wait until finished previous write command. */
	if (wait_till_ready(flash)) {
		mutex_unlock(&flash->lock);
		printk(KERN_ERR"kernel: error spinand_write write cmd fail\n");
		return -EBUSY;
	}

	write_enable(flash);
	/* Set up the opcode in the write buffer. */
	FILL_CMD(flash->cmd_buf, flash->tx_opcode);
	FILL_ADDR(flash->cmd_buf, col);

	t[2].len = len;

	spi_sync(flash->spi, &m);
	*retlen = m.actual_length - CMD_ADDR_SIZE;

	flash->cmd_buf[0] = OPCODE_PP_EXEC;
	flash->cmd_buf[1] = (row >> 16) & 0xff;
	flash->cmd_buf[2] = (row >> 8) & 0xff;
	flash->cmd_buf[3] = row & 0xff;
	
	t[3].tx_buf = flash->cmd_buf;
	t[3].len = 4;
	spi_message_add_tail(&t[3], &m1);
	spi_sync(flash->spi, &m1);		
	
//	spi_write(flash->spi, flash->cmd_buf, 4);

	/* Wait until finished previous write command. */
	if (wait_till_ready(flash))
	{
		printk(KERN_ERR"kernel: error spinand_write write data fail\n");
		return -EBUSY;
	}

	PDEBUG("ak_spinand_write: retlen=%ld\n", *retlen);
	flash_buf_bounce_post(flash, (void*)buf, len, SPI_FLASH_WRITE);

	mutex_unlock(&flash->lock);

	return 0;
}


#define FLASH_OOB_SIZE  8 
static int ak_spinand_write(struct mtd_info *mtd, loff_t to, size_t len,
		size_t *retlen, const u_char *buf)
{
	int ret = 0;
	size_t rlen = 0;
	u32 xfer_len;
	u32 offset = 0;
	u32 count = len;
	int row, column;
	struct ak_spinand *flash = mtd_to_spiflash(mtd);

	*retlen = 0;

	/*decode row/column in address param*/
	row = ((to>>flash->page_shift) & 0xffffff);
	column = (to & 0x7ff);


	while(count > 0) {
		xfer_len = (count > FLASH_BUF_SIZE) ? FLASH_BUF_SIZE : count;

		/*transfer len not greater than page size*/
		if(xfer_len > flash->info.page_size)
			xfer_len = ALIGN_DOWN(xfer_len, flash->info.page_size);
		if(xfer_len+column >= flash->info.page_size)
			xfer_len = flash->info.page_size - column;

		ret = spinand_write(mtd, row, column, xfer_len, &rlen, buf + offset);
		if(unlikely(ret)) {
			ret = -EBUSY;
			goto out;
		}
		
		row++;
		column = 0;
		*retlen += rlen;
		count -= rlen;		
		offset += rlen;
	}	
out:
	return ret;
}


/**
* @brief adjust transfer len according to readlen and column. 
* 
* @author lixinhai
* @date 2014-04-20
* @param[in] spiflash info handle.
* @param[in] column pos.
* @param[in] need read length.
* @retval return transfer len 
*/
static int adjust_xfer_len(struct ak_spinand *flash, int column, int readlen)
{
	int seg_oob;
	int xfer_len;
	int ofs = flash->mtd.writesize;
	int start = column - ofs;
	int end;

	/*|--------------------64bytes------------------------------|*/
	/*|---12---|-4--|---12----|-4--|---12---|-4--|---12----|-4--|*/
	/*|-seglen-|skip|-segllen-|skip|-seglen-|skip|-segllen-|skip|*/

	xfer_len = (readlen > flash->info.oob_seglen) ? flash->info.oob_seglen : readlen;
	end = start + xfer_len;
	seg_oob = flash->info.oob_up_skiplen + flash->info.oob_seglen + flash->info.oob_down_skiplen;
	if(start/seg_oob != end/seg_oob)
		end = (start/seg_oob + 1)*seg_oob;

	xfer_len = end - start;

	return xfer_len;	
}

/**
* @brief convert oob offset and addr pos to row/column coord. 
* 
* @author lixinhai
* @date 2014-04-20
* @param[in] spiflash info handle.
* @param[in] read from addr
* @param[in] offset to read from
* @return int return write success or failed
* @retval returns zero on success
* @retval return a non-zero error code if failed
*/
static int spinand_oobpos_to_coord(struct ak_spinand *flash,
	   	loff_t addr, uint32_t ooboffs, int *row, int *column)
{
	*row = ((addr >> flash->page_shift) & 0xffffff);
	*column = (addr & (flash->mtd.writesize - 1));

	*row += ooboffs / flash->mtd.oobsize;
	*column += ooboffs % flash->mtd.oobsize;

	*column += flash->mtd.writesize;

	if(*column > (flash->mtd.writesize + FLASH_OOB_SIZE))
		return -EINVAL;

	return 0;
}



static int spinand_do_read_oob(struct mtd_info *mtd, loff_t from,
			    struct mtd_oob_ops *ops)
{

	struct ak_spinand *flash = mtd_to_spiflash(mtd);
	int ret = 0;
	size_t rlen = 0;
	u32 xfer_len;
	u32 offset = 0;
	int row, column;
	uint8_t *oobrw_buf;	
	int readlen ;			
	uint8_t *buf = NULL ;
	ops->oobretlen = 0;	
	readlen = ops->ooblen;
	buf = ops->oobbuf;
	
	DEBUG(MTD_DEBUG_LEVEL3, "%s: from = 0x%08Lx, len = %i\n",
			__func__, (unsigned long long)from, readlen);


	if (unlikely(ops->ooboffs >= mtd->oobsize)) {
		DEBUG(MTD_DEBUG_LEVEL0, "%s: Attempt to start read "
					"outside oob\n", __func__);
		return -EINVAL;
	}

	/* Do not allow reads past end of device */
	if (unlikely(from >= mtd->size ||
		     ops->ooboffs + readlen > ((mtd->size >> flash->page_shift) -
					(from >> flash->page_shift)) * mtd->oobsize)) {
		DEBUG(MTD_DEBUG_LEVEL0, "%s: Attempt read beyond end "
					"of device\n", __func__);
		return -EINVAL;
	}				
		oobrw_buf =(uint8_t *)kmalloc(SPINAND_OOB_LEN(flash->info), GFP_KERNEL);	
		if (!oobrw_buf) {
		  oobrw_buf =(uint8_t *)kmalloc(SPINAND_OOB_LEN(flash->info), GFP_KERNEL);	
			if (!oobrw_buf) {
				printk("allocate memory for pInit_info failed\n");
				return -ENOMEM;
				} 
			} 		
		spinand_oobpos_to_coord(flash, from, ops->ooboffs, &row, &column);
		memset(oobrw_buf,0,SPINAND_OOB_LEN(flash->info));
	  // printk("rd:xfer_len:%d,%d,%d,%d,%d", row, column, xfer_len, from, ops->ooboffs);
		ret = spinand_read(mtd, row, column, SPINAND_OOB_LEN(flash->info), &rlen, oobrw_buf);
		if(unlikely(ret)) {
			ret = -EBUSY;
			goto out;
		}
		column = 0;
		//column += flash->info.oob_up_skiplen;
		column += flash->info.oob_vail_data_offset;

		
		while(readlen > 0) {
	
			xfer_len = (readlen > flash->info.oob_seglen) ? flash->info.oob_seglen : readlen;
			memcpy(buf + offset, oobrw_buf + column, xfer_len);
			
			column += (flash->info.oob_up_skiplen + flash->info.oob_seglen + flash->info.oob_down_skiplen);
	
			readlen -= xfer_len;		
			offset += xfer_len;
			ops->oobretlen += xfer_len;
		}
out:
	kfree(oobrw_buf);
	return ret;
}


static int ak_spinand_read_oob(struct mtd_info *mtd, loff_t from,
			 struct mtd_oob_ops *ops)
{
	int ret = -ENOTSUPP;
	struct ak_spinand *flash = mtd_to_spiflash(mtd); 
	uint32_t	page = 0; 

	ops->retlen = 0;

	/* Do not allow reads past end of device */
	if (ops->datbuf && (from + ops->len) > mtd->size) {
		DEBUG(MTD_DEBUG_LEVEL0, "%s: Attempt read "
				"beyond end of device\n", __func__);
		return -EINVAL;
	}

	switch(ops->mode) {
	case MTD_OOB_PLACE:
	case MTD_OOB_AUTO:
	case MTD_OOB_RAW:
		break;

	default:
		goto out;
	}

	if (!ops->datbuf){
//		printk("spinand_do_read_oob\n");
		ret = spinand_do_read_oob(mtd, from, ops);
	} else {
//		printk("spinand_do_read_page\n");
		ret = spinand_do_read_page(mtd, from, ops);
	}

	if(ret != 0)
	{
		page = ((from >> flash->page_shift) & 0xffffff);
		//printk(KERN_ERR"ak_check_error_to_update_flag_and_reboot&&&&&&&&&&&&&&&&&&&:%d,%d,%d\n", (uint32_t)from, page, flash->page_shift);
		ak_check_error_to_update_flag_and_reboot(page);
	}

 out:
// 	printk("%s,%d\n",__func__,ret);
	return ret;//ret;
}


static int spinand_do_write_oob(struct mtd_info *mtd, loff_t to,
			    struct mtd_oob_ops *ops)
{
	int ret = 0;
	size_t rlen = 0;
	u32 xfer_len;
	u32 offset = 0;
	int row, column;
	int ofs = mtd->writesize;
	int writelen = ops->ooblen;
	uint8_t *buf = ops->oobbuf;	
	struct ak_spinand *flash = mtd_to_spiflash(mtd);

	DEBUG(MTD_DEBUG_LEVEL3, "%s: from = 0x%08Lx, len = %i\n",
			__func__, (unsigned long long)to, writelen);

	if (unlikely(ops->ooboffs >= mtd->oobsize)) {
		DEBUG(MTD_DEBUG_LEVEL0, "%s: Attempt to start read "
					"outside oob\n", __func__);
		return -EINVAL;
	}

	/* Do not allow reads past end of device */
	if (unlikely(to >= mtd->size ||
		     ops->ooboffs + writelen > ((mtd->size >> flash->page_shift) -
					(to >> flash->page_shift)) * mtd->oobsize)) {
		DEBUG(MTD_DEBUG_LEVEL0, "%s: Attempt write beyond end "
					"of device\n", __func__);
		return -EINVAL;
	}
		ret = spinand_oobpos_to_coord(flash, to, ops->ooboffs, &row, &column);
		//column += flash->info.oob_up_skiplen;
		column += flash->info.oob_vail_data_offset;
		while(writelen> 0) {
			xfer_len = adjust_xfer_len(flash, column, writelen);
	
			//printk("wr:to(%d)ofs(%d):%d,%d,%d,%p", (u32)to, ops->ooboffs, row, column, xfer_len, buf);
			ret = spinand_write(mtd, row, column, xfer_len, &rlen, buf + offset);
			if(unlikely(ret)) {
				ret = -EBUSY;
				goto out;
			}
			
			column += (flash->info.oob_up_skiplen + rlen + flash->info.oob_down_skiplen);
			if(column >= ofs + SPINAND_OOB_LEN(flash->info)) {
				column = ofs;
				row++;
			}
			writelen -= rlen;
			offset += rlen;
		}
out:	
	return 0;		
	
}

static int spinand_do_write_page(struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops)
{
	int ret = 0;
	size_t rlen = 0;
	u32 xfer_len,read_len_oob,oob_size,oob_xfer_len,oob_seglen_len;// read_len,
	u16 spare_offset;
	u32 offset = 0;
	u32 oob_offset = 0;	
	u32 oob_add_len = 0;
	u32 count = ops->len;
	int row, column;
	int row_oob, column_oob;
	uint8_t buftmp_oob[64]={0};		
	uint8_t *p_buftmp_oob;
	uint8_t buftmp[2112]={0};
	u32 buftmp_len = 0;	
//	uint8_t *buf = NULL;	
	int i;
	uint8_t *oobbuf = ops->oobbuf;
	uint8_t *datbuf = ops->datbuf;
	struct ak_spinand *flash = mtd_to_spiflash(mtd);
	/*decode row/column in address param*/
	row = ((to>>flash->page_shift) & 0xffffff);
	column = (to & 0x7ff);
  
	read_len_oob = ops->ooblen;
	oob_seglen_len = flash->info.oob_up_skiplen + flash->info.oob_seglen + flash->info.oob_down_skiplen;
	oob_size  = flash->info.oob_size;
	oob_xfer_len = flash->info.oob_seglen;
	spare_offset = flash->info.oob_vail_data_offset;

	while(count > 0) {
		xfer_len = (count > FLASH_BUF_SIZE) ? FLASH_BUF_SIZE : count;

		/*transfer len not greater than page size*/
		if(xfer_len > flash->info.page_size)
			xfer_len = ALIGN_DOWN(xfer_len, flash->info.page_size);
		if(xfer_len+column >= flash->info.page_size)
			xfer_len = flash->info.page_size - column;

		memcpy(buftmp, datbuf+offset, xfer_len);
		
		
		spinand_oobpos_to_coord(flash, to, ops->ooboffs, &row_oob, &column_oob);
		ret = spinand_read(mtd, row_oob, column_oob, oob_size, &rlen, buftmp_oob/**/);

		PDEBUG("count=%d, xfer_data_len=%d, xfer_oob_len=%d\n", count, xfer_len, read_len_oob);
		PDEBUG("row_oob=%d, column_oob=%d\n", row_oob, column_oob);
		
		for(i=0; i<4; i++)
			PDEBUG("buftmp_oob[%d] = 0x%02x ", i, buftmp_oob[i]);
		PDEBUG("\n");
		
		p_buftmp_oob = buftmp_oob + spare_offset; //offset to spare data
			
		while(read_len_oob> 0){
			buftmp_len = (read_len_oob > oob_xfer_len) ? oob_xfer_len : read_len_oob;	   	
			memcpy(p_buftmp_oob + oob_add_len, oobbuf+oob_offset, buftmp_len);			
			read_len_oob -= buftmp_len;
			oob_offset += buftmp_len;
			oob_add_len +=oob_seglen_len;
		}
		memcpy(buftmp+xfer_len, buftmp_oob, oob_size);

 
		ret = spinand_write(mtd, row, column, xfer_len+oob_size, &rlen, buftmp);
		if(unlikely(ret)) {
			ret = -EBUSY;
			goto out;
		}
		
		row++;
		column = 0;
//		*retlen += xfer_len;
		count -= xfer_len;		
		offset += xfer_len;
	}	
out:
	return 0;//ret;	
	
}

#if 0
static int spinand_do_write_ops(struct mtd_info *mtd, loff_t to,
			    struct mtd_oob_ops *ops)
{
	int ret;

	ret = spinand_do_write_oob(mtd, to, ops);

	ret |= ak_spinand_write(mtd, to, ops->len, &ops->retlen, ops->datbuf);
	return ret;
}
#endif


static int ak_spinand_write_oob(struct mtd_info *mtd, loff_t to,
			 struct mtd_oob_ops *ops)
{
	int ret = -ENOTSUPP;

	ops->retlen = 0;

	/* Do not allow writes past end of device */
	if (ops->datbuf && (to + ops->len) > mtd->size) {
		DEBUG(MTD_DEBUG_LEVEL0, "%s: Attempt write beyond "
				"end of device\n", __func__);
		return -EINVAL;
	}

	switch(ops->mode) {
	case MTD_OOB_PLACE:
	case MTD_OOB_AUTO:
	case MTD_OOB_RAW:
		break;

	default:
		goto out;
	}

	if (!ops->datbuf)
		ret = spinand_do_write_oob(mtd, to, ops);
	else
		ret =  spinand_do_write_page(mtd, to, ops);//  spinand_do_write_ops(mtd, to, ops);

 out:
	return 0;//ret;
}

static int _ak_spinand_read(struct mtd_info *mtd, loff_t from, size_t len,
			size_t *retlen, u_char *buf)
{
	ak_spinand_read(mtd,from,len,retlen,buf);
	return 0;
}

static int _ak_spinand_write(struct mtd_info *mtd, loff_t to, size_t len,
		size_t *retlen, const u_char *buf)
{
	ak_spinand_write(mtd,to,len,retlen,buf);
	return 0;			
}

/**
 * nand_block_bad - [anyka] Read bad block marker in anyka bbt
 * @mtd:	MTD device structure
 * @ofs:	offset from device start
 *
 * Check, if the block is bad.
 */
int ak_spinand_block_isbad(struct mtd_info *mtd, loff_t offs)
{
	T_U32 blk_no = 0;
	T_U32 ret = 0;

	if (offs > mtd->size)
	{
	    printk("spinand %s %d offs=0x%llx erase:%08x Error!!!\n!!!\n", __func__, __LINE__, offs, mtd->erasesize);
		return -EINVAL;
	}

	blk_no = div_u64(offs, mtd->erasesize);
	//t = FHA_check_bad_block(blk_no); 
	ret = spinand_is_badblock(blk_no);

	ret = !ret;
	if(ret)
	{
		printk("blk_no:%ld is bad block\n!!!\n", blk_no);
	}
	
	return ret;
}

/**
 * ak_nand_block_markbad - [anyka] mark a block bad in anyka bbt
 * @mtd:	MTD device structure
 * @ofs:	offset from device start
 *
*/
int ak_spinand_block_markbad(struct mtd_info *mtd, loff_t offs)
{
	T_U32 blk_no = 0;
	T_U32 ret = 0;
	//bool protect = false;

	if (offs > mtd->size)
	{
	    printk("spinand %s %d offs=0x%llx erase:%08x Error!!!\n!!!\n", __func__, __LINE__, offs, mtd->erasesize);
		return -EINVAL;
	}

	//protect = ak_spinand_get_protect();

   // if(protect)
    	//ak_spinand_cfg_protect(false);

	blk_no = div_u64(offs, mtd->erasesize);
	//t = FHA_set_bad_block(blk_no);
	ret = spinand_set_badblock(blk_no);
	//printk("ak_spinand_block_markbad:%d\n", ret);
   // if(protect)
    	//ak_spinand_cfg_protect(true);

	if(ret == 0)
	{
		return 0;  //mark successed
	}
	else
	{
		return 1;  //mark failed
	}
}



/**
* @brief	MTD get device ID
* 
* get the device ID of  the spi nand chip.
* @author lixinhai
* @date 2014-03-20
* @param[in] mtd	 mtd info handle.
* @return int return device ID of  the spi nand chip.
*/
static int ak_spinand_get_devid(struct mtd_info *mtd)
{
	struct ak_spinand *flash = mtd_to_spiflash(mtd);
	int			ret;
	u8			id[5];
	u32			jedec;

	/* Wait until finished previous write command. */
	if (wait_till_ready(flash))
		return -EBUSY;

	flash->cmd_buf[0]  = OPCODE_RDID;
	flash->cmd_buf[1]  = 0x0;
	/* JEDEC also defines an optional "extended device information"
	 * string for after vendor-specific data, after the three bytes
	 * we use here.  Supporting some chips might require using it.
	 */
	ret = spi_write_then_read(flash->spi, flash->cmd_buf, 2, id, 4);
	if (ret < 0) {
		DEBUG(MTD_DEBUG_LEVEL0, "%s: error %d ak_spinand_get_devid\n",
			dev_name(&flash->spi->dev), ret);
		return AK_FALSE;
	}

	jedec = id[0] | (id[1]<<8) | (id[2]<<16) | (id[3]<<24) ;
	printk("spi nand ID: 0x%08x\n", jedec);

	return jedec;
}

static int ak_fha_erase_callback(T_U32 chip_num,  T_U32 block)
{
	struct erase_info einfo;
	struct ak_spinand *flash = mtd_to_spiflash(ak_mtd_info);
  
	memset(&einfo, 0, sizeof(struct erase_info));
	einfo.addr = block* flash->info.page_per_block * flash->info.page_size;
	einfo.len = ak_mtd_info->erasesize;
	einfo.mtd = ak_mtd_info;

	if(ak_spinand_erase(ak_mtd_info, &einfo) == 0)
	{
		return FHA_SUCCESS;
	}
	else
	{
		printk(KERN_ERR"***erase failed, block:%ld\n", block);
		return FHA_FAIL;
	}
}

#if 0
T_U32 ak_fha_write_callback(T_U32 chip_num, T_U32 page_num, const T_U8 *data,
		T_U32 data_len, T_U8 *oob, T_U32 oob_len, T_U32 eDataType)
{
	int ret;
	ssize_t retlen;
	struct ak_spinand *flash = mtd_to_spiflash(ak_mtd_info);
	loff_t to = page_num * flash->info.page_size;

	
	ret = ak_spinand_write(ak_mtd_info, to, data_len, &retlen, (u_char *)data);
	if(ret)
	{
		printk("%s:%d\n", __func__, __LINE__);
		return FHA_FAIL;
	}
#if 0	
		struct mtd_oob_ops ops;	
		if(oob_len >0)
		{

			ops.ooblen = oob_len;
			ops.oobbuf = oob;	
			ops.ooboffs = 0;			
			ret = spinand_do_write_oob(ak_mtd_info, to, &ops);
			if (ret) {
				printk("%s:%d\n", __func__, __LINE__);
				return FHA_FAIL;
			}
		}
#endif		
	return FHA_SUCCESS;
}

#else
int ak_fha_write_callback(T_U32 chip_num, T_U32 page_num, const T_U8 *data,T_U32 data_len, T_U8 *oob, T_U32 oob_len, T_U32 eDataType)
{
    int row = 0, column = 0;
    int row_oob = 0, column_oob = 0;
    uint8_t buftmp_oob[64]={0};  
    uint8_t *p_buftmp_oob;
    uint8_t buftmp[2112]={0};
    u32 buftmp_len = 0; 
    u32 count = data_len;
    u32 xfer_len,read_len_oob,oob_size,oob_xfer_len,oob_seglen_len;// read_len,
    size_t rlen = 0;
    u32 offset = 0;
    u16 spare_offset = 0;
    int i = 0;
    u32 oob_offset = 0; 
    u32 oob_add_len = 0;
    u32 ooboffs = 0;

    int ret;
    struct ak_spinand *flash = mtd_to_spiflash(ak_mtd_info);
    loff_t to = page_num * flash->info.page_size;


    row = ((to>>flash->page_shift) & 0xffffff);
    column = (to & 0x7ff);

    read_len_oob = oob_len;
    oob_seglen_len = flash->info.oob_up_skiplen + flash->info.oob_seglen + flash->info.oob_down_skiplen;
    oob_size  = flash->info.oob_size;
    oob_xfer_len = flash->info.oob_seglen;
    spare_offset = flash->info.oob_vail_data_offset;
    ooboffs = flash->info.oob_vail_data_offset;

    while(count > 0) {
        xfer_len = (count > FLASH_BUF_SIZE) ? FLASH_BUF_SIZE : count;

        /*transfer len not greater than page size*/
        if(xfer_len > flash->info.page_size)
            xfer_len = ALIGN_DOWN(xfer_len, flash->info.page_size);
        if(xfer_len+column >= flash->info.page_size)
            xfer_len = flash->info.page_size - column;

        memcpy(buftmp, data+offset, xfer_len);

        //printk("ooboffs:%d\n", ooboffs);
        spinand_oobpos_to_coord(flash, to, ooboffs, &row_oob, &column_oob);
        ret = spinand_read(ak_mtd_info, row_oob, column_oob, oob_size, &rlen, buftmp_oob/**/);

        PDEBUG("count=%d, xfer_data_len=%d, xfer_oob_len=%d\n", count, xfer_len, read_len_oob);
        PDEBUG("row_oob=%d, column_oob=%d\n", row_oob, column_oob);

        for(i=0; i<4; i++)
        PDEBUG("buftmp_oob[%d] = 0x%02x ", i, buftmp_oob[i]);
        PDEBUG("\n");

        p_buftmp_oob = buftmp_oob + spare_offset; //offset to spare data


        while(read_len_oob> 0){
            buftmp_len = (read_len_oob > oob_xfer_len) ? oob_xfer_len : read_len_oob;     
            memcpy(p_buftmp_oob + oob_add_len, oob+oob_offset, buftmp_len);   
            read_len_oob -= buftmp_len;
            oob_offset += buftmp_len;
            oob_add_len +=oob_seglen_len;
        }
        memcpy(buftmp+xfer_len, buftmp_oob, oob_size);


        ret = spinand_write(ak_mtd_info, row, column, xfer_len+oob_size, &rlen, buftmp);
        if(unlikely(ret)) {
            ret = -1;
            goto out;
        }

        row++;
        column = 0;
        //  *retlen += xfer_len;
        count -= xfer_len;  
        offset += xfer_len;
    }

out:
    return ret; 
}


#endif

int ak_fha_read_callback(T_U32 chip_num, T_U32 page_num, T_U8 *data,
		T_U32 data_len, T_U8 *oob, T_U32 oob_len, T_U32 eDataType)
{
	int ret;
	ssize_t retlen;
	struct mtd_oob_ops ops;	
	//printk("%s,data_len:%d,oob_len:%d\n",__func__,data_len,oob_len);
	struct ak_spinand *flash = mtd_to_spiflash(ak_mtd_info);
	loff_t from = page_num * flash->info.page_size;


	//printk("%d, %d, %d, %d\n", from, page_num, flash->info.page_size, data_len);
	if(data_len > 0)
		{
			ret = ak_spinand_read(ak_mtd_info, from, data_len, &retlen, data);
			if (ret) {
				printk("%s:%d\n", __func__, __LINE__);
				return FHA_FAIL;
			}
		}
#if 1	
	
	if(oob_len >0)
		{

			ops.ooblen = oob_len;
			ops.oobbuf = oob;
			ops.ooboffs = 0;
			ret = spinand_do_read_oob(ak_mtd_info, from, &ops);
			if (ret) {
				printk("%s:%d\n", __func__, __LINE__);
				return FHA_FAIL;
			}
		}
#endif	
	return FHA_SUCCESS;
}

int ak_fha_readbyte_callback(T_U32 chip_num, T_U32 page_num, T_U8 *data,
		T_U32 data_len, T_U8 *oob, T_U32 oob_len, T_U32 eDataType)
{
	int ret;
	ssize_t retlen;
	//printk("%s,data_len:%d,oob_len:%d\n",__func__,data_len,oob_len);
	struct ak_spinand *flash = mtd_to_spiflash(ak_mtd_info);
	loff_t from = page_num * flash->info.page_size;

	if(data_len > 0)
		{
			ret = ak_spinand_read_noecc(ak_mtd_info, from, data_len, &retlen, data);
			if (ret) {
				printk("%s:%d\n", __func__, __LINE__);
				return FHA_FAIL;
			}
		}
#if 0	
	struct mtd_oob_ops ops;	
	if(oob_len >0)
		{

			ops.ooblen = oob_len;
			ops.oobbuf = oob;
			ops.ooboffs = 0;
			ret = spinand_do_read_oob(ak_mtd_info, from, &ops);
			if (ret) {
				printk("%s:%d\n", __func__, __LINE__);
				return FHA_FAIL;
			}
		}
#endif	
	return FHA_SUCCESS;
}

static T_VOID *fha_ram_alloc(T_U32 size)
{
	return kmalloc(size, GFP_KERNEL);
}

static T_VOID *fha_ram_free(void *point)
{
	kfree(point);
	return NULL;
}

static T_S32 fha_print(T_pCSTR fmt, ...)
{
	va_list args;
	int r;
    
	va_start(args, fmt);
    	vprintk("FHA:",args);
	r = vprintk(fmt, args);
	va_end(args);

	return r;
}

#if 0 
int ak_get_partition_bak_name_idex(T_U32 partition_idex)
{
	char name[8] = {0};
	T_U32 i = 0, nr_parts= 0;
	T_PARTITION_TABLE_INFO *parts = NULL;

	nr_parts = *(unsigned long *)g_part_tab.table;
	parts = (T_PARTITION_TABLE_INFO *)(&g_part_tab.table[sizeof(unsigned long)]);

	//printk(KERN_ERR "nr_parts:%d\n", nr_parts);
	for(i = 0; i < nr_parts; i++)
	{
		memset(name, 0, 8);
		memcpy(name, parts[i].partition_info.name, PARTITION_NAME_LEN);

		//printk(KERN_ERR "name:%s, %s\n",g_partition_info.partition_name_info[partition_idex].partition_name, name);
		if(strlen(g_partition_info.partition_name_info[partition_idex].partition_name) == strlen(name))
		{
			if(memcmp(g_partition_info.partition_name_info[partition_idex].partition_name, name, strlen(name)) == 0)
			{
				break;
			}
		}
	}

	if(i == nr_parts || i == 0)
	{
		return -1;
	}


	return (int)i;
}



int ak_ckeck_partition_A_bak_name(void)
{
	char buf[10] = {0};
	T_U32 str_len = 0;
	int idex = 0;
	char *dev_name_temp = "/dev/mtdblock";
	T_U32 partition_idex = A_PARTITION_IDEX;  //1表示A分区

	//判断ROOT盘应用哪个mtdblock
	if(g_partition_info.partition_name_info[partition_idex].partition_name[0] != 0 && g_partition_info.partition_cnt == PARTITION_CNT)
	{
		str_len = strlen(dev_name_temp);
		idex = ak_get_partition_bak_name_idex(partition_idex);
		if(idex == -1)
		{
			return -1;
		}
		
		printk(KERN_ERR "saved_root_name:%s\n", saved_root_name);
		printk(KERN_ERR "idex:%d, str_len:%ld\n", idex, str_len);
		memset(buf, 0, 10);
		sprintf(buf, "%d", idex + 1);
		memcpy(&saved_root_name[str_len], buf, strlen(buf)+1);
	}
	
	printk(KERN_ERR "saved_root_name:%s\n", saved_root_name);

	return 0;
}
#endif




#define NAND_BOIS_NAME   "KERNEL"
#define NAND_A_NAME      "A"
#define NAND_B_NAME      "B"


int ak_get_readonly_partition_pos(unsigned long page_size)
{
	unsigned long nr_parts = 0;
	unsigned long i = 0;
	T_PARTITION_TABLE_INFO *parts;
	unsigned char partition_name[8] = {0};


	printk(KERN_ERR "ak_get_readonly_partition_pos :%ld\n", page_size);
	
	if (page_size == 0) {
		printk(KERN_ERR "page_size error :%ld\n", page_size);
		return -1;
	}

	nr_parts = ((unsigned long *)g_part_tab.table)[0];

	printk(KERN_ERR "nr_parts:%ld\n", nr_parts);
	
	parts = (T_PARTITION_TABLE_INFO *)(&g_part_tab.table[sizeof(unsigned long)]);
	for (i = 0; i < nr_parts; i++) {
		memset(partition_name, 0, 8);
		memcpy(partition_name, parts[i].partition_info.name, PARTITION_NAME_LEN);

		//printk(KERN_ERR "partition_name:%s, %s\n",NAND_BOIS_NAME, partition_name);

		if(strlen(NAND_BOIS_NAME) == strlen(partition_name) && memcmp(NAND_BOIS_NAME, partition_name, strlen(partition_name)) == 0)
		{
			kernel_start_page = parts[i].partition_info.start_pos/page_size;
			kernel_end_page = kernel_start_page + parts[i].partition_info.ksize*1024/page_size;

			printk(KERN_ERR "NAND_BOIS_NAME:%s, kernel_start_page:%ld, kernel_end_page:%ld\n", NAND_BOIS_NAME, kernel_start_page, kernel_end_page);
		}
		else if(strlen(NAND_A_NAME) == strlen(partition_name) && memcmp(NAND_A_NAME, partition_name, strlen(partition_name)) == 0)
		{
			rtf_start_page = parts[i].partition_info.start_pos/page_size;
			rtf_end_page = rtf_start_page + parts[i].partition_info.ksize*1024/page_size;
			
			printk(KERN_ERR "NAND_A_NAME:%s, rtf_start_page:%ld, rtf_end_page:%ld\n", NAND_A_NAME, rtf_start_page, rtf_end_page);
		}
	
	
	}
	return 0;

}

extern void ak39_restart(char str, const char *cmd);

int ak_check_error_to_update_flag_and_reboot(unsigned long page)
{
	unsigned char buf[64] = {0};
	unsigned char knl_page_error_flag = 0;
	unsigned char rtf_page_error_flag = 0;
	T_PARTITION_INFO *partition_info = (T_PARTITION_INFO *)buf;
	
	struct ak_spinand *flash = mtd_to_spiflash(ak_mtd_info);
	unsigned long blk_no = page/flash->info.page_per_block;
	
	if(spinand_is_badblock(blk_no) == 0)
	{
		return 0;
	}

	//printk(KERN_ERR "ak_check_error_to_update_flag_and_reboot@@@@@@@@@@@@@@@@@@@@@@@@@@@:%d, %d, %ld\r\n", knl_page_error_flag, rtf_page_error_flag, page);

	if(kernel_start_page != 0 && kernel_end_page != 0)
	{
		if(page >= kernel_start_page && page < kernel_end_page)
		{
			knl_page_error_flag = 1;
		
		}
		
	}

	if(rtf_start_page != 0 && rtf_end_page != 0)
	{
		if(page >= rtf_start_page && page < rtf_end_page)
		{
			rtf_page_error_flag = 1;
		
		}
	
	}

	partition_info->partition_cnt = PARTITION_CNT;
	
	//
	if(knl_page_error_flag == 1)
	{
		memcpy(partition_info->partition_name_info[0].update_flag, UPDATE_END, strlen(UPDATE_END));
	}
	
	if(rtf_page_error_flag == 1)
	{
		memcpy(partition_info->partition_name_info[1].update_flag, UPDATE_END, strlen(UPDATE_END));
	}
	
	//printk(KERN_ERR "ak_check_error_to_update_flag_and_reboot@@@@@@@@@@@@@@@@@@@@@@@@@@@:%d, %d, %ld\r\n", knl_page_error_flag, rtf_page_error_flag, page);
	if(knl_page_error_flag == 1 || rtf_page_error_flag == 1)
	{
		//	//printk(KERN_ERR "spinand_write_asa_data@@@@@@@@@@@@@@@@@@@@@@@@@@@:%d, %d, %ld\r\n", knl_page_error_flag, rtf_page_error_flag, page);
		printk(KERN_ERR "page read data error, and reboot system to reconvert the parttition\r\n");
		if(spinand_write_asa_data(buf, sizeof(T_PARTITION_INFO)) == -1)
		{
			printk(KERN_ERR "spinand_write_asa_data fail\r\n");
			return -1;
		}
		//msleep(2000);
	
		//
		ak39_restart(0, NULL);
	}
	
	return 0;
}




static int ak_fha_init(void)
{
	int ret = FHA_FAIL;
	//T_PFHA_INIT_INFO pInit_info = NULL;
	T_PFHA_LIB_CALLBACK pCallback = NULL;
	T_NAND_PHY_INFO pNandPhyInfo;
	struct ak_spinand *flash = mtd_to_spiflash(ak_mtd_info);
	unsigned long asa_start_block = 1;
	//unsigned char pa_data[64] = {0};

	pCallback = kmalloc(sizeof(T_FHA_LIB_CALLBACK), GFP_KERNEL);
	if (!pCallback) {
		printk("allocate memory for pCallback failed\n");
		ret = -ENOMEM;
		goto err_out;
	}
	
	pCallback->Erase = ak_fha_erase_callback;
	pCallback->Write = (FHA_Write)ak_fha_write_callback;
	pCallback->Read = (FHA_Read)ak_fha_read_callback;
	pCallback->ReadNandBytes = (FHA_ReadNandBytes)ak_fha_readbyte_callback;
	pCallback->RamAlloc = fha_ram_alloc;
	pCallback->RamFree = fha_ram_free;
	pCallback->MemCmp = (FHA_MemCmp)memcmp;
	pCallback->MemSet = (FHA_MemSet)memset;
	pCallback->MemCpy = (FHA_MemCpy)memcpy;
	pCallback->Printf = (FHA_Printf)fha_print;	


	//ret = FHA_mount(pInit_info, pCallback, &pNandPhyInfo);

	pNandPhyInfo.chip_id = flash->info.jedec_id;
	pNandPhyInfo.page_size = flash->info.page_size;
	pNandPhyInfo.page_per_blk = flash->info.page_per_block;
	pNandPhyInfo.blk_num = flash->info.n_blocks;
	pNandPhyInfo.plane_blk_num = flash->info.n_blocks;
	pNandPhyInfo.spare_size = flash->info.oob_size;
	
	ret = partition_init(pCallback, &pNandPhyInfo, 0);
	if (ret == FHA_FAIL) {
		printk(KERN_ERR "partition_init failed\n");
		ret = FHA_FAIL;
	} else {
		ret = 0;
	}

	//获取第一个坏块表的开始块
	

	//badblock init
	if(-1 == spinand_babblock_tbl_init(0, true, asa_start_block))
	{
		printk(KERN_ERR "spinand_babblock_tbl_init failed\n");
		ret = FHA_FAIL;
	}
	else
	{
		ret = 0;
	}

	//printk(KERN_ERR "saved_root_name:%s\n", saved_root_name);
	//read the update partition name
	#if 0
	memset(pa_data, 0, 64);
	
	if(spinand_read_asa_data(pa_data, sizeof(T_PARTITION_INFO)) == -1)
	{
		printk(KERN_ERR "not read the update partition name\n");
		memset(&g_partition_info, 0, sizeof(T_PARTITION_INFO));
	}
	else
	{
		memcpy(&g_partition_info, pa_data, sizeof(T_PARTITION_INFO));
		printk(KERN_ERR "g_partition_info.partition_cnt:%ld\n", g_partition_info.partition_cnt);
		printk(KERN_ERR "partition_name_info[0].partition_name:%s\n", g_partition_info.partition_name_info[0].partition_name);
		printk(KERN_ERR "partition_name_info[1].partition_name:%s\n", g_partition_info.partition_name_info[1].partition_name);
		printk(KERN_ERR "partition_name_info[2].partition_name:%s\n", g_partition_info.partition_name_info[2].partition_name);
		
	}
	#endif


	//FHA_asa_scan(AK_TRUE);
	kfree(pCallback);
err_out:
	//kfree(pInit_info);
	return ret;
}

int ak_fha_init_for_update(int n)
{
#if 0
	int ret = 0,retval = 0;
	T_PFHA_INIT_INFO pInit_info = NULL;
	T_PFHA_LIB_CALLBACK pCallback = NULL;
	T_NAND_PHY_INFO pNandPhyInfo;

	pInit_info = kmalloc(sizeof(T_FHA_INIT_INFO), GFP_KERNEL);
	if (!pInit_info) {
		printk("allocate memory for pInit_info failed\n");
		return -ENOMEM;
	}

	pInit_info->nChipCnt = 1;
	pInit_info->nBlockStep = 1;
	pInit_info->eAKChip = FHA_CHIP_SET_TYPE;
	pInit_info->ePlatform = PLAT_LINUX;
	pInit_info->eMedium = MEDIUM_SPI_NAND;
	pInit_info->eMode = MODE_UPDATE;

	pCallback = kmalloc(sizeof(T_FHA_LIB_CALLBACK), GFP_KERNEL);
	if (!pCallback) {
		printk("allocate memory for pCallback failed\n");
		ret = -ENOMEM;
		goto err_out;
	}
	
	pCallback->Erase = ak_fha_erase_callback;
	pCallback->Write = (FHA_Write)ak_fha_write_callback;
	pCallback->Read = (FHA_Read)ak_fha_read_callback;
	pCallback->RamAlloc = fha_ram_alloc;
	pCallback->RamFree = fha_ram_free;
	pCallback->MemCmp = (FHA_MemCmp)memcmp;
	pCallback->MemSet = (FHA_MemSet)memset;
	pCallback->MemCpy = (FHA_MemCpy)memcpy;
	pCallback->Printf = (FHA_Printf)fha_print;

	ret = FHA_mount(pInit_info, pCallback, &pNandPhyInfo);
	if (ret == FHA_FAIL) {
		printk("FHA_mount failed\n");
		ret = -EINVAL;
	} else {
		ret = 0;
	}
  retval =  FHA_burn_init(pInit_info, pCallback, &pNandPhyInfo);
  FHA_asa_scan(AK_TRUE);
  if(retval == FHA_FAIL)
  {
      goto err_out;
  }
  


	kfree(pCallback);
err_out:
	kfree(pInit_info);
	return ret;
#else
	int ret = 0;

	return ret;
#endif	
 

}





#if 0
unsigned long ak_check_sn_flag(unsigned char *data)
{
	void *handle = NULL;
	unsigned char *name = "ENV";//"NVT";  //"ENV"
	unsigned long data_len;
	int ret = -1;
	
	printk(KERN_ERR "partition_open:%s\n", name);
	handle = partition_open(name);
	if(handle == NULL)
	{
		printk(KERN_ERR "partition_open fail:%s\n", name);
		return 0;
	}

	data_len = partition_get_data_size(handle);
	printk(KERN_ERR "data_len:%ld\n", data_len);
	if(data_len == 0 || data_len > 2048)
	{
		partition_close(handle);
		printk(KERN_ERR "the partition no have anything, name:%s, data_len:%ld\n", name, data_len);
		return 0;
	}

	ret = partition_read(handle, data, data_len);
	if(ret == -1)
	{
		partition_close(handle);
		printk(KERN_ERR "partition_read fail, data_len:%ld\n", data_len);
		return 0;
	}

	partition_close(handle);

	//printk(KERN_ERR "data:%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\n", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8]);
	printk(KERN_ERR "data:%s\n", data);


	return data_len; 
	

}


int check_sn_data(char *data)
{
	if(memcmp(data, "0601BOK184403N", strlen("0601BOK184403N")) == 0){
		return 0;
	}

	return -1;
}
#endif

#if 0
static int ak_check_chang_flash_param(void)
{
	unsigned long data_len =  0;
	unsigned char data[2048] = {0};	

	struct ak_spinand *flash = mtd_to_spiflash(ak_mtd_info);

	printk(KERN_ERR "flash->info.oob_up_skiplen:%u\n", flash->info.oob_up_skiplen);
	printk(KERN_ERR "flash->info.oob_seglen:%u\n", flash->info.oob_seglen);
	printk(KERN_ERR "flash->info.oob_down_skiplen:%u\n", flash->info.oob_down_skiplen);
	printk(KERN_ERR "flash->info.oob_seg_perpage:%u\n", flash->info.oob_seg_perpage);

	data_len = ak_check_sn_flag(data);
	if(0 == data_len)
	{
		return -1;
	}

	//0601BOK184403N000145
	if(check_sn_data(data) == 0)
	{
		flash->info.oob_up_skiplen = 0;
		flash->info.oob_seglen = 16;
		flash->info.oob_down_skiplen = 0;
		printk(KERN_ERR "need to chang the flash param\n");
	}
	printk(KERN_ERR "flash->info.oob_up_skiplen:%u\n", flash->info.oob_up_skiplen);
	printk(KERN_ERR "flash->info.oob_seglen:%u\n", flash->info.oob_seglen);
	printk(KERN_ERR "flash->info.oob_down_skiplen:%u\n", flash->info.oob_down_skiplen);
	printk(KERN_ERR "flash->info.oob_seg_perpage:%u\n", flash->info.oob_seg_perpage);

	return 0;
}
#endif


#ifdef CONFIG_MTD_SPINAND_PRODUCER	
static int ak_mount_partitions(struct spi_device *spi)
{
	int i, ret;
	unsigned long nr_parts;
	//unsigned char *buf;
	T_PARTITION_TABLE_INFO *parts = NULL;
	struct mtd_partition *mtd_part;
	struct mtd_part_parser_data	ppdata;	
	struct ak_spinand *flash = mtd_to_spiflash(ak_mtd_info);

	ret = ak_fha_init();
	if (ret == FHA_FAIL) {
		printk("Init FHA lib failed\n");
		goto err_out;
	}


#if 1
	ret = partition_get_partition_table(&g_part_tab,PART_SPINAND);
	if (ret == FHA_FAIL) {
		printk(KERN_ERR "get src partition failed, and read back partition\n");
		ret =  partition_get_partition_backup_table(&g_part_tab,PART_SPINAND);
		if (ret == FHA_FAIL) {
			printk(KERN_ERR "get back partition info failed\n");
			ret = !ret;
			goto no_parts;
		}
	}

	/* 
	* if no partiton to mount, the buf will be all 0xFF but not constant.
	* So, it is not safe here. 
	*/
	nr_parts = *(unsigned long *)g_part_tab.table;
	//15 --> partition_get_partition_table only copy 512 byte,so max cnt is 15
	if (nr_parts <= 0 || nr_parts > 15) {
		printk(KERN_ERR "get src partition num:%ld error, and read back partition\n", nr_parts);
		ret =  partition_get_partition_backup_table(&g_part_tab,PART_SPINAND);
		if (ret == FHA_FAIL) {
			printk(KERN_ERR "get partition info failed\n");
			ret = !ret;
			goto no_parts;
		}

		nr_parts = *(unsigned long *)g_part_tab.table;
		if (nr_parts <= 0 || nr_parts > 15) 
		{
			printk(KERN_ERR "partition count invalid, nr_parts:%ld\n", nr_parts);
			ret = -EINVAL;
			goto no_parts;
		}
	}

	mtd_part = kzalloc(sizeof(struct mtd_partition) * nr_parts, GFP_KERNEL);
	if (!mtd_part) {
		printk(KERN_ERR "allocate memory for mtd_partition failed\n");
		ret = -ENOMEM;
		goto no_parts;
	}

	parts = (T_PARTITION_TABLE_INFO *)(&g_part_tab.table[sizeof(unsigned long)]);
	for (i = 0; i < nr_parts; i++) {
		mtd_part[i].name = kzalloc(PARTITION_NAME_LEN, GFP_KERNEL);
		memcpy(mtd_part[i].name, parts[i].partition_info.name, PARTITION_NAME_LEN);
		mtd_part[i].size = parts[i].partition_info.ksize*1024;
		mtd_part[i].offset = parts[i].partition_info.start_pos;
		mtd_part[i].mask_flags = parts[i].partition_info.r_w_flag;
		printk("mtd_part[%d]:\nname = %s\nsize = 0x%llx\noffset = 0x%llx\nmask_flags = 0x%x\n\n",
				i, 
				mtd_part[i].name, 
				mtd_part[i].size, 
				mtd_part[i].offset, 
				mtd_part[i].mask_flags);

	}

	ak_get_readonly_partition_pos(flash->info.page_size);
	//ak_ckeck_partition_A_bak_name();

	//ak_check_chang_flash_param();
	

	/*
	* call ak_partition_table_sys_create function , create sys kobject for app aplication
	*/
	ret = ak_partition_table_sys_create(&g_part_tab);
	if (ret){
		printk(KERN_ERR "create sys partition table kobject failed!\n");
		ret = -EINVAL;
		kfree(mtd_part);
		goto no_parts;
	}

#else

	buf = kzalloc(flash->info.page_size, GFP_KERNEL);
	if (!buf) {
		printk("allocate memory for page buffer failed\n");
		ret = -ENOMEM;
		goto err_out;
	}

	ret = FHA_get_fs_part(buf, flash->info.page_size);
	if (ret == FHA_FAIL) {
		printk("get partition info failed\n");
		ret = !ret;
		goto no_parts;
	}

	nr_parts = *(unsigned long *)buf;
	/* if no partiton to mount, the buf will be all 0xFF but not constant.
	 * So, it is not safe here. */
	printk("nr_parts=0x%lx\n", nr_parts);
	if (nr_parts <= 0 || nr_parts > 32) {
		printk("partition count invalid\n");
		ret = -EINVAL;
		goto no_parts;
	}

	mtd_part = kzalloc(sizeof(struct mtd_partition) * nr_parts, GFP_KERNEL);
	if (!mtd_part) {
		printk("allocate memory for mtd_partition failed\n");
		ret = -ENOMEM;
		goto no_parts;
	}

	parts = (struct partitions *)(&buf[sizeof(unsigned long)]);


	for (i = 0; i < nr_parts; i++) {
		mtd_part[i].name = kzalloc(MTD_PART_NAME_LEN, GFP_KERNEL);
		memcpy(mtd_part[i].name, parts[i].name, MTD_PART_NAME_LEN);
		mtd_part[i].size = parts[i].size;
		mtd_part[i].offset = parts[i].offset;
		mtd_part[i].mask_flags = parts[i].mask_flags;
		printk("mtd_part[%d]:\nname = %s\nsize = 0x%llx\noffset = 0x%llx\nmask_flags = 0x%x\n\n",
				i, mtd_part[i].name, mtd_part[i].size, mtd_part[i].offset, mtd_part[i].mask_flags);
	}
/*
	ret = add_mtd_partitions(ak_mtd_info, (const struct mtd_partition *)mtd_part, nr_parts);

	if (ret) {
		printk("add mtd partition failed\n");
		goto no_parts;
	}
*/
#endif
	ppdata.of_node = spi->dev.of_node;

	ret = mtd_device_parse_register(ak_mtd_info, NULL, &ppdata, 
			(const struct mtd_partition *)mtd_part, nr_parts);
	if (ret) {
		printk("add mtd partition failed\n");
		goto no_parts;
	}


no_parts:
	//kfree(buf);
err_out:
	return ret;
}
#endif

/**
* @brief	 get_flash_info
* 
* Read the device ID and identify that it was supported or not.
* @author 	luoyongchuang
* @date 		2016-03-17
* @param[in] mtd	  spi device handle.
* @return int return the device info.
*/

static struct flash_info *__devinit get_flash_info(struct spi_device *spi)
{
	int			tmp;
	u8			cmd[2];
	u8			id[5];
	u32			jedec;
	u16                     ext_jedec = 0;
	struct flash_info	*info;
	/* JEDEC also defines an optional "extended device information"
	 * string for after vendor-specific data, after the three bytes
	 * we use here.  Supporting some chips might require using it.
	 */
	cmd[0] = OPCODE_RDID; 
	cmd[1] = 0x0; 
	tmp = spi_write_then_read(spi, cmd, 2, id, 4);
	if (tmp < 0) {
		DEBUG(MTD_DEBUG_LEVEL0, "%s: error %d reading JEDEC ID\n",
			dev_name(&spi->dev), tmp);
		return NULL;
	}
	jedec = id[0];
	jedec = jedec << 8;
	jedec |= id[1];
	jedec = jedec << 8;
	jedec |= id[2];
	jedec = jedec << 8;
	jedec |= id[3];

	printk("akspi nand ID: 0x%08x\n", jedec);

	//ext_jedec = id[3] << 8 | id[4];
	for (tmp = 0, info = ak_spinand_supportlist;
			tmp < ARRAY_SIZE(ak_spinand_supportlist);
			tmp++, info++) {
		if (info->jedec_id == jedec) {
			if (info->ext_id != 0 && info->ext_id != ext_jedec)
				continue;
			return info;
		}
	}
	dev_err(&spi->dev, "get_flash_info() unrecognized flash id %06x\n", jedec);
	return NULL;
}

static int ak_spinand_init_stat_reg(struct ak_spinand *flash)
{
	int i;
	struct flash_status_reg *sr;
	struct flash_info *info = &flash->info;

	for(i=0, sr=status_reg_list; i<ARRAY_SIZE(status_reg_list); i++, sr++) {
		if (sr->jedec_id == info->jedec_id) {
			if (info->ext_id != 0 && info->ext_id != sr->ext_id)
				continue;
			flash->stat_reg = *sr;
			return 0;
		}
	}
	
	flash->stat_reg = status_reg_list[i-1];
	return 0;
}



/**
* @brief	 spi nand probe
* 
* Initial the spi nand device driver to kernel.
* @author luoyongchuang
* @date 2016-03-17
* @param[in] mtd	  spi device handle.
* @return int return write success or failed
* @retval returns zero on success
* @retval return a non-zero error code if failed
*/
static int __devinit ak_spinand_probe(struct spi_device *spi)
{

	unsigned										i;
	unsigned										ret = 0;
	struct flash_info						*info;
	struct ak_spinand						*flash;	
	struct flash_platform_data	*data;	
	
	printk("ak spinandflash probe enter.\n");
	
	/* Platform data helps sort out which chip type we have, as
	 * well as how this board partitions it.  If we don't have
	 * a chip ID, try the JEDEC id commands; they'll work for most
	 * newer chips, even if we don't recognize the particular chip.
	 */
	 
	data = spi->dev.platform_data;
	info = get_flash_info(spi);
	if (!info)
		return -ENODEV;

	flash = kzalloc(sizeof *flash, GFP_KERNEL);
	if (!flash)
		return -ENOMEM;

  memset(flash, 0, sizeof *flash);
  
	ak_mtd_info = &flash->mtd;

	flash->spi = spi;
	flash->info = *info;
	mutex_init(&flash->lock);
	dev_set_drvdata(&spi->dev, flash);
	
	sflash_reset(flash);

	if (data && data->name)
		flash->mtd.name = data->name;
	else
		flash->mtd.name = dev_name(&spi->dev);

	flash->mtd.type = MTD_NANDFLASH;
	flash->mtd.writesize = flash->info.page_size;
	flash->mtd.flags = MTD_WRITEABLE;
	flash->mtd.size = info->block_size * info->n_blocks;
	flash->mtd._erase = ak_spinand_erase;
	flash->mtd._write = _ak_spinand_write;
	flash->mtd._read = _ak_spinand_read;
	flash->mtd._read_oob = ak_spinand_read_oob;
	flash->mtd._write_oob = ak_spinand_write_oob;
	flash->mtd._block_isbad = ak_spinand_block_isbad;
	flash->mtd._block_markbad = ak_spinand_block_markbad;
	flash->mtd.get_device_id = ak_spinand_get_devid;
	printk("%s, info->block_size = %d, info->n_blocks = %d\n", info->name, info->block_size, info->n_blocks);

	flash->erase_opcode = OPCODE_ERASE_BLOCK;
	flash->mtd.erasesize = info->block_size;
	flash->mtd.oobsize = info->oob_size;
	flash->mtd.oobavail = info->oob_seglen * info->oob_seg_perpage;

	flash->bus_width = data->bus_width;

	printk("flash->bus_width:%x\n", flash->bus_width);
	flash->page_shift = ffs(flash->mtd.writesize)-1;
	
#ifdef SPINAND_USE_MTD_BLOCK_LAYER
	/*pre-allocation buffer use for spi nand data transfer.*/
	flash->buf = kzalloc(FLASH_BUF_SIZE, GFP_KERNEL);
	if (!flash->buf) {
		printk("Allocate buf for spi page failed\n");
		kfree(flash);
		return -ENOMEM;
	}
#endif

	ak_spinand_init_stat_reg(flash);
	ak_spinand_cfg_quad_mode(flash);
	init_spiflash_rw_info(flash);

	flash->mtd.dev.parent = &spi->dev;

	dev_info(&spi->dev, "%s (%lld Kbytes)\n", info->name,
			(long long)flash->mtd.size >> 10);

	DEBUG(MTD_DEBUG_LEVEL0,
		"mtd .name = %s, .size = 0x%llx (%lldMiB) "
			".erasesize = 0x%.8x (%uKiB) .numeraseregions = %d\n",
		flash->mtd.name,
		(long long)flash->mtd.size, (long long)(flash->mtd.size >> 20),
		flash->mtd.erasesize, flash->mtd.erasesize / 1024,
		flash->mtd.numeraseregions);

	if (flash->mtd.numeraseregions)
		for (i = 0; i < flash->mtd.numeraseregions; i++)
			DEBUG(MTD_DEBUG_LEVEL0,
				"mtd.eraseregions[%d] = { .offset = 0x%llx, "
				".erasesize = 0x%.8x (%uKiB), "
				".numblocks = %d }\n",
				i, (long long)flash->mtd.eraseregions[i].offset,
				flash->mtd.eraseregions[i].erasesize,
				flash->mtd.eraseregions[i].erasesize / 1024,
				flash->mtd.eraseregions[i].numblocks);

	//ak_spinand_cfg_protect(true);


	/* partitions should match sector boundaries; and it may be good to
	 * use readonly partitions for writeprotected sectors (BP2..BP0).
	 */
	ret = mtd_device_parse_register(&flash->mtd, NULL, NULL, NULL, 0);
	if (ret) {
		printk("Add root MTD device failed\n");
		kfree(flash->buf);
		kfree(flash);
		return -EINVAL;
	}
#ifdef CONFIG_MTD_SPINAND_PRODUCER		
	ret = ak_mount_partitions(spi);
	if (ret)
		printk("Add MTD partitions failed\n");
#endif
    printk("Init AK SPI Flash finish.\n"); 

	return 0;
}

/**
* @brief	  spi nand remove
* 
* Remove the spi nand device driver from kernel.
* @author lixinhai
* @date 2014-03-20
* @param[in] mtd	   spi device handle.
* @return int return write success or failed
* @retval returns zero on success
* @retval return a non-zero error code if failed
*/
static int __devexit ak_spinand_remove(struct spi_device *spi)
{
	struct ak_spinand	*flash = dev_get_drvdata(&spi->dev);
	int		status;
	
	status = mtd_device_unregister(&flash->mtd);
/*	
	if (mtd_has_partitions() && flash->partitioned)
		status = del_mtd_partitions(&flash->mtd);

	status = del_mtd_device(&flash->mtd);
*/
	if (status == 0) {
		kfree(flash->buf);
		kfree(flash);
	}
	return 0;
}


static struct spi_driver ak_spinand_driver = {
	.driver = {
		.name	= "ak-spiflash",
		.bus	= &spi_bus_type,
		.owner	= THIS_MODULE,
	},
	.probe	= ak_spinand_probe,
	.remove	= __devexit_p(ak_spinand_remove),

	/* REVISIT: many of these chips have deep power-down modes, which
	 * should clearly be entered on suspend() to minimize power use.
	 * And also when they're otherwise idle...
	 */
};

/**
* @brief spi nand device init
* 
*  Moudle initial.
* @author luoyongchuang
* @date 2016-05-17
* @return int return write success or failed
* @retval returns zero on success
* @retval return a non-zero error code if failed
*/
static int __init ak_spinand_init(void)
{
	printk("init Anyka SPI Nand Flash driver\n");
	return spi_register_driver(&ak_spinand_driver);
}


/**
* @brief spi nand device exit
* 
*  Moudle exit.
* @author luoyongchuang
* @date 2016-05-17
* @return None
*/
static void __exit ak_spinand_exit(void)
{
	printk("exit Anyka SPI Nand Flash driver\n");	
	spi_unregister_driver(&ak_spinand_driver);
}


module_init(ak_spinand_init);
module_exit(ak_spinand_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luoyongchuang");
MODULE_DESCRIPTION("MTD SPI Nand flash driver for Anyka chips");

