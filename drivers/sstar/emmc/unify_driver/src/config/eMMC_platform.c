/*
 * eMMC_platform.c- Sigmastar
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
#include "eMMC_hal.h"
#include "eMMC_platform.h"
#include "eMMC_err_codes.h"
#include "eMMC_linux.h"
#include "mstar_mci.h"

#if defined(UNIFIED_eMMC_DRIVER) && UNIFIED_eMMC_DRIVER

//

//=============================================================
#if eMMC_DRV_LINUX
//=============================================================
U8 gau8_eMMCPLLSel_52[eMMC_FCIE_VALID_CLK_CNT] = {eMMC_PLL_CLK_200M, eMMC_PLL_CLK_160M, eMMC_PLL_CLK_120M};

U8 gau8_eMMCPLLSel_200[eMMC_FCIE_VALID_CLK_CNT] = {eMMC_PLL_CLK_200M, eMMC_PLL_CLK_160M, eMMC_PLL_CLK_140M};

U8 gau8_FCIEClkSel[eMMC_FCIE_VALID_CLK_CNT] = {
    BIT_FCIE_CLK_48M, BIT_FCIE_CLK_40M, BIT_FCIE_CLK_36M, BIT_FCIE_CLK_32M, BIT_FCIE_CLK_20M,
};
U32 gu32_eMMCDrvExtFlag = 0;

extern EMMCPinDrv_T gst_emmcPinDriving[EMMC_NUM_TOTAL];
static volatile U16 gu16_eMMCMIEEvent_ForInt[3] = {0}; // MIEEvent for Interrupt

#if defined(MSTAR_EMMC_CONFIG_OF)
#ifdef CONFIG_CAM_CLK
//
#else
extern struct clk_data *clkdata;
#endif
#endif

void mdelay_MacroToFun(U32 time)
{
    mdelay(time);
}

U32 eMMC_hw_timer_delay(U32 u32usTick)
{
    volatile U32 u32_i = u32usTick;

    while (u32_i > 1000)
    {
        udelay(1000);
        u32_i -= 1000;
    }

    udelay(u32usTick);
    return u32usTick + 1;
}

U32 eMMC_hw_timer_sleep(U32 u32ms)
{
    U32 u32_i = u32ms;

    while (u32_i > 1000)
    {
        msleep(1000);
        u32_i -= 1000;
    }

    msleep(u32_i);
    return u32ms;
}

//--------------------------------
// use to performance test
U32 eMMC_hw_timer_start(void)
{
    return 0;
}

U32 eMMC_hw_timer_tick(void)
{
    return 0;
}

void eMMC_DumpPadClk(eMMC_IP_EmType emmc_ip)
{
    //---------------------------------------------------------------------
    eMMC_debug(0, 0, "[pad setting]:\r\n");
    switch (g_eMMCDrv[emmc_ip].u8_PadType)
    {
        case FCIE_eMMC_BYPASS:
            eMMC_debug(0, 0, "Bypass\r\n");
            break;
        case FCIE_eMMC_SDR:
            eMMC_debug(0, 0, "SDR\r\n");
            break;
        default:
            eMMC_debug(0, 0, "eMMC Err: Pad unknown, %d\r\n", g_eMMCDrv[emmc_ip].u8_PadType);
            eMMC_die("\r\n");
            break;
    }
}

U8 gu8_NANDeMMC_need_preset_flag = 1;

#if 0
void eMMC_pll_dll_setting(U16 u16_ClkParam)
{
    volatile U16 u16_reg;

    REG_FCIE_CLRBIT(reg_emmcpll_0x09, BIT0); // ???? reg_emmc_rxdll_dline_en

    // Reset eMMC_DLL
    REG_FCIE_SETBIT(REG_EMMC_PLL_RX30, BIT2);
    REG_FCIE_CLRBIT(REG_EMMC_PLL_RX30, BIT2);

    //DLL pulse width and phase
    REG_FCIE_W(REG_EMMC_PLL_RX01, 0x7F72);

    // DLL code
    REG_FCIE_W(REG_EMMC_PLL_RX32, 0xF200);

    // DLL calibration
    REG_FCIE_W(REG_EMMC_PLL_RX30, 0x3378);
    REG_FCIE_SETBIT(REG_EMMC_PLL_RX33, BIT15);

    // Wait 250us
    eMMC_hw_timer_delay(250);

    // Get hw dll0 code
    REG_FCIE_R(REG_EMMC_PLL_RX33, u16_reg);

    //eMMC_debug(eMMC_DEBUG_LEVEL_LOW, 1, "EMMCPLL 0x33=%04Xh\n", u16_reg);

    REG_FCIE_CLRBIT(REG_EMMC_PLL_RX34, (BIT10 - 1));

    // Set dw dll0 code
    REG_FCIE_SETBIT(REG_EMMC_PLL_RX34, u16_reg & 0x03FF);

    // Disable reg_hw_upcode_en
    REG_FCIE_CLRBIT(REG_EMMC_PLL_RX30, BIT9);

    // Disable reg_hw_upcode_en
    REG_FCIE_CLRBIT(REG_EMMC_PLL_RX30, BIT9);

    // Clear reg_emmc_dll_test[7]
    REG_FCIE_CLRBIT(reg_emmcpll_0x02, BIT15); // switch pad ??@ bit13, excel undefine ??

    // Enable reg_rxdll_dline_en
    REG_FCIE_SETBIT(reg_emmcpll_0x09, BIT0); // ????
}
#endif

U32 eMMC_clock_gating(eMMC_IP_EmType emmc_ip)
{
    eMMC_PlatformResetPre();
    REG_FCIE_CLRBIT(FCIE_SD_MODE(emmc_ip), BIT_CLK_EN);
    eMMC_PlatformResetPost();
    return eMMC_ST_SUCCESS;
}

void eMMC_set_WatchDog(U8 u8_IfEnable)
{
    // do nothing
}

void eMMC_reset_WatchDog(void)
{
    // do nothing
}

U32 eMMC_FCIE_BuildHS200TimingTable(void)
{
    return eMMC_ST_SUCCESS;
}

U32 eMMC_FCIE_BuildDDRTimingTable(void)
{
    return eMMC_ST_SUCCESS;
}

extern struct platform_device sg_mstar_emmc_device_st;

dma_addr_t eMMC_DMA_MAP_address(uintptr_t ulongBuffer, U32 u32_ByteCnt, int mode)
{
    dma_addr_t dma_addr;

    if (mode == 0) // write
    {
        dma_addr = dma_map_single(&sg_mstar_emmc_device_st.dev, (void *)ulongBuffer, u32_ByteCnt, DMA_TO_DEVICE);
    }
    else
    {
        dma_addr = dma_map_single(&sg_mstar_emmc_device_st.dev, (void *)ulongBuffer, u32_ByteCnt, DMA_FROM_DEVICE);
    }

    if (dma_mapping_error(&sg_mstar_emmc_device_st.dev, dma_addr))
    {
        dma_unmap_single(&sg_mstar_emmc_device_st.dev, dma_addr, u32_ByteCnt, (mode) ? DMA_FROM_DEVICE : DMA_TO_DEVICE);
        eMMC_die("eMMC_DMA_MAP_address: Kernel can't mapping dma correctly\n");
    }

    return dma_addr;
}

void eMMC_DMA_UNMAP_address(dma_addr_t dma_DMAAddr, U32 u32_ByteCnt, int mode)
{
    if (mode == 0) // write
    {
        dma_unmap_single(&sg_mstar_emmc_device_st.dev, dma_DMAAddr, u32_ByteCnt, DMA_TO_DEVICE);
    }
    else
    {
        dma_unmap_single(&sg_mstar_emmc_device_st.dev, dma_DMAAddr, u32_ByteCnt, DMA_FROM_DEVICE);
    }
}

/*
void eMMC_Invalidate_data_cache_buffer(U32 u32_addr, S32 s32_size)
{
    flush_cache(u32_addr, s32_size);
}

void eMMC_flush_miu_pipe(void)
{

}
*/

//---------------------------------------
#if defined(ENABLE_eMMC_INTERRUPT_MODE) && ENABLE_eMMC_INTERRUPT_MODE

static DECLARE_WAIT_QUEUE_HEAD(fcie_wait);

#define eMMC_IRQ_DEBUG 0

void eMMC_INT_ClearMIEEvent(eMMC_IP_EmType emmc_ip)
{
    gu16_eMMCMIEEvent_ForInt[emmc_ip] = 0;
}

irqreturn_t eMMC_FCIE_IRQ(int irq, void *dummy)
{
    volatile u16           u16_Events;
    struct mstar_mci_host *pSstarHost_st = (struct mstar_mci_host *)dummy;
    eMMC_IP_EmType         emmc_ip       = pSstarHost_st->slot_ip;

    // one time enable one bit
    REG_FCIE_R(FCIE_PWR_SAVE_CTL(emmc_ip), u16_Events);

    if (u16_Events & BIT_POWER_SAVE_MODE_INT)
    {
        REG_FCIE_CLRBIT(FCIE_PWR_SAVE_CTL(emmc_ip), BIT_POWER_SAVE_MODE_INT_EN);
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "SAR5 eMMC WARN.\n");
        while (1)
            ;
        goto done;
    }

    // if ((REG_FCIE(FCIE_MIE_FUNC_CTL(emmc_ip)) & BIT_EMMC_ACTIVE) != BIT_EMMC_ACTIVE)
    //{
    //     return IRQ_NONE;
    // }

    u16_Events = REG_FCIE(FCIE_MIE_EVENT(emmc_ip)) & REG_FCIE(FCIE_MIE_INT_EN(emmc_ip));

    if (u16_Events & BIT_DMA_END)
    {
        gu16_eMMCMIEEvent_ForInt[emmc_ip] |= u16_Events;
        REG_FCIE_CLRBIT(FCIE_MIE_INT_EN(emmc_ip), BIT_DMA_END);
        goto done;
    }
    else if (u16_Events & BIT_SD_CMD_END)
    {
        gu16_eMMCMIEEvent_ForInt[emmc_ip] |= u16_Events;
        REG_FCIE_CLRBIT(FCIE_MIE_INT_EN(emmc_ip), BIT_SD_CMD_END);
        goto done;
    }
    else if (u16_Events & BIT_ERR_STS)
    {
        gu16_eMMCMIEEvent_ForInt[emmc_ip] |= u16_Events;
        REG_FCIE_CLRBIT(FCIE_MIE_INT_EN(emmc_ip), BIT_ERR_STS);
        goto done;
    }
#if defined(ENABLE_FCIE_HW_BUSY_CHECK) && ENABLE_FCIE_HW_BUSY_CHECK
    else if (u16_Events & BIT_BUSY_END_INT)
    {
        REG_FCIE_CLRBIT(FCIE_MIE_INT_EN(emmc_ip), BIT_BUSY_END_INT);
        REG_FCIE_CLRBIT(FCIE_SD_CTRL(emmc_ip), BIT_BUSY_DET_ON);
        goto done;
    }
#endif

#if eMMC_IRQ_DEBUG
    if (0 == (u16_Events & 0x3F))
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Warn: Int St:%Xh, En:%Xh, Evt:%Xh \n",
                   REG_FCIE(FCIE_MIE_EVENT(emmc_ip)), REG_FCIE(FCIE_MIE_INT_EN(emmc_ip)), u16_Events);
#endif

    return IRQ_NONE;
done:
    wake_up(&fcie_wait);
    return IRQ_HANDLED;
}

U32 eMMC_WaitCompleteIntr(eMMC_IP_EmType emmc_ip, uintptr_t u32_RegAddr, U16 u16_WaitEvent, U32 u32_MicroSec)
{
    U32 u32_i = 0;

#if eMMC_IRQ_DEBUG
    U32                u32_isr_tmp[2];
    unsigned long long u64_jiffies_tmp, u64_jiffies_now;
    struct timeval     time_st;
    time_t             sec_tmp;
    suseconds_t        us_tmp;

    u32_isr_tmp[0] = REG_FCIE(u32_RegAddr);
    do_gettimeofday(&time_st);
    sec_tmp         = time_st.tv_sec;
    us_tmp          = time_st.tv_usec;
    u64_jiffies_tmp = jiffies_64;
#endif

    //----------------------------------------
    if (wait_event_timeout(fcie_wait, ((gu16_eMMCMIEEvent_ForInt[emmc_ip] & u16_WaitEvent) == u16_WaitEvent),
                           usecs_to_jiffies(u32_MicroSec))
        == 0)
    {
#if eMMC_IRQ_DEBUG
        u32_isr_tmp[1] = REG_FCIE(u32_RegAddr);
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1,
                   "eMMC Warn: int timeout, WaitEvt:%Xh, NowEvt:%Xh, IntEn:%Xh, ISR:%u->%u->%u \n", u16_WaitEvent,
                   REG_FCIE(FCIE_MIE_EVENT(emmc_ip)), REG_FCIE(FCIE_MIE_INT_EN(emmc_ip)), u32_isr_tmp[0],
                   u32_isr_tmp[1], REG_FCIE(u32_RegAddr));

        do_gettimeofday(&time_st);
        u64_jiffies_now = jiffies_64;
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1,
                   " PassTime: %lu s, %lu us, %llu jiffies.  WaitTime: %u us, %lu jiffies, HZ:%u.\n",
                   time_st.tv_sec - sec_tmp, time_st.tv_usec - us_tmp, u64_jiffies_now - u64_jiffies_tmp, u32_MicroSec,
                   usecs_to_jiffies(u32_MicroSec), HZ);
#else
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Warn: int timeout, WaitEvt:%Xh, NowEvt:%Xh, IntEn:%Xh \n",
                   u16_WaitEvent, REG_FCIE(FCIE_MIE_EVENT(emmc_ip)), REG_FCIE(FCIE_MIE_INT_EN(emmc_ip)));
#endif

        // switch to polling
        for (u32_i = 0; u32_i < u32_MicroSec; u32_i++)
        {
            if ((REG_FCIE(u32_RegAddr) & u16_WaitEvent) == u16_WaitEvent)
                break;

            eMMC_hw_timer_delay(HW_TIMER_DELAY_1us);
        }

        if (u32_i == u32_MicroSec)
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: events lose, WaitEvent: %Xh \n", u16_WaitEvent);
            REG_FCIE_CLRBIT(FCIE_MIE_INT_EN(emmc_ip), u16_WaitEvent);
            return eMMC_ST_ERR_INT_TO;
        }
        else
        {
            REG_FCIE_CLRBIT(FCIE_MIE_INT_EN(emmc_ip), u16_WaitEvent);
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Warn: but polling ok: %Xh \n", REG_FCIE(u32_RegAddr));
        }
    }
    //----------------------------------------
    return eMMC_ST_SUCCESS;
}

#endif

//---------------------------------------
// extern struct semaphore   PfModeSem;
//#include <linux/semaphore.h>
// extern struct semaphore   PfModeSem;
DEFINE_MUTEX(mutex_eMMC0);
DEFINE_MUTEX(mutex_eMMC1);
DEFINE_MUTEX(mutex_eMMC2);
DEFINE_MUTEX(mutex_eMMC3);

static void _eMMC_MUTEX_LOCK(eMMC_IP_EmType emmc_ip)
{
    if (emmc_ip == IP_EMMC0)
        mutex_lock(&mutex_eMMC0);
    else if (emmc_ip == IP_EMMC1)
        mutex_lock(&mutex_eMMC1);
    else if (emmc_ip == IP_EMMC2)
        mutex_lock(&mutex_eMMC2);
    else if (emmc_ip == IP_EMMC3)
        mutex_lock(&mutex_eMMC3);
    else
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "ip_no(%d) do not exist!\n", emmc_ip);
}

static void _eMMC_MUTEX_UNLOCK(eMMC_IP_EmType emmc_ip)
{
    if (emmc_ip == IP_EMMC0)
        mutex_unlock(&mutex_eMMC0);
    else if (emmc_ip == IP_EMMC1)
        mutex_unlock(&mutex_eMMC1);
    else if (emmc_ip == IP_EMMC2)
        mutex_unlock(&mutex_eMMC2);
    else if (emmc_ip == IP_EMMC3)
        mutex_unlock(&mutex_eMMC3);
    else
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "ip_no(%d) do not exist!\n", emmc_ip);
}

//#define CRIT_SECT_BEGIN(x)    mutex_lock(x)
//#define CRIT_SECT_END(x)  mutex_unlock(x)

extern bool ms_sdmmc_wait_d0_for_emmc(void);

void eMMC_LockFCIE(eMMC_IP_EmType emmc_ip, U8 *pu8_str)
{
    _eMMC_MUTEX_LOCK(emmc_ip);
#if defined(MSTAR_EMMC_CONFIG_OF)
    {
#ifdef CONFIG_CAM_CLK
        //
#else
        clk_prepare_enable(clkdata->clk_fcie[emmc_ip]);

#endif
    }
#endif

#if IF_FCIE_SHARE_IP // && defined(CONFIG_MS_SDMMC)
                     //  if(false == ms_sdmmc_wait_d0_for_emmc())
                     //  {
                     //      eMMC_debug(eMMC_DEBUG_LEVEL_ERROR,1,"eMMC Err: SD keep D0 low \n");
                     //      eMMC_FCIE_ErrHandler_Stop();
                     //  }

    REG_FCIE_CLRBIT(FCIE_TEST_MODE(emmc_ip), BIT_DEBUG_MODE_MASK);
    REG_FCIE_SETBIT(FCIE_TEST_MODE(emmc_ip),
                    2 << BIT_DEBUG_MODE_SHIFT); // 2: card_data1_dbus = {xxxxx, Wrstate, RDstate}

    REG_FCIE_CLRBIT(FCIE_EMMC_DEBUG_BUS1(emmc_ip), BIT11 | BIT10 | BIT9 | BIT8);
    REG_FCIE_SETBIT(FCIE_EMMC_DEBUG_BUS1(emmc_ip), 5 << 8); // 5: card

    if (REG_FCIE(FCIE_EMMC_DEBUG_BUS0(emmc_ip)) & 0x0FFF) // Check FICE5 StateMachine
    {
        eMMC_FCIE_Init(emmc_ip);
    }

#if 0 // no need to reconfiguration
    eMMC_clock_setting(emmc_ip, g_eMMCDrv[emmc_ip].u16_ClkRegVal);
    eMMC_pads_switch(emmc_ip, g_eMMCDrv[emmc_ip].u8_PadType);
#endif
#endif
    REG_FCIE_SETBIT(FCIE_SD_MODE(emmc_ip), BIT_SD_CLK_EN); // output clock

    REG_FCIE_SETBIT(FCIE_MIE_FUNC_CTL(emmc_ip), BIT_EMMC_ACTIVE);
}

void eMMC_UnlockFCIE(eMMC_IP_EmType emmc_ip, U8 *pu8_str)
{
    REG_FCIE_CLRBIT(FCIE_MIE_FUNC_CTL(emmc_ip), BIT_EMMC_ACTIVE);
    REG_FCIE_CLRBIT(FCIE_SD_MODE(emmc_ip), BIT_SD_CLK_EN); // not output clock
#if defined(MSTAR_EMMC_CONFIG_OF)
    {
#ifdef CONFIG_CAM_CLK
        //
#else
        clk_disable_unprepare(clkdata->clk_fcie[emmc_ip]);
#endif
    }
#endif

    _eMMC_MUTEX_UNLOCK(emmc_ip);
}

U32 eMMC_PlatformResetPre(void)
{
    return eMMC_ST_SUCCESS;
}

U32 eMMC_PlatformResetPost(void)
{
#if defined(ENABLE_EMMC_POWER_SAVING_MODE) && ENABLE_EMMC_POWER_SAVING_MODE
    eMMC_Prepare_Power_Saving_Mode_Queue();
#endif

    return eMMC_ST_SUCCESS;
}
struct page *eMMC_SectorPage       = 0;
struct page *eMMC_PartInfoPage     = 0;
U8 *         gau8_eMMC_SectorBuf   = 0; // 512 bytes
U8 *         gau8_eMMC_PartInfoBuf = 0; // 512 bytes

U32 eMMC_PlatformInit(eMMC_IP_EmType emmc_ip)
{
    eMMC_pads_switch(emmc_ip, gu8_emmcPADOrderSlot[emmc_ip], EMMC_DEFO_SPEED_MODE);
    eMMC_clock_setting(emmc_ip, FCIE_SLOWEST_CLK);
    eMMC_Init_Padpin(emmc_ip, gu8_emmcPADOrderSlot[emmc_ip]);
    eMMC_driving_control(emmc_ip, gu8_emmcPADOrderSlot[emmc_ip], gst_emmcPinDriving[emmc_ip]);

    // driving ctrl

    // PE

    if (gau8_eMMC_SectorBuf == NULL)
    {
        eMMC_SectorPage = alloc_pages(__GFP_COMP, 2);
        if (eMMC_SectorPage == NULL)
        {
            eMMC_debug(0, 1, "Err allocate page 1 fails\n");
            eMMC_die();
        }
        gau8_eMMC_SectorBuf = (U8 *)kmap(eMMC_SectorPage);
    }

    if (gau8_eMMC_PartInfoBuf == NULL)
    {
        eMMC_PartInfoPage = alloc_pages(__GFP_COMP, 0);
        if (eMMC_PartInfoPage == NULL)
        {
            eMMC_debug(0, 1, "Err allocate page 2 fails\n");
            eMMC_die();
        }
        gau8_eMMC_PartInfoBuf = (U8 *)kmap(eMMC_PartInfoPage);
    }

    return eMMC_ST_SUCCESS;
}

U32 eMMC_BootPartitionHandler_WR(U8 *pDataBuf, U16 u16_PartType, U32 u32_StartSector, U32 u32_SectorCnt, U8 u8_OP)
{
    return eMMC_ST_SUCCESS;
}

U32 eMMC_BootPartitionHandler_E(U16 u16_PartType)
{
    return eMMC_ST_SUCCESS;
}

// --------------------------------------------
static U32              sgu32_MemGuard0 = 0xA55A;
eMMC_ALIGN0 eMMC_DRIVER g_eMMCDrv[EMMC_NUM_TOTAL] eMMC_ALIGN1;
static U32              sgu32_MemGuard1 = 0x1289;

U32 eMMC_CheckIfMemCorrupt(void)
{
    if (0xA55A != sgu32_MemGuard0 || 0x1289 != sgu32_MemGuard1)
        return eMMC_ST_ERR_MEM_CORRUPT;

    return eMMC_ST_SUCCESS;
}

int mstar_mci_Housekeep(void *pData)
{
#if !(defined(eMMC_HOUSEKEEP_THREAD) && eMMC_HOUSEKEEP_THREAD)
    return 0;
#endif

    while (1)
    {
        if (kthread_should_stop())
            break;
    }

    return 0;
}

U32 eMMC_PlatformDeinit(void)
{
    return eMMC_ST_SUCCESS;
}

#else

#error "Error! no platform functions."
#endif
#endif
