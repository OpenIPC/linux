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
#include <stdlib.h>
#include <stdio.h>
#include "time.h"
#include "sys_sys.h"
#include "sys_MsWrapper_cus_os_flag.h"
#include "sys_MsWrapper_cus_os_sem.h"
#include "sys_MsWrapper_cus_os_util.h"
#include "sys_sys_isw_uart.h"
#include "cam_os_wrapper.h"
#include "cam_os_util_list.h"

#define CAM_OS_THREAD_STACKSIZE_DEFAULT         8192

typedef void *CamOsThreadEntry_t(void *);

typedef struct
{
    MsTaskId_e eHandleObj;
    CamOsThreadEntry_t *ptEntry;
    void *pArg;
    Ms_Flag_t tExitFlag;
    void *pStack;
} CamOsThreadHandleRtk_t, *pCamOsThreadHandleRtk;

typedef struct
{
    Ms_Flag_t tFlag;  /* or semaphore if supported */
    Ms_Mutex_t tMutex;
    u32 nSemval;
} CamOsTsemRtk_t, *pCamOsTsemRtk;

static Ms_Mutex_t _gtMemLock;

_Static_assert(sizeof(CamOsMutex_t) >= sizeof(Ms_Flag_t), "CamOsMutex_t size define not enough!");
_Static_assert(sizeof(CamOsTsem_t) >= sizeof(CamOsTsemRtk_t), "CamOsTsem_t size define not enough!");

#elif defined(CAM_OS_LINUX_USER)
#define _GNU_SOURCE
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/time.h>
#include <mdrv_msys_io.h>
#include <mdrv_msys_io_st.h>
#include "mdrv_verchk.h"
#include <sys/mman.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include "time.h"
#include "cam_os_wrapper.h"
#include "cam_os_util_list.h"

typedef struct
{
    pthread_cond_t tCondition;
    pthread_mutex_t tMutex;
    u32 nSemval;
} CamOsTsemLU_t, *pCamOsTsemLU;

// TODO remove extern pthread_setname_np and pthread_getname_np
extern int pthread_setname_np(pthread_t tTargetThread, const char *szTargetName);
extern int pthread_getname_np(pthread_t tThread, char *szName, size_t nLen);

static pthread_mutex_t _gtMemLock = PTHREAD_MUTEX_INITIALIZER;

_Static_assert(sizeof(CamOsMutex_t) >= sizeof(pthread_mutex_t), "CamOsMutex_t size define not enough! %d");
_Static_assert(sizeof(CamOsTsem_t) >= sizeof(CamOsTsemLU_t), "CamOsTsem_t size define not enough!");
_Static_assert(sizeof(CamOsTimespec_t) == sizeof(struct timespec), "CamOsTimespec_t size define error!");

#elif defined(CAM_OS_LINUX_KERNEL)
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <ms_msys.h>
#include <ms_platform.h>
#include "cam_os_wrapper.h"
#include "cam_os_util_list.h"

#define CAM_OS_THREAD_STACKSIZE_DEFAULT         8192

typedef struct
{
    struct semaphore tSem;
    struct mutex tMutex;
    u32 nSemval;
} CamOsTsemLK_t, *pCamOsTsemLK;

typedef s32 CamOsThreadEntry_t(void *);

struct mutex _gtMemLock;

_Static_assert(sizeof(CamOsMutex_t) >= sizeof(struct mutex), "CamOsMutex_t size define not enough! %d");
_Static_assert(sizeof(CamOsTsem_t) >= sizeof(CamOsTsemLK_t), "CamOsTsem_t size define not enough!");
_Static_assert(sizeof(CamOsTimespec_t) == sizeof(struct timespec), "CamOsTimespec_t size define error!");

#endif

typedef struct MemoryList_t
{
    struct CamOsListHead_t tList;
    void *pPtr;
    void *pMemifoPtr;
    char *szName;
} MemoryList_t;

static MemoryList_t _gtMemList;

static s32 _gnDmemDbgListInited = 0;

void CamOsDebug(const char *szFmt, ...)
{
#ifdef CAM_OS_RTK
    va_list tArgs;
    char nLineStr[256];

    va_start(tArgs, szFmt);
    vsprintf(nLineStr, szFmt, tArgs);
    send_msg(nLineStr);
    va_end(tArgs);
#elif defined(CAM_OS_LINUX_USER)
    va_list tArgs;

    va_start(tArgs, szFmt);
    vfprintf(stderr, szFmt, tArgs);
    va_end(tArgs);
#elif defined(CAM_OS_LINUX_KERNEL)
    va_list tArgs;

    va_start(tArgs, szFmt);
    vprintk(szFmt, tArgs);
    va_end(tArgs);
#endif
}

#ifdef CAM_OS_RTK
static char* _CamOsAdvance(char* pBuf) {

    char* pNewBuf = pBuf;

    /* Skip over nonwhite space */
    while ((*pNewBuf != ' ')  && (*pNewBuf != '\t') &&
            (*pNewBuf != '\n') && (*pNewBuf != '\0'))
    {
        pNewBuf++;
    }

    /* Skip white space */
    while ((*pNewBuf == ' ')  || (*pNewBuf == '\t') ||
            (*pNewBuf == '\n') || (*pNewBuf == '\0'))
    {
        pNewBuf++;
    }

    return pNewBuf;
}

static s32 _CamOsVsscanf(char* szBuf, char* szFmt, va_list tArgp)
{
    char*    pFmt;
    char*    pBuf;
    char*    pnSval;
    int*     pnIval;
    double*  pdbDval;
    float*   pfFval;
    s32      nCount = 0;

    pBuf = szBuf;

    for (pFmt = szFmt; *pFmt; pFmt++) {
        if (*pFmt == '%')
            switch (*++pFmt) {
                case 'd':
                    pnIval = va_arg(tArgp, int *);
                    sscanf(pBuf, "%d", pnIval);
                    pBuf = _CamOsAdvance(pBuf);
                    nCount++;
                    break;
                case 'f':
                    pfFval = va_arg(tArgp, float *);
                    sscanf(pBuf, "%f", pfFval);
                    pBuf = _CamOsAdvance(pBuf);
                    nCount++;
                    break;
                case 'l':
                    pdbDval = va_arg(tArgp, double *);
                    sscanf(pBuf, "%lf", pdbDval);
                    pBuf = _CamOsAdvance(pBuf);
                    nCount++;
                    break;
                case 's':
                    pnSval = va_arg(tArgp, char *);
                    sscanf(pBuf, "%s", pnSval);
                    pBuf = _CamOsAdvance(pBuf);
                    nCount++;
                    break;
                default:
                    break;
            }
    }
    return nCount;
}

static s32 _CamOsVfscanf(const char *szFmt, va_list tArgp)
{
    s32 nCount;

    nCount = _CamOsVsscanf(wait_for_command(), (char *)szFmt, tArgp);
    return nCount;
}
#endif

s32 CamOsScanf(const char *szFmt, ...)
{
#ifdef CAM_OS_RTK
    s32 nCount = 0;
    va_list tArgp;

    va_start (tArgp, szFmt);
    nCount = _CamOsVfscanf (szFmt, tArgp);
    va_end (tArgp);
    UartSendTrace("\n");
    return nCount;
#elif defined(CAM_OS_LINUX_USER)
    s32 nCount = 0;
    va_list tArgp;

    va_start (tArgp, szFmt);
    nCount = vfscanf (stdin, szFmt, tArgp);
    va_end (tArgp);
    return nCount;
#elif defined(CAM_OS_LINUX_KERNEL)
    return 0;
#endif
}

s32 CamOsGetChar(void)
{
#ifdef CAM_OS_RTK
    s32 Ret;
    Ret = get_char();
    UartSendTrace("\n");
    return Ret;
#elif defined(CAM_OS_LINUX_USER)
    return getchar();
#elif defined(CAM_OS_LINUX_KERNEL)
    return 0;
#endif
}

void CamOsMsSleep(u32 nMsec)
{
#ifdef CAM_OS_RTK
    MsSleep(RTK_MS_TO_TICK(nMsec));
#elif defined(CAM_OS_LINUX_USER)
    usleep((useconds_t)nMsec * 1000);
#elif defined(CAM_OS_LINUX_KERNEL)
    msleep(nMsec);
#endif
}

void CamOsGetMonotonicTime(CamOsTimespec_t *ptRes)
{
#ifdef CAM_OS_RTK
    u32 nTmp = VM_RTK_TICK_TO_MS(MsGetOsTick());
    ptRes->nSec = nTmp / 1000;
    ptRes->nNanoSec = (nTmp % 1000) * 1000000;
#elif defined(CAM_OS_LINUX_USER)
    clock_gettime(CLOCK_MONOTONIC, (struct timespec *)ptRes);
#elif defined(CAM_OS_LINUX_KERNEL)
    getrawmonotonic((struct timespec *)ptRes);
#endif
}

#ifdef CAM_OS_RTK
static void _CamOSThreadEntry(void *pEntryData)
{
    CamOsThreadHandleRtk_t *ptTaskHandle = (CamOsThreadHandleRtk_t *)pEntryData;

    ptTaskHandle->ptEntry(ptTaskHandle->pArg);

    MsFlagSetbits(&ptTaskHandle->tExitFlag, 0x00000001);
}

static void _CamOsThreadEmptyParser(vm_msg_t *ptMessage)
{

}
#endif

CamOsRet_e CamOsThreadCreate(CamOsThread *pThread,
                             CamOsThreadAttrb_t *ptAttrb,
                             void *(*pfnStartRoutine)(void *),
                             void *pArg)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsThreadHandleRtk_t *ptTaskHandle = NULL;
    MsTaskCreateArgs_t tTaskArgs = {0};
    u32 nPrio = 100;
    u32 nStkSz = CAM_OS_THREAD_STACKSIZE_DEFAULT;

    if(ptAttrb != NULL)
    {
        if((ptAttrb->nPriority >= 0) && (ptAttrb->nPriority < 100))
        {
            nPrio = ptAttrb->nPriority * 2;
        }
        nStkSz = (ptAttrb->nStackSize)? ptAttrb->nStackSize : CAM_OS_THREAD_STACKSIZE_DEFAULT;
    }

    *pThread = (void*) - 1;
    do
    {
        if(!(ptTaskHandle = MsCallocateMem(sizeof(CamOsThreadHandleRtk_t))))
        {
            CamOsDebug("%s : Allocate ptHandle fail\n\r", __FUNCTION__);
            eRet = CAM_OS_ALLOCMEM_FAIL;
            break;
        }

        ptTaskHandle->ptEntry = pfnStartRoutine;
        ptTaskHandle->pArg   = pArg;
        if(!(ptTaskHandle->pStack = MsAllocateMem((nStkSz) ? nStkSz : CAM_OS_THREAD_STACKSIZE_DEFAULT)))
        {
            CamOsDebug("%s : Allocate stack fail\n\r", __FUNCTION__);
            eRet = CAM_OS_ALLOCMEM_FAIL;
            break;
        }

        memset(&ptTaskHandle->tExitFlag, 0, sizeof(Ms_Flag_t));
        MsFlagInit(&ptTaskHandle->tExitFlag);
        //VEN_TEST_CHECK_RESULT((pTaskHandle->exit_flag.FlagId >> 0)  && (pTaskHandle->exit_flag.FlagState == RTK_FLAG_INITIALIZED));

        tTaskArgs.Priority = (nPrio >= 0 && nPrio <= 200) ? nPrio : 100;
        tTaskArgs.StackSize = (nStkSz) ? nStkSz : CAM_OS_THREAD_STACKSIZE_DEFAULT;
        tTaskArgs.pStackTop = (u32*)ptTaskHandle->pStack;
        tTaskArgs.AppliInit = &_CamOSThreadEntry;
        tTaskArgs.AppliParser = _CamOsThreadEmptyParser;
        tTaskArgs.pInitArgs = ptTaskHandle;
        tTaskArgs.TaskId = &ptTaskHandle->eHandleObj;
        tTaskArgs.ImmediatelyStart = TRUE;
        tTaskArgs.TimeSliceMax = 10;
        tTaskArgs.TimeSliceLeft = 10;
        strcpy(tTaskArgs.TaskName, "CamOS");

        if(MS_OK != MsCreateTask(&tTaskArgs))
        {
            CamOsDebug("%s : Create task fail\n\r", __FUNCTION__);
            eRet = CAM_OS_FAIL;
            break;
        }

        *pThread = ptTaskHandle;
    }
    while(0);

    if(!*pThread)
    {
        if(ptTaskHandle)
        {
            if(ptTaskHandle->pStack)
            {
                MsReleaseMemory(ptTaskHandle->pStack);
            }
            MsFlagDestroy(&ptTaskHandle->tExitFlag);
            MsReleaseMemory(ptTaskHandle);
        }
    }

    //CamOsDebug("%s get taskid: %d  priority: %d\n\r", __FUNCTION__, (u32)pTaskHandle->eHandleObj, TaskArgs.Priority);
#elif defined(CAM_OS_LINUX_USER)
    struct sched_param tSched;
    pthread_t tThreadHandle = NULL;
    pthread_attr_t tAttr;
    if(ptAttrb != NULL)
    {
        pthread_attr_init(&tAttr);
        pthread_attr_getschedparam(&tAttr, &tSched);
        pthread_attr_setinheritsched(&tAttr, PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy(&tAttr, SCHED_RR);

        if((ptAttrb->nPriority >= 0) && (ptAttrb->nPriority < 100))
        {
            tSched.sched_priority = ptAttrb->nPriority;
        }

        pthread_attr_setschedparam(&tAttr, &tSched);
        if(0 != ptAttrb->nStackSize)
        {
            pthread_attr_setstacksize(&tAttr, (size_t) ptAttrb->nStackSize);
            eRet = CAM_OS_FAIL;
        }
        pthread_create(&tThreadHandle, &tAttr, pfnStartRoutine, pArg);
        pthread_attr_destroy(&tAttr);
    }
    else
    {
        pthread_create(&tThreadHandle, NULL, pfnStartRoutine, pArg);
    }

    *pThread = (CamOsThread *)tThreadHandle;
#elif defined(CAM_OS_LINUX_KERNEL)
    struct task_struct *tpThreadHandle;
    struct sched_param tSche = { .sched_priority = 0 };
    u32 nStkSz = CAM_OS_THREAD_STACKSIZE_DEFAULT;

    if(ptAttrb != NULL)
    {
        if((ptAttrb->nPriority >= 0) && (ptAttrb->nPriority < 100))
        {
            tSche.sched_priority = ptAttrb->nPriority;
        }
        nStkSz = ptAttrb->nStackSize;
    }
    tpThreadHandle = kthread_run((CamOsThreadEntry_t *)pfnStartRoutine, pArg, "CAMOS");
    sched_setscheduler(tpThreadHandle, SCHED_RR, &tSche);
    *pThread = (CamOsThread *)tpThreadHandle;
#endif

    return eRet;
}

CamOsRet_e CamOsThreadJoin(CamOsThread thread)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsThreadHandleRtk_t *ptTaskHandle = (CamOsThreadHandleRtk_t *)thread;
    if(ptTaskHandle)
    {
        MsFlagWait(&ptTaskHandle->tExitFlag, 0x00000001, RTK_FLAG_WAITMODE_AND | RTK_FLAG_WAITMODE_CLR);
        MsFlagDestroy(&ptTaskHandle->tExitFlag);
        MsDeleteTask(ptTaskHandle->eHandleObj);

        if(ptTaskHandle->pStack)
        {
            MsReleaseMemory(ptTaskHandle->pStack);
        }
        MsReleaseMemory(ptTaskHandle);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    pthread_t tThreadHandle = (pthread_t)thread;
    if(tThreadHandle)
    {
        pthread_join(tThreadHandle, NULL);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)

#endif
    return eRet;
}

CamOsRet_e CamOsThreadStop(CamOsThread thread)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK

#elif defined(CAM_OS_LINUX_USER)

#elif defined(CAM_OS_LINUX_KERNEL)
    struct task_struct *tpThreadHandle = (struct task_struct *)thread;
    if(tpThreadHandle)
    {
        if(0 != kthread_stop((struct task_struct *)thread))
        {
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

CamOsRet_e CamOsThreadShouldStop(void)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK

#elif defined(CAM_OS_LINUX_USER)

#elif defined(CAM_OS_LINUX_KERNEL)
    if(kthread_should_stop())
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

CamOsRet_e CamOsThreadSetName(CamOsThread thread, const char *szName)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    // TODO RTK not support set task name dynamic.
#elif defined(CAM_OS_LINUX_USER)
    pthread_t tThreadHandle = (pthread_t)thread;
    if(tThreadHandle)
    {
        pthread_setname_np(tThreadHandle, szName);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)

#endif

    return eRet;
}

CamOsRet_e CamOsThreadGetName(CamOsThread thread, const char *szName, u32 nLen)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK

#elif defined(CAM_OS_LINUX_USER)
    pthread_t tThreadHandle = (pthread_t)thread;
    if(tThreadHandle)
    {
        pthread_getname_np(tThreadHandle, (char *)szName, nLen);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)

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

CamOsRet_e CamOsMutexInit(CamOsMutex_t *ptMutex)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    Ms_Mutex_t *ptHandle = (Ms_Mutex_t *)ptMutex;
    if(ptHandle)
    {
        if(CUS_OS_OK != MsInitMutex(ptHandle))
        {
            CamOsDebug("%s : Init mutex fail\n\r", __FUNCTION__);
            eRet = CAM_OS_FAIL;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    pthread_mutex_t *ptHandle = (pthread_mutex_t *)ptMutex;
    if(ptHandle)
    {
        if(0 != pthread_mutex_init(ptHandle, NULL))
        {
            fprintf(stderr, "%s : Init mutex fail\n\r", __FUNCTION__);
            eRet = CAM_OS_FAIL;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    struct mutex *ptHandle = (struct mutex *)ptMutex;
    if(ptHandle)
    {
        mutex_init(ptHandle);
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
    // Do Nothing in RTK
#elif defined(CAM_OS_LINUX_USER)
    pthread_mutex_t *ptHandle = (pthread_mutex_t *)ptMutex;
    if(ptHandle)
    {
        pthread_mutex_destroy(ptHandle);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)

#endif
    return eRet;
}

CamOsRet_e CamOsMutexLock(CamOsMutex_t *ptMutex)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    Ms_Mutex_t *ptHandle = (Ms_Mutex_t *)ptMutex;
    if(ptHandle)
    {
        if(CUS_OS_OK != MsMutexLock(ptHandle))
        {
            CamOsDebug("%s : Lock mutex fail\n\r", __FUNCTION__);
            eRet = CAM_OS_FAIL;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    s32 nErr = 0;
    pthread_mutex_t *ptHandle = (pthread_mutex_t *)ptMutex;
    if(ptHandle)
    {
        if(0 != (nErr = pthread_mutex_lock(ptHandle)))
        {
            fprintf(stderr, "%s : Lock mutex fail, err %d\n\r", __FUNCTION__, nErr);
            eRet = CAM_OS_FAIL;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    struct mutex *ptHandle = (struct mutex *)ptMutex;
    if(ptHandle)
    {
        mutex_lock(ptHandle);
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
    Ms_Mutex_t *ptHandle = (Ms_Mutex_t *)ptMutex;
    if(ptHandle)
    {
        if(CUS_OS_OK != MsMutexUnlock(ptHandle))
        {
            CamOsDebug("%s : Unlock mutex fail\n\r", __FUNCTION__);
            eRet = CAM_OS_FAIL;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    s32 nErr = 0;
    pthread_mutex_t *ptHandle = (pthread_mutex_t *)ptMutex;
    if(ptHandle)
    {
        if(0 != (nErr = pthread_mutex_unlock(ptHandle)))
        {
            fprintf(stderr, "%s : Unlock mutex fail, err %d\n\r", __FUNCTION__, nErr);
            eRet = CAM_OS_FAIL;
        }
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    struct mutex *ptHandle = (struct mutex *)ptMutex;
    if(ptHandle)
    {
        mutex_unlock(ptHandle);
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
    if(ptHandle)
    {
        MsFlagInit(&ptHandle->tFlag);
        MsInitMutex(&ptHandle->tMutex);
        ptHandle->nSemval = nVal;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsTsemLU_t *ptHandle = (CamOsTsemLU_t *)ptTsem;
    if(ptHandle)
    {
        if(0 != pthread_cond_init(&ptHandle->tCondition, NULL))
        {
            return CAM_OS_FAIL;
        }
        if(0 != pthread_mutex_init(&ptHandle->tMutex, NULL))
        {
            return CAM_OS_FAIL;
        }
        ptHandle->nSemval = nVal;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsTsemLK_t *ptHandle = (CamOsTsemLK_t *)ptTsem;
    if(ptHandle)
    {
        sema_init(&ptHandle->tSem, nVal);
        mutex_init(&ptHandle->tMutex);
        ptHandle->nSemval = nVal;
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
    if(ptHandle)
    {
        MsFlagDestroy(&ptHandle->tFlag);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsTsemLU_t *ptHandle = (CamOsTsemLU_t *)ptTsem;
    if(ptHandle)
    {
        pthread_cond_destroy(&ptHandle->tCondition);
        pthread_mutex_destroy(&ptHandle->tMutex);
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)

#endif
    return eRet;
}

void CamOsTsemUp(CamOsTsem_t *ptTsem)
{
#ifdef CAM_OS_RTK
    CamOsTsemRtk_t *ptHandle = (CamOsTsemRtk_t *)ptTsem;
    if(ptHandle)
    {
        MsMutexLock(&ptHandle->tMutex);
        ptHandle->nSemval++;
        MsMutexUnlock(&ptHandle->tMutex);
        MsFlagSetbits(&ptHandle->tFlag, 0x00000001);
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsTsemLU_t *ptHandle = (CamOsTsemLU_t *)ptTsem;
    if(ptHandle)
    {
        pthread_mutex_lock(&ptHandle->tMutex);
        ptHandle->nSemval++;
        pthread_cond_signal(&ptHandle->tCondition);
        pthread_mutex_unlock(&ptHandle->tMutex);
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsTsemLK_t *ptHandle = (CamOsTsemLK_t *)ptTsem;
    if(ptHandle)
    {
        mutex_lock(&ptHandle->tMutex);
        ptHandle->nSemval++;
        mutex_unlock(&ptHandle->tMutex);
        up(&ptHandle->tSem);
    }
#endif
}

void CamOsTsemDown(CamOsTsem_t *ptTsem)
{
#ifdef CAM_OS_RTK
    CamOsTsemRtk_t *ptHandle = (CamOsTsemRtk_t *)ptTsem;
    if(ptHandle)
    {
        MsMutexLock(&ptHandle->tMutex);
        while(ptHandle->nSemval == 0)
        {
            MsMutexUnlock(&ptHandle->tMutex);
            MsFlagWait(&ptHandle->tFlag, 0x00000001, MS_FLAG_WAITMODE_OR);
            MsMutexLock(&ptHandle->tMutex);
        }
        if(ptHandle->nSemval > 0)
        {
            ptHandle->nSemval--;
        }
        MsMutexUnlock(&ptHandle->tMutex);
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsTsemLU_t *ptHandle = (CamOsTsemLU_t *)ptTsem;
    if(ptHandle)
    {
        pthread_mutex_lock(&ptHandle->tMutex);
        while(ptHandle->nSemval == 0)
        {
            pthread_cond_wait(&ptHandle->tCondition, &ptHandle->tMutex);
        }
        ptHandle->nSemval--;
        pthread_mutex_unlock(&ptHandle->tMutex);
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsTsemLK_t *ptHandle = (CamOsTsemLK_t *)ptTsem;
    if(ptHandle)
    {
        down(&ptHandle->tSem);
        mutex_lock(&ptHandle->tMutex);
        ptHandle->nSemval--;
        mutex_unlock(&ptHandle->tMutex);
    }
#endif
}

CamOsRet_e CamOsTsemTimedDown(CamOsTsem_t *ptTsem, u32 nMsec)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsTsemRtk_t *ptHandle = (CamOsTsemRtk_t *)ptTsem;
    s32 nErr = 0;
    if(ptHandle)
    {
        MsMutexLock(&ptHandle->tMutex);
        while(ptHandle->nSemval == 0)
        {
            MsMutexUnlock(&ptHandle->tMutex);
            nErr = MsFlagTimedWait(&ptHandle->tFlag, 1, (MS_FLAG_WAITMODE_OR), RTK_MS_TO_TICK(nMsec));
            /*time out*/
            MsMutexLock(&ptHandle->tMutex);
            if(!nErr)
            {
                break;
            }
        }
        if(ptHandle->nSemval > 0)
        {
            ptHandle->nSemval--;
        }
        MsMutexUnlock(&ptHandle->tMutex);
        if(!nErr)
            eRet = CAM_OS_TIMEOUT;
        else
            eRet = CAM_OS_OK;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsTsemLU_t *ptHandle = (CamOsTsemLU_t *)ptTsem;
    s32 nErr = 0;
    struct timespec tFinalTime;
    struct timeval tCurrTime;
    u32 nMicDelay;

    if(ptHandle)
    {
        gettimeofday(&tCurrTime, NULL);
        /** convert timeval to timespec and add delay in milliseconds for the timeout */
        nMicDelay = ((nMsec * 1000 + tCurrTime.tv_usec));
        tFinalTime.tv_sec = tCurrTime.tv_sec + (nMicDelay / 1000000);
        tFinalTime.tv_nsec = (nMicDelay % 1000000) * 1000;
        pthread_mutex_lock(&ptHandle->tMutex);
        while(ptHandle->nSemval == 0)
        {
            nErr = pthread_cond_timedwait(&ptHandle->tCondition, &ptHandle->tMutex,
                                          &tFinalTime);
            if(nErr != 0)
            {
                ptHandle->nSemval--;
            }
        }
        ptHandle->nSemval--;
        pthread_mutex_unlock(&ptHandle->tMutex);

        if(!nErr)
            eRet = CAM_OS_OK;
        else if(nErr == ETIMEDOUT)
            eRet = CAM_OS_TIMEOUT;
        else
            eRet = CAM_OS_FAIL;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsTsemLK_t *ptHandle = (CamOsTsemLK_t *)ptTsem;
    s32 nErr = 0;
    if(ptHandle)
    {
        nErr = down_timeout(&ptHandle->tSem, msecs_to_jiffies(nMsec));
        mutex_lock(&ptHandle->tMutex);
        if(ptHandle->nSemval > 0)
        {
            ptHandle->nSemval--;
        }
        mutex_unlock(&ptHandle->tMutex);
        if(nErr)
            eRet = CAM_OS_TIMEOUT;
        else
            eRet = CAM_OS_OK;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

void CamOsTsemSignal(CamOsTsem_t *ptTsem)
{
#ifdef CAM_OS_RTK
    CamOsTsemRtk_t *ptHandle = (CamOsTsemRtk_t *)ptTsem;
    if(ptHandle)
    {
        MsFlagSetbits(&ptHandle->tFlag, 0x00000001);
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsTsemLU_t *ptHandle = (CamOsTsemLU_t *)ptTsem;
    if(ptHandle)
    {
        pthread_mutex_lock(&ptHandle->tMutex);
        pthread_cond_signal(&ptHandle->tCondition);
        pthread_mutex_unlock(&ptHandle->tMutex);
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsTsemLK_t *ptHandle = (CamOsTsemLK_t *)ptTsem;
    if(ptHandle)
    {
        up(&ptHandle->tSem);
    }
#endif
}

void CamOsTsemWait(CamOsTsem_t *ptTsem)
{
#ifdef CAM_OS_RTK
    CamOsTsemRtk_t *ptHandle = (CamOsTsemRtk_t *)ptTsem;
    if(ptHandle)
    {
        MsFlagWait(&ptHandle->tFlag, 0x00000001, MS_FLAG_WAITMODE_OR);
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsTsemLU_t *ptHandle = (CamOsTsemLU_t *)ptTsem;
    if(ptHandle)
    {
        pthread_mutex_lock(&ptHandle->tMutex);
        pthread_cond_wait(&ptHandle->tCondition, &ptHandle->tMutex);
        pthread_mutex_unlock(&ptHandle->tMutex);
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsTsemLK_t *ptHandle = (CamOsTsemLK_t *)ptTsem;
    if(ptHandle)
    {
        down(&ptHandle->tSem);
    }
#endif
}

CamOsRet_e CamOsTsemTimedWait(CamOsTsem_t *ptTsem, u32 nMsec)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    CamOsTsemRtk_t *ptHandle = (CamOsTsemRtk_t *)ptTsem;
    s32 nErr = 0;
    if(ptHandle)
    {
        nErr = MsFlagTimedWait(&ptHandle->tFlag, 1, (MS_FLAG_WAITMODE_OR), RTK_MS_TO_TICK(nMsec));
        if(!nErr)
            eRet = CAM_OS_TIMEOUT;
        else
            eRet = CAM_OS_OK;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsTsemLU_t *ptHandle = (CamOsTsemLU_t *)ptTsem;
    s32 nErr = 0;
    struct timespec tFinalTime;
    struct timeval tCurrTime;
    u32 nMicDelay;

    if(ptHandle)
    {
        gettimeofday(&tCurrTime, NULL);
        /** convert timeval to timespec and add delay in milliseconds for the timeout */
        nMicDelay = ((nMsec * 1000 + tCurrTime.tv_usec));
        tFinalTime.tv_sec = tCurrTime.tv_sec + (nMicDelay / 1000000);
        tFinalTime.tv_nsec = (nMicDelay % 1000000) * 1000;

        pthread_mutex_lock(&ptHandle->tMutex);

        nErr = pthread_cond_timedwait(&ptHandle->tCondition, &ptHandle->tMutex,
                                      &tFinalTime);

        pthread_mutex_unlock(&ptHandle->tMutex);

        if(!nErr)
            eRet = CAM_OS_OK;
        else if(nErr == ETIMEDOUT)
            eRet = CAM_OS_TIMEOUT;
        else
            eRet = CAM_OS_FAIL;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsTsemLK_t *ptHandle = (CamOsTsemLK_t *)ptTsem;
    s32 nErr = 0;
    if(ptHandle)
    {
        nErr = down_timeout(&ptHandle->tSem, msecs_to_jiffies(nMsec));
        if(nErr)
            eRet = CAM_OS_TIMEOUT;
        else
            eRet = CAM_OS_OK;
    }
    else
    {
        eRet = CAM_OS_PARAM_ERR;
    }
#endif
    return eRet;
}

u32 CamOsTsemGetValue(CamOsTsem_t *ptTsem)
{
    s32 eRet = 0;
#ifdef CAM_OS_RTK
    CamOsTsemRtk_t *ptHandle = (CamOsTsemRtk_t *)ptTsem;
    if(ptHandle)
    {
        eRet = ptHandle->nSemval;
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsTsemLU_t *ptHandle = (CamOsTsemLU_t *)ptTsem;
    if(ptHandle)
    {
        eRet = ptHandle->nSemval;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsTsemLK_t *ptHandle = (CamOsTsemLK_t *)ptTsem;
    if(ptHandle)
    {
        eRet = ptHandle->nSemval;
    }
#endif
    return eRet;
}

void CamOsTsemReset(CamOsTsem_t *ptTsem)
{
#ifdef CAM_OS_RTK
    CamOsTsemRtk_t *ptHandle = (CamOsTsemRtk_t *)ptTsem;
    if(ptHandle)
    {
        MsMutexLock(&ptHandle->tMutex);
        ptHandle->nSemval = 0;
        MsMutexUnlock(&ptHandle->tMutex);
    }
#elif defined(CAM_OS_LINUX_USER)
    CamOsTsemLU_t *ptHandle = (CamOsTsemLU_t *)ptTsem;
    if(ptHandle)
    {
        pthread_mutex_lock(&ptHandle->tMutex);
        ptHandle->nSemval = 0;
        pthread_mutex_unlock(&ptHandle->tMutex);
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    CamOsTsemLK_t *ptHandle = (CamOsTsemLK_t *)ptTsem;
    if(ptHandle)
    {
        sema_init(&ptHandle->tSem, 0);
        mutex_lock(&ptHandle->tMutex);
        ptHandle->nSemval = 0;
        mutex_unlock(&ptHandle->tMutex);
    }
#endif
}

void* CamOsMemAlloc(u32 nSize)
{
#ifdef CAM_OS_RTK
    return MsAllocateMem(nSize);
#elif defined(CAM_OS_LINUX_USER)
    return malloc(nSize);
#elif defined(CAM_OS_LINUX_KERNEL)
    return kzalloc(nSize, GFP_KERNEL);
#endif
}

void* CamOsMemCalloc(u32 nNum, u32 nSize)
{
#ifdef CAM_OS_RTK
    return MsCallocateMem(nNum * nSize);
#elif defined(CAM_OS_LINUX_USER)
    return calloc(nNum, nSize);
#elif defined(CAM_OS_LINUX_KERNEL)
    return kzalloc(nSize, GFP_KERNEL);
#endif
}

void* CamOsMemRealloc(void* pPtr, u32 nSize)
{
#ifdef CAM_OS_RTK
    return MsMemoryReAllocate(pPtr, nSize);
#elif defined(CAM_OS_LINUX_USER)
    return realloc(pPtr, nSize);
#elif defined(CAM_OS_LINUX_KERNEL)
    void *pAddr = kzalloc(nSize, GFP_KERNEL);
    if(pPtr && pAddr)
    {
        memcpy(pAddr, pPtr, nSize);
        kfree(pPtr);
    }
    return pAddr;
#endif
}

void CamOsMemRelease(void* pPtr)
{
#ifdef CAM_OS_RTK
    if(pPtr)
    {
        MsReleaseMemory(pPtr);
    }
#elif defined(CAM_OS_LINUX_USER)
    if(pPtr)
    {
        free(pPtr);
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    if(pPtr)
    {
        kfree(pPtr);
    }
#endif
}

static s32 _CheckDmemInfoListInited(void)
{
#ifdef CAM_OS_RTK
    if(!_gnDmemDbgListInited)
    {
        if(CUS_OS_OK != MsInitMutex(&_gtMemLock))
        {
            CamOsDebug("%s : Init mutex fail\n\r", __FUNCTION__);
        }

        memset(&_gtMemList, 0, sizeof(MemoryList_t));
        CAM_OS_INIT_LIST_HEAD(&_gtMemList.tList);

        _gnDmemDbgListInited = 1;
    }
#elif defined(CAM_OS_LINUX_USER)
    if(!_gnDmemDbgListInited)
    {
        pthread_mutex_init(&_gtMemLock, NULL);

        memset(&_gtMemList, 0, sizeof(MemoryList_t));
        CAM_OS_INIT_LIST_HEAD(&_gtMemList.tList);

        _gnDmemDbgListInited = 1;
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    if(!_gnDmemDbgListInited)
    {
        mutex_init(&_gtMemLock);

        memset(&_gtMemList, 0, sizeof(MemoryList_t));
        CAM_OS_INIT_LIST_HEAD(&_gtMemList.tList);

        _gnDmemDbgListInited = 1;
    }
#endif

    return 0;
}

CamOsRet_e CamOsDirectMemAlloc(const char* szName,
                               u32 nSize,
                               void** ppUserPtr,
                               u32 *pMiuAddr,
                               u64 *lpPhysAddr)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK
    u8 nAllocSucc = TRUE;
    *ppUserPtr = MsAllocateNonCacheMemExt(nSize, 12);
    nAllocSucc &= MsIsHeapMemory(*ppUserPtr);

    if((u32)*ppUserPtr & ((1 << 6) - 1))
    {
        nAllocSucc &= FALSE;
        MsReleaseMemory(*ppUserPtr);
    }

    if(nAllocSucc == TRUE)
    {
        *lpPhysAddr = (u32)MsVA2PA(*ppUserPtr);
        *pMiuAddr = (u32) * lpPhysAddr - (u32)&Image__RAM__Base;
    }
    else
    {
        *ppUserPtr = 0;
        *lpPhysAddr = (u64)0;
        *pMiuAddr = 0;
        eRet = CAM_OS_FAIL;
    }

    /*CamOsDebug("%s    0x%08X  0x%08X  0x%08X\r\n",
            __FUNCTION__,
            (u32)*ppUserPtr,
            (u32)*phys_addr,
            (u32)*miu_addr);*/

    _CheckDmemInfoListInited();

    MsMutexLock(&_gtMemLock);
    MemoryList_t* ptNewEntry = (MemoryList_t*) MsAllocateMem(sizeof(MemoryList_t));
    ptNewEntry->pPtr = *ppUserPtr;
    ptNewEntry->pMemifoPtr = NULL;
    CAM_OS_LIST_ADD_TAIL(&(ptNewEntry->tList), &_gtMemList.tList);
    MsMutexUnlock(&_gtMemLock);
#elif defined(CAM_OS_LINUX_USER)
    s32 nMsysFd = -1;
    s32 nMemFd = -1;
    MSYS_DMEM_INFO * ptMsysMem = NULL;
    unsigned char* pMmapPtr = NULL;
    struct CamOsListHead_t *ptPos, *ptQ;
    MemoryList_t* ptTmp;

    do
    {
        //Check request name to avoid allocate same dmem address.
        _CheckDmemInfoListInited();

        pthread_mutex_lock(&_gtMemLock);
        CAM_OS_LIST_FOR_EACH_SAFE(ptPos, ptQ, &_gtMemList.tList)
        {
            ptTmp = CAM_OS_LIST_ENTRY(ptPos, MemoryList_t, tList);

            if(ptTmp->pPtr && ptTmp->szName && 0 == strcmp(szName, ptTmp->szName))
            {
                fprintf(stderr, "%s request same dmem name: %s\n", __FUNCTION__, szName);
                eRet = CAM_OS_PARAM_ERR;
            }
        }
        pthread_mutex_unlock(&_gtMemLock);
        if(eRet == CAM_OS_PARAM_ERR)
        {
            *ppUserPtr = 0;
            *pMiuAddr = 0;
            *lpPhysAddr = 0;
            break;
        }

        if(0 > (nMsysFd = open("/dev/msys", O_RDWR | O_SYNC)))
        {
            fprintf(stderr, "%s open /dev/msys failed!!\n", __FUNCTION__);
            eRet = CAM_OS_FAIL;
            break;
        }

        if(0 > (nMemFd = open("/dev/mem", O_RDWR | O_SYNC)))
        {
            fprintf(stderr, "%s open /dev/mem failed!!\n", __FUNCTION__);
            eRet = CAM_OS_FAIL;
            break;
        }

        ptMsysMem = (MSYS_DMEM_INFO *) malloc(sizeof(MSYS_DMEM_INFO));
        MSYS_ADDR_TRANSLATION_INFO tAddrInfo;
        FILL_VERCHK_TYPE(tAddrInfo, tAddrInfo.VerChk_Version, tAddrInfo.VerChk_Size,
                         IOCTL_MSYS_VERSION);
        FILL_VERCHK_TYPE(*ptMsysMem, ptMsysMem->VerChk_Version,
                         ptMsysMem->VerChk_Size, IOCTL_MSYS_VERSION);

        ptMsysMem->length = nSize;
        snprintf(ptMsysMem->name, sizeof(ptMsysMem->name), "%s", szName);

        if(ioctl(nMsysFd, IOCTL_MSYS_REQUEST_DMEM, ptMsysMem))
        {
            ptMsysMem->length = 0;
            fprintf(stderr, "%s [%s][%d]Request Direct Memory Failed!!\n", __FUNCTION__, szName, (u32)nSize);
            free(ptMsysMem);
            eRet = CAM_OS_FAIL;
            break;
        }

        tAddrInfo.addr = ptMsysMem->phys;
        *lpPhysAddr = ptMsysMem->phys;
        if(ioctl(nMsysFd, IOCTL_MSYS_PHYS_TO_MIU, &tAddrInfo))
        {
            ioctl(nMsysFd, IOCTL_MSYS_RELEASE_DMEM, ptMsysMem);
            fprintf(stderr, "%s [%s][%d]IOCTL_MSYS_PHYS_TO_MIU Failed!!\n", __FUNCTION__, szName, (u32)nSize);
            free(ptMsysMem);
            eRet = CAM_OS_FAIL;
            break;
        }
        *pMiuAddr = (u32)tAddrInfo.addr;
        pMmapPtr = mmap(0, ptMsysMem->length, PROT_READ | PROT_WRITE, MAP_SHARED,
                        nMemFd, ptMsysMem->phys);
        if(pMmapPtr == (void *) - 1)
        {
            ioctl(nMsysFd, IOCTL_MSYS_RELEASE_DMEM, ptMsysMem);
            fprintf(stderr, "%s failed!! physAddr<0x%x> size<0x%x> errno<%d, %s> \r\n",
                    __FUNCTION__,
                    (u32)ptMsysMem->phys,
                    (u32)ptMsysMem->length, errno, strerror(errno));
            free(ptMsysMem);
            eRet = CAM_OS_FAIL;
            break;
        }
        *ppUserPtr = pMmapPtr;

        fprintf(stderr, "%s <%s> physAddr<0x%x> size<%d>  \r\n",
                __FUNCTION__,
                szName, (u32)ptMsysMem->phys,
                (u32)ptMsysMem->length);

        pthread_mutex_lock(&_gtMemLock);
        MemoryList_t* tpNewEntry = (MemoryList_t*) malloc(sizeof(MemoryList_t));
        tpNewEntry->pPtr = pMmapPtr;
        tpNewEntry->pMemifoPtr = (void *) ptMsysMem;
        tpNewEntry->szName = strdup(szName);
        CAM_OS_LIST_ADD_TAIL(&(tpNewEntry->tList), &_gtMemList.tList);
        pthread_mutex_unlock(&_gtMemLock);
    }
    while(0);

    if(nMsysFd >= 0)
    {
        close(nMsysFd);
    }
    if(nMemFd >= 0)
    {
        close(nMemFd);
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    MSYS_DMEM_INFO *ptDmem = NULL;
    MemoryList_t* ptNewEntry;
    struct CamOsListHead_t *ptPos, *ptQ;
    MemoryList_t* ptTmp;

    *ppUserPtr = 0;
    *pMiuAddr = 0;
    *lpPhysAddr = 0;

    do
    {
        //Check request name to avoid allocate same dmem address.
        _CheckDmemInfoListInited();

        mutex_lock(&_gtMemLock);
        CAM_OS_LIST_FOR_EACH_SAFE(ptPos, ptQ, &_gtMemList.tList)
        {
            ptTmp = CAM_OS_LIST_ENTRY(ptPos, MemoryList_t, tList);

            if(ptTmp->pPtr && ptTmp->szName && 0 == strcmp(szName, ptTmp->szName))
            {
                printk(KERN_WARNING "%s request same dmem name: %s\n", __FUNCTION__, szName);
                eRet = CAM_OS_PARAM_ERR;
            }
        }
        mutex_unlock(&_gtMemLock);
        if(eRet == CAM_OS_PARAM_ERR)
        {
            *ppUserPtr = 0;
            *pMiuAddr = 0;
            *lpPhysAddr = 0;
            break;
        }

        if(0 == (ptDmem = (MSYS_DMEM_INFO *)kzalloc(sizeof(MSYS_DMEM_INFO), GFP_KERNEL)))
        {
            printk(KERN_WARNING "%s kzalloc MSYS_DMEM_INFO fail\n", __FUNCTION__);
            eRet = CAM_OS_FAIL;
            break;
        }

        snprintf(ptDmem->name, 15, szName);
        ptDmem->length = nSize;

        if(0 != msys_request_dmem(ptDmem))
        {
            printk(KERN_WARNING "%s msys_request_dmem fail\n", __FUNCTION__);
            kfree(ptDmem);
            eRet = CAM_OS_FAIL;
            break;
        }

        *ppUserPtr = (void *)(u32)ptDmem->phys;
        *pMiuAddr = Chip_Phys_to_MIU(ptDmem->phys);
        *lpPhysAddr = ptDmem->phys;

        printk(KERN_INFO "%s <%s> physAddr<0x%08X> size<%d>  \r\n",
               __FUNCTION__,
               szName, (u32)ptDmem->phys,
               (u32)ptDmem->length);

        mutex_lock(&_gtMemLock);
        ptNewEntry = (MemoryList_t*) kzalloc(sizeof(MemoryList_t), GFP_KERNEL);
        ptNewEntry->pPtr = (void *)(u32)ptDmem->phys;
        ptNewEntry->pMemifoPtr = (void *) ptDmem;
        ptNewEntry->szName = (char *)kzalloc(strlen(szName), GFP_KERNEL);
        strcpy(ptNewEntry->szName, szName);
        CAM_OS_LIST_ADD_TAIL(&(ptNewEntry->tList), &_gtMemList.tList);
        mutex_unlock(&_gtMemLock);
    }
    while(0);
#endif
    return eRet;
}

CamOsRet_e CamOsDirectMemRelease(void* pUserPtr, u32 nSize)
{
    CamOsRet_e eRet = CAM_OS_OK;
    struct CamOsListHead_t *ptPos, *ptQ;
    MemoryList_t* ptTmp;
#ifdef CAM_OS_RTK
    if(pUserPtr)
    {
        MsReleaseMemory(pUserPtr);
        CamOsDebug("%s do release\n\r", __FUNCTION__);
    }

    _CheckDmemInfoListInited();

    MsMutexLock(&_gtMemLock);
    CAM_OS_LIST_FOR_EACH_SAFE(ptPos, ptQ, &_gtMemList.tList)
    {
        ptTmp = CAM_OS_LIST_ENTRY(ptPos, MemoryList_t, tList);

        if(ptTmp->pPtr == pUserPtr)
        {
            if(ptTmp->szName)
                free(ptTmp->szName);
            CAM_OS_LIST_DEL(ptPos);
            free(ptTmp);
        }
    }
    MsMutexUnlock(&_gtMemLock);
#elif defined(CAM_OS_LINUX_USER)
    s32 nMsysfd = -1;
    s32 nErr = 0;
    MSYS_DMEM_INFO *pMsysMem = NULL;

    do
    {
        if(0 > (nMsysfd = open("/dev/msys", O_RDWR | O_SYNC)))
        {
            fprintf(stderr, "%s open /dev/msys failed!!\n", __FUNCTION__);
            eRet = CAM_OS_FAIL;
            break;
        }

        nErr = munmap(pUserPtr, nSize);
        if(0 != nErr)
        {
            fprintf(stderr, "%s munmap failed!! <%p> size<%d> err<%d> errno<%d, %s> \r\n",
                    __FUNCTION__,
                    pUserPtr, (u32)nSize, nErr, errno, strerror(errno));
        }

        _CheckDmemInfoListInited();

        pthread_mutex_lock(&_gtMemLock);
        CAM_OS_LIST_FOR_EACH_SAFE(ptPos, ptQ, &_gtMemList.tList)
        {
            ptTmp = CAM_OS_LIST_ENTRY(ptPos, MemoryList_t, tList);

            if(ptTmp->pPtr == pUserPtr)
            {
                pMsysMem = (MSYS_DMEM_INFO *) ptTmp->pMemifoPtr;
                break;
            }
        }
        pthread_mutex_unlock(&_gtMemLock);
        if(pMsysMem == NULL)
        {
            fprintf(stderr, "%s find Msys_DMEM_Info node failed!! <%p>  \r\n", __FUNCTION__, pUserPtr);
            eRet = CAM_OS_FAIL;
            break;
        }

        if(ioctl(nMsysfd, IOCTL_MSYS_RELEASE_DMEM, pMsysMem))
        {
            fprintf(stderr, "%s : IOCTL_MSYS_RELEASE_DMEM error physAddr<0x%x>\n", __FUNCTION__, (u32)pMsysMem->phys);
            eRet = CAM_OS_FAIL;
            break;
        }
        if(pMsysMem)
        {
            free(pMsysMem);
            pMsysMem = NULL;
        }
        pthread_mutex_lock(&_gtMemLock);
        CAM_OS_LIST_FOR_EACH_SAFE(ptPos, ptQ, &_gtMemList.tList)
        {
            ptTmp = CAM_OS_LIST_ENTRY(ptPos, MemoryList_t, tList);

            if(ptTmp->pPtr == pUserPtr)
            {
                if(ptTmp->szName)
                    free(ptTmp->szName);
                CAM_OS_LIST_DEL(ptPos);
                free(ptTmp);
            }
        }
        pthread_mutex_unlock(&_gtMemLock);
    }
    while(0);

    if(nMsysfd >= 0)
    {
        close(nMsysfd);
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    MSYS_DMEM_INFO *tpDmem = NULL;

    do
    {
        _CheckDmemInfoListInited();

        mutex_lock(&_gtMemLock);
        CAM_OS_LIST_FOR_EACH_SAFE(ptPos, ptQ, &_gtMemList.tList)
        {
            ptTmp = CAM_OS_LIST_ENTRY(ptPos, MemoryList_t, tList);

            printk("search tmp->ptr: %08X  %s\n", (u32)ptTmp->pPtr, ptTmp->szName);

            if(ptTmp->pPtr == pUserPtr)
            {
                tpDmem = ptTmp->pMemifoPtr;
                printk("search(2) pdmem->name: %s\n", tpDmem->name);
                break;
            }
        }
        mutex_unlock(&_gtMemLock);
        if(tpDmem == NULL)
        {
            printk(KERN_WARNING "%s find Msys_DMEM_Info node failed!! <%p>  \r\n", __FUNCTION__, pUserPtr);
            eRet = CAM_OS_FAIL;
            break;
        }

        msys_release_dmem(tpDmem);

        if(tpDmem)
        {
            kfree(tpDmem);
            tpDmem = NULL;
        }
        mutex_lock(&_gtMemLock);
        CAM_OS_LIST_FOR_EACH_SAFE(ptPos, ptQ, &_gtMemList.tList)
        {
            ptTmp = CAM_OS_LIST_ENTRY(ptPos, MemoryList_t, tList);

            if(ptTmp->pPtr == pUserPtr)
            {
                if(ptTmp->szName)
                    kfree(ptTmp->szName);
                CAM_OS_LIST_DEL(ptPos);
                kfree(ptTmp);
            }
        }
        mutex_unlock(&_gtMemLock);
    }
    while(0);
#endif
    return eRet;
}

CamOsRet_e CamOsDirectMemStat(void)
{
    CamOsRet_e eRet = CAM_OS_OK;
    struct CamOsListHead_t *ptPos, *ptQ;
    MemoryList_t* ptTmp;
#ifdef CAM_OS_RTK
    _CheckDmemInfoListInited();

    MsMutexLock(&_gtMemLock);
    CAM_OS_LIST_FOR_EACH_SAFE(ptPos, ptQ, &_gtMemList.tList)
    {
        ptTmp = CAM_OS_LIST_ENTRY(ptPos, MemoryList_t, tList);

        if(ptTmp->pPtr)
        {
            CamOsDebug("%s memory allocated %p %s\n", __FUNCTION__, ptTmp->pPtr, ptTmp->szName);
        }
    }
    MsMutexUnlock(&_gtMemLock);
#elif defined(CAM_OS_LINUX_USER)
    _CheckDmemInfoListInited();

    pthread_mutex_lock(&_gtMemLock);
    CAM_OS_LIST_FOR_EACH_SAFE(ptPos, ptQ, &_gtMemList.tList)
    {
        ptTmp = CAM_OS_LIST_ENTRY(ptPos, MemoryList_t, tList);

        if(ptTmp->pPtr)
        {
            fprintf(stderr, "%s memory allocated %p %s\n", __FUNCTION__, ptTmp->pPtr, ptTmp->szName);
        }
    }
    pthread_mutex_unlock(&_gtMemLock);
#elif defined(CAM_OS_LINUX_KERNEL)
    _CheckDmemInfoListInited();

    mutex_lock(&_gtMemLock);
    CAM_OS_LIST_FOR_EACH_SAFE(ptPos, ptQ, &_gtMemList.tList)
    {
        ptTmp = CAM_OS_LIST_ENTRY(ptPos, MemoryList_t, tList);

        if(ptTmp->pPtr)
        {
            printk(KERN_WARNING "%s memory allocated %p %s\n", __FUNCTION__, ptTmp->pPtr, ptTmp->szName);
        }
    }
    mutex_unlock(&_gtMemLock);
#endif
    return eRet;
}

CamOsRet_e CamOsPropertySet(const char *szKey, const char *szValue)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK

#elif defined(CAM_OS_LINUX_USER)
    void *pLibHandle = NULL;
    s32(*dlsym_property_set)(const char *szKey, const char *szValue) = NULL;

    do
    {
        pLibHandle = dlopen("libat.so", RTLD_NOW);
        if(NULL == pLibHandle)
        {
            fprintf(stderr, "%s : load libat.so error \n", __FUNCTION__);
            eRet = CAM_OS_FAIL;
            break;
        }

        dlsym_property_set = dlsym(pLibHandle, "property_set");
        if(NULL == dlsym_property_set)
        {
            fprintf(stderr, "%s : dlsym property_set failed, %s\n", __FUNCTION__, dlerror());
            eRet = CAM_OS_FAIL;
            break;
        }

        dlsym_property_set(szKey, szValue);
    }
    while(0);

    if(pLibHandle)
    {
        dlclose(pLibHandle);
    }
#elif defined(CAM_OS_LINUX_KERNEL)

#endif
    return eRet;
}

CamOsRet_e CamOsPropertyGet(const char *szKey, char *szValue, const char *szDefaultValue)
{
    CamOsRet_e eRet = CAM_OS_OK;
#ifdef CAM_OS_RTK

#elif defined(CAM_OS_LINUX_USER)
    void *pLibHandle = NULL;
    s32(*dlsym_property_get)(const char *szKey, char *szValue, const char *szDefaultValue) = NULL;

    do
    {
        pLibHandle = dlopen("libat.so", RTLD_NOW);
        if(NULL == pLibHandle)
        {
            fprintf(stderr, "%s : load libat.so error \n", __FUNCTION__);
            eRet = CAM_OS_FAIL;
            break;
        }
        dlsym_property_get = dlsym(pLibHandle, "property_get");
        if(NULL == dlsym_property_get)
        {
            fprintf(stderr, "%s : dlsym property_get failed, %s\n", __FUNCTION__, dlerror());
            eRet = CAM_OS_FAIL;
            break;
        }

        dlsym_property_get(szKey, szValue, szDefaultValue);
    }
    while(0);

    if(pLibHandle)
    {
        dlclose(pLibHandle);
    }
#elif defined(CAM_OS_LINUX_KERNEL)

#endif
    return eRet;
}