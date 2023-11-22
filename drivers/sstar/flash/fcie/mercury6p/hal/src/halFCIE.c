/*
 * halFCIE.c- Sigmastar
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

#include <mdrvFlashOsImpl.h>
#include <halFCIE_reg.h>

void HAL_FCIE3_write_bytes(u32 u32_offset, u16 u16_value)
{
    *(unsigned short volatile *)(FCIE3_BANK_ADDR + (u32_offset << 2)) = (u16_value & 0xffff);
}

u16 HAL_FCIE3_read_bytes(u32 u32_offset)
{
    return *((unsigned short volatile *)(FCIE3_BANK_ADDR + (u32_offset << 2)));
}

void HAL_FCIE_write_bytes(u32 u32_offset, u16 u16_value)
{
    *(unsigned short volatile *)(FCIE_BANK_ADDR + (u32_offset << 2)) = (u16_value & 0xffff);
}

u16 HAL_FCIE_read_bytes(u32 u32_offset)
{
    return *(unsigned short volatile *)(FCIE_BANK_ADDR + (u32_offset << 2));
}

void HAL_SPI2FCIE_write_bytes(u32 u32_offset, u32 u32_value)
{
    *(unsigned short volatile *)(SPI2FCIE_BANK_ADDR + (u32_offset << 2)) = (u32_value & 0xffff);
}

u16 HAL_SPI2FCIE_read_bytes(u32 u32_offset)
{
    return *(unsigned short volatile *)(SPI2FCIE_BANK_ADDR + (u32_offset << 2));
}

void HAL_FCIE_setup_clock(void)
{
#if 0
    *(unsigned short volatile *)(RIU_BASE_ADDR + (CLKGEN_BASE_ADDR << 1) + (0x44 << 2)) = (0x02 << 2);
	printf("clk 0x1038 4b = 0x%02x\r\n", *(unsigned short volatile *)(RIU_BASE_ADDR + (CLKGEN_BASE_ADDR << 1) + (0x4b << 2)));
	*(unsigned short volatile *)(RIU_BASE_ADDR + (CLKGEN_BASE_ADDR << 1) + (0x4b << 2)) = 0x04;
	printf("clk 0x1038 4b = 0x%02x\r\n", *(unsigned short volatile *)(RIU_BASE_ADDR + (CLKGEN_BASE_ADDR << 1) + (0x4b << 2)));
	*(unsigned short volatile *)(RIU_BASE_ADDR + (CLKGEN_BASE_ADDR << 1) + (0x4b << 2)) |= 0x40;
	printf("clk 0x1038 4b = 0x%02x\r\n", *(unsigned short volatile *)(RIU_BASE_ADDR + (CLKGEN_BASE_ADDR << 1) + (0x4b << 2)));
	*(unsigned short volatile *)(RIU_BASE_ADDR + (0x113300 << 1) + (0x25 << 2)) |= 0x20;
#endif
}
