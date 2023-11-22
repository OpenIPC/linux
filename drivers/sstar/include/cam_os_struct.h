/*
 * cam_os_struct.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#ifndef __CAM_OS_STRUCT_H__
#define __CAM_OS_STRUCT_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "cam_os_util.h"

#if defined(__KERNEL__)
#define CAM_OS_LINUX_KERNEL
#endif

#ifdef CAM_OS_RTK

#include "sys_MsWrapper_cus_os_sem.h"
#include "sys_MsWrapper_cus_os_timer.h"

#define CAM_OS_MUTEX_SIZE \
    CAM_OS_ALIGN_UP(sizeof(Ms_Mutex_t) + sizeof(unsigned long), sizeof(unsigned long)) / sizeof(unsigned long)
#define CAM_OS_TSEM_SIZE \
    CAM_OS_ALIGN_UP(sizeof(Ms_DynSemaphor_t) + sizeof(unsigned long), sizeof(unsigned long)) / sizeof(unsigned long)
#define CAM_OS_RWTSEM_SIZE                                                                                \
    CAM_OS_ALIGN_UP(sizeof(long) + sizeof(Ms_Mutex_t) + sizeof(Ms_DynSemaphor_t) + sizeof(unsigned long), \
                    sizeof(unsigned long))                                                                \
        / sizeof(unsigned long)
#define CAM_OS_SPINLOCK_SIZE \
    CAM_OS_ALIGN_UP(sizeof(unsigned long) + sizeof(unsigned long), sizeof(unsigned long)) / sizeof(unsigned long)
#define CAM_OS_TIMER_SIZE    CAM_OS_ALIGN_UP(sizeof(MsTimerExt_t), sizeof(unsigned long)) / sizeof(unsigned long)
#define CAM_OS_MEMCACHE_SIZE CAM_OS_ALIGN_UP(sizeof(u8) + sizeof(u32), sizeof(unsigned long)) / sizeof(unsigned long)
#define CAM_OS_IDR_SIZE \
    CAM_OS_ALIGN_UP(sizeof(void **) + sizeof(u32 *) + sizeof(u32), sizeof(unsigned long)) / sizeof(unsigned long)
#define CAM_OS_CPUMASK_SIZE CAM_OS_ALIGN_UP(sizeof(u32), sizeof(unsigned long)) / sizeof(unsigned long)

#elif defined(CAM_OS_LINUX_KERNEL)

#include <linux/mutex.h>
#include <linux/rtmutex.h>
#include <linux/semaphore.h>
#include <linux/rwsem.h>
#include <linux/completion.h>
#include <linux/spinlock_types.h>
#include <linux/timer.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/slab_def.h>
#include <linux/idr.h>
#include <linux/version.h>

#define CAM_OS_MUTEX_SIZE                                                                              \
    CAM_OS_ALIGN_UP(sizeof(struct mutex) + sizeof(struct lock_class_key) + sizeof(unsigned long long), \
                    sizeof(unsigned long long))                                                        \
        / sizeof(unsigned long)
#define CAM_OS_TSEM_SIZE \
    CAM_OS_ALIGN_UP(sizeof(struct semaphore) + sizeof(unsigned long), sizeof(unsigned long)) / sizeof(unsigned long)
#define CAM_OS_RWTSEM_SIZE                                                                               \
    CAM_OS_ALIGN_UP(sizeof(struct rw_semaphore) + sizeof(struct lock_class_key) + sizeof(unsigned long), \
                    sizeof(unsigned long long))                                                          \
        / sizeof(unsigned long)
#define CAM_OS_SPINLOCK_SIZE                                                                   \
    CAM_OS_ALIGN_UP(sizeof(spinlock_t) + sizeof(unsigned long) + sizeof(struct lock_class_key) \
                        + sizeof(unsigned long),                                               \
                    sizeof(unsigned long))                                                     \
        / sizeof(unsigned long)
#define CAM_OS_RTMUTEX_SIZE                                                                               \
    CAM_OS_ALIGN_UP(sizeof(struct rt_mutex) + sizeof(struct lock_class_key) + sizeof(unsigned long long), \
                    sizeof(unsigned long long))                                                           \
        / sizeof(unsigned long)
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
#define CAM_OS_TIMER_SIZE CAM_OS_ALIGN_UP(sizeof(struct timer_list), sizeof(unsigned long)) / sizeof(unsigned long)
#else
#define CAM_OS_TIMER_SIZE                                                                               \
    CAM_OS_ALIGN_UP(sizeof(struct timer_list) + sizeof(void *) + sizeof(void *), sizeof(unsigned long)) \
        / sizeof(unsigned long)
#endif
#define CAM_OS_MEMCACHE_SIZE \
    CAM_OS_ALIGN_UP(sizeof(struct kmem_cache *) + 16, sizeof(unsigned long)) / sizeof(unsigned long)
#define CAM_OS_IDR_SIZE     CAM_OS_ALIGN_UP(sizeof(struct idr), sizeof(unsigned long)) / sizeof(unsigned long)
#define CAM_OS_CPUMASK_SIZE CAM_OS_ALIGN_UP(sizeof(struct cpumask), sizeof(unsigned long)) / sizeof(unsigned long)

#else

#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <stdint.h>

#define CAM_OS_MUTEX_SIZE \
    CAM_OS_ALIGN_UP(sizeof(pthread_mutex_t) + sizeof(unsigned long), sizeof(unsigned long)) / sizeof(unsigned long)
#define CAM_OS_TSEM_SIZE \
    CAM_OS_ALIGN_UP(sizeof(sem_t) + sizeof(unsigned long), sizeof(unsigned long)) / sizeof(unsigned long)
#define CAM_OS_RWTSEM_SIZE \
    CAM_OS_ALIGN_UP(sizeof(pthread_rwlock_t) + sizeof(unsigned long), sizeof(unsigned long)) / sizeof(unsigned long)
#define CAM_OS_SPINLOCK_SIZE \
    CAM_OS_ALIGN_UP(sizeof(pthread_spinlock_t) + sizeof(unsigned long), sizeof(unsigned long)) / sizeof(unsigned long)
#define CAM_OS_RTMUTEX_SIZE \
    CAM_OS_ALIGN_UP(sizeof(pthread_mutex_t) + sizeof(unsigned long), sizeof(unsigned long)) / sizeof(unsigned long)
#define CAM_OS_TIMER_SIZE    CAM_OS_ALIGN_UP(sizeof(timer_t), sizeof(unsigned long)) / sizeof(unsigned long)
#define CAM_OS_MEMCACHE_SIZE 0
#define CAM_OS_IDR_SIZE                                                                                       \
    CAM_OS_ALIGN_UP(sizeof(void **) + sizeof(unsigned long *) + sizeof(unsigned long), sizeof(unsigned long)) \
        / sizeof(unsigned long)
#define CAM_OS_CPUMASK_SIZE CAM_OS_ALIGN_UP(sizeof(cpu_set_t), sizeof(unsigned long)) / sizeof(unsigned long)

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //__CAM_OS_STRUCT_H__
