/*
 * cam_os_wrapper_test.c
 *
 *  Created on: Mar 20, 2017
 *      Author: giggs.huang
 */

#include <string.h>
#include <stdlib.h>
#include "sys_sys_core.h"
#include "sys_sys_isw_cli.h"
#include "cam_os_wrapper.h"
#include <time.h>

static u32 _gMutexTestCnt = 0;

static void _TestCamOsThread(void);
static void _TestCamOsMutex(void);
static void _TestCamOsDmem(void);
static void _TestCamOsMem(void);
static void _TestCamOsTimer(void);
static void _TestCamOsTsem(void);
static void _TestCamOsDiv64(void);
static void _TestCamOsSystemTime(void);
static void _TestCamOsPhysMemSize(void);
static void _TestCamOsChipId(void);

s32 CamOsWrapperTest(CLI_t *pCli, char *p)
{
    s32 i, nParamCnt, nRet = -1;
    u32  nCaseNum = 0;
    char *pEnd;


    nParamCnt = CliTokenCount(pCli);

    if(nParamCnt < 1)
    {
        return eCLI_PARSE_INPUT_ERROR;
    }

    for(i = 0; i < nParamCnt; i++)
    {
        pCli->tokenLvl++;
        p = CliTokenPop(pCli);
        if(i == 0)
        {
            //CLIDEBUG(("p: %s, len: %d\n", p, strlen(p)));
            //*pV = _strtoul(p, &pEnd, base);
            nCaseNum = strtoul(p, &pEnd, 10);
            //CLIDEBUG(("*pEnd = %d\n", *pEnd));
            if(p == pEnd || *pEnd)
            {
                CamOsDebug("Invalid input\n");
                return eCLI_PARSE_ERROR;
            }
        }
    }

    switch(nCaseNum)
    {
        case 1:
            _TestCamOsThread();
            break;
        case 2:
            _TestCamOsMutex();
            break;
        case 3:
            _TestCamOsDmem();
            break;
        case 4:
            _TestCamOsMem();
            break;
        case 5:
            _TestCamOsTimer();
            break;
        case 6:
            _TestCamOsTsem();
            break;
        case 7:
            _TestCamOsDiv64();
            break;
        case 8:
            _TestCamOsSystemTime();
            break;
        case 9:
            _TestCamOsPhysMemSize();
            break;
        case 10:
            _TestCamOsChipId();
            break;
        default:
            nRet = -1;
    }

    if(nRet < 0)
        return eCLI_PARSE_ERROR;

    return eCLI_PARSE_OK;
}

static void _CamOsThreadTestEntry0(void *pUserdata)
{
    s32 *pnArg = (s32 *)pUserdata;
    s32 i = 0;

    for(i = 0; i < 5; i++)
    {
        CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] _CamOsThreadTestEntry0 (sleep %dms)  count: %d\n", __LINE__, *pnArg, i);
        CamOsMsSleep(*pnArg);
    }
}

static void _TestCamOsThread(void)
{
    CamOsThread TaskHandle0, TaskHandle1;
    CamOsThreadAttrb_t tAttr;
    s32 nTaskArg0 = 1000, nTaskArg1 = 1500;

    tAttr.nPriority = 50;
    tAttr.nStackSize = 0;
    CamOsThreadCreate(&TaskHandle0, &tAttr, (void *)_CamOsThreadTestEntry0, (void *)&nTaskArg0);
    //CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] _TestCamOsThread get taskid: %d\n", __LINE__, TaskHandle0.eHandleObj);

    tAttr.nPriority = 50;
    tAttr.nStackSize = 0;
    CamOsThreadCreate(&TaskHandle1, &tAttr, (void *)_CamOsThreadTestEntry0, (void *)&nTaskArg1);
    //CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] _TestCamOsThread get taskid: %d\n", __LINE__, TaskHandle1.eHandleObj);

    CamOsThreadJoin(TaskHandle0);
    CamOsThreadJoin(TaskHandle1);

    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] _TestCamOsThread delete task\n", __LINE__);
    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] _TestCamOsThread test end!!!\n", __LINE__);
}

static void _CamOsMutexTestEntry0(void *pUserdata)
{
    CamOsMutex_t *ptMutex = (CamOsMutex_t *)pUserdata;
    u32 i = 0;

    for(i = 0; i < 100; i++)
    {
        CamOsMutexLock(ptMutex);
        _gMutexTestCnt++;
        CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] _CamOsThreadTestEntry0 start  count: %d\n", __LINE__, _gMutexTestCnt);
        CamOsMsSleep(3);
        CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] _CamOsThreadTestEntry0 end    count: %d%s\n", __LINE__, _gMutexTestCnt);
        CamOsMutexUnlock(ptMutex);
    }
}

static void _CamOsMutexTestEntry1(void *pUserdata)
{
    CamOsMutex_t *ptMutex = (CamOsMutex_t *)pUserdata;
    u32 i = 0;

    for(i = 0; i < 100; i++)
    {
        CamOsMutexLock(ptMutex);
        _gMutexTestCnt++;
        CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] _CamOsThreadTestEntry1 start  count: %d\n", __LINE__, _gMutexTestCnt);
        CamOsMsSleep(2);
        CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] _CamOsThreadTestEntry1 end    count: %d\n", __LINE__, _gMutexTestCnt);
        CamOsMutexUnlock(ptMutex);
    }
}

static void _CamOsMutexTestEntry2(void *pUserdata)
{
    CamOsMutex_t *ptMutex = (CamOsMutex_t *)pUserdata;
    u32 i = 0;

    for(i = 0; i < 100; i++)
    {
        CamOsMutexLock(ptMutex);
        _gMutexTestCnt++;
        CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] _CamOsThreadTestEntry2 start  count: %d\n", __LINE__, _gMutexTestCnt);
        CamOsMsSleep(5);
        CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] _CamOsThreadTestEntry2 end    count: %d\n", __LINE__, _gMutexTestCnt);
        CamOsMutexUnlock(ptMutex);
    }
}

static void _TestCamOsMutex(void)
{
    CamOsThread TaskHandle0, TaskHandle1, TaskHandle2;
    CamOsThreadAttrb_t tAttr;
    CamOsMutex_t tCamOsMutex;

    CamOsMutexInit(&tCamOsMutex);

    tAttr.nPriority = 50;
    tAttr.nStackSize = 0;
    CamOsThreadCreate(&TaskHandle0, &tAttr, (void *)_CamOsMutexTestEntry0, &tCamOsMutex);

    tAttr.nPriority = 50;
    tAttr.nStackSize = 0;
    CamOsThreadCreate(&TaskHandle1, &tAttr, (void *)_CamOsMutexTestEntry1, &tCamOsMutex);

    tAttr.nPriority = 100;
    tAttr.nStackSize = 0;
    CamOsThreadCreate(&TaskHandle2, &tAttr, (void *)_CamOsMutexTestEntry2, &tCamOsMutex);

    CamOsThreadJoin(TaskHandle0);
    CamOsThreadJoin(TaskHandle1);
    CamOsThreadJoin(TaskHandle2);

    CamOsMutexDestroy(&tCamOsMutex);

    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] _TestCamOsMutex delete task\n", __LINE__);
    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] _TestCamOsMutex test end!!!\n", __LINE__);
}

static void _TestCamOsDmem(void)
{
    void *pUserPtr = NULL;
    u64 nMiuAddr = 0;
    u64 nPhysAddr = 0;

    CamOsDirectMemAlloc("TESTDMEM", 1025, &pUserPtr, &nMiuAddr, &nPhysAddr);
    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] CamOsDirectMemAlloc get addr:  pUserPtr 0x%08X  nMiuAddr 0x%08X  nPhysAddr 0x%08X\n",
               __LINE__, (u32)pUserPtr, (u32)nMiuAddr, (u32)nPhysAddr);


    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] _TestCamOsDmem: write cached ptr=0x%x, value = 0x%x\n", __LINE__, (u32)nPhysAddr, 0x12345678);
    *(u32 *)(u32)nPhysAddr = 0x12345678;
    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] _TestCamOsDmem: read cached ptr=0x%x, value = 0x%x\n", __LINE__, (u32)nPhysAddr, *(u32 *)(u32)nPhysAddr);
    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] _TestCamOsDmem: read uncached ptr=0x%x, value = 0x%x\n", __LINE__, (u32)pUserPtr, *(u32 *)pUserPtr);

    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] _TestCamOsDmem: write uncached ptr=0x%x, value = 0x%x\n", __LINE__, (u32)pUserPtr, 0x87654321);
    *(u32 *)pUserPtr = 0x87654321;
    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] _TestCamOsDmem: read cached ptr=0x%x, value = 0x%x\n", __LINE__, (u32)nPhysAddr, *(u32 *)(u32)nPhysAddr);
    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] _TestCamOsDmem: read uncached ptr=0x%x, value = 0x%x\n", __LINE__, (u32)pUserPtr, *(u32 *)pUserPtr);

    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] _TestCamOsDmem: flush cached ptr=0x%x\n", __LINE__, (u32)nPhysAddr);
    sys_flush_data_cache_buffer((u32)nPhysAddr, 1025);
    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] _TestCamOsDmem: read cached ptr=0x%x, value = 0x%x\n", __LINE__, (u32)nPhysAddr, *(u32 *)(u32)nPhysAddr);
    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] _TestCamOsDmem: read uncached ptr=0x%x, value = 0x%x\n", __LINE__, (u32)pUserPtr, *(u32 *)pUserPtr);

    CamOsDirectMemRelease((u8 *)pUserPtr, 1025);

    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d]_TestCamOsDmem test end!!!\n", __LINE__);
}

static void _TestCamOsMem(void)
{
    void *pUserPtr = NULL;

    // test CamOsMemAlloc
    pUserPtr = CamOsMemAlloc(2048);
    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] CamOsMemAlloc get addr:  pUserPtr 0x%08X\n", __LINE__, (u32)pUserPtr);

    memset(pUserPtr, 0x5A, 2048);
    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] value in address 0x%08X is 0x%08X\n", __LINE__, (u32)pUserPtr, *(u32 *)pUserPtr);

    CamOsMemRelease(pUserPtr);
    pUserPtr = NULL;
    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] CamOsMemRelease free buffer\n", __LINE__);

    // test CamOsMemCalloc
    pUserPtr = CamOsMemCalloc(2048, 1);
    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] CamOsMemCalloc get addr:  pUserPtr 0x%08X\n", __LINE__, (u32)pUserPtr);

    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] value in address 0x%08X is 0x%08X\n", __LINE__, (u32)pUserPtr, *(u32 *)pUserPtr);

    CamOsMemRelease(pUserPtr);
    pUserPtr = NULL;
    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] CamOsMemRelease free buffer\n", __LINE__);

    // test CamOsMemRealloc
    pUserPtr = CamOsMemAlloc(2048);
    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] CAM_OS_MemMalloc get addr:  pUserPtr 0x%08X\n", __LINE__, (u32)pUserPtr);

    pUserPtr = CamOsMemRealloc(pUserPtr, 4096);
    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] CamOsMemRealloc get addr:  pUserPtr 0x%08X\n", __LINE__, (u32)pUserPtr);

    CamOsMemRelease(pUserPtr);
    pUserPtr = NULL;
    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] CamOsMemRelease free buffer\n", __LINE__);

    CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] _TestCamOsMem test end!!!\n", __LINE__);
}

static void _TestCamOsTimer(void)
{
    s32 nCnt = 0;
    CamOsTimespec_t tTv = {0}, tTv_prev = {0};

    for(nCnt = 0; nCnt <= 10; nCnt++)
    {
        CamOsGetMonotonicTime(&tTv);
        CamOsDebug("[CAM_OS_WRAPPER_TEST:%d] rtk_os_time: %d.%09d    diff: %d\n", __LINE__, tTv.nSec, tTv.nNanoSec,
                   (tTv.nSec - tTv_prev.nSec)*1000000+((s32)tTv.nNanoSec - (s32)tTv_prev.nNanoSec)/1000);
        memcpy((void *)&tTv_prev, (void *)&tTv, sizeof(tTv));
        CamOsMsSleep(1000);
    }
}

static s32 _CamOsTsemTestEntry0(void *pArg)
{
    CamOsTsem_t *pSem = (CamOsTsem_t *)pArg;
    CamOsRet_e nRet;

    CamOsMsSleep(3000);

    CamOsDebug("%s TsemUp\n", __FUNCTION__);
    CamOsTsemUp(pSem);

    CamOsDebug("%s TsemWait start\n", __FUNCTION__);
    CamOsTsemWait(pSem);
    CamOsDebug("%s TsemWait end\n", __FUNCTION__);

    CamOsDebug("%s TsemUp\n", __FUNCTION__);
    CamOsTsemUp(pSem);
    CamOsDebug("%s TsemUp\n", __FUNCTION__);
    CamOsTsemUp(pSem);
    CamOsDebug("%s TsemUp\n", __FUNCTION__);
    CamOsTsemUp(pSem);
    CamOsDebug("%s TsemValue: %d\n", __FUNCTION__, CamOsTsemGetValue(pSem));

    CamOsMsSleep(14000);

    CamOsDebug("%s TsemUp\n", __FUNCTION__);
    CamOsTsemUp(pSem);

    CamOsMsSleep(1000);

    CamOsDebug("%s TsemTimedWait start\n", __FUNCTION__);
    nRet = CamOsTsemTimedWait(pSem, 3000);
    CamOsDebug("%s TsemTimedWait end (%s)\n", __FUNCTION__, (nRet == CAM_OS_OK) ? "wakened" : "timeout");

    CamOsDebug("%s TsemTimedWait start\n", __FUNCTION__);
    nRet = CamOsTsemTimedWait(pSem, 10000);
    CamOsDebug("%s TsemTimedWait end (%s)\n", __FUNCTION__, (nRet == CAM_OS_OK) ? "wakened" : "timeout");


    CamOsDebug("%s break\n", __FUNCTION__);

    return 0;
}

static s32 _CamOsTsemTestEntry1(void *pArg)
{
    CamOsTsem_t *pSem = (CamOsTsem_t *)pArg;
    CamOsRet_e nRet;

    CamOsDebug("%s TsemDown start\n", __FUNCTION__);
    CamOsTsemDown(pSem);
    CamOsDebug("%s TsemDown end\n", __FUNCTION__);

    CamOsMsSleep(3000);

    CamOsDebug("%s TsemSignal\n", __FUNCTION__);
    CamOsTsemSignal(pSem);

    CamOsMsSleep(3000);

    CamOsDebug("%s TsemDown\n", __FUNCTION__);
    CamOsTsemDown(pSem);
    CamOsDebug("%s TsemDown\n", __FUNCTION__);
    CamOsTsemDown(pSem);
    CamOsDebug("%s TsemDown\n", __FUNCTION__);
    CamOsTsemDown(pSem);
    CamOsDebug("%s TsemValue: %d\n", __FUNCTION__, CamOsTsemGetValue(pSem));

    CamOsDebug("%s TsemUp\n", __FUNCTION__);
    CamOsTsemUp(pSem);
    CamOsDebug("%s TsemUp\n", __FUNCTION__);
    CamOsTsemUp(pSem);
    CamOsDebug("%s TsemValue: %d\n", __FUNCTION__, CamOsTsemGetValue(pSem));
    CamOsDebug("%s TsemReset\n", __FUNCTION__);
    CamOsTsemReset(pSem);
    CamOsDebug("%s TsemValue: %d\n", __FUNCTION__, CamOsTsemGetValue(pSem));

    CamOsMsSleep(3000);

    CamOsDebug("%s TsemTimedDown start\n", __FUNCTION__);
    nRet = CamOsTsemTimedDown(pSem, 3000);
    CamOsDebug("%s TsemTimedDown end (%s)\n", __FUNCTION__, (nRet == CAM_OS_OK) ? "wakened" : "timeout");

    CamOsDebug("%s TsemTimedDown start\n", __FUNCTION__);
    nRet = CamOsTsemTimedDown(pSem, 10000);
    CamOsDebug("%s TsemTimedDown end (%s)\n", __FUNCTION__, (nRet == CAM_OS_OK) ? "wakened" : "timeout");

    CamOsMsSleep(6000);

    CamOsDebug("%s TsemSignal\n", __FUNCTION__);
    CamOsTsemSignal(pSem);


    CamOsDebug("%s break\n", __FUNCTION__);

    return 0;
}

static void _TestCamOsTsem(void)
{
    static CamOsThread TaskHandle0, TaskHandle1;
    CamOsTsem_t tSem;

    CamOsTsemInit(&tSem, 0);
    CamOsThreadCreate(&TaskHandle0, NULL, (void *)_CamOsTsemTestEntry0, &tSem);
    CamOsThreadCreate(&TaskHandle1, NULL, (void *)_CamOsTsemTestEntry1, &tSem);

    CamOsThreadJoin(TaskHandle0);
    CamOsThreadJoin(TaskHandle1);
}

static void _TestCamOsDiv64(void)
{
    u64 nDividendU64 = 0, nDivisorU64 = 0, nResultU64 = 0, nRemainderU64 = 0;
    s64 nDividendS64 = 0, nDivisorS64 = 0, nResultS64 = 0, nRemainderS64 = 0;

    CamOsDebug("Unsigned 64 bit dividend:");
    CamOsScanf("%llu", &nDividendU64);
    CamOsDebug("Unsigned 64 bit divisor:");
    CamOsScanf("%llu", &nDivisorU64);

    CamOsDebug("Directly: %llu / %llu = %llu    remaind %llu\n", nDividendU64, nDivisorU64, nDividendU64 / nDivisorU64, nDividendU64 % nDivisorU64);
    nResultU64 = CamOsMathDivU64(nDividendU64, nDivisorU64, &nRemainderU64);
    CamOsDebug("By Div64: %llu / %llu = %llu    remaind %llu\n", nDividendU64, nDivisorU64, nResultU64, nRemainderU64);


    CamOsDebug("Signed 64 bit dividend:");
    CamOsScanf("%lld", &nDividendS64);
    CamOsDebug("Signed 64 bit divisor:");
    CamOsScanf("%lld", &nDivisorS64);

    CamOsDebug("Directly: %lld / %lld = %lld    remaind %lld\n", nDividendS64, nDivisorS64, nDividendS64 / nDivisorS64, nDividendS64 % nDivisorS64);
    nResultS64 = CamOsMathDivS64(nDividendS64, nDivisorS64, &nRemainderS64);
    CamOsDebug("By Div64: %lld / %lld = %lld    remaind %lld\n", nDividendS64, nDivisorS64, nResultS64, nRemainderS64);
}

static void _TestCamOsSystemTime(void)
{
    s32 nCnt = 0;
    CamOsTimespec_t tTs;
    struct tm * tTm;

    for(nCnt = 0; nCnt < 10; nCnt++)
    {
        CamOsGetTimeOfDay(&tTs);
        tTm = localtime ((time_t *)&tTs.nSec);
        CamOsDebug("RawSecond: %d  ->  %d/%02d/%02d [%d]  %02d:%02d:%02d\n",
                   tTs.nSec,
                   tTm->tm_year+1900,
                   tTm->tm_mon+1,
                   tTm->tm_mday,
                   tTm->tm_wday,
                   tTm->tm_hour,
                   tTm->tm_min,
                   tTm->tm_sec);

        tTs.nSec += 90000;
        CamOsSetTimeOfDay(&tTs);

        CamOsMsSleep(3000);
    }

    for(nCnt = 0; nCnt < 10; nCnt++)
    {
        CamOsGetTimeOfDay(&tTs);
        tTm = localtime ((time_t *)&tTs.nSec);
        CamOsDebug("RawSecond: %d  ->  %d/%02d/%02d [%d]  %02d:%02d:%02d\n",
                   tTs.nSec,
                   tTm->tm_year+1900,
                   tTm->tm_mon+1,
                   tTm->tm_mday,
                   tTm->tm_wday,
                   tTm->tm_hour,
                   tTm->tm_min,
                   tTm->tm_sec);

        tTs.nSec -= 90000;
        CamOsSetTimeOfDay(&tTs);

        CamOsMsSleep(3000);
    }
}

static void _TestCamOsPhysMemSize(void)
{
    CamOsMemSize_e eMemSize;
    eMemSize = CamOsPhysMemSize();
    CamOsDebug("System has %dMB physical memory\n", 1<<(u32)eMemSize);
}

static void _TestCamOsChipId(void)
{
    CamOsDebug("Chip ID: 0x%X\n", CamOsChipId());
}