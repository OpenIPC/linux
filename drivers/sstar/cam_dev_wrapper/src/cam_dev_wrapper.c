/*
 * cam_dev_wrapper.c- Sigmastar
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

#include "cam_os_wrapper.h"
#include "cam_dev_wrapper.h"

#ifdef CAM_OS_RTK
#include <string.h>
#include "cam_os_util_list.h"
#include "sys_sys.h"
#include "sys_MsWrapper_cus_os_flag.h"
#include "sys_MsWrapper_cus_os_sem.h"
#include "sys_MsWrapper_cus_os_util.h"
#include "sys_sys_isw_uart.h"
#include "cam_dev_pollsample.h"

typedef int (*PfnCamPoll)(int fdt, short nEvents, short *reqEvents, int ntimeout);

typedef struct __CamDevFuncMap_t
{
    char *szName;
    int (*pOpen)(char *szName);
    int (*pClose)(int fdt);
    int (*pIoctl)(int fdt, unsigned long request, void *param);
    void *(*pMmap)(void *start, int length, int fdt, int offset);
    int (*pMunmap)(void *start, int length);
    int (*pPoll)(int fdt, short nEvents, short *reqEvents, int ntimeout);
    int (*pRead)(int fdt, void *buf, unsigned int count);
    int (*pWrite)(int fdt, const void *buf, unsigned int count);
} CamDevFuncMap_t;

typedef struct __CamDevFD_t
{
    struct CamOsListHead_t list;
    int                    nDevFD;
    int                    nDrvFD;
    int                    nValid;
    CamDevFuncMap_t *      pDevMapFunc;
} CamDevFD_t;

static unsigned int _gCamDevFD      = 1;
static unsigned int _gInitFDone     = 0;
static unsigned int _gInitMutexDone = 0;
static unsigned int _gFileCnt       = 0;
static CamDevFD_t   _gCamDevFDList;
static CamOsMutex_t _gFDLock;

static void _CamDevPollNodeFree(int fd);
int         AlkaidPollAdpOpen(char *szName);
int         AlkaidPollAdpRelease(int fdt);
int         AlkaidPollAdpIoctl(int fdt, unsigned long request, void *param);
int         AlkaidPollAdpPoll(int fdt, short nEvents, short *reqEvents, int ntimeout);

static CamDevFuncMap_t _gDevMapTable[] = {{"alkaidpoll", AlkaidPollAdpOpen, AlkaidPollAdpRelease, AlkaidPollAdpIoctl,
                                           NULL, NULL, AlkaidPollAdpPoll, NULL, NULL}};

#define DEVMAPTABLE_SZ (sizeof(_gDevMapTable) / sizeof(CamDevFuncMap_t))

static CamDevFuncMap_t *_CamDevFindDevMapTable(char *name)
{
    int i;

    for (i = 0; i < DEVMAPTABLE_SZ; i++)
    {
        if (!strcmp(_gDevMapTable[i].szName, name))
            return &_gDevMapTable[i];
    }

    return NULL;
}

static CamDevFD_t *_CamDevAllocFD(int ndrvfd, CamDevFuncMap_t *pfuncmap)
{
    struct CamOsListHead_t *pos, *q;
    CamDevFD_t *            tmp;

    CamOsMutexLock(&_gFDLock);

    CAM_OS_LIST_FOR_EACH_SAFE(pos, q, &_gCamDevFDList.list)
    {
        tmp = CAM_OS_LIST_ENTRY(pos, CamDevFD_t, list);

        if (!tmp->nValid)
        {
            tmp->nDrvFD      = ndrvfd;
            tmp->pDevMapFunc = pfuncmap;
            tmp->nValid      = 1;
            CamOsMutexUnlock(&_gFDLock);
            return tmp;
        }
    }

    tmp = CamOsMemAlloc(sizeof(CamDevFD_t));

    if (tmp == NULL)
    {
        CamOsPrintf("%s : can't allocate memory\n\r", __FUNCTION__);
        CamOsMutexUnlock(&_gFDLock);
        return NULL;
    }

    tmp->nDevFD      = _gCamDevFD++;
    tmp->nDrvFD      = ndrvfd;
    tmp->nValid      = 1;
    tmp->pDevMapFunc = pfuncmap;

    CAM_OS_INIT_LIST_HEAD(&tmp->list);
    CAM_OS_LIST_ADD_TAIL(&tmp->list, &_gCamDevFDList.list);
    CamOsMutexUnlock(&_gFDLock);

    return tmp;
}

static int _CamDevCloseFD(int nFd)
{
    struct CamOsListHead_t *pos, *q;
    CamDevFD_t *            tmp;

    CamOsMutexLock(&_gFDLock);

    CAM_OS_LIST_FOR_EACH_SAFE(pos, q, &_gCamDevFDList.list)
    {
        tmp = CAM_OS_LIST_ENTRY(pos, CamDevFD_t, list);

        if (tmp->nValid && tmp->nDevFD == nFd)
        {
            tmp->nValid = 0;
            CamOsMutexUnlock(&_gFDLock);
            return CAM_OS_OK;
        }
    }

    CamOsMutexUnlock(&_gFDLock);

    return CAM_OS_FAIL;
}

static CamDevFD_t *_CamDevFindFD(int nFd)
{
    struct CamOsListHead_t *pos, *q;
    CamDevFD_t *            tmp;

    CamOsMutexLock(&_gFDLock);

    CAM_OS_LIST_FOR_EACH_SAFE(pos, q, &_gCamDevFDList.list)
    {
        tmp = CAM_OS_LIST_ENTRY(pos, CamDevFD_t, list);

        if (tmp->nValid && tmp->nDevFD == nFd)
        {
            CamOsMutexUnlock(&_gFDLock);
            return tmp;
        }
    }

    CamOsMutexUnlock(&_gFDLock);

    return NULL;
}

/**
 * purpose : open device file.
 * param   :
 * return  :
 */
int CamDevOpen(char *name)
{
    CamDevFuncMap_t *pdevfunc;
    CamDevFD_t *     pfd;
    int              nfd;

    if (!_gInitMutexDone)
    {
        _gInitMutexDone = 1;
        if (CAM_OS_OK != CamOsMutexInit(&_gFDLock))
        {
            _gInitFDone = 1;
            CamOsPrintf("%s : Init mutex fail\n\r", __FUNCTION__);
            return CAM_OS_FAIL;
        }
    }

    CamOsMutexLock(&_gFDLock);
    if (!_gInitFDone)
    {
        _gInitFDone = 1;
        memset(&_gCamDevFDList, 0, sizeof(CamDevFD_t));
        CAM_OS_INIT_LIST_HEAD(&_gCamDevFDList.list);
    }
    CamOsMutexUnlock(&_gFDLock);

    if ((pdevfunc = _CamDevFindDevMapTable(name)) == NULL)
    {
        CamOsPrintf("%s : can't find %s in dev table\n\r", __FUNCTION__, name);
        return CAM_OS_FAIL;
    }

    if (!pdevfunc->pOpen)
    {
        CamOsPrintf("%s : %s no open function\n\r", __FUNCTION__, name);
        return CAM_OS_FAIL;
    }

    nfd = pdevfunc->pOpen(name);

    if (nfd < 0)
    {
        CamOsPrintf("%s : %s open fail 0x%x\n\r", __FUNCTION__, name, nfd);
        return CAM_OS_FAIL;
    }

    if ((pfd = _CamDevAllocFD(nfd, pdevfunc)) == NULL)
    {
        CamOsPrintf("%s : %s _CamDevAllocFD fail %d\n\r", __FUNCTION__, name, nfd);
        return CAM_OS_FAIL;
    }

    CamOsMutexLock(&_gFDLock);
    _gFileCnt++;
    CamOsMutexUnlock(&_gFDLock);

    return pfd->nDevFD;
}

/**
 * purpose : close device file.
 * param   :
 * return  :
 */
int CamDevClose(int fd)
{
    CamDevFD_t *            ptmpfd;
    struct CamOsListHead_t *pos, *q;

    if ((ptmpfd = _CamDevFindFD(fd)) == NULL)
    {
        CamOsPrintf("%s : %s find fd fail %d\n\r", __FUNCTION__, fd);
        return CAM_OS_FAIL;
    }

    if (ptmpfd->pDevMapFunc->pPoll)
    {
        _CamDevPollNodeFree(fd);
    }

    if (ptmpfd->pDevMapFunc->pClose)
    {
        ptmpfd->pDevMapFunc->pClose(ptmpfd->nDrvFD);
    }

    if (_CamDevCloseFD(fd) != CAM_OS_OK)
    {
        CamOsPrintf("%s : %s close fail %d\n\r", __FUNCTION__, fd);
        return CAM_OS_FAIL;
    }

    CamOsMutexLock(&_gFDLock);
    _gFileCnt--;

    if (_gFileCnt <= 0)
    {
        _gFileCnt = 0;

        CAM_OS_LIST_FOR_EACH_SAFE(pos, q, &_gCamDevFDList.list)
        {
            ptmpfd = CAM_OS_LIST_ENTRY(pos, CamDevFD_t, list);

            if (ptmpfd->nValid)
            {
                CamOsPrintf(" %s maybe user not use close function\n\r", ptmpfd->pDevMapFunc->szName);
            }
            CAM_OS_LIST_DEL(&ptmpfd->list);
            CamOsMemRelease((void *)ptmpfd);
        }
        _gInitFDone = 0;
        _gCamDevFD  = 1;
    }
    CamOsMutexUnlock(&_gFDLock);

    return CAM_OS_OK;
}

/**
 * purpose :
 * param   :
 * return  :
 */
int CamDevIoctl(int fd, unsigned long request, void *param)
{
    int         ret;
    CamDevFD_t *ptmpfd;

    if ((ptmpfd = _CamDevFindFD(fd)) == NULL)
    {
        CamOsPrintf("%s : %s find fd fail %d\n\r", __FUNCTION__, fd);
        return CAM_OS_FAIL;
    }

    if (ptmpfd->pDevMapFunc->pIoctl)
    {
        ret = ptmpfd->pDevMapFunc->pIoctl(ptmpfd->nDrvFD, request, param);
    }
    else
    {
        CamOsPrintf("%s : no support ioctl\n\r", ptmpfd->pDevMapFunc->szName);
        return CAM_OS_FAIL;
    }

    return ret;
}

/**
 * purpose :
 * param   :
 * return  :
 */
void *CamDevMmap(int length, int fd, int offset)
{
    CamDevFD_t *ptmpfd;

    if ((ptmpfd = _CamDevFindFD(fd)) == NULL)
    {
        CamOsPrintf("%s : %s find fd fail %d\n\r", __FUNCTION__, fd);
        return NULL;
    }

    if (ptmpfd->pDevMapFunc->pMmap)
    {
        return ptmpfd->pDevMapFunc->pMmap(0, length, ptmpfd->nDrvFD, offset);
    }

    CamOsPrintf("%s : no support MMAP\n\r", ptmpfd->pDevMapFunc->szName);
    return NULL;
}

/**
 * purpose :
 * param   :
 * return  :
 */
int CamDevMunmap(int fd, void *start, int length)
{
    CamDevFD_t *ptmpfd;

    if ((ptmpfd = _CamDevFindFD(fd)) == NULL)
    {
        CamOsPrintf("%s : %s find fd fail %d\n\r", __FUNCTION__, fd);
        return CAM_OS_FAIL;
    }

    if (ptmpfd->pDevMapFunc->pMunmap)
    {
        return ptmpfd->pDevMapFunc->pMunmap(start, length);
    }

    CamOsPrintf("%s : no support Munmap\n\r", ptmpfd->pDevMapFunc->szName);
    return CAM_OS_FAIL;
}

/**
 * purpose :
 * param   :
 * return  :
 */
int CamDevRead(int fd, void *buf, unsigned int count)
{
    int         ret;
    CamDevFD_t *ptmpfd;

    if ((ptmpfd = _CamDevFindFD(fd)) == NULL)
    {
        CamOsPrintf("%s : %s find fd fail %d\n\r", __FUNCTION__, fd);
        return CAM_OS_FAIL;
    }

    if (ptmpfd->pDevMapFunc->pRead)
    {
        ret = ptmpfd->pDevMapFunc->pRead(ptmpfd->nDrvFD, buf, count);
    }
    else
    {
        CamOsPrintf("%s : no support read\n\r", ptmpfd->pDevMapFunc->szName);
        return CAM_OS_FAIL;
    }

    return ret;
}

/**
 * purpose :
 * param   :
 * return  :
 */
int CamDevWrite(int fd, const void *buf, unsigned int count)
{
    int         ret;
    CamDevFD_t *ptmpfd;

    if ((ptmpfd = _CamDevFindFD(fd)) == NULL)
    {
        CamOsPrintf("%s : %s find fd fail %d\n\r", __FUNCTION__, fd);
        return CAM_OS_FAIL;
    }

    if (ptmpfd->pDevMapFunc->pWrite)
    {
        ret = ptmpfd->pDevMapFunc->pWrite(ptmpfd->nDrvFD, buf, count);
    }
    else
    {
        CamOsPrintf("%s : no support write\n\r", ptmpfd->pDevMapFunc->szName);
        return CAM_OS_FAIL;
    }

    return ret;
}

/*=============================================================================
 *
 *                  CamDevPoll Implementation
 *
 *===========================================================================*/
#define POLL_DBG_LV_0    0
#define POLL_DBG_LV_1    0
#define POLL_DBG_LV_2    0
#define POLL_DBG_LV_3    0
#define POLL_DBG_TIMEOUT 0

#define CAM_POLL_DBG(dbglv, _fmt, _args...) \
    if (dbglv)                              \
    {                                       \
        CamOsPrintf(_fmt, ##_args);         \
    }

#define CAM_POLL_ERR(_fmt, _args...) CamOsPrintf(_fmt, ##_args);

#define CAM_DEV_POLL_PANIC()                   \
    {                                          \
        CamOsPrintf("CAM_DEV_POLL_PANIC\n\r"); \
        while (1)                              \
        {                                      \
            CamOsMsSleep(5000);                \
        }                                      \
    }

#define _WITH_SYSTEM_THREAD 1 /* If enabled, RTK init.c should init 5 system threads for CamDevPollThread */

//=============================================================================

#define MAX_POLL_FD_A_BATCH    3    /* max number of fds in a poll */
#define CAM_DEV_POLL_STACKSIZE 8192 /* stack size of poll threads */

#if _WITH_SYSTEM_THREAD
#define NUM_SYSTEM_POOLTHD 5
#else
#define NUM_SYSTEM_POOLTHD 0
#endif

/* We crate dymaic poll threads if no poll threads is available for a coming poll */
#define NUM_DYNAMIC_POOLTHD_A_BATCH 5  /* number if poll threads created in a batch */
#define MAX_POOL_ENTRY_NUM          20 /* The total number of poll threads allowed */

#define MAX_POLL_ENTRY_NUM 20 /* The total number of poll table entries */

/*
 * Poll thread pool table entry.
 * The pool table implements the thread pool concept and is used to manage the
 * poll threads. Each fd-poll is executed by a poll thread, and that thread is
 * reused for that specific fd-poll until fd is closed.
 */
typedef struct
{
    int nUsed;    /* the thread is used by a fd-poll or not */
    int nSuspend; /* the thread is suspend or running-->invoking a driver poll or not */

    int         nPollThdId;  /* ID of this poll thread */
    CamOsTsem_t tActiveTsem; /* the suspend/resume flag for the poll thread */
    void *      pUserData;   /* the data passed from poll requests. It is the poll table entrynow. */
} CamDevPollThreadPool_t;

static CamDevPollThreadPool_t _gThdPoolTable[MAX_POOL_ENTRY_NUM]; // = {-1};  /* pollThd Pool table */
static CamOsMutex_t _gThdPoolMutex; // = { 0 };      /* protect poll table from being simultaneously accessed */
static int          _gInitPoolMutexDone = 0;
static int          _gInitThdPoolDone   = 0;
static int          _gNumPoolEntry      = NUM_SYSTEM_POOLTHD; /*number of the current poll threads */

/*
 * Poll table entry.
 * Each fd in a poll corresponds to a poll table entry which is reused for that specific fd-poll.
 * The poll table entry is released on fd close.
 */
typedef struct
{
    int nValid; /* Is this entry used by a fd-poll ? */

    int             nTid;   /* ID of the thread issuing this poll */
    int             nDevFD; /* device fd */
    int             nDrvFD; /* driver fd */
    Ms_Flag_t *     pFlag;  /* flag used to wait on drv poll events or timeout */
    Ms_flag_value_t tFlagBit;
    short           nEvents;  /* requested events */
    short           nRevents; /* returned events */
    int             nTimeout; /* timeout setting of this poll */

    int        nPoolIndex; /* the index of the pool table entry which provides poll thread for this fd-poll */
    PfnCamPoll pfnDrvPoll; /* the actual poll function corresponding to this fd */
} CamDevPollNode_t;

static CamDevPollNode_t _gPollTable[MAX_POLL_ENTRY_NUM]; /* Poll table */
static CamOsMutex_t     _gPollMutex;                     /* protect poll table from being simultaneously accessed */
static s32              _gInitPollDone = 0;

/*=============================================================================
 *
 *  pollThread Pool Management
 *
 *===========================================================================*/

void              CamDevPollThread(void);
static CamOsRet_e _CamDevPollThreadPoolInit()
{
    int                     i;
    CamDevPollThreadPool_t *pPoolNode;

    if (!_gInitPoolMutexDone)
    {
        if (CAM_OS_OK != CamOsMutexInit(&_gThdPoolMutex))
        {
            CAM_POLL_ERR("%s : Init mutex fail\n\r", __FUNCTION__);
            return CAM_OS_FAIL;
        }
        _gInitPoolMutexDone = 1;
    }

    CamOsMutexLock(&_gThdPoolMutex);
    if (!_gInitThdPoolDone)
    {
        for (i = 0; i < MAX_POOL_ENTRY_NUM; i++)
        {
            pPoolNode = &_gThdPoolTable[i];

            CamOsTsemInit(&pPoolNode->tActiveTsem, 0);
            pPoolNode->nPollThdId = -1;
            pPoolNode->pUserData  = NULL;
            pPoolNode->nUsed      = 0;
            pPoolNode->nSuspend   = 0;
        }
        _gInitThdPoolDone = 1;
    }
    CAM_POLL_DBG(POLL_DBG_LV_0, "%s: done \n\r", __FUNCTION__);
    CamOsMutexUnlock(&_gThdPoolMutex);

    return CAM_OS_OK;
}

static CamOsRet_e _DynPollThreadPoolCreate()
{
    int                i;
    CamOsThreadAttrb_t attr;
    CamOsRet_e         eRet = CAM_OS_OK;
    CamOsThread        thread;
    int                nToCreate = NUM_DYNAMIC_POOLTHD_A_BATCH;

    if (_gNumPoolEntry >= MAX_POOL_ENTRY_NUM)
    {
        return CAM_OS_FAIL;
    }
    else if ((_gNumPoolEntry + nToCreate) > MAX_POOL_ENTRY_NUM)
    {
        nToCreate = MAX_POOL_ENTRY_NUM - _gNumPoolEntry;
    }

    attr.nPriority  = 100;
    attr.nStackSize = CAM_DEV_POLL_STACKSIZE;

    for (i = 0; i < nToCreate; i++)
    {
        eRet = CamOsThreadCreate(&thread, &attr, (void *)CamDevPollThread, NULL);
        if (CAM_OS_OK != eRet)
        {
            CAM_POLL_ERR("%s : Create poll thread fail\n\r", __FUNCTION__);
            break;
        }
        CamOsThreadSetName(thread, "CamDevWrp");
    }

    CamOsMutexLock(&_gThdPoolMutex);
    _gNumPoolEntry += i;

    CAM_POLL_DBG(POLL_DBG_LV_0, "%s : add %d, total=%d \n\r", __FUNCTION__, i, _gNumPoolEntry);
    CamOsMutexUnlock(&_gThdPoolMutex);

    return (i > 0) ? CAM_OS_OK : CAM_OS_FAIL;
}

static int _PollThreadPoolAdd(u32 nTid)
{
    int                     i;
    CamDevPollThreadPool_t *pPool;

    while (1)
    {
        CamOsMutexLock(&_gThdPoolMutex);
        for (i = 0; i < _gNumPoolEntry; i++)
        {
            pPool = &_gThdPoolTable[i];
            if (-1 == pPool->nPollThdId)
            {
                pPool->nPollThdId = nTid;
                break;
            }
        }
        CamOsMutexUnlock(&_gThdPoolMutex);

        if (i < _gNumPoolEntry) /* found */
        {
            // CamOsPrintf("%s[%d] : add to %d-entry\n\r", __FUNCTION__, nTid, i);
            return i;
        }
        else
        {
            CAM_POLL_ERR("%s[%d] : pool full \n\r", __FUNCTION__, nTid);
            return -1;
        }
    }
}

// return: -1 if out of resource; otherwise: index of the entry
static int _CamDevPollThreadPoolAlloc(CamDevPollNode_t *pNode)
{
    int                     i;
    int                     nStart = 0;
    CamDevPollThreadPool_t *pPool  = NULL;

    while (1)
    {
        CamOsMutexLock(&_gThdPoolMutex);
        for (i = nStart; i < _gNumPoolEntry; i++)
        {
            pPool = &_gThdPoolTable[i];

            if ((-1 != pPool->nPollThdId) && !pPool->nUsed)
            {
                /* associate with a poolThd and not alloc to any poll */
                pPool->nUsed     = 1;
                pPool->pUserData = (void *)pNode;
                break;
            }
        }
        CamOsMutexUnlock(&_gThdPoolMutex);

        if (pPool && (i < _gNumPoolEntry)) /* found */
        {
            CamOsMutexLock(&_gPollMutex);
            pNode->nPoolIndex = i;
            CAM_POLL_DBG(POLL_DBG_LV_3, "%s[pool:%d] : alloc tid=%d \n\r", __FUNCTION__, i, pPool->nPollThdId);
            CamOsMutexUnlock(&_gPollMutex);

            CamOsTsemUp(&pPool->tActiveTsem);
            return i;
        }
        else
        {
            if (CAM_OS_OK != _DynPollThreadPoolCreate())
            {
                return -1;
            }
            CamOsMsSleep(100); /* Fix me: wait until the thread initialized */
            nStart = i;
        }
    }

    return -1;
}

static CamOsRet_e _CamDevPollThreadPoolWakeup(int nPoolIndex, int nTimeout)
{
    // TODO... check sync issue
    if (!_gThdPoolTable[nPoolIndex].nSuspend)
    {
        CAM_POLL_DBG(POLL_DBG_LV_1, "%s[pool:%d] : already running(timeout=%d)\n\r", __FUNCTION__, nPoolIndex,
                     nTimeout);
        return CAM_OS_OK;
    }

    CamOsTsemUp(&_gThdPoolTable[nPoolIndex].tActiveTsem);

    return CAM_OS_OK;
}

static int _CamDevPollThreadPoolFree(int nPoolIndex)
{
    CamDevPollThreadPool_t *pPool;

    CamOsMutexLock(&_gThdPoolMutex);
    pPool = &_gThdPoolTable[nPoolIndex];
    if ((nPoolIndex < _gNumPoolEntry) && pPool->nUsed)
    {
        pPool->nUsed     = 0;
        pPool->pUserData = NULL;
    }
    CAM_POLL_DBG(POLL_DBG_LV_1, "%s[pool:%d] used=%d, tid=%d\n\r", __FUNCTION__, nPoolIndex, pPool->nUsed,
                 pPool->nPollThdId);
    CamOsMutexUnlock(&_gThdPoolMutex);

    return CAM_OS_OK;
}

/*=============================================================================
 *
 *  poll Table Management
 *
 *===========================================================================*/
static CamOsRet_e _CamDevPollInit()
{
    if (!_gInitPollDone)
    {
        _gInitPollDone = 1;

        if (CAM_OS_OK != CamOsMutexInit(&_gPollMutex))
        {
            CAM_POLL_ERR("%s : Init _gPollMutex fail\n\r", __FUNCTION__);
            _gInitPollDone = 0;
            return CAM_OS_FAIL;
        }

#if (!_WITH_SYSTEM_THREAD)
        if (CAM_OS_OK != _DynPollThreadPoolCreate())
        {
            return CAM_OS_FAIL;
        }
#endif
    }

    return CAM_OS_OK;
}

#if 0
static CamOsRet_e _CamDevPollDeInit()
{
    CamOsMutexDestroy(&_gPollMutex);
    CamOsMutexDestroy(&_gThdPoolMutex);

    _gInitPollDone = 0;

    _gInitPoolMutexDone = 0;
    _gInitThdPoolDone = 0;
    /* destroy task */

    return CAM_OS_OK;
}
#endif

static CamDevPollNode_t *_CamDevPollNodeAlloc(u32 nTid, int nFd, short nEvents, int nTimeout, CamDevFD_t *pFdNode,
                                              Ms_Flag_t *pFlag, Ms_flag_value_t tFlagBit)
{
    int               i;
    CamDevPollNode_t *pNode = NULL;

    CamOsMutexLock(&_gPollMutex);

    for (i = 0; i < MAX_POLL_ENTRY_NUM; i++)
    {
        pNode = &(_gPollTable[i]);

        if (!pNode->nValid)
        {
            pNode->nTid   = nTid;
            pNode->nDevFD = nFd;

            pNode->pfnDrvPoll = pFdNode->pDevMapFunc->pPoll;
            pNode->nDrvFD     = pFdNode->nDrvFD;
            pNode->nEvents    = nEvents;
            pNode->nRevents   = 0;
            pNode->nTimeout   = nTimeout;

            pNode->pFlag    = pFlag;
            pNode->tFlagBit = tFlagBit;
            pNode->nValid   = 1;
            break;
        }
    }

    CamOsMutexUnlock(&_gPollMutex);
    CAM_POLL_DBG(POLL_DBG_LV_3, "%s[tid=%d,fd=%d,flagBit%x] : done\n\r", __FUNCTION__, nTid, nFd, tFlagBit);
    return pNode;
}

/* free all the node with nDevFD==nFd */
static void _CamDevPollNodeFree(int nFd)
{
    int               i;
    CamDevPollNode_t *pNode = NULL;
    Ms_Flag_t *       pFlag = NULL;

    CamOsMutexLock(&_gPollMutex);

    for (i = 0; i < MAX_POLL_ENTRY_NUM; i++)
    {
        pNode = &(_gPollTable[i]);

        if (pNode->nValid && pNode->nDevFD == nFd)
        {
            pNode->nValid = 0;
            if (pNode->pFlag)
            {
                pFlag = pNode->pFlag;
                CamOsMemRelease((void *)pNode->pFlag);
                pNode->pFlag = NULL;
            }
            _CamDevPollThreadPoolFree(pNode->nPoolIndex);
            CAM_POLL_DBG(POLL_DBG_LV_3, "%s[fd=%d,pool:%d] : \n\r", __FUNCTION__, nFd, pNode->nPoolIndex);
            break;
        }
    }

    if (pFlag)
    {
        for (i = 0; i < MAX_POLL_ENTRY_NUM; i++)
        {
            pNode = &(_gPollTable[i]);

            if (pNode->nValid && pNode->pFlag == pFlag)
            {
                pNode->pFlag = NULL;
            }
        }
    }

    CamOsMutexUnlock(&_gPollMutex);
}

/* nFd<0:  don't care fd */
static CamDevPollNode_t *_CamDevPollNodeFind(u32 nTid, int nFd)
{
    int               i;
    CamDevPollNode_t *pNode;

    CamOsMutexLock(&_gPollMutex);

    for (i = 0; i < MAX_POLL_ENTRY_NUM; i++)
    {
        pNode = &(_gPollTable[i]);

        if (pNode->nValid && pNode->nTid == nTid && (nFd < 0 || pNode->nDevFD == nFd))
        {
            CamOsMutexUnlock(&_gPollMutex);
            return pNode;
        }
    }

    CamOsMutexUnlock(&_gPollMutex);

    CAM_POLL_DBG(POLL_DBG_LV_3, "%s : done\n\r", __FUNCTION__);
    return NULL;
}

static CamDevPollNode_t *_CamDevPollNodeFindAndUpdate(u32 nTid, int nFd, short nEvents)
{
    int               i;
    CamDevPollNode_t *pNode;

    CamOsMutexLock(&_gPollMutex);

    for (i = 0; i < MAX_POLL_ENTRY_NUM; i++)
    {
        pNode = &(_gPollTable[i]);

        if (pNode->nValid && pNode->nTid == nTid && pNode->nDevFD == nFd)
        {
            pNode->nEvents = nEvents;
            CamOsMutexUnlock(&_gPollMutex);
            return pNode;
        }
    }

    CamOsMutexUnlock(&_gPollMutex);

    CAM_POLL_DBG(POLL_DBG_LV_3, "%s : done\n\r", __FUNCTION__);
    return NULL;
}

static Ms_Flag_t *_CamDevPollGetOrAllocFlag(u32 nTid)
{
    Ms_Flag_t *       pFlag = NULL;
    CamDevPollNode_t *pNode = _CamDevPollNodeFind(nTid, -1);

    /* PollTable[tid] exist? */
    if (NULL == pNode)
    {
        if (!(pFlag = (Ms_Flag_t *)CamOsMemCalloc(1, sizeof(Ms_Flag_t))))
        {
            CAM_POLL_ERR("%s : Allocate flag fail\n\r", __FUNCTION__);
            return NULL;
        }

        memset(pFlag, 0, sizeof(Ms_Flag_t));
        MsFlagInit(pFlag);
        CAM_POLL_DBG(POLL_DBG_LV_3, "%s[tid:%d] : alloc flag=%x \n\r", __FUNCTION__, nTid, pFlag);
    }
    else
    {
        pFlag = pNode->pFlag;
        CAM_POLL_DBG(POLL_DBG_LV_3, "%s[tid:%d] : found flag=%x \n\r", __FUNCTION__, nTid, pFlag);
    }
    return pFlag;
}

static int _CamDevPollNodeGetAndClearReventsAndFlag(u32 nTid, int nFd, short nEventsAskedfor)
{
    int               nRevents = 0;
    CamDevPollNode_t *pNode    = _CamDevPollNodeFind(nTid, nFd);

    if (NULL == pNode)
    {
        CAM_POLL_DBG(POLL_DBG_LV_3, "%s[tid=%d, devfd=%d] : not found\n\r", __FUNCTION__, nTid, nFd);
        return 0;
    }
    else
    {
        CamOsMutexLock(&_gPollMutex); //????
        nRevents        = pNode->nRevents & nEventsAskedfor;
        pNode->nRevents = 0;
        MsFlagMaskbits(pNode->pFlag, pNode->tFlagBit);
        CamOsMutexUnlock(&_gPollMutex);
        CAM_POLL_DBG(POLL_DBG_LV_3, "%s[tid=%d, devfd=%d] : revents=%d (tblEvent=0x%x)\n\r", __FUNCTION__, nTid, nFd,
                     nRevents, pNode->nRevents);
        return nRevents;
    }
}

/*=============================================================================
 *
 *  CamDevPollThread: Each fd-poll serviced by a CamDevPollThread.
 *                    CamDevPollThread will call driver poll function.
 *
 *===========================================================================*/
void CamDevPollThread(void)
{
    u32 nTid;
    int i;
    /* int nRet; */
    CamDevPollNode_t *      pNode;
    CamDevPollThreadPool_t *pPoolNode;
    short                   nRevents;

    _CamDevPollThreadPoolInit();

    nTid = CamOsThreadGetID();
    i    = _PollThreadPoolAdd(nTid);
    if (i < 0)
    {
        CAM_DEV_POLL_PANIC();
    }

    pPoolNode = &_gThdPoolTable[i];

    while (1)
    {
        CamOsMutexLock(&_gThdPoolMutex);
        pPoolNode->nSuspend = 1;
        CamOsMutexUnlock(&_gThdPoolMutex);

        CAM_POLL_DBG(POLL_DBG_LV_2, "%s[%d, pool:%d] : suspend \n\r", __FUNCTION__, nTid, i);
        CamOsTsemDown(&pPoolNode->tActiveTsem);
        CAM_POLL_DBG(POLL_DBG_LV_2, "%s[%d, pool:%d] : running \n\r", __FUNCTION__, nTid, i);
        CamOsMutexLock(&_gThdPoolMutex);
        pPoolNode->nSuspend = 0;
        CamOsMutexUnlock(&_gThdPoolMutex);

        if (pPoolNode->nUsed)
        {
            pNode = (CamDevPollNode_t *)pPoolNode->pUserData;
            if (NULL == pNode)
            {
                CAM_POLL_ERR("%s[%d] : userdata NULL \n\r", __FUNCTION__, i);
                continue;
            }
            /* nRet = */ (*pNode->pfnDrvPoll)(pNode->nDrvFD, pNode->nEvents, &nRevents, pNode->nTimeout);
            /* if (pNode->nRevents != 0) : we have to return timeout status. So, don't do this. */
            {
                CAM_POLL_DBG(POLL_DBG_LV_0, "%s[pool:%d;tid:%d] : fd=dev:%d. drv:%d event(%d) wakeup bit %x \n\r",
                             __FUNCTION__, i, nTid, pNode->nDevFD, pNode->nDrvFD, pNode->nRevents, pNode->tFlagBit);

                CamOsMutexLock(&_gPollMutex);
                pNode->nRevents = nRevents;
                MsFlagSetbits(pNode->pFlag, pNode->tFlagBit);
                CamOsMutexUnlock(&_gPollMutex);
            }
        }
    }
}

/*=============================================================================
 *
 *  CamDevPoll:     the API emulating linux poll() function
 *
 *===========================================================================*/
int CamDevPoll(struct pollfd *tFds, int nFds, int nTimeout)
{
    int i;
    int nRetfd = 0;

    Ms_Flag_t *       pFlag;
    CamDevPollNode_t *pNode;
    int               nPoolIndex;
    Ms_flag_value_t   tFlagBit, tFlagBitAll = 0;

    u32         nTid                         = CamOsThreadGetID();
    CamDevFD_t *pFdNode[MAX_POLL_FD_A_BATCH] = {NULL};

    int nRetFd;

    CAM_POLL_DBG(POLL_DBG_LV_1, "%s[%d] : in\n\r", __FUNCTION__, nTid);

    /* 0: CamDevPollInit */
    if (CAM_OS_OK != _CamDevPollInit())
    {
        return -1;
    }

    /*----------------- poll with single fd -----------------------*/
#if 1
    if (1 == nFds)
    {
        nRetFd = -1;
        if (NULL == (pFdNode[0] = _CamDevFindFD(tFds->fd)))
        {
            CAM_POLL_ERR("%s : %d find fd fail %d\n\r", __FUNCTION__, tFds->fd);
            return -1;
        }
        CAM_POLL_DBG(POLL_DBG_LV_1, "%s[devfd=%d,drvfd=%d] : call _CamDevFindFD %s done %x\n\r", __FUNCTION__, tFds->fd,
                     pFdNode[0]->nDrvFD, pFdNode[0]->pDevMapFunc->szName, pFdNode[0]->pDevMapFunc->pPoll);

        if (NULL == pFdNode[0]->pDevMapFunc->pPoll)
        {
            CAM_POLL_ERR("%s : no support poll\n\r", pFdNode[0]->pDevMapFunc->szName);
            return -1;
        }

        nRetFd = (*pFdNode[0]->pDevMapFunc->pPoll)(pFdNode[0]->nDrvFD, tFds->events, &tFds->revents, nTimeout);

        CAM_POLL_DBG(POLL_DBG_LV_0, "%s[tid=%d,nfd=1] : nRetfd=%d\n\r", __FUNCTION__, nTid, nRetfd);
        return nRetFd;
    }
#endif

    /*--------------- poll with multiple fds ----------------*/

    for (i = 0; i < nFds; i++)
    {
        pFdNode[i] = _CamDevFindFD(tFds[i].fd);
        if (NULL == pFdNode[i])
        {
            return -1;
        }

        if (NULL == pFdNode[i]->pDevMapFunc->pPoll)
        {
            CAM_POLL_ERR("%s : no support poll\n\r", pFdNode[0]->pDevMapFunc->szName);
            return -1;
        }
    }

    /* 1: get revents from poll table, return if there are some events */
    for (i = 0; i < nFds; i++)
    {
        tFds[i].revents = _CamDevPollNodeGetAndClearReventsAndFlag(nTid, tFds[i].fd,
                                                                   tFds[i].events); /* fix me: convert to driverFD */
        if (0 != tFds[i].revents)
        {
            nRetfd++;
        }
    }
    if (nRetfd > 0)
    {
        return nRetfd;
    }

    /* 2: Create / resume thread */

    /* PollTable[nTid] exist? */
    pFlag = _CamDevPollGetOrAllocFlag(nTid);
    if (NULL == pFlag)
    {
        return -1;
    }

    for (i = 0; i < nFds; i++)
    {
        tFlagBit = (1 << i);
        tFlagBitAll |= tFlagBit;

        /* if exist, resume PollThread */
        pNode = _CamDevPollNodeFindAndUpdate(nTid, tFds[i].fd, tFds[i].events);
        if (NULL != pNode)
        {
            CamOsMutexLock(&_gPollMutex);
            nPoolIndex = pNode->nPoolIndex;
            CamOsMutexUnlock(&_gPollMutex);
            _CamDevPollThreadPoolWakeup(nPoolIndex, nTimeout);
        }
        /* if not exist, addPollWaitJob and create pollThread */
        else
        {
            pNode      = _CamDevPollNodeAlloc(nTid, tFds[i].fd, tFds[i].events, nTimeout, pFdNode[i], pFlag, tFlagBit);
            nPoolIndex = _CamDevPollThreadPoolAlloc(pNode);
            if (nPoolIndex < 0)
            {
                CAM_POLL_ERR("%s[%d,%d] : _CamDevPollThreadCreate failed \n\r", __FUNCTION__, nTid, tFds[i].fd);
                return -1; // Fix me: error handling
            }
        }
    }

    CAM_POLL_DBG(POLL_DBG_LV_1, "%s : MsFlagTimedWait waiting\n\r", __FUNCTION__);
    if (nTimeout >= 0)
    {
        tFlagBit = MsFlagTimedWait(pFlag, tFlagBitAll, MS_FLAG_WAITMODE_OR, RTK_MS_TO_TICK(nTimeout));
    }
    else
    {
        tFlagBit = MsFlagWait(pFlag, tFlagBitAll, MS_FLAG_WAITMODE_OR | MS_FLAG_WAITMODE_CLR);
    }

    CAM_POLL_DBG(POLL_DBG_LV_0, "%s : MsFlagWait wakeup  %d\n\r", __FUNCTION__, tFlagBit);
    if (0 == tFlagBit) // timeout
    {
        CAM_POLL_DBG(POLL_DBG_LV_0, "%s[%d] : timeout\n\r", __FUNCTION__, nTid);
        return 0;
    }
    else
    {
        for (i = 0; i < nFds; i++)
        {
            // if (tFlagBit & (1<<i))
            {
                tFds[i].revents = _CamDevPollNodeGetAndClearReventsAndFlag(nTid, tFds[i].fd, tFds[i].events);
                if (0 != tFds[i].revents)
                {
                    CAM_POLL_DBG(POLL_DBG_LV_0, "%s[%d,fd=%d] : events(%d)\n\r", __FUNCTION__, nTid, tFds[i].fd,
                                 tFds[i].revents);
                    nRetfd++;
                }
            }
        }
        CAM_POLL_DBG(POLL_DBG_LV_0, "%s[%d] : nRetfd=%d\n\r", __FUNCTION__, nTid, nRetfd);
    }
    return nRetfd;
}

#endif // CAM_OS_RTK
