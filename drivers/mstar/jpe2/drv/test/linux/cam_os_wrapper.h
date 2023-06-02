////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2017 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// ("MStar Confidential Information") by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// @file      cam_os_wrapper.h
/// @brief     Cam OS Wrapper Header File for
///            1. RTK OS
///            2. Linux User Space
///            3. Linux Kernel Space
///////////////////////////////////////////////////////////////////////////////

#ifndef __CAM_OS_WRAPPER_H__
#define __CAM_OS_WRAPPER_H__

#if !defined(__KERNEL__)
typedef unsigned char       bool;
#endif
typedef unsigned char       u8;
typedef signed   char       s8;
typedef unsigned short      u16;
typedef signed   short      s16;
typedef unsigned int        u32;
typedef signed   int        s32;
typedef unsigned long long  u64;
typedef signed   long long  s64;

typedef enum
{
    CAM_OS_OK               = 0,
    CAM_OS_FAIL             = -1,
    CAM_OS_PARAM_ERR        = -2,
    CAM_OS_ALLOCMEM_FAIL    = -3,
    CAM_OS_TIMEOUT          = -4,
} CamOsRet_e;

typedef struct
{
    u8 nPriv[40];
} CamOsMutex_t;

typedef struct
{
    u8 nPriv[80];
} CamOsTsem_t;

typedef struct
{
    u32 nSec;
    u32 nNanoSec;
} CamOsTimespec_t;

typedef struct
{
    u32 nPriority;      /* From 1(lowest) to 99(highest) */
    u32 nStackSize;     /* If nStackSize is zero, use OS default value */
} CamOsThreadAttrb_t, *pCamOsThreadAttrb;

typedef void * CamOsThread;

//=============================================================================
// Description:
//      Writes the C string pointed by format to the standard output.
// Parameters:
//      [in]  szFmt: C string that contains the text to be written, it can
//                   optionally contain embedded format specifiers.
// Return:
//      N/A
//=============================================================================
void CamOsDebug(const char *szFmt, ...);

//=============================================================================
// Description:
//      Reads data from stdin and stores them according to the parameter format
//      into the locations pointed by the additional arguments.
// Parameters:
//      [in]  szFmt: C string that contains the text to be parsing, it can
//                   optionally contain embedded format specifiers.
// Return:
//      The number of items of the argument list successfully filled.
//=============================================================================
s32 CamOsScanf(const char *szFmt, ...);

//=============================================================================
// Description:
//      Returns the next character from the standard input.
// Parameters:
//      N/A
// Return:
//      the character read is returned.
//=============================================================================
s32 CamOsGetChar(void);

//=============================================================================
// Description:
//      Suspend execution for millisecond intervals.
// Parameters:
//      [in]  nMsec: Millisecond to suspend.
// Return:
//      N/A
//=============================================================================
void CamOsMsSleep(u32 nMsec);

//=============================================================================
// Description:
//      Gets the current time of the clock specified, and puts it into the
//      buffer pointed to by ptRes.
// Parameters:
//      [out] nMsec: A pointer to a CamOsTimespec_t structure where
//                   CamOsGetMonotonicTime() can store the time.
// Return:
//      N/A
//=============================================================================
void CamOsGetMonotonicTime(CamOsTimespec_t *ptRes);

//=============================================================================
// Description:
//      The CamOsThreadCreate() function is used to create a new thread/task,
//      with attributes specified by ptAttrb. If ptAttrb is NULL, the default
//      attributes are used.
// Parameters:
//      [out] pThread: A successful call to CamOsThreadCreate() stores the handle
//                     of the new thread in the buffer pointed to by pThread.
//      [in]  ptAttrb: Argument points to a CamOsThreadAttrb_t structure whose
//                     contents are used at thread creation time to determine
//                     thread priority and stack size. Thread priority range from
//                     1(lowest) to 99(highest).
//      [in]  pfnStartRoutine(): The new thread starts execution by invoking it.
//      [in]  pArg: It is passed as the sole argument of pfnStartRoutine().
// Return:
//      CAM_OS_OK is returned if successful; otherwise, returns CamOsRet_e.
//=============================================================================
CamOsRet_e CamOsThreadCreate(CamOsThread *pThread,
                             CamOsThreadAttrb_t *ptAttrb,
                             void *(*pfnStartRoutine)(void *),
                             void *pArg);

//=============================================================================
// Description:
//      Waits for the thread specified by pThread to terminate. If that thread
//      has already terminated, then CamOsThreadJoin() returns immediately. This
//      function is not applicable to Linux kernel space.
// Parameters:
//      [in]  thread: Handle of target thread.
// Return:
//      CAM_OS_OK is returned if successful; otherwise, returns CamOsRet_e.
//=============================================================================
CamOsRet_e CamOsThreadJoin(CamOsThread thread);

//=============================================================================
// Description:
//      Stop a thread created by CamOsThreadCreate in Linux kernel space. This
//      function is not applicable to RTK and Linux user space.
// Parameters:
//      [in]  thread: Handle of target thread.
// Return:
//      CAM_OS_OK is returned if successful; otherwise, returns CamOsRet_e.
//=============================================================================
CamOsRet_e CamOsThreadStop(CamOsThread thread);

//=============================================================================
// Description:
//      When someone calls CamOsThreadStop, it will be woken and this will
//      return true. You should then return from the thread. This function is
//      not applicable to RTK and Linux user space.
// Parameters:
//      N/A
// Return:
//      CAM_OS_OK is returned if successful; otherwise, returns CamOsRet_e.
//=============================================================================
CamOsRet_e CamOsThreadShouldStop(void);

//=============================================================================
// Description:
//      Set the name of a thread. The thread name is a meaningful C language
//      string, whose length is restricted to 16 characters, including the
//      terminating null byte ('\0').
// Parameters:
//      [in]  thread: Handle of target thread.
//      [in]  szName: specifies the new name.
// Return:
//      CAM_OS_OK is returned if successful; otherwise, returns CamOsRet_e.
//=============================================================================
CamOsRet_e CamOsThreadSetName(CamOsThread thread, const char *szName);

//=============================================================================
// Description:
//      Get the name of a thread. The buffer specified by name should be at
//      least 16 characters in length.
// Parameters:
//      [in]  thread: Handle of target thread.
//      [out] szName: Buffer used to return the thread name.
//      [in]  nLen: Specifies the number of bytes available in szName
// Return:
//      CAM_OS_OK is returned if successful; otherwise, returns CamOsRet_e.
//=============================================================================
CamOsRet_e CamOsThreadGetName(CamOsThread thread, const char *szName, u32 nLen);

//=============================================================================
// Description:
//      Get thread identification.
// Parameters:
//      N/A
// Return:
//      On success, returns the thread ID of the calling process.
//=============================================================================
u32 CamOsThreadGetID(void);

//=============================================================================
// Description:
//      Initializes the mutex.
// Parameters:
//      [in]  ptMutex: The mutex to initialize.
// Return:
//      CAM_OS_OK is returned if successful; otherwise, returns CamOsRet_e.
//=============================================================================
CamOsRet_e CamOsMutexInit(CamOsMutex_t *ptMutex);

//=============================================================================
// Description:
//      Destroys the mutex.
// Parameters:
//      [in]  ptMutex: The mutex to destroy.
// Return:
//      CAM_OS_OK is returned if successful; otherwise, returns CamOsRet_e.
//=============================================================================
CamOsRet_e CamOsMutexDestroy(CamOsMutex_t *ptMutex);

//=============================================================================
// Description:
//      Lock the mutex.
// Parameters:
//      [in]  ptMutex: The mutex to lock.
// Return:
//      CAM_OS_OK is returned if successful; otherwise, returns CamOsRet_e.
//=============================================================================
CamOsRet_e CamOsMutexLock(CamOsMutex_t *ptMutex);

//=============================================================================
// Description:
//      Unlock the mutex.
// Parameters:
//      [in]  ptMutex: The mutex to unlock.
// Return:
//      CAM_OS_OK is returned if successful; otherwise, returns CamOsRet_e.
//=============================================================================
CamOsRet_e CamOsMutexUnlock(CamOsMutex_t *ptMutex);

//=============================================================================
// Description:
//      Initializes the semaphore at a given value.
// Parameters:
//      [in]  ptTsem: The semaphore to initialize.
//      [in]  nVal: the initial value of the semaphore.
// Return:
//      CAM_OS_OK is returned if successful; otherwise, returns CamOsRet_e.
//=============================================================================
CamOsRet_e CamOsTsemInit(CamOsTsem_t *ptTsem, u32 nVal);

//=============================================================================
// Description:
//      Destroy the semaphore.
// Parameters:
//      [in]  ptTsem: The semaphore to destroy.
// Return:
//      CAM_OS_OK is returned if successful; otherwise, returns CamOsRet_e.
//=============================================================================
CamOsRet_e CamOsTsemDeinit(CamOsTsem_t *ptTsem);

//=============================================================================
// Description:
//      Increases the value of the semaphore.
// Parameters:
//      [in]  ptTsem: The semaphore to increase.
// Return:
//      N/A
//=============================================================================
void CamOsTsemUp(CamOsTsem_t *ptTsem);

//=============================================================================
// Description:
//      Decreases the value of the semaphore. Blocks if the semaphore value is
//      zero.
// Parameters:
//      [in]  ptTsem: The semaphore to decrease.
// Return:
//      N/A
//=============================================================================
void CamOsTsemDown(CamOsTsem_t *ptTsem);

//=============================================================================
// Description:
//      Decreases the value of the semaphore. Blocks if the semaphore value is
//      zero.
// Parameters:
//      [in]  ptTsem: The semaphore to decrease
// Return:
//      If the timeout is reached the function exits with error CAM_OS_TIMEOUT.
//      CAM_OS_OK is returned if down successful; otherwise, returns CamOsRet_e.
//=============================================================================
CamOsRet_e CamOsTsemTimedDown(CamOsTsem_t *ptTsem, u32 nMsec);

//=============================================================================
// Description:
//      Signal the condition, if waiting.
// Parameters:
//      [in]  ptTsem: The semaphore to signal
// Return:
//      N/A
//=============================================================================
void CamOsTsemSignal(CamOsTsem_t *ptTsem);

//=============================================================================
// Description:
//      Wait on the condition.
// Parameters:
//      [in]  ptTsem: The semaphore to wait.
// Return:
//      N/A
//=============================================================================
void CamOsTsemWait(CamOsTsem_t *ptTsem);

//=============================================================================
// Description:
//      Wait on the condition.
// Parameters:
//      [in]  ptTsem: The semaphore to wait.
// Return:
//      If the timeout is reached the function exits with error CAM_OS_TIMEOUT.
//      CAM_OS_OK is returned if wait successful; otherwise, returns CamOsRet_e.
//=============================================================================
CamOsRet_e CamOsTsemTimedWait(CamOsTsem_t *ptTsem, u32 nMsec);

//=============================================================================
// Description:
//      Get the value of the semaphore.
// Parameters:
//      [in]  ptTsem: The semaphore to reset.
// Return:
//      The value of the semaphore.
//=============================================================================
u32 CamOsTsemGetValue(CamOsTsem_t *ptTsem);

//=============================================================================
// Description:
//      Reset the value of the semaphore.
// Parameters:
//      [in]  ptTsem: The semaphore to reset.
// Return:
//      N/A
//=============================================================================
void CamOsTsemReset(CamOsTsem_t *ptTsem);

//=============================================================================
// Description:
//      Allocates a block of nSize bytes of memory, returning a pointer to the
//      beginning of the block.
// Parameters:
//      [in]  nSize: Size of the memory block, in bytes.
// Return:
//      On success, a pointer to the memory block allocated by the function. If
//      failed to allocate, a null pointer is returned.
//=============================================================================
void* CamOsMemAlloc(u32 nSize);

//=============================================================================
// Description:
//      Allocates a block of memory for an array of nNum elements, each of them
//      nSize bytes long, and initializes all its bits to zero.
// Parameters:
//      [in]  nNum: Number of elements to allocate.
//      [in]  nSize: Size of each element.
// Return:
//      On success, a pointer to the memory block allocated by the function. If
//      failed to allocate, a null pointer is returned.
//=============================================================================
void* CamOsMemCalloc(u32 nNum, u32 nSize);

//=============================================================================
// Description:
//      Changes the size of the memory block pointed to by pPtr. The function
//      may move the memory block to a new location (whose address is returned
//      by the function).
// Parameters:
//      [in]  pPtr: Pointer to a memory block previously allocated with
//                  CamOsMemAlloc, CamOsMemCalloc or CamOsMemRealloc.
//      [in]  nSize: New size for the memory block, in bytes.
// Return:
//      A pointer to the reallocated memory block, which may be either the same
//      as pPtr or a new location.
//=============================================================================
void* CamOsMemRealloc(void* pPtr, u32 nSize);

//=============================================================================
// Description:
//      A block of memory previously allocated by a call to CamOsMemAlloc,
//      CamOsMemCalloc or CamOsMemRealloc is deallocated, making it available
//      again for further allocations. If pPtr is a null pointer, the function
//      does nothing.
// Parameters:
//      [in]  pPtr: Pointer to a memory block previously allocated with
//                  CamOsMemAlloc, CamOsMemCalloc or CamOsMemRealloc.
// Return:
//      N/A
//=============================================================================
void CamOsMemRelease(void* pPtr);

//=============================================================================
// Description:
//      Allocates a block of nSize bytes of direct memory (non-cached memory),
//      returning three pointer for different address domain to the beginning
//      of the block.
// Parameters:
//      [in]  szName: Name of the memory block, whose length is restricted to
//                    16 characters.
//      [in]  nSize: Size of the memory block, in bytes.
//      [out] ppUserPtr: Virtual address pointer to the memory block.
//      [out] pMiuAddr: Memory Interface Unit address pointer to the memory block.
//      [out] lpPhysAddr: Physical address pointer to the memory block.
// Return:
//      CAM_OS_OK is returned if successful; otherwise, returns CamOsRet_e.
//=============================================================================
CamOsRet_e CamOsDirectMemAlloc(const char* szName,
                               u32 nSize,
                               void** ppUserPtr,
                               u32 *pMiuAddr,
                               u64 *lpPhysAddr);

//=============================================================================
// Description:
//      A block of memory previously allocated by a call to CamOsDirectMemAlloc,
//      is deallocated, making it available again for further allocations.
// Parameters:
//      [in]  pUserPtr: Virtual address pointer to a memory block previously allocated with
//                      CamOsDirectMemAlloc.
//      [in]  nSize: Size of the memory block, in bytes.
// Return:
//      CAM_OS_OK is returned if successful; otherwise, returns CamOsRet_e.
//=============================================================================
CamOsRet_e CamOsDirectMemRelease(void* pUserPtr, u32 nSize);

//=============================================================================
// Description:
//      Print all allocated direct memory information to the standard output.
// Parameters:
//      N/A
// Return:
//      CAM_OS_OK is returned if successful; otherwise, returns CamOsRet_e.
//=============================================================================
CamOsRet_e CamOsDirectMemStat(void);

//=============================================================================
// Description:
//      Set property value by property name.
// Parameters:
//      [in]  szKey: Name of property.
//      [in]  szValue: Value if property.
// Return:
//      CAM_OS_OK is returned if successful; otherwise, returns CamOsRet_e.
//=============================================================================
CamOsRet_e CamOsPropertySet(const char *szKey, const char *szValue);

//=============================================================================
// Description:
//      Get property value by property name.
// Parameters:
//      [in]  szKey: Name of property.
//      [out] szValue: Value if property.
//      [in]  szDefaultValue: If the property read fails or returns an empty
//                            value, the default value is used
// Return:
//      CAM_OS_OK is returned if successful; otherwise, returns CamOsRet_e.
//=============================================================================
CamOsRet_e CamOsPropertyGet(const char *szkey, char *szValue, const char *szDefaultValue);

#endif /* __CAM_OS_WRAPPER_H__ */