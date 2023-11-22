/*
 * sstar-dwc3-debugfs.c- Sigmastar
 *
 * Copyright (c) [2019~2021] SigmaStar Technology.
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

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/ptrace.h>
#include <linux/types.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <io.h>
#include <xhci.h>
#include <hub.h>
#include <linux/platform_device.h>
#include <sstar-dwc3-of-simple.h>
#include <xhci.h>
static int sstar_dwc3_show(struct seq_file *s, void *unused)
{
    return 0;
}

static int sstar_dwc3_open(struct inode *inode, struct file *file)
{
    return single_open(file, sstar_dwc3_show, inode->i_private);
}

static ssize_t sstar_dwc3_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    struct seq_file *      s         = file->private_data;
    struct dwc3_of_simple *of_simple = s->private;
    struct usb_hcd *       hcd       = platform_get_drvdata(of_simple->dwc->xhci);
    struct xhci_hcd *      xhci      = hcd_to_xhci(hcd);
    struct xhci_hub *      rhub      = xhci_get_rhub(xhci->shared_hcd);
    int                    pipe_ctrl;

    char buf[32] = {0};
    u32  compl_enabled;
    int  ret;

    if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
        return -EFAULT;

    ret = kstrtouint(buf, 0, &compl_enabled);
    if (ret)
    {
        return ret;
    }

    xhci_set_link_state(xhci, rhub->ports[0], XDEV_COMP_MODE);
    pipe_ctrl = dwc3_readl(of_simple->dwc->regs, DWC3_GUSB3PIPECTL(0));
    printk("%s compl_enabled = %d, pipe_ctrl = 0x%x\r\n", __func__, compl_enabled, pipe_ctrl);
    pipe_ctrl &= (~BIT(30));
    pipe_ctrl |= (compl_enabled << 30);
    printk("%s pipe_ctrl = 0x%x\r\n", __func__, pipe_ctrl);
    dwc3_writel(of_simple->dwc->regs, DWC3_GUSB3PIPECTL(0), pipe_ctrl);

    return count;
}

static const struct file_operations sstar_dwc3_fops = {
    .open    = sstar_dwc3_open,
    .write   = sstar_dwc3_write,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

void sstar_dwc3_debugfs_exit(struct dwc3_of_simple *of_simple)
{
    debugfs_remove_recursive(of_simple->root);
}

void sstar_dwc3_debugfs_init(struct dwc3_of_simple *of_simple)
{
    of_simple->root = debugfs_create_dir(dev_name(of_simple->dev), usb_debug_root);
    debugfs_create_file("compliance-enabled", 0644, of_simple->root, of_simple, &sstar_dwc3_fops);
}
