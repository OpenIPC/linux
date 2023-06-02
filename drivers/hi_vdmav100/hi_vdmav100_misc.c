/*
 * Copyright (c) 2015 HiSilicon Technologies Co., Ltd.
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <asm/pgtable.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <asm/page.h>
#include <linux/spinlock.h>
#include <linux/kernel.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm_types.h>
#include <linux/mmu_context.h>
#include <asm/tlbflush.h>
#include <linux/uaccess.h>

#include "hi_vdma.h"

#define hidmac_error(s...) do { \
    pr_err("hidmac:%s:%d: ", __func__, __LINE__); \
    printk(s); \
    printk("\n"); \
} while (0)


static long hi_vdma_ioctl(struct file *filep, unsigned int cmd,
                          unsigned long arg)
{
    long ret = 0;
    struct dmac_user_para para;

    if (copy_from_user((void *)&para, (void *)arg, sizeof(para))) {
        return -EINVAL;
    }

    switch (cmd) {
        case VDMA_DATA_CMD:
            ret = hi_memcpy((void *)para.dst, (void *)para.src, para.size);
            break;
        default:
            hidmac_error("unknown command :%x\n", cmd);
            ret = -1;
            break;
    }

    return ret;
}

static int hi_vdma_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int hi_vdma_release(struct inode *inode, struct file *file)
{
    return 0;
}

static const struct file_operations hi_vdma_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = hi_vdma_ioctl,
    .open = hi_vdma_open,
    .release = hi_vdma_release,
};

static struct miscdevice hi_vdma_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "hi_vdma",
    .fops = &hi_vdma_fops,
};

static int __init hi_vdma_init(void)
{
    int ret;

    ret = misc_register(&hi_vdma_misc_device);

    return ret;
}

static void __exit hi_vdma_exit(void)
{
    misc_deregister(&hi_vdma_misc_device);
}

module_init(hi_vdma_init);
module_exit(hi_vdma_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Hisilicon VDMA MISC Driver");
