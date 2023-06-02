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

#include "sw_sem.h"

static intercore_sem_t *intercore_sem;

void intercore_sem_init(unsigned int addr)
{
    intercore_sem = (intercore_sem_t *)(addr);
}

void intercore_sem_lock(void)
{
	volatile char *p, *q;
	intercore_sem->nesting++;
	if (intercore_sem->nesting != 1) {
		//printk("Nesting %d\n", intercore_sem->nesting);
	}
	intercore_sem->flag[CORE1] = 1;
	intercore_sem->turn = CORE0;
	q = &intercore_sem->flag[CORE0];
	p = &intercore_sem->turn;
	while (*q == 1 && *p == 0) ;
}

void intercore_sem_unlock(void)
{
	intercore_sem->nesting--;
	intercore_sem->flag[CORE1] = 0;
}
