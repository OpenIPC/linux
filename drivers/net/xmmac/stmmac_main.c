/*
 * Author: ZengChuanJie
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/skbuff.h>
#include <linux/ethtool.h>
#include <linux/if_ether.h>
#include <linux/crc32.h>
#include <linux/mii.h>
#include <linux/phy.h>
#include <linux/if_vlan.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/prefetch.h>
#include "stmmac.h"

#include <mach/irqs.h>
#include <mach/hardware.h>

#include <linux/phy.h>

#define STMMAC_RESOURCE_NAME	"xmmaceth"
#define PHY_RESOURCE_NAME	"xmmacphy"

#undef STMMAC_DEBUG
#ifdef STMMAC_DEBUG
#define DBG(nlevel, klevel, fmt, args...) \
    ((void)(netif_msg_##nlevel(priv) && \
        printk(KERN_##klevel fmt, ## args)))
#else
#define DBG(nlevel, klevel, fmt, args...) do { } while (0)
#endif

#undef STMMAC_RX_DEBUG
#ifdef STMMAC_RX_DEBUG
#define RX_DBG(fmt, args...)  printk(fmt, ## args)
#else
#define RX_DBG(fmt, args...)  do { } while (0)
#endif

#undef STMMAC_XMIT_DEBUG
#ifdef STMMAC_TX_DEBUG
#define TX_DBG(fmt, args...)  printk(fmt, ## args)
#else
#define TX_DBG(fmt, args...)  do { } while (0)
#endif

#define STMMAC_ALIGN(x)	L1_CACHE_ALIGN(x)
#define JUMBO_LEN	9000

/* Module parameters */
#define TX_TIMEO 5000 /* default 5 seconds */
static int watchdog = TX_TIMEO;
module_param(watchdog, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(watchdog, "Transmit timeout in milliseconds");

static int debug = -1;		/* -1: default, 0: no output, 16:  all */
module_param(debug, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug, "Message Level (0: no output, 16: all)");

static int phyaddr = -1;
module_param(phyaddr, int, S_IRUGO);
MODULE_PARM_DESC(phyaddr, "Physical device address");

#define DMA_TX_SIZE 2048
static int dma_txsize = DMA_TX_SIZE;
module_param(dma_txsize, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dma_txsize, "Number of descriptors in the TX list");

#define DMA_RX_SIZE 256
static int dma_rxsize = DMA_RX_SIZE;
module_param(dma_rxsize, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dma_rxsize, "Number of descriptors in the RX list");

static int flow_ctrl = FLOW_AUTO; /* 默认开启流控 */
module_param(flow_ctrl, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(flow_ctrl, "Flow control ability [on/off]");

static int pause = PAUSE_TIME;
module_param(pause, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(pause, "Flow Control Pause Time");

#define TC_DEFAULT 64 /* 当发送缓冲区达到一个阈值值时开始发送数据，使用默认值 */
static int tc = TC_DEFAULT;
module_param(tc, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(tc, "DMA threshold control value");

#define DMA_BUFFER_SIZE	BUF_SIZE_2KiB
static int buf_sz = DMA_BUFFER_SIZE;
module_param(buf_sz, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(buf_sz, "DMA buffer size");

static const u32 default_msg_level = (NETIF_MSG_DRV | NETIF_MSG_PROBE |
        NETIF_MSG_LINK | NETIF_MSG_IFUP |
        NETIF_MSG_IFDOWN | NETIF_MSG_TIMER);

static irqreturn_t stmmac_interrupt(int irq, void *dev_id);


#define EDIT_ZENGCHUANJIE 1

/**
 * stmmac_verify_args - verify the driver parameters.
 * Description: it verifies if some wrong parameter is passed to the driver.
 * Note that wrong parameters are replaced with the default values.
 */
static void stmmac_verify_args(void)
{
    if (unlikely(watchdog < 0))
        watchdog = TX_TIMEO;
    if (unlikely(dma_rxsize < 0))
        dma_rxsize = DMA_RX_SIZE;
    if (unlikely(dma_txsize < 0))
        dma_txsize = DMA_TX_SIZE;
    if (unlikely((buf_sz < DMA_BUFFER_SIZE) || (buf_sz > BUF_SIZE_16KiB)))
        buf_sz = DMA_BUFFER_SIZE;
    if (unlikely(flow_ctrl > 1))
        flow_ctrl = FLOW_AUTO;
    else if (likely(flow_ctrl < 0))
        flow_ctrl = FLOW_OFF;
    if (unlikely((pause < 0) || (pause > 0xffff)))
        pause = PAUSE_TIME;
}

#if defined(STMMAC_XMIT_DEBUG) || defined(STMMAC_RX_DEBUG)
static void print_pkt(unsigned char *buf, int len)
{
    int j;
    pr_info("len = %d byte, buf addr: 0x%p", len, buf);
    for (j = 0; j < len; j++) {
        if ((j % 16) == 0)
            pr_info("\n %03x:", j);
        pr_info(" %02x", buf[j]);
    }
    pr_info("\n");
}
#endif

/* minimum number of free TX descriptors required to wake up TX process */
#define STMMAC_TX_THRESH(x)	(x->dma_tx_size/4)

static inline u32 stmmac_tx_avail(struct stmmac_priv *priv)
{
    /* cur_tx是指最后一个已经传送的的位置 */
    return priv->dirty_tx + priv->dma_tx_size - (priv->cur_tx+1);
}

/* On some ST platforms, some HW system configuraton registers have to be
 * set according to the link speed negotiated.
 */
static inline void stmmac_hw_fix_mac_speed(struct stmmac_priv *priv)
{
    struct phy_device *phydev = priv->phydev;

    if (likely(priv->plat->fix_mac_speed))
        priv->plat->fix_mac_speed(priv->plat->bsp_priv,
                phydev->speed);
}

/**
 * stmmac_adjust_link
 * @dev: net device structure
 * Description: it adjusts the link parameters.
 */
static void stmmac_adjust_link(struct net_device *dev)
{
    struct stmmac_priv *priv = netdev_priv(dev);
    struct phy_device *phydev = priv->phydev;
    unsigned long flags;
    int new_state = 0;
    unsigned int fc = priv->flow_ctrl, pause_time = priv->pause;

    if (phydev == NULL)
        return;

    DBG(probe, DEBUG, "stmmac_adjust_link: called.  address %d link %d\n",
            phydev->addr, phydev->link);

    spin_lock_irqsave(&priv->lock, flags);
    if (phydev->link) {
        u32 ctrl = readl(priv->ioaddr + MAC_CTRL_REG);

        /* Now we make sure that we can be in full duplex mode.
         * If not, we operate in half-duplex mode. */
        if (phydev->duplex != priv->oldduplex) {
            new_state = 1;
            if (!(phydev->duplex))
                ctrl   &= ~(1<<11);
            else
                ctrl   |= 1<<11;
            priv->oldduplex = phydev->duplex;
        }
        /* Flow Control operation */
        priv->hw->mac->flow_ctrl(priv->ioaddr, phydev->duplex,
                fc, pause_time);

        if (phydev->speed != priv->speed) {
            new_state = 1;
            switch (phydev->speed) {
                case 1000:
                    if (likely(priv->plat->has_gmac))
                        ctrl &= ~priv->hw->link.port;
                    stmmac_hw_fix_mac_speed(priv);
                    break;
                case 100:
                    {    
						ctrl |= 1<<14;
                        break;
                    }
                case 10:
                    {
                        /* 网络退化为10M，或者10M网络时使用 */
						ctrl &= ~(1<<14);
                        break;
                    }
                default:
                    if (netif_msg_link(priv))
                        pr_warning("%s: Speed (%d) is not 10"
                                " or 100!\n", dev->name, phydev->speed);
                    break;
            }

            priv->speed = phydev->speed;
        }

        writel(ctrl, priv->ioaddr + MAC_CTRL_REG);

        if (!priv->oldlink) {
            new_state = 1;
            priv->oldlink = 1;
        }
    } else if (priv->oldlink) {
        new_state = 1;
        priv->oldlink = 0;
        priv->speed = 0;
        priv->oldduplex = -1;
    }

    if (new_state && netif_msg_link(priv))
        phy_print_status(phydev);

    spin_unlock_irqrestore(&priv->lock, flags);

    DBG(probe, DEBUG, "stmmac_adjust_link: exiting\n");
}

/**
 * stmmac_init_phy - PHY initialization
 * @dev: net device structure
 * Description: it initializes the driver's PHY state, and attaches the PHY
 * to the mac driver.
 *  Return value:
 *  0 on success
 */
static int stmmac_init_phy(struct net_device *dev)
{
    struct stmmac_priv *priv = netdev_priv(dev);
    struct phy_device *phydev;
    char phy_id[MII_BUS_ID_SIZE + 3];
    char bus_id[MII_BUS_ID_SIZE];

    priv->oldlink = 0;
    priv->speed = 0;
    priv->oldduplex = -1;

    if (priv->phy_addr == -1) {
        /* We don't have a PHY, so do nothing */
        return 0;
    }

    snprintf(bus_id, MII_BUS_ID_SIZE, "%x", priv->plat->bus_id);
    snprintf(phy_id, MII_BUS_ID_SIZE + 3, PHY_ID_FMT, bus_id,
            priv->phy_addr);
    pr_debug("stmmac_init_phy:  trying to attach to %s\n", phy_id);

    phydev = phy_connect(dev, phy_id, &stmmac_adjust_link, 
            priv->phy_interface);

    if (IS_ERR(phydev)) {
        pr_err("%s: Could not attach to PHY\n", dev->name);
        return PTR_ERR(phydev);
    }

    /*
     * Broken HW is sometimes missing the pull-up resistor on the
     * MDIO line, which results in reads to non-existent devices returning
     * 0 rather than 0xffff. Catch this here and treat 0 as a non-existent
     * device as well.
     * Note: phydev->phy_id is the result of reading the UID PHY registers.
     */
    if (phydev->phy_id == 0) {
        phy_disconnect(phydev);
        return -ENODEV;
    }
    pr_debug("stmmac_init_phy:  %s: attached to PHY (UID 0x%x)"
            " Link = %d\n", dev->name, phydev->phy_id, phydev->link);

    priv->phydev = phydev;

    return 0;
}

static inline void stmmac_enable_mac(void __iomem *ioaddr)
{
    u32 value = readl(ioaddr + MAC_CTRL_REG);

    value |= MAC_RNABLE_RX | MAC_ENABLE_TX;
    writel(value, ioaddr + MAC_CTRL_REG);
}

static inline void stmmac_disable_mac(void __iomem *ioaddr)
{
    u32 value = readl(ioaddr + MAC_CTRL_REG);

    value &= ~(MAC_ENABLE_TX | MAC_RNABLE_RX);
    writel(value, ioaddr + MAC_CTRL_REG);
}

/**
 * display_ring
 * @p: pointer to the ring.
 * @size: size of the ring.
 * Description: display all the descriptors within the ring.
 */
static void display_ring(struct dma_desc *p, int size)
{
    struct tmp_s {
        u64 a;
        unsigned int b;
        unsigned int c;
    };
    int i;
    for (i = 0; i < size; i++) {
        struct tmp_s *x = (struct tmp_s *)(p + i);
        pr_info("\t%d [0x%x]: DES0=0x%x DES1=0x%x BUF1=0x%x BUF2=0x%x",
                i, (unsigned int)virt_to_phys(&p[i]),
                (unsigned int)(x->a), (unsigned int)((x->a) >> 32),
                x->b, x->c);
        pr_info("\n");
    }
}

/**
 * init_dma_desc_rings - init the RX/TX descriptor rings
 * @dev: net device structure
 * Description:  this function initializes the DMA RX/TX descriptors
 * and allocates the socket buffers.
 */
static void init_dma_desc_rings(struct net_device *dev)
{
    int i;
    struct stmmac_priv *priv = netdev_priv(dev);
    struct sk_buff *skb;
    unsigned int txsize = priv->dma_tx_size;
    unsigned int rxsize = priv->dma_rx_size;
    unsigned int bfsize = priv->dma_buf_sz;
    int buff2_needed = 0, dis_ic = 0;

    /* Set the Buffer size according to the MTU;
     * indeed, in case of jumbo we need to bump-up the buffer sizes.
     */
    if (unlikely(dev->mtu >= BUF_SIZE_8KiB))
        bfsize = BUF_SIZE_16KiB;
    else if (unlikely(dev->mtu >= BUF_SIZE_4KiB))
        bfsize = BUF_SIZE_8KiB;
    else if (unlikely(dev->mtu >= BUF_SIZE_2KiB))
        bfsize = BUF_SIZE_4KiB;
    else if (unlikely(dev->mtu >= DMA_BUFFER_SIZE))
        bfsize = BUF_SIZE_2KiB;
    else
        bfsize = DMA_BUFFER_SIZE;

    /* If the MTU exceeds 8k so use the second buffer in the chain */
    if (bfsize >= BUF_SIZE_8KiB)
        buff2_needed = 1;

    DBG(probe, INFO, "stmmac: txsize %d, rxsize %d, bfsize %d\n",
            txsize, rxsize, bfsize);

    priv->rx_skbuff_dma = kmalloc(rxsize * sizeof(dma_addr_t), GFP_KERNEL);
    priv->rx_skbuff =
        kmalloc(sizeof(struct sk_buff *) * rxsize, GFP_KERNEL);
    priv->dma_rx =
        (struct dma_desc *)dma_alloc_coherent(priv->device,
                rxsize *
                sizeof(struct dma_desc),
                &priv->dma_rx_phy,
                GFP_KERNEL);
    priv->tx_skbuff = kmalloc(sizeof(struct sk_buff *) * txsize,
            GFP_KERNEL);
    priv->dma_tx =
        (struct dma_desc *)dma_alloc_coherent(priv->device,
                txsize *
                sizeof(struct dma_desc),
                &priv->dma_tx_phy,
                GFP_KERNEL);

    if ((priv->dma_rx == NULL) || (priv->dma_tx == NULL)) {
        pr_err("%s:ERROR allocating the DMA Tx/Rx desc\n", __func__);
        return;
    }

    DBG(probe, INFO, "stmmac (%s) DMA desc rings: virt addr (Rx %p, "
            "Tx %p)\n\tDMA phy addr (Rx 0x%08x, Tx 0x%08x)\n",
            dev->name, priv->dma_rx, priv->dma_tx,
            (unsigned int)priv->dma_rx_phy, (unsigned int)priv->dma_tx_phy);

    /* RX INITIALIZATION */
    DBG(probe, INFO, "stmmac: SKB addresses:\n"
            "skb\t\tskb data\tdma data\n");

    for (i = 0; i < rxsize; i++) {
        struct dma_desc *p = priv->dma_rx + i;

        skb = netdev_alloc_skb_ip_align(dev, bfsize);
        if (unlikely(skb == NULL)) {
            pr_err("%s: Rx init fails; skb is NULL\n", __func__);
            break;
        }
        priv->rx_skbuff[i] = skb;
        priv->rx_skbuff_dma[i] = dma_map_single(priv->device, skb->data,
                bfsize, DMA_FROM_DEVICE);

        p->des2 = priv->rx_skbuff_dma[i];
        /* 采用ring即环形队列方式 */
        if (unlikely(buff2_needed))
            p->des3 = p->des2 + BUF_SIZE_8KiB;
        DBG(probe, INFO, "[%p]\t[%p]\t[%x]\n", priv->rx_skbuff[i],
                priv->rx_skbuff[i]->data, priv->rx_skbuff_dma[i]);
    }
    priv->cur_rx = 0;
    priv->dirty_rx = (unsigned int)(i - rxsize);
    priv->dma_buf_sz = bfsize;
    buf_sz = bfsize;

    /* TX INITIALIZATION */
    for (i = 0; i < txsize; i++) {
        priv->tx_skbuff[i] = NULL;
        priv->dma_tx[i].des2 = 0;
    }
    priv->dirty_tx = 0;
    priv->cur_tx = 0;

    /* Clear the Rx/Tx descriptors */
    priv->hw->desc->init_rx_desc(priv->dma_rx, rxsize, dis_ic);
    priv->hw->desc->init_tx_desc(priv->dma_tx, txsize);

    if (netif_msg_hw(priv)) {
        pr_info("RX descriptor ring:\n");
        display_ring(priv->dma_rx, rxsize);
        pr_info("TX descriptor ring:\n");
        display_ring(priv->dma_tx, txsize);
    }
}

static void dma_free_rx_skbufs(struct stmmac_priv *priv)
{
    int i;

    for (i = 0; i < priv->dma_rx_size; i++) {
        if (priv->rx_skbuff[i]) {
            dma_unmap_single(priv->device, priv->rx_skbuff_dma[i],
                    priv->dma_buf_sz, DMA_FROM_DEVICE);
            dev_kfree_skb_any(priv->rx_skbuff[i]);
        }
        priv->rx_skbuff[i] = NULL;
    }
}

static void dma_free_tx_skbufs(struct stmmac_priv *priv)
{
    int i;

    for (i = 0; i < priv->dma_tx_size; i++) {
        if (priv->tx_skbuff[i] != NULL) {
            struct dma_desc *p = priv->dma_tx + i;
            if (p->des2)
                dma_unmap_single(priv->device, p->des2,
                        priv->hw->desc->get_tx_len(p),
                        DMA_TO_DEVICE);
            /* dev_kfree_skb_any可以在中断上下文、非中断上下文引用 */
            dev_kfree_skb_any(priv->tx_skbuff[i]);
            priv->tx_skbuff[i] = NULL;
        }
    }
}

static void free_dma_desc_resources(struct stmmac_priv *priv)
{
    /* Release the DMA TX/RX socket buffers */
    dma_free_rx_skbufs(priv);
    dma_free_tx_skbufs(priv);

    /* Free the region of consistent memory previously allocated for
     * the DMA */
    dma_free_coherent(priv->device,
            priv->dma_tx_size * sizeof(struct dma_desc),
            priv->dma_tx, priv->dma_tx_phy);
    dma_free_coherent(priv->device,
            priv->dma_rx_size * sizeof(struct dma_desc),
            priv->dma_rx, priv->dma_rx_phy);
    kfree(priv->rx_skbuff_dma);
    kfree(priv->rx_skbuff);
    kfree(priv->tx_skbuff);
}

/**
 *  stmmac_dma_operation_mode - HW DMA operation mode
 *  @priv : pointer to the private device structure.
 *  Description: it sets the DMA operation mode: tx/rx DMA thresholds
 *  or Store-And-Forward capability.
 */
static void stmmac_dma_operation_mode(struct stmmac_priv *priv)
{
    if (likely((priv->plat->tx_coe) && (!priv->no_csum_insertion))) {
        /* In case of GMAC, SF mode has to be enabled
         * to perform the TX COE. This depends on:
         * 1) TX COE if actually supported
         * 2) There is no bugged Jumbo frame support
         *    that needs to not insert csum in the TDES.
         */
        priv->hw->dma->dma_mode(priv->ioaddr,
                SF_DMA_MODE, SF_DMA_MODE);
        tc = SF_DMA_MODE;
    } else
        priv->hw->dma->dma_mode(priv->ioaddr, tc, SF_DMA_MODE);
}

/**
 * stmmac_tx:
 * @priv: private driver structure
 * Description: it reclaims resources after transmission completes.
 */
static void stmmac_tx(struct stmmac_priv *priv)
{
    unsigned int txsize = priv->dma_tx_size;

    while (priv->dirty_tx != priv->cur_tx) {
        int last;
        unsigned int entry = priv->dirty_tx % txsize;
        struct sk_buff *skb = priv->tx_skbuff[entry];
        struct dma_desc *p = priv->dma_tx + entry;

        /* Check if the descriptor is owned by the DMA. */
        if (priv->hw->desc->get_tx_owner(p))
            break;

        /* Verify tx error by looking at the last segment */
        last = priv->hw->desc->get_tx_ls(p);
        if (likely(last)) {
            int tx_error =
                priv->hw->desc->tx_status(&priv->dev->stats,
                        &priv->xstats, p,
                        priv->ioaddr);
            if (likely(tx_error == 0)) {
                priv->dev->stats.tx_packets++;
                priv->xstats.tx_pkt_n++;
            } else
                priv->dev->stats.tx_errors++;
        }
        TX_DBG("%s: curr %d, dirty %d\n", __func__,
                priv->cur_tx, priv->dirty_tx);

        if (likely(p->des2))
            dma_unmap_single(priv->device, p->des2,
                    priv->hw->desc->get_tx_len(p),
                    DMA_TO_DEVICE);
        if (unlikely(p->des3))
            p->des3 = 0;

        if (likely(skb != NULL)) {
            dev_kfree_skb(skb);
            priv->tx_skbuff[entry] = NULL;
        }

        priv->hw->desc->release_tx_desc(p);

        entry = (++priv->dirty_tx) % txsize;
    }
    if (unlikely(netif_queue_stopped(priv->dev) &&
                stmmac_tx_avail(priv) > STMMAC_TX_THRESH(priv))) {
        netif_tx_lock(priv->dev);
        if (netif_queue_stopped(priv->dev) &&
                stmmac_tx_avail(priv) > STMMAC_TX_THRESH(priv)) {
            TX_DBG("%s: restart transmit\n", __func__);
            netif_wake_queue(priv->dev);
        }
        netif_tx_unlock(priv->dev);
    }
}

static inline void stmmac_enable_irq(struct stmmac_priv *priv)
{
    priv->hw->dma->enable_dma_irq(priv->ioaddr);
}

static inline void stmmac_disable_irq(struct stmmac_priv *priv)
{
    priv->hw->dma->disable_dma_irq(priv->ioaddr);
}

static int stmmac_has_work(struct stmmac_priv *priv)
{
    unsigned int has_work = 0;
    int rxret, tx_work = 0;

    rxret = priv->hw->desc->get_rx_owner(priv->dma_rx +
            (priv->cur_rx % priv->dma_rx_size));

    if (priv->dirty_tx != priv->cur_tx)
        tx_work = 1;

    if (likely(!rxret || tx_work))
        has_work = 1;

    return has_work;
}

static inline void _stmmac_schedule(struct stmmac_priv *priv)
{
    if (likely(stmmac_has_work(priv))) {
        stmmac_disable_irq(priv);
        napi_schedule(&priv->napi);
    }
}


/**
 * stmmac_tx_err:
 * @priv: pointer to the private device structure
 * Description: it cleans the descriptors and restarts the transmission
 * in case of errors.
 */
static void stmmac_tx_err(struct stmmac_priv *priv)
{

    netif_stop_queue(priv->dev);

    priv->hw->dma->stop_tx(priv->ioaddr);
    dma_free_tx_skbufs(priv);
    priv->hw->desc->init_tx_desc(priv->dma_tx, priv->dma_tx_size);
    priv->dirty_tx = 0;
    priv->cur_tx = 0;
    priv->hw->dma->start_tx(priv->ioaddr);

    priv->dev->stats.tx_errors++;
    netif_wake_queue(priv->dev);
}




void gpio_reset_phy(void)
{
	/* 输出低 */
	gpio_write(GPIO_OUT_EN | GPIO_OUT_LOW, 59); 
	mdelay(100);
	/* 输出高 */
	gpio_write(GPIO_OUT_EN | GPIO_OUT_HIGH, 59); 
	mdelay(100);
}
static void stmmac_dma_interrupt(struct stmmac_priv *priv)
{
    int status;

    status = priv->hw->dma->dma_interrupt(priv->ioaddr, &priv->xstats);
    if (likely(status == handle_tx_rx)) {
        _stmmac_schedule(priv);
    }

    else if (unlikely(status == tx_hard_error_bump_tc)) {
        /* Try to bump up the dma threshold on this failure */
        if (unlikely(tc != SF_DMA_MODE) && (tc <= 256)) {
            tc += 64;
            priv->hw->dma->dma_mode(priv->ioaddr, tc, SF_DMA_MODE);
            priv->xstats.threshold = tc;
        }
    } else if (unlikely(status == tx_hard_error))
        stmmac_tx_err(priv);
}


/**
 *  stmmac_open - open entry point of the driver
 *  @dev : pointer to the device structure.
 *  Description:
 *  This function is the open entry point of the driver.
 *  Return value:
 *  0 on success and an appropriate (-)ve integer as defined in errno.h
 *  file on failure.
 */
static int stmmac_open(struct net_device *dev)
{
    struct stmmac_priv *priv = netdev_priv(dev);
    int ret;


    /* Check that the MAC address is valid.  If its not, refuse
     * to bring the device up. The user must specify an
     * address using the following linux command:
     *      ifconfig eth0 hw ether xx:xx:xx:xx:xx:xx  */
    if (!is_valid_ether_addr(dev->dev_addr)) {
        random_ether_addr(dev->dev_addr);
        pr_warning("%s: generated random MAC address %pM\n", dev->name,
                dev->dev_addr);
    }

    //gpio_reset_phy();

    stmmac_verify_args();

    ret = stmmac_init_phy(dev);
    if (unlikely(ret)) {
        pr_err("%s: Cannot attach to PHY (error: %d)\n", __func__, ret);
        goto open_error;
    }

    /* Create and initialize the TX/RX descriptors chains. */
    priv->dma_tx_size = STMMAC_ALIGN(dma_txsize);
    priv->dma_rx_size = STMMAC_ALIGN(dma_rxsize);
    priv->dma_buf_sz = STMMAC_ALIGN(buf_sz);
    init_dma_desc_rings(dev);


    /* DMA initialization and SW reset */
    ret = priv->hw->dma->init(priv->ioaddr, priv->plat->pbl,
            priv->dma_tx_phy, priv->dma_rx_phy);
    if (ret < 0) {
        pr_err("%s: DMA initialization failed\n", __func__);
        goto open_error;
    }

    /* Copy the MAC addr into the HW  */
    priv->hw->mac->set_umac_addr(priv->ioaddr, dev->dev_addr, 0);
    /* If required, perform hw setup of the bus. */
    if (priv->plat->bus_setup)
        priv->plat->bus_setup(priv->ioaddr);
    /* Initialize the MAC Core */
    priv->hw->mac->core_init(priv->ioaddr);

    priv->rx_coe = priv->hw->mac->rx_coe(priv->ioaddr);
    if (priv->rx_coe)
        pr_info("stmmac: Rx Checksum Offload Engine supported\n");
    if (priv->plat->tx_coe)
        pr_info("\tTX Checksum insertion supported\n");
    netdev_update_features(dev);

    /* Initialise the MMC (if present) to disable all interrupts. */
    writel(0xffffffff, priv->ioaddr + MMC_HIGH_INTR_MASK);
    writel(0xffffffff, priv->ioaddr + MMC_LOW_INTR_MASK);

    /* Request the IRQ lines */
    ret = request_irq(dev->irq, stmmac_interrupt,
            IRQF_SHARED, dev->name, dev);
    if (unlikely(ret < 0)) {
        pr_err("%s: ERROR: allocating the IRQ %d (error: %d)\n",
                __func__, dev->irq, ret);
        goto open_error;
    }

    /* Enable the MAC Rx/Tx */
    stmmac_enable_mac(priv->ioaddr);

    /* Set the HW DMA mode and the COE */
    stmmac_dma_operation_mode(priv);

    /* Extra statistics */
    memset(&priv->xstats, 0, sizeof(struct stmmac_extra_stats));
    priv->xstats.threshold = tc;

    /* Start the ball rolling... */
    DBG(probe, DEBUG, "%s: DMA RX/TX processes started...\n", dev->name);
    priv->hw->dma->start_tx(priv->ioaddr);
    priv->hw->dma->start_rx(priv->ioaddr);

    /* Dump DMA/MAC registers */
    if (netif_msg_hw(priv)) {
        priv->hw->mac->dump_regs(priv->ioaddr);
        priv->hw->dma->dump_regs(priv->ioaddr);
    }

    if (priv->phydev)
        phy_start(priv->phydev);

    napi_enable(&priv->napi);
    netif_start_queue(dev);

    return 0;

open_error:
    if (priv->phydev)
        phy_disconnect(priv->phydev);

    return ret;
}

/**
 *  stmmac_release - close entry point of the driver
 *  @dev : device pointer.
 *  Description:
 *  This is the stop entry point of the driver.
 */
static int stmmac_release(struct net_device *dev)
{
    struct stmmac_priv *priv = netdev_priv(dev);

    /* Stop and disconnect the PHY */
    if (priv->phydev) {
        phy_stop(priv->phydev);
        phy_disconnect(priv->phydev);
        priv->phydev = NULL;
    }

    netif_stop_queue(dev);

    napi_disable(&priv->napi);

    /* Free the IRQ lines */
    free_irq(dev->irq, dev);

    /* Stop TX/RX DMA and clear the descriptors */
    priv->hw->dma->stop_tx(priv->ioaddr);
    priv->hw->dma->stop_rx(priv->ioaddr);

    /* Release and free the Rx/Tx resources */
    free_dma_desc_resources(priv);

    /* Disable the MAC Rx/Tx */
    stmmac_disable_mac(priv->ioaddr);

    netif_carrier_off(dev);

    return 0;
}


/**
 *  stmmac_xmit:
 *  @skb : the socket buffer
 *  @dev : device pointer
 *  Description : Tx entry point of the driver.
 */
static netdev_tx_t stmmac_xmit(struct sk_buff *skb, struct net_device *dev)
{
    struct stmmac_priv *priv = netdev_priv(dev);
    unsigned int txsize = priv->dma_tx_size;
    unsigned int entry;
    int i, csum_insertion = 0;
    int nfrags = skb_shinfo(skb)->nr_frags; /* 处理多个分散的缓冲区 */
    struct dma_desc *desc, *first;
    unsigned int nopaged_len;

    /* 如果发现可用的缓冲区不足以容纳将要发送的缓冲区 */
    if (unlikely(stmmac_tx_avail(priv) < nfrags + 1)) {
        if (!netif_queue_stopped(dev)) {
            netif_stop_queue(dev);
            /* This is a hard error, log it. */
            pr_err("%s: BUG! Tx Ring full when queue awake\n",
                    __func__);
        }
        return NETDEV_TX_BUSY;
    }

    spin_lock(&priv->tx_lock);
    entry = priv->cur_tx % txsize;

#ifdef STMMAC_XMIT_DEBUG
    if ((skb->len > ETH_FRAME_LEN) || nfrags)
        pr_info("stmmac xmit:\n"
                "\tskb addr %p - len: %d - nopaged_len: %d\n"
                "\tn_frags: %d - ip_summed: %d - %s gso\n",
                skb, skb->len, skb_headlen(skb), nfrags, skb->ip_summed,
                !skb_is_gso(skb) ? "isn't" : "is");
#endif

    desc = priv->dma_tx + entry;
    first = desc;

#ifdef STMMAC_XMIT_DEBUG
    if ((nfrags > 0) || (skb->len > ETH_FRAME_LEN))
        pr_debug("stmmac xmit: skb len: %d, nopaged_len: %d,\n"
                "\t\tn_frags: %d, ip_summed: %d\n",
                skb->len, skb_headlen(skb), nfrags, skb->ip_summed);
#endif
    priv->tx_skbuff[entry] = skb;
    nopaged_len = skb_headlen(skb);
    desc->des2 = dma_map_single(priv->device, skb->data,
            nopaged_len, DMA_TO_DEVICE);
    priv->hw->desc->prepare_tx_desc(desc, 1, nopaged_len,
            csum_insertion);

    for (i = 0; i < nfrags; i++) {
        skb_frag_t *frag = &skb_shinfo(skb)->frags[i];
        int len = frag->size;

        entry = (++priv->cur_tx) % txsize;
        desc = priv->dma_tx + entry;

        TX_DBG("\t[entry %d] segment len: %d\n", entry, len);
        desc->des2 = dma_map_page(priv->device, frag->page.p,
                frag->page_offset,
                len, DMA_TO_DEVICE);
        priv->tx_skbuff[entry] = NULL;
        priv->hw->desc->prepare_tx_desc(desc, 0, len, csum_insertion);
        priv->hw->desc->set_tx_owner(desc);
    }

    /* Interrupt on completition only for the latest segment */
    /* 上层传送多个分段下来，这些分段都属于同一个帧，所以最后一个分段是帧的最后部分 */
    priv->hw->desc->close_tx_desc(desc);

    /* To avoid raise condition */
    priv->hw->desc->set_tx_owner(first);

    priv->cur_tx++;

#ifdef STMMAC_XMIT_DEBUG
    if (netif_msg_pktdata(priv)) {
        pr_info("stmmac xmit: current=%d, dirty=%d, entry=%d, "
                "first=%p, nfrags=%d\n",
                (priv->cur_tx % txsize), (priv->dirty_tx % txsize),
                entry, first, nfrags);
        display_ring(priv->dma_tx, txsize);
        pr_info(">>> frame to be transmitted: ");
        print_pkt(skb->data, skb->len);
    }
#endif
    /* 
     * 上面准备好的缓冲区可以进行发送，但是如果要连续进行下一次的传送，就需要判断下一次
     * 的可用的缓冲区 
     */
    if (unlikely(stmmac_tx_avail(priv) <= (MAX_SKB_FRAGS + 1))) {
        TX_DBG("%s: stop transmitted packets\n", __func__);
        netif_stop_queue(dev);
    }

    dev->stats.tx_bytes += skb->len;

    /* 发送Poll命令 */
    priv->hw->dma->enable_dma_transmission(priv->ioaddr);

    spin_unlock(&priv->tx_lock);
    return NETDEV_TX_OK;
}

static inline void stmmac_rx_refill(struct stmmac_priv *priv)
{
    unsigned int rxsize = priv->dma_rx_size;
    int bfsize = priv->dma_buf_sz;
    struct dma_desc *p = priv->dma_rx;

    for (; priv->cur_rx - priv->dirty_rx > 0; priv->dirty_rx++) {
        /* 将已经处理了的sk_buff重新分配空间 */
        unsigned int entry = priv->dirty_rx % rxsize;
        if (likely(priv->rx_skbuff[entry] == NULL)) {
            struct sk_buff *skb;

			skb = __skb_dequeue(&priv->rx_recycle);
			if (skb == NULL){
				skb = netdev_alloc_skb_ip_align(priv->dev,
				bfsize);
			}

            if (unlikely(skb == NULL))
                break;

            priv->rx_skbuff[entry] = skb;
            priv->rx_skbuff_dma[entry] =
                dma_map_single(priv->device, skb->data, bfsize,
                        DMA_FROM_DEVICE);

            (p + entry)->des2 = priv->rx_skbuff_dma[entry];
            RX_DBG(KERN_INFO "\trefill entry #%d\n", entry);
        }
        priv->hw->desc->set_rx_owner(p + entry);
    }
}

static int stmmac_rx(struct stmmac_priv *priv, int limit)
{
    unsigned int rxsize = priv->dma_rx_size;
    unsigned int entry = priv->cur_rx % rxsize;
    unsigned int next_entry;
    unsigned int count = 0;
    struct dma_desc *p = priv->dma_rx + entry;
    struct dma_desc *p_next;

#ifdef STMMAC_RX_DEBUG
    if (netif_msg_hw(priv)) {
        pr_debug(">>> stmmac_rx: descriptor ring:\n");
        display_ring(priv->dma_rx, rxsize);
    }
#endif
    count = 0;
    while (!priv->hw->desc->get_rx_owner(p)) {
        int status;

        if (count >= limit)
            break;

        count++;

        next_entry = (++priv->cur_rx) % rxsize;
        p_next = priv->dma_rx + next_entry;
        prefetch(p_next);

        /* read the status of the incoming frame */
        status = (priv->hw->desc->rx_status(&priv->dev->stats,
                    &priv->xstats, p));
        if (unlikely(status == discard_frame)) {
            priv->dev->stats.rx_errors++;
        } else {
            struct sk_buff *skb;
            int frame_len;

            frame_len = priv->hw->desc->get_rx_frame_len(p);
            /* ACS is set; GMAC core strips PAD/FCS for IEEE 802.3
             * Type frames (LLC/LLC-SNAP) */
#if !EDIT_ZENGCHUANJIE
            /* 
             * 我们只在ethernet当中使用(frame_len>1536-0x800/0x806/0x808), 不会使用0x802.3帧，
             * 在没有开启checksum的时候，FCS及其长度会被自动丢弃，所以frame_len不需要-4
             * 参考gmac文档p418,Table 6-32(IPCHKSUM_EN=0 && IPC_FULL_OFFLOAD=0)
             *
             */
            if (unlikely(status != llc_snap))
                frame_len -= ETH_FCS_LEN;
#endif
#ifdef STMMAC_RX_DEBUG
            if (frame_len > ETH_FRAME_LEN)
                pr_debug("\tRX frame size %d, COE status: %d\n",
                        frame_len, status);

            if (netif_msg_hw(priv))
                pr_debug("\tdesc: %p [entry %d] buff=0x%x\n",
                        p, entry, p->des2);
#endif
            skb = priv->rx_skbuff[entry];
            if (unlikely(!skb)) {
                pr_err("%s: Inconsistent Rx descriptor chain\n",
                        priv->dev->name);
                priv->dev->stats.rx_dropped++;
                break;
            }
            prefetch(skb->data - NET_IP_ALIGN);
            priv->rx_skbuff[entry] = NULL;

            skb_put(skb, frame_len);
            /* 如果该数据被cache了，那么需要将数据刷新到cache当中去 */
            dma_unmap_single(priv->device,
                    priv->rx_skbuff_dma[entry],
                    priv->dma_buf_sz, DMA_FROM_DEVICE);
#ifdef STMMAC_RX_DEBUG
            if (netif_msg_pktdata(priv)) {
                pr_info(" frame received (%dbytes)", frame_len);
                print_pkt(skb->data, frame_len);
            }
#endif
            skb->protocol = eth_type_trans(skb, priv->dev);

#if EDIT_ZENGCHUANJIE 
            /* always for the old mac 10/100 */
            skb_checksum_none_assert(skb);
            netif_receive_skb(skb);
#else
            /* 由于gmac ip没有开启l4 checksum以及ipv4 checksum
             * 所以这里肯定是CHECKSUM_NONE, 这些校验必须由上层来完成
             * (目的在于简化驱动复杂度)
             */
            if (unlikely(status == csum_none)) {
                /* always for the old mac 10/100 */
                skb_checksum_none_assert(skb);
                netif_receive_skb(skb);
            } else {
                skb->ip_summed = CHECKSUM_UNNECESSARY;
                napi_gro_receive(&priv->napi, skb);
            }
#endif

            priv->dev->stats.rx_packets++;
            priv->dev->stats.rx_bytes += frame_len;
        }
        entry = next_entry;
        p = p_next;	/* use prefetched values */
    }

    /* 
     * 由于在接收的时候sk_buff是由驱动程序提供的，提供给协议处理函数并被上层处理后，
     * skb_buff会被释放调，所以需要重新创建对应的sk_buff
     */
    stmmac_rx_refill(priv);

    priv->xstats.rx_pkt_n += count;

    return count;
}

/**
 *  stmmac_poll - stmmac poll method (NAPI)
 *  @napi : pointer to the napi structure.
 *  @budget : maximum number of packets that the current CPU can receive from
 *	      all interfaces.
 *  Description :
 *   This function implements the the reception process.
 *   Also it runs the TX completion thread
 */
static int stmmac_poll(struct napi_struct *napi, int budget)
{
    struct stmmac_priv *priv = container_of(napi, struct stmmac_priv, napi);
    int work_done = 0;

    priv->xstats.poll_n++;
    /* 产生关于tx/rx的中断，就重新判断驱动程序缓冲区的能力,决定是否重新使能设备发送 */
    spin_lock(&priv->tx_lock);
    stmmac_tx(priv);
    spin_unlock(&priv->tx_lock);
    work_done = stmmac_rx(priv, budget);

    if (work_done < budget) {
        /* 如果接收的数量小于能够接收的配额，则表示全部接收完成，将当前napi
         * 从softnet_data的poll_list当中移除(清除SCHED标识)，如果下次中断产生
         * 再将当中设备的状态置为SCHED
         */
        napi_complete(napi);
        stmmac_enable_irq(priv);
    }
    return work_done;
}

/**
 *  stmmac_tx_timeout
 *  @dev : Pointer to net device structure
 *  Description: this function is called when a packet transmission fails to
 *   complete within a reasonable tmrate. The driver will mark the error in the
 *   netdev structure and arrange for the device to be reset to a sane state
 *   in order to transmit a new packet.
 */
static void stmmac_tx_timeout(struct net_device *dev)
{
    struct stmmac_priv *priv = netdev_priv(dev);

    /* Clear Tx resources and restart transmitting again */
    stmmac_tx_err(priv);
}

/* Configuration changes (passed on by ifconfig) */
static int stmmac_config(struct net_device *dev, struct ifmap *map)
{
    if (dev->flags & IFF_UP)	/* can't act on a running interface */
        return -EBUSY;

    /* Don't allow changing the I/O address */
    if (map->base_addr != dev->base_addr) {
        pr_warning("%s: can't change I/O address\n", dev->name);
        return -EOPNOTSUPP;
    }

    /* Don't allow changing the IRQ */
    if (map->irq != dev->irq) {
        pr_warning("%s: can't change IRQ number %d\n",
                dev->name, dev->irq);
        return -EOPNOTSUPP;
    }

    /* ignore other fields */
    return 0;
}

/**
 *  stmmac_multicast_list - entry point for multicast addressing
 *  @dev : pointer to the device structure
 *  Description:
 *  This function is a driver entry point which gets called by the kernel
 *  whenever multicast addresses must be enabled/disabled.
 *  Return value:
 *  void.
 */
static void stmmac_multicast_list(struct net_device *dev)
{
    struct stmmac_priv *priv = netdev_priv(dev);

    //printk(KERN_EMERG"	Get In stmmac_multicast_list.\n");
    spin_lock(&priv->lock);
    priv->hw->mac->set_filter(dev);
    spin_unlock(&priv->lock);
}

/**
 *  stmmac_change_mtu - entry point to change MTU size for the device.
 *  @dev : device pointer.
 *  @new_mtu : the new MTU size for the device.
 *  Description: the Maximum Transfer Unit (MTU) is used by the network layer
 *  to drive packet transmission. Ethernet has an MTU of 1500 octets
 *  (ETH_DATA_LEN). This value can be changed with ifconfig.
 *  Return value:
 *  0 on success and an appropriate (-)ve integer as defined in errno.h
 *  file on failure.
 */
static int stmmac_change_mtu(struct net_device *dev, int new_mtu)
{
    struct stmmac_priv *priv = netdev_priv(dev);
    int max_mtu;

    if (netif_running(dev)) {
        pr_err("%s: must be stopped to change its MTU\n", dev->name);
        return -EBUSY;
    }

    if (priv->plat->has_gmac)
        max_mtu = JUMBO_LEN;
    else
        max_mtu = ETH_DATA_LEN;

    if ((new_mtu < 46) || (new_mtu > max_mtu)) {
        pr_err("%s: invalid MTU, max MTU is: %d\n", dev->name, max_mtu);
        return -EINVAL;
    }

    dev->mtu = new_mtu;
    netdev_update_features(dev);

    return 0;
}

static netdev_features_t stmmac_fix_features(struct net_device *dev, 
        netdev_features_t features)
{
    struct stmmac_priv *priv = netdev_priv(dev);

    if (!priv->rx_coe)
        features &= ~NETIF_F_RXCSUM;
    if (!priv->plat->tx_coe)
        features &= ~NETIF_F_ALL_CSUM;

    /* Some GMAC devices have a bugged Jumbo frame support that
     * needs to have the Tx COE disabled for oversized frames
     * (due to limited buffer sizes). In this case we disable
     * the TX csum insertionin the TDES and not use SF. */
    if (priv->plat->bugged_jumbo && (dev->mtu > ETH_DATA_LEN))
        features &= ~NETIF_F_ALL_CSUM;

    return features;
}

static irqreturn_t stmmac_interrupt(int irq, void *dev_id)
{
    struct net_device *dev = (struct net_device *)dev_id;
    struct stmmac_priv *priv = netdev_priv(dev);

    if (unlikely(!dev)) {
        pr_err("%s: invalid dev pointer\n", __func__);
        return IRQ_NONE;
    }

    if (priv->plat->has_gmac)
        /* To handle GMAC own interrupts */
        priv->hw->mac->host_irq_status((void __iomem *) dev->base_addr);

    stmmac_dma_interrupt(priv);

    return IRQ_HANDLED;
}


/**
 *  stmmac_ioctl - Entry point for the Ioctl
 *  @dev: Device pointer.
 *  @rq: An IOCTL specefic structure, that can contain a pointer to
 *  a proprietary structure used to pass information to the driver.
 *  @cmd: IOCTL command
 *  Description:
 *  Currently there are no special functionality supported in IOCTL, just the
 *  phy_mii_ioctl(...) can be invoked.
 */
static int stmmac_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
    struct stmmac_priv *priv = netdev_priv(dev);
    int ret;

    if (!netif_running(dev))
        return -EINVAL;

    if (!priv->phydev)
        return -EINVAL;

    spin_lock(&priv->lock);
    ret = phy_mii_ioctl(priv->phydev, rq, cmd);
    spin_unlock(&priv->lock);

    return ret;
}


static const struct net_device_ops stmmac_netdev_ops = {
    .ndo_open = stmmac_open,
    .ndo_start_xmit = stmmac_xmit,
    .ndo_stop = stmmac_release,
    .ndo_change_mtu = stmmac_change_mtu,
    .ndo_fix_features = stmmac_fix_features,
    .ndo_set_rx_mode = stmmac_multicast_list,
    /* 发送首先需要进入发送缓冲区队列，这个函数是缓冲区队列的某个元素一直迟迟没有被驱动程序的所处理 */
    .ndo_tx_timeout = stmmac_tx_timeout,
    .ndo_do_ioctl = stmmac_ioctl,
    .ndo_set_config = stmmac_config,
    .ndo_set_mac_address = eth_mac_addr,
};

/**
 * stmmac_probe - Initialization of the adapter .
 * @dev : device pointer
 * Description: The function initializes the network device structure for
 * the STMMAC driver. It also calls the low level routines
 * in order to init the HW (i.e. the DMA engine)
 */
static int stmmac_probe(struct net_device *dev)
{
    int ret = 0;
    struct stmmac_priv *priv = netdev_priv(dev);

    ether_setup(dev);

    dev->netdev_ops = &stmmac_netdev_ops;
    stmmac_set_ethtool_ops(dev);

    /*
     * IP头部的校验和都是由软件完成的，hw_features不会指定ip头的校验
     *
     * NETF_F_IP_CSUM表示硬件可以计算IPV4的L4校验
     * NETIF_F_HW_CSUM表示硬件可以计算所有协议的L4校验，在TCP/IP里面不会指定这个字段
     * NETIF_F_NO_CSUM表示这个硬件很可靠，不需要任何的校验检查，只适用于回环设备
     */
    dev->hw_features = NETIF_F_SG; /* 暂时不需要支持L4硬件校验 */
    dev->features |= dev->hw_features | NETIF_F_HIGHDMA;
    dev->watchdog_timeo = msecs_to_jiffies(watchdog);
    priv->msg_enable = netif_msg_init(debug, default_msg_level);

    if (flow_ctrl) /* 默认开启流控 */
        priv->flow_ctrl = FLOW_AUTO;	/* RX/TX pause on */

    priv->pause = pause;
    netif_napi_add(dev, &priv->napi, stmmac_poll, 64);

    /* Get the MAC address */
    priv->hw->mac->get_umac_addr((void __iomem *) dev->base_addr,
            dev->dev_addr, 0);

    if (!is_valid_ether_addr(dev->dev_addr))
        pr_warning("\tno valid MAC address;"
                "please, use ifconfig or nwhwconfig!\n");

    spin_lock_init(&priv->lock);
    spin_lock_init(&priv->tx_lock);

    ret = register_netdev(dev);
    if (ret) {
        pr_err("%s: ERROR %i registering the device\n",
                __func__, ret);
        return -ENODEV;
    }

    DBG(probe, DEBUG, "%s: Scatter/Gather: %s - HW checksums: %s\n",
            dev->name, (dev->features & NETIF_F_SG) ? "on" : "off",
            (dev->features & NETIF_F_IP_CSUM) ? "on" : "off");

    return ret;
}

/**
 * stmmac_mac_device_setup
 * @dev : device pointer
 * Description: select and initialise the mac device (mac100 or Gmac).
 */
static int stmmac_mac_device_setup(struct net_device *dev)
{
    struct stmmac_priv *priv = netdev_priv(dev);

    struct mac_device_info *device;

    if (priv->plat->has_gmac) {
        device = dwmac1000_setup(priv->ioaddr);
    } else {
        device = dwmac100_setup(priv->ioaddr);
    }

    if (!device)
        return -ENOMEM;

    if (priv->plat->enh_desc) {
        device->desc = &enh_desc_ops;
        pr_info("\tEnhanced descriptor structure\n");
    } else {
        device->desc = &ndesc_ops;
    }

    priv->hw = device;

    return 0;
}

static int stmmacphy_dvr_probe(struct platform_device *pdev)
{
    struct plat_stmmacphy_data *plat_dat = pdev->dev.platform_data;

    pr_debug("stmmacphy_dvr_probe: added phy for bus %d\n",
            plat_dat->bus_id);

    return 0;
}

static int stmmacphy_dvr_remove(struct platform_device *pdev)
{
    return 0;
}

static struct platform_driver stmmacphy_driver = {
    .driver = {
        .name = PHY_RESOURCE_NAME,
    },
    .probe = stmmacphy_dvr_probe,
    .remove = stmmacphy_dvr_remove,
};

/**
 * stmmac_associate_phy
 * @dev:bus_ pointer to device structure
 * @data: points to the private structure.
 * Description: Scans through all the PHYs we have registered and checks if
 * any are associated with our MAC.  If so, then just fill in
 * the blanks in our local context structure
 */
static int stmmac_associate_phy(struct device *dev, void *data)
{
    struct stmmac_priv *priv = (struct stmmac_priv *)data;
    struct plat_stmmacphy_data *plat_dat = dev->platform_data;


    DBG(probe, DEBUG, "%s: checking phy for bus %d\n", __func__,
            plat_dat->bus_id);

    /* Check that this phy is for the MAC being initialised */
    if (priv->plat->bus_id != plat_dat->bus_id)
        return 0;

    /* OK, this PHY is connected to the MAC.
       Go ahead and get the parameters */
    DBG(probe, DEBUG, "%s: OK. Found PHY config\n", __func__);

    /* Override with kernel parameters if supplied XXX CRS XXX
     * this needs to have multiple instances */
    if ((phyaddr >= 0) && (phyaddr <= 31))
        plat_dat->phy_addr = phyaddr;

    priv->phy_addr = plat_dat->phy_addr;
    priv->phy_mask = plat_dat->phy_mask;
    priv->phy_interface = plat_dat->interface;
    priv->phy_reset = plat_dat->phy_reset;

    DBG(probe, DEBUG, "%s: exiting\n", __func__);
    return 1;	/* forces exit of driver_for_each_device() */
}

/**
 * stmmac_dvr_probe
 * @pdev: platform device pointer
 * Description: the driver is initialized through platform_device.
 */
static int stmmac_dvr_probe(struct platform_device *pdev)
{
    int ret = 0;
    struct resource *res;
    void __iomem *addr = NULL;
    struct net_device *ndev = NULL;
    struct stmmac_priv *priv = NULL;
    struct plat_stmmacenet_data *plat_dat;


    pr_info("XMMMAC driver:\n\tplatform registration... ");
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res)
        return -ENODEV;
    pr_info("\tdone!\n");

    if (!request_mem_region(res->start, resource_size(res),
                pdev->name)) {
        pr_err("%s: ERROR: memory allocation failed"
                "cannot get the I/O addr 0x%x\n",
                __func__, (unsigned int)res->start);
        return -EBUSY;
    }

    addr = ioremap(res->start, resource_size(res));
    if (!addr) {
        pr_err("%s: ERROR: memory mapping failed\n", __func__);
        ret = -ENOMEM;
        goto out_release_region;
    }

    ndev = alloc_etherdev(sizeof(struct stmmac_priv));
    if (!ndev) {
        pr_err("%s: ERROR: allocating the device\n", __func__);
        ret = -ENOMEM;
        goto out_unmap;
    }

    SET_NETDEV_DEV(ndev, &pdev->dev);

    /* Get the MAC information */
    ndev->irq = platform_get_irq_byname(pdev, "gmac_irq");
    if (ndev->irq == -ENXIO) {
        pr_err("%s: ERROR: MAC IRQ configuration "
                "information not found\n", __func__);
        ret = -ENXIO;
        goto out_free_ndev;
    }

    priv = netdev_priv(ndev);
    priv->device = &(pdev->dev);
    priv->dev = ndev;
    plat_dat = pdev->dev.platform_data;

    priv->plat = plat_dat;

    priv->ioaddr = addr;

    platform_set_drvdata(pdev, ndev);

    /* Set the I/O base addr */
    ndev->base_addr = (unsigned long)addr;

    /* Custom initialisation */
    if (priv->plat->init) {
        ret = priv->plat->init(pdev);
        if (unlikely(ret))
            goto out_free_ndev;
    }

    /* MAC HW revice detection */
    ret = stmmac_mac_device_setup(ndev);
    if (ret < 0)
        goto out_plat_exit;

    /* Network Device Registration */
    ret = stmmac_probe(ndev);
    if (ret < 0)
        goto out_plat_exit;


    /* 检查是否注册了一个phy到系统当中去,与stmmacphy_driver对应 */
    /* associate a PHY - it is provided by another platform bus */
    if (!driver_for_each_device
            (&(stmmacphy_driver.driver), NULL, (void *)priv,
             stmmac_associate_phy)) {
        pr_err("No PHY device is associated with this MAC!\n");
        ret = -ENODEV;
        goto out_unregister;
    }

    pr_info("\t%s - (dev. name: %s - id: %d, IRQ #%d\n"
            "\tIO base addr: 0x%p)\n", ndev->name, pdev->name,
            pdev->id, ndev->irq, addr);

    /* MDIO bus Registration */
    pr_debug("\tMDIO bus (id: %d)...", priv->plat->bus_id);
    ret = stmmac_mdio_register(ndev);
    if (ret < 0)
        goto out_unregister;
    pr_debug("registered!\n");
    return 0;

out_unregister:
    unregister_netdev(ndev);
out_plat_exit:
    if (priv->plat->exit)
        priv->plat->exit(pdev);
out_free_ndev:
    free_netdev(ndev);
    platform_set_drvdata(pdev, NULL);
out_unmap:
    iounmap(addr);
out_release_region:
    release_mem_region(res->start, resource_size(res));

    return ret;
}

/**
 * stmmac_dvr_remove
 * @pdev: platform device pointer
 * Description: this function resets the TX/RX processes, disables the MAC RX/TX
 * changes the link status, releases the DMA descriptor rings,
 * unregisters the MDIO bus and unmaps the allocated memory.
 */
static int stmmac_dvr_remove(struct platform_device *pdev)
{
    struct net_device *ndev = platform_get_drvdata(pdev);
    struct stmmac_priv *priv = netdev_priv(ndev);
    struct resource *res;

    pr_info("%s:\n\tremoving driver", __func__);

    priv->hw->dma->stop_rx(priv->ioaddr);
    priv->hw->dma->stop_tx(priv->ioaddr);

    stmmac_disable_mac(priv->ioaddr);

    netif_carrier_off(ndev);

    stmmac_mdio_unregister(ndev);

    if (priv->plat->exit)
        priv->plat->exit(pdev);

    platform_set_drvdata(pdev, NULL);
    unregister_netdev(ndev);

    iounmap((void *)priv->ioaddr);
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    release_mem_region(res->start, resource_size(res));

    free_netdev(ndev);

    return 0;
}

static const struct dev_pm_ops stmmac_pm_ops;

static struct platform_driver stmmac_driver = {
    .probe = stmmac_dvr_probe,
    .remove = stmmac_dvr_remove,
    .driver = {
        .name = STMMAC_RESOURCE_NAME,
        .owner = THIS_MODULE,
        .pm = &stmmac_pm_ops,
    },
};


/* PLATFORM DEVICE */
struct plat_stmmacenet_data xm550_gmac_platdata = {
    .bus_id      = 0,  /* MAC挂在哪条总线上面，默认axi为0,需要和phy的bus_id保持一致， --0*/
    .pbl        = 16,
    .clk_csr    = 0,  /* mdio的时钟频率范围， 必须确保在1-2.5M之间，传输时钟为50M，设为--0000（存在一点问题?） */
    .has_gmac   = 0,  /* 千兆-1/百兆-0, 使用百兆，--0 */
    .enh_desc   = 0,  /* 增强形描述符-1/一般描述符-0, 使用一般描述符，--0 */
    .tx_coe     = 0,  /* gmac是否支持发送L4校验，简单起见暂不支持, --0 */
    .bugged_jumbo = 0, /* 超大帧，不支持千兆，暂不支持, --0 */
    .pmt        = 0,     /* 休眠省电模式，暂不支持, --0 */
    .fix_mac_speed  = NULL,  /* 调整速度，只支持自适应，速度在硬件上面决定， --NULL */
    .bus_setup      = NULL,  /* 暂时无用， --NULL */
    .init           = NULL,  /* 平台自定义初始化函数,不需要 ,NULL*/
    .exit           = NULL,  /* 不需要, --NULL */
    .custom_cfg     = NULL,  /* 不需要， --NULL */
    .bsp_priv       = NULL   /* 不需要， --NULL */
};



static struct resource xm550_gmac_resource[] = {
    [0] = {
        .name  = "gmac_addr",
        .start = GMAC_BASE,
        .end   = GMAC_BASE + 0x10000 - 1,
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .name  = "gmac_irq",
        .start = GMAC_IRQ,
        .end   = GMAC_IRQ,
        .flags = IORESOURCE_IRQ,
    }

};
static void xm550_gmac_release(struct device *dev)
{
    return;
}
struct platform_device xm550_gmac = { 
    .name             = STMMAC_RESOURCE_NAME,
    .id               = -1, 
    .num_resources    = ARRAY_SIZE(xm550_gmac_resource),
    .resource         = xm550_gmac_resource,
    .dev = { 
        .coherent_dma_mask = DMA_BIT_MASK(32), /* 不然dma_alloc_coherent会失败 */
        .platform_data = &xm550_gmac_platdata,
        .release = xm550_gmac_release,
    }   
};

struct plat_stmmacphy_data xm550_phy_platdata = {
    0, 
    1,
    0, /* phy_mask */    
    PHY_INTERFACE_MODE_RMII,  
    NULL,
    NULL                      
};

static void xm550_phy_release(struct device *dev)
{
    return;
}
struct platform_device xm550_phy = { 
    .name  = PHY_RESOURCE_NAME,
    .id = -1 ,
    .dev = {
        .platform_data = &xm550_phy_platdata,
        .release = xm550_phy_release,
    }
};


/**
 * stmmac_init_module - Entry point for the driver
 * Description: This function is the entry point for the driver.
 */
static int __init stmmac_init_module(void)
{
    int ret;

    /* first phy,不然在gmac当中探测不到phy */
    ret = platform_device_register(&xm550_phy);
    if (ret) {
        pr_err("No MAC device registered!\n");
        return -ENODEV;
    }

    if (platform_driver_register(&stmmacphy_driver)) {
        pr_err("No PHY devices registered!\n");
        return -ENODEV;
    }

    /* and mac */
    ret = platform_driver_register(&stmmac_driver);
    if (ret) {
        pr_err("No MAC driver registered!\n");
        return -ENODEV;
    }

    ret = platform_device_register(&xm550_gmac);
    if (ret) {
        pr_err("No MAC device registered!\n");
        return -ENODEV;
    }

    return ret;
}

/**
 * stmmac_cleanup_module - Cleanup routine for the driver
 * Description: This function is the cleanup routine for the driver.
 */
static void __exit stmmac_cleanup_module(void)
{
    platform_driver_unregister(&stmmacphy_driver);
    platform_device_unregister(&xm550_phy);
    platform_driver_unregister(&stmmac_driver);
    platform_device_unregister(&xm550_gmac);
}

#ifndef MODULE
static int __init stmmac_cmdline_opt(char *str)
{
    char *opt;

    if (!str || !*str)
        return -EINVAL;
    while ((opt = strsep(&str, ",")) != NULL) {
        if (!strncmp(opt, "debug:", 6))
            strict_strtoul(opt + 6, 0, (unsigned long *)&debug);
        else if (!strncmp(opt, "phyaddr:", 8))
            strict_strtoul(opt + 8, 0, (unsigned long *)&phyaddr);
        else if (!strncmp(opt, "dma_txsize:", 11))
            strict_strtoul(opt + 11, 0,
                    (unsigned long *)&dma_txsize);
        else if (!strncmp(opt, "dma_rxsize:", 11))
            strict_strtoul(opt + 11, 0,
                    (unsigned long *)&dma_rxsize);
        else if (!strncmp(opt, "buf_sz:", 7))
            strict_strtoul(opt + 7, 0, (unsigned long *)&buf_sz);
        else if (!strncmp(opt, "tc:", 3))
            strict_strtoul(opt + 3, 0, (unsigned long *)&tc);
        else if (!strncmp(opt, "watchdog:", 9))
            strict_strtoul(opt + 9, 0, (unsigned long *)&watchdog);
        else if (!strncmp(opt, "flow_ctrl:", 10))
            strict_strtoul(opt + 10, 0,
                    (unsigned long *)&flow_ctrl);
        else if (!strncmp(opt, "pause:", 6))
            strict_strtoul(opt + 6, 0, (unsigned long *)&pause);
    }
    return 0;
}

/* 内核引导时，命令行当中是否包含stmmaceth参数，如果包含就执行后面的函数(被放到.init_setup区域) */
__setup("stmmaceth=", stmmac_cmdline_opt);
#endif

module_init(stmmac_init_module);
module_exit(stmmac_cleanup_module);

MODULE_DESCRIPTION("XMMAC 10/100/1000 Ethernet driver");
MODULE_AUTHOR("Giuseppe Cavallaro <peppe.cavallaro@st.com>");
MODULE_LICENSE("GPL");
