/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

/**
 * @file z2.h
 * Header of the zhouyi v2 AIPU hardware control info
 */

#ifndef __Z2_H__
#define __Z2_H__

#include "zhouyi.h"

/**
 * Zhouyi v2 AIPU Specific Interrupts
 */
#define ZHOUYI_IRQ_FAULT  0x8

#define ZHOUYIV2_IRQ  (ZHOUYI_IRQ | ZHOUYI_IRQ_FAULT)
#define ZHOUYIV2_IRQ_ENABLE_FLAG  (ZHOUYIV2_IRQ)
#define ZHOUYIV2_IRQ_DISABLE_FLAG (ZHOUYI_IRQ_NONE)

#define ZHOUYI_V2_MAX_SCHED_JOB_NUM  1

#define ZHOUYI_V2_ASE_READ_ENABLE           (1<<31)
#define ZHOUYI_V2_ASE_WRITE_ENABLE          (1<<30)
#define ZHOUYI_V2_ASE_RW_ENABLE             (3<<30)

/**
 * Zhouyi v2 AIPU Specific Host Control Register Map
 */
#define AIPU_DATA_ADDR_2_REG_OFFSET         0x1C
#define AIPU_DATA_ADDR_3_REG_OFFSET         0x20
#define AIPU_SECURE_CONFIG_REG_OFFSET       0x30
#define AIPU_POWER_CTRL_REG_OFFSET          0x38
#define AIPU_ADDR_EXT0_CTRL_REG_OFFSET      0xC0
#define AIPU_ADDR_EXT0_HIGH_BASE_REG_OFFSET 0xC4
#define AIPU_ADDR_EXT0_LOW_BASE_REG_OFFSET  0xC8
#define AIPU_ADDR_EXT1_CTRL_REG_OFFSET      0xCC
#define AIPU_ADDR_EXT1_HIGH_BASE_REG_OFFSET 0xD0
#define AIPU_ADDR_EXT1_LOW_BASE_REG_OFFSET  0xD4
#define AIPU_ADDR_EXT2_CTRL_REG_OFFSET      0xD8
#define AIPU_ADDR_EXT2_HIGH_BASE_REG_OFFSET 0xDC
#define AIPU_ADDR_EXT2_LOW_BASE_REG_OFFSET  0xE0
#define AIPU_ADDR_EXT3_CTRL_REG_OFFSET      0xE4
#define AIPU_ADDR_EXT3_HIGH_BASE_REG_OFFSET 0xE8
#define AIPU_ADDR_EXT3_LOW_BASE_REG_OFFSET  0xEC
#define ZHOUYI_V2_MAX_REG_OFFSET            0xEC

#endif /* __Z2_H__ */
