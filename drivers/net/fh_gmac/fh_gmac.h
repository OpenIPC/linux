/*
 * fh_gmac.h
 *
 *  Created on: May 22, 2014
 *      Author: duobao
 */

#ifndef FH_GMAC_H_
#define FH_GMAC_H_

#include <linux/phy.h>
#include <linux/etherdevice.h>

#include "fh_gmac_phyt.h"
#include "fh_gmac_dma.h"

//GMAC-MAC
#define		REG_GMAC_CONFIG				(0x0000)
#define		REG_GMAC_FRAME_FILTER		(0x0004)
#define		REG_GMAC_HASH_HIGH			(0x0008)
#define		REG_GMAC_HASH_LOW			(0x000C)
#define		REG_GMAC_GMII_ADDRESS		(0x0010)
#define		REG_GMAC_GMII_DATA			(0x0014)
#define		REG_GMAC_FLOW_CTRL			(0x0018)
#define		REG_GMAC_DEBUG				(0x0024)
#define		REG_GMAC_MAC_HIGH 			(0x0040)
#define		REG_GMAC_MAC_LOW 			(0x0044)
//GMAC-DMA
#define		REG_GMAC_BUS_MODE			(0x1000)
#define		REG_GMAC_TX_POLL_DEMAND		(0x1004)
#define		REG_GMAC_RX_POLL_DEMAND		(0x1008)
#define		REG_GMAC_RX_DESC_ADDR		(0x100C)
#define		REG_GMAC_TX_DESC_ADDR		(0x1010)
#define		REG_GMAC_STATUS				(0x1014)
#define		REG_GMAC_OP_MODE			(0x1018)
#define		REG_GMAC_INTR_EN			(0x101C)
#define		REG_GMAC_ERROR_COUNT		(0x1020)
#define		REG_GMAC_AXI_BUS_MODE		(0x1028)
#define		REG_GMAC_AXI_STATUS			(0x102C)
#define 	REG_GMAC_CURR_TX_DESC		(0x1048)
#define 	REG_GMAC_CURR_RX_DESC		(0x104C)

enum tx_dma_irq_status {
	tx_hard_error = 1,
	tx_hard_error_bump_tc = 2,
	handle_tx_rx = 3,
};

enum rx_frame_status {
	good_frame = 0,
	discard_frame = 1,
	csum_none = 2,
	llc_snap = 4,
};

#define GMAC_MIN_ETHPKTSIZE		(60) /* Minimum ethernet pkt size */
#define GMAC_MAX_FRAME_SIZE		(1500 + 14 + 4 + 4)

#define BUFFER_SIZE_2K	2048
#define BUFFER_SIZE_4K	4096
#define BUFFER_SIZE_8K	8192
#define BUFFER_SIZE_16K	16384

#ifdef FH_GMAC_DMA_DEBUG
#define GMAC_DMA_DBG(fmt, args...)  printk(fmt, ## args)
#else
#define GMAC_DMA_DBG(fmt, args...)  do { } while (0)
#endif

#ifdef FH_GMAC_XMIT_DEBUG
#define TX_DBG(fmt, args...)  printk(fmt, ## args)
#else
#define TX_DBG(fmt, args...)  do { } while (0)
#endif

#ifdef FH_GMAC_RX_DEBUG
#define RX_DBG(fmt, args...)  printk(fmt, ## args)
#else
#define RX_DBG(fmt, args...)  do { } while (0)
#endif

#define FH_GMAC_DEBUG	(	NETIF_MSG_DRV | \
								NETIF_MSG_PROBE | \
								NETIF_MSG_LINK | \
								NETIF_MSG_TIMER | \
								NETIF_MSG_IFDOWN | \
								NETIF_MSG_IFUP | \
								NETIF_MSG_RX_ERR | \
								NETIF_MSG_TX_ERR | \
								NETIF_MSG_TX_QUEUED | \
								NETIF_MSG_INTR | \
								NETIF_MSG_TX_DONE | \
								NETIF_MSG_RX_STATUS | \
								NETIF_MSG_PKTDATA | \
								NETIF_MSG_HW | \
								NETIF_MSG_WOL			)

enum {
	gmac_gmii_clock_60_100,
	gmac_gmii_clock_100_150,
	gmac_gmii_clock_20_35,
	gmac_gmii_clock_35_60,
	gmac_gmii_clock_150_250,
	gmac_gmii_clock_250_300
};

enum {
	gmac_interrupt_all = 0x0001ffff,
	gmac_interrupt_none = 0x0
};

typedef struct Gmac_Stats {
	/* Transmit errors */
	unsigned long tx_underflow ____cacheline_aligned;
	unsigned long tx_carrier;
	unsigned long tx_losscarrier;
	unsigned long tx_heartbeat;
	unsigned long tx_deferred;
	unsigned long tx_vlan;
	unsigned long tx_jabber;
	unsigned long tx_frame_flushed;
	unsigned long tx_payload_error;
	unsigned long tx_ip_header_error;
	/* Receive errors */
	unsigned long rx_desc;
	unsigned long rx_partial;
	unsigned long rx_runt;
	unsigned long rx_toolong;
	unsigned long rx_collision;
	unsigned long rx_crc;
	unsigned long rx_length;
	unsigned long rx_mii;
	unsigned long rx_multicast;
	unsigned long rx_gmac_overflow;
	unsigned long rx_watchdog;
	unsigned long da_rx_filter_fail;
	unsigned long sa_rx_filter_fail;
	unsigned long rx_missed_cntr;
	unsigned long rx_overflow_cntr;
	/* Tx/Rx IRQ errors */
	unsigned long tx_undeflow_irq;
	unsigned long tx_process_stopped_irq;
	unsigned long tx_jabber_irq;
	unsigned long rx_overflow_irq;
	unsigned long rx_buf_unav_irq;
	unsigned long rx_process_stopped_irq;
	unsigned long rx_watchdog_irq;
	unsigned long tx_early_irq;
	unsigned long fatal_bus_error_irq;
	/* Extra info */
	unsigned long threshold;
	unsigned long tx_pkt_n;
	unsigned long rx_pkt_n;
	unsigned long poll_n;
	unsigned long sched_timer_n;
	unsigned long normal_irq_n;
}Gmac_Stats;

typedef struct Gmac_Object {
	Gmac_Tx_DMA_Descriptors* tx_dma_descriptors	____cacheline_aligned;
	Gmac_Rx_DMA_Descriptors* rx_dma_descriptors;
	int full_duplex;			//read only
	int speed_100m;				//read only

	struct sk_buff_head rx_recycle;
	struct sk_buff** rx_skbuff;
	struct sk_buff** tx_skbuff;
	dma_addr_t* rx_skbuff_dma;
	__u32 cur_rx;
	__u32 dirty_rx;
	__u32 cur_tx;
	__u32 dirty_tx;
	dma_addr_t tx_bus_addr;
	dma_addr_t rx_bus_addr;
	__u32 dma_tx_size;
	__u32 dma_rx_size;
	__u32 dma_buf_sz;

	spinlock_t lock;

	void __iomem *remap_addr;
	__u8 local_mac_address[6];
	__u32 msg_enable;
	struct device* dev;
	struct net_device* ndev;
	struct platform_device* pdev;
	struct napi_struct napi;
	struct mii_bus *mii;
	struct phy_device *phydev;
	Gmac_Stats stats;

	int oldlink;
	int speed;
	int oldduplex;
	__u32 flow_ctrl;
	__u32 pause;

	int wolopts;
	int wolenabled;

	int phy_interface;
	struct fh_gmac_platform_data* priv_data;

	struct clk* clk;

}Gmac_Object;

#define TX_TIMEO 5000 /* default 5 seconds */
#define DMA_RX_SIZE 256
#define DMA_TX_SIZE 256
#define FLOW_OFF 0
#define FLOW_RX		4
#define FLOW_TX		2
#define FLOW_AUTO	(FLOW_TX | FLOW_RX)
#define PAUSE_TIME 0x200

int fh_mdio_register(struct net_device *ndev);
int fh_mdio_unregister(struct net_device *ndev);

void GMAC_DMA_StartTx(Gmac_Object* pGmac);
void GMAC_DMA_StopTx(Gmac_Object* pGmac);
void GMAC_DMA_StartRx(Gmac_Object* pGmac);
void GMAC_DMA_StopRx(Gmac_Object* pGmac);

void fh_gmac_set_ethtool_ops(struct net_device *netdev);

void GMAC_DMA_InitDescRings(struct net_device *ndev);
int GMAC_DMA_Init(struct net_device *ndev, __u32 dma_tx, __u32 dma_rx);
void GMAC_DMA_InitRxDesc(Gmac_Rx_DMA_Descriptors* desc, unsigned int size);
void GMAC_DMA_InitTxDesc(Gmac_Tx_DMA_Descriptors* desc, unsigned int size);
void GMAC_DMA_OpMode(Gmac_Object* pGmac);
void GMAC_DMA_FreeDesc(Gmac_Object* pGmac);
void GMAC_DMA_FreeRxSkbufs(Gmac_Object* pGmac);
void GMAC_DMA_FreeTxSkbufs(Gmac_Object* pGmac);
void GMAC_DMA_DisplayRxDesc(Gmac_Rx_DMA_Descriptors* desc, int size);
void GMAC_DMA_DisplayTxDesc(Gmac_Tx_DMA_Descriptors* desc, int size);
int GMAC_DMA_Interrupt(Gmac_Object* pGmac);
int GMAC_DMA_TxStatus(Gmac_Object* pGmac, Gmac_Tx_DMA_Descriptors* desc);
int GMAC_DMA_RxStatus(Gmac_Object* pGmac, Gmac_Rx_DMA_Descriptors* desc);
void GMAC_DMA_ReleaseTxDesc(Gmac_Tx_DMA_Descriptors* desc);
void GMAC_DMA_DiagnosticFrame(void *data, Gmac_Object* pGmac);
void GMAC_FlowCtrl(Gmac_Object * pGmac, unsigned int duplex, unsigned int fc,
		   unsigned int pause_time);

#endif /* FH_GMAC_H_ */
