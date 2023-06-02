/*
 * /drivers/net/ethernet/ambarella/ambarella_eth_normal.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 * Copyright (C) 2004-2011, Ambarella, Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_net.h>
#include <linux/of_mdio.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/crc32.h>
#include <linux/time.h>
#include <linux/mii.h>
#include <linux/phy.h>
#include <linux/ethtool.h>

#include <asm/dma.h>

#include <mach/hardware.h>
#include <plat/eth.h>
#include <plat/rct.h>
/* ==========================================================================*/
#define AMBETH_NAPI_WEIGHT		32
#define AMBETH_TX_WATCHDOG		(2 * HZ)
#define AMBETH_MII_RETRY_CNT		200
#define AMBETH_FC_PAUSE_TIME		1954

#define AMBETH_PACKET_MAXFRAME		(1536)
#define AMBETH_RX_COPYBREAK		(1518)
#define AMBETH_RX_RNG_MIN		(8)
#define AMBETH_TX_RNG_MIN		(4)
#define AMBETH_PHY_REG_SIZE		(32)

#define AMBETH_RXDMA_STATUS	(ETH_DMA_STATUS_OVF | ETH_DMA_STATUS_RI | \
				ETH_DMA_STATUS_RU | ETH_DMA_STATUS_RPS | \
				ETH_DMA_STATUS_RWT)
#define AMBETH_RXDMA_INTEN	(ETH_DMA_INTEN_OVE | ETH_DMA_INTEN_RIE | \
				ETH_DMA_INTEN_RUE | ETH_DMA_INTEN_RSE | \
				ETH_DMA_INTEN_RWE)
#define AMBETH_TXDMA_STATUS	(ETH_DMA_STATUS_TI | ETH_DMA_STATUS_TPS | \
				ETH_DMA_STATUS_TU | ETH_DMA_STATUS_TJT | \
				ETH_DMA_STATUS_UNF)
#if defined(CONFIG_NET_VENDOR_AMBARELLA_INTEN_TUE)
#define AMBETH_TXDMA_INTEN	(ETH_DMA_INTEN_TIE | ETH_DMA_INTEN_TSE | \
				ETH_DMA_INTEN_TUE | ETH_DMA_INTEN_TJE | \
				ETH_DMA_INTEN_UNE)
#else
#define AMBETH_TXDMA_INTEN	(ETH_DMA_INTEN_TIE | ETH_DMA_INTEN_TSE | \
				ETH_DMA_INTEN_TJE | ETH_DMA_INTEN_UNE)
#endif
#define AMBETH_DMA_INTEN	(ETH_DMA_INTEN_NIE | ETH_DMA_INTEN_AIE | \
				ETH_DMA_INTEN_FBE | AMBETH_RXDMA_INTEN | \
				AMBETH_TXDMA_INTEN)

/* ==========================================================================*/
struct ambeth_rng_info {
	struct sk_buff			*skb;
	dma_addr_t			mapping;
};

struct ambeth_tx_rngmng {
	unsigned int			cur_tx;
	unsigned int			dirty_tx;
	struct ambeth_rng_info		*rng_tx;
	struct ambeth_desc		*desc_tx;
};

struct ambeth_rx_rngmng {
	unsigned int			cur_rx;
	unsigned int			dirty_rx;
	struct ambeth_rng_info		*rng_rx;
	struct ambeth_desc		*desc_rx;
};

struct ambeth_info {
	unsigned int			rx_count;
	struct ambeth_rx_rngmng		rx;
	unsigned int			tx_count;
	unsigned int			tx_irq_low;
	unsigned int			tx_irq_high;
	struct ambeth_tx_rngmng		tx;
	dma_addr_t			rx_dma_desc;
	dma_addr_t			tx_dma_desc;
	spinlock_t			lock;
	int				oldspeed;
	int				oldduplex;
	int				oldlink;
	int				oldpause;
	int				oldasym_pause;
	u32				flow_ctr;

	struct net_device_stats		stats;
	struct napi_struct		napi;
	struct net_device		*ndev;

	struct mii_bus			new_bus;
	struct phy_device		*phydev;
	int				pwr_gpio;
	u8				pwr_gpio_active;
	int				rst_gpio;
	u8				rst_gpio_active;
	u32				phy_supported;
	u32				fixed_speed; /* only for phy-less */

	unsigned char __iomem		*regbase;
	u32				msg_enable;

	u32				mdio_gpio : 1,
					phy_enabled : 1,
					ipc_tx : 1,
					ipc_rx : 1,
					dump_tx : 1,
					dump_rx : 1,
					dump_rx_free : 1,
					dump_rx_all : 1;
	int				clk_direction;
};

/* ==========================================================================*/
static int msg_level = -1;
module_param (msg_level, int, 0);
MODULE_PARM_DESC (msg_level, "Override default message level");

/* ==========================================================================*/
static void amba_set_eth_desc(struct ambeth_desc *desc, u32 val) {
	u32 status = 0, length = 0;

	if(ETH_ENHANCED) {
		if(val & ETH_TDES_IC)
			status |= ETH_ENHANCED_TDES0_IC;
		if(val & ETH_TDES_LS)
			status |= ETH_ENHANCED_TDES0_LS;
		if(val & ETH_TDES_FS)
			status |= ETH_ENHANCED_TDES0_FS;
		if(val & ETH_TDES_TCH)
			status |= ETH_ENHANCED_TDES0_TCH;
		if(val & ETH_TDES_CIC)
			status |= ETH_ENHANCED_TDES0_CIC_V2;

		desc->status |= status;
	} else {
		if(val & ETH_TDES_IC)
			length |= ETH_TDES1_IC;
		if(val & ETH_TDES_LS)
			length |= ETH_TDES1_LS;
		if(val & ETH_TDES_FS)
			length |= ETH_TDES1_FS;
		if(val & ETH_TDES_TCH)
			length |= ETH_TDES1_TCH;
		if(val & ETH_TDES_CIC)
			length |= ETH_TDES1_CIC_TUI | ETH_TDES1_CIC_HDR;

		desc->length |= length;
	}

}

static void ambhw_dump(struct ambeth_info *lp)
{
	u32 i;
	unsigned int dirty_diff;
	u32 entry;

	dirty_diff = (lp->rx.cur_rx - lp->rx.dirty_rx);
	entry = (lp->rx.cur_rx % lp->rx_count);
	dev_info(&lp->ndev->dev, "RX Info: cur_rx[%u], dirty_rx[%u],"
		" diff[%u], entry[%u].\n", lp->rx.cur_rx, lp->rx.dirty_rx,
		dirty_diff, entry);
	for (i = 0; i < lp->rx_count; i++) {
		dev_info(&lp->ndev->dev, "RX Info: RX descriptor[%u] "
			"0x%08x 0x%08x 0x%08x 0x%08x.\n", i,
			lp->rx.desc_rx[i].status, lp->rx.desc_rx[i].length,
			lp->rx.desc_rx[i].buffer1, lp->rx.desc_rx[i].buffer2);
	}
	dirty_diff = (lp->tx.cur_tx - lp->tx.dirty_tx);
	entry = (lp->tx.cur_tx % lp->tx_count);
	dev_info(&lp->ndev->dev, "TX Info: cur_tx[%u], dirty_tx[%u],"
		" diff[%u], entry[%u].\n", lp->tx.cur_tx, lp->tx.dirty_tx,
		dirty_diff, entry);
	for (i = 0; i < lp->tx_count; i++) {
		dev_info(&lp->ndev->dev, "TX Info: TX descriptor[%u] "
			"0x%08x 0x%08x 0x%08x 0x%08x.\n", i,
			lp->tx.desc_tx[i].status, lp->tx.desc_tx[i].length,
			lp->tx.desc_tx[i].buffer1, lp->tx.desc_tx[i].buffer2);
	}
	for (i = 0; i <= 21; i++) {
		dev_dbg(&lp->ndev->dev, "GMAC[%d]: 0x%08x.\n", i,
		amba_readl(lp->regbase + ETH_MAC_CFG_OFFSET + (i << 2)));
	}
	for (i = 0; i <= 54; i++) {
		dev_dbg(&lp->ndev->dev, "GDMA[%d]: 0x%08x.\n", i,
		amba_readl(lp->regbase + ETH_DMA_BUS_MODE_OFFSET + (i << 2)));
	}
}

static inline int ambhw_dma_reset(struct ambeth_info *lp)
{
	int ret_val = 0;
	u32 counter = 0;

	amba_setbitsl(lp->regbase + ETH_DMA_BUS_MODE_OFFSET,
		ETH_DMA_BUS_MODE_SWR);
	do {
		if (counter++ > 100) {
			ret_val = -EIO;
			break;
		}
		mdelay(1);
	} while (amba_tstbitsl(lp->regbase + ETH_DMA_BUS_MODE_OFFSET,
		ETH_DMA_BUS_MODE_SWR));

	if (ret_val && netif_msg_drv(lp))
		dev_err(&lp->ndev->dev, "DMA Error: Check PHY.\n");

	return ret_val;
}

static inline void ambhw_dma_int_enable(struct ambeth_info *lp)
{
	amba_writel(lp->regbase + ETH_DMA_INTEN_OFFSET, AMBETH_DMA_INTEN);
}

static inline void ambhw_dma_int_disable(struct ambeth_info *lp)
{
	amba_writel(lp->regbase + ETH_DMA_INTEN_OFFSET, 0);
}

static inline void ambhw_dma_rx_start(struct ambeth_info *lp)
{
	amba_setbitsl(lp->regbase + ETH_DMA_OPMODE_OFFSET, ETH_DMA_OPMODE_SR);
}

static inline void ambhw_dma_rx_stop(struct ambeth_info *lp)
{
	unsigned int irq_status;
	int i = 1300;

	amba_clrbitsl(lp->regbase + ETH_DMA_OPMODE_OFFSET, ETH_DMA_OPMODE_SR);
	do {
		udelay(1);
		irq_status = amba_readl(lp->regbase + ETH_DMA_STATUS_OFFSET);
	} while ((irq_status & ETH_DMA_STATUS_RS_MASK) && --i);
	if ((i <= 0) && netif_msg_drv(lp)) {
		dev_err(&lp->ndev->dev,
			"DMA Error: Stop RX status=0x%x, opmode=0x%x.\n",
			amba_readl(lp->regbase + ETH_DMA_STATUS_OFFSET),
			amba_readl(lp->regbase + ETH_DMA_OPMODE_OFFSET));
	}
}

static inline void ambhw_dma_tx_start(struct ambeth_info *lp)
{
	amba_setbitsl(lp->regbase + ETH_DMA_OPMODE_OFFSET, ETH_DMA_OPMODE_ST);
}

static inline void ambhw_dma_tx_stop(struct ambeth_info *lp)
{
	unsigned int irq_status;
	int i = 1300;

	amba_clrbitsl(lp->regbase + ETH_DMA_OPMODE_OFFSET, ETH_DMA_OPMODE_ST);
	do {
		udelay(1);
		irq_status = amba_readl(lp->regbase + ETH_DMA_STATUS_OFFSET);
	} while ((irq_status & ETH_DMA_STATUS_TS_MASK) && --i);
	if ((i <= 0) && netif_msg_drv(lp)) {
		dev_err(&lp->ndev->dev,
			"DMA Error: Stop TX status=0x%x, opmode=0x%x.\n",
			amba_readl(lp->regbase + ETH_DMA_STATUS_OFFSET),
			amba_readl(lp->regbase + ETH_DMA_OPMODE_OFFSET));
	}
	amba_setbitsl(lp->regbase + ETH_DMA_OPMODE_OFFSET, ETH_DMA_OPMODE_FTF);
}

static inline void ambhw_dma_tx_restart(struct ambeth_info *lp, u32 entry)
{
	lp->tx.desc_tx[entry].status = 0;
	amba_set_eth_desc(&lp->tx.desc_tx[entry], ETH_TDES_IC);
	lp->tx.desc_tx[entry].status |= ETH_TDES0_OWN;
	amba_writel(lp->regbase + ETH_DMA_TX_DESC_LIST_OFFSET,
		(u32)lp->tx_dma_desc + (entry * sizeof(struct ambeth_desc)));
	if (netif_msg_tx_err(lp)) {
		dev_err(&lp->ndev->dev, "TX Error: restart %u.\n", entry);
		ambhw_dump(lp);
	}
	ambhw_dma_tx_start(lp);
}

static inline void ambhw_dma_tx_poll(struct ambeth_info *lp)
{
	amba_writel(lp->regbase + ETH_DMA_TX_POLL_DMD_OFFSET, 0x01);
}

static inline void ambhw_stop_tx_rx(struct ambeth_info *lp)
{
	unsigned int irq_status;
	int i = 1300;

	amba_clrbitsl(lp->regbase + ETH_MAC_CFG_OFFSET, ETH_MAC_CFG_RE);
	amba_clrbitsl(lp->regbase + ETH_DMA_OPMODE_OFFSET,
		(ETH_DMA_OPMODE_SR | ETH_DMA_OPMODE_ST));
	do {
		udelay(1);
		irq_status = amba_readl(lp->regbase + ETH_DMA_STATUS_OFFSET);
	} while ((irq_status & (ETH_DMA_STATUS_TS_MASK |
		ETH_DMA_STATUS_RS_MASK)) && --i);
	if ((i <= 0) && netif_msg_drv(lp)) {
		dev_err(&lp->ndev->dev,
			"DMA Error: Stop TX/RX status=0x%x, opmode=0x%x.\n",
			amba_readl(lp->regbase + ETH_DMA_STATUS_OFFSET),
			amba_readl(lp->regbase + ETH_DMA_OPMODE_OFFSET));
	}
	amba_clrbitsl(lp->regbase + ETH_MAC_CFG_OFFSET, ETH_MAC_CFG_TE);
}

static inline void ambhw_set_dma_desc(struct ambeth_info *lp)
{
	amba_writel(lp->regbase + ETH_DMA_RX_DESC_LIST_OFFSET,
		lp->rx_dma_desc);
	amba_writel(lp->regbase + ETH_DMA_TX_DESC_LIST_OFFSET,
		lp->tx_dma_desc);
}

static inline phy_interface_t ambhw_get_interface(struct ambeth_info *lp)
{
	return amba_tstbitsl(lp->regbase + ETH_MAC_CFG_OFFSET,
		ETH_MAC_CFG_PS) ? PHY_INTERFACE_MODE_MII :
		PHY_INTERFACE_MODE_GMII;
}

static inline void ambhw_set_hwaddr(struct ambeth_info *lp, u8 *hwaddr)
{
	u32 val;

	val = (hwaddr[5] << 8) | hwaddr[4];
	amba_writel(lp->regbase + ETH_MAC_MAC0_HI_OFFSET, val);
	udelay(4);
	val = (hwaddr[3] << 24) | (hwaddr[2] << 16) |
		(hwaddr[1] << 8) | hwaddr[0];
	amba_writel(lp->regbase + ETH_MAC_MAC0_LO_OFFSET, val);
}

static inline void ambhw_get_hwaddr(struct ambeth_info *lp, u8 *hwaddr)
{
	u32 hval;
	u32 lval;

	hval = amba_readl(lp->regbase + ETH_MAC_MAC0_HI_OFFSET);
	lval = amba_readl(lp->regbase + ETH_MAC_MAC0_LO_OFFSET);
	hwaddr[5] = ((hval >> 8) & 0xff);
	hwaddr[4] = ((hval >> 0) & 0xff);
	hwaddr[3] = ((lval >> 24) & 0xff);
	hwaddr[2] = ((lval >> 16) & 0xff);
	hwaddr[1] = ((lval >> 8) & 0xff);
	hwaddr[0] = ((lval >> 0) & 0xff);
}

static void ambeth_fc_resolve(struct ambeth_info *lp);

static inline void ambhw_set_link_mode_speed(struct ambeth_info *lp)
{
	u32 val;

	val = amba_readl(lp->regbase + ETH_MAC_CFG_OFFSET);
	switch (lp->oldspeed) {
	case SPEED_1000:
		val &= ~(ETH_MAC_CFG_PS);
		break;
	case SPEED_100:
		val |= ETH_MAC_CFG_PS;
		val |= ETH_MAC_CFG_FES;
		break;
	case SPEED_10:
		val |= ETH_MAC_CFG_PS;
		val &= ~(ETH_MAC_CFG_FES);
		break;
	default:
		break;
	}
	if (lp->oldduplex) {
		val &= ~(ETH_MAC_CFG_DO);
		val |= ETH_MAC_CFG_DM;
	} else {
		val &= ~(ETH_MAC_CFG_DM);
		val |= ETH_MAC_CFG_DO;
	}
	amba_writel(lp->regbase + ETH_MAC_CFG_OFFSET, val);
	ambeth_fc_resolve(lp);
}

static inline int ambhw_enable(struct ambeth_info *lp)
{
	int ret_val = 0;
	u32 val;

	ret_val = ambhw_dma_reset(lp);
	if (ret_val)
		goto ambhw_init_exit;

	ambhw_set_hwaddr(lp, lp->ndev->dev_addr);

	val = ETH_DMA_BUS_MODE_FB | ETH_DMA_BUS_MODE_PBL_32 |
		ETH_DMA_BUS_MODE_DA_RX;

	if(ETH_ENHANCED)
		val |= ETH_DMA_BUS_MODE_ATDS;

	amba_writel(lp->regbase + ETH_DMA_BUS_MODE_OFFSET, val);
	amba_writel(lp->regbase + ETH_MAC_FRAME_FILTER_OFFSET, 0);

	val = ETH_DMA_OPMODE_TTC_256 | ETH_DMA_OPMODE_RTC_64 |
		ETH_DMA_OPMODE_FUF | ETH_DMA_OPMODE_TSF;
	amba_writel(lp->regbase + ETH_DMA_OPMODE_OFFSET, val);
	amba_writel(lp->regbase + ETH_MAC_CFG_OFFSET,
		(ETH_MAC_CFG_TE | ETH_MAC_CFG_RE));

	/*
	 * (512 bits / N) * pause_time = actual pause time
	 * ex:
	 *     512 bits / 1 Gbps * 1954 = ~0.0010 sec = 1 ms
	 *     512 bits / 100 Mbps * 1954 = ~0.010 sec = 10 ms
	 */
	amba_writel(lp->regbase + ETH_MAC_FLOW_CTR_OFFSET,
		ETH_MAC_FLOW_CTR_PLT_256 |
		ETH_MAC_FLOW_CTR_PT(AMBETH_FC_PAUSE_TIME));

	if (lp->ipc_rx) {
		amba_setbitsl(lp->regbase + ETH_DMA_OPMODE_OFFSET,
				ETH_DMA_OPMODE_RSF);
		amba_setbitsl(lp->regbase + ETH_MAC_CFG_OFFSET,
				ETH_MAC_CFG_IPC);
	}

	if (lp->dump_rx_all) {
		amba_setbitsl(lp->regbase + ETH_MAC_FRAME_FILTER_OFFSET,
				ETH_MAC_FRAME_FILTER_RA);
	}

	if(ETH_ENHANCED)
		amba_writel(lp->regbase + ETH_MAC_INTERRUPT_MASK_OFFSET,0xFFFFFFFF);

	amba_writel(lp->regbase + ETH_DMA_STATUS_OFFSET,
		amba_readl(lp->regbase + ETH_DMA_STATUS_OFFSET));

ambhw_init_exit:
	return ret_val;
}

static inline void ambhw_disable(struct ambeth_info *lp)
{
	ambhw_stop_tx_rx(lp);
	ambhw_dma_int_disable(lp);
}

static void ambhw_dump_buffer(const char *msg,
	unsigned char *data, unsigned int length)
{
	unsigned int i;

	if (msg)
		printk("%s", msg);
	for (i = 0; i < length; i++) {
		if (i % 16 == 0) {
			printk("\n%03X:", i);
		}
		printk(" %02x", data[i]);
	}
	printk("\n");
}

/* ==========================================================================*/
static int ambhw_mdio_read(struct mii_bus *bus,
	int mii_id, int regnum)
{
	struct ambeth_info *lp = bus->priv;
	int val, cnt;

	for (cnt = AMBETH_MII_RETRY_CNT; cnt > 0; cnt--) {
		if (!amba_tstbitsl(lp->regbase + ETH_MAC_GMII_ADDR_OFFSET,
			ETH_MAC_GMII_ADDR_GB))
			break;
		udelay(10);
	}
	if ((cnt <= 0) && netif_msg_hw(lp)) {
		dev_err(&lp->ndev->dev, "MII Error: Preread tmo!\n");
		val = 0xFFFFFFFF;
		goto ambhw_mdio_read_exit;
	}

	val = ETH_MAC_GMII_ADDR_PA(mii_id) | ETH_MAC_GMII_ADDR_GR(regnum);
	val |= ETH_MAC_GMII_ADDR_CR_250_300MHZ | ETH_MAC_GMII_ADDR_GB;
	amba_writel(lp->regbase + ETH_MAC_GMII_ADDR_OFFSET, val);

	for (cnt = AMBETH_MII_RETRY_CNT; cnt > 0; cnt--) {
		if (!amba_tstbitsl(lp->regbase + ETH_MAC_GMII_ADDR_OFFSET,
			ETH_MAC_GMII_ADDR_GB))
			break;
		udelay(10);
	}
	if ((cnt <= 0) && netif_msg_hw(lp)) {
		dev_err(&lp->ndev->dev, "MII Error: Postread tmo!\n");
		val = 0xFFFFFFFF;
		goto ambhw_mdio_read_exit;
	}

	val = amba_readl(lp->regbase + ETH_MAC_GMII_DATA_OFFSET);

ambhw_mdio_read_exit:
	if (netif_msg_hw(lp))
		dev_info(&lp->ndev->dev,
			"MII Read: addr[0x%02x], reg[0x%02x], val[0x%04x].\n",
			mii_id, regnum, val);

	return val;
}

static int ambhw_mdio_write(struct mii_bus *bus,
	int mii_id, int regnum, u16 value)
{
	int ret_val = 0;
	struct ambeth_info *lp;
	int val;
	int cnt = 0;

	lp = (struct ambeth_info *)bus->priv;

	if (netif_msg_hw(lp))
		dev_info(&lp->ndev->dev,
			"MII Write: id[0x%02x], add[0x%02x], val[0x%04x].\n",
			mii_id, regnum, value);

	for (cnt = AMBETH_MII_RETRY_CNT; cnt > 0; cnt--) {
		if (!amba_tstbitsl(lp->regbase + ETH_MAC_GMII_ADDR_OFFSET,
			ETH_MAC_GMII_ADDR_GB))
			break;
		udelay(10);
	}
	if ((cnt <= 0) && netif_msg_hw(lp)) {
		dev_err(&lp->ndev->dev, "MII Error: Prewrite tmo!\n");
		ret_val = -EIO;
		goto ambhw_mdio_write_exit;
	}

	val = value;
	amba_writel(lp->regbase + ETH_MAC_GMII_DATA_OFFSET, val);
	val = ETH_MAC_GMII_ADDR_PA(mii_id) | ETH_MAC_GMII_ADDR_GR(regnum);
	val |= ETH_MAC_GMII_ADDR_CR_250_300MHZ | ETH_MAC_GMII_ADDR_GW |
		ETH_MAC_GMII_ADDR_GB;
	amba_writel(lp->regbase + ETH_MAC_GMII_ADDR_OFFSET, val);

	for (cnt = AMBETH_MII_RETRY_CNT; cnt > 0; cnt--) {
		if (!amba_tstbitsl(lp->regbase + ETH_MAC_GMII_ADDR_OFFSET,
			ETH_MAC_GMII_ADDR_GB))
			break;
		udelay(10);
	}
	if ((cnt <= 0) && netif_msg_hw(lp)) {
		dev_err(&lp->ndev->dev, "MII Error: Postwrite tmo!\n");
		ret_val = -EIO;
		goto ambhw_mdio_write_exit;
	}

ambhw_mdio_write_exit:
	return ret_val;
}

static int ambhw_mdio_reset(struct mii_bus *bus)
{
	struct ambeth_info *lp = bus->priv;
	int ret_val = 0;

	if (netif_msg_hw(lp)) {
		dev_info(&lp->ndev->dev, "MII Info: Power gpio = %d, "
			"Reset gpio = %d.\n", lp->pwr_gpio, lp->rst_gpio);
	}

	if (gpio_is_valid(lp->pwr_gpio))
		gpio_set_value_cansleep(lp->pwr_gpio, !lp->pwr_gpio_active);
	if (gpio_is_valid(lp->rst_gpio))
		gpio_set_value_cansleep(lp->rst_gpio, lp->rst_gpio_active);
	if (gpio_is_valid(lp->pwr_gpio))
		gpio_set_value_cansleep(lp->pwr_gpio, lp->pwr_gpio_active);
	if (gpio_is_valid(lp->rst_gpio))
		gpio_set_value_cansleep(lp->rst_gpio, !lp->rst_gpio_active);

	/* waiting for PHY working stable, this delay is a must */
	msleep(50);

	return ret_val;
}

/* ==========================================================================*/
static void ambeth_adjust_link(struct net_device *ndev)
{
	struct ambeth_info *lp = netdev_priv(ndev);
	struct phy_device *phydev = lp->phydev;
	int need_update = 0;
	unsigned long flags;

	spin_lock_irqsave(&lp->lock, flags);

	if (phydev->link) {
		if (phydev->duplex != lp->oldduplex) {
			need_update = 1;
			lp->oldduplex = phydev->duplex;
		}
		if (phydev->speed != lp->oldspeed) {
			switch (phydev->speed) {
			case SPEED_1000:
			case SPEED_100:
			case SPEED_10:
				need_update = 1;
				lp->oldspeed = phydev->speed;
				break;
			default:
				if (netif_msg_link(lp))
					dev_warn(&lp->ndev->dev,
						"Unknown Speed(%d).\n",
						phydev->speed);
				break;
			}
		}
		if (phydev->pause != lp->oldpause ||
		    phydev->asym_pause != lp->oldasym_pause) {
			lp->oldpause = phydev->pause;
			lp->oldasym_pause = phydev->asym_pause;
			need_update = 1;
		}
		if (lp->oldlink != phydev->link) {
			need_update = 1;
			lp->oldlink = phydev->link;
		}
	} else if (lp->oldlink) {
		need_update = 1;
		lp->oldlink = PHY_DOWN;
		lp->oldspeed = 0;
		lp->oldduplex = -1;
	}

	if (need_update) {
		ambhw_set_link_mode_speed(lp);
		if (netif_msg_link(lp))
			phy_print_status(phydev);
	}
	spin_unlock_irqrestore(&lp->lock, flags);
}

static void ambeth_fc_config(struct ambeth_info *lp)
{
	u32 sup, adv, flow_ctr;

	sup = lp->phydev->supported;
	adv = lp->phydev->advertising;
	flow_ctr = lp->flow_ctr;

	sup |= (SUPPORTED_Pause | SUPPORTED_Asym_Pause);
	adv &= ~(ADVERTISED_Pause | ADVERTISED_Asym_Pause);

	sup &= lp->phy_supported;

	if (!(sup & (SUPPORTED_Pause | SUPPORTED_Asym_Pause)))
		goto unsupported;

        if (lp->fixed_speed != SPEED_UNKNOWN)
		goto autoneg_unsupported;

	if (!(sup & SUPPORTED_Autoneg) ||
	    !(flow_ctr & AMBARELLA_ETH_FC_AUTONEG))
		goto autoneg_unsupported;

	if (flow_ctr & AMBARELLA_ETH_FC_RX) {
		/*
		 * Being able to decode pause frames is sufficently to
		 * advertise that we support both sym. and asym. pause.
		 * It doesn't matter if send pause frame or not.
                 */
		adv |= (ADVERTISED_Pause | ADVERTISED_Asym_Pause);
		goto done;
	}

	if (flow_ctr & AMBARELLA_ETH_FC_TX) {
		/* Tell the link parter that we do send the pause frame. */
		adv |= ADVERTISED_Asym_Pause;
		goto done;
	}
	goto done;

autoneg_unsupported:
	/* Sanitize the config value */
	lp->flow_ctr &= ~AMBARELLA_ETH_FC_AUTONEG;

	/* Advertise nothing about pause frame */
	adv &= ~(ADVERTISED_Pause | ADVERTISED_Asym_Pause);
	goto done;

unsupported:
	/* Sanitize the config value */
	lp->flow_ctr &= ~(AMBARELLA_ETH_FC_AUTONEG |
					 AMBARELLA_ETH_FC_RX |
					 AMBARELLA_ETH_FC_TX);
done:
	lp->phydev->advertising = adv;
	lp->phydev->supported = sup;
	dev_info(&lp->ndev->dev, "adv: sym %d, asym: %d\n",
		 !!(adv & ADVERTISED_Pause),
		 !!(adv & ADVERTISED_Asym_Pause));
}

static void ambeth_fc_resolve(struct ambeth_info *lp)
{
	u32 flow_ctr, fc, old_fc;

	flow_ctr = lp->flow_ctr;

	if (!(flow_ctr & AMBARELLA_ETH_FC_AUTONEG))
		goto force_setting;

	fc = old_fc = amba_readl(lp->regbase + ETH_MAC_FLOW_CTR_OFFSET);

	dev_info(&lp->ndev->dev, "lp: sym: %d, asym: %d\n",
		 lp->phydev->pause, lp->phydev->asym_pause);
	/*
	 * Decode pause frames only if user specified, and the link
	 * partner could send them on the same time.
	 */
	if ((flow_ctr & AMBARELLA_ETH_FC_RX) &&
	    (lp->phydev->pause || lp->phydev->asym_pause))
		fc |= ETH_MAC_FLOW_CTR_RFE;
	else
		fc &= ~ETH_MAC_FLOW_CTR_RFE;

	/*
	 * Send pause frames only if user specified, and the link
	 * partner can resopnds to them on the same time.
	 */
	if ((flow_ctr & AMBARELLA_ETH_FC_TX) && lp->phydev->pause)
		fc |= ETH_MAC_FLOW_CTR_TFE;
	else
		fc &= ~ETH_MAC_FLOW_CTR_TFE;

	if (fc != old_fc)
		amba_writel(lp->regbase + ETH_MAC_FLOW_CTR_OFFSET, fc);

	return;

force_setting:

	if (flow_ctr & AMBARELLA_ETH_FC_TX)
		amba_setbitsl(lp->regbase + ETH_MAC_FLOW_CTR_OFFSET,
			ETH_MAC_FLOW_CTR_TFE);

	if (flow_ctr & AMBARELLA_ETH_FC_RX)
		amba_setbitsl(lp->regbase + ETH_MAC_FLOW_CTR_OFFSET,
			ETH_MAC_FLOW_CTR_RFE);
}

static int ambeth_phy_start(struct ambeth_info *lp)
{
	struct net_device *ndev = lp->ndev;
	struct phy_device *phydev = lp->phydev;
	phy_interface_t intf;
	int ret_val = 0;
	unsigned long flags;

	if (lp->phy_enabled)
		return 0;

	lp->oldlink = PHY_DOWN;
	lp->oldspeed = 0;
	lp->oldduplex = -1;

	/* Fixed Link mode: we allow all valid fixed_speed,
	   even HW can not support the speed. */
	switch (lp->fixed_speed) {
	case SPEED_1000:
	case SPEED_100:
	case SPEED_10:
		lp->oldlink = PHY_RUNNING;
		lp->oldspeed = lp->fixed_speed;
		lp->oldduplex = DUPLEX_FULL;
		ambhw_set_link_mode_speed(lp);
		dev_notice(&lp->ndev->dev, "Fixed Link - %d/%s\n", lp->oldspeed,
			((lp->oldduplex == DUPLEX_FULL) ? "Full" : "Half"));
		netif_carrier_on(ndev);
		goto ambeth_init_phy_exit;
		break;
	default:
		break;
	}

	intf = ambhw_get_interface(lp);
	ret_val = phy_connect_direct(ndev, phydev, &ambeth_adjust_link, intf);
	if (ret_val) {
		dev_err(&lp->ndev->dev, "Could not attach to PHY!\n");
		goto ambeth_init_phy_exit;
	}

	phydev->supported &= lp->phy_supported;
	phydev->advertising = phydev->supported;

	spin_lock_irqsave(&lp->lock, flags);
	lp->phy_enabled = 1;
	spin_unlock_irqrestore(&lp->lock, flags);

	ambeth_fc_config(lp);
	ret_val = phy_start_aneg(phydev);

	ambeth_fc_resolve(lp);

ambeth_init_phy_exit:
	return ret_val;
}

static void ambeth_phy_stop(struct ambeth_info *lp)
{
	unsigned long flags;

	spin_lock_irqsave(&lp->lock, flags);
	lp->phy_enabled = 0;
	lp->oldlink = PHY_DOWN;
	spin_unlock_irqrestore(&lp->lock, flags);

	phy_disconnect(lp->phydev);
}

static inline int ambeth_rx_rngmng_check_skb(struct ambeth_info *lp, u32 entry)
{
	int ret_val = 0;
	dma_addr_t mapping;
	struct sk_buff *skb;

	if (lp->rx.rng_rx[entry].skb == NULL) {
		skb = netdev_alloc_skb(lp->ndev, AMBETH_PACKET_MAXFRAME);
		if (skb == NULL) {
			if (netif_msg_drv(lp))
				dev_err(&lp->ndev->dev,
				"RX Error: netdev_alloc_skb.\n");
			ret_val = -ENOMEM;
			goto ambeth_rx_rngmng_skb_exit;
		}
		mapping = dma_map_single(&lp->ndev->dev, skb->data,
			AMBETH_PACKET_MAXFRAME, DMA_FROM_DEVICE);
		lp->rx.rng_rx[entry].skb = skb;
		lp->rx.rng_rx[entry].mapping = mapping;
		lp->rx.desc_rx[entry].buffer1 = mapping;
	}

ambeth_rx_rngmng_skb_exit:
	return ret_val;
}

static inline void ambeth_rx_rngmng_init(struct ambeth_info *lp)
{
	int i;

	lp->rx.cur_rx = 0;
	lp->rx.dirty_rx = 0;
	for (i = 0; i < lp->rx_count; i++) {
		if (ambeth_rx_rngmng_check_skb(lp, i))
			break;
		lp->rx.desc_rx[i].status = ETH_RDES0_OWN;
		lp->rx.desc_rx[i].length = (ETH_RDES1_RCH |
			ETH_RDES1_RBS1x(AMBETH_PACKET_MAXFRAME));
		lp->rx.desc_rx[i].buffer2 = (u32)lp->rx_dma_desc +
			((i + 1) * sizeof(struct ambeth_desc));
	}
	lp->rx.desc_rx[lp->rx_count - 1].buffer2 = (u32)lp->rx_dma_desc;
}

static inline void ambeth_rx_rngmng_refill(struct ambeth_info *lp)
{
	u32 i;
	unsigned int dirty_diff;
	u32 entry;

	dirty_diff = (lp->rx.cur_rx - lp->rx.dirty_rx);
	for (i = 0; i < dirty_diff; i++) {
		entry = lp->rx.dirty_rx % lp->rx_count;
		if (ambeth_rx_rngmng_check_skb(lp, entry))
			break;
		lp->rx.desc_rx[entry].status = ETH_RDES0_OWN;
		lp->rx.dirty_rx++;
	}
}

static inline void ambeth_rx_rngmng_del(struct ambeth_info *lp)
{
	int i;
	dma_addr_t mapping;
	struct sk_buff *skb;

	for (i = 0; i < lp->rx_count; i++) {
		if (lp->rx.rng_rx) {
			skb = lp->rx.rng_rx[i].skb;
			mapping = lp->rx.rng_rx[i].mapping;
			lp->rx.rng_rx[i].skb = NULL;
			lp->rx.rng_rx[i].mapping = 0;
			if (mapping) {
				dma_unmap_single(&lp->ndev->dev, mapping,
					AMBETH_PACKET_MAXFRAME,
					DMA_FROM_DEVICE);
			}
			if (skb) {
				dev_kfree_skb(skb);
			}
		}
		if (lp->rx.desc_rx) {
			lp->rx.desc_rx[i].status = 0;
			lp->rx.desc_rx[i].length = 0;
			lp->rx.desc_rx[i].buffer1 = 0xBADF00D0;
			lp->rx.desc_rx[i].buffer2 = 0xBADF00D0;
		}
	}
}

static inline void ambeth_tx_rngmng_init(struct ambeth_info *lp)
{
	u32 i;

	lp->tx.cur_tx = 0;
	lp->tx.dirty_tx = 0;
	for (i = 0; i < lp->tx_count; i++) {
		lp->tx.rng_tx[i].mapping = 0;
		lp->tx.desc_tx[i].length = 0;
		amba_set_eth_desc(&lp->tx.desc_tx[i], ETH_TDES_LS | ETH_TDES_FS
			| ETH_TDES_TCH);
		lp->tx.desc_tx[i].buffer1 = 0;
		lp->tx.desc_tx[i].buffer2 = (u32)lp->tx_dma_desc +
			((i + 1) * sizeof(struct ambeth_desc));
	}
	lp->tx.desc_tx[lp->tx_count - 1].buffer2 = (u32)lp->tx_dma_desc;
}

static inline void ambeth_tx_rngmng_del(struct ambeth_info *lp)
{
	u32 i;
	dma_addr_t mapping;
	struct sk_buff *skb;

	for (i = 0; i < lp->tx_count; i++) {
		if (lp->tx.rng_tx) {
			skb = lp->tx.rng_tx[i].skb;
			mapping = lp->tx.rng_tx[i].mapping;
			lp->tx.rng_tx[i].skb = NULL;
			lp->tx.rng_tx[i].mapping = 0;
			if (skb) {
				dma_unmap_single(&lp->ndev->dev, mapping,
					skb->len, DMA_TO_DEVICE);
				dev_kfree_skb(skb);
			}
		}
		if (lp->tx.desc_tx) {
			lp->tx.desc_tx[i].status = 0;
			lp->tx.desc_tx[i].length = 0;
			lp->tx.desc_tx[i].buffer1 = 0xBADF00D0;
			lp->tx.desc_tx[i].buffer2 = 0xBADF00D0;
		}
	}
}

static inline void ambeth_check_dma_error(struct ambeth_info *lp,
	u32 irq_status)
{
	u32 miss_ov = 0;

	if (unlikely(irq_status & ETH_DMA_STATUS_AIS)) {
		if (irq_status & (ETH_DMA_STATUS_RU | ETH_DMA_STATUS_OVF))
			miss_ov = amba_readl(lp->regbase +
				ETH_DMA_MISS_FRAME_BOCNT_OFFSET);

		if (irq_status & ETH_DMA_STATUS_FBI) {
			if (netif_msg_drv(lp))
				dev_err(&lp->ndev->dev,
				"DMA Error: Fatal Bus Error 0x%x.\n",
				(irq_status & ETH_DMA_STATUS_EB_MASK));
		}
		if (irq_status & ETH_DMA_STATUS_ETI) {
			if (netif_msg_tx_err(lp))
				dev_err(&lp->ndev->dev,
				"DMA Error: Early Transmit.\n");
		}
		if (irq_status & ETH_DMA_STATUS_RWT) {
			if (netif_msg_rx_err(lp))
				dev_err(&lp->ndev->dev,
				"DMA Error: Receive Watchdog Timeout.\n");
		}
		if (irq_status & ETH_DMA_STATUS_RPS) {
			if (netif_msg_rx_err(lp))
				dev_err(&lp->ndev->dev,
				"DMA Error: Receive Process Stopped.\n");
		}
		if (irq_status & ETH_DMA_STATUS_RU) {
			if (miss_ov & ETH_DMA_MISS_FRAME_BOCNT_FRAME) {
				lp->stats.rx_dropped +=
					ETH_DMA_MISS_FRAME_BOCNT_HOST(miss_ov);
			}
			if (netif_msg_rx_err(lp))
				dev_err(&lp->ndev->dev,
				"DMA Error: Receive Buffer Unavailable, %u.\n",
				ETH_DMA_MISS_FRAME_BOCNT_HOST(miss_ov));
		}
		if (irq_status & ETH_DMA_STATUS_UNF) {
			if (netif_msg_tx_err(lp))
				dev_err(&lp->ndev->dev,
				"DMA Error: Transmit Underflow.\n");
		}
		if (irq_status & ETH_DMA_STATUS_OVF) {
			if (miss_ov & ETH_DMA_MISS_FRAME_BOCNT_FIFO) {
				lp->stats.rx_fifo_errors +=
					ETH_DMA_MISS_FRAME_BOCNT_APP(miss_ov);
			}
			if (netif_msg_rx_err(lp))
				dev_err(&lp->ndev->dev,
				"DMA Error: Receive FIFO Overflow, %u.\n",
				ETH_DMA_MISS_FRAME_BOCNT_APP(miss_ov));
		}
		if (irq_status & ETH_DMA_STATUS_TJT) {
			lp->stats.tx_errors++;
			if (netif_msg_drv(lp))
				dev_err(&lp->ndev->dev,
				"DMA Error: Transmit Jabber Timeout.\n");
		}
		if (irq_status & ETH_DMA_STATUS_TPS) {
			if (netif_msg_tx_err(lp))
				dev_err(&lp->ndev->dev,
				"DMA Error: Transmit Process Stopped.\n");
		}
		if (netif_msg_tx_err(lp) || netif_msg_rx_err(lp)) {
			dev_err(&lp->ndev->dev, "DMA Error: Abnormal: 0x%x.\n",
				irq_status);
			ambhw_dump(lp);
		}
	}
}

static inline void ambeth_pause_frame(struct ambeth_info *lp)
{
	u32					fc;

	fc = amba_readl(lp->regbase + ETH_MAC_FLOW_CTR_OFFSET);
	if (!(fc & ETH_MAC_FLOW_CTR_TFE))
		return;

	fc |= ETH_MAC_FLOW_CTR_FCBBPA;

	amba_writel(lp->regbase + ETH_MAC_FLOW_CTR_OFFSET, fc);
}

static inline void ambeth_interrupt_rx(struct ambeth_info *lp, u32 irq_status)
{
	if (irq_status & AMBETH_RXDMA_STATUS) {
		amba_clrbitsl(lp->regbase + ETH_DMA_INTEN_OFFSET,
			AMBETH_RXDMA_INTEN);
		napi_schedule(&lp->napi);
	}
}

static inline void ambeth_interrupt_gmac(struct ambeth_info *lp, u32 irq_status)
{
	if(ETH_ENHANCED) {
		u32 tmp_reg;

		if (irq_status & ETH_DMA_STATUS_GPI) {
			dev_vdbg(&lp->ndev->dev, "ETH_DMA_STATUS_GPI\n");
		}
		if (irq_status & ETH_DMA_STATUS_GMI) {
			dev_vdbg(&lp->ndev->dev, "ETH_DMA_STATUS_GMI\n");
		}
		if (irq_status & ETH_DMA_STATUS_GLI) {
			dev_vdbg(&lp->ndev->dev, "ETH_DMA_STATUS_GLI\n");
			tmp_reg = amba_readl(lp->regbase +
				ETH_MAC_INTERRUPT_STATUS_OFFSET);
			dev_vdbg(&lp->ndev->dev,
				"ETH_MAC_INTERRUPT_STATUS_OFFSET = 0x%08X\n",tmp_reg);
			tmp_reg = amba_readl(lp->regbase +
				ETH_MAC_INTERRUPT_MASK_OFFSET);
			dev_vdbg(&lp->ndev->dev,
				"ETH_MAC_INTERRUPT_MASK_OFFSET = 0x%08X\n",tmp_reg);
			tmp_reg = amba_readl(lp->regbase +
				ETH_MAC_AN_STATUS_OFFSET);
			dev_vdbg(&lp->ndev->dev,
				"ETH_MAC_AN_STATUS_OFFSET = 0x%08X\n",tmp_reg);
			tmp_reg = amba_readl(lp->regbase +
				ETH_MAC_RGMII_CS_OFFSET);
			dev_vdbg(&lp->ndev->dev,
				"ETH_MAC_RGMII_CS_OFFSET = 0x%08X\n",tmp_reg);
			tmp_reg = amba_readl(lp->regbase +
				ETH_MAC_GPIO_OFFSET);
			dev_vdbg(&lp->ndev->dev,
				"ETH_MAC_GPIO_OFFSET = 0x%08X\n",tmp_reg);
		}

	}
}

static inline u32 ambeth_check_tdes0_status(struct ambeth_info *lp,
	unsigned int status)
{
	u32 tx_retry = 0;

	if (status & ETH_TDES0_JT) {
		lp->stats.tx_heartbeat_errors++;
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev, "TX Error: Jabber Timeout.\n");
	}
	if (status & ETH_TDES0_FF) {
		lp->stats.tx_dropped++;
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev, "TX Error: Frame Flushed.\n");
	}
	if (status & ETH_TDES0_PCE) {
		lp->stats.tx_fifo_errors++;
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev,
			"TX Error: Payload Checksum Error.\n");
	}
	if (status & ETH_TDES0_LCA) {
		lp->stats.tx_carrier_errors++;
		dev_err(&lp->ndev->dev, "TX Error: Loss of Carrier.\n");
	}
	if (status & ETH_TDES0_NC) {
		lp->stats.tx_carrier_errors++;
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev, "TX Error: No Carrier.\n");
	}
	if (status & ETH_TDES0_LCO) {
		lp->stats.tx_aborted_errors++;
		lp->stats.collisions++;
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev, "TX Error: Late Collision.\n");
	}
	if (status & ETH_TDES0_EC) {
		lp->stats.tx_aborted_errors++;
		lp->stats.collisions += ETH_TDES0_CC(status);
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev,
			"TX Error: Excessive Collision %u.\n",
			ETH_TDES0_CC(status));
	}
	if (status & ETH_TDES0_VF) {
		if (netif_msg_drv(lp))
			dev_info(&lp->ndev->dev, "TX Info: VLAN Frame.\n");
	}
	if (status & ETH_TDES0_ED) {
		lp->stats.tx_fifo_errors++;
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev,
			"TX Error: Excessive Deferral.\n");
	}
	if (status & ETH_TDES0_UF) {
		tx_retry = 1;
		if (netif_msg_tx_err(lp)) {
			dev_err(&lp->ndev->dev, "TX Error: Underflow Error.\n");
			ambhw_dump(lp);
		}
	}
	if (status & ETH_TDES0_DB) {
		lp->stats.tx_fifo_errors++;
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev, "TX Error: Deferred Bit.\n");
	}

	return tx_retry;
}

static inline void ambeth_interrupt_tx(struct ambeth_info *lp, u32 irq_status)
{
	u32 i;
	unsigned int dirty_diff;
	u32 entry;
	u32 status;

	if (irq_status & AMBETH_TXDMA_STATUS) {
		dev_vdbg(&lp->ndev->dev, "cur_tx[%u], dirty_tx[%u], 0x%x.\n",
			lp->tx.cur_tx, lp->tx.dirty_tx, irq_status);
		dirty_diff = (lp->tx.cur_tx - lp->tx.dirty_tx);
		for (i = 0; i < dirty_diff; i++) {
			entry = (lp->tx.dirty_tx % lp->tx_count);
			status = lp->tx.desc_tx[entry].status;

			if (status & ETH_TDES0_OWN)
				break;

			if (unlikely(status & ETH_TDES0_ES)) {
				if ((status & ETH_TDES0_ES_MASK) ==
					ETH_TDES0_ES) {
					break;
				}
				if (ambeth_check_tdes0_status(lp, status)) {
					ambhw_dma_tx_stop(lp);
					ambhw_dma_tx_restart(lp, entry);
					ambhw_dma_tx_poll(lp);
					break;
				} else {
					lp->stats.tx_errors++;
				}
			} else {
				if (unlikely(status & ETH_TDES0_IHE)) {
					if (netif_msg_drv(lp))
						dev_err(&lp->ndev->dev,
						"TX Error: IP Header Error.\n");
				}
				lp->stats.tx_bytes +=
					lp->tx.rng_tx[entry].skb->len;
				lp->stats.tx_packets++;
			}

			dma_unmap_single(&lp->ndev->dev,
				lp->tx.rng_tx[entry].mapping,
				lp->tx.rng_tx[entry].skb->len,
				DMA_TO_DEVICE);
			dev_kfree_skb_irq(lp->tx.rng_tx[entry].skb);
			lp->tx.rng_tx[entry].skb = NULL;
			lp->tx.rng_tx[entry].mapping = 0;
			lp->tx.dirty_tx++;
		}
		dirty_diff = (lp->tx.cur_tx - lp->tx.dirty_tx);
		if (dirty_diff && (irq_status & ETH_DMA_STATUS_TU)) {
			ambhw_dma_tx_poll(lp);
		}
		if (likely(dirty_diff < lp->tx_irq_low)) {
			netif_wake_queue(lp->ndev);
		}
		dev_vdbg(&lp->ndev->dev, "cur_tx[%u], dirty_tx[%u], 0x%x.\n",
			lp->tx.cur_tx, lp->tx.dirty_tx, irq_status);
	}
}

static irqreturn_t ambeth_interrupt(int irq, void *dev_id)
{
	struct net_device *ndev;
	struct ambeth_info *lp;
	u32 irq_status;
	unsigned long flags;

	ndev = (struct net_device *)dev_id;
	lp = netdev_priv(ndev);

	spin_lock_irqsave(&lp->lock, flags);
	irq_status = amba_readl(lp->regbase + ETH_DMA_STATUS_OFFSET);
	ambeth_check_dma_error(lp, irq_status);
	ambeth_interrupt_gmac(lp, irq_status);
	ambeth_interrupt_rx(lp, irq_status);
	ambeth_interrupt_tx(lp, irq_status);
	amba_writel(lp->regbase + ETH_DMA_STATUS_OFFSET, irq_status);
	spin_unlock_irqrestore(&lp->lock, flags);

	return IRQ_HANDLED;
}

static int ambeth_start_hw(struct net_device *ndev)
{
	int ret_val = 0;
	struct ambeth_info *lp;
	unsigned long flags;

	lp = (struct ambeth_info *)netdev_priv(ndev);

	if (gpio_is_valid(lp->pwr_gpio))
		gpio_set_value_cansleep(lp->pwr_gpio, lp->pwr_gpio_active);
	if (gpio_is_valid(lp->rst_gpio)) {
		gpio_set_value_cansleep(lp->rst_gpio, lp->rst_gpio_active);
		msleep(200);
		gpio_set_value_cansleep(lp->rst_gpio, !lp->rst_gpio_active);
	}

	spin_lock_irqsave(&lp->lock, flags);
	ret_val = ambhw_enable(lp);
	spin_unlock_irqrestore(&lp->lock, flags);
	if (ret_val)
		goto ambeth_start_hw_exit;

	lp->rx.rng_rx = kmalloc((sizeof(struct ambeth_rng_info) *
		lp->rx_count), GFP_KERNEL);
	if (lp->rx.rng_rx == NULL) {
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev, "alloc rng_rx fail.\n");
		ret_val = -ENOMEM;
		goto ambeth_start_hw_exit;
	}
	lp->rx.desc_rx = dma_alloc_coherent(&lp->ndev->dev,
		(sizeof(struct ambeth_desc) * lp->rx_count),
		&lp->rx_dma_desc, GFP_KERNEL);
	if (lp->rx.desc_rx == NULL) {
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev,
			"dma_alloc_coherent desc_rx fail.\n");
		ret_val = -ENOMEM;
		goto ambeth_start_hw_exit;
	}
	memset(lp->rx.rng_rx, 0,
		(sizeof(struct ambeth_rng_info) * lp->rx_count));
	memset(lp->rx.desc_rx, 0,
		(sizeof(struct ambeth_desc) * lp->rx_count));
	ambeth_rx_rngmng_init(lp);

	lp->tx.rng_tx = kmalloc((sizeof(struct ambeth_rng_info) *
		lp->tx_count), GFP_KERNEL);
	if (lp->tx.rng_tx == NULL) {
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev, "alloc rng_tx fail.\n");
		ret_val = -ENOMEM;
		goto ambeth_start_hw_exit;
	}
	lp->tx.desc_tx = dma_alloc_coherent(&lp->ndev->dev,
		(sizeof(struct ambeth_desc) * lp->tx_count),
		&lp->tx_dma_desc, GFP_KERNEL);
	if (lp->tx.desc_tx == NULL) {
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev,
			"dma_alloc_coherent desc_tx fail.\n");
		ret_val = -ENOMEM;
		goto ambeth_start_hw_exit;
	}
	memset(lp->tx.rng_tx, 0,
		(sizeof(struct ambeth_rng_info) * lp->tx_count));
	memset(lp->tx.desc_tx, 0,
		(sizeof(struct ambeth_desc) * lp->tx_count));
	ambeth_tx_rngmng_init(lp);

	spin_lock_irqsave(&lp->lock, flags);
	ambhw_set_dma_desc(lp);
	ambhw_dma_rx_start(lp);
	ambhw_dma_tx_start(lp);
	spin_unlock_irqrestore(&lp->lock, flags);

ambeth_start_hw_exit:
	return ret_val;
}

static void ambeth_stop_hw(struct net_device *ndev)
{
	struct ambeth_info *lp;
	unsigned long flags;

	lp = (struct ambeth_info *)netdev_priv(ndev);

	spin_lock_irqsave(&lp->lock, flags);
	ambhw_disable(lp);
	spin_unlock_irqrestore(&lp->lock, flags);

	ambeth_tx_rngmng_del(lp);
	if (lp->tx.desc_tx) {
		dma_free_coherent(&lp->ndev->dev,
			(sizeof(struct ambeth_desc) * lp->tx_count),
			lp->tx.desc_tx, lp->tx_dma_desc);
		lp->tx.desc_tx = NULL;
	}
	if (lp->tx.rng_tx) {
		kfree(lp->tx.rng_tx);
		lp->tx.rng_tx = NULL;
	}

	ambeth_rx_rngmng_del(lp);
	if (lp->rx.desc_rx) {
		dma_free_coherent(&lp->ndev->dev,
			(sizeof(struct ambeth_desc) * lp->rx_count),
			lp->rx.desc_rx, lp->rx_dma_desc);
		lp->rx.desc_rx = NULL;
	}
	if (lp->rx.rng_rx) {
		kfree(lp->rx.rng_rx);
		lp->rx.rng_rx = NULL;
	}

	if (gpio_is_valid(lp->pwr_gpio))
		gpio_set_value_cansleep(lp->pwr_gpio, !lp->pwr_gpio_active);
	if (gpio_is_valid(lp->rst_gpio))
		gpio_set_value_cansleep(lp->rst_gpio, lp->rst_gpio_active);
}

static int ambeth_open(struct net_device *ndev)
{
	int ret_val = 0;
	struct ambeth_info *lp;

	lp = (struct ambeth_info *)netdev_priv(ndev);

	ret_val = ambeth_start_hw(ndev);
	if (ret_val)
		goto ambeth_open_exit;

	ret_val = request_irq(ndev->irq, ambeth_interrupt,
		IRQF_SHARED | IRQF_TRIGGER_HIGH, ndev->name, ndev);
	if (ret_val) {
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev,
			"Request_irq[%d] fail.\n", ndev->irq);
		goto ambeth_open_exit;
	}

	napi_enable(&lp->napi);
	netif_start_queue(ndev);
	ambhw_dma_int_enable(lp);

	netif_carrier_off(ndev);
	ret_val = ambeth_phy_start(lp);
	if (ret_val) {
		netif_stop_queue(ndev);
		napi_disable(&lp->napi);
		free_irq(ndev->irq, ndev);
	}

ambeth_open_exit:
	if (ret_val) {
		ambeth_stop_hw(ndev);
	}

	return ret_val;
}

static int ambeth_stop(struct net_device *ndev)
{
	struct ambeth_info *lp = netdev_priv(ndev);
	int ret_val = 0;

	netif_stop_queue(ndev);
	napi_disable(&lp->napi);
	free_irq(ndev->irq, ndev);
	ambeth_phy_stop(lp);
	netif_carrier_off(ndev);
	ambeth_stop_hw(ndev);

	return ret_val;
}

static int ambeth_hard_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	int ret_val = 0;
	struct ambeth_info *lp;
	dma_addr_t mapping;
	u32 entry;
	unsigned int dirty_diff;
	u32 tx_flag;
	unsigned long flags;

	lp = (struct ambeth_info *)netdev_priv(ndev);
	tx_flag = ETH_TDES_LS | ETH_TDES_FS | ETH_TDES_TCH;

	spin_lock_irqsave(&lp->lock, flags);
	dirty_diff = (lp->tx.cur_tx - lp->tx.dirty_tx);
	entry = (lp->tx.cur_tx % lp->tx_count);
	if (dirty_diff == lp->tx_irq_high) {
		tx_flag |= ETH_TDES_IC;
	} else if (dirty_diff == (lp->tx_count - 1)) {
		netif_stop_queue(ndev);
		tx_flag |= ETH_TDES_IC;
	} else if (dirty_diff >= lp->tx_count) {
		netif_stop_queue(ndev);
		ret_val = -ENOMEM;
		ambhw_dma_tx_poll(lp);
		spin_unlock_irqrestore(&lp->lock, flags);
		dev_err(&lp->ndev->dev, "TX Error: TX OV.\n");
		goto ambeth_hard_start_xmit_exit;
	}
	if (unlikely(lp->dump_tx))
		ambhw_dump_buffer(__func__, skb->data, skb->len);

	mapping = dma_map_single(&lp->ndev->dev,
		skb->data, skb->len, DMA_TO_DEVICE);
	if (lp->ipc_tx && (skb->ip_summed == CHECKSUM_PARTIAL)) {
		tx_flag |= ETH_TDES_CIC;
	}

	lp->tx.rng_tx[entry].skb = skb;
	lp->tx.rng_tx[entry].mapping = mapping;
	lp->tx.desc_tx[entry].buffer1 = mapping;
	lp->tx.desc_tx[entry].length = ETH_TDES1_TBS1x(skb->len);
	lp->tx.desc_tx[entry].status = 0;
	amba_set_eth_desc(&lp->tx.desc_tx[entry], tx_flag);
	lp->tx.desc_tx[entry].status |= ETH_TDES0_OWN;

	lp->tx.cur_tx++;
	ambhw_dma_tx_poll(lp);
	spin_unlock_irqrestore(&lp->lock, flags);

	ndev->trans_start = jiffies;
	dev_vdbg(&lp->ndev->dev, "TX Info: cur_tx[%u], dirty_tx[%u], "
		"entry[%u], len[%u], data_len[%u], ip_summed[%u], "
		"csum_start[%u], csum_offset[%u].\n",
		lp->tx.cur_tx, lp->tx.dirty_tx, entry, skb->len, skb->data_len,
		skb->ip_summed, skb->csum_start, skb->csum_offset);

ambeth_hard_start_xmit_exit:
	return ret_val;
}

static void ambeth_timeout(struct net_device *ndev)
{
	struct ambeth_info *lp;
	unsigned long flags;
	u32 irq_status;

	lp = (struct ambeth_info *)netdev_priv(ndev);

	dev_info(&lp->ndev->dev, "OOM Info:...\n");
	spin_lock_irqsave(&lp->lock, flags);
	irq_status = amba_readl(lp->regbase + ETH_DMA_STATUS_OFFSET);
	ambeth_interrupt_tx(lp, irq_status | AMBETH_TXDMA_STATUS);
	ambhw_dump(lp);
	spin_unlock_irqrestore(&lp->lock, flags);

	netif_wake_queue(ndev);
}

static struct net_device_stats *ambeth_get_stats(struct net_device *ndev)
{
	struct ambeth_info *lp = netdev_priv(ndev);

	return &lp->stats;
}

static void ambhw_dump_rx(struct ambeth_info *lp, u32 status, u32 entry)
{
	short pkt_len;
	struct sk_buff *skb;
	dma_addr_t mapping;

	pkt_len = ETH_RDES0_FL(status) - 4;
	if (unlikely(pkt_len > AMBETH_RX_COPYBREAK)) {
		dev_warn(&lp->ndev->dev, "Bogus packet size %u.\n", pkt_len);
		pkt_len = AMBETH_RX_COPYBREAK;
	}

	skb = lp->rx.rng_rx[entry].skb;
	mapping = lp->rx.rng_rx[entry].mapping;
	if (likely(skb && mapping)) {
		dma_unmap_single(&lp->ndev->dev, mapping,
			AMBETH_PACKET_MAXFRAME, DMA_FROM_DEVICE);
		skb_put(skb, pkt_len);
		lp->rx.rng_rx[entry].skb = NULL;
		lp->rx.rng_rx[entry].mapping = 0;
		ambhw_dump_buffer(__func__, skb->data, skb->len);
		dev_kfree_skb(skb);
	}
}

static inline void ambeth_check_rdes0_status(struct ambeth_info *lp,
	u32 status, u32 entry)
{
	if (status & ETH_RDES0_DE) {
		lp->stats.rx_frame_errors++;
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev,
			"RX Error: Descriptor Error.\n");
	}
	if (status & ETH_RDES0_SAF) {
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev,
			"RX Error: Source Address Filter Fail.\n");
	}
	if (status & ETH_RDES0_LE) {
		lp->stats.rx_length_errors++;
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev, "RX Error: Length Error.\n");
	}
	if (status & ETH_RDES0_OE) {
		lp->stats.rx_over_errors++;
		if (netif_msg_rx_err(lp))
			dev_err(&lp->ndev->dev, "RX Error: Overflow Error.\n");
	}
	if (status & ETH_RDES0_VLAN) {
		if (netif_msg_drv(lp))
			dev_info(&lp->ndev->dev, "RX Info: VLAN.\n");
	}
	if (status & ETH_RDES0_IPC) {
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev,
			"RX Error: IPC Checksum/Giant Frame.\n");
	}
	if (status & ETH_RDES0_LC) {
		lp->stats.collisions++;
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev, "RX Error: Late Collision.\n");
	}
	if (status & ETH_RDES0_FT) {
		if (netif_msg_rx_err(lp))
			dev_info(&lp->ndev->dev,
			"RX Info: Ethernet-type frame.\n");
	}
	if (status & ETH_RDES0_RWT) {
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev,
			"RX Error: Watchdog Timeout.\n");
	}
	if (status & ETH_RDES0_RE) {
		lp->stats.rx_errors++;
		if (netif_msg_rx_err(lp))
			dev_err(&lp->ndev->dev, "RX Error: Receive.\n");
	}
	if (status & ETH_RDES0_DBE) {
		if (amba_tstbitsl(lp->regbase + ETH_MAC_CFG_OFFSET,
			ETH_MAC_CFG_PS)) {
			lp->stats.rx_length_errors++;
			if (netif_msg_drv(lp))
				dev_err(&lp->ndev->dev,
				"RX Error: Dribble Bit.\n");
		}
	}
	if (status & ETH_RDES0_CE) {
		lp->stats.rx_crc_errors++;
		if (netif_msg_rx_err(lp)) {
			dev_err(&lp->ndev->dev, "RX Error: CRC.\n");
		}
	}
	if (status & ETH_RDES0_RX) {
		if (netif_msg_drv(lp)) {
			dev_err(&lp->ndev->dev,
			"RX Error: Rx MAC Address/Payload Checksum.\n");
			if (lp->dump_rx)
				ambhw_dump_rx(lp, status, entry);
		}
	}
}

static inline void ambeth_napi_rx(struct ambeth_info *lp, u32 status, u32 entry)
{
	short pkt_len;
	struct sk_buff *skb;
	dma_addr_t mapping;

	pkt_len = ETH_RDES0_FL(status) - 4;
	skb = lp->rx.rng_rx[entry].skb;
	mapping = lp->rx.rng_rx[entry].mapping;
	if (likely(skb && mapping)) {
		dma_unmap_single(&lp->ndev->dev, mapping,
			AMBETH_PACKET_MAXFRAME, DMA_FROM_DEVICE);
		skb_put(skb, pkt_len);
		skb->protocol = eth_type_trans(skb, lp->ndev);
		if(ETH_ENHANCED == 0) {
			if (lp->ipc_rx) {
				if ((status & ETH_RDES0_COE_MASK) ==
					ETH_RDES0_COE_NOCHKERROR) {
					skb->ip_summed = CHECKSUM_UNNECESSARY;
				} else {
					skb->ip_summed = CHECKSUM_NONE;
					if (netif_msg_rx_err(lp)) {
						dev_err(&lp->ndev->dev,
						"RX Error: RDES0_COE[0x%x].\n", status);
					}
				}
			}

		}

		if (unlikely(lp->dump_rx)) {
			ambhw_dump_buffer(__func__, (skb->data - 14),
				(skb->len + 14));
		}
		if (unlikely(lp->dump_rx_free))
			kfree_skb(skb);
		else
			netif_receive_skb(skb);

		lp->rx.rng_rx[entry].skb = NULL;
		lp->rx.rng_rx[entry].mapping = 0;
		lp->ndev->last_rx = jiffies;
		lp->stats.rx_packets++;
		lp->stats.rx_bytes += pkt_len;
		lp->rx.cur_rx++;
	} else {
		if (netif_msg_drv(lp)) {
			dev_err(&lp->ndev->dev,
			"RX Error: %u skb[%p], map[0x%08X].\n",
			entry, skb, mapping);
		}
	}
}

int ambeth_napi(struct napi_struct *napi, int budget)
{
	int rx_budget = budget;
	struct ambeth_info *lp;
	u32 entry;
	u32 status;
	unsigned long flags;
	unsigned int dirty_diff;
	short pkt_len;

	lp = container_of(napi, struct ambeth_info, napi);
	dev_vdbg(&lp->ndev->dev, "cur_rx[%u], dirty_rx[%u]\n",
		lp->rx.cur_rx, lp->rx.dirty_rx);

	if (unlikely(!netif_carrier_ok(lp->ndev)))
		goto ambeth_poll_complete;

	while (rx_budget > 0) {
		entry = lp->rx.cur_rx % lp->rx_count;
		status = lp->rx.desc_rx[entry].status;
		if (status & ETH_RDES0_OWN)
			break;

		pkt_len = ETH_RDES0_FL(status) - 4;
		if (unlikely(pkt_len > AMBETH_RX_COPYBREAK)) {
			dev_err(&lp->ndev->dev, "ambarella eth: jumbo frame[size:%d] received drop\n",
				pkt_len);
			ambhw_dma_rx_stop(lp);
			ambeth_check_rdes0_status(lp, status, entry);
			lp->rx.cur_rx++;
			break;
		}

		if (unlikely((status & (ETH_RDES0_FS | ETH_RDES0_LS)) !=
			(ETH_RDES0_FS | ETH_RDES0_LS))) {
			break;
		}
		if (likely((status & ETH_RDES0_ES) != ETH_RDES0_ES)) {
			ambeth_napi_rx(lp, status, entry);
		} else {
			ambhw_dma_rx_stop(lp);
			ambeth_check_rdes0_status(lp, status, entry);
			rx_budget += lp->rx_count;
			lp->rx.cur_rx++;
		}
		rx_budget--;

		dirty_diff = (lp->rx.cur_rx - lp->rx.dirty_rx);
		if (dirty_diff > (lp->rx_count / 4)) {
			ambeth_rx_rngmng_refill(lp);
		}
	}

ambeth_poll_complete:
	if (rx_budget > 0) {
		ambeth_rx_rngmng_refill(lp);
		spin_lock_irqsave(&lp->lock, flags);
		napi_complete(&lp->napi);
		amba_setbitsl(lp->regbase + ETH_DMA_INTEN_OFFSET,
			AMBETH_RXDMA_INTEN);
		ambhw_dma_rx_start(lp);
		spin_unlock_irqrestore(&lp->lock, flags);
	}

	dev_vdbg(&lp->ndev->dev, "cur_rx[%u], dirty_rx[%u], rx_budget[%u]\n",
		lp->rx.cur_rx, lp->rx.dirty_rx, rx_budget);
	return (budget - rx_budget);
}

static inline u32 ambhw_hashtable_crc(unsigned char *mac)
{
	unsigned char tmpbuf[ETH_ALEN];
	int i;
	u32 crc;

	for (i = 0; i < ETH_ALEN; i++)
		tmpbuf[i] = bitrev8(mac[i]);
	crc = crc32_be(~0, tmpbuf, ETH_ALEN);

	return (crc ^ ~0);
}

static inline void ambhw_hashtable_get(struct net_device *ndev, u32 *hat)
{
	struct netdev_hw_addr *ha;
	unsigned int bitnr;
#if 0
	unsigned char test1[] = {0x1F,0x52,0x41,0x9C,0xB6,0xAF};
	unsigned char test2[] = {0xA0,0x0A,0x98,0x00,0x00,0x45};
	dev_info(&ndev->dev,
		"Test1: 0x%08X.\n", ambhw_hashtable_crc(test1));
	dev_info(&ndev->dev,
		"Test2: 0x%08X.\n", ambhw_hashtable_crc(test2));
#endif

	hat[0] = hat[1] = 0;
	netdev_for_each_mc_addr(ha, ndev) {
		if (!(ha->addr[0] & 1))
			continue;
		bitnr = ambhw_hashtable_crc(ha->addr);
		bitnr >>= 26;
		bitnr &= 0x3F;
		hat[bitnr >> 5] |= 1 << (bitnr & 31);
	}
}

static void ambeth_set_multicast_list(struct net_device *ndev)
{
	struct ambeth_info *lp;
	unsigned int mac_filter;
	u32 hat[2];
	unsigned long flags;

	lp = (struct ambeth_info *)netdev_priv(ndev);
	spin_lock_irqsave(&lp->lock, flags);

	mac_filter = amba_readl(lp->regbase + ETH_MAC_FRAME_FILTER_OFFSET);
	hat[0] = 0;
	hat[1] = 0;

	if (ndev->flags & IFF_PROMISC) {
		mac_filter |= ETH_MAC_FRAME_FILTER_PR;
	} else if (ndev->flags & (~IFF_PROMISC)) {
		mac_filter &= ~ETH_MAC_FRAME_FILTER_PR;
	}

	if (ndev->flags & IFF_ALLMULTI) {
		hat[0] = 0xFFFFFFFF;
		hat[1] = 0xFFFFFFFF;
		mac_filter |= ETH_MAC_FRAME_FILTER_PM;
	} else if (!netdev_mc_empty(ndev)) {
		ambhw_hashtable_get(ndev, hat);
		mac_filter &= ~ETH_MAC_FRAME_FILTER_PM;
		mac_filter |= ETH_MAC_FRAME_FILTER_HMC;
	} else if (ndev->flags & (~IFF_ALLMULTI)) {
		mac_filter &= ~ETH_MAC_FRAME_FILTER_PM;
		mac_filter |= ETH_MAC_FRAME_FILTER_HMC;
	}

	if (netif_msg_hw(lp)) {
		dev_info(&lp->ndev->dev, "MC Info: flags 0x%x.\n", ndev->flags);
		dev_info(&lp->ndev->dev, "MC Info: mc_count 0x%x.\n",
			netdev_mc_count(ndev));
		dev_info(&lp->ndev->dev, "MC Info: mac_filter 0x%x.\n",
			mac_filter);
		dev_info(&lp->ndev->dev, "MC Info: hat[0x%x:0x%x].\n",
			hat[1], hat[0]);
	}

	amba_writel(lp->regbase + ETH_MAC_HASH_HI_OFFSET, hat[1]);
	amba_writel(lp->regbase + ETH_MAC_HASH_LO_OFFSET, hat[0]);
	amba_writel(lp->regbase + ETH_MAC_FRAME_FILTER_OFFSET, mac_filter);

	spin_unlock_irqrestore(&lp->lock, flags);
}

static int ambeth_set_mac_address(struct net_device *ndev, void *addr)
{
	struct ambeth_info *lp = (struct ambeth_info *)netdev_priv(ndev);
	struct sockaddr *saddr = addr;
	unsigned long flags;

	if (!is_valid_ether_addr(saddr->sa_data))
		return -EADDRNOTAVAIL;

	spin_lock_irqsave(&lp->lock, flags);

	if (netif_running(ndev)) {
		spin_unlock_irqrestore(&lp->lock, flags);
		return -EBUSY;
	}

	dev_dbg(&lp->ndev->dev, "MAC address[%pM].\n", saddr->sa_data);

	memcpy(ndev->dev_addr, saddr->sa_data, ndev->addr_len);
	ambhw_set_hwaddr(lp, ndev->dev_addr);
	ambhw_get_hwaddr(lp, ndev->dev_addr);

	spin_unlock_irqrestore(&lp->lock, flags);

	return 0;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
static void ambeth_poll_controller(struct net_device *ndev)
{
	ambeth_interrupt(ndev->irq, ndev);
}
#endif

static int ambeth_ioctl(struct net_device *ndev, struct ifreq *ifr, int ecmd)
{
	struct ambeth_info *lp = netdev_priv(ndev);

	if (!netif_running(ndev))
		return -EINVAL;

	if (!lp->phydev)
		return -ENODEV;

	return phy_mii_ioctl(lp->phydev, ifr, ecmd);
}

static const struct net_device_ops ambeth_netdev_ops = {
	.ndo_open		= ambeth_open,
	.ndo_stop		= ambeth_stop,
	.ndo_start_xmit		= ambeth_hard_start_xmit,
	.ndo_set_rx_mode	= ambeth_set_multicast_list,
	.ndo_set_mac_address 	= ambeth_set_mac_address,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_do_ioctl		= ambeth_ioctl,
	.ndo_change_mtu		= eth_change_mtu,
	.ndo_tx_timeout		= ambeth_timeout,
	.ndo_get_stats		= ambeth_get_stats,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller	= ambeth_poll_controller,
#endif
};

/* ==========================================================================*/
static int ambeth_get_settings(struct net_device *ndev,
	struct ethtool_cmd *ecmd)
{
	struct ambeth_info *lp = netdev_priv(ndev);
	int ret_val = 0;

	if (!netif_running(ndev))
		return -EINVAL;

	if (lp->phy_enabled) {
		ret_val = phy_ethtool_gset(lp->phydev, ecmd);
	} else {
		ret_val = -EINVAL;
		if (lp->oldlink == PHY_RUNNING) {
			ethtool_cmd_speed_set(ecmd, lp->oldspeed);
			ecmd->duplex = lp->oldduplex;
			ecmd->port = PORT_MII;
			ecmd->phy_address = 0xFF;
			ecmd->transceiver = XCVR_EXTERNAL;
			ecmd->autoneg = AUTONEG_DISABLE;
			ecmd->supported = SUPPORTED_MII;
			switch (lp->oldspeed) {
			case SPEED_1000:
				if (lp->oldduplex == DUPLEX_FULL) {
					ecmd->supported |=
						SUPPORTED_1000baseT_Full;
				} else {
					ecmd->supported |=
						SUPPORTED_1000baseT_Half;
				}
				ret_val = 0;
				break;
			case SPEED_100:
				if (lp->oldduplex == DUPLEX_FULL) {
					ecmd->supported |=
						SUPPORTED_100baseT_Full;
				} else {
					ecmd->supported |=
						SUPPORTED_100baseT_Half;
				}
				ret_val = 0;
				break;
			case SPEED_10:
				if (lp->oldduplex == DUPLEX_FULL) {
					ecmd->supported |=
						SUPPORTED_10baseT_Full;
				} else {
					ecmd->supported |=
						SUPPORTED_10baseT_Half;
				}
				ret_val = 0;
				break;
			default:
				break;
			}
			ecmd->advertising = ecmd->supported;
		}
	}

	return ret_val;
}

static int ambeth_set_settings(struct net_device *ndev,
	struct ethtool_cmd *ecmd)
{
	struct ambeth_info *lp = netdev_priv(ndev);

	if (!netif_running(ndev) || !lp->phy_enabled)
		return -EINVAL;

	return phy_ethtool_sset(lp->phydev, ecmd);
}

static int ambeth_get_dump_flag(struct net_device *ndev,
	struct ethtool_dump *ed)
{
	ed->len = (AMBETH_PHY_REG_SIZE * sizeof(u16));
	ed->flag = 0;
	pr_debug("%s: cmd[0x%08X], version[0x%08X], "
		"flag[0x%08X], len[0x%08X]\n",
		__func__, ed->cmd, ed->version,
		ed->flag, ed->len);

	return 0;
}

static int ambeth_get_dump_data(struct net_device *ndev,
	struct ethtool_dump *ed, void *pdata)
{
	struct ambeth_info *lp = netdev_priv(ndev);
	int i;
	u16 *regbuf;

	pr_debug("%s: cmd[0x%08X], version[0x%08X], "
		"flag[0x%08X], len[0x%08X]\n",
		__func__, ed->cmd, ed->version,
		ed->flag, ed->len);

	if (!lp->phy_enabled) {
		return -EINVAL;
	}
	regbuf = (u16 *)pdata;
	for (i = 0; i < (ed->len / 2); i++) {
		regbuf[i] = mdiobus_read(lp->phydev->bus,
			lp->phydev->addr, i);
	}

	return 0;
}

static int ambeth_set_dump(struct net_device *ndev, struct ethtool_dump *ed)
{
	struct ambeth_info *lp = netdev_priv(ndev);
	u16 dbg_address;
	u16 dbg_value;

	pr_debug("%s: cmd[0x%08X], version[0x%08X], "
		"flag[0x%08X], len[0x%08X]\n",
		__func__, ed->cmd, ed->version,
		ed->flag, ed->len);

	if (!lp->phy_enabled) {
		return -EINVAL;
	}
	dbg_address = ((ed->flag & 0xFFFF0000) >> 16);
	dbg_value = (ed->flag & 0x0000FFFF);
	mdiobus_write(lp->phydev->bus, lp->phydev->addr,
		dbg_address, dbg_value);

	return 0;
}

static u32 ambeth_get_msglevel(struct net_device *ndev)
{
	struct ambeth_info *lp;

	lp = (struct ambeth_info *)netdev_priv(ndev);

	return lp->msg_enable;
}

static void ambeth_set_msglevel(struct net_device *ndev, u32 value)
{
	struct ambeth_info *lp;

	lp = (struct ambeth_info *)netdev_priv(ndev);

	lp->msg_enable = value;
}

static void ambeth_get_pauseparam(struct net_device *ndev,
				  struct ethtool_pauseparam *pause)
{
	struct ambeth_info *lp;
	u32 flow_ctr;

	lp = (struct ambeth_info *)netdev_priv(ndev);
	flow_ctr = lp->flow_ctr;

	pause->autoneg = (flow_ctr & AMBARELLA_ETH_FC_AUTONEG) ?
				AUTONEG_ENABLE : AUTONEG_DISABLE;

	pause->rx_pause = (flow_ctr & AMBARELLA_ETH_FC_RX) ? 1 : 0;
	pause->tx_pause = (flow_ctr & AMBARELLA_ETH_FC_TX) ? 1 : 0;
}

static int ambeth_set_pauseparam(struct net_device *ndev,
				 struct ethtool_pauseparam *pause)
{
	struct ambeth_info *lp = netdev_priv(ndev);
	u32 flow_ctr;
	int ret_val = 0;

	/*
	 * Symmeteric pause can respond to recieved pause frames, and
	 * send pause frames to the link partner.
	 *
	 * Asymmetric pause can send pause frames, but can't respond to
	 * pause frames from the link partner.
	 *
	 * Autoneg only advertises and reports the 'cap (or will)' of
	 * the link partner. The final resolution still has to be done in
	 * MAC / Driver.
	 *
	 * Since our MAC can support both directions independently, we
	 * advertise our 'cap' to the link partner based on the
	 * pauseparam specified by the user (ethtool). And take the
	 * 'cap' of the link partner reported into consideration for
	 * makeing the final resolution.
	 */

	flow_ctr = lp->flow_ctr;

	if (pause->autoneg)
		flow_ctr |= AMBARELLA_ETH_FC_AUTONEG;
	else
		flow_ctr &= ~AMBARELLA_ETH_FC_AUTONEG;

	if (pause->rx_pause)
		flow_ctr |= AMBARELLA_ETH_FC_RX;
	else
		flow_ctr &= ~AMBARELLA_ETH_FC_RX;

	if (pause->tx_pause)
		flow_ctr |= AMBARELLA_ETH_FC_TX;
	else
		flow_ctr &= ~AMBARELLA_ETH_FC_TX;

	lp->flow_ctr = flow_ctr;
	if(lp->flow_ctr & (AMBARELLA_ETH_FC_TX | AMBARELLA_ETH_FC_RX))
		lp->phy_supported |= SUPPORTED_Pause;
	else
		lp->phy_supported &= ~SUPPORTED_Pause;


	ambeth_fc_config(lp);

	if (pause->autoneg && lp->phydev->autoneg) {

		ret_val = phy_start_aneg(lp->phydev);
		if (ret_val)
			goto done;
	}
	else {
		ambeth_fc_resolve(lp);
	}
done:
	return ret_val;
}

static int ambeth_get_eee(struct net_device *ndev, struct ethtool_eee *e)
{
	struct ambeth_info *lp = netdev_priv(ndev);

	/*now our chip don't support EEE*/
	e->eee_enabled = false;
	e->eee_active = false;
	e->tx_lpi_enabled = false;
	e->tx_lpi_timer = 0;

	return phy_ethtool_get_eee(lp->phydev, e);
}

static int ambeth_set_eee(struct net_device *ndev, struct ethtool_eee *e)
{
	struct ambeth_info *lp = netdev_priv(ndev);
	int ret = 0;

	if(e->eee_enabled || e->tx_lpi_enabled) {
		dev_err(&lp->ndev->dev, "eth don't support EEE.\n");
		return -EOPNOTSUPP;
	}

	ret = phy_init_eee(lp->phydev, 0);
	if (ret) {
		dev_err(&lp->ndev->dev, "phy don't support EEE.\n");
		return ret;
	}

	return phy_ethtool_set_eee(lp->phydev, e);
}

static const struct ethtool_ops ambeth_ethtool_ops = {
	.get_settings		= ambeth_get_settings,
	.set_settings		= ambeth_set_settings,
	.get_link		= ethtool_op_get_link,
	.get_dump_flag		= ambeth_get_dump_flag,
	.get_dump_data		= ambeth_get_dump_data,
	.set_dump		= ambeth_set_dump,
	.get_msglevel		= ambeth_get_msglevel,
	.set_msglevel		= ambeth_set_msglevel,
	.get_pauseparam		= ambeth_get_pauseparam,
	.set_pauseparam		= ambeth_set_pauseparam,
	.get_eee		= ambeth_get_eee,
	.set_eee		= ambeth_set_eee,
};

/* ==========================================================================*/
static int ambeth_of_parse(struct device_node *np, struct ambeth_info *lp)
{
	struct device_node *phy_np;
	enum of_gpio_flags flags;
	int gmii, ret_val, clk_src, clk_direction, clk_pl;

	ret_val = of_property_read_u32(np, "amb,fixed-speed", &lp->fixed_speed);
	if (ret_val < 0)
		lp->fixed_speed = SPEED_UNKNOWN;

	gmii = !!of_find_property(np, "amb,support-gmii", NULL);
	if (gmii) {
		lp->phy_supported = (	SUPPORTED_10baseT_Half | \
					SUPPORTED_10baseT_Full | \
					SUPPORTED_100baseT_Half | \
					SUPPORTED_100baseT_Full | \
					SUPPORTED_1000baseT_Half | \
					SUPPORTED_1000baseT_Full | \
					SUPPORTED_Autoneg | \
					SUPPORTED_MII);
	} else {
		lp->phy_supported = (	SUPPORTED_10baseT_Half | \
					SUPPORTED_10baseT_Full | \
					SUPPORTED_100baseT_Half | \
					SUPPORTED_100baseT_Full | \
					SUPPORTED_Autoneg | \
					SUPPORTED_MII);
	}

	/*enable flow control*/
	lp->phy_supported |= SUPPORTED_Pause | SUPPORTED_Asym_Pause;

	ret_val = of_property_read_u32(np, "amb,tx-ring-size", &lp->tx_count);
	if (ret_val < 0 || lp->tx_count < AMBETH_TX_RNG_MIN)
		lp->tx_count = AMBETH_TX_RNG_MIN;

	ret_val = of_property_read_u32(np, "amb,rx-ring-size", &lp->rx_count);
	if (ret_val < 0 || lp->rx_count < AMBETH_RX_RNG_MIN)
		lp->rx_count = AMBETH_RX_RNG_MIN;

	lp->tx_irq_low = ((lp->tx_count * 1) / 4);
	lp->tx_irq_high = ((lp->tx_count * 3) / 4);

	lp->ipc_tx = !!of_find_property(np, "amb,ipc-tx", NULL);
	lp->ipc_rx = !!of_find_property(np, "amb,ipc-rx", NULL);
	lp->dump_tx = !!of_find_property(np, "amb,dump-tx", NULL);
	lp->dump_rx = !!of_find_property(np, "amb,dump-rx", NULL);
	lp->dump_rx_free = !!of_find_property(np, "amb,dump-rx-free", NULL);
	lp->dump_rx_all = !!of_find_property(np, "amb,dump-rx-all", NULL);
	lp->mdio_gpio = !!of_find_property(np, "amb,mdio-gpio", NULL);

	for_each_child_of_node(np, phy_np) {
		if (!phy_np->name || of_node_cmp(phy_np->name, "phy"))
			continue;

		lp->pwr_gpio = of_get_named_gpio_flags(phy_np, "pwr-gpios", 0, &flags);
		lp->pwr_gpio_active = !!(flags & OF_GPIO_ACTIVE_LOW);

		lp->rst_gpio = of_get_named_gpio_flags(phy_np, "rst-gpios", 0, &flags);
		lp->rst_gpio_active = !!(flags & OF_GPIO_ACTIVE_LOW);

		ret_val = of_property_read_u32(phy_np, "amb,clk-src", &clk_src);
		if (ret_val == 0 && clk_src == 0) {
			/*clk_src == 0 represent the clk is external*/
			amba_writel(ENET_CLK_SRC_SEL_REG, 0x00);
		} else if(ret_val == 0 && clk_src == 1) {
			/*clk_src == 1 and default represent the clk is internal*/
			amba_writel(ENET_CLK_SRC_SEL_REG, 0x01);
		} else {
			/*default value for clk source*/
		}

		ret_val = of_property_read_u32(phy_np, "amb,clk-invert", &clk_pl);
		if(ret_val == 0 && clk_pl == 1) {
			amba_setbitsl(AHB_SCRATCHPAD_REG(0xc), 0x80000000);
		} else if(ret_val == 0 && clk_pl == 0) {
			amba_clrbitsl(AHB_SCRATCHPAD_REG(0xc), 0x80000000);
		}

		ret_val = of_property_read_u32(phy_np, "amb,clk-dir", &clk_direction);
		if(ret_val == 0 && clk_direction == 1) {
			/*set direction of xx_enet_clk_rx as output from ambarella chip*/
			lp->clk_direction = 1;
			ret_val = amba_readl(AHB_MISC_EN_REG);
			ret_val |= (1 << 5);
			amba_writel(AHB_MISC_EN_REG, ret_val);
		} else if(ret_val == 0 && clk_direction == 0) {
			/*set direction of xx_enet_clk_rx as output from external phy*/
			lp->clk_direction = 0;
			ret_val = amba_readl(AHB_MISC_EN_REG);
			ret_val &= ~(1 << 5);
			amba_writel(AHB_MISC_EN_REG, ret_val);
		} else
			lp->clk_direction = -1;
	}

	return 0;
}

static int ambeth_drv_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node, *mdio_np = NULL;
	struct net_device *ndev;
	struct ambeth_info *lp;
	struct resource *res;
	const char *macaddr;
	int ret_val = 0;

	if (!(ambarella_get_poc() & SYS_CONFIG_ETH_ENABLE)) {
		dev_err(&pdev->dev, "Not enabled, check HW config!\n");
		return -EPERM;
	}

	ndev = alloc_etherdev(sizeof(struct ambeth_info));
	if (ndev == NULL) {
		dev_err(&pdev->dev, "alloc_etherdev fail.\n");
		return -ENOMEM;
	}
	lp = netdev_priv(ndev);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "No mem resource for fio_reg!\n");
		ret_val = -ENXIO;
		goto ambeth_drv_probe_free_netdev;
	}

	lp->regbase = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!lp->regbase) {
		dev_err(&pdev->dev, "devm_ioremap() failed\n");
		ret_val = -ENOMEM;
		goto ambeth_drv_probe_free_netdev;
	}

	ndev->irq = platform_get_irq(pdev, 0);
	if (ndev->irq < 0) {
		dev_err(&pdev->dev, "no irq for ethernet!\n");
		ret_val = -ENODEV;
		goto ambeth_drv_probe_free_netdev;
	}

	SET_NETDEV_DEV(ndev, &pdev->dev);
	ndev->dev.dma_mask = pdev->dev.dma_mask;
	ndev->dev.coherent_dma_mask = pdev->dev.coherent_dma_mask;

	spin_lock_init(&lp->lock);
	lp->ndev = ndev;
	lp->msg_enable = netif_msg_init(msg_level, NETIF_MSG_DRV);

	ambeth_of_parse(np, lp);

	if (lp->ipc_tx)
		ndev->features |= NETIF_F_HW_CSUM;

	/* request gpio for PHY power control */
	if (gpio_is_valid(lp->pwr_gpio)) {
		ret_val = devm_gpio_request(&pdev->dev,
			lp->pwr_gpio, "phy power");
		if (ret_val < 0) {
			dev_err(&pdev->dev, "Failed to request pwr-gpios!\n");
			goto ambeth_drv_probe_free_netdev;
		}
		gpio_direction_output(lp->pwr_gpio, lp->pwr_gpio_active);
	}

	/* request gpio for PHY reset control */
	if (gpio_is_valid(lp->rst_gpio)) {
		ret_val = devm_gpio_request(&pdev->dev,
			lp->rst_gpio, "phy reset");
		if (ret_val < 0) {
			dev_err(&pdev->dev, "Failed to request rst-gpios!\n");
			goto ambeth_drv_probe_free_netdev;
		}
		gpio_direction_output(lp->rst_gpio, !lp->rst_gpio_active);
	}

	if(lp->mdio_gpio) {
		mdio_np = of_find_compatible_node(NULL, NULL, "virtual,mdio-gpio");

		if(mdio_np == NULL) {
			dev_err(&pdev->dev, "Failed to get mdio_gpio device node\n");
			goto ambeth_drv_probe_free_netdev;
		}

		lp->phydev = of_phy_find_device(mdio_np->child);

		if(!lp->phydev) {
			dev_err(&pdev->dev, "Failed to get phydev from mdio_gpio device node\n");
			goto ambeth_drv_probe_free_netdev;
		}

		lp->new_bus = *lp->phydev->bus;
	} else {
		lp->new_bus.name = "Ambarella MII Bus",
		lp->new_bus.read = &ambhw_mdio_read,
		lp->new_bus.write = &ambhw_mdio_write,
		lp->new_bus.reset = &ambhw_mdio_reset,
		snprintf(lp->new_bus.id, MII_BUS_ID_SIZE, "%s", pdev->name);
		lp->new_bus.priv = lp;
		lp->new_bus.irq = kmalloc(sizeof(int)*PHY_MAX_ADDR, GFP_KERNEL);
		if (lp->new_bus.irq == NULL) {
			dev_err(&pdev->dev, "alloc new_bus.irq fail.\n");
			ret_val = -ENOMEM;
			goto ambeth_drv_probe_free_netdev;
		}

		lp->new_bus.parent = &pdev->dev;
		lp->new_bus.state = MDIOBUS_ALLOCATED;

		ret_val = of_mdiobus_register(&lp->new_bus, pdev->dev.of_node);
		if (ret_val < 0) {
			dev_err(&pdev->dev, "mdiobus_register fail%d.\n", ret_val);
			goto ambeth_drv_probe_kfree_mdiobus;
		}

		lp->phydev = phy_find_first(&lp->new_bus);
		if (lp->phydev == NULL) {
			dev_err(&pdev->dev, "No PHY device.\n");
			ret_val = -ENODEV;
			goto ambeth_drv_probe_kfree_mdiobus;
		}
	}

	if (netif_msg_drv(lp)) {
		dev_info(&pdev->dev, "Ethernet PHY[%d]: 0x%08x!\n",
				lp->phydev->addr, lp->phydev->phy_id);
	}

	ether_setup(ndev);
	ndev->netdev_ops = &ambeth_netdev_ops;
	ndev->watchdog_timeo = AMBETH_TX_WATCHDOG;
	netif_napi_add(ndev, &lp->napi, ambeth_napi, AMBETH_NAPI_WEIGHT);

	macaddr = of_get_mac_address(pdev->dev.of_node);
	if (macaddr)
		memcpy(ndev->dev_addr, macaddr, ETH_ALEN);

	if (!is_valid_ether_addr(ndev->dev_addr))
		eth_hw_addr_random(ndev);

	ambhw_disable(lp);

	if (gpio_is_valid(lp->pwr_gpio))
		gpio_set_value_cansleep(lp->pwr_gpio, !lp->pwr_gpio_active);
	if (gpio_is_valid(lp->rst_gpio))
		gpio_set_value_cansleep(lp->rst_gpio, lp->rst_gpio_active);

	SET_ETHTOOL_OPS(ndev, &ambeth_ethtool_ops);
	ret_val = register_netdev(ndev);
	if (ret_val) {
		dev_err(&pdev->dev, " register_netdev fail%d.\n", ret_val);
		goto ambeth_drv_probe_netif_napi_del;
	}

	platform_set_drvdata(pdev, ndev);
	dev_notice(&pdev->dev, "MAC Address[%pM].\n", ndev->dev_addr);

	return 0;

ambeth_drv_probe_netif_napi_del:
	netif_napi_del(&lp->napi);
	mdiobus_unregister(&lp->new_bus);

ambeth_drv_probe_kfree_mdiobus:
	kfree(lp->new_bus.irq);

ambeth_drv_probe_free_netdev:
	free_netdev(ndev);
	return ret_val;
}

static int ambeth_drv_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct ambeth_info *lp = netdev_priv(ndev);

	unregister_netdev(ndev);
	netif_napi_del(&lp->napi);
	mdiobus_unregister(&lp->new_bus);
	kfree(lp->new_bus.irq);
	platform_set_drvdata(pdev, NULL);
	free_netdev(ndev);
	dev_notice(&pdev->dev, "Removed.\n");

	return 0;
}

#ifdef CONFIG_PM
static int ambeth_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct ambeth_info *lp = netdev_priv(ndev);
	int ret_val = 0;
	unsigned long flags;

	if (!netif_running(ndev))
		goto ambeth_drv_suspend_exit;

	napi_disable(&lp->napi);
	netif_device_detach(ndev);
	disable_irq(ndev->irq);

	ambeth_phy_stop(lp);

	spin_lock_irqsave(&lp->lock, flags);
	ambhw_disable(lp);
	spin_unlock_irqrestore(&lp->lock, flags);

	if (gpio_is_valid(lp->pwr_gpio))
		gpio_set_value_cansleep(lp->pwr_gpio, !lp->pwr_gpio_active);
	if (gpio_is_valid(lp->rst_gpio))
		gpio_set_value_cansleep(lp->rst_gpio, lp->rst_gpio_active);

ambeth_drv_suspend_exit:
	dev_dbg(&pdev->dev, "%s exit with %d @ %d\n",
		__func__, ret_val, state.event);

	return ret_val;
}

static int ambeth_drv_resume(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct ambeth_info *lp = netdev_priv(ndev);
	int ret_val = 0;
	unsigned long flags;

	if (!netif_running(ndev))
		goto ambeth_drv_resume_exit;

	if(lp->clk_direction == 1) {
		ret_val = amba_readl(AHB_MISC_EN_REG);
		ret_val |= (1 << 5);
		amba_writel(AHB_MISC_EN_REG, ret_val);
	} else if(lp->clk_direction == 0) {
		ret_val = amba_readl(AHB_MISC_EN_REG);
		ret_val &= ~(1 << 5);
		amba_writel(AHB_MISC_EN_REG, ret_val);
	}

	if (gpio_is_valid(lp->pwr_gpio))
		gpio_set_value_cansleep(lp->pwr_gpio, lp->pwr_gpio_active);
	if (gpio_is_valid(lp->rst_gpio)) {
		gpio_set_value_cansleep(lp->rst_gpio, lp->rst_gpio_active);
		msleep(10);
		gpio_set_value_cansleep(lp->rst_gpio, !lp->rst_gpio_active);
	}

	spin_lock_irqsave(&lp->lock, flags);
	ret_val = ambhw_enable(lp);
	ambhw_set_link_mode_speed(lp);
	ambeth_rx_rngmng_init(lp);
	ambeth_tx_rngmng_init(lp);
	ambhw_set_dma_desc(lp);
	ambhw_dma_rx_start(lp);
	ambhw_dma_tx_start(lp);
	ambhw_dma_int_enable(lp);
	spin_unlock_irqrestore(&lp->lock, flags);

	if (ret_val) {
		dev_err(&pdev->dev, "ambhw_enable.\n");
	} else {
		ambeth_set_multicast_list(ndev);
		netif_carrier_off(ndev);
		ret_val = ambeth_phy_start(lp);
		enable_irq(ndev->irq);
		netif_device_attach(ndev);
		napi_enable(&lp->napi);
	}
ambeth_drv_resume_exit:
	dev_dbg(&pdev->dev, "%s exit with %d\n", __func__, ret_val);
	return ret_val;
}
#endif

static const struct of_device_id ambarella_eth_dt_ids[] = {
	{ .compatible = "ambarella,eth" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ambarella_eth_dt_ids);

static struct platform_driver ambeth_driver = {
	.probe		= ambeth_drv_probe,
	.remove		= ambeth_drv_remove,
#ifdef CONFIG_PM
	.suspend        = ambeth_drv_suspend,
	.resume		= ambeth_drv_resume,
#endif
	.driver = {
		.name	= "ambarella-eth",
		.owner	= THIS_MODULE,
		.of_match_table	= ambarella_eth_dt_ids,
	},
};

module_platform_driver(ambeth_driver);

MODULE_DESCRIPTION("Ambarella Media Processor Ethernet Driver");
MODULE_AUTHOR("Anthony Ginger, <hfjiang@ambarella.com>");
MODULE_LICENSE("GPL");

