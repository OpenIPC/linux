/*
 * Faraday ftgmac030 Gigabit Ethernet
 *
 * (C) Copyright 2014-2016 Faraday Technology
 * Bing-Yao Luo <bjluo@faraday-tech.com>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define pr_fmt(fmt)	KBUILD_MODNAME ": " fmt

#include <linux/crc32.h>
#include <linux/version.h>
#include <linux/dma-mapping.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <asm/setup.h>
#include <net/ip.h>
#include <net/ipv6.h>

#include "ftgmac030.h"

#define DRV_VERSION	"0.1"

#define MAX_PKT_SIZE		1518
#define RX_BUF_SIZE		PAGE_SIZE	/* must be smaller than 0x3fff */

static bool ftgmac030_tx_complete_packet(struct ftgmac030 *priv);
static int ftgmac030_alloc_rx_page(struct ftgmac030 *priv,
				   struct ftgmac030_rxdes *rxdes, gfp_t gfp);

char mdc_cycthr;
static char show_tx_pkg = 0, show_rx_pkg = 0, show_phy = 0, show_rx_dbg = 0;
static struct proc_dir_entry *ftgmac_proc = NULL, *ftgmac_tx_pkg_debug = NULL;
static struct proc_dir_entry *ftgmac_rx_pkg_debug = NULL, *ftgmac_phy_debug = NULL, *ftgmac_rx_debug = NULL, *ftgmac_reg_debug = NULL;
static char *command_line = NULL;

/* ****************************************************************************
 * array contains all platform devices
 * ****************************************************************************/ 

static u64 ftgmac030_dmamask = DMA_BIT_MASK(32);
#ifdef CONFIG_FTGMAC030_DRIVER_0
/*GMAC 0*/
static struct resource ftgmac030_0_resource[] = {
	{
         .start  = MAC_FTGMAC030_0_PA_BASE,
         .end        = MAC_FTGMAC030_0_PA_LIMIT,
         .flags  = IORESOURCE_MEM,
	},
	{
         .start  = MAC_FTGMAC030_0_IRQ,
         .end        = MAC_FTGMAC030_0_IRQ,
         .flags  = IORESOURCE_IRQ,
	},
};

static void ftgmac030_0_release(struct device *dev)
{
    return;
}

static struct platform_device ftgmac030_0_device = {
	.name	= DRV_NAME,
	.id		= 0,
	.num_resources	= ARRAY_SIZE(ftgmac030_0_resource),
	.resource	= ftgmac030_0_resource,
        .dev = {
        	.dma_mask = &ftgmac030_dmamask,
        	.coherent_dma_mask = DMA_BIT_MASK(32),
        	.release = ftgmac030_0_release,
        }, 	
};
#endif

#ifdef CONFIG_FTGMAC030_DRIVER_1
/*GMAC 1*/
static struct resource ftgmac030_1_resource[] = {
	{
         .start  = MAC_FTGMAC030_1_PA_BASE,
         .end        = MAC_FTGMAC030_1_PA_LIMIT,
         .flags  = IORESOURCE_MEM,
	},
	{
         .start  = MAC_FTGMAC030_1_IRQ,
         .end        = MAC_FTGMAC030_1_IRQ,
         .flags  = IORESOURCE_IRQ,
	},
};

static void ftgmac030_1_release(struct device *dev)
{
    return;
}

static struct platform_device ftgmac030_1_device = {
	.name	= DRV_1_NAME,
	.id		= 1,
	.num_resources	= ARRAY_SIZE(ftgmac030_1_resource),
	.resource	= ftgmac030_1_resource,
        .dev = {
        	.dma_mask = &ftgmac030_dmamask,
        	.coherent_dma_mask = DMA_BIT_MASK(32),
        	.release = ftgmac030_1_release,
        }, 	
};
#endif
/******************************************************************************
 * internal functions (hardware register access)
 *****************************************************************************/
#define INT_MASK_ALL_ENABLED	(FTGMAC030_INT_RPKT_LOST	| \
				 FTGMAC030_INT_AHB_ERR		| \
				 FTGMAC030_INT_RPKT_BUF		| \
				 FTGMAC030_INT_NO_RXBUF)

static void ftgmac030_set_rx_ring_base(struct ftgmac030 *priv, dma_addr_t addr)
{
	iowrite32(addr, priv->base + FTGMAC030_REG_RXR_BADR);
}

static void ftgmac030_set_rx_buffer_size(struct ftgmac030 *priv,
		unsigned int size)
{
	size = FTGMAC030_RBSR_SIZE(size);
	iowrite32(size, priv->base + FTGMAC030_REG_RBSR);
}

static void ftgmac030_set_normal_prio_tx_ring_base(struct ftgmac030 *priv,
						   dma_addr_t addr)
{
	iowrite32(addr, priv->base + FTGMAC030_REG_NPTXR_BADR);
}

static void ftgmac030_txdma_normal_prio_start_polling(struct ftgmac030 *priv)
{
	iowrite32(1, priv->base + FTGMAC030_REG_NPTXPD);
}

static int ftgmac030_reset_hw(struct ftgmac030 *priv)
{
	struct net_device *netdev = priv->netdev;
	int i;

	/* NOTE: reset clears all registers */
	iowrite32(FTGMAC030_MACCR_SW_RST, priv->base + FTGMAC030_REG_MACCR);
	for (i = 0; i < 5; i++) {
		unsigned int maccr;

		maccr = ioread32(priv->base + FTGMAC030_REG_MACCR);
		if (!(maccr & FTGMAC030_MACCR_SW_RST))
			return 0;

		udelay(1000);
	}

	netdev_err(netdev, "software reset failed\n");
	return -EIO;
}

static void ftgmac030_set_mac(struct ftgmac030 *priv, const unsigned char *mac)
{
	unsigned int maddr = mac[0] << 8 | mac[1];
	unsigned int laddr = mac[2] << 24 | mac[3] << 16 | mac[4] << 8 | mac[5];

	iowrite32(maddr, priv->base + FTGMAC030_REG_MAC_MADR);
	iowrite32(laddr, priv->base + FTGMAC030_REG_MAC_LADR);
}

static void ftgmac030_init_hw(struct ftgmac030 *priv)
{
	/* setup ring buffer base registers */
	ftgmac030_set_rx_ring_base(priv,
				   priv->descs_dma_addr +
				   offsetof(struct ftgmac030_descs, rxdes));
	ftgmac030_set_normal_prio_tx_ring_base(priv,
					       priv->descs_dma_addr +
					       offsetof(struct ftgmac030_descs, txdes));

	ftgmac030_set_rx_buffer_size(priv, RX_BUF_SIZE);

	iowrite32(FTGMAC030_APTC_RXPOLL_CNT(0xF), priv->base + FTGMAC030_REG_APTC);
	set_MDC_CLK(priv);
#if 1
    {
	unsigned int value;        
  // enable flow control
	value = ioread32(priv->base + FTGMAC030_REG_FCR);			
	iowrite32(value | 0x1, priv->base + FTGMAC030_REG_FCR);

  // enable back pressure register
	value = ioread32(priv->base + FTGMAC030_REG_BPR);			
	iowrite32(value | 0x1, priv->base + FTGMAC030_REG_BPR);
    }
#endif
	ftgmac030_set_mac(priv, priv->netdev->dev_addr);
}

#if 1
#define MACCR_ENABLE_ALL	(FTGMAC030_MACCR_TXDMA_EN	| \
				 FTGMAC030_MACCR_RXDMA_EN	| \
				 FTGMAC030_MACCR_TXMAC_EN	| \
				 FTGMAC030_MACCR_RXMAC_EN	| \
				 FTGMAC030_MACCR_RX_ALL         | \
				 FTGMAC030_MACCR_CRC_APD	| \
				 FTGMAC030_MACCR_RX_RUNT	| \
				 FTGMAC030_MACCR_RX_BROADPKT)
#else
#define MACCR_ENABLE_ALL	(FTGMAC030_MACCR_RX_ALL         | \
				 FTGMAC030_MACCR_FULLDUP	| \
				 FTGMAC030_MACCR_CRC_APD	| \
				 FTGMAC030_MACCR_RX_RUNT	| \
				 FTGMAC030_MACCR_RX_BROADPKT)

#define MACCR_ENABLE_DMA	(FTGMAC030_MACCR_TXDMA_EN	| \
				 FTGMAC030_MACCR_RXDMA_EN	| \
				 FTGMAC030_MACCR_TXMAC_EN	| \
				 FTGMAC030_MACCR_RXMAC_EN)	 
#endif

static void ftgmac030_set_fifo_size(struct ftgmac030 *priv)
{
	int tfifo, rfifo, tpafcr;
	
	tfifo = FTGMAC030_FEAR_TFIFO_RSIZE(ioread32(priv->base + FTGMAC030_REG_FEAR));
	rfifo = FTGMAC030_FEAR_RFIFO_RSIZE(ioread32(priv->base + FTGMAC030_REG_FEAR));
	
	tpafcr = ioread32(priv->base + FTGMAC030_REG_TPAFCR) & 0x00FFFFFF;
	tpafcr = tpafcr | (tfifo << 27) | (rfifo << 24);
	iowrite32(tpafcr, priv->base + FTGMAC030_REG_TPAFCR);
}

static void ftgmac030_start_hw(struct ftgmac030 *priv, int speed, int duplex)
{
	int maccr = MACCR_ENABLE_ALL;

	switch (speed) {
	default:
	case 10:
		break;

	case 100:
		maccr |= FTGMAC030_MACCR_FAST_MODE;
		break;

	case 1000:
		maccr |= FTGMAC030_MACCR_GIGA_MODE;
		break;
	}

	if(duplex)
		maccr |= FTGMAC030_MACCR_FULLDUP;

	iowrite32(maccr, priv->base + FTGMAC030_REG_MACCR);
#if 0		
	maccr |= MACCR_ENABLE_DMA;
	iowrite32(maccr, priv->base + FTGMAC030_REG_MACCR);
#endif
	ftgmac030_set_fifo_size(priv);
}

static void ftgmac030_stop_hw(struct ftgmac030 *priv)
{
#if 1
	u32 maccr;
	struct net_device *netdev = priv->netdev;
	unsigned long deadline;

	maccr = ioread32(priv->base + FTGMAC030_REG_MACCR);
	/* Stop TX DMA and RX MAC */
	maccr &= ~(FTGMAC030_MACCR_TXDMA_EN | FTGMAC030_MACCR_RXMAC_EN);
	iowrite32(maccr, priv->base + FTGMAC030_REG_MACCR);

	deadline = jiffies + HZ;
	while (1) {	
		unsigned long now;
		u32 fifo;

		now = jiffies;
		fifo = ioread32(priv->base + FTGMAC030_REG_DMAFIFOS);
		if ((fifo & FTGMAC030_DMAFIFOS_TXFIFO_EMPTY) &&
		    (fifo & FTGMAC030_DMAFIFOS_RXFIFO_EMPTY)) {
			break;

		}

		if (time_after(now, deadline)) {
			netdev_err(netdev, "Wait FIFO empty timeout -> 0x%08x\n", fifo);
			break;
		}
	}

	/* Stop transmission to ethernet and RX DMA */
	maccr &= ~(FTGMAC030_MACCR_RXDMA_EN | FTGMAC030_MACCR_TXMAC_EN);
	iowrite32(maccr, priv->base + FTGMAC030_REG_MACCR);
#else
	iowrite32(0, priv->base + FTGMAC030_REG_MACCR);
#endif
}

/******************************************************************************
 * internal functions (receive descriptor)
 *****************************************************************************/
static bool ftgmac030_rxdes_first_segment(struct ftgmac030_rxdes *rxdes)
{
	return rxdes->rxdes0 & cpu_to_le32(FTGMAC030_RXDES0_FRS);
}

static bool ftgmac030_rxdes_last_segment(struct ftgmac030_rxdes *rxdes)
{
	return rxdes->rxdes0 & cpu_to_le32(FTGMAC030_RXDES0_LRS);
}

static bool ftgmac030_rxdes_packet_ready(struct ftgmac030_rxdes *rxdes)
{
	return rxdes->rxdes0 & cpu_to_le32(FTGMAC030_RXDES0_RXPKT_RDY);
}

static void ftgmac030_rxdes_set_dma_own(struct ftgmac030_rxdes *rxdes)
{
	/* clear status bits */
	rxdes->rxdes0 &= cpu_to_le32(FTGMAC030_RXDES0_EDORR);
}

static bool ftgmac030_rxdes_rx_error(struct ftgmac030_rxdes *rxdes)
{
	return rxdes->rxdes0 & cpu_to_le32(FTGMAC030_RXDES0_RX_ERR);
}

static bool ftgmac030_rxdes_fifo_error(struct ftgmac030_rxdes *rxdes)
{
    return rxdes->rxdes0 & cpu_to_le32(FTGMAC030_RXDES0_FIFO_FULL);
}

static bool ftgmac030_rxdes_crc_error(struct ftgmac030_rxdes *rxdes)
{
	return rxdes->rxdes0 & cpu_to_le32(FTGMAC030_RXDES0_CRC_ERR);
}

static bool ftgmac030_rxdes_frame_too_long(struct ftgmac030_rxdes *rxdes)
{
	return rxdes->rxdes0 & cpu_to_le32(FTGMAC030_RXDES0_FTL);
}

static bool ftgmac030_rxdes_runt(struct ftgmac030_rxdes *rxdes)
{
	return rxdes->rxdes0 & cpu_to_le32(FTGMAC030_RXDES0_RUNT);
}

static bool ftgmac030_rxdes_odd_nibble(struct ftgmac030_rxdes *rxdes)
{
	return rxdes->rxdes0 & cpu_to_le32(FTGMAC030_RXDES0_RX_ODD_NB);
}

static unsigned int ftgmac030_rxdes_data_length(struct ftgmac030_rxdes *rxdes)
{
	return le32_to_cpu(rxdes->rxdes0) & FTGMAC030_RXDES0_VDBC;
}

static bool ftgmac030_rxdes_multicast(struct ftgmac030_rxdes *rxdes)
{
	return rxdes->rxdes0 & cpu_to_le32(FTGMAC030_RXDES0_MULTICAST);
}

static void ftgmac030_rxdes_set_end_of_ring(struct ftgmac030_rxdes *rxdes)
{
	rxdes->rxdes0 |= cpu_to_le32(FTGMAC030_RXDES0_EDORR);
}

static void ftgmac030_rxdes_set_dma_addr(struct ftgmac030_rxdes *rxdes,
					 dma_addr_t addr)
{
	rxdes->rxdes3 = cpu_to_le32(addr);
}

static dma_addr_t ftgmac030_rxdes_get_dma_addr(struct ftgmac030_rxdes *rxdes)
{
	return le32_to_cpu(rxdes->rxdes3);
}

static bool ftgmac030_rxdes_is_tcp(struct ftgmac030_rxdes *rxdes)
{
	return (rxdes->rxdes1 & cpu_to_le32(FTGMAC030_RXDES1_PROT_MASK)) ==
	       cpu_to_le32(FTGMAC030_RXDES1_PROT_TCPIP);
}

static bool ftgmac030_rxdes_is_udp(struct ftgmac030_rxdes *rxdes)
{
	return (rxdes->rxdes1 & cpu_to_le32(FTGMAC030_RXDES1_PROT_MASK)) ==
	       cpu_to_le32(FTGMAC030_RXDES1_PROT_UDPIP);
}

static bool ftgmac030_rxdes_tcpcs_err(struct ftgmac030_rxdes *rxdes)
{
	return rxdes->rxdes1 & cpu_to_le32(FTGMAC030_RXDES1_TCP_CHKSUM_ERR);
}

static bool ftgmac030_rxdes_udpcs_err(struct ftgmac030_rxdes *rxdes)
{
	return rxdes->rxdes1 & cpu_to_le32(FTGMAC030_RXDES1_UDP_CHKSUM_ERR);
}

static bool ftgmac030_rxdes_ipcs_err(struct ftgmac030_rxdes *rxdes)
{
	return rxdes->rxdes1 & cpu_to_le32(FTGMAC030_RXDES1_IP_CHKSUM_ERR);
}

/*
 * rxdes2 is not used by hardware. We use it to keep track of page.
 * Since hardware does not touch it, we can skip cpu_to_le32()/le32_to_cpu().
 */
static void ftgmac030_rxdes_set_page(struct ftgmac030_rxdes *rxdes, struct page *page)
{
	rxdes->rxdes2 = (unsigned int)page;
}

static struct page *ftgmac030_rxdes_get_page(struct ftgmac030_rxdes *rxdes)
{
	return (struct page *)rxdes->rxdes2;
}

/******************************************************************************
 * internal functions (receive)
 *****************************************************************************/
static int ftgmac030_next_rx_pointer(int pointer)
{
	return (pointer + 1) & (RX_QUEUE_ENTRIES - 1);
}

static void ftgmac030_rx_pointer_advance(struct ftgmac030 *priv)
{
	priv->rx_pointer = ftgmac030_next_rx_pointer(priv->rx_pointer);
}

static struct ftgmac030_rxdes *ftgmac030_current_rxdes(struct ftgmac030 *priv)
{
	return &priv->descs->rxdes[priv->rx_pointer];
}

static struct ftgmac030_rxdes *
ftgmac030_rx_locate_first_segment(struct ftgmac030 *priv)
{
	struct ftgmac030_rxdes *rxdes = ftgmac030_current_rxdes(priv);

	while (ftgmac030_rxdes_packet_ready(rxdes)) {
		if (ftgmac030_rxdes_first_segment(rxdes))
			return rxdes;

		ftgmac030_rxdes_set_dma_own(rxdes);
		ftgmac030_rx_pointer_advance(priv);
		rxdes = ftgmac030_current_rxdes(priv);
	}

	return NULL;
}

static bool ftgmac030_rx_packet_error(struct ftgmac030 *priv,
				      struct ftgmac030_rxdes *rxdes)
{
	struct net_device *netdev = priv->netdev;
	bool error = false;

	if (unlikely(ftgmac030_rxdes_fifo_error(rxdes))) {
		if (show_rx_dbg)
			netdev_info(netdev, "MAC rx fifo full, lose\n");

		error = true;
	} else {
    	if (unlikely(ftgmac030_rxdes_rx_error(rxdes))) {
    		if (show_rx_dbg)
    			netdev_info(netdev, "rx err\n");

    		error = true;
    	} else if (unlikely(ftgmac030_rxdes_crc_error(rxdes))) {
    		if (show_rx_dbg)
    			netdev_info(netdev, "rx crc err\n");
    
    		netdev->stats.rx_crc_errors++;
    		error = true;
    	} else if (unlikely(ftgmac030_rxdes_ipcs_err(rxdes))) {
    		if (show_rx_dbg)
    			netdev_info(netdev, "rx IP checksum err\n");
    
    		netdev->stats.rx_frame_errors++;
    		error = true;
    	} else if (unlikely(ftgmac030_rxdes_frame_too_long(rxdes))) {
    		if (show_rx_dbg)
    			netdev_info(netdev, "rx frame too long\n");
    
    		netdev->stats.rx_length_errors++;
    		error = true;
    	} else if (unlikely(ftgmac030_rxdes_runt(rxdes))) {
    		if (show_rx_dbg)
    			netdev_info(netdev, "rx runt\n");
    
    		netdev->stats.rx_length_errors++;
    		error = true;
    	} else if (unlikely(ftgmac030_rxdes_odd_nibble(rxdes))) {
    		if (show_rx_dbg)
    			netdev_info(netdev, "rx odd nibble\n");
    
    		netdev->stats.rx_length_errors++;
    		error = true;
    	}
    }

    if(error)
        netdev->stats.rx_errors++;

	return error;
}

static void ftgmac030_rx_drop_packet(struct ftgmac030 *priv)
{
	struct net_device *netdev = priv->netdev;
	struct ftgmac030_rxdes *rxdes = ftgmac030_current_rxdes(priv);
	bool done = false;

	if (show_rx_dbg)
		netdev_info(netdev, "drop packet %p\n", rxdes);

	do {
		if (ftgmac030_rxdes_last_segment(rxdes))
			done = true;

		ftgmac030_rxdes_set_dma_own(rxdes);
		ftgmac030_rx_pointer_advance(priv);
		rxdes = ftgmac030_current_rxdes(priv);
	} while (!done && ftgmac030_rxdes_packet_ready(rxdes));

	netdev->stats.rx_dropped++;
}

#ifdef CONFIG_FTGMAC030_PTP
/**
 * ftgmac030_rx_hwtstamp - utility function which checks for Rx time stamp
 * @adapter: board private structure
 * @skb: particular skb to include time stamp
 *
 * If the time stamp is valid, convert it into the timecounter ns value
 * and store that result into the shhwtstamps structure which is passed
 * up the network stack.
 **/
static void ftgmac030_rx_hwtstamp(struct ftgmac030 *adapter, struct sk_buff *skb)
{
	struct skb_shared_hwtstamps *hwtstamps = skb_hwtstamps(skb);
	unsigned long flags;
	u64 rxstmp;

	/* The Rx time stamp registers contain the time stamp.  No other
	 * received packet will be time stamped until the Rx time stamp
	 * registers are read.  Because only one packet can be time stamped
	 * at a time, the register values must belong to this packet and
	 * therefore none of the other additional attributes need to be
	 * compared.
	 */
	spin_lock_irqsave(&adapter->systim_lock, flags);
	rxstmp = ioread32(adapter->base + FTGMAC030_REG_PTP_RX_SEC) * NSEC_PER_SEC;
	rxstmp += ioread32(adapter->base + FTGMAC030_REG_PTP_RX_NSEC);
	spin_unlock_irqrestore(&adapter->systim_lock, flags);

	memset(hwtstamps, 0, sizeof(*hwtstamps));
	hwtstamps->hwtstamp = ns_to_ktime(rxstmp);

}
#endif

static bool ftgmac030_rx_packet(struct ftgmac030 *priv, int *processed)
{
	struct net_device *netdev = priv->netdev;
	struct ftgmac030_rxdes *rxdes;
	struct sk_buff *skb;
	bool done = false;
	int i;

	rxdes = ftgmac030_rx_locate_first_segment(priv);
	if (!rxdes)
		return false;

	if (unlikely(ftgmac030_rx_packet_error(priv, rxdes))) {
		ftgmac030_rx_drop_packet(priv);
		netdev->stats.rx_dropped++;
		return true;
	}

	/* start processing */
	skb = netdev_alloc_skb_ip_align(netdev, 128);
	if (unlikely(!skb)) {
		netdev_err(netdev, "rx skb alloc failed\n");

		ftgmac030_rx_drop_packet(priv);
		netdev->stats.rx_dropped++;
		return true;
	}

	if (unlikely(ftgmac030_rxdes_multicast(rxdes)))
		netdev->stats.multicast++;

	/*
	 * It seems that HW does checksum incorrectly with fragmented packets,
	 * so we are conservative here - if HW checksum error, let software do
	 * the checksum again.
	 */
	if ((ftgmac030_rxdes_is_tcp(rxdes) && !ftgmac030_rxdes_tcpcs_err(rxdes)) ||
	    (ftgmac030_rxdes_is_udp(rxdes) && !ftgmac030_rxdes_udpcs_err(rxdes)))
		skb->ip_summed = CHECKSUM_UNNECESSARY;

	do {
		dma_addr_t map = ftgmac030_rxdes_get_dma_addr(rxdes);
		struct page *page = ftgmac030_rxdes_get_page(rxdes);
		unsigned int size;

		dma_unmap_page(priv->dev, map, RX_BUF_SIZE, DMA_FROM_DEVICE);

		size = ftgmac030_rxdes_data_length(rxdes);
		skb_fill_page_desc(skb, skb_shinfo(skb)->nr_frags, page, 0, size);

		skb->len += size;
		skb->data_len += size;
		skb->truesize += PAGE_SIZE;

		if (ftgmac030_rxdes_last_segment(rxdes))
			done = true;

		ftgmac030_alloc_rx_page(priv, rxdes, GFP_ATOMIC);

		ftgmac030_rx_pointer_advance(priv);
		rxdes = ftgmac030_current_rxdes(priv);
	} while (!done);

	/* Small frames are copied into linear part of skb to free one page */
	if (skb->len <= 128) {
		skb->truesize -= PAGE_SIZE;
		__pskb_pull_tail(skb, skb->len);
	} else {
		/* We pull the minimum amount into linear part */
		__pskb_pull_tail(skb, ETH_HLEN);
	}

#ifdef CONFIG_FTGMAC030_PTP
	ftgmac030_rx_hwtstamp(priv, skb);
#endif
	skb->protocol = eth_type_trans(skb, netdev);

	netdev->stats.rx_packets++;
	netdev->stats.rx_bytes += skb->len;

	if(show_rx_pkg) {
	    printk("<rx pkg=");
	    for(i = 0; i < 10; i++)
	        printk("0x%02x ", *(skb->data + i));
        printk(">\n");
    }

	/* push packet to protocol stack */
	napi_gro_receive(&priv->napi, skb);

	(*processed)++;
	return true;
}

/******************************************************************************
 * internal functions (transmit descriptor)
 *****************************************************************************/
static void ftgmac030_txdes_reset(struct ftgmac030_txdes *txdes)
{
	/* clear all except end of ring bit */
	txdes->txdes0 &= cpu_to_le32(FTGMAC030_TXDES0_EDOTR);
	txdes->txdes1 = 0;
	txdes->txdes2 = 0;
	txdes->txdes3 = 0;
}

static bool ftgmac030_txdes_owned_by_dma(struct ftgmac030_txdes *txdes)
{
	return txdes->txdes0 & cpu_to_le32(FTGMAC030_TXDES0_TXDMA_OWN);
}

static void ftgmac030_txdes_set_dma_own(struct ftgmac030_txdes *txdes)
{
	/*
	 * Make sure dma own bit will not be set before any other
	 * descriptor fields.
	 */
	wmb();
	txdes->txdes0 |= cpu_to_le32(FTGMAC030_TXDES0_TXDMA_OWN);
}

static void ftgmac030_txdes_set_end_of_ring(struct ftgmac030_txdes *txdes)
{
	txdes->txdes0 |= cpu_to_le32(FTGMAC030_TXDES0_EDOTR);
}

static void ftgmac030_txdes_set_first_segment(struct ftgmac030_txdes *txdes)
{
	txdes->txdes0 |= cpu_to_le32(FTGMAC030_TXDES0_FTS);
}

static void ftgmac030_txdes_set_last_segment(struct ftgmac030_txdes *txdes)
{
	txdes->txdes0 |= cpu_to_le32(FTGMAC030_TXDES0_LTS);
}

static void ftgmac030_txdes_set_buffer_size(struct ftgmac030_txdes *txdes,
					    unsigned int len)
{
	txdes->txdes0 |= cpu_to_le32(FTGMAC030_TXDES0_TXBUF_SIZE(len));
}

static void ftgmac030_txdes_set_txint(struct ftgmac030_txdes *txdes)
{
	txdes->txdes1 |= cpu_to_le32(FTGMAC030_TXDES1_TXIC);
}

static void ftgmac030_txdes_set_tcpcs(struct ftgmac030_txdes *txdes)
{
	txdes->txdes1 |= cpu_to_le32(FTGMAC030_TXDES1_TCP_CHKSUM);
}

static void ftgmac030_txdes_set_udpcs(struct ftgmac030_txdes *txdes)
{
	txdes->txdes1 |= cpu_to_le32(FTGMAC030_TXDES1_UDP_CHKSUM);
}

static void ftgmac030_txdes_set_ipcs(struct ftgmac030_txdes *txdes)
{
	txdes->txdes1 |= cpu_to_le32(FTGMAC030_TXDES1_IP_CHKSUM);
}

static void ftgmac030_txdes_set_ipv6(struct ftgmac030_txdes *txdes)
{
	txdes->txdes1 |= cpu_to_le32(FTGMAC030_TXDES1_IPV6_PKT);
}

static void ftgmac030_txdes_set_dma_addr(struct ftgmac030_txdes *txdes,
					 dma_addr_t addr)
{
	txdes->txdes3 = cpu_to_le32(addr);
}

static dma_addr_t ftgmac030_txdes_get_dma_addr(struct ftgmac030_txdes *txdes)
{
	return le32_to_cpu(txdes->txdes3);
}

/*
 * txdes2 is not used by hardware. We use it to keep track of socket buffer.
 * Since hardware does not touch it, we can skip cpu_to_le32()/le32_to_cpu().
 */
static void ftgmac030_txdes_set_skb(struct ftgmac030_txdes *txdes,
				    struct sk_buff *skb)
{
	txdes->txdes2 = (unsigned int)skb;
}

static struct sk_buff *ftgmac030_txdes_get_skb(struct ftgmac030_txdes *txdes)
{
	return (struct sk_buff *)txdes->txdes2;
}

/******************************************************************************
 * internal functions (transmit)
 *****************************************************************************/
static int ftgmac030_next_tx_pointer(int pointer)
{
	return (pointer + 1) & (TX_QUEUE_ENTRIES - 1);
}

static void ftgmac030_tx_pointer_advance(struct ftgmac030 *priv)
{
	priv->tx_pointer = ftgmac030_next_tx_pointer(priv->tx_pointer);
}

static void ftgmac030_tx_clean_pointer_advance(struct ftgmac030 *priv)
{
	priv->tx_clean_pointer = ftgmac030_next_tx_pointer(priv->tx_clean_pointer);
}

static struct ftgmac030_txdes *ftgmac030_current_txdes(struct ftgmac030 *priv)
{
	return &priv->descs->txdes[priv->tx_pointer];
}

static struct ftgmac030_txdes *
ftgmac030_current_clean_txdes(struct ftgmac030 *priv)
{
	return &priv->descs->txdes[priv->tx_clean_pointer];
}

static bool ftgmac030_tx_complete_packet(struct ftgmac030 *priv)
{
	struct net_device *netdev = priv->netdev;
	struct ftgmac030_txdes *txdes;
	struct sk_buff *skb;
	dma_addr_t map;

	spin_lock(&priv->tx_lock);
	if (priv->tx_pending == 0)
		goto out;

	txdes = ftgmac030_current_clean_txdes(priv);

	if (ftgmac030_txdes_owned_by_dma(txdes))
		goto out;

	spin_unlock(&priv->tx_lock);

	skb = ftgmac030_txdes_get_skb(txdes);
	map = ftgmac030_txdes_get_dma_addr(txdes);

	netdev->stats.tx_packets++;
	netdev->stats.tx_bytes += skb->len;

	dma_unmap_single(priv->dev, map, skb_headlen(skb), DMA_TO_DEVICE);

	dev_kfree_skb(skb);

	ftgmac030_txdes_reset(txdes);

	ftgmac030_tx_clean_pointer_advance(priv);

	spin_lock(&priv->tx_lock);
	priv->tx_pending--;
	spin_unlock(&priv->tx_lock);

	return true;

out:
	spin_unlock(&priv->tx_lock);
	return false;
}

#ifdef CONFIG_FTGMAC030_PTP
/**
 * ftgmac030_tx_hwtstamp_work - check for Tx time stamp
 * @work: pointer to work struct
 *
 * This work function polls the TSYNCTXCTL valid bit to determine when a
 * timestamp has been taken for the current stored skb.  The timestamp must
 * be for this skb because only one such packet is allowed in the queue.
 */
static void ftgmac030_tx_hwtstamp_work(struct work_struct *work)
{
	struct ftgmac030 *priv = container_of(work, struct ftgmac030,
					      tx_hwtstamp_work);
	u32 tsmp_valid;

	if (!priv->tx_hwtstamp_skb)
		return;

	tsmp_valid = ioread32(priv->base + FTGMAC030_REG_ISR);
	tsmp_valid &= FTGMAC030_INT_TX_TMSP_VALID;
	if (tsmp_valid) {
		struct skb_shared_hwtstamps hwtstamps;
		unsigned long flags;
		u64 txstmp;

		spin_lock_irqsave(&priv->systim_lock, flags);
		txstmp = ioread32(priv->base + FTGMAC030_REG_PTP_TX_SEC) * NSEC_PER_SEC;
		txstmp += ioread32(priv->base + FTGMAC030_REG_PTP_TX_NSEC);
		spin_unlock_irqrestore(&priv->systim_lock, flags);

		memset(&hwtstamps, 0, sizeof(hwtstamps));
		hwtstamps.hwtstamp = ns_to_ktime(txstmp);

		skb_tstamp_tx(priv->tx_hwtstamp_skb, &hwtstamps);
		dev_kfree_skb_any(priv->tx_hwtstamp_skb);
		priv->tx_hwtstamp_skb = NULL;

		iowrite32(tsmp_valid, priv->base + FTGMAC030_REG_ISR);
	} else {
		/* reschedule to check later */
		schedule_work(&priv->tx_hwtstamp_work);
	}
}
#endif

static void ftgmac030_tx_complete(struct ftgmac030 *priv)
{
	struct net_device *netdev = priv->netdev;

	while (ftgmac030_tx_complete_packet(priv)) {
		netif_wake_queue(netdev);
	}
}

static int ftgmac030_xmit(struct ftgmac030 *priv, struct sk_buff *skb,
			  dma_addr_t map)
{
	struct net_device *netdev = priv->netdev;
	struct ftgmac030_txdes *txdes;
	int i;
	unsigned int len = (skb->len < ETH_ZLEN) ? ETH_ZLEN : skb->len;

	txdes = ftgmac030_current_txdes(priv);
	ftgmac030_tx_pointer_advance(priv);

	/* setup TX descriptor */
	ftgmac030_txdes_set_skb(txdes, skb);
	ftgmac030_txdes_set_dma_addr(txdes, map);
	ftgmac030_txdes_set_buffer_size(txdes, len);

	ftgmac030_txdes_set_first_segment(txdes);
	ftgmac030_txdes_set_last_segment(txdes);
	ftgmac030_txdes_set_txint(txdes);
	if (skb->ip_summed == CHECKSUM_PARTIAL) {
		__be16 protocol = skb->protocol;

		if (protocol == cpu_to_be16(ETH_P_IP)) {
			u8 ip_proto = ip_hdr(skb)->protocol;

			ftgmac030_txdes_set_ipcs(txdes);

			if (ip_proto == IPPROTO_TCP)
				ftgmac030_txdes_set_tcpcs(txdes);
			else if (ip_proto == IPPROTO_UDP)
				ftgmac030_txdes_set_udpcs(txdes);
		} else if (protocol == cpu_to_be16(ETH_P_IPV6)) {
			ftgmac030_txdes_set_ipv6(txdes);
			ftgmac030_txdes_set_tcpcs(txdes);
			ftgmac030_txdes_set_udpcs(txdes);
		}
	}

#ifdef CONFIG_FTGMAC030_PTP
	if (unlikely((skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP) &&
		     !priv->tx_hwtstamp_skb)) {
		skb_shinfo(skb)->tx_flags |= SKBTX_IN_PROGRESS;
		priv->tx_hwtstamp_skb = skb_get(skb);
		schedule_work(&priv->tx_hwtstamp_work);
	} else {
		skb_tx_timestamp(skb);
	}
#endif
	spin_lock(&priv->tx_lock);

	/* start transmit */
	ftgmac030_txdes_set_dma_own(txdes);

	priv->tx_pending++;
	if (priv->tx_pending == TX_QUEUE_ENTRIES) {
		netif_stop_queue(netdev);
		if (show_rx_dbg)
			printk("MAC tx queue full, stop\n");
	}

	spin_unlock(&priv->tx_lock);

	if(show_tx_pkg) {
	    printk("<tx pkg=");
	    for(i = 0; i < len; i++)
	        printk("0x%02x ", *(skb->data + i));
        printk(">\n");
    }

	ftgmac030_txdma_normal_prio_start_polling(priv);

	return NETDEV_TX_OK;
}

/******************************************************************************
 * internal functions (buffer)
 *****************************************************************************/
static int ftgmac030_alloc_rx_page(struct ftgmac030 *priv,
				   struct ftgmac030_rxdes *rxdes, gfp_t gfp)
{
	struct net_device *netdev = priv->netdev;
	struct page *page;
	dma_addr_t map;

	page = alloc_page(gfp);
	if (!page) {
		if (net_ratelimit())
			netdev_err(netdev, "failed to allocate rx page\n");
		return -ENOMEM;
	}

	map = dma_map_page(priv->dev, page, 0, RX_BUF_SIZE, DMA_FROM_DEVICE);
	if (unlikely(dma_mapping_error(priv->dev, map))) {
		if (net_ratelimit())
			netdev_err(netdev, "failed to map rx page\n");
		__free_page(page);
		return -ENOMEM;
	}

	ftgmac030_rxdes_set_page(rxdes, page);
	ftgmac030_rxdes_set_dma_addr(rxdes, map);
	ftgmac030_rxdes_set_dma_own(rxdes);
	return 0;
}

static void ftgmac030_free_buffers(struct ftgmac030 *priv)
{
	int i;

	for (i = 0; i < RX_QUEUE_ENTRIES; i++) {
		struct ftgmac030_rxdes *rxdes = &priv->descs->rxdes[i];
		struct page *page = ftgmac030_rxdes_get_page(rxdes);
		dma_addr_t map = ftgmac030_rxdes_get_dma_addr(rxdes);

		if (!page)
			continue;

		dma_unmap_page(priv->dev, map, RX_BUF_SIZE, DMA_FROM_DEVICE);
		__free_page(page);
	}

	for (i = 0; i < TX_QUEUE_ENTRIES; i++) {
		struct ftgmac030_txdes *txdes = &priv->descs->txdes[i];
		struct sk_buff *skb = ftgmac030_txdes_get_skb(txdes);
		dma_addr_t map = ftgmac030_txdes_get_dma_addr(txdes);

		if (!skb)
			continue;

		dma_unmap_single(priv->dev, map, skb_headlen(skb), DMA_TO_DEVICE);
		dev_kfree_skb(skb);
	}

	dma_free_coherent(priv->dev, sizeof(struct ftgmac030_descs),
			  priv->descs, priv->descs_dma_addr);
}

static int ftgmac030_alloc_buffers(struct ftgmac030 *priv)
{
	int i;

	priv->descs = dma_zalloc_coherent(priv->dev,
					  sizeof(struct ftgmac030_descs),
					  &priv->descs_dma_addr, GFP_ATOMIC);
	if (!priv->descs)
		return -ENOMEM;

	netdev_info(priv->netdev, "rx descs %p, tx descs %p\n",
		    &priv->descs->rxdes[0], &priv->descs->txdes[0]);

	/* initialize RX ring */
	ftgmac030_rxdes_set_end_of_ring(&priv->descs->rxdes[RX_QUEUE_ENTRIES - 1]);

	for (i = 0; i < RX_QUEUE_ENTRIES; i++) {
		struct ftgmac030_rxdes *rxdes = &priv->descs->rxdes[i];

		if (ftgmac030_alloc_rx_page(priv, rxdes, GFP_ATOMIC))
			goto err;
	}

	/* initialize TX ring */
	ftgmac030_txdes_set_end_of_ring(&priv->descs->txdes[TX_QUEUE_ENTRIES - 1]);
	return 0;

err:
	ftgmac030_free_buffers(priv);
	return -ENOMEM;
}

/******************************************************************************
 * internal functions (mdio)
 *****************************************************************************/
static void ftgmac030_adjust_link(struct net_device *netdev)
{
	struct ftgmac030 *priv = netdev_priv(netdev);
	struct phy_device *phydev = priv->phydev;
	int ier;

#if 0//ndef CONFIG_PLATFORM_GM8210
	if (phydev->speed == 1000) {//justin
//gisr = (gisr == 2) ? PHY_INTERFACE_MODE_RGMII_RXID : (gisr == 1) ? PHY_INTERFACE_MODE_RMII : PHY_INTERFACE_MODE_GMII;	    
	    if(interface_type())
		    return;
	}
#endif	
	if (phydev->speed == priv->old_speed && phydev->duplex == priv->old_duplex)
		return;

	if (phydev->speed == 0)
		    return;

	printk("phy speed is %d, %s duplex\n", phydev->speed, phydev->duplex ? "full" : "half");
	
	priv->old_speed = phydev->speed;
	priv->old_duplex = phydev->duplex;

	ier = ioread32(priv->base + FTGMAC030_REG_IER);

	/* disable all interrupts */
	iowrite32(0, priv->base + FTGMAC030_REG_IER);

	netif_stop_queue(netdev);
#if 1
	ftgmac030_stop_hw(priv);
#else
	priv->rx_pointer = 0;
	priv->tx_clean_pointer = 0;
	priv->tx_pointer = 0;
	priv->tx_pending = 0;

	ftgmac030_reset_hw(priv);
#endif
	netif_start_queue(netdev);
	ftgmac030_init_hw(priv);
	ftgmac030_start_hw(priv, phydev->speed, phydev->duplex);

	/* re-enable interrupts */
	iowrite32(ier, priv->base + FTGMAC030_REG_IER);
}

static int ftgmac030_mii_probe(struct ftgmac030 *priv)
{
	struct net_device *netdev = priv->netdev;
	struct phy_device *phydev = NULL;
	int i, gisr;

	/* search for connect PHY device */
	for (i = 0; i < PHY_MAX_ADDR; i++) {
		struct phy_device *tmp = priv->mii_bus->phy_map[i];

		if (tmp) {
			phydev = tmp;
			break;
		}
	}

	/* now we are supposed to have a proper phydev, to attach to... */
	if (!phydev) {
		netdev_info(netdev, "%s: no PHY found\n", netdev->name);
		return -ENODEV;
	}

	gisr = ioread32(priv->base + FTGMAC030_REG_GISR);

	/* RGMII interface about Extened PHY Specific Control Register(offset 20): 
	 * if need to set RGMII Receive Timing Control use PHY_INTERFACE_MODE_RGMII_RXID
	 * if need to set RGMII Transmit Timing Control use PHY_INTERFACE_MODE_RGMII_TXID
	 * If need to set both use PHY_INTERFACE_MODE_RGMII_ID
	 * Otherwise use PHY_INTERFACE_MODE_RGMII
	 */
	gisr = (gisr == 2) ? PHY_INTERFACE_MODE_RGMII_RXID : (gisr == 1) ? PHY_INTERFACE_MODE_RMII : PHY_INTERFACE_MODE_GMII;
	phydev = phy_connect(netdev, dev_name(&phydev->dev),
			     &ftgmac030_adjust_link, 0, gisr);

	if (IS_ERR(phydev)) {
		netdev_err(netdev, "%s: Could not attach to PHY\n", netdev->name);
		return PTR_ERR(phydev);
	}

	priv->phydev = phydev;
	return 0;
}

/******************************************************************************
 * struct mii_bus functions
 *****************************************************************************/
static int ftgmac030_mdiobus_read(struct mii_bus *bus, int phy_addr, int regnum)
{
	struct net_device *netdev = bus->priv;
	struct ftgmac030 *priv = netdev_priv(netdev);
	unsigned int phycr;
	int i;

	phycr = FTGMAC030_PHYCR_ST(1) | FTGMAC030_PHYCR_OP(2) |
		FTGMAC030_PHYCR_PHYAD(phy_addr) |
		FTGMAC030_PHYCR_REGAD(regnum) |
		FTGMAC030_PHYCR_MIIRD | mdc_cycthr;

	iowrite32(phycr, priv->base + FTGMAC030_REG_PHYCR);

	for (i = 0; i < 100; i++) {
		phycr = ioread32(priv->base + FTGMAC030_REG_PHYCR);

		if ((phycr & FTGMAC030_PHYCR_MIIRD) == 0) {
			int data;

			data = ioread32(priv->base + FTGMAC030_REG_PHYDATA);
			data = FTGMAC030_PHYDATA_MIIRDATA(data);
			
			if(show_phy)
			    printk("<r:adr=%d,reg=%d,data=0x%x>", phy_addr, regnum, data);
			    
			return data;
		}

		wait_status(1);
	}

	netdev_err(netdev, "mdio read timed out\n");
	return -EIO;
}

static int ftgmac030_mdiobus_write(struct mii_bus *bus, int phy_addr,
				   int regnum, u16 value)
{
	struct net_device *netdev = bus->priv;
	struct ftgmac030 *priv = netdev_priv(netdev);
	unsigned int phycr;
	int data;
	int i;

    if(show_phy)
        printk("<w:adr=%d,reg=%d,data=0x%x>", phy_addr, regnum, value);

	phycr = FTGMAC030_PHYCR_ST(1) | FTGMAC030_PHYCR_OP(1) |
		FTGMAC030_PHYCR_PHYAD(phy_addr) |
		FTGMAC030_PHYCR_REGAD(regnum) |
		FTGMAC030_PHYCR_MIIWR | mdc_cycthr;

	data = FTGMAC030_PHYDATA_MIIWDATA(value);

	iowrite32(data, priv->base + FTGMAC030_REG_PHYDATA);
	iowrite32(phycr, priv->base + FTGMAC030_REG_PHYCR);

	for (i = 0; i < 100; i++) {
		phycr = ioread32(priv->base + FTGMAC030_REG_PHYCR);

		if ((phycr & FTGMAC030_PHYCR_MIIWR) == 0)
			return 0;

		wait_status(1);
	}

	netdev_err(netdev, "mdio write timed out\n");
	return -EIO;
}

static int ftgmac030_mdiobus_reset(struct mii_bus *bus)
{
	return 0;
}

/******************************************************************************
 * struct ethtool_ops functions
 *****************************************************************************/
static void ftgmac030_get_drvinfo(struct net_device *netdev,
				  struct ethtool_drvinfo *info)
{
	strlcpy(info->driver, DRV_NAME, sizeof(info->driver));
	strlcpy(info->version, DRV_VERSION, sizeof(info->version));
	strlcpy(info->bus_info, dev_name(&netdev->dev), sizeof(info->bus_info));
}

static int ftgmac030_get_settings(struct net_device *netdev,
				  struct ethtool_cmd *cmd)
{
	struct ftgmac030 *priv = netdev_priv(netdev);

	return phy_ethtool_gset(priv->phydev, cmd);
}

static int ftgmac030_set_settings(struct net_device *netdev,
				  struct ethtool_cmd *cmd)
{
	struct ftgmac030 *priv = netdev_priv(netdev);

	return phy_ethtool_sset(priv->phydev, cmd);
}

static int ftgmac030_get_regs_len(struct net_device *netdev)
{
#define FTGMAC030_REGS_LEN 0x52
	return FTGMAC030_REGS_LEN * sizeof(u32);
}

static void ftgmac030_get_regs(struct net_device *netdev,
			       struct ethtool_regs *regs, void *p)
{
	struct ftgmac030 *priv = netdev_priv(netdev);
	u32 *regs_buff = p;
	int i;

	regs->version = 0x470030;

	for (i=0; i < FTGMAC030_REGS_LEN; i++)
		regs_buff[i] = ioread32(priv->base + (i * 0x4));
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0)
static int e1000e_get_ts_info(struct net_device *netdev,
			      struct ethtool_ts_info *info)
{
	struct ftgmac030 *priv = netdev_priv(netdev);
	u32 maccr;

	info->so_timestamping =
		SOF_TIMESTAMPING_TX_SOFTWARE |
		SOF_TIMESTAMPING_RX_SOFTWARE |
		SOF_TIMESTAMPING_SOFTWARE;
	info->phc_index = -1;

	maccr = ioread32(priv->base + FTGMAC030_REG_MACCR);
	if (!(maccr & FTGMAC030_MACCR_PTP_EN))
		return 0;

	info->so_timestamping |= (SOF_TIMESTAMPING_TX_HARDWARE |
				  SOF_TIMESTAMPING_RX_HARDWARE |
				  SOF_TIMESTAMPING_RAW_HARDWARE);

	info->tx_types = (1 << HWTSTAMP_TX_OFF) | (1 << HWTSTAMP_TX_ON);

	info->rx_filters = ((1 << HWTSTAMP_FILTER_NONE) |
			    (1 << HWTSTAMP_FILTER_PTP_V1_L4_SYNC) |
			    (1 << HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ) |
			    (1 << HWTSTAMP_FILTER_PTP_V2_L4_SYNC) |
			    (1 << HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ) |
			    (1 << HWTSTAMP_FILTER_PTP_V2_L2_SYNC) |
			    (1 << HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ) |
			    (1 << HWTSTAMP_FILTER_PTP_V2_EVENT) |
			    (1 << HWTSTAMP_FILTER_PTP_V2_SYNC) |
			    (1 << HWTSTAMP_FILTER_PTP_V2_DELAY_REQ) |
			    (1 << HWTSTAMP_FILTER_ALL));

	if (priv->ptp_clock)
		info->phc_index = ptp_clock_index(priv->ptp_clock);

	return 0;
}
#endif

static void ftgmac030_get_ethtool_stats(struct net_device *netdev,
		struct ethtool_stats *stats, u64 *data)
{
	int i = 0;

    for (i = 0; i < 8; i++)
        data[i] = ((unsigned long *)&netdev->stats)[i];

    data[i++] = netdev->stats.rx_length_errors;
    data[i++] = netdev->stats.rx_crc_errors;
    data[i++] = netdev->stats.rx_frame_errors;
    data[i++] = netdev->stats.rx_fifo_errors;//justin 0xC8
}

static const struct {
	const char name[ETH_GSTRING_LEN];
} sundance_stats[] = {
	{ "rx_packets" },
	{ "tx_packets" },
	{ "rx_bytes" },
	{ "tx_bytes" },
	{ "rx_errors" },
	{ "tx_errors" },
	{ "rx_dropped" },
	{ "tx_dropped" },
	
	{ "rx_length_errors" },
	{ "rx_crc_errors" },
	{ "rx_frame_errors" },
	{ "rx_fifo_errors" },	
};

static void get_strings(struct net_device *dev, u32 stringset,
		u8 *data)
{
	if (stringset == ETH_SS_STATS)
		memcpy(data, sundance_stats, sizeof(sundance_stats));
}

static int get_sset_count(struct net_device *dev, int sset)
{
	switch (sset) {
	case ETH_SS_STATS:
		return ARRAY_SIZE(sundance_stats);
	default:
		return -EOPNOTSUPP;
	}
}

static const struct ethtool_ops ftgmac030_ethtool_ops = {
	.set_settings		= ftgmac030_set_settings,
	.get_settings		= ftgmac030_get_settings,
	.get_drvinfo		= ftgmac030_get_drvinfo,
	.get_link		= ethtool_op_get_link,
	.get_regs_len		= ftgmac030_get_regs_len,
	.get_regs		= ftgmac030_get_regs,
	.get_ethtool_stats = ftgmac030_get_ethtool_stats,
	.get_strings = get_strings,
	.get_sset_count = get_sset_count,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0)
	.get_ts_info		= ftgmac030_get_ts_info,
#endif
};

/******************************************************************************
 * interrupt handler
 *****************************************************************************/
static irqreturn_t ftgmac030_interrupt(int irq, void *dev_id)
{
	struct net_device *netdev = dev_id;
	struct ftgmac030 *priv = netdev_priv(netdev);

	/* Disable interrupts for polling */
	iowrite32(0, priv->base + FTGMAC030_REG_IER);

	if (likely(netif_running(netdev))) {
		napi_schedule(&priv->napi);
	}

	return IRQ_HANDLED;
}

/******************************************************************************
 * struct napi_struct functions
 *****************************************************************************/
static int ftgmac030_poll(struct napi_struct *napi, int budget)
{
	struct ftgmac030 *priv = container_of(napi, struct ftgmac030, napi);
	struct net_device *netdev = priv->netdev;
	unsigned int status;
	bool completed = true;
	int rx = 0;

	status = ioread32(priv->base + FTGMAC030_REG_ISR);
	iowrite32(status, priv->base + FTGMAC030_REG_ISR);

	if (status & (FTGMAC030_INT_RPKT_BUF | FTGMAC030_INT_NO_RXBUF)) {
		/*
		 * FTGMAC030_INT_RPKT_BUF:
		 *	RX DMA has received packets into RX buffer successfully
		 *
		 * FTGMAC030_INT_NO_RXBUF:
		 *	RX buffer unavailable
		 */
		bool retry;

		do {
			retry = ftgmac030_rx_packet(priv, &rx);
		} while (retry && rx < budget);

		if (retry && rx == budget)
			completed = false;
	}

	if (status & (FTGMAC030_INT_XPKT_ETH | FTGMAC030_INT_XPKT_LOST)) {
		/*
		 * FTGMAC030_INT_XPKT_ETH:
		 *	packet transmitted to ethernet successfully
		 *
		 * FTGMAC030_INT_XPKT_LOST:
		 *	packet transmitted to ethernet lost due to late
		 *	collision or excessive collision
		 */
		ftgmac030_tx_complete(priv);
	}

	if (status & FTGMAC030_INT_AHB_ERR)
    	netdev_info(netdev, "AHB bus err\n");

	if (status & FTGMAC030_INT_NO_RXBUF) {
		/* RX buffer unavailable */
		netdev->stats.rx_over_errors++;
	}

	if (status & FTGMAC030_INT_RPKT_LOST) {
		/* received packet lost due to RX FIFO full */
		netdev->stats.rx_fifo_errors++;
    	netdev->stats.rx_errors++;
    	if (show_rx_dbg)
    		netdev_info(netdev, "MAC INT rx fifo full, lose\n");
	}

	if (completed) {
		napi_complete(napi);

		/* enable all interrupts */
		iowrite32(INT_MASK_ALL_ENABLED, priv->base + FTGMAC030_REG_IER);
	}

	return rx;
}

/******************************************************************************
 * struct net_device_ops functions
 *****************************************************************************/
static int ftgmac030_open(struct net_device *netdev)
{
	struct ftgmac030 *priv = netdev_priv(netdev);
	int err;

	err = ftgmac030_alloc_buffers(priv);
	if (err) {
		netdev_err(netdev, "failed to allocate buffers\n");
		goto err_alloc;
	}

	err = request_irq(priv->irq, ftgmac030_interrupt, 0, netdev->name, netdev);
	if (err) {
		netdev_err(netdev, "failed to request irq %d\n", priv->irq);
		goto err_irq;
	}

	priv->rx_pointer = 0;
	priv->tx_clean_pointer = 0;
	priv->tx_pointer = 0;
	priv->tx_pending = 0;

	err = ftgmac030_reset_hw(priv);
	if (err)
		goto err_hw;

	ftgmac030_init_hw(priv);
	ftgmac030_start_hw(priv, priv->old_speed, priv->old_duplex);

	phy_start(priv->phydev);

	napi_enable(&priv->napi);
	netif_start_queue(netdev);

	/* enable all interrupts */
	iowrite32(INT_MASK_ALL_ENABLED, priv->base + FTGMAC030_REG_IER);
	return 0;

err_hw:
	free_irq(priv->irq, netdev);
err_irq:
	ftgmac030_free_buffers(priv);
err_alloc:
	return err;
}

static int ftgmac030_stop(struct net_device *netdev)
{
	struct ftgmac030 *priv = netdev_priv(netdev);

	/* disable all interrupts */
	iowrite32(0, priv->base + FTGMAC030_REG_IER);

	netif_stop_queue(netdev);
	napi_disable(&priv->napi);
	phy_stop(priv->phydev);

	ftgmac030_stop_hw(priv);
	free_irq(priv->irq, netdev);
	ftgmac030_free_buffers(priv);

	return 0;
}

static int ftgmac030_hard_start_xmit(struct sk_buff *skb,
				     struct net_device *netdev)
{
	struct ftgmac030 *priv = netdev_priv(netdev);
	dma_addr_t map;

	if (unlikely(skb->len > MAX_PKT_SIZE)) {
		if (net_ratelimit())
			netdev_dbg(netdev, "tx packet too big\n");

		netdev->stats.tx_dropped++;
		dev_kfree_skb(skb);
		return NETDEV_TX_OK;
	}

	map = dma_map_single(priv->dev, skb->data, skb_headlen(skb), DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(priv->dev, map))) {
		/* drop packet */
		if (net_ratelimit())
			netdev_err(netdev, "map socket buffer failed\n");

		netdev->stats.tx_dropped++;
		dev_kfree_skb(skb);
		return NETDEV_TX_OK;
	}
	return ftgmac030_xmit(priv, skb, map);
}

#ifdef CONFIG_FTGMAC030_PTP
/*
 *
 * ftgmac030_config_hwtstamp - configure the hwtstamp registers and enable/disable
 * @adapter: board private structure
 * Outgoing time stamping can be enabled and disabled. Play nice and
 * disable it when requested, although it shouldn't cause any overhead
 * when no packet needs it. At most one packet in the queue may be
 * marked for time stamping, otherwise it would be impossible to tell
 * for sure to which packet the hardware time stamp belongs.
 *
 * Incoming time stamping has to be configured via the hardware filters.
 * Not all combinations are supported, in particular event type has to be
 * specified. Matching the kind of event packet is not supported, with the
 * exception of "all V2 events regardless of level 2 or 4".
 **/
static int ftgmac030_config_hwtstamp(struct ftgmac030 *adapter)
{
	struct hwtstamp_config *config = &adapter->hwtstamp_config;
	struct timespec now;
	u32 tsync_tx_ctl = 1;
	u32 tsync_rx_ctl = 1;
	u32 maccr;

	switch (config->tx_type) {
	case HWTSTAMP_TX_OFF:
		tsync_tx_ctl = 0;
		break;
	case HWTSTAMP_TX_ON:
		break;
	default:
		return -ERANGE;
	}

	switch (config->rx_filter) {
	case HWTSTAMP_FILTER_NONE:
		tsync_rx_ctl = 0;
		break;
	case HWTSTAMP_FILTER_PTP_V1_L4_EVENT:
	case HWTSTAMP_FILTER_PTP_V1_L4_SYNC:
	case HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ:
	case HWTSTAMP_FILTER_PTP_V2_L2_EVENT:
	case HWTSTAMP_FILTER_PTP_V2_L2_SYNC:
	case HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ:
	case HWTSTAMP_FILTER_PTP_V2_L4_EVENT:
	case HWTSTAMP_FILTER_PTP_V2_L4_SYNC:
	case HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ:
	case HWTSTAMP_FILTER_PTP_V2_EVENT:
	case HWTSTAMP_FILTER_PTP_V2_SYNC:
	case HWTSTAMP_FILTER_PTP_V2_DELAY_REQ:
	case HWTSTAMP_FILTER_ALL:
		break;
	default:
		return -ERANGE;
	}

	maccr = ioread32(adapter->base + FTGMAC030_REG_MACCR);

	/* enable/disable Tx/Rx h/w time stamping */
	if (tsync_tx_ctl ||  tsync_rx_ctl) {
		maccr |= FTGMAC030_MACCR_PTP_EN;

		/* reset the ns time counter */
		getnstimeofday(&now);
		iowrite32(now.tv_sec, adapter->base + FTGMAC030_REG_PTP_TM_SEC);
		iowrite32(now.tv_nsec, adapter->base + FTGMAC030_REG_PTP_TM_NSEC);
	} else {
		maccr &= ~FTGMAC030_MACCR_PTP_EN;
 	}

	iowrite32(maccr, adapter->base + FTGMAC030_REG_MACCR);

	return 0;
}

static int ftgmac030_hwtstamp_set(struct net_device *netdev, struct ifreq *ifr)
{
	struct ftgmac030 *priv = netdev_priv(netdev);
	struct hwtstamp_config config;
	int ret_val;

	if (copy_from_user(&config, ifr->ifr_data, sizeof(config)))
		return -EFAULT;

	priv->hwtstamp_config = config;

	ret_val = ftgmac030_config_hwtstamp(priv);
	if (ret_val)
		return ret_val;

	config = priv->hwtstamp_config;

	switch (config.rx_filter) {
	case HWTSTAMP_FILTER_PTP_V2_L4_SYNC:
	case HWTSTAMP_FILTER_PTP_V2_L2_SYNC:
	case HWTSTAMP_FILTER_PTP_V2_SYNC:
	case HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ:
	case HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ:
	case HWTSTAMP_FILTER_PTP_V2_DELAY_REQ:
		/* With V2 type filters which specify a Sync or Delay Request,
		 * Path Delay Request/Response messages are also time stamped
		 * by hardware so notify the caller the requested packets plus
		 * some others are time stamped.
		 */
		config.rx_filter = HWTSTAMP_FILTER_SOME;
		break;
	default:
		break;
	}

	return copy_to_user(ifr->ifr_data, &config,
			    sizeof(config)) ? -EFAULT : 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0)
static int ftgmac030_hwtstamp_get(struct net_device *netdev, struct ifreq *ifr)
{
	struct ftgmac030 *priv = netdev_priv(netdev);

	return copy_to_user(ifr->ifr_data, &priv->hwtstamp_config,
			    sizeof(priv->hwtstamp_config)) ? -EFAULT : 0;
}
#endif
#endif	//CONFIG_FTGMAC030_PTP

static unsigned int ftgmac030_get_ht_index(unsigned char *mac_addr)
{
	unsigned int crc32 = ether_crc(ETH_ALEN, mac_addr);
	crc32 = ~crc32;
	crc32 = bitrev8(crc32 & 0xff) |
			(bitrev8((crc32 >> 8) & 0xff) << 8) |
			(bitrev8((crc32 >> 16) & 0xff) << 16) |
			(bitrev8((crc32 >> 24) & 0xff) << 24);

	/* return MSB 6 bits */
	return ((unsigned char)(crc32 >> 26));
}

static void ftgmac030_set_rx_mode(struct net_device *netdev)
{
	struct ftgmac030 *priv = netdev_priv(netdev);
	unsigned int maccr = ioread32(priv->base + FTGMAC030_REG_MACCR);

	if (show_rx_dbg)
		printk("set rx mode = 0x%x\n", netdev->flags);

	/* clear filter flags */
	maccr &= ~(FTGMAC030_MACCR_RX_BROADPKT | FTGMAC030_MACCR_RX_MULTIPKT | FTGMAC030_MACCR_HT_MULTI_EN | FTGMAC030_MACCR_RX_ALL);
	if (netdev->flags & IFF_PROMISC) {
		maccr |= FTGMAC030_MACCR_RX_ALL;
	}

	if (netdev->flags & IFF_ALLMULTI) {
		maccr |= FTGMAC030_MACCR_RX_MULTIPKT;
	}

	if (netdev->flags & IFF_BROADCAST) {
		maccr |= FTGMAC030_MACCR_RX_BROADPKT;
	}

	if (netdev->flags & IFF_MULTICAST) {
		unsigned int maht0 = 0x0;
		unsigned int maht1 = 0x0;
		struct netdev_hw_addr *ha;

		maccr |= FTGMAC030_MACCR_HT_MULTI_EN;
		/* clear hash table*/
		iowrite32(maht0, priv->base + FTGMAC030_REG_MAHT0);
		iowrite32(maht1, priv->base + FTGMAC030_REG_MAHT1);
		/* set hash table */
		netdev_for_each_mc_addr(ha, netdev) {
			unsigned char ht_index = ftgmac030_get_ht_index(ha->addr);
			if (ht_index < 32)
				maht0 |= (1 << ht_index);
			else
				maht1 |= (1 << (ht_index - 32));
		}
		iowrite32(maht0, priv->base + FTGMAC030_REG_MAHT0);
		iowrite32(maht1, priv->base + FTGMAC030_REG_MAHT1);
	}

	iowrite32(maccr, priv->base + FTGMAC030_REG_MACCR);
}

/* optional */
static int ftgmac030_do_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd)
{
	struct ftgmac030 *priv = netdev_priv(netdev);

	switch (cmd) {
#ifdef CONFIG_FTGMAC030_PTP
	case SIOCSHWTSTAMP:
		return ftgmac030_hwtstamp_set(netdev, ifr);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0)
	case SIOCGHWTSTAMP:
		return ftgmac030_hwtstamp_get(netdev, ifr);
#endif
#endif
	default:
		return phy_mii_ioctl(priv->phydev, ifr, cmd);
	}
}

static const struct net_device_ops ftgmac030_netdev_ops = {
	.ndo_open		= ftgmac030_open,
	.ndo_stop		= ftgmac030_stop,
	.ndo_start_xmit		= ftgmac030_hard_start_xmit,
	.ndo_set_mac_address	= eth_mac_addr,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_do_ioctl		= ftgmac030_do_ioctl,
	.ndo_set_rx_mode	= ftgmac030_set_rx_mode,
};

/******************************************************************************
 * struct platform_driver functions
 *****************************************************************************/
static int ftgmac030_probe(struct platform_device *pdev)
{
	struct resource *res;
	int irq;
	struct net_device *netdev;
	struct ftgmac030 *priv;
	char *ptr = NULL, *ptr_end;
    u8 addr[6];
	char ethaddr[18];
	int err;
	int i;

	if (!pdev)
		return -ENODEV;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENXIO;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	/* setup net_device */
	netdev = alloc_etherdev(sizeof(*priv));
	if (!netdev) {
		err = -ENOMEM;
		goto err_alloc_etherdev;
	}

	SET_NETDEV_DEV(netdev, &pdev->dev);

	SET_ETHTOOL_OPS(netdev, &ftgmac030_ethtool_ops);
	netdev->netdev_ops = &ftgmac030_netdev_ops;
	netdev->features = (NETIF_F_IP_CSUM | 
			    NETIF_F_IPV6_CSUM |
			    /*NETIF_F_RXCSUM |*/
			    NETIF_F_GRO);

	platform_set_drvdata(pdev, netdev);

	/* setup private data */
	priv = netdev_priv(netdev);
	priv->netdev = netdev;
	priv->dev = &pdev->dev;

	spin_lock_init(&priv->tx_lock);

	/* initialize NAPI */
	netif_napi_add(netdev, &priv->napi, ftgmac030_poll, 64);

	/* map io memory */
	priv->res = request_mem_region(res->start, resource_size(res),
				       dev_name(&pdev->dev));
	if (!priv->res) {
		dev_err(&pdev->dev, "Could not reserve memory region\n");
		err = -ENOMEM;
		goto err_req_mem;
	}

	priv->base = ioremap(res->start, resource_size(res));
	if (!priv->base) {
		dev_err(&pdev->dev, "Failed to ioremap ethernet registers\n");
		err = -EIO;
		goto err_ioremap;
	}

	priv->irq = irq;
	if(mac_scu_init(priv->irq) < 0){
		err = -EIO;
		goto err_alloc_mdiobus;
	}
	
	/* initialize mdio bus */
	set_mac_clock(priv->irq);

#ifdef CONFIG_FTGMAC030_PTP
	spin_lock_init(&priv->systim_lock);
	INIT_WORK(&priv->tx_hwtstamp_work, ftgmac030_tx_hwtstamp_work);
#endif

	i = ioread32(priv->base + FTGMAC030_REG_GISR) & 0x3;
	mdc_cycthr = (i > 0) ? 0x68 : 0x34;

	/* initialize mdio bus */
	priv->mii_bus = mdiobus_alloc();
	if (!priv->mii_bus) {
		err = -EIO;
		goto err_alloc_mdiobus;
	}

	if(irq == MAC_FTGMAC030_0_IRQ){
		priv->mii_bus->name = "ftgmac030-0-mdio";
		snprintf(priv->mii_bus->id, MII_BUS_ID_SIZE, DRV_NAME);
	}else{
		priv->mii_bus->name = "ftgmac030-1-mdio";
		snprintf(priv->mii_bus->id, MII_BUS_ID_SIZE, DRV_1_NAME);	
	}

	priv->mii_bus->priv = netdev;
	priv->mii_bus->read = ftgmac030_mdiobus_read;
	priv->mii_bus->write = ftgmac030_mdiobus_write;
	priv->mii_bus->reset = ftgmac030_mdiobus_reset;
	priv->mii_bus->irq = priv->phy_irq;

	for (i = 0; i < PHY_MAX_ADDR; i++)
		priv->mii_bus->irq[i] = PHY_POLL;

	err = mdiobus_register(priv->mii_bus);
	if (err) {
		dev_err(&pdev->dev, "Cannot register MDIO bus!\n");
		goto err_register_mdiobus;
	}

	err = ftgmac030_mii_probe(priv);
	if (err) {
		dev_err(&pdev->dev, "MII Probe failed!\n");
		goto err_mii_probe;
	}

	/* register network device */
	err = register_netdev(netdev);
	if (err) {
		dev_err(&pdev->dev, "Failed to register netdev\n");
		goto err_register_netdev;
	}

	netdev_info(netdev, "irq %d, mapped at %p\n", priv->irq, priv->base);

#ifdef CONFIG_FTGMAC030_PTP
	/* This is value at the time FPGA verification.
	 * Change based on SOC real period.
	 * Inside ftgmac030_ptp_init function, FTGMAC030_REG_PTP_NS_PERIOD 
	 * will be read, make sure it has valid value. 
	 */
	iowrite32(20, priv->base + FTGMAC030_REG_PTP_NS_PERIOD);

	/* init PTP hardware clock */
	ftgmac030_ptp_init(priv);
#endif

	if (!is_valid_ether_addr(netdev->dev_addr)) {
        ethaddr[17] = '\0';	    
	    	if(irq == MAC_FTGMAC030_0_IRQ){	        
            ptr = strstr(command_line, "ethaddr=");
            if (ptr)
                memcpy(ethaddr, ptr + 8, 17 * sizeof(char));
        }
        else{
            ptr = strstr(command_line, "eth1addr=");
            if (ptr)
                memcpy(ethaddr, ptr + 9, 17 * sizeof(char));
        }
    
        if (ptr) {
            printk(KERN_NOTICE "ethaddr parsed from commandline: %s\n", ethaddr);
            ptr_end = ethaddr;
            
            for (i = 0; i <= 5; i++) {
                addr[i] = simple_strtol(ptr_end, &ptr_end, 16) |
                          simple_strtol(ptr_end, &ptr_end, 16) << 4;
                          ptr_end++; /* skip ":" in  ethaddr */
                          
            }
            memcpy(netdev->dev_addr, addr, sizeof(addr));
            
        } else {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,3,0)
		    random_ether_addr(netdev->dev_addr);
#else
		    eth_hw_addr_random(netdev);
#endif
    		netdev_info(netdev, "generated random MAC address %pM\n",
    			    netdev->dev_addr);
		}
	}

	ftgmac_reg_debug->data = netdev;
	return 0;

err_register_netdev:
	phy_disconnect(priv->phydev);
err_mii_probe:
	mdiobus_unregister(priv->mii_bus);
err_register_mdiobus:
	mdiobus_free(priv->mii_bus);
err_alloc_mdiobus:
	iounmap(priv->base);
err_ioremap:
	release_resource(priv->res);
err_req_mem:
	netif_napi_del(&priv->napi);
	free_netdev(netdev);
err_alloc_etherdev:
	return err;
}

static int __exit ftgmac030_remove(struct platform_device *pdev)
{
	struct net_device *netdev;
	struct ftgmac030 *priv;

	netdev = platform_get_drvdata(pdev);
	priv = netdev_priv(netdev);

	unregister_netdev(netdev);

	phy_disconnect(priv->phydev);
	mdiobus_unregister(priv->mii_bus);
	mdiobus_free(priv->mii_bus);

	mac_scu_close(priv->irq);

	iounmap(priv->base);
	release_resource(priv->res);

	netif_napi_del(&priv->napi);
	free_netdev(netdev);
	return 0;
}


#ifdef CONFIG_FTGMAC030_DRIVER_0
static struct platform_driver ftgmac030_driver = {
	.probe		= ftgmac030_probe,
	.remove		= __exit_p(ftgmac030_remove),
	.driver		= {
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
	},
};
#endif

#ifdef CONFIG_FTGMAC030_DRIVER_1
static struct platform_driver ftgmac030_1_driver = {
	.probe		= ftgmac030_probe,
	.remove		= __exit_p(ftgmac030_remove),
	.driver		= {
		.name	= DRV_1_NAME,
		.owner	= THIS_MODULE,
	},
};
#endif

static int ftgmac_proc_tx_write(struct file *file, const char *buffer, unsigned long count,
                                void *data)
{
    show_tx_pkg = *buffer - 48;
    //printk("buf=%d,%d\n", *buffer, show_tx_pkg);
    return count;
}

static int ftgmac_proc_rx_write(struct file *file, const char *buffer, unsigned long count,
                                void *data)
{
    show_rx_pkg = *buffer - 48;
    return count;
}

static int ftgmac_proc_phy_write(struct file *file, const char *buffer, unsigned long count,
                                void *data)
{
    show_phy = *buffer - 48;
    return count;
}

static int ftgmac_proc_rx_dbg_write(struct file *file, const char *buffer, unsigned long count,
                                void *data)
{
    show_rx_dbg = *buffer - 48;
    //printk("buf=%d,%d\n", *buffer, show_rx_dbg);
    return count;
}

static int ftgmac_proc_reg_dbg_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    struct net_device *netdev = data;
    struct ftgmac030 *priv = netdev_priv(netdev);
    unsigned int i, len = 0;

	len += sprintf(page + len, "GMAC version %s, queue number tx = %d, rx = %d\n", DRV_VERSION, TX_QUEUE_ENTRIES, RX_QUEUE_ENTRIES);
	for(i = 0; i < 15; i++) {
    	len += sprintf(page + len, "reg%2x: %8x %8x %8x %8x\n", i * 16, ioread32(priv->base + i * 16 + 0), 
    			ioread32(priv->base + i * 16 + 4), ioread32(priv->base + i * 16 + 8), ioread32(priv->base + i * 16 + 12));
	}
    return len;
}

int register_mac0 = 0, register_mac1 = 0;
/******************************************************************************
 * initialization / finalization
 *****************************************************************************/
static int __init ftgmac030_init(void)
{
    int result = -ENODEV;
  
    command_line = kmalloc(COMMAND_LINE_SIZE, GFP_ATOMIC);
    memcpy(command_line, boot_command_line, COMMAND_LINE_SIZE);
     
	pr_info("Loading version " DRV_VERSION " ...\n");
	pr_info("Tx queue number = %d, Rx queue number = %d\n", TX_QUEUE_ENTRIES, RX_QUEUE_ENTRIES);

    ftgmac_proc = create_proc_entry("ftgmac", S_IFDIR | S_IRUGO | S_IXUGO, NULL);
    if (ftgmac_proc == NULL) {
        printk(KERN_ERR "Error to create driver ftgmac proc\n");
        goto exit_init;
    }
    
    ftgmac_tx_pkg_debug = create_proc_entry("show_tx", S_IRUGO, ftgmac_proc);
    if (ftgmac_tx_pkg_debug == NULL)
        panic("Fail to create proc ftgmac tx pkg!\n");

    ftgmac_tx_pkg_debug->write_proc = ftgmac_proc_tx_write;

    ftgmac_rx_pkg_debug = create_proc_entry("show_rx", S_IRUGO, ftgmac_proc);
    if (ftgmac_rx_pkg_debug == NULL)
        panic("Fail to create proc ftgmac rx pkg!\n");

    ftgmac_rx_pkg_debug->write_proc = ftgmac_proc_rx_write;

    ftgmac_phy_debug = create_proc_entry("show_phy", S_IRUGO, ftgmac_proc);
    if (ftgmac_phy_debug == NULL)
        panic("Fail to create proc ftgmac phy!\n");

    ftgmac_phy_debug->write_proc = ftgmac_proc_phy_write;

    ftgmac_rx_debug = create_proc_entry("show_rx_dbg", S_IRUGO, ftgmac_proc);
    if (ftgmac_rx_debug == NULL)
        panic("Fail to create proc ftgmac rx dbg!\n");

    ftgmac_rx_debug->write_proc = ftgmac_proc_rx_dbg_write;

    ftgmac_reg_debug = create_proc_entry("reg_dbg", S_IRUGO, ftgmac_proc);
    if (ftgmac_reg_debug == NULL)
        panic("Fail to create proc ftgmac reg dbg!\n");

    ftgmac_reg_debug->read_proc = ftgmac_proc_reg_dbg_read;

#ifdef CONFIG_FTGMAC030_DRIVER_0
    platform_device_register(&ftgmac030_0_device);
    
    result = platform_driver_register(&ftgmac030_driver);
    if (result < 0) {
        platform_device_unregister(&ftgmac030_0_device);
        goto exit_init;
    }
#endif
#ifdef CONFIG_FTGMAC030_DRIVER_1
    platform_device_register(&ftgmac030_1_device);
    
    result = platform_driver_register(&ftgmac030_1_driver);
    if (result < 0) {
        platform_device_unregister(&ftgmac030_1_device);
        goto exit_init;
    }
#endif

exit_init:		    	
	return result;
}

static void __exit ftgmac030_exit(void)
{
    if (ftgmac_tx_pkg_debug != NULL)
        remove_proc_entry(ftgmac_tx_pkg_debug->name, ftgmac_proc);

    if (ftgmac_rx_pkg_debug != NULL)
        remove_proc_entry(ftgmac_rx_pkg_debug->name, ftgmac_proc);

    if (ftgmac_phy_debug != NULL)
        remove_proc_entry(ftgmac_phy_debug->name, ftgmac_proc);

    if (ftgmac_rx_debug != NULL)
        remove_proc_entry(ftgmac_rx_debug->name, ftgmac_proc);

    if (ftgmac_reg_debug != NULL)
        remove_proc_entry(ftgmac_reg_debug->name, ftgmac_proc); 
                
    if (ftgmac_proc != NULL)
        remove_proc_entry(ftgmac_proc->name, NULL);

#ifdef CONFIG_FTGMAC030_DRIVER_0
    platform_driver_unregister(&ftgmac030_driver);
    platform_device_unregister(&ftgmac030_0_device);
#endif
#ifdef CONFIG_FTGMAC030_DRIVER_1
    platform_driver_unregister(&ftgmac030_1_driver);
    platform_device_unregister(&ftgmac030_1_device);
#endif    
}

module_init(ftgmac030_init);
module_exit(ftgmac030_exit);

MODULE_AUTHOR("Bing-Yao Luo <bjluo@faraday-tech.com>");
MODULE_DESCRIPTION("ftgmac030 driver");
MODULE_LICENSE("GPL");
