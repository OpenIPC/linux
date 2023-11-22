/*
 * hal_camclk_complex.c- Sigmastar
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

#define HAL_CAMCLKTBL_LPLL_C
#include "camclk_dbg.h"
#include "hal_camclk_if_st.h"
#include "hal_camclk_lpll_tbl.h"
#include "hal_camclk_util.h"

HalCamClkLpllTbl_t gCamClkLPLLSettingTBL[E_HAL_CAMCLK_SUPPORTED_LPLL_MAX][HAL_CAMCLK_LPLL_REG_NUM] = {
    {
        // E_HAL_CAMCLK_SUPPORTED_LPLL_HS_CH_100TO187D5MHZ    NO.0
        // Address,Value
        {0x103380, 0x0201},
        {0x103382, 0x0420},
        {0x103384, 0x0041},
        {0x103386, 0x0000},
        {0x103394, 0x0001},
        {0x103396, 0x0000},
    },

    {
        // E_HAL_CAMCLK_SUPPORTED_LPLL_HS_CH_50TO100MHZ    NO.1
        // Address,Value
        {0x103380, 0x0201},
        {0x103382, 0x0420},
        {0x103384, 0x0042},
        {0x103386, 0x0001},
        {0x103394, 0x0001},
        {0x103396, 0x0000},
    },

    {
        // E_HAL_CAMCLK_SUPPORTED_LPLL_HS_CH_25TO50MHZ    NO.2
        // Address,Value
        {0x103380, 0x0201},
        {0x103382, 0x0420},
        {0x103384, 0x0043},
        {0x103386, 0x0002},
        {0x103394, 0x0001},
        {0x103396, 0x0000},
    },

    {
        // E_HAL_CAMCLK_SUPPORTED_LPLL_HS_CH_12D5TO25MHZ    NO.3
        // Address,Value
        {0x103380, 0x0201},
        {0x103382, 0x0420},
        {0x103384, 0x0083},
        {0x103386, 0x0003},
        {0x103394, 0x0001},
        {0x103396, 0x0000},
    },
};
#define LPLL_REG_GET(address) (0x1F000000 + ((address) << 1))
u16 u16LoopGain[E_HAL_CAMCLK_SUPPORTED_LPLL_MAX] = {
    16, // E_HAL_CAMCLK_SUPPORTED_LPLL_HS_CH_100TO187D5MHZ    NO.0
    8,  // E_HAL_CAMCLK_SUPPORTED_LPLL_HS_CH_50TO100MHZ    NO.1
    4,  // E_HAL_CAMCLK_SUPPORTED_LPLL_HS_CH_25TO50MHZ    NO.2
    2,  // E_HAL_CAMCLK_SUPPORTED_LPLL_HS_CH_12D5TO25MHZ    NO.3
};
u32  u16gLpllDiv = 8;
u8   bLook       = 0;
void HalCamClkGetLpllIdxFromHw(u16 *pu16Idx)
{
    u16 u16Val;
    u8  idx;
    u16Val = R2BYTE(LPLL_REG_GET(gCamClkLPLLSettingTBL[0][2].address));
    for (idx = 0; idx < E_HAL_CAMCLK_SUPPORTED_LPLL_MAX; idx++)
    {
        if (u16Val == gCamClkLPLLSettingTBL[idx][2].value)
        {
            *pu16Idx = idx;
            break;
        }
    }
}
u8 HalCamClkGetLpllIdx(u32 u32Dclk, u16 *pu16Idx, u16 *pu16Div)
{
    u8 bRet = 1;

    *pu16Div = 8;
    if (IS_DATA_LANE_LESS_9M(u32Dclk) || IS_DATA_LANE_BPS_9M_TO_9_5M(u32Dclk))
    {
        *pu16Idx = E_HAL_CAMCLK_SUPPORTED_LPLL_MAX;
        bRet     = 0;
    }
    else if (IS_DATA_LANE_BPS_12_5M_TO_25M(u32Dclk))
    {
        *pu16Idx = E_HAL_CAMCLK_SUPPORTED_LPLL_HS_CH_12D5TO25MHZ;
    }
    else if (IS_DATA_LANE_BPS_25M_TO_50M(u32Dclk))
    {
        *pu16Idx = E_HAL_CAMCLK_SUPPORTED_LPLL_HS_CH_25TO50MHZ;
    }
    else if (IS_DATA_LANE_BPS_50M_TO_100M(u32Dclk))
    {
        *pu16Idx = E_HAL_CAMCLK_SUPPORTED_LPLL_HS_CH_50TO100MHZ;
    }
    else if (IS_DATA_LANE_BPS_100M_TO_200M(u32Dclk))
    {
        *pu16Idx = E_HAL_CAMCLK_SUPPORTED_LPLL_HS_CH_12D5TO25MHZ;
        *pu16Div = 1;
    }
    else if (IS_DATA_LANE_BPS_200M_TO_400M(u32Dclk))
    {
        *pu16Idx = E_HAL_CAMCLK_SUPPORTED_LPLL_HS_CH_25TO50MHZ;
        *pu16Div = 1;
    }
    else if (IS_DATA_LANE_BPS_400M_TO_800M(u32Dclk))
    {
        *pu16Idx = E_HAL_CAMCLK_SUPPORTED_LPLL_HS_CH_50TO100MHZ;
        *pu16Div = 1;
    }
    else if (IS_DATA_LANE_BPS_800M_TO_15000M(u32Dclk))
    {
        *pu16Idx = E_HAL_CAMCLK_SUPPORTED_LPLL_HS_CH_100TO187D5MHZ;
        *pu16Div = 1;
    }
    else
    {
        *pu16Idx = E_HAL_CAMCLK_SUPPORTED_LPLL_MAX;
        bRet     = 0;
    }

    return bRet;
    ;
}

u16 HalCamClkGetLpllGain(u16 u16Idx)
{
    u16 *pu16Tbl = NULL;

    pu16Tbl = u16LoopGain;

    return (u16Idx < E_HAL_CAMCLK_SUPPORTED_LPLL_MAX) ? pu16Tbl[u16Idx] : 1;
}

void HalCamClkDumpLpllSetting(u16 u16Idx)
{
    u16 u16RegIdx;

    if (u16Idx < E_HAL_CAMCLK_SUPPORTED_LPLL_MAX)
    {
        for (u16RegIdx = 0; u16RegIdx < HAL_CAMCLK_LPLL_REG_NUM; u16RegIdx++)
        {
            if (gCamClkLPLLSettingTBL[u16Idx][u16RegIdx].address == 0xFFFFFFF)
            {
                // DrvSclOsDelayTask(LPLLSettingTBL[u16Idx][u16RegIdx].value);
                continue;
            }

            W2BYTEMSK(LPLL_REG_GET(gCamClkLPLLSettingTBL[u16Idx][u16RegIdx].address),
                      gCamClkLPLLSettingTBL[u16Idx][u16RegIdx].value, 0xFFF); // @suppress("Symbol is not resolved")
        }
    }
}

void HalCamClkSetLpllSet(u32 u32LpllSet)
{
    u16 u16LpllSet_Lo, u16LpllSet_Hi;

    u16LpllSet_Lo = (u16)(u32LpllSet & 0x0000FFFF);
    u16LpllSet_Hi = (u16)((u32LpllSet & 0x00FF0000) >> 16);
    W2BYTE(LPLL_REG_GET(REG_LPLL_48_L), u16LpllSet_Lo); // @suppress("Symbol is not resolved")
    W2BYTE(LPLL_REG_GET(REG_LPLL_49_L), u16LpllSet_Hi); // @suppress("Symbol is not resolved")
}
void HalCamClkGetLpllSet(u32 *u32LpllSet)
{
    u16 u16LpllSet_Lo, u16LpllSet_Hi;

    u16LpllSet_Lo = R2BYTE(LPLL_REG_GET(REG_LPLL_48_L));
    u16LpllSet_Hi = R2BYTE(LPLL_REG_GET(REG_LPLL_49_L));
    *u32LpllSet   = (u16LpllSet_Lo | ((u32)u16LpllSet_Hi << 16));
}
CAMCLK_RET_e HalCamClkSetLpllRate(void *pCfg)
{
    u16                        u16LpllIdx;
    u16                        u16Div;
    u16                        u16LoopGain;
    u32                        u32LplLSet, u32Divisor;
    u64                        u64Dividen;
    HalCamClkSetComplexRate_t *pLpllCfg = pCfg;
    CAMCLK_RET_e               Ret      = CAMCLK_RET_OK;
    if (bLook)
    {
        Ret = CAMCLK_RET_FAIL;
        CAMCLKERR("%s %d, LPLL Locked:%ld\n", __FUNCTION__, __LINE__, pLpllCfg->u32Rate);
    }
    else
    {
        if (HalCamClkGetLpllIdx(pLpllCfg->u32Rate, &u16LpllIdx, &u16Div))
        {
            if (u16LpllIdx != E_HAL_CAMCLK_SUPPORTED_LPLL_MAX)
            {
                u16LoopGain = HalCamClkGetLpllGain(u16LpllIdx);
                u64Dividen  = (pLpllCfg->u32ParentRate);
                u64Dividen  = (u64)(u64Dividen * (u32)524288 * (u32)u16LoopGain);
                u64Dividen  = CamOsMathDivU64(u64Dividen, u16Div, NULL);
                u32Divisor  = pLpllCfg->u32Rate;
                u32LplLSet  = (u32)CamOsMathDivU64(u64Dividen, u32Divisor, NULL);

                CAMCLKDBG(
                    "[CAMCLK]%s %d:: Idx:%d, LoopGain:%d, LoopDiv:%d, dclk=%ld, Divden:0x%llx, Divisor:0x%lx, "
                    "LpllSe:0x%lx\n",
                    __FUNCTION__, __LINE__, u16LpllIdx, u16LoopGain, u16Div, pLpllCfg->u32Rate, u64Dividen, u32Divisor,
                    u32LplLSet);

                HalCamClkDumpLpllSetting(u16LpllIdx);
                HalCamClkSetLpllSet(u32LplLSet);
                u16gLpllDiv = u16Div;
            }
            else
            {
                Ret = CAMCLK_RET_RATEERR;
                CAMCLKERR("%s %d, DCLK Out of Range:%ld\n", __FUNCTION__, __LINE__, pLpllCfg->u32Rate);
            }
        }
        else
        {
            Ret = CAMCLK_RET_RATEERR;
            CAMCLKERR("%s %d, DCLK Out of Range:%ld\n", __FUNCTION__, __LINE__, pLpllCfg->u32Rate);
        }
    }
    return Ret;
}
CAMCLK_RET_e HalCamClkGetLpllRate(void *pCfg)
{
    HalCamClkGetComplexRate_t *pLpllCfg = pCfg;
    CAMCLK_RET_e               Ret      = CAMCLK_RET_OK;
    u16                        u16idx;
    u16                        u16LoopGain;
    u32                        u32LplLSet, u32Divisor;
    u64                        u64Dividen;

    HalCamClkGetLpllIdxFromHw(&u16idx);
    u16LoopGain = HalCamClkGetLpllGain(u16idx);
    HalCamClkGetLpllSet(&u32LplLSet);
    u64Dividen        = pLpllCfg->u32ParentRate;
    u64Dividen        = (u64)((u64Dividen) * (u32)524288 * (u32)u16LoopGain);
    u64Dividen        = CamOsMathDivU64(u64Dividen, u16gLpllDiv, NULL);
    u32Divisor        = u32LplLSet;
    pLpllCfg->u32Rate = (u32)CamOsMathDivU64(u64Dividen, u32Divisor, NULL);

    return Ret;
}
CAMCLK_RET_e HalCamClkGetLpllRoundRate(void *pCfg)
{
    HalCamClkGetComplexRoundRate_t *pLpllCfg = pCfg;
    u16                             u16Temp;
    CAMCLK_RET_e                    Ret = CAMCLK_RET_OK;

    if (HalCamClkGetLpllIdx(pLpllCfg->u32Rate, &u16Temp, &u16Temp))
    {
        pLpllCfg->u32RoundRate = pLpllCfg->u32Rate;
    }
    else
    {
        Ret = CAMCLK_RET_RATEERR;
        if (pLpllCfg->u32Rate < DATA_LANE_12_5MHZ)
        {
            pLpllCfg->u32RoundRate = DATA_LANE_12_5MHZ;
        }
        else
        {
            pLpllCfg->u32RoundRate = DATA_LANE_1500MHZ;
        }
    }

    return Ret;
}
CAMCLK_RET_e HalCamClkGetLpllOnOff(void *pCfg)
{
    HalCamClkComplexOnOff_t *pLpllCfg = pCfg;
    CAMCLK_RET_e             Ret      = CAMCLK_RET_OK;

    pLpllCfg->bEn = (R2BYTE(LPLL_REG_GET(gCamClkLPLLSettingTBL[0][0].address)) & camclk_BIT13) >> 13;
    return Ret;
}
CAMCLK_RET_e HalCamClkSetLpllOnOff(void *pCfg)
{
    HalCamClkComplexOnOff_t *pLpllCfg = pCfg;
    CAMCLK_RET_e             Ret      = CAMCLK_RET_OK;
    W2BYTEMSK(LPLL_REG_GET(gCamClkLPLLSettingTBL[0][0].address), pLpllCfg->bEn ? camclk_BIT13 : 0, camclk_BIT13);
    bLook = pLpllCfg->bEn;
    return Ret;
}
HalCamClkAdjOps_t gLpllOps   = {0,
                              0,
                              HalCamClkSetLpllRate,
                              HalCamClkGetLpllRate,
                              HalCamClkGetLpllRoundRate,
                              HalCamClkSetLpllOnOff,
                              HalCamClkGetLpllOnOff};
HalCamClkAdjOps_t gCpupllOps = {0, 0, 0, 0, 0, 0, 0};
HalCamClkAdjOps_t gVenOps    = {0, 0, 0, 0, 0, 0, 0};
#undef HAL_CAMCLKTBL_LPLL_C
