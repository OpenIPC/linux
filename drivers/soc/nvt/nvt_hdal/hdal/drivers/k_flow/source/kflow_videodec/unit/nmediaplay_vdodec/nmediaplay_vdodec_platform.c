#include "nmediaplay_vdodec_platform.h"
#include "kwrap/spinlock.h"
#include <linux/slab.h>

static VK_DEFINE_SPINLOCK(vdec_lock);

void NMP_VdoDec_Lock_cpu(unsigned long *in_flags)
{
	unsigned long flags=0;
	vk_spin_lock_irqsave(&vdec_lock, flags);
	*in_flags = flags;
}

void NMP_VdoDec_Unlock_cpu(unsigned long *in_flags)
{
	vk_spin_unlock_irqrestore(&vdec_lock, *in_flags);
}

void* NMP_VdoDec_Alloc(int size)
{
	return kmalloc(size, GFP_ATOMIC);
}

void NMP_VdoDec_Free(void *buf)
{
	kfree(buf);
}