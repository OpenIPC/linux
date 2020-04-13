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

/* for RTK2_DISABLE_K()/RTK2_ENABLE_K() */
//#include "rtkincl.h"

#define AMP_OS_CS_INIT()        unsigned long cpu_sr = 0
#define AMP_OS_CS_ENTER()       local_irq_save(cpu_sr)
#define AMP_OS_CS_EXIT()        local_irq_restore(cpu_sr)

#define AMP_CORE_LOCK()         intercore_sem_lock()
#define AMP_CORE_UNLOCK()       intercore_sem_unlock()

#define AMP_LOCK_INIT()         AMP_OS_CS_INIT()
	
#define AMP_LOCK()              AMP_OS_CS_ENTER();  \
					            AMP_CORE_LOCK()
					
#define AMP_UNLOCK()            AMP_CORE_UNLOCK();  \
		 			            AMP_OS_CS_EXIT()

#define CORE0       (0)
#define CORE1       (1)

typedef struct
{
	char flag[2];
	char turn;
	char nesting;
} intercore_sem_t;

void intercore_sem_init(unsigned int addr);
void intercore_sem_lock(void);
void intercore_sem_unlock(void);
