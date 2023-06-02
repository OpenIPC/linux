/*
 * drivers/net/arm/gk_eth.c
 *
 * Author: Steven Yu, <yulindeng@gokemicro.com>
 * Copyright (C) 2012-2015, goke, Inc.
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
#include <mach/io.h>
#include <mach/eth.h>
#include <mach/gpio.h>

/* ==========================================================================*/
#define GKETH_PACKET_MAXFRAME   (1536)
#define GKETH_RX_COPYBREAK      (1518)
#define GKETH_RX_RNG_MIN        (8)
#define GKETH_TX_RNG_MIN        (4)

#define GKETH_RXDMA_STATUS  (ETH_DMA_STATUS_OVF | ETH_DMA_STATUS_RI | \
                ETH_DMA_STATUS_RU | ETH_DMA_STATUS_RPS | \
                ETH_DMA_STATUS_RWT)
#define GKETH_RXDMA_INTEN   (ETH_DMA_INTEN_OVE | ETH_DMA_INTEN_RIE | \
                ETH_DMA_INTEN_RUE | ETH_DMA_INTEN_RSE | \
                ETH_DMA_INTEN_RWE)
#define GKETH_TXDMA_STATUS  (ETH_DMA_STATUS_TI | ETH_DMA_STATUS_TPS | \
                ETH_DMA_STATUS_TU | ETH_DMA_STATUS_TJT | \
                ETH_DMA_STATUS_UNF)
#define GKETH_TXDMA_INTEN   (ETH_DMA_INTEN_TIE | ETH_DMA_INTEN_TSE | \
                ETH_DMA_INTEN_TUE | ETH_DMA_INTEN_TJE | \
                ETH_DMA_INTEN_UNE)
#define GKETH_DMA_INTEN (ETH_DMA_INTEN_NIE | ETH_DMA_INTEN_AIE | \
                ETH_DMA_INTEN_FBE | GKETH_RXDMA_INTEN | \
                GKETH_TXDMA_INTEN)

#define GKETH_TDES0_ATOMIC_CHECK
#undef GKETH_TDES0_ATOMIC_CHECK_ALL
#define GKETH_RDES0_ATOMIC_CHECK
#undef GKETH_RDES0_ATOMIC_CHECK_ALL

/* ==========================================================================*/
struct GKETH_desc {
    u32                     status;
    u32                     length;
    u32                     buffer1;
    u32                     buffer2;
} __attribute((packed));

struct GKETH_rng_info {
    struct sk_buff          *skb;
    dma_addr_t              mapping;
    unsigned long           buf_size;
};

struct GKETH_tx_rngmng {
    unsigned int            cur_tx;
    unsigned int            dirty_tx;
    struct GKETH_rng_info   *rng_tx;
    struct GKETH_desc       *desc_tx;
};

struct GKETH_rx_rngmng {
    unsigned int            cur_rx;
    unsigned int            dirty_rx;
    struct GKETH_rng_info   *rng_rx;
    struct GKETH_desc       *desc_rx;
};

struct GKETH_info {
    unsigned int            rx_count;
    struct GKETH_rx_rngmng  rx;
    unsigned int            tx_count;
    unsigned int            tx_irq_count;
    struct GKETH_tx_rngmng  tx;
    dma_addr_t              rx_dma_desc;
    dma_addr_t              tx_dma_desc;
    spinlock_t              lock;
    int                     oldspeed;
    int                     oldduplex;
    int                     oldlink;

    struct net_device_stats stats;
    struct napi_struct      napi;
    struct net_device       *ndev;
    struct mii_bus          new_bus;
    struct phy_device       *phydev;
    uint32_t                msg_enable;

    unsigned char __iomem   *regbase;
    struct gk_eth_platform_info *platform_info;
};

/* ==========================================================================*/
extern uint8_t cmdline_phytype;
static int msg_level = -1;
module_param (msg_level, int, 0);
MODULE_PARM_DESC (msg_level, "Override default message level");

typedef union { /* EPHY_SPEED */
    u16 all;
    struct {
        u16 ltp_f                       : 8;
        u16 isolate                     : 1;
        u16 rptr                        : 1;
        u16 duplex                      : 1;
        u16 speed                       : 1;
        u16 ane                         : 1;
        u16 ldps                        : 1;
        u16 disable_eee_force           : 1;
        u16                             : 1;
    } bitc;
} GH_EPHY_SPEED_S;

static void GK_EPHY_POWER_OFF(void)
{
    u16 dat;

    dat = gk_eth_readl(REG_EPHY_POWER);
    //printk("power reg : 0x%04x\n", dat);

    gk_eth_writel(REG_EPHY_POWER, 0x03ff);
}

static void GK_EPHY_POWER_ON(void)
{
    u16 dat;

    dat = gk_eth_readl(REG_EPHY_POWER);
    //printk("power reg : 0x%04x\n", dat);

    gk_eth_writel(REG_EPHY_POWER, 0);
}

static void GH_EPHY_set_SPEED_ane(u8 data)
{
    GH_EPHY_SPEED_S d;
	d.all = gk_eth_readl(REG_EPHY_SPEED);
    //d.all = *(volatile u16 *)REG_EPHY_SPEED;
    d.bitc.ane = data;
	gk_eth_writel(REG_EPHY_SPEED, d.all);
    //*(volatile u16 *)REG_EPHY_SPEED = d.all;
}

static void MHal_EMAC_WritReg8( u32 bank, u32 reg, u8 val )
{
    u32 address = REG_EPHY_CONTROL + (bank-0x32)*0x100*2;
    address = address + (reg << 1) - (reg & 1);
	gk_eth_writeb(address, val);
    //*( ( volatile u8* ) address ) = val;
}

static u8 MHal_EMAC_ReadReg8( u32 bank, u32 reg )
{
    u8 val;
    u32 address = REG_EPHY_CONTROL + (bank-0x32)*0x100*2;
    address = address + (reg << 1) - (reg & 1);
	val = gk_eth_readb(address);
    //val = *( ( volatile u8* ) address );
    return val;
}

/* ==========================================================================*/
static inline int gkhw_dma_reset(struct GKETH_info *lp)
{
    int                 errorCode = 0;
    u32                 counter = 0;

    gk_eth_setbitsl((unsigned int)(lp->regbase + ETH_DMA_BUS_MODE_OFFSET),
        ETH_DMA_BUS_MODE_SWR);
    do {
        if (counter++ > 1000) {
            errorCode = -EIO;
            break;
        }
        mdelay(1);
    } while (gk_eth_tstbitsl((unsigned int)(lp->regbase + ETH_DMA_BUS_MODE_OFFSET),
        ETH_DMA_BUS_MODE_SWR));

    if (errorCode && netif_msg_drv(lp))
        dev_err(&lp->ndev->dev, "DMA Error: Check PHY.\n");
    //else printk("dma reset ok\n");

    return errorCode;
}

static inline void gkhw_dma_int_enable(struct GKETH_info *lp)
{
    gk_eth_writel((unsigned int)(lp->regbase + ETH_DMA_INTEN_OFFSET),
                  GKETH_DMA_INTEN);
}

static inline void gkhw_dma_int_disable(struct GKETH_info *lp)
{
    gk_eth_writel((unsigned int)(lp->regbase + ETH_DMA_INTEN_OFFSET), 0);
}

static inline void gkhw_dma_rx_start(struct GKETH_info *lp)
{
    gk_eth_setbitsl((unsigned int)(lp->regbase + ETH_DMA_OPMODE_OFFSET),
                    ETH_DMA_OPMODE_SR);
}

static inline void gkhw_dma_rx_stop(struct GKETH_info *lp)
{
    unsigned int                irq_status;
    int                 i = 1300;

    gk_eth_clrbitsl((unsigned int)(lp->regbase + ETH_DMA_OPMODE_OFFSET),
                    ETH_DMA_OPMODE_SR);
    do {
        udelay(1);
        irq_status =
            gk_eth_readl((unsigned int)(lp->regbase + ETH_DMA_STATUS_OFFSET));
    } while ((irq_status & ETH_DMA_STATUS_RS_MASK) && --i);
    if ((i <= 0) && netif_msg_drv(lp)) {
        dev_err(&lp->ndev->dev,
            "DMA Error: Stop RX status=0x%x, opmode=0x%x.\n",
            gk_eth_readl((unsigned int)(lp->regbase + ETH_DMA_STATUS_OFFSET)),
            gk_eth_readl((unsigned int)(lp->regbase + ETH_DMA_OPMODE_OFFSET)));
    }
}

static inline void gkhw_dma_tx_start(struct GKETH_info *lp)
{
    gk_eth_setbitsl((unsigned int)(lp->regbase + ETH_DMA_OPMODE_OFFSET),
                    ETH_DMA_OPMODE_ST);
}

static inline void gkhw_dma_tx_stop(struct GKETH_info *lp)
{
    unsigned int                irq_status;
    int                 i = 1300;

    gk_eth_clrbitsl((unsigned int)(lp->regbase + ETH_DMA_OPMODE_OFFSET),
                    ETH_DMA_OPMODE_ST);
    do {
        udelay(1);
        irq_status =
            gk_eth_readl((unsigned int)(lp->regbase + ETH_DMA_STATUS_OFFSET));
    } while ((irq_status & ETH_DMA_STATUS_TS_MASK) && --i);
    if ((i <= 0) && netif_msg_drv(lp)) {
        dev_err(&lp->ndev->dev,
            "DMA Error: Stop TX status=0x%x, opmode=0x%x.\n",
            gk_eth_readl((unsigned int)(lp->regbase + ETH_DMA_STATUS_OFFSET)),
            gk_eth_readl((unsigned int)(lp->regbase + ETH_DMA_OPMODE_OFFSET)));
    }
    gk_eth_setbitsl((unsigned int)(lp->regbase + ETH_DMA_OPMODE_OFFSET),
        ETH_DMA_OPMODE_FTF);
}

static inline void gkhw_dma_tx_restart(struct GKETH_info *lp, u32 entry)
{
    gk_eth_writel((unsigned int)(lp->regbase + ETH_DMA_TX_DESC_LIST_OFFSET),
        (u32)lp->tx_dma_desc + (entry * sizeof(struct GKETH_desc)));
    gkhw_dma_tx_start(lp);
}

static inline void gkhw_dma_tx_poll(struct GKETH_info *lp, u32 entry)
{
    lp->tx.desc_tx[entry].status = ETH_TDES0_OWN;
    gk_eth_writel((unsigned int)(lp->regbase + ETH_DMA_TX_POLL_DMD_OFFSET), 0x01);
}

static inline void gkhw_stop_tx_rx(struct GKETH_info *lp)
{
    unsigned int                irq_status;
    int                 i = 1300;

    gk_eth_clrbitsl((unsigned int)(lp->regbase + ETH_MAC_CFG_OFFSET),
                    ETH_MAC_CFG_RE);
    gk_eth_clrbitsl((unsigned int)(lp->regbase + ETH_DMA_OPMODE_OFFSET),
                    (ETH_DMA_OPMODE_SR | ETH_DMA_OPMODE_ST));
    do {
        udelay(1);
        irq_status =
            gk_eth_readl((unsigned int)(lp->regbase + ETH_DMA_STATUS_OFFSET));
    } while ((irq_status & (ETH_DMA_STATUS_TS_MASK |
        ETH_DMA_STATUS_RS_MASK)) && --i);
    if ((i <= 0) && netif_msg_drv(lp)) {
        dev_err(&lp->ndev->dev,
            "DMA Error: Stop TX/RX status=0x%x, opmode=0x%x.\n",
            gk_eth_readl((unsigned int)(lp->regbase + ETH_DMA_STATUS_OFFSET)),
            gk_eth_readl((unsigned int)(lp->regbase + ETH_DMA_OPMODE_OFFSET)));
    }
    gk_eth_clrbitsl((unsigned int)(lp->regbase + ETH_MAC_CFG_OFFSET),
        ETH_MAC_CFG_TE);
}

static inline void gkhw_set_dma_desc(struct GKETH_info *lp)
{
    gk_eth_writel((unsigned int)(lp->regbase + ETH_DMA_RX_DESC_LIST_OFFSET),
        lp->rx_dma_desc);
    gk_eth_writel((unsigned int)(lp->regbase + ETH_DMA_TX_DESC_LIST_OFFSET),
        lp->tx_dma_desc);
}

static inline phy_interface_t gkhw_get_interface(struct GKETH_info *lp)
{
    return gk_eth_tstbitsl((unsigned int)(lp->regbase + ETH_MAC_CFG_OFFSET),
        ETH_MAC_CFG_PS) ? PHY_INTERFACE_MODE_RMII :
        PHY_INTERFACE_MODE_RMII;

}

static inline void gkhw_set_hwaddr(struct GKETH_info *lp, u8 *hwaddr)
{
    u32                 val;

    val = (hwaddr[5] << 8) | hwaddr[4];
    gk_eth_writel((unsigned int)(lp->regbase + ETH_MAC_MAC0_HI_OFFSET), val);
    udelay(4);
    val = (hwaddr[3] << 24) | (hwaddr[2] << 16) |
        (hwaddr[1] << 8) | hwaddr[0];
    gk_eth_writel((unsigned int)(lp->regbase + ETH_MAC_MAC0_LO_OFFSET), val);
}

static inline void gkhw_get_hwaddr(struct GKETH_info *lp, u8 *hwaddr)
{
    u32                 hval;
    u32                 lval;

    hval = gk_eth_readl((unsigned int)(lp->regbase + ETH_MAC_MAC0_HI_OFFSET));
    lval = gk_eth_readl((unsigned int)(lp->regbase + ETH_MAC_MAC0_LO_OFFSET));
    hwaddr[5] = ((hval >> 8) & 0xff);
    hwaddr[4] = ((hval >> 0) & 0xff);
    hwaddr[3] = ((lval >> 24) & 0xff);
    hwaddr[2] = ((lval >> 16) & 0xff);
    hwaddr[1] = ((lval >> 8) & 0xff);
    hwaddr[0] = ((lval >> 0) & 0xff);
}

static inline void gkhw_set_link_mode_speed(struct GKETH_info *lp)
{
    u32                 val;

    val = gk_eth_readl((unsigned int)(lp->regbase + ETH_MAC_CFG_OFFSET));
    switch (lp->oldspeed) {
    case SPEED_1000:
        val &= ~(ETH_MAC_CFG_PS);
        break;
    case SPEED_100:
    case SPEED_10:
    default:
        val |= ETH_MAC_CFG_PS;
        break;
    }
    if (lp->oldduplex) {
        val &= ~(ETH_MAC_CFG_DO);
        val |= ETH_MAC_CFG_DM;
    } else {
        // Steven Yu: if DO==1, would disable the receive.
        val |= (ETH_MAC_CFG_DO);
        //val &= ~(ETH_MAC_CFG_DO);
        // Steven Yu: if DM==0, phytype==0, TX Error: No Carrier.
        val &= ~(ETH_MAC_CFG_DM);
        //val |= ETH_MAC_CFG_DM;
    }
    gk_eth_writel((unsigned int)(lp->regbase + ETH_MAC_CFG_OFFSET), val);
}

static inline int gkhw_enable(struct GKETH_info *lp)
{
    int                 errorCode = 0;

    //gk_set_gpio_output(&lp->platform_info->mii_power, 1);
    //gk_set_gpio_reset(&lp->platform_info->mii_reset);

    errorCode = gkhw_dma_reset(lp);
    if (errorCode)
        goto gkhw_init_exit;

    gkhw_set_hwaddr(lp, lp->ndev->dev_addr);
    gk_eth_writel((unsigned int)(lp->regbase + ETH_DMA_BUS_MODE_OFFSET),
        lp->platform_info->default_dma_bus_mode);
    gk_eth_writel((unsigned int)(lp->regbase + ETH_MAC_FRAME_FILTER_OFFSET), 0);
    gk_eth_writel((unsigned int)(lp->regbase + ETH_DMA_OPMODE_OFFSET),
        lp->platform_info->default_dma_opmode);
#if defined(CONFIG_GK_ETH_SUPPORT_IPC)
    gk_eth_setbitsl((unsigned int)(lp->regbase + ETH_DMA_OPMODE_OFFSET),
                    ETH_DMA_OPMODE_TSF);
#endif
    gk_eth_writel((unsigned int)(lp->regbase + ETH_DMA_STATUS_OFFSET),
        gk_eth_readl((unsigned int)(lp->regbase + ETH_DMA_STATUS_OFFSET)));
    gk_eth_writel((unsigned int)(lp->regbase + ETH_MAC_CFG_OFFSET),
        (ETH_MAC_CFG_TE | ETH_MAC_CFG_RE | (1<<4)| (1<<7)| (1<<8)));
#if defined(CONFIG_GK_ETH_SUPPORT_IPC)
    gk_eth_setbitsl((unsigned int)(lp->regbase + ETH_MAC_CFG_OFFSET),
        ETH_MAC_CFG_IPC);
#endif

gkhw_init_exit:
    return errorCode;
}

static inline void gkhw_disable(struct GKETH_info *lp)
{
    gkhw_stop_tx_rx(lp);
    gkhw_dma_int_disable(lp);
    //gk_set_gpio_output(&lp->platform_info->mii_power, 0);
    //gk_set_gpio_output(&lp->platform_info->mii_reset, 1);
}

static inline void gkhw_dump(struct GKETH_info *lp)
{
    u32                 i;

    dev_info(&lp->ndev->dev, "RX Info: cur_rx %d, dirty_rx %d.\n",
        lp->rx.cur_rx, lp->rx.dirty_rx);
    dev_info(&lp->ndev->dev, "RX Info: RX descriptor "
        "0x%08x 0x%08x 0x%08x 0x%08x.\n",
        lp->rx.desc_rx[lp->rx.dirty_rx % lp->rx_count].status,
        lp->rx.desc_rx[lp->rx.dirty_rx % lp->rx_count].length,
        lp->rx.desc_rx[lp->rx.dirty_rx % lp->rx_count].buffer1,
        lp->rx.desc_rx[lp->rx.dirty_rx % lp->rx_count].buffer2);
    dev_info(&lp->ndev->dev, "TX Info: cur_tx %d, dirty_tx %d.\n",
        lp->tx.cur_tx, lp->tx.dirty_tx);
    for (i = lp->tx.dirty_tx; i < lp->tx.cur_tx; i++) {
        dev_info(&lp->ndev->dev, "TX Info: TX descriptor[%d] "
            "0x%08x 0x%08x 0x%08x 0x%08x.\n", i,
            lp->tx.desc_tx[i % lp->tx_count].status,
            lp->tx.desc_tx[i % lp->tx_count].length,
            lp->tx.desc_tx[i % lp->tx_count].buffer1,
            lp->tx.desc_tx[i % lp->tx_count].buffer2);
    }
    for (i = 0; i <= 21; i++) {
        dev_info(&lp->ndev->dev, "GMAC[%d]: 0x%08x.\n", i,
        gk_eth_readl((unsigned int)(lp->regbase + ETH_MAC_CFG_OFFSET + (i << 2))));
    }
    for (i = 0; i <= 54; i++) {
        dev_info(&lp->ndev->dev, "GDMA[%d]: 0x%08x.\n", i,
        gk_eth_readl((unsigned int)(lp->regbase + ETH_DMA_BUS_MODE_OFFSET + (i << 2))));
    }
}

/* ==========================================================================*/
static int gkhw_mdio_read(struct mii_bus *bus,
    int mii_id, int regnum)
{
//#ifdef CONFIG_SYSTEM_USE_EXTERN_ETHPHY
    struct GKETH_info   *lp;
    int                 val;
    int                 limit;
    int                 addr;

//    printk("%s miid %d regnum:%d ",__FUNCTION__,mii_id,regnum);
    if(cmdline_phytype != 0){
        lp = (struct GKETH_info *)bus->priv;

        for (limit = lp->platform_info->mii_retry_limit; limit > 0; limit--) {
            if (!gk_eth_tstbitsl((unsigned int)(lp->regbase + ETH_MAC_GMII_ADDR_OFFSET),
                ETH_MAC_GMII_ADDR_GB))
                break;
            udelay(lp->platform_info->mii_retry_tmo);
        }
        if ((limit <= 0) && netif_msg_hw(lp)) {
            dev_err(&lp->ndev->dev, "MII Error: Preread tmo!\n");
            val = 0xFFFFFFFF;
            goto gkhw_mdio_read_exit;
        }

        val = ETH_MAC_GMII_ADDR_PA(mii_id) | ETH_MAC_GMII_ADDR_GR(regnum);
        //val |= ETH_MAC_GMII_ADDR_CR_100_150MHZ | ETH_MAC_GMII_ADDR_GB;
        val |= ETH_MAC_GMII_ADDR_CR_60_100MHZ | ETH_MAC_GMII_ADDR_GB;
        //val |= ETH_MAC_GMII_ADDR_CR_250_300MHZ | ETH_MAC_GMII_ADDR_GB;
        gk_eth_writel((unsigned int)(lp->regbase + ETH_MAC_GMII_ADDR_OFFSET), val);

        for (limit = lp->platform_info->mii_retry_limit; limit > 0; limit--) {
            if (!gk_eth_tstbitsl((unsigned int)(lp->regbase + ETH_MAC_GMII_ADDR_OFFSET),
                ETH_MAC_GMII_ADDR_GB))
                break;
            udelay(lp->platform_info->mii_retry_tmo);
        }
        if ((limit <= 0) && netif_msg_hw(lp)) {
            dev_err(&lp->ndev->dev, "MII Error: Postread tmo!\n");
            val = 0xFFFFFFFF;
            goto gkhw_mdio_read_exit;
        }

        val = gk_eth_readl((unsigned int)(lp->regbase + ETH_MAC_GMII_DATA_OFFSET));

    gkhw_mdio_read_exit:
        if (netif_msg_hw(lp))
        {
            printk("mdio read error\n");
            dev_info(&lp->ndev->dev,
                "MII Read: id[0x%02x], add[0x%02x], val[0x%04x].\n",
                mii_id, regnum, val);
        }

		if(val==0)
			val=0xffff;
//printk("value:%x\n",val);
        return val;
    } else {
//#else

        lp = (struct GKETH_info *)bus->priv;
        addr = REG_EPHY_CONTROL + regnum * 4;
        val = gk_eth_readl((unsigned int)(REG_EPHY_CONTROL + regnum * 4));
        if(regnum == 1){
            if((gk_eth_readl((unsigned int)(REG_EPHY_CONTROL + 4)) & (0x1 << 5)) != 0)
                val |= 0x1 << 2;
        }
        return val;
    }
//#endif
}

static int gkhw_mdio_write(struct mii_bus *bus,
    int mii_id, int regnum, u16 value)
{
//#ifdef CONFIG_SYSTEM_USE_EXTERN_ETHPHY
#define INTER_PHY_BASE 0xF0022000
    int                 errorCode = 0;
    struct GKETH_info   *lp;
    int                 val;
    int                 limit = 0;

    if(cmdline_phytype != 0){

        lp = (struct GKETH_info *)bus->priv;

        if (netif_msg_hw(lp))
            dev_info(&lp->ndev->dev,
                "MII Write: id[0x%02x], add[0x%02x], val[0x%04x].\n",
                mii_id, regnum, value);

        for (limit = lp->platform_info->mii_retry_limit; limit > 0; limit--) {
            if (!gk_eth_tstbitsl((unsigned int)(lp->regbase + ETH_MAC_GMII_ADDR_OFFSET),
                ETH_MAC_GMII_ADDR_GB))
                break;
            udelay(lp->platform_info->mii_retry_tmo);
        }
        if ((limit <= 0) && netif_msg_hw(lp)) {
            dev_err(&lp->ndev->dev, "MII Error: Prewrite tmo!\n");
            errorCode = -EIO;
            goto gkhw_mdio_write_exit;
        }

        val = value;
        gk_eth_writel((unsigned int)(lp->regbase + ETH_MAC_GMII_DATA_OFFSET), val);
        val = ETH_MAC_GMII_ADDR_PA(mii_id) | ETH_MAC_GMII_ADDR_GR(regnum);
        //val |= ETH_MAC_GMII_ADDR_CR_100_150MHZ | ETH_MAC_GMII_ADDR_GW |
        val |= ETH_MAC_GMII_ADDR_CR_60_100MHZ | ETH_MAC_GMII_ADDR_GW |
        //val |= ETH_MAC_GMII_ADDR_CR_250_300MHZ | ETH_MAC_GMII_ADDR_GW |
            ETH_MAC_GMII_ADDR_GB;
        gk_eth_writel((unsigned int)(lp->regbase + ETH_MAC_GMII_ADDR_OFFSET), val);

        for (limit = lp->platform_info->mii_retry_limit; limit > 0; limit--) {
            if (!gk_eth_tstbitsl((unsigned int)(lp->regbase + ETH_MAC_GMII_ADDR_OFFSET),
                ETH_MAC_GMII_ADDR_GB))
                break;
            udelay(lp->platform_info->mii_retry_tmo);
        }
        if ((limit <= 0) && netif_msg_hw(lp)) {
            dev_err(&lp->ndev->dev, "MII Error: Postwrite tmo!\n");
            errorCode = -EIO;
            goto gkhw_mdio_write_exit;
        }

gkhw_mdio_write_exit:
        return errorCode;
    } else {
//#else

        lp = (struct GKETH_info *)bus->priv;
        gk_eth_writel((unsigned int)(INTER_PHY_BASE + regnum * 4), value);
        return 0;
    }
//#endif
}

static int gkhw_mdio_reset(struct mii_bus *bus)
{
    int                 errorCode = 0;
    struct GKETH_info           *lp;

    lp = (struct GKETH_info *)bus->priv;

    // LindengYu: Add reset to here
    printk("###### PHY Reset.1.0.2\n");
    //gk_set_gpio_output(&lp->platform_info->phy_reset, 0);
    gk_gpio_set_out(lp->platform_info->phy_reset.gpio_id, 0);
    //*(u32 *)(GK_VA_GPIO0 + REG_GPIO_OUTPUT_CFG_OFFSET + 6*4) = 0X0;
    mdelay(10);//100ms
    //gk_set_gpio_output(&lp->platform_info->phy_reset, 1);
    gk_gpio_set_out(lp->platform_info->phy_reset.gpio_id, 1);
    //*(u32 *)(GK_VA_GPIO0 + REG_GPIO_OUTPUT_CFG_OFFSET + 6*4) = 0X1;
    // RTL8201 need 200ms
    mdelay(100);//100ms
    //gk_set_gpio_reset_can_sleep(&lp->platform_info->phy_reset, 50);
    return errorCode;
}

#if 0
static int gkhw_phy_reset(struct GKETH_info *lp)
{
//#ifdef CONFIG_SYSTEM_USE_EXTERN_ETHPHY
    printk("cmdline_phytype = %d\n", cmdline_phytype);
    if(cmdline_phytype != 0){
        struct gk_eth_platform_info *pf;

        pf = lp->platform_info;

        if(pf != NULL)
        {
            //dev_err(&lp->ndev->dev,"###### PHY Reset.1.0.1\n");
            printk("###### PHY Reset.1.0.1\n");
            gk_set_gpio_output(&pf->phy_reset, 0);
            mdelay(50);//100ms
            gk_set_gpio_output(&pf->phy_reset, 1);
            mdelay(200);//100ms
        }

        //gk_set_gpio_output(&lp->platform_info->mii_power, 0);
        //gk_set_gpio_output(&lp->platform_info->mii_reset, 1);
        //gk_set_gpio_output(&lp->platform_info->mii_power, 1);
        //gk_set_gpio_output(&lp->platform_info->mii_reset, 0);
    }
//#endif
    return 0;

}
#endif

/* ==========================================================================*/
static void GKETH_adjust_link(struct net_device *ndev)
{
    struct GKETH_info   *lp;
    unsigned long       flags;
    struct phy_device   *phydev;
    int                 need_update = 0;

    lp = (struct GKETH_info *)netdev_priv(ndev);

    spin_lock_irqsave(&lp->lock, flags);
    phydev = lp->phydev;
    if (phydev->link)
    {
        if (phydev->duplex != lp->oldduplex)
        {
            need_update = 1;
            lp->oldduplex = phydev->duplex;
        }
        if (phydev->speed != lp->oldspeed)
        {
            switch (phydev->speed)
            {
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
        if (lp->oldlink != phydev->link)
        {
            need_update = 1;
            lp->oldlink = phydev->link;
        }
    }
    else if (lp->oldlink)
    {
        need_update = 1;
        lp->oldlink = PHY_DOWN;
        lp->oldspeed = 0;
        lp->oldduplex = -1;
    }

    if (need_update)
    {
        u16 rmii;
        gkhw_set_link_mode_speed(lp);
        rmii = gkhw_mdio_read(&lp->new_bus, lp->new_bus.phy_used->addr , 0x01);
//        printk("#############-------------reg1 %04x--------\n", rmii);
                if(cmdline_phytype == 0)
                {
        if (phydev->link)
        {
            switch (phydev->speed)
            {
            case SPEED_100:
                if(cmdline_phytype == 0)
                {
                    gk_set_phy_speed_led(GPIO_TYPE_OUTPUT_EPHY_LED_3);
                }
                GH_GPIO_set_INPUT_CFG_in_sel(GPIO_GET_IN_SEL(GPIO_TYPE_INPUT_ENET_PHY_RXD_3) - 2, 0x01);
                break;
            case SPEED_10:
                if(cmdline_phytype == 0)
                {
                    gk_set_phy_speed_led(GPIO_TYPE_OUTPUT_EPHY_LED_2);
                }
                GH_GPIO_set_INPUT_CFG_in_sel(GPIO_GET_IN_SEL(GPIO_TYPE_INPUT_ENET_PHY_RXD_3) - 2, 0x00);
                break;
            default:
                break;
            }
        }
        else
        {
            if(cmdline_phytype == 0)
            {
                gk_set_phy_speed_led(GPIO_TYPE_INPUT_0);
            }
        }
        }
        if (netif_msg_link(lp))
            phy_print_status(phydev);
    }
    spin_unlock_irqrestore(&lp->lock, flags);
}

typedef union { /* EPHY_MII_RMII */
    u32 all;
    struct {
        u32 usb_tm1 : 1;
        u32 rmii    : 1;
        u32         : 30;
    } bitc;
} GH_EPHY_MII_RMII_S;

#define REG_EPHY_MII_RMII  GK_VA_AHB_GREG  //(0xF0020E00) /* read/write */

void GH_EPHY_set_MII_RMII_rmii(u8 data)
{
    GH_EPHY_MII_RMII_S d;
	d.all = gk_eth_readl(REG_EPHY_MII_RMII);
    //d.all = *(volatile u32 *)REG_EPHY_MII_RMII;
    d.bitc.rmii = data;
	gk_eth_writel(REG_EPHY_MII_RMII, d.all);
    //*(volatile u32 *)REG_EPHY_MII_RMII = d.all;
}

static int GKETH_phy_start(struct GKETH_info *lp)
{
//#ifdef CONFIG_SYSTEM_USE_EXTERN_ETHPHY
    int                 errorCode = 0;
    struct phy_device   *phydev;
    phy_interface_t     interface;
    struct net_device   *ndev;
    int                 phy_addr;
    unsigned long       flags;

    spin_lock_irqsave(&lp->lock, flags);
    phydev = lp->phydev;
    spin_unlock_irqrestore(&lp->lock, flags);
    if (phydev)
        goto GKETH_init_phy_exit;

    ndev = lp->ndev;
    lp->oldlink = PHY_DOWN;
    lp->oldspeed = 0;
    lp->oldduplex = -1;

    if(cmdline_phytype != 0)
    {
        phy_addr = lp->platform_info->mii_id;
        if ((phy_addr >= 0) && (phy_addr < PHY_MAX_ADDR))
        {
            if (lp->new_bus.phy_map[phy_addr])
            {
                phydev = lp->new_bus.phy_map[phy_addr];
                if (phydev->phy_id == lp->platform_info->phy_id)
                {
                    goto GKETH_init_phy_default;
                }
            }
            dev_notice(&lp->ndev->dev,
                "Could not find default PHY in %d.\n", phy_addr);
        }
        goto GKETH_init_phy_scan;

    GKETH_init_phy_default:
        printk("GKETH_init_phy_default ...\n");
        if (netif_msg_hw(lp))
            dev_info(&lp->ndev->dev, "Find default PHY in %d!\n", phy_addr);
        goto GKETH_init_phy_connect;

    GKETH_init_phy_scan:
        for (phy_addr = 0; phy_addr < PHY_MAX_ADDR; phy_addr++) {
            if (lp->new_bus.phy_map[phy_addr]) {
                phydev = lp->new_bus.phy_map[phy_addr];
                if (phydev->phy_id == lp->platform_info->phy_id)
                    goto GKETH_init_phy_connect;
            }
        }
        if (!phydev) {
            if (netif_msg_drv(lp))
                dev_err(&lp->ndev->dev, "No PHY device.\n");
            errorCode = -ENODEV;
            goto GKETH_init_phy_exit;
        } else {
            if (netif_msg_drv(lp))
                dev_notice(&lp->ndev->dev,
                "Try PHY[%d] whose id is 0x%08x!\n",
                phydev->addr, phydev->phy_id);
        }

    GKETH_init_phy_connect:
        interface = gkhw_get_interface(lp);

        phydev = phy_connect(ndev, dev_name(&phydev->dev),
            &GKETH_adjust_link, 0, interface);
        if (IS_ERR(phydev)) {
            if (netif_msg_drv(lp))
                dev_err(&lp->ndev->dev, "Could not attach to PHY!\n");
            errorCode = PTR_ERR(phydev);
            goto GKETH_init_phy_exit;
        }

        //phydev->supported &= lp->platform_info->phy_supported;
        phydev->supported = lp->platform_info->phy_supported;
        phydev->advertising = phydev->supported;

        //GH_EPHY_set_MII_RMII_rmii(0x0);


        spin_lock_irqsave(&lp->lock, flags);
        lp->phydev = phydev;
        spin_unlock_irqrestore(&lp->lock, flags);

        dev_err(&lp->ndev->dev,"###### GKETH_phy_start_aneg...\n");

        errorCode = phy_start_aneg(phydev);

    GKETH_init_phy_exit:
        return errorCode;
    } else {

//#else
        interface = gkhw_get_interface(lp);
        phydev = lp->new_bus.phy_map[0];
        phydev = phy_connect(ndev, dev_name(&phydev->dev),
            &GKETH_adjust_link, 0, interface);
        if (IS_ERR(phydev)) {
            if (netif_msg_drv(lp))
                dev_err(&lp->ndev->dev, "Could not attach to PHY!\n");
            errorCode = PTR_ERR(phydev);
            return errorCode;
        }

        phydev->supported = lp->platform_info->phy_supported;
        phydev->advertising = phydev->supported;

        spin_lock_irqsave(&lp->lock, flags);
        lp->phydev = phydev;
        spin_unlock_irqrestore(&lp->lock, flags);

        dev_err(&lp->ndev->dev,"###### GKETH_phy_start_aneg...\n");

        errorCode = phy_start_aneg(phydev);
        return 0;
    }
}

static void GKETH_phy_stop(struct GKETH_info *lp)
{
    struct phy_device           *phydev;
    unsigned long               flags;

    spin_lock_irqsave(&lp->lock, flags);
    phydev = lp->phydev;
    dev_err(&lp->ndev->dev,"###### GKETH_phy_stop\n");
    lp->phydev = NULL;
    spin_unlock_irqrestore(&lp->lock, flags);
    if (phydev)
        phy_disconnect(phydev);
}

static inline int GKETH_rx_rngmng_check_skb(struct GKETH_info *lp, u32 entry)
{
    int                 errorCode = 0;
    dma_addr_t              mapping;
    struct sk_buff              *skb;

    if (lp->rx.rng_rx[entry].skb == NULL) {
        skb = dev_alloc_skb(GKETH_PACKET_MAXFRAME);
        if (skb == NULL) {
            if (netif_msg_drv(lp))
                dev_err(&lp->ndev->dev,
                "RX Error: dev_alloc_skb.\n");
            errorCode = -ENOMEM;
            goto GKETH_rx_rngmng_skb_exit;
        }

        skb->dev                      = lp->ndev;
        lp->rx.rng_rx[entry].buf_size = GKETH_PACKET_MAXFRAME;
        mapping                       = dma_map_single(&lp->ndev->dev,
                                                       skb->data,
                                                       lp->rx.rng_rx[entry].buf_size,
                                                       DMA_FROM_DEVICE);

        lp->rx.rng_rx[entry].skb = skb;
        lp->rx.rng_rx[entry].mapping = mapping;
        lp->rx.desc_rx[entry].buffer1 = mapping;
    }

GKETH_rx_rngmng_skb_exit:
    return errorCode;
}

static inline void GKETH_rx_rngmng_init(struct GKETH_info *lp)
{
    int                 i;

    lp->rx.cur_rx = 0;
    lp->rx.dirty_rx = 0;
    for (i = 0; i < lp->rx_count; i++) {
        if (GKETH_rx_rngmng_check_skb(lp, i))
            break;
        lp->rx.desc_rx[i].status = ETH_RDES0_OWN;
        lp->rx.desc_rx[i].length =
            ETH_RDES1_RCH | GKETH_PACKET_MAXFRAME;
        lp->rx.desc_rx[i].buffer2 = (u32)lp->rx_dma_desc +
            ((i + 1) * sizeof(struct GKETH_desc));
    }
    lp->rx.desc_rx[lp->rx_count - 1].buffer2 = (u32)lp->rx_dma_desc;
}

static inline void GKETH_rx_rngmng_refill(struct GKETH_info *lp)
{
    u32                 entry;

    while (lp->rx.cur_rx > lp->rx.dirty_rx) {
        entry = lp->rx.dirty_rx % lp->rx_count;
        if (GKETH_rx_rngmng_check_skb(lp, entry))
            break;
        lp->rx.desc_rx[entry].status = ETH_RDES0_OWN;
        lp->rx.dirty_rx++;
    }
}

static inline void GKETH_rx_rngmng_del(struct GKETH_info *lp)
{
    int                 i;
    dma_addr_t              mapping;
    struct sk_buff              *skb;

    for (i = 0; i < lp->rx_count; i++) {
        if (lp->rx.rng_rx) {
            skb = lp->rx.rng_rx[i].skb;
            mapping = lp->rx.rng_rx[i].mapping;
            lp->rx.rng_rx[i].skb = NULL;
            lp->rx.rng_rx[i].mapping = 0;
            if (skb) {
                dma_unmap_single(&lp->ndev->dev, mapping,
                    lp->rx.rng_rx[i].buf_size, DMA_FROM_DEVICE);
                dev_kfree_skb(skb);
            }
            lp->rx.rng_rx[i].buf_size = 0;
        }
        if (lp->rx.desc_rx) {
            lp->rx.desc_rx[i].status = 0;
            lp->rx.desc_rx[i].length = 0;
            lp->rx.desc_rx[i].buffer1 = 0xBADF00D0;
            lp->rx.desc_rx[i].buffer2 = 0xBADF00D0;
        }
    }
}

static inline void GKETH_tx_rngmng_init(struct GKETH_info *lp)
{
    int                 i;

    lp->tx.cur_tx = 0;
    lp->tx.dirty_tx = 0;
    for (i = 0; i < lp->tx_count; i++) {
        lp->tx.rng_tx[i].mapping = 0 ;
        lp->tx.desc_tx[i].length = (ETH_TDES1_LS | ETH_TDES1_FS |
            ETH_TDES1_TCH);
        lp->tx.desc_tx[i].buffer1 = 0;
        lp->tx.desc_tx[i].buffer2 = (u32)lp->tx_dma_desc +
            ((i + 1) * sizeof(struct GKETH_desc));
    }
    lp->tx.desc_tx[lp->tx_count - 1].buffer2 = (u32)lp->tx_dma_desc;
}

static inline void GKETH_tx_rngmng_del(struct GKETH_info *lp)
{
    int                 i;
    dma_addr_t              mapping;
    struct sk_buff              *skb;
    unsigned int                dirty_tx;
    u32                 entry;
    u32                 status;

    for (dirty_tx = lp->tx.dirty_tx; lp->tx.cur_tx > dirty_tx; dirty_tx++) {
        entry = dirty_tx % lp->tx_count;
        if (lp->tx.desc_tx) {
            status = lp->tx.desc_tx[entry].status;
            if (status & ETH_TDES0_OWN)
                lp->stats.tx_dropped++;
        }
    }
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

static inline void GKETH_check_dma_error(struct GKETH_info *lp,
    u32 irq_status)
{
    u32                 miss_ov = 0;

    if (unlikely(irq_status & ETH_DMA_STATUS_AIS)) {
        if (irq_status & (ETH_DMA_STATUS_RU | ETH_DMA_STATUS_OVF))
            miss_ov = gk_eth_readl((unsigned int)(lp->regbase +
                ETH_DMA_MISS_FRAME_BOCNT_OFFSET));

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
            gkhw_dma_rx_stop(lp);
            if (netif_msg_rx_err(lp))
                dev_err(&lp->ndev->dev,
                "DMA Error: Receive Buffer Unavailable, %d.\n",
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
                "DMA Error: Receive FIFO Overflow, %d.\n",
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
            gkhw_dump(lp);
        }
    }
}

static inline void GKETH_interrupt_rx(struct GKETH_info *lp, u32 irq_status)
{
    if (irq_status & GKETH_RXDMA_STATUS) {
        dev_vdbg(&lp->ndev->dev, "RX IRQ[0x%x]!\n", irq_status);
        gk_eth_clrbitsl((unsigned int)(lp->regbase + ETH_DMA_INTEN_OFFSET),
            GKETH_RXDMA_INTEN);
        napi_schedule(&lp->napi);
    }
}

static inline u32 GKETH_check_tdes0_status(struct GKETH_info *lp,
    unsigned int status)
{
    u32                 tx_retry = 0;

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
            "TX Error: Excessive Collision %d.\n",
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
            gkhw_dump(lp);
        }
    }
    if (status & ETH_TDES0_DB) {
        lp->stats.tx_fifo_errors++;
        if (netif_msg_drv(lp))
            dev_err(&lp->ndev->dev, "TX Error: Deferred Bit.\n");
    }

    return tx_retry;
}

static inline void GKETH_interrupt_tx(struct GKETH_info *lp, u32 irq_status)
{
    unsigned int                dirty_tx;
    unsigned int                dirty_to_tx;
    u32                 entry;
    u32                 status;

    if (irq_status & GKETH_TXDMA_STATUS)
    {
        dev_vdbg(&lp->ndev->dev, "cur_tx[%d], dirty_tx[%d], 0x%x.\n",
            lp->tx.cur_tx, lp->tx.dirty_tx, irq_status);
        for (dirty_tx = lp->tx.dirty_tx; dirty_tx < lp->tx.cur_tx; dirty_tx++)
        {
            entry = dirty_tx % lp->tx_count;
            status = lp->tx.desc_tx[entry].status;

            if (status & ETH_TDES0_OWN)
                break;

            if (unlikely(status & ETH_TDES0_ES))
            {
#if defined(GKETH_TDES0_ATOMIC_CHECK)
                if ((status & ETH_TDES0_ES_MASK) ==
                    ETH_TDES0_ES) {
                    if (netif_msg_probe(lp)) {
                        dev_err(&lp->ndev->dev,
                        "TX Error: Wrong ES"
                        " 0x%08x vs 0x%08x.\n",
                        status,
                        lp->tx.desc_tx[entry].status);
                        gkhw_dump(lp);
                    }
                    break;
                }
#endif
                if (GKETH_check_tdes0_status(lp, status)) {
                    gkhw_dma_tx_stop(lp);
                    gkhw_dma_tx_restart(lp, entry);
                    gkhw_dma_tx_poll(lp, entry);
                    break;
                } else {
                    lp->stats.tx_errors++;
                }
            }
            else
            {
#if defined(GKETH_TDES0_ATOMIC_CHECK_ALL)
                udelay(1);
                if (unlikely(status !=
                    lp->tx.desc_tx[entry].status)) {
                    if (netif_msg_probe(lp)) {
                        dev_err(&lp->ndev->dev,
                        "TX Error: Wrong status"
                        " 0x%08x vs 0x%08x.\n",
                        status,
                        lp->tx.desc_tx[entry].status);
                        gkhw_dump(lp);
                    }
                }
#endif
#if defined(CONFIG_GK_ETH_SUPPORT_IPC)
                if (unlikely(status & ETH_TDES0_IHE)) {
                    if (netif_msg_drv(lp))
                        dev_err(&lp->ndev->dev,
                        "TX Error: IP Header Error.\n");
                }
#endif
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
        }

        dirty_to_tx = lp->tx.cur_tx - dirty_tx;
        if (unlikely(dirty_to_tx > lp->tx_count)) {
            netif_stop_queue(lp->ndev);
            if (netif_msg_drv(lp))
                dev_err(&lp->ndev->dev, "TX Error: TX OV.\n");
            gkhw_dump(lp);
            gkhw_dma_tx_stop(lp);
            GKETH_tx_rngmng_del(lp);
            GKETH_tx_rngmng_init(lp);
            dirty_tx = dirty_to_tx = 0;
        }
        if (likely(dirty_to_tx < (lp->tx_count / 2))) {
            dev_vdbg(&lp->ndev->dev, "TX Info: Now gap %d.\n",
                dirty_to_tx);
            netif_wake_queue(lp->ndev);
        }
        lp->tx.dirty_tx = dirty_tx;
    }
}

static irqreturn_t GKETH_interrupt(int irq, void *dev_id)
{
    struct net_device           *ndev;
    struct GKETH_info           *lp;
    u32                 irq_status;
    unsigned long               flags;

    ndev = (struct net_device *)dev_id;
    lp = netdev_priv(ndev);

    spin_lock_irqsave(&lp->lock, flags);
    irq_status = gk_eth_readl((unsigned int)(lp->regbase + ETH_DMA_STATUS_OFFSET));
    GKETH_check_dma_error(lp, irq_status);
    GKETH_interrupt_rx(lp, irq_status);
    GKETH_interrupt_tx(lp, irq_status);
    gk_eth_writel((unsigned int)(lp->regbase + ETH_DMA_STATUS_OFFSET), irq_status);
    spin_unlock_irqrestore(&lp->lock, flags);

    return IRQ_HANDLED;
}

static int GKETH_start_hw(struct net_device *ndev)
{
    int                 errorCode = 0;
    struct GKETH_info           *lp;
    unsigned long               flags;

    lp = (struct GKETH_info *)netdev_priv(ndev);
    dev_err(&lp->ndev->dev,"###### GKETH_start_hw\n");

    #if 1
    //kewell add
    //LindengYu: if reset, might change the phy address. So, reset the phy before at the get phy id.
    //gkhw_phy_reset(lp);
    //GKETH_phy_start(lp);
    #else
    gkhw_phy_set_high(lp);
    gkhw_phy_reset(lp);
    GH_EPHY_set_MII_RMII_rmii(0x1);
    #endif

    spin_lock_irqsave(&lp->lock, flags);
    errorCode = gkhw_enable(lp);
    spin_unlock_irqrestore(&lp->lock, flags);
    if (errorCode)
        goto GKETH_start_hw_exit;

    lp->rx.rng_rx = kmalloc((sizeof(struct GKETH_rng_info) *
        lp->rx_count), GFP_KERNEL);
    if (lp->rx.rng_rx == NULL) {
        if (netif_msg_drv(lp))
            dev_err(&lp->ndev->dev, "alloc rng_rx fail.\n");
        errorCode = -ENOMEM;
        goto GKETH_start_hw_exit;
    }
    lp->rx.desc_rx = dma_alloc_coherent(&lp->ndev->dev,
        (sizeof(struct GKETH_desc) * lp->rx_count),
        &lp->rx_dma_desc, GFP_KERNEL);
    if (lp->rx.desc_rx == NULL) {
        if (netif_msg_drv(lp))
            dev_err(&lp->ndev->dev,
            "dma_alloc_coherent desc_rx fail.\n");
        errorCode = -ENOMEM;
        goto GKETH_start_hw_exit;
    }
    memset(lp->rx.rng_rx, 0,
        (sizeof(struct GKETH_rng_info) * lp->rx_count));
    memset(lp->rx.desc_rx, 0,
        (sizeof(struct GKETH_desc) * lp->rx_count));
    GKETH_rx_rngmng_init(lp);

    lp->tx.rng_tx = kmalloc((sizeof(struct GKETH_rng_info) *
        lp->tx_count), GFP_KERNEL);
    if (lp->tx.rng_tx == NULL) {
        if (netif_msg_drv(lp))
            dev_err(&lp->ndev->dev, "alloc rng_tx fail.\n");
        errorCode = -ENOMEM;
        goto GKETH_start_hw_exit;
    }
    lp->tx.desc_tx = dma_alloc_coherent(&lp->ndev->dev,
        (sizeof(struct GKETH_desc) * lp->tx_count),
        &lp->tx_dma_desc, GFP_KERNEL);
    if (lp->tx.desc_tx == NULL) {
        if (netif_msg_drv(lp))
            dev_err(&lp->ndev->dev,
            "dma_alloc_coherent desc_tx fail.\n");
        errorCode = -ENOMEM;
        goto GKETH_start_hw_exit;
    }
    memset(lp->tx.rng_tx, 0,
        (sizeof(struct GKETH_rng_info) * lp->tx_count));
    memset(lp->tx.desc_tx, 0,
        (sizeof(struct GKETH_desc) * lp->tx_count));
    GKETH_tx_rngmng_init(lp);

    spin_lock_irqsave(&lp->lock, flags);
    gkhw_set_dma_desc(lp);
    gkhw_dma_rx_start(lp);
    gkhw_dma_tx_start(lp);
    spin_unlock_irqrestore(&lp->lock, flags);

GKETH_start_hw_exit:
    return errorCode;
}

static void GKETH_stop_hw(struct net_device *ndev)
{
    struct GKETH_info           *lp;
    unsigned long               flags;

    lp = (struct GKETH_info *)netdev_priv(ndev);

    spin_lock_irqsave(&lp->lock, flags);
    gkhw_disable(lp);
    spin_unlock_irqrestore(&lp->lock, flags);

    GKETH_tx_rngmng_del(lp);
    if (lp->tx.desc_tx) {
        dma_free_coherent(&lp->ndev->dev,
            (sizeof(struct GKETH_desc) * lp->tx_count),
            lp->tx.desc_tx, lp->tx_dma_desc);
        lp->tx.desc_tx = NULL;
    }
    if (lp->tx.rng_tx) {
        kfree(lp->tx.rng_tx);
        lp->tx.rng_tx = NULL;
    }

    GKETH_rx_rngmng_del(lp);
    if (lp->rx.desc_rx) {
        dma_free_coherent(&lp->ndev->dev,
            (sizeof(struct GKETH_desc) * lp->rx_count),
            lp->rx.desc_rx, lp->rx_dma_desc);
        lp->rx.desc_rx = NULL;
    }
    if (lp->rx.rng_rx) {
        kfree(lp->rx.rng_rx);
        lp->rx.rng_rx = NULL;
    }
}

static int GKETH_open(struct net_device *ndev)
{
    int                 errorCode = 0;
    struct GKETH_info           *lp;
//#ifdef CONFIG_SYSTEM_USE_EXTERN_ETHPHY
    u32 rmii;
//#endif

	if(!(cmdline_phytype != 0)) {
		GK_EPHY_POWER_ON();
		udelay(1000);
	}

    lp = (struct GKETH_info *)netdev_priv(ndev);

    errorCode = GKETH_start_hw(ndev);
    if (errorCode)
        goto GKETH_open_exit;

    errorCode = request_irq(ndev->irq, GKETH_interrupt,
        IRQF_SHARED | IRQF_TRIGGER_HIGH, ndev->name, ndev);
    if (errorCode) {
        if (netif_msg_drv(lp))
            dev_err(&lp->ndev->dev,
            "Request_irq[%d] fail.\n", ndev->irq);
        goto GKETH_open_exit;
    }

    napi_enable(&lp->napi);
    netif_start_queue(ndev);


    netif_carrier_off(ndev);


    gkhw_dma_int_enable(lp);

    errorCode = GKETH_phy_start(lp);

#if 0
{
    u32 ii;
    u32 phy_register_value = 0;

    for(ii = 0; ii < 32; ii++)
    {
        phy_register_value = gkhw_mdio_read(&lp->new_bus, 3 , ii);
        //printk(" [%02d]: 0x%08x\n", ii, phy_register_value);
        printk( "MII Read: id[0x03], add[0x%02x], val[0x%04x].\n",
             ii, phy_register_value);
    }
}
#endif
//#ifdef CONFIG_SYSTEM_USE_EXTERN_ETHPHY
    if(cmdline_phytype != 0)
    {
        if(lp->new_bus.phy_used->phy_id == 0x02430c54) // IP101g
        {
            // LindengYu: I don't known why do not go into effect.
            // change to page 3
            gkhw_mdio_write(&lp->new_bus, lp->new_bus.phy_used->addr, 20, 0x03);
            rmii = gkhw_mdio_read(&lp->new_bus, lp->new_bus.phy_used->addr, 0x10);
            rmii &= 0x3FFF;
            rmii |= 0x4000;
            // change to LED0 mode1, Link/Act mode
            gkhw_mdio_write(&lp->new_bus, lp->new_bus.phy_used->addr, 0x10, rmii);
            // restore to page 16
            rmii = gkhw_mdio_read(&lp->new_bus, lp->new_bus.phy_used->addr, 0x10);
        }
        else// if(lp->new_bus.phy_used->phy_id == 0x001CC815) // rtl820x
        {
            // change to rmii mode
            rmii = gkhw_mdio_read(&lp->new_bus, lp->new_bus.phy_used->addr, 25);
            //rmii |= (1<<10);
            rmii &= ~(1<<10);
            rmii &= ~(1<<11);
            gkhw_mdio_write(&lp->new_bus, lp->new_bus.phy_used->addr, 25, rmii);
        }
    }

//#endif

GKETH_open_exit:
    if (errorCode) {
        printk("eth open error\n");
        GKETH_phy_stop(lp);
        GKETH_stop_hw(ndev);
    }

    return errorCode;
}

static int GKETH_stop(struct net_device *ndev)
{
    int                 errorCode = 0;
    struct GKETH_info           *lp;

    lp = (struct GKETH_info *)netdev_priv(ndev);

    netif_stop_queue(ndev);
    napi_disable(&lp->napi);
    flush_scheduled_work();
    free_irq(ndev->irq, ndev);
    GKETH_phy_stop(lp);
    GKETH_stop_hw(ndev);

    //printk("GKETH_stop\n");
    GK_EPHY_POWER_OFF();

    return errorCode;
}

static int GKETH_hard_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
    int                 errorCode = 0;
    struct GKETH_info           *lp;
    dma_addr_t              mapping;
    u32                 entry;
    unsigned int                dirty_to_tx;
    u32                 tx_flag;
    unsigned long               flags;

    lp = (struct GKETH_info *)netdev_priv(ndev);

    spin_lock_irqsave(&lp->lock, flags);
    entry = lp->tx.cur_tx % lp->tx_count;
    dirty_to_tx = lp->tx.cur_tx - lp->tx.dirty_tx;
    if (dirty_to_tx >= lp->tx_count) {
        netif_stop_queue(ndev);
        errorCode = -ENOMEM;
        spin_unlock_irqrestore(&lp->lock, flags);
        goto GKETH_hard_start_xmit_exit;
    }

    mapping = dma_map_single(&lp->ndev->dev,
        skb->data, skb->len, DMA_TO_DEVICE);
    tx_flag = ETH_TDES1_LS | ETH_TDES1_FS | ETH_TDES1_TCH;
    if (dirty_to_tx >= lp->tx_irq_count) {
        netif_stop_queue(ndev);
        tx_flag |= ETH_TDES1_IC;
    }
#if defined(CONFIG_GK_ETH_SUPPORT_IPC)
    if (skb->ip_summed == CHECKSUM_PARTIAL) {
        tx_flag |= ETH_TDES1_CIC_TUI | ETH_TDES1_CIC_HDR;
    }
#endif
    lp->tx.rng_tx[entry].skb = skb;
    lp->tx.rng_tx[entry].mapping = mapping;
    lp->tx.desc_tx[entry].buffer1 = mapping;
    lp->tx.desc_tx[entry].length = ETH_TDES1_TBS1x(skb->len) | tx_flag;
    lp->tx.cur_tx++;
    gkhw_dma_tx_poll(lp, entry);
    spin_unlock_irqrestore(&lp->lock, flags);

    ndev->trans_start = jiffies;
    dev_vdbg(&lp->ndev->dev, "TX Info: cur_tx[%d], dirty_tx[%d], "
        "entry[%d], len[%d], data_len[%d], ip_summed[%d], "
        "csum_start[%d], csum_offset[%d].\n",
        lp->tx.cur_tx, lp->tx.dirty_tx, entry, skb->len, skb->data_len,
        skb->ip_summed, skb->csum_start, skb->csum_offset);

GKETH_hard_start_xmit_exit:
    return errorCode;
}

static void GKETH_timeout(struct net_device *ndev)
{
    struct GKETH_info           *lp;
    unsigned long               flags;
    u32                 irq_status;

    lp = (struct GKETH_info *)netdev_priv(ndev);

    dev_info(&lp->ndev->dev, "OOM Info:...\n");
    spin_lock_irqsave(&lp->lock, flags);
    irq_status = gk_eth_readl((unsigned int)(lp->regbase + ETH_DMA_STATUS_OFFSET));
    GKETH_interrupt_tx(lp, irq_status | GKETH_TXDMA_STATUS);
    gkhw_dump(lp);
    spin_unlock_irqrestore(&lp->lock, flags);

    netif_wake_queue(ndev);
}

static struct net_device_stats *GKETH_get_stats(struct net_device *ndev)
{
    struct GKETH_info *lp = netdev_priv(ndev);

    return &lp->stats;
}

static inline void GKETH_check_rdes0_status(struct GKETH_info *lp,
    unsigned int status)
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
        if (gk_eth_tstbitsl((unsigned int)(lp->regbase + ETH_MAC_CFG_OFFSET),
            ETH_MAC_CFG_PS)) {
            lp->stats.rx_length_errors++;
            if (netif_msg_drv(lp))
                dev_err(&lp->ndev->dev,
                "RX Error: Dribble Bit.\n");
        }
    }
    if (status & ETH_RDES0_CE) {
        lp->stats.rx_crc_errors++;
        if (netif_msg_rx_err(lp))
            dev_err(&lp->ndev->dev, "RX Error: CRC.\n");
    }
    if (status & ETH_RDES0_RX) {
        if (netif_msg_drv(lp))
            dev_err(&lp->ndev->dev,
            "RX Error: Rx MAC Address/Payload Checksum.\n");
    }
}

static inline void GKETH_napi_rx(struct GKETH_info *lp, u32 status, u32 entry)
{
    short                   pkt_len;
    struct sk_buff              *skb;
    dma_addr_t              mapping;

    pkt_len = ETH_RDES0_FL(status) - 4;

    if (unlikely(pkt_len > GKETH_RX_COPYBREAK)) {
        dev_warn(&lp->ndev->dev, "Bogus packet size %d.\n", pkt_len);
        pkt_len = GKETH_RX_COPYBREAK;
        lp->stats.rx_length_errors++;
    }

    skb = lp->rx.rng_rx[entry].skb;
    mapping = lp->rx.rng_rx[entry].mapping;
    if (likely(skb && mapping)) {
        dma_unmap_single(&lp->ndev->dev, mapping,
            lp->rx.rng_rx[entry].buf_size, DMA_FROM_DEVICE);
        skb->len = 0;
        skb_put(skb, pkt_len);
        skb->protocol = eth_type_trans(skb, lp->ndev);
#if defined(CONFIG_GK_ETH_SUPPORT_IPC)
        if ((status & ETH_RDES0_COE_MASK) == ETH_RDES0_COE_NOCHKERROR) {
            skb->ip_summed = CHECKSUM_UNNECESSARY;
        } else {
            skb->ip_summed = CHECKSUM_NONE;
            if (netif_msg_rx_err(lp)) {
                dev_err(&lp->ndev->dev,
                "RX Error: RDES0_COE[0x%x].\n", status);
                gkhw_dump(lp);
            }
        }
#endif
        netif_receive_skb(skb);
        lp->rx.rng_rx[entry].skb = NULL;
        lp->rx.rng_rx[entry].mapping = 0;
        lp->ndev->last_rx = jiffies;
        lp->stats.rx_packets++;
        lp->stats.rx_bytes += pkt_len;
    } else {
        if (netif_msg_drv(lp)) {
            dev_err(&lp->ndev->dev,
            "RX Error: %d skb[%p], map[0x%08X].\n",
            entry, skb, mapping);
            gkhw_dump(lp);
        }
    }
}

int GKETH_napi(struct napi_struct *napi, int budget)
{
    int                 rx_budget = budget;
    struct GKETH_info           *lp;
    u32                 entry;
    u32                 status;
    unsigned long               flags;

    lp = container_of(napi, struct GKETH_info, napi);

    if (unlikely(!netif_carrier_ok(lp->ndev)))
        goto GKETH_poll_complete;

    while (rx_budget > 0) {
        entry = lp->rx.cur_rx % lp->rx_count;
        status = lp->rx.desc_rx[entry].status;
        if (status & ETH_RDES0_OWN)
            break;
#if defined(GKETH_RDES0_ATOMIC_CHECK)
        if (unlikely((status & (ETH_RDES0_FS | ETH_RDES0_LS)) !=
            (ETH_RDES0_FS | ETH_RDES0_LS))) {
            if (netif_msg_probe(lp)) {
                dev_err(&lp->ndev->dev, "RX Error: Wrong FS/LS"
                " cur_rx[%d] status 0x%08x.\n",
                lp->rx.cur_rx, status);
                gkhw_dump(lp);
            }
            break;
        }
#endif
#if defined(GKETH_TDES0_ATOMIC_CHECK_ALL)
        udelay(1);
        if (unlikely(status != lp->rx.desc_rx[entry].status)) {
            if (netif_msg_probe(lp)) {
                dev_err(&lp->ndev->dev, "RX Error: Wrong status"
                " 0x%08x vs 0x%08x.\n",
                status, lp->rx.desc_rx[entry].status);
                gkhw_dump(lp);
            }
        }
#endif
        if (likely((status & ETH_RDES0_ES) != ETH_RDES0_ES)) {
            GKETH_napi_rx(lp, status, entry);
        } else {
            GKETH_check_rdes0_status(lp, status);
        }
        rx_budget--;
        lp->rx.cur_rx++;

        if ((lp->rx.cur_rx - lp->rx.dirty_rx) > (lp->rx_count / 4))
            GKETH_rx_rngmng_refill(lp);
    }

GKETH_poll_complete:
    if (rx_budget > 0) {
        GKETH_rx_rngmng_refill(lp);
        spin_lock_irqsave(&lp->lock, flags);
        napi_complete(&lp->napi);
        gk_eth_setbitsl((unsigned int)(lp->regbase + ETH_DMA_INTEN_OFFSET),
            GKETH_RXDMA_INTEN);
        gkhw_dma_rx_start(lp);
        spin_unlock_irqrestore(&lp->lock, flags);
    }

    return (budget - rx_budget);
}

static inline u32 gkhw_hashtable_crc(unsigned char *mac)
{
    unsigned char               tmpbuf[ETH_ALEN];
    int                 i;
    u32                 crc;

    for (i = 0; i < ETH_ALEN; i++)
        tmpbuf[i] = bitrev8(mac[i]);
    crc = crc32_be(~0, tmpbuf, ETH_ALEN);

    return (crc ^ ~0);
}

static inline void gkhw_hashtable_get(struct net_device *ndev, u32 *hat)
{
    struct netdev_hw_addr           *ha;
    unsigned int                bitnr;
#if 0
    unsigned char test1[] = {0x1F,0x52,0x41,0x9C,0xB6,0xAF};
    unsigned char test2[] = {0xA0,0x0A,0x98,0x00,0x00,0x45};
    dev_info(&ndev->dev,
        "Test1: 0x%08X.\n", gkhw_hashtable_crc(test1));
    dev_info(&ndev->dev,
        "Test2: 0x%08X.\n", gkhw_hashtable_crc(test2));
#endif

    hat[0] = hat[1] = 0;
    netdev_for_each_mc_addr(ha, ndev) {
        if (!(ha->addr[0] & 1))
            continue;
        bitnr = gkhw_hashtable_crc(ha->addr);
        bitnr >>= 26;
        bitnr &= 0x3F;
        hat[bitnr >> 5] |= 1 << (bitnr & 31);
    }
}

static int GKETH_set_mac_address(struct net_device *ndev, void *addr)
{
    struct GKETH_info           *lp;
    unsigned long               flags;
    struct sockaddr             *saddr;

    lp = (struct GKETH_info *)netdev_priv(ndev);
    spin_lock_irqsave(&lp->lock, flags);
    saddr = (struct sockaddr *)(addr);

    //if (netif_running(ndev))
    //    goto fail1;

    if (!is_valid_ether_addr(saddr->sa_data))
        goto fail2;

    dev_dbg(&lp->ndev->dev, "MAC address[%pM].\n", saddr->sa_data);

    memcpy(ndev->dev_addr, saddr->sa_data, ndev->addr_len);
    gkhw_set_hwaddr(lp, ndev->dev_addr);
    gkhw_get_hwaddr(lp, ndev->dev_addr);
    memcpy(lp->platform_info->mac_addr, ndev->dev_addr, GKETH_MAC_SIZE);

    spin_unlock_irqrestore(&lp->lock, flags);

    return 0;

fail2:
    spin_unlock_irqrestore(&lp->lock, flags);
    return -EADDRNOTAVAIL;
}

static void GKETH_set_multicast_list(struct net_device *ndev)
{
    struct GKETH_info           *lp;
    unsigned int                mac_filter;
    u32                         hat[2];
    unsigned long               flags;

    lp = (struct GKETH_info *)netdev_priv(ndev);
    spin_lock_irqsave(&lp->lock, flags);

    mac_filter = gk_eth_readl((unsigned int)(lp->regbase +
                    ETH_MAC_FRAME_FILTER_OFFSET));
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
        gkhw_hashtable_get(ndev, hat);
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

    gk_eth_writel((unsigned int)(lp->regbase + ETH_MAC_HASH_HI_OFFSET), hat[1]);
    gk_eth_writel((unsigned int)(lp->regbase + ETH_MAC_HASH_LO_OFFSET), hat[0]);
    gk_eth_writel((unsigned int)(lp->regbase + ETH_MAC_FRAME_FILTER_OFFSET), mac_filter);

    spin_unlock_irqrestore(&lp->lock, flags);
}


#ifdef CONFIG_NET_POLL_CONTROLLER
static void GKETH_poll_controller(struct net_device *ndev)
{
    GKETH_interrupt(ndev->irq, ndev);
}
#endif

static int GKETH_get_settings(struct net_device *ndev,
    struct ethtool_cmd *ecmd)
{
    int                 errorCode = 0;
    struct GKETH_info           *lp;
    unsigned long               flags;

    if (!netif_running(ndev)) {
        errorCode = -EINVAL;
        goto GKETH_get_settings_exit;
    }

    lp = (struct GKETH_info *)netdev_priv(ndev);
    spin_lock_irqsave(&lp->lock, flags);
    if (lp->phydev) {
        errorCode = phy_ethtool_gset(lp->phydev, ecmd);
    } else {
        errorCode = -EINVAL;
    }
    spin_unlock_irqrestore(&lp->lock, flags);

GKETH_get_settings_exit:
    return errorCode;
}

static int GKETH_set_settings(struct net_device *ndev,
    struct ethtool_cmd *ecmd)
{
    int                 errorCode = 0;
    struct GKETH_info           *lp;
    unsigned long               flags;

    if (!netif_running(ndev)) {
        errorCode = -EINVAL;
        goto GKETH_get_settings_exit;
    }

    lp = (struct GKETH_info *)netdev_priv(ndev);
    spin_lock_irqsave(&lp->lock, flags);
    if (lp->phydev) {
        errorCode = phy_ethtool_sset(lp->phydev, ecmd);
    } else {
        errorCode = -EINVAL;
    }
    spin_unlock_irqrestore(&lp->lock, flags);

GKETH_get_settings_exit:
    return errorCode;
}

static int GKETH_ioctl(struct net_device *ndev, struct ifreq *ifr, int cmd)
{
    int                 errorCode = 0;
    struct GKETH_info           *lp;
    unsigned long               flags;

    if (!netif_running(ndev)) {
        errorCode = -EINVAL;
        goto GKETH_get_settings_exit;
    }

    lp = (struct GKETH_info *)netdev_priv(ndev);
    spin_lock_irqsave(&lp->lock, flags);
    if (lp->phydev) {
        errorCode = phy_mii_ioctl(lp->phydev, ifr, cmd);
    } else {
        errorCode = -EINVAL;
    }
    spin_unlock_irqrestore(&lp->lock, flags);

GKETH_get_settings_exit:
    return errorCode;
}

/* ==========================================================================*/
static const struct net_device_ops GKETH_netdev_ops = {
    .ndo_open       = GKETH_open,
    .ndo_stop       = GKETH_stop,
    .ndo_start_xmit     = GKETH_hard_start_xmit,
    .ndo_set_mac_address    = GKETH_set_mac_address,
    .ndo_validate_addr  = eth_validate_addr,
    .ndo_do_ioctl       = GKETH_ioctl,
    .ndo_change_mtu     = eth_change_mtu,
    .ndo_tx_timeout     = GKETH_timeout,
    .ndo_get_stats      = GKETH_get_stats,
    .ndo_set_rx_mode    = GKETH_set_multicast_list,
#ifdef CONFIG_NET_POLL_CONTROLLER
    .ndo_poll_controller    = GKETH_poll_controller,
#endif
};

static const struct ethtool_ops GKETH_ethtool_ops = {
    .get_settings   = GKETH_get_settings,
    .set_settings   = GKETH_set_settings,
    .get_link   = ethtool_op_get_link,
};

static int __devinit GKETH_drv_probe(struct platform_device *pdev)
{
    int                 errorCode = 0;
    struct net_device           *ndev;
    struct GKETH_info           *lp;
    struct gk_eth_platform_info *platform_info;
    struct resource             *reg_res;
    struct resource             *irq_res;

    void __iomem        *eth_base;
    int                 i;
//#ifndef CONFIG_SYSTEM_USE_EXTERN_ETHPHY
    u8 uRegVal;
//#else
    u32 rmii;
//#endif


    platform_info = (struct gk_eth_platform_info *)pdev->dev.platform_data;
    if (platform_info == NULL) {
        dev_err(&pdev->dev, "Can't get platform_data!\n");
        errorCode = - EPERM;
        goto GKETH_drv_probe_exit;
    }

    if (platform_info->is_enabled) {
        if (!platform_info->is_enabled()) {
            dev_err(&pdev->dev, "Not enabled, check HW config!\n");
            errorCode = -EPERM;
            goto GKETH_drv_probe_exit;
        }
    }

    reg_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (reg_res == NULL) {
        dev_err(&pdev->dev, "Get reg_res failed!\n");
        errorCode = -ENXIO;
        goto GKETH_drv_probe_exit;
    }

    irq_res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    if (irq_res == NULL) {
        dev_err(&pdev->dev, "Get irq_res failed!\n");
        errorCode = -ENXIO;
        goto GKETH_drv_probe_exit;
    }

    ndev = alloc_etherdev(sizeof(struct GKETH_info));
    if (ndev == NULL) {
        dev_err(&pdev->dev, "alloc_etherdev fail.\n");
        errorCode = -ENOMEM;
        goto GKETH_drv_probe_exit;
    }
#if 0
    /* get the memory region for the watchdog timer */
    size = resource_size(reg_res);
    if (!request_mem_region(reg_res->start, size, pdev->name)) {
        dev_err(&pdev->dev, "Failed to get eth's memory region\n");
        errorCode = -EBUSY;
        goto GKETH_drv_probe_free_netdev;
    }

    eth_base = ioremap(reg_res->start, size);
    if (eth_base == NULL) {
        dev_err(&pdev->dev, "failed to ioremap() region\n");
        errorCode = -EINVAL;
        goto GKTH_drv_probe_free_res;
    }
#else
	eth_base = (void __iomem *)reg_res->start;
#endif

#define KE_DEBUG
#ifdef KE_DEBUG
    printk("[%s] eth_base = 0x%08x\n", __FUNCTION__, (u32)eth_base);
#endif

    SET_NETDEV_DEV(ndev, &pdev->dev);
    ndev->dev.dma_mask = pdev->dev.dma_mask;
    ndev->dev.coherent_dma_mask = pdev->dev.coherent_dma_mask;
    ndev->irq = irq_res->start;
#if defined(CONFIG_GK_ETH_SUPPORT_IPC)
    ndev->features = NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM;
#endif
    lp = netdev_priv(ndev);
    spin_lock_init(&lp->lock);
    lp->ndev = ndev;
    lp->regbase = (unsigned char __iomem *)eth_base;
    lp->platform_info = platform_info;
    lp->rx_count = lp->platform_info->default_rx_ring_size;
    if (lp->rx_count < GKETH_RX_RNG_MIN)
        lp->rx_count = GKETH_RX_RNG_MIN;
    lp->tx_count = lp->platform_info->default_tx_ring_size;
    if (lp->tx_count < GKETH_TX_RNG_MIN)
        lp->tx_count = GKETH_TX_RNG_MIN;
    lp->tx_irq_count = (lp->tx_count * 2) / 3;
    lp->msg_enable = netif_msg_init(msg_level, NETIF_MSG_DRV | NETIF_MSG_LINK);

    lp->new_bus.name = "goke MII Bus",
    lp->new_bus.read = &gkhw_mdio_read;
    lp->new_bus.write = &gkhw_mdio_write;
    lp->new_bus.reset = &gkhw_mdio_reset;
    //pdev->id = 0x3;
    //lp->platform_info->mii_id = pdev->id;
    printk("mii id = %d \n", pdev->id);
    snprintf(lp->new_bus.id, MII_BUS_ID_SIZE, "%x", pdev->id);
    lp->new_bus.priv = lp;
    lp->new_bus.irq = kmalloc(sizeof(int)*PHY_MAX_ADDR, GFP_KERNEL);
    if (lp->new_bus.irq == NULL) {
        dev_err(&pdev->dev, "alloc new_bus.irq fail.\n");
        errorCode = -ENOMEM;
        goto GKETH_drv_probe_free_mii_gpio_irq;
    }
    for(i = 0; i < PHY_MAX_ADDR; ++i)
        lp->new_bus.irq[i] = PHY_POLL;
    lp->new_bus.parent = &pdev->dev;
    lp->new_bus.state = MDIOBUS_ALLOCATED;
//#ifndef CONFIG_SYSTEM_USE_EXTERN_ETHPHY
    if(cmdline_phytype == 0){
        lp->new_bus.phy_mask = 0xFFFFFFFE;
    }
//#endif

    //printk("mdiobus register ...\n");
    errorCode = mdiobus_register(&lp->new_bus);
    if (errorCode) {
        dev_err(&pdev->dev, "mdiobus_register fail%d.\n", errorCode);
        goto GKETH_drv_probe_kfree_mdiobus;
    }
    //printk("ok\n");
//#ifdef CONFIG_SYSTEM_USE_EXTERN_ETHPHY
    if(cmdline_phytype != 0)
    {
#if 0//ephy rmii mode
        GH_EPHY_set_MII_RMII_rmii(0x1);
        // RMII mode GPIO_TYPE_OUTPUT_ENET_PHY_TXD_3 used as 10/100M indicator
        gk_set_phy_speed_led(GPIO_TYPE_INPUT_0);

        if(lp->new_bus.phy_used->phy_id == 0x02430c54) // IP101g
        {
            // LindengYu: I don't known why do not go into effect.
            // change to page 3
            gkhw_mdio_write(&lp->new_bus, lp->new_bus.phy_used->addr, 20, 0x03);
            rmii = gkhw_mdio_read(&lp->new_bus, lp->new_bus.phy_used->addr, 0x10);
            rmii &= 0x3FFF;
            rmii |= 0x4000;
            // change to LED0 mode1, Link/Act mode
            gkhw_mdio_write(&lp->new_bus, lp->new_bus.phy_used->addr, 0x10, rmii);
            // restore to page 16
            gkhw_mdio_write(&lp->new_bus, lp->new_bus.phy_used->addr, 20, 0x10);
        }
        else// if(lp->new_bus.phy_used->phy_id == 0x001CC815) // rtl820x
        {
            // change to rmii mode
            rmii = gkhw_mdio_read(&lp->new_bus, lp->new_bus.phy_used->addr, 25);
            rmii |= (1<<10);
            rmii &= ~(1<<11);
            gkhw_mdio_write(&lp->new_bus, lp->new_bus.phy_used->addr, 25, rmii);
        }
#else  //ephy mii mode
        GH_EPHY_set_MII_RMII_rmii(0x0);
        // RMII mode GPIO_TYPE_OUTPUT_ENET_PHY_TXD_3 used as 10/100M indicator
#if 0
        gk_set_phy_speed_led(GPIO_TYPE_INPUT_0);
#endif
//        printk("cmdline_phytype0:%d \n",cmdline_phytype);
        if(lp->new_bus.phy_used->phy_id == 0x02430c54) // IP101g
        {
            // LindengYu: I don't known why do not go into effect.
            // change to page 3
            gkhw_mdio_write(&lp->new_bus, lp->new_bus.phy_used->addr, 20, 0x03);
            rmii = gkhw_mdio_read(&lp->new_bus, lp->new_bus.phy_used->addr, 0x10);
            rmii &= 0x3FFF;
            rmii |= 0x4000;
            // change to LED0 mode1, Link/Act mode
            gkhw_mdio_write(&lp->new_bus, lp->new_bus.phy_used->addr, 0x10, rmii);
            // restore to page 16
            gkhw_mdio_write(&lp->new_bus, lp->new_bus.phy_used->addr, 20, 0x10);
        }
        else// if(lp->new_bus.phy_used->phy_id == 0x001CC815) // rtl820x
        {
            // change to rmii mode
//            printk("read 25 reg nuw bus 0x%x\n",&(lp->new_bus));
            rmii = gkhw_mdio_read(&(lp->new_bus), lp->new_bus.phy_used->addr, 25);
//        printk("cmdline_phytype1:%d\n",cmdline_phytype);
            rmii &= ~(1<<10);
            rmii &= ~(1<<11);
            gkhw_mdio_write(&lp->new_bus, lp->new_bus.phy_used->addr, 25, rmii);
        }

#endif
#if 0
        {
            u16 reglist[24][2]=
            {
                {0x10, 0x00},{0x10, 0x01},{0x10, 0x02},{0x10, 0x03},
                {0x10, 0x04},{0x10, 0x05},{0x10, 0x06},{0x10, 0x07},
                {0x10, 0x08},{0x10, 0x10},{0x10, 0x11},{0x10, 0x12},
                {0x10, 0x14},{0x01, 0x11},{0x01, 0x12},{0x01, 0x17},
                {0x02, 0x12},{0x08, 0x11},{0x0B, 0x12},{0x12, 0x11},
                {0x03, 0x10},{0x04, 0x10},{0x05, 0x10},{0x11, 0x11},
            };
            // page 16
            u16 regdata;
            u16 regpage=0x00;
            u32 i;
            printk("Page   Addr    Data\n");
            for(i=0;i<24;i++)
            {
                if(regpage != reglist[i][0])
                {
                    regpage = reglist[i][0];
                    gkhw_mdio_write(&lp->new_bus, lp->new_bus.phy_used->addr, 0x14, regpage);
                }
                regdata = gkhw_mdio_read(&lp->new_bus, lp->new_bus.phy_used->addr, reglist[i][1]);
                printk("%02d   %02d    0x%04x\n",reglist[i][0] ,reglist[i][1] ,regdata);
            }
        }
#endif
		//EPHY POWER OFF
		//printk("net probe ephy power off\n");
		GK_EPHY_POWER_OFF();
    }
    else
    {
//#else
        GH_EPHY_set_MII_RMII_rmii(0x0);
	#if 0
        gk_eth_writel(0xF0022540, 0x00000000);
        gk_eth_writel(0xF00220C8, 0x0000C400);    // reg_test_out(debug_bus_out)
        gk_eth_writel(0xF00220E0, 0x0000810A);    // debug mode
        gk_eth_writel(0xF0022588, 0x00000007);    // DAC 100M clk gate for 10M TX
    #else
		gk_eth_writel(GK_VA_ETH_PHY + 0x0540, 0x00000000);
        gk_eth_writel(GK_VA_ETH_PHY + 0x00C8, 0x0000C400);    // reg_test_out(debug_bus_out)
        gk_eth_writel(GK_VA_ETH_PHY + 0x00E0, 0x0000810A);    // debug mode
        gk_eth_writel(GK_VA_ETH_PHY + 0x0588, 0x00000007);    // DAC 100M clk gate for 10M TX
	#endif
        MHal_EMAC_WritReg8(0x0033, 0xde, 0x59);
        MHal_EMAC_WritReg8(0x0033, 0xf4, 0x21);
        MHal_EMAC_WritReg8(0x0032, 0x72, 0x80);
        MHal_EMAC_WritReg8(0x0033, 0xfc, 0x00);
        MHal_EMAC_WritReg8(0x0033, 0xfd, 0x00);
        MHal_EMAC_WritReg8(0x0033, 0xb7, 0x07);
        MHal_EMAC_WritReg8(0x0033, 0xcb, 0x11);
        MHal_EMAC_WritReg8(0x0033, 0xcc, 0x80);
        MHal_EMAC_WritReg8(0x0033, 0xcd, 0xd1);
        MHal_EMAC_WritReg8(0x0033, 0xd4, 0x00);
        MHal_EMAC_WritReg8(0x0033, 0xb9, 0x40);
        MHal_EMAC_WritReg8(0x0033, 0xbb, 0x05);
        MHal_EMAC_WritReg8(0x0033, 0xea, 0x46);
        MHal_EMAC_WritReg8(0x0033, 0xa1, 0x00);
        MHal_EMAC_WritReg8(0x0034, 0x3a, 0x03);
        MHal_EMAC_WritReg8(0x0034, 0x3b, 0x00);

        //gain shift
        MHal_EMAC_WritReg8(0x0033, 0xb4, 0x56);

        //det max
        MHal_EMAC_WritReg8(0x0033, 0x4f, 0x02);

        //det min
        MHal_EMAC_WritReg8(0x0033, 0x51, 0x01);

        //snr len (emc noise)
        MHal_EMAC_WritReg8(0x0033, 0x77, 0x18);

        //snr k value
        MHal_EMAC_WritReg8(0x0033, 0x43, 0x15);

        //100 gat
        MHal_EMAC_WritReg8(0x0034, 0xc5, 0x00);

        //200 gat
        MHal_EMAC_WritReg8(0x0034, 0x30, 0x43);

        //en_100t_phase
        MHal_EMAC_WritReg8(0x0034, 0x39, 0x41);

        //10T waveform
        uRegVal = MHal_EMAC_ReadReg8(0x0034, 0xe8);
        uRegVal &= 0xf8;
        uRegVal |= 0x06;
        MHal_EMAC_WritReg8(0x0034, 0xe8, uRegVal);
        MHal_EMAC_WritReg8(0x0032, 0x2b, 0x00);

        //analog
        MHal_EMAC_WritReg8(0x0033, 0xd8, 0xb0);
        MHal_EMAC_WritReg8(0x0033, 0xd9, 0x30);

        //disable EEE
        uRegVal = MHal_EMAC_ReadReg8(0x0032, 0x2d);
        uRegVal |= 0x40;
        MHal_EMAC_WritReg8(0x0032, 0x2d, uRegVal);

        GH_EPHY_set_SPEED_ane(0x01);
    }
//#endif

    ether_setup(ndev);
    ndev->netdev_ops = &GKETH_netdev_ops;
    ndev->watchdog_timeo = lp->platform_info->watchdog_timeo;
    netif_napi_add(ndev, &lp->napi, GKETH_napi,
        lp->platform_info->napi_weight);
    if (!is_valid_ether_addr(lp->platform_info->mac_addr))
        random_ether_addr(lp->platform_info->mac_addr);
    memcpy(ndev->dev_addr, lp->platform_info->mac_addr, GKETH_MAC_SIZE);

    SET_ETHTOOL_OPS(ndev, &GKETH_ethtool_ops);
    errorCode = register_netdev(ndev);
    if (errorCode) {
        dev_err(&pdev->dev, " register_netdev fail%d.\n", errorCode);
        goto GKETH_drv_probe_netif_napi_del;
    }

    platform_set_drvdata(pdev, ndev);
    dev_notice(&pdev->dev, "MAC Address[%pM].\n", ndev->dev_addr);

    goto GKETH_drv_probe_exit;

GKETH_drv_probe_netif_napi_del:
    netif_napi_del(&lp->napi);
    mdiobus_unregister(&lp->new_bus);

GKETH_drv_probe_kfree_mdiobus:
    kfree(lp->new_bus.irq);

GKETH_drv_probe_free_mii_gpio_irq:
    //iounmap(eth_base);

//GKTH_drv_probe_free_res:
    //release_mem_region(reg_res->start, size);


//GKETH_drv_probe_free_netdev:
    free_netdev(ndev);

GKETH_drv_probe_exit:
    return errorCode;
}

static int __devexit GKETH_drv_remove(struct platform_device *pdev)
{
    struct net_device           *ndev;
    struct GKETH_info           *lp;

    ndev = platform_get_drvdata(pdev);
    if (ndev) {
        lp = (struct GKETH_info *)netdev_priv(ndev);
        unregister_netdev(ndev);
        netif_napi_del(&lp->napi);
        mdiobus_unregister(&lp->new_bus);
        kfree(lp->new_bus.irq);
        platform_set_drvdata(pdev, NULL);
        free_netdev(ndev);
        dev_notice(&pdev->dev, "Removed.\n");
    }

    return 0;
}

#ifdef CONFIG_PM
static int GKETH_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
    int                 errorCode = 0;
    struct net_device           *ndev;
    struct GKETH_info           *lp;
    unsigned long               flags;

    ndev = platform_get_drvdata(pdev);
    if (ndev) {
        if (!netif_running(ndev))
            goto GKETH_drv_suspend_exit;

        lp = (struct GKETH_info *)netdev_priv(ndev);

        napi_disable(&lp->napi);
        netif_device_detach(ndev);
        disable_irq(ndev->irq);

        spin_lock_irqsave(&lp->lock, flags);
        gkhw_disable(lp);
        spin_unlock_irqrestore(&lp->lock, flags);
    }

GKETH_drv_suspend_exit:
    dev_dbg(&pdev->dev, "%s exit with %d @ %d\n",
        __func__, errorCode, state.event);
    return errorCode;
}

static int GKETH_drv_resume(struct platform_device *pdev)
{
    int                 errorCode = 0;
    struct net_device           *ndev;
    struct GKETH_info           *lp;
    unsigned long               flags;

    ndev = platform_get_drvdata(pdev);
    if (ndev) {
        if (!netif_running(ndev))
            goto GKETH_drv_resume_exit;

        lp = (struct GKETH_info *)netdev_priv(ndev);

        spin_lock_irqsave(&lp->lock, flags);
        errorCode = gkhw_enable(lp);
        gkhw_set_link_mode_speed(lp);
        GKETH_rx_rngmng_init(lp);
        GKETH_tx_rngmng_init(lp);
        gkhw_set_dma_desc(lp);
        gkhw_dma_rx_start(lp);
        gkhw_dma_tx_start(lp);
        gkhw_dma_int_enable(lp);
        spin_unlock_irqrestore(&lp->lock, flags);

        if (errorCode) {
            dev_err(&pdev->dev, "gkhw_enable.\n");
        } else {
            enable_irq(ndev->irq);
            netif_device_attach(ndev);
            napi_enable(&lp->napi);
        }
    }

GKETH_drv_resume_exit:
    dev_dbg(&pdev->dev, "%s exit with %d\n", __func__, errorCode);
    return errorCode;
}
#endif

static struct platform_driver GKETH_driver = {
    .probe      = GKETH_drv_probe,
    .remove     = __devexit_p(GKETH_drv_remove),
#ifdef CONFIG_PM
    .suspend        = GKETH_drv_suspend,
    .resume     = GKETH_drv_resume,
#endif
    .driver = {
        .name   = "gk-eth",
        .owner  = THIS_MODULE,
    },
};

static int __init GKETH_init(void)
{
    printk("%s\n", __FUNCTION__);
    return platform_driver_register(&GKETH_driver);
}

static void __exit GKETH_exit(void)
{
    platform_driver_unregister(&GKETH_driver);
}

module_init(GKETH_init);
module_exit(GKETH_exit);

MODULE_DESCRIPTION("Goke GK Ethernet Driver");
MODULE_AUTHOR("Goke Microelectronics Inc.");
MODULE_LICENSE("GPL");

