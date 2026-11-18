/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __OSAL_USER_AX__H__
#define __OSAL_USER_AX__H__

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_type_ax.h"

#define AX_USED __attribute__((used))

#ifdef CHIP_AX170
    #define GFP_KERNEL          0
    #define THREAD_PRIORITY     9
#else
    #define THREAD_PRIORITY     99
#endif

#ifndef IRQ_NONE
#define IRQ_NONE                (0)
#endif

#ifndef IRQ_HANDLED
#define IRQ_HANDLED             (1)
#endif

#ifndef IRQ_RETVAL
#define IRQ_RETVAL(x)           ((x) != 0)
#endif

#define IRQF_TRIGGER_NONE       0x00000000
#define IRQF_TRIGGER_RISING     0x00000001
#define IRQF_TRIGGER_FALLING    0x00000002
#define IRQF_TRIGGER_HIGH       0x00000004
#define IRQF_TRIGGER_LOW        0x00000008
#define IRQF_TRIGGER_MASK       (IRQF_TRIGGER_HIGH | IRQF_TRIGGER_LOW | \
                                IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING)
#define IRQF_TRIGGER_PROBE      0x00000010

#ifndef IRQF_SHARED
#define IRQF_SHARED             0x00000080
#endif

#define IRQF_PROBE_SHARED       0x00000100
#define __IRQF_TIMER            0x00000200
#define IRQF_PERCPU             0x00000400
#define IRQF_NOBALANCING        0x00000800
#define IRQF_IRQPOLL            0x00001000
#define IRQF_ONESHOT            0x00002000
#define IRQF_NO_SUSPEND         0x00004000
#define IRQF_FORCE_RESUME       0x00008000
#define IRQF_NO_THREAD          0x00010000
#define IRQF_EARLY_RESUME       0x00020000
#define IRQF_COND_SUSPEND       0x00040000

#define POLLIN                  0x00000001
#define POLLPRI                 0x00000002
#define POLLOUT                 0x00000004
#define POLLERR                 0x00000008
#define POLLHUP                 0x00000010
#define POLLNVAL                0x00000020

#define EPOLLIN                 0x00000001
#define EPOLLPRI                0x00000002
#define EPOLLOUT                0x00000004
#define EPOLLERR                0x00000008
#define EPOLLHUP                0x00000010
#define EPOLLNVAL               0x00000020
#define EPOLLRDNORM             0x00000040
#define EPOLLRDBAND             0x00000080
#define EPOLLWRNORM             0x00000100
#define EPOLLWRBAND             0x00000200
#define EPOLLMSG                0x00000400
#define EPOLLRDHUP              0x00002000


#ifdef __cplusplus
}
#endif

#endif /*__OSAL_USER_AX__H__*/
