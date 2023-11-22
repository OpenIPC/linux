/*
* mmap-nommu.c- Sigmastar
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

#include <linux/mm.h>
#include <linux/mtd/super.h>
#include "internal.h"

/*
 * try to determine where a shared mapping can be made
 * - only supported for NOMMU at the moment (MMU can't doesn't copy private
 *   mappings)
 * - attempts to map through to the underlying MTD device
 */
static unsigned long lwfs_get_unmapped_area(struct file *file,
                         unsigned long addr,
                         unsigned long len,
                         unsigned long pgoff,
                         unsigned long flags)
{
    struct inode *inode = file->f_mapping->host;
    struct mtd_info *mtd = inode->i_sb->s_mtd;
    unsigned long isize, offset, maxpages, lpages;
    int ret;

    if (!mtd)
        return (unsigned long) -ENOSYS;

    /* the mapping mustn't extend beyond the EOF */
    lpages = (len + PAGE_SIZE - 1) >> PAGE_SHIFT;
    isize = i_size_read(inode);
    offset = pgoff << PAGE_SHIFT;

    maxpages = (isize + PAGE_SIZE - 1) >> PAGE_SHIFT;
    if ((pgoff >= maxpages) || (maxpages - pgoff < lpages))
        return (unsigned long) -EINVAL;

    if (addr != 0)
        return (unsigned long) -EINVAL;

    if (len > mtd->size || pgoff >= (mtd->size >> PAGE_SHIFT))
        return (unsigned long) -EINVAL;

    offset += LWFS_I(inode)->i_dataoffset;
    if (offset >= mtd->size)
        return (unsigned long) -EINVAL;
    /* the mapping mustn't extend beyond the EOF */
    if ((offset + len) > mtd->size)
        len = mtd->size - offset;

    ret = mtd_get_unmapped_area(mtd, len, offset, flags);
    if (ret == -EOPNOTSUPP)
        ret = -ENOSYS;
    return (unsigned long) ret;
}

/*
 * permit a R/O mapping to be made directly through onto an MTD device if
 * possible
 */
static int lwfs_mmap(struct file *file, struct vm_area_struct *vma)
{
    return vma->vm_flags & (VM_SHARED | VM_MAYSHARE) ? 0 : -ENOSYS;
}

static unsigned lwfs_mmap_capabilities(struct file *file)
{
    struct mtd_info *mtd = file_inode(file)->i_sb->s_mtd;

    if (!mtd)
        return NOMMU_MAP_COPY;
    return mtd_mmap_capabilities(mtd);
}

const struct file_operations lwfs_ro_fops = {
    .llseek            = generic_file_llseek,
    .read_iter        = generic_file_read_iter,
    .splice_read        = generic_file_splice_read,
    .mmap            = lwfs_mmap,
    .get_unmapped_area    = lwfs_get_unmapped_area,
    .mmap_capabilities    = lwfs_mmap_capabilities,
};
