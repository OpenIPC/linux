/*
 * FTNANDC023 NAND driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

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
#include <mach/fmem.h>
#include <linux/gpio.h>
#include "ftnandc023v2_nand.h"
#include <linux/delay.h>
/*
 * Local function or variables declaration
 */
//#define CONFIG_NAND_USE_AXIDMA
#if defined(CONFIG_NAND_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
#include <linux/dmaengine.h>

#ifdef CONFIG_NAND_USE_AHBDMA
#include <mach/ftdmac020.h>
#else
#include <mach/ftdmac030.h>
#endif

/////GM8126/////
#ifdef CONFIG_PLATFORM_GM8126
#define AHBMASTER_R_SRC   FTDMA020_AHBMASTER_0
#define AHBMASTER_R_DST   FTDMA020_AHBMASTER_0
#define AHBMASTER_W_SRC   FTDMA020_AHBMASTER_0
#define AHBMASTER_W_DST   FTDMA020_AHBMASTER_0
#define DMA_NANDC_REQ    9
#endif

/////GM8210/////
#ifdef CONFIG_PLATFORM_GM8210
#ifdef CONFIG_NAND_USE_AXIDMA
#define DMA_NANDC_REQ    2 
#else
#define AHBMASTER_R_SRC   FTDMA020_AHBMASTER_0
#define AHBMASTER_R_DST   FTDMA020_AHBMASTER_0
#define AHBMASTER_W_SRC   FTDMA020_AHBMASTER_0
#define AHBMASTER_W_DST   FTDMA020_AHBMASTER_0
#define DMA_NANDC_REQ    2
#endif

#endif

/////GM8287/////
#ifdef CONFIG_PLATFORM_GM8287
#define AHBMASTER_R_SRC   FTDMA020_AHBMASTER_0
#define AHBMASTER_R_DST   FTDMA020_AHBMASTER_1
#define AHBMASTER_W_SRC   FTDMA020_AHBMASTER_1
#define AHBMASTER_W_DST   FTDMA020_AHBMASTER_0
#define DMA_NANDC_REQ    2
#endif

static void ftnandc023_dma_callback(void *param);
static volatile unsigned int trigger_flag = 0;
static wait_queue_head_t nand023_queue;
#endif 

#define PORTING 
static int ftnandc023v2_block_markbad(struct mtd_info *mtd, loff_t ofs);
int ftnandc_read_bbt(struct mtd_info *mtd, loff_t offs);
static unsigned int ftnandc023v2_get_blockbase(struct mtd_info *mtd, unsigned int base_addr);
static int nand_fd;
static int avail_oob_sz = 0;

/*
 * Macro definitions
 */
#define DEFAULT_BLOCK_SIZE  0x20000 
#define MTD_CFG_SZ          (6 * DEFAULT_BLOCK_SIZE)	//2 block + reserved

#define AHB_Memory_8KByte   (1 << 4)
#define CONFIG_FTNANDC023_START_CHANNEL     0
#define BLOCK_ALIGN(base, blk_shift) ((((base) + (0x1 << (blk_shift)) - 1) >> (blk_shift)) << (blk_shift))

#ifdef CONFIG_GPIO_WP
//#define GPIO_PIN	28
//#define PIN_NAME	"gpio28"
#define GPIO_PIN        ((32*2)+30) 
#define PIN_NAME        "gpio2_30" 
#endif

extern int root_mtd_num;
//=============================================================================
// System Header, size = 512 bytes
//=============================================================================

#ifdef CONFIG_PLATFORM_GM8126 

static struct mtd_partition PORTING ftnandc023_partition_info[] = {
    {.name = "Linux Section"},
    {.name = "User Section",.size = MTDPART_SIZ_FULL},
    {.name = "Loader Section"},
    {.name = "Burin Section"},
    {.name = "UBoot Section"},
    {.name = "CFG Section"},
};
enum { MTD_PART_LINUX, MTD_PART_USER, MTD_PART_LOADER, MTD_PART_BURNIN, MTD_PART_UBOOT,
        MTD_PART_CFG };

static int  ref_count = 0;

int platform_select_pinmux(int fd, int chip){
    int ret = 0;
    volatile int i;
    
    if (chip >= 0) {
        if (ref_count == 0) {
    	    if (ftpmu010_request_pins(fd, 0x6C, (0x1 << 22), 1) < 0) {
    		    printk("Warning............NAND request pin failed\n");
    		    return ret;
    	    }
            
            /* the delay is used to prevent data in the bus from incomplete */
            for (i = 0; i < 1000; i++) {}            
            
            ftpmu010_write_reg(fd, 0x6C, (0x0 << 22), (0x1 << 22));
        }
        ref_count ++;
    }
    else {
        ref_count -- ;
        if (ref_count == 0) {
            /* chip unselect, the delay is used to prevent data in the bus from incomplete */
            for (i = 0; i < 1000; i++) {}
            //ftpmu010_del_lockbits(fd, 0x6C, (0x1 << 22));

            ftpmu010_write_reg(fd, 0x6C, (0x1 << 22), (0x1 << 22)); //to SD
	    ftpmu010_release_pins(fd, 0x6C, (0x1 << 22));
        }
    }
    
    return ret;
}

static pmuReg_t  pmu_reg[] = {
    /* off, bitmask,   lockbit,    init val,   init mask */
    {0x38, (0x1 << 5), (0x1 << 5), (0x0 << 5), (0x1 << 5)},   /* clock gate */
    {0x50, (0x3 << 14),(0x3 << 14),(0x3 << 14),(0x3 << 14)},  /* NAND_BUSY pin with UART2 */
    {0x5C, 0x0FFFFFFF, 0x0FFFFF00, 0x05555555, 0x0FFFFFFF},   /* pinMux with GPIO/SD */
    {0x6C, (0x3 << 21),(0x0 << 21),(0x0 << 21),(0x1 << 21)},  /* pinMux with SD */
    //{0x6C, (0x7 << 20),(0x0 << 21),(0x3 << 20),(0x3 << 20)},  /* pinMux with SD */
};

static pmuRegInfo_t	pmu_reg_info = {
    "NANDC023",
    ARRAY_SIZE(pmu_reg),
    ATTR_TYPE_NONE,
    pmu_reg
};

#define MTD_LINUX_SZ        (20 << 20)  //20M bytes
#define IMAGE_MAGIC         0x805A474D

typedef struct sys_header {	
    char signature[8];          /* Signature is "GM8126" */
    unsigned int bootm_addr;    /* default Image offset to load by nandboot */
    unsigned int burnin_addr;   /* burn-in image address */
    unsigned int uboot_addr;    /* uboot address */
    unsigned int linux_addr;    /* linux image address */
    unsigned int reserved1[7];  /* unused */
    struct {
        unsigned int nand_numblks;      //number of blocks in chip
        unsigned int nand_numpgs_blk;   //how many pages in a block
        unsigned int nand_pagesz;       //real size in bytes                        
        unsigned int nand_sparesz_inpage;       //64bytes for 2k, ...... needed for NANDC023
        unsigned int nand_numce;        //how many CE in chip
        unsigned int nand_status_bit;
        unsigned int nand_cmmand_bit;
        unsigned int nand_ecc_capability;
        unsigned int nand_ecc_base;     //0/1 indicates 512/1024 bytes      
        unsigned int reserved_1;
        unsigned int reserved_2;
        unsigned int nand_row_cycle;
        unsigned int nand_col_cycle;
        unsigned int reserved[1];
    } nandfixup;
    unsigned int reserved2[100];        // unused
    unsigned char last_511[4];  // byte510:0x55, byte511:0xAA
} sys_header_t;

/* Image header , 512 bytes */
typedef struct nand_img_header {
    unsigned int magic;         /* Image header magic number (0x805A474D) */
    unsigned int chksum;        /* Image CRC checksum */
    unsigned int size;          /* Image size */
    unsigned int unused;
    unsigned char name[80];     /* Image name */
    unsigned char reserved[160 + 256];  /* Reserved for future */
} img_hdr_t;

#else

static struct mtd_partition PORTING ftnandc023_partition_info[10];

int platform_select_pinmux(int fd, int chip){
    return 0;
}

#ifdef CONFIG_PLATFORM_GM8210
static pmuReg_t  pmu_reg[] = {
    /* off, bitmask,   lockbit,    init val,   init mask */
    {0x28, (0x1 << 18), (0x1 << 18), (0x0 << 18), (0x1 << 18)},   /* frequency setting */
    {0x5C, 0x08000000, 0x08000000, 0x08000000, 0x08000000},   /* pinMux with GPIO, release 1~3 for other function */
    {0x68, 0x3C000000, 0x3C000000, 0, 0x3C000000},   /* Schmitt trigger */
#ifdef CONFIG_NAND_USE_AXIDMA
    {0xA4, (0x1 << 27), (0x1 << 27), (0x1 << 27), (0x1 << 27)},   /* DMA ack selection */
#endif    
    {0xB4, (0x1 << 11), (0x1 << 11), (0x0 << 11), (0x1 << 11)},   /* AHB clock gate */
};
#endif

#ifdef CONFIG_PLATFORM_GM8287
static pmuReg_t  pmu_reg[] = {
    /* off, bitmask,   lockbit,    init val,   init mask */
    {0x28, (0x1 << 18), (0x1 << 18), (0x0 << 18), (0x1 << 18)},   /* frequency setting */
    {0x5C, (0x1F << 27), (0x1F << 27), (0xB << 27), (0x1F << 27)},   /* pinMux */
    {0xB4, (0x1 << 18), (0x1 << 18), (0x0 << 18), (0x1 << 18)},   /* AHB clock gate */
};
#endif

static pmuRegInfo_t	pmu_reg_info = {
    "NANDC023",
    ARRAY_SIZE(pmu_reg),
    ATTR_TYPE_NONE,
    pmu_reg
};

#define MTD_LOGO_SZ         (4 << 20)  //4M bytes
#define MTD_LINUX_SZ        (30 << 20)  //30M bytes

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
        
    struct {
        unsigned int nand_numblks;      //number of blocks in chip
        unsigned int nand_numpgs_blk;   //how many pages in a block
        unsigned int nand_pagesz;       //real size in bytes                        
        unsigned int nand_sparesz_inpage;       //64bytes for 2k, ...... needed for NANDC023
        unsigned int nand_numce;        //how many CE in chip
        unsigned int nand_status_bit;
        unsigned int nand_cmmand_bit;
        unsigned int nand_ecc_capability;
        unsigned int nand_ecc_base;     //0/1 indicates 512/1024 bytes      
        unsigned int nand_sparesz_insect;
        unsigned int reserved_1;
        unsigned int nand_row_cycle;    //1 for 1 cycle ...
        unsigned int nand_col_cycle;    //1 for 1 cycle ...
        unsigned int reserved[1];
    } nandfixup;
    unsigned int reserved2[58];        // unused
    unsigned char last_511[4];  // byte510:0x55, byte511:0xAA
} sys_header_t;
#endif
//=============================================================================
// BI table, size = 1024 bytes
//=============================================================================
typedef struct bi_table {
    /* This array size is related to USB_BUFF_SZ defined in usb_scsi.h */
    unsigned int bi_status[256];        //each bit indicates a block. 1 for good, 0 for bad
} bi_table_t;

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
#if defined(CONFIG_NAND_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)   
    struct dma_chan             *dma_chan;
    dma_cap_mask_t              cap_mask;
#ifdef CONFIG_PLATFORM_GM8126    
    struct ftdmac020_dma_slave  dma_slave_config;
#else
#ifdef CONFIG_NAND_USE_AXIDMA
    struct ftdmac030_dma_slave  dma_slave_config;
#else 
    struct ftdmac020_dma_slave  dma_slave_config;   
#endif    
#endif    
    dma_cookie_t                cookie;
    dma_addr_t mem_dmaaddr;
    dma_addr_t nand_dmaaddr;
    unsigned char *mem_dmabuf;
    unsigned char *sg_dmabuf;
#endif    
    dma_addr_t syshd_dmaaddr;
    dma_addr_t bitab_dmaaddr;
    int cur_chan;
    int valid_chip[MAX_CHANNEL];
    int scan_state;
    int flash_type;
    int large_page;
    sys_header_t *sys_hdr;      /* system header */
    bi_table_t *bi_table;       /* bad block table next to sys_hdr */
    int (*write_oob) (struct mtd_info * mtd, const u_char * buf, int len);
    int (*read_oob) (struct mtd_info * mtd, u_char * buf);
    int (*write_page) (struct mtd_info * mtd, const uint8_t * buf);
    int (*read_page) (struct mtd_info * mtd, u_char * buf);
};

#ifdef CONFIG_MTD_NAND_FTNANDC023
static int startchn = CONFIG_FTNANDC023_START_CHANNEL;
#else
static int startchn = 0;
module_param(startchn, int, 0644);
#endif

static int eccbasft;
static int spare;
static int usecrc;
static int useecc;
static int legacy;

#define	max_chip_size	(2048 * 16)     //2048MB NAND
static unsigned int page_map[max_chip_size] = { };

#ifdef CONFIG_MTD_CMDLINE_PARTS
static const char *part_probes[] = { "cmdlinepart", NULL };
#endif

#ifndef CONFIG_PLATFORM_GM8126
static int partition_check(struct mtd_partition *partitions, struct ftnandc023_nand_data *data, int block_size)
{
    int i, num = 0;
    sys_header_t *sys_hdr;
    
    sys_hdr = data->sys_hdr;
#if defined(CONFIG_GM8210_FPGA) || defined(CONFIG_FPGA)
    partitions[0].name = 'a';
    partitions[0].offset = 0x140000;
    partitions[0].size = 0x200000;
    
    partitions[1].name = 'b';
    partitions[1].offset = 0x500000;
    partitions[1].size = 0x200000;
    
    num = 2;    
#else
    for(i = 0; i < ARRAY_SIZE(ftnandc023_partition_info); i++){    
    	if(sys_hdr->image[i].size == 0)
    	    continue;

        partitions[num].offset = sys_hdr->image[i].addr;
        partitions[num].size = sys_hdr->image[i].size;
        partitions[num].name = sys_hdr->image[i].name;
        
        if(sys_hdr->image[i].addr % block_size){
    	    printk("Warning... partition %d addr 0x%x not block alignment, one block = 0x%x\n", i, sys_hdr->image[i].addr, block_size);
    	    partitions[i].offset = BLOCK_ALIGN(sys_hdr->image[i].addr, block_size);
        }
        if(sys_hdr->image[i].size % block_size){
    	    printk("Warning... partition %d size 0x%x not block alignment, one block = 0x%x\n", i, sys_hdr->image[i].size, block_size);
    	    partitions[i].size = BLOCK_ALIGN(sys_hdr->image[i].size, block_size);
        } 
        num++;   
    }

    for(i = 0; i < ARRAY_SIZE(ftnandc023_partition_info); i++){
        DBGLEVEL2(ftnandc023_dbg("partation %d addr = 0x%x, size = 0x%x, name = %s\n", i, sys_hdr->image[i].addr, sys_hdr->image[i].size, sys_hdr->image[i].name));
    }
    
    for(i = 0; i < (num - 1); i++)
    	if(partitions[i + 1].offset >= partitions[i].offset)
	        if(partitions[i + 1].offset - partitions[i].offset < (2 * block_size)){
	            printk("Warning... Block reserve for bad issue between partition %d with %d is not enough\n", i, i + 1);
	            printk("partition %d addr = 0x%x, %d addr = 0x%x\n", i, (u32)partitions[i].offset, i + 1, (u32)partitions[i + 1].offset);
	        }    	
#endif    
    return num;
}
#endif

/* tWH, tWP, tREH, tREA, tRP, tWB, tRB, tWHR, tWHR2, tRHW, tRR, tAR,
 * tADL, tRHZ, tCCS, tCS, tCLS, tCLR, tALS, tCALS2, tCWAW, tWPRE,
 * tRPRE, tWPST, tRPST, tWPSTH, tRPSTH, tDQSHZ, tCAD, tDSL
 * tDSH, tDQSL, tDQSH, tCKWR, tWRCK
 */
static struct ftnandc023_chip_timing PORTING chip_timing =
    { 10, 12, 10, 20, 12, 100, 0, 60, 0, 100, 20, 10,
    100, 100, 0, 20, 12, 10, 12, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0
};

static struct resource ftnandc023_resource[] = {
    [0] = {
           .start = NAND_FTNAND023_PA_BASE,     /* Register Base address */
           .end = NAND_FTNAND023_PA_LIMIT,
           .flags = IORESOURCE_MEM,
           },
    [1] = {
           .start = NANDDP_FTNAND023_PA_BASE,   /* BMC buffer or Data Port access */
           .end = NANDDP_FTNAND023_PA_LIMIT,
           .flags = IORESOURCE_MEM,
           },
    [2] = {
           .start = NAND_FTNAND023_IRQ,
           .end = NAND_FTNAND023_IRQ,
           .flags = IORESOURCE_IRQ,
           }
};

static struct nand_ecclayout nand_hw_eccoob = {
    .eccbytes = 0,
    .eccpos = {0},
    .oobfree = {
                {.offset = 0,
                 .length = 64}}
};

static uint8_t ftnandc023_bbt_pattern[] = { 'B', 'b', 't', '0' };
static uint8_t ftnandc023_mirror_pattern[] = { '1', 't', 'b', 'B' };

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

static uint8_t ftnandc023_scan_ff_pattern[] = { 0xff, 0xff, 0xff, 0xff };

static struct nand_bbt_descr ftnandc023_largepage_flashbased = {
    .options = NAND_BBT_SCAN2NDPAGE,
    .offs = 0,
    .len = 4,
    .pattern = ftnandc023_scan_ff_pattern
};

static void ftnandc023_regdump(struct mtd_info *mtd)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    u32 val;

    printk(KERN_INFO "===================================\n");
    val = readl(data->io_base + ECC_INTR_STATUS);
    printk(KERN_INFO "ECC intr sts(0x%2x): 0x%08x\n", ECC_INTR_STATUS, val);
    val = readl(data->io_base + DEV_BUSY);
    printk(KERN_INFO "Device busy(0x%2x): 0x%08x\n", DEV_BUSY, val);
    val = readl(data->io_base + GENERAL_SETTING);
    printk(KERN_INFO "General setting(0x%2x): 0x%08x\n", GENERAL_SETTING, val);
    val = readl(data->io_base + MEM_ATTR_SET);
    printk(KERN_INFO "Mem attr.(0x%2x): 0x%08x\n", MEM_ATTR_SET, val);
    val = readl(data->io_base + INTR_STATUS);
    printk(KERN_INFO "Intr sts(0x%2x): 0x%08x\n", INTR_STATUS, val);
    val = readl(data->io_base + BMC_REGION_STATUS);
    printk(KERN_INFO "BMC region sts(0x%2x): 0x%08x\n", BMC_REGION_STATUS, val);
    printk(KERN_INFO "===================================\n");
}

int show_one_time = 1;
static inline void ftnandc023_set_row_col_addr(struct ftnandc023_nand_data *data)
{
    int val;

    val = readl(data->io_base + MEM_ATTR_SET);
    /* if NAND be write before, ROM CODE will set correct row/col cycle, not need to set again */
#if defined(CONFIG_GM8210_FPGA) || defined(CONFIG_FPGA)    
    val &= ~(0x7 << 12);
    if (data->large_page)
    	val |= (ATTR_ROW_CYCLE(1) | ATTR_COL_CYCLE(1));//row 1G set 2, 2G set 3
	else
		val |= (ATTR_ROW_CYCLE(1) | ATTR_COL_CYCLE(0));
#endif
	if(show_one_time) {
		printk("NAND Row cycle = %d, Col cycle = %d\n", ((val >> 13) & 0x3) + 1, ((val >> 12) & 0x1) + 1);
		show_one_time = 0;
	}
    writel(val, data->io_base + MEM_ATTR_SET);
}

/* low enable write protect, high disable write protect */
void write_protect(int mode)
{
#ifdef CONFIG_GPIO_WP
		if(mode)
			gpio_direction_output(GPIO_PIN, 0);
		else
			gpio_direction_output(GPIO_PIN, 1);
#endif
}

static void ftnandc023_soft_reset(struct mtd_info *mtd)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    unsigned long timeo = jiffies;
    u32 val = 0;

    timeo += HZ;

    platform_select_pinmux(nand_fd, 0);

    writel(1, data->io_base + GLOBAL_RESET);
    while (time_before(jiffies, timeo)) {
    	val = readl(data->io_base + GLOBAL_RESET);
    	if(!val)
    		break;
    }
    if(val)
    	printk("Reset channel timeout!\n");

    platform_select_pinmux(nand_fd, -1);
}

static int ftnandc023_nand_check_cmdq(struct mtd_info *mtd)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    unsigned long timeo = jiffies;
    u32 status;
    int ret;

    platform_select_pinmux(nand_fd, 0);

    ret = -EIO;
    timeo += HZ;
    while (time_before(jiffies, timeo)) {
        status = readl(data->io_base + CMDQUEUE_STATUS);
        if ((status & CMDQUEUE_STATUS_FULL(data->cur_chan)) == 0) {
            ret = 0;
            break;
        }
        cond_resched();
    }
    
		platform_select_pinmux(nand_fd, -1);
    return ret;
}

#if defined(CONFIG_NAND_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
static int ftnandc023_dma_wait(struct mtd_info *mtd)
{
    int ret = -1;
  
    ret = wait_event_timeout(nand023_queue, trigger_flag == 1, 10 * HZ);
    if (ret == 0) {
    	printk("nand023 dma queue wake up timeout signal arrived\n");
    	return -1;
    }

    trigger_flag = 0; 
    
    return 0;
}
#endif

void ecc_handle(struct mtd_info *mtd)
{
		struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    u32 val;
    unsigned long timeo = jiffies;
    
    //step 2
    val = readl(data->io_base + BMC_REGION_STATUS);
    if((val & (1 << 8)) != 0)
    	return;
    //step 4
    writel(data->sel_chip + 1, data->io_base + REGION_SW_RESET);
    //step 5
    writel(1, data->io_base + AHB_RESET);
    timeo += HZ;
    while (time_before(jiffies, timeo)) {
    	val = readl(data->io_base + AHB_RESET);
    	if(!val)
    		break;
    }
    if(val)
    	printk("Reset AHB slave timeout!\n");
   
    //step 7
    timeo += HZ;
    while (time_before(jiffies, timeo)) {
    	val = readl(data->io_base + DEV_BUSY);
    	if(val)
    		break;
    }
    if(!val)
    	printk("Wait channel ready timeout!\n");
    
    //step 9	
    //ftnandc023_soft_reset(mtd);    
}

static int ftnandc023_nand_wait(struct mtd_info *mtd, struct nand_chip *chip)
{
    struct ftnandc023_nand_data *data = chip->priv;

    unsigned long timeo = jiffies;
    int ret, state = chip->state;
    u32 status = 0;
    int i, check_flag = 0, prog_flag = 0;

    platform_select_pinmux(nand_fd, 0);

    ret = NAND_STATUS_FAIL;
    timeo += HZ;

    while (time_before(jiffies, timeo)) {
        status = readl(data->io_base + INTR_STATUS);
        /* Clear interrupt instantly, since we already keep IS to status */
        writel(status, data->io_base + INTR_STATUS);

            if (unlikely(status & STATUS_FAIL(data->cur_chan))) {
                printk(KERN_ERR "NAND: STATUS FAIL(0x%x, off 0x178 = 0x%x)!\n", status,
                       readl(data->io_base + READ_STATUS0));
                ftnandc023_soft_reset(mtd);
                goto out;
            }

        if (status & STATUS_CMD_COMPLETE(data->cur_chan)) {
            //clear status error
            status = readl(data->io_base + ECC_INTR_STATUS);
            if (unlikely(status & ECC_ERR_FAIL(data->cur_chan)))
            		writel(ECC_ERR_FAIL(data->cur_chan), data->io_base + ECC_INTR_STATUS);
		            		
            if (state == FL_READING) {
                if (unlikely(status & STATUS_CRC_FAIL(data->cur_chan))) {
                    if(data->page_addr != 1) {
                    	if(data->buf != NULL) {/* only read OOB for filesystem */
		                    for (i = 0; i < (mtd->writesize >> 2); i++) {
		                    	if (*(data->buf + i) != 0xFFFFFFFF) {
		                    		ret = NAND_STATUS_FAIL;
		                    		check_flag = prog_flag = 1;
		                    		printk(KERN_ERR "CRC err\n");
			                      mtd->ecc_stats.failed++;
			                      goto out;                    		
		                    	}
                            }
                        }
                    }
                }

                if (unlikely(status & ECC_ERR_FAIL(data->cur_chan))) {
                    if(data->page_addr != 1){
	                    if (check_flag) {
	                    		if(prog_flag){
		                        ret = NAND_STATUS_FAIL;
		                        printk(KERN_ERR "ECC err\n");
		                        ecc_handle(mtd);
		                        //printk("addr = 0x%x, data = 0x%x, base = 0x%x\n", (u32)(data->buf + i), *(data->buf + i),(u32)data->buf);
		                        mtd->ecc_stats.failed++;
		                        goto out;
	                      	}
	                    } else {
	                    		if(data->buf != NULL) {/* only read OOB for filesystem */
				                    for (i = 0; i < (mtd->writesize >> 2); i++) {
				                    	if (*(data->buf + i) != 0xFFFFFFFF){
    				                        ret = NAND_STATUS_FAIL;
    				                        printk(KERN_ERR "ECC err\n");
    				                        ecc_handle(mtd);
    				                        //printk("addr = 0x%x, data = 0x%x, base = 0x%x\n", (u32)(data->buf + i), *(data->buf + i),(u32)data->buf);
    				                        mtd->ecc_stats.failed++;
    				                        goto out;
    				                    }
                                    }
                                }
	                    }
                  	}
                }
            }
            ret = NAND_STATUS_READY;
            goto out;

        }

        cond_resched();
    }

    printk("nand wait time out, status = 0x%x\n", status);
    ftnandc023_regdump(mtd);
  out:

  	write_protect(1);
  	platform_select_pinmux(nand_fd, -1);
    return ret;
}

/* The unit of Hclk is MHz, and the unit of Time is ns.
 * We desire to calculate N to satisfy N*(1/Hclk) > Time given Hclk and Time
 * ==> N > Time * Hclk
 * ==> N > Time * 10e(-9) * Hclk *10e(6)        --> take the order out
 * ==> N > Time * Hclk * 10e(-3)
 * ==> N > Time * Hclk / 1000
 * ==> N = (Time * Hclk + 999) / 1000
 */
static void ftnandc023_calc_timing(struct mtd_info *mtd)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    uint16_t tWH, tWP, tREH, tRES, tBSY, tBUF1;
    uint16_t tBUF2, tBUF3, tBUF4, t1, tPRE, tRLAT;
    uint16_t tPST, tPSTH, tCAD, tCS, tCKWR;
    int i;
    struct ftnandc023_chip_timing *p;
    u32 HCLK, timing[4];

    /*
     * The default value is slow and it should be enough for use.
     */
    return;

    //HCLK = a320_get_ahb_clk() * 2 / 1000000;
    HCLK = ftpmu010_get_attr(ATTR_TYPE_AHB) / 1000000;
#if defined (CONFIG_FTNANDC023_MICRON_29F32G08CBABB)
    if (data->flash_type == ONFI)
        p = &sync_timing;
    else
#endif
        p = &chip_timing;

    tWH = (p->tWH * HCLK + 999) / 1000;
    if (tWH >= 1)
        tWH--;

    tWP = (p->tWP * HCLK + 999) / 1000;
    if (tWP >= 1)
        tWP--;

    tREH = (p->tREH * HCLK + 999) / 1000;
    if (tREH >= 1)
        tREH--;

    tRES = max(p->tREA, p->tRP);
    tRES = (tRES * HCLK + 999) / 1000;
    if (tRES >= 1)
        tRES--;

    tRLAT = tREH + tRES + 1;

    tBSY = max(p->tWB, p->tRB);
    tBSY = (tBSY * HCLK + 999) / 1000;
    if (tBSY >= 2)
        tBSY--;

    tBUF1 = max(p->tADL, p->tCCS);
    tBUF1 = (tBUF1 * HCLK + 999) / 1000;
    if (tBUF1 >= 2)
        tBUF1--;

    tBUF2 = max(p->tAR, p->tRR);
    tBUF2 = max(tBUF2, p->tCLR);
    tBUF2 = (tBUF2 * HCLK + 999) / 1000;
    if (tBUF2 >= 2)
        tBUF2--;

    tBUF3 = max(p->tRHW, p->tRHZ);
    tBUF3 = max(tBUF3, p->tDQSHZ);
    tBUF3 = (tBUF3 * HCLK + 999) / 1000;
    if (tBUF3 >= 2)
        tBUF3--;

    tBUF4 = p->tWHR;
    tBUF4 = (tBUF4 * HCLK + 999) / 1000;
    if (tBUF4 >= 2)
        tBUF4--;

    if (data->flash_type == TOGGLE) {
        t1 = max(p->tCLS, p->tALS);
        t1 = max(t1, p->tCS);
        t1 -= p->tWP;
    } else {
        t1 = max((uint16_t) (p->tCALS2 - p->tWP), p->tCWAW);
    }
    t1 = (t1 * HCLK + 999) / 1000;
    if (t1 >= 1)
        t1--;

    tPRE = max(p->tWPRE, p->tRPRE);
    tPRE = (tPRE * HCLK + 999) / 1000;
    if (tPRE >= 1)
        tPRE--;

    tPST = max(p->tWPST, p->tRPST);
    tPST = (tPST * HCLK + 999) / 1000;
    if (tPST >= 1)
        tPST--;

    if (data->flash_type == TOGGLE)
        tPSTH = max(p->tWPSTH, p->tRPSTH);
    else
        tPSTH = p->tDQSHZ;
    tPSTH = (tPSTH * HCLK + 999) / 1000;
    if (tPSTH >= 1)
        tPSTH--;

    tCAD = p->tCAD;
    tCAD = (tCAD * HCLK + 999) / 1000;
    if (tCAD >= 1)
        tCAD--;

    tCS = p->tCS;
    tCS = (tCS * HCLK + 999) / 1000;
    if (tCS >= 1)
        tCS--;

    if (data->flash_type == TOGGLE) {
        tCKWR = max(p->tDSL, p->tDSH);
        tCKWR = max(tCKWR, p->tDQSL);
        tCKWR = max(tCKWR, p->tDQSH);
    } else {
        tCKWR = max(p->tCKWR, p->tWRCK);
    }
    tCKWR = (tCKWR * HCLK + 999) / 1000;
    if (tCKWR >= 1)
        tCKWR--;

    timing[0] = (tWH << 24) | (tWP << 16) | (tREH << 8) | tRES;
    timing[1] = (tRLAT << 16) | (tBSY << 8) | t1;
    timing[2] = (tBUF4 << 24) | (tBUF3 << 16) | (tBUF2 << 8) | tBUF1;
    timing[3] = (tPRE << 28) | (tPST << 24) | (tPSTH << 16) | (tCAD << 8) | (tCS << 5) | tCKWR;

    printk("AC Timing 0:0x%08x\n", timing[0]);
    printk("AC Timing 1:0x%08x\n", timing[1]);
    printk("AC Timing 2:0x%08x\n", timing[2]);
    printk("AC Timing 3:0x%08x\n", timing[3]);

    for (i = 0; i < MAX_CHANNEL; i++) {
        writel(timing[0], data->io_base + FL_AC_TIMING0(i));
        writel(timing[1], data->io_base + FL_AC_TIMING1(i));
        writel(timing[2], data->io_base + FL_AC_TIMING2(i));
        writel(timing[3], data->io_base + FL_AC_TIMING3(i));
    }
}

static uint32_t ftnandc023_onfi_get_feature(struct mtd_info *mtd, int chn, int ce, int type)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    u32 cq6, val;

    platform_select_pinmux(nand_fd, 0);

    ftnandc023_set_row_col_addr(data);

    /* 0x1 is Timing mode feature address */
    writel(0x1, data->io_base + CMDQUEUE1(chn));

    cq6 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(type);
    cq6 |= CMD_START_CE(ce) | CMD_BYTE_MODE | CMD_SPARE_NUM(3);
    cq6 |= CMD_INDEX(ONFI_FIXFLOW_GETFEATURE);
    ftnandc023_nand_check_cmdq(mtd);
    writel(cq6, data->io_base + CMDQUEUE6(chn));

    ftnandc023_nand_wait(mtd, chip);
    val = readl(data->io_base + SPARE_SRAM + (chn << 10)
                + (ce << 6));

		platform_select_pinmux(nand_fd, -1);

    return val;
}

static void ftnandc023_onfi_set_feature(struct mtd_info *mtd, int chn, int ce)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    u32 cq6;

    platform_select_pinmux(nand_fd, 0);
    write_protect(0);

    ftnandc023_set_row_col_addr(data);

    /* 0x11 means Timing mode 1 and Synchronous DDR */
    writel(0x11, data->io_base + SPARE_SRAM + (chn << 10) + (ce << 6));

    /* 0x1 is Timing mode feature address */
    writel(0x1, data->io_base + CMDQUEUE1(chn));

    cq6 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(0);
    cq6 |= CMD_START_CE(ce) | CMD_BYTE_MODE | CMD_SPARE_NUM(3);
    cq6 |= CMD_INDEX(ONFI_FIXFLOW_SETFEATURE);
    ftnandc023_nand_check_cmdq(mtd);
    writel(cq6, data->io_base + CMDQUEUE6(chn));
    ftnandc023_nand_wait(mtd, chip);

    platform_select_pinmux(nand_fd, -1);
}

static int ftnandc023_onfi_sync(struct mtd_info *mtd)
{
    struct nand_chip *p = mtd->priv;
    struct ftnandc023_nand_data *data = p->priv;
    u32 val;
    int i, j, ret = -1;

    platform_select_pinmux(nand_fd, 0);

    for (i = startchn; i < MAX_CHANNEL; i++) {
        for (j = 0; j < data->valid_chip[i]; j++) {
            val = ftnandc023_onfi_get_feature(mtd, i, j, 0);
            printk("onfi feature for channel %d, CE %d: 0x%x\n", i, j, val);
        }
    }
    for (i = startchn; i < MAX_CHANNEL; i++) {
        for (j = 0; j < data->valid_chip[i]; j++) {
            ftnandc023_onfi_set_feature(mtd, i, j);
        }
    }
    for (i = startchn; i < MAX_CHANNEL; i++) {
        for (j = 0; j < data->valid_chip[i]; j++) {
            val = ftnandc023_onfi_get_feature(mtd, i, j, 3);
            printk("onfi feature for channel %d, CE %d: 0x%x\n", i, j, val);
            if (val != 0x1111) {
                goto out;
            }
        }
    }
    ret = 0;

  out:
  	platform_select_pinmux(nand_fd, -1);
    return ret;
}

static int ftnandc023_available_oob(struct mtd_info *mtd)
{
    int ret = 0;
    int tmp, eccbyte;

    if (useecc < 0)
        goto out;
    if (usecrc != 0)
        usecrc = 1;
    else
        usecrc = 0;

    eccbyte = (useecc * 14) / 8;
    if (((useecc * 14) % 8) != 0)
        eccbyte++;
    tmp = (eccbyte * (mtd->writesize >> eccbasft)) + (usecrc << 1) * (mtd->writesize >> eccbasft);

        /*----------------------------------------------------------
	 * YAFFS require 16 bytes OOB without ECC, 28 bytes with
	 * ECC enable.
	 * BBT require 5 bytes for Bad Block Table marker.
	 */
#ifdef CONFIG_YAFFS_FS
    if (spare - tmp >= 16) {
        ret = spare - tmp;
        printk(KERN_INFO "NAND(YAFFS): avaliable OOB is %d byte.\n", ret);
    } else {
        printk(KERN_INFO
               "NAND: Not enough OOB:%d bytes(YAFFS requires 16 bytes without software ECC, "
               "28 bytes with ECC enable), try to reduce ECC correction bits.\n", spare - tmp);
    }
#else
    ret = spare - tmp;
    printk(KERN_INFO "Avaliable OOB is %d byte(%d per sector).\n", ret,
           ret / (mtd->writesize >> eccbasft));
#endif
  out:
    return ret;
}

static uint8_t ftnandc023_read_byte(struct mtd_info *mtd)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    uint32_t lv;
    uint8_t b = 0;

    platform_select_pinmux(nand_fd, 0);

    switch (data->cur_cmd) {
    case NAND_CMD_READID:
        lv = readl(data->io_base + SPARE_SRAM + (data->cur_chan << 10)
                   + (data->sel_chip << 6));
        b = (lv >> data->byte_ofs) & 0xFF;
        data->byte_ofs += 8;
        if (data->byte_ofs == 32)
            data->byte_ofs = 0;
        break;
    case NAND_CMD_STATUS:
        lv = readl(data->io_base + READ_STATUS0);
        lv = lv >> (data->cur_chan * 8);
        b = (lv & 0xFF);
        /* FIXME: status seems has problem, workaround here */
        b |= NAND_STATUS_WP;
        break;
    }
		platform_select_pinmux(nand_fd, -1);
    return b;
}

static int ftnandc023_nand_read_oob_lp(struct mtd_info *mtd, u_char * buf)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    u32 cq6;
    int status = 0, i, j;
    u8 *p;

    p = buf;

    platform_select_pinmux(nand_fd, 0);
    ftnandc023_set_row_col_addr(data);

    writel(data->page_addr, data->io_base + CMDQUEUE1(data->cur_chan));
    writel(CMD_COUNT(mtd->writesize >> eccbasft), data->io_base + CMDQUEUE5(data->cur_chan));

    cq6 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type);
    cq6 |= CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(spare) | CMD_INDEX(LARGE_FIXFLOW_READOOB);
    status = ftnandc023_nand_check_cmdq(mtd);
    if (status < 0)
        goto out;
    writel(cq6, data->io_base + CMDQUEUE6(data->cur_chan));

    if (ftnandc023_nand_wait(mtd, chip) == NAND_STATUS_FAIL) {
        status = -1;
        goto out;
    }

    for (i = 0; i < (mtd->writesize >> eccbasft); i++) {
        for (j = 0; j < (spare + 1); j++) {
            *(p++) =
                readb(data->io_base + SPARE_SRAM + (data->cur_chan << 10) +
                      (data->sel_chip << 6) + i * (64 / (mtd->writesize >> eccbasft)) + j);
        }
    }

    for (i = avail_oob_sz; i < mtd->oobsize; i++)
        *(p++) = 0xFF;          /* clear */
  out:
  	platform_select_pinmux(nand_fd, -1);
    return status;
}

static int ftnandc023_nand_read_oob_sp(struct mtd_info *mtd, u_char * buf)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    u32 cq6;
    int i, status = 0;

    platform_select_pinmux(nand_fd, 0);
    ftnandc023_set_row_col_addr(data);

    writel(data->page_addr, data->io_base + CMDQUEUE1(data->cur_chan));
    writel(CMD_COUNT(1), data->io_base + CMDQUEUE5(data->cur_chan));

    cq6 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type);
    cq6 |= CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(spare) | CMD_INDEX(SMALL_FIXFLOW_READOOB);
    status = ftnandc023_nand_check_cmdq(mtd);
    if (status < 0)
        goto out;
    writel(cq6, data->io_base + CMDQUEUE6(data->cur_chan));

    if (ftnandc023_nand_wait(mtd, chip) == NAND_STATUS_FAIL) {
        status = -1;
        goto out;
    }

    for (i = 0; i < mtd->oobsize; i++) {
        *(buf + i) = readb(data->io_base + SPARE_SRAM + (data->cur_chan << 10) +
                           (data->sel_chip << 6) + i);
    }
  out:
  	platform_select_pinmux(nand_fd, -1);
    return status;
}

static int ftnandc023_nand_read_oob_std(struct mtd_info *mtd, struct nand_chip *chip, int page,
                                        int sndcmd)
{
    struct ftnandc023_nand_data *data = chip->priv;

    data->page_addr = page;

    return data->read_oob(mtd, chip->oob_poi);
}
#if defined(CONFIG_NAND_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)

static int setup_dma(struct ftnandc023_nand_data *data, int direct)
{
    struct dma_slave_config *common;
    
    data->dma_slave_config.id = -1;
    data->dma_slave_config.handshake = DMA_NANDC_REQ;//enable

    common = &data->dma_slave_config.common;
    
#ifdef CONFIG_NAND_USE_AHBDMA    
    if (eccbasft > 9)
    	data->dma_slave_config.src_size = FTDMAC020_BURST_SZ_256;//256x4=1024
    else
    	data->dma_slave_config.src_size = FTDMAC020_BURST_SZ_128;//128x4=512
    
    if(direct == DMA_DEV_TO_MEM){
        data->dma_slave_config.src_sel = AHBMASTER_R_SRC;
        data->dma_slave_config.dst_sel = AHBMASTER_R_DST;
    }else{
        data->dma_slave_config.src_sel = AHBMASTER_W_SRC;
        data->dma_slave_config.dst_sel = AHBMASTER_W_DST;
    }
#else
    common->src_maxburst = 128;
    common->dst_maxburst = 128;
#endif
     
    if(direct == DMA_MEM_TO_DEV){
     	common->src_addr = data->mem_dmaaddr;    
    	common->dst_addr = data->nand_dmaaddr;   
    }else{
     	common->src_addr = data->nand_dmaaddr;    
    	common->dst_addr = data->mem_dmaaddr;     
    }

    common->dst_addr_width = 4;
    common->src_addr_width = 4;
    common->direction = direct;

    return dmaengine_slave_config(data->dma_chan, common);//step 2
}

static int nand_dma_start(struct ftnandc023_nand_data *data, size_t len, int direct)
{
	int ret;
	enum dma_ctrl_flags flags;
	struct dma_async_tx_descriptor *desc;

	ret = setup_dma(data, direct);
	if (ret){
	    printk("Nand setup dma fail\n");
	    return ret;
	}

	flags = DMA_PREP_INTERRUPT | DMA_CTRL_ACK | DMA_COMPL_SKIP_SRC_UNMAP | DMA_COMPL_SKIP_DEST_UNMAP;

	desc = dmaengine_prep_slave_single(data->dma_chan, (void *)data->sg_dmabuf, len, direct, flags);//step 3
			    
	if (!desc){
	    printk("Error dma parameter\n");
	    return ret;
	}	

	desc->callback = ftnandc023_dma_callback;
        desc->callback_param = &data;
        data->cookie = dmaengine_submit(desc);	//step 4
	dma_async_issue_pending(data->dma_chan);//step 5
	
	return 0;
}
#endif

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
                if(ftnandc_read_bbt(mtd, (loff_t) tmp_addr) != 0) {
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
                    if(ftnandc_read_bbt(mtd, (loff_t) tmp_addr) == 0)
                        bad_num--;
    
                    tmp_addr += (0x1 << chip->bbt_erase_shift);
                    page_addr += block_page_num;                    
                }
                //printk("<ar0x%x>\n",page_addr);
            }                        
        }
    }

    return page_addr;
}

static int ftnandc023_nand_read_page_lp(struct mtd_info *mtd, u_char * buf)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    u32 cq6;
    u32 *lbuf, val = 0;
    int status = 0;
    int i, j;
    u8 *p;

    DBGLEVEL2(ftnandc023_dbg
              ("r:page = 0x%x, size = %d, data->column = %d\n", data->page_addr, mtd->writesize,
               data->column));
    lbuf = (u32 *) buf;

    data->page_addr = calc_new_page(mtd, data->page_addr);
   
    platform_select_pinmux(nand_fd, 0);
    if(data->page_addr == 1){//BI table
    	val = readl(data->io_base + ECC_CONTROL);
    	if(!(val & (1 << 8)))
    		printk("### Warning, ECC not enable! ###\n");
    	writel(val & (~(1 << 8)), data->io_base + ECC_CONTROL);
    }
    ftnandc023_set_row_col_addr(data);
    writel(data->page_addr, data->io_base + CMDQUEUE1(data->cur_chan));
    writel(CMD_COUNT(mtd->writesize >> eccbasft) | (data->column / mtd->writesize),
           data->io_base + CMDQUEUE5(data->cur_chan));
    cq6 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type);
    cq6 |= CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(spare) | CMD_INDEX(LARGE_FIXFLOW_PAGEREAD);
#if defined(CONFIG_NAND_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
    cq6 |= (1 << 4);	//enable handshake
#endif
    status = ftnandc023_nand_check_cmdq(mtd);
    if (status < 0)
        goto out;

    writel(cq6, data->io_base + CMDQUEUE6(data->cur_chan));

#if defined(CONFIG_NAND_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
    status = nand_dma_start(data, mtd->writesize, DMA_DEV_TO_MEM);
    if (status < 0) {
        printk("nand dma read page lp fail\n");
        goto out;
    }

    ftnandc023_dma_wait(mtd);
    
    memcpy(buf, data->mem_dmabuf, mtd->writesize);   
#else                     
    for (i = 0; i < mtd->writesize; i += 4)
        *lbuf++ = *(volatile unsigned *)(data->chip.IO_ADDR_R);
#endif

    if (ftnandc023_nand_wait(mtd, chip) == NAND_STATUS_FAIL) {
        status = -1;
        goto out;
    }

    p = chip->oob_poi;
    if (p) {
        for (i = 0; i < (mtd->writesize >> eccbasft); i++) {
            for (j = 0; j < (spare + 1); j++) {
                *(p++) =
                    readb(data->io_base + SPARE_SRAM + (data->cur_chan << 10) +
                          (data->sel_chip << 6) + i * (64 / (mtd->writesize >> eccbasft)) + j);
            }
        }
    }

    /* clear to default value */
    if (p) {
        for (i = avail_oob_sz; i < mtd->oobsize; i++) {
            *(p++) = 0xFF;
        }
    }
  out:
    if((data->page_addr == 1) && (val & (1 << 8)))//BI table
    	writel(val, data->io_base + ECC_CONTROL);

  	platform_select_pinmux(nand_fd, -1);

    return status;
}

static int ftnandc023_nand_read_page_sp(struct mtd_info *mtd, u_char * buf)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    u32 cq6;
    u32 *lbuf;
    int status = 0;
    int i;

    lbuf = (u32 *) buf;

    platform_select_pinmux(nand_fd, 0);
    ftnandc023_set_row_col_addr(data);

    writel(data->page_addr, data->io_base + CMDQUEUE1(data->cur_chan));
    writel(CMD_COUNT(1) | (data->column / mtd->writesize),
           data->io_base + CMDQUEUE5(data->cur_chan));
    cq6 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type);
    cq6 |= CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(spare) | CMD_INDEX(SMALL_FIXFLOW_PAGEREAD);
#if defined(CONFIG_NAND_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
    cq6 |= (1 << 4);	//enable handshake
#endif
    status = ftnandc023_nand_check_cmdq(mtd);
    if (status < 0)
        goto out;
    writel(cq6, data->io_base + CMDQUEUE6(data->cur_chan));
#if defined(CONFIG_NAND_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
    status = nand_dma_start(data, mtd->writesize, DMA_DEV_TO_MEM);
    if (status < 0) {
        printk("nand dma read page sp fail\n");
        goto out;
    }
    ftnandc023_dma_wait(mtd);
    memcpy(buf, data->mem_dmabuf, mtd->writesize);
#else
    for (i = 0; i < mtd->writesize; i += 4)
        *lbuf++ = *(volatile unsigned *)(data->chip.IO_ADDR_R);
#endif
    if (ftnandc023_nand_wait(mtd, chip) == NAND_STATUS_FAIL) {
        status = -1;
        goto out;
    }

    for (i = 0; i < mtd->oobsize; i++) {
        *(chip->oob_poi + i) = readb(data->io_base + SPARE_SRAM + (data->cur_chan << 10) +
                                     (data->sel_chip << 6) + i);
    }

  out:
    platform_select_pinmux(nand_fd, -1);
    return status;
}

static int ftnandc023_nand_read_page(struct mtd_info *mtd, struct nand_chip *chip, uint8_t * buf, int page)
{
    struct ftnandc023_nand_data *data = chip->priv;

    data->buf = (u32 *) buf;

    return data->read_page(mtd, buf);
}

static int ftnandc023_nand_write_oob_lp(struct mtd_info *mtd, const u_char * buf, int len)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    u32 cq6;
    int status = 0, i, j;
    u8 *p;

#if 0
    {
        u8 oob_buf[64] = { 0xAB };
        if (ftnandc023_nand_read_oob_lp(mtd, oob_buf) < 0)
            return -1;

        for (i = 0; i < avail_oob_sz; i++)
            printk("0x%x ", oob_buf[i]);
        printk("\n");

        platform_select_pinmux(nand_fd, 0);
        write_protect(0);
        goto out;
    }
#else

    platform_select_pinmux(nand_fd, 0);
    write_protect(0);

    //have be write by write page with spare
    if (page_map[data->page_addr / 32] & (1 << (data->page_addr % 32))) 
        goto out;
#endif
    ftnandc023_set_row_col_addr(data);

    p = (u8 *) buf;

    for (i = 0; i < (mtd->writesize >> eccbasft); i++) {
        for (j = 0; j < (spare + 1); j++) {
            writeb(*(p++),
                   data->io_base + SPARE_SRAM + (data->cur_chan << 10) +
                   (data->sel_chip << 6) + i * (64 / (mtd->writesize >> eccbasft)) + j);
        }
    }

    writel(data->page_addr, data->io_base + CMDQUEUE1(data->cur_chan));
    writel(CMD_COUNT(mtd->writesize >> eccbasft), data->io_base + CMDQUEUE5(data->cur_chan));
    cq6 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type);
    cq6 |= CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(spare) | CMD_INDEX(LARGE_FIXFLOW_WRITEOOB);
    status = ftnandc023_nand_check_cmdq(mtd);
    if (status < 0) {
        printk("cmdq status error\n");
        goto out;
    }
    writel(cq6, data->io_base + CMDQUEUE6(data->cur_chan));

    if (ftnandc023_nand_wait(mtd, chip) == NAND_STATUS_FAIL) {
        status = -1;
        printk("wait error\n");
        goto out;
    }
  out:
    platform_select_pinmux(nand_fd, -1);
    return status;
}

static int ftnandc023_nand_write_oob_sp(struct mtd_info *mtd, const u_char * buf, int len)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    u32 cq6;
    int i, status = 0;

    platform_select_pinmux(nand_fd, 0);
    write_protect(0);
    ftnandc023_set_row_col_addr(data);

    for (i = 0; i < len; i++)
        writeb(*(buf + i),
               data->io_base + SPARE_SRAM + (data->cur_chan << 10) + (data->sel_chip << 6) + i);

    writel(data->page_addr, data->io_base + CMDQUEUE1(data->cur_chan));
    writel(CMD_COUNT(1), data->io_base + CMDQUEUE5(data->cur_chan));
    cq6 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type);
    cq6 |= CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(spare) | CMD_INDEX(SMALL_FIXFLOW_WRITEOOB);
    status = ftnandc023_nand_check_cmdq(mtd);
    if (status < 0)
        goto out;
    writel(cq6, data->io_base + CMDQUEUE6(data->cur_chan));

    if (ftnandc023_nand_wait(mtd, chip) == NAND_STATUS_FAIL) {
        status = -1;
        goto out;
    }
  out:
    platform_select_pinmux(nand_fd, -1);
    return status;
}

static int ftnandc023_nand_write_oob_std(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
    struct ftnandc023_nand_data *data = chip->priv;

    DBGLEVEL2(ftnandc023_dbg("write oob only to page = 0x%x\n", page));
    data->page_addr = page;

    return data->write_oob(mtd, chip->oob_poi, mtd->oobsize);
}

static int ftnandc023_nand_write_page_lp(struct mtd_info *mtd, const uint8_t * buf)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    u32 cq6;
    int i, j;
    u32 *lbuf;
    int status = 0;
    u8 *p;

    DBGLEVEL2(ftnandc023_dbg
              ("w:page = 0x%x, size = %d, data->column = %d\n", data->page_addr, mtd->writesize,
               data->column));

    platform_select_pinmux(nand_fd, 0);
    write_protect(0);
    p = chip->oob_poi;

    if (p != NULL) {
        for (i = 0; i < (mtd->writesize >> eccbasft); i++) {
            for (j = 0; j < (spare + 1); j++) {
                writeb(*(p++),
                       data->io_base + SPARE_SRAM + (data->cur_chan << 10) +
                       (data->sel_chip << 6) + i * (64 / (mtd->writesize >> eccbasft)) + j);
            }
        }
    }

    lbuf = (u32 *) buf;

    ftnandc023_set_row_col_addr(data);

    writel(data->page_addr, data->io_base + CMDQUEUE1(data->cur_chan));
    writel(CMD_COUNT(mtd->writesize >> eccbasft) | (data->column / mtd->writesize),
           data->io_base + CMDQUEUE5(data->cur_chan));
    cq6 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type);
    cq6 |= CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(spare) | CMD_INDEX(LARGE_FIXFLOW_PAGEWRITE);

#if defined(CONFIG_NAND_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
    cq6 |= (1 << 4);	//enable handshake
#endif
    status = ftnandc023_nand_check_cmdq(mtd);
    if (status < 0)
        goto out;
    writel(cq6, data->io_base + CMDQUEUE6(data->cur_chan));
#if defined(CONFIG_NAND_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
    memcpy(data->mem_dmabuf, buf, mtd->writesize);
    
    status = nand_dma_start(data, mtd->writesize, DMA_MEM_TO_DEV);
    if (status < 0) {
        printk("nand dma write page lp fail\n");
        goto out;
    }
    ftnandc023_dma_wait(mtd);
#else
    for (i = 0; i < mtd->writesize; i += 4)
        *(volatile unsigned *)(data->chip.IO_ADDR_R) = *lbuf++;
#endif
    page_map[data->page_addr / 32] |= (1 << (data->page_addr % 32));

    if (ftnandc023_nand_wait(mtd, chip) == NAND_STATUS_FAIL) {
        status = -1;
        goto out;
    }
  out:
    platform_select_pinmux(nand_fd, -1);

    return status;
}

static int ftnandc023_nand_write_page_sp(struct mtd_info *mtd, const uint8_t * buf)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    u32 cq6;
    int i;
    int status = 0;
    u32 *lbuf;

    platform_select_pinmux(nand_fd, 0);
    write_protect(0);

    lbuf = (u32 *) buf;
    for (i = 0; i < mtd->oobsize; i++)
        writeb(*(chip->oob_poi + i),
               data->io_base + SPARE_SRAM + (data->cur_chan << 10) + (data->sel_chip << 6) + i);

    ftnandc023_set_row_col_addr(data);

    writel(data->page_addr, data->io_base + CMDQUEUE1(data->cur_chan));
    writel(CMD_COUNT(1) | (data->column / mtd->writesize),
           data->io_base + CMDQUEUE5(data->cur_chan));
    cq6 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type);
    cq6 |= CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(spare) | CMD_INDEX(SMALL_FIXFLOW_PAGEWRITE);
#if defined(CONFIG_NAND_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
    cq6 |= (1 << 4);	//enable handshake
#endif
    status = ftnandc023_nand_check_cmdq(mtd);
    if (status < 0)
        goto out;
    writel(cq6, data->io_base + CMDQUEUE6(data->cur_chan));
#if defined(CONFIG_NAND_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
    memcpy(data->mem_dmabuf, buf, mtd->writesize);

    status = nand_dma_start(data, mtd->writesize, DMA_MEM_TO_DEV);
    if (status < 0) {
        printk("nand dma write page sp fail\n");
        goto out;
    }
    ftnandc023_dma_wait(mtd);
#else
    for (i = 0; i < mtd->writesize; i += 4)
        *(volatile unsigned *)(data->chip.IO_ADDR_R) = *lbuf++;
#endif
    if (ftnandc023_nand_wait(mtd, chip) == NAND_STATUS_FAIL) {
        status = -1;
        goto out;
    }
  out:
    platform_select_pinmux(nand_fd, -1);
    return status;
}

static int ftnandc023_nand_write_page(struct mtd_info *mtd, struct nand_chip *chip,
                                      const uint8_t * buf, int page, int cached, int raw)
{
    struct nand_chip *p = mtd->priv;
    struct ftnandc023_nand_data *data = p->priv;
    int status = 0;
#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
    u8 *vbuf;
    int i;
#endif
    data->page_addr = page;

    platform_select_pinmux(nand_fd, 0);
    write_protect(0);

    status = data->write_page(mtd, buf);
    if (status < 0)
        goto out;

#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
    vbuf = vmalloc(mtd->writesize + mtd->oobsize);
    memcpy(vbuf + mtd->writesize, chip->oob_poi, mtd->oobsize);

    status = data->read_page(mtd, vbuf);
    if (status < 0)
        goto out;
    for (i = 0; i < mtd->writesize; i++) {
        if (*(buf + i) != *(vbuf + i)) {
            printk(KERN_ERR "write verify failed at normal region..");
            goto out_free;
        }
    }
    for (i = 0; i < mtd->oobsize; i++) {
        if (*(chip->oob_poi + i) != *(vbuf + mtd->writesize + i)) {
            printk(KERN_ERR "write verify failed at oob region..\n");
            goto out_free;
        }
    }

  out_free:
    vfree(vbuf);
#endif
  out:
    platform_select_pinmux(nand_fd, -1);
    return status;
}

static void ftnandc023_nand_write_page_lowlevel(struct mtd_info *mtd,
                                                struct nand_chip *chip, const uint8_t * buf)
{
}

static void ftnandc023_nand_cmdfunc(struct mtd_info *mtd, unsigned command,
                                    int column, int page_addr)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    u32 cq6, tmp, i;

    platform_select_pinmux(nand_fd, 0);

    cq6 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type);
    data->cur_cmd = command;
    if (page_addr != -1)
        data->page_addr = page_addr;
    if (column != -1)
        data->column = column;
    switch (command) {
    case NAND_CMD_READID:
        /* read ID use sector mode, can't use page mode */
        tmp = readl(data->io_base + SPARE_REGION_ACCESS);
        writel(tmp & ~(1 << 0), data->io_base + SPARE_REGION_ACCESS);
        
        data->byte_ofs = 0;
        cq6 |= CMD_START_CE(data->sel_chip) | CMD_BYTE_MODE | CMD_SPARE_NUM(4);
        if (data->large_page)
            cq6 |= CMD_INDEX(LARGE_FIXFLOW_READID);
        else
            cq6 |= CMD_INDEX(SMALL_FIXFLOW_READID);
        writel(CMD_COUNT(1), data->io_base + CMDQUEUE5(data->cur_chan));
        ftnandc023_nand_check_cmdq(mtd);
        writel(cq6, data->io_base + CMDQUEUE6(data->cur_chan));

        ftnandc023_nand_wait(mtd, chip);
        
        tmp &= ~SPARE_PAGE_MODE(1);//PC tool use sector mode and enable CRC to r/w page
        tmp |= SPARE_PROT_EN(1);
        
        writel(tmp, data->io_base + SPARE_REGION_ACCESS);
        break;
    case NAND_CMD_RESET:
        cq6 |= CMD_START_CE(data->sel_chip);
        if (data->large_page) {
            if (data->flash_type == 3)
                cq6 |= CMD_INDEX(ONFI_FIXFLOW_SYNCRESET);
            else
                cq6 |= CMD_INDEX(LARGE_FIXFLOW_RESET);
        } else {
            cq6 |= CMD_INDEX(SMALL_FIXFLOW_RESET);
        }
        ftnandc023_nand_check_cmdq(mtd);
        writel(cq6, data->io_base + CMDQUEUE6(data->cur_chan));

        ftnandc023_nand_wait(mtd, chip);
        break;
    case NAND_CMD_STATUS:
        writel(CMD_COUNT(1), data->io_base + CMDQUEUE5(data->cur_chan));
        cq6 |= CMD_START_CE(data->sel_chip);
        if (data->large_page)
            cq6 |= CMD_INDEX(LARGE_FIXFLOW_READSTATUS);
        else
            cq6 |= CMD_INDEX(SMALL_FIXFLOW_READSTATUS);
        ftnandc023_nand_check_cmdq(mtd);
        writel(cq6, data->io_base + CMDQUEUE6(data->cur_chan));

        ftnandc023_nand_wait(mtd, chip);
        break;
    case NAND_CMD_ERASE1:
    		write_protect(0);
        DBGLEVEL2(ftnandc023_dbg("erase page: 0x%x\n", data->page_addr));

        if ((data->page_addr / 32) >= max_chip_size)
            printk("=========== Please improve max_chip_size ==========\n");
        //erase one block = N pages
         
        tmp = (data->mtd.erasesize / data->mtd.writesize);
       
        if (tmp <= 32) {
            for (i = 0; i < tmp; i++)
                page_map[data->page_addr / 32] &= ~(1 << (i % 32));
        } else
            for (i = 0; i < tmp / 32; i++)
                page_map[(data->page_addr / 32) + i] = 0;

        writel(data->page_addr, data->io_base + CMDQUEUE1(data->cur_chan));
        writel(CMD_COUNT(1), data->io_base + CMDQUEUE5(data->cur_chan));
        if (data->large_page) {
            ftnandc023_set_row_col_addr(data);
            cq6 |= CMD_INDEX(LARGE_FIXFLOW_ERASE);
        } else {
            ftnandc023_set_row_col_addr(data);
            cq6 |= CMD_INDEX(SMALL_FIXFLOW_ERASE);
        }
        cq6 |= CMD_START_CE(data->sel_chip) | CMD_SCALE(1);
        ftnandc023_nand_check_cmdq(mtd);
        writel(cq6, data->io_base + CMDQUEUE6(data->cur_chan));
        break;
    case NAND_CMD_ERASE2:
    case NAND_CMD_PAGEPROG:
    case NAND_CMD_SEQIN:
        break;
    }
		
		platform_select_pinmux(nand_fd, -1);
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
static int __devinit ftnandc023_nand_probe(struct platform_device *pdev)
{
    struct ftnandc023_nand_data *data;
    struct mtd_partition *partitions;
    int res, chipnum, size;
    int i, type, free_oob_sz;
#ifndef CONFIG_PLATFORM_GM8126
    int partitions_num;
#endif     
    u32 val, tmp;
#if defined(CONFIG_NAND_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
    dma_cap_mask_t mask;
#endif    
    res = chipnum = size = type = 0;
    /* Allocate memory for the device structure (and zero it) */
    data = kzalloc(sizeof(struct ftnandc023_nand_data), GFP_KERNEL);
    if (!data) {
        dev_err(&pdev->dev, "failed to allocate device structure.\n");
        res = -ENOMEM;
        goto out;
    }

    data->io_base = ioremap_nocache(pdev->resource[0].start,
                            pdev->resource[0].end - pdev->resource[0].start + 1);
    printk("NAND reg mapping to addr = 0x%x, phy = 0x%x\n", (u32)data->io_base, (u32)pdev->resource[0].start);
    if (data->io_base == NULL) {
        dev_err(&pdev->dev, "ioremap failed for register.\n");
        res = -EIO;
        goto out_free_data;
    }
    
#if defined(CONFIG_NAND_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
    data->nand_dmaaddr = pdev->resource[1].start;
#else
    data->chip.IO_ADDR_R = ioremap_nocache(pdev->resource[1].start,
                                   pdev->resource[1].end - pdev->resource[1].start + 1);
    //printk("NAND data port mapping to addr = 0x%x, phy = 0x%x\n", (u32)data->chip.IO_ADDR_R, (u32)pdev->resource[1].start);                                   
    if (data->chip.IO_ADDR_R == NULL) {
        dev_err(&pdev->dev, "ioremap failed for data port.\n");
        res = -EIO;
        goto out_no_ior;
    }
  
#endif

    /* The following setting was done in nsboot already. Actually it don't need to be 
     * configured again. 
     */
    writel(REPORT_ADDR_EN | BUSY_RDY_LOC(6) | CMD_STS_LOC(0) | CE_NUM(0),
           data->io_base + GENERAL_SETTING);

    /* Currently, it is fixed in LEGACY_LARGE
     */
    legacy = LEGACY_LARGE;
    val = readl(data->io_base + MEM_ATTR_SET);
    spare = (val >> 16) & 0x3FF;
    val = readl(data->io_base + ECC_CORRECT_BITREG1);
#ifdef CONFIG_PLATFORM_GM8126
    useecc = (val & 0x1F) + 1;
#else
    useecc = (val & 0x3F) + 1;
#endif 
    eccbasft = ((val >> 16) & 0x1) ? 10 : 9;
    val = readl(data->io_base + GENERAL_SETTING);
    usecrc = ((val >> 16) & 0x1) ? 0 : 1;
    data->flash_type = 0;
    data->large_page = 1;

        /*-----------------------------------------------------------
	 * For ONFI or Toggle High Speed Flash, need to adjust delay.
	 */
    if (data->flash_type > LEGACY_LARGE) {
        val = readl(data->io_base + DQS_DELAY);
        val &= ~0x1F;
        val |= 10;
        writel(val, data->io_base + DQS_DELAY);
    }
    
    val = readl(data->io_base + MEM_ATTR_SET);

    data->chip.priv = data;
    data->mtd.priv = &data->chip;
    data->mtd.owner = THIS_MODULE;
    data->mtd.name = "nand-flash";//u-boot commandline
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

        /*--------------------------------------------------------
	 * ONFI flash must work in Asynch mode for READ ID command.
	 * Switch it back to Legacy.
	 */
    if (data->flash_type == ONFI) {
        type = data->flash_type;
        data->flash_type = 0;
    }
    /* Scan to find existance of the device */
    for (i = startchn; i < MAX_CHANNEL; i++) {
        //printk(KERN_INFO "NAND: Scan Channel %d...\n", i);

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

    if (chipnum == 0) {
        res = -ENXIO;
        goto out_unset_drv;
    }

    data->chip.numchips = chipnum;
    data->mtd.size = size;
    data->scan_state = 0;

    data->chip.ecc.layout = &nand_hw_eccoob;
    data->chip.bbt_td = &ftnandc023_bbt_main_descr;
    data->chip.bbt_md = &ftnandc023_bbt_mirror_descr;
    data->chip.badblock_pattern = &ftnandc023_largepage_flashbased;

    /* check spare size */
    if (spare != (data->mtd.writesize >> 5)) {
        printk("Warning............NAND: the spare size %d(%d) is wrong! \n", data->mtd.writesize >> 5, spare);
        res = -ENODEV;
    }

    if (data->mtd.writesize == 2048)
        tmp = 1;
    else if (data->mtd.writesize == 4096)
        tmp = 2;
    else if (data->mtd.writesize == 8192)
        tmp = 3;
    else
        tmp = -1;

    /* check page size */
    if (tmp != (val & 0x3)) {
        printk("Warning............NAND: the page size %d is wrong! \n", data->mtd.writesize);
        res = -ENODEV;
    }

    /* check block size */
    tmp = (data->mtd.erasesize / data->mtd.writesize) - 1;
    if (tmp != ((val >> 2) & 0x3FF)) {
        printk("Warning............NAND: the block size %d is wrong! \n", data->mtd.erasesize);
        res = -ENODEV;
    }

    data->chip.buffers = kmalloc(data->mtd.writesize + (spare << 2), GFP_KERNEL);
    if (!data->chip.buffers) {
        dev_err(&pdev->dev, "failed to allocate chip buffers.\n");
        res = -ENOMEM;
        goto out_unset_drv;
    }

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

#if defined(CONFIG_NAND_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
    dma_cap_set(DMA_SLAVE, data->cap_mask);
    
#ifdef CONFIG_NAND_USE_AHBDMA
    printk("use AHB DMA mode\n");    
    {
    	struct ftdmac020_dma_slave slave;
    	memset(&slave, 0, sizeof(slave));
    	data->dma_chan = dma_request_channel(data->cap_mask, ftdmac020_chan_filter, (void *)&slave);//step 1
    }
#else
    ftpmu010_write_reg(nand_fd, 0xA4, (0x0 << 27), (0x1 << 27));
    printk("use AXI DMA mode\n");
    {
    	struct ftdmac030_dma_slave slave;
    	memset(&slave, 0, sizeof(slave));
    	data->dma_chan = dma_request_channel(data->cap_mask, ftdmac030_chan_filter, (void *)&slave);//step 1
    }  
#endif

    if (!data->dma_chan){
        dev_err(&pdev->dev, "DMA channel allocation failed\n");
        res = -ENODEV;
        goto out_free_buf;
    }
    printk("Nand get DMA channel %d\n", data->dma_chan->chan_id);

    data->mem_dmabuf = dma_alloc_coherent(&pdev->dev, data->mtd.writesize, &data->mem_dmaaddr, GFP_KERNEL);
//printk("vir=%x, phy=%x\n",data->mem_dmabuf,data->mem_dmaaddr);
    if (!data->mem_dmabuf) {
        res = -ENOMEM;
        goto out_free_dma;
    }
    data->sg_dmabuf = dma_to_virt(&pdev->dev, data->mem_dmaaddr);
      
    //printk("sg mem pa = 0x%x, va = 0x%x\n", (u32)data->mem_dmaaddr, (u32)data->sg_dmabuf);

#else
    printk("use PIO mode\n");    
#endif

    /* read the system header first 
     */
    if (1) {
        /* first disble ecc due to potential different ecc capability 
         */
        unsigned int usr_base;
        
#ifdef CONFIG_PLATFORM_GM8126 
        unsigned int blk_base, cfg_base;       
        img_hdr_t *img_hdr;
#endif
        
        /* read system header 
         */
        //data->sys_hdr = kzalloc(data->mtd.writesize, GFP_KERNEL);
        data->sys_hdr = dma_alloc_coherent(&pdev->dev, data->mtd.writesize, &data->syshd_dmaaddr, GFP_KERNEL);
        if (data->sys_hdr == NULL) {
            printk("Warning............NAND: no memory");
            return -ENOMEM;
        }
        data->page_addr = 0;
        data->chip.oob_poi = data->chip.buffers->databuf + data->mtd.writesize;

        if (data->read_page(&data->mtd, (u_char *) data->sys_hdr) < 0) {
            printk("Warning............NAND: read system header fail!");
            return -ENODEV;
        }

        /* read bad block table 
         */  
        //data->bi_table = kzalloc(data->mtd.writesize, GFP_KERNEL);
        data->bi_table = dma_alloc_coherent(&pdev->dev, data->mtd.writesize, &data->bitab_dmaaddr, GFP_KERNEL);
        if (data->bi_table == NULL) {
            printk("Warning............NAND: no table memory");
            return -ENOMEM;
        }
        /* the bi table is next to system header */
        data->page_addr += 1;
#if 0//debug
        memset(data->bi_table, 0xFF, 1024);
#else        
        if (data->read_page(&data->mtd, (u_char *) data->bi_table) < 0) {
            printk("Warning............NAND: read bad block table fail!");
            return -ENODEV;
        }
#endif
            
#ifdef CONFIG_PLATFORM_GM8126 
        blk_base = ftnandc023v2_get_blockbase(&data->mtd, data->sys_hdr->linux_addr);        
        data->page_addr = blk_base >> data->chip.page_shift;

        img_hdr = kmalloc(data->mtd.writesize, GFP_KERNEL);
        if (img_hdr == NULL)
            printk("Warning............NAND: allocate memory fail!");

        if (data->read_page(&data->mtd, (u_char *) img_hdr) < 0)
            printk("Warning............NAND: read bad block table fail!");

        if (img_hdr->magic != IMAGE_MAGIC) {
            printk("NAND:The expected magic number is 0x%x, but we got 0x%x \n",
                   IMAGE_MAGIC, img_hdr->magic);
            printk("Warning............wrong magic number!");
        }
#endif
        partitions = ftnandc023_partition_info;
        
#ifndef CONFIG_PLATFORM_GM8126

        partitions_num = partition_check(partitions, data, data->mtd.writesize); 
#else 
        /* 
         * arrange the MTD partition, PORTING 
         */ 
        /* loader, nsboot.bin */
        partitions[MTD_PART_LOADER].offset =
            ftnandc023v2_get_blockbase(&data->mtd, (1 << data->chip.bbt_erase_shift));
            
        /* uboot */
        partitions[MTD_PART_UBOOT].offset =
            ftnandc023v2_get_blockbase(&data->mtd, data->sys_hdr->uboot_addr);  
        /* burn-in */
        partitions[MTD_PART_BURNIN].offset =
            ftnandc023v2_get_blockbase(&data->mtd, data->sys_hdr->burnin_addr);
            
        /* cfg */
        cfg_base = data->sys_hdr->linux_addr - MTD_CFG_SZ;
        partitions[MTD_PART_CFG].offset = ftnandc023v2_get_blockbase(&data->mtd, cfg_base);

        if (partitions[MTD_PART_CFG].offset <= partitions[MTD_PART_UBOOT].offset + (2 * DEFAULT_BLOCK_SIZE))
            printk("Warning............NAND: uboot space %#x is too small!\n",
                  data->sys_hdr->linux_addr - data->sys_hdr->uboot_addr); 
                  
        /* linux */
        partitions[MTD_PART_LINUX].offset =
            ftnandc023v2_get_blockbase(&data->mtd, data->sys_hdr->linux_addr); 
            
        /* 
         * calculate every partition's MTD size
         */            
        /* loader */
        partitions[MTD_PART_LOADER].size =
            partitions[MTD_PART_BURNIN].offset - partitions[MTD_PART_LOADER].offset;
        /* burn-in */
        partitions[MTD_PART_BURNIN].size =
            partitions[MTD_PART_UBOOT].offset - partitions[MTD_PART_BURNIN].offset;
        /* uboot */
        partitions[MTD_PART_UBOOT].size =
            partitions[MTD_PART_CFG].offset - partitions[MTD_PART_UBOOT].offset;
        /* cfg */
        partitions[MTD_PART_CFG].size =
            partitions[MTD_PART_LINUX].offset - partitions[MTD_PART_CFG].offset;
            
        /* sanity check to see if the space is big enough (including header) */
        if ((img_hdr->size + data->mtd.writesize + (8 * DEFAULT_BLOCK_SIZE)) > MTD_LINUX_SZ)
            printk("Warning............NAND: please redefine MTD_LINUX_SZ: %#x \n", MTD_LINUX_SZ);  
            
        /* linux, image header + image size */
        partitions[MTD_PART_LINUX].size = BLOCK_ALIGN(MTD_LINUX_SZ, data->chip.bbt_erase_shift);                      
#endif

        /* 
         * calculate the mtd of user configuration base
         */
        /* user partition */         
#ifndef CONFIG_PLATFORM_GM8126 
        usr_base = data->sys_hdr->image[partitions_num].addr + data->sys_hdr->image[partitions_num].size + (2 * data->mtd.writesize);
        partitions[partitions_num].offset = ftnandc023v2_get_blockbase(&data->mtd, usr_base);
#else         
        usr_base = partitions[MTD_PART_LINUX].offset + partitions[MTD_PART_LINUX].size;
        partitions[MTD_PART_USER].offset = ftnandc023v2_get_blockbase(&data->mtd, usr_base);
        
        kfree(img_hdr);        
#endif        
        /* restore the orginal setting */
        data->page_addr = 0;
    }

    avail_oob_sz = i = ftnandc023_available_oob(&data->mtd);
    /* default value */
    free_oob_sz = 3 /* bytes */  * (data->mtd.writesize >> eccbasft);
#ifdef CONFIG_YAFFS_FS
    free_oob_sz = 16;
#endif
    if (likely(i >= free_oob_sz)) {
        if (i > 64)
            data->mtd.oobsize = 64;
        else
            data->mtd.oobsize = i;
        data->chip.ecc.layout->oobfree[0].length = data->mtd.oobsize;
    } else {
        printk("NAND: oob size is too small! \n");
        res = -ENXIO;
        goto out_unset_drv;
    }

    printk("NAND Chip: oobsize:%#x, pagesize:%#x, blocksize:%#x, chipsize:%#x, "
           "ECC capbility is %d bits, CRC protection is %s\n",
           (int)spare, (int)data->mtd.writesize, (int)data->mtd.erasesize, (int)data->chip.chipsize,
           useecc, usecrc ? "enabled" : "disabled");

#if 1                           /* Harry, in MTD utility, the oobsize should be multiple of 64. We should be on the way. */
    data->mtd.oobsize = spare;
    spare = (avail_oob_sz / (data->mtd.writesize >> eccbasft)) - 1;
#else
    printk("NAND: total oobsize: %d\n", data->mtd.oobsize);
    spare = (data->mtd.oobsize / (data->mtd.writesize >> eccbasft)) - 1;
    printk("NAND: oobsize per sector: %d\n", spare + 1);
#endif
    /* Scan bad block and create bbt table
     */
    nand_scan_tail(&data->mtd);

        /*----------------------------------------------------------
	 * ONFI synch mode means High Speed. If fails to change to
	 * Synch mode, then use flash as Async mode(Normal speed) and
	 * use LEGACY_LARGE fix flow.
	 */
    if (type == ONFI) {
        if (ftnandc023_onfi_sync(&data->mtd) == 0)
            data->flash_type = ONFI;
        else
            data->flash_type = 0;
    }

    ftnandc023_calc_timing(&data->mtd);
#ifndef CONFIG_PLATFORM_GM8126
    res = mtd_device_parse_register(&data->mtd, NULL, 0, partitions, partitions_num);
#else    
    res = mtd_device_parse_register(&data->mtd, NULL, 0, partitions, ARRAY_SIZE(ftnandc023_partition_info));
#endif    
    if (!res)
        return res;

    nand_release(&data->mtd);
#if defined(CONFIG_NAND_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
  out_free_dma:
    if(data->dma_chan)
    	dma_release_channel(data->dma_chan);
  out_free_buf:
#endif
    kfree(data->chip.buffers);

  out_unset_drv:
    platform_set_drvdata(pdev, NULL);

#if defined(CONFIG_NAND_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
    if(data->mem_dmabuf)
        dma_free_coherent(&pdev->dev, data->mtd.writesize, data->mem_dmabuf, data->mem_dmaaddr);
#else
  out_no_ior:
    iounmap(data->chip.IO_ADDR_R);
#endif
    iounmap(data->io_base);
  out_free_data:
    if (data->sys_hdr)
        kfree(data->sys_hdr);
    if (data->bi_table)
        kfree(data->bi_table);
    kfree(data);
  out:
    return res;
}

/*
 * @consult with bad block table about this block is good or bad.
 *
 * @ftnandc_read_bbt(struct mtd_info *mtd, loff_t offs)
 * @param mtd: MTD device structure
 * @param offs: block base address
 * @return: 1 for bad block, 0 for good block
*/
int ftnandc_read_bbt(struct mtd_info *mtd, loff_t offs)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc023_nand_data *data = chip->priv;
    unsigned int *bi_table = (unsigned int *)data->bi_table;
    int quotient, remainder, blk_id, result;

    blk_id = offs >> chip->bbt_erase_shift;
    quotient = blk_id >> 5;
    remainder = blk_id & 0x1F;
    result = (bi_table[quotient] >> remainder) & 0x1;
    
    if (result == 1)
        return 0;               /* good */

    return 1;                   /* bad */
}

unsigned char spare_buf[64] = {0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7
															,0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7
															,0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7
															,0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7};

extern int erase_write_block0 (struct mtd_info *mtd, unsigned long pos, int len, const char *buf);
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
	struct nand_chip *chip = mtd->priv;
	struct ftnandc023_nand_data *data = chip->priv;
	unsigned int *bi_table = (unsigned int *)data->bi_table;
	int ret = 0;
	int quotient, remainder, blk_id, result;
	size_t retlen;

    printk("markbad, addr = 0x%x\n", (int)ofs);

	/* Get block number */
	blk_id = (int)(ofs >> chip->bbt_erase_shift);
	if (data->bi_table){
	    blk_id = ofs >> chip->bbt_erase_shift;
	    quotient = blk_id >> 5;
	    remainder = blk_id & 0x1F;
    	result = (bi_table[quotient] >> remainder) & 0x1;
    	//printk("BI table %dth, bit %d, data = 0x%x, result = %d\n", quotient, remainder, bi_table[quotient], result);
    	if (result == 0)/* bad block, not need to do it */
      	return 0;               
	    
	    bi_table[quotient] &= ~(1 << remainder);/* Write the block mark. */
	    blk_id <<= 1;
	    //printk("blk_id %dth, data = 0x%x, result = %x\n", blk_id, chip->bbt[blk_id >> 3], (0x3 << (blk_id & 0x06)));
	    chip->bbt[blk_id >> 3] |= (0x3 << (blk_id & 0x06));	 	    
	}

	ret = erase_write_block0(mtd, 0, mtd->writesize, (const uint8_t *)data->sys_hdr);
	if (ret < 0)
		printk("Markbad Step 1 Error\n");

	ret = mtd_write(mtd, mtd->writesize, mtd->writesize, &retlen, (const uint8_t *)bi_table);
	if (ret < 0)
		printk("Markbad Step 2 Error\n");

	return ret;
}

/*
 * @get block baseaddr. It skips the bad block as well.
 *
 * @ftnandc023v2_get_blockbase(struct mtd_info *mtd, unsigned int base_addr)
 * @param mtd: MTD device structure
 * @param base_addr: liner base address
 * @return: good block address
*/
unsigned int ftnandc023v2_get_blockbase(struct mtd_info *mtd, unsigned int base_addr)
{
    struct nand_chip *chip = mtd->priv;
    unsigned int block_base;

    block_base = BLOCK_ALIGN(base_addr, chip->bbt_erase_shift);

    while (ftnandc_read_bbt(mtd, (loff_t) block_base) != 0)        /* find good block */
        block_base += (0x1 << chip->bbt_erase_shift);   //move to next block

    return block_base;
}

/*
 * Remove a NAND device.
 */
static int __devexit ftnandc023_nand_remove(struct platform_device *pdev)
{
    struct ftnandc023_nand_data *data = platform_get_drvdata(pdev);

#if defined(CONFIG_NAND_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
    if(data->mem_dmabuf)
        dma_free_coherent(&pdev->dev, data->mtd.writesize, data->mem_dmabuf, data->mem_dmaaddr);
    if(data->dma_chan)
    	dma_release_channel(data->dma_chan);
#endif
    if(data->sys_hdr)
        dma_free_coherent(&pdev->dev, data->mtd.writesize, data->sys_hdr, data->syshd_dmaaddr);
    if(data->bi_table)
        dma_free_coherent(&pdev->dev, data->mtd.writesize, data->bi_table, data->bitab_dmaaddr);
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
    .probe = ftnandc023_nand_probe,
    .remove = __devexit_p(ftnandc023_nand_remove),
    .driver = {
               .name = "ftnandc023_nand",
               .owner = THIS_MODULE,
               },
};

static int __init ftnandc023_nand_init(void)
{
    int ret = 0;
    
#ifdef CONFIG_PLATFORM_GM8210
		fmem_pci_id_t pci_id;
		fmem_cpu_id_t cpu_id;
		fmem_get_identifier(&pci_id, &cpu_id);
		if((cpu_id != FMEM_CPU_FA726) || (pci_id != FMEM_PCI_HOST))
			return 0;
#endif
    /* check if the system is running NAND system 
     */   
#if defined(CONFIG_PLATFORM_GM8210) || defined(CONFIG_PLATFORM_GM8126) || defined(CONFIG_PLATFORM_GM8287)
    if (platform_check_flash_type() != 0){
        printk("Not for NAND pin mux\n");
        return 0;
    }
#endif

#if defined(CONFIG_NAND_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
    init_waitqueue_head(&nand023_queue);
#endif    
    /* Register PMU and turn on gate clock
     */
    nand_fd = ftpmu010_register_reg(&pmu_reg_info);
    if (nand_fd < 0) {
        printk("Warning............NANDC: register PMU fail");
        ret = -ENODEV;
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

static void __exit ftnandc023_nand_exit(void)
{
    /* check if the system is running NAND system 
     */
#if defined(CONFIG_PLATFORM_GM8210) || defined(CONFIG_PLATFORM_GM8126) || defined(CONFIG_PLATFORM_GM8287)
    if (platform_check_flash_type() != 0)
        return;
#endif
    /* Deregister PMU
     */
    ftpmu010_deregister_reg(nand_fd);

    platform_driver_unregister(&ftnandc023_nand_driver);
    platform_device_unregister(&ftnandc023_device);
}

#if defined(CONFIG_NAND_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
/*
 * @callback function from DMAC module
 *
 * @ftnandc023_dma_callback int func(int ch, u16 int_status, void *data)
 * @param ch is used to indicate DMA channel
 * @param int_status indicates the interrupt status of DMA controller
 * @param data indicates the private data
 * @return: none
*/
void ftnandc023_dma_callback(void *param)
{
    //printk("%s\n", __func__);

    trigger_flag = 1;
    wake_up(&nand023_queue);

    return;
}
#endif

module_init(ftnandc023_nand_init);
module_exit(ftnandc023_nand_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Te-Chen Ying");
MODULE_DESCRIPTION("FTNANDC023 V2.0 NAND driver");
MODULE_ALIAS("platform:ftnandc023_nand");
