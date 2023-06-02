/**
 * system/src/rpclnt/vq.h
 *
 * History:
 *    2012/08/15 - [Tzu-Jung Lee] created file
 *
 * Copyright 2008-2015 Ambarella Inc.  All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __VQ_H__
#define __VQ_H__

#if defined(__KERNEL__)
#include <linux/virtio_ring.h>
#include <linux/sched.h>
#else
#include <itron.h>
#include "virtio_ring.h"

static inline void *ambarella_virt_to_phys(void *addr)
{
        return addr;
}

static inline void *ambarella_phys_to_virt(void *addr)
{
        return addr;
}
#endif

struct vq {
#if defined(__KERNEL__)
	spinlock_t		lock;
	struct completion	comp;
#else
	ID			mtxid;
	ID			flgid;
#endif
	void			(*cb)(struct vq *vq);
	void			(*kick)(struct vq *vq);
	struct vring		vring;
	u16			last_avail_idx;
	u16			last_used_idx;
};

extern struct vq *vq_create(void (*cb)(struct vq *vq),
			    void (*kick)(struct vq *vq),
			    int num_bufs,
			    u32 vring_virt, int vring_algn);

extern void vq_wait_for_completion(struct vq *vq);
extern void vq_complete(struct vq *vq);

extern int vq_kick_prepare(struct vq *vq);
extern void vq_enable_used_notify(struct vq *vq);
extern void vq_disable_used_notify(struct vq *vq);
extern int vq_more_avail_buf(struct vq *vq);
extern int vq_get_avail_buf(struct vq *vq, void **buf, int *len);
extern int vq_add_used_buf(struct vq *vq, int idx, int len);
extern int vq_init_unused_bufs(struct vq *vq, void *buf, int len);

#endif /* __VQ_H__ */
