#include "isf_vdoout_platform.h"
#include <linux/slab.h>

void* vdoout_alloc(int size)
{
	return kmalloc(size, GFP_ATOMIC);
}

void vdoout_free(void *buf)
{
	kfree(buf);
}