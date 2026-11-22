/*
 * Coherent per-device memory handling.
 * Borrowed from i386
 */
#include <linux/bitmap.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>

/* One "shared-dma-pool" region is a single bitmap shared by every device that
 * names it in memory-region, so a pool total cannot say which device grew.
 * Allocations past the last slot stay in the total, attributed to nobody. */
#define DMA_COHERENT_OWNERS	4

struct dma_coherent_owner {
	struct device	*dev;
	unsigned long	pages;
};

struct dma_coherent_mem {
	void		*virt_base;
	dma_addr_t	device_base;
	unsigned long	pfn_base;
	int		size;
	int		flags;
	unsigned long	*bitmap;
	/* first page of every allocation: a run ends at the next one */
	unsigned long	*starts;
	spinlock_t	spinlock;
	struct dma_coherent_owner owner[DMA_COHERENT_OWNERS];
};

/* Caller holds mem->spinlock. */
static void dma_coherent_account(struct dma_coherent_mem *mem,
				 struct device *dev, unsigned long pages,
				 bool freeing)
{
	int i;

	for (i = 0; i < DMA_COHERENT_OWNERS; i++) {
		if (mem->owner[i].dev == dev)
			break;
		if (!mem->owner[i].dev && !freeing) {
			mem->owner[i].dev = dev;
			break;
		}
	}
	if (i == DMA_COHERENT_OWNERS)
		return;
	if (!freeing)
		mem->owner[i].pages += pages;
	else if (mem->owner[i].pages > pages)
		mem->owner[i].pages -= pages;
	else
		mem->owner[i].pages = 0;
}

static int dma_init_coherent_memory(phys_addr_t phys_addr, dma_addr_t device_addr,
			     size_t size, int flags,
			     struct dma_coherent_mem **mem)
{
	struct dma_coherent_mem *dma_mem = NULL;
	void __iomem *mem_base = NULL;
	int pages = size >> PAGE_SHIFT;
	int bitmap_size = BITS_TO_LONGS(pages) * sizeof(long);

	if ((flags & (DMA_MEMORY_MAP | DMA_MEMORY_IO)) == 0)
		goto out;
	if (!size)
		goto out;

	mem_base = ioremap(phys_addr, size);
	if (!mem_base)
		goto out;

	dma_mem = kzalloc(sizeof(struct dma_coherent_mem), GFP_KERNEL);
	if (!dma_mem)
		goto out;
	dma_mem->bitmap = kzalloc(bitmap_size, GFP_KERNEL);
	if (!dma_mem->bitmap)
		goto out;
	dma_mem->starts = kzalloc(bitmap_size, GFP_KERNEL);
	if (!dma_mem->starts)
		goto out;

	dma_mem->virt_base = mem_base;
	dma_mem->device_base = device_addr;
	dma_mem->pfn_base = PFN_DOWN(phys_addr);
	dma_mem->size = pages;
	dma_mem->flags = flags;
	spin_lock_init(&dma_mem->spinlock);

	*mem = dma_mem;

	if (flags & DMA_MEMORY_MAP)
		return DMA_MEMORY_MAP;

	return DMA_MEMORY_IO;

out:
	if (dma_mem) {
		kfree(dma_mem->bitmap);
		kfree(dma_mem->starts);
	}
	kfree(dma_mem);
	if (mem_base)
		iounmap(mem_base);
	return 0;
}

static void dma_release_coherent_memory(struct dma_coherent_mem *mem)
{
	if (!mem)
		return;
	iounmap(mem->virt_base);
	kfree(mem->bitmap);
	kfree(mem->starts);
	kfree(mem);
}

static int dma_assign_coherent_memory(struct device *dev,
				      struct dma_coherent_mem *mem)
{
	if (dev->dma_mem)
		return -EBUSY;

	dev->dma_mem = mem;
	/* FIXME: this routine just ignores DMA_MEMORY_INCLUDES_CHILDREN */

	return 0;
}

int dma_declare_coherent_memory(struct device *dev, phys_addr_t phys_addr,
				dma_addr_t device_addr, size_t size, int flags)
{
	struct dma_coherent_mem *mem;
	int ret;

	ret = dma_init_coherent_memory(phys_addr, device_addr, size, flags,
				       &mem);
	if (ret == 0)
		return 0;

	if (dma_assign_coherent_memory(dev, mem) == 0)
		return ret;

	dma_release_coherent_memory(mem);
	return 0;
}
EXPORT_SYMBOL(dma_declare_coherent_memory);

void dma_release_declared_memory(struct device *dev)
{
	struct dma_coherent_mem *mem = dev->dma_mem;

	if (!mem)
		return;
	dma_release_coherent_memory(mem);
	dev->dma_mem = NULL;
}
EXPORT_SYMBOL(dma_release_declared_memory);

void *dma_mark_declared_memory_occupied(struct device *dev,
					dma_addr_t device_addr, size_t size)
{
	struct dma_coherent_mem *mem = dev->dma_mem;
	unsigned long flags;
	int pos, pages, err = 0;

	size += device_addr & ~PAGE_MASK;
	pages = PAGE_ALIGN(size) >> PAGE_SHIFT;

	if (!mem)
		return ERR_PTR(-EINVAL);

	spin_lock_irqsave(&mem->spinlock, flags);
	pos = (device_addr - mem->device_base) >> PAGE_SHIFT;
	if (!pages) {
		err = -EINVAL;
	} else if (pos + pages > mem->size ||
		   find_next_bit(mem->bitmap, pos + pages, pos) < pos + pages) {
		err = -EBUSY;
	} else {
		bitmap_set(mem->bitmap, pos, pages);
		__set_bit(pos, mem->starts);
		dma_coherent_account(mem, dev, pages, false);
	}
	spin_unlock_irqrestore(&mem->spinlock, flags);

	if (err != 0)
		return ERR_PTR(err);
	return mem->virt_base + (pos << PAGE_SHIFT);
}
EXPORT_SYMBOL(dma_mark_declared_memory_occupied);

/**
 * dma_alloc_from_coherent() - try to allocate memory from the per-device coherent area
 *
 * @dev:	device from which we allocate memory
 * @size:	size of requested memory area
 * @dma_handle:	This will be filled with the correct dma handle
 * @ret:	This pointer will be filled with the virtual address
 *		to allocated area.
 *
 * This function should be only called from per-arch dma_alloc_coherent()
 * to support allocation from per-device coherent memory pools.
 *
 * Returns 0 if dma_alloc_coherent should continue with allocating from
 * generic memory areas, or !0 if dma_alloc_coherent should return @ret.
 */
int dma_alloc_from_coherent(struct device *dev, ssize_t size,
				       dma_addr_t *dma_handle, void **ret)
{
	struct dma_coherent_mem *mem;
	int pages = PAGE_ALIGN(size) >> PAGE_SHIFT;
	unsigned long flags;
	unsigned long pageno;

	if (!dev)
		return 0;
	mem = dev->dma_mem;
	if (!mem)
		return 0;

	*ret = NULL;
	spin_lock_irqsave(&mem->spinlock, flags);

	if (unlikely(size > (mem->size << PAGE_SHIFT) || pages <= 0))
		goto err;

	/*
	 * Exactly the pages asked for, page aligned, where the first run of
	 * them is free. A naturally aligned 1 << order run, as upstream takes,
	 * spends 4 MiB on a 2.4 MB frame: the 24 MiB EV200 video pool then
	 * holds 1280x720 capture and encoding, but not 1280x960, which needs
	 * 22.5 MB of it in exact sizes and 28 MiB in whole orders. Nothing
	 * the pool serves needs more than page alignment (ak-venc: 8 bytes).
	 */
	pageno = bitmap_find_next_zero_area(mem->bitmap, mem->size, 0,
					    pages, 0);
	if (unlikely(pageno >= mem->size))
		goto err;
	bitmap_set(mem->bitmap, pageno, pages);
	__set_bit(pageno, mem->starts);

	/*
	 * Memory was found in the per-device area.
	 */
	*dma_handle = mem->device_base + (pageno << PAGE_SHIFT);
	*ret = mem->virt_base + (pageno << PAGE_SHIFT);
	dma_coherent_account(mem, dev, pages, false);
	memset(*ret, 0, size);
	spin_unlock_irqrestore(&mem->spinlock, flags);

	return 1;

err:
	spin_unlock_irqrestore(&mem->spinlock, flags);
	/*
	 * In the case where the allocation can not be satisfied from the
	 * per-device area, try to fall back to generic memory if the
	 * constraints allow it.
	 */
	return mem->flags & DMA_MEMORY_EXCLUSIVE;
}
EXPORT_SYMBOL(dma_alloc_from_coherent);

/**
 * dma_release_from_coherent() - try to free the memory allocated from per-device coherent memory pool
 * @dev:	device from which the memory was allocated
 * @order:	the order of pages allocated
 * @vaddr:	virtual address of allocated pages
 *
 * This checks whether the memory was allocated from the per-device
 * coherent memory pool and if so, releases that memory.
 *
 * Returns 1 if we correctly released the memory, or 0 if
 * dma_release_coherent() should proceed with releasing memory from
 * generic pools.
 */
int dma_release_from_coherent(struct device *dev, int order, void *vaddr)
{
	struct dma_coherent_mem *mem = dev ? dev->dma_mem : NULL;

	if (mem && vaddr >= mem->virt_base && vaddr <
		   (mem->virt_base + (mem->size << PAGE_SHIFT))) {
		int page = (vaddr - mem->virt_base) >> PAGE_SHIFT;
		unsigned long flags, end, last;

		spin_lock_irqsave(&mem->spinlock, flags);
		if (WARN_ON_ONCE(!test_bit(page, mem->starts))) {
			spin_unlock_irqrestore(&mem->spinlock, flags);
			return 1;
		}
		/* the run is the allocation's: up to the next start or hole */
		last = min_t(unsigned long, mem->size, page + (1UL << order));
		for (end = page + 1; end < last; end++)
			if (!test_bit(end, mem->bitmap) ||
			    test_bit(end, mem->starts))
				break;
		bitmap_clear(mem->bitmap, page, end - page);
		__clear_bit(page, mem->starts);
		dma_coherent_account(mem, dev, end - page, true);
		spin_unlock_irqrestore(&mem->spinlock, flags);
		return 1;
	}
	return 0;
}
EXPORT_SYMBOL(dma_release_from_coherent);

/* dma_alloc_from_coherent() takes the first free run of the pages it needs,
 * so a free total does not predict whether a request can be served; the
 * longest free run does. Caller holds mem->spinlock. */
static unsigned long dma_coherent_largest_free(struct dma_coherent_mem *mem)
{
	unsigned long pos = 0, largest = 0;

	while ((pos = find_next_zero_bit(mem->bitmap, mem->size, pos)) <
	       mem->size) {
		unsigned long end = find_next_bit(mem->bitmap, mem->size, pos);

		largest = max(largest, end - pos);
		pos = end;
	}
	return largest;
}

static ssize_t dma_coherent_pool_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct dma_coherent_mem *mem = dev ? dev->dma_mem : NULL;
	struct dma_coherent_owner owner[DMA_COHERENT_OWNERS];
	unsigned long pages, used, largest, flags;
	ssize_t n = 0;
	int i;

	if (!mem)
		return -ENODEV;

	spin_lock_irqsave(&mem->spinlock, flags);
	pages = mem->size;
	used = bitmap_weight(mem->bitmap, mem->size);
	largest = dma_coherent_largest_free(mem);
	memcpy(owner, mem->owner, sizeof(owner));
	spin_unlock_irqrestore(&mem->spinlock, flags);

	n += scnprintf(buf + n, PAGE_SIZE - n, "size %lu\n", pages << PAGE_SHIFT);
	n += scnprintf(buf + n, PAGE_SIZE - n, "used %lu\n", used << PAGE_SHIFT);
	n += scnprintf(buf + n, PAGE_SIZE - n, "largest %lu\n",
		       largest << PAGE_SHIFT);
	for (i = 0; i < DMA_COHERENT_OWNERS; i++)
		if (owner[i].dev)
			n += scnprintf(buf + n, PAGE_SIZE - n, "owner %s %lu\n",
				       dev_name(owner[i].dev),
				       owner[i].pages << PAGE_SHIFT);
	return n;
}

struct device_attribute dev_attr_dma_coherent_pool =
	__ATTR_RO(dma_coherent_pool);
EXPORT_SYMBOL(dev_attr_dma_coherent_pool);

/**
 * dma_mmap_from_coherent() - try to mmap the memory allocated from
 * per-device coherent memory pool to userspace
 * @dev:	device from which the memory was allocated
 * @vma:	vm_area for the userspace memory
 * @vaddr:	cpu address returned by dma_alloc_from_coherent
 * @size:	size of the memory buffer allocated by dma_alloc_from_coherent
 * @ret:	result from remap_pfn_range()
 *
 * This checks whether the memory was allocated from the per-device
 * coherent memory pool and if so, maps that memory to the provided vma.
 *
 * Returns 1 if we correctly mapped the memory, or 0 if the caller should
 * proceed with mapping memory from generic pools.
 */
int dma_mmap_from_coherent(struct device *dev, struct vm_area_struct *vma,
			   void *vaddr, size_t size, int *ret)
{
	struct dma_coherent_mem *mem = dev ? dev->dma_mem : NULL;

	if (mem && vaddr >= mem->virt_base && vaddr + size <=
		   (mem->virt_base + (mem->size << PAGE_SHIFT))) {
		unsigned long off = vma->vm_pgoff;
		int start = (vaddr - mem->virt_base) >> PAGE_SHIFT;
		int user_count = (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;
		int count = size >> PAGE_SHIFT;

		*ret = -ENXIO;
		if (off < count && user_count <= count - off) {
			unsigned long pfn = mem->pfn_base + start + off;
			*ret = remap_pfn_range(vma, vma->vm_start, pfn,
					       user_count << PAGE_SHIFT,
					       vma->vm_page_prot);
		}
		return 1;
	}
	return 0;
}
EXPORT_SYMBOL(dma_mmap_from_coherent);

/*
 * Support for reserved memory regions defined in device tree
 */
#ifdef CONFIG_OF_RESERVED_MEM
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>

static int rmem_dma_device_init(struct reserved_mem *rmem, struct device *dev)
{
	struct dma_coherent_mem *mem = rmem->priv;

	if (!mem &&
	    dma_init_coherent_memory(rmem->base, rmem->base, rmem->size,
				     DMA_MEMORY_MAP | DMA_MEMORY_EXCLUSIVE,
				     &mem) != DMA_MEMORY_MAP) {
		pr_err("Reserved memory: failed to init DMA memory pool at %pa, size %ld MiB\n",
			&rmem->base, (unsigned long)rmem->size / SZ_1M);
		return -ENODEV;
	}
	rmem->priv = mem;
	dma_assign_coherent_memory(dev, mem);
	return 0;
}

static void rmem_dma_device_release(struct reserved_mem *rmem,
				    struct device *dev)
{
	dev->dma_mem = NULL;
}

static const struct reserved_mem_ops rmem_dma_ops = {
	.device_init	= rmem_dma_device_init,
	.device_release	= rmem_dma_device_release,
};

static int __init rmem_dma_setup(struct reserved_mem *rmem)
{
	unsigned long node = rmem->fdt_node;

	if (of_get_flat_dt_prop(node, "reusable", NULL))
		return -EINVAL;

#ifdef CONFIG_ARM
	if (!of_get_flat_dt_prop(node, "no-map", NULL)) {
		pr_err("Reserved memory: regions without no-map are not yet supported\n");
		return -EINVAL;
	}
#endif

	rmem->ops = &rmem_dma_ops;
	pr_info("Reserved memory: created DMA memory pool at %pa, size %ld MiB\n",
		&rmem->base, (unsigned long)rmem->size / SZ_1M);
	return 0;
}
RESERVEDMEM_OF_DECLARE(dma, "shared-dma-pool", rmem_dma_setup);
#endif
