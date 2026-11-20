// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Hisilicon Fast Ethernet MAC Driver
 *
 * Copyright (c) 2016 HiSilicon Technologies Co., Ltd.
 */

#include <linux/bitfield.h>
#include <linux/circ_buf.h>
#include <linux/clk.h>
#include <linux/etherdevice.h>
#include <linux/interrupt.h>
#include <linux/linkmode.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of_mdio.h>
#include <linux/of_net.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/rtnetlink.h>
#include <linux/spinlock.h>
#include <linux/u64_stats_sync.h>
#include <linux/workqueue.h>

/* MAC control register list */
#define MAC_PORTSEL			0x0200
#define MAC_PORTSEL_STAT_CPU		BIT(0)
#define MAC_PORTSEL_RMII		BIT(1)
#define MAC_PORTSET			0x0208
#define MAC_PORTSET_DUPLEX_FULL		BIT(0)
#define MAC_PORTSET_LINKED		BIT(1)
#define MAC_PORTSET_SPEED_100M		BIT(2)
#define MAC_SET				0x0210
#define MAX_FRAME_SIZE			1600
#define MAX_FRAME_SIZE_MASK		GENMASK(10, 0)
#define BIT_PAUSE_EN			BIT(18)
#define BIT_CNTR_RDCLR_EN		BIT(27)
#define BIT_CNTR_CLR_ALL		BIT(26)
#define BIT_CNTR_ROLL_DIS		BIT(25)
#define FC_LEVEL			0x0348
#define FC_QLIMIT_DOWN_MASK		GENMASK(5, 0)
#define FC_QLIMIT_UP_MASK		GENMASK(13, 8)
#define FC_QLIMIT_ENA			BIT(14)
#define FC_QLIMIT_DOWN			8	/* the reset values */
#define FC_QLIMIT_UP			5
#define RX_COALESCE_SET			0x0340
#define RX_COALESCED_FRAME_OFFSET	24
#define RX_COALESCED_FRAMES		8
#define RX_COALESCED_FRAMES_MAX		31
#define RX_COALESCED_TIMER		0x74
#define QLEN_SET			0x0344
#define RX_DEPTH_OFFSET			8
#define MAX_HW_FIFO_DEPTH		64
#define HW_TX_FIFO_DEPTH		12
#define HW_RX_FIFO_DEPTH		(MAX_HW_FIFO_DEPTH - HW_TX_FIFO_DEPTH)
#define IQFRM_DES			0x0354
#define RX_FRAME_LEN_MASK		GENMASK(11, 0)
#define IQ_ADDR				0x0358
#define EQ_ADDR				0x0360
#define EQFRM_LEN			0x0364
#define ADDRQ_STAT			0x036C
#define TX_CNT_INUSE_MASK		GENMASK(5, 0)
#define BIT_TX_READY			BIT(24)
#define BIT_RX_READY			BIT(25)
/* global control register list */
#define GLB_HOSTMAC_L32			0x0000
#define GLB_HOSTMAC_H16			0x0004
#define GLB_SOFT_RESET			0x0008
#define SOFT_RESET_ALL			BIT(0)
#define GLB_FWCTRL			0x0010
#define FWCTRL_VLAN_ENABLE		BIT(0)
#define FWCTRL_FW2CPU_ENA		BIT(5)
#define FWCTRL_FWALL2CPU		BIT(7)
#define GLB_MACTCTRL			0x0014
#define MACTCTRL_UNI2CPU		BIT(1)
#define MACTCTRL_MULTI2CPU		BIT(3)
#define MACTCTRL_BROAD2CPU		BIT(5)
#define MACTCTRL_MACT_ENA		BIT(7)
#define GLB_ENDIAN_MOD			0x0018
#define ENDIAN_MOD_LITTLE		(BIT(1) | BIT(0))
#define GLB_IRQ_STAT			0x0030
#define GLB_IRQ_ENA			0x0034
#define IRQ_ENA_PORT0_MASK		GENMASK(7, 0)
#define IRQ_ENA_PORT0			BIT(18)
#define IRQ_ENA_ALL			BIT(19)
#define GLB_IRQ_RAW			0x0038
#define IRQ_INT_RX_RDY			BIT(0)
#define IRQ_INT_TX_PER_PACKET		BIT(1)
#define IRQ_INT_TX_FIFO_EMPTY		BIT(6)
#define IRQ_INT_MULTI_RXRDY		BIT(7)
#define DEF_INT_MASK			(IRQ_INT_MULTI_RXRDY | \
					IRQ_INT_TX_PER_PACKET | \
					IRQ_INT_TX_FIFO_EMPTY)
#define GLB_MAC_L32_BASE		0x0100
#define GLB_MAC_H16_BASE		0x0104
#define MACFLT_HI16_MASK		GENMASK(15, 0)
#define BIT_MACFLT_ENA			BIT(17)
#define BIT_MACFLT_FW2CPU		BIT(21)
#define GLB_MAC_H16(reg)		(GLB_MAC_H16_BASE + ((reg) * 0x8))
#define GLB_MAC_L32(reg)		(GLB_MAC_L32_BASE + ((reg) * 0x8))
#define MAX_MAC_FILTER_NUM		8
#define MAX_UNICAST_ADDRESSES		2
#define MAX_MULTICAST_ADDRESSES		(MAX_MAC_FILTER_NUM - \
					MAX_UNICAST_ADDRESSES)
/*
 * Port statistics counters, 32 bit, not cleared by a read and wrapping
 * (MAC_SET[27:25] are kept at 0). Offsets are those of the FEMAC register
 * description.
 */
enum hisi_femac_stat_id {
	HF_RX_OCTS,
	HF_RX_PKTS,
	HF_RX_BROADPKTS,
	HF_RX_MULTPKTS,
	HF_RX_UNIPKTS,
	HF_RX_ERRPKTS,
	HF_RX_CRCERR_PKTS,
	HF_RX_LENERR_PKTS,
	HF_RX_OCRCERR_PKTS,
	HF_RX_PAUSE_PKTS,
	HF_RX_FIFO_OVERFLOW,
	HF_TX_PKTS,
	HF_TX_BROADPKTS,
	HF_TX_MULTPKTS,
	HF_TX_UNIPKTS,
	HF_TX_OCTS,
	HF_TX_PAUSE_PKTS,
	HF_TX_RETRYCNT,
	HF_TX_COLCNT,
	HF_TX_LATE_COL,
	HF_TX_COL_OK,
	HF_TX_EXCESS_COL,
	HF_STAT_NUM
};

static const struct hisi_femac_stat {
	char name[ETH_GSTRING_LEN];
	u16 offset;
} hisi_femac_stats[HF_STAT_NUM] = {
	/* bytes of correct and error frames, without the preamble */
	[HF_RX_OCTS] = { "rx_octets_incl_errors", 0x0608 },
	/* all received frames, good or bad */
	[HF_RX_PKTS] = { "rx_frames_all", 0x0610 },
	/* the next three are frames of valid length and without CRC error */
	[HF_RX_BROADPKTS] = { "rx_broadcast", 0x0614 },
	[HF_RX_MULTPKTS] = { "rx_multicast", 0x0618 },
	[HF_RX_UNIPKTS] = { "rx_unicast", 0x061c },
	/* CRC errors, short and long frames, error TX frames */
	[HF_RX_ERRPKTS] = { "rx_error_frames", 0x0620 },
	/* valid length, but a CRC or an alignment error */
	[HF_RX_CRCERR_PKTS] = { "rx_crc_or_align_errors", 0x0624 },
	[HF_RX_LENERR_PKTS] = { "rx_length_errors", 0x0628 },
	[HF_RX_OCRCERR_PKTS] = { "rx_odd_nibble_crc_errors", 0x062c },
	[HF_RX_PAUSE_PKTS] = { "rx_pause_frames", 0x0630 },
	[HF_RX_FIFO_OVERFLOW] = { "rx_fifo_overflows", 0x0634 },
	/* configured TX frames, without those discarded on timeout */
	[HF_TX_PKTS] = { "tx_frames_configured", 0x0780 },
	[HF_TX_BROADPKTS] = { "tx_broadcast", 0x0784 },
	[HF_TX_MULTPKTS] = { "tx_multicast", 0x0788 },
	[HF_TX_UNIPKTS] = { "tx_unicast", 0x078c },
	/* includes retransmitted and error frames, without the preamble */
	[HF_TX_OCTS] = { "tx_octets_incl_retries", 0x0790 },
	[HF_TX_PAUSE_PKTS] = { "tx_pause_frames", 0x0794 },
	[HF_TX_RETRYCNT] = { "tx_retries", 0x0798 },
	[HF_TX_COLCNT] = { "tx_collisions", 0x079c },
	[HF_TX_LATE_COL] = { "tx_late_collisions", 0x07a0 },
	[HF_TX_COL_OK] = { "tx_collided_ok", 0x07a4 },
	/* discarded after more than 15 retransmissions */
	[HF_TX_EXCESS_COL] = { "tx_excess_collisions", 0x07a8 },
};

/* A counter wraps after 4 GiB (6 minutes at 100 Mbit/s): read at least once a minute */
#define STATS_POLL_PERIOD		(60 * HZ)

/* software tx and rx queue number, should be power of 2 */
#define TXQ_NUM				64
#define RXQ_NUM				128
#define FEMAC_POLL_WEIGHT		16

#define PHY_RESET_DELAYS_PROPERTY	"hisilicon,phy-reset-delays-us"

enum phy_reset_delays {
	PRE_DELAY,
	PULSE,
	POST_DELAY,
	DELAYS_NUM,
};

struct hisi_femac_queue {
	struct sk_buff **skb;
	dma_addr_t *dma_phys;
	int num;
	unsigned int head;
	unsigned int tail;
};

struct hisi_femac_priv {
	void __iomem *port_base;
	void __iomem *glb_base;
	struct clk *clk;
	struct reset_control *mac_rst;
	struct reset_control *phy_rst;
	u32 phy_reset_delays[DELAYS_NUM];
	u32 rx_coalesce_frames;
	/*
	 * Pause settings of ethtool -A and what is in effect. Written with
	 * phydev->lock held, which also serializes the writes of the registers
	 * with adjust_link(): the PHY is stopped whenever port_init() runs.
	 */
	bool pause_autoneg;
	bool tx_pause;
	bool tx_pause_on;
	u32 link_status;

	struct device *dev;
	struct net_device *ndev;

	struct hisi_femac_queue txq;
	struct hisi_femac_queue rxq;
	u32 tx_fifo_used_cnt;
	struct napi_struct napi;
	/* Protects the read-modify-write of GLB_IRQ_ENA */
	spinlock_t irq_lock;
	/* The clock is off and the interface was already closed by suspend */
	bool suspended;
	struct work_struct reset_work;

	/* 64 bit sums of the 32 bit port counters, see hisi_femac_stats_update() */
	struct u64_stats_sync tx_syncp;	/* writer: reclaim, under the TX lock */
	u64 tx_packets;
	u64 tx_bytes;
	struct u64_stats_sync rx_syncp;	/* writer: NAPI poll */
	u64 rx_packets;
	u64 rx_bytes;

	struct mutex stats_lock;
	struct delayed_work stats_work;
	u64 stats[HF_STAT_NUM];
	u32 stats_last[HF_STAT_NUM];
};

/* Add what the counters gained since the last call, with wrap detection */
static void hisi_femac_stats_update(struct hisi_femac_priv *priv)
{
	u32 cur;
	int i;

	for (i = 0; i < HF_STAT_NUM; i++) {
		cur = readl(priv->port_base + hisi_femac_stats[i].offset);
		priv->stats[i] += (u32)(cur - priv->stats_last[i]);
		priv->stats_last[i] = cur;
	}
}

/*
 * Start counting from the present hardware values. The port soft reset clears
 * the counters (measured on the Hi3520D V100; the data sheet does not say),
 * so this runs after every reset of the port and wherever the block may have
 * lost its state (probe, resume). Called with stats_lock held.
 */
static void hisi_femac_stats_baseline_locked(struct hisi_femac_priv *priv)
{
	int i;

	for (i = 0; i < HF_STAT_NUM; i++)
		priv->stats_last[i] =
			readl(priv->port_base + hisi_femac_stats[i].offset);
}

static void hisi_femac_stats_baseline(struct hisi_femac_priv *priv)
{
	mutex_lock(&priv->stats_lock);
	hisi_femac_stats_baseline_locked(priv);
	mutex_unlock(&priv->stats_lock);
}

static void hisi_femac_stats_work(struct work_struct *work)
{
	struct hisi_femac_priv *priv = container_of(to_delayed_work(work),
					struct hisi_femac_priv, stats_work);

	mutex_lock(&priv->stats_lock);
	hisi_femac_stats_update(priv);
	mutex_unlock(&priv->stats_lock);

	schedule_delayed_work(&priv->stats_work, STATS_POLL_PERIOD);
}

static void hisi_femac_irq_enable(struct hisi_femac_priv *priv, int irqs)
{
	unsigned long flags;
	u32 val;

	spin_lock_irqsave(&priv->irq_lock, flags);
	val = readl(priv->glb_base + GLB_IRQ_ENA);
	writel(val | irqs, priv->glb_base + GLB_IRQ_ENA);
	spin_unlock_irqrestore(&priv->irq_lock, flags);
}

static void hisi_femac_irq_disable(struct hisi_femac_priv *priv, int irqs)
{
	unsigned long flags;
	u32 val;

	spin_lock_irqsave(&priv->irq_lock, flags);
	val = readl(priv->glb_base + GLB_IRQ_ENA);
	writel(val & (~irqs), priv->glb_base + GLB_IRQ_ENA);
	spin_unlock_irqrestore(&priv->irq_lock, flags);
}

static void hisi_femac_tx_dma_unmap(struct hisi_femac_priv *priv,
				    struct sk_buff *skb, unsigned int pos)
{
	dma_addr_t dma_addr;

	dma_addr = priv->txq.dma_phys[pos];
	dma_unmap_single(priv->dev, dma_addr, skb->len, DMA_TO_DEVICE);
}

/* Called with the TX lock held, returns the number of reclaimed packets */
static unsigned int hisi_femac_xmit_reclaim_locked(struct net_device *dev)
{
	struct sk_buff *skb;
	struct hisi_femac_priv *priv = netdev_priv(dev);
	struct hisi_femac_queue *txq = &priv->txq;
	unsigned int bytes_compl = 0, pkts_compl = 0;
	u32 val;

	val = readl(priv->port_base + ADDRQ_STAT) & TX_CNT_INUSE_MASK;
	while (val < priv->tx_fifo_used_cnt) {
		skb = txq->skb[txq->tail];
		if (unlikely(!skb)) {
			netdev_err(dev, "xmitq_cnt_inuse=%d, tx_fifo_used=%d\n",
				   val, priv->tx_fifo_used_cnt);
			break;
		}
		hisi_femac_tx_dma_unmap(priv, skb, txq->tail);
		pkts_compl++;
		bytes_compl += skb->len;
		dev_kfree_skb_any(skb);

		priv->tx_fifo_used_cnt--;

		val = readl(priv->port_base + ADDRQ_STAT) & TX_CNT_INUSE_MASK;
		txq->skb[txq->tail] = NULL;
		txq->tail = (txq->tail + 1) % txq->num;
	}

	u64_stats_update_begin(&priv->tx_syncp);
	priv->tx_packets += pkts_compl;
	priv->tx_bytes += bytes_compl;
	u64_stats_update_end(&priv->tx_syncp);

	netdev_completed_queue(dev, pkts_compl, bytes_compl);

	return pkts_compl;
}

static void hisi_femac_xmit_reclaim(struct net_device *dev)
{
	netif_tx_lock(dev);

	if (hisi_femac_xmit_reclaim_locked(dev) && netif_queue_stopped(dev))
		netif_wake_queue(dev);

	netif_tx_unlock(dev);
}

/*
 * The MAC sends a pause frame when the receive queue fills up, if both the
 * pause transmit enable (MAC_SET[18], on after reset) and the traffic
 * control of the queue (FC_LEVEL.qlimit_ena, off after reset) are set. The
 * thresholds of the queue are written as well, with their reset values. The
 * MAC does not look at the link: the caller decides whether to send them.
 */
static void hisi_femac_write_tx_pause(struct hisi_femac_priv *priv, bool on)
{
	u32 val;

	val = readl(priv->port_base + MAC_SET);
	if (on)
		val |= BIT_PAUSE_EN;
	else
		val &= ~BIT_PAUSE_EN;
	writel(val, priv->port_base + MAC_SET);

	val = readl(priv->port_base + FC_LEVEL);
	val &= ~(FC_QLIMIT_DOWN_MASK | FC_QLIMIT_UP_MASK | FC_QLIMIT_ENA);
	val |= FIELD_PREP(FC_QLIMIT_DOWN_MASK, FC_QLIMIT_DOWN) |
	       FIELD_PREP(FC_QLIMIT_UP_MASK, FC_QLIMIT_UP);
	if (on)
		val |= FC_QLIMIT_ENA;
	writel(val, priv->port_base + FC_LEVEL);

	priv->tx_pause_on = on;
}

/*
 * Decide whether the MAC may send pause frames: never without a full duplex
 * link, with pause autonegotiation on a negotiated link only if the result
 * of the negotiation says so, and otherwise as ethtool -A asked. Called with
 * phydev->lock held, from adjust_link() and from set_pauseparam().
 */
static void hisi_femac_update_tx_pause(struct hisi_femac_priv *priv)
{
	struct phy_device *phy = priv->ndev->phydev;
	bool tx = false;
	bool rx;

	if (phy->link && phy->duplex == DUPLEX_FULL) {
		if (priv->pause_autoneg && phy->autoneg == AUTONEG_ENABLE)
			phy_get_pause(phy, &tx, &rx);
		else
			tx = priv->tx_pause;
	}

	if (tx != priv->tx_pause_on)
		hisi_femac_write_tx_pause(priv, tx);
}

static void hisi_femac_adjust_link(struct net_device *dev)
{
	struct hisi_femac_priv *priv = netdev_priv(dev);
	struct phy_device *phy = dev->phydev;
	u32 status = 0;

	if (phy->link)
		status |= MAC_PORTSET_LINKED;
	if (phy->duplex == DUPLEX_FULL)
		status |= MAC_PORTSET_DUPLEX_FULL;
	if (phy->speed == SPEED_100)
		status |= MAC_PORTSET_SPEED_100M;

	if ((status != priv->link_status) &&
	    ((status | priv->link_status) & MAC_PORTSET_LINKED)) {
		writel(status, priv->port_base + MAC_PORTSET);
		priv->link_status = status;
		phy_print_status(phy);
	}

	hisi_femac_update_tx_pause(priv);
}

static void hisi_femac_rx_refill(struct hisi_femac_priv *priv)
{
	struct hisi_femac_queue *rxq = &priv->rxq;
	struct sk_buff *skb;
	u32 pos;
	u32 len = MAX_FRAME_SIZE;
	dma_addr_t addr;

	pos = rxq->head;
	while (readl(priv->port_base + ADDRQ_STAT) & BIT_RX_READY) {
		if (!CIRC_SPACE(pos, rxq->tail, rxq->num))
			break;
		if (unlikely(rxq->skb[pos])) {
			netdev_err(priv->ndev, "err skb[%d]=%p\n",
				   pos, rxq->skb[pos]);
			break;
		}
		skb = netdev_alloc_skb_ip_align(priv->ndev, len);
		if (unlikely(!skb))
			break;

		addr = dma_map_single(priv->dev, skb->data, len,
				      DMA_FROM_DEVICE);
		if (dma_mapping_error(priv->dev, addr)) {
			dev_kfree_skb_any(skb);
			break;
		}
		rxq->dma_phys[pos] = addr;
		rxq->skb[pos] = skb;
		writel(addr, priv->port_base + IQ_ADDR);
		pos = (pos + 1) % rxq->num;
	}
	rxq->head = pos;
}

static int hisi_femac_rx(struct net_device *dev, int limit)
{
	struct hisi_femac_priv *priv = netdev_priv(dev);
	struct hisi_femac_queue *rxq = &priv->rxq;
	struct sk_buff *skb;
	dma_addr_t addr;
	u32 rx_pkt_info, pos, len, rx_pkts_num = 0;

	pos = rxq->tail;
	while (readl(priv->glb_base + GLB_IRQ_RAW) & IRQ_INT_RX_RDY) {
		rx_pkt_info = readl(priv->port_base + IQFRM_DES);
		len = rx_pkt_info & RX_FRAME_LEN_MASK;

		/* tell hardware we will deal with this packet */
		writel(IRQ_INT_RX_RDY, priv->glb_base + GLB_IRQ_RAW);

		rx_pkts_num++;

		skb = rxq->skb[pos];
		if (unlikely(!skb)) {
			netdev_err(dev, "rx skb NULL. pos=%d\n", pos);
			break;
		}
		rxq->skb[pos] = NULL;

		addr = rxq->dma_phys[pos];
		dma_unmap_single(priv->dev, addr, MAX_FRAME_SIZE,
				 DMA_FROM_DEVICE);

		/*
		 * The length includes the FCS. Check it before it is used:
		 * a value below the FCS size would wrap and one above the
		 * buffer size would make skb_put() panic.
		 */
		if (unlikely(len < ETH_HLEN + ETH_FCS_LEN ||
			     len > MAX_FRAME_SIZE)) {
			dev_err_ratelimited(priv->dev, "rcv len err, len = %u\n",
					    len);
			dev->stats.rx_errors++;
			dev->stats.rx_length_errors++;
			dev_kfree_skb_any(skb);
			goto next;
		}
		len -= ETH_FCS_LEN;
		skb_put(skb, len);

		skb->protocol = eth_type_trans(skb, dev);
		napi_gro_receive(&priv->napi, skb);
		u64_stats_update_begin(&priv->rx_syncp);
		priv->rx_packets++;
		priv->rx_bytes += len;
		u64_stats_update_end(&priv->rx_syncp);
next:
		pos = (pos + 1) % rxq->num;
		if (rx_pkts_num >= limit)
			break;
	}
	rxq->tail = pos;

	hisi_femac_rx_refill(priv);

	return rx_pkts_num;
}

static int hisi_femac_poll(struct napi_struct *napi, int budget)
{
	struct hisi_femac_priv *priv = container_of(napi,
					struct hisi_femac_priv, napi);
	struct net_device *dev = priv->ndev;
	int work_done = 0, task = budget;
	int ints, num, enable;

	do {
		hisi_femac_xmit_reclaim(dev);
		num = hisi_femac_rx(dev, task);
		work_done += num;
		task -= num;
		if (work_done >= budget)
			break;

		ints = readl(priv->glb_base + GLB_IRQ_RAW);
		writel(ints & DEF_INT_MASK,
		       priv->glb_base + GLB_IRQ_RAW);
	} while (ints & DEF_INT_MASK);

	if (work_done < budget) {
		napi_complete_done(napi, work_done);
		/*
		 * The handler masked the per-packet TX event as well. Keep it
		 * as the wake-up source while the queue waits for room,
		 * otherwise the queue only wakes when the hardware queue has
		 * run empty.
		 */
		enable = DEF_INT_MASK;
		if (!netif_queue_stopped(dev))
			enable &= ~IRQ_INT_TX_PER_PACKET;
		hisi_femac_irq_enable(priv, enable);
	}

	return work_done;
}

static irqreturn_t hisi_femac_interrupt(int irq, void *dev_id)
{
	int ints, ena;
	struct net_device *dev = (struct net_device *)dev_id;
	struct hisi_femac_priv *priv = netdev_priv(dev);

	/* The clock is off while suspended, and so after a failed resume */
	if (READ_ONCE(priv->suspended))
		return IRQ_NONE;

	/*
	 * The line is shared: only claim events that are raised and enabled,
	 * and nothing at all while the port is closed (close clears the
	 * port enable and leaves the event enables as they are).
	 */
	ena = readl(priv->glb_base + GLB_IRQ_ENA);
	if ((ena & (IRQ_ENA_ALL | IRQ_ENA_PORT0)) !=
	    (IRQ_ENA_ALL | IRQ_ENA_PORT0))
		return IRQ_NONE;

	ints = readl(priv->glb_base + GLB_IRQ_RAW) & ena;
	if (!(ints & DEF_INT_MASK))
		return IRQ_NONE;

	writel(ints & DEF_INT_MASK, priv->glb_base + GLB_IRQ_RAW);
	hisi_femac_irq_disable(priv, DEF_INT_MASK);
	napi_schedule(&priv->napi);

	return IRQ_HANDLED;
}

static int hisi_femac_init_queue(struct device *dev,
				 struct hisi_femac_queue *queue,
				 unsigned int num)
{
	queue->skb = devm_kcalloc(dev, num, sizeof(struct sk_buff *),
				  GFP_KERNEL);
	if (!queue->skb)
		return -ENOMEM;

	queue->dma_phys = devm_kcalloc(dev, num, sizeof(dma_addr_t),
				       GFP_KERNEL);
	if (!queue->dma_phys)
		return -ENOMEM;

	queue->num = num;
	queue->head = 0;
	queue->tail = 0;

	return 0;
}

static int hisi_femac_init_tx_and_rx_queues(struct hisi_femac_priv *priv)
{
	int ret;

	ret = hisi_femac_init_queue(priv->dev, &priv->txq, TXQ_NUM);
	if (ret)
		return ret;

	ret = hisi_femac_init_queue(priv->dev, &priv->rxq, RXQ_NUM);
	if (ret)
		return ret;

	priv->tx_fifo_used_cnt = 0;

	return 0;
}

static void hisi_femac_free_skb_rings(struct hisi_femac_priv *priv)
{
	struct hisi_femac_queue *txq = &priv->txq;
	struct hisi_femac_queue *rxq = &priv->rxq;
	struct sk_buff *skb;
	dma_addr_t dma_addr;
	u32 pos;

	pos = rxq->tail;
	while (pos != rxq->head) {
		skb = rxq->skb[pos];
		if (unlikely(!skb)) {
			netdev_err(priv->ndev, "NULL rx skb. pos=%d, head=%d\n",
				   pos, rxq->head);
			pos = (pos + 1) % rxq->num;
			continue;
		}

		dma_addr = rxq->dma_phys[pos];
		dma_unmap_single(priv->dev, dma_addr, MAX_FRAME_SIZE,
				 DMA_FROM_DEVICE);

		dev_kfree_skb_any(skb);
		rxq->skb[pos] = NULL;
		pos = (pos + 1) % rxq->num;
	}
	rxq->tail = pos;

	pos = txq->tail;
	while (pos != txq->head) {
		skb = txq->skb[pos];
		if (unlikely(!skb)) {
			netdev_err(priv->ndev, "NULL tx skb. pos=%d, head=%d\n",
				   pos, txq->head);
			pos = (pos + 1) % txq->num;
			continue;
		}
		hisi_femac_tx_dma_unmap(priv, skb, pos);
		dev_kfree_skb_any(skb);
		txq->skb[pos] = NULL;
		pos = (pos + 1) % txq->num;
	}
	txq->tail = pos;
	priv->tx_fifo_used_cnt = 0;
}

static int hisi_femac_set_hw_mac_addr(struct hisi_femac_priv *priv,
				      const unsigned char *mac)
{
	u32 reg;

	reg = mac[1] | (mac[0] << 8);
	writel(reg, priv->glb_base + GLB_HOSTMAC_H16);

	reg = mac[5] | (mac[4] << 8) | (mac[3] << 16) | (mac[2] << 24);
	writel(reg, priv->glb_base + GLB_HOSTMAC_L32);

	return 0;
}

static int hisi_femac_port_reset(struct hisi_femac_priv *priv)
{
	u32 val;

	val = readl(priv->glb_base + GLB_SOFT_RESET);
	val |= SOFT_RESET_ALL;
	writel(val, priv->glb_base + GLB_SOFT_RESET);

	usleep_range(500, 800);

	val &= ~SOFT_RESET_ALL;
	writel(val, priv->glb_base + GLB_SOFT_RESET);

	return 0;
}

static int hisi_femac_net_open(struct net_device *dev)
{
	struct hisi_femac_priv *priv = netdev_priv(dev);

	/*
	 * The soft reset clears the statistics counters: add what they counted
	 * since the last poll first, and start from zero after it.
	 */
	mutex_lock(&priv->stats_lock);
	hisi_femac_stats_update(priv);
	hisi_femac_port_reset(priv);
	hisi_femac_stats_baseline_locked(priv);
	mutex_unlock(&priv->stats_lock);
	hisi_femac_set_hw_mac_addr(priv, dev->dev_addr);
	hisi_femac_rx_refill(priv);

	netif_carrier_off(dev);
	netdev_reset_queue(dev);
	netif_start_queue(dev);
	napi_enable(&priv->napi);

	priv->link_status = 0;
	if (dev->phydev)
		phy_start(dev->phydev);

	writel(IRQ_ENA_PORT0_MASK, priv->glb_base + GLB_IRQ_RAW);
	hisi_femac_irq_enable(priv, IRQ_ENA_ALL | IRQ_ENA_PORT0 | DEF_INT_MASK);

	schedule_delayed_work(&priv->stats_work, STATS_POLL_PERIOD);

	return 0;
}

static int hisi_femac_net_close(struct net_device *dev)
{
	struct hisi_femac_priv *priv = netdev_priv(dev);

	/*
	 * Suspend has closed the interface already. If the resume failed it
	 * is still marked running, and touching the block with its clock off
	 * would hang the bus.
	 */
	if (priv->suspended)
		return 0;

	hisi_femac_irq_disable(priv, IRQ_ENA_PORT0);

	if (dev->phydev)
		phy_stop(dev->phydev);

	/*
	 * Nothing may start or wake the transmitter once the rings are freed:
	 * the poll is stopped first (it reclaims frames and wakes the queue),
	 * and netif_tx_disable() waits for a transmit that is in progress on
	 * another CPU. The carrier is off so that the watchdog stays quiet.
	 */
	netif_carrier_off(dev);
	napi_disable(&priv->napi);
	netif_tx_disable(dev);

	cancel_delayed_work_sync(&priv->stats_work);
	mutex_lock(&priv->stats_lock);
	hisi_femac_stats_update(priv);
	mutex_unlock(&priv->stats_lock);

	/* Count the frames that the hardware sent since the last reclaim */
	netif_tx_lock_bh(dev);
	hisi_femac_xmit_reclaim_locked(dev);
	netif_tx_unlock_bh(dev);

	hisi_femac_free_skb_rings(priv);

	return 0;
}

/* The hardware cannot take another frame or the software ring is full */
static bool hisi_femac_tx_full(struct hisi_femac_priv *priv)
{
	struct hisi_femac_queue *txq = &priv->txq;

	return !(readl(priv->port_base + ADDRQ_STAT) & BIT_TX_READY) ||
	       !CIRC_SPACE(txq->head, txq->tail, txq->num);
}

/*
 * Stop the queue and make sure that a completion cannot be missed: the
 * per-packet interrupt is enabled before the check for a completion that
 * came in before the queue was stopped. Called with the TX lock held.
 */
static void hisi_femac_tx_stop(struct net_device *dev)
{
	struct hisi_femac_priv *priv = netdev_priv(dev);

	hisi_femac_irq_enable(priv, IRQ_INT_TX_PER_PACKET);
	netif_stop_queue(dev);
	if (hisi_femac_xmit_reclaim_locked(dev) && !hisi_femac_tx_full(priv))
		netif_wake_queue(dev);
}

static netdev_tx_t hisi_femac_net_xmit(struct sk_buff *skb,
				       struct net_device *dev)
{
	struct hisi_femac_priv *priv = netdev_priv(dev);
	struct hisi_femac_queue *txq = &priv->txq;
	dma_addr_t addr;

	/*
	 * The queue is stopped as soon as the hardware or the ring has no room
	 * left, so there is room here. If the hardware was slow to report the
	 * state, stop the queue now and let the stack requeue the frame; the
	 * queue is stopped, so returning BUSY is allowed. With nothing in
	 * flight no completion would ever wake the queue again, so the frame
	 * is dropped instead and the queue stays running.
	 */
	if (unlikely(hisi_femac_tx_full(priv))) {
		hisi_femac_xmit_reclaim_locked(dev);
		if (hisi_femac_tx_full(priv)) {
			if (priv->tx_fifo_used_cnt) {
				hisi_femac_tx_stop(dev);
				return NETDEV_TX_BUSY;
			}
			dev_kfree_skb_any(skb);
			dev->stats.tx_dropped++;
			return NETDEV_TX_OK;
		}
	}

	addr = dma_map_single(priv->dev, skb->data,
			      skb->len, DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(priv->dev, addr))) {
		dev_kfree_skb_any(skb);
		dev->stats.tx_dropped++;
		return NETDEV_TX_OK;
	}
	txq->dma_phys[txq->head] = addr;

	txq->skb[txq->head] = skb;
	txq->head = (txq->head + 1) % txq->num;

	writel(addr, priv->port_base + EQ_ADDR);
	writel(skb->len + ETH_FCS_LEN, priv->port_base + EQFRM_LEN);

	priv->tx_fifo_used_cnt++;

	netdev_sent_queue(dev, skb->len);

	if (hisi_femac_tx_full(priv))
		hisi_femac_tx_stop(dev);

	return NETDEV_TX_OK;
}

/* Called with all TX queues frozen, so with the TX lock held */
static void hisi_femac_tx_timeout(struct net_device *dev, unsigned int txqueue)
{
	struct hisi_femac_priv *priv = netdev_priv(dev);

	netdev_warn(dev, "transmit timeout, %u frames in flight\n",
		    priv->tx_fifo_used_cnt);
	dev->stats.tx_errors++;

	/*
	 * A completion that was missed leaves the queue stopped. Reclaim what
	 * is done and let the stack try again, xmit copes with a queue that
	 * is still full.
	 */
	hisi_femac_xmit_reclaim_locked(dev);
	if (priv->tx_fifo_used_cnt) {
		/*
		 * Frames are still in flight after the time out, so the
		 * hardware does not complete them and waking the queue would
		 * only stop it again. Reset the port, which needs to sleep
		 * and so cannot be done here.
		 */
		schedule_work(&priv->reset_work);
		return;
	}
	hisi_femac_irq_enable(priv, IRQ_INT_TX_PER_PACKET);
	netif_wake_queue(dev);
}

static int hisi_femac_set_mac_address(struct net_device *dev, void *p)
{
	struct hisi_femac_priv *priv = netdev_priv(dev);
	struct sockaddr *skaddr = p;

	if (!is_valid_ether_addr(skaddr->sa_data))
		return -EADDRNOTAVAIL;

	eth_hw_addr_set(dev, skaddr->sa_data);
	dev->addr_assign_type &= ~NET_ADDR_RANDOM;

	hisi_femac_set_hw_mac_addr(priv, dev->dev_addr);

	return 0;
}

static void hisi_femac_enable_hw_addr_filter(struct hisi_femac_priv *priv,
					     unsigned int reg_n, bool enable)
{
	u32 val;

	val = readl(priv->glb_base + GLB_MAC_H16(reg_n));
	if (enable)
		val |= BIT_MACFLT_ENA;
	else
		val &= ~BIT_MACFLT_ENA;
	writel(val, priv->glb_base + GLB_MAC_H16(reg_n));
}

static void hisi_femac_set_hw_addr_filter(struct hisi_femac_priv *priv,
					  unsigned char *addr,
					  unsigned int reg_n)
{
	unsigned int high, low;
	u32 val;

	high = GLB_MAC_H16(reg_n);
	low = GLB_MAC_L32(reg_n);

	val = (addr[2] << 24) | (addr[3] << 16) | (addr[4] << 8) | addr[5];
	writel(val, priv->glb_base + low);

	val = readl(priv->glb_base + high);
	val &= ~MACFLT_HI16_MASK;
	val |= ((addr[0] << 8) | addr[1]);
	val |= (BIT_MACFLT_ENA | BIT_MACFLT_FW2CPU);
	writel(val, priv->glb_base + high);
}

static void hisi_femac_set_promisc_mode(struct hisi_femac_priv *priv,
					bool promisc_mode)
{
	u32 val;

	val = readl(priv->glb_base + GLB_FWCTRL);
	if (promisc_mode)
		val |= FWCTRL_FWALL2CPU;
	else
		val &= ~FWCTRL_FWALL2CPU;
	writel(val, priv->glb_base + GLB_FWCTRL);
}

/* Handle multiple multicast addresses (perfect filtering)*/
static void hisi_femac_set_mc_addr_filter(struct hisi_femac_priv *priv)
{
	struct net_device *dev = priv->ndev;
	u32 val;

	val = readl(priv->glb_base + GLB_MACTCTRL);
	if ((netdev_mc_count(dev) > MAX_MULTICAST_ADDRESSES) ||
	    (dev->flags & IFF_ALLMULTI)) {
		val |= MACTCTRL_MULTI2CPU;
	} else {
		int reg = MAX_UNICAST_ADDRESSES;
		int i;
		struct netdev_hw_addr *ha;

		for (i = reg; i < MAX_MAC_FILTER_NUM; i++)
			hisi_femac_enable_hw_addr_filter(priv, i, false);

		netdev_for_each_mc_addr(ha, dev) {
			hisi_femac_set_hw_addr_filter(priv, ha->addr, reg);
			reg++;
		}
		val &= ~MACTCTRL_MULTI2CPU;
	}
	writel(val, priv->glb_base + GLB_MACTCTRL);
}

/* Handle multiple unicast addresses (perfect filtering)*/
static void hisi_femac_set_uc_addr_filter(struct hisi_femac_priv *priv)
{
	struct net_device *dev = priv->ndev;
	u32 val;

	val = readl(priv->glb_base + GLB_MACTCTRL);
	if (netdev_uc_count(dev) > MAX_UNICAST_ADDRESSES) {
		val |= MACTCTRL_UNI2CPU;
	} else {
		int reg = 0;
		int i;
		struct netdev_hw_addr *ha;

		for (i = reg; i < MAX_UNICAST_ADDRESSES; i++)
			hisi_femac_enable_hw_addr_filter(priv, i, false);

		netdev_for_each_uc_addr(ha, dev) {
			hisi_femac_set_hw_addr_filter(priv, ha->addr, reg);
			reg++;
		}
		val &= ~MACTCTRL_UNI2CPU;
	}
	writel(val, priv->glb_base + GLB_MACTCTRL);
}

static void hisi_femac_net_set_rx_mode(struct net_device *dev)
{
	struct hisi_femac_priv *priv = netdev_priv(dev);

	if (dev->flags & IFF_PROMISC) {
		hisi_femac_set_promisc_mode(priv, true);
	} else {
		hisi_femac_set_promisc_mode(priv, false);
		hisi_femac_set_mc_addr_filter(priv);
		hisi_femac_set_uc_addr_filter(priv);
	}
}

/*
 * The standard counters are 64 bit sums kept by the driver, they do not touch
 * the hardware. TX frames are counted when they complete.
 */
static void hisi_femac_get_stats64(struct net_device *dev,
				   struct rtnl_link_stats64 *stats)
{
	struct hisi_femac_priv *priv = netdev_priv(dev);
	unsigned int start;

	netdev_stats_to_stats64(stats, &dev->stats);

	do {
		start = u64_stats_fetch_begin(&priv->tx_syncp);
		stats->tx_packets = priv->tx_packets;
		stats->tx_bytes = priv->tx_bytes;
	} while (u64_stats_fetch_retry(&priv->tx_syncp, start));

	do {
		start = u64_stats_fetch_begin(&priv->rx_syncp);
		stats->rx_packets = priv->rx_packets;
		stats->rx_bytes = priv->rx_bytes;
	} while (u64_stats_fetch_retry(&priv->rx_syncp, start));
}

static int hisi_femac_get_sset_count(struct net_device *dev, int sset)
{
	if (sset != ETH_SS_STATS)
		return -EOPNOTSUPP;

	return HF_STAT_NUM;
}

static void hisi_femac_get_strings(struct net_device *dev, u32 sset, u8 *data)
{
	int i;

	if (sset != ETH_SS_STATS)
		return;

	for (i = 0; i < HF_STAT_NUM; i++)
		ethtool_puts(&data, hisi_femac_stats[i].name);
}

/* The ethtool core does not call these while the device is detached (suspended) */
static void hisi_femac_get_ethtool_stats(struct net_device *dev,
					 struct ethtool_stats *stats,
					 u64 *data)
{
	struct hisi_femac_priv *priv = netdev_priv(dev);

	mutex_lock(&priv->stats_lock);
	hisi_femac_stats_update(priv);
	memcpy(data, priv->stats, sizeof(priv->stats));
	mutex_unlock(&priv->stats_lock);
}

/*
 * Only the fields that the registers define the same way are filled in. The
 * hardware CRC counter also holds alignment errors and the alignment counter
 * is "odd nibble and CRC error", so FrameCheckSequenceErrors and
 * AlignmentErrors stay unset; see rx_crc_or_align_errors and
 * rx_odd_nibble_crc_errors in ethtool -S.
 */
static void hisi_femac_get_eth_mac_stats(struct net_device *dev,
					 struct ethtool_eth_mac_stats *stats)
{
	struct hisi_femac_priv *priv = netdev_priv(dev);
	u64 *st = priv->stats;

	mutex_lock(&priv->stats_lock);
	hisi_femac_stats_update(priv);
	stats->FramesTransmittedOK = st[HF_TX_BROADPKTS] + st[HF_TX_MULTPKTS] +
				     st[HF_TX_UNIPKTS];
	stats->LateCollisions = st[HF_TX_LATE_COL];
	stats->FramesAbortedDueToXSColls = st[HF_TX_EXCESS_COL];
	stats->MulticastFramesXmittedOK = st[HF_TX_MULTPKTS];
	stats->BroadcastFramesXmittedOK = st[HF_TX_BROADPKTS];
	stats->MulticastFramesReceivedOK = st[HF_RX_MULTPKTS];
	stats->BroadcastFramesReceivedOK = st[HF_RX_BROADPKTS];
	mutex_unlock(&priv->stats_lock);
}

static void hisi_femac_get_pause_stats(struct net_device *dev,
				       struct ethtool_pause_stats *stats)
{
	struct hisi_femac_priv *priv = netdev_priv(dev);

	mutex_lock(&priv->stats_lock);
	hisi_femac_stats_update(priv);
	stats->tx_pause_frames = priv->stats[HF_TX_PAUSE_PKTS];
	stats->rx_pause_frames = priv->stats[HF_RX_PAUSE_PKTS];
	mutex_unlock(&priv->stats_lock);
}

/*
 * The MAC can send pause frames, and there is no documented control for
 * honoring received ones, so only the transmit side is offered: it is
 * advertised to the link partner (Asym_Pause without Pause) and resolved
 * against the partner's advertisement after every negotiation, or forced
 * with "autoneg off". rx is reported as off although the MAC may honor
 * received pause frames, and refused when asked for. The setting is kept in
 * the private data and applied again when the link comes up, so it survives
 * a power loss of the block.
 */
static void hisi_femac_get_pauseparam(struct net_device *dev,
				      struct ethtool_pauseparam *pause)
{
	struct hisi_femac_priv *priv = netdev_priv(dev);

	pause->autoneg = priv->pause_autoneg &&
			 dev->phydev->autoneg == AUTONEG_ENABLE;
	pause->rx_pause = 0;
	pause->tx_pause = priv->tx_pause;
}

static int hisi_femac_set_pauseparam(struct net_device *dev,
				     struct ethtool_pauseparam *pause)
{
	struct hisi_femac_priv *priv = netdev_priv(dev);
	struct phy_device *phy = dev->phydev;

	if (pause->rx_pause)
		return -EOPNOTSUPP;
	if (pause->autoneg && phy->autoneg != AUTONEG_ENABLE)
		return -EINVAL;

	mutex_lock(&phy->lock);
	priv->pause_autoneg = pause->autoneg;
	priv->tx_pause = pause->tx_pause;
	hisi_femac_update_tx_pause(priv);
	mutex_unlock(&phy->lock);

	/* Restarts the negotiation if the advertisement changed */
	phy_set_asym_pause(phy, false, pause->autoneg && pause->tx_pause);

	return 0;
}

/*
 * Only the frame count of the multi-packet RX interrupt is configurable in
 * a known unit; the aging timer counts main clock cycles / 256 and the
 * clock rate is not known to the clock driver, so it stays at its default.
 * The value is written by port_init() as well, so it survives a power loss.
 */
static int hisi_femac_get_coalesce(struct net_device *dev,
				   struct ethtool_coalesce *ec,
				   struct kernel_ethtool_coalesce *kernel_coal,
				   struct netlink_ext_ack *extack)
{
	struct hisi_femac_priv *priv = netdev_priv(dev);

	ec->rx_max_coalesced_frames = priv->rx_coalesce_frames;

	return 0;
}

static int hisi_femac_set_coalesce(struct net_device *dev,
				   struct ethtool_coalesce *ec,
				   struct kernel_ethtool_coalesce *kernel_coal,
				   struct netlink_ext_ack *extack)
{
	struct hisi_femac_priv *priv = netdev_priv(dev);

	if (ec->rx_max_coalesced_frames < 1 ||
	    ec->rx_max_coalesced_frames > RX_COALESCED_FRAMES_MAX) {
		NL_SET_ERR_MSG_FMT(extack, "rx-frames must be 1 to %d",
				   RX_COALESCED_FRAMES_MAX);
		return -EINVAL;
	}

	priv->rx_coalesce_frames = ec->rx_max_coalesced_frames;
	writel(RX_COALESCED_TIMER |
	       (priv->rx_coalesce_frames << RX_COALESCED_FRAME_OFFSET),
	       priv->port_base + RX_COALESCE_SET);

	return 0;
}

static const struct ethtool_ops hisi_femac_ethtools_ops = {
	.supported_coalesce_params = ETHTOOL_COALESCE_RX_MAX_FRAMES,
	.get_link		= ethtool_op_get_link,
	.get_ts_info		= ethtool_op_get_ts_info,
	.get_link_ksettings	= phy_ethtool_get_link_ksettings,
	.set_link_ksettings	= phy_ethtool_set_link_ksettings,
	.get_sset_count		= hisi_femac_get_sset_count,
	.get_strings		= hisi_femac_get_strings,
	.get_ethtool_stats	= hisi_femac_get_ethtool_stats,
	.get_eth_mac_stats	= hisi_femac_get_eth_mac_stats,
	.get_pause_stats	= hisi_femac_get_pause_stats,
	.nway_reset		= phy_ethtool_nway_reset,
	.get_pauseparam		= hisi_femac_get_pauseparam,
	.set_pauseparam		= hisi_femac_set_pauseparam,
	.get_coalesce		= hisi_femac_get_coalesce,
	.set_coalesce		= hisi_femac_set_coalesce,
};

static const struct net_device_ops hisi_femac_netdev_ops = {
	.ndo_open		= hisi_femac_net_open,
	.ndo_stop		= hisi_femac_net_close,
	.ndo_start_xmit		= hisi_femac_net_xmit,
	.ndo_tx_timeout		= hisi_femac_tx_timeout,
	.ndo_eth_ioctl		= phy_do_ioctl_running,
	.ndo_set_mac_address	= hisi_femac_set_mac_address,
	.ndo_set_rx_mode	= hisi_femac_net_set_rx_mode,
	.ndo_get_stats64	= hisi_femac_get_stats64,
};

static void hisi_femac_core_reset(struct hisi_femac_priv *priv)
{
	reset_control_assert(priv->mac_rst);
	reset_control_deassert(priv->mac_rst);
}

static void hisi_femac_sleep_us(u32 time_us)
{
	u32 time_ms;

	if (!time_us)
		return;

	time_ms = DIV_ROUND_UP(time_us, 1000);
	if (time_ms < 20)
		usleep_range(time_us, time_us + 500);
	else
		msleep(time_ms);
}

static void hisi_femac_phy_reset(struct hisi_femac_priv *priv)
{
	/* To make sure PHY hardware reset success,
	 * we must keep PHY in deassert state first and
	 * then complete the hardware reset operation
	 */
	reset_control_deassert(priv->phy_rst);
	hisi_femac_sleep_us(priv->phy_reset_delays[PRE_DELAY]);

	reset_control_assert(priv->phy_rst);
	/* delay some time to ensure reset ok,
	 * this depends on PHY hardware feature
	 */
	hisi_femac_sleep_us(priv->phy_reset_delays[PULSE]);
	reset_control_deassert(priv->phy_rst);
	/* delay some time to ensure later MDIO access */
	hisi_femac_sleep_us(priv->phy_reset_delays[POST_DELAY]);
}

static void hisi_femac_port_init(struct hisi_femac_priv *priv)
{
	u32 val;

	/* MAC gets link status info and phy mode by software config */
	val = MAC_PORTSEL_STAT_CPU;
	if (priv->ndev->phydev->interface == PHY_INTERFACE_MODE_RMII)
		val |= MAC_PORTSEL_RMII;
	writel(val, priv->port_base + MAC_PORTSEL);

	/*
	 * Packet data endianness of the RX write and TX read DMA. The reset
	 * value is little endian, but a boot loader may have changed it and
	 * the register is not reset by the port soft reset: set it.
	 */
	val = readl(priv->glb_base + GLB_ENDIAN_MOD);
	val &= ~ENDIAN_MOD_LITTLE;
	if (!IS_ENABLED(CONFIG_CPU_BIG_ENDIAN))
		val |= ENDIAN_MOD_LITTLE;
	writel(val, priv->glb_base + GLB_ENDIAN_MOD);

	/*clear all interrupt status */
	writel(IRQ_ENA_PORT0_MASK, priv->glb_base + GLB_IRQ_RAW);
	hisi_femac_irq_disable(priv, IRQ_ENA_PORT0_MASK | IRQ_ENA_PORT0);

	val = readl(priv->glb_base + GLB_FWCTRL);
	val &= ~(FWCTRL_VLAN_ENABLE | FWCTRL_FWALL2CPU);
	val |= FWCTRL_FW2CPU_ENA;
	writel(val, priv->glb_base + GLB_FWCTRL);

	val = readl(priv->glb_base + GLB_MACTCTRL);
	val |= (MACTCTRL_BROAD2CPU | MACTCTRL_MACT_ENA);
	writel(val, priv->glb_base + GLB_MACTCTRL);

	val = readl(priv->port_base + MAC_SET);
	val &= ~MAX_FRAME_SIZE_MASK;
	val |= MAX_FRAME_SIZE;
	/* A read does not clear the counters, and they wrap, whatever was left */
	val &= ~(BIT_CNTR_RDCLR_EN | BIT_CNTR_CLR_ALL | BIT_CNTR_ROLL_DIS);
	writel(val, priv->port_base + MAC_SET);
	/* The link is not up: adjust_link() turns it on if wanted */
	hisi_femac_write_tx_pause(priv, false);

	val = RX_COALESCED_TIMER |
		(priv->rx_coalesce_frames << RX_COALESCED_FRAME_OFFSET);
	writel(val, priv->port_base + RX_COALESCE_SET);

	val = (HW_RX_FIFO_DEPTH << RX_DEPTH_OFFSET) | HW_TX_FIFO_DEPTH;
	writel(val, priv->port_base + QLEN_SET);
}

/* Stop the port and start it again, as a suspend and resume would */
static void hisi_femac_reset_work(struct work_struct *work)
{
	struct hisi_femac_priv *priv = container_of(work,
					struct hisi_femac_priv, reset_work);
	struct net_device *dev = priv->ndev;

	rtnl_lock();
	if (netif_running(dev) && !priv->suspended) {
		netdev_err(dev, "the transmitter does not complete frames, resetting the port\n");
		hisi_femac_net_close(dev);
		hisi_femac_port_init(priv);
		hisi_femac_net_open(dev);
	}
	rtnl_unlock();
}

static int hisi_femac_drv_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct net_device *ndev;
	struct hisi_femac_priv *priv;
	struct phy_device *phy;
	struct device_node *phy_node;
	int mac_ret;
	int ret;

	ndev = alloc_etherdev(sizeof(*priv));
	if (!ndev)
		return -ENOMEM;

	platform_set_drvdata(pdev, ndev);
	SET_NETDEV_DEV(ndev, &pdev->dev);

	priv = netdev_priv(ndev);
	priv->dev = dev;
	priv->ndev = ndev;
	spin_lock_init(&priv->irq_lock);

	/*
	 * Standard sources only: "mac-address", "local-mac-address" or
	 * "address" (set by the bootloader), or an NVMEM cell named
	 * "mac-address". An NVMEM provider that is not there yet defers the
	 * probe instead of leaving the board with a random address. The
	 * lookup needs neither the clock nor a reset, so a deferral has
	 * nothing to unwind. The random address is chosen below, once the
	 * PHY no longer defers the probe.
	 */
	mac_ret = of_get_ethdev_address(node, ndev);
	if (mac_ret == -EPROBE_DEFER) {
		ret = dev_err_probe(dev, mac_ret,
				    "the MAC address is not available yet\n");
		goto out_free_netdev;
	}

	priv->port_base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(priv->port_base)) {
		ret = PTR_ERR(priv->port_base);
		goto out_free_netdev;
	}

	priv->glb_base = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(priv->glb_base)) {
		ret = PTR_ERR(priv->glb_base);
		goto out_free_netdev;
	}

	priv->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(priv->clk)) {
		ret = dev_err_probe(dev, PTR_ERR(priv->clk),
				    "failed to get clk\n");
		goto out_free_netdev;
	}

	ret = clk_prepare_enable(priv->clk);
	if (ret) {
		dev_err(dev, "failed to enable clk %d\n", ret);
		goto out_free_netdev;
	}

	priv->mac_rst = devm_reset_control_get_optional_exclusive(dev, "mac");
	if (IS_ERR(priv->mac_rst)) {
		ret = PTR_ERR(priv->mac_rst);
		goto out_disable_clk;
	}
	hisi_femac_core_reset(priv);

	priv->phy_rst = devm_reset_control_get_optional_exclusive(dev, "phy");
	if (IS_ERR(priv->phy_rst)) {
		ret = dev_err_probe(dev, PTR_ERR(priv->phy_rst),
				    "failed to get the PHY reset\n");
		goto out_disable_clk;
	}
	if (priv->phy_rst) {
		ret = of_property_read_u32_array(node,
						 PHY_RESET_DELAYS_PROPERTY,
						 priv->phy_reset_delays,
						 DELAYS_NUM);
		if (ret)
			goto out_disable_clk;
		hisi_femac_phy_reset(priv);
	}

	/*
	 * The MDIO bus may register after this device: only a PHY that is
	 * not there yet defers the probe. A failed connection (for example
	 * an invalid or unsupported "phy-mode") is an error.
	 */
	phy_node = of_parse_phandle(node, "phy-handle", 0);
	if (phy_node) {
		struct phy_device *found = of_phy_find_device(phy_node);

		of_node_put(phy_node);
		if (!found) {
			ret = dev_err_probe(dev, -EPROBE_DEFER,
					    "PHY not found yet\n");
			goto out_disable_clk;
		}
		put_device(&found->mdio.dev);
	}

	phy = of_phy_get_and_connect(ndev, node, hisi_femac_adjust_link);
	if (!phy) {
		dev_err(dev, "connect to PHY failed!\n");
		ret = -ENODEV;
		goto out_disable_clk;
	}

	phy_attached_print(phy, "phy_id=0x%.8lx, phy_mode=%s\n",
			   (unsigned long)phy->phy_id,
			   phy_modes(phy->interface));

	if (mac_ret) {
		eth_hw_addr_random(ndev);
		dev_warn(dev, "using random MAC address %pM\n",
			 ndev->dev_addr);
	}

	priv->rx_coalesce_frames = RX_COALESCED_FRAMES;
	/*
	 * No pause frames unless asked for with ethtool -A: the driver never
	 * sent any. The MAC can only send them, so that is all that is
	 * advertised (nothing, until tx is turned on), whatever the PHY
	 * advertised by default.
	 */
	phy_support_asym_pause(phy);
	linkmode_set_pause(phy->advertising, false, false);
	priv->pause_autoneg = true;
	priv->tx_pause = false;
	u64_stats_init(&priv->tx_syncp);
	u64_stats_init(&priv->rx_syncp);
	mutex_init(&priv->stats_lock);
	INIT_DELAYED_WORK(&priv->stats_work, hisi_femac_stats_work);

	ndev->watchdog_timeo = 6 * HZ;
	ndev->priv_flags |= IFF_UNICAST_FLT;
	ndev->netdev_ops = &hisi_femac_netdev_ops;
	ndev->ethtool_ops = &hisi_femac_ethtools_ops;
	netif_napi_add_weight(ndev, &priv->napi, hisi_femac_poll,
			      FEMAC_POLL_WEIGHT);
	INIT_WORK(&priv->reset_work, hisi_femac_reset_work);

	hisi_femac_port_init(priv);
	hisi_femac_stats_baseline(priv);

	ret = hisi_femac_init_tx_and_rx_queues(priv);
	if (ret)
		goto out_disconnect_phy;

	ndev->irq = platform_get_irq(pdev, 0);
	if (ndev->irq < 0) {
		ret = ndev->irq;
		goto out_disconnect_phy;
	}

	ret = devm_request_irq(dev, ndev->irq, hisi_femac_interrupt,
			       IRQF_SHARED, pdev->name, ndev);
	if (ret) {
		dev_err(dev, "devm_request_irq %d failed!\n", ndev->irq);
		goto out_disconnect_phy;
	}

	ret = register_netdev(ndev);
	if (ret) {
		dev_err(dev, "register_netdev failed!\n");
		goto out_free_irq;
	}

	return ret;

out_free_irq:
	devm_free_irq(dev, ndev->irq, ndev);
out_disconnect_phy:
	mutex_destroy(&priv->stats_lock);
	netif_napi_del(&priv->napi);
	phy_disconnect(phy);
out_disable_clk:
	clk_disable_unprepare(priv->clk);
out_free_netdev:
	free_netdev(ndev);

	return ret;
}

static void hisi_femac_drv_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct hisi_femac_priv *priv = netdev_priv(ndev);

	unregister_netdev(ndev);
	cancel_work_sync(&priv->reset_work);
	netif_napi_del(&priv->napi);

	phy_disconnect(ndev->phydev);
	/* The shared interrupt must not run once the clock is off */
	devm_free_irq(priv->dev, ndev->irq, ndev);
	mutex_destroy(&priv->stats_lock);
	/* The clock was already disabled if a resume failed */
	if (!priv->suspended)
		clk_disable_unprepare(priv->clk);
	free_netdev(ndev);
}

#ifdef CONFIG_PM
static int hisi_femac_drv_suspend(struct platform_device *pdev,
				  pm_message_t state)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct hisi_femac_priv *priv = netdev_priv(ndev);

	/*
	 * A failed resume left the device suspended with the clock off: it is
	 * still closed and detached, and the clock must not be disabled twice.
	 */
	if (priv->suspended)
		return 0;

	/*
	 * The reset work takes the rtnl lock and looks at the suspended flag,
	 * so the close and the flag are set under the same lock. The work is
	 * cancelled after the lock is released: it would wait for it.
	 */
	rtnl_lock();
	if (netif_running(ndev))
		hisi_femac_net_close(ndev);
	/* Also an interface that is down must not be opened without a clock */
	netif_device_detach(ndev);

	/*
	 * The interrupt line is shared and stays enabled for the other
	 * devices on it: the handler leaves the block alone while suspended.
	 */
	WRITE_ONCE(priv->suspended, true);
	rtnl_unlock();
	cancel_work_sync(&priv->reset_work);
	synchronize_irq(ndev->irq);
	clk_disable_unprepare(priv->clk);

	return 0;
}

static int hisi_femac_drv_resume(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct hisi_femac_priv *priv = netdev_priv(ndev);
	int ret;

	ret = clk_prepare_enable(priv->clk);
	if (ret) {
		/*
		 * Stay marked suspended and detached: with the clock off the
		 * handler, an open of the interface and the close on unbind
		 * would all hang the bus on a register read.
		 */
		dev_err(&pdev->dev, "failed to enable the clock: %d\n", ret);
		return ret;
	}
	WRITE_ONCE(priv->suspended, false);
	if (priv->phy_rst)
		hisi_femac_phy_reset(priv);
	/* Also when down: the settings must be back before the next open */
	hisi_femac_port_init(priv);
	/*
	 * The counters may have been cleared by the power loss, also if down.
	 * Read them after port_init() has fixed their mode bits.
	 */
	hisi_femac_stats_baseline(priv);
	if (netif_running(ndev))
		hisi_femac_net_open(ndev);
	netif_device_attach(ndev);

	return 0;
}
#endif

static const struct of_device_id hisi_femac_match[] = {
	{.compatible = "hisilicon,hisi-femac-v1",},
	{.compatible = "hisilicon,hisi-femac-v2",},
	{.compatible = "hisilicon,hi3516cv300-femac",},
	{},
};

MODULE_DEVICE_TABLE(of, hisi_femac_match);

static struct platform_driver hisi_femac_driver = {
	.driver = {
		.name = "hisi-femac",
		.of_match_table = hisi_femac_match,
	},
	.probe = hisi_femac_drv_probe,
	.remove = hisi_femac_drv_remove,
#ifdef CONFIG_PM
	.suspend = hisi_femac_drv_suspend,
	.resume = hisi_femac_drv_resume,
#endif
};

module_platform_driver(hisi_femac_driver);

MODULE_DESCRIPTION("Hisilicon Fast Ethernet MAC driver");
MODULE_AUTHOR("Dongpo Li <lidongpo@hisilicon.com>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:hisi-femac");
