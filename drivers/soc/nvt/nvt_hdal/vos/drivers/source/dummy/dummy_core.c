#include <kwrap/debug.h>
#include <kwrap/spinlock.h>

VK_DEFINE_SPINLOCK(dummy_test_lock);

int dummy_core(void)
{
	unsigned long flags;

	DBG_DUMP("sizeof(dummy_test_lock) = %ld\r\n", (ULONG)sizeof(dummy_test_lock));

	vk_spin_lock_irqsave(&dummy_test_lock, flags);
	vk_spin_unlock_irqrestore(&dummy_test_lock, flags);

	return 0;
}