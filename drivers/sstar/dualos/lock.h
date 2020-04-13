/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.

 Unless otherwise stipulated in writing, any and all information contained
herein regardless in any format shall remain the sole proprietary of
Sigmastar Technology Corp. and be kept in strict confidence
(Sigmastar Confidential Information) by the recipient.
Any unauthorized act including without limitation unauthorized disclosure,
copying, use, reproduction, sale, distribution, modification, disassembling,
reverse engineering and compiling of the contents of Sigmastar Confidential
Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
rights to any and all damages, losses, costs and expenses resulting therefrom.
*/

#ifdef _LINUX_
    #ifdef CONFIG_SS_AMP
    #define DUALOS_LOCK_INIT    AMP_LOCK_INIT()
    #define DUALOS_LOCK         AMP_LOCK()
    #define DUALOS_UNLOCK       AMP_UNLOCK()
    #endif
    #ifdef CONFIG_LH_RTOS
    #include <linux/irqflags.h>
    #define DUALOS_LOCK_INIT
    #define DUALOS_LOCK         local_fiq_disable()
    #define DUALOS_UNLOCK       local_fiq_enable()
    #endif
#else //RTOS
    #ifdef __ENABLE_AMP__
    #define DUALOS_LOCK_INIT    AMP_LOCK_INIT()
    #define DUALOS_LOCK         AMP_LOCK()
    #define DUALOS_UNLOCK       AMP_UNLOCK()
    #else
    #define DUALOS_LOCK_INIT
    #define DUALOS_LOCK
    #define DUALOS_UNLOCK
    #endif
#endif
