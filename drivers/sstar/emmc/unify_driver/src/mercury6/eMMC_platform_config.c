/*
 * eMMC_platform_config.c- Sigmastar
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

#include "eMMC_linux.h"
#include "eMMC.h"
#include "eMMC_hal.h"
#include "eMMC_platform.h"
#include "eMMC_err_codes.h"

#define REG_BANK_FCIE0_IP_0 GET_REG_ADDR(RIU_BASE, 0x20980) // Bank: 0x1413
#define REG_BANK_FCIE1_IP_0 GET_REG_ADDR(RIU_BASE, 0x20A00) // Bank: 0x1414
#define REG_BANK_FCIE2_IP_0 GET_REG_ADDR(RIU_BASE, 0x20A80) // Bank: 0x1415

#define REG_BANK_FCIE0_IP_1 GET_REG_ADDR(RIU_BASE, 0x20800) // Bank: 0x1410
#define REG_BANK_FCIE1_IP_1 GET_REG_ADDR(RIU_BASE, 0x20880) // Bank: 0x1411
#define REG_BANK_FCIE2_IP_1 GET_REG_ADDR(RIU_BASE, 0x20900) // Bank: 0x1412

#define REG_BANK_FCIE0_IP_2 GET_REG_ADDR(RIU_BASE, 0x20B00) // Bank: 0x1416
#define REG_BANK_FCIE1_IP_2 GET_REG_ADDR(RIU_BASE, 0x20B80) // Bank: 0x1417
#define REG_BANK_FCIE2_IP_2 GET_REG_ADDR(RIU_BASE, 0x20C00) // Bank: 0x1418

#define REG_BANK_FCIE0_IP_3 GET_REG_ADDR(RIU_BASE, 0x20D00) // Bank: 0x141A
#define REG_BANK_FCIE1_IP_3 GET_REG_ADDR(RIU_BASE, 0x20D80) // Bank: 0x141B
#define REG_BANK_FCIE2_IP_3 GET_REG_ADDR(RIU_BASE, 0x20E00) // Bank: 0x141C

#define REG_BANK_SDPLL GET_REG_ADDR(RIU_BASE, 0x20C80) // Bank: 0x1419

//---------------------------------------------------------------------------------
#define REG_CLK_SOURCE_IP0 GET_REG_ADDR(CLKGEN0_BASE, 0x43)
#define REG_CLK_SOURCE_IP1 GET_REG_ADDR(CLKGEN0_BASE, 0x45)
#define REG_CLK_SOURCE_IP2 GET_REG_ADDR(CLKGEN2_BASE, 0x24)
#define REG_CLK_SOURCE_IP3 GET_REG_ADDR(CLKGEN2_BASE, 0x25)

//--------------------------------sc_gp_ctrl----------------------------------------
#define REG_BANK_SC_GP_CTRL (0x1133 - 0x1000) * 0x80 // sc_gp_ctrl BK:x1133 reg block
#define SC_GP_CTRL_BASE     GET_REG_ADDR(RIU_BASE, REG_BANK_SC_GP_CTRL)
#define reg_sc_gp_ctrl      GET_REG_ADDR(SC_GP_CTRL_BASE, 0x25)

#define BIT_CKG_SD_VALUE_IP_0 (BIT7)
#define BIT_CKG_SD_VALUE_IP_1 (BIT3)
#define BIT_CKG_SD_VALUE_IP_2 (BIT4)
#define BIT_CKG_SD_VALUE_IP_3 (BIT5)

//---------------------------------------------------------------------------------
#define CLK1_48M  48000000
#define CLK1_43M2 43200000
#define CLK1_40M  40000000
#define CLK1_36M  36000000
#define CLK1_32M  32000000
#define CLK1_20M  20000000
#define CLK1_12M  12000000
#define CLK1_300K 300000
#define CLK1_0    0

extern U32 gu32_clk_driving[EMMC_NUM_TOTAL];
extern U32 gu32_cmd_driving[EMMC_NUM_TOTAL];
extern U32 gu32_data_driving[EMMC_NUM_TOTAL];

volatile U32 EMMC_GET_REG_BANK(U8 eIP, U8 u8Bank)
{
    U32 pIPBANKArr[EMMC_NUM_TOTAL][3] = {
        {(REG_BANK_FCIE0_IP_0), (REG_BANK_FCIE1_IP_0), (REG_BANK_FCIE2_IP_0)},
        {(REG_BANK_FCIE0_IP_1), (REG_BANK_FCIE1_IP_1), (REG_BANK_FCIE2_IP_1)},
    };

    return pIPBANKArr[eIP][u8Bank];
}

volatile U32 EMMC_GET_CLK_REG(U8 eIP)
{
    U32 IPClkRegArr[EMMC_NUM_TOTAL] = {REG_CLK_SOURCE_IP0, REG_CLK_SOURCE_IP1};
    return IPClkRegArr[eIP];
}

volatile U32 EMMC_GET_PLL_REG(U8 eIP)
{
    U32 IPPLLRegArr[EMMC_NUM_TOTAL] = {REG_BANK_SDPLL, REG_BANK_SDPLL, REG_BANK_SDPLL};
    return IPPLLRegArr[eIP];
}

volatile U16 EMMC_GET_BOOT_CLK(U8 eIP)
{
    U32 IPClkRegArr[EMMC_NUM_TOTAL] = {BIT_CKG_SD_VALUE_IP_0, BIT_CKG_SD_VALUE_IP_1};
    return IPClkRegArr[eIP];
}

volatile U16 IP_BIT_FUNC_ENABLE(U8 eIP)
{
    U16 IPFuncEnableArr[EMMC_NUM_TOTAL] = {BIT_SDIO_MOD, BIT_SDIO_MOD, BIT_SDIO_MOD};
    return IPFuncEnableArr[eIP];
}

U16 eMMC_Find_Clock_Reg(eMMC_IP_EmType emmc_ip, U32 u32_Clk)
{
    U8  u8LV                          = 0;
    U16 u16_ClkParam                  = BIT_FCIE_CLK_300K;
    U32 u32RealClk                    = 0;
    U32 u32ClkArr[EMMC_NUM_TOTAL][16] = {{CLK1_48M, CLK1_43M2, CLK1_40M, CLK1_36M, CLK1_32M, CLK1_20M, CLK1_12M,
                                          CLK1_300K, CLK1_0, CLK1_0, CLK1_0, CLK1_0, CLK1_0, CLK1_0, CLK1_0, CLK1_0},
                                         {CLK1_48M, CLK1_43M2, CLK1_40M, CLK1_36M, CLK1_32M, CLK1_20M, CLK1_12M,
                                          CLK1_300K, CLK1_0, CLK1_0, CLK1_0, CLK1_0, CLK1_0, CLK1_0, CLK1_0, CLK1_0}};

    for (; u8LV < 16; u8LV++)
    {
        if ((u32_Clk >= u32ClkArr[emmc_ip][u8LV]) || (u8LV == 15) || (u32ClkArr[emmc_ip][u8LV + 1] == 0))
        {
            u32RealClk = u32ClkArr[emmc_ip][u8LV];
            break;
        }
    }

    switch (u32RealClk)
    {
        case CLK1_0:
        case CLK1_300K:
            u16_ClkParam = BIT_FCIE_CLK_300K;
            break;
        case CLK1_12M:
            u16_ClkParam = BIT_FCIE_CLK_12M;
            break;
        case CLK1_20M:
            u16_ClkParam = BIT_FCIE_CLK_20M;
            break;
        case CLK1_32M:
            u16_ClkParam = BIT_FCIE_CLK_32M;
            break;
        case CLK1_36M:
            u16_ClkParam = BIT_FCIE_CLK_36M;
            break;
        case CLK1_40M:
            u16_ClkParam = BIT_FCIE_CLK_40M;
            break;
        case CLK1_43M2:
            u16_ClkParam = BIT_FCIE_CLK_43_2M;
            break;
        case CLK1_48M:
            u16_ClkParam = BIT_FCIE_CLK_48M;
            break;
    }

    return u16_ClkParam;
}

U32 _eMMC_pll_setting(eMMC_IP_EmType emmc_ip, U16 u16_ClkParam)
{
    U32 u32_value_reg_emmc_pll_pdiv;

    // 1. reset emmc pll
    REG_FCIE_SETBIT(reg_emmc_pll_reset(emmc_ip), BIT0);
    REG_FCIE_CLRBIT(reg_emmc_pll_reset(emmc_ip), BIT0);

    // 2. synth clock
    switch (u16_ClkParam)
    {
        case eMMC_PLL_CLK_200M: // 200M
            REG_FCIE_CLRBIT(reg_ddfset_23_16(emmc_ip), 0xffff);
            REG_FCIE_CLRBIT(reg_ddfset_15_00(emmc_ip), 0xffff);
            REG_FCIE_SETBIT(reg_ddfset_23_16(emmc_ip), 0x24); // 195MHz
            REG_FCIE_SETBIT(reg_ddfset_15_00(emmc_ip), 0x03D8);
            u32_value_reg_emmc_pll_pdiv = 1; // PostDIV: 2
            break;

        case eMMC_PLL_CLK_160M:
            REG_FCIE_CLRBIT(reg_ddfset_23_16(emmc_ip), 0xffff);
            REG_FCIE_CLRBIT(reg_ddfset_15_00(emmc_ip), 0xffff);
            REG_FCIE_SETBIT(reg_ddfset_23_16(emmc_ip), 0x2B);
            REG_FCIE_SETBIT(reg_ddfset_15_00(emmc_ip), 0x3333);
            u32_value_reg_emmc_pll_pdiv = 1; // PostDIV: 2
            break;

        case eMMC_PLL_CLK_140M:
            REG_FCIE_CLRBIT(reg_ddfset_23_16(emmc_ip), 0xffff);
            REG_FCIE_CLRBIT(reg_ddfset_15_00(emmc_ip), 0xffff);
            REG_FCIE_SETBIT(reg_ddfset_23_16(emmc_ip), 0x31);
            REG_FCIE_SETBIT(reg_ddfset_15_00(emmc_ip), 0x5F15);
            u32_value_reg_emmc_pll_pdiv = 1; // PostDIV: 2
            break;

        case eMMC_PLL_CLK_120M:
            REG_FCIE_CLRBIT(reg_ddfset_23_16(emmc_ip), 0xffff);
            REG_FCIE_CLRBIT(reg_ddfset_15_00(emmc_ip), 0xffff);
            REG_FCIE_SETBIT(reg_ddfset_23_16(emmc_ip), 0x39);
            REG_FCIE_SETBIT(reg_ddfset_15_00(emmc_ip), 0x9999);
            u32_value_reg_emmc_pll_pdiv = 1; // PostDIV: 2
            break;

        case eMMC_PLL_CLK_100M:
            REG_FCIE_CLRBIT(reg_ddfset_23_16(emmc_ip), 0xffff);
            REG_FCIE_CLRBIT(reg_ddfset_15_00(emmc_ip), 0xffff);
            REG_FCIE_SETBIT(reg_ddfset_23_16(emmc_ip), 0x45);
            REG_FCIE_SETBIT(reg_ddfset_15_00(emmc_ip), 0x1EB8);
            u32_value_reg_emmc_pll_pdiv = 1; // PostDIV: 2
            break;

        case eMMC_PLL_CLK__86M:
            REG_FCIE_CLRBIT(reg_ddfset_23_16(emmc_ip), 0xffff);
            REG_FCIE_CLRBIT(reg_ddfset_15_00(emmc_ip), 0xffff);
            REG_FCIE_SETBIT(reg_ddfset_23_16(emmc_ip), 0x28);
            REG_FCIE_SETBIT(reg_ddfset_15_00(emmc_ip), 0x2FA0);
            u32_value_reg_emmc_pll_pdiv = 2; // PostDIV: 4
            break;

        case eMMC_PLL_CLK__80M:
            REG_FCIE_CLRBIT(reg_ddfset_23_16(emmc_ip), 0xffff);
            REG_FCIE_CLRBIT(reg_ddfset_15_00(emmc_ip), 0xffff);
            REG_FCIE_SETBIT(reg_ddfset_23_16(emmc_ip), 0x2B);
            REG_FCIE_SETBIT(reg_ddfset_15_00(emmc_ip), 0x3333);
            u32_value_reg_emmc_pll_pdiv = 2; // PostDIV: 4
            break;

        case eMMC_PLL_CLK__72M:
            REG_FCIE_CLRBIT(reg_ddfset_23_16(emmc_ip), 0xffff);
            REG_FCIE_CLRBIT(reg_ddfset_15_00(emmc_ip), 0xffff);
            REG_FCIE_SETBIT(reg_ddfset_23_16(emmc_ip), 0x30);
            REG_FCIE_SETBIT(reg_ddfset_15_00(emmc_ip), 0x0000);
            u32_value_reg_emmc_pll_pdiv = 2; // PostDIV: 4
            break;

        case eMMC_PLL_CLK__62M:
            REG_FCIE_CLRBIT(reg_ddfset_23_16(emmc_ip), 0xffff);
            REG_FCIE_CLRBIT(reg_ddfset_15_00(emmc_ip), 0xffff);
            REG_FCIE_SETBIT(reg_ddfset_23_16(emmc_ip), 0x37);
            REG_FCIE_SETBIT(reg_ddfset_15_00(emmc_ip), 0xBDEF);
            u32_value_reg_emmc_pll_pdiv = 2; // PostDIV: 4
            break;

        case eMMC_PLL_CLK__52M:
            REG_FCIE_CLRBIT(reg_ddfset_23_16(emmc_ip), 0xffff);
            REG_FCIE_CLRBIT(reg_ddfset_15_00(emmc_ip), 0xffff);
            REG_FCIE_SETBIT(reg_ddfset_23_16(emmc_ip), 0x42);
            REG_FCIE_SETBIT(reg_ddfset_15_00(emmc_ip), 0x7627);
            u32_value_reg_emmc_pll_pdiv = 2; // PostDIV: 4
            break;

        case eMMC_PLL_CLK__48M:
            REG_FCIE_CLRBIT(reg_ddfset_23_16(emmc_ip), 0xffff);
            REG_FCIE_CLRBIT(reg_ddfset_15_00(emmc_ip), 0xffff);
            REG_FCIE_SETBIT(reg_ddfset_23_16(emmc_ip), 0x48);
            REG_FCIE_SETBIT(reg_ddfset_15_00(emmc_ip), 0x0000);
            u32_value_reg_emmc_pll_pdiv = 2; // PostDIV: 4
            break;

        case eMMC_PLL_CLK__40M:
            REG_FCIE_CLRBIT(reg_ddfset_23_16(emmc_ip), 0xffff);
            REG_FCIE_CLRBIT(reg_ddfset_15_00(emmc_ip), 0xffff);
            REG_FCIE_SETBIT(reg_ddfset_23_16(emmc_ip), 0x2B);
            REG_FCIE_SETBIT(reg_ddfset_15_00(emmc_ip), 0x3333);
            u32_value_reg_emmc_pll_pdiv = 4; // PostDIV: 8
            break;

        case eMMC_PLL_CLK__36M:
            REG_FCIE_CLRBIT(reg_ddfset_23_16(emmc_ip), 0xffff);
            REG_FCIE_CLRBIT(reg_ddfset_15_00(emmc_ip), 0xffff);
            REG_FCIE_SETBIT(reg_ddfset_23_16(emmc_ip), 0x30);
            REG_FCIE_SETBIT(reg_ddfset_15_00(emmc_ip), 0x0000);
            u32_value_reg_emmc_pll_pdiv = 4; // PostDIV: 8
            break;

        case eMMC_PLL_CLK__32M:
            REG_FCIE_CLRBIT(reg_ddfset_23_16(emmc_ip), 0xffff);
            REG_FCIE_CLRBIT(reg_ddfset_15_00(emmc_ip), 0xffff);
            REG_FCIE_SETBIT(reg_ddfset_23_16(emmc_ip), 0x36);
            REG_FCIE_SETBIT(reg_ddfset_15_00(emmc_ip), 0x0000);
            u32_value_reg_emmc_pll_pdiv = 4; // PostDIV: 8
            break;

        case eMMC_PLL_CLK__27M:
            REG_FCIE_CLRBIT(reg_ddfset_23_16(emmc_ip), 0xffff);
            REG_FCIE_CLRBIT(reg_ddfset_15_00(emmc_ip), 0xffff);
            REG_FCIE_SETBIT(reg_ddfset_23_16(emmc_ip), 0x40);
            REG_FCIE_SETBIT(reg_ddfset_15_00(emmc_ip), 0x0000);
            u32_value_reg_emmc_pll_pdiv = 4; // PostDIV: 8
            break;

        case eMMC_PLL_CLK__20M:
            REG_FCIE_CLRBIT(reg_ddfset_23_16(emmc_ip), 0xffff);
            REG_FCIE_CLRBIT(reg_ddfset_15_00(emmc_ip), 0xffff);
            REG_FCIE_SETBIT(reg_ddfset_23_16(emmc_ip), 0x2B);
            REG_FCIE_SETBIT(reg_ddfset_15_00(emmc_ip), 0x3333);
            u32_value_reg_emmc_pll_pdiv = 7; // PostDIV: 16
            break;

        default:
            eMMC_debug(0, 0, "eMMC Err: emmc PLL not configed %Xh\n", u16_ClkParam);
            eMMC_die(" ");
            return eMMC_ST_ERR_UNKNOWN_CLK;
    }

    // 3. VCO clock ( loop N = 4 )
    REG_FCIE_CLRBIT(reg_emmcpll_fbdiv(emmc_ip), 0xffff);
    REG_FCIE_SETBIT(reg_emmcpll_fbdiv(emmc_ip), 0x6); // PostDIV: 8

    // 4. 1X clock
    REG_FCIE_CLRBIT(reg_emmcpll_pdiv(emmc_ip), BIT2 | BIT1 | BIT0);
    REG_FCIE_SETBIT(reg_emmcpll_pdiv(emmc_ip), u32_value_reg_emmc_pll_pdiv); // PostDIV: 8

    if (u16_ClkParam == eMMC_PLL_CLK__20M)
    {
        REG_FCIE_SETBIT(reg_emmc_pll_test(emmc_ip), BIT10);
    }
    else
    {
        REG_FCIE_CLRBIT(reg_emmc_pll_test(emmc_ip), BIT10);
    }

    eMMC_hw_timer_delay(HW_TIMER_DELAY_100us); // asked by Irwin

    return eMMC_ST_SUCCESS;
}

U32 eMMC_clock_setting(eMMC_IP_EmType emmc_ip, U16 u16_ClkParam)
{
    eMMC_PlatformResetPre();

    switch (u16_ClkParam)
    {
        // emmc_pll clock
        case eMMC_PLL_CLK__20M:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 20000;
            break;
        case eMMC_PLL_CLK__27M:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 27000;
            break;
        case eMMC_PLL_CLK__32M:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 32000;
            break;
        case eMMC_PLL_CLK__36M:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 36000;
            break;
        case eMMC_PLL_CLK__40M:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 40000;
            break;
        case eMMC_PLL_CLK__48M:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 48000;
            break;
        case eMMC_PLL_CLK__52M:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 52000;
            break;
        case eMMC_PLL_CLK__62M:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 62000;
            break;
        case eMMC_PLL_CLK__72M:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 72000;
            break;
        case eMMC_PLL_CLK__80M:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 80000;
            break;
        case eMMC_PLL_CLK__86M:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 86000;
            break;
        case eMMC_PLL_CLK_100M:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 100000;
            break;
        case eMMC_PLL_CLK_120M:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 120000;
            break;
        case eMMC_PLL_CLK_140M:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 140000;
            break;
        case eMMC_PLL_CLK_160M:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 160000;
            break;
        case eMMC_PLL_CLK_200M:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 200000;
            break;

        case BIT_FCIE_CLK_300K:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 300;
            break;
        case BIT_CLK_XTAL_12M:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 12000;
            break;
        case BIT_FCIE_CLK_20M:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 20000;
            break;
        case BIT_FCIE_CLK_32M:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 32000;
            break;
        case BIT_FCIE_CLK_36M:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 36000;
            break;
        case BIT_FCIE_CLK_40M:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 40000;
            break;
        case BIT_FCIE_CLK_43_2M:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 43200;
            break;
        case BIT_FCIE_CLK_48M:
            g_eMMCDrv[emmc_ip].u32_ClkKHz = 48000;
            break;
        default:
            eMMC_debug(eMMC_DEBUG_LEVEL_LOW, 1, "eMMC Err: %Xh\n", eMMC_ST_ERR_INVALID_PARAM);
            return eMMC_ST_ERR_INVALID_PARAM;
    }

    REG_FCIE_CLRBIT(EMMC_GET_CLK_REG(emmc_ip), BIT0 | BIT1);
    REG_FCIE_CLRBIT(EMMC_GET_CLK_REG(emmc_ip), BIT6 | BIT5 | BIT4 | BIT3 | BIT2);
    REG_FCIE_SETBIT(EMMC_GET_CLK_REG(emmc_ip), BIT6); // reg_ckg_sdio(BK:x1038_x43) [B5] -> 0:clk_boot 1:clk_sd

    REG_FCIE_SETBIT(reg_sc_gp_ctrl,
                    EMMC_GET_BOOT_CLK(emmc_ip)); // reg_ckg_sd(BK:x1133_x25) [B3]SDIO30 [B7]SD30  -> 0:clk_boot 1:clk_sd

    // eMMC_debug(0, 1, "clock %dk\n", g_eMMCDrv[emmc_ip].u32_ClkKHz);
    if (u16_ClkParam & eMMC_PLL_FLAG)
    {
        REG_FCIE_CLRBIT(GET_REG_ADDR(CLKGEN0_BASE, 0x44), BIT15 | BIT14 | BIT13 | BIT12);
        REG_FCIE_SETBIT(GET_REG_ADDR(CLKGEN0_BASE, 0x44), BIT14);
        REG_FCIE_SETBIT(GET_REG_ADDR(CLKGEN0_BASE, 0x43), BIT5 | BIT4 | BIT3); // select clk_sd30_1x_p

        _eMMC_pll_setting(emmc_ip, u16_ClkParam);
    }
    else
    {
        REG_FCIE_SETBIT(EMMC_GET_CLK_REG(emmc_ip), u16_ClkParam << 2);
    }

#if 1 // for fpga
    if (u16_ClkParam == BIT_FCIE_CLK_300K)
        REG_FCIE_SETBIT(EMMC_GET_CLK_REG(0), BIT6);
    else
        REG_FCIE_CLRBIT(EMMC_GET_CLK_REG(0), BIT6);
#endif

    g_eMMCDrv[emmc_ip].u16_ClkRegVal = u16_ClkParam;

    eMMC_PlatformResetPost();

    return eMMC_ST_SUCCESS;
}

void _eMMC_set_bustiming(eMMC_IP_EmType emmc_ip, U32 u32_FCIE_IF_Type)
{
    eMMC_debug(eMMC_DEBUG_LEVEL, 0, "eMMC%d >> [_eMMC_set_bustiming] %s mode. <<\r\n", emmc_ip,
               (u32_FCIE_IF_Type == FCIE_eMMC_BYPASS)              ? "BYPASS"
               : (u32_FCIE_IF_Type == FCIE_eMMC_SDR)               ? "SDR 8-bit macro"
               : (u32_FCIE_IF_Type == FCIE_MODE_8BITS_MACRO_DDR52) ? "DDR"
               : (u32_FCIE_IF_Type == FCIE_eMMC_HS200)             ? "HS200"
               : (u32_FCIE_IF_Type == FCIE_eMMC_HS400)             ? "HS400"
               : (u32_FCIE_IF_Type == FCIE_eMMC_5_1_AFIFO)         ? "HS400 5.1"
                                                                   : "Unknow");

    REG_FCIE(reg_emmcpll_0x6a(emmc_ip)) = (g_eMMCDrv[emmc_ip].u8_BUS_WIDTH >> 1);
    REG_FCIE(reg_emmcpll_0x73(emmc_ip)) = V_PLL_RX_BPS_EN_INIT;
    REG_FCIE(reg_emmcpll_0x74(emmc_ip)) = V_PLL_ATOP_BYP_RX_EN_INIT;
    // REG_FCIE(FCIE_DDR_MODE(emmc_ip)) = 0x0;
    //  fcie
    REG_FCIE_CLRBIT(FCIE_DDR_MODE(emmc_ip), BIT_BYPASS_MODE_MASK | BIT_CIFD_MODE_MASK | BIT_MACRO_MODE_MASK);

    switch (u32_FCIE_IF_Type)
    {
        case FCIE_eMMC_BYPASS:
        case FCIE_eMMC_SDR:
            // fcie
            REG_FCIE_SETBIT(FCIE_DDR_MODE(emmc_ip), BIT_PAD_IN_SEL_SD | BIT_FALL_LATCH | BIT10);

            REG_FCIE(reg_emmcpll_0x03(emmc_ip)) = 0x0;
            REG_FCIE_SETBIT(reg_emmc_test(emmc_ip), BIT10);
            REG_FCIE(reg_emmcpll_0x1c(emmc_ip)) = 0x0;
            REG_FCIE(reg_emmcpll_0x68(emmc_ip)) = 0x0;
            REG_FCIE(reg_emmcpll_0x69(emmc_ip)) = V_PLL_SKEW_SUM_INIT;
            REG_FCIE(reg_emmcpll_0x6b(emmc_ip)) = 0x0;
            REG_FCIE(reg_emmcpll_0x6c(emmc_ip)) = 0x0;
            REG_FCIE(reg_emmcpll_0x6d(emmc_ip)) = 0x0;
            REG_FCIE(reg_emmcpll_0x70(emmc_ip)) = 0x0;
            REG_FCIE(reg_emmcpll_0x71(emmc_ip)) = 0x3F;

            g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_SPEED_HIGH;
            break;

        case FCIE_MODE_8BITS_MACRO_DDR52:
            // fcie
            REG_FCIE_SETBIT(FCIE_DDR_MODE(emmc_ip), BIT_DDR_EN | BIT_8BIT_MACRO_EN);

            REG_FCIE(reg_emmcpll_0x03(emmc_ip)) = 0x0;
            REG_FCIE(reg_emmc_test(emmc_ip))    = 0x0;
            REG_FCIE(reg_emmcpll_0x1c(emmc_ip)) = R_PLL_1X_SYN_ECO_EN;
            REG_FCIE(reg_emmcpll_0x68(emmc_ip)) = R_PLL_EMMC_EN;
            REG_FCIE(reg_emmcpll_0x69(emmc_ip)) = V_PLL_SKEW_SUM_INIT;
            REG_FCIE(reg_emmcpll_0x6b(emmc_ip)) = 0x213;
            REG_FCIE(reg_emmcpll_0x6c(emmc_ip)) = 0x0;
            REG_FCIE(reg_emmcpll_0x6d(emmc_ip)) = R_PLL_DDR_IO_MODE;
            REG_FCIE(reg_emmcpll_0x70(emmc_ip)) = 0x0;
            REG_FCIE(reg_emmcpll_0x71(emmc_ip)) = 0x3F;

            g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_DDR_MODE;
            break;

        case FCIE_eMMC_HS200:
            // fcie
            REG_FCIE_SETBIT(FCIE_DDR_MODE(emmc_ip), BIT_32BIT_MACRO_EN);

            REG_FCIE(reg_emmcpll_0x03(emmc_ip)) = 0x0;
            REG_FCIE_SETBIT(reg_emmc_test(emmc_ip), R_PLL_PAD_DRV | R_PLL_G_RX_W_OEN_DOUT | R_PLL_G_RX_W_OEN_COUT);
            REG_FCIE(reg_emmcpll_0x1c(emmc_ip)) = R_PLL_32BIF_RX_ECO_EN;
            REG_FCIE(reg_emmcpll_0x68(emmc_ip)) = R_PLL_EMMC_EN;
            REG_FCIE(reg_emmcpll_0x69(emmc_ip)) = V_PLL_SKEW_SUM_INIT;
            REG_FCIE(reg_emmcpll_0x6b(emmc_ip)) = 0x413;
            REG_FCIE(reg_emmcpll_0x6c(emmc_ip)) = 0x0;
            REG_FCIE(reg_emmcpll_0x6d(emmc_ip)) = 0x0;
            REG_FCIE(reg_emmcpll_0x70(emmc_ip)) = R_PLL_SEL_FLASH_32BIF | R_PLL_RX_AFIFO_EN | R_PLL_RSP_AFIFO_EN;
            REG_FCIE(reg_emmcpll_0x71(emmc_ip)) = 0x0;

            g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_SPEED_HS200;
            break;

        case FCIE_eMMC_HS400:
            // fcie
            REG_FCIE_SETBIT(FCIE_DDR_MODE(emmc_ip), BIT_32BIT_MACRO_EN | BIT_DDR_EN);
            // emmc_pll
            REG_FCIE_CLRBIT(reg_emmcpll_0x03(emmc_ip),
                            0x0F0F); // only clean skew1 & skew3 , skew2 is set with table value
            REG_FCIE_SETBIT(reg_emmcpll_0x09(emmc_ip), BIT0); // reg_emmc_rxdll_dline_en
            REG_FCIE_SETBIT(reg_emmc_test(emmc_ip), BIT5 | BIT4);
            REG_FCIE_SETBIT(reg_emmcpll_0x1c(emmc_ip), BIT8);        // meta issue of clk4x and skew
            REG_FCIE_SETBIT(reg_emmcpll_0x1d(emmc_ip), BIT9);        // reg_cmd_use_skew4
            REG_FCIE_SETBIT(reg_emmcpll_0x20(emmc_ip), BIT10);       // reg_sel_internal
            REG_FCIE_SETBIT(reg_emmcpll_0x63(emmc_ip), BIT0);        // reg_use_rxdll
            REG_FCIE_SETBIT(reg_emmcpll_0x68(emmc_ip), BIT0 | BIT1); // reg_emmc_en | reg_emmc_ddr_en
            REG_FCIE_CLRBIT(reg_emmcpll_0x69(emmc_ip), BIT3);        // reg_clk_dig_inv
            REG_FCIE_CLRBIT(reg_emmcpll_0x69(emmc_ip), 0xF << 4);    // reg_tune_shot_offset
            REG_FCIE_SETBIT(reg_emmcpll_0x69(emmc_ip), 6 << 4);      // reg_tune_shot_offset

            if (g_eMMCDrv[emmc_ip].u8_BUS_WIDTH == BIT_SD_DATA_WIDTH_4)
            {
                REG_FCIE_SETBIT(reg_emmcpll_0x6a(emmc_ip), BIT0); // reg_io_bus_wid
                REG_FCIE_W(reg_emmcpll_0x6b(emmc_ip), 0x0213);    // reg_dqs_page_no
            }
            else if (g_eMMCDrv[emmc_ip].u8_BUS_WIDTH == BIT_SD_DATA_WIDTH_8)
            {
                REG_FCIE_SETBIT(reg_emmcpll_0x6a(emmc_ip), BIT1); // reg_io_bus_wid
                REG_FCIE_W(reg_emmcpll_0x6b(emmc_ip), 0x0113);    // reg_dqs_page_no
            }

            REG_FCIE_SETBIT(reg_emmcpll_0x70(emmc_ip), BIT8); // reg_sel_flash_32bif
            REG_FCIE_W(reg_emmcpll_0x71(emmc_ip), 0xF800);    // reg_tx_bps_en
            REG_FCIE_W(reg_emmcpll_0x73(emmc_ip), 0xFD00);    // reg_rx_bps_en
            // REG_FCIE_CLRBIT(reg_emmcpll_0x74, BIT15);                         // reg_atop_byp_rx_en
            // REG_FCIE_SETBIT(reg_emmcpll_0x1f,BIT2);                           // sw select TX ref. clock enable
            // REG_FCIE_W(reg_emmcpll_0x1e,0x0200);                              //select TX ref. clock

            g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_SPEED_HS400;
            break;

        case FCIE_eMMC_5_1_AFIFO:
            // fcie
            REG_FCIE_SETBIT(FCIE_DDR_MODE(emmc_ip), BIT_32BIT_MACRO_EN | BIT_DDR_EN);

            // emmc_pll
            REG_FCIE_CLRBIT(reg_emmcpll_0x03(emmc_ip),
                            0x0F0F); // only clean skew1 & skew3 , skew2 is set with table value
            REG_FCIE_SETBIT(reg_emmcpll_0x09(emmc_ip), BIT0); // reg_emmc_rxdll_dline_en
            REG_FCIE_SETBIT(reg_emmcpll_0x1c(emmc_ip), BIT8); // meta issue of clk4x and skew
            REG_FCIE_SETBIT(reg_emmc_test(emmc_ip), BIT5 | BIT4);
            REG_FCIE_SETBIT(reg_emmcpll_0x1d(emmc_ip), BIT9);        // reg_cmd_use_skew4
            REG_FCIE_SETBIT(reg_emmcpll_0x63(emmc_ip), BIT0);        // reg_use_rxdll
            REG_FCIE_SETBIT(reg_emmcpll_0x68(emmc_ip), BIT0 | BIT1); // reg_emmc_en | reg_emmc_ddr_en
            REG_FCIE_CLRBIT(reg_emmcpll_0x69(emmc_ip), BIT3);        // reg_clk_dig_inv
            REG_FCIE_SETBIT(reg_emmcpll_0x69(emmc_ip), 6 << 4);      // reg_tune_shot_offset

            if (g_eMMCDrv[emmc_ip].u8_BUS_WIDTH == BIT_SD_DATA_WIDTH_4)
            {
                REG_FCIE_SETBIT(reg_emmcpll_0x6a(emmc_ip), 1 << 0); // reg_io_bus_wid
                REG_FCIE_W(reg_emmcpll_0x6b(emmc_ip), 0x0213);      // reg_dqs_page_no
            }
            else if (g_eMMCDrv[emmc_ip].u8_BUS_WIDTH == BIT_SD_DATA_WIDTH_8)
            {
                REG_FCIE_SETBIT(reg_emmcpll_0x6a(emmc_ip), 2 << 0); // reg_io_bus_wid
                REG_FCIE_W(reg_emmcpll_0x6b(emmc_ip), 0x0113);      // reg_dqs_page_no
            }

            REG_FCIE_SETBIT(reg_emmcpll_0x70(emmc_ip), BIT8); // reg_sel_flash_32bif
            REG_FCIE_W(reg_emmcpll_0x71(emmc_ip), 0xF800);    // reg_tx_bps_en
            REG_FCIE_W(reg_emmcpll_0x73(emmc_ip), 0xFD00);    // reg_rx_bps_en
            // REG_FCIE_CLRBIT(reg_emmcpll_0x74, BIT15);                         // reg_atop_byp_rx_en
            // REG_FCIE_SETBIT(reg_emmcpll_0x1f,BIT2);                           // sw select TX ref. clock enable
            // REG_FCIE_W(reg_emmcpll_0x1e,0x0200);                              //select TX ref. clock
            REG_FCIE_SETBIT(reg_emmcpll_0x70(emmc_ip), BIT10 | BIT11);
            REG_FCIE_SETBIT(reg_emmcpll_0x7f(emmc_ip), BIT2 | BIT8);

            g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_SPEED_HIGH;
            break;

        default:
            eMMC_debug(1, 1, "eMMC%d Err: wrong parameter for switch pad func\n", emmc_ip);
    }
}

static void _eMMC_PLL_Auto_Turn_ONOFF_LDO(void)
{
    // 1. Disable OSP : FALSE =>(Enable)
    REG_FCIE_CLRBIT(GET_REG_ADDR(REG_BANK_SDPLL, 0x1D), BIT15);

    // 2. Set OSP counter[15:8] = 0x30
    REG_FCIE_CLRBIT(GET_REG_ADDR(REG_BANK_SDPLL, 0x37), BIT15 | BIT14 | BIT13 | BIT12 | BIT11 | BIT10 | BIT9 | BIT8);
    REG_FCIE_SETBIT(GET_REG_ADDR(REG_BANK_SDPLL, 0x37), BIT13 | BIT12);

    // 3. Turning on LDO  1->0
    REG_FCIE_SETBIT(GET_REG_ADDR(REG_BANK_SDPLL, 0x37), BIT5);
    eMMC_hw_timer_sleep(10);
    REG_FCIE_CLRBIT(GET_REG_ADDR(REG_BANK_SDPLL, 0x37), BIT5);
}

U32 eMMC_pads_switch(eMMC_IP_EmType emmc_ip, U32 u32_FCIE_IF_Type)
{
    uintptr_t ptr_reg  = 0;
    U16       u16_mask = 0, u16_4bitmode = 0, u16_8bitmode = 0;

    if (emmc_ip == IP_EMMC0)
    {
        ptr_reg      = GET_REG_ADDR(PAD_CHIPTOP_BASE, 0x61);
        u16_mask     = BIT0 | BIT1 | BIT2 | BIT3;
        u16_4bitmode = BIT0;
        u16_8bitmode = BIT2;
        _eMMC_PLL_Auto_Turn_ONOFF_LDO();
    }
    else if (emmc_ip == IP_EMMC1)
    {
        ptr_reg      = GET_REG_ADDR(PAD_CHIPTOP_BASE, 0x61);
        u16_mask     = BIT9 | BIT8 | BIT7 | BIT6;
        u16_4bitmode = BIT6;
        u16_8bitmode = BIT8;
    }

    REG_FCIE_CLRBIT(reg_all_pad_in, BIT_ALL_PAD_IN);
    REG_FCIE_CLRBIT(ptr_reg, u16_mask);
    switch (g_eMMCDrv[emmc_ip].u16_of_buswidth)
    {
        case 1:
            REG_FCIE_SETBIT(ptr_reg, u16_4bitmode);
            break;
        case 4:
            REG_FCIE_SETBIT(ptr_reg, u16_4bitmode);
            break;
        case 8:
            REG_FCIE_SETBIT(ptr_reg, u16_8bitmode);
            break;
        default:
            pr_err(">> [emmc] Err: wrong buswidth config: %u!\n", g_eMMCDrv[emmc_ip].u16_of_buswidth);
            REG_FCIE_SETBIT(ptr_reg, u16_8bitmode);
            break;
    };

    g_eMMCDrv[emmc_ip].u8_PadType = u32_FCIE_IF_Type;
    _eMMC_set_bustiming(emmc_ip, u32_FCIE_IF_Type);

    return eMMC_ST_SUCCESS;
}

void eMMC_driving_control(eMMC_IP_EmType emmc_ip)
{
    if (gu32_clk_driving[emmc_ip])
    {
    }
    if (gu32_cmd_driving[emmc_ip])
    {
    }
    if (gu32_data_driving[emmc_ip])
    {
    }
}

void eMMC_RST_L(eMMC_IP_EmType emmc_ip)
{
    uintptr_t ptr_reg;
    U16       u16_mask, u16_rstmode;

    if (emmc_ip == IP_EMMC0)
    {
        ptr_reg     = GET_REG_ADDR(PAD_CHIPTOP_BASE, 0x61);
        u16_mask    = BIT5 | BIT4;
        u16_rstmode = BIT4;
    }
    if (emmc_ip == IP_EMMC1)
    {
        ptr_reg     = GET_REG_ADDR(PAD_CHIPTOP_BASE, 0x61);
        u16_mask    = BIT14 | BIT13 | BIT12;
        u16_rstmode = BIT14;
    }

    REG_FCIE_CLRBIT(ptr_reg, u16_mask);
    REG_FCIE_SETBIT(ptr_reg, u16_rstmode);
    REG_FCIE_SETBIT(FCIE_BOOT_CONFIG(emmc_ip), BIT_EMMC_RSTZ_EN);
    REG_FCIE_CLRBIT(FCIE_BOOT_CONFIG(emmc_ip), BIT_EMMC_RSTZ);
}

void eMMC_RST_H(eMMC_IP_EmType emmc_ip)
{
    uintptr_t ptr_reg;
    U16       u16_mask, u16_rstmode;

    if (emmc_ip == IP_EMMC0)
    {
        ptr_reg     = GET_REG_ADDR(PAD_CHIPTOP_BASE, 0x61);
        u16_mask    = BIT5 | BIT4;
        u16_rstmode = BIT4;
    }
    if (emmc_ip == IP_EMMC1)
    {
        ptr_reg     = GET_REG_ADDR(PAD_CHIPTOP_BASE, 0x61);
        u16_mask    = BIT14 | BIT13 | BIT12;
        u16_rstmode = BIT14;
    }

    REG_FCIE_CLRBIT(ptr_reg, u16_mask);
    REG_FCIE_SETBIT(ptr_reg, u16_rstmode);
    REG_FCIE_SETBIT(FCIE_BOOT_CONFIG(emmc_ip), BIT_EMMC_RSTZ_EN);
    REG_FCIE_SETBIT(FCIE_BOOT_CONFIG(emmc_ip), BIT_EMMC_RSTZ);
    REG_FCIE_CLRBIT(FCIE_BOOT_CONFIG(emmc_ip), BIT_EMMC_RSTZ_EN);
}

dma_addr_t eMMC_Platform_Trans_Dma_Addr(eMMC_IP_EmType emmc_ip, dma_addr_t dma_DMAAddr, U32 *u32MiuSel)
{
    U32 miusel;
    if (u32MiuSel == NULL)
        u32MiuSel = &miusel;

    if (dma_DMAAddr >= 0X1000000000)
    {
        dma_DMAAddr -= 0x1000000000;
        *u32MiuSel = 0;
    }
    else
#ifdef MSTAR_MIU2_BUS_BASE
        if (dma_DMAAddr >= MSTAR_MIU2_BUS_BASE) // MIU2
    {
        dma_DMAAddr -= MSTAR_MIU2_BUS_BASE;
        *u32MiuSel = 2;
    }
    else
#endif
#ifdef MSTAR_MIU1_BUS_BASE
        if (dma_DMAAddr >= MSTAR_MIU1_BUS_BASE) // MIU1
    {
        dma_DMAAddr -= MSTAR_MIU1_BUS_BASE;
        *u32MiuSel = 1;
    }
    else // MIU0
#endif
    {
        dma_DMAAddr -= MSTAR_MIU0_BUS_BASE;
        *u32MiuSel = 0;
    }

    return dma_DMAAddr;
}
