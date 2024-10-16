/*
 * xr806/queue.h
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * Transmit Queue APIs for Xr806 drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __XRADIO_QUEUE_H__
#define __XRADIO_QUEUE_H__

struct xradio_queue_item {
	struct list_head head;
	struct sk_buff *skb;
	u8 seq_number;
	u16 queue_index;
	unsigned long queue_timestamp;
	unsigned long xmit_timestamp;
};

typedef void (*xr_free_skb_t)(struct sk_buff *skb, const char *func);

struct xradio_queue {
	size_t capacity;
	size_t num_queued;
	size_t num_pending;
	struct xradio_queue_item *pool;
	struct list_head queue;
	struct list_head free_pool;
	struct list_head pending;
	bool overfull;
	xr_spinlock_t lock;
	u8 queue_id;
	unsigned long ttl;
	xr_free_skb_t free_skb;
};

int xradio_queue_init(struct xradio_queue *queue, uint8_t queue_id, size_t capacity,
		      unsigned long ttl, xr_free_skb_t cb);

void xradio_queue_deinit(struct xradio_queue *queue);

int xradio_queue_put(struct xradio_queue *queue, struct sk_buff *skb, u16 seq);

struct sk_buff *xradio_queue_get(struct xradio_queue *queue);

int xradio_queue_requeue(struct xradio_queue *queue, u16 seq_number);

int xradio_queue_requeue_all(struct xradio_queue *queue, u16 seq_number);

int xradio_queue_multi_remove(struct xradio_queue *queue, u16 seq_number);

int xradio_queue_remove(struct xradio_queue *queue, u16 seq_number);
int xradio_queue_get_pending_num(struct xradio_queue *queue);

int xradio_queue_get_queue_num(struct xradio_queue *queue);

void xradio_queue_debug_info(struct xradio_queue *queue);
#if 0
#define xradio_queue_debug_info(q)                                                                 \
	{                                                                                          \
		struct xradio_queue *queue = (struct xradio_queue *)q;                             \
		struct xradio_queue_item *item = NULL;                                             \
		int free_pool_cnt = 0;                                                             \
		queue_printk(XRADIO_DBG_ALWY, "Queue id    :%d\n", queue->queue_id);               \
		queue_printk(XRADIO_DBG_ALWY, "  Capacity  :%ld\n", queue->capacity);              \
		queue_printk(XRADIO_DBG_ALWY, "  Queued    :%ld\n", queue->num_queued);            \
		queue_printk(XRADIO_DBG_ALWY, "  Pending   :%ld\n", queue->num_pending);           \
		queue_printk(XRADIO_DBG_ALWY, "queued list info\n");                               \
		list_for_each_entry(item, &queue->queue, head) {                                  \
			queue_printk(XRADIO_DBG_ALWY, "  queued--->seq:%d ,id:%d \n",              \
				     item->seq_number, item->queue_index);                         \
		}                                                                                  \
		queue_printk(XRADIO_DBG_ALWY, "pending list info\n");                              \
		list_for_each_entry(item, &queue->pending, head) {                                 \
			queue_printk(XRADIO_DBG_ALWY, "  pending--->seq:%d ,id:%d \n",             \
				     item->seq_number, item->queue_index);                         \
		}                                                                                  \
		list_for_each_entry(item, &queue->free_pool, head) { free_pool_cnt++; }            \
		queue_printk(XRADIO_DBG_ALWY, "pending list node cnt:%d\n", free_pool_cnt);        \
	}
#endif
#endif
