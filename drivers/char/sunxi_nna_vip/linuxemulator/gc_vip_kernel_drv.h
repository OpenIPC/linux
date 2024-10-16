/******************************************************************************\
|* Copyright (c) 2017-2022 by Vivante Corporation.  All Rights Reserved.      *|
|*                                                                            *|
|* The material in this file is confidential and contains trade secrets of    *|
|* of Vivante Corporation.  This is proprietary information owned by Vivante  *|
|* Corporation.  No part of this work may be disclosed, reproduced, copied,   *|
|* transmitted, or used in any way for any purpose, without the express       *|
|* written permission of Vivante Corporation.                                 *|
|*                                                                            *|
\******************************************************************************/

#ifndef __GC_VIP_KERNEL_DRV_H__
#define __GC_VIP_KERNEL_DRV_H__

#include <stdint.h>
#include <stdarg.h>
#include <gc_vip_common.h>

typedef uintptr_t  gckvip_uintptr_t;

vip_status_e gckvip_drv_set_power_clk(
    vip_uint32_t state,
    vip_uint32_t core
    );

vip_status_e gckvip_drv_get_hardware_info(
    gckvip_hardware_info_t *info
    );

#endif /* __GC_VIP_KERNEL_DRV_H__ */
