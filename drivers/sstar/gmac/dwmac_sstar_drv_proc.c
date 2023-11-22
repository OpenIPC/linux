/*
 * dwmac_sstar_drv_proc.c- Sigmastar
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

#include <linux/clk.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_net.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/pm_wakeirq.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/stmmac.h>
#include <linux/notifier.h>
#include <linux/syscalls.h>
#include "dwmac4.h"
#include <linux/platform_device.h>
#include "stmmac_platform.h"
#include "sstar_gmac.h"

char gbDisirq[GMAC_NUM];

static int proc_gmac_sphen_show(struct seq_file *m, void *v)
{
    struct sstar_dwmac *dwmac = m->private;
    struct net_device * ndev  = dev_get_drvdata(dwmac->dev);
    struct stmmac_priv *priv  = netdev_priv(ndev);

    printk("gmac_%u_sphen=%u\n", dwmac->id, priv->sph);
    return 0;
}

static int proc_gmac_sphen_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_gmac_sphen_show, PDE_DATA(inode));
}

static ssize_t proc_gmac_sphen_write(struct file *file, const char __user *buf, size_t len, loff_t *pos)
{
    u8                  chan       = 0;
    u8                  sphen      = 0xFF;
    char                strbuf[16] = {0};
    struct sstar_dwmac *dwmac      = ((struct seq_file *)file->private_data)->private;
    struct net_device * ndev       = dev_get_drvdata(dwmac->dev);
    struct stmmac_priv *priv       = netdev_priv(ndev);

    if (len > sizeof(strbuf) - 1)
    {
        printk(KERN_ERR "command len is to long!\n");
        return len;
    }

    if (copy_from_user(strbuf, buf, len))
    {
        return -EFAULT;
    }

    strbuf[len] = 0;
    sphen       = simple_strtol(strbuf, NULL, 10);

    if (sphen)
    {
        priv->dev->hw_features |= NETIF_F_GRO;
        priv->sph = true;
    }
    else
    {
        priv->dev->hw_features &= ~NETIF_F_GRO;
        priv->sph = false;
    }

    for (chan = 0; chan < priv->plat->rx_queues_to_use; chan++)
        stmmac_enable_sph(priv, priv->ioaddr, sphen, chan);

    printk("gmac_%u_sphen = %u\n", dwmac->id, sphen);

    return len;
}

static const struct proc_ops proc_gmac_sphen_ops = {.proc_open    = proc_gmac_sphen_open,
                                                    .proc_read    = seq_read,
                                                    .proc_write   = proc_gmac_sphen_write,
                                                    .proc_lseek   = seq_lseek,
                                                    .proc_release = single_release};

static int proc_gmac_txcof_show(struct seq_file *m, void *v)
{
    struct sstar_dwmac *dwmac = m->private;
    struct net_device * ndev  = dev_get_drvdata(dwmac->dev);
    struct stmmac_priv *priv  = netdev_priv(ndev);

    printk("gmac_%u_txcof=%u\n", dwmac->id, priv->tx_coal_frames);
    return 0;
}

static int proc_gmac_txcof_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_gmac_txcof_show, PDE_DATA(inode));
}

static ssize_t proc_gmac_txcof_write(struct file *file, const char __user *buf, size_t len, loff_t *pos)
{
    u32                 tx_coalf   = STMMAC_TX_FRAMES;
    char                strbuf[16] = {0};
    struct sstar_dwmac *dwmac      = ((struct seq_file *)file->private_data)->private;
    struct net_device * ndev       = dev_get_drvdata(dwmac->dev);
    struct stmmac_priv *priv       = netdev_priv(ndev);

    if (len > sizeof(strbuf) - 1)
    {
        printk(KERN_ERR "command len is to long!\n");
        return len;
    }

    if (copy_from_user(strbuf, buf, len))
    {
        return -EFAULT;
    }

    strbuf[len] = 0;
    tx_coalf    = simple_strtol(strbuf, NULL, 10);

    priv->tx_coal_frames = tx_coalf;
    printk("gmac_%u_txcof = %u\n", dwmac->id, tx_coalf);

    return len;
}

static const struct proc_ops proc_gmac_txcof_ops = {.proc_open    = proc_gmac_txcof_open,
                                                    .proc_read    = seq_read,
                                                    .proc_write   = proc_gmac_txcof_write,
                                                    .proc_lseek   = seq_lseek,
                                                    .proc_release = single_release};

static int proc_gmac_rxwt_show(struct seq_file *m, void *v)
{
    struct sstar_dwmac *dwmac = m->private;
    struct net_device * ndev  = dev_get_drvdata(dwmac->dev);
    struct stmmac_priv *priv  = netdev_priv(ndev);

    printk("gmac_%u_rxwt=0x%08x\n", dwmac->id, priv->rx_riwt);
    return 0;
}

static int proc_gmac_rxwt_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_gmac_rxwt_show, PDE_DATA(inode));
}

static ssize_t proc_gmac_rxwt_write(struct file *file, const char __user *buf, size_t len, loff_t *pos)
{
    u32                 rxwt       = DEF_DMA_RIWT;
    char                strbuf[16] = {0};
    struct sstar_dwmac *dwmac      = ((struct seq_file *)file->private_data)->private;
    struct net_device * ndev       = dev_get_drvdata(dwmac->dev);
    struct stmmac_priv *priv       = netdev_priv(ndev);

#if defined(CONFIG_ARCH_SSTAR) && defined(CONFIG_SSTAR_SNPS_GMAC_RXIC)
    printk("[%s][%d] RXIC does NOT support RXWT\n", __FUNCTION__, __LINE__);
    return -EFAULT;
#endif
    if (len > sizeof(strbuf) - 1)
    {
        printk(KERN_ERR "command len is to long!\n");
        return len;
    }

    if (copy_from_user(strbuf, buf, len))
    {
        return -EFAULT;
    }

    strbuf[len] = 0;
    // rxwt        = simple_strtol(strbuf, NULL, 10);
    sscanf(strbuf, "%x", &rxwt);

    if (priv->use_riwt)
    {
        priv->rx_riwt = rxwt;
        stmmac_rx_watchdog(priv, priv->ioaddr, priv->rx_riwt, priv->plat->rx_queues_to_use);
    }

    printk("gmac_%u_rxwt = 0x%08x\n", dwmac->id, priv->rx_riwt);

    return len;
}

static const struct proc_ops proc_gmac_rxwt_ops = {.proc_open    = proc_gmac_rxwt_open,
                                                   .proc_read    = seq_read,
                                                   .proc_write   = proc_gmac_rxwt_write,
                                                   .proc_lseek   = seq_lseek,
                                                   .proc_release = single_release};

static int proc_gmac_irqdis_show(struct seq_file *m, void *v)
{
    struct sstar_dwmac *dwmac = m->private;

    printk("gmac_%u_irqdis=%u\n", dwmac->id, gbDisirq[dwmac->id]);
    return 0;
}

static int proc_gmac_irqdis_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_gmac_irqdis_show, PDE_DATA(inode));
}

static ssize_t proc_gmac_irqdis_write(struct file *file, const char __user *buf, size_t len, loff_t *pos)
{
    u32                 bdisirq    = 0;
    char                strbuf[16] = {0};
    struct sstar_dwmac *dwmac      = ((struct seq_file *)file->private_data)->private;

    if (len > sizeof(strbuf) - 1)
    {
        printk(KERN_ERR "command len is to long!\n");
        return len;
    }

    if (copy_from_user(strbuf, buf, len))
    {
        return -EFAULT;
    }

    strbuf[len] = 0;
    bdisirq     = simple_strtol(strbuf, NULL, 10);

    gbDisirq[dwmac->id] = bdisirq;

    printk("gmac_%u_disirq = %u\n", dwmac->id, gbDisirq[dwmac->id]);

    return len;
}

static const struct proc_ops proc_gmac_irqdis_ops = {.proc_open    = proc_gmac_irqdis_open,
                                                     .proc_read    = seq_read,
                                                     .proc_write   = proc_gmac_irqdis_write,
                                                     .proc_lseek   = seq_lseek,
                                                     .proc_release = single_release};

static int proc_gmac_rxwt_disable_show(struct seq_file *m, void *v)
{
    struct sstar_dwmac *dwmac = m->private;
    struct net_device * ndev  = dev_get_drvdata(dwmac->dev);
    struct stmmac_priv *priv  = netdev_priv(ndev);

    printk("gmac_%u_rxwt_dis=%u\n", dwmac->id, priv->plat->riwt_off);
    return 0;
}

static int proc_gmac_rxwt_disable_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_gmac_rxwt_disable_show, PDE_DATA(inode));
}

static ssize_t proc_gmac_rxwt_disable_write(struct file *file, const char __user *buf, size_t len, loff_t *pos)
{
    u32                 brxwt_dis  = 0;
    char                strbuf[16] = {0};
    struct sstar_dwmac *dwmac      = ((struct seq_file *)file->private_data)->private;
    struct net_device * ndev       = dev_get_drvdata(dwmac->dev);
    struct stmmac_priv *priv       = netdev_priv(ndev);

    if (len > sizeof(strbuf) - 1)
    {
        printk(KERN_ERR "command len is to long!\n");
        return len;
    }

    if (copy_from_user(strbuf, buf, len))
    {
        return -EFAULT;
    }

    strbuf[len] = 0;
    brxwt_dis   = simple_strtol(strbuf, NULL, 10);

    if (brxwt_dis)
    {
        priv->plat->riwt_off = 1;
        priv->use_riwt       = 0;
        priv->rx_riwt        = 0;
    }
    else
    {
        priv->plat->riwt_off = 0;
        priv->use_riwt       = 1;
        priv->rx_riwt        = DEF_DMA_RIWT;
    }

    stmmac_rx_watchdog(priv, priv->ioaddr, priv->rx_riwt, priv->plat->rx_queues_to_use);
    printk("gmac_%u_rxwt_dis = %u\n", dwmac->id, brxwt_dis);

    return len;
}

static const struct proc_ops proc_gmac_rxwt_disable_ops = {.proc_open    = proc_gmac_rxwt_disable_open,
                                                           .proc_read    = seq_read,
                                                           .proc_write   = proc_gmac_rxwt_disable_write,
                                                           .proc_lseek   = seq_lseek,
                                                           .proc_release = single_release};

#if defined(CONFIG_ARCH_SSTAR) && defined(CONFIG_SSTAR_SNPS_GMAC_RXIC)
static int proc_gmac_rxic_show(struct seq_file *m, void *v)
{
    struct sstar_dwmac *dwmac = m->private;
    struct net_device * ndev  = dev_get_drvdata(dwmac->dev);
    struct stmmac_priv *priv  = netdev_priv(ndev);

    printk("gmac_%u rxic_max = %u\n", dwmac->id, priv->rxic_msk_bit_max_cur);
    return 0;
}

static int proc_gmac_rxic_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_gmac_rxic_show, PDE_DATA(inode));
}

static ssize_t proc_gmac_rxic_write(struct file *file, const char __user *buf, size_t len, loff_t *pos)
{
    int                 rxic_max;
    char                strbuf[16] = {0};
    struct sstar_dwmac *dwmac      = ((struct seq_file *)file->private_data)->private;
    struct net_device * ndev       = dev_get_drvdata(dwmac->dev);
    struct stmmac_priv *priv       = netdev_priv(ndev);

    if (len > sizeof(strbuf) - 1)
    {
        printk(KERN_ERR "command len is to long!\n");
        return len;
    }

    if (copy_from_user(strbuf, buf, len))
    {
        return -EFAULT;
    }

    strbuf[len] = 0;
    rxic_max    = simple_strtol(strbuf, NULL, 10);

    if (rxic_max > priv->rxic_msk_bit_max)
    {
        printk("[%s][%d] rxic max is %d\n", __FUNCTION__, __LINE__, priv->rxic_msk_bit_max);
        return -EFAULT;
    }

    priv->rxic_msk_bit_max_cur = rxic_max;

    printk("gmac_%u rxic_max = %u\n", dwmac->id, priv->rxic_msk_bit_max_cur);

    return len;
}

static const struct proc_ops proc_gmac_rxic_ops = {.proc_open    = proc_gmac_rxic_open,
                                                   .proc_read    = seq_read,
                                                   .proc_write   = proc_gmac_rxic_write,
                                                   .proc_lseek   = seq_lseek,
                                                   .proc_release = single_release};

#endif

#ifdef CONFIG_SSTAR_SNPS_GMAC_STORM_PROTECT
static int proc_gmac_bcast_thresh_show(struct seq_file *m, void *v)
{
    struct sstar_dwmac *dwmac = m->private;

    printk("gmac_%u_bcast_thresh = %d\n", dwmac->id, dwmac->bcast_thresh);
    return 0;
}

static int proc_gmac_bcast_thresh_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_gmac_bcast_thresh_show, PDE_DATA(inode));
}

static ssize_t proc_gmac_bcast_thresh_write(struct file *file, const char __user *buf, size_t len, loff_t *pos)
{
    char                strbuf[16] = {0};
    struct sstar_dwmac *dwmac      = ((struct seq_file *)file->private_data)->private;

    if (len > sizeof(strbuf) - 1)
    {
        printk(KERN_ERR "command len is to long!\n");
        return len;
    }

    if (copy_from_user(strbuf, buf, len))
    {
        return -EFAULT;
    }

    dwmac->bcast_thresh = simple_strtol(strbuf, NULL, 10);

    printk("gmac_%u_bcast_threshold = %u\n", dwmac->id, dwmac->bcast_thresh);

    return len;
}

static const struct proc_ops proc_gmac_bcast_threshold_ops = {.proc_open    = proc_gmac_bcast_thresh_open,
                                                              .proc_read    = seq_read,
                                                              .proc_write   = proc_gmac_bcast_thresh_write,
                                                              .proc_lseek   = seq_lseek,
                                                              .proc_release = single_release};

static int proc_gmac_mcast_thresh_show(struct seq_file *m, void *v)
{
    struct sstar_dwmac *dwmac = m->private;

    printk("gmac_%u_mcast_thresh = %d\n", dwmac->id, dwmac->mcast_thresh);
    return 0;
}

static int proc_gmac_mcast_thresh_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_gmac_mcast_thresh_show, PDE_DATA(inode));
}

static ssize_t proc_gmac_mcast_thresh_write(struct file *file, const char __user *buf, size_t len, loff_t *pos)
{
    char                strbuf[16] = {0};
    struct sstar_dwmac *dwmac      = ((struct seq_file *)file->private_data)->private;

    if (len > sizeof(strbuf) - 1)
    {
        printk(KERN_ERR "command len is to long!\n");
        return len;
    }

    if (copy_from_user(strbuf, buf, len))
    {
        return -EFAULT;
    }

    dwmac->mcast_thresh = simple_strtol(strbuf, NULL, 10);

    printk("gmac_%u_bcast_threshold = %u\n", dwmac->id, dwmac->mcast_thresh);

    return len;
}

static const struct proc_ops proc_gmac_mcast_threshold_ops = {.proc_open    = proc_gmac_mcast_thresh_open,
                                                              .proc_read    = seq_read,
                                                              .proc_write   = proc_gmac_mcast_thresh_write,
                                                              .proc_lseek   = seq_lseek,
                                                              .proc_release = single_release};

static int proc_gmac_storm_timer_pd_show(struct seq_file *m, void *v)
{
    struct sstar_dwmac *dwmac = m->private;

    printk("gmac_%u_storm_ms_timer_pd = %d\n", dwmac->id, dwmac->storm_ms_timer);
    return 0;
}

static int proc_gmac_storm_timer_pd_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_gmac_storm_timer_pd_show, PDE_DATA(inode));
}

static ssize_t proc_gmac_storm_timer_pd_write(struct file *file, const char __user *buf, size_t len, loff_t *pos)
{
    char                strbuf[16] = {0};
    struct sstar_dwmac *dwmac      = ((struct seq_file *)file->private_data)->private;

    if (len > sizeof(strbuf) - 1)
    {
        printk(KERN_ERR "command len is to long!\n");
        return len;
    }

    if (copy_from_user(strbuf, buf, len))
    {
        return -EFAULT;
    }

    dwmac->storm_ms_timer = simple_strtol(strbuf, NULL, 10);

    printk("gmac_%u_storm_ms_timer_pd = %u\n", dwmac->id, dwmac->storm_ms_timer);

    return len;
}

static const struct proc_ops proc_gmac_storm_timer_pd_ops = {.proc_open    = proc_gmac_storm_timer_pd_open,
                                                             .proc_read    = seq_read,
                                                             .proc_write   = proc_gmac_storm_timer_pd_write,
                                                             .proc_lseek   = seq_lseek,
                                                             .proc_release = single_release};

static int proc_gmac_bcast_status_show(struct seq_file *m, void *v)
{
    struct sstar_dwmac *dwmac = m->private;

    if (dwmac->bcast_prct_on == 1)
        printk("gmac_%u_bcast_protect = On\n", dwmac->id);
    else
        printk("gmac_%u_bcast_protect = Off\n", dwmac->id);

    return 0;
}

static int proc_gmac_bcast_status_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_gmac_bcast_status_show, PDE_DATA(inode));
}

static const struct proc_ops proc_gmac_bcast_status_ops = {.proc_open    = proc_gmac_bcast_status_open,
                                                           .proc_read    = seq_read,
                                                           .proc_lseek   = seq_lseek,
                                                           .proc_release = single_release};

static int proc_gmac_mcast_status_show(struct seq_file *m, void *v)
{
    struct sstar_dwmac *dwmac = m->private;

    if (dwmac->mcast_prct_on == 1)
        printk("gmac_%u_mcast_protect = On\n", dwmac->id);
    else
        printk("gmac_%u_mcast_protect = Off\n", dwmac->id);

    return 0;
}

static int proc_gmac_mcast_status_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_gmac_mcast_status_show, PDE_DATA(inode));
}

static const struct proc_ops proc_gmac_mcast_status_ops = {.proc_open    = proc_gmac_mcast_status_open,
                                                           .proc_read    = seq_read,
                                                           .proc_lseek   = seq_lseek,
                                                           .proc_release = single_release};

static int proc_gmac_bcast_enable_show(struct seq_file *m, void *v)
{
    struct sstar_dwmac *dwmac = m->private;

    printk("gmac_%u_bcast_prct_enable = %d\n", dwmac->id, dwmac->bcast_prct_enable);

    return 0;
}

static int proc_gmac_bcast_enable_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_gmac_bcast_enable_show, PDE_DATA(inode));
}

static ssize_t proc_gmac_bcast_enable_write(struct file *file, const char __user *buf, size_t len, loff_t *pos)
{
    char                strbuf[16] = {0};
    struct sstar_dwmac *dwmac      = ((struct seq_file *)file->private_data)->private;
    struct net_device * ndev       = dev_get_drvdata(dwmac->dev);
    struct stmmac_priv *priv       = netdev_priv(ndev);

    if (len > sizeof(strbuf) - 1)
    {
        printk(KERN_ERR "command len is to long!\n");
        return len;
    }

    if (copy_from_user(strbuf, buf, len))
    {
        return -EFAULT;
    }

    dwmac->bcast_prct_enable = simple_strtol(strbuf, NULL, 10);
    if (!dwmac->bcast_prct_enable)
    {
        u32 value;

        value = readl(priv->ioaddr + GMAC_PACKET_FILTER);
        value &= ~GMAC_PACKET_FILTER_DBF;
        writel(value, priv->ioaddr + GMAC_PACKET_FILTER);

        value = readl(priv->mmcaddr + 0x00); // MMC_CNTRL
        value &= ~BIT(8);
        writel(value, priv->mmcaddr + 0x00);
        dwmac->bcast_prct_on = 0;
    }
    printk("gmac_%u_bcast_prct_enable = %u\n", dwmac->id, dwmac->bcast_prct_enable);

    return len;
}

static const struct proc_ops proc_gmac_bcast_enable_ops = {.proc_open    = proc_gmac_bcast_enable_open,
                                                           .proc_read    = seq_read,
                                                           .proc_write   = proc_gmac_bcast_enable_write,
                                                           .proc_lseek   = seq_lseek,
                                                           .proc_release = single_release};

static int proc_gmac_mcast_enable_show(struct seq_file *m, void *v)
{
    struct sstar_dwmac *dwmac = m->private;

    printk("gmac_%u_mcast_prct_enable = %d\n", dwmac->id, dwmac->mcast_prct_enable);

    return 0;
}

static int proc_gmac_mcast_enable_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_gmac_mcast_enable_show, PDE_DATA(inode));
}

static ssize_t proc_gmac_mcast_enable_write(struct file *file, const char __user *buf, size_t len, loff_t *pos)
{
    char                strbuf[16] = {0};
    struct sstar_dwmac *dwmac      = ((struct seq_file *)file->private_data)->private;
    struct net_device * ndev       = dev_get_drvdata(dwmac->dev);
    struct stmmac_priv *priv       = netdev_priv(ndev);

    if (len > sizeof(strbuf) - 1)
    {
        printk(KERN_ERR "command len is to long!\n");
        return len;
    }

    if (copy_from_user(strbuf, buf, len))
    {
        return -EFAULT;
    }

    dwmac->mcast_prct_enable = simple_strtol(strbuf, NULL, 10);
    if (!dwmac->mcast_prct_enable)
    {
        u32 value;

        value = readl(priv->ioaddr + GMAC_PACKET_FILTER);
        value |= GMAC_PACKET_FILTER_HMC;
        value &= ~GMAC_PACKET_FILTER_DAIF;
        writel(value, priv->ioaddr + GMAC_PACKET_FILTER);
        dwmac->mcast_prct_on = 0;
    }
    printk("gmac_%u_mcast_prct_enable = %u\n", dwmac->id, dwmac->mcast_prct_enable);

    return len;
}

static const struct proc_ops proc_gmac_mcast_enable_ops = {.proc_open    = proc_gmac_mcast_enable_open,
                                                           .proc_read    = seq_read,
                                                           .proc_write   = proc_gmac_mcast_enable_write,
                                                           .proc_lseek   = seq_lseek,
                                                           .proc_release = single_release};
#endif

void dwmac_sstar_drv_proc(struct sstar_dwmac *dwmac)
{
    proc_create_data("sphen", 0, dwmac->gmac_root_dir, &proc_gmac_sphen_ops, dwmac);
    proc_create_data("txcof", 0, dwmac->gmac_root_dir, &proc_gmac_txcof_ops, dwmac);
    proc_create_data("rxwt", 0, dwmac->gmac_root_dir, &proc_gmac_rxwt_ops, dwmac);
    proc_create_data("rxwt_disable", 0, dwmac->gmac_root_dir, &proc_gmac_rxwt_disable_ops, dwmac);
    proc_create_data("irqdis", 0, dwmac->gmac_root_dir, &proc_gmac_irqdis_ops, dwmac);
#if defined(CONFIG_ARCH_SSTAR) && defined(CONFIG_SSTAR_SNPS_GMAC_RXIC)
    proc_create_data("rxic_max", 0, dwmac->gmac_root_dir, &proc_gmac_rxic_ops, dwmac);
#endif
#ifdef CONFIG_SSTAR_SNPS_GMAC_STORM_PROTECT
    proc_create_data("bcast_thresh", 0, dwmac->gmac_root_dir, &proc_gmac_bcast_threshold_ops, dwmac);
    proc_create_data("bcast_status", 0, dwmac->gmac_root_dir, &proc_gmac_bcast_status_ops, dwmac);
    proc_create_data("bcast_enable", 0, dwmac->gmac_root_dir, &proc_gmac_bcast_enable_ops, dwmac);
    proc_create_data("mcast_thresh", 0, dwmac->gmac_root_dir, &proc_gmac_mcast_threshold_ops, dwmac);
    proc_create_data("mcast_status", 0, dwmac->gmac_root_dir, &proc_gmac_mcast_status_ops, dwmac);
    proc_create_data("mcast_enable", 0, dwmac->gmac_root_dir, &proc_gmac_mcast_enable_ops, dwmac);
    proc_create_data("storm_timer", 0, dwmac->gmac_root_dir, &proc_gmac_storm_timer_pd_ops, dwmac);
#endif
}
