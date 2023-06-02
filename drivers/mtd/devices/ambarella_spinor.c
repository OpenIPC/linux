/*
 * drivers/mtd/ambarella_spinor.c
 *
 * History:
 *    2014/03/10 - [cddiao] created file
 *
 * Copyright (C) 2014-2019, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "spinor.h"

/* Flash opcodes. */
#define    OPCODE_WREN        0x06    /* Write enable */
#define    OPCODE_RDSR        0x05    /* Read status register */
#define    OPCODE_WRSR        0x01    /* Write status register 1 byte */
#define    OPCODE_NORM_READ   0x03    /* Read data bytes (low frequency) */
#define    OPCODE_FAST_READ   0x0b    /* Read data bytes (high frequency) */
#define    OPCODE_PP          0x02    /* Page program (up to 256 bytes) */
#define    OPCODE_BE_4K       0x20    /* Erase 4KiB block */
#define    OPCODE_BE_32K      0x52    /* Erase 32KiB block */
#define    OPCODE_CHIP_ERASE  0xc7    /* Erase whole flash chip */
#define    OPCODE_SE          0xd8    /* Sector erase (usually 64KiB) */
#define    OPCODE_RDID        0x9f    /* Read JEDEC ID */

/* Used for SST flashes only. */
#define    OPCODE_BP          0x02    /* Byte program */
#define    OPCODE_WRDI        0x04    /* Write disable */
#define    OPCODE_AAI_WP      0xad    /* Auto address increment word program */

/* Used for Macronix flashes only. */
#define    OPCODE_EN4B        0xb7    /* Enter 4-byte mode */
#define    OPCODE_EX4B        0xe9    /* Exit 4-byte mode */

/* Used for Spansion flashes only. */
#define    OPCODE_BRWR        0x17    /* Bank register write */

/* Status Register bits. */
#define    SR_WIP            1    /* Write in progress */
#define    SR_WEL            2    /* Write enable latch */
/* meaning of other SR_* bits may differ between vendors */
#define    SR_BP0            4    /* Block protect 0 */
#define    SR_BP1            8    /* Block protect 1 */
#define    SR_BP2            0x10    /* Block protect 2 */
#define    SR_SRWD           0x80    /* SR write protect */

/* Define max times to check status register before we give up. */
#define    MAX_READY_WAIT_JIFFIES    (400 * HZ)    /* 40s max chip erase */
#define    MAX_CMD_SIZE        5

#define JEDEC_MFR(_jedec_id)    ((_jedec_id) >> 16)

#define PART_DEV_SPINOR		(0x08)
static u16 spi_addr_mode = 3;

/****************************************************************************/

static inline struct amb_norflash *mtd_to_amb(struct mtd_info *mtd)
{
    return container_of(mtd, struct amb_norflash, mtd);
}

/****************************************************************************/

static int read_sr(struct amb_norflash *flash)
{
    ssize_t retval = 0;
    u8 code = OPCODE_RDSR;
    u8 val;
    retval = ambspi_read_reg(flash, 1, code, &val);

    if (retval < 0) {
        dev_err((const struct device *)&flash->dev, "error %d reading SR\n",
                (int) retval);
        return retval;
    }

    return val;
}

/*
 * Set write enable latch with Write Enable command.
 * Returns negative if error occurred.
 */
int write_enable(struct amb_norflash *flash)
{
    u8 code = OPCODE_WREN;
    return ambspi_send_cmd(flash, code, 0, 0, 0);
}

/*
 * Send write disble instruction to the chip.
 */
static inline int write_disable(struct amb_norflash *flash)
{
    u8 code = OPCODE_WRDI;
    return ambspi_send_cmd(flash, code, 0, 0, 0);
}

/*
 * Enable/disable 4-byte addressing mode.
 */
static inline int set_4byte(struct amb_norflash *flash, u32 jedec_id, int enable)
{
	int ret = 0;

	switch (JEDEC_MFR(jedec_id)) {
		case CFI_MFR_MACRONIX:
		case 0xEF: /* winbond */
		case 0xC8: /* GD */
			flash->command[0] = enable ? OPCODE_EN4B : OPCODE_EX4B;
			return ambspi_send_cmd(flash, flash->command[0], 0, 0, 0);
		case CFI_MFR_ST: /*Micron*/
			write_enable(flash);
			flash->command[0] = enable ? OPCODE_EN4B : OPCODE_EX4B;
			ret = ambspi_send_cmd(flash, flash->command[0], 0, 0, 0);
			write_disable(flash);
			return ret;
		default:
			/* Spansion style */
			flash->command[0] = OPCODE_BRWR;
			flash->command[1] = enable << 7;
			return ambspi_send_cmd(flash, flash->command[0], 0, flash->command[1], 1);
	}
}
/*
mode 1 - enter 4 byte spi addr mode
	 0 - exit  4 byte spi addr mode
*/
static void check_set_spinor_addr_mode(struct amb_norflash *flash, int mode)
{
	if (flash->addr_width == 4) {
		if (spi_addr_mode == 3 && mode){
			set_4byte(flash, flash->jedec_id, 1);
			spi_addr_mode = 4;
		} else if (spi_addr_mode == 4 && mode == 0){
			set_4byte(flash, flash->jedec_id, 0);
			spi_addr_mode = 3;
		}
	}
}
/*
 * Service routine to read status register until ready, or timeout occurs.
 * Returns non-zero if error.
 */
int wait_till_ready(struct amb_norflash *flash)
{
    unsigned long deadline;
    int sr;

    deadline = jiffies + MAX_READY_WAIT_JIFFIES;

    do {
        if ((sr = read_sr(flash)) < 0)
            break;
        else if (!(sr & SR_WIP))
            return 0;

        cond_resched();

    } while (!time_after_eq(jiffies, deadline));

    return 1;
}

/*
 * Erase the whole flash memory
 *
 * Returns 0 if successful, non-zero otherwise.
 */
static int erase_chip(struct amb_norflash *flash)
{
    /* Wait until finished previous write command. */
    if (wait_till_ready(flash))
        return 1;

    /* Send write enable, then erase commands. */
    write_enable(flash);

    /* Set up command buffer. */
    flash->command[0] = OPCODE_CHIP_ERASE;
    ambspi_send_cmd(flash, flash->command[0], 0, 0, 0);

    return 0;
}


/*
 * Erase one sector of flash memory at offset ``offset'' which is any
 * address within the sector which should be erased.
 *
 * Returns 0 if successful, non-zero otherwise.
 */
static int erase_sector(struct amb_norflash *flash, u32 offset)
{

    /* Wait until finished previous write command. */
    if (wait_till_ready(flash))
        return 1;

    /* Send write enable, then erase commands. */
    write_enable(flash);

    /* Set up command buffer. */
    flash->command[0] = flash->erase_opcode;
    ambspi_send_cmd(flash, flash->command[0], 0, offset, flash->addr_width);

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
static int amba_erase(struct mtd_info *mtd, struct erase_info *instr)
{
    struct amb_norflash *flash = mtd_to_amb(mtd);
    u32 addr, len;
    uint32_t rem;

    div_u64_rem(instr->len, mtd->erasesize, &rem);
    if (rem)
        return -EINVAL;

    addr = instr->addr;
    len = instr->len;

    mutex_lock(&flash->lock);
	check_set_spinor_addr_mode(flash, 1);
    /* whole-chip erase? */
    if (len == flash->mtd.size) {
        if (erase_chip(flash)) {
            instr->state = MTD_ERASE_FAILED;
			check_set_spinor_addr_mode(flash, 0);
            mutex_unlock(&flash->lock);
            return -EIO;
        }

    /* REVISIT in some cases we could speed up erasing large regions
     * by using OPCODE_SE instead of OPCODE_BE_4K.  We may have set up
     * to use "small sector erase", but that's not always optimal.
     */

    /* "sector"-at-a-time erase */
    } else {
        while (len) {
            if (erase_sector(flash, addr)) {
                instr->state = MTD_ERASE_FAILED;
				check_set_spinor_addr_mode(flash, 0);
                mutex_unlock(&flash->lock);
				return -EIO;
            }

            addr += mtd->erasesize;
            len -= mtd->erasesize;
        }
    }
	check_set_spinor_addr_mode(flash, 0);
    mutex_unlock(&flash->lock);

    instr->state = MTD_ERASE_DONE;
    mtd_erase_callback(instr);

    return 0;
}

/*
 * Read an address range from the flash chip.  The address range
 * may be any size provided it is within the physical boundaries.
 */
 static int amba_read(struct mtd_info *mtd, loff_t from, size_t len,
    size_t *retlen, u_char *buf)
{
    struct amb_norflash *flash = mtd_to_amb(mtd);
    uint8_t opcode;
    int offset;
    int needread = 0;
    int ret=0;

    mutex_lock(&flash->lock);

    /* Wait till previous write/erase is done. */
    if (wait_till_ready(flash)) {
        /* REVISIT status return?? */
        mutex_unlock(&flash->lock);
        return 1;
    }
	check_set_spinor_addr_mode(flash, 1);
    /* Set up the write data buffer. */
    opcode = flash->fast_read ? OPCODE_FAST_READ : OPCODE_NORM_READ;
    flash->command[0] = opcode;
    flash->dummy = 0;

    for (offset = 0; offset < len; ) {
        needread = len - offset;
        if (needread >= AMBA_SPINOR_DMA_BUFF_SIZE) {
            ret = ambspi_read_data(flash, from+offset, AMBA_SPINOR_DMA_BUFF_SIZE);
            if(ret) {
				dev_err((const struct device *)&flash->dev,
						"SPI NOR read error from=%x len=%d\r\n", (u32)(from+offset), AMBA_SPINOR_DMA_BUFF_SIZE);
				check_set_spinor_addr_mode(flash, 0);
				mutex_unlock(&flash->lock);
				return -1;
            }
			memcpy(buf+offset, flash->dmabuf, AMBA_SPINOR_DMA_BUFF_SIZE);
			offset += AMBA_SPINOR_DMA_BUFF_SIZE;
        }else{
			ret = ambspi_read_data(flash, from+offset, needread);
			if(ret) {
				dev_err((const struct device *)&flash->dev,
						"SPI NOR read error from=%x len=%d\r\n", (u32)(from+offset), needread);
				check_set_spinor_addr_mode(flash, 0);
				mutex_unlock(&flash->lock);
				return -1;
            }
            memcpy(buf+offset, flash->dmabuf, needread);
            offset += needread;
        }
    }
    *retlen = offset;
	check_set_spinor_addr_mode(flash, 0);
    mutex_unlock(&flash->lock);
    return 0;
}

/*
 * Write an address range to the flash chip.  Data must be written in
 * FLASH_PAGESIZE chunks.  The address range may be any size provided
 * it is within the physical boundaries.
 */
static int amba_write(struct mtd_info *mtd, loff_t to, size_t len,
    size_t *retlen, const u_char *buf)
{
    struct amb_norflash *flash = mtd_to_amb(mtd);
    u32 page_offset, needwrite, needcopy, page_write;
    mutex_lock(&flash->lock);

    /* Wait until finished previous write command. */
    if (wait_till_ready(flash)) {
        mutex_unlock(&flash->lock);
        return 1;
    }
	check_set_spinor_addr_mode(flash, 1);
    write_enable(flash);

    /* Set up the opcode in the write buffer. */
    flash->command[0] = OPCODE_PP;
    flash->dummy = 0;
    page_offset = to & (flash->page_size - 1);
    /* do all the bytes fit onto one page? */
    if (page_offset + len <= flash->page_size) {
        memcpy(flash->dmabuf, buf, len);
        ambspi_prog_data(flash, 0, to, len);
        *retlen = len;
    } else {
        u32 i;
        u32 j;

        /* the size of data remaining on the first page */
        needwrite = flash->page_size - page_offset;
        memcpy(flash->dmabuf, buf, needwrite);
        ambspi_prog_data(flash, 0, to, needwrite);
        *retlen = needwrite;
        for (j=needwrite; j < len; j += needcopy) {
            if (len-j < AMBA_SPINOR_DMA_BUFF_SIZE) {
                needcopy = len-j;
            }else{
                needcopy = AMBA_SPINOR_DMA_BUFF_SIZE;
            }
            memcpy(flash->dmabuf, buf+j, needcopy);

            for (i = 0; i < needcopy; i += page_write) {
                page_write = needcopy - i;
                if (page_write > flash->page_size) {
                    page_write = flash->page_size;
                }
                /* write the next page to flash */
                if (wait_till_ready(flash)) {
					check_set_spinor_addr_mode(flash, 0);
                    mutex_unlock(&flash->lock);
                    return 1;
                }
                write_enable(flash);
                ambspi_prog_data(flash, i, to+i+j, page_write);
                *retlen += page_write;
            }
        }
    }
	check_set_spinor_addr_mode(flash, 0);
    mutex_unlock(&flash->lock);
    return 0;
}


/****************************************************************************/
struct flash_info {
    /* JEDEC id zero means "no ID" (most older chips);otherwise it has
     * a high byte of zero plus three data bytes: the manufacturer id,
     * then a two byte device id.
     */
    u32        jedec_id;
    u16        ext_id;

    /* The size listed here is what works with OPCODE_SE, which isn't
     * necessarily called a "sector" by the vendor.
     */
    unsigned    sector_size;
    u16         n_sectors;

    u16        page_size;
    u16        addr_width;

	u32			max_clk_hz;
    u16        flags;
#define    SECT_4K        0x01        /* OPCODE_BE_4K works uniformly */
};


#define INFO(_jedec_id, _ext_id, _sector_size, _n_sectors, _page_size, _max_clk_hz, _flags)    \
    {                                                                 \
        .jedec_id = (_jedec_id),                                      \
        .ext_id = (_ext_id),                                          \
        .sector_size = (_sector_size),                                \
        .n_sectors = (_n_sectors),                                    \
        .page_size = (_page_size),                                      \
		.max_clk_hz = (_max_clk_hz),                                    \
        .flags = (_flags), }


struct ambid_t {
    char name[32];
    struct flash_info driver_data;
};

static const struct ambid_t amb_ids[] = {
    { "s70fl01gs", INFO(0x010220, 0x4d00, 256 * 1024, 256, 512, 50000000, 0) },
	{ "n25q256a", INFO(0x20ba19, 0, 64 * 1024, 512, 256, 50000000, 0) },
	{ "mx25l25645g", INFO(0xc22019, 0, 64 * 1024, 512, 256, 50000000, 0) },
	{ "mx66l51235f", INFO(0xc2201a, 0, 64 * 1024, 1024, 256, 50000000, 0) },
	{ "w25q128", INFO(0xef4018, 0, 64 * 1024, 256, 256, 50000000, 0) },
	{ "w25q256", INFO(0xef4019, 0, 64 * 1024, 512, 256, 50000000, 0) },
	{ "gd25q128", INFO(0xc84018, 0, 64 * 1024, 256, 256, 50000000, 0) },
	{ "gd25q256c", INFO(0xc84019, 0, 64 * 1024, 512, 256, 50000000, 0) },
	{ "gd25q512", INFO(0xc84020, 0, 64 * 1024, 1024, 256, 50000000, 0) },
    { },
};

static const struct ambid_t *jedec_probe(struct amb_norflash *flash)
{
    int	retval;
    u8	code = OPCODE_RDID;
    u8	id[5];
    u32	jedec;
    u16	ext_jedec;
    struct flash_info	*info;
	int		tmp;

    /* JEDEC also defines an optional "extended device information"
     * string for after vendor-specific data, after the three bytes
     * we use here.  Supporting some chips might require using it.
     */
    retval = ambspi_read_reg(flash, 5, code, id);
	if (retval < 0) {
        dev_err((const struct device *)&flash->dev, "error %d reading id\n",
                (int) retval);
        return ERR_PTR(retval);
    }
    jedec = id[0];
    jedec = jedec << 8;
    jedec |= id[1];
    jedec = jedec << 8;
    jedec |= id[2];

    ext_jedec = id[3] << 8 | id[4];

    for (tmp = 0; tmp < ARRAY_SIZE(amb_ids) - 1; tmp++) {
        info = (void *)&amb_ids[tmp].driver_data;
        if (info->jedec_id == jedec) {
            if (info->ext_id != 0 && info->ext_id != ext_jedec)
                continue;
            return &amb_ids[tmp];
        }
    }
    dev_err((const struct device *)&flash->dev, "unrecognized JEDEC id %06x\n", jedec);
    return ERR_PTR(-ENODEV);
}

static int amb_get_resource(struct amb_norflash    *flash, struct platform_device *pdev)
{
    struct resource *res;
    int errorCode = 0;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        dev_err(&pdev->dev, "No mem resource for spinor_reg!\n");
        errorCode = -ENXIO;
        goto spinor_get_resource_err_exit;
    }

    flash->regbase =
        devm_ioremap(&pdev->dev, res->start, resource_size(res));
    if (!flash->regbase) {
        dev_err(&pdev->dev, "devm_ioremap() failed\n");
        errorCode = -ENOMEM;
        goto spinor_get_resource_err_exit;
    }

    res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    if (!res) {
        dev_err(&pdev->dev, "No mem resource for spinor_reg!\n");
        errorCode = -ENXIO;
        goto spinor_get_resource_err_exit;
    }

    flash->dmaregbase =
        devm_ioremap(&pdev->dev, res->start, resource_size(res));
    if (!flash->dmaregbase) {
        dev_err(&pdev->dev, "devm_ioremap() failed\n");
        errorCode = -ENOMEM;
        goto spinor_get_resource_err_exit;
    }

    return 0;

spinor_get_resource_err_exit:
    return errorCode;
}


static int    amb_spi_nor_probe(struct platform_device *pdev)
{
	struct mtd_part_parser_data	ppdata;
	struct flash_platform_data    data;
	struct amb_norflash            *flash;
	struct flash_info        *info;
	unsigned            i;
	int errCode = 0;
	const struct ambid_t *ambid = NULL;;

    flash = kzalloc(sizeof(struct amb_norflash), GFP_KERNEL);
    if (!flash) {
        errCode = -ENOMEM;
        goto amb_spi_nor_probe_exit;
    }

    mutex_init(&flash->lock);
    platform_set_drvdata(pdev, flash);
    flash->dev = &pdev->dev;

	/* set 50Mhz as default spi clock */
	flash->clk = 50000000;
    amb_get_resource(flash, pdev);
    ambspi_init(flash);

	ambid = jedec_probe(flash);
	if (IS_ERR(ambid))
			return PTR_ERR(ambid);
	else {
		info = (void *)&ambid->driver_data;
	}
	data.type = kzalloc(sizeof(32), GFP_KERNEL);
    strcpy(data.type, ambid->name);

	flash->mtd.name = "amba_spinor";
    flash->mtd.type = MTD_NORFLASH;
    flash->mtd.writesize = 1;
    flash->mtd.flags = MTD_CAP_NORFLASH;
    flash->mtd.size = info->sector_size * info->n_sectors;
    flash->mtd._erase = amba_erase;
    flash->mtd._read = amba_read;
    flash->mtd._write = amba_write;
    flash->write_enable = write_enable;
    flash->wait_till_ready = wait_till_ready;

    /* prefer "small sector" erase if possible */
    if (info->flags & SECT_4K) {
        flash->erase_opcode = OPCODE_BE_4K;
        flash->mtd.erasesize = 4096;
    } else {
        flash->erase_opcode = OPCODE_SE;
        flash->mtd.erasesize = info->sector_size;
    }

    flash->mtd.flags |= MTD_NO_ERASE;

    flash->page_size = info->page_size;
    flash->mtd.writebufsize = flash->page_size;

    flash->fast_read = false;
    flash->command = kzalloc(5, GFP_KERNEL);
    if(!flash->command) {
        dev_err((const struct device *)&flash->dev,
            "SPI NOR driver malloc command error\r\n");
        errCode = -ENOMEM;
        goto amb_spi_nor_probe_free_flash;
    }
    if (flash->page_size > AMBA_SPINOR_DMA_BUFF_SIZE) {
        dev_err((const struct device *)&flash->dev,
            "SPI NOR driver buff size should bigger than nor flash page size \r\n");
        errCode = -EINVAL;
        goto amb_spi_nor_probe_free_command;
    }
	flash->clk = info->max_clk_hz;
    ambspi_init(flash);

    if (info->addr_width)
        flash->addr_width = info->addr_width;
    else {
        /* enable 4-byte addressing if the device exceeds 16MiB */
        if (flash->mtd.size > 0x1000000) {
            flash->addr_width = 4;
			/* We have set 4-bytes mode in bootloader */
            //set_4byte(flash, info->jedec_id, 1);
			spi_addr_mode = 4;
        } else {
			flash->addr_width = 3;
			spi_addr_mode = 3;
		}
    }

	flash->jedec_id	= info->jedec_id;
    pr_debug("mtd .name = %s, .size = 0x%llx (%lldMiB) "
            ".erasesize = 0x%.8x (%uKiB) .numeraseregions = %d\n",
        flash->mtd.name,
        (long long)flash->mtd.size, (long long)(flash->mtd.size >> 20),
        flash->mtd.erasesize, flash->mtd.erasesize / 1024,
        flash->mtd.numeraseregions);

    if (flash->mtd.numeraseregions)
        for (i = 0; i < flash->mtd.numeraseregions; i++)
            pr_debug("mtd.eraseregions[%d] = { .offset = 0x%llx, "
                ".erasesize = 0x%.8x (%uKiB), "
                ".numblocks = %d }\n",
                i, (long long)flash->mtd.eraseregions[i].offset,
                flash->mtd.eraseregions[i].erasesize,
                flash->mtd.eraseregions[i].erasesize / 1024,
                flash->mtd.eraseregions[i].numblocks);

    ambspi_dma_config(flash);
	ppdata.of_node = pdev->dev.of_node;
	errCode = mtd_device_parse_register(&flash->mtd, NULL, &ppdata, NULL, 0);
	if (errCode < 0)
		goto amb_spi_nor_probe_free_command;

	check_set_spinor_addr_mode(flash, 0);
    printk("SPI NOR Controller probed\r\n");
    return 0;

amb_spi_nor_probe_free_command:
    kfree(flash->command);
amb_spi_nor_probe_free_flash:
    kfree(flash);
amb_spi_nor_probe_exit:
    return errCode;
}


static int amb_spi_nor_remove(struct platform_device *pdev)
{

    struct amb_norflash    *flash = platform_get_drvdata(pdev);
    int        status;

    /* Clean up MTD stuff. */
    status = mtd_device_unregister(&flash->mtd);
    if (status == 0) {
        kfree(flash->command);
        dma_free_coherent(flash->dev,
            AMBA_SPINOR_DMA_BUFF_SIZE,
            flash->dmabuf, flash->dmaaddr);
        kfree(flash);
    }
    return 0;
}

static void amb_spi_nor_shutdown(struct platform_device *pdev)
{
	struct amb_norflash    *flash = platform_get_drvdata(pdev);

	/* Wait until finished previous write command. */
	if (wait_till_ready(flash))
		return;

	/* Send write enable, then erase commands. */
	//write_enable(flash);
	switch (JEDEC_MFR(flash->jedec_id)) {
		case CFI_MFR_MACRONIX:
		case 0xEF: /* winbond */
		case 0xC8: /* GD */
		case CFI_MFR_ST: /*Micron*/
			flash->command[0] = 0x66;
			ambspi_send_cmd(flash, flash->command[0], 0, 0, 0);
			flash->command[0] = 0x99;
			ambspi_send_cmd(flash, flash->command[0], 0, 0, 0);
			return;
		default:
			/* Spansion style */
			/* Set up command buffer. */
			/* FL01GS use 0xF0 as reset enable command */
			flash->command[0] = 0xF0;
			ambspi_send_cmd(flash, flash->command[0], 0, 0, 0);
	}
}

#ifdef CONFIG_PM
static int amb_spi_nor_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	int	errorCode = 0;
	struct amb_norflash	*flash;

	flash = platform_get_drvdata(pdev);

	dev_dbg(&pdev->dev, "%s exit with %d @ %d\n",
		__func__, errorCode, state.event);

	return errorCode;
}

static int amb_spi_nor_resume(struct platform_device *pdev)
{
	int	errorCode = 0;
	struct amb_norflash	*flash;

	flash = platform_get_drvdata(pdev);
	ambspi_init(flash);
	if (flash->addr_width == 4)
		set_4byte(flash, flash->jedec_id, 1);

	dev_dbg(&pdev->dev, "%s exit with %d\n", __func__, errorCode);

	return errorCode;
}
#endif

static const struct of_device_id ambarella_spi_nor_of_match[] = {
    {.compatible = "ambarella,spinor", },
    {},
};
MODULE_DEVICE_TABLE(of, ambarella_spi_nor_of_match);

static struct platform_driver amb_spi_nor_driver = {
	.driver = {
		.name = "ambarella-spinor",
		.owner = THIS_MODULE,
		.of_match_table = ambarella_spi_nor_of_match,
	},
	.probe = amb_spi_nor_probe,
	.remove = amb_spi_nor_remove,
	.shutdown = amb_spi_nor_shutdown,
#ifdef CONFIG_PM
	.suspend	= amb_spi_nor_suspend,
	.resume		= amb_spi_nor_resume,
#endif
	/* REVISIT: many of these chips have deep power-down modes, which
	 * should clearly be entered on suspend() to minimize power use.
	 * And also when they're otherwise idle...
	 */
};

module_platform_driver(amb_spi_nor_driver);

MODULE_AUTHOR("Johnson Diao");
MODULE_DESCRIPTION("Ambarella Media processor SPI NOR Flash Controller Driver");
MODULE_LICENSE("GPL");

