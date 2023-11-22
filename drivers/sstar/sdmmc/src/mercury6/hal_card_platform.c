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

#if (PADMUX_SET == PADMUX_SET_BY_FUNC)
#include "mdrv_puse.h"
#endif

//***********************************************************************************************************
// Config Setting (Internal)
//***********************************************************************************************************
// Platform Register Basic Address
//------------------------------------------------------------------------------------
#define A_CLKGEN_BANK  GET_CARD_REG_ADDR(A_RIU_BASE, 0x81C00) // 1038h
#define A_SPI_SYNTHPLL GET_CARD_REG_ADDR(A_RIU_BASE, 0x81C80) // 1039h
#define A_MPLL_BANK    GET_CARD_REG_ADDR(A_RIU_BASE, 0x81800) // 1030h
#define A_PADTOP_BANK  GET_CARD_REG_ADDR(A_RIU_BASE, 0x81E00) // 103Ch
//#define A_GPI_INT_BANK      GET_CARD_REG_ADDR(A_RIU_BASE, 0x81E80)//103Dh
#define A_PM_SLEEP_BANK GET_CARD_REG_ADDR(A_RIU_BASE, 0x00700) // 0Eh
#define A_PM_GPIO_BANK  GET_CARD_REG_ADDR(A_RIU_BASE, 0x00780) // 0Fh
#define A_CHIPTOP_BANK  GET_CARD_REG_ADDR(A_RIU_BASE, 0x80F00) // 101Eh
//#define A_MCM_SC_GP_BANK    GET_CARD_REG_ADDR(A_RIU_BASE, 0x89900)//1132h
#define A_SC_GP_CTRL_BANK GET_CARD_REG_ADDR(A_RIU_BASE, 0x89980) // 1133h
#define A_SDPLL_BANK      GET_CARD_REG_ADDR(A_RIU_BASE, 0xA0C80) // Bank: 0x1419
#define A_PADGPIO_BANK    GET_CARD_REG_ADDR(A_RIU_BASE, 0x81F00) // Bank: 0x103E
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
#else
#define CLK1_F 48000000
#define CLK1_E 43200000
#define CLK1_D 40000000
#define CLK1_C 36000000
#define CLK1_B 32000000
#define CLK1_A 20000000
#define CLK1_9 12000000
#define CLK1_8 300000
#define CLK1_7 0
#define CLK1_6 0
#define CLK1_5 0
#define CLK1_4 0
#define CLK1_3 0
#define CLK1_2 0
#define CLK1_1 0
#define CLK1_0 0
#endif

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

#define REG_CLK_IP_SD   (0x43)
#define REG_CLK_IP_SDIO (0x45)

// Bonding ID
//----------------------------------------------------------------------------------------------------------
#define BOND_SSM613D 0x00 // QFN  1G
#define BOND_SSM613Q 0x01 // QFN  2G
#define BOND_SSM616Q 0x31 // LQFN 2G
#define BOND_SSM633Q 0x11 // BGA1 2G
#define BOND_SSM650G 0x27 // BGA2

#define pr_sd_err(fmt, arg...) printk(fmt, ##arg)

static volatile U16_T _gu16PowerPadNumForEachIp[IP_TOTAL] = {PAD_UNKNOWN};
static volatile U16_T _gu16CdzPadNumForEachIp[IP_TOTAL]   = {PAD_UNKNOWN};
static U16_T          BondingID                           = 0;

extern U32_T gu32_EnClkPhase[3];
extern U32_T gu32_TXClkPhase[3];
extern U32_T gu32_RXClkPhase[3];

#if defined(CONFIG_SUPPORT_SD30)
extern volatile BusTimingEmType ge_BusTiming[3];

static void _CARD_PLL_Auto_Turn_ONOFF_LDO(IpSelect eIP)
{
    if (eIP == IP_SD)
    {
        // 1. Disable OSP : FALSE =>(Enable)
        CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_SD_PLL_POS(eIP), 0x1D), BIT15_T);

        // 2. Set OSP counter[15:8] = 0x30
        CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_SD_PLL_POS(eIP), 0x37),
                        BIT15_T | BIT14_T | BIT13_T | BIT12_T | BIT11_T | BIT10_T | BIT09_T | BIT08_T);
        CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_SD_PLL_POS(eIP), 0x37), BIT13_T | BIT12_T);

        // 3. Turning on LDO  1->0
        CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_SD_PLL_POS(eIP), 0x37), BIT05_T);
        Hal_Timer_mSleep(10);
        CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_SD_PLL_POS(eIP), 0x37), BIT05_T);
    }
    else if (eIP == IP_SDIO)
    {
        // 1. Turning off LDO  0->1
        CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_SD_PLL_POS(eIP), 0x37), BIT05_T);
        Hal_Timer_mSleep(10);
        CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_SD_PLL_POS(eIP), 0x37), BIT05_T);
    }
}

static BOOL_T _PLTSD_PLL_Switch_AVDD(IpOrder eIP, PADVddEmType ePADVdd)
{
    U16_T u16Reg    = 1;
    U8_T  u8Timeout = 0;

    if (ePADVdd == EV_NORVOL)
        CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_SD_PLL_POS(eIP), 0x37), BIT04_T | BIT02_T); // Switch to 3.3V
    else if (ePADVdd == EV_LOWVOL)
        CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_SD_PLL_POS(eIP), 0x37), BIT04_T | BIT02_T); // Switch to 1.8V

    do
    {
        if (ePADVdd == EV_NORVOL)
            u16Reg = CARD_REG(GET_CARD_REG_ADDR(A_SD_PLL_POS(eIP), 0x10)) & BIT12_T; // Check 3.3v Ready
        else if (ePADVdd == EV_LOWVOL)
            u16Reg = CARD_REG(GET_CARD_REG_ADDR(A_SD_PLL_POS(eIP), 0x10)) & BIT13_T; // Check 1.8v Ready

        if (u16Reg)
            break;

        u8Timeout++;
        Hal_Timer_mSleep(1);

    } while (u8Timeout < 20);

    if (u8Timeout >= 20)
    {
        if (ePADVdd == EV_NORVOL)
            printk(">>>>> Over Time during 3.3v setting\r\n");
        else if (ePADVdd == EV_LOWVOL)
            printk(">>>>> Over Time during 1.8v setting\r\n");

        return 1;
    }

    return 0;
}

static void _PLTSD_PLL_1XClock_Setting(IpSelect eIP, U32_T u32ClkFromIPSet)
{
    U8_T u8PDIV_REG = BIT00_T;

    //(1) reset emmc pll
    CARD_REG_SETBIT(A_PLL_RESET_PLL_REG(eIP), BIT00_T);
    CARD_REG_CLRBIT(A_PLL_RESET_PLL_REG(eIP), BIT00_T);

    //(2) synth clock
    switch (u32ClkFromIPSet)
    {
        case CLK1_F: // 200000KHz

            // CARD_REG(A_PLL_DFFSET_23_16_REG(eIP)) = 0x22;
            // CARD_REG(A_PLL_DFFSET_15_0_REG(eIP)) = 0x8F5C;

            // 195MHz
            CARD_REG(A_PLL_DFFSET_23_16_REG(eIP)) = 0x24;
            CARD_REG(A_PLL_DFFSET_15_0_REG(eIP))  = 0x03D8;
            u8PDIV_REG                            = BIT00_T; // 1
            break;

        case CLK1_E: // 100000KHz
            CARD_REG(A_PLL_DFFSET_23_16_REG(eIP)) = 0x45;
            CARD_REG(A_PLL_DFFSET_15_0_REG(eIP))  = 0x1EB8;
            u8PDIV_REG                            = BIT00_T; // 1
            break;

        case CLK1_D: // 48000MHz //DDR

            // The same with 195MHz with 8-bit macro
            CARD_REG(A_PLL_DFFSET_23_16_REG(eIP)) = 0x24;
            CARD_REG(A_PLL_DFFSET_15_0_REG(eIP))  = 0xD944;
            u8PDIV_REG                            = BIT00_T; // 1

            break;
    }

    //(3) VCO clock
    CARD_REG(A_PLL_FBDIV_REG(eIP)) = 0x06;

    //(4) 1X clock
    CARD_REG_CLRBIT(A_PLL_PDIV_REG(eIP), BIT02_T | BIT01_T | BIT00_T);
    CARD_REG_SETBIT(A_PLL_PDIV_REG(eIP), u8PDIV_REG);

    CARD_REG_CLRBIT(A_PLL_TEST_REG(eIP), BIT10_T); // 0

    Hal_Timer_uDelay(100);
}
#endif

//***********************************************************************************************************
// IP Setting for Card Platform
//***********************************************************************************************************
void Hal_CARD_IPOnceSetting(IpOrder eIP)
{
    IpSelect eIpSel = (IpSelect)eIP;

    BondingID = CARD_REG(GET_CARD_REG_ADDR(A_CHIPTOP_BANK, 0x48)) & ~(0x08);
#if (FORCE_SWITCH_PAD)
    // reg_all_pad_in => Close
    CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_CHIPTOP_BANK, 0x50), BIT15_T);
#endif
#if defined(CONFIG_SUPPORT_SD30)
    _CARD_PLL_Auto_Turn_ONOFF_LDO(eIpSel);
#endif
    // Clock Source
    if (eIpSel == IP_SD)
    {
        CARD_REG(GET_CARD_REG_ADDR(A_SDPLL_BANK, 0x1A)) = 0x00;          // reg_emmc_test
        CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_SDPLL_BANK, 0x1A), BIT10_T); // set this bit to enable clk for m6

        CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_SC_GP_CTRL_BANK, 0x25),
                        BIT07_T); // BK:x1133(sc_gp_ctrl)[B7] [0:1]/[boot_clk(12M):sd_clk]
        CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD),
                        BIT06_T | BIT05_T | BIT04_T | BIT03_T | BIT02_T | BIT01_T | BIT00_T);
        CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD),
                        BIT06_T); // BK:x1038(reg_ckg_sd)[B5] [0:1]/[boot_clk(12M):sd_clk]
    }
    else if (eIpSel == IP_SDIO)
    {
        CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_SC_GP_CTRL_BANK, 0x25), BIT03_T);
        CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SDIO),
                        BIT06_T | BIT05_T | BIT04_T | BIT03_T | BIT02_T | BIT01_T | BIT00_T);
        CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SDIO), BIT06_T);
    }
}

//***********************************************************************************************************
// PAD Setting for Card Platform
//***********************************************************************************************************
void Hal_CARD_InitPADPin(IpOrder eIP, PadOrder ePAD)
{
    IpSelect  eIpSel  = (IpSelect)eIP;
    PadSelect ePadSel = (PadSelect)ePAD;

    if (eIpSel == IP_SD)
    {
        if ((ePadSel == PAD_SD_MD1) || (ePadSel == PAD_SD_MD2))
        {
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x00), BIT04_T | BIT05_T); // CDZ
            // reg_sd0_pe:D3, D2, D1, D0, CMD=> pull en
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x01), BIT04_T); // D1
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x02), BIT04_T); // D0
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x04), BIT04_T); // CMD
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x05), BIT04_T); // D3
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x06), BIT04_T); // D2

            // reg_sd0_drv: CLK, D3, D2, D1, D0, CMD => drv: 1 for vCore 0.9V->0.85V
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x01), BIT07_T); // D1
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x02), BIT07_T); // D0
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x03), BIT07_T); // CLK
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x04), BIT07_T); // CMD
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x05), BIT07_T); // D3
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x06), BIT07_T); // D2
        }
    }
    else if (eIpSel == IP_SDIO)
    {
        if (ePadSel == PAD_SD_MD1)
        {
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7B), BIT04_T | BIT05_T); // CDZ
            // reg_sd0_pe:D3, D2, D1, D0, CMD=> pull en
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7C), BIT04_T); // D1
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7D), BIT04_T); // D0
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7F), BIT04_T); // CMD
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x00), BIT04_T);  // D3
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x01), BIT04_T);  // D2

            // reg_sd0_drv: CLK, D3, D2, D1, D0, CMD => drv: 1 for vCore 0.9V->0.85V
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7C), BIT07_T); // D1
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7D), BIT07_T); // D0
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7E), BIT07_T); // CLK
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7F), BIT07_T); // CMD
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x00), BIT07_T);  // D3
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x01), BIT07_T);  // D2
        }
        else if (ePadSel == PAD_SD_MD2)
        {
            // reg_sd1_pe:D3, D2, D1, D0, CMD=> pull en
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x10), BIT04_T); // D1
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x0F), BIT04_T); // D0
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x08), BIT04_T); // CMD
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x09), BIT04_T); // D3
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x0A), BIT04_T); // D2

            // reg_sd1_drv: CLK, D3, D2, D1, D0, CMD => drv: 1 for vCore 0.9V->0.85V
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x10), BIT07_T); // D1
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x0F), BIT07_T); // D0
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x07), BIT07_T); // CLK
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x08), BIT07_T); // CMD
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x09), BIT07_T); // D3
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x0A), BIT07_T); // D2
        }
        else if (ePadSel == PAD_SD_MD3)
        {
            // reg_sd1_pe:D3, D2, D1, D0, CMD=> pull en
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x47), BIT04_T); // D1
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x48), BIT04_T); // D0
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x4A), BIT04_T); // CMD
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x4B), BIT04_T); // D3
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x4C), BIT04_T); // D2

            // reg_sd1_drv: CLK, D3, D2, D1, D0, CMD => drv: 1 for vCore 0.9V->0.85V
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x47), BIT07_T); // D1
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x48), BIT07_T); // D0
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x49), BIT07_T); // CLK
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x4A), BIT07_T); // CMD
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x4B), BIT07_T); // D3
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x4C), BIT07_T); // D2
        }
        else if (ePadSel == PAD_SD_MD4)
        {
            if (BondingID == BOND_SSM616Q)                                                   // LQFP
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0X55), BIT04_T | BIT05_T); // CDZ
            else                                                                             // QFN
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x3E), BIT04_T | BIT05_T); // CDZ
            // reg_sd1_pe:D3, D2, D1, D0, CMD=> pull en
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x56), BIT04_T); // D1
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x57), BIT04_T); // D0
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x59), BIT04_T); // CMD
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x5A), BIT04_T); // D3
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x5B), BIT04_T); // D2

            // reg_sd1_drv: CLK, D3, D2, D1, D0, CMD => drv: 1 for vCore 0.9V->0.85V
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x56), BIT07_T); // D1
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x57), BIT07_T); // D0
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x58), BIT07_T); // CLK
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x59), BIT07_T); // CMD
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x5A), BIT07_T); // D3
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x5B), BIT07_T); // D2
        }
        else if (ePadSel == PAD_SD_MD5)
        {
            // reg_sd1_pe:D3, D2, D1, D0, CMD=> pull en
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x13), BIT04_T); // D1
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x14), BIT04_T); // D0
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x16), BIT04_T); // CMD
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x17), BIT04_T); // D3
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x18), BIT04_T); // D2

            // reg_sd1_drv: CLK, D3, D2, D1, D0, CMD => drv: 1 for vCore 0.9V->0.85V
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x13), BIT07_T); // D1
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x14), BIT07_T); // D0
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x15), BIT07_T); // CLK
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x16), BIT07_T); // CMD
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x17), BIT07_T); // D3
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x18), BIT07_T); // D2
        }
    }

    // Pull Down
    Hal_CARD_PullPADPin(eIP, ePAD, EV_PULLDOWN);
}

BOOL_T Hal_CARD_GetPadInfoCdzPad(IpOrder eIP, U32_T *nPadNum)
{
#if (PADMUX_SET == PADMUX_SET_BY_FUNC)
    int puse;

    if (eIP == IP_ORDER_0)
    {
        puse = MDRV_PUSE_SDIO0_CDZ;
    }
    else if (eIP == IP_ORDER_1)
    {
        puse = MDRV_PUSE_SDIO1_CDZ;
    }
    else
    {
        pr_sd_err("sdmmc error ! [%s][%d]\n", __FUNCTION__, __LINE__);

        *nPadNum = PAD_UNKNOWN;
        return FALSE;
    }

    *nPadNum = mdrv_padmux_getpad(puse);
    return TRUE;
#else
    return TRUE;
#endif
}

BOOL_T Hal_CARD_GetPadInfoPowerPad(IpOrder eIP, U32_T *nPadNum)
{
#if (PADMUX_SET == PADMUX_SET_BY_FUNC)
    int puse;

    if (eIP == IP_ORDER_0)
    {
        puse = MDRV_PUSE_SDIO0_PWR;
    }
    else if (eIP == IP_ORDER_1)
    {
        puse = MDRV_PUSE_SDIO1_PWR;
    }
    else
    {
        pr_sd_err("sdmmc error ! [%s][%d]\n", __FUNCTION__, __LINE__);

        *nPadNum = PAD_UNKNOWN;
        return FALSE;
    }

    *nPadNum = mdrv_padmux_getpad(puse);
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

    if (0 == mdrv_padmux_active())
    {
        if (eIpSel == IP_SD)
        {
            if (ePadSel == PAD_SD_MD1)
            {
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD0_MODE_1);
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD0_CDZ_MODE_1);
            }
            else if (ePadSel == PAD_SD_MD2)
            {
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD0_MODE_2);
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD0_CDZ_MODE_1);
            }
        }
        else if (eIpSel == IP_SDIO)
        {
            if (ePadSel == PAD_SD_MD1)
            {
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD1_MODE_1);
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD1_CDZ_MODE_1);
            }
            else if (ePadSel == PAD_SD_MD2)
            {
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD1_MODE_2);
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD1_CDZ_MODE_2);
            }
            else if (ePadSel == PAD_SD_MD3)
            {
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD1_MODE_3);
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD1_CDZ_MODE_3);
            }
            else if (ePadSel == PAD_SD_MD4)
            {
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD1_MODE_4);
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD1_CDZ_MODE_4);
            }
            else if (ePadSel == PAD_SD_MD5)
            {
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD1_MODE_5);
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD1_CDZ_MODE_5);
            }
        }
    }

#else //

    if (eIpSel == IP_SD)
    {
        if (ePadSel == PAD_SD_MD1)
        {
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67),
                            BIT14_T | BIT13_T | BIT12_T | BIT10_T | BIT09_T | BIT08_T);
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x68), BIT14_T | BIT13_T | BIT12_T);
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67),
                            BIT08_T | BIT10_T); //[B8:B10]/[reg_sd0_mode:reg_sd0_cdz_mode]

            // Make sure reg_spi0_mode != 3
            if ((GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x68) & (BIT02_T | BIT01_T | BIT00_T)) == (BIT01_T | BIT00_T))
            {
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x68), BIT02_T | BIT01_T | BIT00_T);
            }
        }
    }
    else if (eIpSel == IP_SDIO)
    {
        if (ePadSel == PAD_SD_MD1)
        {
            // SD mode
            // OFF:x67 [B8]reg_sd0_mode [B9]reg_sd0_cdz_mode [B13:B12]reg_sd1_mode
            // OFF:x68 [B9:B8]reg_sd1_cdz_mode
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT12_T);
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x68), BIT12_T);
        }
        else if (ePadSel == PAD_SD_MD4)
        {
            // SD mode
            // OFF:x67 [B8]reg_sd0_mode [B9]reg_sd0_cdz_mode [B13:B12]reg_sd1_mode
            // OFF:x68 [B9:B8]reg_sd1_cdz_mode
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT14_T);
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x68), BIT14_T);
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
        _gu16PowerPadNumForEachIp[(U16_T)eIpSel] = PAD_UNKNOWN;
        return;
    }

#if (PADMUX_SET == PADMUX_SET_BY_FUNC)

    if (0 == mdrv_padmux_active())
    {
        MDrv_GPIO_PadVal_Set(nPadNo, PINMUX_FOR_GPIO_MODE);
    }

#else

    switch (nPadNo)
    {
        case PAD_PM_GPIO0:
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PM_PAD_TOP_BANK, 0x00), (BIT02_T | BIT01_T | BIT00_T));
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PM_PAD_TOP_BANK, 0x00), BIT03_T);
            break;

        case PAD_TTL7:
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x54), (BIT02_T | BIT01_T | BIT00_T));

            break;

        case PAD_SD_GPIO0:
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x78), (BIT02_T | BIT01_T | BIT00_T));
            break;

        case PAD_SD1_GPIO0:
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x79), (BIT02_T | BIT01_T | BIT00_T));
            break;

        default:
            pr_sd_err("sdmmc error ! [%s][%d]\n", __FUNCTION__, __LINE__);
            return;
            break;
    }

#endif

    // Save Power PadNum
    _gu16PowerPadNumForEachIp[(U16_T)eIpSel] = nPadNo;

    // Default power off
    Hal_CARD_PowerOff(eIP, 0);
}

void Hal_CARD_PullPADPin(IpOrder eIP, PadOrder ePAD, PinPullEmType ePinPull)
{
    IpSelect  eIpSel  = (IpSelect)eIP;
    PadSelect ePadSel = (PadSelect)ePAD;

    // Whatever if using padmux dts, we need to switch the pad mode.

    // IP_SD
    if (eIpSel == IP_SD)
    {
        if (ePadSel == PAD_SD_MD1 || ePadSel == PAD_SD_MD2)
        {
            if (ePinPull == EV_PULLDOWN)
            {
                // reg_sd0_pe: D3, D2, D1, D0, CMD=> pull dis
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x01), BIT04_T); // D1
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x02), BIT04_T); // D0
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x04), BIT04_T); // CMD
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x05), BIT04_T); // D3
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x06), BIT04_T); // D2

                // PAD -> GPIO mode
#if (PADMUX_SET == PADMUX_SET_BY_FUNC)
                MDrv_GPIO_PadVal_Set(PAD_SD0_CLK, PINMUX_FOR_GPIO_MODE);
                MDrv_GPIO_PadVal_Set(PAD_SD0_CMD, PINMUX_FOR_GPIO_MODE);
                MDrv_GPIO_PadVal_Set(PAD_SD0_D0, PINMUX_FOR_GPIO_MODE);
                MDrv_GPIO_PadVal_Set(PAD_SD0_D1, PINMUX_FOR_GPIO_MODE);
                MDrv_GPIO_PadVal_Set(PAD_SD0_D2, PINMUX_FOR_GPIO_MODE);
                MDrv_GPIO_PadVal_Set(PAD_SD0_D3, PINMUX_FOR_GPIO_MODE);
#else
                // OFF:x67 [B8:B9]reg_sd0_mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT08_T | BIT09_T);
#endif

                // Output Low
#if (GPIO_SET == GPIO_SET_BY_FUNC)
                MDrv_GPIO_Set_Low(PAD_SD0_CLK);
                MDrv_GPIO_Set_Low(PAD_SD0_CMD);
                MDrv_GPIO_Set_Low(PAD_SD0_D0);
                MDrv_GPIO_Set_Low(PAD_SD0_D1);
                MDrv_GPIO_Set_Low(PAD_SD0_D2);
                MDrv_GPIO_Set_Low(PAD_SD0_D3);
#else
                // SD_ClK
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x03), BIT02_T); // reg_sd0_gpio_oen_4
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x03), BIT01_T); // reg_sd0_gpio_out_4

                // SD_CMD
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x04), BIT02_T); // reg_sd0_gpio_oen_5
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x04), BIT01_T); // reg_sd0_gpio_out_5

                // SD_D0
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x02), BIT02_T); // reg_sd0_gpio_oen_3
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x02), BIT01_T); // reg_sd0_gpio_out_3

                // SD_D1
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x01), BIT02_T); // reg_sd0_gpio_oen_2
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x01), BIT01_T); // reg_sd0_gpio_out_2

                // SD_D2
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x06), BIT02_T); // reg_sd0_gpio_oen_7
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x06), BIT01_T); // reg_sd0_gpio_out_7

                // SD_D3
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x05), BIT02_T); // reg_sd0_gpio_oen_6
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x05), BIT01_T); // reg_sd0_gpio_out_6
#endif
            }
            else if (ePinPull == EV_PULLUP)
            {
                // reg_sd0_pe:D3, D2, D1, D0, CMD=> pull en
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x01), BIT04_T); // D1
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x02), BIT04_T); // D0
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x04), BIT04_T); // CMD
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x05), BIT04_T); // D3
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x06), BIT04_T); // D2

                // Input
#if (GPIO_SET == GPIO_SET_BY_FUNC)
                MDrv_GPIO_Pad_Odn(PAD_SD0_CLK);
                MDrv_GPIO_Pad_Odn(PAD_SD0_CMD);
                MDrv_GPIO_Pad_Odn(PAD_SD0_D0);
                MDrv_GPIO_Pad_Odn(PAD_SD0_D1);
                MDrv_GPIO_Pad_Odn(PAD_SD0_D2);
                MDrv_GPIO_Pad_Odn(PAD_SD0_D3);
#else
                // SD_CLK
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x03), BIT02_T); // reg_sd0_gpio_oen_4

                // SD_CMD
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x04), BIT02_T); // reg_sd0_gpio_oen_5

                // SD_D0
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x02), BIT02_T); // reg_sd0_gpio_oen_3

                // SD_D1
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x01), BIT02_T); // reg_sd0_gpio_oen_2

                // SD_D2
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x06), BIT02_T); // reg_sd0_gpio_oen_7

                // SD_D3
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x05), BIT02_T); // reg_sd0_gpio_oen_6
#endif
                // SD Mode
#if (PADMUX_SET == PADMUX_SET_BY_FUNC)
                if (ePadSel == PAD_SD_MD1)
                    MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD0_MODE_1);
                else
                    MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD0_MODE_2);
#else
                if (ePadSel == PAD_SD_MD1)
                {
                    // OFF:x67 [B8:B9]reg_sd0_mode [B10]reg_sd0_cdz_mode
                    CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT08_T);
                }
                else
                {
                    // OFF:x67 [B8:B9]reg_sd0_mode [B10]reg_sd0_cdz_mode
                    CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT09_T);
                }
#endif
            }
        }
    }

    // IP_SDIO
    if (eIpSel == IP_SDIO)
    {
        if (ePadSel == PAD_SD_MD1)
        {
            if (ePinPull == EV_PULLDOWN)
            {
                // D3, D2, D1, D0, CMD=> pull dis
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7C), BIT04_T); // D1
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7D), BIT04_T); // D0
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7F), BIT04_T); // CMD
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x00), BIT04_T);  // D3
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x01), BIT04_T);  // D2

                // PAD -> GPIO mode
#if (PADMUX_SET == PADMUX_SET_BY_FUNC)
                MDrv_GPIO_PadVal_Set(PAD_SD1_CLK, PINMUX_FOR_GPIO_MODE); // CLK
                MDrv_GPIO_PadVal_Set(PAD_SD1_CMD, PINMUX_FOR_GPIO_MODE); // CMD
                MDrv_GPIO_PadVal_Set(PAD_SD1_D0, PINMUX_FOR_GPIO_MODE);  // D0
                MDrv_GPIO_PadVal_Set(PAD_SD1_D1, PINMUX_FOR_GPIO_MODE);  // D1
                MDrv_GPIO_PadVal_Set(PAD_SD1_D2, PINMUX_FOR_GPIO_MODE);  // D2
                MDrv_GPIO_PadVal_Set(PAD_SD1_D3, PINMUX_FOR_GPIO_MODE);  // D3
#else
                // OFF:x67 [B14:B12]reg_sd1_mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT12_T | BIT13_T | BIT14_T);
#endif

                // Output Low
#if (GPIO_SET == GPIO_SET_BY_FUNC)
                MDrv_GPIO_Set_Low(PAD_SD1_CLK);
                MDrv_GPIO_Set_Low(PAD_SD1_CMD);
                MDrv_GPIO_Set_Low(PAD_SD1_D0);
                MDrv_GPIO_Set_Low(PAD_SD1_D1);
                MDrv_GPIO_Set_Low(PAD_SD1_D2);
                MDrv_GPIO_Set_Low(PAD_SD1_D3);
#else
                // SD_ClK
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7E), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7E), BIT01_T); // output:0

                // SD_CMD
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7F), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7F), BIT01_T); // output:0

                // SD_D0
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7D), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7D), BIT01_T); // output:0

                // SD_D1
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7C), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7C), BIT01_T); // output:0

                // SD_D2
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x01), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x01), BIT01_T); // output:0

                // SD_D3
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x00), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x00), BIT01_T); // output:0
#endif
            }
            else if (ePinPull == EV_PULLUP)
            {
                // D3, D2, D1, D0, CMD=> pull en
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7C), BIT04_T); // D1
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7D), BIT04_T); // D0
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7F), BIT04_T); // CMD
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x00), BIT04_T);  // D3
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x01), BIT04_T);  // D2

                // Input
#if (GPIO_SET == GPIO_SET_BY_FUNC)
                MDrv_GPIO_Pad_Odn(PAD_SD1_CLK);
                MDrv_GPIO_Pad_Odn(PAD_SD1_CMD);
                MDrv_GPIO_Pad_Odn(PAD_SD1_D0);
                MDrv_GPIO_Pad_Odn(PAD_SD1_D1);
                MDrv_GPIO_Pad_Odn(PAD_SD1_D2);
                MDrv_GPIO_Pad_Odn(PAD_SD1_D3);
#else
                // SD_CLK
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7E), BIT02_T); // input mode

                // SD_CMD
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7F), BIT02_T); // input mode

                // SD_D0
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7D), BIT02_T); // input mode

                // SD_D1
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7C), BIT02_T); // input mode

                // SD_D2
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x01), BIT02_T); // input mode

                // SD_D3
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x00), BIT02_T); // input mode
#endif
                // SD Mode
#if (PADMUX_SET == PADMUX_SET_BY_FUNC)
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD1_MODE_1);
#else
                // OFF:x67 [B14:B12]reg_sd1_mode
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT12_T);
#endif
            }
        }
        else if (ePadSel == PAD_SD_MD2)
        {
            if (ePinPull == EV_PULLDOWN)
            {
                // D3, D2, D1, D0, CMD=> pull dis
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x10), BIT04_T); // D1
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x0F), BIT04_T); // D0
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x08), BIT04_T); // CMD
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x09), BIT04_T); // D3
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x0A), BIT04_T); // D2

                // PAD -> GPIO mode
#if 0 //(PADMUX_SET == PADMUX_SET_BY_FUNC)
                MDrv_GPIO_PadVal_Set(PAD_FUART_RX, PINMUX_FOR_GPIO_MODE);  // CLK
                MDrv_GPIO_PadVal_Set(PAD_FUART_TX, PINMUX_FOR_GPIO_MODE);   // CMD
                MDrv_GPIO_PadVal_Set(PAD_SPI0_DO, PINMUX_FOR_GPIO_MODE);  // D0
                MDrv_GPIO_PadVal_Set(PAD_SPI0_DI, PINMUX_FOR_GPIO_MODE); // D1
                MDrv_GPIO_PadVal_Set(PAD_FUART_RTS, PINMUX_FOR_GPIO_MODE);  // D2
                MDrv_GPIO_PadVal_Set(PAD_FUART_CTS, PINMUX_FOR_GPIO_MODE);  // D3
#else
                // OFF:x67 [B14:B12]reg_sd1_mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT12_T | BIT13_T | BIT14_T);
#endif

                // Output Low
#if (GPIO_SET == GPIO_SET_BY_FUNC)
                MDrv_GPIO_Set_Low(PAD_FUART_RX);
                MDrv_GPIO_Set_Low(PAD_FUART_TX);
                MDrv_GPIO_Set_Low(PAD_FUART_CTS);
                MDrv_GPIO_Set_Low(PAD_FUART_RTS);
                MDrv_GPIO_Set_Low(PAD_SPI0_DO);
                MDrv_GPIO_Set_Low(PAD_SPI0_DI);
#else
                // SD_ClK
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x07), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x07), BIT01_T); // output:0

                // SD_CMD
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x08), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x08), BIT01_T); // output:0

                // SD_D0
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x0F), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x0F), BIT01_T); // output:0

                // SD_D1
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x10), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x10), BIT01_T); // output:0

                // SD_D2
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x0A), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x0A), BIT01_T); // output:0

                // SD_D3
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x09), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x09), BIT01_T); // output:0
#endif
            }
            else if (ePinPull == EV_PULLUP)
            {
                // D3, D2, D1, D0, CMD=> pull en
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x10), BIT04_T); // D1
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x0F), BIT04_T); // D0
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x08), BIT04_T); // CMD
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x09), BIT04_T); // D3
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x0A), BIT04_T); // D2

                // Input
#if (GPIO_SET == GPIO_SET_BY_FUNC)
                MDrv_GPIO_Pad_Odn(PAD_FUART_RX);
                MDrv_GPIO_Pad_Odn(PAD_FUART_TX);
                MDrv_GPIO_Pad_Odn(PAD_FUART_CTS);
                MDrv_GPIO_Pad_Odn(PAD_FUART_RTS);
                MDrv_GPIO_Pad_Odn(PAD_SPI0_DO);
                MDrv_GPIO_Pad_Odn(PAD_SPI0_DI);
#else
                // SD_CLK
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x07), BIT02_T); // input mode
                // SD_CMD
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x08), BIT02_T); // input mode
                // SD_D0
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x0F), BIT02_T); // input mode
                // SD_D1
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x10), BIT02_T); // input mode
                // SD_D2
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x0A), BIT02_T); // input mode
                // SD_D3
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x09), BIT02_T); // input mode
#endif
                // SD Mode
#if (PADMUX_SET == PADMUX_SET_BY_FUNC)
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD1_MODE_2);
#else
                // OFF:x67 [B14:B12]reg_sd1_mode
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT13_T);
#endif
            }
        }
        else if (ePadSel == PAD_SD_MD3)
        {
            if (ePinPull == EV_PULLDOWN)
            {
                // D3, D2, D1, D0, CMD=> pull dis
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x47), BIT04_T); // D1
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x48), BIT04_T); // D0
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x4A), BIT04_T); // CMD
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x4B), BIT04_T); // D3
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x4C), BIT04_T); // D2

                // PAD -> GPIO mode
#if 0 //(PADMUX_SET == PADMUX_SET_BY_FUNC)
                MDrv_GPIO_PadVal_Set(PAD_TX0_IO06, PINMUX_FOR_GPIO_MODE);  // CLK
                MDrv_GPIO_PadVal_Set(PAD_TX0_IO07, PINMUX_FOR_GPIO_MODE);   // CMD
                MDrv_GPIO_PadVal_Set(PAD_TX0_IO05, PINMUX_FOR_GPIO_MODE);  // D0
                MDrv_GPIO_PadVal_Set(PAD_TX0_IO04, PINMUX_FOR_GPIO_MODE); // D1
                MDrv_GPIO_PadVal_Set(PAD_TX0_IO09, PINMUX_FOR_GPIO_MODE);  // D2
                MDrv_GPIO_PadVal_Set(PAD_TX0_IO08, PINMUX_FOR_GPIO_MODE);  // D3
#else
                // OFF:x67 [B14:B12]reg_sd1_mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT12_T | BIT13_T | BIT14_T);
#endif

                // Output Low
#if (GPIO_SET == GPIO_SET_BY_FUNC)
                MDrv_GPIO_Set_Low(PAD_TX0_IO04);
                MDrv_GPIO_Set_Low(PAD_TX0_IO05);
                MDrv_GPIO_Set_Low(PAD_TX0_IO06);
                MDrv_GPIO_Set_Low(PAD_TX0_IO07);
                MDrv_GPIO_Set_Low(PAD_TX0_IO08);
                MDrv_GPIO_Set_Low(PAD_TX0_IO09);
#else
                // SD_ClK
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x49), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x49), BIT01_T); // output:0

                // SD_CMD
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x4A), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x4A), BIT01_T); // output:0

                // SD_D0
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x48), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x48), BIT01_T); // output:0

                // SD_D1
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x47), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x47), BIT01_T); // output:0

                // SD_D2
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x4C), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x4C), BIT01_T); // output:0

                // SD_D3
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x4B), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x4B), BIT01_T); // output:0
#endif
            }
            else if (ePinPull == EV_PULLUP)
            {
                // D3, D2, D1, D0, CMD=> pull en
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x47), BIT04_T); // D1
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x48), BIT04_T); // D0
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x4A), BIT04_T); // CMD
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x4B), BIT04_T); // D3
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x4C), BIT04_T); // D2

                // Input
#if (GPIO_SET == GPIO_SET_BY_FUNC)
                MDrv_GPIO_Pad_Odn(PAD_TX0_IO04);
                MDrv_GPIO_Pad_Odn(PAD_TX0_IO05);
                MDrv_GPIO_Pad_Odn(PAD_TX0_IO06);
                MDrv_GPIO_Pad_Odn(PAD_TX0_IO07);
                MDrv_GPIO_Pad_Odn(PAD_TX0_IO08);
                MDrv_GPIO_Pad_Odn(PAD_TX0_IO09);
#else
                // SD_CLK
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x49), BIT02_T); // input mode
                // SD_CMD
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x4A), BIT02_T); // input mode
                // SD_D0
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x48), BIT02_T); // input mode
                // SD_D1
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x47), BIT02_T); // input mode
                // SD_D2
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x4C), BIT02_T); // input mode
                // SD_D3
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x4B), BIT02_T); // input mode
#endif
                // SD Mode
#if (PADMUX_SET == PADMUX_SET_BY_FUNC)
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD1_MODE_3);
#else
                // OFF:x67 [B14:B12]reg_sd1_mode
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT13_T | BIT12_T);
#endif
            }
        }
        else if (ePadSel == PAD_SD_MD4)
        {
            if (ePinPull == EV_PULLDOWN)
            {
                // D3, D2, D1, D0, CMD=> pull dis
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x56), BIT04_T); // D1
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x57), BIT04_T); // D0
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x59), BIT04_T); // CMD
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x5A), BIT04_T); // D3
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x5B), BIT04_T); // D2

                // PAD -> GPIO mode
#if (PADMUX_SET == PADMUX_SET_BY_FUNC)
                MDrv_GPIO_PadVal_Set(PAD_TTL11, PINMUX_FOR_GPIO_MODE); // CLK
                MDrv_GPIO_PadVal_Set(PAD_TTL12, PINMUX_FOR_GPIO_MODE); // CMD
                MDrv_GPIO_PadVal_Set(PAD_TTL10, PINMUX_FOR_GPIO_MODE); // D0
                MDrv_GPIO_PadVal_Set(PAD_TTL9, PINMUX_FOR_GPIO_MODE);  // D1
                MDrv_GPIO_PadVal_Set(PAD_TTL13, PINMUX_FOR_GPIO_MODE); // D2
                MDrv_GPIO_PadVal_Set(PAD_TTL14, PINMUX_FOR_GPIO_MODE); // D3
#else
                // OFF:x67 [B14:B12]reg_sd1_mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT12_T | BIT13_T | BIT14_T);
#endif

                // Output Low
#if (GPIO_SET == GPIO_SET_BY_FUNC)
                MDrv_GPIO_Set_Low(PAD_TTL9);
                MDrv_GPIO_Set_Low(PAD_TTL10);
                MDrv_GPIO_Set_Low(PAD_TTL11);
                MDrv_GPIO_Set_Low(PAD_TTL12);
                MDrv_GPIO_Set_Low(PAD_TTL13);
                MDrv_GPIO_Set_Low(PAD_TTL14);
#else
                // SD_ClK
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x58), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x58), BIT01_T); // output:0

                // SD_CMD
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x59), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x59), BIT01_T); // output:0

                // SD_D0
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x57), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x57), BIT01_T); // output:0

                // SD_D1
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x56), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x56), BIT01_T); // output:0

                // SD_D2
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x5B), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x5B), BIT01_T); // output:0

                // SD_D3
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x5A), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x5A), BIT01_T); // output:0
#endif
            }
            else if (ePinPull == EV_PULLUP)
            {
                // D3, D2, D1, D0, CMD=> pull en
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x56), BIT04_T); // D1
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x57), BIT04_T); // D0
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x59), BIT04_T); // CMD
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x5A), BIT04_T); // D3
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x5B), BIT04_T); // D2

                // Input
#if (GPIO_SET == GPIO_SET_BY_FUNC)
                MDrv_GPIO_Pad_Odn(PAD_TTL9);
                MDrv_GPIO_Pad_Odn(PAD_TTL10);
                MDrv_GPIO_Pad_Odn(PAD_TTL11);
                MDrv_GPIO_Pad_Odn(PAD_TTL12);
                MDrv_GPIO_Pad_Odn(PAD_TTL13);
                MDrv_GPIO_Pad_Odn(PAD_TTL14);
#else
                // SD_CLK
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x58), BIT02_T); // input mode
                // SD_CMD
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x59), BIT02_T); // input mode
                // SD_D0
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x57), BIT02_T); // input mode
                // SD_D1
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x56), BIT02_T); // input mode
                // SD_D2
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x5B), BIT02_T); // input mode
                // SD_D3
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x5A), BIT02_T); // input mode
#endif
                // SD Mode
#if (PADMUX_SET == PADMUX_SET_BY_FUNC)
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD1_MODE_4);
#else
                // OFF:x67 [B14:B12]reg_sd1_mode
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT14_T);
#endif
            }
        }
        else if (ePadSel == PAD_SD_MD5)
        {
            if (ePinPull == EV_PULLDOWN)
            {
                // D3, D2, D1, D0, CMD=> pull dis
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x13), BIT04_T); // D1
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x14), BIT04_T); // D0
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x16), BIT04_T); // CMD
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x17), BIT04_T); // D3
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x18), BIT04_T); // D2

                // PAD -> GPIO mode
#if 0 //(PADMUX_SET == PADMUX_SET_BY_FUNC)
                MDrv_GPIO_PadVal_Set(PAD_GPIO2, PINMUX_FOR_GPIO_MODE);  // CLK
                MDrv_GPIO_PadVal_Set(PAD_GPIO3, PINMUX_FOR_GPIO_MODE);   // CMD
                MDrv_GPIO_PadVal_Set(PAD_GPIO1, PINMUX_FOR_GPIO_MODE);  // D0
                MDrv_GPIO_PadVal_Set(PAD_GPIO0, PINMUX_FOR_GPIO_MODE); // D1
                MDrv_GPIO_PadVal_Set(PAD_GPIO5, PINMUX_FOR_GPIO_MODE);  // D2
                MDrv_GPIO_PadVal_Set(PAD_GPIO4, PINMUX_FOR_GPIO_MODE);  // D3
#else
                // OFF:x67 [B14:B12]reg_sd1_mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT12_T | BIT13_T | BIT14_T);
#endif

                // Output Low
#if (GPIO_SET == GPIO_SET_BY_FUNC)
                MDrv_GPIO_Set_Low(PAD_GPIO0);
                MDrv_GPIO_Set_Low(PAD_GPIO1);
                MDrv_GPIO_Set_Low(PAD_GPIO2);
                MDrv_GPIO_Set_Low(PAD_GPIO3);
                MDrv_GPIO_Set_Low(PAD_GPIO4);
                MDrv_GPIO_Set_Low(PAD_GPIO5);
#else
                // SD_ClK
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x15), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x15), BIT01_T); // output:0

                // SD_CMD
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x16), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x16), BIT01_T); // output:0

                // SD_D0
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x14), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x14), BIT01_T); // output:0

                // SD_D1
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x13), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x13), BIT01_T); // output:0

                // SD_D2
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x18), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x18), BIT01_T); // output:0

                // SD_D3
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x17), BIT02_T); // output mode
                CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x17), BIT01_T); // output:0
#endif
            }
            else if (ePinPull == EV_PULLUP)
            {
                // D3, D2, D1, D0, CMD=> pull en
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x13), BIT04_T); // D1
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x14), BIT04_T); // D0
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x16), BIT04_T); // CMD
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x17), BIT04_T); // D3
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x18), BIT04_T); // D2

                // Input
#if (GPIO_SET == GPIO_SET_BY_FUNC)
                MDrv_GPIO_Pad_Odn(PAD_GPIO0);
                MDrv_GPIO_Pad_Odn(PAD_GPIO1);
                MDrv_GPIO_Pad_Odn(PAD_GPIO2);
                MDrv_GPIO_Pad_Odn(PAD_GPIO3);
                MDrv_GPIO_Pad_Odn(PAD_GPIO4);
                MDrv_GPIO_Pad_Odn(PAD_GPIO5);
#else
                // SD_CLK
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x15), BIT02_T); // input mode
                // SD_CMD
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x16), BIT02_T); // input mode
                // SD_D0
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x14), BIT02_T); // input mode
                // SD_D1
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x13), BIT02_T); // input mode
                // SD_D2
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x18), BIT02_T); // input mode
                // SD_D3
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x17), BIT02_T); // input mode
#endif
                // SD Mode
#if (PADMUX_SET == PADMUX_SET_BY_FUNC)
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD1_MODE_5);
#else
                // OFF:x67 [B14:B12]reg_sd1_mode
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT14_T | BIT12_T);
#endif
            }
        }
    }
}

//***********************************************************************************************************
// Clock Setting for Card Platform
//***********************************************************************************************************
void Hal_CARD_SetClock(IpOrder eIP, U32_T u32ClkFromIPSet)
{
    IpSelect eIpSel = (IpSelect)eIP;

    if (eIpSel == IP_SD)
    {
        if (gu32_EnClkPhase[eIP])
        {
            CARD_REG(GET_CARD_REG_ADDR(A_MPLL_BANK, 0x1)) = 0;

            CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x0))  = 0;
            CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x1))  = 0;
            CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x3))  = 0;
            CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x4))  = 0;
            CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x33)) = 0;
            CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x34)) = 0x2;
            CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x35)) = 0;
            CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x37)) = 0;
            CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x36)) = 0;

            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD),
                            BIT14_T | BIT13_T | BIT12_T | BIT11_T | BIT10_T | BIT09_T | BIT08_T | BIT07_T);
        }
        CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD),
                        BIT06_T | BIT05_T | BIT04_T | BIT03_T | BIT02_T | BIT01_T
                            | BIT00_T); //[6]Boot_Sel [5:2]: Clk_Sel [1]: Clk_i [0]: Clk_g
        CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD), BIT06_T); // boot sel
        CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_SC_GP_CTRL_BANK, 0x25),
                        BIT07_T); //[B3/B7]:[SDIO/SD] select BOOT clock source (glitch free) - 0: select BOOT clock
                                  // 12MHz (xtali), 1: select FCIE/SPI clock source

#if defined(CONFIG_SUPPORT_SD30)
        if ((ge_BusTiming[eIP] == EV_BUS_SDR50) || (ge_BusTiming[eIP] == EV_BUS_SDR104)
            || (ge_BusTiming[eIP] == EV_BUS_HS200) || (ge_BusTiming[eIP] == EV_BUS_DDR50))
        {
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, 0x44), BIT15_T | BIT14_T | BIT13_T | BIT12_T);
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, 0x44), BIT14_T);

            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD),
                            BIT05_T | BIT04_T | BIT03_T); // select clk_sd30_1x_p

            _PLTSD_PLL_1XClock_Setting(eIpSel, u32ClkFromIPSet);

            return;
        }
#endif

        switch (u32ClkFromIPSet)
        {
#if defined(CONFIG_SUPPORT_SD30)
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
#else
            case CLK1_F: // 48000KHz
                if (gu32_EnClkPhase[eIP])
                {
                    CARD_REG(GET_CARD_REG_ADDR(A_MPLL_BANK, 0x1)) = 0;

                    CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x0))  = 0;
                    CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x1))  = 0x48;
                    CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x33)) = 0;
                    CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x34)) = 0x3;
                    CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x35)) = 0;
                    CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x37)) = 0;
                    CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x36)) = 0;

                    //[b5]:select clk_spi_synth_pll, [b8]:enable tx phase [b13]:enable rx phase
                    CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD),
                                    BIT05_T | BIT08_T | BIT13_T | (gu32_TXClkPhase[eIP] << 9)
                                        | (gu32_RXClkPhase[eIP] << 11));
                }
                break;
            case CLK1_E:                                                                   // 43200KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD), BIT02_T); // 1
                break;
            case CLK1_D:                                                                   // 40000KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD), BIT03_T); // 2
                break;
            case CLK1_C:                                                                             // 36000KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD), BIT03_T | BIT02_T); // 3
                break;
            case CLK1_B:                                                                   // 32000KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD), BIT04_T); // 4
                break;
            case CLK1_A:                                                                             // 20000KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD), BIT04_T | BIT02_T); // 5
                break;
            case CLK1_9:                                                                             // 12000KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD), BIT04_T | BIT03_T); // 6
                break;
            case CLK1_8:                                                                                       // 300KHz
                CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD), BIT04_T | BIT03_T | BIT02_T); // 7
                break;
#endif
        }
    }
    else if (eIpSel == IP_SDIO)
    {
        if (gu32_EnClkPhase[eIP])
        {
            CARD_REG(GET_CARD_REG_ADDR(A_MPLL_BANK, 0x1)) = 0;

            CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x0))  = 0;
            CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x1))  = 0;
            CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x3))  = 0;
            CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x4))  = 0;
            CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x33)) = 0;
            CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x34)) = 0x2;
            CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x35)) = 0;
            CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x37)) = 0;
            CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x36)) = 0;

            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SDIO),
                            BIT14_T | BIT13_T | BIT12_T | BIT11_T | BIT10_T | BIT09_T | BIT08_T | BIT07_T);
        }
        CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SDIO),
                        BIT06_T | BIT05_T | BIT04_T | BIT03_T | BIT02_T | BIT01_T
                            | BIT00_T); //[5]Boot_Sel [4:2]: Clk_Sel [1]: Clk_i [0]: Clk_g
        CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SDIO), BIT06_T); // boot sel
        CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_SC_GP_CTRL_BANK, 0x25),
                        BIT03_T); //[B3/B7]:[SDIO/SD] select BOOT clock source (glitch free) - 0: select BOOT clock
                                  // 12MHz (xtali), 1: select FCIE/SPI clock source

        switch (u32ClkFromIPSet)
        {
            case CLK2_F: // 48000KHz
                if (gu32_EnClkPhase[eIP])
                {
                    CARD_REG(GET_CARD_REG_ADDR(A_MPLL_BANK, 0x1)) = 0;

                    CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x0))  = 0;
                    CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x1))  = 0x48;
                    CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x33)) = 0;
                    CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x34)) = 0x3;
                    CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x35)) = 0;
                    CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x37)) = 0;
                    CARD_REG(GET_CARD_REG_ADDR(A_SPI_SYNTHPLL, 0x36)) = 0;

                    //[b5]:select clk_spi_synth_pll, [b8]:enable tx phase [b13]:enable rx phase
                    CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SDIO),
                                    BIT05_T | BIT08_T | BIT13_T | (gu32_TXClkPhase[eIP] << 9)
                                        | (gu32_RXClkPhase[eIP] << 11));
                }
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
}

#ifdef CONFIG_PM_SLEEP
//***********************************************************************************************************
// Get pm clock from Card Platform
//***********************************************************************************************************
void Hal_CARD_devpm_GetClock(IpOrder eIP, U32_T *pu32PmIPClk, U32_T *pu32PmBlockClk)
{
    IpSelect eIpSel = (IpSelect)eIP;

    *pu32PmBlockClk = CARD_REG(GET_CARD_REG_ADDR(A_SC_GP_CTRL_BANK, 0x25));

    if (eIpSel == IP_SD)
    {
        *pu32PmIPClk = CARD_REG(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SD));
    }
    else if (eIpSel == IP_SDIO)
    {
        *pu32PmIPClk = CARD_REG(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SDIO));
    }
}

//***********************************************************************************************************
// Set pm clock to Card Platform
//***********************************************************************************************************
void Hal_CARD_devpm_setClock(IpOrder eIP, U32_T u32PmIPClk, U32_T u32PmBlockClk)
{
    IpSelect eIpSel = (IpSelect)eIP;

    Hal_CARD_IPOnceSetting(eIP);

    CARD_REG(GET_CARD_REG_ADDR(A_SC_GP_CTRL_BANK, 0x25)) = u32PmBlockClk;

    if (eIpSel == IP_SD)
    {
#if defined(CONFIG_SUPPORT_SD30)
        Hal_CARD_SetPADVdd(eIP, 0, EV_NORVOL, 1);
#endif
    }
    else if (eIpSel == IP_SDIO)
    {
        CARD_REG(GET_CARD_REG_ADDR(A_CLKGEN_BANK, REG_CLK_IP_SDIO)) = u32PmIPClk;
    }
}
#endif

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
    U16_T    nPadNo = 0;

    if (eIpSel >= IP_TOTAL)
    {
        pr_sd_err("sdmmc error ! [%s][%d]\n", __FUNCTION__, __LINE__);
        return;
    }

    nPadNo = _gu16PowerPadNumForEachIp[(U16_T)eIpSel];

    if (nPadNo == PAD_UNKNOWN)
    {
        // Maybe we don't use power pin.
        return;
    }

#if (GPIO_SET == GPIO_SET_BY_FUNC)

    // Whatever mdrv_padmux_active is ON or OFF, just do GPIO_set.
    MDrv_GPIO_Set_Low(nPadNo);

#else

    switch (nPadNo)
    {
        case PAD_SD_GPIO0:
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x78), BIT02_T); // enable output mode
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x78), BIT01_T); // output low
            break;

        case PAD_SD1_GPIO0:
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x79), BIT02_T); // enable output mode
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x79), BIT01_T); // output low
            break;

        case PAD_TTL7:
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x54), BIT02_T); // enable output mode
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x54), BIT01_T); // output low
            break;

        case PAD_PM_GPIO0:
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PM_PAD_TOP_BANK, 0x00), BIT02_T); // enable output mode
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PM_PAD_TOP_BANK, 0x00), BIT01_T); // output low
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
    U16_T    nPadNo = 0;

    if (eIpSel >= IP_TOTAL)
    {
        pr_sd_err("sdmmc error ! [%s][%d]\n", __FUNCTION__, __LINE__);
        return;
    }

    nPadNo = _gu16PowerPadNumForEachIp[(U16_T)eIpSel];

    if (nPadNo == PAD_UNKNOWN)
    {
        // Maybe we don't use power pin.
        return;
    }

#if (GPIO_SET == GPIO_SET_BY_FUNC)

    // Whatever mdrv_padmux_active is ON or OFF, just do GPIO_set.
    MDrv_GPIO_Set_High(nPadNo);

#else

    switch (nPadNo)
    {
        case PAD_SD_GPIO0:
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x78), BIT02_T); // enable output mode
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x78), BIT01_T); // output high
            break;

        case PAD_SD1_GPIO0:
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x79), BIT02_T); // enable output mode
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x79), BIT01_T); // output high
            break;

        case PAD_TTL7:
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x54), BIT02_T); // enable output mode
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x54), BIT01_T); // output high
            break;

        case PAD_PM_GPIO0:
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PM_PAD_TOP_BANK, 0x00), BIT02_T); // enable output mode
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PM_PAD_TOP_BANK, 0x00), BIT01_T); // output high
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
        _gu16CdzPadNumForEachIp[(U16_T)eIpSel] = PAD_UNKNOWN;
        return;
    }

// PADMUX
#if (PADMUX_SET == PADMUX_SET_BY_FUNC)

    if (0 == mdrv_padmux_active())
    {
        switch (nPadNo)
        {
            case PAD_SD0_CDZ:
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD0_CDZ_MODE_1);
                break;

            case PAD_SD1_CDZ:
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD1_CDZ_MODE_1);
                break;

            case PAD_SPI0_CZ:
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD1_CDZ_MODE_2);
                break;

            case PAD_GPIO16:
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD1_CDZ_MODE_3);
                break;

            case PAD_SR1_IO15:
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD1_CDZ_MODE_4);
                break;

            case PAD_TTL8:
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD1_CDZ_MODE_4);
                break;

            case PAD_SR1_IO17:
                MDrv_GPIO_PadGroupMode_Set(PINMUX_FOR_SD1_CDZ_MODE_5);
                break;

            default:
                pr_sd_err("sdmmc error ! [%s][%d]\n", __FUNCTION__, __LINE__);
                return;
                break;
        }
    }

#else

    switch (nPadNo)
    {
        case PAD_SD0_CDZ:
            // OFF:x67 [B8:B9]reg_sd0_mode [B10]reg_sd0_cdz_mode
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x67), BIT10_T);
            break;

        case PAD_SD1_CDZ:
            // OFF:x68 [B14:B12]reg_sd1_cdz_mode
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x68), BIT14_T | BIT13_T | BIT12_T);
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x68), BIT12_T);
            break;

        case PAD_TTL8:
            // OFF:x68 [B14:B12]reg_sd1_cdz_mode
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x68), BIT14_T | BIT13_T | BIT12_T);
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADTOP_BANK, 0x68), BIT14_T);
            break;

        case PAD_SR1_IO15:
            // OFF:x68 [B14:B12]reg_sd1_cdz_mode
            CARD_REG_CLRBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x3E), BIT14_T | BIT13_T | BIT12_T);
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x3E), BIT14_T);
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
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x00), BIT02_T); // PAD_SD0_CDZ:reg_sd0_gpio_oen_1
            break;

        case PAD_SD1_CDZ:
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7B), BIT02_T); // PAD_SD1_CDZ:reg_sd1_gpio_oen_0
            break;

        case PAD_SR1_IO15:
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x3E), BIT02_T); // PAD_TTL8:reg_sd1_gpio_oen_0
            break;

        case PAD_TTL8:
            CARD_REG_SETBIT(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x55), BIT02_T); // PAD_TTL8:reg_sd1_gpio_oen_0
            break;

        default:
            pr_sd_err("sdmmc error ! [%s][%d]\n", __FUNCTION__, __LINE__);
            return;
            break;
    }

#endif

    // Save CDZ PadNum
    _gu16CdzPadNumForEachIp[(U16_T)eIpSel] = nPadNo;
}

BOOL_T Hal_CARD_GetCdzState(IpOrder eIP) // Hal_CARD_GetGPIOState
{
    IpSelect eIpSel = (IpSelect)eIP;
    U16_T    nPadNo = 0;
    U8_T     nLv    = 0;

    if (eIpSel >= IP_TOTAL)
    {
        pr_sd_err("sdmmc error ! [%s][%d]\n", __FUNCTION__, __LINE__);
        goto fail;
    }

    nPadNo = _gu16CdzPadNumForEachIp[(U16_T)eIpSel];

    if (nPadNo == PAD_UNKNOWN)
    {
        // Maybe we don't use CDZ pin.
        goto fail;
    }

#if (GPIO_SET == GPIO_SET_BY_FUNC)
    nLv = MDrv_GPIO_Pad_Read(nPadNo);
#else

    switch (nPadNo)
    {
        case PAD_SD0_CDZ:
            nLv = CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x00)) & BIT00_T;
            break;

        case PAD_SD1_CDZ:
            nLv = CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x7B)) & BIT00_T;
            break;

        case PAD_TTL8:
            nLv = CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x55)) & BIT00_T;
            break;

        case PAD_SR1_IO15:
            nLv = CARD_REG(GET_CARD_REG_ADDR(A_PADGPIO_BANK, 0x3E)) & BIT00_T;
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
    return ptr_Addr - 0x20000000;
}
