#if defined (__LINUX)
#include <linux/slab.h>
#elif defined (__FREERTOS)
#include <malloc.h>
#endif
#include "kdrv_ise_int.h"

void* kdrv_ise_platform_malloc(UINT32 size)
{
	void *p_buf = NULL;

#if defined(__LINUX)
	p_buf = kmalloc(size, GFP_KERNEL);
#elif defined(__FREERTOS)
	p_buf = malloc(size);
#endif

	return p_buf;
}

void kdrv_ise_platform_free(void *ptr)
{
#if defined(__LINUX)
	kfree(ptr);
#elif defined(__FREERTOS)
	free(ptr);
#endif
}