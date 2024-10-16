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

#include <gc_vip_kernel_allocator.h>
#include "gc_vip_kernel_cmodel.h"

#if vpmdENABLE_MMU
#include "gc_vip_kernel_port.h"
#include "gc_vip_hardware.h"
#include "gc_vip_kernel.h"
#include "gc_vip_kernel_os.h"
#include "gc_vip_kernel_heap.h"
#include <stdio.h>

vip_status_e gckvip_allocator_get_userlogical(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t virtual,
    void **logical,
    vip_uint8_t memory_type
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_int32_t offset = virtual - node->vip_virtual_address;

    if (((vip_uint32_t)offset >= 0) && ((vip_uint32_t)offset <= node->size)) {
        *logical = offset + node->user_logical;
    }
    else {
        PRINTK("allocator get use logical failed\n");
        status = VIP_ERROR_IO;
    }

    return status;
}

vip_status_e gckvip_allocator_get_kernellogical(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t virtual,
    void **logical,
    vip_uint8_t memory_type
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_int32_t offset = virtual - node->vip_virtual_address;

    if (((vip_uint32_t)offset >= 0) && ((vip_uint32_t)offset <= node->size)) {
        *logical = offset + node->user_logical;
    }
    else {
        PRINTK("allocator get kernel logical failed\n");
        status = VIP_ERROR_IO;
    }

    return status;
}

vip_status_e gckvip_allocator_get_vipphysical(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t vip_virtual,
    phy_address_t *physical,
    vip_uint8_t memory_type
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_int64_t offset = vip_virtual - node->vip_virtual_address;

    *physical = node->physical_table[0] + offset;

    return status;
}

vip_status_e gckvip_allocator_wrap_usermemory(
    gckvip_context_t *context,
    vip_ptr logical,
    vip_uint32_t memory_type,
    gckvip_dyn_allocate_node_t *node
    )
{
    vip_status_e status = VIP_SUCCESS;

    /* allocate a video memory for wrap user memory on WIN32 */
    cmMemoryInfo *mem = CModelAllocate(GCVIP_ALIGN(node->size, 4 * 1024), vip_true_e);
    PRINTK_D("alloc syn memory handle=%p\n", mem);

    gcOnError(gckvip_os_allocate_memory(sizeof(phy_address_t) * 1, (void**)&node->physical_table));
    gcOnError(gckvip_os_allocate_memory(sizeof(vip_uint32_t) * 1, (void**)&node->size_table));

    node->user_logical = logical; /* user's logical address */
    node->kerl_logical = mem->logical; /* allocate video memory logical address */
    node->alloc_handle = mem;

    node->physical_table[0] = (phy_address_t)mem->physical;
    node->size_table[0] = GCVIP_ALIGN(node->size, 4 * 1024); /* algin to 4Kbytes */
    node->physical_num = 1;   /* map contigous memory, only one page */
    node->num_pages = 1;

onError:
    return status;
}

vip_status_e gckvip_allocator_unwrap_usermemory(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node
    )
{
    vip_status_e status = VIP_SUCCESS;

    if (node != NULL) {
        gckvip_os_free_memory(node->physical_table);

        gckvip_os_free_memory(node->size_table);

        PRINTK("alloc syn memory handle=%p\n", node->alloc_handle);
        CModelFree((cmMemoryInfo *)node->alloc_handle);
    }

    return status;
}
#endif

vip_status_e gckvip_alloctor_flush_cache(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    phy_address_t physical,
    void *logical,
    vip_uint32_t size,
    gckvip_cache_type_e type
    )
{
    vip_status_e status = VIP_SUCCESS;

    if (GCKVIP_CACHE_FLUSH == type) {
        gckvip_os_memcopy(node->kerl_logical, node->user_logical, node->size);
    }
    else if (GCKVIP_CACHE_INVALID == type) {
        gckvip_os_memcopy(node->user_logical, node->kerl_logical, node->size);
    }
    else {
        PRINTK_E("not support this flush cache type=%d\n", type);
    }

    return status;
}

vip_status_e gckvip_allocator_dyn_alloc(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t align,
    vip_uint32_t alloc_flag
    )
{
    vip_status_e status = VIP_SUCCESS;
    cmMemoryInfo *mem = CModelAllocate(node->size, vip_true_e);
    PRINTK("alloc syn memory handle=%p\n", mem);

    gcOnError(gckvip_os_allocate_memory(sizeof(phy_address_t) * 1, (void**)&node->physical_table));
    gcOnError(gckvip_os_allocate_memory(sizeof(vip_uint32_t) * 2, (void**)&node->size_table));

    node->user_logical = mem->logical;
    node->kerl_logical = mem->logical;
    node->alloc_handle = mem;

    node->physical_table[0] = (phy_address_t)mem->physical;
    node->size_table[0] = GCVIP_ALIGN(node->size, 4 * 1024); /* algin to 4Kbytes */
    node->physical_num = 1;   /* map contigous memory, only one page */
    node->num_pages = 1;

onError:
    return status;
}

vip_status_e gckvip_allocator_dyn_free(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node
    )
{
    vip_status_e status = VIP_SUCCESS;

    if (node != NULL) {
        gckvip_os_free_memory(node->physical_table);

        gckvip_os_free_memory(node->size_table);

        PRINTK("alloc syn memory handle=%p\n", node->alloc_handle);
        CModelFree((cmMemoryInfo *)node->alloc_handle);
    }

    return VIP_SUCCESS;
}

