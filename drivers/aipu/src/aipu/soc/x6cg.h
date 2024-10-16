/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

/**
 * @file x6cg.h
 * Header of the Xilinx 6CG SoC registers & operations
 */

#ifndef __X6CG_H__
#define __X6CG_H__

#include "aipu_io.h"

/* logical reset */
#define X6CG_RESET_REG_OFFSET   0x0
#define X6CG_RESET_AIPU_FLAG    0x0
#define X6CG_RELEASE_AIPU_FLAG  0x1

#define X6CG_LOGIC_RESET_APIU(io) \
	aipu_write32(io, X6CG_RESET_REG_OFFSET, X6CG_RESET_AIPU_FLAG)

#define X6CG_RELEASE_APIU(io) \
	aipu_write32(io, X6CG_RESET_REG_OFFSET, X6CG_RELEASE_AIPU_FLAG)

#define X6CG_IS_APIU_ON(io) \
	(aipu_read32(io, X6CG_RESET_REG_OFFSET) == X6CG_RELEASE_AIPU_FLAG)

#define X6CG_RESET_APIU(io) do { \
	X6CG_LOGIC_RESET_APIU(io); \
	X6CG_RELEASE_APIU(io); \
	} while (0)

/* multicore interrupts */
#define X6CG_MULTICORE_INTR_OFFSET         0x500
#define X6CG_MULTICORE_INTR_HIS_OFFSET     0x504
#define X6CG_MULTICORE_INTR_CLEAR_OFFSET   0x508

#define X6CG_IS_CORE_IRQ(io, id) \
	(aipu_read32((io), X6CG_MULTICORE_INTR_OFFSET) & (1 << (id)))

#endif /* __X6CG_H__ */
