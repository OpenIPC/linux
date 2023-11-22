/*
 * hal_mspi.h- Sigmastar
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

#ifndef __MHAL_MSPI_H__
#define __MHAL_MSPI_H__

#define MSPI_MAX_SUPPORT_BITS 16

struct mspi_hal
{
    u64               mspi_base;
    u32               pad_ctrl;
    u32               clk_out_mode;
    u32               cs_num;
    u32               cs_ext[16];
    u32               bits_per_word;
    struct completion done;
};
typedef enum
{
    E_MSPI_CHL0,
    E_MSPI_CHL1,
    E_MSPI_CHL2,
    E_MSPI_CHL3,
    E_MSPI_MAX,
} MSPI_CHL_e;

typedef enum
{
    E_MSPI_MODE0, // CPOL = 0,CPHA =0
    E_MSPI_MODE1, // CPOL = 0,CPHA =1
    E_MSPI_MODE2, // CPOL = 1,CPHA =0
    E_MSPI_MODE3, // CPOL = 1,CPHA =1
    E_MSPI_MODE_MAX,
} MSPI_Mode_Config_e;

typedef enum
{
    E_MSPI_BIT_MSB_FIRST,
    E_MSPI_BIT_LSB_FIRST,
} MSPI_BitSeq_e;

u8   HAL_MSPI_CheckDmaMode(u8 u8Channel);
u8   HAL_MSPI_Config(struct mspi_hal *mspi);
u16  HAL_MSPI_CheckDone(struct mspi_hal *mspi);
void HAL_MSPI_ClearDone(struct mspi_hal *mspi);
u8   HAL_MSPI_SetLSB(struct mspi_hal *mspi, u8 enable);
u8   HAL_MSPI_Set3WireMode(struct mspi_hal *mspi, u8 enable);
u8   HAL_MSPI_SetDivClk(struct mspi_hal *mspi, u8 div);
u8   HAL_MSPI_SetMode(struct mspi_hal *mspi, MSPI_Mode_Config_e eMode);
void HAL_MSPI_ChipSelect(struct mspi_hal *mspi, u8 Enable, u8 eSelect);
u8   HAL_MSPI_Read(u8 u8Channel, struct mspi_hal *mspi, u8 *pData, u16 u16Size);
u8   HAL_MSPI_Write(u8 u8Channel, struct mspi_hal *mspi, u8 *pData, u16 u16Size);
u8   HAL_MSPI_DMA_Write(u8 u8Channel, struct mspi_hal *mspi, u8 *pData, u32 u32Size);
u8   HAL_MSPI_DMA_Read(u8 u8Channel, struct mspi_hal *mspi, u8 *pData, u32 u32Size);
u8   HAL_MSPI_FullDuplex(u8 u8Channel, struct mspi_hal *mspi, u8 *rx_buff, u8 *tx_buff, u16 u16Size);
u8   HAL_MSPI_SET_FRAMECFG(struct mspi_hal *mspi, int bits_per_word);

#endif
