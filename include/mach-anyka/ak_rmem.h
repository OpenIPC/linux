#ifndef __MACH_ANYKA_AK_RMEM_H
#define __MACH_ANYKA_AK_RMEM_H

#include <linux/errno.h>
#include <linux/types.h>

struct device;

#ifdef CONFIG_AK_RMEM
void ak_rmem_reserve(void);
int ak_rmem_device_init(struct device *dev, phys_addr_t *base, size_t *size);
void ak_rmem_device_release(struct device *dev);
#else
static inline void ak_rmem_reserve(void) { }
static inline int ak_rmem_device_init(struct device *dev, phys_addr_t *base,
				      size_t *size)
{
	return -ENODEV;
}
static inline void ak_rmem_device_release(struct device *dev) { }
#endif

#endif
