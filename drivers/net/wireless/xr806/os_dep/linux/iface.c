/*
 * xr806/iface.c
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * wlan interface implementation for Xr806 drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/gpio.h>
#include <linux/errno.h>
#include "iface.h"

static int xradio_iface_open(struct net_device *ndev);
static int xradio_iface_stop(struct net_device *ndev);
static struct net_device_stats *xradio_iface_get_stats(struct net_device *ndev);
static int xradio_iface_set_mac_address(struct net_device *ndev, void *data);
static int xradio_iface_hard_start_xmit(struct sk_buff *skb, struct net_device *ndev);

static const struct net_device_ops xradio_netdev_ops = {
	.ndo_open = xradio_iface_open,
	.ndo_stop = xradio_iface_stop,
	.ndo_start_xmit = xradio_iface_hard_start_xmit,
	.ndo_set_mac_address = xradio_iface_set_mac_address,
	.ndo_get_stats = xradio_iface_get_stats,
};

static int xradio_iface_open(struct net_device *ndev)
{
	netif_start_queue(ndev);
	return 0;
}

static int xradio_iface_stop(struct net_device *ndev)
{
	netif_stop_queue(ndev);
	return 0;
}

static struct net_device_stats *xradio_iface_get_stats(struct net_device *ndev)
{
	struct xradio_net_priv *priv = netdev_priv(ndev);

	return &priv->stats;
}

static int xradio_iface_set_mac_address(struct net_device *ndev, void *data)
{
	struct xradio_net_priv *priv = netdev_priv(ndev);
	struct sockaddr *mac_addr = data;

	if (!priv)
		return -EINVAL;

	ether_addr_copy(priv->mac_address, mac_addr->sa_data);
	ether_addr_copy(ndev->dev_addr, mac_addr->sa_data);
	return 0;
}

static int xradio_iface_hard_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct xradio_net_priv *priv = netdev_priv(ndev);
	int ret;

	if (!priv) {
		dev_kfree_skb(skb);
		return NETDEV_TX_OK;
	}

	if (netif_queue_stopped((const struct net_device *)priv->ndev))
		return NETDEV_TX_BUSY;

	if (!skb->len || (skb->len > ETH_FRAME_LEN)) {
		priv->stats.tx_dropped++;
		dev_kfree_skb(skb);
		return NETDEV_TX_OK;
	}
	ret = priv->tx_func(priv->xradio_priv, skb);
	if (ret) {
		priv->stats.tx_errors++;
	} else {
		priv->stats.tx_packets++;
		priv->stats.tx_bytes += skb->len;
	}
	return ret;
}

void *xradio_iface_add_net_dev(void *priv, u8 if_type, u8 if_id, char *name,
			       xr_net_data_tx_t tx_func)
{
	struct net_device *ndev = NULL;
	struct xradio_net_priv *net_priv = NULL;
	int ret = 0;

	ndev = alloc_netdev_mqs(sizeof(struct xradio_net_priv), name, NET_NAME_ENUM, ether_setup, 1,
				1);
	if (!ndev)
		return NULL;

	net_priv = netdev_priv(ndev);

	net_priv->ndev = ndev;
	net_priv->if_type = if_type;
	net_priv->if_id = if_id;

	memset(&net_priv->stats, 0, sizeof(net_priv->stats));

	net_priv->xradio_priv = priv;

	net_priv->tx_func = tx_func;

	ndev->netdev_ops = &xradio_netdev_ops;

	ether_addr_copy(ndev->dev_addr, net_priv->mac_address);

	ret = register_netdev(ndev);
	if (ret) {
		free_netdev(ndev);
		return NULL;
	}
	return (void *)net_priv;
}

void xradio_iface_remove_net_dev(void *priv)
{
	struct xradio_net_priv *net_priv = (struct xradio_net_priv *)priv;

	if (net_priv->ndev) {
		netif_stop_queue(net_priv->ndev);
		unregister_netdev(net_priv->ndev);
		free_netdev(net_priv->ndev);
	}
}

void xradio_iface_net_tx_pause(void *priv)
{
	struct xradio_net_priv *net_priv = (struct xradio_net_priv *)priv;

	if (net_priv->ndev && !netif_queue_stopped(net_priv->ndev))
		netif_stop_queue(net_priv->ndev);
}

void xradio_iface_net_tx_resume(void *priv)
{
	struct xradio_net_priv *net_priv = (struct xradio_net_priv *)priv;

	if (net_priv->ndev && netif_queue_stopped(net_priv->ndev))
		netif_wake_queue(net_priv->ndev);
}

int xradio_iface_net_input(void *priv, struct sk_buff *skb)
{
	struct xradio_net_priv *net_priv = (struct xradio_net_priv *)priv;

	skb->dev = net_priv->ndev;

	net_priv->stats.rx_bytes += skb->len;

	net_priv->stats.rx_packets++;

	skb->protocol = eth_type_trans(skb, skb->dev);

	skb->ip_summed = CHECKSUM_NONE;

	netif_rx_ni(skb);
	return 0;
}

int xradio_iface_net_set_mac(void *priv, u8 *mac)
{
	struct xradio_net_priv *net_priv = (struct xradio_net_priv *)priv;
	if (!priv)
		return -EINVAL;

	ether_addr_copy(net_priv->mac_address, mac);
	ether_addr_copy(net_priv->ndev->dev_addr, mac);
	return 0;
}
