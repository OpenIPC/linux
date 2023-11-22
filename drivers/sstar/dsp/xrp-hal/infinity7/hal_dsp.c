/*
 * hal_dsp.c- Sigmastar
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
#include "ms_platform.h"
#include "ms_types.h"
#include "registers.h"
#include "cam_os_wrapper.h"
#include "cam_sysfs.h"
#include "hal_dsp.h"
#include "reg.h"

#define CORE_VALID_CHECK_RETURN(eCoreID)                                                  \
    {                                                                                     \
        if (eCoreID < HAL_DSP_CORE0 || eCoreID > HAL_DSP_CORE3)                           \
        {                                                                                 \
            CamOsPrintf("[DSP:%d] %s %d unkown core\n", eCoreID, __FUNCTION__, __LINE__); \
            return HAL_DSP_FAIL;                                                          \
        }                                                                                 \
    }
#define CODE_STUB_CORE_SETTINNG_BIT_IN_ONE_REG(CORE, BOOL_BIT, BANK)                                           \
    {                                                                                                          \
        __u16 u16Value = 0;                                                                                    \
        __u16 u16Mask  = 0;                                                                                    \
        CORE_VALID_CHECK_RETURN(CORE);                                                                         \
                                                                                                               \
        switch (CORE)                                                                                          \
        {                                                                                                      \
            case HAL_DSP_CORE0:                                                                                \
                u16Value = BIT(0);                                                                             \
                u16Mask  = DSP_GP_CORE_##BANK##_CORE0_MASK;                                                    \
                break;                                                                                         \
            case HAL_DSP_CORE1:                                                                                \
                u16Value = BIT(1);                                                                             \
                u16Mask  = DSP_GP_CORE_##BANK##_CORE1_MASK;                                                    \
                break;                                                                                         \
            case HAL_DSP_CORE2:                                                                                \
                u16Value = BIT(2);                                                                             \
                u16Mask  = DSP_GP_CORE_##BANK##_CORE2_MASK;                                                    \
                break;                                                                                         \
            case HAL_DSP_CORE3:                                                                                \
                u16Value = BIT(3);                                                                             \
                u16Mask  = DSP_GP_CORE_##BANK##_CORE3_MASK;                                                    \
                break;                                                                                         \
            default:                                                                                           \
            {                                                                                                  \
                CamOsPrintf("[DSP:%d] %s %d unknown dsp core\n", CORE, __FUNCTION__, __LINE__);                \
                return HAL_DSP_FAIL;                                                                           \
            }                                                                                                  \
        }                                                                                                      \
        if (BOOL_BIT)                                                                                          \
            OUTREGMSK16(_halDspGetGPRegPA(DSP_GP_CORE_##BANK), u16Value << DSP_GP_CORE_##BANK##_LSB, u16Mask); \
        else                                                                                                   \
            CLRREG16(_halDspGetGPRegPA(DSP_GP_CORE_##BANK), u16Value << DSP_GP_CORE_##BANK##_LSB);             \
                                                                                                               \
        return HAL_DSP_OK;                                                                                     \
    }

static inline __u64 _halDspGetCoreRegPA(HalDspCoreID_e eCoreID, __u32 u32RegID)
{
    CORE_VALID_CHECK_RETURN(eCoreID);
    switch (eCoreID)
    {
        case HAL_DSP_CORE0:
            return BASE_REG_DSP0_PA + u32RegID;
        case HAL_DSP_CORE1:
            return BASE_REG_DSP1_PA + u32RegID;
        case HAL_DSP_CORE2:
            return BASE_REG_DSP2_PA + u32RegID;
        case HAL_DSP_CORE3:
            return BASE_REG_DSP3_PA + u32RegID;
        default:
            CamOsPrintf("[DSP:%d] %s %d unknown core\n", eCoreID, __FUNCTION__, __LINE__);
            return BASE_REG_DSP0_PA + u32RegID;
    }
    return 0;
}
static inline __u64 _halDspGetXIURegPA(HalDspCoreID_e eCoreID, __u32 u32ApbLow)
{
    CORE_VALID_CHECK_RETURN(eCoreID);
    switch (eCoreID)
    {
        case HAL_DSP_CORE0:
            return BASE_REG_X32_VQ7_GP0 + u32ApbLow;
        case HAL_DSP_CORE1:
            return BASE_REG_X32_VQ7_GP1 + u32ApbLow;
        case HAL_DSP_CORE2:
            return BASE_REG_X32_VQ7_GP2 + u32ApbLow;
        case HAL_DSP_CORE3:
            return BASE_REG_X32_VQ7_GP3 + u32ApbLow;
        default:
            CamOsPrintf("[DSP:%d] %s %d unknown core\n", eCoreID, __FUNCTION__, __LINE__);
            return BASE_REG_X32_VQ7_GP0 + u32ApbLow;
    }
    return 0;
}
static inline __u64 _halDspGetGPRegPA(__u32 u32RegID)
{
    return BASE_REG_DSPG_PA + u32RegID;
}

typedef struct halDspContext_s
{
    bool bNodieInited;
    bool bGpInited;
} halDspContext_t;
static halDspContext_t halDspCtx = {0};
HalDspRet_e            HalDspInit(HalDspCoreID_e eCoreID)
{
    bool value = -1;
    //__u32 v = 0;
    CORE_VALID_CHECK_RETURN(eCoreID);
    if (!halDspCtx.bNodieInited)
    {
        // //nodie
        HalDspNoDieReset(HAL_DSP_NODIE_RESET_ALL);
        CamOsUsSleep(1);
        HalDspNoDieReset(HAL_DSP_RESET_DONE);
        HalDspNoDieIsoEn(false);
        halDspCtx.bNodieInited = true;
    }
    if (!halDspCtx.bGpInited)
    {
        // gp
        HalDspGPReset(HAL_DSP_GP_RESET_ALL);
        CamOsUsSleep(1);
        HalDspGPReset(HAL_DSP_RESET_DONE);
        halDspCtx.bGpInited = true;
    }
    // CamOsMsSleep(10);
    HalDspSetClkGating(eCoreID, true);
    HalDspSetClkInvert(eCoreID, false);

    HalDspSetTOPClk(false, false, true);
    HalDspSetDFS(eCoreID, false, 31);
    HalDspSetJTAGSelection(eCoreID);
    HalDspSetAllSramForceON(true);
    HalDspSetCorePower(eCoreID, false);
    HalDspSetCoreIso(eCoreID, false);
    HalDspGetCorePower(eCoreID, &value);
    return HAL_DSP_OK;
}
HalDspRet_e HalDspReset(HalDspCoreID_e eCoreID, HalDspResetType_e eRstType)
{
    CORE_VALID_CHECK_RETURN(eCoreID);
    switch (eRstType)
    {
        case HAL_DSP_CORE_RESET_ALL:
            OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_RESET), DSP_CORE_RESET_ALL, DSP_CORE_RESET_MASK);
            break;
        case HAL_DSP_CORE_RESET_CORE:
            OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_RESET), DSP_CORE_RESET_CORE, DSP_CORE_RESET_MASK);
            break;
        case HAL_DSP_CORE_RESET_BRIDGE:
            OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_RESET), DSP_CORE_RESET_BRIDGE, DSP_CORE_RESET_MASK);
            break;
        case HAL_DSP_CORE_RESET_DEBUG:
            OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_RESET), DSP_CORE_RESET_DRESET, DSP_CORE_RESET_MASK);
            break;
        case HAL_DSP_RESET_DONE:
            OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_RESET), 0, DSP_CORE_RESET_MASK);
            break;
        default:
            CamOsPrintf("[DSP:%d] %s %d unknown reset type\n", eCoreID, __FUNCTION__, __LINE__);
            break;
    }
    return HAL_DSP_OK;
}
HalDspRet_e HalDspRunStallReset(HalDspCoreID_e eCoreID, bool bEnable)
{
    CORE_VALID_CHECK_RETURN(eCoreID);
    if (bEnable)
        OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_RUNSTALL_RESET), DSP_CORE_RUNSTALL_RESET_EN,
                    DSP_CORE_RUNSTALL_RESET_MASK);
    else
        CLRREG16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_RUNSTALL_RESET), DSP_CORE_RUNSTALL_RESET_EN);

    return HAL_DSP_OK;
}
HalDspRet_e HalDspSetResetVector(HalDspCoreID_e eCoreID, bool bEnable, __u64 u64Phy)
{
    CORE_VALID_CHECK_RETURN(eCoreID);
    if (bEnable)
    {
        OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_RESET_VECTOR_SEL), DSP_CORE_RESET_VECTOR_SEL_EN,
                    DSP_CORE_RESET_VECTOR_SEL_MASK);
        OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_ALT_RESETVEC_W_LOW), u64Phy, DSP_CORE_ALT_RESETVEC_W_MASK);
        OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_ALT_RESETVEC_W_HIGH), u64Phy >> 16,
                    DSP_CORE_ALT_RESETVEC_W_MASK);
    }
    else
    {
        CLRREG16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_RESET_VECTOR_SEL), DSP_CORE_RESET_VECTOR_SEL_EN);
    }
    return HAL_DSP_OK;
}
HalDspRet_e HalDspGPSetClkFreq(__u16 u16FreqM)
{
    __u32 f1[] = {10, 20, 25, 50};
    __u32 f2[] = {10, 20, 30, 40};
    __u8  scalar1, scalar2, scalar3;
    __u32 synthClk = 0;

    if (u16FreqM < 400 || u16FreqM > 1500)
    {
        CamOsPrintf("invalid clk setting %d,should be in [400,1500] %s %d\n", u16FreqM, __FUNCTION__, __LINE__);
        return HAL_DSP_FAIL;
    }
    for (scalar1 = 0; scalar1 < DSP_PLL_SCALAR1_DIV_THRES; scalar1++)
    {
        for (scalar3 = 0; scalar3 < DSP_PLL_SCALAR3_DIV_THRES; scalar3++)
        {
            for (scalar2 = 1; scalar2 <= DSP_PLL_SCALAR2_DIV_THRES; scalar2++)
            {
                __u32 f3 = scalar2 > 15 ? 35 : scalar2 * 10;
                __u32 f  = f1[scalar1] * f2[scalar3] * f3;
                __u32 synthClkM;

                // To do float computation with integer, we have multiply f1, f2, f3 by 10,
                // so value of f have been scaled by 1000
                synthClkM = u16FreqM * f / DSP_PLL_LOOP_DIV_THRES;
                if ((synthClkM > (DSP_PLL_SYNTH_CLK_THRES_LOW * 1000))
                    && (synthClkM < (DSP_PLL_SYNTH_CLK_THRES_HIGH * 1000)))
                {
                    // CamOsPrintf("%d %d %d %lx\n", scalar1, scalar2, scalar3,
                    //             (__u32)((double)432 * 24 * (1 << 19) / (u16FreqM * f)));
                    synthClk = (__u32)((__u64)MPLL_SOURCE_CLK_FREQ * DSP_PLL_LOOP_DIV_THRES * (1 << 19) * 1000
                                       / (u16FreqM * f));
                    goto found;
                }
            }
        }
    }
    return HAL_DSP_FAIL;
found:
    OUTREGMSK16(BASE_REG_XTAL_PA + DSP_CLK_XTAL_HV_POWER, DSP_CLK_XTAL_HV_POWER_EN, DSP_CLK_XTAL_HV_POWER_MSK);
    OUTREGMSK16(BASE_REG_MPLL_PA + DSP_CLK_MPLL_POWER, DSP_CLK_MPLL_POWER_EN, DSP_CLK_MPLL_POWER_MSK);
    CamOsUsSleep(1);
    OUTREGMSK16(BASE_REG_MPLL_PA + DSP_CLK_MPLL_U02_ECO, DSP_CLK_MPLL_U02_ECO_VALUE, DSP_CLK_MPLL_U02_ECO_MSK);

    OUTREGMSK16(BASE_REG_SYTHESIZER_PA + DSP_CLK_DSPPLL_SCALAR_DIV_1, scalar1 << DSP_CLK_DSPPLL_SCALAR_DIV_1_LSB,
                DSP_CLK_DSPPLL_SCALAR_DIV_1_MSK);
    OUTREGMSK16(BASE_REG_SYTHESIZER_PA + DSP_CLK_DSPPLL_SCALAR_DIV_2, scalar2 << DSP_CLK_DSPPLL_SCALAR_DIV_2_LSB,
                DSP_CLK_DSPPLL_SCALAR_DIV_2_MSK);
    OUTREGMSK16(BASE_REG_SYTHESIZER_PA + DSP_CLK_DSPPLL_SCALAR_DIV_3, scalar3 << DSP_CLK_DSPPLL_SCALAR_DIV_3_LSB,
                DSP_CLK_DSPPLL_SCALAR_DIV_3_MSK);
    OUTREGMSK16(BASE_REG_SYTHESIZER_PA + DSP_CLK_DSPPLL_SYNTHESIZER_CLK_1, synthClk,
                DSP_CLK_DSPPLL_SYNTHESIZER_CLK_1_MSK);
    OUTREGMSK16(BASE_REG_SYTHESIZER_PA + DSP_CLK_DSPPLL_SYNTHESIZER_CLK_2, synthClk,
                DSP_CLK_DSPPLL_SYNTHESIZER_CLK_2_MSK);
    OUTREGMSK16(BASE_REG_SYTHESIZER_PA + DSP_CLK_DSPPLL_SYNTHESIZER_CLK_3, synthClk >> 16,
                DSP_CLK_DSPPLL_SYNTHESIZER_CLK_3_MSK);
    OUTREGMSK16(BASE_REG_SYTHESIZER_PA + DSP_CLK_DSPPLL_SYNTHESIZER_NF, DSP_CLK_DSPPLL_SYNTHESIZER_NF_EN,
                DSP_CLK_DSPPLL_SYNTHESIZER_NF_MSK);
    OUTREGMSK16(BASE_REG_SYTHESIZER_PA + DSP_CLK_PLL_POWER, DSP_CLK_PLL_POWER_EN, DSP_CLK_PLL_POWER_MSK);
    CamOsUsSleep(1);
    OUTREGMSK16(BASE_REG_SYTHESIZER_PA + DSP_CLK_DSPPLL_U02_ECO, DSP_CLK_DSPPLL_U02_ECO_VALUE,
                DSP_CLK_DSPPLL_U02_ECO_MSK);
    return HAL_DSP_OK;
}

HalDspRet_e HalDspSetXIUClkEn(HalDspCoreID_e eCoreID, bool bEnable)
{
    CORE_VALID_CHECK_RETURN(eCoreID);
    if (bEnable)
        CLRREG16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_XIU_CLK_GATING), DSP_CORE_XIU_CLK_GATING_EN);
    else
        OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_XIU_CLK_GATING), DSP_CORE_XIU_CLK_GATING_EN,
                    DSP_CORE_XIU_CLK_GATING_MASK);
    return HAL_DSP_OK;
}
HalDspRet_e HalDspSetAPBSlvClkEn(HalDspCoreID_e eCoreID, bool bEnable)
{
    CORE_VALID_CHECK_RETURN(eCoreID);
    if (bEnable)
        OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_APB_SLAVE_CLK), DSP_CORE_APB_SLAVE_CLK_EN,
                    DSP_CORE_APB_SLAVE_CLK_MASK);
    else
        CLRREG16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_APB_SLAVE_CLK), DSP_CORE_APB_SLAVE_CLK_EN);
    return HAL_DSP_OK;
}
HalDspRet_e HalDspSetCoreSightDbgMode(HalDspCoreID_e eCoreID, HalDspCoreSightDbgMode_e eDbgMode)
{
    __u16 u16Value = 0;
    CORE_VALID_CHECK_RETURN(eCoreID);
    u16Value |= eDbgMode & HAL_DSP_DBG_PDBGEN ? DSP_CORE_DEBUG_MODE_PDBGEN : 0;
    u16Value |= eDbgMode & HAL_DSP_DBG_DBGEN ? DSP_CORE_DEBUG_MODE_DBGEN : 0;
    u16Value |= eDbgMode & HAL_DSP_DBG_NIDEN ? DSP_CORE_DEBUG_MODE_NIDEN : 0;
    u16Value |= eDbgMode & HAL_DSP_DBG_SPIDEN ? DSP_CORE_DEBUG_MODE_SPIDEN : 0;
    u16Value |= eDbgMode & HAL_DSP_DBG_SPNIDEN ? DSP_CORE_DEBUG_MODE_SPNIDEN : 0;
    OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_DEBUG_MODE), u16Value, DSP_CORE_DEBUG_MODE_MASK);
    return HAL_DSP_OK;
}

HalDspRet_e HalDspSetPRID(HalDspCoreID_e eCoreID, __u16 u16PRID)
{
    CORE_VALID_CHECK_RETURN(eCoreID);
    OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_PRID), u16PRID, DSP_CORE_PRID_MASK);
    return HAL_DSP_OK;
}
HalDspRet_e HalDspSetAddressMap(HalDspCoreID_e eCoreID, HalDspAddressMap_t *pstDspAddressMap)
{
    CORE_VALID_CHECK_RETURN(eCoreID);
    if (pstDspAddressMap == NULL)
        return HAL_DSP_FAIL;

    switch (pstDspAddressMap->eSet)
    {
        case HAL_DSP_ADDRESS_SET0:
            OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_REMAP_BASE0_W_LOW),
                        pstDspAddressMap->u32BaseAddress >> DSP_CORE_REMAP_BASE_SHIFT, DSP_CORE_REMAP_BASE0_W_MASK);
            OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_REMAP_BASE0_W_HIGH),
                        pstDspAddressMap->u32BaseAddress >> (16 + DSP_CORE_REMAP_BASE_SHIFT),
                        DSP_CORE_REMAP_BASE0_W_MASK);
            OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_REMAP_LEN0),
                        pstDspAddressMap->u32Len >> DSP_CORE_REMAP_LEN_SHIFT, DSP_CORE_REMAP_LEN0_MASK);
            OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_REMAP_OFFSET0),
                        pstDspAddressMap->offset >> DSP_CORE_REMAP_OFFSET_SHIFT, DSP_CORE_REMAP_OFFSET0_MASK);
            break;
        case HAL_DSP_ADDRESS_SET1:
            OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_REMAP_BASE1_W_LOW),
                        pstDspAddressMap->u32BaseAddress >> DSP_CORE_REMAP_BASE_SHIFT, DSP_CORE_REMAP_BASE1_W_MASK);
            OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_REMAP_BASE1_W_HIGH),
                        pstDspAddressMap->u32BaseAddress >> (16 + DSP_CORE_REMAP_BASE_SHIFT),
                        DSP_CORE_REMAP_BASE1_W_MASK);
            OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_REMAP_LEN1),
                        pstDspAddressMap->u32Len >> DSP_CORE_REMAP_LEN_SHIFT, DSP_CORE_REMAP_LEN1_MASK);
            OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_REMAP_OFFSET1),
                        pstDspAddressMap->offset >> DSP_CORE_REMAP_OFFSET_SHIFT, DSP_CORE_REMAP_OFFSET1_MASK);
            break;
        case HAL_DSP_ADDRESS_SET2:
            OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_REMAP_OFFSET2),
                        pstDspAddressMap->offset >> DSP_CORE_REMAP_OFFSET_SHIFT, DSP_CORE_REMAP_OFFSET2_MASK);
            break;
        default:
        {
            CamOsPrintf("[DSP:%d] %s %d unknown address set:%d\n", eCoreID, __FUNCTION__, __LINE__,
                        pstDspAddressMap->eSet);
        }
    }
    return HAL_DSP_OK;
}
HalDspRet_e HalDspSetInterruptMask(HalDspCoreID_e eCoreID, HalDspIntMask_e eIntMask, __u32 u32Mask)
{
    CORE_VALID_CHECK_RETURN(eCoreID);

    switch (eIntMask)
    {
        case HAL_DSP_INT_MASK_CORE:
            OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_CPU_INT_MSK_W_LOW), u32Mask, DSP_CORE_CPU_INT_MSK_W_MASK);
            OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_CPU_INT_MSK_W_HIGH), u32Mask >> 16,
                        DSP_CORE_CPU_INT_MSK_W_MASK);
            break;
        case HAL_DSP_INT_MASK_CMDQ0:
            OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_CMDQ_INT_MSK0_W_LOW), u32Mask,
                        DSP_CORE_CMDQ_INT_MSK0_W_MASK);
            OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_CMDQ_INT_MSK0_W_HIGH), u32Mask >> 16,
                        DSP_CORE_CMDQ_INT_MSK0_W_MASK);
            break;
        case HAL_DSP_INT_MASK_CMDQ1:
            OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_CMDQ_INT_MSK1_W_LOW), u32Mask,
                        DSP_CORE_CMDQ_INT_MSK1_W_MASK);
            OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_CMDQ_INT_MSK1_W_HIGH), u32Mask >> 16,
                        DSP_CORE_CMDQ_INT_MSK1_W_MASK);
            break;
        default:
        {
            CamOsPrintf("[DSP:%d] %s %d unknown dsp core\n", eCoreID, __FUNCTION__, __LINE__);
            break;
        }
    }
    return HAL_DSP_OK;
}
HalDspRet_e HalDspInterruptTrigger(HalDspCoreID_e eCoreID, __u8 u8IrqNum)
{
    CORE_VALID_CHECK_RETURN(eCoreID);
    if (u8IrqNum > 5 || u8IrqNum == 0)
    {
        CamOsPrintf("[DSP:%d] %s %d unknown u8IrqNum[1:5] :%d \n", eCoreID, __FUNCTION__, __LINE__, u8IrqNum);
        return HAL_DSP_FAIL;
    }
    u8IrqNum = u8IrqNum - 1; // for calculate;
    switch (eCoreID)
    {
        case HAL_DSP_CORE0:
            OUTREGMSK16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP0_LSB),
                        BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP0_LSB));
            OUTREGMSK16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, DSP_DIG_CPU_INT_DSP0, DSP_DIG_CPU_INT_DSP0);
            CLRREG16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, DSP_DIG_CPU_INT_DSP0);
            CLRREG16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP0_LSB));
            break;
        case HAL_DSP_CORE1:
            OUTREGMSK16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP1_LSB),
                        BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP1_LSB));
            OUTREGMSK16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, DSP_DIG_CPU_INT_DSP1, DSP_DIG_CPU_INT_DSP1);
            CLRREG16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, DSP_DIG_CPU_INT_DSP1);
            CLRREG16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP1_LSB));
            break;
        case HAL_DSP_CORE2:
            if (u8IrqNum == 0)
            {
                OUTREGMSK16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP2_LSB_L),
                            BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP2_LSB_L));
                OUTREGMSK16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, DSP_DIG_CPU_INT_DSP2, DSP_DIG_CPU_INT_DSP2);
                CLRREG16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, DSP_DIG_CPU_INT_DSP2);
                CLRREG16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP2_LSB_L));
            }
            else
            {
                u8IrqNum = u8IrqNum - 1;
                OUTREGMSK16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_H, BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP2_LSB_H),
                            BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP2_LSB_H));
                OUTREGMSK16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, DSP_DIG_CPU_INT_DSP2, DSP_DIG_CPU_INT_DSP2);
                CLRREG16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, DSP_DIG_CPU_INT_DSP2);
                CLRREG16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_H, BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP2_LSB_H));
            }
            break;
        case HAL_DSP_CORE3:
            OUTREGMSK16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_H, BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP3_LSB),
                        BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP3_LSB));
            OUTREGMSK16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, DSP_DIG_CPU_INT_DSP3, DSP_DIG_CPU_INT_DSP3);
            CLRREG16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, DSP_DIG_CPU_INT_DSP3);
            CLRREG16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_H, BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP3_LSB));
            break;
        case HAL_DSP_COREQUAD:
            OUTREGMSK16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP0_LSB),
                        BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP0_LSB));
            OUTREGMSK16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP1_LSB),
                        BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP1_LSB));
            if (u8IrqNum == 0)
            {
                OUTREGMSK16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP2_LSB_L),
                            BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP2_LSB_L));
            }
            else
            {
                u8IrqNum = u8IrqNum - 1;
                OUTREGMSK16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_H, BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP2_LSB_H),
                            BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP2_LSB_H));
            }
            OUTREGMSK16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_H, BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP3_LSB),
                        BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP3_LSB));
            OUTREGMSK16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, DSP_DIG_CPU_INT_ALL, DSP_DIG_CPU_INT_ALL);
            CLRREG16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, DSP_DIG_CPU_INT_ALL);
            CLRREG16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP0_LSB));
            CLRREG16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP1_LSB));
            if (u8IrqNum == 0)
            {
                CLRREG16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP2_LSB_L));
            }
            else
            {
                CLRREG16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_H, BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP2_LSB_H));
            }
            CLRREG16(BASE_REG_DIG_GP_PA + DSP_DIG_CPU_INT_W_L, BIT(u8IrqNum + DSP_DIG_CPU_INT_DSP3_LSB));
            break;
        default:
        {
            CamOsPrintf("[DSP:%d] %s %d unknown dsp core\n", eCoreID, __FUNCTION__, __LINE__);
            break;
        }
    }
    return HAL_DSP_OK;
}
HalDspRet_e HalDspGetGPI(HalDspCoreID_e eCoreID, __u32 *u32Mask)
{
    CORE_VALID_CHECK_RETURN(eCoreID);
    *u32Mask = INREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_GPI_W_HIGH), DSP_CORE_GPI_W_MASK);
    *u32Mask = (*u32Mask << 16) | INREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_GPI_W_LOW), DSP_CORE_GPI_W_MASK);
    return HAL_DSP_OK;
}
HalDspRet_e HalDspSetGPI(HalDspCoreID_e eCoreID, __u32 u32Mask)
{
    CORE_VALID_CHECK_RETURN(eCoreID);
    OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_GPI_W_HIGH), u32Mask >> 16, DSP_CORE_GPI_W_MASK);
    OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_GPI_W_LOW), u32Mask, DSP_CORE_GPI_W_MASK);
    return HAL_DSP_OK;
}
HalDspRet_e HalDspGPOInterruptFetch(HalDspCoreID_e eCoreID, __u32 *u32Mask)
{
    CORE_VALID_CHECK_RETURN(eCoreID);
    *u32Mask = INREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_GPO_W_HIGH), DSP_CORE_GPO_W_MASK);
    *u32Mask = *u32Mask << 16;
    *u32Mask |= INREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_GPO_W_LOW), DSP_CORE_GPO_W_MASK);
    return HAL_DSP_OK;
}
HalDspRet_e HalDspGetDspIrqNumById(HalDspCoreID_e eCoreID, __u32 *u32IrqNum)
{
    unsigned int        IrqId = 0; // INT_IRQ_AU_SYSTEM;
    struct device_node *pDspNode;
    char *              pDspCompatString = NULL;

    CORE_VALID_CHECK_RETURN(eCoreID);

    pDspCompatString = CamOsMemAlloc(strlen("sstar,dsp0") + 1);

    if (pDspCompatString == NULL)
    {
        CamOsPrintf("[DSP:%d] %s %d pDspCompatString=NULL\n", eCoreID, __FUNCTION__, __LINE__);
        return HAL_DSP_FAIL;
    }

    strcpy(pDspCompatString, "sstar,dsp0");
    pDspCompatString[strlen("sstar,dsp0") - 1] = '0' + (eCoreID - HAL_DSP_CORE0);
    pDspCompatString[strlen("sstar,dsp0")]     = '\0';
    pDspNode                                   = of_find_compatible_node(NULL, NULL, pDspCompatString);
    if (pDspNode == NULL)
    {
        CamOsPrintf("[DSP:%d] %s %d [%s] not defined in dts\n", eCoreID, __FUNCTION__, __LINE__, pDspCompatString);
        goto fail;
    }
    IrqId = CamOfIrqToResource(pDspNode, 0, NULL);

    if (!IrqId)
    {
        CamOsPrintf("[DSP:%d] %s %d irq not defined\n", eCoreID, __FUNCTION__, __LINE__);
        goto fail;
    }
    *u32IrqNum = IrqId;
    CamOsMemRelease(pDspCompatString);
    return HAL_DSP_OK;
fail:
    CamOsMemRelease(pDspCompatString);
    return HAL_DSP_FAIL;
}
HalDspRet_e HalDspAPBRead(HalDspCoreID_e eCoreID, __u16 u16ApbAddr, __u32 *u32Value, __u32 u32ValueMask)
{
    CORE_VALID_CHECK_RETURN(eCoreID);

    if (u32Value == NULL)
        return HAL_DSP_FAIL;
    OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_APB_ADDRESS_HIGH),
                (u16ApbAddr & DSP_CORE_APB_ADDRESS_HIGH_MASK) >> DSP_CORE_APB_ADDRESS_HIGH_LSB,
                DSP_CORE_APB_ADDRESS_MASK);
    *u32Value = INREGMSK32(_halDspGetXIURegPA(eCoreID, BK_REG((u16ApbAddr & DSP_CORE_APB_ADDRESS_LOW_MASK)
                                                              >> DSP_CORE_APB_ADDRESS_LOW_LSB)),
                           u32ValueMask);
    // *u32Value |= INREGMSK16(_halDspGetXIURegPA(eCoreID, ((u16ApbAddr+2) & DSP_CORE_APB_ADDRESS_LOW_MASK)>>2),
    //                        u32ValueMask)<<16;
    return HAL_DSP_OK;
}
HalDspRet_e HalDspAPBWrite(HalDspCoreID_e eCoreID, __u16 u16ApbAddr, __u32 u32Value, __u32 u32ValueMask)
{
    CORE_VALID_CHECK_RETURN(eCoreID);
    OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_APB_ADDRESS_HIGH),
                (u16ApbAddr & DSP_CORE_APB_ADDRESS_HIGH_MASK) >> DSP_CORE_APB_ADDRESS_HIGH_LSB,
                DSP_CORE_APB_ADDRESS_MASK);
    OUTREGMSK32(_halDspGetXIURegPA(
                    eCoreID, BK_REG((u16ApbAddr & DSP_CORE_APB_ADDRESS_LOW_MASK) >> DSP_CORE_APB_ADDRESS_LOW_LSB)),
                u32Value, u32ValueMask);
    return HAL_DSP_OK;
}
HalDspRet_e HalDspGetPFaultInfo(HalDspCoreID_e eCoreID, __u64 *u64PfaultInfo)
{
    if (u64PfaultInfo == NULL)
    {
        CamOsPrintf("[DSP:%d] %s %d NULL PTR\n", eCoreID, __FUNCTION__, __LINE__);
        return HAL_DSP_FAIL;
    }
    CORE_VALID_CHECK_RETURN(eCoreID);
    *u64PfaultInfo = INREGMSK32(_halDspGetXIURegPA(eCoreID, DSP_CORE_PFAULT_INFO_DW_LOW), DSP_CORE_PFAULT_INFO_DW_MASK);
    *u64PfaultInfo |=
        INREGMSK32(_halDspGetXIURegPA(eCoreID, DSP_CORE_PFAULT_INFO_DW_HIGH), DSP_CORE_PFAULT_INFO_DW_MASK) << 32;
    return HAL_DSP_OK;
}
HalDspRet_e HalDspGetPFaultStatus(HalDspCoreID_e eCoreID, bool *bPFaultInfoValid, bool *bPFatalError)
{
    __u8 ret = 0;
    if (bPFaultInfoValid == NULL || bPFatalError == NULL)
    {
        CamOsPrintf("[DSP:%d] %s %d NULL PTR\n", eCoreID, __FUNCTION__, __LINE__);
        return HAL_DSP_FAIL;
    }
    CORE_VALID_CHECK_RETURN(eCoreID);
    ret = INREGMSK16(_halDspGetXIURegPA(eCoreID, DSP_CORE_PFAULT_VALID_ERROR), DSP_CORE_PFAULT_VALID_ERROR_MASK);
    *bPFaultInfoValid = ret & DSP_CORE_PFAULT_VALID ? true : false;
    *bPFatalError     = ret & DSP_CORE_PFAULT_ERROR ? true : false;
    return HAL_DSP_OK;
}
HalDspRet_e HalDspAPBSlvErrStatus(HalDspCoreID_e eCoreID, bool *bError)
{
    __u8 ret = 0;
    if (bError == NULL)
    {
        CamOsPrintf("[DSP:%d] %s %d NULL PTR\n", eCoreID, __FUNCTION__, __LINE__);
        return HAL_DSP_FAIL;
    }
    CORE_VALID_CHECK_RETURN(eCoreID);
    ret     = INREGMSK16(_halDspGetXIURegPA(eCoreID, DSP_CORE_APB_SLAVE_ERR), DSP_CORE_APB_SLAVE_ERR_MASK);
    *bError = ret & DSP_CORE_APB_SLAVE_ERR_CHECK ? true : false;
    return HAL_DSP_OK;
}

#define _HALDSPSETIRQSTAT(LH, MASK, V)                                                         \
    {                                                                                          \
        if (MASK & HAL_DSP_IRQ_STAT_EDGEEN)                                                    \
        {                                                                                      \
            if (pstIRQStat->bEdgeEn)                                                           \
                OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_IRQ_EDGE_EN_##LH), V, V);    \
            else                                                                               \
                CLRREG16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_IRQ_EDGE_EN_##LH), V);          \
        }                                                                                      \
        if (MASK & HAL_DSP_IRQ_STAT_IRQMSK)                                                    \
        {                                                                                      \
            if (pstIRQStat->bMskIrq)                                                           \
                OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_IRQ_MSK_##LH), V, V);        \
            else                                                                               \
                CLRREG16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_IRQ_MSK_##LH), V);              \
        }                                                                                      \
        if (MASK & HAL_DSP_IRQ_STAT_FORCEVALID)                                                \
        {                                                                                      \
            if (pstIRQStat->bForceValid)                                                       \
                OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_IRQ_EDGE_FORCE_##LH), V, V); \
            else                                                                               \
                CLRREG16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_IRQ_EDGE_FORCE_##LH), V);       \
        }                                                                                      \
        if (MASK & HAL_DSP_IRQ_STAT_CLEAR)                                                     \
        {                                                                                      \
            if (pstIRQStat->bClearIrq)                                                         \
            {                                                                                  \
                OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_IRQ_EDGE_CLEAR_##LH), V, V); \
                CLRREG16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_IRQ_EDGE_CLEAR_##LH), V);       \
            }                                                                                  \
        }                                                                                      \
        if (MASK & HAL_DSP_IRQ_STAT_POL)                                                       \
        {                                                                                      \
            if (pstIRQStat->bNegPos)                                                           \
                OUTREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_IRQ_EDGE_POL_##LH), V, V);   \
            else                                                                               \
                CLRREG16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_IRQ_EDGE_POL_##LH), V);         \
        }                                                                                      \
    }
#define HALDSPSETIRQSTAT(MASK, IRQNO)                     \
    {                                                     \
        if (IRQNO < 16)                                   \
        {                                                 \
            _HALDSPSETIRQSTAT(L, MASK, 1 << IRQNO)        \
        }                                                 \
        else                                              \
        {                                                 \
            _HALDSPSETIRQSTAT(H, MASK, 1 << (IRQNO - 16)) \
        }                                                 \
    }
#define HALDSPGETIRQSTAT(MASK, IRQNO)                                                                                  \
    {                                                                                                                  \
        if (IRQNO < 16)                                                                                                \
        {                                                                                                              \
            if (MASK & HAL_DSP_IRQ_STAT_EDGEEN)                                                                        \
                pstIRQStat->bEdgeEn = !!INREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_IRQ_EDGE_EN_L), BIT(IRQNO));  \
            if (MASK & HAL_DSP_IRQ_STAT_IRQMSK)                                                                        \
                pstIRQStat->bMskIrq = !!INREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_IRQ_MSK_L), BIT(IRQNO));      \
            if (MASK & HAL_DSP_IRQ_STAT_FORCEVALID)                                                                    \
                pstIRQStat->bForceValid =                                                                              \
                    !!INREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_IRQ_EDGE_FORCE_L), BIT(IRQNO));                 \
            if (MASK & HAL_DSP_IRQ_STAT_CLEAR)                                                                         \
                pstIRQStat->bClearIrq =                                                                                \
                    !!INREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_IRQ_EDGE_CLEAR_L), BIT(IRQNO));                 \
            if (MASK & HAL_DSP_IRQ_STAT_POL)                                                                           \
                pstIRQStat->bNegPos = !!INREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_IRQ_EDGE_POL_L), BIT(IRQNO)); \
            if (MASK & HAL_DSP_IRQ_STAT_BEFOREMSK)                                                                     \
                pstIRQStat->bStatBeforeMsk =                                                                           \
                    !!INREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_IRQ_STAT_BEFORE_MSK_L), BIT(IRQNO));            \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            if (MASK & HAL_DSP_IRQ_STAT_EDGEEN)                                                                        \
                pstIRQStat->bEdgeEn =                                                                                  \
                    !!INREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_IRQ_EDGE_EN_H), BIT(IRQNO - 16));               \
            if (MASK & HAL_DSP_IRQ_STAT_IRQMSK)                                                                        \
                pstIRQStat->bMskIrq = !!INREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_IRQ_MSK_H), BIT(IRQNO - 16)); \
            if (MASK & HAL_DSP_IRQ_STAT_FORCEVALID)                                                                    \
                pstIRQStat->bForceValid =                                                                              \
                    !!INREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_IRQ_EDGE_FORCE_H), BIT(IRQNO - 16));            \
            if (MASK & HAL_DSP_IRQ_STAT_CLEAR)                                                                         \
                pstIRQStat->bClearIrq =                                                                                \
                    !!INREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_IRQ_EDGE_CLEAR_H), BIT(IRQNO - 16));            \
            if (MASK & HAL_DSP_IRQ_STAT_POL)                                                                           \
                pstIRQStat->bNegPos =                                                                                  \
                    !!INREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_IRQ_EDGE_POL_H), BIT(IRQNO - 16));              \
            if (MASK & HAL_DSP_IRQ_STAT_BEFOREMSK)                                                                     \
                pstIRQStat->bStatBeforeMsk =                                                                           \
                    !!INREGMSK16(_halDspGetCoreRegPA(eCoreID, DSP_CORE_IRQ_STAT_BEFORE_MSK_H), BIT(IRQNO - 16));       \
        }                                                                                                              \
    }
HalDspRet_e HalDspSetIRQStat(HalDspCoreID_e eCoreID, HalDspIRQStat_t *pstIRQStat, HalDspIRQStatItem_e eStatItemMask,
                             __u8 u8IrqNo)
{
    if (pstIRQStat == NULL)
    {
        CamOsPrintf("[DSP:%d] %s %d NULL PTR\n", eCoreID, __FUNCTION__, __LINE__);
        return HAL_DSP_FAIL;
    }
    CORE_VALID_CHECK_RETURN(eCoreID);
    HALDSPSETIRQSTAT(eStatItemMask, u8IrqNo);
    return HAL_DSP_OK;
}
HalDspRet_e HalDspGetIRQStat(HalDspCoreID_e eCoreID, HalDspIRQStat_t *pstIRQStat, HalDspIRQStatItem_e eStatItemMask,
                             __u8 u8IrqNo)
{
    if (pstIRQStat == NULL)
    {
        CamOsPrintf("[DSP:%d] %s %d NULL PTR\n", eCoreID, __FUNCTION__, __LINE__);
        return HAL_DSP_FAIL;
    }
    CORE_VALID_CHECK_RETURN(eCoreID);
    HALDSPGETIRQSTAT(eStatItemMask, u8IrqNo);
    return HAL_DSP_OK;
}

HalDspRet_e HalDspGPReset(HalDspResetType_e eRstType)
{
    __u16 u16Value = 0;

    switch (eRstType)
    {
        case HAL_DSP_GP_RESET_ALL:
            u16Value = DSP_GP_RESET_ALL;
            break;
        case HAL_DSP_GP_RESET_CORE:
            u16Value = DSP_GP_RESET_CORE;
            break;
        case HAL_DSP_GP_RESET_RIU:
            u16Value = DSP_GP_RESET_RIU;
            break;
        case HAL_DSP_GP_RESET_CORE0:
            u16Value = DSP_GP_RESET_CORE0;
            break;
        case HAL_DSP_GP_RESET_CORE1:
            u16Value = DSP_GP_RESET_CORE1;
            break;
        case HAL_DSP_GP_RESET_CORE2:
            u16Value = DSP_GP_RESET_CORE2;
            break;
        case HAL_DSP_GP_RESET_CORE3:
            u16Value = DSP_GP_RESET_CORE3;
            break;
        case HAL_DSP_RESET_DONE:
            u16Value = 0;
            break;
        default:
        {
            CamOsPrintf("[eRstType:%d] %s %d unknown reset type\n", eRstType, __FUNCTION__, __LINE__);
            return HAL_DSP_FAIL;
        }
    }
    OUTREGMSK16(_halDspGetGPRegPA(DSP_GP_RESET), u16Value, DSP_GP_RESET_MASK);
    return HAL_DSP_OK;
}

HalDspRet_e HalDspSetClkGating(HalDspCoreID_e eCoreID, bool bEnable)
{
    __u16 u16Value = 0;
    __u16 u16Mask  = 0;

    CORE_VALID_CHECK_RETURN(eCoreID);
    switch (eCoreID)
    {
        case HAL_DSP_CORE0:
            u16Value = DSP_GP_VQ7_CLK_GATE_CORE0;
            u16Mask  = DSP_GP_VQ7_CLK_GATE_CORE0_MASK;
            break;
        case HAL_DSP_CORE1:
            u16Value = DSP_GP_VQ7_CLK_GATE_CORE1;
            u16Mask  = DSP_GP_VQ7_CLK_GATE_CORE1_MASK;
            break;
        case HAL_DSP_CORE2:
            u16Value = DSP_GP_VQ7_CLK_GATE_CORE2;
            u16Mask  = DSP_GP_VQ7_CLK_GATE_CORE2_MASK;
            break;
        case HAL_DSP_CORE3:
            u16Value = DSP_GP_VQ7_CLK_GATE_CORE3;
            u16Mask  = DSP_GP_VQ7_CLK_GATE_CORE3_MASK;
            break;
        default:
        {
            CamOsPrintf("[DSP:%d] %s %d unknown dsp core\n", eCoreID, __FUNCTION__, __LINE__);
            return HAL_DSP_FAIL;
        }
    }
    if (!bEnable)
        OUTREGMSK16(_halDspGetGPRegPA(DSP_GP_VQ7_CLK_GATE), u16Value, u16Mask);
    else
        CLRREG16(_halDspGetGPRegPA(DSP_GP_VQ7_CLK_GATE), u16Value);

    return HAL_DSP_OK;
}
HalDspRet_e HalDspSetClkInvert(HalDspCoreID_e eCoreID, bool bEnable)
{
    __u16 u16Value = 0;
    __u16 u16Mask  = 0;

    CORE_VALID_CHECK_RETURN(eCoreID);
    switch (eCoreID)
    {
        case HAL_DSP_CORE0:
            u16Value = DSP_GP_VQ7_CLK_INV_CORE0;
            u16Mask  = DSP_GP_VQ7_CLK_INV_CORE0_MASK;
            break;
        case HAL_DSP_CORE1:
            u16Value = DSP_GP_VQ7_CLK_INV_CORE1;
            u16Mask  = DSP_GP_VQ7_CLK_INV_CORE1_MASK;
            break;
        case HAL_DSP_CORE2:
            u16Value = DSP_GP_VQ7_CLK_INV_CORE2;
            u16Mask  = DSP_GP_VQ7_CLK_INV_CORE2_MASK;
            break;
        case HAL_DSP_CORE3:
            u16Value = DSP_GP_VQ7_CLK_INV_CORE3;
            u16Mask  = DSP_GP_VQ7_CLK_INV_CORE3_MASK;
            break;
        default:
        {
            CamOsPrintf("[DSP:%d] %s %d unknown dsp core\n", eCoreID, __FUNCTION__, __LINE__);
            return HAL_DSP_FAIL;
        }
    }
    if (bEnable)
        OUTREGMSK16(_halDspGetGPRegPA(DSP_GP_VQ7_CLK_INV), u16Value, u16Mask);
    else
        CLRREG16(_halDspGetGPRegPA(DSP_GP_VQ7_CLK_INV), u16Value);

    return HAL_DSP_OK;
}
HalDspRet_e HalDspSetDFS(HalDspCoreID_e eCoreID, bool bEnable, __u8 u8BitMaskIdx)
{
    __u32 u32RegId   = 0;
    __u32 u32MaskEn  = 0;
    __u32 u32MaskCfg = 0;
    __u32 u32ValueEn = 0;
    __u32 u32CfgLsb  = 0;
    CORE_VALID_CHECK_RETURN(eCoreID);
    switch (eCoreID)
    {
        case HAL_DSP_CORE0:
            u32RegId   = DSP_GP_VQ7_DFS_CORE0;
            u32MaskCfg = DSP_GP_VQ7_DFS_CORE0_CFG_MASK;
            u32MaskEn  = DSP_GP_VQ7_DFS_CORE0_EN_MASK;
            u32ValueEn = DSP_GP_VQ7_DFS_CORE0_EN;
            u32CfgLsb  = DSP_GP_VQ7_DFS_CORE0_CFG_LSB;
            break;
        case HAL_DSP_CORE1:
            u32RegId   = DSP_GP_VQ7_DFS_CORE1;
            u32MaskCfg = DSP_GP_VQ7_DFS_CORE1_CFG_MASK;
            u32MaskEn  = DSP_GP_VQ7_DFS_CORE1_EN_MASK;
            u32ValueEn = DSP_GP_VQ7_DFS_CORE1_EN;
            u32CfgLsb  = DSP_GP_VQ7_DFS_CORE1_CFG_LSB;

            break;
        case HAL_DSP_CORE2:
            u32RegId   = DSP_GP_VQ7_DFS_CORE2;
            u32MaskCfg = DSP_GP_VQ7_DFS_CORE2_CFG_MASK;
            u32MaskEn  = DSP_GP_VQ7_DFS_CORE2_EN_MASK;
            u32ValueEn = DSP_GP_VQ7_DFS_CORE2_EN;
            u32CfgLsb  = DSP_GP_VQ7_DFS_CORE2_CFG_LSB;

            break;
        case HAL_DSP_CORE3:
            u32RegId   = DSP_GP_VQ7_DFS_CORE3;
            u32MaskCfg = DSP_GP_VQ7_DFS_CORE3_CFG_MASK;
            u32MaskEn  = DSP_GP_VQ7_DFS_CORE3_EN_MASK;
            u32ValueEn = DSP_GP_VQ7_DFS_CORE3_EN;
            u32CfgLsb  = DSP_GP_VQ7_DFS_CORE3_CFG_LSB;

            break;
        default:
        {
            return HAL_DSP_FAIL;
        }
    }
    if (bEnable)
    {
        if (u8BitMaskIdx <= u32MaskCfg)
        {
            CamOsPrintf("[DSP:%d] %s %d \n", eCoreID, __FUNCTION__, __LINE__);
            OUTREGMSK16(_halDspGetGPRegPA(u32RegId), u8BitMaskIdx << u32CfgLsb, u32MaskCfg);
        }
        else
        {
            CamOsPrintf("[DSP:%d] %s %d %d %d\n", eCoreID, __FUNCTION__, __LINE__, u8BitMaskIdx, u32MaskCfg);
            return HAL_DSP_FAIL;
        }
        OUTREGMSK16(_halDspGetGPRegPA(u32RegId), u32ValueEn, u32MaskEn);
    }
    else
    {
        CLRREG16(_halDspGetGPRegPA(u32RegId), u32ValueEn);
    }

    return HAL_DSP_OK;
}
HalDspRet_e HalDspSetTOPClk(bool bGateEnable, bool bInvert, bool bFastClk)
{
    if (bGateEnable)
        OUTREGMSK16(_halDspGetGPRegPA(DSP_GP_TOP_CLK_CTRL), DSP_GP_TOP_CLK_CTRL_GATE_EN,
                    DSP_GP_TOP_CLK_CTRL_GATE_EN_MASK);
    else
        CLRREG16(_halDspGetGPRegPA(DSP_GP_TOP_CLK_CTRL), DSP_GP_TOP_CLK_CTRL_GATE_EN);
    if (bInvert)
        OUTREGMSK16(_halDspGetGPRegPA(DSP_GP_TOP_CLK_CTRL), DSP_GP_TOP_CLK_CTRL_CLK_INV,
                    DSP_GP_TOP_CLK_CTRL_CLK_INV_MASK);
    else
        CLRREG16(_halDspGetGPRegPA(DSP_GP_TOP_CLK_CTRL), DSP_GP_TOP_CLK_CTRL_CLK_INV);
    if (bFastClk)
        OUTREGMSK16(_halDspGetGPRegPA(DSP_GP_TOP_CLK_CTRL), DSP_GP_TOP_CLK_CTRL_CLK_SEL_FAST,
                    DSP_GP_TOP_CLK_CTRL_CLK_SEL_MASK);
    else
        OUTREGMSK16(_halDspGetGPRegPA(DSP_GP_TOP_CLK_CTRL), DSP_GP_TOP_CLK_CTRL_CLK_SEL_SLOW,
                    DSP_GP_TOP_CLK_CTRL_CLK_SEL_MASK);

    return HAL_DSP_OK;
}
HalDspRet_e HalDspSetGPIOGroup(HalDspCoreID_e eCoreID, HalDspGPIOGroupID_e eGPIOGroup)
{
    __u16 u16Value   = 0;
    __u16 u16RegVal  = 0;
    __u16 u16MaskVal = 0;

    CORE_VALID_CHECK_RETURN(eCoreID);

    switch (eCoreID)
    {
        case HAL_DSP_CORE0:
            u16Value = DSP_GP_GPIO_CORE0;
            break;
        case HAL_DSP_CORE1:
            u16Value = DSP_GP_GPIO_CORE1;
            break;
        case HAL_DSP_CORE2:
            u16Value = DSP_GP_GPIO_CORE2;
            break;
        case HAL_DSP_CORE3:
            u16Value = DSP_GP_GPIO_CORE3;
            break;
        default:
        {
            CamOsPrintf("[DSP:%d] %s %d unknown dsp core\n", eCoreID, __FUNCTION__, __LINE__);
            return HAL_DSP_FAIL;
        }
    }
    switch (eGPIOGroup)
    {
        case HAL_DSP_GPIO_GROUP_0_7:
            u16Value   = u16Value << DSP_GP_GPIO_SEL0_LSHIFT;
            u16RegVal  = DSP_GP_GPIO_SEL0;
            u16MaskVal = DSP_GP_GPIO_SEL0_MASK;
            break;
        case HAL_DSP_GPIO_GROUP_8_15:
            u16Value   = u16Value << DSP_GP_GPIO_SEL1_LSHIFT;
            u16RegVal  = DSP_GP_GPIO_SEL1;
            u16MaskVal = DSP_GP_GPIO_SEL1_MASK;
            break;
        case HAL_DSP_GPIO_GROUP_16_23:
            u16Value   = u16Value << DSP_GP_GPIO_SEL2_LSHIFT;
            u16RegVal  = DSP_GP_GPIO_SEL2;
            u16MaskVal = DSP_GP_GPIO_SEL2_MASK;
            break;
        case HAL_DSP_GPIO_GROUP_24_31:
            u16Value   = u16Value << DSP_GP_GPIO_SEL3_LSHIFT;
            u16RegVal  = DSP_GP_GPIO_SEL3;
            u16MaskVal = DSP_GP_GPIO_SEL3_MASK;
            break;
        default:
        {
            CamOsPrintf("[DSP:%d] %s %d unknown eGPIOGroup %d\n", eCoreID, __FUNCTION__, __LINE__, eGPIOGroup);
            return HAL_DSP_FAIL;
        }
    }
    OUTREGMSK16(_halDspGetGPRegPA(u16RegVal), u16Value, u16MaskVal);
    return HAL_DSP_OK;
}
HalDspRet_e HalDspSetJTAGSelection(HalDspCoreID_e eCoreID)
{
    __u16 u16Value = 0;
    CORE_VALID_CHECK_RETURN(eCoreID);

    switch (eCoreID)
    {
        case HAL_DSP_CORE0:
            u16Value = DSP_GP_JTAG_CORE0;
            break;
        case HAL_DSP_CORE1:
            u16Value = DSP_GP_JTAG_CORE1;
            break;
        case HAL_DSP_CORE2:
            u16Value = DSP_GP_JTAG_CORE2;
            break;
        case HAL_DSP_CORE3:
            u16Value = DSP_GP_JTAG_CORE3;
            break;
        default:
        {
            CamOsPrintf("[DSP:%d] %s %d unknown dsp core\n", eCoreID, __FUNCTION__, __LINE__);
            return HAL_DSP_FAIL;
        }
    }
    OUTREGMSK16(_halDspGetGPRegPA(DSP_GP_JTAG_SEL), u16Value, DSP_GP_JTAG_SEL_MASK);
    return HAL_DSP_OK;
}

HalDspRet_e HalDspSetAllSramForceON(bool bEnable)
{
    if (bEnable)
        OUTREGMSK16(_halDspGetGPRegPA(DSP_GP_FORCE_ALL_SRAM_ON), DSP_GP_FORCE_ALL_SRAM_ON_EN,
                    DSP_GP_FORCE_ALL_SRAM_ON_MASK);
    else
        CLRREG16(_halDspGetGPRegPA(DSP_GP_FORCE_ALL_SRAM_ON), DSP_GP_FORCE_ALL_SRAM_ON_EN);

    return HAL_DSP_OK;
}
HalDspRet_e HalDspSetCoreSramPower(HalDspCoreID_e eCoreID, bool bOff)
{
    CODE_STUB_CORE_SETTINNG_BIT_IN_ONE_REG(eCoreID, bOff, SRAM_POWER);
}
HalDspRet_e HalDspSetCorePGSramPower(HalDspCoreID_e eCoreID, bool bOff)
{
    CODE_STUB_CORE_SETTINNG_BIT_IN_ONE_REG(eCoreID, bOff, PG_SRAM_POWER);
}
HalDspRet_e HalDspSetCoreSramSleep(HalDspCoreID_e eCoreID, bool bEnable)
{
    CODE_STUB_CORE_SETTINNG_BIT_IN_ONE_REG(eCoreID, bEnable, SRAM_SLEEP);
}
HalDspRet_e HalDspSetCoreSramDeepSleep(HalDspCoreID_e eCoreID, bool bEnable)
{
    CODE_STUB_CORE_SETTINNG_BIT_IN_ONE_REG(eCoreID, bEnable, SRAM_DEEP_SLEEP);
}
HalDspRet_e HalDspSetCorePower(HalDspCoreID_e eCoreID, bool bOff)
{
    CODE_STUB_CORE_SETTINNG_BIT_IN_ONE_REG(eCoreID, bOff, POWER);
}
HalDspRet_e HalDspSetCoreIso(HalDspCoreID_e eCoreID, bool bEnable)
{
    CODE_STUB_CORE_SETTINNG_BIT_IN_ONE_REG(eCoreID, bEnable, ISOLATION);
}
HalDspRet_e HalDspGetCorePower(HalDspCoreID_e eCoreID, bool *bOff)
{
    CORE_VALID_CHECK_RETURN(eCoreID);
    *bOff = INREGMSK16(_halDspGetGPRegPA(DSP_GP_CORE_MTCMOS), DSP_GP_CORE_MTCMOS_MASK);
    switch (eCoreID)
    {
        case HAL_DSP_CORE0:
            *bOff = *bOff & BIT(0) ? true : false;
            break;
        case HAL_DSP_CORE1:
            *bOff = *bOff & BIT(1) ? true : false;
            break;
        case HAL_DSP_CORE2:
            *bOff = *bOff & BIT(2) ? true : false;
            break;
        case HAL_DSP_CORE3:
            *bOff = *bOff & BIT(3) ? true : false;
            break;
        default:
        {
            CamOsPrintf("[DSP:%d] %s %d unknown dsp core\n", eCoreID, __FUNCTION__, __LINE__);
            return HAL_DSP_FAIL;
        }
    }
    return HAL_DSP_OK;
}

HalDspRet_e HalDspNoDieReset(HalDspResetType_e eRstType)
{
    switch (eRstType)
    {
        case HAL_DSP_NODIE_RESET_ALL:
            OUTREGMSK16(_halDspGetGPRegPA(DSP_NODIE_RESET), DSP_NODIE_RESET_ALL, DSP_NODIE_RESET_MASK);
            break;
        case HAL_DSP_NODIE_RESET_DIE:
            OUTREGMSK16(_halDspGetGPRegPA(DSP_NODIE_RESET), DSP_NODIE_RESET_DIE, DSP_NODIE_RESET_MASK);
            break;
        case HAL_DSP_NODIE_RESET_MCU:
            OUTREGMSK16(_halDspGetGPRegPA(DSP_NODIE_RESET), DSP_NODIE_RESET_MCU, DSP_NODIE_RESET_MASK);
            break;
        case HAL_DSP_RESET_DONE:
            OUTREGMSK16(_halDspGetGPRegPA(DSP_NODIE_RESET), 0, DSP_NODIE_RESET_MASK);
            break;
        default:
        {
            CamOsPrintf("[eRstType:%d] %s %d unknown reset type\n", eRstType, __FUNCTION__, __LINE__);
            return HAL_DSP_FAIL;
        }
    }

    return HAL_DSP_OK;
}
HalDspRet_e HalDspNoDieIsoEn(bool bEnable)
{
    if (bEnable)
        OUTREGMSK16(_halDspGetGPRegPA(DSP_NODIE_TOP_ISO), DSP_NODIE_TOP_ISO_EN, DSP_NODIE_TOP_ISO_MASK);
    else
        CLRREG16(_halDspGetGPRegPA(DSP_NODIE_TOP_ISO), DSP_NODIE_TOP_ISO_EN);
    return HAL_DSP_OK;
}
HalDspRet_e HalDspNoDieBistFail(__u16 *u16Ret)
{
    *u16Ret = INREGMSK16(_halDspGetGPRegPA(DSP_NODIE_BIST_FAIL), DSP_NODIE_BIST_FAIL_MASK);
    return HAL_DSP_OK;
}
