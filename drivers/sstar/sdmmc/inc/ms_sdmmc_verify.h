/*
 * ms_sdmmc_verify.h- Sigmastar
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
 * FileName ms_sdmmc_verify.h
 *     @author jeremy.wang (2013/07/26)
 * Desc:
 * 	   This file is the header file of ms_sdmmc_verify.c.
 *
 ***************************************************************************************************************/

#ifndef __MS_SDMMC_VERIFY_H
#define __MS_SDMMC_VERIFY_H

#include "hal_card_base.h"
#include <linux/platform_device.h>

#define A_DMA_W_BASE 0x20006000
#define A_DMA_R_BASE 0x20008000
#define A_ADMA_BASE  0x2000A000 // Not Support

//***********************************************************************************************************

void IPV_SDMMC_TimerTest(U8_T u8Sec);
void IPV_SDMMC_CardDetect(U8_T u8Slot);
void IPV_SDMMC_Init(U8_T u8Slot);
void IPV_SDMMC_SetWideBus(U8_T u8Slot);
void IPV_SDMMC_SetHighBus(U8_T u8Slot);
void IPV_SDMMC_SetClock(U8_T u8Slot, U32_T u32ReffClk, U8_T u8DownLevel);
void IPV_SDMMC_SetBusTiming(U8_T u8Slot, U8_T u8BusTiming);
void IPV_SDMMC_CIFD_RW(U8_T u8Slot, U32_T u32SecAddr, BOOL_T bHidden);
void IPV_SDMMC_DMA_RW(U8_T u8Slot, U32_T u32SecAddr, U16_T u16SecCount, BOOL_T bHidden);

//###########################################################################################################

//###########################################################################################################
void IPV_SDMMC_ADMA_RW(U8_T u8Slot, U32_T u32SecAddr, BOOL_T bHidden);
//###########################################################################################################

void IPV_SDMMC_BurnRW(U8_T u8Slot, U8_T u8TransType, U32_T u32StartSec, U32_T u32EndSec);
void IPV_SDMMC_TestPattern(U8_T u8Slot, U8_T u8Pattern);
void IPV_SDMMC_SetPAD(U8_T u8Slot, U8_T u8IP, U8_T u8Port, U8_T u8PAD);

void IPV_SDMMC_Verify(struct device *dev, U8_T u8Val1);
void IPV_SDMMC_PowerSavingModeVerify(U8_T u8Slot);
void SDMMC_SDIOinterrupt(IpOrder eIP);

#endif // End of __MS_SDMMC_VERIFY_H
