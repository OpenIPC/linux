/*
 * hal_iic_reg.h- Sigmastar
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

#ifndef _HAL_IIC_REG_H_
#define _HAL_IIC_REG_H_

#include "mdrv_types.h"

#define I2C_REGMASK_BASE (0x0001)
#define SHIFT_BIT0       (0)
#define SHIFT_BIT1       (1)
#define SHIFT_BIT2       (2)
#define SHIFT_BIT3       (3)
#define SHIFT_BIT4       (4)
#define SHIFT_BIT5       (5)
#define SHIFT_BIT6       (6)
#define SHIFT_BIT7       (7)
#define SHIFT_BIT8       (8)
#define SHIFT_BIT9       (9)
#define SHIFT_BIT10      (10)
#define SHIFT_BIT11      (11)
#define SHIFT_BIT12      (12)
#define SHIFT_BIT13      (13)
#define SHIFT_BIT14      (14)
#define SHIFT_BIT15      (15)

#define REG_I2C_CONFIG          (0x0000)
#define REG_MASK_CFG_RST        BIT0
#define REG_MASK_CFG_DMA        BIT1
#define REG_MASK_CFG_ENINT      BIT2
#define REG_MASK_CFG_CLKSTRECH  BIT3
#define REG_MASK_CFG_TIMOUT_INT BIT4
#define REG_MASK_CFG_ENFILTER   BIT5
#define REG_MASK_CFG_OEN        BIT6
#define REG_MASK_CFG_ENERRDET   BIT7

#define REG_I2C_STAR_STOP  (0x0001)
#define REG_MASK_CMD_START BIT0
#define REG_MASK_CMD_STOP  BIT8

#define REG_I2C_INT_FLAG  (0x0004)
#define REG_MASK_INT_FLAG BIT0

#define REG_I2C_WRITE_DATA        (0x0002)
#define REG_MASK_WRITE_DATA       (0x00FF)
#define REG_MASK_WRITE_ACK_FRMSLV BIT8

#define REG_I2C_READ_DATA          (0x0003)
#define REG_MASK_READ_DATA         (0x00FF)
#define REG_MASK_READ_DATA_TRIGGER BIT8
#define REG_MASK_READ_ACK_TOSLV    BIT9

#define REG_I2C_INT_STATUS      (0x0005)
#define REG_MASK_INTSTU_STATE   (BIT0 | BIT1 | BIT2 | BIT3 | BIT4)
#define REG_MASK_INTSTU_START   BIT8
#define REG_MASK_INTSTU_STOP    BIT9
#define REG_MASK_INTSTU_RXDONE  BIT10
#define REG_MASK_INTSTU_TXDONE  BIT11
#define REG_MASK_INTSTU_TIMEOUT BIT14

#define REG_I2C_CNT_FOR_STOP           (0x0008)
#define REG_I2C_CNT_HIGH_PERIOD        (0x0009)
#define REG_I2C_CNT_LOW_PERIOD         (0x000A)
#define REG_I2C_CNT_BTWN_FALEDGE       (0x000B)
#define REG_I2C_CNT_FOR_START          (0x000C)
#define REG_I2C_CNT_DATA_LATCHTIM      (0x000D)
#define REG_I2C_CNT_DELYTIMOUT_INTOCUR (0x000E)

#define REG_I2C_DMA_CONFIG      (0x0020)
#define REG_MASK_DMACFG_INTFLAG BIT2
#define REG_MASK_DMACFG_SOFTRST BIT1
#define REG_MASK_DMACFG_MIURST  BIT3
#define REG_MASK_DMACFG_MIUPRI  BIT4

#define REG_I2C_BUFDRAMADDR_LOW  (0x0021)
#define REG_I2C_BUFDRAMADDR_HIGH (0x0022)
#define REG_I2C_BUFDRAMADDR_MSB  (0x0023)
#define REG_MASK_IIC_ADDR_MSB    (0x000F)

#define REG_I2C_DMA_MIUCHANL_SEL    (0x0023)
#define REG_I2C_STOP_FORMAT_DISABLE (0x0023)
#define REG_I2C_RDWR_FORMAT         (0x0023)
#define REG_MASK_DMA_MIUCHANLSEL    BIT7
#define REG_MASK_STPFMT_DIS         BIT5
#define REG_MASK_RDWR_FMT           BIT6

#define REG_I2C_DMA_TRANSFER_DONE (0x0024)
#define REG_MASK_DMA_TRSFER_DONE  BIT0

#define REG_I2C_DMA_CMD_DATA_25H (0x0025)

#define REG_I2C_DMA_CMD_LEN_29H (0x0029)
#define REG_MASK_DMA_CMDLEN     (0x000F)

#define REG_I2C_TRANS_DATA_LEN_LOW  (0x002A)
#define REG_I2C_TRANS_DATA_LEN_HIGH (0x002B)
#define REG_I2C_DMA_TC_2CH          (0x002C)
#define REG_I2C_DMA_TC_2DH          (0x002D)

#define REG_I2C_DMA_10BITMODE     (0x002E)
#define REG_I2C_DMA_SLAVEADDR     (0x002E)
#define REG_MASK_DMASLVADDR       (0x3FF)
#define REG_MASK_DMASLVADDR_10BIT (0x3FF)
#define REG_MASK_DMASLVADDR_NORM  (0x7f)
#define REG_MASK_DMABITMODE       BIT10

#define REG_I2C_DMA_TRIGGER (0x002F)
#define REG_MASK_DMATRIG    (BIT0)

#define REG_I2C_DMA_RESERVED (0x0030)
#define REG_MASK_LASTDONE    (0x00FF)

#define RET_I2C_DMA_DATA_LEN (0x0032)
#define REG_MASK_DMA_DATALEN (0x000F)
/**************************************************/

#define HAL_I2CONFIG_RST           REG_MASK_CFG_RST
#define HAL_I2CONFIG_DMA_EN        REG_MASK_CFG_DMA
#define HAL_I2CONFIG_INT_EN        REG_MASK_CFG_ENINT
#define HAL_I2CONFIG_CLKSTRE_EN    REG_MASK_CFG_CLKSTRECH
#define HAL_I2CONFIG_TIMOUT_INT_EN REG_MASK_CFG_TIMOUT_INT
#define HAL_I2CONFIG_FILTER_EN     REG_MASK_CFG_ENFILTER
#define HAL_I2CONFIG_OEN_PUSH_EN   REG_MASK_CFG_OEN
#define HAL_I2CONFIG_ERRDET_EN     REG_MASK_CFG_ENERRDET

#endif
