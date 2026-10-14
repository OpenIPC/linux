/* SPDX-License-Identifier: GPL-2.0 */
// Copyright (c) LOTUS. All rights reserved.

/* Linux FTGMAC030 Ethernet Driver main header file */

#ifndef _FTGMAC030_H_
#define _FTGMAC030_H_

#include <linux/bitops.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/io.h>
#include <linux/netdevice.h>
#include <linux/crc32.h>
#include <linux/if_vlan.h>
#include <linux/clocksource.h>
#include <linux/net_tstamp.h>
#include <linux/ptp_clock_kernel.h>
#include <linux/ptp_classify.h>
#include <linux/mii.h>
#include <linux/mdio.h>
#include <linux/pm_qos.h>

#include "regs.h"

struct ftgmac030_ctrl;

#define e_dbg(type, format, arg...) \
	netif_dbg(ctrl, type, ctrl->netdev, format, ## arg)
#define e_err(type, format, arg...) \
	netif_err(ctrl, type, ctrl->netdev, format, ## arg)
#define e_info(type, format, arg...) \
	netif_info(ctrl, type, ctrl->netdev, format, ## arg)
#define e_warn(type, format, arg...) \
	netif_warn(ctrl, type, ctrl->netdev, format, ## arg)
#define e_notice(type, format, arg...) \
	netif_notice(ctrl, type, ctrl->netdev, format, ## arg)

// Number of Transmit and Receive Descriptors must be a multiple of 8
#define REQ_TX_DESCRIPTOR_MULTIPLE  8
#define REQ_RX_DESCRIPTOR_MULTIPLE  8

// Tx/Rx descriptor defines
#define FTGMAC030_DEFAULT_TXD		64
#define FTGMAC030_MAX_TXD		4096
#define FTGMAC030_MIN_TXD		64

#define FTGMAC030_DEFAULT_RXD		64
#define FTGMAC030_MAX_RXD		4096
#define FTGMAC030_MIN_RXD		64

#define FTGMAC030_MIN_ITR_USECS		10 // 100000 irq/sec
#define FTGMAC030_MAX_ITR_USECS		10000 // 100    irq/sec

#define FTGMAC030_FC_PAUSE_TIME		0x0680 // 858 usec

// How many Rx Buffers do we bundle into one write to the hardware ?
#define FTGMAC030_RX_BUFFER_WRITE	16 /* Must be power of 2 */

#define DEFAULT_JUMBO			9234

#define LINK_TIMEOUT			100

// Count for polling __FTGMAC030_RESETTING condition every 10-20msec.
#define FTGMAC030_CHECK_RESET_COUNT	25

#define BSPETH_CFG_CRG_REG (0x134)
#define ETH_PHY_INTF       ((1UL << (0)) | (1UL << (3)))
#define ETH_PHY_RST_N       (1UL << (1))

#define BSPETH_DLY_CRG_REG (0x8940)
#define BSPETH_DLY_STAT_REG (0x1a8)

#define ETH_DLY_SEL (1UL << (0))
#define ETH_DLY_STAT (1UL << (0))

/* 0:MII or GMII,(don't support) 1:RMII, 2: RGMII */
enum ftgmac030_if_mode {
	FTGMAC030_MODE_MII_GMII,
	FTGMAC030_MODE_RMII,
	FTGMAC030_MODE_RGMII,
};

enum reset_delays {
	PRE_DELAY,
	PULSE,
	POST_DELAY,
	DELAYS_NUM,
};

// Wrappers around a pointer to a socket buffer,
//so a DMA handle can be stored along with the buffer
struct ftgmac030_buffer {
	dma_addr_t dma;
	struct sk_buff *skb;
	union {
		// Tx
		struct {
			unsigned long time_stamp;
			u16 length;
			u16 next_to_watch;
			unsigned int segs;
			unsigned int bytecount;
			u16 mapped_as_page;
		};
		// Rx
		struct {
			struct page *page;
		};
	};
};

struct ftgmac030_ring {
	struct ftgmac030_ctrl *hw;
	void *desc;			// pointer to ring memory
	dma_addr_t dma;			// phys address of ring
	unsigned int size;		// length of ring in bytes
	unsigned int count;		// number of desc. in ring

	u16 next_to_use;
	u16 next_to_clean;
	spinlock_t ntu_lock;		// protects next to use pointer
	spinlock_t ntc_lock;		// protects clean up

	void __iomem *head;
	void __iomem *tail;

	struct ftgmac030_buffer *buffer_info;

	char name[IFNAMSIZ + 5];
	u32 ims_val;
	u32 itr_val;
	int set_itr;

	struct sk_buff *rx_skb_top;
};

// board specific private data structure
struct ftgmac030_ctrl {

	struct work_struct reset_task;

	struct clk *sys_clk;

	u32 msg_enable;

	u32 rx_buffer_len;

	// track device up/down/testing state
	unsigned long state;

	/* Interrupt Throttle Rate */
	u32 itr;
	u32 itr_setting;
	u16 tx_itr;
	u16 rx_itr;

	// Tx - one ring per active queue
	struct ftgmac030_ring *tx_ring ____cacheline_aligned_in_smp;
	u32 tx_fifo_limit;

	struct napi_struct napi;

	unsigned int uncorr_errors;	// uncorrectable ECC errors
	unsigned int corr_errors;	// correctable ECC errors
	unsigned int restart_queue;

	u8 tx_timeout_factor;

	u32 tx_int_delay;

	unsigned int total_tx_bytes;
	unsigned int total_tx_packets;
	unsigned int total_rx_bytes;
	unsigned int total_rx_packets;

	u32 tx_timeout_count;
	u32 tx_dma_failed;
	u32 tx_hwtstamp_timeouts;

	bool (*clean_rx)(struct ftgmac030_ring *ring, int *work_done,
			 int work_to_do) ____cacheline_aligned_in_smp;
	void (*alloc_rx_buf)(struct ftgmac030_ring *ring, int cleaned_count,
			     gfp_t gfp);
	struct ftgmac030_ring *rx_ring;

	u64 hw_csum_err;
	u64 hw_csum_good;
	u64 rx_hdr_split;
	u32 alloc_rx_buff_failed;
	u32 rx_dma_failed;
	u32 rx_hwtstamp_cleared;

	u32 max_frame_size;
	u32 min_frame_size;
	u16 max_tx_fifo;
	u16 max_rx_fifo;

	// OS defined structs
	struct net_device *netdev;
	struct platform_device *pdev;

	void __iomem *io_base;
	void __iomem *mdio_base;
	s32 irq;

	struct mii_bus *mii_bus;
	struct phy_device *phy_dev;
	int phy_speed;
	int phy_duplex;
	int phy_link;

	u8 mdc_cycthr;
	phy_interface_t phy_interface;

	u32 high_water;		// Flow control high-water mark
	u32 low_water;		// Flow control low-water mark
	u16 pause_time;		// Flow control pause timer

	u16 mta_reg_count;

	// Maximum size of the MTA register table in all supported hws
#define MAX_MTA_REG 2
	u32 mta_shadow[MAX_MTA_REG];

	u32 max_hw_frame_size;

	unsigned int flags;
	struct work_struct print_hang_task;

	int phy_hang_count;

	u16 tx_ring_count;
	u16 rx_ring_count;

	struct ftgmac030_ring test_tx_ring;
	struct ftgmac030_ring test_rx_ring;

	struct hwtstamp_config hwtstamp_config;
	struct sk_buff *tx_hwtstamp_skb;
	unsigned long tx_hwtstamp_start;
	struct work_struct tx_hwtstamp_work;
	spinlock_t systim_lock;	// PTP timestamp regsters
	u32 incval;		// increment value ns for one period
	u32 incval_nns;		// fraction part of ns incval in decimal
	struct ptp_clock *ptp_clock;
	struct ptp_clock_info ptp_clock_info;

	u32 eee_enabled;
	u32 tx_lpi_enabled;
	u32 tx_lpi_timer;
	bool clk_stop_enable;

	struct pm_qos_request pm_qos_req;

	enum ftgmac030_if_mode ftgmac030_mac_intf;
	struct reset_control *phy_rst;
	u32 ftgmac030_phy_reset_n;
	u32 phy_reset_delays[DELAYS_NUM];
	struct regmap *sysctrl_regmap;
	struct regmap *crg_regmap;
};

// hardware capability, feature, and workaround flags
#define FLAG_DISABLE_AIM                  (1 << 0)
#define FLAG_HAS_HW_TIMESTAMP             (1 << 1)
#define FLAG_HAS_WOL                      (1 << 2)
#define FLAG_HAS_JUMBO_FRAMES             (1 << 3)
#define FLAG_HAS_EEE                      (1 << 4)
#define FLAG_DISABLE_FC_PAUSE_TIME        (1 << 5)
#define FLAG_HAS_HW_VLAN_FILTER           (1 << 6)
#define FLAG_NO_WAKE_UCAST                (1 << 7)
#define FLAG_DISABLE_IPV6_RECOG           (1 << 8)
#define FLAG_DISABLE_RX_BROADCAST_PERIOD  (1 << 9)

#define FTGMAC030_GET_DESC(R, i, type)	(&(((struct type *)((R).desc))[i]))
#define FTGMAC030_TX_DESC(R, i)		FTGMAC030_GET_DESC(R, i, ftgmac030_txdes)
#define FTGMAC030_RX_DESC(R, i)		FTGMAC030_GET_DESC(R, i, ftgmac030_rxdes)

enum ftgmac030_state_t {
	__FTGMAC030_TESTING,
	__FTGMAC030_RESETTING,
	__FTGMAC030_ACCESS_SHARED_RESOURCE,
	__FTGMAC030_DOWN
};

enum latency_range {
	lowest_latency = 0,
	low_latency = 1,
	bulk_latency = 2,
	latency_invalid = 255
};

extern char ftgmac030_driver_name[];
extern const char ftgmac030_driver_version[];

void ftgmac030_check_options(struct ftgmac030_ctrl *ctrl);
void ftgmac030_set_ethtool_ops(struct net_device *netdev);

int ftgmac030_open(struct net_device *netdev);
int ftgmac030_close(struct net_device *netdev);
int ftgmac030_up(struct ftgmac030_ctrl *ctrl);
void ftgmac030_down(struct ftgmac030_ctrl *ctrl, bool reset);
void ftgmac030_reinit_locked(struct ftgmac030_ctrl *ctrl);
void ftgmac030_reset(struct ftgmac030_ctrl *ctrl);
void ftgmac030_power_up_phy(struct ftgmac030_ctrl *ctrl);
int ftgmac030_setup_rx_resources(struct ftgmac030_ring *ring);
int ftgmac030_setup_tx_resources(struct ftgmac030_ring *ring);
void ftgmac030_free_rx_resources(struct ftgmac030_ring *ring);
void ftgmac030_free_tx_resources(struct ftgmac030_ring *ring);
void ftgmac030_reset_interrupt_capability(struct ftgmac030_ctrl *ctrl);
void ftgmac030_get_hw_control(struct ftgmac030_ctrl *ctrl);
void ftgmac030_release_hw_control(struct ftgmac030_ctrl *ctrl);
void ftgmac030_write_itr(struct ftgmac030_ctrl *ctrl, u32 itr);

extern unsigned int copyBreak;

void ftgmac030_ptp_init(struct ftgmac030_ctrl *ctrl);
void ftgmac030_ptp_remove(struct ftgmac030_ctrl *ctrl);


static inline u32 __ior32(struct ftgmac030_ctrl *ctrl, unsigned long reg)
{
	return ioread32(ctrl->io_base + reg);
}

#define ior32(reg)	__ior32(ctrl, reg)

static void __iow32(struct ftgmac030_ctrl *ctrl, unsigned long reg, u32 val)
{
	iowrite32(val, ctrl->io_base + reg);
}

#define iow32(reg, val)	__iow32(ctrl, reg, val)

#define FTGMAC030_WRITE_REG_ARRAY(reg, offset, value) \
	(__iow32(ctrl, (reg + ((offset) << 2)), (value)))

#define FTGMAC030_READ_REG_ARRAY(a, reg, offset) \
	(readl((a)->io_base + reg + ((offset) << 2)))

#endif // _FTGMAC030_H_
