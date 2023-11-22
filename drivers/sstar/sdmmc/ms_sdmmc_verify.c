/*
 * ms_sdmmc_verify.c- Sigmastar
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
 * FileName ms_sdmmc_verify.c
 *     @author jeremy.wang (2015/10/01)
 * Desc:
 *     This layer is between UBOOT Common API layer and SDMMC Driver layer.
 *     (1) The goal is we could modify any verification flow but don't modify any sdmmc driver code.
 *     (2) Timer Test, PAD Test, Init Test, CIFD/DMA Test, Burning Test
 *
 ***************************************************************************************************************/

#include "inc/ms_sdmmc_verify.h"
#include "inc/ms_sdmmc_drv.h"
#include "inc/hal_card_timer.h"
#include "inc/hal_sdmmc_v5.h"
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include "inc/hal_card_platform.h"

//***********************************************************************************************************
// Config Setting (Internel)
//***********************************************************************************************************
#define D_DUMPCOMP (TRUE)

#define BURN_START_SECTOR 30000
#define BURN_END_SECTOR   70000

#define BURN_CLK_CHG_CNT 120
#define BURN_CLK_CHG_LVL 3

#define BURN_PAT_CHG_CNT 20
#define BURN_PAT_CHG_LVL 5

static U8_T  gu8Pattern = 0;
static U16_T gu16Ret1 = 0, gu16Ret2 = 0, gu16Ret3 = 0;

#define D_TIMERTEST    "(SDMMC TimerTest)   "
#define D_SETPAD       "(SDMMC SetPAD)      "
#define D_DAT1DET      "(SDMMC DAT1Det)     "
#define D_CARDDET      "(SDMMC CardDet)     "
#define D_SDMMCINIT    "(SDMMC Init)        "
#define D_CIFDWRITE    "(SDMMC CIFD W)      "
#define D_CIFDREAD     "(SDMMC CIFD R)      "
#define D_DMAWRITE     "(SDMMC DMA W)       "
#define D_DMAREAD      "(SDMMC DMA R)       "
#define D_ADMAWRITE    "(SDMMC ADMA W)      "
#define D_ADMAREAD     "(SDMMC ADMA R)      "
#define D_COMPARE      "(COMPARE CONTENT)   "
#define D_WIDEBUS      "(SDMMC SetWideBus)  "
#define D_HIGHBUS      "(SDMMC SetHighBus)  "
#define D_SETBUSCLK    "(SDMMC SetBusClock) "
#define D_SETBUSTIMING "(SDMMC SetBusTiming)"
#define D_TESTPATT     "(SDMMC TestPattern) "
#define D_PSINIT       "(SDMMC PS Init)     "
#define D_PSTEST       "(SDMMC PS Test)     "

#define MAX_BLK_SIZE  512  // Maximum Transfer Block Size
#define MAX_BLK_COUNT 1024 // Maximum Transfer Block Count
#define MAX_SEG_CNT   128

#if 1
#define prtstring(fmt, arg...) printk(KERN_CONT fmt, ##arg)
#else
#define prtstring(fmt, arg...)
#endif
static U8_T *dma_test_buf0, *dma_test_buf1;
dma_addr_t   dma_test_addr0, dma_test_addr1;

static AdmaDescStruct *AdmaScripts;
dma_addr_t             AdmaScriptsAddr;

//----------------------------------------------------------------------------------------------------------
void _PRT_LINE(BOOL_T bHidden)
{
    if (!bHidden)
        prtstring("\r\n========================================================================================\r\n");
}

//----------------------------------------------------------------------------------------------------------
void _PRT_ENTER(BOOL_T bHidden)
{
    if (!bHidden)
        prtstring("\r\n");
}

//----------------------------------------------------------------------------------------------------------
void _PRT_MSG(U8_T u8Slot, char arrTestName[], U16_T u16Err, BOOL_T bHidden)
{
    if (u16Err)
    {
        prtstring("[sdmmc_%d]  %s ........ (FAIL)= %04x\n", u8Slot, arrTestName, u16Err);
    }
    else if (!bHidden)
    {
        prtstring("[sdmmc_%d]  %s ........ (PASS)\n", u8Slot, arrTestName);
    }
}

void _PRT_VALUE(U8_T u8Slot, char arrTestName[], U32_T u32Value, BOOL_T bHidden)
{
    if (!bHidden)
    {
        prtstring("[sdmmc_%d]  %s ........  %08x\n", u8Slot, arrTestName, u32Value);
    }
}

void _DUMP_MEM_ARR(volatile U8_T *u8Arr, U16_T u16Size)
{
    unsigned short u16Pos;

    prtstring("\r\n---------------------------------------------------------------------------------------------\r\n");

    for (u16Pos = 0; u16Pos < u16Size; u16Pos++)
    {
        if (u16Pos % 16 == 0 && u16Pos != 0)
            prtstring("\r\n");

        prtstring("(%02x)", u8Arr[u16Pos]);
    }
    prtstring("\r\n----------------------------------------------------------------------------------------------\r\n");
}

U16_T _MEM_COMPARE(volatile U8_T *u8Arr1, volatile U8_T *u8Arr2, U16_T u16Size)
{
    U16_T u16Pos;

    for (u16Pos = 0; u16Pos < u16Size; u16Pos++)
    {
        if (u8Arr1[u16Pos] != u8Arr2[u16Pos])
            return 1;
    }

    return 0;
}

void _SET_MEM_PATTERN(volatile U8_T *u8Arr, U16_T u16Size, U8_T u8Pattern)
{
    U16_T u16Pos = 0;
    U8_T  ctmp   = 0;
    U8_T  change = 0;

    if (u8Pattern == 0)
    {
        ctmp = 0;
        for (u16Pos = 0; u16Pos < u16Size; u16Pos++)
        {
            if (u16Pos % 16 == 0 && u16Pos != 0)
                ctmp++;

            u8Arr[u16Pos] = (unsigned char)ctmp;
        }
    }
    else if (u8Pattern == 1)
    {
        ctmp = 0;
        for (u16Pos = 0; u16Pos < u16Size; u16Pos++)
        {
            u8Arr[u16Pos] = (unsigned char)ctmp;
            ctmp++;
        }
    }
    else if (u8Pattern == 2)
    {
        ctmp = 0xFF;
        for (u16Pos = 0; u16Pos < u16Size; u16Pos++)
        {
            u8Arr[u16Pos] = (unsigned char)ctmp;
            ctmp--;
        }
    }
    else if (u8Pattern == 3)
    {
        for (u16Pos = 0; u16Pos < u16Size; u16Pos++)
        {
            if ((u16Pos % 0x100) == 0)
            {
                if (change)
                {
                    ctmp   = 0xFF;
                    change = 0;
                }
                else
                {
                    ctmp   = 0x00;
                    change = 1;
                }
            }

            u8Arr[u16Pos] = (unsigned char)ctmp;

            if (change)
                ctmp++;
            else
                ctmp--;
        }
    }
    else if (u8Pattern == 4) // 0xFF, 0x00
    {
        ctmp = 0xFF;
        for (u16Pos = 0; u16Pos <= u16Size; u16Pos++)
        {
            u8Arr[u16Pos] = (unsigned char)ctmp;

            if (ctmp == 0)
                ctmp = 0xFF;
            else
                ctmp = 0x0;
        }
    }
    else if (u8Pattern == 5) // 0x55, 0xAA
    {
        ctmp = 0x55;
        for (u16Pos = 0; u16Pos <= u16Size; u16Pos++)
        {
            u8Arr[u16Pos] = (unsigned char)ctmp;

            if (ctmp == 0xAA)
                ctmp = 0x55;
            else
                ctmp = 0xAA;
        }
    }
}

/*----------------------------------------------------------------------------------------------------------
*
* Function: IPV_SDMMC_TimerTest
*     @author jeremy.wang (2015/10/13)
* Desc: Timer Test for IP Verification
*
* @param u8Sec : Seconds
----------------------------------------------------------------------------------------------------------*/
void IPV_SDMMC_TimerTest(U8_T u8Sec)
{
    U8_T u8CurSec;

    _PRT_LINE(FALSE);

    _PRT_MSG(0, D_TIMERTEST, 0, FALSE);
    prtstring("-->");

    for (u8CurSec = 0; (u8CurSec < u8Sec) && (u8CurSec <= 10); u8CurSec++)
    {
        prtstring("(@)");
        Hal_Timer_mDelay(1000);
    }

    _PRT_LINE(FALSE);
}

/*----------------------------------------------------------------------------------------------------------
*
* Function: IPV_SDMMC_CardDetect
*     @author jeremy.wang (2015/10/1)
* Desc: Card Detection for IP Verification
*
* @param u8Slot : Slot ID
----------------------------------------------------------------------------------------------------------*/
void IPV_SDMMC_CardDetect(U8_T u8Slot)
{
    _PRT_LINE(FALSE);

    gu16Ret1 = !SDMMC_CardDetect(u8Slot);
    _PRT_MSG(u8Slot, D_CARDDET, gu16Ret1, FALSE);

    _PRT_LINE(FALSE);
}

/*----------------------------------------------------------------------------------------------------------
*
* Function: IPV_SDMMC_Init
*     @author jeremy.wang (2015/10/1)
* Desc: SDMMC Init for IP Verification
*
* @param u8Slot : Slot ID
----------------------------------------------------------------------------------------------------------*/
void IPV_SDMMC_Init(U8_T u8Slot)
{
    _PRT_LINE(FALSE);

    gu16Ret1 = SDMMC_Init(u8Slot);
    _PRT_MSG(u8Slot, D_SDMMCINIT, gu16Ret1, FALSE);

    _PRT_LINE(FALSE);
}

/*----------------------------------------------------------------------------------------------------------
*
* Function: IPV_SDMMC_SetWideBus
*     @author jeremy.wang (2015/10/6)
* Desc: Set Wide Bus (4Bits) for IP Verification
*
* @param u8Slot : Slot ID
----------------------------------------------------------------------------------------------------------*/
void IPV_SDMMC_SetWideBus(U8_T u8Slot)
{
    _PRT_LINE(FALSE);

    gu16Ret1 = SDMMC_SetWideBus(u8Slot);
    _PRT_MSG(u8Slot, D_WIDEBUS, gu16Ret1, FALSE);

    _PRT_LINE(FALSE);
}

/*----------------------------------------------------------------------------------------------------------
*
* Function: IPV_SDMMC_SetHighBus
*     @author jeremy.wang (2015/10/13)
* Desc: Set HighSpeed Bus for IP Verification
*
* @param u8Slot : Slot ID
----------------------------------------------------------------------------------------------------------*/
void IPV_SDMMC_SetHighBus(U8_T u8Slot)
{
    _PRT_LINE(FALSE);

    gu16Ret1 = SDMMC_SwitchHighBus(u8Slot);
    _PRT_MSG(u8Slot, D_HIGHBUS, gu16Ret1, FALSE);

    _PRT_LINE(FALSE);
}

/*----------------------------------------------------------------------------------------------------------
*
* Function: IPV_SDMMC_SetClock
*     @author jeremy.wang (2015/10/13)
* Desc: Set Clock for IP Verification
*
* @param u8Slot : Slot ID
* @param u32ReffClk :  Clock Hz
* @param u8DownLevel : Downgrade Level
----------------------------------------------------------------------------------------------------------*/
void IPV_SDMMC_SetClock(U8_T u8Slot, U32_T u32ReffClk, U8_T u8DownLevel)
{
    U32_T u32RealClk = 0;

    _PRT_LINE(FALSE);

    u32RealClk = SDMMC_SetClock(u8Slot, u32ReffClk, u8DownLevel);
    _PRT_VALUE(u8Slot, D_SETBUSCLK, u32RealClk, FALSE);

    _PRT_LINE(FALSE);
}

/*----------------------------------------------------------------------------------------------------------
*
* Function: IPV_SDMMC_SetBusTiming
*     @author jeremy.wang (2015/10/13)
* Desc: Set Bus Timing for IP Verification
*
* @param u8Slot : Slot ID
* @param u8BusTiming : Bus Timing (Enum Type number)
----------------------------------------------------------------------------------------------------------*/
void IPV_SDMMC_SetBusTiming(U8_T u8Slot, U8_T u8BusTiming)
{
    _PRT_LINE(FALSE);

    SDMMC_SetBusTiming(u8Slot, (BusTimingEmType)u8BusTiming);
    _PRT_VALUE(u8Slot, D_SETBUSTIMING, u8BusTiming, FALSE);

    _PRT_LINE(FALSE);
}

/*----------------------------------------------------------------------------------------------------------
*
* Function: IPV_SDMMC_CIFD_RW
*     @author jeremy.wang (2015/10/13)
* Desc: CIFD R/W for IP Verification
*
* @param u8Slot : Slot ID
* @param u32SecAddr : Sector Address
* @param bHidden : Hidden Print
----------------------------------------------------------------------------------------------------------*/
void IPV_SDMMC_CIFD_RW(U8_T u8Slot, U32_T u32SecAddr, BOOL_T bHidden)
{
#if (!EN_BIND_CARD_INT)

    _PRT_LINE(bHidden);

    _SET_MEM_PATTERN(dma_test_buf0, 512, gu8Pattern);

    gu16Ret1 = SDMMC_CIF_BLK_W(u8Slot, u32SecAddr << 9, dma_test_buf0);
    _PRT_MSG(u8Slot, D_CIFDWRITE, gu16Ret1, bHidden);
    _PRT_ENTER(bHidden);

    memset(dma_test_buf1, 0, 512);

    gu16Ret2 = SDMMC_CIF_BLK_R(u8Slot, u32SecAddr << 9, dma_test_buf1);
    _PRT_MSG(u8Slot, D_CIFDREAD, gu16Ret2, bHidden);
    _PRT_ENTER(bHidden);

    gu16Ret3 = _MEM_COMPARE(dma_test_buf0, dma_test_buf1, 512);
    _PRT_MSG(u8Slot, D_COMPARE, gu16Ret3, bHidden);

    _PRT_LINE(bHidden);

#if (D_DUMPCOMP)
    if (gu16Ret3)
    {
        _DUMP_MEM_ARR(dma_test_buf0, 512);
        _DUMP_MEM_ARR(dma_test_buf1, 512);
    }
#endif
#endif
}

/*----------------------------------------------------------------------------------------------------------
*
* Function: IPV_SDMMC_DMA_RW
*     @author jeremy.wang (2015/10/13)
* Desc: DMA R/W for IP Verification
*
* @param u8Slot : Slot ID
* @param u32SecAddr : Sector Address
* @param u16SecCount : Sector Count
* @param bHidden : Hidden Print
----------------------------------------------------------------------------------------------------------*/
void IPV_SDMMC_DMA_RW(U8_T u8Slot, U32_T u32SecAddr, U16_T u16SecCount, BOOL_T bHidden)
{
    if (u16SecCount > 8)
        return;

    _PRT_LINE(bHidden);

    _SET_MEM_PATTERN(dma_test_buf0, u16SecCount * 512, gu8Pattern);

    gu16Ret1 = SDMMC_DMA_BLK_W(u8Slot, u32SecAddr << 9, u16SecCount, dma_test_buf0);
    _PRT_MSG(u8Slot, D_DMAWRITE, gu16Ret1, bHidden);
    _PRT_ENTER(bHidden);

    memset(dma_test_buf1, 0, u16SecCount * 512);

    gu16Ret2 = SDMMC_DMA_BLK_R(u8Slot, u32SecAddr << 9, u16SecCount, dma_test_buf1);
    _PRT_MSG(u8Slot, D_DMAREAD, gu16Ret2, bHidden);
    _PRT_ENTER(bHidden);

    gu16Ret3 = _MEM_COMPARE(dma_test_buf0, dma_test_buf1, u16SecCount * 512);
    _PRT_MSG(u8Slot, D_COMPARE, gu16Ret3, bHidden);

    _PRT_LINE(bHidden);

#if (D_DUMPCOMP)
    if (gu16Ret3)
    {
        _DUMP_MEM_ARR(dma_test_buf0, u16SecCount * 512);
        _DUMP_MEM_ARR(dma_test_buf1, u16SecCount * 512);
    }
#endif
}

//###########################################################################################################
#if 1 //(D_FCIE_M_VER == D_FCIE_M_VER__05)
//###########################################################################################################

/*----------------------------------------------------------------------------------------------------------
*
* Function: IPV_SDMMC_ADMA_RW
*     @author jeremy.wang (2015/10/13)
* Desc: ADMA R/W for IP Verification
*
* @param u8Slot : Slot ID
* @param u32SecAddr : Sector Address
* @param bHidden : Hidden Print
----------------------------------------------------------------------------------------------------------*/
void IPV_SDMMC_ADMA_RW(U8_T u8Slot, U32_T u32SecAddr, BOOL_T bHidden)
{
    dma_addr_t auDMAAddr[10];
    U16_T      au16BlkCnt[10];
    U32_T      i, u32seccount = 0;

    _PRT_LINE(bHidden);

    _SET_MEM_PATTERN(dma_test_buf0, 2 * 512, gu8Pattern);

    for (i = 0; i < 10; i++)
    {
        auDMAAddr[i] = dma_test_addr0 + 0x200 * u32seccount;

        au16BlkCnt[i] = 0x1;
        u32seccount += au16BlkCnt[i];
    }

    gu16Ret1 = SDMMC_ADMA_BLK_W(u8Slot, u32SecAddr << 9, auDMAAddr, au16BlkCnt, 10, (volatile void *)AdmaScripts);
    _PRT_MSG(u8Slot, D_ADMAWRITE, gu16Ret1, bHidden);
    _PRT_ENTER(bHidden);
    if (gu16Ret1)
    {
        // while(1);
    }

    memset(dma_test_buf1, 0, 2 * 512);
    u32seccount = 0;

    for (i = 0; i < 10; i++)
    {
        auDMAAddr[i] = dma_test_addr1 + 0x200 * u32seccount;

        au16BlkCnt[i] = 0x5;
        u32seccount += au16BlkCnt[i];
    }

    gu16Ret2 = SDMMC_ADMA_BLK_R(u8Slot, u32SecAddr << 9, auDMAAddr, au16BlkCnt, 10, (volatile void *)AdmaScripts);
    _PRT_MSG(u8Slot, D_ADMAREAD, gu16Ret2, bHidden);
    _PRT_ENTER(bHidden);
    if (gu16Ret2)
    {
        // while(1);
    }

    gu16Ret3 = _MEM_COMPARE(dma_test_buf0, dma_test_buf1, 2 * 512);
    _PRT_MSG(u8Slot, D_COMPARE, gu16Ret3, bHidden);

    _PRT_LINE(bHidden);

#if (D_DUMPCOMP)
    if (gu16Ret3)
    {
        _DUMP_MEM_ARR(dma_test_buf0, 2 * 512);
        _DUMP_MEM_ARR(dma_test_buf1, 2 * 512);
        while (1)
            ;
    }
#endif
}

//###########################################################################################################
#endif // End (D_FCIE_M_VER == D_FCIE_M_VER__05)

/*----------------------------------------------------------------------------------------------------------
*
* Function: IPV_SDMMC_BurnRW
*     @author jeremy.wang (2015/10/13)
* Desc: Burning R/W for IP Verification
*
* @param u8Slot : Slot ID
* @param u8TransType : DMA/ADMA/CIFD ...
* @param u32StartSec : Start Sector
* @param u32EndSec : End Sector
----------------------------------------------------------------------------------------------------------*/
void IPV_SDMMC_BurnRW(U8_T u8Slot, U8_T u8TransType, U32_T u32StartSec, U32_T u32EndSec)
{
    U32_T u32Sec, u32ClkCnt = 0, u32PatCnt = 0, u8DownLvl = 0, u8PatLvl = 0;

    prtstring("\r\n******************************* [Begin Burning ] ***************************************\r\n");

    IPV_SDMMC_SetClock(u8Slot, 0, u8DownLvl); // Set to Maximum Clock
    IPV_SDMMC_TestPattern(u8Slot, u8PatLvl);

    if ((u32StartSec == 0) && (u32EndSec == 0))
    {
        u32StartSec = BURN_START_SECTOR;
        u32EndSec   = BURN_END_SECTOR;
    }

    for (u32Sec = u32StartSec; u32Sec < u32EndSec; u32Sec++)
    {
        u32ClkCnt++;
        u32PatCnt++;

        if (u8TransType == 1) // DMA
        {
            prtstring("Verify_DMA_RW (SEC#)=%08x.........\r\n", u32Sec);
            IPV_SDMMC_DMA_RW(u8Slot, u32Sec, 2, TRUE);

            if ((gu16Ret1 > 0) || (gu16Ret2 > 0) || (gu16Ret3 > 0))
                return;
        }

//###########################################################################################################
#if 1 //(D_FCIE_M_VER == D_FCIE_M_VER__05)
      //###########################################################################################################
        else if (u8TransType == 2)
        {
            prtstring("Verify_ADMA_RW (SEC#)=%08x.........\r\n", u32Sec);
            IPV_SDMMC_ADMA_RW(u8Slot, u32Sec, TRUE);

            if ((gu16Ret1 > 0) || (gu16Ret2 > 0) || (gu16Ret3 > 0))
                return;
        }
//###########################################################################################################
#endif // End (D_FCIE_M_VER == D_FCIE_M_VER__05)

        else
        {
            prtstring("Verify_CIFD_RW (SEC#)=%08x.........\r\n", u32Sec);
            IPV_SDMMC_CIFD_RW(u8Slot, u32Sec, TRUE);

            if ((gu16Ret1 > 0) || (gu16Ret2 > 0) || (gu16Ret3 > 0))
                return;
        }

        if (u32ClkCnt >= BURN_CLK_CHG_CNT)
        {
            u32ClkCnt = 0;
            u8DownLvl++;

            if (u8DownLvl > BURN_CLK_CHG_LVL)
                u8DownLvl = 0;

            IPV_SDMMC_SetClock(u8Slot, 0, u8DownLvl);
        }

        if (u32PatCnt >= BURN_PAT_CHG_CNT)
        {
            u32PatCnt = 0;
            u8PatLvl++;

            if (u8PatLvl > BURN_PAT_CHG_LVL)
                u8PatLvl = 0;

            IPV_SDMMC_TestPattern(u8Slot, u8PatLvl);
        }

        // Hal_Timer_mDelay(200);
    }

    prtstring("\r\n******************************* [Begin Endg ] *****************************************\r\n");
}

/*----------------------------------------------------------------------------------------------------------
*
* Function: IPV_SDMMC_TestPattern
*     @author jeremy.wang (2015/10/1)
* Desc: Test Pattern for IP Verification
*
* @param u8Slot : Slot ID
* @param u8Pattern : Pattern Number
----------------------------------------------------------------------------------------------------------*/
void IPV_SDMMC_TestPattern(U8_T u8Slot, U8_T u8Pattern)
{
    _PRT_LINE(FALSE);

    gu8Pattern = u8Pattern;
    _PRT_VALUE(u8Slot, D_TESTPATT, (U32_T)gu8Pattern, FALSE);

    _PRT_LINE(FALSE);
}

/*----------------------------------------------------------------------------------------------------------
*
* Function: IPV_SDMMC_SetPAD
*     @author jeremy.wang (2015/10/1)
* Desc: Set PAD for IP Verification
*
* @param u8Slot : Slot ID
* @param u8IP : IP (Enum IP number)
* @param u8Port : Port (Enum Port number)
* @param u8PAD : PAD (Enum PAD number)
----------------------------------------------------------------------------------------------------------*/
void IPV_SDMMC_SetPAD(U8_T u8Slot, U8_T u8IP, U8_T u8Port, U8_T u8PAD)
{
    _PRT_LINE(FALSE);

    SDMMC_SetPAD(u8Slot, (IpOrder)u8IP, (SlotEmType)u8Port, (PadOrder)u8PAD);
    SDMMC_SwitchPAD(u8Slot);
    _PRT_MSG(u8Slot, D_SETPAD, 0, FALSE);

    _PRT_LINE(FALSE);
}

#if 1
void do_io_std_test(unsigned char u8Slot)
{
    unsigned int i;

    for (i = 6; i < 16; i++)
    {
        IPV_SDMMC_CIFD_RW(u8Slot, i, 0);
        IPV_SDMMC_DMA_RW(u8Slot, i, 2, 0);

//###########################################################################################################
#if 1 //(D_FCIE_M_VER == D_FCIE_M_VER__05)
      //###########################################################################################################
        IPV_SDMMC_ADMA_RW(u8Slot, i, 0);
//###########################################################################################################
#endif
    }
}

void do_io_std_burntest(unsigned char u8Slot)
{
    IPV_SDMMC_CIFD_RW(u8Slot, 6, 0);
    IPV_SDMMC_DMA_RW(u8Slot, 6, 2, 0);

//###########################################################################################################
#if 1 //(D_FCIE_M_VER == D_FCIE_M_VER__05)
      //###########################################################################################################
    IPV_SDMMC_ADMA_RW(u8Slot, 6, 0);
//###########################################################################################################
#endif

    IPV_SDMMC_BurnRW(u8Slot, 0, 30000, 30050); // CIFD
    IPV_SDMMC_BurnRW(u8Slot, 1, 30000, 30050); // DMA

//###########################################################################################################
#if 1 //(D_FCIE_M_VER == D_FCIE_M_VER__05)
      //###########################################################################################################
    IPV_SDMMC_BurnRW(u8Slot, 2, 30000, 30050); // ADMA
//###########################################################################################################
#endif
}

void IPV_SDMMC_Verify(struct device *dev, U8_T u8Val1)
{
    dma_test_buf0 = dma_alloc_coherent(dev, MAX_BLK_COUNT * MAX_BLK_SIZE, &dma_test_addr0, GFP_KERNEL);
    dma_test_buf1 = dma_alloc_coherent(dev, MAX_BLK_COUNT * MAX_BLK_SIZE, &dma_test_addr1, GFP_KERNEL);
    AdmaScripts   = dma_alloc_coherent(dev, sizeof(AdmaScripts) * MAX_SEG_CNT, &AdmaScriptsAddr, GFP_KERNEL);

    printk(KERN_CONT "dma_test_buf0: phys_addr: %llx | vir_addr: %llx \n", dma_test_addr0,
           (U64_T)(uintptr_t)dma_test_buf0);
    printk(KERN_CONT "dma_test_buf1: phys_addr: %llx | vir_addr: %llx \n", dma_test_addr1,
           (U64_T)(uintptr_t)dma_test_buf1);
    printk(KERN_CONT "ADMA SCRIPTS : phys_addr: %llx | vir_addr: %llx \n", AdmaScriptsAddr,
           (U64_T)(uintptr_t)AdmaScripts);

    // at1d
    IPV_SDMMC_Init(u8Val1);

    printk("\r\n*********************** [ Test 1bit Low Speed I/O ] ************************************\r\n");

    do_io_std_test(u8Val1);

    printk("\r\n*********************** [ Test 1bit Default Speed I/O ] ********************************\r\n");

    IPV_SDMMC_SetClock(u8Val1, 0, 0);
    IPV_SDMMC_SetBusTiming(u8Val1, 1);

    do_io_std_burntest(u8Val1);

    // at1h
    IPV_SDMMC_TimerTest(3);
    IPV_SDMMC_CardDetect(u8Val1);
    IPV_SDMMC_Init(u8Val1);

    printk("\r\n*********************** [ Test 1bit High Speed I/O ] ***********************************\r\n");

    IPV_SDMMC_SetHighBus(u8Val1);
    IPV_SDMMC_SetClock(u8Val1, 0, 0);
    IPV_SDMMC_SetBusTiming(u8Val1, 2);

    do_io_std_burntest(u8Val1);

    // at4d
    IPV_SDMMC_TimerTest(3);
    IPV_SDMMC_CardDetect(u8Val1);
    IPV_SDMMC_Init(u8Val1);

    printk("\r\n*********************** [ Test 4bit Low Speed I/O ] ************************************\r\n");

    IPV_SDMMC_SetWideBus(u8Val1);
    do_io_std_test(u8Val1);

    printk("\r\n*********************** [ Test 4bit Default Speed I/O ] ********************************\r\n");

    IPV_SDMMC_SetClock(u8Val1, 0, 0);
    IPV_SDMMC_SetBusTiming(u8Val1, 1);

    do_io_std_burntest(u8Val1);

    // at4h
    IPV_SDMMC_TimerTest(3);
    IPV_SDMMC_CardDetect(u8Val1);
    IPV_SDMMC_Init(u8Val1);

    printk("\r\n*********************** [ Test 4bit High Speed I/O ] ***********************************\r\n");

    IPV_SDMMC_SetWideBus(u8Val1);
    IPV_SDMMC_SetHighBus(u8Val1);
    IPV_SDMMC_SetClock(u8Val1, 0, 0);
    IPV_SDMMC_SetBusTiming(u8Val1, 2);

    do_io_std_burntest(u8Val1);

    dma_free_coherent(dev, MAX_BLK_COUNT * MAX_BLK_SIZE, dma_test_buf0, dma_test_addr0);
    dma_free_coherent(dev, MAX_BLK_COUNT * MAX_BLK_SIZE, dma_test_buf1, dma_test_addr1);
    dma_free_coherent(dev, sizeof(AdmaScripts) * MAX_SEG_CNT, AdmaScripts, AdmaScriptsAddr);
}

void IPV_SDMMC_PowerSavingModeVerify(U8_T u8Slot)
{
    Hal_SDMMC_PowerSavingModeVerify(u8Slot);
}

void SDMMC_SDIOinterrupt(IpOrder eIP)
{
    Hal_SDMMC_SDIOinterrupt(eIP);
}

#endif
