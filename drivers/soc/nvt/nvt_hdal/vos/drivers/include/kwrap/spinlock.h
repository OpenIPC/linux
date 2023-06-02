#ifndef _VOS_SPINLOCK_H_
#define _VOS_SPINLOCK_H_
#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include <kwrap/nvt_type.h>

#if (defined(__UITRON) || defined(__ECOS))

#elif defined(__FREERTOS)

#define RTOS_SPINLOCK_INITED_TAG MAKEFOURCC('R', 'S', 'P', 'N')

// Types Declarations
typedef struct {
	UINT32 init_tag;
	UINT32 is_lock;
} raw_spinlock_t;

typedef raw_spinlock_t vk_spinlock_t;

// Interface Function Prototype
#define __VK_SPIN_LOCK_INITIALIZER(lock) { .init_tag = RTOS_SPINLOCK_INITED_TAG, .is_lock = 0 }
#define __VK_SPIN_LOCK_UNLOCKED(lock) __VK_SPIN_LOCK_INITIALIZER(lock)
#define VK_DEFINE_SPINLOCK(x) vk_spinlock_t x = __VK_SPIN_LOCK_UNLOCKED(x)
#define VK_DEFINE_RAW_SPINLOCK(x) raw_spinlock_t x = __VK_SPIN_LOCK_UNLOCKED(x)

unsigned long _vk_raw_spin_lock_irqsave(vk_spinlock_t *lock);

void vk_spin_lock_init(vk_spinlock_t *lock);
void vk_spin_unlock_irqrestore(vk_spinlock_t *lock, unsigned long flags);
#define vk_spin_lock_irqsave(lock, flags)   \
do {                                        \
    flags = _vk_raw_spin_lock_irqsave(lock);\
} while (0)

#elif defined(__LINUX) && defined(__KERNEL__)

typedef struct vk_spinlock {
	unsigned int init_tag;
	unsigned int buf[15];
} vk_spinlock_t;

#define __VK_SPIN_LOCK_INITIALIZER(lock) { .init_tag = 0, .buf = {0} }
#define __VK_SPIN_LOCK_UNLOCKED(lock) __VK_SPIN_LOCK_INITIALIZER(lock)
#define VK_DEFINE_SPINLOCK(x) vk_spinlock_t x = __VK_SPIN_LOCK_UNLOCKED(x)

//NOTE: Do NOT use _vk_raw_XXX functions directly, these are used by vk_spin_XXX macro
unsigned long _vk_raw_spin_lock_irqsave(vk_spinlock_t *p_voslock);

void vk_spin_lock_init(vk_spinlock_t *p_voslock);
void vk_spin_unlock_irqrestore(vk_spinlock_t *p_voslock, unsigned long flags);
#define vk_spin_lock_irqsave(p_voslock, flags) \
do { \
    flags = _vk_raw_spin_lock_irqsave(p_voslock); \
} while (0)

void vk_spin_dump_list(void); //for debug, default disabled

#else
#error Not supported OS
#endif


#ifdef __cplusplus
}
#endif

#endif /* _VOS_SPINLOCK_H_ */

