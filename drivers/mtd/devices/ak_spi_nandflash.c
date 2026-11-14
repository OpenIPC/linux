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
#include <linux/mm.h>

#include <mach/map.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>

#include <linux/mtd/nand.h>

//#define SPINAND_DEBUG  
#ifdef SPINAND_DEBUG
#define PDEBUG(fmt, args...) printk( KERN_ALERT fmt,## args)
#define DEBUG(n, args...) printk(KERN_ALERT args)
#else
#define PDEBUG(fmt, args...)
#define DEBUG(n, args...) 
#endif


#define FHA_SUCCESS                 0
#define FHA_FAIL                    -1

#define BBT_BLOCK_GOOD      0x00
#define BBT_BLOCK_WORN      0x01
#define BBT_BLOCK_RESERVED  0x02
#define BBT_BLOCK_FACTORY_BAD   0x03

#define BBT_ENTRY_MASK      0x03
#define BBT_ENTRY_SHIFT     2

#define BADBLOCK_SCAN_MASK (~NAND_BBT_NO_OOB)

#define FEATURE_ECC_EN  (1<<4)  //(1<<5)

#define SPI_NAND_READ_ECC_ENABLE    (1)
#define SPI_NAND_READ_ECC_DISABLE   (0)

#define FLASH_BUF_SIZE          (flash->info.page_size)

#define SPI_FLASH_READ      1
#define SPI_FLASH_WRITE     2
#define BAD_BLOCK_MARK_OFFSET 4
//#define CONFIG_SPINAND_USE_FAST_READ 1

/*mtd layer allocate memory use for 'vmalloc' interface, need to convert.*/
//#define SPINAND_USE_MTD_BLOCK_LAYER  
#define OPCODE_RESET                        0xff   
#define OPCODE_WREN                         0x06    /* Write Enable */ 
#define OPCODE_WRDI                         0x04    /* Write Disable */ 

#define OPCODE_RDSR1                    0x0f    /* Read Status Register1 */
#define OPCODE_RDSR2                    0x35    /* Read Status Register2 */ 
#define OPCODE_RDSR3                    0x15    /* Read Status Register3 */ 
#define OPCODE_WRSR1            0x1f    /* Write Status Register */ 
#define OPCODE_WRSR2            0x31    /* Write Status2 Register*/ 
#define OPCODE_WRSR3            0x11    /* Write Status3 Register*/ 

#define OPCODE_READ_TO_CACHE    0x13
#define OPCODE_NORM_READ        0x03    /* Read Data Bytes */ 
#define OPCODE_FAST_READ        0x0b    /* Read Data Bytes at Higher Speed */ 
#define OPCODE_FAST_D_READ      0x3b    /* Read Data Bytes at Dual output */ 
#define OPCODE_FAST_Q_READ      0x6b    /* Read Data Bytes at Quad output */ 
#define OPCODE_FAST_D_IO        0xbb    /* Read Data Bytes at Dual i/o */ 
#define OPCODE_FAST_Q_IO        0xeb    /* Read Data Bytes at Quad i/o */ 

#define OPCODE_PP               0x02    /* Page Program */
#define OPCODE_PP_DUAL          0x12    /* Dual Page Program*/
#define OPCODE_PP_QUAD          0x32    /* Quad Page Program*/
#define OPCODE_2IO_PP           0x18    /* 2I/O Page Program (tmp)*/
#define OPCODE_4IO_PP           0x38    /* 4I/O Page Program*/
#define OPCODE_PP_EXEC          0x10

#define OPCODE_BE_4K            0x20    /* Sector (4K) Erase */ 
#define OPCODE_BE_32K           0x52    /* Block (32K) Erase */
#define OPCODE_BE_64K           0xd8    /* Block (64K) Erase */ 
#define OPCODE_SE               0xd8    /* Sector erase (usually 64KiB) */
#define OPCODE_ERASE_BLOCK      0xd8    /* block Erase */ 
#define OPCODE_RDID             0x9f    /* Read JEDEC ID */
#define OPCODE_DP               0xb9    /* Deep Power-down */ 
#define OPCODE_RES              0xab    /* Release from DP, and Read Signature*/

#define SPI_STATUS_REG1 1
#define SPI_STATUS_REG2 2

/* Define max times to check status register before we give up. */
#define MAX_READY_WAIT_JIFFIES  (40 * HZ)   /* 40s max chip erase */

#define CMD_SIZE        (1)
#define ADDR_SIZE       (2)
#define CMD_ADDR_SIZE   (CMD_SIZE + ADDR_SIZE)
#define MAX_DUMMY_SIZE  (4)

#define MTD_PART_NAME_LEN (4)

#ifdef CONFIG_SPINAND_USE_FAST_READ
#define OPCODE_READ     OPCODE_FAST_READ
#define FAST_READ_DUMMY_BYTE 1
#else
#define OPCODE_READ     OPCODE_NORM_READ
#define FAST_READ_DUMMY_BYTE 0
#endif

#define SPINAND_OOB_LEN(info)   \
    ((info.oob_up_skiplen + info.oob_seglen + \
    info.oob_down_skiplen)*info.oob_seg_perpage)

#define ALIGN_DOWN(a, b)  (((a) / (b)) * (b))

#define SPINAND_BIN_PAGE_START  (62)
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
    u32 BinPageStart; /*bin data start addr*/
    u32 PageSize;     /*spi page size*/
    u32 PagesPerBlock;/*page per block*/
    u32 BinInfoStart;
    u32 FSPartStart;
}
T_SPI_BURN_INIT_INFO;


uint8_t scan_0xff_pattern[] = { 0xff, 0xff };

#define SFLAG_UNDER_PROTECT         (1<<0)
#define SFLAG_FAST_READ             (1<<1)
#define SFLAG_AAAI                  (1<<2)
#define SFLAG_COM_STATUS2           (1<<3)

#define SFLAG_DUAL_IO_READ          (1<<4)
#define SFLAG_DUAL_READ             (1<<5)
#define SFLAG_QUAD_IO_READ          (1<<6)
#define SFLAG_QUAD_READ             (1<<7)

#define SFLAG_DUAL_IO_WRITE         (1<<8)
#define SFLAG_DUAL_WRITE            (1<<9)
#define SFLAG_QUAD_IO_WRITE         (1<<10)
#define SFLAG_QUAD_WRITE            (1<<11)
#define SFLAG_QUAD_NO_QE            (1<<14)

#define SFLAG_SECT_4K               (1<<12)

static int ak_scan_block_fast(struct mtd_info *mtd,
    struct nand_bbt_descr *bd, loff_t offs, uint8_t *buf, int numpages);

/*
 * SPI device driver setup and teardown
 */
struct flash_info {
    const char      *name;

    /* JEDEC id zero means "no ID" (most older chips); otherwise it has
     * a high byte of zero plus three data bytes: the manufacturer id,
     * then a two byte device id.
     */
    u32         jedec_id;
    u16         ext_id;
    u32         planecnt;
    u32         page_size;
    u32         page_per_block;

    /* The size listed here is what works with OPCODE_SE, which isn't
     * necessarily called a "sector" by the vendor.
     */
    unsigned    block_size;
    u16         n_blocks;

    /*|--------------------64bytes------------------------------|*/
    /*|---12---|-4--|---12----|-4--|---12---|-4--|---12----|-4--|*/
    /*|-seglen-|skip|-segllen-|skip|-seglen-|skip|-segllen-|skip|*/
    u32         oob_size;        
    u16         oob_up_skiplen;
    u16         oob_seglen;
    u16         oob_down_skiplen;   
    u16         oob_seg_perpage;
    u16         oob_vail_data_offset;

    /**
     *  chip character bits:
     *  bit 0: under_protect flag, the serial flash
     *      under protection or not when power on
     *  bit 1: fast read flag, the serial flash support
     *      fast read or not(command 0Bh)
     *  bit 2: AAI flag, the serial flash support
     *      auto address increment word programming
     *  bit 3: support dual write or no
     *  bit 4: support dual read or no
     *  bit 5: support quad write or no
     *  bit 6: support quad read or no
     *  bit 7: the second status command (35h) flag,
     *      if use 4-wire(quad) mode,the bit must be is enable
     */
    u16         flags;

    /* bad block flag ops */
    u16         badflag_offs;
    u16         badflag_len;
    u32         badflag_option;

    struct device_node *child;
};

/**
  *because of some spi nand is difference of status register difinition.
  *this structure use mapping the status reg function and corresponding.
*/
struct flash_status_reg
{
    u32     jedec_id;   
    u16     ext_id;
    unsigned b_wip:4;       /*write in progress*/
    unsigned b_wel:4;       /*wrute ebabke latch*/
    unsigned b_bp0:4;       /*block protected 0*/
    unsigned b_bp1:4;       /*block protected 1*/
    unsigned b_bp2:4;       /*block protected 2*/
    unsigned b_bp3:4;       /*block protected 3*/
    unsigned b_bp4:4;       /*block protected 4*/
    unsigned b_srp0:4;      /*status register protect 0*/
    
    unsigned b_srp1:4;      /*status register protect 1*/
    unsigned b_qe:4;        /*quad enable*/
    /*write protect control and status to the security reg.*/
    unsigned b_lb:4;
/*
    unsigned b_reserved0:4;
    unsigned b_reserved1:4;
    unsigned b_reserved2:4;
*/
    unsigned b_cmp:4;       /*conjunction bp0-bp4 bit*/
    unsigned b_sus:4;       /*exec an erase/program suspend cmd_buf*/
    unsigned b_efail:4;     /**/
    unsigned b_pfail:4;     /**/
};

struct ak_spinand {
    struct spi_device   *spi;
    struct mutex        lock;
    struct flash_info   info;
    struct mtd_info     mtd;
    unsigned            partitioned:1;
    
    u8      bus_width;
    unsigned char       *buf;
    u8      cmd_buf[CMD_ADDR_SIZE + MAX_DUMMY_SIZE];
    u8      dummy_len;

    u8      erase_opcode;
    u8      tx_opcode;
    u8      rx_opcode;
    u8      txd_bus_width;
    u8      rxd_bus_width;
    
    u8      txa_bus_width;
    u8      rxa_bus_width;  
    u32     page_shift;
    struct flash_status_reg stat_reg;
};

static struct mtd_info *ak_mtd_info;

static int HeYangTek_Flag = 0;  //the mata data of HeYangTek's product different form others

/*
 * feature cmd list ,reference by spec.
 * */
static int feature_cmd[3] = {0xC0, 0xB0, 0xA0};

static inline struct ak_spinand *mtd_to_spiflash(struct mtd_info *mtd)
{
    return container_of(mtd, struct ak_spinand, mtd);
}


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

    u8          st_tmp= 0;
    int         status;
    struct spi_message  message;
    struct spi_transfer x[2];

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
    u8  code = OPCODE_RESET;
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
    int         status;
    struct spi_message  message;
    struct spi_transfer x[2];

    spi_message_init(&message);
    memset(x, 0, sizeof x);
    
    flash->cmd_buf[0]= OPCODE_WREN;

    
    x[0].tx_buf = flash->cmd_buf;
    x[0].len = 1;
    spi_message_add_tail(&x[0], &message);

    /* do the i/o */
    status = spi_sync(flash->spi, &message);

    return status;
}


/**
* @brief Set write disble
* 
*  Set write disble instruction to the chip.
* @author lixinhai
* @date 2014-03-20
* @param[in] flash  spiflash handle.
* @return int return write success or failed
* @retval returns zero on success
* @retval return a negative error code if failed
*/
static inline int write_disable(struct ak_spinand *flash)
{
    u8  code = OPCODE_WRDI;

    return spi_write_then_read(flash->spi, &code, 1, NULL, 0);
}

/**
* @brief  Wait for SPI flash ready.
* 
*  Service routine to read status register until ready, or timeout occurs.
* @author lixinhai
* @date 2014-03-20
* @param[in] flash  spiflash handle.
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

        /*
         * XT26G01C和XT26G02C型号nand的ecc状态位需要特殊处理
         */
        if(0x0b110011 == flash->info.jedec_id && 0x0b120012 == flash->info.jedec_id)
        {
            if(((sr >> 4) & 0x0f) > 8){
                printk(KERN_ERR "ecc error sr:%d\n", sr );
                return 1;
            }
        }else{
            if(((sr >> 4) & 0x3) == 2)
            {
                printk(KERN_ERR "ecc error sr:%d\n", sr );
                return 1;
            }
        }
        return 0;

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

    if (flash->info.flags & SFLAG_QUAD_NO_QE) {
        return 0;
    }

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

    if (flash->info.flags & SFLAG_QUAD_NO_QE) {
        return 0;
    }

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
* @param[in] flash      spiflash handle.
* @param[in] offset    which is any address within the sector
*                      which should be erased.
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
        printk(KERN_ERR "kernel: error erase_block \
                        previous write command fail\n" );
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

    pr_debug("%s  offs:0x%llx\n",__func__,(long long)instr->addr);

    DEBUG(MTD_DEBUG_LEVEL2, "%s: %s %s 0x%llx, len %lld\n",
          dev_name(&flash->spi->dev), __func__, "at",
          (long long)instr->addr, (long long)instr->len);

    /* sanity checks */
    if (instr->addr + instr->len > mtd->size)
    {
        printk(KERN_ERR "ak_spinand_erase:instr->addr[0x%llx] + \
                        instr->len[%lld] > mtd->size[%lld]\n",
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
    flash->rxd_bus_width = SPI_NBITS_SINGLE; //XFER_1DATAWIRE;
    flash->rxa_bus_width = SPI_NBITS_SINGLE; //XFER_1DATAWIRE;
    flash->tx_opcode = OPCODE_PP;
    flash->txd_bus_width = SPI_NBITS_SINGLE; //XFER_1DATAWIRE;
    flash->txa_bus_width = SPI_NBITS_SINGLE; //XFER_1DATAWIRE;
    flash->dummy_len = 1;

    if(flash->bus_width & FLASH_BUS_WIDTH_2WIRE){
        if(flash->info.flags & SFLAG_DUAL_READ) {
            flash->rx_opcode = OPCODE_FAST_D_READ;
            flash->rxd_bus_width = SPI_NBITS_DUAL; //XFER_2DATAWIRE;
            flash->rxa_bus_width = SPI_NBITS_SINGLE; //XFER_1DATAWIRE;
            flash->dummy_len = 1;
        } else if (flash->info.flags & SFLAG_DUAL_IO_READ) {
            flash->rx_opcode = OPCODE_FAST_D_IO;
            flash->rxd_bus_width = SPI_NBITS_DUAL; //XFER_2DATAWIRE;
            flash->rxa_bus_width = SPI_NBITS_DUAL; //XFER_2DATAWIRE;
            flash->dummy_len = 1;
        }

        if(flash->info.flags & SFLAG_DUAL_WRITE) {
            flash->tx_opcode = OPCODE_PP_DUAL;
            flash->txd_bus_width = SPI_NBITS_DUAL; //XFER_2DATAWIRE;
            flash->txa_bus_width = SPI_NBITS_SINGLE; //XFER_1DATAWIRE;
        } else if(flash->info.flags & SFLAG_DUAL_IO_WRITE) {
            flash->tx_opcode = OPCODE_2IO_PP;
            flash->txd_bus_width = SPI_NBITS_DUAL; //XFER_2DATAWIRE;
            flash->txa_bus_width = SPI_NBITS_DUAL; //XFER_2DATAWIRE;
        }   
    }

    if(flash->bus_width & FLASH_BUS_WIDTH_4WIRE){
        if(flash->info.flags & SFLAG_QUAD_READ) {
            flash->rx_opcode = OPCODE_FAST_Q_READ;
            flash->rxd_bus_width = SPI_NBITS_QUAD; //XFER_4DATAWIRE;
            flash->rxa_bus_width = SPI_NBITS_SINGLE; //XFER_1DATAWIRE;
            flash->dummy_len = 1;
        }else if(flash->info.flags & SFLAG_QUAD_IO_READ){
            flash->rx_opcode = OPCODE_FAST_Q_IO;
            flash->rxd_bus_width = SPI_NBITS_QUAD; //XFER_4DATAWIRE;
            flash->rxa_bus_width = SPI_NBITS_QUAD; //XFER_4DATAWIRE;
            flash->dummy_len = 3;
        }

        if(flash->info.flags & SFLAG_QUAD_WRITE) {
            flash->tx_opcode = OPCODE_PP_QUAD;
            flash->txd_bus_width = SPI_NBITS_QUAD; //XFER_4DATAWIRE;
            flash->txa_bus_width = SPI_NBITS_SINGLE; //XFER_1DATAWIRE;
        }else if(flash->info.flags & SFLAG_QUAD_IO_WRITE) {
            flash->tx_opcode = OPCODE_4IO_PP;
            flash->txd_bus_width = SPI_NBITS_QUAD; //XFER_4DATAWIRE;
            flash->txa_bus_width = SPI_NBITS_QUAD; //XFER_4DATAWIRE;
        }
    
    }
    return 0;
}

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
            printk("config the spiflash quad enable fail. \
                        transfer use 1 wire.\n");
        }
    }
    else
        quad_mode_disable(flash);

    return ret;
}


#define FILL_CMD(c, val) do{c[0] = (val);}while(0)
#define FILL_ADDR(c, val) do{   \
        c[CMD_SIZE] = ((val) >> 8) & 0xf;   \
        c[CMD_SIZE+1] = ((val) & 0xff); \
        }while(0)
        
#define FILL_DUMMY_DATA(c, val) do{ \
            c[CMD_ADDR_SIZE] = val >> 16;   \
            c[CMD_ADDR_SIZE+1] = 0; \
            c[CMD_ADDR_SIZE+2] = 0; \
            c[CMD_ADDR_SIZE+3] = 0; \
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
        size_t *retlen, u_char *buf, int en_ecc)
{
    struct ak_spinand *flash = mtd_to_spiflash(mtd);
    struct spi_transfer t[4];
    struct spi_message m;
    struct spi_message m1;  
    void *bounce_buf;
    /* row unit is a page,not byte,that is 2048bytes*/
    int block = row/flash->info.page_per_block;
    
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
    
    //spi_write(flash->spi, flash->cmd_buf, 4);

    t[0].tx_buf = flash->cmd_buf;
    t[0].len = CMD_SIZE;
    //t[0].tx_nbits = flash->txa_bus_width;
    spi_message_add_tail(&t[0], &m);

    t[1].tx_buf = &flash->cmd_buf[CMD_SIZE];
    t[1].len = ADDR_SIZE + flash->dummy_len;
    //t[1].xfer_mode = flash->rxa_bus_width;
    //t[1].tx_nbits = flash->txa_bus_width;
    spi_message_add_tail(&t[1], &m);

    t[2].rx_buf = bounce_buf;
    t[2].len = len; 
    t[2].cs_change = 0; //1;    
    //t[2].xfer_mode = flash->rxd_bus_width;
    t[2].rx_nbits = flash->rxd_bus_width;
    

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
    
    /* block number is beginning from 0.*/
    if(flash->info.planecnt == 2 && block%2 != 0)
    {
        flash->cmd_buf[CMD_SIZE] |= 0x10;
    }
    
    FILL_DUMMY_DATA(flash->cmd_buf, 0x00);

    spi_sync(flash->spi, &m);

    *retlen = m.actual_length - CMD_ADDR_SIZE - flash->dummy_len;

    if (en_ecc && check_ecc_status(flash)) {
        /* REVISIT status return?? */
        mutex_unlock(&flash->lock);
        printk(KERN_ERR"kernel: check_ecc_status error, row:%d\n", row);
        return -EBUSY;
    }

    
    flash_buf_bounce_post(flash, buf, len, SPI_FLASH_READ);
    
    mutex_unlock(&flash->lock);
    return 0;
}
/*end of spinand_read*/

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
    //printk(KERN_ERR"count: %d, %d", count, row);
    while(count > 0) {
        xfer_len = (count > FLASH_BUF_SIZE) ? FLASH_BUF_SIZE : count;

        /*transfer len not greater than page size*/
        if(xfer_len > flash->info.page_size)
            xfer_len = ALIGN_DOWN(xfer_len, flash->info.page_size);
        if(xfer_len+column >= flash->info.page_size)
            xfer_len = flash->info.page_size - column;

        ret = spinand_read(mtd, row, column, xfer_len, 
                &rlen, buf + offset, SPI_NAND_READ_ECC_ENABLE);
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

static int spinand_do_read_page(struct mtd_info *mtd,
                    loff_t from, struct mtd_oob_ops *ops)
{
    int ret = 0;
    size_t rlen = 0;
    u32 xfer_len,read_len,read_len_oob;
    u32 oob_size,oob_xfer_len,oob_add_len,oob_seglen_len;
    u32 offset = 0;
    u32 oob_offset = 0; 
    u32 count = ops->len;
    int row, column;
    uint8_t *buf = NULL;    
    uint8_t *oobbuf = ops->oobbuf;
    uint8_t *datbuf = ops->datbuf;
    struct ak_spinand *flash = mtd_to_spiflash(mtd);
    u32 total_page = flash->info.page_size + flash->info.oob_size;
    uint8_t *r_buftmp = vmalloc(total_page);

    /*decode row/column in address param*/
    row = ((from>>flash->page_shift) & 0xffffff);
    column = (from & 0x7ff);

    if (HeYangTek_Flag) {
        read_len_oob = ops->ooblen;
        oob_seglen_len = flash->info.oob_up_skiplen + flash->info.oob_seglen 
                        + flash->info.oob_down_skiplen;
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

            ret = spinand_read(mtd, row, column, read_len, 
                        &rlen, r_buftmp, SPI_NAND_READ_ECC_ENABLE);
            if(unlikely(ret)) {
                ret = -EBUSY;
                goto out;
            }
            memcpy(datbuf + offset, r_buftmp, xfer_len);
            
            buf  = r_buftmp + rlen - oob_size + flash->info.oob_vail_data_offset;
            oob_add_len = 0;

            if(read_len_oob > 0){
                if(read_len_oob > 4){
                    memcpy(oobbuf, buf, 4);
                    read_len_oob -= 4;
                    oob_offset += 4;
                    oob_add_len += 4 + flash->info.oob_down_skiplen;
                    ops->oobretlen += 4;
                    while(read_len_oob){
                        oob_xfer_len = (read_len_oob > 8) ? 8 : read_len_oob;
                        memcpy(oobbuf + oob_offset, buf + oob_add_len, oob_xfer_len);
                        read_len_oob -= oob_xfer_len;
                        oob_offset += oob_xfer_len;
                        oob_add_len += 8 + flash->info.oob_down_skiplen;
                    }
                }else if(read_len_oob <= 4){
                    memcpy(oobbuf, buf, read_len_oob);
                }
            }else{
                oob_size = 0 ;      
            }
            row++;
            column = 0;
            ops->retlen += (rlen - oob_size);
            count -= (rlen - oob_size);     
            offset += (rlen-oob_size);  
        }
    }else{
        read_len_oob = ops->ooblen;
        oob_seglen_len = flash->info.oob_up_skiplen + flash->info.oob_seglen 
                        + flash->info.oob_down_skiplen;
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

            ret = spinand_read(mtd, row, column, read_len, 
                        &rlen, r_buftmp, SPI_NAND_READ_ECC_ENABLE);
            if(unlikely(ret)) {
                ret = -EBUSY;
                goto out;
            }
            memcpy(datbuf + offset, r_buftmp, xfer_len);
            
            buf  = r_buftmp + rlen - oob_size + flash->info.oob_vail_data_offset;
            oob_add_len = flash->info.oob_up_skiplen;

            if(read_len_oob > 0)
            {
                while(read_len_oob > 0)
                {                                          
                    oob_xfer_len = (read_len_oob > flash->info.oob_seglen) 
                                    ? flash->info.oob_seglen : read_len_oob;
                    memcpy(oobbuf+oob_offset, buf+oob_add_len, oob_xfer_len);
                    oob_add_len += oob_seglen_len;                          
                    read_len_oob -= oob_xfer_len;       
                    oob_offset += oob_xfer_len;
                    ops->oobretlen += oob_xfer_len;          
                }
            }else{
                oob_size = 0 ;      
            }
            row++;
            column = 0;
            ops->retlen += (rlen - oob_size);
            count -= (rlen - oob_size);     
            offset += (rlen-oob_size);  
        }
    }
out:
    vfree(r_buftmp);
    return ret;
}
/*end of spinand_do_read_page*/

/**
* @brief   MTD write
* 
* Write an address range to the flash chip.
* @author lixinhai
* @date 2014-03-20
* @param[in] mtd    mtd info handle.
* @param[in] to     write start address.
* @param[in] len    write length.
* @param[out] retlen  write length at actually.
* @param[out] buf      the pointer to write data.
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
    /* row unit is a page,not byte,that is 2048bytes*/
    int block = row/flash->info.page_per_block;

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


    //printk(KERN_ERR"1");
    
    mutex_lock(&flash->lock);
    bounce_buf = flash_buf_bounce_pre(flash, (void*)buf, len, SPI_FLASH_WRITE);
    t[0].tx_buf = flash->cmd_buf;
    t[0].len = CMD_SIZE;
    //t[0].tx_nbits = flash->txa_bus_width;
    spi_message_add_tail(&t[0], &m);
    t[1].tx_buf = &flash->cmd_buf[CMD_SIZE];
    t[1].len = ADDR_SIZE;
    //t[1].xfer_mode = flash->txa_bus_width;
    //t[1].tx_nbits = flash->txa_bus_width;
    spi_message_add_tail(&t[1], &m);
    t[2].tx_buf = bounce_buf;
    t[2].cs_change = 0;
    //t[2].xfer_mode = flash->txd_bus_width;
    t[2].tx_nbits = flash->txd_bus_width;
    
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
    /* block number is beginning from 0.*/
    if(flash->info.planecnt == 2 && block%2 != 0)
    {
        flash->cmd_buf[CMD_SIZE] |= 0x10;
    }   
    
    t[2].len = len;
    spi_sync(flash->spi, &m);
    *retlen = m.actual_length - CMD_ADDR_SIZE;

    flash->cmd_buf[0] = OPCODE_PP_EXEC;
    flash->cmd_buf[1] = (row >> 16) & 0xff;
    flash->cmd_buf[2] = (row >> 8) & 0xff;
    flash->cmd_buf[3] = row & 0xff;
    t[3].tx_buf = flash->cmd_buf;
    t[3].len = 4;
    //t[3].tx_nbits = flash->txd_bus_width;
    spi_message_add_tail(&t[3], &m1);
    spi_sync(flash->spi, &m1);      
    
    //spi_write(flash->spi, flash->cmd_buf, 4);
    /* Wait until finished previous write command. */
    if (wait_till_ready(flash))
    {
        mutex_unlock(&flash->lock);
        printk(KERN_ERR"kernel: error spinand_write write data fail\n");
        return -EBUSY;
    }

    PDEBUG("ak_spinand_write: retlen=%ld\n", *retlen);
    flash_buf_bounce_post(flash, (void*)buf, len, SPI_FLASH_WRITE);
    mutex_unlock(&flash->lock);
    return 0;
}
/*end of spinand_write*/

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

    xfer_len = (readlen > flash->info.oob_seglen) 
                ? flash->info.oob_seglen : readlen;
    end = start + xfer_len;
    seg_oob = flash->info.oob_up_skiplen + flash->info.oob_seglen
            + flash->info.oob_down_skiplen;
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

static int spinand_do_read_badflag(struct mtd_info *mtd, loff_t from,
                struct mtd_oob_ops *ops)
{

    struct ak_spinand *flash = mtd_to_spiflash(mtd);
    int ret = 0;
    size_t rlen = 0;
    /* u32 xfer_len; */
    /* u32 offset = 0; */
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

    if (HeYangTek_Flag) 
    {
        oobrw_buf =(uint8_t *)kmalloc(flash->info.oob_size, GFP_KERNEL);
        if (!oobrw_buf) {
            oobrw_buf =(uint8_t *)kmalloc(flash->info.oob_size, GFP_KERNEL);
            if (!oobrw_buf) {
                printk("allocate memory for pInit_info failed\n");
                return -ENOMEM;
                }
            }
        spinand_oobpos_to_coord(flash, from, ops->ooboffs, &row, &column);
        memset(oobrw_buf,0,flash->info.oob_size);
        ret = spinand_read(mtd, row, column, flash->info.oob_size, &rlen, oobrw_buf, SPI_NAND_READ_ECC_DISABLE);
        if(unlikely(ret)) {
            ret = -EBUSY;
            goto out;
        }
        column = 0;
        //column += flash->info.oob_up_skiplen;
        //column += flash->info.oob_vail_data_offset;
        memcpy(buf, oobrw_buf, 64);
    }else{    
        oobrw_buf =(uint8_t *)kmalloc(SPINAND_OOB_LEN(flash->info), GFP_KERNEL);
        if (!oobrw_buf) {
            oobrw_buf =(uint8_t *)kmalloc(SPINAND_OOB_LEN(flash->info),
                GFP_KERNEL);
            if (!oobrw_buf) {
                printk("allocate memory for pInit_info failed\n");
                return -ENOMEM;
                }
            }
        spinand_oobpos_to_coord(flash, from, ops->ooboffs, &row, &column);
        memset(oobrw_buf,0,SPINAND_OOB_LEN(flash->info));
        ret = spinand_read(mtd, row, column, SPINAND_OOB_LEN(flash->info),
                &rlen, oobrw_buf, SPI_NAND_READ_ECC_DISABLE);
        if(unlikely(ret)) {
            ret = -EBUSY;
            goto out;
        }
        column = 0;
        //column += flash->info.oob_up_skiplen;
        //column += flash->info.oob_vail_data_offset;
        memcpy(buf, oobrw_buf, SPINAND_OOB_LEN(flash->info));
    }
out:
    kfree(oobrw_buf);
    return ret;
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
    
    if  (HeYangTek_Flag)
    {
        oobrw_buf = (uint8_t *)kmalloc(flash->info.oob_size, GFP_KERNEL);
        if(!oobrw_buf){
            oobrw_buf = (uint8_t *)kmalloc(flash->info.oob_size, GFP_KERNEL);
            if(!oobrw_buf){
                printk("allocate memory for pInit_info failed\n");
                return -ENOMEM;
            }
        }
        spinand_oobpos_to_coord(flash, from, ops->ooboffs, &row, &column);

        memset(oobrw_buf, 0, flash->info.oob_size);
        ret = spinand_read(mtd, row, column, flash->info.oob_size, &rlen, oobrw_buf, SPI_NAND_READ_ECC_ENABLE);
        if(unlikely(ret)){
            ret = -EBUSY;
            goto out;
        }
        column = 0;
        column += flash->info.oob_vail_data_offset;
        offset = 0;		
        if(readlen > 4){
            memcpy(buf + offset, oobrw_buf + column, 4);
            column += 4 + flash->info.oob_down_skiplen;
            offset += 4;
            readlen -= 4;
            ops->oobretlen += 4;
            while(readlen > 0){
                xfer_len = (readlen > 8) ? 8 : readlen;
                memcpy(buf + offset, oobrw_buf + column, xfer_len);
                column += 8 + flash->info.oob_down_skiplen;
                offset += xfer_len;
                readlen -= xfer_len;
                ops->oobretlen += xfer_len;
            }
        }
        else if(readlen <= 4){
            memcpy(buf, oobrw_buf, readlen);
        }
    }else{				
        oobrw_buf =(uint8_t *)kmalloc(SPINAND_OOB_LEN(flash->info), GFP_KERNEL);
        if (!oobrw_buf) {
            oobrw_buf =(uint8_t *)kmalloc(SPINAND_OOB_LEN(flash->info),
                GFP_KERNEL);
            if (!oobrw_buf) {
                printk("allocate memory for pInit_info failed\n");
                return -ENOMEM;
            }
        }
        spinand_oobpos_to_coord(flash, from, ops->ooboffs, &row, &column);
        memset(oobrw_buf,0,SPINAND_OOB_LEN(flash->info));
        ret = spinand_read(mtd, row, column, SPINAND_OOB_LEN(flash->info),
                &rlen, oobrw_buf, SPI_NAND_READ_ECC_ENABLE);
        if(unlikely(ret)) {
            ret = -EBUSY;
            goto out;
        }
        column = 0;
        //column += flash->info.oob_up_skiplen;
        column += flash->info.oob_vail_data_offset;

        
        while(readlen > 0) {
    
            xfer_len = (readlen > flash->info.oob_seglen) ?
                            flash->info.oob_seglen : readlen;
            memcpy(buf + offset, oobrw_buf + column, xfer_len);
            column += (flash->info.oob_up_skiplen + flash->info.oob_seglen
                        + flash->info.oob_down_skiplen);

            readlen -= xfer_len;        
            offset += xfer_len;
            ops->oobretlen += xfer_len;
        }
    }
out:
    kfree(oobrw_buf);
    return ret;
}
/*end of spinand_do_read_oob*/

static int ak_spinand_read_oob(struct mtd_info *mtd, loff_t from,
             struct mtd_oob_ops *ops)
{
    int ret = -ENOTSUPP;
    pr_debug("%s  offs:0x%llx\n",__func__,(long long)from);
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
        ret = spinand_do_read_oob(mtd, from, ops);
    } else {
        ret = spinand_do_read_page(mtd, from, ops);
    }

 out:
    return ret;
}

static int spinand_do_write_badflag(struct mtd_info *mtd, loff_t to,
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

    if (HeYangTek_Flag){
        ret = spinand_oobpos_to_coord(flash, to, ops->ooboffs, &row, &column);

        while(writelen > 0){
                xfer_len = (writelen > 8) ? 8 : writelen;
                spinand_write(mtd, row, column, xfer_len, &rlen, buf + offset);
                column += 8 + flash->info.oob_down_skiplen;
                writelen -= rlen;
                offset += rlen;
        }
    }else{
        ret = spinand_oobpos_to_coord(flash, to, ops->ooboffs, &row, &column);
        //column += flash->info.oob_up_skiplen;
        //column += flash->info.oob_vail_data_offset;
        while(writelen> 0) {
            xfer_len = adjust_xfer_len(flash, column, writelen);

            //printk("wr:to(%d)ofs(%d):%d,%d,%d,%p",
            //      (u32)to, ops->ooboffs, row, column, xfer_len, buf);
            ret = spinand_write(mtd, row, column, xfer_len,
                                &rlen, buf + offset);
            if(unlikely(ret)) {
                ret = -EBUSY;
                goto out;
            }

            column += (flash->info.oob_up_skiplen + rlen +
                        flash->info.oob_down_skiplen);
            if(column >= ofs + SPINAND_OOB_LEN(flash->info)) {
                column = ofs;
                row++;
            }
            writelen -= rlen;
            offset += rlen;
        }
    }
out:
    return ret;

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

    if (HeYangTek_Flag){
        ret = spinand_oobpos_to_coord(flash, to, ops->ooboffs, &row, &column);

        column += flash->info.oob_vail_data_offset;
        if(writelen > 4){
            spinand_write(mtd, row, column, 4, &rlen, buf);
            column += 4 + flash->info.oob_down_skiplen;
            writelen -= rlen;
            offset += rlen;
            while(writelen > 0){
                xfer_len = (writelen > 8) ? 8 : writelen;
                spinand_write(mtd, row, column, xfer_len, &rlen, buf + offset);
                column += 8 + flash->info.oob_down_skiplen;
                writelen -= rlen;
                offset += rlen;
            }

        }else if(writelen <= 4){
            ret = spinand_write(mtd, row, column, writelen, &rlen, buf);
            if(unlikely(ret)){
                ret = -EBUSY;
                goto out;
            }
        }
    }else{
        ret = spinand_oobpos_to_coord(flash, to, ops->ooboffs, &row, &column);
        //column += flash->info.oob_up_skiplen;
        column += flash->info.oob_vail_data_offset;
        while(writelen> 0) {
            xfer_len = adjust_xfer_len(flash, column, writelen);
            ret = spinand_write(mtd, row, column, xfer_len,
                        &rlen, buf + offset);
            if(unlikely(ret)) {
                ret = -EBUSY;
                goto out;
            }
            
            column += (flash->info.oob_up_skiplen + rlen +
                        flash->info.oob_down_skiplen);
            if(column >= ofs + SPINAND_OOB_LEN(flash->info)) {
                column = ofs;
                row++;
            }
            writelen -= rlen;
            offset += rlen;
        }
    }
out:    
    return ret;     
    
}


static int spinand_do_write_page(struct mtd_info *mtd,
                    loff_t to, struct mtd_oob_ops *ops)
{
    int ret = 0;
    size_t rlen = 0;
    u32 xfer_len,read_len_oob;
    u32 oob_size,oob_xfer_len,oob_seglen_len;// read_len,
    u16 spare_offset;
    u32 offset = 0;
    u32 oob_offset = 0; 
    u32 oob_add_len = 0;
    u32 count = ops->len;
    int row, column;
    int row_oob, column_oob;
    uint8_t *p_buftmp_oob;
    u32 buftmp_len = 0;
    struct ak_spinand *flash = mtd_to_spiflash(mtd);
    uint8_t *w_buftmp_oob =vmalloc(flash->info.oob_size);
    int i;
    u32 total_page = flash->info.oob_size + flash->info.page_size;
    uint8_t *w_buftmp = vmalloc(total_page);
    uint8_t *oobbuf = ops->oobbuf;
    uint8_t *datbuf = ops->datbuf;

    
    /*decode row/column in address param*/
    row = ((to>>flash->page_shift) & 0xffffff);
    column = (to & 0x7ff);

    if (HeYangTek_Flag){
        read_len_oob = ops->ooblen;
        oob_size = flash->info.oob_size;
        spare_offset = flash->info.oob_vail_data_offset;

        while(count > 0){
            xfer_len = (count > FLASH_BUF_SIZE) ? FLASH_BUF_SIZE : count;

            /*transfer len not greater than page size*/
            if(xfer_len > flash->info.page_size){
                xfer_len = ALIGN_DOWN(xfer_len, flash->info.page_size);
            }
            if(xfer_len + column >= flash->info.page_size){
                xfer_len = flash->info.page_size - column;
            }
            
            memcpy(w_buftmp, datbuf + offset, xfer_len);

            spinand_oobpos_to_coord(flash, to, ops->ooboffs, &row_oob, &column_oob);
            ret = spinand_read(mtd, row_oob, column_oob, oob_size, &rlen, w_buftmp_oob, SPI_NAND_READ_ECC_ENABLE);

            p_buftmp_oob = w_buftmp_oob + spare_offset;
            if(read_len_oob > 4){
                memcpy(p_buftmp_oob, oobbuf, 4);
                read_len_oob -= 4;
                oob_add_len += 4 + flash->info.oob_down_skiplen;
                oob_offset += 4;
                // printk("line:%d:read_len_oob:%d, oob_add_len:%d, oob_offset:%d\r\n",
                //          __LINE__, read_len_oob, oob_add_len, oob_offset);
                while(read_len_oob > 0){
                    buftmp_len = (read_len_oob > 8) ? 8 : read_len_oob;
                    memcpy(p_buftmp_oob + oob_add_len, oobbuf + oob_offset, buftmp_len);
                    read_len_oob -= buftmp_len;
                    oob_add_len += 8 + flash->info.oob_down_skiplen;
                    oob_offset += buftmp_len;
                    // printk("line:%d:read_len_oob:%d, oob_add_len:%d, oob_offset:%d, buftmp_len:%d\r\n",
                    // __LINE__, read_len_oob, oob_add_len, oob_offset, buftmp_len);
                }

            }else if(read_len_oob <= 4){
                memcpy(p_buftmp_oob, oobbuf, read_len_oob);
            }
            memcpy(w_buftmp+xfer_len, w_buftmp_oob, oob_size);
            ret = spinand_write(mtd, row, column, xfer_len + oob_size, &rlen, w_buftmp);
            if(unlikely(ret)){
                ret = -EBUSY;
                goto out;
            }

            row++;
            column = 0;
            count -= xfer_len;
            offset += xfer_len;
        }
    }else{
        read_len_oob = ops->ooblen;
        oob_seglen_len = flash->info.oob_up_skiplen + flash->info.oob_seglen
                        + flash->info.oob_down_skiplen;
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

            memcpy(w_buftmp, datbuf+offset, xfer_len);
            
            spinand_oobpos_to_coord(flash, to, ops->ooboffs, &row_oob, &column_oob);
            ret = spinand_read(mtd, row_oob, column_oob, oob_size, 
                                &rlen, w_buftmp_oob, SPI_NAND_READ_ECC_ENABLE);

            PDEBUG("count=%d, xfer_data_len=%d, xfer_oob_len=%d\n",
                        count, xfer_len, read_len_oob);
            PDEBUG("row_oob=%d, column_oob=%d\n", row_oob, column_oob);
            
            for(i=0; i<4; i++)
                PDEBUG("w_buftmp_oob[%d] = 0x%02x ", i, w_buftmp_oob[i]);
            PDEBUG("\n");
            
            p_buftmp_oob = w_buftmp_oob + spare_offset; //offset to spare data
                
            while(read_len_oob> 0){
                buftmp_len = (read_len_oob > oob_xfer_len) ?
                            oob_xfer_len : read_len_oob;
                memcpy(p_buftmp_oob + oob_add_len, oobbuf+oob_offset, buftmp_len);
                read_len_oob -= buftmp_len;
                oob_offset += buftmp_len;
                oob_add_len +=oob_seglen_len;
            }
            memcpy(w_buftmp+xfer_len, w_buftmp_oob, oob_size);

            ret = spinand_write(mtd, row, column, xfer_len+oob_size,
                                &rlen, w_buftmp);
            if(unlikely(ret)) {
                ret = -EBUSY;
                goto out;
            }
            
            row++;
            column = 0;
    //      *retlen += xfer_len;
            count -= xfer_len;      
            offset += xfer_len;
        }
    }
out:
    vfree(w_buftmp_oob);
    vfree(w_buftmp);
    return ret; 
    
}
/*end of spinand_do_write_page*/

static int ak_spinand_write_oob(struct mtd_info *mtd, loff_t to,
             struct mtd_oob_ops *ops)
{
    int ret = -ENOTSUPP;
    pr_debug("%s  offs:0x%llx\n",__func__,(long long)to);
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
        ret = spinand_do_write_page(mtd, to, ops);

 out:
    return ret;
}

static int _ak_spinand_read(struct mtd_info *mtd, loff_t from, size_t len,
            size_t *retlen, u_char *buf)
{
    pr_debug("%s  offs:0x%llx\n",__func__,(long long)from);
    ak_spinand_read(mtd,from,len,retlen,buf);
    return 0;
}

static int _ak_spinand_write(struct mtd_info *mtd, loff_t to, size_t len,
        size_t *retlen, const u_char *buf)
{
    pr_debug("%s  offs:0x%llx\n",__func__,(long long)to);
    ak_spinand_write(mtd,to,len,retlen,buf);
    return 0;           
}

/**
 * ak_nand_default_block_markbad - [DEFAULT] 
 *  mark a block bad via bad block marker
 * @mtd: MTD device structure
 * @ofs: offset from device start
 *
 * This is the default implementation, which can be overridden by a hardware
 * specific driver. It provides the details for writing a bad block marker to a
 * block.
 */
static int ak_nand_default_block_markbad(struct mtd_info *mtd,
    loff_t ofs)
{
    struct nand_chip *chip = mtd->priv;
    struct mtd_oob_ops ops;
    uint8_t buf[2] = { 0, 0 };
    int ret = 0, res, i = 0;

    memset(&ops, 0, sizeof(ops));
    ops.oobbuf = buf;
    ops.ooboffs = chip->badblockpos;
    /* ops.len = ops.ooblen = 1; */
    ops.len = ops.ooblen = chip->badblock_pattern->len;

    ops.mode = MTD_OPS_PLACE_OOB;

    /* Write to first/last page(s) if necessary */
    if (chip->bbt_options & NAND_BBT_SCANLASTPAGE)
        ofs += mtd->erasesize - mtd->writesize;
    do {
        /* res = spinand_do_write_all_oob(mtd, ofs, &ops); */
        res = spinand_do_write_badflag(mtd, ofs, &ops);
        if (!ret)
            ret = res;

        i++;
        //ofs += mtd->writesize;
        
        if((chip->bbt_options & NAND_BBT_SCANLASTPAGE)
            && (chip->bbt_options & NAND_BBT_SCAN2NDPAGE))
        {
            ofs -= mtd->writesize;   /* mark last 2 page */
        }
        else if(chip->badblock_pattern->options & NAND_BBT_FIRSTANDLAST)
        {
            /* mark first and last page */
            ofs += mtd->erasesize - mtd->writesize;
        }
        else
        {
            ofs += mtd->writesize;   /* mark first two page */
        }
        
    } while (((chip->bbt_options & NAND_BBT_SCAN2NDPAGE)
            || (chip->badblock_pattern->options & NAND_BBT_FIRSTANDLAST))
            && i < 2);

    return ret;
}

static inline void ak_bbt_mark_entry(struct nand_chip *chip, int block,
        uint8_t mark)
{
    uint8_t msk = (mark & 0x03) << ((block & 0x03) * 2);
    chip->bbt[block >> 2] |= msk;
}

/**
 * ak_nand_markbad_bbt - [NAND Interface] Mark a block bad in the BBT
 * @mtd: MTD device structure
 * @offs: offset of the bad block
 */
static int ak_nand_markbad_bbt(struct mtd_info *mtd, loff_t offs)
{
    struct nand_chip *this = mtd->priv;
    int block, ret = 0;

    block = (int)(offs >> this->bbt_erase_shift);

    /* Mark bad block in memory */
    /* if generate bad block durning operate(erase,write),mark 0x01 */
    ak_bbt_mark_entry(this, block, BBT_BLOCK_WORN);
    return ret;
}

/**
 * ak_nand_block_markbad_lowlevel - mark a block bad
 * @mtd: MTD device structure
 * @ofs: offset from device start
 *
 * This function performs the generic NAND bad block marking steps (i.e., bad
 * block table(s) and/or marker(s)). We only allow the hardware driver to
 * specify how to write bad block markers to OOB (chip->block_markbad).
 *
 * We try operations in the following order:
 *  (1) erase the affected block, to allow OOB marker to be written cleanly
 *  (2) write bad block marker to OOB area of affected block (unless flag
 *      NAND_BBT_NO_OOB_BBM is present)
 *  (3) update the BBT
 * Note that we retain the first error encountered in (2) or (3), finish the
 * procedures, and dump the error in the end.
*/
static int ak_nand_block_markbad_lowlevel(struct mtd_info *mtd,
    loff_t ofs)
{
    struct nand_chip *chip = mtd->priv;
    int res, ret = 0;

    if (!(chip->bbt_options & NAND_BBT_NO_OOB_BBM)) {
        #if 1
        struct erase_info einfo;

        /* Attempt erase before marking OOB */
        memset(&einfo, 0, sizeof(einfo));
        einfo.mtd = mtd;
        einfo.addr = ofs;
        einfo.len = 1ULL << chip->phys_erase_shift;
        ak_spinand_erase(mtd, &einfo);   /* erase block */
        #endif

        /* Write bad block marker to OOB */
        ret = ak_nand_default_block_markbad(mtd, ofs);

        if (!ret)
            printk("%s:block %d has marked in oob",
                    __func__, (int)(ofs >> chip->bbt_erase_shift));
        
    }

    /* Mark block bad in BBT */
    if (chip->bbt) {
        res = ak_nand_markbad_bbt(mtd, ofs);
        if (!ret)
            ret = res;

        if (!ret)
            printk("%s:block %d has marked in bbt",
                    __func__, (int)(ofs >> chip->bbt_erase_shift));
        
    }

    if (!ret)
        mtd->ecc_stats.badblocks++;

    return ret;
}

static inline uint8_t ak_bbt_get_entry(struct nand_chip *chip, int block)
{
    uint8_t entry = chip->bbt[block >> BBT_ENTRY_SHIFT];
    entry >>= (block & BBT_ENTRY_MASK) * 2;
    return entry & BBT_ENTRY_MASK;
}

/**
 * ak_nand_isbad_bbt - [NAND Interface] Check if a block is bad
 * @mtd: MTD device structure
 * @offs: offset in the device
 * @allowbbt: allow access to bad block table region
 */
static int ak_nand_isbad_bbt(struct mtd_info *mtd, loff_t offs,
    int allowbbt)
{
    struct nand_chip *this = mtd->priv;
    int block, res;

    block = (int)(offs >> this->bbt_erase_shift);
    /* 2bit per block, find the block from bbt */
    res = ak_bbt_get_entry(this, block);
    pr_debug("nand_isbad_bbt(): bbt info for offs 0x%08x: (block %d) 0x%02x\n",
         (unsigned int)offs, block, res);

    if(res)
        pr_info("%s:block %d is a badblock", __func__, block);

    switch (res) {
    case BBT_BLOCK_GOOD:
        /* if res is 00B, it's a good block */
        return 0;
    case BBT_BLOCK_WORN:
        /* if res is 01B, it's a WORN block, WORN block is also a badlock.
            WORN block generate during operation(erase,write) */
        return 1;
    case BBT_BLOCK_RESERVED:
        /* if res is 10B, it's a RESERVED block.
            RESERVED block is use to save bbt.not use in mem_bbt */
        return allowbbt ? 0 : 1;
    }
    return 1;
}

/**
 * ak_nand_block_checkbad - [GENERIC] Check if a block is marked bad
 * @mtd: MTD device structure
 * @ofs: offset from device start
 * @getchip: 0, if the chip is already selected
 * @allowbbt: 1, if its allowed to access the bbt area
 *
 * Check, if the block is bad. Either by reading the bad block table or
 * calling of the scan function.
 */
static int ak_nand_block_checkbad(struct mtd_info *mtd, loff_t ofs,
    int getchip, int allowbbt)
{
    struct nand_chip *chip = mtd->priv;
    int numpages = 0;
    loff_t from;

    /* if not bbt, direct to read badflag in oob */
    if (!chip->bbt)
    {
        if (chip->badblock_pattern->options & NAND_BBT_SCAN2NDPAGE)
        {   
            numpages = 2;
        }
        else
        {
            numpages = 1;
        }

        /* Search first page and last page */
        if (chip->badblock_pattern->options & NAND_BBT_FIRSTANDLAST)
        {
            numpages = 2;
        }

        from = ofs;
        from = (from >> chip->bbt_erase_shift) << chip->bbt_erase_shift;
        if ((chip->bbt_options & NAND_BBT_SCANLASTPAGE)
            && !(chip->badblock_pattern->options & NAND_BBT_FIRSTANDLAST))
            from += mtd->erasesize - (mtd->writesize * numpages);
        
        return ak_scan_block_fast(mtd, chip->badblock_pattern, from,
                                    chip->buffers->databuf, numpages);
    }

    /* Return info from the table */
    return ak_nand_isbad_bbt(mtd, ofs, allowbbt);
}

/**
 * ak_new_nand_block_bad - [anyka] Read bad block marker in anyka bbt
 * @mtd:    MTD device structure
 * @ofs:    offset from device start
 *
 * Check, if the block is bad.
 */
int ak_new_spinand_block_isbad(struct mtd_info *mtd, loff_t offs)
{
    pr_debug("%s  offs:0x%llx\n",__func__,(long long)offs);
    return ak_nand_block_checkbad(mtd, offs, 1, 0);
}

int ak_new_spinand_block_markbad(struct mtd_info *mtd, loff_t offs)
{
    int ret;
    pr_debug("%s  offs:0x%llx\n",__func__,(long long)offs);
       /* check badblock before markblock */
    ret = ak_new_spinand_block_isbad(mtd, offs);
    if (ret) {
        /* If it was bad already, return success and do nothing */
        if (ret > 0)
            return 0;
        return ret;
    }

    return ak_nand_block_markbad_lowlevel(mtd, offs);
}

/**
* @brief    MTD get device ID
* 
* get the device ID of  the spi nand chip.
* @author lixinhai
* @date 2014-03-20
* @param[in] mtd     mtd info handle.
* @return int return device ID of  the spi nand chip.
*/
static int ak_spinand_get_devid(struct mtd_info *mtd)
{
    struct ak_spinand *flash = mtd_to_spiflash(mtd);
    int         ret;
    u8          id[5];
    u32         jedec;

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
        return false;
    }

    jedec = id[0] | (id[1]<<8) | (id[2]<<16) | (id[3]<<24) ;
    printk("spi nand ID: 0x%08x\n", jedec);

    return jedec;
}

/*
* @brief enable all meta data to ecc for HeYangTak product
* @author chenqiwen
* @date 2021-09-10
* @param void
* @return 0 in success,other false
*/
static int YHY_Set_Meta_All_Protect(struct ak_spinand *flash)
{
    int ret = 0;

    printk("set all meta data ecc\r\n");

    flash->cmd_buf[0] = 0x02;
    flash->cmd_buf[1] = 0x00;
    flash->cmd_buf[2] = 0x00;
    flash->cmd_buf[3] = 0xa5;
    flash->cmd_buf[4] = 0x01;
    flash->cmd_buf[5] = 0x00;
    ret = spi_write_then_read(flash->spi, flash->cmd_buf, 6, NULL, 0);
    if (ret < 0) {
        printk("%d: error %d set all meta data ecc failed\n",
            __LINE__, ret);
        return -1;
    }

    flash->cmd_buf[0] = 0x5f;
    flash->cmd_buf[1] = 0x25;
    flash->cmd_buf[2] = 0x5a;
    ret = spi_write_then_read(flash->spi, flash->cmd_buf, 3, NULL, 0);
    if (ret < 0) {
        printk("%d: error %d set all meta data ecc failed\n",
            __LINE__, ret);
        return -1;
    }

    if (wait_till_ready(flash))
        return -EBUSY;
    return 0;
}

/**
* @brief     get_flash_info
* 
* Read the device ID and identify that it was supported or not.
* @author   luoyongchuang
* @date         2016-03-17
* @param[in] mtd      spi device handle.
* @return int return the device info.
*/

static struct flash_info *get_flash_info(struct spi_device *spi)
{
    int         tmp;
    u8          cmd[2];
    u8          id[5];
    u32         jedec;
    u16                     ext_jedec = 0;
    struct flash_info *ofinfo = NULL;
    struct device *dev = &spi->dev;
    struct property *prop  = NULL;
    const char *namestr = NULL;
    struct device_node *child;
    u32 readid = 0;
    u32 ext_readid = 0;
    u32 dataout;
    
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

    pr_info("akspi nand ID: 0x%08x\n", jedec);

    /*cdh:search for spi controller device */
    prop = of_find_property(dev->of_node,"compatible", NULL);
    if(!prop){
        pr_err("%s,line:%d, find compatible failed\n", __func__, __LINE__);
        return NULL;
    }

    /*cdh:from norflash dtsi get match spi nandflash information */
    namestr = (const char *)kzalloc(20, GFP_KERNEL);
    if (!namestr){
        printk(KERN_ERR"%s,line:%d, allocate name buffer failed\n",
                    __func__, __LINE__);
        return NULL;
    }

    ofinfo = (struct flash_info *)kzalloc(sizeof(struct flash_info),
        GFP_KERNEL);
    if (!ofinfo){
        printk(KERN_ERR"%s,line:%d, allocate flash info failed\n",
                    __func__, __LINE__);
        kfree(namestr);
        return NULL;
    }

    for_each_available_child_of_node(dev->of_node, child){
        if (child->name && (of_node_cmp(child->name, "spi-nandflash") == 0)){
            of_property_read_u32(child, "nandflash-jedec-id", &readid);
            if (readid == jedec){
                of_property_read_u32(child, "nandflash-ext-id", &ext_readid);
                if (ext_readid != 0 && ext_readid != ext_jedec){
                    continue;
                }else{
                    ofinfo->child = child;
                    break;
                }
            }
        }
    }

    /*
    * judge search spi norflash device whether success or not
    */
    if (!ofinfo->child){
        printk(KERN_ERR"%s,line:%d, no find match device!\n",
                    __func__, __LINE__);
        goto err_exit;
    }


    
    /*
    * get spi nandflash information
    */
    of_property_read_string(child, "nandflash-name", &namestr);
    //printk(KERN_ERR"%s,child qijian. node name = %s\n", __func__, namestr);
    ofinfo->name = namestr;
    ofinfo->jedec_id = readid;
    ofinfo->ext_id = ext_readid;
    of_property_read_u32(child, "nandflash-planecnt", &ofinfo->planecnt);
    of_property_read_u32(child, "nandflash-page_size", &ofinfo->page_size);
    of_property_read_u32(child, "nandflash-page_per_block",
                                                &ofinfo->page_per_block);
    of_property_read_u32(child, "nandflash-block_size", &ofinfo->block_size);
    of_property_read_u32(child, "nandflash-n_blocks", &dataout);
    ofinfo->n_blocks = (u16)dataout;
    of_property_read_u32(child, "nandflash-flags", &dataout);
    ofinfo->flags = (u16)dataout;

    of_property_read_u32(child, "nandflash-oob_size", &ofinfo->oob_size);
    of_property_read_u32(child, "nandflash-oob_seglen", &dataout);
    ofinfo->oob_seglen = (u16)dataout;
    of_property_read_u32(child, "nandflash-oob_seg_perpage", &dataout);
    ofinfo->oob_seg_perpage = (u16)dataout;
    of_property_read_u32(child, "nandflash-oob_up_skiplen", &dataout);
    ofinfo->oob_up_skiplen = (u16)dataout;
    of_property_read_u32(child, "nandflash-oob_down_skiplen", &dataout);
    ofinfo->oob_down_skiplen = (u16)dataout;
    of_property_read_u32(child, "nandflash-oob_vail_data_offset", &dataout);
    ofinfo->oob_vail_data_offset = (u16)dataout;

    of_property_read_u32(child, "nandflash-badflag_offs", &dataout);
    ofinfo->badflag_offs = (u16)dataout;
    of_property_read_u32(child, "nandflash-badflag_len", &dataout);
    ofinfo->badflag_len = (u16)dataout;
    of_property_read_u32(child, "nandflash-badflag_option", &dataout);
    ofinfo->badflag_option = (u32)dataout;
    
    if(!strcmp(ofinfo->name, "HYF1GQ4UDACAE"))
    {
        pr_info("HYF1GQ4UDACAE especially\r\n");
        HeYangTek_Flag = 1;
    }else if(!strcmp(ofinfo->name, "HYF2GQ4UAACAE"))
    {
        pr_info("HYF2GQ4UAACAE especially\r\n");
        HeYangTek_Flag = 1;
    }else if(!strcmp(ofinfo->name, "HYF4GQ4UAACAE"))
    {
        pr_info("HYF4GQ4UAACAE especially\r\n");
        HeYangTek_Flag = 1;
    }

    return ofinfo;
err_exit:
    kfree(namestr);
    kfree(ofinfo);
    dev_err(&spi->dev, "get_flash_info() unrecognized flash id %06x\n", jedec);
    return NULL;
}
/*end of get_flash_info*/

static int ak_spinand_init_stat_reg(struct ak_spinand *flash)
{
    struct flash_info *info = &flash->info;
    u32 dataout;

    of_property_read_u32(info->child, "nandflash-b_wip", &dataout);
    flash->stat_reg.b_wip = dataout;
    of_property_read_u32(info->child, "nandflash-b_qe", &dataout);
    flash->stat_reg.b_qe = dataout;

    return 0;
}

int ak_set_chip(struct nand_chip *chip, struct flash_info *info)
{
    struct nand_bbt_descr *bd;
    struct nand_buffers *nbuf;

    if (!chip)
        return -ENOMEM;

    if (!info)
        return -ENODEV;

    chip->priv = (struct flash_info *)info;
    /* if scan bbt in memory,chip->bbt_td need to be set NULL */
    chip->bbt_td = NULL;
    /* if scan bbt in memory,chip->bbt_md need to be set NULL */
    chip->bbt_md = NULL;
    /* scan bad flag in first page and last page */
    chip->bbt_options |= info->badflag_option;
    chip->bbt_erase_shift = ffs(info->block_size) - 1;
    chip->phys_erase_shift = chip->bbt_erase_shift;
    /* bad flag opsition in a page oob */
    chip->badblockpos = info->badflag_offs;

    /* bad block scan steup */
    bd = kzalloc(sizeof(*bd), GFP_KERNEL);
    if (!bd)
        return -ENOMEM;
    /* bad flag in nand oob */
    bd->options = chip->bbt_options & BADBLOCK_SCAN_MASK;
    bd->offs = chip->badblockpos;   /* bad flag opsition in a page oob */
    bd->len = info->badflag_len;   /* bad flag len in a page oob */
    bd->pattern = scan_0xff_pattern;
    bd->options |= NAND_BBT_DYNAMICSTRUCT;
    chip->badblock_pattern = bd;
    pr_debug("FILE:%s FUNCTION:%s LINE:%d bd->options:%x, \
              bd->offs:%d, bd->len:%d, bd->pattern:0x%x, \
              chip->bbt_erase_shift:%d\n",
              __FILE__,__FUNCTION__,__LINE__, bd->options,
              bd->offs,bd->len,bd->pattern[0],
              chip->bbt_erase_shift);
    pr_debug("chip->badblockpos:%d   bd->offs:%d   \
                bd->len:%d   chip->bbt_options:0x%x\n",
              chip->badblockpos,bd->offs,bd->len,chip->bbt_options);

    /* nand_buffers setting */
    nbuf = kzalloc(sizeof(*nbuf) + info->page_size
                + info->oob_size * 3, GFP_KERNEL);

    if (!nbuf)
        return -ENOMEM;
    nbuf->ecccalc = (uint8_t *)(nbuf + 1);
    nbuf->ecccode = nbuf->ecccalc + info->oob_size;
    nbuf->databuf = nbuf->ecccode + info->oob_size;

    chip->buffers = nbuf;

    return 0;
}

static int ak_mtd_read_badflag(struct mtd_info *mtd,
            loff_t from, struct mtd_oob_ops *ops)
{
    int ret_code;
    ops->retlen = ops->oobretlen = 0;
    if (!mtd->_read_oob)
        return -EOPNOTSUPP;
    /*
     * In cases where ops->datbuf != NULL, mtd->_read_oob() has semantics
     * similar to mtd->_read(), returning a non-negative integer
     * representing max bitflips. In other cases, mtd->_read_oob() may
     * return -EUCLEAN. In all cases, perform similar logic to mtd_read().
     */
    /* ret_code = spinand_do_read_all_oob(mtd, from, ops); */
    ret_code = spinand_do_read_badflag(mtd, from, ops);
    if (unlikely(ret_code < 0))
        return ret_code;
    if (mtd->ecc_strength == 0)
        return 0;   /* device lacks ecc */
    return ret_code >= mtd->bitflip_threshold ? -EUCLEAN : 0;
}

/**
 * ak_check_short_pattern - [GENERIC] check if a pattern is in the buffer
 * @buf: the buffer to search
 * @td: search pattern descriptor
 *
 * Check for a pattern at the given place. Used to search bad block tables and
 * good / bad block identifiers. Same as check_pattern, but no optional empty
 * check.
 */
static int ak_check_short_pattern(uint8_t *buf, struct nand_bbt_descr *td)
{
    /* Compare the pattern */
     /*printk("FILE:%s FUNCTION:%s LINE:%d buf + \
            td->offs:%x td->pattern:%x td->len:%d\n",
            __FILE__,__FUNCTION__,__LINE__,(buf + td->offs)[0],
            (td->pattern)[0], td->len); */
    if (memcmp(buf + td->offs, td->pattern, td->len))
        return -1;
    return 0;
}


/* Scan a given block partially */
static int ak_scan_block_fast(struct mtd_info *mtd,
    struct nand_bbt_descr *bd, loff_t offs, uint8_t *buf, int numpages)
{
    struct mtd_oob_ops ops;
    int j, ret;

    ops.ooblen = mtd->oobsize;
    ops.oobbuf = buf;
    ops.ooboffs = 0;
    ops.datbuf = NULL;
    ops.mode = MTD_OPS_PLACE_OOB;

    for (j = 0; j < numpages; j++) {
        /*
         * read oob data
         */
        ret = ak_mtd_read_badflag(mtd, offs, &ops);

        /* Ignore ECC errors when checking for BBM */
        if (ret && !mtd_is_bitflip_or_eccerr(ret))
            return ret;

        /* check the oob bad block flag */
        if (ak_check_short_pattern(buf, bd))
        {
            pr_err("%s:block %lld is a badblock",
                    __func__, offs >> (ffs(mtd->erasesize) - 1));
            return 1;
        }

        if(2 == numpages && (bd->options & NAND_BBT_FIRSTANDLAST))
        {
            /* scan first page and last page */
            offs += mtd->erasesize - (mtd->writesize * 1);
        }
        else
        {
            offs += mtd->writesize;   /* scan first 2 page or last 2 page */
        }
    }
    return 0;
}


/**
 * ak_create_bbt - [GENERIC] Create a bad block table by scanning the device
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @bd: descriptor for the good/bad block search pattern
 * @chip: create the table for a specific chip, -1 read all chips; applies only
 *        if NAND_BBT_PERCHIP option is set
 *
 * Create a bad block table by scanning the device for the given good/bad block
 * identify pattern.
 */
static int ak_create_bbt(struct mtd_info *mtd, uint8_t *buf,
    struct nand_bbt_descr *bd, int chip)
{
    struct nand_chip *this = mtd->priv;
    int i, numblocks, numpages;
    int startblock;
    loff_t from;

    pr_info("Scanning device for bad blocks\n");

    if (bd->options & NAND_BBT_SCAN2NDPAGE)
        numpages = 2;
    else
        numpages = 1;

    /* Search first page and last page */
    if (bd->options & NAND_BBT_FIRSTANDLAST)
    {
        numpages = 2;
    }

    if (chip == -1) {
        numblocks = mtd->size >> this->bbt_erase_shift;
        startblock = 0;
        from = 0;
    } else {
        if (chip >= this->numchips) {
            pr_warn("ak_create_bbt(): chipnr (%d) > available chips (%d)\n",
                   chip + 1, this->numchips);
            return -EINVAL;
        }
        numblocks = this->chipsize >> this->bbt_erase_shift;
        startblock = chip * numblocks;
        numblocks += startblock;
        from = (loff_t)startblock << this->bbt_erase_shift;
    }

    if ((this->bbt_options & NAND_BBT_SCANLASTPAGE) &&
            !(bd->options & NAND_BBT_FIRSTANDLAST))
        from += mtd->erasesize - (mtd->writesize * numpages);

    for (i = startblock; i < numblocks; i++) {
        int ret;
        BUG_ON(bd->options & NAND_BBT_NO_OOB);
        /* check the block is good or not */
        ret = ak_scan_block_fast(mtd, bd, from, buf, numpages);
        pr_debug("FILE:%s FUNCTION:%s LINE:%d ret:%d\n",
                __FILE__,__FUNCTION__,__LINE__,ret);
        if (ret < 0)
            return ret;

        if (ret) {
            /* factory bad block mark 0x3 in bbt */
            ak_bbt_mark_entry(this, i, BBT_BLOCK_FACTORY_BAD);
            pr_warn("Bad eraseblock %d at 0x%012llx\n",
                i, (unsigned long long)from);
            mtd->ecc_stats.badblocks++;
        }

        from += (1 << this->bbt_erase_shift);
    }
    return 0;
}


/**
 * ak_nand_memory_bbt - [GENERIC] create a memory based bad block table
 * @mtd: MTD device structure
 * @bd: descriptor for the good/bad block search pattern
 *
 * The function creates a memory based bbt by scanning the device for
 * manufacturer / software marked good / bad blocks.
 */
static inline int ak_nand_memory_bbt(struct mtd_info *mtd,
                            struct nand_bbt_descr *bd)
{
    struct nand_chip *this = mtd->priv;
    /* create bbt */
    return ak_create_bbt(mtd, this->buffers->databuf, bd, -1);
}

/**
 * ak_nand_default_bbt -
 * [NAND Interface] Select a default bad block table for the device
 * @mtd: MTD device structure
 *
 * This function selects the default bad block table support for the device and
 * calls the nand_scan_bbt function.
 */
int ak_nand_default_bbt(struct mtd_info *mtd, struct nand_bbt_descr *bd)
{
    struct nand_chip *this = mtd->priv;
    int len, res;
    struct nand_bbt_descr *td = this->bbt_td;

    len = (mtd->size >> (this->bbt_erase_shift + 2)) ? : 1;
    /*
     * Allocate memory (2bit per block) and clear the memory bad block
     * table.
     */
    pr_debug("FILE:%s FUNCTION:%s LINE:%d len:%d\n",
            __FILE__, __FUNCTION__, __LINE__, len);
    this->bbt = kzalloc(len, GFP_KERNEL);
    if (!this->bbt)
        return -ENOMEM;

    /*
     * If no primary table decriptor is given, scan the device to build a
     * memory based bad block table.
     */
    if (!td) {
        pr_debug("FILE:%s FUNCTION:%s LINE:%d use memory\n",
                __FILE__,__FUNCTION__,__LINE__);
        if ((res = ak_nand_memory_bbt(mtd, bd))) {
            pr_err("nand_bbt: can't scan flash and build the RAM-based BBT\n");
            goto err;
        }
        return 0;
    }

err:
    kfree(this->bbt);
    this->bbt = NULL;
    return res;
}

#define OTP_ECC_MASK            0x10
/**
 * ak_spinand_enable_ecc- send command 0x1f to write the SPI Nand OTP register
 * Description:
 *   There is one bit( bit 0x10 ) to set or to clear the internal ECC.
 *   Enable chip internal ECC, set the bit to 1
 *   Disable chip internal ECC, clear the bit to 0
 */
static int ak_spinand_enable_ecc(struct ak_spinand *flash)
{
    int retval;
    u8 otp = 0;

    otp=(u8)read_sr(flash,0xb0);

    if ((otp & OTP_ECC_MASK) == OTP_ECC_MASK) {
        return 0;
    }
    else
    {
        otp |= OTP_ECC_MASK;
        retval=write_sr(flash, 0xb0, (u16)otp);
        if (retval != 0)
            return retval;
    }

    return 0;
}

static int ak_spinand_disable_ecc(struct ak_spinand *flash)
{
    int retval;
    u8 otp = 0;

    otp=(u8)read_sr(flash,0xb0);


    if ((otp & OTP_ECC_MASK) == OTP_ECC_MASK) {
        otp &= ~OTP_ECC_MASK;
        retval=write_sr(flash, 0xb0, (u16)otp);
        if (retval != 0)
            return retval;
    }
    return 0;
}

int spinand_read_partition(unsigned long start_pos,
            unsigned long partition_size, unsigned char *buf,
            unsigned long buf_len)
{
    unsigned long read_len = buf_len;
    unsigned long write_idex = 0;
    unsigned long page_idex = 0;
    unsigned char *tmp_buf = NULL;
    unsigned long will_read_len = 0;
    unsigned long buf_idex = 0, b_idex = 0;
    unsigned long page_size = ak_mtd_info->writesize;
    unsigned long erase_size = ak_mtd_info->erasesize;
    loff_t offs = 0;
    size_t retlen = 0;

    while(1)
    {

        if(read_len == 0)
        {
            break;
        }
        if(page_idex*page_size > partition_size)
        {
            pr_err("error: more than partition size, \
                    saved_size = %ld, write_size:%ld\n",
                    partition_size, page_idex*page_size);
            return -1;
        }

        offs = start_pos + b_idex*erase_size;
        if(ak_new_spinand_block_isbad(ak_mtd_info, offs) == 1)
        {
            pr_err("the block is bad block = %ld\n",
                    (start_pos + b_idex*erase_size)/erase_size);
            b_idex++;
            page_idex += (erase_size/page_size);
            continue;
        }

        tmp_buf = buf + write_idex*page_size;
        buf_idex = 0;
        while(1)
        {

            if(read_len > page_size)
            {
                will_read_len = page_size;
            }
            else
            {
                will_read_len = read_len;
            }

            if (ak_spinand_read(ak_mtd_info, start_pos+page_idex*page_size,
                          will_read_len, &retlen, &tmp_buf[buf_idex]) != 0)
            {
                pr_err("error: spinand_flash_read, pos:%ld\n",
                        start_pos+page_idex*page_size);
                return -1;
            }
            page_idex++;
            write_idex++;
            read_len -= will_read_len;
            buf_idex += will_read_len;

            if(read_len == 0)
            {
                break;
            }

            if((start_pos+page_idex*page_size)%erase_size == 0)
            {
                break;
            }

        }
        b_idex++;
    }
    return 0;
}


extern int parse_mtd_partitions(struct mtd_info *master,
                const char *const *types,struct mtd_partition **pparts,
                struct mtd_part_parser_data *data);

int spinand_get_partition_data(char *partition_name,
                char *data, unsigned long *date_len)
{
    int ret = 0, i = 0;
    struct mtd_partition *real_parts = NULL;
    struct mtd_partition *real_parts_saved = NULL;
    unsigned long partition_offset = 0;
    unsigned long partition_size = 0;

    if(data == NULL){
        pr_err("data null\n");
        return -1;
    }


    if(partition_name == NULL){
        pr_err("partition_name null\n");
        return -1;
    }

    ret = parse_mtd_partitions(ak_mtd_info, NULL, &real_parts, NULL);
    /* Didn't come up with either parsed OR fallback partitions */
    if (ret <= 0) {
        pr_err("failed to find partition %d\n",ret);
        /* Don't abort on errors; we can still use unpartitioned MTD */
        return -1;
    }
    real_parts_saved = real_parts;

    pr_err("partition num:%d\n",ret);

    pr_err("partition writesize:%d\n",ak_mtd_info->writesize);
    pr_err("partition erasesize:%d\n",ak_mtd_info->erasesize);

    for(i = 0; i < ret; i++){
        if(strncmp(real_parts->name, partition_name,
                        strlen(partition_name)) == 0){

            partition_offset = real_parts->offset;
            partition_size = real_parts->size;
            break;
        }

        pr_info("real_parts->name: %s\n",real_parts->name);
        pr_info("real_parts->offset: %x\n",(uint32_t)real_parts->offset);
        pr_info("real_parts->size: %x\n",(uint32_t)real_parts->size);
        real_parts++;
    }

    if(i == ret){
        pr_err("no find partition %s\n",partition_name);
        return -1;
    }
    // avoid write too much
    if ((*date_len > 0) && (partition_size > *date_len)) {
        //pr_err("partition_size %ld date_len %ld\n",
        //partition_size, *date_len);
        partition_size = *date_len;
    }

    if(spinand_read_partition(partition_offset, partition_size,
                                data, partition_size) == -1){
        pr_err("read partition %s data fail\n",partition_name);
        return -1;
    }

    *date_len = partition_size;

    kfree(real_parts_saved);
    return 0;

}

/**
*
*@brief: /sys/devices/platform/soc/21100000.spi0/spi_master/spi0/spi0.1/ak_spiflash_id
*@param[in] struct device *dev
*@param[in] struct device_attribute *attr
*@param[in] char *buf
*@return: ssize_t
*
**/
static ssize_t ak_spiflash_id_show(struct device *dev,
                struct device_attribute *attr,
                char *buf)
{
    struct ak_spinand *flash = dev_get_drvdata(dev);

    sprintf(buf, "%x\n", flash->info.jedec_id);
    return strlen(buf);
}

static DEVICE_ATTR_RO(ak_spiflash_id);

/**
* @brief     spi nand probe
* 
* Initial the spi nand device driver to kernel.
* @author luoyongchuang
* @date 2016-03-17
* @param[in] mtd      spi device handle.
* @return int return write success or failed
* @retval returns zero on success
* @retval return a non-zero error code if failed
*/
//static
int ak_spinand_probe(struct spi_device *spi)
{

    unsigned    i,bus_width = 0;
    unsigned    ret = 0;
    struct flash_info                       *info;
    struct ak_spinand                       *flash;
    struct nand_chip                        *chip;
    struct flash_platform_data  *data;  
    
    pr_info("ak spinandflash probe enter.\n");

    ret = of_property_read_u32(spi->dev.of_node, "bus-width",
                   &bus_width);
    if (ret < 0) {
         pr_err("Could not read bus-width property\n");
         return -ENODEV;
    }
    
    /* Platform data helps sort out which chip type we have, as
     * well as how this board partitions it.  If we don't have
     * a chip ID, try the JEDEC id commands; they'll work for most
     * newer chips, even if we don't recognize the particular chip.
     */
     
    data = spi->dev.platform_data;

    info = get_flash_info(spi);
    if (!info)
        return -ENODEV;

    chip = devm_kzalloc(&spi->dev, sizeof(struct nand_chip),
                GFP_KERNEL);
    if (!chip)
        return -ENOMEM;
    

    flash = kzalloc(sizeof *flash, GFP_KERNEL);
    if (!flash)
        return -ENOMEM;

     memset(flash, 0, sizeof *flash);
  
    ak_mtd_info = &flash->mtd;

    spi->mode =  SPI_TX_QUAD | SPI_RX_QUAD; 

    flash->spi = spi;
    flash->info = *info;
    mutex_init(&flash->lock);
    dev_set_drvdata(&spi->dev, flash);

    ret = ak_set_chip(chip,info);    /* use flash_info to setup nand_chip */
    if (ret != 0)
        return ret;
    
    sflash_reset(flash);

    if (data && data->name)
        flash->mtd.name = data->name;
    else
        flash->mtd.name = dev_name(&spi->dev);

    flash->mtd.type = MTD_NANDFLASH;
    flash->mtd.writesize = flash->info.page_size;
    flash->mtd.writebufsize = flash->info.page_size;
    flash->mtd.flags = MTD_WRITEABLE;
    flash->mtd.size = info->block_size * info->n_blocks;
    flash->mtd._erase = ak_spinand_erase;
    flash->mtd._write = _ak_spinand_write;
    flash->mtd._read = _ak_spinand_read;
    flash->mtd._read_oob = ak_spinand_read_oob;
    flash->mtd._write_oob = ak_spinand_write_oob;

    flash->mtd._block_isbad = ak_new_spinand_block_isbad;
    flash->mtd._block_markbad = ak_new_spinand_block_markbad;
    flash->mtd.get_device_id = ak_spinand_get_devid;
    flash->erase_opcode = OPCODE_ERASE_BLOCK;
    flash->mtd.erasesize = info->block_size;
    flash->mtd.oobsize = info->oob_size;
    flash->mtd.oobavail = info->oob_seglen * info->oob_seg_perpage;
    flash->mtd.priv = chip;
    //FLASH_BUS_WIDTH_4WIRE | FLASH_BUS_WIDTH_2WIRE |
    //FLASH_BUS_WIDTH_1WIRE; //data->bus_width;
    flash->bus_width = bus_width;

    pr_info("flash->bus_width:%d(note:1:1 wire, \
            2:2 wire, 4:4 wire)\n", flash->bus_width);
    
    flash->page_shift = ffs(flash->mtd.writesize)-1; 
#ifdef SPINAND_USE_MTD_BLOCK_LAYER
    /*pre-allocation buffer use for spi nand data transfer.*/
    flash->buf = kzalloc(FLASH_BUF_SIZE + flash->info.oob_size, GFP_KERNEL);
    if (!flash->buf) {
        pr_err("Allocate buf for spi page failed\n");
        kfree(flash);
        return -ENOMEM;
    }
#endif

    ak_spinand_init_stat_reg(flash); 
    ak_spinand_cfg_quad_mode(flash); 
    init_spiflash_rw_info(flash);
    if (HeYangTek_Flag)
    {
        YHY_Set_Meta_All_Protect(flash);
    }

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

    /* partitions should match sector boundaries; and it may be good to
     * use readonly partitions for writeprotected sectors (BP2..BP0).
     */
#ifdef CONFIG_MTD_CMDLINE_PARTS
    /* badblock scan and bbt creat in mem */
    if (0x1150115 != flash->info.jedec_id)  //GSS01GSAM0 must always enable ECC
    {
        ak_spinand_disable_ecc(flash);
    }   
    ret = ak_nand_default_bbt(&flash->mtd, chip->badblock_pattern);
    if (0x1150115 != flash->info.jedec_id)  //GSS01GSAM0 must always enable ECC
    {
        ak_spinand_enable_ecc(flash);
    }

    if(ret)
    {
        pr_err("scan bbt failed in memory\n");
        kfree(flash->buf);
        kfree(flash);
        return ret;
    }
#endif

    ret = mtd_device_parse_register(&flash->mtd, NULL, NULL, NULL, 0);
    if (ret) {
        pr_err("Add root MTD device failed\n");
        kfree(flash->buf);
        kfree(flash);
        return -EINVAL;
    }

    ret = sysfs_create_file(&spi->dev.kobj, &dev_attr_ak_spiflash_id.attr);
    if (ret) {
        pr_err("%s,line:%d, create ak_spiflash_id sysfs file failed!\n",
                    __func__, __LINE__);
    }

    pr_info("Init AK SPI Flash finish.\n"); 
    kfree(info);
    return 0;
}
/*end of ak_spinand_probe*/

/**
* @brief      spi nand remove
* 
* Remove the spi nand device driver from kernel.
* @author lixinhai
* @date 2014-03-20
* @param[in] mtd       spi device handle.
* @return int return write success or failed
* @retval returns zero on success
* @retval return a non-zero error code if failed
*/
static int ak_spinand_remove(struct spi_device *spi)
{
    struct ak_spinand   *flash = dev_get_drvdata(&spi->dev);
    int     status;
    
    sysfs_remove_file(&spi->dev.kobj, &dev_attr_ak_spiflash_id.attr);
    status = mtd_device_unregister(&flash->mtd);
    if (status == 0) {
        kfree(flash->buf);
        kfree(flash);
    }
    return 0;
}


static const struct of_device_id spinand_of_table[] = {
    { .compatible = "anyka,ak-spinand" },
    {}
};


MODULE_DEVICE_TABLE(of, spinand_of_table);


static struct spi_driver ak_spinand_driver = {
    .driver = {
        .name   = "ak-spinand",
        .bus    = &spi_bus_type,
        .owner  = THIS_MODULE,
        .of_match_table = spinand_of_table,
    },
    .probe  = ak_spinand_probe,
    .remove = ak_spinand_remove,

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
    pr_info("init Anyka SPI Nand Flash driver\n");
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
    pr_info("exit Anyka SPI Nand Flash driver\n");  
    spi_unregister_driver(&ak_spinand_driver);
}


module_init(ak_spinand_init);
module_exit(ak_spinand_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anyka Microelectronic");
MODULE_DESCRIPTION("MTD SPI Nand flash driver for Anyka chips");
MODULE_VERSION("1.0.02");
