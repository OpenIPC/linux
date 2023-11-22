/*
 * eMMC_platform.h- Sigmastar
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

//=====================================================
// unit for HW Timer delay (unit of us)
//=====================================================
#define HW_TIMER_DELAY_1us   1
#define HW_TIMER_DELAY_5us   5
#define HW_TIMER_DELAY_10us  10
#define HW_TIMER_DELAY_100us 100
#define HW_TIMER_DELAY_500us 500
#define HW_TIMER_DELAY_1ms   (1000 * HW_TIMER_DELAY_1us)
#define HW_TIMER_DELAY_5ms   (5 * HW_TIMER_DELAY_1ms)
#define HW_TIMER_DELAY_10ms  (10 * HW_TIMER_DELAY_1ms)
#define HW_TIMER_DELAY_100ms (100 * HW_TIMER_DELAY_1ms)
#define HW_TIMER_DELAY_500ms (500 * HW_TIMER_DELAY_1ms)
#define HW_TIMER_DELAY_1s    (1000 * HW_TIMER_DELAY_1ms)

//=====================================================
// transfer DMA Address
//=====================================================
#define MIU_BUS_WIDTH_BITS 3 // 8 bytes width [FIXME]

/*
 * Important:
 * The following buffers should be large enough for a whole eMMC block
 */
// FIXME, this is only for verifing IP
#define DMA_W_ADDR       0x40C00000
#define DMA_R_ADDR       0x40D00000
#define DMA_W_SPARE_ADDR 0x40E00000
#define DMA_R_SPARE_ADDR 0x40E80000
#define DMA_BAD_BLK_BUF  0x40F00000

//=====================================================
// misc
//=====================================================
//#define BIG_ENDIAN
#define LITTLE_ENDIAN

//=====================================================
// CLK sel
//=====================================================
extern U8 gau8_FCIEClkSel[];
extern U8 gau8_eMMCPLLSel_52[];
extern U8 gau8_eMMCPLLSel_200[]; // for DDR52 or HS200

//=====================================================
// eMMC platform declarations
//=====================================================
volatile U32 EMMC_GET_REG_BANK(U8 eIP, U8 u8Bank);
volatile U32 EMMC_GET_CLK_REG(U8 eIP);
volatile U32 EMMC_GET_PLL_REG(U8 eIP);
volatile U16 EMMC_GET_BOOT_CLK(U8 eIP);
volatile U16 IP_BIT_FUNC_ENABLE(U8 eIP);
U16          eMMC_Find_Clock_Reg(eMMC_IP_EmType emmc_ip, U32 u32_Clk);
U32          _eMMC_pll_setting(eMMC_IP_EmType emmc_ip, U16 u16_ClkParam);
U32          eMMC_clock_setting(eMMC_IP_EmType emmc_ip, U16 u16_ClkParam);
void         _eMMC_set_bustiming(eMMC_IP_EmType emmc_ip, U32 u32_FCIE_IF_Type);
U32          eMMC_pads_switch(eMMC_IP_EmType emmc_ip, U8 u8_PAD, U32 u32_FCIE_IF_Type);
void         eMMC_Read_Timeout_Set(eMMC_IP_EmType emmc_ip);
void         eMMC_Read_Timeout_Clear(eMMC_IP_EmType emmc_ip);
void         eMMC_Init_Padpin(eMMC_IP_EmType emmc_ip, U8 u8_PAD);
void         eMMC_driving_control(eMMC_IP_EmType emmc_ip, U8 u8_PAD, EMMCPinDrv_T st_EmmcPindrv);
void         eMMC_RST_L(eMMC_IP_EmType emmc_ip);
void         eMMC_RST_H(eMMC_IP_EmType emmc_ip);
dma_addr_t   eMMC_Platform_Trans_Dma_Addr(eMMC_IP_EmType emmc_ip, dma_addr_t dma_DMAAddr, U32 *u32MiuSel);

//=====================================================
// API declarations
//=====================================================
void mdelay_MacroToFun(U32 time);
U32  eMMC_hw_timer_delay(U32 u32usTick);
U32  eMMC_hw_timer_sleep(U32 u32ms);
U32  eMMC_hw_timer_start(void);
U32  eMMC_hw_timer_tick(void);
void eMMC_DumpPadClk(eMMC_IP_EmType emmc_ip);
U32  eMMC_clock_gating(eMMC_IP_EmType emmc_ip);
void eMMC_set_WatchDog(U8 u8_IfEnable);
void eMMC_reset_WatchDog(void);
U32  eMMC_FCIE_BuildHS200TimingTable(void);
U32  eMMC_FCIE_BuildDDRTimingTable(void);
void eMMC_LockFCIE(eMMC_IP_EmType emmc_ip, U8 *pu8_str);
void eMMC_UnlockFCIE(eMMC_IP_EmType emmc_ip, U8 *pu8_str);
U32  eMMC_PlatformResetPre(void);
U32  eMMC_PlatformResetPost(void);
U32  eMMC_PlatformInit(eMMC_IP_EmType emmc_ip);
U32  eMMC_BootPartitionHandler_WR(U8 *pDataBuf, U16 u16_PartType, U32 u32_StartSector, U32 u32_SectorCnt, U8 u8_OP);
U32  eMMC_BootPartitionHandler_E(U16 u16_PartType);
U32  eMMC_CheckIfMemCorrupt(void);
int  mstar_mci_Housekeep(void *pData);
U32  eMMC_PlatformDeinit(void);
void eMMC_DMA_UNMAP_address(dma_addr_t dma_DMAAddr, U32 u32_ByteCnt, int mode);
dma_addr_t eMMC_DMA_MAP_address(uintptr_t ulongBuffer, U32 u32_ByteCnt, int mode);

#if defined(ENABLE_eMMC_INTERRUPT_MODE) && ENABLE_eMMC_INTERRUPT_MODE
void        eMMC_INT_ClearMIEEvent(eMMC_IP_EmType emmc_ip);
irqreturn_t eMMC_FCIE_IRQ(int irq, void *dummy);
U32         eMMC_WaitCompleteIntr(eMMC_IP_EmType emmc_ip, uintptr_t u32_RegAddr, U16 u16_WaitEvent, U32 u32_MicroSec);
#endif
