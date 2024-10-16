/*
 * xr806/iface.c
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * low protocol command implementation for Xr806 drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/netdevice.h>
#include <linux/semaphore.h>
#include <linux/version.h>
#include "os_intf.h"
#include "queue.h"
#include "xradio.h"
#include "xr_version.h"
#include "low_cmd.h"
#include "debug.h"
#include "txrx.h"
#include "cmd_proto.h"
#include "os_net.h"

#define CMD_WAIT_TIMEOUT (HZ * 5)
#define EVENT_QUEUE_LEN (16)

struct xradio_evt_item {
	struct list_head head;
	u8 *data;
	u32 len;
};

struct cmd_priv {
	void *xr_priv;
	struct mutex lock;

	struct xradio_evt_item *pool;

	struct list_head free_pool;
	struct list_head even_queue;
	struct semaphore sem_wait;

	xr_atomic_t cmd_sync_pending;
};

static struct cmd_priv cmd;

int xradio_low_cmd_init(void *priv)
{
	int i = 0;

	cmd.xr_priv = priv;

	sema_init(&cmd.sem_wait, 0);

	mutex_init(&cmd.lock);

	INIT_LIST_HEAD(&cmd.even_queue);
	INIT_LIST_HEAD(&cmd.free_pool);

	cmd.pool = xradio_k_zmalloc(sizeof(struct xradio_evt_item) * EVENT_QUEUE_LEN);
	if (!cmd.pool)
		return -ENOMEM;

	memset(cmd.pool, 0, sizeof(struct xradio_evt_item) * EVENT_QUEUE_LEN);

	for (i = 0; i < EVENT_QUEUE_LEN; i++)
		list_add_tail(&cmd.pool[i].head, &cmd.free_pool);
	return 0;
}

void xradio_low_cmd_deinit(void *priv)
{
	if (cmd.pool)
		xradio_k_free(cmd.pool);

	memset(&cmd, 0, sizeof(struct cmd_priv));
}

static struct cmd_payload *xradio_low_cmd_payload_calloc(u16 type, u32 len, void *param)
{
	struct cmd_payload *payload = NULL;

	payload = (struct cmd_payload *)xradio_k_zmalloc(sizeof(struct cmd_payload) + len);

	if (!payload)
		return NULL;

	payload->type = type;
	payload->len = len;

	if (param)
		memcpy(payload->param, param, len);

	return payload;
}

static void xradio_low_cmd_payload_free(struct cmd_payload *payload)
{
	if (payload)
		xradio_k_free(payload);
}

static void xradio_low_cmd_res_handle(struct cmd_payload *payload)
{
	if (!payload)
		return;

	switch (payload->type) {
	case XR_WIFI_DEV_DATA_TEST_RES:
		cmd_printk(XRADIO_DBG_ALWY, "Recv data len:%d\n", payload->len);
		break;
	}
}

int xradio_low_cmd_push(u8 *buff, u32 len)
{
	u8 *data = NULL;
	struct cmd_payload *payload = NULL;

	payload = (struct cmd_payload *)buff;

	if (xradio_k_atomic_read(&cmd.cmd_sync_pending)) {
		data = xradio_k_zmalloc(len);
		if (!data) {
			cmd_printk(XRADIO_DBG_ERROR, "Cmd event malloc failed\n");
			goto sem;
		}

		memcpy(data, payload->param, payload->len);

		if (!list_empty(&cmd.free_pool)) {
			struct xradio_evt_item *item =
				list_first_entry(&cmd.free_pool, struct xradio_evt_item, head);

			item->data = data;
			item->len = len;

			list_move_tail(&item->head, &cmd.even_queue);
		} else {
			cmd_printk(XRADIO_DBG_ERROR, "cmd free pool is empty.\n");
		}

sem:
		up(&cmd.sem_wait);
	} else {
		xradio_low_cmd_res_handle(payload);
	}
	return 0;
}

static int xradio_low_cmd_send_sync(void *priv, u8 *buff, u32 len, void *call_args,
				    u32 call_args_len)
{
	int ret = -1;

	mutex_lock(&cmd.lock);

	xradio_k_atomic_set(&cmd.cmd_sync_pending, 1);

	if (xradio_tx_cmd_process(priv, buff, len)) {
		cmd_printk(XRADIO_DBG_ERROR, "tx cmd failed\n");
		goto end;
	}
	if (down_timeout(&cmd.sem_wait, CMD_WAIT_TIMEOUT)) {
		cmd_printk(XRADIO_DBG_ERROR, "wait cmd response timeout");
		goto end;
	} else {
		if (!list_empty(&cmd.even_queue)) {
			struct xradio_evt_item *item =
				list_first_entry(&cmd.even_queue, struct xradio_evt_item, head);

			memcpy(call_args, item->data, call_args_len);

			xradio_k_free(item->data);

			item->data = NULL;

			list_move_tail(&item->head, &cmd.free_pool);
		}
		ret = 0;
	}
end:
	xradio_k_atomic_set(&cmd.cmd_sync_pending, 0);
	mutex_unlock(&cmd.lock);
	return ret;
}

int xraido_low_cmd_dev_hand_way(void)
{
	int ret = -1;

	struct cmd_payload *payload = NULL;
	struct cmd_para_hand_way hand_way = {
		.id = XRADIO_VERSION_ID,
	};

	struct cmd_para_hand_way_res event;

	payload = xradio_low_cmd_payload_calloc(XR_WIFI_HOST_HAND_WAY,
						sizeof(struct cmd_para_hand_way), &hand_way);

	if (!payload)
		return -ENOMEM;

	ret = xradio_low_cmd_send_sync(cmd.xr_priv, (u8 *)payload,
				       sizeof(struct cmd_payload) + payload->len, &event,
				       sizeof(struct cmd_para_hand_way_res));

	if (ret)
		cmd_printk(XRADIO_DBG_ERROR, "hand way send data failed\n");
	if (event.id == XRADIO_VERSION_ID + 1) {
		cmd_printk(XRADIO_DBG_ALWY, "Host hand way dev success.\n");
		xradio_net_set_mac(cmd.xr_priv, event.mac);
		// TODO,set ip address.
	} else {
		cmd_printk(XRADIO_DBG_ERROR, "Host hand way dev failed:%d\n", event.id);
		ret = -1;
	}
	xradio_low_cmd_payload_free(payload);
	return ret;
}
