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


#define NEW_CMODEL_PIPE

#ifdef NEW_CMODEL_PIPE
#include "aqcm.hpp"
#include <stdarg.h>
#include <string.h>
#include "gc_vip_kernel_cmodel.h"
#include <gc_vip_platform_config_linuxemulator.h>

extern "C" {
#include <gc_vip_kernel_port.h>
#include <gc_vip_kernel_os.h>
}

/* Customized memory object for cmodel. */
class CMemory : public aqMemoryBase
{
protected:
    size_t          m_PageCount;
    void **         m_PageMap;
    gckvip_mutex   memory_mutex;

public:
    CMemory(size_t TotalMemory)
    {
        // Allocate the memory.
        m_PageCount = TotalMemory >> 12;
        m_PageMap   = new void * [m_PageCount];
        memset(m_PageMap, 0, sizeof(void *) * m_PageCount);

        // Initialize the mutex.
        gckvip_os_create_mutex(&memory_mutex);
    }

    virtual ~CMemory(void)
    {
        // Free the memory.
        delete [] m_PageMap;

        // Destroy the mutex.
        gckvip_os_destroy_mutex(memory_mutex);

    }

    virtual U32 GetMemorySize(void)
    {
        // Return memory size.
        return static_cast<U32>(m_PageCount << 12);
    }

    virtual bool SetMemorySize(U32 MemorySize)
    {
        return false;
    }

    virtual U32 Read(U32 Address)
    {
        void * logical;
        U32 data = 0;

        // Get logical pointer.
        logical = GetLogical(Address);
        if (logical != NULL)
        {
            // Read data from the memory.
            data = *reinterpret_cast<U32 *>(logical);
        }

        // Return the data.
        return data;
    }

    virtual void Write(U32 Address, U04 Mask, U32 Data)
    {
        void * logical;

        // Get logical pointer.
        logical = GetLogical(Address);
        if (logical != NULL)
        {
            // Dispatch on mask.
            switch (Mask)
            {
                case 0x1:
                    // Lower 8-bit write.
                    reinterpret_cast<U08 *>(logical)[0] = static_cast<U08>(GETBITS(Data, 7, 0));
                    break;

                case 0x2:
                    // Mid-lower 8-bit write.
                    reinterpret_cast<U08 *>(logical)[1] = static_cast<U08>(GETBITS(Data, 15, 8));
                    break;

                case 0x4:
                    // Mid-upper 8-bit write.
                    reinterpret_cast<U08 *>(logical)[2] = static_cast<U08>(GETBITS(Data, 23, 16));
                    break;

                case 0x8:
                    // Upper 8-bit write.
                    reinterpret_cast<U08 *>(logical)[3] = static_cast<U08>(GETBITS(Data, 31, 24));
                    break;

                case 0x3:
                    // Lower 16-bit write.
                    reinterpret_cast<U16 *>(logical)[0] = static_cast<U16>(GETBITS(Data, 15, 0));
                    break;

                case 0xC:
                    // Upper 16-bit write.
                    reinterpret_cast<U16 *>(logical)[1] = static_cast<U16>(GETBITS(Data, 31, 16));
                    break;

                case 0xF:
                    // 32-bit write.
                    reinterpret_cast<U32 *>(logical)[0] = Data;
                    break;

                default:
                    if (Mask & 0x1)
                    {
                        // Write bits [7:0].
                        reinterpret_cast<U08 *>(logical)[0] = static_cast<U08>(GETBITS(Data, 7, 0));
                    }
                    if (Mask & 0x2)
                    {
                        // Write bits [15:8].
                        reinterpret_cast<U08 *>(logical)[1] = static_cast<U08>(GETBITS(Data, 15, 8));
                    }
                    if (Mask & 0x4)
                    {
                        // Write bits [23:16].
                        reinterpret_cast<U08 *>(logical)[2] = static_cast<U08>(GETBITS(Data, 23, 16));
                    }
                    if (Mask & 0x8)
                    {
                        // Write bits [31:24].
                        reinterpret_cast<U08 *>(logical)[3] = static_cast<U08>(GETBITS(Data, 31, 24));
                    }
                    break;
            }
        }
    }

    virtual void* GetMemoryAddress(void) const
    {
        return NULL;
    }

    void * GetLogical(U32 Physical)
    {
        // Convert physical address to page index.
        U32 page = (Physical - gcdCONTIGUOUS_BASE) >> 12;
        if (page >= m_PageCount)
        {
            // Page is out-of-bounds.
            return NULL;
        }

        // Get logical address.
        void * logical = m_PageMap[page];
        if (logical == NULL)
        {
            // Page is not allocated.
            return NULL;
        }

        // Add page offset to logical address.
        return static_cast<U08 *>(logical) + ((Physical-gcdCONTIGUOUS_BASE) & 0xFFF);
    }

    U32 GetPhysical(void * Logical)
    {
        // Cast the pointer to byte-addressable memory.
        U08 * address = static_cast<cmUInt8 *>(Logical);

        for (cmSize i = 0; i < m_PageCount; i++)
        {
            // Get mapped address.
            U08 * map = static_cast<U08 *>(m_PageMap[i]);

            // Check if the pointer is inside this mapped page.
            if ((address >= map) && (address < map + 4096))
            {
                // Return physical address for this pointer.
                return static_cast<U32>((i << 12) + (address - map) + gcdCONTIGUOUS_BASE);
            }
        }

        // Invalid address.
        return ~0U;
    }

    cmMemoryInfo * AllocateContiguous(size_t Bytes)
    {
        // Compute number of required pages.
        size_t pages = (Bytes + 4095) >> 12;

        // Create a new memory info structure.
        cmMemoryInfo *memory = new cmMemoryInfo;
        if (memory == NULL)
        {
            // Out of memory.
            return NULL;
        }

        // Allocate a new memory blob.
        U08 * logical = new U08 [pages << 12];
        if (logical == NULL)
        {
            // Delete the memory info structure.
            delete memory;

            // Out of memory.
            return NULL;
        }

        // Lock the mutex.
        gckvip_os_lock_mutex(memory_mutex);

        // Process all pages.
        for (size_t i = 0; i < m_PageCount - pages; i++)
        {
            // Check if there are enough free pages.
            size_t j;
            for (j = 0; j < pages; j++)
            {
                if (m_PageMap[i + j] != NULL)
                {
                    // Page is in use, break loop.
                    break;
                }
            }

            // Check if this range is large enough.
            if (j == pages)
            {
                // Process all pages in the range.
                for (j = 0; j < pages; j++)
                {
                    // Map the pages.
                    m_PageMap[i + j] = static_cast<void *>(logical + (j << 12));
                }

                // Unlock the mutex.
                gckvip_os_unlock_mutex(memory_mutex);

                // Fill in the memory information.
                memory->physical = static_cast<U32>((i << 12) + gcdCONTIGUOUS_BASE);
                memory->logical  = logical;
                memory->bytes    = pages << 12;

                // Return the memory info structure.
                return memory;
            }
        }

        // Unlock the mutex.
        gckvip_os_unlock_mutex(memory_mutex);

        // Delete the memory blob.
        delete [] logical;

        // Delete the memory info structure.
        delete memory;

        // Out of memory.
        return NULL;
    }

    cmMemoryInfo * Allocate(size_t Bytes)
    {
        // Compute the required number of pages.
        size_t pages = (Bytes + 4095) >> 12;

        // Create a new memory info structure.
        cmMemoryInfo * memory = new cmMemoryInfo;
        if (memory == NULL)
        {
            // Out of memory.
            return NULL;
        }

        // Allocate a new memory blob.
        U08 * logical = new U08 [pages << 12];
        if (logical == NULL)
        {
            // Delete the memory info structure.
            delete memory;

            // Out of memory.
            return NULL;
        }

        // Fill in the memory info structure.
        memory->logical = logical;
        memory->bytes   = pages << 12;

        // Lock the mutex.
        gckvip_os_lock_mutex(memory_mutex);

        // Process all pages.
        size_t j = 0;
        for (size_t i = 0; (i < m_PageCount) && (pages > 0); i++)
        {
            // Test if page is empty.
            if (m_PageMap[i] == NULL)
            {
                // Test if this is the first page.
                if (j == 0)
                {
                    // Set physiucal memory.
                    memory->physical = U32((i << 12) + gcdCONTIGUOUS_BASE);
                }

                // Set localgical address for page.
                m_PageMap[i] = logical + j;

                // Next logical address.
                j += 1 << 12;

                // Decrease the number of pages.
                pages--;
            }
        }

        // Unlock the mutex.
        gckvip_os_unlock_mutex(memory_mutex);

        // Test if we are out of memory.
        if (pages > 0)
        {
            if (j)
            {
                // Free the allocated memory.
                Free(memory);
            }
            else
            {
                // Delete loglcal memory.
                delete logical;

                // Delete the memory info structure.
                delete memory;
            }

            // Out of memory.
            return NULL;
        }

        // Return the memory info pointer.
        return memory;
    }

    void Free(cmMemoryInfo *Memory)
    {
        // Compute the number of pages and the initial page.
        size_t pages = Memory->bytes    >> 12;
        size_t page  = (Memory->physical - gcdCONTIGUOUS_BASE) >> 12;

        // Set range of logical memory.
        U08 * start = static_cast<U08 *>(Memory->logical);
        U08 * end   = start + Memory->bytes;

        // Lock the mutex.
        gckvip_os_lock_mutex(memory_mutex);

        // Check if the starting page is valid.
        if (page < m_PageCount)
        {
            // Process all pages.
            for (size_t i = page; pages > 0;)
            {
                // Check if the current page is in the logical memory range.
                if (m_PageMap[i] >= start && m_PageMap[i] < end)
                {
                    // Mark page as empty.
                    m_PageMap[i] = NULL;

                    // Decrease the number of pages.
                    pages--;
                }

                // Increase the page index and test for overflow.
                if (++i >= m_PageCount)
                {
                    // Wrap to page 0.
                    i = 0;
                }
                else if ((i == page) && (pages > 0))
                {
                    // No more pages found.
                    break;
                }
            }
        }

        // Unlock the mutex.
        gckvip_os_unlock_mutex(memory_mutex);

        // Delete the logical memory.
        delete Memory->logical;

        // Delete the memory info structure.
        delete Memory;
    }
};

class CModelWrapper
{
public:
    CModelWrapper(U32 ChipID = 0)
    : chipID(0), cmodelThreadRunning(FALSE)
    {
        cmodel = new cmoTopLevel(FALSE, TRUE, chipID);
        gckvip_os_create_signal(&waitSignal);
        gckvip_os_create_semaphore(&interruptSemaphore);
        gckvip_os_create_semaphore(&cmodelSemaphore);
        interruptSuspended = 0;
        printf("CModelWapper:\nwaitSignal %p\ninterruptSemaphore %p\ncmodelSemaphore %p.\n",
            &waitSignal, &interruptSemaphore, &cmodelSemaphore);
    }

    ~CModelWrapper()
    {

        gckvip_os_destroy_signal(waitSignal);
        gckvip_os_destory_semaphore(&interruptSemaphore);
        gckvip_os_destory_semaphore(&cmodelSemaphore);

        delete cmodel;
    }

    cmoTopLevel* cmodel;
    U32 chipID;
    gckvip_signal waitSignal;
    gckvip_semaphore interruptSemaphore;
    gckvip_semaphore cmodelSemaphore;
    bool cmodelThreadQuit;
    bool interruptQuit;
    bool interruptSuspended;
    pthread_t interruptThread;
    pthread_t cmodelThread;
    volatile bool cmodelThreadRunning;
    void (*irq_handler)(void *data);
    void *pdata;    //Private data.
};

typedef CModelWrapper* CModelWrapperPtr;

static CModelWrapperPtr wrapper = VIP_NULL;

aqMemoryBase* _SystemMemory = VIP_NULL;


static void CModelWait(IN void * Context, IN U32 CommandDelay)
{
    CModelWrapperPtr  wrapper = static_cast<CModelWrapperPtr>(Context);
    gckvip_os_wait_signal(wrapper->waitSignal, CommandDelay);
}

vip_status_e CModelRelease()
{
    return gckvip_os_set_signal(wrapper->waitSignal, vip_true_e);
}

static void CModelInterrupt(IN void * Context)
{
    CModelWrapperPtr  wrapper = static_cast<CModelWrapperPtr>(Context);

    /* Kick off interrupt thread. */
    gckvip_os_relase_semaphore(&wrapper->interruptSemaphore);
}

void* CModelThread(void * Context)
{
    CModelWrapperPtr  wrapper = static_cast<CModelWrapperPtr>(Context);

    wrapper->cmodelThreadRunning = true;

    for (;;)
    {
        gckvip_os_wait_semaphore(&wrapper->cmodelSemaphore);

        if (wrapper->cmodelThreadQuit)
        {
            break;
        }
        wrapper->cmodel->Execute();
    }

    return VIP_NULL;
}

void* InterruptThread(void * Context)
{
    CModelWrapperPtr  wrapper = static_cast<CModelWrapperPtr>(Context);

    for (;;)
    {
        gckvip_os_wait_semaphore(&wrapper->interruptSemaphore);
        if (wrapper->interruptQuit)
        {
            break;
        }

        // Wait for end of critical section.
        while (wrapper->interruptSuspended) ;

        //Handle interrupt.
        if (wrapper->irq_handler != NULL) {
            (*wrapper->irq_handler)(wrapper->pdata);
        }
    }

    return VIP_NULL;
}

int CModelConstructor(cmSize Size)
{
    /* Create memory. */
    _SystemMemory = new CMemory(Size);

    /* Build C-Models. */
    wrapper       = new CModelWrapper();

    cmsCALLBACKS callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.Interrupt = CModelInterrupt;
    callbacks.Abort = NULL;
    callbacks.Wait = CModelWait;

    /* Set call-back functions. */
    wrapper->cmodel->RegisterCallback(wrapper, callbacks);

    /* Create interrupt threads. */
    wrapper->interruptQuit = false;
    pthread_create(&wrapper->interruptThread, VIP_NULL, InterruptThread, wrapper);
    /* Create C-Model threads. */
    wrapper->cmodelThreadQuit = false;
    pthread_create(&wrapper->cmodelThread, VIP_NULL, CModelThread, wrapper);

    /* Success. */
    return 0;
}

cmMemoryInfo * CModelAllocate(size_t Bytes, cmBool Contiguous)
{
    return ((CMemory*)_SystemMemory)->AllocateContiguous(Bytes);
}

void CModelDestructor()
{
    /* Destroy the interrupt thread. */
    wrapper->interruptQuit = true;

    gckvip_os_relase_semaphore(&wrapper->interruptSemaphore);
    pthread_join(wrapper->interruptThread, VIP_NULL);

    /* Destroy the C-Model thread. */
    wrapper->cmodelThreadQuit = true;
    gckvip_os_set_signal(wrapper->waitSignal, vip_true_e);
    gckvip_os_relase_semaphore(&wrapper->cmodelSemaphore);
    pthread_join(wrapper->cmodelThread, VIP_NULL);

    /* Destroy the C-Model. */
    delete wrapper;

    delete _SystemMemory;
    _SystemMemory = VIP_NULL;

    /* Success. */

}

void CModelWriteRegister(
    IN cmUInt32 Address, IN cmUInt32 Data)
{
    /* Write the register to the C-Model. */
    wrapper->cmodel->WriteRegister(Address, Data);


    /* Test for special registers. */
#if gcdSECURITY_ROBUSTNESS
    if (Address == GCREG_CMD_BUFFER_AHB_CTRL_Address)
#else
    if (Address == AQ_CMD_BUFFER_CTRL_Address)
#endif
    {
        /* Run the C-Model. */
        while (!wrapper->cmodelThreadRunning);
        {
            /* Run the C-Model. */
            gckvip_os_relase_semaphore(&wrapper->cmodelSemaphore);

        }
    }
}

cmUInt32 CModelReadRegister(IN cmUInt32 Address)
{
    return wrapper->cmodel->ReadRegister(Address);
}

cmBool CModelRegisterIrq(IRQ_HANDLER handler, void * private_data)
{
    wrapper->irq_handler = handler;
    wrapper->pdata = private_data;

    return 1;
}

bool CModelInterruptSuspend(IN bool Suspend)
{
    wrapper->interruptSuspended = Suspend;
    return true;
}

void CModelPrint(
    const char* message,
    ...
    )
{
    va_list arg;
    va_start(arg, message);
    vprintf(message, arg);
    va_end(arg);
}

void CModelFree(IN cmMemoryInfo *Memory)
{
    ((CMemory *)_SystemMemory)->Free(Memory);
}

cmUInt32 CModelPhysical(IN void *Logical)
{
    return ((CMemory *)_SystemMemory)->GetPhysical(Logical);
}

void * CModelLogical(IN cmUInt32 Physical)
{
    return ((CMemory *)_SystemMemory)->GetLogical(Physical);
}
#endif

