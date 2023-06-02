#if defined(__LINUX)
#include <linux/slab.h>
#elif defined(__FREERTOS)
#include <malloc.h>
#endif
#include "kwrap/type.h"

void *kdrv_ipe_os_malloc_wrap(UINT32 want_size)
{
	void *p_buf = NULL;

#if defined(__LINUX)
	p_buf = kmalloc(want_size, GFP_KERNEL);
#elif defined(__FREERTOS)
	p_buf = malloc(want_size);
#endif

	return p_buf;
}

void kdrv_ipe_os_mfree_wrap(void *p_buf)
{
#if defined(__LINUX)
	kfree(p_buf);
#elif defined(__FREERTOS)
	free(p_buf);
#endif
}
