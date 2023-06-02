/*
    @file       nvt_flash_spi_int.h

    @brief      NVT Flash SPI internal header file

    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.

*/

#ifndef _NVT_FLASH_SPI_INT_H
#define _NVT_FLASH_SPI_INT_H

#include "nvt_flash_spi_reg.h"
#include <linux/types.h>
#include <linux/mtd/rawnand.h>
#include <linux/mtd/nand.h>
#include <asm/cacheflush.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <plat/top.h>
#define NAND_SETREG(info, ofs,value)	   OUTW(info->mmio_base +(ofs),(value))
#define NAND_GETREG(info, ofs)		   INW(info->mmio_base+(ofs))


#define SPINAND_PAGE_SIZE           (2048)
#define SPINAND_SPARE_SIZE          (64)
#define SPINAND_BLOCK_SIZE          (128*1024)
#define SPINAND_DUMMY_BYTE          (0x00)
#define SPI_FLASH_16MB_BOUN         0x1000000
#define WORD_ALIGN_OFFSET           (4)
#define ECC_CORRECTED               0xB0
#define WR_QPP 0x1

typedef enum
{
	NAND_STS_BIT_0 = 0x01,
	NAND_STS_BIT_1 = 0x02,
	NAND_STS_BIT_2 = 0x04,
	NAND_STS_BIT_3 = 0x08,
	NAND_STS_BIT_4 = 0x10,
	NAND_STS_BIT_5 = 0x20,
	NAND_STS_BIT_6 = 0x40,
	NAND_STS_BIT_7 = 0x80,
} NAND_STATUS_CHECK_BIT;

typedef enum
{
	NAND_STS_BIT_0_EQUAL_0= 0x00000000,
	NAND_STS_BIT_0_EQUAL_1= 0x00000001,

	NAND_STS_BIT_1_EQUAL_0= 0x00000000,
	NAND_STS_BIT_1_EQUAL_1= 0x00000002,

	NAND_STS_BIT_2_EQUAL_0= 0x00000000,
	NAND_STS_BIT_2_EQUAL_1= 0x00000004,

	NAND_STS_BIT_3_EQUAL_0= 0x00000000,
	NAND_STS_BIT_3_EQUAL_1= 0x00000008,

	NAND_STS_BIT_4_EQUAL_0= 0x00000000,
	NAND_STS_BIT_4_EQUAL_1= 0x00000010,

	NAND_STS_BIT_5_EQUAL_0= 0x00000000,
	NAND_STS_BIT_5_EQUAL_1= 0x00000020,

	NAND_STS_BIT_6_EQUAL_0= 0x00000000,
	NAND_STS_BIT_6_EQUAL_1= 0x00000040,

	NAND_STS_BIT_7_EQUAL_0= 0x00000000,
	NAND_STS_BIT_7_EQUAL_1= 0x00000080,
} NAND_STATUS_CHECK_BIT_VALUE;

typedef enum
{
	NANDCTRL_ONFI_NAND_TYPE = 0x0,
	NANDCTRL_SPI_NAND_TYPE,
	NANDCTRL_SPI_NOR_TYPE,
	SPICTRL_SPI_NAND_TYPE,

	NAND_TYPE_NUM,
} NAND_TYPE_SEL;

typedef enum {
	NAND_SPI_STS_FEATURE_1 = 0,
	NAND_SPI_STS_FEATURE_2,
	NAND_SPI_STS_FEATURE_3,
	NAND_SPI_STS_FEATURE_4,

	ENUM_DUMMY4WORD(NAND_SPI_STATUS_SET)
} NAND_SPI_STATUS_SET;

typedef enum
{
	NAND_SPI_CS_ACTIVE_LOW= 0,
	NAND_SPI_CS_ACTIVE_HIGH,
} NAND_SPI_CS_POLARITY;

typedef enum
{
	NAND_SPI_CS_AUTO_MODE= 0,
	NAND_SPI_CS_MANUAL_MODE,
} NAND_SPI_CS_OP_MODE;

typedef enum
{
	NAND_SPI_CS_LOW= 0,
	NAND_SPI_CS_HIGH,
} NAND_SPI_CS_LEVEL;

typedef enum
{
	NAND_SPI_NORMAL_DUPLEX_MODE = 0,    //1bit
	NAND_SPI_DUAL_MODE,
	NAND_SPI_QUAD_MODE,
} NAND_SPI_BUS_WIDTH;

typedef enum
{
	NAND_SPI_ORDER_MODE_0 = 0,          //dual: IO1->IO0->IO1->IO0
					//quad: IO3->IO2->IO1->IO0->IO3->IO2 ...
	NAND_SPI_ORDER_MODE_1,              //dual: IO0->IO1->IO0->IO1
					//quad: IO0->IO1->IO2->IO3->IO0->IO1 ...
} NAND_SPI_IO_ORDER;

typedef enum
{
	NANDCTRL_SPIFLASH_USE_INTERNAL_RS_ECC = 0x0,
	NANDCTRL_SPIFLASH_USE_ONDIE_ECC,
} NAND_ECC_SEL;

typedef enum
{
	NAND_1_ADDRESS_CYCLE_CNT = 0x0,
	NAND_ROW_ADDRESS_1_CYCLE_CNT = NAND_1_ADDRESS_CYCLE_CNT,
	NAND_COL_ADDRESS_1_CYCLE_CNT = NAND_1_ADDRESS_CYCLE_CNT,

	NAND_2_ADDRESS_CYCLE_CNT = 0x1,
	NAND_ROW_ADDRESS_2_CYCLE_CNT = NAND_2_ADDRESS_CYCLE_CNT,
	NAND_COL_ADDRESS_2_CYCLE_CNT = NAND_2_ADDRESS_CYCLE_CNT,

	NAND_3_ADDRESS_CYCLE_CNT = 0x2,
	NAND_ROW_ADDRESS_3_CYCLE_CNT = NAND_3_ADDRESS_CYCLE_CNT,
	NAND_COL_ADDRESS_3_CYCLE_CNT = NAND_3_ADDRESS_CYCLE_CNT,

	NAND_4_ADDRESS_CYCLE_CNT = 0x3,
	NAND_ROW_ADDRESS_4_CYCLE_CNT = NAND_4_ADDRESS_CYCLE_CNT,
	NAND_COL_ADDRESS_4_CYCLE_CNT = NAND_4_ADDRESS_CYCLE_CNT,

	NAND_NOT_CONFIGED_ADDRESS_CYCLE_CNT = 0x4,
	NAND_NOT_CONFIGED_ROW_ADDRESS_CYCLE_CNT = NAND_NOT_CONFIGED_ADDRESS_CYCLE_CNT,
	NAND_NOT_CONFIGED_COL_ADDRESS_CYCLE_CNT = NAND_NOT_CONFIGED_ADDRESS_CYCLE_CNT,
} NAND_ADDRESS_CYCLE_CNT;

typedef enum
{
	NAND_PAGE_SIZE_512 = 0x0,
	NAND_PAGE_SIZE_2048 = 0x3,
	NAND_PAGE_SIZE_4096 = 0x7,
} NAND_PAGE_SIZE;


typedef enum {
	NANDCTRL_SPIFLASH_USE_1_BIT = 0x0,
	NANDCTRL_SPIFLASH_USE_2_BIT,
	NANDCTRL_SPIFLASH_USE_4_BIT,

	ENUM_DUMMY4WORD(NAND_SPINAND_BIT_SEL)
} NAND_SPINAND_BIT_SEL;

typedef enum
{
	SPI_NOR_NORMAL_READ = 0,
	SPI_NOR_DUAL_READ = 1,
	SPI_NOR_QUAD_READ = 2,

	ENUM_DUMMY4WORD(NAND_SPI_NOR_READ_MODE)
} NAND_SPI_NOR_READ_MODE;

struct spi_nand_sts {
	int32_t quad_en;
	int32_t quad_program;
	int32_t block_unlocked;
	uint32_t timer_record;
};

struct nvt_nand_flash {
	//const struct nvt_nand_timing *timing; /* NAND Flash timing */

	uint32_t page_per_block;/* Pages per block (PG_PER_BLK) */
	uint32_t page_size;	    /* Page size in bytes (PAGE_SZ) */
	uint32_t oob_size;	    /* Spare size	*/
	uint32_t subpage_size;	/* Subpage size in bytes */
	uint32_t flash_width;	/* Width of Flash memory */
	uint32_t nfc_width;	    /* Width of Flash controller  */
	uint32_t num_blocks;	/* Number of physical blocks in Flash */
	uint32_t chip_id;
	uint32_t phy_page_ratio;/* Physical pages in a block. */
	uint32_t device_size;	/* NAND flash capacity	*/
	uint32_t chip_sel;	    /* NAND flash chip select */
	uint32_t block_size;	/* NAND flash block size	*/
	uint32_t module_config; /* NAND controller configuration*/
	uint32_t nor_quad_support; /*if nor flash support quad mode*/
	NAND_SPI_NOR_READ_MODE nor_read_mode; /*normal/dual/quad read*/
	int plane_flags;        /*Flags to distinguish dual planes flash*/
	struct spi_nand_sts spi_nand_status;
};

struct drv_nand_dev_info {
	struct nand_chip        nand_chip;

	struct platform_device  *pdev;
	struct nvt_nand_flash   *flash_info;

	struct clk              *clk;
	void __iomem            *mmio_base;

	unsigned int            buf_start;
	unsigned int            buf_count;


	unsigned char           *data_buff;
	size_t                  data_buff_size;
	dma_addr_t              data_desc_addr;

	/* saved column/page_addr during CMD_SEQIN */
	int                     seqin_column;
	int                     seqin_page_addr;

	/* relate to the command */
	unsigned int            state;

	int                     use_ecc;    /* use HW ECC ? */
	int                     use_dma;    /* use DMA ? */

	size_t                  data_size;  /* data size in FIFO */
	int                     retcode;
	struct completion       cmd_complete;
	unsigned int            ecc_corrected; /* ecc corrected bit for regsiter */

	/* calculated from nt72568_nand_flash data */
	size_t                  oob_size;
	size_t                  read_id_bytes;

	unsigned int            col_addr_cycles;
	unsigned int            row_addr_cycles;
	uint32_t                nand_int_status;
	int                     irq;
	struct mutex            lock;
	u32                     flash_freq;
	u32                     trace_stdtable;
	u8                      hspeed_dummy_cycle;

    /* W25Q256FV not support 0x12 cmd, need ues 4byte addr mode*/
    int                     enter_4byte_addr_mode;
};

#define SRAMRDY_DELAY           5
#ifndef CONFIG_SYS_NAND_BASE
#define CONFIG_SYS_NAND_BASE    IOADDR_NAND_REG_BASE
#endif

#define CACHE_LINE_SIZE         64
#define TOSHIBA_TC58CVG         0xC298
#define TOSHIBA_TC58CVG2        0xED98
#define MXIC25L25635F_SFDP      0xF3
#define MXIC_MX35LF2G14AC       0x20C2
#define MICRON_MT29F2G01AB      0x242C
#define WINBOND_W25N02KV        0x22AAEF
#define WINBOND_W25Q256FV       0x1940EF
#define MICRON_MT29F1G01AA      0x122C  /* ecc only corrected 1 bit */
/* vendor ID */
#define MFG_ID_MICRON           0x2C
#define MFG_ID_WINBOND          0xEF
#define MFG_ID_GD               0xC8
#define MFG_ID_ESMT             0xC8
#define MFG_ID_ATO              0x9B
#define MFG_ID_MXIC             0xC2
#define MFG_ID_ETRON            0xD5
#define MFG_ID_TOSHIBA          0x98
#define MFG_ID_DOSILICON        0xE5
#define MFG_ID_XTX              0x0B

#define	EPERM                   1     /* Operation not permitted */
#define	ENOENT                  2     /* No such file or directory */
#define	ESRCH                   3     /* No such process */
#define	EINTR                   4     /* Interrupted system call */
#define	EIO                     5     /* I/O error */
#define	ENXIO                   6     /* No such device or address */
#define	E2BIG                   7     /* Argument list too long */
#define	ENOEXEC                 8     /* Exec format error */
#define	EBADF                   9     /* Bad file number */
#define	ECHILD                 10     /* No child processes */
#define	EAGAIN                 11     /* Try again */
#define	ENOMEM                 12     /* Out of memory */
#define	EACCES                 13     /* Permission denied */
#define	EFAULT                 14     /* Bad address */
#define	ENOTBLK                15     /* Block device required */
#define	EBUSY                  16     /* Device or resource busy */
#define	EEXIST                 17     /* File exists */
#define	EXDEV                  18     /* Cross-device link */
#define	ENODEV                 19     /* No such device */
#define	ENOTDIR                20     /* Not a directory */
#define	EISDIR                 21     /* Is a directory */
#define	EINVAL                 22     /* Invalid argument */
#define	ENFILE                 23     /* File table overflow */
#define	EMFILE                 24     /* Too many open files */
#define	ENOTTY                 25     /* Not a typewriter */
#define	ETXTBSY                26     /* Text file busy */
#define	EFBIG                  27     /* File too large */
#define	ENOSPC                 28     /* No space left on device */
#define	ESPIPE                 29     /* Illegal seek */
#define	EROFS                  30     /* Read-only file system */
#define	EMLINK                 31     /* Too many links */
#define	EPIPE                  32     /* Broken pipe */
#define	EDOM                   33     /* Math argument out of domain of func */
#define	ERANGE                 34     /* Math result not representable */

// ONFI NAND size definition
#define NAND_2M                0xEA
#define NAND_4M                0xE3
#define NAND_8M                0xE6
#define NAND_16M               0x73
#define NAND_32M               0x75
#define NAND_64M               0x76
#define NAND_128M              0x79
#define NAND_256M              0x71
#define NAND_512M              0xDC
#define NAND_1G                0xD3
#define NAND_2G                0xD5
#define NAND_128M_2K           0xF1
#define NAND_256M_2K           0xDA

// SPI NAND size definition
#define ESMT_SPI_NAND_512Mb             0x20
#define ESMT_SPI_NAND_1Gb               0x21
#define WINBOND_SPI_NAND_1024Mb         0xAA
#define WINBOND_SPI_NAND_2048Mb         0xAB
#define GD_SPI_NAND_1Gb                 0xF1
#define MICRON_SPI_NAND_1Gb             0x1C
#define GD_SPI_NAND_5F1GQ4U             0xD1
#define ATO_SPI_NAND_1Gb                0x12
#define MXIC_SPI_NAND_1Gb               0x12

#define SPIFLASH_MAX_PROGRAM_BYTE_AT_ONCE   (256)
//# Request DE modify maximun dma transfer size expend to 29bit(now = 16bit max = 0x10000 - 1)
#define SPIFLASH_MAX_READ_BYTE_AT_ONCE      (0x3F00000)
#define SPI_BLOCK_SIZE                      (0x10000)
#define SPI_SECTOR_SIZE                     (0x1000)
#define SPI_PAGE_PROGRAM_TIME               (3)     // max 3ms per page
#define SPI_ERASE_SECTOR_TIME               (300)   // max 300ms per sector
#define SPI_ERASE_BLOCK_TIME                (2000000)   // max 2sec per block
#define SPI_ERASE_CHIP_TIME                 (4294000)   // max 100sec per CHIP, but time max timeout is 4294 sec
#define SPINAND_CONFIG_QUADEN               (1<<0)  // GD/MXIC only
#define SPINAND_CONFIG_ECCEN                (1<<4)  // Enable ECC-ON
#define SPINAND_CONFIG_OTP_PRT              (1<<7)  // Enable OTP_PRT

// SMC manufacture ID definition
/*
    @name ONFI NAND device ID
*/
//@{
#define NAND_SAMSUNG                    0xEC
#define NAND_TOSHIBA                    0x98
#define NAND_HYNIX                      0xAD
#define NAND_MXIC                       0xC2
#define NAND_STM                        0x20
#define NAND_SPANSION                   0x01
#define NAND_ATO                        0x9B
#define NAND_ESMT                       0x92
#define NAND_WINBOND                    0xEF
//@}

/*
    @name SPI NAND device ID
*/
#define SPI_NAND_ESMT                   0xC8
#define SPI_NAND_GD                     0xC8
#define SPI_NAND_WINBOND                0xEF
#define SPI_NAND_MICRON                 0x2C


//Commands for Winbond's serial flash
#define FLASH_CMD_WREN                   0x06            //FLASH CMD WREN
#define FLASH_CMD_WRDI                   0x04            //FLASH CMD WRDI
#define FLASH_CMD_RDSR                   0x05            //FLASH CMD RDSR
#define FLASH_CMD_WRSR                   0x01            //FLASH CMD WRSR
#define FLASH_CMD_WRSR2                  0x31            //FLASH CMD WRSR2
#define FLASH_CMD_RDSR2                  0x35            //FLASH CMD RDSR2
#define FLASH_CMD_READ                   0x03            //FLASH CMD READ
#define FLASH_CMD_READ_4BYTE             0x13            //FLASH CMD READ with 4Byte address
#define FLASH_CMD_FASTREAD               0x0B            //FLASH CMD FAST READ
#define FLASH_CMD_DUAL_READ              0x3B            //FLASH CMD DUAL READ
#define FLASH_CMD_DUAL_READ_4BYTE        0x3C            //FLASH CMD DUAL with 4Byte address
#define FLASH_CMD_QUAD_READ_NORMAL       0x6B            //FLASH CMD FAST READ normal (address 1bit)
#define FLASH_CMD_QUAD_READ_NORMAL_4BYTE 0x6C            //FLASH CMD FAST READ normal (address 1bit) with 4Byte address
#define FLASH_CMD_QUAD_READ              0xEB            //FLASH CMD QUAD READ
#define FLASH_CMD_PAGEPROG               0x02            //FLASH CMD PAGE PROGRAM
#define FLASH_CMD_PAGEPROG_4BYTE         0x12            //FLASH CMD PAGE PROGRAM with 4byte address
#define FLASH_CMD_PAGEPROG_4X            0x32            //FLASH CMD PAGE PROGRAM 4X
#define FLASH_CMD_PAGEPROG_4X_4BYTE      0x34            //FLASH CMD PAGE PROGRAM 4X with 4byte adress
#define FLASH_CMD_SECTOR_ERASE           0x20            //FLASH CMD SECTOR ERASE
#define FLASH_CMD_SECTOR_ERASE_4BYTE     0x21            //FLASH CMD SECTOR ERASE with 4Byte address
#define FLASH_CMD_BLOCK_ERASE            0xD8            //FLASH CMD BLOCK ERASE
#define FLASH_CMD_BLOCK_ERASE_4BYTE      0xDC            //FLASH CMD BLOCK ERASE 4byte address
#define FLASH_CMD_CHIP_ERASE             0xC7            //FLASH CMD CHIP ERASE
#define FLASH_CMD_POWER_UP               0xAB            //Release power down
#define FLASH_CMD_POWER_DOWN             0xB9            //Power down
#define FLASH_CMD_RD_ALL_ID              0x90            //Read manufacture and device ID
#define FLASH_CMD_RD_JEDEC_ID            (0x9Ful)        //Read JEDEC ID
#define FLASH_CMD_RD_SFDP                (0x5Aul)        //Read SFDP

#define FLASH_CMD_AAI                    (0xADul)        //(AAI)Single-WORD program
#define FLASH_CMD_EWSR                   (0x50)          //Enable Write Status Register
#define FLASH_CMD_ENTER_4BYTE_ADDR       0xB7            //FLASH CMD Enter 4-byte address mode
#define FLASH_CMD_LEAVE_4BYTE_ADDR       0xE9            //FLASH CMD Leave 4-byte address mode

// ------------------------------------------------------------------------
// memory module 0,1,2,3,4 configuration register (0x00,0x04,0x08,0x0C,0x10)
// ------------------------------------------------------------------------
#define NAND_1COL_ADDR          0x00000000  // bit[0]
#define NAND_2COL_ADDR          0x00000001  // bit[0]
#define NAND_3COL_ADDR          0x00000002  // bit[0]
#define NAND_1ROW_ADDR          0x00000000  // bit[3:2]
#define NAND_2ROW_ADDR          0x00000004  // bit[3:2]
#define NAND_3ROW_ADDR          0x00000008  // bit[3:2]
#define NAND_4ROW_ADDR          0x0000000C  // bit[3:2]

#define NAND_PAGE512            0x00000000  // bit[7:4] = 0
#define NAND_PAGE2K             0x00000030  // bit[7:4] = 3
#define NAND_PAGE4K             0x00000070  // bit[7:4] = 7
#define NAND_PRI_ECC_DIS        0x00000000  // bit[9:8] = 0
#define NAND_PRI_ECC_HAM        0x00000100  // bit[9:8] = 1
#define NAND_PRI_ECC_RS         0x00000200  // bit[9:8] = 2
#define NAND_PRI_ECC_SPI_ON_DIE 0x00000300  // bit[9:8] = 3
#define NAND_SEC_ECC_DIS        0x00000000  // bit[11:10] = 0
#define NAND_SEC_ECC_MTH1       0x00000400  // bit[11:10] = 1
#define NAND_SEC_ECC_MTH2       0x00000800  // bit[11:10] = 2

                                            // bit[15:12] = Reserved
#define NAND_DLY_LATCH_0_5T     0x00010000  // bit[18:16] = 1 : 0.5T / 2 : 1T / 3 : 1.5T / 4 : 2T
#define NAND_DLY_LATCH_1_0T     0x00010000  // bit[18:16] = 1 : 0.5T / 2 : 1T / 3 : 1.5T / 4 : 2T
#define NAND_DLY_LATCH_1_5T     0x00010000  // bit[18:16] = 1 : 0.5T / 2 : 1T / 3 : 1.5T / 4 : 2T
#define NAND_DLY_LATCH_2_0T     0x00010000  // bit[18:16] = 1 : 0.5T / 2 : 1T / 3 : 1.5T / 4 : 2T

#define NAND_RS_ECC_FIELD_RSFT0 0x00000000  // bit[15:12] = 0~15 right shift n bit of rs ecc field(default from 518 ~ 527)
#define NAND_RS_ECC_FIELD_RSFT1 0x00010000  // bit[15:12] = 0~15 right shift n bit of rs ecc field(default from 518 ~ 527)
#define NAND_RS_ECC_FIELD_RSFT2 0x00020000  // bit[15:12] = 0~15 right shift n bit of rs ecc field(default from 518 ~ 527)


#define	NAND_SPI_NAND_TYPE      0x00000000  // bit[20] = 1 : SPI NOR, 0 : SPI NAND
#define	NAND_SPI_NOR_TYPE       0x00100000  // bit[20] = 1 : SPI NOR, 0 : SPI NAND

struct smc_setup_trans {
	uint32_t cs;                /* which chip select*/
	uint32_t type;              /* controller operation command*/
	uint32_t col_addr;          /* column address*/
	uint32_t row_addr;          /* row address*/
	uint32_t fifo_dir;          /* FIFO direct*/
	uint32_t trans_mode;        /* Use DMA / PIO mode*/
};


// ------------------------------------------------------------------------
// Control register 1 (0x20)
// ------------------------------------------------------------------------
#define _DATA_RD1                       0x00000000
#define _DATA_RD2                       0x00000001
#define _DATA_WR1                       0x00000002
#define _DATA_WR2                       0x00000003
#define _BLK_ERSE                       0x00000005
#define _READ_ID                        0x00000006
#define _CMD                            0x00000007
#define _WR_DATA                        0x00000008
#define _RD_DATA                        0x00000009
#define _1ADDR                          0x0000000a
#define _CMD_STS                        0x0000000b
#define _CMD_ADDR                       0x0000000c
#define _WR_CMD                         0x0000000d
#define _BLK_CACHE_PRO                  0x0000000e
#define _CP_BK_WR                       0x0000000f
#define _CMD_ADDR_CMD                   0x00000010
#define _CMD_RDATA                      0x00000011
#define _WR_512_SPARE                   0x00000013
#define _SPI_PROG_CMD                   0x00000015
#define _DATA_RD3                       0x00000016
#define _SPI_MULTI_PAGE_PROG_CMD        0x00000017
#define _SPI_READ_N_BYTES               0x00000018
#define _SPI_WRITE_N_BYTES              0x00000019
#define _SPI_CMD_ADDR_DATA_CMD          0x0000001C
#define _AUTO_STATUS                    0x0000001D
#define _DUMMY_CLOCK                    0x0000001E
#define NAND_CS0_EN                     0x00000000      // bit[8]
#define NAND_CS1_EN                     0x00000100      // bit[8]
                                                        // bit[11:9]
#define _OPER_EN                        0x00001000      // bit[12]
#define _NAND_WP_EN                     0x00002000      // bit[13] = 1
#define _NAND_WP_DIS                    0x00000000      // bit[13] = 0
#define _XD_WP_EN                       0x00004000      // bit[14] = 1
#define _XD_WP_DIS                      0x00000000      // bit[14] = 0
#define _SOFT_RESET                     0x00008000      // bit[15] = 1
#define _MULTI_PAGE                     0x00010000      // bit[18:16] = 1
#define _MULTI_SPARE                    0x00020000      // bit[18:16] = 2
#define _SINGLE_PAGE                    0x00000000
#define _MULTI_PAGE_MASK                0x00070000      // bit[18:16] = 1
#define _TOUT_EN                        0x00080000      // bit[19] = 1
#define _PROTECT1_EN                    0x00100000      // bit[20] = 1
#define _PROTECT2_EN                    0x00200000      // bit[21] = 1
#define _PAGE_NUM_MASK                  0xFFF00000      // bit[31:20] = 1
#define _PAGE_NUM_SFT                   20


// ------------------------------------------------------------------------
// Data Length register (0x28)
// ------------------------------------------------------------------------
#define _SPARE_ACCESS_EN  0x00000001  // bit[0] = 1
#define _ERROR_ACCESS_EN  0x00000002  // bit[1] = 1


// ------------------------------------------------------------------------
// Timing register 0 (0x2c)
// ------------------------------------------------------------------------
#define _TRP             0x00000002  // bit[3:0]
#define _TREH            0x00000020  // bit[7:4]
#define _TWP             0x00000200  // bit[11:8]
#define _TWH             0x00002000  // bit[15:12]
#define _TCLS            0x00020000  // bit[19:16]
#define _TCLH            0x00200000  // bit[23:20]
#define _TADL            0x07000000  // bit[27:24]
#define _TCLCH           0x00000000  // bit[31:28]


// ------------------------------------------------------------------------
// Timing register 1 (0x30)
// ------------------------------------------------------------------------
#define _TMPCEH          0x0000000F  // bit[7:0]
#define _TWB             0x00000F00  // bit[11:8]
#define _TMPRB           0x00007000  // bit[15:12]
#define _TALS            0x00000000  // bit[19:16]
#define _TALH            0x00000000  // bit[23:20]

// ------------------------------------------------------------------------
// Command register (0x34)
// ------------------------------------------------------------------------
#define _RESET_CMD              0x000000ff
#define _READ_ID_CMD            0x00000090
#define _READ_ID_91_CMD         0x00000091
#define _READ_ID_9A_CMD         0x0000009A
#define _BLK_ERASE_CMD          0x0070D060
#define _WRITE_CMD              0x00701080
#define _WRITE_SPR_CMD          0x50701080
#define _READ_CMD1              0x00000000
#define _READ_CMD2              0x00000001
#define _READ_CMD3              0x00000050
#define _READ_CMD4              0x00003000
#define _READ_CMD5              0x00003500
#define _COPY_BACK_CMD          0x00003500
#define _CACHE_POR              0x15701080
#define _CP_BK_WRITE            0x00701085
#define _CP_BK_WRITE_512        0x0000708A
#define _READ_ST_CMD            0x00000070
#define _WRITE_ST_CMD           0x00000001
#define _COPY_BACK_CMD_512      0x00000000

/*
    @name SPI NAND commands

*/
//@{
#define SPINAND_CMD_RESET               (0xFF)
#define SPINAND_CMD_JEDECID             (0x9F)
#define SPINAND_CMD_BLK_ERASE           (0xD8)
#define SPINAND_CMD_WEN                 (0x06)
#define SPINAND_CMD_WDIS                (0x04)
#define SPINAND_CMD_GET_FEATURE         (0x0F)
#define SPINAND_CMD_SET_FEATURE         (0x1F)
#define SPINAND_CMD_PROGRAM_LOAD        (0x02)
#define SPINAND_CMD_PROGRAM_LOADX4      (0x32)
#define SPINAND_CMD_PROGRAM_EXE         (0x10)
#define SPINAND_CMD_PAGE_READ           (0x13)
#define SPINAND_CMD_CACHE_READ          (0x03)
#define SPINAND_CMD_CACHE_READX2        (0x3B)
#define SPINAND_CMD_CACHE_READX4        (0x6B)
#define SPINAND_CMD_MXIC_READ_ECC       (0x7C)
//@}

/*
    @name SPI NAND FEATURE offset

    @note For SPINAND_CMD_GET_FEATURE, SPINAND_CMD_SET_FEATURE
*/
//@{
#define SPINAND_FEATURE_BLOCK_LOCK              (0xA0)
#define SPINAND_FEATURE_OPT                     (0xB0)
#define SPINAND_FEATURE_STATUS                  (0xC0)
#define SPINAND_FEATURE_STATUS_PROG_FAIL        (0x08)
#define SPINAND_FEATURE_STATUS_ERASE_FAIL       (0x04)
#define SPINAND_FEATURE_STATUS_ECC_ERR          (0x10)
#define SPINAND_FEATURE_STATUS_ECC_UNCORR_ERR   (0x20)
#define SPINAND_FEATURE_STATUS_ECC_MASK         (0x30)  /* ecc_status[1..0] */

// ------------------------------------------------------------------------
//  Time out register (0x40)
// ------------------------------------------------------------------------
#define _TOUT_TRIGGER 0x3FFFFFFF

// ------------------------------------------------------------------------
// Interrupt enabled register (0x44)
// ------------------------------------------------------------------------
#define NAND_COMP_INTREN        0x00001000  // bit[12]
#define NAND_PRI_ECC_INTREN     0x00002000  // bit[13]
#define NAND_STS_FAIL_INTREN    0x00004000  // bit[14]
#define NAND_TOUT_INTREN        0x00008000  // bit[15]
#define NAND_SEC_ECC_INTREN     0x00010000  // bit[16]
#define NAND_PROTECT1_INTREN    0x00020000  // bit[17]
#define NAND_PROTECT2_INTREN    0x00040000  // bit[18]
#define NAND_INTEN_ALL          (NAND_COMP_INTREN | NAND_PRI_ECC_INTREN | NAND_STS_FAIL_INTREN | NAND_TOUT_INTREN | NAND_SEC_ECC_INTREN | NAND_PROTECT1_INTREN | NAND_PROTECT2_INTREN)
// ------------------------------------------------------------------------
// Controller status register (0x48)
// ------------------------------------------------------------------------
#define NAND_SM_STATUS          0x000000FF  // bit[7:0]
#define NAND_SM_BUSY            0x00000100  // bit[8]
#define NAND_COMP_STS           0x00001000  // bit[12]
#define NAND_PRI_ECC_STS        0x00002000  // bit[13]
#define NAND_STS_FAIL_STS       0x00004000  // bit[14]
#define NAND_TOUT_STS           0x00008000  // bit[15]
#define NAND_SEC_ECC_STS        0x00010000  // bit[16]
#define NAND_PROTECT1_STS       0x00020000  // bit[17]
#define NAND_PROTECT2_STS       0x00040000  // bit[26]


// ------------------------------------------------------------------------
// Defination about DMA Controller
// ------------------------------------------------------------------------
#define _CHA_SRC_ADDR    0x00000080
#define _CHA_DES_ADDR    0x00000084
#define _CHA_CYC_CNT     0x00000088
#define _CHA_CMD         0x0000008c
#define _CHB_SRC_ADDR    0x00000090
#define _CHB_DES_ADDR    0x00000094
#define _CHB_CYC_CNT     0x00000098
#define _CHB_CMD         0x0000009c
#define _CHC_SRC_ADDR    0x000000a0
#define _CHC_DES_ADDR    0x000000a4
#define _CHC_CYC_CNT     0x000000a8
#define _CHC_CMD         0x000000ac
#define _CHD_SRC_ADDR    0x000000b0
#define _CHD_DES_ADDR    0x000000b4
#define _CHD_CYC_CNT     0x000000b8
#define _CHD_CMD         0x000000bc
#define _DMA_INTR_EN     0x000000c0
#define _DMA_INTR_STS    0x000000c4


// ------------------------------------------------------------------------
// DMA Channel A Command register (0x8c)
// ------------------------------------------------------------------------
#define _CHA_EN_DMA             0x00000001    // bit 0
#define _CHA_DIS_DMA            0x00000000    // bit 0
#define _CHA_NO_BURST           0x00000000    // bit 3
#define _CHA_BURST_MODE         0x00000008    // bit 3
#define _CHA_SRC_APB            0x00000000    // bit 6
#define _CHA_SRC_AHB            0x00000040    // bit 6
#define _CHA_DES_APB            0x00000000    // bit 7
#define _CHA_DES_AHB            0x00000080    // bit 7
#define _CHA_SRC_NO_INC         0x00000000    // bit [10:8]
#define _CHA_SRC_BURST4         0x00000300    // bit [10:8], for no burst in bit 3
#define _CHA_SRC_BURST16        0x00000300    // bit [10:8], for burst 4 in bit 22
#define _CHA_SRC_BURST32        0x00000300    // bit [10:8], for burst 8 in bit 22
#define _CHA_DES_NO_INC         0x00000000    // bit [14:12]
#define _CHA_DES_BURST4         0x00003000    // bit [14:12], for no burst in bit 3
#define _CHA_DES_BURST16        0x00003000    // bit [14:12], for burst 4 in bit 22
#define _CHA_DES_BURST32        0x00003000    // bit [14:12], for burst 8 in bit 22
#define _CHA_WORD_TRANS         0x00000000    // bit [21:20]
#define _CHA_BURST4_SEL         0x00000000    // bit 22
#define _CHA_BURST8_SEL         0x00400000    // bit 22
#define _CHA_REQ_SM             0x0a000000    // bit [27:24]
#define _CHA_AUTO               0x00000000    // bit 28
#define _CHA_HD_MODE            0x10000000    // bit 28

// ------------------------------------------------------------------------
// ECC error Status register (0x50)
// ------------------------------------------------------------------------
#define _ECC_SEC0_F1_MSK        0x00000003
#define _ECC_SEC0_F1_1BIT       0x00000001   // bit 0
#define _ECC_SEC0_F1_ERR        0x00000002   // bit 1
#define _ECC_SEC0_F2_MSK        0x0000000c   // bit 0
#define _ECC_SEC0_F2_1BIT       0x00000004   // bit 2
#define _ECC_SEC0_F2_ERR        0x00000008   // bit 3

#define _ECC_SEC1_F1_MSK        0x00000030
#define _ECC_SEC1_F1_1BIT       0x00000010   // bit 4
#define _ECC_SEC1_F1_ERR        0x00000020   // bit 5
#define _ECC_SEC1_F2_MSK        0x000000c0   // bit 0
#define _ECC_SEC1_F2_1BIT       0x00000040   // bit 6
#define _ECC_SEC1_F2_ERR        0x00000080   // bit 7

#define _ECC_SEC2_F1_MSK        0x00000300
#define _ECC_SEC2_F1_1BIT       0x00000100   // bit 8
#define _ECC_SEC2_F1_ERR        0x00000200   // bit 9
#define _ECC_SEC2_F2_MSK        0x00000c00   // bit 0
#define _ECC_SEC2_F2_1BIT       0x00000400   // bit 10
#define _ECC_SEC2_F2_ERR        0x00000800   // bit 11

#define _ECC_SEC3_F1_MSK        0x00003000
#define _ECC_SEC3_F1_1BIT       0x00001000   // bit 12
#define _ECC_SEC3_F1_ERR        0x00002000   // bit 13
#define _ECC_SEC3_F2_MSK        0x0000c000   // bit 0
#define _ECC_SEC3_F2_1BIT       0x00004000   // bit 14
#define _ECC_SEC3_F2_ERR        0x00008000   // bit 15

#define _ECC_SEC4_F1_MSK        0x00030000
#define _ECC_SEC4_F1_1BIT       0x00010000   // bit 0
#define _ECC_SEC4_F1_ERR        0x00020000   // bit 1
#define _ECC_SEC4_F2_MSK        0x000C0000   // bit 0
#define _ECC_SEC4_F2_1BIT       0x00040000   // bit 2
#define _ECC_SEC4_F2_ERR        0x00080000   // bit 3

#define _ECC_SEC5_F1_MSK        0x00300000
#define _ECC_SEC5_F1_1BIT       0x00100000   // bit 4
#define _ECC_SEC5_F1_ERR        0x00200000   // bit 5
#define _ECC_SEC5_F2_MSK        0x00c00000   // bit 0
#define _ECC_SEC5_F2_1BIT       0x00400000   // bit 6
#define _ECC_SEC5_F2_ERR        0x00800000   // bit 7

#define _ECC_SEC6_F1_MSK        0x03000000
#define _ECC_SEC6_F1_1BIT       0x01000000   // bit 8
#define _ECC_SEC6_F1_ERR        0x02000000   // bit 9
#define _ECC_SEC6_F2_MSK        0x0c000000   // bit 0
#define _ECC_SEC6_F2_1BIT       0x04000000   // bit 10
#define _ECC_SEC6_F2_ERR        0x08000000   // bit 11

#define _ECC_SEC7_F1_MSK        0x30000000
#define _ECC_SEC7_F1_1BIT       0x10000000   // bit 12
#define _ECC_SEC7_F1_ERR        0x20000000   // bit 13
#define _ECC_SEC7_F2_MSK        0xc0000000   // bit 0
#define _ECC_SEC7_F2_1BIT       0x40000000   // bit 14
#define _ECC_SEC7_F2_ERR        0x80000000   // bit 15


// ------------------------------------------------------------------------
// ECC error Status register (0x54)
// ------------------------------------------------------------------------
#define _ECC_SEC8_F1_MSK        0x00000003
#define _ECC_SEC8_F1_1BIT       0x00000001   // bit 0
#define _ECC_SEC8_F1_ERR        0x00000002   // bit 1
#define _ECC_SEC8_F2_MSK        0x0000000c   // bit 0
#define _ECC_SEC8_F2_1BIT       0x00000004   // bit 2
#define _ECC_SEC8_F2_ERR        0x00000008   // bit 3

#define _ECC_SEC9_F1_MSK        0x00000030
#define _ECC_SEC9_F1_1BIT       0x00000010   // bit 4
#define _ECC_SEC9_F1_ERR        0x00000020   // bit 5
#define _ECC_SEC9_F2_MSK        0x000000c0   // bit 0
#define _ECC_SEC9_F2_1BIT       0x00000040   // bit 6
#define _ECC_SEC9_F2_ERR        0x00000080   // bit 7

#define _ECC_SEC10_F1_MSK       0x00000300
#define _ECC_SEC10_F1_1BIT      0x00000100   // bit 8
#define _ECC_SEC10_F1_ERR       0x00000200   // bit 9
#define _ECC_SEC10_F2_MSK       0x00000c00   // bit 0
#define _ECC_SEC10_F2_1BIT      0x00000400   // bit 10
#define _ECC_SEC10_F2_ERR       0x00000800   // bit 11

#define _ECC_SEC11_F1_MSK       0x00003000
#define _ECC_SEC11_F1_1BIT      0x00001000   // bit 12
#define _ECC_SEC11_F1_ERR       0x00002000   // bit 13
#define _ECC_SEC11_F2_MSK       0x0000c000   // bit 0
#define _ECC_SEC11_F2_1BIT      0x00004000   // bit 14
#define _ECC_SEC11_F2_ERR       0x00008000   // bit 15

#define _ECC_SEC12_F1_MSK       0x00030000
#define _ECC_SEC12_F1_1BIT      0x00010000   // bit 0
#define _ECC_SEC12_F1_ERR       0x00020000   // bit 1
#define _ECC_SEC12_F2_MSK       0x000C0000   // bit 0
#define _ECC_SEC12_F2_1BIT      0x00040000   // bit 2
#define _ECC_SEC12_F2_ERR       0x00080000   // bit 3

#define _ECC_SEC13_F1_MSK       0x00300000
#define _ECC_SEC13_F1_1BIT      0x00100000   // bit 4
#define _ECC_SEC13_F1_ERR       0x00200000   // bit 5
#define _ECC_SEC13_F2_MSK       0x00c00000   // bit 0
#define _ECC_SEC13_F2_1BIT      0x00400000   // bit 6
#define _ECC_SEC13_F2_ERR       0x00800000   // bit 7

#define _ECC_SEC14_F1_MSK       0x03000000
#define _ECC_SEC14_F1_1BIT      0x01000000   // bit 8
#define _ECC_SEC14_F1_ERR       0x02000000   // bit 9
#define _ECC_SEC14_F2_MSK       0x0c000000   // bit 0
#define _ECC_SEC14_F2_1BIT      0x04000000   // bit 10
#define _ECC_SEC14_F2_ERR       0x08000000   // bit 11

#define _ECC_SEC15_F1_MSK       0x30000000
#define _ECC_SEC15_F1_1BIT      0x10000000   // bit 12
#define _ECC_SEC15_F1_ERR       0x20000000   // bit 13
#define _ECC_SEC15_F2_MSK       0xc0000000   // bit 0
#define _ECC_SEC15_F2_1BIT      0x40000000   // bit 14
#define _ECC_SEC15_F2_ERR       0x80000000   // bit 15


// ------------------------------------------------------------------------
// ECC error Status register (0x70)
// ------------------------------------------------------------------------
#define _ECC_SEC0_MSK   0x0000000F
#define _ECC_SEC0_1BIT  0x00000001   // bit 0
#define _ECC_SEC0_2BIT  0x00000002   // bit 1
#define _ECC_SEC0_3BIT  0x00000003   // bit 0
#define _ECC_SEC0_4BIT  0x00000004   // bit 2
#define _ECC_SEC0_ERR   0x00000005   // bit 3

#define _ECC_SEC1_MSK   0x000000F0
#define _ECC_SEC1_1BIT  0x00000010   // bit 0
#define _ECC_SEC1_2BIT  0x00000020   // bit 1
#define _ECC_SEC1_3BIT  0x00000030   // bit 0
#define _ECC_SEC1_4BIT  0x00000040   // bit 2
#define _ECC_SEC1_ERR   0x00000050   // bit 3

#define _ECC_SEC2_MSK   0x00000F00
#define _ECC_SEC2_1BIT  0x00000100   // bit 0
#define _ECC_SEC2_2BIT  0x00000200   // bit 1
#define _ECC_SEC2_3BIT  0x00000300   // bit 0
#define _ECC_SEC2_4BIT  0x00000400   // bit 2
#define _ECC_SEC2_ERR   0x00000500   // bit 3

#define _ECC_SEC3_MSK   0x0000F000
#define _ECC_SEC3_1BIT  0x00001000   // bit 0
#define _ECC_SEC3_2BIT  0x00002000   // bit 1
#define _ECC_SEC3_3BIT  0x00003000   // bit 0
#define _ECC_SEC3_4BIT  0x00004000   // bit 2
#define _ECC_SEC3_ERR   0x00005000   // bit 3

#define _ECC_SEC4_MSK   0x000F0000
#define _ECC_SEC4_1BIT  0x00010000   // bit 0
#define _ECC_SEC4_2BIT  0x00020000   // bit 1
#define _ECC_SEC4_3BIT  0x00030000   // bit 0
#define _ECC_SEC4_4BIT  0x00040000   // bit 2
#define _ECC_SEC4_ERR   0x00050000   // bit 3

#define _ECC_SEC5_MSK   0x00F00000
#define _ECC_SEC5_1BIT  0x00100000   // bit 0
#define _ECC_SEC5_2BIT  0x00200000   // bit 1
#define _ECC_SEC5_3BIT  0x00300000   // bit 0
#define _ECC_SEC5_4BIT  0x00400000   // bit 2
#define _ECC_SEC5_ERR   0x00500000   // bit 3

#define _ECC_SEC6_MSK   0x0F000000
#define _ECC_SEC6_1BIT  0x01000000   // bit 0
#define _ECC_SEC6_2BIT  0x02000000   // bit 1
#define _ECC_SEC6_3BIT  0x03000000   // bit 0
#define _ECC_SEC6_4BIT  0x04000000   // bit 2
#define _ECC_SEC6_ERR   0x05000000   // bit 3

#define _ECC_SEC7_MSK   0xF0000000
#define _ECC_SEC7_1BIT  0x10000000   // bit 0
#define _ECC_SEC7_2BIT  0x20000000   // bit 1
#define _ECC_SEC7_3BIT  0x30000000   // bit 0
#define _ECC_SEC7_4BIT  0x40000000   // bit 2
#define _ECC_SEC7_ERR   0x50000000   // bit 3

// ------------------------------------------------------------------------
// ECC error Status register (0x74)
// ------------------------------------------------------------------------
#define _ECC_SEC8_MSK   0x0000000F
#define _ECC_SEC8_1BIT  0x00000001   // bit 0
#define _ECC_SEC8_2BIT  0x00000002   // bit 1
#define _ECC_SEC8_3BIT  0x00000003   // bit 0
#define _ECC_SEC8_4BIT  0x00000004   // bit 2
#define _ECC_SEC8_ERR   0x00000005   // bit 3

#define _ECC_SEC9_MSK   0x000000F0
#define _ECC_SEC9_1BIT  0x00000010   // bit 0
#define _ECC_SEC9_2BIT  0x00000020   // bit 1
#define _ECC_SEC9_3BIT  0x00000030   // bit 0
#define _ECC_SEC9_4BIT  0x00000040   // bit 2
#define _ECC_SEC9_ERR   0x00000050   // bit 3

#define _ECC_SEC10_MSK  0x00000F00
#define _ECC_SEC10_1BIT 0x00000100   // bit 0
#define _ECC_SEC10_2BIT 0x00000200   // bit 1
#define _ECC_SEC10_3BIT 0x00000300   // bit 0
#define _ECC_SEC10_4BIT 0x00000400   // bit 2
#define _ECC_SEC10_ERR  0x00000500   // bit 3

#define _ECC_SEC11_MSK  0x0000F000
#define _ECC_SEC11_1BIT 0x00001000   // bit 0
#define _ECC_SEC11_2BIT 0x00002000   // bit 1
#define _ECC_SEC11_3BIT 0x00003000   // bit 0
#define _ECC_SEC11_4BIT 0x00004000   // bit 2
#define _ECC_SEC11_ERR  0x00005000   // bit 3

#define _ECC_SEC12_MSK  0x000F0000
#define _ECC_SEC12_1BIT 0x00010000   // bit 0
#define _ECC_SEC12_2BIT 0x00020000   // bit 1
#define _ECC_SEC12_3BIT 0x00030000   // bit 0
#define _ECC_SEC12_4BIT 0x00040000   // bit 2
#define _ECC_SEC12_ERR  0x00050000   // bit 3

#define _ECC_SEC13_MSK  0x00F00000
#define _ECC_SEC13_1BIT 0x00100000   // bit 0
#define _ECC_SEC13_2BIT 0x00200000   // bit 1
#define _ECC_SEC13_3BIT 0x00300000   // bit 0
#define _ECC_SEC13_4BIT 0x00400000   // bit 2
#define _ECC_SEC13_ERR  0x00500000   // bit 3

#define _ECC_SEC14_MSK  0x0F000000
#define _ECC_SEC14_1BIT 0x01000000   // bit 0
#define _ECC_SEC14_2BIT 0x02000000   // bit 1
#define _ECC_SEC14_3BIT 0x03000000   // bit 0
#define _ECC_SEC14_4BIT 0x04000000   // bit 2
#define _ECC_SEC14_ERR  0x05000000   // bit 3

#define _ECC_SEC15_MSK  0xF0000000
#define _ECC_SEC15_1BIT 0x10000000   // bit 0
#define _ECC_SEC15_2BIT 0x20000000   // bit 1
#define _ECC_SEC15_3BIT 0x30000000   // bit 0
#define _ECC_SEC15_4BIT 0x40000000   // bit 2
#define _ECC_SEC15_ERR  0x50000000   // bit 3

// ------------------------------------------------------------------------
// DMA Interrupt Status register (0xc4)
// ------------------------------------------------------------------------
#define _CHA_FINISH     0x00000001   // bit 0
#define _CHA_ERR_RES    0x00000002   // bit 1
#define _CHB_FINISH     0x00000010   // bit 4
#define _CHB_ERR_RES    0x00000020   // bit 5
#define _CHC_FINISH     0x00000100   // bit 8
#define _CHC_ERR_RES    0x00000200   // bit 9
#define _CHD_FINISH     0x00001000   // bit 12
#define _CHD_ERR_RES    0x00002000   // bit 13

// ------------------------------------------------------------------------
// Data Length register (0x104)
// ------------------------------------------------------------------------
#define _512BYTE        0x00000200  // bit[15:0]
#define _528BYTE        0x00000210  // bit[15:0]
#define _2KBYTE         0x00000800

// ------------------------------------------------------------------------
// FIFO Status register (0x108)
// ------------------------------------------------------------------------
#define _FIFO_CNT_MASK  0x0000001F  // bit[12]
#define _FIFO_EMPTY     0x00000100  // bit[12]
#define _FIFO_FULL      0x00000200  // bit[13]


// ------------------------------------------------------------------------
// FIFO Status register (0x10C)
// ------------------------------------------------------------------------
#define _FIFO_EN        0x00000001  // bit[18] = 1
#define _PIO_MODE       0x00000000  // bit[13] = 0
#define _DMA_MODE       0x00000002  // bit[13] = 1
#define _FIFO_READ      0x00000000  // bit[14] = 0
#define _FIFO_WRITE     0x00000004  // bit[14] = 1

// ------------------------------------------------------------------------
// ECC error Status register0 (0x110)
// ------------------------------------------------------------------------
#define _ECC_SEC0_F1_ERRBYTE    0x0000FFF   // bit 0~11
#define _ECC_SEC0_F1_ERRBIT     0x000F000   // bit 13~15
#define _ECC_SEC0_F1_BYTESFT    0
#define _ECC_SEC0_F1_BITSFT     12

// ------------------------------------------------------------------------
// ECC error Status register1 (0x114)
// ------------------------------------------------------------------------
#define _ECC_SEC0_F2_ERRBYTE    0x0FFF0000   // bit 16~27
#define _ECC_SEC0_F2_ERRBIT     0xF0000000   // bit 27~31
#define _ECC_SEC0_F2_BYTESFT    16
#define _ECC_SEC0_F2_BITSFT     28

#define _ECC_SEC0_SECONDARY_ERRBYTE    0x0000FFF   // bit 0~11
#define _ECC_SEC0_SECONDARY_ERRBIT     0x000F000   // bit 13~15
#define _ECC_SEC0_SECONDARY_BYTESFT    0
#define _ECC_SEC0_SECONDARY_BITSFT     12

#define NAND_PASS_BIT   1     //D0 : 0 if Pass, 1 if Fail
#define NAND_READY_BIT  0x40  //D6 : 1 if ready, 0 if Busy

#define _WRITE_PROTECT_FLAG(x)		((x == _FIFO_READ)?  \
				    _NAND_WP_EN:   \
				    _NAND_WP_DIS)
#define GET_NAND_SPARE_SIZE(m)		((m)->phy_page_ratio << 4)
#define NAND_CHIP_SEL(m)	    (((m)->chip_sel == 0)?   \
					    NAND_CS0_EN:    \
					    NAND_CS1_EN)


#define NAND_HOST_USE_READ(ctx) 	(((ctx)->page_size == 512)?\
					(_DATA_RD1):			    \
					(_DATA_RD2))

#define NAND_CMD_READ(ctx, spare)	(((ctx)->page_size == 512)?\
					((!spare)? (_READ_CMD1): (_READ_CMD3)):\
					(_READ_CMD4))

// -----------------------------------------------------------------------------
// -- Internal usage global control funcionts --
// -----------------------------------------------------------------------------
int nand_host_set_nand_type(struct drv_nand_dev_info *info, NAND_TYPE_SEL nand_type);
int nand_host_setup_page_size(struct drv_nand_dev_info *info, NAND_PAGE_SIZE page_size);
int nand_host_setup_status_check_bit(struct drv_nand_dev_info *info, u8 status_mask, u8 status_value);
int nand_host_setup_address_cycle(struct drv_nand_dev_info *info, NAND_ADDRESS_CYCLE_CNT row, NAND_ADDRESS_CYCLE_CNT col);
uint32_t nand_host_get_multi_page_select(struct drv_nand_dev_info *info);
int nand_host_set_spi_io(struct drv_nand_dev_info *info, NAND_SPI_CS_POLARITY cs_pol, NAND_SPI_BUS_WIDTH bus_width, NAND_SPI_IO_ORDER io_order);
int spi_nand_unlock_bp(struct drv_nand_dev_info *info);
int nand_host_set_cs_active(struct drv_nand_dev_info *info, \
				NAND_SPI_CS_LEVEL level);
int nand_host_config_cs_opmode(struct drv_nand_dev_info *info, \
				NAND_SPI_CS_OP_MODE mode);

/* --	NAND command --*/
void nand_dll_reset(struct drv_nand_dev_info *info);
int nvt_nand_read_id(struct drv_nand_dev_info *info, uint32_t *id);

int nand_cmd_reset(struct drv_nand_dev_info *info);
int nand_cmd_wait_complete(struct drv_nand_dev_info *info);
int nand_cmd_read_status(struct drv_nand_dev_info *info, uint32_t set);

int nand_cmd_read_id(uint8_t *card_id, struct drv_nand_dev_info *info);
int nand_cmd_erase_block(struct drv_nand_dev_info *info,
						uint32_t block_address);
int nand_cmd_read_operation(struct drv_nand_dev_info *info, int8_t * buffer,
					uint32_t pageAddress, uint32_t numPage);
int nand_cmd_write_operation_single(struct drv_nand_dev_info *info,
			uint32_t pageAddress, uint32_t column);
int nand_cmd_read_page_spare_data(struct drv_nand_dev_info *info, \
					int8_t *buffer, uint32_t page_address);



/* --	NAND host operation  --*/
void nand_host_send_command(struct drv_nand_dev_info *info, \
				uint32_t command, uint32_t bTmOutEN);
void nand_host_setup_transfer(struct drv_nand_dev_info *info, \
				struct smc_setup_trans *trans_param, \
				uint32_t pageCount, \
				uint32_t length, \
				uint32_t multi_page_select);


void nand_host_set_fifo_enable(struct drv_nand_dev_info *info, \
					uint32_t uiEN);
void nand_host_set_cpu_access_err(struct drv_nand_dev_info *info, \
					uint32_t bcpu_Acc);
int nand_host_correct_secondary_ecc(struct drv_nand_dev_info *info, \
					uint32_t uiSection);
int nand_host_correct_reedsolomon_ecc(struct drv_nand_dev_info *info, \
					uint8_t *Buf, uint32_t uiSection);


int nand_host_transmit_data(struct drv_nand_dev_info *info, \
			uint8_t *buffer, uint32_t length, uint32_t trans_mode);
int nand_host_receive_data(struct drv_nand_dev_info *info, uint8_t *buffer,\
				uint32_t length, uint32_t trans_mode);
void nand_host_set_cpu_access_spare(struct drv_nand_dev_info *info,
					uint32_t bcpu_Acc);

void nand_host_settiming2(struct drv_nand_dev_info *info, u32 time);

void nand_cmd_write_spare_sram(struct drv_nand_dev_info *info);

int nand_cmd_read_ecc_corrected(struct drv_nand_dev_info *info);

int nand_cmd_read_flash_ecc_corrected(struct drv_nand_dev_info *info);

int spinor_program_operation(struct drv_nand_dev_info *info, u32 byte_addr, \
				u32 sector_size, u8* pbuf);

int nand_cmd_write_status(struct drv_nand_dev_info *info, u32 set, u32 status);
int nand_cmd_read_config(struct drv_nand_dev_info *info, u32 config);

int spinor_read_operation(struct drv_nand_dev_info *info, u32 byte_addr, \
				u32 byte_size, u8 *pbuf);

int spinor_erase_sector(struct drv_nand_dev_info *info, u8 cmd, u32 addr);

int spinor_read_sfdp(struct drv_nand_dev_info *info, u32 addr, u32 byte_size, u8 *pbuf);

void nand_phy_config(struct drv_nand_dev_info *info);

void flash_copy_info(struct drv_nand_dev_info *info);

int enter_4byte_address(struct drv_nand_dev_info *info);
int leave_4byte_address(struct drv_nand_dev_info *info);

#endif
