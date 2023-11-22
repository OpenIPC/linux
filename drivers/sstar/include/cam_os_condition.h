/*
 * cam_os_condition.h- Sigmastar
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

#ifndef __CAM_OS_CONDITION_H__
#define __CAM_OS_CONDITION_H__

#include "cam_os_wrapper.h"

#if defined(__KERNEL__)
#define CAM_OS_LINUX_KERNEL
#endif

#if defined(CAM_OS_RTK)
#include "sys_MsWrapper_cus_os_sem.h"
typedef Ms_DynSemaphor_t CamOsCondition_t;
#elif defined(CAM_OS_LINUX_KERNEL)
#include "linux/wait.h"
typedef wait_queue_head_t CamOsCondition_t;
#else // CAM_OS_LINUX_USER
#include "pthread.h"
typedef struct
{
    pthread_mutex_t tMutex;
    pthread_cond_t  tCondition;
} CamOsCondition_t;
#endif

#if defined(CAM_OS_RTK)
#define CamOsConditionInit(ptCondition) MsCreateDynSem(ptCondition, 0);
#elif defined(CAM_OS_LINUX_KERNEL)
#define CamOsConditionInit(ptCondition) init_waitqueue_head(ptCondition);
#else // CAM_OS_LINUX_USER
#define CamOsConditionInit(ptCondition)                         \
    (                                                           \
        {                                                       \
            pthread_condattr_t cattr;                           \
            pthread_condattr_init(&cattr);                      \
            pthread_condattr_setclock(&cattr, CLOCK_MONOTONIC); \
            pthread_cond_init(ptCondition.tCondition, &cattr);  \
            pthread_mutex_init(ptCondition.tMutex, NULL);       \
        })
#endif

#if defined(CAM_OS_RTK)
#define CamOsConditionDeinit(ptCondition) MsDestroyDynSem(ptCondition)
#elif defined(CAM_OS_LINUX_KERNEL)
#define CamOsConditionDeinit(ptCondition)
#else // CAM_OS_LINUX_USER
#define CamOsConditionDeinit(ptCondition)                 \
    (                                                     \
        {                                                 \
            pthread_cond_destroy(ptCondition.tCondition); \
            pthread_mutex_destroy(ptCondition.tMutex);    \
        })
#endif

#if defined(CAM_OS_RTK)
#define CamOsConditionWakeUpAll(ptCondition) MsProduceSafeDynSem(ptCondition, 1);
#elif defined(CAM_OS_LINUX_KERNEL)
#define CamOsConditionWakeUpAll(ptCondition) wake_up_all(ptCondition);
#else // CAM_OS_LINUX_USER
#define CamOsConditionWakeUpAll(ptCondition)                \
    (                                                       \
        {                                                   \
            pthread_mutex_lock(ptCondition.tMutex);         \
            pthread_cond_broadcast(ptCondition.tCondition); \
            pthread_mutex_unlock(ptCondition.tMutex);       \
        })
#endif

#if defined(CAM_OS_RTK)
#define CamOsConditionWait(ptCondition, condition) \
    (                                              \
        {                                          \
            CamOsRet_e __eRet = CAM_OS_OK;         \
            while (!(condition))                   \
                MsConsumeDynSem(ptCondition);      \
            __eRet;                                \
        })
#elif defined(CAM_OS_LINUX_KERNEL)
#define CamOsConditionWait(ptCondition, condition)   \
    (                                                \
        {                                            \
            CamOsRet_e __eRet = CAM_OS_OK;           \
            wait_event((*(ptCondition)), condition); \
            __eRet;                                  \
        })
#else // CAM_OS_LINUX_USER
#define CamOsConditionWait(ptCondition, condition)                             \
    (                                                                          \
        {                                                                      \
            CamOsRet_e __eRet = CAM_OS_OK;                                     \
            pthread_mutex_lock(ptCondition.tMutex);                            \
            while (!(condition))                                               \
                pthread_cond_wait(ptCondition.tCondition, ptCondition.tMutex); \
            pthread_mutex_unlock(ptCondition.tMutex);                          \
            __eRet;                                                            \
        })
#endif

#if defined(CAM_OS_LINUX_KERNEL)
#define CamOsConditionTimedWait(ptCondition, condition, nMsec)                             \
    (                                                                                      \
        {                                                                                  \
            CamOsRet_e __eRet = CAM_OS_OK;                                                 \
            if (!wait_event_timeout((*(ptCondition)), condition, msecs_to_jiffies(nMsec))) \
            {                                                                              \
                __eRet = CAM_OS_TIMEOUT;                                                   \
            }                                                                              \
            __eRet;                                                                        \
        })
#elif defined(CAM_OS_RTK)
#define __CamOsConditionTimedWait(ptCondition, condition, timeout_ms)           \
    (                                                                           \
        {                                                                       \
            unsigned long      __ret         = timeout_ms;                      \
            unsigned long long __target_time = CamOsGetTimeInMs() + timeout_ms; \
            while (!(condition))                                                \
            {                                                                   \
                if (MS_NO_MESSAGE == MsConsumeDynSemDelay(ptCondition, __ret))  \
                {                                                               \
                    __ret = (condition);                                        \
                    break;                                                      \
                }                                                               \
                __ret = (unsigned long)(__target_time - CamOsGetTimeInMs());    \
            }                                                                   \
            __ret;                                                              \
        })

#define CamOsConditionTimedWait(ptCondition, condition, nMsec)             \
    (                                                                      \
        {                                                                  \
            CamOsRet_e __eRet = CAM_OS_OK;                                 \
            if (!__CamOsConditionTimedWait(ptCondition, condition, nMsec)) \
            {                                                              \
                __eRet = CAM_OS_TIMEOUT;                                   \
            }                                                              \
            __eRet;                                                        \
        })
#else // CAM_OS_LINUX_USER
#define __CamOsConditionTimedWait(ptCondition, condition, timeout_ms)                                   \
    (                                                                                                   \
        {                                                                                               \
            int             __ret = 1;                                                                  \
            struct timespec max_wait;                                                                   \
            s64             nano_sec = 0;                                                               \
            clock_gettime(CLOCK_MONOTONIC, &max_wait);                                                  \
            nano_sec = (timeout_ms * 1000000LL) + max_wait.tv_nsec;                                     \
            max_wait.tv_sec += (nano_sec / 1000000000LL);                                               \
            max_wait.tv_nsec = nano_sec % 1000000000LL;                                                 \
            pthread_mutex_lock(ptCondition.tMutex);                                                     \
            while (!(condition))                                                                        \
            {                                                                                           \
                if (0 != pthread_cond_timedwait(ptCondition.tCondition, ptCondition.tMutex, &max_wait)) \
                {                                                                                       \
                    __ret = (condition);                                                                \
                    break;                                                                              \
                }                                                                                       \
            }                                                                                           \
            pthread_mutex_unlock(ptCondition.tMutex);                                                   \
            __ret;                                                                                      \
        })

#define CamOsConditionTimedWait(ptCondition, condition, nMsec)             \
    (                                                                      \
        {                                                                  \
            CamOsRet_e __eRet = CAM_OS_OK;                                 \
            if (!__CamOsConditionTimedWait(ptCondition, condition, nMsec)) \
            {                                                              \
                __eRet = CAM_OS_TIMEOUT;                                   \
            }                                                              \
            __eRet;                                                        \
        })
#endif

#endif //__CAM_OS_CONDITION_H__
