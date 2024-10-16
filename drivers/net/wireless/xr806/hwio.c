/*
 * xr806/hwio.c
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * hardware interface implementation for Xr806 drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#include "os_intf.h"
#include "xradio.h"
#include "hwio.h"
#include "spi.h"
#include "txrx.h"
#include "debug.h"

#define SPI_BUF_SIZE 1536
static struct xradio_bus_ops *ops;

void xradio_hwio_rx_irq(void *arg)
{
	if (!arg)
		return;

	xradio_wake_up_rx_work(arg);
}

int xradio_hwio_init(void *priv)
{
	ops = xradio_get_bus_ops();
	if (!ops)
		return -EFAULT;

	if (ops->init(priv))
		return -1;

	if (ops->gpio_init()) {
		ops->deinit(priv);
		return -1;
	}

	if (ops->reg_rx_ind(xradio_hwio_rx_irq)) {
		ops->gpio_deinit();
		ops->deinit(priv);
		return -1;
	}

	return 0;
}

int xradio_hwio_deinit(void *priv)
{
	ops->gpio_deinit();

	ops->deinit(priv);

	return 0;
}

static int xradio_write_response(void)
{
	u8 res[8];

	// wait dev enter write state(gpio == 0)
	while (ops->read_rw_gpio())
		xradio_k_udelay(1);

	if (!ops->read(res, 8))
		return -1;

	return 0;
}

int xradio_hwio_write(struct sk_buff *skb)
{
	if (!ops || !ops->write)
		return -EFAULT;

	if (!skb)
		return -ENOMEM;

	// wait dev enter read state(gpio == 1)
	while (!ops->read_rw_gpio())
		xradio_k_udelay(1);

	if (ops->write(skb->data, skb->len)) {
		hwio_printk(XRADIO_DBG_ERROR, "write data error\n");
		return -1;
	}

	if (xradio_write_response() != 0)
		hwio_printk(XRADIO_DBG_ERROR, "write rsp error\n");
	return 0;
}

static int xradio_read_request(void)
{
	u8 req[8] = {0, 0, 2, 0, 0, 0, 0, 0};
	// wait dev enter read state(gpio == 1)
	while (!ops->read_rw_gpio())
		xradio_k_udelay(1);

	return ops->write(req, 8);
}

struct sk_buff *xradio_hwio_read(u16 len)
{
	struct sk_buff *rx_skb = NULL;
	u8 *buf = NULL;

	if (!ops || !ops->read)
		return NULL;

	if (xradio_read_request()) {
		hwio_printk(XRADIO_DBG_ERROR, "read request failed\n");
		return NULL;
	}

	// wait dev enter write state(gpio == 0)
	while (ops->read_rw_gpio())
		xradio_k_udelay(1);

	if (len == 0 || len > SPI_BUF_SIZE)
		len = SPI_BUF_SIZE;

	rx_skb = xradio_alloc_skb(len, __func__);

	buf = skb_put(rx_skb, len);

	memset(buf, 0, len);

	if (ops->read(buf, len) < 0)
		goto end;

	if (buf[0] == 0 && buf[1] == 0) {
		// hwio_printk(XRADIO_DBG_ERROR, "read data is exception.\n");
		goto end;
	}

	return rx_skb;

end:
	xradio_free_skb(rx_skb, __func__);
	return NULL;
}

int xradio_hwio_rx_pending(void)
{
	if (!ops || !ops->read_rx_gpio)
		return 0;

	return ops->read_rx_gpio();
}
