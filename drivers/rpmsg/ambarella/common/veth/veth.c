/*
 * Author: Tzu-Jung Lee <tjlee@ambarella.com>
 * Copyright (C) 2012-2013, Ambarella, Inc.
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
#include <linux/err.h>

#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/time.h>
#include <linux/crc16.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/rpmsg.h>

#include <plat/ambcache.h>

#define MIN_MTU 68
#define MAX_MTU (4096 - ETH_HLEN - sizeof(struct rpmsg_hdr))
#define DEF_MTU MAX_MTU

extern int ambveth_do_send(void *data, int len);

struct ambveth
{
	spinlock_t		lock;

	struct net_device_stats	stats;
	struct net_device	*ndev;
};

void ambveth_enqueue(void *priv, void *data, int len)
{
	struct ambveth		*lp;
	struct sk_buff		*skb;

	lp = ((struct platform_device*)priv)->dev.platform_data;

	skb = dev_alloc_skb(len + NET_IP_ALIGN);
	if (!skb) {
		lp->stats.rx_dropped++;
		return;
	}
	skb_put(skb, len);

	memcpy(skb->data, data, len);

	lp->stats.rx_packets++;
	lp->stats.rx_bytes += len;

	skb->dev = lp->ndev;
	skb->protocol = eth_type_trans(skb, skb->dev);

	netif_rx_ni(skb);
}

static int ambveth_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct ambveth		*lp;
	int			ret;

	lp = netdev_priv(ndev);

	ret = ambveth_do_send(skb->data, skb->len);
	if (ret) {
	    lp->stats.tx_dropped++;

	    return NETDEV_TX_BUSY;
	}

	lp->stats.tx_packets++;
	lp->stats.tx_bytes += skb->len;

	dev_kfree_skb(skb);

	return NETDEV_TX_OK;
}

static int ambveth_open(struct net_device *ndev)
{
	struct ambveth	*lp;

	lp = netdev_priv(ndev);

	netif_start_queue(ndev);

	return 0;
}

static int ambveth_stop(struct net_device *ndev)
{
	struct ambveth	*lp;

	lp = netdev_priv(ndev);

	netif_stop_queue(ndev);
	flush_scheduled_work();

	return 0;
}

static void ambveth_timeout(struct net_device *ndev)
{
	netif_wake_queue(ndev);
}

static struct net_device_stats *ambveth_get_stats(struct net_device *ndev)
{
	struct ambveth *lp;

	lp = netdev_priv(ndev);

	return &lp->stats;
}

static int ambveth_change_mtu(struct net_device *dev, int mtu)
{
	if (mtu < MIN_MTU || mtu + dev->hard_header_len > MAX_MTU)

		return -EINVAL;

	dev->mtu = mtu;

	return 0;
}

static int ambveth_ioctl(struct net_device *ndev, struct ifreq *ifr, int cmd)
{
	int		ret = 0;

	if (!netif_running(ndev)) {
		ret = -EINVAL;
		goto ambveth_get_settings_exit;
	}

ambveth_get_settings_exit:
	return ret;
}

/* ==========================================================================*/
static const struct net_device_ops ambveth_netdev_ops = {
	.ndo_open		= ambveth_open,
	.ndo_stop		= ambveth_stop,
	.ndo_start_xmit		= ambveth_start_xmit,
	.ndo_set_mac_address 	= eth_mac_addr,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_do_ioctl		= ambveth_ioctl,
	.ndo_tx_timeout		= ambveth_timeout,
	.ndo_get_stats		= ambveth_get_stats,
	.ndo_change_mtu		= ambveth_change_mtu,
};

struct platform_device ambveth_device = {
	.name		= "ambveth",
	.id		= 0,
	.resource	= NULL,
	.num_resources	= 0,
	.dev		= {
		.platform_data		= NULL,
	}
};

static int ambveth_drv_probe(struct platform_device *pdev)
{
	int			ret = 0;
	struct net_device	*ndev;
	struct ambveth		*lp;
	char			mac_addr[6];

	ndev = alloc_etherdev(sizeof(struct ambveth));
	if (!ndev)
		ret = -ENOMEM;
	SET_NETDEV_DEV(ndev, &pdev->dev);

	lp = netdev_priv(ndev);
	lp->ndev = ndev;
	spin_lock_init(&lp->lock);
	pdev->dev.platform_data = lp;

	ndev->netdev_ops = &ambveth_netdev_ops;
	ndev->mtu = DEF_MTU;
	sprintf(ndev->name, "veth0");

	if (!is_valid_ether_addr(mac_addr))
		random_ether_addr(mac_addr);

	memcpy(ndev->dev_addr, mac_addr, 6);

	ret = register_netdev(ndev);
	if (ret)
		dev_err(&pdev->dev, " register_netdev fail%d.\n", ret);

	return ret;
}

static struct platform_driver ambveth_driver = {
	.probe		= ambveth_drv_probe,
	.driver = {
		.name	= "ambveth",
		.owner	= THIS_MODULE,
	},
};

static int __init ambveth_init(void)
{
	return platform_driver_register(&ambveth_driver);
}

static void __exit ambveth_fini(void)
{
}

module_init(ambveth_init);
module_exit(ambveth_fini);

MODULE_DESCRIPTION("Ambarella veth driver");
