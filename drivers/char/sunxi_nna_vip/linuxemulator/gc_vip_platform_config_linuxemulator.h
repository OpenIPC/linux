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


#ifndef _GC_VIP_PLATFORM_CONFIG_H
#define _GC_VIP_PLATFORM_CONFIG_H
#include <gc_vip_common.h>

/* the contiguous memory base address,we need disable MMU when use it.*/
#define gcdCONTIGUOUS_BASE 0x8000000


#define CMODEL_MEMORY_SIZE             (512 << 20)

/* default size 32M for system memory heap, access by CPU only */
#define SYSTEM_MEMORY_HEAP_SIZE         (32 << 20)


#if vpmdENABLE_MMU
#define VIDEO_MEMORY_HEAP_SIZE  (4 << 20)

#else
#define VIDEO_MEMORY_HEAP_SIZE  (256 << 20)
#endif


/* the size of VIP SRAM */
#define VIP_SRAM_SIZE                   0xA00000


/* the size of AXI SRAM */
#define AXI_SRAM_SIZE                   0xA00000

#define VIP_SRAM_BASE_ADDRESS       0x400000

/* the core number of VIP */
#define  CORE_COUNT                  1

/* the logic devices number and core number for each logical device.
 * we need configure this after create nbg files.
 * for example: LOGIC_DEVICES_WITH_CORE[8] = {2,2,4,0,0,0,0,0}.
 * indicate device[0] and device[1] have 2 cores, device[2] have 4 cores.
 * the others devices not available, each device have different core index.
*/
static vip_uint32_t LOGIC_DEVICES_WITH_CORE[CORE_COUNT] = {1};


#endif
