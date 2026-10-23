/* SPDX-License-Identifier: GPL-2.0 */
/*
 * From-source frame-buffer allocator for the AX630C VC8000E VCMD driver (#45).
 *
 * Manages a physically-contiguous CMM carveout (outside kernel-managed DRAM)
 * and hands userspace bus addresses for encoder frame buffers, replacing the
 * fixed-address /dev/mem placement of Stage B. The kernel never maps the
 * memory: only the encoder DMAs it and userspace mmaps it (writecombine)
 * through the driver, so the allocator is pure address-space bookkeeping.
 *
 * ABI (added to the Hantro ioctl surface; 36/37 are unused upstream):
 *   HANTRO_IOCH_ALLOC_FRAMEBUF  in: size          out: bus_addr
 *   HANTRO_IOCH_FREE_FRAMEBUF   in: bus_addr      (owner-checked)
 * Allocations are owned by the open file and freed automatically on close.
 * mmap offset = bus_addr, length <= allocation size.
 */
#ifndef FRAMEBUF_ALLOC_H
#define FRAMEBUF_ALLOC_H

#include <linux/ioctl.h>

struct framebuf_parameter {
	unsigned long size;      /* in: bytes (page-rounded internally) */
	unsigned long bus_addr;  /* out (alloc) / in (free) */
};

#define HANTRO_IOCH_ALLOC_FRAMEBUF _IOWR('k', 36, struct framebuf_parameter)
#define HANTRO_IOCH_FREE_FRAMEBUF  _IOWR('k', 37, struct framebuf_parameter)

/*
 * dma-buf import (#60 M3): resolve a dma-buf fd -- in practice a V4L2
 * EXPBUF export from the open capture driver -- to the bus address the
 * encoder register program consumes (zero-copy capture -> VC8000E input).
 * The buffer must be one physically contiguous run. Imports are owned by
 * the open file and dropped automatically on close.
 *   HANTRO_IOCH_IMPORT_DMABUF   in: fd            out: bus_addr, size
 *   HANTRO_IOCH_RELEASE_DMABUF  in: bus_addr      (owner-checked)
 */
struct dmabuf_import_parameter {
	int fd;                  /* in: dma-buf file descriptor */
	unsigned long bus_addr;  /* out (import) / in (release) */
	unsigned long size;      /* out: bytes */
};

#define HANTRO_IOCH_IMPORT_DMABUF  _IOWR('k', 38, struct dmabuf_import_parameter)
#define HANTRO_IOCH_RELEASE_DMABUF _IOWR('k', 39, struct dmabuf_import_parameter)

#ifdef __KERNEL__
struct file;
struct device;

int vcmd_fb_alloc(struct file *filp, unsigned long size, unsigned long *bus);
int vcmd_fb_free(struct file *filp, unsigned long bus);
/* free everything owned by filp (device close): allocations AND imports */
void vcmd_fb_release_filp(struct file *filp);
/* the device dma-buf attachments are made on behalf of (the venc pdev) */
void vcmd_fb_set_dev(struct device *dev);
/* the reserved-memory carveout this allocator hands out, from DT (#83) */
void vcmd_fb_set_region(unsigned long base, unsigned long size);
int vcmd_dmabuf_import(struct file *filp, int fd, unsigned long *bus,
		       unsigned long *size);
int vcmd_dmabuf_release(struct file *filp, unsigned long bus);
/* mmap support: 0 + size out if phy is an allocation's base */
int vcmd_fb_lookup(unsigned long phy, unsigned long *size);
#endif

#endif /* FRAMEBUF_ALLOC_H */
