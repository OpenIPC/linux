/*
 *
 * Copyright (C) 2011 Dante Su <dantesu@faraday-tech.com>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/version.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/mii.h>
#include <linux/phy.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/timer.h>

#include <linux/crc32.h>
#include <linux/inet_lro.h>
#include <linux/if_ether.h>
#include <linux/if_vlan.h>
#include <linux/if_link.h>
#include <linux/ethtool.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/tcp.h>
#include <net/ip6_checksum.h>

#include <asm/io.h>
#include <mach/ftpmu010.h>
#include "ftmac110.h"

#define CFG_XDESC_NUM
#define CFG_TXDESC_NUM           CONFIG_FTMAC110_NDESC_TX
#define CFG_RXDESC_NUM           CONFIG_FTMAC110_NDESC_RX
#define CFG_XBUF_SIZE            1536

#define CFG_NAPI_WEIGHT          CONFIG_FTMAC110_NDESC_RX
#define CFG_LRO_MAX_AGGR         CFG_NAPI_WEIGHT
#define CFG_LRO_MAX_DESC         (CFG_LRO_MAX_AGGR >> 1)

#define DRV_NAME                 "ftmac110"     //name must match with platform.c
#define DRV_VERSION              "1.0rc2"
#define DRV_RELDATE              "May 24, 2011"

#ifndef EMAC_REG32
#define EMAC_REG32(priv, off)    *(volatile uint32_t *)((priv)->iobase + (off))
#endif

static int mac_fd = 0;

typedef struct ftmac110_priv {
    struct net_device_stats stats;
    spinlock_t lock;
#ifdef CONFIG_FTMAC110_PHYMON
    struct timer_list timer;
#endif
#ifdef CONFIG_FTMAC110_NAPI
    struct napi_struct napi;
# ifdef CONFIG_FTMAC110_NAPI_LRO
    struct net_lro_mgr lro_mgr;
    struct net_lro_desc lro_desc[CFG_LRO_MAX_DESC];
# endif
#endif                          /* #ifdef CONFIG_FTMAC110_NAPI */
    struct net_device *netdev;
    struct device *dev;

    uint32_t irq;
    uint32_t irqmask;
    uint32_t iobase;
    uint32_t maccr;

    void *desc_pool;

    ftmac110_rxdesc_t *rx_descs;
    dma_addr_t rx_descs_dma;
    uint32_t rx_insert;
    uint32_t rx_remove;
    uint32_t rx_free;

    ftmac110_txdesc_t *tx_descs;
    dma_addr_t tx_descs_dma;
    uint32_t tx_insert;
    uint32_t tx_remove;
    uint32_t tx_free;

    struct mii_if_info mii_if;

    struct {
        uint32_t oui:24;
        uint32_t model:6;
        uint32_t rsvd:30;
        uint32_t rev:4;
    } phyid;

} ftmac110_priv_t;

static void wait_status(int millisecond)
{
        if (in_interrupt() || in_atomic()){
            mdelay(millisecond);
        }else{           
            set_current_state(TASK_INTERRUPTIBLE);
            schedule_timeout((HZ / 1000) * millisecond);
        }
} 

/* 
 * Local driver functions
 */
static int mdio_read(struct net_device *dev, int phy_id, int phy_reg)
{
    ftmac110_priv_t *priv = netdev_priv(dev);
    uint32_t tmp, i;

    tmp = MIIREG_READ | (phy_id << MIIREG_PHYADDR_SHIFT)
        | (phy_reg << MIIREG_PHYREG_SHIFT)
        | 0x30000000;

    EMAC_REG32(priv, PHYCR_REG) = tmp;

    for (i = 0; i < 100; i++) {
        tmp = EMAC_REG32(priv, PHYCR_REG);
        if ((tmp & MIIREG_READ) == 0) 
           return (int)(tmp & 0xFFFF);  
        
        wait_status(10);
    }

    printk("mdio read timed out\n");
    return -EIO;
}

static void mdio_write(struct net_device *dev, int phy_id, int phy_reg, int val)
{
    ftmac110_priv_t *priv = netdev_priv(dev);
    uint32_t tmp, i;

    tmp = MIIREG_WRITE | (phy_id << MIIREG_PHYADDR_SHIFT)
        | (phy_reg << MIIREG_PHYREG_SHIFT)
        | 0x30000000;

    EMAC_REG32(priv, PHYDATA_REG) = val;
    EMAC_REG32(priv, PHYCR_REG) = tmp;

    for (i = 0; i < 100; i++) {
        tmp = EMAC_REG32(priv, PHYCR_REG);
        
        if ((tmp & MIIREG_WRITE) == 0) 
            return;   
                 
        wait_status(10);
    }
	
    printk("mdio write timed out\n");   
    return;    
}

#ifdef CONFIG_PLATFORM_GM8126
/*
 * GM8126
 */
#define GM_PHY

static pmuReg_t pmu_reg_8126[] = {
    {0x38, (0x1 << 8) | (0x1 << 10), (0x1 << 8) | (0x1 << 10), 0, 0},
    {0x58, (0x1 << 31), (0x1 << 31), 0, 0},
    {0x5C, (0xF << 28), (0xF << 28), 0, 0},
    {0x60, (0xFF << 22), (0xFF << 22), 0, 0},
    {0x64, 0xFFFFFFFF, 0, 0, 0},
    {0x6c, (0x1 << 10) | (0x1 << 11) | (0x1 << 18), (0x1 << 10) | (0x1 << 11) | (0x1 << 18), 0x0,
     0x0},
    {0x74, (0x3F << 26), (0x3F << 26), 0, 0},
    {0x84, 0xFFFFFFFF, 0xFFFFFFFF, 0, 0},
};

static pmuRegInfo_t pmu_reg_info_8126 = {
    "MAC_8126",
    ARRAY_SIZE(pmu_reg_8126),
    ATTR_TYPE_PLL1,
    &pmu_reg_8126[0]
};

static int ftmac110_pmu8126_init(void)
{
    int div = 0, tmp;

    mac_fd = ftpmu010_register_reg(&pmu_reg_info_8126);
    if (mac_fd < 0) {
        printk("MAC: %s fail \n", __FUNCTION__);
        return -1;
    }
    //MHz change to hz
    tmp = ftpmu010_get_attr(ATTR_TYPE_PMUVER);
    tmp &= 0xF0;
    switch (tmp) {
    case 0x10:
        div = ftpmu010_clock_divisor(mac_fd, 100000000, 1) - 1;
        break;
    case 0x20:
#ifdef RMII
        div = ftpmu010_clock_divisor(mac_fd, 50000000, 1) - 1;  //RMII 50MHz
#else
        div = ftpmu010_clock_divisor(mac_fd, 25000000, 1) - 1;  //MII 25MHz
#endif
        break;
    default:
        printk("chip ID not match, use default div = %d\n", div);
        break;
    }
    if (!div)
        div = 1;
    //printk("MAC clock div = %d, PLL1 clk = %d\n",div,ftpmu010_get_attr(ATTR_TYPE_PLL1));

    //MAC PHY pin mux
    ftpmu010_write_reg(mac_fd, 0x5C, (0x5 << 28), (0xF << 28));
    ftpmu010_write_reg(mac_fd, 0x64, 0x55555555, 0xFFFFFFFF);

#ifdef GM_PHY
    /* MAC clock on(bit8) and PHY clock on(bit10) */
    ftpmu010_write_reg(mac_fd, 0x38, 0, (0x1 << 8) | (0x1 << 10));
    /* set MAC CLK div */
    if (tmp == 0x10) {
        printk("Not support test chip\n");
    } else
        ftpmu010_write_reg(mac_fd, 0x74, (div << 26), (0x3F << 26));

    /* MII clock enable */
#ifdef RMII
    ftpmu010_write_reg(mac_fd, 0x6c, (0x1 << 10) | (0x1 << 18),
                       (0x1 << 10) | (0x1 << 11) | (0x1 << 18) | (0x1 << 19));
#else
    ftpmu010_write_reg(mac_fd, 0x6c, (0x1 << 10) | (0x1 << 18),
                       (0x1 << 10) | (0x1 << 11) | (0x1 << 18));
#endif
#else
    /* MII pinmux */
    ftpmu010_write_reg(mac_fd, 0x60, (0x55 << 22), (0xFF << 22));

    /* MAC clock on(bit8) and PHY clock on(bit10) */
    ftpmu010_write_reg(mac_fd, 0x38, (0x1 << 10), (0x1 << 8) | (0x1 << 10));
    /* set MAC CLK div */
    ftpmu010_write_reg(mac_fd, 0x74, (div << 26), (0x3F << 26));

    /* MII always enable */
    ftpmu010_write_reg(mac_fd, 0x58, (0x1 << 31), (0x1 << 31));
    /* Select MII amd bypass internal PHY */
#ifdef RMII
    ftpmu010_write_reg(mac_fd, 0x6C, (0x1 << 10) | (0x1 << 11) | (0x1 << 18),
                       (0x1 << 10) | (0x1 << 11) | (0x1 << 18) | (0x1 << 19));
#else
    ftpmu010_write_reg(mac_fd, 0x6C, (0x1 << 10) | (0x1 << 11) | (0x1 << 18),
                       (0x1 << 10) | (0x1 << 11) | (0x1 << 18));
#endif
#endif
    return 0;
}
#endif

static uint32_t ftmac110_phyqry(ftmac110_priv_t * priv)
{
    uint32_t retry;
    uint32_t maccr;
    int adv, lpa, tmp, media;

    maccr = Speed_100_bit | FULLDUP_bit;

    // 1. check link status
    for (retry = 3000000; retry; --retry) {
        if (mii_link_ok(&priv->mii_if))
            break;
    }
    if (!retry) {
        netif_carrier_off(priv->netdev);
        printk("ftmac110: link down\n");
        goto exit;
    }
    netif_carrier_on(priv->netdev);

    // 2. check A/N status
    for (retry = 3000000; retry; --retry) {
        tmp = mdio_read(priv->netdev, priv->mii_if.phy_id, MII_BMSR);
        if (tmp & BMSR_ANEGCOMPLETE)
            break;
    }
    if (!retry) {
        printk("ftmac110: A/N failed\n");
        goto exit;
    }
    adv = mdio_read(priv->netdev, priv->mii_if.phy_id, MII_ADVERTISE);
    lpa = mdio_read(priv->netdev, priv->mii_if.phy_id, MII_LPA);
    media = mii_nway_result(lpa & adv);

    // 3. build MACCR with the PHY status
    maccr = 0;

    // 10/100Mbps Detection
    if (media & ADVERTISE_100FULL)
        maccr |= Speed_100_bit | FULLDUP_bit;
    else if (media & ADVERTISE_100HALF)
        maccr |= Speed_100_bit | ENRX_IN_HALFTX_bit;
    else if (media & ADVERTISE_10FULL)
        maccr |= FULLDUP_bit;
    else if (media & ADVERTISE_10HALF)
        maccr |= ENRX_IN_HALFTX_bit;

    // 3-2. Vendor ID
    /*
       tmp = mdio_read(priv->netdev, priv->mii_if.phy_id, MII_PHYSID2);
       priv->phyid.oui   = (mdio_read(priv->netdev, priv->mii_if.phy_id, MII_PHYSID1) << 6) | (tmp >> 10);
       priv->phyid.model = (tmp >> 4) & 0x3F;
       priv->phyid.rev   = (tmp & 0x0F);    
       printk("ftmac110: PHY OUI=0x%06X, Model=0x%02X, Rev=0x%02X\n",
       priv->phyid.oui, priv->phyid.model, priv->phyid.rev); 
     */

    printk("ftmac110: %d Mbps, %s\r\n",
           (maccr & Speed_100_bit) ? 100 : 10, (maccr & FULLDUP_bit) ? "Full" : "half");

    maccr |= CRC_APD_bit | RX_BROADPKT_bit | RX_RUNT_bit
        | RCV_EN_bit | XMT_EN_bit | RDMA_EN_bit | XDMA_EN_bit;

  exit:
    return maccr;
}

#ifdef CONFIG_FTMAC110_PHYMON
static void ftmac110_timer(unsigned long d)
{
    ftmac110_priv_t *priv = (ftmac110_priv_t *) d;

    if (mii_link_ok(&priv->mii_if)) {
        if (!netif_carrier_ok(priv->netdev)) {
            printk("ftmac110: link up\n");
            spin_lock(&priv->lock);
            priv->maccr = ftmac110_phyqry(priv);
            EMAC_REG32(priv, MACCR_REG) = priv->maccr;
            spin_unlock(&priv->lock);
        }
    } else {
        if (netif_carrier_ok(priv->netdev)) {
            netif_carrier_off(priv->netdev);
            printk("ftmac110: link down\n");
        }
    }

    priv->timer.data = (unsigned long)priv;
    priv->timer.function = ftmac110_timer;
    priv->timer.expires = jiffies + msecs_to_jiffies(250);
    add_timer(&priv->timer);
}
#endif /* #ifdef CONFIG_FTMAC110_PHYMON */

static int ftmac110_rx_alloc(ftmac110_priv_t * priv)
{
    struct sk_buff *skb;
    volatile ftmac110_rxdesc_t *rxd;

    while (priv->rx_free < CFG_RXDESC_NUM) {
        rxd = priv->rx_descs + priv->rx_insert;
        if (rxd->owner == 1)    // owned by HW
            break;

        skb = netdev_alloc_skb(priv->netdev, CFG_XBUF_SIZE + NET_IP_ALIGN);
        if (skb == NULL) {
            printk("ftmac110: out of memory\n");
            return -1;
        }
        skb_reserve(skb, NET_IP_ALIGN);

        //printk("ftmac110: rxd[%d]: buf@0x%08X\n", priv->rx_insert, (uint32_t)skb->tail);
        priv->rx_insert = (priv->rx_insert + 1) % CFG_RXDESC_NUM;
        priv->rx_free++;

        rxd->skb = skb;
        rxd->buf = dma_map_single(priv->dev, skb->tail, CFG_XBUF_SIZE, DMA_FROM_DEVICE);
        rxd->len = 0;
        rxd->bufsz = CFG_XBUF_SIZE;
        rxd->owner = 1;         // owned by HW
    }

    return 0;
}

static int ftmac110_set_mac_address(struct net_device *dev, void *p)
{
    struct ftmac110_priv *priv = netdev_priv(dev);
    unsigned char *a = dev->dev_addr;
    struct sockaddr *addr = p;

    if (addr != NULL) {
        if (!is_valid_ether_addr(addr->sa_data))
            return -EADDRNOTAVAIL;
        memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);
    }
    //spin_lock_irq(&priv->lock);

    EMAC_REG32(priv, MAC_MADR_REG) = a[1] | (a[0] << 8);
    EMAC_REG32(priv, MAC_LADR_REG) = a[5] | (a[4] << 8) | (a[3] << 16) | (a[2] << 24);

    //spin_unlock_irq(&priv->lock);

    return 0;
}

static int ftmac110_up(ftmac110_priv_t * priv)
{
    uint32_t maccr;

    // 1. MAC reset
    EMAC_REG32(priv, MACCR_REG) = SW_RST_bit;
    do {
        maccr = EMAC_REG32(priv, MACCR_REG);
    } while (maccr & SW_RST_bit);

    // 2. PHY status query & Fix up the MACCR value 
    maccr = ftmac110_phyqry(priv);
    priv->maccr = maccr;
    priv->irqmask = XPKT_LOST_bit | NOTXBUF_bit | XPKT_FINISH_bit
        | RPKT_LOST_bit | NORXBUF_bit | RPKT_FINISH_bit | PHYSTS_CHG_bit;

    // 3. MAC address setup
    ftmac110_set_mac_address(priv->netdev, NULL);

    // 4. MAC registers setup
    EMAC_REG32(priv, RXR_BADR_REG) = priv->rx_descs_dma;
    EMAC_REG32(priv, TXR_BADR_REG) = priv->tx_descs_dma;
    EMAC_REG32(priv, ITC_REG) = 0x0000FFFF;
    EMAC_REG32(priv, APTC_REG) = 0x00000011;
    EMAC_REG32(priv, DBLAC_REG) = 0x00000390;
    EMAC_REG32(priv, ISR_REG) = 0x000003FF;
    EMAC_REG32(priv, IMR_REG) = priv->irqmask;
    EMAC_REG32(priv, MACCR_REG) = priv->maccr;

    EMAC_REG32(priv, FCR_REG) |= 0x1;   //Flow Control
    EMAC_REG32(priv, BPR_REG) |= 0x1;

    return 0;
}

static int ftmac110_down(ftmac110_priv_t * priv)
{
    EMAC_REG32(priv, IMR_REG) = 0;
    EMAC_REG32(priv, MACCR_REG) = 0;
    return 0;
}

static int ftmac110_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    ftmac110_priv_t *priv = netdev_priv(dev);
    volatile ftmac110_txdesc_t *txd;
    unsigned long flags;

    if (skb == NULL)            /* NULL skb directly return */
        return 0;

    if (skb->len >= CFG_XBUF_SIZE) {    /* Packet too long, drop it */
        dev_kfree_skb(skb);
        return 0;
    }

    /* Critical Section */
    spin_lock_irqsave(&priv->lock, flags);

    /* TX resource check */
    if (priv->tx_free == 0) {
        netif_stop_queue(dev);
        priv->stats.tx_dropped++;
        EMAC_REG32(priv, TXPD_REG) = 0xffffffff;        // kick-off Tx DMA
        spin_unlock_irqrestore(&priv->lock, flags);
        //printk("ftmac110: out of txd\n");
        return 1;
    }

    /* Statistic Counter */
    priv->stats.tx_packets++;
    priv->stats.tx_bytes += skb->len;

    /* Set TX descriptor & Transmit it */
    priv->tx_free--;
    txd = priv->tx_descs + priv->tx_insert;
    priv->tx_insert = (priv->tx_insert + 1) % CFG_TXDESC_NUM;

    txd->skb = skb;
    txd->len = skb->len < 64 ? 64 : skb->len;
    txd->buf = dma_map_single(priv->dev, skb->data, skb->len, DMA_TO_DEVICE);
    txd->lts = 1;
    txd->fts = 1;
    txd->owner = 1;

    EMAC_REG32(priv, TXPD_REG) = 0xffffffff;    // kick-off Tx DMA

    dev->trans_start = jiffies;
    spin_unlock_irqrestore(&priv->lock, flags);

    //printk("ftmac110: send..1\n");

    return 0;
}

int phy_read_duplex(struct net_device *dev)
{
    int bmcr;
    ftmac110_priv_t *priv = netdev_priv(dev);

    bmcr = mdio_read(priv->netdev, priv->mii_if.phy_id, MII_BMCR);
    if (bmcr & BMCR_FULLDPLX) {
        return 1;               //Full
    } else {
        return 0;               //Half
    }
}

int phy_read_speed(struct net_device *dev)
{
    int bmcr;
    ftmac110_priv_t *priv = netdev_priv(dev);

    bmcr = mdio_read(priv->netdev, priv->mii_if.phy_id, MII_BMCR);
    if (bmcr & BMCR_SPEED100)
        return 1;
    else
        return 0;
}

static void ftmac110_link_change(struct net_device *dev)
{
    unsigned int data;
    ftmac110_priv_t *priv = netdev_priv(dev);

    data = mdio_read(priv->netdev, priv->mii_if.phy_id, MII_BMSR);

    if (data & BMSR_LSTATUS)
        netif_carrier_on(dev);
    else
        netif_carrier_off(dev);

    data = EMAC_REG32(priv, MACCR_REG);
    if (phy_read_speed(dev)) {
        EMAC_REG32(priv, MACCR_REG) = data | Speed_100_bit;
        printk("Link Change: <SPEED100>\n");
    } else {
        EMAC_REG32(priv, MACCR_REG) = data & (~Speed_100_bit);
        printk("Link Change: <SPEED10>\n");
    }

    if (phy_read_duplex(dev)) {
        EMAC_REG32(priv, MACCR_REG) = data | FULLDUP_bit;
        printk("Link Change: <FULL>\n");
    } else {
        EMAC_REG32(priv, MACCR_REG) = data & (~FULLDUP_bit);
        printk("Link Change: <HALF>\n");
    }

#ifdef GM_PHY
#if 0                           //for customer tune 10M mode PHY TX curren

    val = mdio_read(dev, 1, 0x11) & 0xFFCF;
    //mdio_write(dev, 1, 0x11, val | 0x10);//improve 10M mode PHY TX current 25uA
    //mdio_write(dev, 1, 0x11, val | 0x20);//improve 10M mode PHY TX current 40uA
    //mdio_write(dev, 1, 0x11, val | 0x30);//improve 10M mode PHY TX current 65uA
    //mdio_write(dev, 1, 0x11, val);//default
    //printk("PHY reg 0x11 = %#x\n",mdio_read(dev, 1, 0x11));
#endif

#if 0                           //for customer tune 100M mode PHY TX curren
    //mdio_write(dev, 1, 0x12, 0x0E49);//improve 100M mode PHY TX current 6%
    //mdio_write(dev, 1, 0x12, 0x0C49);//improve 100M mode PHY TX current 4%
    //mdio_write(dev, 1, 0x12, 0x0A49);//improve 100M mode PHY TX current 2%
    //mdio_write(dev, 1, 0x12, 0x0849);//default
    //printk("PHY reg 0x12 = %#x\n",mdio_read(dev, 1, 0x12));
#endif
#endif
}

static irqreturn_t ftmac110_interrupt(int irq, void *dev_id)
{
    uint32_t stat;
    unsigned long flags;
    struct net_device *dev = dev_id;
    ftmac110_priv_t *priv = netdev_priv(dev);

    spin_lock_irqsave(&priv->lock, flags);

    // fetch&clear interrupt status
    stat = EMAC_REG32(priv, ISR_REG);
    // disable interrupt
    EMAC_REG32(priv, IMR_REG) = 0;

    //printk("ftmac110: isr=0x%08X\n", stat);

    // 1. rx drop
    if (stat & NORXBUF_bit)
        priv->stats.rx_dropped++;

    // 3. tx done
    if (stat & (XPKT_OK_bit | XPKT_LOST_bit | NOTXBUF_bit)) {
        volatile ftmac110_txdesc_t *txd;
        while (priv->tx_free < CFG_TXDESC_NUM) {
            txd = priv->tx_descs + priv->tx_remove;
            if (txd->owner)     // owned by HW
                break;
            if (txd->error)
                priv->stats.tx_errors++;
            dev_kfree_skb_irq(txd->skb);
            txd->skb = NULL;
            priv->tx_free++;
            priv->tx_remove = (priv->tx_remove + 1) % CFG_TXDESC_NUM;
        }
    }
    // 2. rx done
    if (stat & (RPKT_FINISH_bit | RPKT_LOST_bit)) {
#ifdef CONFIG_FTMAC110_NAPI
        if (napi_schedule_prep(&priv->napi)) {
            priv->irqmask &= ~(RPKT_LOST_bit | NORXBUF_bit | RPKT_FINISH_bit);
            __napi_schedule(&priv->napi);
        }
#else
        struct sk_buff *skb;
        volatile ftmac110_rxdesc_t *rxd;
        while (priv->rx_free > 0) {
            rxd = priv->rx_descs + priv->rx_remove;
            if (rxd->owner == 1)        // owned by HW
                break;
            skb = rxd->skb;
            rxd->skb = NULL;
            if (rxd->error) {
                priv->stats.rx_errors++;
                dev_kfree_skb_irq(skb);
            } else {
                skb->dev = dev;
                skb_put(skb, rxd->len);
                skb->protocol = eth_type_trans(skb, dev);
                netif_rx(skb);  /* Send to upper layer */
                priv->stats.rx_packets++;
                priv->stats.rx_bytes += rxd->len;
            }
            priv->rx_free--;
            priv->rx_remove = (priv->rx_remove + 1) % CFG_RXDESC_NUM;
        }
#endif
    }
    // 4. link change
    if (stat & PHYSTS_CHG_bit)
        ftmac110_link_change(dev);

    if (priv->tx_free > 0)
        netif_wake_queue(dev);

    if (priv->rx_free < CFG_RXDESC_NUM)
        ftmac110_rx_alloc(priv);

    // enable interrupt
    EMAC_REG32(priv, IMR_REG) = priv->irqmask;

    spin_unlock_irqrestore(&priv->lock, flags);
    return IRQ_HANDLED;
}

static inline int ftmac110_mcast_hash(int len, const uint8_t * data)
{
    return ether_crc(len, data) >> 26;
}

#if 0
static void ftmac110_set_rx_mode(struct net_device *dev)
{
    uint32_t maccr;
    unsigned long flags;
    struct ftmac110_priv *priv = netdev_priv(dev);

    spin_lock_irqsave(&priv->lock, flags);

    maccr = priv->maccr & ~(RCV_ALL_bit | RX_MULTIPKT_bit | HT_MULTI_EN_bit);

    if (dev->flags & IFF_PROMISC) {
        maccr |= RCV_ALL_bit;
    } else if (dev->flags & IFF_ALLMULTI) {
        maccr |= RX_MULTIPKT_bit;
    } else {
        uint32_t ht[2] = { 0, 0 };
        struct dev_mc_list *mclist;
        int i;

        for (i = 0, mclist = dev->mc_list; mclist && i < dev->mc_count; i++, mclist = mclist->next) {
            int hash = ftmac110_mcast_hash(ETH_ALEN, mclist->dmi_addr);
            /*
               printk("ftmac110: %02X:%02X:%02X:%02X:%02X:%02X=%d\n",
               ha->addr[0], ha->addr[1], ha->addr[2],
               ha->addr[3], ha->addr[4], ha->addr[5],
               hash);
             */
            if (hash >= 32) {
                ht[1] |= (1 << (hash - 32));
            } else {
                ht[0] |= (1 << (hash - 0));
            }
        }

        if (ht[0] || ht[1]) {
            maccr |= HT_MULTI_EN_bit;
            EMAC_REG32(priv, MAHT0_REG) = ht[0];
            EMAC_REG32(priv, MAHT1_REG) = ht[1];
        }
    }

    priv->maccr = maccr;
    EMAC_REG32(priv, MACCR_REG) = priv->maccr;

    spin_unlock_irqrestore(&priv->lock, flags);

    //printk("ftmac110: ftmac110_set_rx_mode...0x%08X\n", maccr);
}
#endif

static int ftmac110_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
    struct ftmac110_priv *priv = netdev_priv(dev);
    unsigned long flags;
    int rc;

    spin_lock_irqsave(&priv->lock, flags);
    rc = generic_mii_ioctl(&priv->mii_if, if_mii(ifr), cmd, NULL);
    spin_unlock_irqrestore(&priv->lock, flags);
    return rc;
}

static struct net_device_stats *ftmac110_get_stats(struct net_device *dev)
{
    struct ftmac110_priv *priv = netdev_priv(dev);
    return &priv->stats;
}

static int ftmac110_phy_reset(void)
{
#ifdef GM_PHY
    //reset Faraday PHY, for N-Way
    printk("reset Faraday Internal PHY.\n");
    ftpmu010_write_reg(mac_fd, 0x84, 0, 0x1 | (0x3 << 4));
    wait_status(1);
    ftpmu010_write_reg(mac_fd, 0x84, 0x1, 0x1);
#endif
    return 0;
}

static int ftmac110_open(struct net_device *dev)
{
    struct ftmac110_priv *priv = netdev_priv(dev);
    int i;
    size_t sz;
    void *va;
    dma_addr_t pa;

    spin_lock_irq(&priv->lock);

    /* Request IRQ and Register interrupt handler */
    if (request_irq(priv->irq, ftmac110_interrupt, IRQF_SHARED, dev->name, dev)) {
        printk("ftmac110: failed to register irq\n");
        return -1;
    }

    /* Allocate Descriptor memory (16 bytes aligned) */
    sz = 16 + sizeof(ftmac110_txdesc_t) * CFG_TXDESC_NUM +
        sizeof(ftmac110_rxdesc_t) * CFG_RXDESC_NUM;
    va = dma_alloc_coherent(priv->dev, sz, &pa, GFP_KERNEL);
    if (!va) {
        printk("ftmac110: out of memory\n");
        return -ENOMEM;
    }
    memset(va, 0, sz);
    pa = (pa + 15) & 0xFFFFFFF0;
    va = (void *)(((uint32_t) va + 15) & 0xFFFFFFF0);

    priv->desc_pool = va;

    // Init tx ring
    priv->tx_descs = va;
    priv->tx_descs_dma = pa;
    for (i = 0; i < CFG_TXDESC_NUM; ++i) {
        priv->tx_descs[i].len = 0;
        priv->tx_descs[i].end = 0;
        priv->tx_descs[i].owner = 0;    // owned by SW
        //printk("ftmac110: txd[%d]@0x%08X\n", i, (uint32_t)&priv->tx_descs[i]);
    }
    priv->tx_descs[CFG_TXDESC_NUM - 1].end = 1;
    priv->tx_insert = priv->tx_remove = 0;
    priv->tx_free = CFG_TXDESC_NUM;

    // Init rx ring
    priv->rx_descs =
        (ftmac110_rxdesc_t *) ((uint32_t) va + sizeof(ftmac110_txdesc_t) * CFG_TXDESC_NUM);
    priv->rx_descs_dma = pa + sizeof(ftmac110_txdesc_t) * CFG_TXDESC_NUM;
    for (i = 0; i < CFG_RXDESC_NUM; ++i) {
        priv->rx_descs[i].end = 0;
        priv->rx_descs[i].owner = 0;    // owned by SW
        //printk("ftmac110: rxd[%d]@0x%08X\n", i, (uint32_t)&priv->rx_descs[i]);
    }
    priv->rx_descs[CFG_RXDESC_NUM - 1].end = 1;
    priv->rx_insert = priv->rx_remove = 0;
    priv->rx_free = 0;
    ftmac110_rx_alloc(priv);

    ftmac110_phy_reset();
    // chip enable
    ftmac110_up(priv);

#ifdef CONFIG_FTMAC110_NAPI
    napi_enable(&priv->napi);
#endif
    // netdev enable
    netif_start_queue(dev);

    spin_unlock_irq(&priv->lock);

#ifdef CONFIG_FTMAC110_PHYMON
    priv->timer.data = (unsigned long)priv;
    priv->timer.function = ftmac110_timer;
    priv->timer.expires = jiffies + msecs_to_jiffies(500);
    add_timer(&priv->timer);
#endif
    return 0;
}

static int ftmac110_close(struct net_device *dev)
{
    struct ftmac110_priv *priv = netdev_priv(dev);

#ifdef CONFIG_FTMAC110_PHYMON
    del_timer_sync(&priv->timer);
#endif

    spin_lock_irq(&priv->lock);

#ifdef CONFIG_FTMAC110_NAPI
    napi_disable(&priv->napi);
#endif

    netif_stop_queue(dev);

    ftmac110_down(priv);

    free_irq(priv->irq, dev);

    spin_unlock_irq(&priv->lock);
    return 0;
}

#ifdef CONFIG_FTMAC110_NAPI

# ifdef CONFIG_FTMAC110_NAPI_LRO
static int
ftmac110_get_skb_header(struct sk_buff *skb, void **iphdr, void **tcph, u64 * hdr_flags, void *priv)
{
    struct iphdr *iph;
    unsigned int ipl;

    skb_reset_network_header(skb);
    iph = ip_hdr(skb);
    ipl = ip_hdrlen(skb);

    /* check if ipv4 & tcp */
    if (iph->version != 4 || iph->protocol != IPPROTO_TCP)
        return -1;

    skb_set_transport_header(skb, ipl);

    /* check if ip header and tcp header are complete */
    if (ntohs(iph->tot_len) < ipl + tcp_hdrlen(skb))
        return -1;

    *iphdr = iph;
    *tcph = tcp_hdr(skb);
    *hdr_flags = LRO_IPV4 | LRO_TCP;

    return 0;
}
# endif /* #ifdef CONFIG_FTMAC110_NAPI_LRO */

static int ftmac110_rx_poll(struct napi_struct *napi, int budget)
{
    struct ftmac110_priv *priv = container_of(napi, struct ftmac110_priv, napi);
    struct net_device *dev = priv->netdev;
    int rx;

    struct sk_buff *skb;
    volatile ftmac110_rxdesc_t *rxd;

    rx = 0;
    while (priv->rx_free > 0) {
        rxd = priv->rx_descs + priv->rx_remove;
        if (rxd->owner == 1)    // owned by HW
            break;
        skb = rxd->skb;
        rxd->skb = NULL;
        if (rxd->error) {
            priv->stats.rx_errors++;
            dev_kfree_skb_irq(skb);
        } else {
            skb->dev = dev;
            skb_put(skb, rxd->len);
            skb->protocol = eth_type_trans(skb, dev);
#ifdef CONFIG_FTMAC110_NAPI_LRO
            lro_receive_skb(&priv->lro_mgr, skb, priv);
#else
            netif_receive_skb(skb);     /* Send to upper layer */
#endif
            priv->stats.rx_packets++;
            priv->stats.rx_bytes += rxd->len;
        }
        priv->rx_free--;
        priv->rx_remove = (priv->rx_remove + 1) % CFG_RXDESC_NUM;

        if (++rx >= budget)
            break;
    }

#ifdef CONFIG_FTMAC110_NAPI_LRO
    lro_flush_all(&priv->lro_mgr);
#endif

    /* if we did not reach work limit, then we're done with
     * this round of polling
     */
    if (rx < budget) {
        unsigned long flags;

        spin_lock_irqsave(&priv->lock, flags);
        priv->irqmask |= (RPKT_LOST_bit | NORXBUF_bit | RPKT_FINISH_bit);
        EMAC_REG32(priv, IMR_REG) = priv->irqmask;
        __napi_complete(napi);
        spin_unlock_irqrestore(&priv->lock, flags);
    }

    return 0;
}

#endif /* #ifdef CONFIG_FTMAC110_NAPI */

/******************************************************************************
 * ethtool stuff
 *****************************************************************************/
#if 0
static int ftmac110_get_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
    struct ftmac110_priv *priv = netdev_priv(dev);
    unsigned long flags;
    int rc;

    spin_lock_irqsave(&priv->lock, flags);
    rc = mii_ethtool_gset(&priv->mii, cmd);
    spin_unlock_irqrestore(&priv->lock, flags);

    return rc;
}

static int ftmac110_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
    struct ftmac110_priv *priv = netdev_priv(dev);
    unsigned long flags;
    int rc;

    spin_lock_irqsave(&priv->lock, flags);
    rc = mii_ethtool_sset(&priv->mii, cmd);
    spin_unlock_irqrestore(&priv->lock, flags);

    return rc;
}
#endif
static void ftmac110_get_drvinfo(struct net_device *dev, struct ethtool_drvinfo *info)
{
    strcpy(info->driver, DRV_NAME);
    strcpy(info->version, DRV_VERSION);
}

static int ftmac110_nway_reset(struct net_device *dev)
{
    struct ftmac110_priv *priv = netdev_priv(dev);
    unsigned long flags;
    int rc;

    spin_lock_irqsave(&priv->lock, flags);
    rc = mii_nway_restart(&priv->mii_if);
    spin_unlock_irqrestore(&priv->lock, flags);
    return rc;
}

static const struct ethtool_ops ftmac110_ethtool_ops = {
    //.set_settings    = ftmac110_set_settings,
    //.get_settings    = ftmac110_get_settings,
    .get_drvinfo = ftmac110_get_drvinfo,
    .get_link = ethtool_op_get_link,
    .nway_reset = ftmac110_nway_reset,
};

/******************************************************************************
 * struct platform_driver functions
 *****************************************************************************/
static const struct net_device_ops ftmac110_netdev_ops = {
    .ndo_open = ftmac110_open,
    .ndo_stop = ftmac110_close,
    .ndo_start_xmit = ftmac110_start_xmit,
    //.ndo_set_rx_mode      = ftmac110_set_rx_mode,
    .ndo_set_mac_address = ftmac110_set_mac_address,
    .ndo_validate_addr = eth_validate_addr,
    .ndo_do_ioctl = ftmac110_ioctl,
    .ndo_get_stats = ftmac110_get_stats,
};

static int ftmac110_probe(struct platform_device *pdev)
{
    int irq, err;
    void *iobase;
    struct net_device *dev;
    ftmac110_priv_t *priv;
    struct resource *res;
    static u64 dma_mask = DMA_BIT_MASK(32);

    if (pdev == NULL)
        return -ENODEV;

    /* find IRQ */
    res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    if (!res)
        return -ENODEV;
    irq = res->start;

    /* find IOBASE */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res)
        return -ENODEV;
    if (!request_mem_region(res->start, res->end - res->start + 1, "ftmac110")) {
        printk("ftmac110: controller already in use\n");
        return -EBUSY;
    }
    iobase = ioremap(res->start, res->end - res->start + 1);
    if (iobase == NULL) {
        printk("ftmac110: failed to map memory\n");
        release_mem_region(res->start, res->end - res->start + 1);
        return -EFAULT;
    }

    /* setup net_device */
    dev = alloc_etherdev(sizeof(ftmac110_priv_t));
    if (dev == NULL) {
        printk("ftmac110: out of memory\n");
        iounmap(iobase);
        release_mem_region(res->start, res->end - res->start + 1);
        return -ENOMEM;
    }

    SET_NETDEV_DEV(dev, &pdev->dev);
    SET_ETHTOOL_OPS(dev, &ftmac110_ethtool_ops);
    dev->netdev_ops = &ftmac110_netdev_ops;

    platform_set_drvdata(pdev, dev);

    /* setup private data */
    priv = netdev_priv(dev);
    priv->netdev = dev;
    priv->dev = &(dev->dev);
    priv->irq = irq;
    priv->iobase = (uint32_t) iobase;
#ifdef CONFIG_FTMAC110_NAPI
    printk("Enable NAPI\n");
    netif_napi_add(dev, &priv->napi, ftmac110_rx_poll, CFG_NAPI_WEIGHT);
# ifdef CONFIG_FTMAC110_NAPI_LRO
    printk("Enable NAPI_LRO\n");
    dev->features |= NETIF_F_LRO;
    priv->lro_mgr.max_aggr = CFG_LRO_MAX_AGGR;
    priv->lro_mgr.max_desc = CFG_LRO_MAX_DESC;
    priv->lro_mgr.lro_arr = priv->lro_desc;
    priv->lro_mgr.get_skb_header = ftmac110_get_skb_header;
    priv->lro_mgr.features = LRO_F_NAPI;
    priv->lro_mgr.dev = priv->netdev;
    priv->lro_mgr.ip_summed = CHECKSUM_UNNECESSARY;
    priv->lro_mgr.ip_summed_aggr = CHECKSUM_UNNECESSARY;
# endif
#endif /* #ifdef CONFIG_FTMAC110_NAPI */
    spin_lock_init(&priv->lock);

    priv->mii_if.dev = dev;
    priv->mii_if.mdio_read = mdio_read;
    priv->mii_if.mdio_write = mdio_write;
    priv->mii_if.phy_id = CONFIG_FTMAC110_PHYADDR;
    priv->mii_if.phy_id_mask = 0x1f;
    priv->mii_if.reg_num_mask = 0x1f;

    dev->base_addr = (uint32_t) iobase;
    dev->irq = irq;
    dev->dev.dma_mask = &dma_mask;
    dev->dev.coherent_dma_mask = DMA_BIT_MASK(32);

    pdev->dev.dma_mask = &dma_mask;
    pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);

    /* register network device */
    err = register_netdev(dev);
    if (err) {
        printk("ftmac110: failed to register netdev\n");
        iounmap(iobase);
        release_mem_region(res->start, res->end - res->start + 1);
        return err;
    }

    printk("ftmac110: irq=%d, iobase=0x%08X\n", priv->irq, priv->iobase);

    if (is_zero_ether_addr(dev->dev_addr))
        random_ether_addr(dev->dev_addr);
    dev->dev_addr[0] = 0x00;
    printk("ftmac110: mac=%02x:%02x:%02x:%02x:%02x:%02x\n",
           dev->dev_addr[0], dev->dev_addr[1],
           dev->dev_addr[2], dev->dev_addr[3], dev->dev_addr[4], dev->dev_addr[5]);

#ifdef CONFIG_FTMAC110_PHYMON
    init_timer(&priv->timer);
#endif
    return 0;
}

static int ftmac110_remove(struct platform_device *pdev)
{
    struct net_device *dev;
    ftmac110_priv_t *priv;

    dev = platform_get_drvdata(pdev);
    priv = netdev_priv(dev);

    unregister_netdev(dev);

    iounmap((volatile void *)priv->iobase);

    platform_set_drvdata(pdev, NULL);

    free_netdev(dev);

    return 0;
}

static struct platform_driver ftmac110_driver = {
    .probe = ftmac110_probe,
    .remove = ftmac110_remove,
    .driver = {
               .name = DRV_NAME,
               .owner = THIS_MODULE,
               },
};

static int __init ftmac110_init(void)
{
#ifdef CONFIG_PLATFORM_GM8126
    int result = -ENODEV;

    result = ftmac110_pmu8126_init();
    if (result < 0) {
        printk("GM8126 pmu init fail\n");
        return result;
    }
#endif
    return platform_driver_register(&ftmac110_driver);
}

static void __exit ftmac110_cleanup(void)
{
    ftpmu010_deregister_reg(mac_fd);
    platform_driver_unregister(&ftmac110_driver);
}

module_init(ftmac110_init);
module_exit(ftmac110_cleanup);

MODULE_AUTHOR("Dante Su <dantesu@faraday-tech.com>");
MODULE_DESCRIPTION("ftmac110 driver");
MODULE_LICENSE("GPL");
