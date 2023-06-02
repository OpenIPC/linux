/*
 * FTnandc024 NAND driver
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
#include <mach/ftpmu010.h>
#include <linux/gpio.h>
#include "ftnandc024v2_nand.h"
#include <linux/delay.h>
/*
 * Local function or variables declaration
 */
//#undef CONFIG_NAND_V2_USE_AHBDMA//???
#if defined(CONFIG_NAND_V2_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
#include <linux/dmaengine.h>

#ifdef CONFIG_NAND_V2_USE_AHBDMA
#include <mach/ftdmac020.h>
#else
#include <mach/ftdmac030.h>
#endif

#ifdef CONFIG_PLATFORM_GM8139
#define AHBMASTER_R_SRC   FTDMA020_AHBMASTER_0
#define AHBMASTER_R_DST   FTDMA020_AHBMASTER_1
#define AHBMASTER_W_SRC   FTDMA020_AHBMASTER_1
#define AHBMASTER_W_DST   FTDMA020_AHBMASTER_0
#define DMA_NANDC_REQ    2 
#endif

static void ftnandc024_dma_callback(void *param);
static volatile unsigned int trigger_flag = 0;
static wait_queue_head_t nand024_queue;
struct ftnandc024_nand_data;
static int nand_dma_start(struct ftnandc024_nand_data *data, size_t len, int direct);
#endif 

#define PORTING 
static int ftnandc024v2_block_markbad(struct mtd_info *mtd, loff_t ofs);
static unsigned int ftnandc024v2_get_blockbase(struct mtd_info *mtd, unsigned int base_addr);
static int nand_fd;

/*
 * Macro definitions
 */
#define DEFAULT_BLOCK_SIZE  0x20000 
#define MTD_CFG_SZ          (6 * DEFAULT_BLOCK_SIZE)	//2 block + reserved

#define AHB_Memory_8KByte   (1 << 4)
#define CONFIG_FTnandc024_START_CHANNEL     0
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
static struct ftnandc024_nandchip_attr nand_attr[] = {
	/* 
 	 * Manufacturer ID, spare size, ECC bits, ECC base shift, 
 	 * ECC for spare, Block Boundary, Protect Spare, legacy flash
 	 * */
	{"Micron 29F16G08MAA", 
          218, 8, 9, 4, 128, 1, LEGACY_FLASH},	/* 4K MLC */
	{"Samsung K9F4G08U0A", 
          64, 6, 9, 1, 64, 1, LEGACY_FLASH},	/* 2K SLC */
	{"Samsung K9F1G08U0D", 
          64, 6, 9, 1, 64, 1, LEGACY_FLASH},	/* 2K SLC */
	{"Hynix HY27US08561A", 
          16, 3, 9, 1, 32, 1, LEGACY_FLASH},	/* 512B SLC */
};

static int eccbasft, spare, protect_spare, useecc, useecc_spare, block_boundary;

static struct mtd_partition PORTING ftnandc024_partition_info[20];

static int ref_count = 0;
/*
 * @the purpose of this function is used to switch the pinmux
 *
 * @int platform_select_pinmux(int fd)
 * @param : fd 
 * @return: 0 for success, < 0 for fail.
 */
static int platform_select_pinmux(int fd, int chip)
{
    int ret = 0;
    volatile int i;

#if defined(CONFIG_PLATFORM_GM8139)  
    if (chip >= 0) {
        if (ref_count == 0) {
            
    		if (ftpmu010_request_pins(fd, 0x28, (0xF << 7), 1) < 0) {
    			printk("Warning ===============> NAND request pin failed\n");
    		}
       
            /* the delay is used to prevent data in the bus from incomplete */
            for (i = 0; i < 1000; i++) {}            
            
            ftpmu010_write_reg(fd, 0x58, (0xA << 0), (0xF << 0));
            ftpmu010_write_reg(fd, 0x58, (0xAAA << 6), (0xFFF << 6));
            ftpmu010_write_reg(fd, 0x58, (0xA << 24), (0xF << 24));
            ftpmu010_write_reg(fd, 0x64, (0x3 << 12), (0x3 << 12));
        }
        ref_count ++;
    }
    else {
        ref_count -- ;
        if (ref_count == 0) {
            /* chip unselect, the delay is used to prevent data in the bus from incomplete */
            for (i = 0; i < 1000; i++) {}

	        ftpmu010_release_pins(fd, 0x28, (0xF << 7));
        }
    }  
#endif    
    return ret;
}
int ftnandc_read_bbt(struct mtd_info *mtd, loff_t offs);

#ifdef CONFIG_PLATFORM_GM8139
static pmuReg_t  pmu_reg[] = {
    /* off, bitmask,   lockbit,    init val,   init mask */
    //{0x28, (0x1 << 6), (0x1 << 6), (0x0 << 6), (0x1 << 6)},   /* frequency setting */
    {0x54, (0x3F << 26), (0x3F << 26), (0x2A << 26), (0x3F << 26)},   /* pinMux */
    {0x58, (0xF << 0), (0xF << 0), (0xA << 0), (0xF << 0)},   /* pinMux bit 0~3 */    
    {0x58, (0xFFF << 6), (0xFFF << 6), (0xAAA << 6), (0xFFF << 6)},	/* pinMux bit 6~17 */
    {0x58, (0xF << 24), (0xF << 24), (0xA << 24), (0xF << 24)},	/* pinMux bit 24~27 */
    {0x64, (0x3 << 12), (0x3 << 12), (0x3 << 12), (0x3 << 12)},		/* pinMux */
    {0xB4, (0x1 << 16), (0x1 << 16), (0x0 << 16), (0x1 << 16)},   /* AHB clock gate */
};
#endif

static pmuRegInfo_t	pmu_reg_info = {
    "nandc024",
    ARRAY_SIZE(pmu_reg),
    ATTR_TYPE_NONE,
    pmu_reg
};

#define MTD_LOGO_SZ         (4 << 20)  //4M bytes
#define MTD_LINUX_SZ        (30 << 20)  //30M bytes

typedef struct nand024_sys_header {	
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
        unsigned int nand_sparesz_inpage;       //64bytes for 2k, ...... needed for nandc024
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
} nand024_sys_header_t;

//=============================================================================
// BI table, size = 1024 bytes
//=============================================================================
typedef struct bi_table {
    /* This array size is related to USB_BUFF_SZ defined in usb_scsi.h */
    unsigned int bi_status[256];        //each bit indicates a block. 1 for good, 0 for bad
} bi_table_t;

struct ftnandc024_nand_data {
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
#if defined(CONFIG_NAND_V2_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)   
    struct dma_chan             *dma_chan;
    dma_cap_mask_t              cap_mask;
#ifdef CONFIG_NAND_USE_AXIDMA
    struct ftdmac030_dma_slave  dma_slave_config;
#else 
    struct ftdmac020_dma_slave  dma_slave_config;   
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
    int cmd_status;
    char flash_raw_id[5];
    int flash_type;
    int large_page;
    nand024_sys_header_t *sys_hdr;      /* system header */
    bi_table_t *bi_table;       /* bad block table next to sys_hdr */
    int curr_param_group;
    int (*write_oob) (struct mtd_info * mtd, const u_char * buf, int len);
    int (*read_oob) (struct mtd_info * mtd, u_char * buf);
    int (*write_page) (struct mtd_info * mtd, const uint8_t * buf);
    int (*read_page) (struct mtd_info * mtd, u_char * buf);
};

#ifdef CONFIG_MTD_NAND_FTNANDC024
static int startchn = CONFIG_FTnandc024_START_CHANNEL;
#else
static int startchn = 0;
module_param(startchn, int, 0644);
#endif

#define	max_chip_size	(2048 * 16)     //2048MB NAND

static int partition_check(struct mtd_partition *partitions, struct ftnandc024_nand_data *data, int block_size)
{
    int i, num = 0;
    int j, A_begin, A_end, B_begin, B_end;
    
    nand024_sys_header_t *sys_hdr;
    
    sys_hdr = data->sys_hdr;

	if(sys_hdr->image[0].size == 0){
		printk("Not find partition message, use default setting\n");
	    //maybe has bad block, we can change address
	    partitions[0].name = "a";
	    partitions[0].offset = 0x240000;
	    partitions[0].size = 0x200000;
	    
	    partitions[1].name = "b";
	    partitions[1].offset = 0x600000;
	    partitions[1].size = 0x200000;
	
	    partitions[2].name = "c";
	    partitions[2].offset = 0x0;
	    partitions[2].size = 0x100000;
	        
	    num = 3;    
	}else{
	    for(i = 0; i < ARRAY_SIZE(ftnandc024_partition_info); i++){    
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

        /* nsboot image */
        partitions[num].offset = sys_hdr->bootm_addr;
        partitions[num].size = block_size;
        partitions[num].name = "NSBOOT";        
        num++;
    		
	    for(i = 0; i < ARRAY_SIZE(ftnandc024_partition_info); i++){
	        DBGLEVEL2(ftnandc024_dbg("partation %d addr = 0x%x, size = 0x%x, name = %s\n", i, sys_hdr->image[i].addr, sys_hdr->image[i].size, sys_hdr->image[i].name));
	    }
	    
	    for(i = 0; i < (num - 1); i++)
	    	if(partitions[i + 1].offset >= partitions[i].offset)
		        if(partitions[i + 1].offset - partitions[i].offset < (2 * block_size)){
		            printk("Warning... Block reserve for bad issue between partition %d with %d is not enough\n", i, i + 1);
		            printk("partition %d addr = 0x%x, %d addr = 0x%x\n", i, (u32)partitions[i].offset, i + 1, (u32)partitions[i + 1].offset);
		        }
	} 
		
		//overlap check
    for(i = 0; i < (num - 2); i++){
      A_begin = sys_hdr->image[i].addr;
      A_end = A_begin + sys_hdr->image[i].size;
			//printk("A %x,%x\n",A_begin,A_end);
			
			for(j = (i + 1); j < num; j++){
	      B_begin = sys_hdr->image[j].addr;
	      B_end = B_begin + sys_hdr->image[j].size;
				//printk("B %x,%x\n",B_begin,B_end);
				      
	      /* A_end between B_end and B_begin */
	      if((B_end >= A_end) && (A_end > B_begin))
	      	goto check_fail;
	      /* A_begin between B_end and B_begin */
	      if((B_end > A_begin) && (A_begin >= B_begin))
	      	goto check_fail;
	      /* B between A */
	      if((A_end >= B_end) && (B_begin >= A_begin))
	      	goto check_fail;	      	      	
			}
    }
    		   
    return num;
check_fail:
		printk("Warning ============> partition %d overlap with %d\n", i, j);
    return num;    
}

#if 0
/* Note: The unit of tWPST/tRPST/tWPRE/tRPRE field of ftnandc024_chip_timing is ns.
 *
 * tWH, tCH, tCLH, tALH, tCALH, tWP, tREH, tCR, tRSTO, tREAID,
 * tREA, tRP, tWB, tRB, tWHR, tWHR2, tRHW, tRR, tAR, tRC
 * tADL, tRHZ, tCCS, tCS, tCS2, tCLS, tCLR, tALS, tCALS, tCAL2, tCRES, tCDQSS, tDBS, tCWAW, tWPRE,
 * tRPRE, tWPST, tRPST, tWPSTH, tRPSTH, tDQSHZ, tDQSCK, tCAD, tDSL
 * tDSH, tDQSL, tDQSH, tDQSD, tCKWR, tWRCK, tCK, tCALS2, tDQSRE, tWPRE2, tRPRE2, tCEH
 
 */
static struct ftnandc024_chip_timing chip_timing = {
	10, 5, 5, 5, 0, 12, 10, 0, 0, 0,
	20, 12, 100, 0, 60, 0, 100, 20, 10, 0,
	70, 100, 0, 20, 0, 12, 10, 12, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#endif

static struct resource ftnandc024_resource[] = {
    [0] = {
           .start = NAND_FTNAND024_PA_BASE,     /* Register Base address */
           .end = NAND_FTNAND024_PA_LIMIT,
           .flags = IORESOURCE_MEM,
           },
    [1] = {
           .start = NANDDP_FTNAND024_PA_BASE,   /* BMC buffer or Data Port access */
           .end = NANDDP_FTNAND024_PA_LIMIT,
           .flags = IORESOURCE_MEM,
           },
    [2] = {
           .start = NAND_FTNAND024_IRQ,
           .end = NAND_FTNAND024_IRQ,
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

static uint8_t ftnandc024_bbt_pattern[] = { 'B', 'b', 't', '0' };
static uint8_t ftnandc024_mirror_pattern[] = { '1', 't', 'b', 'B' };

static struct nand_bbt_descr ftnandc024_bbt_mirror_descr = {
    .options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
        | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
    .offs = 0,
    .len = 4,
    .veroffs = 4,
    .maxblocks = 4,
    .pattern = ftnandc024_mirror_pattern
};

static struct nand_bbt_descr ftnandc024_bbt_main_descr = {
    .options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
        | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
    .offs = 0,
    .len = 4,
    .veroffs = 4,
    .maxblocks = 4,
    .pattern = ftnandc024_bbt_pattern
};

static uint8_t ftnandc024_scan_ff_pattern[] = { 0xff, 0xff, 0xff, 0xff };

static struct nand_bbt_descr ftnandc024_largepage_flashbased = {
    .options = NAND_BBT_SCAN2NDPAGE,
    .offs = 0,
    .len = 4,
    .pattern = ftnandc024_scan_ff_pattern
};

static void ftnandc024_regdump(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
	volatile u32 val;
	u32 i;
	
	printk("===================================================\n");
	printk("0x0000: ");
	for(i = 0; i< 0x50; i=i+4){
		if(i != 0 && (i%0x10)==0){
			printk("\n");
			printk("0x%04x: ", i);
		}
		val = readl(data->io_base + i);
		printk("0x%08x ", val);
	} 
	for(i = 0x100; i< 0x1B0; i=i+4){
		if(i != 0 && (i%0x10)==0){
			printk("\n");
			printk("0x%04x: ", i);
		}
		val = readl(data->io_base + i);
		printk("0x%08x ", val);
	} 
	for(i = 0x200; i< 0x530; i=i+4){
		if(i != 0 && (i%0x10)==0){
			printk("\n");
			printk("0x%04x: ", i);
		}
		val = readl(data->io_base + i);
		printk("0x%08x ", val);
	} 
	printk("\n===================================================\n");
}

static inline void ftnandc024_set_row_col_addr(struct ftnandc024_nand_data *data, int row, int col)
{
    int val;

    val = readl(data->io_base + MEM_ATTR_SET);
    val &= ~(0x7 << 12);
    val |= (ATTR_ROW_CYCLE(row) | ATTR_COL_CYCLE(col));

    writel(val, data->io_base + MEM_ATTR_SET);
}

/* low enable write protect, high disable write protect */
static void write_protect(int mode)
{
#ifdef CONFIG_GPIO_WP
		if(mode)
			gpio_direction_output(GPIO_PIN, 0);
		else
			gpio_direction_output(GPIO_PIN, 1);
#endif
}

static int ftnandc024_nand_check_cmdq(struct mtd_info *mtd)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc024_nand_data *data = chip->priv;
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

static int ftnandc024_issue_cmd(struct mtd_info *mtd, struct cmd_feature *cmd_f) {
	struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
	int status;

	status = ftnandc024_nand_check_cmdq(mtd);
	if (status == 0) {
		ftnandc024_set_row_col_addr(data, cmd_f->row_cycle, cmd_f->col_cycle);
		
		writel(cmd_f->cq1, data->io_base + CMDQUEUE1(data->cur_chan));
		writel(cmd_f->cq2, data->io_base + CMDQUEUE2(data->cur_chan));
		writel(cmd_f->cq3, data->io_base + CMDQUEUE3(data->cur_chan));
		writel(cmd_f->cq4, data->io_base + CMDQUEUE4(data->cur_chan)); // Issue cmd

		//printk("cmd = 0x%x\n", (cmd_f->cq4 >> 8) & 0x3FF);
	}
	return status;
}
/*
static void ftnandc024_soft_reset(struct mtd_info *mtd)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc024_nand_data *data = chip->priv;
    unsigned long timeo = jiffies;

    platform_select_pinmux(nand_fd, 0);

    writel(1, data->io_base + GLOBAL_RESET);
    
    timeo += HZ;
    while (time_before(jiffies, timeo)) {
        if (readl(data->io_base + GLOBAL_RESET) == 0)
            break;
    }
    platform_select_pinmux(nand_fd, -1);
}*/

static void ftnandc024_fill_prog_flow(struct mtd_info *mtd, int *program_flow_buf, int buf_len)
{
	struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
	u8 *p = (u8 *)program_flow_buf;
	
	int i;
	for(i = 0; i < buf_len; i++) {
		writeb( *(p+i), data->io_base + PROGRAMMABLE_FLOW_CONTROL + i);
	}
}

#if defined(CONFIG_NAND_V2_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
static int ftnandc024_dma_wait(struct mtd_info *mtd)
{
    int ret = -1;
  
    ret = wait_event_timeout(nand024_queue, trigger_flag == 1, 10 * HZ);
    if (ret < 0) {
    	printk("nand024 dma queue wake up timeout signal arrived\n");
    	return -1;
    }

    trigger_flag = 0; 
    
    return 0;
}
#endif

static int ftnandc024_nand_wait(struct mtd_info *mtd, struct nand_chip *chip)
{
    struct ftnandc024_nand_data *data = chip->priv;

    unsigned long timeo = jiffies;
    int ret, state = chip->state;
	volatile u32 intr_sts, ecc_intr_sts;
	volatile u8 cmd_comp_sts, sts_fail_sts;
	volatile u8 ecc_sts_for_data, ecc_hit_sts_for_data;
	volatile u8 ecc_sts_for_spare, ecc_hit_sts_for_spare;

    platform_select_pinmux(nand_fd, 0);

    ret = NAND_STATUS_FAIL;
    timeo += 5 * HZ;


	while (time_before(jiffies, timeo)) {//ecc error handle
		intr_sts = readl(data->io_base + INTR_STATUS);
		cmd_comp_sts = ((intr_sts & 0xFF0000) >> 16);

		if (likely(cmd_comp_sts & (1 << (data->cur_chan)))) {
			// Clear the intr status when the cmd complete occurs.
			writel(intr_sts, data->io_base + INTR_STATUS);
			
			ret = NAND_STATUS_READY;
			sts_fail_sts = (intr_sts & 0xFF);
				
			if (sts_fail_sts & (1 << (data->cur_chan))) {
				printk(KERN_ERR "Warning, Card report status not stable@(pg_addr:0x%x), please use another page next time\n", data->page_addr);
				data->cmd_status |= CMD_STATUS_FAIL;
				ret = NAND_STATUS_FAIL;
				//ftnandc024_soft_reset(mtd);
			}
			
			ecc_intr_sts = readl(data->io_base + ECC_INTR_STATUS);
			// Clear the ECC intr status
			writel(ecc_intr_sts, data->io_base + ECC_INTR_STATUS);
			//if(ecc_intr_sts)
			//    printk("<ecc sts=0x%x, flow=0x%x>",ecc_intr_sts,state);	
		
			//if ((state == FL_READING) || (state == FL_READY)) {//???
			if (state == FL_READY) {

				// ECC failed on data		
				ecc_sts_for_data = (ecc_intr_sts & 0xFF); 
				if (ecc_sts_for_data & (1 << data->cur_chan)) {
					data->cmd_status |= CMD_ECC_FAIL_ON_DATA;
					//printk("CMD_ECC_FAIL_ON_DATA\n");
					ret = NAND_STATUS_FAIL;
				}
				// Hit the ECC threshold
				ecc_hit_sts_for_data = ((ecc_intr_sts & 0xFF00) >> 8);
				if (ecc_hit_sts_for_data & (1 << data->cur_chan)) {
				}
				
				ecc_sts_for_spare = ((ecc_intr_sts & 0xFF0000) >> 16); 
				// ECC failed on spare	
				if (ecc_sts_for_spare & (1 << data->cur_chan)) {
					data->cmd_status |= CMD_ECC_FAIL_ON_SPARE;
					//printk("CMD_ECC_FAIL_ON_SPARE\n");
					ret = NAND_STATUS_FAIL;
				}
				// Hit the ECC threshold for spare
				ecc_hit_sts_for_spare = ((ecc_intr_sts & 0xFF000000) >> 24);
				if (ecc_hit_sts_for_spare & (1 << data->cur_chan)) {
				}

			}
			goto out_wait;
		}	
		cond_resched();	
	}

    DBGLEVEL1(ftnandc024_dbg("nand wait time out\n"));
    ftnandc024_regdump(mtd);
  out_wait:

  	write_protect(1);
  	platform_select_pinmux(nand_fd, -1);
    return ret;
}

static void ftnandc024_set_default_timing(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
	int i;
	u32 timing[4];

	timing[0] = 0x0f1f0f1f;
	timing[1] = 0x00007f7f;
	timing[2] = 0x7f7f7f7f;
	timing[3] = 0xff1f001f;

	for (i = 0;i < MAX_CHANNEL;i++) {
		writel(timing[0], data->io_base + FL_AC_TIMING0(i));
		writel(timing[1], data->io_base + FL_AC_TIMING1(i));
		writel(timing[2], data->io_base + FL_AC_TIMING2(i));
		writel(timing[3], data->io_base + FL_AC_TIMING3(i));
	}
}

static void ftnandc024_set_default_warmup_cyc(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
	char val;
	
	val = readb(data->io_base + MEM_ATTR_SET2);
	val &= (char)(~0xFF);
	writeb(val, data->io_base + MEM_ATTR_SET2); 
}

/* The unit of Hclk is MHz, and the unit of Time is ns.
 * We desire to calculate N to satisfy N*(1/Hclk) > Time given Hclk and Time
 * ==> N > Time * Hclk
 * ==> N > Time * 10e(-9) * Hclk *10e(6)        --> take the order out
 * ==> N > Time * Hclk * 10e(-3)
 * ==> N > Time * Hclk / 1000
 * ==> N = (Time * Hclk + 999) / 1000
 */
static void ftnandc024_calc_timing(struct mtd_info *mtd)
{
#if 1//use low speed, use default timing
    return;
#else
	struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
	int i;
	int tWH, tWP, tREH, tRES, tBSY, tBUF1;
	int tBUF2, tBUF3, tBUF4, tPRE, tRLAT, t1;
	int tPST, tPSTH, tWRCK;

	struct ftnandc024_chip_timing *p;
	u32 CLK, timing[4];

	CLK = ftpmu010_get_attr(ATTR_TYPE_AHB) / 1000000;
	
	tWH = tWP = tREH = tRES =  0;
	tRLAT = tBSY = t1 = 0;
	tBUF4 = tBUF3 = tBUF2 = tBUF1 = 0;
	tPRE = tPST = tPSTH = tWRCK = 0;
#if defined (CONFIG_FTNANDC024_MICRON_29F32G08CBABB)
	if (data->flash_type == ONFI2)
		p = &sync_timing;
	else
#endif
		p = &chip_timing;

	// tWH = max(tWH, tCH, tCLH, tALH)
	tWH = max_4(p->tWH, p->tCH, (int)p->tCLH, (int)p->tALH);
	tWH = (tWH * CLK) / 1000;
	// tWP = tWP
	tWP = (p->tWP * CLK) / 1000;
	// tREH = tREH
	tREH = (p->tREH * CLK) / 1000;
	// tRES = max(tREA, tRSTO, tREAID)
	tRES = max_3(p->tREA, p->tRSTO, (int)p->tREAID);
	tRES = (tRES * CLK) / 1000;
	// tRLAT < (tRES + tREH) + 2
	tRLAT = tRES + tREH;
	// t1 = max(tCS, tCLS, tALS) - tWP
	t1 = max_3(p->tCS, p->tCLS, (int)p->tALS) - p->tWP;
	if (t1 < 0)
		t1 = 0;
	else 
		t1 = (t1 * CLK) / 1000;
	// tPSTH(EBI setup time) = max(tCS, tCLS, tALS)
	tPSTH = max_3(p->tCS, p->tCLS, (int)p->tALS);
	tPSTH = (tPSTH * CLK) / 1000;
	// tWRCK(EBI hold time) = max(tRHZ, tREH)
	tWRCK = max_2(p->tRHZ, p->tREH);
	tWRCK = (tWRCK * CLK) / 1000;
		
	// tBSY = max(tWB, tRB), min value = 1
	tBSY = max_2(p->tWB, p->tRB);
	tBSY = (tBSY * CLK) / 1000;
	if(tBSY < 1)
		tBSY = 1;
	// tBUF1 = max(tADL, tCCS)
	tBUF1 = max_2(p->tADL, (int)p->tCCS);
	tBUF1 = (tBUF1 * CLK) / 1000;
	// tBUF2 = max(tAR, tRR, tCLR, tCDQSS, tCRES, tCALS, tCALS2, tDBS)
	tBUF2 = max_2(max_4(p->tAR, p->tRR, (int)p->tCLR, (int)p->tCDQSS),
				max_4((int)p->tCRES, (int)p->tCALS, (int)p->tCALS2, (int)p->tDBS));
	tBUF2 = (tBUF2 * CLK) / 1000;
	// tBUF3 = max(tRHW, tRHZ, tDQSHZ)
	tBUF3 = max_3(p->tRHW, p->tRHZ, (int)p->tDQSHZ);
	tBUF3 = (tBUF3 * CLK) / 1000;
	// tBUF4 = max(tWHR, tWHR2)
	tBUF4 = max_2((int)p->tWHR, p->tWHR2);
	tBUF4 = (tBUF4 * CLK) / 1000;
	
	timing[0] = (tWH << 24) | (tWP << 16) | (tREH << 8) | tRES;
	timing[1] = (tRLAT << 16) | (tBSY << 8) | t1;
	timing[2] = (tBUF4 << 24) | (tBUF3 << 16) | (tBUF2 << 8) | tBUF1;
	timing[3] = (tPRE << 28) | (tPST << 24) | (tPSTH << 16) | tWRCK;

	for (i = 0;i < MAX_CHANNEL;i++) {
		writel(timing[0], data->io_base + FL_AC_TIMING0(i));
		writel(timing[1], data->io_base + FL_AC_TIMING1(i));
		writel(timing[2], data->io_base + FL_AC_TIMING2(i));
		writel(timing[3], data->io_base + FL_AC_TIMING3(i));
	}	

	DBGLEVEL2(ftnandc024_dbg("AC Timing 0:0x%08x\n", timing[0]));
	DBGLEVEL2(ftnandc024_dbg("AC Timing 1:0x%08x\n", timing[1]));
	DBGLEVEL2(ftnandc024_dbg("AC Timing 2:0x%08x\n", timing[2]));
	DBGLEVEL2(ftnandc024_dbg("AC Timing 3:0x%08x\n", timing[3]));
#endif	
}

#if defined(CONFIG_NAND_V2_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
static int ftnandc024_nand_dma_rd(struct mtd_info *mtd, uint8_t *buf) {
    
    struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
    int status = 0;
    
    status = nand_dma_start(data, mtd->writesize, DMA_DEV_TO_MEM);
    if (status < 0) {
        printk("nand dma read page lp fail\n");
        goto out;
    }

    ftnandc024_dma_wait(mtd);
    
    memcpy(buf, data->mem_dmabuf, mtd->writesize);  
 out: 
    return status;
}

static int ftnandc024_nand_dma_wr(struct mtd_info *mtd, const uint8_t *buf) {

    struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
    int status = 0;

    memcpy(data->mem_dmabuf, buf, mtd->writesize);
    
    status = nand_dma_start(data, mtd->writesize, DMA_MEM_TO_DEV);
    if (status < 0) {
        printk("nand dma write page lp fail\n");
        goto out;
    }
    ftnandc024_dma_wait(mtd);
 out: 
    return status;
}
#endif

static int byte_rd(struct mtd_info *mtd, int real_pg, int col, int len, 
				u_char *spare_buf) {
	
	struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
	struct cmd_feature cmd_f = {0};
	int status, i, tmp_col, tmp_len, cmd_len, ret;
	u_char *buf;

	ret = 0;
	tmp_col = col;
	tmp_len = len;

	if(data->flash_type == TOGGLE1 || data->flash_type == TOGGLE2 ||
		data->flash_type == ONFI2 || data->flash_type == ONFI3) {
		if(col & 0x1) {
			tmp_len += 2;
			tmp_col --;
		}
		else if(tmp_len & 0x1) {
			tmp_len ++;
		}
	}
	
	buf = (u_char *)vmalloc(tmp_len);
	
	for(i = 0; i < tmp_len; i += 32) {	
		if(tmp_len - i >= 32)
			cmd_len = 32;
		else
			cmd_len = tmp_len - i;
			
	    cmd_f.row_cycle = data->sys_hdr->nandfixup.nand_row_cycle - 1;
	    cmd_f.col_cycle = data->sys_hdr->nandfixup.nand_col_cycle - 1;
		cmd_f.cq1 = real_pg;
		cmd_f.cq2 = 0;
		cmd_f.cq3 = CMD_COUNT(1) | (tmp_col & 0xFF);
		cmd_f.cq4 = CMD_COMPLETE_EN | CMD_BYTE_MODE |\
				CMD_FLASH_TYPE(data->flash_type) |\
				CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(cmd_len) |\
				CMD_INDEX(LARGE_FIXFLOW_BYTEREAD);
		#ifdef CONFIG_NAND_V2_USE_AHBDMA
		cmd_f.cq4 |= CMD_DMA_HANDSHAKE_EN;
		#endif
	
		status = ftnandc024_issue_cmd(mtd, &cmd_f);
		if(status < 0) {
			ret = 1;
			break;
		}
		ftnandc024_nand_wait(mtd, chip);
		memcpy(buf + i, data->io_base + SPARE_SRAM + (data->cur_chan << 5), cmd_len);
		
		tmp_col += cmd_len;
	}

	if(data->flash_type == TOGGLE1 || data->flash_type == TOGGLE2 ||
		data->flash_type == ONFI2 || data->flash_type == ONFI3) {
		if(col & 0x1)
			memcpy(spare_buf, buf + 1, len);
		else
			memcpy(spare_buf, buf, len);
	}
	else
		memcpy(spare_buf, buf, len);
	
	vfree(buf);
	return ret;
}

static int rd_pg_w_oob(struct mtd_info *mtd, int real_pg, 
					u_char *data_buf, u_char *spare_buf) {
	struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
	struct cmd_feature cmd_f;
	int status;
	#ifndef CONFIG_NAND_V2_USE_AHBDMA
	int i;
	u32 *lbuf;
	#endif

	cmd_f.row_cycle = data->sys_hdr->nandfixup.nand_row_cycle - 1;
	cmd_f.col_cycle = data->sys_hdr->nandfixup.nand_col_cycle - 1;
	cmd_f.cq1 = real_pg;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = CMD_COUNT(mtd->writesize >> eccbasft) | (data->column & 0xFF);
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type) |\
			CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(spare) |\
			CMD_INDEX(LARGE_FIXFLOW_PAGEREAD_W_SPARE);
	#ifdef CONFIG_NAND_V2_USE_AHBDMA
	cmd_f.cq4 |= CMD_DMA_HANDSHAKE_EN;
	#endif
	
	status = ftnandc024_issue_cmd(mtd, &cmd_f);
	if(status < 0)
		return 1;

	#ifdef CONFIG_NAND_V2_USE_AHBDMA
	if(ftnandc024_nand_dma_rd(mtd, data_buf)) {
		return 1;
	}
	#else
	lbuf = (u32 *) data_buf;
	for (i = 0; i < mtd->writesize; i += 4)
		*lbuf++ = *(volatile unsigned *)(data->chip.IO_ADDR_R);
	#endif
	ftnandc024_nand_wait(mtd, chip);
	memcpy(spare_buf, data->io_base + SPARE_SRAM + (data->cur_chan << 5), mtd->oobsize); 

	return 0;

}

static int rd_pg_w_oob_sp(struct mtd_info *mtd, int real_pg, 
					u_char *data_buf, u_char *spare_buf) {
	struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
	struct cmd_feature cmd_f;
	int status;
	#ifndef CONFIG_NAND_V2_USE_AHBDMA
	u32 *lbuf;
	int i;
	#endif

	cmd_f.row_cycle = ROW_ADDR_2CYCLE;
	cmd_f.col_cycle = COL_ADDR_1CYCLE;
	cmd_f.cq1 = real_pg;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = CMD_COUNT(1) | (data->column & 0xFF);
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type) |\
			CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(spare) |\
			CMD_INDEX(SMALL_FIXFLOW_PAGEREAD);
	#ifdef CONFIG_NAND_V2_USE_AHBDMA
	cmd_f.cq4 |= CMD_DMA_HANDSHAKE_EN;
	#endif

	status = ftnandc024_issue_cmd(mtd, &cmd_f);
	if (status < 0)
		return 1;

	#ifdef CONFIG_NAND_V2_USE_AHBDMA
	if(ftnandc024_nand_dma_rd(mtd, data_buf)) 
		return 1;
	
	#else
	lbuf = (u32 *)data_buf;
	for (i = 0; i < mtd->writesize; i += 4)
		*lbuf++ = *(volatile unsigned *)(data->chip.IO_ADDR_R);
	#endif
	ftnandc024_nand_wait(mtd, chip);
	
	memcpy(spare_buf, data->io_base + SPARE_SRAM + (data->cur_chan << 5), mtd->oobsize);
	
	return 0;
}

static int rd_oob(struct mtd_info *mtd, int real_pg, u_char *spare_buf) {
	struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
	struct cmd_feature cmd_f;
	int status;

	cmd_f.row_cycle = data->sys_hdr->nandfixup.nand_row_cycle - 1;
	cmd_f.col_cycle = data->sys_hdr->nandfixup.nand_col_cycle - 1;
	cmd_f.cq1 = real_pg;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = CMD_COUNT(1);
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type) |\
			CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(spare) |\
			CMD_INDEX(LARGE_FIXFLOW_READOOB);

	status = ftnandc024_issue_cmd(mtd, &cmd_f);
	if (status < 0)
		return 1;

	ftnandc024_nand_wait(mtd, chip);
	memcpy(spare_buf, data->io_base + SPARE_SRAM + (data->cur_chan << 5), mtd->oobsize);
	return 0;
}

static int rd_oob_sp(struct mtd_info *mtd, int real_pg, u_char *spare_buf) {
	struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
	struct cmd_feature cmd_f;
	int status;
	
	cmd_f.row_cycle = ROW_ADDR_2CYCLE;
	cmd_f.col_cycle = COL_ADDR_1CYCLE;
	cmd_f.cq1 = real_pg;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = CMD_COUNT(1);
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type) |\
			CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(spare) |\
			CMD_INDEX(SMALL_FIXFLOW_READOOB);

	status = ftnandc024_issue_cmd(mtd, &cmd_f);
	if (status < 0)
		return 1;
	
	ftnandc024_nand_wait(mtd, chip);
	
	memcpy(spare_buf, data->io_base + SPARE_SRAM + (data->cur_chan << 5), mtd->oobsize);

	return 0;
}

static int rd_pg(struct mtd_info *mtd, int real_pg, u_char *data_buf) {
	struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
	struct cmd_feature cmd_f;
	int status;	
	#ifndef CONFIG_NAND_V2_USE_AHBDMA
	u32 *lbuf;
	int i;
	#endif

	cmd_f.row_cycle = data->sys_hdr->nandfixup.nand_row_cycle - 1;
	cmd_f.col_cycle = data->sys_hdr->nandfixup.nand_col_cycle - 1;
	cmd_f.cq1 = real_pg;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = CMD_COUNT(mtd->writesize >> eccbasft) | (data->column & 0xFF);
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type) |\
			CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(spare) |\
			CMD_INDEX(LARGE_FIXFLOW_PAGEREAD);
	#ifdef CONFIG_NAND_V2_USE_AHBDMA
	cmd_f.cq4 |= CMD_DMA_HANDSHAKE_EN;
	#endif
	
	status = ftnandc024_issue_cmd(mtd, &cmd_f);
	if(status < 0)
		return 1;
	#ifdef CONFIG_NAND_V2_USE_AHBDMA
	if(ftnandc024_nand_dma_rd(mtd, data_buf)) {
		return 1;
	}
	#else
	lbuf = (u32 *) data_buf;
	for (i = 0; i < mtd->writesize; i += 4)
		*lbuf++ = *(volatile unsigned *)(data->chip.IO_ADDR_R);
	#endif
	ftnandc024_nand_wait(mtd, chip);

	return 0;
}

static int rd_pg_sp(struct mtd_info *mtd, int real_pg, u_char *data_buf) {
	struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
	struct cmd_feature cmd_f;
	int progflow_buf[3];
	int status;
	#ifndef CONFIG_NAND_V2_USE_AHBDMA
	int i;
	u32 *lbuf;
	#endif

	cmd_f.row_cycle = ROW_ADDR_2CYCLE;
	cmd_f.col_cycle = COL_ADDR_1CYCLE;
	cmd_f.cq1 = real_pg;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = CMD_COUNT(1) | (data->column & 0xFF);
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type) |\
			CMD_START_CE(data->sel_chip);
	#ifdef CONFIG_NAND_V2_USE_AHBDMA
	cmd_f.cq4 |= CMD_DMA_HANDSHAKE_EN;
	#endif

	progflow_buf[0] = 0x66414200;
	progflow_buf[1] = 0x66626561;
	progflow_buf[2] = 0x000067C8;
	ftnandc024_fill_prog_flow(mtd, progflow_buf, 10);
	cmd_f.cq4 |= CMD_PROM_FLOW | CMD_INDEX(0x0);

	status = ftnandc024_issue_cmd(mtd, &cmd_f);
	if (status < 0)
		return 1;

	#ifdef CONFIG_NAND_V2_USE_AHBDMA
	if(ftnandc024_nand_dma_rd(mtd, data_buf)) 
		return 1;
	
	#else
	lbuf = (u32 *)data_buf;
	for (i = 0; i < mtd->writesize; i += 4)
		*lbuf++ = *(volatile unsigned *)(data->chip.IO_ADDR_R);
	#endif
	ftnandc024_nand_wait(mtd, chip);
		
	return 0;
}

static void ftnandc024_onfi_set_feature(struct mtd_info *mtd, int chn, int ce, int val)
{
	struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
	struct cmd_feature cmd_f;

    platform_select_pinmux(nand_fd, 0);
    write_protect(0);
    
	/* 0x11 means Timing mode 1 and Synchronous DDR */
	writel(val, data->io_base + SPARE_SRAM + (chn << 5));

	/* 0x1 is Timing mode feature address */
	cmd_f.row_cycle = ROW_ADDR_2CYCLE;
	cmd_f.col_cycle = COL_ADDR_1CYCLE;
	cmd_f.cq1 = 0x1;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = 0;
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(LEGACY_FLASH) |\
			CMD_START_CE(ce) | CMD_BYTE_MODE | CMD_SPARE_NUM(4) |\
			CMD_INDEX(ONFI_FIXFLOW_SETFEATURE);

	ftnandc024_issue_cmd(mtd, &cmd_f);

	ftnandc024_nand_wait(mtd, chip);

    platform_select_pinmux(nand_fd, -1);
}

static void ftnandc024_read_raw_id(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
	struct cmd_feature cmd_f;
	u8 id_size = 5;

	data->cur_chan = 0;
	data->sel_chip = 0;

	// Set the flash to Legacy mode, in advance.
	if(data->flash_type == ONFI2 || data->flash_type == ONFI3) {
		ftnandc024_onfi_set_feature(mtd, data->cur_chan, data->sel_chip, 0x00);
	}

	// Issue the RESET cmd
	cmd_f.cq1 = 0;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = 0;
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(LEGACY_FLASH) |\
			CMD_START_CE(data->sel_chip) | CMD_INDEX(FIXFLOW_RESET);

	ftnandc024_issue_cmd(mtd, &cmd_f);
	
	ftnandc024_nand_wait(mtd, chip);

	// Issue the READID cmd
	cmd_f.row_cycle = ROW_ADDR_1CYCLE;
	cmd_f.col_cycle = COL_ADDR_1CYCLE;
	cmd_f.cq1 = 0;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = CMD_COUNT(1);
	cmd_f.cq4 = CMD_FLASH_TYPE(LEGACY_FLASH) | CMD_COMPLETE_EN |\
			CMD_INDEX(FIXFLOW_READID) | CMD_START_CE(data->sel_chip) |\
			CMD_BYTE_MODE | CMD_SPARE_NUM(id_size);
	
	ftnandc024_issue_cmd(mtd, &cmd_f);	

	ftnandc024_nand_wait(mtd, chip);
	
	memcpy(data->flash_raw_id, data->io_base + SPARE_SRAM + (data->cur_chan << 5) , id_size);

	DBGLEVEL2(ftnandc024_dbg("ID@(ch:%d, ce:%d):0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", 
					data->cur_chan, data->sel_chip, data->flash_raw_id[0], 
					data->flash_raw_id[1], data->flash_raw_id[2], 
					data->flash_raw_id[3], data->flash_raw_id[4]));
}

static int ftnandc024_check_bad_spare(struct mtd_info *mtd, int pg){
	struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
	int spare_phy_start, spare_phy_len, eccbyte;
	int errbit_num, i, j, ret;
	int sec_num = (mtd->writesize >> eccbasft);
	u_char *spare_buf;
	
	eccbyte = (useecc * 14) / 8;
	if (((useecc * 14) % 8) != 0)
		eccbyte++;

	spare_phy_start = mtd->writesize + (eccbyte * sec_num);

	eccbyte = (useecc_spare * 14) / 8;
	if (((useecc_spare * 14) % 8) != 0)
		eccbyte++;

	spare_phy_len = spare + eccbyte;
	spare_buf = vmalloc(spare_phy_len);
	
	ret = 0;
	errbit_num = 0;

	if(!byte_rd(mtd, pg, spare_phy_start, spare_phy_len, spare_buf)) {
		
		for(i = 0; i < spare_phy_len; i++) {
			if(*(spare_buf + i) != 0xFF) {
				for(j = 0; j < 8; j ++) {
					if((*(spare_buf + i) & (0x1 << j)) == 0)
						errbit_num ++;
				}
			}
		}
		
		if(errbit_num > readl(data->io_base + ECC_CORRECT_BIT_FOR_SPARE_REG1) + 1)
			ret = 1;
		else
			ret = 0;
	}
	else
		ret = 1;

	vfree(spare_buf);
	return ret;

}

static void ftnandc024_calibrate_dqs_delay(struct mtd_info *mtd, int type)
{
	struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
	struct cmd_feature cmd_f;
	int i, max_dqs_delay = 0;
	int id_size = 5;
	int id_size_ddr = (id_size << 1);
	u8 *p, *golden_p;
	u8 dqs_lower_bound, dqs_upper_bound, state;
	u32 val;

	dqs_lower_bound = dqs_upper_bound = 0;
	p = kmalloc(id_size, GFP_KERNEL);
	golden_p = kmalloc(id_size, GFP_KERNEL);
	 	
	if(type == ONFI2) {
		/* Extent the data from SDR to DDR. 
		   Ex. If "0xaa, 0xbb, 0xcc, 0xdd, 0xee" is in SDR, 
		          "0xaa, 0xaa, 0xbb, 0xbb, 0xcc, 0xcc, 0xdd, 0xdd, 0xee, 0xee" is in DDR(ONFI).
		*/
		for(i = 0; i< id_size; i++) {
			*(golden_p + (i << 1) + 0) = *(data->flash_raw_id + i);
			*(golden_p + (i << 1) + 1) = *(data->flash_raw_id + i);
		}
		DBGLEVEL2(ftnandc024_dbg("Golden ID:0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", 
					*golden_p, *(golden_p+1), *(golden_p+2), 
					*(golden_p+3), *(golden_p+4), *(golden_p+5)));
		max_dqs_delay = 20;
	}
	else if(type == TOGGLE1 || TOGGLE2) {
		/* Extent the data from SDR to DDR. 
		   Ex. If "0xaa, 0xbb, 0xcc, 0xdd, 0xee" is in SDR, 
		          "0xaa, 0xbb, 0xbb, 0xcc, 0xcc, 0xdd, 0xdd, 0xee, 0xee" is in DDR(TOGGLE).
		*/
		for(i = 0; i< id_size; i++) {
			*(golden_p + (i << 1) + 0) = *(data->flash_raw_id + i);
			*(golden_p + (i << 1) + 1) = *(data->flash_raw_id + i);
		}
		golden_p ++;
		
		DBGLEVEL2(ftnandc024_dbg("Golden ID:0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", 
					*golden_p, *(golden_p+1), *(golden_p+2), 
					*(golden_p+3), *(golden_p+4), *(golden_p+5)));
		max_dqs_delay = 18;
	}
	else {
		printk("%s:Type:%d isn't allowed\n", __func__, type);
		goto out;
	}	
		

	state = 0;
	for(i = 0; i <= max_dqs_delay; i++) {
		// setting the dqs delay before READID.
		writel(i, data->io_base + DQS_DELAY);
		memset(p, 0, id_size_ddr);
		
		// Issuing the READID
		cmd_f.row_cycle = ROW_ADDR_1CYCLE;
		cmd_f.col_cycle = COL_ADDR_1CYCLE;
		cmd_f.cq1 = 0;
		cmd_f.cq2 = 0;
		cmd_f.cq3 = CMD_COUNT(1);
		cmd_f.cq4 = CMD_FLASH_TYPE(type) | CMD_COMPLETE_EN |\
				CMD_INDEX(FIXFLOW_READID) | CMD_BYTE_MODE |\
				CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(id_size_ddr);
		
		ftnandc024_issue_cmd(mtd, &cmd_f);
		
		ftnandc024_nand_wait(mtd, chip);
		
			
		if(type == ONFI2) {
			memcpy(p, data->io_base + SPARE_SRAM + (data->cur_chan<< 5), id_size_ddr);
			if(state == 0 && memcmp(golden_p, p, id_size_ddr) == 0) {
				dqs_lower_bound = i;
				state = 1;
			}
			else if(state == 1 && memcmp(golden_p, p, id_size_ddr) != 0){
				dqs_upper_bound = i - 1;
				break;
			}
		}
		else if(type == TOGGLE1 || type == TOGGLE2) {
			memcpy(p, data->io_base + SPARE_SRAM + (data->cur_chan<< 5), id_size_ddr-1);
			
			if(state == 0 && memcmp(golden_p, p, (id_size_ddr - 1)) == 0) {
				dqs_lower_bound = i;
				state = 1;
			}
			else if(state == 1 && memcmp(golden_p, p, (id_size_ddr - 1)) != 0){
				dqs_upper_bound = (i - 1);
				break;
			}

		}
		DBGLEVEL2(ftnandc024_dbg("===============================================\n"));
		DBGLEVEL2(ftnandc024_dbg("ID       :0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", 
							*p, *(p+1), *(p+2), *(p+3), 
							*(p+4), *(p+5), *(p+6), *(p+7),
							*(p+8) ));
		DBGLEVEL2(ftnandc024_dbg("Golden ID:0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", 
							*golden_p, *(golden_p+1), *(golden_p+2), *(golden_p+3), 
							*(golden_p+4), *(golden_p+5),*(golden_p+6), *(golden_p+7), 
							*(golden_p+8) ));
		DBGLEVEL2(ftnandc024_dbg("===============================================\n"));	
	}
	// Prevent the dqs_upper_bound is zero when ID still accuracy on the max dqs delay
	if(i == max_dqs_delay + 1)
		dqs_upper_bound = max_dqs_delay;
		
	printk("Upper:%d & Lower:%d for DQS, then Middle:%d\n", 
		dqs_upper_bound, dqs_lower_bound, ((dqs_upper_bound + dqs_lower_bound) >> 1));
	// Setting the middle dqs delay
	val = readl(data->io_base + DQS_DELAY);
	val &= ~0x1F;
	val |= (((dqs_lower_bound + dqs_upper_bound) >> 1) & 0x1F);
	writel(val, data->io_base + DQS_DELAY);
out:
	kfree(p);
	kfree(golden_p);

}

static uint32_t ftnandc024_onfi_get_feature(struct mtd_info *mtd, int chn, int ce, int type)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc024_nand_data *data = chip->priv;
	struct cmd_feature cmd_f;
    u32 val;

    platform_select_pinmux(nand_fd, 0);

	/* 0x1 is Timing mode feature address */
	cmd_f.row_cycle = ROW_ADDR_2CYCLE;
	cmd_f.col_cycle = COL_ADDR_1CYCLE;
	cmd_f.cq1 = 0x1;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = 0;
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(type) |\
			CMD_START_CE(ce) | CMD_BYTE_MODE | CMD_SPARE_NUM(4) |\
			CMD_INDEX(ONFI_FIXFLOW_GETFEATURE);

	ftnandc024_issue_cmd(mtd, &cmd_f);

	ftnandc024_nand_wait(mtd, chip);

	val = readl(data->io_base + SPARE_SRAM + (chn << 5));

	platform_select_pinmux(nand_fd, -1);

    return val;
}

static int ftnandc024_onfi_sync(struct mtd_info *mtd)
{
    struct nand_chip *p = mtd->priv;
    struct ftnandc024_nand_data *data = p->priv;
    u32 val;
    int ret = -1;

    platform_select_pinmux(nand_fd, 0);
    
	ftnandc024_nand_select_chip(mtd, 0);
	val = ftnandc024_onfi_get_feature(mtd, data->cur_chan, data->sel_chip, 0);
	printk("onfi feature for Ch %d, CE %d: 0x%x\n", data->cur_chan, data->sel_chip, val);

	ftnandc024_onfi_set_feature(mtd, data->cur_chan, data->sel_chip, 0x11);
	
	val = ftnandc024_onfi_get_feature(mtd, data->cur_chan, data->sel_chip, ONFI2);
	printk("onfi feature for Ch %d, CE %d: 0x%x\n", data->cur_chan, data->sel_chip, val);
	if (val != 0x1111) {
		goto out_onfi_sync;
	}
	ret = 0;

  out_onfi_sync:
  	platform_select_pinmux(nand_fd, -1);
    return ret;
}

static int ftnandc024_available_oob(struct mtd_info *mtd)
{
	int ret = 0;
	int consume_byte, eccbyte, eccbyte_spare;
	int available_spare;
	int sector_num = (mtd->writesize >> eccbasft);
	
	if (useecc < 0)
		goto out;
	if (protect_spare != 0)
		protect_spare = 1;
	else
		protect_spare = 0;

	eccbyte = (useecc * 14) / 8;
	if (((useecc * 14) % 8) != 0)
		eccbyte++;

	consume_byte = (eccbyte * sector_num);
	if (protect_spare == 1) { 
		
		eccbyte_spare = (useecc_spare * 14) / 8;
		if (((useecc_spare * 14) % 8) != 0)
			eccbyte_spare++;
		consume_byte += eccbyte_spare;
	}
	available_spare = spare - consume_byte;	

	DBGLEVEL2(ftnandc024_dbg(
		"mtd->erasesize:%d, mtd->writesize:%d, block_boundary:%d", 
		mtd->erasesize, mtd->writesize, block_boundary));
	DBGLEVEL2(ftnandc024_dbg(
		"page num:%d, eccbasft:%d, protect_spare:%d, spare:%d", 
		 mtd->erasesize/mtd->writesize,eccbasft, protect_spare, spare));
	DBGLEVEL2(ftnandc024_dbg(
		"consume_byte:%d, eccbyte:%d, eccbytes(spare):%d, useecc:%d",
		consume_byte, eccbyte, eccbyte_spare, useecc));
	
	/*----------------------------------------------------------
	 * YAFFS require 16 bytes OOB without ECC, 28 bytes with
	 * ECC enable.
	 * BBT require 5 bytes for Bad Block Table marker.
	 */
	if (available_spare >= 4) {
		if(available_spare >= 32) {
			ret = 32;
		}
		else if(available_spare >= 16) {
			ret = 16;
		}
		else if(available_spare >= 8) {
			ret = 8;
		}
		else if(available_spare >= 4) {
			ret = 4;
		}
		printk(KERN_INFO "Available OOB is %d byte, but we use %d bytes in page mode.\n", available_spare, ret);
	} else {
		printk(KERN_INFO "Not enough OOB, try to reduce ECC correction bits.\n");
		printk(KERN_INFO "(Currently ECC setting for Data:%d)\n", useecc);
		printk(KERN_INFO "(Currently ECC setting for Spare:%d)\n", useecc_spare);
	}
  out:
    return ret;
}

static uint8_t ftnandc024_read_byte(struct mtd_info *mtd)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc024_nand_data *data = chip->priv;
    uint32_t lv;
    uint8_t b = 0;

    platform_select_pinmux(nand_fd, 0);

    switch (data->cur_cmd) {
    case NAND_CMD_READID:
		lv = readl(data->io_base + SPARE_SRAM + (data->cur_chan << 5));
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

static int ftnandc024_nand_read_oob_lp(struct mtd_info *mtd, u_char * buf)
{
	struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
	int status = 0, i, ecc_original_setting, generic_original_setting, val;
	int real_pg, empty;
	
	real_pg = data->page_addr;

	DBGLEVEL2(ftnandc024_dbg(
		"read_oob: ch = %d, ce = %d, page = 0x%x, real: 0x%x, spare = %d\n", 
		data->cur_chan, data->sel_chip, data->page_addr, real_pg, spare));

    platform_select_pinmux(nand_fd, 0);

	if(!rd_oob(mtd, real_pg, chip->oob_poi)) {
	    //check if this page is erased and first time to use it, let's ecc error
		if(data->cmd_status & CMD_ECC_FAIL_ON_SPARE) {
			// Store the original setting
			ecc_original_setting = readl(data->io_base + ECC_CONTROL); 
			generic_original_setting = readl(data->io_base + GENERAL_SETTING);
			// Disable the ECC engine & HW-Scramble, temporarily.
			val = readl(data->io_base + ECC_CONTROL);
			val = val & ~(ECC_EN(0xFF));
			writel(val, data->io_base + ECC_CONTROL);
			val = readl(data->io_base + GENERAL_SETTING);
			val &= ~DATA_SCRAMBLER;
			writel(val, data->io_base + GENERAL_SETTING);
			
			if(!rd_oob(mtd, real_pg, chip->oob_poi)) {	
				empty = 1;
				for (i = 0; i < mtd->oobsize; i++) {
					if (*(buf + i) != 0xFF) {
						printk(KERN_ERR "ECC err for spare(Read oob) @");//???
						printk(KERN_ERR	"ch:%d ce:%d page0x%x real:0x%x\n",
							data->cur_chan, data->sel_chip, data->page_addr, real_pg);
						mtd->ecc_stats.failed++;
						status = -1;
						empty = 0;
						break;
					}
				}
				if (empty == 1)
					DBGLEVEL2(ftnandc024_dbg("Spare real 0xFF"));
			}			
			// Restore the ecc original setting & generic original setting.
			writel(ecc_original_setting, data->io_base + ECC_CONTROL);
			writel(generic_original_setting, data->io_base + GENERAL_SETTING);
		}
	}
  	platform_select_pinmux(nand_fd, -1);
	// Returning the any value isn't allowed, except 0, -EBADMSG, or -EUCLEAN
	return 0;
}

static int ftnandc024_nand_read_oob_sp(struct mtd_info *mtd, u_char * buf)
{
	struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
	int i, status = 0;
	int val, ecc_original_setting, empty;

	DBGLEVEL2(ftnandc024_dbg("smallread_oob: ch = %d, ce = %d, page = 0x%x, size = %d\n", 
				data->cur_chan, data->sel_chip, data->page_addr, mtd->writesize));

    platform_select_pinmux(nand_fd, 0);

	if(!rd_oob_sp(mtd, data->page_addr, buf)) {
		if(data->cmd_status & CMD_ECC_FAIL_ON_SPARE) {
			// Store the original setting
			ecc_original_setting = readl(data->io_base + ECC_CONTROL); 
			// Disable the ECC engine & HW-Scramble, temporarily.
			val = readl(data->io_base + ECC_CONTROL);
			val = val & ~(ECC_EN(0xFF));
			writel(val, data->io_base + ECC_CONTROL);
			
			if(!rd_oob_sp(mtd, data->page_addr, buf)) {
				empty = 1;
				for (i = 0; i < mtd->oobsize; i++) {             
					if (*(buf + i) != 0xFF) {                
						printk(KERN_ERR "ECC err for spare(Read oob) @ page0x%x\n",
								data->page_addr);
						mtd->ecc_stats.failed++;
						status = -1;
						empty = 0;
						break;           
					}                      
				}
				if(empty == 1)	
					DBGLEVEL2(ftnandc024_dbg("Spare real 0xFF"));
			}
			// Restore the ecc original setting & generic original setting.
			writel(ecc_original_setting, data->io_base + ECC_CONTROL);
		}
	}
	
  	platform_select_pinmux(nand_fd, -1);
	// Returning the any value isn't allowed, except 0, -EBADMSG, or -EUCLEAN
	return 0;
}

static int ftnandc024_nand_read_oob_std(struct mtd_info *mtd, struct nand_chip *chip, int page,
                                        int sndcmd)
{
    struct ftnandc024_nand_data *data = chip->priv;

    data->page_addr = page;

    return data->read_oob(mtd, chip->oob_poi);
}
#if defined(CONFIG_NAND_V2_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)

static int setup_dma(struct ftnandc024_nand_data *data, int direct)
{
    struct dma_slave_config *common;
    
    data->dma_slave_config.id = -1;
    data->dma_slave_config.handshake = DMA_NANDC_REQ;//enable

    common = &data->dma_slave_config.common;
    
#ifdef CONFIG_NAND_V2_USE_AHBDMA    
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

static int nand_dma_start(struct ftnandc024_nand_data *data, size_t len, int direct)
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

	desc->callback = ftnandc024_dma_callback;
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
        start_addr = ftnandc024_partition_info[root_mtd_num].offset;
        end_addr = ftnandc024_partition_info[root_mtd_num].size + start_addr;
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

static int ftnandc024_nand_read_page_lp(struct mtd_info *mtd, u_char * buf)
{
	struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
	int status = 0, chk_data_0xFF, chk_spare_0xFF;
	int i, ecc_original_setting, generic_original_setting, val;
	int real_pg;
	u8  empty;
	u32 *lbuf;
	
	real_pg = data->page_addr;

	DBGLEVEL2(ftnandc024_dbg
		("r: ch = %d, ce = %d, page = 0x%x, real = 0x%x, size = %d, spare = %d\n", 
		data->cur_chan, data->sel_chip, data->page_addr, real_pg, mtd->writesize, spare));
    platform_select_pinmux(nand_fd, 0);

    real_pg = data->page_addr = calc_new_page(mtd, data->page_addr);

	chk_data_0xFF = chk_spare_0xFF = 0;

	if(!rd_pg_w_oob(mtd, real_pg, buf, chip->oob_poi)) {
	    //check if this page is erased and first time to use it, let's ecc error
		if (data->cmd_status & 
			(CMD_ECC_FAIL_ON_DATA | CMD_ECC_FAIL_ON_SPARE)) {
			// Store the original setting
			ecc_original_setting = readl(data->io_base + ECC_CONTROL); 
			generic_original_setting = readl(data->io_base + GENERAL_SETTING);
			// Disable the ECC engine & HW-Scramble, temporarily.
			val = readl(data->io_base + ECC_CONTROL);
			val = val & ~(ECC_EN(0xFF));
			writel(val, data->io_base + ECC_CONTROL);
			val = readl(data->io_base + GENERAL_SETTING);
			val &= ~DATA_SCRAMBLER;
			writel(val, data->io_base + GENERAL_SETTING);

			if( data->cmd_status == (CMD_ECC_FAIL_ON_DATA | CMD_ECC_FAIL_ON_SPARE) ) {
				if(!rd_pg_w_oob(mtd, real_pg, buf, chip->oob_poi)) {
					chk_data_0xFF = chk_spare_0xFF = 1;
				}	
			}
			else if(data->cmd_status == CMD_ECC_FAIL_ON_DATA) {
				if(!rd_pg(mtd, real_pg, buf)) {
					chk_data_0xFF = 1;
				}	
			}
			else if(data->cmd_status == CMD_ECC_FAIL_ON_SPARE) {
				if(!rd_oob(mtd, real_pg, chip->oob_poi)) {
					chk_spare_0xFF = 1;
				}	
			}
		
			// Restore the ecc original setting & generic original setting.
			writel(ecc_original_setting, data->io_base + ECC_CONTROL);
			writel(generic_original_setting, data->io_base + GENERAL_SETTING);

			if(chk_data_0xFF == 1) {
			    data->cmd_status &= ~CMD_ECC_FAIL_ON_DATA;
				empty = 1;
				lbuf = (int *)buf;
				for (i = 0; i < (mtd->writesize >> 2); i++) {
					if (*(lbuf + i) != 0xFFFFFFFF) {
						printk(KERN_ERR "ECC err @ page0x%x real:0x%x\n", 
							data->page_addr, real_pg);
						mtd->ecc_stats.failed++;						
						status = -1;
						empty = 0;
						break;	
					}
				}
				if(empty == 1)
					DBGLEVEL2(ftnandc024_dbg("Data Real 0xFF\n"));
			}
			if(chk_spare_0xFF == 1) {
			    data->cmd_status &= ~CMD_ECC_FAIL_ON_SPARE;
				if(!ftnandc024_check_bad_spare(mtd, data->page_addr))
					empty = 1;
				else {				    
					printk(KERN_ERR "ECC err for spare(Read page) @");
					printk(KERN_ERR	"ch:%d ce:%d page0x%x real:0x%x\n",
						data->cur_chan, data->sel_chip, data->page_addr, real_pg);
					mtd->ecc_stats.failed++;					
					status = -1;
					empty = 0;
				}
				if(empty == 1)
					DBGLEVEL2(ftnandc024_dbg("Spare Real 0xFF\n"));
			}	
		}
	}

  	platform_select_pinmux(nand_fd, -1);		
	// Returning the any value isn't allowed, except 0, -EBADMSG, or -EUCLEAN
	return 0;	
}

static int ftnandc024_nand_read_page_sp(struct mtd_info *mtd, u_char * buf)
{
	struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
	int status = 0;
	int i, ecc_original_setting, val;
	int chk_data_0xFF, chk_spare_0xFF, empty;
	u8 *p;
	u32 *lbuf;

	DBGLEVEL2(ftnandc024_dbg("smallr: ch = %d, ce = %d, page = 0x%x, size = %d, data->column = %d\n", 
			data->cur_chan, data->sel_chip, data->page_addr, mtd->writesize, data->column));

    platform_select_pinmux(nand_fd, 0);
    
	chk_data_0xFF = chk_spare_0xFF = 0;
	if(!rd_pg_w_oob_sp(mtd, data->page_addr, buf, chip->oob_poi)) {
		if(data->cmd_status & (CMD_ECC_FAIL_ON_DATA | CMD_ECC_FAIL_ON_SPARE)) {
			// Store the original setting
			ecc_original_setting = readl(data->io_base + ECC_CONTROL); 
			// Disable the ECC engine & HW-Scramble, temporarily.
			val = readl(data->io_base + ECC_CONTROL);
			val = val & ~(ECC_EN(0xFF));
			writel(val, data->io_base + ECC_CONTROL);
			
			if(data->cmd_status == (CMD_ECC_FAIL_ON_DATA | CMD_ECC_FAIL_ON_SPARE)) {
				if(!rd_pg_w_oob_sp(mtd, data->page_addr, buf, chip->oob_poi)) {
					chk_data_0xFF = chk_spare_0xFF = 1;
				}	
			}
			else if(data->cmd_status == CMD_ECC_FAIL_ON_DATA) {
				if(!rd_pg_sp(mtd, data->page_addr, buf)) {
					chk_data_0xFF = 1;
				}
			}
			else if(data->cmd_status == CMD_ECC_FAIL_ON_SPARE) {
				if(!rd_oob_sp(mtd, data->page_addr, chip->oob_poi)) {
					chk_spare_0xFF = 1;		
				}
			}
			// Restore the ecc original setting & generic original setting.
			writel(ecc_original_setting, data->io_base + ECC_CONTROL);
			
			if(chk_data_0xFF == 1) {
				lbuf = (u32 *)buf;
				empty = 1;
				for (i = 0; i < (mtd->writesize >> 2); i++) {
					if (*(lbuf + i) != 0xFFFFFFFF) {
						printk(KERN_ERR "ECC err @ page0x%x\n", data->page_addr); 
						ftnandc024_regdump(mtd);
						mtd->ecc_stats.failed++;
						status = -1;
						empty = 0;
						break;
					}
				}
				if (empty == 1)
					DBGLEVEL2(ftnandc024_dbg("Data Real 0xFF\n"));
			}
			if(chk_spare_0xFF == 1) {
				p = chip->oob_poi;
				empty = 1;
				for (i = 0; i < mtd->oobsize; i++) {
					if (*(p + i) != 0xFF) {
						printk(KERN_ERR"ECC err for spare(Read page) @ page0x%x\n", data->page_addr);
						mtd->ecc_stats.failed++;
						status = -1;
						empty = 0;
						break;
					}
				}
				if (empty == 1)
					DBGLEVEL2(ftnandc024_dbg("Spare Real 0xFF\n"));
			}
		}
	}
    platform_select_pinmux(nand_fd, -1);
	// Returning the any value isn't allowed, except 0, -EBADMSG, or -EUCLEAN
	return 0;
}

static int ftnandc024_nand_read_page(struct mtd_info *mtd, struct nand_chip *chip, uint8_t * buf, int page)
{
    struct ftnandc024_nand_data *data = chip->priv;

    data->buf = (u32 *) buf;

    return data->read_page(mtd, buf);
}

static int ftnandc024_nand_write_oob_lp(struct mtd_info *mtd, const u_char * buf, int len)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc024_nand_data *data = chip->priv;
    struct cmd_feature cmd_f;
    int status = 0, real_pg;

    real_pg = data->page_addr;

	DBGLEVEL2(ftnandc024_dbg(
		"write_oob: ch = %d, ce = %d, page = 0x%x, real page:0x%x, sz = %d, oobsz = %d\n", 
		data->cur_chan, data->sel_chip, data->page_addr, real_pg, mtd->writesize, mtd->oobsize));

    platform_select_pinmux(nand_fd, 0);
    write_protect(0);

	memcpy(data->io_base + SPARE_SRAM + (data->cur_chan << 5), buf, mtd->oobsize);

	cmd_f.row_cycle = data->sys_hdr->nandfixup.nand_row_cycle - 1;
	cmd_f.col_cycle = data->sys_hdr->nandfixup.nand_col_cycle - 1;
	cmd_f.cq1 = real_pg;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = CMD_COUNT(1);
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type) |\
			CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(spare) |\
			CMD_INDEX(LARGE_FIXFLOW_WRITEOOB);
		
	status = ftnandc024_issue_cmd(mtd, &cmd_f);
    if (status < 0) {
        printk("status error\n");
        goto out_write_oob_lp;
    }

    if (ftnandc024_nand_wait(mtd, chip) == NAND_STATUS_FAIL) {
        status = -1;
        printk("wait error\n");
        goto out_write_oob_lp;
    }
  out_write_oob_lp:
    platform_select_pinmux(nand_fd, -1);
    return status;
}

static int ftnandc024_nand_write_oob_sp(struct mtd_info *mtd, const u_char * buf, int len)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc024_nand_data *data = chip->priv;
    struct cmd_feature cmd_f;
    int status = 0;

    platform_select_pinmux(nand_fd, 0);
    
	DBGLEVEL2(ftnandc024_dbg("smallwrite_oob: ch = %d, ce = %d, page = 0x%x, size = %d\n", 
				data->cur_chan, data->sel_chip, data->page_addr, mtd->writesize));
	
	memcpy(data->io_base + SPARE_SRAM + (data->cur_chan << 5), buf, len);

	cmd_f.row_cycle = ROW_ADDR_2CYCLE;
	cmd_f.col_cycle = COL_ADDR_1CYCLE;
	cmd_f.cq1 = data->page_addr;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = CMD_COUNT(1);
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type) |\
			CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(spare) |\
			CMD_INDEX(SMALL_FIXFLOW_WRITEOOB);

	status = ftnandc024_issue_cmd(mtd, &cmd_f);
	if (status < 0)
		goto out_write_oob_sp;

    if (ftnandc024_nand_wait(mtd, chip) == NAND_STATUS_FAIL) {
        status = -1;
        goto out_write_oob_sp;
    }
  out_write_oob_sp:
    platform_select_pinmux(nand_fd, -1);
    return status;
}

static int ftnandc024_nand_write_oob_std(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
    struct ftnandc024_nand_data *data = chip->priv;

    DBGLEVEL2(ftnandc024_dbg("write oob only to page = 0x%x\n", page));
    data->page_addr = page;

    return data->write_oob(mtd, chip->oob_poi, mtd->oobsize);
}

static int ftnandc024_nand_write_page_lp(struct mtd_info *mtd, const uint8_t * buf)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc024_nand_data *data = chip->priv;
    struct cmd_feature cmd_f;
#ifndef CONFIG_NAND_V2_USE_AHBDMA
    int i;
    u32 *lbuf;
#endif
    int status = 0;
    int real_pg;
    u8 *p, w_wo_spare = 1;

    DBGLEVEL2(ftnandc024_dbg (
              "w:page = 0x%x, size = %d, data->column = %d, spare = %d\n", data->page_addr, mtd->writesize,
               data->column, spare));

    platform_select_pinmux(nand_fd, 0);
    write_protect(0);
    
    real_pg = data->page_addr;
    p = chip->oob_poi;
#if 1//always set LARGE_PAGEWRITE_W_SPARE command, need assign data into SRAM ???
    w_wo_spare = 0;
#else
    if (p != NULL) {
    	for(i = 0; i < mtd->oobsize; i++) {
    		if( *( p + i) != 0xFF) {
    			//w_wo_spare = 0;
    			printk("w spare %x\n",*( p + i));
    			break;
    		}
    	}
    }
#endif

	cmd_f.row_cycle = data->sys_hdr->nandfixup.nand_row_cycle - 1;
	cmd_f.col_cycle = data->sys_hdr->nandfixup.nand_col_cycle - 1;
	cmd_f.cq1 = real_pg;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = CMD_COUNT(mtd->writesize >> eccbasft) | (data->column & 0xFF);
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type) | \
			CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(spare);

#if defined(CONFIG_NAND_V2_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
    cmd_f.cq4 |= CMD_DMA_HANDSHAKE_EN;
#endif
	if(w_wo_spare == 0) {
		memcpy((data->io_base + SPARE_SRAM + (data->cur_chan << 5)), p, mtd->oobsize);
		cmd_f.cq4 |= CMD_INDEX(LARGE_PAGEWRITE_W_SPARE);
	}
	else {
		cmd_f.cq4 |= CMD_INDEX(LARGE_PAGEWRITE);
	}

	status = ftnandc024_issue_cmd(mtd, &cmd_f);
	if (status < 0)
		goto out_write_page_lp;
	
#if defined(CONFIG_NAND_V2_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
    if(ftnandc024_nand_dma_wr(mtd, buf)) {
        printk("nand dma write page lp fail\n");
        goto out_write_page_lp;
    }
#else
	lbuf = (u32 *) buf;
	for (i = 0; i < mtd->writesize; i += 4)
		*(volatile unsigned *)(data->chip.IO_ADDR_R) = *lbuf++;
#endif

    if (ftnandc024_nand_wait(mtd, chip) == NAND_STATUS_FAIL) {
        status = -1;
        goto out_write_page_lp;
    }
  out_write_page_lp:
    platform_select_pinmux(nand_fd, -1);

    return status;
}

static int ftnandc024_nand_write_page_sp(struct mtd_info *mtd, const uint8_t * buf)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc024_nand_data *data = chip->priv;
    struct cmd_feature cmd_f;
    int progflow_buf[3];
    int i;
    int status = 0;
    u8 *p, w_wo_spare = 1;    
#ifndef CONFIG_NAND_V2_USE_AHBDMA
    u32 *lbuf;
#endif

    platform_select_pinmux(nand_fd, 0);
    write_protect(0);

	p = chip->oob_poi;
	for(i = 0; i < mtd->oobsize; i++) {
		if( *( p + i) != 0xFF) {
			w_wo_spare = 0;
			break;
		}
	}
	
	cmd_f.row_cycle = data->sys_hdr->nandfixup.nand_row_cycle - 1;
	cmd_f.col_cycle = data->sys_hdr->nandfixup.nand_col_cycle - 1;
	cmd_f.cq1 = data->page_addr;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = CMD_COUNT(1) | (data->column / mtd->writesize);
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type) |\
			CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(spare);
#if defined(CONFIG_NAND_V2_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
    cmd_f.cq4 |= CMD_DMA_HANDSHAKE_EN;
#endif
 	if(w_wo_spare == 0) {
		memcpy((data->io_base + SPARE_SRAM + (data->cur_chan << 5)), chip->oob_poi, mtd->oobsize);
		cmd_f.cq4 |= CMD_INDEX(SMALL_FIXFLOW_PAGEWRITE);
	}
	else {
		progflow_buf[0] = 0x41421D00;
		progflow_buf[1] = 0x66086460;
		progflow_buf[2] = 0x000067C7;
		ftnandc024_fill_prog_flow(mtd, progflow_buf, 10);
		cmd_f.cq4 |= CMD_PROM_FLOW | CMD_INDEX(0x0);
	}

	status = ftnandc024_issue_cmd(mtd, &cmd_f);
	if (status < 0)
		goto out_write_page_sp;
		
#if defined(CONFIG_NAND_V2_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
    if(ftnandc024_nand_dma_wr(mtd, buf)) {
        printk("nand dma write page sp fail\n");
        goto out_write_page_sp;
    }
#else
    lbuf = (u32 *) buf;
    for (i = 0; i < mtd->writesize; i += 4)
        *(volatile unsigned *)(data->chip.IO_ADDR_R) = *lbuf++;
#endif
    if (ftnandc024_nand_wait(mtd, chip) == NAND_STATUS_FAIL) {
        status = -1;
        goto out_write_page_sp;
    }
  out_write_page_sp:
    platform_select_pinmux(nand_fd, -1);
    return status;
}

static int ftnandc024_nand_write_page(struct mtd_info *mtd, struct nand_chip *chip,
                                      const uint8_t * buf, int page, int cached, int raw)
{
    struct nand_chip *p = mtd->priv;
    struct ftnandc024_nand_data *data = p->priv;
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

static void ftnandc024_nand_write_page_lowlevel(struct mtd_info *mtd,
                                                struct nand_chip *chip, const uint8_t * buf)
{
}

static void ftnandc024_nand_cmdfunc(struct mtd_info *mtd, unsigned command,
                                    int column, int page_addr)
{
    struct nand_chip *chip = mtd->priv;
    struct ftnandc024_nand_data *data = chip->priv;
    struct cmd_feature cmd_f;
    int real_pg;
    u8 id_size = 6;

    platform_select_pinmux(nand_fd, 0);

    cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type);
    data->cur_cmd = command;
    if (page_addr != -1)
        data->page_addr = page_addr;
    if (column != -1)
        data->column = column;
        
    real_pg = data->page_addr;
    
    switch (command) {
    case NAND_CMD_READID:
        /* read ID use sector mode, can't use page mode */
        DBGLEVEL2(ftnandc024_dbg( "Read ID@(CH:%d, CE:%d)\n", data->cur_chan, data->sel_chip));	
		
        data->byte_ofs = 0;
		cmd_f.row_cycle = ROW_ADDR_1CYCLE;
		cmd_f.col_cycle = COL_ADDR_1CYCLE;
		cmd_f.cq1 = 0;
		cmd_f.cq2 = 0;
		cmd_f.cq3 = CMD_COUNT(1);
		cmd_f.cq4 |= CMD_START_CE(data->sel_chip) | CMD_BYTE_MODE |\
				CMD_SPARE_NUM(id_size) | CMD_INDEX(FIXFLOW_READID);
		
		ftnandc024_issue_cmd(mtd, &cmd_f);
		
		ftnandc024_nand_wait(mtd, chip);
        break;
    case NAND_CMD_RESET:
		DBGLEVEL2(ftnandc024_dbg("Cmd Reset@(CH:%d, CE:%d)\n", data->cur_chan, data->sel_chip));
		
		cmd_f.cq1 = 0;
		cmd_f.cq2 = 0;
		cmd_f.cq3 = 0;
		cmd_f.cq4 |= CMD_START_CE(data->sel_chip);
		if (data->flash_type == ONFI2 || data->flash_type == ONFI3)
			cmd_f.cq4 |= CMD_INDEX(ONFI_FIXFLOW_SYNCRESET);
		else
			cmd_f.cq4 |= CMD_INDEX(FIXFLOW_RESET);

		ftnandc024_issue_cmd(mtd, &cmd_f);

		ftnandc024_nand_wait(mtd, chip);

        break;
    case NAND_CMD_STATUS:
		DBGLEVEL2(ftnandc024_dbg( "Read Status\n"));
		
		cmd_f.cq1 = 0;
		cmd_f.cq2 = 0;
		cmd_f.cq3 = CMD_COUNT(1);
		cmd_f.cq4 |= CMD_START_CE(data->sel_chip) | CMD_INDEX(FIXFLOW_READSTATUS);

		ftnandc024_issue_cmd(mtd, &cmd_f);
		
		ftnandc024_nand_wait(mtd, chip);
        break;
    case NAND_CMD_ERASE1:
    		write_protect(0);
		DBGLEVEL2(ftnandc024_dbg(
			"Erase Page: 0x%x, Real:0x%x\n", data->page_addr, real_pg));
		
		cmd_f.cq1 = real_pg;
		cmd_f.cq2 = 0;
		cmd_f.cq3 = CMD_COUNT(1);
		cmd_f.cq4 |= CMD_START_CE(data->sel_chip) | CMD_SCALE(1);
		
		cmd_f.row_cycle = data->sys_hdr->nandfixup.nand_row_cycle - 1;
		cmd_f.col_cycle = data->sys_hdr->nandfixup.nand_col_cycle - 1;
		if (data->large_page)
			cmd_f.cq4 |= CMD_INDEX(LARGE_FIXFLOW_ERASE);
		else
			cmd_f.cq4 |= CMD_INDEX(SMALL_FIXFLOW_ERASE);
		
		ftnandc024_issue_cmd(mtd, &cmd_f);
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
void ftnandc024_nand_select_chip(struct mtd_info *mtd, int chip)//???
{
    struct nand_chip *p = mtd->priv;
    struct ftnandc024_nand_data *data = p->priv;
    int chn = 0;

    //DBGLEVEL2(ftnandc024_dbg("chip = %d, ", chip));
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
#ifdef CONFIG_FTnandc024_HYNIX_HY27US08561A
    if (chip == 1)
        data->sel_chip = 2;
    else if (chip == 2)
        data->sel_chip = 1;
    else
        data->sel_chip = chip;
#else
    data->sel_chip = chip;
#endif

    //DBGLEVEL2(ftnandc024_dbg("==>chan = %d, ce = %d\n", data->cur_chan, data->sel_chip));
}

/*
 * Probe for the NAND device.
 */
static int __devinit ftnandc024_nand_probe(struct platform_device *pdev)
{
    struct ftnandc024_nand_data *data;
    struct mtd_partition *partitions;
    int res, chipnum, size;
    int i, sel, type;
    int partitions_num;
    u32 val;
    unsigned char spare_buf[64];

    res = chipnum = size = type = 0;
    /* Allocate memory for the device structure (and zero it) */
    data = kzalloc(sizeof(struct ftnandc024_nand_data), GFP_KERNEL);
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
    
#if defined(CONFIG_NAND_V2_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
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
    writel(BUSY_RDY_LOC(6) | CMD_STS_LOC(0) | CE_NUM(0) | DATA_INVERSE,
           data->io_base + GENERAL_SETTING);

	val = readl(data->io_base + AHB_SLAVEPORT_SIZE);
	val &= ~0xFFF0FF;
	val |= AHB_SLAVE_SPACE_32KB;
	for(i = 0; i < MAX_CHANNEL; i++)
		val |= AHB_PREFETCH(i);
	val |= AHB_PRERETCH_LEN(128);	
	writel(val, data->io_base + AHB_SLAVEPORT_SIZE);

    /* Currently, it is fixed in LEGACY_FLASH//???
     */
    sel = 2;
	//printk(KERN_INFO "Use %s NAND chip...\n", nand_attr[sel].name);
	spare = nand_attr[sel].sparesize;
	useecc = nand_attr[sel].ecc;
	useecc_spare = nand_attr[sel].ecc_spare;
	eccbasft = nand_attr[sel].eccbaseshift;
	protect_spare = nand_attr[sel].crc;
	
	data->flash_type = nand_attr[sel].flash_type;

    data->chip.priv = data;
    data->mtd.priv = &data->chip;
    data->mtd.owner = THIS_MODULE;
    data->mtd.name = "nand-flash";//u-boot commandline
    data->dev = &pdev->dev;
    data->chip.IO_ADDR_W = data->chip.IO_ADDR_R;
    data->chip.select_chip = ftnandc024_nand_select_chip;
    data->chip.cmdfunc = ftnandc024_nand_cmdfunc;
    data->chip.read_byte = ftnandc024_read_byte;
    data->chip.write_page = ftnandc024_nand_write_page;
    data->chip.waitfunc = ftnandc024_nand_wait;
    data->chip.block_markbad = ftnandc024v2_block_markbad;
    data->chip.chip_delay = 0;

    //data->chip.options = NAND_BBT_USE_FLASH | NAND_NO_SUBPAGE_WRITE | NAND_OWN_BUFFERS;
    data->chip.options = NAND_NO_SUBPAGE_WRITE | NAND_OWN_BUFFERS;      /* remove NAND_BBT_USE_FLASH */
    block_boundary = nand_attr[sel].block_boundary;
    platform_set_drvdata(pdev, data);

	// Set the default AC timing/Warmup cyc for ftnandc024. 
	// The register of AC timing/Warmup  keeps the value 
	// set before although the Global Reset is set. 
	ftnandc024_set_default_timing(&data->mtd);
	ftnandc024_set_default_warmup_cyc(&data->mtd);
	
	// Disable the scan state for ftnandc024_nand_select_chip
	data->scan_state = 0;

	// Read the raw id to calibrate the DQS delay for Sync. latching(DDR)
	ftnandc024_read_raw_id(&data->mtd);
	if(data->flash_type == TOGGLE1 || data->flash_type == TOGGLE2) {	
		ftnandc024_calc_timing(&data->mtd);	
		ftnandc024_calibrate_dqs_delay(&data->mtd, data->flash_type);	
	}

        /*--------------------------------------------------------
	 * ONFI flash must work in Asynch mode for READ ID command.
	 * Switch it back to Legacy.
	 */
	if (data->flash_type == ONFI2) {
		type = data->flash_type;
		data->flash_type = LEGACY_FLASH;
	}
	
	// Enable the scan state for ftnandc024_nand_select_chip
	data->scan_state = 1;
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
    data->chip.bbt_td = &ftnandc024_bbt_main_descr;
    data->chip.bbt_md = &ftnandc024_bbt_mirror_descr;
    data->chip.badblock_pattern = &ftnandc024_largepage_flashbased;

	if (spare < (data->mtd.writesize >> 5))
		spare = (data->mtd.writesize >> 5);

	val = readl(data->io_base + MEM_ATTR_SET);
	val &= ~(0x7 << 16);
	switch (data->mtd.writesize) {
	case 512:
		val |= PG_SZ_512;
		data->large_page = 0;
		break;
	case 2048:
		val |= PG_SZ_2K;
		data->large_page = 1;
		break;
	case 4096:
		val |= PG_SZ_4K;
		data->large_page = 1;
		break;
	case 8192:
		val |= PG_SZ_8K;
		data->large_page = 1;
		break;
	case 16384:
		val |= PG_SZ_16K;
		data->large_page = 1;
		break;
	}
	val &= ~(0x3FF << 2);
	val |= ((block_boundary - 1) << 2);
	writel(val, data->io_base + MEM_ATTR_SET);
	
	val = readl(data->io_base + MEM_ATTR_SET2); 
	val &= ~(0x3FF << 16);
	val |=  VALID_PAGE((data->mtd.erasesize / data->mtd.writesize - 1));
	writel(val, data->io_base + MEM_ATTR_SET2);

	i = ftnandc024_available_oob(&data->mtd);
	if (likely(i >= 4)) {
		if (i > 32)
			data->mtd.oobsize = 32;
		else
			data->mtd.oobsize = i;
		data->chip.ecc.layout->oobfree[0].length = data->mtd.oobsize;
	} else {
		res = -ENXIO;
		goto out_unset_drv;
	}	
	DBGLEVEL1(ftnandc024_dbg("total oobsize: %d\n", data->mtd.oobsize));
	
	switch(data->mtd.oobsize){
	case 4:
	case 8:
	case 16:
	case 32:
		spare = data->mtd.oobsize;
		/* In MTD utility, the oobsize should be multiple of 64. We should be on the way. */
		data->mtd.oobsize = nand_attr[sel].sparesize;
		DBGLEVEL1(ftnandc024_dbg("oobsize(page mode): %02d\n", data->mtd.oobsize));
		break;
	default:
		data->mtd.oobsize = 4;
		spare = data->mtd.oobsize;
		DBGLEVEL1(
			ftnandc024_dbg("Warning: Unknown spare setting %d, use default oobsize(page mode): 4\n"
			, data->mtd.oobsize));
		break;
	}
	
	if (useecc > 0) {
		DBGLEVEL1(ftnandc024_dbg("ECC correction bits: %d\n", useecc));
		writel(0x01010101, data->io_base + ECC_THRES_BITREG1);
		writel(0x01010101, data->io_base + ECC_THRES_BITREG2);
		val = (useecc - 1) | ((useecc - 1) << 8) | ((useecc - 1) << 16) | ((useecc - 1) << 24);
		writel(val, data->io_base + ECC_CORRECT_BITREG1);
		writel(val, data->io_base + ECC_CORRECT_BITREG2);

		val = readl(data->io_base + ECC_CONTROL);
		val &= ~ECC_BASE;
		if (eccbasft > 9)
			val |= ECC_BASE;
		val |= (ECC_EN(0xFF) | ECC_ERR_MASK(0xFF));
		writel(val, data->io_base + ECC_CONTROL);
		writel(ECC_INTR_THRES_HIT | ECC_INTR_CORRECT_FAIL, data->io_base + ECC_INTR_EN);
	} else {
		DBGLEVEL1(ftnandc024_dbg("ECC disabled\n"));
		writel(0, data->io_base + ECC_THRES_BITREG1);
		writel(0, data->io_base + ECC_THRES_BITREG2);
		writel(0, data->io_base + ECC_CORRECT_BITREG1);
		writel(0, data->io_base + ECC_CORRECT_BITREG2);

		val = readl(data->io_base + ECC_CONTROL);
		val &= ~ECC_BASE;
		val &= ~(ECC_EN(0xFF) | ECC_ERR_MASK(0xFF));
		val |= ECC_NO_PARITY;
		writel(val, data->io_base + ECC_CONTROL);
	}

	// Enable the Status Check Intr
	val = readl(data->io_base + INTR_ENABLE);
	val &= ~INTR_ENABLE_STS_CHECK_EN(0xff);
	val |= INTR_ENABLE_STS_CHECK_EN(0xff);
	writel(val, data->io_base + INTR_ENABLE);

	// Setting the ecc capability & threshold for spare
	writel(0x01010101, data->io_base + ECC_THRES_BITREG1);
	writel(0x01010101, data->io_base + ECC_THRES_BITREG2);
	val = (useecc_spare-1) | ((useecc_spare-1) << 8) | ((useecc_spare-1) << 16) | ((useecc_spare-1) << 24);
	writel(val, data->io_base + ECC_CORRECT_BIT_FOR_SPARE_REG1);  
	writel(val, data->io_base + ECC_CORRECT_BIT_FOR_SPARE_REG2);  

	DBGLEVEL2(ftnandc024_dbg("struct nand_buffers size:%d\n", sizeof(struct nand_buffers)));
	data->chip.buffers = kmalloc(sizeof(struct nand_buffers), GFP_KERNEL);
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

    data->chip.ecc.mode = NAND_ECC_HW;
    data->chip.ecc.size = data->mtd.writesize;
    data->chip.ecc.bytes = 0;
    data->chip.ecc.read_page = ftnandc024_nand_read_page;
    data->chip.ecc.write_page = ftnandc024_nand_write_page_lowlevel;
    data->chip.ecc.read_oob = ftnandc024_nand_read_oob_std;
    data->chip.ecc.write_oob = ftnandc024_nand_write_oob_std;
    data->chip.ecc.read_page_raw = ftnandc024_nand_read_page;

    if (data->large_page) {
        data->read_page = ftnandc024_nand_read_page_lp;
        data->write_page = ftnandc024_nand_write_page_lp;
        data->read_oob = ftnandc024_nand_read_oob_lp;
        data->write_oob = ftnandc024_nand_write_oob_lp;
    } else {
        data->read_page = ftnandc024_nand_read_page_sp;
        data->write_page = ftnandc024_nand_write_page_sp;
        data->read_oob = ftnandc024_nand_read_oob_sp;
        data->write_oob = ftnandc024_nand_write_oob_sp;
    }

#if defined(CONFIG_NAND_V2_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
#ifdef CONFIG_NAND_V2_USE_AHBDMA
    printk("use AHB DMA mode\n");
#else
    printk("use AXI DMA mode\n");
#endif    
    dma_cap_set(DMA_SLAVE, data->cap_mask);
    data->dma_chan = dma_request_channel(data->cap_mask, NULL, NULL);//step 1

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
        
        /* read system header 
         */
        //data->sys_hdr = kzalloc(data->mtd.writesize, GFP_KERNEL);
        data->sys_hdr = dma_alloc_coherent(&pdev->dev, data->mtd.writesize, &data->syshd_dmaaddr, GFP_KERNEL);
        if (data->sys_hdr == NULL) {
            printk("Warning............NAND: can't alloc memory");
            return -ENOMEM; 
        }
        data->page_addr = 0;

        //data->chip.oob_poi = data->chip.buffers->databuf + data->mtd.writesize;
        data->chip.oob_poi = spare_buf;

        if (data->read_page(&data->mtd, (u_char *) data->sys_hdr) < 0) {
            printk("Warning............NAND: read system header fail!");
            return -ENODEV;
        }

        printk("row_cycle = %d, nand_col_cycle = %d\n", data->sys_hdr->nandfixup.nand_row_cycle, data->sys_hdr->nandfixup.nand_col_cycle);
        /* read bad block table 
         */  
        //data->bi_table = kzalloc(data->mtd.writesize, GFP_KERNEL);
        data->bi_table = dma_alloc_coherent(&pdev->dev, data->mtd.writesize, &data->bitab_dmaaddr, GFP_KERNEL);
        if (data->bi_table == NULL) {
            printk("Warning............NAND: can't alloc table memory");
            return -ENOMEM;            
        }
        /* the bi table is next to system header */
        data->page_addr += 1;
        if (data->read_page(&data->mtd, (u_char *) data->bi_table) < 0) {
            printk("Warning............NAND: read bad block table fail!");
            return -ENODEV;
        }
            
        partitions = ftnandc024_partition_info;
        partitions_num = partition_check(partitions, data, data->mtd.erasesize); 

        /* 
         * calculate the mtd of user configuration base
         */
        /* user partition */       
        usr_base = data->sys_hdr->image[partitions_num].addr + data->sys_hdr->image[partitions_num].size + (2 * data->mtd.writesize);
        partitions[partitions_num].offset = ftnandc024v2_get_blockbase(&data->mtd, usr_base);

        /* restore the orginal setting */
        data->page_addr = 0;
    }

    /* Scan bad block and create bbt table
     */    
    nand_scan_tail(&data->mtd);

        /*----------------------------------------------------------
	 * ONFI synch mode means High Speed. If fails to change to
	 * Synch mode, then use flash as Async mode(Normal speed) and
	 * use LEGACY_LARGE fix flow.
	 */
	if (type == ONFI2) {
		if (ftnandc024_onfi_sync(&data->mtd) == 0) {
			data->flash_type = ONFI2;
			ftnandc024_calc_timing(&data->mtd);
			ftnandc024_calibrate_dqs_delay(&data->mtd, ONFI2);
		}
		else{
			data->flash_type = LEGACY_FLASH;
		}
	}
	
	// Toggle & ONFI flash has set the proper timing before READ ID.
	// We don't do that twice.	
	//if (data->flash_type == LEGACY_FLASH) {
		ftnandc024_calc_timing(&data->mtd);
	//}

    res = mtd_device_parse_register(&data->mtd, NULL, 0, partitions, partitions_num);
    if (!res)
        return res;

    nand_release(&data->mtd);
#if defined(CONFIG_NAND_V2_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
  out_free_dma:
    if(data->dma_chan)
    	dma_release_channel(data->dma_chan);
  out_free_buf:
#endif
    kfree(data->chip.buffers);

  out_unset_drv:
    platform_set_drvdata(pdev, NULL);

#if defined(CONFIG_NAND_V2_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
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
    struct ftnandc024_nand_data *data = chip->priv;
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

extern int erase_write_block0 (struct mtd_info *mtd, unsigned long pos, int len, const char *buf);
/**
 * ftnandc024v2_default_block_markbad - mark a block bad
 * @mtd:	MTD device structure
 * @ofs:	offset from device start
 *
 * This is the default implementation, which can be overridden by
 * a hardware specific driver.
*/
static int ftnandc024v2_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *chip = mtd->priv;
	struct ftnandc024_nand_data *data = chip->priv;
	unsigned int *bi_table = (unsigned int *)data->bi_table;
	int ret = 0;
	int quotient, remainder, blk_id, result;
	size_t retlen;

    printk("markbad, addr = 0x%x\n", (int)ofs);
    
    platform_select_pinmux(nand_fd, 0);
    
	/* Get block number */
	blk_id = (int)(ofs >> chip->bbt_erase_shift);
	if (data->bi_table){
	    blk_id = ofs >> chip->bbt_erase_shift;
	    quotient = blk_id >> 5;
	    remainder = blk_id & 0x1F;
    	result = (bi_table[quotient] >> remainder) & 0x1;
    	//printk("BI table %dth, bit %d, data = 0x%x, result = %d\n", quotient, remainder, bi_table[quotient], result);
    	if (result == 0) {/* bad block, not need to do it */
      	    ret = 0;
      	    goto exit_block_markbad;
        }      
	    
	    bi_table[quotient] &= ~(1 << remainder);/* Write the block mark. */
	    blk_id <<= 1;
	    //printk("blk_id %dth, data = 0x%x, result = %x\n", blk_id, chip->bbt[blk_id >> 3], (0x3 << (blk_id & 0x06)));
	    chip->bbt[blk_id >> 3] |= (0x3 << (blk_id & 0x06));	 	    
	}

	ret = erase_write_block0(mtd, 0, mtd->writesize, (const uint8_t *)data->sys_hdr);
	if (ret < 0)
		printk("Markbad Step 1 Error\n");

	//data->page_addr = 1;
	ret = mtd_write(mtd, mtd->writesize, mtd->writesize, &retlen, (const uint8_t *)bi_table);
	if (ret < 0)
		printk("Markbad Step 2 Error\n");
		
exit_block_markbad:	
	platform_select_pinmux(nand_fd, -1);
		
	return ret;
}

/*
 * @get block baseaddr. It skips the bad block as well.
 *
 * @ftnandc024v2_get_blockbase(struct mtd_info *mtd, unsigned int base_addr)
 * @param mtd: MTD device structure
 * @param base_addr: liner base address
 * @return: good block address
*/
unsigned int ftnandc024v2_get_blockbase(struct mtd_info *mtd, unsigned int base_addr)
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
static int __devexit ftnandc024_nand_remove(struct platform_device *pdev)
{
    struct ftnandc024_nand_data *data = platform_get_drvdata(pdev);

#if defined(CONFIG_NAND_V2_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
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

static void ftnandc024_release(struct device *dev)
{
}

static u64 ftnandc024_dmamask = DMA_BIT_MASK(32);

static struct platform_device ftnandc024_device = {
    .name = "ftnandc024_nand",
    .id = -1,
    .num_resources = ARRAY_SIZE(ftnandc024_resource),
    .resource = ftnandc024_resource,
    .dev = {
            .dma_mask = &ftnandc024_dmamask,
            .coherent_dma_mask = DMA_BIT_MASK(32),
            .release = ftnandc024_release,
            },
};

static struct platform_driver ftnandc024_nand_driver = {
    .probe = ftnandc024_nand_probe,
    .remove = __devexit_p(ftnandc024_nand_remove),
    .driver = {
               .name = "ftnandc024_nand",
               .owner = THIS_MODULE,
               },
};

static int __init ftnandc024_nand_init(void)
{
    int ret = 0;

    /* check if the system is running NAND system 
     */   
#ifndef CONFIG_FPGA
    if (platform_check_flash_type() != 0){
        printk("Not for NAND pin mux\n");
        return 0;
    }
#endif        
#if defined(CONFIG_NAND_V2_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
    init_waitqueue_head(&nand024_queue);
#endif    
    /* Register PMU and turn on gate clock
     */
    nand_fd = ftpmu010_register_reg(&pmu_reg_info);
    if (nand_fd < 0)
        printk("NANDC: register PMU fail");

#ifdef CONFIG_GPIO_WP
    if ((ret = gpio_request(GPIO_PIN, PIN_NAME)) != 0) {
        printk("gpio request fail\n");
        return ret;
    }
    printk("register GPIO for NAND write protect\n");
#endif

    if (platform_device_register(&ftnandc024_device)) {
        printk(KERN_ERR "device register failed\n");
        ret = -ENODEV;
    }
    if (platform_driver_register(&ftnandc024_nand_driver)) {
        printk(KERN_ERR "driver register failed\n");
        ret = -ENODEV;
    }
    return ret;
}

static void __exit ftnandc024_nand_exit(void)
{
    /* check if the system is running NAND system 
     */
#ifndef CONFIG_FPGA
    if (platform_check_flash_type() != 0)
        return;
#endif
    /* Deregister PMU
     */
    ftpmu010_deregister_reg(nand_fd);

    platform_driver_unregister(&ftnandc024_nand_driver);
    platform_device_unregister(&ftnandc024_device);
}

#if defined(CONFIG_NAND_V2_USE_AHBDMA) || defined(CONFIG_NAND_USE_AXIDMA)
/*
 * @callback function from DMAC module
 *
 * @ftnandc024_dma_callback int func(int ch, u16 int_status, void *data)
 * @param ch is used to indicate DMA channel
 * @param int_status indicates the interrupt status of DMA controller
 * @param data indicates the private data
 * @return: none
*/
void ftnandc024_dma_callback(void *param)
{
    //printk("%s\n", __func__);

    trigger_flag = 1;
    wake_up(&nand024_queue);

    return;
}
#endif

module_init(ftnandc024_nand_init);
module_exit(ftnandc024_nand_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Te-Chen Ying");
MODULE_DESCRIPTION("FTnandc024 V2.0 NAND driver");
MODULE_ALIAS("platform:ftnandc024_nand");
