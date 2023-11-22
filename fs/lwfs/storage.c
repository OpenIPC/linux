/*
* storage.c- Sigmastar
*
* Copyright (c) [2019~2020] SigmaStar Technology.
*
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License version 2 for more details.
*
*/

#include <linux/fs.h>
#include <linux/mtd/super.h>
#include <linux/buffer_head.h>
#include "internal.h"

#if !defined(CONFIG_LWFS_ON_MTD) && !defined(CONFIG_LWFS_ON_BLOCK)
#error no LWFS backing store interface configured
#endif

#ifdef CONFIG_LWFS_ON_MTD
#define LWFS_MTD_READ(sb, ...) mtd_read((sb)->s_mtd, ##__VA_ARGS__)

/*
 * read data from an lwfs image on an MTD device
 */
static int lwfs_mtd_read(struct super_block *sb, unsigned long pos,
              void *buf, size_t buflen)
{
    size_t rlen;
    int ret;

    ret = LWFS_MTD_READ(sb, pos, buflen, &rlen, buf);
    return (ret < 0 || rlen != buflen) ? -EIO : 0;
}
#endif /* CONFIG_LWFS_ON_MTD */

#ifdef CONFIG_LWFS_ON_BLOCK
/*
 * read data from an lwfs image on a block device
 */
static int lwfs_blk_read(struct super_block *sb, unsigned long pos,
              void *buf, size_t buflen)
{
    struct buffer_head *bh;
    unsigned long offset;
    size_t segment;

    /* copy the string up to blocksize bytes at a time */
    while (buflen > 0) {
        offset = pos & (LWBSIZE - 1);
        segment = min_t(size_t, buflen, LWBSIZE - offset);
        bh = sb_bread(sb, pos >> LWBSBITS);
        if (!bh)
            return -EIO;
        memcpy(buf, bh->b_data + offset, segment);
        brelse(bh);
        buf += segment;
        buflen -= segment;
        pos += segment;
    }

    return 0;
}
#endif /* CONFIG_LWFS_ON_BLOCK */

/*
 * read data from the lwfs image
 */
int lwfs_dev_read(struct super_block *sb, unsigned long pos,
           void *buf, size_t buflen)
{
    size_t limit;

    limit = lwfs_maxsize(sb);
    if (pos >= limit)
        return -EIO;
    if (buflen > limit - pos)
        buflen = limit - pos;

#ifdef CONFIG_LWFS_ON_MTD
    if (sb->s_mtd)
        return lwfs_mtd_read(sb, pos, buf, buflen);
#endif
#ifdef CONFIG_LWFS_ON_BLOCK
    if (sb->s_bdev)
        return lwfs_blk_read(sb, pos, buf, buflen);
#endif
    return -EIO;
}
