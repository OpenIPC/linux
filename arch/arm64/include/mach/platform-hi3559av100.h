/*
 * Copyright (c) 2018 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __HI_CHIP_REGS_H__
#define __HI_CHIP_REGS_H__

#define GET_SYS_BOOT_MODE(_reg)        (((_reg) >> 4) & 0x3)
#define BOOT_FROM_SPI                  0
#define BOOT_FROM_NAND                 1
#endif /* End of __HI_CHIP_REGS_H__ */
