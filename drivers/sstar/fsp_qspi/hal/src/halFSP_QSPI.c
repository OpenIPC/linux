/*
 * halFSP_QSPI.c- Sigmastar
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

#include <halFSP_QSPI.h>

u32 g_u32_fsp_address  = 0;
u32 g_u32_qspi_address = 0;

void HAL_FSP_QSPI_BankSel(void)
{
#ifdef CONFIG_FSP_QSPI_DUAL_BANK
    u16 u16BoundID;
    // chekc boundID to select pm-spi or non-pm spi
    u16BoundID = *(unsigned short volatile *)(RIU_BASE_ADDR + CHIP_BASE_ADDR + BONDING_VAL_ADDR) & CHIP_BOUND_TYPES;
    if (u16BoundID != CHIP_BOUND_QFN88)
    {
        g_u32_fsp_address  = FSP_BASE_ADDR;
        g_u32_qspi_address = QSPI_BASE_ADDR;
    }
    else
#endif
    {
        g_u32_fsp_address  = FSP_PMBASE_ADDR;
        g_u32_qspi_address = QSPI_PMBASE_ADDR;
    }
}

void HAL_REG_Write2Byte(u32 u32_address, u16 u16_val)
{
    *(unsigned short volatile *)(RIU_BASE_ADDR + u32_address) = u16_val;
}

void HAL_REG_WriteByte(u32 u32_address, u8 u8_val)
{
    *(unsigned char volatile *)(RIU_BASE_ADDR + u32_address) = u8_val;
}

void HAL_PIU_NONPM_Write2Byte_Mask(u32 u32_address, u16 u16_val, u16 u16_mask)
{
    u32 u16_value;

    u16_value = (*(unsigned short volatile *)(RIU_BASE_ADDR + PIU_NONPM_BASE_ADDR + u32_address));
    u16_value = (u16_value & ~u16_mask) | (u16_val & u16_mask);
    *(unsigned short volatile *)(RIU_BASE_ADDR + PIU_NONPM_BASE_ADDR + u32_address) = u16_value;
}

void HAL_CHIP_Write2Byte(u32 u32_address, u16 u16_val)
{
    *(unsigned short volatile *)(RIU_BASE_ADDR + CHIP_BASE_ADDR + (u32_address << 2)) = u16_val;
}

void HAL_PM_SLEEP_Write2Byte(u32 u32_address, u16 u16_val)
{
    *(unsigned short volatile *)(RIU_BASE_ADDR + PM_SLEEP_BASE_ADDR + (u32_address << 2)) = u16_val;
}

void HAL_FSP_WriteByte(u32 u32_address, u8 u8_val)
{
    *(unsigned char volatile *)(RIU_BASE_ADDR + g_u32_fsp_address + u32_address) = u8_val;
}

void HAL_FSP_Write2Byte(u32 u32_address, u16 u16_val)
{
    *(unsigned short volatile *)(RIU_BASE_ADDR + g_u32_fsp_address + u32_address) = u16_val;
}

void HAL_QSPI_WriteByte(u32 u32_address, u8 u8_val)
{
    *(unsigned char volatile *)(RIU_BASE_ADDR + g_u32_qspi_address + u32_address) = u8_val;
}

void HAL_ISP_Write2Byte_Mask(u32 u32_address, u16 u16_val, u16 u16_mask)
{
    u32 u16_reg_val;

    u16_reg_val = (*(unsigned short volatile *)(RIU_BASE_ADDR + ISP_PMBASE_ADDR + u32_address));
    u16_reg_val = (u16_reg_val & ~u16_mask) | (u16_val & u16_mask);
    *(unsigned short volatile *)(RIU_BASE_ADDR + ISP_PMBASE_ADDR + u32_address) = u16_reg_val;
}

void HAL_QSPI_Write2Byte(u32 u32_address, u16 u16_val)
{
    *(unsigned short volatile *)(RIU_BASE_ADDR + g_u32_qspi_address + u32_address) = u16_val;
}

void HAL_QSPI_Write2Byte_Mask(u32 u32_address, u16 u16_val, u16 u16_mask)
{
    u32 u16_reg_timeout_val;

    u16_reg_timeout_val = (*(unsigned short volatile *)(RIU_BASE_ADDR + g_u32_qspi_address + u32_address));
    u16_reg_timeout_val = (u16_reg_timeout_val & ~u16_mask) | (u16_val & u16_mask);
    *(unsigned short volatile *)(RIU_BASE_ADDR + g_u32_qspi_address + u32_address) = u16_reg_timeout_val;
}

u8 HAL_QSPI_ReadByte(u32 u32_address)
{
    return (*(unsigned char volatile *)(RIU_BASE_ADDR + g_u32_qspi_address + u32_address));
}
u16 HAL_FSP_Read2Byte(u32 u32_address)
{
    return (*(unsigned short volatile *)(RIU_BASE_ADDR + g_u32_fsp_address + u32_address));
}

u8 HAL_FSP_ReadByte(u32 u32_address)
{
    return (*(unsigned char volatile *)(RIU_BASE_ADDR + g_u32_fsp_address + u32_address));
}
