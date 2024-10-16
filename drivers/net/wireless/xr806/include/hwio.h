/*
 * xr806/hwio.h
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * hardware interface APIs for Xr806 drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __HWIO_H__
#define __HWIO_H__

typedef void (*rx_ind_func)(void *arg);

struct xradio_bus_ops {
	int (*gpio_init)(void);
	int (*gpio_deinit)(void);
	int (*reg_rx_ind)(rx_ind_func func);
	int (*read_rx_gpio)(void);
	int (*read_rw_gpio)(void);
	int (*init)(void *priv);
	int (*write)(u8 *buff, u16 len);
	int (*read)(u8 *buff, u32 len);
	int (*deinit)(void *priv);
};

int xradio_hwio_init(void *priv);
int xradio_hwio_deinit(void *priv);
int xradio_hwio_write(struct sk_buff *skb);
struct sk_buff *xradio_hwio_read(u16 len);
int xradio_hwio_rx_pending(void);

#endif
