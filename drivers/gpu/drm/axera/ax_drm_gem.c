/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <drm/drm.h>
#include <drm/drm_print.h>
#include <drm/drm_gem.h>
#include <drm/drm_vma_manager.h>
#include <drm/drm_gem_cma_helper.h>

#include <linux/dma-buf.h>
#include <linux/iommu.h>

#include "ax_drm_drv.h"
#include "ax_drm_gem.h"

#define GEM_FLAGS_USE_BLK 0xa5a5a5a5
#define GEM_NON_ALLOC_MAGIC 0x5a

static int ax_gem_alloc_dma(struct ax_gem_object *ac_obj, bool alloc_kmap)
{
	int ret;
	struct drm_gem_object *obj = &ac_obj->base;
	struct ax_drm_mgr *priv = ac_obj->priv;
	struct ax_display_dev *dp_dev = &priv->dp_mgr->display_dev[0];
	struct ax_display_funcs *dp_funs = dp_dev->display_funs;

	DRM_DEBUG_DRIVER("enter, flags: 0x%x\n", ac_obj->flags);

	/* When this flag is used, there is no need to allocate memory */
	if (ac_obj->flags == GEM_FLAGS_USE_BLK) {
		ac_obj->kvaddr = (void *)GEM_NON_ALLOC_MAGIC;
		ac_obj->dma_addr = GEM_NON_ALLOC_MAGIC;
		return 0;
	}

	if (!(dp_funs && dp_funs->dpu_mem_alloc)) {
		ac_obj->dma_attrs = DMA_ATTR_WRITE_COMBINE;

		if (!alloc_kmap)
			ac_obj->dma_attrs |= DMA_ATTR_NO_KERNEL_MAPPING;

		ac_obj->kvaddr = dma_alloc_attrs(obj->dev->dev,
						 obj->size,
						 &ac_obj->dma_addr,
						 GFP_KERNEL, ac_obj->dma_attrs);
		if (!ac_obj->kvaddr) {
			DRM_ERROR("failed to allocate %zu byte dma buffer",
				  obj->size);
			return -ENOMEM;
		}

	} else {
		ret =
		    dp_funs->dpu_mem_alloc(dp_dev->data,
					   (u64 *) & ac_obj->dma_addr,
					   &ac_obj->kvaddr, obj->size, 64,
					   "drm-gem");
		if (ret) {
			DRM_ERROR("failed to allocate %zu byte dma buffer",
				  obj->size);
			return -ENOMEM;
		}
	}

	DRM_DEBUG_DRIVER("kvaddr = 0x%p, dma_addr = 0x%llx, size = %zu\n",
			 ac_obj->kvaddr, (u64) ac_obj->dma_addr, obj->size);

	return 0;
}

static int ax_gem_alloc_buf(struct ax_gem_object *ac_obj, bool alloc_kmap)
{
	return ax_gem_alloc_dma(ac_obj, alloc_kmap);
}

static void ax_gem_free_dma(struct ax_gem_object *ac_obj)
{
	int ret;
	struct drm_gem_object *obj = &ac_obj->base;
	struct ax_drm_mgr *priv = ac_obj->priv;
	struct ax_display_dev *dp_dev = &priv->dp_mgr->display_dev[0];
	struct ax_display_funcs *dp_funs = dp_dev->display_funs;

	DRM_DEBUG_DRIVER("enter\n");

	if (ac_obj->flags == GEM_FLAGS_USE_BLK) {
		ac_obj->kvaddr = 0;
		ac_obj->dma_addr = 0;
		return;
	}

	if (!(dp_funs && dp_funs->dpu_mem_free)) {
		dma_free_attrs(obj->dev->dev, obj->size, ac_obj->kvaddr,
			       ac_obj->dma_addr, ac_obj->dma_attrs);
	} else {
		ret = dp_funs->dpu_mem_free(dp_dev->data,
					    (u64) ac_obj->dma_addr,
					    ac_obj->kvaddr);
		if (ret) {
			DRM_ERROR
			    ("failed to free dma buffer, dma_addr = 0x%llx, vaddr = 0x%px\n",
			     (u64) ac_obj->dma_addr, ac_obj->kvaddr);
		}
	}
}

static void ax_gem_free_buf(struct ax_gem_object *ac_obj)
{
	ax_gem_free_dma(ac_obj);
}

static int ax_drm_gem_object_mmap_dma(struct drm_gem_object *obj,
				      struct vm_area_struct *vma)
{
	struct ax_gem_object *ac_obj = to_ax_obj(obj);
	struct drm_device *drm_dev = obj->dev;

	return dma_mmap_attrs(drm_dev->dev, vma, ac_obj->kvaddr,
			      ac_obj->dma_addr, obj->size, ac_obj->dma_attrs);
}

static int ax_drm_gem_object_mmap(struct drm_gem_object *obj,
				  struct vm_area_struct *vma)
{
	int ret;

	/*
	 * We allocated a struct page table for ac_obj, so clear
	 * VM_PFNMAP flag that was set by drm_gem_mmap_obj()/drm_gem_mmap().
	 */
	vma->vm_flags &= ~VM_PFNMAP;

	ret = ax_drm_gem_object_mmap_dma(obj, vma);
	if (ret)
		drm_gem_vm_close(vma);

	return ret;
}

int ax_gem_mmap_buf(struct drm_gem_object *obj, struct vm_area_struct *vma)
{
	int ret;

	ret = drm_gem_mmap_obj(obj, obj->size, vma);
	if (ret)
		return ret;

	return ax_drm_gem_object_mmap(obj, vma);
}

/* drm driver mmap file operations */
int ax_gem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct drm_gem_object *obj;
	int ret;

	ret = drm_gem_mmap(filp, vma);
	if (ret)
		return ret;

	/*
	 * Set vm_pgoff (used as a fake buffer offset by DRM) to 0 and map the
	 * whole buffer from the start.
	 */
	vma->vm_pgoff = 0;

	obj = vma->vm_private_data;

	return ax_drm_gem_object_mmap(obj, vma);
}

static void ax_gem_release_object(struct ax_gem_object *ac_obj)
{
	drm_gem_object_release(&ac_obj->base);
	kfree(ac_obj);
}

struct ax_gem_object *ax_gem_alloc_object(struct drm_device *drm_dev,
					  unsigned int size, unsigned int flags)
{
	struct ax_gem_object *ac_obj;
	struct drm_gem_object *obj;

	size = round_up(size, PAGE_SIZE);

	ac_obj = kzalloc(sizeof(*ac_obj), GFP_KERNEL);
	if (!ac_obj)
		return ERR_PTR(-ENOMEM);

	obj = &ac_obj->base;
	ac_obj->flags = flags;
	ac_obj->priv = drm_dev->dev_private;

	drm_gem_object_init(drm_dev, obj, size);

	return ac_obj;
}

static struct ax_gem_object *__ax_gem_create_object(struct drm_device *drm_dev,
						    unsigned int size,
						    unsigned int flags,
						    bool alloc_kmap)
{
	struct ax_gem_object *ac_obj;
	int ret;

	DRM_DEBUG_DRIVER("enter, size = 0x%x, flags = 0x%x\n", size, flags);

	ac_obj = ax_gem_alloc_object(drm_dev, size, flags);
	if (IS_ERR(ac_obj))
		return ac_obj;

	ret = ax_gem_alloc_buf(ac_obj, alloc_kmap);
	if (ret)
		goto err_free_rk_obj;

	return ac_obj;

err_free_rk_obj:
	ax_gem_release_object(ac_obj);
	return ERR_PTR(ret);
}

struct ax_gem_object *ax_gem_create_object(struct drm_device *drm_dev,
					   unsigned int size, bool alloc_kmap)
{
	return __ax_gem_create_object(drm_dev, size, 0, alloc_kmap);
}

/*
 * ax_gem_free_object - (struct drm_driver)->gem_free_object_unlocked
 * callback function
 */
void ax_gem_free_object(struct drm_gem_object *obj)
{
	struct ax_gem_object *ac_obj = to_ax_obj(obj);

	DRM_DEBUG_DRIVER("enter\n");

	ax_gem_free_buf(ac_obj);

	ax_gem_release_object(ac_obj);
}

/*
 * ax_gem_create_with_handle - allocate an object with the given
 * size and create a gem handle on it
 *
 * returns a struct ax_gem_object* on success or ERR_PTR values
 * on failure.
 */
static struct ax_gem_object *ax_gem_create_with_handle(struct drm_file *file_priv,
						       struct drm_device *drm_dev,
						       unsigned int size,
						       unsigned int flags,
						       unsigned int *handle)
{
	struct ax_gem_object *ac_obj;
	struct drm_gem_object *obj;
	int ret;

	ac_obj = __ax_gem_create_object(drm_dev, size, flags, false);
	if (IS_ERR(ac_obj))
		return ERR_CAST(ac_obj);

	obj = &ac_obj->base;

	/*
	 * allocate a id of idr table where the obj is registered
	 * and handle has the id what user can see.
	 */
	ret = drm_gem_handle_create(file_priv, obj, handle);
	if (ret)
		goto err_handle_create;

	/* drop reference from allocate - handle holds it now. */
	drm_gem_object_put_unlocked(obj);

	return ac_obj;

err_handle_create:
	ax_gem_free_object(obj);

	return ERR_PTR(ret);
}

/*
 * ax_gem_dumb_create - (struct drm_driver)->dumb_create callback
 * function
 *
 * This aligns the pitch and size arguments to the minimum required. wrap
 * this into your own function if you need bigger alignment.
 */
int ax_gem_dumb_create(struct drm_file *file_priv, struct drm_device *dev,
		       struct drm_mode_create_dumb *args)
{
	struct ax_gem_object *ax_obj;
	int min_pitch = DIV_ROUND_UP(args->width * args->bpp, 8);

	/*
	 * align to 16 bytes since dpu requires it.
	 */
	args->pitch = ALIGN(min_pitch, 16);
	args->size = args->pitch * args->height;

	ax_obj = ax_gem_create_with_handle(file_priv, dev, args->size, args->flags,
				      &args->handle);

	DRM_DEBUG_DRIVER("enter, obj: 0x%px\n", &ax_obj->base);

	return PTR_ERR_OR_ZERO(ax_obj);
}

void *ax_gem_prime_vmap(struct drm_gem_object *obj)
{
	struct ax_gem_object *ac_obj = to_ax_obj(obj);

	if (ac_obj->dma_attrs & DMA_ATTR_NO_KERNEL_MAPPING)
		return NULL;

	return ac_obj->kvaddr;
}
