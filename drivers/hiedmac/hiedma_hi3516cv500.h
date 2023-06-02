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

#ifndef __HIEDMA_HI3516CV500_H__
#define __HIEDMA_HI3516CV500_H__

#include "hiedmacv310.h"
#define EDMAC_MAX_PERIPHERALS  32
#define HIEDMAC_CHANNEL_NUM 8

#define UART4_REG_BASE          0x120A4000
#define UART3_REG_BASE          0x120A3000
#define UART2_REG_BASE          0x120A2000
#define UART1_REG_BASE          0x120A1000
#define UART0_REG_BASE          0x120A0000

#define UART0_DR    (UART0_REG_BASE + 0x0)
#define UART1_DR    (UART1_REG_BASE + 0x0)
#define UART2_DR    (UART2_REG_BASE + 0x0)
#define UART3_DR    (UART3_REG_BASE + 0x0)
#define UART4_DR    (UART4_REG_BASE + 0x0)

#define I2C7_REG_BASE             0x120B7000
#define I2C6_REG_BASE             0x120B6000
#define I2C5_REG_BASE             0x120B5000
#define I2C4_REG_BASE             0x120B4000
#define I2C3_REG_BASE             0x120B3000
#define I2C2_REG_BASE             0x120B2000
#define I2C1_REG_BASE             0x120B1000
#define I2C0_REG_BASE             0x120B0000

#define I2C0_TX_FIFO   (I2C0_REG_BASE + 0x20)
#define I2C0_RX_FIFO   (I2C0_REG_BASE + 0x24)

#define I2C1_TX_FIFO   (I2C1_REG_BASE + 0x20)
#define I2C1_RX_FIFO   (I2C1_REG_BASE + 0x24)

#define I2C2_TX_FIFO   (I2C2_REG_BASE + 0x20)
#define I2C2_RX_FIFO   (I2C2_REG_BASE + 0x24)

#define I2C3_TX_FIFO   (I2C3_REG_BASE + 0x20)
#define I2C3_RX_FIFO   (I2C3_REG_BASE + 0x24)

#define I2C4_TX_FIFO   (I2C4_REG_BASE + 0x20)
#define I2C4_RX_FIFO   (I2C4_REG_BASE + 0x24)

#define I2C5_TX_FIFO   (I2C5_REG_BASE + 0x20)
#define I2C5_RX_FIFO   (I2C5_REG_BASE + 0x24)

#define I2C6_TX_FIFO   (I2C6_REG_BASE + 0x20)
#define I2C6_RX_FIFO   (I2C6_REG_BASE + 0x24)

#define I2C7_TX_FIFO   (I2C7_REG_BASE + 0x20)
#define I2C7_RX_FIFO   (I2C7_REG_BASE + 0x24)

#define EDMAC_TX 0
#define EDMAC_RX 1

edmac_peripheral  g_peripheral[EDMAC_MAX_PERIPHERALS] = {
    {0, I2C0_RX_FIFO, DMAC_HOST0, (0x40000004), PERI_8BIT_MODE, 0},
    {1, I2C0_TX_FIFO, DMAC_HOST0, (0x80000004), PERI_8BIT_MODE, 0},
    {2, I2C1_RX_FIFO, DMAC_HOST0, (0x40000004), PERI_8BIT_MODE, 0},
    {3, I2C1_TX_FIFO, DMAC_HOST0, (0x80000004), PERI_8BIT_MODE, 0},
    {4, I2C2_RX_FIFO, DMAC_HOST0, (0x40000004), PERI_8BIT_MODE, 0},
    {5, I2C2_TX_FIFO, DMAC_HOST0, (0x80000004), PERI_8BIT_MODE, 0},
    {6, I2C3_RX_FIFO, DMAC_HOST0, (0x40000004), PERI_8BIT_MODE, 0},
    {7, I2C3_TX_FIFO, DMAC_HOST0, (0x80000004), PERI_8BIT_MODE, 0},
    {8, I2C4_RX_FIFO, DMAC_HOST0, (0x40000004), PERI_8BIT_MODE, 0},
    {9, I2C4_TX_FIFO, DMAC_HOST0, (0x80000004), PERI_8BIT_MODE, 0},
    {10, I2C5_RX_FIFO, DMAC_HOST0, (0x40000004), PERI_8BIT_MODE, 0},
    {11, I2C5_TX_FIFO, DMAC_HOST0, (0x80000004), PERI_8BIT_MODE, 0},
    {12, I2C6_RX_FIFO, DMAC_HOST0, (0x40000004), PERI_8BIT_MODE, 0},
    {13, I2C6_TX_FIFO, DMAC_HOST0, (0x80000004), PERI_8BIT_MODE, 0},
    {14, I2C7_RX_FIFO, DMAC_HOST0, (0x40000004), PERI_8BIT_MODE, 0},
    {15, I2C7_TX_FIFO, DMAC_HOST0, (0x80000004), PERI_8BIT_MODE, 0},
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
