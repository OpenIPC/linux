/*
 * cam_inter_os.h- Sigmastar
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

#ifndef __CAM_INTER_OS_H__
#define __CAM_INTER_OS_H__

int          CamInterOsSignalReg(unsigned int id, void *func, const char *name);
int          CamInterOsSignalDereg(unsigned int id, void *func);
unsigned int CamInterOsSignal(unsigned int arg0, unsigned int arg1, unsigned int arg2, unsigned int arg3);
void         CamInterOsSignalAsync(unsigned int arg0, unsigned int arg1, unsigned int arg2, unsigned int arg3);
unsigned int CamInterOsRequestLock(void);
void         CamInterOsFreeLock(unsigned int sem_id);
void         CamInterOsLock(unsigned int *lock, unsigned int sem_id);
void         CamInterOsUnlock(unsigned int *lock, unsigned int sem_id);

#endif //__CAM_INTER_OS_H__
