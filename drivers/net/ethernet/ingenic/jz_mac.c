/*
 * jz On-Chip MAC Driver
 *
 *  Copyright (C) 2010 - 2011  Ingenic Semiconductor Inc.
 *
 * Licensed under the GPL-2 or later.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/crc32.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/mii.h>
#include <linux/phy.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/dma-direction.h>
#include <linux/ethtool.h>
#include <linux/skbuff.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <soc/gpio.h>
#include <soc/irq.h>
#include <linux/clk.h>
#include <soc/cpm.h>
#include <soc/base.h>

#include "jz_mac.h"

#define IOCTL_DUMP_REGISTER  SIOCDEVPRIVATE+1
#define IOCTL_DUMP_DESC      SIOCDEVPRIVATE+2
#ifndef IRQ_ETH
#define IRQ_ETH		IRQ_GMAC
#endif
int debug_enable = 0;
module_param(debug_enable, int, 0644);

synopGMACdevice *gmacdev;

/* driver version: 1.0 */
#define JZMAC_DRV_NAME		"jz_mac"
#define JZMAC_DRV_VERSION	"1.0"
#define JZMAC_DRV_DESC		"JZ on-chip Ethernet MAC driver"

MODULE_AUTHOR("Lutts Wolf <slcao@ingenic.cn>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(JZMAC_DRV_DESC);
MODULE_ALIAS("platform:jz_mac");

typedef enum {
	/* MAC PHY Speed */
	MAC_1000M,
	MAC_100M,
	MAC_10M,

	/* MAC PHY Interface */
	MAC_GMII,
	MAC_MII,
	MAC_RGMII,
	MAC_RMII,

	/* MAC PHY Soft reset */
	MAC_SREST,
} mac_clock_control;

/* MDC  = 2.5 MHz */
#ifndef JZMAC_MDC_CLK
#define JZMAC_MDC_CLK 2500000
#endif

#define COPYBREAK_DEFAULT 256
static unsigned int copybreak __read_mostly = COPYBREAK_DEFAULT;
module_param(copybreak, uint, 0644);
MODULE_PARM_DESC(copybreak,
		 "Maximum size of packet that is copied to a new buffer on receive");

#define PKT_BUF_SZ (1580 + NET_IP_ALIGN)
#define MAX_TIMEOUT_CNT	5000
#define JZMAC_RX_BUFFER_WRITE	16	/* Must be power of 2 */

/* Generate the bit field mask from msb to lsb */
#define BITS_H2L(msb, lsb)  ((0xFFFFFFFF >> (32-((msb)-(lsb)+1))) << (lsb))
#define MPHYC_ENA_GMII		(1 << 31)
#define MPHYC_MAC_PHYINTF_MASK	BITS_H2L(2, 0)
#define MPHYC_MAC_PHYINTF_MII	(0 << 0)
#define MPHYC_MAC_PHYINTF_RGMII	(1 << 0)
#define MPHYC_MAC_PHYINTF_RMII	(4 << 0)

static int jz_mdiobus_read(struct mii_bus *bus, int phy_addr, int regnum);
static int jz_mdio_phy_read(struct net_device *dev, int phy_id, int location);
static int jz_mdiobus_write(struct mii_bus *bus, int phy_addr, int regnum, u16 value);
static int jz_mdio_phy_write(struct net_device *dev, int phy_id, int location, int value);

void set_mac_phy_clk(mac_clock_control mac_control)
{
	unsigned int mphy_value = cpm_inl(CPM_MPHYC);

	mphy_value &= ~((1 << 31) | (7 << 0));

	switch (mac_control){
	case MAC_GMII:
		mphy_value |= (1 << 31) | (0 << 0);
		break;

	case MAC_RGMII:
		mphy_value |= (1 << 31) | (1 << 0);
		break;

	case MAC_MII:
		mphy_value |= (0 << 0);
		break;

	case MAC_RMII:
		mphy_value |= (4 << 0);
		break;

	case MAC_1000M:
	case MAC_100M:
	case MAC_10M:
		return;

	default:
		printk("WARMING: can NOT set MAC mode %d\n", mac_control);
		return;
	}

	cpm_outl(mphy_value, CPM_MPHYC);

	return;
}

static inline unsigned char str2hexnum(unsigned char c)
{
        if (c >= '0' && c <= '9')
                return c - '0';
        if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;

        return 0; /* foo */
}

static inline void str2eaddr(unsigned char *ea, unsigned char *str)
{
        int i;

        for (i = 0; i < 6; i++) {
                unsigned char num;

                if ((*str == '.') || (*str == ':'))
                        str++;
                num  = str2hexnum(*str++) << 4;
                num |= str2hexnum(*str++);
                ea[i] = num;
        }
}

static int bootargs_ethaddr = 0;
static unsigned char ethaddr_hex[6];

static int __init ethernet_addr_setup(char *str)
{
	if (!str) {
	        printk("ethaddr not set in command line\n");
		return -1;
	}
	bootargs_ethaddr = 1;
	str2eaddr(ethaddr_hex, str);

	return 0;
}

__setup("ethaddr=", ethernet_addr_setup);

/* debug routines */
__attribute__((__unused__)) static void jzmac_dump_pkt_data(unsigned char *data, int len) {
	int i = 0;
        printk("\t0x0000: ");
        for (i = 0; i < len; i++) {
                printk("%02x", data[i]);

                if (i % 2)
                        printk(" ");

                if ( (i != 0) && ((i % 16) == 15) )
                        printk("\n\t0x%04x: ", i+1);
        }
	printk("\n");
}

__attribute__((__unused__)) static void jzmac_dump_skb_data(struct sk_buff *skb) {
        printk("\n\n===================================\n");
        printk("head = 0x%08x, data = 0x%08x, tail = 0x%08x, end = 0x%08x\n",
               (unsigned int)(skb->head), (unsigned int)(skb->data),
               (unsigned int)(skb->tail), (unsigned int)(skb->end));
        printk("len = %d\n", skb->len);
	jzmac_dump_pkt_data(skb->data, skb->len);
        printk("\n=====================================\n");
}

struct jzmac_reg
{
	u32    addr;
	char   * name;
};

static struct jzmac_reg mac[] =
{
	{ 0x0000, "                  Config" },
	{ 0x0004, "            Frame Filter" },
	{ 0x0008, "             MAC HT High" },
	{ 0x000C, "              MAC HT Low" },
	{ 0x0010, "               GMII Addr" },
	{ 0x0014, "               GMII Data" },
	{ 0x0018, "            Flow Control" },
	{ 0x001C, "                VLAN Tag" },
	{ 0x0020, "            GMAC Version" },
	{ 0x0024, "            GMAC Debug  " },
	{ 0x0028, "Remote Wake-Up Frame Filter" },
	{ 0x002C, "  PMT Control and Status" },
	{ 0x0030, "  LPI Control and status" },
	{ 0x0034, "      LPI Timers Control" },
	{ 0x0038, "        Interrupt Status" },
	{ 0x003c, "        Interrupt Mask" },
	{ 0x0040, "          MAC Addr0 High" },
	{ 0x0044, "           MAC Addr0 Low" },
	{ 0x0048, "          MAC Addr1 High" },
	{ 0x004c, "           MAC Addr1 Low" },
	{ 0x0100, "           MMC Ctrl Reg " },
	{ 0x010c, "        MMC Intr Msk(rx)" },
	{ 0x0110, "        MMC Intr Msk(tx)" },
	{ 0x0200, "    MMC Intr Msk(rx ipc)" },
	{ 0x0738, "          AVMAC Ctrl Reg" },
	{ 0x00D8, "           RGMII C/S Reg" },
	{ 0, 0 }
};
static struct jzmac_reg dma0[] =
{
	{ 0x0000, "[CH0] CSR0   Bus Mode" },
	{ 0x0004, "[CH0] CSR1   TxPlDmnd" },
	{ 0x0008, "[CH0] CSR2   RxPlDmnd" },
	{ 0x000C, "[CH0] CSR3    Rx Base" },
	{ 0x0010, "[CH0] CSR4    Tx Base" },
	{ 0x0014, "[CH0] CSR5     Status" },
	{ 0x0018, "[CH0] CSR6    Control" },
	{ 0x001C, "[CH0] CSR7 Int Enable" },
	{ 0x0020, "[CH0] CSR8 Missed Fr." },
	{ 0x0024, "[CH0] Recv Intr Wd.Tm." },
	{ 0x0028, "[CH0] AXI Bus Mode   " },
	{ 0x002c, "[CH0] AHB or AXI Status" },
	{ 0x0048, "[CH0] CSR18 Tx Desc  " },
	{ 0x004C, "[CH0] CSR19 Rx Desc  " },
	{ 0x0050, "[CH0] CSR20 Tx Buffer" },
	{ 0x0054, "[CH0] CSR21 Rx Buffer" },
	{ 0x0058, "CSR22 HWCFG          " },
	{ 0, 0 }
};

__attribute__((__unused__)) static void jzmac_dump_dma_regs(const char *func, int line)
{
	struct jzmac_reg *reg = dma0;

	printk("======================DMA Regs start===================\n");
	while(reg->name) {
		printk("===>%s:\t0x%08x\n", reg->name, synopGMACReadReg((u32 *)gmacdev->DmaBase,reg->addr));
		reg++;
	}
	printk("======================DMA Regs end===================\n");
}

__attribute__((__unused__)) static void jzmac_dump_mac_regs(const char *func, int line)
{
	struct jzmac_reg *reg = mac;

	printk("======================MAC Regs start===================\n");
	while(reg->name) {
		printk("===>%s:\t0x%08x\n", reg->name, synopGMACReadReg((u32 *)gmacdev->MacBase,reg->addr));
		reg++;
	}
	printk("======================MAC Regs end===================\n");
}

__attribute__((__unused__)) static void jzmac_dump_all_regs(const char *func, int line) {
	jzmac_dump_dma_regs(func, line);
	jzmac_dump_mac_regs(func, line);
}

__attribute__((__unused__)) static void jzmac_dump_dma_buffer_info(struct jzmac_buffer *buffer_info) {
	printk("\tbuffer_info(%p):\n", buffer_info);
	printk("\t\tskb = %p\n", buffer_info->skb);
	printk("\t\tdma = 0x%08x\n", buffer_info->dma);
	printk("\t\tlen = %u\n", buffer_info->length);
	printk("\t\ttrans = %d\n", buffer_info->transfering);
	printk("\t\tinvalid = %d\n", buffer_info->invalid);
}

__attribute__((__unused__)) static void jzmac_dump_dma_desc(DmaDesc *desc) {
	printk("\tdma desc(%p):\n", desc);
	printk("\t\tstatus = 0x%08x\n", desc->status);
	printk("\t\tbuffer1 = 0x%08x\n", desc->buffer1);
	printk("\t\tlength = %u\n", desc->length);
}

__attribute__((__unused__)) static void jzmac_dump_dma_desc2(DmaDesc *desc, struct jzmac_buffer *buffer_info) {
	printk("desc: %p, status: 0x%08x buf1: 0x%08x dma: 0x%08x len: %u bi: %p skb: %p trans: %d inv: %d\n",
			desc, desc->status, desc->buffer1, buffer_info->dma, desc->length,
			buffer_info, buffer_info->skb, buffer_info->transfering, buffer_info->invalid);
}

__attribute__((__unused__)) static void jzmac_dump_rx_desc(struct jz_mac_local *lp) {
	int i = 0;
	printk("\n===================rx====================\n");
	printk("count = %d, next_to_use = %d next_to_clean = %d\n",
	       lp->rx_ring.count, lp->rx_ring.next_to_use, lp->rx_ring.next_to_clean);
	for (i = 0; i < JZMAC_RX_DESC_COUNT; i++) {
		DmaDesc *desc = lp->rx_ring.desc + i;
		struct jzmac_buffer *b = lp->rx_ring.buffer_info + i;

#if 0
		printk("desc %d:\n", i);
		jzmac_dump_dma_desc(desc);
		jzmac_dump_dma_buffer_info(b);
#endif
		jzmac_dump_dma_desc2(desc, b);
	}
	printk("\n=========================================\n");
}

__attribute__((__unused__)) static void jzmac_dump_tx_desc(struct jz_mac_local *lp) {
	int i = 0;
	printk("\n===================tx====================\n");
	printk("count = %d, next_to_use = %d next_to_clean = %d\n",
	       lp->tx_ring.count, lp->tx_ring.next_to_use, lp->tx_ring.next_to_clean);
	for (i = 0; i < JZMAC_TX_DESC_COUNT; i++) {
		DmaDesc *desc = lp->tx_ring.desc + i;
		struct jzmac_buffer *b = lp->tx_ring.buffer_info + i;

#if 0
		printk("desc %d:\n", i);
		jzmac_dump_dma_desc(desc);
		jzmac_dump_dma_buffer_info(b);
#endif
		jzmac_dump_dma_desc2(desc, b);
	}
	printk("\n=========================================\n");
}

__attribute__((__unused__)) static void jzmac_dump_all_desc(struct jz_mac_local *lp) {
	jzmac_dump_rx_desc(lp);
	jzmac_dump_tx_desc(lp);
}

__attribute__((__unused__)) static int get_rx_index_by_desc(struct jz_mac_local *lp, DmaDesc *desc) {
	int i = 0;

	for (i = 0; i < JZMAC_RX_DESC_COUNT; i++) {
		if ( (lp->rx_ring.desc + i) == desc)
			return i;
	}

	BUG_ON(i == JZMAC_RX_DESC_COUNT);
	return -1;
}

__attribute__((__unused__)) static void jzmac_phy_dump(struct jz_mac_local *lp) {
	u16 phy[] = {0, 1, 4, 5, 6, 9, 16, 17, 18, 20, 21, 24, 0x1c};

	u16 data[sizeof(phy) / sizeof(u16)];
	int i;

	printk("\n-------->PHY dump: %08X\n", lp->phydev->phy_id);
	for (i = 0; i < sizeof(phy) / sizeof(u16); i++)
		data[i] = lp->mii_bus->read(lp->mii_bus, lp->phydev->addr, phy[i]);

	for (i = 0; i < sizeof(phy) / sizeof(u16); i++)
		printk("PHY reg%d, value %04X\n", phy[i], data[i]);
}

static void jzmac_restart_rx_dma(struct jz_mac_local *lp) {
	synopGMAC_enable_dma_rx(gmacdev);
}

static void jzmac_alloc_rx_buffers(struct jz_mac_local *lp, int cleaned_count,
				   int restart_dma) {
	int i = 0;
	struct jzmac_buffer *buffer_info;
	struct sk_buff *skb;
	struct jzmac_rx_ring *rx_ring = &lp->rx_ring;
	DmaDesc *rx_desc;
	DmaDesc *first_desc;
	int first;

	first = rx_ring->next_to_use;
	i = rx_ring->next_to_use;
	rx_desc = JZMAC_RX_DESC(*rx_ring, i);
	first_desc = rx_desc;
	buffer_info = &rx_ring->buffer_info[i];

	while (cleaned_count--) {
		skb = buffer_info->skb;
		if (skb) {
			skb_trim(skb, 0);
			goto map_skb;
		}

		skb = netdev_alloc_skb_ip_align(lp->netdev, lp->netdev->mtu + ETHERNET_HEADER + ETHERNET_CRC);
		if (unlikely(!skb)) {
			/* Better luck next round */
			lp->alloc_rx_buff_failed++;
			break;
		}

		//skb_reserve(skb, NET_IP_ALIGN);

		buffer_info->skb = skb;
		buffer_info->length = skb_tailroom(skb);
	map_skb:
		buffer_info->dma = dma_map_single(&lp->netdev->dev,
						  skb->data, skb_tailroom(skb),
						  DMA_FROM_DEVICE);
		if (dma_mapping_error(&lp->netdev->dev, buffer_info->dma)) {
			dev_err(&lp->netdev->dev, "Rx DMA map failed\n");
			lp->alloc_rx_buff_failed++;
			break;
		}

		rx_desc->length |= ((skb_tailroom(skb) <<DescSize1Shift) & DescSize1Mask) |
			((0 << DescSize2Shift) & DescSize2Mask);
		rx_desc->buffer1 = cpu_to_le32(buffer_info->dma);

		rx_desc->extstatus = 0;
		rx_desc->reserved1 = 0;
		rx_desc->timestamplow = 0;
		rx_desc->timestamphigh = 0;


		/* clr invalid first, then start transfer */
		buffer_info->invalid = 0;

		/* start transfer */
		rx_desc->status = DescOwnByDma;

		/* next */
		if (unlikely(++i == rx_ring->count))
			i = 0;

		wmb();

		rx_desc = JZMAC_RX_DESC(*rx_ring, i);
		buffer_info = &rx_ring->buffer_info[i];
	}


	if (likely(rx_ring->next_to_use != i)) {
		rx_ring->next_to_use = i;
		/* sanity check: ensure next_to_use is not used */
		rx_desc->buffer1 = cpu_to_le32(0);
		rx_desc->status &= ~DescOwnByDma;
		buffer_info->invalid = 1;
		wmb();

		/* assure that if there's any buffer space, dma is enabled */
		if (likely(restart_dma))
			jzmac_restart_rx_dma(lp);
	}
}

static int desc_list_init_rx(struct jz_mac_local *lp) {
	int i;
	int size;

	/* rx init */
	lp->rx_ring.count = JZMAC_RX_DESC_COUNT;

	size = lp->rx_ring.count * sizeof(struct jzmac_buffer);
	lp->rx_ring.buffer_info = vmalloc(size);
	if (!lp->rx_ring.buffer_info) {
		printk(KERN_ERR "Unable to allocate memory for the receive descriptor ring\n");
		return -ENOMEM;
	}
	memset(lp->rx_ring.buffer_info, 0, size);

	lp->rx_ring.desc = dma_alloc_noncoherent(&lp->netdev->dev,
						 lp->rx_ring.count * sizeof(DmaDesc),
						 &lp->rx_ring.dma, GFP_KERNEL);

	if (lp->rx_ring.desc == NULL) {
		vfree(lp->rx_ring.buffer_info);
		lp->rx_ring.buffer_info = NULL;
		return -ENOMEM;
	}

	dma_cache_wback_inv((unsigned long)lp->rx_ring.desc,
			    lp->rx_ring.count * sizeof(DmaDesc));

	/* we always use uncached address for descriptors */
	lp->rx_ring.desc = (DmaDesc *)CKSEG1ADDR(lp->rx_ring.desc);

	for (i = 0; i < JZMAC_RX_DESC_COUNT; i++) {
		DmaDesc *desc = lp->rx_ring.desc + i;

		synopGMAC_rx_desc_init_ring(desc, i == (JZMAC_RX_DESC_COUNT - 1));
	}

	lp->rx_ring.next_to_use = lp->rx_ring.next_to_clean = 0;
	jzmac_alloc_rx_buffers(lp, JZMAC_DESC_UNUSED(&lp->rx_ring), 0);

	synopGMACWriteReg((u32 *)gmacdev->DmaBase,DmaRxBaseAddr, lp->rx_ring.dma);

	return 0;
}

static void desc_list_free_rx(struct jz_mac_local *lp) {
	struct jzmac_buffer *b;
	int i = 0;

	if (lp->rx_ring.desc)
		dma_free_noncoherent(&lp->netdev->dev,
				     lp->rx_ring.count * sizeof(DmaDesc),
				     (void *)CKSEG0ADDR(lp->rx_ring.desc),
				     lp->rx_ring.dma);

	if (lp->rx_ring.buffer_info) {
		b = lp->rx_ring.buffer_info;

		for(i = 0; i < JZMAC_RX_DESC_COUNT; i++) {
			if (b[i].skb) {
				if (b[i].dma) {
					dma_unmap_single(&lp->netdev->dev, b[i].dma,
							 b[i].length, DMA_FROM_DEVICE);
					b[i].dma = 0;
				}

				dev_kfree_skb_any(b[i].skb);
				b[i].skb = NULL;
				b[i].time_stamp = 0;
			}
		}
	}
	vfree(lp->rx_ring.buffer_info);
	lp->rx_ring.buffer_info = NULL;
	lp->rx_ring.next_to_use = lp->rx_ring.next_to_clean = 0;
}

/* must be called from interrupt handler */
static void jzmac_take_desc_ownership_rx(struct jz_mac_local *lp) {
	int i = 0;

	/* must called with interrupts disabled */
	BUG_ON(synopGMAC_get_interrupt_mask(gmacdev));

	for (i = 0; i < JZMAC_RX_DESC_COUNT; i++) {
		DmaDesc *desc = lp->rx_ring.desc + i;
		struct jzmac_buffer *b = lp->rx_ring.buffer_info + i;

		if (!b->invalid) {
			synopGMAC_take_desc_ownership(desc);
		}
	}
}

/* MUST ensure that rx is stopped ans rx_dma is disabled */
static void desc_list_reinit_rx(struct jz_mac_local *lp) {
	DmaDesc *desc;
	int i = 0;

	//TODO: BUG_ON(!jzmac_rx_dma_stopped());

	for (i = 0; i < JZMAC_RX_DESC_COUNT; i++) {
		desc = lp->rx_ring.desc + i;

		/* owned by DMA, can fill data */
		synopGMAC_rx_desc_init_ring(desc, i == (JZMAC_RX_DESC_COUNT - 1));
	}

	lp->rx_ring.next_to_use = lp->rx_ring.next_to_clean = 0;
	jzmac_alloc_rx_buffers(lp, JZMAC_DESC_UNUSED(&lp->rx_ring), 0);
}

static int desc_list_init_tx(struct jz_mac_local *lp) {
	int i;
	int size;

	/* tx init */
	lp->tx_ring.count = JZMAC_TX_DESC_COUNT;

	size = lp->tx_ring.count * sizeof(struct jzmac_buffer);
	lp->tx_ring.buffer_info = vmalloc(size);
	if (!lp->tx_ring.buffer_info) {
		printk(KERN_ERR"Unable to allocate memory for the receive descriptor ring\n");
		return -ENOMEM;
	}
	memset(lp->tx_ring.buffer_info, 0, size);

	lp->tx_ring.desc = dma_alloc_noncoherent(&lp->netdev->dev,
						 lp->tx_ring.count * sizeof(DmaDesc),
						 &lp->tx_ring.dma, GFP_KERNEL);

	if (lp->tx_ring.desc == NULL) {
		vfree(lp->tx_ring.buffer_info);
		lp->tx_ring.buffer_info = NULL;
		return -ENOMEM;
	}

	dma_cache_wback_inv((unsigned long)lp->tx_ring.desc,
			    lp->tx_ring.count * sizeof(DmaDesc));

	/* we always use uncached address for descriptors */
	lp->tx_ring.desc = (DmaDesc *)CKSEG1ADDR(lp->tx_ring.desc);

	for (i = 0; i < JZMAC_TX_DESC_COUNT; i++) {
		DmaDesc *desc = lp->tx_ring.desc + i;

		synopGMAC_tx_desc_init_ring(desc, i == (JZMAC_TX_DESC_COUNT - 1));
	}

	lp->tx_ring.next_to_use = lp->tx_ring.next_to_clean = 0;
	synopGMACWriteReg((u32 *)gmacdev->DmaBase,DmaTxBaseAddr,(u32)lp->tx_ring.dma);
	return 0;
}

__attribute__((__unused__)) static int get_tx_index_by_desc(struct jz_mac_local *lp, DmaDesc *desc) {
	int i = 0;

	for (i = 0; i < JZMAC_TX_DESC_COUNT; i++) {
		if ( (lp->tx_ring.desc + i) == desc)
			return i;
	}

	BUG_ON(i == JZMAC_TX_DESC_COUNT);
	return -1;
}

static void jzmac_unmap_and_free_tx_resource(struct jz_mac_local *lp,
					     struct jzmac_buffer *buffer_info)
{
	buffer_info->transfering = 0;

	if (buffer_info->skb) {
		if (buffer_info->dma) {
			if (buffer_info->mapped_as_page)
				dma_unmap_page(&lp->netdev->dev, buffer_info->dma,
					       buffer_info->length, DMA_TO_DEVICE);
			else
				dma_unmap_single(&lp->netdev->dev, buffer_info->dma,
						 buffer_info->length, DMA_TO_DEVICE);
			buffer_info->dma = 0;
		}
		dev_kfree_skb_any(buffer_info->skb);
		buffer_info->skb = NULL;
	}
	buffer_info->time_stamp = 0;
}

static void desc_list_free_tx(struct jz_mac_local *lp) {
	struct jzmac_buffer *b;
	int i = 0;

	if (lp->tx_ring.desc)
		dma_free_noncoherent(&lp->netdev->dev,
				     lp->tx_ring.count * sizeof(DmaDesc),
				     (void *)CKSEG0ADDR(lp->tx_ring.desc),
				     lp->tx_ring.dma);

	if (lp->tx_ring.buffer_info) {
		b = lp->tx_ring.buffer_info;

		for (i = 0; i < JZMAC_TX_DESC_COUNT; i++) {
			// panic("===>ahha, testing! please do not goes here(%s:%d)!!!\n", __func__, __LINE__);
			jzmac_unmap_and_free_tx_resource(lp, b + i);
		}

	}
	vfree(lp->tx_ring.buffer_info);
	lp->tx_ring.buffer_info = NULL;
	lp->tx_ring.next_to_use = lp->tx_ring.next_to_clean = 0;
}

/* must called in interrupt handler */
static void jzmac_take_desc_ownership_tx(struct jz_mac_local *lp) {
	int i = 0;

	/* must called with interrupts disabled */
	BUG_ON(synopGMAC_get_interrupt_mask(gmacdev));

	for (i = 0; i < JZMAC_TX_DESC_COUNT; i++) {
		DmaDesc *desc = lp->tx_ring.desc + i;
		struct jzmac_buffer *b = lp->tx_ring.buffer_info + i;

		if (!b->invalid) {
			synopGMAC_take_desc_ownership(desc);
		}
	}
}

/* must assure that tx transfer are stopped and tx_dma is disabled */
static void desc_list_reinit_tx(struct jz_mac_local *lp) {
	int i = 0;
	DmaDesc *desc;
	struct jzmac_buffer *b;

	//TODO: BUG_ON(!jzmac_tx_dma_stopped());

	for (i = 0; i < JZMAC_TX_DESC_COUNT; i++) {
		desc = lp->tx_ring.desc + i;
		b = lp->tx_ring.buffer_info + i;

		/* owned by CPU, no valid data */
		synopGMAC_tx_desc_init_ring(desc, i == (JZMAC_TX_DESC_COUNT - 1));

		jzmac_unmap_and_free_tx_resource(lp, b);
	}

	lp->tx_ring.next_to_use = lp->tx_ring.next_to_clean = 0;
}

static void desc_list_free(struct jz_mac_local *lp)
{
	desc_list_free_rx(lp);
	desc_list_free_tx(lp);
}

static void desc_list_reinit(struct jz_mac_local *lp) {
	desc_list_reinit_rx(lp);
	desc_list_reinit_tx(lp);
	//jzmac_dump_all_desc(lp);
}

static int desc_list_init(struct jz_mac_local *lp)
{
	if (desc_list_init_rx(lp) < 0)
		goto init_error;

	if (desc_list_init_tx(lp) < 0)
		goto init_error;


	//jzmac_dump_all_desc(lp);
	return 0;

 init_error:
	desc_list_free(lp);
	printk(KERN_ERR JZMAC_DRV_NAME ": kmalloc failed\n");
	return -ENOMEM;
}


/*---PHY CONTROL AND CONFIGURATION-----------------------------------------*/
static void jz_mac_adjust_link(struct net_device *dev)
{
	struct jz_mac_local *lp = netdev_priv(dev);
	struct phy_device *phydev = lp->phydev;
	unsigned long flags;
	int new_state = 0;

	//printk("===>ajust link, old_duplex = %d, old_speed = %d, old_link = %d\n",
	//       lp->old_duplex, lp->old_speed, lp->old_link);

	spin_lock_irqsave(&lp->link_lock, flags);
	if (phydev->link) {
		/* Now we make sure that we can be in full duplex mode.
		 * If not, we operate in half-duplex mode. */
		if (phydev->duplex != lp->old_duplex) {
			new_state = 1;

			if (phydev->duplex) {
				synopGMAC_set_full_duplex(gmacdev);
				//synopGMAC_rx_own_enable(gmacdev);
				//synopGMAC_set_Inter_Frame_Gap(gmacdev, GmacInterFrameGap7);
			} else {
				synopGMAC_set_half_duplex(gmacdev);
				//synopGMAC_rx_own_disable(gmacdev);
				//synopGMAC_set_Inter_Frame_Gap(gmacdev, GmacInterFrameGap4);
			}

			lp->old_duplex = phydev->duplex;
		}

		if (phydev->speed != lp->old_speed) {
			switch (phydev->speed) {
			case 1000:
				synopGMAC_select_speed1000(gmacdev);
				break;
			case 100:
				synopGMAC_select_speed100(gmacdev);
				break;
			case 10:
				synopGMAC_select_speed10(gmacdev);
				break;
			default:
				printk(KERN_ERR "GMAC PHY speed NOT match!\n");
				synopGMAC_select_speed100(gmacdev);
			}

			new_state = 1;
			lp->old_speed = phydev->speed;
		}

		if (!lp->old_link) {
			new_state = 1;
			lp->old_link = 1;
			netif_carrier_on(dev);
			//mod_timer(&lp->watchdog_timer, jiffies + 1);
		}
	} else if (lp->old_link) {
		new_state = 1;
		lp->old_link = 0;
		lp->old_speed = 0;
		lp->old_duplex = -1;
		netif_carrier_off(dev);
	}

	if (new_state)
		phy_print_status(phydev);

	//printk("===>ajust link, new_duplex = %d, new_speed = %d, new_link = %d\n",
	//       lp->old_duplex, lp->old_speed, lp->old_link);

	spin_unlock_irqrestore(&lp->link_lock, flags);

}

static int mii_probe(struct net_device *dev)
{
	struct jz_mac_local *lp = netdev_priv(dev);
	struct phy_device *phydev = NULL;
	//int clkdiv[8] = { 4, 4, 6, 8, 10, 14, 20, 28 };
	int i;

	/* search for connect PHY device */
	for (i = 0; i < PHY_MAX_ADDR; i++) {
		struct phy_device *const tmp_phydev = lp->mii_bus->phy_map[i];

		if (!tmp_phydev)
			continue; /* no PHY here... */

		phydev = tmp_phydev;
		break; /* found it */
	}

	/* now we are supposed to have a proper phydev, to attach to... */
	if (!phydev) {
		printk(KERN_INFO "%s: Don't found any phy device at all\n",
		       dev->name);
		return -ENODEV;
	}

#if defined(CONFIG_jz_MAC_RMII)
	phydev = phy_connect(dev, dev_name(&phydev->dev), &jz_mac_adjust_link,
			      PHY_INTERFACE_MODE_RMII);
#elif defined(CONFIG_jz_MAC_RGMII)
	phydev = phy_connect(dev, dev_name(&phydev->dev), &jz_mac_adjust_link,
			      PHY_INTERFACE_MODE_RGMII);
#elif defined(CONFIG_jz_MAC_GMII)
	phydev = phy_connect(dev, dev_name(&phydev->dev), &jz_mac_adjust_link,
			      PHY_INTERFACE_MODE_GMII);
#else
	phydev = phy_connect(dev, dev_name(&phydev->dev), &jz_mac_adjust_link,
			      PHY_INTERFACE_MODE_MII);
#endif

	if (IS_ERR(phydev)) {
		printk(KERN_ERR "%s: Could not attach to PHY\n", dev->name);
		return PTR_ERR(phydev);
	}

	/* mask with MAC supported features */
	phydev->supported &= (SUPPORTED_10baseT_Half
			      | SUPPORTED_10baseT_Full
			      | SUPPORTED_100baseT_Half
			      | SUPPORTED_100baseT_Full
			      | SUPPORTED_1000baseT_Half
			      | SUPPORTED_1000baseT_Full
			      | SUPPORTED_Autoneg
			      | SUPPORTED_Pause | SUPPORTED_Asym_Pause
			      | SUPPORTED_MII
			      | SUPPORTED_TP);

	phydev->advertising = phydev->supported;

	lp->old_link = 0;
	lp->old_speed = 0;
	lp->old_duplex = -1;
	lp->phydev = phydev;

	//jzmac_phy_dump(lp);

	return 0;
}

/**
 * jzmac_update_stats - Update the board statistics counters
 * @lp: board private structure
 **/

void jzmac_update_stats(struct jz_mac_local *lp)
{
	if ((lp->old_link == 0) || (lp->old_speed == 0) || (lp->old_duplex == -1))
		return;

#if 0
	//spin_lock_irqsave(&lp->stats_lock, flags);
	//spin_lock(&lp->stats_lock);

	/* Fill out the OS statistics structure */
	lp->net_stats.multicast = REG32(MAC_STAT_RMCA);
	lp->net_stats.collisions = REG32(MAC_STAT_RBCA);

	/* Rx Errors */

	/* RLEC on some newer hardware can be incorrect so build
	 * our own version based on RUC and ROC */
	lp->net_stats.rx_errors = lp->stats.rxerrc +
		lp->stats.crcerrs + lp->stats.algnerrc +
		lp->stats.ruc + lp->stats.roc +
		lp->stats.cexterr;
	lp->net_stats.rx_length_errors = REG32(MAC_STAT_RFLR);
	lp->net_stats.rx_crc_errors = REG32(MAC_STAT_RFCS);
	lp->net_stats.rx_frame_errors = REG32(MAC_STAT_RALN);
	lp->net_stats.rx_missed_errors = lp->stats.mpc;

	/* Tx Errors */
	lp->stats.txerrc = lp->stats.ecol + lp->stats.latecol;
	lp->net_stats.tx_errors = lp->stats.txerrc;
	lp->net_stats.tx_aborted_errors = lp->stats.ecol;
	lp->net_stats.tx_window_errors = lp->stats.latecol;
	lp->net_stats.tx_carrier_errors = lp->stats.tncrs;
	if (hw->bad_tx_carr_stats_fd &&
	    lp->link_duplex == FULL_DUPLEX) {
		lp->net_stats.tx_carrier_errors = 0;
		lp->stats.tncrs = 0;
	}

	/* Tx Dropped needs to be maintained elsewhere */

	/* Phy Stats */
	if (hw->media_type == e1000_media_type_copper) {
		if ((lp->link_speed == SPEED_1000) &&
		    (!e1000_read_phy_reg(hw, PHY_1000T_STATUS, &phy_tmp))) {
			phy_tmp &= PHY_IDLE_ERROR_COUNT_MASK;
			lp->phy_stats.idle_errors += phy_tmp;
		}

		if ((hw->mac_type <= e1000_82546) &&
		    (hw->phy_type == e1000_phy_m88) &&
		    !e1000_read_phy_reg(hw, M88E1000_RX_ERR_CNTR, &phy_tmp))
			lp->phy_stats.receive_errors += phy_tmp;
	}

	/* Management Stats */
	if (hw->has_smbus) {
		lp->stats.mgptc += er32(MGTPTC);
		lp->stats.mgprc += er32(MGTPRC);
		lp->stats.mgpdc += er32(MGTPDC);
	}

	//spin_unlock_irqrestore(&lp->stats_lock, flags);
	//spin_unlock(&lp->stats_lock);
#endif
}

/**
 * jzmac_watchdog - Timer Call-back
 * @data: pointer to lp cast into an unsigned long
 **/
static void jzmac_watchdog(unsigned long data) {
	struct jz_mac_local *lp = (struct jz_mac_local *)data;
	//struct net_device *netdev = lp->netdev;

	jzmac_update_stats(lp);

	mod_timer(&lp->watchdog_timer, round_jiffies(jiffies + 5 * HZ));
}

static int __jzmac_maybe_stop_tx(struct net_device *netdev, int size)
{
	struct jz_mac_local *lp = netdev_priv(netdev);
	struct jzmac_tx_ring *tx_ring = &lp->tx_ring;

	netif_stop_queue(netdev);
	smp_mb();

	/* We need to check again in a case another CPU has just
	 * made room available. */
	if (likely(JZMAC_DESC_UNUSED(tx_ring) < size))
		return -EBUSY;

	/* A reprieve! */
	netif_start_queue(netdev);
	++lp->restart_queue;
	return 0;
}

static int jzmac_maybe_stop_tx(struct net_device *netdev,
                               struct jzmac_tx_ring *tx_ring, int size)
{
	if (likely(JZMAC_DESC_UNUSED(tx_ring) >= size))
		return 0;
	return __jzmac_maybe_stop_tx(netdev, size);
}

#if 0
static int jzmac_tx_map(struct jz_mac_local *lp,
			struct jzmac_tx_ring *tx_ring,
			struct sk_buff *skb)
{
	struct jzmac_buffer *buffer_info;
	unsigned int i;

	i = tx_ring->next_to_use;

	if (skb_dma_map(&lp->netdev->dev, skb, DMA_TO_DEVICE)) {
		dev_err(&lp->netdev->dev, "TX DMA map failed\n");
		return 0;
	}

	buffer_info = &tx_ring->buffer_info[i];

	//buffer_info->length = (skb->len < ETH_ZLEN) ? ETH_ZLEN : skb->len;
	buffer_info->length = skb->len;
	buffer_info->dma = skb_shinfo(skb)->dma_head;
	buffer_info->time_stamp = jiffies;
	buffer_info->skb = skb;
	return 1;
}
#else
static int jzmac_tx_map(struct jz_mac_local *lp,
			struct jzmac_tx_ring *tx_ring,
			struct sk_buff *skb)
{
	struct net_device *pdev = lp->netdev;
	struct jzmac_buffer *buffer_info;
	unsigned int len = skb_headlen(skb);
	unsigned int offset = 0, count = 0, i;
	unsigned int f, segs;
	unsigned int nr_frags = skb_shinfo(skb)->nr_frags;

	i = tx_ring->next_to_use;

	buffer_info = &tx_ring->buffer_info[i];
	buffer_info->length = len;
	buffer_info->time_stamp = jiffies;
	buffer_info->dma = dma_map_single(&pdev->dev,
					  skb->data + offset,
					  len, DMA_TO_DEVICE);
	buffer_info->mapped_as_page = false;
	if (dma_mapping_error(&pdev->dev, buffer_info->dma))
		goto dma_error;
	segs = skb_shinfo(skb)->gso_segs ? : 1;
	tx_ring->buffer_info[i].skb = skb;
	tx_ring->buffer_info[i].segs = segs;
	tx_ring->buffer_info[i].transfering = 1;
	count++;

	for (f = 0; f < nr_frags; f++) {
		struct skb_frag_struct *frag;

		//struct page *p;
		frag = &skb_shinfo(skb)->frags[f];
		len = frag->size;
		offset = frag->page_offset;
		i++;
		if (i == tx_ring->count)
			i = 0;

		buffer_info = &tx_ring->buffer_info[i];
		buffer_info->length = len;
		buffer_info->time_stamp = jiffies;
		buffer_info->dma = dma_map_page(&pdev->dev, frag->page.p,
						offset,
						len,
						DMA_TO_DEVICE);

		buffer_info->mapped_as_page = true;
		if (dma_mapping_error(&pdev->dev, buffer_info->dma))
			goto dma_unwind;
		segs = skb_shinfo(skb)->gso_segs ? : 1;
		tx_ring->buffer_info[i].skb = skb;
		tx_ring->buffer_info[i].segs = segs;
		tx_ring->buffer_info[i].transfering = 1;

		count++;
	}


	return count;
dma_unwind:
	dev_err(&pdev->dev, "Tx DMA map failed at dma_unwind\n");
	while(count-- > 0) {
		i--;
		if (i == 0) {
			i = tx_ring->count;
		}
		buffer_info = &tx_ring->buffer_info[i];
		if (buffer_info->dma) {
			if (buffer_info->mapped_as_page)
				dma_unmap_page(&pdev->dev, buffer_info->dma,
					       buffer_info->length, DMA_TO_DEVICE);
			else
				dma_unmap_single(&pdev->dev, buffer_info->dma,
						 buffer_info->length, DMA_TO_DEVICE);
			buffer_info->dma = 0;
		}
		if (buffer_info->skb) {
			dev_kfree_skb_any(buffer_info->skb);
			buffer_info->skb = NULL;
		}
		buffer_info->time_stamp = 0;
	}

dma_error:
	dev_err(&pdev->dev, "Tx DMA map failed at dma_error\n");
	buffer_info->dma = 0;
	return -ENOMEM;
}
#endif

static void jzmac_restart_tx_dma(struct jz_mac_local *lp) {
	/* TODO: clear error status bits if any */
	u32 data;

	data = synopGMACReadReg((u32 *)gmacdev->DmaBase, DmaControl);
	if (data & DmaTxStart) {
		synopGMAC_resume_dma_tx(gmacdev);
	} else {
		synopGMAC_enable_dma_tx(gmacdev);
	}

	/* ensure irq is enabled */
	synopGMAC_enable_interrupt(gmacdev,DmaIntEnable);
}

static void jzmac_tx_queue(struct jz_mac_local *lp,
			   struct jzmac_tx_ring *tx_ring)
{
	DmaDesc *tx_desc = NULL;
	struct jzmac_buffer *buffer_info;
	unsigned int i;

	i = tx_ring->next_to_use;

	buffer_info = &tx_ring->buffer_info[i];
	tx_desc = JZMAC_TX_DESC(*tx_ring, i);

	tx_desc->length |= (((cpu_to_le32(buffer_info->length) <<DescSize1Shift) & DescSize1Mask)
			| ((0 <<DescSize2Shift) & DescSize2Mask));  // buffer2 is not used
	tx_desc->buffer1 = cpu_to_le32(buffer_info->dma);
	tx_desc->buffer2 = 0;
	tx_desc->status |=  (DescTxFirst | DescTxLast | DescTxIntEnable); //ENH_DESC
	tx_desc->status |= DescOwnByDma;//ENH_DESC

	wmb();

	buffer_info->transfering = 1;

	if (unlikely(++i == tx_ring->count)) i = 0;
	tx_ring->next_to_use = i;

	wmb();
	jzmac_restart_tx_dma(lp);
}


static int jz_mac_hard_start_xmit(struct sk_buff *skb,
				      struct net_device *netdev)
{
	struct jz_mac_local *lp = netdev_priv(netdev);
	struct jzmac_tx_ring *tx_ring;
	unsigned int first;
	int count = 1;

	tx_ring = &lp->tx_ring;

	if (unlikely(skb->len <= 0)) {
		printk(JZMAC_DRV_NAME ": WARNING: skb->len < 0\n");
		dev_kfree_skb_any(skb);
		return NETDEV_TX_OK;
	}

	/* this can be cacelled for we should support it*/
	/*
	if (skb_shinfo(skb)->nr_frags) {
		printk(JZMAC_DRV_NAME ": WARNING: fragment packet do not handled!!!\n");
		dev_kfree_skb_any(skb);
		return NETDEV_TX_OK;
	}
	*/

	/* need: count + 2 desc gap to keep tail from touching
	 * head, otherwise try next time */
	if (unlikely(jzmac_maybe_stop_tx(netdev, tx_ring, count + 2)))
		return NETDEV_TX_BUSY;

	first = tx_ring->next_to_use;
	count = jzmac_tx_map(lp, tx_ring, skb);

	if (likely(count)) {
		jzmac_tx_queue(lp, tx_ring);
		/* Make sure there is space in the ring for the next send.*/
		jzmac_maybe_stop_tx(netdev, tx_ring, MAX_SKB_FRAGS + 2);
		//jzmac_dump_all_regs(__func__, __LINE__);
	} else {
		dev_kfree_skb_any(skb);
		tx_ring->buffer_info[first].time_stamp = 0;
		tx_ring->next_to_use = first;
	}

	//jzmac_dump_all_regs(__func__, __LINE__);
	//jzmac_dump_all_desc(lp);

	return NETDEV_TX_OK;
}

static bool jzmac_clean_tx_irq(struct jz_mac_local *lp) {
	struct net_device *netdev = lp->netdev;
	DmaDesc *desc;
	struct jzmac_buffer *buffer_info;
	struct jzmac_tx_ring *tx_ring = &lp->tx_ring;
	unsigned int i;
	unsigned int count = 0;
	unsigned int total_tx_bytes=0, total_tx_packets=0;

	i = tx_ring->next_to_clean;
	desc = JZMAC_TX_DESC(*tx_ring, i);
	buffer_info = &tx_ring->buffer_info[i];

	while (buffer_info->transfering &&
	       !synopGMAC_is_desc_owned_by_dma(desc) &&
	       (count < tx_ring->count)) {

		buffer_info->transfering = 0;
		count++;

		if(synopGMAC_is_desc_valid(desc->status)){
			total_tx_packets ++;
			total_tx_bytes += buffer_info->length;
		}

		jzmac_unmap_and_free_tx_resource(lp, buffer_info);
		synopGMAC_tx_desc_init_ring(desc, i == (tx_ring->count - 1));

		i++;
		if (unlikely(i == tx_ring->count)) i = 0;

		desc = JZMAC_TX_DESC(*tx_ring, i);
		buffer_info = &tx_ring->buffer_info[i];
	}

	tx_ring->next_to_clean = i;
	//printk("===>tx: %d pkts cleaned\n", count);

#define TX_WAKE_THRESHOLD 16
	if (unlikely(count && netif_carrier_ok(netdev) &&
		     JZMAC_DESC_UNUSED(tx_ring) >= TX_WAKE_THRESHOLD)) {
		/* Make sure that anybody stopping the queue after this
		 * sees the new next_to_clean.
		 */
		smp_mb();
		if (netif_queue_stopped(netdev)) {
			netif_wake_queue(netdev);
			++lp->restart_queue;
		}
	}

	lp->total_tx_bytes += total_tx_bytes;
	lp->total_tx_packets += total_tx_packets;
	lp->net_stats.tx_bytes += total_tx_bytes;
	lp->net_stats.tx_packets += total_tx_packets;

	return (count < tx_ring->count);
}

static bool jzmac_clean_rx_irq(struct jz_mac_local *lp,
			       int *work_done, int work_to_do) {

	struct net_device *netdev = lp->netdev;
	struct jzmac_rx_ring *rx_ring = &lp->rx_ring;
	DmaDesc *rx_desc, *next_rxd;
	struct jzmac_buffer *buffer_info, *next_buffer;
	u32 length;
	unsigned int i;
	unsigned int rx_desc_i;
	int cleaned_count = 0;
	bool cleaned = false;
	unsigned int total_rx_bytes=0, total_rx_packets=0;

	i = rx_ring->next_to_clean;
	rx_desc = JZMAC_RX_DESC(*rx_ring, i);
	buffer_info = &rx_ring->buffer_info[i];

	/* except the slot not used, if transfer done, buffer_info->invalid is always 0 */
	while ((!synopGMAC_is_desc_owned_by_dma(rx_desc)) && (!buffer_info->invalid)) {
		struct sk_buff *skb;

		if (*work_done >= work_to_do)
			break;
		(*work_done)++;
		rmb();  /* read descriptor and rx_buffer_info after status DD */

		buffer_info->invalid = 1;
		skb = buffer_info->skb;
		buffer_info->skb = NULL; /* cleaned */

		if(synopGMAC_is_rx_desc_valid(rx_desc->status))
			prefetch(skb->data - NET_IP_ALIGN);

		rx_desc_i = i;
		if (++i == rx_ring->count) i = 0;

		next_rxd = JZMAC_RX_DESC(*rx_ring, i);
		prefetch(next_rxd);

		next_buffer = &rx_ring->buffer_info[i];

		cleaned = true;
		cleaned_count++;

		if(!synopGMAC_is_rx_desc_valid(rx_desc->status)) {
			/* save the skb in buffer_info as good */
			buffer_info->skb = skb;
			//printk("====>invalid pkt\n");
			goto invalid_pkt;
		}

		length = synopGMAC_get_rx_desc_frame_length(rx_desc->status);
		synopGMAC_rx_desc_init_ring(rx_desc, rx_desc_i == (rx_ring->count - 1));
#if 0
		printk("============================================\n");
		jzmac_dump_pkt_data((unsigned char *)CKSEG1ADDR(buffer_info->dma),
				    length - 4);
		printk("============================================\n");
#endif
		dma_unmap_single(&lp->netdev->dev,
				 buffer_info->dma, buffer_info->length,
				 DMA_FROM_DEVICE);
		buffer_info->dma = 0;


		/* adjust length to remove Ethernet CRC, this must be
		 * done after the TBI_ACCEPT workaround above */
		length -= 4;

		/* probably a little skewed due to removing CRC */
		total_rx_bytes += length;
		total_rx_packets++;

		/* code added for copybreak, this should improve
		 * performance for small packets with large amounts
		 * of reassembly being done in the stack */
		if (length < copybreak) {
			struct sk_buff *new_skb =
				netdev_alloc_skb_ip_align(netdev, length);

			if (new_skb) {
				skb_copy_to_linear_data_offset(new_skb,
							       -NET_IP_ALIGN,
							       (skb->data -
							        NET_IP_ALIGN),
							       (length +
							        NET_IP_ALIGN));
				/* save the skb in buffer_info as good */
				buffer_info->skb = skb;
				skb = new_skb;
			}
			/* else just continue with the old one */
		}

		/* end copybreak code */
		skb_put(skb, length);
		skb->protocol = eth_type_trans(skb, netdev);

		//jzmac_dump_skb_data(skb);
		netif_receive_skb(skb);
		//netdev->last_rx = jiffies;

	invalid_pkt:
		rx_desc->status = 0;
		/* return some buffers to hardware, one at a time is too slow */
		if (unlikely(cleaned_count >= JZMAC_RX_BUFFER_WRITE)) {
			jzmac_alloc_rx_buffers(lp, cleaned_count, 1);
			cleaned_count = 0;
		}

		/* use prefetched values */
		rx_desc = next_rxd;
		buffer_info = next_buffer;
	}

	rx_ring->next_to_clean = i;

	cleaned_count = JZMAC_DESC_UNUSED(rx_ring);
	if (cleaned_count)
		jzmac_alloc_rx_buffers(lp, cleaned_count, 1);

	lp->total_rx_packets += total_rx_packets;
	lp->total_rx_bytes += total_rx_bytes;
	lp->net_stats.rx_bytes += total_rx_bytes;
	lp->net_stats.rx_packets += total_rx_packets;

	return cleaned;
}

/**
 * jzmac_clean - NAPI Rx polling callback
 **/
static int jzmac_clean(struct napi_struct *napi, int budget) {
	struct jz_mac_local *lp = container_of(napi, struct jz_mac_local, napi);
	int tx_cleaned = 0;
	int work_done = 0;

	spin_lock(&lp->napi_poll_lock);

	tx_cleaned = jzmac_clean_tx_irq(lp);

	jzmac_clean_rx_irq(lp, &work_done, budget);

	if (!tx_cleaned)
		work_done = budget;

	//printk("===>workdone = %d, budget = %d\n", work_done, budget);
	//jzmac_dump_all_regs(__func__, __LINE__);

	/* If budget not fully consumed, exit the polling mode */
	if (work_done < budget) {
		napi_complete(napi);
		synopGMAC_enable_interrupt(gmacdev,DmaIntEnable);
	}

	spin_unlock(&lp->napi_poll_lock);
	return work_done;
}



/* interrupt routine to handle rx and error signal */
static irqreturn_t jz_mac_interrupt(int irq, void *data)
{
	struct net_device *netdev = data;
	struct jz_mac_local *lp = netdev_priv(netdev);
	u32 interrupt,dma_status_reg;
	u32 mac_interrupt_status;
	u32 rgmii_interrupt_status;

	/* Read the Dma interrupt status to know whether the interrupt got generated by our device or not*/
	dma_status_reg = synopGMACReadReg((u32 *)gmacdev->DmaBase, DmaStatus);
	mac_interrupt_status = synopGMACReadReg((u32 *)gmacdev->MacBase, GmacInterruptStatus);

	//printk("===>enter %s:%d DmaStatus = 0x%08x\n", __func__, __LINE__, dma_status_reg);
	if(dma_status_reg == 0)
		return IRQ_NONE;

	synopGMAC_disable_interrupt_all(gmacdev);


	if(dma_status_reg & GmacPmtIntr){
		dev_err(&netdev->dev, "%s:: Interrupt due to PMT module\n",__FUNCTION__);
		//synopGMAC_linux_powerup_mac(gmacdev);
	}

	if(dma_status_reg & GmacMmcIntr){
		dev_err(&netdev->dev, "%s:: Interrupt due to MMC module\n",__FUNCTION__);
		dev_err(&netdev->dev, "%s:: synopGMAC_rx_int_status = %08x\n",
			 __FUNCTION__,synopGMAC_read_mmc_rx_int_status(gmacdev));
		dev_err(&netdev->dev, "%s:: synopGMAC_tx_int_status = %08x\n",
			 __FUNCTION__,synopGMAC_read_mmc_tx_int_status(gmacdev));
	}

	if(dma_status_reg & GmacLineIntfIntr){
		if (mac_interrupt_status & GmacRgmiiIntSts) {
			rgmii_interrupt_status = synopGMACReadReg((u32 *)gmacdev->MacBase, GmacRGMIIControl);
       			dev_dbg(&netdev->dev, "%s:Interrupts to RGMII/SGMII: 0x%08x\n",__FUNCTION__,rgmii_interrupt_status);
		} else
			dev_err(&netdev->dev, "%s:: Interrupt due to GMAC LINE module\n",__FUNCTION__);
	}

	/* Now lets handle the DMA interrupts*/
        interrupt = synopGMAC_get_interrupt_type(gmacdev);
       	dev_dbg(&netdev->dev, "%s:Interrupts to be handled: 0x%08x\n",__FUNCTION__,interrupt);


	if(interrupt & synopGMACDmaError){
		dev_err(&netdev->dev, "%s::Fatal Bus Error Inetrrupt Seen\n",__FUNCTION__);
#if 0   /* do nothing here, let tx_timeout to handle it */
		synopGMAC_disable_dma_tx(gmacdev);
                synopGMAC_disable_dma_rx(gmacdev);

		synopGMAC_take_desc_ownership_tx(gmacdev);
		synopGMAC_take_desc_ownership_rx(gmacdev);

		synopGMAC_init_tx_rx_desc_queue(gmacdev);

		synopGMAC_reset(gmacdev);//reset the DMA engine and the GMAC ip

		synopGMAC_set_mac_addr(gmacdev,GmacAddr0High,GmacAddr0Low, netdev->dev_addr);
		synopGMAC_dma_bus_mode_init(gmacdev,DmaFixedBurstEnable| DmaBurstLength8 | DmaDescriptorSkip2 );
	 	synopGMAC_dma_control_init(gmacdev,DmaStoreAndForward);
		synopGMAC_init_rx_desc_base(gmacdev);
		synopGMAC_init_tx_desc_base(gmacdev);
		synopGMAC_mac_init(gmacdev);
		synopGMAC_enable_dma_rx(gmacdev);
		synopGMAC_enable_dma_tx(gmacdev);
#endif
        }

	if(interrupt & synopGMACDmaRxAbnormal){
	        dev_dbg(&netdev->dev, "%s::Abnormal Rx Interrupt Seen\n",__FUNCTION__);
		 synopGMAC_resume_dma_rx(gmacdev);
	}

	if(interrupt & synopGMACDmaRxStopped){
		// Receiver gone in to stopped state
		// why Rx Stopped? no enough descriptor? but why no enough descriptor need cause an interrupt?
		// we have no enough descriptor because we can't handle packets that fast, isn't it?
		// So I think if DmaRxStopped Interrupt can disabled
        	dev_info(&netdev->dev, "%s::Receiver stopped seeing Rx interrupts\n",__FUNCTION__);

		synopGMAC_enable_dma_rx(gmacdev);
	}

	if(interrupt & synopGMACDmaTxAbnormal){
		dev_dbg(&netdev->dev, "%s::Abnormal Tx Interrupt Seen\n",__FUNCTION__);
	}

	if(interrupt & synopGMACDmaTxStopped){
		dev_err(&netdev->dev, "%s::Transmitter stopped sending the packets\n",__FUNCTION__);
		synopGMAC_disable_dma_tx(gmacdev);
		jzmac_dump_all_desc(lp);
		jzmac_take_desc_ownership_tx(lp);
		synopGMAC_enable_dma_tx(gmacdev);
	}

	if (likely(napi_schedule_prep(&lp->napi))) {
		lp->total_tx_bytes = 0;
		lp->total_tx_packets = 0;
		lp->total_rx_bytes = 0;
		lp->total_rx_packets = 0;
		dev_dbg(&netdev->dev, "enter %s:%d, call __napi_schedule\n", __func__, __LINE__);
		__napi_schedule(&lp->napi);
	} else {
		/* this really should not happen! if it does it is basically a
		 * bug, but not a hard error, so enable ints and continue */
		synopGMAC_enable_interrupt(gmacdev,DmaIntEnable);
	}

	return IRQ_HANDLED;
}

/* MAC control and configuration */

static void jz_mac_stop_activity(struct jz_mac_local *lp) {
	synopGMAC_disable_interrupt_all(gmacdev);

	// Disable the Dma in rx path
        synopGMAC_disable_dma_rx(gmacdev);
	jzmac_take_desc_ownership_rx(lp);
	//msleep(100);	      // Allow any pending buffer to be read by host
	//synopGMAC_rx_disable(gmacdev);

	synopGMAC_disable_dma_tx(gmacdev);
	jzmac_take_desc_ownership_tx(lp);
	//msleep(100);	      // allow any pending transmission to complete
	// Disable the Mac for both tx and rx
	//synopGMAC_tx_disable(gmacdev);
}

static void jz_mac_disable(struct jz_mac_local *lp) {
	/* First ensure that the upper network stack is stopped */
	/* can be netif_tx_disable when NETIF_F_LLTX is removed */
	netif_stop_queue(lp->netdev); /* tx */
	napi_disable(&lp->napi); /* rx */

	jz_mac_stop_activity(lp);
	del_timer_sync(&lp->watchdog_timer);

	spin_lock(&lp->napi_poll_lock);
	desc_list_reinit(lp);
	spin_unlock(&lp->napi_poll_lock);
}

static void jzmac_init(void) {
	/*
	 * disable the watchdog and gab to receive frames up to 16384 bytes
	 * to adjust IP protocol
	 */
	synopGMAC_wd_disable(gmacdev);
	synopGMAC_jab_disable(gmacdev);

	/* cancel to set Frame Burst Enable for now we use duplex mode */
	//synopGMAC_frame_burst_enable(gmacdev);

	/* set jumbo to allow to receive Jumbo frames of 9,018 bytes */
	//synopGMAC_jumbo_frame_disable(gmacdev);
	synopGMAC_jumbo_frame_enable(gmacdev);

	/* for we try to use duplex */
	synopGMAC_rx_own_disable(gmacdev);
	synopGMAC_loopback_off(gmacdev);
	/* default to full duplex, I think this will be the common case */
	synopGMAC_set_full_duplex(gmacdev);
	/* here retry enabe may useless */
	synopGMAC_retry_enable(gmacdev);
	synopGMAC_pad_crc_strip_disable(gmacdev);
	synopGMAC_back_off_limit(gmacdev,GmacBackoffLimit0);
	synopGMAC_deferral_check_disable(gmacdev);
	synopGMAC_tx_enable(gmacdev);
	synopGMAC_rx_enable(gmacdev);

	/* default to 100M, I think this will be the common case */
	synopGMAC_select_mii(gmacdev);
	synopGMAC_select_speed100(gmacdev);

	/* Frame Filter Configuration */
	synopGMAC_frame_filter_enable(gmacdev);
	synopGMAC_set_pass_control(gmacdev,GmacPassControl0);
	synopGMAC_broadcast_enable(gmacdev);
	synopGMAC_src_addr_filter_disable(gmacdev);
	synopGMAC_multicast_disable(gmacdev);
	synopGMAC_dst_addr_filter_normal(gmacdev);
	synopGMAC_multicast_hash_filter_disable(gmacdev);
	synopGMAC_promisc_disable(gmacdev);
	synopGMAC_unicast_hash_filter_disable(gmacdev);

	/*Flow Control Configuration*/
	synopGMAC_unicast_pause_frame_detect_disable(gmacdev);
	synopGMAC_rx_flow_control_disable(gmacdev);
	synopGMAC_tx_flow_control_disable(gmacdev);
}

static void jz_mac_configure(struct jz_mac_local *lp) {
#ifdef ENH_DESC_8W
	synopGMAC_dma_bus_mode_init(gmacdev, DmaBurstLength32 | DmaDescriptorSkip2 | DmaDescriptor8Words | DmaFixedBurstEnable | 0x02000000); //pbl32 incr with rxthreshold 128 and Desc is 8 Words
	//synopGMAC_dma_bus_mode_init(gmacdev, DmaBurstLength32 | DmaDescriptorSkip2 | DmaDescriptor8Words | DmaFixedBurstEnable); //pbl32 incr with rxthreshold 128 and Desc is 8 Words
#else
	synopGMAC_dma_bus_mode_init(gmacdev, DmaBurstLength32 | DmaDescriptorSkip2);                      //pbl32 incr with rxthreshold 128
#endif
	/* DmaRxThreshCtrl128 is ok for the RX FIFO is configured to 256 Bytes */
	synopGMAC_dma_control_init(gmacdev,DmaStoreAndForward |DmaTxSecondFrame|DmaRxThreshCtrl128);

	/*Initialize the mac interface*/

	jzmac_init();
	//synopGMAC_pause_control(gmacdev); // This enables the pause control in Full duplex mode of operation

#ifdef IPC_OFFLOAD
	/*IPC Checksum offloading is enabled for this driver. Should only be used if Full Ip checksumm offload engine is configured in the hardware*/
	synopGMAC_enable_rx_chksum_offload(gmacdev);  	//Enable the offload engine in the receive path
	synopGMAC_rx_tcpip_chksum_drop_enable(gmacdev); // This is default configuration, DMA drops the packets if error in encapsulated ethernet payload
							// The FEF bit in DMA control register is configured to 0 indicating DMA to drop the errored frames.
	/*Inform the Linux Networking stack about the hardware capability of checksum offloading*/
	netdev->features = NETIF_F_HW_CSUM;
#endif

	synopGMAC_clear_interrupt(gmacdev);
	/*
	  Disable the interrupts generated by MMC and IPC counters.
	  If these are not disabled ISR should be modified accordingly to handle these interrupts.
	*/
	synopGMAC_disable_mmc_tx_interrupt(gmacdev, 0xFFFFFFFF);
	synopGMAC_disable_mmc_rx_interrupt(gmacdev, 0xFFFFFFFF);
	synopGMAC_disable_mmc_ipc_rx_interrupt(gmacdev, 0xFFFFFFFF);
}

/*
 * Enable Interrupts, Receive, and Transmit(The same sequence as jz_mac_open, only a bit different)
 */
static void jz_mac_enable(struct jz_mac_local *lp) {
	jz_mac_configure(lp);

	napi_enable(&lp->napi);
	synopGMAC_enable_interrupt(gmacdev,DmaIntEnable);
	/* we only enable rx here */
	synopGMAC_enable_dma_rx(gmacdev);
	/* We can accept TX packets again */
	lp->netdev->trans_start = jiffies;
	netif_wake_queue(lp->netdev);
}

static void jzmac_reinit_locked(struct jz_mac_local *lp)
{
	WARN_ON(in_interrupt());
	jz_mac_disable(lp);
	jz_mac_enable(lp);
}

/* Our watchdog timed out. Called by the networking layer */
static void jz_mac_tx_timeout(struct net_device *dev)
{
	struct jz_mac_local *lp = netdev_priv(dev);

	/* Do the reset outside of interrupt context */
	lp->tx_timeout_count++;
	schedule_work(&lp->reset_task);
}

static void jzmac_reset_task(struct work_struct *work)
{
	struct jz_mac_local *lp =
		container_of(work, struct jz_mac_local, reset_task);

	jzmac_reinit_locked(lp);
}

/*
 * Multicast filter and config multicast hash table
 */
#define MULTICAST_FILTER_LIMIT 64

static void jzmac_multicast_hash(struct net_device *dev)
{
	u32 emac_hashhi, emac_hashlo;
	//struct dev_mc_list *dmi = dev->mc_list;
	struct list_head *nodei = dev->mc.list.next;
	struct netdev_hw_addr *dmi = (struct netdev_hw_addr *)nodei;
	char *addrs;
	int i;
	u32 crc;

	emac_hashhi = emac_hashlo = 0;

	for (i = 0; i < dev->mc.count; i++) {
		addrs = dmi->addr;
		nodei = nodei->next;
		dmi = (struct netdev_hw_addr *)nodei;

		/* skip non-multicast addresses */
		if (!(*addrs & 1))
			continue;

		crc = ether_crc(ETH_ALEN, addrs);
		crc >>= 26;

		if (crc & 0x20)
			emac_hashhi |= 1 << (crc & 0x1f);
		else
			emac_hashlo |= 1 << (crc & 0x1f);
	}

	//__sal_set_hash_table(emac_hashhi, emac_hashlo);
	/* TODO: set multicast hash filter here */

	return;
}

static void jzmac_set_multicast_list(struct net_device *dev)
{
	if (dev->flags & IFF_PROMISC) {
		/* Accept any kinds of packets */
		synopGMAC_promisc_enable(gmacdev);
		printk("%s: Enter promisc mode!\n",dev->name);
	} else  if ((dev->flags & IFF_ALLMULTI) || (dev->mc.count > MULTICAST_FILTER_LIMIT)) {
		/* Accept all multicast packets */
		synopGMAC_multicast_enable(gmacdev);

		/* TODO: accept broadcast and enable multicast here */
		printk("%s: Enter allmulticast mode!   %d \n",dev->name,dev->mc.count);
	} else if (dev->mc.count) {
		/* Update multicast hash table */
		jzmac_multicast_hash(dev);
		/* TODO: enable multicast here */
	} else {
		/* FIXME: clear promisc or multicast mode */
		synopGMAC_promisc_disable(gmacdev);
	}
}

static void setup_mac_addr(u8 *mac_addr) {
	synopGMAC_set_mac_addr(gmacdev,
			       GmacAddr0High,GmacAddr0Low,
			       mac_addr);
}

static int jz_mac_set_mac_address(struct net_device *dev, void *p) {
	struct sockaddr *addr = p;
	if (netif_running(dev))
		return -EBUSY;
	memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);
	setup_mac_addr(dev->dev_addr);
	return 0;
}

/*
 * Open and Initialize the interface
 *
 * Set up everything, reset the card, etc..
 */
static int jz_mac_open(struct net_device *dev)
{
	struct jz_mac_local *lp = netdev_priv(dev);
	int retval;

	pr_debug("%s: %s\n", dev->name, __func__);

	/*
	 * Check that the address is valid.  If its not, refuse
	 * to bring the device up.  The user must specify an
	 * address using ifconfig eth0 hw ether xx:xx:xx:xx:xx:xx
	 */
	if (!is_valid_ether_addr(dev->dev_addr)) {
		printk(KERN_WARNING JZMAC_DRV_NAME ": no valid ethernet hw addr\n");
		return -EINVAL;
	}

	phy_write(lp->phydev, MII_BMCR, BMCR_RESET);
	while(phy_read(lp->phydev, MII_BMCR) & BMCR_RESET);
	phy_start(lp->phydev);

	if (synopGMAC_reset(gmacdev) < 0) {
		printk("func:%s, synopGMAC_reset failed\n", __func__);
		phy_stop(lp->phydev);
		return -1;
	}

	/* init MDC CLK */
	synopGMAC_set_mdc_clk_div(gmacdev,GmiiCsrClk4);
	gmacdev->ClockDivMdc = synopGMAC_get_mdc_clk_div(gmacdev);

	/* initial rx and tx list */
	retval = desc_list_init(lp);

	if (retval)
		return retval;



	setup_mac_addr(dev->dev_addr);
	jz_mac_configure(lp);

	//jzmac_dump_all_regs(__func__, __LINE__);

	napi_enable(&lp->napi);

	/* we are ready, reset GMAC and enable interrupts */
	synopGMAC_enable_interrupt(gmacdev,DmaIntEnable);
	/* we only enable rx here */
	synopGMAC_enable_dma_rx(gmacdev);

	/* We can accept TX packets again */
	lp->netdev->trans_start = jiffies;
	netif_start_queue(dev);

	return 0;
}

/*
 * this makes the board clean up everything that it can
 * and not talk to the outside world.   Caused by
 * an 'ifconfig ethX down'
 */
static int jz_mac_close(struct net_device *dev)
{
	struct jz_mac_local *lp = netdev_priv(dev);

	jz_mac_disable(lp);

	netif_carrier_off(dev);

	phy_stop(lp->phydev);
	phy_write(lp->phydev, MII_BMCR, BMCR_PDOWN);

	/* free the rx/tx buffers */
	desc_list_free(lp);
	return 0;
}

static struct net_device_stats *jz_mac_get_stats(struct net_device *netdev)
{
	struct jz_mac_local *lp = netdev_priv(netdev);

	/* only return the current stats */
	return &lp->net_stats;
}

static int jz_mac_change_mtu(struct net_device *netdev, int new_mtu) {
	printk("===>new_mtu = %d\n", new_mtu);
#if 0
	return eth_change_mtu(netdev, new_mtu);
#else
	netdev->mtu = new_mtu;
	return 0;
#endif
}

static int jzmac_do_ioctl(struct net_device *netdev, struct ifreq *ifr, s32 cmd) {
	struct jz_mac_local *lp = netdev_priv(netdev);

	if (!netif_running(netdev)) {
		printk("error : it is not in netif_running\n");
		return -EINVAL;
	}

	if(!(lp->phydev->link))
		return 0;

	return generic_mii_ioctl(&lp->mii, if_mii(ifr), cmd, NULL);
}

static const struct net_device_ops jz_mac_netdev_ops = {
	.ndo_open		= jz_mac_open,
	.ndo_stop		= jz_mac_close,
	.ndo_start_xmit		= jz_mac_hard_start_xmit,
	.ndo_get_stats		= jz_mac_get_stats,
	.ndo_set_mac_address	= jz_mac_set_mac_address,
	.ndo_tx_timeout		= jz_mac_tx_timeout,
//	.ndo_set_multicast_list	= jzmac_set_multicast_list,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_change_mtu		= jz_mac_change_mtu,
	.ndo_do_ioctl		= jzmac_do_ioctl,
};

static int jz_mac_probe(struct platform_device *pdev)
{
	struct net_device *ndev;
	struct jz_mac_local *lp;
	struct platform_device *pd;
	int rc;
	int i;

	printk("=======>gmacdev = 0x%08x<================\n", (u32)gmacdev);
	synopGMAC_multicast_enable(gmacdev);
	if (gmacdev) {
		printk("=========>gmacdev->MacBase = 0x%08x DmaBase = 0x%08x\n",
		       gmacdev->MacBase, gmacdev->DmaBase);
	}
	if (synopGMAC_reset(gmacdev) < 0) {
		printk("func:%s, synopGMAC_reset failed\n", __func__);
		return -1;
	}
	/* init MDC CLK */
	synopGMAC_set_mdc_clk_div(gmacdev,GmiiCsrClk4);
	gmacdev->ClockDivMdc = synopGMAC_get_mdc_clk_div(gmacdev);
	/*Lets read the version of ip in to device structure*/
	synopGMAC_read_version(gmacdev);

	ndev = alloc_etherdev(sizeof(struct jz_mac_local));
	if (!ndev) {
		dev_err(&pdev->dev, "Cannot allocate net device!\n");
		return -ENOMEM;
	}

	SET_NETDEV_DEV(ndev, &pdev->dev);
	platform_set_drvdata(pdev, ndev);
	lp = netdev_priv(ndev);
	lp->netdev = ndev;
	lp->pdev = pdev;

	/* configure MAC address */
	if (bootargs_ethaddr) {
		for (i=0; i<6; i++) {
			ndev->dev_addr[i] = ethaddr_hex[i];
		}
	} else {
		random_ether_addr(ndev->dev_addr);
	}

	setup_mac_addr(ndev->dev_addr);

	if (!pdev->dev.platform_data) {
		dev_err(&pdev->dev, "Cannot get platform device jz_mii_bus!\n");
		rc = -ENODEV;
		goto out_err_probe_mac;
	}
	pd = pdev->dev.platform_data;
	lp->mii_bus = platform_get_drvdata(pd);
#ifdef CONFIG_MDIO_GPIO
	lp->mii_bus->priv = ndev;
#endif

	rc = mii_probe(ndev);
	if (rc) {
		dev_err(&pdev->dev, "MII Probe failed!\n");
		goto out_err_mii_probe;
	}

	/* Fill in the fields of the device structure with ethernet values. */
	ether_setup(ndev);

	ndev->netdev_ops = &jz_mac_netdev_ops;
	//ndev->ethtool_ops = &jz_mac_ethtool_ops;
	ndev->watchdog_timeo = 2 * HZ;

	lp->mii.phy_id	= lp->phydev->phy_id;
	lp->mii.phy_id_mask  = 0x1f;
	lp->mii.reg_num_mask = 0x1f;
	lp->mii.dev	= ndev;
	lp->mii.mdio_read    = jz_mdio_phy_read;
	lp->mii.mdio_write   = jz_mdio_phy_write;

	netif_napi_add(ndev, &lp->napi, jzmac_clean, 32);

	spin_lock_init(&lp->link_lock);
	spin_lock_init(&lp->napi_poll_lock);

	init_timer(&lp->watchdog_timer);
	lp->watchdog_timer.data = (unsigned long)lp;
	lp->watchdog_timer.function = &jzmac_watchdog;

	INIT_WORK(&lp->reset_task, jzmac_reset_task);

	/* register irq handler */
	rc = request_irq(IRQ_ETH, jz_mac_interrupt,
			 IRQF_DISABLED, "jzmac", ndev);
	if (rc) {
		dev_err(&pdev->dev, "Cannot request jz MAC IRQ!\n");
		rc = -EBUSY;
		goto out_err_request_irq;
	}

	strcpy(ndev->name, "eth%d");
	rc = register_netdev(ndev);
	if (rc) {
		dev_err(&pdev->dev, "Cannot register net device!\n");
		goto out_err_reg_ndev;
	}

	/* now, print out the card info, in a short format.. */
	dev_info(&pdev->dev, "%s, Version %s\n", JZMAC_DRV_DESC, JZMAC_DRV_VERSION);
//	jzmac_dump_all_regs(__func__, __LINE__);
	synopGMAC_multicast_enable(gmacdev);
	return 0;

 out_err_reg_ndev:
	free_irq(IRQ_ETH, ndev);
 out_err_request_irq:
 out_err_mii_probe:
#ifndef CONFIG_MDIO_GPIO
	mdiobus_unregister(lp->mii_bus);
	mdiobus_free(lp->mii_bus);
#endif
 out_err_probe_mac:
	platform_set_drvdata(pdev, NULL);
	free_netdev(ndev);

	return rc;
}

static int  jz_mac_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct jz_mac_local *lp = netdev_priv(ndev);

	platform_set_drvdata(pdev, NULL);

#ifndef CONFIG_MDIO_GPIO
	lp->mii_bus->priv = NULL;
#endif

	unregister_netdev(ndev);

	free_irq(IRQ_ETH, ndev);

	free_netdev(ndev);

	return 0;
}

#ifdef CONFIG_PM
static int jz_mac_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	struct net_device *net_dev = platform_get_drvdata(pdev);

	if (netif_running(net_dev))
		jz_mac_close(net_dev);

	return 0;
}

static int jz_mac_resume(struct platform_device *pdev)
{
	struct net_device *net_dev = platform_get_drvdata(pdev);

	if (netif_running(net_dev))
		jz_mac_open(net_dev);

	return 0;
}
#else
#define jz_mac_suspend NULL
#define jz_mac_resume NULL
#endif	/* CONFIG_PM */

#ifndef CONFIG_MDIO_GPIO
/* MII Bus platform routines */
/*
 * MII operations
 */

/* Read an off-chip register in a PHY through the MDC/MDIO port */
static int jz_mdiobus_read(struct mii_bus *bus, int phy_addr, int regnum)
{
	u16 data = 0;
	s32 status;

	status = synopGMAC_read_phy_reg(gmacdev, phy_addr, regnum, &data);
	//printk("=======>mdio read phy%d reg %d, return data = 0x%04x status = %d\n", phy_addr, regnum, data, status);

	/* if failed, set data to all 0 */
	if (status)
		data = 0;

	return (int)data;
}

static int jz_mdio_phy_read(struct net_device *dev, int phy_id, int location)
{
	struct jz_mac_local *lp = netdev_priv(dev);
	return jz_mdiobus_read(lp->mii_bus, phy_id, location);
}

/* Write an off-chip register in a PHY through the MDC/MDIO port */
static int jz_mdiobus_write(struct mii_bus *bus, int phy_addr, int regnum,
				u16 value)
{
	//printk("======>mdio write phy%d reg %d with value = 0x%04x\n", phy_addr, regnum, value);
	return synopGMAC_write_phy_reg(gmacdev, phy_addr, regnum, value);
}

static int jz_mdio_phy_write(struct net_device *dev, int phy_id, int location, int value)
{
	struct jz_mac_local *lp = netdev_priv(dev);
	return jz_mdiobus_write(lp->mii_bus, phy_id, location, value);
}

static int jz_mdiobus_reset(struct mii_bus *bus)
{
	return 0;
}

static int  jz_mii_bus_probe(struct platform_device *pdev)
{
	struct mii_bus *miibus;
	int rc = 0, i;
#ifdef CONFIG_JZGPIO_PHY_RESET /* PHY hard reset */
	struct jz_gpio_phy_reset *gpio_phy_reset;
#endif
	struct clk *gmac_clk = clk_get(NULL, "gmac");

//	if (clk_enable(gmac_clk) < 0) {
//		printk("enable gmac clk failed\n");
	//	clk_put(gmac_clk);
//		goto out_err_alloc;
//	}

	synopGMAC_multicast_enable(gmacdev);
#if defined(CONFIG_JZ_MAC_RMII)
	set_mac_phy_clk(MAC_RMII);
#elif defined(CONFIG_JZ_MAC_RGMII)
	set_mac_phy_clk(MAC_RGMII);
#elif defined(CONFIG_JZ_MAC_GMII)
	set_mac_phy_clk(MAC_GMII);
#else
	set_mac_phy_clk(MAC_MII);
#endif
//	clk_put(gmac_clk);

#ifdef CONFIG_JZGPIO_PHY_RESET /* PHY hard reset */
	gpio_phy_reset = dev_get_platdata(&pdev->dev);
	jzgpio_phy_reset(gpio_phy_reset);
#endif

#if defined(CONFIG_jz_MAC_RGMII) || defined(CONFIG_jz_MAC_GMII)
	/* CIM0_MCLK/GMAC_GTXC/EPD_PWC/PB9 */
	jzgpio_set_func(GPIO_PORT_B, GPIO_FUNC_0, 0x00000200);
	/* Set GMAC_TXCLK/PF6 gpio in */
	jzgpio_set_func(GPIO_PORT_F, GPIO_INPUT, 0x00000040);
#endif

	if (synopGMAC_reset(gmacdev) < 0) {
		printk("func:%s, synopGMAC_reset failed\n", __func__);
		return -1;
	}

	/* init MDC CLK */
	/* The CSR clock is used to generate the MDC clock for the SMA interface. */
	synopGMAC_set_mdc_clk_div(gmacdev,GmiiCsrClk4);
	gmacdev->ClockDivMdc = synopGMAC_get_mdc_clk_div(gmacdev);

	synopGMAC_disable_interrupt_all(gmacdev);

	/* TODO: init MII stuffs here, such as reset MII, set to MII/RMII/GMII by default */

	rc = -ENOMEM;
	miibus = mdiobus_alloc();
	if (miibus == NULL)
		goto out_err_alloc;
	miibus->read = jz_mdiobus_read;
	miibus->write = jz_mdiobus_write;
	miibus->reset = jz_mdiobus_reset;

	miibus->parent = &pdev->dev;
	miibus->name = "jz_mii_bus";
	snprintf(miibus->id, MII_BUS_ID_SIZE, "0");
	miibus->irq = kmalloc(sizeof(int)*PHY_MAX_ADDR, GFP_KERNEL);
	if (miibus->irq == NULL)
		goto out_err_alloc;
	for (i = 0; i < PHY_MAX_ADDR; ++i)
		miibus->irq[i] = PHY_POLL;

	rc = mdiobus_register(miibus);
	if (rc) {
		dev_err(&pdev->dev, "Cannot register MDIO bus!\n");
		goto out_err_mdiobus_register;
	}

	platform_set_drvdata(pdev, miibus);
synopGMAC_multicast_enable(gmacdev);
	return 0;

 out_err_mdiobus_register:
	mdiobus_free(miibus);

 out_err_alloc:

	return rc;
}

static int  jz_mii_bus_remove(struct platform_device *pdev)
{
	struct mii_bus *miibus = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
	mdiobus_unregister(miibus);
	mdiobus_free(miibus);
	return 0;
}


static struct platform_driver jz_mii_bus_driver = {
	.probe = jz_mii_bus_probe,
	.remove = jz_mii_bus_remove,
	.driver = {
		.name = "jz_mii_bus",
		.owner	= THIS_MODULE,
	},
};
#else  /* CONFIG_MDIO_GPIO */
static void jz_mdio_gpio_init(void) {
	cpm_start_clock(CGM_MAC);

	synopGMAC_disable_interrupt_all(gmacdev);
}
#endif	/* CONFIG_MDIO_GPIO */

static struct platform_driver jz_mac_driver = {
	.probe = jz_mac_probe,
	.remove = jz_mac_remove,
	.resume = jz_mac_resume,
	.suspend = jz_mac_suspend,
	.driver = {
		.name = JZMAC_DRV_NAME,
		.owner	= THIS_MODULE,
	},
};

#define JZ_GMAC_BASE 0xb34b0000

extern int disable_gmac;

static int __init jz_mac_init(void)
{
if (!disable_gmac){
#ifndef CONFIG_MDIO_GPIO
	int ret;
#endif
	gmacdev = kmalloc(sizeof (synopGMACdevice), GFP_KERNEL);
	if(!gmacdev){
		printk("Error when alloc gmacdev.\n");
		return -ENOMEM;
	}

	memset((void *)gmacdev,0,sizeof(synopGMACdevice));

	gmacdev->DmaBase =  JZ_GMAC_BASE + DMABASE;
	gmacdev->MacBase =  JZ_GMAC_BASE + MACBASE;

	synopGMAC_multicast_enable(gmacdev);
#ifndef CONFIG_MDIO_GPIO
	ret = platform_driver_register(&jz_mii_bus_driver);
	if (!ret) {
		return platform_driver_register(&jz_mac_driver);
	}
	return -ENODEV;
#else
	return platform_driver_register(&jz_mac_driver);

#endif
}
else{
	pr_info("JZ GMAC disabled\n");
	return -ENODEV;
}
}

module_init(jz_mac_init);

static void __exit jz_mac_cleanup(void)
{
	platform_driver_unregister(&jz_mac_driver);
#ifndef CONFIG_MDIO_GPIO
	platform_driver_unregister(&jz_mii_bus_driver);
#endif
}

module_exit(jz_mac_cleanup);
