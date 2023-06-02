#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/mii.h>
#include <linux/phy.h>
#include <mach/fh_gmac.h>
#include "fh_gmac.h"

#define REG_SPACE_SIZE	0x1054
#define GMAC_ETHTOOL_NAME	"fh_gmac"

struct gmac_stats
{
	char stat_string[ETH_GSTRING_LEN];
	int sizeof_stat;
	int stat_offset;
};

#define FH_GMAC_STAT(m)	\
	{ #m, FIELD_SIZEOF(Gmac_Stats, m),	\
	offsetof(Gmac_Object, stats.m)}

static const struct gmac_stats gmac_gstrings_stats[] =
{
	FH_GMAC_STAT(tx_underflow),
	FH_GMAC_STAT(tx_carrier),
	FH_GMAC_STAT(tx_losscarrier),
	FH_GMAC_STAT(tx_heartbeat),
	FH_GMAC_STAT(tx_deferred),
	FH_GMAC_STAT(tx_vlan),
	FH_GMAC_STAT(tx_jabber),
	FH_GMAC_STAT(tx_frame_flushed),
	FH_GMAC_STAT(tx_payload_error),
	FH_GMAC_STAT(tx_ip_header_error),
	FH_GMAC_STAT(rx_desc),
	FH_GMAC_STAT(rx_partial),
	FH_GMAC_STAT(rx_runt),
	FH_GMAC_STAT(rx_toolong),
	FH_GMAC_STAT(rx_collision),
	FH_GMAC_STAT(rx_crc),
	FH_GMAC_STAT(rx_length),
	FH_GMAC_STAT(rx_mii),
	FH_GMAC_STAT(rx_multicast),
	FH_GMAC_STAT(rx_gmac_overflow),
	FH_GMAC_STAT(rx_watchdog),
	FH_GMAC_STAT(da_rx_filter_fail),
	FH_GMAC_STAT(sa_rx_filter_fail),
	FH_GMAC_STAT(rx_missed_cntr),
	FH_GMAC_STAT(rx_overflow_cntr),
	FH_GMAC_STAT(tx_undeflow_irq),
	FH_GMAC_STAT(tx_process_stopped_irq),
	FH_GMAC_STAT(tx_jabber_irq),
	FH_GMAC_STAT(rx_overflow_irq),
	FH_GMAC_STAT(rx_buf_unav_irq),
	FH_GMAC_STAT(rx_process_stopped_irq),
	FH_GMAC_STAT(rx_watchdog_irq),
	FH_GMAC_STAT(tx_early_irq),
	FH_GMAC_STAT(fatal_bus_error_irq),
	FH_GMAC_STAT(threshold),
	FH_GMAC_STAT(tx_pkt_n),
	FH_GMAC_STAT(rx_pkt_n),
	FH_GMAC_STAT(poll_n),
	FH_GMAC_STAT(sched_timer_n),
	FH_GMAC_STAT(normal_irq_n),
};
#define FH_GMAC_STATS_LEN ARRAY_SIZE(gmac_gstrings_stats)

static void gmac_ethtool_getdrvinfo(struct net_device *ndev, struct ethtool_drvinfo *info)
{
	strcpy(info->driver, GMAC_ETHTOOL_NAME);

	strcpy(info->version, "0.0.1");
	info->fw_version[0] = '\0';
	info->n_stats = FH_GMAC_STATS_LEN;
}

static int gmac_ethtool_getsettings(struct net_device *ndev, struct ethtool_cmd *cmd)
{
	Gmac_Object* pGmac = netdev_priv(ndev);
	struct phy_device *phy = pGmac->phydev;
	int rc;
	if (phy == NULL)
	{
		pr_err("%s: %s: PHY is not registered\n",
		       __func__, ndev->name);
		return -ENODEV;
	}
	if (!netif_running(ndev))
	{
		pr_err("%s: interface is disabled: we cannot track "
		"link speed / duplex setting\n", ndev->name);
		return -EBUSY;
	}
	cmd->transceiver = XCVR_INTERNAL;
	spin_lock_irq(&pGmac->lock);
	rc = phy_ethtool_gset(phy, cmd);
	spin_unlock_irq(&pGmac->lock);
	return rc;
}

static int gmac_ethtool_setsettings(struct net_device *ndev, struct ethtool_cmd *cmd)
{
	Gmac_Object* pGmac = netdev_priv(ndev);
	struct phy_device *phy = pGmac->phydev;
	int rc;

	spin_lock(&pGmac->lock);
	rc = phy_ethtool_sset(phy, cmd);
	spin_unlock(&pGmac->lock);

	return rc;
}

static __u32 gmac_ethtool_getmsglevel(struct net_device *ndev)
{
	Gmac_Object* pGmac = netdev_priv(ndev);
	return pGmac->msg_enable;
}

static void gmac_ethtool_setmsglevel(struct net_device *ndev, __u32 level)
{
	Gmac_Object* pGmac = netdev_priv(ndev);
	pGmac->msg_enable = level;

}

static int gmac_check_if_running(struct net_device *ndev)
{
	if (!netif_running(ndev))
		return -EBUSY;
	return 0;
}

static int gmac_ethtool_get_regs_len(struct net_device *ndev)
{
	return REG_SPACE_SIZE;
}

static void gmac_ethtool_gregs(struct net_device *ndev, struct ethtool_regs *regs, void *space)
{
	int i;
	__u32 *reg_space = (__u32 *) space;

	Gmac_Object* pGmac = netdev_priv(ndev);

	memset(reg_space, 0x0, REG_SPACE_SIZE);

	/* MAC registers */
	for (i = 0; i < 55; i++)
		reg_space[i] = readl(pGmac->remap_addr + (i * 4));
	/* DMA registers */
	for (i = 0; i < 22; i++)
		reg_space[i + 55] = readl(pGmac->remap_addr + (REG_GMAC_BUS_MODE + (i * 4)));
}

static void gmac_get_pauseparam(struct net_device *ndev, struct ethtool_pauseparam *pause)
{
	Gmac_Object* pGmac = netdev_priv(ndev);

	spin_lock(&pGmac->lock);

	pause->rx_pause = 0;
	pause->tx_pause = 0;
	pause->autoneg = pGmac->phydev->autoneg;

	if (pGmac->flow_ctrl & FLOW_RX)
		pause->rx_pause = 1;
	if (pGmac->flow_ctrl & FLOW_TX)
		pause->tx_pause = 1;

	spin_unlock(&pGmac->lock);
}

static int gmac_set_pauseparam(struct net_device *ndev, struct ethtool_pauseparam *pause)
{
	Gmac_Object* pGmac = netdev_priv(ndev);
	struct phy_device *phy = pGmac->phydev;
	int new_pause = FLOW_OFF;
	int ret = 0;

	spin_lock(&pGmac->lock);

	if (pause->rx_pause)
		new_pause |= FLOW_RX;
	if (pause->tx_pause)
		new_pause |= FLOW_TX;

	pGmac->flow_ctrl = new_pause;
	phy->autoneg = pause->autoneg;

	if (phy->autoneg)
	{
		if (netif_running(ndev))
			ret = phy_start_aneg(phy);
	}
	else
	{
		GMAC_FlowCtrl(pGmac, phy->duplex, pGmac->flow_ctrl, pGmac->pause);
	}
	spin_unlock(&pGmac->lock);
	return ret;
}

static void gmac_get_ethtool_stats(struct net_device *ndev, struct ethtool_stats *dummy, __u64 *data)
{
	Gmac_Object* pGmac = netdev_priv(ndev);
	int i;

	/* Update HW stats if supported */
	GMAC_DMA_DiagnosticFrame(&ndev->stats, pGmac);

	for (i = 0; i < FH_GMAC_STATS_LEN; i++)
	{
		char *p = (char *)pGmac + gmac_gstrings_stats[i].stat_offset;
		data[i] = (gmac_gstrings_stats[i].sizeof_stat ==
		sizeof(__u64)) ? (*(__u64 *)p) : (*(__u32 *)p);
	}
}

static int gmac_get_sset_count(struct net_device *netdev, int sset)
{
	switch (sset)
	{
	case ETH_SS_STATS:
		return FH_GMAC_STATS_LEN;
	default:
		return -EOPNOTSUPP;
	}
}

static void gmac_get_strings(struct net_device *ndev, __u32 stringset, __u8 *data)
{
	int i;
	__u8 *p = data;

	switch (stringset)
	{
	case ETH_SS_STATS:
		for (i = 0; i < FH_GMAC_STATS_LEN; i++)
		{
			memcpy(p, gmac_gstrings_stats[i].stat_string, ETH_GSTRING_LEN);
			p += ETH_GSTRING_LEN;
		}
		break;
	default:
		WARN_ON(1);
		break;
	}
}

/* Currently only support WOL through Magic packet. */
static void gmac_get_wol(struct net_device *ndev, struct ethtool_wolinfo *wol)
{
	Gmac_Object* pGmac = netdev_priv(ndev);

	spin_lock_irq(&pGmac->lock);
	if (device_can_wakeup(pGmac->dev))
	{
		wol->supported = WAKE_MAGIC | WAKE_UCAST;
		wol->wolopts = pGmac->wolopts;
	}
	spin_unlock_irq(&pGmac->lock);
}

static int gmac_set_wol(struct net_device *ndev, struct ethtool_wolinfo *wol)
{
	Gmac_Object* pGmac = netdev_priv(ndev);
	__u32 support = WAKE_MAGIC | WAKE_UCAST;

	if (!device_can_wakeup(pGmac->dev))
		return -EINVAL;

	if (wol->wolopts & ~support)
		return -EINVAL;

	if (wol->wolopts)
	{
		pr_info("fh_gmac: wakeup enable\n");
		device_set_wakeup_enable(pGmac->dev, 1);
		enable_irq_wake(ndev->irq);
	}
	else
	{
		device_set_wakeup_enable(pGmac->dev, 0);
		disable_irq_wake(ndev->irq);
	}

	spin_lock_irq(&pGmac->lock);
	pGmac->wolopts = wol->wolopts;
	spin_unlock_irq(&pGmac->lock);

	return 0;
}

static struct ethtool_ops fh_gmac_ethtool_ops = {
	.begin = gmac_check_if_running,
	.get_drvinfo = gmac_ethtool_getdrvinfo,
	.get_settings = gmac_ethtool_getsettings,
	.set_settings = gmac_ethtool_setsettings,
	.get_msglevel = gmac_ethtool_getmsglevel,
	.set_msglevel = gmac_ethtool_setmsglevel,
	.get_regs = gmac_ethtool_gregs,
	.get_regs_len = gmac_ethtool_get_regs_len,
	.get_link = ethtool_op_get_link,
	.get_pauseparam = gmac_get_pauseparam,
	.set_pauseparam = gmac_set_pauseparam,
	.get_ethtool_stats = gmac_get_ethtool_stats,
	.get_strings = gmac_get_strings,
	.get_wol = gmac_get_wol,
	.set_wol = gmac_set_wol,
	.get_sset_count	= gmac_get_sset_count,
};

void fh_gmac_set_ethtool_ops(struct net_device *netdev)
{
	SET_ETHTOOL_OPS(netdev, &fh_gmac_ethtool_ops);
}
