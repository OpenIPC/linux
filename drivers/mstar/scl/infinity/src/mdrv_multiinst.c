////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2011 MStar Semiconductor, Inc.
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
#define _MDRV_MULTI_INST_C
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include "MsCommon.h"
#include "MsTypes.h"
#include "MsOS.h"
#include "drvpnl.h"


#include "mdrv_scl_dbg.h"
#include "mdrv_hvsp_io_st.h"
#include "mdrv_scldma_io_st.h"
#include "drvhvsp_st.h"
#include "drvhvsp.h"
#include "mdrv_hvsp.h"
#include "mdrv_scldma.h"
#include "mdrv_multiinst_st.h"
#include "mdrv_multiinst.h"

//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define MULTIINST_SEM_DBG 0
#define _MULTIINST_SEM_TIMIE_OUT 5000

#define MULTIINST_WAIT_SEM_FOREVER(_sem)      down(&_sem)
#define MULTIINST_RELEASE_SEM_FOREVER(_sem)    up(&_sem)
#define DRV_SC1_MUTEX_LOCK()            MsOS_ObtainMutex(_LOCK_Mutex[E_MDRV_MULTI_INST_LOCK_ID_SC_1_2],MSOS_WAIT_FOREVER)
#define DRV_SC1_MUTEX_UNLOCK()          MsOS_ReleaseMutex(_LOCK_Mutex[E_MDRV_MULTI_INST_LOCK_ID_SC_1_2])
#define DRV_SC3_MUTEX_LOCK()            MsOS_ObtainMutex(_LOCK_Mutex[E_MDRV_MULTI_INST_LOCK_ID_SC_3],MSOS_WAIT_FOREVER)
#define DRV_SC3_MUTEX_UNLOCK()          MsOS_ReleaseMutex(_LOCK_Mutex[E_MDRV_MULTI_INST_LOCK_ID_SC_3])
#define DRV_DSP_MUTEX_LOCK()            MsOS_ObtainMutex(_LOCK_Mutex[E_MDRV_MULTI_INST_LOCK_ID_DISP],MSOS_WAIT_FOREVER)
#define DRV_DSP_MUTEX_UNLOCK()          MsOS_ReleaseMutex(_LOCK_Mutex[E_MDRV_MULTI_INST_LOCK_ID_DISP])
#if MULTIINST_SEM_DBG

#define MULTIINST_WAIT_SEM(_sem)                                                 \
        printk(KERN_INFO "+++ [LOCK][%s]_1_[%d] \n", __FUNCTION__, __LINE__);         \
        if(down_timeout(&_sem, msecs_to_jiffies(_MULTIINST_SEM_TIMIE_OUT)) < 0)  \
        {                                                                       \
            printk(KERN_INFO "[LOCK][%s] [%d], LOCK3 Mutex Time Out \n", __FUNCTION__, __LINE__); \
        }   \
        printk(KERN_INFO "+++ [LOCK][%s]_2_[%d] \n", __FUNCTION__, __LINE__);


#define MULTIINST_RELEASE_SEM(_sem)                                              \
        printk(KERN_INFO "---  [LOCK][%s] [%d] \n", __FUNCTION__, __LINE__);          \
        up(&_sem);


#else
#define MULTIINST_WAIT_SEM(_sem)                                               \
        if(down_timeout(&_sem, msecs_to_jiffies(_MULTIINST_SEM_TIMIE_OUT)) < 0)    \
        {                                                                     \
            printk(KERN_INFO "[LOCK][%s] [%d], LOCK3 Mutex Time Out \n", __FUNCTION__, __LINE__); \
        }

#define MULTIINST_RELEASE_SEM(_sem)  up(&_sem)

#endif //MULTIINST_SEM_DBG


#define MDRV_MULTI_INST_HVSP_PRIVATE_ID_HEADER       0xA00000
#define MDRV_MULTI_INST_SCLDMA_PRIVATE_ID_HEADER     0xD00000
//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------

struct semaphore gMultiInstLockSem[E_MDRV_MULTI_INST_LOCK_ID_MAX];
struct semaphore gMultiInstHvspEntrySem[E_MDRV_MULTI_INST_HVSP_DATA_ID_MAX];
struct semaphore gMultiInstScldmaEntrySem[E_MDRV_MULTI_INST_SCLDMA_DATA_ID_MAX];

ST_MDRV_MULTI_INST_LOCK_CONFIG gstMultiInstLockCfg[E_MDRV_MULTI_INST_LOCK_ID_MAX];
ST_MDRV_MULTI_INST_SCLDMA_ENTRY_CONFIG gstMultiInstScldmaEntryCfg[E_MDRV_MULTI_INST_SCLDMA_DATA_ID_MAX][MDRV_MULTI_INST_SCLDMA_NUM];
ST_MDRV_MULTI_INST_HVSP_ENTRY_CONFIG   gstMultiInstHvspEntryCfg[E_MDRV_MULTI_INST_HVSP_DATA_ID_MAX][MDRV_MULTI_INST_HVSP_NUM];
signed long gs32PreMultiInstScldmaPrivateId[E_MDRV_MULTI_INST_SCLDMA_DATA_ID_MAX];
signed long gs32PreMultiInstHvspPrivateId[E_MDRV_MULTI_INST_HVSP_DATA_ID_MAX];

MS_S32 _LOCK_Mutex[E_MDRV_MULTI_INST_LOCK_ID_MAX]={-1,-1,-1};
MS_BOOL _LOCK_Mutex_flag[E_MDRV_MULTI_INST_LOCK_ID_MAX]={0,0,0};
//-------------------------------------------------------------------------------------------------
//  Function
//-------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------
//
// Multi Instance Lock
//
//----------------------------------------------------------------------------------------------------------------

unsigned char _MDrv_MultiInst_Lock_Get_PrivateID_Num(EN_MDRV_MULTI_INST_LOCK_ID_TYPE enLock_ID)
{
    unsigned char u8Num;

    switch(enLock_ID)
    {
    case E_MDRV_MULTI_INST_LOCK_ID_SC_1_2:
        u8Num = 4;
        break;
    case E_MDRV_MULTI_INST_LOCK_ID_SC_3:
        u8Num = 2;
        break;
    case E_MDRV_MULTI_INST_LOCK_ID_DISP:
        u8Num = 1;
        break;
    default:
        u8Num = 0;
        break;
    }

    return u8Num;
}


unsigned char MDrv_MultiInst_Lock_Init(EN_MDRV_MULTI_INST_LOCK_ID_TYPE enLock_ID)
{
    unsigned char bRet = 1;
    unsigned char i;
    unsigned char u8PrivateIDNum = 0;
    char word[]     = {"_SC1_Mutex"};
    char word2[]    = {"_SC3_Mutex"};
    char word3[]    = {"_DSP_Mutex"};
    SCL_DBG(SCL_DBG_LV_MULTI_INST_FUNC()&(Get_DBGMG_MULTI(enLock_ID)), "[MULTIINST]%s(%d)\n", __FUNCTION__, __LINE__);

    u8PrivateIDNum = _MDrv_MultiInst_Lock_Get_PrivateID_Num(enLock_ID);
    if(_LOCK_Mutex[E_MDRV_MULTI_INST_LOCK_ID_SC_1_2] == -1)
    {
        _LOCK_Mutex[E_MDRV_MULTI_INST_LOCK_ID_SC_1_2] = MsOS_CreateMutex(E_MSOS_FIFO, word, MSOS_PROCESS_SHARED);
    }
    if(_LOCK_Mutex[E_MDRV_MULTI_INST_LOCK_ID_SC_3] == -1)
    {
        _LOCK_Mutex[E_MDRV_MULTI_INST_LOCK_ID_SC_3] = MsOS_CreateMutex(E_MSOS_FIFO, word2, MSOS_PROCESS_SHARED);
    }
    if(_LOCK_Mutex[E_MDRV_MULTI_INST_LOCK_ID_DISP] == -1)
    {
        _LOCK_Mutex[E_MDRV_MULTI_INST_LOCK_ID_DISP] = MsOS_CreateMutex(E_MSOS_FIFO, word3, MSOS_PROCESS_SHARED);
    }
    if(gstMultiInstLockCfg[enLock_ID].ps32PrivateID == NULL)
    {
        gstMultiInstLockCfg[enLock_ID].u8IDNum = u8PrivateIDNum;
        gstMultiInstLockCfg[enLock_ID].ps32PrivateID = MsOS_Memalloc(sizeof(signed long)*u8PrivateIDNum, GFP_KERNEL);
        if(gstMultiInstLockCfg[enLock_ID].ps32PrivateID)
        {
            for(i=0; i<u8PrivateIDNum; i++)
            {
                gstMultiInstLockCfg[enLock_ID].ps32PrivateID[i] = -1;
            }
            sema_init(&gMultiInstLockSem[enLock_ID], 1);
            bRet = 1;
        }
        else
        {

            SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d) LockInit fail: LockId=%d\n", __FUNCTION__, __LINE__, enLock_ID);
            bRet = 0;
        }
    }

    return bRet;
}

unsigned char MDrv_MultiInst_Lock_Exit(EN_MDRV_MULTI_INST_LOCK_ID_TYPE enLock_ID)
{
    SCL_DBG(SCL_DBG_LV_MULTI_INST_FUNC()&(Get_DBGMG_MULTI(enLock_ID)), "[MULTIINST]%s(%d)\n", __FUNCTION__, __LINE__);

    if(gstMultiInstLockCfg[enLock_ID].ps32PrivateID)
    {
        MsOS_MemFree(gstMultiInstLockCfg[enLock_ID].ps32PrivateID);
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d) LockExit fail : LockId:%d\n", __FUNCTION__, __LINE__, enLock_ID);
    }

    return 1;
}


unsigned char MDrv_MultiInst_Lock_IsFree(EN_MDRV_MULTI_INST_LOCK_ID_TYPE enLock_ID, signed long s32PrivateId)
{
    unsigned char i, bIsFree;
    unsigned char bEmptyLock;
    unsigned char u8PrivateIDNum = _MDrv_MultiInst_Lock_Get_PrivateID_Num(enLock_ID);

    SCL_DBG(SCL_DBG_LV_MULTI_INST_FUNC()&(Get_DBGMG_MULTI(enLock_ID)), "[MULTIINST]%s(%d)\n", __FUNCTION__, __LINE__);

    MULTIINST_WAIT_SEM(gMultiInstLockSem[enLock_ID]);

    bEmptyLock = 1;
    for(i=0;i<u8PrivateIDNum;i++)
    {
        if(gstMultiInstLockCfg[enLock_ID].ps32PrivateID[i] != -1)
        {
            bEmptyLock = 0;
        }
    }

    if(bEmptyLock == 0)
    {
        bIsFree = 0;
        for(i=0;i<u8PrivateIDNum;i++)
        {
            if(gstMultiInstLockCfg[enLock_ID].ps32PrivateID[i] == s32PrivateId)
            {
                bIsFree = 1;
                SCL_DBG(SCL_DBG_LV_MULTI_INST_LOCK()&(Get_DBGMG_MULTI(enLock_ID)), "[MULTIINST]%s(%d) LockID:%d, idx:%d PirvateID= %lx \n",
                    __FUNCTION__, __LINE__, enLock_ID, i, s32PrivateId);
                break;
            }
        }
    }
    else
    {
        bIsFree = 1;
    }
    SCL_DBG(SCL_DBG_LV_MULTI_INST_LOCK()&(Get_DBGMG_MULTI(enLock_ID)), "[MULTIINST]%s(%d),LockID:%d IsFree:%d, PrivateId:%lx\n",
        __FUNCTION__, __LINE__,
        enLock_ID, bIsFree, s32PrivateId);

    MULTIINST_RELEASE_SEM(gMultiInstLockSem[enLock_ID]);
    return bIsFree;

}

unsigned char MDrv_MultiInst_Lock_Alloc(EN_MDRV_MULTI_INST_LOCK_ID_TYPE enLock_ID, ST_MDRV_MULTI_INST_LOCK_CONFIG stCfg)
{
    unsigned char i, bRet,bFree = 1;
    unsigned char u8PrivateIDNum = _MDrv_MultiInst_Lock_Get_PrivateID_Num(enLock_ID);

    SCL_DBG(SCL_DBG_LV_MULTI_INST_FUNC()&(Get_DBGMG_MULTI(enLock_ID)), "[MULTIINST]%s(%d)Id:%ld\n", __FUNCTION__, __LINE__,stCfg.ps32PrivateID[0]);
    if(stCfg.u8IDNum != u8PrivateIDNum)
    {
        SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d) LockAlloc fail LockID:%d, %d!=%d\n",
            __FUNCTION__, __LINE__, enLock_ID, stCfg.u8IDNum, u8PrivateIDNum);

        return 0;
    }
    if(enLock_ID == E_MDRV_MULTI_INST_LOCK_ID_SC_1_2)
    {
        DRV_SC1_MUTEX_LOCK();
        MULTIINST_WAIT_SEM(gMultiInstLockSem[enLock_ID]);
        _LOCK_Mutex_flag[E_MDRV_MULTI_INST_LOCK_ID_SC_1_2] = 1;
        MULTIINST_RELEASE_SEM(gMultiInstLockSem[enLock_ID]);
    }
    else if(enLock_ID == E_MDRV_MULTI_INST_LOCK_ID_SC_3)
    {
        DRV_SC3_MUTEX_LOCK();
        MULTIINST_WAIT_SEM(gMultiInstLockSem[enLock_ID]);
        _LOCK_Mutex_flag[E_MDRV_MULTI_INST_LOCK_ID_SC_3] = 1;
        MULTIINST_RELEASE_SEM(gMultiInstLockSem[enLock_ID]);
        SCL_DBG(SCL_DBG_LV_MULTI_INST_FUNC()&(Get_DBGMG_MULTI(enLock_ID)), "[MULTIINST]LOCK !! Id:%ld\n",stCfg.ps32PrivateID[0]);
    }
    else if(enLock_ID == E_MDRV_MULTI_INST_LOCK_ID_DISP)
    {
        DRV_DSP_MUTEX_LOCK();
        MULTIINST_WAIT_SEM(gMultiInstLockSem[enLock_ID]);
        _LOCK_Mutex_flag[E_MDRV_MULTI_INST_LOCK_ID_DISP] = 1;
        MULTIINST_RELEASE_SEM(gMultiInstLockSem[enLock_ID]);
    }


    MULTIINST_WAIT_SEM(gMultiInstLockSem[enLock_ID]);

    for(i=0; i<u8PrivateIDNum; i++)
    {
        if(gstMultiInstLockCfg[enLock_ID].ps32PrivateID[i] != -1)
        {
            bFree = 0;
            break;
        }
    }

    if( bFree )
    {
        for(i=0;i<u8PrivateIDNum;i++)
        {
            SCL_DBG(SCL_DBG_LV_MULTI_INST_LOCK()&(Get_DBGMG_MULTI(enLock_ID)), "[MULTIINST]%s(%d): LockID:%d, idx:%d, PirvateId:%lx\n",
                __FUNCTION__, __LINE__, enLock_ID, i, stCfg.ps32PrivateID[i]);

            gstMultiInstLockCfg[enLock_ID].ps32PrivateID[i] = stCfg.ps32PrivateID[i];
        }

        bRet = 1;
    }
    else
    {

    #if SCL_DBG_LV_MULTI_INST_LOCK_LOG
        for(i=0;i<u8PrivateIDNum;i++)
        {
            SCL_DBG(SCL_DBG_LV_MULTI_INST_LOCK()&(Get_DBGMG_MULTI(enLock_ID)), "[MULTIINST]%s(%d) LockAlloc fail: idx:%d, PrivateId:%lx\n",
                __FUNCTION__, __LINE__, i, stCfg.ps32PrivateID[i]);
        }
    #endif
        bRet = 0;
    }

    MULTIINST_RELEASE_SEM(gMultiInstLockSem[enLock_ID]);
    return bRet;

}

//----------------------------------------------------------------------------------------------------------------
//
// Multi Instance Data
//
//----------------------------------------------------------------------------------------------------------------
unsigned char MDrv_MultiInst_Lock_Free(EN_MDRV_MULTI_INST_LOCK_ID_TYPE enLock_ID, ST_MDRV_MULTI_INST_LOCK_CONFIG *pstCfg)
{
    unsigned char i;
    unsigned char bRet = 1;
    unsigned char u8PrivateIDNum = _MDrv_MultiInst_Lock_Get_PrivateID_Num(enLock_ID);

    SCL_DBG(SCL_DBG_LV_MULTI_INST_FUNC()&(Get_DBGMG_MULTI(enLock_ID)), "[MULTIINST]%s(%d)\n", __FUNCTION__, __LINE__);

    MULTIINST_WAIT_SEM(gMultiInstLockSem[enLock_ID]);

    if(pstCfg)
    {
        for(i=0; i<u8PrivateIDNum; i++)
        {
            if(gstMultiInstLockCfg[enLock_ID].ps32PrivateID[i] != pstCfg->ps32PrivateID[i])
            {
                bRet = 0;
                break;
            }
        }
    }

    if(bRet)
    {
        for(i=0;i<u8PrivateIDNum; i++)
        {
        #if SCL_DBG_LV_MULTI_INST_LOCK_LOG
            SCL_DBG(SCL_DBG_LV_MULTI_INST_LOCK()&(Get_DBGMG_MULTI(enLock_ID)), "[MULTIINST]%s(%d): idx=%d, PrivateId:%lx\n", __FUNCTION__, __LINE__, i, gstMultiInstLockCfg[enLock_ID].ps32PrivateID[i]);
        #endif
            gstMultiInstLockCfg[enLock_ID].ps32PrivateID[i] = -1;
        }
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_MULTI_INST_LOCK()&(Get_DBGMG_MULTI(enLock_ID)), "[MULTIINST]%s(%d) LockFree fail LockID:%d\n", __FUNCTION__, __LINE__, enLock_ID);

    }

    if(enLock_ID == E_MDRV_MULTI_INST_LOCK_ID_SC_1_2 && _LOCK_Mutex_flag[E_MDRV_MULTI_INST_LOCK_ID_SC_1_2])
    {
        _LOCK_Mutex_flag[E_MDRV_MULTI_INST_LOCK_ID_SC_1_2]=0;
        DRV_SC1_MUTEX_UNLOCK();
    }
    else if(enLock_ID == E_MDRV_MULTI_INST_LOCK_ID_SC_3 && _LOCK_Mutex_flag[E_MDRV_MULTI_INST_LOCK_ID_SC_3])
    {
        _LOCK_Mutex_flag[E_MDRV_MULTI_INST_LOCK_ID_SC_3]=0;
        DRV_SC3_MUTEX_UNLOCK();
    }
    else if(enLock_ID == E_MDRV_MULTI_INST_LOCK_ID_DISP && _LOCK_Mutex_flag[E_MDRV_MULTI_INST_LOCK_ID_DISP])
    {
        _LOCK_Mutex_flag[E_MDRV_MULTI_INST_LOCK_ID_DISP]=0;
        DRV_DSP_MUTEX_LOCK();
    }
    MULTIINST_RELEASE_SEM(gMultiInstLockSem[enLock_ID]);
    return bRet;
}

unsigned char _MDrv_MultiInst_Trans_HvspID(EN_MDRV_MULTI_INST_ENTRY_ID_TYPE enID, EN_MDRV_MULTI_INST_HVSP_DATA_ID_TYPE *pHvspID)
{
    unsigned char bRet = 1;
    switch(enID)
    {
    case E_MDRV_MULTI_INST_ENTRY_ID_HVSP1:
        *pHvspID = E_MDRV_MULTI_INST_HVSP_DATA_ID_1;
        break;
    case E_MDRV_MULTI_INST_ENTRY_ID_HVSP2:
        *pHvspID = E_MDRV_MULTI_INST_HVSP_DATA_ID_2;
        break;

    case E_MDRV_MULTI_INST_ENTRY_ID_HVSP3:
        *pHvspID = E_MDRV_MULTI_INST_HVSP_DATA_ID_3;
        break;

    default:
        *pHvspID = E_MDRV_MULTI_INST_HVSP_DATA_ID_MAX;
        bRet = 0;
        break;
    }
    return bRet;
}

unsigned char _MDrv_MultiInst_Trans_ScldmaID(EN_MDRV_MULTI_INST_ENTRY_ID_TYPE enID, EN_MDRV_MULTI_INST_SCLDMA_DATA_ID_TYPE *pScldmaID)
{
    unsigned char bRet = 1;
    switch(enID)
    {
    case E_MDRV_MULTI_INST_ENTRY_ID_SCLDMA1:
        *pScldmaID = E_MDRV_MULTI_INST_SCLDMA_DATA_ID_1;
        break;
    case E_MDRV_MULTI_INST_ENTRY_ID_SCLDMA2:
        *pScldmaID = E_MDRV_MULTI_INST_SCLDMA_DATA_ID_2;
        break;

    case E_MDRV_MULTI_INST_ENTRY_ID_SCLDMA3:
        *pScldmaID = E_MDRV_MULTI_INST_SCLDMA_DATA_ID_3;
        break;

    case E_MDRV_MULTI_INST_ENTRY_ID_DISP:
        *pScldmaID = E_MDRV_MULTI_INST_SCLDMA_DATA_ID_4;
        break;

    default:
        *pScldmaID = E_MDRV_MULTI_INST_SCLDMA_DATA_ID_MAX;
        bRet = 0;
        break;
    }
    return bRet;
}


unsigned char _MDrv_MultiInst_Entry_SaveHvspData(
    EN_MDRV_MULTI_INST_HVSP_DATA_ID_TYPE enID,
    ST_MDRV_MULTI_INST_HVSP_ENTRY_CONFIG *pEntry,
    EN_MDRV_MULTI_INST_CMD_TYPE enCmd,
    void *pData)
{
    unsigned char bRet = 1;

    if(pEntry)
    {
        MULTIINST_WAIT_SEM(gMultiInstHvspEntrySem[enID]);

        if(enCmd == E_MDRV_MULTI_INST_CMD_HVSP_IN_CONFIG)
        {
            pEntry->stData.stFlag.bInCfg = 1;
            MsOS_Memcpy(&pEntry->stData.stInCfg, (ST_MDRV_HVSP_INPUT_CONFIG *)pData, sizeof(ST_MDRV_HVSP_INPUT_CONFIG));
        }
        else if(enCmd == E_MDRV_MULTI_INST_CMD_HVSP_SCALING_CONFIG)
        {
            pEntry->stData.stFlag.bScaleCfg = 1;
            MsOS_Memcpy(&pEntry->stData.stScaleCfg, (ST_MDRV_HVSP_SCALING_CONFIG *)pData, sizeof(ST_MDRV_HVSP_SCALING_CONFIG));
        }
        else if(enCmd == E_MDRV_MULTI_INST_CMD_HVSP_POST_CROP_CONFIG)
        {
            pEntry->stData.stFlag.bPostCropCfg = 1;
            MsOS_Memcpy(&pEntry->stData.stPostCropCfg, (ST_MDRV_HVSP_POSTCROP_CONFIG *)pData, sizeof(ST_MDRV_HVSP_POSTCROP_CONFIG));
        }
        else if(enCmd == E_MDRV_MULTI_INST_CMD_HVSP_MEM_REQ_CONFIG)
        {
            pEntry->stData.stFlag.bMemCfg = 1;
            MsOS_Memcpy(&pEntry->stData.stMemCfg, (ST_MDRV_HVSP_IPM_CONFIG *)pData, sizeof(ST_MDRV_HVSP_IPM_CONFIG));
        }
        else if(enCmd == E_MDRV_MULTI_INST_CMD_HVSP_SET_OSD_CONFIG)
        {
            pEntry->stData.stFlag.bOSDCfg= 1;
            MsOS_Memcpy(&pEntry->stData.stOSDCfg, (ST_MDRV_HVSP_OSD_CONFIG*)pData, sizeof(ST_MDRV_HVSP_OSD_CONFIG));
        }

        SCL_DBG(SCL_DBG_LV_MULTI_INST_ENTRY()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d): PrivateId:%lx, bMem:%d, bIn:%d, bScale:%d\n",
            __FUNCTION__, __LINE__,
            pEntry->s32PivateId, pEntry->stData.stFlag.bMemCfg, pEntry->stData.stFlag.bInCfg,
            pEntry->stData.stFlag.bScaleCfg);

        MULTIINST_RELEASE_SEM(gMultiInstHvspEntrySem[enID]);
        bRet = 1;
    }
    else
    {
        bRet = 0;
        SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d): HvspSaveData fail: PrivateId=%lx\n", __FUNCTION__, __LINE__, pEntry->s32PivateId);
    }

    return bRet;
}

unsigned char _MDrv_MultiInst_Entry_SaveScldmaData(
    EN_MDRV_MULTI_INST_SCLDMA_DATA_ID_TYPE enID,
    ST_MDRV_MULTI_INST_SCLDMA_ENTRY_CONFIG *pEntry,
    EN_MDRV_MULTI_INST_CMD_TYPE enCmd,
    void *pData)
{
    unsigned char bRet = 1;
    unsigned char index = 0;
    if(pEntry)
    {

        MULTIINST_WAIT_SEM(gMultiInstScldmaEntrySem[enID]);
        if(enCmd == E_MDRV_MULTI_INST_CMD_SCLDMA_IN_BUFFER_CONFIG)
        {
            pEntry->stData.stFlag.bInBufCfg = 1;
            MsOS_Memcpy(&pEntry->stData.stInBufCfg, (ST_MDRV_SCLDMA_BUFFER_CONFIG *)pData, sizeof(ST_MDRV_SCLDMA_BUFFER_CONFIG));
        }
        else if(enCmd == E_MDRV_MULTI_INST_CMD_SCLDMA_OUT_BUFFER_CONFIG)
        {
            ST_MDRV_SCLDMA_BUFFER_CONFIG stCfg;
            MsOS_Memcpy(&stCfg, (ST_MDRV_SCLDMA_BUFFER_CONFIG *)pData, sizeof(ST_MDRV_SCLDMA_BUFFER_CONFIG));
            index = stCfg.enMemType;
            if(stCfg.enMemType == E_MDRV_SCLDMA_MEM_FRM)
            {
                pEntry->stData.stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_FRM]= 1;
                MsOS_Memcpy(&pEntry->stData.stOutBufCfg[E_MDRV_SCLDMA_MEM_FRM], (ST_MDRV_SCLDMA_BUFFER_CONFIG *)pData, sizeof(ST_MDRV_SCLDMA_BUFFER_CONFIG));
            }
            else if(stCfg.enMemType == E_MDRV_SCLDMA_MEM_SNP)
            {
                pEntry->stData.stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_SNP]= 1;
                MsOS_Memcpy(&pEntry->stData.stOutBufCfg[E_MDRV_SCLDMA_MEM_SNP], (ST_MDRV_SCLDMA_BUFFER_CONFIG *)pData, sizeof(ST_MDRV_SCLDMA_BUFFER_CONFIG));
            }
            else if(stCfg.enMemType == E_MDRV_SCLDMA_MEM_IMI)
            {
                pEntry->stData.stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_IMI]= 1;
                MsOS_Memcpy(&pEntry->stData.stOutBufCfg[E_MDRV_SCLDMA_MEM_IMI], (ST_MDRV_SCLDMA_BUFFER_CONFIG *)pData, sizeof(ST_MDRV_SCLDMA_BUFFER_CONFIG));
            }
        }
        else if(enCmd == E_MDRV_MULTI_INST_CMD_SCLDMA_IN_TRIGGER_CONFIG)
        {
            pEntry->stData.stFlag.bInTrigCfg = 1;
            MsOS_Memcpy(&pEntry->stData.stInTrigCfg, (ST_MDRV_SCLDMA_TRIGGER_CONFIG *)pData, sizeof(ST_MDRV_SCLDMA_TRIGGER_CONFIG));
        }
        else if(enCmd == E_MDRV_MULTI_INST_CMD_SCLDMA_OUT_TRIGGER_CONFIG)
        {
            ST_MDRV_SCLDMA_TRIGGER_CONFIG stCfg;
            MsOS_Memcpy(&stCfg, (ST_MDRV_SCLDMA_TRIGGER_CONFIG *)pData, sizeof(ST_MDRV_SCLDMA_TRIGGER_CONFIG));
            index = stCfg.enMemType;
            if(stCfg.enMemType == E_MDRV_SCLDMA_MEM_FRM)
            {
                pEntry->stData.stFlag.bOutTrigCfg[E_MDRV_SCLDMA_MEM_FRM]= 1;
                MsOS_Memcpy(&pEntry->stData.stOutTrigCfg[E_MDRV_SCLDMA_MEM_FRM], (ST_MDRV_SCLDMA_TRIGGER_CONFIG *)pData, sizeof(ST_MDRV_SCLDMA_TRIGGER_CONFIG));
            }
            else if(stCfg.enMemType == E_MDRV_SCLDMA_MEM_SNP)
            {
                pEntry->stData.stFlag.bOutTrigCfg[E_MDRV_SCLDMA_MEM_SNP]= 1;
                MsOS_Memcpy(&pEntry->stData.stOutTrigCfg[E_MDRV_SCLDMA_MEM_SNP], (ST_MDRV_SCLDMA_TRIGGER_CONFIG *)pData, sizeof(ST_MDRV_SCLDMA_TRIGGER_CONFIG));
            }
            else if(stCfg.enMemType == E_MDRV_SCLDMA_MEM_IMI)
            {
                pEntry->stData.stFlag.bOutTrigCfg[E_MDRV_SCLDMA_MEM_IMI]= 1;
                MsOS_Memcpy(&pEntry->stData.stOutTrigCfg[E_MDRV_SCLDMA_MEM_IMI], (ST_MDRV_SCLDMA_TRIGGER_CONFIG *)pData, sizeof(ST_MDRV_SCLDMA_TRIGGER_CONFIG));
            }
        }

        SCL_DBG(SCL_DBG_LV_MULTI_INST_ENTRY()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d): PrivateId:%lx, bInBuf:%d, bOutBuf[%hhd]:%d\n",
            __FUNCTION__, __LINE__,
            pEntry->s32PivateId, pEntry->stData.stFlag.bInBufCfg, index,pEntry->stData.stFlag.bOutBufCfg[index]);

        MULTIINST_RELEASE_SEM(gMultiInstScldmaEntrySem[enID]);
        bRet = 1;
    }
    else
    {
        bRet = 0;
        SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d): ScldmaSaveData fail: PrivateId:%lx\n", __FUNCTION__, __LINE__, pEntry->s32PivateId);
    }

    return bRet;
}

unsigned char _MDrv_MultiInst_Entry_ReloadHvspData(
    EN_MDRV_MULTI_INST_HVSP_DATA_ID_TYPE enID,
    ST_MDRV_MULTI_INST_HVSP_ENTRY_CONFIG *pEntry,
    EN_MDRV_MULTI_INST_CMD_TYPE enCmd)
{
    unsigned char bRet = 1;
    ST_MDRV_MULTI_INST_HVSP_FLAG_TYPE stFlag;
    EN_MDRV_HVSP_ID_TYPE enHvspId = enID == E_MDRV_MULTI_INST_HVSP_DATA_ID_1 ? E_MDRV_HVSP_ID_1 :
                                    enID == E_MDRV_MULTI_INST_HVSP_DATA_ID_2 ? E_MDRV_HVSP_ID_2 :
                                    enID == E_MDRV_MULTI_INST_HVSP_DATA_ID_3 ? E_MDRV_HVSP_ID_3 :
                                                                               E_MDRV_HVSP_ID_MAX ;
    MsOS_Memset(&stFlag, 0, sizeof(ST_MDRV_MULTI_INST_HVSP_FLAG_TYPE));

    MULTIINST_WAIT_SEM(gMultiInstHvspEntrySem[enID]);

    if(pEntry)
    {
        if(enCmd == E_MDRV_MULTI_INST_CMD_FORCE_RELOAD_CONFIG)
        {
            //Reload
            stFlag.bInCfg           = pEntry->stData.stFlag.bInCfg;
            stFlag.bScaleCfg        = pEntry->stData.stFlag.bScaleCfg;
            stFlag.bPostCropCfg     = pEntry->stData.stFlag.bPostCropCfg;
            stFlag.bMemCfg          = pEntry->stData.stFlag.bMemCfg;
            stFlag.bOSDCfg          = pEntry->stData.stFlag.bOSDCfg;
        }
        else if(pEntry->s32PivateId != gs32PreMultiInstHvspPrivateId[enID] &&
           gs32PreMultiInstHvspPrivateId[enID] != -1)
        {
            //if not the same fd,reload others setting.(ioctl will set itself,so just set others configuration)
            if(enCmd == E_MDRV_MULTI_INST_CMD_HVSP_IN_CONFIG)
            {
                stFlag.bScaleCfg        = pEntry->stData.stFlag.bScaleCfg;
                stFlag.bPostCropCfg     = pEntry->stData.stFlag.bPostCropCfg;
                stFlag.bMemCfg          = pEntry->stData.stFlag.bMemCfg;
                stFlag.bOSDCfg          = pEntry->stData.stFlag.bOSDCfg;
            }
            else if(enCmd == E_MDRV_MULTI_INST_CMD_HVSP_SCALING_CONFIG)
            {
                stFlag.bInCfg           = pEntry->stData.stFlag.bInCfg;
                stFlag.bPostCropCfg     = pEntry->stData.stFlag.bPostCropCfg;
                stFlag.bMemCfg          = pEntry->stData.stFlag.bMemCfg;
                stFlag.bOSDCfg          = pEntry->stData.stFlag.bOSDCfg;
            }
            else if(enCmd == E_MDRV_MULTI_INST_CMD_HVSP_POST_CROP_CONFIG)
            {
                stFlag.bInCfg           = pEntry->stData.stFlag.bInCfg;
                stFlag.bScaleCfg        = pEntry->stData.stFlag.bScaleCfg;
                stFlag.bMemCfg          = pEntry->stData.stFlag.bMemCfg;
                stFlag.bOSDCfg          = pEntry->stData.stFlag.bOSDCfg;
            }
            else if(enCmd == E_MDRV_MULTI_INST_CMD_HVSP_SET_OSD_CONFIG)
            {
                stFlag.bInCfg           = pEntry->stData.stFlag.bInCfg;
                stFlag.bScaleCfg        = pEntry->stData.stFlag.bScaleCfg;
                stFlag.bPostCropCfg     = pEntry->stData.stFlag.bPostCropCfg;
                stFlag.bMemCfg          = pEntry->stData.stFlag.bMemCfg;
            }
        }
        else
        {
            SCL_DBG(SCL_DBG_LV_MULTI_INST_ENTRY()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d)::The first time\n", __FUNCTION__, __LINE__);
        }

        SCL_DBG(SCL_DBG_LV_MULTI_INST_ENTRY()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d)::PrivateId:%lx, bMem:%d, bIn:%d, bScale:%d,bPostCrop:%d\n",
            __FUNCTION__, __LINE__,
            pEntry->s32PivateId, stFlag.bMemCfg, stFlag.bInCfg,
            stFlag.bScaleCfg, stFlag.bPostCropCfg);

        bRet = 1;

        if(stFlag.bMemCfg)
        {
            ST_HVSP_CMD_TRIG_CONFIG stHvspCmdTrigCfg;
            stHvspCmdTrigCfg = Drv_HVSP_SetCMDQTrigTypeByRIU();

            bRet &= MDrv_HVSP_SetInitIPMConfig(enHvspId, &pEntry->stData.stMemCfg);

            Drv_HVSP_SetCMDQTrigType(stHvspCmdTrigCfg);
        }

        if(stFlag.bInCfg)
        {
            bRet &= MDrv_HVSP_SetInputConfig(enHvspId, &pEntry->stData.stInCfg);
        }


        if(stFlag.bScaleCfg)
        {
            bRet &= MDrv_HVSP_SetScalingConfig(enHvspId, &pEntry->stData.stScaleCfg);
        }

        if(stFlag.bPostCropCfg)
        {
            bRet &= MDrv_HVSP_SetPostCropConfig(enHvspId, &pEntry->stData.stPostCropCfg);
        }

        if(stFlag.bOSDCfg)
        {
            bRet &= MDrv_HVSP_SetOSDConfig(enHvspId, &pEntry->stData.stOSDCfg);
        }

        gs32PreMultiInstHvspPrivateId[enID] = pEntry->s32PivateId;
    }
    else
    {
        bRet = 0;
        SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d):: ReloadHvsp Fail, hvspId:%d\n",
            __FUNCTION__, __LINE__, enID);
    }

    MULTIINST_RELEASE_SEM(gMultiInstHvspEntrySem[enID]);

    return bRet;
}


unsigned char _MDrv_MultiInst_Entry_ReloadScldmaData(
    EN_MDRV_MULTI_INST_SCLDMA_DATA_ID_TYPE enID,
    ST_MDRV_MULTI_INST_SCLDMA_ENTRY_CONFIG *pEntry,
    EN_MDRV_MULTI_INST_CMD_TYPE enCmd)
{
    unsigned char bRet;
    ST_MDRV_MULTI_INST_SCLDMA_FLAG_TYPE stFlag;
    EN_MDRV_SCLDMA_ID_TYPE enScldmaId = enID == E_MDRV_MULTI_INST_SCLDMA_DATA_ID_1 ? E_MDRV_SCLDMA_ID_1 :
                                        enID == E_MDRV_MULTI_INST_SCLDMA_DATA_ID_2 ? E_MDRV_SCLDMA_ID_2 :
                                        enID == E_MDRV_MULTI_INST_SCLDMA_DATA_ID_3 ? E_MDRV_SCLDMA_ID_3 :
                                        enID == E_MDRV_MULTI_INST_SCLDMA_DATA_ID_4 ? E_MDRV_SCLDMA_ID_PNL :
                                                                                     E_MDRV_HVSP_ID_MAX ;
    MsOS_Memset(&stFlag, 0, sizeof(ST_MDRV_MULTI_INST_SCLDMA_FLAG_TYPE));

    MULTIINST_WAIT_SEM(gMultiInstScldmaEntrySem[enID]);

    if(pEntry)
    {
        if(enCmd == E_MDRV_MULTI_INST_CMD_FORCE_RELOAD_CONFIG)
        {
            stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_FRM]  = pEntry->stData.stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_FRM];
            stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_SNP]  = pEntry->stData.stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_SNP];
            stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_IMI]  = pEntry->stData.stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_IMI];
            stFlag.bInBufCfg   = pEntry->stData.stFlag.bInBufCfg;
            stFlag.bInTrigCfg  = pEntry->stData.stFlag.bInTrigCfg;
            if(pEntry->stData.stOutBufCfg[E_MDRV_SCLDMA_MEM_FRM].enBufMDType==E_MDRV_SCLDMA_BUFFER_MD_RING
                &&(enScldmaId==E_MDRV_SCLDMA_ID_1 || enScldmaId==E_MDRV_SCLDMA_ID_2))
            {
                stFlag.bOutTrigCfg[E_MDRV_SCLDMA_MEM_FRM] = 0;
            }
            else
            {
                stFlag.bOutTrigCfg[E_MDRV_SCLDMA_MEM_FRM] =  pEntry->stData.stFlag.bOutTrigCfg[E_MDRV_SCLDMA_MEM_FRM];
            }
            stFlag.bOutTrigCfg[E_MDRV_SCLDMA_MEM_SNP] = pEntry->stData.stFlag.bOutTrigCfg[E_MDRV_SCLDMA_MEM_SNP];
            stFlag.bOutTrigCfg[E_MDRV_SCLDMA_MEM_IMI] = pEntry->stData.stFlag.bOutTrigCfg[E_MDRV_SCLDMA_MEM_IMI];
        }
        else if(pEntry->s32PivateId != gs32PreMultiInstScldmaPrivateId[enID] &&
                gs32PreMultiInstScldmaPrivateId[enID] != -1)
        {
            if(enCmd == E_MDRV_MULTI_INST_CMD_SCLDMA_IN_BUFFER_CONFIG)
            {
                //for Sc3 ,   Sc1,Sc2 ignore
                stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_FRM] = pEntry->stData.stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_FRM];
                stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_SNP] = pEntry->stData.stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_SNP];
                stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_IMI] = pEntry->stData.stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_IMI];
            }
            else if(enCmd == E_MDRV_MULTI_INST_CMD_SCLDMA_OUT_BUFFER_CONFIG)
            {
                //for Sc3 ,  Sc1,Sc2 ignore
                stFlag.bInBufCfg  = pEntry->stData.stFlag.bInBufCfg;
            }
            else if(enCmd == E_MDRV_MULTI_INST_CMD_SCLDMA_IN_TRIGGER_CONFIG ||
                    enCmd == E_MDRV_MULTI_INST_CMD_SCLDMA_OUT_TRIGGER_CONFIG
            )
            {
                stFlag.bInBufCfg  = pEntry->stData.stFlag.bInBufCfg;
                stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_FRM] = pEntry->stData.stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_FRM];
                stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_SNP] = pEntry->stData.stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_SNP];
                stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_IMI] = pEntry->stData.stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_IMI];
            }
        }
        else
        {
            SCL_DBG(SCL_DBG_LV_MULTI_INST_ENTRY()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d)::The first time\n", __FUNCTION__, __LINE__);
        }

        SCL_DBG(SCL_DBG_LV_MULTI_INST_ENTRY()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d)::PrivateId:%lx, bInBuf:%d,bOutBuf_FRM:%d, bOutBuf_SNP:%d, bOutBuf_IMI:%d, bInTrig:%d, bOutTrig_FRM:%d, bOutTrig_SNP:%d, bOutTrig_IMI:%d\n",
            __FUNCTION__, __LINE__,
            pEntry->s32PivateId, stFlag.bInBufCfg,
            stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_FRM],stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_SNP],stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_IMI],
            stFlag.bInTrigCfg,
            stFlag.bOutTrigCfg[E_MDRV_SCLDMA_MEM_FRM],stFlag.bOutTrigCfg[E_MDRV_SCLDMA_MEM_SNP],stFlag.bOutTrigCfg[E_MDRV_SCLDMA_MEM_IMI]);

        bRet = 1;
        if(stFlag.bInBufCfg)
        {
            bRet &= MDrv_SCLDMA_SetDMAReadClientConfig(enScldmaId, &pEntry->stData.stInBufCfg);
        }

        if(stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_FRM])
        {
            bRet &= MDrv_SCLDMA_SetDMAWriteClientConfig(enScldmaId, &pEntry->stData.stOutBufCfg[E_MDRV_SCLDMA_MEM_FRM]);
        }


        if(stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_SNP])
        {
            bRet &= MDrv_SCLDMA_SetDMAWriteClientConfig(enScldmaId, &pEntry->stData.stOutBufCfg[E_MDRV_SCLDMA_MEM_SNP]);
        }


        if(stFlag.bOutBufCfg[E_MDRV_SCLDMA_MEM_IMI])
        {
            bRet &= MDrv_SCLDMA_SetDMAWriteClientConfig(enScldmaId, &pEntry->stData.stOutBufCfg[E_MDRV_SCLDMA_MEM_IMI]);
        }

        if(stFlag.bInTrigCfg)
        {
            bRet &= MDrv_SCLDMA_SetDMAReadClientTrigger(enScldmaId, &pEntry->stData.stInTrigCfg);
        }

        if(stFlag.bOutTrigCfg[E_MDRV_SCLDMA_MEM_FRM])
        {
            bRet &= MDrv_SCLDMA_SetDMAWriteClientTrigger(enScldmaId, &pEntry->stData.stOutTrigCfg[E_MDRV_SCLDMA_MEM_FRM]);
        }

        if(stFlag.bOutTrigCfg[E_MDRV_SCLDMA_MEM_SNP])
        {
            bRet &= MDrv_SCLDMA_SetDMAWriteClientTrigger(enScldmaId, &pEntry->stData.stOutTrigCfg[E_MDRV_SCLDMA_MEM_SNP]);
        }

        if(stFlag.bOutTrigCfg[E_MDRV_SCLDMA_MEM_IMI])
        {
            bRet &= MDrv_SCLDMA_SetDMAWriteClientTrigger(enScldmaId, &pEntry->stData.stOutTrigCfg[E_MDRV_SCLDMA_MEM_IMI]);
        }

        gs32PreMultiInstScldmaPrivateId[enID] = pEntry->s32PivateId;
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d):: ReloadScldmaFail, ScldmaId:%d\n",
            __FUNCTION__, __LINE__, enID);

        bRet = 0;
    }

    MULTIINST_RELEASE_SEM(gMultiInstScldmaEntrySem[enID]);

    return bRet;
}



EN_MDRV_MULTI_INST_STATUS_TYPE _MDrv_MultiInst_Entry_FlashHvspData(
    EN_MDRV_MULTI_INST_HVSP_DATA_ID_TYPE enID,
    ST_MDRV_MULTI_INST_HVSP_ENTRY_CONFIG *pEntry,
    EN_MDRV_MULTI_INST_CMD_TYPE enCmd,
    void *pData)
{
    EN_MDRV_MULTI_INST_STATUS_TYPE enRet = E_MDRV_MULTI_INST_STATUS_SUCCESS;
    EN_MDRV_MULTI_INST_LOCK_ID_TYPE enLockID;
    unsigned char bSave, bReload ;

    if(enCmd == E_MDRV_MULTI_INST_CMD_FORCE_RELOAD_CONFIG && pEntry == NULL)
    {
        MS_U8 i;

        SCL_DBG(SCL_DBG_LV_MULTI_INST_ENTRY()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d): Force_Relaod, PrivateId=%lx\n",
            __FUNCTION__, __LINE__, gs32PreMultiInstHvspPrivateId[enID]);

        for(i=0; i<E_MDRV_MULTI_INST_HVSP_DATA_ID_MAX; i++)
        {
            if(gs32PreMultiInstHvspPrivateId[enID] == gstMultiInstHvspEntryCfg[enID][i].s32PivateId)
            {
                pEntry = &gstMultiInstHvspEntryCfg[enID][i];
                SCL_DBG(SCL_DBG_LV_MULTI_INST_ENTRY()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d): Force_Relaod, idx=%d\n",
                    __FUNCTION__, __LINE__, i);
                break;
            }
        }

        if(pEntry == NULL)
        {
            return E_MDRV_MULTI_INST_STATUS_SUCCESS;
        }
    }

    if(pEntry)
    {
        if(enCmd == E_MDRV_MULTI_INST_CMD_MAX)
        {
            enRet = E_MDRV_MULTI_INST_STATUS_SUCCESS;
        }
        else
        {
            enLockID = enID == E_MDRV_MULTI_INST_HVSP_DATA_ID_1 ? E_MDRV_MULTI_INST_LOCK_ID_SC_1_2 :
                       enID == E_MDRV_MULTI_INST_HVSP_DATA_ID_2 ? E_MDRV_MULTI_INST_LOCK_ID_SC_1_2 :
                       enID == E_MDRV_MULTI_INST_HVSP_DATA_ID_3 ? E_MDRV_MULTI_INST_LOCK_ID_SC_3   :
                                                                  E_MDRV_MULTI_INST_LOCK_ID_MAX;

            if( MDrv_MultiInst_Lock_IsFree(enLockID, pEntry->s32PivateId) )
            {
                // Reload Data
                bReload = _MDrv_MultiInst_Entry_ReloadHvspData(enID, pEntry, enCmd);
                // Save Data
                bSave = (enCmd == E_MDRV_MULTI_INST_CMD_FORCE_RELOAD_CONFIG) ?
                        1 :
                        _MDrv_MultiInst_Entry_SaveHvspData(enID, pEntry, enCmd, pData);

                if(bReload & bSave)
                {
                    enRet = E_MDRV_MULTI_INST_STATUS_SUCCESS;
                }
                else
                {
                    enRet = E_MDRV_MULTI_INST_STATUS_FAIL;
                    SCL_DBG(SCL_DBG_LV_MULTI_INST_ENTRY()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d): FlashHvspData fail, PrivateId=%lx, R:%d, S:%d\n",
                        __FUNCTION__, __LINE__, pEntry->s32PivateId, bReload, bSave);
                }
            }
            else
            {
                enRet = E_MDRV_MULTI_INST_STATUS_LOCKED;
                SCL_DBG(SCL_DBG_LV_MULTI_INST_ENTRY()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d): FlashHvspData LOCKED, PrivateId=%lx\n",
                    __FUNCTION__, __LINE__, pEntry->s32PivateId);
            }
        }
    }
    else
    {
        enRet = E_MDRV_MULTI_INST_STATUS_FAIL;
        SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d): FlashHvspData fail, PrivateId= NULL\n", __FUNCTION__, __LINE__);
    }
    return enRet;
}

EN_MDRV_MULTI_INST_STATUS_TYPE _MDrv_MultiInst_Entry_FlashScldmaData(
    EN_MDRV_MULTI_INST_SCLDMA_DATA_ID_TYPE enID,
    ST_MDRV_MULTI_INST_SCLDMA_ENTRY_CONFIG *pEntry,
    EN_MDRV_MULTI_INST_CMD_TYPE enCmd,
    void *pData)
{
    EN_MDRV_MULTI_INST_STATUS_TYPE enRet = E_MDRV_MULTI_INST_STATUS_SUCCESS;
    EN_MDRV_MULTI_INST_LOCK_ID_TYPE enLockID;
    unsigned char bSave, bReload ;

    if(enCmd == E_MDRV_MULTI_INST_CMD_FORCE_RELOAD_CONFIG && pEntry == NULL)
    {
        MS_U8 i;

        SCL_DBG(SCL_DBG_LV_MULTI_INST_ENTRY()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d): Force_Relaod, PrivateId=%lx\n",
            __FUNCTION__, __LINE__, gs32PreMultiInstScldmaPrivateId[enID]);

        for(i=0; i<E_MDRV_MULTI_INST_SCLDMA_DATA_ID_MAX; i++)
        {
            if(gs32PreMultiInstScldmaPrivateId[enID] == gstMultiInstScldmaEntryCfg[enID][i].s32PivateId)
            {
                pEntry = &gstMultiInstScldmaEntryCfg[enID][i];
                SCL_DBG(SCL_DBG_LV_MULTI_INST_ENTRY()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d): Force_Relaod, idx=%d\n",
                    __FUNCTION__, __LINE__, i);
                break;
            }
        }

        if(pEntry == NULL)
        {
            return E_MDRV_MULTI_INST_STATUS_SUCCESS;
        }

    }

    if(pEntry)
    {
        if(enCmd == E_MDRV_MULTI_INST_CMD_MAX)
        {
            enRet = E_MDRV_MULTI_INST_STATUS_SUCCESS;
        }
        else
        {
            enLockID = enID == E_MDRV_MULTI_INST_SCLDMA_DATA_ID_1 ? E_MDRV_MULTI_INST_LOCK_ID_SC_1_2 :
                       enID == E_MDRV_MULTI_INST_SCLDMA_DATA_ID_2 ? E_MDRV_MULTI_INST_LOCK_ID_SC_1_2 :
                       enID == E_MDRV_MULTI_INST_SCLDMA_DATA_ID_3 ? E_MDRV_MULTI_INST_LOCK_ID_SC_3   :
                       enID == E_MDRV_MULTI_INST_SCLDMA_DATA_ID_4 ? E_MDRV_MULTI_INST_LOCK_ID_DISP   :
                                                                    E_MDRV_MULTI_INST_LOCK_ID_MAX;

            if( MDrv_MultiInst_Lock_IsFree(enLockID, pEntry->s32PivateId) )
            {
                // Reload Data
                bReload = _MDrv_MultiInst_Entry_ReloadScldmaData(enID, pEntry, enCmd);

                // Save Data
                bSave = (enCmd == E_MDRV_MULTI_INST_CMD_FORCE_RELOAD_CONFIG) ?
                        1 :
                        _MDrv_MultiInst_Entry_SaveScldmaData(enID, pEntry, enCmd, pData);

                if(bReload && bSave)
                {
                    enRet = E_MDRV_MULTI_INST_STATUS_SUCCESS;
                }
                else
                {
                    enRet = E_MDRV_MULTI_INST_STATUS_FAIL;
                    SCL_DBG(SCL_DBG_LV_MULTI_INST_ENTRY()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d): FlashScldmaData fail, PrivateId=%lx, R:%d, S:%d\n",
                        __FUNCTION__, __LINE__, pEntry->s32PivateId, bReload, bSave);
                }
            }
            else
            {
                enRet = E_MDRV_MULTI_INST_STATUS_LOCKED;
                SCL_DBG(SCL_DBG_LV_MULTI_INST_ENTRY()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d): FlashScldmaData LOCKED, PrivateId=%lx\n", __FUNCTION__, __LINE__, pEntry->s32PivateId);
            }
        }
    }
    else
    {
        enRet = E_MDRV_MULTI_INST_STATUS_FAIL;
        SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d): FlashScldmaData fail, PrivateId=%lx\n", __FUNCTION__, __LINE__, pEntry->s32PivateId);
    }

    return enRet;
}



unsigned char MDrv_MultiInst_Entry_Init_Variable(EN_MDRV_MULTI_INST_ENTRY_ID_TYPE enID)
{
    unsigned char bRet = 1;
    unsigned short i;

    SCL_DBG(SCL_DBG_LV_MULTI_INST_FUNC()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d)\n", __FUNCTION__, __LINE__);

    if(enID <= E_MDRV_MULTI_INST_ENTRY_ID_HVSP3)
    {
        EN_MDRV_MULTI_INST_HVSP_DATA_ID_TYPE enHvspId;

        if( _MDrv_MultiInst_Trans_HvspID(enID, &enHvspId))
        {
            for(i=0;i<MDRV_MULTI_INST_HVSP_NUM; i++)
            {
                gstMultiInstHvspEntryCfg[enHvspId][i].s32PivateId = MDRV_MULTI_INST_HVSP_PRIVATE_ID_HEADER | (enHvspId << 8) | i;
                gstMultiInstHvspEntryCfg[enHvspId][i].bUsed = 0;
                MsOS_Memset(&gstMultiInstHvspEntryCfg[enHvspId][i].stData,  0, sizeof(ST_MDRV_MULTI_INST_HVSP_DATA_TYPE));
            }

            gs32PreMultiInstHvspPrivateId[enHvspId] = -1;
            sema_init(&gMultiInstHvspEntrySem[enHvspId], 1);
            bRet = 1;
        }
        else
        {
            SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d): Init fail, EntryId=%d\n", __FUNCTION__, __LINE__, enID);
            bRet = 0;
        }

    }
    else if(enID <= E_MDRV_MULTI_INST_ENTRY_ID_DISP)
    {
        EN_MDRV_MULTI_INST_SCLDMA_DATA_ID_TYPE enScldmaId;
        if( _MDrv_MultiInst_Trans_ScldmaID(enID, &enScldmaId))
        {
            for(i=0;i<MDRV_MULTI_INST_SCLDMA_NUM; i++)
            {
                gstMultiInstScldmaEntryCfg[enScldmaId][i].s32PivateId = MDRV_MULTI_INST_SCLDMA_PRIVATE_ID_HEADER | (enScldmaId << 8) | i;
                gstMultiInstScldmaEntryCfg[enScldmaId][i].bUsed = 0;
                MsOS_Memset(&gstMultiInstScldmaEntryCfg[enScldmaId][i].stData,  0, sizeof(ST_MDRV_MULTI_INST_SCLDMA_DATA_TYPE));
            }
            gs32PreMultiInstScldmaPrivateId[enScldmaId] = -1;
            sema_init(&gMultiInstScldmaEntrySem[enScldmaId], 1);
            bRet = 1;
        }
        else
        {
            SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d): Init fail, EntryId=%d\n", __FUNCTION__, __LINE__, enID);
            bRet = 0;
        }
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d): Init fail, EntyrId=%d\n", __FUNCTION__, __LINE__, enID);
        bRet = 0;
    }

    return bRet;
}


unsigned char MDrv_MultiInst_Entry_Alloc(EN_MDRV_MULTI_INST_ENTRY_ID_TYPE enID, void **pPrivate_Data)
{
    signed short i;
    signed short s16Entry = -1;
    unsigned char bRet = 1;

    SCL_DBG(SCL_DBG_LV_MULTI_INST_FUNC()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d)\n", __FUNCTION__, __LINE__);

    if(enID <= E_MDRV_MULTI_INST_ENTRY_ID_HVSP3)
    {
        EN_MDRV_MULTI_INST_HVSP_DATA_ID_TYPE enHvspId;
        if( _MDrv_MultiInst_Trans_HvspID(enID, &enHvspId))
        {
            MULTIINST_WAIT_SEM(gMultiInstHvspEntrySem[enHvspId]);
            for(i=0; i<MDRV_MULTI_INST_HVSP_NUM; i++)
            {
                if(gstMultiInstHvspEntryCfg[enHvspId][i].bUsed == 0)
                {
                    s16Entry = i;
                    break;
                }
            }
            if(s16Entry < 0)
            {
                *pPrivate_Data = NULL;
                bRet = 0;
            }
            else
            {
                gstMultiInstHvspEntryCfg[enHvspId][s16Entry].bUsed = 1;
                *pPrivate_Data =  (void *)&gstMultiInstHvspEntryCfg[enHvspId][s16Entry];
                bRet = 1;
            }
            MULTIINST_RELEASE_SEM(gMultiInstHvspEntrySem[enHvspId]);
            SCL_DBG(SCL_DBG_LV_MULTI_INST_ENTRY()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d)::PrivateId:%lx, %x\n",
                __FUNCTION__, __LINE__, gstMultiInstHvspEntryCfg[enHvspId][s16Entry].s32PivateId, (unsigned int)*pPrivate_Data);
        }
        else
        {
            SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d): Alloc fail, EntryId=%d\n", __FUNCTION__, __LINE__, enID);
            bRet = 0;
        }
    }
    else if(enID <= E_MDRV_MULTI_INST_ENTRY_ID_DISP)
    {
        EN_MDRV_MULTI_INST_SCLDMA_DATA_ID_TYPE enScldmaId;
        if( _MDrv_MultiInst_Trans_ScldmaID(enID, &enScldmaId))
        {
            MULTIINST_WAIT_SEM(gMultiInstScldmaEntrySem[enScldmaId]);

            for(i=0;i<MDRV_MULTI_INST_SCLDMA_NUM; i++)
            {
                if(gstMultiInstScldmaEntryCfg[enScldmaId][i].bUsed == 0)
                {
                    s16Entry = i;
                    break;
                }
            }

            if(s16Entry < 0)
            {
                *pPrivate_Data = NULL;
                bRet = 0;
            }
            else
            {
                gstMultiInstScldmaEntryCfg[enScldmaId][s16Entry].bUsed = 1;
                *pPrivate_Data =  (void *)&gstMultiInstScldmaEntryCfg[enScldmaId][i];
                bRet = 1;
            }
            MULTIINST_RELEASE_SEM(gMultiInstScldmaEntrySem[enScldmaId]);
            SCL_DBG(SCL_DBG_LV_MULTI_INST_ENTRY()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d):: PrivateId:%lx, %x\n",
                __FUNCTION__, __LINE__, gstMultiInstScldmaEntryCfg[enScldmaId][s16Entry].s32PivateId, (unsigned int)*pPrivate_Data);

        }
        else
        {
            SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d): Alloc fail EntryId=%d\n", __FUNCTION__, __LINE__, enID);
            bRet = 0;
        }
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d): Alloc fail, EntryId=%d\n", __FUNCTION__, __LINE__, enID);
        bRet = 0;
    }

    return bRet;
}

unsigned char MDrv_MultiInst_Entry_Free(EN_MDRV_MULTI_INST_ENTRY_ID_TYPE enID, void *pPrivate_Data)
{
    unsigned char bRet;
    SCL_DBG(SCL_DBG_LV_MULTI_INST_FUNC()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d)\n", __FUNCTION__, __LINE__);

    if(enID <= E_MDRV_MULTI_INST_ENTRY_ID_HVSP3)
    {
        EN_MDRV_MULTI_INST_HVSP_DATA_ID_TYPE enHvspId;
        if( _MDrv_MultiInst_Trans_HvspID(enID, &enHvspId) && pPrivate_Data)
        {
            ST_MDRV_MULTI_INST_HVSP_ENTRY_CONFIG *pEntry = NULL;

            MULTIINST_WAIT_SEM(gMultiInstHvspEntrySem[enHvspId]);
            pEntry = (ST_MDRV_MULTI_INST_HVSP_ENTRY_CONFIG *)pPrivate_Data;
            pEntry->bUsed = 0;
            MsOS_Memset(&pEntry->stData, 0, sizeof(ST_MDRV_MULTI_INST_HVSP_DATA_TYPE));
            MULTIINST_RELEASE_SEM(gMultiInstHvspEntrySem[enHvspId]);

            bRet = 1;
        }
        else
        {
            SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d): Free fail, EntryId=%d\n", __FUNCTION__, __LINE__, enID);
            bRet = 0;
        }
    }
    else if(enID <= E_MDRV_MULTI_INST_ENTRY_ID_DISP)
    {
        EN_MDRV_MULTI_INST_SCLDMA_DATA_ID_TYPE enSlcmdaId;
        if( _MDrv_MultiInst_Trans_ScldmaID(enID, &enSlcmdaId) && pPrivate_Data)
        {
            ST_MDRV_MULTI_INST_SCLDMA_ENTRY_CONFIG *pEntry = NULL;

            MULTIINST_WAIT_SEM(gMultiInstScldmaEntrySem[enSlcmdaId]);
            pEntry = (ST_MDRV_MULTI_INST_SCLDMA_ENTRY_CONFIG *)pPrivate_Data;
            pEntry->bUsed = 0;
            MsOS_Memset(&pEntry->stData, 0, sizeof(ST_MDRV_MULTI_INST_SCLDMA_DATA_TYPE));
            MULTIINST_RELEASE_SEM(gMultiInstScldmaEntrySem[enSlcmdaId]);

            bRet = 1;
        }
        else
        {
            SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d): Free fail EntryId=%d\n", __FUNCTION__, __LINE__, enID);
            bRet = 0;
        }
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d): Free fail, EntryId=%d\n", __FUNCTION__, __LINE__, enID);
        bRet = 0;
    }

    return bRet;
}

EN_MDRV_MULTI_INST_STATUS_TYPE MDrv_MultiInst_Entry_FlashData(
    EN_MDRV_MULTI_INST_ENTRY_ID_TYPE enID,
    void *pPrivateData,
    EN_MDRV_MULTI_INST_CMD_TYPE enCmd,
    void *pData)
{
    EN_MDRV_MULTI_INST_STATUS_TYPE enRet = E_MDRV_MULTI_INST_STATUS_SUCCESS;
    SCL_DBG(SCL_DBG_LV_MULTI_INST_FUNC()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d)\n", __FUNCTION__, __LINE__);

    if(enID <= E_MDRV_MULTI_INST_ENTRY_ID_HVSP3)
    {
        EN_MDRV_MULTI_INST_HVSP_DATA_ID_TYPE enHvspId;
        ST_MDRV_MULTI_INST_HVSP_ENTRY_CONFIG *pEntry =  (ST_MDRV_MULTI_INST_HVSP_ENTRY_CONFIG *)pPrivateData;
        if(_MDrv_MultiInst_Trans_HvspID(enID, &enHvspId))
        {
            enRet = _MDrv_MultiInst_Entry_FlashHvspData(enHvspId, pEntry, enCmd, pData);
        }
        else
        {
            SCL_DBG(SCL_DBG_LV_MULTI_INST_ENTRY()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d): FlashData fail, PrivateId=%lx\n", __FUNCTION__, __LINE__, pEntry->s32PivateId);
            enRet = E_MDRV_MULTI_INST_STATUS_FAIL;
        }
    }
    else if(enID <= E_MDRV_MULTI_INST_ENTRY_ID_DISP)
    {
        EN_MDRV_MULTI_INST_SCLDMA_DATA_ID_TYPE enSclmdaId;
        ST_MDRV_MULTI_INST_SCLDMA_ENTRY_CONFIG *pEntry =  (ST_MDRV_MULTI_INST_SCLDMA_ENTRY_CONFIG *)pPrivateData;
        if(_MDrv_MultiInst_Trans_ScldmaID(enID, &enSclmdaId ))
        {
            enRet = _MDrv_MultiInst_Entry_FlashScldmaData(enSclmdaId, pEntry, enCmd, pData);
        }
        else
        {
            SCL_DBG(SCL_DBG_LV_MULTI_INST_ENTRY()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d): FlashData fail, PrivateId=%lx\n", __FUNCTION__, __LINE__, pEntry->s32PivateId);
            enRet = E_MDRV_MULTI_INST_STATUS_FAIL;
        }
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d): FlashData fail, EntryId=%d\n", __FUNCTION__, __LINE__, enID);
        enRet = E_MDRV_MULTI_INST_STATUS_FAIL;
    }

    return enRet;
}


unsigned char MDrv_MultiInst_Entry_GetPirvateId(EN_MDRV_MULTI_INST_ENTRY_ID_TYPE enID, void *pPrivateData, signed long *ps32PrivateId)
{
    unsigned char bRet;

    SCL_DBG(SCL_DBG_LV_MULTI_INST_FUNC()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d)\n", __FUNCTION__, __LINE__);

    if(enID <= E_MDRV_MULTI_INST_ENTRY_ID_HVSP3)
    {
        ST_MDRV_MULTI_INST_HVSP_ENTRY_CONFIG *pEntry =  (ST_MDRV_MULTI_INST_HVSP_ENTRY_CONFIG *)pPrivateData;

        if(pEntry)
        {
            *ps32PrivateId = pEntry->s32PivateId;
             bRet = 1;
        }
        else
        {
            SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d): GetPrivateID  fail, EntryId=%d\n", __FUNCTION__, __LINE__, enID);
            *ps32PrivateId = -1;
            bRet = 0;
        }
    }
    else if(enID <= E_MDRV_MULTI_INST_ENTRY_ID_DISP)
    {
        ST_MDRV_MULTI_INST_SCLDMA_ENTRY_CONFIG *pEntry =  (ST_MDRV_MULTI_INST_SCLDMA_ENTRY_CONFIG *)pPrivateData;
        if(pEntry)
        {
            *ps32PrivateId = pEntry->s32PivateId;
            bRet = 1;
        }
        else
        {
            SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d): GetPrivateID  fail, EntryId=%d\n", __FUNCTION__, __LINE__, enID);
            *ps32PrivateId = -1;
            bRet = 0;
        }
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d): GetPrivateID  fail, EntryId:%d\n", __FUNCTION__, __LINE__, enID);
        bRet = 0;
    }

    SCL_DBG(SCL_DBG_LV_MULTI_INST_ENTRY()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d) EntryId:%x PrivateID=%lx\n",
        __FUNCTION__, __LINE__, enID, *ps32PrivateId);
    return bRet;
}

EN_MDRV_MULTI_INST_STATUS_TYPE MDrv_MultiInst_Etnry_IsFree(EN_MDRV_MULTI_INST_ENTRY_ID_TYPE enID, void *pPrivateData)
{
    EN_MDRV_MULTI_INST_STATUS_TYPE enIsFree;
    EN_MDRV_MULTI_INST_LOCK_ID_TYPE enLock_ID;

    SCL_DBG(SCL_DBG_LV_MULTI_INST_FUNC()&(Get_DBGMG_MULTI(enID)), "[MULTIINST]%s(%d)\n", __FUNCTION__, __LINE__);

    if(enID <= E_MDRV_MULTI_INST_ENTRY_ID_HVSP3)
    {
        ST_MDRV_MULTI_INST_HVSP_ENTRY_CONFIG *pEntry =  (ST_MDRV_MULTI_INST_HVSP_ENTRY_CONFIG *)pPrivateData;

        enLock_ID = (enID == E_MDRV_MULTI_INST_ENTRY_ID_HVSP1) ? E_MDRV_MULTI_INST_LOCK_ID_SC_1_2 :
                    (enID == E_MDRV_MULTI_INST_ENTRY_ID_HVSP2) ? E_MDRV_MULTI_INST_LOCK_ID_SC_1_2 :
                    (enID == E_MDRV_MULTI_INST_ENTRY_ID_HVSP3) ? E_MDRV_MULTI_INST_LOCK_ID_SC_3   :
                                                                 E_MDRV_MULTI_INST_LOCK_ID_MAX;
        if(pEntry)
        {
            if( MDrv_MultiInst_Lock_IsFree(enLock_ID, pEntry->s32PivateId) )
            {
                enIsFree = E_MDRV_MULTI_INST_STATUS_SUCCESS;
            }
            else
            {
                enIsFree = E_MDRV_MULTI_INST_STATUS_LOCKED;
            }
        }
        else
        {
            SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d): EntryIsFree  fail, EntryId:%d\n", __FUNCTION__, __LINE__, enID);
            enIsFree = E_MDRV_MULTI_INST_STATUS_FAIL;
        }
    }
    else if(enID <= E_MDRV_MULTI_INST_ENTRY_ID_DISP)
    {
        ST_MDRV_MULTI_INST_SCLDMA_ENTRY_CONFIG *pEntry =  (ST_MDRV_MULTI_INST_SCLDMA_ENTRY_CONFIG *)pPrivateData;
        enLock_ID = (enID == E_MDRV_MULTI_INST_ENTRY_ID_SCLDMA1) ? E_MDRV_MULTI_INST_LOCK_ID_SC_1_2 :
                    (enID == E_MDRV_MULTI_INST_ENTRY_ID_SCLDMA2) ? E_MDRV_MULTI_INST_LOCK_ID_SC_1_2 :
                    (enID == E_MDRV_MULTI_INST_ENTRY_ID_SCLDMA3) ? E_MDRV_MULTI_INST_LOCK_ID_SC_3   :
                    (enID == E_MDRV_MULTI_INST_ENTRY_ID_DISP)    ? E_MDRV_MULTI_INST_LOCK_ID_DISP   :
                                                                   E_MDRV_MULTI_INST_LOCK_ID_MAX;
        if(pEntry)
        {
            if( MDrv_MultiInst_Lock_IsFree(enLock_ID, pEntry->s32PivateId) )
            {
                enIsFree = E_MDRV_MULTI_INST_STATUS_SUCCESS;
            }
            else
            {
                enIsFree = E_MDRV_MULTI_INST_STATUS_LOCKED;
            }
        }
        else
        {
            SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d): EntryIsFree  fail, EntryId:%d\n", __FUNCTION__, __LINE__, enID);
            enIsFree = E_MDRV_MULTI_INST_STATUS_FAIL;
        }
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_MULTI_INST_ERR, "[MULTIINST]%s(%d): EntryIsFree  fail, EntryId:%d\n", __FUNCTION__, __LINE__, enID);
        enIsFree = E_MDRV_MULTI_INST_STATUS_FAIL;
    }

    return enIsFree;
}
