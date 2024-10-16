/******************************************************************************\
|* Copyright (c) 2017-2021 by Vivante Corporation.  All Rights Reserved.      *|
|*                                                                            *|
|* The material in this file is confidential and contains trade secrets of    *|
|* of Vivante Corporation.  This is proprietary information owned by Vivante  *|
|* Corporation.  No part of this work may be disclosed, reproduced, copied,   *|
|* transmitted, or used in any way for any purpose, without the express       *|
|* written permission of Vivante Corporation.                                 *|
|*                                                                            *|
\******************************************************************************/


#ifndef _VIP_KERNEL_OS_H
#define _VIP_KERNEL_OS_H

#include <semaphore.h>
#include <gc_vip_common.h>
#include <gc_vip_kernel_share.h>

vip_status_e CallEmulator(gckvip_command_id_e Command, void *Data);

typedef struct _gckvip_semaphore
{
    sem_t semaphore;
}gckvip_semaphore;

vip_status_e gckvip_os_create_semaphore(
    gckvip_semaphore * semaphore
    );

vip_status_e gckvip_os_destory_semaphore(
    gckvip_semaphore *semaphore
    );

vip_status_e gckvip_os_wait_semaphore(
    gckvip_semaphore *semaphore
    );

vip_status_e gckvip_os_relase_semaphore(
    gckvip_semaphore *semaphore
    );
#endif
