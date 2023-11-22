/*
 * hal_card_platform.c- Sigmastar
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
/***************************************************************************************************************
 *
 * FileName hal_card_platform.c
 *     @author jeremy.wang (2016/11/29)
 * Desc:
 *     The platform setting of all cards will run here.
 *     Because register setting that doesn't belong to FCIE/SDIO may have different register setting at different
 *projects. The goal is that we don't need to change "other" HAL_XX.c Level code. (Timing, FCIE/SDIO)
 *
 *     The limitations were listed as below:
 *     (1) Each Project will have XX project name for different hal_card_platform_XX.c files.
 *     (2) IP init, PAD , clock, power and miu setting belong to here.
 *     (4) Timer setting doesn't belong to here, because it will be included by other HAL level.
 *     (5) FCIE/SDIO IP Reg Setting doesn't belong to here.
 *     (6) If we could, we don't need to change any code of hal_card_platform.h
 *
 ***************************************************************************************************************/

#include "../inc/hal_card_platform.h"
#include "../inc/hal_card_timer.h"
#include "gpio.h"
#include "padmux.h"
#include "mdrv_gpio.h"
#include "mdrv_padmux.h"
#include "hal_card_platform_pri_config.h"
#include "mdrv_puse.h"

//***********************************************************************************************************
// Config Setting (Internal)
//***********************************************************************************************************
// Platform Register Basic Address
//------------------------------------------------------------------------------------
#define A_CLKGEN_BANK  GET_CARD_REG_ADDR(A_RIU_BASE, 0x81C00) // 1038h
#define A_PADTOP_BANK  GET_CARD_REG_ADDR(A_RIU_BASE, 0x81E00) // 103Ch
#define A_CLKGEN2_BANK GET_CARD_REG_ADDR(A_RIU_BASE, 0x81F80) // 103Fh
//#define A_GPI_INT_BANK      GET_CARD_REG_ADDR(A_RIU_BASE, 0x81E80)//103Dh
#define A_PM_SLEEP_BANK GET_CARD_REG_ADDR(A_RIU_BASE, 0x00700) // 0Eh
#define A_PM_GPIO_BANK  GET_CARD_REG_ADDR(A_RIU_BASE, 0x00780) // 0Fh
#define A_CHIPTOP_BANK  GET_CARD_REG_ADDR(A_RIU_BASE, 0x80F00) // 101Eh
//#define A_MCM_SC_GP_BANK    GET_CARD_REG_ADDR(A_RIU_BASE, 0x89900)//1132h
#define A_SC_GP_CTRL_BANK GET_CARD_REG_ADDR(A_RIU_BASE, 0x89980) // 1133h
#define A_SDPLL_BANK      GET_CARD_REG_ADDR(A_RIU_BASE, 0xA0C80) // Bank: 0x1419
#define A_PADGPIO_BANK    GET_CARD_REG_ADDR(A_RIU_BASE, 0x81F00) // Bank: 0x103E
#define A_PADGPIO2_BANK   GET_CARD_REG_ADDR(A_RIU_BASE, 0x88200) // Bank: 0x1104
#define A_PM_PAD_TOP_BANK GET_CARD_REG_ADDR(A_RIU_BASE, 0x01F80) // 3Fh

#if defined(CONFIG_SUPPORT_SD30)
//-----------------------------------------------------------------------------------------------------------
// SD_PLL Register Basic Address
//-----------------------------------------------------------------------------------------------------------
#define A_SD_PLL_POS(IP) GET_PLL_BANK((IpOrder)IP, 0)

#define A_PLL_FBDIV_REG(IP)        GET_CARD_REG_ADDR(A_SD_PLL_POS(IP), 0x04)
#define A_PLL_PDIV_REG(IP)         GET_CARD_REG_ADDR(A_SD_PLL_POS(IP), 0x05)
#define A_PLL_RESET_PLL_REG(IP)    GET_CARD_REG_ADDR(A_SD_PLL_POS(IP), 0x06)
#define A_PLL_TEST_REG(IP)         GET_CARD_REG_ADDR(A_SD_PLL_POS(IP), 0x07)
#define A_PLL_DFFSET_15_0_REG(IP)  GET_CARD_REG_ADDR(A_SD_PLL_POS(IP), 0x18)
#define A_PLL_DFFSET_23_16_REG(IP) GET_CARD_REG_ADDR(A_SD_PLL_POS(IP), 0x19)
#endif

// Clock Level Setting (From High Speed to Low Speed)
//-----------------------------------------------------------------------------------------------------------
#define CLK1_F 200000000
#define CLK1_E 100000000
#define CLK1_D 48000000
#define CLK1_C 43200000 // 43200000
#define CLK1_B 40000000
#define CLK1_A 36000000 // 36000000
#define CLK1_9 32000000
#define CLK1_8 20000000
#define CLK1_7 12000000 // 12000000
#define CLK1_6 300000   // alway 400KHz for DTS
#define CLK1_5 0
#define CLK1_4 0
#define CLK1_3 0
#define CLK1_2 0
#define CLK1_1 0
#define CLK1_0 0

#define CLK2_F 48000000
#define CLK2_E 43200000
#define CLK2_D 40000000
#define CLK2_C 36000000
#define CLK2_B 32000000
#define CLK2_A 20000000
#define CLK2_9 12000000
#define CLK2_8 300000
#define CLK2_7 0
#define CLK2_6 0
#define CLK2_5 0
#define CLK2_4 0
#define CLK2_3 0
#define CLK2_2 0
#define CLK2_1 0
#define CLK2_0 0

#define CLK3_F 48000000
#define CLK3_E 43200000
#define CLK3_D 40000000
#define CLK3_C 36000000
#define CLK3_B 32000000
#define CLK3_A 20000000
#define CLK3_9 12000000
#define CLK3_8 300000
#define CLK3_7 0
#define CLK3_6 0
#define CLK3_5 0
#define CLK3_4 0
#define CLK3_3 0
#define CLK3_2 0
#define CLK3_1 0
#define CLK3_0 0

#define REG_CLK_IP_SD    (0x43)
#define REG_CLK_IP_SDIO  (0x45)
#define REG_CLK_IP_SDIO2 (0x24)

// Bonding ID
//----------------------------------------------------------------------------------------------------------
#define BOND_SSM613D 0x00 // QFN  1G
#define BOND_SSM613Q 0x01 // QFN  2G
#define BOND_SSM616Q 0x31 // LQFN 2G
#define BOND_SSM633Q 0x11 // BGA1 2G
#define BOND_SSM650G 0x27 // BGA2

// fpga verify setting
//----------------------------------------------------------------------------------------------------------
#define pr_sd_err(fmt, arg...) printk(fmt, ##arg)
#define pr_sd_dbg(fmt, arg...) printk(fmt, ##arg)

#define UNUSED(x) (x = x)

static U32_T gu32_SdPWR[3];
static U32_T gu32_SdCDZ[3];

#if (PADMUX_SET == PADMUX_SET_BY_FUNC)
static U32_T gu32_SdMode[3];
static U32_T gu32_SdPwrMode[3];
static U32_T gu32_SdCdzMode[3];
static U32_T gu32_SdCLK[3];
static U32_T gu32_SdCMD[3];
static U32_T gu32_SdDAT[3][4];

static U32_T gu32_SDPuse[3][8] = {
    {MDRV_PUSE_SDIO0_CLK, MDRV_PUSE_SDIO0_CMD, MDRV_PUSE_SDIO0_D0, MDRV_PUSE_SDIO0_D1, MDRV_PUSE_SDIO0_D2,
     MDRV_PUSE_SDIO0_D3, MDRV_PUSE_SDIO0_PWR, MDRV_PUSE_SDIO0_CDZ},
    {MDRV_PUSE_SDIO1_CLK, MDRV_PUSE_SDIO1_CMD, MDRV_PUSE_SDIO1_D0, MDRV_PUSE_SDIO1_D1, MDRV_PUSE_SDIO1_D2,
     MDRV_PUSE_SDIO1_D3, MDRV_PUSE_SDIO1_PWR, MDRV_PUSE_SDIO1_CDZ},
    {MDRV_PUSE_SDIO2_CLK, MDRV_PUSE_SDIO2_CMD, MDRV_PUSE_SDIO2_D0, MDRV_PUSE_SDIO2_D1, MDRV_PUSE_SDIO2_D2,
     MDRV_PUSE_SDIO2_D3, MDRV_PUSE_SDIO2_PWR, MDRV_PUSE_SDIO2_CDZ},
};
#endif
extern DrvCtrlType ge_ClkDriving[3];
extern DrvCtrlType ge_CmdDriving[3];
extern DrvCtrlType ge_DataDriving[3];

#if defined(CONFIG_SUPPORT_SD30)
extern volatile BusTimingEmType ge_BusTiming[3];

static BOOL_T _PLTSD_PLL_Switch_AVDD(IpOrder eIP, PADVddEmType ePADVdd)
{
    if (ePADVdd == EV_NORVOL)
    {
        CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_SDPLL_BANK, 0X22), BIT01_T); // MS = 0
        Hal_Timer_mSleep(5);
        CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x0F)) = 0x0008; // _VCTRL = 0
    }
    else if (ePADVdd == EV_LOWVOL)
    {
        CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x0F)) = 0x000A; // _VCTRL = 1
        Hal_Timer_mSleep(5);
        CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_SDPLL_BANK, 0X22), BIT01_T); // MS = 1
    }

    return 0;
}

void _PLTSD_PLL_1XClock_Setting(IpSelect eIP, U32_T u32ClkFromIPSet)
{
    U8_T u8PLOOP_DIV_REG = 0;
    U8_T u8PDIV_REG      = 0;

    //(1) reset emmc pll
    CARD_REG_SETBIT(A_PLL_RESET_PLL_REG(eIP), BIT00_T);
    CARD_REG_CLRBIT(A_PLL_RESET_PLL_REG(eIP), BIT00_T);

    //(2) synth clock
    switch (u32ClkFromIPSet)
    {
        case CLK1_F: // 200000KHz
            // 195MHz
            CARD_REG(A_PLL_DFFSET_23_16_REG(eIP)) = 0x21;
            CARD_REG(A_PLL_DFFSET_15_0_REG(eIP))  = 0x3B13;
            u8PLOOP_DIV_REG                       = 0x12;
            u8PDIV_REG                            = 0x00;
            break;

        case CLK1_E: // 100000KHz
            CARD_REG(A_PLL_DFFSET_23_16_REG(eIP)) = 0x21;
            CARD_REG(A_PLL_DFFSET_15_0_REG(eIP))  = 0x3B13;
            u8PLOOP_DIV_REG                       = 0x09;
            u8PDIV_REG                            = 0x00;
            break;

        case CLK1_D: // 48000MHz //DDR
            // The same with 195MHz with 8-bit macro
            CARD_REG(A_PLL_DFFSET_23_16_REG(eIP)) = 0x22;
            CARD_REG(A_PLL_DFFSET_15_0_REG(eIP))  = 0x8F5C;
            u8PLOOP_DIV_REG                       = 0x12;
            u8PDIV_REG                            = 0x00;
            break;
    }

    //(3) LOOP DIV setting
    CARD_REG_CLRBIT(A_PLL_FBDIV_REG(eIP), 0xFF00);
    CARD_REG_SETBIT(A_PLL_FBDIV_REG(eIP), (u8PLOOP_DIV_REG << 8));

    //(4) PDIV setting
    CARD_REG_CLRBIT(A_PLL_PDIV_REG(eIP), BIT02_T | BIT01_T | BIT00_T);
    CARD_REG_SETBIT(A_PLL_PDIV_REG(eIP), u8PDIV_REG);

    //(5) select clk_sd30_1x_p
    CARD_REG(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD)) = 0x8078;

    Hal_Timer_uDelay(100);
}

void _PLTSD_PLL_Init(IpSelect eIP)
{
    CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x4C)) = 0xfD8;

    CARD_REG_CLRBIT(0x1F223624, 0x00FF); // bank:111b, off:0x09 value:0x00;
    CARD_REG_CLRBIT(0x1F206006, 0x00FF); // bank:1030, off:0x01 bit[15:8] value:0x00;
    CARD_REG(GET_CARD_REG_ADDR(A_CLKGEN_BANK, 0x44)) = 0x1400;

    CARD_REG(GET_CARD_REG_ADDR(A_SD_PLL_POS(eIP), 0x18)) = 0x0000;

    CARD_REG(GET_CARD_REG_ADDR(A_SD_PLL_POS(eIP), 0x19)) &= 0xFF00;
    CARD_REG(GET_CARD_REG_ADDR(A_SD_PLL_POS(eIP), 0x19)) |= 0x0020;

    CARD_REG(GET_CARD_REG_ADDR(A_SD_PLL_POS(eIP), 0x04)) = 0x0C08;

    CARD_REG(GET_CARD_REG_ADDR(A_SD_PLL_POS(eIP), 0x08)) = 0x0180;

    CARD_REG(GET_CARD_REG_ADDR(A_SD_PLL_POS(eIP), 0x07)) = 0x006F;

    CARD_REG(GET_CARD_REG_ADDR(A_SD_PLL_POS(eIP), 0x21)) = 0x0005;

    CARD_REG(GET_CARD_REG_ADDR(A_SD_PLL_POS(eIP), 0x2A)) = 0x00F3;

    // Set pll skew
    CARD_REG(GET_CARD_REG_ADDR(A_SD_PLL_POS(eIP), 0x03)) &= 0xFF00;

    Hal_Timer_mDelay(100);
}
#endif

U8_T Hal_CARD_PadmuxGetting(U32_T *u32_SDIP_Arr)
{
#if (PADMUX_SET == PADMUX_SET_BY_FUNC)
    U32_T u32_SdPadPin[3][6];
    U32_T i, j, k, slotNo = 0;

    if (mdrv_padmux_active())
    {
        for (i = 0; i < 3; i++)
        {
            for (j = 0; j < 6; j++)
            {
                // sd0 pad pin getting
                u32_SdPadPin[i][j] = mdrv_padmux_getpad(gu32_SDPuse[i][j]);
                if (u32_SdPadPin[i][j] == PAD_UNKNOWN)
                {
                    pr_err("Fail to get pad(%d) form padmux !\n", gu32_SDPuse[i][j]);
                    break;
                }
            }
            if (j == 6)
            {
                gu32_SdCLK[i] = u32_SdPadPin[i][0];
                gu32_SdCMD[i] = u32_SdPadPin[i][1];
                for (k = 0; k < 4; k++)
                    gu32_SdDAT[i][k] = u32_SdPadPin[i][2 + k];

                gu32_SdMode[i]         = mdrv_padmux_getmode(gu32_SDPuse[i][0]);
                u32_SDIP_Arr[slotNo++] = i;
            }
        }
    }
    return slotNo;
#else
    return 0;
#endif
}

//***********************************************************************************************************
// IP Setting for Card Platform
//***********************************************************************************************************
void Hal_CARD_IPOnceSetting(IpOrder eIP)
{
    IpSelect eIpSel = (IpSelect)eIP;

#if (FORCE_SWITCH_PAD)
    // reg_all_pad_in => Close
    CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_CHIPTOP_BANK, 0x50), BIT15_T);
#endif
#if defined(CONFIG_SUPPORT_SD30)
    // pll init
    _PLTSD_PLL_Init(eIpSel);
#endif
    // Clock Source
    if (eIpSel == IP_SD)
    {
        // BK:x1133(sc_gp_ctrl)[B7] [0:1]/[boot_clk(12M):sd_clk]
        CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_SC_GP_CTRL_BANK, 0x25), BIT07_T);
        // BK:x1038(reg_ckg_sd)[B6] [0:1]/[boot_clk(12M):sd_clk]
        CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD), BIT06_T);
    }
    else if (eIpSel == IP_SDIO)
    {
        CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_SC_GP_CTRL_BANK, 0x25), BIT03_T);
        CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SDIO), BIT06_T);
    }
    else if (eIpSel == IP_SDIO2)
    {
        CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_SC_GP_CTRL_BANK, 0x25), BIT04_T);
        CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN2_BANK, REG_CLK_IP_SDIO2), BIT06_T);
    }
}
//***********************************************************************************************************
// PAD Setting for Card Platform
//***********************************************************************************************************
void Hal_CARD_InitPADPin(IpOrder eIP, PadOrder ePAD)
{
    IpSelect  eIpSel  = (IpSelect)eIP;
    PadSelect ePadSel = (PadSelect)ePAD;
    U8_T      i;

#if (PADMUX_SET == PADMUX_SET_BY_FUNC)
    MDrv_GPIO_Pull_Up(gu32_SdCDZ[eIpSel]);

    MDrv_GPIO_Pull_Up(gu32_SdCMD[eIpSel]);
    for (i = 0; i < 4; i++)
    {
        MDrv_GPIO_Pull_Up(gu32_SdDAT[eIpSel][i]);
    }

    UNUSED(ePadSel);
#else
    if (eIpSel == IP_SD)
    {
        if (ePadSel == PAD_SD_MD1)
        {
            // pll 0xx29[8:0] reg_pd, pull down, 0x2b[8:0] reg_pu
            // [B3:B0]/DAT[3:0], [B4]/CMD, [B5]/CLK, [B6]/CDZ
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_SDPLL_BANK, 0x29), 0x7F);
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_SDPLL_BANK, 0x2B), 0x5F);
        }
        else
        {
            printk(" >> sdmmc_%d mode_%d is not configured\n", eIpSel, ePadSel);
        }
    }
    else if (eIpSel == IP_SDIO)
    {
        if (ePadSel == PAD_SD_MD1)
        {
            CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x48)) = 0xFD4;
            CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x49)) = 0xFD4;
            CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x4A)) = 0xFD4;
            CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x4B)) = 0xFE4;
            CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x4C)) = 0xFD4;
            CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x4D)) = 0xFD4;
            CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x4E)) = 0xFD4;
        }
    }
    else if (eIpSel == IP_SDIO2)
    {
        if (ePadSel == PAD_SD_MD2)
        {
            CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x31))  = 0xFD4;
            CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x29)) = 0xFD4;
            CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x2A)) = 0xFD4;
            CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x2B)) = 0xFE4;
            CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x2C)) = 0xFD4;
            CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x2D)) = 0xFD4;
            CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x2E)) = 0xFD4;
        }
    }
    UNUSED(i);
#endif
    // Pull Down
    Hal_CARD_PullPADPin(eIP, ePAD, EV_PULLDOWN);
}

BOOL_T Hal_CARD_GetPadInfoCdzPad(IpOrder eIP, U32_T *nPadNum)
{
#if (PADMUX_SET == PADMUX_SET_BY_FUNC)
    gu32_SdCDZ[eIP] = mdrv_padmux_getpad(gu32_SDPuse[eIP][7]);
    if (gu32_SdCDZ[eIP] == PAD_UNKNOWN)
    {
        if (nPadNum)
        {
            gu32_SdCDZ[eIP] = *nPadNum;
            return TRUE;
        }
        return FALSE;
    }

    gu32_SdCdzMode[eIP] = mdrv_padmux_getmode(gu32_SDPuse[eIP][7]);
    *nPadNum            = gu32_SdCDZ[eIP];
    return TRUE;
#else
    return TRUE;
#endif
}

BOOL_T Hal_CARD_GetPadInfoPowerPad(IpOrder eIP, U32_T *nPadNum)
{
#if (PADMUX_SET == PADMUX_SET_BY_FUNC)
    gu32_SdPWR[eIP] = mdrv_padmux_getpad(gu32_SDPuse[eIP][6]);
    if (gu32_SdPWR[eIP] == PAD_UNKNOWN)
    {
        if (nPadNum)
        {
            gu32_SdPWR[eIP]     = *nPadNum;
            gu32_SdPwrMode[eIP] = PINMUX_FOR_GPIO_MODE;
            return TRUE;
        }
        return FALSE;
    }

    gu32_SdPwrMode[eIP] = mdrv_padmux_getmode(gu32_SDPuse[eIP][6]);
    *nPadNum            = gu32_SdPWR[eIP];
    return TRUE;
#else
    return TRUE;
#endif
}

void Hal_CARD_ConfigSdPad(IpOrder eIP, PadOrder ePAD) // Hal_CARD_SetPADToPortPath
{
    IpSelect  eIpSel  = (IpSelect)eIP;
    PadSelect ePadSel = (PadSelect)ePAD;

#if (PADMUX_SET == PADMUX_SET_BY_FUNC)
    // padmux get null, get pad pin from dts setting
    if (!gu32_SdMode[eIpSel])
    {
        if (eIpSel == IP_SD)
        {
            if (ePadSel == PAD_SD_MD1)
            {
                gu32_SdCLK[eIpSel]     = PAD_SD0_CLK;
                gu32_SdCMD[eIpSel]     = PAD_SD0_CMD;
                gu32_SdDAT[eIpSel][0]  = PAD_SD0_D0;
                gu32_SdDAT[eIpSel][1]  = PAD_SD0_D1;
                gu32_SdDAT[eIpSel][2]  = PAD_SD0_D2;
                gu32_SdDAT[eIpSel][3]  = PAD_SD0_D3;
                gu32_SdMode[eIpSel]    = PINMUX_FOR_SD0_MODE_1;
                gu32_SdCdzMode[eIpSel] = PINMUX_FOR_SD0_CDZ_MODE_1;
            }
            else if (ePadSel == PAD_SD_MD2)
            {
                gu32_SdCLK[eIpSel]     = PAD_GPIO11;
                gu32_SdCMD[eIpSel]     = PAD_GPIO10;
                gu32_SdDAT[eIpSel][0]  = PAD_GPIO12;
                gu32_SdDAT[eIpSel][1]  = PAD_GPIO13;
                gu32_SdDAT[eIpSel][2]  = PAD_GPIO8;
                gu32_SdDAT[eIpSel][3]  = PAD_GPIO9;
                gu32_SdMode[eIpSel]    = PINMUX_FOR_SD0_MODE_2;
                gu32_SdCdzMode[eIpSel] = PINMUX_FOR_SD0_CDZ_MODE_2;
            }
        }
        else if (eIpSel == IP_SDIO)
        {
            if (ePadSel == PAD_SD_MD1)
            {
                gu32_SdCLK[eIpSel]     = PAD_SD1_CLK;
                gu32_SdCMD[eIpSel]     = PAD_SD1_CMD;
                gu32_SdDAT[eIpSel][0]  = PAD_SD1_D0;
                gu32_SdDAT[eIpSel][1]  = PAD_SD1_D1;
                gu32_SdDAT[eIpSel][2]  = PAD_SD1_D2;
                gu32_SdDAT[eIpSel][3]  = PAD_SD1_D3;
                gu32_SdMode[eIpSel]    = PINMUX_FOR_SD1_MODE_1;
                gu32_SdCdzMode[eIpSel] = PINMUX_FOR_SD1_CDZ_MODE_1;
            }
            else if (ePadSel == PAD_SD_MD2)
            {
                gu32_SdCLK[eIpSel]     = PAD_I2S2_RX_DI;
                gu32_SdCMD[eIpSel]     = PAD_I2S3_RX_BCK;
                gu32_SdDAT[eIpSel][0]  = PAD_I2S2_RX_WCK;
                gu32_SdDAT[eIpSel][1]  = PAD_I2S2_RX_BCK;
                gu32_SdDAT[eIpSel][2]  = PAD_I2S3_RX_DI;
                gu32_SdDAT[eIpSel][3]  = PAD_I2S3_RX_WCK;
                gu32_SdMode[eIpSel]    = PINMUX_FOR_SD1_MODE_2;
                gu32_SdCdzMode[eIpSel] = PINMUX_FOR_SD1_CDZ_MODE_2;
            }
            else if (ePadSel == PAD_SD_MD2)
            {
                gu32_SdCLK[eIpSel]     = PAD_I2S0_TX_DO;
                gu32_SdCMD[eIpSel]     = PAD_I2S1_TX_BCK;
                gu32_SdDAT[eIpSel][0]  = PAD_I2S0_TX_WCK;
                gu32_SdDAT[eIpSel][1]  = PAD_I2S0_TX_BCK;
                gu32_SdDAT[eIpSel][2]  = PAD_I2S1_TX_DO;
                gu32_SdDAT[eIpSel][3]  = PAD_I2S1_TX_WCK;
                gu32_SdMode[eIpSel]    = PINMUX_FOR_SD1_MODE_3;
                gu32_SdCdzMode[eIpSel] = PINMUX_FOR_SD1_CDZ_MODE_3;
            }
        }
        else if (eIpSel == IP_SDIO2)
        {
            if (ePadSel == PAD_SD_MD1)
            {
                gu32_SdCLK[eIpSel]     = PAD_GPIO0;
                gu32_SdCMD[eIpSel]     = PAD_GPIO1;
                gu32_SdDAT[eIpSel][0]  = PAD_PWM0;
                gu32_SdDAT[eIpSel][1]  = PAD_PWM1;
                gu32_SdDAT[eIpSel][2]  = PAD_PWM2;
                gu32_SdDAT[eIpSel][3]  = PAD_PWM3;
                gu32_SdMode[eIpSel]    = PINMUX_FOR_SD2_MODE_1;
                gu32_SdCdzMode[eIpSel] = PINMUX_FOR_SD2_CDZ_MODE_1;
            }
            else if (ePadSel == PAD_SD_MD2)
            {
                gu32_SdCLK[eIpSel]     = PAD_I2S2_RX_DI;
                gu32_SdCMD[eIpSel]     = PAD_I2S3_RX_BCK;
                gu32_SdDAT[eIpSel][0]  = PAD_I2S2_RX_WCK;
                gu32_SdDAT[eIpSel][1]  = PAD_I2S2_RX_BCK;
                gu32_SdDAT[eIpSel][2]  = PAD_I2S3_RX_DI;
                gu32_SdDAT[eIpSel][3]  = PAD_I2S3_RX_WCK;
                gu32_SdMode[eIpSel]    = PINMUX_FOR_SD2_MODE_2;
                gu32_SdCdzMode[eIpSel] = PINMUX_FOR_SD2_CDZ_MODE_2;
            }
            else if (ePadSel == PAD_SD_MD2)
            {
                gu32_SdCLK[eIpSel]     = PAD_I2S0_TX_DO;
                gu32_SdCMD[eIpSel]     = PAD_I2S1_TX_BCK;
                gu32_SdDAT[eIpSel][0]  = PAD_I2S0_TX_WCK;
                gu32_SdDAT[eIpSel][1]  = PAD_I2S0_TX_BCK;
                gu32_SdDAT[eIpSel][2]  = PAD_I2S1_TX_DO;
                gu32_SdDAT[eIpSel][3]  = PAD_I2S1_TX_WCK;
                gu32_SdMode[eIpSel]    = PINMUX_FOR_SD2_MODE_3;
                gu32_SdCdzMode[eIpSel] = PINMUX_FOR_SD2_CDZ_MODE_3;
            }
        }
    }

    if (0 != mdrv_padmux_active())
    {
        MDrv_GPIO_PadGroupMode_Set(gu32_SdMode[eIP]);
        MDrv_GPIO_PadGroupMode_Set(gu32_SdCdzMode[eIP]);
    }

#else //

    // SD mode
    if (eIpSel == IP_SD)
    {
        // OFF:x67 [B9:B8]/[reg_sd0_mode], [B10]/[reg_sd0_cdz_mode]
        CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT10_T | BIT09_T | BIT08_T);
        if (ePadSel == PAD_SD_MD1)
        {
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT08_T | BIT10_T);
        }
    }
    else if (eIpSel == IP_SDIO)
    {
        // OFF:x67 [B14:B12]reg_sd1_mode
        // OFF:x68 [B14:B12]reg_sd1_cdz_mode
        CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT14_T | BIT13_T | BIT12_T);
        CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x68), BIT14_T | BIT13_T | BIT12_T);
        if (ePadSel == PAD_SD_MD1)
        {
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT12_T);
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x68), BIT12_T);
        }
    }
    else if (eIpSel == IP_SDIO2)
    {
        // OFF:x4d [B1:B0]reg_sd2_mode, [B5:B4]reg_sd2_cdz_mode
        CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x4d), BIT05_T | BIT04_T | BIT01_T | BIT00_T);
        if (ePadSel == PAD_SD_MD2)
        {
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x4d), BIT01_T);
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x4d), BIT04_T);
        }
    }

#endif
}

void Hal_CARD_ConfigPowerPad(IpOrder eIP, U16_T nPadNum)
{
    IpSelect eIpSel = (IpSelect)eIP;
    U16_T    nPadNo = nPadNum;

    if (eIpSel >= IP_TOTAL)
    {
        pr_sd_err("sdmmc error ! [%s][%d]\n", __FUNCTION__, __LINE__);
        return;
    }

    if (nPadNo == PAD_UNKNOWN)
    {
        // Save Power PadNum
        gu32_SdPWR[eIP] = PAD_UNKNOWN;
        return;
    }

#if (PADMUX_SET == PADMUX_SET_BY_FUNC)

    if (0 != mdrv_padmux_active())
    {
        MDrv_GPIO_PadVal_Set(gu32_SdPWR[eIP], gu32_SdPwrMode[eIP]);
        MDrv_GPIO_Pad_Oen(gu32_SdPWR[eIP]);
    }

#else
    switch (nPadNo)
    {
        case PAD_SD0_GPIO0:
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x10), BIT03_T);
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x10), (BIT02_T | BIT01_T | BIT00_T));
            break;

        case PAD_SD1_GPIO1:
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x47), BIT03_T);
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x47), (BIT02_T | BIT01_T | BIT00_T));
            break;

        default:
            pr_sd_err("sdmmc error ! [%s][%d]\n", __FUNCTION__, __LINE__);
            return;
            break;
    }
#endif
    gu32_SdPWR[eIP] = nPadNo;

    // Default power off
    Hal_CARD_PowerOff(eIP, 0);
}

void Hal_CARD_PullPADPin(IpOrder eIP, PadOrder ePAD, PinPullEmType ePinPull)
{
    IpSelect  eIpSel  = (IpSelect)eIP;
    PadSelect ePadSel = (PadSelect)ePAD;

#if (PADMUX_SET == PADMUX_SET_BY_FUNC)
    U8_T i;

    if (ePinPull == EV_PULLDOWN)
    {
        MDrv_GPIO_Set_Low(gu32_SdCLK[eIpSel]);
        MDrv_GPIO_Pad_Oen(gu32_SdCLK[eIpSel]);
        MDrv_GPIO_PadVal_Set(gu32_SdCLK[eIpSel], PINMUX_FOR_GPIO_MODE);

        MDrv_GPIO_Pull_Off(gu32_SdCMD[eIpSel]);
        MDrv_GPIO_Set_Low(gu32_SdCMD[eIpSel]);
        MDrv_GPIO_Pad_Oen(gu32_SdCMD[eIpSel]);
        MDrv_GPIO_PadVal_Set(gu32_SdCMD[eIpSel], PINMUX_FOR_GPIO_MODE);
        for (i = 0; i < 4; i++)
        {
            MDrv_GPIO_Pull_Off(gu32_SdDAT[eIpSel][i]);
            MDrv_GPIO_Set_Low(gu32_SdDAT[eIpSel][i]);
            MDrv_GPIO_Pad_Oen(gu32_SdDAT[eIpSel][i]);
            MDrv_GPIO_PadVal_Set(gu32_SdDAT[eIpSel][i], PINMUX_FOR_GPIO_MODE);
        }
    }
    else if (ePinPull == EV_PULLUP)
    {
        MDrv_GPIO_Pad_Odn(gu32_SdCLK[eIpSel]);

        MDrv_GPIO_Pull_Up(gu32_SdCMD[eIpSel]);
        MDrv_GPIO_Pad_Odn(gu32_SdCMD[eIpSel]);
        for (i = 0; i < 4; i++)
        {
            MDrv_GPIO_Pull_Up(gu32_SdDAT[eIpSel][i]);
            MDrv_GPIO_Pad_Odn(gu32_SdDAT[eIpSel][i]);
        }
        MDrv_GPIO_PadGroupMode_Set(gu32_SdMode[eIpSel]);
    }

    UNUSED(ePadSel);
#else
    // IP_SD
    if (eIpSel == IP_SD)
    {
        if (ePadSel == PAD_SD_MD1)
        {
            if (ePinPull == EV_PULLDOWN)
            {
                // OFF:x67 [B8:B9]reg_sd0_mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT08_T | BIT09_T);
                // pull pad pin DOWN
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_SDPLL_BANK, 0x29), 0x3F);
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_SDPLL_BANK, 0x2B), 0x3F);
            }
            else if (ePinPull == EV_PULLUP)
            {
                // pull pad pin up. ex:clk no need
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_SDPLL_BANK, 0x29), 0x3F);
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_SDPLL_BANK, 0x2B), 0x1F);
                // SD Mode
                // OFF:x67 [B8:B9]reg_sd0_mode [B10]reg_sd0_cdz_mode
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT08_T);
            }
        }
    }
    else if (eIpSel == IP_SDIO) // SDIO PAD
    {
        if (ePadSel == PAD_SD_MD1)
        {
            if (ePinPull == EV_PULLDOWN)
            {
                // pull down and output low
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x49)) = 0xFE4; // D1
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x4A)) = 0xFE0; // D0
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x4B)) = 0xFE0; // CLK
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x4C)) = 0xFE0; // CMD
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x4D)) = 0xFE0; // D3
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x4E)) = 0xFE0; // D2

                //[B14:B12] sd1 mode clear
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT14_T | BIT13_T | BIT12_T);
            }
            else if (ePinPull == EV_PULLUP)
            {
                // pull up and input
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x49)) = 0xFD5; // D1
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x4A)) = 0xFD5; // D0
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x4B)) = 0xFE5; // CLK
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x4C)) = 0xFD5; // CMD
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x4D)) = 0xFD5; // D3
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x4E)) = 0xFD5; // D2

                //[B14:B12] set sd1 mode 1
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT12_T);
            }
        }
    }
    else if (eIpSel == IP_SDIO2) // SDIO2
    {
        if (ePadSel == PAD_SD_MD2)
        {
            if (ePinPull == EV_PULLDOWN)
            {
                // pull down and output low
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x29)) = 0xFE0; // D1
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x2A)) = 0xFE0; // D0
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x2B)) = 0xFE0; // CLK
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x2C)) = 0xFE0; // CMD
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x2D)) = 0xFE0; // D3
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x2E)) = 0xFE0; // D2

                //[B1:B0] sd2 mode clear
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x4D), BIT01_T | BIT00_T);
            }
            else if (ePinPull == EV_PULLUP)
            {
                // pull up and input
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x29)) = 0xFD5; // D1
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x2A)) = 0xFD5; // D0
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x2B)) = 0xFE5; // CLK
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x2C)) = 0xFD5; // CMD
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x2D)) = 0xFD5; // D3
                CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x2E)) = 0xFD5; // D2

                //[B1:B0] set sd2 mode 2
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x4D), BIT01_T);
            }
        }
    }
#endif
}

//***********************************************************************************************************
// Signal line driving control Setting for Card Platform
//***********************************************************************************************************
void Hal_CARD_DrvCtrlPin(IpOrder eIP, PadOrder ePAD)
{
    IpSelect  eIpSel  = (IpSelect)eIP;
    PadSelect ePadSel = (PadSelect)ePAD;

    U8_T u8_PadClk, u8_ClkDrvMax      = DRV_CTRL_15;
    U8_T u8_PadCmd, u8_CmdDrvMax      = DRV_CTRL_15;
    U8_T u8_PadData[4], u8_DataDrvMax = DRV_CTRL_15, i;

#if (PADMUX_SET == PADMUX_SET_BY_FUNC)
    u8_PadClk = gu32_SdCLK[eIP];
    u8_PadCmd = gu32_SdCMD[eIP];
    for (i = 0; i < 4; i++)
        u8_PadData[i] = gu32_SdDAT[eIP][i];

    UNUSED(ePadSel);
#else
    if (eIpSel == IP_SD) // sd3.0 padpin specially set by reg.
    {
        if (ePadSel == PAD_SD_MD1)
        {
#if 0
            // sd0 mode1 pad's driving control is set by sd_pll reg_ds{3:0}[8:0]
            // bank1419, off:0x23 ds0; x24 ds1; x25 ds2; x26 ds3
            // ds{3:0} [B3:B0]/[DAT[3:0], [B4]/[CMD], [B5]/[CLK], [B6]/CDZ
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_SDPLL_BANK, 0x23), 0x3F);
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_SDPLL_BANK, 0x23),
                            (((ge_ClkDriving[eIpSel] & 0x1) << 5) | ((ge_CmdDriving[eIpSel] & 0x1) << 4)
                             | ((ge_DataDriving[eIpSel] & 0x1) ? 0xF : 0)));

            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_SDPLL_BANK, 0x24), 0x3F);
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_SDPLL_BANK, 0x24),
                            (((ge_ClkDriving[eIpSel] & 0x2) << 4) | ((ge_CmdDriving[eIpSel] & 0x2) << 3)
                             | ((ge_DataDriving[eIpSel] & 0x2) ? 0xF : 0)));

            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_SDPLL_BANK, 0x25), 0x3F);
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_SDPLL_BANK, 0x25),
                            (((ge_ClkDriving[eIpSel] & 0x4) << 3) | ((ge_CmdDriving[eIpSel] & 0x4) << 2)
                             | ((ge_DataDriving[eIpSel] & 0x4) ? 0xF : 0)));

            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_SDPLL_BANK, 0x26), 0x3F);
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_SDPLL_BANK, 0x26),
                            (((ge_ClkDriving[eIpSel] & 0x8) << 2) | ((ge_CmdDriving[eIpSel] & 0x8) << 1)
                             | ((ge_DataDriving[eIpSel] & 0x8) ? 0xF : 0)));
#endif
            u8_PadClk     = PAD_SD0_CLK;
            u8_PadCmd     = PAD_SD0_CMD;
            u8_PadData[0] = PAD_SD0_D0;
            u8_PadData[1] = PAD_SD0_D1;
            u8_PadData[2] = PAD_SD0_D2;
            u8_PadData[3] = PAD_SD0_D3;
        }
        return;
    }
    else if (eIpSel == IP_SDIO) // gpio set by function
    {
        // get sd signal line pad
        if (ePadSel == PAD_SD_MD1)
        {
            u8_PadClk     = PAD_SD1_CLK;
            u8_PadCmd     = PAD_SD1_CMD;
            u8_PadData[0] = PAD_SD1_D0;
            u8_PadData[1] = PAD_SD1_D1;
            u8_PadData[2] = PAD_SD1_D2;
            u8_PadData[3] = PAD_SD1_D3;
        }
    }
    else if (eIpSel == IP_SDIO) // gpio set by function
    {
        // get sd signal line pad
        if (ePadSel == PAD_SD_MD2)
        {
            u8_PadClk     = PAD_I2S2_RX_DI;
            u8_PadCmd     = PAD_I2S3_RX_BCK;
            u8_PadData[0] = PAD_I2S2_RX_WCK;
            u8_PadData[1] = PAD_I2S2_RX_BCK;
            u8_PadData[2] = PAD_I2S3_RX_WCK;
            u8_PadData[3] = PAD_I2S3_RX_DI;
        }
    }
#endif

    // driving control switch
    if (ge_ClkDriving[eIpSel] != DRV_NOSET)
        MDrv_GPIO_Drv_Set(u8_PadClk, ge_ClkDriving[eIpSel] > u8_ClkDrvMax ? u8_ClkDrvMax : ge_ClkDriving[eIpSel]);

    if (ge_CmdDriving[eIpSel] != DRV_NOSET)
        MDrv_GPIO_Drv_Set(u8_PadCmd, ge_CmdDriving[eIpSel] > u8_CmdDrvMax ? u8_CmdDrvMax : ge_CmdDriving[eIpSel]);

    if (ge_CmdDriving[eIpSel] != DRV_NOSET)
    {
        for (i = 0; i < 4; i++)
            MDrv_GPIO_Drv_Set(u8_PadData[i],
                              ge_DataDriving[eIpSel] > u8_DataDrvMax ? u8_DataDrvMax : ge_DataDriving[eIpSel]);
    }
}

// Check clk and cmd line's driving control is setting by the same reg?
BOOL_T Hal_Check_ClkCmd_Interrelate(IpOrder eIP, PadOrder ePAD)
{
    return FALSE;
}

//***********************************************************************************************************
// Clock Setting for Card Platform
//***********************************************************************************************************
void Hal_CARD_SetClock(IpOrder eIP, U32_T u32ClkFromIPSet)
{
    IpSelect eIpSel = (IpSelect)eIP;

    if (eIpSel == IP_SD)
    {
        // [5:2]: Clk_Sel
        CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD), BIT05_T | BIT04_T | BIT03_T | BIT02_T);

#if defined(CONFIG_SUPPORT_SD30)
        if ((ge_BusTiming[eIP] == EV_BUS_SDR50) || (ge_BusTiming[eIP] == EV_BUS_SDR104)
            || (ge_BusTiming[eIP] == EV_BUS_HS200) || (ge_BusTiming[eIP] == EV_BUS_DDR50))
        {
            // select clk sd3.0 setting
            _PLTSD_PLL_1XClock_Setting(eIpSel, u32ClkFromIPSet);
            return;
        }
        CARD_REG(GET_CARD_REG_ADDR(A_CLKGEN_BANK, 0x44)) = 0x1400;
#endif

        switch (u32ClkFromIPSet)
        {
            case CLK1_D: // 48000KHz
                break;
            case CLK1_C:                                                                   // 43200KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD), BIT02_T); // 1
                break;
            case CLK1_B:                                                                   // 40000KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD), BIT03_T); // 2
                break;
            case CLK1_A:                                                                             // 36000KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD), BIT03_T | BIT02_T); // 3
                break;
            case CLK1_9:                                                                   // 32000KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD), BIT04_T); // 4
                break;
            case CLK1_8:                                                                             // 20000KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD), BIT04_T | BIT02_T); // 5
                break;
            case CLK1_7:                                                                             // 12000KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD), BIT04_T | BIT03_T); // 6
                break;
            case CLK1_6:                                                                                       // 300KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD), BIT04_T | BIT03_T | BIT02_T); // 7
                break;
        }
    }
    else if (eIpSel == IP_SDIO)
    {
        // [4:2]: Clk_Sel
        CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SDIO), BIT05_T | BIT04_T | BIT03_T | BIT02_T);

        switch (u32ClkFromIPSet)
        {
            case CLK2_F: // 48000KHz
                break;
            case CLK2_E:                                                                     // 43200KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SDIO), BIT02_T); // 1
                break;
            case CLK2_D:                                                                     // 40000KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SDIO), BIT03_T); // 2
                break;
            case CLK2_C:                                                                               // 36000KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SDIO), BIT03_T | BIT02_T); // 3
                break;
            case CLK2_B:                                                                     // 32000KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SDIO), BIT04_T); // 4
                break;
            case CLK2_A:                                                                               // 20000KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SDIO), BIT04_T | BIT02_T); // 5
                break;
            case CLK2_9:                                                                               // 12000KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SDIO), BIT04_T | BIT03_T); // 6
                break;
            case CLK2_8: // 300KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SDIO), BIT04_T | BIT03_T | BIT02_T); // 7
                break;
        }
    }
    else if (eIpSel == IP_SDIO2)
    {
        //[5:2]: Clk_Sel
        CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_CLKGEN2_BANK, REG_CLK_IP_SDIO2), BIT05_T | BIT04_T | BIT03_T | BIT02_T);

        switch (u32ClkFromIPSet)
        {
            case CLK2_F: // 48000KHz
                break;
            case CLK2_E:                                                                       // 43200KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN2_BANK, REG_CLK_IP_SDIO2), BIT02_T); // 1
                break;
            case CLK2_D:                                                                       // 40000KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN2_BANK, REG_CLK_IP_SDIO2), BIT03_T); // 2
                break;
            case CLK2_C:                                                                                 // 36000KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN2_BANK, REG_CLK_IP_SDIO2), BIT03_T | BIT02_T); // 3
                break;
            case CLK2_B:                                                                       // 32000KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN2_BANK, REG_CLK_IP_SDIO2), BIT04_T); // 4
                break;
            case CLK2_A:                                                                                 // 20000KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN2_BANK, REG_CLK_IP_SDIO2), BIT04_T | BIT02_T); // 5
                break;
            case CLK2_9:                                                                                 // 12000KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN2_BANK, REG_CLK_IP_SDIO2), BIT04_T | BIT03_T); // 6
                break;
            case CLK2_8: // 300KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN2_BANK, REG_CLK_IP_SDIO2), BIT04_T | BIT03_T | BIT02_T); // 7
                break;
        }
    }
}

U32_T Hal_CARD_FindClockSetting(IpOrder eIP, U32_T u32ReffClk)
{
    U8_T  u8LV             = 0;
    U32_T u32RealClk       = 0;
    U32_T u32ClkArr[3][16] = {{CLK1_F, CLK1_E, CLK1_D, CLK1_C, CLK1_B, CLK1_A, CLK1_9, CLK1_8, CLK1_7, CLK1_6, CLK1_5,
                               CLK1_4, CLK1_3, CLK1_2, CLK1_1, CLK1_0},
                              {CLK2_F, CLK2_E, CLK2_D, CLK2_C, CLK2_B, CLK2_A, CLK2_9, CLK2_8, CLK2_7, CLK2_6, CLK2_5,
                               CLK2_4, CLK2_3, CLK2_2, CLK2_1, CLK2_0},
                              {CLK3_F, CLK3_E, CLK3_D, CLK3_C, CLK3_B, CLK3_A, CLK3_9, CLK3_8, CLK3_7, CLK3_6, CLK3_5,
                               CLK3_4, CLK3_3, CLK3_2, CLK3_1, CLK3_0}};

    for (; u8LV < 16; u8LV++)
    {
        if ((u32ReffClk >= u32ClkArr[eIP][u8LV]) || (u8LV == 15) || (u32ClkArr[eIP][u8LV + 1] == 0))
        {
            u32RealClk = u32ClkArr[eIP][u8LV];
            break;
        }
    }

    return u32RealClk;
}

//***********************************************************************************************************
// Power and Voltage Setting for Card Platform
//***********************************************************************************************************
void Hal_CARD_PowerOn(IpOrder eIP, U16_T u16DelayMs)
{
    IpSelect eIpSel = (IpSelect)eIP;

    if (eIpSel >= IP_TOTAL)
    {
        pr_sd_err("sdmmc error ! [%s][%d]\n", __FUNCTION__, __LINE__);
        return;
    }

    if (gu32_SdPWR[eIP] == PAD_UNKNOWN)
    {
        // Maybe we don't use power pin.
        return;
    }

#if (GPIO_SET == GPIO_SET_BY_FUNC)

    // Whatever mdrv_padmux_active is ON or OFF, just do GPIO_set.
    MDrv_GPIO_Set_Low(gu32_SdPWR[eIP]);

#else
    switch (gu32_SdPWR[eIP])
    {
        case PAD_SD0_GPIO0:
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x10), BIT03_T);
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x10), BIT02_T);
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x10), BIT01_T);
            break;

        case PAD_SD1_GPIO1:
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x47), BIT03_T);
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x47), BIT02_T);
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x47), BIT01_T);
            break;

        default:
            pr_sd_err("sdmmc error ! [%s][%d]\n", __FUNCTION__, __LINE__);
            break;
    }
#endif

    Hal_Timer_mSleep(u16DelayMs);
    // Hal_Timer_mDelay(u16DelayMs);
}

void Hal_CARD_DumpPadMux(PadOrder ePAD)
{
    PadSelect ePadSel = (PadSelect)ePAD;

    if (ePadSel == PAD_SD_MD1)
    {
        printk("reg_allpad_in; reg[101EA1]#7=0b\n");
        printk("%8X\n", CARD_REG(GET_CARD_REG_ADDR(A_CHIPTOP_BANK, 0x50)));
        printk("reg_test_in_mode; reg[101E24]#1 ~ #0=0b\n");
        printk("reg_test_out_mode; reg[101E24]#5 ~ #4=0b\n");
        printk("%8X\n", CARD_REG(GET_CARD_REG_ADDR(A_CHIPTOP_BANK, 0x12)));
        printk("reg_spi0_mode; reg[101E18]#2 ~ #0=0b\n");
        printk("%8X\n", CARD_REG(GET_CARD_REG_ADDR(A_CHIPTOP_BANK, 0x0C)));
        printk("reg_fuart_mode; reg[101E06]#2 ~ #0=0b !=4,!=6\n");
        printk("%8X\n", CARD_REG(GET_CARD_REG_ADDR(A_CHIPTOP_BANK, 0x03)));
        printk("reg_sdio_mode; reg[101E11]#0=0b ==0x100\n");
        printk("%8X\n", CARD_REG(GET_CARD_REG_ADDR(A_CHIPTOP_BANK, 0x08)));
        printk("reg_pwm0_mode; reg[101E0E]#2 ~ #0=0b\n");
        printk("reg_pwm2_mode; reg[101E0F]#0 ~ #-2=0b\n");
        printk("%8X\n", CARD_REG(GET_CARD_REG_ADDR(A_CHIPTOP_BANK, 0x07)));
        printk("reg_i2s_mode; reg[101E1F]#3 ~ #2=0b\n");
        printk("reg_ttl_mode; reg[101E1E]#7 ~ #6=0b\n");
        printk("%8X\n", CARD_REG(GET_CARD_REG_ADDR(A_CHIPTOP_BANK, 0x0F)));
    }
}

void Hal_CARD_PowerOff(IpOrder eIP, U16_T u16DelayMs)
{
    IpSelect eIpSel = (IpSelect)eIP;

    if (eIpSel >= IP_TOTAL)
    {
        pr_sd_err("sdmmc error ! [%s][%d]\n", __FUNCTION__, __LINE__);
        return;
    }

    if (gu32_SdPWR[eIP] == PAD_UNKNOWN)
    {
        // Maybe we don't use power pin.
        return;
    }

#if (GPIO_SET == GPIO_SET_BY_FUNC)
    // Whatever mdrv_padmux_active is ON or OFF, just do GPIO_set.
    MDrv_GPIO_Set_High(gu32_SdPWR[eIP]);

#else
    switch (gu32_SdPWR[eIP])
    {
        case PAD_SD0_GPIO0:
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x10), BIT03_T);
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x10), BIT02_T);
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x10), BIT01_T);
            break;

        case PAD_SD1_GPIO1:
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x47), BIT03_T);
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x47), BIT02_T);
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x47), BIT01_T);
            break;

        default:
            pr_sd_err("sdmmc error ! [%s][%d]\n", __FUNCTION__, __LINE__);
            break;
    }
#endif

    Hal_Timer_mSleep(u16DelayMs);
    // Hal_Timer_mDelay(u16DelayMs);
}

#if defined(CONFIG_SUPPORT_SD30)
/*----------------------------------------------------------------------------------------------------------
 *
 * Function: Hal_CARD_SetPADVdd
 *     @author jeremy.wang (2018/1/29)
 * Desc:
 *
 * @param eIP : FCIE1/FCIE2/...
 * @param ePAD : PAD
 * @param ePADVdd :
 * @param u16DelayMs : Delay ms after set PAD power
 *
 * @return BOOL_T  :
 ----------------------------------------------------------------------------------------------------------*/
BOOL_T Hal_CARD_SetPADVdd(IpOrder eIP, PadOrder ePAD, PADVddEmType ePADVdd, U16_T u16DelayMs)
{
    BOOL_T bRet = FALSE;

    bRet = _PLTSD_PLL_Switch_AVDD(eIP, ePADVdd);

    if (ePAD == PAD_ORDER_0) // Pad SD0
    {
    }
    else if (ePAD == PAD_ORDER_1) // PAD_SD1
    {
    }

    Hal_Timer_mSleep(u16DelayMs);

    return bRet;
}
#endif

//***********************************************************************************************************
// Card Detect and GPIO Setting for Card Platform
//***********************************************************************************************************
void Hal_CARD_ConfigCdzPad(IpOrder eIP, U16_T nPadNum) // Hal_CARD_InitGPIO
{
    IpSelect eIpSel = (IpSelect)eIP;
    U16_T    nPadNo = nPadNum;

    if (eIpSel >= IP_TOTAL)
    {
        pr_sd_err("sdmmc error ! [%s][%d]\n", __FUNCTION__, __LINE__);
        return;
    }

    if (nPadNo == PAD_UNKNOWN)
    {
        // Save CDZ PadNum
        gu32_SdCDZ[(U16_T)eIpSel] = PAD_UNKNOWN;
        return;
    }

// PADMUX
#if (PADMUX_SET == PADMUX_SET_BY_FUNC)

    if (0 != mdrv_padmux_active())
    {
        MDrv_GPIO_PadGroupMode_Set(gu32_SdCdzMode[eIP]);
        MDrv_GPIO_Pad_Odn(gu32_SdCDZ[eIP]);
    }

#else

    switch (nPadNo)
    {
        case PAD_SD0_CDZ:
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x11), BIT03_T | BIT02_T);
            break;
        case PAD_SD1_CDZ:
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x48), BIT03_T | BIT02_T);
            break;
        case PAD_GPIO2:
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x31), BIT03_T | BIT02_T);
            break;

        default:
            pr_sd_err("sdmmc error ! [%s][%d]\n", __FUNCTION__, __LINE__);
            return;
            break;
    }
#endif

// GPIO
#if (GPIO_SET == GPIO_SET_BY_FUNC)
    // Whatever mdrv_padmux_active is ON or OFF, just set it to input mode.
    MDrv_GPIO_Pad_Odn(nPadNo);
#else

    switch (nPadNo)
    {
        case PAD_SD0_CDZ:
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x11), BIT03_T | BIT02_T);
            break;
        case PAD_SD1_CDZ:
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x48), BIT03_T | BIT02_T);
            break;
        case PAD_GPIO2:
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x31), BIT03_T | BIT02_T);
            break;

        default:
            pr_sd_err("sdmmc error ! [%s][%d]\n", __FUNCTION__, __LINE__);
            return;
            break;
    }
#endif
    gu32_SdCDZ[(U16_T)eIpSel] = nPadNo;
}

BOOL_T Hal_CARD_GetCdzState(IpOrder eIP) // Hal_CARD_GetGPIOState
{
    IpSelect eIpSel = (IpSelect)eIP;
    U8_T     nLv    = 0;

    if (eIpSel >= IP_TOTAL)
    {
        pr_sd_err("sdmmc error ! [%s][%d]\n", __FUNCTION__, __LINE__);
        goto fail;
    }

    if (gu32_SdCDZ[eIP] == PAD_UNKNOWN)
    {
        // Maybe we don't use CDZ pin.
        goto fail;
    }

#if (GPIO_SET == GPIO_SET_BY_FUNC)
    nLv = MDrv_GPIO_Pad_Read(gu32_SdCDZ[eIP]);
#else

    switch (gu32_SdCDZ[eIP])
    {
        case PAD_SD0_CDZ:
            nLv = CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x11)) & BIT00_T;
            break;
        case PAD_SD1_CDZ:
            nLv = CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO2_BANK, 0x48)) & BIT00_T;
            break;
        case PAD_GPIO2:
            nLv = CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x31)) & BIT00_T;
            break;

        default:
            pr_sd_err("sdmmc error ! [%s][%d]\n", __FUNCTION__, __LINE__);
            goto fail;
            break;
    }
#endif

    if (!nLv) // Low Active
    {
        return TRUE;
    }

fail:

    return FALSE;
}

U32_T Hal_CARD_CheckCdzMode(IpOrder eIP)
{
#if (PADMUX_SET == PADMUX_SET_BY_FUNC)
    if (gu32_SdCdzMode[eIP] == PINMUX_FOR_GPIO_MODE)
        return 0;
#endif

    return 1;
}

//***********************************************************************************************************
// MIU Setting for Card Platform
//***********************************************************************************************************

/*----------------------------------------------------------------------------------------------------------
 *
 * Function: Hal_CARD_TransMIUAddr
 *     @author jeremy.wang (2015/7/31)
 * Desc: Transfer original address to HW special dma address (MIU0/MIU1)
 *
 * @param u32Addr : Original address
 *
 * @return U32_T  : DMA address
 ----------------------------------------------------------------------------------------------------------*/
dma_addr_t Hal_CARD_TransMIUAddr(dma_addr_t ptr_Addr, U8_T *pu8MIUSel)
{
#if defined(PHYS_TO_MIU_USE_FUNC) && PHYS_TO_MIU_USE_FUNC
    return Chip_Phys_to_MIU(ptr_Addr);

#else
    U64 U64MiuBase = 0x20000000;
    if (ptr_Addr >= 0X1000000000)
        U64MiuBase = 0x1000000000;
    return ptr_Addr - U64MiuBase;

#endif
}
