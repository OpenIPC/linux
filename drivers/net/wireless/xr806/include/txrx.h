/*
 * xr806/txrx.c
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * tx and rx APIs for Xr806 drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __TXRX_H__
#define __TXRX_H__

int xradio_tx_cmd_process(void *priv, char *buffer, uint16_t len);
int xradio_tx_net_process(struct xradio_priv *priv, struct sk_buff *skb);
int xradio_register_trans(struct xradio_priv *priv);
void xradio_unregister_trans(struct xradio_priv *priv);
void xradio_wake_up_rx_work(void *priv);
void xradio_wake_up_tx_work(void *priv);
#endif
