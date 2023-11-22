/*
 * sw_sem.h- Sigmastar
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

/* for RTK2_DISABLE_K()/RTK2_ENABLE_K() */
//#include "rtkincl.h"

#define AMP_OS_CS_INIT()  unsigned long cpu_sr = 0
#define AMP_OS_CS_ENTER() local_irq_save(cpu_sr)
#define AMP_OS_CS_EXIT()  local_irq_restore(cpu_sr)

#define AMP_CORE_LOCK(id)   intercore_sem_lock(id)
#define AMP_CORE_UNLOCK(id) intercore_sem_unlock(id)

#define AMP_LOCK_INIT() AMP_OS_CS_INIT()

#define AMP_LOCK(id)   \
    AMP_OS_CS_ENTER(); \
    AMP_CORE_LOCK(id)

#define AMP_UNLOCK(id)   \
    AMP_CORE_UNLOCK(id); \
    AMP_OS_CS_EXIT()

#define CORE0 (0)
#define CORE1 (1)

typedef struct
{
    char flag[2];
    char turn;
    char nesting;
    char used;
    char dummy[3];
} intercore_sem_t;

#define SW_SEM_RESERVE_LOCK_ID 0
#define SW_SEM_LOCK_TOTAL_NUM  16

void         intercore_sem_init(unsigned long addr);
unsigned int intercore_sem_lock_request(void);
void         intercore_sem_lock_free(unsigned int sem_id);
void         intercore_sem_lock(unsigned int sem_id);
void         intercore_sem_unlock(unsigned int sem_id);
