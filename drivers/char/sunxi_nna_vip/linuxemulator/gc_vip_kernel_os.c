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

#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <stdio.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/time.h>
#include <semaphore.h>
#include <stdint.h>
#include "gc_vip_kernel_port.h"
#include "gc_vip_kernel.h"
#include "gc_vip_kernel_drv.h"
#include "gc_vip_kernel_heap.h"
#include "gc_vip_hardware.h"
#include "gc_vip_kernel_debug.h"
#include <gc_vip_kernel_os.h>
#include "gc_vip_kernel_cmodel.h"
#include "gc_vip_platform_config_linuxemulator.h"

volatile vip_uint32_t irq_flag[gcdMAX_CORE];
vip_uint32_t  irq_queue[gcdMAX_CORE];
vip_uint32_t vip_reg[gcdMAX_CORE];

typedef struct _gckvip_signal_t
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    vip_bool_e  value;
} gckvip_signal_t;

typedef struct _device
{
    /* HW required objects. */
    vip_uint32_t      irq_event;
    gckvip_semaphore  semaphore;
    void           *contiguous;
} device_t;

static device_t    s_Device = {0};

#define GET_DEVICE() \
    device_t *device = &s_Device

void irq_handler(void * data)
{
    /* Check whether it's VIP's interrupt */
    vip_uint32_t irq_value = 0 ;
    irq_value = CModelReadRegister(0x00010);

    if (irq_value != 0) {
        device_t *device = (device_t *) data;
        device->irq_event |= irq_value;
        gckvip_os_wakeup_interrupt((void*)(&device->semaphore));
    }
}

static void InitEmulator();
static void DestroyEmulator();

static void __attribute__((constructor)) InitEmulator()
{
    GET_DEVICE();
    /* Construct Cmodel object: acquire the memory pool. */
    CModelConstructor(CMODEL_MEMORY_SIZE);

    device->contiguous = CModelAllocate((VIDEO_MEMORY_HEAP_SIZE + AXI_SRAM_SIZE * 2 + VIP_SRAM_SIZE * 5), vip_true_e);

    /* Register irq handler. */
    CModelRegisterIrq(irq_handler, device);

    /* Create the semaphore to setup irq handler loop. */
    gckvip_os_create_semaphore(&device->semaphore);

    return;
}

static void __attribute__((destructor)) DestroyEmulator()
{
    GET_DEVICE();

    /* Release the signal object. */
    gckvip_os_relase_semaphore(&device->semaphore);
    gckvip_os_destory_semaphore(&device->semaphore);

    if (device->contiguous != NULL) {
        CModelFree((cmMemoryInfo *)device->contiguous);
        device->contiguous = NULL;
    }

    /* Destroy the C-Model. */
    CModelDestructor();

    return;
}

/*
@brief Get all hardware basic information.
*/
vip_status_e gckvip_drv_get_hardware_info(
    gckvip_hardware_info_t *info
    )
{
    #define SRAM_ALIGN_GAP  (4*1024)

    vip_status_e status = VIP_SUCCESS;
    if (info == VIP_NULL) {
        PRINTK("failed to hardware info, parameter is NULLi.\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    info->core_count = CORE_COUNT;

      {
      vip_uint32_t core =0;
      for (core =0; core < CORE_COUNT; core++) {
          info->vip_reg[core] = (void*)&vip_reg[core];
          info->irq_queue[core] = (void*)&irq_queue[core];
          info->irq_flag[core] = (vip_uint32_t*)&irq_flag[core];
      }
    }

#if vpmdENABLE_VIDEO_MEMORY_HEAP
    {
    GET_DEVICE();
    cmMemoryInfo *mem = (cmMemoryInfo *)device->contiguous;
    info->cpu_virtual = (void*)mem->logical;
    info->cpu_virtual_kernel = (void*)mem->logical;
    info->vip_physical = (phy_address_t)mem->physical;
    info->vip_memsize = VIDEO_MEMORY_HEAP_SIZE;
    }
#endif
    /* caculate logical device */
    {
        vip_uint32_t  iter = 0;
        for (iter = 0; iter < CORE_COUNT; iter++) {
            info->device_core_number[iter] = LOGIC_DEVICES_WITH_CORE[iter];
        }
    }

    info->axi_sram_size = AXI_SRAM_SIZE;
#if AXI_SRAM_BASE_ADDRESS
    info->axi_sram_base = AXI_SRAM_BASE_ADDRESS;
#else
    /* the end 10Mbytes reserve for AXI-SRAM */;
    info->axi_sram_base = (vip_uint32_t)info->vip_physical +
                            GCVIP_ALIGN((VIDEO_MEMORY_HEAP_SIZE + SRAM_ALIGN_GAP), SRAM_ALIGN_GAP);
#endif

    info->vip_sram_size = VIP_SRAM_SIZE;
#if VIP_SRAM_BASE_ADDRESS
    info->vip_sram_base = VIP_SRAM_BASE_ADDRESS;
#else
    info->vip_sram_base = GCVIP_ALIGN((info->axi_sram_base + SRAM_ALIGN_GAP), SRAM_ALIGN_GAP);
#endif

    return status;
}

vip_status_e gckvip_drv_set_power_clk(
    vip_uint32_t core,
    vip_uint32_t state
    )
{
    return VIP_SUCCESS;
}

/*
@brief Get CORE information.
@param core_count, the number of vip core.
*/
vip_status_e gckvip_drv_get_core_info(
    vip_uint32_t *core_count)
{
    *core_count = 1;
    return VIP_SUCCESS;
}

/*
@ brief
    Read a register.
*/
vip_status_e gckvip_os_read_reg(
    volatile void *reg_base,
    vip_uint32_t Address,
    vip_uint32_t *Data
    )
{
    *Data = CModelReadRegister(Address);
    return VIP_SUCCESS;
}

/*
@ brief
    Write a register.
*/
vip_status_e gckvip_os_write_reg(
    volatile void *reg_base,
    vip_uint32_t Address,
    vip_uint32_t Data
    )
{
    CModelWriteRegister(Address, Data);

#if vpmdENABLE_CAPTURE_IN_KERNEL
    PRINTK("@[register.write 0x%05X 0x%08X]\n", Address, Data);
#endif

    return VIP_SUCCESS;
}

/*
@ brief
    Wait for interrupt.
*/
vip_status_e gckvip_os_wait_interrupt(
    IN void    *irq_queue,
    IN volatile vip_uint32_t* volatile irq_flag,
    IN vip_uint32_t TimeOut,
    IN vip_uint32_t Mask
    )
{
    vip_status_e error = VIP_SUCCESS;
    GET_DEVICE();

    error = gckvip_os_wait_semaphore(&device->semaphore);

    switch (error) {
    case VIP_SUCCESS:
        error = VIP_SUCCESS;
        break;
    case VIP_ERROR_TIMEOUT:
        error = VIP_ERROR_TIMEOUT;
        break;

    default:
        error = VIP_ERROR_IO;
        break;
    }

    return error;
}

vip_status_e gckvip_os_wakeup_interrupt(
    IN void *irq_queue
    )
{
    vip_status_e status = VIP_SUCCESS;

    gckvip_semaphore *semaphore = (gckvip_semaphore*)irq_queue;
    gckvip_os_relase_semaphore(semaphore);

    return status;
}

vip_status_e CallEmulator(gckvip_command_id_e Command, void *Data)
{
    return gckvip_kernel_call(Command, Data);
}

/*
@brief Do some initialization works in KERNEL_OS layer.
@param video_mem_size, the size of video memory heap.
*/
vip_status_e gckvip_os_init(
    vip_uint32_t video_mem_size
    )
{
    vip_status_e status = VIP_SUCCESS;

    return status;
}

/*
@brief Do some un-initialization works in KERNEL_OS layer.
*/
vip_status_e gckvip_os_close(void)
{
    vip_status_e status = VIP_SUCCESS;

    return status;
}


/************************************************************************/
/************************************************************************/
/* *********************need to be ported functions**********************/
/************************************************************************/
/************************************************************************/
/************************************************************************/
/*
@brief Allocate system memory in kernel space.
@param video_mem_size, the size of video memory heap.
@param size, Memory size to be allocated.
@param memory, Pointer to a variable that will hold the pointer to the memory.
*/
vip_status_e gckvip_os_allocate_memory(
    IN  vip_uint32_t size,
    OUT void **memory
    )
{
    void *allo_memory = VIP_NULL;

    if (size == 0 || !memory) {
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    allo_memory = malloc(size);
    if (!allo_memory) {
        return VIP_ERROR_OUT_OF_MEMORY;
    }

    *memory = allo_memory;

    return VIP_SUCCESS;
}

/*
@brief Free system memory in kernel space.
@param memory, Memory to be freed.
*/
vip_status_e gckvip_os_free_memory(
    IN void *memory
    )
{
    if (!memory) {
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    free(memory);

    return VIP_SUCCESS;
}

/*
@brief Set memory to zero.
@param memory, Memory to be set to zero.
@param size, the size of memory.
*/
vip_status_e gckvip_os_zero_memory(
    IN void *memory,
    IN vip_uint32_t size
    )
{
    vip_uint8_t *data = (vip_uint8_t*)memory;

    if (!memory) {
        printf("failed to zero memory, memory is NULL\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }
    memset(data, 0, size);

    return VIP_SUCCESS;
}

/*
@brief copy src memory to dst memory.
@param dst, Destination memory.
@param src. Source memory
@param size, the size of memory should be copy.
*/
vip_status_e gckvip_os_memcopy(
    IN void *dst,
    IN const void *src,
    IN vip_uint32_t size
    )
{
    if (!dst || !src || size == 0) {
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    memcpy(dst, src, size);

    return VIP_SUCCESS;
}

/*
@brief Delay execution of the current thread for a number of milliseconds.
@param ms, delay unit ms. Delay to sleep, specified in milliseconds.
*/
void gckvip_os_delay(
    IN vip_uint32_t ms
    )
{
    usleep(ms * 1000);
}

/*
@brief Delay execution of the current thread for a number of microseconds.
@param ms, delay unit us. Delay to sleep, specified in microseconds.
*/
void gckvip_os_udelay(
    IN vip_uint32_t us
    )
{
    usleep(us);
}

/*
@brief Print string on console.
@param msg, which message to be print.
*/
vip_status_e gckvip_os_print(
    gckvip_log_level level,
    IN const char *message,
    ...
    )
{
    vip_status_e status = VIP_SUCCESS;
#define DEFAULT_ENABLE_LOG     1

#if (vpmdENABLE_DEBUG_LOG > 2)
    if ((level == GCKVIP_DEBUG) || (level == GCKVIP_INFO)) {
        static vip_uint32_t log_val = DEFAULT_ENABLE_LOG;
        static vip_uint8_t read_env = 0;
        if (0 == read_env) {
            char *env = VIP_NULL;
            env = getenv("VIP_LOG_LEVEL");
            if (env) {
                log_val = atoi(env);
            }
            read_env = 1;
        }

        if (0 == log_val) {
            return VIP_SUCCESS;
        }
    }
#endif

    va_list arg;
    va_start(arg, message);
    vprintf(message, arg);
    va_end(arg);

    return status;
}

/*
@brief Print string to buffer.
@param, size, the size of msg.
@param msg, which message to be print.
*/
vip_status_e gckvip_os_snprint(
    IN vip_char_t *buffer,
    IN vip_uint32_t size,
    IN const vip_char_t *message,
    ...
    )
{
    vip_status_e status = VIP_SUCCESS;

    va_list arg;
    va_start(arg, message);
    vsprintf(buffer, message, arg);
    va_end(arg);

    return status;
}

/*
@brief get the current system time. return value unit us, microsecond
*/
vip_uint64_t  gckvip_os_get_time(void)
{
    struct timeval tv;
    vip_uint64_t time = 0;

    gettimeofday(&tv, 0);
    time = (tv.tv_sec * (vip_uint64_t)1000000) + tv.tv_usec;

    return time;
}

/*
@brief Memory barrier function for memory consistency.
*/
vip_status_e gckvip_os_memorybarrier(void)
{
    return VIP_SUCCESS;
}

/*
@brief Get kernel space process id
*/
vip_uint32_t gckvip_os_get_pid(void)
{
     vip_uint32_t pid = 0;

    pid = (vip_uint32_t)getpid();
    return pid;
}

/*
@brief Get kernel space thread id
*/
vip_uint32_t gckvip_os_get_tid(void)
{
    vip_uint32_t tid = 0;

    tid = (vip_uint32_t)pthread_self();
    return tid;
}

#if vpmdENABLE_FLUSH_CPU_CACHE
/*
@brief 1.Flush CPU cache for video memory which allocated from heap.
       2.Flush external cache device if support.
@param physical, VIP physical address.
        gckvip_drv_get_cpuphysical to convert the physical address should be flush CPU cache.
@param logical, the logical address should be flush.
@param size, the size of the memory should be flush.
@param type The type of operate cache. see gckvip_cache_type_e.
*/
vip_status_e gckvip_os_flush_cache(
    IN phy_address_t physical,
    IN void *logical,
    IN vip_uint32_t size,
    IN vip_uint8_t type
    )
{
    vip_status_e status = VIP_SUCCESS;

    /* 1. Flush video memory heap CPU's cache */
    /* Vivante's video memory heap non-cache */

    return status;
}
#endif

/*
@brief Create a mutex for multiple thread.
@param mutex, create mutex pointer.
*/
vip_status_e gckvip_os_create_mutex(
    OUT gckvip_mutex *mutex
    )
{
    vip_status_e error = VIP_SUCCESS;
    vip_int32_t ret = 0;
    pthread_mutex_t *pmutex = VIP_NULL;

    pmutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if (pmutex == VIP_NULL) {
        printf("fail to allocate for pmutex\n");
        return VIP_ERROR_IO;
    }

    ret = pthread_mutex_init(pmutex, VIP_NULL);
    if(ret != 0) {
        printf("create mutex failed.\n");
        error = VIP_ERROR_NOT_SUPPORTED;
    }

    *mutex = pmutex;

    return error;
}

vip_status_e gckvip_os_lock_mutex(
    IN gckvip_mutex mutex
    )
{
    vip_status_e error = VIP_SUCCESS;
    vip_int32_t ret = 0;

    if (mutex == VIP_NULL) {
        printf("failed to lock, mutex is NULL\n");
        return VIP_ERROR_OUT_OF_MEMORY;
    }

    ret = pthread_mutex_lock((pthread_mutex_t*)mutex);
    if(ret != 0) {
        printf("a thread lock mutex failed.\n");
        error = VIP_ERROR_NOT_SUPPORTED;
    }

    return error;
}

vip_status_e gckvip_os_unlock_mutex(
    IN gckvip_mutex mutex
    )
{
    vip_status_e error = VIP_SUCCESS;
    vip_int32_t ret = 0;

    if (mutex == VIP_NULL) {
        printf("failed to unlock, mutex is NULL\n");
        return VIP_ERROR_OUT_OF_MEMORY;
    }

    ret = pthread_mutex_unlock((pthread_mutex_t*)mutex);
    if(ret != 0) {
        printf("a thread unlock mutex failed.\n");
        error = VIP_ERROR_NOT_SUPPORTED;
    }

    return error;
}

/*
@brief Destroy a mutex which create in gckvip_os_create_mutex..
@param mutex, create mutex pointer.
*/
vip_status_e gckvip_os_destroy_mutex(
    IN gckvip_mutex mutex
    )
{
    if (mutex == VIP_NULL) {
        printf("failed to destory, mutex is NULL\n");
        return VIP_ERROR_OUT_OF_MEMORY;
    }

    pthread_mutex_destroy((pthread_mutex_t*)mutex);
    free(mutex);

    return VIP_SUCCESS;
}

vip_status_e gckvip_os_create_semaphore(
    gckvip_semaphore  *semaphore
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t error;

    if (semaphore == VIP_NULL) {
        printf("failed to create semaphore, semaphore is NULL\n");
        return VIP_ERROR_FAILURE;
    }
    /* Create the semaphore. */
    error = sem_init(&semaphore->semaphore, 0, 0);
    if (error != 0)
    {
        printf("failed to init semaphore.\n");
        gcOnError(VIP_ERROR_FAILURE);
    }
    return status;

onError:
    return status;
}

vip_status_e gckvip_os_destory_semaphore(
    gckvip_semaphore *semaphore
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t error;

    /* Delete the semaphore. */
    error = sem_destroy(&semaphore->semaphore);
    if (error != 0)
    {
        printf("failed to destory semaphore.\n");
        gcOnError(VIP_ERROR_FAILURE);
    }

    return status;

onError:
    return status;
}

vip_status_e gckvip_os_wait_semaphore(
    gckvip_semaphore *semaphore
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t error;

    /* Lock the semaphore. */
    error = sem_wait(&semaphore->semaphore);
    if (error != 0)
    {
        printf("failed to wait semaphore.\n");
        gcOnError(VIP_ERROR_FAILURE);
    }

    return status;

onError:
    return status;
}

vip_status_e gckvip_os_relase_semaphore(
    gckvip_semaphore *semaphore
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t error;

    /* Unlock the semaphore. */
    error = sem_post(&semaphore->semaphore);
    if ((error != 0))
    {
        printf("failed to relase semaphore.\n");
        gcOnError(VIP_ERROR_FAILURE);
    }
    return status;

onError:
    return status;
}

#if vpmdENABLE_MULTIPLE_TASK || vpmdPOWER_OFF_TIMEOUT
vip_status_e gckvip_os_create_thread(
    gckvip_thread_func func,
    vip_ptr parm,
    gckvip_thread *handle
    )
{
    vip_int32_t error;
    if ((handle != VIP_NULL) && (func != VIP_NULL)) {
        error = pthread_create((pthread_t*)handle, VIP_NULL, (void*)func, parm);
        if (error != 0) {
            PRINTK("failed to create thread\n");
            return VIP_ERROR_IO;
        }
    }
    else {
        PRINTK("failed to create thread. parameter is NULL\n");
        *handle = VIP_NULL;
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    return VIP_SUCCESS;
}

vip_status_e gckvip_os_destroy_thread(
    gckvip_thread handle
    )
{
    if (handle != VIP_NULL) {
        pthread_kill((pthread_t*)handle);
    }
    else {
        PRINTK("failed to destroy thread");
        return VIP_ERROR_IO;
    }

    return VIP_SUCCESS;
}

#endif

/*
@brief Create a atomic.
@param atomic, create atomic pointer.
*/
vip_status_e gckvip_os_create_atomic(
    OUT gckvip_atomic *atomic
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t *c_atomic = VIP_NULL;
    gcIsNULL(atomic);

    /* please add atomic implement in here if system supports atomic
        otherwise use below function to instead atmoic */

    status = gckvip_os_allocate_memory(sizeof(vip_uint32_t), (void**)&c_atomic);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to allocate memory for atomic\n");
        return VIP_ERROR_IO;
    }

    *c_atomic = 0;
    *atomic = (vip_ptr)c_atomic;

onError:
    return status;
}

/*
@brief Destroy a atomic which create in gckvip_os_create_atomic
@param atomic, create atomic pointer.
*/
vip_status_e gckvip_os_destroy_atomic(
    IN gckvip_atomic atomic
    )
{
    vip_status_e status = VIP_SUCCESS;
    gcIsNULL(atomic);

    gckvip_os_free_memory(atomic);

onError:
    return status;
}

/*
@brief Set the 32-bit value protected by an atomic.
@param atomic, create atomic pointer.
*/
vip_status_e gckvip_os_set_atomic(
    IN gckvip_atomic atomic,
    IN vip_uint32_t value
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t *data = (vip_uint32_t*)atomic;
    gcIsNULL(atomic);

    *data = value;

onError:
    return status;
}

/*
@brief Get the 32-bit value protected by an atomic.
@param atomic, create atomic pointer.
*/
vip_uint32_t gckvip_os_get_atomic(
    IN gckvip_atomic atomic
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t value = 0xFFFFFFFE;
    vip_uint32_t *data = (vip_uint32_t*)atomic;
    gcIsNULL(atomic);

    value = *data;
onError:
    return value;
}

/*
@brief Increase the 32-bit value protected by an atomic.
@param atomic, create atomic pointer.
*/
vip_status_e gckvip_os_inc_atomic(
    IN gckvip_atomic atomic
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t *data = (vip_uint32_t*)atomic;
    gcIsNULL(atomic);

    *data += 1;

onError:
    return status;
}

/*
@brief Decrease the 32-bit value protected by an atomic.
@param atomic, create atomic pointer.
*/
vip_status_e gckvip_os_dec_atomic(
    IN gckvip_atomic atomic
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t *data = (vip_uint32_t*)atomic;
    gcIsNULL(atomic);

    *data -= 1;

onError:
    return status;
}

#if vpmdENABLE_MULTIPLE_TASK || vpmdENABLE_SUSPEND_RESUME || vpmdPOWER_OFF_TIMEOUT
/* Create a signal. */
vip_status_e gckvip_os_create_signal(
    OUT gckvip_signal *handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_signal_t *sg = VIP_NULL;
    int error;

    if (handle == VIP_NULL) {
        printf("failed to create signal, param is NULL\n");
        return VIP_ERROR_FAILURE;
    }

    /* Allocate the signal. */
    gcOnError(gckvip_os_allocate_memory(sizeof(gckvip_signal_t), (vip_ptr*)&sg));

    /* Copy the manual reset flag. */
    sg->value = vip_false_e;

    /* Initialize the mutex. */
    error = pthread_mutex_init(&sg->mutex, VIP_NULL);
    if (error != 0) {
        printf("init mutex fail for signal.\n");
        gcOnError(VIP_ERROR_FAILURE);
    }

    /* Initialize the condition. */
    error = pthread_cond_init(&sg->cond, VIP_NULL);
    if (error != 0) {
        printf("init cond fail for signal.\n");
        gcOnError(VIP_ERROR_FAILURE);
    }

    /* Return the signal. */
    *handle = (gckvip_signal_t*)sg;

    return status;

onError:
    if (sg != VIP_NULL)
    {
        /* Destroy te mutex. */
        pthread_mutex_destroy(&sg->mutex);
        /* Destroy the condition. */
        pthread_cond_destroy(&sg->cond);
    }
    return status;
}

/* Destroy a signal. */
vip_status_e gckvip_os_destroy_signal(
    IN gckvip_signal handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    int error;
    gckvip_signal_t *sg = (gckvip_signal_t*)handle;

    /* Destroy the mutex. */
    error = pthread_mutex_destroy(&sg->mutex);
    if (error != 0) {
        printf("destory mutex fail for signal.\n");
        gcOnError(VIP_ERROR_FAILURE);
    }

    /* Destroy the condition. */
    error = pthread_cond_destroy(&sg->cond);
    if (error != 0) {
        printf("destory cond fail for signal.\n");
        gcOnError(VIP_ERROR_FAILURE);
    }

    gckvip_os_free_memory(handle);

    return status;

onError:
    return status;
}

# define TIMEVAL_TO_TIMESPEC(tv, ts) {     \
  (ts)->tv_sec = (tv)->tv_sec;             \
  (ts)->tv_nsec = (tv)->tv_usec * 1000;    \
}
/* Wait for a signal. */
vip_status_e gckvip_os_wait_signal(
    IN gckvip_signal handle,
    IN vip_uint32_t timeout
    )
{
    vip_status_e status = VIP_SUCCESS;
    int error;

    gckvip_signal_t *sg = (gckvip_signal_t*)handle;
    if (sg == VIP_NULL) {
        printf("failed to wait signal, parameter is NULL\n");
        gcGoOnError(VIP_ERROR_FAILURE);
    }

    /* Lock the mutex. */
    error = pthread_mutex_lock(&sg->mutex);
    if (error != 0) {
        printf("fail to lock mutex for signal.\n");
        gcOnError(VIP_ERROR_FAILURE);
    }

    if (sg->value)
    {
        /* Success. */
        status = VIP_SUCCESS;
    }

    else if (timeout == 0)
    {
        /* Wait for the condition. */
        error = pthread_cond_wait(&sg->cond, &sg->mutex);
        if (error != 0) {
            printf("fail wait cond for signal.\n");
            gcOnError(VIP_ERROR_FAILURE);
        }
    }
    else
    {
        struct timeval tv;
        struct timespec ts;

        /* Get the current time. */
        gettimeofday(&tv, VIP_NULL);
        TIMEVAL_TO_TIMESPEC(&tv, &ts);

        if (timeout != 0)
        {
            /* Add the timeout. */
            ts.tv_sec  += timeout / 1000;
            ts.tv_nsec += (timeout % 1000) * 1000;
            if (ts.tv_nsec > 1000000)
            {
                ts.tv_sec += 1;
            }
        }

        /* Wait for the condition. */
        error = pthread_cond_timedwait(&sg->cond, &sg->mutex, &ts);
        if (error != 0) {
            printf("fail wait cond for signal.\n");
            gcOnError(VIP_ERROR_FAILURE);
        }
    }

    /* Reset signal value. */
    sg->value = vip_false_e;

    /* Unlock the mutex. */
    error = pthread_mutex_unlock(&sg->mutex);
    if (error != 0) {
        printf("fail unlock for signal.\n");
        gcOnError(VIP_ERROR_FAILURE);
    }

    return status;

onError:
    /* Unlock the mutex. */
    pthread_mutex_unlock(&sg->mutex);
    return status;
}

/* Signal a signal. */
vip_status_e gckvip_os_set_signal(
    IN gckvip_signal handle,
    IN vip_bool_e state
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_bool_e locked = vip_false_e;
    int error;

    gckvip_signal_t *sg = (gckvip_signal_t*)handle;
    if (sg == VIP_NULL) {
        printf("failed to wait signal, parameter is NULL\n");
        gcGoOnError(VIP_ERROR_FAILURE);
    }

    /* Lock the mutex. */
    error = pthread_mutex_lock(&sg->mutex);
    if (error != 0) {
        printf("fail lock for signal.\n");
        gcOnError(VIP_ERROR_FAILURE);
    }
    locked = vip_true_e;

    sg->value = state;
    if (state)
    {
        /* Unlock the semaphore. */
        error = pthread_cond_signal(&sg->cond);
        if (error != 0) {
            printf("fail lock for signal.\n");
            gcOnError(VIP_ERROR_FAILURE);
        }
    }
    /* Unlock the mutex. */
    error  = pthread_mutex_unlock(&sg->mutex);
    if (error != 0) {
        printf("fail unlock for signal.\n");
        gcOnError(VIP_ERROR_FAILURE);
    }
    locked = vip_false_e;

    return status;

onError:
    if (locked)
    {
        /* Unlock the mutex. */
        pthread_mutex_unlock(&sg->mutex);
    }
    return status;
}
#endif

#if vpmdPOWER_OFF_TIMEOUT
typedef void (*gckvip_timer_function)(vip_ptr);
/*
@brief create a system timer callback.
@param func, the callback function of this timer.
@param param, the paramete of the timer.
*/
vip_status_e gckvip_os_create_timer(
    IN vip_ptr func,
    IN vip_ptr param,
    OUT gckvip_timer *timer
    )
{
    vip_status_e status = VIP_SUCCESS;
    return status;
}

vip_status_e gckvip_os_start_timer(
    IN gckvip_timer timer,
    IN vip_uint32_t time_out
    )
{
    return VIP_SUCCESS;
}

vip_status_e gckvip_os_stop_timer(
    IN gckvip_timer timer
    )
{
    return VIP_SUCCESS;
}

/*
@brief destroy a system timer
*/
vip_status_e gckvip_os_destroy_timer(
    IN gckvip_timer timer
    )
{

    return VIP_SUCCESS;
}
#endif

