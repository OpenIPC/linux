/*
 * xr806/queue.c
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * Transmit Queue implementation for Xr806 drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "os_intf.h"
#include "queue.h"
#include "debug.h"

int xradio_queue_init(struct xradio_queue *queue, uint8_t queue_id, size_t capacity,
		      unsigned long ttl, xr_free_skb_t cb)
{
	int i = 0;

	memset(queue, 0, sizeof(*queue));
	queue->capacity = capacity;
	queue->queue_id = queue_id;
	queue->ttl = ttl;
	queue->free_skb = cb;

	INIT_LIST_HEAD(&queue->queue);
	INIT_LIST_HEAD(&queue->pending);
	INIT_LIST_HEAD(&queue->free_pool);

	xradio_k_spin_lock_init(&queue->lock);

	queue->pool = xradio_k_zmalloc(sizeof(struct xradio_queue_item) * capacity);
	if (!queue->pool)
		return -ENOMEM;

	for (i = 0; i < capacity; ++i)
		list_add_tail(&queue->pool[i].head, &queue->free_pool);

	return 0;
}

void xradio_queue_deinit(struct xradio_queue *queue)
{
	// TODO - xradio_queue_clear(queue, XRWL_ALL_IFS);
	INIT_LIST_HEAD(&queue->free_pool);
	xradio_k_free(queue->pool);
	queue->pool = NULL;
	queue->capacity = 0;
}

int xradio_queue_put(struct xradio_queue *queue, struct sk_buff *skb, u16 seq)
{
	int ret = -1;

	xradio_k_spin_lock_bh(&queue->lock);

	ret = list_empty(&queue->free_pool);

	if (!ret) {
		struct xradio_queue_item *item =
			list_first_entry(&queue->free_pool, struct xradio_queue_item, head);

		if (!item) {
			ret = -ENOMEM; // TODO,check
			goto end;
		}

		item->skb = skb;

		item->seq_number = seq;

		item->queue_timestamp = jiffies;

		list_move_tail(&item->head, &queue->queue);

		++item->queue_index;

		++queue->num_queued;

		item->queue_index = queue->num_queued;

		if (queue->num_queued + queue->num_pending >= queue->capacity) {
			xradio_k_spin_unlock_bh(&queue->lock);
			return -ENOMEM;
		}

	} else {
		txrx_printk(XRADIO_DBG_WARN, "xradio queue is full.\n");

		xradio_queue_debug_info(queue);
	}
end:
	xradio_k_spin_unlock_bh(&queue->lock);

	return ret;
}

struct sk_buff *xradio_queue_get(struct xradio_queue *queue)
{
	struct xradio_queue_item *item = NULL;
	struct sk_buff *skb = NULL;

	xradio_k_spin_lock_bh(&queue->lock);

	if (!list_empty(&queue->queue)) {
		item = list_first_entry(&queue->queue, struct xradio_queue_item, head);

		if (!item) {
			xradio_queue_debug_info(queue);
			goto end;
		}

		skb = item->skb;

		list_move_tail(&item->head, &queue->pending);

		++queue->num_pending;

		--queue->num_queued;

		item->xmit_timestamp = jiffies;
	}
end:
	xradio_k_spin_unlock_bh(&queue->lock);
	return skb;
}

int xradio_queue_remove(struct xradio_queue *queue, u16 seq_number)
{
	int ret = -ENOENT;
	struct xradio_queue_item *item;

	xradio_k_spin_lock_bh(&queue->lock);

	list_for_each_entry(item, &queue->pending, head) {
		if (item->seq_number == seq_number) {
			ret = 0;
			break;
		}
	}

	if (!ret) {
		if (queue->free_skb) {
			queue->free_skb(item->skb, __func__);
			item->skb = NULL;
		}
		list_move(&item->head, &queue->free_pool);
		--queue->num_pending;
		txrx_printk(XRADIO_DBG_MSG, "Remove item seq_number%d\n", item->seq_number);
	} else {
		txrx_printk(XRADIO_DBG_ERROR, "item is not found,seq:%d\n", seq_number);
		xradio_queue_debug_info(queue);
	}

	xradio_k_spin_unlock_bh(&queue->lock);
	return ret;
}

int xradio_queue_multi_remove(struct xradio_queue *queue, u16 seq_number)
{
	int ret = -ENOENT;
	struct xradio_queue_item *item, *tmp_item;

	xradio_k_spin_lock_bh(&queue->lock);

	list_for_each_entry_safe(item, tmp_item, &queue->pending, head) {
		if ((item->seq_number % 255) <= seq_number) {
			if (queue->free_skb) {
				queue->free_skb(item->skb, __func__);
				item->skb = NULL;
			}
			list_move(&item->head, &queue->free_pool);
			--queue->num_pending;
			txrx_printk(XRADIO_DBG_MSG, "Remove item seq_number%d\n", item->seq_number);
		}
	}
	xradio_k_spin_unlock_bh(&queue->lock);
	return ret;
}

int xradio_queue_requeue(struct xradio_queue *queue, uint16_t seq_number)
{
	struct xradio_queue_item *item;
	int ret = -1;

	xradio_k_spin_lock_bh(&queue->lock);

	list_for_each_entry(item, &queue->pending, head) {
		if (item->seq_number == seq_number) {
			ret = 0;
			break;
		}
	}

	if (!ret) {
		list_move(&item->head, &queue->queue);
		--queue->num_pending;
		txrx_printk(XRADIO_DBG_MSG, "Requeue item seq_number%d\n", item->seq_number);
	}
	xradio_k_spin_unlock_bh(&queue->lock);
	return 0;
}

int xradio_queue_requeue_all(struct xradio_queue *queue, uint16_t seq_number)
{
	struct xradio_queue_item *item, *tmp_item;
	struct list_head tmp;
	int ret = 0;

	xradio_k_spin_lock_bh(&queue->lock);
	xradio_queue_debug_info(queue);

	INIT_LIST_HEAD(&tmp);

	list_for_each_entry_safe(item, tmp_item, &queue->pending, head) {
		if (item->seq_number < seq_number) {
			if (queue->free_skb) {
				queue->free_skb(item->skb, __func__);
				item->skb = NULL;
			}
			list_move(&item->head, &queue->free_pool);
			--queue->num_pending;
			txrx_printk(XRADIO_DBG_MSG, "Remove item seq_number%d\n", item->seq_number);
		} else {
			list_move(&item->head, &tmp);
			--queue->num_pending;
			ret++;
		}
	}

	if (ret) {
		list_for_each_entry_safe(item, tmp_item, &tmp, head) {
			item->queue_timestamp = jiffies;
			list_move(&item->head, &queue->queue);
			++queue->num_queued;

			txrx_printk(XRADIO_DBG_MSG, "Requeue item seq_number%d\n",
				    item->seq_number);
		}
	}
	xradio_queue_debug_info(queue);
	xradio_k_spin_unlock_bh(&queue->lock);

	return 0;
}

int xradio_queue_get_pending_num(struct xradio_queue *queue)
{
	return queue->num_pending;
}

int xradio_queue_get_queue_num(struct xradio_queue *queue)
{
	return queue->num_pending + queue->num_queued;
}

void xradio_queue_debug_info(struct xradio_queue *queue)
{
	struct xradio_queue_item *item = NULL;
	int free_pool_cnt = 0;

	queue_printk(XRADIO_DBG_ALWY, "Queue id    :%d\n", queue->queue_id);
	queue_printk(XRADIO_DBG_ALWY, "  Capacity  :%ld\n", queue->capacity);
	queue_printk(XRADIO_DBG_ALWY, "  Queued    :%ld\n", queue->num_queued);
	queue_printk(XRADIO_DBG_ALWY, "  Pending   :%ld\n", queue->num_pending);
	queue_printk(XRADIO_DBG_ALWY, "queued list info\n");
	list_for_each_entry(item, &queue->queue, head) {
		queue_printk(XRADIO_DBG_ALWY, "  queued--->seq:%d ,id:%d \n", item->seq_number,
			     item->queue_index);
	}
	queue_printk(XRADIO_DBG_ALWY, "pending list info\n");
	list_for_each_entry(item, &queue->pending, head) {
		queue_printk(XRADIO_DBG_ALWY, "  pending--->seq:%d ,id:%d skb:%pK\n",
			     item->seq_number, item->queue_index, item->skb);
	}
	list_for_each_entry(item, &queue->free_pool, head) { free_pool_cnt++; }
	queue_printk(XRADIO_DBG_ALWY, "pending list node cnt:%d\n", free_pool_cnt);
}
