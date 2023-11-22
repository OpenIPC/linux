/*
 * phy_sstar_pipe_debugfs.c- Sigmastar
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
#include <linux/spinlock.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include "phy_sstar_u3phy.h"
#include <io.h>

static int sstar_phy_pipe_tx_biasi_show(struct seq_file *s, void *unused)
{
    struct sstar_phy_port *port       = s->private;
    void __iomem *         pipe_phya2 = port->reg + (0x200 * 3);
    unsigned long          flags;
    u32                    biasi_val;
    int                    bit_masks;

    spin_lock_irqsave(&port->lock, flags);
    biasi_val = INREG16(pipe_phya2 + (0x11 << 2));
    spin_unlock_irqrestore(&port->lock, flags);
    bit_masks = BIT(16) - BIT(12);
    seq_printf(s, "tx-biasi = 0x%x\r\n", (bit_masks & biasi_val) >> 12);
    return 0;
}

static int sstar_phy_pipe_tx_biasi_open(struct inode *inode, struct file *file)
{
    return single_open(file, sstar_phy_pipe_tx_biasi_show, inode->i_private);
}

static ssize_t sstar_phy_pipe_tx_biasi_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    struct seq_file *      s          = file->private_data;
    struct sstar_phy_port *port       = s->private;
    void __iomem *         pipe_phya2 = port->reg + (0x200 * 3);
    unsigned long          flags;
    char                   buf[32] = {0};
    u32                    biasi_val;
    int                    ret;
    int                    bit_masks;

    if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
        return -EFAULT;

    ret = kstrtouint(buf, 0, &biasi_val);
    if (ret)
        return ret;

    spin_lock_irqsave(&port->lock, flags);
    bit_masks = BIT(16) - BIT(12);
    CLRREG16(pipe_phya2 + (0x11 << 2), bit_masks);
    SETREG16(pipe_phya2 + (0x11 << 2), bit_masks & (biasi_val << 12));
    spin_unlock_irqrestore(&port->lock, flags);

    return count;
}

static const struct file_operations sstar_tx_biasi_fops = {
    .open    = sstar_phy_pipe_tx_biasi_open,
    .write   = sstar_phy_pipe_tx_biasi_write,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

static int sstar_phy_pipe_tx_idem_show(struct seq_file *s, void *unused)
{
    struct sstar_phy_port *port      = s->private;
    void __iomem *         pipe_phyd = port->reg;
    unsigned long          flags;
    u32                    idem_val;
    int                    bit_masks;

    spin_lock_irqsave(&port->lock, flags);
    idem_val = INREG16(pipe_phyd + (0x41 << 2));
    spin_unlock_irqrestore(&port->lock, flags);
    bit_masks = BIT(14) - BIT(8);
    seq_printf(s, "tx-idem = 0x%x\r\n", (bit_masks & idem_val) >> 8);
    return 0;
}

static int sstar_phy_pipe_tx_idem_open(struct inode *inode, struct file *file)
{
    return single_open(file, sstar_phy_pipe_tx_idem_show, inode->i_private);
}

static ssize_t sstar_phy_pipe_tx_idem_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    struct seq_file *      s         = file->private_data;
    struct sstar_phy_port *port      = s->private;
    void __iomem *         pipe_phyd = port->reg;
    unsigned long          flags;
    char                   buf[32] = {0};
    u32                    idem_val;
    int                    ret;
    int                    bit_masks;

    if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
        return -EFAULT;

    ret = kstrtouint(buf, 0, &idem_val);
    if (ret)
        return ret;

    spin_lock_irqsave(&port->lock, flags);
    bit_masks = BIT(14) - BIT(8);
    CLRREG16(pipe_phyd + (0x41 << 2), bit_masks);
    SETREG16(pipe_phyd + (0x41 << 2), bit_masks & (idem_val << 8));
    spin_unlock_irqrestore(&port->lock, flags);

    return count;
}

static const struct file_operations sstar_tx_idem_fops = {
    .open    = sstar_phy_pipe_tx_idem_open,
    .write   = sstar_phy_pipe_tx_idem_write,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

static int sstar_phy_pipe_tx_idrv_show(struct seq_file *s, void *unused)
{
    struct sstar_phy_port *port      = s->private;
    void __iomem *         pipe_phyd = port->reg;
    unsigned long          flags;
    u32                    idrv_val;
    int                    bit_masks;

    spin_lock_irqsave(&port->lock, flags);
    idrv_val = INREG16(pipe_phyd + (0x41 << 2));
    spin_unlock_irqrestore(&port->lock, flags);
    bit_masks = BIT(8) - BIT(2);
    seq_printf(s, "tx-idrv = 0x%x\r\n", (bit_masks & idrv_val) >> 2);
    return 0;
}

static int sstar_phy_pipe_tx_idrv_open(struct inode *inode, struct file *file)
{
    return single_open(file, sstar_phy_pipe_tx_idrv_show, inode->i_private);
}

static ssize_t sstar_phy_pipe_tx_idrv_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    struct seq_file *      s         = file->private_data;
    struct sstar_phy_port *port      = s->private;
    void __iomem *         pipe_phyd = port->reg;
    unsigned long          flags;
    char                   buf[32] = {0};
    u32                    idrv_val;
    int                    ret;
    int                    bit_masks;

    if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
        return -EFAULT;

    ret = kstrtouint(buf, 0, &idrv_val);
    if (ret)
        return ret;

    spin_lock_irqsave(&port->lock, flags);
    bit_masks = BIT(8) - BIT(2);
    CLRREG16(pipe_phyd + (0x41 << 2), bit_masks);
    SETREG16(pipe_phyd + (0x41 << 2), bit_masks & (idrv_val << 2));
    spin_unlock_irqrestore(&port->lock, flags);

    return count;
}

static const struct file_operations sstar_tx_idrv_fops = {
    .open    = sstar_phy_pipe_tx_idrv_open,
    .write   = sstar_phy_pipe_tx_idrv_write,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

void sstar_phy_pipe_debugfs_init(struct sstar_phy_port *port)
{
    spin_lock_init(&port->lock);
    port->root = debugfs_create_dir(dev_name(port->dev), usb_debug_root);

    debugfs_create_file("tx-idrv", 0644, port->root, port, &sstar_tx_idrv_fops);
    debugfs_create_file("tx-idem", 0644, port->root, port, &sstar_tx_idem_fops);
    debugfs_create_file("tx-biasi", 0644, port->root, port, &sstar_tx_biasi_fops);
}

void sstar_phy_pipe_debugfs_exit(struct sstar_phy_port *port)
{
    debugfs_remove_recursive(port->root);
}
