/*
 * drv_camclk.c- Sigmastar
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

#define DRV_CAMCLK_C
#include "cam_os_wrapper.h"
#include "drv_camclk_st.h"
#include "drv_camclk.h"
#include "drv_camclk_Api.h"
#include "hal_camclk_if.h"
#include "camclk_dbg.h"

#define CID_HANDLER_MUGIC         0x28520000
#define CAMCLK_HANDLER_ENTRY_SIZE (sizeof(CAMCLK_Handler))
u16 gu16HandlerCnt;
CAM_OS_LIST_HEAD(gstClkListHead);
CamOsMemCache_t    gstPoolCfg = {{0, 0}};
HalCamClkTopClk_t  gstCamClkTopCurrent[HAL_CAMCLK_SRC_Id_MAX];
HalCamClkTopClk_t *gCamClkTopCurrent = gstCamClkTopCurrent;

void _DrvCamClkCheckUnregisterHandler(void)
{
    struct CamOsListHead_t *pos;
    CAMCLK_Handler *        pCfg;

    CAM_OS_LIST_FOR_EACH(pos, &gstClkListHead)
    {
        pCfg = CAM_OS_LIST_ENTRY(pos, CAMCLK_Handler, stList);
        CAMCLKERR("[%s @ %d] %s Clk_Id:%d bEn:%hhd\n", __FUNCTION__, __LINE__, pCfg->pDevName, pCfg->u32ClkId,
                  pCfg->bEnable);
        CamClkUnregister((void *)pCfg);
    }
}
CAMCLK_RET_e _CamClkCheckHandlerLegal(CAMCLK_Handler *pCheckp)
{
    struct CamOsListHead_t *pos;
    CAMCLK_Handler *        pCfg;

    CAM_OS_LIST_FOR_EACH(pos, &gstClkListHead)
    {
        pCfg = CAM_OS_LIST_ENTRY(pos, CAMCLK_Handler, stList);
        if (pCheckp == pCfg)
        {
            CAMCLKERR("[%s @ %d] ALLOCATE Handler pool is crash!!!. same as %s Clk_Id:%d bEn:%hhd\n", __FUNCTION__,
                      __LINE__, pCfg->pDevName, pCfg->u32ClkId, pCfg->bEnable);
            return CAMCLK_RET_FAIL;
        }
    }
    CAMCLKDBG("[%s @ %d] ALLOCATE Handler pool is ok. @:%p\n", __FUNCTION__, __LINE__, pCheckp);
    return CAMCLK_RET_OK;
}
//================================================================================
CAMCLK_RET_e CamClkInit(void)
{
    CAMCLK_RET_e enRet = CAMCLK_RET_OK;

    CAMCLKINFO("[%s @ %d]\n", __FUNCTION__, __LINE__);
    // coverity[array_free]
    if (CamOsMemCacheCreate(&gstPoolCfg, "CAMCLK_POOL", CAMCLK_HANDLER_ENTRY_SIZE, 0))
    {
        CAMCLKERR("[%s @ %d] ALLOCATE Handler pool fail.\n", __FUNCTION__, __LINE__);
        return CAMCLK_RET_FAIL;
    }
    DrvCamClkImplInit();
    gu16HandlerCnt = 0;
    return enRet;
}
CAMCLK_RET_e CamClkDeinit(void)
{
    CAMCLK_RET_e enRet = CAMCLK_RET_OK;
    CAMCLKINFO("[%s @ %d]\n", __FUNCTION__, __LINE__);
    DrvCamClkImplDeinit();
    if (gstPoolCfg.nPriv[0] || gstPoolCfg.nPriv[1])
    {
        if (gu16HandlerCnt != 0)
        {
            CAMCLKERR("[%s @ %d] There are ctx w/o Unregister!!. Maybe Memory leak.\n", __FUNCTION__, __LINE__);
            _DrvCamClkCheckUnregisterHandler();
        }
        CamOsMemCacheDestroy(&gstPoolCfg);
    }
    return enRet;
}
u32 CamClkParentGet(u32 u32ClkId)
{
    DrvCamClkGetParent_t stParentCfg;

    stParentCfg.u32Id = u32ClkId;
    DrvCamClkImplGetParent((void *)&stParentCfg);
    return (u32)stParentCfg.u32ParentId;
}
CAMCLK_RET_e DrvCamClkResume(void)
{
    CAMCLK_RET_e               enRet = CAMCLK_RET_OK;
    struct CamOsListHead_t *   pos;
    CAMCLK_Handler *           pCfg;
    DrvCamClkSetParent_t       stCfg;
    DrvCamClkSetAdjRoundRate_t stAdjCfg;

    CAM_OS_LIST_FOR_EACH(pos, &gstClkListHead)
    {
        pCfg = CAM_OS_LIST_ENTRY(pos, CAMCLK_Handler, stList);
        if (gCamClkSrcNode[pCfg->u32ClkId].u8ClkType == HAL_CAMCLK_TYPE_COMPLEX)
        {
            stAdjCfg.u32Id       = pCfg->u32ClkId;
            stAdjCfg.u32Freq     = pCfg->u32RoundRate;
            stAdjCfg.u32ParentId = pCfg->u32CurrentParent;
            enRet                = DrvCamClkImplSetAdjustRate(&stAdjCfg);
        }
        else if (gCamClkSrcNode[pCfg->u32ClkId].u8ClkType == HAL_CAMCLK_TYPE_COMPOSITE)
        {
            stCfg.u32Id         = pCfg->u32ClkId;
            stCfg.u32ParentId   = pCfg->u32CurrentParent;
            stCfg.u32ParentFreq = 0;
            enRet               = DrvCamClkImplSetParent(&stCfg);
        }
    }
    DrvCamClkImplRestoreClk();
    return enRet;
}
CAMCLK_RET_e CamClkRegister(u8 *pDevName, u32 u32ClkId, void **pvHandlerId)
{
    CAMCLK_RET_e    enRet     = CAMCLK_RET_OK;
    static u8       u8RollNum = 0;
    CAMCLK_Handler *pCfg;

    CAMCLKINFO("[%s @ %d] Dev:%s CID:%d\n", __FUNCTION__, __LINE__, pDevName, u32ClkId);
    if (gCamClkTopCurrent)
    {
        pCfg = CamOsMemCacheAlloc(&gstPoolCfg);
        if (pCfg && (CAMCLK_RET_OK == _CamClkCheckHandlerLegal(pCfg)))
        {
            gu16HandlerCnt++;
            *pvHandlerId       = (void *)pCfg;
            pCfg->u32HandlerId = (CID_HANDLER_MUGIC | u8RollNum | (pCfg->u32ClkId << 8));
            u8RollNum++;
            pCfg->u32ClkId = u32ClkId;
            strcpy((char *)pCfg->pDevName, (char *)pDevName);
            pCfg->bEnable             = 0;
            pCfg->u32RoundRate        = CamClkRateGet(u32ClkId);
            pCfg->u32CurrentParent    = CamClkParentGet(u32ClkId);
            pCfg->attribute.u32Parent = 0;
            CAM_CLK_LOCK_SEM();
            CAM_OS_LIST_ADD_TAIL(&(pCfg->stList), &gstClkListHead);
            CAM_CLK_UNLOCK_SEM();
        }
        else
        {
            enRet        = CAMCLK_RET_FAIL;
            *pvHandlerId = 0;
            CAMCLKERR("[%s @ %d] ALLOCATE Handler pool fail.\n", __FUNCTION__, __LINE__);
        }
    }
    else
    {
        enRet        = CAMCLK_RET_FAIL;
        *pvHandlerId = 0;
        CAMCLKERR("[%s @ %d] W/O Init!!.\n", __FUNCTION__, __LINE__);
    }
    // coverity[leaked_storage]
    return enRet;
}
CAMCLK_RET_e CamClkUnregister(void *pvHandlerId)
{
    CAMCLK_RET_e    enRet = CAMCLK_RET_OK;
    CAMCLK_Handler *pCtx  = (CAMCLK_Handler *)pvHandlerId;

    if (pCtx && ((pCtx->u32HandlerId & CID_HANDLER_MUGIC) == CID_HANDLER_MUGIC))
    {
        pCtx->u32HandlerId = 0;
        if (pCtx->bEnable)
        {
            CAMCLKWARN("[%s @ %d] W/O Disable Dev:%s CID:%d\n", __FUNCTION__, __LINE__, pCtx->pDevName, pCtx->u32ClkId);
        }
        else
        {
            CAMCLKINFO("[%s @ %d] Dev:%s CID:%d\n", __FUNCTION__, __LINE__, pCtx->pDevName, pCtx->u32ClkId);
        }
        CAM_CLK_LOCK_SEM();
        CAM_OS_LIST_DEL(&(pCtx->stList));
        CAM_CLK_UNLOCK_SEM();
        CamOsMemCacheFree(&gstPoolCfg, pCtx);
        if (gu16HandlerCnt)
        {
            gu16HandlerCnt--;
        }
    }
    else
    {
        enRet = CAMCLK_RET_FAIL;
        CAMCLKERR("[%s @ %d] Handler Not exist.\n", __FUNCTION__, __LINE__);
    }
    return enRet;
}

CAMCLK_RET_e CamClkAttrGet(void *pvHandlerId, CAMCLK_Get_Attribute *pCfg)
{
    CAMCLK_RET_e            enRet = CAMCLK_RET_OK;
    DrvCamClkGetAllParent_t stParent;
    CAMCLK_Handler *        pCtx = (CAMCLK_Handler *)pvHandlerId;

    if (pCtx && ((pCtx->u32HandlerId & CID_HANDLER_MUGIC) == CID_HANDLER_MUGIC))
    {
        if (pCtx->attribute.u32Parent == 0)
        {
            pCfg->u32Rate          = CamClkRateGet(pCtx->u32ClkId);
            pCfg->u32CurrentParent = CamClkParentGet(pCtx->u32ClkId);
        }
        else
        {
            pCfg->u32Rate          = pCtx->u32RoundRate;
            pCfg->u32CurrentParent = pCtx->u32CurrentParent;
        }
        CAMCLKINFO("[%s @ %d] ClkId = %d Rate:%lu\n", __FUNCTION__, __LINE__, (u32)pvHandlerId, pCfg->u32Rate);
        stParent.u32Id = pCtx->u32ClkId;
        DrvCamClkImplGetIdAllParent(&stParent);
        pCfg->u32NodeCount = stParent.u32ParentCount;
        memcpy(pCfg->u32Parent, stParent.u32ParentId, pCfg->u32NodeCount * sizeof(HalCamClkSrcId_e));
    }
    else
    {
        enRet = CAMCLK_RET_FAIL;
        CAMCLKERR("[%s @ %d] Handler Not exist.\n", __FUNCTION__, __LINE__);
    }
    return enRet;
}
CAMCLK_RET_e CamClkGetOnOff(void *pvHandlerId, u8 *pbEn)
{
    CAMCLK_RET_e    enRet = CAMCLK_RET_OK;
    CAMCLK_Handler *pCtx  = (CAMCLK_Handler *)pvHandlerId;

    if (pCtx && ((pCtx->u32HandlerId & CID_HANDLER_MUGIC) == CID_HANDLER_MUGIC))
    {
        *pbEn = pCtx->bEnable;
        CAMCLKINFO("[%s @ %d] ClkId = %d OnOff:%hhu\n", __FUNCTION__, __LINE__, (u32)pvHandlerId, *pbEn);
    }
    else
    {
        enRet = CAMCLK_RET_FAIL;
        CAMCLKERR("[%s @ %d] Handler Not exist.\n", __FUNCTION__, __LINE__);
    }
    return enRet;
}
CAMCLK_RET_e CamClkAttrSet(void *pvHandlerId, CAMCLK_Set_Attribute *pCfg)
{
    CAMCLK_RET_e                  enRet = CAMCLK_RET_OK;
    DrvCamClkSetParent_t          stCfg;
    DrvCamClkGetRoundRateParent_t stRoundCfg;
    DrvCamClkSetAdjRoundRate_t    stAdjCfg;
    CAMCLK_Handler *              pCtx = (CAMCLK_Handler *)pvHandlerId;
    if (pCtx && ((pCtx->u32HandlerId & CID_HANDLER_MUGIC) == CID_HANDLER_MUGIC))
    {
        CAM_CLK_LOCK_SEM();
        CAM_OS_LIST_DEL(&(pCtx->stList));
        CAM_OS_LIST_ADD_TAIL(&(pCtx->stList), &gstClkListHead);
        CAM_CLK_UNLOCK_SEM();
        if (pCfg->eSetType == CAMCLK_SET_ATTR_RATE)
        {
            CAMCLKINFO("[%s @ %d] HandlerId = %p RoundType:%d Rate=%lu\n", __FUNCTION__, __LINE__, pvHandlerId,
                       pCfg->eRoundType, pCfg->attribute.u32Rate);
            stRoundCfg.u32Freq = pCfg->attribute.u32Rate;
            stRoundCfg.u32Id   = pCtx->u32ClkId;
            stRoundCfg.enType  = pCfg->eRoundType;
            if (gCamClkSrcNode[pCtx->u32ClkId].u8ClkType == HAL_CAMCLK_TYPE_COMPLEX)
            {
                DrvCamClkImplGetAdjustRoundRate(&stRoundCfg);
            }
            else
            {
                enRet = DrvCamClkImplGetRoundRateParent(&stRoundCfg);
            }

            if (enRet)
            {
                CAMCLKERR("[%s @ %d] GetRoundRate FAIL.\n", __FUNCTION__, __LINE__);
                goto ReturnBack;
            }
        }
        stCfg.u32Id = pCtx->u32ClkId;
        if (pCfg->eSetType == CAMCLK_SET_ATTR_RATE)
        {
            if (gCamClkSrcNode[pCtx->u32ClkId].u8ClkType == HAL_CAMCLK_TYPE_COMPLEX)
            {
                stAdjCfg.u32Id     = pCtx->u32ClkId;
                pCtx->u32RoundRate = stAdjCfg.u32Freq = stRoundCfg.u32RetFreq;
                pCtx->u32CurrentParent = stAdjCfg.u32ParentId = stRoundCfg.u32ParentId;
                enRet                                         = DrvCamClkImplSetAdjustRate(&stAdjCfg);
                goto ReturnBack;
            }
            else
            {
                pCtx->u32CurrentParent = stCfg.u32ParentId = stRoundCfg.u32ParentId;
                pCtx->attribute.u32Rate                    = pCfg->attribute.u32Rate;
                pCtx->u32RoundRate = stCfg.u32ParentFreq = stRoundCfg.u32RetFreq;
                CAMCLKINFO("[%s @ %d] ClkId = %d New Parent:%d\n", __FUNCTION__, __LINE__, pCtx->u32ClkId,
                           stCfg.u32ParentId);
            }
        }
        else
        {
            CAMCLKINFO("[%s @ %d] ClkId = %d Parent:%d\n", __FUNCTION__, __LINE__, pCtx->u32ClkId,
                       pCfg->attribute.u32Parent);
            pCtx->attribute.u32Parent = stCfg.u32ParentId = pCfg->attribute.u32Parent;
            stCfg.u32ParentFreq                           = 0;
        }
        enRet = DrvCamClkImplSetParent(&stCfg);
        if (enRet == CAMCLK_RET_OK && pCfg->eSetType == CAMCLK_SET_ATTR_PARENT)
        {
            pCtx->u32RoundRate     = CamClkRateGet(pCtx->u32ClkId);
            pCtx->u32CurrentParent = CamClkParentGet(pCtx->u32ClkId);
        }
    }
    else
    {
        enRet = CAMCLK_RET_FAIL;
        CAMCLKERR("[%s @ %d] Handler Not exist.\n", __FUNCTION__, __LINE__);
    }
ReturnBack:
    return enRet;
}
CAMCLK_RET_e CamClkSetOnOff(void *pvHandlerId, u8 bEn)
{
    CAMCLK_RET_e        enRet = CAMCLK_RET_OK;
    DrvCamClkSetOnOff_t stCfg;
    CAMCLK_Handler *    pCtx = (CAMCLK_Handler *)pvHandlerId;

    CAMCLKINFO("[%s @ %d] Handler = %p bEn = %hhd\n", __FUNCTION__, __LINE__, (u32)pvHandlerId, bEn);
    if (pCtx && ((pCtx->u32HandlerId & CID_HANDLER_MUGIC) == CID_HANDLER_MUGIC))
    {
        if ((bEn == 0 && pCtx->bEnable == 0) || (bEn == 1 && pCtx->bEnable == 1))
        {
            enRet = CAMCLK_RET_FAIL;
            if ((bEn == 0 && pCtx->bEnable == 0))
            {
                CAMCLKERR("[%s @ %d] %s Handler not enable once.\n", __FUNCTION__, __LINE__, pCtx->pDevName);
            }
            else
            {
                CAMCLKERR("[%s @ %d] %s Handler already enable.\n", __FUNCTION__, __LINE__, pCtx->pDevName);
            }
        }
        else
        {
            CAM_CLK_LOCK_SEM();
            CAM_OS_LIST_DEL(&(pCtx->stList));
            CAM_OS_LIST_ADD_TAIL(&(pCtx->stList), &gstClkListHead);
            CAM_CLK_UNLOCK_SEM();
            pCtx->bEnable = stCfg.bEn = bEn;
            stCfg.u32Id               = pCtx->u32ClkId;
            enRet                     = DrvCamClkImplSetOnOff(&stCfg);
        }
    }
    else
    {
        enRet = CAMCLK_RET_FAIL;
        CAMCLKERR("[%s @ %d] Handler Not exist.\n", __FUNCTION__, __LINE__);
    }
    return enRet;
}
u32 CamClkRateGet(u32 u32ClkId)
{
    DrvCamClkGetRate_t stCfg;

    stCfg.u32Id = u32ClkId;
    DrvCamClkImplGetRate(&stCfg);
    CAMCLKINFO("[%s @ %d] ClkId = %d Rate:%lu\n", __FUNCTION__, __LINE__, u32ClkId, stCfg.u32Freq);

    return stCfg.u32Freq;
}
