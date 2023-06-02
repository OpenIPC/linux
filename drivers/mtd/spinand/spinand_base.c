/*
 *  drivers/mtd/spinand/spinand_base.c
 *
 *  Overview:
 *   This is the generic MTD driver for SPI NAND flash devices. It should be
 *   capable of working with almost all SPI NAND chips currently available.
 *
 *  Copyright (c) 2013 Gofortune Semiconductor Corporation.
 *   Kewell Liu  <liujingke@gofortune-semi.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/bitops.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>

#include "spinand.h"


/******************************************************/
/*               MTD Part  ( Hardware Independent )                        */
/******************************************************/

static int spinand_check_offs_len(struct mtd_info *mtd,
                    loff_t ofs, uint64_t len)
{
    struct spinand_chip *chip = mtd->priv;
    int ret = 0;

    /* Start address must align on block boundary */
    if (ofs & ((1 << chip->phys_erase_shift) - 1)) {
        pr_err("%s: unaligned address\n", __func__);
        ret = -EINVAL;
    }

    /* Length must align on block boundary */
    if (len & ((1 << chip->phys_erase_shift) - 1)) {
        pr_err("%s: length not block aligned\n", __func__);
        ret = -EINVAL;
    }

    return ret;

}

/**
 * nand_transfer_oob - [INTERN] Transfer oob to client buffer
 * @chip: nand chip structure
 * @oob: oob destination address
 * @ops: oob ops structure
 * @len: size of oob to transfer
 */
static int spinand_transfer_oob(struct spinand_chip *chip, uint8_t *oob,
                  struct mtd_oob_ops *ops, size_t len)
{
    struct spinand_info *info = chip->info;

    switch (ops->mode) {

    case MTD_OPS_PLACE_OOB:
    case MTD_OPS_RAW:
        memcpy(oob, (chip->oob_poi + ops->ooboffs), len);
        ops->ooboffs = 0;  //for second read
        return 0;

    case MTD_OPS_AUTO_OOB: {
        struct nand_oobfree *free = info->ecclayout->oobfree;
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
            memcpy(oob, (chip->oob_poi + boffs), bytes);
            oob += bytes;
        }
        return 0;
    }
    default:
        BUG();
    }

    return -EINVAL;

}

/**
 * spinand_fill_oob - [INTERN] Transfer client buffer to oob
 * @mtd: MTD device structure
 * @oob: oob data buffer
 * @len: oob data write length
 * @ops: oob ops structure
 */
static int spinand_fill_oob(struct mtd_info *mtd, uint8_t *oob, size_t len,
                  struct mtd_oob_ops *ops)
{
    struct spinand_chip *chip = mtd->priv;
    struct spinand_info *info = chip->info;

    /*
     * Initialise to all 0xFF, to avoid the possibility of left over OOB
     * data from a previous OOB read.
     */
    memset(chip->oob_poi, 0xff, mtd->oobsize);

    switch (ops->mode) {

    case MTD_OPS_PLACE_OOB:
    case MTD_OPS_RAW:
        memcpy(chip->oob_poi + ops->ooboffs, oob, len);
        ops->ooboffs = 0;   //for second write
        return 0;

    case MTD_OPS_AUTO_OOB: {
        struct nand_oobfree *free = info->ecclayout->oobfree;
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
            memcpy(chip->oob_poi + boffs, oob, bytes);
            oob += bytes;
        }
        return 0;
    }
    default:
        BUG();
    }

    return -EINVAL;

}

/**
 * spinand_check_wp - [GENERIC] check if the chip is write protected
 * @mtd: MTD device structure
 *
 * Check, if the device is write protected. The function expects, that the
 * device is already selected.
 */
static int spinand_check_wp(struct mtd_info *mtd)
{
    //struct spinand_chip *chip = mtd->priv;

    /* Broken xD cards report WP despite being writable */

    /* Check the WP bit */
    //chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);
    //return (chip->read_byte(mtd) & NAND_STATUS_WP) ? 0 : 1;
    return 0;
}

/**
 * spinand_block_checkbad - [GENERIC] Check if a block is marked bad
 * @mtd: MTD device structure
 * @ofs: offset from device start
 * @allowbbt: 1, if its allowed to access the bbt area
 *
 * Check, if the block is bad. Either by reading the bad block table or
 * calling of the scan function.
 */
static int spinand_block_checkbad(struct mtd_info *mtd, loff_t ofs, int allowbbt)
{
    struct spinand_chip *chip = mtd->priv;

    if (!chip->bbt)
        return chip->block_bad(mtd, ofs);

    /* Return info from the table */
    return spinand_isbad_bbt(mtd, ofs, allowbbt);
}

/**
 * spinand_do_read_oob - [INTERN] NAND read out-of-band
 * @mtd: MTD device structure
 * @from: offset to read from
 * @ops: oob operations description structure
 *
 * NAND read out-of-band data from the spare area.
 */
static int spinand_do_read_oob(struct mtd_info *mtd, loff_t from,
                struct mtd_oob_ops *ops)
{
    struct spinand_chip *chip = mtd->priv;
    struct spinand_info *info = chip->info;
    uint32_t page_id, oob_num;

    uint32_t count;
    uint32_t oob_ok, oob_left;

    int readlen = ops->ooblen;
    int ooblen;

    signed int retval;

    #ifdef CONFIG_DEBUG_R
    pr_info("%s: from = 0x%08Lx, len = %i\n",
            __func__, (unsigned long long)from, readlen);
    #endif

    if (ops->mode == MTD_OPS_AUTO_OOB)
    {
        ooblen = mtd->oobavail;
    }
    else
    {
        ooblen = mtd->oobsize;
    }

    if (unlikely(ops->ooboffs >= ooblen)) {
        dev_err(&mtd->dev, "%s: attempt to start read outside oob\n", __func__);
        return -EINVAL;
    }

    /* Do not allow reads past end of device */
    if (unlikely(from >= mtd->size ||
             ops->ooboffs + readlen > ((mtd->size >> info->page_shift) -
                    (from >> info->page_shift)) * ooblen)) {
        dev_err(&mtd->dev, "%s: attempt to read beyond end of device\n", __func__);
        return -EINVAL;
    }

    /* Shift to get page */
    page_id = (uint32_t)(from >> info->page_shift);

    /* for oob */
    oob_num = (ops->ooblen + ooblen -1) / ooblen;

    count = 0;

    oob_left = ops->ooblen;
    oob_ok = 0;

#ifdef CONFIG_DEBUG_R
    pr_info("page_id      0x%08x\n"
            "oob_num      0x%08x\n"
            "ooblen       0x%08x\n"
            "oob_left     0x%08x\n",
            page_id,
            oob_num,
            ooblen,
            oob_left);
#endif

    while (1)
    {
        if (count < oob_num)
        {
            memset(chip->oob_poi, 0, info->page_spare_size);
            if(ops->mode == MTD_OPS_RAW)
            {
                retval = chip->read_page_raw(chip, (page_id + count), info->page_main_size,
                            info->page_spare_size, chip->oob_poi);
            }
            else
            {
                retval = chip->read_page(chip, (page_id + count), info->page_main_size,
                            info->page_spare_size, chip->oob_poi);
            }
            if (retval != 0)
            {
                dev_err(&mtd->dev, "%s: fail, block = %d, PageId = 0x%08x\n", __FUNCTION__,
                        ((page_id + count)/info->page_per_block), (page_id + count));
                return -EBADMSG;
            }

            #ifdef CONFIG_DEBUG_R
            {
                int i = 0;

                for(i = 0; i < mtd->oobsize; i++)
                {
                    if(i % 16 == 0)
                    {
                        printk("\n");
                    }
                    printk("%02x ", chip->oob_poi[i]);
                }

                printk("\n");
            }
            #endif
        }
        else
        {
            break;
        }

        if (count < oob_num && ops->oobbuf && chip->oob_poi)
        {
            int size;

            /* copy oobbuf to ops oobbuf */
            if (oob_left < ooblen)
            {
                size = oob_left;
            }
            else
            {
                size = ooblen;
            }

            retval = spinand_transfer_oob(chip, (ops->oobbuf + oob_ok), ops, size);
            if (retval != 0)
            {
                dev_err(&mtd->dev, "[%s]: fail oob mode = %d!\n", __FUNCTION__, ops->mode);
                return -EUCLEAN;
            }

            oob_ok += size;
            oob_left -= size;

            ops->oobretlen = oob_ok;

        }

        count++;

    }

    return 0;

}


static int spinand_do_read_ops(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops)
{
    struct spinand_chip *chip = mtd->priv;
    struct spinand_info *info = chip->info;
    uint32_t page_id, page_offset, page_num, oob_num = 0;

    uint32_t count;
    uint32_t main_ok, main_left, main_offset;
    uint32_t oob_ok, oob_left;

    signed int retval;
    int ooblen;

    if (!chip->buffers)
        return -ENOMEM;

    if (ops->mode == MTD_OPS_AUTO_OOB)
    {
        ooblen = mtd->oobavail;
    }
    else
    {
        ooblen = mtd->oobsize;
    }

    page_id = (uint32_t)(from >> info->page_shift);

    /* for main data */
    page_offset = (uint32_t)(from & info->page_mask);
    page_num = (page_offset + ops->len + info->page_main_size -1 ) / info->page_main_size;

    /* for oob */
    if(ooblen)
        oob_num = (ops->ooblen + ooblen - 1) / ooblen;

    count = 0;

    main_left = ops->len;
    main_ok = 0;
    main_offset = page_offset;

    oob_left = ops->ooblen;
    oob_ok = 0;

    #ifdef CONFIG_DEBUG_R
    pr_info("[DEBUG_R]:%s\n",__FUNCTION__);
    pr_info("page_id      0x%08x\n"
            "page_offset  0x%08x\n"
            "page_num     0x%08x\n"
            "oob_num      0x%08x\n"
            "main_left    0x%08x\n"
            "main_offset  0x%08x\n"
            "oob_left     0x%08x\n",
            page_id,
            page_offset,
            page_num,
            oob_num,
            main_left,
            main_offset,
            oob_left);
    #endif

    while (1)
    {
        if (count < page_num || count < oob_num)
        {
            memset(chip->pagebuf, 0, info->page_size);
            retval = chip->read_page(chip, (page_id + count), 0, info->page_size, chip->pagebuf);
            if (retval != 0)
            {
                dev_err(&mtd->dev, "%s: fail, block = %d, PageId = 0x%08x\n", __FUNCTION__,
                        ((page_id + count)/info->page_per_block), (page_id + count));
                return -EBADMSG;
            }
        }
        else
        {
            break;
        }

        if (count < page_num && ops->datbuf)
        {
            int size;

            if ((main_offset + main_left) < info->page_main_size)
            {
                size = main_left;
            }
            else
            {
                size = info->page_main_size - main_offset;
            }

            memcpy((ops->datbuf + main_ok), (chip->pagebuf + main_offset), size);

            main_ok += size;
            main_left -= size;
            main_offset = 0;

            ops->retlen = main_ok;
        }

        if (count < oob_num && ops->oobbuf && chip->oob_poi)
        {
            int size;

            /* copy oobbuf to ops oobbuf */
            if (oob_left < ooblen)
            {
                size = oob_left;
            }
            else
            {
                size = ooblen;
            }

            retval = spinand_transfer_oob(chip, (ops->oobbuf + oob_ok), ops, size);
            if (retval != 0)
            {
                dev_err(&mtd->dev, "[%s]: fail oob mode = %d!\n", __FUNCTION__, ops->mode);
                return -EUCLEAN;
            }

            oob_ok += size;
            oob_left -= size;

            ops->oobretlen = oob_ok;

        }

        count++;

    }

    return 0;

}

/**
 * spinand_do_write_oob - [MTD Interface] NAND write out-of-band
 * @mtd: MTD device structure
 * @to: offset to write to
 * @ops: oob operation description structure
 *
 * NAND write out-of-band.
 */
static int spinand_do_write_oob(struct mtd_info *mtd, loff_t to,
                 struct mtd_oob_ops *ops)
{
    struct spinand_chip *chip = mtd->priv;
    struct spinand_info *info = chip->info;
    uint32_t page_id, oob_num;

    uint32_t count;
    uint32_t oob_ok, oob_left;

    uint32_t    ooblen;

    signed int retval;

    #ifdef CONFIG_DEBUG_W
    pr_info("%s: to = 0x%08x, len = %i\n",
            __func__, (unsigned int)to, (int)ops->ooblen);
    #endif

    if (ops->mode == MTD_OPS_AUTO_OOB)
        ooblen = mtd->oobavail;
    else
        ooblen = mtd->oobsize;

    /* Do not allow write past end of page */
    if ((ops->ooboffs + ops->ooblen) > ooblen) {
        dev_err(&mtd->dev, "%s: attempt to write past end of page\n", __func__);
        return -EINVAL;
    }

    if (unlikely(ops->ooboffs >= ooblen)) {
        dev_err(&mtd->dev, "%s: attempt to start write outside oob\n", __func__);
        return -EINVAL;
    }

    /* Do not allow write past end of device */
    if (unlikely(to >= mtd->size ||
             ops->ooboffs + ops->ooblen >
            ((mtd->size >> info->page_shift) -
             (to >> info->page_shift)) * ooblen)) {
        dev_err(&mtd->dev, "%s: attempt to write beyond end of device\n", __func__);
        return -EINVAL;
    }

    /* Shift to get page */
    page_id = (uint32_t)(to >> info->page_shift);

    /* Check, if it is write protected */
    if (spinand_check_wp(mtd))
        return -EROFS;

    /* Invalidate the page cache, if we write to the cached page */
    if (page_id == chip->page_id)
        chip->page_id = -1;

    /* for oob */
    oob_num = (ops->ooblen + ooblen - 1) / ooblen;

    count = 0;

    oob_left = ops->ooblen;
    oob_ok = 0;

    memset(chip->pagebuf, 0xff, mtd->writesize);

#ifdef CONFIG_DEBUG_W
    pr_info("page_id      0x%08x\n"
            "oob_num      0x%08x\n"
            "ooblen       0x%08x\n"
            "oob_left     0x%08x\n",
            page_id,
            oob_num,
            ooblen,
            oob_left);
#endif
    while (1)
    {
        uint32_t size;

        if(count >= oob_num)
        {
            break;
        }

        if (oob_left < ooblen)
        {
            size = oob_left;
        }
        else
        {
            size = ooblen;
        }

        if (ops->oobbuf && chip->oob_poi)
        {
            retval = spinand_fill_oob(mtd, (ops->oobbuf + oob_ok), size, ops);
            if (retval != 0)
            {
                dev_err(&mtd->dev, "[%s]: fail oob mode = %d!\n", __FUNCTION__, ops->mode);
                return -EPERM;
            }
            oob_ok += size;
            ops->oobretlen = oob_ok;

            #ifdef CONFIG_DEBUG_W
            {
                int i = 0, j = 0;

                for(i = 0,j = 0; i < mtd->oobsize; i++,j++)
                {
                    if(j % 16 == 0)
                    {
                        printk("\n");
                    }
                    printk("%02x ", chip->oob_poi[i]);
                }

                printk("\n");
            }
            #endif
        }

        if (ops->mode == MTD_OPS_RAW)
        {
            retval = chip->program_page_raw(chip, (page_id + count),
                         info->page_main_size, info->page_spare_size, chip->pagebuf);
        }
        else
        {
            retval = chip->program_page(chip, (page_id + count),
                         info->page_main_size, info->page_spare_size, chip->pagebuf);
        }

        if (retval != 0)
        {
            dev_err(&mtd->dev, "[%s]: fail, block = %d, PageId = 0x%08x\n", __FUNCTION__,
                        ((page_id + count)/info->page_per_block), (page_id + count));
            return -EPERM;
        }

        count++;

    }

    return 0;

}

static int spinand_do_write_ops(struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops)
{
    struct spinand_chip *chip = mtd->priv;
    struct spinand_info *info = chip->info;

    uint32_t page_id, page_offset, page_num, oob_num = 0;

    int count;

    int main_ok, main_left, main_offset;
    int oob_ok, oob_left;

    uint32_t    ooblen;

    signed int retval;

#ifdef CONFIG_DEBUG_W
    pr_info("%s: \n"
            "to = 0x%08x,\n"
            "len = %i,\n"
            "ooblen = %i\n", __func__, (unsigned int)to, (int)ops->len,(int)ops->ooblen);
#endif

    if (!chip->buffers)
        return -ENOMEM;

    if (ops->mode == MTD_OPS_AUTO_OOB)
        ooblen = mtd->oobavail;
    else
        ooblen = mtd->oobsize;

    page_id = (uint32_t)(to >> info->page_shift);

    /* Check, if it is write protected */
    if (spinand_check_wp(mtd))
        return -EROFS;

    /* for main data */
    page_offset = to & info->page_mask;
    page_num = (page_offset + ops->len + info->page_main_size - 1 ) / info->page_main_size;

    /* for oob */
    if(ooblen)
        oob_num = (ops->ooblen + ooblen - 1) / ooblen;

    count = 0;

    main_left = ops->len;
    main_ok = 0;
    main_offset = page_offset;

    oob_left = ops->ooblen;
    oob_ok = 0;

#ifdef CONFIG_DEBUG_W

    pr_info("page_id      0x%08x\n"
            "page_offset  0x%08x\n"
            "page_num     0x%08x\n"
            "oob_num      0x%08x\n"
            "main_left    0x%08x\n"
            "main_offset  0x%08x\n"
            "oob_left     0x%08x\n",
            page_id,
            page_offset,
            page_num,
            oob_num,
            main_left,
            main_offset,
            oob_left);
#endif

    while (1)
    {
        if (count < page_num || count < oob_num)
        {
            memset(chip->pagebuf, 0xFF, info->page_size);
        }
        else
        {
            break;
        }

        if (count < page_num && ops->datbuf)
        {
            int size;

            if ((main_offset + main_left) < info->page_main_size)
            {
                size = main_left;
            }
            else
            {
                size = info->page_main_size - main_offset;
            }

            memcpy ((chip->pagebuf + main_offset), (ops->datbuf + main_ok), size);

            main_ok += size;
            main_left -= size;
        }

        if (count < oob_num && ops->oobbuf && chip->oob_poi)
        {
            int size;

            if (oob_left < ooblen)
            {
                size = oob_left;
            }
            else
            {
                size = ooblen;
            }

            retval = spinand_fill_oob(mtd, (ops->oobbuf + oob_ok), size, ops);
            if (retval != 0)
            {
                dev_err(&mtd->dev, "[%s]: fail oob mode = %d!\n", __FUNCTION__, ops->mode);
                return -EPERM;
            }

            oob_ok += size;
            oob_left -= size;

        }

        if (count < page_num || count < oob_num)
        {
            #ifdef CONFIG_DEBUG_W
            {
                int i = 0;

                for(i = 0; i < 64; i++)
                {
                    if(i % 16 == 0)
                    {
                        printk("\n");
                    }
                    printk("%02x ", chip->pagebuf[i]);
                }

                printk("\n");
            }
            #endif

            if(oob_num > 0)
            {
                retval = chip->program_page(chip, (page_id + count), main_offset,
                            info->page_size, chip->pagebuf);
            }
            else
            {
                retval = chip->program_page(chip, (page_id + count), main_offset,
                            info->page_main_size, chip->pagebuf);
            }

            if (retval != 0)
            {
                dev_err(&mtd->dev, "[%s]: fail, block = %d, PageId = 0x%08x\n", __FUNCTION__,
                        ((page_id + count)/info->page_per_block), (page_id + count));
                return -EPERM;
            }

            main_offset = 0;

        }

        if (count < page_num && ops->datbuf)
        {
            ops->retlen = main_ok;
        }

        if (count < oob_num && ops->oobbuf && chip->oob_poi)
        {
            ops->oobretlen = oob_ok;
        }

        count++;

    }

    return 0;

}

static int spinand_read(struct mtd_info *mtd, loff_t from, size_t len,
    size_t *retlen, u_char *buf)
{
    struct spinand_chip *chip = mtd->priv;
    struct mtd_oob_ops ops = {0};
    int ret;

    /* Do not allow reads past end of device */
    if ((from + len) > mtd->size)
        return -EINVAL;

    if (!len)
        return 0;

    mutex_lock(&chip->lock);

    ops.mode    = 0;
    ops.len     = len;
    ops.datbuf  = buf;
    ops.ooblen  = 0;
    ops.oobbuf  = NULL;

    #ifdef CONFIG_DEBUG_R
    pr_info("[DEBUG_R]:%s\n",__FUNCTION__);
    pr_info("from         0x%llx\n"
            "len          0x%x\n",
             from,
             len);
    #endif

    ret = spinand_do_read_ops(mtd, from, &ops);

    *retlen = ops.retlen;

    mutex_unlock(&chip->lock);

    return ret;

}

static int spinand_write(struct mtd_info *mtd, loff_t to, size_t len,
    size_t *retlen, const u_char *buf)
{
    struct spinand_chip *chip = mtd->priv;
    struct mtd_oob_ops ops = {0};
    int ret;

    /* Do not allow reads past end of device */
    if ((to + len) > mtd->size)
        return -EINVAL;
    if (!len)
        return 0;

    mutex_lock(&chip->lock);

    ops.len     = len;
    ops.datbuf  = (uint8_t *)buf;
    ops.ooblen  = 0;
    ops.oobbuf  = NULL;
    ops.mode    = 0;

#ifdef CONFIG_DEBUG_W
    pr_info("[DEBUG_W]:%s\n",__FUNCTION__);
    pr_info("to           0x%llx\n"
            "len          0x%x\n",
             to,
             len);
#endif

    ret = spinand_do_write_ops(mtd, to, &ops);

    *retlen = ops.retlen;

    mutex_unlock(&chip->lock);

    return ret;

}

static int spinand_read_oob(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops)
{
    struct spinand_chip *chip = mtd->priv;
    int ret = -ENOTSUPP;

    ops->retlen = 0;

    /* Do not allow reads past end of device */
    if (ops->datbuf && (from + ops->len) > mtd->size) {
        dev_err(&mtd->dev, "%s: attempt to read beyond end of device\n", __func__);
        return -EINVAL;
    }

    switch (ops->mode) {
    case MTD_OPS_PLACE_OOB:
    case MTD_OPS_AUTO_OOB:
    case MTD_OPS_RAW:
        break;

    default:
        dev_err(&mtd->dev, "%s: The mode = %d no supported\n", __func__, ops->mode);
        goto out;
    }

    mutex_lock(&chip->lock);

    if (!ops->datbuf)
        ret = spinand_do_read_oob(mtd, from, ops);
    else
        ret = spinand_do_read_ops(mtd, from, ops);

    mutex_unlock(&chip->lock);
out:

    return ret;

}

/**
 * spinand_write_oob - [MTD Interface] NAND write data and/or out-of-band
 * @mtd: MTD device structure
 * @to: offset to write to
 * @ops: oob operation description structure
 */
static int spinand_write_oob(struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops)
{
    struct spinand_chip *chip = mtd->priv;
    int ret = -ENOTSUPP;

    ops->retlen = 0;

    /* Do not allow writes past end of device */
    if (ops->datbuf && (to + ops->len) > mtd->size) {
        pr_debug("%s: attempt to write beyond end of device\n", __func__);
        return -EINVAL;
    }

#ifdef CONFIG_DEBUG_W
    pr_info("[DEBUG_W]:%s\n",__FUNCTION__);
    pr_info("to           0x%llx\n"
            "len          0x%x\n"
            "ooblen       0x%x\n",
             to,
             ops->len, ops->ooblen);
#endif


    switch (ops->mode) {
    case MTD_OPS_PLACE_OOB:
    case MTD_OPS_AUTO_OOB:
    case MTD_OPS_RAW:
        break;

    default:
        dev_err(&mtd->dev, "%s: The mode = %d no supported\n", __func__, ops->mode);
        goto out;
    }

    mutex_lock(&chip->lock);

    if (!ops->datbuf)
        ret = spinand_do_write_oob(mtd, to, ops);
    else
        ret = spinand_do_write_ops(mtd, to, ops);

    mutex_unlock(&chip->lock);

out:

    return ret;

}

#define BBT_PAGE_MASK   0xffffff3f
/**
 * spinand_erase_nand - [INTERN] erase block(s)
 * @mtd: MTD device structure
 * @instr: erase instruction
 * @allowbbt: allow erasing the bbt area
 *
 * Erase one ore more blocks.
 */
int spinand_erase_nand(struct mtd_info *mtd, struct erase_info *instr, int allowbbt)
{
    int page, pages_per_block, ret;
    struct spinand_chip *chip = mtd->priv;
    struct spinand_info *info = chip->info;

    loff_t rewrite_bbt[NAND_MAX_CHIPS] = {0};
    unsigned int bbt_masked_page = 0xffffffff;
    loff_t len;

    pr_debug("%s: start = 0x%012llx, len = %llu\n",
            __func__, (unsigned long long)instr->addr,
            (unsigned long long)instr->len);

    if (spinand_check_offs_len(mtd, instr->addr, instr->len))
        return -EINVAL;

    /* Do not allow erase past end of device */
    if ((instr->len + instr->addr) > info->usable_size) {
        pr_err("%s: Erase past end of device\n",__FUNCTION__);
        return -EINVAL;
    }

    mutex_lock(&chip->lock);

    /* Grab the lock and see if the device is available */
    //nand_get_device(chip, mtd, FL_ERASING);

    /* Shift to get first page */
    page = (int)(instr->addr >> chip->page_shift);
    page &= chip->pagemask;

    /* Calculate pages in each block */
    pages_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);

    /* Check, if it is write protected */
    if (spinand_check_wp(mtd)) {
        pr_debug("%s: device is write protected!\n", __func__);
        instr->state = MTD_ERASE_FAILED;
        goto erase_exit;
    }

    /*
        * If BBT requires refresh, set the BBT page mask to see if the BBT
        * should be rewritten. Otherwise the mask is set to 0xffffffff which
        * can not be matched. This is also done when the bbt is actually
        * erased to avoid recursive updates.
        */
    if (chip->options & BBT_AUTO_REFRESH && !allowbbt)
        bbt_masked_page = chip->bbt_td->pages[0] & BBT_PAGE_MASK;

    /* Loop through the pages */
    len = instr->len;

    instr->state = MTD_ERASING;

    while (len)
    {
        /* Check if we have a bad block, we do not erase bad blocks! */
        if (spinand_block_checkbad(mtd, ((loff_t) page) << chip->page_shift, allowbbt))
        {
            pr_warn("%s: attempt to erase a bad block at page 0x%08x\n", __func__, page);
            instr->state = MTD_ERASE_FAILED;
            goto erase_exit;
        }

        ret = chip->erase_block(chip, (uint32_t)page);

        /* See if block erase succeeded */
        if (ret != 0)
        {
            ret = chip->erase_block(chip, (uint32_t)page);
            if (ret != 0)
            {
                pr_err("%s: failed erase, block = %d!\n", __FUNCTION__,
                        (uint32_t)(page/pages_per_block));
                instr->state = MTD_ERASE_FAILED;
                instr->fail_addr = ((loff_t)page << chip->page_shift);
                goto erase_exit;
            }
        }

        /*
              * If BBT requires refresh, set the BBT rewrite flag to the
              * page being erased.
              */
        if (bbt_masked_page != 0xffffffff && (page & BBT_PAGE_MASK) == bbt_masked_page)
                rewrite_bbt[0] = ((loff_t)page << chip->page_shift);

        /* Increment page address and decrement length */
        len -= (1 << chip->phys_erase_shift);
        page += pages_per_block;

    }
    instr->state = MTD_ERASE_DONE;

erase_exit:

    mutex_unlock(&chip->lock);

    ret = (instr->state == MTD_ERASE_DONE ? 0 : -EIO);

    /* Do call back function */
    if (!ret)
        mtd_erase_callback(instr);

    /*
        * If BBT requires refresh and erase was successful, rewrite any
        * selected bad block tables.
        */
    if (bbt_masked_page == 0xffffffff || ret)
        return ret;

    if (rewrite_bbt[0])
    {
        /* Update the BBT for chip */
        pr_debug("%s: update_bbt (0x%0llx 0x%08x)\n",
                __func__, rewrite_bbt[0], chip->bbt_td->pages[0]);
        spinand_update_bbt(mtd, rewrite_bbt[0]);
    }

    /* Return more or less happy */
    return ret;

}

/**
 * spinand_erase - [MTD Interface] erase block(s)
 * @mtd: MTD device structure
 * @instr: erase instruction
 *
 * Erase one ore more blocks.
 */
static int spinand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
    return spinand_erase_nand(mtd, instr, 0);
}


/**
 * spinand_sync - [MTD Interface] sync
 * @param mtd       MTD device structure
 *
 * Sync is actually a wait for chip ready function
 */
static void spinand_sync(struct mtd_info *mtd)
{
    dev_err(&mtd->dev, "spinand_sync: called\n");
}

/**
 * spinand_block_isbad - [MTD Interface] Check if block at offset is bad
 * @mtd: MTD device structure
 * @offs: offset relative to mtd start
 */
static int spinand_block_isbad(struct mtd_info *mtd, loff_t offs)
{
    return spinand_block_checkbad(mtd, offs, 0);
}

/**
 * spinand_block_bad - [DEFAULT] Read bad block marker from the chip
 * @mtd: MTD device structure
 * @ofs: offset from device start
 *
 * Check, if the block is bad.
 */
static int spinand_block_bad(struct mtd_info *mtd, loff_t ofs)
{
    struct spinand_chip *chip = mtd->priv;
    struct spinand_info *info = chip->info;
    struct nand_bbt_descr *badblock_ptn = info->badblock_pattern;
    uint32_t    page_id;
    uint16_t    is_bad = 0;
    int    i = 0, res = 0;

    if (chip->bbt_options & NAND_BBT_SCANLASTPAGE)
        ofs += mtd->erasesize - mtd->writesize;

    page_id = (uint32_t)(ofs >> chip->page_shift) & chip->pagemask;

    #ifdef KE_DEBUG_R
        pr_debug("[%s]: page_id = 0x%08x, Ofs = 0x%08x\n",
                __FUNCTION__, page_id, ofs);
    #endif

    do
    {
        if (chip->options & NAND_BUSWIDTH_16)
        {
            chip->read_page(chip, page_id,
                    (info->page_main_size + chip->badblockpos), badblock_ptn->len, (uint8_t*)&is_bad);
            is_bad = cpu_to_le16(is_bad);
            if (chip->badblockpos & 0x1)
                is_bad >>= 8;
            else
                is_bad &= 0xFF;
        }
        else
        {
            chip->read_page(chip, page_id,
                    (info->page_main_size + chip->badblockpos), badblock_ptn->len, (uint8_t*)&is_bad);
        }

        if (likely(chip->badblockbits == 8))
        {
            res = is_bad != 0xFF;
        }
        else
        {
            res = hweight8(is_bad) < chip->badblockbits;
        }
        ofs += mtd->writesize;
        page_id = (uint32_t)(ofs >> chip->page_shift) & chip->pagemask;
        i++;
    } while (!res && i < 2 && (chip->bbt_options & NAND_BBT_SCAN2NDPAGE));

    #ifdef KE_DEBUG_R
    pr_debug("[%s]: Block = 0x%04x, is %s\n",
                __FUNCTION__, (uint32_t)(ofs >> chip->bbt_erase_shift), (res > 0)? "bad":"good");
    #endif

    return res;

}

#if 0

/**
 * spinand_default_block_bad - [DEFAULT] Read bad block marker from the chip
 * @mtd: MTD device structure
 * @ofs: offset from device start
 *
 * Check, if the block is bad.
 */
static int spinand_default_block_bad(struct mtd_info *mtd, loff_t ofs)
{
    struct spinand_chip *chip = mtd->priv;
    struct spinand_info *info = chip->info;
    uint32_t    page_id;
    uint8_t     is_bad = 0;
    int    ret = 0;

    if (chip->bbt_options & NAND_BBT_SCANLASTPAGE)
        ofs += mtd->erasesize - mtd->writesize;

    page_id = (uint32_t)(ofs >> chip->page_shift);

    mutex_lock(&chip->lock);

    chip->read_page(chip, page_id,
                    (info->page_main_size + chip->badblockpos), 1, &is_bad);

    mutex_unlock(&chip->lock);

    if (is_bad != 0xff)
    {
        ret = 1;
    }

    return ret;

}

#endif

/**
 * spinand_default_block_markbad - [DEFAULT] mark a block bad
 * @mtd: MTD device structure
 * @ofs: offset from device start
 *
 * This is the default implementation, which can be overridden by a hardware
 * specific driver. We try operations in the following order, according to our
 * bbt_options (NAND_BBT_NO_OOB_BBM and NAND_BBT_USE_FLASH):
 *  (1) erase the affected block, to allow OOB marker to be written cleanly
 *  (2) update in-memory BBT
 *  (3) write bad block marker to OOB area of affected block
 *  (4) update flash-based BBT
 * Note that we retain the first error encountered in (3) or (4), finish the
 * procedures, and dump the error in the end.
*/
static int spinand_default_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
    struct spinand_chip *chip = mtd->priv;
    uint8_t buf[2] = { 0, 0 };
    int block, res, ret = 0, i = 0;
    int write_oob = !(chip->bbt_options & NAND_BBT_NO_OOB_BBM);

    if (write_oob) {
        struct erase_info einfo;

        /* Attempt erase before marking OOB */
        memset(&einfo, 0, sizeof(einfo));
        einfo.mtd = mtd;
        einfo.addr = ofs;
        einfo.len = 1 << chip->phys_erase_shift;
        spinand_erase_nand(mtd, &einfo, 0);
    }

    /* Get block number */
    block = (int)(ofs >> chip->bbt_erase_shift);
    /* Mark block bad in memory-based BBT */
    if (chip->bbt)
        chip->bbt[block >> 2] |= 0x01 << ((block & 0x03) << 1);

    /* Write bad block marker to OOB */
    if (write_oob)
    {
        struct mtd_oob_ops ops;
        loff_t wr_ofs = ofs;

        //nand_get_device(chip, mtd, FL_WRITING);

        ops.datbuf = NULL;
        ops.oobbuf = buf;
        ops.ooboffs = chip->badblockpos;
        if (chip->options & NAND_BUSWIDTH_16) {
            ops.ooboffs &= ~0x01;
            ops.len = ops.ooblen = 2;
        } else {
            ops.len = ops.ooblen = 1;
        }
        ops.mode = MTD_OPS_PLACE_OOB;

        /* Write to first/last page(s) if necessary */
        if (chip->bbt_options & NAND_BBT_SCANLASTPAGE)
            wr_ofs += mtd->erasesize - mtd->writesize;
        do {
            res = spinand_do_write_oob(mtd, wr_ofs, &ops);
            if (!ret)
                ret = res;

            i++;
            wr_ofs += mtd->writesize;
        } while ((chip->bbt_options & NAND_BBT_SCAN2NDPAGE) && i < 2);

        //nand_release_device(mtd);
    }

    /* Update flash-based bad block table */
    if (chip->bbt_options & NAND_BBT_USE_FLASH) {
        res = spinand_update_bbt(mtd, ofs);
        if (!ret)
            ret = res;
    }

    if (!ret)
        mtd->ecc_stats.badblocks++;

    return ret;

}

/**
 * spinand_block_markbad - [MTD Interface] Mark bad block
 * @param mtd       MTD device structure
 * @param ofs       Bad block number
 */
static int spinand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
    struct spinand_chip *chip = mtd->priv;
    uint32_t ret = 0;

    ret = spinand_block_isbad(mtd, ofs);
    if (ret)
    {
        /* If it was bad already, return success and do nothing */
        if (ret > 0)
            return 0;
        return ret;
    }

    return chip->block_markbad(mtd, ofs);
}


/**
 * spinand_suspend - [MTD Interface] Suspend the spinand flash
 * @param mtd       MTD device structure
 */
static int spinand_suspend(struct mtd_info *mtd)
{
    return 0;
}

/**
 * spinand_resume - [MTD Interface] Resume the spinand flash
 * @param mtd       MTD device structure
 */
static void spinand_resume(struct mtd_info *mtd)
{
    return;
}

/* Set default functions */
static void spinand_set_defaults(struct spinand_chip *chip)
{
    /* check for proper chip_delay setup, set 20us if not */
    if (!chip->chip_delay)
        chip->chip_delay = 20;

    if (!chip->scan_bbt)
        chip->scan_bbt = spinand_default_bbt;

    if (!chip->block_markbad)
        chip->block_markbad = spinand_default_block_markbad;

    if (!chip->block_bad)
        chip->block_bad = spinand_block_bad;
}


/**
 * spinand_scan - [SpiNAND Interface] Scan for the SpiNAND device
 * @param mtd       MTD device structure
 *
 * This fills out all the not initialized function pointers
 * with the defaults.
 * The flash ID is read and the mtd/chip structures are
 * filled with the appropriate values.
 */
int spinand_scan(struct mtd_info *mtd)
{
    struct spinand_chip *chip = mtd->priv;
    struct spinand_info *info = chip->info;

    //chip->state = FL_READY;
    //init_waitqueue_head(&chip->wq);
    //spin_lock_init(&chip->chip_lock);
    spinand_set_defaults(chip);

    mtd->name           = info->deviceName;
    mtd->size           = info->usable_size;
    mtd->erasesize      = info->block_main_size;
    mtd->writesize      = info->page_main_size;
    mtd->writebufsize   = info->page_main_size;
    mtd->oobsize        = info->page_spare_size;
    mtd->oobavail       = info->ecclayout->oobavail;
    mtd->owner          = THIS_MODULE;
    mtd->type           = MTD_NANDFLASH;
    mtd->flags          = MTD_CAP_NANDFLASH;
    mtd->ecclayout      = info->ecclayout;

    mtd->_erase         = spinand_erase;
    mtd->_point         = NULL;
    mtd->_unpoint       = NULL;
    mtd->_read          = spinand_read;
    mtd->_write         = spinand_write;
    mtd->_read_oob      = spinand_read_oob;
    mtd->_write_oob     = spinand_write_oob;
    mtd->_sync          = spinand_sync;
    mtd->_lock          = NULL;
    mtd->_unlock        = NULL;
    mtd->_suspend       = spinand_suspend;
    mtd->_resume        = spinand_resume;
    mtd->_block_isbad   = spinand_block_isbad;
    mtd->_block_markbad = spinand_block_markbad;

    /* Check, if we should skip the bad block table scan */
    if (chip->options & NAND_SKIP_BBTSCAN)
        return 0;

    /* Build bad block table */
    return chip->scan_bbt(mtd);

}

EXPORT_SYMBOL_GPL(spinand_scan);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kewell Liu, <liujingke@gofortune-semi.com>");
MODULE_DESCRIPTION("SPI NAND driver code");
