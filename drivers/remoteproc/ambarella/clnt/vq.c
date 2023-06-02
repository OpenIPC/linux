/**
 * system/src/rpclnt/vq.c
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

#if defined(__KERNEL__)
#include <linux/slab.h>
#else
#include <itron.h>
#include <kutil.h>
extern void clean_d_cache_region(void *addr, unsigned int size);
extern void flush_d_cache_region(void *addr, unsigned int size);
#endif

#include "vq.h"

/******************************************************************************
 * OS dependent utility functions
 *****************************************************************************/

#if defined(__KERNEL__)

static inline struct vq *vq_alloc(void)
{
	return kmalloc(sizeof(struct vq), GFP_KERNEL);
}

static inline void vq_init_completion(struct vq *vq)
{
	init_completion(&vq->comp);
}

inline void vq_wait_for_completion(struct vq *vq)
{
	wait_for_completion(&vq->comp);
}

inline void vq_complete(struct vq *vq)
{
	complete(&vq->comp);
}

/*
 * For Linux, we map the whole PPM region as non-cached.
 * So no cache operation needed.
 *
 */
static inline void vq_clean_d_cache_region(void *addr, unsigned int size)
{
}

static inline void vq_flush_d_cache_region(void *addr, unsigned int size)
{
}

static inline void vq_lock_init(struct vq *vq)
{
	spin_lock_init(&vq->lock);
}

#define vq_lock(vq, flag)	spin_lock_irqsave(&vq->lock, flag)
#define vq_unlock(vq, flag)	spin_unlock_irqrestore(&vq->lock, flag)

#else /* ITRON */

static inline struct vq *vq_alloc(void)
{
	struct vq *vq;

	pget_mpl(HEAP_MPLID, sizeof(*vq), (VP *)&vq);
	return vq;
}

static inline void vq_init_completion(struct vq *vq)
{
	T_CFLG pk_cflg;

	pk_cflg.flgatr	= (TA_TFIFO | TA_WMUL | TA_CLR);
	pk_cflg.iflgptn	= 0x0;
	vq->flgid	= acre_flg(&pk_cflg);

	K_ASSERT(vq->flgid > 0);
}

void vq_wait_for_completion(struct vq *vq)
{
	ER ercd;
	FLGPTN flgptn;

	ercd = wai_flg(vq->flgid, 0x1, TWF_ANDW, &flgptn);
	K_ASSERT(ercd == E_OK);
}

void vq_complete(struct vq *vq)
{
	ER ercd;

	ercd = set_flg(vq->flgid, 0x1);
	K_ASSERT(ercd == E_OK);
}

static inline void vq_clean_d_cache_region(void *addr, unsigned int size)
{
	u32 mask = (1 << CLINE) - 1;

	addr = (u32)addr &~ mask;
	size = (size + mask) &~ mask;

	clean_d_cache_region(addr, size);
}

static inline void vq_flush_d_cache_region(void *addr, unsigned int size)
{
	u32 mask = (1 << CLINE) - 1;

	addr = (u32)addr &~ mask;
	size = (size + mask) &~ mask;

	flush_d_cache_region(addr, size);
}

static inline void vq_lock_init(struct vq *vq)
{
	T_CMTX pk_cmtx;

	pk_cmtx.mtxatr  = TA_TFIFO;
	pk_cmtx.ceilpri = 0;
	vq->mtxid = acre_mtx(&pk_cmtx);
	K_ASSERT(vq->mtxid > 0);
}

static inline void vq_lock(struct vq *vq)
{
	ER er;

	er = loc_mtx(vq->mtxid);
	K_ASSERT (er == E_OK);
}

static inline void vq_unlock(struct vq *vq)
{
	ER er;

	er = unl_mtx(vq->mtxid);
	K_ASSERT (er == E_OK);
}
#endif /* __KERNEL__ */

/******************************************************************************/

int vq_kick_prepare(struct vq *vq)
{
	vq_flush_d_cache_region(&vq->vring.avail->flags,
				sizeof(vq->vring.avail->flags));

	return !(vq->vring.avail->flags & VRING_AVAIL_F_NO_INTERRUPT);
}
void vq_enable_used_notify(struct vq *vq)
{
	vq_flush_d_cache_region(&vq->vring.used->flags,
				sizeof(vq->vring.used->flags));

	vq->vring.used->flags &= ~VRING_USED_F_NO_NOTIFY;

	vq_clean_d_cache_region(&vq->vring.used->flags,
				sizeof(vq->vring.used->flags));
}

void vq_disable_used_notify(struct vq *vq)
{
	vq_flush_d_cache_region(&vq->vring.used->flags,
				sizeof(vq->vring.used->flags));

	vq->vring.used->flags |= VRING_USED_F_NO_NOTIFY;

	vq_clean_d_cache_region(&vq->vring.used->flags,
				sizeof(vq->vring.used->flags));
}

int vq_more_avail_buf(struct vq *vq)
{
	struct vring_avail *avail = vq->vring.avail;

	vq_flush_d_cache_region(avail, sizeof(*avail));

	return vq->last_avail_idx != avail->idx;
}

struct vq *vq_create(void (*cb)(struct vq *vq),
		     void (*kick)(struct vq *vq),
		     int num_bufs,
		     u32 vring_addr, int vring_algn)
{
	struct vq *vq = vq_alloc();

	vq_init_completion(vq);
	vq_lock_init(vq);

	vq->cb 			= cb;
	vq->kick		= kick;
	vq->last_avail_idx	= 0;
	vq->last_used_idx	= 0;

	vring_init(&vq->vring, num_bufs, (void*)vring_addr, vring_algn);
	printk("vring size: 0x%x\n", vring_size(num_bufs, vring_algn));

	return vq;
}

/*
 * vq_get_avail_buf and vq_add_used_buf usually used in pairs to
 * grab a "available" buffer, use it, and then tell the HOST, we
 * finish our usage.
 */
int vq_get_avail_buf(struct vq *vq, void **buf, int *len)
{
	int				idx;
	u16				last_avail_idx;
	struct vring_avail		*avail;
	struct vring_desc		*desc;
	unsigned long			flag;

	vq_lock(vq, flag);

	if (!vq_more_avail_buf(vq)) {
		vq_unlock(vq, flag);
		return -1;
	}

	last_avail_idx	= vq->last_avail_idx++ % vq->vring.num;
	avail		= vq->vring.avail;

	vq_flush_d_cache_region(avail, sizeof(*avail));
	vq_flush_d_cache_region(&avail->ring[last_avail_idx], sizeof(*avail->ring));

	idx		= avail->ring[last_avail_idx];
	desc		= &vq->vring.desc[idx];
	vq_flush_d_cache_region(desc, sizeof(*desc));

	*buf		= (void*)ambarella_phys_to_virt(desc->addr);
	*len		= desc->len;

	vq_flush_d_cache_region(*buf, *len);

	vq_unlock(vq, flag);

	return idx;
}

int vq_add_used_buf(struct vq *vq, int idx, int len)
{
	struct vring_used_elem	*used_elem;
	struct vring_used	*used;
	unsigned long		flag;

	vq_lock(vq, flag);

	/* Clean the cache of buffer of the used buffer */
	vq_clean_d_cache_region((void*)(unsigned long)vq->vring.desc[idx].addr, len);

	/* Update the used descriptor and clean its cache region */
	used		= vq->vring.used;
	used_elem	= &used->ring[used->idx % vq->vring.num];
	used_elem->id	= idx;
	used_elem->len	= len;
	vq_clean_d_cache_region(used_elem, sizeof(*used_elem));

	/*
	 * Update the used descriptor index and clean its cache region.
	 * This step has to be done as the last step.  The order matters.
	 */
	used->idx++;
	vq_clean_d_cache_region(used, sizeof(*used));

	vq_unlock(vq, flag);

	return 0;
}

/*
 * Linux HOST rpmsg implementation only setup the RX buffer descriptors on HOST.
 * Buffer descriptors for HOST TX (CLIENT RX) are left for CLIENT to setup.
 *
 * A very IMPORTANT thing should be noted: Though the Linux HOST does not setup
 * the HOST TX descriptors, it "zeros out" them during vring initialization.
 * So this API should be synchronized and called only after the Linux HOST has
 * finished their initialization.
 */
int vq_init_unused_bufs(struct vq *vq, void *buf, int len)
{
	struct vring_desc	*desc;
	unsigned long		flag;
	int			i;

	vq_lock(vq, flag);

	for (i = 0; i < vq->vring.num; i++) {

		desc		= &vq->vring.desc[i];
		desc->addr	= ambarella_virt_to_phys((u32)buf);
		desc->len	= len;
		vq_clean_d_cache_region(desc, sizeof(*desc));

		buf += len;
	}

	vq_unlock(vq, flag);

	return 0;
}

#if 0
/*
 * Normally, this API should not be used for now. We just keep it in case the
 * API expansion or more comprehensive bits are added in the future.
 */

int vq_add_avail_buf(struct vq *vq, void *buf, int len)
{
	struct vring_avail	*avail;
	struct vring_desc	*desc;
	unsigned long		flag;

	vq_lock(vq, flag);
	vq->num_free--;

	avail		= vq->vring.avail;
	desc		= &vq->vring.desc[avail->idx % vq->vring.num];
	desc->addr	= ambarella_virt_to_phys((u32)buf);
	desc->len	= len;
	vq_clean_d_cache_region(desc, sizeof(*desc));

	/*
	 * Update the avail descriptor index and clean its cache region.
	 * This step has to be done as the last step.  The order matters.
	 */
	avail->idx++;
	vq_clean_d_cache_region(avail, sizeof(*avail));

	vq_unlock(vq, flag);

	return vq->num_free;
}

void *vq_get_used_buf(struct vq *vq)
{
	int			last_used_idx;
	struct vring_used_elem	*used_elem;
	struct vring_desc	*desc;
	void			*buf;
	unsigned long		flag;

	vq_lock(vq, flag);

	last_used_idx	= vq->last_used_idx++ % vq->vring.num;
	used_elem	= &vq->vring.used->ring[last_used_idx];
	vq_flush_d_cache_region(used_elem, sizeof(*used_elem));

	desc		= &vq->vring.desc[used_elem->id];
	vq_flush_d_cache_region(desc, sizeof(*desc));

	buf		= (void*)ambarella_phys_to_virt(desc->addr);
	vq_flush_d_cache_region(buf, desc->len);

	vq_unlock(vq, flag);

	return buf;
}
#endif
