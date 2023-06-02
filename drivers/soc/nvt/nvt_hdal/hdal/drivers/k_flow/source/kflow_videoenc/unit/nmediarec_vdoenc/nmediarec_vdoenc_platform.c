#include "nmediarec_vdoenc_platform.h"
#include "kwrap/spinlock.h"

static VK_DEFINE_SPINLOCK(my_lock);
static VK_DEFINE_SPINLOCK(my_lock2);

void NMR_VdoEnc_Lock_cpu(unsigned long *in_flags)
{
	unsigned long flags=0;
	vk_spin_lock_irqsave(&my_lock, flags);
	*in_flags = flags;
}

void NMR_VdoEnc_Unlock_cpu(unsigned long *in_flags)
{
	vk_spin_unlock_irqrestore(&my_lock, *in_flags);
}

void NMR_VdoEnc_Lock_cpu2(unsigned long *in_flags)
{
	unsigned long flags=0;
	vk_spin_lock_irqsave(&my_lock2, flags);
	*in_flags = flags;
}

void NMR_VdoEnc_Unlock_cpu2(unsigned long *in_flags)
{
	vk_spin_unlock_irqrestore(&my_lock2, *in_flags);
}