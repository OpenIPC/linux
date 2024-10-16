/*
* Copyright (c) 2008-2020 Allwinner Technology Co. Ltd.
* All rights reserved.
*
* File : ion_mem.c
* Description :
* History :
*   Author  : jilinglin <jilinglin@allwinnertech.com>
*   Date    : 2020/04/14
*   Comment :
*/
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>
#include <linux/ion_uapi.h>
#include <linux/ion_sunxi.h>
#include <asm/cacheflush.h>
#include "mem_interface.h"
#include "ve_mem_list.h"
#include "ion_mem.h"
#include "cdc_log.h"
//#include "cedar_ve_out.h"
extern struct device *cedar_get_device(void);

#define DEBUG_SHOW_TOTAL_PALLOC_SIZE (0)

struct buf_node {
	struct aw_mem_list_head entry;
	unsigned long phy_addr; //phisical address
	void *vir_addr; //virtual address
	unsigned int size; //buffer size
	struct ion_handle *handle;
	int dma_buf_fd;
	struct dma_buf *buf;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
};

struct mem_ion_ctx {
	struct mem_interface interface;
	int fd; // driver handle
	struct aw_mem_list_head list; // buffer list
	int ref_cnt; // reference count
	unsigned int phyOffset;
	struct ion_client *client;
	struct device *dev;
};

static int get_iommu_phy_addr(struct mem_ion_ctx *p, struct buf_node *node)
{
	struct sg_table *sgt, *sgt_bak;
	struct scatterlist *sgl, *sgl_bak;
	s32 sg_count = 0;
	int ret      = -1;
	int i;

#if 0
	node->attach = dma_buf_attach(node->buf, p->dev);
	if (IS_ERR(node->attach)) {
		loge("dma_buf_attach failed\n");
		goto err_buf_put;
	}
	sgt = dma_buf_map_attachment(node->attach, DMA_FROM_DEVICE);
	if (IS_ERR_OR_NULL(sgt)) {
		loge("dma_buf_map_attachment failed\n");
		goto err_buf_detach;
	}
#endif

	sgt = node->handle->buffer->sg_table;

	sgt_bak = kmalloc(sizeof(struct sg_table), GFP_KERNEL | __GFP_ZERO);
	if (sgt_bak == NULL) {
		loge("alloc sgt fail\n");
		goto err_buf_unmap;
	}
	ret = sg_alloc_table(sgt_bak, sgt->nents, GFP_KERNEL);
	if (ret != 0) {
		loge("alloc sgt fail\n");
		goto err_kfree;
	}
	sgl_bak = sgt_bak->sgl;
	for_each_sg (sgt->sgl, sgl, sgt->nents, i) {
		sg_set_page(sgl_bak, sg_page(sgl), sgl->length, sgl->offset);
		sgl_bak = sg_next(sgl_bak);
	}

	sg_count = dma_map_sg(p->dev, sgt_bak->sgl,
			      sgt_bak->nents, DMA_FROM_DEVICE);

	if (sg_count != 1) {
		loge("dma_map_sg failed:%d\n", sg_count);
		ret = -1;
		goto err_sgt_free;
	}

	node->sgt      = sgt_bak;
	node->phy_addr = sg_dma_address(sgt_bak->sgl);
	ret	    = 0;
	goto exit;

err_sgt_free:
	sg_free_table(sgt_bak);
err_kfree:
	kfree(sgt_bak);
err_buf_unmap:
	/* unmap attachment sgt, not sgt_bak, cause it's not alloc yet! */
	dma_buf_unmap_attachment(node->attach, sgt, DMA_FROM_DEVICE);
	/*err_buf_detach:*/
	dma_buf_detach(node->buf, node->attach);
	/*err_buf_put:*/
	dma_buf_put(node->buf);
exit:
	return ret;
}

#if DEBUG_SHOW_TOTAL_PALLOC_SIZE
static int total_palloc_size;
#endif

void *mem_ion_palloc_base(struct mem_interface *interface, int size, int bIsCacheFlag)
{
	struct ion_client *client;
	struct buf_node *node;
	unsigned int flags = 0;
	unsigned int heap_mask;
	unsigned int align;

	struct mem_ion_ctx *p = (struct mem_ion_ctx *)interface;

	if (!p)
		return NULL;

	node = kmalloc(sizeof(struct buf_node), GFP_KERNEL);

	if (node == NULL) {
		logd("malloc buffer node fail");
		return NULL;
	}

	client = p->client;

	if (bIsCacheFlag == 1)
		flags = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;

#if 0 //for debug
    int oldsize = size;
    if(size == 576000)
    {
        size *= 2;
    }

	logv("bisCacheFlag = %d, flags = %d, oldsize = %d, size = %d", bIsCacheFlag, flags, oldsize, size);
#else
	logv("bisCacheFlag = %d, flags = %d, size = %d", bIsCacheFlag, flags, size);
#endif

#if defined(CONFIG_SUNXI_IOMMU)
	heap_mask = 1 << ION_HEAP_TYPE_SYSTEM;
	align     = PAGE_SIZE;
#else
	heap_mask = 1 << ION_HEAP_TYPE_CARVEOUT;
	align     = PAGE_SIZE;
#endif

	node->handle = ion_alloc(client, size, align, heap_mask, flags);

	if (IS_ERR_OR_NULL(node->handle)) {
		logd("ion_alloc failed, size=%d 0x%p!", size, node->handle);
		return NULL;
	}

	node->vir_addr = ion_map_kernel(client, node->handle);

	if (IS_ERR_OR_NULL(node->vir_addr)) {
		logd("ion_map_kernel failed!!\n");
		goto err_map;
	}

	node->size = size;
#if 0
	//node->dma_buf_fd = ion_share_dma_buf_fd2(client, node->handle);
	//node->buf = dma_buf_get(node->dma_buf_fd);

	node->buf = ion_share_dma_buf(client, node->handle);

	if (IS_ERR(node->buf)) {
		loge("Get dmabuf of fd %d fail!\n", node->dma_buf_fd);
		goto err_phyaddr;
	}
#endif

#ifndef CONFIG_SUNXI_IOMMU
	if (ion_phys(client, node->handle, (ion_phys_addr_t)&node->phys_addr, &size)) {
		logd("ion_phys failed!!\n");
		goto err_phyaddr;
	}
#else
	if (get_iommu_phy_addr(p, node) != 0) {
		logd("iommu phys failed");
		goto err_phyaddr;
	}
#endif

	aw_mem_list_add_tail(&node->entry, &p->list);

#if DEBUG_SHOW_TOTAL_PALLOC_SIZE
	total_palloc_size += node->size;
	logd("palloc phy:%lx, vir:%p, flag: %d, size:%d--%d kb, totalS:%d--%d kb",
	     node->phy_addr, node->vir_addr, flags, node->size, node->size / 1024,
	     total_palloc_size, total_palloc_size / 1024);
#endif

	return node->vir_addr;

err_phyaddr:
	ion_unmap_kernel(client, node->handle);
err_map:
	ion_free(client, node->handle);
	return NULL;
}

void *mem_ion_palloc(struct mem_interface *interface, int size)
{
	int bIsCacheFlag = 1;

	return mem_ion_palloc_base(interface, size, bIsCacheFlag);
}

void *mem_ion_palloc_no_cache(struct mem_interface *interface, int size)
{
	int bIsCacheFlag = 0;

	return mem_ion_palloc_base(interface, size, bIsCacheFlag);
}

void mem_ion_pfree(struct mem_interface *interface, void *pbuf)
{
	struct buf_node *tmp;

	struct mem_ion_ctx *p = (struct mem_ion_ctx *)interface;

	if (p == NULL) {
		loge("ion_alloc do not opened, should call ion_alloc_open() \
			before ion_alloc_alloc(size) \n");
		return;
	}

	aw_mem_list_for_each_entry(tmp, &p->list, entry)
	{
		if (tmp->vir_addr == pbuf) {
#if DEBUG_SHOW_TOTAL_PALLOC_SIZE
			total_palloc_size -= tmp->size;
			logd("pfree phy:%lx, vir:%p, size:%d--%d kb, totalS:%d--%d kb",
			     tmp->phy_addr, tmp->vir_addr, tmp->size, tmp->size / 1024,
			     total_palloc_size, total_palloc_size / 1024);
#endif
			dma_unmap_sg(p->dev, tmp->sgt->sgl, tmp->sgt->nents, DMA_BIDIRECTIONAL);
			//dma_buf_unmap_attachment(tmp->attach, tmp->sgt, DMA_BIDIRECTIONAL);
			sg_free_table(tmp->sgt);
			kfree(tmp->sgt);
			//dma_buf_detach(tmp->buf, tmp->attach);
			//dma_buf_put(tmp->buf);
			//dma_buf_put(tmp->buf);
			ion_unmap_kernel(p->client, tmp->handle);
			ion_free(p->client, tmp->handle);
			aw_mem_list_del(&tmp->entry);
			kfree(tmp);
			break;
		}
	}
}

void *mem_ion_phy2vir(struct mem_interface *interface, unsigned long addr_phy)
{
	int flag       = 0;
	void *addr_vir = 0;
	struct buf_node *tmp;

	struct mem_ion_ctx *p = (struct mem_ion_ctx *)interface;

	aw_mem_list_for_each_entry(tmp, &p->list, entry)
	{
		if (addr_phy >= tmp->phy_addr && addr_phy < tmp->phy_addr + tmp->size) {
			addr_vir = tmp->vir_addr + addr_phy - tmp->phy_addr;
			flag     = 1;
			break;
		}
	}

	if (flag == 0)
		loge("ion_alloc_phy2vir failed, do not find physical address: 0x%lx \n", addr_phy);

	return addr_vir;
}

unsigned long mem_ion_vir2phy(struct mem_interface *interface, void *addr_vir)
{
	int flag	       = 0;
	unsigned long addr_phy = 0;
	struct buf_node *tmp;

	struct mem_ion_ctx *p = (struct mem_ion_ctx *)interface;

	aw_mem_list_for_each_entry(tmp, &p->list, entry)
	{
		//logd("vir:%p node vir:%p size:%d", addr_vir, tmp->vir_addr, tmp->size);
		if (addr_vir >= tmp->vir_addr && addr_vir < tmp->vir_addr + tmp->size) {
			addr_phy = tmp->phy_addr + addr_vir - tmp->vir_addr;
			flag     = 1;
			break;
		}
	}

	if (flag == 0)
		loge("ion_alloc_vir2phy failed, do not find physical address: %p \n", addr_vir);

	return addr_phy;
}

void mem_ion_flush_cache(struct mem_interface *p, void *startAddr, int size)
{
	struct sunxi_cache_range range;

	range.start = (unsigned long)startAddr;
	range.end   = (unsigned long)startAddr + size;

#ifdef CONFIG_ARM64
	__dma_flush_range((void *)range.start, range.end - range.start);
#else
	//logd("flush cache start:%lu end:%lu size:%d",range.start,range.end,size);
	dmac_flush_range((void *)range.start, (void *)range.end);
#endif
}

static int mem_ion_get_flag(struct mem_interface *p)
{
#if defined(CONFIG_SUNXI_IOMMU)
	return MEM_IOMMU_FLAG;
#else
	return MEM_NORMAL_FLAG;
#endif
}

int mem_ion_share_fd(struct mem_interface *interface, void *addr_vir)
{
	int flag = 0;
	struct buf_node *tmp;
	int share_fd = -1;

	struct mem_ion_ctx *p = (struct mem_ion_ctx *)interface;

	aw_mem_list_for_each_entry(tmp, &p->list, entry)
	{
		//logd("vir:%p node vir:%p size:%d", addr_vir, tmp->vir_addr, tmp->size);
		if (addr_vir >= tmp->vir_addr && addr_vir < tmp->vir_addr + tmp->size) {
			share_fd = ion_share_dma_buf_fd2(p->client, tmp->handle);
			flag     = 1;
			break;
		}
	}

	if (flag == 0)
		loge("mem_ion_share_fd failed, do not find physical address: %p \n", addr_vir);

	logv("share_fd = %d", share_fd);

	return share_fd;
}

struct mem_interface *mem_ion_create(void)
{
	struct mem_ion_ctx *context;

	context = kmalloc(sizeof(struct mem_ion_ctx), GFP_KERNEL);
	if (context == NULL) {
		loge("create ion allocator failed, out of memory \n");
		return NULL;
	}

	memset(context, 0, sizeof(struct mem_ion_ctx));

	context->interface.palloc_no_cache = mem_ion_palloc_no_cache;
	context->interface.palloc	  = mem_ion_palloc;
	context->interface.pfree	   = mem_ion_pfree;
	context->interface.get_phyaddr     = mem_ion_vir2phy;
	context->interface.get_viraddr     = mem_ion_phy2vir;
	context->interface.flush_cache     = mem_ion_flush_cache;
	context->interface.get_mem_flag    = mem_ion_get_flag;
	context->interface.share_fd	= mem_ion_share_fd;

	AW_MEM_INIT_LIST_HEAD(&context->list);

	//create ion client
	context->client = sunxi_ion_client_create("ion_ve");

	if (IS_ERR_OR_NULL(context->client)) {
		logd("ion client create failed!");
		return NULL;
	}
	context->dev = cedar_get_device();

	return &context->interface;
}

void mem_ion_destory(struct mem_interface *p)
{
	struct mem_ion_ctx *context = (struct mem_ion_ctx *)p;

	if (context == NULL) {
		logd("you destroy mem but p is null");
		return;
	}

	ion_client_destroy(context->client);
	kfree(context);
}
