/*
 * phy_sstar_utmi_debugfs.c- Sigmastar
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
#include <linux/bitfield.h>
#include <linux/slab.h>
#include <linux/ptrace.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/of_address.h>
#include "phy_sstar_u3phy.h"
#include <io.h>

static int sstar_phy_utmi_cm_cur_show(struct seq_file *s, void *unused)
{
    struct sstar_phy_port *port = s->private;
    void __iomem *         base = port->reg;
    unsigned long          flags;
    u32                    cm_cur;
    int                    bit_masks;

    spin_lock_irqsave(&port->lock, flags);
    cm_cur = INREG16(base + (0x17 << 2));
    spin_unlock_irqrestore(&port->lock, flags);
    seq_printf(s, "cm_current = 0x%04x\r\n", cm_cur);
    bit_masks = BIT(3) | BIT(4) | BIT(5);
    cm_cur    = (bit_masks & cm_cur) >> 3;
    switch (cm_cur)
    {
        case 0x03:
            cm_cur = 105;
            break;
        case 0x02:
            cm_cur = 110;
            break;
        case 0x01:
        case 0x07:
            cm_cur = 115;
            break;
        case 0x00:
        case 0x06:
            cm_cur = 120;
            break;
        case 0x05:
            cm_cur = 125;
            break;
        case 0x04:
            cm_cur = 130;
            break;
        default:
            cm_cur = 105;
    }
    seq_printf(s, "cm_current: %d%%\r\n", cm_cur);
    return 0;
}

static int sstar_phy_utmi_cm_cur_open(struct inode *inode, struct file *file)
{
    return single_open(file, sstar_phy_utmi_cm_cur_show, inode->i_private);
}

static ssize_t sstar_phy_utmi_cm_cur_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    struct seq_file *      s    = file->private_data;
    struct sstar_phy_port *port = s->private;
    void __iomem *         base = port->reg;
    unsigned long          flags;
    char                   buf[32] = {0};
    u32                    cm_cur;
    int                    ret;
    int                    bit_masks;

    if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
        return -EFAULT;

    ret = kstrtouint(buf, 0, &cm_cur);
    if (ret)
    {
        return ret;
    }

    spin_lock_irqsave(&port->lock, flags);
    bit_masks = BIT(3) | BIT(4) | BIT(5);
    CLRREG16(base + (0x17 << 2), bit_masks);

    switch (cm_cur)
    {
        case 105:
            cm_cur = 0x03;
            break;
        case 110:
            cm_cur = 0x02;
            break;
        case 115:
            cm_cur = 0x01;
            break;
        case 120:
            cm_cur = 0x00;
            break;
        case 125:
            cm_cur = 0x05;
            break;
        case 130:
            cm_cur = 0x04;
            break;
        default:
            cm_cur = 0x03;
    }

    SETREG16(base + (0x17 << 2), bit_masks & (cm_cur << 3));
    spin_unlock_irqrestore(&port->lock, flags);

    return count;
}

static const struct file_operations sstar_cm_cur_fops = {
    .open    = sstar_phy_utmi_cm_cur_open,
    .write   = sstar_phy_utmi_cm_cur_write,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

static int sstar_phy_utmi_dem_cur_show(struct seq_file *s, void *unused)
{
    struct sstar_phy_port *port = s->private;
    void __iomem *         base = port->reg;
    unsigned long          flags;
    u32                    dem_cur;
    int                    bit_masks;

    spin_lock_irqsave(&port->lock, flags);
    dem_cur = INREG16(base + (0x16 << 2));
    spin_unlock_irqrestore(&port->lock, flags);
    seq_printf(s, "de_emphasis_current = 0x%04x\r\n", dem_cur);
    bit_masks = (BIT(7) | BIT(8) | BIT(9));
    dem_cur   = (bit_masks & dem_cur) >> 7;
    switch (dem_cur)
    {
        case 0x04:
            dem_cur = 105;
            break;
        case 0x05:
            dem_cur = 110;
            break;
        case 0x06:
            dem_cur = 115;
            break;
        case 0x07:
            dem_cur = 120;
            break;
        default:
            dem_cur = 100;
    }
    seq_printf(s, "de_emphasis_current: %d%%\r\n", dem_cur);
    return 0;
}

static int sstar_phy_utmi_dem_cur_open(struct inode *inode, struct file *file)
{
    return single_open(file, sstar_phy_utmi_dem_cur_show, inode->i_private);
}

static ssize_t sstar_phy_utmi_dem_cur_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    struct seq_file *      s    = file->private_data;
    struct sstar_phy_port *port = s->private;
    void __iomem *         base = port->reg;
    unsigned long          flags;
    char                   buf[32] = {0};
    u32                    dem_cur;
    int                    ret;
    int                    bit_masks;

    if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
        return -EFAULT;

    ret = kstrtouint(buf, 0, &dem_cur);
    if (ret)
    {
        return ret;
    }

    spin_lock_irqsave(&port->lock, flags);
    bit_masks = (BIT(7) | BIT(8) | BIT(9));
    CLRREG16(base + (0x16 << 2), bit_masks);
    switch (dem_cur)
    {
        case 100:
            dem_cur = 0x00;
            break;
        case 105:
            dem_cur = 0x04;
            break;
        case 110:
            dem_cur = 0x05;
            break;
        case 115:
            dem_cur = 0x06;
            break;
        case 120:
            dem_cur = 0x07;
            break;
        default:
            dem_cur = 0x11;
    }
    SETREG16(base + (0x16 << 2), bit_masks & (dem_cur << 7));
    spin_unlock_irqrestore(&port->lock, flags);

    return count;
}

static const struct file_operations sstar_dem_cur_fops = {
    .open    = sstar_phy_utmi_dem_cur_open,
    .write   = sstar_phy_utmi_dem_cur_write,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

static int sstar_phy_utmi_tx_swing_show(struct seq_file *s, void *unused)
{
    struct sstar_phy_port *port = s->private;
    void __iomem *         base = port->reg;
    unsigned long          flags;
    u32                    tx_swing;
    int                    bit_masks;

    spin_lock_irqsave(&port->lock, flags);
    tx_swing = INREG16(base + (0x16 << 2));
    spin_unlock_irqrestore(&port->lock, flags);
    seq_printf(s, "tx_swing = 0x%04x\r\n", tx_swing);
    bit_masks = (BIT(4) | BIT(5) | BIT(6));
    tx_swing  = (bit_masks & tx_swing) >> 4;

    switch (tx_swing)
    {
        case 0x04:
            tx_swing = 80;
            break;
        case 0x05:
            tx_swing = 85;
            break;
        case 0x06:
            tx_swing = 90;
            break;
        case 0x07:
            tx_swing = 95;
            break;
        case 0x00:
            tx_swing = 100;
            break;
        case 0x01:
            tx_swing = 105;
            break;
        case 0x02:
            tx_swing = 110;
            break;
        case 0x03:
            tx_swing = 115;
            break;
        default:
            tx_swing = 110;
    }

    seq_printf(s, "tx_swing: %d%%\r\n", tx_swing);
    return 0;
}

static int sstar_phy_utmi_tx_swing_open(struct inode *inode, struct file *file)
{
    return single_open(file, sstar_phy_utmi_tx_swing_show, inode->i_private);
}

static ssize_t sstar_phy_utmi_tx_swing_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    struct seq_file *      s    = file->private_data;
    struct sstar_phy_port *port = s->private;
    void __iomem *         base = port->reg;
    unsigned long          flags;
    char                   buf[32] = {0};
    u32                    tx_swing;
    int                    ret;
    int                    bit_masks;

    if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
        return -EFAULT;

    ret = kstrtouint(buf, 0, &tx_swing);
    if (ret)
    {
        return ret;
    }

    switch (tx_swing)
    {
        case 80:
            tx_swing = 0x04;
            break;
        case 85:
            tx_swing = 0x05;
            break;
        case 90:
            tx_swing = 0x06;
            break;
        case 95:
            tx_swing = 0x07;
            break;
        case 100:
            tx_swing = 0x00;
            break;
        case 105:
            tx_swing = 0x01;
            break;
        case 110:
            tx_swing = 0x02;
            break;
        case 115:
            tx_swing = 0x03;
            break;
        default:
            tx_swing = 0x02;
    }

    spin_lock_irqsave(&port->lock, flags);
    bit_masks = (BIT(4) | BIT(5) | BIT(6));
    CLRREG16(base + (0x16 << 2), bit_masks);
    SETREG16(base + (0x16 << 2), bit_masks & (tx_swing << 4));
    spin_unlock_irqrestore(&port->lock, flags);

    return count;
}

static const struct file_operations sstar_tx_swing_fops = {
    .open    = sstar_phy_utmi_tx_swing_open,
    .write   = sstar_phy_utmi_tx_swing_write,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

static int sstar_phy_utmi_ls_cross_rise_show(struct seq_file *s, void *unused)
{
    struct sstar_phy_port *port = s->private;
    void __iomem *         base = port->reg;
    unsigned long          flags;
    u32                    ls_cross_level;

    spin_lock_irqsave(&port->lock, flags);
    ls_cross_level = INREG16(base + (0x49 << 2));
    spin_unlock_irqrestore(&port->lock, flags);
    seq_printf(s, "ls_cross_rise(0x00~0x7f) = 0x%lx\r\n", FIELD_GET(GENMASK(14, 8), ls_cross_level));

    return 0;
}

static int sstar_phy_utmi_ls_cross_rise_open(struct inode *inode, struct file *file)
{
    return single_open(file, sstar_phy_utmi_ls_cross_rise_show, inode->i_private);
}

static ssize_t sstar_phy_utmi_ls_cross_rise_write(struct file *file, const char __user *ubuf, size_t count,
                                                  loff_t *ppos)
{
    struct seq_file *      s    = file->private_data;
    struct sstar_phy_port *port = s->private;
    void __iomem *         base = port->reg;
    unsigned long          flags;
    char                   buf[32] = {0};
    u32                    ls_cross_level;
    int                    ret;

    if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
        return -EFAULT;

    ret = kstrtouint(buf, 0, &ls_cross_level);
    if (ret)
    {
        return ret;
    }

    spin_lock_irqsave(&port->lock, flags);
    CLRSETREG16(base + (0x49 << 2), GENMASK(14, 8), FIELD_PREP(GENMASK(14, 8), ls_cross_level));
    spin_unlock_irqrestore(&port->lock, flags);

    return count;
}

static const struct file_operations sstar_ls_cross_rise_fops = {
    .open    = sstar_phy_utmi_ls_cross_rise_open,
    .write   = sstar_phy_utmi_ls_cross_rise_write,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

static int sstar_phy_utmi_ls_cross_fall_show(struct seq_file *s, void *unused)
{
    struct sstar_phy_port *port = s->private;
    void __iomem *         base = port->reg;
    unsigned long          flags;
    u32                    ls_cross_level;

    spin_lock_irqsave(&port->lock, flags);
    ls_cross_level = INREG16(base + (0x49 << 2));
    spin_unlock_irqrestore(&port->lock, flags);
    seq_printf(s, "ls_cross_fall(0x00~0x7f) = 0x%lx\r\n", FIELD_GET(GENMASK(6, 0), ls_cross_level));

    return 0;
}

static int sstar_phy_utmi_ls_cross_fall_open(struct inode *inode, struct file *file)
{
    return single_open(file, sstar_phy_utmi_ls_cross_fall_show, inode->i_private);
}

static ssize_t sstar_phy_utmi_ls_cross_fall_write(struct file *file, const char __user *ubuf, size_t count,
                                                  loff_t *ppos)
{
    struct seq_file *      s    = file->private_data;
    struct sstar_phy_port *port = s->private;
    void __iomem *         base = port->reg;
    unsigned long          flags;
    char                   buf[32] = {0};
    u32                    ls_cross_level;
    int                    ret;

    if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
        return -EFAULT;

    ret = kstrtouint(buf, 0, &ls_cross_level);
    if (ret)
    {
        return ret;
    }

    spin_lock_irqsave(&port->lock, flags);
    CLRSETREG16(base + (0x49 << 2), GENMASK(6, 0), FIELD_PREP(GENMASK(6, 0), ls_cross_level));
    spin_unlock_irqrestore(&port->lock, flags);

    return count;
}

static const struct file_operations sstar_ls_cross_fall_fops = {
    .open    = sstar_phy_utmi_ls_cross_fall_open,
    .write   = sstar_phy_utmi_ls_cross_fall_write,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

static int sstar_phy_utmi_pre_emphasis_show(struct seq_file *s, void *unused)
{
    struct sstar_phy_port *port = s->private;
    void __iomem *         base = port->reg;
    unsigned long          flags;
    u32                    val;

    spin_lock_irqsave(&port->lock, flags);
    val = INREG16(base + (0x43 << 2));
    spin_unlock_irqrestore(&port->lock, flags);

    seq_printf(s, "pre_emphasis(0~3): 0x%lx\r\n", FIELD_GET(GENMASK(9, 8), val));
    return 0;
}

static int sstar_phy_utmi_pre_emphasis_open(struct inode *inode, struct file *file)
{
    return single_open(file, sstar_phy_utmi_pre_emphasis_show, inode->i_private);
}

static ssize_t sstar_phy_utmi_pre_emphasis_set(struct sstar_phy_port *port, unsigned int val)
{
    void __iomem *base = port->reg;
    unsigned long flags;

    spin_lock_irqsave(&port->lock, flags);
    CLRSETREG16(base + (0x43 << 2), GENMASK(9, 8), FIELD_PREP(GENMASK(9, 8), val));
    SETREG16(base + (0x43 << 2), BIT(10));
    spin_unlock_irqrestore(&port->lock, flags);
    return 0;
}

static ssize_t sstar_phy_utmi_pre_emphasis_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    struct seq_file *      s       = file->private_data;
    struct sstar_phy_port *port    = s->private;
    char                   buf[32] = {0};
    u32                    val;
    int                    ret;

    if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
        return -EFAULT;

    ret = kstrtouint(buf, 0, &val);
    if (ret)
    {
        return ret;
    }

    ret = sstar_phy_utmi_pre_emphasis_set(port, val);
    if (ret)
    {
        return ret;
    }

    return count;
}

static const struct file_operations sstar_pre_emphasis_fops = {
    .open    = sstar_phy_utmi_pre_emphasis_open,
    .write   = sstar_phy_utmi_pre_emphasis_write,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

static int sstar_phy_utmi_slew_rate_show(struct seq_file *s, void *unused)
{
    struct sstar_phy_port *port = s->private;
    void __iomem *         base = port->reg;
    unsigned long          flags;
    u32                    val;

    spin_lock_irqsave(&port->lock, flags);
    val = INREG16(base + (0x44 << 2));
    spin_unlock_irqrestore(&port->lock, flags);

    seq_printf(s, "slew_rate(0~3): 0x%lx\r\n", FIELD_GET(GENMASK(2, 1), val));
    return 0;
}

static int sstar_phy_utmi_slew_rate_open(struct inode *inode, struct file *file)
{
    return single_open(file, sstar_phy_utmi_slew_rate_show, inode->i_private);
}

static ssize_t sstar_phy_utmi_slew_rate_set(struct sstar_phy_port *port, unsigned int val)
{
    void __iomem *base = port->reg;
    unsigned long flags;

    spin_lock_irqsave(&port->lock, flags);
    CLRSETREG16(base + (0x44 << 2), GENMASK(2, 1), FIELD_PREP(GENMASK(2, 1), val));
    SETREG16(base + (0x44 << 2), BIT(3));
    spin_unlock_irqrestore(&port->lock, flags);
    return 0;
}

static ssize_t sstar_phy_utmi_slew_rate_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    struct seq_file *      s       = file->private_data;
    struct sstar_phy_port *port    = s->private;
    char                   buf[32] = {0};
    u32                    val;
    int                    ret;

    if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
        return -EFAULT;

    ret = kstrtouint(buf, 0, &val);
    if (ret)
    {
        return ret;
    }

    ret = sstar_phy_utmi_slew_rate_set(port, val);
    if (ret)
    {
        return ret;
    }

    return count;
}

static const struct file_operations sstar_slew_rate_fops = {
    .open    = sstar_phy_utmi_slew_rate_open,
    .write   = sstar_phy_utmi_slew_rate_write,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

static int sstar_phy_utmi_swing_trim_show(struct seq_file *s, void *unused)
{
    struct sstar_phy_port *port = s->private;
    void __iomem *         base = port->reg;
    unsigned long          flags;
    u32                    val;

    spin_lock_irqsave(&port->lock, flags);
    val = INREG16(base + (0x44 << 2));
    spin_unlock_irqrestore(&port->lock, flags);

    seq_printf(s, "swing_trim(0~63): 0x%lx\r\n", FIELD_GET(GENMASK(9, 4), val));
    return 0;
}

static int sstar_phy_utmi_swing_trim_open(struct inode *inode, struct file *file)
{
    return single_open(file, sstar_phy_utmi_swing_trim_show, inode->i_private);
}

static ssize_t sstar_phy_utmi_swing_trim_set(struct sstar_phy_port *port, unsigned int val)
{
    void __iomem *base = port->reg;
    unsigned long flags;

    spin_lock_irqsave(&port->lock, flags);
    CLRSETREG16(base + (0x44 << 2), GENMASK(9, 4), FIELD_PREP(GENMASK(9, 4), val));
    spin_unlock_irqrestore(&port->lock, flags);
    return 0;
}

static ssize_t sstar_phy_utmi_swing_trim_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    struct seq_file *      s       = file->private_data;
    struct sstar_phy_port *port    = s->private;
    char                   buf[32] = {0};
    u32                    val;
    int                    ret;

    if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
        return -EFAULT;

    ret = kstrtouint(buf, 0, &val);
    if (ret)
    {
        return ret;
    }

    ret = sstar_phy_utmi_swing_trim_set(port, val);
    if (ret)
    {
        return ret;
    }

    return count;
}

static const struct file_operations sstar_swing_trim_fops = {
    .open    = sstar_phy_utmi_swing_trim_open,
    .write   = sstar_phy_utmi_swing_trim_write,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

static int sstar_phy_utmi_disc_ref_vol_show(struct seq_file *s, void *unused)
{
    struct sstar_phy_port *port = s->private;
    void __iomem *         base = port->reg;
    unsigned long          flags;
    u32                    val;

    spin_lock_irqsave(&port->lock, flags);
    val = INREG16(base + (0x41 << 2));
    spin_unlock_irqrestore(&port->lock, flags);

    seq_printf(s, "disconnect_refer_voltage(0~31): 0x%lx\r\n", FIELD_GET(GENMASK(4, 0), val));
    return 0;
}

static int sstar_phy_utmi_disc_ref_vol_open(struct inode *inode, struct file *file)
{
    return single_open(file, sstar_phy_utmi_disc_ref_vol_show, inode->i_private);
}

static ssize_t sstar_phy_utmi_disc_ref_vol_set(struct sstar_phy_port *port, unsigned int val)
{
    void __iomem *base = port->reg;
    unsigned long flags;

    spin_lock_irqsave(&port->lock, flags);
    CLRSETREG16(base + (0x41 << 2), GENMASK(4, 0), FIELD_PREP(GENMASK(4, 0), val));
    spin_unlock_irqrestore(&port->lock, flags);
    return 0;
}

static ssize_t sstar_phy_utmi_disc_ref_vol_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    struct seq_file *      s       = file->private_data;
    struct sstar_phy_port *port    = s->private;
    char                   buf[32] = {0};
    u32                    val;
    int                    ret;

    if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
        return -EFAULT;

    ret = kstrtouint(buf, 0, &val);
    if (ret)
    {
        return ret;
    }

    ret = sstar_phy_utmi_disc_ref_vol_set(port, val);
    if (ret)
    {
        return ret;
    }

    return count;
}

static const struct file_operations sstar_disc_ref_vol_fops = {
    .open    = sstar_phy_utmi_disc_ref_vol_open,
    .write   = sstar_phy_utmi_disc_ref_vol_write,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

static int sstar_phy_utmi_squelch_ref_vol_show(struct seq_file *s, void *unused)
{
    struct sstar_phy_port *port = s->private;
    void __iomem *         base = port->reg;
    unsigned long          flags;
    u32                    val;

    spin_lock_irqsave(&port->lock, flags);
    val = INREG16(base + (0x4B << 2));
    spin_unlock_irqrestore(&port->lock, flags);

    seq_printf(s, "squelch_refer_voltage(0~31): 0x%lx\r\n", FIELD_GET(GENMASK(4, 0), val));
    return 0;
}

static int sstar_phy_utmi_squelch_ref_vol_open(struct inode *inode, struct file *file)
{
    return single_open(file, sstar_phy_utmi_squelch_ref_vol_show, inode->i_private);
}

static ssize_t sstar_phy_utmi_squelch_ref_vol_set(struct sstar_phy_port *port, unsigned int val)
{
    void __iomem *base = port->reg;
    unsigned long flags;

    spin_lock_irqsave(&port->lock, flags);
    CLRSETREG16(base + (0x4B << 2), GENMASK(4, 0), FIELD_PREP(GENMASK(4, 0), val));
    spin_unlock_irqrestore(&port->lock, flags);
    return 0;
}

static ssize_t sstar_phy_utmi_squelch_ref_vol_write(struct file *file, const char __user *ubuf, size_t count,
                                                    loff_t *ppos)
{
    struct seq_file *      s       = file->private_data;
    struct sstar_phy_port *port    = s->private;
    char                   buf[32] = {0};
    u32                    val;
    int                    ret;

    if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
        return -EFAULT;

    ret = kstrtouint(buf, 0, &val);
    if (ret)
    {
        return ret;
    }

    ret = sstar_phy_utmi_squelch_ref_vol_set(port, val);
    if (ret)
    {
        return ret;
    }

    return count;
}

static const struct file_operations sstar_squelch_ref_vol_fops = {
    .open    = sstar_phy_utmi_squelch_ref_vol_open,
    .write   = sstar_phy_utmi_squelch_ref_vol_write,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

void sstar_phy_utmi_atop_set(struct sstar_phy_port *port)
{
    void __iomem *base = port->reg;
    u32           val;

    if (!device_property_read_u32(port->dev, "pre-emphasis", &val))
    {
        sstar_phy_utmi_pre_emphasis_set(port, val);
        dev_info(port->dev, "pre emphasis: 0x%02x\n", val);
    }

    if (!device_property_read_u32(port->dev, "slew-rate", &val))
    {
        sstar_phy_utmi_slew_rate_set(port, val);
        dev_info(port->dev, "slew rate: 0x%02x\n", val);
    }

    if (!device_property_read_u32(port->dev, "swing", &val))
    {
        sstar_phy_utmi_swing_trim_set(port, val);
        dev_info(port->dev, "swing trim code: 0x%04x\n", val);
    }

    if (!device_property_read_u32(port->dev, "disconnect-refer-vol", &val))
    {
        sstar_phy_utmi_disc_ref_vol_set(port, val);
        dev_info(port->dev, "disconnect reference voltage: 0x%04x\n", val);
    }

    if (!device_property_read_u32(port->dev, "squelch-refer-vol", &val))
    {
        sstar_phy_utmi_squelch_ref_vol_set(port, val);
        dev_info(port->dev, "squelch reference voltage: 0x%04x\n", val);
    }

    if (!device_property_read_u32(port->dev, "ls-cross-level", &val))
    {
        SETREG16(base + (0x49 << 2), val);
        val = INREG16(base + (0x49 << 2));
        dev_info(port->dev, "ls-cross-level = 0x%04x\r\n", val);
    }

    if (!device_property_read_u32(port->dev, "fs-cross-level", &val))
    {
        SETREG16(base + (0x42 << 2), val);
        val = INREG16(base + (0x42 << 2));
        dev_info(port->dev, "fs-cross-level = 0x%04x\r\n", val);
    }

    return;
}
EXPORT_SYMBOL_GPL(sstar_phy_utmi_atop_set);

static int sstar_phy_edswitch_show(struct seq_file *s, void *unused)
{
    struct sstar_phy_port *port = s->private;

    if (port->ed_hs_switch_on)
        seq_printf(s, "eye-diagram mode: high speed on\n");
    else if (port->ed_fs_switch_on)
        seq_printf(s, "eye-diagram mode: full speed on\n");
    else
        seq_printf(s, "eye-diagram mode: off\n");

    seq_printf(s, "\nNote: When switch eye-diagram mode on, the host is unable to work\n");
    seq_printf(s, "You must reboot to back to normal\n");

    return 0;
}

static int sstar_phy_edswitch_open(struct inode *inode, struct file *file)
{
    return single_open(file, sstar_phy_edswitch_show, inode->i_private);
}

static void sstar_phy_edswitch_set_v1(struct sstar_phy_port *priv, EYE_DIAGRAM_MODE_E speed)
{
    void __iomem *reg_bank1 = priv->ed_bank1;
    void __iomem *reg_bank2 = priv->ed_bank2;
    void __iomem *reg_bank3 = priv->ed_bank3;
    void __iomem *reg_bank4 = priv->ed_bank4;

    OUTREG8(GET_REG8_ADDR(reg_bank3, 0x00), 0xff);
    OUTREG8(GET_REG8_ADDR(reg_bank3, 0x02), 0x00);
    OUTREG8(GET_REG8_ADDR(reg_bank1, 0x00), 0xb0);
    OUTREG8(GET_REG8_ADDR(reg_bank1, 0x01), 0x10);
    OUTREG8(GET_REG8_ADDR(reg_bank1, 0x04), 0x10);
    OUTREG8(GET_REG8_ADDR(reg_bank1, 0x05), 0x01);
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x08), 0x0f);
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x09), 0x04);
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x00), 0x05);
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x01), 0x7f);
    OUTREG8(GET_REG8_ADDR(reg_bank4, 0xfe), 0xe1);
    OUTREG8(GET_REG8_ADDR(reg_bank4, 0xff), 0x08);
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x08), 0x0f);
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x09), 0x04);
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x20), 0xa1);
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x21), 0x80);
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x22), 0x88);
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x23), 0x20);

    if (EYE_DIAGRAM_MODE_HIGH_SPEED == speed)
    {
        OUTREG8(GET_REG8_ADDR(reg_bank2, 0x00), 0x03);
        OUTREG8(GET_REG8_ADDR(reg_bank2, 0x01), 0x6b);
        mdelay(1);
    }

    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x00), 0xc3);
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x01), 0x69);
    mdelay(2);

    if (EYE_DIAGRAM_MODE_HIGH_SPEED == speed)
    {
        OUTREG8(GET_REG8_ADDR(reg_bank2, 0x00), 0x01);
        OUTREG8(GET_REG8_ADDR(reg_bank2, 0x01), 0x00);
    }
    else if (EYE_DIAGRAM_MODE_FULL_SPEED == speed)
    {
        OUTREG8(GET_REG8_ADDR(reg_bank2, 0x02), 0x84);
        OUTREG8(GET_REG8_ADDR(reg_bank2, 0x03), 0x90);
        OUTREG8(GET_REG8_ADDR(reg_bank2, 0x00), 0x01);
        OUTREG8(GET_REG8_ADDR(reg_bank2, 0x01), 0x02);
        OUTREG8(GET_REG8_ADDR(reg_bank2, 0x00), 0x01);
        OUTREG8(GET_REG8_ADDR(reg_bank2, 0x01), 0x00);
        mdelay(1);
    }
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x3c), 0x01);
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x3d), 0x00);
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x3c), 0x00);
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x3d), 0x00);

    if (EYE_DIAGRAM_MODE_HIGH_SPEED == speed)
        mdelay(2);
    else if (EYE_DIAGRAM_MODE_FULL_SPEED == speed)
        mdelay(5);

    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x10), 0x78);
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x11), 0x00);
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x06), 0x43);
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x07), 0x00);
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x06), 0x40);
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x07), 0x00);

    if (EYE_DIAGRAM_MODE_HIGH_SPEED == speed)
    {
        OUTREG8(GET_REG8_ADDR(reg_bank2, 0x00), 0x01);
        OUTREG8(GET_REG8_ADDR(reg_bank2, 0x01), 0x00);
        OUTREG8(GET_REG8_ADDR(reg_bank2, 0x14), 0x00);
        OUTREG8(GET_REG8_ADDR(reg_bank2, 0x15), 0x06);
    }
    else if (EYE_DIAGRAM_MODE_FULL_SPEED == speed)
    {
        OUTREG8(GET_REG8_ADDR(reg_bank2, 0x00), 0xeb);
        OUTREG8(GET_REG8_ADDR(reg_bank2, 0x01), 0x00);
        OUTREG8(GET_REG8_ADDR(reg_bank2, 0x14), 0x40);
        OUTREG8(GET_REG8_ADDR(reg_bank2, 0x15), 0x07);
    }
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x34), 0x00);
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x35), 0x00);
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x32), 0xfe);
    OUTREG8(GET_REG8_ADDR(reg_bank2, 0x33), 0x0b);

    if (EYE_DIAGRAM_MODE_HIGH_SPEED == speed)
        mdelay(2);
    else if (EYE_DIAGRAM_MODE_FULL_SPEED == speed)
        mdelay(170);
}

static void sstar_phy_edswitch_set_v2(struct sstar_phy_port *priv, EYE_DIAGRAM_MODE_E speed)
{
    void __iomem *reg_bank1 = priv->ed_bank1;
    void __iomem *reg_bank2 = priv->ed_bank2;
    void __iomem *reg_bank3 = priv->ed_bank3;
    unsigned long flags;

    spin_lock_irqsave(&priv->lock, flags);
    if (EYE_DIAGRAM_MODE_HIGH_SPEED == speed)
    {
        OUTREG8(GET_REG8_ADDR(reg_bank1, 0x00), 0xc0);
        OUTREG8(GET_REG8_ADDR(reg_bank2, 0x00), 0x05);
        OUTREG8(GET_REG8_ADDR(reg_bank2, 0x01), 0x7f);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x08), 0xef);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x09), 0x04);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x0a), 0x00);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x0b), 0x40);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x08), 0xef);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x09), 0x04);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x0a), 0x40);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x0b), 0x40);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x08), 0x6f);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x09), 0x04);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x00), 0x05);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x01), 0x3f);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x00), 0x07);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x01), 0x3f);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x00), 0x47);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x01), 0x3f);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x00), 0x47);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x01), 0x3f);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x00), 0xc7);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x01), 0x3f);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x10), 0x40);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x11), 0x00);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x00), 0xc7);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x01), 0x3f);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x10), 0xc0);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x11), 0x00);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x06), 0x24);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x07), 0x30);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x08), 0x6f);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x09), 0x04);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x0a), 0x40);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x0b), 0x40);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x00), 0xc7);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x01), 0x3b);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x3c), 0x01);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x3d), 0x00);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x3c), 0x00);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x3d), 0x00);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x00), 0xc7);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x01), 0x33);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x00), 0xc7);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x01), 0x32);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x10), 0x78);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x11), 0x00);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x06), 0x43);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x07), 0x00);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x06), 0x40);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x07), 0x00);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x00), 0x01);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x01), 0x00);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x14), 0x00);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x15), 0x06);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x34), 0x00);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x35), 0x00);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x32), 0xfe);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x33), 0x0b);

        dev_info(priv->dev, "eye pattern high speed setup done\n");
    }
    else if (EYE_DIAGRAM_MODE_FULL_SPEED == speed)
    {
        OUTREG8(GET_REG8_ADDR(reg_bank1, 0x00141e00), 0xc0);
        OUTREG8(GET_REG8_ADDR(reg_bank2, 0x00), 0x05);
        OUTREG8(GET_REG8_ADDR(reg_bank2, 0x01), 0x7f);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x08), 0xef);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x09), 0x04);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x0a), 0x00);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x0b), 0x40);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x08), 0xef);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x09), 0x04);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x0a), 0x40);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x0b), 0x40);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x08), 0x6f);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x09), 0x04);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x00), 0x05);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x01), 0x3f);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x00), 0x07);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x01), 0x3f);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x00), 0x0f);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x01), 0x3f);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x00), 0x0f);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x01), 0x3f);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x00), 0x4f);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x01), 0x3f);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x00), 0x4f);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x01), 0x3f);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x00), 0xcf);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x01), 0x3f);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x02), 0x80);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x03), 0x94);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x02), 0x80);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x03), 0x94);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x00), 0xcf);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x01), 0x2f);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x10), 0x40);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x11), 0x08);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x10), 0x40);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x11), 0x0a);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x10), 0x78);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x11), 0x00);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x06), 0x43);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x07), 0x00);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x06), 0x40);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x07), 0x00);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x00), 0xeb);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x01), 0x00);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x14), 0x40);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x15), 0x07);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x34), 0x00);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x35), 0x00);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x32), 0xfe);
        OUTREG8(GET_REG8_ADDR(reg_bank3, 0x33), 0x0b);

        dev_info(priv->dev, "eye pattern full speed setup done\n");
    }

    spin_unlock_irqrestore(&priv->lock, flags);
}

static void sstar_phy_edswitch_set(struct sstar_phy_port *priv, EYE_DIAGRAM_MODE_E speed)
{
    if (priv->phy_data && priv->phy_data->revision == 1)
    {
        sstar_phy_edswitch_set_v1(priv, speed);
    }
    else
    {
        sstar_phy_edswitch_set_v2(priv, speed);
    }
}

static ssize_t sstar_phy_edswitch_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    struct seq_file *      s    = file->private_data;
    struct sstar_phy_port *priv = s->private;

    unsigned long      flags;
    char               buf[32] = {0};
    EYE_DIAGRAM_MODE_E speed   = EYE_DIAGRAM_MODE_UNKNOWN;

    if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
        return -EFAULT;

    if (!strncmp(buf, "hs", 2))
    {
        priv->ed_hs_switch_on = true;
        speed                 = EYE_DIAGRAM_MODE_HIGH_SPEED;
    }
    else if (!strncmp(buf, "fs", 2))
    {
        priv->ed_fs_switch_on = true;
        speed                 = EYE_DIAGRAM_MODE_FULL_SPEED;
    }
    else
    {
        dev_err(&priv->phy->dev, "Unknow eye diagram switch option: %s\n", buf);
        return -EINVAL;
    }

    spin_lock_irqsave(&priv->lock, flags);
    sstar_phy_edswitch_set(priv, speed);
    spin_unlock_irqrestore(&priv->lock, flags);

    return count;
}

static const struct file_operations sstar_phy_edswitch_fops = {
    .open    = sstar_phy_edswitch_open,
    .write   = sstar_phy_edswitch_write,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

int sstar_phy_edswitch_creat(struct device_node *node, struct sstar_phy_port *port)
{
    phandle             ph;
    struct device_node *ednode;

    if (!of_property_read_u32(node, "eye-diagram", &ph))
    {
        ednode = of_find_node_by_phandle(ph);
        if (!ednode)
        {
            dev_err(port->dev, "No specified eye diagram node\n");
            return -EINVAL;
        }

        port->ed_bank1 = of_iomap(ednode, 0);
        port->ed_bank2 = of_iomap(ednode, 1);
        port->ed_bank3 = of_iomap(ednode, 2);
        if (port->phy_data && port->phy_data->revision == 1)
        {
            port->ed_bank4 = of_iomap(ednode, 3);
        }
        port->has_ed_switch = true;
        dev_info(port->dev, "Eye diagram bank: %p %p %p %p\n", port->ed_bank1, port->ed_bank2, port->ed_bank3,
                 port->ed_bank4);
        return 0;
    }
    return -EINVAL;
}

void sstar_phy_utmi_debugfs_init(struct sstar_phy_port *port)
{
    spin_lock_init(&port->lock);
    port->root = debugfs_create_dir(dev_name(port->dev), usb_debug_root);

    if (port->phy_data && port->phy_data->revision == 1)
    {
        debugfs_create_file("tx_swing", 0644, port->root, port, &sstar_tx_swing_fops);
        debugfs_create_file("de_emphasis_current", 0644, port->root, port, &sstar_dem_cur_fops);
        debugfs_create_file("cm_current", 0644, port->root, port, &sstar_cm_cur_fops);
    }
    else
    {
        debugfs_create_file("pre_emphasis", 0644, port->root, port, &sstar_pre_emphasis_fops);
        debugfs_create_file("slew_rate", 0644, port->root, port, &sstar_slew_rate_fops);
        debugfs_create_file("swing_trim", 0644, port->root, port, &sstar_swing_trim_fops);
        debugfs_create_file("disconnect_refer_voltage", 0644, port->root, port, &sstar_disc_ref_vol_fops);
        debugfs_create_file("squelch_refer_voltage", 0644, port->root, port, &sstar_squelch_ref_vol_fops);
        debugfs_create_file("ls_cross_rise", 0644, port->root, port, &sstar_ls_cross_rise_fops);
        debugfs_create_file("ls_cross_fall", 0644, port->root, port, &sstar_ls_cross_fall_fops);
    }

    if (port->has_ed_switch)
        debugfs_create_file("eye_diagram_switch", 0644, port->root, port, &sstar_phy_edswitch_fops);
}

void sstar_phy_utmi_debugfs_exit(struct sstar_phy_port *port)
{
    debugfs_remove_recursive(port->root);
}
