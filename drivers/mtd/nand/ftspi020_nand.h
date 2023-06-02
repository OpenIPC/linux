/**
 * Command arguments definition for FTSPI020.
 *
 * Author: Justin Shih <wdshih@faraday-tech.com> 
 *
 * Copyright (c) 2013, Faraday Technology Corp.
 *
 */  
    
/* FIX_FLOW_INDEX for ONFI change mode */ 
#define		ONFI_FIXFLOW_GETFEATURE		0x34B
#define		ONFI_FIXFLOW_SETFEATURE		0x352
#define		ONFI_FIXFLOW_SYNCRESET		0x33A
    
#define		MAX_CE				1       //4
#define		MAX_CHANNEL			1       //2
//#define               NAND_USE_AHBDMA_CHANNEL         0
    
/*
 * Settings value required to send command.
 */ 
    typedef struct ftspi020_cmd {
    char flags;
    
        /* offset 0x00: CMD Queue first word */ 
     u32 spi_addr;
    
        /* offset 0x04: CMD Queue second word */ 
     u32 addr_len;
     u32 dum_2nd_cyc;
     u32 ins_len;
     u32 conti_read_mode_en;
    
        /* offset 0x08: CMD queue third word */ 
     u32 data_cnt;
    
        /* offset 0x0C: CMD queue fourth word */ 
     u32 intr_en;
     u32 write_en;
     u32 read_status_en;
     u32 read_status;
     u32 dtr_mode;
     u32 spi_mode;
     u32 start_ce;
     u32 conti_read_mode_code;
     u32 ins_code;
     
        /* User data buffer pointer */ 
    const void *tx_buf;
     void *rx_buf;
 } ftspi020_cmd_t;

#ifndef CONFIG_FTNANDC023_DEBUG
#define CONFIG_FTNANDC023_DEBUG			0
#endif  /*  */
#if CONFIG_FTNANDC023_DEBUG > 0
#define ftnandc023_dbg(fmt, ...) printk(KERN_INFO fmt, ##__VA_ARGS__);
#else   /*  */
#define ftnandc023_dbg(fmt, ...) do {} while (0);
#endif  /*  */
    
#if CONFIG_FTNANDC023_DEBUG > 0
#define DBGLEVEL1(x)	x
#else   /*  */
#define DBGLEVEL1(x)
#endif  /*  */
    
#if CONFIG_FTNANDC023_DEBUG > 1
#define DBGLEVEL2(x)	x
#else   /*  */
#define DBGLEVEL2(x)
#endif  /*  */
typedef enum { LEGACY_SMALL = 0, LEGACY_LARGE, TOGGLE, ONFI, 
} flashtype;
struct ftnandc023_nandchip_attr {
    char *name;
     int sparesize;
     int pagesize;
     int blocksize;
     int flashsize;
     flashtype legacy;
};

/******************************************************************************
 * SPI COMMAND
 *****************************************************************************/ 
#define COMMAND_READ_ID							0x9f
#define COMMAND_WRITE_ENABLE        0x06
#define COMMAND_WRITE_DISABLE       0x04
#define COMMAND_ERASE_SECTOR        0x20
#define COMMAND_ERASE_128K_BLOCK    0xD8
#define COMMAND_ERASE_CHIP          0xC7
#define COMMAND_READ_STATUS1        0x05
#define COMMAND_READ_STATUS2        0x35
#define COMMAND_WRITE_STATUS        0x01
#define COMMAND_WRITE_PAGE          0x02
#define COMMAND_WINBOND_QUAD_WRITE_PAGE     0x32
#define COMMAND_QUAD_WRITE_PAGE     0x38
#define COMMAND_READ_DATA           0x03
#define COMMAND_FAST_READ           0x0B
#define COMMAND_FAST_READ_DUAL	    0x3B
#define COMMAND_FAST_READ_DUAL_IO   0xBB
#define COMMAND_FAST_READ_QUAD	    0x6B
#define COMMAND_FAST_READ_QUAD_IO   0xEB
#define COMMAND_WORD_READ_QUAD_IO   0xE7
#define COMMAND_READ_UNIQUE_ID	    0x4B
#define COMMAND_EN4B                0xB7        //enter 4 byte mode
#define COMMAND_EX4B                0xE9        //exit 4 byte mode
#define COMMAND_RESET               0xFF
    
/******************************************************************************
 * SPI NAND COMMAND
 *****************************************************************************/ 
#define COMMAND_ERASE_128K_BLOCK    0xD8
#define COMMAND_GET_FEATURES        0x0F
#define COMMAND_SET_FEATURES        0x1F
#define COMMAND_PAGE_READ           0x13
#define COMMAND_PROGRAM_LOAD        0x02
#define COMMAND_PROGRAM_EXECUTE     0x10
#define COMMAND_BLOCK_ERASE         0xD8
    
/* Status register 1 bits */ 
#define FLASH_STS_BUSY              (0x1 << 0)
#define FLASH_STS_WE_LATCH          (0x1 << 1)
#define FLASH_STS_REG_PROTECT0	    (0x1 << 7)
    
/* Status register 2 bits */ 
#define FLASH_STS_REG_PROTECT1	    (0x1 << 0)
#define FLASH_STS_QUAD_ENABLE       (0x1 << 6)
#define FLASH_WINBOND_STS_QUAD_ENABLE   (0x1 << 1)
/******************************************************************************
 * SPI020 definitions
 *****************************************************************************/ 
#define	FTSPI020_REG_CMD0		0x00	/* Flash address */
#define	FTSPI020_REG_CMD1		0x04
#define	FTSPI020_REG_CMD2		0x08
#define	FTSPI020_REG_CMD3		0x0c
#define	FTSPI020_REG_CTRL		0x10	/* Control */
#define	FTSPI020_REG_AC_TIME		0x14
#define	FTSPI020_REG_STS		0x18	/* Status */
#define	FTSPI020_REG_ICR		0x20	/* Interrupt Enable */
#define	FTSPI020_REG_ISR		0x24	/* Interrupt Status */
#define	FTSPI020_REG_READ_STS		0x28
#define FTSPI020_REG_REVISION		0x50
#define FTSPI020_REG_FEATURE		0x54
#define FTSPI020_REG_DATA_PORT		0x100
    
/*
 * Control Register offset 0x10
 */ 
#define	FTSPI020_CTRL_READY_LOC_MASK	~(0x7 << 16)
#define	FTSPI020_CTRL_READY_LOC(x)	(((x) & 0x7) << 16)
    
#define	FTSPI020_CTRL_ABORT		(1 << 8)
    
#define	FTSPI020_CTRL_CLK_MODE_MASK	~(1 << 4)
#define	FTSPI020_CTRL_CLK_MODE_0	(0 << 4)
#define	FTSPI020_CTRL_CLK_MODE_3	(1 << 4)
    
#define	FTSPI020_CTRL_CLK_DIVIDER_MASK	~(3 << 0)
#define	FTSPI020_CTRL_CLK_DIVIDER_2	(0 << 0)
#define FTSPI020_CTRL_CLK_DIVIDER_4	(1 << 0)
#define FTSPI020_CTRL_CLK_DIVIDER_6	(2 << 0)
#define FTSPI020_CTRL_CLK_DIVIDER_8	(3 << 0)
    
/*
 * Status Register offset 0x18
 */ 
#define	FTSPI020_STS_RFR	(1 << 1) /* RX FIFO ready */
#define	FTSPI020_STS_TFR	(1 << 0) /* TX FIFO ready */
    
/*
 * Interrupt Control Register
 */ 
#define	FTSPI020_ICR_RFTH(x)	(((x) & 0x3) << 12)	/* RX FIFO threshold interrupt */
#define	FTSPI020_ICR_TFTH(x)	(((x) & 0x3) << 8)	/* TX FIFO threshold interrupt */
#define	FTSPI020_ICR_DMA	(1 << 0)		/* DMA handshake enable */
    
/*
 * Interrupt Status Register
 */ 
#define	FTSPI020_ISR_CMD_CMPL	(1 << 0)	/* Command complete interrupt  */
    
/*
 * Feature Register
 */ 
#define FTSPI020_FEATURE_CLK_MODE(reg)		(((reg) >> 25) & 0x1)
#define FTSPI020_FEATURE_DTR_MODE(reg)		(((reg) >> 24) & 0x1)
#define FTSPI020_FEATURE_CMDQ_DEPTH(reg)	(((reg) >> 16) & 0xff)
#define FTSPI020_FEATURE_RXFIFO_DEPTH(reg)	(((reg) >>  8) & 0xff)
#define FTSPI020_FEATURE_TXFIFO_DEPTH(reg)	(((reg) >>  0) & 0xff)
    
/*
 * In what state this command belong ?
 */ 
#define FTSPI020_XFER_BEGIN			0x01
#define FTSPI020_XFER_END			0x02
#define FTSPI020_XFER_CMD_STATE			0x04
#define FTSPI020_XFER_DATA_STATE		0x08
#define FTSPI020_XFER_CHECK_CMD_COMPLETE	0x10
    
/*
 * CMD1 Register offset 4: Command Queue Second Word
 */ 
//#define FTSPI020_CMD1_CONT_READ_MODE_EN               (1 << 28)
//#define FTSPI020_CMD1_CONT_READ_MODE_DIS      (0 << 28)
    
#define FTSPI020_CMD1_OP_CODE_0_BYTE		0
#define FTSPI020_CMD1_OP_CODE_1_BYTE		1
#define FTSPI020_CMD1_OP_CODE_2_BYTE		2
    
#define FTSPI020_CMD1_DUMMY_CYCLE(x)	(((x) & 0xff) << 16)
    
#define FTSPI020_CMD1_NO_ADDR			0
#define FTSPI020_CMD1_ADDR_1BYTE	1
#define FTSPI020_CMD1_ADDR_2BYTE	2
#define FTSPI020_CMD1_ADDR_3BYTE	3
#define FTSPI020_CMD1_ADDR_4BYTE	4
    
/*
 * CMD3 Register offset 0xc: Command Queue Fourth Word
 */ 
#define FTSPI020_CMD3_INSTR_CODE(x)	(((x) & 0xff) << 24)
#define FTSPI020_CMD3_CONT_READ_CODE(x)	(((x) & 0xff) << 16)
    
#define FTSPI020_CMD3_CE(x)		(((x) & 0x3) << 8)
    
#define FTSPI020_CMD3_SERIAL_MODE	  0
#define FTSPI020_CMD3_DUAL_MODE		  1
#define FTSPI020_CMD3_QUAD_MODE		  2
#define FTSPI020_CMD3_DUAL_IO_MODE	3
#define FTSPI020_CMD3_QUAD_IO_MODE	4
    
#define FTSPI020_CMD3_DTR_MODE_EN		1
#define FTSPI020_CMD3_DTR_MODE_DIS	0
    
#define FTSPI020_CMD3_STS_SW_READ	1
#define FTSPI020_CMD3_STS_HW_READ	0
    
#define FTSPI020_CMD3_RD_STS_EN		1
#define FTSPI020_CMD3_RD_STS_DIS	0
    
#define	FTSPI020_CMD3_WRITE		1
#define	FTSPI020_CMD3_READ		0
    
#define FTSPI020_CMD3_CMD_COMPL_INTR	(1 << 0)
    
#define FLASH_NAND_BLOCK_PG 64
#define FLASH_NAND_PAGE_SZ  2048
#define FLASH_NAND_SPARE_SZ 64
    
#define FLASH_NAND_P_FAIL 	0x8
#define FLASH_NAND_E_FAIL 	0x4
#define FLASH_NAND_R_FAIL 	0x20
#define FLASH_NAND_R_FIELD 	0x30
#define FLASH_NAND_R_ECC_LIMIT 	0x10
#define FLASH_NAND_R_ECC_LIMIT_GD 	0x30

/* PRIVATE use */ 
#define SPI_XFER_CMD_STATE              0x00000002
//#define SPI_XFER_DATA_STATE               0x00000004
#define SPI_XFER_CHECK_CMD_COMPLETE     0x00000008
#define SPI_XFER_DATA_IN_STATE          0x00000010
#define SPI_XFER_DATA_OUT_STATE         0x00000020
