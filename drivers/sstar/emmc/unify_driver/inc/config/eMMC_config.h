/*
 * eMMC_config.h- Sigmastar
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

#ifndef __eMMC_CONFIG_H__
#define __eMMC_CONFIG_H__

#define UNIFIED_eMMC_DRIVER 1

//=====================================================
// select a HW platform:
//   - 1: enable, 0: disable.
//   - only one platform can be 1, others have to be 0.
//   - search and check all [FIXME] if need modify or not
//=====================================================

#ifndef U64
#define U64 unsigned long long
#endif
#ifndef U32
#define U32 unsigned int
#endif
#ifndef U16
#define U16 unsigned short
#endif
#ifndef U8
#define U8 unsigned char
#endif
#ifndef S64
#define S64 signed long long
#endif
#ifndef S32
#define S32 signed int
#endif
#ifndef S16
#define S16 signed short
#endif
#ifndef S8
#define S8 signed char
#endif

#ifndef BOOL
#define BOOL unsigned int
#endif
#ifndef TRUE
#define TRUE (1 == 1)
#endif
#ifndef FALSE
#define FALSE (!TRUE)
#endif

#include "ms_platform.h"

//=====================================================
// HW registers
//=====================================================
#define REG_FCIE_U16(Reg_Addr) (*(volatile U16*)(IO_ADDRESS(Reg_Addr)))
#define REG_OFFSET_SHIFT_BITS  (2)
#ifndef GET_REG_ADDR
#define GET_REG_ADDR(x, y) (x + (y << REG_OFFSET_SHIFT_BITS))
#endif

#define REG_FCIE(reg_addr)             REG_FCIE_U16(reg_addr)
#define REG_FCIE_W(reg_addr, val)      REG_FCIE(reg_addr) = (val)
#define REG_FCIE_R(reg_addr, val)      val = REG_FCIE(reg_addr)
#define REG_FCIE_SETBIT(reg_addr, val) REG_FCIE(reg_addr) |= (val)
#define REG_FCIE_CLRBIT(reg_addr, val) REG_FCIE(reg_addr) &= ~((U32)val)
#define REG_FCIE_W1C(reg_addr, val)    REG_FCIE_W(reg_addr, REG_FCIE(reg_addr) & (val))

//=====================================================
// misc. do NOT edit the following content.
//=====================================================
#define eMMC_DMA_RACING_PATCH    1
#define eMMC_DMA_PATCH_WAIT_TIME DELAY_10ms_in_us
#define eMMC_DMA_RACING_PATTERN0 (((U32)'M' << 24) | ((U32)0 << 16) | ((U32)'S' << 8) | (U32)1)
#define eMMC_DMA_RACING_PATTERN1 (((U32)'T' << 24) | ((U32)6 << 16) | ((U32)'A' << 8) | (U32)8)

//===========================================================
// Time Dalay, do NOT edit the following content
//===========================================================
#if defined(eMMC_UPDATE_FIRMWARE) && (eMMC_UPDATE_FIRMWARE)
#define TIME_WAIT_DAT0_HIGH           (HW_TIMER_DELAY_1s * 10)
#define TIME_WAIT_FCIE_RESET          (HW_TIMER_DELAY_1s * 10)
#define TIME_WAIT_FCIE_RST_TOGGLE_CNT (HW_TIMER_DELAY_1s * 10)
#define TIME_WAIT_FIFOCLK_RDY         (HW_TIMER_DELAY_10ms * 10)
#define TIME_WAIT_CMDRSP_END          (HW_TIMER_DELAY_10ms * 10)
#define TIME_WAIT_1_BLK_END           (HW_TIMER_DELAY_1s * 5)
#define TIME_WAIT_n_BLK_END           (HW_TIMER_DELAY_1s * 10) // safe for 512 blocks
#else
#define TIME_WAIT_DAT0_HIGH           (HW_TIMER_DELAY_1s * 10)
#define TIME_WAIT_ERASE_DAT0_HIGH     (HW_TIMER_DELAY_1s * 10)
#define TIME_WAIT_FCIE_RESET          HW_TIMER_DELAY_500ms
#define TIME_WAIT_FCIE_RST_TOGGLE_CNT HW_TIMER_DELAY_1us
#define TIME_WAIT_FIFOCLK_RDY         HW_TIMER_DELAY_10ms
#define TIME_WAIT_CMDRSP_END          HW_TIMER_DELAY_10ms
#define TIME_WAIT_1_BLK_END           (HW_TIMER_DELAY_1s * 1)
#define TIME_WAIT_n_BLK_END           (HW_TIMER_DELAY_1s * 2) // safe for 512 blocks
#endif

#ifdef CONFIG_OF
#define MSTAR_EMMC_CONFIG_OF 1
#else
#define MSTAR_EMMC_CONFIG_OF 0
#endif

#define eMMC_CACHE_LINE (0x20) // [FIXME]

//=====================================================
// tool-chain attributes
//===================================================== [FIXME] -->
#define eMMC_PACK0
#define eMMC_PACK1 __attribute__((__packed__))
#define eMMC_ALIGN0
#define eMMC_ALIGN1 __attribute__((aligned(eMMC_CACHE_LINE)))
// <-- [FIXME]

#endif /* __eMMC_CONFIG_H__ */
