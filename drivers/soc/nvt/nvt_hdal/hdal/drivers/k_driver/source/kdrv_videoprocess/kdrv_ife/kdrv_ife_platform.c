#if defined(__LINUX)
#include <linux/slab.h>
#elif defined(__FREERTOS)
#include <malloc.h>
#endif
#include "kwrap/type.h"

void *kdrv_ife_os_malloc_wrap(UINT32 want_size)
{
	void *p_buf = NULL;

#if defined(__LINUX)
	p_buf = kmalloc(want_size, GFP_KERNEL);
#elif defined(__FREERTOS)
	p_buf = malloc(want_size);
#endif

	return p_buf;
}

void kdrv_ife_os_mfree_wrap(void *p_buf)
{
#if defined(__LINUX)
	kfree(p_buf);
#elif defined(__FREERTOS)
	free(p_buf);
#endif
}

UINT32 kdrv_ife_do_div(UINT64 n, UINT64 base)
{
	UINT32 rt=0;
#if defined(__LINUX)
	do_div(n, base);
#elif defined(__FREERTOS)
	n = n / base;
#endif

	rt = (UINT32) n;

	return rt;
}
