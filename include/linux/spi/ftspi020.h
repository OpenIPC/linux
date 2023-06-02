/**
 * Command arguments definition for FTSPI020.
 *
 * Author: BingJiun Luo <bjluo@faraday-tech.com> 
 *
 * Copyright (c) 2011, Faraday Technology Corp.
 *
 */

typedef struct sys_header {	
    char signature[8];          /* Signature is "GM8xxx" */
    unsigned int bootm_addr;    /* Image offset to load by spiboot */
    unsigned int bootm_size;
    unsigned int bootm_reserved;
    
    struct {
        unsigned int addr;          /* image address */
        unsigned int size;          /* image size */
        unsigned char name[8];      /* image name */
        unsigned int reserved[1];
    } image[10];
    
    struct 
    {
        unsigned int pagesz_log2;   // page size with bytes in log2
        unsigned int secsz_log2;    // sector size with bytes in log2
        unsigned int chipsz_log2;   // chip size with bytes in log2
        unsigned int clk_div;       // 2/4/6/8
    } norfixup;	

#ifdef CONFIG_PLATFORM_GM8210
    unsigned int addr;          /* image address */
    unsigned int size;          /* image size */
    unsigned char name[8];      /* image name */
#else        
    unsigned int reserved[4];   // unused
#endif
    
    unsigned char   last_256[4];    // byte254:0x55, byte255:0xAA
} sys_header_t;

/*
 * Settings value required to send command.
 */
struct ftspi020_cmd {
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
	u32 write_en;
	u32 read_status_en;
	u32 read_status;
	u32 dtr_mode;
	u32 spi_mode;
	u32 conti_read_mode_code;
	u32 ins_code;

	/* User data buffer pointer */
	const void *tx_buf;
	void *rx_buf;
};

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
#define FTSPI020_CMD1_CONT_READ_MODE_EN		(1 << 28)
#define FTSPI020_CMD1_CONT_READ_MODE_DIS	(0 << 28)

#define FTSPI020_CMD1_OP_CODE_0_BYTE		(0 << 24)
#define FTSPI020_CMD1_OP_CODE_1_BYTE		(1 << 24)
#define FTSPI020_CMD1_OP_CODE_2_BYTE		(2 << 24)

#define FTSPI020_CMD1_DUMMY_CYCLE(x)	(((x) & 0xff) << 16)

#define FTSPI020_CMD1_NO_ADDR		(0 << 0)
#define FTSPI020_CMD1_ADDR_1BYTE	(1 << 0)
#define FTSPI020_CMD1_ADDR_2BYTE	(2 << 0)
#define FTSPI020_CMD1_ADDR_3BYTE	(3 << 0)
#define FTSPI020_CMD1_ADDR_4BYTE	(4 << 0)


/*
 * CMD3 Register offset 0xc: Command Queue Fourth Word
 */
#define FTSPI020_CMD3_INSTR_CODE(x)	(((x) & 0xff) << 24)
#define FTSPI020_CMD3_CONT_READ_CODE(x)	(((x) & 0xff) << 16)

#define FTSPI020_CMD3_CE(x)		(((x) & 0x3) << 8)

#define FTSPI020_CMD3_SERIAL_MODE	(0 << 5)
#define FTSPI020_CMD3_DUAL_MODE		(1 << 5)
#define FTSPI020_CMD3_QUAD_MODE		(2 << 5)
#define FTSPI020_CMD3_DUAL_IO_MODE	(3 << 5)
#define FTSPI020_CMD3_QUAD_IO_MODE	(4 << 5)

#define FTSPI020_CMD3_DTR_MODE_EN	(1 << 4)
#define FTSPI020_CMD3_DTR_MODE_DIS	(0 << 4)

#define FTSPI020_CMD3_STS_SW_READ	(1 << 3)
#define FTSPI020_CMD3_STS_HW_READ	(0 << 3)

#define FTSPI020_CMD3_RD_STS_EN		(1 << 2)
#define FTSPI020_CMD3_RD_STS_DIS	(0 << 2)

#define	FTSPI020_CMD3_WRITE		(1 << 1)	
#define	FTSPI020_CMD3_READ		(0 << 1)	

#define FTSPI020_CMD3_CMD_COMPL_INTR	(1 << 0)

