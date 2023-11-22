/*
 * halFCIE.h- Sigmastar
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

#ifndef _HAL_FCIE_H_
#define _HAL_FCIE_H_
void HAL_FCIE3_write_bytes(u32 u32_offset, u16 u16_value);
u16  HAL_FCIE3_read_bytes(u32 u32_offset);
void HAL_FCIE_write_bytes(u32 u32_offset, u16 u16_value);
u16  HAL_FCIE_read_bytes(u32 u32_offset);
void HAL_SPI2FCIE_write_bytes(u32 u32_offset, u32 u32_value);
u16  HAL_SPI2FCIE_read_bytes(u32 u32_offset);
void HAL_FCIE_setup_clock(void);

#endif /* _HAL_FCIE_H_ */
