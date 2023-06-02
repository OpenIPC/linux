/*
 *  drivers/mtd/spinand/spinand_bbt.c
 *
 *  Overview:
 *   Bad block table support for the SPI NAND driver
 *
 *  Copyright (c) 2013 Gofortune Semiconductor Corporation.
 *   Kewell Liu  <liujingke@gofortune-semi.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Description:
 *
 * When nand_scan_bbt is called, then it tries to find the bad block table
 * depending on the options in the BBT descriptor(s). If no flash based BBT
 * (NAND_BBT_USE_FLASH) is specified then the device is scanned for factory
 * marked good / bad blocks. This information is used to create a memory BBT.
 * Once a new bad block is discovered then the "factory" information is updated
 * on the device.
 * If a flash based BBT is specified then the function first tries to find the
 * BBT on flash. If a BBT is found then the contents are read and the memory
 * based BBT is created. If a mirrored BBT is selected then the mirror is
 * searched too and the versions are compared. If the mirror has a greater
 * version number than the mirror BBT is used to build the memory based BBT.
 * If the tables are not versioned, then we "or" the bad block information.
 * If one of the BBTs is out of date or does not exist it is (re)created.
 * If no BBT exists at all then the device is scanned for factory marked
 * good / bad blocks and the bad block tables are created.
 *
 * For manufacturer created BBTs like the one found on M-SYS DOC devices
 * the BBT is searched and read but never created
 *
 * The auto generated bad block table is located in the last good blocks
 * of the device. The table is mirrored, so it can be updated eventually.
 * The table is marked in the OOB area with an ident pattern and a version
 * number which indicates which of both tables is more up to date. If the NAND
 * controller needs the complete OOB area for the ECC information then the
 * option NAND_BBT_NO_OOB should be used (along with NAND_BBT_USE_FLASH, of
 * course): it moves the ident pattern and the version byte into the data area
 * and the OOB area will remain untouched.
 *
 * The table uses 2 bits per block
 * 11b:     block is good
 * 00b:     block is factory marked bad
 * 01b, 10b:    block is marked bad due to wear
 *
 * The memory bad block table uses the following scheme:
 * 00b:     block is good
 * 01b:     block is marked bad due to wear
 * 10b:     block is reserved (to protect the bbt area)
 * 11b:     block is factory marked bad
 *
 * Multichip devices like DOC store the bad block info per floor.
 *
 * Following assumptions are made:
 * - bbts start at a page boundary, if autolocated on a block boundary
 * - the space necessary for a bbt in FLASH does not exceed a block boundary
 *
 */

#include <linux/slab.h>
#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/export.h>

#include "spinand.h"

#ifdef CONFIG_SELF_START_BLOCK
#define START_BLOCK     18
#else
#define START_BLOCK     0
#endif


static int spinand_check_pattern_no_oob(uint8_t *buf, struct nand_bbt_descr *td)
{
    int ret;

    ret = memcmp(buf, td->pattern, td->len);
    if (!ret)
        return ret;
    return -1;
}

/**
 * spinand_check_pattern - [GENERIC] check if a pattern is in the buffer
 * @buf: the buffer to search
 * @len: the length of buffer to search
 * @paglen: the pagelength
 * @td: search pattern descriptor
 *
 * Check for a pattern at the given place. Used to search bad block tables and
 * good / bad block identifiers. If the SCAN_EMPTY option is set then check, if
 * all bytes except the pattern area contain 0xff.
 */
static int spinand_check_pattern(uint8_t *buf, int len, int paglen, struct nand_bbt_descr *td)
{
    int i, end = 0;
    uint8_t *p = buf;

    if (td->options & NAND_BBT_NO_OOB)
        return spinand_check_pattern_no_oob(buf, td);

    end = paglen + td->offs;
    if (td->options & NAND_BBT_SCANEMPTY)
    {
        for (i = 0; i < end; i++)
        {
            if (p[i] != 0xff)
                return -1;
        }
    }
    p += end;

    /* Compare the pattern */
    if (memcmp(p, td->pattern, td->len))
        return -1;

    if (td->options & NAND_BBT_SCANEMPTY)
    {
        p += td->len;
        end += td->len;
        for (i = end; i < len; i++)
        {
            if (*p++ != 0xff)
                return -1;
        }
    }

    return 0;

}

/**
 * spinand_check_short_pattern - [GENERIC] check if a pattern is in the buffer
 * @buf: the buffer to search
 * @td: search pattern descriptor
 *
 * Check for a pattern at the given place. Used to search bad block tables and
 * good / bad block identifiers. Same as check_pattern, but no optional empty
 * check.
 */
static int spinand_check_short_pattern(uint8_t *buf, struct nand_bbt_descr *td)
{
    int i;
    uint8_t *p = buf;

    /* Compare the pattern */
    for (i = 0; i < td->len; i++) {
        if (p[td->offs + i] != td->pattern[i])
            return -1;
    }
    return 0;
}

/**
 * spinand_add_marker_len - compute the length of the marker in data area
 * @td: BBT descriptor used for computation
 *
 * The length will be 0 if the marker is located in OOB area.
 */
static u32 spinand_add_marker_len(struct nand_bbt_descr *td)
{
    u32 len;

    if (!(td->options & NAND_BBT_NO_OOB))
        return 0;

    len = td->len;
    if (td->options & NAND_BBT_VERSION)
        len++;
    return len;
}

/**
 * spinand_read_bbt - [GENERIC] Read the bad block table starting from page
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @page: the starting page
 * @num: the number of bbt descriptors to read
 * @td: the bbt describtion table
 * @offs: offset in the memory table
 *
 * Read the bad block table starting from page.
 */
static int spinand_read_bbt(struct mtd_info *mtd, uint8_t *buf, int page, int num,
        struct nand_bbt_descr *td, int offs)
{
    int res, ret = 0, i, j, act = 0;
    struct spinand_chip *this = mtd->priv;
    size_t retlen, len, totlen;
    loff_t from;
    int bits = td->options & NAND_BBT_NRBITS_MSK;
    uint8_t msk = (uint8_t)((1 << bits) - 1);
    u32 marker_len;
    int reserved_block_code = td->reserved_block_code;

    totlen = (num * bits) >> 3;
    marker_len = spinand_add_marker_len(td);
    from = ((loff_t)page) << this->page_shift;

#ifdef CONFIG_DEBUG_R

    pr_info("[%s]: read bbt >>>\n", __FUNCTION__);
    pr_info("BlockNum     %d\n"
            "totlen       %d\n"
            "marker_len   %d\n"
            "from         0x%llx\n",
            totlen,
            marker_len,
            from);

#endif


    while (totlen)
    {
        len = min(totlen, (size_t)(1 << this->bbt_erase_shift));
        if (marker_len) {
           /*
                   * In case the BBT marker is not in the OOB area it
                   * will be just in the first page.
                   */
            len -= marker_len;
            from += marker_len;
            marker_len = 0;
        }

        res = mtd_read(mtd, from, len, &retlen, buf);
        if (res < 0)
        {
            if (mtd_is_eccerr(res)) {
                pr_info("%s: ECC error in BBT at 0x%012llx\n", __FUNCTION__,from & ~mtd->writesize);
                return res;
            } else if (mtd_is_bitflip(res)) {
                pr_info("%s: corrected error in BBT at 0x%012llx\n", __FUNCTION__,from & ~mtd->writesize);
                ret = res;
            } else {
                pr_info("%s: error reading BBT\n",__FUNCTION__);
                return res;
            }
        }

        /* Analyse data */
        for (i = 0; i < len; i++)
        {
            uint8_t dat = buf[i];
            for (j = 0; j < 8; j += bits, act += 2)
            {
                uint8_t tmp = (dat >> j) & msk;
                if (tmp == msk)
                    continue;
                if (reserved_block_code && (tmp == reserved_block_code))
                {
                    pr_info("%s: reserved block at 0x%012llx\n",__FUNCTION__,
                         (loff_t)((offs << 2) + (act >> 1)) << this->bbt_erase_shift);
                    this->bbt[offs + (act >> 3)] |= 0x2 << (act & 0x06);
                    mtd->ecc_stats.bbtblocks++;
                    continue;
                }

                /*
                           * Leave it for now, if it's matured we can
                           * move this message to pr_debug.
                           */
                pr_info("%s: bad block at 0x%012llx\n",__FUNCTION__,
                     (loff_t)((offs << 2) + (act >> 1)) << this->bbt_erase_shift);

                /* Factory marked bad or worn out? */
                if (tmp == 0)
                    this->bbt[offs + (act >> 3)] |= 0x3 << (act & 0x06);
                else
                    this->bbt[offs + (act >> 3)] |= 0x1 << (act & 0x06);
                mtd->ecc_stats.badblocks++;

            }
        }

        totlen -= len;
        from += len;

    }

    return ret;

}

/**
 * spinand_read_abs_bbt - [GENERIC] Read the bad block table starting at a given page
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @td: descriptor for the bad block table
 *
 * Read the bad block table for all chips starting at a given page. We assume
 * that the bbt bits are in consecutive order.
 */
static int spinand_read_abs_bbt(struct mtd_info *mtd, uint8_t *buf, struct nand_bbt_descr *td)
{
    struct spinand_chip *this = mtd->priv;
    int res = 0;

    res = spinand_read_bbt(mtd, buf, td->pages[0], mtd->size >> this->bbt_erase_shift, td, 0);
    if (res)
        return res;

    return 0;
}

/**
 * spinand_scan_read_raw_data - [GENERIC] Scan read raw data from flash
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @offs: offset in the memory table
 * @readlen: the length of the data to read
 *
 * Scan read raw data from flash, BBT marker is in the first page, no OOB
 */
static int spinand_scan_read_raw_data(struct mtd_info *mtd, uint8_t *buf, loff_t offs,
             struct nand_bbt_descr *td)
{
    size_t retlen;
    size_t len;

    len = td->len;
    if (td->options & NAND_BBT_VERSION)
        len++;

    return mtd_read(mtd, offs, len, &retlen, buf);
}

/**
 * spinand_scan_read_raw_oob - [GENERIC] Scan read oob raw data from flash
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @offs: offset in the memory table
 * @readlen: the length of the data to read
 *
 * Scan read oob raw data from flash
 */
static int spinand_scan_read_raw_oob(struct mtd_info *mtd, uint8_t *buf, loff_t offs,
             size_t readlen)
{
    struct mtd_oob_ops ops;
    int res;

    ops.mode = MTD_OPS_RAW;
    ops.ooboffs = 0;
    ops.ooblen = mtd->oobsize;

    while (readlen > 0)
    {
        ops.datbuf = buf;
        ops.len = min(readlen, (size_t)mtd->writesize);
        ops.oobbuf = buf + ops.len;

        res = mtd_read_oob(mtd, offs, &ops);

        if (res)
        {
            return res;
        }

        buf += mtd->oobsize + mtd->writesize;
        readlen -= mtd->writesize;
        offs += mtd->writesize;
    }

    return 0;

}

static int spinand_scan_read_raw(struct mtd_info *mtd, uint8_t *buf, loff_t offs,
             size_t len, struct nand_bbt_descr *td)
{
    if (td->options & NAND_BBT_NO_OOB)
        return spinand_scan_read_raw_data(mtd, buf, offs, td);
    else
        return spinand_scan_read_raw_oob(mtd, buf, offs, len);
}

/**
 * spinand_scan_write_bbt - [GENERIC] Scan write data with oob to flash
 * @mtd: MTD device structure
 * @offs: offset to write
 * @len: the length of the data to write
 * @buf: temporary buffer for main data
 * @oob: temporary buffer for oob data
 *
 * Scan write data with oob to flash
 */
static int spinand_scan_write_bbt(struct mtd_info *mtd, loff_t offs, size_t len,
              uint8_t *buf, uint8_t *oob)
{
    struct mtd_oob_ops ops;

    ops.mode = MTD_OPS_PLACE_OOB;
    ops.ooboffs = 0;
    ops.ooblen = mtd->oobsize;
    ops.datbuf = buf;
    ops.oobbuf = oob;
    ops.len = len;

#ifdef CONFIG_DEBUG_W
    pr_info("[%s]: scan write bbt >>>\n", __FUNCTION__);
    pr_info("offs         0x%llx\n"
            "len          0x%x\n"
            "oobbuf       0x%x\n",
             offs,
             len, oob);

#endif

    return mtd_write_oob(mtd, offs, &ops);
}

static u32 spinand_bbt_get_ver_offs(struct mtd_info *mtd, struct nand_bbt_descr *td)
{
    u32 ver_offs = td->veroffs;

    if (!(td->options & NAND_BBT_NO_OOB))
        ver_offs += mtd->writesize;
    return ver_offs;
}

/**
 * spinand_read_abs_bbts - [GENERIC] Read the bad block table(s) for all chips starting at a given page
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @td: descriptor for the bad block table
 * @md: descriptor for the bad block table mirror
 *
 * Read the bad block table(s) for all chips starting at a given page. We
 * assume that the bbt bits are in consecutive order.
 */
static int spinand_read_abs_bbts(struct mtd_info *mtd, uint8_t *buf,
             struct nand_bbt_descr *td, struct nand_bbt_descr *md)
{
    struct spinand_chip *this = mtd->priv;

    /* Read the primary version, if available */
    if (td->options & NAND_BBT_VERSION)
    {
        spinand_scan_read_raw(mtd, buf, (loff_t)td->pages[0] << this->page_shift, mtd->writesize, td);
        td->version[0] = buf[spinand_bbt_get_ver_offs(mtd, td)];
        pr_info("line%d: TD BBT at page %d, version 0x%02X\n", __LINE__,td->pages[0], td->version[0]);
    }

    /* Read the mirror version, if available */
    if (md && (md->options & NAND_BBT_VERSION))
    {
        spinand_scan_read_raw(mtd, buf, (loff_t)md->pages[0] << this->page_shift, mtd->writesize, md);
        md->version[0] = buf[spinand_bbt_get_ver_offs(mtd, md)];
        pr_info("Line%d: MD BBT at page %d, version 0x%02X\n", __LINE__,md->pages[0], md->version[0]);
    }

    return 1;

}

/**
 * spinand_scan_block_full - [GENERIC] Scan a given block full
 * @mtd: MTD device structure
 * @bd: descriptor for the good/bad block search pattern
 * @offs: address offset of spi nand(from)
 * @buf: temporary buffer
 * @readlen: the length of the data to read
 * @scanlen: the length of the data to scan
 * @rpagenum: the number of the pages to read
 *
 * Scan the full pages in a given block
 */
static int spinand_scan_block_full(
        struct mtd_info *mtd, struct nand_bbt_descr *bd,
        loff_t offs, uint8_t *buf, size_t readlen,
        int scanlen, int rpagenum)
{
    int ret, j;

    /* read oob raw */
    ret = spinand_scan_read_raw_oob(mtd, buf, offs, readlen);

    /* Ignore ECC errors when checking for BBM */
    if (ret && !mtd_is_bitflip_or_eccerr(ret))
        return ret;

    /* check pattern per page */
    for (j = 0; j < rpagenum; j++, buf += scanlen) {
        if (spinand_check_pattern(buf, scanlen, mtd->writesize, bd))
            return 1;
    }
    return 0;
}

/**
 * spinand_scan_block_fast - [GENERIC] Scan a given block partially
 * @mtd: MTD device structure
 * @bd: descriptor for the good/bad block search pattern
 * @offs: address offset of spi nand(from)
 * @buf: temporary buffer
 * @rpagenum: the number of the pages to read
 *
 * Scan the partially pages in a given block
 */
static int spinand_scan_block_fast(struct mtd_info *mtd, struct nand_bbt_descr *bd,
               loff_t offs, uint8_t *buf, int rpagenum)
{
    struct mtd_oob_ops ops;
    int j, ret;

    ops.ooblen = mtd->oobsize;
    ops.oobbuf = buf;
    ops.ooboffs = 0;
    ops.datbuf = NULL;
    ops.mode = MTD_OPS_RAW;
    ops.len = 0;

    for (j = 0; j < rpagenum; j++)
    {
        /*
              * Read the full oob until read_oob is fixed to handle single
              * byte reads for 16 bit buswidth.
              */
        ret = mtd_read_oob(mtd, offs, &ops);
//		printk("read oob return val is 0x%x\n", ret); 
        /* Ignore ECC errors when checking for BBM */
        if (ret && !mtd_is_bitflip_or_eccerr(ret))
            return ret;

        if (spinand_check_short_pattern(buf, bd))
            return 1;

        offs += mtd->writesize;
    }
    return 0;
}

/**
 * spinand_create_bbt - [GENERIC] Create a bad block table by scanning the device
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @bd: descriptor for the good/bad block search pattern
 * @chip: create the table for a specific chip, -1 read all chips; applies only
 *        if NAND_BBT_PERCHIP option is set
 *
 * Create a bad block table by scanning the device for the given good/bad block
 * identify pattern.
 */
static int spinand_create_bbt(struct mtd_info *mtd, uint8_t *buf, struct nand_bbt_descr *bd)
{
    struct spinand_chip *this = mtd->priv;
    int i, numblocks, rpagenum, scanlen;
    int startblock;
    loff_t from;
    size_t readlen;

    pr_info("%s: Scanning device for bad blocks\n", __FUNCTION__);

    /* get the page numbers what we want to scan */
    if (bd->options & NAND_BBT_SCANALLPAGES)
        rpagenum = 1 << (this->bbt_erase_shift - this->page_shift);
    else if (bd->options & NAND_BBT_SCAN2NDPAGE)
        rpagenum = 2;
    else
        rpagenum = 1;

    if (!(bd->options & NAND_BBT_SCANEMPTY)) {
        /* We need only read few bytes from the OOB area */
        scanlen = 0;
        readlen = bd->len;
    } else {
        /* Full page content should be read */
        scanlen = mtd->writesize + mtd->oobsize;
        readlen = rpagenum * mtd->writesize;
    }

    /*
        * Note that numblocks is 2 * (real numblocks) here, see i+=2
        * below as it makes shifting and masking less painful
        */
    numblocks = mtd->size >> (this->bbt_erase_shift - 1);
    startblock = 0;
    from = 0;

    /* only scan the last page */
    if (this->bbt_options & NAND_BBT_SCANLASTPAGE)
        from += mtd->erasesize - (mtd->writesize * rpagenum);

    for (i = startblock; i < numblocks;)
    {
        int ret;

        BUG_ON(bd->options & NAND_BBT_NO_OOB);

        if (bd->options & NAND_BBT_SCANALLPAGES)
            ret = spinand_scan_block_full(mtd, bd, from, buf, readlen, scanlen, rpagenum);
        else
            ret = spinand_scan_block_fast(mtd, bd, from, buf, rpagenum);

        if (ret < 0)
        {
            pr_err("[%s]Never go here!\n", __FUNCTION__);
            return ret;
        }

        if (ret) {
            this->bbt[i >> 3] |= 0x03 << (i & 0x6);
            pr_info("Bad eraseblock %d at 0x%012llx\n",
                i >> 1, (unsigned long long)from);
            mtd->ecc_stats.badblocks++;
        }

        i += 2;
        from += (1 << this->bbt_erase_shift);

    }

#ifdef CONFIG_DEBUG_R
{
    pr_info("%s: BBT content:\n", __FUNCTION__);
    
    int i = 0, j = 0;

    for(i = 0,j = 0; i < 512; i++,j++)
    {
        if(j % 16 == 0)
        {
            printk("\n");
        }
        printk("%02x ", this->bbt[i]);
    }

    printk("\n");
}
#endif

    return 0;

}

/**
 * spinand_search_bbt - [GENERIC] scan the device for a specific bad block table
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @td: descriptor for the bad block table
 *
 * Read the bad block table by searching for a given ident pattern. Search is
 * preformed either from the beginning up or from the end of the device
 * downwards. The search starts always at the start of a block. If the option
 * NAND_BBT_PERCHIP is given, each chip is searched for a bbt, which contains
 * the bad block information of this chip. This is necessary to provide support
 * for certain DOC devices.
 *
 * The bbt ident pattern resides in the oob area of the first page in a block.
 */
static int spinand_search_bbt(struct mtd_info *mtd, uint8_t *buf, struct nand_bbt_descr *td)
{
    struct spinand_chip *this = mtd->priv;

    int startblock, block, dir;
    int scanlen = mtd->writesize + mtd->oobsize;
    int bbtblocks;
    int blocktopage = this->bbt_erase_shift - this->page_shift;

#ifdef CONFIG_DEBUG_R

    pr_info("[%s]: search bbt >>>\n", __FUNCTION__);

#endif


    /* Search direction top -> down? */
    if (td->options & NAND_BBT_LASTBLOCK)
    {
        startblock = (mtd->size >> this->bbt_erase_shift) - 1;
        dir = -1;
    }
    else
    {
        startblock = START_BLOCK;
        dir = 1;
    }

    /* calc block number on device */
    bbtblocks = mtd->size >> this->bbt_erase_shift;

    /* Reset version information */
    td->version[0] = 0;
    td->pages[0] = -1;

    /* Scan the maximum number of blocks */
    for (block = 0; block < td->maxblocks; block++)
    {
        int actblock = startblock + dir * block;
        loff_t offs = (loff_t)actblock << this->bbt_erase_shift;

#ifdef CONFIG_DEBUG_R

        pr_info("actblock     0x%08x\n"
                "offs         0x%llx\n",
                actblock,
                offs);

#endif

        /* Read first page */
        spinand_scan_read_raw(mtd, buf, offs, mtd->writesize, td);

#ifdef CONFIG_DEBUG_R
        printk("Pattern: %c,%c,%c,%c\n",td->pattern[0],td->pattern[1],td->pattern[2],td->pattern[3]);
        printk("Pattern: %x,%x,%x,%x",td->pattern[0],td->pattern[1],td->pattern[2],td->pattern[3]);

        {
            int i = 0, j = 0;

            for(i = 0,j = 0; i < 128; i++,j++)
            {
                if(j % 16 == 0)
                {
                    printk("\n");
                }
                printk("%02x ", buf[i]);
            }

            printk("\n");
        }
#endif

        if (!spinand_check_pattern(buf, scanlen, mtd->writesize, td))
        {
            td->pages[0] = actblock << blocktopage;
            if (td->options & NAND_BBT_VERSION)
            {
                offs = spinand_bbt_get_ver_offs(mtd, td);
                td->version[0] = buf[offs];
            }
            break;
        }
    }

    /* Check, if we found a bbt  */
    if (td->pages[0] == -1)
        pr_warn("Bad block table not found\n");
    else
        pr_info("Bad block table found at page %d, version 0x%02X\n", td->pages[0], td->version[0]);

    return 0;

}

/**
 * spinand_search_read_bbts - [GENERIC] scan the device for bad block table(s)
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @td: descriptor for the bad block table
 * @md: descriptor for the bad block table mirror
 *
 * Search and read the bad block table(s).
 */
static int spinand_search_read_bbts(
        struct mtd_info *mtd, uint8_t * buf,
        struct nand_bbt_descr *td, struct nand_bbt_descr *md)
{
    /* Search the primary table */
    spinand_search_bbt(mtd, buf, td);

    /* Search the mirror table */
    if (md)
        spinand_search_bbt(mtd, buf, md);

    /* Force result check */
    return 1;
}

/**
 * spinand_write_bbt - [GENERIC] (Re)write the bad block table
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @td: descriptor for the bad block table
 * @md: descriptor for the bad block table mirror
 *
 * (Re)write the bad block table.
 */
static int spinand_write_bbt(struct mtd_info *mtd, uint8_t *buf,
             struct nand_bbt_descr *td, struct nand_bbt_descr *md)
{
    struct spinand_chip *this = mtd->priv;
    struct erase_info einfo;
    int i, j, res;
    int bits, startblock, dir, page, offs, numblocks, sft, sftmsk;
    int bbtoffs, pageoffs, ooboffs;
    uint8_t msk[4];
    uint8_t rcode = td->reserved_block_code;
    size_t retlen, len = 0;
    loff_t to;
    struct mtd_oob_ops ops;

    ops.ooblen = mtd->oobsize;
    ops.ooboffs = 0;
    ops.datbuf = NULL;
    ops.mode = MTD_OPS_PLACE_OOB;
    ops.len = 0;

#ifdef CONFIG_DEBUG_W
    pr_info("[%s]: write bbt start >>>\n", __FUNCTION__);
#endif

    if (!rcode)
        rcode = 0xff;

    /* Write bad block table per chip rather than per device? */
    numblocks = (int)(mtd->size >> this->bbt_erase_shift);

    /*
        * There was already a version of the table, reuse the page
        * This applies for absolute placement too, as we have the
        * page nr. in td->pages.
        */
    if (td->pages[0] != -1) {
        page = td->pages[0];
        goto write;
    }

    /*
        * Automatic placement of the bad block table. Search direction
        * top -> down?
        */
    if (td->options & NAND_BBT_LASTBLOCK) {
        startblock = numblocks - 1;
        dir = -1;
    } else {
        startblock = START_BLOCK;
        dir = 1;
    }

    for (i = 0; i < td->maxblocks; i++)
    {
        int block = startblock + dir * i;

        /* Check, if the block is bad */
        switch ((this->bbt[block >> 2] >> (2 * (block & 0x03))) & 0x03)
        {
            case 0x01:
            case 0x03:
                continue;
        }
        page = block << (this->bbt_erase_shift - this->page_shift);

        /* Check, if the block is used by the mirror table */
        if (!md || md->pages[0] != page)
            goto write;
    }

    pr_err("No space left to write bad block table\n");
    return -ENOSPC;

write:

    /* Set up shift count and masks for the flash table */
    bits = td->options & NAND_BBT_NRBITS_MSK;
    msk[2] = ~rcode;
    switch (bits) {
    case 1: sft = 3; sftmsk = 0x07; msk[0] = 0x00; msk[1] = 0x01;
        msk[3] = 0x01;
        break;
    case 2: sft = 2; sftmsk = 0x06; msk[0] = 0x00; msk[1] = 0x01;
        msk[3] = 0x03;
        break;
    case 4: sft = 1; sftmsk = 0x04; msk[0] = 0x00; msk[1] = 0x0C;
        msk[3] = 0x0f;
        break;
    case 8: sft = 0; sftmsk = 0x00; msk[0] = 0x00; msk[1] = 0x0F;
        msk[3] = 0xff;
        break;
    default: return -EINVAL;
    }

    bbtoffs = 0;

    to = ((loff_t)page) << this->page_shift;

    /* Must we save the block contents? */
    if (td->options & NAND_BBT_SAVECONTENT)
    {
        /* Make it block aligned */
        to &= ~((loff_t)((1 << this->bbt_erase_shift) - 1));
        len = 1 << this->bbt_erase_shift;
        res = mtd_read(mtd, to, len, &retlen, buf);
        if (res < 0)
        {
            if (retlen != len) {
                pr_info("nand_bbt: error reading block for writing the bad block table\n");
                return res;
            }
            pr_warn("nand_bbt: ECC error while reading block for writing bad block table\n");
        }

        /* Read oob data */
        ops.ooblen = (len >> this->page_shift) * mtd->oobsize;
        ops.oobbuf = &buf[len];
        res = mtd_read_oob(mtd, to + mtd->writesize, &ops);
        if (res < 0 || ops.oobretlen != ops.ooblen)
            goto outerr;

        /* Calc the byte offset in the buffer */
        pageoffs = page - (int)(to >> this->page_shift);
        offs = pageoffs << this->page_shift;
        /* Preset the bbt area with 0xff */
        memset(&buf[offs], 0xff, (size_t)(numblocks >> sft));
        ooboffs = len + (pageoffs * mtd->oobsize);

    }
    else if (td->options & NAND_BBT_NO_OOB)
    {
        ooboffs = 0;
        offs = td->len;

        /* The version byte */
        if (td->options & NAND_BBT_VERSION)
            offs++;
        /* Calc length */
        len = (size_t)(numblocks >> sft);
        len += offs;
        /* Make it page aligned! */
        len = ALIGN(len, mtd->writesize);

#ifdef CONFIG_DEBUG_W

        pr_info("[%s]: must go here >>>\n", __FUNCTION__);
        pr_info("  len = %d\n", len);

#endif

        
        /* Preset the buffer with 0xff */
        memset(buf, 0xff, len);
        /* Pattern is located at the begin of first page */
        memcpy(buf, td->pattern, td->len);
    }
    else
    {
        /* Calc length */
        len = (size_t)(numblocks >> sft);
        /* Make it page aligned! */
        len = ALIGN(len, mtd->writesize);
        /* Preset the buffer with 0xff */
        memset(buf, 0xff, len +
               (len >> this->page_shift)* mtd->oobsize);
        offs = 0;
        ooboffs = len;
        /* Pattern is located in oob area of first page */
        memcpy(&buf[ooboffs + td->offs], td->pattern, td->len);
    }

    if (td->options & NAND_BBT_VERSION)
        buf[ooboffs + td->veroffs] = td->version[0];

    /* Walk through the memory table */
    for (i = 0; i < numblocks;)
    {
        uint8_t dat;
        dat = this->bbt[bbtoffs + (i >> 2)];
        for (j = 0; j < 4; j++, i++)
        {
            int sftcnt = (i << (3 - sft)) & sftmsk;
            /* Do not store the reserved bbt blocks! */
            buf[offs + (i >> sft)] &= ~(msk[dat & 0x03] << sftcnt);
            dat >>= 2;
        }
    }

    memset(&einfo, 0, sizeof(einfo));
    einfo.mtd = mtd;
    einfo.addr = to;
    einfo.len = 1 << this->bbt_erase_shift;
    res = spinand_erase_nand(mtd, &einfo, 1);
    if (res < 0)
        goto outerr;

#ifdef CONFIG_DEBUG_W
    pr_info("[%s]: erase OK! len = %d >>>\n", __FUNCTION__, len);
#endif


    res = spinand_scan_write_bbt(mtd, to, len, buf,
            td->options & NAND_BBT_NO_OOB ? NULL : &buf[len]);
    if (res < 0)
        goto outerr;

    pr_info("Bad block table written to 0x%012llx, version 0x%02X\n",
         (unsigned long long)to, td->version[0]);

    /* Mark it as used */
    td->pages[0] = page;

    return 0;

 outerr:
    pr_warn("write_bbt: error while writing bbt block = 0x%llx, %d\n",to,res);
    return res;

}

/**
 * spinand_memory_bbt - [GENERIC] create a memory based bad block table
 * @mtd: MTD device structure
 * @bd: descriptor for the good/bad block search pattern
 *
 * The function creates a memory based bbt by scanning the device for
 * manufacturer / software marked good / bad blocks.
 */
static inline int spinand_memory_bbt(struct mtd_info *mtd, struct nand_bbt_descr *bd)
{
    struct spinand_chip *chip = mtd->priv;

    bd->options &= ~NAND_BBT_SCANEMPTY;
    return spinand_create_bbt(mtd, chip->buffers->databuf, bd);
}

/**
 * spinand_check_create - [GENERIC] create and write bbt(s) if necessary
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @bd: descriptor for the good/bad block search pattern
 *
 * The function checks the results of the previous call to read_bbt and creates
 * / updates the bbt(s) if necessary. Creation is necessary if no bbt was found
 * for the chip/device. Update is necessary if one of the tables is missing or
 * the version nr. of one table is less than the other.
 */
static int spinand_check_create(struct mtd_info *mtd, uint8_t *buf, struct nand_bbt_descr *bd)
{
    int writeops, create, res, res2, tpageno, mpageno;
    struct spinand_chip *this = mtd->priv;
    struct nand_bbt_descr *td = this->bbt_td;
    struct nand_bbt_descr *md = this->bbt_md;
    struct nand_bbt_descr *rd, *rd2;
    uint8_t tversion, mversion;

    writeops = 0;
    create = 0;
    rd = NULL;
    rd2 = NULL;
    res = res2 = 0;
    tpageno = td->pages[0];
    mpageno = md->pages[0];
    tversion = td->version[0];
    mversion = md->version[0];

#ifdef CONFIG_DEBUG_R

    pr_info("[%s]: check & create >>>\n", __FUNCTION__);
    pr_info("tpageno      0x%08x\n"
            "mpageno      0x%08x\n"
            "tversion     0x%08x\n"
            "mversion     0x%08x\n",
            tpageno,
            mpageno,
            tversion,
            mversion);

#endif


    /* Mirrored table available? */
    if (md)
    {
        if (tpageno == -1 && mpageno == -1) {
            create = 1;
            writeops = 0x03;
        } else if (tpageno == -1) {
            rd = md;
            writeops = 0x01;
        } else if (mpageno == -1) {
            rd = td;
            writeops = 0x02;
        } else if (tversion == mversion) {
            rd = td;
            if (!(td->options & NAND_BBT_VERSION))
                rd2 = md;
        } else if (((int8_t)(tversion - mversion)) > 0) {
            rd = td;
            writeops = 0x02;
        } else {
            rd = md;
            writeops = 0x01;
        }
    }
    else {
        if (tpageno == -1) {
            create = 1;
            writeops = 0x01;
        } else {
            rd = td;
        }
    }

    if (create)
    {
        /* Create the bad block table by scanning the device? */
        if (!(td->options & NAND_BBT_CREATE))
           goto out;

        /* Create the table in memory by scanning the chip(s) */
        if (!(this->bbt_options & NAND_BBT_CREATE_EMPTY))
            spinand_create_bbt(mtd, buf, bd);

        td->version[0] = 1;
        if (md)
            md->version[0] = 1;
    }

    /* Read back first? */
    if (rd)
    {
        res = spinand_read_abs_bbt(mtd, buf, rd);
        if (mtd_is_eccerr(res)) {
            /* Mark table as invalid */
            rd->pages[0] = -1;
            rd->version[0] = 0;
            goto out;;
        }
    }

    /* If they weren't versioned, read both */
    if (rd2)
    {
        res2 = spinand_read_abs_bbt(mtd, buf, rd2);
        if (mtd_is_eccerr(res2)) {
            /* Mark table as invalid */
            rd2->pages[0] = -1;
            rd2->version[0] = 0;
            goto out;;
        }
    }

    /* Scrub the flash table(s)? */
    if (mtd_is_bitflip(res) || mtd_is_bitflip(res2))
        writeops = 0x03;

    /* Update version numbers before writing */
    if (md) {
        td->version[0] = max(td->version[0], md->version[0]);
        md->version[0] = td->version[0];
    }

    /* Write the bad block table to the device? */
    if ((writeops & 0x01) && (td->options & NAND_BBT_WRITE)) {
        res = spinand_write_bbt(mtd, buf, td, md);
        if (res < 0)
            return res;
    }

    /* Write the mirror bad block table to the device? */
    if ((writeops & 0x02) && md && (md->options & NAND_BBT_WRITE)) {
        res = spinand_write_bbt(mtd, buf, md, td);
        if (res < 0)
            return res;
    }

out:

    return 0;

}

/**
 * spinand_update_bbt - [NAND Interface] update bad block table(s)
 * @mtd: MTD device structure
 * @offs: the offset of the newly marked block
 *
 * The function updates the bad block table(s).
 */
int spinand_update_bbt(struct mtd_info *mtd, loff_t offs)
{
    struct spinand_chip *this = mtd->priv;
    int len, res = 0;
    uint8_t *buf;
    struct nand_bbt_descr *td = this->bbt_td;
    struct nand_bbt_descr *md = this->bbt_md;

#ifdef CONFIG_DEBUG_W
        pr_info("[%s]: update bbt >>>\n", __FUNCTION__);
#endif

    if (!this->bbt || !td)
        return -EINVAL;

    /* Allocate a temporary buffer for one eraseblock incl. oob */
    len = (1 << this->bbt_erase_shift);
    len += (len >> this->page_shift) * mtd->oobsize;
    buf = kmalloc(len, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;

    td->version[0]++;
    if (md)
        md->version[0]++;

    /* Write the bad block table to the device? */
    if (td->options & NAND_BBT_WRITE)
    {
        res = spinand_write_bbt(mtd, buf, td, md);
        if (res < 0)
            goto out;
    }

    /* Write the mirror bad block table to the device? */
    if (md && (md->options & NAND_BBT_WRITE))
    {
        res = spinand_write_bbt(mtd, buf, md, td);
    }

 out:

#ifdef CONFIG_DEBUG_W
    pr_info("[%s]: update bbt ret = %d <<<\n", __FUNCTION__, res);
#endif

    kfree(buf);
    return res;

}

/**
 * spinand_mark_bbt_regions - [GENERIC] mark the bad block table regions
 * @mtd: MTD device structure
 * @td: bad block table descriptor
 *
 * The bad block table regions are marked as "bad" to prevent accidental
 * erasures / writes. The regions are identified by the mark 0x02.
 */
static void spinand_mark_bbt_region(struct mtd_info *mtd, struct nand_bbt_descr *td)
{
    struct spinand_chip *this = mtd->priv;
    int j, block, nrblocks, update;
    uint8_t oldval, newval;

#ifdef CONFIG_DEBUG_W
    pr_info("[%s]: mark start >>>\n", __FUNCTION__);
#endif


    /* calc the block number */
    nrblocks = (int)(mtd->size >> this->bbt_erase_shift);

    if ((td->options & NAND_BBT_ABSPAGE) || !(td->options & NAND_BBT_WRITE))
    {
        if (td->pages[0] == -1)
            return;
        block = td->pages[0] >> (this->bbt_erase_shift - this->page_shift);
        block <<= 1;
        oldval = this->bbt[(block >> 3)];
        newval = oldval | (0x2 << (block & 0x06));
        this->bbt[(block >> 3)] = newval;
        if ((oldval != newval) && td->reserved_block_code)
            spinand_update_bbt(mtd, (loff_t)block << (this->bbt_erase_shift - 1));
        return;
    }

    update = 0;
    /* get the first block */
    if (td->options & NAND_BBT_LASTBLOCK)
        block = nrblocks - td->maxblocks;
    else
        block = START_BLOCK;

    block <<= 1;

#ifdef CONFIG_DEBUG_W
    pr_info("   block    : %d\n"
            "   maxblock : %d\n", block, td->maxblocks);
#endif

    for (j = 0; j < td->maxblocks; j++)
    {
        oldval = this->bbt[(block >> 3)];
        newval = oldval | (0x2 << (block & 0x06));
        this->bbt[(block >> 3)] = newval;
        if (oldval != newval)
            update = 1;
        block += 2;

#ifdef CONFIG_DEBUG_W
    pr_info("Reserve block:\n");
    pr_info("   oldval    : %d\n"
            "   newval    : %d\n"
            "   block     : %d\n", oldval, newval, (block - 2));
#endif
    }

    /*
        * If we want reserved blocks to be recorded to flash, and some
        * new ones have been marked, then we need to update the stored
        * bbts.  This should only happen once.
        */
    if (update && td->reserved_block_code)
        spinand_update_bbt(mtd, (loff_t)(block - 2) << (this->bbt_erase_shift - 1));

#ifdef CONFIG_DEBUG_W
    pr_info("[%s]: mark end <<<\n", __FUNCTION__);
#endif

}

/**
 * spinand_verify_bbt_descr - verify the bad block description
 * @mtd: MTD device structure
 * @bd: the table to verify
 *
 * This functions performs a few sanity checks on the bad block description
 * table.
 */
static void spinand_verify_bbt_descr(struct mtd_info *mtd, struct nand_bbt_descr *bd)
{
    struct spinand_chip *this = mtd->priv;
    u32 pattern_len;
    u32 bits;
    u32 table_size;

    if (!bd)
    {
        return;
    }

    pattern_len = bd->len;
    bits = bd->options & NAND_BBT_NRBITS_MSK;

    BUG_ON((this->bbt_options & NAND_BBT_NO_OOB) &&
            !(this->bbt_options & NAND_BBT_USE_FLASH));
    BUG_ON(!bits);

    if (bd->options & NAND_BBT_VERSION)
    {
        pattern_len++;
    }

    if (bd->options & NAND_BBT_NO_OOB)
    {
        BUG_ON(!(this->bbt_options & NAND_BBT_USE_FLASH));
        BUG_ON(!(this->bbt_options & NAND_BBT_NO_OOB));
        BUG_ON(bd->offs);
        if (bd->options & NAND_BBT_VERSION)
        {
            BUG_ON(bd->veroffs != bd->len);
        }
        BUG_ON(bd->options & NAND_BBT_SAVECONTENT);
    }

    table_size = mtd->size >> this->bbt_erase_shift;

    table_size >>= 3;
    table_size *= bits;
    if (bd->options & NAND_BBT_NO_OOB)
    {
        table_size += pattern_len;
    }

    BUG_ON(table_size > (1 << this->bbt_erase_shift));

}

/**
 * spinand_scan_bbt - [NAND Interface] scan, find, read and maybe create bad block table(s)
 * @mtd: MTD device structure
 * @bd: descriptor for the good/bad block search pattern
 *
 * The function checks, if a bad block table(s) is/are already available. If
 * not it scans the device for manufacturer marked good / bad blocks and writes
 * the bad block table(s) to the selected place.
 *
 * The bad block table memory is allocated here. It must be freed by calling
 * the nand_free_bbt function.
 */
int spinand_scan_bbt(struct mtd_info *mtd, struct nand_bbt_descr *bd)
{
    struct spinand_chip *this = mtd->priv;
    int len, res = 0;
    uint8_t *buf;
    struct nand_bbt_descr *td = this->bbt_td;
    struct nand_bbt_descr *md = this->bbt_md;

    len = mtd->size >> (this->bbt_erase_shift + 2);
    /*
     * Allocate memory (2bit per block) and clear the memory bad block
     * table.
     */
    this->bbt = kzalloc(len, GFP_KERNEL);
    if (!this->bbt)
    {
        pr_err("scan_bbt: no memory for bad block\n");
        return -ENOMEM;
    }

    /*
     * If no primary table decriptor is given, scan the device to build a
     * memory based bad block table.
     */
    if (!td) {
        if ((res = spinand_memory_bbt(mtd, bd))) {
            pr_err("scan_bbt: can't scan flash and build the RAM-based BBT\n");
            kfree(this->bbt);
            this->bbt = NULL;
        }
        return res;
    }
    spinand_verify_bbt_descr(mtd, td);
    spinand_verify_bbt_descr(mtd, md);

    /* Allocate a temporary buffer for one eraseblock incl. oob */
    len = (1 << this->bbt_erase_shift);
    len += (len >> this->page_shift) * mtd->oobsize;
    buf = vmalloc(len);
    if (!buf) {
        kfree(this->bbt);
        this->bbt = NULL;
        return -ENOMEM;
    }

    /* Is the bbt at a given page? */
    if (td->options & NAND_BBT_ABSPAGE) {
        res = spinand_read_abs_bbts(mtd, buf, td, md);
    } else {
        /* Search the bad block table using a pattern in oob */
        res = spinand_search_read_bbts(mtd, buf, td, md);
    }

    /* create a new BBT */
    if (res)
    {
        res = spinand_check_create(mtd, buf, bd);
    }

    /* Prevent the bbt regions from erasing / writing */
    spinand_mark_bbt_region(mtd, td);
    if (md)
    {
        spinand_mark_bbt_region(mtd, md);
    }

    /* free the above temporary buffer */
    vfree(buf);
    return res;

}

/*
 * Define some generic bad / good block scan pattern which are used
 * while scanning a device for factory marked good / bad blocks.
 */
static uint8_t scan_ff_pattern[] = { 0xff, 0xff };

/* Generic flash bbt descriptors */
static uint8_t bbt_pattern[] = {'B', 'b', 't', '0' };
static uint8_t mirror_pattern[] = {'1', 't', 'b', 'B' };

static struct nand_bbt_descr spinand_bbt_main = {
    .options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
        | NAND_BBT_2BIT | NAND_BBT_VERSION ,
    .offs = 8,
    .len = 4,
    .veroffs = 12,
    .maxblocks = 4,
    .pattern = bbt_pattern
};

static struct nand_bbt_descr spinand_bbt_mirror = {
    .options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
        | NAND_BBT_2BIT | NAND_BBT_VERSION ,
    .offs = 8,
    .len = 4,
    .veroffs = 12,
    .maxblocks = 4,
    .pattern = mirror_pattern
};

static struct nand_bbt_descr spinand_bbt_main_no_bbt = {
    .options = NAND_BBT_CREATE | NAND_BBT_WRITE
        | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_NO_OOB,
    .len = 4,
    .veroffs = 4,
    .maxblocks = 4,
    //.reserved_block_code = 2,
    .pattern = bbt_pattern
};

static struct nand_bbt_descr spinand_bbt_mirror_no_bbt = {
    .options = NAND_BBT_CREATE | NAND_BBT_WRITE
        | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_NO_OOB,
    .len = 4,
    .veroffs = 4,
    .maxblocks = 4,
    //.reserved_block_code = 2,
    .pattern = mirror_pattern
};

#define BADBLOCK_SCAN_MASK (~NAND_BBT_NO_OOB)
/**
 * spinand_create_badblock_pattern - [INTERN] Creates a BBT descriptor structure
 * @this: NAND chip to create descriptor for
 *
 * This function allocates and initializes a nand_bbt_descr for BBM detection
 * based on the properties of @this. The new descriptor is stored in
 * this->badblock_pattern. Thus, this->badblock_pattern should be NULL when
 * passed to this function.
 */
static int spinand_create_badblock_pattern(struct spinand_chip *this)
{
    struct nand_bbt_descr *bd;
    if (this->badblock_pattern) {
        pr_warn("Bad block pattern already allocated; not replacing\n");
        return -EINVAL;
    }
    bd = kzalloc(sizeof(*bd), GFP_KERNEL);
    if (!bd)
        return -ENOMEM;
    bd->options = this->bbt_options & BADBLOCK_SCAN_MASK;
    bd->offs = this->badblockpos;
    bd->len = (this->options & NAND_BUSWIDTH_16) ? 2 : 1;
    bd->pattern = scan_ff_pattern;
    bd->options |= NAND_BBT_DYNAMICSTRUCT;
    this->badblock_pattern = bd;
    return 0;
}

/**
 * spinand_default_bbt - [NAND Interface] Select a default bad block table for the device
 * @mtd: MTD device structure
 *
 * This function selects the default bad block table support for the device and
 * calls the nand_scan_bbt function.
 */
int spinand_default_bbt(struct mtd_info *mtd)
{
    struct spinand_chip *this = mtd->priv;


    /* Is a flash based bad block table requested? */
    if (this->bbt_options & NAND_BBT_USE_FLASH) {
        /* Use the default pattern descriptors */
        if (!this->bbt_td) {
            if (this->bbt_options & NAND_BBT_NO_OOB) {
                this->bbt_td = &spinand_bbt_main_no_bbt;
                this->bbt_md = &spinand_bbt_mirror_no_bbt;
            } else {
                this->bbt_td = &spinand_bbt_main;
                this->bbt_md = &spinand_bbt_mirror;
            }
        }
    } else {
        this->bbt_td = NULL;
        this->bbt_md = NULL;
    }

    if (!this->badblock_pattern)
        spinand_create_badblock_pattern(this);

    return spinand_scan_bbt(mtd, this->badblock_pattern);

}

/**
 * spinand_isbad_bbt - [NAND Interface] Check if a block is bad
 * @mtd: MTD device structure
 * @offs: offset in the device
 * @allowbbt: allow access to bad block table region
 */
int spinand_isbad_bbt(struct mtd_info *mtd, loff_t offs, int allowbbt)
{
    struct spinand_chip *this = mtd->priv;
    int block;
    uint8_t res;

    /* Get block number * 2 */
    block = (int)(offs >> (this->bbt_erase_shift - 1));
    res = (this->bbt[block >> 3] >> (block & 0x06)) & 0x03;

    pr_debug("spinand_isbad_bbt(): bbt info for offs 0x%08x: "
            "(block %d) 0x%02x\n",(unsigned int)offs, block >> 1, res);

    switch ((int)res) {
    case 0x00:
        return 0;
    case 0x01:
        return 1;
    case 0x02:
        return allowbbt ? 0 : 1;
    }
    return 1;

}

EXPORT_SYMBOL(spinand_isbad_bbt);
EXPORT_SYMBOL(spinand_scan_bbt);
EXPORT_SYMBOL(spinand_default_bbt);

