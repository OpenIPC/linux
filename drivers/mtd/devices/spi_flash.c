/*
 * MTD SPI driver for ST M25Pxx (and similar) serial flash chips
 *
 * Author: Mike Lavender, mike@steroidmicros.com
 *
 * Copyright (c) 2005, Intec Automation Inc.
 *
 * Some parts are based on lart.c by Abraham Van Der Merwe
 *
 * Cleaned up and generalized based on mtd_dataflash.c
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Copyright (c) 2011 BingJiun Luo <bjluo@faraday-tech.com>
 * - Modify for FTSPI020 Faraday SPI flash controller.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/delay.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <mach/platform/pmu.h>

#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#ifdef MODULE
#include "ftspi020.h"
#else
#include <linux/spi/ftspi020.h>
#endif

#define FLASH_PAGESIZE		256

/* Flash opcodes. */
#define	OPCODE_WREN		0x06    /* Write enable */
#define	OPCODE_RDSR		0x05    /* Read status register */
#define	OPCODE_RDSR2	0x35
#define	OPCODE_WRSR		0x01    /* Write status register 1 byte */
#define	OPCODE_EN4B		0xB7
#define	OPCODE_EX4B		0xE9
#define	OPCODE_NORM_READ	0x03    /* Read data bytes (low frequency) */
#define	OPCODE_FAST_READ	0x0b    /* Read data bytes (high frequency) */
#define OPCODE_FAST_READ_DUAL	    0x3B
#define	OPCODE_QUAD_READ	0xEB
#define OPCODE_WINBOND_PP 0x32
#define	OPCODE_QUAD_PP		0x38
#define	OPCODE_PP		0x02    /* Page program (up to 256 bytes) */
#define	OPCODE_SE		0x20    /* Erase Sector 4KiB */
#define	OPCODE_BE_32K		0x52    /* Erase 32KiB block */
#define	OPCODE_BE_64K		0xd8    /* Erase 64KiB block */
#define	OPCODE_CHIP_ERASE	0xc7    /* Erase whole flash chip */
#define	OPCODE_RDID		0x9f    /* Read JEDEC ID */

/* Status Register bits. */
#define	SR_WIP			1       /* Write in progress */
#define	SR_WEL			2       /* Write enable latch */
/* meaning of other SR_* bits may differ between vendors */
#define	SR_BP0			4       /* Block protect 0 */
#define	SR_BP1			8       /* Block protect 1 */
#define	SR_BP2			0x10    /* Block protect 2 */
#define	SR_SRWD			0x80    /* SR write protect */

#define spi_operate_serial_mode         0
#define spi_operate_dual_mode           0x20
#define spi_operate_quad_mode           0x40
#define spi_operate_dualio_mode         0x60
#define spi_operate_quadio_mode         0x80

#define	CMD_SIZE		4
/*
#ifdef CONFIG_SPI_QUAD_MODE
#define OPCODE_READ 	OPCODE_QUAD_READ
#define READ_DUMMY_CYCLE	4
#else
#if 1                           //USE_FAST_READ
#define OPCODE_READ 	OPCODE_FAST_READ
#define READ_DUMMY_CYCLE	8
#else
#define OPCODE_READ 	OPCODE_NORM_READ
#define READ_DUMMY_CYCLE	0
#endif
#endif
*/
#define READ_DUMMY_CYCLE	8

unsigned char OPCODE_READ_COMMAND, OPCODE_WRITE_COMMAND, OPCODE_ERASE_COMMAND;
  
#define SPI_NAME	"SPI_FLASH"

#define BLOCK_ALIGN(base, blk_shift) ((((base) + (0x1 << (blk_shift)) - 1) >> (blk_shift)) << (blk_shift))

static struct mtd_partition ftspi020_partition_info[20];

static struct flash_platform_data spi_flash_platform_data = {
    .name = "wb_spi_flash",
    .parts = ftspi020_partition_info,
    .nr_parts = ARRAY_SIZE(ftspi020_partition_info)
};

#ifdef CONFIG_SECOND_SPI_FLASH
static struct mtd_partition extra_partitions[] = {
    {
     .name = "Extra Code Section",
     .offset = 0x000000,
     .size = 0x800000           // 8MB
     },
    {
     .name = "Extra User Section",      // free for use
     .offset = 0x800000,
     .size = 0x800000},
};

static struct flash_platform_data extra_spi_flash_platform_data = {
    .name = "wb_spi_flash",
    .parts = extra_partitions,
    .nr_parts = ARRAY_SIZE(extra_partitions)
};
#endif //end of CONFIG_SECOND_SPI_FLASH
static struct spi_board_info spi_devs_info[] __initdata = {
    {
     .modalias = SPI_NAME,
     .platform_data = &spi_flash_platform_data,
     .max_speed_hz = 45 * 1000 * 1000,  //40Mhz
     .bus_num = 0,              //on bus 0
     .chip_select = 0,          //first chip select//???
     .mode = SPI_MODE_0,
     },
#ifdef CONFIG_SECOND_SPI_FLASH
    {
     .modalias = SPI_NAME,
     .platform_data = &extra_spi_flash_platform_data,
     .max_speed_hz = 45 * 1000 * 1000,  //40Mhz
     .bus_num = 0,              //on bus 0
     .chip_select = 1,          //first chip select
     .mode = SPI_MODE_0,
     }
#endif
};

static const char *part_probes[] = { "cmdlinepart", NULL, };

/****************************************************************************/

struct common_spi_flash {
    struct spi_device *spi;
    struct mutex lock;
    struct mtd_info mtd;
    sys_header_t *sys_hdr;      /* system header */

    u8 erase_opcode;
    u8 mode_3_4;
    u8 flash_type;
};

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
    struct spi_transfer t[2];
    struct spi_message m;
    u8 val;
    struct ftspi020_cmd cmd[2];

    pr_debug("%s: %s\n", dev_name(&flash->spi->dev), __func__);

    spi_message_init(&m);
    memset(t, 0, (sizeof t));

    t[0].tx_buf = &cmd[0];
    spi_message_add_tail(&t[0], &m);

    memset(&cmd[0], 0, sizeof(struct ftspi020_cmd));
    cmd[0].ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
    cmd[0].write_en = FTSPI020_CMD3_READ;
    cmd[0].read_status_en = FTSPI020_CMD3_RD_STS_EN;
    cmd[0].read_status = FTSPI020_CMD3_STS_SW_READ;
    cmd[0].ins_code = FTSPI020_CMD3_INSTR_CODE(ins_cmd);
    cmd[0].flags = (FTSPI020_XFER_CMD_STATE | FTSPI020_XFER_CHECK_CMD_COMPLETE);

    t[1].tx_buf = &cmd[1];
    spi_message_add_tail(&t[1], &m);

    memset(&cmd[1], 0, sizeof(struct ftspi020_cmd));
    cmd[1].rx_buf = &val;
    cmd[1].data_cnt = 1;
    cmd[1].read_status_en = FTSPI020_CMD3_RD_STS_EN;
    cmd[1].read_status = FTSPI020_CMD3_STS_SW_READ;;
    cmd[1].flags = FTSPI020_XFER_DATA_STATE;

    spi_sync(flash->spi, &m);

    if (m.status < 0) {
        dev_err(&flash->spi->dev, "error %d reading SR\n", (int)m.actual_length);
        return m.status;
    }

    return val;
}

/*
 * Write status register 1 byte
 * Returns negative if error occurred.
 */
static int write_sr(struct common_spi_flash *flash, u8 * val, u8 len)
{
    struct spi_transfer t;
    struct spi_message m;
    struct ftspi020_cmd cmd;

    pr_debug("%s: %s\n", dev_name(&flash->spi->dev), __func__);

    spi_message_init(&m);
    memset(&t, 0, (sizeof t));

    t.tx_buf = &cmd;
    spi_message_add_tail(&t, &m);

    memset(&cmd, 0, sizeof(struct ftspi020_cmd));
    cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
    cmd.write_en = FTSPI020_CMD3_WRITE;
    cmd.ins_code = FTSPI020_CMD3_INSTR_CODE(OPCODE_WRSR);
    cmd.tx_buf = val;
    cmd.data_cnt = len;
    cmd.flags =
        (FTSPI020_XFER_CMD_STATE | FTSPI020_XFER_DATA_STATE | FTSPI020_XFER_CHECK_CMD_COMPLETE);

    spi_sync(flash->spi, &m);

    if (m.status < 0) {
        dev_err(&flash->spi->dev, "error %d write enable\n", (int)m.status);
        return m.status;
    }

    return 0;
}

static inline int change_4b(struct common_spi_flash *flash, u8 val)
{
    struct spi_transfer t;
    struct spi_message m;
    struct ftspi020_cmd cmd;

    pr_debug("%s: %s\n", dev_name(&flash->spi->dev), __func__);

    spi_message_init(&m);
    memset(&t, 0, (sizeof t));

    t.tx_buf = &cmd;
    spi_message_add_tail(&t, &m);

    memset(&cmd, 0, sizeof(struct ftspi020_cmd));
    cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
    cmd.write_en = FTSPI020_CMD3_WRITE;
    if (val)
        cmd.ins_code = FTSPI020_CMD3_INSTR_CODE(OPCODE_EN4B);
    else
        cmd.ins_code = FTSPI020_CMD3_INSTR_CODE(OPCODE_EX4B);
    cmd.flags = (FTSPI020_XFER_CMD_STATE | FTSPI020_XFER_CHECK_CMD_COMPLETE);

    spi_sync(flash->spi, &m);

    if (m.status < 0) {
        dev_err(&flash->spi->dev, "error %d write enable\n", (int)m.status);
        return m.status;
    }

    return 0;
}

/*
 * Set write enable latch with Write Enable command.
 * Returns negative if error occurred.
 */
static inline int write_enable(struct common_spi_flash *flash)
{
    struct spi_transfer t;
    struct spi_message m;
    struct ftspi020_cmd cmd;

    pr_debug("%s: %s\n", dev_name(&flash->spi->dev), __func__);

    spi_message_init(&m);
    memset(&t, 0, (sizeof t));

    t.tx_buf = &cmd;
    spi_message_add_tail(&t, &m);

    memset(&cmd, 0, sizeof(struct ftspi020_cmd));
    cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
    cmd.write_en = FTSPI020_CMD3_WRITE;
    cmd.ins_code = FTSPI020_CMD3_INSTR_CODE(OPCODE_WREN);
    cmd.flags = (FTSPI020_XFER_CMD_STATE | FTSPI020_XFER_CHECK_CMD_COMPLETE);

    spi_sync(flash->spi, &m);

    if (m.status < 0) {
        dev_err(&flash->spi->dev, "error %d write enable\n", (int)m.status);
        return m.status;
    }

    return 0;
}

/*
 * Service routine to read status register until ready, or timeout occurs.
 * Returns non-zero if error.
 */
static int wait_till_ready(struct common_spi_flash *flash, int wait_time)
{
	int sr = 0;
	unsigned long timeo = jiffies;

    timeo += (wait_time * HZ);
	/* one chip guarantees max 5 msec wait here after page writes,
	 * but potentially three seconds (!) after page erase.
	 */
	while (time_before(jiffies, timeo)){
		if ((sr = read_sr(flash, OPCODE_RDSR)) < 0)
			break;
		else if (!(sr & SR_WIP))
			return 0;
	}
	if (sr & SR_WIP)
        printk(KERN_ERR "SPI wait ready time out\n");
        
	return 1;
}

static inline int flash_set_quad_enable(struct common_spi_flash *flash)
{
    u8 sr[2];

    pr_debug("%s: %s\n", dev_name(&flash->spi->dev), __func__);

    /* Wait until finished previous write command. */
    if (wait_till_ready(flash, 3))
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

/*
 * Erase the whole flash memory
 *
 * Returns 0 if successful, non-zero otherwise.
 */
static int erase_chip(struct common_spi_flash *flash)
{
    struct spi_transfer t;
    struct spi_message m;
    struct ftspi020_cmd cmd;

    pr_debug("%s: %s %dKiB\n", dev_name(&flash->spi->dev), __func__, (int)(flash->mtd.size / 1024));

    /* Wait until finished previous write command. */
    if (wait_till_ready(flash, 3))
        return 1;

    /* Send write enable, then erase commands. */
    write_enable(flash);

    spi_message_init(&m);
    memset(&t, 0, (sizeof t));

    t.tx_buf = &cmd;
    spi_message_add_tail(&t, &m);

    /* Set up command buffer. */
    memset(&cmd, 0, sizeof(struct ftspi020_cmd));
    cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
    cmd.write_en = FTSPI020_CMD3_WRITE;
    cmd.ins_code = FTSPI020_CMD3_INSTR_CODE(OPCODE_CHIP_ERASE);
    cmd.flags = (FTSPI020_XFER_CMD_STATE | FTSPI020_XFER_CHECK_CMD_COMPLETE);

    spi_sync(flash->spi, &m);
    if (m.status < 0) {
        dev_err(&flash->spi->dev, "error %d erase chip\n", (int)m.status);
        return m.status;
    }

    return 0;
}

/*
 * Erase one sector of flash memory at offset ``offset'' which is any
 * address within the sector which should be erased.
 *
 * Returns 0 if successful, non-zero otherwise.
 */
static int erase_sector(struct common_spi_flash *flash, u32 offset)
{
    struct spi_transfer t;
    struct spi_message m;
    struct ftspi020_cmd cmd;
    int ret = 0;

    pr_debug("%s: %s %dKiB at 0x%08x\n",
             dev_name(&flash->spi->dev), __func__, flash->mtd.erasesize / 1024, offset);

    /* Wait until finished previous write command. */
    if (wait_till_ready(flash, 3))
        return 1;

    /* Enter 4 byte mode */
    //if (flash->mode_3_4 == 3)
    //    change_4b(flash, 1);

    /* Send write enable, then erase commands. */
    write_enable(flash);

    spi_message_init(&m);
    memset(&t, 0, (sizeof t));

    t.tx_buf = &cmd;
    spi_message_add_tail(&t, &m);

    /* Set up command buffer. */
    memset(&cmd, 0, sizeof(struct ftspi020_cmd));
    cmd.spi_addr = offset;
    if (flash->mode_3_4 == 3)
        cmd.addr_len = FTSPI020_CMD1_ADDR_3BYTE;
    else
        cmd.addr_len = FTSPI020_CMD1_ADDR_4BYTE;
    cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
    cmd.write_en = FTSPI020_CMD3_WRITE;
    cmd.ins_code = FTSPI020_CMD3_INSTR_CODE(flash->erase_opcode);
    cmd.flags = (FTSPI020_XFER_CMD_STATE | FTSPI020_XFER_CHECK_CMD_COMPLETE);

    spi_sync(flash->spi, &m);
    if (m.status < 0) {
        dev_err(&flash->spi->dev, "error %d erase sector\n", (int)m.status);
        ret = m.status;
    }
    /* Exit 4 byte mode */
    //if (flash->mode_3_4 == 3)
    //    change_4b(flash, 0);

    return 0;
}

/****************************************************************************/

/*
 * MTD implementation
 */

/*
 * Erase an address range on the flash chip.  The address range may extend
 * one or more erase sectors.  Return an error is there is a problem erasing.
 */
static int common_spi_flash_erase(struct mtd_info *mtd, struct erase_info *instr)
{
    struct common_spi_flash *flash = mtd_to_common_spi_flash(mtd);
    u32 addr, len;
    uint32_t rem;

    pr_debug("%s: %s %s 0x%08x, len %d\n",
             dev_name(&flash->spi->dev), __func__, "at", (u32) instr->addr, (int)instr->len);

    /* sanity checks */
    if (instr->addr + instr->len > flash->mtd.size)
        return -EINVAL;
    //if (((instr->addr % mtd->erasesize) != 0) || ((instr->len % mtd->erasesize) != 0)) {
    //    return -EINVAL;
    //}
    div_u64_rem(instr->len, mtd->erasesize, &rem);
    if (rem)
        return -EINVAL;

    addr = instr->addr;
    len = instr->len;

	mutex_lock(&flash->lock);

	if (addr != 0 || len != mtd->size) {
		while (len) {

			if (erase_sector(flash, addr)) {
				instr->state = MTD_ERASE_FAILED;
				mutex_unlock(&flash->lock);
				return -EIO;
			}

			addr += mtd->erasesize;
			len -= mtd->erasesize;
        	/* Wait until finished previous write command. */
        	if (wait_till_ready(flash, 5)) {
        	    printk(KERN_ERR "SPI wait previous erase command time out\n");
        		mutex_unlock(&flash->lock);
        		return -EIO;
        	}			
		}
	} else {
		if (erase_chip(flash)) {
			instr->state = MTD_ERASE_FAILED;
			mutex_unlock(&flash->lock);
			return -EIO;
		}
        if (wait_till_ready(flash, 500)) {
            printk(KERN_ERR "SPI wait chip erase command time out\n");
        	mutex_unlock(&flash->lock);
        	return -EIO;
        }		
    }
	mutex_unlock(&flash->lock);

    instr->state = MTD_ERASE_DONE;
    mtd_erase_callback(instr);

    return 0;
}

/*
 * Read an address range from the flash chip.  The address range
 * may be any size provided it is within the physical boundaries.
 */
static int common_spi_flash_read(struct mtd_info *mtd, loff_t from, size_t len,
                                 size_t * retlen, u_char * buf)
{
    struct common_spi_flash *flash = mtd_to_common_spi_flash(mtd);
    struct spi_transfer t;
    struct spi_message m;
    struct ftspi020_cmd cmd;
    int ret = 0;

    pr_debug("%s: %s %s 0x%08x, len %zd\n",
             dev_name(&flash->spi->dev), __func__, "from", (u32) from, len);
    /* sanity checks */
    if (!len)
        return 0;

    if (from + len > flash->mtd.size)
        return -EINVAL;

    /* Enter 4 byte mode */
    //if (flash->mode_3_4 == 3)
    //    change_4b(flash, 1);

    spi_message_init(&m);
    memset(&t, 0, (sizeof t));

    t.tx_buf = &cmd;
    t.bits_per_word = 8;
    spi_message_add_tail(&t, &m);

    /* NOTE:
     * OPCODE_FAST_READ (if available) is faster.
     * Should add 1 byte DUMMY_BYTE.
     */
    memset(&cmd, 0, sizeof(struct ftspi020_cmd));
    cmd.spi_addr = from;
    if (flash->mode_3_4 == 3)
        cmd.addr_len = FTSPI020_CMD1_ADDR_3BYTE;
    else
        cmd.addr_len = FTSPI020_CMD1_ADDR_4BYTE;

    cmd.dum_2nd_cyc = FTSPI020_CMD1_DUMMY_CYCLE(READ_DUMMY_CYCLE);
    cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
    cmd.write_en = FTSPI020_CMD3_READ;
	if(OPCODE_READ_COMMAND == OPCODE_FAST_READ_DUAL)
		cmd.ins_code = FTSPI020_CMD3_INSTR_CODE(OPCODE_READ_COMMAND) | spi_operate_dual_mode;
	else
    	cmd.ins_code = FTSPI020_CMD3_INSTR_CODE(OPCODE_READ_COMMAND);

    cmd.flags =
        (FTSPI020_XFER_CMD_STATE | FTSPI020_XFER_DATA_STATE | FTSPI020_XFER_CHECK_CMD_COMPLETE);
    cmd.rx_buf = buf;
    cmd.data_cnt = len;

    /* Byte count starts at zero. */
    if (retlen)
        *retlen = 0;

    mutex_lock(&flash->lock);

    /* Wait till previous write/erase is done. */
    if (wait_till_ready(flash, 2)) {
        /* REVISIT status return?? */
        mutex_unlock(&flash->lock);
        ret = 1;
        goto exit_read;
    }

    spi_sync(flash->spi, &m);
    if (m.status < 0) {
        dev_err(&flash->spi->dev, "error %d read\n", (int)m.status);
        mutex_unlock(&flash->lock);
        ret = m.status;
        goto exit_read;
    }

    *retlen = m.actual_length;

    mutex_unlock(&flash->lock);

exit_read:
    /* Exit 4 byte mode */
    //if (flash->mode_3_4 == 3)
    //    change_4b(flash, 0);

    return ret;

}

/*
 * Write an address range to the flash chip.  Data must be written in
 * FLASH_PAGESIZE chunks.  The address range may be any size provided
 * it is within the physical boundaries.
 */
static int common_spi_flash_write(struct mtd_info *mtd, loff_t to, size_t len,
                                  size_t * retlen, const u_char * buf)
{
    struct common_spi_flash *flash = mtd_to_common_spi_flash(mtd);
    u32 page_offset, page_size;
    struct spi_transfer t;
    struct spi_message m;
    struct ftspi020_cmd cmd;

    pr_debug("%s: %s %s 0x%08x, len %zd\n",
             dev_name(&flash->spi->dev), __func__, "to", (u32) to, len);

    if (retlen)
        *retlen = 0;

    /* sanity checks */
    if (!len)
        return (0);

    if (to + len > flash->mtd.size)
        return -EINVAL;

    spi_message_init(&m);
    memset(&t, 0, (sizeof t));

    t.tx_buf = &cmd;
    t.bits_per_word = 8;
    spi_message_add_tail(&t, &m);

    memset(&cmd, 0, sizeof(struct ftspi020_cmd));
    if (flash->mode_3_4 == 3)
        cmd.addr_len = FTSPI020_CMD1_ADDR_3BYTE;
    else
        cmd.addr_len = FTSPI020_CMD1_ADDR_4BYTE;
    cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
    cmd.write_en = FTSPI020_CMD3_WRITE;
#ifdef CONFIG_SPI_QUAD_MODE
    if (flash->flash_type == 1)
        cmd.ins_code = FTSPI020_CMD3_INSTR_CODE(OPCODE_WINBOND_PP);
    else
        cmd.ins_code = FTSPI020_CMD3_INSTR_CODE(OPCODE_QUAD_PP);
#else
    cmd.ins_code = FTSPI020_CMD3_INSTR_CODE(OPCODE_WRITE_COMMAND);
#endif
    cmd.flags =
        (FTSPI020_XFER_CMD_STATE | FTSPI020_XFER_DATA_STATE | FTSPI020_XFER_CHECK_CMD_COMPLETE);
    cmd.tx_buf = buf;

    mutex_lock(&flash->lock);

    /* Wait until finished previous write command. */
	if (wait_till_ready(flash, 3)) {
	    printk(KERN_ERR "SPI wait previous command time out\n");
        mutex_unlock(&flash->lock);
        return 1;
    }

    /* Enter 4 byte mode */
    //if (flash->mode_3_4 == 3)
    //    change_4b(flash, 1);

    write_enable(flash);

    /* Set up the opcode in the write buffer. */
    cmd.spi_addr = to;

    /* what page do we start with? */
    page_offset = to % FLASH_PAGESIZE;

    /* do all the bytes fit onto one page? */
    if (page_offset + len <= FLASH_PAGESIZE) {
        cmd.data_cnt = len;

        spi_sync(flash->spi, &m);

        if (retlen)
            *retlen = m.actual_length;
    } else {
        u32 i;

        /* the size of data remaining on the first page */
        page_size = FLASH_PAGESIZE - page_offset;

        cmd.data_cnt = page_size;
        spi_sync(flash->spi, &m);

        *retlen = m.actual_length;

        /* write everything in PAGESIZE chunks */
        for (i = page_size; i < len; i += page_size) {
            page_size = len - i;
            if (page_size > FLASH_PAGESIZE)
                page_size = FLASH_PAGESIZE;

            /* write the next page to flash */
            cmd.spi_addr = to + i;

            cmd.tx_buf = buf + i;
            cmd.data_cnt = page_size;

            if (wait_till_ready(flash, 3)) {
			    printk(KERN_ERR "SPI wait previous write command time out\n");
		        mutex_unlock(&flash->lock);
		        return 1;
            }
            write_enable(flash);

            spi_sync(flash->spi, &m);

            if (retlen)
                *retlen += m.actual_length;
        }
    }

    /* Exit 4 byte mode */
    //if (flash->mode_3_4 == 3)
    //    change_4b(flash, 0);

    mutex_unlock(&flash->lock);
    return 0;
}

/****************************************************************************/

/*
 * SPI device driver setup and teardown
 */

struct flash_info {
    char *name;

    /* JEDEC id zero means "no ID" (most older chips); otherwise it has
     * a high byte of zero plus three data bytes: the manufacturer id,
     * then a two byte device id.
     */
    u32 jedec_id;
    u16 ext_id;

    /* The size listed here is what works with OPCODE_SE, which isn't
     * necessarily called a "sector" by the vendor.
     */
    unsigned sector_size;
    u16 n_sectors;

    u16 flags;
#define	SECT_4K		0x01    /* OPCODE_SE works uniformly */
#define	SECT_64K	0x02    /* OPCODE_SE works uniformly */
    u8 mode_3_4;
    u8 flash_type;
};

/* NOTE: double check command sets and memory organization when you add
 * more flash chips.  This current list focusses on newer chips, which
 * have been converging on command sets which including JEDEC ID.
 */
static struct flash_info __devinitdata common_spi_flash_data[] = {
#ifdef CONFIG_MTD_ERASE_64K     //for erase performance, one time 64KB
    /* winbond */
    {"w25q32bv", 0xef4016, 0, 65536, 64, SECT_64K, 3, 1},
    {"w25X64V", 0xef3017, 0, 65536, 128, SECT_64K, 3, 1},
    {"w25Q64F", 0xef4017, 0, 65536, 128, SECT_64K, 3, 1},
    {"w25q128bv", 0xef4018, 0, 65536, 256, SECT_64K, 3, 1},
    {"w25q25xFV", 0xef4019, 0, 65536, 512, SECT_64K, 4, 1},
    /* mxic */
    {"MX25L64", 0xc22017, 0, 65536, 128, SECT_64K, 3, 2},    
    {"KH25L64", 0xc22617, 0, 65536, 128, SECT_64K, 3, 2}, 
    {"MX25L12845E", 0xc22018, 0, 65536, 256, SECT_64K, 3, 2},
    {"MX25L25x35E", 0xc22019, 0, 65536, 512, SECT_64K, 4, 2},
    /* eon */
    {"en25q128", 0x1c3018, 0, 65536, 256, SECT_64K, 3, 3},
    {"en25qh128a", 0x1c7018, 0, 65536, 256, SECT_64K, 3, 3},
    {"en25qh256", 0x1c7019, 0, 65536, 512, SECT_64K, 4, 3},
    /* esmt */
    {"w25Q64F", 0x8c4117, 0, 65536, 128, SECT_64K, 3, 3},
	/* Spansion */
	{"s25fl164k", 0x014017, 0, 65536, 128, SECT_64K, 3, 3},
	{"s25fl128s", 0x012018, 0, 65536, 256, SECT_64K, 3, 3},
	{"s25fl256s", 0x010219, 0, 65536, 512, SECT_64K, 4, 3},
	/* GD */
    {"gd25q64c", 0xc84017, 0, 65536, 128, SECT_64K, 3, 3},	
	{"gd25q128c", 0xc84018, 0, 65536, 256, SECT_64K, 3, 3},
	{"gd25q256c", 0xc84019, 0, 65536, 512, SECT_64K, 4, 3},
	/* Micron */
	{"N25q256", 0x20BA19, 0, 65536, 512, SECT_64K, 4, 4},
#else
    /* winbond */
    {"w25q32bv", 0xef4016, 0, 4096, 1024, SECT_4K, 3, 1},
    {"w25X64V", 0xef3017, 0, 4096, 2048, SECT_4K, 3, 1},
    {"w25Q64F", 0xef4017, 0, 4096, 2048, SECT_4K, 3, 1},
    {"w25q128bv", 0xef4018, 0, 4096, 4096, SECT_4K, 3, 1},
    /* mxic */
    {"MX25L12845E", 0xc22018, 0, 4096, 4096, SECT_4K, 3, 2},
    {"MX25L25635E", 0xc22019, 0, 4096, 8192, SECT_4K, 4, 2},
    /* eon */
    {"en25q128", 0x1c3018, 0, 4096, 4096, SECT_4K, 3, 3},
    {"en25qh256", 0x1c7019, 0, 4096, 8192, SECT_4K, 4, 3},
#endif
#if 0                           //ratbert
    /* Atmel -- some are (confusingly) marketed as "DataFlash" */
    {"at25fs010", 0x1f6601, 0, 32 * 1024, 4, SECT_4K,},
    {"at25fs040", 0x1f6604, 0, 64 * 1024, 8, SECT_4K,},

    {"at25df041a", 0x1f4401, 0, 64 * 1024, 8, SECT_4K,},
    {"at25df641", 0x1f4800, 0, 64 * 1024, 128, SECT_4K,},

    {"at26f004", 0x1f0400, 0, 64 * 1024, 8, SECT_4K,},
    {"at26df081a", 0x1f4501, 0, 64 * 1024, 16, SECT_4K,},
    {"at26df161a", 0x1f4601, 0, 64 * 1024, 32, SECT_4K,},
    {"at26df321", 0x1f4701, 0, 64 * 1024, 64, SECT_4K,},

    /* Spansion -- single (large) sector size only, at least
     * for the chips listed here (without boot sectors).
     */
    {"s25sl004a", 0x010212, 0, 64 * 1024, 8,},
    {"s25sl008a", 0x010213, 0, 64 * 1024, 16,},
    {"s25sl016a", 0x010214, 0, 64 * 1024, 32,},
    {"s25sl032a", 0x010215, 0, 64 * 1024, 64,},
    {"s25sl064a", 0x010216, 0, 64 * 1024, 128,},
    {"s25sl12800", 0x012018, 0x0300, 256 * 1024, 64,},
    {"s25sl12801", 0x012018, 0x0301, 64 * 1024, 256,},

    /* SST -- large erase sizes are "overlays", "sectors" are 4K */
    {"sst25vf040b", 0xbf258d, 0, 64 * 1024, 8, SECT_4K,},
    {"sst25vf080b", 0xbf258e, 0, 64 * 1024, 16, SECT_4K,},
    {"sst25vf016b", 0xbf2541, 0, 64 * 1024, 32, SECT_4K,},
    {"sst25vf032b", 0xbf254a, 0, 64 * 1024, 64, SECT_4K,},

    /* ST Microelectronics -- newer production may have feature updates */
    {"w25q05", 0x202010, 0, 32 * 1024, 2,},
    {"w25q10", 0x202011, 0, 32 * 1024, 4,},
    {"w25q20", 0x202012, 0, 64 * 1024, 4,},
    {"w25q40", 0x202013, 0, 64 * 1024, 8,},
    {"w25q80", 0, 0, 64 * 1024, 16,},
    {"w25q16", 0x202015, 0, 64 * 1024, 32,},
    {"w25q32", 0x202016, 0, 64 * 1024, 64,},
    {"w25q64", 0x202017, 0, 64 * 1024, 128,},
    {"w25q128", 0x202018, 0, 256 * 1024, 64,},

    {"m45pe80", 0x204014, 0, 64 * 1024, 16,},
    {"m45pe16", 0x204015, 0, 64 * 1024, 32,},

    {"w25qe80", 0x208014, 0, 64 * 1024, 16,},
    {"w25qe16", 0x208015, 0, 64 * 1024, 32, SECT_4K,},

    /* Winbond -- w25x "blocks" are 64K, "sectors" are 4KiB */
    {"w25x10", 0xef3011, 0, 64 * 1024, 2, SECT_4K,},
    {"w25x20", 0xef3012, 0, 64 * 1024, 4, SECT_4K,},
    {"w25x40", 0xef3013, 0, 64 * 1024, 8, SECT_4K,},
    {"w25x80", 0xef3014, 0, 64 * 1024, 16, SECT_4K,},
    {"w25x16", 0xef3015, 0, 64 * 1024, 32, SECT_4K,},
    {"w25x32", 0xef3016, 0, 64 * 1024, 64, SECT_4K,},
    {"w25x64", 0xef3017, 0, 64 * 1024, 128, SECT_4K,},
    {"w25p16", 0xef2015, 0, 64 * 1024, 32,},    //ratbert
#endif
};

static struct flash_info *__devinit jedec_probe(struct spi_device *spi)
{
    u8 id[4];
    u32 jedec;
    u32 tmp;
    struct flash_info *info;
    struct ftspi020_cmd spi_cmd;
    struct spi_transfer t;
    struct spi_message m;

    pr_debug("%s: %s\n", dev_name(&spi->dev), __func__);

    spi_message_init(&m);
    memset(&t, 0, (sizeof t));

    t.tx_buf = &spi_cmd;
    t.bits_per_word = 8;
    spi_message_add_tail(&t, &m);

    memset(&spi_cmd, 0, sizeof(struct ftspi020_cmd));
    spi_cmd.flags =
        (FTSPI020_XFER_CMD_STATE | FTSPI020_XFER_DATA_STATE | FTSPI020_XFER_CHECK_CMD_COMPLETE);
    spi_cmd.ins_len = FTSPI020_CMD1_OP_CODE_1_BYTE;
    spi_cmd.data_cnt = 4;
    spi_cmd.ins_code = FTSPI020_CMD3_INSTR_CODE(OPCODE_RDID);
    spi_cmd.write_en = FTSPI020_CMD3_READ;

    spi_cmd.rx_buf = &id;
    spi_sync(spi, &m);

    if (m.status < 0) {
        printk(KERN_ERR "%s: error %d reading JEDEC ID\n", dev_name(&spi->dev), m.actual_length);
        return NULL;
    }

    jedec = id[0];
    jedec = jedec << 8;
    jedec |= id[1];
    jedec = jedec << 8;
    jedec |= id[2];

    pr_debug("SPI flash id[0] = %x, id[1] = %x, id[2] = %x\n", id[0], id[1], id[2]);
    for (tmp = 0, info = common_spi_flash_data;
         tmp < ARRAY_SIZE(common_spi_flash_data); tmp++, info++) {
        if (info->jedec_id == jedec)
            return info;
    }

    dev_err(&spi->dev, "unrecognized JEDEC id %06x\n", jedec);
    return NULL;
}

static int partition_check(struct mtd_partition *partitions, sys_header_t *sys_hdr, int page_size, int chip_size)
{
    int i, num = 0;
    int j, A_begin, A_end, B_begin, B_end;

	//block alignment check
    for(i = 0; i < ARRAY_SIZE(sys_hdr->image); i++){
        
        if(sys_hdr->image[i].size == 0)
    	    continue;

        if(sys_hdr->image[i].addr % page_size){
    	    printk(KERN_WARNING "Warning............partition %d addr 0x%x not block alignment, one block = 0x%x\n", i, sys_hdr->image[i].addr, page_size);
    	    partitions[num].offset = BLOCK_ALIGN(sys_hdr->image[i].addr, page_size);
        } else {
       	    partitions[num].offset = sys_hdr->image[i].addr;
       	}

        if(sys_hdr->image[i].size % page_size){
    	    printk(KERN_WARNING "Warning............partition %d size 0x%x not block alignment, one block = 0x%x\n", i, sys_hdr->image[i].size, page_size);
    	    partitions[num].size = BLOCK_ALIGN(sys_hdr->image[i].size, page_size);
        } else {
      	    partitions[num].size = sys_hdr->image[i].size;
      	}

        partitions[num].name = sys_hdr->image[i].name;
	//printk("p[%d] offset = 0x%x, size = 0x%x, name = %s\n", num, (u32)partitions[num].offset, (u32)partitions[num].size, partitions[num].name);
        num++;
    }

    //overlap check
    for(i = 0; i < (num - 1); i++){
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
    
#ifdef CONFIG_PLATFORM_GM8210
    if(sys_hdr->addr != 0) {
        partitions[num].offset = sys_hdr->addr;
        partitions[num].size = sys_hdr->size;
        partitions[num].name = sys_hdr->name;
        num++;        
    }
#endif 
    
    /* total flash map to last MTD
     */
    partitions[num].offset = 0;
    partitions[num].size = chip_size;

    partitions[num].name = "ALL";
    num++;
    return num;
    
check_fail:
    printk(KERN_WARNING "Warning ============> partition %d overlap with %d\n", i, j);
    return num;
}

/*
 * board specific setup should have ensured the SPI clock used here
 * matches what the READ command supports, at least until this driver
 * understands FAST_READ (for clocks over 25 MHz).
 */
static int __devinit common_spi_flash_probe(struct spi_device *spi)
{
    struct flash_platform_data *data;
    struct common_spi_flash *flash;
    struct flash_info *info;
    struct mtd_partition *parts = NULL;
    int nr_parts = 0;
    size_t retlen[1], i;

    pr_debug("%s: %s\n", dev_name(&spi->dev), __func__);

    /* Platform data helps sort out which chip type we have, as
     * well as how this board partitions it.  If we don't have
     * a chip ID, try the JEDEC id commands; they'll work for most
     * newer chips, even if we don't recognize the particular chip.
     */
    data = spi->dev.platform_data;
    if (data && data->type) {
        for (i = 0, info = common_spi_flash_data;
             i < ARRAY_SIZE(common_spi_flash_data); i++, info++) {
            if (strcmp(data->type, info->name) == 0)
                break;
        }

        /* unrecognized chip? */
        if (i == ARRAY_SIZE(common_spi_flash_data)) {
            printk(KERN_ERR "%s: unrecognized id %s\n", dev_name(&spi->dev), data->type);
            info = NULL;

            /* recognized; is that chip really what's there? */
        } else if (info->jedec_id) {
            struct flash_info *chip = jedec_probe(spi);

            if (!chip || chip != info) {
                dev_warn(&spi->dev, "found %s, expected %s\n",
                         chip ? chip->name : "UNKNOWN", info->name);
                info = NULL;
            }
        }
    } else
        info = jedec_probe(spi);

    if (!info)
        return -ENODEV;

    flash = kzalloc(sizeof *flash, GFP_KERNEL);
    if (!flash)
        return -ENOMEM;

    flash->spi = spi;
    mutex_init(&flash->lock);
    dev_set_drvdata(&spi->dev, flash);

    if (data && data->name)
        flash->mtd.name = "nor-flash";//u-boot commandline
    else
        flash->mtd.name = dev_name(&spi->dev);

    flash->mtd.type = MTD_NORFLASH;
    flash->mtd.writesize = 1;
    flash->mtd.flags = MTD_CAP_NORFLASH;
    flash->mtd.size = info->sector_size * info->n_sectors;
    flash->mtd.erase = common_spi_flash_erase;
    flash->mtd.read = common_spi_flash_read;
    flash->mtd.write = common_spi_flash_write;
    flash->mode_3_4 = info->mode_3_4;
    flash->flash_type = info->flash_type;

    if (flash->mode_3_4 == 3){
        OPCODE_READ_COMMAND = OPCODE_FAST_READ_DUAL;//0x0B
        OPCODE_WRITE_COMMAND = 0x02;
        OPCODE_ERASE_COMMAND = 0xD8;
    } else {
        switch (flash->flash_type){
            case 1:	//winbond
                OPCODE_READ_COMMAND = OPCODE_FAST_READ_DUAL;//0x0B
                OPCODE_WRITE_COMMAND = 0x02;
                OPCODE_ERASE_COMMAND = 0xD8;
                break;
            case 2:	//MXIC
#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)                
                if (platform_spi_four_byte_mode()) {	//3 byte mode
                    OPCODE_READ_COMMAND = OPCODE_FAST_READ_DUAL;//0x0B
                    OPCODE_WRITE_COMMAND = 0x02;
                    OPCODE_ERASE_COMMAND = 0xD8;
                } else {	//4 byte mode
                    OPCODE_READ_COMMAND = 0x0C;//0x3C
                    OPCODE_WRITE_COMMAND = 0x12;
                    OPCODE_ERASE_COMMAND = 0xDC;
                }
                break;
#else
                OPCODE_READ_COMMAND = 0x0B;
                OPCODE_WRITE_COMMAND = 0x02;
                OPCODE_ERASE_COMMAND = 0xD8;                
#endif                
            case 3:	//EON, ESMT, spansion, GD
                OPCODE_READ_COMMAND = 0x0B;
                OPCODE_WRITE_COMMAND = 0x02;
                OPCODE_ERASE_COMMAND = 0xD8;
                break;                
            case 4:	//Micron
                OPCODE_READ_COMMAND = 0x0C;
                OPCODE_WRITE_COMMAND = 0x12;
                OPCODE_ERASE_COMMAND = 0xD8;
                break; 
            default:
                OPCODE_READ_COMMAND = 0x0B;
                OPCODE_WRITE_COMMAND = 0x02;
                OPCODE_ERASE_COMMAND = 0xD8;
                break;
        }
        change_4b(flash, 1);
    }
#ifdef CONFIG_SPI_QUAD_MODE
    flash_set_quad_enable(flash);
#endif

    /* prefer "small sector" erase if possible */
    if (info->flags & SECT_4K) {
        flash->erase_opcode = OPCODE_SE;
        flash->mtd.erasesize = 4096;
        printk(KERN_INFO "ERASE SECTOR 4K\n");
    } else if (info->flags & SECT_64K) {
        flash->erase_opcode = OPCODE_ERASE_COMMAND;
        flash->mtd.erasesize = 64 * 1024;
        printk(KERN_INFO "ERASE SECTOR 64K\n");
    } else {
        flash->erase_opcode = OPCODE_CHIP_ERASE;
        flash->mtd.erasesize = flash->mtd.size;
    }

    dev_info(&spi->dev, "%s (%d Kbytes)\n", info->name, (int)(flash->mtd.size / 1024));

    pr_debug("mtd .name = %s, .size = 0x%.8x (%uMiB) "
             ".erasesize = 0x%.8x (%uKiB) .numeraseregions = %d\n",
             flash->mtd.name,
             (int)flash->mtd.size, (int)(flash->mtd.size / (1024 * 1024)),
             (int)flash->mtd.erasesize, (int)(flash->mtd.erasesize / 1024),
             flash->mtd.numeraseregions);

    if (flash->mtd.numeraseregions)
        for (i = 0; i < flash->mtd.numeraseregions; i++)
            pr_debug("mtd.eraseregions[%d] = { .offset = 0x%.8x, "
                     ".erasesize = 0x%.8x (%uKiB), "
                     ".numblocks = %d }\n",
                     i, (u32) flash->mtd.eraseregions[i].offset,
                     (int)flash->mtd.eraseregions[i].erasesize,
                     (int)(flash->mtd.eraseregions[i].erasesize / 1024),
                     flash->mtd.eraseregions[i].numblocks);

    /* partitions should match sector boundaries; and it may be good to
     * use readonly partitions for writeprotected sectors (BP2..BP0).
     */

    /* read system header
     */
    flash->sys_hdr = kzalloc(sizeof(struct sys_header), GFP_KERNEL);

    if (flash->sys_hdr == NULL) {
        printk(KERN_ERR "Warning............SPI: can't alloc memory");
        return -ENOMEM;
    }

    if (common_spi_flash_read
        (&flash->mtd, 0x0, sizeof(struct sys_header), retlen, (u_char *) flash->sys_hdr) < 0)
        printk(KERN_ERR "SPI: read system header fail!");

    //printk("addr=%x, size=%x\n",flash->sys_hdr->image[0].addr, flash->sys_hdr->image[0].size);//???
    if (flash->sys_hdr->image[0].size == 0) {
        printk(KERN_WARNING "Not find partition message, use default setting\n");

        flash->sys_hdr->image[0].name[0] = 'a'; //???
        flash->sys_hdr->image[0].addr = 0x20000;
        flash->sys_hdr->image[0].size = 0x30000;

        flash->sys_hdr->image[1].name[0] = 'u';
        flash->sys_hdr->image[1].addr = 0x700000;
        flash->sys_hdr->image[1].size = 0x100000;

        flash->sys_hdr->image[2].name[0] = 'r';
        flash->sys_hdr->image[2].addr = 0x00000;
        flash->sys_hdr->image[2].size = 0x10000;

        for (i = 3; i < 10; i++) {
            flash->sys_hdr->image[i].addr = 0;
            flash->sys_hdr->image[i].size = 0;
        }
    }
    nr_parts =
        partition_check(ftspi020_partition_info, flash->sys_hdr, sizeof(struct sys_header),
                        flash->mtd.size);
    parts = ftspi020_partition_info;

    if (nr_parts <= 0 && data && data->parts) {
        parts = data->parts;
        nr_parts = data->nr_parts;
    }

    if (nr_parts > 0) {
        for (i = 0; i < nr_parts; i++) {
            pr_debug("partitions[%d] = "
                     "{.name = %s, .offset = 0x%.8x, "
                     ".size = 0x%.8x (%uKiB) }\n",
                     i, parts[i].name,
                     (u32) parts[i].offset, (int)parts[i].size, (int)(parts[i].size / 1024));
        }
    }
    return mtd_device_parse_register(&flash->mtd, part_probes, 0, parts, nr_parts);
}

static int __devexit common_spi_flash_remove(struct spi_device *spi)
{
    struct common_spi_flash *flash = dev_get_drvdata(&spi->dev);
    int status;

    /* Clean up MTD stuff. */
    status = mtd_device_unregister(&flash->mtd);
    if (status == 0) {
        kfree(flash->sys_hdr);
        kfree(flash);
    }
    return 0;
}

static struct spi_driver common_spi_flash_driver = {
    .driver = {
               .name = SPI_NAME,
               .bus = &spi_bus_type,
               .owner = THIS_MODULE,
               },
    .probe = common_spi_flash_probe,
    .remove = __devexit_p(common_spi_flash_remove),

    /* REVISIT: many of these chips have deep power-down modes, which
     * should clearly be entered on suspend() to minimize power use.
     * And also when they're otherwise idle...
     */
};

static int __init common_spi_flash_init(void)
{
    /* add spi device here, when add corresponding spi driver, they will bind together
     */
    spi_register_board_info(spi_devs_info, ARRAY_SIZE(spi_devs_info));

    return spi_register_driver(&common_spi_flash_driver);
}

static void common_spi_flash_exit(void)
{
    spi_unregister_driver(&common_spi_flash_driver);
}

module_init(common_spi_flash_init);
module_exit(common_spi_flash_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mike Lavender");
MODULE_DESCRIPTION("MTD SPI driver for SPI flash chips");
