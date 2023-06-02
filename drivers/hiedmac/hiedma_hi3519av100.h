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

#ifndef __HIEDMA_HI3519AV100_H__
#define __HIEDMA_HI3519AV100_H__

#include "hiedmacv310.h"
#define EDMAC_MAX_PERIPHERALS  64
#define HIEDMAC_CHANNEL_NUM 8

#define UART8_REG_BASE          0x04548000
#define UART7_REG_BASE          0x04547000
#define UART6_REG_BASE          0x04546000
#define UART5_REG_BASE          0x04545000
#define UART4_REG_BASE          0x04544000
#define UART3_REG_BASE          0x04543000
#define UART2_REG_BASE          0x04542000
#define UART1_REG_BASE          0x04541000
#define UART0_REG_BASE          0x04540000

#define UART0_DR    (UART0_REG_BASE + 0x0)
#define UART1_DR    (UART1_REG_BASE + 0x0)
#define UART2_DR    (UART2_REG_BASE + 0x0)
#define UART3_DR    (UART3_REG_BASE + 0x0)
#define UART4_DR    (UART4_REG_BASE + 0x0)
#define UART5_DR    (UART4_REG_BASE + 0x0)
#define UART6_DR    (UART4_REG_BASE + 0x0)
#define UART7_DR    (UART4_REG_BASE + 0x0)
#define UART8_DR    (UART4_REG_BASE + 0x0)

#define I2C9_REG_BASE             0x04569000
#define I2C8_REG_BASE             0x04568000
#define I2C7_REG_BASE             0x04567000
#define I2C6_REG_BASE             0x04566000
#define I2C5_REG_BASE             0x04565000
#define I2C4_REG_BASE             0x04564000
#define I2C3_REG_BASE             0x04563000
#define I2C2_REG_BASE             0x04562000
#define I2C1_REG_BASE             0x04561000
#define I2C0_REG_BASE             0x04560000

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

#define I2C8_TX_FIFO   (I2C8_REG_BASE + 0x20)
#define I2C8_RX_FIFO   (I2C8_REG_BASE + 0x24)

#define I2C9_TX_FIFO   (I2C9_REG_BASE + 0x20)
#define I2C9_RX_FIFO   (I2C9_REG_BASE + 0x24)


#define EDMAC_TX 0
#define EDMAC_RX 1

edmac_peripheral  g_peripheral[EDMAC_MAX_PERIPHERALS] = {
    {0, 0, DMAC_NOT_USE, 0, PERI_8BIT_MODE, 0},
    {1, 0, DMAC_NOT_USE, 0, PERI_8BIT_MODE, 0},
    {2, 0, DMAC_NOT_USE, 0, PERI_8BIT_MODE, 0},
    {3, 0, DMAC_NOT_USE, 0, PERI_8BIT_MODE, 0},
    {4, 0, DMAC_NOT_USE, 0, PERI_8BIT_MODE, 0},
    {5, 0, DMAC_NOT_USE, 0, PERI_8BIT_MODE, 0},
    {6, 0, DMAC_NOT_USE, 0, PERI_8BIT_MODE, 0},
    {7, 0, DMAC_NOT_USE, 0, PERI_8BIT_MODE, 0},
    {8, 0, DMAC_NOT_USE, 0, PERI_8BIT_MODE, 0},
    {9, 0, DMAC_NOT_USE, 0, PERI_8BIT_MODE, 0},
    {10, 0, DMAC_NOT_USE, 0, PERI_8BIT_MODE, 0},
    {11, 0, DMAC_NOT_USE, 0, PERI_8BIT_MODE, 0},
    {12, 0, DMAC_NOT_USE, 0, PERI_8BIT_MODE, 0},
    {13, 0, DMAC_NOT_USE, 0, PERI_8BIT_MODE, 0},
    {14, I2C0_TX_FIFO, DMAC_HOST1, (0x80000004), PERI_8BIT_MODE, 0},
    {15, I2C0_RX_FIFO, DMAC_HOST1, (0x40000004), PERI_8BIT_MODE, 0},
    {16, I2C1_TX_FIFO, DMAC_HOST1, (0x80000004), PERI_8BIT_MODE, 0},
    {17, I2C1_RX_FIFO, DMAC_HOST1, (0x40000004), PERI_8BIT_MODE, 0},
    {18, I2C2_TX_FIFO, DMAC_HOST1, (0x80000004), PERI_8BIT_MODE, 0},
    {19, I2C2_RX_FIFO, DMAC_HOST1, (0x40000004), PERI_8BIT_MODE, 0},
    {20, I2C3_TX_FIFO, DMAC_HOST1, (0x80000004), PERI_8BIT_MODE, 0},
    {21, I2C3_RX_FIFO, DMAC_HOST1, (0x40000004), PERI_8BIT_MODE, 0},
    {22, I2C4_TX_FIFO, DMAC_HOST1, (0x80000004), PERI_8BIT_MODE, 0},
    {23, I2C4_RX_FIFO, DMAC_HOST1, (0x40000004), PERI_8BIT_MODE, 0},
    {24, I2C5_TX_FIFO, DMAC_HOST1, (0x80000004), PERI_8BIT_MODE, 0},
    {25, I2C5_RX_FIFO, DMAC_HOST1, (0x40000004), PERI_8BIT_MODE, 0},
    {26, I2C6_TX_FIFO, DMAC_HOST1, (0x80000004), PERI_8BIT_MODE, 0},
    {27, I2C6_RX_FIFO, DMAC_HOST1, (0x40000004), PERI_8BIT_MODE, 0},
    {28, I2C7_TX_FIFO, DMAC_HOST1, (0x80000004), PERI_8BIT_MODE, 0},
    {29, I2C7_RX_FIFO, DMAC_HOST1, (0x40000004), PERI_8BIT_MODE, 0},
    {30, I2C8_TX_FIFO, DMAC_HOST1, (0x80000004), PERI_8BIT_MODE, 0},
    {31, I2C8_RX_FIFO, DMAC_HOST1, (0x40000004), PERI_8BIT_MODE, 0},
    {32, I2C9_TX_FIFO, DMAC_HOST1, (0x80000004), PERI_8BIT_MODE, 0},
    {33, I2C9_RX_FIFO, DMAC_HOST1, (0x40000004), PERI_8BIT_MODE, 0},
    {34, 0, DMAC_NOT_USE, 0, 0, 0},
    {35, 0, DMAC_NOT_USE, 0, 0, 0},
    {36, 0, DMAC_NOT_USE, 0, 0, 0},
    {37, 0, DMAC_NOT_USE, 0, 0, 0},
    {38, 0, DMAC_NOT_USE, 0, 0, 0},
    {39, 0, DMAC_NOT_USE, 0, 0, 0},
    {40, 0, DMAC_NOT_USE, 0, 0, 0},
    {41, 0, DMAC_NOT_USE, 0, 0, 0},
    {42, 0, DMAC_NOT_USE, 0, 0, 0},
    {43, 0, DMAC_NOT_USE, 0, 0, 0},
    {44, 0, DMAC_NOT_USE, 0, 0, 0},
    {45, 0, DMAC_NOT_USE, 0, 0, 0},
    {46, 0, DMAC_NOT_USE, 0, 0, 0},
    {47, 0, DMAC_NOT_USE, 0, 0, 0},
    {48, 0, DMAC_NOT_USE, 0, 0, 0},
    {49, 0, DMAC_NOT_USE, 0, 0, 0},
    {50, 0, DMAC_NOT_USE, 0, 0, 0},
    {51, 0, DMAC_NOT_USE, 0, 0, 0},
    {52, 0, DMAC_NOT_USE, 0, 0, 0},
    {53, 0, DMAC_NOT_USE, 0, 0, 0},
    {54, 0, DMAC_NOT_USE, 0, 0, 0},
    {55, 0, DMAC_NOT_USE, 0, 0, 0},
    {56, 0, DMAC_NOT_USE, 0, 0, 0},
    {57, 0, DMAC_NOT_USE, 0, 0, 0},
    {58, 0, DMAC_NOT_USE, 0, 0, 0},
    {59, 0, DMAC_NOT_USE, 0, 0, 0},
    {60, 0, DMAC_NOT_USE, 0, 0, 0},
    {61, 0, DMAC_NOT_USE, 0, 0, 0},
    {62, 0, DMAC_NOT_USE, 0, 0, 0},
    {63, 0, DMAC_NOT_USE, 0, 0, 0},
};
#endif
