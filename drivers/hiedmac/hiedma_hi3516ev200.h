/*
 * Copyright (c) 2017-2018 HiSilicon Technologies Co., Ltd.
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

#ifndef __HIEDMA_HI3516EV200_H__
#define __HIEDMA_HI3516EV200_H__

#include "hiedmacv310.h"
#define EDMAC_MAX_PERIPHERALS  32
#define HIEDMAC_CHANNEL_NUM 4

#define UART2_REG_BASE          0x12042000
#define UART1_REG_BASE          0x12041000
#define UART0_REG_BASE          0x12040000

#define UART0_DR    (UART0_REG_BASE + 0x0)
#define UART1_DR    (UART1_REG_BASE + 0x0)
#define UART2_DR    (UART2_REG_BASE + 0x0)

#define I2C2_REG_BASE             0x12062000
#define I2C1_REG_BASE             0x12061000
#define I2C0_REG_BASE             0x12060000

#define I2C0_TX_FIFO   (I2C0_REG_BASE + 0x20)
#define I2C0_RX_FIFO   (I2C0_REG_BASE + 0x24)

#define I2C1_TX_FIFO   (I2C1_REG_BASE + 0x20)
#define I2C1_RX_FIFO   (I2C1_REG_BASE + 0x24)

#define I2C2_TX_FIFO   (I2C2_REG_BASE + 0x20)
#define I2C2_RX_FIFO   (I2C2_REG_BASE + 0x24)

#define EDMAC_TX 0
#define EDMAC_RX 1

edmac_peripheral  g_peripheral[EDMAC_MAX_PERIPHERALS] = {
    {0, I2C0_RX_FIFO, DMAC_HOST0, (0x40000004), PERI_8BIT_MODE, 0},
    {1, I2C0_TX_FIFO, DMAC_HOST0, (0x80000004), PERI_8BIT_MODE, 0},
    {2, I2C1_RX_FIFO, DMAC_HOST0, (0x40000004), PERI_8BIT_MODE, 0},
    {3, I2C1_TX_FIFO, DMAC_HOST0, (0x80000004), PERI_8BIT_MODE, 0},
    {4, I2C2_RX_FIFO, DMAC_HOST0, (0x40000004), PERI_8BIT_MODE, 0},
    {5, I2C2_TX_FIFO, DMAC_HOST0, (0x80000004), PERI_8BIT_MODE, 0},
    {6, 0, DMAC_NOT_USE, 0, 0, 0},
    {7, 0, DMAC_NOT_USE, 0, 0, 0},
    {8, 0, DMAC_NOT_USE, 0, 0, 0},
    {9, 0, DMAC_NOT_USE, 0, 0, 0},
    {10, 0, DMAC_NOT_USE, 0, 0, 0},
    {11, 0, DMAC_NOT_USE, 0, 0, 0},
    {12, 0, DMAC_NOT_USE, 0, 0, 0},
    {13, 0, DMAC_NOT_USE, 0, 0, 0},
    {14, 0, DMAC_NOT_USE, 0, 0, 0},
    {15, 0, DMAC_NOT_USE, 0, 0, 0},
    {16, 0, DMAC_NOT_USE, 0, 0, 0},
    {17, 0, DMAC_NOT_USE, 0, 0, 0},
    {18, 0, DMAC_NOT_USE, 0, 0, 0},
    {19, 0, DMAC_NOT_USE, 0, 0, 0},
    {20, 0, DMAC_NOT_USE, 0, 0, 0},
    {21, 0, DMAC_NOT_USE, 0, 0, 0},
    {22, 0, DMAC_NOT_USE, 0, 0, 0},
    {23, 0, DMAC_NOT_USE, 0, 0, 0},
    {24, 0, DMAC_NOT_USE, 0, 0, 0},
    {25, 0, DMAC_NOT_USE, 0, 0, 0},
    {26, 0, DMAC_NOT_USE, 0, 0, 0},
    {27, 0, DMAC_NOT_USE, 0, 0, 0},
    {28, 0, DMAC_NOT_USE, 0, 0, 0},
    {29, 0, DMAC_NOT_USE, 0, 0, 0},
    {30, 0, DMAC_NOT_USE, 0, 0, 0},
    {31, 0, DMAC_NOT_USE, 0, 0, 0},
};
#endif
