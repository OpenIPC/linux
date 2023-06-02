/**
* spi-nand-base.c
*
* Copyright (c) 2009-2014 Micron Technology, Inc.
*
* Derived from nand_base.c
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/spi-nand.h>
#include <linux/mtd/bbm.h>
#include <linux/spi/spi.h>
#include <linux/slab.h>
#include "spi-nand-ids.h"

int fh_start_debug = 0;
/*#define SPINAND_BBT_DEBUG*/
#ifdef SPINAND_BBT_DEBUG
#define fh_dev_debug	dev_err
#else
#define fh_dev_debug(...)
#endif

static int spi_nand_erase(struct mtd_info *mtd, struct erase_info *einfo);

/**
 * spi_nand_get_device - [GENERIC] Get chip for selected access
 * @mtd: MTD device structure
 * @new_state: the state which is requested
 *
 * Get the device and lock it for exclusive access
 */
static int spi_nand_get_device(struct mtd_info *mtd, int new_state)
{
	struct spi_nand_chip *this = mtd->priv;
	DECLARE_WAITQUEUE(wait, current);

	/*
	 * Grab the lock and see if the device is available
	 */
	while (1) {
		spin_lock(&this->chip_lock);
		if (this->state == FL_READY) {
			this->state = new_state;
			spin_unlock(&this->chip_lock);
			break;
		}
		if (new_state == FL_PM_SUSPENDED) {
			spin_unlock(&this->chip_lock);
			return (this->state == FL_PM_SUSPENDED) ? 0 : -EAGAIN;
		}
		set_current_state(TASK_UNINTERRUPTIBLE);
		add_wait_queue(&this->wq, &wait);
		spin_unlock(&this->chip_lock);
		schedule();
		remove_wait_queue(&this->wq, &wait);
	}
	return 0;
}

/**
 * spi_nand_release_device - [GENERIC] release chip
 * @mtd: MTD device structure
 *
 * Deselect, release chip lock and wake up anyone waiting on the device
 */
static void spi_nand_release_device(struct mtd_info *mtd)
{
	struct spi_nand_chip *this = mtd->priv;

	/* Release the chip */
	spin_lock(&this->chip_lock);
	this->state = FL_READY;
	wake_up(&this->wq);
	spin_unlock(&this->chip_lock);
}

/**
 * __spi_nand_do_read_page - [INTERN] read data from flash to buffer
 * @mtd: MTD device structure
 * @page_addr: page address/raw address
 * @column :column address
 * @raw: without ecc or not
 * @corrected: how many bit error corrected
 *
 * read a page to buffer pointed by chip->buf
 */
static int __spi_nand_do_read_page(struct mtd_info *mtd, u32 page_addr,
				   u32 colunm, bool raw, int *corrected)
{
	struct spi_nand_chip *chip = mtd->priv;
	int ret, ecc_error;
	u8 status;

	fh_dev_debug(&chip->spi->dev, "Enter %s\n", __func__);

	/*read data from chip*/
	memset(chip->buf, 0, chip->page_size + chip->page_spare_size);
	if (raw) {
		ret = chip->disable_ecc(chip);
		if (ret < 0) {
			pr_debug("disable ecc failed\n");
			return ret;
		}
	}
	ret = chip->load_page(chip, page_addr);
	if (ret < 0) {
		pr_debug("error %d loading page 0x%x to cache\n",
			 ret, page_addr);
		return ret;
	}
	ret = chip->waitfunc(chip, &status);
	if (ret < 0) {
		pr_debug("error %d waiting page 0x%x to cache\n",
			 ret, page_addr);
		return ret;
	}
	chip->get_ecc_status(chip, status, corrected, &ecc_error);
	/*
	 * If there's an ECC error, print a message and notify MTD
	 * about it. Then complete the read, to load actual data on
	 * the buffer (instead of the status result).
	 */
	if (ecc_error) {
		pr_warn("internal ECC error reading page 0x%x with status 0x%02x\n",
			 page_addr, status);
		mtd->ecc_stats.failed++;
	} else if (*corrected)
		mtd->ecc_stats.corrected += *corrected;
	/* Get page from the device cache into our internal buffer */
	ret = chip->read_cache(chip, page_addr, colunm,
			       chip->page_size + chip->page_spare_size - colunm,
			       chip->buf + colunm);
	if (ret < 0) {
		pr_debug("error %d reading page 0x%x from cache\n",
			 ret, page_addr);
		return ret;
	}
	if (raw) {
		ret = chip->enable_ecc(chip);
		if (ret < 0) {
			pr_debug("enable ecc failed\n");
			return ret;
		}
	}

	return 0;
}

/**
 * spi_nand_do_read_page - [INTERN] read a page from flash to buffer
 * @mtd: MTD device structure
 * @page_addr: page address/raw address
 * @raw: without ecc or not
 * @corrected: how many bit error corrected
 *
 * read a page to buffer pointed by chip->buf
 */
static int spi_nand_do_read_page(struct mtd_info *mtd, u32 page_addr,
				 bool raw, int *corrected)
{
	return __spi_nand_do_read_page(mtd, page_addr, 0, raw, corrected);
}

/**
 * spi_nand_do_read_page_oob - [INTERN] read page oob from flash to buffer
 * @mtd: MTD device structure
 * @page_addr: page address/raw address
 * @raw: without ecc or not
 * @corrected: how many bit error corrected
 *
 * read page oob to buffer pointed by chip->oobbuf
 */
static int spi_nand_do_read_page_oob(struct mtd_info *mtd, u32 page_addr,
				     bool raw, int *corrected)
{
	struct spi_nand_chip *chip = mtd->priv;

	return __spi_nand_do_read_page(mtd, page_addr, chip->page_size,
				       raw, corrected);
}


/**
 * __spi_nand_do_write_page - [INTERN] write data from buffer to flash
 * @mtd: MTD device structure
 * @page_addr: page address/raw address
 * @column :column address
 * @raw: without ecc or not
 *
 * write data from buffer pointed by chip->buf to flash
 */
static int __spi_nand_do_write_page(struct mtd_info *mtd, u32 page_addr,
				    u32 column, bool raw)
{
	struct spi_nand_chip *chip = mtd->priv;
	u8 status;
	bool p_fail = false;
	bool p_timeout = false;
	int ret = 0;

	fh_dev_debug(&chip->spi->dev, "Enter %s, with buf \n", __func__);

	if (raw) {
		ret = chip->disable_ecc(chip);
		if (ret < 0) {
			pr_debug("disable ecc failed\n");
			return ret;
		}
	}
	ret = chip->write_enable(chip);
	if (ret < 0) {
		pr_debug("write enable command failed\n");
		return ret;
	}
	/* Store the page to cache */
	ret = chip->store_cache(chip, page_addr, column,
				chip->page_size + chip->page_spare_size - column,
				chip->buf + column);
	if (ret < 0) {
		pr_debug("error %d storing page 0x%x to cache\n",
			 ret, page_addr);
		return ret;
	}
	/* Get page from the device cache into our internal buffer */
	ret = chip->write_page(chip, page_addr);
	if (ret < 0) {
		pr_debug("error %d reading page 0x%x from cache\n",
			 ret, page_addr);
		return ret;
	}
	ret = chip->waitfunc(chip, &status);
	if (ret < 0) {
		pr_info("error %d write page 0x%x timeout\n",
			 ret, page_addr);
		return ret;
	}
	if ((status & STATUS_P_FAIL_MASK) == STATUS_P_FAIL) {
		pr_debug("program page 0x%x failed\n", page_addr);
		p_fail = true;
	}

	if ((status & STATUS_OIP_MASK) == STATUS_BUSY) {
			pr_debug("program page 0x%x timeout\n", page_addr);
			p_timeout = true;
		}
	if (raw) {
		ret = chip->enable_ecc(chip);
		if (ret < 0) {
			pr_debug("enable ecc failed\n");
			return ret;
		}
	}
	if ((p_fail == true)||(p_timeout == true))
		ret = -EIO;

	return ret;
}

/**
 * spi_nand_do_write_page - [INTERN] write page from buffer to flash
 * @mtd: MTD device structure
 * @page_addr: page address/raw address
 * @raw: without ecc or not
 *
 * write page from buffer pointed by chip->buf to flash
 */
static int spi_nand_do_write_page(struct mtd_info *mtd, u32 page_addr,
				  bool raw)
{
	return __spi_nand_do_write_page(mtd, page_addr, 0, raw);
}

/**
 * spi_nand_do_write_page_oob - [INTERN] write oob from buffer to flash
 * @mtd: MTD device structure
 * @page_addr: page address/raw address
 * @raw: without ecc or not
 *
 * write oob from buffer pointed by chip->oobbuf to flash
 */
static int spi_nand_do_write_page_oob(struct mtd_info *mtd, u32 page_addr,
				      bool raw)
{
	struct spi_nand_chip *chip = mtd->priv;

	return __spi_nand_do_write_page(mtd, page_addr, chip->page_size, raw);
}


/**
 * spi_nand_transfer_oob - [INTERN] Transfer oob to client buffer
 * @chip: SPI-NAND device structure
 * @oob: oob destination address
 * @ops: oob ops structure
 * @len: size of oob to transfer
 */
static void spi_nand_transfer_oob(struct spi_nand_chip *chip, u8 *oob,
				  struct mtd_oob_ops *ops, size_t len)
{
	switch (ops->mode) {

	case MTD_OOB_PLACE: /*MTD_OPS_PLACE_OOB:*/
	case MTD_OOB_RAW: /*MTD_OPS_RAW:*/
		memcpy(oob, chip->oobbuf + ops->ooboffs, len);
		return;

	case MTD_OOB_AUTO: { /*MTD_OPS_AUTO_OOB:*/
		struct nand_oobfree *free = chip->ecclayout->oobfree;
		uint32_t boffs = 0, roffs = ops->ooboffs;
		size_t bytes = 0;

		for (; free->length && len; free++, len -= bytes) {
			/* Read request not from offset 0? */
			if (unlikely(roffs)) {
				if (roffs >= free->length) {
					roffs -= free->length;
					continue;
				}
				boffs = free->offset + roffs;
				bytes = min_t(size_t, len,
					      (free->length - roffs));
				roffs = 0;
			} else {
				bytes = min_t(size_t, len, free->length);
				boffs = free->offset;
			}
			memcpy(oob, chip->oobbuf + boffs, bytes);
			oob += bytes;
		}
		return;
	}
	default:
		BUG();
	}
}

/**
 * spi_nand_fill_oob - [INTERN] Transfer client buffer to oob
 * @chip: SPI-NAND device structure
 * @oob: oob data buffer
 * @len: oob data write length
 * @ops: oob ops structure
 */
static void spi_nand_fill_oob(struct spi_nand_chip *chip, uint8_t *oob,
			      size_t len, struct mtd_oob_ops *ops)
{
	fh_dev_debug(&chip->spi->dev, "Enter %s\n", __func__);
	memset(chip->oobbuf, 0xff, chip->page_spare_size);

	switch (ops->mode) {

	case MTD_OOB_PLACE:
	case MTD_OOB_RAW:
		memcpy(chip->oobbuf + ops->ooboffs, oob, len);
		return;

	case MTD_OOB_AUTO: {
		struct nand_oobfree *free = chip->ecclayout->oobfree;
		uint32_t boffs = 0, woffs = ops->ooboffs;
		size_t bytes = 0;

		for (; free->length && len; free++, len -= bytes) {
			/* Write request not from offset 0? */
			if (unlikely(woffs)) {
				if (woffs >= free->length) {
					woffs -= free->length;
					continue;
				}
				boffs = free->offset + woffs;
				bytes = min_t(size_t, len,
					      (free->length - woffs));
				woffs = 0;
			} else {
				bytes = min_t(size_t, len, free->length);
				boffs = free->offset;
			}
			memcpy(chip->oobbuf + boffs, oob, bytes);
			oob += bytes;
		}
		return;
	}
	default:
		BUG();
	}
}

/**
 * spi_nand_do_read_ops - [INTERN] Read data with ECC
 * @mtd: MTD device structure
 * @from: offset to read from
 * @ops: oob ops structure
 *
 * Internal function. Called with chip held.
 */
static int spi_nand_do_read_ops(struct mtd_info *mtd, loff_t from,
				struct mtd_oob_ops *ops)
{
	struct spi_nand_chip *chip = mtd->priv;
	int page_addr, page_offset, size;
	int ret;
	unsigned int corrected = 0;
	struct mtd_ecc_stats stats;
	unsigned int max_bitflips = 0;
	int readlen = ops->len;
	int oobreadlen = ops->ooblen;
	int ooblen = ops->mode == MTD_OOB_AUTO ?
		     mtd->oobavail : mtd->oobsize;

	fh_dev_debug(&chip->spi->dev, "Enter %s\n", __func__);

	/* Do not allow reads past end of device */
	if (unlikely(from >= mtd->size)) {
		pr_debug("%s: attempt to read beyond end of device\n",
			 __func__);
		return -EINVAL;
	}
	stats = mtd->ecc_stats;

	page_addr = from >> chip->page_shift;

	/* for main data */
	page_offset = from & chip->page_mask;
	ops->retlen = 0;

	/* for oob */
	if (oobreadlen > 0) {
		if (unlikely(ops->ooboffs >= ooblen)) {
			pr_debug("%s: attempt to start read outside oob\n",
				 __func__);
			return -EINVAL;
		}

		if (unlikely(ops->ooboffs + oobreadlen >
			     ((mtd->size >> chip->page_shift) - (from >> chip->page_shift))
			     * ooblen)) {
			pr_debug("%s: attempt to read beyond end of device\n",
				 __func__);
			return -EINVAL;
		}
		ooblen -= ops->ooboffs;
		ops->oobretlen = 0;
	}

	while (1) {
		if (page_addr != chip->pagebuf || oobreadlen > 0) {
			ret = spi_nand_do_read_page(mtd, page_addr,
						    ops->mode == MTD_OOB_RAW, &corrected);
			if (ret) {
				pr_debug("error %d reading page 0x%x\n",
					 ret, page_addr);
				return ret;
			}
			chip->pagebuf_bitflips = corrected;
			chip->pagebuf = page_addr;
		}
		max_bitflips = max(max_bitflips, chip->pagebuf_bitflips);
		size = min(readlen, chip->page_size - page_offset);
		memcpy(ops->datbuf + ops->retlen,
		       chip->buf + page_offset, size);

		ops->retlen += size;
		readlen -= size;
		page_offset = 0;

		if (unlikely(ops->oobbuf)) {
			size = min(oobreadlen, ooblen);
			spi_nand_transfer_oob(chip,
					      ops->oobbuf + ops->oobretlen, ops, size);

			ops->oobretlen += size;
			oobreadlen -= size;
		}
		if (!readlen)
			break;

		page_addr++;
	}

	if (mtd->ecc_stats.failed - stats.failed)
		return -EBADMSG;

	return max_bitflips;
}

/**
 * spi_nand_do_write_ops - [INTERN] SPI-NAND write with ECC
 * @mtd: MTD device structure
 * @to: offset to write to
 * @ops: oob operations description structure
 *
 */
static int spi_nand_do_write_ops(struct mtd_info *mtd, loff_t to,
				 struct mtd_oob_ops *ops)
{
	struct spi_nand_chip *chip = mtd->priv;
	int page_addr, page_offset, size;
	int writelen = ops->len;
	int oobwritelen = ops->ooblen;
	int ret;
	int ooblen = ops->mode == MTD_OOB_AUTO ?
		     mtd->oobavail : mtd->oobsize;

	fh_dev_debug(&chip->spi->dev, "Enter %s\n", __func__);

	/* Do not allow reads past end of device */
	if (unlikely(to >= mtd->size)) {
		pr_debug("%s: attempt to write beyond end of device\n",
			 __func__);
		return -EINVAL;
	}

	page_addr = to >> chip->page_shift;

	/* for main data */
	page_offset = to & chip->page_mask;
	ops->retlen = 0;

	/* for oob */
	if (oobwritelen > 0) {
		/* Do not allow write past end of page */
		if ((ops->ooboffs + oobwritelen) > ooblen) {
			pr_debug("%s: attempt to write past end of page\n",
				 __func__);
			return -EINVAL;
		}

		if (unlikely(ops->ooboffs >= ooblen)) {
			pr_debug("%s: attempt to start write outside oob\n",
				 __func__);
			return -EINVAL;
		}

		if (unlikely(ops->ooboffs + oobwritelen >
			     ((mtd->size >> chip->page_shift) - (to >> chip->page_shift))
			     * ooblen)) {
			pr_debug("%s: attempt to write beyond end of device\n",
				 __func__);
			return -EINVAL;
		}
		ooblen -= ops->ooboffs;
		ops->oobretlen = 0;
	}

	chip->pagebuf = -1;

	while (1) {
		memset(chip->buf, 0xFF,
		       chip->page_size + chip->page_spare_size);

		size = min(writelen, chip->page_size - page_offset);
		memcpy(chip->buf + page_offset,
		       ops->datbuf + ops->retlen, size);

		ops->retlen += size;
		writelen -= size;
		page_offset = 0;

		if (unlikely(ops->oobbuf)) {
			size = min(oobwritelen, ooblen);

			spi_nand_fill_oob(chip, ops->oobbuf + ops->oobretlen,
					  size, ops);

			ops->oobretlen += size;
			oobwritelen -= size;
		}
		ret = spi_nand_do_write_page(mtd, page_addr,
					     ops->mode == MTD_OOB_RAW);
		if (ret) {
			pr_debug("error %d writing page 0x%x\n",
				 ret, page_addr);
			return ret;
		}
		if (!writelen)
			break;
		page_addr++;
	}
	return 0;
}

/**
 * nand_read - [MTD Interface] SPI-NAND read
 * @mtd: MTD device structure
 * @from: offset to read from
 * @len: number of bytes to read
 * @retlen: pointer to variable to store the number of read bytes
 * @buf: the databuffer to put data
 *
 */
static int spi_nand_read(struct mtd_info *mtd, loff_t from, size_t len,
			 size_t *retlen, u_char *buf)
{
	struct mtd_oob_ops ops = { 0 };
	int ret;

	spi_nand_get_device(mtd, FL_READING);

	ops.len = len;
	ops.datbuf = buf;
	ret = spi_nand_do_read_ops(mtd, from, &ops);

	*retlen = ops.retlen;

	spi_nand_release_device(mtd);

	return ret;
}

/**
 * spi_nand_write - [MTD Interface] SPI-NAND write
 * @mtd: MTD device structure
 * @to: offset to write to
 * @len: number of bytes to write
 * @retlen: pointer to variable to store the number of written bytes
 * @buf: the data to write
 *
 */
static int spi_nand_write(struct mtd_info *mtd, loff_t to, size_t len,
			  size_t *retlen, const u_char *buf)
{
	struct mtd_oob_ops ops = {0};
	int ret;

	spi_nand_get_device(mtd, FL_WRITING);

	ops.len = len;
	ops.datbuf = (uint8_t *)buf;


	ret =  spi_nand_do_write_ops(mtd, to, &ops);

	*retlen = ops.retlen;

	spi_nand_release_device(mtd);

	return ret;
}

/**
 * spi_nand_do_read_oob - [INTERN] SPI-NAND read out-of-band
 * @mtd: MTD device structure
 * @from: offset to read from
 * @ops: oob operations description structure
 *
 * SPI-NAND read out-of-band data from the spare area.
 */
static int spi_nand_do_read_oob(struct mtd_info *mtd, loff_t from,
				struct mtd_oob_ops *ops)
{
	struct spi_nand_chip *chip = mtd->priv;
	int page_addr;
	int corrected = 0;
	struct mtd_ecc_stats stats;
	int readlen = ops->ooblen;
	int len;
	int ret = 0;

	fh_dev_debug(&chip->spi->dev, "Enter %s\n", __func__);

	pr_debug("%s: from = 0x%08Lx, len = %i\n",
		 __func__, (unsigned long long)from, readlen);

	stats = mtd->ecc_stats;

	len = ops->mode == MTD_OOB_AUTO ? mtd->oobavail : mtd->oobsize;

	if (unlikely(ops->ooboffs >= len)) {
		pr_debug("%s: attempt to start read outside oob\n",
			 __func__);
		return -EINVAL;
	}

	/* Do not allow reads past end of device */
	if (unlikely(from >= mtd->size ||
		     ops->ooboffs + readlen > ((mtd->size >> chip->page_shift) -
					       (from >> chip->page_shift)) * len)) {
		pr_debug("%s: attempt to read beyond end of device\n",
			 __func__);
		return -EINVAL;
	}

	/* Shift to get page */
	page_addr = (from >> chip->page_shift);
	len -= ops->ooboffs;
	ops->oobretlen = 0;

	while (1) {
		/*read data from chip*/
		ret = spi_nand_do_read_page_oob(mtd, page_addr,
						ops->mode == MTD_OOB_RAW, &corrected);
		if (ret) {
			pr_debug("error %d reading page 0x%x\n",
				 ret, page_addr);
			return ret;
		}
		if (page_addr == chip->pagebuf)
			chip->pagebuf = -1;

		len = min(len, readlen);
		spi_nand_transfer_oob(chip, ops->oobbuf + ops->oobretlen,
				      ops, len);

		readlen -= len;
		ops->oobretlen += len;
		if (!readlen)
			break;

		page_addr++;
	}

	if (ret < 0)
		return ret;

	if (mtd->ecc_stats.failed - stats.failed)
		return -EBADMSG;

	return  mtd->ecc_stats.corrected - stats.corrected ? -EUCLEAN : 0;
}

/**
 * spi_nand_do_write_oob - [MTD Interface] SPI-NAND write out-of-band
 * @mtd: MTD device structure
 * @to: offset to write to
 * @ops: oob operation description structure
 *
 * SPI-NAND write out-of-band.
 */
static int spi_nand_do_write_oob(struct mtd_info *mtd, loff_t to,
				 struct mtd_oob_ops *ops)
{
	int page_addr, len, ret;
	struct spi_nand_chip *chip = mtd->priv;
	int writelen = ops->ooblen;

	fh_dev_debug(&chip->spi->dev, "Enter %s\n", __func__);

	pr_debug("%s: to = 0x%08x, len = %i\n",
		 __func__, (unsigned int)to, (int)writelen);

	len = ops->mode == MTD_OOB_AUTO ? mtd->oobavail : mtd->oobsize;

	/* Do not allow write past end of page */
	if ((ops->ooboffs + writelen) > len) {
		pr_debug("%s: attempt to write past end of page\n",
			 __func__);
		return -EINVAL;
	}

	if (unlikely(ops->ooboffs >= len)) {
		pr_debug("%s: attempt to start write outside oob\n",
			 __func__);
		return -EINVAL;
	}

	/* Do not allow write past end of device */
	if (unlikely(to >= mtd->size ||
		     ops->ooboffs + writelen >
		     ((mtd->size >> chip->page_shift) -
		      (to >> chip->page_shift)) * len)) {
		pr_debug("%s: attempt to write beyond end of device\n",
			 __func__);
		return -EINVAL;
	}

	/* Shift to get page */
	page_addr = to >> chip->page_shift;
	/* Invalidate the page cache, if we write to the cached page */
	if (page_addr == chip->pagebuf)
		chip->pagebuf = -1;

	spi_nand_fill_oob(chip, ops->oobbuf, writelen, ops);

	ret = spi_nand_do_write_page_oob(mtd, page_addr,
					 ops->mode == MTD_OOB_RAW);
	if (ret) {
		pr_debug("error %d writing page 0x%x\n",
			 ret, page_addr);
		return ret;
	}
	ops->oobretlen = writelen;

	return 0;
}

/**
 * spi_nand_read_oob - [MTD Interface] SPI-NAND read data and/or out-of-band
 * @mtd: MTD device structure
 * @from: offset to read from
 * @ops: oob operation description structure
 *
 * SPI-NAND read data and/or out-of-band data.
 */
static int spi_nand_read_oob(struct mtd_info *mtd, loff_t from,
			     struct mtd_oob_ops *ops)
{
	int ret = -ENOTSUPP;

	struct spi_nand_chip *this = mtd->priv;
	fh_dev_debug(&this->spi->dev, "Enter %s, from 0x%08llx \n", __func__, from);
	ops->retlen = 0;

	/* Do not allow reads past end of device */
	if (ops->datbuf && (from + ops->len) > mtd->size) {
		pr_debug("%s: attempt to read beyond end of device\n",
			 __func__);
		return -EINVAL;
	}

	spi_nand_get_device(mtd, FL_READING);

	switch (ops->mode) {
	case MTD_OOB_PLACE:
	case MTD_OOB_AUTO:
	case MTD_OOB_RAW:
		break;

	default:
		goto out;
	}

	if (!ops->datbuf)
		ret = spi_nand_do_read_oob(mtd, from, ops);
	else
		ret = spi_nand_do_read_ops(mtd, from, ops);

out:
	spi_nand_release_device(mtd);

	return ret;
}

/**
 * spi_nand_write_oob - [MTD Interface] SPI-NAND write data and/or out-of-band
 * @mtd: MTD device structure
 * @to: offset to write to
 * @ops: oob operation description structure
 */
static int spi_nand_write_oob(struct mtd_info *mtd, loff_t to,
			      struct mtd_oob_ops *ops)
{
	int ret = -ENOTSUPP;
	struct spi_nand_chip *this = mtd->priv;
	fh_dev_debug(&this->spi->dev, "Enter %s\n", __func__);

	ops->retlen = 0;

	/* Do not allow writes past end of device */
	if (ops->datbuf && (to + ops->len) > mtd->size) {
		pr_debug("%s: attempt to write beyond end of device\n",
			 __func__);
		return -EINVAL;
	}

	spi_nand_get_device(mtd, FL_WRITING);

	switch (ops->mode) {
	case MTD_OOB_PLACE:
	case MTD_OOB_AUTO:
	case MTD_OOB_RAW:
		break;

	default:
		goto out;
	}

	if (!ops->datbuf)
		ret = spi_nand_do_write_oob(mtd, to, ops);
	else
		ret = spi_nand_do_write_ops(mtd, to, ops);

out:
	spi_nand_release_device(mtd);

	return ret;
}

/**
 * spi_nand_block_bad - [INTERN] Check if block at offset is bad
 * @mtd: MTD device structure
 * @offs: offset relative to mtd start
 */
static int spi_nand_block_bad(struct mtd_info *mtd, loff_t ofs)
{
	struct spi_nand_chip *chip = mtd->priv;
	struct mtd_oob_ops ops = {0};
	u32 block_addr;
	u8 bad[2] = {0, 0};
	u8 ret = 0;

	block_addr = ofs >> chip->block_shift;
	ops.mode = MTD_OOB_PLACE;
	ops.ooblen = 2;
	ops.oobbuf = bad;

	ret = spi_nand_do_read_oob(mtd, block_addr << chip->block_shift, &ops);
	if (bad[0] != 0xFF || bad[1] != 0xFF)
		ret =  1;

	return ret;

}

/**
 * spi_nand_block_checkbad - [GENERIC] Check if a block is marked bad
 * @mtd: MTD device structure
 * @ofs: offset from device start
 * @allowbbt: 1, if its allowed to access the bbt area
 *
 * Check, if the block is bad. Either by reading the bad block table or
 * calling of the scan function.
 */
static int spi_nand_block_checkbad(struct mtd_info *mtd, loff_t ofs,
				   int allowbbt)
{
	struct spi_nand_chip *chip = mtd->priv;

	if (!chip->bbt)
		return spi_nand_block_bad(mtd, ofs);

	/* Return info from the table */
	return spi_nand_isbad_bbt(mtd, ofs, allowbbt);
}

/**
 * spi_nand_block_isbad - [MTD Interface] Check if block at offset is bad
 * @mtd: MTD device structure
 * @offs: offset relative to mtd start
 */
static int spi_nand_block_isbad(struct mtd_info *mtd, loff_t offs)
{
	struct spi_nand_chip *chip = mtd->priv;
	return chip->block_bad(mtd, offs, 0);
}

/**
 * spi_nand_block_markbad_lowlevel - mark a block bad
 * @mtd: MTD device structure
 * @ofs: offset from device start
 *
 * This function performs the generic bad block marking steps (i.e., bad
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
static int spi_nand_block_markbad_lowlevel(struct mtd_info *mtd, loff_t ofs)
{
	struct spi_nand_chip *chip = mtd->priv;
	struct mtd_oob_ops ops = {0};
	struct erase_info einfo = {0};
	u32 block_addr;
	u8 buf[2] = {0, 0};
	int res, ret = 0;

	if (!(chip->bbt_options & NAND_BBT_NO_OOB_BBM)) {
		/*erase bad block before mark bad block*/
		einfo.mtd = mtd;
		einfo.addr = ofs;
		einfo.len = 1UL << chip->block_shift;
		spi_nand_erase(mtd, &einfo);

		block_addr = ofs >> chip->block_shift;
		ops.mode = MTD_OOB_PLACE;
		ops.ooblen = 2;
		ops.oobbuf = buf;

		ret = spi_nand_do_write_oob(mtd,
					    block_addr << chip->block_shift, &ops);
	}

	/* Mark block bad in BBT */
	if (chip->bbt) {
		res = spi_nand_markbad_bbt(mtd, ofs);
		if (!ret)
			ret = res;
	}

	if (!ret)
		mtd->ecc_stats.badblocks++;

	return ret;
}

/**
 * spi_nand_block_markbad - [MTD Interface] Mark block at the given offset
 * as bad
 * @mtd: MTD device structure
 * @ofs: offset relative to mtd start
 */
static int spi_nand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	int ret;

	ret = spi_nand_block_isbad(mtd, ofs);
	if (ret) {
		/* If it was bad already, return success and do nothing */
		if (ret > 0)
			return 0;
		return ret;
	}

	return spi_nand_block_markbad_lowlevel(mtd, ofs);
}

/**
 * __spi_nand_erase - [INTERN] erase block(s)
 * @mtd: MTD device structure
 * @einfo: erase instruction
 * @allowbbt: allow to access bbt
 *
 * Erase one ore more blocks
 */
int __spi_nand_erase(struct mtd_info *mtd, struct erase_info *einfo,
		     int allowbbt)
{
	struct spi_nand_chip *chip = mtd->priv;
	int page_addr, pages_per_block;
	loff_t len;
	u8 status;
	int ret = 0;


	/* check address align on block boundary */
	if (einfo->addr & (chip->block_size - 1)) {
		pr_debug("%s: Unaligned address\n", __func__);
		return -EINVAL;
	}

	if (einfo->len & (chip->block_size - 1)) {
		pr_debug("%s: Length not block aligned\n", __func__);
		return -EINVAL;
	}

	/* Do not allow erase past end of device */
	if ((einfo->len + einfo->addr) > chip->size) {
		pr_debug("%s: Erase past end of device\n", __func__);
		return -EINVAL;
	}

	einfo->fail_addr = MTD_FAIL_ADDR_UNKNOWN;

	/* Grab the lock and see if the device is available */
	spi_nand_get_device(mtd, FL_ERASING);

	pages_per_block = 1 << (chip->block_shift - chip->page_shift);
	page_addr = einfo->addr >> chip->page_shift;
	len = einfo->len;

	einfo->state = MTD_ERASING;

	while (len) {
		/* Check if we have a bad block, we do not erase bad blocks! */
		if (chip->block_bad(mtd, ((loff_t) page_addr) <<
					    chip->page_shift, allowbbt)) {
			pr_warn("%s: attempt to erase a bad block at page 0x%08x\n",
				__func__, page_addr);
			einfo->state = MTD_ERASE_FAILED;
			goto erase_exit;
		}
		/*
		 * Invalidate the page cache, if we erase the block which
		 * contains the current cached page.
		 */
		if (page_addr <= chip->pagebuf && chip->pagebuf <
		    (page_addr + pages_per_block))
			chip->pagebuf = -1;

		ret = chip->write_enable(chip);
		if (ret < 0) {
			pr_debug("write enable command failed\n");
			einfo->state = MTD_ERASE_FAILED;
			goto erase_exit;
		}

		ret = chip->erase_block(chip, page_addr);
		if (ret < 0) {
			pr_debug("block erase command failed\n");
			einfo->state = MTD_ERASE_FAILED;
			einfo->fail_addr = (loff_t)page_addr
					   << chip->page_shift;
			goto erase_exit;
		}
		ret = chip->waitfunc(chip, &status);
		if (ret < 0) {
			pr_debug("block erase command wait failed\n");
			einfo->state = MTD_ERASE_FAILED;
			goto erase_exit;
		}
		if ((status & STATUS_E_FAIL_MASK) == STATUS_E_FAIL) {
			pr_debug("erase block 0x%012llx failed\n",
				 ((loff_t) page_addr) << chip->page_shift);
			einfo->state = MTD_ERASE_FAILED;
			einfo->fail_addr = (loff_t)page_addr
					   << chip->page_shift;
			goto erase_exit;
		}

		/* Increment page address and decrement length */
		len -= (1ULL << chip->block_shift);
		page_addr += pages_per_block;
	}

	einfo->state = MTD_ERASE_DONE;

erase_exit:

	ret = einfo->state == MTD_ERASE_DONE ? 0 : -EIO;

	spi_nand_release_device(mtd);

	/* Do call back function */
	if (!ret)
		mtd_erase_callback(einfo);

	/* Return more or less happy */
	return ret;
}
EXPORT_SYMBOL(__spi_nand_erase);

/**
 * spi_nand_erase - [MTD Interface] erase block(s)
 * @mtd: MTD device structure
 * @einfo: erase instruction
 *
 * Erase one ore more blocks
 */
static int spi_nand_erase(struct mtd_info *mtd, struct erase_info *einfo)
{
	return __spi_nand_erase(mtd, einfo, 0);
}

/**
 * spi_nand_sync - [MTD Interface] sync
 * @mtd: MTD device structure
 *
 * Sync is actually a wait for chip ready function
 */
static void spi_nand_sync(struct mtd_info *mtd)
{
	pr_debug("spi_nand_sync: called\n");

	/* Grab the lock and see if the device is available */
	spi_nand_get_device(mtd, FL_SYNCING);

	/* Release it and go back */
	spi_nand_release_device(mtd);
}

/**
 * spi_nand_suspend - [MTD Interface] Suspend the SPI-NAND flash
 * @mtd: MTD device structure
 */
static int spi_nand_suspend(struct mtd_info *mtd)
{
	return spi_nand_get_device(mtd, FL_PM_SUSPENDED);
}

/**
 * spi_nand_resume - [MTD Interface] Resume the SPI-NAND flash
 * @mtd: MTD device structure
 */
static void spi_nand_resume(struct mtd_info *mtd)
{
	struct spi_nand_chip *this = mtd->priv;

	if (this->state == FL_PM_SUSPENDED)
		spi_nand_release_device(mtd);
	else
		pr_err("%s is not called in suspended state\n:", __func__);
}


/*
 * spi_nand_send_cmd - to process a command to send to the SPI-NAND
 * @spi: spi device structure
 * @cmd: command structure
 *
 *    Set up the command buffer to send to the SPI controller.
 *    The command buffer has to initialized to 0.
 */
int spi_nand_send_cmd(struct spi_device *spi, struct spi_nand_cmd *cmd)
{
	struct spi_message message;
	struct spi_transfer x[4];
	u8 buf[8],  i;
	u32 buflen = 0;

	spi_message_init(&message);
	memset(x, 0, sizeof(x));
	x[0].len = 1;
	x[0].tx_buf = &cmd->cmd;
	x[0].xfer_wire_mode = ONE_WIRE_SUPPORT;
	spi_message_add_tail(&x[0], &message);

	buf[buflen++] = cmd->cmd;

	if (cmd->n_addr) {
		x[1].len = cmd->n_addr;
		x[1].tx_buf = cmd->addr;
		spi_message_add_tail(&x[1], &message);
		x[1].xfer_wire_mode = ONE_WIRE_SUPPORT;
	}
	for (i = 0; i < cmd->n_addr && buflen < 7; i++)
		buf[buflen++] = cmd->addr[i];

	if (cmd->n_tx) {
		x[2].len = cmd->n_tx;
		/*x[2].tx_nbits = cmd->tx_nbits; always 0 for single future version*/
		x[2].tx_buf = cmd->tx_buf;
		x[2].xfer_wire_mode = ONE_WIRE_SUPPORT;
		spi_message_add_tail(&x[2], &message);
	}
	for (i = 0; i < cmd->n_tx && buflen < 7; i++)
		buf[buflen++] = cmd->tx_buf[i];

	if (cmd->n_rx) {
		x[3].len = cmd->n_rx;
		/*x[3].rx_nbits = cmd->rx_nbits;*/
		x[3].rx_buf = cmd->rx_buf;
		if (cmd->cmd == SPINAND_CMD_READ_CACHE_X4)
			x[3].xfer_wire_mode = QUAD_WIRE_SUPPORT;
		else if (cmd->cmd == SPINAND_CMD_READ_CACHE_X2)
			x[3].xfer_wire_mode = DUAL_WIRE_SUPPORT;
		else
		    x[3].xfer_wire_mode = ONE_WIRE_SUPPORT;
		spi_message_add_tail(&x[3], &message);
	}
	for (i = 0; i < cmd->n_rx && buflen < 7; i++)
		buf[buflen++] = cmd->rx_buf[i];

	buflen = 1 + cmd->n_addr + cmd->n_tx + cmd->n_rx;
	if (fh_start_debug || cmd->cmd == 0x1f)
	fh_dev_debug(&spi->dev, " spi%d:%d: send cmd 0x: %02x %02x %02x %02x %02x %02x, size %d\n",
			spi->master->bus_num, spi->chip_select,
		     buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buflen);

	return spi_sync(spi, &message);
}
EXPORT_SYMBOL(spi_nand_send_cmd);
/*
 * spi_nand_read_status- send command 0x0f to the SPI-NAND status register value
 * @spi: spi device structure
 * @status: buffer to store value
 * Description:
 *    After read, write, or erase, the Nand device is expected to set the
 *    busy status.
 *    This function is to allow reading the status of the command: read,
 *    write, and erase.
 *    Once the status turns to be ready, the other status bits also are
 *    valid status bits.
 */
static int spi_nand_read_status(struct spi_device *spi, uint8_t *status)
{
	struct spi_nand_cmd cmd = {0};
	int ret;

	cmd.cmd = SPINAND_CMD_READ_REG;
	cmd.n_addr = 1;
	cmd.addr[0] = REG_STATUS;
	cmd.n_rx = 1;
	cmd.rx_buf = status;

	ret = spi_nand_send_cmd(spi, &cmd);
	if (ret < 0)
		dev_err(&spi->dev, "err: %d read status register\n", ret);

	return ret;
}

/**
 * spi_nand_get_otp- send command 0x0f to read the SPI-NAND OTP register
 * @spi: spi device structure
 * @opt: buffer to store value
 * Description:
 *   There is one bit( bit 0x10 ) to set or to clear the internal ECC.
 *   Enable chip internal ECC, set the bit to 1
 *   Disable chip internal ECC, clear the bit to 0
 */
static int spi_nand_get_otp(struct spi_device *spi, u8 *otp)
{
	struct spi_nand_cmd cmd = {0};
	int ret;

	cmd.cmd = SPINAND_CMD_READ_REG;
	cmd.n_addr = 1;
	cmd.addr[0] = REG_OTP;
	cmd.n_rx = 1;
	cmd.rx_buf = otp;

	ret = spi_nand_send_cmd(spi, &cmd);
	if (ret < 0)
		dev_err(&spi->dev, "error %d get otp\n", ret);
	return ret;
}

/**
 * spi_nand_set_otp- send command 0x1f to write the SPI-NAND OTP register
 * @spi: spi device structure
 * @status: buffer stored value
 * Description:
 *   There is one bit( bit 0x10 ) to set or to clear the internal ECC.
 *   Enable chip internal ECC, set the bit to 1
 *   Disable chip internal ECC, clear the bit to 0
 */
static int spi_nand_set_otp(struct spi_device *spi, u8 *otp)
{
	int ret;
	struct spi_nand_cmd cmd = { 0 };

	cmd.cmd = SPINAND_CMD_WRITE_REG;
	cmd.n_addr = 1;
	cmd.addr[0] = REG_OTP;
	cmd.n_tx = 1;
	cmd.tx_buf = otp;

	ret = spi_nand_send_cmd(spi, &cmd);
	if (ret < 0)
		dev_err(&spi->dev, "error %d set otp\n", ret);

	return ret;
}

/**
 * spi_nand_enable_ecc- enable internal ECC
 * @chip: SPI-NAND device structure
 * Description:
 *   There is one bit( bit 0x10 ) to set or to clear the internal ECC.
 *   Enable chip internal ECC, set the bit to 1
 *   Disable chip internal ECC, clear the bit to 0
 */
static int spi_nand_enable_ecc(struct spi_nand_chip *chip)
{
	struct spi_device *spi = chip->spi;
	int ret;
	u8 otp = 0;

	fh_dev_debug(&spi->dev, "Enter %s\n", __func__);

	ret = spi_nand_get_otp(spi, &otp);
	if (ret < 0)
		return ret;

	fh_dev_debug(&spi->dev, "get opt: 0x%02x\n", otp);
	if ((otp & OTP_ECC_MASK) == OTP_ECC_ENABLE)
		return 0;

	otp |= OTP_ECC_ENABLE;
	ret = spi_nand_set_otp(spi, &otp);
	if (ret < 0)
		return ret;
	fh_dev_debug(&spi->dev, "set opt: 0x%02x\n", otp);
	return spi_nand_get_otp(spi, &otp);
}

static int spi_nand_get_qe_value(struct spi_nand_chip *chip, u8 *otp)
{
	struct spi_nand_cmd cmd = {0};
	int ret;

	cmd.cmd = SPINAND_CMD_READ_REG;
	cmd.n_addr = 1;
	cmd.addr[0] = chip->qe_addr;
	cmd.n_rx = 1;
	cmd.rx_buf = otp;

	ret = spi_nand_send_cmd(chip->spi, &cmd);
	return ret;
}

static int spi_nand_set_qe_value(struct spi_nand_chip *chip, u8 *otp)
{
	int ret;
	struct spi_nand_cmd cmd = { 0 };

	cmd.cmd = SPINAND_CMD_WRITE_REG;
	cmd.n_addr = 1;
	cmd.addr[0] = chip->qe_addr;
	cmd.n_tx = 1;
	cmd.tx_buf = otp;

	ret = spi_nand_send_cmd(chip->spi, &cmd);
	return ret;
}



static int spi_nand_set_qe(struct spi_nand_chip *chip)
{
	int ret;
	u8 otp = 0;

	ret = spi_nand_get_qe_value(chip, &otp);
	if (ret < 0)
		return ret;

	if (chip->qe_flag)
	    otp |= chip->qe_mask;
	else
		otp &= (~chip->qe_mask);
	ret = spi_nand_set_qe_value(chip, &otp);
	if (ret < 0)
		return ret;
	return spi_nand_get_qe_value(chip, &otp);
}


/**
 * spi_nand_disable_ecc- disable internal ECC
 * @chip: SPI-NAND device structure
 * Description:
 *   There is one bit( bit 0x10 ) to set or to clear the internal ECC.
 *   Enable chip internal ECC, set the bit to 1
 *   Disable chip internal ECC, clear the bit to 0
 */
static int spi_nand_disable_ecc(struct spi_nand_chip *chip)
{
	struct spi_device *spi = chip->spi;
	int ret;
	u8 otp = 0;

	fh_dev_debug(&spi->dev, "Enter %s\n", __func__);
	ret = spi_nand_get_otp(spi, &otp);
	if (ret < 0)
		return ret;

	if ((otp & OTP_ECC_MASK) == OTP_ECC_ENABLE) {
		otp &= ~OTP_ECC_ENABLE;
		ret = spi_nand_set_otp(spi, &otp);
		if (ret < 0)
			return ret;
		return spi_nand_get_otp(spi, &otp);
	} else
		return 0;
}

/**
 * spi_nand_write_enable- send command 0x06 to enable write or erase the
 * Nand cells
 * @chip: SPI-NAND device structure
 * Description:
 *   Before write and erase the Nand cells, the write enable has to be set.
 *   After the write or erase, the write enable bit is automatically
 *   cleared (status register bit 2)
 *   Set the bit 2 of the status register has the same effect
 */
static int spi_nand_write_enable(struct spi_nand_chip *chip)
{
	struct spi_nand_cmd cmd = {0};
	struct spi_device *spi = chip->spi;
	fh_dev_debug(&spi->dev, "Enter %s\n", __func__);

	cmd.cmd = SPINAND_CMD_WR_ENABLE;
	return spi_nand_send_cmd(spi, &cmd);
}

/*
 * spi_nand_read_from_cache- send command 0x13 to read data from Nand to cache
 * @chip: SPI-NAND device structure
 * @page_addr: page to read
 */
static int spi_nand_read_page_to_cache(struct spi_nand_chip *chip,
				       u32 page_addr)
{
	struct spi_nand_cmd cmd = {0};
	struct spi_device *spi = chip->spi;
	fh_dev_debug(&spi->dev, "Enter %s\n", __func__);

	cmd.cmd = SPINAND_CMD_READ;
	cmd.n_addr = 3;
	cmd.addr[0] = (u8)(page_addr >> 16);
	cmd.addr[1] = (u8)(page_addr >> 8);
	cmd.addr[2] = (u8)page_addr;

	return spi_nand_send_cmd(spi, &cmd);
}

/*
 * spi_nand_read_from_cache- send command 0x03 to read out the data from the
 * cache register
 * Description:
 *   The read can specify 1 to (page size + spare size) bytes of data read at
 *   the corresponding locations.
 *   No tRd delay.
 */
int spi_nand_read_from_cache(struct spi_nand_chip *chip, u32 page_addr,
			     u32 column, size_t len, u8 *rbuf)
{
	struct spi_nand_cmd cmd = {0};
	struct spi_device *spi = chip->spi;
	fh_dev_debug(&spi->dev, "Enter %s\n", __func__);

	/*cmd.cmd = SPINAND_CMD_READ_RDM;*/
	    cmd.n_addr = 3;
		if (spi->dev_open_multi_wire_flag & QUAD_WIRE_SUPPORT) {
			cmd.cmd = SPINAND_CMD_READ_CACHE_X4;
			if (chip->multi_wire_command_length == 5)
				cmd.n_addr = 4;
		} else if (spi->dev_open_multi_wire_flag & DUAL_WIRE_SUPPORT) {
			cmd.cmd = SPINAND_CMD_READ_CACHE_X2;
			if (chip->multi_wire_command_length == 5)
				cmd.n_addr = 4;
		} else
				 cmd.cmd = SPINAND_CMD_READ_RDM;
	if (chip->dev_id[0] == 0xC8) {/*FIXME: early GD chips, test 1G*/
		cmd.addr[0] = 0;
		cmd.addr[1] = (u8)(column >> 8);
		if (chip->options & SPINAND_NEED_PLANE_SELECT)
			cmd.addr[0] |= (u8)(((page_addr >>
				(chip->block_shift - chip->page_shift)) & 0x1) << 4);
		cmd.addr[2] = (u8)column;
	}
	else{
		cmd.addr[0] = (u8)(column >> 8);
		if (chip->options & SPINAND_NEED_PLANE_SELECT)
			cmd.addr[0] |= (u8)(((page_addr >>
					      (chip->block_shift - chip->page_shift)) & 0x1) << 4);
		cmd.addr[1] = (u8)column;
		cmd.addr[2] = 0;
	}
	cmd.n_rx = len;
	cmd.rx_buf = rbuf;

	return spi_nand_send_cmd(spi, &cmd);
}

/*
 * spi_nand_read_from_cache_x2- send command 0x3b to read out the data from the
 * cache register
 * Description:
 *   The read can specify 1 to (page size + spare size) bytes of data read at
 *   the corresponding locations.
 *   No tRd delay.
 */
/*int spi_nand_read_from_cache_x2(struct spi_nand_chip *chip, u32 page_addr,
		u32 column, size_t len, u8 *rbuf)
{
	struct spi_nand_cmd cmd = {0};
	struct spi_device *spi = chip->spi;

	cmd.cmd = SPINAND_CMD_READ_CACHE_X2;
	cmd.n_addr = 3;
	cmd.addr[0] = (u8)(column >> 8);
	if (chip->options & SPINAND_NEED_PLANE_SELECT)
		cmd.addr[0] |= (u8)(((page_addr >>
			(chip->block_shift - chip->page_shift)) & 0x1) << 4);
	cmd.addr[1] = (u8)column;
	cmd.addr[2] = 0;
	cmd.n_rx = len;
	cmd.rx_nbits = SPI_NBITS_DUAL;
	cmd.rx_buf = rbuf;

	return spi_nand_send_cmd(spi, &cmd);
}*/

/*
 * spi_nand_read_from_cache_x4- send command 0x6b to read out the data from the
 * cache register
 * Description:
 *   The read can specify 1 to (page size + spare size) bytes of data read at
 *   the corresponding locations.
 *   No tRd delay.
 */
/*int spi_nand_read_from_cache_x4(struct spi_nand_chip *chip, u32 page_addr,
		u32 column, size_t len, u8 *rbuf)
{
	struct spi_nand_cmd cmd = {0};
	struct spi_device *spi = chip->spi;

	cmd.cmd = SPINAND_CMD_READ_CACHE_X4;
	cmd.n_addr = 3;
	cmd.addr[0] = (u8)(column >> 8);
	if (chip->options & SPINAND_NEED_PLANE_SELECT)
		cmd.addr[0] |= (u8)(((page_addr >>
			(chip->block_shift - chip->page_shift)) & 0x1) << 4);
	cmd.addr[1] = (u8)column;
	cmd.addr[2] = 0;
	cmd.n_rx = len;
	cmd.rx_nbits = SPI_NBITS_QUAD;
	cmd.rx_buf = rbuf;

	return spi_nand_send_cmd(spi, &cmd);
}*/

/*
 * spi_nand_read_from_cache_snor_protocol- send command 0x03 to read out the
 * data from the cache register, 0x03 command protocol is same as SPI NOR
 * read command
 * Description:
 *   The read can specify 1 to (page size + spare size) bytes of data read at
 *   the corresponding locations.
 *   No tRd delay.
 */
int spi_nand_read_from_cache_snor_protocol(struct spi_nand_chip *chip,
		u32 page_addr, u32 column, size_t len, u8 *rbuf)
{
	struct spi_nand_cmd cmd = {0};
	struct spi_device *spi = chip->spi;
	fh_dev_debug(&spi->dev, "Enter %s\n", __func__);

	cmd.cmd = SPINAND_CMD_READ_RDM;
	cmd.n_addr = 3;
	cmd.addr[0] = 0;
	cmd.addr[1] = (u8)(column >> 8);
	if (chip->options & SPINAND_NEED_PLANE_SELECT)
		cmd.addr[1] |= (u8)(((page_addr >>
				      (chip->block_shift - chip->page_shift)) & 0x1) << 4);
	cmd.addr[2] = (u8)column;
	cmd.n_rx = len;
	cmd.rx_buf = rbuf;

	return spi_nand_send_cmd(spi, &cmd);
}
EXPORT_SYMBOL(spi_nand_read_from_cache_snor_protocol);

/*
 * spi_nand_program_data_to_cache--to write a page to cache
 * @chip: SPI-NAND device structure
 * @page_addr: page to write
 * @column: the location to write to the cache
 * @len: number of bytes to write
 * wrbuf: buffer held @len bytes
 *
 * Description:
 *   The write command used here is 0x02--indicating that the cache is
 *   cleared first.
 *   Since it is writing the data to cache, there is no tPROG time.
 */
static int spi_nand_program_data_to_cache(struct spi_nand_chip *chip,
		u32 page_addr, u32 column, size_t len, u8 *wbuf)
{
	struct spi_nand_cmd cmd = {0};
	struct spi_device *spi = chip->spi;
	fh_dev_debug(&spi->dev, "Enter %s\n", __func__);

	cmd.cmd = SPINAND_CMD_PROG_LOAD;
	cmd.n_addr = 2;
	cmd.addr[0] = (u8)(column >> 8);
	if (chip->options & SPINAND_NEED_PLANE_SELECT)
		cmd.addr[0] |= (u8)(((page_addr >>
	      (chip->block_shift - chip->page_shift)) & 0x1) << 4);
	cmd.addr[1] = (u8)column;
	cmd.n_tx = len;
	cmd.tx_buf = wbuf;
	fh_dev_debug(&spi->dev, "see n_tx %d,  oob[4] 0x%08x\n", 
		len, *(uint32_t*)(wbuf+2048));

	return spi_nand_send_cmd(spi, &cmd);
}

/**
 * spi_nand_program_execute--to write a page from cache to the Nand array
 * @chip: SPI-NAND device structure
 * @page_addr: the physical page location to write the page.
 *
 * Description:
 *   The write command used here is 0x10--indicating the cache is writing to
 *   the Nand array.
 *   Need to wait for tPROG time to finish the transaction.
 */
static int spi_nand_program_execute(struct spi_nand_chip *chip, u32 page_addr)
{
	struct spi_nand_cmd cmd = {0};
	struct spi_device *spi = chip->spi;

	fh_dev_debug(&spi->dev, "Enter %s\n", __func__);
	cmd.cmd = SPINAND_CMD_PROG;
	cmd.n_addr = 3;
	cmd.addr[0] = (u8)(page_addr >> 16);
	cmd.addr[1] = (u8)(page_addr >> 8);
	cmd.addr[2] = (u8)page_addr;


	return spi_nand_send_cmd(spi, &cmd);
}


/**
 * spi_nand_erase_block_erase--to erase a block
 * @chip: SPI-NAND device structure
 * @page_addr: the page to erase.
 *
 * Description:
 *   The command used here is 0xd8--indicating an erase command to erase
 *   one block
 *   Need to wait for tERS.
 */
static int spi_nand_erase_block(struct spi_nand_chip *chip,
				u32 page_addr)
{
	struct spi_nand_cmd cmd = {0};
	struct spi_device *spi = chip->spi;
	fh_dev_debug(&spi->dev, "Enter %s\n", __func__);

	cmd.cmd = SPINAND_CMD_ERASE_BLK;
	cmd.n_addr = 3;
	cmd.addr[0] = (u8)(page_addr >> 16);
	cmd.addr[1] = (u8)(page_addr >> 8);
	cmd.addr[2] = (u8)page_addr;

	return spi_nand_send_cmd(spi, &cmd);
}

/**
 * spi_nand_wait - [DEFAULT] wait until the command is done
 * @chip: SPI-NAND device structure
 * @s: buffer to store status register(can be NULL)
 *
 * Wait for command done. This applies to erase and program only. Erase can
 * take up to 400ms and program up to 20ms.
 */
static int spi_nand_wait(struct spi_nand_chip *chip, u8 *s)
{
	unsigned long timeo = jiffies;
	u8 status, state = chip->state;
	int ret = -ETIMEDOUT;
	fh_dev_debug(&chip->spi->dev, "Enter %s\n", __func__);

	if (state == FL_ERASING)
		timeo += msecs_to_jiffies(400);
	else
		timeo += msecs_to_jiffies(1000); // 20 -> 40 for mx2g

	while (time_before(jiffies, timeo)) {
		spi_nand_read_status(chip->spi, &status);
		if ((status & STATUS_OIP_MASK) == STATUS_READY) {
			ret = 0;
			goto out;
		}
		cond_resched();
	}
out:
	if (s)
		*s = status;

	return 0;
}


/*
 * spi_nand_reset- send RESET command "0xff" to the SPI-NAND.
 * @chip: SPI-NAND device structure
 */
static int spi_nand_reset(struct spi_nand_chip *chip)
{
	struct spi_nand_cmd cmd = {0};
	struct spi_device *spi = chip->spi;
	fh_dev_debug(&spi->dev, "Enter %s\n", __func__);

	cmd.cmd = SPINAND_CMD_RESET;

	if (spi_nand_send_cmd(spi, &cmd) < 0)
		pr_err("spi_nand reset failed!\n");

	/* elapse 1ms before issuing any other command */
	udelay(1000);

	return 0;
}


/**
 * spi_nand_lock_block- send write register 0x1f command to the lock/unlock device
 * @spi: spi device structure
 * @lock: value to set to block lock register
 *
 * Description:
 *    After power up, all the Nand blocks are locked.  This function allows
 *    one to unlock the blocks, and so it can be written or erased.
 */
static int spi_nand_lock_block(struct spi_device *spi, u8 lock)
{
	struct spi_nand_cmd cmd = {0};
	int ret;

	cmd.cmd = SPINAND_CMD_WRITE_REG;
	cmd.n_addr = 1;
	cmd.addr[0] = REG_BLOCK_LOCK;
	cmd.n_tx = 1;
	cmd.tx_buf = &lock;

	ret = spi_nand_send_cmd(spi, &cmd);
	if (ret < 0)
		dev_err(&spi->dev, "error %d lock block\n", ret);

	return ret;
}

static u16 onfi_crc16(u16 crc, u8 const *p, size_t len)
{
	int i;

	while (len--) {
		crc ^= *p++ << 8;
		for (i = 0; i < 8; i++)
			crc = (crc << 1) ^ ((crc & 0x8000) ? 0x8005 : 0);
	}

	return crc;
}

/* Sanitize ONFI strings so we can safely print them */
static void sanitize_string(uint8_t *s, size_t len)
{
	ssize_t i;

	/* Null terminate */
	s[len - 1] = 0;

	/* Remove non printable chars */
	for (i = 0; i < len - 1; i++) {
		if (s[i] < ' ' || s[i] > 127)
			s[i] = '?';
	}

	/* Remove trailing spaces */
	strim(s);
}

/*
 * Check if the SPI-NAND chip is ONFI compliant,
 * returns 1 if it is, 0 otherwise.
 */
static bool spi_nand_detect_onfi(struct spi_nand_chip *chip)
{
	struct spi_device *spi = chip->spi;
	struct spi_nand_onfi_params *p;
	char *buffer;
	bool ret = true;
	u8 otp;
	int i;

	/*FIXME buffer size*/
	buffer = kmalloc(256 * 3, GFP_KERNEL);
	otp = OTP_ENABLE;
	spi_nand_set_otp(spi, &otp);
	chip->load_page(chip, 0x01);
	chip->waitfunc(chip, NULL);
	spi_nand_read_from_cache(chip, 0x01, 0x00, 256 * 3, buffer);
	otp = OTP_ECC_ENABLE;
	spi_nand_set_otp(spi, &otp);

	p = (struct spi_nand_onfi_params *)buffer;
	for (i = 0; i < 3; i++, p++) {
		if (p->sig[0] != 'O' || p->sig[1] != 'N' ||
		    p->sig[2] != 'F' || p->sig[3] != 'I')
			continue;
		if (onfi_crc16(ONFI_CRC_BASE, (uint8_t *)p, 254) ==
		    le16_to_cpu(p->crc))
			break;
	}
	if (i == 3) {
		pr_err("Could not find valid ONFI parameter page; aborting\n");
		ret = false;
		goto out;
	}

	memcpy(&chip->onfi_params, p, sizeof(*p));

	p = &chip->onfi_params;

	sanitize_string(p->manufacturer, sizeof(p->manufacturer));
	sanitize_string(p->model, sizeof(p->model));

	chip->name = p->model;
	chip->size = le32_to_cpu(p->byte_per_page) *
		     le32_to_cpu(p->pages_per_block) *
		     le32_to_cpu(p->blocks_per_lun) * p->lun_count;
	chip->block_size = le32_to_cpu(p->byte_per_page) *
			   le32_to_cpu(p->pages_per_block);
	chip->page_size = le32_to_cpu(p->byte_per_page);
	chip->page_spare_size = le16_to_cpu(p->spare_bytes_per_page);
	chip->block_shift = ilog2(chip->block_size);
	chip->page_shift = ilog2(chip->page_size);
	chip->page_mask = chip->page_size - 1;
	chip->bits_per_cell = p->bits_per_cell;
	/*FIXME need to find a way to read options from ONFI table*/
	chip->options = SPINAND_NEED_PLANE_SELECT;
	if (p->ecc_bits != 0xff) {
		chip->ecc_strength_ds = p->ecc_bits;
		chip->ecc_step_ds = 512;
	}

out:
	kfree(buffer);
	return ret;
}

static void spi_nand_set_defaults(struct spi_nand_chip *chip)
{
	/*struct spi_device *spi = chip->spi;*/

	/*	if (spi->mode & SPI_RX_QUAD)
			chip->read_cache = spi_nand_read_from_cache_x4;
		else if (spi->mode & SPI_RX_DUAL)
			chip->read_cache = spi_nand_read_from_cache_x2;
		else*/
	chip->read_cache = spi_nand_read_from_cache;

	if (!chip->reset)
		chip->reset = spi_nand_reset;
	if (!chip->erase_block)
		chip->erase_block = spi_nand_erase_block;
	if (!chip->load_page)
		chip->load_page = spi_nand_read_page_to_cache;
	if (!chip->store_cache)
		chip->store_cache = spi_nand_program_data_to_cache;
	if (!chip->write_page)
		chip->write_page = spi_nand_program_execute;
	if (!chip->write_enable)
		chip->write_enable = spi_nand_write_enable;
	if (!chip->waitfunc)
		chip->waitfunc = spi_nand_wait;
	if (!chip->enable_ecc)
		chip->enable_ecc = spi_nand_enable_ecc;
	if (!chip->disable_ecc)
		chip->disable_ecc = spi_nand_disable_ecc;
	if (!chip->block_bad)
		chip->block_bad = spi_nand_block_checkbad;
	if (!chip->set_qe)
		chip->set_qe = spi_nand_set_qe;
}

static int spi_nand_check(struct spi_nand_chip *chip)
{
	if (!chip->reset)
		return -ENODEV;
	if (!chip->read_id)
		return -ENODEV;
	if (!chip->load_page)
		return -ENODEV;
	if (!chip->read_cache)
		return -ENODEV;
	if (!chip->store_cache)
		return -ENODEV;
	if (!chip->write_page)
		return -ENODEV;
	if (!chip->erase_block)
		return -ENODEV;
	if (!chip->waitfunc)
		return -ENODEV;
	if (!chip->write_enable)
		return -ENODEV;
	if (!chip->get_ecc_status)
		return -ENODEV;
	if (!chip->enable_ecc)
		return -ENODEV;
	if (!chip->disable_ecc)
		return -ENODEV;
	if (!chip->ecclayout)
		return -ENODEV;
	return 0;
}

/**
 * spi_nand_scan_ident - [SPI-NAND Interface] Scan for the SPI-NAND device
 * @mtd: MTD device structure
 *
 * This is the first phase of the normal spi_nand_scan() function. It reads the
 * flash ID and sets up MTD fields accordingly.
 *
 */
int spi_nand_scan_ident(struct mtd_info *mtd)
{
	int ret;
	u8 id[SPINAND_MAX_ID_LEN] = {0};
	struct spi_nand_chip *chip = mtd->priv;

	spi_nand_set_defaults(chip);
	chip->reset(chip);

	chip->read_id(chip, id);
	if (id[0] == 0 && id[1] == 0) {
		pr_err("SPINAND: read id error! 0x%02x, 0x%02x!\n",
		       id[0], id[1]);
		return -ENODEV;
	}

	pr_err("SPINAND: read id ! 0x%02x, 0x%02x 0x%02x, 0x%02x!\n",
		id[0], id[1], id[2], id[3]);
	if (spi_nand_scan_id_table(chip, id))
		goto ident_done;
	pr_info("SPI-NAND type mfr_id: %x, dev_id: %x is not in id table.\n",
		id[0], id[1]);


	if (chip->spi->dev_open_multi_wire_flag & QUAD_WIRE_SUPPORT)
	chip->set_qe(chip);
	 
	if (spi_nand_detect_onfi(chip))
		goto ident_done;

	return -ENODEV;

ident_done:
	pr_info("SPI-NAND: %s is found.\n", chip->name);

	/*chip->mfr_id = id[0];
	chip->dev_id = id[1];*/

	chip->buf = kzalloc(chip->page_size + chip->page_spare_size,
			    GFP_KERNEL);
	if (!chip->buf)
		return -ENOMEM;

	chip->oobbuf = chip->buf + chip->page_size;

	ret = spi_nand_lock_block(chip->spi, BL_ALL_UNLOCKED);
	ret = chip->enable_ecc(chip);

	return ret;
}
EXPORT_SYMBOL_GPL(spi_nand_scan_ident);

/**
 * spi_nand_scan_tail - [SPI-NAND Interface] Scan for the SPI-NAND device
 * @mtd: MTD device structure
 *
 * This is the second phase of the normal spi_nand_scan() function. It fills out
 * all the uninitialized function pointers with the defaults.
 */
int spi_nand_scan_tail(struct mtd_info *mtd)
{
	struct spi_nand_chip *chip = mtd->priv;
	int ret;

	ret = spi_nand_check(chip);
	if (ret)
		return ret;
	/* Initialize state */
	chip->state = FL_READY;
	/* Invalidate the pagebuffer reference */
	chip->pagebuf = -1;
	chip->bbt_options |= NAND_BBT_USE_FLASH;
	chip->badblockpos = NAND_LARGE_BADBLOCK_POS;

	init_waitqueue_head(&chip->wq);
	spin_lock_init(&chip->chip_lock);

	mtd->name = chip->name;
	mtd->size = chip->size;
	mtd->erasesize = chip->block_size;
	mtd->writesize = chip->page_size;
	mtd->writebufsize = mtd->writesize;
	mtd->oobsize = chip->page_spare_size;
	mtd->owner = THIS_MODULE;
	mtd->type = MTD_NANDFLASH;
	mtd->flags = MTD_CAP_NANDFLASH;
	/*xxx:porting down: if (!mtd->ecc_strength)
			mtd->ecc_strength = chip->ecc_strength_ds ?
						chip->ecc_strength_ds : 1;*/
	mtd->bitflip_threshold = 1;
	mtd->ecclayout = chip->ecclayout;
	mtd->oobsize = chip->page_spare_size;
	mtd->oobavail = chip->ecclayout->oobavail;
	/* remove _* */
	mtd->erase = spi_nand_erase;
	mtd->point = NULL;
	mtd->unpoint = NULL;
	mtd->read = spi_nand_read;
	mtd->write = spi_nand_write;
	mtd->read_oob = spi_nand_read_oob;
	mtd->write_oob = spi_nand_write_oob;
	mtd->sync = spi_nand_sync;
	mtd->lock = NULL;
	mtd->unlock = NULL;
	mtd->suspend = spi_nand_suspend;
	mtd->resume = spi_nand_resume;
	mtd->block_isbad = spi_nand_block_isbad;
	mtd->block_markbad = spi_nand_block_markbad;

#ifndef CONFIG_SPI_NAND_BBT
	/* Build bad block table */
	return spi_nand_default_bbt(mtd);
#else
	return 0;
#endif
}
EXPORT_SYMBOL_GPL(spi_nand_scan_tail);

/**
 * spi_nand_scan_ident_release - [SPI-NAND Interface] Free resources
 * applied by spi_nand_scan_ident
 * @mtd: MTD device structure
 */
int spi_nand_scan_ident_release(struct mtd_info *mtd)
{
	struct spi_nand_chip *chip = mtd->priv;

	kfree(chip->buf);

	return 0;
}
EXPORT_SYMBOL_GPL(spi_nand_scan_ident_release);

/**
 * spi_nand_scan_tail_release - [SPI-NAND Interface] Free resources
 * applied by spi_nand_scan_tail
 * @mtd: MTD device structure
 */
int spi_nand_scan_tail_release(struct mtd_info *mtd)
{
	return 0;
}
EXPORT_SYMBOL_GPL(spi_nand_scan_tail_release);

/**
 * spi_nand_release - [SPI-NAND Interface] Free resources held by the SPI-NAND
 * device
 * @mtd: MTD device structure
 */
int spi_nand_release(struct mtd_info *mtd)
{
	struct spi_nand_chip *chip = mtd->priv;

	mtd_device_unregister(mtd);
	kfree(chip->buf);

	return 0;
}
EXPORT_SYMBOL_GPL(spi_nand_release);

MODULE_DESCRIPTION("SPI NAND framework");
MODULE_AUTHOR("Peter Pan<peterpandong at micron.com>");
MODULE_LICENSE("GPL v2");

