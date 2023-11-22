/*
 * cam_os_wrapper.c- Sigmastar
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

///////////////////////////////////////////////////////////////////////////////
/// @file      cam_os_wrapper.c
/// @brief     Cam OS Wrapper Source File for
///            1. RTK OS
///            2. Linux User Space
///            3. Linux Kernel Space
///////////////////////////////////////////////////////////////////////////////

#if defined(__KERNEL__)
#define CAM_OS_LINUX_KERNEL
#endif

#ifdef CAM_OS_RTK
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "vm_types.ht"
#include "time.h"
#include "sys_sys.h"
#include "sys_sys_math64.h"
#include "sys_sys_dbg.h"
#include "sys_sys_core.h"
#include "sys_sys_time.h"
#include "sys_MsWrapper_cus_os_flag.h"
#include "sys_MsWrapper_cus_os_sem.h"
#include "sys_MsWrapper_cus_os_util.h"
#include "sys_MsWrapper_cus_os_timer.h"
#include "sys_MsWrapper_cus_os_mem.h"
#include "sys_sys_isw_uart.h"
#include "sys_sys_chip.h"
#include "hal_drv_util.h"
#include "sys_arch_timer.h"
#include "drv_bus_axi.h"
#include "drv_int_ctrl_pub_api.h"
#include "drv_rtc.h"
#include "mdrv_miu.h"
#include "cam_os_wrapper.h"
#include "cam_os_util.h"
#include "cam_os_util_list.h"
#include "cam_os_util_bitmap.h"

#define OS_NAME "RTK"

#define CAM_OS_THREAD_STACKSIZE_DEFAULT 8192

typedef void *CamOsThreadEntry_t(void *);

typedef struct
{
    Ms_Mutex_t    tMutex;
    unsigned long nInited;
} CamOsMutexRtk_t, *pCamOsMutexRtk;

typedef struct
{
    Ms_DynSemaphor_t Tsem;
    unsigned long    nInited;
} CamOsTsemRtk_t, *pCamOsTsemRtk;

typedef struct
{
    long             nReadCount;
    Ms_Mutex_t       tRMutex;
    Ms_DynSemaphor_t WTsem;
    unsigned long    nInited;
} CamOsRwsemRtk_t, *pCamOsRwsemRtk;

typedef struct
{
    unsigned long nFlags;
    unsigned long nInited;
} CamOsSpinlockRtk_t, *pCamOsSpinlockRtk;

typedef struct
{
    u8  nPoolID;
    u32 nObjSize;
} CamOsMemCacheRtk_t, *pCamOsMemCacheRtk;

typedef struct
{
    void **        ppEntryPtr;
    unsigned long *pBitmap;
    unsigned long  entry_num;
} CamOsInformalIdr_t, *pCamOsInformalIdr;

static Ms_Mutex_t _gtMemLock = {0};

_Static_assert(sizeof(CamOsMutex_t) >= sizeof(CamOsMutexRtk_t), "CamOsMutex_t size define not enough!");
_Static_assert(sizeof(CamOsTsem_t) >= sizeof(CamOsTsemRtk_t), "CamOsTsem_t size define not enough!");
_Static_assert(sizeof(CamOsRwsem_t) >= sizeof(CamOsRwsemRtk_t), "CamOsRwsem_t size define not enough!");
_Static_assert(sizeof(CamOsSpinlock_t) >= sizeof(CamOsSpinlockRtk_t), "CamOsSpinlock_t size define not enough!");
_Static_assert(sizeof(CamOsTimer_t) >= sizeof(MsTimerExt_t), "CamOsTimer_t size define not enough!");
_Static_assert(sizeof(CamOsMemCache_t) >= sizeof(CamOsMemCacheRtk_t), "CamOsMemCache_t size define not enough!");
_Static_assert(sizeof(CamOsIdr_t) >= sizeof(CamOsInformalIdr_t), "CamOsIdr_t size define not enough!");

#elif defined(CAM_OS_LINUX_USER)
#define _GNU_SOURCE
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/time.h>
#ifndef NO_MDRV_MSYS
#include <mdrv_msys_io.h>
#include <mdrv_msys_io_st.h>
#include "mdrv_verchk.h"
#endif
#include <sys/mman.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <stdarg.h>
#include <time.h>
#include <execinfo.h>
#include "cam_os_wrapper.h"
#include "cam_os_util.h"
#include "cam_os_util_list.h"
#include "cam_os_util_bitmap.h"

#define OS_NAME "LINUX USER"

typedef void *CamOsThreadEntry_t(void *);

typedef struct
{
    struct CamOsListHead_t tList;
    pthread_t              tThreadHandle;
    CamOsThreadEntry_t *   pfnEntry;
    void *                 pArg;
    u32                    nPid;
    u32                    nShouldStop;
    CamOsTsem_t            tEntryIn;
} CamOsThreadHandleLinuxUser_t, *pCamOsThreadHandleLinuxUser;

typedef struct
{
    pthread_mutex_t tMutex;
    unsigned long   nInited;
} CamOsMutexLU_t, *pCamOsMutexLU;

typedef struct
{
    sem_t         tSem;
    unsigned long nInited;
} CamOsTsemLU_t, *pCamOsTsemLU;

typedef struct
{
    pthread_rwlock_t tRwsem;
    unsigned long    nInited;
} CamOsRwsemLU_t, *pCamOsRwsemLU;

typedef struct
{
    pthread_spinlock_t tLock;
    unsigned long      nInited;
} CamOsSpinlockLU_t, *pCamOsSpinlockLU;

typedef struct
{
    timer_t tTimerID;
} CamOsTimerLU_t, *pCamOsTimerLU;

typedef struct
{
    void **        ppEntryPtr;
    unsigned long *pBitmap;
    unsigned long  entry_num;
} CamOsInformalIdr_t, *pCamOsInformalIdr;

static pthread_mutex_t _gtMemLock        = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t _gtThreadListLock = PTHREAD_MUTEX_INITIALIZER;
CAM_OS_LIST_HEAD(_gtThreadList);

_Static_assert(sizeof(CamOsMutex_t) >= sizeof(CamOsMutexLU_t), "CamOsMutex_t size define not enough!");
_Static_assert(sizeof(CamOsTsem_t) >= sizeof(CamOsTsemLU_t), "CamOsTsem_t size define not enough!");
_Static_assert(sizeof(CamOsRwsem_t) >= sizeof(CamOsRwsemLU_t), "CamOsRwsem_t size define not enough!");
_Static_assert(sizeof(CamOsSpinlock_t) >= sizeof(CamOsSpinlockLU_t), "CamOsSpinlock_t size define not enough!");
_Static_assert(sizeof(CamOsTimer_t) >= sizeof(CamOsTimerLU_t), "CamOsTimer_t size define not enough!");
_Static_assert(sizeof(CamOsIdr_t) >= sizeof(CamOsInformalIdr_t), "CamOsIdr_t size define not enough!");

#elif defined(CAM_OS_LINUX_KERNEL)
#include <linux/sched.h>
#include <uapi/linux/sched/types.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/math64.h>
#include <linux/mm.h>
#include <linux/wait.h>
#include <linux/vmalloc.h>
#include <linux/list.h>
#include <linux/io.h>
#include <linux/types.h>
#include <linux/scatterlist.h>
#include <linux/slab_def.h>
#include <linux/idr.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/mman.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include "linux/vmalloc.h"
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/task.h>
#include <linux/sched/types.h>
#endif
#include <asm/io.h>
#include <asm/cacheflush.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <ms_msys.h>
#include <ms_platform.h>
#include "mdrv_miu.h"
#include "cam_os_wrapper.h"
#include "cam_os_util.h"
#include "cam_os_util_list.h"
#include "cam_os_util_bitmap.h"

#define OS_NAME "LINUX KERNEL"

#define CAM_OS_THREAD_STACKSIZE_DEFAULT 8192
#define KMALLOC_THRESHOLD_SIZE          (PAGE_SIZE >> 1)
#define LOG_MAX_TRACE_LEN               256
#define LINUX_KERNEL_MAX_IRQ            256
#define CAM_OS_MAX_ISR                  16

typedef void *CamOsThreadEntry_t(void *);

typedef struct
{
    struct task_struct *tThreadHandle;
    CamOsThreadEntry_t *pfnEntry;
    void *              pArg;
    struct semaphore    tWaitStop;
} CamOsThreadHandleLK_t, *pCamOsThreadHandleLK;

typedef struct
{
    struct mutex          tMutex;
    struct lock_class_key lockKey;
    unsigned long         nInited;
} CamOsMutexLK_t, *pCamOsMutexLK;

typedef struct
{
    struct semaphore tSem;
    unsigned long    nInited;
} CamOsTsemLK_t, *pCamOsTsemLK;

typedef struct
{
    struct rw_semaphore   tRwsem;
    struct lock_class_key lockKey;
    unsigned long         nInited;
} CamOsRwsemLK_t, *pCamOsRwsemLK;

typedef struct
{
    spinlock_t            tLock;
    unsigned long         nFlags;
    struct lock_class_key lockKey;
    unsigned long         nInited;
} CamOsSpinlockLK_t, *pCamOsSpinlockLK;

typedef struct
{
    struct rt_mutex       tRtMutex;
    struct lock_class_key lockKey;
    unsigned long         nInited;
} CamOsRtMutexLK_t, *pCamOsRtMutexLK;

typedef struct
{
    struct timer_list  tTimerID;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
    CamOsTimerCallback pfnCallback;
    void *             pDataPtr;
#endif
} CamOsTimerLK_t, *pCamOsTimerLK;

typedef struct
{
    struct kmem_cache *ptKmemCache;
    char               szName[16];
} CamOsMemCacheLK_t, *pCamOsMemCacheLK;

typedef struct
{
    struct idr tIdr;
} CamOsIdrLK_t, *pCamOsIdrLK;

typedef struct
{
    struct CamOsHListNode_t tEntry;
    CamOsIrqHandler         pfnIsr;
    void *                  pDevId;
    u32                     nIntNum;
} CamOsIsrLK_t;

#define HASHTBL_CNT 4
static CAM_OS_DEFINE_HASHTABLE(interrupt_hashtbl, HASHTBL_CNT);
static DEFINE_MUTEX(_gtIsrLock);

static irqreturn_t CamOsIrqCommonHandler(int nIrq, void *pDevId)
{
    struct CamOsHListNode_t *n              = NULL;
    CamOsIsrLK_t *           pHashListNode2 = NULL;

    CAM_OS_HASH_FOR_EACH_POSSIBLE_SAFE(interrupt_hashtbl, pHashListNode2, n, tEntry, nIrq)
    {
        if (pHashListNode2->nIntNum == nIrq)
        {
            if ((pHashListNode2->pfnIsr != NULL) && (pHashListNode2->pDevId == pDevId))
            {
                pHashListNode2->pfnIsr(nIrq, pDevId);
                break;
            }
        }
    }

    return IRQ_HANDLED;
}

extern int msys_find_dmem_by_phys(unsigned long long phys, MSYS_DMEM_INFO *pdmem);

static DEFINE_MUTEX(_gtMemLock);

#if defined(CONFIG_TRACE_CAM_OS_MEM)
#define CAM_OS_MEM_TRACE_HASH_BITS 3 // function type
static CAM_OS_DEFINE_HASHTABLE(_gtCamOsMemTraceHash, CAM_OS_MEM_TRACE_HASH_BITS);
static DEFINE_SPINLOCK(_gtCamOsMemTracelock);
#endif

_Static_assert(sizeof(CamOsMutex_t) >= sizeof(CamOsMutexLK_t), "CamOsMutex_t size define not enough!");
_Static_assert(sizeof(CamOsTsem_t) >= sizeof(CamOsTsemLK_t), "CamOsTsem_t size define not enough!");
_Static_assert(sizeof(CamOsRwsem_t) >= sizeof(CamOsRwsemLK_t), "CamOsRwsem_t size define not enough!");
_Static_assert(sizeof(CamOsSpinlock_t) >= sizeof(CamOsSpinlockLK_t), "CamOsSpinlock_t size define not enough!");
_Static_assert(sizeof(CamOsTimer_t) >= sizeof(CamOsTimerLK_t), "CamOsTimer_t size define not enough!");
_Static_assert(sizeof(CamOsMemCache_t) >= sizeof(CamOsMemCacheLK_t), "CamOsMemCache_t size define not enough!");
_Static_assert(sizeof(CamOsAtomic_t) == sizeof(atomic_t), "CamOsAtomic_t size define not enough!");
_Static_assert(sizeof(CamOsAtomic64_t) == sizeof(atomic64_t), "CamOsAtomic64_t size define not enough!");
_Static_assert(sizeof(CamOsIdr_t) >= sizeof(CamOsIdrLK_t), "CamOsIdr_t size define not enough!");

#endif

#define TO_STR_NATIVE(e)   #e
#define MACRO_TO_STRING(e) TO_STR_NATIVE(e)
char cam_os_wrapper_version_string[] = MACRO_TO_STRING(SIGMASTAR_MODULE_VERSION) " wrapper." CAM_OS_WRAPPER_VERSION;

#define ASSIGN_POINTER_VALUE(a, b) \
    if ((a))                       \
    *(a) = (b)

#define INIT_MAGIC_NUM 0x55AA5AA5

#define CAM_OS_MAX_LIST_LENGTH_BITS 20

#define CAM_OS_WARN_TRACE_LR
#ifdef CAM_OS_WARN_TRACE_LR
#define CAM_OS_WARN(x) CamOsPrintf("%s " x ", LR:0x%08X\n", __FUNCTION__, __builtin_return_address(0))
#else
#define CAM_OS_WARN(x) CamOsPrintf("%s " x "\n", __FUNCTION__)
#endif

typedef struct ContMemoryList_t
{
    struct CamOsListHead_t tList;
    ss_phys_addr_t         tPhysAddr;
    MSYS_DMEM_INFO *       pMemifoPtr;
} ContMemoryList_t;

static ContMemoryList_t _gtContMemList;
static s32              _gnContMemListInited = 0;
static u32              _gnContMemReqCnt     = 0;

typedef enum
{
    CAM_OS_WORKQUEUE_AVAILABLE = 0,
    CAM_OS_WORKQUEUE_WAIT,
    CAM_OS_WORKQUEUE_WAIT_DELAY,
    CAM_OS_WORKQUEUE_EXECUTING,
} CamOsWorkStatus_e;

typedef struct
{
    void *pWorkQueue;
    void (*pfnFunc)(void *);
    void *            pData;
    CamOsWorkStatus_e eStatus;
    u64               nWorkSerialNum;
    CamOsTimer_t      tTimer;
} CamOsWork_t;

typedef struct
{
    CamOsTsem_t     tTsem;
    CamOsThread     tThread;
    CamOsWork_t *   tWorks;
    u32             nMax;
    u64             nWorkSerialNumMax;
    CamOsSpinlock_t tLock;
    CamOsAtomic_t   tReady;
} CamOsWorkQueuePriv_t;

char *CamOsVersion(void)
{
    return CAM_OS_WRAPPER_VERSION;
}

void CamOsPrintf(const char *szFmt, ...)
{
#ifdef CAM_OS_RTK
    va_list      tArgs;
    unsigned int u32MsgLen     = 0;
    char         nLineStr[256] = {0};

    va_start(tArgs, szFmt);
    u32MsgLen = vsnprintf(nLineStr, sizeof(nLineStr), szFmt, tArgs);
    if (u32MsgLen >= sizeof(nLineStr))
    {
        nLineStr[sizeof(nLineStr) - 1] = '\0'; /* even the 'vsnprintf' commond will do it */
        nLineStr[sizeof(nLineStr) - 2] = '\n';
        nLineStr[sizeof(nLineStr) - 3] = '.';
        nLineStr[sizeof(nLineStr) - 4] = '.';
        nLineStr[sizeof(nLineStr) - 5] = '.';
    }
    drv_uart_dbg_send_msg(nLineStr);
    va_end(tArgs);
#elif defined(CAM_OS_LINUX_USER)
    va_list tArgs;

    va_start(tArgs, szFmt);
    vfprintf(stderr, szFmt, tArgs);
    va_end(tArgs);
#elif defined(CAM_OS_LINUX_KERNEL)
#if defined(CONFIG_MS_MSYS_LOG)
    va_list      tArgs;
    unsigned int u32MsgLen                   = 0;
    char         szLogStr[LOG_MAX_TRACE_LEN] = {'a'};
    va_start(tArgs, szFmt);
    u32MsgLen = vsnprintf(szLogStr, LOG_MAX_TRACE_LEN, szFmt, tArgs);
    va_end(tArgs);
    if (u32MsgLen >= LOG_MAX_TRACE_LEN)
    {
        szLogStr[LOG_MAX_TRACE_LEN - 1] = '\0'; /* even the 'vsnprintf' commond will do it */
        szLogStr[LOG_MAX_TRACE_LEN - 2] = '\n';
        szLogStr[LOG_MAX_TRACE_LEN - 3] = '.';
        szLogStr[LOG_MAX_TRACE_LEN - 4] = '.';
        szLogStr[LOG_MAX_TRACE_LEN - 5] = '.';
    }
    msys_prints(szLogStr, u32MsgLen);
#else
    va_list tArgs;
    va_start(tArgs, szFmt);
    vprintk(szFmt, tArgs);
    va_end(tArgs);
#endif
#endif
}

void CamOsPrintString(const char *szStr)
{
#ifdef CAM_OS_RTK
    drv_uart_dbg_send_msg((char *)szStr);
#elif defined(CAM_OS_LINUX_USER)
    printf(szStr);
#elif defined(CAM_OS_LINUX_KERNEL)
#if defined(CONFIG_MS_MSYS_LOG)
    msys_prints(szStr, strlen(szStr));
#else
    printk(szStr);
#endif
#endif
}

#ifdef CAM_OS_RTK
static char *_CamOsAdvance(char *pBuf)
{
    char *pNewBuf = pBuf;

    /* Skip over nonwhite space */
    while ((*pNewBuf != ' ') && (*pNewBuf != '\t') && (*pNewBuf != '\n') && (*pNewBuf != '\0'))
    {
        pNewBuf++;
    }

    /* Skip white space */
    while ((*pNewBuf == ' ') || (*pNewBuf == '\t') || (*pNewBuf == '\n') || (*pNewBuf == '\0'))
    {
        pNewBuf++;
    }

    return pNewBuf;
}

static s32 _CamOsVsscanf(char *szBuf, char *szFmt, va_list tArgp)
{
    char *  pFmt;
    char *  pBuf;
    char *  pnSval;
    u32 *   pnU32Val;
    s32 *   pnS32Val;
    u64 *   pnU64Val;
    s64 *   pnS64Val;
    double *pdbDval;
    float * pfFval;
    s32     nCount = 0;

    pBuf = szBuf;

    for (pFmt = szFmt; *pFmt; pFmt++)
    {
        if (*pFmt == '%')
        {
            pFmt++;
            if (strncmp(pFmt, "u", 1) == 0)
            {
                pnU32Val = va_arg(tArgp, u32 *);
                sscanf(pBuf, "%u", pnU32Val);
                pBuf = _CamOsAdvance(pBuf);
                nCount++;
            }
            else if (strncmp(pFmt, "d", 1) == 0)
            {
                pnS32Val = va_arg(tArgp, s32 *);
                sscanf(pBuf, "%d", pnS32Val);
                pBuf = _CamOsAdvance(pBuf);
                nCount++;
            }
            else if (strncmp(pFmt, "llu", 3) == 0)
            {
                pnU64Val = va_arg(tArgp, u64 *);
                sscanf(pBuf, "%llu", pnU64Val);
                pBuf = _CamOsAdvance(pBuf);
                nCount++;
            }
            else if (strncmp(pFmt, "lld", 3) == 0)
            {
                pnS64Val = va_arg(tArgp, s64 *);
                sscanf(pBuf, "%lld", pnS64Val);
                pBuf = _CamOsAdvance(pBuf);
                nCount++;
            }
            else if (strncmp(pFmt, "f", 1) == 0)
            {
                pfFval = va_arg(tArgp, float *);
                sscanf(pBuf, "%f", pfFval);
                pBuf = _CamOsAdvance(pBuf);
                nCount++;
            }
            else if (strncmp(pFmt, "lf", 2) == 0)
            {
                pdbDval = va_arg(tArgp, double *);
                sscanf(pBuf, "%lf", pdbDval);
                pBuf = _CamOsAdvance(pBuf);
                nCount++;
            }
            else if (strncmp(pFmt, "s", 1) == 0)
            {
                pnSval = va_arg(tArgp, char *);
                sscanf(pBuf, "%s", pnSval);
                pBuf = _CamOsAdvance(pBuf);
                nCount++;
            }
            else
            {
                CamOsPrintf("%s error: unsupported format (\%%s)\n", __FUNCTION__, pFmt);
            }
        }
    }

    return nCount;
}

static s32 _CamOsGetString(char *szBuf, s32 nMaxLen, s32 nEcho)
{
    s32         nLen;
    static char ch = '\0';

    nLen = 0;
    while (1)
    {
        szBuf[nLen] = drv_uart_dbg_get_char_direct();

        // To ignore one for (\r,\n) or (\n, \r) pair
        if ((szBuf[nLen] == '\n' && ch == '\r') || (szBuf[nLen] == '\r' && ch == '\n'))
        {
            ch = '\0';
            continue;
        }
        ch = szBuf[nLen];
        if (ch == '\n' || ch == '\r')
        {
            if (nEcho)
                CamOsPrintf("\n");
            break;
        }
        if (nLen < (nMaxLen - 1))
        {
            if (ch == '\b') /* Backspace? */
            {
                if (nLen <= 0)
                    CamOsPrintf("\007");
                else
                {
                    CamOsPrintf("\b \b");
                    nLen--;
                }
                continue;
            }
            nLen++;
        }
        if (nEcho)
            CamOsPrintf("%c", ch);
    }
    szBuf[nLen] = '\0';
    return nLen;
}

static s32 _CamOsVfscanf(const char *szFmt, va_list tArgp)
{
    s32  nCount;
    char szCommandBuf[128];

    _CamOsGetString(szCommandBuf, sizeof(szCommandBuf), 1);

    nCount = _CamOsVsscanf(szCommandBuf, (char *)szFmt, tArgp);
    return nCount;
}
#endif

s32 CamOsScanf(const char *szFmt, ...)
{
#ifdef CAM_OS_RTK
    s32     nCount = 0;
    va_list tArgp;

    va_start(tArgp, szFmt);
    nCount = _CamOsVfscanf(szFmt, tArgp);
    va_end(tArgp);
    return nCount;
#elif defined(CAM_OS_LINUX_USER)
    s32     nCount = 0;
    va_list tArgp;

    va_start(tArgp, szFmt);
    nCount = vfscanf(stdin, szFmt, tArgp);
    va_end(tArgp);
    return nCount;
#elif defined(CAM_OS_LINUX_KERNEL)
    return 0;
#endif
}

s32 CamOsGetChar(void)
{
#ifdef CAM_OS_RTK
    s32 Ret;
    Ret = drv_uart_dbg_get_char_direct();
    CamOsPrintf("\n");
    return Ret;
#elif defined(CAM_OS_LINUX_USER)
    return getchar();
#elif defined(CAM_OS_LINUX_KERNEL)
    return 0;
#endif
}

s32 CamOsSnprintf(char *szBuf, u32 nSize, const char *szFmt, ...)
{
    va_list tArgs;
    s32     i;

    va_start(tArgs, szFmt);
    i = vsnprintf(szBuf, nSize, szFmt, tArgs);
    va_end(tArgs);

    return i;
}

void CamOsHexdump(char *szBuf, u32 nSize)
{
    int  i, j;
    int  cx         = 0;
    char szLine[80] = {0};

    CamOsPrintf(
        "\nOffset(h)  00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F\n"
        "-----------------------------------------------------------\n");

    if ((uintptr_t)szBuf % 16)
    {
        cx = 0;
        cx += snprintf(szLine + cx, sizeof(szLine) - cx, "%016llX  ", ((u64)(uintptr_t)szBuf >> 4) << 4);

        for (i = 0; i < (uintptr_t)szBuf % 16; i++)
        {
            cx += snprintf(szLine + cx, sizeof(szLine) - cx, "   ");
            szLine[i + 62] = ' ';
            if (i % 8 == 0)
            {
                cx += snprintf(szLine + cx, sizeof(szLine) - cx, " ");
            }
        }
    }

    for (i = 0; i < nSize; i++)
    {
        if ((i + (uintptr_t)szBuf) % 16 == 0)
        {
            cx = 0;
            cx += snprintf(szLine + cx, sizeof(szLine) - cx, "%016llXX  ", (u64)(uintptr_t)szBuf + i);
        }
        if ((i + (uintptr_t)szBuf) % 8 == 0)
        {
            cx += snprintf(szLine + cx, sizeof(szLine) - cx, " ");
        }

        cx += snprintf(szLine + cx, sizeof(szLine) - cx, "%02X ", szBuf[i]);

        if (((unsigned char *)szBuf)[i] >= ' ' && ((unsigned char *)szBuf)[i] <= '~')
        {
            szLine[(i + (uintptr_t)szBuf) % 16 + 62] = ((unsigned char *)szBuf)[i];
        }
        else
        {
            szLine[(i + (uintptr_t)szBuf) % 16 + 62] = '.';
        }

        if ((i + (uintptr_t)szBuf) % 16 == 15)
        {
            szLine[59] = ' ';
            szLine[60] = ' ';
            szLine[61] = '|';
            szLine[78] = '|';
            szLine[79] = 0;
            CamOsPrintf("%s\n", szLine);
        }
        else if (i == nSize - 1)
        {
            for (j = ((i + (uintptr_t)szBuf) + 1) % 16; j < 16; j++)
            {
                cx += snprintf(szLine + cx, sizeof(szLine) - cx, "   ");
                szLine[j + 62] = ' ';
            }
            if (((i + (uintptr_t)szBuf) + 1) % 16 <= 8)
            {
                cx += snprintf(szLine + cx, sizeof(szLine) - cx, " ");
            }
            szLine[59] = ' ';
            szLine[60] = ' ';
            szLine[61] = '|';
            szLine[78] = '|';
            szLine[79] = 0;
            CamOsPrintf("%s\n", szLine);
        }
    }
    CamOsPrintf("\n");
}

void CamOsMsSleep(u32 nMsec)
{
#ifdef CAM_OS_RTK
    MsSleep(nMsec);
#elif defined(CAM_OS_LINUX_USER)
#define DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))
#define MS_SLEEP_ACCURACY  10
    usleep((useconds_t)DIV_ROUND_UP(nMsec, MS_SLEEP_ACCURACY) * MS_SLEEP_ACCURACY * 1000);
#elif defined(CAM_OS_LINUX_KERNEL)
    msleep(nMsec);
#endif
}

u32 CamOsMsSleepInterruptible(u32 nMsec)
{
#ifdef CAM_OS_RTK
    MsSleep(nMsec);
    return 0;
#elif defined(CAM_OS_LINUX_USER)
#define DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))
#define MS_SLEEP_ACCURACY  10
    usleep((useconds_t)DIV_ROUND_UP(nMsec, MS_SLEEP_ACCURACY) * MS_SLEEP_ACCURACY * 1000);
    return 0;
#elif defined(CAM_OS_LINUX_KERNEL)
    return (u32)msleep_interruptible(nMsec);
#endif
}

void CamOsUsSleep(u32 nUsec)
{
#ifdef CAM_OS_RTK
    u32 nMsec = (nUsec + 999) / 1000;
    MsSleep(nMsec);
#elif defined(CAM_OS_LINUX_USER)
    usleep((useconds_t)nUsec);
#elif defined(CAM_OS_LINUX_KERNEL)
    usleep_range(nUsec, nUsec + 10);
#endif
}

void CamOsMsDelay(u32 nMsec)
{
#ifdef CAM_OS_RTK
    u64 nTicks = sys_arch_timer_get_counter();

    while (((sys_arch_timer_get_counter() - nTicks) / (sys_arch_timer_get_cntfrq() / 1000)) < nMsec)
    {
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsTimespec_t tTvStart = {0}, tTvEnd = {0};
    CamOsGetMonotonicTime(&tTvStart);
    do
    {
        CamOsGetMonotonicTime(&tTvEnd);
    } while (CamOsTimeDiff(&tTvStart, &tTvEnd, CAM_OS_TIME_DIFF_MS) < nMsec);
#elif defined(CAM_OS_LINUX_KERNEL)
    mdelay(nMsec);
#endif
}

void CamOsUsDelay(u32 nUsec)
{
#ifdef CAM_OS_RTK
    u64 nTicks = sys_arch_timer_get_counter();

    while (((sys_arch_timer_get_counter() - nTicks) / (sys_arch_timer_get_cntfrq() / 1000000)) < nUsec)
    {
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsTimespec_t tTvStart = {0}, tTvEnd = {0};
    CamOsGetMonotonicTime(&tTvStart);
    do
    {
        CamOsGetMonotonicTime(&tTvEnd);
    } while (CamOsTimeDiff(&tTvStart, &tTvEnd, CAM_OS_TIME_DIFF_US) < nUsec);
#elif defined(CAM_OS_LINUX_KERNEL)
    udelay(nUsec);
#endif
}

void CamOsGetTimeOfDay(CamOsTimespec_t *ptRes)
{
#ifdef CAM_OS_RTK
    struct timespec tTs;
    if (ptRes)
    {
        SysGetTimeOfDay(&tTs);
        ptRes->nSec     = tTs.tv_sec;
        ptRes->nNanoSec = tTs.tv_nsec;
    }
#elif defined(CAM_OS_LINUX_USER)
    struct timeval tTV;
    if (ptRes)
    {
        gettimeofday(&tTV, NULL);
        ptRes->nSec     = tTV.tv_sec;
        ptRes->nNanoSec = tTV.tv_usec * 1000;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    struct timespec64 tTv;
    if (ptRes)
    {
        ktime_get_real_ts64(&tTv);
        ptRes->nSec     = tTv.tv_sec;
        ptRes->nNanoSec = tTv.tv_nsec;
    }
#endif
}

void CamOsSetTimeOfDay(const CamOsTimespec_t *ptRes)
{
#ifdef CAM_OS_RTK
    struct timespec tTs;
    if (ptRes)
    {
        tTs.tv_sec  = ptRes->nSec;
        tTs.tv_nsec = ptRes->nNanoSec;
        SysSetTimeOfDay(&tTs);
    }
#elif defined(CAM_OS_LINUX_USER)
    struct timeval tTV;
    if (ptRes)
    {
        tTV.tv_sec  = ptRes->nSec;
        tTV.tv_usec = ptRes->nNanoSec / 1000;
        settimeofday(&tTV, NULL);
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    struct timespec64 tTs;
    if (ptRes)
    {
        tTs.tv_sec  = ptRes->nSec;
        tTs.tv_nsec = ptRes->nNanoSec;
        do_settimeofday64(&tTs);
    }
#endif
}

void CamOsGetMonotonicTime(CamOsTimespec_t *ptRes)
{
#ifdef CAM_OS_RTK
    struct timespec tTs;
    if (ptRes)
    {
        SysGetMonotonicTime(&tTs);
        ptRes->nSec     = tTs.tv_sec;
        ptRes->nNanoSec = tTs.tv_nsec;
    }
#elif defined(CAM_OS_LINUX_USER)
    struct timespec tTs;
    if (ptRes)
    {
        clock_gettime(CLOCK_MONOTONIC, &tTs);
        ptRes->nSec     = tTs.tv_sec;
        ptRes->nNanoSec = tTs.tv_nsec;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    struct timespec64 tTs;
    if (ptRes)
    {
        ktime_get_raw_ts64(&tTs);
        ptRes->nSec     = tTs.tv_sec;
        ptRes->nNanoSec = tTs.tv_nsec;
    }
#endif
}

s64 CamOsTimeDiff(CamOsTimespec_t *ptStart, CamOsTimespec_t *ptEnd, CamOsTimeDiffUnit_e eUnit)
{
    if (ptStart && ptEnd)
    {
        switch (eUnit)
        {
            case CAM_OS_TIME_DIFF_SEC:
                return (s64)ptEnd->nSec - ptStart->nSec;
            case CAM_OS_TIME_DIFF_MS:
                return ((s64)ptEnd->nSec - ptStart->nSec) * 1000 + ((s64)ptEnd->nNanoSec - ptStart->nNanoSec) / 1000000;
            case CAM_OS_TIME_DIFF_US:
                return ((s64)ptEnd->nSec - ptStart->nSec) * 1000000 + ((s64)ptEnd->nNanoSec - ptStart->nNanoSec) / 1000;
            case CAM_OS_TIME_DIFF_NS:
                return ((s64)ptEnd->nSec - ptStart->nSec) * 1000000000 + ((s64)ptEnd->nNanoSec - ptStart->nNanoSec);
            default:
                return 0;
        }
    }
    else
        return 0;
}

u64 CamOsGetTimeInMs(void)
{
#ifdef CAM_OS_RTK
    u64 nTicks = sys_arch_timer_get_counter();
    return nTicks / (sys_arch_timer_get_cntfrq() / 1000);
#elif defined(CAM_OS_LINUX_USER)
    struct timespec tTime;
    clock_gettime(CLOCK_MONOTONIC, &tTime);
    return (u64)tTime.tv_sec * 1000 + CamOsMathDivU64((u64)tTime.tv_nsec, 1000000, NULL);
#elif defined(CAM_OS_LINUX_KERNEL)
    struct timespec64 tTime;
    ktime_get_raw_ts64(&tTime);
    return (u64)tTime.tv_sec * 1000 + CamOsMathDivU64((u64)tTime.tv_nsec, 1000000, NULL);
#endif
}

unsigned long CamOsGetJiffies(void)
{
#ifdef CAM_OS_RTK
    return MsGetOsTick();
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
    return 0;
#elif defined(CAM_OS_LINUX_KERNEL)
    return jiffies;
#endif
}

u32 CamOsGetHz(void)
{
#ifdef CAM_OS_RTK
    return MsGetOsHz();
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
    return 0;
#elif defined(CAM_OS_LINUX_KERNEL)
    return HZ;
#endif
}

u32 CamOsJiffiesToMs(unsigned long nJiffies)
{
#ifdef CAM_OS_RTK
    return nJiffies * 1000 / MsGetOsHz();
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
    return 0;
#elif defined(CAM_OS_LINUX_KERNEL)
    return jiffies_to_msecs(nJiffies);
#endif
}

unsigned long CamOsMsToJiffies(u32 nMsec)
{
#ifdef CAM_OS_RTK
    return nMsec * MsGetOsHz() / 1000;
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
    return 0;
#elif defined(CAM_OS_LINUX_KERNEL)
    return msecs_to_jiffies(nMsec);
#endif
}

unsigned long CamOsJiffiesDiff(unsigned long nStart, unsigned long nEnd)
{
#ifdef CAM_OS_RTK
    // In RTK, one jiffie is equivalent to 1ms (refer to CamOsGetJiffies).
    /* overflow upper bound in u32:
        (u32)((u64)0xFFFFFFFFFFFFFFFF * 1000 / sys_arch_timer_get_cntfrq()) = 0xD3F01E52 */
    if (nEnd < nStart)
        return (u32)(0xD3F01E52 + nEnd - nStart);
    else
        return (u32)((s32)nEnd - (s32)nStart);
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
    return 0;
#elif defined(CAM_OS_LINUX_KERNEL)
    return (nEnd >= nStart) ? (nEnd - nStart) : ((unsigned long)(-1) + nEnd - nStart);
#endif
}

#if defined(CAM_OS_LINUX_USER)
static void _CamOSThreadEntry(void *pEntryData)
{
    CamOsThreadHandleLinuxUser_t *ptTaskHandle = (CamOsThreadHandleLinuxUser_t *)pEntryData;

    ptTaskHandle->nPid = CamOsThreadGetID();
    pthread_mutex_lock(&_gtThreadListLock);
    CAM_OS_LIST_ADD_TAIL(&(ptTaskHandle->tList), &_gtThreadList);
    pthread_mutex_unlock(&_gtThreadListLock);
    CamOsTsemUp(&ptTaskHandle->tEntryIn);

    if (ptTaskHandle->pfnEntry)
    {
        ptTaskHandle->pfnEntry(ptTaskHandle->pArg);
    }
}
#elif defined(CAM_OS_LINUX_KERNEL)
static int _CamOSThreadEntry(void *pEntryData)
{
    CamOsThreadHandleLK_t *ptTaskHandle = (CamOsThreadHandleLK_t *)pEntryData;

    if (ptTaskHandle->pfnEntry)
    {
        ptTaskHandle->pfnEntry(ptTaskHandle->pArg);
    }

    down(&ptTaskHandle->tWaitStop);

    return 0;
}
#endif

CamOsRet_e CamOsThreadCreate(CamOsThread *ptThread, CamOsThreadAttrb_t *ptAttrb, void *(*pfnStartRoutine)(void *),
                             void *pArg)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    do
    {
        if (sizeof(CamOsThreadAttrb_t) != sizeof(MsThreadAttrb_t))
        {
            eRet = CAM_OS_FAIL;
            break;
        }

        if (MsCreateTask((MsThread *)ptThread, (MsThreadAttrb_t *)ptAttrb, pfnStartRoutine, pArg) != MS_OK)
        {
            eRet = CAM_OS_FAIL;
        }
    } while (0);
#elif defined(CAM_OS_LINUX_USER)
    CamOsThreadHandleLinuxUser_t *ptTaskHandle = NULL;
    struct sched_param            tSched;
    pthread_attr_t                tAttr;
    s32                           nNiceVal = 0;

    *ptThread = NULL;
    do
    {
        if (!(ptTaskHandle = CamOsMemCalloc(sizeof(CamOsThreadHandleLinuxUser_t), 1)))
        {
            CAM_OS_WARN("alloc handle fail");
            eRet = CAM_OS_ALLOCMEM_FAIL;
            break;
        }

        CAM_OS_INIT_LIST_HEAD(&ptTaskHandle->tList);
        ptTaskHandle->pfnEntry    = pfnStartRoutine;
        ptTaskHandle->pArg        = pArg;
        ptTaskHandle->nPid        = 0;
        ptTaskHandle->nShouldStop = 0;
        CamOsTsemInit(&ptTaskHandle->tEntryIn, 0);

        if (ptAttrb != NULL)
        {
            do
            {
                pthread_attr_init(&tAttr);

                // Set SCHED_RR priority before thread created.
                if ((ptAttrb->nPriority > 70) && (ptAttrb->nPriority <= 100))
                {
                    pthread_attr_getschedparam(&tAttr, &tSched);
                    pthread_attr_setinheritsched(&tAttr, PTHREAD_EXPLICIT_SCHED);
                    pthread_attr_setschedpolicy(&tAttr, SCHED_RR);
                    if (ptAttrb->nPriority < 95) // nPriority 71~94 mapping to Linux PrioRT 1~94
                        tSched.sched_priority = (ptAttrb->nPriority - 71) * 93 / 23 + 1;
                    else // nPriority 95~99 mapping to Linux PrioRT 95~99
                        tSched.sched_priority = (ptAttrb->nPriority < 100) ? ptAttrb->nPriority : 99;
                    if (0 != pthread_attr_setschedparam(&tAttr, &tSched))
                    {
                        CAM_OS_WARN("set priority fail");
                        eRet = CAM_OS_FAIL;
                        break;
                    }
                }

                if (0 != ptAttrb->nStackSize)
                {
                    if (0 != pthread_attr_setstacksize(&tAttr, (size_t)ptAttrb->nStackSize))
                    {
                        eRet = CAM_OS_FAIL;
                        CAM_OS_WARN("set stack size fail");
                        break;
                    }
                }
                pthread_create(&ptTaskHandle->tThreadHandle, &tAttr, (void *)_CamOSThreadEntry, ptTaskHandle);
            } while (0);
            pthread_attr_destroy(&tAttr);
        }
        else
        {
            pthread_create(&ptTaskHandle->tThreadHandle, NULL, (void *)_CamOSThreadEntry, ptTaskHandle);
        }

        if (ptAttrb && ptAttrb->szName)
        {
            CamOsThreadSetName((CamOsThread *)ptTaskHandle, ptAttrb->szName);
        }

        // Set SCHED_OTHER priority after thread created.
        if (ptAttrb && (ptAttrb->nPriority > 0) && (ptAttrb->nPriority <= 70))
        {
            if (CamOsTsemTimedDown(&ptTaskHandle->tEntryIn, 1000) == CAM_OS_OK && ptTaskHandle->nPid != 0)
            {
                if (ptAttrb->nPriority <= 50)
                {
                    nNiceVal = 19 - ptAttrb->nPriority * 19 / 50;
                }
                else
                {
                    nNiceVal = 50 - ptAttrb->nPriority;
                }

                setpriority(PRIO_PROCESS, ptTaskHandle->nPid, nNiceVal);
            }
            else
            {
                CAM_OS_WARN("set priority fail");
            }
        }

        *ptThread = (CamOsThread *)ptTaskHandle;
    } while (0);

    if (!*ptThread)
    {
        if (ptTaskHandle)
        {
            CamOsMemRelease(ptTaskHandle);
        }
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsThreadHandleLK_t *ptTaskHandle = NULL;
    struct sched_param     tSched       = {.sched_priority = 0};
    struct sched_attr      tSchedAttr   = {0};
    s32                    nNiceVal     = 0;

    *ptThread = NULL;
    do
    {
        if (!(ptTaskHandle = CamOsMemCalloc(sizeof(CamOsThreadHandleLK_t), 1)))
        {
            CAM_OS_WARN("alloc handle fail");
            eRet = CAM_OS_ALLOCMEM_FAIL;
            break;
        }

        ptTaskHandle->pfnEntry = pfnStartRoutine;
        ptTaskHandle->pArg     = pArg;
        sema_init(&ptTaskHandle->tWaitStop, 0);

        ptTaskHandle->tThreadHandle =
            kthread_run(_CamOSThreadEntry, ptTaskHandle, (ptAttrb && ptAttrb->szName) ? ptAttrb->szName : "CAMOS");
        if (IS_ERR(ptTaskHandle->tThreadHandle))
        {
            eRet      = CAM_OS_FAIL;
            *ptThread = NULL;
            break;
        }

        if (ptAttrb != NULL)
        {
            if ((ptAttrb->nPriority > 0) && (ptAttrb->nPriority <= 70))
            {
                if (ptAttrb->nPriority <= 50)
                {
                    nNiceVal = 19 - ptAttrb->nPriority * 19 / 50;
                }
                else
                {
                    nNiceVal = 50 - ptAttrb->nPriority;
                }

                set_user_nice(ptTaskHandle->tThreadHandle, nNiceVal);
            }
            else if ((ptAttrb->nPriority > 70) && (ptAttrb->nPriority <= 100))
            {
                if (ptAttrb->nPriority < 95) // nPriority 71~94 mapping to Linux PrioRT 1~94
                    tSched.sched_priority = (ptAttrb->nPriority - 71) * 93 / 23 + 1;
                else // nPriority 95~99 mapping to Linux PrioRT 95~99
                    tSched.sched_priority = (ptAttrb->nPriority < 100) ? ptAttrb->nPriority : 99;
                if (sched_setscheduler(ptTaskHandle->tThreadHandle, SCHED_RR, &tSched) != 0)
                {
                    CAM_OS_WARN("set priority fail");
                }
            }

            if (ptAttrb->Sched.nRuntime && ptAttrb->Sched.nDeadline)
            {
                tSchedAttr.sched_policy   = SCHED_DEADLINE;
                tSchedAttr.sched_flags    = SCHED_FLAG_RESET_ON_FORK;
                tSchedAttr.sched_runtime  = ptAttrb->Sched.nRuntime;
                tSchedAttr.sched_deadline = ptAttrb->Sched.nDeadline;
                tSchedAttr.sched_period   = ptAttrb->Sched.nDeadline;
                if (sched_setattr(ptTaskHandle->tThreadHandle, &tSchedAttr) != 0)
                {
                    CAM_OS_WARN("set deadline fail");
                }
            }
        }

        *ptThread = (CamOsThread *)ptTaskHandle;
    } while (0);

    if (!*ptThread)
    {
        if (ptTaskHandle)
        {
            CamOsMemRelease(ptTaskHandle);
        }
    }
#endif

    // coverity[leaked_storage]
    return eRet;
}

CamOsRet_e CamOsThreadChangePriority(CamOsThread tThread, u32 nPriority)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    if (tThread)
    {
        MsChangeTaskPriority((MsThread)tThread, nPriority);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsThreadHandleLinuxUser_t *ptTaskHandle = (CamOsThreadHandleLinuxUser_t *)tThread;
    struct sched_param            tSched       = {.sched_priority = 0};
    s32                           nNiceVal     = 0;

    if (ptTaskHandle && (nPriority >= 0) && (nPriority <= 70))
    {
        nPriority = nPriority ? nPriority : 50;
        if (nPriority <= 50)
        {
            nNiceVal = 19 - nPriority * 19 / 50;
        }
        else
        {
            nNiceVal = 50 - nPriority;
        }

        setpriority(PRIO_PROCESS, ptTaskHandle->nPid, nNiceVal);
        tSched.sched_priority = 0;
        if (pthread_setschedparam(ptTaskHandle->tThreadHandle, SCHED_OTHER, &tSched) != 0)
        {
            CAM_OS_WARN("set priority fail");
            eRet = CAM_OS_FAIL;
        }
    }
    else if (ptTaskHandle && (nPriority > 70) && (nPriority <= 100))
    {
        if (nPriority < 95) // nPriority 71~94 mapping to Linux PrioRT 1~94
            tSched.sched_priority = (nPriority - 71) * 93 / 23 + 1;
        else // nPriority 95~99 mapping to Linux PrioRT 95~99
            tSched.sched_priority = (nPriority < 100) ? nPriority : 99;
        if (pthread_setschedparam(ptTaskHandle->tThreadHandle, SCHED_RR, &tSched) != 0)
        {
            CAM_OS_WARN("set priority fail");
            eRet = CAM_OS_FAIL;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsThreadHandleLK_t *ptTaskHandle = (CamOsThreadHandleLK_t *)tThread;
    struct sched_param     tSched       = {.sched_priority = 0};
    s32                    nNiceVal     = 0;

    if (ptTaskHandle && (nPriority >= 0) && (nPriority <= 70))
    {
        nPriority = nPriority ? nPriority : 50;
        if (nPriority <= 50)
        {
            nNiceVal = 19 - nPriority * 19 / 50;
        }
        else
        {
            nNiceVal = 50 - nPriority;
        }

        set_user_nice(ptTaskHandle->tThreadHandle, nNiceVal);
        tSched.sched_priority = 0;
        if (sched_setscheduler(ptTaskHandle->tThreadHandle, SCHED_NORMAL, &tSched) != 0)
        {
            CAM_OS_WARN("set priority fail");
            eRet = CAM_OS_FAIL;
        }
    }
    else if (ptTaskHandle && (nPriority > 70) && (nPriority <= 100))
    {
        if (nPriority < 95) // nPriority 71~94 mapping to Linux PrioRT 1~94
            tSched.sched_priority = (nPriority - 71) * 93 / 23 + 1;
        else // nPriority 95~99 mapping to Linux PrioRT 95~99
            tSched.sched_priority = (nPriority < 100) ? nPriority : 99;
        if (sched_setscheduler(ptTaskHandle->tThreadHandle, SCHED_RR, &tSched) != 0)
        {
            CAM_OS_WARN("set priority fail");
            eRet = CAM_OS_FAIL;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsThreadSetAffinity(CamOsThread tThread, const CamOsCpuMask_t *ptMask)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CAM_OS_WARN("not support in " OS_NAME);
    eRet = CAM_OS_FAIL;
#elif defined(CAM_OS_LINUX_USER)
    CamOsThreadHandleLinuxUser_t *ptTaskHandle = (CamOsThreadHandleLinuxUser_t *)tThread;
    cpu_set_t                     mask;
    int                           i = 0;

    if (ptTaskHandle)
    {
        CPU_ZERO(&mask);

        for (i = 0; i < (sizeof(CamOsCpuMask_t) * 8); i++)
        {
            if (CAM_OS_TEST_BIT(i, (unsigned long *)ptMask))
            {
                CPU_SET(i, &mask);
            }
        }
        pthread_setaffinity_np(ptTaskHandle->tThreadHandle, sizeof(mask), &mask);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsThreadHandleLK_t *ptTaskHandle = (CamOsThreadHandleLK_t *)tThread;

    if (ptTaskHandle)
    {
        sched_setaffinity(ptTaskHandle->tThreadHandle->pid, (struct cpumask *)ptMask);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsThreadSchedule(u8 bInterruptible, u32 nMsec)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    if (nMsec == CAM_OS_MAX_TIMEOUT)
    {
        // Avoid u32 overflow, put nMsec to MsScheduleTask without convert
        MsScheduleTask(CAM_OS_MAX_TIMEOUT);
        eRet = CAM_OS_TIMEOUT;
    }
    else if (nMsec > 0)
    {
        MsScheduleTask(nMsec);
        eRet = CAM_OS_TIMEOUT;
    }
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
    eRet = CAM_OS_FAIL;
#elif defined(CAM_OS_LINUX_KERNEL)
    if (nMsec == CAM_OS_MAX_TIMEOUT)
    {
        if (bInterruptible)
            set_current_state(TASK_INTERRUPTIBLE);
        else
            set_current_state(TASK_UNINTERRUPTIBLE);

        schedule();
    }
    else if (nMsec > 0)
    {
        if (bInterruptible)
        {
            if (0 == schedule_timeout(msecs_to_jiffies(nMsec) + 1))
                eRet = CAM_OS_TIMEOUT;
        }
        else
        {
            schedule_timeout_uninterruptible(msecs_to_jiffies(nMsec) + 1);
        }
    }
#endif
    return eRet;
}

CamOsRet_e CamOsThreadWakeUp(CamOsThread tThread)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    if (tThread)
    {
        MsWakeUpTask((MsThread)tThread);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
    eRet                                       = CAM_OS_FAIL;
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsThreadHandleLK_t *ptTaskHandle = (CamOsThreadHandleLK_t *)tThread;

    if (ptTaskHandle)
    {
        wake_up_process(ptTaskHandle->tThreadHandle);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsThreadJoin(CamOsThread tThread)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    if (tThread)
    {
        MsDeleteTask((MsThread)tThread);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsThreadHandleLinuxUser_t *ptTaskHandle = (CamOsThreadHandleLinuxUser_t *)tThread;
    if (ptTaskHandle)
    {
        ptTaskHandle->nShouldStop = 1;
        if (ptTaskHandle->tThreadHandle)
        {
            pthread_join(ptTaskHandle->tThreadHandle, NULL);
        }
        pthread_mutex_lock(&_gtThreadListLock);
        CAM_OS_LIST_DEL(&ptTaskHandle->tList);
        pthread_mutex_unlock(&_gtThreadListLock);
        CamOsMemRelease(ptTaskHandle);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsThreadHandleLK_t *ptTaskHandle = (CamOsThreadHandleLK_t *)tThread;
    s32                    nErr;
    if (ptTaskHandle)
    {
        up(&ptTaskHandle->tWaitStop);

        if (0 != (nErr = kthread_stop(ptTaskHandle->tThreadHandle)))
        {
            CAM_OS_WARN("stop fail");
            CamOsPrintf("%s Err=%d\n", __FUNCTION__, nErr);
            eRet = CAM_OS_FAIL;
        }

        CamOsMemRelease(ptTaskHandle);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsThreadStop(CamOsThread tThread)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    if (tThread)
    {
        MsSetTaskReadyToExit((MsThread)tThread);
        CamOsThreadJoin(tThread);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsThreadJoin(tThread);
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsThreadJoin(tThread);
#endif
    return eRet;
}

CamOsRet_e CamOsThreadShouldStop(void)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    if (!MsIsCurTaskReadyToExit())
        eRet = CAM_OS_FAIL;
#elif defined(CAM_OS_LINUX_USER)
    CamOsThreadHandleLinuxUser_t *ptTaskHandle;
    struct CamOsListHead_t *      ptPos, *ptQ;
    u32                           nCurPid = (u32)syscall(__NR_gettid);

    pthread_mutex_lock(&_gtThreadListLock);
    CAM_OS_LIST_FOR_EACH_SAFE(ptPos, ptQ, &_gtThreadList)
    {
        ptTaskHandle = CAM_OS_LIST_ENTRY(ptPos, CamOsThreadHandleLinuxUser_t, tList);

        if (ptTaskHandle->nPid == nCurPid)
        {
            if (!ptTaskHandle->nShouldStop)
                eRet = CAM_OS_FAIL;
            break;
        }
    }
    pthread_mutex_unlock(&_gtThreadListLock);
#elif defined(CAM_OS_LINUX_KERNEL)
    if (kthread_should_stop())
    {
        eRet = CAM_OS_OK;
    }
    else
    {
        eRet = CAM_OS_FAIL;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsThreadSetName(CamOsThread tThread, const char *szName)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    if (tThread)
    {
        MsSetTaskName((MsThread)tThread, szName);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsThreadHandleLinuxUser_t *ptTaskHandle = (CamOsThreadHandleLinuxUser_t *)tThread;
    if (ptTaskHandle && ptTaskHandle->tThreadHandle)
    {
        if (strlen(szName) >= 16) // Linux limitation
            return CAM_OS_PARAM_ERR;
#if defined(__GLIBC__) && defined(__GLIBC_PREREQ) && __GLIBC_PREREQ(2, 12)
        if (pthread_setname_np(ptTaskHandle->tThreadHandle, szName) != 0)
        {
            eRet = CAM_OS_PARAM_ERR;
        }
#else
        if (ptTaskHandle->tThreadHandle == pthread_self())
        {
            if (prctl(PR_SET_NAME, szName) != 0)
                eRet = CAM_OS_PARAM_ERR;
        }
        else
        {
            CAM_OS_WARN("not support set by other thread (in uclibc?)");
        }
#endif
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsThreadHandleLK_t *ptTaskHandle = (CamOsThreadHandleLK_t *)tThread;
    if (ptTaskHandle)
    {
        task_lock(ptTaskHandle->tThreadHandle);
        strlcpy(ptTaskHandle->tThreadHandle->comm, szName, sizeof(ptTaskHandle->tThreadHandle->comm));
        task_unlock(ptTaskHandle->tThreadHandle);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif

    return eRet;
}

CamOsRet_e CamOsThreadGetName(CamOsThread tThread, char *szName, u32 nLen)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    if (tThread)
    {
        MsGetTaskName((MsThread)tThread, szName, nLen);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsThreadHandleLinuxUser_t *ptTaskHandle = (CamOsThreadHandleLinuxUser_t *)tThread;
    if (szName)
    {
        if (nLen < 16) // Linux limitation
            return CAM_OS_PARAM_ERR;
        if (ptTaskHandle && ptTaskHandle->tThreadHandle)
        {
            if (pthread_getname_np(ptTaskHandle->tThreadHandle, szName, nLen) != 0)
            {
                eRet = CAM_OS_PARAM_ERR;
            }
        }
        else
        {
            if (pthread_getname_np(pthread_self(), szName, nLen) != 0)
            {
                eRet = CAM_OS_PARAM_ERR;
            }
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsThreadHandleLK_t *ptTaskHandle = (CamOsThreadHandleLK_t *)tThread;
    if (szName)
    {
        memset(szName, 0, nLen);
        if (ptTaskHandle)
        {
            task_lock(ptTaskHandle->tThreadHandle);
            strncpy(szName, ptTaskHandle->tThreadHandle->comm, nLen - 1);
            task_unlock(ptTaskHandle->tThreadHandle);
        }
        else
        {
            task_lock(current);
            strncpy(szName, current->comm, nLen - 1);
            task_unlock(current);
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

u32 CamOsThreadGetID()
{
#ifdef CAM_OS_RTK
    return MsCurrTask();
#elif defined(CAM_OS_LINUX_USER)
    return (u32)syscall(__NR_gettid);
#elif defined(CAM_OS_LINUX_KERNEL)
    return current->tgid;
#endif
}

void CamOsCpumaskSetCpu(unsigned int cpu, CamOsCpuMask_t *mask)
{
    CAM_OS_SET_BIT(cpu, mask->nPriv);
}

void CamOsCpumaskSetAll(CamOsCpuMask_t *mask)
{
    memset(&mask[0], 0xFF, sizeof(CamOsCpuMask_t));
}

void CamOsCpumaskClearCpu(int cpu, CamOsCpuMask_t *mask)
{
    CAM_OS_CLEAR_BIT(cpu, mask->nPriv);
}

void CamOsCpumaskClearAll(CamOsCpuMask_t *mask)
{
    memset(&mask[0], 0, sizeof(CamOsCpuMask_t));
}

CamOsRet_e CamOsCpumaskGetOnline(CamOsCpuMask_t *mask)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    if (mask)
    {
        CamOsCpumaskSetCpu(0, mask);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
    eRet                     = CAM_OS_FAIL;
#elif defined(CAM_OS_LINUX_KERNEL)
    if (mask)
    {
        memcpy((void *)mask, (void *)&__cpu_online_mask, sizeof(CamOsCpuMask_t));
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

#if defined(CAM_OS_LINUX_KERNEL)
#if defined(CONFIG_LOCKDEP)
static int _CamOsCheckStaticObj(const void *obj)
{
    unsigned long start = (unsigned long)&_stext, end = (unsigned long)&_end, addr = (unsigned long)obj;

    if (arch_is_kernel_initmem_freed(addr))
        return 0;

    /*
     * static variable?
     */
    if ((addr >= start) && (addr < end))
        return 1;

    if (arch_is_kernel_data(addr))
        return 1;

    /*
     * in-kernel percpu var?
     */
    if (is_kernel_percpu_address(addr))
        return 1;

    /*
     * module static or percpu var?
     */
    return is_module_address(addr) || is_module_percpu_address(addr);
}
#endif
#endif

CamOsRet_e CamOsMutexInit(CamOsMutex_t *ptMutex)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsMutexRtk_t *ptHandle = (CamOsMutexRtk_t *)ptMutex;
    if (ptHandle)
    {
        if (MS_OK != MsInitMutex(&ptHandle->tMutex))
        {
            CAM_OS_WARN("init fail");
            eRet = CAM_OS_FAIL;
        }
        else
        {
            ptHandle->nInited = INIT_MAGIC_NUM;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsMutexLU_t *ptHandle = (CamOsMutexLU_t *)ptMutex;
    if (ptHandle)
    {
        if (0 != pthread_mutex_init(&ptHandle->tMutex, NULL))
        {
            CAM_OS_WARN("init fail");
            eRet = CAM_OS_FAIL;
        }
        else
        {
            ptHandle->nInited = INIT_MAGIC_NUM;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsMutexLK_t *ptHandle = (CamOsMutexLK_t *)ptMutex;
    if (ptHandle)
    {
#ifdef CONFIG_LOCKDEP
        if (!_CamOsCheckStaticObj(&ptHandle->lockKey))
        {
            lockdep_register_key(&ptHandle->lockKey);
        }
        __mutex_init(&ptHandle->tMutex, "cam_os_mutex", &ptHandle->lockKey);
#else
        mutex_init(&ptHandle->tMutex);
#endif
        ptHandle->nInited = INIT_MAGIC_NUM;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsMutexDestroy(CamOsMutex_t *ptMutex)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsMutexRtk_t *ptHandle = (CamOsMutexRtk_t *)ptMutex;
    if (ptHandle)
    {
        ptHandle->nInited = 0;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsMutexLU_t *ptHandle = (CamOsMutexLU_t *)ptMutex;
    if (ptHandle)
    {
        if (0 != pthread_mutex_destroy(&ptHandle->tMutex))
        {
            CAM_OS_WARN("destroy fail");
            eRet = CAM_OS_FAIL;
        }
        else
        {
            ptHandle->nInited = 0;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsMutexLK_t *ptHandle = (CamOsMutexLK_t *)ptMutex;
    if (ptHandle)
    {
#ifdef CONFIG_LOCKDEP
        if (!_CamOsCheckStaticObj(&ptHandle->lockKey))
        {
            lockdep_unregister_key(&ptHandle->lockKey);
        }
#endif
        ptHandle->nInited = 0;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsMutexLock(CamOsMutex_t *ptMutex)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsMutexRtk_t *ptHandle = (CamOsMutexRtk_t *)ptMutex;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Mutex is not initialized before lock\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        if (MS_OK != MsMutexLock(&ptHandle->tMutex))
        {
            CAM_OS_WARN("lock fail");
            eRet = CAM_OS_FAIL;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsMutexLU_t *ptHandle = (CamOsMutexLU_t *)ptMutex;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Mutex is not initialized before lock\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        if (0 != pthread_mutex_lock(&ptHandle->tMutex))
        {
            CAM_OS_WARN("lock fail");
            eRet = CAM_OS_FAIL;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsMutexLK_t *ptHandle = (CamOsMutexLK_t *)ptMutex;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Mutex is not initialized before lock\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        mutex_lock(&ptHandle->tMutex);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsMutexTryLock(CamOsMutex_t *ptMutex)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsMutexRtk_t *ptHandle = (CamOsMutexRtk_t *)ptMutex;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Mutex is not initialized before try_lock\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        if (MS_UNIT_NOAVAIL == MsMutexTryLock(&ptHandle->tMutex))
        {
            eRet = CAM_OS_RESOURCE_BUSY;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    s32             nErr     = 0;
    CamOsMutexLU_t *ptHandle = (CamOsMutexLU_t *)ptMutex;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Mutex is not initialized before try_lock\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        if (0 != (nErr = pthread_mutex_trylock(&ptHandle->tMutex)))
        {
            if (nErr == EAGAIN)
            {
                eRet = CAM_OS_RESOURCE_BUSY;
            }
            else
            {
                CAM_OS_WARN("trylock fail");
                eRet = CAM_OS_FAIL;
            }
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsMutexLK_t *ptHandle = (CamOsMutexLK_t *)ptMutex;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Mutex is not initialized before try_lock\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        if (1 != mutex_trylock(&ptHandle->tMutex))
            eRet = CAM_OS_RESOURCE_BUSY;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsMutexUnlock(CamOsMutex_t *ptMutex)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsMutexRtk_t *ptHandle = (CamOsMutexRtk_t *)ptMutex;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Mutex is not initialized before unlock\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        if (MS_OK != MsMutexUnlock(&ptHandle->tMutex))
        {
            CAM_OS_WARN("unlock fail");
            eRet = CAM_OS_FAIL;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsMutexLU_t *ptHandle = (CamOsMutexLU_t *)ptMutex;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Mutex is not initialized before unlock\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        if (0 != pthread_mutex_unlock(&ptHandle->tMutex))
        {
            CAM_OS_WARN("unlock fail");
            eRet = CAM_OS_FAIL;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsMutexLK_t *ptHandle = (CamOsMutexLK_t *)ptMutex;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Mutex is not initialized before unlock\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        mutex_unlock(&ptHandle->tMutex);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsTsemInit(CamOsTsem_t *ptTsem, u32 nVal)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsTsemRtk_t *ptHandle = (CamOsTsemRtk_t *)ptTsem;
    if (ptHandle)
    {
        if (MS_OK != MsCreateDynSemExtend(&ptHandle->Tsem, CAM_OS_MAX_INT - 1, nVal))
        {
            CAM_OS_WARN("init fail");
            eRet = CAM_OS_FAIL;
        }
        else
        {
            ptHandle->nInited = INIT_MAGIC_NUM;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsTsemLU_t *ptHandle = (CamOsTsemLU_t *)ptTsem;
    if (ptHandle)
    {
        if (0 != sem_init(&ptHandle->tSem, 1, nVal))
        {
            CAM_OS_WARN("init fail");
            CamOsPrintf("err: %d\n", errno);
            eRet = CAM_OS_FAIL;
        }
        else
        {
            ptHandle->nInited = INIT_MAGIC_NUM;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsTsemLK_t *ptHandle = (CamOsTsemLK_t *)ptTsem;
    if (ptHandle)
    {
        sema_init(&ptHandle->tSem, nVal);
        ptHandle->nInited = INIT_MAGIC_NUM;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsTsemDeinit(CamOsTsem_t *ptTsem)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsTsemRtk_t *ptHandle = (CamOsTsemRtk_t *)ptTsem;
    if (ptHandle)
    {
        MsDestroyDynSem(&ptHandle->Tsem);
        ptHandle->nInited = 0;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsTsemLU_t *ptHandle = (CamOsTsemLU_t *)ptTsem;
    if (ptHandle)
    {
        if (0 != sem_destroy(&ptHandle->tSem))
        {
            eRet = CAM_OS_FAIL;
        }
        else
        {
            ptHandle->nInited = 0;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsTsemLK_t *ptHandle = (CamOsTsemLK_t *)ptTsem;
    if (ptHandle)
    {
        ptHandle->nInited = 0;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

void CamOsTsemUp(CamOsTsem_t *ptTsem)
{
#ifdef CAM_OS_RTK
    CamOsTsemRtk_t *ptHandle = (CamOsTsemRtk_t *)ptTsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Semaphore is not initialized before up\n");
            CamOsCallStack();
        }

        MsProduceDynSem(&ptHandle->Tsem);
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsTsemLU_t *ptHandle = (CamOsTsemLU_t *)ptTsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Semaphore is not initialized before up\n");
            CamOsCallStack();
        }

        sem_post(&ptHandle->tSem);
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsTsemLK_t *ptHandle = (CamOsTsemLK_t *)ptTsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Semaphore is not initialized before up\n");
            CamOsCallStack();
        }

        up(&ptHandle->tSem);
    }
#endif
}

void CamOsTsemDown(CamOsTsem_t *ptTsem)
{
#ifdef CAM_OS_RTK
    CamOsTsemRtk_t *ptHandle = (CamOsTsemRtk_t *)ptTsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Semaphore is not initialized before down\n");
            CamOsCallStack();
        }

        MsConsumeDynSem(&ptHandle->Tsem);
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsTsemLU_t *ptHandle = (CamOsTsemLU_t *)ptTsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Semaphore is not initialized before down\n");
            CamOsCallStack();
        }

        sem_wait(&ptHandle->tSem);
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsTsemLK_t *ptHandle = (CamOsTsemLK_t *)ptTsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Semaphore is not initialized before down\n");
            CamOsCallStack();
        }

        down(&ptHandle->tSem);
    }
#endif
}

CamOsRet_e CamOsTsemDownInterruptible(CamOsTsem_t *ptTsem)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsTsemRtk_t *ptHandle = (CamOsTsemRtk_t *)ptTsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Semaphore is not initialized before down_interruptible\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        MsConsumeDynSem(&ptHandle->Tsem);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsTsemLU_t *ptHandle = (CamOsTsemLU_t *)ptTsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Semaphore is not initialized before down_interruptible\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        if (0 != sem_wait(&ptHandle->tSem))
        {
            eRet = CAM_OS_FAIL;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsTsemLK_t *ptHandle = (CamOsTsemLK_t *)ptTsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Semaphore is not initialized before down_interruptible\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        if (-EINTR == down_interruptible(&ptHandle->tSem))
        {
            eRet = CAM_OS_INTERRUPTED;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsTsemTimedDown(CamOsTsem_t *ptTsem, u32 nMsec)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsTsemRtk_t *ptHandle = (CamOsTsemRtk_t *)ptTsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Semaphore is not initialized before timed_down\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        if (MS_NO_MESSAGE == MsConsumeDynSemDelay(&ptHandle->Tsem, nMsec))
        {
            eRet = CAM_OS_TIMEOUT;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsTsemLU_t * ptHandle = (CamOsTsemLU_t *)ptTsem;
    struct timespec tFinalTime;
    s64             nNanoDelay = 0;

    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Semaphore is not initialized before timed_down\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        if (clock_gettime(CLOCK_REALTIME, &tFinalTime) == -1)
            CAM_OS_WARN("clock_gettime fail");

        nNanoDelay = (nMsec * 1000000LL) + tFinalTime.tv_nsec;
        tFinalTime.tv_sec += (nNanoDelay / 1000000000LL);
        tFinalTime.tv_nsec = nNanoDelay % 1000000000LL;

        if (0 != sem_timedwait(&ptHandle->tSem, &tFinalTime))
        {
            if (errno == ETIMEDOUT)
            {
                eRet = CAM_OS_TIMEOUT;
            }
            else
            {
                CAM_OS_WARN("down fail");
                eRet = CAM_OS_FAIL;
            }
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsTsemLK_t *ptHandle = (CamOsTsemLK_t *)ptTsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Semaphore is not initialized before timed_down\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        if (0 != down_timeout(&ptHandle->tSem, msecs_to_jiffies(nMsec)))
        {
            eRet = CAM_OS_TIMEOUT;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsTsemTryDown(CamOsTsem_t *ptTsem)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsTsemRtk_t *ptHandle = (CamOsTsemRtk_t *)ptTsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Semaphore is not initialized before try_down\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        if (MS_UNIT_NOAVAIL == MsPollDynSem(&ptHandle->Tsem))
        {
            eRet = CAM_OS_RESOURCE_BUSY;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsTsemLU_t *ptHandle = (CamOsTsemLU_t *)ptTsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Semaphore is not initialized before try_down\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        if (0 != sem_trywait(&ptHandle->tSem))
        {
            if (errno == EAGAIN)
            {
                eRet = CAM_OS_RESOURCE_BUSY;
            }
            else
            {
                CAM_OS_WARN("down fail");
                eRet = CAM_OS_FAIL;
            }
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsTsemLK_t *ptHandle = (CamOsTsemLK_t *)ptTsem;
    s32            nErr;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Semaphore is not initialized before try_down\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        if (0 != (nErr = down_trylock(&ptHandle->tSem)))
        {
            CAM_OS_WARN("down fail");
            eRet = CAM_OS_RESOURCE_BUSY;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsRwsemInit(CamOsRwsem_t *ptRwsem)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsRwsemRtk_t *ptHandle = (CamOsRwsemRtk_t *)ptRwsem;
    if (ptHandle)
    {
        ptHandle->nReadCount = 0;
        if (MS_OK != MsInitMutex(&ptHandle->tRMutex) || MS_OK != MsCreateDynSem(&ptHandle->WTsem, 1))
        {
            CAM_OS_WARN("init fail");
            eRet = CAM_OS_FAIL;
        }
        ptHandle->nInited = INIT_MAGIC_NUM;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsRwsemLU_t *ptHandle = (CamOsRwsemLU_t *)ptRwsem;
    if (ptHandle)
    {
        if (0 != pthread_rwlock_init(&ptHandle->tRwsem, NULL))
        {
            CAM_OS_WARN("init fail");
            eRet = CAM_OS_FAIL;
        }
        else
        {
            ptHandle->nInited = INIT_MAGIC_NUM;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsRwsemLK_t *ptHandle = (CamOsRwsemLK_t *)ptRwsem;
    if (ptHandle)
    {
#ifdef CONFIG_LOCKDEP
        if (!_CamOsCheckStaticObj(&ptHandle->lockKey))
        {
            lockdep_register_key(&ptHandle->lockKey);
        }
        __init_rwsem(&ptHandle->tRwsem, "cam_os_rwsem", &ptHandle->lockKey);
#else
        init_rwsem(&ptHandle->tRwsem);
#endif
        ptHandle->nInited = INIT_MAGIC_NUM;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsRwsemDeinit(CamOsRwsem_t *ptRwsem)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsRwsemRtk_t *ptHandle = (CamOsRwsemRtk_t *)ptRwsem;
    if (ptHandle)
    {
        MsDestroyDynSem(&ptHandle->WTsem);
        ptHandle->nInited = 0;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsRwsemLU_t *ptHandle = (CamOsRwsemLU_t *)ptRwsem;
    if (ptHandle)
    {
        if (0 != pthread_rwlock_destroy(&ptHandle->tRwsem))
        {
            eRet = CAM_OS_FAIL;
        }
        else
        {
            ptHandle->nInited = 0;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsRwsemLK_t *ptHandle = (CamOsRwsemLK_t *)ptRwsem;
    if (ptHandle)
    {
#ifdef CONFIG_LOCKDEP
        if (!_CamOsCheckStaticObj(&ptHandle->lockKey))
        {
            lockdep_unregister_key(&ptHandle->lockKey);
        }
#endif
        ptHandle->nInited = 0;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

void CamOsRwsemUpRead(CamOsRwsem_t *ptRwsem)
{
#ifdef CAM_OS_RTK
    CamOsRwsemRtk_t *ptHandle = (CamOsRwsemRtk_t *)ptRwsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: RW_Semaphore is not initialized before up_read\n");
            CamOsCallStack();
        }

        MsMutexLock(&ptHandle->tRMutex);
        ptHandle->nReadCount--;
        if (ptHandle->nReadCount == 0)
            MsProduceDynSem(&ptHandle->WTsem);
        MsMutexUnlock(&ptHandle->tRMutex);
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsRwsemLU_t *ptHandle = (CamOsRwsemLU_t *)ptRwsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: RW_Semaphore is not initialized before up_read\n");
            CamOsCallStack();
        }

        pthread_rwlock_unlock(&ptHandle->tRwsem);
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsRwsemLK_t *ptHandle = (CamOsRwsemLK_t *)ptRwsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: RW_Semaphore is not initialized before up_read\n");
            CamOsCallStack();
        }

        up_read(&ptHandle->tRwsem);
    }
#endif
}

void CamOsRwsemUpWrite(CamOsRwsem_t *ptRwsem)
{
#ifdef CAM_OS_RTK
    CamOsRwsemRtk_t *ptHandle = (CamOsRwsemRtk_t *)ptRwsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: RW_Semaphore is not initialized before up_write\n");
            CamOsCallStack();
        }

        MsProduceDynSem(&ptHandle->WTsem);
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsRwsemLU_t *ptHandle = (CamOsRwsemLU_t *)ptRwsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: RW_Semaphore is not initialized before up_write\n");
            CamOsCallStack();
        }

        pthread_rwlock_unlock(&ptHandle->tRwsem);
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsRwsemLK_t *ptHandle = (CamOsRwsemLK_t *)ptRwsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: RW_Semaphore is not initialized before up_write\n");
            CamOsCallStack();
        }

        up_write(&ptHandle->tRwsem);
    }
#endif
}

void CamOsRwsemDownRead(CamOsRwsem_t *ptRwsem)
{
#ifdef CAM_OS_RTK
    CamOsRwsemRtk_t *ptHandle = (CamOsRwsemRtk_t *)ptRwsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: RW_Semaphore is not initialized before down_read\n");
            CamOsCallStack();
        }

        MsMutexLock(&ptHandle->tRMutex);
        ptHandle->nReadCount++;
        if (ptHandle->nReadCount == 1)
            MsConsumeDynSem(&ptHandle->WTsem);
        MsMutexUnlock(&ptHandle->tRMutex);
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsRwsemLU_t *ptHandle = (CamOsRwsemLU_t *)ptRwsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: RW_Semaphore is not initialized before down_read\n");
            CamOsCallStack();
        }

        pthread_rwlock_rdlock(&ptHandle->tRwsem);
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsRwsemLK_t *ptHandle = (CamOsRwsemLK_t *)ptRwsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: RW_Semaphore is not initialized before down_read\n");
            CamOsCallStack();
        }

        down_read(&ptHandle->tRwsem);
    }
#endif
}

void CamOsRwsemDownWrite(CamOsRwsem_t *ptRwsem)
{
#ifdef CAM_OS_RTK
    CamOsRwsemRtk_t *ptHandle = (CamOsRwsemRtk_t *)ptRwsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: RW_Semaphore is not initialized before down_write\n");
            CamOsCallStack();
        }

        MsConsumeDynSem(&ptHandle->WTsem);
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsRwsemLU_t *ptHandle = (CamOsRwsemLU_t *)ptRwsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: RW_Semaphore is not initialized before down_write\n");
            CamOsCallStack();
        }

        pthread_rwlock_wrlock(&ptHandle->tRwsem);
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsRwsemLK_t *ptHandle = (CamOsRwsemLK_t *)ptRwsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: RW_Semaphore is not initialized before down_write\n");
            CamOsCallStack();
        }

        down_write(&ptHandle->tRwsem);
    }
#endif
}

CamOsRet_e CamOsRwsemTryDownRead(CamOsRwsem_t *ptRwsem)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsRwsemRtk_t *ptHandle = (CamOsRwsemRtk_t *)ptRwsem;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: RW_Semaphore is not initialized before try_down_read\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        if (MS_UNIT_NOAVAIL == MsMutexTryLock(&ptHandle->tRMutex))
        {
            eRet = CAM_OS_RESOURCE_BUSY;
        }
        else
        {
            ptHandle->nReadCount++;
            if (ptHandle->nReadCount == 1)
            {
                if (MS_UNIT_NOAVAIL == MsPollDynSem(&ptHandle->WTsem))
                {
                    eRet = CAM_OS_RESOURCE_BUSY;
                    ptHandle->nReadCount--;
                }
            }
            MsMutexUnlock(&ptHandle->tRMutex);
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsRwsemLU_t *ptHandle = (CamOsRwsemLU_t *)ptRwsem;
    s32             nErr;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: RW_Semaphore is not initialized before try_down_read\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        if (0 != (nErr = pthread_rwlock_tryrdlock(&ptHandle->tRwsem)))
        {
            if (nErr == EBUSY)
            {
                eRet = CAM_OS_RESOURCE_BUSY;
            }
            else
            {
                CAM_OS_WARN("lock fail");
                eRet = CAM_OS_FAIL;
            }
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsRwsemLK_t *ptHandle = (CamOsRwsemLK_t *)ptRwsem;
    s32             nErr;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: RW_Semaphore is not initialized before try_down_read\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        if (1 != (nErr = down_read_trylock(&ptHandle->tRwsem)))
        {
            eRet = CAM_OS_RESOURCE_BUSY;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsRwsemTryDownWrite(CamOsRwsem_t *ptRwsem)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsRwsemRtk_t *ptHandle = (CamOsRwsemRtk_t *)ptRwsem;

    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: RW_Semaphore is not initialized before try_down_write\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        if (MS_UNIT_NOAVAIL == MsPollDynSem(&ptHandle->WTsem))
        {
            eRet = CAM_OS_RESOURCE_BUSY;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsRwsemLU_t *ptHandle = (CamOsRwsemLU_t *)ptRwsem;
    s32             nErr;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: RW_Semaphore is not initialized before try_down_write\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        if (0 != (nErr = pthread_rwlock_trywrlock(&ptHandle->tRwsem)))
        {
            if (nErr == EBUSY)
            {
                eRet = CAM_OS_RESOURCE_BUSY;
            }
            else
            {
                CAM_OS_WARN("lock fail");
                eRet = CAM_OS_FAIL;
            }
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsRwsemLK_t *ptHandle = (CamOsRwsemLK_t *)ptRwsem;
    s32             nErr;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: RW_Semaphore is not initialized before try_down_write\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        if (1 != (nErr = down_write_trylock(&ptHandle->tRwsem)))
        {
            eRet = CAM_OS_RESOURCE_BUSY;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsSpinInit(CamOsSpinlock_t *ptSpinlock)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsSpinlockRtk_t *ptHandle = (CamOsSpinlockRtk_t *)ptSpinlock;
    if (ptHandle)
    {
        ptHandle->nFlags  = 0;
        ptHandle->nInited = INIT_MAGIC_NUM;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsSpinlockLU_t *ptHandle = (CamOsSpinlockLU_t *)ptSpinlock;
    if (ptHandle)
    {
        if (0 != pthread_spin_init(&ptHandle->tLock, 0))
        {
            eRet = CAM_OS_FAIL;
        }
        else
        {
            ptHandle->nInited = INIT_MAGIC_NUM;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsSpinlockLK_t *ptHandle = (CamOsSpinlockLK_t *)ptSpinlock;
    if (ptHandle)
    {
#ifdef CONFIG_LOCKDEP
        if (!_CamOsCheckStaticObj(&ptHandle->lockKey))
        {
            lockdep_register_key(&ptHandle->lockKey);
        }
        __raw_spin_lock_init(spinlock_check(&ptHandle->tLock), "cam_os_spin", &ptHandle->lockKey, LD_WAIT_CONFIG);
#else
        spin_lock_init(&ptHandle->tLock);
#endif
        ptHandle->nInited = INIT_MAGIC_NUM;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsSpinDeinit(CamOsSpinlock_t *ptSpinlock)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsSpinlockRtk_t *ptHandle = (CamOsSpinlockRtk_t *)ptSpinlock;
    if (ptHandle)
    {
        ptHandle->nInited = 0;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsSpinlockLU_t *ptHandle = (CamOsSpinlockLU_t *)ptSpinlock;
    if (ptHandle)
    {
        pthread_spin_destroy(&ptHandle->tLock);
        ptHandle->nInited = 0;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsSpinlockLK_t *ptHandle = (CamOsSpinlockLK_t *)ptSpinlock;
    if (ptHandle)
    {
#ifdef CONFIG_LOCKDEP
        if (!_CamOsCheckStaticObj(&ptHandle->lockKey))
        {
            lockdep_unregister_key(&ptHandle->lockKey);
        }
#endif
        ptHandle->nInited = 0;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsSpinLock(CamOsSpinlock_t *ptSpinlock)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsSpinlockRtk_t *ptHandle = (CamOsSpinlockRtk_t *)ptSpinlock;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Spinlock is not initialized before lock\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        MsEnterRegion();
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsSpinlockLU_t *ptHandle = (CamOsSpinlockLU_t *)ptSpinlock;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Spinlock is not initialized before lock\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        if (0 != pthread_spin_lock(&ptHandle->tLock))
        {
            eRet = CAM_OS_FAIL;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsSpinlockLK_t *ptHandle = (CamOsSpinlockLK_t *)ptSpinlock;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Spinlock is not initialized before lock\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        spin_lock(&ptHandle->tLock);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsSpinUnlock(CamOsSpinlock_t *ptSpinlock)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsSpinlockRtk_t *ptHandle = (CamOsSpinlockRtk_t *)ptSpinlock;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Spinlock is not initialized before unlock\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        MsLeaveRegion();
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsSpinlockLU_t *ptHandle = (CamOsSpinlockLU_t *)ptSpinlock;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Spinlock is not initialized before unlock\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        if (0 != pthread_spin_unlock(&ptHandle->tLock))
        {
            eRet = CAM_OS_FAIL;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsSpinlockLK_t *ptHandle = (CamOsSpinlockLK_t *)ptSpinlock;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Spinlock is not initialized before unlock\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        spin_unlock(&ptHandle->tLock);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsSpinLockIrqSave(CamOsSpinlock_t *ptSpinlock)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsSpinlockRtk_t *ptHandle = (CamOsSpinlockRtk_t *)ptSpinlock;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Spinlock is not initialized before lock_irq_save\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        MsEnterRegionSaveIrq(&ptHandle->nFlags);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
    eRet = CAM_OS_FAIL;
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsSpinlockLK_t *ptHandle = (CamOsSpinlockLK_t *)ptSpinlock;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Spinlock is not initialized before lock_irq_save\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        spin_lock_irqsave(&ptHandle->tLock, ptHandle->nFlags);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsSpinUnlockIrqRestore(CamOsSpinlock_t *ptSpinlock)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsSpinlockRtk_t *ptHandle = (CamOsSpinlockRtk_t *)ptSpinlock;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Spinlock is not initialized before lock_irq_restore\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        MsLeaveRegionRestoreIrq(&ptHandle->nFlags);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
    eRet = CAM_OS_FAIL;
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsSpinlockLK_t *ptHandle = (CamOsSpinlockLK_t *)ptSpinlock;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Spinlock is not initialized before lock_irq_restore\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        spin_unlock_irqrestore(&ptHandle->tLock, ptHandle->nFlags);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsRtMutexInit(CamOsRtMutex_t *ptRtMutex)
{
    CamOsRet_e eRet = CAM_OS_OK;

#if defined(CAM_OS_LINUX_KERNEL)
    CamOsRtMutexLK_t *ptHandle = (CamOsRtMutexLK_t *)ptRtMutex;
    if (ptHandle)
    {
#ifdef CONFIG_LOCKDEP
        if (!_CamOsCheckStaticObj(&ptHandle->lockKey))
        {
            lockdep_register_key(&ptHandle->lockKey);
        }
        __rt_mutex_init(&ptHandle->tRtMutex, "cam_os_rtmutex", &ptHandle->lockKey);
#else
        rt_mutex_init(&ptHandle->tRtMutex);
#endif
        ptHandle->nInited = INIT_MAGIC_NUM;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsRtMutexDestroy(CamOsRtMutex_t *ptRtMutex)
{
    CamOsRet_e eRet = CAM_OS_OK;

#if defined(CAM_OS_LINUX_KERNEL)
    CamOsRtMutexLK_t *ptHandle = (CamOsRtMutexLK_t *)ptRtMutex;
    if (ptHandle)
    {
#ifdef CONFIG_LOCKDEP
        if (!_CamOsCheckStaticObj(&ptHandle->lockKey))
        {
            lockdep_unregister_key(&ptHandle->lockKey);
        }
#endif
        rt_mutex_destroy(&ptHandle->tRtMutex);
        ptHandle->nInited = 0;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsRtMutexLock(CamOsRtMutex_t *ptRtMutex)
{
    CamOsRet_e eRet = CAM_OS_OK;

#if defined(CAM_OS_LINUX_KERNEL)
    CamOsRtMutexLK_t *ptHandle = (CamOsRtMutexLK_t *)ptRtMutex;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Mutex is not initialized before lock\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        rt_mutex_lock(&ptHandle->tRtMutex);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsRtMutexUnlock(CamOsRtMutex_t *ptRtMutex)
{
    CamOsRet_e eRet = CAM_OS_OK;

#if defined(CAM_OS_LINUX_KERNEL)
    CamOsRtMutexLK_t *ptHandle = (CamOsRtMutexLK_t *)ptRtMutex;
    if (ptHandle)
    {
        if (ptHandle->nInited != INIT_MAGIC_NUM)
        {
            CamOsPrintf("WARNING: Mutex is not initialized before unlock\n");
            CamOsCallStack();
            eRet = CAM_OS_FAIL;
        }

        rt_mutex_unlock(&ptHandle->tRtMutex);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

static CamOsWork_t *_CamOsWorkDeQueue(CamOsWorkQueuePriv_t *ptWorkQueuePriv)
{
    u32          i          = 0;
    u64          serial_max = 0xFFFFFFFFFFFFFFFF;
    CamOsWork_t *ptWork     = NULL;

    // Find oldest immediately work
    CamOsSpinLockIrqSave(&ptWorkQueuePriv->tLock);
    for (i = 0; i < ptWorkQueuePriv->nMax; i++)
    {
        if (ptWorkQueuePriv->tWorks[i].eStatus == CAM_OS_WORKQUEUE_WAIT)
        {
            if (ptWorkQueuePriv->tWorks[i].nWorkSerialNum < serial_max)
            {
                serial_max = ptWorkQueuePriv->tWorks[i].nWorkSerialNum;
                ptWork     = &ptWorkQueuePriv->tWorks[i];
            }
        }
    }
    CamOsSpinUnlockIrqRestore(&ptWorkQueuePriv->tLock);

    return ptWork;
}

static void _CmaOsWorkQurueDelayCb(void *nDataAddr)
{
    CamOsWork_t *         tWork           = (CamOsWork_t *)nDataAddr;
    CamOsWorkQueuePriv_t *ptWorkQueuePriv = (CamOsWorkQueuePriv_t *)tWork->pWorkQueue;

    CamOsSpinLockIrqSave(&ptWorkQueuePriv->tLock);
    if (tWork->eStatus == CAM_OS_WORKQUEUE_WAIT_DELAY)
    {
        tWork->eStatus = CAM_OS_WORKQUEUE_WAIT;
    }
    else
    {
        tWork->pfnFunc = NULL;
        CamOsPrintf("%s: work status error (%d)\n", __FUNCTION__, tWork->eStatus);
    }
    CamOsSpinUnlockIrqRestore(&ptWorkQueuePriv->tLock);

    CamOsTimerDelete(&tWork->tTimer);
    CamOsTsemUp(&ptWorkQueuePriv->tTsem);

    return;
}

static s32 _CamOsWorkQueueProcThread(void *pArg)
{
    CamOsWork_t *         tWork           = NULL;
    CamOsWorkQueuePriv_t *ptWorkQueuePriv = (CamOsWorkQueuePriv_t *)pArg;

    while (1)
    {
        CamOsRet_e eRet = CamOsTsemTimedDown(&ptWorkQueuePriv->tTsem, 1000);

        if (CamOsAtomicRead(&ptWorkQueuePriv->tReady) == 0)
            break;

        if (eRet == CAM_OS_OK || eRet == CAM_OS_TIMEOUT)
        {
            while (1)
            {
                tWork = _CamOsWorkDeQueue(ptWorkQueuePriv);
                if (tWork)
                {
                    CamOsSpinLockIrqSave(&ptWorkQueuePriv->tLock);
                    if (tWork->eStatus == CAM_OS_WORKQUEUE_WAIT)
                    {
                        tWork->eStatus = CAM_OS_WORKQUEUE_EXECUTING;
                    }
                    CamOsSpinUnlockIrqRestore(&ptWorkQueuePriv->tLock);

                    if (tWork->pfnFunc)
                    {
                        tWork->pfnFunc(tWork->pData);
                    }

                    CamOsSpinLockIrqSave(&ptWorkQueuePriv->tLock);
                    tWork->pfnFunc = NULL;
                    tWork->eStatus = CAM_OS_WORKQUEUE_AVAILABLE;
                    CamOsSpinUnlockIrqRestore(&ptWorkQueuePriv->tLock);
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            // CamOsPrintf("Wait Unknown\n");
            break;
        }
    }
    return 0;
}

CamOsRet_e CamOsWorkQueueCreate(CamOsWorkQueue *ptWorkQueue, const char *szName, u32 nMax)
{
    CamOsWorkQueuePriv_t *ptWorkQueuePriv;
    CamOsThreadAttrb_t    tAttr = {0};
    u32                   i     = 0;

    ptWorkQueuePriv = (CamOsWorkQueuePriv_t *)CamOsMemCalloc(1, sizeof(CamOsWorkQueuePriv_t));

    CamOsSpinInit(&ptWorkQueuePriv->tLock);

    // Initialize works
    ptWorkQueuePriv->tWorks = (CamOsWork_t *)CamOsMemCalloc(1, nMax * sizeof(CamOsWork_t));
    for (i = 0; i < nMax; i++)
    {
        ptWorkQueuePriv->tWorks[i].pWorkQueue     = ptWorkQueuePriv;
        ptWorkQueuePriv->tWorks[i].nWorkSerialNum = 0;
        ptWorkQueuePriv->tWorks[i].pfnFunc        = NULL;
        ptWorkQueuePriv->tWorks[i].eStatus        = CAM_OS_WORKQUEUE_AVAILABLE;
    }

    ptWorkQueuePriv->nMax              = nMax;
    ptWorkQueuePriv->nWorkSerialNumMax = 0;

    /* Initialize condition wait object*/
    CamOsTsemInit(&ptWorkQueuePriv->tTsem, 0);

    /* Create thread */
    tAttr.nPriority  = 50;
    tAttr.nStackSize = 2048;
    tAttr.szName     = (char *)szName;

    CamOsThreadCreate(&(ptWorkQueuePriv->tThread), &tAttr, (void *)_CamOsWorkQueueProcThread, ptWorkQueuePriv);

    CamOsAtomicSet(&ptWorkQueuePriv->tReady, 1);

    *ptWorkQueue = (void *)ptWorkQueuePriv;

    return CAM_OS_OK;
}

CamOsRet_e CamOsWorkQueueDestroy(CamOsWorkQueue ptWorkQueue)
{
    CamOsWorkQueuePriv_t *ptWorkQueuePriv = (CamOsWorkQueuePriv_t *)ptWorkQueue;
    u32                   i               = 0;

    if (ptWorkQueuePriv)
    {
        CamOsAtomicSet(&ptWorkQueuePriv->tReady, 0);
        CamOsTsemUp(&ptWorkQueuePriv->tTsem);

#if defined(CAM_OS_RTK) || defined(CAM_OS_LINUX_USER)
        CamOsThreadJoin(ptWorkQueuePriv->tThread);
#endif
        CamOsTsemDeinit(&ptWorkQueuePriv->tTsem);
        for (i = 0; i < ptWorkQueuePriv->nMax; i++)
        {
            if (ptWorkQueuePriv->tWorks[i].eStatus == CAM_OS_WORKQUEUE_WAIT_DELAY)
            {
                CamOsTimerDeleteSync(&ptWorkQueuePriv->tWorks[i].tTimer);
            }
        }

        CamOsSpinDeinit(&ptWorkQueuePriv->tLock);

        CamOsMemRelease(ptWorkQueuePriv->tWorks);
        CamOsMemRelease(ptWorkQueuePriv);

        return CAM_OS_OK;
    }
    else
    {
        return CAM_OS_FAIL;
    }
}

CamOsRet_e CamOsWorkQueueAdd(CamOsWorkQueue ptWorkQueue, void (*pfnFunc)(void *), void *pData, u32 nDelay)
{
    CamOsWorkQueuePriv_t *ptWorkQueuePriv = (CamOsWorkQueuePriv_t *)ptWorkQueue;
    CamOsWork_t *         ptWork          = NULL;
    u32                   i               = 0;

    if (!CamOsAtomicRead(&ptWorkQueuePriv->tReady))
        return CAM_OS_FAIL;

    CamOsSpinLockIrqSave(&ptWorkQueuePriv->tLock);
    for (i = 0; i < ptWorkQueuePriv->nMax; i++)
    {
        if (ptWorkQueuePriv->tWorks[i].eStatus == CAM_OS_WORKQUEUE_AVAILABLE)
        {
            ptWork = &ptWorkQueuePriv->tWorks[i];
            break;
        }
    }
    CamOsSpinUnlockIrqRestore(&ptWorkQueuePriv->tLock);

    if (ptWork)
    {
        ptWork->pfnFunc = pfnFunc;
        ptWork->pData   = pData;

        if (!nDelay)
        {
            ptWork->eStatus = CAM_OS_WORKQUEUE_WAIT;
            ptWorkQueuePriv->nWorkSerialNumMax++;
            ptWork->nWorkSerialNum = ptWorkQueuePriv->nWorkSerialNumMax;
            CamOsTsemUp(&ptWorkQueuePriv->tTsem);
        }
        else
        {
            ptWork->eStatus = CAM_OS_WORKQUEUE_WAIT_DELAY;
            CamOsTimerInit(&ptWork->tTimer);
            CamOsTimerAdd(&ptWork->tTimer, nDelay, ptWork, _CmaOsWorkQurueDelayCb);
        }

        return CAM_OS_OK;
    }
    else
    {
        CamOsPrintf("%s: work exceed max num %d\n", __FUNCTION__, ptWorkQueuePriv->nMax);
        return CAM_OS_FAIL;
    }
}

CamOsRet_e CamOsWorkQueueCancel(CamOsWorkQueue ptWorkQueue, void *pData)
{
    CamOsWorkQueuePriv_t *ptWorkQueuePriv = (CamOsWorkQueuePriv_t *)ptWorkQueue;
    u32                   i               = 0;

    if (!CamOsAtomicRead(&ptWorkQueuePriv->tReady))
        return CAM_OS_FAIL;

    CamOsSpinLockIrqSave(&ptWorkQueuePriv->tLock);
    for (i = 0; i < ptWorkQueuePriv->nMax; i++)
    {
        if (ptWorkQueuePriv->tWorks[i].pData == pData && ptWorkQueuePriv->tWorks[i].eStatus == CAM_OS_WORKQUEUE_WAIT)
        {
            ptWorkQueuePriv->tWorks[i].pfnFunc = NULL;
            ptWorkQueuePriv->tWorks[i].eStatus = CAM_OS_WORKQUEUE_AVAILABLE;
        }

        if (ptWorkQueuePriv->tWorks[i].pData == pData
            && ptWorkQueuePriv->tWorks[i].eStatus == CAM_OS_WORKQUEUE_WAIT_DELAY)
        {
            CamOsTimerDeleteSync(&ptWorkQueuePriv->tWorks[i].tTimer);
            ptWorkQueuePriv->tWorks[i].pfnFunc = NULL;
            ptWorkQueuePriv->tWorks[i].eStatus = CAM_OS_WORKQUEUE_AVAILABLE;
        }
    }
    CamOsSpinUnlockIrqRestore(&ptWorkQueuePriv->tLock);

    return CAM_OS_OK;
}

#if defined(CONFIG_TRACE_CAM_OS_MEM)
static void _CamOSTraceInfoAlloc(unsigned long nPtr, u32 nSize, CamOsMemTrace_e eTraceType, unsigned long nTraceCaller)
{
    CamOsMemTraceInfo_t *pstMemInfo = NULL;
    unsigned long        flags      = 0;

    pstMemInfo = (CamOsMemTraceInfo_t *)kzalloc(sizeof(CamOsMemTraceInfo_t), in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
    if (!pstMemInfo)
    {
        printk(KERN_WARNING "%s alloc TRACE_MEM INFO fail\n", __FUNCTION__);
        return;
    }
    memset(pstMemInfo, 0, sizeof(CamOsMemTraceInfo_t));
    pstMemInfo->nVirAddr   = nPtr;
    pstMemInfo->nSize      = nSize;
    pstMemInfo->nTraceFunc = nTraceCaller;
    spin_lock_irqsave(&_gtCamOsMemTracelock, flags);
    CAM_OS_HASH_ADD(_gtCamOsMemTraceHash, &pstMemInfo->tHList, eTraceType);
    spin_unlock_irqrestore(&_gtCamOsMemTracelock, flags);
}

static void _CamOSTraceInfoFree(unsigned long nPtr, CamOsMemTrace_e eTraceType)
{
    CamOsMemTraceInfo_t *    pstMemInfo = NULL;
    struct CamOsHListNode_t *pos        = NULL;
    unsigned long            flags      = 0;

    spin_lock_irqsave(&_gtCamOsMemTracelock, flags);
    CAM_OS_HASH_FOR_EACH_POSSIBLE_SAFE(_gtCamOsMemTraceHash, pstMemInfo, pos, tHList, eTraceType)
    {
        if (pstMemInfo && (pstMemInfo->nVirAddr == nPtr))
        {
            CAM_OS_HASH_DEL(&pstMemInfo->tHList);
            spin_unlock_irqrestore(&_gtCamOsMemTracelock, flags);
            kfree((void *)pstMemInfo);
            return;
        }
    }
    spin_unlock_irqrestore(&_gtCamOsMemTracelock, flags);
}

#endif

void *CamOsMemAlloc(u32 nSize)
{
#ifdef CAM_OS_RTK
    return MsGetCHeapMemory(nSize);
#elif defined(CAM_OS_LINUX_USER)
    return malloc(nSize);
#elif defined(CAM_OS_LINUX_KERNEL)
    void *pPtr = NULL;
    if (nSize > KMALLOC_THRESHOLD_SIZE)
        pPtr = vzalloc(nSize);
    else
        pPtr = kzalloc(nSize, in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
#if defined(CONFIG_TRACE_CAM_OS_MEM)
    _CamOSTraceInfoAlloc((unsigned long)pPtr, nSize, CAM_OS_MEM_TRACE_ALLOC,
                         (unsigned long)__builtin_return_address(0));
#endif
    return pPtr;
#endif
}

void *CamOsMemAllocAtomic(u32 nSize)
{
#ifdef CAM_OS_RTK
    return MsGetCHeapMemory(nSize);
#elif defined(CAM_OS_LINUX_USER)
    return malloc(nSize);
#elif defined(CAM_OS_LINUX_KERNEL)
    if (nSize > KMALLOC_THRESHOLD_SIZE)
        return NULL;
    else
    {
        void *pPtr = NULL;
        pPtr       = kzalloc(nSize, GFP_ATOMIC);
#if defined(CONFIG_TRACE_CAM_OS_MEM)
        _CamOSTraceInfoAlloc((unsigned long)pPtr, nSize, CAM_OS_MEM_TRACE_ALLOC_ATOMIC,
                             (unsigned long)__builtin_return_address(0));
#endif
        return pPtr;
    }
#endif
}

void *CamOsMemCalloc(u32 nNum, u32 nSize)
{
#ifdef CAM_OS_RTK
    return MsGetCHeapMemory(nNum * nSize);
#elif defined(CAM_OS_LINUX_USER)
    return calloc(nNum, nSize);
#elif defined(CAM_OS_LINUX_KERNEL)
    void *pPtr = NULL;
    if ((nNum * nSize) > KMALLOC_THRESHOLD_SIZE)
        pPtr = vzalloc(nNum * nSize);
    else
        pPtr = kzalloc(nNum * nSize, in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
#if defined(CONFIG_TRACE_CAM_OS_MEM)
    _CamOSTraceInfoAlloc((unsigned long)pPtr, nNum * nSize, CAM_OS_MEM_TRACE_CALLOC,
                         (unsigned long)__builtin_return_address(0));
#endif
    return pPtr;
#endif
}

void *CamOsMemCallocAtomic(u32 nNum, u32 nSize)
{
#ifdef CAM_OS_RTK
    return MsGetCHeapMemory(nNum * nSize);
#elif defined(CAM_OS_LINUX_USER)
    return calloc(nNum, nSize);
#elif defined(CAM_OS_LINUX_KERNEL)
    if ((nNum * nSize) > KMALLOC_THRESHOLD_SIZE)
        return NULL;
    else
    {
        void *pPtr = NULL;
        pPtr       = kzalloc(nNum * nSize, GFP_ATOMIC);
#if defined(CONFIG_TRACE_CAM_OS_MEM)
        _CamOSTraceInfoAlloc((unsigned long)pPtr, nNum * nSize, CAM_OS_MEM_TRACE_CALLOC_ATOMIC,
                             (unsigned long)__builtin_return_address(0));
#endif
        return pPtr;
    }
#endif
}

void CamOsMemRelease(void *pPtr)
{
#ifdef CAM_OS_RTK
    if (pPtr)
    {
        MsReleaseMemory(pPtr);
    }
#elif defined(CAM_OS_LINUX_USER)
    if (pPtr)
    {
        free(pPtr);
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    if (pPtr)
    {
#if defined(CONFIG_TRACE_CAM_OS_MEM)
        CamOsMemTrace_e eMemType = 0;
        for (eMemType = CAM_OS_MEM_TRACE_ALLOC; eMemType <= CAM_OS_MEM_TRACE_CALLOC_ATOMIC; eMemType++)
        {
            _CamOSTraceInfoFree((unsigned long)pPtr, eMemType);
        }
#endif
        kvfree(pPtr);
    }
#endif
}

void CamOsMemFlush(void *pPtr, u32 nSize)
{
#ifdef CAM_OS_RTK
    sys_flush_data_cache_buffer((u32)pPtr, nSize);
#elif defined(CAM_OS_LINUX_USER)
    // TODO: implement cache flush in linux user space.
    CAM_OS_WARN("not support in " OS_NAME);
#elif defined(CAM_OS_LINUX_KERNEL)
    Chip_Flush_Cache_Range(pPtr, nSize);
#endif
}

void CamOsMemFlushExt(void *pVa, ss_phys_addr_t pPa, u32 nSize)
{
#ifdef CAM_OS_RTK
    CAM_OS_WARN("not support in " OS_NAME);
#elif defined(CAM_OS_LINUX_USER)
    // TODO: implement cache flush in linux user space.
    CAM_OS_WARN("not support in " OS_NAME);
#elif defined(CAM_OS_LINUX_KERNEL)
    Chip_Flush_Cache_Range_VA_PA(pVa, pPa, nSize);
#endif
}

void CamOSMemTraceSort(int (*pfnFunc)(CamOsMemTraceInfo_t *, void *), void *pData)
{
#if defined(CONFIG_TRACE_CAM_OS_MEM)
    CamOsMemTrace_e      eMemType   = 0;
    unsigned long        flags      = 0;
    CamOsMemTraceInfo_t *pstMemInfo = NULL;

    if (!pfnFunc)
        return;
    spin_lock_irqsave(&_gtCamOsMemTracelock, flags);
    for (eMemType = CAM_OS_MEM_TRACE_ALLOC; eMemType < CAM_OS_MEM_TRACE_MAX; eMemType++)
    {
        CAM_OS_HASH_FOR_EACH_POSSIBLE(_gtCamOsMemTraceHash, pstMemInfo, tHList, eMemType)
        {
            pfnFunc(pstMemInfo, pData);
        }
    }
    spin_unlock_irqrestore(&_gtCamOsMemTracelock, flags);
#else
    CAM_OS_WARN("not set CONFIG_TRACE_CAM_OS_MEM in " OS_NAME);
#endif
}

void CamOsMemInvalidate(void *pPtr, u32 nSize)
{
#ifdef CAM_OS_RTK
    sys_Invalidate_data_cache_buffer((u32)pPtr, nSize);
#elif defined(CAM_OS_LINUX_USER)
    // TODO: implement cache flush in linux user space.
    CAM_OS_WARN("not support in " OS_NAME);
#elif defined(CAM_OS_LINUX_KERNEL)
    Chip_Inv_Cache_Range(pPtr, nSize);
#endif
}

void CamOsMiuPipeFlush(void)
{
#ifdef CAM_OS_RTK
    DrvChipFlushMiuPipe();
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
#elif defined(CAM_OS_LINUX_KERNEL)
    Chip_Flush_MIU_Pipe();
#endif
}

static s32 _CheckContMemInfoListInited(void)
{
#ifdef CAM_OS_RTK
    MsMutexLock(&_gtMemLock);
#elif defined(CAM_OS_LINUX_USER)
    pthread_mutex_lock(&_gtMemLock);
#elif defined(CAM_OS_LINUX_KERNEL)
    mutex_lock(&_gtMemLock);
#endif
    if (!_gnContMemListInited)
    {
        memset(&_gtContMemList, 0, sizeof(ContMemoryList_t));
        CAM_OS_INIT_LIST_HEAD(&_gtContMemList.tList);

        _gnContMemListInited = 1;
    }
#ifdef CAM_OS_RTK
    MsMutexUnlock(&_gtMemLock);
#elif defined(CAM_OS_LINUX_USER)
    pthread_mutex_unlock(&_gtMemLock);
#elif defined(CAM_OS_LINUX_KERNEL)
    mutex_unlock(&_gtMemLock);
#endif

    return 0;
}

ss_phys_addr_t CamOsContiguousMemAlloc(u32 nSize)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    u8    nAllocSucc   = TRUE;
    void *pNonCachePtr = NULL;

    pNonCachePtr = MsAllocateNonCacheMemExt(nSize, 12);
    nAllocSucc &= MsIsHeapMemory(pNonCachePtr);

    if ((u32)pNonCachePtr & ((1 << 6) - 1))
    {
        nAllocSucc &= FALSE;
        MsReleaseMemory(pNonCachePtr);
    }

    if (nAllocSucc == TRUE)
    {
        ASSIGN_POINTER_VALUE(ppVirtAddr, pNonCachePtr);
        ASSIGN_POINTER_VALUE(ptPhysAddr, MsVA2PA(pNonCachePtr));
        ASSIGN_POINTER_VALUE(ptMiuAddr, (ss_miu_addr_t)HalUtilPHY2MIUAddr((u32)*ptPhysAddr));

        _CheckContMemInfoListInited();

        MsMutexLock(&_gtMemLock);
        MemoryList_t *ptNewEntry = (MemoryList_t *)MsAllocateMem(sizeof(MemoryList_t));
        ptNewEntry->tPhysAddr    = *ptPhysAddr;
        ptNewEntry->pVirtPtr     = *ppVirtAddr;
        ptNewEntry->pMemifoPtr   = NULL;
        ptNewEntry->szName       = (char *)MsAllocateMem(strlen(szName) + 1);
        if (ptNewEntry->szName)
            strncpy(ptNewEntry->szName, szName, strlen(szName));
        ptNewEntry->nSize = nSize;
        CAM_OS_LIST_ADD_TAIL(&(ptNewEntry->tList), &_gtMemList.tList);
        MsMutexUnlock(&_gtMemLock);
    }
    else
    {
        ASSIGN_POINTER_VALUE(ppVirtAddr, NULL);
        ASSIGN_POINTER_VALUE(ptPhysAddr, 0);
        ASSIGN_POINTER_VALUE(ptMiuAddr, 0);
        eRet = CAM_OS_FAIL;
    }
#elif defined(CAM_OS_LINUX_USER)
#ifndef NO_MDRV_MSYS
    s32             nMsysFd   = -1;
    MSYS_DMEM_INFO *ptMsysMem = NULL;
    ss_phys_addr_t  tPhysAddr = 0;

    do
    {
        _CheckContMemInfoListInited();

        if (0 > (nMsysFd = open("/dev/msys", O_RDWR | O_SYNC)))
        {
            fprintf(stderr, "%s open /dev/msys failed!!\n", __FUNCTION__);
            eRet = CAM_OS_FAIL;
            break;
        }

        ptMsysMem = (MSYS_DMEM_INFO *)malloc(sizeof(MSYS_DMEM_INFO));
        MSYS_ADDR_TRANSLATION_INFO tAddrInfo;
        FILL_VERCHK_TYPE(tAddrInfo, tAddrInfo.VerChk_Version, tAddrInfo.VerChk_Size, IOCTL_MSYS_VERSION);
        FILL_VERCHK_TYPE(*ptMsysMem, ptMsysMem->VerChk_Version, ptMsysMem->VerChk_Size, IOCTL_MSYS_VERSION);

        ptMsysMem->length = nSize;
        snprintf(ptMsysMem->name, sizeof(ptMsysMem->name), "CamOs%d", _gnContMemReqCnt++);

        if (ioctl(nMsysFd, IOCTL_MSYS_REQUEST_DMEM, ptMsysMem))
        {
            ptMsysMem->length = 0;
            fprintf(stderr, "%s [%d]Request Direct Memory Failed!!\n", __FUNCTION__, nSize);
            free(ptMsysMem);
            eRet = CAM_OS_FAIL;
            break;
        }

        if (ptMsysMem->length < nSize)
        {
            ioctl(nMsysFd, IOCTL_MSYS_RELEASE_DMEM, ptMsysMem);
            fprintf(stderr, "%s Request Direct Memory Failed!! because dmem size <%d>smaller than <%d>\n", __FUNCTION__,
                    ptMsysMem->length, nSize);
            free(ptMsysMem);
            eRet = CAM_OS_FAIL;
            break;
        }

        tPhysAddr = ptMsysMem->phys;

        fprintf(stderr, "%s physAddr<0x%llx> size<%d>\n", __FUNCTION__, ptMsysMem->phys, ptMsysMem->length);

        pthread_mutex_lock(&_gtMemLock);
        ContMemoryList_t *ptNewEntry = (ContMemoryList_t *)malloc(sizeof(ContMemoryList_t));
        ptNewEntry->tPhysAddr        = tPhysAddr;
        ptNewEntry->pMemifoPtr       = ptMsysMem;
        CAM_OS_LIST_ADD_TAIL(&(ptNewEntry->tList), &_gtContMemList.tList);
        pthread_mutex_unlock(&_gtMemLock);
    } while (0);

    if (nMsysFd >= 0)
    {
        close(nMsysFd);
    }

    return tPhysAddr;
#else
    _CheckContMemInfoListInited();
#endif
#elif defined(CAM_OS_LINUX_KERNEL)
    MSYS_DMEM_INFO *  ptDmem     = NULL;
    ContMemoryList_t *ptNewEntry = NULL;
    ss_phys_addr_t    tPhysAddr  = 0;

    do
    {
        _CheckContMemInfoListInited();

        if (0 == (ptDmem = (MSYS_DMEM_INFO *)kzalloc(sizeof(MSYS_DMEM_INFO), in_interrupt() ? GFP_ATOMIC : GFP_KERNEL)))
        {
            printk(KERN_WARNING "%s alloc DMEM INFO fail\n", __FUNCTION__);
            eRet = CAM_OS_FAIL;
            break;
        }

        snprintf(ptDmem->name, 15, "CamOs%d", _gnContMemReqCnt++);
        ptDmem->length = nSize;

        if (0 != msys_request_dmem(ptDmem))
        {
            printk(KERN_WARNING "%s request dmem fail\n", __FUNCTION__);
            eRet = CAM_OS_FAIL;
            break;
        }

        tPhysAddr = ptDmem->phys;

        printk(KERN_INFO "%s physAddr<0x%08llX> size<%d>\n", __FUNCTION__, ptDmem->phys, ptDmem->length);

        mutex_lock(&_gtMemLock);
        if (0
            == (ptNewEntry =
                    (ContMemoryList_t *)kzalloc(sizeof(ContMemoryList_t), in_interrupt() ? GFP_ATOMIC : GFP_KERNEL)))
        {
            printk(KERN_WARNING "%s alloc entry fail\n", __FUNCTION__);
            eRet = CAM_OS_FAIL;
            break;
        }
        ptNewEntry->tPhysAddr  = tPhysAddr;
        ptNewEntry->pMemifoPtr = ptDmem;
        CAM_OS_LIST_ADD_TAIL(&(ptNewEntry->tList), &_gtContMemList.tList);
        mutex_unlock(&_gtMemLock);
    } while (0);

    if (eRet == CAM_OS_FAIL)
    {
        if (ptDmem)
        {
            if (ptDmem->phys)
            {
                msys_release_dmem(ptDmem);
            }
            kfree(ptDmem);
        }

        if (ptNewEntry)
        {
            kfree(ptNewEntry);
        }
    }

#if defined(CONFIG_TRACE_CAM_OS_MEM)
    _CamOSTraceInfoAlloc((unsigned long)tPhysAddr, nSize, CAM_OS_MEM_TRACE_CONTIGUOUS_ALLOC,
                         (unsigned long)__builtin_return_address(0));
#endif
    return tPhysAddr;
#endif
    return eRet;
}

CamOsRet_e CamOsContiguousMemRelease(ss_phys_addr_t tPhysAddr)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    struct CamOsListHead_t *ptPos, *ptQ;
    MemoryList_t *          ptTmp;

    if (tPhysAddr)
    {
        _CheckContMemInfoListInited();

        MsMutexLock(&_gtMemLock);
        CAM_OS_LIST_FOR_EACH_SAFE(ptPos, ptQ, &_gtMemList.tList)
        {
            ptTmp = CAM_OS_LIST_ENTRY(ptPos, MemoryList_t, tList);

            if (ptTmp->tPhysAddr == tPhysAddr)
            {
                if (ptTmp->szName)
                    MsReleaseMemory(ptTmp->szName);
                CAM_OS_LIST_DEL(ptPos);
                MsReleaseMemory(ptTmp);
            }
        }
        MsMutexUnlock(&_gtMemLock);

        MsReleaseMemory(tPhysAddr);
    }
#elif defined(CAM_OS_LINUX_USER)
#ifndef NO_MDRV_MSYS
    struct CamOsListHead_t *ptPos, *ptQ;
    ContMemoryList_t *      ptTmp;
    s32                     nMsysfd  = -1;
    MSYS_DMEM_INFO *        pMsysMem = NULL;

    if (tPhysAddr)
    {
        do
        {
            if (0 > (nMsysfd = open("/dev/msys", O_RDWR | O_SYNC)))
            {
                fprintf(stderr, "%s open /dev/msys failed!!\n", __FUNCTION__);
                eRet = CAM_OS_FAIL;
                break;
            }

            _CheckContMemInfoListInited();

            pthread_mutex_lock(&_gtMemLock);
            CAM_OS_LIST_FOR_EACH_SAFE(ptPos, ptQ, &_gtContMemList.tList)
            {
                ptTmp = CAM_OS_LIST_ENTRY(ptPos, ContMemoryList_t, tList);

                if (ptTmp->tPhysAddr == tPhysAddr)
                {
                    pMsysMem = ptTmp->pMemifoPtr;
                    break;
                }
            }
            pthread_mutex_unlock(&_gtMemLock);
            if (pMsysMem == NULL)
            {
                fprintf(stderr, "%s find Msys_DMEM_Info node failed!! <0x%08llX>\n", __FUNCTION__, tPhysAddr);
                eRet = CAM_OS_FAIL;
                break;
            }

            if (ioctl(nMsysfd, IOCTL_MSYS_RELEASE_DMEM, pMsysMem))
            {
                fprintf(stderr, "%s : IOCTL_MSYS_RELEASE_DMEM error physAddr<0x%llx>\n", __FUNCTION__, pMsysMem->phys);
                eRet = CAM_OS_FAIL;
                break;
            }
            if (pMsysMem)
            {
                free(pMsysMem);
                pMsysMem = NULL;
            }
            pthread_mutex_lock(&_gtMemLock);
            CAM_OS_LIST_FOR_EACH_SAFE(ptPos, ptQ, &_gtContMemList.tList)
            {
                ptTmp = CAM_OS_LIST_ENTRY(ptPos, ContMemoryList_t, tList);

                if (ptTmp->tPhysAddr == tPhysAddr)
                {
                    CAM_OS_LIST_DEL(ptPos);
                    free(ptTmp);
                }
            }
            pthread_mutex_unlock(&_gtMemLock);
        } while (0);

        if (nMsysfd >= 0)
        {
            close(nMsysfd);
        }
    }
#endif
#elif defined(CAM_OS_LINUX_KERNEL)
    MSYS_DMEM_INFO *        tpDmem = NULL;
    struct CamOsListHead_t *ptPos, *ptQ;
    ContMemoryList_t *      ptTmp = NULL;

    if (tPhysAddr)
    {
        do
        {
            _CheckContMemInfoListInited();

            mutex_lock(&_gtMemLock);
            CAM_OS_LIST_FOR_EACH_SAFE(ptPos, ptQ, &_gtContMemList.tList)
            {
                ptTmp = CAM_OS_LIST_ENTRY(ptPos, ContMemoryList_t, tList);

                // printk("search tmp->ptr: %p  %s\n", ptTmp->pVirtPtr, ptTmp->szName);

                if (ptTmp->tPhysAddr == tPhysAddr)
                {
                    tpDmem = ptTmp->pMemifoPtr;
                    // printk("search(2) pdmem->name: %s\n", tpDmem->name);
                    break;
                }
            }
            mutex_unlock(&_gtMemLock);
            if (tpDmem == NULL)
            {
                printk(KERN_WARNING "%s find node fail <%llx>\n", __FUNCTION__, tPhysAddr);
                eRet = CAM_OS_FAIL;
                break;
            }

            msys_release_dmem(tpDmem);

            if (tpDmem)
            {
                kfree(tpDmem);
                tpDmem = NULL;
            }
            mutex_lock(&_gtMemLock);
            CAM_OS_LIST_FOR_EACH_SAFE(ptPos, ptQ, &_gtContMemList.tList)
            {
                ptTmp = CAM_OS_LIST_ENTRY(ptPos, ContMemoryList_t, tList);

                if (ptTmp->tPhysAddr == tPhysAddr)
                {
                    CAM_OS_LIST_DEL(ptPos);
                    kfree(ptTmp);
                }
            }
            mutex_unlock(&_gtMemLock);
        } while (0);

#if defined(CONFIG_TRACE_CAM_OS_MEM)
        _CamOSTraceInfoFree((unsigned long)tPhysAddr, CAM_OS_MEM_TRACE_CONTIGUOUS_ALLOC);
#endif
    }
#endif
    return eRet;
}

CamOsRet_e CamOsContiguousMemStat(void)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    struct CamOsListHead_t *ptPos, *ptQ;
    MemoryList_t *          ptTmp;

    _CheckContMemInfoListInited();

    MsMutexLock(&_gtMemLock);
    CAM_OS_LIST_FOR_EACH_SAFE(ptPos, ptQ, &_gtMemList.tList)
    {
        ptTmp = CAM_OS_LIST_ENTRY(ptPos, MemoryList_t, tList);

        if (ptTmp->pVirtPtr)
        {
            CamOsPrintf("%s memory allocated %p %s\n", __FUNCTION__, ptTmp->pVirtPtr, ptTmp->szName);
        }
    }
    MsMutexUnlock(&_gtMemLock);
#elif defined(CAM_OS_LINUX_USER)
#ifndef NO_MDRV_MSYS
    struct CamOsListHead_t *ptPos, *ptQ;
    ContMemoryList_t *      ptTmp;

    _CheckContMemInfoListInited();

    pthread_mutex_lock(&_gtMemLock);
    CAM_OS_LIST_FOR_EACH_SAFE(ptPos, ptQ, &_gtContMemList.tList)
    {
        ptTmp = CAM_OS_LIST_ENTRY(ptPos, ContMemoryList_t, tList);

        if (ptTmp->tPhysAddr)
        {
            fprintf(stderr, "%s memory allocated %llu\n", __FUNCTION__, ptTmp->tPhysAddr);
        }
    }
    pthread_mutex_unlock(&_gtMemLock);
#endif
#elif defined(CAM_OS_LINUX_KERNEL)
    struct CamOsListHead_t *ptPos, *ptQ;
    ContMemoryList_t *      ptTmp;

    _CheckContMemInfoListInited();

    mutex_lock(&_gtMemLock);
    CAM_OS_LIST_FOR_EACH_SAFE(ptPos, ptQ, &_gtContMemList.tList)
    {
        ptTmp = CAM_OS_LIST_ENTRY(ptPos, ContMemoryList_t, tList);

        if (ptTmp->tPhysAddr)
        {
            printk(KERN_WARNING "%s allocated %llu\n", __FUNCTION__, ptTmp->tPhysAddr);
        }
    }
    mutex_unlock(&_gtMemLock);
#endif
    return eRet;
}

ss_miu_addr_t CamOsMemPhysToMiu(ss_phys_addr_t pPtr)
{
#ifdef CAM_OS_RTK
    return (void *)MDrv_MIU_PHY2MIUAddr((u32)pPtr);
#elif defined(CAM_OS_LINUX_USER)
    ss_miu_addr_t              nMiuAddr = 0;
#ifndef NO_MDRV_MSYS
    s32                        nMsysFd  = -1;
    MSYS_ADDR_TRANSLATION_INFO tAddrInfo;

    do
    {
        if (0 > (nMsysFd = open("/dev/msys", O_RDWR | O_SYNC)))
        {
            fprintf(stderr, "%s open /dev/msys failed!!\n", __FUNCTION__);
            break;
        }

        FILL_VERCHK_TYPE(tAddrInfo, tAddrInfo.VerChk_Version, tAddrInfo.VerChk_Size, IOCTL_MSYS_VERSION);

        tAddrInfo.addr = pPtr;
        if (ioctl(nMsysFd, IOCTL_MSYS_PHYS_TO_MIU, &tAddrInfo))
        {
            fprintf(stderr, "%s IOCTL_MSYS_PHYS_TO_MIU Failed!!\n", __FUNCTION__);
            break;
        }
        nMiuAddr = tAddrInfo.addr;
    } while (0);

    if (nMsysFd >= 0)
    {
        close(nMsysFd);
    }
#endif
    return nMiuAddr;
#elif defined(CAM_OS_LINUX_KERNEL)
    return Chip_Phys_to_MIU(pPtr);
#endif
}

ss_phys_addr_t CamOsMemMiuToPhys(ss_miu_addr_t pPtr)
{
#ifdef CAM_OS_RTK
    return (void *)MDrv_MIU_MIU2PHYAddr((u32)pPtr);
#elif defined(CAM_OS_LINUX_USER)
    ss_phys_addr_t             nPhysAddr = 0;
#ifndef NO_MDRV_MSYS
    s32                        nMsysFd   = -1;
    MSYS_ADDR_TRANSLATION_INFO tAddrInfo;

    do
    {
        if (0 > (nMsysFd = open("/dev/msys", O_RDWR | O_SYNC)))
        {
            fprintf(stderr, "%s open /dev/msys failed!!\n", __FUNCTION__);
            break;
        }

        FILL_VERCHK_TYPE(tAddrInfo, tAddrInfo.VerChk_Version, tAddrInfo.VerChk_Size, IOCTL_MSYS_VERSION);

        tAddrInfo.addr = pPtr;
        if (ioctl(nMsysFd, IOCTL_MSYS_MIU_TO_PHYS, &tAddrInfo))
        {
            fprintf(stderr, "%s IOCTL_MSYS_MIU_TO_PHYS Failed!!\n", __FUNCTION__);
            break;
        }
        nPhysAddr = tAddrInfo.addr;
    } while (0);

    if (nMsysFd >= 0)
    {
        close(nMsysFd);
    }
#endif
    return nPhysAddr;
#elif defined(CAM_OS_LINUX_KERNEL)
    return Chip_MIU_to_Phys(pPtr);
#endif
}

ss_phys_addr_t CamOsMemVirtToPhys(void *pPtr)
{
#ifdef CAM_OS_RTK
    return MsVA2PA(pPtr);
#elif defined(CAM_OS_LINUX_USER)
    // TODO: implement VirtToPhys in linux user space.
    CAM_OS_WARN("not support in " OS_NAME);
    return 0;
#elif defined(CAM_OS_LINUX_KERNEL)
    ss_phys_addr_t    pa   = 0;
    struct vm_struct *area = NULL;

    if (pPtr)
    {
        if (virt_addr_valid(pPtr))
        {
            pa = (ss_phys_addr_t)virt_to_phys(pPtr);
        }
        else
        {
            area = find_vm_area(pPtr + offset_in_page(pPtr));
            pa   = __pfn_to_phys(page_to_pfn(*area->pages)) + offset_in_page(pPtr);
        }
    }

    return pa;
#endif
}

void *CamOsMemMap(ss_phys_addr_t pPhyPtr, u32 nSize, u8 bCache)
{
#ifdef CAM_OS_RTK
    return MsMEMMap(pPhyPtr, nSize, bCache);
#elif defined(CAM_OS_LINUX_USER)
    void *pMmapPtr = NULL;
    s32   nMemFd   = -1;

    if (!bCache)
    {
        fprintf(stderr, "%s not support noncache mapping in user space!!\n", __FUNCTION__);
        return NULL;
    }

    if (0 > (nMemFd = open("/dev/mem", O_RDWR | O_SYNC)))
    {
        fprintf(stderr, "%s open /dev/mem failed!!\n", __FUNCTION__);
        return NULL;
    }

    pMmapPtr = mmap(0, nSize, PROT_READ | PROT_WRITE, MAP_SHARED, nMemFd, pPhyPtr);

    if (nMemFd >= 0)
        close(nMemFd);

    if (pMmapPtr == (void *)-1)
        return NULL;

    return pMmapPtr;
#elif defined(CAM_OS_LINUX_KERNEL)
    ss_phys_addr_t      nCpuBusAddr = 0;
    void *              pVirtPtr    = NULL;
    int                 nRet, i, j;
    struct sg_table *   pSgTable;
    struct scatterlist *pScatterList;
    int                 nPageCount = 0;
    int                 nPageTotal = 0;
    struct page **      ppPages;
    pgprot_t            tPgProt;

    if (!nSize)
        return NULL;

    if (bCache)
        tPgProt = PAGE_KERNEL;
    else
        tPgProt = pgprot_writecombine(PAGE_KERNEL);

    pSgTable = kmalloc(sizeof(struct sg_table), in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
    if (!pSgTable)
    {
        CAM_OS_WARN("kmalloc fail");
        return NULL;
    }

    nRet = sg_alloc_table(pSgTable, 1, in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);

    if (unlikely(nRet))
    {
        CAM_OS_WARN("sg_alloc_table fail");
        kfree(pSgTable);
        return NULL;
    }

    nCpuBusAddr = pPhyPtr;
    nSize       = CAM_OS_ALIGN_UP(nSize + offset_in_page(nCpuBusAddr), PAGE_SIZE);

    sg_set_page(pSgTable->sgl, pfn_to_page(__phys_to_pfn(nCpuBusAddr)), PAGE_ALIGN(nSize), 0);

    for_each_sg(pSgTable->sgl, pScatterList, pSgTable->nents, i)
    {
        nPageCount += pScatterList->length / PAGE_SIZE;
    }

    ppPages = vmalloc(sizeof(struct page *) * nPageCount);

    if (ppPages == NULL)
    {
        CAM_OS_WARN("vmalloc fail");
        sg_free_table(pSgTable);
        kfree(pSgTable);
        return NULL;
    }

    for_each_sg(pSgTable->sgl, pScatterList, pSgTable->nents, i)
    {
        nPageCount = PAGE_ALIGN(pScatterList->length) / PAGE_SIZE;
        for (j = 0; j < nPageCount; j++)
            ppPages[nPageTotal + j] = sg_page(pScatterList) + j;
        nPageTotal += nPageCount;
    }
    pVirtPtr = vmap(ppPages, nPageTotal, VM_MAP | VM_MAP_PUT_PAGES, tPgProt);

    sg_free_table(pSgTable);
    kfree(pSgTable);

    if (pVirtPtr == NULL)
    {
        CAM_OS_WARN("vmap fail");
        vfree(ppPages);
    }
    return (pVirtPtr) ? pVirtPtr + offset_in_page(pPhyPtr) : pVirtPtr;
#endif
}

void CamOsMemUnmap(void *pVirtPtr, u32 nSize)
{
#ifdef CAM_OS_RTK
    MsMEMUnmap(pVirtPtr, nSize);
#elif defined(CAM_OS_LINUX_USER)
    s32 nErr;

    nErr = munmap(pVirtPtr, nSize);
    if (0 != nErr)
    {
        fprintf(stderr, "%s munmap failed!! <%p> size<%d> err<%d> errno<%d, %s>\n", __FUNCTION__, pVirtPtr, nSize, nErr,
                errno, strerror(errno));
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    struct vm_struct *area = NULL;

    area = find_vm_area(pVirtPtr + offset_in_page(pVirtPtr));
    vfree(area->pages);
    vunmap(pVirtPtr - offset_in_page(pVirtPtr));
#endif
}

void *CamOsMemFromUserModeMap(ss_phys_addr_t pPhyPtr, u32 nSize, u8 bCache)
{
#ifdef CAM_OS_RTK
    return MsMEMMap(pPhyPtr, nSize, bCache);
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
    return 0;
#elif defined(CAM_OS_LINUX_KERNEL)
    s32                    nErr;
    unsigned long          populate;
    unsigned long          vm_len;
    struct vm_area_struct *vma;
    unsigned long          nVirtualAddress = 0;

    nSize = CAM_OS_ALIGN_UP(nSize + offset_in_page(pPhyPtr), PAGE_SIZE);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
    down_write(&current->mm->mmap_lock);
#else
    down_write(&current->mm->mmap_sem);
#endif
    nVirtualAddress =
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
        do_mmap(
#else
        do_mmap_pgoff(
#endif
            NULL, 0, nSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, 0, &populate
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
            ,
            NULL
#endif
        );

    if (IS_ERR((void *)nVirtualAddress))
    {
        nVirtualAddress = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
        up_write(&current->mm->mmap_lock);
#else
        up_write(&current->mm->mmap_sem);
#endif
        printk("%s: anon_inode_getfile failed\n", __FUNCTION__);
        return NULL;
    }
    if (populate)
        mm_populate(nVirtualAddress, populate);

    vma    = find_vma(current->mm, nVirtualAddress);
    vm_len = vma->vm_end - vma->vm_start;

    if (vm_len != nSize || nVirtualAddress != vma->vm_start)
    {
        goto FAIL_UNMAP;
    }

    vma->vm_pgoff = 0;
    if (bCache)
        vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);
    else
        vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

    nErr = io_remap_pfn_range(vma, vma->vm_start, __phys_to_pfn(pPhyPtr), vm_len, vma->vm_page_prot);
    if (nErr)
    {
        goto FAIL_UNMAP;
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
    up_write(&current->mm->mmap_lock);
#else
    up_write(&current->mm->mmap_sem);
#endif
    return (void *)(vma->vm_start + offset_in_page(pPhyPtr));

FAIL_UNMAP:
    do_munmap(current->mm, nVirtualAddress, nSize
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
              ,
              NULL
#endif
    );

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
    up_write(&current->mm->mmap_lock);
#else
    up_write(&current->mm->mmap_sem);
#endif
    printk("%s:%d failed, vm_len:%lu nSize:%u nVirtualAddress:%lx, vm_start:%lx\n", __FUNCTION__, __LINE__, vm_len,
           nSize, nVirtualAddress, vma->vm_start);
    return NULL;
#endif
}

void CamOsMemFromUserModeUnmap(void *pVirtPtr, u32 nSize)
{
#ifdef CAM_OS_RTK
    MsMEMUnmap(pVirtPtr, nSize);
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
    return;
#elif defined(CAM_OS_LINUX_KERNEL)
    s32           nErr;
    unsigned long nVirAddr = (unsigned long)pVirtPtr;
    unsigned long offset   = offset_in_page(pVirtPtr);

    nSize = CAM_OS_ALIGN_UP(nSize + offset, PAGE_SIZE);
    nVirAddr -= offset;

    if (pVirtPtr == NULL || nSize == 0)
    {
        printk("%s: pVirtPtr=%p nSize=%d\n", __FUNCTION__, pVirtPtr, nSize);
        return;
    }

    if (current->mm) // if current->mm == NULL ,user app already exit
    {
        struct vm_area_struct *vma;
        unsigned long          vm_len;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
        down_write(&current->mm->mmap_lock);
#else
        down_write(&current->mm->mmap_sem);
#endif
        vma = find_vma(current->mm, nVirAddr);

        if (!vma)
        {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
            up_write(&current->mm->mmap_lock);
#else
            up_write(&current->mm->mmap_sem);
#endif
            return;
        }

        vm_len = vma->vm_end - vma->vm_start;

        if (vm_len != nSize || nVirAddr != vma->vm_start)
        {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
            up_write(&current->mm->mmap_lock);
#else
            up_write(&current->mm->mmap_sem);
#endif
            return;
        }

        nErr = do_munmap(current->mm, nVirAddr, nSize
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
                         ,
                         NULL
#endif
        );
        if (nErr)
        {
            printk("%s: do_munmap failed:%d\n", __FUNCTION__, nErr);
        }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
        up_write(&current->mm->mmap_lock);
#else
        up_write(&current->mm->mmap_sem);
#endif
    }

    return;
#endif
}

CamOsRet_e CamOsMemIsUserSpace(void *pVirtPtr)
{
#ifdef CAM_OS_RTK
    return CAM_OS_FAIL;
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
    return CAM_OS_FAIL;
#elif defined(CAM_OS_LINUX_KERNEL)
    return (pVirtPtr < (void *)TASK_SIZE) ? CAM_OS_OK : CAM_OS_FAIL;
#endif
}

CamOsRet_e CamOsMemCacheCreate(CamOsMemCache_t *ptMemCache, char *szName, u32 nSize, u8 bHwCacheAlign)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsMemCacheRtk_t *ptHandle = (CamOsMemCacheRtk_t *)ptMemCache;

    if (ptHandle)
    {
        if (RTOSIsCustPoolCreated())
        {
            if (MS_OK == MsFindBestPool(nSize, &ptHandle->nPoolID))
            {
                ptHandle->nObjSize = nSize;
            }
            else
            {
                CAM_OS_WARN("no satisfactory mem pool");
                eRet = CAM_OS_FAIL;
            }
        }
        else
        {
            // Use heap
            ptHandle->nObjSize = nSize;
        }
    }
    else
        eRet = CAM_OS_PARAM_ERR;
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
    eRet = CAM_OS_FAIL;
#elif defined(CAM_OS_LINUX_KERNEL)
    struct kmem_cache *ptKmemCache;
    CamOsMemCacheLK_t *ptHandle = (CamOsMemCacheLK_t *)ptMemCache;

    if (ptHandle)
    {
        strncpy(ptHandle->szName, szName, sizeof(ptHandle->szName) - 1);
        ptHandle->szName[sizeof(ptHandle->szName) - 1] = '\0';
        ptKmemCache = kmem_cache_create(ptHandle->szName, nSize, 0, bHwCacheAlign ? SLAB_HWCACHE_ALIGN : 0, NULL);

        if (ptKmemCache)
        {
            ptHandle->ptKmemCache = ptKmemCache;
        }
        else
        {
            ptHandle->ptKmemCache = NULL;
            eRet                  = CAM_OS_FAIL;
        }
    }
    else
        eRet = CAM_OS_PARAM_ERR;
#endif
    return eRet;
}

void CamOsMemCacheDestroy(CamOsMemCache_t *ptMemCache)
{
#ifdef CAM_OS_RTK
    CamOsMemCacheRtk_t *ptHandle = (CamOsMemCacheRtk_t *)ptMemCache;

    if (ptHandle)
        memset(ptHandle, 0, sizeof(CamOsMemCacheRtk_t));
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsMemCacheLK_t *ptHandle = (CamOsMemCacheLK_t *)ptMemCache;

    if (ptHandle && ptHandle->ptKmemCache)
    {
        kmem_cache_destroy(ptHandle->ptKmemCache);
        ptHandle->ptKmemCache = NULL;
    }
#endif
}

void *CamOsMemCacheAlloc(CamOsMemCache_t *ptMemCache)
{
#ifdef CAM_OS_RTK
    CamOsMemCacheRtk_t *ptHandle = (CamOsMemCacheRtk_t *)ptMemCache;

    if (ptHandle)
    {
        if (RTOSIsCustPoolCreated())
            return MsGetPoolMemory(ptHandle->nObjSize);
        else
            return MsGetCHeapMemory(ptHandle->nObjSize);
    }
    else
        return NULL;
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
    return NULL;
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsMemCacheLK_t *ptHandle = (CamOsMemCacheLK_t *)ptMemCache;

    if (ptHandle && ptHandle->ptKmemCache)
    {
        void *pPtr = NULL;
        pPtr       = kmem_cache_alloc(ptHandle->ptKmemCache, in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
#if defined(CONFIG_TRACE_CAM_OS_MEM)
        _CamOSTraceInfoAlloc((unsigned long)pPtr, ptHandle->ptKmemCache->size, CAM_OS_MEM_TRACE_CACHE_ALLOC,
                             (unsigned long)__builtin_return_address(0));
#endif
        return pPtr;
    }
    else
        return NULL;
#endif
}

void *CamOsMemCacheAllocAtomic(CamOsMemCache_t *ptMemCache)
{
#ifdef CAM_OS_RTK
    CamOsMemCacheRtk_t *ptHandle = (CamOsMemCacheRtk_t *)ptMemCache;

    if (ptHandle)
    {
        if (RTOSIsCustPoolCreated())
            return MsGetPoolMemory(ptHandle->nObjSize);
        else
            return MsGetCHeapMemory(ptHandle->nObjSize);
    }
    else
        return NULL;
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
    return NULL;
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsMemCacheLK_t *ptHandle = (CamOsMemCacheLK_t *)ptMemCache;

    if (ptHandle && ptHandle->ptKmemCache)
    {
        void *pPtr = NULL;
        pPtr       = kmem_cache_alloc(ptHandle->ptKmemCache, GFP_ATOMIC);
#if defined(CONFIG_TRACE_CAM_OS_MEM)
        _CamOSTraceInfoAlloc((unsigned long)pPtr, ptHandle->ptKmemCache->size, CAM_OS_MEM_TRACE_CACHE_ALLOC_ATOMIC,
                             (unsigned long)__builtin_return_address(0));
#endif
        return pPtr;
    }
    else
        return NULL;
#endif
}

void CamOsMemCacheFree(CamOsMemCache_t *ptMemCache, void *pObjPtr)
{
#ifdef CAM_OS_RTK
    MsReleaseMemory(pObjPtr);
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsMemCacheLK_t *ptHandle = (CamOsMemCacheLK_t *)ptMemCache;

    if (ptHandle && ptHandle->ptKmemCache)
    {
#if defined(CONFIG_TRACE_CAM_OS_MEM)
        CamOsMemTrace_e eMemType = 0;
        for (eMemType = CAM_OS_MEM_TRACE_CACHE_ALLOC; eMemType <= CAM_OS_MEM_TRACE_CACHE_ALLOC_ATOMIC; eMemType++)
        {
            _CamOSTraceInfoFree((unsigned long)pObjPtr, eMemType);
        }
#endif
        kmem_cache_free(ptHandle->ptKmemCache, pObjPtr);
    }
#endif
}

u64 CamOsMathDivU64(u64 nDividend, u64 nDivisor, u64 *pRemainder)
{
#ifdef CAM_OS_RTK
    return (pRemainder) ? ss_div64_u64_rem(nDividend, nDivisor, pRemainder) : ss_div64_u64(nDividend, nDivisor);
#elif defined(CAM_OS_LINUX_USER)
    if (pRemainder)
        *pRemainder = nDividend % nDivisor;
    return nDividend / nDivisor;
#elif defined(CAM_OS_LINUX_KERNEL)
    return (pRemainder) ? div64_u64_rem(nDividend, nDivisor, pRemainder) : div64_u64(nDividend, nDivisor);
#endif
}

s64 CamOsMathDivS64(s64 nDividend, s64 nDivisor, s64 *pRemainder)
{
#ifdef CAM_OS_RTK
    s64 nQuotient = ss_div64_s64(nDividend, nDivisor);
    if (pRemainder)
        *pRemainder = nDividend - nDivisor * nQuotient;
    return nQuotient;
#elif defined(CAM_OS_LINUX_USER)
    if (pRemainder)
        *pRemainder = nDividend % nDivisor;
    return nDividend / nDivisor;
#elif defined(CAM_OS_LINUX_KERNEL)
    s64 nQuotient = div64_s64(nDividend, nDivisor);
    if (pRemainder)
        *pRemainder = nDividend - nDivisor * nQuotient;
    return nQuotient;
#endif
}

u32 CamOsCopyFromUpperLayer(void *pTo, const void *pFrom, u32 nLen)
{
#ifdef CAM_OS_RTK
    memcpy(pTo, pFrom, nLen);
    return 0;
#elif defined(CAM_OS_LINUX_USER)
    memcpy(pTo, pFrom, nLen);
    return 0;
#elif defined(CAM_OS_LINUX_KERNEL)
    return copy_from_user(pTo, pFrom, nLen);
#endif
}

u32 CamOsCopyToUpperLayer(void *pTo, const void *pFrom, u32 nLen)
{
#ifdef CAM_OS_RTK
    memcpy(pTo, pFrom, nLen);
    return 0;
#elif defined(CAM_OS_LINUX_USER)
    memcpy(pTo, pFrom, nLen);
    return 0;
#elif defined(CAM_OS_LINUX_KERNEL)
    return copy_to_user(pTo, pFrom, nLen);
#endif
}

#if defined(CAM_OS_LINUX_KERNEL)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
void _CamOsTimerCallbackLK(struct timer_list *t)
{
    CamOsTimerLK_t *ptHandle = from_timer(ptHandle, t, tTimerID);

    if (ptHandle->pfnCallback)
    {
        ptHandle->pfnCallback(ptHandle->pDataPtr);
    }
}
#endif
#endif

CamOsRet_e CamOsTimerInit(CamOsTimer_t *ptTimer)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    if (ptTimer)
    {
        MsTimerExtInit((MsTimerExt_t *)ptTimer);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    // TODO: implement timer in linux user space.
    CAM_OS_WARN("not support in " OS_NAME);
    eRet = CAM_OS_FAIL;
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsTimerLK_t *ptHandle = (CamOsTimerLK_t *)ptTimer;
    if (ptHandle)
    {
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
        init_timer(&ptHandle->tTimerID);
#else
        timer_setup(&ptHandle->tTimerID, _CamOsTimerCallbackLK, 0);
#endif
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

u32 CamOsTimerDelete(CamOsTimer_t *ptTimer)
{
#ifdef CAM_OS_RTK
    if (ptTimer)
    {
        return MsTimerExtDelete((MsTimerExt_t *)ptTimer);
    }
#elif defined(CAM_OS_LINUX_USER)
    // TODO: implement timer in linux user space.
    CAM_OS_WARN("not support in " OS_NAME);
    return 0;
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsTimerLK_t *ptHandle = (CamOsTimerLK_t *)ptTimer;
    if (ptHandle)
    {
        return del_timer(&ptHandle->tTimerID);
    }
#endif
    return 0;
}

u32 CamOsTimerDeleteSync(CamOsTimer_t *ptTimer)
{
#ifdef CAM_OS_RTK
    if (ptTimer)
    {
        return MsTimerExtDelete((MsTimerExt_t *)ptTimer);
    }
#elif defined(CAM_OS_LINUX_USER)
    // TODO: implement timer in linux user space.
    CAM_OS_WARN("not support in " OS_NAME);
    return 0;
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsTimerLK_t *ptHandle = (CamOsTimerLK_t *)ptTimer;
    if (ptHandle)
    {
        return del_timer_sync(&ptHandle->tTimerID);
    }
#endif
    return 0;
}

CamOsRet_e CamOsTimerAdd(CamOsTimer_t *ptTimer, u32 nMsec, void *pDataPtr, CamOsTimerCallback pfnFunc)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    if (ptTimer)
    {
        MsTimerExtAdd((MsTimerExt_t *)ptTimer, nMsec, pDataPtr, pfnFunc);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    // TODO: implement timer in linux user space.
    CAM_OS_WARN("not support in " OS_NAME);
    eRet = CAM_OS_FAIL;
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsTimerLK_t *ptHandle = (CamOsTimerLK_t *)ptTimer;
    if (ptHandle)
    {
        ptHandle->tTimerID.expires  = jiffies + msecs_to_jiffies(nMsec);
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
        ptHandle->tTimerID.function = pfnFunc;
        ptHandle->tTimerID.data     = (unsigned long)pDataPtr;
#else
        ptHandle->pfnCallback = pfnFunc;
        ptHandle->pDataPtr    = pDataPtr;
#endif
        add_timer(&ptHandle->tTimerID);
    }
    else
        eRet = CAM_OS_PARAM_ERR;
#endif
    return eRet;
}

CamOsRet_e CamOsTimerModify(CamOsTimer_t *ptTimer, u32 nMsec)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    if (ptTimer)
    {
        MsTimerExtModify((MsTimerExt_t *)ptTimer, nMsec);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    // TODO: implement timer in linux user space.
    CAM_OS_WARN("not support in " OS_NAME);
    eRet                   = CAM_OS_FAIL;
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsTimerLK_t *ptHandle = (CamOsTimerLK_t *)ptTimer;
    if (ptHandle)
    {
        mod_timer(&ptHandle->tTimerID, jiffies + msecs_to_jiffies(nMsec));
    }
    else
        eRet = CAM_OS_PARAM_ERR;
#endif
    return eRet;
}

s32 CamOsAtomicRead(CamOsAtomic_t *ptAtomic)
{
    return ptAtomic->nCounter;
}

void CamOsAtomicSet(CamOsAtomic_t *ptAtomic, s32 nValue)
{
    ptAtomic->nCounter = nValue;
}

s32 CamOsAtomicAddReturn(CamOsAtomic_t *ptAtomic, s32 nValue)
{
    return __sync_add_and_fetch(&ptAtomic->nCounter, nValue);
}

s32 CamOsAtomicSubReturn(CamOsAtomic_t *ptAtomic, s32 nValue)
{
    return __sync_sub_and_fetch(&ptAtomic->nCounter, nValue);
}

s32 CamOsAtomicSubAndTest(CamOsAtomic_t *ptAtomic, s32 nValue)
{
    return !(__sync_sub_and_fetch(&ptAtomic->nCounter, nValue));
}

s32 CamOsAtomicIncReturn(CamOsAtomic_t *ptAtomic)
{
    return __sync_add_and_fetch(&ptAtomic->nCounter, 1);
}

s32 CamOsAtomicDecReturn(CamOsAtomic_t *ptAtomic)
{
    return __sync_sub_and_fetch(&ptAtomic->nCounter, 1);
}

s32 CamOsAtomicIncAndTest(CamOsAtomic_t *ptAtomic)
{
    return !(__sync_add_and_fetch(&ptAtomic->nCounter, 1));
}

s32 CamOsAtomicDecAndTest(CamOsAtomic_t *ptAtomic)
{
    return !(__sync_sub_and_fetch(&ptAtomic->nCounter, 1));
}

s32 CamOsAtomicAddNegative(CamOsAtomic_t *ptAtomic, s32 nValue)
{
    return (__sync_add_and_fetch(&ptAtomic->nCounter, nValue) < 0);
}

s32 CamOsAtomicCompareAndSwap(CamOsAtomic_t *ptAtomic, s32 nCmpVal, s32 nExchVal)
{
    return __sync_val_compare_and_swap(&ptAtomic->nCounter, nCmpVal, nExchVal);
}

s32 CamOsAtomicAndFetch(CamOsAtomic_t *ptAtomic, s32 nValue)
{
    return __sync_and_and_fetch(&ptAtomic->nCounter, nValue);
}

s32 CamOsAtomicFetchAnd(CamOsAtomic_t *ptAtomic, s32 nValue)
{
    return __sync_fetch_and_and(&ptAtomic->nCounter, nValue);
}

s32 CamOsAtomicNandFetch(CamOsAtomic_t *ptAtomic, s32 nValue)
{
    return __sync_nand_and_fetch(&ptAtomic->nCounter, nValue);
}

s32 CamOsAtomicFetchNand(CamOsAtomic_t *ptAtomic, s32 nValue)
{
    return __sync_fetch_and_nand(&ptAtomic->nCounter, nValue);
}

s32 CamOsAtomicOrFetch(CamOsAtomic_t *ptAtomic, s32 nValue)
{
    return __sync_or_and_fetch(&ptAtomic->nCounter, nValue);
}

s32 CamOsAtomicFetchOr(CamOsAtomic_t *ptAtomic, s32 nValue)
{
    return __sync_fetch_and_or(&ptAtomic->nCounter, nValue);
}

s32 CamOsAtomicXorFetch(CamOsAtomic_t *ptAtomic, s32 nValue)
{
    return __sync_xor_and_fetch(&ptAtomic->nCounter, nValue);
}

s32 CamOsAtomicFetchXor(CamOsAtomic_t *ptAtomic, s32 nValue)
{
    return __sync_fetch_and_xor(&ptAtomic->nCounter, nValue);
}

s64 CamOsAtomic64Read(CamOsAtomic64_t *ptAtomic64)
{
    return ptAtomic64->nCounter;
}

void CamOsAtomic64Set(CamOsAtomic64_t *ptAtomic64, s64 nValue)
{
    ptAtomic64->nCounter = nValue;
}

s64 CamOsAtomic64AddReturn(CamOsAtomic64_t *ptAtomic64, s64 nValue)
{
    return __sync_add_and_fetch(&ptAtomic64->nCounter, nValue);
}

s64 CamOsAtomic64SubReturn(CamOsAtomic64_t *ptAtomic64, s64 nValue)
{
    return __sync_sub_and_fetch(&ptAtomic64->nCounter, nValue);
}

s64 CamOsAtomic64SubAndTest(CamOsAtomic64_t *ptAtomic64, s64 nValue)
{
    return !(__sync_sub_and_fetch(&ptAtomic64->nCounter, nValue));
}

s64 CamOsAtomic64IncReturn(CamOsAtomic64_t *ptAtomic64)
{
    return __sync_add_and_fetch(&ptAtomic64->nCounter, 1);
}

s64 CamOsAtomic64DecReturn(CamOsAtomic64_t *ptAtomic64)
{
    return __sync_sub_and_fetch(&ptAtomic64->nCounter, 1);
}

s64 CamOsAtomic64IncAndTest(CamOsAtomic64_t *ptAtomic64)
{
    return !(__sync_add_and_fetch(&ptAtomic64->nCounter, 1));
}

s64 CamOsAtomic64DecAndTest(CamOsAtomic64_t *ptAtomic64)
{
    return !(__sync_sub_and_fetch(&ptAtomic64->nCounter, 1));
}

s64 CamOsAtomic64AddNegative(CamOsAtomic64_t *ptAtomic64, s64 nValue)
{
    return (__sync_add_and_fetch(&ptAtomic64->nCounter, nValue) < 0);
}

s64 CamOsAtomic64CompareAndSwap(CamOsAtomic64_t *ptAtomic64, s64 nCmpVal, s64 nExchVal)
{
    return __sync_val_compare_and_swap(&ptAtomic64->nCounter, nCmpVal, nExchVal);
}

s64 CamOsAtomic64AndFetch(CamOsAtomic64_t *ptAtomic64, s64 nValue)
{
    return __sync_and_and_fetch(&ptAtomic64->nCounter, nValue);
}

s64 CamOsAtomic64FetchAnd(CamOsAtomic64_t *ptAtomic64, s64 nValue)
{
    return __sync_fetch_and_and(&ptAtomic64->nCounter, nValue);
}

s64 CamOsAtomic64NandFetch(CamOsAtomic64_t *ptAtomic64, s64 nValue)
{
    return __sync_nand_and_fetch(&ptAtomic64->nCounter, nValue);
}

s64 CamOsAtomic64FetchNand(CamOsAtomic64_t *ptAtomic64, s64 nValue)
{
    return __sync_fetch_and_nand(&ptAtomic64->nCounter, nValue);
}

s64 CamOsAtomic64OrFetch(CamOsAtomic64_t *ptAtomic64, s64 nValue)
{
    return __sync_or_and_fetch(&ptAtomic64->nCounter, nValue);
}

s64 CamOsAtomic64FetchOr(CamOsAtomic64_t *ptAtomic64, s64 nValue)
{
    return __sync_fetch_and_or(&ptAtomic64->nCounter, nValue);
}

s64 CamOsAtomic64XorFetch(CamOsAtomic64_t *ptAtomic64, s64 nValue)
{
    return __sync_xor_and_fetch(&ptAtomic64->nCounter, nValue);
}

s64 CamOsAtomic64FetchXor(CamOsAtomic64_t *ptAtomic64, s64 nValue)
{
    return __sync_fetch_and_xor(&ptAtomic64->nCounter, nValue);
}

#if defined(CAM_OS_RTK) || defined(CAM_OS_LINUX_USER)
extern CamOsRet_e _CamOsIdrInit(CamOsIdr_t *ptIdr, u32 nEntryNum);
extern void       _CamOsIdrDestroy(CamOsIdr_t *ptIdr);
extern s32        _CamOsIdrAlloc(CamOsIdr_t *ptIdr, void *pPtr, s32 nStart, s32 nEnd);
extern void       _CamOsIdrRemove(CamOsIdr_t *ptIdr, s32 nId);
extern void *     _CamOsIdrFind(CamOsIdr_t *ptIdr, s32 nId);
#endif

CamOsRet_e CamOsIdrInit(CamOsIdr_t *ptIdr)
{
    CamOsRet_e eRet = CAM_OS_OK;
#if defined(CAM_OS_RTK) || defined(CAM_OS_LINUX_USER)
    eRet = _CamOsIdrInit(ptIdr, 0);
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsIdrLK_t *ptHandle = (CamOsIdrLK_t *)ptIdr;

    if (ptHandle)
    {
        idr_init(&ptHandle->tIdr);
    }
    else
        eRet = CAM_OS_PARAM_ERR;
#endif
    return eRet;
}

CamOsRet_e CamOsIdrInitEx(CamOsIdr_t *ptIdr, u32 nEntryNum)
{
    CamOsRet_e eRet = CAM_OS_OK;
#if defined(CAM_OS_RTK) || defined(CAM_OS_LINUX_USER)
    eRet = _CamOsIdrInit(ptIdr, nEntryNum);
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsIdrLK_t *ptHandle = (CamOsIdrLK_t *)ptIdr;

    if (ptHandle)
    {
        idr_init(&ptHandle->tIdr);
    }
    else
        eRet = CAM_OS_PARAM_ERR;
#endif
    return eRet;
}

void CamOsIdrDestroy(CamOsIdr_t *ptIdr)
{
#if defined(CAM_OS_RTK) || defined(CAM_OS_LINUX_USER)
    _CamOsIdrDestroy(ptIdr);
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsIdrLK_t *ptHandle = (CamOsIdrLK_t *)ptIdr;

    if (ptHandle)
    {
        idr_destroy(&ptHandle->tIdr);
    }
#endif
}

s32 CamOsIdrAlloc(CamOsIdr_t *ptIdr, void *pPtr, s32 nStart, s32 nEnd)
{
#if defined(CAM_OS_RTK) || defined(CAM_OS_LINUX_USER)
    return _CamOsIdrAlloc(ptIdr, pPtr, nStart, nEnd);
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsIdrLK_t *ptHandle = (CamOsIdrLK_t *)ptIdr;

    if (ptHandle)
    {
        return idr_alloc(&ptHandle->tIdr, pPtr, nStart, nEnd, in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
    }
    else
        return -1;
#endif
}

void CamOsIdrRemove(CamOsIdr_t *ptIdr, s32 nId)
{
#if defined(CAM_OS_RTK) || defined(CAM_OS_LINUX_USER)
    _CamOsIdrRemove(ptIdr, nId);
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsIdrLK_t *ptHandle = (CamOsIdrLK_t *)ptIdr;

    if (ptHandle)
    {
        idr_remove(&ptHandle->tIdr, nId);
    }
#endif
}

void *CamOsIdrFind(CamOsIdr_t *ptIdr, s32 nId)
{
#if defined(CAM_OS_RTK) || defined(CAM_OS_LINUX_USER)
    return _CamOsIdrFind(ptIdr, nId);
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsIdrLK_t *ptHandle = (CamOsIdrLK_t *)ptIdr;

    if (ptHandle)
    {
        return idr_find(&ptHandle->tIdr, nId);
    }
    else
        return NULL;
#endif
}

CamOsMemSize_e CamOsPhysMemSize(void)
{
    CamOsDramInfo_t tInfo      = {0};
    u32             i          = 0;
    u32             nMegaBytes = 0;

    CamOsDramInfo(&tInfo);
    nMegaBytes = tInfo.nBytes >> 20;
    while (nMegaBytes >>= 1)
        ++i;

    return i;
}

CamOsRet_e CamOsDramInfo(CamOsDramInfo_t *ptInfo)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    if (ptInfo)
        sys_GetDramInfo(&ptInfo->nBytes, &ptInfo->nType, &ptInfo->nBusWidth);
    else
        eRet = CAM_OS_PARAM_ERR;
#elif defined(CAM_OS_LINUX_USER)
    int  fd;
    char buf[512];
    int  dram_freq, miupll_freq;

    if (ptInfo)
    {
        fd = open("/sys/devices/system/miu/miu_bw0/dram_info", O_RDONLY);
        if (fd >= 0)
        {
            read(fd, buf, sizeof(buf));
            sscanf(buf,
                   "DRAM Type:   DDR%hd\n"
                   "DRAM Size:   %lldMB\n"
                   "DRAM Freq:   %dMHz\n"
                   "MIUPLL Freq: %dMHz\n"
                   "Bus Width:   %hdbit",
                   &ptInfo->nType, &ptInfo->nBytes, &dram_freq, &miupll_freq, &ptInfo->nBusWidth);
            close(fd);

            ptInfo->nBytes = (ptInfo->nBytes << 20);
        }
        else
        {
            eRet = CAM_OS_FAIL;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    MIU_DramInfo stDramInfo = {0};
    MDrv_MIU_Info(&stDramInfo);
    ptInfo->nBytes    = stDramInfo.size;
    ptInfo->nType     = stDramInfo.type;
    ptInfo->nBusWidth = stDramInfo.bus_width;
#endif
    return eRet;
}

u32 CamOsChipId(void)
{
#ifdef CAM_OS_RTK
    return sys_GetChipId();
#elif defined(CAM_OS_LINUX_USER)
    int  fd;
    char buf[32];
    u32  id = 0;

    fd = open("/sys/devices/virtual/mstar/msys/CHIP_ID", O_RDONLY);
    if (fd >= 0)
    {
        read(fd, buf, sizeof(buf));
        sscanf(buf, "Chip_ID: 0x%X", &id);
        close(fd);
    }

    return id;
#elif defined(CAM_OS_LINUX_KERNEL)
    return Chip_Get_Device_ID();
#endif
}

u32 CamOsChipRevision(void)
{
#ifdef CAM_OS_RTK
    return sys_GetChipRevisionId();
#elif defined(CAM_OS_LINUX_USER)
    int  fd;
    char buf[32];
    u32  rev = 0;

    fd = open("/sys/devices/virtual/mstar/msys/CHIP_VERSION", O_RDONLY);
    if (fd >= 0)
    {
        read(fd, buf, sizeof(buf));
        sscanf(buf, "Chip_Version: %u", &rev);
        rev += 1; // 1:U01, 2:U02 ...
        close(fd);
    }

    return rev;
#elif defined(CAM_OS_LINUX_KERNEL)
    return Chip_Get_Revision();
#endif
}

CamOsRet_e CamOsGetCpuOnline(CamOsCpuMask_t *ptMask)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    u32 *mask = (u32 *)ptMask;
    if (ptMask)
    {
        *mask = 0x1;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
    eRet = CAM_OS_FAIL;
#elif defined(CAM_OS_LINUX_KERNEL)
    if (ptMask)
    {
        memcpy(ptMask, cpu_online_mask, sizeof(CamOsCpuMask_t));
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsIrqRequest(u32 nIrq, CamOsIrqHandler pfnHandler, const char *szName, void *pDevId)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    IntInitParam_u uInitParam = {{0}};
    uInitParam.intc.eMap      = INTC_MAP_IRQ;
    uInitParam.intc.ePriority = INTC_PRIORITY_7;
    uInitParam.intc.pfnIsr    = pfnHandler;
    uInitParam.intc.pDevId    = pDevId;
    DrvInitInterrupt(&uInitParam, nIrq);
    DrvUnmaskInterrupt(nIrq);
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
#elif defined(CAM_OS_LINUX_KERNEL)
    struct CamOsHListNode_t *n              = NULL;
    CamOsIsrLK_t *           pHashListNode  = NULL;
    CamOsIsrLK_t *           pHashListNode2 = NULL;
    s32                      nResult        = 0;

    if ((nIrq < LINUX_KERNEL_MAX_IRQ) && (pDevId != NULL))
    {
        mutex_lock(&_gtIsrLock);
        CAM_OS_HASH_FOR_EACH_POSSIBLE_SAFE(interrupt_hashtbl, pHashListNode2, n, tEntry, nIrq)
        {
            if (pHashListNode2->nIntNum == nIrq)
            {
                if ((pHashListNode2->pfnIsr == pfnHandler) && (pHashListNode2->pDevId == pDevId))
                {
                    CamOsPrintf("%s irq(%d) (%s) is already registered\n", __FUNCTION__, nIrq, szName);
                    eRet = CAM_OS_FAIL;
                    goto READY_REGISTER;
                }
            }
        }

        if ((nResult =
                 request_irq(nIrq, (irq_handler_t)CamOsIrqCommonHandler, IRQF_SHARED | IRQF_ONESHOT, szName, pDevId)))
        {
            CamOsPanic("request_irq error in CamOsIrqRequest\r\n");
            eRet = CAM_OS_FAIL;
            goto READY_REGISTER;
        }

        pHashListNode = (CamOsIsrLK_t *)CamOsMemCalloc(1, sizeof(CamOsIsrLK_t));
        if (pHashListNode == NULL)
        {
            CamOsPanic("CamOsMemCalloc error in CamOsIrqRequest\r\n");
            eRet = CAM_OS_FAIL;
            goto READY_REGISTER;
        }
        pHashListNode->nIntNum = nIrq;
        pHashListNode->pfnIsr  = pfnHandler;
        pHashListNode->pDevId  = pDevId;
        CAM_OS_HASH_ADD(interrupt_hashtbl, &pHashListNode->tEntry, pHashListNode->nIntNum);
    READY_REGISTER:
        mutex_unlock(&_gtIsrLock);
    }
    else
    {
        eRet = CAM_OS_FAIL;
    }
#endif
    return eRet;
}

CamOsRet_e CamOsIrqSetAffinityHint(u32 nIrq, const CamOsCpuMask_t *ptMask)
{
    CamOsRet_e eRet = CAM_OS_FAIL;
#ifdef CAM_OS_RTK
    CAM_OS_WARN("not support in " OS_NAME);
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
#elif defined(CAM_OS_LINUX_KERNEL)
    if (!irq_set_affinity_hint(nIrq, (struct cpumask *)ptMask))
        eRet = CAM_OS_OK;
#endif
    return eRet;
}

void CamOsIrqFree(u32 nIrq, void *pDevId)
{
#ifdef CAM_OS_RTK
    DrvIntcFreeIsr(nIrq, pDevId);
    return;
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
#elif defined(CAM_OS_LINUX_KERNEL)
    struct CamOsHListNode_t *n              = NULL;
    CamOsIsrLK_t *           pHashListNode2 = NULL;
    u8                       nItemNum       = 0;
    u8                       u8Del          = 0;

    free_irq(nIrq, pDevId);
    mutex_lock(&_gtIsrLock);
    CAM_OS_HASH_FOR_EACH_POSSIBLE_SAFE(interrupt_hashtbl, pHashListNode2, n, tEntry, nIrq)
    {
        if (pHashListNode2->nIntNum == nIrq)
        {
            nItemNum++;
            if (pHashListNode2->pDevId == pDevId)
            {
                u8Del++;
                CAM_OS_HASH_DEL(&pHashListNode2->tEntry);
                CamOsMemRelease((void *)pHashListNode2);
                break;
            }
        }
    }
    mutex_unlock(&_gtIsrLock);

    if ((nItemNum == 0) || (u8Del == 0)) // IntNum err or pvDevId isn't match
    {
        CamOsPrintf("[%s][%d] nItemNum=%d u8Del=%d\r\n", __FUNCTION__, __LINE__, nItemNum, u8Del);
        CamOsPrintf("[%s][%d] Invalid ISR free in nIntNum=%d , DevId=0x%08X\r\n", __FUNCTION__, __LINE__, nIrq, pDevId);
        CamOsPanic("[INT_ERR]  Invalid interrupt parameters\r\n");
    }
#endif
}

void CamOsIrqEnable(u32 nIrq)
{
#ifdef CAM_OS_RTK
    DrvUnmaskInterrupt(nIrq);
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
#elif defined(CAM_OS_LINUX_KERNEL)
    enable_irq(nIrq);
#endif
}

void CamOsIrqDisable(u32 nIrq)
{
#ifdef CAM_OS_RTK
    DrvMaskInterrupt(nIrq);
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
#elif defined(CAM_OS_LINUX_KERNEL)
    disable_irq(nIrq);
#endif
}

CamOsRet_e FORCE_INLINE CamOsInInterrupt(void)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    if (!MsRunInIsrContext())
        eRet = CAM_OS_FAIL;
#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
#elif defined(CAM_OS_LINUX_KERNEL)
    if (!in_interrupt())
        eRet = CAM_OS_FAIL;
#endif
    return eRet;
}

void FORCE_INLINE CamOsMemoryBarrier(void)
{
    asm volatile("" : : : "memory");
}

void FORCE_INLINE CamOsSmpMemoryBarrier(void)
{
#ifdef CAM_OS_RTK

#elif defined(CAM_OS_LINUX_USER)
    CAM_OS_WARN("not support in " OS_NAME);
#elif defined(CAM_OS_LINUX_KERNEL)
    smp_mb();
#endif
}

#if defined(CAM_OS_RTK) || defined(CAM_OS_LINUX_KERNEL)
char _szErrStrBuf[128];
#endif

char *CamOsStrError(s32 nErrNo)
{
#ifdef CAM_OS_RTK
    sprintf(_szErrStrBuf, "errno: %d", nErrNo);
    return _szErrStrBuf;
#elif defined(CAM_OS_LINUX_USER)
    return strerror(nErrNo);
#elif defined(CAM_OS_LINUX_KERNEL)
    sprintf(_szErrStrBuf, "errno: %d", nErrNo);
    return _szErrStrBuf;
#endif
}

void CamOsPanic(const char *szMessage)
{
#ifdef CAM_OS_RTK
    CamOsPrintf("%s: %s\n", __FUNCTION__, szMessage);
    RtkExceptionRoutine(240, 0); // SYSTEM_ASSERT = 240
#elif defined(CAM_OS_LINUX_USER)
    CamOsPrintf("%s: %s\n", __FUNCTION__, szMessage);
    abort();
#elif defined(CAM_OS_LINUX_KERNEL)
    panic(szMessage);
#endif
}

void CamOsCallStack(void)
{
#ifdef CAM_OS_RTK
    MsCallStackDump();
#elif defined(CAM_OS_LINUX_USER)
    int    j, nptrs;
#define LINUX_USER_BT_BUF_SIZE 64
    void * buffer[LINUX_USER_BT_BUF_SIZE];
    char **strings;

    nptrs = backtrace(buffer, LINUX_USER_BT_BUF_SIZE);
    printf("backtrace() returned %d addresses\n", nptrs);

    /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
      would produce similar output to the following: */

    strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL)
    {
        perror("backtrace_symbols");
        exit(EXIT_FAILURE);
    }

    for (j = 0; j < nptrs; j++)
        printf("%s\n", strings[j]);

    free(strings);
#elif defined(CAM_OS_LINUX_KERNEL)
    dump_stack();
#endif
}

long CamOsStrtol(const char *szStr, char **szEndptr, s32 nBase)
{
#if defined(CAM_OS_RTK) || defined(CAM_OS_LINUX_USER)
    return strtol(szStr, szEndptr, nBase);
#elif defined(CAM_OS_LINUX_KERNEL)
    return simple_strtol(szStr, szEndptr, nBase);
#endif
}

unsigned long CamOsStrtoul(const char *szStr, char **szEndptr, s32 nBase)
{
#if defined(CAM_OS_RTK) || defined(CAM_OS_LINUX_USER)
    return strtoul(szStr, szEndptr, nBase);
#elif defined(CAM_OS_LINUX_KERNEL)
    return simple_strtoul(szStr, szEndptr, nBase);
#endif
}

unsigned long long CamOsStrtoull(const char *szStr, char **szEndptr, s32 nBase)
{
#if defined(CAM_OS_RTK) || defined(CAM_OS_LINUX_USER)
    return strtoull(szStr, szEndptr, nBase);
#elif defined(CAM_OS_LINUX_KERNEL)
    return simple_strtoull(szStr, szEndptr, nBase);
#endif
}

unsigned long _CamOsFindFirstZeroBit(unsigned long *pAddr, unsigned long nSize, unsigned long nOffset)
{
    unsigned long *pLongBitmap = pAddr + (nOffset / CAM_OS_BITS_PER_LONG);
    unsigned long  nResult     = nOffset & ~(CAM_OS_BITS_PER_LONG - 1), nTemp;

    if (nOffset >= nSize)
        return nSize;

    nSize -= nResult;
    nOffset %= CAM_OS_BITS_PER_LONG;
    if (nOffset)
    {
        nTemp = *(pLongBitmap++);
        nTemp |= ~0UL >> (CAM_OS_BITS_PER_LONG - nOffset);
        if (nSize < CAM_OS_BITS_PER_LONG)
            goto IN_FIRST_BIT;

        if (~nTemp)
            goto IN_OTHER_BIT;

        nSize -= CAM_OS_BITS_PER_LONG;
        nResult += CAM_OS_BITS_PER_LONG;
    }

    // while (nSize > CAM_OS_BITS_PER_LONG)
    while (nSize & ~(CAM_OS_BITS_PER_LONG - 1))
    {
        if (~(nTemp = *pLongBitmap))
            goto IN_OTHER_BIT;

        nResult += CAM_OS_BITS_PER_LONG;
        nSize -= CAM_OS_BITS_PER_LONG;
        pLongBitmap++;
    }

    if (!nSize)
        return nResult;

    nTemp = *pLongBitmap;

IN_FIRST_BIT:
    nTemp |= ~0UL << nSize;
    if (nTemp == ~0UL)
        return nResult + nSize;

IN_OTHER_BIT:
    return nResult + CAM_OS_FFZ(nTemp);
}

static struct CamOsListHead_t *
_CamOsListMerge(void *priv, int (*cmp)(void *priv, struct CamOsListHead_t *a, struct CamOsListHead_t *b),
                struct CamOsListHead_t *a, struct CamOsListHead_t *b)
{
    struct CamOsListHead_t head, *tail = &head;

    while (a && b)
    {
        /* if equal, take 'a' -- important for sort stability */
        if ((*cmp)(priv, a, b) <= 0)
        {
            tail->pNext = a;
            a           = a->pNext;
        }
        else
        {
            tail->pNext = b;
            b           = b->pNext;
        }
        tail = tail->pNext;
    }
    tail->pNext = a ?: b;
    return head.pNext;
}

static void
_CamOsListMergeAndRestoreBackLinks(void *priv,
                                   int (*cmp)(void *priv, struct CamOsListHead_t *a, struct CamOsListHead_t *b),
                                   struct CamOsListHead_t *head, struct CamOsListHead_t *a, struct CamOsListHead_t *b)
{
    struct CamOsListHead_t *tail  = head;
    u8                      count = 0;

    while (a && b)
    {
        /* if equal, take 'a' -- important for sort stability */
        if ((*cmp)(priv, a, b) <= 0)
        {
            tail->pNext = a;
            a->pPrev    = tail;
            a           = a->pNext;
        }
        else
        {
            tail->pNext = b;
            b->pPrev    = tail;
            b           = b->pNext;
        }
        tail = tail->pNext;
    }
    tail->pNext = a ?: b;

    do
    {
        /*
         * In worst cases this loop may run many iterations.
         * Continue callbacks to the client even though no
         * element comparison is needed, so the client's cmp()
         * routine can invoke cond_resched() periodically.
         */
        if (!(++count))
            (*cmp)(priv, tail->pNext, tail->pNext);

        tail->pNext->pPrev = tail;
        tail               = tail->pNext;
    } while (tail->pNext);

    tail->pNext = head;
    head->pPrev = tail;
}

void CamOsListSort(void *priv, struct CamOsListHead_t *head,
                   int (*cmp)(void *priv, struct CamOsListHead_t *a, struct CamOsListHead_t *b))
{
    struct CamOsListHead_t *part[CAM_OS_MAX_LIST_LENGTH_BITS + 1]; /* sorted partial lists
                        -- last slot is a sentinel */
    int                     lev;                                   /* index into part[] */
    int                     max_lev = 0;
    struct CamOsListHead_t *list;

    if (CAM_OS_LIST_EMPTY(head))
        return;

    memset(part, 0, sizeof(part));

    head->pPrev->pNext = NULL;
    list               = head->pNext;

    while (list)
    {
        struct CamOsListHead_t *cur = list;
        list                        = list->pNext;
        cur->pNext                  = NULL;

        for (lev = 0; part[lev]; lev++)
        {
            cur       = _CamOsListMerge(priv, cmp, part[lev], cur);
            part[lev] = NULL;
        }
        if (lev > max_lev)
        {
            if (lev >= CAM_OS_ARRAY_SIZE(part) - 1)
            {
                CAM_OS_WARN("list too long");
                lev--;
            }
            max_lev = lev;
        }
        part[lev] = cur;
    }

    for (lev = 0; lev < max_lev; lev++)
        if (part[lev])
            list = _CamOsListMerge(priv, cmp, part[lev], list);

    _CamOsListMergeAndRestoreBackLinks(priv, cmp, head, part[max_lev], list);
}
