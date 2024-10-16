/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

/**
 * @file junor2.h
 * Header of the Juno r2 SoC registers & operations
 */

#ifndef __JUNOR2_H__
#define __JUNOR2_H__

#include "aipu_io.h"

/* bandwidth profiling */
#define JUNOR2_WORK_STAT_START_REQ            0x140
#define JUNOR2_WORK_STAT_END_REQ              0x144
#define JUNOR2_ALL_RDATA_TOT_LSB              0x200
#define JUNOR2_ALL_RDATA_TOT_MSB              0x204
#define JUNOR2_ALL_WDATA_TOT_LSB              0x208
#define JUNOR2_ALL_WDATA_TOT_MSB              0x20C
#define JUNOR2_TOT_CYCLE_LSB                  0x220
#define JUNOR2_TOT_CYCLE_MSB                  0x224
#define JUNOR2_ID_LATENCY_MAX_MSB             0x280
#define JUNOR2_ID_LATENCY_MAX_LSB             0x284
#define JUNOR2_ID_LATENCY_SINGLE              0x288
#define JUNOR2_DMA_LATENCY_TOT_MSB            0x290
#define JUNOR2_DMA_LATENCY_TOT_LSB            0x294
#define JUNOR2_DMA_RDATA_TOT_MSB              0x298
#define JUNOR2_DMA_RDATA_TOT_LSB              0x29C
#define JUNOR2_DMA_AR_HANDSHAKE_MSB           0x2A0
#define JUNOR2_DMA_AR_HANDSHAKE_LSB           0x2A4
#define JUNOR2_MAX_OUTSTAND                   0x2A8
#define JUNOR2_LATENCY_TOT_ID0_MSB            0x300
#define JUNOR2_LATENCY_TOT_ID0_LSB            0x304
#define JUNOR2_LATENCY_TOT_ID1_MSB            0x308
#define JUNOR2_LATENCY_TOT_ID1_LSB            0x30C
#define JUNOR2_LATENCY_TOT_ID2_MSB            0x310
#define JUNOR2_LATENCY_TOT_ID2_LSB            0x314
#define JUNOR2_LATENCY_TOT_ID3_MSB            0x318
#define JUNOR2_LATENCY_TOT_ID3_LSB            0x31C
#define JUNOR2_LATENCY_TOT_ID4_MSB            0x320
#define JUNOR2_LATENCY_TOT_ID4_LSB            0x324
#define JUNOR2_LATENCY_TOT_ID5_MSB            0x328
#define JUNOR2_LATENCY_TOT_ID5_LSB            0x32C
#define JUNOR2_LATENCY_TOT_ID6_MSB            0x330
#define JUNOR2_LATENCY_TOT_ID6_LSB            0x334
#define JUNOR2_LATENCY_TOT_ID7_MSB            0x338
#define JUNOR2_LATENCY_TOT_ID7_LSB            0x33C
#define JUNOR2_LATENCY_TOT_ID8_MSB            0x340
#define JUNOR2_LATENCY_TOT_ID8_LSB            0x344
#define JUNOR2_LATENCY_TOT_ID9_MSB            0x348
#define JUNOR2_LATENCY_TOT_ID9_LSB            0x34C
#define JUNOR2_LATENCY_TOT_ID10_MSB           0x350
#define JUNOR2_LATENCY_TOT_ID10_LSB           0x354
#define JUNOR2_LATENCY_TOT_ID11_MSB           0x358
#define JUNOR2_LATENCY_TOT_ID11_LSB           0x35C
#define JUNOR2_LATENCY_TOT_ID12_MSB           0x360
#define JUNOR2_LATENCY_TOT_ID12_LSB           0x364
#define JUNOR2_LATENCY_TOT_ID13_MSB           0x368
#define JUNOR2_LATENCY_TOT_ID13_LSB           0x36C
#define JUNOR2_LATENCY_TOT_ID14_MSB           0x370
#define JUNOR2_LATENCY_TOT_ID14_LSB           0x374
#define JUNOR2_LATENCY_TOT_ID15_MSB           0x378
#define JUNOR2_LATENCY_TOT_ID15_LSB           0x37C
#define JUNOR2_LATENCY_MAX_ID0                0x3C0
#define JUNOR2_LATENCY_MAX_ID1                0x3C4
#define JUNOR2_LATENCY_MAX_ID2                0x3C8
#define JUNOR2_LATENCY_MAX_ID3                0x3CC
#define JUNOR2_LATENCY_MAX_ID4                0x3D0
#define JUNOR2_LATENCY_MAX_ID5                0x3D4
#define JUNOR2_LATENCY_MAX_ID6                0x3D8
#define JUNOR2_LATENCY_MAX_ID7                0x3DC
#define JUNOR2_LATENCY_MAX_ID8                0x3E0
#define JUNOR2_LATENCY_MAX_ID9                0x3E4
#define JUNOR2_LATENCY_MAX_ID10               0x3E8
#define JUNOR2_LATENCY_MAX_ID11               0x3EC
#define JUNOR2_LATENCY_MAX_ID12               0x3F0
#define JUNOR2_LATENCY_MAX_ID13               0x3F4
#define JUNOR2_LATENCY_MAX_ID14               0x3F8
#define JUNOR2_LATENCY_MAX_ID15               0x3FC

#define JUNOR2_ENABLE_BW_STAT_FLAG        0x1
#define JUNOR2_DISABLE_BW_STAT_FLAG       0x1

#define JUNOR2_START_APIU_BW_STAT(io) \
	aipu_write32(io, JUNOR2_WORK_STAT_START_REQ, JUNOR2_ENABLE_BW_STAT_FLAG)

#define JUNOR2_STOP_APIU_BW_STAT(io) \
	aipu_write32(io, JUNOR2_WORK_STAT_END_REQ, JUNOR2_DISABLE_BW_STAT_FLAG)

/* clock gating */
#define JUNOR2_CLK_GATING_REG_OFFSET          0x104
#define JUNOR2_CLK_GATING_ENABLE_FLAG         0x31
#define JUNOR2_CLK_GATING_DISABLE_FLAG        0x1

#define JUNOR2_ENABLE_AIPU_CLK_GATING(io) \
	aipu_write32(io, JUNOR2_CLK_GATING_REG_OFFSET, JUNOR2_CLK_GATING_ENABLE_FLAG)

#define JUNOR2_DISABLE_AIPU_CLK_GATING(io) \
	aipu_write32(io, JUNOR2_CLK_GATING_REG_OFFSET, JUNOR2_CLK_GATING_DISABLE_FLAG)

#define JUNOR2_IS_AIPU_CLK_GATED(io) \
	(aipu_read32(io, JUNOR2_CLK_GATING_REG_OFFSET) == JUNOR2_CLK_GATING_ENABLE_FLAG)

/* logical reset */
#define JUNOR2_RESET_REG_OFFSET               0x104
#define JUNOR2_RESET_AIPU_FLAG                0x0
#define JUNOR2_RELEASE_AIPU_FLAG              0x1

#define JUNOR2_LOGIC_RESET_APIU(io) \
	aipu_write32(io, JUNOR2_RESET_REG_OFFSET, JUNOR2_RESET_AIPU_FLAG)

#define JUNOR2_RELEASE_APIU(io) \
	aipu_write32(io, JUNOR2_RESET_REG_OFFSET, JUNOR2_RELEASE_AIPU_FLAG)

#define JUNOR2_IS_APIU_ON(io) \
	(aipu_read32(io, JUNOR2_RESET_REG_OFFSET) == JUNOR2_RELEASE_AIPU_FLAG)

#define JUNOR2_RESET_APIU(io) do { \
	JUNOR2_LOGIC_RESET_APIU(io); \
	JUNOR2_RELEASE_APIU(io); \
	} while (0)

/* multicore interrupts */
#define JUNOR2_MULTICORE_INTR_OFFSET         0x500
#define JUNOR2_MULTICORE_INTR_HIS_OFFSET     0x504
#define JUNOR2_MULTICORE_INTR_CLEAR_OFFSET   0x508

#define JUNOR2_IS_CORE_IRQ(io, id) \
	(aipu_read32((io), JUNOR2_MULTICORE_INTR_OFFSET) & (1 << (id)))

#define JUNOR2_MULTICORE_IRQ(io) \
	(aipu_read32((io), JUNOR2_MULTICORE_INTR_OFFSET))

#endif /* __JUNOR2_H__ */
