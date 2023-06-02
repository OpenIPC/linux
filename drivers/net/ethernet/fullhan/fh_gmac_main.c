#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/errno.h>
#include <linux/in.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/ethtool.h>
#include <linux/highmem.h>
#include <linux/proc_fs.h>
#include <linux/ctype.h>
#include <linux/version.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/semaphore.h>
#include <linux/phy.h>
#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/irqreturn.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_mdio.h>
#include <linux/of_net.h>
#include <linux/of_platform.h>

#include <asm/irq.h>
#include <asm/page.h>
#include <asm/setup.h>
#include <linux/crc32.h>
#include <mach/pmu.h>
#include "fh_gmac.h"
#include <linux/gpio.h>
#include <mach/fh_gmac_plat.h>
/* Module parameters */
static int watchdog = TX_TIMEO;
static int debug = 16;      /* -1: default, 0: no output, 16:  all */
static int dma_txsize = DMA_TX_SIZE;
static int dma_rxsize = DMA_RX_SIZE;
static int flow_ctrl = FLOW_AUTO;
static int pause = PAUSE_TIME;

static unsigned int phymode = PHY_INTERFACE_MODE_RMII;
extern int auto_find_phy(Gmac_Object* gmac);

#if defined(FH_GMAC_XMIT_DEBUG) || defined(FH_GMAC_RX_DEBUG)
static void print_pkt(unsigned char *buf, int len)
{
	int j;

	printk(KERN_DEBUG "len = %d byte, buf addr: 0x%p", len, buf);
	for (j = 0; j < len; j++) {
		if ((j % 16) == 0)
			printk("\n %03x:", j);
		printk(KERN_DEBUG " %02x", buf[j]);
	}
	printk(KERN_DEBUG "\n");
}
#endif

static __u32 GMAC_BitReverse(register __u32 x)
{
	register __u32 y = 0x55555555;
	x = (((x >> 1) & y) | ((x & y) << 1));
	y = 0x33333333;
	x = (((x >> 2) & y) | ((x & y) << 2));
	y = 0x0f0f0f0f;
	x = (((x >> 4) & y) | ((x & y) << 4));
	y = 0x00ff00ff;
	x = (((x >> 8) & y) | ((x & y) << 8));
	return((x >> 16) | (x << 16));
}

static void fh_gmac_plat_init(void)
{
	fh_pmu_eth_set_speed(100);
	fh_pmu_eth_reset();
}

static void fh_gmac_set_rmii_speed(Gmac_Object *pGmac, int speed)
{
	fh_pmu_eth_set_speed(speed);
}

static void GMAC_SetMacAddress(Gmac_Object* pGmac)
{

	__u32 macHigh = pGmac->local_mac_address[5]<<8 | pGmac->local_mac_address[4];
	__u32 macLow = pGmac->local_mac_address[3]<<24 | pGmac->local_mac_address[2]<<16 | pGmac->local_mac_address[1]<<8 | pGmac->local_mac_address[0];

	writel(macHigh, pGmac->remap_addr + REG_GMAC_MAC_HIGH);
	writel(macLow, pGmac->remap_addr + REG_GMAC_MAC_LOW);
}

int gmac_dev_set_mac_addr(struct net_device *dev, void *p)
{
    Gmac_Object *pGmac = netdev_priv(dev);
    struct sockaddr *addr = p;
    memcpy(pGmac->local_mac_address, addr->sa_data, ETH_ALEN);
    GMAC_SetMacAddress(pGmac);
    return eth_mac_addr(dev, p);
}

static inline void GMAC_EnableMac(Gmac_Object* pGmac)
{
	/* transmitter enable */
	/* receive enable */
	__u32 reg = readl(pGmac->remap_addr + REG_GMAC_CONFIG);
	reg |= 0xc;
	writel(reg, pGmac->remap_addr + REG_GMAC_CONFIG);
}



static inline void GMAC_DisableMac(Gmac_Object* pGmac)
{
	/* transmitter disable */
	/* receive disable */
	__u32 reg = readl(pGmac->remap_addr + REG_GMAC_CONFIG);
	reg &= ~0xc;
	writel(reg | 0xc, pGmac->remap_addr + REG_GMAC_CONFIG);
}

static inline void GMAC_CoreInit(Gmac_Object* pGmac)
{
	//FIXME: heartbeat disable
	//auto pad or crc stripping
	__u32 reg = readl(pGmac->remap_addr + REG_GMAC_CONFIG);
	reg |= 0x80;
	writel(reg | 0xc, pGmac->remap_addr + REG_GMAC_CONFIG);
}

void GMAC_FlowCtrl(Gmac_Object* pGmac, unsigned int duplex,
			       unsigned int fc, unsigned int pause_time)
{
	__u32 flow = fc;

	if (duplex)
		flow |= (pause_time << 16);
	writel(flow, pGmac->remap_addr + REG_GMAC_FLOW_CTRL);
}

void GMAC_IsrSet(Gmac_Object* pGmac, unsigned int isr)
{
	writel(isr, pGmac->remap_addr + REG_GMAC_INTR_EN);
}

static void gmac_tx_err(Gmac_Object* pGmac)
{

	netif_stop_queue(pGmac->ndev);
	GMAC_DMA_StopTx(pGmac);
	GMAC_DMA_FreeTxSkbufs(pGmac);
	GMAC_DMA_InitTxDesc(pGmac->tx_dma_descriptors, pGmac->dma_tx_size);
	pGmac->dirty_tx = 0;
	pGmac->cur_tx = 0;
	GMAC_DMA_StartTx(pGmac);
	pGmac->ndev->stats.tx_errors++;
	netif_wake_queue(pGmac->ndev);
}


static irqreturn_t fh_gmac_interrupt(int irq, void *dev_id)
{

	struct net_device *ndev = (struct net_device *)dev_id;
	Gmac_Object *pGmac = netdev_priv(ndev);
	int status;

	if (unlikely(!ndev))
	{
		pr_err("%s: invalid ndev pointer\n", __func__);
		return IRQ_NONE;
	}


	status = GMAC_DMA_Interrupt(pGmac);
	if (likely(status == handle_tx_rx))
	{
		if (likely(napi_schedule_prep(&pGmac->napi)))
		{
			GMAC_IsrSet(pGmac, 0);
			__napi_schedule(&pGmac->napi);
		}
	}
	else if (unlikely(status & tx_hard_error_bump_tc))
	{
		//FIXME: tx underflow
	}
	else if (unlikely(status == tx_hard_error))
	{
		gmac_tx_err(pGmac);
	}

	return IRQ_HANDLED;
}

static void fh_gmac_verify_args(void)
{
	if (unlikely(watchdog < 0))
		watchdog = TX_TIMEO;
	if (unlikely(dma_rxsize < 0))
		dma_rxsize = DMA_RX_SIZE;
	if (unlikely(dma_txsize < 0))
		dma_txsize = DMA_TX_SIZE;
	if (unlikely(flow_ctrl > 1))
		flow_ctrl = FLOW_AUTO;
	else if (likely(flow_ctrl < 0))
		flow_ctrl = FLOW_OFF;
	if (unlikely((pause < 0) || (pause > 0xffff)))
		pause = PAUSE_TIME;

}

static void fh_gmac_adjust_link(struct net_device *ndev)
{
	Gmac_Object *pGmac = netdev_priv(ndev);
	struct phy_device *phydev = pGmac->phydev;
	unsigned long flags;
	int new_state = 0;

	if (phydev == NULL)
		return;

	spin_lock_irqsave(&pGmac->lock, flags);
	if (phydev->link)
	{
		__u32 ctrl = readl(pGmac->remap_addr + REG_GMAC_CONFIG);

		/* Now we make sure that we can be in full duplex mode.
		 * If not, we operate in half-duplex mode. */
		if (phydev->duplex != pGmac->oldduplex)
		{
			new_state = 1;
			if (!(phydev->duplex))
				ctrl &= ~0x800;
			else
				ctrl |= 0x800;
			pGmac->oldduplex = phydev->duplex;
		}
		/* Flow Control operation */
		if (phydev->pause)
		{
			__u32 fc = pGmac->flow_ctrl, pause_time = pGmac->pause;
			GMAC_FlowCtrl(pGmac, phydev->duplex, fc, pause_time);
		}

		if (phydev->speed != pGmac->speed)
		{
			new_state = 1;
			switch (phydev->speed)
			{
			case 100:
				ctrl |= 0x4000;
				fh_gmac_set_rmii_speed(pGmac, phydev->speed);
				break;
			case 10:
				ctrl &= ~0x4000;
				fh_gmac_set_rmii_speed(pGmac, phydev->speed);
				break;
			default:
				if (netif_msg_link(pGmac))
					pr_warning("%s: Speed (%d) is not 10"
				       " or 100!\n", ndev->name, phydev->speed);
				break;
			}

			pGmac->speed = phydev->speed;
		}
		writel(ctrl, pGmac->remap_addr + REG_GMAC_CONFIG);
		if (!pGmac->oldlink)
		{
			new_state = 1;
			pGmac->oldlink = 1;
		}
	}
	else if (pGmac->oldlink)
	{
		new_state = 1;
		pGmac->oldlink = 0;
		pGmac->speed = 0;
		pGmac->oldduplex = -1;
	}

	if (new_state && netif_msg_link(pGmac))
		phy_print_status(phydev);

	spin_unlock_irqrestore(&pGmac->lock, flags);
}


static inline void fh_gmac_rx_refill(Gmac_Object* pGmac)
{
	__u32 rxsize = pGmac->dma_rx_size;
	int bfsize = pGmac->dma_buf_sz;
	Gmac_Rx_DMA_Descriptors *desc = pGmac->rx_dma_descriptors;

	for (; pGmac->cur_rx - pGmac->dirty_rx > 0; pGmac->dirty_rx++)
	{
		__u32 entry = pGmac->dirty_rx % rxsize;
		if (likely(pGmac->rx_skbuff[entry] == NULL))
		{
			struct sk_buff *skb;

			skb = __skb_dequeue(&pGmac->rx_recycle);
			if (skb == NULL)
				skb = netdev_alloc_skb_ip_align(pGmac->ndev, bfsize);

			if (unlikely(skb == NULL))
				break;

			pGmac->rx_skbuff[entry] = skb;
			pGmac->rx_skbuff_dma[entry] =
			    dma_map_single(pGmac->dev, skb->data, bfsize,
					   DMA_FROM_DEVICE);

			(desc + entry)->desc2.dw = pGmac->rx_skbuff_dma[entry];
			RX_DBG(KERN_INFO "\trefill entry #%d\n", entry);
		}
		wmb();
		(desc+entry)->desc0.bit.own = 1;
		wmb();
	}
}

static int fh_gmac_rx(Gmac_Object* pGmac, int limit)
{
	__u32 rxsize = pGmac->dma_rx_size;
	__u32 entry = pGmac->cur_rx % rxsize;
	__u32 next_entry;
	__u32 count = 0;
	Gmac_Rx_DMA_Descriptors *desc = pGmac->rx_dma_descriptors + entry;
	Gmac_Rx_DMA_Descriptors *desc_next;

#ifdef FH_GMAC_RX_DEBUG
	if (netif_msg_hw(pGmac))
	{
		printk(">>> fh_gmac_rx: descriptor ring:\n");
		GMAC_DMA_DisplayRxDesc(pGmac->rx_dma_descriptors, rxsize);
	}
#endif
	count = 0;
	while (!desc->desc0.bit.own)
	{
		int status;

		if (count >= limit)
			break;

		count++;

		next_entry = (++pGmac->cur_rx) % rxsize;
		desc_next = pGmac->rx_dma_descriptors + next_entry;
		prefetch(desc_next);

		/* read the status of the incoming frame */
		status = (GMAC_DMA_RxStatus(pGmac, desc));
		if (unlikely(status == discard_frame))
		{
			pGmac->ndev->stats.rx_errors++;
		}
		else
		{
			struct sk_buff *skb;
			int frame_len;
			frame_len = desc->desc0.bit.frame_length;
			/* ACS is set; GMAC core strips PAD/FCS for IEEE 802.3
			 * Type frames (LLC/LLC-SNAP) */
			if (unlikely(status != llc_snap))
				frame_len -= ETH_FCS_LEN;
#ifdef FH_GMAC_RX_DEBUG
			if (frame_len > ETH_FRAME_LEN)
				pr_debug("\tRX frame size %d, COE status: %d\n",
					frame_len, status);

			if (netif_msg_hw(pGmac))
				pr_debug("\tdesc: %p [entry %d] buff=0x%x\n",
					desc, entry, desc->desc2.dw);
#endif
			skb = pGmac->rx_skbuff[entry];
			if (unlikely(!skb))
			{
				pr_err("%s: Inconsistent Rx descriptor chain\n",
						pGmac->ndev->name);
				pGmac->ndev->stats.rx_dropped++;
				break;
			}
			prefetch(skb->data - NET_IP_ALIGN);
			pGmac->rx_skbuff[entry] = NULL;

			skb_put(skb, frame_len);
			dma_unmap_single(pGmac->dev,
					pGmac->rx_skbuff_dma[entry],
					pGmac->dma_buf_sz, DMA_FROM_DEVICE);
#ifdef FH_GMAC_RX_DEBUG
			if (netif_msg_pktdata(pGmac))
			{
				pr_info(" frame received (%dbytes)", frame_len);
				print_pkt(skb->data, frame_len);
			}
#endif
			skb->protocol = eth_type_trans(skb, pGmac->ndev);

			if (unlikely(status == csum_none))
			{
				/* always for the old mac 10/100 */
				skb_checksum_none_assert(skb);
				netif_receive_skb(skb);
			}
			else
			{
				skb->ip_summed = CHECKSUM_UNNECESSARY;
				napi_gro_receive(&pGmac->napi, skb);
			}

			pGmac->ndev->stats.rx_packets++;
			pGmac->ndev->stats.rx_bytes += frame_len;
		}
		entry = next_entry;
		desc = desc_next;	/* use prefetched values */
	}

	fh_gmac_rx_refill(pGmac);

	pGmac->stats.rx_pkt_n += count;

	return count;
}

/* minimum number of free TX descriptors required to wake up TX process */
#define FH_GMAC_TX_THRESH(x)	(x->dma_tx_size/4)

static inline __u32 gmac_tx_avail(Gmac_Object* pGmac)
{
	return pGmac->dirty_tx + pGmac->dma_tx_size - pGmac->cur_tx - 1;
}



static void fh_gmac_tx(Gmac_Object* pGmac)
{
	__u32 txsize = pGmac->dma_tx_size;

	while (pGmac->dirty_tx != pGmac->cur_tx)
	{
		int last;
		__u32 entry = pGmac->dirty_tx % txsize;
		struct sk_buff *skb = pGmac->tx_skbuff[entry];
		Gmac_Tx_DMA_Descriptors *desc = pGmac->tx_dma_descriptors + entry;

		/* Check if the descriptor is owned by the DMA. */
		if (desc->desc0.bit.own)
			break;

		/* Verify tx error by looking at the last segment */
		last = desc->desc1.bit.last_segment;
		if (likely(last))
		{
			int tx_error = GMAC_DMA_TxStatus(pGmac, desc);
			if (likely(tx_error == 0))
			{
				pGmac->ndev->stats.tx_packets++;
				pGmac->stats.tx_pkt_n++;
			}
			else
			{
				pGmac->ndev->stats.tx_errors++;
			}
		}
		TX_DBG("%s: curr %d, dirty %d\n", __func__,
				pGmac->cur_tx, pGmac->dirty_tx);

		if (likely(desc->desc2.dw))
		{
			dma_unmap_single(pGmac->dev, desc->desc2.dw, desc->desc1.bit.buffer1_size,
					 DMA_TO_DEVICE);
		}
		if (unlikely(desc->desc3.dw))
		{
			desc->desc3.dw = 0;
		}

		if (likely(skb != NULL))
		{
			dev_consume_skb_any(skb);
			pGmac->tx_skbuff[entry] = NULL;
		}
		GMAC_DMA_ReleaseTxDesc(desc);

		entry = (++pGmac->dirty_tx) % txsize;
	}
	if (unlikely(netif_queue_stopped(pGmac->ndev) &&
		     gmac_tx_avail(pGmac) > FH_GMAC_TX_THRESH(pGmac)))
	{
		netif_tx_lock(pGmac->ndev);
		if (netif_queue_stopped(pGmac->ndev) &&
				gmac_tx_avail(pGmac) > FH_GMAC_TX_THRESH(pGmac))
		{
			TX_DBG("%s: restart transmit\n", __func__);
			netif_wake_queue(pGmac->ndev);
		}
		netif_tx_unlock(pGmac->ndev);
	}

}


static int fh_gmac_poll(struct napi_struct *napi, int budget)
{
	Gmac_Object* pGmac = container_of(napi, Gmac_Object, napi);
	int work_done = 0;

	pGmac->stats.poll_n++;
	fh_gmac_tx(pGmac);
	work_done = fh_gmac_rx(pGmac, budget);

	if (work_done < budget)
	{
		napi_complete(napi);
		GMAC_IsrSet(pGmac, 0x1a061);
	}
	return work_done;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
/* Polling receive - used by NETCONSOLE and other diagnostic tools
 * to allow network I/O with interrupts disabled. */
static void gmac_poll_controller(struct net_device *ndev)
{
	disable_irq(ndev->irq);
	fh_gmac_interrupt(ndev->irq, ndev);
	enable_irq(ndev->irq);
}
#endif


static __u32 gmac_handle_jumbo_frames(struct sk_buff *skb, struct net_device *dev, int checksum_insertion)
{
	//FIXME: 8K jumbo frame
	Gmac_Object* pGmac = netdev_priv(dev);
	__u32 nopaged_len = skb_headlen(skb);
	__u32 txsize = pGmac->dma_tx_size;
	__u32 entry = pGmac->cur_tx % txsize;
	Gmac_Tx_DMA_Descriptors *desc = pGmac->tx_dma_descriptors + entry;

	if (nopaged_len > BUFFER_SIZE_2K)
	{

		int buf2_size = nopaged_len - BUFFER_SIZE_2K + 1;

		desc->desc2.dw = dma_map_single(pGmac->dev, skb->data,
					BUFFER_SIZE_2K, DMA_TO_DEVICE);
		desc->desc3.dw = desc->desc2.dw + BUFFER_SIZE_2K;
		desc->desc1.bit.first_segment = 1;
		desc->desc1.bit.buffer1_size = BUFFER_SIZE_2K - 1;
		desc->desc1.bit.checksum_insertion_ctrl = 3;
		entry = (++pGmac->cur_tx) % txsize;
		desc = pGmac->tx_dma_descriptors + entry;
		desc->desc2.dw = dma_map_single(pGmac->dev,
					skb->data + BUFFER_SIZE_2K,
					buf2_size, DMA_TO_DEVICE);
		desc->desc3.dw = desc->desc2.dw + BUFFER_SIZE_2K;
		desc->desc1.bit.first_segment = 0;
		desc->desc1.bit.buffer1_size = buf2_size;
		desc->desc1.bit.checksum_insertion_ctrl = checksum_insertion;
		desc->desc0.bit.own = 1;
		pGmac->tx_skbuff[entry] = NULL;
	}
	else
	{
		desc->desc2.dw = dma_map_single(pGmac->dev, skb->data,
					nopaged_len, DMA_TO_DEVICE);
		desc->desc3.dw = desc->desc2.dw + BUFFER_SIZE_2K;
		desc->desc1.bit.first_segment = 1;
		desc->desc1.bit.buffer1_size = nopaged_len;
		desc->desc1.bit.checksum_insertion_ctrl = checksum_insertion;
	}
	return entry;
}

/* Configuration changes (passed on by ifconfig) */
static int gmac_dev_set_config(struct net_device *ndev, struct ifmap *map)
{
	if (ndev->flags & IFF_UP)	/* can't act on a running interface */
		return -EBUSY;

	/* Don't allow changing the I/O address */
	if (map->base_addr != ndev->base_addr) {
		pr_warning("%s: can't change I/O address\n", ndev->name);
		return -EOPNOTSUPP;
	}

	/* Don't allow changing the IRQ */
	if (map->irq != ndev->irq) {
		pr_warning("%s: can't change IRQ number %d\n",
				ndev->name, ndev->irq);
		return -EOPNOTSUPP;
	}

	/* ignore other fields */
	return 0;
}


static int gmac_dev_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	Gmac_Object* pGmac = netdev_priv(ndev);
	__u32 txsize = pGmac->dma_tx_size;
	__u32 entry;
	int i, csum_insertion = 0;
	int nfrags = skb_shinfo(skb)->nr_frags;
	Gmac_Tx_DMA_Descriptors *desc, *first;

	if (unlikely(gmac_tx_avail(pGmac) < nfrags + 1))
	{

		if (!netif_queue_stopped(ndev))
		{
			netif_stop_queue(ndev);
			/* This is a hard error, log it. */
			pr_err("%s: BUG! Tx Ring full when queue awake\n",
				__func__);
		}
		return NETDEV_TX_BUSY;
	}

	entry = pGmac->cur_tx % txsize;
	//fixme: debug

#ifdef FH_GMAC_XMIT_DEBUG
	if ((skb->len > ETH_FRAME_LEN) || nfrags)
		pr_info("fh gmac xmit:\n"
			   "\tskb addr %p - len: %d - nopaged_len: %d\n"
			   "\tn_frags: %d - ip_summed: %d - %s gso\n",
			   skb, skb->len, skb_headlen(skb), nfrags,
			   skb->ip_summed,
			   !skb_is_gso(skb) ? "isn't" : "is");
#endif

	csum_insertion = (skb->ip_summed == CHECKSUM_PARTIAL)? 3 : 0;

	desc = pGmac->tx_dma_descriptors + entry;
	first = desc;

#ifdef FH_GMAC_XMIT_DEBUG
	if ((nfrags > 0) || (skb->len > ETH_FRAME_LEN))
		pr_debug("stmmac xmit: skb len: %d, nopaged_len: %d,\n"
			   "\t\tn_frags: %d, ip_summed: %d\n",
			   skb->len, skb_headlen(skb), nfrags, skb->ip_summed);
#endif
	pGmac->tx_skbuff[entry] = skb;
	if (unlikely(skb->len >= BUFFER_SIZE_2K))
	{
		printk("jumbo_frames detected\n");
		entry = gmac_handle_jumbo_frames(skb, ndev, csum_insertion);
		desc = pGmac->tx_dma_descriptors + entry;
	}
	else
	{
		__u32 nopaged_len = skb_headlen(skb);

		desc->desc2.dw = dma_map_single(pGmac->dev, skb->data,
					nopaged_len, DMA_TO_DEVICE);
		desc->desc1.bit.first_segment = 1;
		desc->desc1.bit.buffer1_size = nopaged_len;
		desc->desc1.bit.checksum_insertion_ctrl = csum_insertion;
	}

	for (i = 0; i < nfrags; i++)
	{
		const skb_frag_t *frag = &skb_shinfo(skb)->frags[i];
		int len = frag->size;

		entry = (++pGmac->cur_tx) % txsize;
		desc = pGmac->tx_dma_descriptors + entry;

		TX_DBG("\t[entry %d] segment len: %d\n", entry, len);
		desc->desc2.dw = skb_frag_dma_map(pGmac->dev, frag, 0, len,
					      DMA_TO_DEVICE);
		pGmac->tx_skbuff[entry] = NULL;
		desc->desc1.bit.first_segment = 0;
		desc->desc1.bit.buffer1_size = len;
		wmb();
		desc->desc1.bit.checksum_insertion_ctrl = csum_insertion;
		desc->desc0.bit.own = 1;
		wmb();
	}

	/* Interrupt on completition only for the latest segment */
	desc->desc1.bit.last_segment = 1;
	desc->desc1.bit.intr_on_completion = 1;
	wmb();

	/* To avoid raise condition */
	first->desc0.bit.own = 1;
	wmb();
	pGmac->cur_tx++;

#ifdef FH_GMAC_XMIT_DEBUG
	if (netif_msg_pktdata(pGmac)) {
		pr_info("fh gmac xmit: current=%d, dirty=%d, entry=%d, "
			   "first=%p, nfrags=%d\n",
			   (pGmac->cur_tx % txsize), (pGmac->dirty_tx % txsize),
			   entry, first, nfrags);
		GMAC_DMA_DisplayTxDesc(pGmac->tx_dma_descriptors, 3);
		pr_info(">>> frame to be transmitted: ");
		print_pkt(skb->data, skb->len);
	}
#endif
	if (unlikely(gmac_tx_avail(pGmac) <= (MAX_SKB_FRAGS + 1)))
	{
		TX_DBG("%s: stop transmitted packets\n", __func__);
		netif_stop_queue(ndev);
	}

	ndev->stats.tx_bytes += skb->len;
	writel(0x1, pGmac->remap_addr + REG_GMAC_TX_POLL_DEMAND);

	return NETDEV_TX_OK;
}

static void gmac_dev_tx_timeout(struct net_device *ndev)
{
	Gmac_Object *pGmac = netdev_priv(ndev);

	gmac_tx_err(pGmac);
}

static int gmac_dev_change_mtu(struct net_device *ndev, int new_mtu)
{
	int max_mtu;

	if (netif_running(ndev))
	{
		pr_err("%s: must be stopped to change its MTU\n", ndev->name);
		return -EBUSY;
	}

	max_mtu = ETH_DATA_LEN;

	if ((new_mtu < 46) || (new_mtu > max_mtu))
	{
		pr_err("%s: invalid MTU, max MTU is: %d\n",
		ndev->name, max_mtu);
		return -EINVAL;
	}

	ndev->mtu = new_mtu;
	netdev_update_features(ndev);

	return 0;
}

static void gmac_set_filter(struct net_device *ndev)
{
	Gmac_Object* pGmac = netdev_priv(ndev);
	__u32 value = readl(pGmac->remap_addr + REG_GMAC_FRAME_FILTER);

	if (ndev->flags & IFF_PROMISC)
	{
		//enable Promiscuous Mode
		value = 0x1;
	}
	else if ((netdev_mc_count(ndev) > 64)	//HASH_TABLE_SIZE = 64
		   || (ndev->flags & IFF_ALLMULTI))
	{
		//enable Pass All Multicast
		value = 0x10;

		writel(0xffffffff, pGmac->remap_addr + REG_GMAC_HASH_HIGH);
		writel(0xffffffff, pGmac->remap_addr + REG_GMAC_HASH_LOW);
	}
	else if (netdev_mc_empty(ndev))
	{	/* no multicast */
		value = 0;
	}
	else
	{
		__u32 mc_filter[2];
		struct netdev_hw_addr *ha;

		/* Perfect filter mode for physical address and Hash
		filter for multicast */
		value = 0x404;

		memset(mc_filter, 0, sizeof(mc_filter));
		netdev_for_each_mc_addr(ha, ndev)
		{
			/* The upper 6 bits of the calculated CRC are used to
			 * index the contens of the hash table */
			int bit_nr = ether_crc(ETH_ALEN, ha->addr) >> 26;

			/* The most significant bit determines the register to
			 * use (H/L) while the other 5 bits determine the bit
			 * within the register. */
			mc_filter[bit_nr >> 5] |= 1 << (bit_nr & 31);
		}
		writel(GMAC_BitReverse(mc_filter[0]),
		pGmac->remap_addr + REG_GMAC_HASH_HIGH);
		writel(GMAC_BitReverse(mc_filter[1]),
		pGmac->remap_addr + REG_GMAC_HASH_LOW);
	}
	writel(value, pGmac->remap_addr + REG_GMAC_FRAME_FILTER);
}

static void gmac_dev_mcast_set(struct net_device *ndev)
{
	Gmac_Object* pGmac = netdev_priv(ndev);

	spin_lock(&pGmac->lock);
	gmac_set_filter(ndev);
	spin_unlock(&pGmac->lock);
}

static int gmac_dev_ioctl(struct net_device *ndev, struct ifreq *ifrq, int cmd)
{
	Gmac_Object* pGmac = netdev_priv(ndev);
	int ret;

	if (!netif_running(ndev))
		return -EINVAL;

	if (!pGmac->phydev)
		return -EINVAL;

	ret = phy_mii_ioctl(pGmac->phydev, ifrq, cmd);

	return ret;

}

static int fh_gmac_init_phy(struct net_device *ndev)
{
	Gmac_Object *pGmac = netdev_priv(ndev);
	struct phy_device *phydev;
	char phy_id[MII_BUS_ID_SIZE + 3];
	char bus_id[MII_BUS_ID_SIZE];

	if (pGmac->phydev == NULL)
		return -ENODEV;

	snprintf(bus_id, MII_BUS_ID_SIZE, "%x", 0);
	snprintf(phy_id, MII_BUS_ID_SIZE + 3, PHY_ID_FMT, bus_id, pGmac->phyid);
	pr_debug("fh_gmac_init_phy:  trying to attach to %s\n", phy_id);

	phydev = phy_connect(ndev, phy_id, &fh_gmac_adjust_link,
			pGmac->phy_interface);
	if (IS_ERR(phydev))
	{
		pr_err("%s: Could not attach to PHY\n", ndev->name);
		return PTR_ERR(phydev);
	}

	phydev->advertising &=
	~(SUPPORTED_1000baseT_Half | SUPPORTED_1000baseT_Full);


	//FIXME: phy_id == 0
	/*
	 * Broken HW is sometimes missing the pull-up resistor on the
	 * MDIO line, which results in reads to non-existent devices returning
	 * 0 rather than 0xffff. Catch this here and treat 0 as a non-existent
	 * device as well.
	 * Note: phydev->phy_id is the result of reading the UID PHY registers.
	 */
	if (phydev->phy_id == 0)
	{
		phy_disconnect(phydev);
		return -ENODEV;
	}
	pr_debug("fh_gmac_init_phy:  %s: attached to PHY (UID 0x%x)"
	       " Link = %d\n", ndev->name, phydev->phy_id, phydev->link);

	return 0;
}


static int gmac_dev_open(struct net_device *ndev)
{
	Gmac_Object *pGmac = netdev_priv(ndev);

	bind_no_link_process(pGmac->phydev->drv, fh_phy_reset_api);
	phy_start(pGmac->phydev);
	GMAC_DMA_StartTx(pGmac);
	GMAC_DMA_StartRx(pGmac);
	//ndev->operstate = IF_OPER_UP;
	napi_enable(&pGmac->napi);
	skb_queue_head_init(&pGmac->rx_recycle);
	netif_start_queue(ndev);
	GMAC_IsrSet(pGmac, 0x1a061);
	return 0;
}

static int gmac_dev_stop(struct net_device *ndev)
{
	Gmac_Object *pGmac = netdev_priv(ndev);

	GMAC_IsrSet(pGmac, 0);
	phy_stop(pGmac->phydev);
	netif_stop_queue(ndev);
	napi_disable(&pGmac->napi);
	skb_queue_purge(&pGmac->rx_recycle);
	/* Stop TX/RX DMA and clear the descriptors */
	GMAC_DMA_StopTx(pGmac);
	GMAC_DMA_StopRx(pGmac);
	netif_carrier_off(ndev);
	return 0;
}


static const struct net_device_ops fh_gmac_netdev_ops =
{
	.ndo_open				= gmac_dev_open,
	.ndo_stop				= gmac_dev_stop,
	.ndo_start_xmit			= gmac_dev_xmit,
	.ndo_set_rx_mode		= gmac_dev_mcast_set,
	.ndo_set_mac_address	= gmac_dev_set_mac_addr,
	.ndo_do_ioctl			= gmac_dev_ioctl,
	.ndo_tx_timeout			= gmac_dev_tx_timeout,
	.ndo_change_mtu			= gmac_dev_change_mtu,
	.ndo_fix_features		= NULL,
	.ndo_set_config			= gmac_dev_set_config,
#ifdef STMMAC_VLAN_TAG_USED
	.ndo_vlan_rx_register		= stmmac_vlan_rx_register,
#endif
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller		= gmac_poll_controller,
#endif
};

static int fh_gmac_probe(struct platform_device *pdev)
{
	int ret = 0;
	Gmac_Object *pGmac;
	struct clk *rmii_clk;
	struct net_device *ndev;
#ifdef CONFIG_USE_OF
	struct device_node *np = pdev->dev.of_node;
#else
	struct resource *regs;
	struct fh_gmac_platform_data *p_plat_data;
#endif

	pr_info("GMAC driver:\n\tplatform registration... ");

	ndev = alloc_etherdev(sizeof(Gmac_Object));
	if (!ndev)
	{
		pr_err("%s: ERROR: allocating the device\n", __func__);
		ret = -ENOMEM;
		goto out_free_ndev;
	}
	pGmac = netdev_priv(ndev);

#ifdef CONFIG_USE_OF
	ndev->irq = irq_of_parse_and_map(np, 0);
	if (!ndev->irq)
	{
		pr_err("%s: ERROR: getting resource failed"
			"cannot get IORESOURCE_IRQ\n", __func__);
		ret = -ENXIO;
		goto out_free_ndev;
	}

	/* fill in parameters for net-dev structure */

	pGmac->remap_addr = of_iomap(np, 0);
	if (!pGmac->remap_addr) {
		pr_err("%s: ERROR: memory mapping failed\n", __func__);
		ret = -ENOMEM;
		goto out_free_ndev;
	}

	ndev->base_addr = (unsigned long)pGmac->remap_addr;

	of_property_read_u32(np, "phyreset-gpio",
	&pGmac->phyreset_gpio);

#else

	p_plat_data = (struct fh_gmac_platform_data *)pdev->dev.platform_data;
	ndev->irq = platform_get_irq(pdev, 0);
	ndev->irq = irq_create_mapping(NULL, ndev->irq);

	if (!ndev->irq) {
		pr_err("%s: ERROR: getting resource failed"
		"cannot get IORESOURCE_IRQ\n", __func__);
		ret = -ENXIO;
		goto out_free_ndev;
	}
	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!regs) {
		pr_err("%s: ERROR: no resource\n", __func__);
		ret = -ENOMEM;
		goto out_free_ndev;
	}

	pGmac->remap_addr = ioremap_nocache(regs->start,
		regs->end - regs->start + 1);
	if (!pGmac->remap_addr) {
		pr_err("%s: ERROR: memory mapping failed\n", __func__);
		ret = -ENOMEM;
		goto out_free_ndev;
	}
	ndev->base_addr = (unsigned long)pGmac->remap_addr;
	pGmac->phyreset_gpio = p_plat_data->phy_reset_pin;
#endif	/* end CONFIG_USE_OF */

	pGmac->clk = clk_get(&pdev->dev, "eth_clk");
	if (IS_ERR(pGmac->clk)) {
		ret = PTR_ERR(pGmac->clk);
		goto out_unmap;
	}

	clk_prepare_enable(pGmac->clk);

	rmii_clk = clk_get(&pdev->dev, "eth_rmii_clk");
	if (!IS_ERR(rmii_clk))
		clk_prepare_enable(rmii_clk);

	/* add net_device to platform_device */
	SET_NETDEV_DEV(ndev, &pdev->dev);

	pGmac->dev = &(pdev->dev);
	pGmac->pdev = pdev;
	pGmac->ndev = ndev;

	platform_set_drvdata(pdev, ndev);
	ndev->base_addr = (unsigned long)pGmac->remap_addr;

	ether_setup(ndev);
	ndev->netdev_ops = &fh_gmac_netdev_ops;
	fh_gmac_set_ethtool_ops(ndev);

	ndev->hw_features = NETIF_F_SG | NETIF_F_IP_CSUM |
			NETIF_F_HW_CSUM | NETIF_F_RXCSUM;
	ndev->features |= ndev->hw_features | NETIF_F_HIGHDMA;
	ndev->watchdog_timeo = msecs_to_jiffies(watchdog);
	pGmac->msg_enable = netif_msg_init(debug, FH_GMAC_DEBUG);
	if (flow_ctrl)
		pGmac->flow_ctrl = FLOW_AUTO;	/* RX/TX pause on */
	pGmac->pause = pause;
	netif_napi_add(ndev, &(pGmac->napi), fh_gmac_poll, NAPI_POLL_WEIGHT);
	if (!is_valid_ether_addr(pGmac->local_mac_address)) {
		/* Use random MAC if none passed */
		random_ether_addr(pGmac->local_mac_address);
		pr_warning("\tusing random MAC address: %pM\n",
		pGmac->local_mac_address);
	}
	ndev->dev_addr = pGmac->local_mac_address;

	if (auto_find_phy(pGmac))
		pr_err("find no phy !!!!!!!");

	spin_lock_init(&pGmac->lock);
	/* Carrier starts down, phylib will bring it up */
	netif_carrier_off(ndev);
	ret = register_netdev(ndev);
	if (ret) {
		pr_err("%s: ERROR %i registering the netdevice\n", __func__, ret);
		ret = -ENODEV;
		goto out_plat_exit;
	}

	pr_info("\t%s - (dev. name: %s - id: %d, IRQ #%d\n"
		   "\t\tIO base addr: 0x%p)\n", ndev->name, pdev->name,
		   pdev->id, ndev->irq, pGmac->remap_addr);

	pGmac->phy_interface = phymode;
	/* MDIO bus Registration */
	ret = fh_mdio_register(ndev);
	if (ret < 0)
	{
		goto open_error;
	}
	fh_gmac_plat_init();
	ret = fh_gmac_init_phy(ndev);
	if (unlikely(ret)) {
		pr_err("%s: Cannot attach to PHY (error: %d)\n", __func__, ret);
		goto open_error;
	}

	fh_gmac_verify_args();
	pGmac->dma_tx_size = dma_txsize;
	pGmac->dma_rx_size = dma_rxsize;
	pGmac->dma_buf_sz = BUFFER_SIZE_2K;
	GMAC_DMA_InitDescRings(ndev);
	/* Copy the MAC addr into the HW  */
	GMAC_SetMacAddress(pGmac);
	/* Initialize the MAC Core */
	GMAC_CoreInit(pGmac);
	/* Request the IRQ lines */
	ret = request_irq(ndev->irq, fh_gmac_interrupt, IRQF_SHARED, ndev->name, ndev);
	if (unlikely(ret < 0)) {
		pr_err("%s: ERROR: allocating the IRQ %d (error: %d)\n",
			   __func__, ndev->irq, ret);
		goto open_error;
	}
	/* Enable the MAC Rx/Tx */
	GMAC_EnableMac(pGmac);
	/* Set the HW DMA mode and the COE */
	GMAC_DMA_OpMode(pGmac);
	/* Extra statistics */
	memset(&pGmac->stats, 0, sizeof(struct Gmac_Stats));
	pGmac->stats.threshold = 64;
	GMAC_DMA_Init(ndev, pGmac->tx_bus_addr, pGmac->rx_bus_addr);
	if (pGmac->phydev)
		phy_start(pGmac->phydev);
	return 0;

open_error:
	if (pGmac->phydev)
		phy_disconnect(pGmac->phydev);
out_plat_exit:

	clk_disable_unprepare(pGmac->clk);
	clk_put(pGmac->clk);
out_unmap:
	iounmap(pGmac->remap_addr);
out_free_ndev:
	unregister_netdev(ndev);
	free_netdev(ndev);
	platform_set_drvdata(pdev, NULL);

	return ret;
}

static int fh_gmac_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	Gmac_Object *pGmac = netdev_priv(ndev);
	struct resource *res;

	pr_info("%s:\n\tremoving driver", __func__);
	GMAC_DMA_StopTx(pGmac);
	GMAC_DMA_StopRx(pGmac);
	GMAC_DisableMac(pGmac);
	netif_carrier_off(ndev);
	GMAC_DMA_FreeDesc(pGmac);
	platform_set_drvdata(pdev, NULL);
	unregister_netdev(ndev);
	free_irq(ndev->irq, ndev);
	mdiobus_unregister(pGmac->mii);
	mdiobus_free(pGmac->mii);
	clk_disable_unprepare(pGmac->clk);
	clk_put(pGmac->clk);
	iounmap((void *)pGmac->remap_addr);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(res->start, resource_size(res));
	free_netdev(ndev);

	return 0;
}

#ifdef CONFIG_PM
static int fh_gmac_suspend(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	Gmac_Object *pGmac = netdev_priv(ndev);

	if (!ndev || !netif_running(ndev))
		return 0;

	spin_lock(&pGmac->lock);

	netif_device_detach(ndev);
	netif_stop_queue(ndev);
	if (pGmac->phydev)
		phy_stop(pGmac->phydev);

	napi_disable(&pGmac->napi);

	/* Stop TX/RX DMA */
	GMAC_DMA_StopTx(pGmac);
	GMAC_DMA_StopRx(pGmac);
	/* Clear the Rx/Tx descriptors */
	GMAC_DMA_InitRxDesc(pGmac->rx_dma_descriptors, pGmac->dma_rx_size);
	GMAC_DMA_InitTxDesc(pGmac->tx_dma_descriptors, pGmac->dma_tx_size);

	/* Enable Power down mode by programming the PMT regs */
	if (device_may_wakeup(pGmac->dev)) {
		//priv->hw->mac->pmt(priv->ioaddr, priv->wolopts);
	} else {
		GMAC_DisableMac(pGmac);
	}

	spin_unlock(&pGmac->lock);
	return 0;
}

static int fh_gmac_resume(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	Gmac_Object *pGmac = netdev_priv(ndev);
	u32 reg;

	reg = pGmac->mii->read(pGmac->mii, 0, 0);
	reg |= 1 << 15;
	pGmac->mii->write(pGmac->mii, 0, 0, reg);

	pGmac->mii->reset(pGmac->mii);

	if (!netif_running(ndev))
		return 0;

	spin_lock(&pGmac->lock);

	/* Power Down bit, into the PM register, is cleared
	 * automatically as soon as a magic packet or a Wake-up frame
	 * is received. Anyway, it's better to manually clear
	 * this bit because it can generate problems while resuming
	 * from another devices (e.g. serial console). */
	if (device_may_wakeup(pGmac->dev)) {
		//priv->hw->mac->pmt(priv->ioaddr, 0);
	}

	netif_device_attach(ndev);

	/* Enable the MAC and DMA */
	GMAC_EnableMac(pGmac);
	GMAC_DMA_StartTx(pGmac);
	GMAC_DMA_StartRx(pGmac);

	napi_enable(&pGmac->napi);

	if (pGmac->phydev)
		phy_start(pGmac->phydev);

	netif_start_queue(ndev);

	spin_unlock(&pGmac->lock);
	return 0;
}

static int fh_gmac_freeze(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);

	if (!ndev || !netif_running(ndev))
		return 0;

	return gmac_dev_stop(ndev);
}

static int fh_gmac_restore(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);

	if (!ndev || !netif_running(ndev))
		return 0;

	return gmac_dev_open(ndev);
}

static const struct dev_pm_ops fh_gmac_pm_ops = {
	.suspend = fh_gmac_suspend,
	.resume = fh_gmac_resume,
	.freeze = fh_gmac_freeze,
	.thaw = fh_gmac_restore,
	.restore = fh_gmac_restore,
};
#else
static const struct dev_pm_ops fh_gmac_pm_ops;
#endif /* CONFIG_PM */

static const struct of_device_id fh_gmac_of_match[] = {
	{.compatible = "fh,fh-gmac",},
	{},
};

MODULE_DEVICE_TABLE(of, fh_gmac_of_match);

static struct platform_driver fh_gmac_driver = {
	.driver = {
		.name = "fh_gmac",
		.of_match_table = fh_gmac_of_match,
	},
	.probe = fh_gmac_probe,
	.remove = fh_gmac_remove,
};

#ifdef CONFIG_DEFERRED_INIICALLS_GMAC
deferred_module_platform_driver(fh_gmac_driver);
#else
module_platform_driver(fh_gmac_driver);
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("QIN");
MODULE_DESCRIPTION("Fullhan Ethernet driver");
