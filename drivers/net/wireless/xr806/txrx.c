/*
 * xr806/txrx.c
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * tx and rx implementation for Xr806 drivers
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
#include "queue.h"
#include "os_net.h"
#include "debug.h"
#include "checksum.h"
#include "cmd_proto.h"
#include "up_cmd.h"
#include "low_cmd.h"
#include "data_test.h"

void xradio_wake_up_tx_work(void *priv)
{
	struct xradio_priv *_priv = (struct xradio_priv *)priv;

	xradio_k_atomic_add(1, &_priv->th_tx);

	wake_up(&_priv->txrx_wq);
}

void xradio_wake_up_rx_work(void *priv)
{
	struct xradio_priv *_priv = (struct xradio_priv *)priv;

	if (!xradio_k_atomic_read(&_priv->tranc_ready))
		return;

	wake_up(&_priv->txrx_wq);
}

int xradio_tx_cmd_process(void *priv, char *buffer, uint16_t len)
{
	struct xradio_hdr *hdr = NULL;
	struct xradio_priv *_priv = (struct xradio_priv *)priv;
	size_t total_len = 0;
	struct sk_buff *skb = NULL;
	u8 *tx_buff = NULL;
	int ret = -1;
	int pad_len = 0;

	static u8 seq_number;

	pad_len = SKB_DATA_ADDR_ALIGNMENT -
		  ((len + sizeof(struct xradio_hdr)) % SKB_DATA_ADDR_ALIGNMENT);

	total_len = sizeof(struct xradio_hdr) + len + pad_len;

	skb = xradio_alloc_skb(total_len, __func__);
	if (!skb) {
		txrx_printk(XRADIO_DBG_ERROR, "xradio alloc skb failed.\n");
		return -ENOMEM;
	}

	tx_buff = skb_put(skb, total_len);

	hdr = (struct xradio_hdr *)tx_buff;

	memset(hdr, 0, sizeof(struct xradio_hdr));

	hdr->cur_len = xradio_k_cpu_to_le16(len);

	hdr->next_len = 0;

	hdr->offset = xradio_k_cpu_to_le16(sizeof(struct xradio_hdr) + pad_len);

	hdr->message = xradio_k_cpu_to_le16(seq_number << 8 | XR_REQ_CMD);

	memcpy(tx_buff + hdr->offset, buffer, len);

	hdr->checksum = xradio_k_cpu_to_le16(
		xradio_crc_16(tx_buff + sizeof(struct xradio_hdr) + pad_len, len));

	ret = xradio_queue_put(&_priv->tx_queue[XR_CMD], skb, seq_number);
	if (!ret) {
		seq_number++;
		xradio_wake_up_tx_work(_priv);
	} else {
		txrx_printk(XRADIO_DBG_ERROR, "xradio queue is full, push cmd failed.\n");
		xraido_free_skb_any(skb);
	}
	return ret;
}

int xradio_tx_net_process(struct xradio_priv *priv, struct sk_buff *skb)
{
	u16 len = 0;
	int total_len = 0;
	int pad_len = 0;
	struct sk_buff *new_skb = NULL;
	struct xradio_hdr *hdr = NULL;
	u8 *pos = NULL;
	int ret = -1;
	static u8 seq_number;

	pad_len = sizeof(struct xradio_hdr);

	len = skb->len;

	total_len = len + pad_len;

	pad_len += SKB_DATA_ADDR_ALIGNMENT - (total_len % SKB_DATA_ADDR_ALIGNMENT);

	if ((skb_headroom(skb) < pad_len) ||
	    !IS_ALIGNED((unsigned long)skb->data, SKB_DATA_ADDR_ALIGNMENT)) {
		if (skb_linearize(skb)) {
			dev_kfree_skb(skb);
			return -1;
		}

		new_skb = xradio_alloc_skb(skb->len + pad_len, __func__);

		if (!new_skb) {
			txrx_printk(XRADIO_DBG_ERROR, "failed to allocate skb\n");
			dev_kfree_skb(skb);
			return -1;
		}

		pos = new_skb->data;
		pos += pad_len;

		skb_copy_from_linear_data(skb, pos, skb->len);

		skb_put(new_skb, skb->len + pad_len);

		dev_kfree_skb_any(skb);
		skb = new_skb;

	} else {
		skb_push(skb, pad_len);
	}

	hdr = (struct xradio_hdr *)skb->data;

	memset(hdr, 0, pad_len);

	hdr->cur_len = xradio_k_cpu_to_le16(len);

	hdr->next_len = 0;

	hdr->offset = xradio_k_cpu_to_le16(pad_len);

	hdr->message = xradio_k_cpu_to_le16(seq_number << 8 | XR_REQ_DATA);

	hdr->checksum = xradio_k_cpu_to_le16(xradio_crc_16((u8 *)hdr + pad_len, len));

	txrx_printk(XRADIO_DBG_MSG, "type:%2.2X, seq number:%d, len:%d\n", XR_REQ_DATA, seq_number,
		    len);

	ret = xradio_queue_put(&priv->tx_queue[XR_DATA], skb, seq_number);
	if (!ret) {
		seq_number++;
		xradio_wake_up_tx_work(priv);
	} else {
		txrx_printk(XRADIO_DBG_WARN, "tx data queue will full, tx data pause\n");
		xradio_net_tx_pause(priv);
		xradio_k_atomic_set(&priv->tx_pause, 1);
	}

	return ret;
}

static int xradio_rx_net_process(struct xradio_priv *priv, struct sk_buff *skb, u16 len, u8 seq)
{
	if (!priv || !skb)
		return -EFAULT;
	skb_trim(skb, len);
	// xradio_parse_frame(__func__, skb->data, 0, 0xffff);
	xradio_net_data_input(priv, skb);

	return 0;
}

static int xradio_rx_cmd_process(struct xradio_priv *priv, struct sk_buff *skb, u16 len, u8 seq)
{
	struct cmd_payload *cmd = NULL;
	int ret = -1;

	if (!priv || !skb)
		return -EFAULT;

	cmd = (struct cmd_payload *)skb->data;
	if (cmd->type >= XR_WIFI_DEV_HAND_WAY_RES && cmd->type <= XR_WIFI_DEV_KERNEL_MAX)
		ret = xradio_low_cmd_push(skb->data, len);
	else
		ret = xradio_up_cmd_push(skb->data, len);
	xradio_free_skb(skb, __func__);
	return ret;
}

static int xradio_rx_process(struct xradio_priv *priv, struct sk_buff *skb)
{
	struct xradio_hdr *hdr = NULL;
	u16 cur_len = 0, next_len;
	u16 offset = 0;
	u8 type_id = 0;
	u16 checksum = 0, c_checksum = 0;
	u8 seq = 0;
	static int dev_seq = -1;
	int i;

	if (!priv || !skb)
		return -EFAULT;

	hdr = (struct xradio_hdr *)skb->data;

	cur_len = le16_to_cpu(hdr->cur_len);

	next_len = le16_to_cpu(hdr->next_len);

	offset = le16_to_cpu(hdr->offset);

	checksum = le16_to_cpu(hdr->checksum);

	c_checksum = le16_to_cpu(xradio_crc_16((u8 *)hdr + offset, cur_len));

	if (checksum != c_checksum) {
		txrx_printk(XRADIO_DBG_ERROR,
			    "cur_len:%d, next_len:%d, offset:%d, checksum failed,[%d,%d]\n",
			    cur_len, next_len, offset, checksum, c_checksum);
		printk(KERN_INFO "RX:\n");
		for (i = 0; i < (cur_len + offset < 50 ? cur_len + offset : 40); i++)
			printk(KERN_INFO "%x ", skb->data[i]);
		printk(KERN_INFO "\n");
		xradio_free_skb(skb, __func__);
		return -1;
	}
	type_id = le16_to_cpu(hdr->message) & TYPE_ID_MASK;

	seq = (le16_to_cpu(hdr->message) & SEQ_NUM_MASK) >> 8;

	if (dev_seq == -1)
		dev_seq = seq;

	if (dev_seq != seq) {
		txrx_printk(XRADIO_DBG_WARN, "Missing pkt, expect:%d,actual:%d\n", dev_seq, seq);
		dev_seq = seq;
	}

	dev_seq++;

	if (dev_seq > 255)
		dev_seq = 0;

	skb_pull(skb, offset);

	/* incom data */
	if (type_id == XR_REQ_CMD) {
		xradio_rx_cmd_process(priv, skb, cur_len, seq);
	} else {
#if DATA_TEST
		xradio_data_test_rx_handle(skb->data, skb->len);
		xradio_free_skb(skb);
#else
		xradio_rx_net_process(priv, skb, cur_len, seq);
#endif
	}

	return next_len;
}

static void xradio_free_tx_buff(struct xradio_priv *priv, struct sk_buff *skb)
{
	struct xradio_hdr *hdr = NULL;
	u8 seq;
	u8 type_id = 0;

	if (!priv || !skb)
		return;
	hdr = (struct xradio_hdr *)skb->data;

	seq = (le16_to_cpu(hdr->message) & SEQ_NUM_MASK) >> 8;

	type_id = le16_to_cpu(hdr->message) & TYPE_ID_MASK;

	if (type_id)
		xradio_queue_remove(&priv->tx_queue[XR_CMD], seq);
	else
		xradio_queue_remove(&priv->tx_queue[XR_DATA], seq);
}

static struct sk_buff *xradio_get_tx_buff(struct xradio_priv *priv)
{
	struct sk_buff *skb = NULL;

	skb = xradio_queue_get(&priv->tx_queue[XR_CMD]);

	if (!skb) {
		skb = xradio_queue_get(&priv->tx_queue[XR_DATA]);

		if (xradio_k_atomic_read(&priv->tx_pause) &&
		    xradio_queue_get_queue_num(&priv->tx_queue[XR_DATA]) < XRWL_MAX_QUEUE_SZ / 2) {
			txrx_printk(XRADIO_DBG_WARN, "tx data resume\n");

			xradio_net_tx_resume(priv);

			xradio_k_atomic_set(&priv->tx_pause, 0);
		}
	}
	return skb;
}

static int xradio_txrx_thread(void *data)
{
	struct xradio_priv *priv = (struct xradio_priv *)data;
	struct sk_buff *tx_skb = NULL, *next_txskb = NULL, *rx_skb = NULL;
	struct xradio_hdr *cur_hdr = NULL, *next_hdr = NULL;
	int rx = 0, tx = 0, term = 0;
	int status = 0;
	int rx_len = 0;
	int tx_status = 0;

	xradio_k_atomic_set(&priv->tranc_ready, 1);

	while (1) {
		status = wait_event_interruptible(priv->txrx_wq, ({
							  rx = !xradio_hwio_rx_pending();
							  tx = xradio_k_atomic_read(&priv->th_tx);
							  term = xradio_k_thread_should_stop(
								  &priv->txrx_thread);
							  (rx || tx || term || rx_len);
						  }));

		if (term) {
			txrx_printk(XRADIO_DBG_ALWY, "xradio tx rx thread exit!\n");
			break;
		}
		if (rx || rx_len) {
			rx_skb = xradio_hwio_read(rx_len);
			if (rx_skb)
				rx_len = xradio_rx_process(priv, rx_skb);
		}

		if (tx) {
			if (tx_skb == NULL)
				tx_skb = xradio_get_tx_buff(priv);

			while (tx_skb) {
				if (tx_status == 0) {
					next_txskb = xradio_get_tx_buff(priv);

					if (next_txskb) {
						cur_hdr = (struct xradio_hdr *)tx_skb->data;

						next_hdr = (struct xradio_hdr *)next_txskb->data;

						cur_hdr->next_len = xradio_k_cpu_to_le16(
							le16_to_cpu(next_hdr->cur_len) +
							le16_to_cpu(next_hdr->offset));
					}
				}

				tx_status = xradio_hwio_write(tx_skb);

				if (tx_status == 0) {
					xradio_free_tx_buff(priv, tx_skb);

					if (xradio_k_atomic_read(&priv->th_tx) > 0)
						xradio_k_atomic_dec(&priv->th_tx);

					tx_skb = next_txskb;
				}
			}
		}
	}
	xradio_k_thread_exit(&priv->txrx_thread);
	return 0;
}

void xradio_unregister_trans(struct xradio_priv *priv)
{
	txrx_printk(XRADIO_DBG_ALWY, "txrx thread unregister.\n");
	wake_up(&priv->txrx_wq);
	xradio_k_thread_delete(&priv->txrx_thread);
}

int xradio_register_trans(struct xradio_priv *priv)
{
	xradio_k_atomic_set(&priv->th_tx, 0);
	xradio_k_atomic_set(&priv->th_rx, 0);

	xradio_k_atomic_set(&priv->tx_pause, 0);

	init_waitqueue_head(&priv->txrx_wq);

	if (xradio_k_thread_create(&priv->txrx_thread, "xr_txrx", xradio_txrx_thread, (void *)priv,
				   0, 4096)) {
		txrx_printk(XRADIO_DBG_ERROR, "create tx and rx thread failed\n");
		return -1;
	}
	return 0;
}
