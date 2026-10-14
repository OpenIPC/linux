// SPDX-License-Identifier: GPL-2.0
// Copyright (c) LOTUS. All rights reserved.

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/clk.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/pagemap.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/interrupt.h>
#include <linux/tcp.h>
#include <linux/ipv6.h>
#include <linux/slab.h>
#include <net/checksum.h>
#include <net/ip6_checksum.h>
#include <linux/ethtool.h>
#include <linux/if_vlan.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#include <linux/pm_runtime.h>
#include <linux/prefetch.h>
#include <linux/of.h>
#include <linux/of_net.h>
#include <linux/of_mdio.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/reset.h>

#include "ftgmac030.h"

#define DEEDBEEF		0

// Define this whenever h/w capabale doing 2-byte align DMA
#undef FTGMAC030_2BYTE_DMA_ALIGN

#define DRV_VERSION "1.0.0"
char ftgmac030_driver_name[] = "ftgmac030";
const char ftgmac030_driver_version[] = DRV_VERSION;

#define DEFAULT_MSG_ENABLE (NETIF_MSG_DRV|NETIF_MSG_PROBE|NETIF_MSG_LINK\
			   |NETIF_MSG_RX_ERR|NETIF_MSG_TX_ERR|NETIF_MSG_HW\
			   |NETIF_MSG_TX_DONE|NETIF_MSG_RX_STATUS|NETIF_MSG_PKTDATA|NETIF_MSG_INTR)

static int debug = -1;
module_param(debug, int, 0);
MODULE_PARM_DESC(debug, "Debug level (0=none,...,16=all)");

#define COPYBREAK_DEFAULT 256
unsigned int copyBreak = COPYBREAK_DEFAULT;
module_param(copyBreak, uint, 0644);
MODULE_PARM_DESC(copyBreak,
		 "Maximum size of packet that is copied to a new buffer on receive");

struct ftgmac030_reg_info {
	u32 ofs;
	char *name;
};

static const struct ftgmac030_reg_info ftgmac030_reg_info_tbl[] = {
	// Interrupt Registers
	{FTGMAC030_REG_ISR, "ISR"},
	{FTGMAC030_REG_IER, "IER"},
	// MAC address Registers
	{FTGMAC030_REG_MAC_MADR, "MAC_MADR"},
	{FTGMAC030_REG_MAC_LADR, "MAC_LADR"},

	// Multicast Address Hash table
	{FTGMAC030_REG_MAHT0, "MAHT0"},
	{FTGMAC030_REG_MAHT1, "MAHT1"},

	// Tx/Rx Descriptor base address
	{FTGMAC030_REG_NPTXR_BADR, "NPTXR_BADR"},
	{FTGMAC030_REG_RXR_BADR, "RXR_BADR"},
	{FTGMAC030_REG_HPTXR_BADR, "HPTXR_BADR"},

	// Interrupt Time Control
	{FTGMAC030_REG_TXITC, "TXITC"},
	{FTGMAC030_REG_RXITC, "RXITC"},

	// DMA and FIFO related Registers
	{FTGMAC030_REG_APTC, "APTC"},
	{FTGMAC030_REG_DBLAC, "DBLAC"},
	{FTGMAC030_REG_DMAFIFOS, "DMAFIFOS"},
	{FTGMAC030_REG_TPAFCR, "TPAFCR"},

	// Receive Buffer Size
	{FTGMAC030_REG_RBSR, "RXBUF_SIZE"},

	// MAC control and status
	{FTGMAC030_REG_MACCR, "MACCR"},
	{FTGMAC030_REG_MACSR, "MACSR"},

	// Phy Access Registers
	{FTGMAC030_REG_PHYCR, "PHYCR"},
	{FTGMAC030_REG_PHYDATA, "PHYDATA"},

	// Flow Control
	{FTGMAC030_REG_FCR, "FCR"},

	{FTGMAC030_REG_BPR, "BPR"},

	// Wake On LAN Registers
	{FTGMAC030_REG_WOLCR, "WOLCR"},
	{FTGMAC030_REG_WOLSR, "WOLSR"},
	{FTGMAC030_REG_WFCRC, "WFCRC"},
	{FTGMAC030_REG_WFBM1, "WFBM"},

	// Tx/Rx ring pointer Registers
	{FTGMAC030_REG_NPTXR_PTR, "NPTXR_PTR"},
	{FTGMAC030_REG_HPTXR_PTR, "HPTXR_PTR"},
	{FTGMAC030_REG_RXR_PTR, "RXR_PTR"},

	// Statistics value Registers
	{FTGMAC030_REG_TX_CNT, "TX_CNT"},
	{FTGMAC030_REG_TX_MCOL_SCOL, "TX_MCOL_SCOL"},
	{FTGMAC030_REG_TX_ECOL_FAIL, "TX_ECOL_FAIL"},
	{FTGMAC030_REG_TX_LCOL_UND, "TX_LCOL_UND"},
	{FTGMAC030_REG_RX_CNT, "RX_CNT"},
	{FTGMAC030_REG_RX_BC, "RX_BC"},
	{FTGMAC030_REG_RX_MC, "RX_MC"},
	{FTGMAC030_REG_RX_PF_AEP, "RX_PF_AEP"},
	{FTGMAC030_REG_RX_RUNT, "RX_RUNT"},
	{FTGMAC030_REG_RX_CRCER_FTL, "RX_CRCER_FTL"},
	{FTGMAC030_REG_RX_COL_LOST, "RX_COL_LOST"},

	// Broadcast and Multicast received control
	{FTGMAC030_REG_BMRCR, "BMRCR"},

	// Error and Debug Registers
	{FTGMAC030_REG_ERCR, "ERCR"},
	{FTGMAC030_REG_ACIR, "ACIR"},

	// Phy Interface
	{FTGMAC030_REG_GISR, "GISR"},

	{FTGMAC030_REG_EEE, "EEE"},

	// Revision and Feature
	{FTGMAC030_REG_REVR, "REVR"},
	{FTGMAC030_REG_FEAR, "FEAR"},

	// PTP Registers
	{FTGMAC030_REG_PTP_TX_SEC, "PTP_TX_SEC"},
	{FTGMAC030_REG_PTP_TX_NSEC, "PTP_TX_NSEC"},
	{FTGMAC030_REG_PTP_RX_SEC, "PTP_RX_SEC"},
	{FTGMAC030_REG_PTP_RX_NSEC, "PTP_RX_NSEC"},
	{FTGMAC030_REG_PTP_TX_P_SEC, "PTP_TX_P_SEC"},
	{FTGMAC030_REG_PTP_TX_P_NSEC, "PTP_TX_P_NSEC"},
	{FTGMAC030_REG_PTP_RX_P_SEC, "PTP_RX_P_SEC"},
	{FTGMAC030_REG_PTP_RX_P_NSEC, "PTP_RX_P_NSEC"},
	{FTGMAC030_REG_PTP_TM_NNSEC, "PTP_TM_NNSEC"},
	{FTGMAC030_REG_PTP_TM_NSEC, "PTP_TM_NSEC"},
	{FTGMAC030_REG_PTP_TM_SEC, "PTP_TM_SEC"},
	{FTGMAC030_REG_PTP_NS_PERIOD, "PTP_NS_PERIOD"},
	{FTGMAC030_REG_PTP_NNS_PERIOD, "PTP_NNS_PERIOD"},
	{FTGMAC030_REG_PTP_PERIOD_INCR, "PTP_PERIOD_INCR"},
	{FTGMAC030_REG_PTP_ADJ_VAL, "PTP_ADJ_VAL"},

	// List Terminator
	{0, NULL}
};

/**
 * ftgmac030_dump - Print registers, Tx-ring and Rx-ring
 * @ctrl: board private structure
 */
void ftgmac030_dump(struct ftgmac030_ctrl *ctrl)
{
	struct net_device *netdev = ctrl->netdev;
	struct platform_device *pdev = ctrl->pdev;
	struct ftgmac030_reg_info *reginfo;
	struct ftgmac030_ring *tx_ring = ctrl->tx_ring;
	struct ftgmac030_txdes *tx_desc;
	struct ftgmac030_buffer *buffer_info;
	struct ftgmac030_ring *rx_ring = ctrl->rx_ring;
	struct ftgmac030_rxdes *rx_desc;
	const char *next_desc;
	u32 rxdes0;
	struct my_u0 {
		__le32 a;
		__le32 b;
		__le32 c;
		__le32 d;
	} *u0;
	int i = 0, j;

	// Print netdevice Info
	if (netdev) {
		dev_info(&pdev->dev, "Net device Info\n");
		pr_info("Device Name     state            trans_start\n");
		pr_info("%-15s %016lX %016lX\n", netdev->name,
			netdev->state, dev_trans_start(netdev));
	}

	// Print Registers
	dev_info(&pdev->dev, "Register Dump\n");
	pr_info(" Register Name   Value\n");
	for (reginfo = (struct ftgmac030_reg_info *)ftgmac030_reg_info_tbl;
	     reginfo->name; reginfo++) {
		char regs1[32], regs2[32];

		snprintf(regs1, 32, "%-15s[%3x] %08x", reginfo->name,
			 reginfo->ofs, ior32(reginfo->ofs));

		reginfo++;
		if (reginfo->name)
			snprintf(regs2, 32, "%-15s[%3x] %08x", reginfo->name,
				 reginfo->ofs, ior32(reginfo->ofs));

		pr_info("%s,  %s\n", regs1, (reginfo->name) ? regs2 : "");
	}

	if (!netdev || !netif_running(netdev))
		return;

	dev_info(&pdev->dev, "Tx FIFO limit %d\n", ctrl->tx_fifo_limit);

	dev_info(&pdev->dev, "Tx Ring Summary\n");
	pr_info("Queue [NTU] [NTC] [ntc->dma] [ head ] leng ntw timestamp\n");
	buffer_info = &tx_ring->buffer_info[tx_ring->next_to_clean];
	pr_info("%5d %5d %5d  0x%08X  0x%08X 0x%04X %3d 0x%016llX\n",
		0, tx_ring->next_to_use, tx_ring->next_to_clean,
		cpu_to_le32(buffer_info->dma), readl(tx_ring->head),
		buffer_info->length, buffer_info->next_to_watch,
		(unsigned long long)buffer_info->time_stamp);

	dev_info(&pdev->dev, "Tx Ring Dump\n");

	// Transmit Descriptor Formats
	//     31   30  29  28    27   26-20    19  18-16  15    14  13       0
	//   +-----------------------------------------------------------------+
	// 0 |OWN| Rsvd|FTS|LTS|BUS_ERR|Rsvd|CRC_ERR|Rsvd|EDOTR|Rsvd|TXBUF_SIZE|
	//   +-----------------------------------------------------------------+
	//     31    30  29-21  22   21-20   19  18   17   16   15           0
	//   +----------------------------------------------------------------+
	// 1 |TXIC|TX2FIC|Rsvd|LLC|PKT_TYPE|IPC|UDPC|TCPC|VLAN|   VLAN Tag    |
	//   +----------------------------------------------------------------+
	// 2 |                     Reserved(SW used) [31:0]                   |
	//   +----------------------------------------------------------------+
	// 3 |                     Buffer Address [31:0]                      |
	//   +----------------------------------------------------------------+
	pr_info("[desc]   [txdes0] [txdes1] [txdes2] [txdes3] "
		"[bi->dma] leng  ntw timestamp        bi->skb  next\n");

	i = (readl(tx_ring->head) - ior32(FTGMAC030_REG_NPTXR_BADR)) / 0x10;

	dev_info(&pdev->dev, "Hardware descriptor index %d\n", i);

	// Due to HW problem not updated NPTXR_PTR correctly
	// before transmit a packet.
	if (i > tx_ring->count)
		goto skip_hw_desc;

	j = i + 2;
	for (; tx_ring->desc && (i < j); i++) {

		if (i ==  tx_ring->count)
			i = 0;

		tx_desc = FTGMAC030_TX_DESC(*tx_ring, i);
		buffer_info = &tx_ring->buffer_info[i];
		u0 = (struct my_u0 *)tx_desc;

		pr_info("[%04d]   %08X %08X %08X %08X  %08X "
			"%04X  %3X %016llX %p\n", i,
			le32_to_cpu(u0->a), le32_to_cpu(u0->b),
			le32_to_cpu(u0->c), le32_to_cpu(u0->d),
			cpu_to_le32(buffer_info->dma), buffer_info->length,
			buffer_info->next_to_watch,
			(unsigned long long)buffer_info->time_stamp,
			buffer_info->skb);

		if (netif_msg_pktdata(ctrl) && buffer_info->skb)
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS,
			       16, 1, buffer_info->skb->data,
			       (buffer_info->skb->len < 128) ?
			       buffer_info->skb->len : 128, true);
	}

skip_hw_desc:
	// Tx Ring
	if (!netif_msg_tx_done(ctrl))
		goto rx_ring_summary;

	dev_info(&pdev->dev, "Software descriptors content\n");
	for (i = 0; tx_ring->desc && (i < tx_ring->count); i++) {
		tx_desc = FTGMAC030_TX_DESC(*tx_ring, i);
		buffer_info = &tx_ring->buffer_info[i];
		u0 = (struct my_u0 *)tx_desc;

		if ((i != tx_ring->next_to_use) &&
		    (i != tx_ring->next_to_clean) &&
		    (!u0->a && !u0->b && !u0->c && !u0->d))
			continue;

		if (i == tx_ring->next_to_use && i == tx_ring->next_to_clean)
			next_desc = " NTC/U";
		else if (i == tx_ring->next_to_use)
			next_desc = " NTU";
		else if (i == tx_ring->next_to_clean)
			next_desc = " NTC";
		else
			next_desc = "";
		pr_info("[%04d]   %08X %08X %08X %08X  %08X "
			"%04X  %3d %016llX %p %s\n", i,
			le32_to_cpu(u0->a), le32_to_cpu(u0->b),
			le32_to_cpu(u0->c), le32_to_cpu(u0->d),
			cpu_to_le32(buffer_info->dma), buffer_info->length,
			buffer_info->next_to_watch,
			(unsigned long long)buffer_info->time_stamp,
			buffer_info->skb, next_desc);

		if (netif_msg_pktdata(ctrl) && buffer_info->skb)
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS,
			       16, 1, buffer_info->skb->data,
			       (buffer_info->skb->len < 128) ?
			       buffer_info->skb->len : 128, true);
	}

	// Rx Ring Summary
rx_ring_summary:
	dev_info(&pdev->dev, "Rx Ring Summary\n");
	pr_info("Queue [NTU] [NTC] [ head ] [ tail ]\n");
	pr_info(" %3d   %3d   %3d  %08X  %08X\n",
		0, rx_ring->next_to_use, rx_ring->next_to_clean,
		readl(rx_ring->head), readl(rx_ring->tail));

	if (!netif_msg_rx_status(ctrl))
		return;

	dev_info(&pdev->dev, "Rx Ring Dump, mtu %d bytes\n",
			     ctrl->netdev->mtu);
	// Receive Descriptor Format
	//    31   30  29  28  27   26  25 24 23 22  21   20  19    18   17  16   15    14  13-0
	//   +-----------------------------------------------------------------------------------+
	//   |RDY|Rsvd|FRS|LRS|ERR|Rsvd|PF|PC|FF|ODB|RUNT|FTL|CRC|RX_ERR| B | M |EDORR|Rsvd|BYTES|
	//   +-----------------------------------------------------------------------------------+
	//
	//   31-30 29-28 27   26   25   24   23  22  21-20  19 18-16  15      0
	//   +-----------------------------------------------------------------+
	// 1 |Rsvd| PTP |IPC|UDPC|TCPC|VLAN|FRAG|LLC|PROTO|IPv6|Rsvd| VLAN Tag |
	//   +-----------------------------------------------------------------+
	// 2 |                      Reserved                       |
	//   +-----------------------------------------------------+
	// 3 |                Buffer Address [31:0]                |
	//   +-----------------------------------------------------+
	pr_info("[desc] [rxdes0] [rxdes1] [rxdes2] "
		"[rxdes3] [bi->dma] [bi->skb]   first/last/Ownn");

	for (i = 0; i < rx_ring->count; i++) {
		buffer_info = &rx_ring->buffer_info[i];
		rx_desc = FTGMAC030_RX_DESC(*rx_ring, i);
		u0 = (struct my_u0 *)rx_desc;

		if (!u0->a && !u0->b && !u0->c && !u0->d)
			continue;

		if (i == rx_ring->next_to_use)
			next_desc = " NTU";
		else if (i == rx_ring->next_to_clean)
			next_desc = " NTC";
		else
			next_desc = "";

		if (i > (rx_ring->next_to_use - 3) &&
		    i < (rx_ring->next_to_use + 3)) {
			rxdes0 = le32_to_cpu(rx_desc->rxdes0);
			pr_info("[%04d] %08X %08X %08X %08X  %08X %p%s    %c/%c/%s\n",
				 i,
				le32_to_cpu(u0->a), le32_to_cpu(u0->b),
				le32_to_cpu(u0->c), le32_to_cpu(u0->d),
				cpu_to_le32(buffer_info->dma), buffer_info->skb, next_desc,
				(rxdes0 & FTGMAC030_RXDES0_FRS) ? 'f' : ' ',
				(rxdes0 & FTGMAC030_RXDES0_LRS) ? 'l' : ' ',
				(rxdes0 & FTGMAC030_RXDES0_RXPKT_RDY) ? "sw" : "hw");
		}
	}

	/* Last descriptor */
	i = rx_ring->count - 1;
	buffer_info = &rx_ring->buffer_info[i];
	rx_desc = FTGMAC030_RX_DESC(*rx_ring, i);
	u0 = (struct my_u0 *)rx_desc;
	rxdes0 = le32_to_cpu(rx_desc->rxdes0);
	if (i == rx_ring->next_to_use)
		next_desc = " NTU";
	else if (i == rx_ring->next_to_clean)
		next_desc = " NTC";
	else
		next_desc = "";
	pr_info("[%04d] %08X %08X %08X %08X  %08X %p%s    %c/%c/%s\n",
		 i,
		le32_to_cpu(u0->a), le32_to_cpu(u0->b),
		le32_to_cpu(u0->c), le32_to_cpu(u0->d),
		cpu_to_le32(buffer_info->dma), buffer_info->skb, next_desc,
		(rxdes0 & FTGMAC030_RXDES0_FRS) ? 'f' : ' ',
		(rxdes0 & FTGMAC030_RXDES0_LRS) ? 'l' : ' ',
		(rxdes0 & FTGMAC030_RXDES0_RXPKT_RDY) ? "sw" : "hw");
}

static void ftgmac030_set_mdio_intf(struct ftgmac030_ctrl *ctrl)
{
	/* set the phy intf*/
	iow32(FTGMAC030_REG_GISR, ctrl->ftgmac030_mac_intf);
}

static void ftgmac030_sleep_us(u32 time_us)
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
static void ftgmac030_phy_reset(struct ftgmac030_ctrl *ctrl)
{
	/* To make sure PHY hardware reset success,
	 * we must keep PHY in deassert state first and
	 * then complete the hardware reset operation
	 */
	if (ctrl->ftgmac030_phy_reset_n)
		reset_control_assert(ctrl->phy_rst);
	else
		reset_control_deassert(ctrl->phy_rst);
	ftgmac030_sleep_us(ctrl->phy_reset_delays[PRE_DELAY]);
	if (ctrl->ftgmac030_phy_reset_n)
		reset_control_deassert(ctrl->phy_rst);
	else
		reset_control_assert(ctrl->phy_rst);
	/* delay some time to ensure reset ok,
	 * this depends on PHY hardware feature
	 */
	ftgmac030_sleep_us(ctrl->phy_reset_delays[PULSE]);
	if (ctrl->ftgmac030_phy_reset_n)
		reset_control_assert(ctrl->phy_rst);
	else
		reset_control_deassert(ctrl->phy_rst);
	/* delay some time to ensure later MDIO access */
	ftgmac030_sleep_us(ctrl->phy_reset_delays[POST_DELAY]);
}

/* Set rgmii tx delay for 2ns */
static int ftgmac030_set_tx_delay(struct ftgmac030_ctrl *ctrl)
{
	unsigned int regval;
	unsigned int timeout = 100;//1ms

	if(ctrl->ftgmac030_mac_intf != FTGMAC030_MODE_RGMII)
		return 0;

	regmap_read(ctrl->sysctrl_regmap, BSPETH_DLY_CRG_REG, &regval);

	if(!(regval & ETH_DLY_SEL)) {
		regmap_write(ctrl->sysctrl_regmap, BSPETH_DLY_CRG_REG, regval | ETH_DLY_SEL);

		do {
			udelay(10);
			regmap_read(ctrl->crg_regmap, BSPETH_DLY_STAT_REG, &regval);
			--timeout;
		} while ((!(regval & ETH_DLY_STAT) && timeout));
	}

	if(!timeout) {
		pr_err("ftgmac030_set_tx_delay timeout.\n");
		return -1;
	}

	return 0;
}

static void ftgmac030_phy_hw(struct ftgmac030_ctrl *ctrl)
{
	ftgmac030_set_mdio_intf(ctrl);

	ftgmac030_set_tx_delay(ctrl);

	/*reset phy*/
	ftgmac030_phy_reset(ctrl);
}

/**
 * ftgmac030_desc_unused - calculate if we have unused descriptors
 */
static int ftgmac030_desc_unused(struct ftgmac030_ring *ring)
{
	if (ring->next_to_clean > ring->next_to_use)
		return ring->next_to_clean - ring->next_to_use;

	return ring->count + ring->next_to_clean - ring->next_to_use;
}

static void ftgmac030_print_hw_hang(struct work_struct *work)
{
	struct ftgmac030_ctrl *ctrl = container_of(work,
						 struct ftgmac030_ctrl,
						 print_hang_task);
	struct net_device *netdev = ctrl->netdev;
	struct ftgmac030_ring *tx_ring = ctrl->tx_ring;
	unsigned int i = tx_ring->next_to_clean;
	unsigned int lts = tx_ring->buffer_info[i].next_to_watch;
	struct ftgmac030_txdes *lts_desc = FTGMAC030_TX_DESC(*tx_ring, lts);

	if (test_bit(__FTGMAC030_DOWN, &ctrl->state))
		return;

	netif_stop_queue(netdev);

	/* detected Hardware unit hang */
	e_err(hw, "Detected Hardware Unit Hang:\n"
	      "  NPTXR_PTR            <0x%x>\n"
	      "  next_to_use          <%d>\n"
	      "  next_to_clean        <%d>\n"
	      "buffer_info[next_to_clean]:\n"
	      "  time_stamp           <0x%lx>\n"
	      "  next_to_watch        <%d>\n"
	      "  jiffies              <0x%lx>\n"
	      "  txdes0               <0x%x>\n"
	      "MAC Status             <0x%x>\n",
	      readl(tx_ring->head), tx_ring->next_to_use, tx_ring->next_to_clean,
	      tx_ring->buffer_info[lts].time_stamp, lts, jiffies, lts_desc->txdes0,
	      ior32(FTGMAC030_REG_MACCR));

	//ftgmac030_dump(ctrl);
}

/**
 * ftgmac030_tx_hwtstamp - check for Tx time stamp
 * @ctrl: board private structure
 * @skb: pointer to sk_buff that require Tx time stamp
 * @status: interrupt status to check valid time stamp value
 *
 * This function reads interrupt status TX_TMSP_VALIDT valid bit to determine
 * when a timestamp has been taken for the current stored skb.  The timestamp
 * must be for this skb because only one such packet is allowed in the queue.
 */
static void ftgmac030_tx_hwtstamp(struct ftgmac030_ctrl *ctrl,
				  struct sk_buff *skb, u32 status)
{
	if (ctrl->tx_hwtstamp_skb == skb) {

		if (status & FTGMAC030_INT_TX_TMSP_VALID) {
			struct skb_shared_hwtstamps hwtstamps;
			u64 txstmp;
			u32 s_off, ns_off;

			if (status & (FTGMAC030_INT_SYNC_OUT |
				      FTGMAC030_INT_DELAY_REQ_OUT)) {
				s_off = FTGMAC030_REG_PTP_TX_SEC;
				ns_off = FTGMAC030_REG_PTP_TX_NSEC;
			} else {
				s_off = FTGMAC030_REG_PTP_TX_P_SEC;
				ns_off = FTGMAC030_REG_PTP_TX_P_NSEC;
			}

			spin_lock(&ctrl->systim_lock);
			txstmp = (u64)ior32(s_off) * NSEC_PER_SEC;
			txstmp +=  ior32(ns_off);
			spin_unlock(&ctrl->systim_lock);

			memset(&hwtstamps, 0, sizeof(hwtstamps));
			hwtstamps.hwtstamp = ns_to_ktime(txstmp);

			skb_tstamp_tx(ctrl->tx_hwtstamp_skb, &hwtstamps);
			dev_consume_skb_any(ctrl->tx_hwtstamp_skb);
			ctrl->tx_hwtstamp_skb = NULL;
		} else if (time_after(jiffies, ctrl->tx_hwtstamp_start
				      + ctrl->tx_timeout_factor * HZ)) {
			dev_kfree_skb_any(ctrl->tx_hwtstamp_skb);
			ctrl->tx_hwtstamp_skb = NULL;
			ctrl->tx_hwtstamp_timeouts++;
			e_warn(hw, "clearing Tx timestamp hang\n");
		}

	}
}

static void ftgmac030_put_txbuf(struct ftgmac030_ring *tx_ring,
				struct ftgmac030_buffer *buffer_info,
				bool drop)
{
	struct ftgmac030_ctrl *ctrl = tx_ring->hw;

	if (buffer_info->dma) {
		if (buffer_info->mapped_as_page)
			dma_unmap_page(&ctrl->pdev->dev, buffer_info->dma,
				       buffer_info->length, DMA_TO_DEVICE);
		else
			dma_unmap_single(&ctrl->pdev->dev, buffer_info->dma,
					 buffer_info->length, DMA_TO_DEVICE);
		buffer_info->dma = 0;
	}
	if (buffer_info->skb) {
		if (drop)
			dev_kfree_skb_any(buffer_info->skb);
		else
			dev_consume_skb_any(buffer_info->skb);
		buffer_info->skb = NULL;
	}
	buffer_info->time_stamp = 0;
}

/**
 * ftgmac030_clean_tx_irq - Reclaim resources after transmit completes
 * @tx_ring: Tx descriptor ring
 * @status: interrupt status for checkig ptp
 * the return value indicates whether actual cleaning was done, there
 * is no guarantee that everything was cleaned
 */
static bool ftgmac030_clean_tx_irq(struct ftgmac030_ring *tx_ring, u32 status)
{
	struct ftgmac030_ctrl *ctrl = tx_ring->hw;
	struct net_device *netdev = ctrl->netdev;
	struct ftgmac030_txdes *tx_desc;
	struct ftgmac030_buffer *buffer_info;
	unsigned int i, maccr;
	unsigned int count = 0;
	unsigned int total_tx_bytes = 0, total_tx_packets = 0;
	unsigned int bytes_compl = 0, pkts_compl = 0;

	spin_lock(&tx_ring->ntu_lock);

	i = tx_ring->next_to_clean;

	while ((i != tx_ring->next_to_use) && (count < tx_ring->count)) {

		tx_desc = FTGMAC030_TX_DESC(*tx_ring, i);
		buffer_info = &tx_ring->buffer_info[i];

		// Make hw descriptor updates visible to CPU.
		rmb();

		if (((le32_to_cpu(tx_desc->txdes0) & 0x3fff) == 0) ||
		    !tx_desc->txdes1 || !tx_desc->txdes2 ||
		    (tx_desc->txdes0 & cpu_to_le32(FTGMAC030_TXDES0_TXDMA_OWN)))
			break;

		if (tx_desc->txdes0 & cpu_to_le32(FTGMAC030_TXDES0_LTS)) {

			total_tx_packets += buffer_info->segs;
			total_tx_bytes += buffer_info->bytecount;

			if (buffer_info->skb) {
				bytes_compl += buffer_info->skb->len;
				pkts_compl++;

				ftgmac030_tx_hwtstamp(ctrl, buffer_info->skb,
						      status);
			}
		}

		ftgmac030_put_txbuf(tx_ring, buffer_info, false);
		// Clear all except end of ring bit.
		tx_desc->txdes0 &= cpu_to_le32(FTGMAC030_TXDES0_EDOTR);
		tx_desc->txdes1 = 0;
		tx_desc->txdes2 = 0;
		tx_desc->txdes3 = 0;

		count++;

		i++;
		if (i == tx_ring->count)
			i = 0;
	}

	tx_ring->next_to_clean = i;

	spin_unlock(&tx_ring->ntu_lock);

	netdev_completed_queue(netdev, pkts_compl, bytes_compl);

#define TX_WAKE_THRESHOLD 32
	if (count && netif_carrier_ok(netdev) &&
	    ftgmac030_desc_unused(tx_ring) >= TX_WAKE_THRESHOLD) {
		// Make sure that anybody stopping the queue after this
		// sees the new next_to_clean.
		smp_mb();

		if (netif_queue_stopped(netdev) &&
		    !(test_bit(__FTGMAC030_DOWN, &ctrl->state))) {
			netif_wake_queue(netdev);
			++ctrl->restart_queue;
		}
	}

	// Detect a transmit hang in hardware, this serializes the
	// check with the clearing of time_stamp and movement of i
	maccr = ior32(FTGMAC030_REG_MACCR);
	if (tx_ring->buffer_info[i].time_stamp &&
	    time_after(jiffies, tx_ring->buffer_info[i].time_stamp
		       + (ctrl->tx_timeout_factor * HZ)) &&
	    (maccr & FTGMAC030_MACCR_TXMAC_EN))
		schedule_work(&ctrl->print_hang_task);

	ctrl->total_tx_bytes += total_tx_bytes;
	ctrl->total_tx_packets += total_tx_packets;

	netdev->stats.tx_bytes += total_tx_bytes;
	netdev->stats.tx_packets += total_tx_packets;

	return count < tx_ring->count;
}

static int ftgmac030_maybe_stop_tx(struct ftgmac030_ring *tx_ring, int size)
{
	struct ftgmac030_ctrl *ctrl;

	WARN_ON(size > tx_ring->count);

	if (ftgmac030_desc_unused(tx_ring) >= size)
		return 0;

	ctrl = tx_ring->hw;
	netif_stop_queue(ctrl->netdev);

	/* Restart queue if needed */
	smp_mb();

	if (ftgmac030_desc_unused(tx_ring) < size)
		return -EBUSY;

	netif_start_queue(ctrl->netdev);
	++ctrl->restart_queue;

	return 0;
}

static int ftgmac030_tx_map(struct ftgmac030_ring *tx_ring, struct sk_buff *skb,
			    unsigned int max_per_txd, int first)
{
	struct ftgmac030_ctrl *ctrl = tx_ring->hw;
	struct platform_device *pdev = ctrl->pdev;
	struct ftgmac030_buffer *buffer_info;
	unsigned int len = skb_headlen(skb);
	unsigned int offset = 0, size, count = 0, i;
	unsigned int f, bytecount, segs;
	struct ftgmac030_txdes *first_desc = NULL, *tx_desc = NULL;
	u32 txdes1 = FTGMAC030_TXDES1_TXIC;
	__be16 protocol;

	if (skb->ip_summed == CHECKSUM_PARTIAL) {
		if (skb->protocol == cpu_to_be16(ETH_P_8021Q))
			protocol = vlan_eth_hdr(skb)->h_vlan_encapsulated_proto;
		else
			protocol = skb->protocol;

		switch (protocol) {
		case cpu_to_be16(ETH_P_IP):
			txdes1 |= FTGMAC030_TXDES1_IP_CHKSUM;
			if (ip_hdr(skb)->protocol == IPPROTO_TCP)
				txdes1 |= FTGMAC030_TXDES1_TCP_CHKSUM;
			else if  (ip_hdr(skb)->protocol == IPPROTO_UDP)
				txdes1 |= FTGMAC030_TXDES1_UDP_CHKSUM;
			break;
		case cpu_to_be16(ETH_P_IPV6):
			txdes1 |= FTGMAC030_TXDES1_IPV6_PKT;
			if (ipv6_hdr(skb)->nexthdr == IPPROTO_TCP)
				txdes1 |= FTGMAC030_TXDES1_TCP_CHKSUM;
			else if (ipv6_hdr(skb)->nexthdr == IPPROTO_UDP)
				txdes1 |= FTGMAC030_TXDES1_UDP_CHKSUM;
			break;
		default:
			if (unlikely(net_ratelimit()))
				e_warn(tx_queued, "checksum_partial proto=%x!\n",
				       be16_to_cpu(protocol));
			break;
		}
	}

	if (skb_vlan_tag_present(skb)) {
		txdes1 |= FTGMAC030_TXDES1_INS_VLANTAG;
		txdes1 |= FTGMAC030_TXDES1_VLANTAG_CI(skb_vlan_tag_get(skb));
	}

	i = first;

	while (len) {
		buffer_info = &tx_ring->buffer_info[i];
		size = min(len, max_per_txd);

		buffer_info->length = size;
		buffer_info->time_stamp = jiffies;
		buffer_info->next_to_watch = i;
		buffer_info->dma = dma_map_single(&pdev->dev,
						  skb->data + offset,
						  size, DMA_TO_DEVICE);
		buffer_info->mapped_as_page = false;
		if (dma_mapping_error(&pdev->dev, buffer_info->dma))
			goto dma_error;

		if (i != first) {
			tx_desc = FTGMAC030_TX_DESC(*tx_ring, i);
			tx_desc->txdes0 |= cpu_to_le32(FTGMAC030_TXDES0_TXDMA_OWN|
						       FTGMAC030_TXDES0_TXBUF_SIZE
						       (buffer_info->length));
			tx_desc->txdes1 = cpu_to_le32(txdes1);
			tx_desc->txdes2 = cpu_to_le32((size_t)skb);
			tx_desc->txdes3 = cpu_to_le32(buffer_info->dma);
		}

		len -= size;
		offset += size;
		count++;

		if (len) {
			i++;
			if (i == tx_ring->count)
				i = 0;
		}
	}

	for (f = 0; f < skb_shinfo(skb)->nr_frags; f++) {
		const skb_frag_t *frag;

		frag = &skb_shinfo(skb)->frags[f];
		len = skb_frag_size(frag);
		offset = 0;

		while (len) {
			i++;
			if (i == tx_ring->count)
				i = 0;

			buffer_info = &tx_ring->buffer_info[i];
			size = min(len, max_per_txd);

			buffer_info->length = size;
			buffer_info->time_stamp = jiffies;
			buffer_info->next_to_watch = i;
			buffer_info->dma = skb_frag_dma_map(&pdev->dev, frag,
							    offset, size,
							    DMA_TO_DEVICE);
			buffer_info->mapped_as_page = true;
			if (dma_mapping_error(&pdev->dev, buffer_info->dma))
				goto dma_error;

			if (i != first) {
				tx_desc = FTGMAC030_TX_DESC(*tx_ring, i);
				tx_desc->txdes0 |= cpu_to_le32(FTGMAC030_TXDES0_TXDMA_OWN|
							       FTGMAC030_TXDES0_TXBUF_SIZE
							       (buffer_info->length));
				tx_desc->txdes1 = cpu_to_le32(txdes1);
				tx_desc->txdes2 = cpu_to_le32((size_t)skb);
				tx_desc->txdes3 = cpu_to_le32(buffer_info->dma);
			}

			len -= size;
			offset += size;
			count++;
		}
	}

	segs = skb_shinfo(skb)->gso_segs ? : 1;
	// Multiply data chunks by size of headers
	bytecount = ((segs - 1) * skb_headlen(skb)) + skb->len;

	tx_ring->buffer_info[i].skb = skb;
	tx_ring->buffer_info[i].segs = segs;
	tx_ring->buffer_info[i].bytecount = bytecount;
	tx_ring->buffer_info[first].next_to_watch = i;

	first_desc = FTGMAC030_TX_DESC(*tx_ring, first);
	buffer_info = &tx_ring->buffer_info[first];

	first_desc->txdes1 = cpu_to_le32(txdes1);
	first_desc->txdes2 = cpu_to_le32((size_t)skb);
	first_desc->txdes3 = cpu_to_le32(buffer_info->dma);

	/* Order the previous packet and descriptor udpates
	 * before setting the OWN bit on the first descriptor.
	 */
	dma_wmb();

	if (tx_desc) {
		tx_desc->txdes0 |= cpu_to_le32(FTGMAC030_TXDES0_LTS);

		/* Order the tx_desc LST bit udpates
		 * before setting the OWN bit on the first descriptor.
		 */
		dma_wmb();

		first_desc->txdes0 |= cpu_to_le32(FTGMAC030_TXDES0_TXDMA_OWN|
						  FTGMAC030_TXDES0_FTS|
						  FTGMAC030_TXDES0_TXBUF_SIZE
						  (buffer_info->length));
	} else {
		first_desc->txdes0 |= cpu_to_le32(FTGMAC030_TXDES0_TXDMA_OWN|
						  FTGMAC030_TXDES0_FTS|
						  FTGMAC030_TXDES0_LTS|
						  FTGMAC030_TXDES0_TXBUF_SIZE
						  (buffer_info->length));
	}

	// Force memory writes to complete before letting
	// HW know there are new descriptors to fetch.
	wmb();

	return count;

dma_error:
	dev_err(&pdev->dev, "Tx DMA map failed\n");
	buffer_info->dma = 0;
	if (count)
		count--;

	while (count--) {
		if (i == 0)
			i += tx_ring->count;
		i--;
		buffer_info = &tx_ring->buffer_info[i];
		ftgmac030_put_txbuf(tx_ring, buffer_info, true);
	}

	return 0;
}

static netdev_tx_t ftgmac030_xmit_frame(struct sk_buff *skb,
					struct net_device *netdev)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);
	struct ftgmac030_ring *tx_ring = ctrl->tx_ring;
	unsigned int first, len, nr_frags;
	int count;
	unsigned int f;
	unsigned long flags;

	// Do not support GSO
	WARN_ON(skb_is_gso(skb));

	if (test_bit(__FTGMAC030_DOWN, &ctrl->state)) {
		dev_kfree_skb_any(skb);
		return NETDEV_TX_OK;
	}

	if (skb->len <= 0) {
		dev_kfree_skb_any(skb);
		return NETDEV_TX_OK;
	}

	// If skb is linear, the length of skb->data is skb->len.
	// If skb is not linear (i.e., skb->data_len != 0), the length
	// of skb->data is (skb->len) - (skb->data_len) for the head ONLY.
	//
	// The minimum packet size is 60 bytes not include CRC so
	// pad skb in order to meet this minimum size requirement.
	if (unlikely(skb->len < ETH_ZLEN)) {
		if (skb_pad(skb, ETH_ZLEN - skb->len))
			return NETDEV_TX_OK;
		skb->len = ETH_ZLEN;
		skb_set_tail_pointer(skb, ETH_ZLEN);
	}

	len = skb_headlen(skb);

	count = DIV_ROUND_UP(len, ctrl->tx_fifo_limit);

	nr_frags = skb_shinfo(skb)->nr_frags;
	for (f = 0; f < nr_frags; f++) {
		count += DIV_ROUND_UP(skb_frag_size(&skb_shinfo(skb)->frags[f]),
				      ctrl->tx_fifo_limit);
	}

	if (ftgmac030_maybe_stop_tx(tx_ring, count))
		return NETDEV_TX_BUSY;


	// FTGMAC030 can not disable append CRC per packet
	if (unlikely(skb->no_fcs)) {
		u32 maccr;

		maccr = ior32(FTGMAC030_REG_MACCR);
		maccr &= ~FTGMAC030_MACCR_CRC_APD;
		iow32(FTGMAC030_REG_MACCR, maccr);
	}

	first = tx_ring->next_to_use;

	// if count is 0 then mapping error has occurred.
	count = ftgmac030_tx_map(tx_ring, skb, ctrl->tx_fifo_limit, first);
	if (count) {
		if (unlikely((skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP) &&
			     !ctrl->tx_hwtstamp_skb)) {
			skb_shinfo(skb)->tx_flags |= SKBTX_IN_PROGRESS;
			ctrl->tx_hwtstamp_skb = skb_get(skb);
			ctrl->tx_hwtstamp_start = jiffies;
		} else {
			skb_tx_timestamp(skb);
		}

		// Increment dql->num_queued before start DMA to transfer,
		// Otherwise, it could be already transmit completed before
		// we add dql->num_queued and cause BUG_ON condition met at
		// dql_completed function(lib/dynamic_queue_limits.c).
		netdev_sent_queue(ctrl->netdev, skb->len);

		spin_lock_irqsave(&tx_ring->ntu_lock, flags);

		tx_ring->next_to_use += count;
		if (tx_ring->next_to_use  >= tx_ring->count)
			tx_ring->next_to_use -= tx_ring->count;

		writel(1, tx_ring->tail);

		spin_unlock_irqrestore(&tx_ring->ntu_lock, flags);

		ftgmac030_maybe_stop_tx(tx_ring,
					(MAX_SKB_FRAGS *
					 DIV_ROUND_UP(PAGE_SIZE,
						      ctrl->tx_fifo_limit) + 2));
	} else {
		dev_kfree_skb_any(skb);
		tx_ring->buffer_info[first].time_stamp = 0;
		tx_ring->next_to_use = first;
	}

	return NETDEV_TX_OK;
}

#ifndef FTGMAC030_2BYTE_DMA_ALIGN
static void skb_align(struct sk_buff *skb, int align)
{
	int off = ((unsigned long)skb->data) & (align - 1);

	if (off)
		skb_reserve(skb, align - off);
}
#endif

/**
 * ftgmac030_alloc_rx_buffers - Replace used receive buffers
 * @rx_ring: Rx descriptor ring
 */
static void ftgmac030_alloc_rx_buffers(struct ftgmac030_ring *rx_ring,
				       int cleaned_count, gfp_t gfp)
{
	struct ftgmac030_ctrl *ctrl = rx_ring->hw;
	struct net_device *netdev = ctrl->netdev;
	struct platform_device *pdev = ctrl->pdev;
	struct ftgmac030_rxdes *rx_desc;
	struct ftgmac030_buffer *buffer_info;
	struct sk_buff *skb;
	unsigned int i;
	unsigned int bufsz = ctrl->rx_buffer_len;

	if (gfp == GFP_ATOMIC)
		spin_lock(&rx_ring->ntu_lock);

	i = rx_ring->next_to_use;
	buffer_info = &rx_ring->buffer_info[i];

	while (cleaned_count--) {
		skb = buffer_info->skb;
		if (skb) {
			skb_trim(skb, 0);
			goto map_skb;
		}

#ifdef FTGMAC030_2BYTE_DMA_ALIGN
		skb = __netdev_alloc_skb_ip_align(netdev, bufsz, gfp);
#else
		// FTGMAC030 can not support 2-bytes alignment DMA
		skb = __netdev_alloc_skb(netdev, bufsz, gfp);
#endif
		if (!skb) {
			/* Better luck next round */
			ctrl->alloc_rx_buff_failed++;
			break;
		}

#ifndef FTGMAC030_2BYTE_DMA_ALIGN
		// HW requires at least 8-bytes alignment
		skb_align(skb, 8);
#endif
		buffer_info->skb = skb;
map_skb:
		buffer_info->dma = dma_map_single(&pdev->dev, skb->data,
						  ctrl->rx_buffer_len,
						  DMA_FROM_DEVICE);
		if (dma_mapping_error(&pdev->dev, buffer_info->dma)) {
			dev_err(&pdev->dev, "Rx DMA map failed\n");
			ctrl->rx_dma_failed++;
			break;
		}

		if (gfp != GFP_ATOMIC)
			spin_lock(&rx_ring->ntu_lock);

		rx_desc = FTGMAC030_RX_DESC(*rx_ring, i);
		rx_desc->rxdes1 = 0;
		// rxdes2 is not used by hardware. We use it to keep track of skb.
		rx_desc->rxdes2 = cpu_to_le32((size_t) skb->data);
		rx_desc->rxdes3 = cpu_to_le32(buffer_info->dma);

		if (gfp != GFP_ATOMIC)
			spin_unlock(&rx_ring->ntu_lock);

		i++;
		// Set bit 31 RXPKT_RDY to 0, pass owner to hw and set
		// bit 15 EDORR if it is last descriptor
		if (unlikely(i == rx_ring->count)) {
			i = 0;
			rx_desc->rxdes0 = cpu_to_le32(FTGMAC030_RXDES0_EDORR);
		} else {
			rx_desc->rxdes0 = 0;
		}

		buffer_info = &rx_ring->buffer_info[i];
	}

	rx_ring->next_to_use = i;

	if (gfp == GFP_ATOMIC)
		spin_unlock(&rx_ring->ntu_lock);
}

/**
 * ftgmac030_alloc_jumbo_rx_buffers - Replace used jumbo receive buffers
 * @rx_ring: Rx descriptor ring
 * @cleaned_count: number of buffers to allocate this pass
 */
static void ftgmac030_alloc_jumbo_rx_buffers(struct ftgmac030_ring *rx_ring,
					     int cleaned_count, gfp_t gfp)
{
	struct ftgmac030_ctrl *ctrl = rx_ring->hw;
	struct net_device *netdev = ctrl->netdev;
	struct platform_device *pdev = ctrl->pdev;
	struct ftgmac030_rxdes *rx_desc;
	struct ftgmac030_buffer *buffer_info;
	struct sk_buff *skb;
	unsigned int i;
	unsigned int bufsz = 256 - 16;	// for skb_reserve

	if (gfp == GFP_ATOMIC)
		spin_lock(&rx_ring->ntu_lock);

	i = rx_ring->next_to_use;
	buffer_info = &rx_ring->buffer_info[i];

	while (cleaned_count--) {
		skb = buffer_info->skb;
		if (skb) {
			skb_trim(skb, 0);
			goto check_page;
		}

		skb = __netdev_alloc_skb_ip_align(netdev, bufsz, gfp);
		if (unlikely(!skb)) {
			ctrl->alloc_rx_buff_failed++;
			break;
		}

		buffer_info->skb = skb;
check_page:
		if (!buffer_info->page) {
			buffer_info->page = alloc_page(gfp);
			if (unlikely(!buffer_info->page)) {
				ctrl->alloc_rx_buff_failed++;
				break;
			}
		}

		if (!buffer_info->dma) {
			buffer_info->dma = dma_map_page(&pdev->dev,
							buffer_info->page, 0,
							PAGE_SIZE,
							DMA_FROM_DEVICE);
			if (dma_mapping_error(&pdev->dev, buffer_info->dma)) {
				ctrl->alloc_rx_buff_failed++;
				break;
			}
		}

		if (gfp != GFP_ATOMIC)
			spin_lock(&rx_ring->ntu_lock);

		rx_desc = FTGMAC030_RX_DESC(*rx_ring, i);
		rx_desc->rxdes1 = 0;
		// rxdes2 is not used by hardware. We use it to keep track of skb.
		rx_desc->rxdes2 = cpu_to_le32((size_t) skb->data);
		rx_desc->rxdes3 = cpu_to_le32(buffer_info->dma);

		if (gfp != GFP_ATOMIC)
			spin_unlock(&rx_ring->ntu_lock);

		// Set bit 31 RXPKT_RDY to 0, pass owner to hw and set
		// bit 15 EDORR if it is last descriptor
		if (unlikely(++i == rx_ring->count)) {
			i = 0;
			rx_desc->rxdes0 = cpu_to_le32(FTGMAC030_RXDES0_EDORR);
		} else {
			rx_desc->rxdes0 = 0;
		}

		buffer_info = &rx_ring->buffer_info[i];
	}

	rx_ring->next_to_use = i;

	if (gfp == GFP_ATOMIC)
		spin_unlock(&rx_ring->ntu_lock);
}

/**
 * ftgmac030_rx_hwtstamp - utility function which checks for Rx time stamp
 * @ctrl: board private structure
 * @status: descriptor extended error and status field
 * @skb: particular skb to include time stamp
 *
 * If the time stamp is valid, convert it into the timecounter ns value
 * and store that result into the shhwtstamps structure which is passed
 * up the network stack.
 */
static void ftgmac030_rx_hwtstamp(struct ftgmac030_ctrl *ctrl, u8 ptp_type,
				  struct sk_buff *skb)
{
	struct skb_shared_hwtstamps *hwtstamps = skb_hwtstamps(skb);
	u64 rxstmp = 0;

	if (!ptp_type || (ptp_type == FTGMAC030_RXDES1_PTP_NO_TMSTMP))
		return;

	// Rx timestamp registers contain the timestamp value. No other
	// received packet will be timestamped until the Rx timestamp
	// registers are read.  Because only one packet can be timestamped
	// at a time, the register values must belong to this packet and
	// therefore none of the other additional attributes need to be
	// compared.
	if (ptp_type == FTGMAC030_RXDES1_PTP_EVENT_TMSTMP) {
		spin_lock(&ctrl->systim_lock);
		rxstmp = (u64)ior32(FTGMAC030_REG_PTP_RX_SEC) *
			 NSEC_PER_SEC;
		rxstmp += ior32(FTGMAC030_REG_PTP_RX_NSEC);
		spin_unlock(&ctrl->systim_lock);
	} else if (ptp_type == FTGMAC030_RXDES1_PTP_PEER_TMSTMP) {
		spin_lock(&ctrl->systim_lock);
		rxstmp = (u64)ior32(FTGMAC030_REG_PTP_RX_P_SEC) *
			 NSEC_PER_SEC;
		rxstmp += ior32(FTGMAC030_REG_PTP_RX_P_NSEC);
		spin_unlock(&ctrl->systim_lock);
	}

	memset(hwtstamps, 0, sizeof(*hwtstamps));
	hwtstamps->hwtstamp = ns_to_ktime(rxstmp);
}

/**
 * ftgmac030_receive_skb - helper function to handle Rx indications
 * @ctrl: board private structure
 * @rxdes1: descriptor contains status field as written by hardware
 * @skb: pointer to sk_buff to be indicated to stack
 */
static void ftgmac030_receive_skb(struct ftgmac030_ctrl *ctrl,
				  struct net_device *netdev,
				  struct sk_buff *skb, u32 rxdes1)
{
	u16 tag = rxdes1 & FTGMAC030_RXDES1_VLANTAG_CI;
	u8 ptp_type = FTGMAC030_RXDES1_PTP_TYPE(rxdes1);

	ftgmac030_rx_hwtstamp(ctrl, ptp_type, skb);

	skb->protocol = eth_type_trans(skb, netdev);

	if (rxdes1 & FTGMAC030_RXDES1_VLANTAG_AVAIL)
		__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q), tag);

	napi_gro_receive(&ctrl->napi, skb);
}

/**
 * ftgmac030_rx_checksum - Receive Checksum Offload
 * @ctrl: board private structure
 * @status_err: receive descriptor status and error fields
 * @csum: receive descriptor csum field
 * @sk_buff: socket buffer with received data
 */
static void ftgmac030_rx_checksum(struct ftgmac030_ctrl *ctrl, u32 status_err,
				  struct sk_buff *skb)
{
	u32 proto = (status_err & FTGMAC030_RXDES1_PROT_MASK);

	skb_checksum_none_assert(skb);

	// Rx checksum disabled
	if (!(ctrl->netdev->features & NETIF_F_RXCSUM))
		return;

	if (((proto == FTGMAC030_RXDES1_PROT_TCPIP) &&
	     !(status_err & FTGMAC030_RXDES1_TCP_CHKSUM_ERR)) ||
	     ((proto == FTGMAC030_RXDES1_PROT_UDPIP) &&
	     !(status_err & FTGMAC030_RXDES1_UDP_CHKSUM_ERR))) {
		// TCP or UDP packet with a valid checksum
		skb->ip_summed = CHECKSUM_UNNECESSARY;
		ctrl->hw_csum_good++;
	} else {
		// Let the stack verify checksum errors
		ctrl->hw_csum_err++;
	}
}

static bool ftgmac030_rx_packet_error(struct ftgmac030_ctrl *ctrl,
				      struct ftgmac030_rxdes *rxdes)
{
	struct net_device *netdev = ctrl->netdev;
	bool error = false;

	if (unlikely(rxdes->rxdes0 &
		     cpu_to_le32(FTGMAC030_RXDES0_RX_ERR))) {
		if (net_ratelimit())
			e_info(rx_err, "rx err\n");

		netdev->stats.rx_errors++;
		error = true;
	}

	if (unlikely(rxdes->rxdes0 &
		     cpu_to_le32(FTGMAC030_RXDES0_CRC_ERR))) {
		if (net_ratelimit())
			e_info(rx_err, "rx crc err\n");

		netdev->stats.rx_crc_errors++;
		error = true;
	} else if (unlikely(rxdes->rxdes1 &
		   cpu_to_le32(FTGMAC030_RXDES1_IP_CHKSUM_ERR))) {
		if (net_ratelimit())
			e_info(rx_err, "rx IP checksum err\n");

		error = true;
	}

	if (unlikely(rxdes->rxdes0 & cpu_to_le32(FTGMAC030_RXDES0_FTL))) {
		if (net_ratelimit())
			e_info(rx_err, "rx frame too long\n");

		netdev->stats.rx_length_errors++;
		error = true;
	} else if (unlikely(rxdes->rxdes0 &
		   cpu_to_le32(FTGMAC030_RXDES0_RUNT))) {
		if (net_ratelimit())
			e_info(rx_err, "rx runt\n");

		netdev->stats.rx_length_errors++;
		error = true;
	} else if (unlikely(rxdes->rxdes0 &
		   cpu_to_le32(FTGMAC030_RXDES0_RX_ODD_NB))) {
		if (net_ratelimit())
			e_info(rx_err, "rx odd nibble\n");

		netdev->stats.rx_length_errors++;
		error = true;
	}

	return error;
}

/**
 * ftgmac030_clean_rx_irq - Send received data up the network stack
 * @rx_ring: Rx descriptor ring
 */
static bool ftgmac030_clean_rx_irq(struct ftgmac030_ring *rx_ring, int *work_done,
				   int work_to_do)
{
	struct ftgmac030_ctrl *ctrl = rx_ring->hw;
	struct net_device *netdev = ctrl->netdev;
	struct platform_device *pdev = ctrl->pdev;
	struct ftgmac030_buffer *buffer_info, *next_buffer;
	struct ftgmac030_rxdes *rx_desc, *next_rxd;
	u32 length, staterr;
	unsigned int i;
	int cleaned_count = 0;
	int cleaned = 0;
	unsigned int total_rx_bytes = 0, total_rx_packets = 0;

	spin_lock(&rx_ring->ntc_lock);

	i = rx_ring->next_to_clean;
	rx_desc = FTGMAC030_RX_DESC(*rx_ring, i);
	staterr = le32_to_cpu(rx_desc->rxdes0);
	buffer_info = &rx_ring->buffer_info[i];

	while (staterr & FTGMAC030_RXDES0_RXPKT_RDY) {
		struct sk_buff *skb;

		if (*work_done >= work_to_do)
			break;
		(*work_done)++;

		/* Order subsequent reads with the rx desc */
		rmb();

		skb = buffer_info->skb;
		buffer_info->skb = NULL;

		prefetch(skb->data);

		i++;
		if (i == rx_ring->count)
			i = 0;
		next_rxd = FTGMAC030_RX_DESC(*rx_ring, i);
		prefetch(next_rxd);

		next_buffer = &rx_ring->buffer_info[i];

		cleaned = 1;
		cleaned_count++;
		dma_unmap_single(&pdev->dev, buffer_info->dma,
				 ctrl->rx_buffer_len, DMA_FROM_DEVICE);
		buffer_info->dma = 0;

		length = staterr & FTGMAC030_RXDES0_VDBC;

		// !FRS and !LRS means multiple descriptors were used to
		// store a single packet, if that's the case we need to
		// toss it. Go to next frame until we find FRS and LRS
		// bit set, as it is by definition one packet in one
		// descriptor entry.
		if (unlikely((staterr & FTGMAC030_RXDES0_EOP)
			     != FTGMAC030_RXDES0_EOP)) {
			// All receives must fit into a single descriptor.
			e_dbg(rx_err, "Receive packet consumed multiple buffers\n");
			// recycle
			buffer_info->skb = skb;
			goto next_desc;
		}

		if (unlikely(rx_desc->rxdes0 &
			     cpu_to_le32(FTGMAC030_RXDES0_FIFO_FULL))) {
			if (net_ratelimit())
				e_info(rx_err, "rx fifo full\n");

			if ((ctrl->phy_speed == SPEED_1000) &&
			    (ctrl->phy_duplex == DUPLEX_HALF)) {
				buffer_info->skb = skb;
				rx_ring->next_to_clean = i;
				spin_unlock(&rx_ring->ntc_lock);
				return -EIO;
			}

		}

		if (unlikely(ftgmac030_rx_packet_error(ctrl, rx_desc) &&
			     !(netdev->features & NETIF_F_RXALL))) {
			/* recycle */
			buffer_info->skb = skb;
			goto next_desc;
		}
		// FTGMAC030 always include 4 bytes CRC in length.
		// Adjust length to remove Ethernet CRC.
		//
		// If configured to store CRC, don't subtract FCS,
		// but keep the FCS bytes out of the total_rx_bytes
		// counter.
		if (netdev->features & NETIF_F_RXFCS)
			total_rx_bytes -= 4;
		else
			length -= 4;

		total_rx_bytes += length;
		total_rx_packets++;

		if (length < copyBreak) {
			struct sk_buff *new_skb =
					netdev_alloc_skb_ip_align(netdev, length);
		// code added for copyBreak, this should improve
		// performance for small packets with large amounts
		// of reassembly being done in the stack
			if (new_skb) {
#ifdef FTGMAC030_2BYTE_DMA_ALIGN
				skb_copy_to_linear_data_offset(new_skb,
							       -NET_IP_ALIGN,
							       (skb->data -
								NET_IP_ALIGN),
							       (length +
								NET_IP_ALIGN));
#else
		// Modification due to HW restriction can not
		// do 2-bytes alignment DMA. Hopefully HW support
		// this in the future, we can use below code.
				skb_copy_to_linear_data(new_skb, skb->data, length);

#endif
				/* save the skb in buffer_info as good */
				buffer_info->skb = skb;
				skb = new_skb;
			}
		}

		// End copyBreak code
		skb_put(skb, length);

		// Receive Checksum Offload
		ftgmac030_rx_checksum(ctrl, le32_to_cpu(rx_desc->rxdes1), skb);

		ftgmac030_receive_skb(ctrl, netdev, skb,
				      le32_to_cpu(rx_desc->rxdes1));
next_desc:
		if (cleaned_count >= FTGMAC030_RX_BUFFER_WRITE) {
			ctrl->alloc_rx_buf(rx_ring, cleaned_count, GFP_ATOMIC);
			cleaned_count = 0;
		}

		// Use prefetched values
		rx_desc = next_rxd;
		buffer_info = next_buffer;

		staterr = le32_to_cpu(rx_desc->rxdes0);
	}
	rx_ring->next_to_clean = i;

	spin_unlock(&rx_ring->ntc_lock);

	if (cleaned_count)
		ctrl->alloc_rx_buf(rx_ring, cleaned_count, GFP_ATOMIC);

	netdev->stats.rx_bytes += total_rx_bytes;
	netdev->stats.rx_packets += total_rx_packets;

	ctrl->total_rx_bytes += total_rx_bytes;
	ctrl->total_rx_packets += total_rx_packets;
	return cleaned;
}

/**
 * ftgmac030_consume_page - helper function
 */
static void ftgmac030_consume_page(struct ftgmac030_buffer *bi,
				   struct sk_buff *skb, u16 length)
{
	bi->page = NULL;
	skb->len += length;
	skb->data_len += length;
	skb->truesize += PAGE_SIZE;
}

/**
 * ftgmac030_clean_jumbo_rx_irq - Send received data up the network stack
 * @ctrl: board private structure
 */
static bool ftgmac030_clean_jumbo_rx_irq(struct ftgmac030_ring *rx_ring,
					 int *work_done, int work_to_do)
{
	struct ftgmac030_ctrl *ctrl = rx_ring->hw;
	struct net_device *netdev = ctrl->netdev;
	struct platform_device *pdev = ctrl->pdev;
	struct ftgmac030_rxdes *rx_desc, *next_rxd;
	struct ftgmac030_buffer *buffer_info, *next_buffer;
	u32 length, staterr;
	unsigned int i;
	int cleaned_count = 0;
	int cleaned = 0;
	unsigned int total_rx_bytes = 0, total_rx_packets = 0;
	struct skb_shared_info *shinfo;

	spin_lock(&rx_ring->ntc_lock);

	i = rx_ring->next_to_clean;
	rx_desc = FTGMAC030_RX_DESC(*rx_ring, i);
	staterr = le32_to_cpu(rx_desc->rxdes0);
	buffer_info = &rx_ring->buffer_info[i];

	while (staterr & FTGMAC030_RXDES0_RXPKT_RDY) {
		struct sk_buff *skb;

		if (*work_done >= work_to_do)
			break;
		(*work_done)++;

		/* Order subsequent reads with the rx desc */
		rmb();

		skb = buffer_info->skb;
		buffer_info->skb = NULL;

		++i;
		if (i == rx_ring->count)
			i = 0;
		next_rxd = FTGMAC030_RX_DESC(*rx_ring, i);
		prefetch(next_rxd);

		next_buffer = &rx_ring->buffer_info[i];

		cleaned = 1;
		cleaned_count++;
		dma_unmap_page(&pdev->dev, buffer_info->dma, PAGE_SIZE,
			       DMA_FROM_DEVICE);
		buffer_info->dma = 0;

		length = staterr & FTGMAC030_RXDES0_VDBC;

		if (unlikely(staterr & FTGMAC030_RXDES0_FIFO_FULL)) {
			if (net_ratelimit())
				e_info(rx_err, "rx fifo full.\n");

			if ((ctrl->phy_speed == SPEED_1000) &&
			    (ctrl->phy_duplex == DUPLEX_HALF)) {
				rx_ring->next_to_clean = i;
				buffer_info->skb = skb;
				if (rx_ring->rx_skb_top)
					dev_kfree_skb_irq(rx_ring->rx_skb_top);
				rx_ring->rx_skb_top = NULL;
				spin_unlock(&rx_ring->ntc_lock);
				return -EIO;
			}

		}

		// Errors value are only valid at FRS(first) descriptor.
		if (unlikely((staterr & FTGMAC030_RXDES0_FRS) &&
			     (ftgmac030_rx_packet_error(ctrl, rx_desc) &&
			      !(netdev->features & NETIF_F_RXALL)))) {
			// Recycle both page and skb
			buffer_info->skb = skb;

			if (rx_ring->rx_skb_top)
				dev_kfree_skb_irq(rx_ring->rx_skb_top);
			rx_ring->rx_skb_top = NULL;
			goto next_desc;
		}

		// Receive Checksum Offload only valid for FRS descriptor
		if (staterr & FTGMAC030_RXDES0_FRS)
			ftgmac030_rx_checksum(ctrl, le32_to_cpu(rx_desc->rxdes1),
					      skb);
#define rxtop (rx_ring->rx_skb_top)
		if (!(staterr & FTGMAC030_RXDES0_LRS)) {
			// This descriptor is the beginning (or middle)
			if (!rxtop) {
				// FRS must be one for beginning of a chain
				WARN_ON(!(staterr & FTGMAC030_RXDES0_FRS));

				rxtop = skb;
				skb_fill_page_desc(rxtop, 0, buffer_info->page,
						   0, length);
			} else {
				// Middle of a chain
				shinfo = skb_shinfo(rxtop);
				skb_fill_page_desc(rxtop, shinfo->nr_frags,
						   buffer_info->page, 0,
						   length);
				// Reuse the skb, only consumed the page
				buffer_info->skb = skb;
			}
			ftgmac030_consume_page(buffer_info, rxtop, length);
			goto next_desc;
		} else {
			if (rxtop) {
				// End of the chain
				shinfo = skb_shinfo(rxtop);
				skb_fill_page_desc(rxtop, shinfo->nr_frags,
						   buffer_info->page, 0,
						   length);
				// Reuse the current skb, we only consumed the page
				buffer_info->skb = skb;
				skb = rxtop;
				rxtop = NULL;
				ftgmac030_consume_page(buffer_info, skb, length);
			} else {
				// No chain, FRS and LRS is one, this buf is the
				// packet copyBreak to save the put_page/alloc_page
				if (length <= copyBreak &&
				    skb_tailroom(skb) >= length) {
					u8 *vaddr;

					vaddr = kmap_atomic(buffer_info->page);
					memcpy(skb_tail_pointer(skb), vaddr,
					       length);
					kunmap_atomic(vaddr);
					// Reuse the page, so don't erase
					// buffer_info->page.
					skb_put(skb, length);
				} else {
					skb_fill_page_desc(skb, 0,
							   buffer_info->page, 0,
							   length);
					ftgmac030_consume_page(buffer_info, skb,
							   length);
				}
			}
		}

		// Probably a little skewed due to removing CRC
		total_rx_bytes += skb->len;
		total_rx_packets++;

		// eth type trans needs skb->data to point to something.
		if (!pskb_may_pull(skb, ETH_HLEN)) {
			e_err(tx_err, "pskb_may_pull failed.\n");
			dev_kfree_skb_irq(skb);
			goto next_desc;
		}

		ftgmac030_receive_skb(ctrl, netdev, skb,
				      le32_to_cpu(rx_desc->rxdes1));
next_desc:
		if (unlikely(cleaned_count >= FTGMAC030_RX_BUFFER_WRITE)) {
			ctrl->alloc_rx_buf(rx_ring, cleaned_count, GFP_ATOMIC);
			cleaned_count = 0;
		}

		// Use prefetched values
		rx_desc = next_rxd;
		buffer_info = next_buffer;

		staterr = le32_to_cpu(rx_desc->rxdes0);
	}
	rx_ring->next_to_clean = i;

	spin_unlock(&rx_ring->ntc_lock);

	if (cleaned_count)
		ctrl->alloc_rx_buf(rx_ring, cleaned_count, GFP_ATOMIC);

	netdev->stats.rx_bytes += total_rx_bytes;
	netdev->stats.rx_packets += total_rx_packets;

	ctrl->total_rx_bytes += total_rx_bytes;
	ctrl->total_rx_packets += total_rx_packets;
	return cleaned;
}

/**
 * ftgmac030_update_itr - update the dynamic ITR value based on statistics
 * @itr_setting: current ctrl->itr
 * @packets: the number of packets during this measurement interval
 * @bytes: the number of bytes during this measurement interval
 *
 *      Stores a new ITR value based on packets and byte
 *      counts during the last interrupt.  The advantage of per interrupt
 *      computation is faster updates and more accurate ITR for the current
 *      traffic pattern.  Constants in this function were computed
 *      based on theoretical maximum wire speed and thresholds were set based
 *      on testing data as well as attempting to minimize response time
 *      while increasing bulk throughput.  This functionality is controlled
 *      by the InterruptThrottleRate module parameter.
 */
static unsigned int ftgmac030_update_itr(u16 itr_setting, int packets, int bytes)
{
	unsigned int retval = itr_setting;

	if (packets == 0)
		return itr_setting;

	switch (itr_setting) {
	case lowest_latency:
		// Handle jumbo frames
		if (bytes / packets > 8000)
			retval = bulk_latency;
		else if ((packets < 5) && (bytes > 512))
			retval = low_latency;
		break;
	case low_latency:
		if (bytes > 10000) {
			if (bytes / packets > 8000)
				retval = bulk_latency;
			else if ((packets < 10) || ((bytes / packets) > 1200))
				retval = bulk_latency;
			else if ((packets > 35))
				retval = lowest_latency;
		} else if (bytes / packets > 2000) {
			retval = bulk_latency;
		} else if (packets <= 2 && bytes < 512) {
			retval = lowest_latency;
		}
		break;
	case bulk_latency:
		if (bytes > 25000) {
			if (packets > 35)
				retval = low_latency;
		} else if (bytes < 6000) {
			retval = low_latency;
		}
		break;
	}

	return retval;
}

static void ftgmac030_set_itr(struct ftgmac030_ctrl *ctrl)
{
	u16 current_itr;
	u32 new_itr = ctrl->itr;

	// Non-gigabit speeds, just fix the interrupt rate at 4000.
	if (ctrl->phy_speed != SPEED_1000) {
		current_itr = 0;
		new_itr = 4000;
		goto set_itr_now;
	}

	if (ctrl->flags & FLAG_DISABLE_AIM) {
		new_itr = 0;
		goto set_itr_now;
	}

	ctrl->tx_itr = ftgmac030_update_itr(ctrl->tx_itr,
					  ctrl->total_tx_packets,
					  ctrl->total_tx_bytes);
	// Conservative mode (itr 3) eliminates the lowest_latency setting
	if (ctrl->itr_setting == 3 && ctrl->tx_itr == lowest_latency)
		ctrl->tx_itr = low_latency;

	ctrl->rx_itr = ftgmac030_update_itr(ctrl->rx_itr,
					  ctrl->total_rx_packets,
					  ctrl->total_rx_bytes);
	if (ctrl->itr_setting == 3 && ctrl->rx_itr == lowest_latency)
		ctrl->rx_itr = low_latency;

	current_itr = max(ctrl->rx_itr, ctrl->tx_itr);

	switch (current_itr) {
	case lowest_latency:
		new_itr = 70000;
		break;
	case low_latency:	// 50 usec aka 20000 int/s
		new_itr = 20000;
		break;
	case bulk_latency:	// 250 usec aka 4000 int/s
		new_itr = 4000;
		break;
	default:
		break;
	}

set_itr_now:
	if (new_itr != ctrl->itr) {
		// This attempts to bias the interrupt rate towards Bulk
		// by adding intermediate steps when interrupt rate is
		// increasing.
		new_itr = new_itr > ctrl->itr ?
		    min(ctrl->itr + (new_itr >> 2), new_itr) : new_itr;
		ctrl->itr = new_itr;
		ctrl->rx_ring->itr_val = new_itr;
		ftgmac030_write_itr(ctrl, new_itr);
	}
}

/**
 * ftgmac030_write_itr - write the ITR value to the appropriate registers
 * @ctrl: address of board private structure
 * @itr: new ITR value to program
 *
 * ftgmac030_write_itr writes the ITR value into the RX Interrupt Timer
 * Control Register(RXITC)
 */
void ftgmac030_write_itr(struct ftgmac030_ctrl *ctrl, u32 itr)
{
	u32 rxitc = 0;

	// itr is expected number of interrupt happens per second.
	//
	// FTGMAC030 requires to set RX cycle time.
	//
	// When RXITC.RXINT_TIME_SEL set, the RX cycle times are:
	// 1000 Mbps mode => 16.384 µs
	// 100 Mbps mode => 81.92 µs
	// 10 Mbps mode => 819.2 µs
	//
	// See FTGMAC030 datasheet register offset 0x34.
	if (itr) {
		u32 rx_cycle, new_itr;

		// Convert it to nanoseconds to avoid decimal point calculation
		switch (ctrl->phy_speed) {
		default:
		case SPEED_10:
			rx_cycle = 819200;
			break;

		case SPEED_100:
			rx_cycle = 81920;
			break;

		case SPEED_1000:
			rx_cycle = 16384;
			break;
		}

		new_itr = DIV_ROUND_UP(1000000000, (itr * rx_cycle));

		rxitc = FTGMAC030_ITC_TIME_SEL;
		rxitc |= FTGMAC030_ITC_CYCL_CNT(new_itr);
	}

	iow32(FTGMAC030_REG_RXITC, rxitc);
}

/**
 * ftgmac030_irq_disable - Mask off interrupt generation on the NIC
 */
static void ftgmac030_irq_disable(struct ftgmac030_ctrl *ctrl)
{
	iow32(FTGMAC030_REG_IER, 0);

	if (!in_irq())
		synchronize_irq(ctrl->irq);
}

/**
 * ftgmac030_irq_enable - Enable default interrupt generation settings
 */
static void ftgmac030_irq_enable(struct ftgmac030_ctrl *ctrl)
{
	iow32(FTGMAC030_REG_IER, INT_MASK_ALL_ENABLED);
}

/**
 * ftgmac030_poll - NAPI Rx polling callback
 * @napi: struct associated with this polling callback
 * @weight: number of packets driver is allowed to process this poll
 */
static int ftgmac030_poll(struct napi_struct *napi, int weight)
{
	struct ftgmac030_ctrl *ctrl = container_of(napi,
						   struct ftgmac030_ctrl,
						   napi);
	struct net_device *netdev = ctrl->netdev;
	int tx_cleaned = 1, work_done = 0, err;
	u32 status;

	status = ior32(FTGMAC030_REG_ISR);
	iow32(FTGMAC030_REG_ISR, status);

	if (status & (FTGMAC030_INT_RPKT_LOST | FTGMAC030_INT_AHB_ERR)) {
		if (net_ratelimit())
			e_info(rx_err, "[POLL] = 0x%x: %s%s\n", status,
			       status & FTGMAC030_INT_RPKT_LOST ?
			       "RPKT_LOST " : "",
			       status & FTGMAC030_INT_AHB_ERR ?
			       "AHB_ERR " : "");

		if (status & FTGMAC030_INT_NO_RXBUF) {
			// RX buffer unavailable
			netdev->stats.rx_over_errors++;
		}

		if (status & FTGMAC030_INT_RPKT_LOST) {
			// Received packet lost due to RX FIFO full
			netdev->stats.rx_fifo_errors++;
		}
	}

	tx_cleaned = ftgmac030_clean_tx_irq(ctrl->tx_ring, status);

	if (status & (FTGMAC030_INT_RPKT_BUF | FTGMAC030_INT_NO_RXBUF |
		      FTGMAC030_INT_RX_TMSP_VALID)) {
		err = ctrl->clean_rx(ctrl->rx_ring, &work_done, weight);
		if (err < 0) {
			napi_complete(napi);
			schedule_work(&ctrl->reset_task);
			return 0;
		}
	}

	if (!tx_cleaned)
		work_done = weight;

	// If weight not fully consumed, exit the polling mode
	if (work_done < weight) {
		if (ctrl->itr_setting & 3)
			ftgmac030_set_itr(ctrl);

		napi_complete(napi);
		if (!test_bit(__FTGMAC030_DOWN, &ctrl->state))
			ftgmac030_irq_enable(ctrl);
	}

	return work_done;
}

/**
 * ftgmac030_intr - Interrupt Handler
 * @irq: interrupt number
 * @data: pointer to a network interface device structure
 */
static irqreturn_t ftgmac030_intr(int __always_unused irq, void *data)
{
	struct net_device *netdev = data;
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);

	e_dbg(intr, "[ISR] = 0x%x\n", ior32(FTGMAC030_REG_ISR));

	ftgmac030_irq_disable(ctrl);

	if (test_bit(__FTGMAC030_DOWN, &ctrl->state)) {
		e_info(intr, "[ISR] Not ours\n");
		return IRQ_NONE;
	}

	if (napi_schedule_prep(&ctrl->napi)) {
		ctrl->total_tx_bytes = 0;
		ctrl->total_tx_packets = 0;
		ctrl->total_rx_bytes = 0;
		ctrl->total_rx_packets = 0;
		__napi_schedule(&ctrl->napi);
	}

	return IRQ_HANDLED;
}

/**
 * ftgmac030_clean_rx_ring - Free Rx Buffers per Queue
 * @rx_ring: Rx descriptor ring
 */
static void ftgmac030_clean_rx_ring(struct ftgmac030_ring *rx_ring)
{
	struct ftgmac030_ctrl *ctrl = rx_ring->hw;
	struct ftgmac030_buffer *buffer_info;
	struct platform_device *pdev = ctrl->pdev;
	unsigned int i;

	for (i = 0; i < rx_ring->count; i++) {
		buffer_info = &rx_ring->buffer_info[i];
		if (buffer_info->dma) {
			if (ctrl->clean_rx == ftgmac030_clean_rx_irq)
				dma_unmap_single(&pdev->dev, buffer_info->dma,
						 ctrl->rx_buffer_len,
						 DMA_FROM_DEVICE);
			else if (ctrl->clean_rx == ftgmac030_clean_jumbo_rx_irq)
				dma_unmap_page(&pdev->dev, buffer_info->dma,
					       PAGE_SIZE, DMA_FROM_DEVICE);
			buffer_info->dma = 0;
		}

		if (buffer_info->page) {
			put_page(buffer_info->page);
			buffer_info->page = NULL;
		}

		if (buffer_info->skb) {
			dev_kfree_skb(buffer_info->skb);
			buffer_info->skb = NULL;
		}
	}

	// There also may be some cached data from a chained receive
	if (rx_ring->rx_skb_top) {
		dev_kfree_skb(rx_ring->rx_skb_top);
		rx_ring->rx_skb_top = NULL;
	}

	memset(rx_ring->desc, 0, rx_ring->size);

	rx_ring->next_to_clean = 0;
	rx_ring->next_to_use = 0;
}

/**
 * ftgmac030_request_irq - initialize interrupts
 *
 * Attempts to configure interrupts using the best available
 * capabilities of the hardware and kernel.
 */
static int ftgmac030_request_irq(struct ftgmac030_ctrl *ctrl)
{
	struct net_device *netdev = ctrl->netdev;
	int err;

	err = devm_request_irq(&ctrl->pdev->dev, ctrl->irq,
			       ftgmac030_intr, 0, netdev->name, netdev);
	if (err)
		e_err(intr, "Unable to allocate interrupt, Error: %d\n", err);

	return err;
}

static void ftgmac030_free_irq(struct ftgmac030_ctrl *ctrl)
{
	struct net_device *netdev = ctrl->netdev;

	devm_free_irq(&ctrl->pdev->dev, ctrl->irq, netdev);
}

/**
 * ftgmac030_alloc_ring_dma - allocate memory for a ring structure
 */
static int ftgmac030_alloc_ring_dma(struct ftgmac030_ctrl *ctrl,
				    struct ftgmac030_ring *ring)
{
	ring->desc = dma_alloc_coherent(&ctrl->pdev->dev, ring->size, &ring->dma,
					GFP_KERNEL);
	if (!ring->desc)
		return -ENOMEM;

	return 0;
}

/**
 * ftgmac030_setup_tx_resources - allocate Tx resources (Descriptors)
 * @tx_ring: Tx descriptor ring
 *
 * Return 0 on success, negative on failure
 */
int ftgmac030_setup_tx_resources(struct ftgmac030_ring *tx_ring)
{
	struct ftgmac030_ctrl *ctrl = tx_ring->hw;
	int err = -ENOMEM, size;

	size = sizeof(struct ftgmac030_buffer) * tx_ring->count;
	tx_ring->buffer_info = vzalloc(size);
	if (!tx_ring->buffer_info)
		goto err;

	tx_ring->size = tx_ring->count * sizeof(struct ftgmac030_txdes);
	tx_ring->size = ALIGN(tx_ring->size, 4096);

	err = ftgmac030_alloc_ring_dma(ctrl, tx_ring);
	if (err)
		goto err;

	tx_ring->next_to_use = 0;
	tx_ring->next_to_clean = 0;

	spin_lock_init(&tx_ring->ntu_lock);

	return 0;
err:
	vfree(tx_ring->buffer_info);
	e_err(drv, "Unable to allocate memory for the tx descriptor ring\n");
	return err;
}

/**
 * ftgmac030_setup_rx_resources - allocate Rx resources (Descriptors)
 * @rx_ring: Rx descriptor ring
 *
 * Returns 0 on success, negative on failure
 */
int ftgmac030_setup_rx_resources(struct ftgmac030_ring *rx_ring)
{
	struct ftgmac030_ctrl *ctrl = rx_ring->hw;
	int size, err = -ENOMEM;

	size = sizeof(struct ftgmac030_buffer) * rx_ring->count;
	rx_ring->buffer_info = vzalloc(size);
	if (!rx_ring->buffer_info)
		goto err;

	rx_ring->size = rx_ring->count * sizeof(struct ftgmac030_rxdes);
	rx_ring->size = ALIGN(rx_ring->size, 4096);

	err = ftgmac030_alloc_ring_dma(ctrl, rx_ring);
	if (err)
		goto err;

	rx_ring->next_to_clean = 0;
	rx_ring->next_to_use = 0;
	rx_ring->rx_skb_top = NULL;

	spin_lock_init(&rx_ring->ntc_lock);
	spin_lock_init(&rx_ring->ntu_lock);
	return 0;

err:
	vfree(rx_ring->buffer_info);
	e_err(drv, "Unable to allocate memory for the rx descriptor ring\n");
	return err;
}

/**
 * ftgmac030_clean_tx_ring - Free Tx Buffers
 * @tx_ring: Tx descriptor ring
 */
static void ftgmac030_clean_tx_ring(struct ftgmac030_ring *tx_ring)
{
	struct ftgmac030_ctrl *ctrl = tx_ring->hw;
	struct ftgmac030_buffer *buffer_info;
	unsigned long size;
	unsigned int i;

	for (i = 0; i < tx_ring->count; i++) {
		buffer_info = &tx_ring->buffer_info[i];
		ftgmac030_put_txbuf(tx_ring, buffer_info, false);
	}

	netdev_reset_queue(ctrl->netdev);
	size = sizeof(struct ftgmac030_buffer) * tx_ring->count;
	memset(tx_ring->buffer_info, 0, size);

	memset(tx_ring->desc, 0, tx_ring->size);

	tx_ring->next_to_use = 0;
	tx_ring->next_to_clean = 0;
}

/**
 * ftgmac030_free_tx_resources - Free Tx Resources per Queue
 * @tx_ring: Tx descriptor ring
 *
 * Free all transmit software resources
 */
void ftgmac030_free_tx_resources(struct ftgmac030_ring *tx_ring)
{
	struct ftgmac030_ctrl *ctrl = tx_ring->hw;
	struct platform_device *pdev = ctrl->pdev;

	ftgmac030_clean_tx_ring(tx_ring);

	vfree(tx_ring->buffer_info);
	tx_ring->buffer_info = NULL;

	dma_free_coherent(&pdev->dev, tx_ring->size, tx_ring->desc,
			  tx_ring->dma);
	tx_ring->desc = NULL;
}

/**
 * ftgmac030_free_rx_resources - Free Rx Resources
 * @rx_ring: Rx descriptor ring
 *
 * Free all receive software resources
 */
void ftgmac030_free_rx_resources(struct ftgmac030_ring *rx_ring)
{
	struct ftgmac030_ctrl *ctrl = rx_ring->hw;
	struct platform_device *pdev = ctrl->pdev;

	ftgmac030_clean_rx_ring(rx_ring);

	vfree(rx_ring->buffer_info);
	rx_ring->buffer_info = NULL;

	dma_free_coherent(&pdev->dev, rx_ring->size, rx_ring->desc,
			  rx_ring->dma);
	rx_ring->desc = NULL;
}


/**
 * ftgmac030_alloc_queues - Allocate memory for all rings
 * @ctrl: board private structure to initialize
 */
static int ftgmac030_alloc_queues(struct ftgmac030_ctrl *ctrl)
{
	int size = sizeof(struct ftgmac030_ring);

	ctrl->tx_ring = kzalloc(size, GFP_KERNEL);
	if (!ctrl->tx_ring)
		goto err;
	ctrl->tx_ring->count = ctrl->tx_ring_count;
	ctrl->tx_ring->hw = ctrl;

	ctrl->rx_ring = kzalloc(size, GFP_KERNEL);
	if (!ctrl->rx_ring)
		goto err;
	ctrl->rx_ring->count = ctrl->rx_ring_count;
	ctrl->rx_ring->hw = ctrl;

	return 0;
err:
	e_err(drv, "Unable to allocate memory for queues\n");
	kfree(ctrl->rx_ring);
	kfree(ctrl->tx_ring);
	return -ENOMEM;
}

/**
 * ftgmac030_configure_tx - Configure Transmit Unit after Reset
 * @ctrl: board private structure
 *
 * Configure the Tx unit of the MAC after a reset.
 */
static void ftgmac030_configure_tx(struct ftgmac030_ctrl *ctrl)
{
	struct ftgmac030_ring *tx_ring = ctrl->tx_ring;
	u32 tdba, maccr, aptc;
	struct ftgmac030_txdes *tdben;

	// Setup the HW Tx Head
	tdba = tx_ring->dma;
	iow32(FTGMAC030_REG_NPTXR_BADR, (tdba & DMA_BIT_MASK(32)));

	// Setup end of Tx descriptor bit
	tdben = FTGMAC030_TX_DESC(*tx_ring, (tx_ring->count - 1));
	tdben->txdes0 = cpu_to_le32(FTGMAC030_TXDES0_EDOTR);

	tx_ring->head =  (void __iomem *)(ctrl->io_base +
					  FTGMAC030_REG_NPTXR_PTR);
	// FTGMAC030 does not have register to read end of
	// tx descriptor ring.
	// tail is used to let h/w know there are new descriptors
	// to fetch.
	tx_ring->tail = (void __iomem *)(ctrl->io_base +
					 FTGMAC030_REG_NPTXPD);

	e_dbg(drv, "tx desc dma %x, vaddr %p, nptxpd %08x\n",
			   cpu_to_le32(tx_ring->dma), tx_ring->desc, readl(tx_ring->head));

	aptc = ior32(FTGMAC030_REG_APTC);
	aptc &= ~FTGMAC030_APTC_TXPOLL_CNT(0xf);
	aptc |= (FTGMAC030_APTC_TXPOLL_CNT(10) | FTGMAC030_APTC_TXPOLL_TIME_SEL);
	iow32(FTGMAC030_REG_APTC, aptc);

	// FTGMAC030 can not disable append CRC per packet.
	// See _xmit_frame
	maccr = ior32(FTGMAC030_REG_MACCR);
	maccr |= FTGMAC030_MACCR_CRC_APD;
	iow32(FTGMAC030_REG_MACCR, maccr);

	// Set the Tx Interrupt Delay register
	iow32(FTGMAC030_REG_TXITC, FTGMAC030_ITC_CYCL_CNT(ctrl->tx_int_delay));
}

/**
 * ftgmac030_configure_rx - Configure Receive Unit after Reset
 * @ctrl: board private structure
 *
 * Configure the Rx unit of the MAC after a reset.
 */
static void ftgmac030_configure_rx(struct ftgmac030_ctrl *ctrl)
{
	struct ftgmac030_ring *rx_ring = ctrl->rx_ring;
	struct ftgmac030_rxdes *rdben;
	u32 aptc;

	if (ctrl->netdev->mtu > ETH_FRAME_LEN + ETH_FCS_LEN) {
		ctrl->clean_rx = ftgmac030_clean_jumbo_rx_irq;
		ctrl->alloc_rx_buf = ftgmac030_alloc_jumbo_rx_buffers;
	} else {
		ctrl->clean_rx = ftgmac030_clean_rx_irq;
		ctrl->alloc_rx_buf = ftgmac030_alloc_rx_buffers;
	}

	// Set the Receive Delay Timer Register
	if ((ctrl->itr_setting != 0) && (ctrl->itr != 0))
		ftgmac030_write_itr(ctrl, ctrl->itr);
	else
		iow32(FTGMAC030_REG_RXITC, FTGMAC030_ITC_THR_CNT(1));

	// Setup the HW Rx Head and Tail Descriptor Pointers and
	// the Base and Length of the Rx Descriptor Ring
	iow32(FTGMAC030_REG_RXR_BADR, (rx_ring->dma & DMA_BIT_MASK(32)));

	e_dbg(drv, "rx desc dma %x, vaddr %p\n",
			   cpu_to_le32(rx_ring->dma), rx_ring->desc);

	rx_ring->head = (void __iomem *)(ctrl->io_base +
					 FTGMAC030_REG_RXR_BADR);
	// FTGMAC030 does not have register to read end of
	// rx descriptor ring
	rx_ring->tail = (void __iomem *)(ctrl->io_base +
					 FTGMAC030_REG_RXR_PTR);

	// Setup end of Rx descriptor bit
	rdben = FTGMAC030_RX_DESC(*rx_ring, (rx_ring->count - 1));
	rdben->rxdes0 = cpu_to_le32(FTGMAC030_RXDES0_EDORR);

	aptc = ior32(FTGMAC030_REG_APTC);
	aptc &= ~FTGMAC030_APTC_RXPOLL_CNT(0xf);
	aptc |= FTGMAC030_APTC_RXPOLL_CNT(1);
	iow32(FTGMAC030_REG_APTC, aptc);

	// With jumbo frames, do not let CPU leave C0-state
	if (ctrl->netdev->mtu > ETH_DATA_LEN)
		cpu_latency_qos_update_request(&ctrl->pm_qos_req, 0);
	else
		cpu_latency_qos_update_request(&ctrl->pm_qos_req,
					       PM_QOS_DEFAULT_VALUE);

}

/**
 * Internal helper function to calculate hash value of
 * Multicast Address Hash Table.
 */
static u32 ftgmac030_hash_mc_addr(unsigned char *mac_addr)
{
	unsigned int crc32 = ether_crc(ETH_ALEN, mac_addr);

	crc32 = ~crc32;
	crc32 = bitrev8(crc32 & 0xff) |
		(bitrev8((crc32 >> 8) & 0xff) << 8) |
		(bitrev8((crc32 >> 16) & 0xff) << 16) |
		(bitrev8((crc32 >> 24) & 0xff) << 24);

	return ((unsigned char)(crc32 >> 26));
}

/**
 * ftgmac030_write_mc_addr_list - write multicast addresses to MAHT
 * @netdev: network interface device structure
 *
 * Writes multicast address list to the MTA hash table.
 * Returns: -ENOMEM on failure
 *                0 on no addresses written
 *                X on writing X addresses to MTA
 */
static int ftgmac030_write_mc_addr_list(struct net_device *netdev)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);
	struct netdev_hw_addr *ha;
	u32 hash_index, hash_bit, hash_reg;
	int i;

	memset(&ctrl->mta_shadow, 0, sizeof(ctrl->mta_shadow));

	// Update_mc_addr_list expects a packed array of only addresses
	i = 0;
	netdev_for_each_mc_addr(ha, netdev) {
		hash_index = ftgmac030_hash_mc_addr(ha->addr);

		e_dbg(drv, "Multicat MAC address %pM, hash_val %d\n",
		ha->addr, hash_index);

		hash_reg = (hash_index >> 5) & (ctrl->mta_reg_count - 1);
		hash_bit = hash_index & 0x1F;

		ctrl->mta_shadow[hash_reg] |= (1 << hash_bit);
	}

	// Replace the entire Multicast Address Hash table
	for (i = 0; i < ctrl->mta_reg_count; i++)
		FTGMAC030_WRITE_REG_ARRAY(FTGMAC030_REG_MAHT0, i,
					  ctrl->mta_shadow[i]);
	return netdev_mc_count(netdev);
}

/**
 * ftgmac030_set_rx_mode - secondary unicast, Multicast and Promiscuous mode set
 * @netdev: network interface device structure
 *
 * The ndo_set_rx_mode entry point is called whenever the unicast or multicast
 * address list or the network interface flags are updated.  This routine is
 * responsible for configuring the hardware for proper unicast, multicast,
 * promiscuous mode, and all-multi behavior.
 */
static void ftgmac030_set_rx_mode(struct net_device *netdev)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);
	u32 rctl;

	if (pm_runtime_suspended(netdev->dev.parent))
		return;

	rctl = ior32(FTGMAC030_REG_MACCR);

	/* clear the affected bits */
	rctl &= ~(FTGMAC030_MACCR_RX_ALL | FTGMAC030_MACCR_RX_BROADPKT |
		  FTGMAC030_MACCR_HT_MULTI_EN | FTGMAC030_MACCR_RX_MULTIPKT |
		  FTGMAC030_MACCR_JUMBO_LF | FTGMAC030_MACCR_DISCARD_CRCERR);

	if (netdev->flags & IFF_BROADCAST)
		rctl |= FTGMAC030_MACCR_RX_BROADPKT;

	if (netdev->flags & IFF_PROMISC) {
		rctl |= FTGMAC030_MACCR_RX_ALL;
	} else {
		int count;

		if (netdev->flags & IFF_ALLMULTI) {
			rctl |= FTGMAC030_MACCR_RX_MULTIPKT;
		} else if (netdev->flags & IFF_MULTICAST) {
			// Try to write MAHT0-1, if the attempt fails
			// then just set to receive all multicast.
			count = ftgmac030_write_mc_addr_list(netdev);
			if (count)
				rctl |= FTGMAC030_MACCR_HT_MULTI_EN;
			else
				rctl |= FTGMAC030_MACCR_RX_MULTIPKT;
		}
	}

	// Rx VLAN tag is stripped
	if (netdev->features & NETIF_F_HW_VLAN_CTAG_RX)
		rctl |= FTGMAC030_MACCR_REMOVE_VLAN;
	else
		rctl &= ~FTGMAC030_MACCR_REMOVE_VLAN;

	// Enable Long Packet receive
	if (ctrl->netdev->mtu > ETH_DATA_LEN)
		rctl |= FTGMAC030_MACCR_JUMBO_LF;

	// This is useful for sniffing bad packets.
	if (ctrl->netdev->features & NETIF_F_RXALL) {
		rctl |= (FTGMAC030_MACCR_RX_ALL |
			 FTGMAC030_MACCR_RX_RUNT);
		rctl &= ~FTGMAC030_MACCR_DISCARD_CRCERR;
	} else
		rctl |= FTGMAC030_MACCR_DISCARD_CRCERR;

	// Below are hardware capability specific
	if (ctrl->flags & FLAG_DISABLE_IPV6_RECOG)
		rctl |= FTGMAC030_MACCR_DIS_IPV6_PKTREC;

	iow32(FTGMAC030_REG_MACCR, rctl);

	iow32(FTGMAC030_REG_RBSR, ctrl->rx_buffer_len);

	// if receive broadcast larger than BMRCR[4:0] within
	// BMRCR[23:16] * BMRCR[24] ms/us depends on current
	// link speed, h/w discards the broadcast packets for
	// this period time and enable receive again.
	//
	// Actually, I don't have any idea or experience what
	// number should be configured. Below code just a guideline
	// how to program this register. Please adjust by yourself
	// according to your real application situations.
	//
	// I am assumed to be 0.5 s and receive 1024 broadcast
	// packets and  BMRCR[24] is one.
	if (ctrl->flags & FLAG_DISABLE_RX_BROADCAST_PERIOD) {
		u32 bmrcr, time_thrld;

		bmrcr = (1 << 24);

		switch (ctrl->phy_speed) {
		default:
		case SPEED_10:
			time_thrld = 500000000 / 409600;
			break;

		case SPEED_100:
			time_thrld = 500000000 / 40960;
			break;

		case SPEED_1000:
			time_thrld = 500000000 / 8190;
			break;
		}

		bmrcr |= ((time_thrld & 0xff) << 16);

		/* unit is 256 packets */
		bmrcr |= 4;

		iow32(FTGMAC030_REG_BMRCR, bmrcr);
	}
}

/**
 * ftgmac030_config_hwtstamp - configure the hwtstamp registers and
 * enable/disable
 * @ctrl: board private structure
 *
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
 */
static int ftgmac030_config_hwtstamp(struct ftgmac030_ctrl *ctrl,
				     struct hwtstamp_config *config)
{
	struct timespec64 now;
	u32 tsync_tx_ctl = 1;
	u32 tsync_rx_ctl = 1;
	u32 maccr;

	if (!(ctrl->flags & FLAG_HAS_HW_TIMESTAMP))
		return -EINVAL;
#if DEEDBEEF
	e_info(hw, "Config hwstamp tx 0x%x rx 0x%x, flags 0x%x\n",
	       config->tx_type, config->rx_filter, config->flags);
#endif
	if (config->flags)
		return -EINVAL;

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

	ctrl->hwtstamp_config = *config;

	maccr = ior32(FTGMAC030_REG_MACCR);

	// Enable/disable Tx h/w time stamping.
	// FTGMAC030 cannot enable/disable the Tx/Rx h/w timestamping
	// separately, so just enable PTP if one the them want to be enabled.
	if (tsync_tx_ctl || tsync_rx_ctl) {

		iow32(FTGMAC030_REG_PTP_NS_PERIOD, ctrl->incval);
		iow32(FTGMAC030_REG_PTP_NNS_PERIOD, ctrl->incval_nns);

		maccr |= FTGMAC030_MACCR_PTP_EN;

		// Reset the ns time counter
		ktime_get_real_ts64(&now);
		iow32(FTGMAC030_REG_PTP_TM_SEC, now.tv_sec);
		iow32(FTGMAC030_REG_PTP_TM_NSEC, now.tv_nsec);

	} else
		maccr &= ~FTGMAC030_MACCR_PTP_EN;

	iow32(FTGMAC030_REG_MACCR, maccr);

	return 0;
}

static void ftgmac030_set_mac_hw(struct ftgmac030_ctrl *ctrl)
{
	u8 *mac;
	unsigned int maddr, laddr;

	mac = ctrl->netdev->dev_addr;
	maddr = mac[0] << 8 | mac[1];
	laddr = mac[2] << 24 | mac[3] << 16 | mac[4] << 8 | mac[5];

	iow32(FTGMAC030_REG_MAC_MADR, maddr);
	iow32(FTGMAC030_REG_MAC_LADR, laddr);
}

/**
 * ftgmac030_configure - configure the hardware for Rx and Tx
 * @ctrl: private board structure
 */
static void ftgmac030_configure(struct ftgmac030_ctrl *ctrl)
{
	struct ftgmac030_ring *rx_ring = ctrl->rx_ring;
	u32 maccr;

	ftgmac030_set_mac_hw(ctrl);

	ftgmac030_configure_tx(ctrl);

	ftgmac030_set_rx_mode(ctrl->netdev);
	ftgmac030_configure_rx(ctrl);

	if (ctrl->alloc_rx_buf == ftgmac030_alloc_jumbo_rx_buffers)
		ctrl->alloc_rx_buf(rx_ring, ftgmac030_desc_unused(rx_ring), GFP_ATOMIC);
	else
		ctrl->alloc_rx_buf(rx_ring, ftgmac030_desc_unused(rx_ring), GFP_KERNEL);

	maccr = ior32(FTGMAC030_REG_MACCR);
	maccr |= (FTGMAC030_MACCR_TXDMA_EN | FTGMAC030_MACCR_RXDMA_EN
		  | FTGMAC030_MACCR_TXMAC_EN | FTGMAC030_MACCR_RXMAC_EN);

	maccr &= ~(FTGMAC030_MACCR_MODE_100 | FTGMAC030_MACCR_MODE_1000);
	switch (ctrl->phy_speed) {
	default:
	case SPEED_10:
		ctrl->tx_timeout_factor = 16;
		break;

	case SPEED_100:
		ctrl->tx_timeout_factor = 10;
		maccr |= FTGMAC030_MACCR_MODE_100;
		break;

	case SPEED_1000:
		ctrl->tx_timeout_factor = 1;
		maccr |= FTGMAC030_MACCR_MODE_1000;
		break;
	}

	if (ctrl->phy_duplex)
		maccr |= FTGMAC030_MACCR_FULLDUP;
	else
		maccr &= ~FTGMAC030_MACCR_FULLDUP;

	iow32(FTGMAC030_REG_MACCR, maccr);
}

/**
 *  ftgmac030_set_flow_control - Set flow control high/low watermarks
 *  @ctrl: pointer to the HW structure
 *
 *  Sets the flow control high/low threshold (watermark) registers.
 *
 *  FTGMAC030 does not support disable transmision of XON frame.
 *  A pause frame is sent with pause time = 0 when the RX FIFO free
 *  space is larger than the high threshold.
 */
s32 ftgmac030_set_flow_control(struct ftgmac030_ctrl *ctrl)
{
#if DEEDBEEF
	u32 fcr, fcrl, fcrh;

	/* Set the flow control receive threshold registers.
	 * The unit is 256 bytes.
	 */
	fcr = 0;
	if (ctrl->low_water) {
		fcrl = DIV_ROUND_UP(ctrl->low_water, 256);
		fcr = FTGMAC030_FCR_FC_H_L(fcrl);

		fcr |= FTGMAC030_FCR_PAUSE_TIME(ctrl->pause_time);
		fcr |= (FTGMAC030_FCR_FCTHR_EN | FTGMAC030_FCR_FC_EN);

		iow32(FTGMAC030_REG_FCR, fcr);
	}

	if (ctrl->high_water) {
		fcrh = ctrl->high_water / 256;
		fcr = FTGMAC030_FCR_FC_H_L(fcrh);
		fcr |= FTGMAC030_FCR_HTHR;

		fcr |= FTGMAC030_FCR_PAUSE_TIME(ctrl->pause_time);
		fcr |= (FTGMAC030_FCR_FCTHR_EN | FTGMAC030_FCR_FC_EN);

		iow32(FTGMAC030_REG_FCR, fcr);
	}
#endif
	return 0;
}

/**
 *  ftgmac030_init_hw - Initialize hardware
 *  @ctrl: pointer to the HW structure
 *
 *  This inits the hardware readying it for operation.
 */
static s32 ftgmac030_init_hw(struct ftgmac030_ctrl *ctrl)
{
	s32 ret_val;
	u16 i;

	e_dbg(hw, "Zeroing the MTA\n");
	for (i = 0; i < ctrl->mta_reg_count; i++)
		FTGMAC030_WRITE_REG_ARRAY(FTGMAC030_REG_MAHT0, i, 0);

	/* Setup link and flow control */
	ret_val = ftgmac030_set_flow_control(ctrl);

	return ret_val;
}
/**
 *  ftgmac030_reset_hw - Reset hardware
 *  @ctrl: pointer to the controller structure
 *
 *  This resets the hardware into a known state.
 */
static s32 ftgmac030_reset_hw(struct ftgmac030_ctrl *ctrl)
{
	int i;

	iow32(FTGMAC030_REG_MACCR, FTGMAC030_MACCR_SW_RST);
	for (i = 0; i < 5; i++) {
		unsigned int maccr;

		maccr = ior32(FTGMAC030_REG_MACCR);
		if (!(maccr & FTGMAC030_MACCR_SW_RST))
			return 0;

		udelay(1000);
	}

	e_err(hw, "Software reset failed\n");
	return -EIO;
}

/**
 * ftgmac030_reset - bring the hardware into a known good state
 *
 * This function boots the hardware and enables some settings that
 * require a configuration cycle of the hardware - those cannot be
 * set/changed during runtime. After reset the device needs to be
 * properly configured for Rx, Tx etc.
 */
void ftgmac030_reset(struct ftgmac030_ctrl *ctrl)
{
	u32 tx_space, min_tx_space, min_rx_space;
	u32 fifo, fifo_val;

	ftgmac030_reset_hw(ctrl);

	fifo = ior32(FTGMAC030_REG_TPAFCR);
	fifo &= ~(FTGMAC030_TPAFCR_TFIFO_SIZE(7) |
		  FTGMAC030_TPAFCR_RFIFO_SIZE(7));
#if DEEDBEEF
	// To maintain wire speed transmits, the Tx FIFO should be
	// large enough to accommodate two full transmit packets,
	// rounded up to the next 1KB and expressed in KB.
	min_tx_space = (ctrl->max_frame_size +
			sizeof(struct ftgmac030_txdes) -
			ETH_FCS_LEN) * 2;
	min_tx_space = ALIGN(min_tx_space, 1024);

	if (min_tx_space <= (2 << 10))
		fifo_val = 0;
	else if (min_tx_space <= (4 << 10))
		fifo_val = 1;
	else if (min_tx_space <= (8 << 10))
		fifo_val = 2;
	else if (min_tx_space <= (16 << 10))
		fifo_val = 3;
	else if (min_tx_space <= (32 << 10))
		fifo_val = 4;
	else if (min_tx_space <= (64 << 10))
		fifo_val = 5;
	else
		fifo_val = 6;

	if (fifo_val > 5 || fifo_val > ctrl->max_tx_fifo) {
		WARN(1, "tx fifo sel(%d) larger than hw max(%d)\n",
		     fifo_val, ctrl->max_tx_fifo);
		fifo_val = ctrl->max_tx_fifo;
	}
#else
	fifo_val = ctrl->max_tx_fifo;

	min_tx_space = 2048 << fifo_val;
#endif
	fifo |= FTGMAC030_TPAFCR_TFIFO_SIZE(fifo_val);

#if DEEDBEEF
	min_rx_space = ctrl->max_frame_size;
	min_rx_space = ALIGN(min_rx_space, 1024);

	if (min_rx_space <= (2 << 10))
		fifo_val = 0;
	else if (min_rx_space <= (4 << 10))
		fifo_val = 1;
	else if (min_rx_space <= (8 << 10))
		fifo_val = 2;
	else if (min_rx_space <= (16 << 10))
		fifo_val = 3;
	else if (min_rx_space <= (32 << 10))
		fifo_val = 4;
	else if (min_rx_space <= (64 << 10))
		fifo_val = 5;
	else
		fifo_val = 6;

	if (fifo_val > 5 || fifo_val > ctrl->max_rx_fifo) {
		WARN_ONCE(1, "rx fifo sel(%d) larger than hw max(%d)\n",
			  fifo_val, ctrl->max_rx_fifo);
		fifo_val = ctrl->max_rx_fifo;
	}
#else
	fifo_val = ctrl->max_rx_fifo;

	min_rx_space = 2048 << fifo_val;
#endif
	fifo |= FTGMAC030_TPAFCR_RFIFO_SIZE(fifo_val);

	iow32(FTGMAC030_REG_TPAFCR, fifo);

	// flow control settings
	//
	// The high water mark must be low enough to fit one full frame
	// (or the size used for early receive) above it in the Rx FIFO.
	// Set it to the lower of 10% of:
	// - the Rx FIFO free size
	// - the full Rx FIFO size minus one full frame
	if (ctrl->flags & FLAG_DISABLE_FC_PAUSE_TIME)
		ctrl->pause_time = 0;
	else
		ctrl->pause_time = FTGMAC030_FC_PAUSE_TIME;

	fifo = FTGMAC030_TPAFCR_RFIFO_VAL(ior32(FTGMAC030_REG_TPAFCR));
	fifo = ((1 << fifo) * 2048);

	ctrl->low_water = min((fifo / 10), (fifo - ctrl->max_frame_size));
	ctrl->high_water = (fifo * 9) / 10;

	fifo = FTGMAC030_TPAFCR_TFIFO_VAL(ior32(FTGMAC030_REG_TPAFCR));
	tx_space = ((1 << fifo) * 2048);

	// Maximum size per Tx descriptor limited to 14 bits.
	// Align it with 1024 bytes, so total 15360 bytes.
	ctrl->tx_fifo_limit = min_t(u32, tx_space, 15360);

	// Disable Adaptive Interrupt Moderation if 2 full packets cannot
	// fit in receive buffer.
	fifo = FTGMAC030_TPAFCR_RFIFO_VAL(ior32(FTGMAC030_REG_TPAFCR));
	fifo = ((1 << fifo) * 2048);

	if (ctrl->itr_setting & 0x3) {
		if ((ctrl->max_frame_size * 2) > fifo) {
			if (!(ctrl->flags & FLAG_DISABLE_AIM)) {
				dev_info(&ctrl->pdev->dev,
					 "Interrupt Throttle Rate off\n");
				ctrl->flags |= FLAG_DISABLE_AIM;
				ctrl->itr = 0;
				ftgmac030_write_itr(ctrl, 0);
			}
		} else if (ctrl->flags & FLAG_DISABLE_AIM) {
			dev_info(&ctrl->pdev->dev,
				 "Interrupt Throttle Rate on\n");
			ctrl->flags &= ~FLAG_DISABLE_AIM;
			ctrl->itr = 20000;
			ftgmac030_write_itr(ctrl, ctrl->itr);
		}
	}
#if DEEDBEEF
	e_info(hw, "tx_fifo %d rx_fifo %d fc low %d fc high %d itr %d\n",
	       ctrl->tx_fifo_limit, min_rx_space, ctrl->low_water,
	       ctrl->high_water, ctrl->itr);
#endif
	if (ftgmac030_init_hw(ctrl))
		e_err(hw, "Hardware Error\n");

	ctrl->hwtstamp_config.flags = 0;
	ctrl->hwtstamp_config.tx_type = HWTSTAMP_TX_OFF;
	ctrl->hwtstamp_config.rx_filter = HWTSTAMP_FILTER_NONE;
	ftgmac030_config_hwtstamp(ctrl, &ctrl->hwtstamp_config);

	if (ctrl->flags & FLAG_HAS_EEE) {
		u32 eee;

		eee = ior32(FTGMAC030_REG_EEE);
		eee &= ~FTGMAC030_EEE_TX_LPI;
		iow32(FTGMAC030_REG_EEE, eee);
	}
}

int ftgmac030_up(struct ftgmac030_ctrl *ctrl)
{
	struct phy_device *phydev = ctrl->phy_dev;
	u32 eee;
	int ret;

	/* If RX_CLK is stopped when PHY is Link down,
	 * It might cause Rx FIFO not empty.
	 * Do the software reset again for RX FIFO is empty.
	 */
	if (!(ior32(FTGMAC030_REG_DMAFIFOS) & FTGMAC030_DMAFIFOS_RXFIFO_EMPTY)) {
		pr_info("%s: ftgmac030_reset retry\n", __func__);
		ftgmac030_reset(ctrl);
	}

	// Hardware has been reset, need to reload
	// appropriate values.
	ftgmac030_configure(ctrl);

	clear_bit(__FTGMAC030_DOWN, &ctrl->state);

	// Set EEE advertisement as appropriate
	if ((ctrl->flags & FLAG_HAS_EEE) &&
	     phydev->link) {
		eee = ior32(FTGMAC030_REG_EEE);
		eee &= ~FTGMAC030_EEE_TX_LPI;

		// User wants to enable EEE
		if (ctrl->eee_enabled) {
			// Check if Link Partner OK to enable EEE
			ret = phy_init_eee(phydev, ctrl->clk_stop_enable);
			if (ret == 0) {
				if (ctrl->tx_lpi_enabled)
					eee |= FTGMAC030_EEE_TX_LPI;

				eee &= ~(FTGMAC030_EEE_TX_ENTER |
					 FTGMAC030_EEE_TX_WAKEUP);
				eee |= ctrl->tx_lpi_timer;
			}
		}

		iow32(FTGMAC030_REG_EEE, eee);
	}

	ftgmac030_irq_enable(ctrl);

	netif_start_queue(ctrl->netdev);

	return 0;
}

/**
 * ftgmac030_down - quiesce the device and optionally reset the hardware
 * @ctrl: board private structure
 * @reset: boolean flag to reset the hardware or not
 */
void ftgmac030_down(struct ftgmac030_ctrl *ctrl, bool reset)
{
	struct net_device *netdev = ctrl->netdev;
	u32 maccr;

	ftgmac030_irq_disable(ctrl);

	// Signal that we're down so the interrupt handler does not
	set_bit(__FTGMAC030_DOWN, &ctrl->state);

	netif_stop_queue(netdev);

	maccr = ior32(FTGMAC030_REG_MACCR);

	/* Wait for TX FIFO empty */
	while (!(ior32(FTGMAC030_REG_DMAFIFOS) & FTGMAC030_DMAFIFOS_TXFIFO_EMPTY))
		;

	/* disable transmits DMA */
	maccr &= ~FTGMAC030_MACCR_TXDMA_EN;
	iow32(FTGMAC030_REG_MACCR, maccr);

	usleep_range(10000, 20000);

	/* disable transmits in the hardware */
	maccr &= ~FTGMAC030_MACCR_TXMAC_EN;
	iow32(FTGMAC030_REG_MACCR, maccr);

	/* disable receives packet from ethernet */
	maccr &= ~FTGMAC030_MACCR_RXMAC_EN;
	iow32(FTGMAC030_REG_MACCR, maccr);

	/* Wait for RX FIFO empty */
	while (!(ior32(FTGMAC030_REG_DMAFIFOS) & FTGMAC030_DMAFIFOS_RXFIFO_EMPTY))
		;

	/* disable Rx DMA */
	maccr &= ~FTGMAC030_MACCR_RXDMA_EN;
	iow32(FTGMAC030_REG_MACCR, maccr);

	napi_synchronize(&ctrl->napi);

	ftgmac030_clean_tx_ring(ctrl->tx_ring);
	ftgmac030_clean_rx_ring(ctrl->rx_ring);

	if (reset) {
		struct phy_device *phydev = ctrl->phy_dev;

		if (ctrl->flags & FLAG_HAS_EEE) {
			bool clk_stop_enable_local;

			clk_stop_enable_local = !!(phy_read_mmd(phydev, MDIO_MMD_PCS, MDIO_CTRL1) &
						   MDIO_PCS_CTRL1_CLKSTOP_EN);
			/* Clear clock_stop_enable bit if it is set,
			 * FTGMAC030 requires Rx_CLK to do software reset.
			 */
			e_info(ifdown, "%s : clk_stop_enable_local=%x\n", __func__, clk_stop_enable_local);
			if (clk_stop_enable_local) {
				phy_clear_bits_mmd(phydev, MDIO_MMD_PCS, MDIO_CTRL1,
						   MDIO_PCS_CTRL1_CLKSTOP_EN);
				e_info(ifdown, "%s : clk stop disabled.\n", __func__);
			}
		}

		ftgmac030_reset(ctrl);
	}
}

void ftgmac030_reinit_locked(struct ftgmac030_ctrl *ctrl)
{
	might_sleep();
	while (test_and_set_bit(__FTGMAC030_RESETTING, &ctrl->state))
		usleep_range(1000, 2000);
	ftgmac030_down(ctrl, true);
	ftgmac030_up(ctrl);
	clear_bit(__FTGMAC030_RESETTING, &ctrl->state);
}

/**
 * ftgmac030_sw_init - Initialize general software structures
		       (struct ftgmac030_ctrl)
 * @ctrl: board private structure to initialize
 *
 * ftgmac030_sw_init initializes the ctrl private data structure.
 * Fields are initialized based on PCI device information and
 * OS network device settings (MTU size).
 */
static int ftgmac030_sw_init(struct ftgmac030_ctrl *ctrl)
{
	struct net_device *netdev = ctrl->netdev;
	u32 fea, bufsz;

	bufsz = ETH_FRAME_LEN + VLAN_HLEN + ETH_FCS_LEN;
	ctrl->rx_buffer_len = ALIGN(bufsz, 16);
	ctrl->max_frame_size = netdev->mtu + ETH_HLEN + ETH_FCS_LEN;
	ctrl->min_frame_size = ETH_ZLEN + ETH_FCS_LEN;
	ctrl->tx_ring_count = FTGMAC030_DEFAULT_TXD;
	ctrl->rx_ring_count = FTGMAC030_DEFAULT_RXD;

	if (ftgmac030_alloc_queues(ctrl))
		return -ENOMEM;

	fea = ior32(FTGMAC030_REG_FEAR);
	ctrl->max_rx_fifo = fea & 0x7;
	ctrl->max_tx_fifo = (fea >> 4) & 0x7;

	// Setup hardware time stamping cyclecounter
	if (ctrl->flags & FLAG_HAS_HW_TIMESTAMP) {
		ctrl->incval = INCVALUE_25MHz;
		ctrl->incval_nns = INCVALUE_25MHz_NNS;

		spin_lock_init(&ctrl->systim_lock);
	}

	ftgmac030_irq_disable(ctrl);

	set_bit(__FTGMAC030_DOWN, &ctrl->state);
	return 0;
}

/**
 * ftgmac030_open - Called when a network interface is made active
 * @netdev: network interface device structure
 *
 * Returns 0 on success, negative value on failure
 *
 * The open entry point is called when a network interface is made
 * active by the system (IFF_UP).  At this point all resources needed
 * for transmit and receive operations are allocated, the interrupt
 * handler is registered with the OS, the watchdog timer is started,
 * and the stack is notified that the interface is ready.
 **/
int ftgmac030_open(struct net_device *netdev)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);
	struct platform_device *pdev = ctrl->pdev;
	int err;

	pm_runtime_get_sync(&pdev->dev);

	ftgmac030_reset(ctrl);

	// Allocate transmit descriptors
	err = ftgmac030_setup_tx_resources(ctrl->tx_ring);
	if (err)
		goto err_setup_tx;

	// Allocate receive descriptors
	err = ftgmac030_setup_rx_resources(ctrl->rx_ring);
	if (err)
		goto err_setup_rx;

	cpu_latency_qos_add_request(&ctrl->pm_qos_req, PM_QOS_DEFAULT_VALUE);

	ftgmac030_configure(ctrl);

	clear_bit(__FTGMAC030_DOWN, &ctrl->state);

	napi_enable(&ctrl->napi);

	ftgmac030_irq_enable(ctrl);

	netif_start_queue(netdev);

	phy_start(ctrl->phy_dev);

	pm_runtime_put_noidle(&pdev->dev);

	return 0;

err_setup_rx:
	ftgmac030_free_tx_resources(ctrl->tx_ring);
err_setup_tx:
	pm_runtime_put_sync(&pdev->dev);

	return err;
}

/**
 * ftgmac030_close - Disables a network interface
 * @netdev: network interface device structure
 *
 * Returns 0, this is not allowed to fail
 *
 * The close entry point is called when an interface is de-activated
 * by the OS.  The hardware is still under the drivers control, but
 * needs to be disabled.  A global MAC reset is issued to stop the
 * hardware, and all transmit and receive resources are freed.
 **/
int ftgmac030_close(struct net_device *netdev)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);
	struct platform_device *pdev = ctrl->pdev;
	int count = FTGMAC030_CHECK_RESET_COUNT;

	while (test_bit(__FTGMAC030_RESETTING, &ctrl->state) && count--)
		usleep_range(10000, 20000);

	WARN_ON(test_bit(__FTGMAC030_RESETTING, &ctrl->state));

	pm_runtime_get_sync(&pdev->dev);

	if (!test_bit(__FTGMAC030_DOWN, &ctrl->state)) {
		ftgmac030_down(ctrl, true);
		e_info(ifdown, "%s Interface is Down\n", ctrl->netdev->name);
	}

	napi_disable(&ctrl->napi);
	phy_stop(ctrl->phy_dev);

	ftgmac030_free_tx_resources(ctrl->tx_ring);
	ftgmac030_free_rx_resources(ctrl->rx_ring);

	cpu_latency_qos_remove_request(&ctrl->pm_qos_req);

	pm_runtime_put_sync(&pdev->dev);

	return 0;
}

/**
 * ftgmac030_set_mac - Change the Ethernet Address of the NIC
 * @netdev: network interface device structure
 * @p: pointer to an address structure
 *
 * Returns 0 on success, negative on failure
 **/
static int ftgmac030_set_mac(struct net_device *netdev, void *p)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);
	struct sockaddr *addr = p;

	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;

	memcpy(netdev->dev_addr, addr->sa_data, netdev->addr_len);

	ftgmac030_set_mac_hw(ctrl);
	return 0;
}

static void ftgmac030_adjust_link(struct net_device *netdev)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);
	struct phy_device *phydev = ctrl->phy_dev;
	u32 eee;

	if ((ctrl->phy_link == phydev->link) &&
	    (ctrl->phy_duplex == phydev->duplex) &&
	    (ctrl->phy_speed == phydev->speed))
		return;

	e_dbg(link, "new speed %d, old speed %d, link %s\n",
	       phydev->speed, ctrl->phy_speed, phydev->link ? "up" : "down");

	while (test_and_set_bit(__FTGMAC030_RESETTING, &ctrl->state))
		usleep_range(1000, 2000);

	if (!test_bit(__FTGMAC030_DOWN, &ctrl->state))
		ftgmac030_down(ctrl, true);

	ctrl->phy_speed = phydev->speed;
	ctrl->phy_duplex = phydev->duplex;
	ctrl->phy_link = phydev->link;

	if (phydev->link) {
		/* Cancel scheduled suspend requests. */
		pm_runtime_get_sync(netdev->dev.parent);

		ftgmac030_up(ctrl);
	} else {
		eee = ior32(FTGMAC030_REG_EEE);
		eee &= ~FTGMAC030_EEE_TX_LPI;
		iow32(FTGMAC030_REG_EEE, eee);

		pm_schedule_suspend(netdev->dev.parent, LINK_TIMEOUT);
	}

	phy_print_status(phydev);

	clear_bit(__FTGMAC030_RESETTING, &ctrl->state);

}

/**
 * ftgmac030_tx_timeout - Respond to a Tx Hang
 * @netdev: network interface device structure
 */
static void ftgmac030_tx_timeout(struct net_device *netdev,
				 unsigned int __always_unused txqueue)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);

	// Do the reset outside of interrupt context
	ctrl->tx_timeout_count++;
	e_err(hw, "Schedule reset tasks\n");
	schedule_work(&ctrl->reset_task);
}

static void ftgmac030_reset_task(struct work_struct *work)
{
	struct ftgmac030_ctrl *ctrl;

	ctrl = container_of(work, struct ftgmac030_ctrl, reset_task);

	if (test_bit(__FTGMAC030_DOWN, &ctrl->state)) {
		e_err(hw, "Reset tasks .. do nothing\n");
		return;
	}
	e_err(hw, "Reset tasks\n");

	//ftgmac030_dump(ctrl);

	ftgmac030_reinit_locked(ctrl);
}

/**
 * ftgmac030_change_mtu - Change the Maximum Transfer Unit
 * @netdev: network interface device structure
 * @new_mtu: new value for maximum frame size
 *
 * Returns 0 on success, negative on failure
 */
static int ftgmac030_change_mtu(struct net_device *netdev, int new_mtu)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);
	int max_frame = new_mtu + VLAN_HLEN + ETH_HLEN + ETH_FCS_LEN;

	if ((max_frame > ETH_FRAME_LEN + ETH_FCS_LEN) &&
	    !(ctrl->flags & FLAG_HAS_JUMBO_FRAMES)) {
		e_err(drv, "Jumbo Frames not supported.\n");
		return -EINVAL;
	}

	if ((new_mtu < ETH_ZLEN + ETH_FCS_LEN + VLAN_HLEN) ||
	    (max_frame > ctrl->max_hw_frame_size)) {
		e_err(drv, "Unsupported MTU setting\n");
		return -EINVAL;
	}

	while (test_and_set_bit(__FTGMAC030_RESETTING, &ctrl->state))
		usleep_range(1000, 2000);

	ctrl->max_frame_size = max_frame;
	e_info(drv, "changing MTU from %d to %d\n", netdev->mtu, new_mtu);
	netdev->mtu = new_mtu;

	pm_runtime_get_sync(netdev->dev.parent);

	if (netif_running(netdev))
		ftgmac030_down(ctrl, true);

	if (max_frame <= 2048)
		ctrl->rx_buffer_len = 2048;
	else
		ctrl->rx_buffer_len = 4096;

	if (netif_running(netdev))
		ftgmac030_up(ctrl);
	else
		ftgmac030_reset(ctrl);

	pm_runtime_put_sync(netdev->dev.parent);

	clear_bit(__FTGMAC030_RESETTING, &ctrl->state);

	return 0;
}

/**
 * ftgmac030_hwtstamp_set - control hardware time stamping
 * @netdev: network interface device structure
 * @ifreq: interface request
 *
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
 */
static int ftgmac030_hwtstamp_set(struct net_device *netdev, struct ifreq *ifr)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);
	struct hwtstamp_config config;
	int ret_val;

	if (copy_from_user(&config, ifr->ifr_data, sizeof(config)))
		return -EFAULT;

	ret_val = ftgmac030_config_hwtstamp(ctrl, &config);
	if (ret_val)
		return ret_val;

	return copy_to_user(ifr->ifr_data, &config,
			    sizeof(config)) ? -EFAULT : 0;
}

static int ftgmac030_hwtstamp_get(struct net_device *netdev, struct ifreq *ifr)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);

	return copy_to_user(ifr->ifr_data, &ctrl->hwtstamp_config,
			    sizeof(ctrl->hwtstamp_config)) ? -EFAULT : 0;
}

static int ftgmac030_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);

	switch (cmd) {
	case SIOCSHWTSTAMP:
		return ftgmac030_hwtstamp_set(netdev, ifr);
	case SIOCGHWTSTAMP:
		return ftgmac030_hwtstamp_get(netdev, ifr);
	default:
		return phy_mii_ioctl(ctrl->phy_dev, ifr, cmd);
	}
}

static int ftgmac030_pm_freeze(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct net_device *netdev = platform_get_drvdata(pdev);
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);

	netif_device_detach(netdev);

	if (netif_running(netdev)) {
		int count = FTGMAC030_CHECK_RESET_COUNT;

		while (test_bit(__FTGMAC030_RESETTING, &ctrl->state) && count--)
			usleep_range(10000, 20000);

		WARN_ON(test_bit(__FTGMAC030_RESETTING, &ctrl->state));

		ftgmac030_down(ctrl, false);

		if (ctrl->phy_dev)
			phy_stop(ctrl->phy_dev);
	}

	return 0;
}

#ifdef CONFIG_PM
static int __ftgmac030_resume(struct platform_device *pdev)
{
	struct net_device *netdev = platform_get_drvdata(pdev);
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);

	ftgmac030_phy_hw(ctrl);

	ftgmac030_reset(ctrl);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int ftgmac030_pm_thaw(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct net_device *netdev = platform_get_drvdata(pdev);
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);

	if (netif_running(netdev)) {
		ftgmac030_up(ctrl);
		phy_start(ctrl->phy_dev);
	}

	netif_device_attach(netdev);

	return 0;
}

static int ftgmac030_pm_suspend(struct device *dev)
{
	return ftgmac030_pm_freeze(dev);
}

static int ftgmac030_pm_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	int rc;

	rc = __ftgmac030_resume(pdev);
	if (rc)
		return rc;

	return ftgmac030_pm_thaw(dev);
}
#endif /* CONFIG_PM_SLEEP */

static int ftgmac030_pm_runtime_idle(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct net_device *netdev = platform_get_drvdata(pdev);
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);
	struct phy_device *phydev = ctrl->phy_dev;

	if (!phydev->link)
		pm_schedule_suspend(dev, 5 * MSEC_PER_SEC);

	return -EBUSY;
}

static int ftgmac030_pm_runtime_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct net_device *netdev = platform_get_drvdata(pdev);
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);
	int rc;

	rc = __ftgmac030_resume(pdev);
	if (rc)
		return rc;

	if (netdev->flags & IFF_UP)
		rc = ftgmac030_up(ctrl);

	return rc;
}

static int ftgmac030_pm_runtime_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct net_device *netdev = platform_get_drvdata(pdev);
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);

	if (netdev->flags & IFF_UP) {
		int count = FTGMAC030_CHECK_RESET_COUNT;

		while (test_bit(__FTGMAC030_RESETTING, &ctrl->state) && count--)
			usleep_range(10000, 20000);

		WARN_ON(test_bit(__FTGMAC030_RESETTING, &ctrl->state));

		ftgmac030_down(ctrl, false);
	}

	return 0;
}
#endif /* CONFIG_PM */

static void ftgmac030_shutdown(struct platform_device *pdev)
{
	ftgmac030_pm_freeze(&pdev->dev);
}

#ifdef CONFIG_NET_POLL_CONTROLLER
/**
 * ftgmac030_netpoll
 * @netdev: network interface device structure
 *
 * Polling 'interrupt' - used by things like netconsole to send skbs
 * without having to re-enable interrupts. It's not called while
 * the interrupt routine is executing.
 */
static void ftgmac030_netpoll(struct net_device *netdev)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);

	disable_irq(ctrl->irq);
	ftgmac030_intr(ctrl->irq, netdev);
	enable_irq(ctrl->irq);
}
#endif

static int ftgmac030_set_features(struct net_device *netdev,
				  netdev_features_t features)
{
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);
	netdev_features_t changed = features ^ netdev->features;

	if (!(changed & (NETIF_F_RXCSUM | NETIF_F_RXFCS | NETIF_F_RXALL)))
		return 0;

	netdev->features = features;

	if (netif_running(netdev))
		ftgmac030_reinit_locked(ctrl);
	else
		ftgmac030_reset(ctrl);

	return 0;
}

static int ftgmac030_mdiobus_read(struct mii_bus *bus, int phy_addr, int regnum)
{
	struct net_device *netdev = bus->priv;
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);
	unsigned int phycr;
	int i;

	phycr = FTGMAC030_PHYCR_ST(1) | FTGMAC030_PHYCR_OP(2) |
		FTGMAC030_PHYCR_PHYAD(phy_addr) |
		FTGMAC030_PHYCR_REGAD(regnum) |
		FTGMAC030_PHYCR_MIIRD | ctrl->mdc_cycthr;

	iowrite32(phycr, ctrl->mdio_base + FTGMAC030_REG_PHYCR);

	for (i = 0; i < 100; i++) {
		phycr = ioread32(ctrl->mdio_base + FTGMAC030_REG_PHYCR);

		if ((phycr & FTGMAC030_PHYCR_MIIRD) == 0) {
			int data;

			data = ioread32(ctrl->mdio_base + FTGMAC030_REG_PHYDATA);
			return FTGMAC030_PHYDATA_MIIRDATA(data);
		}

		udelay(100);
	}

	e_err(link, "mdio read timed out\n");
	return -EIO;
}

static int ftgmac030_mdiobus_write(struct mii_bus *bus, int phy_addr,
				   int regnum, u16 value)
{
	struct net_device *netdev = bus->priv;
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);
	unsigned int phycr;
	int data;
	int i;

	phycr = FTGMAC030_PHYCR_ST(1) | FTGMAC030_PHYCR_OP(1) |
		FTGMAC030_PHYCR_PHYAD(phy_addr) |
		FTGMAC030_PHYCR_REGAD(regnum) |
		FTGMAC030_PHYCR_MIIWR | ctrl->mdc_cycthr;

	data = FTGMAC030_PHYDATA_MIIWDATA(value);

	iowrite32(data, ctrl->mdio_base + FTGMAC030_REG_PHYDATA);
	iowrite32(phycr, ctrl->mdio_base + FTGMAC030_REG_PHYCR);

	for (i = 0; i < 100; i++) {
		phycr = ioread32(ctrl->mdio_base + FTGMAC030_REG_PHYCR);

		if ((phycr & FTGMAC030_PHYCR_MIIWR) == 0)
			return 0;

		udelay(100);
	}

	e_err(link, "mdio write timed out\n");
	return -EIO;
}

static int ftgmac030_mdiobus_reset(struct mii_bus *bus)
{
	return 0;
}

static int ftgmac030_mii_probe(struct ftgmac030_ctrl *ctrl)
{
	struct net_device *netdev = ctrl->netdev;
	struct phy_device *phydev = NULL;
	struct device_node *np, *phy_node;
	int ret, addr;

	np = ctrl->pdev->dev.of_node;

	if (of_phy_is_fixed_link(np)) {
		e_info(probe, "%s register fixed link\n", np->full_name);
		ret = of_phy_register_fixed_link(np);
		if (ret < 0) {
			e_err(link, "%s broken fixed-link spec\n", netdev->name);
			return ret;
		}

		phy_node = of_node_get(np);
		if (!phy_node) {
			e_err(link, "get phy_node failed\n");
			return -ENODEV;
		}
		e_info(probe, "phy_node: %s\n", phy_node->full_name);

		phydev = of_phy_connect(netdev, phy_node,
					&ftgmac030_adjust_link, 0,
					ctrl->phy_interface);
		if (!phydev)
			return -ENODEV;
	} else {
		ret = of_property_read_s32(np, "phy-addr", &addr);
		if (ret < 0) {
			e_info(probe, "%s has no phy-addr property, find first.\n",
				      np->full_name);
			/* search for connect PHY device */
			phydev = phy_find_first(ctrl->mii_bus);
			if (!phydev || !phydev->phy_id) {
				e_err(link, "%s no PHY found\n", netdev->name);
				return -ENODEV;
			}
		} else {
			/* A PHY must have a reg property in the range [0-31] */
			if (addr >= PHY_MAX_ADDR) {
				e_err(link, "%s PHY address %i is too large\n",
					np->full_name, addr);
				return -EINVAL;
			}

			phydev = mdiobus_get_phy(ctrl->mii_bus, addr);
			if (!phydev) {
				e_err(link, "%s no PHY found\n", netdev->name);
				return -ENODEV;
			}
		}

		/* attach the mac to the phy */
		ret = phy_connect_direct(netdev, phydev, &ftgmac030_adjust_link,
					 ctrl->phy_interface);
		if (ret) {
			e_err(link, "Could not attach to PHY\n");
			return ret;
		}

	}

	phy_attached_info(phydev);

	e_info(probe, "%s find PHY (phy_addr=%d, phy_id=%x)\n", netdev->name, phydev->mdio.addr, phydev->phy_id);

	ctrl->phy_dev = phydev;

	// Set default EEE value
	if (ctrl->flags & FLAG_HAS_EEE) {
		int eee_cap;

		/* Check if the EEE ability is supported */
		eee_cap = phy_read_mmd(ctrl->phy_dev, MDIO_MMD_PCS, MDIO_PCS_EEE_ABLE);
		if (eee_cap <= 0) {
			ctrl->eee_enabled = 0;
			ctrl->tx_lpi_enabled = 0;
			ctrl->tx_lpi_timer = 0;
			ctrl->clk_stop_enable = 0;
		} else {
			u32 eee;

			ctrl->eee_enabled = ctrl->tx_lpi_enabled = 1;
			ctrl->clk_stop_enable = 1;

			eee = ior32(FTGMAC030_REG_EEE);
			eee |= FTGMAC030_EEE_TX_LPI;
			iow32(FTGMAC030_REG_EEE, eee);

			ctrl->tx_lpi_timer = eee & ~FTGMAC030_EEE_TX_LPI;
		}
	}

	return 0;
}

static int ftgmac030_mii_init(struct ftgmac030_ctrl *ctrl)
{
	int i, err = 0;

	ctrl->mii_bus = mdiobus_alloc();
	if (!ctrl->mii_bus) {
		err = -EIO;
		dev_err(&ctrl->pdev->dev, "Allocate mii bus failed!\n");
		goto err_alloc_mdiobus;
	}

	ctrl->mii_bus->name = "ftgmac030_mdio";
	snprintf(ctrl->mii_bus->id, MII_BUS_ID_SIZE, "%s-%x",
		 dev_name(&ctrl->pdev->dev), ctrl->pdev->dev.id);

	ctrl->mii_bus->parent = &ctrl->pdev->dev;
	ctrl->mii_bus->priv = ctrl->netdev;
	ctrl->mii_bus->read = ftgmac030_mdiobus_read;
	ctrl->mii_bus->write = ftgmac030_mdiobus_write;
	ctrl->mii_bus->reset = ftgmac030_mdiobus_reset;

	for (i = 0; i < PHY_MAX_ADDR; i++)
		ctrl->mii_bus->irq[i] = PHY_POLL;

	err = mdiobus_register(ctrl->mii_bus);
	if (err) {
		dev_err(&ctrl->pdev->dev, "Cannot register MDIO bus!\n");
		goto err_register_mdiobus;
	}

	err = ftgmac030_mii_probe(ctrl);
	if (err) {
		dev_err(&ctrl->pdev->dev, "MII Probe failed!\n");
		goto err_mii_probe;
	}

	return 0;

err_mii_probe:
	mdiobus_unregister(ctrl->mii_bus);
err_register_mdiobus:
	mdiobus_free(ctrl->mii_bus);
err_alloc_mdiobus:

	return err;
}

static const struct net_device_ops ftgmac030_netdev_ops = {
	.ndo_open		= ftgmac030_open,
	.ndo_stop		= ftgmac030_close,
	.ndo_start_xmit		= ftgmac030_xmit_frame,
	.ndo_set_rx_mode	= ftgmac030_set_rx_mode,
	.ndo_set_mac_address	= ftgmac030_set_mac,
	.ndo_change_mtu		= ftgmac030_change_mtu,
	.ndo_do_ioctl		= ftgmac030_ioctl,
	.ndo_tx_timeout		= ftgmac030_tx_timeout,
	.ndo_validate_addr	= eth_validate_addr,

#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller	= ftgmac030_netpoll,
#endif
	.ndo_set_features = ftgmac030_set_features,
};

static int ftgmac030_pltfm_init(struct platform_device *pdev, struct ftgmac030_ctrl *ctrl)
{
	struct device_node *np = pdev->dev.of_node;
	u32 def_reset_delays[DELAYS_NUM] = {10000, 20000, 150000};
	unsigned int reg;

	if (of_property_read_u32_array(pdev->dev.of_node, "phy-reset-delay", ctrl->phy_reset_delays, DELAYS_NUM))
		memcpy(ctrl->phy_reset_delays, def_reset_delays, sizeof(def_reset_delays));

	ctrl->phy_rst = devm_reset_control_get(&pdev->dev, "phy_reset");
	if (IS_ERR_OR_NULL(ctrl->phy_rst)) {
		dev_err(&pdev->dev, "get phy_rst failed.\n");
		return PTR_ERR(ctrl->phy_rst);
	}
	ctrl->sysctrl_regmap = syscon_regmap_lookup_by_phandle(np, "sysctrl_regmap");
	if (IS_ERR(ctrl->sysctrl_regmap)) {
		dev_err(&pdev->dev, "get sysctrl regmap failed.\n");
		return -1;
	}

	ctrl->crg_regmap = syscon_regmap_lookup_by_phandle(np, "crg_regmap");
	if (IS_ERR(ctrl->crg_regmap)) {
		dev_err(&pdev->dev, "get crg regmap failed.\n");
		return -1;
	}

	regmap_read(ctrl->sysctrl_regmap, BSPETH_CFG_CRG_REG, &reg);

	if (reg & (1UL << (3)))
		ctrl->ftgmac030_mac_intf = FTGMAC030_MODE_MII_GMII;
	else if (reg & (1UL << (0)))
		ctrl->ftgmac030_mac_intf = FTGMAC030_MODE_RGMII;
	else
		ctrl->ftgmac030_mac_intf = FTGMAC030_MODE_RMII;

	ctrl->ftgmac030_phy_reset_n = reg & ETH_PHY_RST_N;

	return 0;
}

/**
 * ftgmac030_probe - Device Initialization Routine
 * @pdev: platform device information struct
 *
 * Returns 0 on success, negative on failure
 *
 * ftgmac030_probe initializes an hw identified by a
 * platform_device structure.
 * The OS initialization, configuring of the ctrl private structure,
 * and a hardware reset occur.
 **/
static int ftgmac030_probe(struct platform_device *pdev)
{
	struct net_device *netdev;
	struct ftgmac030_ctrl *ctrl;
	struct resource *res0, *res1;
	int err;

	pr_info("Faraday %s Network Driver - %s\n",
		ftgmac030_driver_name, ftgmac030_driver_version);

	err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	if (err) {
		dev_err(&pdev->dev, "No usable DMA configuration, aborting\n");
		goto err_dma;
	}

	err = -ENOMEM;
	netdev = alloc_etherdev(sizeof(struct ftgmac030_ctrl));
	if (!netdev)
		goto err_alloc_etherdev;

	SET_NETDEV_DEV(netdev, &pdev->dev);

	platform_set_drvdata(pdev, netdev);

	ctrl = netdev_priv(netdev);
	ctrl->netdev = netdev;

	err = ftgmac030_pltfm_init(pdev, ctrl);
	if (err) {
		dev_err(&pdev->dev, "platform init fail\n");
		goto err_ioremap;
	}

	err = -ENXIO;
	res0 = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res0) {
		dev_err(&pdev->dev, "no ctrl base defined\n");
		goto err_ioremap;
	}

	ctrl->io_base = devm_ioremap(&pdev->dev, res0->start, resource_size(res0));
	if (!ctrl->io_base) {
		dev_err(&pdev->dev, "failed to map registers, aborting.\n");
		err = -ENOMEM;
		goto err_ioremap;
	}

	err = -ENXIO;
	res1 = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res1) {
		dev_err(&pdev->dev, "no mdio resource defined, use ctrl base.\n");
		ctrl->mdio_base = ctrl->io_base;
	} else {
		ctrl->mdio_base = devm_ioremap(&pdev->dev, res1->start, resource_size(res1));
		if (!ctrl->mdio_base) {
			dev_err(&pdev->dev, "failed to map mdio\n");
			err = -ENOMEM;
			goto err_ioremap;
		}
	}

	ftgmac030_phy_hw(ctrl);

	netdev->irq = platform_get_irq(pdev, 0);

	ctrl->irq = netdev->irq;
	ctrl->pdev = pdev;
	ctrl->max_hw_frame_size = DEFAULT_JUMBO;
	ctrl->msg_enable = netif_msg_init(debug, DEFAULT_MSG_ENABLE);

	/* See ftgmac030.h for more flags */
	ctrl->flags = FLAG_HAS_HW_VLAN_FILTER
		    | FLAG_HAS_JUMBO_FRAMES
		    | FLAG_HAS_EEE;

	if (ior32(FTGMAC030_REG_FEAR) & FTGMAC030_FEAR_PTP)
		ctrl->flags |= FLAG_HAS_HW_TIMESTAMP;

	netdev->netdev_ops = &ftgmac030_netdev_ops;
	ftgmac030_set_ethtool_ops(netdev);
	netdev->watchdog_timeo = 5 * HZ;
	netif_napi_add(netdev, &ctrl->napi, ftgmac030_poll, 64);
	strlcpy(netdev->name, dev_name(&pdev->dev), sizeof(netdev->name));

	netdev->mem_start = res0->start;
	netdev->mem_end = res0->end;

	ctrl->itr_setting = 0;
	ctrl->mta_reg_count = 2;
	// Transmit Interrupt Delay in cycle units:
	//  1000 Mbps mode -> 16.384 µs
	//  100 Mbps mode -> 81.92 µs
	//  10 Mbps mode -> 819.2 µs
	ctrl->tx_int_delay = 0xFF;

	err = ftgmac030_sw_init(ctrl);
	if (err)
		goto err_sw_init;

	// Set initial default active device features
	netdev->features = (NETIF_F_SG | NETIF_F_RXCSUM | NETIF_F_HW_CSUM);

	// Set user-changeable features (subset of all device features)
	netdev->hw_features = netdev->features;
	netdev->hw_features |= (NETIF_F_RXFCS |
				NETIF_F_HW_VLAN_CTAG_RX |
				NETIF_F_RXALL);
	netdev->priv_flags |= IFF_SUPP_NOFCS;

	netdev->min_mtu = ETH_MIN_MTU;
	netdev->max_mtu = 9000;

	ctrl->phy_interface = (ctrl->ftgmac030_mac_intf == FTGMAC030_MODE_MII_GMII) ?
		PHY_INTERFACE_MODE_MII : ((ctrl->ftgmac030_mac_intf == FTGMAC030_MODE_RMII) ?
		PHY_INTERFACE_MODE_RMII : PHY_INTERFACE_MODE_RGMII_RXID);

	/* set the mdc cycthr sys_clk mpw:200M pliot:297*/
	ctrl->mdc_cycthr = 0xc8;

	INIT_WORK(&ctrl->print_hang_task, ftgmac030_print_hw_hang);
	INIT_WORK(&ctrl->reset_task, ftgmac030_reset_task);

	ftgmac030_reset_hw(ctrl);

	err = ftgmac030_mii_init(ctrl);
	if (err)
		goto err_init_miibus;

	strlcpy(netdev->name, "eth%d", sizeof(netdev->name));
	if (!is_valid_ether_addr(netdev->dev_addr)) {
		random_ether_addr(netdev->dev_addr);
		e_info(probe, "generated random MAC address %pM\n",
		       netdev->dev_addr);
	}

	err = register_netdev(netdev);
	if (err) {
		e_err(probe, "Register netdev failed\n");
		goto err_register;
	}

	netif_carrier_off(netdev);

	ftgmac030_ptp_init(ctrl);

	err = ftgmac030_request_irq(ctrl);
	if (err)
		goto err_req_irq;

	e_info(probe, "irq %d, mapped at %p, %p\n", ctrl->irq,
	       ctrl->io_base, ctrl->mdio_base);
	e_info(probe, "osclk 297Mhz, mdc 0x%x, phy-mode %s\n",
	       ctrl->mdc_cycthr, phy_modes(ctrl->phy_interface));

	/* Dropping the reference, no idle */
	pm_runtime_put_noidle(&pdev->dev);
	return 0;

err_req_irq:
err_init_miibus:
err_register:
	kfree(ctrl->tx_ring);
	kfree(ctrl->rx_ring);
err_sw_init:
err_ioremap:
	free_netdev(netdev);
err_alloc_etherdev:
err_dma:
	return err;
}

/**
 * ftgmac030_remove - Device Removal Routine
 * @pdev: platform device information struct
 *
 * ftgmac030_remove is called by the platform bus subsystem to alert
 * the driver that it should release a device.  The could be caused by
 * a Hot-Plug event, or because the driver is going to be removed from
 * memory.
 **/
static int __exit ftgmac030_remove(struct platform_device *pdev)
{
	struct net_device *netdev = platform_get_drvdata(pdev);
	struct ftgmac030_ctrl *ctrl = netdev_priv(netdev);
	bool down = test_bit(__FTGMAC030_DOWN, &ctrl->state);

	ftgmac030_ptp_remove(ctrl);

	// The timers may be rescheduled, so explicitly disable them
	// from being rescheduled.
	if (!down)
		set_bit(__FTGMAC030_DOWN, &ctrl->state);

	cancel_work_sync(&ctrl->print_hang_task);
	cancel_work_sync(&ctrl->reset_task);

	if (ctrl->flags & FLAG_HAS_HW_TIMESTAMP) {
		if (ctrl->tx_hwtstamp_skb) {
			dev_consume_skb_any(ctrl->tx_hwtstamp_skb);
			ctrl->tx_hwtstamp_skb = NULL;
		}
	}

	if (!down)
		clear_bit(__FTGMAC030_DOWN, &ctrl->state);
	unregister_netdev(netdev);

	phy_disconnect(ctrl->phy_dev);
	mdiobus_unregister(ctrl->mii_bus);
	mdiobus_free(ctrl->mii_bus);

	kfree(ctrl->tx_ring);
	kfree(ctrl->rx_ring);

	netif_napi_del(&ctrl->napi);
	free_netdev(netdev);

	ftgmac030_free_irq(ctrl);

	pm_runtime_get_noresume(&pdev->dev);
	return 0;
}

/* freeze and thaw for Hibernation(mostly for systems with disks),
 * a.k.a. "suspend to disk". Do we need this ?
 * freeze: quiesce all devices
 * thaw: reactivate all devices
 */
static const struct dev_pm_ops ftgmac030_pm_ops = {
#ifdef CONFIG_PM_SLEEP
	.suspend	= ftgmac030_pm_suspend,
	.resume		= ftgmac030_pm_resume,
	.freeze		= ftgmac030_pm_freeze,
	.thaw		= ftgmac030_pm_thaw,
	.poweroff	= ftgmac030_pm_suspend,
	.restore	= ftgmac030_pm_resume,
#endif
	SET_RUNTIME_PM_OPS(ftgmac030_pm_runtime_suspend, ftgmac030_pm_runtime_resume,
			   ftgmac030_pm_runtime_idle)
};

#ifdef CONFIG_OF
static const struct of_device_id ftgmac030_dt_ids[] = {
	{ .compatible = "lotus,ftgmac030" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ftgmac030_dt_ids);
#endif

static struct platform_driver ftgmac030_driver = {
	.remove		= __exit_p(ftgmac030_remove),
	.driver		= {
		.name	= ftgmac030_driver_name,
		.owner	= THIS_MODULE,
		.of_match_table	= of_match_ptr(ftgmac030_dt_ids),
		.pm	= &ftgmac030_pm_ops,
	},
	.shutdown = ftgmac030_shutdown,
};

module_platform_driver_probe(ftgmac030_driver, ftgmac030_probe);

MODULE_DESCRIPTION("Lotus Ftgmac030 Ethernet MAC driver");
MODULE_AUTHOR("Lotus");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:ftgmac030");

