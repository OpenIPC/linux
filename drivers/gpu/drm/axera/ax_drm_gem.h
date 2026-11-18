/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_DRM_GEM_H
#define __AX_DRM_GEM_H

#define to_ax_obj(x) container_of(x, struct ax_gem_object, base)

struct ax_gem_object {
	struct drm_gem_object base;
	unsigned int flags;

	void *kvaddr;
	dma_addr_t dma_addr;
	unsigned long dma_attrs;

	void *priv;
};

void *ax_gem_prime_vmap(struct drm_gem_object *obj);
void ax_gem_prime_vunmap(struct drm_gem_object *obj, void *vaddr);

/* drm driver mmap file operations */
int ax_gem_mmap(struct file *filp, struct vm_area_struct *vma);

/* mmap a gem object to userspace. */
int ax_gem_mmap_buf(struct drm_gem_object *obj, struct vm_area_struct *vma);

struct ax_gem_object *ax_gem_create_object(struct drm_device *drm,
					   unsigned int size, bool alloc_kmap);

void ax_gem_free_object(struct drm_gem_object *obj);

int ax_gem_dumb_create(struct drm_file *file_priv,
		       struct drm_device *dev,
		       struct drm_mode_create_dumb *args);

#endif /* __AX_DRM_GEM_H */
