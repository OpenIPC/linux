/*
 * xr806/iface.h
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * wlan interface APIs for Xr806 drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __IFACE_H__
#define __IFACE_H__

typedef int (*xr_net_data_tx_t)(void *priv, struct sk_buff *skb);

struct xradio_net_priv {
	struct net_device *ndev;
	struct net_device_stats stats;

	u8 mac_address[6];
	u8 if_type;
	u8 if_id;

	xr_net_data_tx_t tx_func;

	void *xradio_priv;
};

void *xradio_iface_add_net_dev(void *priv, u8 if_type, u8 if_id, char *name,
			       xr_net_data_tx_t tx_func);

void xradio_iface_remove_net_dev(void *priv);

void xradio_iface_net_tx_pause(void *priv);

void xradio_iface_net_tx_resume(void *priv);

int xradio_iface_net_input(void *priv, struct sk_buff *skb);

int xradio_iface_net_set_mac(void *priv, u8 *mac);
#endif
