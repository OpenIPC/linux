/****************************************************************************
*
*    The MIT License (MIT)
*
*    Copyright (c) 2017 - 2022 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************
*
*    The GPL License (GPL)
*
*    Copyright (C) 2017 - 2022 Vivante Corporation
*
*    This program is free software; you can redistribute it and/or
*    modify it under the terms of the GNU General Public License
*    as published by the Free Software Foundation; either version 2
*    of the License, or (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*****************************************************************************
*
*    Note: This software is released under dual MIT and GPL licenses. A
*    recipient may use this file under the terms of either the MIT license or
*    GPL License. If you wish to use only one license not the other, you can
*    indicate your decision by deleting one of the above license notices in your
*    version of this file.
*
*****************************************************************************/

#include <gc_vip_kernel_pm.h>
#include <gc_vip_hardware.h>
#include <gc_vip_kernel.h>
#include <gc_vip_kernel_drv.h>
#include <gc_vip_kernel_allocator.h>
#include <gc_vip_kernel_mmu.h>
#include <gc_vip_video_memory.h>
#include <gc_vip_kernel_debug.h>
#include <gc_vip_kernel_command.h>

vip_status_e gckvip_cmd_submit_init(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_submit_t submission;
    if (VIP_NULL == device->init_command.handle) {
        PRINTK_E("init command memory is NULL\n");
        gcGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }

    /* Submit commands. */
    submission.cmd_logical = (vip_uint8_t *)device->init_command.logical;
    submission.cmd_physical = (vip_uint32_t)device->init_command.physical;
    submission.cmd_handle = device->init_command.handle;
    submission.cmd_size = device->init_command.size;
    submission.last_cmd_logical = (vip_uint8_t *)device->init_command.logical;
    submission.last_cmd_physical = (vip_uint32_t)device->init_command.physical;
    submission.last_cmd_handle = device->init_command.handle;
    submission.last_cmd_size = device->init_command.size;
    submission.device_id = device->device_id;
    PRINTK_D("submit initialize commands size=0x%x, hand=0x%"PRPx"\n",
              device->init_command.size, device->init_command.handle);
#if vpmdENABLE_APP_PROFILING
    gckvip_prepare_profiling(&submission, device);
#endif
    gcOnError(gckvip_hw_submit(context, device, &submission));
#if vpmdENABLE_DEBUGFS
    gckvip_debug_set_profile_data(gckvip_os_get_time(), PROFILE_SUBMIT);
#endif

#if vpmdENABLE_APP_PROFILING
    gckvip_start_profiling(&submission, device);
#endif

    /* Wait to be done. */
    {
    gckvip_wait_cmd_t wait_data;
    wait_data.mask = 0xFFFFFFFF;
    wait_data.time_out = 100; /* 100ms timeout for initialize states */
    wait_data.cmd_handle = device->init_command.handle;
#if vpmdENABLE_CLOCK_SUSPEND
    gcOnError(gckvip_os_inc_atomic(device->disable_clock_suspend));
#endif
#if vpmdPOWER_OFF_TIMEOUT && !vpmdONE_POWER_DOMAIN
    gckvip_os_set_atomic(device->dp_management.enable_timer, vip_false_e);
#elif vpmdPOWER_OFF_TIMEOUT && vpmdONE_POWER_DOMAIN
    gckvip_os_set_atomic(context->sp_management.enable_timer, vip_false_e);
#endif
    gcOnError(gckvip_cmd_wait(context, (void *)&wait_data, device));
#if vpmdPOWER_OFF_TIMEOUT && !vpmdONE_POWER_DOMAIN
    gckvip_os_set_atomic(device->dp_management.enable_timer, vip_true_e);
#elif vpmdPOWER_OFF_TIMEOUT && vpmdONE_POWER_DOMAIN
    gckvip_os_set_atomic(context->sp_management.enable_timer, vip_true_e);
#endif
#if vpmdENABLE_CLOCK_SUSPEND
    gcOnError(gckvip_os_dec_atomic(device->disable_clock_suspend));
#endif
    }
    PRINTK_D("initialize commands done\n");

    return status;
onError:
    PRINTK_E("fail to submit initialize command\n");
    return status;
}

/*
@brief To run any initial commands once in the beginning.
*/
vip_status_e gckvip_cmd_prepare_init(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_uint32_t cmd_size = sizeof(vip_uint32_t) * 20;
    vip_uint32_t *logical = VIP_NULL;
    vip_uint32_t buf_size = 0;
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t count = 0;
    vip_uint32_t alloc_flag = GCVIP_VIDEO_MEM_ALLOC_MAP_KERNEL_LOGICAL |
                              GCVIP_VIDEO_MEM_ALLOC_CONTIGUOUS;
    phy_address_t physical_tmp = 0;
#if !vpmdENABLE_VIDEO_MEMORY_CACHE
    alloc_flag |= GCVIP_VIDEO_MEM_ALLOC_NONE_CACHE;
#endif

    if (context->vip_sram_size > 0) {
        cmd_size += sizeof(vip_uint32_t) * 2;
    }
    if (context->axi_sram_size > 0) {
        cmd_size += sizeof(vip_uint32_t) * 4;
    }

    buf_size = cmd_size + APPEND_COMMAND_SIZE;
    if (1 < device->hardware_count) {
        buf_size += 2 * CHIP_ENABLE_SIZE;
    }
    buf_size = GCVIP_ALIGN(buf_size, vpmdCPU_CACHE_LINE_SIZE);
    gcOnError(gckvip_mem_allocate_videomemory(context, buf_size, &device->init_command.handle,
                                              (void **)&logical, &physical_tmp,
                                              gcdVIP_MEMORY_ALIGN_SIZE, alloc_flag));
    gckvip_os_zero_memory(logical, buf_size);
    device->init_command.logical = (void*)logical;
    device->init_command.physical = (vip_uint32_t)physical_tmp;

    logical[count++] = 0x0801028A;
    logical[count++] = 0x00000011;
    logical[count++] = 0x08010E13;
    logical[count++] = 0x00000002;
    logical[count++] = 0x08010E12;
    logical[count++] = SETBIT(0, 16, 1);
    logical[count++] = 0x08010E21;
    logical[count++] = 0x00020000;
    logical[count++] = 0x08010e02;
    logical[count++] = 0x00000701;
    logical[count++] = 0x48000000;
    logical[count++] = 0x00000701;
    logical[count++] = 0x0801022c;
    logical[count++] = 0x0000001f;
    if (context->vip_sram_size > 0) {
        logical[count++] = 0x08010e4e;
        logical[count++] = context->vip_sram_address;
    }
    if (context->axi_sram_size > 0) {
        logical[count++] = 0x08010e4f;
        logical[count++] = context->axi_sram_address;
        logical[count++] = 0x08010e50;
        logical[count++] = context->axi_sram_address + context->axi_sram_size;
    }
    logical[count++] = 0x08010e02;
    logical[count++] = 0x00000701;
    logical[count++] = 0x48000000;
    logical[count++] = 0x00000701;

#if !vpmdENABLE_SECURE
    if (1 < device->hardware_count) {
        logical[count++] = 0x68000000 | (1 << device->hardware[0].core_id);
        logical[count++] = 0;
    }
    logical[count++] = 0x08010E01;
    logical[count++] = 0x00000044;
    if (1 < device->hardware_count) {
        logical[count++] = 0x6800FFFF;
        logical[count++] = 0;
    }
#endif
#if !vpmdENABLE_WAIT_LINK_LOOP
    logical[count++] = 0x10000000;
    logical[count++] = 0;
#endif
    device->init_command_size = count * sizeof(vip_uint32_t);

    if ((count * sizeof(vip_uint32_t)) > buf_size) {
        PRINTK_E("failed to init commands, size is overflow\n");
        gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);
    }
    cmd_size = count * sizeof(vip_uint32_t);
    device->init_command.size = cmd_size;

#if vpmdENABLE_VIDEO_MEMORY_CACHE
    gckvip_mem_flush_cache(device->init_command.handle, device->init_command.physical,
                           device->init_command.logical, buf_size, GCKVIP_CACHE_FLUSH);
#endif
    gcOnError(gckvip_database_get_id(&context->videomem_database, device->init_command.handle,
                                     gckvip_os_get_pid(), &device->init_command.handle_id));
onError:
    return status;
}

/*
@brief wait hardware interrupt return.
*/
vip_status_e gckvip_cmd_wait(
    gckvip_context_t *context,
    gckvip_wait_cmd_t *info,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t timeout = info->time_out;
    gckvip_hardware_t *hardware = VIP_NULL;
    if (vip_false_e == gckvip_os_get_atomic(device->device_idle))
    {
        PRINTK_I("wait mem_handle=0x%"PRPx" start\n", info->cmd_handle);
        hardware = &device->hardware[0];
        #if vpmdENABLE_POLLING
        /* poll hardware idle, only need first core? */
        status = gckvip_hw_wait_poll(hardware, device, timeout);
        #else
        /* wait interrupt on first core */
        if ((VIP_NULL == hardware->irq_queue) || (VIP_NULL == hardware->irq_flag)) {
            PRINTK_E("fail to wait interrupt. int queue or int falg is NULL\n");
            return VIP_ERROR_OUT_OF_MEMORY;
        }
        status = gckvip_os_wait_interrupt(hardware->irq_queue, hardware->irq_flag,
                                          timeout, info->mask);
        #endif
        if (status == VIP_ERROR_TIMEOUT) {
            #if vpmdENABLE_HANG_DUMP
            gckvip_hang_dump(device);
            #endif

            #if vpmdENABLE_RECOVERY
            status = gckvip_hw_recover(context, device);
            if (status != VIP_SUCCESS) {
                PRINTK_E("fail to recover hardware, status=%d.\n", status);
                gcGoOnError(status);
            }
            PRINTK_I("hardware recovery done, return %d for app use.\n", status);
            gcGoOnError(VIP_ERROR_RECOVERY);
            #else
            PRINTK_E("error hardware hang wait irq time=%dms.......\n", info->time_out);
            gcGoOnError(VIP_ERROR_TIMEOUT);
            #endif
        }
        else {
            status = gckvip_check_irq_value(context, device);
            if ((status != VIP_SUCCESS) && (status != VIP_ERROR_RECOVERY) && (status != VIP_ERROR_CANCELED)) {
                PRINTK_E("fail to check irq value status=%d\n", status);
                gcGoOnError(status);
            }
            *hardware->irq_flag = 0;
        }

        #if vpmdENABLE_APP_PROFILING
        gckvip_end_profiling(context, device, info->cmd_handle);
        #endif
        #if vpmdENABLE_DEBUGFS
        gckvip_debug_set_profile_data(gckvip_os_get_time(), PROFILE_WAIT);
        #endif

        /* Then link back to the WL commands for letting vip going to idle. */
        if (VIP_SUCCESS != gckvip_hw_idle(context, device)) {
            PRINTK_E("fail to hw idle.\n");
            gcGoOnError(VIP_ERROR_FAILURE)
        }

        PRINTK_I("wait mem_handle=0x%"PRPx" done\n", info->cmd_handle);
    }

    return status;
onError:
    PRINTK_E("failed to wait command complete status=%d\n", status);
    return status;
}

/*
@brief commit commands to hardware.
*/
vip_status_e gckvip_cmd_submit(
    gckvip_context_t *context,
    void *data
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_device_t *device = VIP_NULL;
    gckvip_submit_t *submit = VIP_NULL;

    /* defined(__linux__) is just for viplite run on Linux.
       user space logical need to be switched kernel space logical address
    */
#if defined(__linux__)
    gckvip_submit_t submitTmp;
    gckvip_submit_t *tmp = (gckvip_submit_t *)data;
    gcOnError(gckvip_mem_get_kernellogical(context, tmp->cmd_handle, tmp->cmd_physical,
                                (void**)&submitTmp.cmd_logical));
    submitTmp.cmd_physical = tmp->cmd_physical;
    submitTmp.cmd_handle = tmp->cmd_handle;
    submitTmp.cmd_size = tmp->cmd_size;
    gcOnError(gckvip_mem_get_kernellogical(context, tmp->last_cmd_handle,
                                 tmp->last_cmd_physical,
                                (void**)&submitTmp.last_cmd_logical));
    submitTmp.last_cmd_physical = tmp->last_cmd_physical;
    submitTmp.last_cmd_handle = tmp->last_cmd_handle;
    submitTmp.last_cmd_size = tmp->last_cmd_size;
    submitTmp.device_id = tmp->device_id;
    submit = &submitTmp;
#else
    submit = (gckvip_submit_t *)data;
#endif
    if (VIP_NULL == submit->cmd_handle) {
        gcGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }
    device = &context->device[submit->device_id];

#if vpmdPOWER_OFF_TIMEOUT
#if vpmdONE_POWER_DOMAIN
    gckvip_lock_recursive_mutex(&context->sp_management.mutex);
#else
    gckvip_lock_recursive_mutex(&device->dp_management.mutex);
#endif
#endif

#if vpmdENABLE_SUSPEND_RESUME || vpmdPOWER_OFF_TIMEOUT
    status = gckvip_pm_check_power(context, device);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to check power in submit status=%d\n", status);
        gcGoOnError(status);
    }
#endif

    PRINTK_D("submit command to hardware, command size=0x%x, physical=0x%08x\n",
              submit->cmd_size, submit->cmd_physical);

    GCKVIP_CHECK_USER_PM_STATUS();
#if vpmdENABLE_APP_PROFILING
    gckvip_prepare_profiling(submit, device);
#endif
    gcOnError(gckvip_hw_submit(context, device, submit));

#if vpmdPOWER_OFF_TIMEOUT
#if vpmdONE_POWER_DOMAIN
    gckvip_unlock_recursive_mutex(&context->sp_management.mutex);
#else
    gckvip_unlock_recursive_mutex(&device->dp_management.mutex);
#endif
#endif

#if vpmdENABLE_DEBUGFS
    gckvip_debug_set_profile_data(gckvip_os_get_time(), PROFILE_SUBMIT);
#endif

#if vpmdENABLE_APP_PROFILING
    gckvip_start_profiling(submit, device);
#endif

#if vpmdENABLE_RECOVERY
    device->recovery_times = MAX_RECOVERY_TIMES;
#endif

    return status;
onError:
#if vpmdPOWER_OFF_TIMEOUT
#if vpmdONE_POWER_DOMAIN
    gckvip_unlock_recursive_mutex(&context->sp_management.mutex);
#else
    gckvip_unlock_recursive_mutex(&device->dp_management.mutex);
#endif
#endif
    PRINTK_E("fail to submit cmds to hardware status=%d\n", status);
    return status;
}

#if vpmdENABLE_MULTIPLE_TASK
vip_status_e gckvip_cmd_thread_handle(
    gckvip_context_t *context,
    gckvip_queue_t *queue
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_queue_data_t *queue_data = VIP_NULL;
    volatile gckvip_multi_thread_submit_t *submit = VIP_NULL;
    vip_uint32_t device_id = queue->device_id;
    gckvip_device_t *device = &context->device[device_id];
    gckvip_wait_cmd_t wait_data;
    wait_data.mask = 0xFFFFFFFF;
    wait_data.time_out = vpmdHARDWARE_TIMEOUT;

    gckvip_os_set_atomic(device->mt_thread_running, vip_true_e);
    PRINTK_I("multi-thread process thread start\n");
    gckvip_os_set_signal(device->mt_thread_signal, vip_false_e);

    while (gckvip_os_get_atomic(device->mt_thread_running)) {
        if (gckvip_queue_read(queue, &queue_data)) {
            if (VIP_NULL == queue_data) {
                PRINTK_E("multi-process read queue data is NULL\n");
                continue;
            }
            gckvip_os_lock_mutex(device->mt_mutex_expand);
            submit = &device->mt_submit[queue_data->v1];
            if (GCKVIP_SUBMIT_EMPTY == submit->status) {
                PRINTK_E("multi-process read queue submit is empty\n");
                gckvip_os_unlock_mutex(device->mt_mutex_expand);
                continue;
            }
            if (VIP_ERROR_CANCELED == submit->queue_data->v2) {
                PRINTK_E("queue_data is canceled, not need submit.\n");
                gckvip_os_unlock_mutex(device->mt_mutex_expand);
                continue;
            }
            submit->status = GCKVIP_SUBMIT_INFER_START;
            PRINTK_I("multi-task start submit command mem_handle=0x%"PRPx"\n", submit->submit_handle.cmd_handle);
            status = gckvip_cmd_submit(context, (void*)&submit->submit_handle);
            if (VIP_SUCCESS == status) {
                wait_data.cmd_handle = submit->submit_handle.cmd_handle;
                status = gckvip_cmd_wait(context, &wait_data, device);
                if (status != VIP_SUCCESS) {
                    PRINTK_E("fail to wait interrupt in multi-task thread, status=%d, mem_handle=0x%"PRPx"\n",
                             status, wait_data.cmd_handle);
                }
                PRINTK_I("multi-task wait task done\n");
            }
            else {
                PRINTK_E("fail to submit command mem_handle=0x%"PRPx", physical=0x%08x, size=0x%x, status=%d\n",
                          submit->submit_handle.cmd_handle, submit->submit_handle.cmd_physical,
                          submit->submit_handle.cmd_size, status);
            }

            submit->queue_data->v2 = (vip_status_e)status;
            submit->status = GCKVIP_SUBMIT_INFER_END;
            gckvip_os_set_signal(submit->wait_signal, vip_true_e);
            gckvip_os_unlock_mutex(device->mt_mutex_expand);
        }
    }

    /* wake up wait the thread exit task */
    gckvip_os_set_signal(device->mt_thread_signal, vip_true_e);
    PRINTK_I("multi-task thread exit\n");

    return status;
}
#endif

vip_status_e gckvip_cmd_do_wait(
    gckvip_context_t *context,
    gckvip_wait_t *info
    )
{
    vip_status_e status = VIP_SUCCESS;
    void *mem_handle = VIP_NULL;
    gckvip_device_t *device = VIP_NULL;
    vip_uint32_t handle_id = info->handle;

    gcOnError(gckvip_database_get_handle(&context->videomem_database, handle_id, &mem_handle));
    PRINTK_I("user start do wait device%d, mem_handle=0x%"PRPx"\n", info->device_id, mem_handle);

#if vpmdENABLE_MULTIPLE_TASK
    {
    volatile gckvip_multi_thread_submit_t *submit = VIP_NULL;
    vip_uint32_t i = 0;
    vip_bool_e valid_wait = vip_false_e;

    device = &context->device[info->device_id];

    gckvip_os_lock_mutex(device->mt_mutex);
    i = gckvip_hashmap_get(&device->mt_hashmap, mem_handle, vip_false_e);
    if (0 < i) {
        valid_wait = vip_true_e;
    }
    gckvip_os_unlock_mutex(device->mt_mutex);

    if ((vip_true_e == gckvip_os_get_atomic(device->mt_thread_running)) && valid_wait) {
        vip_uint32_t count = (vpmdHARDWARE_TIMEOUT + (WAIT_SIGNAL_TIMEOUT - 1)) / WAIT_SIGNAL_TIMEOUT;
        submit = &device->mt_submit[i];
        if (0 == count) {
            count = 1;
        }

        for (i = 0; i < count; i++) {
            status = gckvip_os_wait_signal(submit->wait_signal, WAIT_SIGNAL_TIMEOUT);
            if (VIP_SUCCESS == status) {
                break;
            }
            else {
                PRINTK_D("wait task timeout loop index=%d\n", i);
            }
        }
        gckvip_os_set_signal(submit->wait_signal, vip_false_e);
        if ((status != VIP_SUCCESS) && (count == i) && (GCKVIP_SUBMIT_INFER_START == submit->status)) {
        #if (vpmdENABLE_DEBUG_LOG >= 4)
            gckvip_os_lock_mutex(device->mt_mutex);
            PRINTK("dump task in submit_mt:\n");
            for (i = 0; i < device->mt_hashmap.size; i++) {
                submit = &device->mt_submit[i];
                if (VIP_NULL != submit->submit_handle.cmd_handle) {
                    PRINTK("wait signal=0x%"PRPx", mem_handle=0x%"PRPx"\n", submit->wait_signal,
                            submit->submit_handle.cmd_handle);
                }
            }
            gckvip_os_unlock_mutex(device->mt_mutex);
        #endif
            PRINTK_E("do wait signal mem_id=0x%x, mem_handle=0x%"PRPx", status=%d\n",
                     handle_id, mem_handle, status);
            submit->submit_handle.cmd_handle = VIP_NULL;
            submit->status = GCKVIP_SUBMIT_EMPTY;
            gckvip_os_lock_mutex(device->mt_mutex);
            gckvip_hashmap_get(&device->mt_hashmap, mem_handle, vip_true_e);
            gckvip_os_unlock_mutex(device->mt_mutex);
            gcGoOnError(status);
        }
        submit->submit_handle.cmd_handle = VIP_NULL;
        submit->status = GCKVIP_SUBMIT_EMPTY;
        gckvip_os_lock_mutex(device->mt_mutex);
        gckvip_hashmap_get(&device->mt_hashmap, mem_handle, vip_true_e);
        gckvip_os_unlock_mutex(device->mt_mutex);
        status = (vip_status_e)submit->queue_data->v2;
        if ((status != VIP_SUCCESS) && (status != VIP_ERROR_CANCELED)) {
            PRINTK_E("do wait mt thread fail=%d, mem_id=0x%x, mem_handle=0x%"PRPx", status=%d\n",
                      status, handle_id, mem_handle, status);
            gcGoOnError(status);
        }
    }
    else {
        PRINTK_D("bypass wait irq, mt_thread=%d valid_wait=%d, device_idle=%s, mem_handle=0x%"PRPx"\n",
                  gckvip_os_get_atomic(device->mt_thread_running), valid_wait,
                  (gckvip_os_get_atomic(device->device_idle)) ? "true" : "false", mem_handle);
    }
    }
#else
    {
    gckvip_wait_cmd_t wait;
    wait.mask = info->mask;
    wait.time_out = info->time_out;
    wait.cmd_handle = mem_handle;
    device = &context->device[info->device_id];
    GCKVIP_CHECK_USER_PM_STATUS();
    gcOnError(gckvip_cmd_wait(context, &wait, device));
    }
#endif

    GCKVIP_CHECK_USER_PM_STATUS();

    PRINTK_I("user end do wait device%d, mem_handle=0x%"PRPx"\n", info->device_id, mem_handle);
onError:
    return status;
}

#if vpmdENABLE_CANCELATION
static vip_status_e gckvip_hardware_do_cancel(
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0, data = 0, orgi_data = 0;
    gckvip_hardware_t *hardware = VIP_NULL;

    for (i = 0; i < device->hardware_count; i++) {
        hardware = &device->hardware[i];

        gcOnError(gckvip_read_register(hardware, 0x003A8, &orgi_data));

        data = orgi_data | (1 << 3);
        gcOnError(gckvip_write_register(hardware, 0x003A8, data));

        do {
            gckvip_os_udelay(100);
            gcOnError(gckvip_read_register(hardware, 0x003A8, &data));
        } while (!(data & (0x01 << 4)));

        data = orgi_data & (~(1 << 3));
        gcOnError(gckvip_write_register(hardware, 0x003A8, data));
    }

    gckvip_os_udelay(10);

    return status;

onError:
    return status;
}

vip_status_e gckvip_cmd_do_cancel(
    gckvip_context_t *context,
    gckvip_cancel_t *info
    )
{
    vip_status_e status = VIP_SUCCESS;
    void *mem_handle = VIP_NULL;
    gckvip_device_t *device = VIP_NULL;
    gckvip_hardware_t *hardware = VIP_NULL;
    vip_uint32_t handle_id = info->handle;
    vip_bool_e hw_cancel = info->hw_cancel;
    device = &context->device[info->device_id];
    hardware = &device->hardware[0];

    PRINTK_I("user start do cancel on device%d, hw_cancel = %d.\n", info->device_id, hw_cancel);

    {
        if (vip_false_e == hw_cancel) {
            vip_uint32_t i = 0;
            vip_uint32_t wait_time = 300; /* 300ms */
            #if vpmdFPGA_BUILD
            wait_time = 3000;
            #endif
            /* wait hw idle */
            if (vip_false_e == gckvip_os_get_atomic(device->device_idle)) {
                for (i = 0; i < device->hardware_count; i++) {
                    gckvip_hw_wait_idle(context, &device->hardware[i], wait_time, vip_true_e);
                }
            }
            PRINTK_I("user end do cancel on device%d, hw_cancel = %d.\n", info->device_id, hw_cancel);
            return status;
        }
    }
    gcOnError(gckvip_database_get_handle(&context->videomem_database, handle_id, &mem_handle));
    PRINTK_I("hw cancel mem_handle=0x%"PRPx"\n", mem_handle);

#if vpmdENABLE_MULTIPLE_TASK
    {
      volatile gckvip_multi_thread_submit_t *submit = VIP_NULL;
      vip_uint32_t i = 0;
      vip_bool_e valid_cancel = vip_false_e;

      gckvip_os_lock_mutex(device->mt_mutex);
      i = gckvip_hashmap_get(&device->mt_hashmap, mem_handle, vip_false_e);
      if (0 < i) valid_cancel = vip_true_e;
      gckvip_os_unlock_mutex(device->mt_mutex);

      if (valid_cancel) {
          submit = &device->mt_submit[i];
          if (submit->status < GCKVIP_SUBMIT_INFER_START) {
              submit->queue_data->v2 = (vip_status_e)VIP_ERROR_CANCELED;
              gckvip_os_set_signal(submit->wait_signal, vip_true_e);
          }
          else if (submit->status == GCKVIP_SUBMIT_INFER_START) {
              gcOnError(gckvip_hardware_do_cancel(device));
              *hardware->irq_flag = 0x10000000;
              gckvip_os_wakeup_interrupt(hardware->irq_queue);
              /* remap vip-sram*/
              gckvip_cmd_submit_init(context, device);
              gckvip_os_udelay(5);
          }
          else if (submit->status == GCKVIP_SUBMIT_INFER_END) {
              PRINTK_I("mem_handle=0x%"PRPx" already forward done\n", mem_handle);
          }
      }
      else {
          PRINTK_D("bypass cancel, mt_thread=%d valid_cancel=%d, device_idle=%s, mem_handle=0x%"PRPx"\n",
                    gckvip_os_get_atomic(device->mt_thread_running), valid_cancel,
                    (gckvip_os_get_atomic(device->device_idle)) ? "true" : "false", mem_handle);
      }
    }
#else
    {
        gcOnError(gckvip_hardware_do_cancel(device));
        *hardware->irq_flag = 0x10000000;
        gckvip_os_wakeup_interrupt(hardware->irq_queue);
        /* remap vip-sram*/
        gckvip_cmd_submit_init(context, device);
        gckvip_os_udelay(5);
    }
#endif

    PRINTK_I("user end do cancel device%d, mem_handle=0x%"PRPx"\n", info->device_id, mem_handle);
onError:
    return status;
}
#endif

vip_status_e gckvip_cmd_do_submit(
    gckvip_context_t *context,
    gckvip_commit_t *commit
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_device_t *device = VIP_NULL;
    void *cmd_handle = VIP_NULL;
    device = &context->device[commit->device_id];

    GCKVIP_CHECK_USER_PM_STATUS();

    status = gckvip_database_get_handle(&context->videomem_database, commit->cmd_handle, &cmd_handle);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail convert command buffer video memory id, handle=0x%x, status=%d.\n", commit->cmd_handle, status);
        gcGoOnError(status);
    }
    if (VIP_NULL == cmd_handle) {
        PRINTK_E("fail cmd handle is NULL in submit\n");
        gcGoOnError(VIP_ERROR_OUT_OF_RESOURCE);
    }
    PRINTK_I("start do submit device%d, cmd size=0x%x, id=0x%x, handle=0x%"PRPx"\n",
              commit->device_id, commit->cmd_size, commit->cmd_handle, cmd_handle);

#if vpmdENABLE_MULTIPLE_TASK
    if (vip_true_e == gckvip_os_get_atomic(device->mt_thread_running)) {
        volatile gckvip_multi_thread_submit_t *submit = VIP_NULL;
        vip_bool_e ret = vip_true_e;
        vip_bool_e exist = vip_false_e;
        vip_uint32_t i = 0;

        /* delivery the command buffer to multi thread processing */
        status = gckvip_os_lock_mutex(device->mt_mutex);
        if (status != VIP_SUCCESS) {
            PRINTK_E("failed to lock multiple thread mutex\n");
            gcGoOnError(status);
        }

        /* get a empty multi thread object */
        if (0 == device->mt_hashmap.idle_list->right_index) {
            volatile gckvip_multi_thread_submit_t *new_mt_submit = VIP_NULL;
            vip_status_e status = VIP_SUCCESS;
            status = gckvip_os_allocate_memory(sizeof(gckvip_multi_thread_submit_t) *
                                               (device->mt_hashmap.size + HASH_MAP_CAPABILITY),
                                               (void**)&new_mt_submit);
            if (VIP_SUCCESS != status) {
                PRINTK_E("multi thread is full, not support so many threads\n");
                gckvip_os_unlock_mutex(device->mt_mutex);
                gcGoOnError(VIP_ERROR_FAILURE);
            }
            status = gckvip_hashmap_expand(&device->mt_hashmap);
            if (VIP_SUCCESS != status) {
                gckvip_os_free_memory((void*)new_mt_submit);
                PRINTK_E("multi thread is full, not support so many threads\n");
                gckvip_os_unlock_mutex(device->mt_mutex);
                gcGoOnError(VIP_ERROR_FAILURE);
            }

            gckvip_os_zero_memory((void*)new_mt_submit,
                                  sizeof(gckvip_multi_thread_submit_t) * (device->mt_hashmap.size));
            gckvip_os_lock_mutex(device->mt_mutex_expand);
            gckvip_os_memcopy((void*)new_mt_submit, (void*)device->mt_submit,
                              sizeof(gckvip_multi_thread_submit_t) * (device->mt_hashmap.size - HASH_MAP_CAPABILITY));
            gckvip_os_free_memory((void*)device->mt_submit);
            device->mt_submit = new_mt_submit;
            gckvip_os_unlock_mutex(device->mt_mutex_expand);
            for (i = device->mt_hashmap.size - HASH_MAP_CAPABILITY; i < device->mt_hashmap.size; i++) {
                volatile gckvip_multi_thread_submit_t *submit = &device->mt_submit[i];
                status = gckvip_os_create_signal((gckvip_signal*)&submit->wait_signal);
                if (status != VIP_SUCCESS) {
                    PRINTK_E("failed to create a signal wait_signal\n");
                    gckvip_os_unlock_mutex(device->mt_mutex);
                    gcGoOnError(VIP_ERROR_IO);
                }
                gckvip_os_set_signal(submit->wait_signal, vip_false_e);
            }
        }

        i = gckvip_hashmap_put(&device->mt_hashmap, cmd_handle, &exist);
        submit = &device->mt_submit[i];
        if (vip_true_e == exist) {
            PRINTK_E("fail to submit, repeate command handle=0x%x\n", commit->cmd_handle);
            gckvip_os_unlock_mutex(device->mt_mutex);
            gcGoOnError(VIP_ERROR_FAILURE);
        }

        if (VIP_NULL == submit->queue_data) {
            status = gckvip_os_allocate_memory(sizeof(gckvip_queue_data_t), (void**)&submit->queue_data);
            if (VIP_SUCCESS != status) {
                PRINTK_E("fail to submit, allocate queue_data fail\n");
                gcGoOnError(VIP_ERROR_FAILURE);
            }
            gckvip_os_zero_memory((void*)submit->queue_data, sizeof(gckvip_queue_data_t));
        }

        submit->status = GCKVIP_SUBMIT_WITH_DATA;
        submit->submit_handle.cmd_handle = cmd_handle;
        submit->submit_handle.cmd_logical = GCKVIPUINT64_TO_PTR(commit->cmd_logical);
        submit->submit_handle.cmd_physical = commit->cmd_physical;
        submit->submit_handle.cmd_size = commit->cmd_size;
        status = gckvip_database_get_handle(&context->videomem_database, commit->last_cmd_handle,
                                            (vip_ptr *)&submit->submit_handle.last_cmd_handle);
        if (status != VIP_SUCCESS) {
            PRINTK_E("failed to convert command buffer video memory id, handle=0x%x\n",
                     commit->last_cmd_handle);
            gckvip_os_unlock_mutex(device->mt_mutex);
            gcGoOnError(status);
        }
        submit->submit_handle.last_cmd_logical = GCKVIPUINT64_TO_PTR(commit->last_cmd_logical);
        submit->submit_handle.last_cmd_physical = commit->last_cmd_physical;
        submit->submit_handle.last_cmd_size = commit->last_cmd_size;
        submit->submit_handle.device_id = commit->device_id;
#if vpmdENABLE_PREEMPTION
        submit->queue_data->priority = commit->priority;
#endif
        submit->queue_data->v1 = i;
#if vpmdENABLE_CANCELATION
        submit->queue_data->v2 = VIP_ERROR_FAILURE;
#endif
        status = gckvip_os_unlock_mutex(device->mt_mutex);
        if (status != VIP_SUCCESS) {
            PRINTK_E("failed to unlock multiple thread mutex\n");
            gcGoOnError(status);
        }

        gckvip_os_set_signal(submit->wait_signal, vip_false_e);
        ret = gckvip_queue_write(&device->mt_input_queue, submit->queue_data);
        if (!ret) {
            PRINTK_E("failed to write task into queue\n");
            submit->status = GCKVIP_SUBMIT_EMPTY;
            gckvip_hashmap_get(&device->mt_hashmap, submit->submit_handle.cmd_handle, vip_true_e);
            gcGoOnError(VIP_ERROR_FAILURE);
        }
    }
    else {
        PRINTK_E("mutli task thread is not running\n");
        gcGoOnError(VIP_ERROR_NOT_SUPPORTED);
    }
#else
    {
        gckvip_submit_t submit;
        submit.cmd_handle = cmd_handle;
        submit.cmd_logical = GCKVIPUINT64_TO_PTR(commit->cmd_logical);
        submit.cmd_physical = commit->cmd_physical;
        submit.cmd_size = commit->cmd_size;
        gcOnError(gckvip_database_get_handle(&context->videomem_database,
                                             commit->last_cmd_handle,
                                             &submit.last_cmd_handle));
        submit.last_cmd_logical = GCKVIPUINT64_TO_PTR(commit->last_cmd_logical);
        submit.last_cmd_physical = commit->last_cmd_physical;
        submit.last_cmd_size = commit->last_cmd_size;
        submit.device_id = commit->device_id;
        gcOnError(gckvip_cmd_submit(context, &submit));
    }
#endif
    device->execute_count++;
    PRINTK_I("end do submit..\n");

    return status;
onError:
    PRINTK_E("fail to do command submit\n");
    return status;
}
