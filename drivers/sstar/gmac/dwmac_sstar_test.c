/*
 * dwmac_sstar_test.c- Sigmastar
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
#include <linux/bitrev.h>
#include <linux/completion.h>
#include <linux/crc32.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <net/pkt_cls.h>
#include <net/pkt_sched.h>
#include <net/tcp.h>
#include <net/udp.h>
#include "dwmac4.h"
#include <linux/platform_device.h>
#include "stmmac_platform.h"
#include "sstar_gmac.h"

struct sstargmachdr
{
    __be32 version;
    __be64 magic;
    u8     id;
} __packed;

#define SSTAR_GMAC_TEST_PKT_SIZE  (sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct sstargmachdr))
#define SSTAR_GMAC_TEST_PKT_MAGIC 0xdeadcafecafedeadULL
#define SSTAR_GMAC_LB_TIMEOUT     msecs_to_jiffies(200)

struct sstar_gmac_packet_attrs
{
    int            vlan;
    int            vlan_id_in;
    int            vlan_id_out;
    unsigned char *src;
    unsigned char *dst;
    u32            ip_src;
    u32            ip_dst;
    int            tcp;
    int            sport;
    int            dport;
    u32            exp_hash;
    int            dont_wait;
    int            timeout;
    int            size;
    int            max_size;
    int            remove_sa;
    u8             id;
    int            sarc;
    u16            queue_mapping;
    u64            timestamp;
};

struct sstar_gmac_test_priv
{
    struct sstar_gmac_packet_attrs *packet;
    struct packet_type              pt;
    struct completion               comp;
    int                             double_vlan;
    int                             vlan_id;
    int                             ok;
};

extern struct platform_driver sstar_dwmac_driver;

static u8 sstar_gmac_test_next_id;

static int proc_gmac_reprobe_test_show(struct seq_file *m, void *v)
{
    platform_driver_unregister(&sstar_dwmac_driver);
    __platform_driver_register(&sstar_dwmac_driver, sstar_dwmac_driver.driver.owner);
    // sstar_dwmac_driver.remove(gTestpdev);
    // sstar_dwmac_driver.probe(gTestpdev);

    printk("reprobe done\n");
    return 0;
}

static int proc_gmac_reprobe_test_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_gmac_reprobe_test_show, PDE_DATA(inode));
}

static ssize_t proc_gmac_reprobe_test_write(struct file *file, const char __user *buf, size_t len, loff_t *pos)
{
    return len;
}

static const struct proc_ops proc_gmac_reprobe_test_ops = {.proc_open    = proc_gmac_reprobe_test_open,
                                                           .proc_read    = seq_read,
                                                           .proc_write   = proc_gmac_reprobe_test_write,
                                                           .proc_lseek   = seq_lseek,
                                                           .proc_release = single_release};

static struct sk_buff *sstar_test_get_udp_skb(struct stmmac_priv *priv, struct sstar_gmac_packet_attrs *attr)
{
    struct sk_buff *     skb  = NULL;
    struct udphdr *      uhdr = NULL;
    struct tcphdr *      thdr = NULL;
    struct sstargmachdr *shdr;
    struct ethhdr *      ehdr;
    struct iphdr *       ihdr;
    int                  iplen, size;

    size = attr->size + SSTAR_GMAC_TEST_PKT_SIZE;
    if (attr->vlan)
    {
        size += 4;
        if (attr->vlan > 1)
            size += 4;
    }

    if (attr->tcp)
        size += sizeof(struct tcphdr);
    else
        size += sizeof(struct udphdr);

    if (attr->max_size && (attr->max_size > size))
        size = attr->max_size;

    skb = netdev_alloc_skb(priv->dev, size);
    if (!skb)
        return NULL;

    prefetchw(skb->data);

    if (attr->vlan > 1)
        ehdr = skb_push(skb, ETH_HLEN + 8);
    else if (attr->vlan)
        ehdr = skb_push(skb, ETH_HLEN + 4);
    else if (attr->remove_sa)
        ehdr = skb_push(skb, ETH_HLEN - 6);
    else
        ehdr = skb_push(skb, ETH_HLEN);
    skb_reset_mac_header(skb);

    skb_set_network_header(skb, skb->len);
    ihdr = skb_put(skb, sizeof(*ihdr));

    skb_set_transport_header(skb, skb->len);
    if (attr->tcp)
        thdr = skb_put(skb, sizeof(*thdr));
    else
        uhdr = skb_put(skb, sizeof(*uhdr));

    if (!attr->remove_sa)
        eth_zero_addr(ehdr->h_source);
    eth_zero_addr(ehdr->h_dest);
    if (attr->src && !attr->remove_sa)
        ether_addr_copy(ehdr->h_source, attr->src);
    if (attr->dst)
        ether_addr_copy(ehdr->h_dest, attr->dst);

    if (!attr->remove_sa)
    {
        ehdr->h_proto = htons(ETH_P_IP);
    }
    else
    {
        __be16 *ptr = (__be16 *)ehdr;

        /* HACK */
        ptr[3] = htons(ETH_P_IP);
    }

    if (attr->vlan)
    {
        __be16 *tag, *proto;

        if (!attr->remove_sa)
        {
            tag   = (void *)ehdr + ETH_HLEN;
            proto = (void *)ehdr + (2 * ETH_ALEN);
        }
        else
        {
            tag   = (void *)ehdr + ETH_HLEN - 6;
            proto = (void *)ehdr + ETH_ALEN;
        }

        proto[0] = htons(ETH_P_8021Q);
        tag[0]   = htons(attr->vlan_id_out);
        tag[1]   = htons(ETH_P_IP);
        if (attr->vlan > 1)
        {
            proto[0] = htons(ETH_P_8021AD);
            tag[1]   = htons(ETH_P_8021Q);
            tag[2]   = htons(attr->vlan_id_in);
            tag[3]   = htons(ETH_P_IP);
        }
    }

    if (attr->tcp)
    {
        thdr->source = htons(attr->sport);
        thdr->dest   = htons(attr->dport);
        thdr->doff   = sizeof(struct tcphdr) / 4;
        thdr->check  = 0;
    }
    else
    {
        uhdr->source = htons(attr->sport);
        uhdr->dest   = htons(attr->dport);
        uhdr->len    = htons(sizeof(*shdr) + sizeof(*uhdr) + attr->size);
        if (attr->max_size)
            uhdr->len = htons(attr->max_size - (sizeof(*ihdr) + sizeof(*ehdr)));
        uhdr->check = 0;
    }

    ihdr->ihl     = 5;
    ihdr->ttl     = 32;
    ihdr->version = 4;
    if (attr->tcp)
        ihdr->protocol = IPPROTO_TCP;
    else
        ihdr->protocol = IPPROTO_UDP;
    iplen = sizeof(*ihdr) + sizeof(*shdr) + attr->size;
    if (attr->tcp)
        iplen += sizeof(*thdr);
    else
        iplen += sizeof(*uhdr);

    if (attr->max_size)
        iplen = attr->max_size - sizeof(*ehdr);

    ihdr->tot_len  = htons(iplen);
    ihdr->frag_off = 0;
    ihdr->saddr    = htonl(attr->ip_src);
    ihdr->daddr    = htonl(attr->ip_dst);
    ihdr->tos      = 0;
    ihdr->id       = 0;
    ip_send_check(ihdr);

    shdr          = skb_put(skb, sizeof(*shdr));
    shdr->version = 0;
    shdr->magic   = cpu_to_be64(SSTAR_GMAC_TEST_PKT_MAGIC);
    attr->id      = sstar_gmac_test_next_id;
    shdr->id      = sstar_gmac_test_next_id++;

    if (attr->size)
        skb_put(skb, attr->size);
    if (attr->max_size && (attr->max_size > skb->len))
        skb_put(skb, attr->max_size - skb->len);

    skb->csum      = 0;
    skb->ip_summed = CHECKSUM_PARTIAL;
    if (attr->tcp)
    {
        thdr->check      = ~tcp_v4_check(skb->len, ihdr->saddr, ihdr->daddr, 0);
        skb->csum_start  = skb_transport_header(skb) - skb->head;
        skb->csum_offset = offsetof(struct tcphdr, check);
    }
    else
    {
        udp4_hwcsum(skb, ihdr->saddr, ihdr->daddr);
    }

    skb->protocol = htons(ETH_P_IP);
    skb->pkt_type = PACKET_HOST;
    skb->dev      = priv->dev;

    if (attr->timestamp)
        skb->tstamp = ns_to_ktime(attr->timestamp);

    return skb;
}

static int sstar_gmac_test_loopback_validate(struct sk_buff *skb, struct net_device *ndev, struct packet_type *pt,
                                             struct net_device *orig_ndev)
{
    struct sstar_gmac_test_priv *tpriv = pt->af_packet_priv;
    unsigned char *              src   = tpriv->packet->src;
    unsigned char *              dst   = tpriv->packet->dst;
    struct sstargmachdr *        shdr;
    struct ethhdr *              ehdr;
    struct udphdr *              uhdr;
    struct tcphdr *              thdr;
    struct iphdr *               ihdr;

    skb = skb_unshare(skb, GFP_ATOMIC);
    if (!skb)
        goto out;

    if (skb_linearize(skb))
        goto out;
    if (skb_headlen(skb) < (SSTAR_GMAC_TEST_PKT_SIZE - ETH_HLEN))
        goto out;

    ehdr = (struct ethhdr *)skb_mac_header(skb);
    if (dst)
    {
        if (!ether_addr_equal_unaligned(ehdr->h_dest, dst))
            goto out;
    }
    if (tpriv->packet->sarc)
    {
        if (!ether_addr_equal_unaligned(ehdr->h_source, ehdr->h_dest))
            goto out;
    }
    else if (src)
    {
        if (!ether_addr_equal_unaligned(ehdr->h_source, src))
            goto out;
    }

    ihdr = ip_hdr(skb);
    if (tpriv->double_vlan)
        ihdr = (struct iphdr *)(skb_network_header(skb) + 4);

    if (tpriv->packet->tcp)
    {
        if (ihdr->protocol != IPPROTO_TCP)
            goto out;

        thdr = (struct tcphdr *)((u8 *)ihdr + 4 * ihdr->ihl);
        if (thdr->dest != htons(tpriv->packet->dport))
            goto out;

        shdr = (struct sstargmachdr *)((u8 *)thdr + sizeof(*thdr));
    }
    else
    {
        if (ihdr->protocol != IPPROTO_UDP)
            goto out;

        uhdr = (struct udphdr *)((u8 *)ihdr + 4 * ihdr->ihl);
        if (uhdr->dest != htons(tpriv->packet->dport))
            goto out;

        shdr = (struct sstargmachdr *)((u8 *)uhdr + sizeof(*uhdr));
    }

    if (shdr->magic != cpu_to_be64(SSTAR_GMAC_TEST_PKT_MAGIC))
        goto out;
    if (tpriv->packet->exp_hash && !skb->hash)
        goto out;
    if (tpriv->packet->id != shdr->id)
        goto out;

    tpriv->ok = true;
    complete(&tpriv->comp);
out:
    kfree_skb(skb);
    return 0;
}

static int __sstar_gmac_test_loopback(struct stmmac_priv *priv, struct sstar_gmac_packet_attrs *attr)
{
    struct sstar_gmac_test_priv *tpriv;
    struct sk_buff *             skb = NULL;
    int                          ret = 0;

    tpriv = kzalloc(sizeof(*tpriv), GFP_KERNEL);
    if (!tpriv)
        return -ENOMEM;

    tpriv->ok = false;
    init_completion(&tpriv->comp);

    tpriv->pt.type           = htons(ETH_P_IP);
    tpriv->pt.func           = sstar_gmac_test_loopback_validate;
    tpriv->pt.dev            = priv->dev;
    tpriv->pt.af_packet_priv = tpriv;
    tpriv->packet            = attr;

    if (!attr->dont_wait)
        dev_add_pack(&tpriv->pt);

    skb = sstar_test_get_udp_skb(priv, attr);
    if (!skb)
    {
        ret = -ENOMEM;
        goto cleanup;
    }

    ret = dev_direct_xmit(skb, attr->queue_mapping);
    if (ret)
        goto cleanup;

    if (attr->dont_wait)
        goto cleanup;

    if (!attr->timeout)
        attr->timeout = SSTAR_GMAC_LB_TIMEOUT;

    wait_for_completion_timeout(&tpriv->comp, attr->timeout);
    ret = tpriv->ok ? 0 : -ETIMEDOUT;

cleanup:
    if (!attr->dont_wait)
        dev_remove_pack(&tpriv->pt);
    kfree(tpriv);
    return ret;
}

static int proc_gmac_loopback_test_show(struct seq_file *m, void *v)
{
    printk("perform loopback test\n");
    printk("echo <type> /proc/gmac_0/loopback\n");
    printk("<type> : \"mac\" or \"phy\"\n");
    return 0;
}

static int proc_gmac_loopback_test_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_gmac_loopback_test_show, PDE_DATA(inode));
}

static ssize_t proc_gmac_loopback_test_write(struct file *file, const char __user *buf, size_t len, loff_t *pos)
{
#define WAIT_LINK_UP_TIME  5000 // 5s
#define WAIT_LINK_UP_DELAY 100  // 100 ms
    u32                            wait_linkup_cnt = WAIT_LINK_UP_TIME / WAIT_LINK_UP_DELAY;
    struct sstar_dwmac *           dwmac           = ((struct seq_file *)file->private_data)->private;
    struct net_device *            ndev            = dev_get_drvdata(dwmac->dev);
    struct stmmac_priv *           priv            = netdev_priv(ndev);
    struct phy_device *            phydev          = priv->dev->phydev;
    struct sstar_gmac_packet_attrs attr            = {};
    char                           strbuf[64]      = {0};
    int                            loopback_type   = 0; // PHY LOOPBACK
    int                            ret             = 0;
    u16                            reg0_bk         = 0;
    int                            i               = 0;
    int                            pass            = 1;

    if (len > sizeof(strbuf) - 1)
    {
        printk(KERN_ERR "command len is too long!\n");
        return len;
    }

    if (copy_from_user(strbuf, buf, len))
    {
        return -EFAULT;
    }

    if (strcmp(strbuf, "mac") == 0)
    {
        loopback_type = 1; // MAC LOOPBACK
        printk("\r\nstart mac loopback!\n");
    }
    else
    {
        loopback_type = 0; // PHY LOOPBACK
        printk("\r\nstart phy loopback!\n");
    }

    if (loopback_type == 0)
    {
        ret = mdiobus_read(priv->mii, phydev->mdio.addr, MII_BMCR);
        if (ret < 0)
            printk("%s %d err=0x%x\n", __FUNCTION__, __LINE__, ret);

        reg0_bk = ret;

        ret = phy_loopback(priv->dev->phydev, true);
        if (ret)
            return len;
        ret = genphy_setup_forced(priv->dev->phydev);
    }
    else
    {
        ret = stmmac_set_mac_loopback(priv, priv->ioaddr, true);
    }

    if (ret)
        return len;

    mdelay(100);

    attr.dst = priv->dev->dev_addr;

    for (i = 0; i < 100; i++)
    {
        ret = __sstar_gmac_test_loopback(priv, &attr);

        if (ret)
        {
            pass = 0;
            printk("loopback failed!\n");
            break;
        }
    }

    if (pass)
        printk("loopback passed!\n");

    if (loopback_type == 0)
    {
        phy_loopback(priv->dev->phydev, false);
        mdiobus_write(priv->mii, phydev->mdio.addr, MII_BMCR, reg0_bk);
    }
    else
    {
        stmmac_set_mac_loopback(priv, priv->ioaddr, false);
    }

    while (wait_linkup_cnt)
    {
        ret = mdiobus_read(priv->mii, phydev->mdio.addr, MII_BMSR);
        if (ret < 0)
            printk("%s %d err=0x%x\n", __FUNCTION__, __LINE__, ret);
        if (ret & BMSR_LSTATUS)
            break;
        msleep(WAIT_LINK_UP_DELAY);
        wait_linkup_cnt--;
    }

    return len;
}

static const struct proc_ops proc_gmac_loopback_test_ops = {.proc_open    = proc_gmac_loopback_test_open,
                                                            .proc_read    = seq_read,
                                                            .proc_write   = proc_gmac_loopback_test_write,
                                                            .proc_lseek   = seq_lseek,
                                                            .proc_release = single_release};

void dwmac_sstar_test(struct sstar_dwmac *dwmac)
{
    proc_create_data("reprobe", 0, dwmac->gmac_root_dir, &proc_gmac_reprobe_test_ops, dwmac);
    proc_create_data("loopback", 0, dwmac->gmac_root_dir, &proc_gmac_loopback_test_ops, dwmac);
}