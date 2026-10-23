// SPDX-License-Identifier: GPL-2.0
/*
 * From-source frame-buffer allocator over a CMM carveout (#45).
 * See framebuf_alloc.h for the ABI and design notes.
 *
 * First-fit over a bus-address-sorted allocation list. The region is a DT
 * reserved-memory node (#83) whose base and size the platform glue hands over
 * with vcmd_fb_set_region(); on 4.19 it was a pair of module parameters that a
 * shell loader computed from the board id. 136MB covers the 4K floorplan
 * (91MB with the prover input region, 59MB without; 1080p is 23MB).
 * docs/vcmd-cma-unblock.md.
 *
 * The kernel never maps this memory: only the encoder DMAs it and userspace
 * mmaps it (write-combining) through the char device, so the reserved-memory
 * node is a plain `no-map` region rather than a dma pool, and this file is
 * pure address-space bookkeeping.
 */
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/dma-buf.h>
#include <linux/dma-direction.h>
#include <linux/scatterlist.h>
#include <linux/err.h>

#include "framebuf_alloc.h"

static unsigned long framebuf_base;
static unsigned long framebuf_size;

void vcmd_fb_set_region(unsigned long base, unsigned long size)
{
	framebuf_base = base;
	framebuf_size = size;
}

struct fb_alloc {
	struct list_head node;   /* sorted by bus, ascending */
	unsigned long bus;
	unsigned long size;
	struct file *owner;
};

static LIST_HEAD(fb_list);
static DEFINE_MUTEX(fb_lock);

int vcmd_fb_alloc(struct file *filp, unsigned long size, unsigned long *bus)
{
	struct fb_alloc *a, *n;
	unsigned long hole;

	size = PAGE_ALIGN(size);
	if (!size || size > framebuf_size)
		return -EINVAL;

	n = kzalloc(sizeof(*n), GFP_KERNEL);
	if (!n)
		return -ENOMEM;
	n->size = size;
	n->owner = filp;

	mutex_lock(&fb_lock);
	hole = framebuf_base;
	list_for_each_entry(a, &fb_list, node) {
		if (a->bus - hole >= size)
			break;
		hole = a->bus + a->size;
	}
	if (framebuf_base + framebuf_size - hole < size) {
		mutex_unlock(&fb_lock);
		kfree(n);
		return -ENOMEM;
	}
	n->bus = hole;
	/* insert sorted: before the first entry above the hole */
	list_for_each_entry(a, &fb_list, node)
		if (a->bus > hole)
			break;
	list_add_tail(&n->node, &a->node); /* works for head too (entry==head) */
	mutex_unlock(&fb_lock);

	*bus = n->bus;
	return 0;
}

int vcmd_fb_free(struct file *filp, unsigned long bus)
{
	struct fb_alloc *a;
	int ret = -ENOENT;

	mutex_lock(&fb_lock);
	list_for_each_entry(a, &fb_list, node) {
		if (a->bus == bus) {
			if (a->owner != filp) {
				ret = -EPERM;
				break;
			}
			list_del(&a->node);
			kfree(a);
			ret = 0;
			break;
		}
	}
	mutex_unlock(&fb_lock);
	return ret;
}

static void vcmd_imp_release_filp(struct file *filp);

void vcmd_fb_release_filp(struct file *filp)
{
	struct fb_alloc *a, *tmp;

	mutex_lock(&fb_lock);
	list_for_each_entry_safe(a, tmp, &fb_list, node) {
		if (a->owner == filp) {
			list_del(&a->node);
			kfree(a);
		}
	}
	mutex_unlock(&fb_lock);
	vcmd_imp_release_filp(filp);
}

int vcmd_fb_lookup(unsigned long phy, unsigned long *size)
{
	struct fb_alloc *a;
	int ret = -ENOENT;

	mutex_lock(&fb_lock);
	list_for_each_entry(a, &fb_list, node) {
		if (a->bus == phy) {
			*size = a->size;
			ret = 0;
			break;
		}
	}
	mutex_unlock(&fb_lock);
	return ret;
}

/* ------------------------------------------------------------------------ */
/* dma-buf import (#60 M3): V4L2 capture frame -> encoder bus address       */
/* ------------------------------------------------------------------------ */

struct fb_import {
	struct list_head node;
	struct dma_buf *dbuf;
	struct dma_buf_attachment *att;
	struct sg_table *sgt;
	unsigned long bus;
	unsigned long size;
	struct file *owner;
};

static LIST_HEAD(imp_list);            /* guarded by fb_lock */
static struct device *fb_dev;

void vcmd_fb_set_dev(struct device *dev)
{
	fb_dev = dev;
}

static void imp_drop(struct fb_import *im)
{
	dma_buf_unmap_attachment_unlocked(im->att, im->sgt, DMA_TO_DEVICE);
	dma_buf_detach(im->dbuf, im->att);
	dma_buf_put(im->dbuf);
	kfree(im);
}

int vcmd_dmabuf_import(struct file *filp, int fd, unsigned long *bus,
		       unsigned long *size)
{
	struct fb_import *im;
	struct dma_buf *dbuf;
	struct dma_buf_attachment *att;
	struct sg_table *sgt;
	int ret;

	if (!fb_dev)
		return -ENODEV;

	dbuf = dma_buf_get(fd);
	if (IS_ERR(dbuf))
		return PTR_ERR(dbuf);

	im = kzalloc(sizeof(*im), GFP_KERNEL);
	if (!im) {
		ret = -ENOMEM;
		goto err_put;
	}

	att = dma_buf_attach(dbuf, fb_dev);
	if (IS_ERR(att)) {
		ret = PTR_ERR(att);
		goto err_free;
	}

	sgt = dma_buf_map_attachment_unlocked(att, DMA_TO_DEVICE);
	if (IS_ERR(sgt)) {
		ret = PTR_ERR(sgt);
		goto err_detach;
	}
	/* The encoder register program takes one base address: the buffer
	 * must be a single contiguous run (the open capture exporter's are). */
	if (sgt->nents != 1 || !sg_dma_len(sgt->sgl)) {
		ret = -EINVAL;
		goto err_unmap;
	}

	im->dbuf = dbuf;
	im->att = att;
	im->sgt = sgt;
	im->bus = sg_dma_address(sgt->sgl);
	im->size = sg_dma_len(sgt->sgl);
	im->owner = filp;

	mutex_lock(&fb_lock);
	list_add_tail(&im->node, &imp_list);
	mutex_unlock(&fb_lock);

	*bus = im->bus;
	*size = im->size;
	return 0;

err_unmap:
	dma_buf_unmap_attachment_unlocked(att, sgt, DMA_TO_DEVICE);
err_detach:
	dma_buf_detach(dbuf, att);
err_free:
	kfree(im);
err_put:
	dma_buf_put(dbuf);
	return ret;
}

int vcmd_dmabuf_release(struct file *filp, unsigned long bus)
{
	struct fb_import *im, *found = NULL;
	int ret = -ENOENT;

	mutex_lock(&fb_lock);
	list_for_each_entry(im, &imp_list, node) {
		if (im->bus == bus) {
			if (im->owner != filp) {
				ret = -EPERM;
				break;
			}
			list_del(&im->node);
			found = im;
			ret = 0;
			break;
		}
	}
	mutex_unlock(&fb_lock);
	if (found)
		imp_drop(found);
	return ret;
}

/* device close: drop every import this file made (called by
 * vcmd_fb_release_filp after the allocations) */
static void vcmd_imp_release_filp(struct file *filp)
{
	struct fb_import *im, *tmp;
	LIST_HEAD(mine);

	mutex_lock(&fb_lock);
	list_for_each_entry_safe(im, tmp, &imp_list, node)
		if (im->owner == filp)
			list_move_tail(&im->node, &mine);
	mutex_unlock(&fb_lock);

	list_for_each_entry_safe(im, tmp, &mine, node) {
		list_del(&im->node);
		imp_drop(im);
	}
}
