/*
 * eMMC_hal_v5.c- Sigmastar
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
#include "eMMC.h"
#include "eMMC_platform.h"
#include "eMMC_err_codes.h"
#include "eMMC_hal.h"
#include "drv_eMMC.h"
#include "eMMC_utl.h"

#if defined(UNIFIED_eMMC_DRIVER) && UNIFIED_eMMC_DRIVER

void eMMC_FCIE_GetCMDFIFO(eMMC_IP_EmType emmc_ip, U16 u16_WordPos, U16 u16_WordCnt, U16 *pu16_Buf);

static U32 (*sgpFn_eMMC_FCIE_WaitEvents_Ex)(eMMC_IP_EmType, uintptr_t, U16, U32) = NULL;
void (*sleep_or_delay)(U32)                                                      = msleep;

char *gDebugModeName[] = {
    NULL,
    NULL, //"NAND",
    NULL, //"CIFD",
    "MMA",
    NULL, //"BOOT",
    "CARD",
    NULL, //"BIST",
    NULL,   "REG_QIFX_CNT", "REG_QIFY_CNT", "REG_DMA_REQ_CNT", "REG_DMA_RSP_CNT",
    NULL, //"BOOT_RDATA_CNT",
    NULL, //"BIST_RDATA_CNT",
    NULL, //"MACRO_DBUS1",
    NULL, //"MACRO_DBUS2",
};

U8 sgu8_IfNeedRestorePadType = 0xFF;
U8 u8_sdr_retry_count        = 0;

volatile U8 *gpu8CifdBuf;
static U32   gu32Arg;
static U8    gu8Cmd;

//========================================================
// HAL APIs
//========================================================
U32 eMMC_FCIE_WaitEvents(eMMC_IP_EmType emmc_ip, uintptr_t ulongRegAddr, U16 u16_Events, U32 u32_MicroSec)
{
    return (*sgpFn_eMMC_FCIE_WaitEvents_Ex)(emmc_ip, ulongRegAddr, u16_Events, u32_MicroSec);
}

U32 eMMC_FCIE_PollingEvents(eMMC_IP_EmType emmc_ip, uintptr_t ulongRegAddr, U16 u16_Events, U32 u32_MicroSec)
{
    volatile U32 u32_i, u32_DelayX;
    volatile U16 u16_val;

    REG_FCIE_W(FCIE_MIE_INT_EN(emmc_ip), 0); // mask interrupts
    if (u32_MicroSec > HW_TIMER_DELAY_100us)
    {
        u32_DelayX = HW_TIMER_DELAY_100us / HW_TIMER_DELAY_1us;
        u32_MicroSec /= u32_DelayX;
    }
    else
        u32_DelayX = 1;

    for (u32_i = 0; u32_i < u32_MicroSec; u32_i++)
    {
        eMMC_hw_timer_delay(HW_TIMER_DELAY_1us * u32_DelayX);
        REG_FCIE_R(ulongRegAddr, u16_val);
        if ((u16_val & u16_Events) == u16_Events)
            break;
    }

    if (u32_i == u32_MicroSec)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC_%d Err: %u us, Reg.%04lXh: %04Xh, but wanted: %04Xh\n", emmc_ip,
                   u32_MicroSec * u32_DelayX, (ulongRegAddr - FCIE_MIE_EVENT(emmc_ip)) >> REG_OFFSET_SHIFT_BITS,
                   u16_val, u16_Events);

        return eMMC_ST_ERR_TIMEOUT_WAIT_REG0;
    }

    return eMMC_ST_SUCCESS;
}

static void eMMC_FCIE_DumpDebugBus(eMMC_IP_EmType emmc_ip)
{
    U16 u16_i;
    U16 u16_j;

    eMMC_debug(eMMC_DEBUG_LEVEL, 0, "\n\n");
    eMMC_debug(eMMC_DEBUG_LEVEL, 1, "\n");

    for (u16_j = 0; u16_j < 0x10; u16_j++)
    {
        if (gDebugModeName[u16_j] == NULL)
            continue;

        REG_FCIE_CLRBIT(FCIE_EMMC_DEBUG_BUS1(emmc_ip), BIT_DEBUG_MODE_MSK);
        REG_FCIE_SETBIT(FCIE_EMMC_DEBUG_BUS1(emmc_ip), u16_j << 8);

        if (u16_j == 5)
        {
            for (u16_i = 1; u16_i <= 4; u16_i++)
            {
                REG_FCIE_CLRBIT(FCIE_TEST_MODE(emmc_ip), BIT_DEBUG_MODE_MASK);
                REG_FCIE_SETBIT(FCIE_TEST_MODE(emmc_ip), u16_i << BIT_DEBUG_MODE_SHIFT);

                eMMC_debug(0, 0, "0x39 = %04Xh, ", REG_FCIE(FCIE_EMMC_DEBUG_BUS1(emmc_ip)));
                eMMC_debug(0, 0, "0x15 = %04Xh, ", REG_FCIE(FCIE_TEST_MODE(emmc_ip)));
                eMMC_debug(0, 0, "0x38 = %04Xh", REG_FCIE(FCIE_EMMC_DEBUG_BUS0(emmc_ip)));
                eMMC_debug(eMMC_DEBUG_LEVEL, 0, "\n");
            }
        }
        else
        {
            eMMC_debug(0, 0, "0x39 = %04Xh, ", REG_FCIE(FCIE_EMMC_DEBUG_BUS1(emmc_ip)));
            eMMC_debug(0, 0, "0x38 = %04Xh", REG_FCIE(FCIE_EMMC_DEBUG_BUS0(emmc_ip)));
            eMMC_debug(eMMC_DEBUG_LEVEL, 0, "\n");
        }
    }

    eMMC_debug(eMMC_DEBUG_LEVEL, 0, "\n");
}

// static U16 sgau16_eMMCDebugReg[100];
static void eMMC_FCIE_DumpRegisters(eMMC_IP_EmType emmc_ip)
{
    volatile U16 u16_reg;
    U16          u16_i;

    eMMC_debug(eMMC_DEBUG_LEVEL, 1, "\n\neMMC_%d FCIE Reg:x%x(BK:x%x)", emmc_ip, FCIE_REG_BASE_ADDR(emmc_ip),
               (FCIE_REG_BASE_ADDR(emmc_ip) - 0x1F000000) >> 9);

    for (u16_i = 0; u16_i < 0x80; u16_i++)
    {
        if (0 == u16_i % 8)
            eMMC_debug(eMMC_DEBUG_LEVEL, 0, "\n%02Xh:| ", u16_i);

        REG_FCIE_R(GET_REG_ADDR(FCIE_REG_BASE_ADDR(emmc_ip), u16_i), u16_reg);
        eMMC_debug(eMMC_DEBUG_LEVEL, 0, "%04Xh ", u16_reg);
    }
    eMMC_debug(eMMC_DEBUG_LEVEL, 0, "\n");

#if 0
    //eMMC_debug(eMMC_DEBUG_LEVEL, 0, "FCIE JobCnt: \n");
    //eMMC_debug(eMMC_DEBUG_LEVEL, 0, "JobCnt: %Xh \n", REG_FCIE(FCIE_JOB_BL_CNT(emmc_ip)));

    eMMC_debug(eMMC_DEBUG_LEVEL, 0, "\nFCIE CMDFIFO:");
    eMMC_FCIE_GetCMDFIFO(emmc_ip, 0, 0x09, (U16*)sgau16_eMMCDebugReg);
    for(u16_i=0 ; u16_i<0x20; u16_i++)
    {
        if(0 == u16_i%8)
            eMMC_debug(eMMC_DEBUG_LEVEL,0,"\n%02Xh:| ", u16_i);

        eMMC_debug(eMMC_DEBUG_LEVEL, 0, "%04Xh ", sgau16_eMMCDebugReg[u16_i]);
    }
#endif

#if defined(EMMC_PLL_BASE)
    eMMC_debug(eMMC_DEBUG_LEVEL, 1, "\n\nEMMCPLL Reg:x%x(BK:x%x)", EMMC_PLL_BASE(emmc_ip),
               (EMMC_PLL_BASE(emmc_ip) - 0x1F000000) >> 9);

    for (u16_i = 0; u16_i < 0x80; u16_i++)
    {
        if (0 == u16_i % 8)
            eMMC_debug(eMMC_DEBUG_LEVEL, 0, "\n%02Xh:| ", u16_i);

        REG_FCIE_R(GET_REG_ADDR(EMMC_PLL_BASE(emmc_ip), u16_i), u16_reg);
        eMMC_debug(eMMC_DEBUG_LEVEL, 0, "%04Xh ", u16_reg);
    }
    eMMC_debug(eMMC_DEBUG_LEVEL, 0, "\n");
#endif

    eMMC_debug(eMMC_DEBUG_LEVEL, 1, "\n\nCHIPTOP Reg:x%x(BK:x%x)", PAD_CHIPTOP_BASE,
               (PAD_CHIPTOP_BASE - 0x1F000000) >> 9);

    for (u16_i = 0; u16_i < 0x80; u16_i++)
    {
        if (0 == u16_i % 8)
            eMMC_debug(eMMC_DEBUG_LEVEL, 0, "\n%02Xh:| ", u16_i);

        REG_FCIE_R(GET_REG_ADDR(PAD_CHIPTOP_BASE, u16_i), u16_reg);
        eMMC_debug(eMMC_DEBUG_LEVEL, 0, "%04Xh ", u16_reg);
    }
    eMMC_debug(eMMC_DEBUG_LEVEL, 0, "\n");

    eMMC_debug(eMMC_DEBUG_LEVEL, 1, "\r\n\r\nCLK_GEN Reg:x%x(BK:x%x)", CLKGEN0_BASE, (CLKGEN0_BASE & 0x00FFFFFF) >> 9);
    for (u16_i = 0; u16_i < 0x80; u16_i++)
    {
        if (0 == u16_i % 8)
            eMMC_debug(eMMC_DEBUG_LEVEL, 0, "\r\n%02Xh:| ", u16_i);

        REG_FCIE_R(GET_REG_ADDR(CLKGEN0_BASE, u16_i), u16_reg);
        eMMC_debug(eMMC_DEBUG_LEVEL, 0, "%04Xh ", u16_reg);
    }
    eMMC_debug(eMMC_DEBUG_LEVEL, 0, "\r\n");
}

/* Dump relevant */
static U32 eMMC_Dump_eMMCStatus(eMMC_IP_EmType emmc_ip)
{
    eMMC_CMD13(emmc_ip, g_eMMCDrv[emmc_ip].u16_RCA);
    eMMC_debug(eMMC_DEBUG_LEVEL, 0, "\n");
    eMMC_debug(eMMC_DEBUG_LEVEL, 1, "eMMC St: %Xh %Xh %Xh %Xh \n", g_eMMCDrv[emmc_ip].au8_Rsp[1],
               g_eMMCDrv[emmc_ip].au8_Rsp[2], g_eMMCDrv[emmc_ip].au8_Rsp[3], g_eMMCDrv[emmc_ip].au8_Rsp[4]);
    return eMMC_ST_SUCCESS;
}

void eMMC_FCIE_ErrHandler_Stop(eMMC_IP_EmType emmc_ip)
{
    if (g_eMMCDrv[emmc_ip].u8_make_sts_err)
        return;

    if (0 == eMMC_IF_TUNING_TTABLE(emmc_ip))
    {
        eMMC_DumpDriverStatus(emmc_ip);
        eMMC_DumpPadClk(emmc_ip);
        eMMC_FCIE_DumpRegisters(emmc_ip);
        eMMC_FCIE_DumpDebugBus(emmc_ip);
        if (gu8Cmd != 13)
            eMMC_Dump_eMMCStatus(emmc_ip);
        // eMMC_die("\n");
    }
    else
    {
        // eMMC_FCIE_Init(emmc_ip);
    }
}

static void eMMC_FCIE_SymmetrySkew4(eMMC_IP_EmType emmc_ip)
{
    g_eMMCDrv[emmc_ip].TimingTable_G_t.u8_CurSetIdx = 4 - g_eMMCDrv[emmc_ip].TimingTable_G_t.u8_CurSetIdx;
    eMMC_FCIE_ApplyTimingSet(emmc_ip, g_eMMCDrv[emmc_ip].TimingTable_G_t.u8_CurSetIdx); // need ip
    // eMMC_DumpTimingTable();
}

U32 eMMC_FCIE_ErrHandler_Retry(eMMC_IP_EmType emmc_ip)
{
#if 0 //!(defined(ENABLE_eMMC_ATOP) && ENABLE_eMMC_ATOP)
    static U8 u8_IfToggleDataSync=0;
#endif
    U32 u32_err;
    if (g_eMMCDrv[emmc_ip].u8_make_sts_err)
        return eMMC_ST_SUCCESS;

    eMMC_FCIE_Init(emmc_ip);

    REG_FCIE_SETBIT(FCIE_MIE_FUNC_CTL(emmc_ip), BIT_EMMC_ACTIVE);

#if 0 //!(defined(ENABLE_eMMC_ATOP) && ENABLE_eMMC_ATOP)
    if((++u8_IfToggleDataSync) & 1)
    {
        g_eMMCDrv[emmc_ip].u16_Reg10_Mode ^= BIT_SD_DATA_SYNC;
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 0, "eMMC: retry DATA_SYNC: %Xh\n",
            g_eMMCDrv[emmc_ip].u16_Reg10_Mode&BIT_SD_DATA_SYNC);

        return eMMC_ST_SUCCESS;
    }
#endif

    // -------------------------------------------------
    // if error @ init, do not retry.
    if (0 == (g_eMMCDrv[emmc_ip].u32_DrvFlag & DRV_FLAG_INIT_DONE))
        return eMMC_ST_SUCCESS;

    if (u8_sdr_retry_count < 2 && g_eMMCDrv[emmc_ip].u8_PadType == FCIE_eMMC_HS400
        && g_eMMCDrv[emmc_ip].TimingTable_G_t.u8_SetCnt != 0)
    {
        eMMC_FCIE_SymmetrySkew4(emmc_ip);
        ++u8_sdr_retry_count;
    }
    else
    {
        // -------------------------------------------------
        if (!eMMC_IF_NORMAL_SDR(emmc_ip))
        {
            sgu8_IfNeedRestorePadType = g_eMMCDrv[emmc_ip].u8_PadType;
            u32_err                   = eMMC_FCIE_EnableSDRMode(emmc_ip);
            if (eMMC_ST_SUCCESS != u32_err)
            {
                eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC%d Err: EnableSDRModefail: %Xh\n", emmc_ip, u32_err);
            }
        }
        u8_sdr_retry_count = 0;
    }

    eMMC_debug(eMMC_DEBUG_LEVEL_LOW, 0, "eMMC%d Warn: slow clk to %u.%uMHz, %Xh\n", emmc_ip,
               g_eMMCDrv[emmc_ip].u32_ClkKHz / 1000, (g_eMMCDrv[emmc_ip].u32_ClkKHz % 1000) / 100,
               g_eMMCDrv[emmc_ip].u16_ClkRegVal);

    return eMMC_ST_SUCCESS;
}

void eMMC_FCIE_ErrHandler_RestoreClk(eMMC_IP_EmType emmc_ip)
{
    if (0xFF != sgu8_IfNeedRestorePadType)
    {
        U32 u32_err;
        u32_err = eMMC_FCIE_EnableFastMode_Ex(emmc_ip, sgu8_IfNeedRestorePadType);
        if (u32_err)
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC%d Err: EnableFastMode_Ex fail, %Xh\n", emmc_ip, u32_err);
            return;
        }
        sgu8_IfNeedRestorePadType = 0xFF;
        u8_sdr_retry_count        = 0;
        return;
    }

    if (eMMC_IF_NORMAL_SDR(emmc_ip) && FCIE_DEFAULT_CLK != g_eMMCDrv[emmc_ip].u16_ClkRegVal)
    {
        // Some Cards need to mask this step. eg: NM Card.
        // eMMC_clock_setting(emmc_ip, FCIE_DEFAULT_CLK);
        return;
    }
}

U32 eMMC_FCIE_ErrHandler_ReInit_Ex(eMMC_IP_EmType emmc_ip)
{
    U32 u32_err;
    u32_err = eMMC_FCIE_Init(emmc_ip);
    if (u32_err)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC%d Err: FCIE_Init fail, %Xh\n", emmc_ip, u32_err);
        return u32_err;
    }

    REG_FCIE_SETBIT(FCIE_MIE_FUNC_CTL(emmc_ip), BIT_EMMC_ACTIVE);

    g_eMMCDrv[emmc_ip].u32_DrvFlag = 0;
    eMMC_PlatformInit(emmc_ip);
    u32_err = eMMC_Identify(emmc_ip);
    if (u32_err)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC%d Err: Identify fail, %Xh\n", emmc_ip, u32_err);
        return u32_err;
    }

    eMMC_clock_setting(emmc_ip, FCIE_SLOW_CLK);

    u32_err = eMMC_CMD3_CMD7(emmc_ip, g_eMMCDrv[emmc_ip].u16_RCA, 7);
    if (u32_err)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC%d Err: CMD7 fail, %Xh\n", emmc_ip, u32_err);
        return u32_err;
    }

    u32_err = eMMC_SetBusSpeed(emmc_ip, eMMC_SPEED_HIGH);

    return u32_err;
}

void eMMC_FCIE_ErrHandler_ReInit(eMMC_IP_EmType emmc_ip)
{
    U32 u32_err;
    U32 u32_DrvFlag   = g_eMMCDrv[emmc_ip].u32_DrvFlag;
    U16 u16_Reg10     = g_eMMCDrv[emmc_ip].u16_Reg10_Mode;
    U8  u8_OriPadType = g_eMMCDrv[emmc_ip].u8_PadType;
    // U16 u16_OldClkRegVal = g_eMMCDrv[emmc_ip].u16_ClkRegVal;

    if (g_eMMCDrv[emmc_ip].u8_make_sts_err)
        return;

    u32_err = eMMC_FCIE_ErrHandler_ReInit_Ex(emmc_ip);
    if (u32_err)
        goto LABEL_REINIT_END;

    // ---------------------------------
    g_eMMCDrv[emmc_ip].u32_DrvFlag = u32_DrvFlag;
    if (eMMC_IF_NORMAL_SDR(emmc_ip))
    {
        u32_err = eMMC_FCIE_EnableSDRMode(emmc_ip);
        if (u32_err)
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC%d Err: EnableSDRMode fail, %Xh\n", emmc_ip, u32_err);
            goto LABEL_REINIT_END;
        }
        // eMMC_clock_setting(emmc_ip, u16_OldClkRegVal);
    }
    else
    {
        u32_err = eMMC_FCIE_EnableFastMode_Ex(emmc_ip, u8_OriPadType);
        if (u32_err)
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC%d Err: EnableFastMode_Ex fail, %Xh\n", emmc_ip, u32_err);
            goto LABEL_REINIT_END;
        }
    }

    g_eMMCDrv[emmc_ip].u16_Reg10_Mode = u16_Reg10;
    REG_FCIE_W(FCIE_SD_MODE(emmc_ip), g_eMMCDrv[emmc_ip].u16_Reg10_Mode);

LABEL_REINIT_END:
    if (u32_err)
        eMMC_die("\n");
}

void eMMC_FCIE_CheckResetDone(eMMC_IP_EmType emmc_ip)
{
    volatile U16 au16_tmp[FCIE_WAIT_RST_DONE_CNT] = {0};
    volatile U32 u32_i, u32_j, u32_err;

    for (u32_i = 0; u32_i < TIME_WAIT_FCIE_RESET; u32_i++)
    {
        for (u32_j = 0; u32_j < FCIE_WAIT_RST_DONE_CNT; u32_j++)
            REG_FCIE_W(FCIE_CMDFIFO_ADDR(emmc_ip, u32_j), FCIE_WAIT_RST_DONE_D1 + u32_j);

        for (u32_j = 0; u32_j < FCIE_WAIT_RST_DONE_CNT; u32_j++)
            REG_FCIE_R(FCIE_CMDFIFO_ADDR(emmc_ip, u32_j), au16_tmp[u32_j]);

        for (u32_j = 0; u32_j < FCIE_WAIT_RST_DONE_CNT; u32_j++)
            if (FCIE_WAIT_RST_DONE_D1 + u32_j != au16_tmp[u32_j])
                break;

        if (FCIE_WAIT_RST_DONE_CNT == u32_j)
            break;
        eMMC_hw_timer_delay(HW_TIMER_DELAY_1us);
    }

    if (TIME_WAIT_FCIE_RESET == u32_i)
    {
        u32_err = eMMC_ST_ERR_FCIE_NO_RIU;
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC%d Err: check CIFC fail: %Xh \n", emmc_ip, u32_err);
        eMMC_FCIE_ErrHandler_Stop(emmc_ip);
    }
}

U32 eMMC_FCIE_Reset(eMMC_IP_EmType emmc_ip)
{
    U16 u16Cnt;
    // Some Cards need to mask this step. eg: NM Card.
    // U16 u16_clk = g_eMMCDrv[emmc_ip].u16_ClkRegVal;

    // eMMC_debug(1, 1, "eMMC_FCIE_Reset()\n");

    REG_FCIE_CLRBIT(FCIE_SD_CTRL(emmc_ip), BIT_JOB_START); // clear for safe

    // Some Cards need to mask this step. eg: NM Card.
    // eMMC_clock_setting(emmc_ip, FCIE_DEFO_SPEED_CLK); // instead reset clock source from UPLL to XTAL

    REG_FCIE_CLRBIT(FCIE_RST(emmc_ip), BIT_FCIE_SOFT_RST_n); /* active low */

    u16Cnt = 0;
    while (1)
    {
        if ((REG_FCIE(FCIE_RST(emmc_ip)) & BIT_RST_STS_MASK) == BIT_RST_STS_MASK) // reset success
            break;

        eMMC_hw_timer_delay(HW_TIMER_DELAY_1us);

        if (u16Cnt++ >= 1000)
            eMMC_debug(1, 0, "eMMC%d Err: FCIE reset fail!\n", emmc_ip);
    }

    REG_FCIE_SETBIT(FCIE_RST(emmc_ip), BIT_FCIE_SOFT_RST_n);
    u16Cnt = 0;
    while (1)
    {
        if ((REG_FCIE(FCIE_RST(emmc_ip)) & BIT_RST_STS_MASK) == 0) // reset success
            break;

        eMMC_hw_timer_delay(HW_TIMER_DELAY_1us);

        if (u16Cnt++ >= 1000)
            eMMC_debug(1, 0, "eMMC%d Err: FCIE reset fail2!\n", emmc_ip);
    }

    // Some Cards need to mask this step. eg: NM Card.
    // eMMC_clock_setting(emmc_ip, u16_clk);
    return 0;

#if 0
    U16 u16Reg, u16Cnt;
    U32 u32_err = eMMC_ST_SUCCESS;
    U16 u16_clk = g_eMMCDrv[emmc_ip].u16_ClkRegVal;

    eMMC_clock_setting(emmc_ip, gau8_FCIEClkSel[0]); // speed up FCIE reset done
    REG_FCIE_CLRBIT(FCIE_MIE_FUNC_CTL(emmc_ip), BIT_FUNC_MASK);
    REG_FCIE_SETBIT(FCIE_MIE_FUNC_CTL(emmc_ip), BIT_MIE_FUNC_ENABLE);
    eMMC_FCIE_CLK_DIS(emmc_ip); // do not output clock

    // FCIE reset - set
    REG_FCIE_CLRBIT(FCIE_RST(emmc_ip), BIT_FCIE_SOFT_RST_n); /* active low */
    REG_FCIE_CLRBIT(reg_emmcpll_0x6f, BIT1|BIT0);       //macro reset

    // FCIE reset - wait

    u16Cnt=0;
    do
    {
        eMMC_hw_timer_delay(HW_TIMER_DELAY_1us);
        if(0x1000 == u16Cnt++)
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR,1,"eMMC Err: FCIE Reset fail: %Xh \n", eMMC_ST_ERR_FCIE_NO_CLK);
            return eMMC_ST_ERR_FCIE_NO_CLK;
        }

        REG_FCIE_R(FCIE_RST(emmc_ip), u16Reg);

    }while (BIT_RST_STS_MASK  != (u16Reg  & BIT_RST_STS_MASK));

    //[FIXME] is there any method to check that reseting FCIE is done?

    // FCIE reset - clear
    REG_FCIE_SETBIT(FCIE_RST(emmc_ip), BIT_FCIE_SOFT_RST_n);
    REG_FCIE_SETBIT(reg_emmcpll_0x6f, BIT1|BIT0);
    // FCIE reset - check

    u16Cnt=0;
    do
    {
        eMMC_hw_timer_delay(HW_TIMER_DELAY_1us);
        if(0x1000 == u16Cnt++)
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR,1,"eMMC Err: FCIE Reset fail2: %Xh \n", eMMC_ST_ERR_FCIE_NO_CLK);
            return eMMC_ST_ERR_FCIE_NO_CLK;
        }

        REG_FCIE_R(FCIE_RST(emmc_ip), u16Reg);

    }while (0  != (u16Reg  & BIT_RST_STS_MASK));

    eMMC_clock_setting(emmc_ip, u16_clk);
    return u32_err;
#endif
}

U32 eMMC_FCIE_Init(eMMC_IP_EmType emmc_ip)
{
    U32 u32_err;

// ------------------------------------------
// setup function pointer to wait for events
#if defined(ENABLE_eMMC_INTERRUPT_MODE) && ENABLE_eMMC_INTERRUPT_MODE
    if (NULL == sgpFn_eMMC_FCIE_WaitEvents_Ex)
        sgpFn_eMMC_FCIE_WaitEvents_Ex = eMMC_WaitCompleteIntr;
#else
    if (NULL == sgpFn_eMMC_FCIE_WaitEvents_Ex)
        sgpFn_eMMC_FCIE_WaitEvents_Ex = eMMC_FCIE_PollingEvents;
#endif

    // ------------------------------------------
    eMMC_PlatformResetPre();

// ------------------------------------------
#if eMMC_TEST_IN_DESIGN
    {
        volatile U16 u16_i, u16_reg;
        // check timer clock
        eMMC_debug(eMMC_DEBUG_LEVEL, 1, "Timer test, for 6 sec: ");
        for (u16_i = 6; u16_i > 0; u16_i--)
        {
            eMMC_debug(eMMC_DEBUG_LEVEL, 1, "%u ", u16_i);
            eMMC_hw_timer_delay(HW_TIMER_DELAY_1s);
        }
        eMMC_debug(eMMC_DEBUG_LEVEL, 0, "\n");

        // check FCIE reg.30h
        REG_FCIE_R(FCIE_TEST_MODE(emmc_ip), u16_reg);
        if (0) // u16_reg & BIT_FCIE_BIST_FAIL) /* Andersen: "don't care." */
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC%d Err: Reg0x30h BIST fail: %04Xh \r\n", emmc_ip, u16_reg);
            return eMMC_ST_ERR_BIST_FAIL;
        }
        if (u16_reg & BIT_FCIE_DEBUG_MODE_MASK)
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC%d Err: Reg0x30h Debug Mode: %04Xh \r\n", emmc_ip, u16_reg);
            return eMMC_ST_ERR_DEBUG_MODE;
        }

        u32_err = eMMC_FCIE_Reset(emmc_ip);
        if (eMMC_ST_SUCCESS != u32_err)
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC%d Err: reset fail\n", emmc_ip);
            eMMC_FCIE_ErrHandler_Stop(emmc_ip);
            return u32_err;
        }
    }
#endif // eMMC_TEST_IN_DESIGN

    // ------------------------------------------
    u32_err = eMMC_FCIE_Reset(emmc_ip);
    if (eMMC_ST_SUCCESS != u32_err)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC%d Err: reset fail: %Xh\n", emmc_ip, u32_err);
        eMMC_FCIE_ErrHandler_Stop(emmc_ip);
        return u32_err;
    }

    REG_FCIE_W(FCIE_MIE_INT_EN(emmc_ip), 0);
    REG_FCIE_W(FCIE_MIE_FUNC_CTL(emmc_ip), IP_BIT_FUNC_ENABLE(emmc_ip) /*| BIT_EMMC_ACTIVE*/);

    // Select MIU burst 8, if set to 1, the MIU may not have time to process events.
    REG_FCIE_SETBIT(FCIE_MMA_PRI_REG(emmc_ip), BIT0 | BIT1 | BIT4 | BIT5);

    // all cmd are 5 bytes (excluding CRC)
    REG_FCIE_CLRBIT(FCIE_CMD_RSP_SIZE(emmc_ip), BIT_CMD_SIZE_MASK);
    REG_FCIE_SETBIT(FCIE_CMD_RSP_SIZE(emmc_ip), (eMMC_CMD_BYTE_CNT) << BIT_CMD_SIZE_SHIFT);
    REG_FCIE_W(FCIE_SD_CTRL(emmc_ip), 0);
    REG_FCIE_W(FCIE_SD_MODE(emmc_ip), g_eMMCDrv[emmc_ip].u16_Reg10_Mode);
    // default sector size: 0x200
    REG_FCIE_W(FCIE_BLK_SIZE(emmc_ip), eMMC_SECTOR_512BYTE);

    REG_FCIE_W(FCIE_RSP_SHIFT_CNT(emmc_ip), 0);
    REG_FCIE_W(FCIE_RX_SHIFT_CNT(emmc_ip), 0);
    // REG_FCIE_CLRBIT(FCIE_RX_SHIFT_CNT(emmc_ip), BIT_RSTOP_SHIFT_SEL|BIT_RSTOP_SHIFT_TUNE_MASK);

    eMMC_FCIE_ClearEvents(emmc_ip);
    eMMC_PlatformResetPost();

    return eMMC_ST_SUCCESS;
}

/* Clear Event and error status */
void eMMC_FCIE_ClearEvents(eMMC_IP_EmType emmc_ip)
{
    volatile U16 u16_reg;
    while (1)
    {
        REG_FCIE_W(FCIE_MIE_EVENT(emmc_ip), BIT_ALL_CARD_INT_EVENTS);
        REG_FCIE_R(FCIE_MIE_EVENT(emmc_ip), u16_reg);
        if (0 == (u16_reg & BIT_ALL_CARD_INT_EVENTS))
            break;
        REG_FCIE_W(FCIE_MIE_EVENT(emmc_ip), 0);
        REG_FCIE_W(FCIE_MIE_EVENT(emmc_ip), 0);
    }
    REG_FCIE_W1C(FCIE_SD_STATUS(emmc_ip), BIT_SD_FCIE_ERR_FLAGS); // W1C
}

/* Clear Event only */
void eMMC_FCIE_ClearEvents_Reg0(eMMC_IP_EmType emmc_ip)
{
    volatile U16 u16_reg;

    while (1)
    {
        REG_FCIE_W(FCIE_MIE_EVENT(emmc_ip), BIT_ALL_CARD_INT_EVENTS);
        REG_FCIE_R(FCIE_MIE_EVENT(emmc_ip), u16_reg);
        if (0 == (u16_reg & BIT_ALL_CARD_INT_EVENTS))
            break;
        REG_FCIE_W(FCIE_MIE_EVENT(emmc_ip), 0);
        REG_FCIE_W(FCIE_MIE_EVENT(emmc_ip), 0);
    }
}

U32 eMMC_FCIE_WaitD0High_Ex(eMMC_IP_EmType emmc_ip, U32 u32_us)
{
    volatile U32 u32_cnt;
    volatile U16 u16_read0 = 0, u16_read1 = 0;

    for (u32_cnt = 0; u32_cnt < u32_us; u32_cnt++)
    {
        REG_FCIE_R(FCIE_SD_STATUS(emmc_ip), u16_read0);
        eMMC_hw_timer_delay(HW_TIMER_DELAY_1us);
        REG_FCIE_R(FCIE_SD_STATUS(emmc_ip), u16_read1);

        if ((u16_read0 & BIT_SD_CARD_BUSY) == 0 && (u16_read1 & BIT_SD_CARD_BUSY) == 0)
            break;

        if (u32_cnt > 500 && u32_us - u32_cnt > 1000)
        {
            eMMC_hw_timer_sleep(1);
            u32_cnt += 1000 - 2;
        }
    }

    return u32_cnt;
}

U32 eMMC_FCIE_WaitD0High(eMMC_IP_EmType emmc_ip, U32 u32_us)
{
    volatile U32 u32_cnt;

    REG_FCIE_SETBIT(FCIE_SD_MODE(emmc_ip), BIT_CLK_EN);
    u32_cnt = eMMC_FCIE_WaitD0High_Ex(emmc_ip, u32_us);

    if (u32_us == u32_cnt)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC%d Err: wait D0 H timeout %u us\n", emmc_ip, u32_cnt);
        return eMMC_ST_ERR_TIMEOUT_WAITD0HIGH;
    }

    return eMMC_ST_SUCCESS;
}

void eMMC_FCIE_TransCmdSetting(eMMC_IP_EmType emmc_ip, U16 u16BlkCnt, U16 u16BlkSize, dma_addr_t sizeBufAddr,
                               eMMC_Trans_EmType e_TransType)
{
    U32 u32BufLen = u16BlkCnt * u16BlkSize;

    if (g_eMMCDrv[emmc_ip].u8_make_sts_err == FCIE_MAKE_RD_CRC_ERR)
    {
        u32BufLen++;
        u16BlkSize++;
    }
    else if (g_eMMCDrv[emmc_ip].u8_make_sts_err == FCIE_MAKE_WR_CRC_ERR)
    {
        u32BufLen--;
        u16BlkSize--;
    }
    else if (g_eMMCDrv[emmc_ip].u8_make_sts_err == FCIE_MAKE_WR_TOUT_ERR)
    {
        sizeBufAddr = 0;
    }

    REG_FCIE_W(FCIE_BLK_SIZE(emmc_ip), u16BlkSize);
    REG_FCIE_W(FCIE_JOB_BL_CNT(emmc_ip), u16BlkCnt);

    if (e_TransType == EMMC_ADMA)
    {
        REG_FCIE_W(FCIE_JOB_BL_CNT(emmc_ip), 1);
        REG_FCIE_W(FCIE_MIU_DMA_LEN_15_0(emmc_ip), 0x10);
        REG_FCIE_W(FCIE_MIU_DMA_LEN_31_16(emmc_ip), 0);
    }
    else
    {
        REG_FCIE_W(FCIE_JOB_BL_CNT(emmc_ip), u16BlkCnt);
        REG_FCIE_W(FCIE_MIU_DMA_LEN_15_0(emmc_ip), u32BufLen & 0xFFFF);
        REG_FCIE_W(FCIE_MIU_DMA_LEN_31_16(emmc_ip), u32BufLen >> 16);
    }

    if (e_TransType == EMMC_CIF)
    {
        gpu8CifdBuf = (U8 *)(uintptr_t)sizeBufAddr;
    }
    else
    {
        REG_FCIE_W(FCIE_MIU_DMA_ADDR_15_0(emmc_ip), sizeBufAddr & 0xFFFF);
        REG_FCIE_W(FCIE_MIU_DMA_ADDR_31_16(emmc_ip), (sizeBufAddr >> 16) & 0xFFFF);
        REG_FCIE_W(FCIE_MIU_DMA_ADDR_35_32(emmc_ip), ((U64)sizeBufAddr >> 32) & 0xF);
    }
}

void eMMC_FCIE_SetCmdTaken(eMMC_IP_EmType emmc_ip, U32 u32_Arg, U8 u8_CmdIdx)
{
    REG_FCIE_W(FCIE_CMDFIFO_ADDR(emmc_ip, 0), ((u32_Arg >> 24) << 8) | (0x40 | u8_CmdIdx));
    REG_FCIE_W(FCIE_CMDFIFO_ADDR(emmc_ip, 1), (u32_Arg & 0xFF00) | ((u32_Arg >> 16) & 0xFF));
    REG_FCIE_W(FCIE_CMDFIFO_ADDR(emmc_ip, 2), u32_Arg & 0xFF);

    gu8Cmd  = u8_CmdIdx;
    gu32Arg = u32_Arg;
}

U32 eMMC_FCIE_JobStartAndWaitEvent(eMMC_IP_EmType emmc_ip, eMMC_Trans_EmType e_TransType, eMMC_Cmd_EmType e_CmdType,
                                   U16 u16_waitEvent, U32 u32_waitTimes)
{
    U32 u32_err;
    U16 u16_reg;
    U8  u8_retry_fcie = 0, u8_retry_cmd = 0;
    U16 u16_ctrl = (((U8)e_TransType & BIT_ADMA_EN) | BIT_SD_DTRX_EN | BIT_ERR_DET_ON);

LABEL_SEND_CMD:
    eMMC_FCIE_ClearEvents_Reg0(emmc_ip);
    if (e_CmdType == eMMC_CMD_WRITE)
    {
        if (eMMC_ST_SUCCESS != eMMC_FCIE_WaitD0High(emmc_ip, TIME_WAIT_DAT0_HIGH))
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: wait D0 H TO\n");
            eMMC_FCIE_ErrHandler_Stop(emmc_ip);
        }
        u16_ctrl |= BIT_SD_DAT_DIR_W;
    }
#if defined(ENABLE_eMMC_INTERRUPT_MODE) && ENABLE_eMMC_INTERRUPT_MODE
    REG_FCIE_W(FCIE_MIE_INT_EN(emmc_ip), BIT_DMA_END | BIT_ERR_STS);
#endif

    REG_FCIE_W(FCIE_SD_CTRL(emmc_ip), u16_ctrl);
    REG_FCIE_W(FCIE_SD_CTRL(emmc_ip), u16_ctrl | BIT_JOB_START);

    u32_err = eMMC_FCIE_WaitEvents(emmc_ip, FCIE_MIE_EVENT(emmc_ip), u16_waitEvent, u32_waitTimes);
    if (eMMC_ST_SUCCESS != u32_err)
    {
        if (u8_retry_cmd < eMMC_FCIE_CMD_RSP_ERR_RETRY_CNT && 0 == eMMC_IF_TUNING_TTABLE(emmc_ip)
            && eMMC_Error_Retry(emmc_ip))
        {
            u8_retry_cmd++;
            eMMC_debug(eMMC_DEBUG_LEVEL_WARNING, 1, "eMMC_%d Jobstart WARN: wait event error:%X \n", emmc_ip, u32_err);
            goto SEND_CMD_RETRY;
        }
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC_%d Jobstart ERR: wait event error:%X \n", emmc_ip, u32_err);
        goto LABEL_SEND_CMD_ERROR;
    }

    REG_FCIE_R(FCIE_SD_STATUS(emmc_ip), u16_reg);
    if (u16_reg & BIT_SD_FCIE_ERR_FLAGS)
    {
        if (u8_retry_fcie < eMMC_CMD_API_ERR_RETRY_CNT && 0 == eMMC_IF_TUNING_TTABLE(emmc_ip)
            && eMMC_Error_Retry(emmc_ip))
        {
            u8_retry_fcie++;
            eMMC_debug(eMMC_DEBUG_LEVEL_WARNING, 1, "eMMC_%d Jobstart WARN: check SD_STS:%X \n", emmc_ip, u16_reg);
            goto SEND_CMD_RETRY;
        }
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC_%d Jobstart ERR: check SD_STS:%X \n", emmc_ip, u16_reg);
        u32_err = u16_reg;
        goto LABEL_SEND_CMD_ERROR;
    }

LABEL_SEND_CMD_ERROR:
    if (u32_err)
    {
        eMMC_FCIE_ErrHandler_Stop(emmc_ip);
    }

    return u32_err;

SEND_CMD_RETRY:
    eMMC_FCIE_ErrHandler_Retry(emmc_ip);
    goto LABEL_SEND_CMD;
}

U32 eMMC_FCIE_SendCmdAndWaitProcess(eMMC_IP_EmType emmc_ip, U8 u8_CmdIdx, eMMC_Trans_EmType e_TransType,
                                    eMMC_Cmd_EmType e_CmdType, eMMC_Rsp_EmType e_RspType)
{
    U16 u16_mode, u16_ctrl, u16_reg;
    U32 u32_err;
    U8  u8_retry_r1 = 0, u8_retry_fcie = 0, u8_retry_cmd = 0;

LABEL_SEND_CMD:
    u16_mode = (g_eMMCDrv[emmc_ip].u16_Reg10_Mode | g_eMMCDrv[emmc_ip].u8_BUS_WIDTH | (e_TransType >> 8));
    u16_ctrl = (BIT_SD_CMD_EN | (e_RspType >> 12) | ((U8)e_TransType & BIT_ADMA_EN) | BIT_ERR_DET_ON);

    if (e_TransType == EMMC_DMA)
    {
        if (u8_CmdIdx == 18)
            u16_mode |= BIT_SD_DMA_R_CLK_STOP;
        else
            u16_mode &= ~BIT_SD_DMA_R_CLK_STOP;
    }

    if (e_CmdType == eMMC_CMD_READ)
    {
        u16_ctrl |= BIT_SD_DTRX_EN;
        if (e_TransType == EMMC_CIF)
        {
            REG_FCIE_SETBIT(FCIE_DDR_MODE(emmc_ip), BIT1 | BIT2 | BIT3);
        }
    }

    eMMC_FCIE_ClearEvents(emmc_ip);

    REG_FCIE_CLRBIT(FCIE_CMD_RSP_SIZE(emmc_ip), BIT_RSP_SIZE_MASK);
    REG_FCIE_SETBIT(FCIE_CMD_RSP_SIZE(emmc_ip), ((U8)e_RspType) & BIT_RSP_SIZE_MASK);
    REG_FCIE_W(FCIE_SD_MODE(emmc_ip), u16_mode);

    if (12 != u8_CmdIdx)
    {
        u32_err = eMMC_FCIE_WaitD0High(emmc_ip, TIME_WAIT_DAT0_HIGH);
        if (eMMC_ST_SUCCESS != u32_err)
            goto LABEL_SEND_CMD_ERROR;
    }

#if defined(ENABLE_eMMC_INTERRUPT_MODE) && ENABLE_eMMC_INTERRUPT_MODE
    eMMC_INT_ClearMIEEvent(emmc_ip);
    REG_FCIE_W(FCIE_MIE_INT_EN(emmc_ip), BIT_SD_CMD_END | BIT_ERR_STS);
#endif

    REG_FCIE_W(FCIE_SD_CTRL(emmc_ip), u16_ctrl);
    REG_FCIE_W(FCIE_SD_CTRL(emmc_ip), u16_ctrl | BIT_JOB_START);

    // wait event
    u32_err = eMMC_FCIE_WaitEvents(emmc_ip, FCIE_MIE_EVENT(emmc_ip), BIT_SD_CMD_END, TIME_WAIT_CMDRSP_END);
    if (!u32_err && (e_CmdType == eMMC_CMD_READ) && (e_TransType != EMMC_CIF))
    {
#if defined(ENABLE_eMMC_INTERRUPT_MODE) && ENABLE_eMMC_INTERRUPT_MODE
        REG_FCIE_W(FCIE_MIE_INT_EN(emmc_ip), BIT_DMA_END | BIT_ERR_STS);
#endif
        u32_err |= eMMC_FCIE_WaitEvents(emmc_ip, FCIE_MIE_EVENT(emmc_ip), BIT_DMA_END, TIME_WAIT_n_BLK_END);
    }

    if (eMMC_ST_SUCCESS != u32_err)
    {
        if (u8_retry_cmd < eMMC_FCIE_CMD_RSP_ERR_RETRY_CNT && 0 == eMMC_IF_TUNING_TTABLE(emmc_ip)
            && eMMC_Error_Retry(emmc_ip))
        {
            u8_retry_cmd++;
            eMMC_debug(eMMC_DEBUG_LEVEL_WARNING, 1, "eMMC_%d cmd[%d] WARN: wait event error:%X \n", emmc_ip, u8_CmdIdx,
                       u32_err);

            goto SEND_CMD_RETRY;
        }
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC_%d cmd[%d] ERR: wait event error:%X \n", emmc_ip, u8_CmdIdx,
                   u32_err);

        if (g_eMMCDrv[emmc_ip].u8_make_sts_err == FCIE_MAKE_WR_TOUT_ERR
            || g_eMMCDrv[emmc_ip].u8_make_sts_err == FCIE_MAKE_RD_TOUT_ERR)
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC_%d cmd[%d] ERR: check SD_STS:%X \n", emmc_ip, u8_CmdIdx,
                       REG_FCIE(FCIE_SD_STATUS(emmc_ip)));

        goto LABEL_SEND_CMD_ERROR;
    }

    if (g_eMMCDrv[emmc_ip].u32_DrvFlag & DRV_FLAG_RSP_WAIT_D0H)
    {
        u32_err = eMMC_FCIE_WaitD0High(emmc_ip, TIME_WAIT_DAT0_HIGH);
        if (eMMC_ST_SUCCESS != u32_err)
            goto LABEL_SEND_CMD_ERROR;
    }

    if ((e_CmdType == eMMC_CMD_WRITE) && (!eMMC_FCIE_CHECK_STSERR(emmc_ip))) // read or write
    {
        if (e_TransType != EMMC_CIF)
        {
            eMMC_FCIE_JobStartAndWaitEvent(emmc_ip, e_TransType, e_CmdType, BIT_DMA_END, TIME_WAIT_n_BLK_END);
        }
    }

    if (e_TransType == EMMC_CIF)
    {
        if (e_CmdType == eMMC_CMD_WRITE)
        {
            REG_FCIE_W(FCIE_SD_CTRL(emmc_ip), BIT_SD_DAT_EN | BIT_SD_DAT_DIR_W);
            REG_FCIE_W(FCIE_SD_CTRL(emmc_ip), BIT_SD_DAT_EN | BIT_SD_DAT_DIR_W | BIT_JOB_START);
        }

        u32_err = eMMC_FCIE_CIFD_WaitProcess(emmc_ip, e_CmdType, gpu8CifdBuf);
        if (eMMC_ST_SUCCESS == u32_err)
            u32_err = eMMC_FCIE_WaitEvents(emmc_ip, FCIE_MIE_EVENT(emmc_ip), BIT_DMA_END, TIME_WAIT_1_BLK_END);

        if (eMMC_ST_SUCCESS != u32_err)
        {
            if (u8_retry_cmd < eMMC_FCIE_CMD_RSP_ERR_RETRY_CNT && 0 == eMMC_IF_TUNING_TTABLE(emmc_ip)
                && eMMC_Error_Retry(emmc_ip))
            {
                u8_retry_cmd++;
                eMMC_debug(eMMC_DEBUG_LEVEL_WARNING, 1, "eMMC_%d cmd[%d] args(0x%x) WARN: wait event error:%X \n",
                           emmc_ip, gu32Arg, u8_CmdIdx, u32_err);
                goto SEND_CMD_RETRY;
            }
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC_%d cmd[%d] args(0x%x) ERR: wait event error:%X \n", emmc_ip,
                       u8_CmdIdx, gu32Arg, u32_err);
            goto LABEL_SEND_CMD_ERROR;
        }
    }

    // check status
    if (e_RspType == eMMC_R3) // R3 no need crc
        REG_FCIE_W(FCIE_SD_STATUS(emmc_ip), BIT_SD_RSP_CRC_ERR);

    REG_FCIE_R(FCIE_SD_STATUS(emmc_ip), u16_reg);
    if (u16_reg & BIT_SD_FCIE_ERR_FLAGS)
    {
        if (u8_retry_fcie < eMMC_CMD_API_ERR_RETRY_CNT && 0 == eMMC_IF_TUNING_TTABLE(emmc_ip)
            && eMMC_Error_Retry(emmc_ip))
        {
            u8_retry_fcie++;
            eMMC_debug(eMMC_DEBUG_LEVEL_WARNING, 1, "eMMC_%d cmd[%d] args(0x%x) WARN: check SD_STS:%X \n", emmc_ip,
                       u8_CmdIdx, gu32Arg, u16_reg);
            goto SEND_CMD_RETRY;
        }
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC_%d cmd[%d] args(0x%x) ERR: check SD_STS:%X \n", emmc_ip, u8_CmdIdx,
                   gu32Arg, u16_reg);
        u32_err = u16_reg;
        goto LABEL_SEND_CMD_ERROR;
    }

    // checkR1Error
    if ((e_RspType == eMMC_R1) || (e_RspType == eMMC_R1B))
    {
        u32_err = eMMC_CheckR1Error(emmc_ip);
        if (eMMC_ST_SUCCESS != u32_err)
        {
            if (u8_retry_r1 < eMMC_CMD_API_ERR_RETRY_CNT && 0 == eMMC_IF_TUNING_TTABLE(emmc_ip)
                && eMMC_Error_Retry(emmc_ip))
            {
                u8_retry_r1++;
                eMMC_debug(eMMC_DEBUG_LEVEL_WARNING, 1, "eMMC_%d cmd[%d] args(0x%x) WARN: check R1 error: %X \n",
                           emmc_ip, u8_CmdIdx, gu32Arg, u32_err);
                goto SEND_CMD_RETRY;
            }
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC_%d cmd[%d] args(0x%x) ERR: check R1 error:%X \n", emmc_ip,
                       u8_CmdIdx, gu32Arg, u32_err);
            goto LABEL_SEND_CMD_ERROR;
        }
    }

LABEL_SEND_CMD_ERROR:
    if (u32_err)
    {
        eMMC_FCIE_ErrHandler_Stop(emmc_ip);
    }

    REG_FCIE_CLRBIT(FCIE_DDR_MODE(emmc_ip), BIT1 | BIT2 | BIT3); // CIFD read

    return u32_err;

SEND_CMD_RETRY:
    if (e_CmdType != eMMC_CMD_RSP)
        eMMC_FCIE_ErrHandler_ReInit(emmc_ip);

    eMMC_FCIE_ErrHandler_Retry(emmc_ip);
    goto LABEL_SEND_CMD;
}

void eMMC_FCIE_GetCMDFIFO(eMMC_IP_EmType emmc_ip, U16 u16_WordPos, U16 u16_WordCnt, U16 *pu16_Buf)
{
    U16 u16_i;

    for (u16_i = 0; u16_i < u16_WordCnt; u16_i++)
        REG_FCIE_R(FCIE_CMDFIFO_ADDR(emmc_ip, u16_i), pu16_Buf[u16_i]);
}

U32 eMMC_WaitCIFD_Event(eMMC_IP_EmType emmc_ip, U16 u16_WaitEvent, U32 u32_MicroSec)
{
    volatile U32 u32_Count;
    volatile U16 u16_Reg;

    for (u32_Count = 0; u32_Count < u32_MicroSec; u32_Count++)
    {
        REG_FCIE_R(NC_CIFD_EVENT(emmc_ip), u16_Reg);
        if ((u16_Reg & u16_WaitEvent) == u16_WaitEvent)
            break;

        eMMC_hw_timer_delay(HW_TIMER_DELAY_1us);
    }
    if (u32_Count >= u32_MicroSec)
    {
        REG_FCIE_R(NC_CIFD_EVENT(emmc_ip), u16_Reg);
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC%d CIFD Event Timout %X\n", emmc_ip, u16_Reg);
    }
    return u32_Count;
}

U32 eMMC_WaitSetCIFD(eMMC_IP_EmType emmc_ip, U8 *pu8_DataBuf, U32 u32_ByteCnt)
{
    U16 u16_i, *pu16_Data = (U16 *)pu8_DataBuf;

    if (u32_ByteCnt > FCIE_CIFD_BYTE_CNT)
    {
        return eMMC_ST_ERR_INVALID_PARAM;
    }

    for (u16_i = 0; u16_i < (u32_ByteCnt >> 1); u16_i++)
        REG_FCIE_W(NC_RBUF_CIFD_ADDR(emmc_ip, u16_i), pu16_Data[u16_i]);

    REG_FCIE_SETBIT(NC_CIFD_EVENT(emmc_ip), BIT_RBUF_FULL_TRI);

    if (eMMC_WaitCIFD_Event(emmc_ip, BIT_RBUF_EMPTY, HW_TIMER_DELAY_500ms) == (HW_TIMER_DELAY_500ms))
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC%d Error: CIFD timeout 0, ErrCode:%Xh\r\n", emmc_ip,
                   eMMC_ST_ERR_TIMEOUT_WAITCIFDEVENT);
        return eMMC_ST_ERR_TIMEOUT_WAITCIFDEVENT; // timeout
    }

    REG_FCIE_SETBIT(NC_CIFD_EVENT(emmc_ip), BIT_RBUF_EMPTY);

    return eMMC_ST_SUCCESS;
}

void eMMC_FCIE_GetCIFD(eMMC_IP_EmType emmc_ip, U16 u16_WordPos, U16 u16_WordCnt, U16 *pu16_Buf)
{
    U16 u16_i;

    for (u16_i = 0; u16_i < u16_WordCnt; u16_i++)
        REG_FCIE_R(FCIE_CIFD_ADDR(emmc_ip, u16_i), pu16_Buf[u16_i]);
}

U32 eMMC_WaitGetCIFD(eMMC_IP_EmType emmc_ip, U8 *pu8_DataBuf, U32 u32_ByteCnt)
{
    U16 u16_i, *pu16_Data = (U16 *)pu8_DataBuf;

    if (u32_ByteCnt > FCIE_CIFD_BYTE_CNT)
    {
        return eMMC_ST_ERR_INVALID_PARAM;
    }
    if (eMMC_WaitCIFD_Event(emmc_ip, BIT_WBUF_FULL, HW_TIMER_DELAY_500ms) == (HW_TIMER_DELAY_500ms))
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC%d Error: CIFD timeout 0, ErrCode:%Xh\r\n", emmc_ip,
                   eMMC_ST_ERR_TIMEOUT_WAITCIFDEVENT);
        return eMMC_ST_ERR_TIMEOUT_WAITCIFDEVENT; // timeout
    }

    for (u16_i = 0; u16_i < (u32_ByteCnt >> 1); u16_i++)
        REG_FCIE_R(NC_WBUF_CIFD_ADDR(emmc_ip, u16_i), pu16_Data[u16_i]);

    REG_FCIE_W(NC_CIFD_EVENT(emmc_ip), BIT_WBUF_FULL);

    REG_FCIE_W(NC_CIFD_EVENT(emmc_ip), BIT_WBUF_EMPTY_TRI);

    return eMMC_ST_SUCCESS;
}

U8 eMMC_FCIE_DataFifoGet(U8 emmc_id, U8 u8addr)
{
    U16 u16Tmp;

    u16Tmp = REG_FCIE(FCIE_CIFD_ADDR(emmc_id, u8addr >> 1));

    if (u8addr & 0x1)
        return ((u16Tmp >> 8) & 0xFF);
    else
        return (u16Tmp & 0xFF);
}

U32 eMMC_FCIE_CIFD_WaitProcess(eMMC_IP_EmType emmc_ip, eMMC_Cmd_EmType e_CmdType, volatile U8 *u8Buf)
{
    U8  u8RegionNo = 0, u8RegionMax = 0, u8RemainByte = 0;
    U32 u32_err;
    U32 u32_TranLen = REG_FCIE(FCIE_MIU_DMA_LEN_15_0(emmc_ip)) + (REG_FCIE(FCIE_MIU_DMA_LEN_31_16(emmc_ip)) << 16);

    u8RemainByte = u32_TranLen & (64 - 1); // u32_TranLen % 64
    u8RegionMax  = (u32_TranLen >> 6) + (u8RemainByte ? 1 : 0);

    for (u8RegionNo = 0; u8RegionNo < u8RegionMax; u8RegionNo++)
    {
        if (e_CmdType == eMMC_CMD_READ)
        {
            if ((u8RegionNo == (u8RegionMax - 1)) && (u8RemainByte > 0))
                u32_err = eMMC_WaitGetCIFD(emmc_ip, (U8 *)(u8Buf + (u8RegionNo << 6)), u8RemainByte);
            else
                u32_err = eMMC_WaitGetCIFD(emmc_ip, (U8 *)(u8Buf + (u8RegionNo << 6)), 0x40);
        }
        else // Write
        {
            if ((u8RegionNo == (u8RegionMax - 1)) && (u8RemainByte > 0))
                u32_err = eMMC_WaitSetCIFD(emmc_ip, (U8 *)(u8Buf + (u8RegionNo << 6)), u8RemainByte);
            else
                u32_err = eMMC_WaitSetCIFD(emmc_ip, (U8 *)(u8Buf + (u8RegionNo << 6)), 0x40);
        }

        if (u32_err)
            break;
    }

    return u32_err;
}

U32 eMMC_CheckR1Error(eMMC_IP_EmType emmc_ip)
{
    U32 u32_err = eMMC_ST_SUCCESS;

    eMMC_FCIE_GetCMDFIFO(emmc_ip, 0, 3, (U16 *)g_eMMCDrv[emmc_ip].au8_Rsp);

    if (g_eMMCDrv[emmc_ip].au8_Rsp[1] & (eMMC_ERR_R1_31_24 >> 24))
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: eMMC_ST_ERR_R1_31_24 \n ");
        u32_err = eMMC_ST_ERR_R1_31_24;
        goto LABEL_CHECK_R1_END;
    }

    if (g_eMMCDrv[emmc_ip].au8_Rsp[2] & (eMMC_ERR_R1_23_16 >> 16))
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: eMMC_ST_ERR_R1_23_16 \n ");
        u32_err = eMMC_ST_ERR_R1_23_16;
        goto LABEL_CHECK_R1_END;
    }

    if (g_eMMCDrv[emmc_ip].au8_Rsp[3] & (eMMC_ERR_R1_15_8 >> 8))
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: eMMC_ST_ERR_R1_15_8 \n ");
        u32_err = eMMC_ST_ERR_R1_15_8;
        goto LABEL_CHECK_R1_END;
    }

    if (g_eMMCDrv[emmc_ip].au8_Rsp[4] & (eMMC_ERR_R1_7_0 >> 0))
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: eMMC_ST_ERR_R1_7_0 \n ");
        u32_err = eMMC_ST_ERR_R1_7_0;
        goto LABEL_CHECK_R1_END;
    }

LABEL_CHECK_R1_END:

    /*u8_cs = (eMMC_FCIE_CmdRspBufGet(emmc_id, 3) & (eMMC_R1_CURRENT_STATE>>8))>>1;
    eMMC_debug(0,0,"card state: %d ", u8_cs);
    switch(u8_cs) {
        case 0:     eMMC_debug(0,0,"(idle)\n"); break;
        case 1:     eMMC_debug(0,0,"(ready)\n");    break;
        case 2:     eMMC_debug(0,0,"(ident)\n");    break;
        case 3:     eMMC_debug(0,0,"(stby)\n"); break;
        case 4:     eMMC_debug(0,0,"(tran)\n"); break;
        case 5:     eMMC_debug(0,0,"(data)\n"); break;
        case 6:     eMMC_debug(0,0,"(rcv)\n");  break;
        case 7:     eMMC_debug(0,0,"(prg)\n");  break;
        case 8:     eMMC_debug(0,0,"(dis)\n");  break;
        default:    eMMC_debug(0,0,"(?)\n");    break;
    }*/

    if (eMMC_ST_SUCCESS != u32_err) // && 0==eMMC_IF_TUNING_TTABLE(emmc_ip))
    {
        eMMC_dump_mem(g_eMMCDrv[emmc_ip].au8_Rsp, eMMC_R1_BYTE_CNT);
    }
    return u32_err;
}
void eMMC_PreparePowerSavingModeQueue(eMMC_IP_EmType emmc_ip)
{
    U32 u32_Count;
    U16 u16_Reg;

    REG_FCIE_CLRBIT(FCIE_PWR_SAVE_CTL(emmc_ip), BIT_BAT_SD_POWER_SAVE_MASK);

    /* (1) Clear HW Enable */
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x00), 0x0000);
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x01),
               PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x07);

    /* (2) Clear All Interrupt */
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x02), 0xffff);
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x03),
               PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x00);

    /* (3) Clear SD MODE Enable */
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x04), 0x0000);
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x05),
               PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x0B);

    /* (4) Clear SD CTL Enable */
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x06), 0x0000);
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x07),
               PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x0C);

    /* (5) Reset Start */
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x08), 0x0000);
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x09),
               PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x3F);

    /* (6) Reset End */
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x0A), 0x0001);
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x0B),
               PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x3F);

    /* (7) Set "SD_MOD" */
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x0C), 0x0021);
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x0D),
               PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x0B);

    /* (8) Enable "reg_sd_en" */
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x0E), 0x0001);
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x0F),
               PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x07);

    /* (9) Command Content, IDLE */
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x10), 0x0040);
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x11),
               PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x20);

    /* (10) Command Content, STOP */
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x12), 0x0000);
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x13),
               PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x21);

    /* (11) Command Content, STOP */
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x14), 0x0000);
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x15),
               PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x22);

    /* (12) Command & Response Size */
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x16), 0x0500);
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x17),
               PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x0A);

    /* (13) Enable Interrupt, SD_CMD_END */
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x18), 0x0002);
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x19),
               PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x01);

    /* (14) Command Enable + job Start */
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x1A), 0x0044);
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x1B),
               PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x0C);

    /* (15) Wait Interrupt */
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x1C), 0x0000);
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x1D), PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WINT);

    /* (16) STOP */
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x1E), 0x0000);
    REG_FCIE_W(GET_REG_ADDR(FCIE_POWEER_SAVE_MODE_BASE(emmc_ip), 0x1F), PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_STOP);

    REG_FCIE_CLRBIT(FCIE_PWR_SAVE_CTL(emmc_ip), BIT_SD_POWER_SAVE_RST);
    REG_FCIE_SETBIT(FCIE_PWR_SAVE_CTL(emmc_ip), BIT_SD_POWER_SAVE_RST);

    REG_FCIE_SETBIT(FCIE_PWR_SAVE_CTL(emmc_ip), BIT_POWER_SAVE_MODE);

    /* Step 4: Before Nand IP DMA end, use RIU interface to test power save function */
    REG_FCIE_W(FCIE_PWR_SAVE_CTL(emmc_ip),
               BIT_POWER_SAVE_MODE | BIT_SD_POWER_SAVE_RIU | BIT_POWER_SAVE_MODE_INT_EN | BIT_SD_POWER_SAVE_RST);

    for (u32_Count = 0; u32_Count < HW_TIMER_DELAY_1s; u32_Count++)
    {
        REG_FCIE_R(FCIE_PWR_SAVE_CTL(emmc_ip), u16_Reg);
        if ((u16_Reg & BIT_POWER_SAVE_MODE_INT) == BIT_POWER_SAVE_MODE_INT)
        {
            eMMC_debug(0, 1, "eMMC_PWRSAVE_CTL: 0x%x\r\n", REG_FCIE(FCIE_PWR_SAVE_CTL(emmc_ip)));
            break;
        }
        eMMC_hw_timer_delay(HW_TIMER_DELAY_1us);
    }
    REG_FCIE_R(FCIE_PWR_SAVE_CTL(emmc_ip), u16_Reg);
    eMMC_debug(0, 1, "eMMC_PWRSAVE_CTL: 0x%x\r\n", u16_Reg);
}
#endif // UNIFIED_eMMC_DRIVER*/
