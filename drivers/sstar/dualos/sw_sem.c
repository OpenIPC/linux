/*
 * sw_sem.c- Sigmastar
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

#include "sw_sem.h"

static intercore_sem_t *intercore_sem;

void intercore_sem_init(unsigned long addr)
{
    intercore_sem = (intercore_sem_t *)(addr);
    /* lock ID 0 already reserved by RTOS for dualos driver internal use */
}

unsigned int intercore_sem_lock_request(void)
{
    unsigned int i      = 0;
    unsigned int sem_id = 0;

    intercore_sem_lock(SW_SEM_RESERVE_LOCK_ID);
    for (i = 1; i < SW_SEM_LOCK_TOTAL_NUM; i++)
    {
        if (!(intercore_sem + i)->used)
        {
            (intercore_sem + i)->used = 1;
            sem_id                    = i;
            break;
        }
    }
    intercore_sem_unlock(SW_SEM_RESERVE_LOCK_ID);

    return sem_id;
}

void intercore_sem_lock_free(unsigned int sem_id)
{
    if (sem_id != 0 && sem_id < SW_SEM_LOCK_TOTAL_NUM)
    {
        intercore_sem_lock(SW_SEM_RESERVE_LOCK_ID);
        (intercore_sem + sem_id)->used = 0;
        intercore_sem_unlock(SW_SEM_RESERVE_LOCK_ID);
    }
}

void intercore_sem_lock(unsigned int sem_id)
{
    volatile char *p, *q;
    (intercore_sem + sem_id)->nesting++;
    if ((intercore_sem + sem_id)->nesting != 1)
    {
        // printk("Nesting %d\n", (intercore_sem + sem_id)->nesting);
    }
    (intercore_sem + sem_id)->flag[CORE1] = 1;
    (intercore_sem + sem_id)->turn        = CORE0;
    __asm__ __volatile__("dsb sy" : : : "memory");
    q = &(intercore_sem + sem_id)->flag[CORE0];
    p = &(intercore_sem + sem_id)->turn;
    while (*q == 1 && *p == 0)
        ;
}

void intercore_sem_unlock(unsigned int sem_id)
{
    (intercore_sem + sem_id)->nesting--;
    (intercore_sem + sem_id)->flag[CORE1] = 0;
    __asm__ __volatile__("dsb sy" : : : "memory");
}
