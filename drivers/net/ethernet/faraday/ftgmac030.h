/*
 * Faraday FTGMAC030 Gigabit Ethernet
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

#ifndef __FTGMAC030_H
#define __FTGMAC030_H

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/phy.h>
#include <linux/netdevice.h>
#include <linux/clocksource.h>
#include <linux/net_tstamp.h>
#include <linux/ptp_clock_kernel.h>
#include <linux/ptp_classify.h>

#define DRV_NAME	"ftgmac100-0"
#define DRV_1_NAME	"ftgmac100-1"

#define RX_QUEUE_ENTRIES	256	/* must be power of 2 */
#define TX_QUEUE_ENTRIES	512	/* must be power of 2 */

//#define CONFIG_FTGMAC030_PTP

#define FTGMAC030_REG_ISR               0x00
#define FTGMAC030_REG_IER               0x04
#define FTGMAC030_REG_MAC_MADR          0x08
#define FTGMAC030_REG_MAC_LADR          0x0c
#define FTGMAC030_REG_MAHT0             0x10
#define FTGMAC030_REG_MAHT1             0x14
#define FTGMAC030_REG_NPTXPD            0x18
#define FTGMAC030_REG_RXPD              0x1c
#define FTGMAC030_REG_NPTXR_BADR        0x20
#define FTGMAC030_REG_RXR_BADR          0x24
#define FTGMAC030_REG_HPTXPD            0x28
#define FTGMAC030_REG_HPTXR_BADR	0x2c
#define FTGMAC030_REG_TXITC		0x30
#define FTGMAC030_REG_RXITC		0x34
#define FTGMAC030_REG_APTC		0x38
#define FTGMAC030_REG_DBLAC		0x3C
#define FTGMAC030_REG_DMAFIFOS          0x40
#define FTGMAC030_REG_TPAFCR            0x48
#define FTGMAC030_REG_RBSR              0x4c
#define FTGMAC030_REG_MACCR		0x50
#define FTGMAC030_REG_MACSR		0x54
#define FTGMAC030_REG_TM		0x58
#define FTGMAC030_REG_PHYCR		0x60
#define FTGMAC030_REG_PHYDATA           0x64
#define FTGMAC030_REG_FCR               0x68
#define FTGMAC030_REG_BPR               0x6c
#define FTGMAC030_REG_WOLCR             0x70
#define FTGMAC030_REG_WOLSR             0x74
#define FTGMAC030_REG_WFCRC             0x78
#define FTGMAC030_REG_WFBM1             0x80
#define FTGMAC030_REG_WFBM2             0x84
#define FTGMAC030_REG_WFBM3             0x88
#define FTGMAC030_REG_WFBM4		0x8c
#define FTGMAC030_REG_NPTXR_PTR	        0x90
#define FTGMAC030_REG_HPTXR_PTR	        0x94
#define FTGMAC030_REG_RXR_PTR	        0x98
#define FTGMAC030_REG_TX_CNT		0xA0
#define FTGMAC030_REG_TX_MCOL_SCOL	0xA4
#define FTGMAC030_REG_TX_ECOL_FAIL	0xA8
#define FTGMAC030_REG_TX_LCOL_UND	0xAc
#define FTGMAC030_REG_RX_CNT		0xB0
#define FTGMAC030_REG_RX_BC		0xB4
#define FTGMAC030_REG_RX_MC		0xB8
#define FTGMAC030_REG_RX_PF_AEP         0xBC
#define FTGMAC030_REG_RX_RUNT           0xC0
#define FTGMAC030_REG_RX_CRCER_FTL      0xC4
#define FTGMAC030_REG_RX_COL_LOST       0xC8
#define FTGMAC030_REG_BIST       	0xCC
#define FTGMAC030_REG_GISR       	0xE8
#define FTGMAC030_REG_EEE       	0xF0
#define FTGMAC030_REG_REVR              0xF4
#define FTGMAC030_REG_FEAR              0xF8
#define FTGMAC030_REG_PTP_TX_SEC        0x110 /* Sync and Delay_req */
#define FTGMAC030_REG_PTP_TX_NSEC       0x114
#define FTGMAC030_REG_PTP_RX_SEC        0x118
#define FTGMAC030_REG_PTP_RX_NSEC       0x11C
#define FTGMAC030_REG_PTP_TX_P_SEC      0x120 /* PDelay_req and PDelay_resp */
#define FTGMAC030_REG_PTP_TX_P_NSEC     0x124
#define FTGMAC030_REG_PTP_RX_P_SEC      0x128
#define FTGMAC030_REG_PTP_RX_P_NSEC     0x12C
#define FTGMAC030_REG_PTP_TM_NNSEC      0x130
#define FTGMAC030_REG_PTP_TM_NSEC       0x134
#define FTGMAC030_REG_PTP_TM_SEC        0x138
#define FTGMAC030_REG_PTP_NS_PERIOD     0x13C /* max is 255 ns */
#define FTGMAC030_REG_PTP_NNS_PERIOD    0x140 /* max is 0.999999999 ns */
#define FTGMAC030_REG_PTP_PERIOD_INCR   0x144 /* add xx ns every xx cycles */
#define FTGMAC030_REG_PTP_ADJ_VAL	0x148

/*
 * Interrupt status register & interrupt enable register
 */
#define FTGMAC030_INT_RPKT_BUF          (1 << 0)
#define FTGMAC030_INT_RPKT_FIFO         (1 << 1)
#define FTGMAC030_INT_NO_RXBUF          (1 << 2)
#define FTGMAC030_INT_RPKT_LOST         (1 << 3)
#define FTGMAC030_INT_XPKT_ETH          (1 << 4)
#define FTGMAC030_INT_XPKT_FIFO         (1 << 5)
#define FTGMAC030_INT_NO_NPTXBUF        (1 << 6)
#define FTGMAC030_INT_XPKT_LOST         (1 << 7)
#define FTGMAC030_INT_AHB_ERR           (1 << 8)
#define FTGMAC030_INT_PHYSTS_CHG        (1 << 9)
#define FTGMAC030_INT_NO_HPTXBUF        (1 << 10)
#define FTGMAC030_INT_SYNC_IN           (1 << 17)
#define FTGMAC030_INT_SYNC_OUT          (1 << 18)
#define FTGMAC030_INT_DELAY_REQ_IN      (1 << 19)
#define FTGMAC030_INT_DELAY_REQ_OUT     (1 << 20)
#define FTGMAC030_INT_PDELAY_REQ_IN     (1 << 21)
#define FTGMAC030_INT_PDELAY_REQ_OUT    (1 << 22)
#define FTGMAC030_INT_PDELAY_RESP_IN    (1 << 23)
#define FTGMAC030_INT_PDELAY_RESP_OUT   (1 << 24)
#define FTGMAC030_INT_TX_TMSP_VALID     (FTGMAC030_INT_SYNC_OUT | FTGMAC030_INT_DELAY_REQ_OUT | \
					 FTGMAC030_INT_PDELAY_REQ_OUT | FTGMAC030_INT_PDELAY_RESP_OUT)
#define FTGMAC030_INT_RX_TMSP_VALID     (FTGMAC030_INT_SYNC_IN | FTGMAC030_INT_DELAY_REQ_IN | \
					 FTGMAC030_INT_PDELAY_REQ_IN | FTGMAC030_INT_PDELAY_RESP_IN)

/*
 * Interrupt timer control register
 */
#define FTGMAC030_ITC_RXINT_CNT(x)	(((x) & 0xf) << 0)
#define FTGMAC030_ITC_RXINT_THR(x)	(((x) & 0x7) << 4)
#define FTGMAC030_ITC_RXINT_TIME_SEL	(1 << 7)
#define FTGMAC030_ITC_TXINT_CNT(x)	(((x) & 0xf) << 8)
#define FTGMAC030_ITC_TXINT_THR(x)	(((x) & 0x7) << 12)
#define FTGMAC030_ITC_TXINT_TIME_SEL	(1 << 15)

/*
 * Automatic polling timer control register
 */
#define FTGMAC030_APTC_RXPOLL_CNT(x)	(((x) & 0xf) << 0)
#define FTGMAC030_APTC_RXPOLL_TIME_SEL	(1 << 4)
#define FTGMAC030_APTC_TXPOLL_CNT(x)	(((x) & 0xf) << 8)
#define FTGMAC030_APTC_TXPOLL_TIME_SEL	(1 << 12)

/*
 * DMA burst length and arbitration control register
 */
#define FTGMAC030_DBLAC_RXFIFO_LTHR(x)	(((x) & 0x7) << 0)
#define FTGMAC030_DBLAC_RXFIFO_HTHR(x)	(((x) & 0x7) << 3)
#define FTGMAC030_DBLAC_RX_THR_EN	(1 << 6)
#define FTGMAC030_DBLAC_RXBURST_SIZE(x)	(((x) & 0x3) << 8)
#define FTGMAC030_DBLAC_TXBURST_SIZE(x)	(((x) & 0x3) << 10)
#define FTGMAC030_DBLAC_RXDES_SIZE(x)	(((x) & 0xf) << 12)
#define FTGMAC030_DBLAC_TXDES_SIZE(x)	(((x) & 0xf) << 16)
#define FTGMAC030_DBLAC_IFG_CNT(x)	(((x) & 0x7) << 20)
#define FTGMAC030_DBLAC_IFG_INC		(1 << 23)

/*
 * DMA FIFO status register
 */
#define FTGMAC030_DMAFIFOS_RXDMA1_SM(dmafifos)	((dmafifos) & 0xf)
#define FTGMAC030_DMAFIFOS_RXDMA2_SM(dmafifos)	(((dmafifos) >> 4) & 0xf)
#define FTGMAC030_DMAFIFOS_RXDMA3_SM(dmafifos)	(((dmafifos) >> 8) & 0x7)
#define FTGMAC030_DMAFIFOS_TXDMA1_SM(dmafifos)	(((dmafifos) >> 12) & 0xf)
#define FTGMAC030_DMAFIFOS_TXDMA2_SM(dmafifos)	(((dmafifos) >> 16) & 0x3)
#define FTGMAC030_DMAFIFOS_TXDMA3_SM(dmafifos)	(((dmafifos) >> 18) & 0xf)
#define FTGMAC030_DMAFIFOS_RXFIFO_EMPTY		(1 << 26)
#define FTGMAC030_DMAFIFOS_TXFIFO_EMPTY		(1 << 27)
#define FTGMAC030_DMAFIFOS_RXDMA_GRANT		(1 << 28)
#define FTGMAC030_DMAFIFOS_TXDMA_GRANT		(1 << 29)
#define FTGMAC030_DMAFIFOS_RXDMA_REQ		(1 << 30)
#define FTGMAC030_DMAFIFOS_TXDMA_REQ		(1 << 31)

/*
 * Receive buffer size register
 */
#define FTGMAC030_RBSR_SIZE(x)		((x) & 0x3fff)

/*
 * MAC control register
 */
#define FTGMAC030_MACCR_TXDMA_EN	(1 << 0)
#define FTGMAC030_MACCR_RXDMA_EN	(1 << 1)
#define FTGMAC030_MACCR_TXMAC_EN	(1 << 2)
#define FTGMAC030_MACCR_RXMAC_EN	(1 << 3)
#define FTGMAC030_MACCR_RX_ALL		(1 << 8)
#define FTGMAC030_MACCR_HT_MULTI_EN	(1 << 9)
#define FTGMAC030_MACCR_RX_MULTIPKT	(1 << 10)
#define FTGMAC030_MACCR_RX_BROADPKT	(1 << 11)
#define FTGMAC030_MACCR_RX_RUNT		(1 << 12)
#define FTGMAC030_MACCR_JUMBO_LF	(1 << 13)
#define FTGMAC030_MACCR_ENRX_IN_HALFTX	(1 << 14)
#define FTGMAC030_MACCR_DISCARD_CRCERR	(1 << 16)
#define FTGMAC030_MACCR_CRC_APD		(1 << 17)
#define FTGMAC030_MACCR_REMOVE_VLAN	(1 << 18)
#define FTGMAC030_MACCR_PTP_EN		(1 << 20)
#define FTGMAC030_MACCR_LOOP_EN		(1 << 21)
#define FTGMAC030_MACCR_HPTXR_EN	(1 << 22)
#define FTGMAC030_MACCR_GIGA_MODE	(2 << 24)
#define FTGMAC030_MACCR_FAST_MODE	(1 << 24)
#define FTGMAC030_MACCR_FULLDUP		(1 << 26)
#define FTGMAC030_MACCR_SW_RST		(1 << 31)

/*
 * PHY control register
 */
#define FTGMAC030_PHYCR_MDC_CYCTHR_MASK         0x3f
#define FTGMAC030_PHYCR_MDC_CYCTHR(x)           ((x) & 0x3f)
#define FTGMAC030_PHYCR_OP(x)           	(((x) & 0x3) << 14)
#define FTGMAC030_PHYCR_ST(x)           	(((x) & 0x3) << 12)
#define FTGMAC030_PHYCR_PHYAD(x)                (((x) & 0x1f) << 16)
#define FTGMAC030_PHYCR_REGAD(x)                (((x) & 0x1f) << 21)
#define FTGMAC030_PHYCR_MIIRD                   (1 << 26)
#define FTGMAC030_PHYCR_MIIWR                   (1 << 27)

/*
 * PHY data register
 */
#define FTGMAC030_PHYDATA_MIIWDATA(x)           ((x) & 0xffff)
#define FTGMAC030_PHYDATA_MIIRDATA(phydata)     (((phydata) >> 16) & 0xffff)

/*
 * Feature Register (0x44)
 */
#define FTGMAC030_FEAR_TFIFO_RSIZE(x) ((x & 0x70) >> 4)
#define FTGMAC030_FEAR_RFIFO_RSIZE(x) ((x & 0x7) >> 0)

/*
 * Transmit descriptor, aligned to 16 bytes
 */
struct ftgmac030_txdes {
	unsigned int	txdes0;
	unsigned int	txdes1;
	unsigned int	txdes2;	/* not used by HW */
	unsigned int	txdes3;	/* TXBUF_BADR */
} __attribute__ ((aligned(16)));

#define FTGMAC030_TXDES0_TXBUF_SIZE(x)	((x) & 0x3fff)
#define FTGMAC030_TXDES0_EDOTR		(1 << 15)
#define FTGMAC030_TXDES0_CRC_ERR	(1 << 19)
#define FTGMAC030_TXDES0_LTS		(1 << 28)
#define FTGMAC030_TXDES0_FTS		(1 << 29)
#define FTGMAC030_TXDES0_TXDMA_OWN	(1 << 31)

#define FTGMAC030_TXDES1_VLANTAG_CI(x)	((x) & 0xffff)
#define FTGMAC030_TXDES1_INS_VLANTAG	(1 << 16)
#define FTGMAC030_TXDES1_TCP_CHKSUM	(1 << 17)
#define FTGMAC030_TXDES1_UDP_CHKSUM	(1 << 18)
#define FTGMAC030_TXDES1_IP_CHKSUM	(1 << 19)
#define FTGMAC030_TXDES1_IPV6_PKT       (1 << 21)
#define FTGMAC030_TXDES1_LLC		(1 << 22)
#define FTGMAC030_TXDES1_TX2FIC		(1 << 30)
#define FTGMAC030_TXDES1_TXIC		(1 << 31)

/*
 * Receive descriptor, aligned to 16 bytes
 */
struct ftgmac030_rxdes {
	unsigned int	rxdes0;
	unsigned int	rxdes1;
	unsigned int	rxdes2;	/* not used by HW */
	unsigned int	rxdes3;	/* RXBUF_BADR */
} __attribute__ ((aligned(16)));

#define FTGMAC030_RXDES0_VDBC		0x3fff
#define FTGMAC030_RXDES0_EDORR		(1 << 15)
#define FTGMAC030_RXDES0_MULTICAST	(1 << 16)
#define FTGMAC030_RXDES0_BROADCAST	(1 << 17)
#define FTGMAC030_RXDES0_RX_ERR		(1 << 18)
#define FTGMAC030_RXDES0_CRC_ERR	(1 << 19)
#define FTGMAC030_RXDES0_FTL		(1 << 20)
#define FTGMAC030_RXDES0_RUNT		(1 << 21)
#define FTGMAC030_RXDES0_RX_ODD_NB	(1 << 22)
#define FTGMAC030_RXDES0_FIFO_FULL	(1 << 23)
#define FTGMAC030_RXDES0_PAUSE_OPCODE	(1 << 24)
#define FTGMAC030_RXDES0_PAUSE_FRAME	(1 << 25)
#define FTGMAC030_RXDES0_LRS		(1 << 28)
#define FTGMAC030_RXDES0_FRS		(1 << 29)
#define FTGMAC030_RXDES0_RXPKT_RDY	(1 << 31)

#define FTGMAC030_RXDES1_IPV6		(1 << 19)
#define FTGMAC030_RXDES1_VLANTAG_CI	0xffff
#define FTGMAC030_RXDES1_PROT_MASK	(0x3 << 20)
#define FTGMAC030_RXDES1_PROT_NONIP	(0x0 << 20)
#define FTGMAC030_RXDES1_PROT_IP	(0x1 << 20)
#define FTGMAC030_RXDES1_PROT_TCPIP	(0x2 << 20)
#define FTGMAC030_RXDES1_PROT_UDPIP	(0x3 << 20)
#define FTGMAC030_RXDES1_LLC		(1 << 22)
#define FTGMAC030_RXDES1_DF		(1 << 23)
#define FTGMAC030_RXDES1_VLANTAG_AVAIL	(1 << 24)
#define FTGMAC030_RXDES1_TCP_CHKSUM_ERR	(1 << 25)
#define FTGMAC030_RXDES1_UDP_CHKSUM_ERR	(1 << 26)
#define FTGMAC030_RXDES1_IP_CHKSUM_ERR	(1 << 27)

/******************************************************************************
 * private data
 *****************************************************************************/
struct ftgmac030_descs {
	struct ftgmac030_rxdes rxdes[RX_QUEUE_ENTRIES];
	struct ftgmac030_txdes txdes[TX_QUEUE_ENTRIES];
};

struct ftgmac030 {
	struct resource *res;
	void __iomem *base;
	int irq;

	struct ftgmac030_descs *descs;
	dma_addr_t descs_dma_addr;

	unsigned int rx_pointer;
	unsigned int tx_clean_pointer;
	unsigned int tx_pointer;
	unsigned int tx_pending;

	spinlock_t tx_lock;

	struct net_device *netdev;
	struct device *dev;
	struct napi_struct napi;

	struct mii_bus *mii_bus;
	int phy_irq[PHY_MAX_ADDR];
	struct phy_device *phydev;
	int old_speed;
	int old_duplex;

#ifdef CONFIG_FTGMAC030_PTP
	struct hwtstamp_config hwtstamp_config;
	struct delayed_work systim_overflow_work;
	struct sk_buff *tx_hwtstamp_skb;
	struct work_struct tx_hwtstamp_work;
	spinlock_t systim_lock;	/* protects SYSTIML/H regsters */
	struct ptp_clock *ptp_clock;
	struct ptp_clock_info ptp_clock_info;
#endif

};

void wait_status(int millisecond);
int mac_scu_init(int irq);
void mac_scu_close(int irq);
void set_mac_clock(int irq);
void set_MDC_CLK(struct ftgmac030 *priv);

#ifdef CONFIG_FTGMAC030_PTP

#define FTGMAC030_SYSTIM_OVERFLOW_PERIOD	(HZ * 60 * 60)

extern void ftgmac030_ptp_init(struct ftgmac030 *adapter);
extern u32 REG_PTP_TM_SEC;
extern u32 REG_PTP_TM_NSEC;
extern struct ptp_clock *ptp_clock_register(struct ptp_clock_info *info);
extern int ptp_clock_unregister(struct ptp_clock *ptp);
#endif

#endif /* __FTGMAC030_H */
