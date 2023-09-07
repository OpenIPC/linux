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
 */

#include <linux/init.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/math64.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/mod_devicetable.h>

#include <linux/mtd/cfi.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

#include <linux/spi/spi.h>
#include <linux/spi/flash.h>

/* Flash opcodes. */
#define	OPCODE_WREN		0x06	/* Write enable */
#define	OPCODE_RDSR		0x05	/* Read status register */
#define	OPCODE_WRSR		0x01	/* Write status register 1 byte */
#define	OPCODE_NORM_READ	0x03	/* Read data bytes (low frequency) */
#define	OPCODE_FAST_READ	0x0b	/* Read data bytes (high frequency) */
#define	OPCODE_PP		0x02	/* Page program (up to 256 bytes) */
#define	OPCODE_BE_4K		0x20	/* Erase 4KiB block */
#define	OPCODE_BE_32K		0x52	/* Erase 32KiB block */
#define	OPCODE_CHIP_ERASE	0xc7	/* Erase whole flash chip */
#define	OPCODE_SE		0xd8	/* Sector erase (usually 64KiB) */
#define	OPCODE_RDID		0x9f	/* Read JEDEC ID */

#define OPCODE_DUAL_READ_3_ADR		0x3b
#define OPCODE_QUAD_READ_3_ADR		0x6b
#define OPCODE_DUAL_READ_4_ADR		0x3c
#define OPCODE_QUAD_READ_4_ADR		0x6c

/* Used for SST flashes only. */
#define	OPCODE_BP		0x02	/* Byte program */
#define	OPCODE_WRDI		0x04	/* Write disable */
#define	OPCODE_AAI_WP	0xad	/* Auto address increment word program */

/* Used for Macronix flashes only. */
#define	OPCODE_EN4B		0xb7	/* Enter 4-byte mode */
#define	OPCODE_EX4B		0xe9	/* Exit 4-byte mode */

/* Used for Spansion flashes only. */
#define	OPCODE_BRWR		0x17	/* Bank register write */

/* Status Register bits. */
#define	SR_WIP			1	/* Write in progress */
#define	SR_WEL			2	/* Write enable latch */
/* meaning of other SR_* bits may differ between vendors */
#define	SR_BP0			4	/* Block protect 0 */
#define	SR_BP1			8	/* Block protect 1 */
#define	SR_BP2			0x10	/* Block protect 2 */
#define	SR_SRWD			0x80	/* SR write protect */

/* Define max times to check status register before we give up. */
#define	MAX_READY_WAIT_JIFFIES	(40 * HZ)	/* M25P16 specs 40s max chip erase */
#define	MAX_CMD_SIZE		5

#ifdef CONFIG_M25PXX_USE_FAST_READ
#define OPCODE_READ 	OPCODE_FAST_READ
#define FAST_READ_DUMMY_BYTE 1
#else
#define OPCODE_READ 	OPCODE_NORM_READ
#define FAST_READ_DUMMY_BYTE 0
#endif

#define JEDEC_MFR(_jedec_id)	((_jedec_id) >> 16)

/****************************************************************************/

struct m25p {
	struct spi_device	*spi;
	struct mutex		lock;
	struct mtd_info		mtd;
	unsigned		partitioned:1;
	u16			page_size;
	u16			addr_width;
	u8			erase_opcode;
	u8			*command;
};

static inline struct m25p *mtd_to_m25p(struct mtd_info *mtd)
{
	return container_of(mtd, struct m25p, mtd);
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
static int read_sr(struct m25p *flash)
{
	ssize_t retval;
	u8 code = OPCODE_RDSR;
	u8 val;

	retval = spi_write_then_read(flash->spi, &code, 1, &val, 1);

	if (retval < 0) {
		dev_err(&flash->spi->dev, "error %d reading SR\n",
				(int) retval);
		return retval;
	}

	return val;
}

/*
 * Write status register 1 byte
 * Returns negative if error occurred.
 */
static int write_sr(struct m25p *flash, u8 val)
{
	flash->command[0] = OPCODE_WRSR;
	flash->command[1] = val;

	return spi_write(flash->spi, flash->command, 2);
}

/*
 * Set write enable latch with Write Enable command.
 * Returns negative if error occurred.
 */
static inline int write_enable(struct m25p *flash)
{
	u8	code = OPCODE_WREN;

	return spi_write_then_read(flash->spi, &code, 1, NULL, 0);
}

/*
 * Send write disble instruction to the chip.
 */
static inline int write_disable(struct m25p *flash)
{
	u8	code = OPCODE_WRDI;

	return spi_write_then_read(flash->spi, &code, 1, NULL, 0);
}


/*
 * Enable/disable QE.
 */
static inline int set_qe(struct m25p *flash, u32 jedec_id, int enable)
{
	int ret = 0;
	struct spi_device *spi = flash->spi;
	u8 send_cmd[5] = {0};
	u8 get_data[5] = {0};

	switch (JEDEC_MFR(jedec_id)) {
	case CFI_MFR_MACRONIX:
		send_cmd[0] = 0x05;
		ret = spi_write_then_read(spi, send_cmd, 1, get_data, 1);
		if(ret < 0)
			return -1;
		get_data[0] &= ~(1 << 6);
		get_data[0] |= (enable << 6);
		send_cmd[0] = 0x01;
		send_cmd[1] = get_data[0];
		write_enable(flash);
		ret = spi_write(spi, send_cmd, 2);
		if (ret < 0)
			return -1;
		break;
	case 0xC8 /* GD */ :
		send_cmd[0] = 0x35;
		ret = spi_write_then_read(spi, send_cmd, 1, get_data, 1);
		if (ret < 0)
			return -1;
		get_data[0] &= ~(1 << 1);
		get_data[0] |= (enable << 1);
		send_cmd[0] = 0x31;
		send_cmd[1] = get_data[0];
		write_enable(flash);
		ret = spi_write(spi, send_cmd, 2);
		if (ret < 0)
			return -1;
		break;
	case 0xEF /* winbond */:
		/* status 0 */
		send_cmd[0] = 0x05;
		ret = spi_write_then_read(spi, send_cmd, 1, get_data, 1);
		if (ret < 0)
			return -1;
		/* status 1 */
		send_cmd[0] = 0x35;
		ret = spi_write_then_read(spi, send_cmd, 1, &get_data[1], 1);
		if (ret < 0)
			return -1;
		get_data[1] &= ~(1 << 1);
		get_data[1] |= (enable << 1);
		send_cmd[0] = 0x01;
		send_cmd[1] = get_data[0];
		send_cmd[2] = get_data[1];
		write_enable(flash);
		ret = spi_write(spi, send_cmd, 3);
		if (ret < 0)
			return -1;
		break;
	case 0x20 /* xmc */:
		/* 16MB and 8MB default support multi wire*/
		break;
	default:
		ret = -1;
		dev_err(&spi->dev, "%s : %d  default not support multi wire..\n", __func__, __LINE__);
		break;
	}
	return ret;

}
/*
 * Enable/disable 4-byte addressing mode.
 */
static inline int set_4byte(struct m25p *flash, u32 jedec_id, int enable)
{
	int ret;
	switch (JEDEC_MFR(jedec_id)) {
	case CFI_MFR_MACRONIX:
	case CFI_MFR_ST: /* Micron, actually */
	case 0xC8 /* GD */ :
		flash->command[0] = enable ? OPCODE_EN4B : OPCODE_EX4B;
		ret = spi_write(flash->spi, flash->command, 1);
		return ret;
	case 0xEF /* winbond */:
		flash->command[0] = enable ? OPCODE_EN4B : OPCODE_EX4B;
		ret = spi_write(flash->spi, flash->command, 1);
		if (!enable)
		{
			flash->command[0] = 0x06;
			spi_write(flash->spi, flash->command, 1);
			flash->command[0] = 0xc5;
			flash->command[1] = 0x00;
			ret = spi_write(flash->spi, flash->command, 2);
		}
		return ret;
	default:
		/* Spansion style */
		flash->command[0] = OPCODE_BRWR;
		flash->command[1] = enable << 7;
		return spi_write(flash->spi, flash->command, 2);
	}
}


/*
 * Service routine to read status register until ready, or timeout occurs.
 * Returns non-zero if error.
 */
static int wait_till_ready(struct m25p *flash)
{
	unsigned long deadline;
	int sr;

	deadline = jiffies + MAX_READY_WAIT_JIFFIES;

	do {
		if ((sr = read_sr(flash)) < 0)
			break;
		else if (!(sr & (SR_WIP | SR_WEL)))
			return 0;

		cond_resched();

	} while (!time_after_eq(jiffies, deadline));

	return 1;
}


static  int reset_chip(struct m25p *flash, u32 jedec_id)
{
	int ret;
	mutex_lock(&flash->lock);

	/* Wait till previous write/erase is done. */
	if (wait_till_ready(flash)) {
		mutex_unlock(&flash->lock);
		return 1;
	}

	switch (JEDEC_MFR(jedec_id)) {
	case 0x9F: /* S25FL128/256S spansion */
		flash->command[0] = 0xFF;
		ret = spi_write(flash->spi, flash->command, 1);
		flash->command[0] = 0xF0;
		ret = spi_write(flash->spi, flash->command, 1);
		mutex_unlock(&flash->lock);
		return ret;
	case 0xef:	/*winbond*/
	case 0xc8:	/*GD*/
		flash->command[0] = 0x66;
		ret = spi_write(flash->spi, flash->command, 1);
		flash->command[0] = 0x99;
		ret = spi_write(flash->spi, flash->command, 1);
		udelay(100);
		mutex_unlock(&flash->lock);
		return ret;
	case CFI_MFR_MACRONIX:
	case CFI_MFR_ST: /* Micron, actually */
	default:
		mutex_unlock(&flash->lock);
		return 0;
	}
}

/*
 * Erase the whole flash memory
 *
 * Returns 0 if successful, non-zero otherwise.
 */
static int erase_chip(struct m25p *flash)
{
	DEBUG(MTD_DEBUG_LEVEL3, "%s: %s %lldKiB\n",
	      dev_name(&flash->spi->dev), __func__,
	      (long long)(flash->mtd.size >> 10));

	/* Wait until finished previous write command. */
	if (wait_till_ready(flash))
		return 1;

	/* Send write enable, then erase commands. */
	write_enable(flash);

	/* Set up command buffer. */
	flash->command[0] = OPCODE_CHIP_ERASE;

	spi_write(flash->spi, flash->command, 1);

	return 0;
}

static void m25p_addr2cmd(struct m25p *flash, unsigned int addr, u8 *cmd)
{
	/* opcode is in cmd[0] */
	cmd[1] = addr >> (flash->addr_width * 8 -  8);
	cmd[2] = addr >> (flash->addr_width * 8 - 16);
	cmd[3] = addr >> (flash->addr_width * 8 - 24);
	cmd[4] = addr >> (flash->addr_width * 8 - 32);
}

static int m25p_cmdsz(struct m25p *flash)
{
	return 1 + flash->addr_width;
}

/*
 * Erase one sector of flash memory at offset ``offset'' which is any
 * address within the sector which should be erased.
 *
 * Returns 0 if successful, non-zero otherwise.
 */
static int erase_sector(struct m25p *flash, u32 offset)
{
	DEBUG(MTD_DEBUG_LEVEL3, "%s: %s %dKiB at 0x%08x\n",
			dev_name(&flash->spi->dev), __func__,
			flash->mtd.erasesize / 1024, offset);

	/* Wait until finished previous write command. */
	if (wait_till_ready(flash))
		return 1;

	/* Send write enable, then erase commands. */
	write_enable(flash);

	/* Set up command buffer. */
	flash->command[0] = flash->erase_opcode;
	m25p_addr2cmd(flash, offset, flash->command);

	spi_write(flash->spi, flash->command, m25p_cmdsz(flash));

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
static int m25p80_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct m25p *flash = mtd_to_m25p(mtd);
	u32 addr, len;
	uint32_t rem;

	DEBUG(MTD_DEBUG_LEVEL2, "%s: %s %s 0x%llx, len %lld\n",
	      dev_name(&flash->spi->dev), __func__, "at",
	      (long long)instr->addr, (long long)instr->len);

	/* sanity checks */
	if (instr->addr + instr->len > flash->mtd.size)
		return -EINVAL;
	div_u64_rem(instr->len, mtd->erasesize, &rem);
	if (rem)
		return -EINVAL;

	addr = instr->addr;
	len = instr->len;

	mutex_lock(&flash->lock);

	/* whole-chip erase? */
	if (len == flash->mtd.size) {
		if (erase_chip(flash)) {
			instr->state = MTD_ERASE_FAILED;
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
				mutex_unlock(&flash->lock);
				return -EIO;
			}

			addr += mtd->erasesize;
			len -= mtd->erasesize;
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

void fix_read_cmd(struct m25p *p_m25p, struct spi_transfer *p_trans)
{
	struct spi_device *spi;
	struct m25p *flash;
	flash = p_m25p;
	spi = flash->spi;
	/*if spi dev open as multi wire..check cmd data.*/
	if (spi->dev_open_multi_wire_flag & MULTI_WIRE_SUPPORT) {
		p_trans[0].xfer_wire_mode = ONE_WIRE_SUPPORT;
		if (spi->dev_open_multi_wire_flag & QUAD_WIRE_SUPPORT) {
			p_trans[1].xfer_wire_mode = QUAD_WIRE_SUPPORT;
			p_trans[1].xfer_dir = SPI_DATA_DIR_IN;
			if (flash->addr_width == 4)
				flash->command[0] = OPCODE_QUAD_READ_4_ADR;
			else
				flash->command[0] = OPCODE_QUAD_READ_3_ADR;
		} else if (spi->dev_open_multi_wire_flag & DUAL_WIRE_SUPPORT) {
			p_trans[1].xfer_wire_mode = DUAL_WIRE_SUPPORT;
			p_trans[1].xfer_dir = SPI_DATA_DIR_IN;
			if (flash->addr_width == 4)
				flash->command[0] = OPCODE_DUAL_READ_4_ADR;
			else
				flash->command[0] = OPCODE_DUAL_READ_3_ADR;
		} else {
			/*p_trans[0].xfer_wire_mode = ONE_WIRE_SUPPORT;*/
			p_trans[1].xfer_wire_mode = ONE_WIRE_SUPPORT;
			p_trans[1].xfer_dir = SPI_DATA_DIR_IN;
			flash->command[0] = OPCODE_READ; }
	} else
		flash->command[0] = OPCODE_READ;
}

static int m25p80_read(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, u_char *buf)
{
	struct m25p *flash = mtd_to_m25p(mtd);
	struct spi_transfer t[2];
	struct spi_message m;
	struct spi_device *spi;
	struct spi_master *master;
	spi = flash->spi;
	master = spi->master;
	DEBUG(MTD_DEBUG_LEVEL2, "%s: %s %s 0x%08x, len %zd\n",
			dev_name(&flash->spi->dev), __func__, "from",
			(u32)from, len);

	/* sanity checks */
	if (!len)
		return 0;

	if (from + len > flash->mtd.size)
		return -EINVAL;

	spi_message_init(&m);
	memset(t, 0, (sizeof t));

	/* NOTE:
	 * OPCODE_FAST_READ (if available) is faster.
	 * Should add 1 byte DUMMY_BYTE.
	 */
	t[0].tx_buf = flash->command;
	t[0].len = m25p_cmdsz(flash) + FAST_READ_DUMMY_BYTE;
	spi_message_add_tail(&t[0], &m);

	t[1].rx_buf = buf;
	t[1].len = len;
	spi_message_add_tail(&t[1], &m);

	/* Byte count starts at zero. */
	*retlen = 0;

	mutex_lock(&flash->lock);

	/* Wait till previous write/erase is done. */
	if (wait_till_ready(flash)) {
		/* REVISIT status return?? */
		mutex_unlock(&flash->lock);
		return 1;
	}

	/* FIXME switch to OPCODE_FAST_READ.  It's required for higher
	 * clocks; and at this writing, every chip this driver handles
	 * supports that opcode.
	 */

	/* Set up the write data buffer. */
	/*fix cmd here...*/
	fix_read_cmd(flash, t);
	m25p_addr2cmd(flash, from, flash->command);

	spi_sync(flash->spi, &m);

	*retlen = m.actual_length - m25p_cmdsz(flash) - FAST_READ_DUMMY_BYTE;
	/*back to one wire..*/
	if (spi->dev_open_multi_wire_flag & MULTI_WIRE_SUPPORT) {
		/*change to one wire here first.....*/
		/*printk("back to one wire..\n");*/
		master->ctl_multi_wire_info.change_to_1_wire(master);
	}
	mutex_unlock(&flash->lock);

	return 0;
}

/*
 * Write an address range to the flash chip.  Data must be written in
 * FLASH_PAGESIZE chunks.  The address range may be any size provided
 * it is within the physical boundaries.
 */
static int m25p80_write(struct mtd_info *mtd, loff_t to, size_t len,
	size_t *retlen, const u_char *buf)
{
	struct m25p *flash = mtd_to_m25p(mtd);
	u32 page_offset, page_size;
	struct spi_transfer t[2];
	struct spi_message m;

	DEBUG(MTD_DEBUG_LEVEL2, "%s: %s %s 0x%08x, len %zd\n",
			dev_name(&flash->spi->dev), __func__, "to",
			(u32)to, len);

	*retlen = 0;

	/* sanity checks */
	if (!len)
		return 0;

	if (to + len > flash->mtd.size)
		return -EINVAL;

	spi_message_init(&m);
	memset(t, 0, (sizeof t));

	t[0].tx_buf = flash->command;
	t[0].len = m25p_cmdsz(flash);
	spi_message_add_tail(&t[0], &m);

	t[1].tx_buf = buf;
	spi_message_add_tail(&t[1], &m);

	mutex_lock(&flash->lock);

	/* Wait until finished previous write command. */
	if (wait_till_ready(flash)) {
		mutex_unlock(&flash->lock);
		return 1;
	}

	write_enable(flash);

	/* Set up the opcode in the write buffer. */
	flash->command[0] = OPCODE_PP;
	m25p_addr2cmd(flash, to, flash->command);

	page_offset = to & (flash->page_size - 1);

	/* do all the bytes fit onto one page? */
	if (page_offset + len <= flash->page_size) {
		t[1].len = len;

		spi_sync(flash->spi, &m);

		*retlen = m.actual_length - m25p_cmdsz(flash);
	} else {
		u32 i;

		/* the size of data remaining on the first page */
		page_size = flash->page_size - page_offset;

		t[1].len = page_size;
		spi_sync(flash->spi, &m);

		*retlen = m.actual_length - m25p_cmdsz(flash);

		/* write everything in flash->page_size chunks */
		for (i = page_size; i < len; i += page_size) {
			page_size = len - i;
			if (page_size > flash->page_size)
				page_size = flash->page_size;

			/* write the next page to flash */
			m25p_addr2cmd(flash, to + i, flash->command);

			t[1].tx_buf = buf + i;
			t[1].len = page_size;

			wait_till_ready(flash);

			write_enable(flash);

			spi_sync(flash->spi, &m);

			*retlen += m.actual_length - m25p_cmdsz(flash);
		}
	}

	mutex_unlock(&flash->lock);

	return 0;
}

static int sst_write(struct mtd_info *mtd, loff_t to, size_t len,
		size_t *retlen, const u_char *buf)
{
	struct m25p *flash = mtd_to_m25p(mtd);
	struct spi_transfer t[2];
	struct spi_message m;
	size_t actual;
	int cmd_sz, ret;

	DEBUG(MTD_DEBUG_LEVEL2, "%s: %s %s 0x%08x, len %zd\n",
			dev_name(&flash->spi->dev), __func__, "to",
			(u32)to, len);

	*retlen = 0;

	/* sanity checks */
	if (!len)
		return 0;

	if (to + len > flash->mtd.size)
		return -EINVAL;

	spi_message_init(&m);
	memset(t, 0, (sizeof t));

	t[0].tx_buf = flash->command;
	t[0].len = m25p_cmdsz(flash);
	spi_message_add_tail(&t[0], &m);

	t[1].tx_buf = buf;
	spi_message_add_tail(&t[1], &m);

	mutex_lock(&flash->lock);

	/* Wait until finished previous write command. */
	ret = wait_till_ready(flash);
	if (ret)
		goto time_out;

	write_enable(flash);

	actual = to % 2;
	/* Start write from odd address. */
	if (actual) {
		flash->command[0] = OPCODE_BP;
		m25p_addr2cmd(flash, to, flash->command);

		/* write one byte. */
		t[1].len = 1;
		spi_sync(flash->spi, &m);
		ret = wait_till_ready(flash);
		if (ret)
			goto time_out;
		*retlen += m.actual_length - m25p_cmdsz(flash);
	}
	to += actual;

	flash->command[0] = OPCODE_AAI_WP;
	m25p_addr2cmd(flash, to, flash->command);

	/* Write out most of the data here. */
	cmd_sz = m25p_cmdsz(flash);
	for (; actual < len - 1; actual += 2) {
		t[0].len = cmd_sz;
		/* write two bytes. */
		t[1].len = 2;
		t[1].tx_buf = buf + actual;

		spi_sync(flash->spi, &m);
		ret = wait_till_ready(flash);
		if (ret)
			goto time_out;
		*retlen += m.actual_length - cmd_sz;
		cmd_sz = 1;
		to += 2;
	}
	write_disable(flash);
	ret = wait_till_ready(flash);
	if (ret)
		goto time_out;

	/* Write out trailing byte if it exists. */
	if (actual != len) {
		write_enable(flash);
		flash->command[0] = OPCODE_BP;
		m25p_addr2cmd(flash, to, flash->command);
		t[0].len = m25p_cmdsz(flash);
		t[1].len = 1;
		t[1].tx_buf = buf + actual;

		spi_sync(flash->spi, &m);
		ret = wait_till_ready(flash);
		if (ret)
			goto time_out;
		*retlen += m.actual_length - m25p_cmdsz(flash);
		write_disable(flash);
	}

time_out:
	mutex_unlock(&flash->lock);
	return ret;
}

/****************************************************************************/

/*
 * SPI device driver setup and teardown
 */

struct flash_info {
	/* JEDEC id zero means "no ID" (most older chips); otherwise it has
	 * a high byte of zero plus three data bytes: the manufacturer id,
	 * then a two byte device id.
	 */
	u32		jedec_id;
	u16             ext_id;

	/* The size listed here is what works with OPCODE_SE, which isn't
	 * necessarily called a "sector" by the vendor.
	 */
	unsigned	sector_size;
	u16		n_sectors;

	u16		page_size;
	u16		addr_width;

	u16		flags;
#define	SECT_4K		0x01		/* OPCODE_BE_4K works uniformly */
#define	M25P_NO_ERASE	0x02		/* No erase command needed */

	u32		multi_wire_open;
};

#define INFO(_jedec_id, _ext_id, _sector_size, _n_sectors, _flags, _multi_wire_flag)	\
	((kernel_ulong_t)&(struct flash_info) {				\
		.jedec_id = (_jedec_id),				\
		.ext_id = (_ext_id),					\
		.sector_size = (_sector_size),				\
		.n_sectors = (_n_sectors),				\
		.page_size = 256,					\
		.flags = (_flags),					\
		.multi_wire_open = (_multi_wire_flag),\
	})

#define CAT25_INFO(_sector_size, _n_sectors, _page_size, _addr_width, _multi_wire_flag)	\
	((kernel_ulong_t)&(struct flash_info) {				\
		.sector_size = (_sector_size),				\
		.n_sectors = (_n_sectors),				\
		.page_size = (_page_size),				\
		.addr_width = (_addr_width),				\
		.flags = M25P_NO_ERASE,					\
		.multi_wire_open = (_multi_wire_flag),\
	})

/* NOTE: double check command sets and memory organization when you add
 * more flash chips.  This current list focusses on newer chips, which
 * have been converging on command sets which including JEDEC ID.
 */
#define M25P80_4_WIRE_ALL_SUPPORT (ONE_WIRE_SUPPORT | \
DUAL_WIRE_SUPPORT | QUAD_WIRE_SUPPORT | MULTI_WIRE_SUPPORT)
#define M25P80_2_WIRE_ALL_SUPPORT (ONE_WIRE_SUPPORT | \
DUAL_WIRE_SUPPORT | MULTI_WIRE_SUPPORT)
static const struct spi_device_id m25p_ids[] = {
	/* Atmel -- some are (confusingly) marketed as "DataFlash" */
	{ "at25fs010",  INFO(0x1f6601, 0, 32 * 1024,   4, SECT_4K, 0) },
	{ "at25fs040",  INFO(0x1f6604, 0, 64 * 1024,   8, SECT_4K, 0) },

	{ "at25df041a", INFO(0x1f4401, 0, 64 * 1024,   8, SECT_4K, 0) },
	{ "at25df641",  INFO(0x1f4800, 0, 64 * 1024, 128, SECT_4K, 0) },

	{ "at26f004",   INFO(0x1f0400, 0, 64 * 1024,  8, SECT_4K, 0) },
	{ "at26df081a", INFO(0x1f4501, 0, 64 * 1024, 16, SECT_4K, 0) },
	{ "at26df161a", INFO(0x1f4601, 0, 64 * 1024, 32, SECT_4K, 0) },
	{ "at26df321",  INFO(0x1f4700, 0, 64 * 1024, 64, SECT_4K, 0) },

	/* EON -- en25xxx */
	{ "en25f32", INFO(0x1c3116, 0, 64 * 1024,  64, SECT_4K, 0) },
	{ "en25p32", INFO(0x1c2016, 0, 64 * 1024,  64, 0, 0) },
	{ "en25p64", INFO(0x1c2017, 0, 64 * 1024, 128, 0, 0) },

	/* Intel/Numonyx -- xxxs33b */
	{ "160s33b",  INFO(0x898911, 0, 64 * 1024,  32, 0, 0) },
	{ "320s33b",  INFO(0x898912, 0, 64 * 1024,  64, 0, 0) },
	{ "640s33b",  INFO(0x898913, 0, 64 * 1024, 128, 0, 0) },

	/* Macronix */
	{ "mx25l4005a",  INFO(0xc22013, 0, 64 * 1024,   8, SECT_4K, 0) },
	{ "mx25l8005",   INFO(0xc22014, 0, 64 * 1024,  16, 0, 0) },
	{ "mx25l1606e",  INFO(0xc22015, 0, 64 * 1024,  32, SECT_4K, 0) },
	{ "mx25l3205d",  INFO(0xc22016, 0, 64 * 1024,  64, 0, 0) },
	{ "mx25l6405d",  INFO(0xc22017, 0, 64 * 1024, 128, 0,
	M25P80_4_WIRE_ALL_SUPPORT) },
	{ "mx25l12805d", INFO(0xc22018, 0, 64 * 1024, 256, 0,
	M25P80_4_WIRE_ALL_SUPPORT) },
	{ "mx25l12855e", INFO(0xc22618, 0, 64 * 1024, 256, 0, 0) },
	{ "mx25l25635e", INFO(0xc22019, 0, 64 * 1024, 512, 0, 0) },
	{ "mx25l25655e", INFO(0xc22619, 0, 64 * 1024, 512, 0, 0) },

	/* Spansion -- single (large) sector size only, at least
	 * for the chips listed here (without boot sectors).
	 */
	{ "s25sl004a",  INFO(0x010212,      0,  64 * 1024,   8, 0, 0) },
	{ "s25sl008a",  INFO(0x010213,      0,  64 * 1024,  16, 0, 0) },
	{ "s25sl016a",  INFO(0x010214,      0,  64 * 1024,  32, 0, 0) },
	{ "s25sl032a",  INFO(0x010215,      0,  64 * 1024,  64, 0, 0) },
	{ "s25sl032p",  INFO(0x010215, 0x4d00,  64 * 1024,  64, SECT_4K, 0) },
	{ "s25sl064a",  INFO(0x010216,      0,  64 * 1024, 128, 0, 0) },
	{ "s25fl256s0", INFO(0x010219, 0x4d00, 256 * 1024, 128, 0, 0) },
	{ "s25fl256s1", INFO(0x010219, 0x4d01,  64 * 1024, 512, 0, 0) },
	{ "s25fl512s",  INFO(0x010220, 0x4d00, 256 * 1024, 256, 0, 0) },
	{ "s70fl01gs",  INFO(0x010221, 0x4d00, 256 * 1024, 256, 0, 0) },
	{ "s25sl12800", INFO(0x012018, 0x0300, 256 * 1024,  64, 0, 0) },
	{ "s25sl12801", INFO(0x012018, 0x0301,  64 * 1024, 256, 0, 0) },
	{ "s25fl129p0", INFO(0x012018, 0x4d00, 256 * 1024,  64, 0, 0) },
	{ "s25fl129p1", INFO(0x012018, 0x4d01,  64 * 1024, 256, 0, 0) },
	/* SST -- large erase sizes are "overlays", "sectors" are 4K */
	{ "sst25vf040b", INFO(0xbf258d, 0, 64 * 1024,  8, SECT_4K, 0) },
	{ "sst25vf080b", INFO(0xbf258e, 0, 64 * 1024, 16, SECT_4K, 0) },
	{ "sst25vf016b", INFO(0xbf2541, 0, 64 * 1024, 32, SECT_4K, 0) },
	{ "sst25vf032b", INFO(0xbf254a, 0, 64 * 1024, 64, SECT_4K, 0) },
	{ "sst25wf512",  INFO(0xbf2501, 0, 64 * 1024,  1, SECT_4K, 0) },
	{ "sst25wf010",  INFO(0xbf2502, 0, 64 * 1024,  2, SECT_4K, 0) },
	{ "sst25wf020",  INFO(0xbf2503, 0, 64 * 1024,  4, SECT_4K, 0) },
	{ "sst25wf040",  INFO(0xbf2504, 0, 64 * 1024,  8, SECT_4K, 0) },

	/* ST Microelectronics -- newer production may have feature updates */
	{ "m25p05",  INFO(0x202010,  0,  32 * 1024,   2, 0, 0) },
	{ "m25p10",  INFO(0x202011,  0,  32 * 1024,   4, 0, 0) },
	{ "m25p20",  INFO(0x202012,  0,  64 * 1024,   4, 0, 0) },
	{ "m25p40",  INFO(0x202013,  0,  64 * 1024,   8, 0, 0) },
	{ "m25p80",  INFO(0x202014,  0,  64 * 1024,  16, 0, 0) },
	{ "m25p16",  INFO(0x202015,  0,  64 * 1024,  32, 0, 0) },
	{ "m25p32",  INFO(0x202016,  0,  64 * 1024,  64, 0, 0) },
	{ "m25p64",  INFO(0x202017,  0,  64 * 1024, 128, 0, 0) },
	{ "m25p128", INFO(0x202018,  0, 256 * 1024,  64, 0, 0) },

	{ "m25p05-nonjedec",  INFO(0, 0,  32 * 1024,   2, 0, 0) },
	{ "m25p10-nonjedec",  INFO(0, 0,  32 * 1024,   4, 0, 0) },
	{ "m25p20-nonjedec",  INFO(0, 0,  64 * 1024,   4, 0, 0) },
	{ "m25p40-nonjedec",  INFO(0, 0,  64 * 1024,   8, 0, 0) },
	{ "m25p80-nonjedec",  INFO(0, 0,  64 * 1024,  16, 0, 0) },
	{ "m25p16-nonjedec",  INFO(0, 0,  64 * 1024,  32, 0, 0) },
	{ "m25p32-nonjedec",  INFO(0, 0,  64 * 1024,  64, 0, 0) },
	{ "m25p64-nonjedec",  INFO(0, 0,  64 * 1024, 128, 0, 0) },
	{ "m25p128-nonjedec", INFO(0, 0, 256 * 1024,  64, 0, 0) },

	{ "m45pe10", INFO(0x204011,  0, 64 * 1024,    2, 0, 0) },
	{ "m45pe80", INFO(0x204014,  0, 64 * 1024,   16, 0, 0) },
	{ "m45pe16", INFO(0x204015,  0, 64 * 1024,   32, 0, 0) },

	{ "m25pe80", INFO(0x208014,  0, 64 * 1024, 16,       0, 0) },
	{ "m25pe16", INFO(0x208015,  0, 64 * 1024, 32, SECT_4K, 0) },

	{ "m25px32",    INFO(0x207116,  0, 64 * 1024, 64, SECT_4K, 0) },
	{ "m25px32-s0", INFO(0x207316,  0, 64 * 1024, 64, SECT_4K, 0) },
	{ "m25px32-s1", INFO(0x206316,  0, 64 * 1024, 64, SECT_4K, 0) },
	{ "m25px64",    INFO(0x207117,  0, 64 * 1024, 128, 0, 0) },

	/* Winbond -- w25x "blocks" are 64K, "sectors" are 4KiB */
	{ "w25x10", INFO(0xef3011, 0, 64 * 1024,  2,  SECT_4K, 0) },
	{ "w25x20", INFO(0xef3012, 0, 64 * 1024,  4,  SECT_4K, 0) },
	{ "w25x40", INFO(0xef3013, 0, 64 * 1024,  8,  SECT_4K, 0) },
	{ "w25x80", INFO(0xef3014, 0, 64 * 1024,  16, SECT_4K, 0) },
	{ "w25x16", INFO(0xef3015, 0, 64 * 1024,  32, SECT_4K, 0) },
	{ "w25x32", INFO(0xef3016, 0, 64 * 1024,  64, SECT_4K, 0) },
	{ "w25q32", INFO(0xef4016, 0, 64 * 1024,  64, SECT_4K, 0) },
	{ "w25q32dw", INFO(0xef6016, 0, 64 * 1024,  64, SECT_4K, 0) },
	{ "w25x64", INFO(0xef3017, 0, 64 * 1024, 128, SECT_4K, 0) },
	{ "w25q64", INFO(0xef4017, 0, 64 * 1024, 128, 0,
	M25P80_4_WIRE_ALL_SUPPORT) },
	{ "w25q80", INFO(0xef5014, 0, 64 * 1024,  16, SECT_4K, 0) },
	{ "w25q80bl", INFO(0xef4014, 0, 64 * 1024,  16, SECT_4K, 0) },
	{ "w25q128", INFO(0xef4018, 0, 64 * 1024, 256, 0,
	M25P80_4_WIRE_ALL_SUPPORT) },
	{ "w25q256", INFO(0xef4019, 0, 64 * 1024, 512, 0,
	M25P80_4_WIRE_ALL_SUPPORT) },
	{ "w25q16",  INFO(0xef4015, 0, 64 * 1024, 32, SECT_4K,
	M25P80_4_WIRE_ALL_SUPPORT) },
	/* Catalyst / On Semiconductor -- non-JEDEC */
	{ "cat25c11", CAT25_INFO(16, 8, 16, 1, 0) },
	{ "cat25c03", CAT25_INFO(32, 8, 16, 2, 0) },
	{ "cat25c09", CAT25_INFO(128, 8, 32, 2, 0) },
	{ "cat25c17", CAT25_INFO(256, 8, 32, 2, 0) },
	{ "cat25128", CAT25_INFO(2048, 8, 64, 2, 0) },

	/*for GD flash..*/
	{ "gd25q128", INFO(0xc84018, 0, 64 * 1024, 256, 0,
	M25P80_4_WIRE_ALL_SUPPORT) },
	{ "gd25q64", INFO(0xc84017, 0, 64 * 1024, 128, 0,
	M25P80_4_WIRE_ALL_SUPPORT) },
	{ "gd25q16", INFO(0xc84015, 0, 64 * 1024, 32, 0,
	M25P80_4_WIRE_ALL_SUPPORT) },
	/*for xmc flash..*/
	{ "XM25QH128A", INFO(0x207018, 0, 64 * 1024, 256, 0,
	M25P80_4_WIRE_ALL_SUPPORT) },
	{ "XM25QH64A", INFO(0x207017, 0, 64 * 1024, 128, 0,
	M25P80_4_WIRE_ALL_SUPPORT) },
	
	// BY25Q64AS 
	{ "by25q64as", INFO(0x684017, 0, 64 * 1024, 128, 0, 0 )},


	{ },
};
MODULE_DEVICE_TABLE(spi, m25p_ids);

static const struct spi_device_id *__devinit jedec_probe(struct spi_device *spi)
{
	int			tmp;
	u8			code = OPCODE_RDID;
	u8			id[5];
	u32			jedec;
	u16                     ext_jedec;
	struct flash_info	*info;

	/* JEDEC also defines an optional "extended device information"
	 * string for after vendor-specific data, after the three bytes
	 * we use here.  Supporting some chips might require using it.
	 */
	tmp = spi_write_then_read(spi, &code, 1, id, 5);
	if (tmp < 0) {
		DEBUG(MTD_DEBUG_LEVEL0, "%s: error %d reading JEDEC ID\n",
			dev_name(&spi->dev), tmp);
		return ERR_PTR(tmp);
	}
	jedec = id[0];
	jedec = jedec << 8;
	jedec |= id[1];
	jedec = jedec << 8;
	jedec |= id[2];

	ext_jedec = id[3] << 8 | id[4];

	for (tmp = 0; tmp < ARRAY_SIZE(m25p_ids) - 1; tmp++) {
		info = (void *)m25p_ids[tmp].driver_data;
		if (info->jedec_id == jedec) {
			if (info->ext_id != 0 && info->ext_id != ext_jedec)
				continue;
			return &m25p_ids[tmp];
		}
	}
	dev_err(&spi->dev, "unrecognized JEDEC id %06x\n", jedec);
	return ERR_PTR(-ENODEV);
}


/*
 * board specific setup should have ensured the SPI clock used here
 * matches what the READ command supports, at least until this driver
 * understands FAST_READ (for clocks over 25 MHz).
 */
static int __devinit m25p_probe(struct spi_device *spi)
{
	const struct spi_device_id	*id = spi_get_device_id(spi);
	struct flash_platform_data	*data;
	struct m25p			*flash;
	struct flash_info		*info;
	unsigned			i;
	struct mtd_partition		*parts = NULL;
	int				nr_parts = 0;
	struct spi_master *p_master;
	int ret;
	/* Platform data helps sort out which chip type we have, as
	 * well as how this board partitions it.  If we don't have
	 * a chip ID, try the JEDEC id commands; they'll work for most
	 * newer chips, even if we don't recognize the particular chip.
	 */
	p_master = spi->master;
	if (p_master->ctl_multi_wire_info.ctl_wire_support
	& MULTI_WIRE_SUPPORT) {
		/*if master support multi wire, set one wire here..*/
		p_master->ctl_multi_wire_info.change_to_1_wire(p_master);
	}
	data = spi->dev.platform_data;
	if (data && data->type) {
		const struct spi_device_id *plat_id;

		for (i = 0; i < ARRAY_SIZE(m25p_ids) - 1; i++) {
			plat_id = &m25p_ids[i];
			if (strcmp(data->type, plat_id->name))
				continue;
			break;
		}

		if (i < ARRAY_SIZE(m25p_ids) - 1)
			id = plat_id;
		else
			dev_warn(&spi->dev, "unrecognized id %s\n", data->type);
	}

	info = (void *)id->driver_data;

	if (info->jedec_id) {
		const struct spi_device_id *jid;

		jid = jedec_probe(spi);
		if (IS_ERR(jid)) {
			return PTR_ERR(jid);
		} else if (jid != id) {
			/*
			 * JEDEC knows better, so overwrite platform ID. We
			 * can't trust partitions any longer, but we'll let
			 * mtd apply them anyway, since some partitions may be
			 * marked read-only, and we don't want to lose that
			 * information, even if it's not 100% accurate.
			 */
			dev_warn(&spi->dev, "found %s, expected %s\n",
				 jid->name, id->name);
			id = jid;
			info = (void *)jid->driver_data;
		}
	}

	flash = kzalloc(sizeof *flash, GFP_KERNEL);
	if (!flash)
		return -ENOMEM;
	flash->command =
	kmalloc(MAX_CMD_SIZE + FAST_READ_DUMMY_BYTE, GFP_KERNEL);
	if (!flash->command) {
		kfree(flash);
		return -ENOMEM;
	}

	flash->spi = spi;
	mutex_init(&flash->lock);
	dev_set_drvdata(&spi->dev, flash);

	/*
	 * Atmel, SST and Intel/Numonyx serial flash tend to power
	 * up with the software protection bits set
	 */

	if (JEDEC_MFR(info->jedec_id) == CFI_MFR_ATMEL ||
	    JEDEC_MFR(info->jedec_id) == CFI_MFR_INTEL ||
	    JEDEC_MFR(info->jedec_id) == CFI_MFR_SST) {
		write_enable(flash);
		write_sr(flash, 0);
	}

	if (data && data->name)
		flash->mtd.name = data->name;
	else
		flash->mtd.name = dev_name(&spi->dev);

	flash->mtd.type = MTD_NORFLASH;
	flash->mtd.writesize = 1;
	flash->mtd.flags = MTD_CAP_NORFLASH;
	flash->mtd.size = info->sector_size * info->n_sectors;
	flash->mtd.erase = m25p80_erase;
	flash->mtd.read = m25p80_read;
	flash->mtd.priv = (void *)info->jedec_id;

	/* sst flash chips use AAI word program */
	if (JEDEC_MFR(info->jedec_id) == CFI_MFR_SST)
		flash->mtd.write = sst_write;
	else
		flash->mtd.write = m25p80_write;

	/* prefer "small sector" erase if possible */
	if (info->flags & SECT_4K) {
		flash->erase_opcode = OPCODE_BE_4K;
		flash->mtd.erasesize = 4096;
	} else {
		flash->erase_opcode = OPCODE_SE;
		flash->mtd.erasesize = info->sector_size;
	}

	if (info->flags & M25P_NO_ERASE)
		flash->mtd.flags |= MTD_NO_ERASE;

	flash->mtd.dev.parent = &spi->dev;
	flash->page_size = info->page_size;

	if (info->addr_width)
		flash->addr_width = info->addr_width;
	else {
		/* enable 4-byte addressing if the device exceeds 16MiB */
		if (flash->mtd.size > 0x1000000) {
			flash->addr_width = 4;
			set_4byte(flash, info->jedec_id, 1);
		} else
			flash->addr_width = 3;
	}
	/* add set qe bit here.. */
	ret = set_qe(flash, info->jedec_id, 1);
	if (ret != 0)
		info->multi_wire_open = 0;
	spi->dev_open_multi_wire_flag = info->multi_wire_open;
	spi_dev_set_multi_data(p_master, spi);

	dev_info(&spi->dev, "%s (%lld Kbytes)\n", id->name,
			(long long)flash->mtd.size >> 10);

	DEBUG(MTD_DEBUG_LEVEL2,
		"mtd .name = %s, .size = 0x%llx (%lldMiB) "
			".erasesize = 0x%.8x (%uKiB) .numeraseregions = %d\n",
		flash->mtd.name,
		(long long)flash->mtd.size, (long long)(flash->mtd.size >> 20),
		flash->mtd.erasesize, flash->mtd.erasesize / 1024,
		flash->mtd.numeraseregions);

	if (flash->mtd.numeraseregions)
		for (i = 0; i < flash->mtd.numeraseregions; i++)
			DEBUG(MTD_DEBUG_LEVEL2,
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
	if (mtd_has_cmdlinepart()) {
		static const char *part_probes[]
			= { "cmdlinepart", NULL, };

		nr_parts = parse_mtd_partitions(&flash->mtd,
						part_probes, &parts, 0);
	}

	if (nr_parts <= 0 && data && data->parts) {
		parts = data->parts;
		nr_parts = data->nr_parts;
	}

#ifdef CONFIG_MTD_OF_PARTS
	if (nr_parts <= 0 && spi->dev.of_node) {
		nr_parts = of_mtd_parse_partitions(&spi->dev,
						   spi->dev.of_node, &parts);
	}
#endif

	if (nr_parts > 0) {
		for (i = 0; i < nr_parts; i++) {
			DEBUG(MTD_DEBUG_LEVEL2, "partitions[%d] = "
			      "{.name = %s, .offset = 0x%llx, "
			      ".size = 0x%llx (%lldKiB) }\n",
			      i, parts[i].name,
			      (long long)parts[i].offset,
			      (long long)parts[i].size,
			      (long long)(parts[i].size >> 10));
		}
		flash->partitioned = 1;
	}

	return mtd_device_register(&flash->mtd, parts, nr_parts) == 1 ?
		-ENODEV : 0;
}

static void m25p_shutdown(struct spi_device *spi)
{
	struct m25p	*flash = dev_get_drvdata(&spi->dev);
	u32 jedec = (u32)flash->mtd.priv;
	dev_err(&spi->dev, "[m25] shutdown here? \n");
	if (flash->addr_width == 4) {
		set_4byte(flash, jedec, 0);
		flash->addr_width = 3;
	}

	if (reset_chip(flash, jedec))
		dev_err(&spi->dev, "[m25] reset chip error...\n");
}


static int __devexit m25p_remove(struct spi_device *spi)
{
	struct m25p	*flash = dev_get_drvdata(&spi->dev);
	int		status;

	/* Clean up MTD stuff. */
	status = mtd_device_unregister(&flash->mtd);
	if (status == 0) {
		kfree(flash->command);
		kfree(flash);
	}
	return 0;
}


static struct spi_driver m25p80_driver = {
	.driver = {
		.name	= "m25p80",
		.bus	= &spi_bus_type,
		.owner	= THIS_MODULE,
	},
	.id_table	= m25p_ids,
	.probe	= m25p_probe,
	.remove	= __devexit_p(m25p_remove),
	.shutdown = m25p_shutdown,
	/* REVISIT: many of these chips have deep power-down modes, which
	 * should clearly be entered on suspend() to minimize power use.
	 * And also when they're otherwise idle...
	 */
};


static int __init m25p80_init(void)
{
	return spi_register_driver(&m25p80_driver);
}


static void __exit m25p80_exit(void)
{
	spi_unregister_driver(&m25p80_driver);
}


module_init(m25p80_init);
module_exit(m25p80_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mike Lavender");
MODULE_DESCRIPTION("MTD SPI driver for ST M25Pxx flash chips");
