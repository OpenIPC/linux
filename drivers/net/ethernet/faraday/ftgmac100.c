/*
 * Faraday FTGMAC100 Gigabit Ethernet
 *
 * (C) Copyright 2009-2011 Faraday Technology
 * Po-Yu Chuang <ratbert@faraday-tech.com>
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
#include <linux/vmalloc.h>
#include <net/ip.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <asm/setup.h>
#include <linux/proc_fs.h>
#include <mach/gm_jiffies.h>
#include "ftgmac100.h"

static int first_time = 0, MDIO_first_time = 1;
static char show_tx_pkg = 0, show_rx_pkg = 0, show_phy = 0, show_rx_dbg = 0;
static struct proc_dir_entry *ftgmac_proc = NULL, *ftgmac_tx_pkg_debug = NULL;
static struct proc_dir_entry *ftgmac_rx_pkg_debug = NULL, *ftgmac_phy_debug = NULL, *ftgmac_rx_debug = NULL, *ftgmac_reg_debug = NULL;
static char *command_line = NULL;

/* ****************************************************************************
 * array contains all platform devices
 * ****************************************************************************/ 

static u64 ftgmac100_dmamask = DMA_BIT_MASK(32);
#if defined(CONFIG_FTGMAC100_DRIVER_0_MASTER) || defined(CONFIG_FTGMAC100_DRIVER_0_SLAVE)
/*GMAC 0*/
static struct resource ftgmac100_0_resource[] = {
	{
         .start  = MAC_FTGMAC100_0_PA_BASE,
         .end        = MAC_FTGMAC100_0_PA_LIMIT,
         .flags  = IORESOURCE_MEM,
	},
	{
         .start  = MAC_FTGMAC100_0_IRQ,
         .end        = MAC_FTGMAC100_0_IRQ,
         .flags  = IORESOURCE_IRQ,
	},
};

static void ftgmac100_0_release(struct device *dev)
{
    return;
}

static struct platform_device ftgmac100_0_device = {
	.name	= DRV_NAME,
	.id		= 0,
	.num_resources	= ARRAY_SIZE(ftgmac100_0_resource),
	.resource	= ftgmac100_0_resource,
        .dev = {
        	.dma_mask = &ftgmac100_dmamask,
        	.coherent_dma_mask = DMA_BIT_MASK(32),
        	.release = ftgmac100_0_release,
        }, 	
};
#endif

#if defined(CONFIG_FTGMAC100_DRIVER_1_MASTER) || defined(CONFIG_FTGMAC100_DRIVER_1_SLAVE)
/*GMAC 1*/
static struct resource ftgmac100_1_resource[] = {
	{
         .start  = MAC_FTGMAC100_1_PA_BASE,
         .end        = MAC_FTGMAC100_1_PA_LIMIT,
         .flags  = IORESOURCE_MEM,
	},
	{
         .start  = MAC_FTGMAC100_1_IRQ,
         .end        = MAC_FTGMAC100_1_IRQ,
         .flags  = IORESOURCE_IRQ,
	},
};

static void ftgmac100_1_release(struct device *dev)
{
    return;
}

static struct platform_device ftgmac100_1_device = {
	.name	= DRV_1_NAME,
	.id		= 1,
	.num_resources	= ARRAY_SIZE(ftgmac100_1_resource),
	.resource	= ftgmac100_1_resource,
        .dev = {
        	.dma_mask = &ftgmac100_dmamask,
        	.coherent_dma_mask = DMA_BIT_MASK(32),
        	.release = ftgmac100_1_release,
        }, 	
};
#endif 

/******************************************************************************
 * private data
 *****************************************************************************/

struct ftgmac100_rx_reg_offset {
	unsigned int offset_rxpd;
	unsigned int offset_rxr_badr;
};

const struct ftgmac100_rx_reg_offset ftgmac100_rx_reg_offset[MAX_RX_QUEUES] = {
	{FTGMAC100_OFFSET_RXPD, FTGMAC100_OFFSET_RXR_BADR}
};

struct ftgmac100_rx_ring {
	unsigned int rx_num_entries;
	u16 rx_queue_index;
	dma_addr_t rx_dma_addr;
	struct ftgmac100_rxdes *rxdes;
	unsigned int rx_pointer;
	unsigned int rx_offset_rxpd;
	unsigned int rx_offset_rxr_badr;
	struct net_device *netdev;
	struct device *dev;
};

struct ftgmac100_tx_reg_offset {
	unsigned int offset_txpd;
	unsigned int offset_txr_badr;
};

const struct ftgmac100_tx_reg_offset ftgmac100_tx_reg_offset[MAX_TX_QUEUES] = {
	{FTGMAC100_OFFSET_NPTXPD, FTGMAC100_OFFSET_NPTXR_BADR},
	{FTGMAC100_OFFSET_HPTXPD, FTGMAC100_OFFSET_HPTXR_BADR}
};

struct ftgmac100_tx_ring {
	unsigned int tx_num_entries;
	u16 tx_queue_index;
	dma_addr_t tx_dma_addr;
	struct ftgmac100_txdes *txdes;
	unsigned int tx_clean_pointer;
	unsigned int tx_pointer;
	unsigned int tx_pending;

	unsigned int tx_offset_txpd;
	unsigned int tx_offset_txr_badr;
	struct net_device *netdev;
	struct device *dev;
};

static int ftgmac100_alloc_rx_page(struct ftgmac100_rx_ring *rx_ring,
				   struct ftgmac100_rxdes *rxdes, gfp_t gfp);
/******************************************************************************
 * internal functions (hardware register access)
 *****************************************************************************/
#define INT_MASK_ALL_ENABLED	(FTGMAC100_INT_RPKT_LOST	| \
				 FTGMAC100_INT_AHB_ERR		| \
				 FTGMAC100_INT_RPKT_BUF		| \
				 FTGMAC100_INT_NO_RXBUF)

#define INT_MASK_TX (FTGMAC100_INT_XPKT_ETH | FTGMAC100_INT_XPKT_LOST)

static void ftgmac100_set_rx_ring_base(struct ftgmac100 *priv, struct ftgmac100_rx_ring *rx_ring)
{
	iowrite32(rx_ring->rx_dma_addr, priv->base + rx_ring->rx_offset_rxr_badr);
}

static void ftgmac100_set_rx_buffer_size(struct ftgmac100 *priv,
		unsigned int size)
{
	size = FTGMAC100_RBSR_SIZE(size);
	iowrite32(size, priv->base + FTGMAC100_OFFSET_RBSR);
}

static void ftgmac100_set_tx_ring_base(struct ftgmac100 *priv, struct ftgmac100_tx_ring *tx_ring)
{
	iowrite32(tx_ring->tx_dma_addr, priv->base + tx_ring->tx_offset_txr_badr);
}

static void ftgmac100_txdma_start_polling(struct ftgmac100 *priv, struct ftgmac100_tx_ring *tx_ring)
{
	iowrite32(1, priv->base + tx_ring->tx_offset_txpd);
}


static int ftgmac100_reset_hw(struct ftgmac100 *priv)
{
#ifdef CONFIG_PLATFORM_GM8210    
	struct net_device *netdev = priv->netdev;
	int i;
#endif

    MDIO_first_time = 1;
#ifdef CONFIG_PLATFORM_GM8210
	/* NOTE: reset clears all registers */
	printk("reset MAC IP\n");
	iowrite32(FTGMAC100_MACCR_SW_RST, priv->base + FTGMAC100_OFFSET_MACCR);
	for (i = 0; i < 5; i++) {
		unsigned int maccr;

		maccr = ioread32(priv->base + FTGMAC100_OFFSET_MACCR);
		if (!(maccr & FTGMAC100_MACCR_SW_RST))
			return 0;

		wait_status(1);
	}

	netdev_err(netdev, "software reset failed\n");
	return -EIO;
#else
    mac_reset();
    return 0;
#endif	
}

static void ftgmac100_set_mac(struct ftgmac100 *priv, const unsigned char *mac)
{
	unsigned int maddr = mac[0] << 8 | mac[1];
	unsigned int laddr = mac[2] << 24 | mac[3] << 16 | mac[4] << 8 | mac[5];

	iowrite32(maddr, priv->base + FTGMAC100_OFFSET_MAC_MADR);
	iowrite32(laddr, priv->base + FTGMAC100_OFFSET_MAC_LADR);
}

static void ftgmac100_init_hw(struct ftgmac100 *priv)
{
	int i;
	
	if(priv->rx_rings != NULL) {
		/* setup ring buffer base registers */
		for (i = 0; i < priv->rx_num_queues; i++) {
			ftgmac100_set_rx_ring_base(priv, &priv->rx_rings[i]);
		}
		for (i = 0; i < priv->tx_num_queues; i++) {
			ftgmac100_set_tx_ring_base(priv, &priv->tx_rings[i]);
		}
	}
	ftgmac100_set_rx_buffer_size(priv, RX_BUF_SIZE);

	iowrite32(FTGMAC100_APTC_RXPOLL_CNT(0xF), priv->base + FTGMAC100_OFFSET_APTC);
	set_MDC_CLK(priv);
#if 1
    {
	unsigned int value;        
  // enable flow control
	value = ioread32(priv->base + FTGMAC100_OFFSET_FCR);			
	iowrite32(value | 0x1, priv->base + FTGMAC100_OFFSET_FCR);

  // enable back pressure register
	value = ioread32(priv->base + FTGMAC100_OFFSET_BPR);			
	iowrite32(value | 0x1, priv->base + FTGMAC100_OFFSET_BPR);
    }
#endif		
	//iowrite32(0xFF, priv->base + FTGMAC100_OFFSET_ITC);
	
	ftgmac100_set_mac(priv, priv->netdev->dev_addr);
}

#define MACCR_ENABLE_ALL	(FTGMAC100_MACCR_TXDMA_EN	| \
				 FTGMAC100_MACCR_RXDMA_EN	| \
				 FTGMAC100_MACCR_TXMAC_EN	| \
				 FTGMAC100_MACCR_RXMAC_EN	| \
				 FTGMAC100_MACCR_HPTXR_EN	| \
				 FTGMAC100_MACCR_CRC_APD	| \
				 FTGMAC100_MACCR_RX_RUNT	| \
				 FTGMAC100_MACCR_RX_BROADPKT | \
				 FTGMAC100_MACCR_JUMBO_LF)

static int ftgmac100_set_tfifo_size(struct ftgmac100 *priv, int size_sel);
static int ftgmac100_set_rfifo_size(struct ftgmac100 *priv, int size_sel);

static void ftgmac100_start_hw(struct ftgmac100 *priv, int speed, int duplex)
{
	int maccr = MACCR_ENABLE_ALL;

	switch (speed) {
	default:
	case 10:
		break;

	case 100:
		maccr |= FTGMAC100_MACCR_FAST_MODE;
		break;

	case 1000:
		maccr |= FTGMAC100_MACCR_GIGA_MODE;
		break;
	}

	if(duplex)
		maccr |= FTGMAC100_MACCR_FULLDUP;

	iowrite32(maccr, priv->base + FTGMAC100_OFFSET_MACCR);
	priv->tx_fifo_sel = ftgmac100_set_tfifo_size(priv, priv->tx_fifo_sel);
	priv->rx_fifo_sel = ftgmac100_set_rfifo_size(priv, priv->rx_fifo_sel);
}

static void ftgmac100_stop_hw(struct ftgmac100 *priv)
{
	iowrite32(0, priv->base + FTGMAC100_OFFSET_MACCR);
}
/******************************************************************************
 * internal functions (receive descriptor)
 *****************************************************************************/
static bool ftgmac100_rxdes_first_segment(struct ftgmac100_rxdes *rxdes)
{
	return rxdes->rxdes0 & cpu_to_le32(FTGMAC100_RXDES0_FRS);
}

static bool ftgmac100_rxdes_last_segment(struct ftgmac100_rxdes *rxdes)
{
	return rxdes->rxdes0 & cpu_to_le32(FTGMAC100_RXDES0_LRS);
}

static bool ftgmac100_rxdes_packet_ready(struct ftgmac100_rxdes *rxdes)
{
	return rxdes->rxdes0 & cpu_to_le32(FTGMAC100_RXDES0_RXPKT_RDY);
}

static void ftgmac100_rxdes_set_dma_own(struct ftgmac100_rxdes *rxdes)
{
	/* clear status bits */
	rxdes->rxdes0 &= cpu_to_le32(FTGMAC100_RXDES0_EDORR);
}

static bool ftgmac100_rxdes_rx_error(struct ftgmac100_rxdes *rxdes)
{
	return rxdes->rxdes0 & cpu_to_le32(FTGMAC100_RXDES0_RX_ERR);
}

static bool ftgmac100_rxdes_fifo_error(struct ftgmac100_rxdes *rxdes)
{
    return rxdes->rxdes0 & cpu_to_le32(FTGMAC100_RXDES0_FIFO_FULL);
}

static bool ftgmac100_rxdes_crc_error(struct ftgmac100_rxdes *rxdes)
{
    return rxdes->rxdes0 & cpu_to_le32(FTGMAC100_RXDES0_CRC_ERR);
}

static bool ftgmac100_rxdes_frame_too_long(struct ftgmac100_rxdes *rxdes)
{
	return rxdes->rxdes0 & cpu_to_le32(FTGMAC100_RXDES0_FTL);
}

static bool ftgmac100_rxdes_runt(struct ftgmac100_rxdes *rxdes)
{
	return rxdes->rxdes0 & cpu_to_le32(FTGMAC100_RXDES0_RUNT);
}

static bool ftgmac100_rxdes_odd_nibble(struct ftgmac100_rxdes *rxdes)
{
	return rxdes->rxdes0 & cpu_to_le32(FTGMAC100_RXDES0_RX_ODD_NB);
}

static unsigned int ftgmac100_rxdes_data_length(struct ftgmac100_rxdes *rxdes)
{
	return le32_to_cpu(rxdes->rxdes0) & FTGMAC100_RXDES0_VDBC;
}

static bool ftgmac100_rxdes_multicast(struct ftgmac100_rxdes *rxdes)
{
	return rxdes->rxdes0 & cpu_to_le32(FTGMAC100_RXDES0_MULTICAST);
}

static void ftgmac100_rxdes_set_end_of_ring(struct ftgmac100_rxdes *rxdes)
{
	rxdes->rxdes0 |= cpu_to_le32(FTGMAC100_RXDES0_EDORR);
}

static void ftgmac100_rxdes_set_dma_addr(struct ftgmac100_rxdes *rxdes,
					 dma_addr_t addr)
{
	rxdes->rxdes3 = cpu_to_le32(addr);
}

static dma_addr_t ftgmac100_rxdes_get_dma_addr(struct ftgmac100_rxdes *rxdes)
{
	return le32_to_cpu(rxdes->rxdes3);
}

static bool ftgmac100_rxdes_is_tcp(struct ftgmac100_rxdes *rxdes)
{
	return (rxdes->rxdes1 & cpu_to_le32(FTGMAC100_RXDES1_PROT_MASK)) ==
	       cpu_to_le32(FTGMAC100_RXDES1_PROT_TCPIP);
}

static bool ftgmac100_rxdes_is_udp(struct ftgmac100_rxdes *rxdes)
{
	return (rxdes->rxdes1 & cpu_to_le32(FTGMAC100_RXDES1_PROT_MASK)) ==
	       cpu_to_le32(FTGMAC100_RXDES1_PROT_UDPIP);
}

static bool ftgmac100_rxdes_tcpcs_err(struct ftgmac100_rxdes *rxdes)
{
	return rxdes->rxdes1 & cpu_to_le32(FTGMAC100_RXDES1_TCP_CHKSUM_ERR);
}

static bool ftgmac100_rxdes_udpcs_err(struct ftgmac100_rxdes *rxdes)
{
	return rxdes->rxdes1 & cpu_to_le32(FTGMAC100_RXDES1_UDP_CHKSUM_ERR);
}

static bool ftgmac100_rxdes_ipcs_err(struct ftgmac100_rxdes *rxdes)
{
	return rxdes->rxdes1 & cpu_to_le32(FTGMAC100_RXDES1_IP_CHKSUM_ERR);
}

/*
 * rxdes2 is not used by hardware. We use it to keep track of page.
 * Since hardware does not touch it, we can skip cpu_to_le32()/le32_to_cpu().
 */
static void ftgmac100_rxdes_set_page(struct ftgmac100_rxdes *rxdes, struct page *page)
{
	struct ftgmac100_page *p = (struct ftgmac100_page *)rxdes->rxdes2;
	//p->page_va = page_address(p->page);
	p->page = page;
}

static struct page *ftgmac100_rxdes_get_page(struct ftgmac100_rxdes *rxdes)
{
	struct ftgmac100_page *p = (struct ftgmac100_page *)rxdes->rxdes2;
	return p->page;
}

/******************************************************************************
 * internal functions (transmit & receive)
 *****************************************************************************/
static int ftgmac100_next_pointer(int pointer, int num_entries)
{
	return (pointer + 1) & (num_entries - 1);
}

/******************************************************************************
 * internal functions (receive)
 *****************************************************************************/
static void ftgmac100_rx_pointer_advance(struct ftgmac100_rx_ring *rx_ring)
{
	rx_ring->rx_pointer = ftgmac100_next_pointer(rx_ring->rx_pointer, rx_ring->rx_num_entries);
}

static struct ftgmac100_rxdes *ftgmac100_current_rxdes(struct ftgmac100_rx_ring *rx_ring)
{
	return &rx_ring->rxdes[rx_ring->rx_pointer];
}

static struct ftgmac100_rxdes *
ftgmac100_rx_locate_first_segment(struct ftgmac100_rx_ring *rx_ring)
{
	struct ftgmac100_rxdes *rxdes = ftgmac100_current_rxdes(rx_ring);

	while (ftgmac100_rxdes_packet_ready(rxdes)) {
		if (ftgmac100_rxdes_first_segment(rxdes))
			return rxdes;

		ftgmac100_rxdes_set_dma_own(rxdes);
		ftgmac100_rx_pointer_advance(rx_ring);
		rxdes = ftgmac100_current_rxdes(rx_ring);
	}

	return NULL;
}

static bool ftgmac100_rx_packet_error(struct ftgmac100_rx_ring *rx_ring,
				      struct ftgmac100_rxdes *rxdes)
{
	struct net_device *netdev = rx_ring->netdev;
	bool error = false;

	if (unlikely(ftgmac100_rxdes_fifo_error(rxdes))) {
		if (show_rx_dbg)
			netdev_info(netdev, "MAC rx fifo full, lose\n");

		error = true;
	} else {
    	if (unlikely(ftgmac100_rxdes_rx_error(rxdes))) {
    		if (show_rx_dbg)
    			netdev_info(netdev, "rx err\n");
    
    		error = true;
    	} else if (unlikely(ftgmac100_rxdes_crc_error(rxdes))) {
    	    if (show_rx_dbg)
    			netdev_info(netdev, "rx crc err\n");
    
    		netdev->stats.rx_crc_errors++;
    		error = true;
    	} else if (unlikely(ftgmac100_rxdes_ipcs_err(rxdes))) {
    		if (show_rx_dbg)
    			netdev_info(netdev, "rx IP checksum err\n");
            
            netdev->stats.rx_frame_errors++;
    		error = true;
    	} else if (unlikely(ftgmac100_rxdes_frame_too_long(rxdes))) {
    		if (show_rx_dbg)
    			netdev_info(netdev, "rx frame too long\n");
    
    		netdev->stats.rx_length_errors++;
    		error = true;
    	} else if (unlikely(ftgmac100_rxdes_runt(rxdes))) {
    		if (show_rx_dbg)
    			netdev_info(netdev, "rx runt\n");
    
    		netdev->stats.rx_length_errors++;
    		error = true;
    	} else if (unlikely(ftgmac100_rxdes_odd_nibble(rxdes))) {
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

static void ftgmac100_rx_drop_packet(struct ftgmac100_rx_ring *rx_ring)
{
	struct net_device *netdev = rx_ring->netdev;
	struct ftgmac100_rxdes *rxdes = ftgmac100_current_rxdes(rx_ring);
	bool done = false;

	if (show_rx_dbg)
		netdev_err(netdev, "drop packet %p\n", rxdes);

	do {
		if (ftgmac100_rxdes_last_segment(rxdes))
			done = true;

		ftgmac100_rxdes_set_dma_own(rxdes);
		ftgmac100_rx_pointer_advance(rx_ring);
		rxdes = ftgmac100_current_rxdes(rx_ring);
	} while (!done && ftgmac100_rxdes_packet_ready(rxdes));
}

static bool ftgmac100_process_rx_jumbo_ring(struct ftgmac100_rx_ring *rx_ring, struct napi_struct *napi)
{
	struct net_device *netdev = rx_ring->netdev;
	struct ftgmac100_rxdes *rxdes;
	struct sk_buff *skb;
	bool done = false;
	int i;

	rxdes = ftgmac100_rx_locate_first_segment(rx_ring);
	if (!rxdes)
		return false;

	if (unlikely(ftgmac100_rx_packet_error(rx_ring, rxdes))) {
		ftgmac100_rx_drop_packet(rx_ring);
		netdev->stats.rx_dropped++;
		return true;
	}

	/* start processing */
	skb = netdev_alloc_skb_ip_align(netdev, 128);
	if (unlikely(!skb)) {
		netdev_err(netdev, "rx skb alloc failed\n");

		ftgmac100_rx_drop_packet(rx_ring);
		netdev->stats.rx_dropped++;

		return true;
	}

	if (unlikely(ftgmac100_rxdes_multicast(rxdes)))
		netdev->stats.multicast++;

	/*
	 * It seems that HW does checksum incorrectly with fragmented packets,
	 * so we are conservative here - if HW checksum error, let software do
	 * the checksum again.
	 */
	if ((ftgmac100_rxdes_is_tcp(rxdes) && !ftgmac100_rxdes_tcpcs_err(rxdes)) ||
	    (ftgmac100_rxdes_is_udp(rxdes) && !ftgmac100_rxdes_udpcs_err(rxdes)))
		skb->ip_summed = CHECKSUM_UNNECESSARY;

	do {
		dma_addr_t map = ftgmac100_rxdes_get_dma_addr(rxdes);
		struct page *page = ftgmac100_rxdes_get_page(rxdes);
		unsigned int size;

		dma_unmap_page(rx_ring->dev, map, RX_BUF_SIZE, DMA_FROM_DEVICE);

		size = ftgmac100_rxdes_data_length(rxdes);
		skb_fill_page_desc(skb, skb_shinfo(skb)->nr_frags, page, 0, size);

		skb->len += size;
		skb->data_len += size;
		skb->truesize += RX_BUF_SIZE;

		if (ftgmac100_rxdes_last_segment(rxdes))
			done = true;

		ftgmac100_alloc_rx_page(rx_ring, rxdes, GFP_ATOMIC);

		ftgmac100_rx_pointer_advance(rx_ring);
		rxdes = ftgmac100_current_rxdes(rx_ring);
	} while (!done);

	if (skb->len <= 64)
		skb->truesize -= RX_BUF_SIZE;
	__pskb_pull_tail(skb, min(skb->len, 64U));
	skb->protocol = eth_type_trans(skb, netdev);

	netdev->stats.rx_packets++;
	netdev->stats.rx_bytes += skb->len;

	if(show_rx_pkg) {
	    printk("<rx pkg=");
	    for(i = 0; i < 10; i++)
	    	printk("0x%02x ", *(skb->mac_header + i));
	    for(i = 0; i < 10; i++)
	        printk("0x%02x ", *(skb->data + i));
        printk(">\n");
    }
    
	/* push packet to protocol stack */
	napi_gro_receive(napi, skb);
	return true;
}

static bool ftgmac100_rx_packet(struct ftgmac100 *priv)
{
	int i;
	bool ret = false;
	for (i = 0; i < priv->rx_num_queues; i++) {
		if (priv->process_rx_ring(&priv->rx_rings[i], &priv->napi))
			ret = true;
	}
	return ret;
}

/******************************************************************************
 * internal functions (transmit descriptor)
 *****************************************************************************/
static void ftgmac100_txdes_reset(struct ftgmac100_txdes *txdes)
{
	/* clear all except end of ring bit */
	txdes->txdes0 &= cpu_to_le32(FTGMAC100_TXDES0_EDOTR);
	txdes->txdes1 = 0;
	txdes->txdes2 = 0;
	txdes->txdes3 = 0;
}

static bool ftgmac100_txdes_owned_by_dma(struct ftgmac100_txdes *txdes)
{
	return txdes->txdes0 & cpu_to_le32(FTGMAC100_TXDES0_TXDMA_OWN);
}

static bool ftgmac100_txdes_first_segment(struct ftgmac100_txdes *txdes)
{
	return txdes->txdes0 & cpu_to_le32(FTGMAC100_TXDES0_FTS);
}

static void ftgmac100_txdes_set_dma_own(struct ftgmac100_txdes *txdes)
{
	/*
	 * Make sure dma own bit will not be set before any other
	 * descriptor fields.
	 */
	wmb();
	txdes->txdes0 |= cpu_to_le32(FTGMAC100_TXDES0_TXDMA_OWN);
}

static void ftgmac100_txdes_set_end_of_ring(struct ftgmac100_txdes *txdes)
{
	txdes->txdes0 |= cpu_to_le32(FTGMAC100_TXDES0_EDOTR);
}

static void ftgmac100_txdes_set_first_segment(struct ftgmac100_txdes *txdes)
{
	txdes->txdes0 |= cpu_to_le32(FTGMAC100_TXDES0_FTS);
}

static void ftgmac100_txdes_set_last_segment(struct ftgmac100_txdes *txdes)
{
	txdes->txdes0 |= cpu_to_le32(FTGMAC100_TXDES0_LTS);
}

static void ftgmac100_txdes_set_buffer_size(struct ftgmac100_txdes *txdes,
					    unsigned int len)
{
	txdes->txdes0 |= cpu_to_le32(FTGMAC100_TXDES0_TXBUF_SIZE(len));
}

static void ftgmac100_txdes_set_txint(struct ftgmac100_txdes *txdes)
{
	txdes->txdes1 |= cpu_to_le32(FTGMAC100_TXDES1_TXIC);
}

static void ftgmac100_txdes_set_tcpcs(struct ftgmac100_txdes *txdes)
{
	txdes->txdes1 |= cpu_to_le32(FTGMAC100_TXDES1_TCP_CHKSUM);
}

static void ftgmac100_txdes_set_udpcs(struct ftgmac100_txdes *txdes)
{
	txdes->txdes1 |= cpu_to_le32(FTGMAC100_TXDES1_UDP_CHKSUM);
}

static void ftgmac100_txdes_set_ipcs(struct ftgmac100_txdes *txdes)
{
	txdes->txdes1 |= cpu_to_le32(FTGMAC100_TXDES1_IP_CHKSUM);
}

static void ftgmac100_txdes_set_used(struct ftgmac100_txdes *txdes)
{
	txdes->txdes1 |= cpu_to_le32(FTGMAC100_TXDES1_USED);
}

static void ftgmac100_txdes_set_dma_addr(struct ftgmac100_txdes *txdes,
					 dma_addr_t addr)
{
	txdes->txdes3 = cpu_to_le32(addr);
}

static dma_addr_t ftgmac100_txdes_get_dma_addr(struct ftgmac100_txdes *txdes)
{
	return le32_to_cpu(txdes->txdes3);
}

/*
 * txdes2 is not used by hardware. We use it to keep track of socket buffer.
 * Since hardware does not touch it, we can skip cpu_to_le32()/le32_to_cpu().
 */
static void ftgmac100_txdes_set_skb(struct ftgmac100_txdes *txdes,
				    struct sk_buff *skb)
{
	txdes->txdes2 = (unsigned int)skb;
}

static struct sk_buff *ftgmac100_txdes_get_skb(struct ftgmac100_txdes *txdes)
{
	return (struct sk_buff *)txdes->txdes2;
}

/******************************************************************************
 * internal functions (transmit)
 *****************************************************************************/
static void ftgmac100_tx_clean_pointer_advance(struct ftgmac100_tx_ring *tx_ring)
{
	tx_ring->tx_clean_pointer = ftgmac100_next_pointer(tx_ring->tx_clean_pointer, tx_ring->tx_num_entries);
}

static int ftgmac100_current_txdes_idx(struct ftgmac100_tx_ring *tx_ring)
{
	return tx_ring->tx_pointer;
}

static struct ftgmac100_txdes *ftgmac100_idx_txdes(struct ftgmac100_tx_ring *tx_ring, int idx)
{
	if (idx >= tx_ring->tx_num_entries)
		return NULL;
	return &tx_ring->txdes[idx];
}

static struct ftgmac100_txdes *
ftgmac100_current_clean_txdes(struct ftgmac100_tx_ring *tx_ring)
{
	return &tx_ring->txdes[tx_ring->tx_clean_pointer];
}

static bool ftgmac100_clean_tx_ring(struct ftgmac100 *priv, struct ftgmac100_tx_ring *tx_ring, int *nr_frag)
{
	struct net_device *netdev = tx_ring->netdev;
	struct ftgmac100_txdes *txdes;
	struct sk_buff *skb;
	dma_addr_t map;

    spin_lock(&priv->mac_lock);
	/* spinlock protect */
	if (tx_ring->tx_pending == 0)
		goto out;

	txdes = ftgmac100_current_clean_txdes(tx_ring);

	if (ftgmac100_txdes_owned_by_dma(txdes))
		goto out;

    spin_unlock(&priv->mac_lock);

	skb = ftgmac100_txdes_get_skb(txdes);

	/* unmap the first txdes (single buffer) */
	map = ftgmac100_txdes_get_dma_addr(txdes);
	if (ftgmac100_txdes_first_segment(txdes)) {
		dma_unmap_single(tx_ring->dev, map, skb_headlen(skb), DMA_TO_DEVICE);
		(*nr_frag) = 0;
	}
	else {
		const struct skb_frag_struct *frag = &skb_shinfo(skb)->frags[*nr_frag];
		dma_unmap_page(tx_ring->dev, map, skb_frag_size(frag), DMA_TO_DEVICE);
		(*nr_frag)++;
	}

	netdev->stats.tx_packets++;
	netdev->stats.tx_bytes += skb->len;
	dev_kfree_skb(skb);

	ftgmac100_tx_clean_pointer_advance(tx_ring);
	ftgmac100_txdes_reset(txdes);

	spin_lock(&priv->mac_lock);
	tx_ring->tx_pending--;
	spin_unlock(&priv->mac_lock);

	return true;
out:
	spin_unlock(&priv->mac_lock);
	return false;
}

static void ftgmac100_tx_complete(struct ftgmac100 *priv)
{
	int i, nr_frag = 0;
	for (i = 0; i < priv->tx_num_queues; i++) {
		while(ftgmac100_clean_tx_ring(priv, &priv->tx_rings[i], &nr_frag))
			;
	}
}

/*
 * In order to prevent the critical section of these tx descriptors,
 * we need to reserve these tx descriptors in advance.
 * return value is the index of the first tx descriptor.
 */
static int ftgmac100_reserve_txdeses(struct ftgmac100 *priv, struct ftgmac100_tx_ring *tx_ring, int num)
{
	int cur_idx = ftgmac100_current_txdes_idx(tx_ring);

	spin_lock(&priv->mac_lock);

	if ((TX_QUEUE_ENTRIES - tx_ring->tx_pending) < num)
		goto tx_full;

	tx_ring->tx_pointer += num;
	if (tx_ring->tx_pointer >= TX_QUEUE_ENTRIES)
		tx_ring->tx_pointer -= TX_QUEUE_ENTRIES;

	//tx_ring->tx_pending += num;//tx_ring->tx_pending ++;

	spin_unlock(&priv->mac_lock);

	return cur_idx;

tx_full:
	spin_unlock(&priv->mac_lock);
	return -1;
}

static int ftgmac100_xmit(struct ftgmac100 *priv, struct sk_buff *skb)
{
	struct net_device *netdev = priv->netdev;
	struct ftgmac100_tx_ring *tx_ring = &priv->tx_rings[skb->queue_mapping];
	struct ftgmac100_txdes *txdes = NULL, *first_txdes = NULL;
	int txdes_idx;
	int i;
	unsigned int num_txdeses = 1;
	const struct skb_frag_struct *frag;
	dma_addr_t map;
	unsigned int total_len = 0, len;

	num_txdeses += skb_shinfo(skb)->nr_frags;

	/* the first descriptor must be set at last */
	txdes_idx = ftgmac100_reserve_txdeses(priv, tx_ring, num_txdeses);

	if (unlikely(txdes_idx < 0)) {
		netif_stop_queue(netdev);
		if (show_rx_dbg)
			printk("MAC tx queue full, wait to digest\n");
		
		return NETDEV_TX_BUSY;
	}

#if 0//debug
	if (tx_ring->tx_pending > 480) {
		printk(">>> tx pending %d <<<\n", tx_ring->tx_pending);
	}
#endif	
	if ((TX_QUEUE_ENTRIES - tx_ring->tx_pending) < TX_QUEUE_THRESHOLD)
		iowrite32(INT_MASK_TX, priv->base + FTGMAC100_OFFSET_IER);

	first_txdes = ftgmac100_idx_txdes(tx_ring, txdes_idx);
	txdes_idx = ftgmac100_next_pointer(txdes_idx, tx_ring->tx_num_entries);

	if (num_txdeses > 1) {
		txdes = ftgmac100_idx_txdes(tx_ring, txdes_idx);
		txdes_idx = ftgmac100_next_pointer(txdes_idx, tx_ring->tx_num_entries);

		for (i = 1; i < (num_txdeses - 1); i++) {
			frag = &skb_shinfo(skb)->frags[i - 1];
			len = skb_frag_size(frag);
			total_len += len;
			map = skb_frag_dma_map(priv->dev, frag, 0, len, DMA_TO_DEVICE);

			if (unlikely(dma_mapping_error(priv->dev, map)))
				goto dma_err;
			/* setup TX descriptor */
			ftgmac100_txdes_set_skb(txdes, skb);
			ftgmac100_txdes_set_dma_addr(txdes, map);
			ftgmac100_txdes_set_buffer_size(txdes, len);

			ftgmac100_txdes_set_dma_own(txdes);
			ftgmac100_txdes_set_used(txdes);
			txdes = ftgmac100_idx_txdes(tx_ring, txdes_idx);
			txdes_idx = ftgmac100_next_pointer(txdes_idx, tx_ring->tx_num_entries);
		}

		/* setup the last descriptor */
		frag = &skb_shinfo(skb)->frags[i - 1];
		len = skb_frag_size(frag);
		
		map = skb_frag_dma_map(priv->dev, frag, 0, len, DMA_TO_DEVICE);

		if (unlikely(dma_mapping_error(priv->dev, map)))
			goto dma_err;

		ftgmac100_txdes_set_skb(txdes, skb);
		ftgmac100_txdes_set_dma_addr(txdes, map);
		
		if((total_len + len + skb_headlen(skb)) < ETH_ZLEN)
		    len = ETH_ZLEN - skb_headlen(skb) - total_len;

        total_len += len;

		ftgmac100_txdes_set_buffer_size(txdes, len);
		ftgmac100_txdes_set_last_segment(txdes);

		ftgmac100_txdes_set_dma_own(txdes);
		ftgmac100_txdes_set_used(txdes);
	}

	/* setup the first descriptor */
	len = skb_headlen(skb);
	total_len += len;
	map = dma_map_single(priv->dev, skb->data, len, DMA_TO_DEVICE);

	if(show_tx_pkg) {
	    printk("<tx pkg=");
	    for(i = 0; i < len; i++)
	        printk("0x%02x ", *(skb->data + i));
        printk(">\n");
    }
  
	if (unlikely(dma_mapping_error(priv->dev, map)))
		goto dma_err;

	ftgmac100_txdes_set_skb(first_txdes, skb);
	ftgmac100_txdes_set_dma_addr(first_txdes, map);

	if (1 == num_txdeses) {
		len = (total_len < ETH_ZLEN) ? ETH_ZLEN : len;
		ftgmac100_txdes_set_buffer_size(first_txdes, len);
	}
	else
		ftgmac100_txdes_set_buffer_size(first_txdes, len);

	ftgmac100_txdes_set_first_segment(first_txdes);

	if (1 == num_txdeses)
		ftgmac100_txdes_set_last_segment(first_txdes);

	ftgmac100_txdes_set_txint(first_txdes);

	if (skb->ip_summed == CHECKSUM_PARTIAL) {
		__be16 protocol = skb->protocol;

		if (protocol == cpu_to_be16(ETH_P_IP)) {
			u8 ip_proto = ip_hdr(skb)->protocol;

			ftgmac100_txdes_set_ipcs(first_txdes);
			if (ip_proto == IPPROTO_TCP)
				ftgmac100_txdes_set_tcpcs(first_txdes);
			else if (ip_proto == IPPROTO_UDP)
				ftgmac100_txdes_set_udpcs(first_txdes);
		}
	}

	/* spinlock protect */
    spin_lock(&priv->mac_lock);
	ftgmac100_txdes_set_dma_own(first_txdes);
	ftgmac100_txdes_set_used(first_txdes);
	tx_ring->tx_pending ++;

	if (unlikely(tx_ring->tx_pending == TX_QUEUE_ENTRIES)) {
		netif_stop_queue(netdev);
		if (show_rx_dbg)
			printk("MAC tx queue full, wait to digest!\n");
	}
	spin_unlock(&priv->mac_lock);

	ftgmac100_txdma_start_polling(priv, tx_ring);
	return NETDEV_TX_OK;

dma_err:
	netdev_err(netdev, "map socket buffer failed\n");

	netdev->stats.tx_dropped++;
	dev_kfree_skb(skb);
	return NETDEV_TX_OK;
}

/******************************************************************************
 * internal functions (buffer)
 *****************************************************************************/
static int ftgmac100_alloc_tx_entries(struct ftgmac100_tx_ring *tx_ring, 
					unsigned int num_entries, 
					const struct ftgmac100_tx_reg_offset *reg_offset)
{
	struct net_device *netdev = tx_ring->netdev;

	tx_ring->tx_num_entries = num_entries;
	tx_ring->txdes = dma_alloc_coherent(tx_ring->dev, 
										tx_ring->tx_num_entries * sizeof(struct ftgmac100_txdes), 
										&tx_ring->tx_dma_addr, GFP_ATOMIC);
	if (!tx_ring->txdes) {
		netdev_err(netdev, "failed to allocate tx descriptors\n");
		return -ENOMEM;
	}
	tx_ring->tx_clean_pointer = 0;
	tx_ring->tx_pointer = 0;
	tx_ring->tx_pending = 0;

	tx_ring->tx_offset_txpd = reg_offset->offset_txpd;
	tx_ring->tx_offset_txr_badr = reg_offset->offset_txr_badr;
	ftgmac100_txdes_set_end_of_ring(&tx_ring->txdes[tx_ring->tx_num_entries - 1]);
	return 0;
}

static int ftgmac100_alloc_rx_page(struct ftgmac100_rx_ring *rx_ring,
				   struct ftgmac100_rxdes *rxdes, gfp_t gfp)
{
	struct net_device *netdev = rx_ring->netdev;
	struct page *page;
	dma_addr_t map;

	page = alloc_page(gfp);
	if (!page) {
		netdev_err(netdev, "failed to allocate rx page\n");
		return -ENOMEM;
	}

	map = dma_map_page(rx_ring->dev, page, 0, RX_BUF_SIZE, DMA_FROM_DEVICE);
	if (unlikely(dma_mapping_error(rx_ring->dev, map))) {
		netdev_err(netdev, "failed to map rx page\n");
		__free_page(page);
		return -ENOMEM;
	}

	ftgmac100_rxdes_set_page(rxdes, page);
	ftgmac100_rxdes_set_dma_addr(rxdes, map);
	ftgmac100_rxdes_set_dma_own(rxdes);
	return 0;
}

static int ftgmac100_alloc_rx_entries(struct ftgmac100_rx_ring *rx_ring,
				unsigned int num_entries,
				const struct ftgmac100_rx_reg_offset *reg_offset)
{
	int i;
	struct net_device *netdev = rx_ring->netdev;

	rx_ring->rx_num_entries = num_entries;
	rx_ring->rxdes = dma_alloc_coherent(rx_ring->dev, 
										rx_ring->rx_num_entries * sizeof(struct ftgmac100_rxdes), 
										&rx_ring->rx_dma_addr, GFP_ATOMIC);

	if (!rx_ring->rxdes) {
		netdev_err(netdev, "failed to allocate rx descriptors\n");
		return -ENOMEM;
	}

	for (i = 0; i < rx_ring->rx_num_entries; i++) {
		struct ftgmac100_rxdes *rxdes = &rx_ring->rxdes[i];
		rxdes->rxdes2 = (unsigned int)vzalloc(sizeof(struct ftgmac100_page));
		if (ftgmac100_alloc_rx_page(rx_ring, rxdes, GFP_ATOMIC | GFP_DMA)) {
		    netdev_err(netdev, "failed to allocate rx page\n");
			return -ENOMEM;
		}
    }
	rx_ring->rx_pointer = 0;
	rx_ring->rx_offset_rxpd = reg_offset->offset_rxpd;
	rx_ring->rx_offset_rxr_badr = reg_offset->offset_rxr_badr;
	ftgmac100_rxdes_set_end_of_ring(&rx_ring->rxdes[rx_ring->rx_num_entries - 1]);
	return 0;
}

static void ftgmac100_free_rxdes(struct ftgmac100_rx_ring *rx_ring)
{
	int i;
	if (rx_ring->rxdes) {
		for (i = 0; i < rx_ring->rx_num_entries; i++) {
			struct ftgmac100_rxdes *rxdes = &rx_ring->rxdes[i];
			struct page *page = ftgmac100_rxdes_get_page(rxdes);
			dma_addr_t map = ftgmac100_rxdes_get_dma_addr(rxdes);

			if (!page)
				continue;

			dma_unmap_page(rx_ring->dev, map, RX_BUF_SIZE, DMA_FROM_DEVICE);
			__free_page(page);
			vfree((void *)rxdes->rxdes2);
		}
		dma_free_coherent(rx_ring->dev, rx_ring->rx_num_entries * sizeof(struct ftgmac100_rxdes),
					rx_ring->rxdes, rx_ring->rx_dma_addr);
	}
}

static void ftgmac100_free_txdes(struct ftgmac100 *priv, struct ftgmac100_tx_ring *tx_ring)
{
	int i;
	if (tx_ring->txdes) {
		/* free txdes on by one */
    	for (i = 0; i < TX_QUEUE_ENTRIES; i++) {
    		struct ftgmac100_txdes *txdes = &tx_ring->txdes[i];
    		struct sk_buff *skb = ftgmac100_txdes_get_skb(txdes);
    		dma_addr_t map = ftgmac100_txdes_get_dma_addr(txdes);
    
    		if (!skb)
    			continue;
    
    		dma_unmap_single(tx_ring->dev, map, skb_headlen(skb), DMA_TO_DEVICE);
    		kfree_skb(skb);
    	}
		dma_free_coherent(tx_ring->dev, tx_ring->tx_num_entries * sizeof(struct ftgmac100_txdes),
					tx_ring->txdes, tx_ring->tx_dma_addr);
	}
}

static void ftgmac100_free_buffers(struct ftgmac100 *priv)
{
	int i;

	if (priv->rx_rings) {
		for (i = 0; i < priv->rx_num_queues; i++) {
			ftgmac100_free_rxdes(&priv->rx_rings[i]);
		}
		vfree(priv->rx_rings);
	}

	if (priv->tx_rings) {
		for (i = 0; i< priv->tx_num_queues; i++) {
			ftgmac100_free_txdes(priv, &priv->tx_rings[i]);
		}
		vfree(priv->tx_rings);
	}
}

static int ftgmac100_alloc_buffers(struct ftgmac100 *priv)
{
	int i;

	priv->rx_rings = vzalloc(priv->rx_num_queues * sizeof(struct ftgmac100_rx_ring));
	if (!priv->rx_rings){
	    printk(KERN_ERR "GMAC alloc rx rings fail\n");
		goto err;
	}
	priv->tx_rings = vzalloc(priv->tx_num_queues * sizeof(struct ftgmac100_tx_ring));
	if (!priv->tx_rings){
	    printk(KERN_ERR "GMAC alloc tx rings fail\n");
		goto err;
	}

	/* initialize RX ring */
	for (i = 0; i < priv->rx_num_queues; i++) {
		priv->rx_rings[i].netdev = priv->netdev;
		priv->rx_rings[i].dev = priv->dev;
		priv->rx_rings[i].rx_queue_index = i;
		if (ftgmac100_alloc_rx_entries(&priv->rx_rings[i], 
										RX_QUEUE_ENTRIES, 
										&ftgmac100_rx_reg_offset[i]))
			goto err;
	}

	/* initialize TX ring */
	for (i = 0; i < priv->tx_num_queues; i++) {
		priv->tx_rings[i].netdev = priv->netdev;
		priv->tx_rings[i].dev = priv->dev;
		priv->tx_rings[i].tx_queue_index = i;
		if (ftgmac100_alloc_tx_entries(&priv->tx_rings[i], 
										TX_QUEUE_ENTRIES, 
										&ftgmac100_tx_reg_offset[i]))
			goto err;
	}
	return 0;

err:
	ftgmac100_free_buffers(priv);
	return -ENOMEM;
}
    
/******************************************************************************
 * internal functions (mdio)
 *****************************************************************************/
static void ftgmac100_adjust_link(struct net_device *netdev)
{
	struct ftgmac100 *priv = netdev_priv(netdev);
	struct phy_device *phydev = priv->phydev;
	int err;
	unsigned int maht0, maht1;
	unsigned int maccr = 0x0, rx_mode = 0x0;

#ifndef CONFIG_PLATFORM_GM8210
	if (phydev->speed == 1000) {
	    if(interface_type())
		    return;
	}
#endif	
	if (phydev->speed == priv->old_speed && phydev->duplex == priv->old_duplex)
		return;

	if (phydev->speed == 0)
		    return;

	printk("phy speed is %d, %s duplex\n", phydev->speed, phydev->duplex ? "full" : "half");
	/* some PHY must reset when link change */
	if(first_time)
		set_mac_clock(priv->irq);
	first_time = 1;
	
	priv->old_speed = phydev->speed;
	priv->old_duplex = phydev->duplex;

	/* keep rx mode */
	rx_mode = ioread32(priv->base + FTGMAC100_OFFSET_MACCR);
	rx_mode &= (FTGMAC100_MACCR_RX_BROADPKT | FTGMAC100_MACCR_RX_MULTIPKT | FTGMAC100_MACCR_HT_MULTI_EN | FTGMAC100_MACCR_RX_ALL);

	maht0 = ioread32(priv->base + FTGMAC100_OFFSET_MAHT0);
	maht1 = ioread32(priv->base + FTGMAC100_OFFSET_MAHT1);
#if 1
    netif_stop_queue(netdev);
    
    iowrite32(0, priv->base + FTGMAC100_OFFSET_IER);
    
    ftgmac100_stop_hw(priv);
    
    ftgmac100_reset_hw(priv);
    ftgmac100_free_buffers(priv);
    
    err = ftgmac100_alloc_buffers(priv);
    if (err)
    	return;
    
    ftgmac100_init_hw(priv);
    ftgmac100_start_hw(priv, phydev->speed, phydev->duplex);
    
    netif_start_queue(netdev);
#endif
#ifdef CONFIG_FTGMAC100_STORM
	priv->broadcast_num = 0;
	priv->begin_jiffies = 0;
#endif
	/* restore multicast settings */
	maccr = ioread32(priv->base + FTGMAC100_OFFSET_MACCR);
	maccr |= rx_mode;
	iowrite32(maccr, priv->base + FTGMAC100_OFFSET_MACCR);

    iowrite32(maht0, priv->base + FTGMAC100_OFFSET_MAHT0);
    iowrite32(maht1, priv->base + FTGMAC100_OFFSET_MAHT1);

	/* enable all interrupts */
	iowrite32(INT_MASK_ALL_ENABLED, priv->base + FTGMAC100_OFFSET_IER);
	
}

static int ftgmac100_mii_probe(struct ftgmac100 *priv)
{
	struct net_device *netdev = priv->netdev;
	struct phy_device *phydev = NULL;
	int i;

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

	phydev = phy_connect(netdev, dev_name(&phydev->dev),
			     &ftgmac100_adjust_link, 0,
			     PHY_INTERFACE_MODE_GMII);

	if (IS_ERR(phydev)) {
		netdev_err(netdev, "%s: Could not attach to PHY\n", netdev->name);
		return PTR_ERR(phydev);
	}

	priv->phydev = phydev;
	return 0;
}

/******************************************************************************
 * internal functions (hash table)
 ******************************************************************************/
static unsigned int ftgmac100_get_ht_index(unsigned char *mac_addr)
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

/******************************************************************************
 * struct mii_bus functions
 *****************************************************************************/
static int ftgmac100_dummy_read(struct mii_bus *bus, int phy_addr, int regnum)
{
	struct net_device *netdev = bus->priv;
	struct ftgmac100 *priv = netdev_priv(netdev);
	unsigned int phycr;
	int i;

	phycr = ioread32(priv->base + FTGMAC100_OFFSET_PHYCR);

	/* preserve MDC cycle threshold */
	phycr &= FTGMAC100_PHYCR_MDC_CYCTHR_MASK;

	phycr |= FTGMAC100_PHYCR_PHYAD(phy_addr) |
		 FTGMAC100_PHYCR_REGAD(regnum) |
		 FTGMAC100_PHYCR_MIIRD;

	iowrite32(phycr, priv->base + FTGMAC100_OFFSET_PHYCR);

	for (i = 0; i < 100; i++) {
		phycr = ioread32(priv->base + FTGMAC100_OFFSET_PHYCR);

		if ((phycr & FTGMAC100_PHYCR_MIIRD) == 0) {
			int data;

			data = ioread32(priv->base + FTGMAC100_OFFSET_PHYDATA);
			return FTGMAC100_PHYDATA_MIIRDATA(data);
		}

		wait_status(1);
	}

	netdev_err(netdev, "mdio read timed out\n");
	return -EIO;
}

static int ftgmac100_mdiobus_read(struct mii_bus *bus, int phy_addr, int regnum)
{
	struct net_device *netdev = bus->priv;
	struct ftgmac100 *priv = netdev_priv(netdev);
	unsigned int phycr;
	int i, data;

	if(MDIO_first_time) {	/* for 8287 and 8139 */    
	    data = ftgmac100_dummy_read(bus, phy_addr, regnum);
	    MDIO_first_time = 0;
	}
	    
	phycr = ioread32(priv->base + FTGMAC100_OFFSET_PHYCR);

	/* preserve MDC cycle threshold */
	phycr &= FTGMAC100_PHYCR_MDC_CYCTHR_MASK;

	phycr |= FTGMAC100_PHYCR_PHYAD(phy_addr) |
		 FTGMAC100_PHYCR_REGAD(regnum) |
		 FTGMAC100_PHYCR_MIIRD;

	iowrite32(phycr, priv->base + FTGMAC100_OFFSET_PHYCR);

	for (i = 0; i < 100; i++) {
		phycr = ioread32(priv->base + FTGMAC100_OFFSET_PHYCR);

		if ((phycr & FTGMAC100_PHYCR_MIIRD) == 0) {

			data = ioread32(priv->base + FTGMAC100_OFFSET_PHYDATA);
			data = FTGMAC100_PHYDATA_MIIRDATA(data);
			
			if(show_phy)
			    printk("<r:adr=%d,reg=%d,data=0x%x>", phy_addr, regnum, data);
			    
			return data;
		}

		wait_status(1);
	}

	netdev_err(netdev, "mdio read timed out\n");
	return -EIO;
}

static int ftgmac100_mdiobus_write(struct mii_bus *bus, int phy_addr,
				   int regnum, u16 value)
{
	struct net_device *netdev = bus->priv;
	struct ftgmac100 *priv = netdev_priv(netdev);
	unsigned int phycr;
	int data;
	int i;

    if(show_phy)
        printk("<w:adr=%d,reg=%d,data=0x%x>", phy_addr, regnum, value);
        
	phycr = ioread32(priv->base + FTGMAC100_OFFSET_PHYCR);

	/* preserve MDC cycle threshold */
	phycr &= FTGMAC100_PHYCR_MDC_CYCTHR_MASK;

	phycr |= FTGMAC100_PHYCR_PHYAD(phy_addr) |
		 FTGMAC100_PHYCR_REGAD(regnum) |
		 FTGMAC100_PHYCR_MIIWR;

	data = FTGMAC100_PHYDATA_MIIWDATA(value);

	iowrite32(data, priv->base + FTGMAC100_OFFSET_PHYDATA);
	iowrite32(phycr, priv->base + FTGMAC100_OFFSET_PHYCR);

	for (i = 0; i < 100; i++) {
		phycr = ioread32(priv->base + FTGMAC100_OFFSET_PHYCR);

		if ((phycr & FTGMAC100_PHYCR_MIIWR) == 0)
			return 0;

		wait_status(1);
	}

	netdev_err(netdev, "mdio write timed out\n");
	return -EIO;
}

static int ftgmac100_mdiobus_reset(struct mii_bus *bus)
{
	return 0;
}

/******************************************************************************
 * struct ethtool_ops functions
 *****************************************************************************/
static void ftgmac100_get_drvinfo(struct net_device *netdev,
				  struct ethtool_drvinfo *info)
{
	strcpy(info->driver, DRV_NAME);
	strcpy(info->version, DRV_VERSION);
	strcpy(info->bus_info, dev_name(&netdev->dev));
}

static int ftgmac100_get_settings(struct net_device *netdev,
				  struct ethtool_cmd *cmd)
{
	struct ftgmac100 *priv = netdev_priv(netdev);

	return phy_ethtool_gset(priv->phydev, cmd);
}

static int ftgmac100_set_settings(struct net_device *netdev,
				  struct ethtool_cmd *cmd)
{
	struct ftgmac100 *priv = netdev_priv(netdev);

	return phy_ethtool_sset(priv->phydev, cmd);
}

static void ftgmac100_get_ethtool_stats(struct net_device *netdev,
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

static const struct ethtool_ops ftgmac100_ethtool_ops = {
	.set_settings		= ftgmac100_set_settings,
	.get_settings		= ftgmac100_get_settings,
	.get_drvinfo		= ftgmac100_get_drvinfo,
	.get_link		= ethtool_op_get_link,
	.get_ethtool_stats = ftgmac100_get_ethtool_stats,
	.get_strings = get_strings,
	.get_sset_count = get_sset_count,
};

/******************************************************************************
 * interrupt handler
 *****************************************************************************/
static int ftgmac100_poll(struct napi_struct *napi, int budget);
static irqreturn_t ftgmac100_interrupt(int irq, void *dev_id)
{
	struct net_device *netdev = dev_id;
	struct ftgmac100 *priv = netdev_priv(netdev);

	/* Disable interrupts for polling */
	iowrite32(0, priv->base + FTGMAC100_OFFSET_IER);

	if (likely(netif_running(netdev))) {
		napi_schedule(&priv->napi);
	}

	return IRQ_HANDLED;
}

#ifdef CONFIG_FTGMAC100_STORM
static void calc_counter(struct ftgmac100 *priv, int mode)
{
	struct net_device *netdev = priv->netdev;
	unsigned int data, counter;

	priv->end_jiffies = get_gm_jiffies();
	if(((priv->end_jiffies - priv->begin_jiffies) < (priv->delay_time - 50)) && (mode)) {
		//printk("<t=%d,%ld>", priv->delay_time, priv->end_jiffies - priv->begin_jiffies);
		return;
	}

	data = ioread32(priv->base + FTGMAC100_OFFSET_RX_BC);

	if(data < priv->broadcast_num)
		counter = 0xFFFFFFFF - priv->broadcast_num + data;
	else
		counter = data - priv->broadcast_num;

    priv->broadcast_num = data;

    //printk("<b=%d,t=%ld>", counter, priv->end_jiffies - priv->begin_jiffies);
    priv->begin_jiffies = priv->end_jiffies;

    if(counter > RX_BROADCAST_THRESHOLD) {
    	priv->enable_rx = false;
    	priv->delay_time = RX_DELAY_TIME;
    	iowrite32(ioread32(priv->base + FTGMAC100_OFFSET_MACCR) & ~(FTGMAC100_MACCR_RX_BROADPKT | FTGMAC100_MACCR_RX_MULTIPKT | FTGMAC100_MACCR_HT_MULTI_EN), priv->base + FTGMAC100_OFFSET_MACCR);
	} else {
		priv->enable_rx = true;
		priv->delay_time = RX_DETECT_TIME;

		data = ioread32(priv->base + FTGMAC100_OFFSET_MACCR);
		if(data) {
			if (netdev->flags & IFF_ALLMULTI)
				data |= FTGMAC100_MACCR_RX_MULTIPKT;

			if (netdev->flags & IFF_BROADCAST)
				data |= FTGMAC100_MACCR_RX_BROADPKT;

			if (netdev->flags & IFF_MULTICAST)
				data |= FTGMAC100_MACCR_HT_MULTI_EN;

			iowrite32(data, priv->base + FTGMAC100_OFFSET_MACCR);
		}
	}
}

static void ftgmac100_timer(unsigned long d)
{
	struct ftgmac100 *priv = (struct ftgmac100 *)d;

    mod_timer(&priv->timer, jiffies + msecs_to_jiffies(priv->delay_time));
    calc_counter(priv, 0);
}
#endif	/* #ifdef CONFIG_FTGMAC100_STORM */

/******************************************************************************
 * struct napi_struct functions
 *****************************************************************************/
static int ftgmac100_poll(struct napi_struct *napi, int budget)
{
	struct ftgmac100 *priv = container_of(napi, struct ftgmac100, napi);
	struct net_device *netdev = priv->netdev;
	unsigned int status;
	bool completed = true;
	int rx = 0;

#ifdef CONFIG_FTGMAC100_STORM
	calc_counter(priv, 1);
#endif	/* #ifdef CONFIG_FTGMAC100_STORM */

	status = ioread32(priv->base + FTGMAC100_OFFSET_ISR);
	iowrite32(status, priv->base + FTGMAC100_OFFSET_ISR);

	if (status & (FTGMAC100_INT_RPKT_BUF | FTGMAC100_INT_NO_RXBUF)) {
		/*
		 * FTGMAC100_INT_RPKT_BUF:
		 *	RX DMA has received packets into RX buffer successfully
		 *
		 * FTGMAC100_INT_NO_RXBUF:
		 *	RX buffer unavailable
		 */
		bool retry;

		do {
			retry = ftgmac100_rx_packet(priv);
			if (retry)
				rx++;
		} while (retry && rx < budget);

		if (retry && rx == budget)
			completed = false;
	}

	if (status & (FTGMAC100_INT_XPKT_ETH | FTGMAC100_INT_XPKT_LOST)) {
		/*
		 * FTGMAC100_INT_XPKT_ETH:
		 *	packet transmitted to ethernet successfully
		 *
		 * FTGMAC100_INT_XPKT_LOST:
		 *	packet transmitted to ethernet lost due to late
		 *	collision or excessive collision
		 */
		ftgmac100_tx_complete(priv);
	}

	if (status & FTGMAC100_INT_AHB_ERR)
    	netdev_info(netdev, "AHB bus err\n");			
	
	if (status & FTGMAC100_INT_RPKT_LOST) {
		/* received packet lost due to RX FIFO full */
		netdev->stats.rx_fifo_errors++;
		netdev->stats.rx_errors++;
    	if (show_rx_dbg)
    		netdev_info(netdev, "MAC INT rx fifo full, lose\n");			
	}

	if (completed) {
		napi_complete(napi);
		/* enable all interrupts */
		iowrite32(INT_MASK_ALL_ENABLED, priv->base + FTGMAC100_OFFSET_IER);
	}

	return rx;
}

/******************************************************************************
 * struct net_device_ops functions
 *****************************************************************************/
static int ftgmac100_open(struct net_device *netdev)
{
	struct ftgmac100 *priv = netdev_priv(netdev);
	int err;

	err = ftgmac100_alloc_buffers(priv);
	if (err) {
		netdev_err(netdev, "failed to allocate buffers\n");
		goto err_alloc;
	}
	priv->process_rx_ring = ftgmac100_process_rx_jumbo_ring;

#ifdef CONFIG_FTGMAC100_STORM
	priv->broadcast_num = 0;
	priv->begin_jiffies = 0;
	priv->enable_rx = true;

	init_timer(&priv->timer);
	priv->timer.data     = (unsigned long)priv;
	priv->timer.function = ftgmac100_timer;
	priv->timer.expires  = jiffies + msecs_to_jiffies(RX_DETECT_TIME);
	add_timer(&priv->timer);
	priv->delay_time = RX_DETECT_TIME;
#endif
	err = request_irq(priv->irq, ftgmac100_interrupt, 0, netdev->name, netdev);
	if (err) {
		netdev_err(netdev, "failed to request irq %d\n", priv->irq);
		goto err_irq;
	}

	err = ftgmac100_reset_hw(priv);
	if (err)
		goto err_hw;

	ftgmac100_init_hw(priv);
	ftgmac100_start_hw(priv, priv->old_speed, priv->old_duplex);

	phy_start(priv->phydev);
	napi_enable(&priv->napi);
	netif_start_queue(netdev);

	/* enable all interrupts */
	iowrite32(INT_MASK_ALL_ENABLED, priv->base + FTGMAC100_OFFSET_IER);
	return 0;

err_hw:
	free_irq(priv->irq, netdev);
err_irq:
	ftgmac100_free_buffers(priv);
err_alloc:
	return err;
}

static int ftgmac100_stop(struct net_device *netdev)
{
	struct ftgmac100 *priv = netdev_priv(netdev);

#ifdef CONFIG_FTGMAC100_STORM
	del_timer_sync(&priv->timer);
#endif

	/* disable all interrupts */
	iowrite32(0, priv->base + FTGMAC100_OFFSET_IER);

	netif_stop_queue(netdev);
	napi_disable(&priv->napi);
	
	phy_stop(priv->phydev);

	ftgmac100_stop_hw(priv);
	ftgmac100_reset_hw(priv);
	free_irq(priv->irq, netdev);
	ftgmac100_free_buffers(priv);

	return 0;
}

static int ftgmac100_hard_start_xmit(struct sk_buff *skb,
				     struct net_device *netdev)
{
	struct ftgmac100 *priv = netdev_priv(netdev);

	if (unlikely(skb->len > (netdev->mtu + 18))) {

		netdev->stats.tx_dropped++;
		dev_kfree_skb(skb);
		return NETDEV_TX_OK;
	}

	return ftgmac100_xmit(priv, skb);
}


static void ftgmac100_set_rx_mode(struct net_device *netdev)
{
	struct ftgmac100 *priv = netdev_priv(netdev);
	unsigned int maccr = ioread32(priv->base + FTGMAC100_OFFSET_MACCR);

	if (show_rx_dbg)
		printk("set rx mode = 0x%x\n", netdev->flags);

	/* clear filter flags */
	maccr &= ~(FTGMAC100_MACCR_RX_BROADPKT | FTGMAC100_MACCR_RX_MULTIPKT | FTGMAC100_MACCR_HT_MULTI_EN | FTGMAC100_MACCR_RX_ALL);
	if (netdev->flags & IFF_PROMISC) {
		maccr |= FTGMAC100_MACCR_RX_ALL;
	}

	if (netdev->flags & IFF_ALLMULTI) {
		maccr |= FTGMAC100_MACCR_RX_MULTIPKT;
	}

	if (netdev->flags & IFF_BROADCAST) {
		maccr |= FTGMAC100_MACCR_RX_BROADPKT;
	}

	if (netdev->flags & IFF_MULTICAST) {
		unsigned int maht0 = 0x0;
		unsigned int maht1 = 0x0;
		struct netdev_hw_addr *ha;

		maccr |= FTGMAC100_MACCR_HT_MULTI_EN;
		/* clear hash table*/
		iowrite32(maht0, priv->base + FTGMAC100_OFFSET_MAHT0);
		iowrite32(maht1, priv->base + FTGMAC100_OFFSET_MAHT1);
		/* set hash table */
		netdev_for_each_mc_addr(ha, netdev) {
			unsigned char ht_index = ftgmac100_get_ht_index(ha->addr);
			if (ht_index < 32)
				maht0 |= (1 << ht_index);
			else
				maht1 |= (1 << (ht_index - 32));
		}
		iowrite32(maht0, priv->base + FTGMAC100_OFFSET_MAHT0);
		iowrite32(maht1, priv->base + FTGMAC100_OFFSET_MAHT1);
	}
	iowrite32(maccr, priv->base + FTGMAC100_OFFSET_MACCR);
}

static int ftgmac100_mac_addr(struct net_device *netdev, void *p)
{
	struct ftgmac100 *priv = netdev_priv(netdev);
	int ret;
	ret = eth_mac_addr(netdev, p);
	if (unlikely(ret))
		return ret;

	ftgmac100_set_mac(priv, p);
	return ret;
}

/* optional */
static int ftgmac100_do_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd)
{
	struct ftgmac100 *priv = netdev_priv(netdev);

	return phy_mii_ioctl(priv->phydev, ifr, cmd);
}


/*
 * tx fifo size and rx fifo size selection (only 3 bits):
 * 0x0: 2K
 * 0x1: 4K
 * 0x2: 8K
 * 0x3: 16K
 * 0x4: 32K
 * else: reserved
 */
static int ftgmac100_set_tfifo_size(struct ftgmac100 *priv, int size_sel)
{
	int max_sel = FTGMAC100_FEAR_TFIFO_RSIZE(ioread32(priv->base + FTGMAC100_OFFSET_FEAR));
	int tpafcr = ioread32(priv->base + FTGMAC100_OFFSET_TPAFCR);;

	tpafcr &= ~FTGMAC100_TPAFCR_TFIFO_SIZE(0x7);

	tpafcr |= FTGMAC100_TPAFCR_TFIFO_SIZE(max_sel);
	iowrite32(tpafcr, priv->base + FTGMAC100_OFFSET_TPAFCR);
	return max_sel;
}

static int ftgmac100_set_rfifo_size(struct ftgmac100 *priv, int size_sel)
{
	int max_sel = FTGMAC100_FEAR_RFIFO_RSIZE(ioread32(priv->base + FTGMAC100_OFFSET_FEAR));
	int tpafcr = ioread32(priv->base + FTGMAC100_OFFSET_TPAFCR);;

	tpafcr &= ~FTGMAC100_TPAFCR_RFIFO_SIZE(0x7);

	tpafcr |= FTGMAC100_TPAFCR_RFIFO_SIZE(max_sel);
	iowrite32(tpafcr, priv->base + FTGMAC100_OFFSET_TPAFCR);
	return max_sel;
}

static int ftgmac100_fifo_size_to_sel(int fifo_size)
{
	if (fifo_size <= (SZ_2K - SZ_32))
		return 0;
	else if (fifo_size <= (SZ_4K -SZ_32))
		return 1;
	else if (fifo_size <= (SZ_8K -SZ_32))
		return 2;
	else if (fifo_size <= (SZ_16K -SZ_32))
		return 3;
	else if (fifo_size <= (SZ_32K -SZ_32))
		return 4;
	else
		return 5;
}

static unsigned int ftgmac100_fifo_size_sel_to_size(int sel)
{
	switch (sel) {
	case 0:
		return (SZ_2K - SZ_32);
	case 1:
		return (SZ_4K - SZ_32);
	case 2:
		return (SZ_8K -SZ_32);
	case 3:
		return (SZ_16K -SZ_32);
	case 4:
		return (SZ_32K -SZ_32);
	default:
		return 0;
	}
}

static int ftgmac100_change_mtu(struct net_device *netdev, int new_mtu)
{
	unsigned int tx_size, rx_size, tx_sel, rx_sel, max_size;
	struct ftgmac100 *priv = netdev_priv(netdev);
	int sel = ftgmac100_fifo_size_to_sel(new_mtu);

	tx_sel = ftgmac100_set_tfifo_size(priv, sel);
	rx_sel = ftgmac100_set_rfifo_size(priv, sel);

	priv->tx_fifo_sel = tx_sel;
	priv->rx_fifo_sel = rx_sel;

	tx_size = ftgmac100_fifo_size_sel_to_size(tx_sel);
	rx_size = ftgmac100_fifo_size_sel_to_size(rx_sel);

	max_size = (tx_size > rx_size) ? tx_size : rx_size;

	netdev->mtu = (new_mtu < max_size) ? new_mtu : (max_size < MAX_PKT_SIZE) ? max_size : MAX_PKT_SIZE;

	return 0;
}

static const struct net_device_ops ftgmac100_netdev_ops = {
	.ndo_open		= ftgmac100_open,
	.ndo_stop		= ftgmac100_stop,
	.ndo_start_xmit		= ftgmac100_hard_start_xmit,
	.ndo_set_rx_mode	= ftgmac100_set_rx_mode,
	.ndo_set_mac_address	= ftgmac100_mac_addr,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_do_ioctl		= ftgmac100_do_ioctl,
	.ndo_change_mtu		= ftgmac100_change_mtu,
};

/******************************************************************************
 * struct platform_driver functions
 *****************************************************************************/
static int ftgmac100_probe(struct platform_device *pdev)
{
	struct resource *res;
	int irq;
	struct net_device *netdev;
	struct ftgmac100 *priv;
	int err;
	int i;
	int tx_num_queues = 1, rx_num_queues = 1;
	char *ptr = NULL, *ptr_end;
	u8 addr[6];
	char ethaddr[18];
	
	if (!pdev)
		return -ENODEV;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENXIO;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	if (num_online_cpus() > 1) {
		tx_num_queues = TX_NUM_QUEUES;
		rx_num_queues = RX_NUM_QUEUES;
	}

	/* setup net_device */
	netdev = alloc_etherdev_mqs(sizeof(*priv), tx_num_queues, rx_num_queues);
	if (!netdev) {
		err = -ENOMEM;
		goto err_alloc_etherdev;
	}

	SET_NETDEV_DEV(netdev, &pdev->dev);

	SET_ETHTOOL_OPS(netdev, &ftgmac100_ethtool_ops);
	netdev->netdev_ops = &ftgmac100_netdev_ops;
	netdev->features = NETIF_F_IP_CSUM | NETIF_F_GRO | NETIF_F_GSO;

	platform_set_drvdata(pdev, netdev);

	/* setup private data */
	priv = netdev_priv(netdev);
	priv->tx_num_queues = tx_num_queues;
	priv->rx_num_queues = rx_num_queues;
	priv->netdev = netdev;
	priv->dev = &pdev->dev;

	/* initialize NAPI */
	netif_napi_add(netdev, &priv->napi, ftgmac100_poll, 64);

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
	priv->mii_bus = mdiobus_alloc();
	if (!priv->mii_bus) {
		err = -EIO;
		goto err_alloc_mdiobus;
	}

	if(irq == MAC_FTGMAC100_0_IRQ){
		priv->mii_bus->name = "ftgmac100-0-mdio";
		snprintf(priv->mii_bus->id, MII_BUS_ID_SIZE, DRV_NAME);
	}else{
		priv->mii_bus->name = "ftgmac100-1-mdio";
		snprintf(priv->mii_bus->id, MII_BUS_ID_SIZE, DRV_1_NAME);	
	}

	priv->mii_bus->priv = netdev;
	priv->mii_bus->read = ftgmac100_mdiobus_read;
	priv->mii_bus->write = ftgmac100_mdiobus_write;
	priv->mii_bus->reset = ftgmac100_mdiobus_reset;
	priv->mii_bus->irq = priv->phy_irq;

	for (i = 0; i < PHY_MAX_ADDR; i++)
		priv->mii_bus->irq[i] = PHY_POLL;

	err = mdiobus_register(priv->mii_bus);
	if (err) {
		dev_err(&pdev->dev, "Cannot register MDIO bus!\n");
		goto err_register_mdiobus;
	}

	err = ftgmac100_mii_probe(priv);
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

	//netdev_info(netdev, "%d tx queue used (max: %d)", priv->tx_num_queues, MAX_TX_QUEUES);
	//netdev_info(netdev, "%d rx queue used (max: %d)", priv->rx_num_queues, MAX_RX_QUEUES);
	//netdev_info(netdev, "irq %d, mapped at %p\n", priv->irq, priv->base);

	if (!is_valid_ether_addr(netdev->dev_addr)) {
        ethaddr[17] = '\0';	    
	    	if(irq == MAC_FTGMAC100_0_IRQ){	        
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
    		random_ether_addr(netdev->dev_addr);
    		//netdev_info(netdev, "generated random MAC address %pM\n", netdev->dev_addr);
	    }
    }

	priv->tx_fifo_sel = 0x0;
	priv->rx_fifo_sel = 0x0;
	spin_lock_init(&priv->mac_lock);	

	ftgmac_reg_debug->data = netdev;
/*
	err = ftgmac100_alloc_buffers(priv);
	if (err) {
		netdev_err(netdev, "failed to allocate buffers\n");
		goto err_alloc_etherdev;
	}
	priv->process_rx_ring = ftgmac100_process_rx_jumbo_ring;
*/
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
	platform_set_drvdata(pdev, NULL);
	free_netdev(netdev);
err_alloc_etherdev:
	return err;
}

static int __exit ftgmac100_remove(struct platform_device *pdev)
{
	struct net_device *netdev;
	struct ftgmac100 *priv;

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
	platform_set_drvdata(pdev, NULL);
	free_netdev(netdev);
	return 0;
}

#if defined(CONFIG_FTGMAC100_DRIVER_0_MASTER) || defined(CONFIG_FTGMAC100_DRIVER_0_SLAVE)
static struct platform_driver ftgmac100_driver = {
	.probe		= ftgmac100_probe,
	.remove		= __exit_p(ftgmac100_remove),
	.driver		= {
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
	},
};
#endif
#if defined(CONFIG_FTGMAC100_DRIVER_1_MASTER) || defined(CONFIG_FTGMAC100_DRIVER_1_SLAVE)
static struct platform_driver ftgmac100_1_driver = {
	.probe		= ftgmac100_probe,
	.remove		= __exit_p(ftgmac100_remove),
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
    struct ftgmac100 *priv = netdev_priv(netdev);
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
static int __init ftgmac100_init(void)
{
    int result = -ENODEV;
  
    command_line = kmalloc(COMMAND_LINE_SIZE, GFP_ATOMIC);
    memcpy(command_line, boot_command_line, COMMAND_LINE_SIZE);

	printk("GMAC version %s, queue number tx = %d, rx = %d\n", DRV_VERSION, TX_QUEUE_ENTRIES, RX_QUEUE_ENTRIES);

    ftgmac_proc = create_proc_entry("ftgmac", S_IFDIR | S_IRUGO | S_IXUGO, NULL);
    if (ftgmac_proc == NULL) {
        printk("Error to create driver ftgmac proc\n");
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

#ifdef Generate_PHY_Clock
    if ((result = gpio_request(GPIO_pin, "gpio1")) != 0) {
        printk("gpio request fail\n");
        return result;
    }
#endif

#ifdef CONFIG_PLATFORM_GM8210
#ifdef CONFIG_FTGMAC100_DRIVER_0_MASTER
    if(!CPU_detect())
        register_mac0 = 1;
#endif
#ifdef CONFIG_FTGMAC100_DRIVER_0_SLAVE
    if(CPU_detect())
        register_mac0 = 1;
#endif
#ifdef CONFIG_FTGMAC100_DRIVER_1_MASTER
    if(!CPU_detect())
        register_mac1 = 1;
#endif
#ifdef CONFIG_FTGMAC100_DRIVER_1_SLAVE
    if(CPU_detect())
        register_mac1 = 1;
#endif
#endif

#if defined(CONFIG_PLATFORM_GM8287) || defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
    register_mac0 = 1;
#endif

	if(register_mac0){
	    platform_device_register(&ftgmac100_0_device);
	    
    	result = platform_driver_register(&ftgmac100_driver);
    	if (result < 0) {
    	    platform_device_unregister(&ftgmac100_0_device);
	        goto exit_init;
	    }
	}
#if defined(CONFIG_FTGMAC100_DRIVER_1_MASTER) || defined(CONFIG_FTGMAC100_DRIVER_1_SLAVE)	
	if(register_mac1){
	    platform_device_register(&ftgmac100_1_device);
	    
    	result = platform_driver_register(&ftgmac100_1_driver);
    	if (result < 0) {
    	    platform_device_unregister(&ftgmac100_1_device);
	        goto exit_init;
	    }
	}
#endif

exit_init:		    	
	return result;
}

static void __exit ftgmac100_exit(void)
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
        
#ifdef Generate_PHY_Clock
	gpio_free(GPIO_pin);
#endif

    if(register_mac0) {
    	platform_driver_unregister(&ftgmac100_driver);
    	platform_device_unregister(&ftgmac100_0_device);
    }
#if defined(CONFIG_FTGMAC100_DRIVER_1_MASTER) || defined(CONFIG_FTGMAC100_DRIVER_1_SLAVE)
    if(register_mac1) {
    	platform_driver_unregister(&ftgmac100_1_driver);
    	platform_device_unregister(&ftgmac100_1_device);
    }
#endif    
}

module_init(ftgmac100_init);
module_exit(ftgmac100_exit);

MODULE_AUTHOR("Po-Yu Chuang <ratbert@faraday-tech.com>");
MODULE_DESCRIPTION("FTGMAC100 driver");
MODULE_LICENSE("GPL");
