/*
 * cam_os_wrapper_linux_kernel_test.c
 *
 *  Created on: Apr. 5, 2017
 *      Author: jeff-cl.lin
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include "cam_os_wrapper.h"
#include <linux/rtc.h>

MODULE_AUTHOR("MSTAR");
MODULE_DESCRIPTION("MStar Kernel Wrapper Test");
MODULE_LICENSE("GPL");

#define TEST_TIME_AND_SLEEP
#define TEST_DIRECT_MEMORY
#define TEST_SEMAPHORE
#define TEST_MUTEX
#define TEST_DIV64
#define TEST_SYSTEM_TIME
#define TEST_MEM_SIZE
#define TEST_CHIP_ID

#ifdef TEST_TIME_AND_SLEEP
static CamOsThread tThread0;
static int ThreadTestTimeMsleep(void *pArg)
{
    CamOsTimespec_t tTime;

    for(;;)
    {
        if(CAM_OS_OK == CamOsThreadShouldStop()) break;

        CamOsGetMonotonicTime(&tTime);
        CamOsDebug("%s  time: %lu.%06lu\n", __FUNCTION__, tTime.nSec, tTime.nNanoSec);
        CamOsMsSleep(1000);
    }
    CamOsDebug("%s break\n", __FUNCTION__);

    return 0;
}
#endif

#ifdef TEST_SEMAPHORE
static CamOsThread tThread1, tThread2;
CamOsTsem_t tSem;
static int ThreadTestTsem1(void *pArg)
{
    CamOsTsem_t *pSem = (CamOsTsem_t *)pArg;
    CamOsRet_e eRet;

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
    CamOsDebug("%s Tsem Value: %d\n", __FUNCTION__, CamOsTsemGetValue(pSem));

    CamOsMsSleep(14000);

    CamOsDebug("%s TsemUp\n", __FUNCTION__);
    CamOsTsemUp(pSem);

    CamOsMsSleep(1000);

    CamOsDebug("%s TsemTimedWait start\n", __FUNCTION__);
    eRet = CamOsTsemTimedWait(pSem, 3000);
    CamOsDebug("%s TsemTimedWait end (%s)\n", __FUNCTION__, (eRet == CAM_OS_OK) ? "wakened" : "timeout");

    CamOsDebug("%s TsemTimedWait start\n", __FUNCTION__);
    eRet = CamOsTsemTimedWait(pSem, 10000);
    CamOsDebug("%s TsemTimedWait end (%s)\n", __FUNCTION__, (eRet == CAM_OS_OK) ? "wakened" : "timeout");

    for(;;)
    {
        if(CAM_OS_OK == CamOsThreadShouldStop()) break;
        CamOsMsSleep(100);
    }
    CamOsDebug("%s break\n", __FUNCTION__);

    return 0;
}

static int ThreadTestTsem2(void *pArg)
{
    CamOsTsem_t *pSem = (CamOsTsem_t *)pArg;
    CamOsRet_e eRet;

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
    CamOsDebug("%s Tsem Value: %d\n", __FUNCTION__, CamOsTsemGetValue(pSem));

    CamOsDebug("%s TsemUp\n", __FUNCTION__);
    CamOsTsemUp(pSem);
    CamOsDebug("%s TsemUp\n", __FUNCTION__);
    CamOsTsemUp(pSem);
    CamOsDebug("%s Tsem Value: %d\n", __FUNCTION__, CamOsTsemGetValue(pSem));
    CamOsDebug("%s TsemReset\n", __FUNCTION__);
    CamOsTsemReset(pSem);
    CamOsDebug("%s Tsem Value: %d\n", __FUNCTION__, CamOsTsemGetValue(pSem));

    CamOsMsSleep(3000);

    CamOsDebug("%s TsemTimedDown start\n", __FUNCTION__);
    eRet = CamOsTsemTimedDown(pSem, 3000);
    CamOsDebug("%s TsemTimedDown end (%s)\n", __FUNCTION__, (eRet == CAM_OS_OK) ? "wakened" : "timeout");

    CamOsDebug("%s TsemTimedDown start\n", __FUNCTION__);
    eRet = CamOsTsemTimedDown(pSem, 10000);
    CamOsDebug("%s TsemTimedDown end (%s)\n", __FUNCTION__, (eRet == CAM_OS_OK) ? "wakened" : "timeout");

    CamOsMsSleep(6000);

    CamOsDebug("%s TsemSignal\n", __FUNCTION__);
    CamOsTsemSignal(pSem);

    for(;;)
    {
        if(CAM_OS_OK == CamOsThreadShouldStop()) break;
        CamOsMsSleep(100);
    }
    CamOsDebug("%s break\n", __FUNCTION__);

    return 0;
}
#endif

#ifdef TEST_MUTEX
CamOsThread tTaskHandle0, tTaskHandle1, tTaskHandle2;
CamOsThreadAttrb_t tAttr;
CamOsMutex_t tCamOsMutex;
static u32 nMutexTestCnt = 0;

static void CamOsMutexTestEntry0(void *pUserData)
{
    CamOsMutex_t *tpMutex = (CamOsMutex_t *)pUserData;
    unsigned int i = 0;

    for(i = 0; i < 100; i++)
    {
        CamOsMutexLock(tpMutex);
        nMutexTestCnt++;
        CamOsDebug("%s start count: %d\n\r", __FUNCTION__, nMutexTestCnt);
        CamOsMsSleep(2);
        CamOsDebug("%s end count: %d\n\r", __FUNCTION__, nMutexTestCnt);
        CamOsMutexUnlock(tpMutex);
    }
}

static void CamOsMutexTestEntry1(void *pUserData)
{
    CamOsMutex_t *tpMutex = (CamOsMutex_t *)pUserData;
    unsigned int i = 0;

    for(i = 0; i < 100; i++)
    {
        CamOsMutexLock(tpMutex);
        nMutexTestCnt++;
        CamOsDebug("%s start count: %d\n\r", __FUNCTION__, nMutexTestCnt);
        CamOsMsSleep(2);
        CamOsDebug("%s end count: %d\n\r", __FUNCTION__, nMutexTestCnt);
        CamOsMutexUnlock(tpMutex);
    }
}

static void CamOsMutexTestEntry2(void *pUserData)
{
    CamOsMutex_t *tpMutex = (CamOsMutex_t *)pUserData;
    unsigned int i = 0;

    for(i = 0; i < 100; i++)
    {
        CamOsMutexLock(tpMutex);
        nMutexTestCnt++;
        CamOsDebug("%s start count: %d\n\r", __FUNCTION__, nMutexTestCnt);
        CamOsMsSleep(2);
        CamOsDebug("%s end count: %d\n\r", __FUNCTION__, nMutexTestCnt);
        CamOsMutexUnlock(tpMutex);
    }
}

#endif

static int __init KernelTestInit(void)
{
#ifdef TEST_DIV64
    u64 nDividendU64 = 0, nDivisorU64 = 0, nResultU64 = 0, nRemainderU64 = 0;
    s64 nDividendS64 = 0, nDivisorS64 = 0, nResultS64 = 0, nRemainderS64 = 0;
#endif
#ifdef TEST_SYSTEM_TIME
    s32 nCnt = 0;
    CamOsTimespec_t tTs;
    struct rtc_time tTm;
#endif
#ifdef TEST_MEM_SIZE
    CamOsMemSize_e eMemSize;
#endif

#ifdef TEST_DIRECT_MEMORY
    void *pUserPtr1 = 0, *pUserPtr2 = 0, *pUserPtr3 = 0;
    u64 nMiuAddr = 0;
    u64 nPhysAddr = 0;

    CamOsDebug(KERN_INFO "Test DirectMem start\n");
    CamOsDirectMemAlloc("AAAAA", 1024, &pUserPtr1, &nMiuAddr, &nPhysAddr);
    CamOsDebug(KERN_INFO "        UserAddr: 0x%08X\n", (u32)pUserPtr1);
    CamOsDebug(KERN_INFO "        MiuAddr:  0x%08X\n", (u32)CamOsDirectMemPhysToMiu(nPhysAddr));
    CamOsDebug(KERN_INFO "        PhysAddr: 0x%08X\n", (u32)CamOsDirectMemMiuToPhys(nMiuAddr));

    CamOsDirectMemAlloc("BBBBB", 2048, &pUserPtr2, &nMiuAddr, &nPhysAddr);
    CamOsDebug(KERN_INFO "        UserAddr: 0x%08X\n", (u32)pUserPtr2);
    CamOsDebug(KERN_INFO "        MiuAddr:  0x%08X\n", (u32)CamOsDirectMemPhysToMiu(nPhysAddr));
    CamOsDebug(KERN_INFO "        PhysAddr: 0x%08X\n", (u32)CamOsDirectMemMiuToPhys(nMiuAddr));

    CamOsDirectMemAlloc("CCCCC", 2048, &pUserPtr3, &nMiuAddr, &nPhysAddr);
    CamOsDebug(KERN_INFO "        UserAddr: 0x%08X\n", (u32)pUserPtr3);
    CamOsDebug(KERN_INFO "        MiuAddr:  0x%08X\n", (u32)CamOsDirectMemPhysToMiu(nPhysAddr));
    CamOsDebug(KERN_INFO "        PhysAddr: 0x%08X\n", (u32)CamOsDirectMemMiuToPhys(nMiuAddr));

    CamOsDebug(KERN_INFO "List DMEM Status\n");
    CamOsDirectMemStat();

    CamOsDebug(KERN_INFO "Release DMEM BBBBB\n");
    CamOsDirectMemRelease(pUserPtr2, 2048);

    CamOsDebug(KERN_INFO "List DMEM Status\n");
    CamOsDirectMemStat();

    CamOsDebug(KERN_INFO "Release DMEM AAAAA\n");
    CamOsDirectMemRelease(pUserPtr1, 1024);

    CamOsDebug(KERN_INFO "List DMEM Status\n");
    CamOsDirectMemStat();

    CamOsDebug(KERN_INFO "Release DMEM CCCCC\n");
    CamOsDirectMemRelease(pUserPtr3, 2048);

    CamOsDebug(KERN_INFO "List DMEM Status\n");
    CamOsDirectMemStat();

    CamOsDebug(KERN_INFO "Test DirectMem end\n");
#endif

#ifdef TEST_TIME_AND_SLEEP
    CamOsThreadCreate(&tThread0, NULL, (void *)ThreadTestTimeMsleep, NULL);
    CamOsMsSleep(10000);
    CamOsThreadStop(tThread0);
#endif

#ifdef TEST_SEMAPHORE
    CamOsTsemInit(&tSem, 0);
    CamOsThreadCreate(&tThread1, NULL, (void *)ThreadTestTsem1, &tSem);
    CamOsThreadCreate(&tThread2, NULL, (void *)ThreadTestTsem2, &tSem);
#endif

#ifdef TEST_MUTEX
    CamOsMutexInit(&tCamOsMutex);
    tAttr.nPriority = 1;
    tAttr.nStackSize = 3072;
    CamOsThreadCreate(&tTaskHandle0, &tAttr, (void *)CamOsMutexTestEntry0, &tCamOsMutex);
    tAttr.nPriority = 10;
    tAttr.nStackSize = 3072;
    CamOsThreadCreate(&tTaskHandle1, &tAttr, (void *)CamOsMutexTestEntry1, &tCamOsMutex);
    tAttr.nPriority = 20;
    tAttr.nStackSize = 3072;
    CamOsThreadCreate(&tTaskHandle2, &tAttr, (void *)CamOsMutexTestEntry2, &tCamOsMutex);
    CamOsMutexDestroy(&tCamOsMutex);
#endif

#ifdef TEST_DIV64
    nDividendU64 = 858993459978593;
    nDivisorU64 = 34358634759;
    nResultU64 = CamOsMathDivU64(nDividendU64, nDivisorU64, &nRemainderU64);
    CamOsDebug("Dividend: %llu  Divisor: %llu  Result: %llu  Remainder: %llu\n",
               nDividendU64, nDivisorU64, nResultU64, nRemainderU64);

    nDividendS64 = -858993459978593;
    nDivisorS64 = 34358634759;
    nResultS64 = CamOsMathDivS64(nDividendS64, nDivisorS64, &nRemainderS64);
    CamOsDebug("Dividend: %lld  Divisor: %lld  Result: %lld  Remainder: %lld\n",
               nDividendS64, nDivisorS64, nResultS64, nRemainderS64);

    nDividendS64 = 858993459978593;
    nDivisorS64 = -34358634759;
    nResultS64 = CamOsMathDivS64(nDividendS64, nDivisorS64, &nRemainderS64);
    CamOsDebug("Dividend: %lld  Divisor: %lld  Result: %lld  Remainder: %lld\n",
               nDividendS64, nDivisorS64, nResultS64, nRemainderS64);

    nDividendS64 = -858993459978593;
    nDivisorS64 = -34358634759;
    nResultS64 = CamOsMathDivS64(nDividendS64, nDivisorS64, &nRemainderS64);
    CamOsDebug("Dividend: %lld  Divisor: %lld  Result: %lld  Remainder: %lld\n",
               nDividendS64, nDivisorS64, nResultS64, nRemainderS64);
#endif

#ifdef TEST_SYSTEM_TIME
    for(nCnt = 0; nCnt < 10; nCnt++)
    {
        CamOsGetTimeOfDay(&tTs);
        rtc_time_to_tm(tTs.nSec, &tTm);
        CamOsDebug("RawSecond: %d  ->  %d/%02d/%02d [%d]  %02d:%02d:%02d\n",
                   tTs.nSec,
                   tTm.tm_year+1900,
                   tTm.tm_mon+1,
                   tTm.tm_mday,
                   tTm.tm_wday,
                   tTm.tm_hour,
                   tTm.tm_min,
                   tTm.tm_sec);

        tTs.nSec += 90000;
        CamOsSetTimeOfDay(&tTs);

        CamOsMsSleep(3000);
    }

    for(nCnt = 0; nCnt < 10; nCnt++)
    {
        CamOsGetTimeOfDay(&tTs);
        rtc_time_to_tm(tTs.nSec, &tTm);
        CamOsDebug("RawSecond: %d  ->  %d/%02d/%02d [%d]  %02d:%02d:%02d\n",
                   tTs.nSec,
                   tTm.tm_year+1900,
                   tTm.tm_mon+1,
                   tTm.tm_mday,
                   tTm.tm_wday,
                   tTm.tm_hour,
                   tTm.tm_min,
                   tTm.tm_sec);

        tTs.nSec -= 90000;
        CamOsSetTimeOfDay(&tTs);

        CamOsMsSleep(3000);
    }
#endif

#ifdef TEST_MEM_SIZE
    eMemSize = CamOsPhysMemSize();
    CamOsDebug("System has %dMB physical memory\n", 1<<(u32)eMemSize);
#endif

#ifdef TEST_CHIP_ID
    CamOsDebug("Chip ID: 0x%X\n", CamOsChipId());
#endif

    return 0;
}

static void __exit KernelTestExit(void)
{
#ifdef TEST_SEMAPHORE
    CamOsThreadStop(tThread1);
    CamOsThreadStop(tThread2);
#endif

#ifdef TEST_MUTEX
    CamOsThreadStop(tTaskHandle0);
    CamOsThreadStop(tTaskHandle1);
    CamOsThreadStop(tTaskHandle2);
#endif
    CamOsDebug(KERN_INFO "Goodbye\n");
}

module_init(KernelTestInit);
module_exit(KernelTestExit);