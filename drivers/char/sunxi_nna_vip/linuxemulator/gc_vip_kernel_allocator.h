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

#ifndef _VIP_KERNEL_ALLOCATOR_H
#define _VIP_KERNEL_ALLOCATOR_H

#include <gc_vip_common.h>
#include <gc_vip_video_memory.h>

#if vpmdENABLE_MMU
#include <gc_vip_kernel_port.h>
#include <gc_vip_kernel_heap.h>
vip_status_e gckvip_allocator_get_userlogical(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t virtual,
    void **logical,
    vip_uint8_t memory_type
    );

vip_status_e gckvip_allocator_get_kernellogical(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t virtual,
    void **logical,
    vip_uint8_t memory_type
    );

vip_status_e gckvip_allocator_get_vipphysical(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t vip_virtual,
    phy_address_t *physical,
    vip_uint8_t memory_type
    );

vip_status_e gckvip_allocator_wrap_usermemory(
    gckvip_context_t *context,
    vip_ptr logical,
    vip_uint32_t memory_type,
    gckvip_dyn_allocate_node_t *node
    );

vip_status_e gckvip_allocator_unwrap_usermemory(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node
    );

#endif

vip_status_e gckvip_alloctor_flush_cache(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    phy_address_t physical,
    void *logical,
    vip_uint32_t size,
    gckvip_cache_type_e type
    );

vip_status_e gckvip_allocator_dyn_alloc(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t align,
    vip_uint32_t alloc_flag
    );

vip_status_e gckvip_allocator_dyn_free(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node
    );

#endif
