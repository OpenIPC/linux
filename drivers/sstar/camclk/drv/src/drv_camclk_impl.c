/*
 * drv_camclk_impl.c- Sigmastar
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

#define HAL_CAMCLK_C
#include "hal_camclk_if.h"
#include "hal_camclk.h"
#include "hal_camclk_lpll_tbl.h"
#include "camclk_dbg.h"
#include "hal_camclk_util.h"
#include "drv_camclk.h"
CamOsTsem_t gstTsem;
u32         gu32CamclkOsLock;
u32         gu32CamclkOsLockNum = 1;
#define CAMCLK_1M 1000000

CAMCLK_RET_e _DrvCamClkImplSwInit(void)
{
    CAMCLK_RET_e enRet = CAMCLK_RET_OK;
    memset(gCamClkTopCurrent, 0, sizeof(HalCamClkTopClk_t) * HAL_CAMCLK_SRC_Id_MAX);
    CamOsTsemInit(&gstTsem, 1);
    return enRet;
}
CAMCLK_RET_e _DrvCamClkImplSwDeinit(void)
{
    CAMCLK_RET_e enRet = CAMCLK_RET_OK;
    CamOsTsemDeinit(&gstTsem);
    return enRet;
}

CAMCLK_RET_e _DrvCamClkImplSetAutoEnable(HalCamClkSrcId_e u32Id)
{
    CAMCLK_RET_e enRet = CAMCLK_RET_OK;
#ifndef CONFIG_NOTTOAUTOENABLE
    DrvCamClkSetOnOff_t stCfg;

    if (gCamClkSrcNode[u32Id].u8ClkType == HAL_CAMCLK_TYPE_COMPOSITE)
    {
        CAM_CLK_INVALIDATE((void *)gCamClkTopCurrent);
#ifndef CONFIG_CAMCLK_AUTOENABLE
        if (gCamClkSrcNode[u32Id].attribute.stComposite.u8auto && (gCamClkTopCurrent[u32Id].u8RefCnt == 0))
#else
        if (gCamClkTopCurrent[u32Id].u8RefCnt == 0)
#endif
        {
            stCfg.bEn   = 1;
            stCfg.u32Id = u32Id;
            DrvCamClkImplSetOnOff(&stCfg);
        }
    }
#endif
    return enRet;
}
CAMCLK_RET_e _DrvCamClkImplHwInit(void)
{
    CAMCLK_RET_e     enRet = CAMCLK_RET_OK;
    HalCamClkSrcId_e u32Id;

    for (u32Id = 0; u32Id < HAL_CAMCLK_SRC_Id_MAX; u32Id++)
    {
        _DrvCamClkImplSetAutoEnable(u32Id);
        if (gCamClkSrcNode[u32Id].u8ClkType == HAL_CAMCLK_TYPE_COMPLEX)
        {
            if (gCamClkSrcNode[u32Id].attribute.stComplex.ptOps->PfnSetAdjInit)
            {
                gCamClkSrcNode[u32Id].attribute.stComplex.ptOps->PfnSetAdjInit(NULL);
            }
        }
    }
    return enRet;
}
CAMCLK_RET_e _DrvCamClkImplHwDeinit(void)
{
    CAMCLK_RET_e     enRet = CAMCLK_RET_OK;
    HalCamClkSrcId_e u32Id;
    // ToDo: disable all clock.
    for (u32Id = 0; u32Id < HAL_CAMCLK_SRC_Id_MAX; u32Id++)
    {
        if (gCamClkSrcNode[u32Id].u8ClkType == HAL_CAMCLK_TYPE_COMPLEX)
        {
            if (gCamClkSrcNode[u32Id].attribute.stComplex.ptOps->PfnSetAdjDeInit)
            {
                gCamClkSrcNode[u32Id].attribute.stComplex.ptOps->PfnSetAdjDeInit(NULL);
            }
        }
    }
    return enRet;
}

u32 _DrvCamClkImplGetParentCount(HalCamClkSrcId_e u32Id)
{
    u32 u32ParentCount = 1;
    u32 idx;

    if (gCamClkSrcNode[u32Id].u8ClkType == HAL_CAMCLK_TYPE_COMPOSITE)
    {
        if ((gCamClkSrcNode[u32Id].attribute.stComposite.u8SelectWidth) > 0)
        {
            for (idx = 0; idx < (gCamClkSrcNode[u32Id].attribute.stComposite.u8SelectWidth); idx++)
            {
                u32ParentCount *= 2;
            }
        }
    }

    return u32ParentCount;
}

CAMCLK_RET_e _DrvCamClkImplGetAllParentId(HalCamClkSrcId_e u32Id, HalCamClkSrcId_e *u32ParentId, u32 u32Count)
{
    u32          u32idx;
    CAMCLK_RET_e enRet = CAMCLK_RET_OK;

    if (gCamClkSrcNode[u32Id].u8ClkType == HAL_CAMCLK_TYPE_COMPOSITE)
    {
        for (u32idx = 0; u32idx < u32Count; u32idx++)
        {
            u32ParentId[u32idx] = (HalCamClkSrcId_e)gCamClkSrcNode[u32Id].attribute.stComposite.u8parent[u32idx];
        }
    }
    else if (gCamClkSrcNode[u32Id].u8ClkType == HAL_CAMCLK_TYPE_FIXED_FACTOR)
    {
        u32ParentId[0] = gCamClkSrcNode[u32Id].attribute.stFixedFac.eParent;
    }
    else if (gCamClkSrcNode[u32Id].u8ClkType == HAL_CAMCLK_TYPE_COMPLEX)
    {
        u32ParentId[0] = gCamClkSrcNode[u32Id].attribute.stComplex.eParent;
    }
    else
    {
        u32ParentId[0] = HAL_CAMCLK_SRC_CLK_VOID;
    }
    return enRet;
}
CAMCLK_RET_e _DrvCamClkImplGetSelectMsk(HalCamClkSrcId_e u32Id, u16 *p16Select)
{
    CAMCLK_RET_e enRet = CAMCLK_RET_OK;
    u8           u8idx;
    u8           u8off;
    if (gCamClkSrcNode[u32Id].u8ClkType != HAL_CAMCLK_TYPE_COMPOSITE)
    {
        enRet = CAMCLK_RET_FAIL;
    }
    else
    {
        *p16Select = 0;
        u8off      = gCamClkSrcNode[u32Id].attribute.stComposite.u8SelectShift;
        for (u8idx = 0; u8idx < gCamClkSrcNode[u32Id].attribute.stComposite.u8SelectWidth; u8idx++)
        {
            *p16Select |= (0x1 << (u8idx + u8off));
        }
    }

    return enRet;
}
CAMCLK_RET_e DrvCamClkImplGetHwOnOff(HalCamClkSrcId_e u32Id, u8 *p8bEn)
{
    CAMCLK_RET_e            enRet = CAMCLK_RET_OK;
    u16                     u16off;
    HalCamClkComplexOnOff_t stCfg;

    if (gCamClkSrcNode[u32Id].u8ClkType != HAL_CAMCLK_TYPE_COMPOSITE
        && gCamClkSrcNode[u32Id].u8ClkType != HAL_CAMCLK_TYPE_COMPLEX)
    {
        *p8bEn = 2;
        enRet  = CAMCLK_RET_FAIL;
    }
    else if (gCamClkSrcNode[u32Id].u8ClkType == HAL_CAMCLK_TYPE_COMPLEX)
    {
        if (gCamClkSrcNode[u32Id].attribute.stComplex.ptOps->PfnGetAdjOnOff)
        {
            gCamClkSrcNode[u32Id].attribute.stComplex.ptOps->PfnGetAdjOnOff(&stCfg);
            *p8bEn = stCfg.bEn;
        }
        else
        {
            *p8bEn = 0;
            enRet  = CAMCLK_RET_FAIL;
        }
    }
    else
    {
        u16off = gCamClkSrcNode[u32Id].attribute.stComposite.u16Gated;
        if (u16off)
        {
            *p8bEn = (R2BYTE(gCamClkSrcNode[u32Id].attribute.stComposite.u32Reg) & u16off) ? 0 : 1;
        }
        else
        {
            *p8bEn = 2;
        }
    }

    return enRet;
}
CAMCLK_RET_e _DrvCamClkImplGetSelect(DrvCamClkSetParent_t *pCfg, u16 *p16Select)
{
    CAMCLK_RET_e enRet = CAMCLK_RET_OK;
    u8           u8idx;
    u8           u8off, u8wid;
    if (gCamClkSrcNode[pCfg->u32Id].u8ClkType != HAL_CAMCLK_TYPE_COMPOSITE)
    {
        enRet = CAMCLK_RET_FAIL;
    }
    else
    {
        *p16Select = 0;
        u8off      = gCamClkSrcNode[pCfg->u32Id].attribute.stComposite.u8SelectShift;
        u8wid      = 1;
        if ((gCamClkSrcNode[pCfg->u32Id].attribute.stComposite.u8SelectWidth) > 0)
        {
            for (u8idx = 0; u8idx < (gCamClkSrcNode[pCfg->u32Id].attribute.stComposite.u8SelectWidth); u8idx++)
            {
                u8wid *= 2;
            }
        }
        else
        {
            enRet = CAMCLK_RET_NOTSUPPORT;
            goto ReturnBack;
        }
        for (u8idx = 0; u8idx < u8wid; u8idx++)
        {
            if (pCfg->u32ParentId == gCamClkSrcNode[pCfg->u32Id].attribute.stComposite.u8parent[u8idx])
            {
                *p16Select = (u8idx << u8off);
                break;
            }
        }
    }
ReturnBack:
    return enRet;
}
CAMCLK_RET_e DrvCamClkImplGetHwSelect(HalCamClkSrcId_e u32Id, u16 *p16Select)
{
    CAMCLK_RET_e enRet = CAMCLK_RET_OK;
    u16          u16SelectMsk;
    if (gCamClkSrcNode[u32Id].u8ClkType == HAL_CAMCLK_TYPE_COMPOSITE)
    {
        _DrvCamClkImplGetSelectMsk(u32Id, &u16SelectMsk);
        *p16Select = (R2BYTE(gCamClkSrcNode[u32Id].attribute.stComposite.u32Reg) & u16SelectMsk);
        *p16Select = (*p16Select >> (gCamClkSrcNode[u32Id].attribute.stComposite.u8SelectShift));
    }
    else
    {
        *p16Select = 0;
        enRet      = CAMCLK_RET_NOTSUPPORT;
    }
    return enRet;
}
CAMCLK_RET_e DrvCamClkImplGetParent(void *pCfg)
{
    CAMCLK_RET_e          enRet  = CAMCLK_RET_OK;
    DrvCamClkGetParent_t *pstCfg = pCfg;
    u16                   u16Select;

    CAMCLKDBG("[%s @ %d] Id :%d\n", __FUNCTION__, __LINE__, pstCfg->u32Id);
    if (pstCfg->u32Id < HAL_CAMCLK_SRC_Id_MAX)
    {
        if (gCamClkSrcNode[pstCfg->u32Id].u8ClkType == HAL_CAMCLK_TYPE_COMPOSITE)
        {
            DrvCamClkImplGetHwSelect(pstCfg->u32Id, &u16Select);
            pstCfg->u32ParentId = gCamClkSrcNode[pstCfg->u32Id].attribute.stComposite.u8parent[u16Select];
        }
        else
        {
            _DrvCamClkImplGetAllParentId(pstCfg->u32Id, &pstCfg->u32ParentId, 1);
        }
    }
    else
    {
        enRet = CAMCLK_RET_FAIL;
    }
    return enRet;
}
// For Horizontal
CAMCLK_RET_e DrvCamClkImplGetIdAllParent(void *pCfg)
{
    CAMCLK_RET_e             enRet  = CAMCLK_RET_OK;
    DrvCamClkGetAllParent_t *pstCfg = pCfg;

    CAMCLKDBG("[%s @ %d] Id :%d\n", __FUNCTION__, __LINE__, pstCfg->u32Id);
    if (pstCfg->u32Id < HAL_CAMCLK_SRC_Id_MAX)
    {
        pstCfg->u32ParentCount = _DrvCamClkImplGetParentCount(pstCfg->u32Id);
        _DrvCamClkImplGetAllParentId(pstCfg->u32Id, pstCfg->u32ParentId, pstCfg->u32ParentCount);
    }
    else
    {
        enRet = CAMCLK_RET_FAIL;
    }
    return enRet;
}
// For Vertical
CAMCLK_RET_e _DrvCamClkImplGetAllParentList(void *pCfg)
{
    CAMCLK_RET_e             enRet  = CAMCLK_RET_OK;
    DrvCamClkGetAllParent_t *pstCfg = pCfg;
    HalCamClkSrcId_e         u32ParentId;
    DrvCamClkGetParent_t     stParentCfg;

    CAMCLKDBG("[%s @ %d] Id :%d\n", __FUNCTION__, __LINE__, pstCfg->u32Id);
    u32ParentId = pstCfg->u32Id;
    if (pstCfg->u32Id < HAL_CAMCLK_SRC_Id_MAX)
    {
        pstCfg->u32ParentCount = 1;
        pstCfg->u32ParentId[0] = pstCfg->u32Id;
        while (1)
        {
            if (u32ParentId != HAL_CAMCLK_SRC_CLK_VOID)
            {
                if (pstCfg->u32ParentCount >= MAX_CLK_SRC_PARENT_NODE_CNT)
                {
                    enRet = CAMCLK_RET_FAIL;
                    CAMCLKERR("[%s @ %d] ParentList > MAX\n", __FUNCTION__, __LINE__);
                    break;
                }
                stParentCfg.u32Id = u32ParentId;
                DrvCamClkImplGetParent((void *)&stParentCfg);
                u32ParentId                                 = stParentCfg.u32ParentId;
                pstCfg->u32ParentId[pstCfg->u32ParentCount] = u32ParentId;
                pstCfg->u32ParentCount++;
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        enRet = CAMCLK_RET_FAIL;
    }
    return enRet;
}
void _DrvCamClkImplGetParentFreq(HalCamClkSrcId_e u32Id, u32 *p32Freq)
{
    DrvCamClkGetRate_t   stRateCfg;
    DrvCamClkGetParent_t stPCfg;

    stPCfg.u32Id = u32Id;
    DrvCamClkImplGetParent(&stPCfg);
    stRateCfg.u32Id = stPCfg.u32ParentId;
    DrvCamClkImplGetRate(&stRateCfg);
    *p32Freq = stRateCfg.u32Freq;
}
void _DrvCamClkImplGetHeaderFreq(DrvCamClkGetAllParent_t *pCfg, u32 *u32Freq)
{
    u32                       u32Idx;
    HalCamClkGetComplexRate_t stPllCfg;

    for (u32Idx = 0; u32Idx < pCfg->u32ParentCount; u32Idx++)
    {
        if (gCamClkSrcNode[pCfg->u32ParentId[u32Idx]].u8ClkType == HAL_CAMCLK_TYPE_COMPLEX)
        {
            if (gCamClkSrcNode[pCfg->u32ParentId[u32Idx]].attribute.stComplex.ptOps->PfnGetAdjRate)
            {
                _DrvCamClkImplGetParentFreq(pCfg->u32ParentId[u32Idx], &stPllCfg.u32ParentRate);
                gCamClkSrcNode[pCfg->u32ParentId[u32Idx]].attribute.stComplex.ptOps->PfnGetAdjRate(&stPllCfg);
                *u32Freq = stPllCfg.u32Rate;
                break;
            }
        }
        if (gCamClkSrcNode[pCfg->u32ParentId[u32Idx]].u8ClkType == HAL_CAMCLK_TYPE_FIXED)
        {
            *u32Freq = gCamClkSrcNode[pCfg->u32ParentId[u32Idx]].attribute.stFixed.u32Freq;
            break;
        }
    }
}
void _DrvCamClkImplGetMultDiv(DrvCamClkGetAllParent_t *pCfg, u32 *u32Mult, u32 *u32Div)
{
    u32 u32Idx;
    *u32Mult = 1;
    *u32Div  = 1;
    for (u32Idx = 0; u32Idx < pCfg->u32ParentCount; u32Idx++)
    {
        if (gCamClkSrcNode[pCfg->u32ParentId[u32Idx]].u8ClkType == HAL_CAMCLK_TYPE_FIXED_FACTOR)
        {
            *u32Mult *= gCamClkSrcNode[pCfg->u32ParentId[u32Idx]].attribute.stFixedFac.u8Mult;
            *u32Div *= gCamClkSrcNode[pCfg->u32ParentId[u32Idx]].attribute.stFixedFac.u8Div;
        }
    }
}
void _DrvCamClkImplCalClkFreq(DrvCamClkGetAllParent_t *pCfg, DrvCamClkGetRate_t *pstCfg)
{
    u32 u32Mult, u32Div;
    _DrvCamClkImplGetHeaderFreq(pCfg, &pstCfg->u32Freq);
    _DrvCamClkImplGetMultDiv(pCfg, &u32Mult, &u32Div);
    pstCfg->u32Freq = (pstCfg->u32Freq / u32Div) * u32Mult;
}

void _DrvCamClkImplGetRateDiff(u32 u32Freq, u32 u32ParentFreq, DrvCamClkRateDiff_t *pDiff)
{
    if (u32ParentFreq >= u32Freq)
    {
        pDiff->bLess   = 0;
        pDiff->u32diff = u32ParentFreq - u32Freq;
    }
    else
    {
        pDiff->bLess   = 1;
        pDiff->u32diff = u32Freq - u32ParentFreq;
    }
}
void _DrvCamClkImplGetRoundRateParent(DrvCamClkGetRoundRateParent_t *pstCfg, DrvCamClkGetAllParent_t *pCfg,
                                      DrvCamClkRateDiff_t *pstDiff)
{
    u8  bGood = 0; // if condition match,first priority.
    u32 u32idx, u32TempIdx;
    u32 u32Diff = 0xFFFFFFFF;
    u32TempIdx  = 0;

    for (u32idx = 0; u32idx < pCfg->u32ParentCount; u32idx++)
    {
        if (pCfg->u32ParentId[u32idx] == HAL_CAMCLK_SRC_CLK_VOID)
        {
            continue;
        }
        if (pstDiff[u32idx].u32diff == 0)
        {
            u32Diff    = pstDiff[u32idx].u32diff;
            u32TempIdx = u32idx;
            if (gCamClkSrcNode[pCfg->u32ParentId[u32idx]].u8ClkType != HAL_CAMCLK_TYPE_COMPLEX)
            {
                break;
            }
        }
        else if (pstDiff[u32idx].u32diff < u32Diff)
        {
            if (pstCfg->enType == CAMCLK_ROUNDRATE_DOWN)
            {
                if (!pstDiff[u32idx].bLess && bGood)
                {
                    // if already find down rate,pass this up rate.
                    continue;
                }
                if (pstDiff[u32idx].bLess)
                {
                    bGood = 1;
                }
            }
            if (pstCfg->enType == CAMCLK_ROUNDRATE_UP)
            {
                if (pstDiff[u32idx].bLess && bGood)
                {
                    // if already find up rate,pass this down rate.
                    continue;
                }
                if (!pstDiff[u32idx].bLess)
                {
                    bGood = 1;
                }
            }
            u32Diff    = pstDiff[u32idx].u32diff;
            u32TempIdx = u32idx;
        }
    }
    pstCfg->u32ParentId = pCfg->u32ParentId[u32TempIdx];
    pstCfg->u32RetFreq  = (pstDiff[u32TempIdx].bLess) ? (pstCfg->u32Freq - u32Diff) : (pstCfg->u32Freq + u32Diff);
}

CAMCLK_RET_e DrvCamClkImplGetAdjustRoundRate(DrvCamClkGetRoundRateParent_t *pstCfg)
{
    // ToDo:complex pll roundrate ops
    HalCamClkGetComplexRoundRate_t stCfg;
    CAMCLKDBG("[%s @ %d] Id :%d input Freq:%d\n", __FUNCTION__, __LINE__, pstCfg->u32Id, pstCfg->u32Freq);
    if ((gCamClkSrcNode[pstCfg->u32Id].u8ClkType == HAL_CAMCLK_TYPE_COMPLEX)
        && gCamClkSrcNode[pstCfg->u32Id].attribute.stComplex.ptOps->PfnGetAdjRoundRate)
    {
        stCfg.u32Rate = pstCfg->u32Freq;
        gCamClkSrcNode[pstCfg->u32Id].attribute.stComplex.ptOps->PfnGetAdjRoundRate((void *)&stCfg);
        pstCfg->u32RetFreq  = stCfg.u32RoundRate;
        pstCfg->u32ParentId = gCamClkSrcNode[pstCfg->u32Id].attribute.stComplex.eParent;
        return CAMCLK_RET_OK;
    }
    else
    {
        pstCfg->u32RetFreq  = pstCfg->u32Freq;
        pstCfg->u32ParentId = gCamClkSrcNode[pstCfg->u32Id].attribute.stComplex.eParent;
    }
    return CAMCLK_RET_NOTSUPPORT;
}
CAMCLK_RET_e DrvCamClkImplSetAdjustRate(DrvCamClkSetAdjRoundRate_t *pstCfg)
{
    CAMCLK_RET_e              enRet = CAMCLK_RET_OK;
    HalCamClkSetComplexRate_t stCfg;
    CAMCLKDBG("[%s @ %d] Id :%d input Freq:%d\n", __FUNCTION__, __LINE__, pstCfg->u32Id, pstCfg->u32Freq);
    if ((gCamClkSrcNode[pstCfg->u32Id].u8ClkType == HAL_CAMCLK_TYPE_COMPLEX)
        && gCamClkSrcNode[pstCfg->u32Id].attribute.stComplex.ptOps->PfnSetAdjRate)
    {
        _DrvCamClkImplGetParentFreq(pstCfg->u32Id, &stCfg.u32ParentRate);
        stCfg.u32Rate = (pstCfg->u32Freq) ? pstCfg->u32Freq : stCfg.u32ParentRate;
        gCamClkSrcNode[pstCfg->u32Id].attribute.stComplex.ptOps->PfnSetAdjRate((void *)&stCfg);
    }
    else
    {
        enRet = CAMCLK_RET_NOTSUPPORT;
    }
    return enRet;
}

CAMCLK_RET_e _DrvCamClkImplSetParent(DrvCamClkSetParent_t *pCfg)
{
    CAMCLK_RET_e enRet        = CAMCLK_RET_OK;
    u16          u16SelectMsk = 0;
    u16          u16Select    = 0;

    CAMCLKDBG("[%s @ %d] Id :%d ParentId :%d\n", __FUNCTION__, __LINE__, pCfg->u32Id, pCfg->u32ParentId);
    enRet = _DrvCamClkImplGetSelectMsk(pCfg->u32Id, &u16SelectMsk);
    enRet = _DrvCamClkImplGetSelect(pCfg, &u16Select);
    if (enRet)
    {
        if (enRet == CAMCLK_RET_FAIL)
        {
            CAMCLKERR("[%s @ %d] Get HW InfoFail\n", __FUNCTION__, __LINE__);
        }
        goto ReturnBack;
    }
    else
    {
        CAMCLKDBG("[%s @ %d] u16Select :%hx u16SelectMsk :%hx\n", __FUNCTION__, __LINE__, u16Select, u16SelectMsk);
    }
    if (gCamClkSrcNode[pCfg->u32Id].attribute.stComposite.u16Glitch)
    {
        W2BYTEMSK(gCamClkSrcNode[pCfg->u32Id].attribute.stComposite.u32Reg, 0,
                  gCamClkSrcNode[pCfg->u32Id].attribute.stComposite.u16Glitch);
    }
    W2BYTEMSK(gCamClkSrcNode[pCfg->u32Id].attribute.stComposite.u32Reg, u16Select, u16SelectMsk);
    if (gCamClkSrcNode[pCfg->u32Id].attribute.stComposite.u16Glitch)
    {
        W2BYTEMSK(gCamClkSrcNode[pCfg->u32Id].attribute.stComposite.u32Reg,
                  gCamClkSrcNode[pCfg->u32Id].attribute.stComposite.u16Glitch,
                  gCamClkSrcNode[pCfg->u32Id].attribute.stComposite.u16Glitch);
    }
ReturnBack:
    return enRet;
}
void _DrvCamClkImplSetRefCnt(HalCamClkSrcId_e eId, u8 bEn)
{
    // CAM_CLK_LOCK_SEM();
    CAM_CLK_INVALIDATE((void *)gCamClkTopCurrent);
    if (bEn)
    {
        gCamClkTopCurrent[eId].u8RefCnt += bEn;
    }
    else
    {
        if (gCamClkTopCurrent[eId].u8RefCnt)
        {
            gCamClkTopCurrent[eId].u8RefCnt--;
        }
    }
    CAM_CLK_FLUSH((void *)gCamClkTopCurrent);
    // CAM_CLK_UNLOCK_SEM();
}
u32 _DrvCamClkImplCheckRefCnt(HalCamClkSrcId_e eId, u8 bEn)
{
    u32                     Ret = bEn;
    HalCamClkComplexOnOff_t stCfg;
    CAM_CLK_INVALIDATE((void *)gCamClkTopCurrent);
    if (gCamClkTopCurrent[eId].u8RefCnt == 0 && bEn)
    {
        if (gCamClkSrcNode[eId].u8ClkType == HAL_CAMCLK_TYPE_COMPOSITE)
        {
            if (!(R2BYTE(gCamClkSrcNode[eId].attribute.stComposite.u32Reg)
                  & gCamClkSrcNode[eId].attribute.stComposite.u16Gated))
            {
                Ret = 2;
            }
        }
        else if (gCamClkSrcNode[eId].u8ClkType == HAL_CAMCLK_TYPE_COMPLEX)
        {
            if (gCamClkSrcNode[eId].attribute.stComplex.ptOps->PfnGetAdjOnOff)
            {
                gCamClkSrcNode[eId].attribute.stComplex.ptOps->PfnGetAdjOnOff(&stCfg);
                if (stCfg.bEn)
                {
                    Ret = 2;
                }
            }
        }
    }
    return Ret;
}
void _DrvCamClkImplSetGated(HalCamClkSrcId_e eId, u8 bEn)
{
    HalCamClkComplexOnOff_t stCfg;
#ifdef CONFIG_NOTTOGATED
    if (bEn == 0)
    {
        return;
    }
#endif
    if (gCamClkSrcNode[eId].u8ClkType == HAL_CAMCLK_TYPE_COMPOSITE)
    {
        if (gCamClkSrcNode[eId].attribute.stComposite.u16Glitch)
        {
            W2BYTEMSK(gCamClkSrcNode[eId].attribute.stComposite.u32Reg, 0,
                      gCamClkSrcNode[eId].attribute.stComposite.u16Glitch);
        }
        if (gCamClkSrcNode[eId].attribute.stComposite.u16Gated)
        {
            W2BYTEMSK(gCamClkSrcNode[eId].attribute.stComposite.u32Reg,
                      bEn ? 0 : gCamClkSrcNode[eId].attribute.stComposite.u16Gated,
                      gCamClkSrcNode[eId].attribute.stComposite.u16Gated);
        }
        if (gCamClkSrcNode[eId].attribute.stComposite.u16Glitch)
        {
            W2BYTEMSK(gCamClkSrcNode[eId].attribute.stComposite.u32Reg,
                      gCamClkSrcNode[eId].attribute.stComposite.u16Glitch,
                      gCamClkSrcNode[eId].attribute.stComposite.u16Glitch);
        }
    }
    else if (gCamClkSrcNode[eId].u8ClkType == HAL_CAMCLK_TYPE_COMPLEX)
    {
        stCfg.bEn = bEn;
        if (gCamClkSrcNode[eId].attribute.stComplex.ptOps->PfnSetAdjOnOff)
        {
            // CAM_CLK_LOCK_SEM();
            gCamClkSrcNode[eId].attribute.stComplex.ptOps->PfnSetAdjOnOff(&stCfg);
            // CAM_CLK_UNLOCK_SEM();
        }
    }
}
void _DrvCamClkImplExecGated(HalCamClkSrcId_e eId, u8 bEn)
{
    CAM_CLK_INVALIDATE((void *)gCamClkTopCurrent);
    if (!((gCamClkTopCurrent[eId].u8RefCnt >= 1 && bEn) || (gCamClkTopCurrent[eId].u8RefCnt == 0 && !bEn)))
    {
        return;
    }
    _DrvCamClkImplSetGated(eId, bEn);
}

CAMCLK_RET_e _DrvCamClkImplResetRefCnt(DrvCamClkSetParent_t *pCfg)
{
    DrvCamClkGetAllParent_t stCfg, stCfgori;
    CAMCLK_RET_e            Ret      = CAMCLK_RET_OK;
    u8                      u8RefCnt = 0, idx, idy, idz;
    CAM_CLK_INVALIDATE((void *)gCamClkTopCurrent);
    if (gCamClkTopCurrent[pCfg->u32Id].u8RefCnt)
    {
        stCfg.u32Id    = pCfg->u32ParentId;
        stCfgori.u32Id = pCfg->u32Id;
        _DrvCamClkImplGetAllParentList(&stCfg);
        _DrvCamClkImplGetAllParentList(&stCfgori);
        if (pCfg->u32ParentId != stCfgori.u32ParentId[1])
        {
            // change parent
            CAMCLKDBG("[%s @ %d] Clk Source change,need to reset ref count\n", __FUNCTION__, __LINE__);
            u8RefCnt = gCamClkTopCurrent[pCfg->u32Id].u8RefCnt;
            for (idx = 0; idx < u8RefCnt; idx++)
            {
                for (idy = 0; idy < stCfg.u32ParentCount; idy++)
                {
                    _DrvCamClkImplSetRefCnt(stCfg.u32ParentId[idy], 1);
                    _DrvCamClkImplExecGated(stCfg.u32ParentId[idy], 1);
                    CAMCLKDBG("[%s @ %d] Id: %hhd RefCnt=%hhd\n", __FUNCTION__, __LINE__, stCfg.u32ParentId[idy],
                              gCamClkTopCurrent[stCfg.u32ParentId[idy]].u8RefCnt);
                }
                for (idz = 1; idz < stCfgori.u32ParentCount; idz++)
                {
                    _DrvCamClkImplSetRefCnt(stCfgori.u32ParentId[idz], 0);
                    _DrvCamClkImplExecGated(stCfgori.u32ParentId[idz], 0);
                    CAMCLKDBG("[%s @ %d]  Id: %hhd RefCnt=%hhd\n", __FUNCTION__, __LINE__, stCfgori.u32ParentId[idz],
                              gCamClkTopCurrent[stCfgori.u32ParentId[idz]].u8RefCnt);
                }
            }
        }
    }
    return Ret;
}
//========================================================================

CAMCLK_RET_e DrvCamClkImplGetRate(void *pCfg)
{
    CAMCLK_RET_e            enRet  = CAMCLK_RET_OK;
    DrvCamClkGetRate_t *    pstCfg = pCfg;
    DrvCamClkGetAllParent_t stCfg;

    if (pstCfg->u32Id < HAL_CAMCLK_SRC_Id_MAX)
    {
        stCfg.u32Id     = pstCfg->u32Id;
        pstCfg->u32Freq = 0;
        enRet           = _DrvCamClkImplGetAllParentList((void *)&stCfg);
        if (enRet)
        {
            goto ReturnBack;
        }
        _DrvCamClkImplCalClkFreq(&stCfg, pstCfg);
    }
    else
    {
        enRet = CAMCLK_RET_FAIL;
        goto ReturnBack;
    }
    CAMCLKDBG("[%s @ %d] Id :%d Freq:%u\n", __FUNCTION__, __LINE__, pstCfg->u32Id, (u32)(pstCfg->u32Freq));
ReturnBack:
    return enRet;
}
CAMCLK_RET_e DrvCamClkImplGetRoundRateParent(void *pCfg)
{
    CAMCLK_RET_e                   enRet  = CAMCLK_RET_OK;
    DrvCamClkGetRoundRateParent_t *pstCfg = pCfg;
    DrvCamClkGetAllParent_t        stCfg;
    DrvCamClkGetRate_t             stRateCfg;
    DrvCamClkRateDiff_t            stDiff[MAX_CLK_SRC_PARENT_NODE_CNT];
    u32                            u32idx;

    CAMCLKDBG("[%s @ %d] Id :%d input Freq:%d\n", __FUNCTION__, __LINE__, pstCfg->u32Id, pstCfg->u32Freq);
    stCfg.u32Id = pstCfg->u32Id;
    DrvCamClkImplGetIdAllParent((void *)&stCfg);
    if (stCfg.u32ParentCount > 0)
    {
        for (u32idx = 0; u32idx < stCfg.u32ParentCount; u32idx++)
        {
            if (stCfg.u32ParentId[u32idx] == HAL_CAMCLK_SRC_CLK_VOID)
            {
                continue;
            }
            stRateCfg.u32Id = stCfg.u32ParentId[u32idx];
            DrvCamClkImplGetRate(&stRateCfg);
            _DrvCamClkImplGetRateDiff(pstCfg->u32Freq, stRateCfg.u32Freq, &stDiff[u32idx]);
        }
        _DrvCamClkImplGetRoundRateParent(pstCfg, &stCfg, stDiff);
    }
    else
    {
        enRet = CAMCLK_RET_FAIL;
    }
    return enRet;
}
CAMCLK_RET_e _DrvCamClkImplCheckParent(DrvCamClkSetParent_t *pstCfg)
{
    u8 idx;
    for (idx = 0; idx < MAX_CLK_SRC_PARENT_NODE_CNT; idx++)
    {
        if (gCamClkSrcNode[pstCfg->u32Id].attribute.stComposite.u8parent[idx] == pstCfg->u32ParentId)
        {
            return CAMCLK_RET_OK;
        }
    }
    return CAMCLK_RET_FAIL;
}
CAMCLK_RET_e DrvCamClkImplSetParent(void *pCfg)
{
    CAMCLK_RET_e          enRet  = CAMCLK_RET_OK;
    DrvCamClkSetParent_t *pstCfg = pCfg;
    // ToDo: to control switch clk source,it need to reduce/add ref count.
    CAMCLKDBG("[%s @ %d] Id :%d Parent Freq: %lu Parent Id :%d\n", __FUNCTION__, __LINE__, pstCfg->u32Id,
              pstCfg->u32ParentFreq, pstCfg->u32ParentId);
    if (gCamClkSrcNode[pstCfg->u32Id].u8ClkType == HAL_CAMCLK_TYPE_COMPOSITE)
    {
        if (_DrvCamClkImplCheckParent(pstCfg))
        {
            return CAMCLK_RET_FAIL;
        }
        CAM_CLK_OSLOCK();
        CAM_CLK_LOCK_SEM();
        _DrvCamClkImplResetRefCnt(pstCfg);
        _DrvCamClkImplSetParent(pstCfg);
        CAM_CLK_UNLOCK_SEM();
        CAM_CLK_OSUNLOCK();
    }
    else
    {
        enRet = CAMCLK_RET_FAIL;
    }
    return enRet;
}
CAMCLK_RET_e DrvCamClkImplGetOnOff(void *pCfg)
{
    CAMCLK_RET_e         enRet  = CAMCLK_RET_OK;
    DrvCamClkGetOnOff_t *pstCfg = pCfg;
    CAM_CLK_INVALIDATE((void *)gCamClkTopCurrent);
    if (gCamClkTopCurrent[pstCfg->u32Id].u8RefCnt)
    {
        *pstCfg->pbEn = 1;
    }
    CAMCLKDBG("[%s @ %d] Id :%d En:%hhd\n", __FUNCTION__, __LINE__, pstCfg->u32Id, *pstCfg->pbEn);
    return enRet;
}
CAMCLK_RET_e DrvCamClkImplSetOnOff(void *pCfg)
{
    CAMCLK_RET_e            enRet  = CAMCLK_RET_OK;
    DrvCamClkSetOnOff_t *   pstCfg = pCfg;
    DrvCamClkGetAllParent_t stCfg;
    u32                     u32idx;
    u32                     u32AddRefCnt;

    CAMCLKDBG("[%s @ %d] Id :%d\n", __FUNCTION__, __LINE__, pstCfg->u32Id, pstCfg->bEn);
    if (pstCfg->u32Id < HAL_CAMCLK_SRC_Id_MAX)
    {
        stCfg.u32Id = pstCfg->u32Id;
        enRet       = _DrvCamClkImplGetAllParentList((void *)&stCfg);
        if (enRet)
        {
            goto ReturnBack;
        }
        // u32AddRefCnt = _DrvCamClkImplCheckRefCnt(pstCfg->u32Id,pstCfg->bEn);
        u32AddRefCnt = pstCfg->bEn;
        CAM_CLK_OSLOCK();
        CAM_CLK_LOCK_SEM();
        for (u32idx = 0; u32idx < stCfg.u32ParentCount; u32idx++)
        {
            _DrvCamClkImplSetRefCnt(stCfg.u32ParentId[u32idx], u32AddRefCnt);
            _DrvCamClkImplExecGated(stCfg.u32ParentId[u32idx], pstCfg->bEn);
        }
        CAM_CLK_UNLOCK_SEM();
        CAM_CLK_OSUNLOCK();
    }
ReturnBack:
    return enRet;
}
CAMCLK_RET_e DrvCamClkImplSetForceOnOff(void *pCfg)
{
    CAMCLK_RET_e            enRet  = CAMCLK_RET_OK;
    DrvCamClkSetOnOff_t *   pstCfg = pCfg;
    DrvCamClkGetAllParent_t stCfg;
    u32                     u32idx;

    CAMCLKDBG("[%s @ %d] Id :%d\n", __FUNCTION__, __LINE__, pstCfg->u32Id, pstCfg->bEn);
    if (pstCfg->u32Id < HAL_CAMCLK_SRC_Id_MAX)
    {
        stCfg.u32Id = pstCfg->u32Id;
        enRet       = _DrvCamClkImplGetAllParentList((void *)&stCfg);
        if (enRet)
        {
            goto ReturnBack;
        }
        for (u32idx = 0; u32idx < stCfg.u32ParentCount; u32idx++)
        {
            _DrvCamClkImplSetGated(stCfg.u32ParentId[u32idx], pstCfg->bEn);
        }
    }
ReturnBack:
    return enRet;
}
CAMCLK_RET_e DrvCamClkImplRestoreClk(void)
{
    HalCamClkSrcId_e    u32Id;
    u8                  u8En;
    DrvCamClkSetOnOff_t stCfg;
    CAM_CLK_OSLOCK();
    CAM_CLK_LOCK_SEM();
    CAM_CLK_INVALIDATE((void *)gCamClkTopCurrent);
    for (u32Id = 0; u32Id < HAL_CAMCLK_SRC_Id_MAX; u32Id++)
    {
        if (gCamClkSrcNode[u32Id].u8ClkType == HAL_CAMCLK_TYPE_COMPOSITE
            || gCamClkSrcNode[u32Id].u8ClkType == HAL_CAMCLK_TYPE_COMPLEX)
        {
            DrvCamClkImplGetHwOnOff(u32Id, &u8En);
            if (gCamClkTopCurrent[u32Id].u8RefCnt != 0 && u8En == 0)
            {
                stCfg.bEn   = 1;
                stCfg.u32Id = u32Id;
                DrvCamClkImplSetForceOnOff(&stCfg);
            }
        }
    }
    CAM_CLK_UNLOCK_SEM();
    CAM_CLK_OSUNLOCK();
    return 0;
}
int DrvCamClkImplDisableUnuseClk(void)
{
#if 0
    HalCamClkSrcId_e u32Id;
    u8 u8En;
    CAM_CLK_INVALIDATE((void*)gCamClkTopCurrent);
    for(u32Id=0;u32Id<HAL_CAMCLK_SRC_Id_MAX;u32Id++)
    {
        if(gCamClkSrcNode[u32Id].u8ClkType==HAL_CAMCLK_TYPE_COMPOSITE)
        {
            if(gCamClkTopCurrent[u32Id].u8RefCnt == 0)
            {
                DrvCamClkImplGetHwOnOff(u32Id,&u8En);
                if(u8En)
                {
                    stCfg.bEn = 0;
                    stCfg.u32Id = u32Id;
                    DrvCamClkImplSetOnOff(&stCfg);
                }
            }
        }
    }
#endif
    CAMCLKDBG("[%s @ %d]\n", __FUNCTION__, __LINE__);
    return 0;
}
CAMCLK_RET_e DrvCamClkImplInit(void)
{
    CAMCLK_RET_e enRet = CAMCLK_RET_OK;
    CAMCLKDBG("[%s @ %d]\n", __FUNCTION__, __LINE__);
    _DrvCamClkImplSwInit();
    _DrvCamClkImplHwInit();
    return enRet;
}
CAMCLK_RET_e DrvCamClkImplDeinit(void)
{
    CAMCLK_RET_e enRet = CAMCLK_RET_OK;
    CAMCLKDBG("[%s @ %d]\n", __FUNCTION__, __LINE__);
    _DrvCamClkImplHwDeinit();
    _DrvCamClkImplSwDeinit();
    return enRet;
}
#undef HAL_CAMCLK_C
