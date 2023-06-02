/*
 * SPI NAND driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/version.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/jiffies.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <mach/platform/platform_io.h>
#include <mach/platform/board.h>
#include <mach/ftpmu010.h>
//#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/spi/spi.h>
#include "ftspi020_nand.h"
#include <linux/delay.h>

#define CONFIG_FTSPI_NAND_DEBUG			0
#if CONFIG_FTSPI_NAND_DEBUG > 0
#define ftspi_nand_dbg(fmt, ...) printk(KERN_INFO fmt, ##__VA_ARGS__);
#else
#define ftspi_nand_dbg(fmt, ...) do {} while (0);
#endif
/*
 * Local function or variables declaration
 */

#define CONFIG_NAND_DISTRUB
//#define CONFIG_SPI_NAND_USE_AHBDMA//???
#if defined(CONFIG_SPI_NAND_USE_AHBDMA) || defined(CONFIG_SPI_NAND_USE_AXIDMA)
#include <linux/dmaengine.h>

#ifdef CONFIG_SPI_NAND_USE_AHBDMA
#include <mach/ftdmac020.h>
#else
#include <mach/ftdmac030.h>
#endif

#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136) || defined(CONFIG_PLATFORM_GM8220)
#define AHBMASTER_R_SRC   FTDMA020_AHBMASTER_0
#define AHBMASTER_R_DST   FTDMA020_AHBMASTER_1
#define AHBMASTER_W_SRC   FTDMA020_AHBMASTER_1
#define AHBMASTER_W_DST   FTDMA020_AHBMASTER_0
#define SPI020_REQ    8
#endif

#endif

#define PORTING
static int ftnandc023v2_block_markbad(struct mtd_info *mtd, loff_t ofs);
static int calc_new_page(struct mtd_info *mtd, int page_addr);
int ftnandc_spi_read_bbt(struct mtd_info *mtd, loff_t offs);
static int nand_fd, cmdfunc_status = NAND_STATUS_READY;

#define FLASH_PAGESIZE  256
#define IDCODE_LEN		4

/* Flash opcodes. */
#define	OPCODE_WREN		0x06    /* Write enable */
#define	OPCODE_RDSR		0x05    /* Read status register */
#define	OPCODE_RDSR2	0x35
#define	OPCODE_WRSR		0x01    /* Write status register 1 byte */
#define	OPCODE_EN4B		0xB7
#define	OPCODE_EX4B		0xE9

#define	OPCODE_BE_32K		0x52    /* Erase 32KiB block */
#define	OPCODE_BE_64K		0xd8    /* Erase 64KiB block */

/* Status Register bits. */
#define	SR_WIP			1       /* Write in progress */
#define	SR_WEL			2       /* Write enable latch */
/* meaning of other SR_* bits may differ between vendors */
#define	SR_BP0			4       /* Block protect 0 */
#define	SR_BP1			8       /* Block protect 1 */
#define	SR_BP2			0x10    /* Block protect 2 */
#define	SR_SRWD			0x80    /* SR write protect */

/* Define max times to check status register before we give up. */
#define	MAX_READY_WAIT_COUNT	30000
#define	CMD_SIZE		    4

#define SPI_NAME	"SPI_NAND_FLASH"
#define BLOCK_ALIGN(base, blk_shift) ((((base) + (0x1 << (blk_shift)) - 1) >> (blk_shift)) << (blk_shift))

/*
 * Macro definitions
 */
#define DEFAULT_BLOCK_SIZE  0x20000
#define MTD_CFG_SZ          (6 * DEFAULT_BLOCK_SIZE)    //2 block + reserved

#define CONFIG_FTNANDC023_START_CHANNEL     0
#define BLOCK_ALIGN(base, blk_shift) ((((base) + (0x1 << (blk_shift)) - 1) >> (blk_shift)) << (blk_shift))

#define FTSPI020_FEATURE_RXFIFO_DEPTH(reg)	(((reg) >>  8) & 0xff)
#define FTSPI020_FEATURE_TXFIFO_DEPTH(reg)	(((reg) >>  0) & 0xff)

static uint8_t ftnandc023_bbt_pattern[] = { 'B', 'b', 't', '0' };
static uint8_t ftnandc023_mirror_pattern[] = { '1', 't', 'b', 'B' };

#ifdef CONFIG_GPIO_WP
#define GPIO_PIN      28
#define PIN_NAME      "gpio28"
#endif

//=============================================================================
// System Header, size = 512 bytes
//=============================================================================

static int read_id_time = 0, ID_point = 0;
static int startchn = CONFIG_FTNANDC023_START_CHANNEL;
static int legacy;

static u8 idcode[IDCODE_LEN];
static struct mtd_partition PORTING ftnandc023_partition_info[25];

int show_one_time = 1;
extern int root_mtd_num;

#if defined(CONFIG_SPI_NAND_USE_AHBDMA) || defined(CONFIG_SPI_NAND_USE_AXIDMA)
static void ftspi020_dma_callback(void *param);
static unsigned int dma_trigger_flag = 0;
static wait_queue_head_t dma_queue;
#endif

/******************************************************************************
 * spi private data
 *****************************************************************************/
typedef struct spi_nand_sys_header {
    char signature[8];          /* Signature is "GM8xxx" */
    struct {
        unsigned int addr;          /* partition address */
        unsigned int size;          /* partition size */
        unsigned char name[8];      /* partition name */
        unsigned int img_size;		/* image size */
    } image[23];
    
    struct {
        unsigned int nand_numblks;      //number of blocks in chip
        unsigned int nand_numpgs_blk;   //how many pages in a block
        unsigned int nand_pagesz;       //real size in bytes                        
        unsigned int nand_sparesz_inpage;       //64bytes for 2k, ...... needed for NANDC023
        unsigned int nand_numce;        //how many CE in chip
        unsigned int nand_clkdiv;				// 2/4/6/8
        unsigned int nand_ecc_capability;   
        unsigned int nand_sparesz_insect;
    } nandfixup;
    
    unsigned int reserved1; 	// unused
    unsigned char function[4];
    unsigned char reserved2[2];	// unused
    unsigned char last_511[2];  // byte510:0x55, byte511:0xAA
} spi_nand_sys_header_t;

struct common_spi_flash {
    struct spi_device *spi;
    struct mutex lock;
    struct mtd_info mtd;
    spi_nand_sys_header_t *sys_hdr;      /* system header */

    u8 erase_opcode;
    u8 mode_3_4;
    u8 flash_type;              //1:winbond, status register is different
};

static struct resource ftnandc023_resource[] = {
    [0] = {
           .start = SPI_FTSPI020_PA_BASE,
           .end = SPI_FTSPI020_PA_LIMIT,
           .flags = IORESOURCE_MEM,
           },
    [1] = {
           .start = SPI_FTSPI020_IRQ,
           .end = SPI_FTSPI020_IRQ,
           .flags = IORESOURCE_IRQ,
           }
};

static struct nand_ecclayout nand_0_hw_eccoob = {
    .eccbytes = 16,    
    .eccpos = {
               },        
    .oobfree = {
                {.offset = 4,.length = 12,},
                {.offset = 20,.length = 12,},
                {.offset = 36,.length = 12,},
                {.offset = 52,.length = 12,}
                },
};

static struct nand_ecclayout nand_1_hw_eccoob = {
    .eccbytes = 16,    
    .eccpos = {
               /* at the end of spare sector */
               12, 13, 14, 15,
               28, 29, 30, 31,
               44, 45, 46, 47,
               60, 61, 62, 63,
               },        
    .oobfree = {
                {.offset = 4,.length = 8},
                {.offset = 20,.length = 8},
                {.offset = 36,.length = 8},
                {.offset = 52,.length = 8}
                },
};

static struct nand_ecclayout nand_2_hw_eccoob = {
    .eccbytes = 12,    
    .eccpos = {
               /* at the end of spare sector */
               1, 2, 3,
               17, 18, 19,
               33, 34, 35,
               49, 50, 51,
               },
    .oobfree = {
                {.offset = 8,.length = 8,},
                {.offset = 24,.length = 8,},
                {.offset = 40,.length = 8,},
                {.offset = 56,.length = 8,}
                },
};

static struct nand_ecclayout nand_3_hw_eccoob = {
    .eccbytes = 24,    
    .eccpos = {
               /* at the end of spare sector */
               8, 9, 10, 11, 12, 13,
               24, 25, 26, 27, 28, 29,
               40, 41, 42, 43, 44, 45,
               56, 57, 58, 59, 60, 61,
               },        
    .oobfree = {
                {.offset = 4,.length = 4,},
                {.offset = 20,.length = 4,},
                {.offset = 36,.length = 4,},
                {.offset = 52,.length = 4,}
                },
};

static struct nand_ecclayout nand_4_hw_eccoob = {
    .eccbytes = 28,    
    .eccpos = {
               /* at the end of spare sector */
               18, 19, 20, 21, 22, 23, 24,
               25, 26, 27, 28, 29, 30, 31,
               50, 51, 52, 53, 54, 55, 56,
               57, 58, 59, 60, 61, 62, 63,
               },        
    .oobfree = {/* chip & datasheet not match */
                {.offset = 2,.length = 8,},
                {.offset = 10,.length = 8,},
                {.offset = 34,.length = 8,},
                {.offset = 42,.length = 8,}
                },
};

static struct nand_bbt_descr ftnandc023_bbt_mirror_descr = {
    .options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
        | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
    .offs = 0,
    .len = 4,
    .veroffs = 4,
    .maxblocks = 4,
    .pattern = ftnandc023_mirror_pattern
};

static struct nand_bbt_descr ftnandc023_bbt_main_descr = {
    .options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
        | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
    .offs = 0,
    .len = 4,
    .veroffs = 4,
    .maxblocks = 4,
    .pattern = ftnandc023_bbt_pattern
};

static uint8_t ftnandc023_scan_ff_pattern[] = { 0xff };

static struct nand_bbt_descr ftnandc023_largepage_flashbased = {
    .options = NAND_BBT_SCAN2NDPAGE,
    .offs = 0,
    .len = 1,
    .pattern = ftnandc023_scan_ff_pattern
};

//=============================================================================
// BI table, size = 1024 bytes
//=============================================================================
typedef struct bi_table {
    /* This array size is related to USB_BUFF_SZ defined in usb_scsi.h */
    unsigned int bi_status[256];        //each bit indicates a block. 1 for good, 0 for bad
} bi_table_t;

typedef struct lookup_table
{
    unsigned short physical[8192];
} lookup_table_t;

struct ftnandc023_nand_data {
    struct nand_chip chip;
    struct mtd_info mtd;
    void __iomem *io_base;
    int sel_chip;
    int cur_cmd;
    int page_addr;
    int column;
    int byte_ofs;
    u32 *buf;            
    struct device *dev;
#if defined(CONFIG_SPI_NAND_USE_AHBDMA) || defined(CONFIG_SPI_NAND_USE_AXIDMA)
    struct dma_chan *dma_chan;
    dma_cap_mask_t cap_mask;
#ifdef CONFIG_SPI_NAND_USE_AXIDMA
    struct ftdmac030_dma_slave dma_slave_config;
#else
    struct ftdmac020_dma_slave dma_slave_config;
#endif
    dma_cookie_t cookie;
    dma_addr_t mem_dmaaddr;
    dma_addr_t nand_dmaaddr;
    unsigned char *mem_dmabuf;
    unsigned char *sg_dmabuf;
#endif
    dma_addr_t syshd_dmaaddr;
    dma_addr_t bitab_dmaaddr;
    dma_addr_t lookuptab_dmaaddr;
       
    unsigned int ce;
    unsigned int erase_sector_size;
    unsigned short page_size;
    unsigned int nr_pages;
    unsigned int size;          //chip size
    unsigned int mode_3_4;      //addres length = 3 or 4 bytes
    unsigned short chip_type;
    
    struct {
        unsigned int block_pg;
        unsigned int page_size;
        unsigned int spare_size;
        unsigned int block_size;
    } nand_parm;

    int rxfifo_depth;
    int txfifo_depth;

    int cur_chan;
    int valid_chip[MAX_CHANNEL];
    int scan_state;
    int flash_type;
    int large_page;
    int lookup_flag;
    unsigned char ecc_type;
    unsigned short block_limit;
    spi_nand_sys_header_t *sys_hdr;      /* system header */
    bi_table_t *bi_table;       /* bad block table next to sys_hdr */
    lookup_table_t *lookup_table;
    int (*write_oob) (struct mtd_info * mtd, const u_char * buf, int len);
    int (*read_oob) (struct mtd_info * mtd, u_char * buf);
    int (*write_page) (struct mtd_info * mtd, const uint8_t * buf);
    int (*read_page) (struct mtd_info * mtd, u_char * buf);
};

static int ftspi020_data_access(struct ftnandc023_nand_data *data, u8 direction, u8 * addr, u32 len);
static int spi_flash_cmd_read_fast(struct ftnandc023_nand_data *data, u32 offset, size_t len, u8 *buf);
/******************************************************************************
 * internal functions for FTSPI020
 *****************************************************************************/
#ifdef CONFIG_PLATFORM_GM8139
static pmuReg_t pmu_reg[] = {
    /* off, bitmask,   lockbit,    init val,   init mask */
    {0x54, (0x3F << 26), 0, (0x15 << 26), (0x3F << 26)},        /* pinMux with NAND */
    {0x58, (0x3 << 0), (0x3 << 0), (0x1 << 0), (0x3 << 0)},     /* DMA ack selection */
    {0x6C, (0x7 << 0), (0x7 << 0), (0x7 << 0), (0x7 << 0)},
    {0xB4, (0x1 << 15), (0x1 << 15), (0x0 << 15), (0x1 << 15)}, /* AHB clock gate */
};
#endif

#ifdef CONFIG_PLATFORM_GM8136
static pmuReg_t pmu_reg[] = {
    /* off, bitmask,   lockbit,    init val,   init mask */
    {0x6C, (0x7 << 0), (0x7 << 0), (0x7 << 0), (0x7 << 0)},
    {0xB4, (0x1 << 15), (0x1 << 15), (0x0 << 15), (0x1 << 15)}, /* AHB clock gate */
};
#endif

#ifdef CONFIG_PLATFORM_GM8220
static pmuReg_t  pmu_reg[] = {
    /* off, bitmask,   lockbit,    init val,   init mask */
    {0x1A4, (0xFFFF << 14), (0xFFFF << 14), (0x5555 << 14), (0xFFFF << 14)},   /* pinMux */
    //{0x1A4, (0xFFFF << 14), (0xFFFF << 14), (0xAAAA << 14), (0xFFFF << 14)},   /* pinMux, 4-bit mode */
    {0x68, (0x1 << 17), (0x1 << 17), (0x0 << 17), (0x1 << 17)},   /* AHB clock gate */
};
#endif

static pmuRegInfo_t pmu_reg_info = {
    "SPI020_NAND",
    ARRAY_SIZE(pmu_reg),
    ATTR_TYPE_NONE,
    pmu_reg
};

static void ftspi020_dma_disable(void __iomem * base)
{
    unsigned int tmp = inl(base + FTSPI020_REG_ICR);

    tmp &= ~FTSPI020_ICR_DMA;
    outl(tmp, base + FTSPI020_REG_ICR);
}

#if defined(CONFIG_SPI_NAND_USE_AHBDMA) || defined(CONFIG_SPI_NAND_USE_AXIDMA)
static void ftspi020_dma_enable(void __iomem * base)
{
    unsigned int tmp = inl(base + FTSPI020_REG_ICR);

    tmp |= FTSPI020_ICR_DMA;
    outl(tmp, base + FTSPI020_REG_ICR);
}

static int ftspi020_dma_wait(void)
{
    int rc = 0;

    rc = wait_event_timeout(dma_queue, dma_trigger_flag == 1, 10 * HZ);
    if (rc == 0) {
        printk(KERN_ERR "SPI-NAND queue wake up timeout signal arrived\n");
        return 1;
    }

    dma_trigger_flag = 0;
    return 0;
}

#else

static void ftspi020_wait_rxfifo_ready(void __iomem * base)
{
    volatile u32 value;
    unsigned long timeo = jiffies;
    int ret = 1;

    timeo += HZ;
    while (time_before(jiffies, timeo)) {
        //udelay (1);
        value = inl(base + FTSPI020_REG_STS);
        if (value & FTSPI020_STS_RFR)
            break;
    }
    if (!(value & FTSPI020_STS_RFR))
        printk(KERN_ERR "Wait RX FIFO timeout!\n");
    else
        ret = 0

    return ret;        
}

static int ftspi020_wait_txfifo_ready(void __iomem * base)
{
    volatile u32 value;
    int ret = 1;

    unsigned long timeo = jiffies;

    timeo += HZ;
    while (time_before(jiffies, timeo)) {
        //udelay (1);
        value = inl(base + FTSPI020_REG_STS);
        if (value & FTSPI020_STS_TFR)
            break;
    }
    if (!(value & FTSPI020_STS_TFR))
        printk(KERN_ERR "Wait TX FIFO timeout!\n");
    else
        ret = 0

    return ret;
}
#endif

static unsigned int ftspi020_read_feature(void __iomem * base)
{
    return inl(base + FTSPI020_REG_FEATURE);
}

static int ftspi020_txfifo_depth(void __iomem * base)
{
    return FTSPI020_FEATURE_TXFIFO_DEPTH(ftspi020_read_feature(base));
}

static int ftspi020_rxfifo_depth(void __iomem * base)
{
    return FTSPI020_FEATURE_RXFIFO_DEPTH(ftspi020_read_feature(base));
}

static unsigned char ftspi020_read_status(void __iomem * base)
{
    return inb(base + FTSPI020_REG_READ_STS);
}

static void ftspi020_clk_divider(void __iomem * base, char div)
{
	unsigned int tmp = inl(base + FTSPI020_REG_CTRL);

	tmp &= FTSPI020_CTRL_CLK_DIVIDER_MASK;
	tmp |= div;
	outl(tmp, base + FTSPI020_REG_CTRL);
}

static void setup_clk(void __iomem * base)
{
    int CLK, SCU_DIV, SPI_DIV, mod, max_speed_hz;

#ifdef CONFIG_PLATFORM_GM8139
    max_speed_hz = 45 * 1000 * 1000; //45MHz

    if(ftpmu010_read_reg(0x28) & (1 << 13))
        CLK = ftpmu010_get_attr(ATTR_TYPE_PLL2) / 4;
    else
        CLK = ftpmu010_get_attr(ATTR_TYPE_PLL1) / 2;
#endif
#ifdef CONFIG_PLATFORM_GM8136
    max_speed_hz = 45 * 1000 * 1000; //45MHz

    if(ftpmu010_read_reg(0x28) & (1 << 10))
        CLK = ftpmu010_get_attr(ATTR_TYPE_PLL1);
    else
        CLK = ftpmu010_get_attr(ATTR_TYPE_PLL2);
#endif
#ifdef CONFIG_PLATFORM_GM8220
    max_speed_hz = 100 * 1000 * 1000; //90MHz
	CLK = ftpmu010_get_attr(ATTR_TYPE_AHB);
#endif

	//printk("CLK = %d\n", CLK);
    SCU_DIV = CLK / 2;
    mod = do_div(SCU_DIV, max_speed_hz);
    
    if(mod == 0)
        SCU_DIV -= 1;

    if(SCU_DIV > 7){
        printk(KERN_ERR "SPI clock too quick\n");
        SPI_DIV = FTSPI020_CTRL_CLK_DIVIDER_4;
        SCU_DIV = 3;
    }
    else {
        SPI_DIV = FTSPI020_CTRL_CLK_DIVIDER_2;
    }
    
    CLK = CLK / ((1 << (SPI_DIV + 1)) * (SCU_DIV + 1));
    ftpmu010_write_reg(nand_fd, 0x6C, SCU_DIV, 0x7);

    printk(KERN_INFO "CLK scu div %d, spi div %d, clock = %dHz\n", SCU_DIV, SPI_DIV, CLK);
	ftspi020_clk_divider(base, SPI_DIV);
}

/* polling command complete interrupt until the interrupt comes.
 */
static int ftspi020_wait_cmd_complete(struct ftnandc023_nand_data *data)
{
    u32 value = 0;
    int ret = 1;
    unsigned long timeo = jiffies;

    ftspi_nand_dbg("wait_cmd_complete\n");

    timeo += HZ;
    while (time_before(jiffies, timeo)) {
        //udelay (1);
        value = inl(data->io_base + FTSPI020_REG_ISR);
        if (value & FTSPI020_ISR_CMD_CMPL) {
            outl(value, data->io_base + FTSPI020_REG_ISR);
            break;
        }
    }
    if (!(value & FTSPI020_ISR_CMD_CMPL))
        printk(KERN_ERR "Wait command complete timeout!\n");
    else
        ret = 0;
        
    return ret;
}

void ftspi020_issue_cmd(struct ftnandc023_nand_data *data, struct ftspi020_cmd *command)
{
    u32 cmd_feature1, cmd_feature2;

    outl(command->spi_addr, data->io_base + FTSPI020_REG_CMD0);

    cmd_feature1 = ((command->conti_read_mode_en & 0x1) << 28) |
        ((command->ins_len & 0x3) << 24) | ((command->dum_2nd_cyc & 0xFF) << 16) | 
        ((command->addr_len & 0x7) << 0);
    outl(cmd_feature1, data->io_base + FTSPI020_REG_CMD1);

    outl(command->data_cnt, data->io_base + FTSPI020_REG_CMD2);

    cmd_feature2 = ((command->ins_code & 0xFF) << 24) |
        ((command->conti_read_mode_code & 0xFF) << 16) |
        ((command->start_ce & 0x3) << 8) |
        ((command->spi_mode & 0x7) << 5) |
        ((command->dtr_mode & 0x1) << 4) |
        ((command->read_status & 0x1) << 3) |
        ((command->read_status_en & 0x1) << 2) | ((command->write_en & 0x1) << 1) | 
        ((command->intr_en & 0x1) << 0);
    ftspi_nand_dbg("spi reg 0x%x = 0x%x, reg 0x%x = 0x%x\n", FTSPI020_REG_ICR,
                   inl(data->io_base + FTSPI020_REG_ICR), FTSPI020_REG_ISR,
                   inl(data->io_base + FTSPI020_REG_ISR));
    ftspi_nand_dbg("spi cmd queue = 0x%x, 0x%x, 0x%x, 0x%x\n", command->spi_addr, cmd_feature1,
                   command->data_cnt, cmd_feature2);
    outl(cmd_feature2, data->io_base + FTSPI020_REG_CMD3);
}

void ftspi020_setup_cmdq(struct ftnandc023_nand_data *data, ftspi020_cmd_t * spi_cmd, int ins_code,
                         int ins_len, int write_en, int spi_mode, int data_cnt)
{
    memset(spi_cmd, 0x0, sizeof(ftspi020_cmd_t));
//SPI_DEBUG("<ftspi020_setup_cmdq=0x%x>",ins_code);      
    spi_cmd->start_ce = 0;      //only one CE???
    spi_cmd->ins_code = ins_code;
    spi_cmd->intr_en = FTSPI020_CMD3_CMD_COMPL_INTR;
    spi_cmd->ins_len = ins_len;
    spi_cmd->write_en = write_en;       //0 for the read ata or read status, others are 1.
    spi_cmd->dtr_mode = FTSPI020_CMD3_DTR_MODE_DIS;
    spi_cmd->spi_mode = spi_mode;
    spi_cmd->data_cnt = data_cnt;

    switch (ins_code) {
    case COMMAND_READ_ID:
        spi_cmd->spi_addr = 0x00;
        spi_cmd->addr_len = FTSPI020_CMD1_ADDR_1BYTE;
        break;
    case COMMAND_SET_FEATURES:
        spi_cmd->spi_addr = 0xA0;       // to disble write protect
        spi_cmd->addr_len = FTSPI020_CMD1_ADDR_1BYTE;
        break;
    default:
        break;
    }

    /* update to the controller */
    ftspi020_issue_cmd(data, spi_cmd);
}

static int spi_xfer(struct ftnandc023_nand_data *data, unsigned int len, const void *dout, void *din,
             unsigned long flags)
{
    struct ftspi020_cmd spi_cmd;
    u8 *u8_data_out = (u8 *) dout;
    int ret = 0;

    memset(&spi_cmd, 0, sizeof(struct ftspi020_cmd));

    /* send the instruction */
    if (flags & SPI_XFER_CMD_STATE) {

        spi_cmd.ins_code = *u8_data_out;
        //printk("dout=0x%x,din=0x%x,%d,%d\n",*(UINT8 *)dout,*(UINT8 *)din, len, spi->cs);
        spi_cmd.intr_en = FTSPI020_ISR_CMD_CMPL;
#ifdef CONFIG_CMD_FPGA
        spi_cmd.start_ce = 0;
#endif
        ftspi_nand_dbg("spi cmd = 0x%x\n", spi_cmd.ins_code);
        switch (spi_cmd.ins_code) {
        case COMMAND_READ_ID:
            spi_cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
            spi_cmd.write_en = FTSPI020_CMD3_READ;
            spi_cmd.dtr_mode = FTSPI020_CMD3_DTR_MODE_DIS;
            spi_cmd.spi_mode = FTSPI020_CMD3_SERIAL_MODE;
            spi_cmd.data_cnt = IDCODE_LEN;
            spi_cmd.spi_addr = (u32) * (u8_data_out + 1);
            spi_cmd.addr_len = FTSPI020_CMD1_ADDR_1BYTE;
            break;
        case COMMAND_RESET:
            spi_cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
            spi_cmd.write_en = FTSPI020_CMD3_WRITE;
            spi_cmd.dtr_mode = FTSPI020_CMD3_DTR_MODE_DIS;
            spi_cmd.spi_mode = FTSPI020_CMD3_SERIAL_MODE;
            break;
        case COMMAND_WRITE_ENABLE:
        case COMMAND_WRITE_DISABLE:
            //if (write_enable == spi_cmd.ins_code){
            //          printk("exit\n");
            //    goto exit;
            //  }
            //write_enable = spi_cmd.ins_code;
            spi_cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
            spi_cmd.write_en = FTSPI020_CMD3_WRITE;
            spi_cmd.dtr_mode = FTSPI020_CMD3_DTR_MODE_DIS;
            spi_cmd.spi_mode = FTSPI020_CMD3_SERIAL_MODE;
            break;
        case COMMAND_ERASE_128K_BLOCK:
            spi_cmd.spi_addr =
                (*(u8_data_out + 3) << 16) | (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
            spi_cmd.addr_len = FTSPI020_CMD1_ADDR_3BYTE;

            spi_cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
            spi_cmd.write_en = FTSPI020_CMD3_WRITE;
            spi_cmd.dtr_mode = FTSPI020_CMD3_DTR_MODE_DIS;
            spi_cmd.spi_mode = FTSPI020_CMD3_SERIAL_MODE;
            break;
        case COMMAND_ERASE_CHIP:
            spi_cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
            spi_cmd.write_en = FTSPI020_CMD3_WRITE;
            spi_cmd.dtr_mode = FTSPI020_CMD3_DTR_MODE_DIS;
            spi_cmd.spi_mode = FTSPI020_CMD3_SERIAL_MODE;
            break;
        case COMMAND_WRITE_STATUS:
            spi_cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
            spi_cmd.write_en = FTSPI020_CMD3_WRITE;
            spi_cmd.spi_mode = FTSPI020_CMD3_SERIAL_MODE;
            spi_cmd.data_cnt = len;
            break;
        case COMMAND_READ_STATUS1:
        case COMMAND_READ_STATUS2:
            spi_cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
            spi_cmd.write_en = FTSPI020_CMD3_READ;
            spi_cmd.read_status_en = FTSPI020_CMD3_RD_STS_EN;
            spi_cmd.read_status = FTSPI020_CMD3_STS_SW_READ;
            spi_cmd.dtr_mode = FTSPI020_CMD3_DTR_MODE_DIS;
            spi_cmd.spi_mode = FTSPI020_CMD3_SERIAL_MODE;
            break;
        case COMMAND_WRITE_PAGE:       //cmd as the same with COMMAND_PROGRAM_EXECUTE
            spi_cmd.spi_addr = (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
            spi_cmd.addr_len = FTSPI020_CMD1_ADDR_2BYTE;

            spi_cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
            spi_cmd.data_cnt = len;
            spi_cmd.write_en = FTSPI020_CMD3_WRITE;
            spi_cmd.read_status_en = FTSPI020_CMD3_RD_STS_DIS;
            spi_cmd.dtr_mode = FTSPI020_CMD3_DTR_MODE_DIS;
            spi_cmd.spi_mode = FTSPI020_CMD3_SERIAL_MODE;
            break;
        case COMMAND_WINBOND_QUAD_WRITE_PAGE:
        case COMMAND_QUAD_WRITE_PAGE:
            spi_cmd.spi_addr =
                (*(u8_data_out + 3) << 16) | (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
            spi_cmd.addr_len = FTSPI020_CMD1_ADDR_3BYTE;

            spi_cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
            spi_cmd.data_cnt = len;
            spi_cmd.write_en = FTSPI020_CMD3_WRITE;
            spi_cmd.read_status_en = FTSPI020_CMD3_RD_STS_DIS;
            spi_cmd.dtr_mode = FTSPI020_CMD3_DTR_MODE_DIS;
            spi_cmd.spi_mode = FTSPI020_CMD3_QUAD_MODE;
            break;
        case COMMAND_FAST_READ_QUAD_IO:
            spi_cmd.spi_addr =
                (*(u8_data_out + 3) << 16) | (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
            spi_cmd.addr_len = FTSPI020_CMD1_ADDR_3BYTE;

            spi_cmd.dum_2nd_cyc = 4;
            spi_cmd.conti_read_mode_en = 1;
            spi_cmd.conti_read_mode_code = 0;
            spi_cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
            spi_cmd.data_cnt = len;
            spi_cmd.write_en = FTSPI020_CMD3_READ;
            spi_cmd.read_status_en = FTSPI020_CMD3_RD_STS_DIS;
            spi_cmd.dtr_mode = FTSPI020_CMD3_DTR_MODE_DIS;
            spi_cmd.spi_mode = FTSPI020_CMD3_QUAD_MODE;
            break;
        case COMMAND_READ_DATA:
        case COMMAND_FAST_READ:
            spi_cmd.spi_addr = (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
            spi_cmd.addr_len = FTSPI020_CMD1_ADDR_2BYTE;

            spi_cmd.dum_2nd_cyc = 8;
            spi_cmd.conti_read_mode_en = 0;
            spi_cmd.conti_read_mode_code = 0;
            spi_cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
            spi_cmd.data_cnt = len;
            spi_cmd.write_en = FTSPI020_CMD3_READ;
            spi_cmd.read_status_en = FTSPI020_CMD3_RD_STS_DIS;
            spi_cmd.dtr_mode = FTSPI020_CMD3_DTR_MODE_DIS;
            spi_cmd.spi_mode = FTSPI020_CMD3_SERIAL_MODE;
            break;
        case COMMAND_FAST_READ_DUAL:
            spi_cmd.spi_addr = (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
            spi_cmd.addr_len = FTSPI020_CMD1_ADDR_2BYTE;

            spi_cmd.dum_2nd_cyc = 8;
            spi_cmd.conti_read_mode_en = 0;
            spi_cmd.conti_read_mode_code = 0;
            spi_cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
            spi_cmd.data_cnt = len;
            spi_cmd.write_en = FTSPI020_CMD3_READ;
            spi_cmd.read_status_en = FTSPI020_CMD3_RD_STS_DIS;
            spi_cmd.dtr_mode = FTSPI020_CMD3_DTR_MODE_DIS;
            spi_cmd.spi_mode = FTSPI020_CMD3_DUAL_MODE;
            break;
        case COMMAND_GET_FEATURES:
            spi_cmd.spi_addr = (u32) * (u8_data_out + 1);
            spi_cmd.addr_len = FTSPI020_CMD1_ADDR_1BYTE;
            spi_cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
            spi_cmd.write_en = FTSPI020_CMD3_READ;
            spi_cmd.read_status_en = FTSPI020_CMD3_RD_STS_EN;
            spi_cmd.read_status = FTSPI020_CMD3_STS_SW_READ;
            spi_cmd.dtr_mode = FTSPI020_CMD3_DTR_MODE_DIS;
            spi_cmd.spi_mode = FTSPI020_CMD3_SERIAL_MODE;
            break;
        case COMMAND_PAGE_READ:
            spi_cmd.spi_addr =
                (*(u8_data_out + 3) << 16) | (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
            spi_cmd.addr_len = FTSPI020_CMD1_ADDR_3BYTE;
            spi_cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
            spi_cmd.data_cnt = 0;
            spi_cmd.write_en = FTSPI020_CMD3_WRITE;
            spi_cmd.read_status_en = FTSPI020_CMD3_RD_STS_DIS;
            spi_cmd.dtr_mode = FTSPI020_CMD3_DTR_MODE_DIS;
            spi_cmd.spi_mode = FTSPI020_CMD3_SERIAL_MODE;
            break;
        case COMMAND_PROGRAM_EXECUTE:
            spi_cmd.spi_addr =
                (*(u8_data_out + 3) << 16) | (*(u8_data_out + 2) << 8) | (*(u8_data_out + 1));
            spi_cmd.addr_len = FTSPI020_CMD1_ADDR_3BYTE;
            spi_cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
            spi_cmd.data_cnt = len;
            spi_cmd.write_en = FTSPI020_CMD3_WRITE;
            spi_cmd.read_status_en = FTSPI020_CMD3_RD_STS_DIS;
            spi_cmd.dtr_mode = FTSPI020_CMD3_DTR_MODE_DIS;
            spi_cmd.spi_mode = FTSPI020_CMD3_SERIAL_MODE;
            break;
        default:
            printk(KERN_ERR "Not define this command 0x%x!!!!!!!\n", spi_cmd.ins_code);
            goto xfer_exit;
            break;
        }
        /* sent the command out */
        ftspi020_issue_cmd(data, &spi_cmd);

    }
    else if (flags & SPI_XFER_DATA_IN_STATE) {
        /* read the data */
        //printk("read len = %d, buf = 0x%x\n", len, din);
        ret = ftspi020_data_access(data, 1, (u8 *) din, len);
        if(ret)
            return ret;
        ftspi_nand_dbg("read data finish\n");
    }
    else if (flags & SPI_XFER_DATA_OUT_STATE) {
        /* send the data */
        ftspi_nand_dbg("write len = %d, buf = 0x%x\n", len, dout);
        ret = ftspi020_data_access(data, 0, (u8 *) dout, len);
        if(ret)
            return ret;        
        ftspi_nand_dbg("write data finish\n");
    }

    /* check command complete */
    if (flags & SPI_XFER_CHECK_CMD_COMPLETE)
        ret = ftspi020_wait_cmd_complete(data);

xfer_exit:
    return ret;
}

/******************************************************************************
 * workqueue
 *****************************************************************************/
#if defined(CONFIG_SPI_NAND_USE_AHBDMA) || defined(CONFIG_SPI_NAND_USE_AXIDMA)

static int setup_dma(struct ftnandc023_nand_data *ctrl, int direct)
{
    struct dma_slave_config *common;

    ctrl->dma_slave_config.id = -1;
    ctrl->dma_slave_config.handshake = SPI020_REQ;      //enable

    common = &ctrl->dma_slave_config.common;

#ifdef CONFIG_SPI_NAND_USE_AHBDMA
    ctrl->dma_slave_config.src_size = FTDMAC020_BURST_SZ_4;     //64x4=256

    if (direct == DMA_DEV_TO_MEM) {
        ctrl->dma_slave_config.src_sel = AHBMASTER_R_SRC;
        ctrl->dma_slave_config.dst_sel = AHBMASTER_R_DST;
    } else {
        ctrl->dma_slave_config.src_sel = AHBMASTER_W_SRC;
        ctrl->dma_slave_config.dst_sel = AHBMASTER_W_DST;
    }
#else
    common->dst_maxburst = 4;
    common->src_maxburst = 4;
#endif

    if (direct == DMA_MEM_TO_DEV) {
        common->src_addr = ctrl->mem_dmaaddr;
        common->dst_addr = ctrl->nand_dmaaddr;
    } else {
        common->src_addr = ctrl->nand_dmaaddr;
        common->dst_addr = ctrl->mem_dmaaddr;
    }

    /* SPI kernel maybe send len = 2011, so can't div 4 */
    common->dst_addr_width = 1;
    common->src_addr_width = 1;

    common->direction = direct;

    return dmaengine_slave_config(ctrl->dma_chan, common);      //step 2
}

static int spi020_dma_start(struct ftnandc023_nand_data *ctrl, size_t len, int direct)
{
    int ret;
    enum dma_ctrl_flags flags;
    struct dma_async_tx_descriptor *desc;

    ret = setup_dma(ctrl, direct);
    if (ret)
        return ret;

    flags =
        DMA_PREP_INTERRUPT | DMA_CTRL_ACK | DMA_COMPL_SKIP_SRC_UNMAP | DMA_COMPL_SKIP_DEST_UNMAP;

    desc = dmaengine_prep_slave_single(ctrl->dma_chan, (void *)ctrl->sg_dmabuf, len, direct, flags);    //step 3

    if (!desc)
        return -1;

    desc->callback = ftspi020_dma_callback;
    desc->callback_param = &ctrl;
    ctrl->cookie = dmaengine_submit(desc);      //step 4
    dma_async_issue_pending(ctrl->dma_chan);    //step 5

    return 0;
}

static void ftspi020_dma_callback(void *param)
{
    //printk("<D>\n");

    dma_trigger_flag = 1;
    wake_up(&dma_queue);

    return;
}
#endif

/* direction: 1 is read, 0 is write */
static int ftspi020_data_access(struct ftnandc023_nand_data *data, u8 direction, u8 * addr, u32 len)
{
    u32 *tmp;
#ifdef CONFIG_SPI_NAND_USE_AHBDMA
    int ret = -1;
#endif

    tmp = (u32 *) addr;

    if (direction) {
        /* read direction */
        if (len == IDCODE_LEN) {        // read ID
            *tmp++ = *(volatile unsigned *)(data->chip.IO_ADDR_R);
            return 0;
        }
#ifdef CONFIG_SPI_NAND_USE_AHBDMA

//printk("len=%d\n",len); 
        ret = spi020_dma_start(data, len, DMA_DEV_TO_MEM);
        if (ret < 0) {
            printk(KERN_ERR "spi020 dma read fail\n");
            return ret;
        }

        if(ftspi020_dma_wait())
            return ret;

        memcpy(addr, data->mem_dmabuf, len);
#else
        {
            int i, j, tmp_size;
            //printk("len = %d, one time %d, tmp = 0x%x\n",len,data->rxfifo_depth * 4, tmp);

            tmp_size = len % (data->rxfifo_depth * 4);
            //read 2048 bytes       
            for (i = 0; i < (len / (data->rxfifo_depth * 4)); i++) {
                ret = ftspi020_wait_rxfifo_ready(data->io_base);
                if(ret)
                    return ret;                
                for (j = 0; j < data->rxfifo_depth; j++, tmp++) {
                    *tmp = inl(data->io_base + 0x100);
                    //printk("<%x>", *tmp);
                }
                //printk("\n");
            }
            //read 64 bytes
            if (tmp_size) {
                ret = ftspi020_wait_rxfifo_ready(data->io_base);
                if(ret)
                    return ret;                  
                for (i = 0; i < (tmp_size / 4); i++, tmp++) {
                    *tmp = inl(data->io_base + 0x100);
                    //printk("<%x>", *tmp);
                }
            }
            //printk("\n");
        }
#endif
    } else {
        /* write direction */
#ifdef CONFIG_SPI_NAND_USE_AHBDMA

        memcpy(data->mem_dmabuf, addr, len);
        ret = spi020_dma_start(data, len, DMA_MEM_TO_DEV);
        if (ret < 0) {
            printk(KERN_ERR "spi020 dma write fail\n");
            return ret;
        }
        if(ftspi020_dma_wait())
            return ret;
#else
        int i, j, tmp_size;

        tmp_size = len % (data->txfifo_depth * 4);
        //write 2048 bytes      
        for (i = 0; i < (len / (data->txfifo_depth * 4)); i++) {
            ret = ftspi020_wait_txfifo_ready(data->io_base);
            if(ret)
                return ret;
            for (j = 0; j < data->txfifo_depth; j++, tmp++)
                //*(volatile u32 *)(data->chip.IO_ADDR_R) = *tmp++;
                outl(*tmp, data->io_base + 0x100);
        }
        //write 64 bytes
        //*tmp = 0xFF;//good block//??? set bad block?
        if (tmp_size) {
            ret = ftspi020_wait_txfifo_ready(data->io_base);
            if(ret)
                return ret;
            for (i = 0; i < (tmp_size / 4); i++, tmp++) {
                outl(*tmp, data->io_base + 0x100);
                //printk("<%x>", *tmp);
            }
        }
#endif
    }
    return 0;
}

static inline struct common_spi_flash *mtd_to_common_spi_flash(struct mtd_info *mtd)
{
    return container_of(mtd, struct common_spi_flash, mtd);
}

/****************************************************************************/

/*
 * Internal helper functions
 */

/*
 * Read the status register, returning its value in the location
 * Return the status register value.
 * Returns negative if error occurred.
 */
static int read_sr(struct common_spi_flash *flash, u8 ins_cmd)
{
    u8 val;
    struct ftspi020_cmd cmd[2];

    pr_debug("%s: %s\n", dev_name(&flash->spi->dev), __func__);

    memset(&cmd[0], 0, sizeof(struct ftspi020_cmd));
    cmd[0].ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
    cmd[0].write_en = FTSPI020_CMD3_READ;
    cmd[0].read_status_en = FTSPI020_CMD3_RD_STS_EN;
    cmd[0].read_status = FTSPI020_CMD3_STS_SW_READ;
    cmd[0].ins_code = FTSPI020_CMD3_INSTR_CODE(ins_cmd);
    cmd[0].flags = (FTSPI020_XFER_CMD_STATE | FTSPI020_XFER_CHECK_CMD_COMPLETE);

    memset(&cmd[1], 0, sizeof(struct ftspi020_cmd));
    cmd[1].rx_buf = &val;
    cmd[1].data_cnt = 1;
    cmd[1].read_status_en = FTSPI020_CMD3_RD_STS_EN;
    cmd[1].read_status = FTSPI020_CMD3_STS_SW_READ;;
    cmd[1].flags = FTSPI020_XFER_DATA_STATE;

    return val;
}

/*
 * Write status register 1 byte
 * Returns negative if error occurred.
 */
static int write_sr(struct common_spi_flash *flash, u8 * val, u8 len)
{
    struct ftspi020_cmd cmd;

    pr_debug("%s: %s\n", dev_name(&flash->spi->dev), __func__);

    memset(&cmd, 0, sizeof(struct ftspi020_cmd));
    cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
    cmd.write_en = FTSPI020_CMD3_WRITE;
    cmd.ins_code = FTSPI020_CMD3_INSTR_CODE(OPCODE_WRSR);
    cmd.tx_buf = val;
    cmd.data_cnt = len;
    cmd.flags =
        (FTSPI020_XFER_CMD_STATE | FTSPI020_XFER_DATA_STATE | FTSPI020_XFER_CHECK_CMD_COMPLETE);

    return 0;
}

static inline int change_4b(struct common_spi_flash *flash, u8 val)
{
    struct ftspi020_cmd cmd;

    pr_debug("%s: %s\n", dev_name(&flash->spi->dev), __func__);

    memset(&cmd, 0, sizeof(struct ftspi020_cmd));
    cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
    cmd.write_en = FTSPI020_CMD3_WRITE;
    if (val)
        cmd.ins_code = FTSPI020_CMD3_INSTR_CODE(OPCODE_EN4B);
    else
        cmd.ins_code = FTSPI020_CMD3_INSTR_CODE(OPCODE_EX4B);
        
    cmd.flags = (FTSPI020_XFER_CMD_STATE | FTSPI020_XFER_CHECK_CMD_COMPLETE);

    return 0;
}

/*
 * Set write enable latch with Write Enable command.
 * Returns negative if error occurred.
 */
static inline int write_enable(struct common_spi_flash *flash)
{
    struct ftspi020_cmd cmd;

    pr_debug("%s: %s\n", dev_name(&flash->spi->dev), __func__);

    memset(&cmd, 0, sizeof(struct ftspi020_cmd));
    cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
    cmd.write_en = FTSPI020_CMD3_WRITE;
    cmd.ins_code = FTSPI020_CMD3_INSTR_CODE(OPCODE_WREN);
    cmd.flags = (FTSPI020_XFER_CMD_STATE | FTSPI020_XFER_CHECK_CMD_COMPLETE);

    return 0;
}

/*
 * Service routine to read status register until ready, or timeout occurs.
 * Returns non-zero if error.
 */
static int wait_till_ready(struct common_spi_flash *flash)
{
    int count;
    int sr;

    /* one chip guarantees max 5 msec wait here after page writes,
     * but potentially three seconds (!) after page erase.
     */
    for (count = 0; count < MAX_READY_WAIT_COUNT; count++) {
        if ((sr = read_sr(flash, OPCODE_RDSR)) < 0)
            break;
        else if (!(sr & SR_WIP))
            return 0;

        /* REVISIT sometimes sleeping would be best */
        ndelay(10);
    }

    return 1;
}

static inline int flash_set_quad_enable(struct common_spi_flash *flash)
{
    u8 sr[2];

    pr_debug("%s: %s\n", dev_name(&flash->spi->dev), __func__);

    /* Wait until finished previous write command. */
    if (wait_till_ready(flash))
        return 1;

    if ((sr[0] = read_sr(flash, OPCODE_RDSR)) < 0)
        return 1;

    if (flash->flash_type == 1) {       //winbond
        if ((sr[1] = read_sr(flash, OPCODE_RDSR2)) < 0)
            return 1;
        if (sr[1] & (1 << 1))   //has enable
            return 0;
    } else {
        if (sr[0] & (1 << 6))   //has enable
            return 0;
    }
    /* Send write enable, then erase commands. */
    write_enable(flash);
    
    if (flash->flash_type == 1) //winbond
        write_sr(flash, &sr[0], 2);
    else
        write_sr(flash, &sr[0], 1);

    return 0;
}

static int partition_check(struct mtd_partition *partitions, struct ftnandc023_nand_data *data,
                           int block_size)
{
    int i, num = 1;
    int j, A_begin, A_end, B_begin, B_end;
    unsigned int backup_offset = 0, addr;

    spi_nand_sys_header_t *sys_hdr;

    sys_hdr = data->sys_hdr;

    if (sys_hdr->image[0].size == 0) {
        printk(KERN_WARNING "Warning...Not find partition message, use default setting\n");
        partitions[0].name = "a";
        partitions[0].offset = 0x140000;
        partitions[0].size = 0x200000;

        partitions[1].name = "b";
        partitions[1].offset = 0x500000;
        partitions[1].size = 0x200000;

        num = 2;
    } else {
        for (i = 0; i < ARRAY_SIZE(ftnandc023_partition_info); i++) {
            if ((sys_hdr->image[i].size == 0) || ((sys_hdr->image[i].addr % block_size) != 0)) //compatible with 8136, so set 2048
                continue;

            partitions[num].offset = sys_hdr->image[i].addr + backup_offset;

            partitions[num].size = sys_hdr->image[i].size;
            partitions[num].name = sys_hdr->image[i].name;
 
            //printk("MTD%d addr = 0x%x, size = 0x%x\n", num, (unsigned int)partitions[num].offset, (unsigned int)sys_hdr->image[i].size);     
            if (sys_hdr->image[i].addr % block_size) {
                printk(KERN_WARNING "Warning... partition %d addr 0x%x not block alignment, one block = 0x%x\n",
                       i, sys_hdr->image[i].addr, block_size);
                partitions[i].offset = BLOCK_ALIGN(sys_hdr->image[i].addr, block_size);
            }
            if (sys_hdr->image[i].size % block_size) {
                printk(KERN_WARNING "Warning... partition %d size 0x%x not block alignment, one block = 0x%x\n",
                       i, sys_hdr->image[i].size, block_size);
                partitions[i].size = BLOCK_ALIGN(sys_hdr->image[i].size, block_size);
            }
            if(data->lookup_flag) {
            	addr = sys_hdr->image[i].addr + partitions[i].size;
            	if(addr > data->nand_parm.page_size * data->nand_parm.block_pg * data->block_limit)
            		printk(KERN_WARNING "Warning... partition %d addr 0x%x overlap with reserve addr 0x%x to end\n", i, addr, data->nand_parm.page_size * data->nand_parm.block_pg * data->block_limit);
        	}
            num++;
        }
#if 0
        /* system header image */
        partitions[0].offset = 0;
        partitions[0].size = block_size;
        partitions[0].name = "SYSHDR";
#endif
        /* nsboot image */
        partitions[0].offset = block_size;
        partitions[0].size = block_size;
        partitions[0].name = "NSBOOT";

        for (i = 0; i < ARRAY_SIZE(ftnandc023_partition_info); i++)
            ftspi_nand_dbg("partation %d addr = 0x%x, size = 0x%x, name = %s\n", i,
                           sys_hdr->image[i].addr, sys_hdr->image[i].size, sys_hdr->image[i].name);

    //overlap check
    for (i = 0; i < (num - 3); i++) {
        A_begin = partitions[i].offset;
        A_end = A_begin + partitions[i].size;
        //printk("A %x,%x\n",A_begin,A_end);

        for (j = (i + 1); j < num; j++) {
            B_begin = partitions[j].offset;
            B_end = B_begin + partitions[j].size;
            //printk("B %x,%x\n",B_begin,B_end);

            /* A_end between B_end and B_begin */
            if ((B_end >= A_end) && (A_end > B_begin))
                goto check_fail;
            /* A_begin between B_end and B_begin */
            if ((B_end > A_begin) && (A_begin >= B_begin))
                goto check_fail;
            /* B between A */
            if ((A_end >= B_end) && (B_begin >= A_begin))
                goto check_fail;
        }
    }
    }
    return num;
check_fail:
    printk(KERN_WARNING "Warning ============> partition %d overlap with %d\n", i, j);
    return num;
}

/* low enable write protect, high disable write protect */
static void write_protect(int mode)
{
#ifdef CONFIG_GPIO_WP
    if (mode)
        gpio_direction_output(GPIO_PIN, 0);
    else
        gpio_direction_output(GPIO_PIN, 1);
#endif
}

int spi020_flash_cmd(struct ftnandc023_nand_data *data, uint8_t * u8_cmd, void *response,
                     size_t len)
{
    int ret;

    ret = spi_xfer(data, len, u8_cmd, NULL, SPI_XFER_CMD_STATE | SPI_XFER_CHECK_CMD_COMPLETE);
    if (ret) {
        printk(KERN_ERR "SF: Failed to send command %02x: %d\n", u8_cmd[0], ret);
        return ret;
    }

    if (len && response != NULL) {
        ret = spi_xfer(data, len, NULL, response, SPI_XFER_DATA_IN_STATE);
        if (ret) {
            printk(KERN_ERR "SF: Failed to read response (%zu bytes): %d\n", len, ret);
        }
    } else if ((len && response == NULL) || (!len && response != NULL)) {
        printk
            (KERN_ERR "SF: Failed to read response due to the mismatch of len and response (%zu bytes): %d\n",
             len, ret);
    }

    return ret;
}

/* Wait until BUSY bit clear
 */
static int flash_wait_busy_ready(struct ftnandc023_nand_data *data)
{
    unsigned long timeo = jiffies;
    u8 rd_sts_cmd[1];
    u8 status = 0xFF;

    rd_sts_cmd[0] = COMMAND_GET_FEATURES;
    rd_sts_cmd[1] = 0xC0;

    timeo += (10 * HZ);
    while (time_before(jiffies, timeo)) {
        if (spi020_flash_cmd(data, rd_sts_cmd, NULL, 0)) {
            printk(KERN_ERR "Failed to check status!\n");
            break;
        }

        status = ftspi020_read_status(data->io_base);
        if (!(status & FLASH_STS_BUSY))
            break;
    };
    if (status & FLASH_STS_BUSY) {
        printk(KERN_ERR "Wait bus busy timeout!\n");
        return 0xFF;
    } else {
        ftspi_nand_dbg("get feature = 0x%x\n", status);
        //printk("<get feature = 0x%x>", status);
    }

    return status;
}

static int ftnandc023_nand_wait(struct mtd_info *mtd, struct nand_chip *chip)
{
    ftspi_nand_dbg("%s\n", __FUNCTION__);
    return cmdfunc_status;
}

static int ftnandc023_available_oob(struct ftnandc023_nand_data *data)
{
    int ret = 0;

    ret = data->chip.ecc.layout->oobfree[0].length * 4;

        /*----------------------------------------------------------
	 * YAFFS require 16 bytes OOB without ECC, 28 bytes with
	 * ECC enable.
	 * BBT require 5 bytes for Bad Block Table marker.
	 */
#ifdef CONFIG_YAFFS_FS
    if (ret >= 16) {
        printk(KERN_INFO "NAND(YAFFS): avaliable OOB is %d byte.\n", ret);
    } else {
        printk(KERN_INFO
               "NAND: Not enough OOB:%d bytes(YAFFS requires 16 bytes without software ECC, "
               "28 bytes with ECC enable), try to reduce ECC correction bits.\n", ret);
    }
#else
    printk(KERN_INFO "Avaliable OOB is %d byte.\n", ret);
#endif
    return ret;
}

static void nand_read_buffer(struct ftnandc023_nand_data *data, int cmd, u_char * buf, int len)
{
    if (cmd == NAND_CMD_READID) {
        ftspi_nand_dbg("read_buffer ID\n");
        ftspi020_data_access(data, 1, buf, IDCODE_LEN);
        //*(u32 *)buf=0xF1C8F1C8;
    }
}

static uint8_t ftnandc023_read_byte(struct mtd_info *mtd)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    uint8_t b = 0xFF;

    ftspi_nand_dbg("%s, cmd = 0x%x\n", __FUNCTION__, data->cur_cmd);

    switch (data->cur_cmd) {
    case NAND_CMD_READID:
        if (read_id_time == 0) {
            //buf = *(u32 *)(data->chip.IO_ADDR_R)

            nand_read_buffer(data, NAND_CMD_READID, idcode, IDCODE_LEN);
            ftspi_nand_dbg("ID = 0x%x,0x%x,0x%x,0x%x\n", idcode[0], idcode[1], idcode[2],
                           idcode[3]);

            read_id_time = 1;
        }
        b = idcode[ID_point++];
        break;
    case NAND_CMD_STATUS:
        /* fail to return 0 */
        ftspi_nand_dbg("NAND_CMD_STATUS\n");    //not need to implement, r/w/e already do it.
        break;
    }
    
    return b;
}

int log_to_phy(struct ftnandc023_nand_data *data, int page_addr)
{
    int log_block_num, phy_block_num, ren, new_page_addr;
    
    log_block_num = page_addr / data->nand_parm.block_pg;
    ren = page_addr % data->nand_parm.block_pg;

    /* read system header and lookup tab */
    if(log_block_num == 0)
        return page_addr;

    phy_block_num = data->lookup_table->physical[log_block_num];

    if(phy_block_num > 0x8000) {
        printk(KERN_WARNING "Block %d not be mapping\n", log_block_num);
        return 0x0FFFFFFF;
    } else if(phy_block_num == 0xFFFF) {
        printk(KERN_WARNING "Block %d is bad\n", log_block_num);
        return 0x0FFFFFFF;
    }

    new_page_addr = phy_block_num * data->nand_parm.block_pg + ren;
    
    ftspi_nand_dbg("page in = %d, log_b = %d, phy_b = %d, page out = %d\n", page_addr, log_block_num, phy_block_num, new_page_addr);
    return new_page_addr;
}

static int ftnandc023_nand_read_oob_lp(struct mtd_info *mtd, u_char * buf)
{
    int ret, new_page;    
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;

    ftspi_nand_dbg("%s\n", __FUNCTION__);

    if(data->lookup_flag) {
        new_page = log_to_phy(data, data->page_addr);
        ftspi_nand_dbg("read_oob page num logical = %d, physical = %d\n", data->page_addr, new_page);
        if(new_page == 0x0FFFFFFF) {
            printk(KERN_ERR "Try to write bad block\n");
            return -1;
        }
        data->page_addr = new_page;
    }
    else
        data->page_addr = calc_new_page(mtd, data->page_addr);

	ret = spi_flash_cmd_read_fast(data, data->page_addr, mtd->oobsize, buf);

    if(ret) {
        cmdfunc_status = NAND_STATUS_FAIL;
        return -EIO;
    } else {
        cmdfunc_status = NAND_STATUS_READY;
        return 0;
    }
}

static int ftnandc023_nand_read_oob_sp(struct mtd_info *mtd, u_char * buf)
{
    printk("%s: not implement.\n", __FUNCTION__);
    return 0;
}

static int ftnandc023_nand_read_oob_std(struct mtd_info *mtd, struct nand_chip *chip, int page, int sndcmd)
{
    struct ftnandc023_nand_data *data = chip->priv;

    ftspi_nand_dbg("read oob page = 0x%x\n", page);
    data->page_addr = page;

    return data->read_oob(mtd, chip->oob_poi);
}

static int spi020_flash_cmd_write(struct ftnandc023_nand_data *spi, u8 * u8_cmd, const void *data, int data_len)
{
    int ret;

    ret = spi_xfer(spi, data_len, u8_cmd, NULL, SPI_XFER_CMD_STATE);
    if (ret) {
        printk(KERN_ERR "SF: Failed to send command %02x: %d\n", u8_cmd[0], ret);
        return ret;
    } else if (data_len != 0) {
        ret = spi_xfer(spi, data_len, data, NULL, SPI_XFER_DATA_OUT_STATE | SPI_XFER_CHECK_CMD_COMPLETE);       //SPI_XFER_CHECK_CMD_COMPLETE);
        if (ret) {
            printk(KERN_ERR "SF: Failed to write data (%zu bytes): %d\n", data_len, ret);
        }
    }

    return ret;
}

static int spi020_flash_cmd_read(struct ftnandc023_nand_data *spi, uint8_t * u8_cmd, void *data, int data_len)
{
    int ret;

    ret = spi_xfer(spi, data_len, u8_cmd, NULL, SPI_XFER_CMD_STATE);
//printk("spi020_flash_cmd_read cmd=0x%x,len=0x%x\n",*u8_cmd,data_len);    
    if (ret) {
        printk(KERN_ERR "SF: Failed to send command %02x: %d\n", u8_cmd[0], ret);
    } else if (data_len != 0) {
        //printk("A0\n");
        ret =
            spi_xfer(spi, data_len, NULL, data,
                     SPI_XFER_DATA_IN_STATE | SPI_XFER_CHECK_CMD_COMPLETE);
        if (ret) {
            printk(KERN_ERR "SF: Failed to read data (%zu bytes): %d\n", data_len, ret);
        }
    }

    return ret;
}

static int spi_flash_read_write(struct ftnandc023_nand_data *spi,
                                const u8 * cmd, size_t cmd_len,
                                const u8 * data_out, u8 * data_in, size_t data_len)
{
    unsigned long flags;
    int ret;

    if (data_len == 0)
        flags = (SPI_XFER_CMD_STATE | SPI_XFER_CHECK_CMD_COMPLETE);     //(SPI_XFER_END | SPI_XFER_CHECK_CMD_COMPLETE);

    ret = spi_xfer(spi, cmd_len * 8, cmd, NULL, flags);
    
    if (ret) {
        printk(KERN_ERR "SF: Failed to send command (%zu bytes): %d\n", cmd_len, ret);
    } else if (data_len != 0) {
        if (data_in == NULL) {  // write
            if (data_len % 4) {
                if (*cmd != COMMAND_WRITE_STATUS)
                    printk(KERN_WARNING "data len %x not 4 times\n", data_len);
                return ret;
            }
        }
        ret = spi_xfer(spi, data_len * 8, data_out, data_in, SPI_XFER_CHECK_CMD_COMPLETE);      //SPI_XFER_END, 0);
        if (ret)
            printk(KERN_ERR "SF: Failed to transfer %zu bytes of data: %d\n", data_len, ret);
    }

    return ret;
}

static int spi_flash_cmd(struct ftnandc023_nand_data *spi, u8 * cmd, void *response, size_t len)
{
    return spi_flash_read_write(spi, cmd, 1, NULL, response, len);
}

#ifdef CONFIG_NAND_DISTRUB
int read_ecc_limit = 0;
volatile int mm = 0;
#endif

static int spi_flash_cmd_read_fast(struct ftnandc023_nand_data *data, u32 offset, size_t len, u8 *buf)
{
    int ret;
    u8 u8_rd_cmd[5], status = 0;

    /* send PAGE READ command first for SPI NAND */
    //flash_wait_busy_ready(flash);

    memset(u8_rd_cmd, 0, 5);
    u8_rd_cmd[0] = COMMAND_PAGE_READ;
    /* assign row address */
    u8_rd_cmd[1] = offset & 0xFF;
    u8_rd_cmd[2] = ((offset & 0xFF00) >> 8);
    u8_rd_cmd[3] = ((offset & 0xFF0000) >> 16);

    ret = spi_flash_cmd(data, u8_rd_cmd, NULL, 0);
    if (ret) {
        printk(KERN_ERR "Read fast fail\n");
        return ret;
    }
    
    /* check if the flash is busy */
    status = flash_wait_busy_ready(data);
    if ((status & FLASH_NAND_R_FIELD ) == FLASH_NAND_R_FAIL) {
        printk(KERN_ERR "read offset = 0x%x ECC fail\n", offset * data->nand_parm.page_size);
#if 0//debug all good
        if(data->nand_parm.spare_size == len)
            *buf = 0xFF;
        return 0;
#else         
        return 1;
#endif
    }

#ifdef CONFIG_NAND_DISTRUB
#if 1    
    if ((status & FLASH_NAND_R_FIELD ) == data->ecc_type)//justin
    	read_ecc_limit = 1;
#else //debug
    mm++;
    if(mm < 5)
    	read_ecc_limit = 1;
#endif
#endif

    memset(u8_rd_cmd, 0, 5);

#ifdef CONFIG_SPI_QUAD
    flash_set_quad_enable(data, 1);
    u8_rd_cmd[0] = COMMAND_FAST_READ_QUAD_IO;
#else
	if(idcode[0] == 0x9B)        //ATO not have COMMAND_FAST_READ_DUAL mode
		u8_rd_cmd[0] = COMMAND_FAST_READ;
   	else
		u8_rd_cmd[0] = COMMAND_FAST_READ_DUAL;
#endif

    /* assign column address */
    if (len == data->nand_parm.spare_size) {
        u8_rd_cmd[1] = data->nand_parm.page_size & 0xFF;
        u8_rd_cmd[2] = ((data->nand_parm.page_size & 0xFF00) >> 8);
    } else {
        u8_rd_cmd[1] = 0x00;
        u8_rd_cmd[2] = 0x00;
    }    

    ret = spi020_flash_cmd_read(data, u8_rd_cmd, buf, len);

    return ret;
}

/* 0 is good, 1 is bad */
static int check_bbt(struct mtd_info *mtd, loff_t offs)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    unsigned int *bi_table = (unsigned int *)data->bi_table;
    int quotient, remainder, blk_id, result, ret = 0;

    if(!data->lookup_flag) {
        blk_id = offs >> chip->bbt_erase_shift;
        quotient = blk_id >> 5;
        remainder = blk_id & 0x1F;
        result = (bi_table[quotient] >> remainder) & 0x1;
        
        if (result != 1)
            ret = 1;
    }
    return ret;
}

static int calc_new_page(struct mtd_info *mtd, int page_addr)
{
    unsigned int start_addr, end_addr, check_addr, tmp_addr, bad_num = 0;
    unsigned int block_page_num = mtd->erasesize / mtd->writesize;
    struct nand_chip *chip = mtd->priv;

    if (root_mtd_num != 0xFFFF) {// is squashfs
        start_addr = ftnandc023_partition_info[root_mtd_num].offset;
        end_addr = ftnandc023_partition_info[root_mtd_num].size + start_addr;
        //printk("squash start addr = 0x%x, end addr = 0x%x\n", start_addr, end_addr); 

        check_addr = page_addr * mtd->writesize;
        
        if((check_addr >= start_addr) && (check_addr < end_addr)) {
            tmp_addr = start_addr;
            
            /* have bad block between start to here? */
            while (check_addr >= tmp_addr) {
                if(check_bbt(mtd, (loff_t) tmp_addr) != 0) {
                    bad_num++;
                    //printk("<ba=0x%x>", tmp_addr);
                }

                tmp_addr += (0x1 << chip->bbt_erase_shift);
            }
            
            /* move n good block */
            if(bad_num) {
                //printk("<br0x%x,bad=%d>",page_addr,bad_num);
                while(bad_num) {
                    //printk("<tmp_page = 0x%x>", tmp_addr / mtd->writesize);
                    if(check_bbt(mtd, (loff_t) tmp_addr) == 0)
                        bad_num--;
    
                    tmp_addr += (0x1 << chip->bbt_erase_shift);
                    page_addr += block_page_num;                    
                }
                //printk("<a=0x%x>\n",page_addr);
            }                        
        } else {
                while(check_bbt(mtd, (loff_t) (page_addr * mtd->writesize)))
                    page_addr += block_page_num;
        }
    }

    return page_addr;
}

static int ftnandc023_nand_read_page_lp(struct mtd_info *mtd, u_char * buf)
{
    int ret, new_page;
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;

    ftspi_nand_dbg("r:page = 0x%x, size = %d\n", data->page_addr, mtd->writesize);

    if(data->lookup_flag) {
        new_page = log_to_phy(data, data->page_addr);
        ftspi_nand_dbg("read_page page num logical = %d, physical = %d\n", data->page_addr, new_page);
        if(new_page == 0x0FFFFFFF) {
            printk(KERN_ERR "Try to write bad block\n");
            return -1;
        }
        data->page_addr = new_page;
    }
    else
        data->page_addr = calc_new_page(mtd, data->page_addr);
    
    //spi_flash_cmd_read_fast(data, data->page_addr, data_len, data->chip.buffers->databuf);
    //memcpy(buf, data->chip.buffers->databuf, mtd->writesize);
    
    /* nand_do_read_ops will reserve data->chip.buffers->databuf data last time, can't be overlap */
    ret = spi_flash_cmd_read_fast(data, data->page_addr, mtd->writesize, buf);
    if(ret) {
        cmdfunc_status = NAND_STATUS_FAIL;
        return -EIO;
    } else {
        cmdfunc_status = NAND_STATUS_READY;
        return 0;
    }
}

static int ftnandc023_nand_read_page_sp(struct mtd_info *mtd, u_char * buf)
{
    printk("%s: not implement.\n", __FUNCTION__);
    return 0;
}

static int ftnandc023_nand_read_page(struct mtd_info *mtd, struct nand_chip *chip, uint8_t * buf,
                                     int page)
{
    struct ftnandc023_nand_data *data = chip->priv;

    ftspi_nand_dbg("%s, page = 0x%x\n", __FUNCTION__, page);
    data->buf = (u32 *) buf;
    data->page_addr = page;

    return data->read_page(mtd, buf);
}

static int spi_flash_cmd_write(struct ftnandc023_nand_data *data, u32 offset, size_t len,
                        const uint8_t * buf)
{
    int ret = 1;
    u8 u8_wr_en_cmd[1], u8_wr_cmd[5];
    u8 *u8_buf = (u8 *) buf;

    /* check if the flash is busy */
    //flash_wait_busy_ready(flash);

#ifdef CONFIG_SPI_QUAD
    ret = flash_set_quad_enable(data, 1);
    if (ret)
        goto prog0_exit;
#endif

    u8_wr_en_cmd[0] = COMMAND_WRITE_ENABLE;
    if (spi020_flash_cmd(data, u8_wr_en_cmd, NULL, 0)) {
        printk(KERN_ERR "COMMAND_WRITE_ENABLE fail\n");
        goto prog_exit;
    }

    /* send PROGRAM LOAD command for SPI NAND */
    u8_wr_cmd[0] = COMMAND_PROGRAM_LOAD;
    /* assign column address */
    if (len == data->nand_parm.spare_size) {
        u8_wr_cmd[1] = data->nand_parm.page_size & 0xFF;
        u8_wr_cmd[2] = ((data->nand_parm.page_size & 0xFF00) >> 8);
    } else {
        u8_wr_cmd[1] = 0x00;
        u8_wr_cmd[2] = 0x00;
    }

    ret = spi020_flash_cmd_write(data, u8_wr_cmd, u8_buf, len);
    if (ret) {
        printk(KERN_ERR "COMMAND_WRITE_ENABLE fail\n");
        goto prog_exit;
    }
    //ftspi020_wait_cmd_complete(data);

#ifdef CONFIG_SPI_QUAD
    //only for QUAD_PROGRAM
    if (data->chip_type)
        u8_wr_cmd[0] = COMMAND_WINBOND_QUAD_WRITE_PAGE;
    else
        u8_wr_cmd[0] = COMMAND_QUAD_WRITE_PAGE;
#else
    u8_wr_cmd[0] = COMMAND_PROGRAM_EXECUTE;
#endif

    /* assign row address */
    u8_wr_cmd[1] = offset & 0xFF;
    u8_wr_cmd[2] = ((offset & 0xFF00) >> 8);
    u8_wr_cmd[3] = ((offset & 0xFF0000) >> 16);

    if (spi020_flash_cmd(data, u8_wr_cmd, NULL, 0)) {
        printk(KERN_ERR "COMMAND_PROGRAM_EXECUTE fail\n");
        goto prog_exit;
    }

    if (flash_wait_busy_ready(data) & FLASH_NAND_P_FAIL) {
        printk(KERN_ERR "program result fail\n");
        ret = 1;
    } else
        ret = 0;       
        
prog_exit:
    return ret;
}

static int ftnandc023_nand_write_oob_lp(struct mtd_info *mtd, const uint8_t * buf, int len)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    int ret, new_page;

    ftspi_nand_dbg("oob w:page = 0x%x, size = %d, data = 0x%x\n", data->page_addr, len, *buf);

    //memcpy(data->chip.buffers->databuf + FLASH_NAND_PAGE_SZ, buf, mtd->oobsize);
    if(data->lookup_flag) {
        new_page = log_to_phy(data, data->page_addr);
        ftspi_nand_dbg("write_oob page num logical = %d, physical = %d\n", data->page_addr, new_page);
        if(new_page == 0x0FFFFFFF) {
            printk(KERN_ERR "Try to write bad block\n");
            return -1;
        }
        data->page_addr = new_page;
    }

	ret = spi_flash_cmd_write(data, data->page_addr, mtd->oobsize, buf);	
    if(ret) {
        cmdfunc_status = NAND_STATUS_FAIL;
        return -EIO;
    } else {
        cmdfunc_status = NAND_STATUS_READY;
        return 0;
    }
}

static int ftnandc023_nand_write_oob_sp(struct mtd_info *mtd, const u_char * buf, int len)
{
    printk("%s: not implement.\n", __FUNCTION__);
    return 0;
}

#if 0
static int ftnandc023_nand_write_oob_std(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
    struct ftnandc023_nand_data *data = chip->priv;

    printk("write oob page = 0x%x\n", page);
    data->page_addr = page;

    memcpy(data->chip.buffers->databuf + FLASH_NAND_PAGE_SZ, chip->oob_poi, mtd->oobsize);
    data->write_page(mtd, data->chip.buffers->databuf);

    printk("ddw = 0x%x, 0x%x, 0x%x\n", *(u32 *) chip->oob_poi, *(u32 *) (chip->oob_poi + 1),
           *(u32 *) (chip->oob_poi + 2));
    return true;                //data->write_oob(mtd, chip->oob_poi, mtd->oobsize);
}
#else
static int ftnandc023_nand_write_oob_std(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
    struct ftnandc023_nand_data *data = chip->priv;
    //u32 *tmp = (u32 *)chip->oob_poi;

    ftspi_nand_dbg("write oob page = 0x%x\n", page);
    data->page_addr = page;

    //printk("ddw = 0x%x, 0x%x, 0x%x\n", *tmp, *(tmp + 1), *(tmp + 2));
    return data->write_oob(mtd, chip->oob_poi, mtd->oobsize);
}
#endif

static int ftnandc023_nand_write_page_lp(struct mtd_info *mtd, const uint8_t * buf)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    int ret, new_page;

    ftspi_nand_dbg("lp w:page = 0x%x, size = %d, data->column = %d,0x%x\n", data->page_addr,
                   mtd->writesize, data->column, *buf);

    if(data->lookup_flag) {
        new_page = log_to_phy(data, data->page_addr);
        ftspi_nand_dbg("write_page page num logical = %d, physical = %d\n", data->page_addr, new_page);
        if(new_page == 0x0FFFFFFF) {
            printk(KERN_ERR "Try to write bad block\n");
            return -1;
        }
        data->page_addr = new_page;
    }
  
    ret = spi_flash_cmd_write(data, data->page_addr, mtd->writesize, buf);    
#if 0//debug
	if(mm == 0) {
		mm ++;
		return -1;
	} else {
		return 0;
	}
#else
    if(ret) {
        cmdfunc_status = NAND_STATUS_FAIL;
        return -EIO;
    } else {
        cmdfunc_status = NAND_STATUS_READY;
        return 0;
    }
#endif
}

static int ftnandc023_nand_write_page_sp(struct mtd_info *mtd, const uint8_t * buf)
{
    printk("%s: not implement.\n", __FUNCTION__);
    return 0;
}

static int ftnandc023_nand_write_page(struct mtd_info *mtd, struct nand_chip *chip,
                                      const uint8_t * buf, int page, int cached, int raw)
{
    struct nand_chip *p = mtd->priv;
    struct ftnandc023_nand_data *data = p->priv;
    int status = 0;

    ftspi_nand_dbg("%s, page = 0x%x\n", __FUNCTION__, page);
    data->page_addr = page;

    status = data->write_page(mtd, buf);

    return status;
}

static void ftnandc023_nand_write_page_lowlevel(struct mtd_info *mtd,
                                                struct nand_chip *chip, const uint8_t * buf)
{
}

/* offset: byte offset
 * len: how many bytes.
 */
static int spi_flash_cmd_erase(struct ftnandc023_nand_data *data, u32 offset)
{
    int ret = 1, new_page;
    u8 u8_er_cmd[5];

    /* check if the flash is busy */
    //flash_wait_busy_ready(data);
    if(data->lookup_flag) {
        new_page = log_to_phy(data, offset);
        ftspi_nand_dbg("erase page num logical = %d, physical = %d\n", offset, new_page);
        if(new_page == 0x0FFFFFFF) {
            printk(KERN_ERR "Try to erase bad block\n");
            return 1;
        }
        offset = new_page;
    }

    u8_er_cmd[0] = COMMAND_WRITE_ENABLE;
    ret = spi020_flash_cmd(data, u8_er_cmd, NULL, 0);
    if (ret) {
        printk(KERN_ERR "cmd_erase write enable fail\n");
        return ret;
    }

    u8_er_cmd[0] = COMMAND_ERASE_128K_BLOCK;
    u8_er_cmd[1] = offset & 0xFF;
    u8_er_cmd[2] = ((offset & 0xFF00) >> 8);
    u8_er_cmd[3] = ((offset & 0xFF0000) >> 16);

    ret = spi020_flash_cmd(data, u8_er_cmd, NULL, 0);
    if (ret) {
        printk(KERN_ERR "cmd_erase erase fail\n");
        return ret;
    }

    /* check if the flash is busy */
    if (flash_wait_busy_ready(data) & FLASH_NAND_E_FAIL) {
        printk(KERN_ERR "erase result fail\n");
        ret = 1;
    } else
        ret = 0;        

#if 0//debug
	if(mm == 0) {
		mm ++;
		ret = 1;
	} else {
		ret = 0;
	}
#endif
    return ret;
}

static void ftnandc023_nand_cmdfunc(struct mtd_info *mtd, unsigned command,
                                    int column, int page_addr)
{
    int ret;
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;

    ftspi_nand_dbg("%s, cmd = 0x%x\n", __func__, command);

    data->cur_cmd = command;

    switch (command) {
    case NAND_CMD_READID:
        {
            ftspi020_cmd_t spi_cmd;

            read_id_time = 0;
            ID_point = 0;
            ftspi020_setup_cmdq(data, &spi_cmd, COMMAND_READ_ID, FTSPI020_CMD1_OP_CODE_1_BYTE,
                                FTSPI020_CMD3_READ, FTSPI020_CMD3_SERIAL_MODE, IDCODE_LEN);
            ftspi020_wait_cmd_complete(data);
        }
        break;
    case NAND_CMD_RESET:
        break;
    case NAND_CMD_STATUS:
        break;
    case NAND_CMD_ERASE1:
        write_protect(0);
        //DBGLEVEL2(ftnandc023_dbg("erase page: 0x%x\n", data->page_addr));
        ftspi_nand_dbg("erase sector: 0x%x\n", page_addr);
        ret = spi_flash_cmd_erase(data, page_addr);
        if(ret)
            cmdfunc_status = NAND_STATUS_FAIL;
        else
            cmdfunc_status = NAND_STATUS_READY;

        break;
    case NAND_CMD_ERASE2:
    case NAND_CMD_PAGEPROG:
    case NAND_CMD_SEQIN:
        break;
    }
}

/*
 * Currently, we have pin mux with SD card
 */
static void ftnandc023_nand_select_chip(struct mtd_info *mtd, int chip)
{
    struct nand_chip *p = mtd->priv;
    struct ftnandc023_nand_data *data = p->priv;
    int chn = 0;

    //DBGLEVEL2(ftnandc023_dbg("chip = %d, ", chip));
    if (data->scan_state != 1) {
        while (chip != -1) {
            if (chip < data->valid_chip[chn]) {
                break;
            } else {
                chip = chip - data->valid_chip[chn];
                chn++;
            }
        }
        data->cur_chan = chn;
    }
#ifdef CONFIG_FTNANDC023_HYNIX_HY27US08561A
    if (chip == 1)
        data->sel_chip = 2;
    else if (chip == 2)
        data->sel_chip = 1;
    else
        data->sel_chip = chip;
#else
    data->sel_chip = chip;
#endif

    //DBGLEVEL2(ftnandc023_dbg("==>chan = %d, ce = %d\n", data->cur_chan, data->sel_chip));
}

/*
 * Probe for the NAND device.
 */
static int __devinit ftspi020_nand_probe(struct platform_device *pdev)
{
    struct ftnandc023_nand_data *data;
    struct mtd_partition *partitions;
    int res, chipnum, size;
    static struct resource *irq;
    int i;
    int partitions_num;

    res = chipnum = size = 0;
    /* Allocate memory for the device structure (and zero it) */
    data = kzalloc(sizeof(struct ftnandc023_nand_data), GFP_KERNEL);
    if (!data) {
        dev_err(&pdev->dev, "failed to allocate device structure.\n");
        res = -ENOMEM;
        goto out;
    }

    data->io_base = ioremap_nocache(pdev->resource[0].start,
                                    pdev->resource[0].end - pdev->resource[0].start + 1);
    printk(KERN_INFO "SPI-NAND reg mapping to addr = 0x%x, phy = 0x%x\n", (u32) data->io_base,
           (u32) pdev->resource[0].start);
    if (data->io_base == NULL) {
        dev_err(&pdev->dev, "ioremap failed for register.\n");
        res = -EIO;
        goto out_free_data;
    }
    
#if defined(CONFIG_SPI_NAND_USE_AHBDMA) || defined(CONFIG_SPI_NAND_USE_AXIDMA)
    init_waitqueue_head(&dma_queue);
    data->nand_dmaaddr = pdev->resource[0].start + 0x100;
#endif

    data->chip.IO_ADDR_R = data->io_base + 0x100;
    printk(KERN_INFO "NAND data port mapping to addr = 0x%x\n", (u32) data->chip.IO_ADDR_R);

    irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    if (!irq) {
        printk(KERN_ERR "no irq resource\n");
        res = -ENODEV;
        goto out_free_data;
    }

    setup_clk(data->io_base);

    /* Currently, it is fixed in LEGACY_LARGE
     */
    legacy = LEGACY_LARGE;
	data->flash_type = 0;
    data->large_page = 1;
    data->rxfifo_depth = ftspi020_rxfifo_depth(data->io_base);
    data->txfifo_depth = ftspi020_txfifo_depth(data->io_base);
    data->chip.priv = data;
    data->mtd.priv = &data->chip;
    data->mtd.owner = THIS_MODULE;
    data->mtd.name = pdev->dev.init_name;
    data->dev = &pdev->dev;
    data->chip.IO_ADDR_W = data->chip.IO_ADDR_R;
    data->chip.select_chip = ftnandc023_nand_select_chip;
    data->chip.cmdfunc = ftnandc023_nand_cmdfunc;
    data->chip.read_byte = ftnandc023_read_byte;
    data->chip.write_page = ftnandc023_nand_write_page;
    data->chip.waitfunc = ftnandc023_nand_wait;
    data->chip.block_markbad = ftnandc023v2_block_markbad;
    data->chip.chip_delay = 0;
    //data->chip.options = NAND_BBT_USE_FLASH | NAND_NO_SUBPAGE_WRITE | NAND_OWN_BUFFERS;
    data->chip.options = NAND_NO_SUBPAGE_WRITE | NAND_OWN_BUFFERS;      /* remove NAND_BBT_USE_FLASH */

    platform_set_drvdata(pdev, data);

    data->scan_state = 1;
    
    /* read ID disable DMA */
    ftspi020_dma_disable(data->io_base);
    
    /* Scan to find existance of the device */
    for (i = startchn; i < MAX_CHANNEL; i++) {
        printk(KERN_INFO "NAND: Scan Channel %d...\n", i);
        data->cur_chan = i;
        if (!nand_scan_ident(&data->mtd, MAX_CE, NULL)) {
            if ((0xFFFFFFFF - size) > (data->mtd.size)
                && ((chipnum + data->chip.numchips) <= NAND_MAX_CHIPS)) {
                data->valid_chip[i] = data->chip.numchips;
                chipnum += data->chip.numchips;
                size += (chipnum * data->chip.chipsize);
            } else {
                printk(KERN_INFO "Can not accept more flash chips.\n");
                break;
            }
        }
    }
    printk(KERN_INFO "NAND: Scan Channel finish, chipnum = %d\n", chipnum);
    if (chipnum == 0) {
        res = -ENXIO;
        goto out_unset_drv;
    }
    
#ifdef CONFIG_SPI_NAND_USE_AHBDMA
    ftspi020_dma_enable(data->io_base);
#endif

    data->chip.numchips = chipnum;
    data->mtd.size = size;
    data->scan_state = 0;

	data->ecc_type = FLASH_NAND_R_ECC_LIMIT;
    if(idcode[1] == 0xF1) {	//GD5F1GQ4UA
        data->chip.ecc.layout = &nand_1_hw_eccoob;
    } else if(((idcode[0] == 0xC8) && (idcode[1] == 0x20)) || ((idcode[0] == 0xC8) && (idcode[1] == 0x21))){
        data->chip.ecc.layout = &nand_2_hw_eccoob;
    } else if((idcode[0] == 0xEF) || (idcode[0] == 0xC2)) {	//MXIC & Winbond
        data->chip.ecc.layout = &nand_3_hw_eccoob;
    } else if(idcode[0] == 0x91) {	//HY
        data->chip.ecc.layout = &nand_4_hw_eccoob;
    } else {	//GD5FxGQ4UB
        data->chip.ecc.layout = &nand_0_hw_eccoob;
        //data->ecc_type = FLASH_NAND_R_ECC_LIMIT_GD;
    }
    data->chip.bbt_td = &ftnandc023_bbt_main_descr;
    data->chip.bbt_md = &ftnandc023_bbt_mirror_descr;
    data->chip.badblock_pattern = &ftnandc023_largepage_flashbased;

    data->ce = 0;               //???

    data->chip.ecc.mode = NAND_ECC_HW;

    data->chip.ecc.size = data->mtd.writesize;
    data->chip.ecc.bytes = 0;
    data->chip.ecc.read_page = ftnandc023_nand_read_page;
    data->chip.ecc.write_page = ftnandc023_nand_write_page_lowlevel;
    data->chip.ecc.read_oob = ftnandc023_nand_read_oob_std;
    data->chip.ecc.write_oob = ftnandc023_nand_write_oob_std;
    data->chip.ecc.read_page_raw = ftnandc023_nand_read_page;

    if (data->large_page) {
        data->read_page = ftnandc023_nand_read_page_lp;
        data->write_page = ftnandc023_nand_write_page_lp;
        data->read_oob = ftnandc023_nand_read_oob_lp;
        data->write_oob = ftnandc023_nand_write_oob_lp;
    } else {
        data->read_page = ftnandc023_nand_read_page_sp;
        data->write_page = ftnandc023_nand_write_page_sp;
        data->read_oob = ftnandc023_nand_read_oob_sp;
        data->write_oob = ftnandc023_nand_write_oob_sp;
    }
    
#if defined(CONFIG_SPI_NAND_USE_AHBDMA) || defined(CONFIG_SPI_NAND_USE_AXIDMA)
    dma_cap_set(DMA_SLAVE, data->cap_mask);

#ifdef CONFIG_SPI_NAND_USE_AHBDMA
    printk(KERN_INFO "use AHB DMA mode\n");
    {
        struct ftdmac020_dma_slave slave;
        memset(&slave, 0, sizeof(slave));
        data->dma_chan = dma_request_channel(data->cap_mask, ftdmac020_chan_filter, (void *)&slave);    //step 1
    }
#else
    ftpmu010_write_reg(nand_fd, 0xA4, (0x0 << 27), (0x1 << 27));
    printk(KERN_INFO "use AXI DMA mode\n");
    {
        struct ftdmac030_dma_slave slave;
        memset(&slave, 0, sizeof(slave));
        data->dma_chan = dma_request_channel(data->cap_mask, ftdmac030_chan_filter, (void *)&slave);    //step 1
    }
#endif

    if (!data->dma_chan) {
        dev_err(&pdev->dev, "DMA channel allocation failed\n");
        res = -ENODEV;
        goto out_free_buf;
    }
    printk(KERN_INFO "Nand get DMA channel %d\n", data->dma_chan->chan_id);

    data->mem_dmabuf =
        dma_alloc_coherent(&pdev->dev, data->mtd.writesize, &data->mem_dmaaddr, GFP_KERNEL);
    if (!data->mem_dmabuf) {
        dev_err(&pdev->dev, "failed to allocate dma buffers.\n");
        res = -ENOMEM;
        goto out_free_dma;
    }
    data->sg_dmabuf = dma_to_virt(&pdev->dev, data->mem_dmaaddr);

    //printk("sg mem pa = 0x%x, va = 0x%x\n", (u32)data->mem_dmaaddr, (u32)data->sg_dmabuf);
#else
    printk(KERN_INFO "use PIO mode\n");
#endif

    /* read the system header first 
     */
    if (1) {
        /* read system header 
         */
        //data->sys_hdr = kzalloc(data->mtd.writesize, GFP_KERNEL);
        data->sys_hdr =
            dma_alloc_coherent(&pdev->dev, data->mtd.writesize, &data->syshd_dmaaddr, GFP_KERNEL);
        if (data->sys_hdr == NULL) {
            printk(KERN_ERR "Warning............SPI-NAND: can't alloc memory");
            return -ENOMEM;
        }
        
        data->chip.buffers = kmalloc(sizeof(struct nand_buffers), GFP_KERNEL);//kmalloc(data->mtd.writesize + (spare << 2), GFP_KERNEL);
        if (!data->chip.buffers) {
            dev_err(&pdev->dev, "failed to allocate chip buffers.\n");
            res = -ENOMEM;
            goto out_unset_drv;
        }
        else if((sizeof(struct nand_buffers) <  data->mtd.writesize)) {
    		dev_err(&pdev->dev, "Please adjust the NAND_MAX_OOBSIZE & NAND_MAX_PAGESIZE\n");
    		dev_err(&pdev->dev, "Flash Page size:%d, but NAND_MAX_PAGESIZE is %d\n", data->mtd.writesize, NAND_MAX_PAGESIZE);
    		res = -ENOMEM;
    		goto out_unset_drv;
	    }

        data->page_addr = 0;
        data->chip.oob_poi = data->chip.buffers->databuf + data->mtd.writesize;

        printk(KERN_INFO "NAND: read system header\n");
        if (data->read_page(&data->mtd, (u_char *) data->sys_hdr) < 0) {
            printk(KERN_ERR "Warning............SPI-NAND: read system header fail!");
            return -ENODEV;
        }
        data->nand_parm.page_size = data->sys_hdr->nandfixup.nand_pagesz;
        data->nand_parm.spare_size = data->sys_hdr->nandfixup.nand_sparesz_inpage;
        data->nand_parm.block_pg = data->sys_hdr->nandfixup.nand_numpgs_blk;

        data->lookup_flag = data->sys_hdr->function[0];
        /* empty flash */
        if(data->lookup_flag == 0xFF)
            data->lookup_flag = 0;

        data->bi_table =
            dma_alloc_coherent(&pdev->dev, data->mtd.writesize, &data->bitab_dmaaddr, GFP_KERNEL);
        if (data->bi_table == NULL) {
            printk(KERN_ERR "Warning............SPI-NAND: can't alloc bi table memory");
            return -ENOMEM;
        }

		memset(data->bi_table, 0, sizeof(bi_table_t));
        if(data->lookup_flag) {
            data->lookup_table = kzalloc(8192*2, GFP_KERNEL);
        	if (!data->lookup_table) {
        		printk(KERN_ERR "SF: Failed to allocate lookup table\n");
        		return -ENOMEM;
        	} else {
        	    int i, num, loop;
        	    u8 *buf;
        	    
        	    num = data->sys_hdr->nandfixup.nand_pagesz;
        	    if(data->sys_hdr->nandfixup.nand_numblks == 512) {
        	        loop = 1;
        	        data->block_limit = 500;
        	    } else {
        	        loop = data->sys_hdr->nandfixup.nand_numblks / 1024;
        	        data->block_limit = data->sys_hdr->nandfixup.nand_numblks - (data->sys_hdr->nandfixup.nand_numblks % 1000);
        	    }

                buf = (u8 *)data->lookup_table;
        	    for(i = 0; i < loop; i++) {
        	        //printk("lookup addr = 0x%x, spi_pgbuf addr = 0x%x\n", nand_lookup + (i * num / 4), spi_pgbuf);
        	        data->page_addr = i + 1;
                    if (data->read_page(&data->mtd, buf) < 0) {
                        printk(KERN_ERR "Warning............SPI-NAND: read lookup table fail!");
                        return -ENODEV;
                    }
        
                    buf += num;
                }

                num *= data->sys_hdr->nandfixup.nand_numpgs_blk;
                for(i = 0; i < data->sys_hdr->nandfixup.nand_numblks; i++)
                    ftspi_nand_dbg("<B l:0x%4x, p:0x%4x>", i * num, data->lookup_table->physical[i] * num);
            }            
        }

        partitions = ftnandc023_partition_info;
        partitions_num = partition_check(partitions, data, data->mtd.erasesize);

        /* restore the orginal setting */
        data->page_addr = 0;
    }
    
    i = ftnandc023_available_oob(data);
	data->mtd.oobsize = data->nand_parm.spare_size;

    printk(KERN_INFO "NAND Chip: oobsize:%#x, pagesize:%#x, blocksize:%#x, chipsize:%#x\n",
           (int)data->mtd.oobsize, (int)data->mtd.writesize, (int)data->mtd.erasesize, (int)data->chip.chipsize);

    /* Scan bad block and create bbt table
     */
    nand_scan_tail(&data->mtd);

        /*----------------------------------------------------------
	 * ONFI synch mode means High Speed. If fails to change to
	 * Synch mode, then use flash as Async mode(Normal speed) and
	 * use LEGACY_LARGE fix flow.
	 */
    //if (type == ONFI)
    //    data->flash_type = ONFI;

    res = mtd_device_parse_register(&data->mtd, NULL, 0, partitions, partitions_num);
    if (!res)
        return res;

    nand_release(&data->mtd);
#if defined(CONFIG_SPI_NAND_USE_AHBDMA) || defined(CONFIG_SPI_NAND_USE_AXIDMA)
out_free_dma:
    if (data->dma_chan)
        dma_release_channel(data->dma_chan);
out_free_buf:
#endif
    kfree(data->chip.buffers);

out_unset_drv:
    platform_set_drvdata(pdev, NULL);

#if defined(CONFIG_SPI_NAND_USE_AHBDMA) || defined(CONFIG_SPI_NAND_USE_AXIDMA)
    if (data->mem_dmabuf)
        dma_free_coherent(&pdev->dev, data->mtd.writesize, data->mem_dmabuf, data->mem_dmaaddr);
#endif
    iounmap(data->io_base);
out_free_data:
    if (data->sys_hdr)
        dma_free_coherent(&pdev->dev, data->mtd.writesize, data->sys_hdr, data->syshd_dmaaddr);
    if (data->bi_table)
        dma_free_coherent(&pdev->dev, data->mtd.writesize, data->bi_table, data->bitab_dmaaddr);
    if (data->lookup_table)
        kfree(data->lookup_table);
    kfree(data);
out:
    return res;
}

/*
 * @consult with bad block table about this block is good or bad.
 *
 * @ftnandc_spi_read_bbt(struct mtd_info *mtd, loff_t offs)
 * @param mtd: MTD device structure
 * @param offs: block base address
 * @return: 1 for bad block, 0 for good block
*/
int ftnandc_spi_read_bbt(struct mtd_info *mtd, loff_t offs)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    struct bi_table *bi = (bi_table_t *) data->bi_table;
    int ret = 0, blk_id, result;

    if(!data->lookup_flag) {
        blk_id = offs >> chip->bbt_erase_shift;
        data->page_addr = blk_id << (chip->bbt_erase_shift - chip->page_shift);
    
        ftspi_nand_dbg("%s, offs = 0x%x, page = 0x%x\n", __FUNCTION__, (u32) offs, data->page_addr);
    
        if (data->read_oob(&data->mtd, data->chip.oob_poi) < 0) {
            //printk("NAND: read system header fail!");
            return 1;
        }
    
        result = bi->bi_status[blk_id / 32];
    
        if (*data->chip.oob_poi == 0xFF) {
            bi->bi_status[blk_id / 32] = result;
            result |= (1 << (blk_id % 32));
            ret = 0;                /* good */
        } else {
            result &= ~(1 << (blk_id % 32));
            ret = 1;                /* bad */
        }
        bi->bi_status[blk_id / 32] = result;
    }

    return ret;
}

/**
 * ftnandc023v2_default_block_markbad - mark a block bad
 * @mtd:	MTD device structure
 * @ofs:	offset from device start
 *
 * This is the default implementation, which can be overridden by
 * a hardware specific driver.
*/
static int ftnandc023v2_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
    int ret = 0;
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    unsigned int *bi_table = (unsigned int *)data->bi_table;
    int quotient, remainder, blk_id, result;
    int i, num, loop, value;
    u8 *buf;    

    printk(KERN_INFO "%s, addr = 0x%x\n", __FUNCTION__, (int)ofs);

    /* Get block number */
    blk_id = (int)(ofs >> chip->bbt_erase_shift);

    if(!data->lookup_flag) {
        if (data->bi_table) {
            blk_id = ofs >> chip->bbt_erase_shift;
            quotient = blk_id >> 5;
            remainder = blk_id & 0x1F;
            result = (bi_table[quotient] >> remainder) & 0x1;
            //printk("BI table %dth, bit %d, data = 0x%x, result = %d\n", quotient, remainder, bi_table[quotient], result);
            if (result == 0)        /* bad block, not need to do it */
                return 0;
    
            bi_table[quotient] &= ~(1 << remainder);        /* Write the block mark. */
            blk_id <<= 1;
            //printk("blk_id %dth, data = 0x%x, result = %x\n", blk_id, chip->bbt[blk_id >> 3], (0x3 << (blk_id & 0x06)));
            chip->bbt[blk_id >> 3] |= (0x3 << (blk_id & 0x06));
        }
        /* write 1 to 0, not need to erase first */
        data->page_addr = (int)ofs / mtd->writesize;
        *chip->oob_poi = 0x0;
        ret = data->write_oob(mtd, chip->oob_poi, mtd->oobsize);
        if (!ret)
            mtd->ecc_stats.badblocks++;

    } else {
        /* set BI field */
        data->page_addr = (int)ofs / mtd->writesize;
        *chip->oob_poi = 0x0;
        ret = data->write_oob(mtd, chip->oob_poi, mtd->oobsize);
        if (!ret)
            mtd->ecc_stats.badblocks++;

        /* use another block to replace it */
        for(i = data->block_limit; i < data->sys_hdr->nandfixup.nand_numblks; i++) {
            value = data->lookup_table->physical[i];
            if((value != 0xFFFF) && (value & 0x8000)) {
                value &= ~0x8000;
                data->lookup_table->physical[blk_id] = value;
                data->lookup_table->physical[i] = 0xFFFF;
                printk(KERN_NOTICE "Use physical block %d replace logical block %d\n", value, blk_id);
                break;
            }
        }
        if(i == data->sys_hdr->nandfixup.nand_numblks){
            printk(KERN_ERR "Can't find good block to replace it");
            return -EIO;            
        }

        data->page_addr = 0;
        data->page_addr = 0;
        data->sel_chip = 0;
        data->flash_type = 0;
        
        chip->cmdfunc(mtd, NAND_CMD_ERASE1, 0, 0);
        if (data->write_page(mtd, (const uint8_t *)data->sys_hdr) < 0) {
                printk(KERN_ERR "Warning............SPI-NAND: write system header fail!");
                return -ENODEV;
        }
   
        num = data->sys_hdr->nandfixup.nand_pagesz;
        if(data->sys_hdr->nandfixup.nand_numblks <= 1024)
            loop = 1;
        else
            loop = data->sys_hdr->nandfixup.nand_numblks / 1024;
        
        buf = (u8 *)data->lookup_table;
        for(i = 0; i < loop; i++) {
            data->page_addr = i + 1;
            if (data->write_page(mtd, buf) < 0) {
                printk(KERN_ERR "Warning............SPI-NAND: write lookup table fail!");
                return -ENODEV;
            }
        
            buf += num;
        }
    }
    return ret;
}

/*
 * Remove a NAND device.
 */
int __devexit ftnandc023_nand_remove(struct platform_device *pdev)
{
    struct ftnandc023_nand_data *data = platform_get_drvdata(pdev);

#if defined(CONFIG_SPI_NAND_USE_AHBDMA) || defined(CONFIG_SPI_NAND_USE_AXIDMA)
    if (data->mem_dmabuf)
        dma_free_coherent(&pdev->dev, data->mtd.writesize, data->mem_dmabuf, data->mem_dmaaddr);
    if (data->dma_chan)
        dma_release_channel(data->dma_chan);
#endif
    if (data->sys_hdr)
        dma_free_coherent(&pdev->dev, data->mtd.writesize, data->sys_hdr, data->syshd_dmaaddr);
    if (data->bi_table)
        dma_free_coherent(&pdev->dev, data->mtd.writesize, data->bi_table, data->bitab_dmaaddr);
    if (data->lookup_table)
        kfree(data->lookup_table);

    nand_release(&data->mtd);
    iounmap(data->io_base);
    kfree(data->chip.buffers);
    kfree(data);

    return 0;
}

static void ftnandc023_release(struct device *dev)
{
}

static u64 ftnandc023_dmamask = DMA_BIT_MASK(32);

static struct platform_device ftnandc023_device = {
    .name = "ftnandc023_nand",
    .id = -1,
    .num_resources = ARRAY_SIZE(ftnandc023_resource),
    .resource = ftnandc023_resource,
    .dev = {
            .dma_mask = &ftnandc023_dmamask,
            .coherent_dma_mask = DMA_BIT_MASK(32),
            .release = ftnandc023_release,
            },
};

static struct platform_driver ftnandc023_nand_driver = {
    .probe = ftspi020_nand_probe,
    .remove = __devexit_p(ftnandc023_nand_remove),
    .driver = {
               .name = "ftnandc023_nand",
               .owner = THIS_MODULE,
               },
};

static int __init ftspi020_nand_init(void)
{
    int ret = 0;

    /* check if the system is running NAND system 
     */
    if (platform_check_flash_type() != 1) {
        printk(KERN_INFO "Not for SPI-NAND pin mux\n");
        return 0;
    }

    /* Register PMU and turn on gate clock
     */
    nand_fd = ftpmu010_register_reg(&pmu_reg_info);
    if (nand_fd < 0) {
        printk(KERN_ERR "Warning............SPI-NAND: register PMU fail");
        return 0;
    }

#ifdef CONFIG_GPIO_WP
    if ((ret = gpio_request(GPIO_PIN, PIN_NAME)) != 0) {
        printk("gpio request fail\n");
        return ret;
    }
    printk("register GPIO for NAND write protect\n");
#endif

    if (platform_device_register(&ftnandc023_device)) {
        printk(KERN_ERR "device register failed\n");
        ret = -ENODEV;
    }
    if (platform_driver_register(&ftnandc023_nand_driver)) {
        printk(KERN_ERR "driver register failed\n");
        ret = -ENODEV;
    }
    return ret;
}

static void __exit ftspi020_nand_exit(void)
{
    /* check if the system is running NAND system 
     */
    if (platform_check_flash_type() != 0)
        return;

    /* Deregister PMU
     */
    ftpmu010_deregister_reg(nand_fd);

    platform_driver_unregister(&ftnandc023_nand_driver);
    platform_device_unregister(&ftnandc023_device);
}

module_init(ftspi020_nand_init);
module_exit(ftspi020_nand_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Te-Chen Ying");
MODULE_DESCRIPTION("FTNANDC023 V2.0 NAND driver");
MODULE_ALIAS("platform:ftnandc023_nand");
