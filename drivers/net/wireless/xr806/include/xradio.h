/*
 * xr806/xradio.h
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * xradio common private APIs for Xr806 drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __XRADIO_H__
#define __XRADIO_H__

#include <linux/netdevice.h>
#include "xr_hdr.h"
#include "queue.h"

typedef enum {
	XR_STA_IF,
	XR_AP_IF,
} XR_INTERFACE_TYPE;

typedef enum {
	XR_LINK_DOWN,
	XR_LINK_UP,
} XR_LINK_STATUS;

struct xradio_priv {
	struct xradio_queue tx_queue[XR_MAX];
	xr_thread_handle_t txrx_thread;

	wait_queue_head_t txrx_wq;

	xr_atomic_t th_tx;
	xr_atomic_t th_rx;

	xr_atomic_t tranc_ready;

	xr_atomic_t tx_pause;

	u8 link_state;

	void *net_dev_priv;
};

#define XRWL_MAX_QUEUE_SZ (128)
#endif
