/*
* super.c- Sigmastar
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

#include <linux/module.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/parser.h>
#include <linux/mount.h>
#include <linux/namei.h>
#include <linux/statfs.h>
#include <linux/mtd/super.h>
#include <linux/ctype.h>
#include <linux/highmem.h>
#include <linux/pagemap.h>
#include <linux/uaccess.h>
#include <linux/major.h>
#include "internal.h"

static struct kmem_cache *lwfs_inode_cachep;

static struct inode *lwfs_iget(struct super_block *sb, unsigned long pos);

/*
 * read a page worth of data from the image
 */
static int lwfs_readpage(struct file *file, struct page *page)
{
    struct inode *inode = page->mapping->host;
    loff_t offset, size;
    unsigned long fillsize, pos;
    void *buf;
    int ret;

    buf = kmap(page);
    if (!buf)
        return -ENOMEM;

    /* 32 bit warning -- but not for us :) */
    offset = page_offset(page);
    size = i_size_read(inode);
    fillsize = 0;
    ret = 0;
    if (offset < size) {
        size -= offset;
        fillsize = size > PAGE_SIZE ? PAGE_SIZE : size;

        pos = LWFS_I(inode)->i_dataoffset + offset;

        ret = lwfs_dev_read(inode->i_sb, pos, buf, fillsize);
        if (ret < 0) {
            SetPageError(page);
            fillsize = 0;
            ret = -EIO;
        }
    }

    if (fillsize < PAGE_SIZE)
        memset(buf + fillsize, 0, PAGE_SIZE - fillsize);
    if (ret == 0)
        SetPageUptodate(page);

    flush_dcache_page(page);
    kunmap(page);
    unlock_page(page);
    return ret;
}

static const struct address_space_operations lwfs_aops = {
    .readpage    = lwfs_readpage
};

/*
 * read the entries from a directory
 */
static int lwfs_readdir(struct file *file, struct dir_context *ctx)
{
    struct inode *i = file_inode(file);
    unsigned long offset, maxoff;
    int ino;
    char fsname[LWFS_MAXFN+1] = {0};    /* XXX dynamic? */
    LwFsPartitionInfo_t p_header;
    LwFsFileInfo_t f_header;
    int ret;

    /* Read super block to get the last one file header position and assign to maxoff */
    ret = lwfs_dev_read(i->i_sb, 0, &p_header, LWFS_PH_SIZE);
    if (ret < 0)
        goto out;
    maxoff = le32_to_cpu(p_header.size);

    if (!ctx->pos)
        offset = LWFS_PH_SIZE;
    else
        offset = ctx->pos;

    /* Not really failsafe, but we are read-only... */
    for (;;) {
        if (!offset || offset >= maxoff) {
            offset = maxoff;
            ctx->pos = offset;
            goto out;
        }
        ctx->pos = offset;

        /* Fetch inode info */
        ret = lwfs_dev_read(i->i_sb, offset, &f_header, LWFS_FH_SIZE);
        if (ret < 0)
            goto out;

        memset(fsname, 0, sizeof(fsname));
        memcpy(fsname, f_header.name, LWFS_MAXFN);

        ino = offset;

        if (!dir_emit(ctx, fsname, strlen(fsname), ino, DT_REG))
            goto out;

        offset += LWFS_FH_SIZE;
    }
out:
    return 0;
}

/*
 * look up an entry in a directory
 */
static struct dentry *lwfs_lookup(struct inode *dir, struct dentry *dentry,
                   unsigned int flags)
{
    unsigned long offset, maxoff;
    struct inode *inode;
    const char *name;        /* got from dentry */
    LwFsPartitionInfo_t p_header;
    LwFsFileInfo_t f_header;
    int len, ret;

    /* search all the file entries in the list starting from the one
     * pointed to by the directory's special data */
    ret = lwfs_dev_read(dir->i_sb, dir->i_ino, &p_header, LWFS_PH_SIZE);
    if (ret < 0)
        goto error;

    maxoff = le32_to_cpu(p_header.size);
    offset = dir->i_ino + LWFS_PH_SIZE;

    name = dentry->d_name.name;
    len = dentry->d_name.len;

    for (;;) {
        if (!offset || offset >= maxoff)
            goto out0;

        ret = lwfs_dev_read(dir->i_sb, offset, &f_header, LWFS_FH_SIZE);
        if (ret < 0)
            goto error;

        /* try to match file name */
        if (strncmp(f_header.name, dentry->d_name.name, dentry->d_name.len) == 0)
            break;  // match!

        offset += LWFS_FH_SIZE;
    }

    inode = lwfs_iget(dir->i_sb, offset);
    if (IS_ERR(inode)) {
        ret = PTR_ERR(inode);
        goto error;
    }
    goto outi;

    /*
     * it's a bit funky, _lookup needs to return an error code
     * (negative) or a NULL, both as a dentry.  ENOENT should not
     * be returned, instead we need to create a negative dentry by
     * d_add(dentry, NULL); and return 0 as no error.
     * (Although as I see, it only matters on writable file
     * systems).
     */
out0:
    inode = NULL;
outi:
    d_add(dentry, inode);
    ret = 0;
error:
    return ERR_PTR(ret);
}

static const struct file_operations lwfs_dir_operations = {
    .read        = generic_read_dir,
    .iterate_shared    = lwfs_readdir,
    .llseek        = generic_file_llseek,
};

static const struct inode_operations lwfs_dir_inode_operations = {
    .lookup        = lwfs_lookup,
};

static struct inode *lwfs_prepare_root_dir(struct super_block *sb, unsigned long pos, unsigned long size)
{
    struct inode *i;

    /* get an inode for this image position */
    i = iget_locked(sb, pos);
    if (!i)
        return ERR_PTR(-ENOMEM);

    if (!(i->i_state & I_NEW))
        return i;

    set_nlink(i, 1);        /* Hard to decide.. */
    i->i_size = size;
    i->i_mtime.tv_sec = i->i_atime.tv_sec = i->i_ctime.tv_sec = 0;
    i->i_mtime.tv_nsec = i->i_atime.tv_nsec = i->i_ctime.tv_nsec = 0;

    /* set up mode and ops */
    i->i_op = &lwfs_dir_inode_operations;
    i->i_fop = &lwfs_dir_operations;
    i->i_mode = S_IFDIR | 0644 | S_IXUGO;

    unlock_new_inode(i);
    return i;
}

/*
 * get a lwfs inode based on its position in the image (which doubles as the
 * inode number)
 */
static struct inode *lwfs_iget(struct super_block *sb, unsigned long pos)
{
    struct lwfs_inode_info *inode;
    struct inode *i;
    int ret;
    LwFsFileInfo_t f_header;

    ret = lwfs_dev_read(sb, pos, &f_header, LWFS_FH_SIZE);
    if (ret < 0)
        goto error;

    /* get an inode for this image position */
    i = iget_locked(sb, pos);
    if (!i)
        return ERR_PTR(-ENOMEM);

    if (!(i->i_state & I_NEW))
        return i;

    /* precalculate the data offset */
    inode = LWFS_I(i);
    inode->i_metasize = LWFS_FH_SIZE;
    inode->i_dataoffset = le32_to_cpu(f_header.offset);

    set_nlink(i, 1);        /* Hard to decide.. */
    i->i_size = le32_to_cpu(f_header.length);
    i->i_mtime.tv_sec = i->i_atime.tv_sec = i->i_ctime.tv_sec = 0;
    i->i_mtime.tv_nsec = i->i_atime.tv_nsec = i->i_ctime.tv_nsec = 0;
    i->i_fop = &lwfs_ro_fops;
    i->i_data.a_ops = &lwfs_aops;
    i->i_mode = S_IFREG | S_IXUGO;

    unlock_new_inode(i);
    return i;

error:
    pr_err("read error for inode 0x%lx\n", pos);
    return ERR_PTR(ret);
}

/*
 * allocate a new inode
 */
static struct inode *lwfs_alloc_inode(struct super_block *sb)
{
    struct lwfs_inode_info *inode;

    inode = kmem_cache_alloc(lwfs_inode_cachep, GFP_KERNEL);
    return inode ? &inode->vfs_inode : NULL;
}

/*
 * return a spent inode to the slab cache
 */
static void lwfs_i_callback(struct rcu_head *head)
{
    struct inode *inode = container_of(head, struct inode, i_rcu);

    kmem_cache_free(lwfs_inode_cachep, LWFS_I(inode));
}

static void lwfs_destroy_inode(struct inode *inode)
{
    call_rcu(&inode->i_rcu, lwfs_i_callback);
}

/*
 * get filesystem statistics
 */
static int lwfs_statfs(struct dentry *dentry, struct kstatfs *buf)
{
    struct super_block *sb = dentry->d_sb;
    u64 id = 0;

    /* When calling huge_encode_dev(),
     * use sb->s_bdev->bd_dev when,
     *   - CONFIG_ROMFS_ON_BLOCK defined
     * use sb->s_dev when,
     *   - CONFIG_ROMFS_ON_BLOCK undefined and
     *   - CONFIG_ROMFS_ON_MTD defined
     * leave id as 0 when,
     *   - CONFIG_ROMFS_ON_BLOCK undefined and
     *   - CONFIG_ROMFS_ON_MTD undefined
     */
    if (sb->s_bdev)
        id = huge_encode_dev(sb->s_bdev->bd_dev);
    else if (sb->s_dev)
        id = huge_encode_dev(sb->s_dev);

    buf->f_type = LWFS_MAGIC;
    buf->f_namelen = LWFS_MAXFN;
    buf->f_bsize = LWBSIZE;
    buf->f_bfree = buf->f_bavail = buf->f_ffree;
    buf->f_blocks =
        (lwfs_maxsize(dentry->d_sb) + LWBSIZE - 1) >> LWBSBITS;
    buf->f_fsid.val[0] = (u32)id;
    buf->f_fsid.val[1] = (u32)(id >> 32);
    return 0;
}

/*
 * remounting must involve read-only
 */
static int lwfs_remount(struct super_block *sb, int *flags, char *data)
{
    sync_filesystem(sb);
    *flags |= MS_RDONLY;
    return 0;
}

static const struct super_operations lwfs_super_ops = {
    .alloc_inode    = lwfs_alloc_inode,
    .destroy_inode    = lwfs_destroy_inode,
    .statfs        = lwfs_statfs,
    .remount_fs    = lwfs_remount,
};

/*
 * fill in the superblock
 */
static int lwfs_fill_super(struct super_block *sb, void *data, int silent)
{
    LwFsPartitionInfo_t p_header;
    LwFsFileInfo_t f_header;
    struct inode *root;
    unsigned long img_size;
    const char *storage;
    int ret;

#ifdef CONFIG_BLOCK
    if (!sb->s_mtd) {
        sb_set_blocksize(sb, LWBSIZE);
    } else {
        sb->s_blocksize = LWBSIZE;
        sb->s_blocksize_bits = blksize_bits(LWBSIZE);
    }
#endif

    sb->s_maxbytes = 0xFFFFFFFF;
    sb->s_magic = LWFS_MAGIC;
    sb->s_flags |= MS_RDONLY | MS_NOATIME;
    sb->s_op = &lwfs_super_ops;

#ifdef CONFIG_LWFS_ON_MTD
    /* Use same dev ID from the underlying mtdblock device */
    if (sb->s_mtd)
        sb->s_dev = MKDEV(MTD_BLOCK_MAJOR, sb->s_mtd->index);
#endif

    /* get partition header */
    sb->s_fs_info = (void *)LWFS_PH_SIZE;
    ret = lwfs_dev_read(sb, 0, &p_header, LWFS_PH_SIZE);
    if (ret < 0)
        goto error_rsb;

    /* get latest file header */
    sb->s_fs_info = (void *)le32_to_cpu(p_header.size);
    ret = lwfs_dev_read(sb, le32_to_cpu(p_header.size) - LWFS_FH_SIZE, &f_header, LWFS_FH_SIZE);
    if (ret < 0)
        goto error_rsb;

    img_size = le32_to_cpu(f_header.offset) + le32_to_cpu(f_header.length) + f_header.padding;

    if (sb->s_mtd && img_size > sb->s_mtd->size)
        goto error_rsb_inval;

    sb->s_fs_info = (void *) img_size;

    if (le32_to_cpu(p_header.magic) != LWFS_MAGIC)
    {
        if (!silent)
            pr_warn("VFS: Can't find a lwfs filesystem on dev %s.\n",
                   sb->s_id);
        goto error_rsb_inval;
    }

    storage = sb->s_mtd ? "MTD" : "the block layer";

    /* prepare root directory */
    root = lwfs_prepare_root_dir(sb, 0, img_size-LWFS_PH_SIZE);
    if (IS_ERR(root))
        return PTR_ERR(root);

    sb->s_root = d_make_root(root);
    if (!sb->s_root)
        return -ENOMEM;

    return 0;

error_rsb_inval:
    ret = -EINVAL;
error_rsb:
    return ret;
}

/*
 * get a superblock for mounting
 */
static struct dentry *lwfs_mount(struct file_system_type *fs_type,
            int flags, const char *dev_name,
            void *data)
{
    struct dentry *ret = ERR_PTR(-EINVAL);

#ifdef CONFIG_LWFS_ON_MTD
    ret = mount_mtd(fs_type, flags, dev_name, data, lwfs_fill_super);
#endif
#ifdef CONFIG_LWFS_ON_BLOCK
    if (ret == ERR_PTR(-EINVAL))
        ret = mount_bdev(fs_type, flags, dev_name, data,
                  lwfs_fill_super);
#endif
    return ret;
}

/*
 * destroy a lwfs superblock in the appropriate manner
 */
static void lwfs_kill_sb(struct super_block *sb)
{
#ifdef CONFIG_LWFS_ON_MTD
    if (sb->s_mtd) {
        kill_mtd_super(sb);
        return;
    }
#endif
#ifdef CONFIG_LWFS_ON_BLOCK
    if (sb->s_bdev) {
        kill_block_super(sb);
        return;
    }
#endif
}

static struct file_system_type lwfs_fs_type = {
    .owner        = THIS_MODULE,
    .name        = "lwfs",
    .mount        = lwfs_mount,
    .kill_sb    = lwfs_kill_sb,
    .fs_flags    = FS_REQUIRES_DEV,
};
MODULE_ALIAS_FS("lwfs");

/*
 * inode storage initialiser
 */
static void lwfs_i_init_once(void *_inode)
{
    struct lwfs_inode_info *inode = _inode;

    inode_init_once(&inode->vfs_inode);
}

/*
 * lwfs module initialisation
 */
static int __init init_lwfs_fs(void)
{
    int ret;

    pr_info("SStar LWFS - Lightweight Filesystem)\n");

    lwfs_inode_cachep =
        kmem_cache_create("lwfs_i",
                  sizeof(struct lwfs_inode_info), 0,
                  SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD |
                  SLAB_ACCOUNT, lwfs_i_init_once);

    if (!lwfs_inode_cachep) {
        pr_err("Failed to initialise inode cache\n");
        return -ENOMEM;
    }
    ret = register_filesystem(&lwfs_fs_type);
    if (ret) {
        pr_err("Failed to register filesystem\n");
        goto error_register;
    }
    return 0;

error_register:
    kmem_cache_destroy(lwfs_inode_cachep);
    return ret;
}

/*
 * lwfs module removal
 */
static void __exit exit_lwfs_fs(void)
{
    unregister_filesystem(&lwfs_fs_type);
    /*
     * Make sure all delayed rcu free inodes are flushed before we
     * destroy cache.
     */
    rcu_barrier();
    kmem_cache_destroy(lwfs_inode_cachep);
}

module_init(init_lwfs_fs);
module_exit(exit_lwfs_fs);

MODULE_DESCRIPTION("Direct-MTD Capable LwFS");
MODULE_AUTHOR("SStar");
MODULE_LICENSE("GPL"); /* Actually dual-licensed, but it doesn't matter for */
